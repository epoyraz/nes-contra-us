/* Persistent background overlays and final native frame assembly.
   Included by core.c; not compiled as a separate translation unit. */


static void contra_render_overlay_supertile(
    ContraCore *core,
    uint16_t supertile_data_addr,
    uint16_t palette_data_addr,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    const uint8_t stripped_index = (uint8_t)(supertile_index & 0x7Fu);
    const uint16_t supertile_addr = (uint16_t)(
        supertile_data_addr + ((uint16_t)stripped_index * 16u)
    );
    const uint8_t supertile_palette = contra_rom_read_u8(
        3u,
        (uint16_t)(palette_data_addr + stripped_index)
    );
    unsigned tile_y;

    for (tile_y = 0u; tile_y < 4u; ++tile_y)
    {
        unsigned tile_x;

        for (tile_x = 0u; tile_x < 4u; ++tile_x)
        {
            const uint8_t pattern_index = contra_rom_read_u8(
                3u,
                (uint16_t)(supertile_addr + ((uint16_t)tile_y * 4u) + tile_x)
            );
            const uint8_t palette_shift = (uint8_t)(((tile_y & 0x02u) << 1u) | (tile_x & 0x02u));
            const uint8_t palette_slot = (uint8_t)((supertile_palette >> palette_shift) & 0x03u);

            contra_draw_background_tile(
                core,
                dest_x + (int)(tile_x * 8u),
                dest_y + (int)(tile_y * 8u),
                pattern_index,
                palette_slot
            );
        }
    }
}

static void contra_calculate_update_supertile_ppu_addr(
    const ContraCore *core,
    int dest_x,
    int dest_y,
    uint16_t *tile_ppu_addr,
    uint16_t *attr_ppu_addr
)
{
    const uint8_t *const ram = core->ram;
    uint8_t rounded_y;
    uint8_t nt_low;
    uint8_t nt_high_bits = 0u;
    uint8_t attr_low_base;
    uint8_t scrolled_x;
    uint8_t nametable_index;
    uint8_t coarse_x;
    uint8_t attr_column_offset;
    unsigned carry;

    {
        const unsigned vertical_sum = (unsigned)(uint8_t)dest_y + (unsigned)ram[CONTRA_RAM_VERTICAL_SCROLL];
        rounded_y = (uint8_t)vertical_sum;
        if ((vertical_sum > 0xFFu) || (rounded_y >= 0xF0u))
        {
            rounded_y = (uint8_t)(rounded_y + 0x10u);
        }
        rounded_y &= 0xF8u;
    }

    nt_low = rounded_y;
    attr_low_base = (uint8_t)((rounded_y >> 2u) & 0x38u);
    carry = (unsigned)(nt_low >> 7u);
    nt_low = (uint8_t)(nt_low << 1u);
    nt_high_bits = (uint8_t)((nt_high_bits << 1u) | (uint8_t)carry);
    carry = (unsigned)(nt_low >> 7u);
    nt_low = (uint8_t)(nt_low << 1u);
    nt_high_bits = (uint8_t)((nt_high_bits << 1u) | (uint8_t)carry);

    {
        const unsigned horizontal_sum = (unsigned)(uint8_t)dest_x + (unsigned)ram[CONTRA_RAM_HORIZONTAL_SCROLL];
        scrolled_x = (uint8_t)horizontal_sum;
        nametable_index = (uint8_t)(ram[CONTRA_RAM_PPUCTRL_SETTINGS] & 0x01u);
        if (horizontal_sum > 0xFFu)
        {
            nametable_index ^= 0x01u;
        }
    }

    coarse_x = (uint8_t)((scrolled_x & 0xF8u) >> 3u);
    attr_column_offset = (uint8_t)((coarse_x >> 2u) | attr_low_base);

    *tile_ppu_addr = (uint16_t)(
        ((uint16_t)((nametable_index == 0u ? 0x20u : 0x24u) | nt_high_bits) << 8u) |
        (uint16_t)(nt_low | coarse_x)
    );
    *attr_ppu_addr = (uint16_t)(
        ((uint16_t)((nametable_index == 0u ? 0x23u : 0x27u) | 0x03u) << 8u) |
        (uint16_t)(0xC0u | attr_column_offset)
    );
}

