/* Gameplay loop, enemy execution, end-level/game-over flow, pause, and ending.
   Included by core.c; not compiled as a separate translation unit. */

static void contra_run_level_enemy_logic(ContraCore *core)
{
    contra_load_bank_3_handle_scroll(core);
    contra_rom_exe_all_enemy_routine(core);
    if ((core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) ||
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u))
    {
        contra_rom_load_indoor_enemy_data(core);
    }
    else
    {
        contra_rom_load_screen_enemy_data(core);
    }
    contra_rom_exe_soldier_generation(core);
    contra_load_palette_indexes(core);
    contra_load_bank_2_alternate_tile_loading(core);
}

static void contra_init_game_over(ContraCore *core)
{
    core->ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] = 0x60u;
    contra_set_a_as_current_level_routine(core, 0x0Au);
}

static void contra_set_to_level_routine_05(ContraCore *core)
{
    core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x00u;
    contra_set_a_as_current_level_routine(core, 0x05u);
}

static bool contra_check_game_over_run_enemy_logic(ContraCore *core)
{
    contra_set_frame_scroll_draw_player_bullets(core);
    if ((uint8_t)(core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS] & core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS]) != 0u)
    {
        contra_init_game_over(core);
        return true;
    }

    contra_run_level_enemy_logic(core);
    return false;
}

static void contra_end_level_set_delay_advance(ContraCore *core, uint8_t delay)
{
    core->ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] = delay;
    core->ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] =
        (uint8_t)(core->ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_make_off_screen_player_invisible(ContraCore *core, uint8_t player_index)
{
    const uint8_t sprite_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    const uint8_t sprite_x = core->ram[CONTRA_RAM_SPRITE_X_POS + player_index];

    if ((sprite_y >= 0x08u) && (sprite_x < 0xF8u) && (sprite_x >= 0x04u))
    {
        return;
    }

    core->ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] = 0xFFu;
    core->ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x00u;
    core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = 0x00u;
}

/* end_of_lvl_routine_lvl_3 (bank3:1439): two states -- walk to the dragon gate at
   screen center, then jump into it and vanish behind the wall. */
static void contra_run_level_3_end_level_player_routine(ContraCore *core, uint8_t player_index, uint8_t state_index)
{
    uint8_t *const ram = core->ram;

    if (state_index == 0u)
    {
        const uint8_t px = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
        const uint8_t dist = (px >= 0x80u) ? (uint8_t)(px - 0x80u) : (uint8_t)(0x80u - px);

        ram[CONTRA_RAM_CONTROLLER_STATE + player_index] =
            (px < 0x80u) ? CONTRA_BUTTON_RIGHT : CONTRA_BUTTON_LEFT;
        if (dist < 0x08u)
        {
            ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
        }
        return;
    }

    /* end_of_lvl_routine_lvl_3_01 (bank3:1466): jump into the gate. */
    ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_index] = CONTRA_BUTTON_A;
    if (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] == 0u)
    {
        return; /* the jump is just starting */
    }
    if ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) != 0u)
    {
        return; /* still rising */
    }
    if (ram[CONTRA_RAM_SPRITE_Y_POS + player_index] < 0xB0u)
    {
        return; /* not yet fallen to the top of the wall below the gate */
    }
    /* make_player_invisible (bank3:1550): disappear behind the wall. */
    ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] = 0xFFu;
    ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x00u;
}

static void contra_run_level_1_end_level_player_routine(ContraCore *core, uint8_t player_index, uint8_t state_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] =
        (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >= 0x98u) ? 0x80u : 0x00u;

    switch (state_index)
    {
        case 0x00u:
            ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = CONTRA_BUTTON_RIGHT;
            if (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >= 0x90u)
            {
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            }
            break;

        case 0x01u:
            if (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
            {
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            }
            else
            {
                ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = (uint8_t)(CONTRA_BUTTON_A | CONTRA_BUTTON_RIGHT);
                ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_index] = (uint8_t)(CONTRA_BUTTON_A | CONTRA_BUTTON_RIGHT);
            }
            break;

        default:
            ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = CONTRA_BUTTON_RIGHT;
            break;
    }
}

/* end_of_lvl_routine_indoor (bank3:1385): walk each player to their elevator
   (P1 left to x=0x0C, P2 right to x=0xF4), mount it (state 3 = can't move,
   sprite_91), wait for the other player, then ride up. */
