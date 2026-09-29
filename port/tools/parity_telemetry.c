/* Exhaustive frame-by-frame parity check: native port vs the original ROM.
 *
 * Replays a play recording's inputs through the native core and compares EVERY
 * layer of machine state against a Mesen ground-truth archive
 * (tools/mesen_telemetry.lua -> tools/ctel.py sink -> .ctel), frame by frame:
 *
 *   RAM        all 2 KiB, classified into named regions (src/ram.asm):
 *                game     -- everything the game logic owns (the parity metric)
 *                scratch  -- $00-$17 subroutine temporaries
 *                sound    -- the bank-1 sound engine's RAM (the port has none)
 *                stack    -- $0197-$01FF 6502 return addresses / pushes
 *                rng      -- $34 at endFrame (advanced by the cycle-counted
 *                            idle loop after the logic; the NMI-time value is
 *                            injected from the recording)
 *   PPU        nametable RAM (CIRAM), palette RAM, OAM, CHR RAM, and the
 *              rendering registers (PPUCTRL, PPUMASK, t / fine-x scroll)
 *   PIXELS     all 61,440 pixels, compared as canonical NES palette indices
 *              (exact visual identity under Mesen's palette)
 *
 * Usage:
 *   contra_parity_telemetry RECORDING.jsonl MESEN.ctel [options]
 *     --from N            first frame to compare (default 1)
 *     --to N              last frame (default: end of archive)
 *     --report PATH       per-frame JSONL (diff counts per layer/region)
 *     --detail F[,F...]   write full two-sided dumps + a PNG triptych
 *                         (mesen | native | diff) for these frames
 *     --outdir DIR        detail output directory (default tmp/parity)
 *     --stop LAYER        stop at the first frame where LAYER differs
 *                         (game, scratch, nt, pal, oam, chr, regs, fb, any)
 *     --symbols PATH      ram.asm for RAM names (default src/ram.asm)
 *     --watch A[-B],...   print these RAM addresses (hex) every compared frame,
 *                         mesen/native where they differ (use with --from/--to)
 *     --addr-histogram    list every game-class RAM address that differs,
 *                         with its frame count and first frame
 *     --quiet             summary only
 *
 * Input: by default each frame's pad bits are the ones the ROM itself
 * validated that frame (CTRL_KNOWN_GOOD $F9/$FA from the archive: DPCM
 * controller-read glitches already resolved, and free of the scripted input
 * the game writes into CONTROLLER_STATE, e.g. the end-of-level walk).
 * Env: CONTRA_NATIVE_PLAY_INPUT=raw|latched selects the recording's raw pad
 * bits or its CONTROLLER_STATE column instead; CONTRA_NATIVE_PLAY_RNG=free
 * disables the RANDOM_NUM injection. Timing: like RANDOM_NUM, the dot at
 * which a load's clear_ppu lands mid-picture depends on CPU cycles the port
 * does not count (NMI entry jitter, the sound engine), so the reference's
 * position is passed in as core.raster_cut_hint; CONTRA_NATIVE_PLAY_RASTER=free
 * uses the port's typical per-load timing instead.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "contra/core.h"
#include "contra/ppu.h"

enum
{
    FB_W = 256,
    FB_H = 240,
    FB_PIXELS = FB_W * FB_H,
    SEC_RAM = 1,
    SEC_CIRAM = 2,
    SEC_PALETTE = 3,
    SEC_OAM = 4,
    SEC_CHR = 5,
    SEC_REGS = 6,
    SEC_FB = 7,
    SEC_PPU_LOG = 8,
    SEC_REG_LOG = 9,
    SEC_INFO = 10,
    SEC_MAX = 16,
    MAX_DETAIL = 256
};

/* Mesen 2's default palette -- see tools/mesen_telemetry.lua */
static const uint32_t mesen_palette[64] = {
    0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600, 0x561D00,
    0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000, 0x000000,
    0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00,
    0x6B6D00, 0x388700, 0x0C9300, 0x008F32, 0x007C8D, 0x000000, 0x000000, 0x000000,
    0xFFFEFF, 0x64B0FF, 0x9290FF, 0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22,
    0xBCBE00, 0x88D800, 0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000,
    0xFFFEFF, 0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
    0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000,
};

/* ------------------------------------------------------------------------ */
/* RAM classification                                                        */

typedef enum
{
    CLS_GAME = 0,
    CLS_SCRATCH,
    CLS_SOUND,
    CLS_STACK,
    CLS_RNG,
    CLS_COUNT
} RamClass;

static const char *const class_names[CLS_COUNT] = {"game", "scratch", "sound", "stack", "rng"};

typedef struct
{
    const char *name;
    uint16_t lo;
    uint16_t hi; /* exclusive */
    RamClass cls;
} RamRegion;

/* Report regions, in address order. The sound engine's zero-page slice
   ($E0-$EF: SOUND_TABLE_PTR & co.) is split out of zp so the game metric only
   counts what the port is responsible for. */
static const RamRegion ram_regions[] = {
    {"zp_scratch", 0x000, 0x018, CLS_SCRATCH},
    {"zp", 0x018, 0x034, CLS_GAME},
    {"rng", 0x034, 0x035, CLS_RNG},
    {"zp", 0x035, 0x0E0, CLS_GAME},
    {"zp_sound", 0x0E0, 0x0F0, CLS_SOUND},
    {"zp", 0x0F0, 0x100, CLS_GAME},
    {"sound", 0x100, 0x190, CLS_SOUND},
    {"level_end", 0x190, 0x197, CLS_GAME},
    {"stack", 0x197, 0x200, CLS_STACK},
    {"oam_buffer", 0x200, 0x300, CLS_GAME},
    {"sprites", 0x300, 0x368, CLS_GAME},
    {"player_bullets", 0x368, 0x4B8, CLS_GAME},
    {"enemies", 0x4B8, 0x600, CLS_GAME},
    {"supertiles", 0x600, 0x680, CLS_GAME},
    {"bg_collision", 0x680, 0x700, CLS_GAME},
    {"gfx_buffer", 0x700, 0x7C0, CLS_GAME},
    {"palette_buffer", 0x7C0, 0x7E0, CLS_GAME},
    {"scores", 0x7E0, 0x7ED, CLS_GAME},
    /* PREVIOUS_ROM_BANK_1: also written by the sound engine's own chained
       play_sound calls (bank 1), so it follows the unported sound engine */
    {"bank_1_sound", 0x7ED, 0x7EE, CLS_SOUND},
    {"scores", 0x7EE, 0x800, CLS_GAME},
};
enum { RAM_REGION_COUNT = (int)(sizeof(ram_regions) / sizeof(ram_regions[0])) };

