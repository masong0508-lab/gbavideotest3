#!/usr/bin/env python3
"""
gbatool.py - asset converter for the GBA video-player template.

Sub-commands
  video    <input.mp4> [--slot main|secret]   video+audio -> frames/palette/audio .bin
  bg       <image.png> --slot menu|secret     any image   -> 240x160 background .bin + palette
  placeholders                                regenerate the built-in test assets

Needs: python3, Pillow, numpy, and ffmpeg on PATH (ffmpeg only for `video`).

Formats (must match main.c):
  frames  : 120x68, 8-bit palette indices, 5 fps, frames stored back to back
  palette : 256 x u16 little-endian, GBA BGR555   (video palette is shared by all frames)
  audio   : 4-bit ADPCM, mono, ~9078.5 Hz, 152 bytes per chunk (304 samples), low nibble first
  bg      : 240x160 8-bit indices (38400 bytes); palette entry 254 = black, 255 = white
            (the menu font is drawn with those two colours, so they are reserved)
"""
import argparse, math, os, struct, subprocess, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VID_W, VID_H = 120, 68
FRAME_BYTES = VID_W * VID_H
FPS_NUM, FPS_DEN = 5486, 65536          # frame index = vblank * 5486 / 65536  (~5 fps at 59.7275 Hz)
VBLANK_HZ = 16777216 / 280896
AUDIO_HZ = 16777216 / 1848              # 9078.5 Hz (timer period 1848)
SAMPLES_PER_CHUNK = 304
CHUNK_BYTES = SAMPLES_PER_CHUNK // 2
ROM_LIMIT = 32 * 1024 * 1024

STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7847,8631,9494,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
IDX = [-1, -1, -1, -1, 2, 4, 6, 8]

# ---------------------------------------------------------------- ADPCM
def adpcm_encode(samples):
    """samples: iterable of int16. Mirrors decode_chunk() in main.c exactly
    (including the 'pred -= pred >> 9' leak) so encoder and player never drift."""
    pred = 0; sidx = 0
    out = bytearray(); low = None
    for s in samples:
        s = int(s)
        step = STEP[sidx]
        d = s - pred; nib = 0
        if d < 0: nib = 8; d = -d
        st = step
        if d >= st: nib |= 4; d -= st
        st >>= 1
        if d >= st: nib |= 2; d -= st
        st >>= 1
        if d >= st: nib |= 1
        diff = step >> 3
        if nib & 1: diff += step >> 2
        if nib & 2: diff += step >> 1
        if nib & 4: diff += step
        pred = pred - diff if nib & 8 else pred + diff
        pred -= pred >> 9
        pred = max(-32768, min(32767, pred))
        sidx = max(0, min(88, sidx + IDX[nib & 7]))
        if low is None: low = nib
        else: out.append(low | (nib << 4)); low = None
    return bytes(out)

def adpcm_decode(data):
    """Reference decoder (same as the C one). Returns signed 8-bit samples."""
    pred = 0; sidx = 0; out = []
    for b in data:
        for nib in (b & 15, b >> 4):
            step = STEP[sidx]; diff = step >> 3
            if nib & 1: diff += step >> 2
            if nib & 2: diff += step >> 1
            if nib & 4: diff += step
            pred = pred - diff if nib & 8 else pred + diff
            pred -= pred >> 9
            pred = max(-32768, min(32767, pred))
            sidx = max(0, min(88, sidx + IDX[nib & 7]))
            out.append(pred >> 8)
    return out

def chunks_for_frames(nframes):
    vblanks = math.ceil(nframes * FPS_DEN / FPS_NUM)
    return max(1, math.ceil(vblanks / 2))

def encode_audio(pcm16, nframes):
    """pcm16: int16 mono @ AUDIO_HZ. Padded/trimmed to exactly what the video needs."""
    want = chunks_for_frames(nframes) * SAMPLES_PER_CHUNK
    pcm = np.zeros(want, dtype=np.int16)
    n = min(want, len(pcm16)); pcm[:n] = pcm16[:n]
    return adpcm_encode(pcm)

# ---------------------------------------------------------------- palettes / frames
def bgr555(rgb_triplets):
    words = []
    for r, g, b in rgb_triplets:
        words.append((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10))
    words += [0] * (256 - len(words))
    return struct.pack('<256H', *words[:256])

def pal_rgb(img_p, count=256):
    flat = img_p.getpalette() or []
    flat = flat + [0] * (768 - len(flat))
    return [tuple(flat[i*3:i*3+3]) for i in range(count)]