static void contra_run_indoor_end_level_player_routine(ContraCore *core, uint8_t player_index, uint8_t state_index)
{
    static const uint8_t indoor_lvl_end_input_tbl[2] = {CONTRA_BUTTON_LEFT, CONTRA_BUTTON_RIGHT};
    static const uint8_t indoor_lvl_elevator_pos_tbl[2] = {0x0Cu, 0xF4u};
    static const uint8_t indoor_lvl_elevator_attr_tbl[2] = {0x00u, 0x45u};
    uint8_t *const ram = core->ram;

    switch (state_index)
    {
        case 0x00u:
        {
            const uint8_t dx = (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] -
                                         indoor_lvl_elevator_pos_tbl[player_index]);
            const uint8_t dist = ((dx & 0x80u) != 0u) ? (uint8_t)(0u - dx) : dx;

            ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = indoor_lvl_end_input_tbl[player_index];
            if (dist < 0x02u)
            {
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            }
            break;
        }

        case 0x01u:
            ram[CONTRA_RAM_PLAYER_STATE + player_index] = 0x03u; /* can't move */
            ram[CONTRA_RAM_SPRITE_ATTR + player_index] = indoor_lvl_elevator_attr_tbl[player_index];
            ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = 0x91u; /* on the elevator */
            ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            break;

        case 0x02u:
        {
            const uint8_t other_state =
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + (player_index ^ 1u)];

            if ((other_state == 0u) || (other_state >= 0x03u))
            {
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            }
            break;
        }

        default:
            ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - 1u); /* ride up */
            ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = 0x91u;
            break;
    }
}

/* end_of_lvl_routine_lvl_5/6/7 (bank3:1489-1524): hold right; past the
   per-level trigger X (x_pos_bg_priority_trigger_tbl: 0xB8 / 0xD0 / 0xD0) the
   player walks behind the background (bit 7) while bit 0 keeps the
   edge-detect walking off the ledge. The state never advances -- the walk
   ends when the player leaves the screen and is made invisible. */
static void contra_run_outdoor_walk_right_end_level_player_routine(
    ContraCore *core, uint8_t player_index, uint8_t trigger_x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = CONTRA_BUTTON_RIGHT;
    ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] =
        (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >= trigger_x) ? 0x81u : 0x01u;
}

/* end_of_lvl_routine_lvl_8 (bank3:1526): no walk -- the player stands while
   the boss explosions play out; state 0 just shortens the sequence-1 timer
   to 0x40 and parks in state 1. */
static void contra_run_level_8_end_level_player_routine(
    ContraCore *core, uint8_t player_index, uint8_t state_index)
{
    uint8_t *const ram = core->ram;

    if (state_index != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] = 0x40u;
    ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
        (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
}

static void contra_run_end_level_sequence(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_CONTROLLER_STATE + 0u] = 0x00u;
    ram[CONTRA_RAM_CONTROLLER_STATE + 1u] = 0x00u;
    ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + 0u] = 0x00u;
    ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + 1u] = 0x00u;

    switch (ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX])
    {
        case 0x00u:
        {
            uint8_t merged_jump_status = 0x00u;
            int player_index;

            for (player_index = 1; player_index >= 0; --player_index)
            {
                uint8_t state = 0x00u;

                if (ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] == 0u)
                {
                    merged_jump_status |= ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index];
                    state = 0x01u;
                }

                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] = state;
            }

            if (merged_jump_status != 0u)
            {
                return;
            }

            ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x81u;
            ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] = 0xF0u;
            contra_end_level_set_delay_advance(core, 0x20u);
            break;
        }

        case 0x01u:
            if (ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] != 0u)
            {
                ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] - 1u);
                return;
            }

            {
                int player_index;

                for (player_index = 1; player_index >= 0; --player_index)
                {
                    const uint8_t routine_state = ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index];

                    if (routine_state == 0u)
                    {
                        continue;
                    }

                    if (ram[CONTRA_RAM_CURRENT_LEVEL] == 0u)
                    {
                        contra_run_level_1_end_level_player_routine(core, (uint8_t)player_index, (uint8_t)(routine_state - 1u));
                    }
                    else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) ||
                             (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u))
                    {
                        contra_run_indoor_end_level_player_routine(core, (uint8_t)player_index, (uint8_t)(routine_state - 1u));
                    }
                    else if (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
                    {
                        contra_run_level_3_end_level_player_routine(core, (uint8_t)player_index, (uint8_t)(routine_state - 1u));
                    }
                    else if (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u)
                    {
                        contra_run_outdoor_walk_right_end_level_player_routine(core, (uint8_t)player_index, 0xB8u);
                    }
                    else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u) ||
                             (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u))
                    {
                        contra_run_outdoor_walk_right_end_level_player_routine(core, (uint8_t)player_index, 0xD0u);
                    }
                    else if (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
                    {
                        contra_run_level_8_end_level_player_routine(core, (uint8_t)player_index, (uint8_t)(routine_state - 1u));
                    }

                    contra_make_off_screen_player_invisible(core, (uint8_t)player_index);
                }
            }

            if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u)
            {
                ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] - 1u);
                if (ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] == 0u)
                {
                    contra_end_level_set_delay_advance(
                        core,
                        contra_level_end_level_delay_timer_tbl[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u]
                    );
                    return;
                }
            }

            if ((uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + 0u] |
                          ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + 1u]) == 0u)
            {
                contra_end_level_set_delay_advance(
                    core,
                    contra_level_end_level_delay_timer_tbl[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u]
                );
            }
            break;

        default:
            ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x02u;
            if (ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] != 0u)
            {
                ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] - 1u);
                if (ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] != 0u)
                {
                    return;
                }
            }

            contra_set_graphics_zero_mode(core);
            contra_set_a_as_current_level_routine(core, 0x05u);
            break;
    }
}

