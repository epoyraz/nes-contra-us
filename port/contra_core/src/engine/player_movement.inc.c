/* Player collision, sprites, movement states, scrolling, death, and respawn.
   Included by core.c; not compiled as a separate translation unit. */


static uint8_t contra_classify_pattern_collision(const ContraCore *core, uint8_t pattern_index)
{
    if (pattern_index == 0u)
    {
        return 0u;
    }

    if (pattern_index < core->ram[CONTRA_RAM_COLLISION_CODE_1_TILE_INDEX])
    {
        return 1u;
    }

    if (pattern_index < core->ram[CONTRA_RAM_COLLISION_CODE_0_TILE_INDEX])
    {
        return 0u;
    }

    if (pattern_index < core->ram[CONTRA_RAM_COLLISION_CODE_2_TILE_INDEX])
    {
        return 2u;
    }

    return 3u;
}

static bool contra_read_level_collision_pattern_index(
    const ContraCore *core,
    uint16_t world_tile_x,
    uint16_t world_tile_y,
    uint8_t *pattern_index
)
{
    const uint16_t supertile_ptr = (uint16_t)(
        (uint16_t)core->ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
        ((uint16_t)core->ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u)
    );
    uint8_t screen_supertiles[CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE];
    uint8_t screen_number;
    uint8_t tile_x_in_screen;
    uint8_t tile_y_in_screen;
    uint8_t supertile_column;
    uint8_t supertile_row;
    uint8_t tile_x_in_supertile;
    uint8_t tile_y_in_supertile;
    size_t supertile_offset;
    size_t tile_offset;
    uint8_t supertile_index;

    if (!contra_load_rom_image())
    {
        return false;
    }

    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        if (world_tile_x >= 32u)
        {
            return false;
        }

        screen_number = (uint8_t)(world_tile_y / 30u);
        tile_x_in_screen = (uint8_t)world_tile_x;
        tile_y_in_screen = (uint8_t)(world_tile_y % 30u);
    }
    else
    {
        screen_number = (uint8_t)(world_tile_x >> 5u);
        tile_x_in_screen = (uint8_t)(world_tile_x & 0x1Fu);
        tile_y_in_screen = (uint8_t)world_tile_y;
        if (tile_y_in_screen >= 28u)
        {
            return false;
        }
    }

    supertile_column = (uint8_t)(tile_x_in_screen >> 2u);
    supertile_row = (uint8_t)(tile_y_in_screen >> 2u);
    tile_x_in_supertile = (uint8_t)((tile_x_in_screen & 0x03u) & 0x02u);
    tile_y_in_supertile = (uint8_t)((tile_y_in_screen & 0x03u) & 0x02u);
    supertile_offset = ((size_t)supertile_row * 8u) + (size_t)supertile_column;
    tile_offset = ((size_t)tile_y_in_supertile * 4u) + (size_t)tile_x_in_supertile;

    if ((supertile_column >= 8u) || (supertile_row >= 8u))
    {
        return false;
    }

    memset(screen_supertiles, 0, sizeof(screen_supertiles));
    contra_decode_level_screen_supertiles((ContraCore *)core, screen_number, screen_supertiles, 0u);
    supertile_index = screen_supertiles[supertile_offset];
    *pattern_index = contra_rom_read_u8(3u, (uint16_t)(supertile_ptr + ((uint16_t)supertile_index * 16u) + (uint16_t)tile_offset));
    return true;
}

/* Faithful exploding-bridge collision gap: true when (screen_x, screen_y) falls
   in a background super-tile the real-RAM bridge has cleared. The gap is anchored
   in world space (screen<<8 + scroll + x is scroll-invariant), so it persists as
   the level scrolls and after the bridge enemy is removed. When the faithful
   system is off this list stays empty, so the check is a no-op. */
/* PORT HARNESS (supertile_collision_override): no single ASM routine -- look up
   the port-side runtime collision rewrites (bridge gaps via clear_supertile_bg_
   collision, boss-door tunnel cells and L7 spiked walls via set_supertile_bg_
   collisions) at a world position. Returns true with the ROM collision code for
   the queried point: the stored byte holds one nibble per 16px half of the cell
   (low = left, high = right); within a nibble bits 0-1 = top 16px row,
   bits 2-3 = bottom row. */