static uint8_t ram_class[CONTRA_CPU_RAM_SIZE];
static uint8_t ram_region_of[CONTRA_CPU_RAM_SIZE];

static void init_ram_classes(void)
{
    int r;
    for (r = 0; r < RAM_REGION_COUNT; ++r)
    {
        unsigned a;
        for (a = ram_regions[r].lo; a < ram_regions[r].hi; ++a)
        {
            ram_class[a] = (uint8_t)ram_regions[r].cls;
            ram_region_of[a] = (uint8_t)r;
        }
    }
}

/* RAM symbol names parsed from src/ram.asm (".export[zp] NAME ; $addr") */
typedef struct
{
    uint16_t addr;
    char name[40];
} RamSymbol;

static RamSymbol symbols[512];
static int symbol_count;

static int symbol_cmp(const void *a, const void *b)
{
    const RamSymbol *sa = (const RamSymbol *)a;
    const RamSymbol *sb = (const RamSymbol *)b;
    return (int)sa->addr - (int)sb->addr;
}

static void load_symbols(const char *path)
{
    char line[512];
    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        return;
    }
    while (fgets(line, sizeof(line), f) != NULL && symbol_count < 512)
    {
        char name[64];
        unsigned addr;
        const char *p = line;
        if (strncmp(p, ".exportzp", 9) == 0)
        {
            p += 9;
        }
        else if (strncmp(p, ".export", 7) == 0)
        {
            p += 7;
        }
        else
        {
            continue;
        }
        if (sscanf(p, " %63s ; $%x", name, &addr) == 2 && addr < 0x800u)
        {
            symbols[symbol_count].addr = (uint16_t)addr;
            snprintf(symbols[symbol_count].name, sizeof(symbols[symbol_count].name), "%s", name);
            ++symbol_count;
        }
    }
    fclose(f);
    qsort(symbols, (size_t)symbol_count, sizeof(symbols[0]), symbol_cmp);
}

static void symbol_name(uint16_t addr, char *out, size_t size)
{
    int i;
    int best = -1;
    for (i = 0; i < symbol_count; ++i)
    {
        if (symbols[i].addr <= addr)
        {
            best = i;
        }
        else
        {
            break;
        }
    }
    if (best < 0)
    {
        snprintf(out, size, "$%03X", addr);
    }
    else if (symbols[best].addr == addr)
    {
        snprintf(out, size, "%s", symbols[best].name);
    }
    else
    {
        snprintf(out, size, "%s+%u", symbols[best].name, (unsigned)(addr - symbols[best].addr));
    }
}

/* ------------------------------------------------------------------------ */
/* recording (inputs + NMI-time RNG + lag schedule)                          */

enum
{
    INPUT_KNOWN_GOOD, /* default: Mesen's CTRL_KNOWN_GOOD ($F9/$FA) each frame */
    INPUT_RAW,        /* the recording's raw pad bits */
    INPUT_LATCHED     /* the recording's CONTROLLER_STATE (includes scripted input) */
};

typedef struct
{
    uint8_t p1_raw, p2_raw, p1_latched, p2_latched, frame_counter, rng;
    uint8_t has_row, has_fc, has_rng;
} InputRow;

static bool extract_unsigned(const char *line, const char *key, unsigned *out)
{
    char needle[64];
    const char *pos;
    snprintf(needle, sizeof(needle), "\"%s\":", key);
    pos = strstr(line, needle);
    if (pos == NULL)
    {
        return false;
    }
    *out = (unsigned)strtoul(pos + strlen(needle), NULL, 10);
    return true;
}

static InputRow *load_recording(const char *path, unsigned *out_last)
{
    static char line[1 << 16];
    FILE *fp = fopen(path, "r");
    InputRow *rows = NULL;
    size_t cap = 0;
    unsigned last = 0;
    if (fp == NULL)
    {
        fprintf(stderr, "FAIL cannot open recording %s\n", path);
        return NULL;
    }
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        unsigned frame, v;
        InputRow row;
        if (!extract_unsigned(line, "frame", &frame) || frame == 0u)
        {
            continue;
        }
        memset(&row, 0, sizeof(row));
        row.has_row = 1;
        if (extract_unsigned(line, "p1_raw", &v)) row.p1_raw = (uint8_t)v;
        if (extract_unsigned(line, "p2_raw", &v)) row.p2_raw = (uint8_t)v;
        if (extract_unsigned(line, "controller", &v)) row.p1_latched = (uint8_t)v;
        if (extract_unsigned(line, "p2_controller", &v)) row.p2_latched = (uint8_t)v;
        if (extract_unsigned(line, "frame_counter", &v)) { row.frame_counter = (uint8_t)v; row.has_fc = 1; }
        if (extract_unsigned(line, "rng", &v)) { row.rng = (uint8_t)v; row.has_rng = 1; }
        if ((size_t)frame >= cap)
        {
            size_t ncap = cap ? cap : 4096;
            InputRow *grown;
            while ((size_t)frame >= ncap) ncap *= 2;
            grown = (InputRow *)realloc(rows, ncap * sizeof(*rows));
            if (grown == NULL) { free(rows); fclose(fp); return NULL; }
            memset(grown + cap, 0, (ncap - cap) * sizeof(*rows));
            rows = grown;
            cap = ncap;
        }
        rows[frame] = row;
        if (frame > last) last = frame;
    }
    fclose(fp);
    *out_last = last;
    return rows;
}

/* A recording row whose FRAME_COUNTER equals the previous row's is a real-NES
   lag frame (the logic overran the video frame). */
static bool is_lag_row(const InputRow *rows, unsigned last, unsigned f)
{
    return (f >= 2u) && (f <= last) && rows[f].has_row && rows[f].has_fc &&
           rows[f - 1u].has_row && rows[f - 1u].has_fc &&
           (rows[f].frame_counter == rows[f - 1u].frame_counter);
}

/* The reference frame's first mid-picture PPUMASK write that turns rendering
   off (register log: u32 n, n x {u16 addr, u8 value, i16 scanline, u16 dot}),
   as scanline * 341 + dot + 1 -- the core's raster_cut_hint. 0 = none. */
