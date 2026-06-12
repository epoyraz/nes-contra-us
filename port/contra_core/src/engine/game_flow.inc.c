/* Intro, demo, player-select, level setup, palette setup, input, and scroll flow.
   Included by core.c; not compiled as a separate translation unit. */


static bool contra_is_native_level_1_active(const ContraCore *core)
{
    const uint8_t game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];

    return ((game_routine == 0x05u) ||
            ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0u) &&
        (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u);
}

static bool contra_is_native_level_2_active(const ContraCore *core)
{
    const uint8_t game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    const uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    return ((game_routine == 0x05u) ||
            ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        ((level == 0x01u) || (level == 0x03u)) &&
        ((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) ||
         (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u));
}

static bool contra_is_native_level_7_active(const ContraCore *core)
{
    const uint8_t game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];

    return ((game_routine == 0x05u) ||
            ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u) &&
        (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u);
}

/* Levels 5, 6, and 8: outdoor horizontal levels whose enemies stamp
   background overlays (tank/boss UFO, fire beams, alien guardian/spawns/
   heart) through the position-keyed caches. */
static bool contra_is_native_overlay_level_active(const ContraCore *core)
{
    const uint8_t game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    const uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    return ((game_routine == 0x05u) ||
            ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        ((level == 0x04u) || (level == 0x05u) || (level == 0x07u)) &&
        (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u);
}

static bool contra_is_native_combat_active(const ContraCore *core)
{
    return contra_is_native_level_1_active(core) ||
        contra_is_native_level_2_active(core) ||
        contra_is_native_level_7_active(core) ||
        contra_is_native_overlay_level_active(core);
}

/* Find the screen Y of the top of the first solid floor tile at or below
   start_y, scanning at the 8px collision-tile resolution (the coarse finder
   above steps 16px). Returns false if no solid ground is found. */
/* Seat a grounded enemy at the same height the player would settle to on the
   floor beneath it. The enemy and player share the +0x10 foot anchor, so
   running the detected floor surface through the player's landing snap keeps
   ground enemies aligned with the player instead of sitting below the floor. */
static void contra_load_bank_2_set_players_paused_sprite_attr(ContraCore *core)
{
    contra_set_player_sprite_and_attrs(core, 0u);
    contra_set_player_sprite_and_attrs(core, 1u);
}

/* scroll_player (bank7:4012-4050): keep the player who is NOT driving the
   scroll at their on-screen position. Horizontal levels walk them back 1px
   (the ROM assumes 1px scroll/frame -- including its documented moving-cart
   2px bug); vertical levels carry them down by FRAME_SCROLL. */
static void contra_scroll_vertical_non_scrolling_player(ContraCore *core, uint8_t active_players)
{
    uint8_t *const ram = core->ram;
    size_t scrolled_player;

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 0u] == ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u]))
    {
        return;
    }
    scrolled_player = (ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 0u] < ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u]) ? 0u : 1u;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u)
    {
        /* Horizontal: the ROM decs unconditionally and relies on the
           game-over path re-parking a dead player's sprite afterwards; the
           port skips the dec for them instead (same NMI-visible state). */
        if (ram[CONTRA_RAM_P1_GAME_OVER_STATUS + scrolled_player] == 0u)
        {
            ram[CONTRA_RAM_SPRITE_X_POS + scrolled_player] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + scrolled_player] - 1u);
        }
        return;
    }

    if ((active_players != 0x03u) ||
        (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] != 0u) ||
        (ram[CONTRA_RAM_FRAME_SCROLL] == 0u))
    {
        return;
    }
    ram[CONTRA_RAM_SPRITE_Y_POS + scrolled_player] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + scrolled_player] + ram[CONTRA_RAM_FRAME_SCROLL]);
    if (ram[CONTRA_RAM_SPRITE_Y_POS + scrolled_player] < ram[CONTRA_RAM_FRAME_SCROLL])
    {
        ram[CONTRA_RAM_PLAYER_HIDDEN + scrolled_player] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_HIDDEN + scrolled_player] + 1u);
    }
}

/* animate_indoor_fence (bank7:2917): the electric fence isn't in the nametable --
   the ROM rebuilds the 4 CHR pattern tiles at PPU $1FC0 (pattern-table-1 tiles
   0xFC-0xFF, which the indoor room super-tiles reference) every animation frame by
   OR-ing a fixed tile "background" shape with a frame-cycled "electricity" overlay.
   The native renderer composes the background straight from ppu_pattern, so writing
   the animated CHR there is what makes the fence appear and flicker. Once the screen
   is cleared the blank overlay is written and the fence vanishes. */
static const uint8_t contra_pattern_tile_bg_00[0x40] = {
    0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu,
    0xFEu, 0xFCu, 0xF8u, 0xF0u, 0xE0u, 0xC0u, 0x80u, 0x00u, 0x00u, 0x01u, 0x03u, 0x07u, 0x0Fu, 0x1Fu, 0x3Fu, 0x7Fu,
    0x7Fu, 0x3Fu, 0x1Fu, 0x0Fu, 0x07u, 0x03u, 0x01u, 0x00u, 0x00u, 0x80u, 0xC0u, 0xE0u, 0xF0u, 0xF8u, 0xFCu, 0xFEu};
static const uint8_t contra_pattern_tile_fence_tbl[0x28] = {
    0x00u, 0x00u, 0x04u, 0x44u, 0xEBu, 0x32u, 0x20u, 0x00u,
    0x00u, 0x00u, 0x10u, 0x30u, 0xEBu, 0x6Au, 0x44u, 0x00u,
    0x00u, 0x00u, 0x08u, 0x0Cu, 0xD7u, 0x56u, 0x22u, 0x00u,
    0x00u, 0x00u, 0x20u, 0x22u, 0xD7u, 0x4Cu, 0x04u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u}; /* blank: no fence */

