#include "contra/ppu.h"

#include <string.h>

enum
{
    PPU_WIDTH = 256,
    PPU_HEIGHT = 240
};

/* palette RAM index for a $3Fxx address: $3F10/$3F14/$3F18/$3F1C are the
   backdrop entries $3F00/$3F04/$3F08/$3F0C */
static uint8_t palette_index(uint16_t addr)
{
    uint8_t index = (uint8_t)(addr & 0x1Fu);

    if ((index & 0x13u) == 0x10u)
    {
        index = (uint8_t)(index & 0x0Fu);
    }
    return index;
}

uint8_t contra_ppu_peek(const ContraPpuMemory *mem, uint16_t addr)
{
    addr = (uint16_t)(addr & 0x3FFFu);
    if (addr < 0x2000u)
    {
        return mem->chr[addr];
    }
    if (addr < 0x3F00u)
    {
        /* vertical mirroring: $2000/$2800 -> CIRAM A, $2400/$2C00 -> CIRAM B */
        return mem->ciram[addr & 0x07FFu];
    }
    return mem->palette[palette_index(addr)];
}

void contra_ppu_poke(const ContraPpuMemory *mem, uint16_t addr, uint8_t value)
{
    addr = (uint16_t)(addr & 0x3FFFu);
    if (addr < 0x2000u)
    {
        mem->chr[addr] = value;
    }
    else if (addr < 0x3F00u)
    {
        mem->ciram[addr & 0x07FFu] = value;
    }
    else
    {
        const uint8_t index = palette_index(addr);

        /* 6-bit palette RAM; keep both halves of the backdrop mirrors in
           sync so a dump of the 32 bytes reads like the real (and Mesen's)
           palette RAM */
        mem->palette[index] = (uint8_t)(value & 0x3Fu);
        if ((index & 0x03u) == 0u)
        {
            mem->palette[index | 0x10u] = (uint8_t)(value & 0x3Fu);
        }
    }
}

void contra_ppu_write_ctrl(ContraPpuRegs *regs, uint8_t value)
{
    regs->ctrl = value;
    regs->t = (uint16_t)((regs->t & 0xF3FFu) | ((uint16_t)(value & 0x03u) << 10));
}

void contra_ppu_write_mask(ContraPpuRegs *regs, uint8_t value)
{
    regs->mask = value;
}

void contra_ppu_read_status(ContraPpuRegs *regs)
{
    regs->w = 0u;
}

void contra_ppu_write_oam_addr(ContraPpuRegs *regs, uint8_t value)
{
    regs->oam_addr = value;
}

void contra_ppu_write_scroll(ContraPpuRegs *regs, uint8_t value)
{
    if (regs->w == 0u)
    {
        regs->t = (uint16_t)((regs->t & 0xFFE0u) | (uint16_t)(value >> 3));
        regs->fine_x = (uint8_t)(value & 0x07u);
        regs->w = 1u;
    }
    else
    {
        regs->t = (uint16_t)((regs->t & 0x8C1Fu) | ((uint16_t)(value & 0x07u) << 12) |
                             ((uint16_t)(value & 0xF8u) << 2));
        regs->w = 0u;
    }
}

void contra_ppu_write_addr(ContraPpuRegs *regs, uint8_t value)
{
    if (regs->w == 0u)
    {
        regs->t = (uint16_t)((regs->t & 0x80FFu) | ((uint16_t)(value & 0x3Fu) << 8));
        regs->w = 1u;
    }
    else
    {
        regs->t = (uint16_t)((regs->t & 0xFF00u) | value);
        regs->v = regs->t;
        regs->w = 0u;
    }
}

static void increment_v(ContraPpuRegs *regs)
{
    regs->v = (uint16_t)((regs->v + (((regs->ctrl & 0x04u) != 0u) ? 32u : 1u)) & 0x7FFFu);
}

void contra_ppu_write_data(ContraPpuRegs *regs, const ContraPpuMemory *mem, uint8_t value)
{
    contra_ppu_poke(mem, regs->v, value);
    increment_v(regs);
}

