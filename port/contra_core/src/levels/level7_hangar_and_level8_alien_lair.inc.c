/* Level 7 hangar mechanisms/boss and Level 8 alien lair enemies.
   Included by core.c; not compiled as a separate translation unit. */

/* ===== level-7 hangar and level-8 alien enemies (bank0.asm:8027-10385) ===== */

/* Total horizontal scroll of the level in pixels. The L5-L8 overlay caches
   anchor entries to the WORLD: recorded tiles then scroll with the background,
   and a later update to the same world tile (the claw erasing its chain, a
   spiked wall's destruction rubble) replaces the original entry even after the
   screen has scrolled between the two stamps. Recording bare screen positions
   (as before) left every stamp glued to the screen -- the rubble/beam/chain
   tiles smeared sideways as the player walked. */
static int contra_level7_world_scroll_x(const ContraCore *core)
{
    return ((int)core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8) |
        (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
}

static int contra_world_tile_aligned_x(const ContraCore *core, int x)
{
    /* align on the world tile grid, anchored to the world */
    return (x + contra_level7_world_scroll_x(core)) & ~7;
}

static int contra_screen_tile_aligned_y(int y)
{
    return y & ~7;
}

static void contra_level7_record_tile_update(ContraCore *core, int x, int y, uint8_t tile_index)
{
    const int aligned_x = contra_world_tile_aligned_x(core, x);
    const int aligned_y = contra_screen_tile_aligned_y(y);
    uint8_t i;

    for (i = 0u; i < core->l7_tile_update_count; ++i)
    {
        if ((core->l7_tile_update_x[i] == aligned_x) && (core->l7_tile_update_y[i] == aligned_y))
        {
            core->l7_tile_update_index[i] = tile_index;
            return;
        }
    }

    if (core->l7_tile_update_count < CONTRA_LEVEL7_TILE_UPDATE_CACHE)
    {
        i = core->l7_tile_update_count++;
        core->l7_tile_update_x[i] = (int16_t)aligned_x;
        core->l7_tile_update_y[i] = (int16_t)aligned_y;
        core->l7_tile_update_index[i] = tile_index;
    }
}

static void contra_level7_record_supertile_update(ContraCore *core, int x, int y, uint8_t supertile_index)
{
    const int aligned_x = contra_world_tile_aligned_x(core, x);
    const int aligned_y = contra_screen_tile_aligned_y(y);
    uint8_t i;

    for (i = 0u; i < core->l7_supertile_update_count; ++i)
    {
        if ((core->l7_supertile_update_x[i] == aligned_x) && (core->l7_supertile_update_y[i] == aligned_y))
        {
            core->l7_supertile_update_index[i] = supertile_index;
            return;
        }
    }

    if (core->l7_supertile_update_count < CONTRA_LEVEL7_SUPERTILE_UPDATE_CACHE)
    {
        i = core->l7_supertile_update_count++;
        core->l7_supertile_update_x[i] = (int16_t)aligned_x;
        core->l7_supertile_update_y[i] = (int16_t)aligned_y;
        core->l7_supertile_update_index[i] = supertile_index;
    }
}

static const uint8_t contra_claw_frame_trigger_tbl[4] = {0x00u, 0x20u, 0x40u, 0x60u};
static const uint8_t contra_claw_length_tbl[4] = {0x04u, 0x03u, 0x08u, 0x03u};
/* claw_update_nametable_ptr_tbl -> claw_tile_code_* (bank0:8190-8223). The ROM's
   code tables are only 4 bytes each but animate_claw indexes them by ENEMY_VAR_3
   (0..7 for the length-8 claw), deliberately reading into the FOLLOWING table's
   bytes. Memory order is code_00{86 86 86 86} code_01{80 80 82 84}
   code_02{86 86 86 86} unused_00{86 86 86 86} code_03{80 80 80 80}
   unused_01{80 80 82 84}, so each row below is the 8-byte window the ROM reads.
   Filling columns 4-7 with 0x00 (as before) drew garbage in the lower half of
   the long grappler -- visible as wrong tiles when it retracts (ascent reads
   VAR_3 7..0). VAR_4 selects ascent (even -> 0x86 restore) or descent rows. */
static const uint8_t contra_claw_tile_code_tbl[8][8] = {
    {0x86u, 0x86u, 0x86u, 0x86u, 0x80u, 0x80u, 0x82u, 0x84u}, /* 0: code_00 -> code_01 */
    {0x80u, 0x80u, 0x82u, 0x84u, 0x86u, 0x86u, 0x86u, 0x86u}, /* 1: code_01 -> code_02 */
    {0x86u, 0x86u, 0x86u, 0x86u, 0x80u, 0x80u, 0x82u, 0x84u}, /* 2: code_00 -> code_01 */
    {0x80u, 0x80u, 0x82u, 0x84u, 0x86u, 0x86u, 0x86u, 0x86u}, /* 3: code_01 -> code_02 */
    {0x86u, 0x86u, 0x86u, 0x86u, 0x86u, 0x86u, 0x86u, 0x86u}, /* 4: code_02 -> unused_00 */
    {0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x82u, 0x84u}, /* 5: code_03 -> unused_01 */
    {0x86u, 0x86u, 0x86u, 0x86u, 0x80u, 0x80u, 0x82u, 0x84u}, /* 6: code_00 -> code_01 */
    {0x80u, 0x80u, 0x82u, 0x84u, 0x86u, 0x86u, 0x86u, 0x86u}, /* 7: code_01 -> code_02 */
};

static bool contra_rom_animate_claw(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return false;
    }

    contra_level7_record_tile_update(
        core,
        ram[CONTRA_RAM_ENEMY_X_POS + x],
        ram[CONTRA_RAM_ENEMY_Y_POS + x],
        contra_claw_tile_code_tbl[ram[CONTRA_RAM_ENEMY_VAR_4 + x] & 0x07u]
            [ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x07u]);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x2Cu) ? 0x00u : 0x02u;
    return true;
}