static void contra_animate_indoor_fence(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t base;
    unsigned y;

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0x01u)
    {
        return; /* indoor boss screen (0x80) / non-indoor: no fence */
    }

    if (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0u)
    {
        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u) != 0u)
        {
            return; /* electricity is redrawn only every 4th frame */
        }
        base = (uint8_t)((ram[CONTRA_RAM_FRAME_COUNTER] & 0x0Cu) << 1u); /* 0x00/0x08/0x10/0x18 */
    }
    else if ((ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] & 0x80u) == 0u)
    {
        base = 0x20u; /* screen cleared: blank overlay removes the fence (once) */
        ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x80u;
    }
    else
    {
        return; /* fence already removed */
    }

    /* the fence CHR rewrite occupies 0x45 bytes of CPU_GRAPHICS_BUFFER
       (header 5 + 0x40 pattern bytes) -- on these frames a full enemy
       super-tile stamp finds the buffer past its 0x40 entry check and must
       retry next frame (bank7:1353). */
    ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] =
        (uint8_t)(ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] + 0x45u);

    for (y = 0u; y < 0x40u; ++y)
    {
        core->ppu_pattern[0x1FC0u + y] =
            (uint8_t)(contra_pattern_tile_bg_00[y] |
                      contra_pattern_tile_fence_tbl[base + (y & 0x07u)]);
    }
}

static void contra_load_bank_3_handle_scroll(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t scroll_pixels = (uint8_t)(ram[CONTRA_RAM_FRAME_SCROLL] + ram[CONTRA_RAM_TANK_AUTO_SCROLL]);

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) &&
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u))
    {
        contra_animate_indoor_fence(core); /* runs every frame, regardless of scroll */

        if ((ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] & 0x80u) != 0u)
        {
            /* @indoor_screen_transition (bank7:5845): the 0x20-frame background
               segment finished last frame -- swap to the next of the 4 corridor
               screens. The walking player sees INDOOR_SCROLL=2 this frame and
               ends the segment (restoring position); the still-held Up press
               re-arms the next segment. */
        }
        else if (ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] != 0u)
        {
            /* @write_column_tiles_exit: stream one nametable column per frame */
            ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] =
                (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] + 1u);
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] =
                (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] + 1u);
            ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] - 1u);
            if (ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] == 0u)
            {
                ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x80u; /* swap next frame */
            }
            return;
        }
        else
        {
            if (ram[CONTRA_RAM_INDOOR_SCROLL] == 0u)
            {
                return;
            }
            /* segment init (bank7:5786): reset the streaming counters, flip the
               write nametable, load the NEXT corridor screen's super-tiles
               (screen*4 + offset + 1, note the SEC), then stream the FIRST
               column the same frame. */
            ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
            ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_LOW_BYTE] = 0x00u;
            ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x20u;
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] ^= 0x04u;
            ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] ^= 0x04u;
            contra_load_supertiles_screen_indexes(
                core,
                (uint8_t)((ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] * 4u) +
                          ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u)
            );
            ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x01u;
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x01u;
            ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x1Fu;
            return;
        }

        ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x00u;
        ram[CONTRA_RAM_INDOOR_SCROLL] =
            (uint8_t)(ram[CONTRA_RAM_INDOOR_SCROLL] + 1u); /* -> 2: ends the walk segment */
        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] =
            (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u);
        if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] >= 0x04u)
        {
            /* all 4 corridor screens shown: jump both players into the room */
            ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + 0u] =
                (uint8_t)(ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + 0u] + 1u);
            ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + 1u] =
                (uint8_t)(ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + 1u] + 1u);
            ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x00u;
            ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
            ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
            ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 1u);

            if (ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == ram[CONTRA_RAM_LEVEL_STOP_SCROLL])
            {
                ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x80u;
                ram[CONTRA_RAM_VERTICAL_SCROLL] = 0xE0u;
                contra_load_alternate_graphics(core);
                contra_init_apu_channels(core);
                /* load the boss-room CHR set: lda CURRENT_LEVEL; lsr; ora #$08;
                   jsr load_A_offset_graphic_data (bank7:5845-5848) -- list 8 for L2
                   (level_2_boss_graphic_data $03,$04,$13,$08), list 9 for L4. Without
                   this the cannon/plating/wall tiles render from the wrong CHR. */
                contra_load_graphic_data_list(
                    core, (uint8_t)(0x08u | (ram[CONTRA_RAM_CURRENT_LEVEL] >> 1u)));
                /* Compose the flat mechanical boss wall: repoint the super-tile /
                   tile-data / palette pointers to the boss tables (handle_indoor_scroll
                   pointer swap, bank7:5772-5787, source level_2_4_boss_graphics_data
                   bank7:5870) and reload the boss-wall layout. Screen index 0 = L2 wall,
                   1 = L4 wall (CURRENT_LEVEL>>1). Without this the boss room composes the
                   generic corridor layout (now drawn with boss CHR -> garbled). */
                ram[CONTRA_RAM_LEVEL_SCREEN_SUPERTILES_PTR] = 0x13u; /* $9013 boss screen ptr tbl */
                ram[CONTRA_RAM_LEVEL_SCREEN_SUPERTILES_PTR + 1u] = 0x90u;
                ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] = 0x7Au; /* $b57a boss super-tile data */
                ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] = 0xB5u;
                ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] = 0x7Au; /* $bd7a boss palette data */
                ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] = 0xBDu;
                contra_decode_level_screen_supertiles(
                    core, (uint8_t)(ram[CONTRA_RAM_CURRENT_LEVEL] >> 1u),
                    core->level_screen_supertiles, 0u);
                contra_play_sound(core, 0x42u);
            }
        }

        contra_load_supertiles_screen_indexes(
            core,
            (uint8_t)((ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] * 4u) + ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET])
        );
        ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x0Cu;
        contra_load_palettes_color_to_cpu(core, 0x20u);
        ram[CONTRA_RAM_PPUCTRL_SETTINGS] ^= 0x01u;
        return;
    }

    /* handle_vertical_scroll (bank7:5667-5747): advance the vertical-level camera.
       The original streams nametable rows into the PPU as it scrolls; the port's
       framebuffer renderer redraws from LEVEL_SCREEN_NUMBER / SCROLL_OFFSET each
       frame, so only the scroll bookkeeping is ported here (the PPU write-address
       and supertile-streaming steps are unnecessary). */
    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        uint8_t vertical_scroll_pixels;

        /* boss-reveal auto-scroll (bank7:5668-5676) */
        if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] != 0u)
        {
            ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x10u;
            ram[CONTRA_RAM_FRAME_SCROLL] = 0x01u;
            ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] =
                (uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] - 1u);
            if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] == 0u)
            {
                ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] =
                    (uint8_t)(ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] + 1u);
            }
        }

        vertical_scroll_pixels = ram[CONTRA_RAM_FRAME_SCROLL]; /* @init_loop, bank7:5678-5681 */
        while (vertical_scroll_pixels-- != 0u)                 /* @frame_scroll_loop */
        {
            ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u);
            if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] >= 0xF0u) /* bank7:5685-5699 */
            {
                ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
                ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
                ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 1u);
                if (ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == ram[CONTRA_RAM_LEVEL_ALT_GRAPHICS_POS])
                {
                    ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x01u;
                    ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x80u;
                    contra_load_alternate_graphics(core);
                }
            }

            if ((ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] & 0x07u) == 0u)
            {
                const uint8_t old_low = ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE];

                ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] =
                    (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] - 0x20u);
                if (old_low < 0x20u)
                {
                    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] =
                        (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] - 1u);
                }

                ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] =
                    (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] - 1u);
                if ((ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] & 0x80u) != 0u)
                {
                    ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] ^= 0x40u;
                    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x1Du;
                    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0xA0u;
                    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x23u;
                    contra_load_next_supertiles_screen_indexes(core);
                }
            }

            /* @dec_scroll_continue (bank7:5726-5732): VERTICAL_SCROLL counts down
               one PPU line per scrolled pixel, wrapping #$ff back to #$ef. */
            ram[CONTRA_RAM_VERTICAL_SCROLL] =
                (uint8_t)(ram[CONTRA_RAM_VERTICAL_SCROLL] - 1u);
            if (ram[CONTRA_RAM_VERTICAL_SCROLL] == 0xFFu)
            {
                ram[CONTRA_RAM_VERTICAL_SCROLL] = 0xEFu;
            }
        }
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        return;
    }

    /* @handle_outdoor_level (bank7:5575-5598): advance the boss-reveal auto-scroll.
       Tick AUTO_SCROLL_TIMER_01 then _00; while either is running force a 1px frame
       scroll, and when _00 elapses set BOSS_AUTO_SCROLL_COMPLETE so the boss
       routines (which gate on it) can start. Then recompute the scroll amount. */
    {
        bool force_frame_scroll = false;

        if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] != 0u)
        {
            ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] =
                (uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] - 1u);
            if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] != 0u)
            {
                force_frame_scroll = true;
            }
        }
        if (!force_frame_scroll && (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] != 0u))
        {
            ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] =
                (uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] - 1u);
            if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] == 0u)
            {
                ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] =
                    (uint8_t)(ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] + 1u);
            }
            force_frame_scroll = true;
        }
        if (force_frame_scroll)
        {
            ram[CONTRA_RAM_FRAME_SCROLL] = 0x01u;
        }
        scroll_pixels = (uint8_t)(ram[CONTRA_RAM_FRAME_SCROLL] + ram[CONTRA_RAM_TANK_AUTO_SCROLL]);
    }

    if (scroll_pixels == 0u)
    {
        return;
    }

    while (scroll_pixels-- != 0u)
    {
        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] =
            (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u);

        if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] == 0u)
        {
            ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 1u);
            ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
            ram[CONTRA_RAM_PPUCTRL_SETTINGS] ^= 0x01u;

            if (ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == ram[CONTRA_RAM_LEVEL_ALT_GRAPHICS_POS])
            {
                ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x01u;
                contra_load_alternate_graphics(core);
            }
        }

        if ((ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] & 0x07u) == 0u)
        {
            contra_schedule_horizontal_level_column_write(core);
            contra_advance_horizontal_level_ppu_column(core);
        }
        else if ((ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] & 0x0Fu) == 0x03u)
        {
            contra_schedule_horizontal_level_column_attributes_write(core);
        }

        ram[CONTRA_RAM_HORIZONTAL_SCROLL] =
            (uint8_t)(ram[CONTRA_RAM_HORIZONTAL_SCROLL] + 1u);
    }
}