static void contra_show_game_over_screen(ContraCore *core)
{
    if (core->ram[CONTRA_RAM_DEMO_MODE] != 0u)
    {
        core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] = 0x01u;
        return;
    }

    contra_zero_out_nametables(core);
    contra_load_level_intro_screen_graphics(core);
    contra_load_bank_6_write_text_palette_to_mem(core, 0x06u);
    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Du);
    contra_play_sound(core, 0x4Eu);

    core->ram[CONTRA_RAM_NUM_CONTINUES] = (uint8_t)(core->ram[CONTRA_RAM_NUM_CONTINUES] - 1u);
    if ((core->ram[CONTRA_RAM_NUM_CONTINUES] & 0x80u) != 0u)
    {
        contra_set_a_as_current_level_routine(core, 0x07u);
        return;
    }

    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Eu);
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_check_for_pause(ContraCore *core)
{
    uint8_t input = core->ram[CONTRA_RAM_CONTROLLER_STATE_DIFF];

    if ((uint8_t)(core->ram[CONTRA_RAM_DEMO_MODE] | core->ram[0x26u] | core->ram[CONTRA_RAM_PPU_READY]) != 0u)
    {
        return;
    }

    if (core->ram[CONTRA_RAM_PAUSE_STATE] == 0u)
    {
        if ((input & CONTRA_BUTTON_START) == 0u)
        {
            return;
        }

        core->ram[CONTRA_RAM_PAUSE_STATE] = 0x01u;
        contra_play_sound(core, 0x54u);
        return;
    }

    contra_draw_player_bullet_sprites(core);
    contra_load_bank_2_set_players_paused_sprite_attr(core);

    if ((input & CONTRA_BUTTON_START) == 0u)
    {
        return;
    }

    core->ram[CONTRA_RAM_PAUSE_STATE] = 0x00u;
}

static void contra_level_routine_04(ContraCore *core)
{
    contra_check_for_pause(core);
    if (core->ram[CONTRA_RAM_PAUSE_STATE] != 0u)
    {
        return;
    }

    if (core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0u)
    {
        contra_set_a_as_current_level_routine(core, 0x08u);
        return;
    }

    (void)contra_check_game_over_run_enemy_logic(core);
}

static void contra_level_routine_05(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t p1_weapon = ram[CONTRA_RAM_P1_CURRENT_WEAPON];
    const uint8_t p2_weapon = ram[CONTRA_RAM_P2_CURRENT_WEAPON];

    ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0x00u;
    ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x00u;
    contra_clear_level_runtime_memory(core);

    if (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0u)
    {
        contra_show_game_over_screen(core);
        return;
    }

    ram[CONTRA_RAM_P1_CURRENT_WEAPON] = p1_weapon;
    ram[CONTRA_RAM_P2_CURRENT_WEAPON] = p2_weapon;
    ram[CONTRA_RAM_CURRENT_LEVEL] = (uint8_t)(ram[CONTRA_RAM_CURRENT_LEVEL] + 1u);
    if (ram[CONTRA_RAM_CURRENT_LEVEL] >= 0x08u)
    {
        /* level_routine_05 (bank7:3276): completed the last level -- start the
           game_routine_06 ending and reset the level routine so the post-
           credits handoff (game_end_routine_05 decrements the game routine)
           re-enters level_routine_00 on level 1. */
        contra_inc_routine_index_set_timer(core);
        ram[CONTRA_RAM_GAME_COMPLETION_COUNT] = (uint8_t)(ram[CONTRA_RAM_GAME_COMPLETION_COUNT] + 1u);
        ram[CONTRA_RAM_CURRENT_LEVEL] = 0x09u;
        ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
        ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] = 0x00u;
        ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0x00u;
        return;
    }

    contra_load_level_intro_screen_graphics(core);
    ram[CONTRA_RAM_VERTICAL_SCROLL] = 0x00u; /* intro screen scrolls from 0 */
    /* The ROM busy-writes the next stage's intro graphics across several
       video frames (FRAME_COUNTER frozen, LEVEL_ROUTINE_INDEX still 5) and
       flips the routine index to 0 only as the load finishes. Measured from
       the reference recording: entering an indoor base (levels 2/4) freezes
       5 frames and the level_routine_00 that follows costs one more flush
       frame (frames 7443-7449); entering an outdoor stage freezes 4 with no
       extra flush frame (frames 15509-15513). */
    if ((ram[CONTRA_RAM_CURRENT_LEVEL] & 0x01u) != 0u)
    {
        core->frame_stall_frames = 0x05u;
        core->level_transition_pending = 0x01u;
    }
    else
    {
        core->frame_stall_frames = 0x04u;
        core->level_transition_pending = 0x00u;
    }
    core->frame_stall_routine_reset = 0x01u;
    ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0x00u;
}