static void contra_write_overlay_supertile_to_ppu_addr(
    ContraCore *core,
    uint16_t supertile_data_addr,
    uint16_t palette_data_addr,
    uint16_t tile_ppu_addr,
    uint16_t attr_ppu_addr,
    uint8_t supertile_index
)
{
    const bool update_palette = (supertile_index & 0x80u) == 0u;
    const uint8_t stripped_index = (uint8_t)(supertile_index & 0x7Fu);
    const uint16_t supertile_addr = (uint16_t)(
        supertile_data_addr + ((uint16_t)stripped_index * 16u)
    );
    unsigned tile_y;

    if (!contra_load_rom_image())
    {
        return;
    }

    if (update_palette)
    {
        contra_write_ppu_byte(
            core,
            attr_ppu_addr,
            contra_rom_read_u8(3u, (uint16_t)(palette_data_addr + stripped_index))
        );
    }

    for (tile_y = 0u; tile_y < 4u; ++tile_y)
    {
        unsigned tile_x;

        for (tile_x = 0u; tile_x < 4u; ++tile_x)
        {
            const uint8_t pattern_index = contra_rom_read_u8(
                3u,
                (uint16_t)(supertile_addr + ((uint16_t)tile_y * 4u) + tile_x)
            );

            contra_write_ppu_byte(core, (uint16_t)(tile_ppu_addr + tile_x), pattern_index);
        }

        tile_ppu_addr = (uint16_t)(tile_ppu_addr + 0x20u);
    }
}

static void contra_write_overlay_supertile_to_ppu(
    ContraCore *core,
    uint16_t supertile_data_addr,
    uint16_t palette_data_addr,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    uint16_t tile_ppu_addr;
    uint16_t attr_ppu_addr;

    contra_calculate_update_supertile_ppu_addr(core, dest_x, dest_y, &tile_ppu_addr, &attr_ppu_addr);
    contra_write_overlay_supertile_to_ppu_addr(
        core,
        supertile_data_addr,
        palette_data_addr,
        tile_ppu_addr,
        attr_ppu_addr,
        supertile_index
    );
}

static void contra_calculate_level_1_nametable_update_supertile_ppu_addr(
    const ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint16_t *tile_ppu_addr,
    uint16_t *attr_ppu_addr
)
{
    const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    const int aligned_x = (((enemy_x - 12) + scroll_offset) & ~7) - scroll_offset;
    const int aligned_y = (enemy_y - 12) & ~7;

    contra_calculate_update_supertile_ppu_addr(core, aligned_x, aligned_y, tile_ppu_addr, attr_ppu_addr);
}

static void contra_write_level_1_nametable_update_supertile_to_ppu_addr(
    ContraCore *core,
    uint16_t tile_ppu_addr,
    uint16_t attr_ppu_addr,
    uint8_t supertile_index
)
{
    contra_write_overlay_supertile_to_ppu_addr(
        core,
        contra_level_1_nametable_update_supertile_data_addr,
        contra_level_1_nametable_update_palette_data_addr,
        tile_ppu_addr,
        attr_ppu_addr,
        supertile_index
    );
}

static void contra_write_level_1_nametable_update_supertile_to_ppu(
    ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint8_t supertile_index
)
{
    uint16_t tile_ppu_addr;
    uint16_t attr_ppu_addr;

    contra_calculate_level_1_nametable_update_supertile_ppu_addr(
        core,
        enemy_x,
        enemy_y,
        &tile_ppu_addr,
        &attr_ppu_addr
    );
    contra_write_level_1_nametable_update_supertile_to_ppu_addr(
        core,
        tile_ppu_addr,
        attr_ppu_addr,
        supertile_index
    );
}