static void contra_load_palette_indexes(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t cycle_index;

    if (ram[CONTRA_RAM_NUM_PALETTES_TO_LOAD] >= ram[CONTRA_RAM_GAME_ROUTINE_INDEX])
    {
        return;
    }

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) != 0x05u)
    {
        return;
    }

    if (ram[CONTRA_RAM_PAUSE_PALETTE_CYCLE] != 0u)
    {
        return;
    }

    ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE] = (uint8_t)(ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE] + 1u);
    cycle_index = ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE];
    if (cycle_index >= contra_level_palette_animation_count[ram[CONTRA_RAM_CURRENT_LEVEL]])
    {
        cycle_index = 0x00u;
        ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE] = cycle_index;
    }

    ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + 3u] = ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE_INDEXES + cycle_index];

    if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0u) ||
        (((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) == 0u) &&
         (ram[CONTRA_RAM_CURRENT_LEVEL] != 0x07u) &&
         ((ram[CONTRA_RAM_LEVEL_ALT_GRAPHICS_POS] & 0x80u) == 0u)))
    {
        ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + 2u] = contra_level_palette_2_index_tbl[cycle_index];
    }

    contra_load_palettes_color_to_cpu(core, 0x10u);
}

static void contra_load_bank_2_alternate_tile_loading(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t loading_flag = ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG];

    if (loading_flag == 0u)
    {
        return;
    }

    if ((loading_flag & 0x80u) == 0u)
    {
        const uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];
        const ContraAltGraphicDataRef *ref;

        if (level >= 8u)
        {
            ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x00u;
            return;
        }

        ref = &contra_alt_graphic_data_refs[level];
        if (ref->chunk_count == 0u)
        {
            ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x00u;
            return;
        }

        ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_LO] = (uint8_t)(ref->ppu_addr & 0xFFu);
        ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_HI] = (uint8_t)(ref->ppu_addr >> 8u);
        ram[CONTRA_RAM_ALT_GFX_READ_ADDR_LO] = (uint8_t)(ref->cpu_addr & 0xFFu);
        ram[CONTRA_RAM_ALT_GFX_READ_ADDR_HI] = (uint8_t)(ref->cpu_addr >> 8u);
        ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] = ref->chunk_count;
        ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x80u;
    }

    if (ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] == 0u)
    {
        ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x00u;
        return;
    }

    if (contra_load_rom_image())
    {
        const uint16_t ppu_addr = (uint16_t)(
            (uint16_t)ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_LO] |
            ((uint16_t)ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_HI] << 8u)
        );
        const uint16_t read_addr = (uint16_t)(
            (uint16_t)ram[CONTRA_RAM_ALT_GFX_READ_ADDR_LO] |
            ((uint16_t)ram[CONTRA_RAM_ALT_GFX_READ_ADDR_HI] << 8u)
        );
        unsigned chunk_offset;

        for (chunk_offset = 0u; chunk_offset < 0x20u; ++chunk_offset)
        {
            contra_write_ppu_byte(
                core,
                (uint16_t)(ppu_addr + chunk_offset),
                contra_rom_read_u8(2u, (uint16_t)(read_addr + chunk_offset))
            );
        }

        ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_LO] = (uint8_t)((ppu_addr + 0x20u) & 0xFFu);
        ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_HI] = (uint8_t)((ppu_addr + 0x20u) >> 8u);
        ram[CONTRA_RAM_ALT_GFX_READ_ADDR_LO] = (uint8_t)((read_addr + 0x20u) & 0xFFu);
        ram[CONTRA_RAM_ALT_GFX_READ_ADDR_HI] = (uint8_t)((read_addr + 0x20u) >> 8u);
    }

    ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] = (uint8_t)(ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] - 1u);
    if (ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] == 0u)
    {
        ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x00u;
    }
}