uint8_t contra_ppu_read_data(ContraPpuRegs *regs, const ContraPpuMemory *mem)
{
    const uint16_t addr = (uint16_t)(regs->v & 0x3FFFu);
    uint8_t result;

    if (addr < 0x3F00u)
    {
        result = regs->read_buffer;
        regs->read_buffer = contra_ppu_peek(mem, addr);
    }
    else
    {
        /* palette reads are immediate; the buffer gets the nametable byte
           "underneath" the palette */
        result = contra_ppu_peek(mem, addr);
        regs->read_buffer = contra_ppu_peek(mem, (uint16_t)(addr - 0x1000u));
    }
    increment_v(regs);
    return result;
}

void contra_ppu_oam_dma(ContraPpuRegs *regs, const ContraPpuMemory *mem, const uint8_t *page)
{
    unsigned index;

    for (index = 0u; index < 0x100u; ++index)
    {
        const uint8_t addr = (uint8_t)(regs->oam_addr + index);

        /* attribute bytes have no storage for bits 2-4 (they read back 0) */
        mem->oam[addr] = ((addr & 0x03u) == 0x02u) ? (uint8_t)(page[index] & 0xE3u) : page[index];
    }
}

/* loopy "increment vertical position" (dot 256 of every rendering line) */
static uint16_t increment_y(uint16_t v)
{
    if ((v & 0x7000u) != 0x7000u)
    {
        return (uint16_t)(v + 0x1000u);
    }
    {
        unsigned coarse_y = (v & 0x03E0u) >> 5;

        v = (uint16_t)(v & ~0x7000u);
        if (coarse_y == 29u)
        {
            coarse_y = 0u;
            v ^= 0x0800u;
        }
        else if (coarse_y == 31u)
        {
            coarse_y = 0u; /* attribute rows: wraps WITHOUT switching nametables */
        }
        else
        {
            ++coarse_y;
        }
        return (uint16_t)((v & ~0x03E0u) | (coarse_y << 5));
    }
}

/* Mid-scanline register writes that change background fetches on one line:
   v <- v_value once dot v_dot is done, the BG pattern table <- bg_base once
   dot ctrl_dot is done. Like Mesen, a tile's pattern address (table base and
   fine Y) is fixed at its nametable fetch -- dot 8 * (t - 2) + 1 for the
   line's tile t (tiles 0 and 1 come from the previous line's prefetch). */
typedef struct LineFetchOverride
{
    unsigned v_dot;
    uint16_t v_value;
    unsigned ctrl_dot;
    uint16_t bg_base;
} LineFetchOverride;

/* loopy "increment horizontal position" (every 8th dot while rendering) */
static uint16_t increment_x(uint16_t v)
{
    if ((v & 0x001Fu) == 31u)
    {
        return (uint16_t)((v & ~0x001Fu) ^ 0x0400u);
    }
    return (uint16_t)(v + 1u);
}