static bool contra_rom_supertile_collision_override(
    const ContraCore *core, uint8_t screen_x, uint8_t screen_y, uint8_t *out_code)
{
    static const uint8_t collision_code_lookup_tbl[4] = {0x00u, 0x01u, 0x02u, 0x80u};
    const uint16_t world_x =
        (uint16_t)(((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                   core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + screen_x);
    uint8_t i;

    for (i = 0u; i < core->l1_bridge_gap_count; ++i)
    {
        /* clear_supertile_bg_collision (bank7:8143) clears the GRID-ALIGNED
           super-tile cell containing the draw point; the stored coordinates
           are the visual draw anchor (a few px inside the cell), so snap to
           the 32px grid here -- the world grid at multiples of 32, the screen
           rows at 0x10 + 32k. Unsnapped, the gap leaked 4px into the intact
           neighbor cell and a soldier stepped off a bridge the ROM still
           considers floor. */
        const uint16_t gx = (uint16_t)(core->l1_bridge_gap_world_x[i] & ~(uint16_t)31u);
        const int gy = ((((int)core->l1_bridge_gap_screen_y[i] - 0x10) & ~31) + 0x10);

        if ((world_x >= gx) && (world_x < (uint16_t)(gx + 32u)) &&
            ((int)screen_y >= gy) && ((int)screen_y < (gy + 32)))
        {
            const uint8_t packed = core->l1_bridge_gap_coll[i];
            const uint8_t nibble = ((uint16_t)(world_x - gx) >= 16u)
                ? (uint8_t)(packed >> 4u)
                : (uint8_t)(packed & 0x0Fu);
            const uint8_t bits = (((int)screen_y - gy) >= 16)
                ? (uint8_t)(nibble >> 2u)
                : nibble;

            *out_code = collision_code_lookup_tbl[bits & 0x03u];
            return true;
        }
    }
    return false;
}

static uint8_t contra_get_outdoor_horizontal_bg_collision(
    const ContraCore *core,
    uint8_t screen_x,
    uint8_t screen_y
)
{
    const uint16_t world_pixel_x =
        ((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
        (uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] +
        (uint16_t)screen_x;
    uint8_t pattern_index;
    uint8_t collision_code;

    if (screen_y < 0x10u)
    {
        return 0u;
    }

    {
        uint8_t override_code;

        if (contra_rom_supertile_collision_override(core, screen_x, screen_y, &override_code))
        {
            return override_code;
        }
    }

    /* BG_COLLISION_DATA granularity (set_tile_collision, bank7:6222): the ROM
       classifies ONE pattern tile per 16x16 quadrant -- the quadrant's top-left
       8x8 tile (odd nametable columns and rows are skipped during the build).
       Snap the probe to that tile or probes in a quadrant's right/bottom 8px
       read a tile the ROM never consults. */
    if (!contra_read_level_collision_pattern_index(
            core,
            (uint16_t)((world_pixel_x >> 3u) & ~(uint16_t)1u),
            (uint8_t)(((screen_y - 0x10u) >> 3u) & 0xFEu),
            &pattern_index))
    {
        return 0u;
    }

    collision_code = contra_classify_pattern_collision(core, pattern_index);
    return (collision_code == 3u) ? 0x80u : collision_code;
}

static uint8_t contra_get_outdoor_bg_collision(
    const ContraCore *core,
    uint8_t screen_x,
    uint8_t screen_y
)
{
    uint8_t pattern_index;
    uint8_t collision_code;

    /* read_bg_collision_byte (bank7:6404): the raw screen Y (pre-scroll, $15)
       past 0xE0 is "past last bg collision row" -> empty. On the waterfall
       this is what lets falling rocks drop off the bottom instead of bouncing
       on the last supertile row. */
    if (screen_y >= 0xE0u)
    {
        return 0u;
    }

    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u)
    {
        return contra_get_outdoor_horizontal_bg_collision(core, screen_x, screen_y);
    }

    {
        /* bg_collision_logic (bank7:6439): $11 = Y + VERTICAL_SCROLL, +0x10 when
           the 8-bit add carries OR lands in 0xF0..0xFF (skipping the attribute
           rows). BG_COLLISION_DATA is a 15-row ring updated as the level streams:
           a row belongs to the NEXT screen once it has fully scrolled in at the
           window top (row*16 >= VERTICAL_SCROLL), and still holds the CURRENT
           screen's stale data otherwise -- including rows that have scrolled off
           the bottom. The waterfall soldier generator depends on the stale rows:
           probes at y=0..6 wrap to ring row 12 (only partially revealed for the
           next screen) and find the current screen's lone left-column floor
           (frame 20968), while freshly streamed top-of-screen soldiers stand on
           fully-revealed next-screen rows (frame 16121). */
        const uint8_t screen_number = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
        const uint8_t vs = core->ram[CONTRA_RAM_VERTICAL_SCROLL];
        /* the next screen's rows stream in at the window top as the level
           scrolls; rows at/below 240-scroll_off have been written. At off=0
           (unscrolled screen) the boundary is 240: no ring row belongs to the
           next screen yet. */
        const uint16_t streamed_boundary =
            (uint16_t)(240u - core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET]);
        const uint16_t sum = (uint16_t)screen_y + vs;
        uint8_t row_px = (uint8_t)sum;

        if ((sum > 0xFFu) || (row_px >= 0xF0u))
        {
            row_px = (uint8_t)(row_px + 0x10u);
        }
        /* quadrant snap: BG_COLLISION_DATA samples only the top-left tile of
           each 16x16 quadrant (set_tile_collision, bank7:6222) */
        {
            const uint8_t data_screen = ((uint16_t)(row_px & 0xF0u) >= streamed_boundary)
                ? (uint8_t)(screen_number + 1u)
                : screen_number;
            const uint16_t world_tile_y =
                (uint16_t)((uint16_t)data_screen * 30u +
                           (((uint16_t)row_px >> 3u) & ~(uint16_t)1u));

            if (!contra_read_level_collision_pattern_index(
                    core, (uint16_t)((screen_x >> 3u) & 0x1Eu), world_tile_y, &pattern_index))
            {
                return 0u;
            }
        }
    }

    collision_code = contra_classify_pattern_collision(core, pattern_index);
    return (collision_code == 3u) ? 0x80u : collision_code;
}

static uint8_t contra_get_player_bg_collision_code(const ContraCore *core, uint8_t player_index)
{
    const uint8_t location_type = core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];
    const uint8_t sprite_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    const uint8_t sprite_x = core->ram[CONTRA_RAM_SPRITE_X_POS + player_index];

    if ((location_type & 0x80u) != 0u)
    {
        return (sprite_y > 0xC8u) ? 0x01u : 0x00u;
    }

    if (location_type != 0u)
    {
        return (sprite_y > 0xA0u) ? 0x01u : 0x00u;
    }

    return contra_get_outdoor_bg_collision(core, sprite_x, (uint8_t)(sprite_y + 0x10u));
}

static bool contra_can_player_drop_down(const ContraCore *core, uint8_t player_index)
{
    const uint8_t *const ram = core->ram;
    const uint8_t sample_x = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
    const uint8_t sample_y = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + 0x10u);
    uint16_t row_sample_y;

    if ((ram[CONTRA_RAM_LEVEL_STOP_SCROLL] != 0xFFu) &&
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u))
    {
        return true;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        return false;
    }

    if (contra_get_outdoor_bg_collision(core, sample_x, sample_y) == 0x80u)
    {
        return false;
    }

    row_sample_y = (uint16_t)(sample_y & 0xF0u) + 0x10u;
    while (row_sample_y < 0xE0u)
    {
        if (contra_get_outdoor_bg_collision(core, sample_x, (uint8_t)row_sample_y) != 0u)
        {
            return true;
        }

        row_sample_y += 0x10u;
    }

    return false;
}

static bool contra_player_has_solid_collision_ahead(const ContraCore *core, uint8_t player_index, int delta_x)
{
    const uint8_t location_type = core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];
    const uint8_t scrolling_type = core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE];
    uint8_t sample_x = (uint8_t)(core->ram[CONTRA_RAM_SPRITE_X_POS + player_index] + delta_x);
    uint8_t sample_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    unsigned sample_index;

    if ((location_type != 0u) || (scrolling_type != 0u))
    {
        return false;
    }

    sample_y = (sample_y > 0x0Bu) ? (uint8_t)(sample_y - 0x0Bu) : 0x0Au;
    for (sample_index = 0u; sample_index < 3u; ++sample_index)
    {
        const uint8_t collision = contra_get_outdoor_horizontal_bg_collision(
            core,
            sample_x,
            (uint8_t)(sample_y + (uint8_t)(sample_index * 0x10u))
        );

        if (collision == 0x80u)
        {
            return true;
        }
    }

    return false;
}

static void contra_init_player_attributes(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = 0x00u;
    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = 0x00u;
    ram[CONTRA_RAM_SPRITE_X_POS + player_index] = 0x00u;
    ram[CONTRA_RAM_SPRITE_ATTR + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + player_index] = 0x00u;
    ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_Y + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME + player_index] = 0x00u;
    ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_DEATH_FLAG + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ON_ENEMY + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_X + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_AIM_PREV_FRAME + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_FAST_X_VEL_BOOST + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] = 0x00u;
}

static void contra_init_player_data(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] = 0x00u;
    ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] = 0x00u;
}

static uint8_t contra_get_level_screen_type(const ContraCore *core)
{
    const uint8_t location_type = core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];

    if (location_type == 0u)
    {
        return 0u;
    }

    if ((location_type & 0x80u) != 0u)
    {
        return 1u;
    }

    return 2u;
}

static void contra_set_player_horizontal_flip(ContraCore *core, uint8_t player_index)
{
    uint8_t flip = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x3Fu);

    if (core->ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u)
    {
        flip |= 0x40u;
    }

    core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
}

static void contra_set_player_jump_sprite(ContraCore *core, uint8_t player_index)
{
    uint8_t base_flip = 0x00u;
    uint8_t flip = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x3Fu);
    uint8_t anim_index = core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] & 0x03u;

    if ((core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] & 0x80u) != 0u)
    {
        base_flip = 0x40u;
    }

    if (anim_index >= 0x02u)
    {
        flip |= 0xC0u;
    }

    flip ^= base_flip;
    core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
    core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_curled_sprite_code_tbl[anim_index];

    core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] =
        (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] + 1u);
    if (core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] >= 0x05u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] = 0x00u;
        core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
            (uint8_t)((core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u) & 0x03u);
    }
}