/* claw_routine_00 (bank0:8034-8054): split ENEMY_ATTRIBUTES into frame trigger
   bits and length bits, then set the initial delay and advance. */
static void contra_rom_claw_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_FRAME + x] = contra_claw_frame_trigger_tbl[(attr >> 2u) & 0x03u];
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = (uint8_t)(attr & 0x03u);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x20u);
}

/* claw_routine_01 (bank0:8059-8099): wait for the timed or seeking descent trigger. */
static void contra_rom_claw_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x03u;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u) { return; }
    if (attr == 0x03u)
    {
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
            return;
        }
        if (ram[CONTRA_RAM_FRAME_COUNTER] >= 0xC0u) { return; }
        if (contra_rom_player_enemy_x_dist(core, x) >= 0x10u) { return; }
    }
    else if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x7Fu) != ram[CONTRA_RAM_ENEMY_FRAME + x])
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x2Cu) { return; }
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(attr << 1u);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = contra_claw_length_tbl[attr];
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x00u);
}

/* claw_routine_02/03 (bank0:8106-8138): descend/ascent state plus animate_claw
   nametable writes through level_7_tile_animation. */
static void contra_rom_claw_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (!contra_rom_animate_claw(core, x)) { return; }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 1u);
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u);
        return;
    }
    contra_rom_add_a_to_enemy_y_pos(core, x, 0x08u);
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 1u);
}

static void contra_rom_claw_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (!contra_rom_animate_claw(core, x)) { return; }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = ram[CONTRA_RAM_ENEMY_FRAME + x];
        contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
        return;
    }
    contra_rom_add_a_to_enemy_y_pos(core, x, 0xF8u);
}

/* rising_spiked_wall_routine_00/01 (bank0:8222-8271): init distance/delay and wait
   until a player is close enough to raise the wall. */
static const uint8_t contra_rising_spiked_wall_trigger_tbl[4][2] = {
    {0x30u, 0x00u}, {0x50u, 0x0Fu}, {0x70u, 0x1Eu}, {0x40u, 0x00u}};
static const uint8_t contra_rising_spiked_wall_delay_tbl[4] = {0x0Cu, 0x08u, 0x04u, 0x02u};
static const uint8_t contra_rising_spiked_wall_data_tbl[7][3] = {
    {0xC0u, 0x91u, 0xD0u},
    {0xD0u, 0x91u, 0xE0u},
    {0xE0u, 0x90u, 0xF0u},
    {0xE0u, 0x8Fu, 0xF8u},
    {0xF0u, 0x8Eu, 0x00u},
    {0xF0u, 0x8Du, 0x09u},
    {0xF0u, 0x8Cu, 0x09u},
};
static const uint8_t contra_spiked_wall_destroyed_update_tbl[7][3] = {
    {0x00u, 0x84u, 0x08u},
    {0xE0u, 0x8Bu, 0xF0u},
    {0xC0u, 0x8Au, 0xD0u},
    {0xA0u, 0x86u, 0xB0u},
    {0x00u, 0x84u, 0x08u},
    {0xE0u, 0x8Bu, 0xF0u},
    {0xC0u, 0x86u, 0xD0u},
};

static void contra_rom_rising_spiked_wall_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    const uint8_t trig = (attr >> 2u) & 0x03u;

    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = contra_rising_spiked_wall_trigger_tbl[trig][0];
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = contra_rising_spiked_wall_trigger_tbl[trig][1];
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = contra_rising_spiked_wall_delay_tbl[attr & 0x03u];
    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = 0xC0u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_rising_spiked_wall_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (contra_rom_player_enemy_x_dist(core, x) >= ram[CONTRA_RAM_ENEMY_VAR_3 + x]) { return; }
    contra_rom_enable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x06u;
    contra_rom_set_enemy_delay_adv_routine(core, x, ram[CONTRA_RAM_ENEMY_VAR_4 + x]);
}

/* rising_spiked_wall_routine_02/05 and spiked_wall_routine_00/02
   (bank0:8273-8385, 8397-8414): RAM/timing and level-7 supertile writes. */
static void contra_rom_rising_spiked_wall_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t idx = ram[CONTRA_RAM_ENEMY_VAR_2 + x];

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }
    if (idx < (sizeof(contra_rising_spiked_wall_data_tbl) / sizeof(contra_rising_spiked_wall_data_tbl[0])))
    {
        const uint8_t *const row = contra_rising_spiked_wall_data_tbl[idx];

        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = row[2];
        contra_level7_record_supertile_update(
            core,
            (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 0x0Du),
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + row[0]),
            row[1]);
        if (idx < 0x04u)
        {
            /* bank0:8311-8315: once the wall is far enough out of the ground
               (table entries 0-3) the stamped cell turns solid -- left half
               empty ($00), right half solid ($0f); the wall column occupies
               the right 16px of its super-tile. */
            contra_rom_set_supertile_bg_collisions(
                core,
                (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 0x0Du),
                (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + row[0]),
                0x00u, 0x0Fu);
        }
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x];
    if ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x80u) != 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
    }
}