static void contra_process_level_1_weapon_box_restore(ContraCore *core)
{
    if (core->level1_weapon_box_restore_timer == 0u)
    {
        return;
    }

    core->level1_weapon_box_restore_timer = (uint8_t)(core->level1_weapon_box_restore_timer - 1u);
    if (core->level1_weapon_box_restore_timer != 0u)
    {
        return;
    }

    contra_write_level_1_nametable_update_supertile_to_ppu_addr(
        core,
        (uint16_t)core->level1_weapon_box_restore_x,
        (uint16_t)core->level1_weapon_box_restore_y,
        0x16u
    );
    core->level1_weapon_box_restore_x = 0;
    core->level1_weapon_box_restore_y = 0;
}

static void contra_render_level_1_overlay_supertile(
    ContraCore *core,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    contra_write_overlay_supertile_to_ppu(
        core,
        contra_level_1_nametable_update_supertile_data_addr,
        contra_level_1_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
    contra_render_overlay_supertile(
        core,
        contra_level_1_nametable_update_supertile_data_addr,
        contra_level_1_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
}

static void contra_render_level_2_overlay_supertile(
    ContraCore *core,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    /* update_nametable_supertile (bank7:1356-1365) forces the boss-room overlay table
       when LEVEL_LOCATION_TYPE bit 7 is set, so the wall cannon/plating housings draw
       from level_2_4_nametable_update_supertile_data ($BA1A) not the corridor set. */
    const int boss = (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u;
    const uint16_t supertile_addr = boss
        ? contra_level_2_4_boss_nametable_update_supertile_data_addr
        : contra_level_2_nametable_update_supertile_data_addr;
    const uint16_t palette_addr = boss
        ? contra_level_2_4_boss_nametable_update_palette_data_addr
        : contra_level_2_nametable_update_palette_data_addr;

    contra_write_overlay_supertile_to_ppu(
        core,
        supertile_addr,
        palette_addr,
        dest_x,
        dest_y,
        supertile_index
    );
    contra_render_overlay_supertile(
        core,
        supertile_addr,
        palette_addr,
        dest_x,
        dest_y,
        supertile_index
    );
}

static void contra_render_level_3_overlay_supertile(
    ContraCore *core,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    contra_write_overlay_supertile_to_ppu(
        core,
        contra_level_3_nametable_update_supertile_data_addr,
        contra_level_3_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
    contra_render_overlay_supertile(
        core,
        contra_level_3_nametable_update_supertile_data_addr,
        contra_level_3_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
}

static void contra_render_level_1_nametable_update_supertile(
    ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint8_t supertile_index
)
{
    /*
     * The original engine writes the supertile to the nametable rounded down
     * to an 8-pixel grid in *world* space (set_ppu_addresses_in_mem
     * `AND #$F8` after adding HORIZONTAL_SCROLL/VERTICAL_SCROLL). Match that
     * so the supertile's 8x8 tiles line up with the underlying wall tiles
     * instead of leaking the wall pattern through and producing a doubled
     * appearance, while still scrolling smoothly with the level.
     */
    const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    const int aligned_x = (((enemy_x - 12) + scroll_offset) & ~7) - scroll_offset;
    const int aligned_y = (enemy_y - 12) & ~7;

    /* draw_enemy_supertile_10 (bank7:8552) draws from the CURRENT level's
       nametable update data. Level 3 spawns the same shared enemies that restore
       a background super-tile (pill box 0x02, rotating gun 0x04, red turret 0x07),
       so on level 3 read from the level-3 data, not level-1. */
    if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
    {
        contra_write_overlay_supertile_to_ppu(
            core, contra_level_3_nametable_update_supertile_data_addr,
            contra_level_3_nametable_update_palette_data_addr,
            aligned_x, aligned_y, supertile_index);
        contra_render_overlay_supertile(
            core, contra_level_3_nametable_update_supertile_data_addr,
            contra_level_3_nametable_update_palette_data_addr,
            aligned_x, aligned_y, supertile_index);
        return;
    }

    contra_write_level_1_nametable_update_supertile_to_ppu(core, enemy_x, enemy_y, supertile_index);
    contra_render_level_1_overlay_supertile(core, aligned_x, aligned_y, supertile_index);
}

/* level_2_4_tile_animation (bank 3, CPU $86E1): 11 five-byte entries
   {row_flag, tile0, tile1, tile2, tile3}. For level 2/4 the row flag is always 0,
   meaning a 2x2 pattern-tile block (16x16 px): tile0,tile1 on the top row,
   tile2,tile3 below. (data bank3.asm:185; drawn by update_enemy_nametable_tiles
   bank7.asm:8631 / update_nametable_tiles bank7.asm:1643.) */
#define CONTRA_LEVEL_2_4_TILE_ANIMATION_ADDR 0x86E1u

/* Draw the 2x2 level_2_4_tile_animation block for offset anim_offset (0x80..0x8a)
   at the enemy position, as a framebuffer overlay. Mirrors the placement of
   update_enemy_nametable_tiles (top-left = enemy pos - 4, rounded to the 8px tile
   grid in world space so it lines up with the back wall).

   Palette: the wall turret/core write their attribute quadrant when they draw
   (update_nametable_tiles, bank7.asm:1676) by merging the tile_animation entry's
   first byte (the "row flag") into the super-tile palette for that quadrant. For
   every level_2_4_tile_animation entry that first byte is $00, so the structure's
   quadrant is forced to background palette slot 0 -- which is why the metallic
   turret/core stands out against the blue (slot 3) back wall. The open frames
   inherit this with bit 7 set ("leave existing palette"), so they stay slot 0
   too. We therefore draw the whole block at slot 0 rather than inheriting the
   surrounding wall's palette (which would tint the turret blue and wash out the
   head). */
static void contra_render_level_2_tile_animation(
    ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint8_t anim_offset
)
{
    const uint8_t index = (uint8_t)(anim_offset & 0x7Fu);
    const uint16_t entry = (uint16_t)(CONTRA_LEVEL_2_4_TILE_ANIMATION_ADDR + ((uint16_t)index * 5u));
    const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    const int aligned_x = (((enemy_x - 4) + scroll_offset) & ~7) - scroll_offset;
    const int aligned_y = (enemy_y - 4) & ~7;
    unsigned tile_y;

    if (!contra_load_rom_image())
    {
        return;
    }

    for (tile_y = 0u; tile_y < 2u; ++tile_y)
    {
        unsigned tile_x;

        for (tile_x = 0u; tile_x < 2u; ++tile_x)
        {
            const uint8_t pattern_index = contra_rom_read_u8(
                3u, (uint16_t)(entry + 1u + (tile_y * 2u) + tile_x));
            const int px = aligned_x + (int)(tile_x * 8u);
            const int py = aligned_y + (int)(tile_y * 8u);

            /* row-flag $00 = keep the underlying super-tile palette; the back wall
               the core/turret sits on uses the cycling 4th BG palette (slot 3), so
               the structure flashes red/blue with the wall as in the ROM (not the
               forced gray slot 0 that washed the target out). */
            contra_draw_background_tile(core, px, py, pattern_index, 3u);
        }
    }
}

/* Faithful level-2 wall turret/core (types 0x13/0x14) live on real RAM. Their
   background appears only via this overlay re-draw each render frame, from the
   per-slot tile cache the routines populate at their draw points. */
static void contra_render_level_2_wall_structures(ContraCore *core)
{
    int slot;

    for (slot = 0; slot < 16; ++slot)
    {
        const uint8_t sx = (uint8_t)slot;
        const uint8_t type = core->ram[CONTRA_RAM_ENEMY_TYPE + sx];
        const uint8_t tile = core->l2_structure_tile[sx];
        const uint8_t supertile = core->l2_supertile[sx];

        if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + sx] == 0u)
        {
            continue;
        }
        /* 0xFE = a killed structure mid-explosion: keep its last (destroyed) tile
           on the wall under the explosion sprite until the room reloads. The
           core/turret draw a 2x2 tile-animation block; the boss-room cannon and
           plating draw a 4x4 super-tile. */
        if ((tile != 0u) && ((type == 0x13u) || (type == 0x14u) || (type == 0xFEu)))
        {
            contra_render_level_2_tile_animation(
                core,
                (int)core->ram[CONTRA_RAM_ENEMY_X_POS + sx],
                (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + sx],
                tile);
        }
        if ((supertile != 0xFFu) && ((type == 0x08u) || (type == 0x0Au) || (type == 0xFEu)))
        {
            const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
            const int ex = (int)core->ram[CONTRA_RAM_ENEMY_X_POS + sx];
            const int ey = (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + sx];
            const int aligned_x = (((ex - 12) + scroll_offset) & ~7) - scroll_offset;
            const int aligned_y = (ey - 12) & ~7;

            contra_render_level_2_overlay_supertile(core, aligned_x, aligned_y, supertile);
        }
    }

    /* back-wall blow-open: 4 destroyed quadrant super-tiles at fixed positions,
       drawn one-by-one by wall_core_routine_08 and persisting until the room
       reloads (same positions the invented path uses). Only valid while the room is
       static -- during the walk-into-screen transition (INDOOR_SCROLL != 0) the room
       re-composes at the perspective scroll offsets, so these fixed-position quadrants
       would float as a misaligned block over the receding back wall. */
    if ((core->l2_blowopen_quadrants != 0u) &&
        (core->ram[CONTRA_RAM_INDOOR_SCROLL] == 0u))
    {
        unsigned q;

        for (q = 0u; q < 4u; ++q)
        {
            if ((core->l2_blowopen_quadrants & (uint8_t)(1u << q)) != 0u)
            {
                contra_render_level_2_overlay_supertile(
                    core,
                    (int)contra_level_2_wall_core_update_x_tbl[q],
                    (int)contra_level_2_wall_core_update_y_tbl[q],
                    contra_level_2_wall_core_update_supertile_tbl[q]);
            }
        }
    }

    /* destroyed boss-room cannon/plating housings: redraw the destroyed super-tile
       (index 5) at each recorded position so it persists after the enemy explodes and
       its slot is freed (the ROM's nametable write persists; the port re-composes). */
    {
        unsigned d;

        for (d = 0u; d < core->l2_destroyed_struct_count; ++d)
        {
            const int ex = (int)core->l2_destroyed_struct_x[d];
            const int ey = (int)core->l2_destroyed_struct_y[d];

            contra_render_level_2_overlay_supertile(
                core, (ex - 12) & ~7, (ey - 12) & ~7, 0x05u);
        }
    }
}

static uint8_t contra_read_screen_palette_slot_at(const ContraCore *core, int px, int py)
{
    const unsigned horizontal_sum = (unsigned)(uint8_t)px + (unsigned)core->ram[CONTRA_RAM_HORIZONTAL_SCROLL];
    const unsigned vertical_sum = (unsigned)(uint8_t)py + (unsigned)core->ram[CONTRA_RAM_VERTICAL_SCROLL];
    uint8_t nametable_index = (uint8_t)(core->ram[CONTRA_RAM_PPUCTRL_SETTINGS] & 0x01u);
    const uint8_t coarse_x = (uint8_t)(((uint8_t)horizontal_sum & 0xF8u) >> 3u);
    const uint8_t coarse_y = (uint8_t)(((uint8_t)vertical_sum & 0xF8u) >> 3u);

    if (horizontal_sum > 0xFFu)
    {
        nametable_index ^= 0x01u;
    }

    return contra_read_nametable_palette_slot(core, nametable_index, coarse_x, coarse_y);
}

static void contra_render_level_7_tile_animation(ContraCore *core, int x, int y, uint8_t tile_index)
{
    const uint8_t index = (uint8_t)(tile_index & 0x7Fu);
    /* level 6 (the fire beams) records into the same tile cache; only the
       source data table differs */
    const uint16_t tile_animation_addr =
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u)
            ? contra_level_6_tile_animation_addr
            : contra_level_7_tile_animation_addr;
    const uint16_t entry = (uint16_t)(tile_animation_addr + ((uint16_t)index * 5u));
    uint8_t row_count;
    uint16_t read_addr;
    uint8_t row;

    if (!contra_load_rom_image())
    {
        return;
    }

    row_count = 0x02u;
    if ((contra_rom_read_u8(3u, entry) & 0x80u) != 0u)
    {
        row_count = (uint8_t)(contra_rom_read_u8(3u, entry) & 0x07u);
    }
    read_addr = (uint16_t)(entry + 1u);

    for (row = 0u; row < row_count; ++row)
    {
        uint8_t col;

        for (col = 0u; col < 2u; ++col)
        {
            const int px = x + (int)(col * 8u);
            const int py = y + (int)(row * 8u);
            const uint8_t pattern_index = contra_rom_read_u8(3u, read_addr++);

            contra_draw_background_tile(
                core,
                px,
                py,
                pattern_index,
                contra_read_screen_palette_slot_at(core, px, py));
        }
    }
}

static void contra_render_level_7_nametable_writes(ContraCore *core)
{
    /* levels 5 and 8 record into the same position-keyed overlay cache;
       only the source data tables differ */
    const uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];
    const uint16_t supertile_addr = (level == 0x04u)
        ? contra_level_5_nametable_update_supertile_data_addr
        : ((level == 0x07u)
               ? contra_level_8_nametable_update_supertile_data_addr
               : contra_level_7_nametable_update_supertile_data_addr);
    const uint16_t palette_addr = (level == 0x04u)
        ? contra_level_5_nametable_update_palette_data_addr
        : ((level == 0x07u)
               ? contra_level_8_nametable_update_palette_data_addr
               : contra_level_7_nametable_update_palette_data_addr);
    const int scroll_x = contra_level7_world_scroll_x(core);
    uint8_t i;
    uint8_t kept;

    /* the cache stores WORLD-anchored positions; convert to screen space at the
       current scroll, and evict entries that have scrolled fully off the left
       edge (the scroll never reverses, so they can never come back) */
    kept = 0u;
    for (i = 0u; i < core->l7_supertile_update_count; ++i)
    {
        const int x = (int)core->l7_supertile_update_x[i] - scroll_x;
        const int y = (int)core->l7_supertile_update_y[i];

        if (x <= -32)
        {
            continue; /* evict: a 32px super-tile fully off-screen left */
        }
        core->l7_supertile_update_x[kept] = core->l7_supertile_update_x[i];
        core->l7_supertile_update_y[kept] = core->l7_supertile_update_y[i];
        core->l7_supertile_update_index[kept] = core->l7_supertile_update_index[i];
        ++kept;

        if (x >= 0)
        {
            /* the PPU mirror write needs an on-screen anchor; the uint8 cast
               below would wrap a partially-scrolled-off entry to the far right */
            contra_write_overlay_supertile_to_ppu(
                core,
                supertile_addr,
                palette_addr,
                x,
                y,
                core->l7_supertile_update_index[i]);
        }
        contra_render_overlay_supertile(
            core,
            supertile_addr,
            palette_addr,
            x,
            y,
            core->l7_supertile_update_index[i]);
    }
    core->l7_supertile_update_count = kept;

    kept = 0u;
    for (i = 0u; i < core->l7_tile_update_count; ++i)
    {
        const int x = (int)core->l7_tile_update_x[i] - scroll_x;

        if (x <= -16)
        {
            continue; /* evict: a 16px-wide tile pair fully off-screen left */
        }
        core->l7_tile_update_x[kept] = core->l7_tile_update_x[i];
        core->l7_tile_update_y[kept] = core->l7_tile_update_y[i];
        core->l7_tile_update_index[kept] = core->l7_tile_update_index[i];
        ++kept;

        contra_render_level_7_tile_animation(
            core,
            x,
            (int)core->l7_tile_update_y[i],
            core->l7_tile_update_index[i]);
    }
    core->l7_tile_update_count = kept;
}

static void contra_render_native_enemies(ContraCore *core)
{
    /* Level 3 (vertical outdoor) also draws background-overlay super-tiles -- the
       dragon boss mouth -- so let its gameplay through to the level-1/3 branch. */
    const bool level3_active =
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u);

    if (!contra_is_native_combat_active(core) && !level3_active)
    {
        return;
    }
    if (contra_is_native_level_7_active(core))
    {
        contra_render_level_7_nametable_writes(core);
        return;
    }
    if ((core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u) ||
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u) ||
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u))
    {
        /* level-5 tank/boss UFO, level-6 fire beams, and level-8 guardian/
           spawn/heart background overlays; sprite enemies render normally */
        contra_render_level_7_nametable_writes(core);
    }
    if (contra_is_native_level_2_active(core))
    {
        /* faithful indoor wall turret/core backgrounds; L1 + soldiers/bullets
           render via their routines and the OAM build */
        contra_render_level_2_wall_structures(core);
    }
    else
    {
        /* level-3 dragon boss mouth: two stacked background super-tiles drawn from
           the mouth's current animation frame (closed/partial/open), redrawn over
           the recomposed background like the level-1 super-tile enemies. */
        if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
        {
            int slot;

            for (slot = 0; slot < CONTRA_ROM_ENEMY_SLOTS; ++slot)
            {
                const uint8_t sx = (uint8_t)slot;
                uint8_t frame;
                int ex;
                int ey;

                if ((core->ram[CONTRA_RAM_ENEMY_TYPE + sx] != 0x14u) ||
                    (core->ram[CONTRA_RAM_ENEMY_ROUTINE + sx] < 0x03u))
                {
                    continue; /* not the mouth, or still hidden in the wall (pre routine_02) */
                }
                frame = core->ram[CONTRA_RAM_ENEMY_FRAME + sx];
                if (frame > 2u)
                {
                    frame = 2u;
                }
                ex = (int)core->ram[CONTRA_RAM_ENEMY_X_POS + sx];
                ey = (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + sx];
                contra_render_level_3_overlay_supertile(
                    core, ex - 12, ey - 12,
                    (uint8_t)(contra_boss_mouth_nametable_update_tbl[frame * 2u] & 0x7Fu));
                contra_render_level_3_overlay_supertile(
                    core, ex - 12, ey + 20,
                    (uint8_t)(contra_boss_mouth_nametable_update_tbl[(frame * 2u) + 1u] & 0x7Fu));
            }
        }
    }
}

