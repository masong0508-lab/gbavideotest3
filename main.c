/* GBA video player template: menu + main video + hidden second video.
   120x68 @ 5fps (2x scaled, Mode 4) + 4-bit ADPCM audio.
   All content lives in the .bin files; see README.md and tools/gbatool.py. */
typedef unsigned char  u8;
typedef signed char    s8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define REG16(a) (*(volatile u16*)(a))
#define REG32(a) (*(volatile u32*)(a))
#define REG_DISPCNT    REG16(0x04000000)
#define REG_DISPSTAT   REG16(0x04000004)
#define REG_SOUND1CNT_L REG16(0x04000060)
#define REG_SOUND1CNT_H REG16(0x04000062)
#define REG_SOUND1CNT_X REG16(0x04000064)
#define REG_SOUND2CNT_L REG16(0x04000068)
#define REG_SOUND2CNT_H REG16(0x0400006C)
#define REG_SOUND3CNT_L REG16(0x04000070)
#define REG_SOUND3CNT_H REG16(0x04000072)
#define REG_SOUND3CNT_X REG16(0x04000074)
#define REG_SOUND4CNT_L REG16(0x04000078)
#define REG_SOUND4CNT_H REG16(0x0400007C)
#define REG_SOUNDCNT_L  REG16(0x04000080)
#define REG_SOUNDCNT_H REG16(0x04000082)
#define REG_SOUNDCNT_X REG16(0x04000084)
#define REG_TM0CNT_L   REG16(0x04000100)
#define REG_TM0CNT_H   REG16(0x04000102)
#define REG_DMA1SAD    REG32(0x040000BC)
#define REG_DMA1DAD    REG32(0x040000C0)
#define REG_DMA1CNT    REG32(0x040000C4)
#define REG_IE         REG16(0x04000200)
#define REG_IF         REG16(0x04000202)
#define REG_IME        REG16(0x04000208)
#define REG_VCOUNT     REG16(0x04000006)
#define REG_BLDCNT     REG16(0x04000050)
#define REG_BLDALPHA   REG16(0x04000052)
#define REG_TM2CNT_L   REG16(0x04000108)
#define REG_TM2CNT_H   REG16(0x0400010A)
#define REG_TM3CNT_L   REG16(0x0400010C)
#define REG_TM3CNT_H   REG16(0x0400010E)
#define REG_BG2PA      REG16(0x04000020)
#define REG_BG2PB      REG16(0x04000022)
#define REG_BG2PC      REG16(0x04000024)
#define REG_BG2PD      REG16(0x04000026)
#define REG_BG2X       REG32(0x04000028)
#define REG_BG2Y       REG32(0x0400002C)
#define REG_KEYINPUT   REG16(0x04000130)
#define OAM         ((volatile u16*)0x07000000)
#define OBJ_PAL     ((volatile u16*)0x05000200)
#define OBJ_TILES   ((volatile u32*)0x06014000)
#define IRQ_VECTOR     REG32(0x03007FFC)
#define PALETTE     ((volatile u16*)0x05000000)
#define VRAM_PAGE0  ((volatile u16*)0x06000000)
#define VRAM_PAGE1  ((volatile u16*)0x0600A000)

#define VID_W 120
#define VID_H 68
#define Y_OFFSET 12                 /* (160 - 68*2) / 2 */
#define FRAME_BYTES (VID_W * VID_H)

/* Audio: 9078.5 Hz (timer period 1848 cycles). One 304-sample chunk plays
   for exactly 2 vblanks (1848 * 304 * 2 = 2 * 280896 cycles). */
#define SAMPLES_PER_CHUNK 304
#define NCHUNKS ((u32)(audio_end - audio_start) / (SAMPLES_PER_CHUNK / 2))   /* derived from audio.bin */