/* one rendering line; v is the line's scroll after the dot-257 t -> v copy */
static void render_line(const ContraPpuRegs *regs, const ContraPpuMemory *mem, unsigned y, uint16_t v,
                        const LineFetchOverride *ovr, uint8_t *out)
{
    const uint8_t mask = regs->mask;
    const uint8_t ctrl = regs->ctrl;
    const int bg_enabled = (mask & 0x08u) != 0u;
    const int sprites_enabled = (mask & 0x10u) != 0u;
    const unsigned sprite_height = ((ctrl & 0x20u) != 0u) ? 16u : 8u;
    const uint16_t bg_pattern_base = ((ctrl & 0x10u) != 0u) ? 0x1000u : 0x0000u;
    const uint16_t sprite_pattern_base_8x8 = ((ctrl & 0x08u) != 0u) ? 0x1000u : 0x0000u;
    uint8_t bg_pixel[PPU_WIDTH];       /* 0-3 */
    uint8_t bg_palette[PPU_WIDTH];     /* 0-3 */
    uint8_t sprite_pixel[PPU_WIDTH];   /* 0 = none, else palette index 0x11-0x1F */
    uint8_t sprite_behind[PPU_WIDTH];
    unsigned x;

    memset(bg_pixel, 0, sizeof(bg_pixel));
    memset(bg_palette, 0, sizeof(bg_palette));
    memset(sprite_pixel, 0, sizeof(sprite_pixel));
    memset(sprite_behind, 0, sizeof(sprite_behind));

    if (bg_enabled)
    {
        unsigned tile;

        /* 33 tiles cover 256 pixels at any fine x */
        for (tile = 0u; tile < 33u; ++tile)
        {
            const unsigned cx = (v & 0x1Fu) + tile;
            uint16_t tv = (uint16_t)((v & ~0x041Fu) | ((v & 0x0C00u) ^ ((cx >= 32u) ? 0x0400u : 0u)) | (cx & 0x1Fu));
            uint16_t base = bg_pattern_base;
            unsigned fine_y;
            unsigned coarse_y;
            unsigned coarse_x;
            uint16_t nt_bits;
            uint8_t tile_index;
            uint8_t attr;
            uint8_t palette;
            uint16_t pattern;
            uint8_t lo;
            uint8_t hi;
            unsigned px;

            if ((ovr != NULL) && (tile >= 2u))
            {
                const unsigned nt_dot = 8u * (tile - 2u) + 1u;

                if (nt_dot > ovr->v_dot)
                {
                    /* coarse X increments on dots 8, 16, ... after the write */
                    unsigned increments = (nt_dot - 1u) / 8u - ovr->v_dot / 8u;

                    tv = ovr->v_value;
                    while (increments-- != 0u)
                    {
                        tv = increment_x(tv);
                    }
                }
                if (nt_dot > ovr->ctrl_dot)
                {
                    base = ovr->bg_base;
                }
            }
            fine_y = (tv >> 12) & 0x07u;
            coarse_y = (tv >> 5) & 0x1Fu;
            coarse_x = tv & 0x1Fu;
            nt_bits = (uint16_t)(tv & 0x0C00u);
            tile_index = contra_ppu_peek(mem, (uint16_t)(0x2000u | nt_bits | (coarse_y << 5) | coarse_x));
            attr = contra_ppu_peek(mem, (uint16_t)(0x23C0u | nt_bits | ((coarse_y >> 2) << 3) | (coarse_x >> 2)));
            palette = (uint8_t)((attr >> (((coarse_y & 0x02u) << 1) | (coarse_x & 0x02u))) & 0x03u);
            pattern = (uint16_t)(base + (uint16_t)tile_index * 16u + fine_y);
            lo = mem->chr[pattern & 0x1FFFu];
            hi = mem->chr[(pattern + 8u) & 0x1FFFu];

            for (px = 0u; px < 8u; ++px)
            {
                const int screen_x = (int)(tile * 8u + px) - (int)regs->fine_x;
                const unsigned bit = 7u - px;
                const uint8_t value = (uint8_t)(((lo >> bit) & 1u) | (((hi >> bit) & 1u) << 1));

                if ((screen_x < 0) || (screen_x >= PPU_WIDTH))
                {
                    continue;
                }
                if ((screen_x < 8) && ((mask & 0x02u) == 0u))
                {
                    continue; /* left 8 pixels clipped */
                }
                bg_pixel[screen_x] = value;
                bg_palette[screen_x] = palette;
            }
        }
    }

    /* sprites: evaluation on line y-1 selects the first 8 sprites (OAM
       order) whose Y+1..Y+height covers line y; nothing is displayed on
       line 0 */
    if (sprites_enabled && (y > 0u))
    {
        unsigned found = 0u;
        unsigned index;
        uint8_t chosen[8];

        for (index = 0u; (index < 64u) && (found < 8u); ++index)
        {
            const unsigned row = (y - 1u) - (unsigned)mem->oam[index * 4u];

            if (((int)(y - 1u) - (int)mem->oam[index * 4u] >= 0) && (row < sprite_height))
            {
                chosen[found++] = (uint8_t)index;
            }
        }

        /* draw in reverse so the lowest OAM index wins */
        while (found-- != 0u)
        {
            const uint8_t *sprite = &mem->oam[chosen[found] * 4u];
            const uint8_t attr = sprite[2];
            unsigned row = (y - 1u) - (unsigned)sprite[0];
            uint16_t pattern;
            uint8_t lo;
            uint8_t hi;
            unsigned px;

            if ((attr & 0x80u) != 0u)
            {
                row = sprite_height - 1u - row;
            }
            if (sprite_height == 16u)
            {
                const uint16_t bank = ((sprite[1] & 0x01u) != 0u) ? 0x1000u : 0x0000u;
                const uint8_t tile = (uint8_t)((sprite[1] & 0xFEu) + ((row >= 8u) ? 1u : 0u));

                pattern = (uint16_t)(bank + (uint16_t)tile * 16u + (row & 0x07u));
            }
            else
            {
                pattern = (uint16_t)(sprite_pattern_base_8x8 + (uint16_t)sprite[1] * 16u + row);
            }
            lo = mem->chr[pattern & 0x1FFFu];
            hi = mem->chr[(pattern + 8u) & 0x1FFFu];

            for (px = 0u; px < 8u; ++px)
            {
                const unsigned screen_x = (unsigned)sprite[3] + px;
                const unsigned bit = ((attr & 0x40u) != 0u) ? px : (7u - px);
                const uint8_t value = (uint8_t)(((lo >> bit) & 1u) | (((hi >> bit) & 1u) << 1));

                if ((screen_x >= PPU_WIDTH) || (value == 0u))
                {
                    continue;
                }
                if ((screen_x < 8u) && ((mask & 0x04u) == 0u))
                {
                    continue;
                }
                sprite_pixel[screen_x] = (uint8_t)(0x10u | ((attr & 0x03u) << 2) | value);
                sprite_behind[screen_x] = (uint8_t)((attr & 0x20u) != 0u);
            }
        }
    }

    for (x = 0u; x < PPU_WIDTH; ++x)
    {
        uint8_t index;

        if ((sprite_pixel[x] != 0u) && ((bg_pixel[x] == 0u) || (sprite_behind[x] == 0u)))
        {
            index = sprite_pixel[x];
        }
        else if (bg_pixel[x] != 0u)
        {
            index = (uint8_t)((bg_palette[x] << 2) | bg_pixel[x]);
        }
        else
        {
            index = 0u;
        }
        out[x] = (uint8_t)(mem->palette[index] & 0x3Fu);
    }
}

