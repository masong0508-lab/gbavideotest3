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
    ".incbin \"frames.bin\"\n"
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
    ".incbin \"vid2_frames.bin\"\n"
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
    ".text\n"
);
extern const u8  frames_start[], frames_end[];
extern const u32 frames_idx_start[], frames_idx_end[];
extern const u16 palette_data[256];
extern const u8  audio_start[], audio_end[];
extern const u16 menu_bg[19200];
extern const u16 menu_pal[256];
extern const u16 secret_bg[19200];
extern const u16 secret_pal[256];
extern const u8  vid2_frames_start[], vid2_frames_end[];
extern const u32 vid2_frames_idx_start[], vid2_frames_idx_end[];
extern const u16 vid2_palette_data[256];
extern const u8  vid2_audio_start[], vid2_audio_end[];
#define VID2_NCHUNKS ((u32)(vid2_audio_end - vid2_audio_start) / (SAMPLES_PER_CHUNK / 2))   /* derived from vid2_audio.bin */

/* ---- audio state ---- */
static s8 abuf[2][SAMPLES_PER_CHUNK] __attribute__((aligned(4)));
static volatile u32 tick;          /* vblank counter, reset when the loop restarts */
static volatile u32 started;       /* chunks started so far in this loop */
static volatile u32 fill_needed;
static volatile u32 play_idx;      /* buffer to start at the next chunk boundary */

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
        if (tick & 1) {                       /* every second vblank */
            if (started == g_nchunks) { started = 0; tick = 1; }   /* loop */
            REG_DMA1CNT = 0;
            REG_TM0CNT_H = 0;
            REG_DMA1SAD = (u32)abuf[play_idx];
            REG_DMA1DAD = 0x040000A0;         /* FIFO A */
            REG_DMA1CNT = 0xB6400001;         /* fifo mode, 32-bit, repeat, enable */
            REG_TM0CNT_L = 65536 - 1848;
            REG_TM0CNT_H = 0x80;
            play_idx ^= 1;
            started++;
            fill_needed = 1;
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
        volatile u16 *row0 = dst + (Y_OFFSET + y * 2) * 120;
        volatile u16 *row1 = row0 + 120;
        for (int x = 0; x < VID_W; x++) {
            u16 p = src[y * VID_W + x];
            u16 pair = (u16)((p << 8) | p);
            row0[x] = pair;
            row1[x] = pair;
        }
    }
}