static void contra_set_player_death_sprite(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] == 0x06u)
    {
        if (ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] < 0x1Bu)
        {
            ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] + 1u);
            ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x55u;
        }
        else
        {
            ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x56u;
        }

        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = 0x00u;
        return;
    }

    ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] + 1u);
    if ((ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] & 0x07u) == 0u)
    {
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u);
        if (ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] >= 0x05u)
        {
            ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x04u;
        }
    }

    {
        uint8_t frame_index = ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index];
        uint8_t flip = contra_player_death_sprite_tbl[frame_index][1];

        if ((ram[CONTRA_RAM_PLAYER_DEATH_FLAG + player_index] & 0x02u) != 0u)
        {
            flip ^= 0x40u;
        }

        ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_death_sprite_tbl[frame_index][0];
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
    }
}

static void contra_set_player_water_transition_flip(ContraCore *core, uint8_t player_index)
{
    uint8_t flip = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x3Fu);

    if ((core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] & 0x02u) != 0u)
    {
        flip |= 0x40u;
    }

    core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
}

static void contra_set_player_water_sprite(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t water_state = ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index];
    const uint8_t aim_dir = (uint8_t)(ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] % 10u);

    if ((water_state & 0x04u) == 0u)
    {
        if ((water_state & 0x10u) == 0u)
        {
            ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
            ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x05u;
            ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + 0x10u);
            if (aim_dir >= 0x05u)
            {
                water_state |= 0x02u;
            }
            ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] = 0x10u;
            water_state |= 0x90u;
            ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = water_state;
        }

        ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
        if (ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] != 0u)
        {
            if (ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] < 0x0Cu)
            {
                ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x73u;
            }
            contra_set_player_water_transition_flip(core, player_index);
            ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] - 1u);
            return;
        }
    }

    water_state = ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index];
    if ((water_state & 0x08u) != 0u)
    {
        ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x06u;
        if (ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] == 0u)
        {
            ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x00u;
            ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
            ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - 0x10u);
            return;
        }

        if (ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] < 0x05u)
        {
            ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x05u;
        }

        contra_set_player_water_transition_flip(core, player_index);
        ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] - 1u);
        return;
    }

    water_state = (uint8_t)((water_state | 0x04u) & 0x7Fu);
    ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = water_state;

    if (ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u)
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_water_firing_sprite_tbl[aim_dir][0];
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] =
            (uint8_t)((ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x0Fu) |
                      contra_player_water_firing_sprite_tbl[aim_dir][1]);
    }
    else
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_water_sprite_tbl[aim_dir][0];
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] =
            (uint8_t)((ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x0Fu) |
                      contra_player_water_sprite_tbl[aim_dir][1]);
    }

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x0Fu) == 0u)
    {
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u);
    }

    if ((ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] & 0x01u) == 0u)
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] |= 0x08u;
    }
    else
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] &= (uint8_t)~0x08u;
    }

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (contra_get_outdoor_bg_collision(
             core,
             ram[CONTRA_RAM_SPRITE_X_POS + player_index],
             ram[CONTRA_RAM_SPRITE_Y_POS + player_index]) != 0x02u))
    {
        water_state = (uint8_t)(ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] | 0x88u);
        if (aim_dir >= 0x05u)
        {
            water_state |= 0x02u;
        }
        else
        {
            water_state &= (uint8_t)~0x02u;
        }

        ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = water_state;
        ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] = 0x0Cu;
        ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x1Au;
        contra_set_player_water_transition_flip(core, player_index);
    }
}

static void contra_set_player_sprite(ContraCore *core, uint8_t player_index)
{
    uint8_t sequence = core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index];

    if (core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] != 0u)
    {
        contra_set_player_water_sprite(core, player_index);
        return;
    }

    if (core->ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x05u;
        contra_set_player_horizontal_flip(core, player_index);
        return;
    }

    if (core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
    {
        contra_set_player_jump_sprite(core, player_index);
        return;
    }

    if ((sequence == 0x04u) || (sequence == 0x06u))
    {
        contra_set_player_death_sprite(core, player_index);
        return;
    }

    if (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        if (sequence == 0x00u)
        {
            /* facing up / standing (player_sprite_indoor_facing_up, bank2:1321) */
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x50u;
        }
        else if (sequence == 0x01u)
        {
            /* electrocuted by the fence (player_sprite_indoor_electrocuted,
               bank2:1325) -- was wrongly drawn as facing-up (0x50), so the shock
               pose never showed when you pressed Up into the live fence. */
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x55u;
        }
        else if (sequence == 0x02u)
        {
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x54u;
        }
        else if (sequence == 0x05u)
        {
            core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] =
                (uint8_t)(core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] - 1u);
            if ((core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] & 0x80u) != 0u)
            {
                contra_play_sound(core, 0x03u);
                core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x0Au;
                core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
                    (uint8_t)(core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u);
            }
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] =
                (uint8_t)(0x57u + (core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] & 0x01u));
        }
        else if (sequence == 0x06u)
        {
            contra_set_player_death_sprite(core, player_index);
            return;
        }
        else if ((sequence == 0x03u) || (sequence == 0x04u))
        {
            /* indoor walking animation (player_sprite_indoor_walking_animation,
               bank2:1305 -> set_player_frame_sprite_from_a): cycle the 6-frame walk
               table (player_frame_sprite_tbl_00 normally, _04 while firing),
               advancing one frame every 8 game frames. This is the sideways-walk
               leg animation the indoor branch previously left static. */
            const uint8_t *const frame_table =
                (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u)
                    ? contra_player_frame_sprite_tbl_04
                    : contra_player_frame_sprite_tbl_00;

            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] =
                frame_table[core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] % 6u];
            core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] =
                (uint8_t)(core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] + 1u);
            if ((core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] & 0x07u) == 0u)
            {
                core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
                    (uint8_t)((core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u) % 6u);
            }
        }
        else
        {
            /* default to facing-up; sequences 4/6 are handled as death above this
               indoor block, so this effectively only catches the standing pose. */
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x50u;
        }
        contra_set_player_horizontal_flip(core, player_index);
        return;
    }

    if (sequence < 0x03u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_small_seq_sprite_tbl[sequence];
        contra_set_player_horizontal_flip(core, player_index);
        return;
    }

    if (sequence == 0x03u)
    {
        uint8_t frame_type = contra_player_frame_sprite_type_tbl[core->ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] % 10u];
        const uint8_t *frame_table = contra_player_frame_sprite_tbl_00;

        if ((frame_type == 0x00u) && (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u))
        {
            frame_type = 0x01u;
        }

        if (frame_type == 0x01u)
        {
            frame_table = contra_player_frame_sprite_tbl_01;
        }
        else if (frame_type == 0x02u)
        {
            frame_table = contra_player_frame_sprite_tbl_02;
        }
        else if (frame_type == 0x03u)
        {
            frame_table = contra_player_frame_sprite_tbl_03;
        }

        core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] =
            frame_table[core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] % 6u];
        core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] =
            (uint8_t)(core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] + 1u);
        if ((core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] & 0x07u) == 0u)
        {
            core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
                (uint8_t)((core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u) % 6u);
        }

        contra_set_player_horizontal_flip(core, player_index);
        return;
    }

    core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x0Au;
}