static void contra_load_palettes_color_to_cpu(ContraCore *core, uint8_t num_colors)
{
    uint8_t palette_index_offset = 0x00u;
    uint8_t write_offset = 0x00u;
    uint8_t palette_buffer[CONTRA_PPU_PALETTE_SIZE];

    core->ram[CONTRA_RAM_NUM_PALETTES_TO_LOAD] = num_colors;
    memcpy(palette_buffer, core->ppu_palette, sizeof(palette_buffer));

    while (write_offset < num_colors)
    {
        const uint8_t palette_index = core->ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + palette_index_offset];
        const uint16_t game_palette_addr = (uint16_t)(0xD227u + ((uint16_t)palette_index * 3u));
        uint8_t color_index;

        palette_buffer[write_offset++] = 0x0Fu;

        for (color_index = 0u; (color_index < 3u) && (write_offset < num_colors); ++color_index)
        {
            uint8_t color = contra_rom_read_u8(7u, (uint16_t)(game_palette_addr + color_index));

            if ((core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] != 0u) &&
                (core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] < 0x09u) &&
                (write_offset >= 4u) &&
                (write_offset < 16u))
            {
                static const uint8_t palette_shift_amounts[8] = {0x00u, 0x00u, 0x10u, 0x10u, 0x20u, 0x20u, 0x30u, 0x30u};
                const uint8_t shift_index = (uint8_t)(core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] - 1u);
                const uint8_t shift = palette_shift_amounts[shift_index];

                color = (color >= shift) ? (uint8_t)(color - shift) : 0x0Fu;
            }

            palette_buffer[write_offset++] = color;
        }

        /* load_palette_colors_to_cpu (bank7:3520) leaves the game_palettes
           pointer low byte in the $06 zero-page temp each slot. The leftover
           from the LAST slot is consumed as junk by create_default_soldiers'
           ledge-handling bit on the next frame, so the temp must be mirrored. */
        core->ram[0x06u] = (uint8_t)((uint8_t)(palette_index * 3u) + 0x27u);

        ++palette_index_offset;
    }

    if ((core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] != 0u) &&
        ((core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] & 0x80u) == 0u))
    {
        core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = (uint8_t)(core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] - 1u);
    }

    core->ram[CONTRA_RAM_NUM_PALETTES_TO_LOAD] = 0x00u;
    memcpy(core->pending_palette, palette_buffer, sizeof(core->pending_palette));
    core->pending_palette_count = num_colors;
    core->pending_palette_write = 0x01u;
}

static void contra_load_alternate_graphics(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];

    if (level >= 9u)
    {
        return; /* row 8 is the ending-animation palette set */
    }

    ram[CONTRA_RAM_LEVEL_ALT_GRAPHICS_POS] = 0xFFu;
    memcpy(
        &ram[CONTRA_RAM_COLLISION_CODE_1_TILE_INDEX],
        contra_level_alt_collision_and_palette_tbl[level],
        sizeof(contra_level_alt_collision_and_palette_tbl[0])
    );
    contra_load_palettes_color_to_cpu(core, 0x20u);
}

static void contra_load_bank_0_load_level_enemies_to_mem(ContraCore *core)
{
    core->ram[CONTRA_RAM_ENEMY_LEVEL_ROUTINES] = core->ram[CONTRA_RAM_CURRENT_LEVEL];
}

static void contra_load_next_supertiles_screen_indexes(ContraCore *core)
{
    contra_load_supertiles_screen_indexes(core, (uint8_t)(core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 1u));
}

static void contra_load_level_header(ContraCore *core)
{
    uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    if (level >= 8u)
    {
        memset(&core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE], 0, 0x20u);
        return;
    }

    memcpy(
        &core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE],
        contra_level_headers[level],
        0x20u
    );
}

