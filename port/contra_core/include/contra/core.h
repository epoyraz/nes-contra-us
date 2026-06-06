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
    /* Sizes the level-2 wall structure render caches (l2_structure_tile /
       l2_supertile) — one slot per enemy object slot. */
    CONTRA_NATIVE_MAX_ENEMIES = 24u,
    CONTRA_CPU_SPRITE_RENDER_SLOTS = 26u
};

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
    /* Render cache for the faithful level-2 (indoor base) wall turret/core: the
       level_2_4_tile_animation offset currently shown on the background for each
       enemy slot. The original game leaves this on the nametable; the native
       background composes from level_screen_supertiles (super-tile granularity)
       and can't hold sub-super-tile pattern writes, so the wall structures are
       re-drawn as a framebuffer overlay each frame from this cache. 0 = nothing
       drawn yet. Set at the routines' update_enemy_nametable_tiles points,
       cleared on enemy init/clear. */
    uint8_t l2_structure_tile[CONTRA_NATIVE_MAX_ENEMIES];
    /* Companion render cache for the level-2 boss-room fortress targets (wall
       cannon 0x08 / wall plating 0x0A), which draw as 4x4 background SUPER-tiles
       (draw_enemy_supertile_a, level_2_nametable_update_supertile_data) rather
       than the 2x2 tile-animation the core/turret use. 0xFF = nothing drawn. */
    uint8_t l2_supertile[CONTRA_NATIVE_MAX_ENEMIES];
    /* Bitmask of the back-wall blow-open quadrants drawn so far when a level-2
       wall core is destroyed (wall_core_routine_08). They persist as super-tiles
       at FIXED back-wall positions (not the enemy slot), so they live here rather
       than in l2_supertile; reset on room load. */
    uint8_t l2_blowopen_quadrants;
    /* Faithful level-1 exploding bridge: world positions of the background
       super-tiles whose collision the bridge has cleared (clear_supertile_bg_
       collision) so the player falls through. The native background composes from
       the level supertile data and has no per-tile collision array like the ROM's
       BG_COLLISION_DATA, so cleared cells are recorded here and consulted by the
       outdoor collision lookup. Reset on level load. */
    uint16_t l1_bridge_gap_world_x[16];
    uint8_t l1_bridge_gap_screen_y[16];
    /* Destroyed-bridge super-tile index drawn for each cleared cell. The native
       background re-composes from the level supertile data every frame and does
       not persist nametable writes, so (like the level-2 wall structures) the
       destroyed super-tiles must be re-drawn each frame from this record or the
       intact bridge re-appears over the gap. */
    uint8_t l1_bridge_gap_tile[16];
    uint8_t l1_bridge_gap_count;
    /* Render cache for faithful level-1 background super-tile enemies (red turret
       0x07, rotating gun 0x04, door tunnel, etc.). Like the level-2 wall
       structures and the bridge gaps above, the native L1 background re-composes
       from the level super-tile data every frame and does not persist the
       routines' one-shot nametable writes, so each active enemy's current
       super-tile is re-drawn every frame from this per-slot cache or it is
       invisible. 0xFF = nothing drawn. */
    uint8_t l1_supertile[CONTRA_NATIVE_MAX_ENEMIES];
} ContraCore;

void contra_core_init(ContraCore *core);
void contra_core_reset(ContraCore *core);
void contra_core_set_input(ContraCore *core, const ContraInputSnapshot *input);
void contra_core_step_frame(ContraCore *core);

const uint32_t *contra_core_framebuffer(const ContraCore *core);
const uint8_t *contra_core_ram(const ContraCore *core);

#endif
