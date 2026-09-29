/* egg.h - scaling "face" sprite for the secret screen (U U D D L R L R B A)
 *
 * Adds ONE image asset (egg_tiles.bin + egg_pal.bin, made by tools/make_egg_sprite.py)
 * and the keyframe motion that plays over the last ~6.5 s of the secret music:
 *
 *   A  peeks up from the bottom, bobs, POPs huge, shrinks and spins away
 *   B  drops in upside-down from the top, then cuts out
 *   C  rises again, zooms to fill the whole screen, and when the music ends the menu is
 *      swapped in underneath it and the face slides off to the right
 *
 * How it works: the face is a 128x128 4bpp texture stored as four 64x64 sprites
 * (quadrants). All four share one affine matrix and are placed so they form one big
 * rotating / scaling picture. A single GBA sprite can never show more than 128x128
 * pixels, so four of them are what lets the face grow past the screen (max 2x per
 * axis = 256x256). Width and height are scaled independently (the picture is stored
 * squashed and stretched back to its real shape here).
 *
 * Everything is in keyframes at the bottom of this file: time, centre x/y, width,
 * height (screen pixels) and angle (degrees). Edit them freely; time is seconds
 * measured on your reference video (music ends at EGG_END_S).
 *
 * OAM entries 4..7 + affine group 1, OBJ tiles 544..799, OBJ palette bank 1 are used;
 * the menu highlight (sprite 0, tiles 512..543, palette entries 1-2) is untouched.
 */
#ifndef EGG_H
#define EGG_H

extern const u32 egg_tiles[2048];   /* 4 x (64x64 @ 4bpp) = 8192 bytes */
extern const u16 egg_pal[16];

typedef signed short s16;

#define EGG_TILE0    544            /* first OBJ tile used (512..543 = menu highlight) */
#define EGG_SPR0     4              /* OAM entries 4..7 */
#define EGG_GROUP    1              /* affine group 1 = 4th halfword of OAM entries 4..7 */
#define EGG_PALBANK  1              /* OBJ palette entries 16..31 */
#define EGG_DISP     ((1 << 6) | (1 << 12))   /* DISPCNT: 1D OBJ mapping + OBJ enabled */

/* ---- sine / cosine, 256 steps per turn, 8.8 fixed point ---- */
static const s16 egg_qsin[65] = {
    0,6,13,19,25,31,38,44,50,56,62,68,74,
    80,86,92,98,104,109,115,121,126,132,137,142,147,
    152,157,162,167,172,177,181,185,190,194,198,202,206,
    209,213,216,220,223,226,229,231,234,237,239,241,243,
    245,247,248,250,251,252,253,254,255,255,256,256,256,
};
static int egg_sin(int a) {
    a &= 255;
    if (a < 64)  return egg_qsin[a];
    if (a < 128) return egg_qsin[128 - a];
    if (a < 192) return -egg_qsin[a - 128];
    return -egg_qsin[256 - a];
}
static int egg_cos(int a) { return egg_sin(a + 64); }

/* ---- keyframes ---- */
typedef struct {
    s16 t;              /* vblanks relative to the moment the music ends (negative = before) */
    s16 x, y;           /* centre of the picture on screen */
    s16 w, h;           /* size on screen in pixels (each 4..256) */
    s16 a;              /* rotation, 256 = full turn (use EGG_DEG) */
    u8  ease;           /* 1 = smooth-step from this key to the next, 0 = linear */
} egg_key;
typedef struct { const egg_key *k; int n; } egg_seg;

#define EGG_END_S   11.0                                   /* music end on the reference video */
#define EGG_T(s)    ((int)(((s) - EGG_END_S) * 59.7275))   /* reference seconds -> vblanks */
#define EGG_DEG(d)  ((d) * 256 / 360)