static void contra_init_ppu_write_screen_supertiles(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
        ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
        ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] = 0x00u;
        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x1Du;
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0xA0u;
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x23u;
        contra_load_supertiles_screen_indexes(core, ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER]);
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x10u;
        contra_load_palettes_color_to_cpu(core, 0x10u);
        ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x20u;
    }
    else
    {
        ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x30u;
    }

    ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
    ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
    ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x20u;
    ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_LOW_BYTE] = 0xC0u;
    ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] = 0x23u;
    contra_load_supertiles_screen_indexes(core, ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER]);
}

static void contra_set_a_as_current_level_routine(ContraCore *core, uint8_t level_routine)
{
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = level_routine;
    core->ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] = 0x00u;
}

static void contra_set_graphics_zero_mode(ContraCore *core)
{
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = 0x00u;
    memset(&core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER], 0, CONTRA_CPU_GRAPHICS_BUFFER_SIZE);
    contra_init_apu_channels(core);
}

static void contra_clear_level_runtime_memory(ContraCore *core)
{
    size_t index;

    memset(&core->ram[0x40u], 0, 0xF0u - 0x40u);
    memset(&core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER], 0, CONTRA_RAM_CPU_GRAPHICS_BUFFER - CONTRA_RAM_CPU_SPRITE_BUFFER);
    core->level1_weapon_box_restore_timer = 0x00u;
    core->level1_weapon_box_restore_x = 0;
    core->level1_weapon_box_restore_y = 0;
    core->pending_horizontal_column_write = 0x00u;
    core->pending_horizontal_attr_write = 0x00u;
    core->l7_tile_update_count = 0x00u;
    core->l7_supertile_update_count = 0x00u;
    /* the ROM's BG_COLLISION_DATA ring is rebuilt from the new level's map;
       stale world-anchored overrides from the previous level would corrupt
       collision at the same world X here */
    core->l1_bridge_gap_count = 0x00u;
    for (index = 0u; index < CONTRA_NATIVE_MAX_ENEMIES; ++index)
    {
        core->l2_structure_tile[index] = 0x00u;
        core->l2_supertile[index] = 0xFFu;
        core->l1_supertile[index] = 0xFFu;
        if (index < CONTRA_ROM_ENEMY_SLOTS)
        {
            core->l1_supertile_world_x[index] = 0u;
            core->l1_supertile_screen_y[index] = 0u;
        }
    }
}

static bool contra_init_lvl_nametable_animation_elapsed(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] - 0x20u);
        if (ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] > 0xDFu)
        {
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] - 1u);
        }

        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] - 1u);
        if ((ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] & 0x80u) == 0u)
        {
            return false;
        }

        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
        ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
        ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] = 0x40u;
        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x1Du;
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0xA0u;
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x23u;
        contra_load_next_supertiles_screen_indexes(core);
        return true;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u)
    {
        contra_write_horizontal_level_column_to_ppu(core);
        contra_write_horizontal_level_column_attributes_to_ppu(core);
    }

    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] + 1u);
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] + 1u);
    ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = (uint8_t)(ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] - 1u);

    if (ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] == 0u)
    {
        return true;
    }

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) || (ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] < 0x20u))
    {
        return false;
    }

    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x24u;
    ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] = 0x27u;
    ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] = 0x40u;
    contra_load_next_supertiles_screen_indexes(core);
    return false;
}

static void contra_apply_controller_state(ContraCore *core)
{
    uint8_t first_read[2];
    uint8_t second_read[2];
    int player_index;

    first_read[0] = core->pending_input.player[0];
    first_read[1] = core->pending_input.player[1];
    second_read[0] = core->pending_input.player[0];
    second_read[1] = core->pending_input.player[1];

    for (player_index = 1; player_index >= 0; --player_index)
    {
        if (second_read[player_index] != first_read[player_index])
        {
            second_read[player_index] = core->ram[CONTRA_RAM_CTRL_KNOWN_GOOD + (size_t)player_index];
        }
    }

    if ((core->ram[CONTRA_RAM_PLAYER_MODE_1D] & 0x04u) == 0u)
    {
        second_read[0] = (uint8_t)(second_read[0] | second_read[1]);
    }

    for (player_index = 1; player_index >= 0; --player_index)
    {
        const size_t state_offset = CONTRA_RAM_CONTROLLER_STATE + (size_t)player_index;
        const size_t diff_offset = CONTRA_RAM_CONTROLLER_STATE_DIFF + (size_t)player_index;
        const size_t known_good_offset = CONTRA_RAM_CTRL_KNOWN_GOOD + (size_t)player_index;
        const uint8_t current = second_read[player_index];
        const uint8_t previous = core->ram[known_good_offset];

        core->ram[diff_offset] = (uint8_t)((current ^ previous) & current);
        core->ram[state_offset] = current;
        core->ram[known_good_offset] = current;
    }
}

static void contra_init_game_routine_flags(ContraCore *core)
{
    core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] = 0x00u;
    core->ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] = 0x00u;
}

static void contra_increment_game_routine(ContraCore *core)
{
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] + 1u);
    contra_init_game_routine_flags(core);
}

static void contra_inc_routine_index_set_timer(ContraCore *core)
{
    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x80u;
    contra_increment_game_routine(core);
}

static void contra_reset_delay_timer(ContraCore *core)
{
    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x40u;
    core->ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x02u;
}

static void contra_set_game_routine_index(ContraCore *core, uint8_t game_routine_index)
{
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = game_routine_index;
    contra_reset_delay_timer(core);
    contra_init_game_routine_flags(core);
}

static bool contra_decrement_delay_timer_elapsed(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if ((uint8_t)(ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] | ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE]) == 0u)
    {
        return true;
    }

    ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = (uint8_t)(ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] - 1u);
    if (ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] != 0u)
    {
        return false;
    }

    if (ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] == 0u)
    {
        return false;
    }

    ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = (uint8_t)(ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] - 1u);
    return false;
}