static void contra_sync_native_sprite_objects_to_cpu_buffer(ContraCore *core)
{
    /* The faithful enemy system maintains the enemy sprite-object slots
       (ENEMY_SPRITES / ENEMY_Y_POS / ENEMY_X_POS = sprite slots 10..25)
       directly in RAM, exactly as the ROM does — they are persistent enemy
       state, not a per-frame rebuild. Nothing to do here. */
    (void)core;
}

static void contra_latch_cpu_sprite_state(ContraCore *core)
{
    size_t sprite_index;

    contra_sync_native_sprite_objects_to_cpu_buffer(core);

    for (sprite_index = 0u; sprite_index < CONTRA_CPU_SPRITE_RENDER_SLOTS; ++sprite_index)
    {
        core->latched_cpu_sprite_buffer[sprite_index] =
            core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + sprite_index];
        core->latched_sprite_y[sprite_index] =
            core->ram[CONTRA_RAM_SPRITE_Y_POS + sprite_index];
        core->latched_sprite_x[sprite_index] =
            core->ram[CONTRA_RAM_SPRITE_X_POS + sprite_index];
        core->latched_sprite_attr[sprite_index] =
            core->ram[CONTRA_RAM_SPRITE_ATTR + sprite_index];
    }

    core->latched_sprite_load_type = core->ram[CONTRA_RAM_SPRITE_LOAD_TYPE];
    core->latched_demo_mode = core->ram[CONTRA_RAM_DEMO_MODE];
    core->latched_player_mode = core->ram[CONTRA_RAM_PLAYER_MODE];
    core->latched_num_lives[0] = core->ram[CONTRA_RAM_P1_NUM_LIVES];
    core->latched_num_lives[1] = core->ram[CONTRA_RAM_P2_NUM_LIVES];
    core->latched_game_over_status[0] = core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS];
    core->latched_game_over_status[1] = core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS];
    contra_build_oam_for_next_frame(core);
}