static void contra_set_player_sprite_and_attrs(ContraCore *core, uint8_t player_index)
{
    uint8_t sprite_code;
    uint8_t attr;
    uint8_t effect_palette = contra_sprite_attr_start_tbl[player_index];

    contra_set_player_sprite(core, player_index);

    sprite_code = core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index];
    if (core->ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] != 0u)
    {
        sprite_code = 0x00u;
    }
    else if ((core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] != 0u) &&
             ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u))
    {
        sprite_code = 0x00u;
    }

    core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = sprite_code;

    if (core->ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] != 0u)
    {
        effect_palette = 0x02u;
    }
    else if ((core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] != 0u) &&
             (((uint8_t)(core->ram[CONTRA_RAM_FRAME_COUNTER] ^ contra_player_effect_xor_tbl[player_index]) & 0x04u) != 0u))
    {
        effect_palette = 0x05u;
    }

    attr = effect_palette;
    if ((core->ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] & 0x80u) != 0u)
    {
        attr |= 0x20u;
    }

    if (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] >= 0x0Cu)
    {
        attr |= 0x08u;
    }

    attr |= (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0xC8u);
    core->ram[CONTRA_RAM_SPRITE_ATTR + player_index] = attr;
}

/* apply_gravity (bank7:5115-5123): increment the player's Y fractional velocity by
   #$23, carrying into the fast (whole-pixel) velocity. */
static void contra_apply_gravity(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] + 0x23u);
    if (ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] < 0x23u)
    {
        ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] + 1u);
    }
}

/* player_jumping_set_y_pos (bank7:5093-5112): advance the jump coefficient by the
   fractional velocity and move the player's Y position by the fast velocity (plus
   the coefficient carry), updating the off-screen HIDDEN counter. */
static void contra_player_jumping_set_y_pos(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t previous_y = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    const uint8_t visibility_delta =
        ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) != 0u) ? 0xFFu : 0x00u;
    uint8_t jump_carry = 0u;
    uint16_t y_sum;

    ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] +
                  ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index]);
    if (ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] < ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index])
    {
        jump_carry = 0x01u;
    }

    y_sum = (uint16_t)previous_y +
        (uint16_t)ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] +
        (uint16_t)jump_carry;
    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = (uint8_t)y_sum;
    ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] + visibility_delta + (uint8_t)(y_sum >> 8u));
}

/* The common @set_y_pos path (bank7:4730-4740): gravity then move the player by
   velocity. Used by the edge-fall and indoor paths that never vertical-scroll. */
static void contra_apply_gravity_set_player_y(ContraCore *core, uint8_t player_index)
{
    contra_apply_gravity(core, player_index);
    contra_player_jumping_set_y_pos(core, player_index);
}

/* set_boss_auto_scroll (bank7:6528-6548): once the player reaches the boss screen
   (LEVEL_STOP_SCROLL == LEVEL_SCREEN_NUMBER) and has scrolled past the trigger
   offset, start the auto-scroll that reveals the boss (AUTO_SCROLL_TIMER_00) and
   latch LEVEL_STOP_SCROLL to #$ff. Returns true when the boss auto-scroll is active
   (the caller then skips the player-driven scroll). */
static bool contra_rom_set_boss_auto_scroll(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    static const uint8_t scroll_trigger_tbl[2] = {0xA0u, 0xC0u};   /* bank7:6553 */
    static const uint8_t auto_scroll_timer_tbl[2] = {0x60u, 0x40u}; /* bank7:6559 */
    const uint8_t st = (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ? 1u : 0u;

    if (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0u)
    {
        return false; /* boss defeated: resume normal scroll */
    }
    if ((ram[CONTRA_RAM_LEVEL_STOP_SCROLL] & 0x80u) != 0u)
    {
        return true; /* auto-scroll already started */
    }
    if (ram[CONTRA_RAM_LEVEL_STOP_SCROLL] != ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER])
    {
        return false; /* not yet on the boss screen */
    }
    if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] < scroll_trigger_tbl[st])
    {
        return false; /* not yet far enough into the boss screen */
    }
    ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] = auto_scroll_timer_tbl[st];
    ram[CONTRA_RAM_LEVEL_STOP_SCROLL] = 0xFFu;
    return true;
}

/* set_vertical_level_frame_scroll (bank7:5213-5237): on a vertical level, when the
   player is jumping up (negative fast velocity) and high on the screen (y < #$50),
   scroll the screen up by the player's would-be upward movement instead of moving
   the player sprite. Mirrors player_jumping_set_y_pos' coefficient update but
   stores the negated Y delta to FRAME_SCROLL. Returns true when it scrolled, so
   the caller skips the player Y-position update (the `bcs` at bank7:4736). */
static bool contra_set_vertical_level_frame_scroll(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t y = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    uint8_t coeff;
    uint8_t carry;
    uint8_t new_y;

    if (y >= 0x50u)
    {
        return false;
    }
    if ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) == 0u)
    {
        return false; /* falling (or stationary): don't scroll */
    }
    if (contra_rom_set_boss_auto_scroll(core))
    {
        return false; /* boss-reveal auto-scroll takes over (bank7:5220-5221) */
    }

    coeff = (uint8_t)(ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] +
                      ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index]);
    carry = (coeff < ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index]) ? 1u : 0u;
    ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] = coeff;
    new_y = (uint8_t)((uint16_t)y +
                      (uint16_t)ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] +
                      (uint16_t)carry);

    ram[CONTRA_RAM_FRAME_SCROLL] = (uint8_t)(y - new_y);
    ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + player_index] = 0x01u;
    return true;
}

static uint8_t contra_adjust_bg_collision_y(uint8_t screen_y, uint8_t vertical_scroll)
{
    const uint16_t sum = (uint16_t)screen_y + (uint16_t)vertical_scroll;
    uint8_t adjusted = (uint8_t)sum;

    if (((sum >> 8u) != 0u) || (adjusted >= 0xF0u))
    {
        adjusted = (uint8_t)(adjusted + 0x10u);
    }

    return adjusted;
}

static uint8_t contra_get_x_velocity_d_pad_code(const ContraCore *core, uint8_t player_index)
{
    const uint8_t controller = core->ram[CONTRA_RAM_CONTROLLER_STATE + player_index];
    uint8_t motion_code = 0x00u;

    if ((controller & CONTRA_BUTTON_RIGHT) != 0u)
    {
        motion_code = 0x20u;
    }

    if ((controller & CONTRA_BUTTON_LEFT) != 0u)
    {
        motion_code = 0x40u;
    }

    return motion_code;
}

static void contra_set_player_x_velocity_from_code(ContraCore *core, uint8_t player_index, uint8_t motion_code)
{
    const uint8_t controller = core->ram[CONTRA_RAM_CONTROLLER_STATE + player_index];

    if ((motion_code & 0x40u) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0xFFu;
    }
    else if ((motion_code & 0x20u) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x01u;
    }
    else
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
    }

    if ((core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] != 0u) &&
        (((controller & CONTRA_BUTTON_DOWN) != 0u) ||
         ((core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] & 0x80u) != 0u)))
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
    }
}

static void contra_end_indoor_transition(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] = 0x00u;
    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_Y + player_index];
    ram[CONTRA_RAM_SPRITE_X_POS + player_index] = ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_X + player_index];
}