/* A: peek from the bottom, bob, pop, shrink, spin away */
static const egg_key egg_segA[] = {
    { EGG_T(4.42), 120, 235,  62, 112, EGG_DEG(  0), 0 },
    { EGG_T(4.56), 120, 190,  62, 112, EGG_DEG(  0), 1 },
    { EGG_T(4.68), 120, 165,  62, 112, EGG_DEG(  0), 1 },
    { EGG_T(4.78), 120, 185,  62, 112, EGG_DEG(  0), 0 },
    { EGG_T(4.86), 120,  95, 200, 256, EGG_DEG(  0), 0 },
    { EGG_T(5.15), 120,  95, 200, 256, EGG_DEG(  0), 1 },
    { EGG_T(5.45), 121,  90, 150, 210, EGG_DEG(  0), 1 },
    { EGG_T(5.80), 124,  92,  88, 150, EGG_DEG( -6), 0 },
    { EGG_T(5.92), 100, 100,  60, 105, EGG_DEG(-35), 0 },
    { EGG_T(6.03),  92,  98,  22,  38, EGG_DEG(-70), 0 },
    { EGG_T(6.12),  90,  98,   6,  10, EGG_DEG(-90), 0 },
};
/* B: upside-down drop from the top */
static const egg_key egg_segB[] = {
    { EGG_T(7.95), 120, -132, 132, 240, EGG_DEG(180), 0 },
    { EGG_T(8.10), 120,  -95, 132, 240, EGG_DEG(180), 0 },
    { EGG_T(8.30), 120,  -55, 132, 240, EGG_DEG(180), 0 },
    { EGG_T(8.55), 120,  -20, 132, 240, EGG_DEG(180), 0 },
    { EGG_T(8.90), 120,   10, 132, 240, EGG_DEG(180), 0 },
    { EGG_T(9.34), 120,   14, 132, 240, EGG_DEG(180), 0 },
};
/* C: rise, zoom to full screen, hold; music ends at t = 0, then slide off to the right */
static const egg_key egg_segC[] = {
    { EGG_T( 9.60), 120, 270, 110, 200, EGG_DEG(0), 0 },
    { EGG_T( 9.72), 120, 118, 110, 200, EGG_DEG(0), 0 },
    { EGG_T( 9.85), 120, 118, 118, 215, EGG_DEG(0), 0 },
    { EGG_T( 9.92), 120,  90, 256, 256, EGG_DEG(0), 0 },
    { EGG_T(10.00), 120, 100, 256, 256, EGG_DEG(0), 0 },
    { EGG_T(10.25), 120,  86, 256, 256, EGG_DEG(0), 0 },
    { EGG_T(10.40), 120,  80, 256, 256, EGG_DEG(0), 0 },
    { 0,            120,  80, 256, 256, EGG_DEG(0), 1 },     /* music ends: menu swapped in behind */
    { 15,           400,  80, 256, 256, EGG_DEG(0), 0 },     /* face slides off to the right */
};
static const egg_seg egg_segs[] = {
    { egg_segA, sizeof egg_segA / sizeof egg_segA[0] },
    { egg_segB, sizeof egg_segB / sizeof egg_segB[0] },
    { egg_segC, sizeof egg_segC / sizeof egg_segC[0] },
};
#define EGG_SLIDE_TICKS 15          /* length of the slide-off after the music ends */
#define EGG_FADE_T0     EGG_T(9.85) /* secret picture fades to black behind the zoom ... */
#define EGG_FADE_T1     EGG_T(10.10)/* ... completely black by here */

/* ---- engine ---- */
static int egg_on;          /* 1 while any egg sprite may be visible */
static void egg_hide(void) {
    for (int i = 0; i < 4; i++) OAM[(EGG_SPR0 + i) * 4] = 0x200;
    egg_on = 0;
}

/* copy tiles + palette into OBJ VRAM (do it while the screen is black) */
static void egg_load(void) {
    for (int i = 0; i < 2048; i++) OBJ_TILES[(EGG_TILE0 - 512) * 8 + i] = egg_tiles[i];
    for (int i = 0; i < 16; i++) OBJ_PAL[EGG_PALBANK * 16 + i] = egg_pal[i];
    egg_hide();
}