static void contra_latch_ppu_render_state(ContraCore *core)
{
    core->latched_horizontal_scroll = core->ram[CONTRA_RAM_HORIZONTAL_SCROLL];
    core->latched_vertical_scroll = core->ram[CONTRA_RAM_VERTICAL_SCROLL];
    core->latched_level_screen_number = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    core->latched_level_screen_scroll_offset = core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    core->latched_ppuctrl_settings = core->ram[CONTRA_RAM_PPUCTRL_SETTINGS];
}

static void contra_apply_pending_palette_write(ContraCore *core)
{
    if (core->pending_palette_write == 0u)
    {
        return;
    }

    memcpy(core->ppu_palette, core->pending_palette, sizeof(core->ppu_palette));
    core->pending_palette_write = 0x00u;
    core->pending_palette_count = 0x00u;
}

static void contra_render_frame(ContraCore *core, bool update_latches)
{
    const uint8_t *const ram = core->ram;
    const uint8_t game_routine = ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    const uint8_t level_routine = ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX];
    const bool demo_level_scene = (game_routine == 0x02u) && (ram[CONTRA_RAM_DEMO_MODE] != 0u);
    const bool gameplay_scene = (game_routine == 0x05u) || demo_level_scene;
    const bool game_over_screen_scene =
        gameplay_scene &&
        (level_routine >= 0x05u) &&
        (level_routine <= 0x07u) &&
        (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0u);
    const uint32_t clear_color = contra_background_palette_color_rgba(core, 0u, 0u);
    size_t pixel_index;

    /*
     * During level_routine_03 the original game is still building the opening
     * gameplay nametable off-screen. Rendering the current PPU state here
     * exposes transient junk or blank frames, so keep the last stage-card
     * frame visible until level_routine_04 begins actual gameplay.
     */
    if (gameplay_scene && (level_routine == 0x03u))
    {
        if (update_latches)
        {
            contra_latch_cpu_sprite_state(core);
            contra_latch_ppu_render_state(core);
            contra_apply_pending_palette_write(core);
        }
        return;
    }

    for (pixel_index = 0; pixel_index < (size_t)(CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT); ++pixel_index)
    {
        core->framebuffer[pixel_index] = clear_color;
        core->background_opaque[pixel_index] = 0u;
        core->sprite_priority[pixel_index] = 0xFFu;
    }

    if (((game_routine <= 0x03u) && !demo_level_scene) || (game_routine == 0x06u))
    {
        /* game_routine_06 (the ending) is a pure PPU-model scene: the melt
           erases pattern tiles under the last nametable, the island scene and
           credits write the nametable directly, and the helicopter/explosions
           live in the enemy sprite slots. */
        contra_render_intro_background(core);
        contra_render_cpu_sprites(core);
        if (update_latches)
        {
            contra_latch_cpu_sprite_state(core);
            contra_latch_ppu_render_state(core);
            contra_apply_pending_palette_write(core);
        }
        return;
    }

    /*
     * The level intro / score screen still uses the PPU nametable text path.
     * Keep that view through level_routine_03 because the level background
     * initialization is still in flight there; switching early exposes
     * transient floor-only frames before gameplay begins.
     */
    if (gameplay_scene && ((level_routine < 0x04u) || game_over_screen_scene))
    {
        contra_render_intro_background(core);
    }
    else if (gameplay_scene && (level_routine >= 0x04u))
    {
        contra_render_level_background(core);
    }

    if (!game_over_screen_scene)
    {
        contra_render_native_enemies(core);
    }
    contra_render_cpu_sprites(core);
    if (update_latches)
    {
        contra_latch_cpu_sprite_state(core);
        contra_latch_ppu_render_state(core);
        contra_apply_pending_palette_write(core);
    }
}