static void contra_level_routine_06(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t input_diff = ram[CONTRA_RAM_CONTROLLER_STATE_DIFF];

    if ((input_diff & CONTRA_BUTTON_START) != 0u)
    {
        contra_init_apu_channels(core);
        if (ram[CONTRA_RAM_CONT_END_SELECTION] != 0u)
        {
            contra_set_game_routine_index(core, 0x00u);
            return;
        }

        contra_reset_players_score_and_lives(core);
        ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
        ram[CONTRA_RAM_SPRITE_X_POS + 0u] = 0x00u;
        ram[CONTRA_RAM_SPRITE_Y_POS + 0u] = 0x00u;
        ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u] = 0x00u;
        return;
    }

    if ((input_diff & CONTRA_BUTTON_SELECT) != 0u)
    {
        ram[CONTRA_RAM_CONT_END_SELECTION] ^= 0x01u;
    }

    ram[CONTRA_RAM_SPRITE_X_POS + 0u] = 0x52u;
    ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u] = 0xAAu;
    ram[CONTRA_RAM_SPRITE_Y_POS + 0u] =
        contra_player_select_cursor_pos[ram[CONTRA_RAM_CONT_END_SELECTION] & 0x01u];
    contra_draw_the_scores(core);
}

static void contra_level_routine_07(ContraCore *core)
{
    if ((core->ram[CONTRA_RAM_CONTROLLER_STATE_DIFF] & CONTRA_BUTTON_START) != 0u)
    {
        contra_set_game_routine_index(core, 0x00u);
        return;
    }

    contra_draw_the_scores(core);
}

static void contra_level_routine_08(ContraCore *core)
{
    uint8_t alive_players = 0x00u;
    int player_index;

    if (contra_check_game_over_run_enemy_logic(core))
    {
        return;
    }

    if (core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] != 0u)
    {
        if ((uint8_t)(core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] + 1u) == 0u)
        {
            return;
        }

        if (!contra_decrement_delay_timer_elapsed(core))
        {
            return;
        }
    }

    for (player_index = 1; player_index >= 0; --player_index)
    {
        if ((core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] == 0u) &&
            (core->ram[CONTRA_RAM_PLAYER_STATE + player_index] == 0x01u))
        {
            ++alive_players;
        }
    }

    core->ram[CONTRA_RAM_LEVEL_END_PLAYERS_ALIVE] = alive_players;
    if (alive_players == 0u)
    {
        return;
    }

    contra_play_sound(core, 0x46u);
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_level_routine_09(ContraCore *core)
{
    contra_run_end_level_sequence(core);
    contra_set_frame_scroll_draw_player_bullets(core);
    contra_load_bank_3_handle_scroll(core);
    contra_rom_exe_all_enemy_routine(core);
    contra_load_palette_indexes(core);
}

static void contra_level_routine_0a(ContraCore *core)
{
    contra_run_level_enemy_logic(core);
    core->ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] = (uint8_t)(core->ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] - 1u);
    if (core->ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] != 0u)
    {
        return;
    }

    contra_set_to_level_routine_05(core);
}

static void contra_run_level_routine(ContraCore *core)
{
    switch (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX])
    {
        case 0x00u:
            contra_level_routine_00(core);
            break;
        case 0x01u:
            contra_level_routine_01(core);
            break;
        case 0x02u:
            contra_level_routine_02(core);
            break;
        case 0x03u:
            contra_level_routine_03(core);
            break;
        case 0x04u:
            contra_level_routine_04(core);
            break;
        case 0x05u:
            contra_level_routine_05(core);
            break;
        case 0x06u:
            contra_level_routine_06(core);
            break;
        case 0x07u:
            contra_level_routine_07(core);
            break;
        case 0x08u:
            contra_level_routine_08(core);
            break;
        case 0x09u:
            contra_level_routine_09(core);
            break;
        case 0x0Au:
            contra_level_routine_0a(core);
            break;
        default:
            break;
    }
}