void contra_ppu_render_frame(const ContraPpuRegs *regs, const ContraPpuMemory *mem, uint8_t *out_colors)
{
    uint16_t v;
    unsigned y;

    if ((regs->mask & 0x18u) == 0u)
    {
        /* forced blank: the PPU outputs the backdrop color -- or, when v
           points into palette RAM, the color at v (the "background palette
           hack") */
        const uint8_t color = ((regs->v & 0x3F00u) == 0x3F00u)
            ? mem->palette[palette_index(regs->v)]
            : mem->palette[0];
        memset(out_colors, color & 0x3Fu, (size_t)PPU_WIDTH * PPU_HEIGHT);
        return;
    }

    /* pre-render line dots 280-304: v = t (vertical), dot 257: horizontal */
    v = regs->t;
    for (y = 0u; y < PPU_HEIGHT; ++y)
    {
        /* dot 257 of the previous line (or the pre-render line) copies the
           horizontal scroll bits t -> v */
        v = (uint16_t)((v & ~0x041Fu) | (regs->t & 0x041Fu));
        render_line(regs, mem, y, v, NULL, out_colors + (size_t)y * PPU_WIDTH);
        /* dot 256: fine/coarse Y increment */
        v = increment_y(v);
    }
}

void contra_ppu_render_clear_ppu_line(const ContraPpuRegs *regs, const ContraPpuMemory *mem, uint8_t *out_colors,
                                      unsigned line, unsigned off_dot)
{
    LineFetchOverride ovr;
    uint8_t scanned[PPU_WIDTH];
    uint16_t v;
    unsigned y;
    unsigned x;

    if (((regs->mask & 0x18u) == 0u) || (line >= PPU_HEIGHT) || (off_dot < 37u))
    {
        return; /* nothing rendered, or the writes began on the line before */
    }
    v = regs->t;
    for (y = 0u; y <= line; ++y)
    {
        v = (uint16_t)((v & ~0x041Fu) | (regs->t & 0x041Fu));
        if (y < line)
        {
            v = increment_y(v);
        }
    }
    /* clear_ppu: sta PPUADDR x2 (t = $0000; v follows 3 dots after the second
       write), sta PPUCTRL (#$00), sta PPUMASK -- 12 dots apart */
    ovr.v_dot = off_dot - 24u + 3u;
    ovr.v_value = 0x0000u;
    ovr.ctrl_dot = off_dot - 12u;
    ovr.bg_base = 0x0000u;
    render_line(regs, mem, line, v, &ovr, scanned);
    for (x = 0u; (x < off_dot) && (x < PPU_WIDTH); ++x)
    {
        out_colors[(size_t)line * PPU_WIDTH + x] = scanned[x];
    }
}