/* ================= menu ================= */
static const char font_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123";
static const u8 font5x7[29][5] = {
{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},{0x3E,0x41,0x41,0x41,0x22},
{0x7F,0x41,0x41,0x41,0x3E},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
{0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},
{0x20,0x40,0x41,0x3F,0x01},{0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},
{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},{0x3E,0x41,0x41,0x41,0x3E},
{0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
{0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},
{0x1F,0x20,0x40,0x20,0x1F},{0x3F,0x40,0x38,0x40,0x3F},{0x63,0x14,0x08,0x14,0x63},
{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31}
};

static void wait_vb(void) { while (REG_VCOUNT >= 160) {} while (REG_VCOUNT < 160) {} }

static void put(int x, int y, u8 c) {
    if ((unsigned)x >= 240 || (unsigned)y >= 160) return;
    volatile u16 *p = VRAM_PAGE0 + ((y * 240 + x) >> 1);
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

static void menu_setup(void) {
    REG_IME = 0;
    for (int i = 0; i < 256; i++) PALETTE[i] = menu_pal[i];
    REG_DISPCNT = 4 | (1 << 10);
    for (int i = 0; i < 19200; i++) VRAM_PAGE0[i] = menu_bg[i];
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;      /* hide all sprites */
}

/* 64x32 rounded highlight sprite (4bpp), alpha-blended over the background */
static void make_highlight(void) {
    OBJ_PAL[1] = 8 | (31 << 5) | (4 << 10);     /* bright lime core */
    OBJ_PAL[2] = 4 | (24 << 5) | (2 << 10);     /* darker rim */
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

/* Secret screen (Konami code on the main menu). Same format as the menu
   background: 240x160 8-bit, palette idx 254 = black, 255 = white.
   Shows the image until any button is pressed, then returns to the menu.
   Put your own sound/animation here if you want one. */
static void secret(void) {
    REG_IME = 0;
    for (int i = 0; i < 256; i++) PALETTE[i] = secret_pal[i];
    REG_DISPCNT = 4 | (1 << 10);
    for (int i = 0; i < 19200; i++) VRAM_PAGE0[i] = secret_bg[i];
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;
    text(120 - text_w("SECRET") / 2, 76, "SECRET");   /* remove this line for a text-free screen */

    u16 prev = (u16)(~REG_KEYINPUT & 0x3FF);
    for (;;) {
        wait_vb();
        u16 k = (u16)(~REG_KEYINPUT & 0x3FF);
        u16 hit = k & ~prev;
        prev = k;
        if (hit) return;
    }
}

/* returns chosen index, or -1 for B (only when allow_back) */
static int menu(const char *const *items, int allow_back) {
    menu_setup();
    for (int i = 0; i < 3; i++) text(COL_CX - text_w(items[i]) / 2, ROW_Y(i) - 3, items[i]);
    make_highlight();
    REG_DISPCNT = 4 | (1 << 10) | (1 << 6) | (1 << 12);
    int sel = 0;
    static const u16 konami[10]  = { 0x40, 0x40, 0x80, 0x80, 0x20, 0x10, 0x20, 0x10, 0x02, 0x01 };
    /* Altered code: D D U U L R L R B A, triggers the easter-egg clip */
    static const u16 konami2[10] = { 0x80, 0x80, 0x40, 0x40, 0x20, 0x10, 0x20, 0x10, 0x02, 0x01 };
    int ki = 0, ki2 = 0;
    u16 prev = (u16)(~REG_KEYINPUT & 0x3FF);
    for (;;) {
        wait_vb();
        OAM[0] = (u16)(((ROW_Y(sel) - 16) & 0xFF) | (1 << 10) | (1 << 14));
        OAM[1] = (u16)(((COL_CX - 32) & 0x1FF) | (3 << 14));
        OAM[2] = 512;
        u16 k = (u16)(~REG_KEYINPUT & 0x3FF);
        u16 hit = k & ~prev;
        prev = k;
        if (!allow_back && hit) {                            /* Konami: U U D D L R L R B A */
            if (hit == konami[ki]) ki++;
            else ki = (hit == konami[0]) ? 1 : 0;
            if (ki == 10) { secret(); return -2; }

            if (hit == konami2[ki2]) ki2++;
            else ki2 = (hit == konami2[0]) ? 1 : 0;
            if (ki2 == 10) { ki2 = 0; return -3; }
        }
        if (hit & 0x40) sel = (sel + 2) % 3;                 /* up */
        if (hit & 0x80) sel = (sel + 1) % 3;                 /* down */
        if (hit & 0x09) return sel;                          /* A / START */
        if ((hit & 0x02) && allow_back) return -1;           /* B */
    }
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
static void start_audio(u32 st, const u8 *audio_base, u32 nchunks) {
    REG_IME = 0;
    REG_SOUNDCNT_X = 0x80;
    REG_SOUNDCNT_H = 0x0B04;
    g_audio_base = audio_base;
    g_nchunks = nchunks;
    u32 c = st >> 1;
    dec = c; ap = audio_base + c * (SAMPLES_PER_CHUNK / 2);
    pred = 0; sidx = 0;
    play_idx = 0; started = c; tick = st; fill_needed = 0;
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
}

#define SEEK_STEP 4                         /* ticks (vblanks) per frame while seeking = 4x speed */

static void play_generic(u32 st, const u8 *fr_start, const u32 *idx, unsigned count,
                          const u16 *pal, const u8 *audio_base, u32 nchunks) {
    REG_IME = 0;
    for (int i = 0; i < 256; i++) PALETTE[i] = pal[i];
    for (int i = 0; i < 19200; i++) VRAM_PAGE0[i] = 0;
    for (int i = 0; i < 128; i++) OAM[i * 4] = 0x200;
    REG_DISPCNT = 4 | (1 << 10);

    u32 maxpos = ((u32)count << 16) / 5486u;
    if (maxpos > nchunks * 2 - 2) maxpos = nchunks * 2 - 2;
    maxpos &= ~1u;

    start_audio(st, audio_base, nchunks);

    u32 last = st, pos = st;
    int mode = 0;                           /* 0 play, 1 pause, 2 fast-forward, 3 rewind */
    int paused = 0;
    int drawn = -1, pending = 0, page = 0;
    u16 prev = (u16)(~REG_KEYINPUT & 0x3FF);

    for (;;) {
        u32 t;
        if (mode == 0) {
            while (tick == last) {}
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
                u32 s = pos & ~1u;
                start_audio(s, audio_base, nchunks);
                last = s; t = s;
            }
            mode = nm;
        }

        if (pending) {
            page ^= 1;
            REG_DISPCNT = 4 | (1 << 10) | (page << 4);
            pending = 0;
        }
        if (mode == 0 && fill_needed) {
            fill_needed = 0;
            decode_chunk(abuf[play_idx]);
        }
        unsigned f = (t * 5486u) >> 16;
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
    play_generic(st, frames_start, frames_idx_start, count, palette_data, audio_start, NCHUNKS);
}

/* Second (hidden) video, triggered by the alternate Konami code (D D U U L R L R B A). */
static void play_vid2(void) {
    unsigned count = (unsigned)(vid2_frames_idx_end - vid2_frames_idx_start) - 1;
    play_generic(0, vid2_frames_start, vid2_frames_idx_start, count, vid2_palette_data,
                 vid2_audio_start, VID2_NCHUNKS);
}

int main(void) {
    static const char *const main_items[3] = { "PLAY", "CHAPTERS", "CONTROLS" };
    static const char *const chap_items[3] = { "PART 1", "PART 2", "PART 3" };
    for (;;) {
        int a = menu(main_items, 0);
        if (a == -2) continue;
        if (a == -3) { play_vid2(); continue; }
        if (a == 0) play(0);
        else if (a == 1) { int c = menu(chap_items, 1); if (c >= 0) play(chapter_tick(c)); }
        else controls();
    }
}