static void contra_game_routine_05(ContraCore *core)
{
    contra_run_level_routine(core);
}

static void contra_stop_demo_load_player_select_ui(ContraCore *core)
{
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
    contra_zero_out_nametables(core);
    contra_load_intro_graphics(core);
    contra_load_intro_palette2_play_intro_sound(core);
    contra_set_game_routine_index(core, 0x01u);
}

static void contra_player_mode_change(ContraCore *core)
{
    uint8_t player_mode = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_MODE] + 1u);

    if ((uint8_t)(0x02u - player_mode) == 0u)
    {
        player_mode = 0x00u;
    }

    core->ram[CONTRA_RAM_PLAYER_MODE] = player_mode;
}

static void contra_dec_theme_delay_check_user_input(ContraCore *core)
{
    uint8_t input;

    contra_dec_intro_theme_delay(core);

    input = (uint8_t)(core->ram[CONTRA_RAM_CONTROLLER_STATE_DIFF] & 0x30u);
    if (input == 0u)
    {
        return;
    }

    contra_reset_delay_timer(core);

    if (core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] != 0x01u)
    {
        contra_stop_demo_load_player_select_ui(core);
        return;
    }

    if (core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] != 0u)
    {
        contra_load_intro_palette2_play_intro_sound(core);
        return;
    }

    if ((input & 0x20u) != 0u)
    {
        contra_player_mode_change(core);
        return;
    }

    contra_set_game_routine_index(core, 0x03u);
}

/* ===== game_routine_06: the ending (bank4:118-616) -- screen melt, the
   helicopter island escape, and the credits crawl, ending back at level 1
   with GAME_COMPLETION_COUNT bumped. ===== */

/* init_game_routine_reset_timer_low_byte (bank7:863): DELAY low = 0x80 and
   advance GAME_END_ROUTINE_INDEX (the GAME_ROUTINE_INIT_FLAG byte). */
static void contra_init_game_routine_reset_timer_low_byte(ContraCore *core)
{
    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x80u;
    core->ram[CONTRA_RAM_GAME_END_ROUTINE_INDEX] =
        (uint8_t)(core->ram[CONTRA_RAM_GAME_END_ROUTINE_INDEX] + 1u);
}

static void contra_game_end_routine_00(ContraCore *core)
{
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = 0x08u; /* the ending "level 9" */
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] =
        (uint8_t)(core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] - 1u);
    contra_init_game_routine_reset_timer_low_byte(core);
}

/* game_end_routine_01 (bank4:139): the screen melt -- each frame zero one
   0x20-byte vertical strip of the background pattern table ($1000-$1FFF);
   8 strips x 16 byte offsets dissolve every tile on screen. When done, load
   the ending palette row and the island scene (graphic list 0x0C). */
static void contra_game_end_routine_01(ContraCore *core)
{
    /* screen_melt_ppu_add_tbl (bank4:200): {PPU low, PPU high} */
    static const uint8_t screen_melt_ppu_add_tbl[8][2] = {
        {0x00u, 0x10u}, {0x00u, 0x14u}, {0x00u, 0x18u}, {0x00u, 0x1Cu},
        {0x10u, 0x10u}, {0x10u, 0x14u}, {0x10u, 0x18u}, {0x10u, 0x1Cu}};
    uint8_t *const ram = core->ram;
    const uint8_t strip = (uint8_t)(ram[0x40u] & 0x07u); /* zp $40 repurposed */
    const uint8_t byte_off = ram[0x41u];
    const uint16_t ppu_addr = (uint16_t)(
        ((uint16_t)screen_melt_ppu_add_tbl[strip][1] << 8u) |
        (uint8_t)(screen_melt_ppu_add_tbl[strip][0] + byte_off));
    unsigned k;

    for (k = 0u; k < 0x20u; ++k)
    {
        contra_write_ppu_byte(core, (uint16_t)(ppu_addr + (k * 0x20u)), 0x00u);
    }

    if ((uint8_t)(strip + 1u) != 0x08u)
    {
        ram[0x40u] = (uint8_t)(strip + 1u);
        return;
    }
    ram[0x41u] = (uint8_t)(byte_off + 1u);
    if (ram[0x41u] != 0x10u)
    {
        ram[0x40u] = 0x00u;
        return;
    }
    /* @load_alt_graphics (bank4:189) */
    ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x40u; /* dead store, ROM order */
    contra_init_game_routine_reset_timer_low_byte(core);
    contra_load_alternate_graphics(core);
    contra_load_graphic_data_list(core, 12u); /* ending_graphic_data */
}

/* end_scene_sprite_anim_tbl (bank4): {delay, x, y}; entry 0 -> slot 9
   (helicopter), entry 1 -> slot 8 (mountains), 2..9 -> slots 7..0
   (the island explosions). */