static void contra_rom_spiked_wall_routine_00(ContraCore *core, uint8_t x)
{
    static const uint8_t destroyed_data[2][2] = {{0x04u, 0x03u}, {0x00u, 0x04u}};
    uint8_t *const ram = core->ram;
    const uint8_t idx = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u;

    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0xB8u;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = destroyed_data[idx][0];
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = destroyed_data[idx][1];
    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = 0xC0u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_spiked_wall_destroyed_routine(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_play_sound(core, 0x24u);
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_spiked_wall_destroy_anim(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t idx = ram[CONTRA_RAM_ENEMY_VAR_4 + x];

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (idx >= (sizeof(contra_spiked_wall_destroyed_update_tbl) / sizeof(contra_spiked_wall_destroyed_update_tbl[0])))
    {
        contra_rom_clear_enemy(core, x);
        return;
    }
    contra_level7_record_supertile_update(
        core,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 0x0Du),
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + contra_spiked_wall_destroyed_update_tbl[idx][0]),
        contra_spiked_wall_destroyed_update_tbl[idx][1]);
    if ((idx & 0x03u) != 0u)
    {
        /* bank0:8374-8377: every non-floor cell of the destroyed wall becomes
           passable (clear_supertile_bg_collision); the floor cell (entry 0/4)
           keeps the map's collision. Without this the standing spiked wall
           (solid in the original level map) still blocked the player after
           it was shot to rubble. */
        contra_rom_set_supertile_bg_collisions(
            core,
            (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 0x0Du),
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + contra_spiked_wall_destroyed_update_tbl[idx][0]),
            0x00u, 0x00u);
    }
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    }
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_clear_enemy(core, x);
    }
}

/* mine_cart_generator / moving_cart / immobile_cart routines
   (bank0:8420-8502, 8523-8594): generate carts, animate wheels, reverse or explode
   on collision, and start immobile carts when ENEMY_FRAME is set by land-on-enemy. */
static void contra_rom_init_cart_vel_and_y_pos(ContraCore *core, uint8_t x, uint8_t fract)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = fract;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x2Au;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0xC8u;
}

static void contra_rom_mine_cart_generator_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x80u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
}

static void contra_rom_mine_cart_generator_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    int slot;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_FRAME + x] & 0x80u) == 0u)
    {
        const uint8_t cart = ram[CONTRA_RAM_ENEMY_FRAME + x];
        if (ram[CONTRA_RAM_ENEMY_ROUTINE + cart] == 0u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x80u;
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x80u;
        }
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u) { return; }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
    slot = contra_rom_find_next_enemy_slot(core);
    if (slot < 0) { return; }
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x14u;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    ram[CONTRA_RAM_ENEMY_X_POS + slot] = 0xF8u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + slot] = 0xFFu;
    ram[CONTRA_RAM_ENEMY_VAR_4 + slot] = 0x02u;
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = 0x80u;
    contra_rom_init_cart_vel_and_y_pos(core, (uint8_t)slot, ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + slot]);
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)slot;
}

static void contra_rom_moving_cart_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(0x2Au + ((ram[CONTRA_RAM_FRAME_COUNTER] >> 2u) & 0x01u));
    contra_rom_update_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x10u) || (ram[CONTRA_RAM_ENEMY_X_POS + x] > 0xF0u))
    {
        if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x80u) != 0u)
        {
            contra_rom_set_enemy_routine_to_a(core, x, 0x04u);
        }
        else
        {
            ram[CONTRA_RAM_ENEMY_VAR_4 + x] ^= 0x02u;
            ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] =
                (uint8_t)(0u - ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x]);
            ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] =
                (uint8_t)(0u - ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x]);
        }
    }
}

static void contra_rom_immobile_cart_generator_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_init_cart_vel_and_y_pos(core, x, 0xC0u);
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_immobile_cart_generator_routine_01(ContraCore *core, uint8_t x)
{
    if (core->ram[CONTRA_RAM_ENEMY_FRAME + x] != 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
    }
}

/* level-7 armored door and boss mortar/generator (bank0:8609-8838). */
static void contra_rom_level7_boss_door_routine_00(ContraCore *core, uint8_t x)
{
    contra_play_sound(core, 0x1Bu);
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_level7_boss_door_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_HP + x] < 0x05u)
    {
        ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED] = 0x02u;
    }
}

static void contra_rom_level7_boss_door_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u)
    {
        contra_rom_add_a_to_enemy_x_pos(core, x, 0x08u);
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x01u;
    }
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_level7_boss_door_routine_05(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u;
    contra_rom_advance_enemy_routine(core, x);
    contra_rom_add_a_to_enemy_y_pos(core, x, 0x20u);
}

static void contra_rom_level7_boss_door_routine_06(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_a_to_enemy_y_pos(core, x, 0x20u);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] >= 0x02u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x80u;
        contra_rom_clear_enemy(core, x);
    }
}

static void contra_rom_boss_mortar_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t delay = (ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) ? 0x10u : 0x60u;

    contra_rom_add_a_to_enemy_y_pos(core, x, 0x04u);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x04u;
    contra_rom_set_enemy_delay_adv_routine(core, x, delay);
}

static void contra_rom_boss_mortar_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] == 0u) { return; }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u) { return; }
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= 0x02u)
    {
        contra_rom_enable_enemy_collision(core, x);
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x10u;
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x60u);
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    }
}

static void contra_rom_boss_mortar_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_disable_enemy_collision(core, x);
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] == 0u)
    {
        const int slot = contra_rom_find_next_enemy_slot(core);
        if (slot >= 0)
        {
            ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x0Bu;
            contra_rom_initialize_enemy(core, (uint8_t)slot);
            ram[CONTRA_RAM_ENEMY_X_POS + slot] = ram[CONTRA_RAM_ENEMY_X_POS + x];
            ram[CONTRA_RAM_ENEMY_Y_POS + slot] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
            ram[CONTRA_RAM_ENEMY_VAR_1 + slot] = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
        }
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u) { ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x04u; }
    }
}

static void contra_rom_boss_mortar_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u) { return; }
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0xA0u;
        contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    }
}

static void contra_rom_boss_mortar_routine_04(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED] =
        (uint8_t)(core->ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED] + 1u);
    contra_rom_advance_enemy_routine(core, x);
}

