#ifndef CONTRA_CORE_H
#define CONTRA_CORE_H

#include <stdint.h>

#include "contra/ram.h"

typedef struct ContraInputSnapshot
{
    uint8_t player[2];
} ContraInputSnapshot;

enum
{
    CONTRA_NATIVE_MAX_ENEMIES = 24u,
    CONTRA_NATIVE_MAX_ENEMY_PROJECTILES = 8u,
    CONTRA_CPU_SPRITE_RENDER_SLOTS = 26u
};

typedef struct ContraNativeEnemy
{
    uint8_t active;
    uint8_t type;
    uint8_t attrs;
    uint8_t hp;
    uint8_t state;
    uint8_t timer;
    uint8_t cooldown;
    uint8_t flags;
    uint8_t screen_id;
    uint8_t sprite_code;
    uint8_t sprite_attr;
    int16_t x;
    int16_t y;
    int8_t vx;
    int8_t vy;
    uint8_t origin_x;
    uint8_t origin_y;
    uint8_t x_frac;
    uint8_t y_frac;
    uint8_t x_accum;
    uint8_t y_accum;
} ContraNativeEnemy;

typedef struct ContraNativeProjectile
{
    uint8_t active;
    uint8_t damage;
    uint8_t sprite_code;
    uint8_t sprite_attr;
    uint8_t owner;
    uint8_t timer;
    int16_t x;
    int16_t y;
    int8_t vx;
    int8_t vy;
} ContraNativeProjectile;

typedef struct ContraCore
{
    uint8_t ram[CONTRA_CPU_RAM_SIZE];
    uint8_t ppu_pattern[CONTRA_PPU_PATTERN_TABLE_SIZE];
    uint8_t ppu_nametable[CONTRA_PPU_NAMETABLE_SIZE];
    uint8_t ppu_palette[CONTRA_PPU_PALETTE_SIZE];
    uint8_t level_screen_supertiles[CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE];
    uint32_t framebuffer[CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT];
    uint8_t background_opaque[CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT];
    uint8_t sprite_priority[CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT];
    uint8_t latched_cpu_sprite_buffer[CONTRA_CPU_SPRITE_RENDER_SLOTS];
    uint8_t latched_sprite_y[CONTRA_CPU_SPRITE_RENDER_SLOTS];
    uint8_t latched_sprite_x[CONTRA_CPU_SPRITE_RENDER_SLOTS];
    uint8_t latched_sprite_attr[CONTRA_CPU_SPRITE_RENDER_SLOTS];
    uint8_t latched_sprite_load_type;
    uint8_t latched_demo_mode;
    uint8_t latched_player_mode;
    uint8_t latched_num_lives[2];
    uint8_t latched_game_over_status[2];
    uint8_t latched_oam[0x100u];
    uint8_t latched_horizontal_scroll;
    uint8_t latched_vertical_scroll;
    uint8_t latched_level_screen_number;
    uint8_t latched_level_screen_scroll_offset;
    uint8_t latched_ppuctrl_settings;
    uint8_t pending_palette_write;
    uint8_t pending_palette_count;
    uint8_t pending_palette[CONTRA_PPU_PALETTE_SIZE];
    ContraNativeEnemy enemies[CONTRA_NATIVE_MAX_ENEMIES];
    ContraNativeProjectile enemy_projectiles[CONTRA_NATIVE_MAX_ENEMY_PROJECTILES];
    ContraInputSnapshot pending_input;
    uint8_t startup_wait_frames;
    uint8_t level_graphics_wait_frames;
    uint8_t level1_weapon_box_restore_timer;
    int16_t level1_weapon_box_restore_x;
    int16_t level1_weapon_box_restore_y;
    uint8_t pending_horizontal_column_write;
    uint8_t pending_horizontal_column_tile_offset;
    uint8_t pending_horizontal_column_supertile_offset;
    uint16_t pending_horizontal_column_ppu_addr;
    uint8_t pending_horizontal_attr_write;
    uint8_t pending_horizontal_attr_tile_offset;
    uint8_t pending_horizontal_attr_supertile_offset;
    uint8_t pending_horizontal_attr_high;
} ContraCore;

void contra_core_init(ContraCore *core);
void contra_core_reset(ContraCore *core);
void contra_core_set_input(ContraCore *core, const ContraInputSnapshot *input);
void contra_core_step_frame(ContraCore *core);

const uint32_t *contra_core_framebuffer(const ContraCore *core);
const uint8_t *contra_core_ram(const ContraCore *core);

#endif
