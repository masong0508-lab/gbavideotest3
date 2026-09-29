#!/usr/bin/env python3
"""
make_finale.py - encodes the idle-menu easter-egg clip ("finale") into fin_frames.bin / fin_audio.bin.

  python3 tools/make_finale.py <video.mp4> [--out-check DIR]

Format (must match finale.h / main.c):
  Screen  : Mode 5 (160x128 15-bit direct colour, NO palette), picture 160x92, scaled 1.5x by BG2 to 240x138.
  Rate    : 30 fps (one frame per 2 vblanks -> perfectly even cadence, no 3:2 judder).
  Frames  : 23 x 40 blocks of 4x4 px, 8 bytes each = 7360 bytes per frame, frames back to back:
              u16 c0, u16 c1      15-bit BGR555 endpoints (bit 15 = 0)
              u32 idx             16 x 2-bit selectors, pixel n at bits 2n..2n+1, row-major
            palette of a block: c0, (2*c0+c1)/3, (c0+2*c1)/3, c1 per 5-bit channel, integer /3 = (x*683)>>11
  Audio   : 4-bit ADPCM exactly like the other clips (gbatool.encode_audio format), 2 vblanks per chunk.
"""
import argparse, os, subprocess, sys, numpy as np
from PIL import Image
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gbatool as G

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
W, H = 160, 92
BW, BH = W // 4, H // 4          # 40 x 23
FPS = 30

def div3(x): return (x * 683) >> 11

def palette_of(c0, c1):
    """c0,c1: int arrays (...,) of BGR555 -> (...,4,3) int RGB555 channels, exactly as the GBA decoder."""
    def split(c): return np.stack([c & 31, (c >> 5) & 31, (c >> 10) & 31], -1)
    a, b = split(c0), split(c1)
    return np.stack([a, div3(2 * a + b), div3(a + 2 * b), b], -2)

def pack(rgb): return (rgb[..., 0] | (rgb[..., 1] << 5) | (rgb[..., 2] << 10)).astype(np.int64)

def encode_frame(img):
    """img: (H,W,3) uint8 -> bytes (BW*BH*8). Also returns the decoded RGB555 image for checking."""
    t = img.astype(np.float64) / 255.0 * 31.0                      # target in 5-bit units
    blocks = t.reshape(BH, 4, BW, 4, 3).transpose(0, 2, 1, 3, 4).reshape(-1, 16, 3)   # (N,16,3)
    N = blocks.shape[0]
    mean = blocks.mean(1, keepdims=True)
    d = blocks - mean
    cov = np.einsum('npi,npj->nij', d, d)
    w, v = np.linalg.eigh(cov)
    axis = v[:, :, -1]                                              # principal axis (N,3)
    proj = np.einsum('npi,ni->np', d, axis)
    lo = mean[:, 0, :] + axis * proj.min(1, keepdims=True)
    hi = mean[:, 0, :] + axis * proj.max(1, keepdims=True)
    best_err = np.full(N, 1e18); best_c0 = np.zeros(N, np.int64); best_c1 = np.zeros(N, np.int64)
    best_idx = np.zeros((N, 16), np.int64)
    for it in range(6):
        q0 = np.clip(np.rint(lo), 0, 31).astype(np.int64); q1 = np.clip(np.rint(hi), 0, 31).astype(np.int64)
        c0, c1 = pack(q0), pack(q1)
        pal = palette_of(c0, c1).astype(np.float64)                 # (N,4,3)
        e = ((blocks[:, :, None, :] - pal[:, None, :, :]) ** 2).sum(-1)   # (N,16,4)
        idx = e.argmin(-1); err = e.min(-1).sum(-1)
        better = err < best_err
        best_err = np.where(better, err, best_err)
        best_c0 = np.where(better, c0, best_c0); best_c1 = np.where(better, c1, best_c1)
        best_idx = np.where(better[:, None], idx, best_idx)
        # least-squares update of the endpoints given the current selectors
        w1 = idx / 3.0; w0 = 1.0 - w1
        a00 = (w0 * w0).sum(1); a01 = (w0 * w1).sum(1); a11 = (w1 * w1).sum(1)
        det = a00 * a11 - a01 * a01
        ok = det > 1e-6
        b0 = np.einsum('np,npc->nc', w0, blocks); b1 = np.einsum('np,npc->nc', w1, blocks)
        sd = np.where(ok, det, 1.0)[:, None]
        nlo = (a11[:, None] * b0 - a01[:, None] * b1) / sd
        nhi = (a00[:, None] * b1 - a01[:, None] * b0) / sd
        lo = np.where(ok[:, None], nlo, lo); hi = np.where(ok[:, None], nhi, hi)
    # pack selectors
    sel = (best_idx.astype(np.uint64) << (2 * np.arange(16, dtype=np.uint64))).sum(1).astype(np.uint32)
    rec = np.zeros((N, 8), np.uint8)
    rec[:, 0] = best_c0 & 255; rec[:, 1] = best_c0 >> 8
    rec[:, 2] = best_c1 & 255; rec[:, 3] = best_c1 >> 8
    for k in range(4): rec[:, 4 + k] = (sel >> (8 * k)) & 255
    return rec.tobytes()