static int egg_pose(int t, int *cx, int *cy, int *w, int *h, int *ang) {
    for (int s = 0; s < (int)(sizeof egg_segs / sizeof egg_segs[0]); s++) {
        const egg_key *k = egg_segs[s].k;
        int n = egg_segs[s].n;
        if (t < k[0].t || t > k[n - 1].t) continue;
        int i = 0;
        while (i < n - 2 && t >= k[i + 1].t) i++;
        int span = k[i + 1].t - k[i].t;
        int f = span > 0 ? ((t - k[i].t) * 256) / span : 256;
        if (f < 0) f = 0;
        if (f > 256) f = 256;
        if (k[i].ease) f = (((f * f) >> 8) * (768 - 2 * f)) >> 8;      /* smooth-step */
#define LERP(fld) (k[i].fld + (((k[i + 1].fld - k[i].fld) * f) >> 8))
        *cx = LERP(x); *cy = LERP(y); *w = LERP(w); *h = LERP(h); *ang = LERP(a);
#undef LERP
        return 1;
    }
    return 0;
}

static int egg_abs(int v) { return v < 0 ? -v : v; }

/* place the four quadrant sprites for one pose (or hide them) - call during vblank */
static void egg_update(int t) {
    int cx, cy, w, h, ang;
    if (!egg_pose(t, &cx, &cy, &w, &h, &ang)) {
        if (egg_on) { while (REG_VCOUNT < 160) {} egg_hide(); }   /* only touch OAM in vblank */
        return;
    }
    if (w < 4) w = 4;
    if (h < 4) h = 4;
    if (w > 256) w = 256;
    if (h > 256) h = 256;
    int c = egg_cos(ang), s = egg_sin(ang);
    /* screen -> texture matrix (8.8): inverse of rotate * scale(w/128, h/128) */
    u16 pa = (u16)(c * 128 / w), pb = (u16)(s * 128 / w);
    u16 pc = (u16)(-(s * 128 / h)), pd = (u16)(c * 128 / h);
    int ev = (egg_abs(c) * h + egg_abs(s) * w + 512) >> 10;    /* half height of a rotated quadrant */
    u16 a0[4], a1[4], a2[4];
    for (int k = 0; k < 4; k++) {
        int qx = (k & 1) ? 1 : -1, qy = (k & 2) ? 1 : -1;
        /* pull each quadrant ~0.5 px toward the middle so rounding can never leave a hairline gap */
        int ow = w > 8 ? w - 2 : w, oh = h > 8 ? h - 2 : h;
        int dx = (c * qx * ow - s * qy * oh + 512) >> 10;
        int dy = (s * qx * ow + c * qy * oh + 512) >> 10;
        int bx = cx + dx - 64, by = cy + dy - 64;              /* top-left of the 128x128 box */
        int ok = !(bx <= -128 || bx >= 240 || by <= -128 || by >= 160);
        /* OAM Y wraps at 256: hide a box whose opaque rows would wrap onto the other screen edge */
        if (ok && by > 128 && by >= 192 - ev) ok = 0;
        if (ok && by < -96 && by <= ev - 161) ok = 0;
        if (!ok) { a0[k] = 0x200; a1[k] = 0; a2[k] = 0; continue; }
        a0[k] = (u16)((by & 0xFF) | (1 << 8) | (1 << 9));                  /* rot/scale + double size, 4bpp, square */
        a1[k] = (u16)((bx & 0x1FF) | (EGG_GROUP << 9) | (3 << 14));        /* 64x64 */
        a2[k] = (u16)((EGG_TILE0 + 64 * k) | (EGG_PALBANK << 12));
    }
    while (REG_VCOUNT < 160) {}                                /* OAM is only safe to write in vblank */
    egg_on = 1;
    for (int k = 0; k < 4; k++) {
        OAM[(EGG_SPR0 + k) * 4 + 0] = a0[k];
        OAM[(EGG_SPR0 + k) * 4 + 1] = a1[k];
        OAM[(EGG_SPR0 + k) * 4 + 2] = a2[k];
    }
    OAM[EGG_GROUP * 16 + 3]  = pa;
    OAM[EGG_GROUP * 16 + 7]  = pb;
    OAM[EGG_GROUP * 16 + 11] = pc;
    OAM[EGG_GROUP * 16 + 15] = pd;
}

/* how bright the secret picture should be (FADE_N = full .. 0 = black) while the face zooms */
static int egg_fade_level(int t, int full) {
    if (t <= EGG_FADE_T0) return full;
    if (t >= EGG_FADE_T1) return 0;
    return full - full * (t - EGG_FADE_T0) / (EGG_FADE_T1 - EGG_FADE_T0);
}

#endif