static uint16_t reference_raster_cut(const uint8_t *log, uint32_t len)
{
    uint32_t n, i;

    if (log == NULL || len < 4u) return 0u;
    n = (uint32_t)log[0] | ((uint32_t)log[1] << 8) | ((uint32_t)log[2] << 16) | ((uint32_t)log[3] << 24);
    for (i = 0; i < n && 4u + (i + 1u) * 7u <= len; ++i)
    {
        const uint8_t *e = log + 4u + i * 7u;
        const unsigned addr = (unsigned)e[0] | ((unsigned)e[1] << 8);
        const int scanline = (int16_t)((uint16_t)e[3] | ((uint16_t)e[4] << 8));
        const unsigned dot = (unsigned)e[5] | ((unsigned)e[6] << 8);

        if (addr == 0x2001u && (e[2] & 0x18u) == 0u && scanline >= 0 && scanline < 240)
        {
            return (uint16_t)((unsigned)scanline * 341u + dot + 1u);
        }
    }
    return 0u;
}

/* lag burst neighborhood: the endFrame snapshot is torn mid-logic */
static bool is_torn_row(const InputRow *rows, unsigned last, unsigned f)
{
    return is_lag_row(rows, last, f) || ((f + 1u <= last) && is_lag_row(rows, last, f + 1u));
}

/* ------------------------------------------------------------------------ */
/* .ctel archive reader                                                      */

typedef struct
{
    FILE *fp;
    uint8_t *raw;
    size_t raw_cap;
    uint8_t *comp;
    size_t comp_cap;
    uint32_t frame;
    const uint8_t *sec[SEC_MAX];
    uint32_t sec_len[SEC_MAX];
    uint8_t chr[0x2000];
    bool has_chr;
} Archive;

static bool archive_open(Archive *a, const char *path)
{
    char magic[8];
    memset(a, 0, sizeof(*a));
    a->fp = fopen(path, "rb");
    if (a->fp == NULL || fread(magic, 1, 8, a->fp) != 8 || memcmp(magic, "CTEL", 4) != 0)
    {
        fprintf(stderr, "FAIL %s is not a .ctel archive\n", path);
        return false;
    }
    return true;
}

static bool archive_next(Archive *a)
{
    uint32_t header[3];
    uLongf raw_len;
    uint16_t count;
    size_t pos;
    unsigned i;
    if (fread(header, 4, 3, a->fp) != 3)
    {
        return false;
    }
    if (header[1] > a->raw_cap)
    {
        a->raw_cap = header[1] * 2u;
        a->raw = (uint8_t *)realloc(a->raw, a->raw_cap);
    }
    if (header[2] > a->comp_cap)
    {
        a->comp_cap = header[2] * 2u;
        a->comp = (uint8_t *)realloc(a->comp, a->comp_cap);
    }
    if (fread(a->comp, 1, header[2], a->fp) != header[2])
    {
        return false;
    }
    raw_len = header[1];
    if (uncompress(a->raw, &raw_len, a->comp, header[2]) != Z_OK || raw_len != header[1])
    {
        fprintf(stderr, "FAIL corrupt archive record at frame %u\n", header[0]);
        return false;
    }
    memset(a->sec, 0, sizeof(a->sec));
    memset(a->sec_len, 0, sizeof(a->sec_len));
    memcpy(&a->frame, a->raw, 4);
    memcpy(&count, a->raw + 4, 2);
    pos = 6;
    for (i = 0; i < count && pos + 5 <= raw_len; ++i)
    {
        const uint8_t id = a->raw[pos];
        uint32_t len;
        memcpy(&len, a->raw + pos + 1, 4);
        pos += 5;
        if (id < SEC_MAX)
        {
            a->sec[id] = a->raw + pos;
            a->sec_len[id] = len;
        }
        pos += len;
    }
    if (a->sec[SEC_CHR] != NULL && a->sec_len[SEC_CHR] == 0x2000u)
    {
        memcpy(a->chr, a->sec[SEC_CHR], 0x2000u);
        a->has_chr = true;
    }
    return true;
}

/* ------------------------------------------------------------------------ */
/* native capture                                                            */

typedef struct
{
    uint8_t ram[0x800];
    uint8_t ciram[0x800];
    uint8_t palette[0x20];
    uint8_t oam[0x100];
    uint8_t chr[0x2000];
    uint8_t ctrl;
    uint8_t mask;
    uint16_t t;
    uint8_t fine_x;
    uint8_t fb[FB_PIXELS];
} Telemetry;

static uint8_t canonical_index[64];

static void init_palette_maps(void)
{
    int i, j;
    for (i = 0; i < 64; ++i)
    {
        canonical_index[i] = (uint8_t)i;
        for (j = 0; j < i; ++j)
        {
            if (mesen_palette[j] == mesen_palette[i])
            {
                canonical_index[i] = (uint8_t)j;
                break;
            }
        }
    }
}

/* native RGB -> canonical Mesen palette index (0xFE: RGB not in Mesen's palette) */
static uint8_t rgb_to_canonical(uint32_t rgb)
{
    int i;
    rgb &= 0xFFFFFFu;
    for (i = 0; i < 64; ++i)
    {
        if (mesen_palette[i] == rgb)
        {
            return canonical_index[i];
        }
    }
    return 0xFEu;
}

static void capture_native(const ContraCore *core, const uint8_t *render_oam, const uint8_t *render_regs,
                           Telemetry *t)
{
    static uint32_t last_rgb = 0xFFFFFFFFu;
    static uint8_t last_idx;
    unsigned i;
    memcpy(t->ram, core->ram, sizeof(t->ram));
    memcpy(t->ciram, core->ppu_nametable, sizeof(t->ciram));
    memcpy(t->palette, core->ppu_palette, sizeof(t->palette));
    memcpy(t->oam, render_oam, sizeof(t->oam));
    memcpy(t->chr, core->ppu_pattern, sizeof(t->chr));
    t->ctrl = render_regs[0];
    t->mask = render_regs[1];
    t->t = (uint16_t)(render_regs[2] | (render_regs[3] << 8));
    t->fine_x = render_regs[4];
    for (i = 0; i < FB_PIXELS; ++i)
    {
        const uint32_t rgb = core->framebuffer[i] & 0xFFFFFFu;
        if (rgb != last_rgb)
        {
            last_rgb = rgb;
            last_idx = rgb_to_canonical(rgb);
        }
        t->fb[i] = last_idx;
    }
}

/* the port's PPU register state after the step (== Mesen's at endFrame) */
static void native_render_regs(const ContraCore *core, uint8_t *regs)
{
    regs[0] = core->ppu.ctrl;
    regs[1] = core->ppu.mask;
    regs[2] = (uint8_t)(core->ppu.t & 0xFFu);
    regs[3] = (uint8_t)(core->ppu.t >> 8);
    regs[4] = core->ppu.fine_x;
}