static void contra_dec_intro_theme_delay(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u)
    {
        return;
    }

    if (ram[CONTRA_RAM_INTRO_THEME_DELAY] == 0u)
    {
        return;
    }

    ram[CONTRA_RAM_INTRO_THEME_DELAY] = (uint8_t)(ram[CONTRA_RAM_INTRO_THEME_DELAY] - 1u);
}

static void contra_load_intro_palette2_play_intro_sound(ContraCore *core)
{
    core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] = 0x00u;
    core->ram[CONTRA_RAM_PPUCTRL_SETTINGS] = 0xB0u;
    core->ram[CONTRA_RAM_INTRO_THEME_DELAY] = 0xA4u;
    contra_load_bank_6_write_text_palette_to_mem(core, 0x04u);
    contra_play_sound(core, 0x26u);
}

static void contra_konami_input_check(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t index = ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT];
    uint8_t input;

    if ((index & 0x80u) != 0u)
    {
        return;
    }

    input = (uint8_t)(ram[CONTRA_RAM_CONTROLLER_STATE_DIFF] & 0xCFu);
    if (input == 0u)
    {
        return;
    }

    if (input != contra_konami_code_lookup_table[index])
    {
        ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] = 0xFFu;
        return;
    }

    index = (uint8_t)(index + 1u);
    ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] = index;
    if (index >= 10u)
    {
        ram[CONTRA_RAM_KONAMI_CODE_STATUS] = 0x01u;
    }
}

static void contra_clear_memory_3(ContraCore *core)
{
    memset(&core->ram[0x28u], 0, 0xF0u - 0x28u);
    memset(&core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER], 0, CONTRA_RAM_CPU_GRAPHICS_BUFFER - CONTRA_RAM_CPU_SPRITE_BUFFER);
}

static void contra_end_demo_level(ContraCore *core)
{
    core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] = 0x01u;
}

static void contra_simulate_demo_input(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    int player_index;

    if ((ram[CONTRA_RAM_CONTROLLER_STATE_DIFF] & (CONTRA_BUTTON_START | CONTRA_BUTTON_SELECT)) != 0u)
    {
        contra_end_demo_level(core);
        return;
    }

    if (ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] != 0xFFu)
    {
        ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] = (uint8_t)(ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] + 1u);
        if (ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] == 0x00u)
        {
            ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] = 0xFFu;
        }
    }

    for (player_index = 1; player_index >= 0; --player_index)
    {
        const size_t player_offset = (size_t)player_index;
        uint8_t input = ram[CONTRA_RAM_DEMO_INPUT_VAL + player_offset];

        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u)
        {
            if (ram[CONTRA_RAM_DEMO_INPUT_NUM_FRAMES + player_offset] == 0u)
            {
                uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];
                const uint16_t table_addr = contra_rom_read_u16(
                    5u,
                    (uint16_t)(contra_demo_input_pointer_table_addr + (((uint16_t)level * 2u + (uint16_t)player_offset) * 2u))
                );
                const uint8_t table_index = ram[CONTRA_RAM_DEMO_INPUT_TBL_INDEX + player_offset];

                input = contra_rom_read_u8(5u, (uint16_t)(table_addr + table_index));
                if (input == 0xFFu)
                {
                    contra_end_demo_level(core);
                    return;
                }

                ram[CONTRA_RAM_DEMO_INPUT_VAL + player_offset] = input;
                ram[CONTRA_RAM_DEMO_INPUT_NUM_FRAMES + player_offset] =
                    contra_rom_read_u8(5u, (uint16_t)(table_addr + table_index + 1u));
                ram[CONTRA_RAM_DEMO_INPUT_TBL_INDEX + player_offset] = (uint8_t)(table_index + 2u);
            }

            ram[CONTRA_RAM_DEMO_INPUT_NUM_FRAMES + player_offset] =
                (uint8_t)(ram[CONTRA_RAM_DEMO_INPUT_NUM_FRAMES + player_offset] - 1u);
            input = ram[CONTRA_RAM_DEMO_INPUT_VAL + player_offset];
        }

        ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_offset] = input;
        ram[CONTRA_RAM_CONTROLLER_STATE + player_offset] = input;

        if (ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] >= 0x50u)
        {
            const uint8_t weapon = (uint8_t)(ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_offset] & 0x0Fu);
            const bool auto_fire = (weapon == 0x01u) || (weapon == 0x04u);

            if (auto_fire)
            {
                ram[CONTRA_RAM_CONTROLLER_STATE + player_offset] |= CONTRA_BUTTON_B;
            }
            else if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) == 0u)
            {
                ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_offset] |= CONTRA_BUTTON_B;
            }
        }
    }
}

static void contra_set_next_demo_level(ContraCore *core)
{
    uint8_t demo_level;

    contra_clear_memory_3(core);
    core->ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x07u;
    core->ram[CONTRA_RAM_FRAME_COUNTER] = 0x00u;
    core->ram[CONTRA_RAM_RANDOM_NUM] = 0x00u;

    demo_level = core->ram[CONTRA_RAM_DEMO_LEVEL];
    if (demo_level >= 0x03u)
    {
        demo_level = 0x00u;
    }

    core->ram[CONTRA_RAM_DEMO_LEVEL] = demo_level;
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = demo_level;
    core->ram[CONTRA_RAM_DEMO_LEVEL] = (uint8_t)(demo_level + 1u);
    core->ram[CONTRA_RAM_P1_NUM_LIVES] = 0x62u;
    core->ram[CONTRA_RAM_P2_NUM_LIVES] = 0x62u;

    /* TEST HOOK (not part of the faithful port): CONTRA_FORCE_DEMO_LEVEL=<index>
       pins every attract demo to one level (0-based), so the chosen level's demo
       runs FIRST, clean from boot -- isolating it from upstream demo-level desync
       for the frame-exact comparison harness. */
    {
        const char *const force = getenv("CONTRA_FORCE_DEMO_LEVEL");

        if (force != NULL)
        {
            const int level = atoi(force);

            if ((level >= 0) && (level <= 2))
            {
                core->ram[CONTRA_RAM_CURRENT_LEVEL] = (uint8_t)level;
                core->ram[CONTRA_RAM_DEMO_LEVEL] = (uint8_t)level;
            }
        }
    }
}