static const uint8_t contra_boss_soldier_num_tbl[4] = {0x03u, 0x04u, 0x02u, 0x04u};
static const uint8_t contra_boss_soldier_door_delay_tbl[4] = {0xF0u, 0x80u, 0xA0u, 0xC0u};

static void contra_rom_boss_soldier_generator_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0xA0u);
}

static void contra_rom_boss_soldier_generator_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t side = 0u;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] >= 0x1Eu) ||
        (ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED] >= 0x02u))
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0xF0u;
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x02u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 1u);
    if ((ram[CONTRA_RAM_SPRITE_X_POS] >= 0xA0u) || (ram[CONTRA_RAM_SPRITE_X_POS + 1u] >= 0xA0u))
    {
        side = 1u;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = side;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = contra_boss_soldier_num_tbl[ram[CONTRA_RAM_RANDOM_NUM] & 0x03u];
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x10u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x80u);
}

static void contra_rom_boss_soldier_generator_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u) { return; }
    {
        const int slot = contra_rom_find_next_enemy_slot(core);
        uint8_t attr = ram[CONTRA_RAM_ENEMY_VAR_2 + x];
        if (slot >= 0)
        {
            ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x05u;
            contra_rom_initialize_enemy(core, (uint8_t)slot);
            ram[CONTRA_RAM_ENEMY_X_POS + slot] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 0xF8u);
            ram[CONTRA_RAM_ENEMY_Y_POS + slot] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
            if ((attr == 0u) &&
                ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] >= 0x14u) ||
                 (ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED] != 0u)))
            {
                attr = (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] >> 1u) & 0x01u);
            }
            ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = attr;
        }
    }
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0u) ? 0x10u : 0xFFu;
}

static void contra_rom_boss_soldier_generator_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        contra_boss_soldier_door_delay_tbl[(ram[CONTRA_RAM_RANDOM_NUM] >> 3u) & 0x03u];
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
}

/* level-8 alien fetus/blob shared velocity tables (bank0:9559-9907). */
static const uint8_t contra_white_blob_alien_fetus_vel_tbl[15][4] = {
    {0x00u,0x00u,0x00u,0x00u}, {0x00u,0x42u,0x00u,0x42u},
    {0x00u,0x7Fu,0x00u,0x7Fu}, {0x00u,0xB2u,0x00u,0xB2u},
    {0x00u,0xDDu,0x00u,0xDDu}, {0x00u,0xF7u,0x00u,0xF7u},
    {0x00u,0xFFu,0x00u,0xFFu}, {0x00u,0xF7u,0xFFu,0xF7u},
    {0x00u,0xDDu,0xFFu,0xDDu}, {0x00u,0xB2u,0xFFu,0xB2u},
    {0x00u,0x7Fu,0xFFu,0x7Fu}, {0x00u,0x42u,0xFFu,0x42u},
    {0x00u,0x00u,0x00u,0x00u}, {0xFFu,0xBEu,0x00u,0x42u},
    {0xFFu,0x81u,0x00u,0x7Fu}};
static const uint8_t contra_alien_fetus_aim_timer_tbl[14] = {
    0x16u, 0x0Fu, 0x08u, 0x13u, 0x3Au, 0x06u, 0x21u,
    0x3Au, 0x1Du, 0x14u, 0x12u, 0x28u, 0x48u, 0xFFu};

static uint8_t contra_rom_alien_fetus_get_aim_timer(ContraCore *core)
{
    uint8_t idx = core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] & 0x0Fu;
    uint8_t v = contra_alien_fetus_aim_timer_tbl[idx % 14u];

    if (v == 0xFFu)
    {
        idx = 0u;
        v = contra_alien_fetus_aim_timer_tbl[0];
    }
    core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] = (uint8_t)(idx + 1u);
    return v;
}

static void contra_rom_set_white_blob_alien_fetus_vel(ContraCore *core, uint8_t x, uint8_t aim)
{
    uint8_t *const ram = core->ram;
    const uint8_t *v = contra_white_blob_alien_fetus_vel_tbl[aim % 15u];

    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = v[0];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = v[1];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = v[2];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = v[3];
}

static void contra_rom_alien_fetus_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t aim;

    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(contra_rom_alien_fetus_get_aim_timer(core) << 1u);
    ram[CONTRA_RAM_ENEMY_HP + x] = (uint8_t)(ram[CONTRA_RAM_GAME_COMPLETION_COUNT] + 0x02u);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xACu;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x06u;
    if (ram[CONTRA_RAM_P2_GAME_OVER_STATUS] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] =
            (uint8_t)(((ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x1Fu) + 0x0Eu);
        if (ram[CONTRA_RAM_P1_GAME_OVER_STATUS] != 0u) { ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x01u; }
    }
    aim = ram[CONTRA_RAM_RANDOM_NUM] & 0x03u;
    if (aim == 0u) { aim = 0x03u; }
    aim = (uint8_t)(aim << 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] != 0u) { aim = 0x06u; }
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = aim;
    contra_rom_advance_enemy_routine(core, x);
    contra_rom_set_white_blob_alien_fetus_vel(core, x, aim);
}

static void contra_rom_alien_fetus_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x06u;
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] ^= 0x01u;
        ram[CONTRA_RAM_ENEMY_SPRITES + x] =
            (uint8_t)(0xACu + (((ram[CONTRA_RAM_ENEMY_VAR_1 + x] / 3u) & 0x03u) % 3u) +
                      ram[CONTRA_RAM_ENEMY_VAR_2 + x]);
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] == 0u)
    {
        uint8_t quadrant = 0u;
        const uint8_t aim = contra_rom_aim_at_player(
            core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
            contra_quadrant_aim_dir_01, &quadrant);
        (void)quadrant;
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)((aim < 12u) ? aim : (aim % 12u));
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = contra_rom_alien_fetus_get_aim_timer(core);
        contra_rom_set_white_blob_alien_fetus_vel(core, x, ram[CONTRA_RAM_ENEMY_VAR_1 + x]);
    }
    contra_rom_update_enemy_pos(core, x);
}

