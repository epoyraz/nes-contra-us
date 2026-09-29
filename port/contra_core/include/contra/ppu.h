#ifndef CONTRA_PPU_H
#define CONTRA_PPU_H

#include <stdint.h>

/*
 * A 2C02 model at the granularity Contra needs: the CPU-visible register
 * interface ($2000-$2007 + OAM DMA) with the exact loopy v/t/x/w semantics,
 * and a frame renderer that reproduces what the real PPU scans out for a
 * frame whose registers are not touched while it is being drawn (every
 * Contra frame except the handful where a level load switches rendering off
 * mid-picture).
 *
 * Memory is owned by the caller (ContraCore): CHR RAM (8 KiB), CIRAM (2 KiB,
 * vertical mirroring -- the cartridge ties CIRAM A10 to PPU A10), palette RAM
 * (32 B, with the $3F10/$3F14/$3F18/$3F1C mirrors of the backdrop entries) and
 * primary OAM (256 B).
 */

typedef struct ContraPpuRegs
{
    uint8_t ctrl;        /* last $2000 write */
    uint8_t mask;        /* last $2001 write */
    uint16_t v;          /* current VRAM address */
    uint16_t t;          /* temporary VRAM address (scroll latch) */
    uint8_t fine_x;      /* fine X scroll */
    uint8_t w;           /* $2005/$2006 write toggle */
    uint8_t oam_addr;    /* $2003 */
    uint8_t read_buffer; /* $2007 read buffer */
} ContraPpuRegs;

typedef struct ContraPpuMemory
{
    uint8_t *chr;     /* 0x2000 */
    uint8_t *ciram;   /* 0x800 */
    uint8_t *palette; /* 0x20 */
    uint8_t *oam;     /* 0x100 */
} ContraPpuMemory;

/* PPU address space access (what $2007 reaches) */
uint8_t contra_ppu_peek(const ContraPpuMemory *mem, uint16_t addr);
void contra_ppu_poke(const ContraPpuMemory *mem, uint16_t addr, uint8_t value);

/* CPU register writes/reads, loopy-exact */
void contra_ppu_write_ctrl(ContraPpuRegs *regs, uint8_t value);   /* $2000 */
void contra_ppu_write_mask(ContraPpuRegs *regs, uint8_t value);   /* $2001 */
void contra_ppu_read_status(ContraPpuRegs *regs);                 /* $2002 (resets w) */
void contra_ppu_write_oam_addr(ContraPpuRegs *regs, uint8_t value); /* $2003 */
void contra_ppu_write_scroll(ContraPpuRegs *regs, uint8_t value); /* $2005 */
void contra_ppu_write_addr(ContraPpuRegs *regs, uint8_t value);   /* $2006 */
void contra_ppu_write_data(ContraPpuRegs *regs, const ContraPpuMemory *mem, uint8_t value); /* $2007 */
uint8_t contra_ppu_read_data(ContraPpuRegs *regs, const ContraPpuMemory *mem);              /* $2007 */
/* $4014: copy 256 bytes into OAM starting at OAMADDR */
void contra_ppu_oam_dma(ContraPpuRegs *regs, const ContraPpuMemory *mem, const uint8_t *page);

/* Render one frame. `regs` is the register state at the start of the frame
   (after the NMI's writes): scrolling comes from t/fine_x, the forced-blank
   backdrop from v. Output: one palette RAM color (0-63) per pixel, 256x240. */
void contra_ppu_render_frame(const ContraPpuRegs *regs, const ContraPpuMemory *mem, uint8_t *out_colors);

/* clear_ppu (sta PPUADDR, PPUADDR, PPUCTRL, PPUMASK, 12 dots apart) executed
   while `line` is being drawn, its $2001 write at dot `off_dot`: re-render
   that line's pixels left of the cut the way the PPU fetched them (v reset
   to $0000 and the BG pattern table switched to $0000 mid-line). `regs` and
   `mem` are the frame's scan-out state. */
void contra_ppu_render_clear_ppu_line(const ContraPpuRegs *regs, const ContraPpuMemory *mem, uint8_t *out_colors,
                                      unsigned line, unsigned off_dot);

#endif