static void contra_apply_start_player_env(ContraCore *core)
{
    const char *const lives_env = getenv("CONTRA_START_LIVES");
    const char *const weapon_env = getenv("CONTRA_START_WEAPON");
    uint8_t weapon;

    if (lives_env != NULL)
    {
        char *end = NULL;
        const long total_lives = strtol(lives_env, &end, 10);

        if ((end != lives_env) && (end != NULL) && (*end == '\0') &&
            (total_lives >= 1) && (total_lives <= 256))
        {
            /* P1_NUM_LIVES stores lives remaining after the current life.
               CONTRA_START_LIVES is user-facing total lives, so 30 -> 0x1d. */
            core->ram[CONTRA_RAM_P1_NUM_LIVES] = (uint8_t)(total_lives - 1);
        }
    }

    if (contra_parse_start_weapon(weapon_env, &weapon))
    {
        core->ram[CONTRA_RAM_P1_CURRENT_WEAPON] = weapon;
    }
}

static void contra_init_player_lives(ContraCore *core)
{
    uint8_t player_index;
    uint8_t initial_lives = 0x02u;

    player_index = core->ram[CONTRA_RAM_PLAYER_MODE] & 0x01u;
    core->ram[CONTRA_RAM_PLAYER_MODE_1D] = contra_player_mode_1d_table[player_index];
    core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = contra_p2_game_over_status_table[player_index];
    core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;

    if (core->ram[CONTRA_RAM_KONAMI_CODE_STATUS] != 0u)
    {
        initial_lives = 0x1Du;
    }

    do
    {
        core->ram[CONTRA_RAM_P1_NUM_LIVES + player_index] = initial_lives;
    } while (player_index-- != 0u);

    core->ram[CONTRA_RAM_EXTRA_LIFE_SCORE_LOW] = 0xC8u;
    core->ram[CONTRA_RAM_EXTRA_LIFE_SCORE_HIGH] = 0x00u;
    core->ram[CONTRA_RAM_EXTRA_LIFE_SCORE_HIGH + 1u] = 0xC8u;
}

static void contra_reset_players_score_and_lives(ContraCore *core)
{
    memset(&core->ram[CONTRA_RAM_PLAYER_1_SCORE_LOW], 0, 4u);
    core->ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
    contra_init_player_lives(core);
}

static void contra_init_score_player_lives(ContraCore *core)
{
    contra_clear_memory_3(core);
    core->ram[CONTRA_RAM_DEMO_LEVEL] = 0x00u;
    core->ram[CONTRA_RAM_NUM_CONTINUES] = 0x03u;

    memset(&core->ram[CONTRA_RAM_PLAYER_1_SCORE_LOW], 0, 4u);
    core->ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
    contra_init_player_lives(core);
    core->ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] = 0x00u;

    /* TEST HOOK (not part of the faithful port): CONTRA_START_LEVEL=<index> starts
       the game on a chosen level. Index is 0-based, so CONTRA_START_LEVEL=2 begins
       directly on Level 3. Applied once, here at game start, after clear_memory_3
       has zeroed CURRENT_LEVEL; level_routine_00 then loads that level's header. */
    {
        const char *const start_level = getenv("CONTRA_START_LEVEL");

        if (start_level != NULL)
        {
            const int level = atoi(start_level);

            if ((level >= 0) && (level <= 7))
            {
                core->ram[CONTRA_RAM_CURRENT_LEVEL] = (uint8_t)level;
            }
        }
    }
    contra_apply_start_player_env(core);
}

static void contra_game_routine_00(ContraCore *core)
{
    contra_zero_out_nametables(core);
    contra_load_intro_graphics(core);

    core->ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] = 0x00u;
    core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] = 0x01u;
    core->ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x02u;
    core->ram[CONTRA_RAM_PPUCTRL_SETTINGS] = 0xB1u;

    contra_inc_routine_index_set_timer(core);
}

static void contra_load_intro_title_sprites(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_SPRITE_X_POS + 0u] = 0x2Cu;
    ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u] = 0xAAu;
    ram[CONTRA_RAM_SPRITE_Y_POS + 0u] = contra_player_select_cursor_pos[ram[CONTRA_RAM_PLAYER_MODE] & 0x01u];
    ram[CONTRA_RAM_SPRITE_ATTR + 0u] = 0x00u;
    ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 1u] = 0xABu;
    ram[CONTRA_RAM_SPRITE_X_POS + 1u] = 0xB3u;
    ram[CONTRA_RAM_SPRITE_Y_POS + 1u] = 0x77u;
    ram[CONTRA_RAM_SPRITE_ATTR + 1u] = 0x00u;
}

static void contra_game_routine_01(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    contra_konami_input_check(core);

    if (ram[CONTRA_RAM_HORIZONTAL_SCROLL] != 0u)
    {
        ram[CONTRA_RAM_HORIZONTAL_SCROLL] = (uint8_t)(ram[CONTRA_RAM_HORIZONTAL_SCROLL] + 1u);
        if (ram[CONTRA_RAM_HORIZONTAL_SCROLL] != 0u)
        {
            return;
        }

        contra_load_intro_palette2_play_intro_sound(core);
    }

    contra_load_intro_title_sprites(core);

    if (contra_decrement_delay_timer_elapsed(core))
    {
        contra_increment_game_routine(core);
    }
}

static void contra_game_routine_02(ContraCore *core)
{
    if (core->ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] == 0u)
    {
        core->ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] = (uint8_t)(core->ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] + 1u);
        contra_set_next_demo_level(core);
        return;
    }

    if (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u)
    {
        contra_simulate_demo_input(core);
    }

    contra_run_level_routine(core);

    if (core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] != 0u)
    {
        core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
        contra_set_game_routine_index(core, 0x00u);
    }
}