static void contra_rom_alien_mouth_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_HP + x] =
        (uint8_t)((ram[CONTRA_RAM_GAME_COMPLETION_COUNT] << 1u) +
                  ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] + 0x04u);
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x20u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x0Au);
}

static void contra_rom_alien_mouth_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x20u) { return; }
    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
        {
            contra_rom_generate_enemy_at_pos(core, x, 0x13u);
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
                (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] & 0x1Fu) + 0xC0u);
        }
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 1u);
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x20u;
    }
}

static void contra_rom_alien_mouth_routine_02(ContraCore *core, uint8_t x)
{
    contra_rom_advance_enemy_routine(core, x);
}

static const uint8_t contra_white_blob_spider_sprite_tbl[4] = {0x00u, 0x01u, 0x02u, 0x01u};
static void contra_rom_white_blob_spider_set_sprite(ContraCore *core, uint8_t x, uint8_t base)
{
    uint8_t *const ram = core->ram;
    uint8_t timer = (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] >> 4u);
    uint8_t idx = ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] & 0x0Fu;

    if (timer != 0u) { --timer; }
    if (timer == 0u)
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(base + contra_white_blob_spider_sprite_tbl[idx & 0x03u]);
        timer = 0x08u;
        idx = (uint8_t)((idx + 1u) & 0x03u);
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = (uint8_t)((timer << 4u) | idx);
}

static void contra_rom_white_blob_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t quadrant = 0u;

    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] & 0x1Fu) + 0x50u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0xC0u;
    if (ram[CONTRA_RAM_P2_GAME_OVER_STATUS] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] =
            (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x01u);
        if (ram[CONTRA_RAM_P1_GAME_OVER_STATUS] != 0u) { ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x01u; }
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xB0u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = contra_rom_aim_at_player(
        core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
        contra_quadrant_aim_dir_01, &quadrant);
    (void)quadrant;
    contra_rom_advance_enemy_routine(core, x);
    contra_rom_set_white_blob_alien_fetus_vel(core, x, ram[CONTRA_RAM_ENEMY_VAR_1 + x]);
}

static void contra_rom_white_blob_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_white_blob_spider_set_sprite(core, x, 0xB0u);
    contra_rom_update_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] == 0u)
        {
            contra_rom_advance_enemy_routine(core, x);
        }
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x20u;
        contra_rom_set_enemy_velocity_to_0(core, x);
    }
}

static void contra_rom_white_blob_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t quadrant = 0u;
    uint8_t aim;

    contra_rom_white_blob_spider_set_sprite(core, x, 0xB0u);
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] == 0u)
    {
        aim = contra_rom_aim_at_player(
            core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
            contra_quadrant_aim_dir_01, &quadrant);
        (void)quadrant;
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = aim;
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x20u;
        contra_rom_set_white_blob_alien_fetus_vel(core, x, aim);
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] * 3u);
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] * 3u);
    }
    contra_rom_update_enemy_pos(core, x);
}

static void contra_rom_set_alien_spider_hp_sprite_attr(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_HP + x] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] + ram[CONTRA_RAM_GAME_COMPLETION_COUNT] + 0x02u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x60u;
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x80u) ? 0x80u : 0x00u;
}

static void contra_rom_alien_spider_set_ground_vel_and_routine(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xB3u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0xFEu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x80u;
    contra_rom_set_enemy_y_velocity_to_0(core, x);
    if (ram[CONTRA_RAM_P2_GAME_OVER_STATUS] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
            (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x01u);
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] =
        (uint8_t)(((ram[CONTRA_RAM_RANDOM_NUM] >> 1u) + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x02u);
    contra_rom_set_enemy_routine_to_a(core, x, 0x04u);
}

static void contra_rom_alien_spider_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_alien_spider_hp_sprite_attr(core, x);
    contra_rom_alien_spider_set_ground_vel_and_routine(core, x);
}

static void contra_rom_alien_spider_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x33u;
    contra_rom_set_alien_spider_hp_sprite_attr(core, x);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xB6u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0xFFu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0xB0u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0xFCu;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x00u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_alien_spider_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint16_t yv = (uint16_t)ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 0x28u;

    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)yv;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + (yv >> 8u));
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x80u)
    {
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
            (uint8_t)(0u - ram[CONTRA_RAM_ENEMY_VAR_3 + x]);
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] =
            (uint8_t)(0u - ram[CONTRA_RAM_ENEMY_VAR_4 + x]);
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = ram[CONTRA_RAM_ENEMY_VAR_3 + x];
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = ram[CONTRA_RAM_ENEMY_VAR_4 + x];
    }
    contra_rom_update_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xC1u) || (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x30u))
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xB3u;
        contra_rom_alien_spider_set_ground_vel_and_routine(core, x);
    }
}

static void contra_rom_alien_spider_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_white_blob_spider_set_sprite(core, x, 0xB3u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] == 0u) && (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u))
    {
        const uint8_t p = ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x01u;
        if ((ram[CONTRA_RAM_SPRITE_Y_POS + p] >= 0x20u) &&
            ((uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - ram[CONTRA_RAM_SPRITE_X_POS + p]) < 0x30u))
        {
            ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
                (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= ram[CONTRA_RAM_SPRITE_Y_POS + p]) ? 0xFFu : 0x02u;
            ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] =
                (ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] == 0x02u) ? 0x40u : 0x00u;
            ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x01u;
            contra_rom_advance_enemy_routine(core, x);
            contra_rom_update_enemy_pos(core, x);
            return;
        }
    }
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] > 0xB8u) { ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0xB8u; }
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x38u) { ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0x38u; }
    contra_rom_update_enemy_pos(core, x);
    contra_rom_set_enemy_y_velocity_to_0(core, x);
}

