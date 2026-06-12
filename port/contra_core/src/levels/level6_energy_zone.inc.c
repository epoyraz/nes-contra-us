/* Level 6 energy-zone fire beams, giant soldier boss, and spiked disk projectile.
   Included by core.c; not compiled as a separate translation unit. */

static const uint8_t contra_fire_beam_anim_delay_tbl[4] = {0x00u, 0x20u, 0x40u, 0x60u};
static const uint8_t contra_fire_beam_length_tbl[4] = {0x05u, 0x09u, 0x0Du, 0x0Fu};
static const uint8_t contra_fire_beam_not_firing_sprite_tbl[8] = {
    0x01u, 0xBFu, 0xC0u, 0xBFu, 0x01u, 0xC1u, 0xC2u, 0xC1u};

static void contra_rom_fire_beam_add_pos_set_delay(ContraCore *core, uint8_t x, uint8_t attr_bits)
{
    uint8_t *const ram = core->ram;
    uint8_t attr;

    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] | attr_bits);
    contra_rom_add_a_to_enemy_y_pos(core, x, 0x08u);
    attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_fire_beam_anim_delay_tbl[(attr >> 2u) & 0x03u];
    contra_rom_set_enemy_delay_adv_routine(core, x, ram[CONTRA_RAM_ENEMY_VAR_A + x]);
}

static void contra_rom_animate_small_flame(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t delay;

    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    delay = ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x];
    if ((delay & 0x07u) != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        contra_fire_beam_not_firing_sprite_tbl[((delay >> 3u) & 0x03u) |
                                               ram[CONTRA_RAM_ENEMY_FRAME + x]];
}

static void contra_rom_begin_fire_beam_attack(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t player_index = 0u;

    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x30u)
    {
        return;
    }
    (void)contra_rom_player_enemy_x_distance(core, x, &player_index);
    contra_play_sound(core, 0x09u);
    contra_rom_enable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] =
        contra_fire_beam_length_tbl[ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x03u];
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x00u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x00u);
}

static void contra_level7_record_tile_update(ContraCore *core, int x, int y, uint8_t tile_index);

/* draw_fire_beam_if_anim_elapsed + draw_fire_beam_tiles (bank0:7389-7447):
   stamp one beam section (a level_6_tile_animation code from
   fire_beam_tile_tbl) at the beam tip on the 0x50 tile budget. Returns the
   ROM's inverted carry: true = the caller steps the beam (drawn, or the tip
   is off-screen), false = hold (delay ticking, or the stamp missed the
   budget). */
static bool contra_rom_draw_fire_beam_if_anim_elapsed(
    ContraCore *core, uint8_t x, uint8_t tbl_base)
{
    static const uint8_t fire_beam_tile_tbl[12] = {
        0x87u, 0x88u, 0x80u, 0x89u,  /* down: grow start/mid, retract blank/end */
        0x84u, 0x85u, 0x80u, 0x86u,  /* left */
        0x81u, 0x82u, 0x80u, 0x83u}; /* right */
    uint8_t *const ram = core->ram;
    const uint8_t var3 = ram[CONTRA_RAM_ENEMY_VAR_3 + x];
    uint8_t tile;
    uint8_t py;
    unsigned sum;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return false;
    }
    tile = fire_beam_tile_tbl[tbl_base +
        ((((uint8_t)(var3 | ram[CONTRA_RAM_ENEMY_VAR_4 + x])) != 0u) ? 1u : 0u)];
    py = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] +
                   ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
    sum = (unsigned)(uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 0x07u) + var3;
    if ((var3 & 0x80u) != 0u)
    {
        if (sum <= 0xFFu)
        {
            return true; /* leftward tip off-screen left: skip the stamp, step */
        }
    }
    else if (sum > 0xFFu)
    {
        return true; /* rightward tip off-screen right: skip the stamp, step */
    }
    if (!contra_rom_tile_animation_draw_budget(core))
    {
        return false; /* graphics budget full -- retry next frame */
    }
    contra_level7_record_tile_update(core, (int)(uint8_t)sum, (int)py, tile);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x30u) ? 0x00u : 0x01u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
        (uint8_t)(var3 | ram[CONTRA_RAM_ENEMY_VAR_4 + x]);
    return true;
}

static void contra_rom_fire_beam_disable_collision_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = ram[CONTRA_RAM_ENEMY_VAR_A + x];
    contra_rom_disable_enemy_collision(core, x);
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
}

static void contra_rom_fire_beam_set_delay_10_adv_routine(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x10u);
}

static void contra_rom_fire_beam_down_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x04u;
    contra_rom_fire_beam_add_pos_set_delay(core, x, 0x80u);
}