/* set_player_advancing_vel (bank7:4882): the walk-into-the-screen velocities.
   A speed code rotates 7 bits right into a fast:fract pair (i.e. code/128
   px/frame). Y always uses code 0x58 negated (-0x00B0, a slow upward drift);
   X uses the player's distance to the screen center |x - 0x80|, negated when
   starting right of center -- so both players converge on the door. A
   game-over player 2 at x=0 gets exactly +1 px/frame (code 0x80). */
static void contra_set_player_indoor_advancing_velocity(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t sprite_x = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
    const uint8_t dx = (uint8_t)(sprite_x - 0x80u);
    const uint8_t mag = (sprite_x >= 0x80u) ? dx : (uint8_t)(0u - dx);
    uint16_t v = (uint16_t)((uint16_t)mag << 1u);

    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] = 0xFFu;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index] = 0x50u;
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_Y + player_index] = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_X + player_index] = sprite_x;

    if (sprite_x >= 0x80u)
    {
        v = (uint16_t)(0u - v); /* right of center: walk left */
    }
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = (uint8_t)(v >> 8u);
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = (uint8_t)v;
}

/* The Up-press on a cleared indoor screen (bank7:4602 @indoor_screen_cleared):
   if the OTHER player is alive but not standing ready (state 1, not jumping),
   the press only sets the presser's standing sprite. Otherwise BOTH player
   slots get the walking sequence, and any slot not already advancing (a
   game-over player 2 included!) is armed with the advance flag + velocities.
   The walk restarts this way for each of the 4 background screens (the held
   Up re-triggers after every swap ends the segment). */
static void contra_start_indoor_room_advance(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t other = (uint8_t)(player_index ^ 1u);
    int x;

    if ((ram[CONTRA_RAM_P1_GAME_OVER_STATUS + other] == 0u) &&
        ((ram[CONTRA_RAM_PLAYER_STATE + other] != 0x01u) ||
         (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + other] != 0u)))
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x00u;
        return;
    }

    for (x = 1; x >= 0; --x)
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + x] = 0x05u;
        if (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + x] != 0u)
        {
            continue;
        }
        ram[CONTRA_RAM_INDOOR_SCROLL] = 0x01u;
        ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + x] = 0x01u;
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + x] = 0x00u;
        ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + x] = 0x00u;
        contra_set_player_indoor_advancing_velocity(core, (uint8_t)x);
    }
}

static bool contra_handle_indoor_player_up_input(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0x01u) ||
        ((ram[CONTRA_RAM_CONTROLLER_STATE + player_index] & CONTRA_BUTTON_UP) == 0u))
    {
        return false;
    }
    /* handle_d_pad (bank7:4561) shifts RIGHT, LEFT, DOWN out of the pad first;
       d_pad_up_pressed only runs when UP is the lone direction. UP+LEFT keeps
       walking and does NOT arm the room advance (frame 29017). */
    if ((ram[CONTRA_RAM_CONTROLLER_STATE + player_index] &
         (CONTRA_BUTTON_RIGHT | CONTRA_BUTTON_LEFT | CONTRA_BUTTON_DOWN)) != 0u)
    {
        return false;
    }

    ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x01u;
    if (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0u)
    {
        if (ram[CONTRA_RAM_DEMO_MODE] != 0u)
        {
            return true;
        }

        ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] = 0x30u;
        contra_play_sound(core, 0x1Cu);
        return true;
    }

    contra_start_indoor_room_advance(core, player_index);
    return true;
}

static bool contra_update_indoor_player_transition(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint16_t sum;

    if (ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + player_index] != 0u)
    {
        ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + player_index] = 0x00u;
        ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] = 0x00u;
        contra_end_indoor_transition(core, player_index);
        contra_set_jump_status_and_y_velocity(core, player_index);
        return true;
    }

    if (ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] != 0u)
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x01u;
        ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] =
            (uint8_t)(ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] - 1u);
        if (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] != 0u)
        {
            ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] = 0x00u;
        }
        return true;
    }

    if (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] == 0u)
    {
        return false;
    }

    ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x05u;
    sum = (uint16_t)ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] +
        (uint16_t)ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index];
    ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] = (uint8_t)sum;
    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] +
                  ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] +
                  (uint8_t)(sum >> 8u));

    if (ram[CONTRA_RAM_INDOOR_SCROLL] >= 0x02u)
    {
        contra_end_indoor_transition(core, player_index);
    }

    return true;
}

/* Snap a screen Y to the standing height the game settles a grounded actor to on
   the outdoor horizontal level: align to the 16px grid (relative to the vertical
   scroll) and add the 4px fine offset that seats feet on the grass surface. */
static uint8_t contra_outdoor_landing_snap_y(const ContraCore *core, uint8_t reference_y)
{
    const uint8_t vertical_offset =
        (uint8_t)((core->ram[CONTRA_RAM_VERTICAL_SCROLL] & 0x0Fu) | 0xF0u);
    uint8_t landing_y = (uint8_t)(reference_y + vertical_offset);

    landing_y &= 0xF0u;
    landing_y = (uint8_t)(landing_y - vertical_offset);
    landing_y = (uint8_t)(landing_y + 0x04u);
    return landing_y;
}

static void contra_set_player_landing_y_offset(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t location_type = ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];

    if ((location_type & 0x80u) != 0u)
    {
        ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = 0xC9u;
        return;
    }

    if (location_type != 0u)
    {
        ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = 0xA8u;
        return;
    }

    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
        contra_outdoor_landing_snap_y(core, ram[CONTRA_RAM_SPRITE_Y_POS + player_index]);
}

static void contra_land_player_on_ground(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    /* player_land_on_ground (bank7:4847). When arriving from a jump or edge
       fall: reset the walk animation phase and the fall X freeze, and play the
       landing sound. PLAYER_SPECIAL_SPRITE_TIMER is NOT reset -- the somersault
       spin phase carries across landings into the next jump. */
    if ((uint8_t)(ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] |
                  ram[CONTRA_RAM_EDGE_FALL_CODE + player_index]) != 0u)
    {
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
        ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
        ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index] = 0x00u;
        contra_play_sound(core, 0x03u); /* sound_03: landing */
    }

    /* @check_aim_dir: re-derive the horizontal flip from the aim direction */
    {
        uint8_t flip = (uint8_t)(ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x3Fu);

        if (ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u)
        {
            flip |= 0x40u;
        }
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
    }

    contra_init_player_data(core, player_index);
}

static void contra_begin_player_edge_fall(ContraCore *core, uint8_t player_index, uint8_t edge_fall_code)
{
    uint8_t *const ram = core->ram;
    uint8_t freeze_y = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + 0x14u);

    ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] = edge_fall_code;

    if (freeze_y < ram[CONTRA_RAM_SPRITE_Y_POS + player_index])
    {
        freeze_y = 0xFFu;
    }

    ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index] = freeze_y;
}

static void contra_walk_player_off_ledge(ContraCore *core, uint8_t player_index)
{
    contra_begin_player_edge_fall(
        core,
        player_index,
        (core->ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u) ? 0x41u : 0x21u
    );
}