static void contra_rom_alien_spider_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_update_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xB8u) || (ram[CONTRA_RAM_ENEMY_Y_POS + x] <= 0x38u))
    {
        contra_rom_set_enemy_y_velocity_to_0(core, x);
        contra_rom_set_enemy_routine_to_a(core, x, 0x04u);
    }
}

static void contra_rom_alien_spider_spawn_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_HP + x] =
        (uint8_t)((ram[CONTRA_RAM_GAME_COMPLETION_COUNT] << 1u) +
                  (ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] << 1u) + 0x18u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(((ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x3Fu) + 0xA0u);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x80u) ? 0xA1u : 0xA5u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_alien_spider_spawn_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] != 0u)
        {
            int slot = contra_rom_find_next_enemy_slot(core);
            if (slot >= 0)
            {
                ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x14u;
                contra_rom_initialize_enemy(core, (uint8_t)slot);
                ram[CONTRA_RAM_ENEMY_X_POS + slot] = ram[CONTRA_RAM_ENEMY_X_POS + x];
                ram[CONTRA_RAM_ENEMY_Y_POS + slot] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
                ram[CONTRA_RAM_ENEMY_ROUTINE + slot] = 0x02u;
            }
        }
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] =
            (uint8_t)(0x0Au - ((ram[CONTRA_RAM_P1_CURRENT_WEAPON] | ram[CONTRA_RAM_P2_CURRENT_WEAPON]) & 0x07u) +
                      ((ram[CONTRA_RAM_FRAME_COUNTER] + ram[CONTRA_RAM_RANDOM_NUM]) & 0x03u));
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x14u;
    }
}

static void contra_rom_alien_spider_spawn_routine_02(ContraCore *core, uint8_t x)
{
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_boss_heart_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_HP + x] =
        (uint8_t)((ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] << 1u) +
                  (ram[CONTRA_RAM_GAME_COMPLETION_COUNT] << 1u) + 0x20u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_boss_heart_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u) { return; }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
    contra_rom_set_enemy_delay_adv_routine(core, x, (uint8_t)((ram[CONTRA_RAM_ENEMY_HP + x] >> 1u) ? (ram[CONTRA_RAM_ENEMY_HP + x] >> 1u) : 0x01u));
}

static void contra_rom_boss_heart_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 1u);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0u;
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
}

static void contra_rom_boss_heart_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_init_apu_channels(core);
    contra_play_sound(core, 0x57u);
    ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0xFFu;
    ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
    contra_rom_destroy_all_enemies(core, (int)x);
    contra_rom_begin_enemy_explosion(core, x);
}

static void contra_rom_boss_heart_routine_05(ContraCore *core, uint8_t x)
{
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_boss_heart_routine_06(ContraCore *core, uint8_t x)
{
    contra_rom_advance_enemy_routine(core, x);
}

/* ===== level-8 alien guardian (type 0x10, bank0:8969-9520) ===== */

/* set_guardian_and_heart_enemy_hp (bank0:9016): (weapon strength * 0x10) +
   0x37 + (completion count * 0x10); any 8-bit overflow or sum >= 0xA0 caps
   the HP at 0xA0. */
static void contra_rom_set_guardian_and_heart_enemy_hp(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t ws = (uint8_t)(ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] << 4u);
    const uint8_t cc = (uint8_t)(ram[CONTRA_RAM_GAME_COMPLETION_COUNT] << 4u);
    unsigned sum = (unsigned)ws + 0x37u;

    if (sum <= 0xFFu)
    {
        sum += cc;
    }
    ram[CONTRA_RAM_ENEMY_HP + x] = (uint8_t)((sum >= 0xA0u) ? 0xA0u : sum);
}

/* draw_alien_guardian_supertile (bank0:9095): stamp super-tile `code` at
   (px - 0x0e, py - 0x10) on the 0x40 graphics budget. Returns true when drawn
   (the ROM's $0b success flag inverted). */
static bool contra_rom_alien_guardian_draw_supertile(
    ContraCore *core, uint8_t px, uint8_t py, uint8_t code)
{
    return contra_rom_level5_draw_nametable_supertile(
        core, (uint8_t)(px - 0x0Eu), (uint8_t)(py - 0x10u), code);
}

/* draw_alien_boss_supertiles (bank0:9162): the right tile at (px, py), the
   left tile 0x20 to its left. The ROM's $0b reflects only the LAST (left)
   stamp; a half-drawn pair retries whole next frame. */
static bool contra_rom_alien_guardian_draw_pair(
    ContraCore *core, uint8_t px, uint8_t py, uint8_t right_code, uint8_t left_code)
{
    (void)contra_rom_alien_guardian_draw_supertile(core, px, py, right_code);
    return contra_rom_alien_guardian_draw_supertile(
        core, (uint8_t)(px - 0x20u), py, left_code);
}

/* draw_alien_guardian_top_jaw (bank0:9120): VAR_1 (right) and VAR_1+1 (left)
   at (X + 0x10, Y). VAR_2 holds the retry flag (0 = drawn, 1 = retry). */
static void contra_rom_draw_alien_guardian_top_jaw(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t code = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    const bool ok = contra_rom_alien_guardian_draw_pair(
        core,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 0x10u),
        ram[CONTRA_RAM_ENEMY_Y_POS + x],
        code, (uint8_t)(code + 1u));

    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = ok ? 0x00u : 0x01u;
}

/* draw_alien_guardian_lower_jaw (bank0:9136): super-tile 0x94 (lower jaw,
   mouth open) or 0x83 (blank) at (X + 0x11, Y + 0x20). VAR_3 = retry flag. */