static void contra_rom_fire_beam_down_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t player_index = 0u;

    contra_rom_animate_small_flame(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }
    if (contra_rom_player_enemy_x_distance(core, x, &player_index) < 0x20u)
    {
        contra_rom_begin_fire_beam_attack(core, x);
    }
}

static void contra_rom_fire_beam_down_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u) ||
        !contra_rom_draw_fire_beam_if_anim_elapsed(core, x, 0u))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        contra_rom_fire_beam_set_delay_10_adv_routine(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 0x08u);
}

static void contra_rom_fire_beam_down_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u) ||
        !contra_rom_draw_fire_beam_if_anim_elapsed(core, x, 2u))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 0x08u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_4 + x] & 0x80u) != 0u)
    {
        contra_rom_fire_beam_disable_collision_routine_01(core, x);
    }
}

static void contra_rom_fire_beam_left_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_fire_beam_add_pos_set_delay(core, x, 0x40u);
}

static void contra_rom_fire_beam_left_routine_01(ContraCore *core, uint8_t x)
{
    contra_rom_animate_small_flame(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x7Fu) == core->ram[CONTRA_RAM_ENEMY_VAR_A + x])
    {
        contra_rom_begin_fire_beam_attack(core, x);
    }
}

static void contra_rom_fire_beam_left_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u) ||
        !contra_rom_draw_fire_beam_if_anim_elapsed(core, x, 4u))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        contra_rom_fire_beam_set_delay_10_adv_routine(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 0x08u);
}

static void contra_rom_fire_beam_left_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u) ||
        !contra_rom_draw_fire_beam_if_anim_elapsed(core, x, 6u))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 0x08u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u) &&
        ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) == 0u))
    {
        contra_rom_fire_beam_disable_collision_routine_01(core, x);
    }
}

static void contra_rom_fire_beam_right_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x40u;
    contra_rom_fire_beam_add_pos_set_delay(core, x, 0x00u);
}

static void contra_rom_fire_beam_right_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_animate_small_flame(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_A + x] =
        (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x3Fu);
    contra_rom_begin_fire_beam_attack(core, x);
}

static void contra_rom_fire_beam_right_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u) ||
        !contra_rom_draw_fire_beam_if_anim_elapsed(core, x, 8u))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        contra_rom_fire_beam_set_delay_10_adv_routine(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 0x08u);
}

static void contra_rom_fire_beam_right_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u) ||
        !contra_rom_draw_fire_beam_if_anim_elapsed(core, x, 10u))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 0x08u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) != 0u)
    {
        contra_rom_fire_beam_disable_collision_routine_01(core, x);
    }
}

static const uint8_t contra_boss_giant_jump_x_vel_tbl[4][2] = {
    {0x00u, 0x80u}, {0x00u, 0x00u}, {0x00u, 0x00u}, {0xFFu, 0x80u}};
static const uint8_t contra_boss_giant_walk_x_vel_tbl[2][2] = {
    {0x01u, 0x18u}, {0xFEu, 0xE8u}};

static void contra_rom_boss_giant_set_x_velocity_to_0(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
}

static void contra_rom_boss_giant_set_palette(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) != 0x03u)
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_HP + x] >= 0x20u)
    {
        return;
    }
    ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + 7u] =
        (ram[CONTRA_RAM_ENEMY_HP + x] >= 0x10u) ? 0x51u : 0x52u;
    contra_load_palettes_color_to_cpu(core, 0x20u);
}

static void contra_rom_boss_giant_face_random_player(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t player = 0u;

    if (ram[CONTRA_RAM_P2_GAME_OVER_STATUS] == 0u)
    {
        player = ram[CONTRA_RAM_RANDOM_NUM] & 0x01u;
        if (ram[CONTRA_RAM_P1_GAME_OVER_STATUS] != 0u)
        {
            player = 0x01u;
        }
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (ram[CONTRA_RAM_ENEMY_X_POS + x] < ram[CONTRA_RAM_SPRITE_X_POS + player]) ? 0x40u : 0x00u;
}

static void contra_rom_boss_giant_stay_still(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xB8u;
    contra_rom_set_enemy_velocity_to_0(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_RANDOM_NUM];
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x80u) |
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x];
    contra_rom_set_enemy_routine_to_a(core, x, 0x03u);
}

/* boss_giant_soldier_routine_00..06 (bank0:7474-7930): Level-6 boss robot
   active combat states plus the first destroyed-state explosion handoff. */
static void contra_rom_boss_giant_soldier_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_HP + x] =
        (uint8_t)(0x40u + ((ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] << 3u) & 0xFFu));
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_RANDOM_NUM];
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xB8u;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0x9Bu;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x31u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_boss_giant_soldier_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_update_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
    }
}