/* ------------------------------------------------------------------------ */
/* PNG writer (stored zlib, no deps beyond zlib)                             */

static void png_chunk(FILE *f, const char *tag, const uint8_t *data, uint32_t len)
{
    uint8_t be[4];
    uLong crc;
    be[0] = (uint8_t)(len >> 24); be[1] = (uint8_t)(len >> 16); be[2] = (uint8_t)(len >> 8); be[3] = (uint8_t)len;
    fwrite(be, 1, 4, f);
    fwrite(tag, 1, 4, f);
    if (len) fwrite(data, 1, len, f);
    crc = crc32(0L, (const Bytef *)tag, 4);
    if (len) crc = crc32(crc, data, len);
    be[0] = (uint8_t)(crc >> 24); be[1] = (uint8_t)(crc >> 16); be[2] = (uint8_t)(crc >> 8); be[3] = (uint8_t)crc;
    fwrite(be, 1, 4, f);
}

static void write_png(const char *path, int w, int h, const uint8_t *rgb)
{
    FILE *f = fopen(path, "wb");
    uint8_t ihdr[13];
    size_t raw_len = (size_t)(w * 3 + 1) * (size_t)h;
    uint8_t *raw = (uint8_t *)malloc(raw_len);
    uLongf comp_len = compressBound((uLong)raw_len);
    uint8_t *comp = (uint8_t *)malloc(comp_len);
    int y;
    static const uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (f == NULL || raw == NULL || comp == NULL)
    {
        if (f) fclose(f);
        free(raw);
        free(comp);
        return;
    }
    for (y = 0; y < h; ++y)
    {
        raw[(size_t)y * (size_t)(w * 3 + 1)] = 0;
        memcpy(raw + (size_t)y * (size_t)(w * 3 + 1) + 1, rgb + (size_t)y * (size_t)w * 3u, (size_t)w * 3u);
    }
    compress2(comp, &comp_len, raw, (uLong)raw_len, 6);
    fwrite(sig, 1, 8, f);
    ihdr[0] = (uint8_t)(w >> 24); ihdr[1] = (uint8_t)(w >> 16); ihdr[2] = (uint8_t)(w >> 8); ihdr[3] = (uint8_t)w;
    ihdr[4] = (uint8_t)(h >> 24); ihdr[5] = (uint8_t)(h >> 16); ihdr[6] = (uint8_t)(h >> 8); ihdr[7] = (uint8_t)h;
    ihdr[8] = 8; ihdr[9] = 2; ihdr[10] = 0; ihdr[11] = 0; ihdr[12] = 0;
    png_chunk(f, "IHDR", ihdr, 13);
    png_chunk(f, "IDAT", comp, (uint32_t)comp_len);
    png_chunk(f, "IEND", NULL, 0);
    fclose(f);
    free(raw);
    free(comp);
}

static void index_rgb(uint8_t idx, uint8_t *out)
{
    const uint32_t c = (idx < 64u) ? mesen_palette[idx] : 0xFF00FFu;
    out[0] = (uint8_t)(c >> 16);
    out[1] = (uint8_t)(c >> 8);
    out[2] = (uint8_t)c;
}

/* mesen | native | diff (diff: dimmed mesen, differing pixels red), 2x scale */
static void write_triptych(const char *path, const uint8_t *mfb, const uint8_t *nfb)
{
    const int scale = 2;
    const int w = FB_W * 3 * scale;
    const int h = FB_H * scale;
    uint8_t *rgb = (uint8_t *)malloc((size_t)w * (size_t)h * 3u);
    int x, y;
    if (rgb == NULL) return;
    for (y = 0; y < FB_H; ++y)
    {
        for (x = 0; x < FB_W; ++x)
        {
            const int i = y * FB_W + x;
            uint8_t px[3][3];
            int panel, sx, sy;
            index_rgb(mfb[i], px[0]);
            index_rgb(nfb[i], px[1]);
            if (mfb[i] != nfb[i])
            {
                px[2][0] = 255; px[2][1] = 0; px[2][2] = 0;
            }
            else
            {
                const uint8_t g = (uint8_t)((px[0][0] + px[0][1] + px[0][2]) / 9);
                px[2][0] = g; px[2][1] = g; px[2][2] = g;
            }
            for (panel = 0; panel < 3; ++panel)
                for (sy = 0; sy < scale; ++sy)
                    for (sx = 0; sx < scale; ++sx)
                    {
                        const size_t o = ((size_t)(y * scale + sy) * (size_t)w +
                                          (size_t)(panel * FB_W * scale + x * scale + sx)) * 3u;
                        rgb[o] = px[panel][0]; rgb[o + 1] = px[panel][1]; rgb[o + 2] = px[panel][2];
                    }
        }
    }
    write_png(path, w, h, rgb);
    free(rgb);
}

/* ------------------------------------------------------------------------ */
/* comparison                                                                */

typedef enum
{
    L_GAME = 0, L_SCRATCH, L_SOUND, L_STACK, L_RNG, L_NT, L_PAL, L_OAM, L_CHR, L_REGS, L_FB, L_COUNT
} Layer;

static const char *const layer_names[L_COUNT] = {
    "game", "scratch", "sound", "stack", "rng", "nt", "pal", "oam", "chr", "regs", "fb"};

typedef struct
{
    unsigned count[L_COUNT];
    unsigned region[RAM_REGION_COUNT];
    int fb_x0, fb_y0, fb_x1, fb_y1;
    unsigned fb_unknown;
} FrameDiff;