static void contra_rom_draw_alien_guardian_lower_jaw(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t code =
        (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x92u) ? 0x94u : 0x83u;
    const bool ok = contra_rom_alien_guardian_draw_supertile(
        core,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 0x11u),
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x20u),
        code);

    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = ok ? 0x00u : 0x01u;
}

/* alien_boss_supertile_tbl (bank0:9251): {right code, left code, rel y, rel x}
   pairs shared by the guardian and the heart. */
static const uint8_t contra_alien_boss_supertile_tbl[12][4] = {
    {0x83u, 0x83u, 0x00u, 0x10u}, /* 0: two blanks */
    {0x95u, 0x96u, 0xC0u, 0x30u}, /* 1: body destroyed (top) */
    {0x97u, 0x83u, 0xE0u, 0x50u}, /* 2: body destroyed + blank */
    {0x84u, 0x85u, 0xF0u, 0x10u}, /* 3: heart frame 0 top */
    {0x86u, 0x87u, 0x10u, 0x10u}, /* 4: heart frame 0 bottom */
    {0x88u, 0x89u, 0xF0u, 0x10u}, /* 5: heart frame 1 top */
    {0x8Au, 0x8Bu, 0x10u, 0x10u}, /* 6: heart frame 1 bottom */
    {0x8Cu, 0x8Du, 0xF0u, 0x10u}, /* 7: heart destroyed top */
    {0x8Eu, 0x8Fu, 0x10u, 0x10u}, /* 8: heart destroyed bottom */
    {0x83u, 0x83u, 0x20u, 0xF0u}, /* 9: blanks (wall top) */
    {0x83u, 0x83u, 0x40u, 0xF0u}, /* 10: blanks (wall bottom) */
    {0x29u, 0x29u, 0x60u, 0xF0u}, /* 11: empty ground */
};

/* update_alien_boss_supertiles (bank0:9234): stamp a table entry's pair at the
   enemy-relative position. out_px/out_py return the right tile's position (the
   ROM leaves the left stamp's PPU address in $12 for the collision clears). */
static bool contra_rom_update_alien_boss_supertiles(
    ContraCore *core, uint8_t x, uint8_t entry, uint8_t *out_px, uint8_t *out_py)
{
    uint8_t *const ram = core->ram;
    const uint8_t *const e = contra_alien_boss_supertile_tbl[entry];
    const uint8_t px = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + e[3]);
    const uint8_t py = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + e[2]);

    if (out_px != NULL)
    {
        *out_px = px;
    }
    if (out_py != NULL)
    {
        *out_py = py;
    }
    return contra_rom_alien_guardian_draw_pair(core, px, py, e[0], e[1]);
}

/* alien_guardian_clear_wall_bg_collision (bank0:9381) on the native model:
   record world-anchored collision overrides (code 0 = empty) for both stamped
   super-tile cells -- the ROM keys off the left stamp's PPU address and its
   +4-column neighbor. (px, py) is the pair's right-tile position as passed to
   update_alien_boss_supertiles; -0x0e/-0x10 yields the draw anchor. */
static void contra_rom_alien_guardian_clear_wall_cells(
    ContraCore *core, uint8_t px, uint8_t py)
{
    const uint8_t cy = (uint8_t)(py - 0x10u);

    contra_rom_record_supertile_collision_override(
        core, (uint8_t)(px - 0x20u - 0x0Eu), cy, 0x83u, 0x00u);
    contra_rom_record_supertile_collision_override(
        core, (uint8_t)(px - 0x0Eu), cy, 0x83u, 0x00u);
}

/* alien_guardian_routine_00 (bank0:9002): HP, mouth-cycle delay, closed-mouth
   super-tile code, and the 0x40-frame reveal auto-scroll. */
static void contra_rom_alien_guardian_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_set_guardian_and_heart_enemy_hp(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x20u; /* delay between mouth movements */
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x90u; /* super-tile code: mouth closed */
    ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] = 0x40u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x03u);
}

/* alien_guardian_routine_01 (bank0:9037): toggle the mouth every 0x20 frames,
   retrying failed jaw stamps; each open decrements ANIMATION_DELAY (3 from
   routine_00) and the final open advances to the fetus spawner. */
static void contra_rom_alien_guardian_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x50u)
    {
        contra_rom_add_scroll_to_enemy_pos(core, x);
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        contra_rom_draw_alien_guardian_top_jaw(core, x);
    }
    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
    {
        contra_rom_draw_alien_guardian_lower_jaw(core, x);
    }
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x20u;
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x90u)
        {
            ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x92u; /* open the mouth */
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        }
        else
        {
            ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x90u; /* close the mouth */
        }
        contra_rom_draw_alien_guardian_top_jaw(core, x);
        contra_rom_draw_alien_guardian_lower_jaw(core, x);
    }
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    /* draw_lower_jaw_open_adv_routine (bank0:9193) */
    contra_rom_draw_alien_guardian_lower_jaw(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x20u);
}

/* alien_guardian_routine_02 (bank0:9198): with the attack flag on, run the
   0x20-frame delay out and spawn alien fetuses at ticks 1 and 0 -- plus an
   extra at tick 2 when the player's weapon strength is >= 3 -- then go back
   to the mouth loop for 3 more cycles. */
static void contra_rom_alien_guardian_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t delay;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x03u;
        contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
        return;
    }
    delay = (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = delay;
    if (delay > 0x02u)
    {
        return;
    }
    if ((delay == 0x02u) && (ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] < 0x03u))
    {
        return;
    }
    contra_rom_generate_enemy_at_offset(core, x, 0x11u, 0x00u, 0x00u);
    if (delay != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x03u;
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
}

/* create_boss_heart_explosion (bank0:9418): zero the work vars, pop the first
   explosion on the boss, and start the 5-frame burst cadence. */
static void contra_rom_create_boss_heart_explosion(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0u;
    contra_rom_create_explosion_at(
        core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x]);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x05u);
}