static void contra_check_player_ledge(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t collision_code;

    if ((ram[CONTRA_RAM_PLAYER_ON_ENEMY + player_index] != 0u) ||
        (ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] != 0u) ||
        (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u) ||
        (ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] != 0u) ||
        (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] != 0u))
    {
        return;
    }

    if ((ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] & 0x01u) != 0u)
    {
        ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] = 0x00u;
        return;
    }

    collision_code = contra_get_player_bg_collision_code(core, player_index);
    if (collision_code == 0u)
    {
        contra_walk_player_off_ledge(core, player_index);
        return;
    }

    if (collision_code == 0x02u)
    {
        ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x01u;
        return;
    }

    if (collision_code != 0x02u)
    {
        ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x00u;
    }
}

static void contra_update_player_edge_fall(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t motion_code;

    if (ram[CONTRA_RAM_SPRITE_Y_POS + player_index] >= ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index])
    {
        const uint8_t collision_code = contra_get_player_bg_collision_code(core, player_index);

        if (collision_code != 0u)
        {
            contra_set_player_landing_y_offset(core, player_index);
            contra_land_player_on_ground(core, player_index);
            return;
        }
    }

    contra_apply_gravity_set_player_y(core, player_index);

    if (ram[CONTRA_RAM_SPRITE_Y_POS + player_index] >= ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index])
    {
        motion_code = contra_get_x_velocity_d_pad_code(core, player_index);
        if (motion_code != 0u)
        {
            ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] =
                (uint8_t)((ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] & 0x9Fu) | motion_code);
        }
    }

    contra_set_player_x_velocity_from_code(core, player_index, ram[CONTRA_RAM_EDGE_FALL_CODE + player_index]);
}

/* set_frame_scroll_if_appropriate (bank7:5141-5266), per player, called from
   inside calc_player_x_vel between the d-pad velocity and the move: when the
   player is at/past the scroll point, CONSUME the X velocity into FRAME_SCROLL
   (so the move that follows applies zero and the player stays put). */
static void contra_set_frame_scroll_if_appropriate(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    static const uint8_t horizontal_scroll_point_tbl[3] = {0x80u, 0x80u, 0xB0u};
    unsigned accum;

    if (((uint8_t)(ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] |
                   ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] |
                   ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01]) != 0u) ||
        (ram[CONTRA_RAM_PLAYER_STATE + player_index] != 0x01u))
    {
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        /* set_vertical_level_frame_scroll (bank7:5236): while jumping up past
           Y 0x50, scroll the screen by the jump velocity instead of moving the
           player. Reached from the horizontal move on vertical levels too --
           the ROM's double-application "platform skip" quirk is faithful. */
        unsigned coeff;
        uint8_t new_y;

        if ((ram[CONTRA_RAM_SPRITE_Y_POS + player_index] >= 0x50u) ||
            ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) == 0u))
        {
            return;
        }
        if (contra_rom_set_boss_auto_scroll(core))
        {
            return;
        }
        coeff = (unsigned)ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] +
            ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index];
        ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] = (uint8_t)coeff;
        new_y = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] +
                          ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] +
                          (uint8_t)(coeff >> 8u));
        ram[CONTRA_RAM_FRAME_SCROLL] =
            (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - new_y);
        ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + player_index] = 0x01u;
        return;
    }

    if ((ram[CONTRA_RAM_LEVEL_STOP_SCROLL] & 0x80u) != 0u)
    {
        return; /* boss auto-scroll has latched */
    }
    if (ram[CONTRA_RAM_SPRITE_X_POS + player_index] <
        horizontal_scroll_point_tbl[ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD] % 3u])
    {
        return;
    }
    if (ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD] == 0x02u)
    {
        /* both players active: the other player at the left edge blocks the
           scroll AND stops the scroller (@stop_player_x_velocity) */
        if (ram[CONTRA_RAM_SPRITE_X_POS + (player_index ^ 1u)] < 0x21u)
        {
            ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x00u;
            ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
            return;
        }
    }
    if (contra_rom_set_boss_auto_scroll(core))
    {
        return;
    }

    /* @set_horizontal_level_frame_scroll: consume the velocity into the scroll */
    accum = (unsigned)ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] +
        ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index];
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] = (uint8_t)accum;
    ram[CONTRA_RAM_FRAME_SCROLL] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] + (accum >> 8u));
    ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + player_index] = 0x01u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
}

/* calc_player_x_vel (bank7:4357-4438): apply the X velocity. The scroll check
   runs INSIDE the rightward branch (after the solid/right-edge/boss-max gates)
   and may consume the velocity, in which case the add below applies zero.
   Once the level boss is defeated (BOSS_DEFEATED_FLAG bit 7, the scripted
   end-of-level walk) the geometry gates are skipped so the player can walk
   past the boss-screen solids. */
static void contra_move_player_horizontally(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    static const uint8_t lvl_boss_max_x_scroll_tbl[8] = {
        0x90u, 0xFFu, 0xFFu, 0xFFu, 0xA0u, 0xD0u, 0xB0u, 0xB0u};
    const uint8_t screen_type = contra_get_level_screen_type(core);
    const uint8_t right_edge = (screen_type == 0u) ? 0xE6u : ((screen_type == 1u) ? 0xE0u : 0xD0u);
    const uint8_t left_edge = (screen_type == 0u) ? 0x1Au : ((screen_type == 1u) ? 0x20u : 0x30u);
    const bool boss_defeated_walk = (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] & 0x80u) != 0u;
    uint8_t velocity;
    unsigned accum;

    /* fold in the velocity boost from riding a moving platform */
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] +
                  ram[CONTRA_RAM_PLAYER_FAST_X_VEL_BOOST + player_index]);
    velocity = ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index];

    if ((uint8_t)(velocity | ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index]) == 0u)
    {
        return;
    }

    if ((velocity & 0x80u) == 0u)
    {
        if (!boss_defeated_walk)
        {
            if (contra_player_has_solid_collision_ahead(core, player_index, 8) ||
                (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >= right_edge))
            {
                return;
            }
            if ((ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] != 0u) &&
                (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >=
                 lvl_boss_max_x_scroll_tbl[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u]))
            {
                return;
            }
        }
        contra_set_frame_scroll_if_appropriate(core, player_index);
        velocity = ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index];
    }
    else
    {
        if (contra_player_has_solid_collision_ahead(core, player_index, -8))
        {
            return;
        }
        if (!boss_defeated_walk &&
            (ram[CONTRA_RAM_SPRITE_X_POS + player_index] < left_edge))
        {
            return;
        }
    }

    /* @apply_vel_to_player_x_pos */
    accum = (unsigned)ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] +
        ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index];
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] = (uint8_t)accum;
    ram[CONTRA_RAM_SPRITE_X_POS + player_index] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] + velocity + (accum >> 8u));
}

/* auto_scroll_player (bank7:4279-4296), the tail of player_state_routine_01
   and the head of player_state_routine_02: while a boss-reveal auto-scroll
   timer runs, push the player back against the scroll every frame -- 1px left
   on horizontal levels (stopping at the outdoor left edge 0x1A), 1px down on
   the vertical level. */
static void contra_auto_scroll_player(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    if ((uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] |
                  ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01]) == 0u)
    {
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
            (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + 1u);
        return;
    }

    if (ram[CONTRA_RAM_SPRITE_X_POS + player_index] < 0x1Au)
    {
        return;
    }

    ram[CONTRA_RAM_SPRITE_X_POS + player_index] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] - 1u);
}