static void compare_frame(const Archive *m, const Telemetry *n, FrameDiff *d)
{
    unsigned i;
    memset(d, 0, sizeof(*d));
    d->fb_x0 = FB_W; d->fb_y0 = FB_H; d->fb_x1 = -1; d->fb_y1 = -1;
    if (m->sec[SEC_RAM] != NULL)
    {
        for (i = 0; i < 0x800u; ++i)
        {
            if (m->sec[SEC_RAM][i] != n->ram[i])
            {
                ++d->count[ram_class[i]]; /* CLS_* == L_* for the RAM classes */
                ++d->region[ram_region_of[i]];
            }
        }
    }
    if (m->sec[SEC_CIRAM] != NULL)
        for (i = 0; i < 0x800u; ++i) d->count[L_NT] += (m->sec[SEC_CIRAM][i] != n->ciram[i]);
    if (m->sec[SEC_PALETTE] != NULL)
        for (i = 0; i < 0x20u; ++i) d->count[L_PAL] += ((m->sec[SEC_PALETTE][i] & 0x3Fu) != (n->palette[i] & 0x3Fu));
    if (m->sec[SEC_OAM] != NULL)
        for (i = 0; i < 0x40u; ++i) d->count[L_OAM] += (memcmp(m->sec[SEC_OAM] + i * 4u, n->oam + i * 4u, 4) != 0);
    if (m->has_chr)
        for (i = 0; i < 0x2000u; ++i) d->count[L_CHR] += (m->chr[i] != n->chr[i]);
    if (m->sec[SEC_REGS] != NULL)
    {
        const uint8_t *r = m->sec[SEC_REGS];
        const uint16_t mt = (uint16_t)(r[4] | (r[5] << 8));
        /* compare the rendering-relevant bits: nametable/pattern/sprite-size
           ctrl bits, the mask, the scroll (t without its nametable bits is
           compared via ctrl), fine x */
        d->count[L_REGS] += ((r[0] & 0x3Bu) != (n->ctrl & 0x3Bu));
        d->count[L_REGS] += (r[1] != n->mask);
        d->count[L_REGS] += ((mt & 0x73FFu) != (n->t & 0x73FFu));
        d->count[L_REGS] += (r[6] != n->fine_x);
    }
    if (m->sec[SEC_FB] != NULL && m->sec_len[SEC_FB] == FB_PIXELS)
    {
        const uint8_t *mf = m->sec[SEC_FB];
        for (i = 0; i < FB_PIXELS; ++i)
        {
            if (mf[i] == 0xFFu) ++d->fb_unknown;
            if (mf[i] != n->fb[i])
            {
                const int x = (int)(i % FB_W), y = (int)(i / FB_W);
                ++d->count[L_FB];
                if (x < d->fb_x0) d->fb_x0 = x;
                if (x > d->fb_x1) d->fb_x1 = x;
                if (y < d->fb_y0) d->fb_y0 = y;
                if (y > d->fb_y1) d->fb_y1 = y;
            }
        }
    }
}

static const char *stage_of(const uint8_t *mram)
{
    static char buf[32];
    if (mram[0x18] != 0x05u)
    {
        snprintf(buf, sizeof(buf), "intro");
    }
    else
    {
        snprintf(buf, sizeof(buf), "stage%u", (unsigned)mram[0x30] + 1u);
    }
    return buf;
}

static void write_detail(const char *outdir, unsigned frame, const Archive *m, const Telemetry *n,
                         const FrameDiff *d)
{
    char path[1024];
    FILE *f;
    unsigned i;
    snprintf(path, sizeof(path), "%s/%u.txt", outdir, frame);
    f = fopen(path, "w");
    if (f == NULL)
    {
        fprintf(stderr, "cannot write %s\n", path);
        return;
    }
    fprintf(f, "frame %u  (%s)\n", frame, stage_of(m->sec[SEC_RAM]));
    for (i = 0; i < L_COUNT; ++i)
        fprintf(f, "  %-8s %u\n", layer_names[i], d->count[i]);
    fprintf(f, "\nRAM differences (mesen / native):\n");
    for (i = 0; i < 0x800u; ++i)
    {
        if (m->sec[SEC_RAM][i] != n->ram[i])
        {
            char name[64];
            symbol_name((uint16_t)i, name, sizeof(name));
            fprintf(f, "  $%03X %-36s %02X / %02X   [%s:%s]\n", i, name, m->sec[SEC_RAM][i], n->ram[i],
                    ram_regions[ram_region_of[i]].name, class_names[ram_class[i]]);
        }
    }
    fprintf(f, "\nnametable differences (addr: mesen / native):\n");
    for (i = 0; i < 0x800u; ++i)
    {
        if (m->sec[SEC_CIRAM][i] != n->ciram[i])
        {
            const unsigned nt = i >> 10, off = i & 0x3FFu;
            if (off < 0x3C0u)
                fprintf(f, "  $%04X nt%u row %2u col %2u  %02X / %02X\n", 0x2000u + (nt << 10) + off, nt,
                        off >> 5, off & 31u, m->sec[SEC_CIRAM][i], n->ciram[i]);
            else
                fprintf(f, "  $%04X nt%u attr %2u        %02X / %02X\n", 0x2000u + (nt << 10) + off, nt,
                        off - 0x3C0u, m->sec[SEC_CIRAM][i], n->ciram[i]);
        }
    }
    fprintf(f, "\npalette (mesen / native):\n ");
    for (i = 0; i < 0x20u; ++i)
        fprintf(f, " %02X/%02X%s", m->sec[SEC_PALETTE][i], n->palette[i],
                ((m->sec[SEC_PALETTE][i] & 0x3F) != (n->palette[i] & 0x3F)) ? "*" : "");
    fprintf(f, "\n\nOAM differences (sprite: y tile attr x):\n");
    for (i = 0; i < 0x40u; ++i)
    {
        const uint8_t *a = m->sec[SEC_OAM] + i * 4u;
        const uint8_t *b = n->oam + i * 4u;
        if (memcmp(a, b, 4) != 0)
            fprintf(f, "  #%02u  mesen %02X %02X %02X %02X   native %02X %02X %02X %02X\n", i, a[0], a[1], a[2],
                    a[3], b[0], b[1], b[2], b[3]);
    }
    {
        const uint8_t *r = m->sec[SEC_REGS];
        fprintf(f, "\nregs: mesen ctrl=%02X mask=%02X v=%04X t=%04X x=%u w=%u | native ctrl=%02X mask=%02X t=%04X x=%u\n",
                r[0], r[1], r[2] | (r[3] << 8), r[4] | (r[5] << 8), r[6], r[7], n->ctrl, n->mask, n->t,
                n->fine_x);
    }
    if (m->has_chr)
    {
        unsigned first = 0xFFFFu, cnt = 0;
        for (i = 0; i < 0x2000u; ++i)
            if (m->chr[i] != n->chr[i]) { if (first == 0xFFFFu) first = i; ++cnt; }
        fprintf(f, "\nCHR: %u bytes differ%s", cnt, cnt ? "" : "\n");
        if (cnt) fprintf(f, " (first $%04X, tile %s$%02X)\n", first, first >= 0x1000u ? "BG " : "SPR ", (first & 0xFFFu) >> 4);
    }
    if (d->count[L_FB])
        fprintf(f, "\npixels: %u differ, bbox x %d..%d y %d..%d\n", d->count[L_FB], d->fb_x0, d->fb_x1, d->fb_y0,
                d->fb_y1);
    if (m->sec[SEC_PPU_LOG] != NULL)
    {
        uint32_t cnt;
        memcpy(&cnt, m->sec[SEC_PPU_LOG], 4);
        fprintf(f, "\nmesen PPU writes this frame: %u\n ", cnt);
        for (i = 0; i < cnt && i < 400u; ++i)
        {
            const uint8_t *e = m->sec[SEC_PPU_LOG] + 4u + i * 3u;
            fprintf(f, " %04X=%02X", e[0] | (e[1] << 8), e[2]);
        }
        fprintf(f, "\n");
    }
    fclose(f);

    if (m->sec[SEC_FB] != NULL)
    {
        snprintf(path, sizeof(path), "%s/%u.png", outdir, frame);
        write_triptych(path, m->sec[SEC_FB], n->fb);
    }
    {
        static const struct { const char *suffix; int sec; size_t size; } dumps[] = {
            {"ram", SEC_RAM, 0x800}, {"ciram", SEC_CIRAM, 0x800}, {"pal", SEC_PALETTE, 0x20}, {"oam", SEC_OAM, 0x100}};
        const uint8_t *native_parts[] = {n->ram, n->ciram, n->palette, n->oam};
        size_t k;
        for (k = 0; k < sizeof(dumps) / sizeof(dumps[0]); ++k)
        {
            FILE *df;
            snprintf(path, sizeof(path), "%s/%u.mesen.%s", outdir, frame, dumps[k].suffix);
            df = fopen(path, "wb");
            if (df) { fwrite(m->sec[dumps[k].sec], 1, dumps[k].size, df); fclose(df); }
            snprintf(path, sizeof(path), "%s/%u.native.%s", outdir, frame, dumps[k].suffix);
            df = fopen(path, "wb");
            if (df) { fwrite(native_parts[k], 1, dumps[k].size, df); fclose(df); }
        }
    }
}

