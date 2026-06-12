/* Public ContraCore API functions and debug warp helpers.
   Included by core.c; not compiled as a separate translation unit. */


void contra_core_init(ContraCore *core)
{
    contra_core_reset(core);
}

void contra_core_reset(ContraCore *core)
{
    size_t index;

    memset(core, 0, sizeof(*core));
    core->startup_wait_frames = 0x0Au;

    for (index = 0u; index < 16u; ++index)
    {
        core->ram[0x7F0u + index] = (uint8_t)(0xF0u + index);
    }

    for (index = 0u; index < CONTRA_NATIVE_MAX_ENEMIES; ++index)
    {
        core->l2_supertile[index] = 0xFFu;
        core->l1_supertile[index] = 0xFFu;
        if (index < CONTRA_ROM_ENEMY_SLOTS)
        {
            core->l1_supertile_world_x[index] = 0u;
            core->l1_supertile_screen_y[index] = 0u;
        }
    }

    memset(core->ppu_palette, 0x0Fu, sizeof(core->ppu_palette));
    core->ram[CONTRA_RAM_HIGH_SCORE_LOW] = 0xC8u;
    core->ram[CONTRA_RAM_HIGH_SCORE_HIGH] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_MODE] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    core->ram[CONTRA_RAM_PPU_READY] = 0x05u;
    core->ram[CONTRA_RAM_PPUCTRL_SETTINGS] = 0xB0u;
    core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER] = 0x00u;

    contra_render_frame(core, true);
}

void contra_core_set_input(ContraCore *core, const ContraInputSnapshot *input)
{
    if (input == NULL)
    {
        memset(&core->pending_input, 0, sizeof(core->pending_input));
        return;
    }

    core->pending_input = *input;
}

void contra_core_step_frame(ContraCore *core)
{
    if (core->startup_wait_frames != 0u)
    {
        const bool startup_state =
            (core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0u) &&
            (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0u) &&
            (core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] == 0u);

        if (startup_state)
        {
            const uint8_t startup_frame = (uint8_t)(0x0Bu - core->startup_wait_frames);

            if (startup_frame >= 0x04u)
            {
                core->ram[CONTRA_RAM_FRAME_COUNTER] = 0x01u;
            }

            if (startup_frame >= 0x05u)
            {
                core->ram[CONTRA_RAM_DEMO_MODE] = 0x01u;
            }

            if (startup_frame == 0x0Au)
            {
                core->startup_wait_frames = 0u;
                contra_game_routine_00(core);
                contra_flush_cpu_graphics_buffer_to_ppu(core);
                contra_render_frame(core, true);
                return;
            }

            core->startup_wait_frames = (uint8_t)(core->startup_wait_frames - 1u);
            contra_render_frame(core, true);
            return;
        }

        core->startup_wait_frames = 0u;
    }

    if (core->level_graphics_wait_frames != 0u)
    {
        bool update_latches = false;

        core->level_graphics_wait_frames = (uint8_t)(core->level_graphics_wait_frames - 1u);
        if (core->level_graphics_wait_frames == 0u)
        {
            contra_finish_level_graphics_load(core);
            contra_flush_cpu_graphics_buffer_to_ppu(core);
            update_latches = true;
        }
        contra_render_frame(core, update_latches);
        return;
    }

    if (core->frame_stall_frames != 0u)
    {
        core->frame_stall_frames = (uint8_t)(core->frame_stall_frames - 1u);
        if ((core->frame_stall_frames == 0u) && (core->frame_stall_routine_reset != 0u))
        {
            core->frame_stall_routine_reset = 0u;
            core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
        }
        contra_render_frame(core, true);
        return;
    }

    /* the NMI flushed CPU_GRAPHICS_BUFFER at the end of the previous frame:
       this frame's writers (fence animation, enemy super-tile stamps) start
       from an empty buffer. Only the BUDGET is modeled (ram[$21]); the actual
       pixels go straight to the native renderer. */
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = 0x00u;

    contra_flush_pending_horizontal_level_writes(core);
    contra_process_level_1_weapon_box_restore(core);
    contra_apply_controller_state(core);
    contra_exe_game_routine(core);
    contra_flush_cpu_graphics_buffer_to_ppu(core);
    contra_render_frame(core, core->level_graphics_wait_frames == 0u);
}

/* DEBUG (NOT part of the faithful port): warp an initialized core straight to the
   Level 2 boss room so the front-end can launch "just the L2 boss fight". Sets up a
   single-player L2 game, runs it to gameplay, then replays the room-advance loop --
   shoot down each room's wall cores (enemy type 0x14) by injecting a player bullet at
   them, then hold Up to walk into the next room -- until LEVEL_LOCATION_TYPE bit 7
   (boss room) is set. Mirrors the reach_level2_boss_room behavior test. */