static void contra_handle_player_fall_out(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const bool demo_mode = ram[CONTRA_RAM_DEMO_MODE] != 0u;
    const bool demo_life_floor_reached =
        demo_mode && (ram[CONTRA_RAM_P1_NUM_LIVES + player_index] <= 0x61u);

    /* init_player_and_weapon (bank7:5314): reset current weapon to the default
       rifle on death, then fall through to init_player_attributes. */
    ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index] = 0x00u;
    if ((player_index == 0u) && contra_env_flag_enabled("CONTRA_KEEP_START_WEAPON"))
    {
        uint8_t start_weapon;

        if (contra_parse_start_weapon(getenv("CONTRA_START_WEAPON"), &start_weapon))
        {
            ram[CONTRA_RAM_P1_CURRENT_WEAPON] = start_weapon;
        }
    }

    contra_init_player_attributes(core, player_index);
    ram[CONTRA_RAM_PLAYER_STATE + player_index] = 0x00u;

    if (ram[CONTRA_RAM_P1_NUM_LIVES + player_index] == 0u)
    {
        ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] = 0x01u;
    }
    else if (!demo_life_floor_reached)
    {
        ram[CONTRA_RAM_P1_NUM_LIVES + player_index] =
            (uint8_t)(ram[CONTRA_RAM_P1_NUM_LIVES + player_index] - 1u);
    }
}

static void contra_kill_player(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    contra_play_sound(core, 0x52u);
    contra_init_player_data(core, player_index);
    ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x00u;
    ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_DEATH_FLAG + player_index] = 0x01u;
    ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0xFDu;
    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x80u;
    ram[CONTRA_RAM_PLAYER_STATE + player_index] = 0x02u;
}

static void contra_set_player_aim_for_input(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t table_row;
    uint8_t aim_context;
    uint8_t dpad = (uint8_t)(ram[CONTRA_RAM_CONTROLLER_STATE + player_index] & 0x0Fu);

    /* bank7:4965-4990 set_player_aim_for_input
     * Default to the jump-aim rows ($20 standing / $30 jumping). These rows are
     * only actually used on the indoor (base) boss screen; for every other
     * screen the row is overridden below with the facing-direction rows
     * ($00/$10) regardless of jump status. */
    table_row = (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u) ? 0x30u : 0x20u;

    /* bank7:4110-4119 run_player_state_routine computes $08: it is #$03 on the
     * indoor (base) boss screen (LEVEL_LOCATION_TYPE bit 7 set), otherwise the
     * level_spawn_position_index entry (values 0/1/2 only, never 3). Only the
     * indoor-boss case keeps the jump-aim row. */
    aim_context = ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u)
        ? 0x03u
        : contra_level_spawn_position_index[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u];

    if (aim_context != 0x03u)
    {
        table_row = (ram[CONTRA_RAM_PLAYER_AIM_PREV_FRAME + player_index] >= 0x05u)
            ? 0x10u
            : 0x00u;
    }

    ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] = contra_d_pad_player_aim_tbl[table_row + dpad];
}

static void contra_set_jump_status_and_y_velocity(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] =
        (ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u) ? 0x91u : 0x11u;
    ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;

    /* the ROM tests only BIT 0 of the location type (`lsr`, bank7:4954): the
       indoor BOSS room (0x80) jumps with the outdoor -5.94, not the indoor
       -4.56 -- that's the jump into the boss room from the last corridor. */
    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x01u) == 0u)
    {
        ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0xFBu;
        ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0xF0u;
    }
    else
    {
        ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0xFCu;
        ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x90u;
    }
}

static void contra_handle_player_jump(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t current_y_fast_velocity = ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index];
    uint8_t motion_code;

    if ((current_y_fast_velocity & 0x80u) != 0u)
    {
        if ((ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0u) &&
            (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
            (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u))
        {
            const uint8_t sample_y = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - 0x0Au);
            const uint8_t collision = contra_get_outdoor_horizontal_bg_collision(
                core,
                ram[CONTRA_RAM_SPRITE_X_POS + player_index],
                sample_y
            );

            if (collision == 0x80u)
            {
                ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0x00u;
                ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x00u;
            }
        }
    }
    else
    {
        bool should_check_collision = true;

        if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u)
        {
            if (current_y_fast_velocity < 0x01u)
            {
                should_check_collision = false;
            }
            else if (current_y_fast_velocity < 0x04u)
            {
                const uint8_t adjusted_y = contra_adjust_bg_collision_y(
                    ram[CONTRA_RAM_SPRITE_Y_POS + player_index],
                    ram[CONTRA_RAM_VERTICAL_SCROLL]
                );

                if ((adjusted_y & 0x0Fu) >= 0x08u)
                {
                    should_check_collision = false;
                }
            }
        }

        if (should_check_collision &&
            (contra_get_player_bg_collision_code(core, player_index) != 0u))
        {
            motion_code = contra_get_x_velocity_d_pad_code(core, player_index);
            if (motion_code != 0u)
            {
                ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] =
                    (uint8_t)((ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] & 0x9Fu) | motion_code);
            }

            contra_set_player_x_velocity_from_code(
                core,
                player_index,
                ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index]
            );
            contra_set_player_landing_y_offset(core, player_index);
            contra_land_player_on_ground(core, player_index);
            return;
        }
    }

    /* @apply_gravity (bank7:4730-4740): gravity always runs; on a vertical level a
       climbing player scrolls the screen up instead of moving the sprite up. When
       the scroll is taken, player_jumping_set_y_pos is skipped (the `bcs` branch). */
    contra_apply_gravity(core, player_index);
    if ((ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u) ||
        !contra_set_vertical_level_frame_scroll(core, player_index))
    {
        contra_player_jumping_set_y_pos(core, player_index);
    }

    motion_code = contra_get_x_velocity_d_pad_code(core, player_index);
    if (motion_code != 0u)
    {
        ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] =
            (uint8_t)((ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] & 0x9Fu) | motion_code);
    }

    contra_set_player_x_velocity_from_code(
        core,
        player_index,
        ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index]
    );
}

static void contra_handle_d_pad(ContraCore *core, uint8_t player_index)
{
    const uint8_t controller = core->ram[CONTRA_RAM_CONTROLLER_STATE + player_index];
    const uint8_t water_state = core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index];

    if ((controller & CONTRA_BUTTON_RIGHT) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] =
            (((water_state & 0x80u) != 0u) || ((water_state != 0u) && ((controller & CONTRA_BUTTON_DOWN) != 0u)))
            ? 0x00u
            : 0x01u;
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x03u;
        return;
    }

    if ((controller & CONTRA_BUTTON_LEFT) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] =
            (((water_state & 0x80u) != 0u) || ((water_state != 0u) && ((controller & CONTRA_BUTTON_DOWN) != 0u)))
            ? 0x00u
            : 0xFFu;
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x03u;
        return;
    }

    core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
    if ((controller & CONTRA_BUTTON_DOWN) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x02u;
    }
    else if ((controller & CONTRA_BUTTON_UP) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x01u;
    }
    else
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x00u;
    }
}

/* player_state_routine_00's landing test (bank7:4205 @check_if_floor_exit +
   check_collision_below bank7:5296): a spawn column is landable when there is
   no solid at the spawn row, no solid at y+0x20, and SOME collision (floor,
   water or solid) in the 16px rows below it down to row 0x0E. */