static const uint8_t contra_end_scene_sprite_anim_tbl[10][3] = {
    {0x00u, 0x80u, 0x90u}, {0x00u, 0x50u, 0x86u},
    {0xA8u, 0x60u, 0x8Cu}, {0xB4u, 0x98u, 0x8Au}, {0xB8u, 0x70u, 0x94u},
    {0xD0u, 0x50u, 0x96u}, {0xD3u, 0xA8u, 0x98u}, {0xD6u, 0x78u, 0x94u},
    {0xDBu, 0x68u, 0x96u}, {0xEFu, 0x88u, 0x94u}};

static void contra_end_game_sequence_00(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    int x;

    ram[CONTRA_RAM_ENEMY_SPRITES + 8u] = 0xCFu; /* 3 mountain peaks */
    ram[CONTRA_RAM_ENEMY_SPRITES + 9u] = 0xC5u; /* helicopter frame 1 */
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + 9u] = 0xFFu;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + 9u] = 0xFFu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + 9u] = 0x60u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + 9u] = 0x70u;
    for (x = 9; x >= 0; --x)
    {
        const uint8_t *const e = contra_end_scene_sprite_anim_tbl[9 - x];

        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = e[0];
        ram[CONTRA_RAM_ENEMY_X_POS + x] = e[1];
        ram[CONTRA_RAM_ENEMY_Y_POS + x] = e[2];
    }
    contra_play_sound(core, 0x21u); /* helicopter rotors */
    ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] =
        (uint8_t)(ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] + 1u);
}

/* destroyed_island_tile_tbl (bank4, CPU $ba67): a gameplay-format graphics
   stream -- [vram inc][group size][group count] then per group
   [PPU hi][PPU lo][bytes] -- applied directly to the PPU model. The last
   group recolors the island's attribute run at $23da. */
static void contra_apply_gameplay_graphics_stream(ContraCore *core, const uint8_t *s)
{
    const uint16_t stride = (s[0] == 0x02u) ? 0x20u : 0x01u;
    const uint8_t size = s[1];
    uint8_t groups = s[2];
    const uint8_t *p = s + 3;

    while (groups-- != 0u)
    {
        uint16_t addr = (uint16_t)(((uint16_t)p[0] << 8u) | p[1]);
        uint8_t i;

        p += 2;
        for (i = 0u; i < size; ++i)
        {
            contra_write_ppu_byte(core, addr, *p++);
            addr = (uint16_t)(addr + stride);
        }
    }
}

static void contra_end_game_draw_destroyed_island(ContraCore *core)
{
    static const uint8_t destroyed_island_tile_tbl[0x43] = {
        0x01u, 0x0Eu, 0x04u, 0x22u, 0x29u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x22u,
        0x49u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
        0x70u, 0x71u, 0x72u, 0x00u, 0x00u, 0x22u, 0x69u, 0x7Cu, 0x00u, 0x00u,
        0x70u, 0x00u, 0x7Cu, 0x74u, 0x7Eu, 0x74u, 0x80u, 0x81u, 0x74u, 0x73u,
        0x7Fu, 0x23u, 0xDAu, 0xAAu, 0xAAu, 0xAAu, 0xAAu, 0xAAu, 0xAAu, 0xAAu,
        0xAAu, 0xAAu, 0xAAu, 0xAAu, 0xAAu, 0xAAu, 0xAAu};

    core->ram[CONTRA_RAM_ENEMY_SPRITES + 8u] = 0x00u; /* mountains gone */
    contra_apply_gameplay_graphics_stream(core, destroyed_island_tile_tbl);
}

/* end_game_sequence_01 (bank4:260): fly the helicopter up-left (accelerating
   rightward until it exits off the top), and run the island explosion slots:
   each slot pops sound $25 as its delay crosses 0x80 (slot 3 also redraws the
   island destroyed), shows the explosion sprites through 0x7F..0x60, then
   blanks; slot 0's delay (the longest) reaching 0 advances the scene. */