def decode_frame(data):
    """Reference decoder -> (H,W,3) uint8 (5-bit channels expanded), mirrors finale.h."""
    rec = np.frombuffer(data, np.uint8).reshape(-1, 8)
    c0 = rec[:, 0].astype(np.int64) | (rec[:, 1].astype(np.int64) << 8)
    c1 = rec[:, 2].astype(np.int64) | (rec[:, 3].astype(np.int64) << 8)
    sel = (rec[:, 4].astype(np.uint64) | (rec[:, 5].astype(np.uint64) << 8) |
           (rec[:, 6].astype(np.uint64) << 16) | (rec[:, 7].astype(np.uint64) << 24))
    pal = palette_of(c0, c1)
    idx = ((sel[:, None] >> (2 * np.arange(16, dtype=np.uint64))) & 3).astype(np.int64)
    px = np.take_along_axis(pal, idx[:, :, None].repeat(3, 2), 1)   # (N,16,3)
    img = px.reshape(BH, BW, 4, 4, 3).transpose(0, 2, 1, 3, 4).reshape(H, W, 3)
    return img

def load_frames(path, tmp):
    os.makedirs(tmp, exist_ok=True)
    for f in os.listdir(tmp): os.remove(os.path.join(tmp, f))
    vf = (f"minterpolate=fps={FPS}:mi_mode=mci:mc_mode=aobmc:vsbmc=1,"
          f"scale={W}:90:flags=lanczos,pad={W}:{H}:0:1:black")
    r = subprocess.run(['ffmpeg', '-y', '-v', 'error', '-i', path, '-vf', vf, '-vsync', '0',
                        os.path.join(tmp, 'f%03d.png')])
    if r.returncode: sys.exit('ffmpeg failed')
    fs = sorted(os.listdir(tmp))
    return [np.asarray(Image.open(os.path.join(tmp, f)).convert('RGB')) for f in fs]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('input'); ap.add_argument('--gain', type=float, default=1.0)
    ap.add_argument('--out-check', default=None, help='write decoded PNGs here for eyeballing')
    a = ap.parse_args()
    frames = load_frames(a.input, '/tmp/fin_png')
    pcm = G.ffmpeg_audio(a.input, a.gain)
    # audio decides the length: 2 vblanks per chunk, 1 frame per 2 vblanks -> 1 frame per chunk
    nchunks = max(1, -(-len(pcm) // G.SAMPLES_PER_CHUNK))
    while len(frames) < nchunks: frames.append(frames[-1])         # hold the last picture
    frames = frames[:nchunks]
    out = bytearray(); psnr = []
    for i, fr in enumerate(frames):
        d = encode_frame(fr); out += d
        if a.out_check:
            os.makedirs(a.out_check, exist_ok=True)
            dec = decode_frame(d).astype(np.float64)
            dec8 = np.clip(np.rint(dec * 255 / 31), 0, 255).astype(np.uint8)
            Image.fromarray(dec8).save(os.path.join(a.out_check, f'd{i:03d}.png'))
            mse = ((dec8.astype(float) - fr.astype(float)) ** 2).mean()
            psnr.append(10 * np.log10(255 ** 2 / max(mse, 1e-9)))
    pad = np.zeros(nchunks * G.SAMPLES_PER_CHUNK, np.int16); pad[:len(pcm)] = pcm[:len(pad)]
    ab = G.adpcm_encode(pad)
    open(os.path.join(ROOT, 'fin_frames.bin'), 'wb').write(out)
    open(os.path.join(ROOT, 'fin_audio.bin'), 'wb').write(ab)
    print(f'{len(frames)} frames @ {FPS} fps, {len(out):,} B video + {len(ab):,} B audio = {len(out)+len(ab):,} B')
    if psnr: print(f'mean PSNR vs source {np.mean(psnr):.1f} dB')

if __name__ == '__main__':
    main()
