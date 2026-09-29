/* finale.h - decoder for the idle-menu easter-egg clip (see tools/make_finale.py for the format).
   Pure function of (source bytes -> 160x92 pixels), so it can be unit-tested on a PC. */
#define FIN_W        160
#define FIN_H        92
#define FIN_BW       (FIN_W / 4)                 /* 40 blocks across */
#define FIN_BH       (FIN_H / 4)                 /* 23 blocks down   */
#define FIN_FRAME_BYTES (FIN_BW * FIN_BH * 8)    /* 7360 */

/* dst = start of a Mode 5 page (160 px per row, 16-bit BGR555). src must be 4-byte aligned. */
static void fin_decode_frame(const u8 *src, volatile u32 *dst) {
    const u32 *s = (const u32 *)src;
    for (int by = 0; by < FIN_BH; by++) {
        volatile u32 *row = dst + by * 4 * (FIN_W / 2);
        for (int bx = 0; bx < FIN_BW; bx++, s += 2) {
            u32 ends = s[0], sel = s[1];
            u32 c0 = ends & 0xFFFF, c1 = ends >> 16;
            u32 r0 = c0 & 31, g0 = (c0 >> 5) & 31, b0 = (c0 >> 10) & 31;
            u32 r1 = c1 & 31, g1 = (c1 >> 5) & 31, b1 = (c1 >> 10) & 31;
            u32 p[4];
            p[0] = c0;
            p[1] = ((2 * r0 + r1) * 683 >> 11) | (((2 * g0 + g1) * 683 >> 11) << 5) | (((2 * b0 + b1) * 683 >> 11) << 10);
            p[2] = ((r0 + 2 * r1) * 683 >> 11) | (((g0 + 2 * g1) * 683 >> 11) << 5) | (((b0 + 2 * b1) * 683 >> 11) << 10);
            p[3] = c1;
            volatile u32 *d = row + bx * 2;
            for (int y = 0; y < 4; y++, d += FIN_W / 2) {
                u32 q = sel >> (y * 8);
                d[0] = p[q & 3]        | (p[(q >> 2) & 3] << 16);
                d[1] = p[(q >> 4) & 3] | (p[(q >> 6) & 3] << 16);
            }
        }
    }
}