static void contra_end_game_sequence_01(ContraCore *core)
{
    static const uint8_t helicopter_sprite_anim_tbl[32] = {
        0xC5u, 0xC6u, 0xC7u, 0xC5u, 0xC6u, 0xC7u, 0xC5u, 0xC6u,
        0xC7u, 0xC5u, 0xC8u, 0xC9u, 0xCAu, 0xCBu, 0xCCu, 0xCDu,
        0xCEu, 0xCCu, 0xCDu, 0xCEu, 0xCCu, 0xCDu, 0xCEu, 0xCCu,
        0xCDu, 0xCEu, 0xCCu, 0xCDu, 0xCEu, 0xCCu, 0xCDu, 0xCEu};
    static const uint8_t ending_sequence_explosion_tbl[4] = {0x37u, 0x36u, 0x35u, 0x37u};
    uint8_t *const ram = core->ram;
    int x;

    if (ram[CONTRA_RAM_ENEMY_SPRITES + 9u] != 0x01u)
    {
        unsigned sum;

        sum = (unsigned)ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + 9u] +
              ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + 9u];
        ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + 9u] = (uint8_t)sum;
        ram[CONTRA_RAM_ENEMY_X_POS + 9u] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + 9u] +
                      ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + 9u] + (sum >> 8u));
        sum = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + 9u] +
              ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + 9u];
        ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + 9u] = (uint8_t)sum;
        sum = (unsigned)ram[CONTRA_RAM_ENEMY_Y_POS + 9u] +
              ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + 9u] + (sum >> 8u);
        ram[CONTRA_RAM_ENEMY_Y_POS + 9u] = (uint8_t)sum;
        if (sum <= 0xFFu)
        {
            /* the Y add's missing carry: flew off the top -- hide it */
            ram[CONTRA_RAM_ENEMY_SPRITES + 9u] = 0x01u;
        }
        else
        {
            const unsigned anim = (unsigned)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + 9u] >> 2u);
            unsigned v;

            v = (unsigned)ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + 9u] + 2u;
            ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + 9u] = (uint8_t)v;
            ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + 9u] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + 9u] + (v >> 8u));
            ram[CONTRA_RAM_ENEMY_SPRITES + 9u] =
                helicopter_sprite_anim_tbl[(anim < 32u) ? anim : 31u];
            if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u)
            {
                return; /* odd frames skip the explosion slots */
            }
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + 9u] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + 9u] + 1u);
        }
    }

    for (x = 7; x >= 0; --x)
    {
        const uint8_t delay = ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x];

        if (delay == 0u)
        {
            if (x == 0)
            {
                ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] =
                    (uint8_t)(ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] + 1u);
                return;
            }
            /* ROM quirk: an elapsed non-zero slot still decrements (wrapping
               to 0xFF) and compares the slot NUMBER, landing on sprite 0 */
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0xFFu;
            ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u;
            continue;
        }
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = (uint8_t)(delay - 1u);
        if (delay >= 0x80u)
        {
            if (delay == 0x80u)
            {
                contra_play_sound(core, 0x25u); /* big island explosion */
                if (x == 3)
                {
                    contra_end_game_draw_destroyed_island(core);
                    continue; /* island draw skips the sprite write */
                }
            }
            ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u;
        }
        else if (delay >= 0x60u)
        {
            ram[CONTRA_RAM_ENEMY_SPRITES + x] =
                ending_sequence_explosion_tbl[(delay >> 3u) & 0x03u];
        }
        else
        {
            ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u;
        }
    }
}

/* end_game_sequence_02 (bank4:420): after the scene delay, clear the screen
   and start the credits music and crawl. */
static void contra_end_game_sequence_02(ContraCore *core)
{
    if (!contra_decrement_delay_timer_elapsed(core))
    {
        return;
    }
    core->ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] = 0x20u; /* zp $44: credits
        PPU write pointer high byte ($43 low is already 0) */
    contra_init_apu_channels(core);
    contra_reset_delay_timer(core);
    contra_play_sound(core, 0x4Au); /* end credits music */
    contra_zero_out_nametables(core);
    contra_init_game_routine_reset_timer_low_byte(core);
}

static void contra_game_end_routine_03(ContraCore *core)
{
    contra_load_palette_indexes(core);
    switch (core->ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX])
    {
        case 0x00u: contra_end_game_sequence_00(core); break;
        case 0x01u: contra_end_game_sequence_01(core); break;
        default: contra_end_game_sequence_02(core); break;
    }
}

/* Write one 0x20-tile credits row at the $43/$44 PPU pointer. A terminator
   entry (high byte 0) starts the 0x0300-frame post-credits delay instead. */
