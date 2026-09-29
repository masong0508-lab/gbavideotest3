#!/usr/bin/env python3
"""
make_egg_sprite.py - turn the face PNG into the GBA sprite data used by egg.h

    python3 tools/make_egg_sprite.py face.png

Writes (next to main.c):
    egg_tiles.bin   8192 bytes : 4 quadrant sprites (TL,TR,BL,BR), each 64x64, 4bpp, 8x8 tiles row-major
    egg_pal.bin       32 bytes : 16 x BGR555 (index 0 = transparent, 1..15 = face colours)
    egg_preview.png            : what the GBA will actually show (128x128 stored, and un-squashed)

The face is squashed into a 128x128 texture (the hardware can stretch each axis
separately, so the sprite is un-squashed again at run time). Needs: pillow, numpy.
"""
import os, sys
import numpy as np
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEX = 128

def main(src):
    im = Image.open(src).convert("RGBA")
    a = np.array(im)
    ys, xs = np.where(a[..., 3] > 16)
    im = im.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    print("cropped face: %dx%d  (width/height = %.3f)" % (im.width, im.height, im.width / im.height))

    # premultiplied down-scale so the edge doesn't pick up halo colours
    f = np.array(im).astype(np.float32)
    al = f[..., 3:4] / 255.0
    pm = Image.fromarray(np.dstack([f[..., :3] * al, al * 255.0]).astype(np.uint8), "RGBA")
    pm = pm.resize((TEX, TEX), Image.LANCZOS)
    p = np.array(pm).astype(np.float32)
    alpha = p[..., 3] / 255.0
    rgb = np.where(alpha[..., None] > 0.01, p[..., :3] / np.maximum(alpha[..., None], 0.01), 0)
    rgb = np.clip(rgb, 0, 255)
    opaque = alpha >= 0.5

    # punch the contrast a little: 15 colours need it
    rgb = np.clip((rgb - 128) * 1.12 + 128, 0, 255)
    fill = rgb[opaque].mean(axis=0)
    rgb[~opaque] = fill
    q = Image.fromarray(rgb.astype(np.uint8), "RGB").quantize(
        colors=15, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
    idx = np.array(q).astype(np.uint8) + 1          # 1..15
    idx[~opaque] = 0                                 # 0 = transparent
    pal = np.array(q.getpalette()[:45], dtype=np.uint8).reshape(-1, 3)
    pal15 = np.zeros((16, 3), np.uint8)
    pal15[1:1 + len(pal)] = pal[:15]

    out = bytearray()
    for qy in (0, 1):
        for qx in (0, 1):
            quad = idx[qy * 64:(qy + 1) * 64, qx * 64:(qx + 1) * 64]
            for ty in range(8):
                for tx in range(8):
                    tile = quad[ty * 8:(ty + 1) * 8, tx * 8:(tx + 1) * 8]
                    for r in range(8):
                        for k in range(0, 8, 2):
                            out.append(int(tile[r, k]) | (int(tile[r, k + 1]) << 4))
    assert len(out) == 8192
    open(os.path.join(ROOT, "egg_tiles.bin"), "wb").write(out)

    pb = bytearray()
    for r, g, b in pal15:
        v = (int(r) >> 3) | ((int(g) >> 3) << 5) | ((int(b) >> 3) << 10)
        pb += v.to_bytes(2, "little")
    pb[0:2] = b"\0\0"
    open(os.path.join(ROOT, "egg_pal.bin"), "wb").write(pb)

    # preview
    prev = np.zeros((TEX, TEX, 4), np.uint8)
    for i in range(1, 16):
        prev[idx == i] = [*(pal15[i] >> 3 << 3), 255]
    pim = Image.fromarray(prev, "RGBA")
    nat = pim.resize((int(TEX * im.width / im.height * 2), TEX * 2), Image.NEAREST)
    sheet = Image.new("RGBA", (TEX * 2 + nat.width + 30, TEX * 2 + 20), (40, 40, 40, 255))
    sheet.alpha_composite(pim.resize((TEX * 2, TEX * 2), Image.NEAREST), (10, 10))
    sheet.alpha_composite(nat, (TEX * 2 + 20, 10))
    sheet.save(os.path.join(ROOT, "egg_preview.png"))
    print("wrote egg_tiles.bin (8192 B), egg_pal.bin (32 B), egg_preview.png")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    main(sys.argv[1])