static const short step_tab[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7847,8631,9494,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const s8 idx_tab[8] = {-1,-1,-1,-1,2,4,6,8};

/* Everything is embedded in the ROM */
__asm__(
    ".section .rodata\n"
    ".balign 4\n"
    ".global frames_start\nframes_start:\n"
    ".incbin \"frames1.bin\"\n"          /* split so no single file exceeds GitHub's 25MB upload limit */
    ".incbin \"frames2.bin\"\n"
    ".global frames_end\nframes_end:\n"
    ".balign 4\n"
    ".global frames_idx_start\nframes_idx_start:\n"
    ".incbin \"frames_idx.bin\"\n"
    ".global frames_idx_end\nframes_idx_end:\n"
    ".balign 4\n"
    ".global palette_data\npalette_data:\n"
    ".incbin \"palette.bin\"\n"
    ".balign 4\n"
    ".global audio_start\naudio_start:\n"
    ".incbin \"audio.bin\"\n"
    ".global audio_end\naudio_end:\n"
    ".balign 4\n"
    ".global audio_state\naudio_state:\n"
    ".incbin \"audio_state.bin\"\n"
    ".balign 4\n"
    ".global menu_bg\nmenu_bg:\n"
    ".incbin \"menu_bg.bin\"\n"
    ".balign 4\n"
    ".global menu_pal\nmenu_pal:\n"
    ".incbin \"menu_pal.bin\"\n"
    ".balign 4\n"
    ".global secret_bg\nsecret_bg:\n"
    ".incbin \"secret_bg.bin\"\n"
    ".balign 4\n"
    ".global secret_pal\nsecret_pal:\n"
    ".incbin \"secret_pal.bin\"\n"
    ".balign 4\n"
    ".global vid2_frames_start\nvid2_frames_start:\n"
    ".incbin \"vid2_frames1.bin\"\n"
    ".incbin \"vid2_frames2.bin\"\n"
    ".global vid2_frames_end\nvid2_frames_end:\n"
    ".balign 4\n"
    ".global vid2_frames_idx_start\nvid2_frames_idx_start:\n"
    ".incbin \"vid2_frames_idx.bin\"\n"
    ".global vid2_frames_idx_end\nvid2_frames_idx_end:\n"
    ".balign 4\n"
    ".global vid2_palette_data\nvid2_palette_data:\n"
    ".incbin \"vid2_palette.bin\"\n"
    ".balign 4\n"
    ".global vid2_audio_start\nvid2_audio_start:\n"
    ".incbin \"vid2_audio.bin\"\n"
    ".global vid2_audio_end\nvid2_audio_end:\n"
    ".balign 4\n"
    ".global vid2_audio_state\nvid2_audio_state:\n"
    ".incbin \"vid2_audio_state.bin\"\n"
    ".balign 4\n"
    ".global secret_audio_start\nsecret_audio_start:\n"
    ".incbin \"secret_audio.bin\"\n"
    ".global secret_audio_end\nsecret_audio_end:\n"
    ".balign 4\n"
    ".global egg_tiles\negg_tiles:\n"                 /* face sprite for the secret-screen finale (egg.h) */
    ".incbin \"egg_tiles.bin\"\n"
    ".balign 4\n"
    ".global egg_pal\negg_pal:\n"
    ".incbin \"egg_pal.bin\"\n"
    ".balign 4\n"
    ".global fin_frames_start\nfin_frames_start:\n"      /* idle-menu finale clip (tools/make_finale.py) */
    ".incbin \"fin_frames.bin\"\n"
    ".global fin_frames_end\nfin_frames_end:\n"
    ".balign 4\n"
    ".global fin_audio_start\nfin_audio_start:\n"
    ".incbin \"fin_audio.bin\"\n"
    ".global fin_audio_end\nfin_audio_end:\n"
    ".balign 4\n"
    ".text\n"
);
extern const u8  frames_start[], frames_end[];
extern const u32 frames_idx_start[], frames_idx_end[];
extern const u16 palette_data[256];
extern const u8  audio_start[], audio_end[];
extern const u32 audio_state[];          /* per-chunk ADPCM state: low 16 bits = predictor (s16), bits 16-23 = step index */
extern const u32 vid2_audio_state[];
extern const u8  menu_bg[3][19200];     /* 3 menu frames, shown 0,1,2,1,0,... ; 4 bits/pixel, low nibble = left pixel */
extern const u16 menu_pal[3][16];       /* 16-colour palette per frame (254/255 are added at runtime) */

/* set BG palette entries 0-15 to menu frame k, plus the reserved text colours */
static void set_menu_pal(int k) {
    for (int i = 0; i < 16; i++) PALETTE[i] = menu_pal[k][i];
    PALETTE[254] = 0; PALETTE[255] = 0x7FFF;
}
/* unpack `n` u16 of 4bpp menu art (n bytes of source) into Mode 4 VRAM */
static void unpack_menu(volatile u16 *dst, const u8 *src, int n) {
    for (int i = 0; i < n; i++) { u8 b = src[i]; dst[i] = (u16)((b & 15) | ((b >> 4) << 8)); }
}
extern const u16 secret_bg[19200];
extern const u16 secret_pal[256];
extern const u8  vid2_frames_start[], vid2_frames_end[];
extern const u32 vid2_frames_idx_start[], vid2_frames_idx_end[];
extern const u16 vid2_palette_data[256];
extern const u8  vid2_audio_start[], vid2_audio_end[];
extern const u8  secret_audio_start[], secret_audio_end[];
extern const u8  fin_frames_start[], fin_frames_end[];
extern const u8  fin_audio_start[], fin_audio_end[];
#include "finale.h"
#define FIN_NFRAMES  ((u32)(fin_frames_end - fin_frames_start) / FIN_FRAME_BYTES)
#define FIN_NCHUNKS  ((u32)(fin_audio_end - fin_audio_start) / (SAMPLES_PER_CHUNK / 2))
#define SECRET_NCHUNKS ((u32)(secret_audio_end - secret_audio_start) / (SAMPLES_PER_CHUNK / 2))
#define VID2_NCHUNKS ((u32)(vid2_audio_end - vid2_audio_start) / (SAMPLES_PER_CHUNK / 2))   /* derived from vid2_audio.bin */

/* ---- audio state ---- */
static s8 abuf[2][SAMPLES_PER_CHUNK] __attribute__((aligned(4)));
static volatile u32 tick;          /* vblank counter: drives video position, reset when the loop restarts */
static volatile u32 achunk_ctr;    /* vblanks since the last audio chunk boundary */
static volatile u32 started;       /* chunks started so far in this loop */
static volatile u32 fill_needed;
static volatile u32 play_idx;      /* buffer to start at the next chunk boundary */
static volatile u32 g_vb_per_chunk;   /* vblanks per audio chunk: 2 normal, 4 for a half-rate stream */
static volatile u16 g_timer_reload;   /* TM0CNT_L reload matching g_vb_per_chunk */
static volatile u32 g_once;           /* 1 = play the stream once instead of looping (secret screen) */
static volatile u32 g_done;           /* set by the IRQ at the exact vblank the one-shot stream ends */
static volatile u32 g_irq_decode;     /* 1 = the IRQ decodes the next audio chunk itself (videos); 0 = main loop does it (jingle) */
static void decode_chunk(s8 *out);

static const u8 *ap;
static int pred, sidx;
static u32 dec;
static const u8 *g_audio_base;     /* start of the currently-playing stream, for looping */
static u32 g_nchunks;              /* chunk count of the currently-playing stream */

static void decode_chunk(s8 *out) {
    if (dec == g_nchunks) { dec = 0; ap = g_audio_base; pred = 0; sidx = 0; }
    for (int i = 0; i < SAMPLES_PER_CHUNK / 2; i++) {
        u8 b = *ap++;
        for (int k = 0; k < 2; k++) {
            int nib = k ? (b >> 4) : (b & 15);
            int step = step_tab[sidx];
            int diff = step >> 3;
            if (nib & 1) diff += step >> 2;
            if (nib & 2) diff += step >> 1;
            if (nib & 4) diff += step;
            if (nib & 8) pred -= diff; else pred += diff;
            pred -= pred >> 9;                    /* leak: bleeds off the wrong-start offset after a seek */
            if (pred > 32767) pred = 32767;
            if (pred < -32768) pred = -32768;
            sidx += idx_tab[nib & 7];
            if (sidx < 0) sidx = 0;
            if (sidx > 88) sidx = 88;
            out[i * 2 + k] = (s8)(pred >> 8);
        }
    }
    dec++;
}

/* VBlank interrupt: called by the BIOS in ARM state */
__attribute__((target("arm")))
static void irq_handler(void) {
    u16 flags = REG_IF;
    if (flags & 1) {
        tick++;
        achunk_ctr++;
        if (achunk_ctr >= g_vb_per_chunk) {           /* one audio-chunk boundary */
            achunk_ctr = 0;
            if (g_once && started == g_nchunks) {          /* one-shot stream just finished: go silent */
                g_done = 1;
                REG_DMA1CNT = 0;
                REG_TM0CNT_H = 0;
            } else {
                if (started == g_nchunks) { started = 0; tick = 0; }   /* loop: resync video too */
                REG_DMA1CNT = 0;
                REG_TM0CNT_H = 0;
                REG_DMA1SAD = (u32)abuf[play_idx];
                REG_DMA1DAD = 0x040000A0;         /* FIFO A */
                REG_DMA1CNT = 0xB6400001;         /* fifo mode, 32-bit, repeat, enable */
                REG_TM0CNT_L = g_timer_reload;
                REG_TM0CNT_H = 0x80;
                play_idx ^= 1;
                started++;
                if (g_irq_decode) decode_chunk(abuf[play_idx]);   /* refill right here: never depends on the main loop being free */
                else fill_needed = 1;
            }
        }
    }
    REG_IF = flags;
}

/* Frames are stored RLE-compressed (run,value byte pairs; run 1-255) since the raw
   8160 bytes/frame would not fit a long video in 32 MB. idx[f]..idx[f+1] bounds the
   compressed bytes for frame f in the stream starting at comp. Decoded into a scratch
   buffer, then drawn exactly as before. */
static u8 frame_buf[FRAME_BYTES];

static void decode_frame(const u8 *comp, u32 off, u32 end) {
    const u8 *p = comp + off, *stop = comp + end;
    u8 *out = frame_buf, *out_end = frame_buf + FRAME_BYTES;
    while (p < stop && out < out_end) {
        u8 run = *p++, val = *p++;
        for (u8 i = 0; i < run && out < out_end; i++) *out++ = val;
    }
}

/* Draw a 120x68 frame doubled to 240x136 into Mode 4 VRAM, centred. */
static void draw_frame(const u8 *src, volatile u16 *dst) {
    for (int y = 0; y < VID_H; y++) {
        volatile u32 *row0 = (volatile u32 *)(dst + (Y_OFFSET + y * 2) * 120);
        volatile u32 *row1 = row0 + 60;
        const u8 *s = src + y * VID_W;
        for (int x = 0; x < VID_W; x += 2) {
            u32 a = s[x], b = s[x + 1];
            u32 w = a | (a << 8) | (b << 16) | (b << 24);     /* 2 source pixels -> 4 VRAM bytes */
            row0[x >> 1] = w;
            row1[x >> 1] = w;
        }
    }
}

/* ================= menu ================= */
static const char font_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123 0456789.,!?'-";
static const u8 font5x7[29+15][5] = {
{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
{0x7F,0x41,0x41,0x41,0x3E},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
{0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
{0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
{0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
/* added: space 0 4 5 6 7 8 9 . , ! ? ' - */
{0x00,0x00,0x00,0x00,0x00},{0x3E,0x51,0x49,0x45,0x3E},{0x18,0x14,0x12,0x7F,0x10},
{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
{0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x00,0x60,0x60,0x00,0x00},
{0x00,0x50,0x30,0x00,0x00},{0x00,0x00,0x5F,0x00,0x00},{0x02,0x01,0x51,0x09,0x06},
{0x00,0x05,0x03,0x00,0x00},{0x08,0x08,0x08,0x08,0x08}
};

static void wait_vb(void) { while (REG_VCOUNT >= 160) {} while (REG_VCOUNT < 160) {} }

static volatile u16 *g_page = VRAM_PAGE0;   /* which Mode 4 page put()/text() draw into */

static void put(int x, int y, u8 c) {
    if ((unsigned)x >= 240 || (unsigned)y >= 160) return;
    volatile u16 *p = g_page + ((y * 240 + x) >> 1);
    u16 v = *p;
    *p = (x & 1) ? (u16)((v & 0xFF) | (c << 8)) : (u16)((v & 0xFF00) | c);
}

static const u8 *glyph(char ch) {
    for (int i = 0; font_chars[i]; i++) if (font_chars[i] == ch) return font5x7[i];
    return 0;
}

static int text_w(const char *s) { int n = 0; while (*s++) n++; return n * 7 - 1; }

/* bold 5x7 text: colour 255 (white) with colour 254 (black) shadow */
static void text(int x, int y, const char *s) {
    for (int pass = 0; pass < 2; pass++) {
        int cx = x;
        for (const char *p = s; *p; p++, cx += 7) {
            const u8 *g = glyph(*p);
            if (!g) continue;
            for (int c = 0; c < 5; c++)
                for (int r = 0; r < 7; r++)
                    if ((g[c] >> r) & 1) {
                        if (pass == 0) { put(cx + c + 1, y + r + 1, 254); put(cx + c + 2, y + r + 1, 254); }
                        else           { put(cx + c, y + r, 255);         put(cx + c + 1, y + r, 255); }
                    }
        }
    }
}

#include "subs.h"   /* sub_start / sub_end / sub_text / SUB_N - generated by tools/make_subs.py */

static int g_subs_on = 0;    /* subtitles on/off, toggled from the main menu */

static char sub_label[24];
static void update_sub_label(void) {
    static const char *const names[2] = { "SUBTITLES OFF", "SUBTITLES ON" };
    int i = 0; for (const char *p = names[g_subs_on]; *p; p++) sub_label[i++] = *p;
    sub_label[i] = 0;
}

static void menu_setup(void) {
    REG_IME = 0;
    set_menu_pal(0);
    REG_DISPCNT = 4 | (1 << 10);
    unpack_menu(VRAM_PAGE0, menu_bg[0], 19200);
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;      /* hide all sprites */
}

/* 64x32 rounded highlight sprite (4bpp), alpha-blended over the background */
static void make_highlight(void) {
    OBJ_PAL[1] = 31 | (6 << 5) | (16 << 10);    /* bright pinkish-red core */
    OBJ_PAL[2] = 22 | (2 << 5) | (10 << 10);    /* darker rim */
    for (int ty = 0; ty < 4; ty++)
        for (int tx = 0; tx < 8; tx++)
            for (int r = 0; r < 8; r++) {
                u32 w = 0;
                for (int k = 0; k < 8; k++) {
                    int X = tx * 8 + k, Y = ty * 8 + r;
                    int dx = X < 12 ? 12 - X : (X > 51 ? X - 51 : 0);
                    int dy = Y < 12 ? 12 - Y : (Y > 19 ? Y - 19 : 0);
                    int d = dx * dx + dy * dy;
                    u32 px = d > 144 ? 0 : (d > 81 ? 2 : 1);
                    w |= px << (4 * k);
                }
                OBJ_TILES[(ty * 8 + tx) * 8 + r] = w;
            }
    REG_BLDCNT = 0x0450;                        /* OBJ 1st target, alpha, BG2 2nd target */
    REG_BLDALPHA = 5 | (11 << 8);
}

#define ROW_Y(i) (15 + 23 * (i))                /* text row centres from the layout guide */
#define COL_CX 192

/* ================= secret screen =================
   Unlocked on the main menu with U U D D L R L R B A.
   Sequence: menu fades to black (~0.27 s) -> the picture fades in while its own
   music starts -> "YOU FOUND HER" sits on top and procedurally generated ocean waves
   swell up and clash along the bottom -> the instant the music ends the menu is cut
   back in (it is built on the hidden Mode 4 page and flipped in during a single
   vblank, so there is no black frame, fade or flicker).
   Picture: secret_bg.bin, 240x160 8-bit; colours 0..223 are the photo, 224..253 are
   filled in here at runtime with the water colours, 254/255 = black/white text. */
static void start_audio(u32 st, const u8 *audio_base, u32 nchunks, u32 vb_per_chunk, const u32 *states);
static void stop_audio(void);

#define FADE_N 16                               /* fade steps; 1 vblank each */
#define SECRET_SKIPPABLE 0                      /* 1 = any button leaves early */

static u16 sec_pal[256];                        /* full secret palette (photo + water + text) */

static u16 shade_col(u16 c, int lvl) {          /* lvl 0 (black) .. FADE_N (full) */
    int r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    r = r * lvl / FADE_N; g = g * lvl / FADE_N; b = b * lvl / FADE_N;
    return (u16)(r | (g << 5) | (b << 10));
}

static void fade_pal(const u16 *src, int lvl) {
    for (int i = 0; i < 256; i++) PALETTE[i] = shade_col(src[i], lvl);
}

/* ---- procedural ocean ---------------------------------------------------------
   Three stacked water layers (dark back, mid, bright front). Every layer is the sum of
   two wave trains travelling in OPPOSITE directions, so their crests keep meeting and
   clashing. Each train is re-rolled from a small LCG (wavelength, speed, height,
   phase, lifetime) whenever it dies away, so the sea never repeats; where two crests
   in the back layer line up they burst into white foam and throw spray. The water
   rises from below the screen when the picture first appears. Runs at 30 updates/s. */
#define WV_TOP 104                              /* first row redrawn every update */
#define WV_CB  224                              /* first water palette entry */

static s8  sine_tab[256];
static u32 rng_state;
static u32 rnd(void) { rng_state = rng_state * 1664525u + 1013904223u; return rng_state >> 16; }

typedef struct { u16 k, w, ph, age; u8 amp, lsh; } Wave;   /* life = 2 << lsh updates */
static Wave wv[3][2];
typedef struct { int x, y, vx, vy; u8 life; } Spray;
#define NSPRAY 20
static Spray spray[NSPRAY];
static int wv_t;
static const u8 wv_base[3]  = { 131, 140, 149 };  /* resting surface row of each layer */
static const u8 wv_amp_lo[3] = { 8, 5, 4 };
static const u8 wv_amp_rn[3] = { 5, 3, 3 };

static void wave_roll(int L, int j) {
    Wave *w = &wv[L][j];
    w->k   = (u16)(600 + (rnd() & 511));              /* wavelength ~59..109 px */
    w->w   = (u16)((w->k * (40 + (rnd() & 63))) >> 6);/* speed ~0.6..1.6 px per update */
    w->amp = (u8)(wv_amp_lo[L] + ((rnd() * wv_amp_rn[L]) >> 16));
    w->lsh = (u8)(5 + ((rnd() * 3) >> 16));           /* lives 64 / 128 / 256 updates */
    w->age = 0;
    w->ph  = (u16)rnd();
}

static void waves_init(void) {
    for (int i = 0; i < 128; i++) {                   /* sine from a parabola: plenty for water */
        int t = i, v = (t * (128 - t) * 508) >> 14;
        sine_tab[i] = (s8)v; sine_tab[i + 128] = (s8)-v;
    }
    for (int L = 0; L < 3; L++) for (int j = 0; j < 2; j++) wave_roll(L, j);
    for (int i = 0; i < NSPRAY; i++) spray[i].life = 0;
    wv_t = 0;
}

static u16 rgb15(int r, int g, int b) { return (u16)((r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)); }

static void waves_palette(u16 *pal) {
    static const u8 lo[3][3] = { { 8, 24, 80 },  { 16, 48, 130 }, { 28, 80, 200 } };
    static const u8 hi[3][3] = { { 20, 60, 160 }, { 30, 90, 220 }, { 50, 130, 255 } };
    static const u8 foam[3][3] = { { 140, 190, 255 }, { 190, 220, 255 }, { 235, 245, 255 } };
    for (int L = 0; L < 3; L++) {
        for (int i = 0; i < 6; i++)
            pal[WV_CB + L * 6 + i] = rgb15(lo[L][0] + (hi[L][0] - lo[L][0]) * i / 5,
                                           lo[L][1] + (hi[L][1] - lo[L][1]) * i / 5,
                                           lo[L][2] + (hi[L][2] - lo[L][2]) * i / 5);
        pal[WV_CB + 18 + L] = rgb15(foam[L][0], foam[L][1], foam[L][2]);
    }
}

static void spray_spawn(int x, int y) {
    for (int i = 0; i < NSPRAY; i++) if (!spray[i].life) {
        spray[i].x = x << 7; spray[i].y = y << 7;       /* 8.7 fixed point */
        spray[i].vx = (int)(rnd() & 255) - 128;
        spray[i].vy = -(0xA0 + (int)(rnd() & 0xBF));
        spray[i].life = 30;
        return;
    }
}

/* redraw rows WV_TOP..159 of one Mode 4 page: photo, water, foam, spray */
static void waves_frame(volatile u16 *dst) {
    int amp[3][2];
    for (int L = 0; L < 3; L++) for (int j = 0; j < 2; j++) {
        Wave *w = &wv[L][j];
        int half = 1 << w->lsh;
        int e = (int)w->age < half ? (int)w->age : 2 * half - (int)w->age;
        amp[L][j] = (w->amp * e) >> w->lsh;
    }
    int rr = 75 - wv_t; if (rr < 0) rr = 0;
    int rise = (80 * rr * rr) >> 13;                  /* water climbs in from below, easing out */

    for (int xp = 0; xp < 120; xp++) {
        int x = xp * 2, sv[3], sc0 = 0, sc1 = 0;
        for (int L = 0; L < 3; L++) {
            int h = 0;
            for (int j = 0; j < 2; j++) {
                const Wave *w = &wv[L][j];
                u16 a = (u16)(w->k * x + (j ? w->ph : (u16)-w->ph));   /* j0 travels right, j1 left */
                int sn = sine_tab[a >> 8];
                if (L == 0) { if (j) sc1 = sn; else sc0 = sn; }
                h += (sn * amp[L][j]) >> 7;
            }
            int y = wv_base[L] + rise - h;
            if (y > 160) y = 160;
            if (y < WV_TOP + 6) y = WV_TOP + 6;
            sv[L] = y;
        }
        if (sv[1] < sv[0]) sv[1] = sv[0];
        if (sv[2] < sv[1]) sv[2] = sv[1];
        int clash = sc0 > 100 && sc1 > 100 && amp[0][0] > 3 && amp[0][1] > 3;   /* two crests meet */
        if (clash && !(rnd() & 7) && sv[0] < 158) spray_spawn(x, sv[0]);

        volatile u16 *d = dst + WV_TOP * 120 + xp;
        const u16 *b = secret_bg + WV_TOP * 120 + xp;
        int y = WV_TOP;
        for (; y < sv[0]; y++, d += 120, b += 120) *d = *b;
        int fr = clash ? 4 : 2;
        for (; y < sv[1]; y++, d += 120) {
            int dep = y - sv[0], c = dep < fr ? WV_CB + 18 : WV_CB + ((dep >> 2) > 5 ? 5 : (dep >> 2));
            *d = (u16)(c | (c << 8));
        }
        for (; y < sv[2]; y++, d += 120) {
            int dep = y - sv[1], c = dep < 1 ? WV_CB + 19 : WV_CB + 6 + ((dep >> 2) > 5 ? 5 : (dep >> 2));
            *d = (u16)(c | (c << 8));
        }
        for (; y < 160; y++, d += 120) {
            int dep = y - sv[2], c = dep < 1 ? WV_CB + 20 : WV_CB + 12 + ((dep >> 2) > 5 ? 5 : (dep >> 2));
            *d = (u16)(c | (c << 8));
        }
    }

    g_page = dst;
    for (int i = 0; i < NSPRAY; i++) if (spray[i].life) {
        Spray *s = &spray[i];
        s->x += s->vx; s->y += s->vy; s->vy += 0x18; s->life--;
        int px = s->x >> 7, py = s->y >> 7;
        if (py >= WV_TOP && py < 160) { put(px, py, 255); put(px + 1, py, 255); }
    }

    /* advance the sea */
    wv_t++;
    for (int L = 0; L < 3; L++) for (int j = 0; j < 2; j++) {
        Wave *w = &wv[L][j];
        w->ph += w->w; w->age++;
        if (w->age >= (2u << w->lsh)) wave_roll(L, j);
    }
}

/* 2x-size text with a black outline (glyphs come from the menu font) */
static void big_text(int x, int y, const char *s) {
    for (int pass = 0; pass < 2; pass++) {
        int cx = x;
        for (const char *p = s; *p; p++, cx += 14) {
            const u8 *g = glyph(*p);
            if (!g) continue;
            for (int c = 0; c < 5; c++) for (int r = 0; r < 7; r++) if ((g[c] >> r) & 1) {
                int px = cx + c * 2, py = y + r * 2;
                if (pass == 0) {
                    for (int oy = -1; oy <= 3; oy++) for (int ox = -1; ox <= 3; ox++)
                        put(px + ox, py + oy, 254);
                } else {
                    put(px, py, 255); put(px + 1, py, 255); put(px, py + 1, 255); put(px + 1, py + 1, 255);
                }
            }
        }
    }
}

#include "egg.h"      /* scaling face sprite: keyframes + OAM driver */

/* Returns the Mode 4 page (0/1) that is on screen when the music ends. */
static int secret(int menu_frame) {
    int o1 = OBJ_PAL[1], o2 = OBJ_PAL[2];
    static u16 curpal[256];                       /* the menu frame's palette, expanded for fading */
    for (int i = 0; i < 256; i++) curpal[i] = 0;
    for (int i = 0; i < 16; i++) curpal[i] = menu_pal[menu_frame][i];
    curpal[255] = 0x7FFF;
    rng_state = 0x9E3779B9u ^ ((u32)REG_VCOUNT << 8) ^ ((u32)REG_TM0CNT_L << 16);

    /* 1. fade the menu (background + highlight sprite) out */
    for (int l = FADE_N - 1; l >= 0; l--) {
        wait_vb();
        fade_pal(curpal, l);
        OBJ_PAL[1] = shade_col((u16)o1, l);
        OBJ_PAL[2] = shade_col((u16)o2, l);
    }
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;
    REG_DISPCNT = 4 | (1 << 10);
    OBJ_PAL[1] = (u16)o1; OBJ_PAL[2] = (u16)o2;
    REG_BLDCNT = 0;                               /* face sprite must be opaque (menu re-enables blending) */
    egg_load();                                   /* face tiles + palette into OBJ VRAM */

    /* 2. build the picture on both pages while everything is black */
    for (int i = 0; i < 256; i++) sec_pal[i] = secret_pal[i];
    waves_palette(sec_pal);
    for (int i = 0; i < 256; i++) PALETTE[i] = 0;
    for (int i = 0; i < 19200; i++) { VRAM_PAGE0[i] = secret_bg[i]; VRAM_PAGE1[i] = secret_bg[i]; }
    g_page = VRAM_PAGE0; big_text(31, 6, "YOU FOUND HER");
    g_page = VRAM_PAGE1; big_text(31, 6, "YOU FOUND HER");
    waves_init();

    /* 3. music + fade in + waves, until the music ends */
    g_once = 1;
    start_audio(0, secret_audio_start, SECRET_NCHUNKS, 2, 0);
    u32 last = 0, ldraw = 0;
    int page = 0, pending = 0, fade = 0, bgfade = FADE_N;
    const int t_end = (int)(2 * SECRET_NCHUNKS + 2);  /* vblank on which the IRQ flags the end of the music */
    u16 prev = (u16)(~REG_KEYINPUT & 0x3FF);
    for (;;) {
        while (tick == last && !g_done) {}
        if (g_done) break;                            /* IRQ flags the exact vblank the music ends */
        last = tick;
        egg_update((int)last - t_end);                /* face sprite keyframes, right at the top of vblank */
        if (SECRET_SKIPPABLE) {
            u16 k = (u16)(~REG_KEYINPUT & 0x3FF);
            u16 hit = k & ~prev; prev = k;
            if (hit) break;
        }
        if (pending) { page ^= 1; REG_DISPCNT = 4 | (1 << 10) | EGG_DISP | (page << 4); pending = 0; }
        if (fade < FADE_N) { fade++; fade_pal(sec_pal, fade); }
        else {                                        /* picture goes dark behind the full-screen face */
            int lv = egg_fade_level((int)last - t_end, FADE_N);
            if (lv != bgfade) { bgfade = lv; fade_pal(sec_pal, lv); }
        }
        if (fill_needed) { fill_needed = 0; decode_chunk(abuf[play_idx]); }
        if (last - ldraw >= 2) {
            ldraw = last;
            waves_frame(page ? VRAM_PAGE0 : VRAM_PAGE1);   /* draw on the page that is NOT showing */
            pending = 1;
        }
    }
    stop_audio();
    g_page = VRAM_PAGE0;
    return page;
}

static void place_hl(int sel) {
    OAM[0] = (u16)(((ROW_Y(sel) - 16) & 0xFF) | (1 << 10) | (1 << 14));
    OAM[1] = (u16)(((COL_CX - 32) & 0x1FF) | (3 << 14));
    OAM[2] = 512;
}

/* ================= idle-menu finale =================
   After FIN_IDLE_SEC seconds with no button press on the MAIN menu, a short 30 fps clip plays
   (Mode 5, 15-bit colour, no palette, scaled 1.5x by BG2), then the menu is rebuilt exactly as it
   was (same highlighted item, same background frame) and play resumes as if nothing happened.
   It plays once per power-up: g_fin_seen is plain RAM, so only a console reset re-arms it.
   Set FIN_IDLE_RESETS_ON_INPUT to 0 to count total time in the menu instead of idle time. */
#define FIN_IDLE_SEC 77u
#define FIN_IDLE_TICKS (FIN_IDLE_SEC * 16384u)     /* TM2 runs at 16384 Hz (clk/1024), TM3 counts its overflows */
#define FIN_IDLE_RESETS_ON_INPUT 1
static u32 g_fin_seen = 0;

static void idle_reset(void) {
    REG_TM2CNT_H = 0; REG_TM3CNT_H = 0;
    REG_TM2CNT_L = 0; REG_TM3CNT_L = 0;
    REG_TM3CNT_H = 0x84;                           /* enable + cascade from TM2 */
    REG_TM2CNT_H = 0x83;                           /* enable + prescaler 1024 */
}
static u32 idle_elapsed(void) {
    u32 hi = REG_TM3CNT_L, lo = REG_TM2CNT_L;
    if (REG_TM3CNT_L != hi) { hi = REG_TM3CNT_L; lo = REG_TM2CNT_L; }   /* overflow happened mid-read */
    return (hi << 16) | lo;
}

static void fin_play(void) {
    REG_IME = 0;
    REG_DISPCNT = 0x80;                            /* forced blank while everything is swapped */
    for (int i = 0; i < 256; i++) PALETTE[i] = 0;  /* backdrop (letterbox bars) black */
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;
    REG_BLDCNT = 0;
    { volatile u32 *v = (volatile u32 *)0x06000000; for (int i = 0; i < 0x14000 / 4; i++) v[i] = 0; }
    /* BG2 affine: 2/3 step = 1.5x zoom, 160x92 -> 240x138, centred vertically (11 px bars) */
    REG_BG2PA = 171; REG_BG2PB = 0; REG_BG2PC = 0; REG_BG2PD = 171;
    REG_BG2X = 0;    REG_BG2Y = (u32)(-11 * 171);
    fin_decode_frame(fin_frames_start, (volatile u32 *)VRAM_PAGE0);

    g_once = 1;
    start_audio(0, fin_audio_start, FIN_NCHUNKS, 2, 0);   /* first chunk starts on tick 2 */
    REG_DISPCNT = 5 | (1 << 10);                   /* Mode 5, BG2, page 0 */

    u32 last = 0; int page = 0, pending = 0, drawn = 0;
    for (;;) {
        while (tick == last && !g_done) __asm__ volatile("swi 0x02" ::: "r0","r1","r2","r3","memory");
        if (g_done) break;                         /* IRQ flags the vblank the audio (and clip) ends */
        last = tick;
        if (pending) { page ^= 1; REG_DISPCNT = 5 | (1 << 10) | (page << 4); pending = 0; }
        if (fill_needed) { fill_needed = 0; decode_chunk(abuf[play_idx]); }
        /* build the picture for NEXT tick on the hidden page; 1 frame = 2 vblanks, audio starts on tick 2 */
        u32 nt = last + 1;
        int f = nt >= 2 ? (int)((nt - 2) >> 1) : 0;
        if (f >= (int)FIN_NFRAMES) f = (int)FIN_NFRAMES - 1;
        if (f != drawn) {
            fin_decode_frame(fin_frames_start + f * FIN_FRAME_BYTES,
                             (volatile u32 *)(page ? VRAM_PAGE0 : VRAM_PAGE1));
            drawn = f; pending = 1;
        }
    }
    stop_audio();
    REG_DISPCNT = 0x80;
    REG_BG2PA = 0x100; REG_BG2PB = 0; REG_BG2PC = 0; REG_BG2PD = 0x100;   /* back to identity for Mode 4 */
    REG_BG2X = 0; REG_BG2Y = 0;
}

/* rebuild the menu screen exactly as menu() left it (background frame k, highlight on `sel`) */
static void fin_menu_restore(const char *const *items, int n, int k, int sel) {
    for (int i = 0; i < 256; i++) PALETTE[i] = 0;            /* black until the picture is ready */
    REG_DISPCNT = 4 | (1 << 10) | 0x80;
    REG_BLDCNT = 0;
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;
    unpack_menu(VRAM_PAGE0, menu_bg[k], 19200);
    unpack_menu(VRAM_PAGE1, menu_bg[k], 19200);
    for (int j = 0; j < n; j++) {
        g_page = VRAM_PAGE0; text(COL_CX - text_w(items[j]) / 2, ROW_Y(j) - 3, items[j]);
        g_page = VRAM_PAGE1; text(COL_CX - text_w(items[j]) / 2, ROW_Y(j) - 3, items[j]);
    }
    g_page = VRAM_PAGE0;
    make_highlight();
    place_hl(sel);
    REG_BLDCNT = 0x0450; REG_BLDALPHA = 5 | (11 << 8);
    wait_vb();
    set_menu_pal(k);
    REG_DISPCNT = 4 | (1 << 10) | (1 << 6) | (1 << 12);      /* page 0, sprites on */
}

/* returns chosen index, or -1 for B (only when allow_back) */
static int menu(const char *const *items, int n, int allow_back) {
    menu_setup();
    for (int i = 0; i < n; i++) text(COL_CX - text_w(items[i]) / 2, ROW_Y(i) - 3, items[i]);
    make_highlight();
    REG_DISPCNT = 4 | (1 << 10) | (1 << 6) | (1 << 12);
    int sel = 0;
    static const u16 konami[10]  = { 0x40, 0x40, 0x80, 0x80, 0x20, 0x10, 0x20, 0x10, 0x02, 0x01 };
    /* Altered code: D D U U L R L R B A, triggers the easter-egg clip */
    static const u16 konami2[10] = { 0x80, 0x80, 0x40, 0x40, 0x20, 0x10, 0x20, 0x10, 0x02, 0x01 };
    int ki = 0, ki2 = 0;
    int idle_on = (!allow_back && !g_fin_seen);
    if (idle_on) idle_reset();
    /* Background animation: frames 0,1,2,1 repeat, 30 vblanks (0.5 s) each. The next frame is
       built on the hidden page a slice per vblank, then palette + page are swapped at the
       start of a vblank, so the change is instant and never flickers or stalls input. */
    static const u8 anim_seq[4] = { 0, 1, 2, 1 };
    int mp = 0, si = 0, ac = 0, prep = 0;
    u16 prev = (u16)(~REG_KEYINPUT & 0x3FF);
    for (;;) {
        wait_vb();
        place_hl(sel);
        {
            int ni = anim_seq[(si + 1) & 3];
            if (ac < 30) ac++;
            if (ac >= 30 && prep == 5) {
                set_menu_pal(ni);
                mp ^= 1;
                REG_DISPCNT = 4 | (1 << 10) | (1 << 6) | (1 << 12) | (mp << 4);
                si = (si + 1) & 3; ac = 0; prep = 0;
                ni = anim_seq[(si + 1) & 3];
            }
            volatile u16 *back = mp ? VRAM_PAGE0 : VRAM_PAGE1;
            if (prep < 4) {
                unpack_menu(back + prep * 4800, menu_bg[ni] + prep * 4800, 4800);
                prep++;
            } else if (prep == 4) {
                g_page = back;
                for (int i = 0; i < n; i++) text(COL_CX - text_w(items[i]) / 2, ROW_Y(i) - 3, items[i]);
                g_page = VRAM_PAGE0;
                prep = 5;
            }
        }
        u16 k = (u16)(~REG_KEYINPUT & 0x3FF);
        u16 hit = k & ~prev;
        prev = k;
        if (idle_on) {
#if FIN_IDLE_RESETS_ON_INPUT
            if (hit) idle_reset();
#endif
            if (idle_elapsed() >= FIN_IDLE_TICKS) {
                idle_on = 0; g_fin_seen = 1;
                REG_TM2CNT_H = 0; REG_TM3CNT_H = 0;
                fin_play();
                fin_menu_restore(items, n, anim_seq[si], sel);
                mp = 0; ac = 0; prep = 0; ki = ki2 = 0;
                prev = (u16)(~REG_KEYINPUT & 0x3FF);
                continue;
            }
        }
        if (!allow_back && hit) {                            /* Konami: U U D D L R L R B A */
            if (hit == konami[ki]) ki++;
            else ki = (hit == konami[0]) ? 1 : 0;
            if (ki == 10) {
                /* secret screen; when its music ends, cut straight back to this menu:
                   draw it on the hidden page, then swap palette + page inside one vblank */
                int shown = secret(anim_seq[si]);
                volatile u16 *back = shown ? VRAM_PAGE0 : VRAM_PAGE1;
                unpack_menu(back, menu_bg[0], 19200);
                g_page = back;
                for (int i = 0; i < n; i++) text(COL_CX - text_w(items[i]) / 2, ROW_Y(i) - 3, items[i]);
                g_page = VRAM_PAGE0;
                make_highlight();
                REG_BLDCNT = 0;                               /* the face is still on screen: keep it opaque */
                wait_vb();
                set_menu_pal(0);
                REG_DISPCNT = 4 | (1 << 10) | (1 << 6) | (1 << 12) | ((shown ^ 1) << 4);
                for (int n = 1; n <= EGG_SLIDE_TICKS; n++) {  /* menu is live underneath: face slides off */
                    wait_vb();
                    egg_update(n);
                }
                egg_hide();
                REG_BLDCNT = 0x0450;                          /* back to the menu's translucent highlight */
                place_hl(sel);
                mp = shown ^ 1; si = 0; ac = 0; prep = 0;     /* animation restarts at frame 0 */
                ki = ki2 = 0;
                prev = (u16)(~REG_KEYINPUT & 0x3FF);
                continue;
            }

            if (hit == konami2[ki2]) ki2++;
            else ki2 = (hit == konami2[0]) ? 1 : 0;
            if (ki2 == 10) { ki2 = 0; return -3; }
        }
        if (hit & 0x40) sel = (sel + n - 1) % n;              /* up */
        if (hit & 0x80) sel = (sel + 1) % n;                  /* down */
        if ((hit & 0x09) && n == 4 && sel == 3) {             /* A/START on SUBTITLES: cycle, don't leave menu */
            g_subs_on ^= 1;
            update_sub_label();
            unpack_menu(VRAM_PAGE0, menu_bg[anim_seq[si]], 19200);
            unpack_menu(VRAM_PAGE1, menu_bg[anim_seq[si]], 19200);
            for (int j = 0; j < n; j++) {
                g_page = VRAM_PAGE0; text(COL_CX - text_w(items[j]) / 2, ROW_Y(j) - 3, items[j]);
                g_page = VRAM_PAGE1; text(COL_CX - text_w(items[j]) / 2, ROW_Y(j) - 3, items[j]);
            }
            g_page = VRAM_PAGE0;
            prep = 5; ac = 0;
            continue;
        }
        if (hit & 0x09) return sel;                          /* A / START */
        if ((hit & 0x02) && allow_back) return -1;           /* B */
    }
}

/* ================= title intro =================
   Runs once at boot, before the menu ever appears: 19 letter-sprites (spaces are just gaps)
   scatter in from random points, dart through a random mid-screen clump, then land on the
   exact centred layout that spells "TOP 10 ANIMOMENTS !!!!" - 240 vblanks = 4.0s @ 60Hz.
   Pure integer/fixed math (no sin/cos, no float) so the landing position is always exact,
   never approximated. Uses OBJ tiles 512..530 (local 0..18) and OBJ_PAL bank 0 colours 5-7;
   both ranges are free at boot and get overwritten harmlessly once make_highlight() runs. */
#define INTRO_N     19
#define INTRO_P1    150          /* ticks: scatter point -> mid clump */
#define INTRO_P2    210          /* ticks: mid clump -> exact word (then hold) */
#define INTRO_TOTAL 240          /* 4.0 s at 60 Hz */

static void build_letter_tile(int idx, char c, int pal) {
    const u8 *g = glyph(c);
    for (int r = 0; r < 8; r++) {
        u32 w = 0;
        if (g) for (int col = 0; col < 5; col++)
            if (r < 7 && ((g[col] >> r) & 1)) w |= (u32)pal << (4 * col);
        OBJ_TILES[idx * 8 + r] = w;
    }
}

/* Subtitle-only glyph builder: bakes a black outline (pal 10) behind the white
   fill (pal 9) into the SAME tile, offset by 1px down-right - same bold trick
   text() uses on the menu (shadow pass, then fill pass). Plain white-only glyphs
   disappear against light video frames (sky, fur, skin); plain black would just
   fail the same way on dark frames. Outlined text reads on either. */
static void build_sub_letter_tile(int idx, char c) {
    const u8 *g = glyph(c);
    u8 px[8][8];
    for (int r = 0; r < 8; r++) for (int cc = 0; cc < 8; cc++) px[r][cc] = 0;
    if (g) {
        for (int col = 0; col < 5; col++)
            for (int row = 0; row < 7; row++)
                if ((g[col] >> row) & 1) {
                    px[row + 1][col + 1] = 10;             /* shadow, drawn first */
                }
        for (int col = 0; col < 5; col++)
            for (int row = 0; row < 7; row++)
                if ((g[col] >> row) & 1) {
                    px[row][col] = 9;                      /* fill, overwrites any shadow under it */
                }
    }
    for (int r = 0; r < 8; r++) {
        u32 w = 0;
        for (int cc = 0; cc < 8; cc++) w |= (u32)px[r][cc] << (4 * cc);
        OBJ_TILES[idx * 8 + r] = w;
    }
}

static void title_intro(void) {
    static const char title[] = "TOP 10 ANIMOMENTS !!!!";
    int fx[INTRO_N], fy[INTRO_N];    /* final, exact positions */
    int sx[INTRO_N], sy[INTRO_N];    /* scatter start */
    int mx[INTRO_N], my[INTRO_N];    /* mid clump waypoint */
    u8  lpal[INTRO_N];

    REG_IME = 0;
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;       /* hide everything */
    PALETTE[0] = 0;                                          /* black backdrop; no BG2 needed */
    OBJ_PAL[5] = 0x7FFF;                                     /* white */
    OBJ_PAL[6] = 16 | (6 << 5) | (26 << 10);                 /* bright pinkish red */
    OBJ_PAL[7] = 10 | (3 << 5) | (17 << 10);                 /* darker pinkish red */

    rng_state = 0x9E3779B9u ^ ((u32)REG_VCOUNT << 8) ^ ((u32)REG_TM0CNT_L << 16);

    int n = 0, cx = 120 - text_w(title) / 2;
    for (const char *p = title; *p; p++, cx += 7) {
        if (*p == ' ') continue;                             /* space: advance only, no sprite */
        fx[n] = cx; fy[n] = 76;
        lpal[n] = (u8)(5 + (n % 3));
        build_letter_tile(n, *p, lpal[n]);
        sx[n] = (int)(rnd() % 288) - 24;               /* scattered off/around the screen */
        sy[n] = (int)(rnd() % 192) - 16;
        mx[n] = 96 + (int)(rnd() % 48);                /* random mid-screen clump */
        my[n] = 52 + (int)(rnd() % 56);
        n++;
    }

    REG_DISPCNT = 4 | (1 << 6) | (1 << 12);                  /* mode 4, 1D obj mapping, OBJ layer only */

    u16 prev = (u16)(~REG_KEYINPUT & 0x3FF);
    for (int t = 0; t < INTRO_TOTAL; t++) {
        wait_vb();
        u16 k = (u16)(~REG_KEYINPUT & 0x3FF);
        u16 hit = k & ~prev; prev = k;
        if (hit) break;                                      /* any button: skip straight to the menu */

        for (int i = 0; i < n; i++) {
            int x, y;
            if (t < INTRO_P1) {
                x = sx[i] + (int)(mx[i] - sx[i]) * t / INTRO_P1;
                y = sy[i] + (int)(my[i] - sy[i]) * t / INTRO_P1;
            } else if (t < INTRO_P2) {
                int u = t - INTRO_P1, d = INTRO_P2 - INTRO_P1;
                x = mx[i] + (int)(fx[i] - mx[i]) * u / d;
                y = my[i] + (int)(fy[i] - my[i]) * u / d;
            } else {
                x = fx[i]; y = fy[i];                          /* held exactly, no drift */
            }
            OAM[i * 4]     = (u16)(y & 0xFF);
            OAM[i * 4 + 1] = (u16)(x & 0x1FF);
            OAM[i * 4 + 2] = (u16)(512 + i);                   /* bank 0: nibble value picks the colour */
        }
    }
    for (int i = 0; i < n; i++) {                              /* final frame: pin exact coords */
        OAM[i * 4]     = (u16)(fy[i] & 0xFF);
        OAM[i * 4 + 1] = (u16)(fx[i] & 0x1FF);
    }
    wait_vb(); wait_vb(); wait_vb(); wait_vb(); wait_vb(); wait_vb();  /* hold ~0.1s so it reads before the menu cuts in */
}

static void controls(void) {
    menu_setup();
    for (int y = 10; y < 150; y++) for (int x = 10; x < 230; x++) put(x, y, 254);
    text(20, 16, "CONTROLS");
    text(20, 34, "UP DOWN   MOVE");
    text(20, 48, "A OR START   SELECT");
    text(20, 62, "B   BACK");
    text(20, 80, "IN VIDEO");
    text(20, 94, "A UP   PAUSE");
    text(20, 108, "A RIGHT   FAST FORWARD");
    text(20, 122, "A DOWN   REWIND");
    text(20, 136, "SELECT   MENU");
    u16 prev = (u16)(~REG_KEYINPUT & 0x3FF);
    for (;;) {
        wait_vb();
        u16 k = (u16)(~REG_KEYINPUT & 0x3FF);
        u16 hit = k & ~prev;
        prev = k;
        if (hit & 0x0B) return;
    }
}

static u32 chapter_tick(int p) {
    unsigned count = (unsigned)(frames_idx_end - frames_idx_start) - 1;
    u32 f = count * p / 3;
    u32 t = ((f << 16) + 5485u) / 5486u;
    t &= ~1u;
    if (t / 2 >= NCHUNKS) t = 0;
    return t;
}

/* ================= playback ================= */
/* Generalised player: plays any frame stream + palette + ADPCM audio stream
   in the same layout as the main video, so it can drive either the main
   feature or the easter-egg clip. */
static void start_audio(u32 st, const u8 *audio_base, u32 nchunks, u32 vb_per_chunk, const u32 *states) {
    REG_IME = 0;
    REG_SOUNDCNT_X = 0x80;
    REG_SOUNDCNT_H = 0x0B04;
    g_audio_base = audio_base;
    g_nchunks = nchunks;
    g_vb_per_chunk = vb_per_chunk;
    g_timer_reload = (u16)(65536 - 1848 * (vb_per_chunk / 2));   /* half chunk-rate = half timer rate = half pitch */
    u32 c = st / vb_per_chunk;
    dec = c; ap = audio_base + c * (SAMPLES_PER_CHUNK / 2);
    /* ADPCM only decodes correctly from the exact state it had at that point in the stream.
       Restarting from (0,0) after a seek leaves the step size wrong for a very long time
       (loud/quiet, distorted audio), so resume from the state saved for this chunk. */
    if (states) { u32 sv = states[c]; pred = (short)(sv & 0xFFFF); sidx = (int)((sv >> 16) & 0xFF); }
    else        { pred = 0; sidx = 0; }
    /* With a state table (the videos), start the first chunk on the very next vblank instead of
       waiting a whole chunk (2 vblanks, 4 for the half-rate video), which left audio late after
       every resume. The jingle passes no table and keeps its original timing. */
    play_idx = 0; started = c; tick = st; achunk_ctr = states ? vb_per_chunk - 1 : 0; fill_needed = 0; g_done = 0;
    g_irq_decode = states ? 1 : 0;
    decode_chunk(abuf[0]);
    IRQ_VECTOR = (u32)irq_handler;
    REG_DISPSTAT = 8;
    REG_IF = 0xFFFF;
    REG_IE = 1;
    REG_IME = 1;
}

static void stop_audio(void) {
    REG_IME = 0;
    REG_IE = 0;
    REG_DISPSTAT = 0;
    REG_DMA1CNT = 0;
    REG_TM0CNT_H = 0;
    REG_SOUNDCNT_H = 0x0800;                /* reset FIFO A, silence */
    REG_IF = 0xFFFF;
    g_once = 0;
}

#define SEEK_STEP 4                         /* ticks (vblanks) per frame while seeking = 4x speed */

/* Subtitles are drawn as OBJ sprites, not baked into the video's BG pixels. Two reasons:
   1) the video's BG palette is the real picture's 256 colours - stomping indices 254/255 for
      text (as an earlier version did) silently recolours any video pixel that legitimately
      used those two indices (that's the "missing pixel" - it was never missing, just forced
      black/white). Sprites use the separate OBJ palette bank instead, so the picture is untouched.
   2) sprites aren't page-based, so there's no stale-pixel residue when the video's own redraw
      doesn't happen to touch that screen row (that was the clutter/ghosting). */
#define SUB_COLS     34                     /* characters per line: 34 * 7px = 238px, fits the 240px screen */
#define SUB_MAXLINES 3                      /* a cue may wrap onto up to 3 lines (tools/make_subs.py splits longer ones) */
#define SUB_MAXSPR   (SUB_COLS * SUB_MAXLINES)   /* 102 of the 128 hardware sprites */
#define SUB_LINE_H   9                      /* px between lines (8px glyph tile + 1px gap) */
#define SUB_BASE_Y   148                    /* y of the last line (just under the video); earlier lines stack upward */
static int g_sub_shown = -1;                /* cue index currently on screen, -1 = none */

static void sub_hide(void) {
    for (int i = 0; i < SUB_MAXSPR; i++) OAM[i * 4] = 0x200;
    g_sub_shown = -1;
}

/* Split a cue into display lines. A '\n' in the text forces a break; otherwise a line is wrapped at
   the last space that keeps it within SUB_COLS (a word longer than a line is hard-split). Returns the
   number of lines (<= SUB_MAXLINES); anything that would need a further line is dropped. */
static int sub_layout(const char *s, const char **ls, int *ll) {
    int nl = 0;
    while (nl < SUB_MAXLINES) {
        while (*s == ' ') s++;
        if (!*s) break;
        int len = -1, brk = -1;
        for (int i = 0; i <= SUB_COLS; i++) {
            char c = s[i];
            if (c == 0 || c == '\n') { len = i; break; }
            if (c == ' ') brk = i;
        }
        if (len < 0) len = brk > 0 ? brk : SUB_COLS;       /* too long: wrap at the last space */
        int keep = len;
        while (keep > 0 && s[keep - 1] == ' ') keep--;       /* trim trailing spaces so centring is exact */
        ls[nl] = s; ll[nl] = keep; nl++;
        s += len;
        if (*s == '\n') s++;
    }
    return nl;
}

static void sub_show(const char *s) {
    const char *ls[SUB_MAXLINES]; int ll[SUB_MAXLINES];
    int nl = sub_layout(s, ls, ll);
    int n = 0;
    for (int k = 0; k < nl; k++) {
        int y = SUB_BASE_Y - (nl - 1 - k) * SUB_LINE_H;     /* last line sits on SUB_BASE_Y */
        int cx = 120 - (ll[k] * 7 - 1) / 2;
        for (int j = 0; j < ll[k]; j++, cx += 7) {
            char ch = ls[k][j];
            if (ch == ' ') continue;
            build_sub_letter_tile(n, ch);                   /* white fill (9) + black outline (10) */
            OAM[n * 4]     = (u16)(y & 0xFF);
            OAM[n * 4 + 1] = (u16)(cx & 0x1FF);
            OAM[n * 4 + 2] = (u16)(512 + n);
            n++;
        }
    }
    for (int i = n; i < SUB_MAXSPR; i++) OAM[i * 4] = 0x200;   /* hide any leftover slots */
}

/* called every tick (not just when the video frame changes), so a cue appears on the exact vblank of its
   start tick instead of waiting for the next ~5fps frame redraw. t is the playback position in vblanks. */
static void sub_update(u32 t) {
    int idx = -1;
    for (int i = 0; i < SUB_N; i++)
        if (t >= sub_start[i] && t < sub_end[i]) { idx = i; break; }
    if (idx == g_sub_shown) return;
    if (idx < 0) sub_hide();
    else { sub_show(sub_text[idx]); g_sub_shown = idx; }
}

static void play_generic(u32 st, const u8 *fr_start, const u32 *idx, unsigned count,
                          const u16 *pal, const u8 *audio_base, u32 nchunks, u32 rate, const u32 *states,
                          int use_subs) {
    REG_IME = 0;
    for (int i = 0; i < 256; i++) PALETTE[i] = pal[i];   /* video's real 256-colour palette, untouched */
    for (int i = 0; i < 19200; i++) VRAM_PAGE0[i] = 0;
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;
    g_sub_shown = -1;
    REG_BLDCNT = 0;     /* menu leaves alpha-blend on (translucent cursor) - kill it or subtitle
                           sprites get blended 5:11 against the video and end up nearly invisible */
    if (use_subs && g_subs_on) { OBJ_PAL[9] = 0x7FFF; OBJ_PAL[10] = 0x0000; }  /* subtitle white + black outline, OBJ bank0 */
    REG_DISPCNT = 4 | (1 << 10) | (1 << 6) | (1 << 12);  /* BG2 + OBJ (1D mapping) */

    /* rate 1 = normal (5fps, vblanks/chunk=2); rate 2 = half-speed playback (2.5fps
       equivalent hold time, vblanks/chunk=4) for a stream whose source was sped up
       2x before encoding - see README for how that trades quality for ROM space. */
    u32 vbc = 2 * rate;
    u32 fps_num = 5486u / rate;
    u32 maxpos = ((u32)count << 16) / fps_num;
    if (maxpos > nchunks * vbc - vbc) maxpos = nchunks * vbc - vbc;
    maxpos &= ~(vbc - 1);

    start_audio(st, audio_base, nchunks, vbc, states);

    u32 last = st, pos = st;
    int mode = 0;                           /* 0 play, 1 pause, 2 fast-forward, 3 rewind */
    int paused = 0;
    int drawn = -1, pending = 0, page = 0;
    u16 prev = (u16)(~REG_KEYINPUT & 0x3FF);

    for (;;) {
        u32 t;
        if (mode == 0) {
            while (tick == last) __asm__ volatile("swi 0x02" ::: "r0","r1","r2","r3","memory");   /* BIOS Halt: sleep until the vblank IRQ */
            t = tick;
            last = t;
        } else {
            wait_vb();
            if (mode == 2) { pos += SEEK_STEP; if (pos > maxpos) pos = maxpos; }
            if (mode == 3) { pos = pos > SEEK_STEP ? pos - SEEK_STEP : 0; }
            t = pos;
        }

        u16 k = (u16)(~REG_KEYINPUT & 0x3FF);
        u16 hit = k & ~prev;
        prev = k;
        if (k & 4) break;                   /* SELECT: back to menu */

        int a = k & 1;
        if (a && (hit & 0x40)) paused ^= 1; /* A + Up: pause / resume */
        if ((hit & 1) && (k & 0x40)) paused ^= 1;
        int nm = paused ? 1 : 0;
        if (a && (k & 0x10)) nm = 2;        /* A + Right: fast-forward */
        else if (a && (k & 0x80)) nm = 3;   /* A + Down: rewind */

        if (nm != mode) {
            if (mode == 0) { pos = t > maxpos ? maxpos : t; stop_audio(); }
            if (nm == 0) {                  /* resume: re-sync audio to the video position */
                u32 s = pos & ~(vbc - 1);
                start_audio(s, audio_base, nchunks, vbc, states);
                last = s; t = s;
            }
            mode = nm;
        }

        if (pending) {
            page ^= 1;
            REG_DISPCNT = 4 | (1 << 10) | (1 << 6) | (1 << 12) | (page << 4);
            pending = 0;
        }
        if (mode == 0 && fill_needed) {
            fill_needed = 0;
            decode_chunk(abuf[play_idx]);
        }
        if (use_subs && g_subs_on) sub_update(t);     /* every tick - not gated on frame redraw */
        unsigned f = (t * fps_num) >> 16;
        if (f >= count) f = count - 1;
        if ((int)f != drawn) {
            decode_frame(fr_start, idx[f], idx[f + 1]);
            draw_frame(frame_buf, page ? VRAM_PAGE0 : VRAM_PAGE1);
            drawn = (int)f;
            pending = 1;
        }
    }

    stop_audio();
    REG_DISPCNT = 4 | (1 << 10);            /* back to page 0 */
}

static void play(u32 st) {
    unsigned count = (unsigned)(frames_idx_end - frames_idx_start) - 1;
    play_generic(st, frames_start, frames_idx_start, count, palette_data, audio_start, NCHUNKS, 1, audio_state, 1);
}

/* Second (hidden) video, triggered by the alternate Konami code (D D U U L R L R B A).
   Encoded from a 2x-sped-up source (half the frames, half the audio bytes for the same
   real-world runtime), then played back at rate=2 here: half frame-rate (each frame held
   twice as long) and half the audio timer rate (halves the sample rate, which stretches
   duration 2x AND drops the pitch back an octave - undoing the speedup, not just the
   duration change). Only correct if the source's audio was itself a naive resample
   (pitch-shifted "chipmunk" speedup) rather than a pitch-preserving time-stretch. */
static void play_vid2(void) {
    unsigned count = (unsigned)(vid2_frames_idx_end - vid2_frames_idx_start) - 1;
    play_generic(0, vid2_frames_start, vid2_frames_idx_start, count, vid2_palette_data,
                 vid2_audio_start, VID2_NCHUNKS, 2, vid2_audio_state, 0);
}

int main(void) {
    title_intro();
    update_sub_label();
    static const char *const chap_items[3] = { "PART 1", "PART 2", "PART 3" };
    const char *main_items[4] = { "PLAY", "CHAPTERS", "CONTROLS", sub_label };
    for (;;) {
        main_items[3] = sub_label;
        int a = menu(main_items, 4, 0);
        if (a == -2) continue;
        if (a == -3) { play_vid2(); continue; }
        if (a == 0) play(0);
        else if (a == 1) { int c = menu(chap_items, 3, 1); if (c >= 0) play(chapter_tick(c)); }
        else if (a == 2) controls();
    }
}