static void contra_game_end_draw_credits_line(ContraCore *core, uint8_t entry_index)
{
    const uint16_t ending_credits_ptr_tbl = 0xBB95u; /* bank 4 */
    const uint16_t line_addr = contra_rom_read_u16(
        4u, (uint16_t)(ending_credits_ptr_tbl + ((uint16_t)entry_index << 1u)));
    uint8_t *const ram = core->ram;
    const uint16_t ppu = (uint16_t)(((uint16_t)ram[0x44u] << 8u) | ram[0x43u]);
    uint8_t n_chars;
    uint8_t x_off;
    uint8_t i;

    if ((line_addr >> 8u) == 0u)
    {
        /* end_credits_text (bank4:531): ~12.8s then the final routine */
        ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x00u;
        ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x03u;
        contra_init_game_routine_reset_timer_low_byte(core);
        return;
    }
    n_chars = contra_rom_read_u8(4u, line_addr);
    x_off = contra_rom_read_u8(4u, (uint16_t)(line_addr + 1u));
    for (i = 0u; i < 0x20u; ++i)
    {
        uint8_t tile = 0x00u;

        if ((i >= x_off) && ((uint8_t)(i - x_off) < n_chars))
        {
            tile = contra_rom_read_u8(4u, (uint16_t)(line_addr + 2u + (uint16_t)(i - x_off)));
        }
        contra_write_ppu_byte(core, (uint16_t)(ppu + i), tile);
    }
    ram[0x43u] = (uint8_t)((ppu + 0x20u) & 0xFFu);
    ram[0x44u] = (uint8_t)((ppu + 0x20u) >> 8u);
}

/* game_end_routine_04 (bank4:435): the credits crawl -- scroll up one line
   every 4 frames; rows are written just below the visible window (a text line
   when the scroll's low nibble hits 4, a blank spacer at 0xC), wrapping the
   write pointer with the 240-line scroll reset. */
static void contra_game_end_routine_04(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t vs;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u) != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_VERTICAL_SCROLL] = (uint8_t)(ram[CONTRA_RAM_VERTICAL_SCROLL] + 1u);
    vs = ram[CONTRA_RAM_VERTICAL_SCROLL];
    if (vs == 0xF0u)
    {
        ram[0x44u] = 0x20u;
        ram[0x43u] = 0x00u;
        ram[CONTRA_RAM_VERTICAL_SCROLL] = 0x00u;
        vs = 0x00u;
    }
    if ((vs & 0x0Fu) == 0x04u)
    {
        const uint8_t entry = ram[0x42u]; /* zp $42: credits line index */

        ram[0x42u] = (uint8_t)(entry + 1u);
        contra_game_end_draw_credits_line(core, entry);
    }
    else if ((vs & 0x0Fu) == 0x0Cu)
    {
        contra_game_end_draw_credits_line(core, 0u); /* blank spacer row */
    }
}

static void contra_game_end_routine_05(ContraCore *core)
{
    if (!contra_decrement_delay_timer_elapsed(core))
    {
        return;
    }
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    /* back to game_routine_05: the second loop starts at level 1 */
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] =
        (uint8_t)(core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] - 1u);
}

static void contra_game_routine_06(ContraCore *core)
{
    switch (core->ram[CONTRA_RAM_GAME_END_ROUTINE_INDEX])
    {
        case 0x00u: contra_game_end_routine_00(core); break;
        case 0x01u: contra_game_end_routine_01(core); break;
        case 0x02u: contra_init_game_routine_reset_timer_low_byte(core); break;
        case 0x03u: contra_game_end_routine_03(core); break;
        case 0x04u: contra_game_end_routine_04(core); break;
        default: contra_game_end_routine_05(core); break;
    }
}

static void contra_exe_game_routine(ContraCore *core)
{
    uint8_t routine;

    /* Faithful (approximate) port of the ROM's RANDOM_NUM generator. Between
       NMIs the ROM spins in a busy loop (bank7.asm:284) repeatedly doing
       RANDOM_NUM += FRAME_COUNTER. The iteration count is CPU-timing dependent
       and cannot be reproduced exactly without cycle-accurate emulation, so we
       model a single iteration per frame using the pre-increment frame counter
       (the value the loop was adding during the gap before this NMI). This
       restores RNG variation — the port previously froze RANDOM_NUM at 0 — but
       does not reproduce the ROM's exact RNG sequence. */
    core->ram[CONTRA_RAM_RANDOM_NUM] =
        (uint8_t)(core->ram[CONTRA_RAM_RANDOM_NUM] + core->ram[CONTRA_RAM_FRAME_COUNTER]);

    core->ram[CONTRA_RAM_FRAME_COUNTER] = (uint8_t)(core->ram[CONTRA_RAM_FRAME_COUNTER] + 1u);

    routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    if ((routine != 0u) && (routine < 0x03u))
    {
        contra_dec_theme_delay_check_user_input(core);
        routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    }

    switch (routine)
    {
        case 0x00u:
            contra_game_routine_00(core);
            break;
        case 0x01u:
            contra_game_routine_01(core);
            break;
        case 0x02u:
            contra_game_routine_02(core);
            break;
        case 0x03u:
            contra_game_routine_03(core);
            break;
        case 0x04u:
            contra_game_routine_04(core);
            break;
        case 0x05u:
            contra_game_routine_05(core);
            break;
        case 0x06u:
            contra_game_routine_06(core);
            break;
        default:
            break;
    }
}