static void contra_game_routine_03(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] == 0u)
    {
        ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
        ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x40u;
        ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] = (uint8_t)(ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] + 1u);
        return;
    }

    contra_dec_intro_theme_delay(core);

    {
        /* bank7.asm game_routine_03: `dec DELAY_TIME_LOW_BYTE` decrements memory
           while A keeps the PRE-decrement value, and the `ora INTRO_THEME_DELAY`
           elapsed test runs on that stale A -- the ROM advances on the frame
           AFTER the timer reaches 0, not the frame it reaches 0. */
        const uint8_t delay_before = ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE];

        if (delay_before != 0u)
        {
            ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = (uint8_t)(delay_before - 1u);
        }

        if ((uint8_t)(delay_before | ram[CONTRA_RAM_INTRO_THEME_DELAY]) == 0u)
        {
            contra_increment_game_routine(core);
            return;
        }
    }

    {
        uint8_t text_code = (uint8_t)(0x01u + (ram[CONTRA_RAM_PLAYER_MODE] & 0x01u));

        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x08u) != 0u)
        {
            text_code |= 0x80u;
        }

        contra_load_bank_6_write_text_palette_to_mem(core, text_code);
    }
}

static void contra_game_routine_04(ContraCore *core)
{
    contra_init_score_player_lives(core);
    contra_increment_game_routine(core);
}

static void contra_level_routine_00(ContraCore *core)
{
    contra_init_apu_channels(core);
    contra_zero_out_nametables(core);
    core->level1_weapon_box_restore_timer = 0x00u;
    core->level1_weapon_box_restore_x = 0;
    core->level1_weapon_box_restore_y = 0;
    core->pending_horizontal_column_write = 0x00u;
    core->pending_horizontal_attr_write = 0x00u;
    contra_load_bank_6_write_text_palette_to_mem(core, 0x06u);
    core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x00u;
    core->ram[CONTRA_RAM_LEVEL_END_PLAYERS_ALIVE] = 0x00u;
    contra_load_level_header(core);
    contra_init_ppu_write_screen_supertiles(core);
    contra_load_bank_0_load_level_enemies_to_mem(core);
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
    if (core->level_transition_pending != 0u)
    {
        /* mid-game level transition only (the boot path is already timed by
           startup_wait_frames): the ROM spends one extra video frame flushing
           this routine's PPU writes (reference frame 7449). */
        core->level_transition_pending = 0u;
        core->frame_stall_frames = 0x01u;
    }
}

static void contra_level_routine_01(ContraCore *core)
{
    uint8_t delay = core->ram[CONTRA_RAM_DEMO_MODE];

    if (delay == 0u)
    {
        contra_draw_stage_and_level_name(core);
        core->ram[CONTRA_RAM_DRAW_PLAYER_INDEX] = 0x00u;
        contra_draw_player_num_lives(core);

        if (core->ram[CONTRA_RAM_PLAYER_MODE] != 0u)
        {
            core->ram[CONTRA_RAM_DRAW_PLAYER_INDEX] = core->ram[CONTRA_RAM_PLAYER_MODE];
            contra_draw_player_num_lives(core);
        }

        delay = 0xC0u;
    }

    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = delay;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_level_routine_02(ContraCore *core)
{
    if (!contra_decrement_delay_timer_elapsed(core))
    {
        contra_draw_the_scores(core);
        return;
    }

    contra_zero_out_nametables(core);
    /* the ROM's level graphics load spans more video frames for the indoor
       base levels (reference recording: level 1 = 8 frozen frames at boot,
       level 2 = 10 at frames 7643-7652) */
    core->level_graphics_wait_frames =
        (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ? 0x0Au : 0x08u;
}

static void contra_finish_level_graphics_load(ContraCore *core)
{
    const uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    contra_load_level_graphics(core);
    contra_load_palettes_color_to_cpu(core, 0x20u);

    if (level < 8u)
    {
        core->ram[CONTRA_RAM_VERTICAL_SCROLL] = contra_level_vert_scroll_and_song[level][0];

        if (core->ram[CONTRA_RAM_DEMO_MODE] == 0u)
        {
            contra_play_sound(core, contra_level_vert_scroll_and_song[level][1]);
        }
    }

    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0xFFu;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_level_routine_03(ContraCore *core)
{
    if (!contra_init_lvl_nametable_animation_elapsed(core))
    {
        return;
    }

    core->ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0xFFu;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_set_frame_scroll_draw_player_bullets(ContraCore *core)
{
    uint8_t active_players = 0x00u;

    core->ram[CONTRA_RAM_FRAME_SCROLL] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 0u] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] = 0x00u;

    if (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0u)
    {
        active_players |= 0x01u;
    }

    if (core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS] == 0u)
    {
        active_players |= 0x02u;
    }

    if (active_players != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD] = (uint8_t)(active_players - 1u);
    }
    else
    {
        core->ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD] = 0x00u;
    }

    if (active_players == 0x01u)
    {
        contra_handle_invincibility_and_weapon_strength(core, 0u);
    }
    else if (active_players == 0x02u)
    {
        contra_handle_invincibility_and_weapon_strength(core, 1u);
    }
    else if (active_players == 0x03u)
    {
        contra_handle_invincibility_and_weapon_strength(core, 0u);
        contra_handle_invincibility_and_weapon_strength(core, 1u);
        contra_scroll_vertical_non_scrolling_player(core, active_players);
    }

    if (core->ram[CONTRA_RAM_INDOOR_SCROLL] >= 0x02u)
    {
        core->ram[CONTRA_RAM_INDOOR_SCROLL] = 0x00u;
    }

    /* bank7:3879-3884: the auto-scroll FRAME_SCROLL=1 is set AFTER the player
       routines (a latch this frame counts) and BEFORE the bullet routines, so
       in-flight bullets ride the scroll on the latch frame itself. */
    if ((uint8_t)(core->ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] | core->ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01]) != 0u)
    {
        core->ram[CONTRA_RAM_FRAME_SCROLL] = 0x01u;
    }

    contra_update_player_bullets(core);
    contra_draw_player_bullet_sprites(core);
}