/* alien_guardian_routine_03 (bank0:9414): the kill entry -- destroyed sound,
   first explosion. */
static void contra_rom_alien_guardian_routine_03(ContraCore *core, uint8_t x)
{
    contra_play_sound(core, 0x55u);
    contra_rom_create_boss_heart_explosion(core, x);
}

/* alien_guardian_explosion_offset_tbl (bank0:10467): {vertical, horizontal}
   offset pairs (create_explosion_at_x_y feeds byte 0 to the Y adder and byte 1
   to the X adder; the ROM table comment has them swapped). Entry 0 is unused
   -- routine_04 pre-increments VAR_1. */
static const uint8_t contra_alien_guardian_explosion_offset_tbl[24] = {
    0x10u, 0x10u, 0xF0u, 0x10u, 0xF0u, 0xF0u, 0x10u, 0xF0u,
    0x20u, 0x20u, 0xE0u, 0x20u, 0xE0u, 0xE0u, 0x20u, 0xE0u,
    0x40u, 0x40u, 0xC0u, 0x40u, 0xC0u, 0xC0u, 0x50u, 0x00u};

/* alien_guardian_routine_04 (bank0:9425): 11 explosions, 5 frames apart, at
   the table offsets; shared with the heart's destroyed sequence. */
static void contra_rom_alien_guardian_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t n;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x05u;
    n = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = n;
    if (n == 0x0Cu)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    contra_rom_create_explosion_at(
        core,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] +
                  contra_alien_guardian_explosion_offset_tbl[(size_t)n * 2u + 1u]),
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] +
                  contra_alien_guardian_explosion_offset_tbl[(size_t)n * 2u]));
}

/* alien_guardian_routine_05 (bank0:9225): blank the pair above the top jaw;
   on success flag VAR_2 (=1, "next stamp pending") and advance. Also called
   by routine_06's body pass with Y temporarily shifted up 0x20. */
static void contra_rom_alien_guardian_routine_05(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (!contra_rom_update_alien_boss_supertiles(core, x, 0u, NULL, NULL))
    {
        return; /* graphics budget full -- retry next frame */
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x01u;
    contra_rom_advance_enemy_routine(core, x);
}

/* alien_guardian_routine_06 (bank0:9279): blank the single top-jaw tile, then
   on the next pass blank the body pair 0x20 above via routine_05 (which also
   advances). The ROM applies add_scroll twice on the body pass; kept. */
static void contra_rom_alien_guardian_routine_06(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        const bool ok = contra_rom_alien_guardian_draw_supertile(
            core,
            (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 0x10u),
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x20u),
            0x83u);

        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = ok ? 0x00u : 0x01u;
        return;
    }
    ram[CONTRA_RAM_ENEMY_Y_POS + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0xE0u);
    contra_rom_alien_guardian_routine_05(core, x);
    ram[CONTRA_RAM_ENEMY_Y_POS + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x20u);
}

/* alien_guardian_routine_07 (bank0:9302): destroyed-body pair (entry 1), then
   entry 2; advance once the second pair lands. */
static void contra_rom_alien_guardian_routine_07(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] =
            contra_rom_update_alien_boss_supertiles(core, x, 1u, NULL, NULL)
                ? 0x00u : 0x01u;
        return;
    }
    if (!contra_rom_update_alien_boss_supertiles(core, x, 2u, NULL, NULL))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
    contra_rom_advance_enemy_routine(core, x);
}

/* alien_guardian_routine_08 (bank0:9329): blank at (X+0x30, Y) and at
   (X-0x10, Y+0xC0); only the second stamp's success gates the advance. */
static void contra_rom_alien_guardian_routine_08(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    (void)contra_rom_alien_guardian_draw_supertile(
        core,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 0x30u),
        ram[CONTRA_RAM_ENEMY_Y_POS + x],
        0x83u);
    if (!contra_rom_alien_guardian_draw_supertile(
            core,
            (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 0x10u),
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0xC0u),
            0x83u))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
    contra_rom_advance_enemy_routine(core, x);
}

/* alien_guardian_routine_09 (bank0:9345): blank the wall pair at Y+0x20, then
   Y+0x40, clearing both cells' bg collision each pass (the ROM clears even
   when the stamp missed the budget; the cells are the same on retry). */
static void contra_rom_alien_guardian_routine_09(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t px, py;
    bool ok;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        ok = contra_rom_update_alien_boss_supertiles(core, x, 9u, &px, &py);
        contra_rom_alien_guardian_clear_wall_cells(core, px, py);
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = ok ? 0x00u : 0x01u;
        return;
    }
    ok = contra_rom_update_alien_boss_supertiles(core, x, 10u, &px, &py);
    contra_rom_alien_guardian_clear_wall_cells(core, px, py);
    if (ok)
    {
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x01u;
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x01u; /* keep retrying the bottom pair */
}

/* alien_guardian_routine_0a (bank0:9394): stamp empty ground where the wall
   was (entry 11) and clear the cell row above it (PPU address - 0x20 = one
   8px row up, snapping into the already-cleared Y+0x40 cells). */
static void contra_rom_alien_guardian_routine_0a(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t px, py;
    bool ok;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    ok = contra_rom_update_alien_boss_supertiles(core, x, 11u, &px, &py);
    contra_rom_alien_guardian_clear_wall_cells(core, px, (uint8_t)(py - 0x08u));
    if (ok)
    {
        contra_rom_advance_enemy_routine(core, x);
    }
}

static void contra_rom_alien_guardian_routine_0b(ContraCore *core, uint8_t x)
{
    contra_rom_destroy_all_enemies(core, (int)x);
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0xA0u;
    contra_rom_clear_enemy(core, x);
}

/* exe_enemy_type (bank7:7360-7474): dispatch enemy type/routine to handler. */