static bool contra_rom_spawn_spot_landable(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t x = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
    const uint8_t y = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    uint8_t code = contra_get_player_bg_collision_code(core, player_index);
    uint8_t row;

    if ((code & 0x80u) != 0u)
    {
        return false; /* solid object at the spawn row */
    }
    code = contra_rom_get_bg_collision_far(core, x, (uint8_t)(y + 0x20u));
    if ((code & 0x80u) != 0u)
    {
        return false;
    }
    for (row = (uint8_t)(((uint8_t)(y + 0x20u) >> 4u) + 1u); row < 0x0Eu; ++row)
    {
        if (contra_get_outdoor_bg_collision(core, x, (uint8_t)(row << 4u)) != 0u)
        {
            return true; /* something to land on below */
        }
    }
    return false;
}

static void contra_run_player_state_routine(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t current_level = ram[CONTRA_RAM_CURRENT_LEVEL];

    if (current_level > 7u)
    {
        current_level = 7u;
    }

    switch (ram[CONTRA_RAM_PLAYER_STATE + player_index])
    {
        case 0x00u:
        {
            const uint8_t spawn_index = (uint8_t)(((uint8_t)(player_index << 2u)) + contra_level_spawn_position_index[current_level]);

            contra_init_player_attributes(core, player_index);
            ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = contra_vertical_spawn_position[spawn_index];
            ram[CONTRA_RAM_SPRITE_X_POS + player_index] = contra_horizontal_spawn_position[spawn_index];
            ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] = 0x01u;

            /* player_state_routine_00 (bank7:4166-4188), outdoor only: if the
               table spawn column has nothing to land on, restart at x=0x10 and
               walk right in 0x10 steps; past 0xE0 fall back to x=0x30. */
            if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
                !contra_rom_spawn_spot_landable(core, player_index))
            {
                ram[CONTRA_RAM_SPRITE_X_POS + player_index] = 0x10u;
                for (;;)
                {
                    uint8_t sx;

                    if (contra_rom_spawn_spot_landable(core, player_index))
                    {
                        break;
                    }
                    sx = (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] + 0x10u);
                    ram[CONTRA_RAM_SPRITE_X_POS + player_index] = sx;
                    if (sx >= 0xE0u)
                    {
                        ram[CONTRA_RAM_SPRITE_X_POS + player_index] = 0x30u;
                        break;
                    }
                }
            }
            ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x02u;
            ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0x00u;
            ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] = 0x80u;
            ram[CONTRA_RAM_PLAYER_STATE + player_index] = 0x01u;
            break;
        }

        case 0x01u:
        {
            contra_set_player_aim_for_input(core, player_index);
            contra_check_player_ledge(core, player_index);
            contra_check_player_fire(core, player_index);

            /* handle_player_state_calc_x_vel (bank7:4359): the per-frame X
               velocity reset is STATE-1 ONLY -- a dead player's corpse keeps
               its last velocity byte (the ROM never clears it). */
            if (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] == 0u)
            {
                ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
                ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x00u;
            }

            /* handle_player_state (bank7:4455) defaults the sprite sequence to
               3 (walking/curled-jump) every frame; the d-pad branches overwrite
               it for standing/aiming/crouching, so it survives exactly while
               jumping or falling -- including the spawn drop-in. */
            ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x03u;

            if (contra_update_indoor_player_transition(core, player_index))
            {
            }
            else if (ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] != 0u)
            {
                contra_update_player_edge_fall(core, player_index);
            }
            else if (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
            {
                contra_handle_player_jump(core, player_index);
            }
            else
            {
                const bool jump_pressed =
                    (ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] == 0u) &&
                    ((ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_index] & CONTRA_BUTTON_A) != 0u);

                if (jump_pressed)
                {
                    const uint8_t directional =
                        (uint8_t)(ram[CONTRA_RAM_CONTROLLER_STATE + player_index] &
                                  (CONTRA_BUTTON_DOWN | CONTRA_BUTTON_LEFT | CONTRA_BUTTON_RIGHT));

                    if (directional == CONTRA_BUTTON_DOWN)
                    {
                        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x02u;
                        if (contra_can_player_drop_down(core, player_index))
                        {
                            contra_begin_player_edge_fall(core, player_index, 0x81u);
                        }
                    }
                    else
                    {
                        contra_set_jump_status_and_y_velocity(core, player_index);
                    }
                }
                else if (!contra_handle_indoor_player_up_input(core, player_index))
                {
                    contra_handle_d_pad(core, player_index);
                }
            }

            if ((ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] & 0x80u) != 0u)
            {
                ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
            }

            contra_move_player_horizontally(core, player_index);
            contra_auto_scroll_player(core, player_index);

            contra_set_player_sprite_and_attrs(core, player_index);
            ram[CONTRA_RAM_PLAYER_AIM_PREV_FRAME + player_index] = ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index];

            if ((ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] == 0u) &&
                (ram[CONTRA_RAM_SPRITE_Y_POS + player_index] >= 0xE8u))
            {
                contra_kill_player(core, player_index);
            }
            break;
        }

        case 0x02u:
        {
            const uint8_t screen_type = contra_get_level_screen_type(core);

            contra_auto_scroll_player(core, player_index);
            ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = contra_player_dead_sequence_tbl[screen_type];
            if (ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] != 0u)
            {
                ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] - 1u);
                if (ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] == 0u)
                {
                    contra_handle_player_fall_out(core, player_index);
                    break;
                }
            }
            else
            {
                int8_t x_velocity = contra_player_died_x_velocity_tbl[screen_type];

                ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = (uint8_t)x_velocity;
                if (ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u)
                {
                    ram[CONTRA_RAM_PLAYER_DEATH_FLAG + player_index] |= 0x02u;
                    x_velocity = (int8_t)(-x_velocity);
                    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = (uint8_t)x_velocity;
                }

                if (((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) == 0u) &&
                    (ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] >= 0x02u))
                {
                    if (ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] == 0x01u)
                    {
                        ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x40u;
                    }
                    else
                    {
                        const uint8_t collision_code = contra_get_player_bg_collision_code(core, player_index);

                        if ((collision_code != 0u) && (collision_code != 0x02u))
                        {
                            contra_set_player_landing_y_offset(core, player_index);
                            ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x40u;
                        }
                    }
                }

                contra_apply_gravity_set_player_y(core, player_index);
                contra_move_player_horizontally(core, player_index);
            }

            contra_set_player_sprite_and_attrs(core, player_index);
            break;
        }

        default:
            contra_set_player_sprite_and_attrs(core, player_index);
            break;
    }
}

static void contra_handle_invincibility_and_weapon_strength(ContraCore *core, uint8_t player_index)
{
    contra_run_player_state_routine(core, player_index);

    if (core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] != 0u)
    {
        core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] =
            (uint8_t)(core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] - 1u);
    }
    else if (core->ram[CONTRA_RAM_PLAYER_STATE + player_index] == 0x01u)
    {
        core->ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] = 0x01u;
    }

    if ((core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] != 0u) &&
        ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) == 0u))
    {
        core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] =
            (uint8_t)(core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] - 1u);
    }

    if (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] =
            (uint8_t)(core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] - 1u);
    }

    core->ram[CONTRA_RAM_PLAYER_FAST_X_VEL_BOOST + player_index] = 0x00u;

    {
        const uint8_t weapon = (uint8_t)(core->ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index] & 0x07u);
        const uint8_t strength = contra_weapon_strength_tbl[(weapon < 5u) ? weapon : 0u];

        if (strength >= core->ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH])
        {
            core->ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] = strength;
        }
    }
}