def encode_frames(frames, dither):
    """frames: list of 120x68 RGB PIL images -> (frames_bytes, palette_bytes). One shared palette."""
    step = max(1, len(frames) // 64)
    sample = frames[::step][:64]
    cols = 8
    rows = math.ceil(len(sample) / cols)
    sheet = Image.new('RGB', (cols * VID_W, rows * VID_H))
    for i, f in enumerate(sample):
        sheet.paste(f, ((i % cols) * VID_W, (i // cols) * VID_H))
    pal_img = sheet.quantize(colors=256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    d = Image.Dither.FLOYDSTEINBERG if dither else Image.Dither.NONE
    buf = bytearray()
    for f in frames:
        q = f.quantize(palette=pal_img, dither=d)
        buf += np.asarray(q, dtype=np.uint8).tobytes()
    return bytes(buf), bgr555(pal_rgb(pal_img))

def encode_bg(img, dither):
    img = img.convert('RGB').resize((240, 160), Image.LANCZOS)
    q = img.quantize(colors=254, method=Image.Quantize.MEDIANCUT,
                     dither=Image.Dither.FLOYDSTEINBERG if dither else Image.Dither.NONE)
    rgb = pal_rgb(q, 254) + [(0, 0, 0), (255, 255, 255)]      # 254 = black, 255 = white
    data = np.asarray(q, dtype=np.uint8)
    assert data.max() < 254
    return data.tobytes(), bgr555(rgb)

def write(name, data):
    with open(os.path.join(ROOT, name), 'wb') as f: f.write(data)
    print(f'  wrote {name:16s} {len(data):>10,} bytes')

# ---------------------------------------------------------------- ffmpeg input
def ffmpeg_frames(path, stretch):
    if stretch: vf = f'fps=5,scale={VID_W}:{VID_H}:flags=lanczos'
    else:       vf = (f'fps=5,scale={VID_W}:{VID_H}:force_original_aspect_ratio=decrease:flags=lanczos,'
                      f'pad={VID_W}:{VID_H}:(ow-iw)/2:(oh-ih)/2:black')
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-an', '-vf', vf,
                          '-f', 'rawvideo', '-pix_fmt', 'rgb24', '-'],
                         check=True, stdout=subprocess.PIPE).stdout
    n = len(raw) // (VID_W * VID_H * 3)
    arr = np.frombuffer(raw[:n * VID_W * VID_H * 3], dtype=np.uint8).reshape(n, VID_H, VID_W, 3)
    return [Image.fromarray(a) for a in arr]

def ffmpeg_audio(path, gain):
    r = subprocess.run(['ffmpeg', '-v', 'error', '-i', path, '-vn', '-ac', '1', '-ar', str(round(AUDIO_HZ)),
                        '-af', f'volume={gain}', '-f', 's16le', '-'], stdout=subprocess.PIPE)
    return np.frombuffer(r.stdout, dtype='<i2') if r.returncode == 0 else np.zeros(0, dtype=np.int16)

# ---------------------------------------------------------------- commands
SLOTS = {'main': ('frames.bin', 'palette.bin', 'audio.bin'),
         'secret': ('vid2_frames.bin', 'vid2_palette.bin', 'vid2_audio.bin')}

def build_video(frames, pcm, slot, dither):
    fname, pname, aname = SLOTS[slot]
    fb, pb = encode_frames(frames, dither)
    ab = encode_audio(pcm, len(frames))
    write(fname, fb); write(pname, pb); write(aname, ab)
    secs = len(frames) / 5
    print(f'  {len(frames)} frames = {secs:.1f} s ({secs/60:.1f} min), {len(ab)//CHUNK_BYTES} audio chunks')

def cmd_video(a):
    frames = ffmpeg_frames(a.input, a.stretch)
    if not frames: sys.exit('ffmpeg produced no frames - is that a valid video file?')
    build_video(frames, ffmpeg_audio(a.input, a.gain), a.slot, a.dither)
    check_size()

def cmd_bg(a):
    d, p = encode_bg(Image.open(a.input), a.dither)
    write(f'{a.slot}_bg.bin', d); write(f'{a.slot}_pal.bin', p)

def check_size():
    tot = sum(os.path.getsize(os.path.join(ROOT, n)) for n in os.listdir(ROOT) if n.endswith('.bin'))
    print(f'  total embedded data: {tot/1048576:.2f} MB of the 32 MB GBA ROM limit')
    if tot > ROM_LIMIT - 200_000:
        print('  WARNING: this will not fit in a GBA ROM - shorten the video(s).')

# ---------------------------------------------------------------- placeholders
def font(size):
    try: return ImageFont.load_default(size)
    except TypeError: return ImageFont.load_default()

def gradient(w, h, top, bottom):
    t = np.linspace(0, 1, h)[:, None, None]
    arr = (np.array(top) * (1 - t) + np.array(bottom) * t) * np.ones((h, w, 3))
    return Image.fromarray(arr.astype(np.uint8))

def make_menu_png():
    img = gradient(240, 160, (24, 28, 60), (70, 40, 90))
    d = ImageDraw.Draw(img)
    for i in range(3):                                       # button slots; highlight sprite is 64x32 at (160, 15+23i-16)
        cy = 15 + 23 * i
        d.rounded_rectangle((162, cy - 13, 222, cy + 13), radius=10, fill=(20, 20, 40), outline=(120, 120, 170))
    d.text((12, 60), 'MENU BACKGROUND', fill=(200, 200, 230), font=font(14))
    d.text((12, 80), 'replace assets_src/menu_bg.png', fill=(150, 150, 190), font=font(9))
    return img

def make_secret_png():
    img = gradient(240, 160, (10, 50, 40), (5, 20, 25))
    d = ImageDraw.Draw(img)
    d.text((120, 40), 'SECRET BACKGROUND', fill=(160, 230, 200), font=font(14), anchor='mm')
    d.text((120, 120), 'replace assets_src/secret_bg.png', fill=(110, 170, 150), font=font(9), anchor='mm')
    return img

def test_frames(n, hue0, label):
    frames = []
    f = font(22)
    for i in range(n):
        top = tuple(int(c) for c in np.array(hue0) * (0.6 + 0.4 * math.sin(i / n * 2 * math.pi)))
        img = gradient(VID_W, VID_H, top, (10, 10, 20))
        d = ImageDraw.Draw(img)
        x = 4 + (VID_W - 24) * i // max(1, n - 1)
        d.rectangle((x, 46, x + 16, 62), fill=(255, 255, 255))
        d.text((VID_W // 2, 16), label, fill=(255, 255, 255), font=font(11), anchor='mm')
        d.text((VID_W // 2, 34), f'{i:03d}', fill=(255, 220, 120), font=f, anchor='mm')
        frames.append(img)
    return frames

def test_audio(nframes, hz):
    n = chunks_for_frames(nframes) * SAMPLES_PER_CHUNK
    t = np.arange(n) / AUDIO_HZ
    env = ((t % 1.0) < 0.12) * 0.25                          # short quiet beep at the start of every second
    return (np.sin(2 * math.pi * hz * t) * env * 32767).astype(np.int16)

def cmd_placeholders(a):
    print('placeholder assets:')
    os.makedirs(os.path.join(ROOT, 'assets_src'), exist_ok=True)
    for name, img in (('menu', make_menu_png()), ('secret', make_secret_png())):
        img.save(os.path.join(ROOT, 'assets_src', f'{name}_bg.png'))
        d, p = encode_bg(img, False)
        write(f'{name}_bg.bin', d); write(f'{name}_pal.bin', p)
    build_video(test_frames(30, (200, 60, 60), 'MAIN VIDEO'), test_audio(30, 440), 'main', False)
    build_video(test_frames(15, (60, 160, 90), 'SECRET VIDEO'), test_audio(15, 660), 'secret', False)
    check_size()

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    v = sub.add_parser('video'); v.add_argument('input')
    v.add_argument('--slot', choices=SLOTS, default='main')
    v.add_argument('--stretch', action='store_true', help='stretch to 120x68 instead of letterboxing')
    v.add_argument('--dither', action='store_true', help='Floyd-Steinberg dithering (bigger-looking noise, smoother gradients)')
    v.add_argument('--gain', type=float, default=1.0, help='audio volume multiplier (default 1.0)')
    v.set_defaults(fn=cmd_video)
    b = sub.add_parser('bg'); b.add_argument('input')
    b.add_argument('--slot', choices=['menu', 'secret'], required=True)
    b.add_argument('--dither', action='store_true')
    b.set_defaults(fn=cmd_bg)
    p = sub.add_parser('placeholders'); p.set_defaults(fn=cmd_placeholders)
    a = ap.parse_args(); a.fn(a)

if __name__ == '__main__':
    main()