/* ------------------------------------------------------------------------ */

/* Renderer self-test: render Mesen's OWN captured PPU state (CIRAM, palette,
   OAM, CHR, ctrl/mask/t/fine-x/v) with the native PPU renderer and compare to
   Mesen's framebuffer. Isolates renderer bugs from game-state bugs: a frame
   that fails here is either a renderer defect or a frame whose registers were
   rewritten while it was being drawn (level loads). */
static int renderer_selftest(const char *archive_path, unsigned from, unsigned to, const char *outdir,
                             unsigned max_details)
{
    static uint8_t ciram[0x800], palette[0x20], oam[0x100], chr[0x2000];
    static uint8_t colors[FB_PIXELS], canon[FB_PIXELS];
    Archive a;
    ContraPpuMemory mem;
    unsigned frames = 0, failed = 0, details = 0;
    unsigned long long pixels = 0;
    mem.chr = chr; mem.ciram = ciram; mem.palette = palette; mem.oam = oam;
    if (!archive_open(&a, archive_path)) return 1;
    while (archive_next(&a))
    {
        ContraPpuRegs regs;
        const uint8_t *r;
        unsigned i, diff = 0;
        int x0 = FB_W, y0 = FB_H, x1 = -1, y1 = -1;
        if (a.frame < from) continue;
        if (a.frame > to) break;
        if (a.sec[SEC_FB] == NULL || a.sec[SEC_REGS] == NULL || !a.has_chr) continue;
        memcpy(ciram, a.sec[SEC_CIRAM], sizeof(ciram));
        memcpy(palette, a.sec[SEC_PALETTE], sizeof(palette));
        memcpy(oam, a.sec[SEC_OAM], sizeof(oam));
        memcpy(chr, a.chr, sizeof(chr));
        r = a.sec[SEC_REGS];
        memset(&regs, 0, sizeof(regs));
        regs.ctrl = r[0];
        regs.mask = r[1];
        regs.v = (uint16_t)(r[2] | (r[3] << 8));
        regs.t = (uint16_t)(r[4] | (r[5] << 8));
        regs.fine_x = r[6];
        contra_ppu_render_frame(&regs, &mem, colors);
        for (i = 0; i < FB_PIXELS; ++i)
        {
            canon[i] = canonical_index[colors[i] & 0x3Fu];
            if (canon[i] != a.sec[SEC_FB][i])
            {
                const int x = (int)(i % FB_W), y = (int)(i / FB_W);
                ++diff;
                if (x < x0) x0 = x;
                if (x > x1) x1 = x;
                if (y < y0) y0 = y;
                if (y > y1) y1 = y;
            }
        }
        ++frames;
        if (diff)
        {
            ++failed;
            pixels += diff;
            printf("frame %u: %u pixels differ (x %d..%d y %d..%d) ctrl=%02X mask=%02X t=%04X x=%u v=%04X\n",
                   a.frame, diff, x0, x1, y0, y1, r[0], r[1], regs.t, regs.fine_x, regs.v);
            if (details < max_details)
            {
                char path[1024];
                char cmd[1200];
                snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", outdir);
                if (system(cmd) != 0) fprintf(stderr, "warning: mkdir failed\n");
                snprintf(path, sizeof(path), "%s/selftest_%u.png", outdir, a.frame);
                write_triptych(path, a.sec[SEC_FB], canon);
                ++details;
            }
        }
    }
    printf("renderer self-test: %u frames, %u differ (%llu pixels)\n", frames, failed, pixels);
    return failed ? 1 : 0;
}

static int parse_layer(const char *s)
{
    int i;
    if (strcmp(s, "any") == 0) return L_COUNT;
    for (i = 0; i < L_COUNT; ++i)
        if (strcmp(s, layer_names[i]) == 0) return i;
    return -1;
}