void contra_core_debug_warp_level2_boss(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    ContraInputSnapshot none = {{0u, 0u}};
    ContraInputSnapshot up = {{0u, 0u}};
    unsigned room;
    unsigned frame;

    up.player[0] = CONTRA_BUTTON_UP;

    /* force a single-player Level 2 game and run it to gameplay (level_routine 4) */
    ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_CURRENT_LEVEL] = 0x01u;
    ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;
    ram[CONTRA_RAM_P2_NUM_LIVES] = 0x00u;
    contra_apply_start_player_env(core);
    for (frame = 0u; frame < 600u; ++frame)
    {
        contra_core_set_input(core, &none);
        contra_core_step_frame(core);
        if ((ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
            (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
        {
            break;
        }
    }

    for (room = 0u; room < 24u; ++room)
    {
        const uint8_t initial_screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
        size_t i;
        int found;

        if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u)
        {
            break; /* boss room reached */
        }

        /* settle until grounded after the previous room transition */
        ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
        for (frame = 0u; frame < 180u; ++frame)
        {
            contra_core_set_input(core, &none);
            contra_core_step_frame(core);
            if ((ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0u) &&
                (ram[CONTRA_RAM_EDGE_FALL_CODE] == 0u))
            {
                break;
            }
        }

        /* shoot down every wall core in this room, then wait for the room to clear */
        for (frame = 0u; frame < 600u; ++frame)
        {
            found = 0;
            for (i = 0u; i < 16u; ++i)
            {
                if ((ram[CONTRA_RAM_ENEMY_ROUTINE + i] != 0u) &&
                    (ram[CONTRA_RAM_ENEMY_TYPE + i] == 0x14u))
                {
                    ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE] = 0x01u;
                    ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE] = 0x01u;
                    ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = ram[CONTRA_RAM_ENEMY_X_POS + i];
                    ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = ram[CONTRA_RAM_ENEMY_Y_POS + i];
                    found = 1;
                }
            }
            ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
            contra_core_set_input(core, &none);
            contra_core_step_frame(core);
            if ((found == 0) && (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] != 0u))
            {
                break;
            }
        }

        /* hold Up to walk into the next room */
        for (frame = 0u; frame < 420u; ++frame)
        {
            ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
            contra_core_set_input(core, &up);
            contra_core_step_frame(core);
            if ((ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen) ||
                ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u))
            {
                break;
            }
        }
    }

    /* leave the player alive and not mid-jump for the handover to live input */
    ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0x00u;
    contra_apply_start_player_env(core);
}

void contra_core_debug_warp_level4(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    ContraInputSnapshot none = {{0u, 0u}};
    unsigned frame;

    ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_CURRENT_LEVEL] = 0x03u;
    ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;
    ram[CONTRA_RAM_P2_NUM_LIVES] = 0x00u;
    contra_apply_start_player_env(core);

    for (frame = 0u; frame < 600u; ++frame)
    {
        contra_core_set_input(core, &none);
        contra_core_step_frame(core);
        if ((ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
            (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
        {
            break;
        }
    }
    contra_apply_start_player_env(core);
}

void contra_core_debug_warp_level5(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    ContraInputSnapshot none = {{0u, 0u}};
    unsigned frame;

    ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_CURRENT_LEVEL] = 0x04u; /* Level 5 (snow field) */
    ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;
    ram[CONTRA_RAM_P2_NUM_LIVES] = 0x00u;
    contra_apply_start_player_env(core);

    for (frame = 0u; frame < 600u; ++frame)
    {
        contra_core_set_input(core, &none);
        contra_core_step_frame(core);
        if ((ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
            (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
        {
            break;
        }
    }
    contra_apply_start_player_env(core);
}

void contra_core_debug_warp_level7(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    ContraInputSnapshot none = {{0u, 0u}};
    unsigned frame;

    ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_CURRENT_LEVEL] = 0x06u; /* Level 7 (hangar) */
    ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;
    ram[CONTRA_RAM_P2_NUM_LIVES] = 0x00u;
    contra_apply_start_player_env(core);

    for (frame = 0u; frame < 600u; ++frame)
    {
        contra_core_set_input(core, &none);
        contra_core_step_frame(core);
        if ((ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
            (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
        {
            break;
        }
    }
    contra_apply_start_player_env(core);
}

const uint32_t *contra_core_framebuffer(const ContraCore *core)
{
    return core->framebuffer;
}

const uint8_t *contra_core_ram(const ContraCore *core)
{
    return core->ram;
}