static void contra_rom_boss_giant_soldier_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t action = ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x03u;

    contra_rom_update_enemy_pos(core, x);
    if (action == 0u)
    {
        const uint8_t idx = (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x03u);

        contra_rom_boss_giant_face_random_player(core, x);
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xF9u;
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x80u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_boss_giant_jump_x_vel_tbl[idx][0];
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_boss_giant_jump_x_vel_tbl[idx][1];
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xBAu;
        contra_rom_set_enemy_routine_to_a(core, x, 0x05u);
        return;
    }

    contra_rom_boss_giant_face_random_player(core, x);
    if (action == 1u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 1u);
        if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] >= 0x04u)
        {
            contra_rom_boss_giant_stay_still(core, x);
            return;
        }
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x20u;
        contra_rom_advance_enemy_routine(core, x);
        return;
    }

    {
        const uint8_t idx = ram[CONTRA_RAM_RANDOM_NUM] & 0x01u;

        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_boss_giant_walk_x_vel_tbl[idx][0];
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_boss_giant_walk_x_vel_tbl[idx][1];
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x0Cu;
    contra_rom_set_enemy_routine_to_a(core, x, 0x06u);
}

static void contra_rom_boss_giant_create_spiked_projectile(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const int slot = contra_rom_find_next_enemy_slot(core);

    if (slot >= 0)
    {
        const uint8_t sx = (uint8_t)slot;

        ram[CONTRA_RAM_ENEMY_TYPE + sx] = 0x14u;
        contra_rom_initialize_enemy(core, sx);
        ram[CONTRA_RAM_ENEMY_X_POS + sx] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 0xF0u);
        ram[CONTRA_RAM_ENEMY_Y_POS + sx] = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0xE8u);
        if ((ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0x40u) != 0u)
        {
            ram[CONTRA_RAM_ENEMY_X_POS + sx] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + sx] + 0x30u);
        }
    }
    contra_rom_boss_giant_stay_still(core, x);
}

static void contra_rom_boss_giant_soldier_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_update_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        contra_rom_boss_giant_stay_still(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_boss_giant_create_spiked_projectile(core, x);
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] < 0x0Fu)
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xC3u;
    }
    contra_rom_boss_giant_set_palette(core, x);
}

static void contra_rom_boss_giant_apply_gravity(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_to_enemy_y_fract_vel(core, x, 0x38u);
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x21u)
    {
        ram[CONTRA_RAM_ENEMY_X_POS + x] = 0x20u;
        contra_rom_boss_giant_set_x_velocity_to_0(core, x);
    }
    else if (ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xC0u)
    {
        ram[CONTRA_RAM_ENEMY_X_POS + x] = 0xC0u;
        contra_rom_boss_giant_set_x_velocity_to_0(core, x);
    }
}

static void contra_rom_boss_giant_soldier_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_boss_giant_set_palette(core, x);
    contra_rom_boss_giant_apply_gravity(core, x);
    contra_rom_update_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x9Bu)
    {
        return;
    }
    contra_play_sound(core, 0x15u);
    contra_rom_set_enemy_velocity_to_0(core, x);
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0x9Bu;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xB8u;
    contra_rom_boss_giant_stay_still(core, x);
}

static void contra_rom_boss_giant_soldier_routine_05(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_boss_giant_set_palette(core, x);
    contra_rom_update_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x20u) || (ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xC0u))
    {
        contra_rom_boss_giant_stay_still(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x0Cu;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 1u);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        (uint8_t)(0xB8u + (ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x01u));
}

static void contra_rom_boss_giant_soldier_routine_06(ContraCore *core, uint8_t x)
{
    contra_init_apu_channels(core);
    contra_play_sound(core, 0x55u);
    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0xFFu;
    core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
    contra_rom_destroy_all_enemies(core, (int)x);
    core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u;
    contra_rom_create_explosion_at(core, core->ram[CONTRA_RAM_ENEMY_X_POS + x], core->ram[CONTRA_RAM_ENEMY_Y_POS + x]);
    contra_rom_advance_enemy_routine(core, x);
}

/* boss_giant_explosion_loc_tbl (bank0:7833): the 4 corner explosion offsets
   (Y,X) relative to the giant's center, created one-per-frame by routine_07. */
static const uint8_t contra_boss_giant_explosion_loc_tbl[8] = {
    0xF0u, 0xF0u, 0x10u, 0x10u, 0xF0u, 0x10u, 0x10u, 0xF0u};

/* boss_giant_door_open_00/01 (bank0:7950-7956): the end-of-level door opening
   animation tile rows. byte 0 is the initial Y position, the remaining bytes
   are level_6_tile_animation codes terminated by 0xFF. */