int main(int argc, char **argv)
{
    static ContraCore core;
    static Telemetry native;
    static uint8_t render_oam[0x100];
    uint8_t render_regs[5] = {0};
    Archive mesen;
    InputRow *rows;
    unsigned last_row = 0;
    unsigned from = 1, to = 0xFFFFFFFFu;
    const char *report_path = NULL;
    const char *outdir = "tmp/parity";
    const char *symbols_path = "src/ram.asm";
    unsigned detail[MAX_DETAIL];
    unsigned detail_count = 0;
    uint16_t watch[64];
    unsigned watch_count = 0;
    bool addr_histogram = false;
    static unsigned addr_diff_frames[0x800];
    static unsigned addr_first[0x800];
    int stop_layer = -1;
    bool quiet = false;
    FILE *report = NULL;
    const char *input_env = getenv("CONTRA_NATIVE_PLAY_INPUT");
    const char *rng_mode = getenv("CONTRA_NATIVE_PLAY_RNG");
    const int input_mode = (input_env == NULL) ? INPUT_KNOWN_GOOD
                           : (strcmp(input_env, "raw") == 0) ? INPUT_RAW
                           : (strcmp(input_env, "latched") == 0) ? INPUT_LATCHED
                                                                  : INPUT_KNOWN_GOOD;
    const bool rng_free = (rng_mode != NULL) && (strcmp(rng_mode, "free") == 0);
    const char *raster_mode = getenv("CONTRA_NATIVE_PLAY_RASTER");
    const bool raster_free = (raster_mode != NULL) && (strcmp(raster_mode, "free") == 0);
    unsigned frame;
    int argi;
    /* summary accumulators */
    unsigned frames_compared = 0, torn_frames = 0;
    unsigned diverged[L_COUNT] = {0};
    unsigned first_div[L_COUNT];
    unsigned region_frames[RAM_REGION_COUNT] = {0};
    unsigned region_first[RAM_REGION_COUNT];
    unsigned long long region_bytes[RAM_REGION_COUNT] = {0};
    unsigned long long fb_pixels_total = 0;
    unsigned fb_unknown_frames = 0;

    if ((argc >= 3) && (strcmp(argv[1], "--selftest-renderer") == 0))
    {
        init_palette_maps();
        return renderer_selftest(argv[2], (argc > 3) ? (unsigned)strtoul(argv[3], NULL, 10) : 1u,
                                 (argc > 4) ? (unsigned)strtoul(argv[4], NULL, 10) : 0xFFFFFFFFu,
                                 (argc > 5) ? argv[5] : "tmp/parity", 20u);
    }
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s RECORDING.jsonl MESEN.ctel [--from N] [--to N] [--report PATH] "
                        "[--detail F,F] [--outdir DIR] [--stop LAYER] [--symbols PATH] [--watch ADDR[-ADDR],...] [--quiet]\n", argv[0]);
        return 2;
    }
    for (argi = 3; argi < argc; ++argi)
    {
        const char *a = argv[argi];
        const char *v = (argi + 1 < argc) ? argv[argi + 1] : NULL;
        if (strcmp(a, "--quiet") == 0) { quiet = true; continue; }
        if (strcmp(a, "--addr-histogram") == 0) { addr_histogram = true; continue; }
        if (v == NULL) { fprintf(stderr, "missing value for %s\n", a); return 2; }
        if (strcmp(a, "--from") == 0) from = (unsigned)strtoul(v, NULL, 10);
        else if (strcmp(a, "--to") == 0) to = (unsigned)strtoul(v, NULL, 10);
        else if (strcmp(a, "--report") == 0) report_path = v;
        else if (strcmp(a, "--outdir") == 0) outdir = v;
        else if (strcmp(a, "--symbols") == 0) symbols_path = v;
        else if (strcmp(a, "--watch") == 0)
        {
            const char *p = v;
            while (*p && watch_count < 64u)
            {
                char *end;
                const unsigned long lo = strtoul(p, &end, 16);
                unsigned long hi = lo;
                unsigned long w;
                if (end == p) break;
                if (*end == '-') { p = end + 1; hi = strtoul(p, &end, 16); }
                for (w = lo; w <= hi && watch_count < 64u; ++w) watch[watch_count++] = (uint16_t)w;
                p = (*end == ',') ? end + 1 : end;
            }
        }
        else if (strcmp(a, "--stop") == 0)
        {
            stop_layer = parse_layer(v);
            if (stop_layer < 0) { fprintf(stderr, "unknown layer %s\n", v); return 2; }
        }
        else if (strcmp(a, "--detail") == 0)
        {
            const char *p = v;
            while (*p && detail_count < MAX_DETAIL)
            {
                char *end;
                const unsigned long lo = strtoul(p, &end, 10);
                unsigned long hi = lo;
                unsigned long f2;
                if (end == p) break;
                if (*end == '-') { p = end + 1; hi = strtoul(p, &end, 10); }
                for (f2 = lo; f2 <= hi && detail_count < MAX_DETAIL; ++f2) detail[detail_count++] = (unsigned)f2;
                p = (*end == ',') ? end + 1 : end;
            }
        }
        else { fprintf(stderr, "unknown option %s\n", a); return 2; }
        ++argi;
    }

    init_ram_classes();
    init_palette_maps();
    load_symbols(symbols_path);
    for (argi = 0; argi < L_COUNT; ++argi) first_div[argi] = 0;
    for (argi = 0; argi < RAM_REGION_COUNT; ++argi) region_first[argi] = 0;

    rows = load_recording(argv[1], &last_row);
    if (rows == NULL) return 1;
    if (!archive_open(&mesen, argv[2])) return 1;
    if (report_path != NULL)
    {
        report = fopen(report_path, "w");
        if (report == NULL) { fprintf(stderr, "cannot write %s\n", report_path); return 1; }
    }
    if (detail_count)
    {
        char cmd[1200];
        snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", outdir);
        if (system(cmd) != 0) fprintf(stderr, "warning: mkdir %s failed\n", outdir);
    }

    contra_core_init(&core);

    for (frame = 1; frame <= last_row && frame <= to; ++frame)
    {
        ContraInputSnapshot input = {{0u, 0u}};
        const bool lag = is_lag_row(rows, last_row, frame) &&
                         (core.startup_wait_frames == 0u) && (core.level_graphics_wait_frames == 0u) &&
                         (core.frame_stall_frames == 0u);
        FrameDiff d;
        unsigned k;

        if (!archive_next(&mesen))
        {
            if (!quiet) fprintf(stderr, "archive ended at frame %u\n", frame - 1);
            break;
        }
        if (mesen.frame != frame)
        {
            fprintf(stderr, "FAIL archive frame %u != replay frame %u\n", mesen.frame, frame);
            return 1;
        }
        if (!lag && rows[frame].has_row)
        {
            const InputRow *row = &rows[frame];
            if (input_mode == INPUT_RAW)
            {
                input.player[0] = row->p1_raw;
                input.player[1] = row->p2_raw;
            }
            else if (input_mode == INPUT_LATCHED)
            {
                input.player[0] = row->p1_latched;
                input.player[1] = row->p2_latched;
            }
            else
            {
                /* the pad bits the ROM's load_controller_state validated this
                   frame (CTRL_KNOWN_GOOD, before any scripted input such as
                   the end-of-level walk overwrites CONTROLLER_STATE) */
                input.player[0] = mesen.sec[SEC_RAM][0xF9u];
                input.player[1] = mesen.sec[SEC_RAM][0xFAu];
            }
            if (row->has_rng && !rng_free)
                core.ram[CONTRA_RAM_RANDOM_NUM] = (uint8_t)(row->rng - core.ram[CONTRA_RAM_FRAME_COUNTER]);
        }
        if (!lag)
        {
            contra_core_set_input(&core, &input);
            if (!raster_free)
                core.raster_cut_hint = reference_raster_cut(mesen.sec[SEC_REG_LOG], mesen.sec_len[SEC_REG_LOG]);
            contra_core_step_frame(&core);
        }
        else
        {
            contra_core_step_lag_frame(&core);
        }
        /* the port's step mirrors the NMI: OAM DMA'd, registers written, the
           frame scanned out -- the same moment as Mesen's endFrame */
        memcpy(render_oam, core.ppu_oam, sizeof(render_oam));
        native_render_regs(&core, render_regs);
        if (frame < from) continue;

        capture_native(&core, render_oam, render_regs, &native);
        compare_frame(&mesen, &native, &d);
        if (addr_histogram && !is_torn_row(rows, last_row, frame))
        {
            unsigned ai;
            for (ai = 0; ai < 0x800u; ++ai)
            {
                if (ram_class[ai] == CLS_GAME && mesen.sec[SEC_RAM][ai] != native.ram[ai])
                {
                    if (!addr_diff_frames[ai]++) addr_first[ai] = frame;
                }
            }
        }
        if (watch_count)
        {
            unsigned w;
            bool any = false;
            for (w = 0; w < watch_count; ++w)
                if (mesen.sec[SEC_RAM][watch[w]] != native.ram[watch[w]]) any = true;
            printf("%6u %s%s w%02X s%02X", frame, any ? "*" : " ", lag ? "L" : " ",
                   core.level_graphics_wait_frames, core.frame_stall_frames);
            for (w = 0; w < watch_count; ++w)
            {
                const uint8_t mv = mesen.sec[SEC_RAM][watch[w]], nv = native.ram[watch[w]];
                if (mv == nv) printf(" %03X=%02X   ", watch[w], mv);
                else printf(" %03X=%02X/%02X", watch[w], mv, nv);
            }
            printf("\n");
        }
        ++frames_compared;
        {
            const bool torn = is_torn_row(rows, last_row, frame);
            if (torn) ++torn_frames;
            for (k = 0; k < L_COUNT; ++k)
            {
                if (d.count[k] && !torn)
                {
                    if (!diverged[k]++) first_div[k] = frame;
                }
            }
            if (!torn)
            {
                for (k = 0; k < (unsigned)RAM_REGION_COUNT; ++k)
                {
                    if (d.region[k])
                    {
                        if (!region_frames[k]++) region_first[k] = frame;
                        region_bytes[k] += d.region[k];
                    }
                }
                fb_pixels_total += d.count[L_FB];
            }
            if (d.fb_unknown) ++fb_unknown_frames;

            if (report != NULL)
            {
                fprintf(report, "{\"f\":%u,\"stage\":\"%s\",\"torn\":%d", frame, stage_of(mesen.sec[SEC_RAM]),
                        torn ? 1 : 0);
                for (k = 0; k < L_COUNT; ++k) fprintf(report, ",\"%s\":%u", layer_names[k], d.count[k]);
                fprintf(report, ",\"regions\":{");
                {
                    bool first = true;
                    for (k = 0; k < (unsigned)RAM_REGION_COUNT; ++k)
                    {
                        if (d.region[k])
                        {
                            fprintf(report, "%s\"%s@%03X\":%u", first ? "" : ",", ram_regions[k].name,
                                    ram_regions[k].lo, d.region[k]);
                            first = false;
                        }
                    }
                }
                fprintf(report, "}");
                if (d.count[L_FB])
                    fprintf(report, ",\"fbbox\":[%d,%d,%d,%d]", d.fb_x0, d.fb_y0, d.fb_x1, d.fb_y1);
                fprintf(report, "}\n");
            }

            for (k = 0; k < detail_count; ++k)
            {
                if (detail[k] == frame)
                {
                    write_detail(outdir, frame, &mesen, &native, &d);
                    break;
                }
            }

            if (stop_layer >= 0 && !torn)
            {
                bool hit = false;
                if (stop_layer == L_COUNT)
                {
                    for (k = 0; k < L_COUNT; ++k)
                        if (k != L_SOUND && k != L_STACK && k != L_RNG && d.count[k]) hit = true;
                }
                else
                {
                    hit = d.count[stop_layer] != 0;
                }
                if (hit)
                {
                    printf("STOP at frame %u (%s): layer %s differs\n", frame, stage_of(mesen.sec[SEC_RAM]),
                           stop_layer == L_COUNT ? "any" : layer_names[stop_layer]);
                    write_detail(outdir, frame, &mesen, &native, &d);
                    printf("detail: %s/%u.txt %s/%u.png\n", outdir, frame, outdir, frame);
                    break;
                }
            }
        }
    }

    if (report) fclose(report);

    printf("compared %u frames (%u torn lag-burst frames excluded from the tallies)\n", frames_compared,
           torn_frames);
    printf("%-10s %10s %12s\n", "layer", "frames≠", "first");
    for (argi = 0; argi < L_COUNT; ++argi)
    {
        if (diverged[argi])
            printf("%-10s %10u %12u\n", layer_names[argi], diverged[argi], first_div[argi]);
        else
            printf("%-10s %10s %12s\n", layer_names[argi], "0", "IDENTICAL");
    }
    printf("\nRAM regions (frames with a difference / first frame / total differing bytes):\n");
    for (argi = 0; argi < RAM_REGION_COUNT; ++argi)
    {
        if (region_frames[argi])
            printf("  %-15s $%03X-$%03X  %6u  %6u  %10llu  [%s]\n", ram_regions[argi].name, ram_regions[argi].lo,
                   ram_regions[argi].hi - 1u, region_frames[argi], region_first[argi], region_bytes[argi],
                   class_names[ram_regions[argi].cls]);
    }
    if (addr_histogram)
    {
        unsigned ai;
        printf("\ngame RAM addresses that differ (frames / first frame):\n");
        for (ai = 0; ai < 0x800u; ++ai)
        {
            if (addr_diff_frames[ai])
            {
                char name[64];
                symbol_name((uint16_t)ai, name, sizeof(name));
                printf("  $%03X %-36s %6u %6u\n", ai, name, addr_diff_frames[ai], addr_first[ai]);
            }
        }
    }
    printf("pixels differing in total: %llu", fb_pixels_total);
    if (fb_unknown_frames) printf("  (WARNING: %u mesen frames contain colors outside the palette)", fb_unknown_frames);
    printf("\n");
    free(rows);
    return 0;
}