static const uint8_t contra_boss_giant_door_open_00[4] = {0x90u, 0x8Bu, 0x8Au, 0xFFu};
static const uint8_t contra_boss_giant_door_open_01[3] = {0x58u, 0x8Du, 0xFFu};

/* boss_giant_soldier_routine_07 (bank0:7800): after the boss is destroyed,
   spawn the four corner explosions (one per frame). Once all four are out,
   set a 0x30 delay and advance to routine_08. */
static void contra_rom_boss_giant_soldier_routine_07(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t old_var1;
    uint8_t idx;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] >= 0x04u)
    {
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x30u); /* -> routine_08 */
        return;
    }
    old_var1 = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    idx = (uint8_t)(old_var1 << 1u);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(old_var1 + 1u);
    contra_rom_create_explosion_at(
        core,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + contra_boss_giant_explosion_loc_tbl[idx + 1u]),
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + contra_boss_giant_explosion_loc_tbl[idx]));
}

/* boss_giant_soldier_routine_08 (bank0:7842): one-shot set-up of the door
   opening -- the dec/beq on ANIMATION_DELAY is a no-op (always falls through),
   so this clears the custom vars, primes 8 door-opening steps, and advances. */
static void contra_rom_boss_giant_soldier_routine_08(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x08u; /* number of door opening steps */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x0Au); /* -> routine_09 */
}

/* boss_giant_soldier_routine_09 (bank0:7855): animate the end-of-level door
   opening in the wall, one step every 0x0A frames. ENEMY_VAR_1 selects the
   animation phase (0 = bottom rows, 1 = the repeated 0x8C middle, 2 = top row);
   once phase 2 finishes, set_delay_remove_enemy(0x01) clears DELAY_TIME_LOW
   from the level_boss_defeated 0xFF sentinel so level_routine_08 can proceed. */
static void contra_rom_boss_giant_soldier_routine_09(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t *tbl;
    uint8_t read_idx;
    uint8_t draw_y;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }

    /* @open_door_section */
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;

    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x01u)
    {
        /* @mode_1_open_door: stamp tile 0x8C at (0xD0, VAR_2) for 8 steps. */
        if (contra_rom_tile_animation_draw_budget(core))
        {
            contra_level7_record_tile_update(
                core, 0xD0, ram[CONTRA_RAM_ENEMY_VAR_2 + x], 0x8Cu);
        }
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] == 0u)
        {
            ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u); /* -> phase 2 */
        }
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 0x08u);
        return;
    }

    /* phase 0 or 2: draw a column of door tiles from the matching table. */
    tbl = (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u)
        ? contra_boss_giant_door_open_00
        : contra_boss_giant_door_open_01;
    draw_y = (uint8_t)(tbl[0] + 0x10u);
    for (read_idx = 1u; tbl[read_idx] != 0xFFu; ++read_idx)
    {
        if (contra_rom_tile_animation_draw_budget(core))
        {
            contra_level7_record_tile_update(core, 0xD0, draw_y, tbl[read_idx]);
        }
        draw_y = (uint8_t)(draw_y + 0x10u);
    }

    /* @finished_drawing_door */
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(draw_y - 0x20u);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x03u)
    {
        /* @remove_enemy: set_delay_remove_enemy(0x01). */
        ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x01u;
        ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x00u;
        contra_rom_remove_enemy(core, x);
    }
}

/* boss_giant_projectile_routine_00/01 (bank0:7947-8025): Level-6 spiked disk
   projectile. It starts with the giant soldier's facing direction, falls until it
   hits Y #$af, alternates sprite_bb/sprite_bc every six frames, and advances to
   remove_enemy once off the right edge. */
static void contra_rom_boss_giant_projectile_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    int boss_slot;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x06u;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xBBu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0xFDu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x00u;

    for (boss_slot = 15; boss_slot > 0; --boss_slot)
    {
        if (ram[CONTRA_RAM_ENEMY_TYPE + (uint8_t)boss_slot] == 0x13u)
        {
            if ((ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + (uint8_t)boss_slot] & 0x40u) != 0u)
            {
                ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x03u;
                ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x00u;
            }
            break;
        }
    }

    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0x02u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_boss_giant_projectile_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xE0u)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x06u;
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u);
        ram[CONTRA_RAM_ENEMY_SPRITES + x] =
            (uint8_t)(0xBBu + (ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x01u));
        contra_rom_update_enemy_pos(core, x);
        return;
    }

    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xAFu)
    {
        contra_rom_set_enemy_y_velocity_to_0(core, x);
    }
    contra_rom_update_enemy_pos(core, x);
}
