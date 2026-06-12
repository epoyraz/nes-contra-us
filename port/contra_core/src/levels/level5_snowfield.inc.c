/* Level 5 snowfield ice grenades, tank, UFO, saucers, and bombs.
   Included by core.c; not compiled as a separate translation unit. */

static void contra_rom_add_to_enemy_y_fract_vel(ContraCore *core, uint8_t x, uint8_t a)
{
    uint8_t *const ram = core->ram;
    const unsigned f = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] + a;

    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)f;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (f >> 8u));
}

/* ice_grenade_generator_routine_00/01 (bank0:6175-6190): scroll the generator
   until it reaches X < #$c8, then spawn a grenade every #$80 frames. */
static void contra_rom_ice_grenade_generator_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (core->ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xC8u)
    {
        return;
    }
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
}

static void contra_rom_ice_grenade_generator_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

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
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x80u;
    contra_rom_generate_enemy_at_pos(core, x, 0x11u);
}

static const uint8_t contra_ice_grenade_sprite_tbl[4] = {0x74u, 0x75u, 0x76u, 0x77u};

/* ice_grenade_routine_00/01 (bank0:6201-6248): whistle in from the generator,
   animate under gravity, and explode when the ground collision probe hits. */
static void contra_rom_ice_grenade_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_play_sound(core, 0x1Au);
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x20u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x80u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xFEu;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_ice_grenade_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) == 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        contra_ice_grenade_sprite_tbl[ram[CONTRA_RAM_ENEMY_FRAME + x] & 0x03u];
    contra_rom_update_enemy_pos(core, x);
    contra_rom_add_to_enemy_y_fract_vel(core, x, 0x0Au);
    if (ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] < 0x80u)
    {
        ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x00u;
        if (contra_rom_add_y_to_y_pos_get_bg_collision(core, x, 0x04u) != 0u)
        {
            contra_play_sound(core, 0x24u);
            contra_rom_advance_enemy_routine(core, x);
        }
    }
}

/* ice_separator_routine_00 (bank0:7143-7156): pipe joint sprite; when the tank's
   ICE-JOINT scroll flag ($7F, set for the tank's whole visit) is set it moves
   left by one whenever the frame scrolls, otherwise it is terrain-anchored
   through add_scroll_to_enemy_pos. */
static void contra_rom_ice_separator_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xC4u;
    if (ram[CONTRA_RAM_TANK_ICE_JOINT_SCROLL_FLAG] != 0u)
    {
        if (ram[CONTRA_RAM_FRAME_SCROLL] != 0u)
        {
            ram[CONTRA_RAM_ENEMY_X_POS + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 1u);
        }
        return;
    }
    contra_rom_add_scroll_to_enemy_pos(core, x);
}

/* ===== level-5 tank (type 0x12), bank0:6250-6650 ===========================
   The tank is BACKGROUND super-tiles, not a sprite: it scrolls itself onto the
   screen with TANK_AUTO_SCROLL, redraws its tires/turret as nametable
   super-tiles on the graphics budget, and tracks its own visibility in
   ENEMY_X_VEL_ACCUM (0x01 = off right, 0x00 = visible, 0xFF = off left). */
static const uint8_t contra_tank_wheel_supertile_tbl[4] = {0x10u, 0x11u, 0x14u, 0x15u};
static const uint8_t contra_tank_attack_delay_tbl[2] = {0x00u, 0xF8u};
static const uint8_t contra_tank_turret_supertile_tbl[3] = {0x13u, 0x12u, 0x0Fu};
static const uint8_t contra_tank_palette_tbl[5] = {0x61u, 0x60u, 0x5Fu, 0x3Fu, 0x3Fu};
/* {x off, y off, bullet type|angle} per turret direction 0x0A..0x0C */
static const uint8_t contra_tank_bullet_pos_vel_tbl[9] = {
    0x24u, 0x03u, 0x09u,
    0x29u, 0x09u, 0x0Au,
    0x2Eu, 0x14u, 0x0Cu};
/* {visibility add, blank-tile x, blank-tile y, explosion x, explosion y} */
static const uint8_t contra_tank_destroy_tbl[30] = {
    0x00u, 0x16u, 0x04u, 0x1Cu, 0x0Eu,
    0x00u, 0x16u, 0xE4u, 0x1Cu, 0xF2u,
    0xFFu, 0xF6u, 0x04u, 0x00u, 0x0Eu,
    0xFFu, 0xF6u, 0xE4u, 0x00u, 0xF2u,
    0xFFu, 0xD6u, 0x04u, 0xE4u, 0x0Eu,
    0xFFu, 0xD6u, 0xE4u, 0xE4u, 0xF2u};

static void contra_level7_record_supertile_update(ContraCore *core, int x, int y, uint8_t supertile_index);

/* load_bank_3_update_nametable_supertile on level 5: a full super-tile stamp on
   the 0x40 graphics budget; the drawn cell is recorded in the (generic)
   position-keyed overlay cache so the native renderer persists it. Returns
   false when the budget rejects the draw (the ROM's carry-set). */
static bool contra_rom_level5_draw_nametable_supertile(
    ContraCore *core, uint8_t px, uint8_t py, uint8_t code)
{
    if (!contra_rom_enemy_supertile_draw_budget(core))
    {
        return false;
    }
    contra_level7_record_supertile_update(core, (int)px, (int)py, (uint8_t)(code & 0x7Fu));
    return true;
}

/* tank_update_pos (bank0:6359): X -= FRAME_SCROLL + TANK_AUTO_SCROLL; an
   underflow steps the visibility state down (appearing from the right, or
   leaving to the left) and -- while the tank has HP -- TOGGLES the bullet
   collision bits (sw ^= 0x81): on for the visible crossing, back off for the
   exit crossing. */
static void contra_rom_tank_update_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t scroll =
        (uint8_t)(ram[CONTRA_RAM_FRAME_SCROLL] + ram[CONTRA_RAM_TANK_AUTO_SCROLL]);
    const uint8_t old_x = ram[CONTRA_RAM_ENEMY_X_POS + x];

    ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(old_x - scroll);
    if (old_x >= scroll)
    {
        return; /* no underflow */
    }
    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_HP + x] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] ^ 0x81u);
}

/* tank_check_removal (bank0:6520): once the tank is off-screen left
   (visibility negative) and X wraps below 0xD0, remove it and restore the
   scroll/palette flags. Returns false when removed (the ROM's carry-clear). */
static bool contra_rom_tank_check_removal(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] & 0x80u) == 0u)
    {
        return true;
    }
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xD0u)
    {
        return true;
    }
    contra_rom_remove_enemy(core, x);
    ram[CONTRA_RAM_TANK_AUTO_SCROLL] = 0x00u;
    ram[CONTRA_RAM_PAUSE_PALETTE_CYCLE] = 0x00u;
    ram[CONTRA_RAM_TANK_ICE_JOINT_SCROLL_FLAG] = 0x00u;
    return false;
}

/* tank_set_palette (bank0:6634): palette row 2 follows HP in 16-point steps. */
static void contra_rom_tank_set_palette(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + 2u] =
        contra_tank_palette_tbl[(core->ram[CONTRA_RAM_ENEMY_HP + x] >> 4u) & 0x07u];
    contra_load_palettes_color_to_cpu(core, 0x10u);
}

/* tank_move_logic + the tire redraw (bank0:6300-6356). */
static void contra_rom_tank_move_logic(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t tire_x;
    uint8_t vis;

    if (ram[CONTRA_RAM_ENEMY_HP + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] == 0u)
        {
            contra_play_sound(core, 0x1Eu); /* tank engine */
            ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x06u;
        }
    }

    {
        const uint8_t supertile =
            contra_tank_wheel_supertile_tbl[ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u];

        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) == 0u)
        {
            /* front tire: X - 0x0C, the borrow chains into the visibility check */
            const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];

            tire_x = (uint8_t)(ex - 0x0Cu);
            vis = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] -
                            ((ex < 0x0Cu) ? 1u : 0u));
        }
        else
        {
            /* back tire: X + 0x14, the carry chains into the visibility check */
            const unsigned sum = (unsigned)ram[CONTRA_RAM_ENEMY_X_POS + x] + 0x14u;

            tire_x = (uint8_t)sum;
            vis = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] + (sum >> 8u));
        }
        if (vis == 0u)
        {
            (void)contra_rom_level5_draw_nametable_supertile(
                core, tire_x, ram[CONTRA_RAM_ENEMY_Y_POS + x], supertile);
        }
    }
}

/* tank_routine_00 (bank0:6258): seat the tank off-screen right, freeze the
   palette cycle, flag the pipe joints, and override the level palettes. */
static void contra_rom_tank_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_X_POS + x] = 0x30u;
    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] = 0x01u; /* off-screen to the right */
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0x90u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x0Cu; /* turret aimed straight left */
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x06u;
    ram[CONTRA_RAM_PAUSE_PALETTE_CYCLE] = 0x3Fu;
    ram[CONTRA_RAM_TANK_ICE_JOINT_SCROLL_FLAG] = 0x3Fu;
    ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + 2u] = 0x3Fu;
    ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + 3u] = 0x41u;
    contra_load_palettes_color_to_cpu(core, 0x10u);
    contra_rom_advance_enemy_routine(core, x);
}

/* tank_routine_01 (bank0:6286): drive left (auto-scrolling every other frame)
   until fully visible and within 0xA0 of the left, then arm the stop. */
static void contra_rom_tank_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_TANK_AUTO_SCROLL] = (uint8_t)(ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u);
    contra_rom_tank_update_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] == 0u) &&
        (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0xA0u))
    {
        /* tank_stop (bank0:6342) */
        ram[CONTRA_RAM_TANK_AUTO_SCROLL] = 0x00u;
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] =
            contra_tank_attack_delay_tbl[ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u];
        ram[CONTRA_RAM_ENEMY_HP + x] = 0x47u;
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    contra_rom_tank_move_logic(core, x);
}

/* tank_routine_02 (bank0:6385): stopped -- count the stop timer down on even
   frames, slowly swing the turret toward the player (aim wheel clamped to
   0x0A..0x0C), redraw the turret super-tile, and fire 3-round bursts. */
static void contra_rom_tank_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_tank_set_palette(core, x);
    contra_rom_tank_update_pos(core, x);
    if (!contra_rom_tank_check_removal(core, x))
    {
        return;
    }
    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] == 0u)
        {
            contra_rom_advance_enemy_routine(core, x);
            return;
        }
    }
    if ((ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x80u) != 0u)
    {
        return; /* not yet hittable -> not yet firing */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }

    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
    {
        /* burst in progress: spawn a bullet from the turret muzzle */
        const uint8_t dir = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 0x0Au);
        const uint8_t *const e = &contra_tank_bullet_pos_vel_tbl[dir * 3u];
        const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
        const uint8_t bx = (uint8_t)(ex - e[0]);
        const uint8_t vis = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] -
                                      ((ex < e[0]) ? 1u : 0u));

        if ((vis & 0x80u) != 0u)
        {
            return; /* muzzle off-screen left */
        }
        contra_rom_create_enemy_bullet_angle_a(
            core, e[2], ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x], bx,
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] - e[1]));
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x20u;
        return;
    }

    {
        /* re-aim one wheel step; out-of-range targets keep the old direction */
        const uint8_t prev = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
        const uint8_t player = contra_rom_player_enemy_x_dist(core, x);
        bool aimed;
        uint8_t v1;

        /* aim source is (X, Y - 0x0C) */
        {
            const uint8_t sy = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] - 0x0Cu);
            const uint8_t sx = ram[CONTRA_RAM_ENEMY_X_POS + x];
            uint8_t quadrant = 0u;
            uint8_t target = 0u;
            uint8_t rot;

            rot = contra_rom_get_quadrant_aim_dir_for_player(
                core, sx, sy, player, contra_quadrant_aim_dir_01, &quadrant);
            rot = contra_rom_get_rotate_dir(core, x, rot, quadrant, 1u, &target);
            if ((rot & 0x80u) != 0u)
            {
                aimed = true;
            }
            else
            {
                uint8_t v = ram[CONTRA_RAM_ENEMY_VAR_1 + x];

                if (rot != 0u)
                {
                    v = (uint8_t)(v - 1u);
                    if ((v & 0x80u) != 0u)
                    {
                        v = 0x17u;
                    }
                }
                else
                {
                    v = (uint8_t)(v + 1u);
                    if (v >= 0x18u)
                    {
                        v = 0u;
                    }
                }
                ram[CONTRA_RAM_ENEMY_VAR_1 + x] = v;
                aimed = (v == target);
            }
            v1 = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
            if ((v1 < 0x0Au) || (v1 >= 0x0Du))
            {
                ram[CONTRA_RAM_ENEMY_VAR_1 + x] = prev; /* clamp to the turret arc */
                aimed = true;                           /* $0c is overwritten too */
            }
        }
        if (aimed)
        {
            ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x03u; /* next round: 3 bullets */
        }
        /* redraw the turret super-tile at (X-0x2C, Y-0x1C) */
        {
            const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
            const uint8_t tx = (uint8_t)(ex - 0x2Cu);
            const uint8_t ty = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] - 0x1Cu);
            const uint8_t vis = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] -
                                          ((ex < 0x2Cu) ? 1u : 0u));

            if (vis != 0u)
            {
                return;
            }
            if (!contra_rom_level5_draw_nametable_supertile(
                    core, tx, ty,
                    contra_tank_turret_supertile_tbl[
                        ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 0x0Au]))
            {
                ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u; /* retry */
                return;
            }
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x30u;
        }
    }
}

/* tank_routine_03 (bank0:6506): resume driving left until removed. */
static void contra_rom_tank_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_TANK_AUTO_SCROLL] = (uint8_t)(ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u);
    contra_rom_tank_set_palette(core, x);
    contra_rom_tank_update_pos(core, x);
    if (!contra_rom_tank_check_removal(core, x))
    {
        return;
    }
    contra_rom_tank_move_logic(core, x);
}

/* tank_routine_04 (bank0:6545): destroyed -- arm the 6-step demolition. */
static void contra_rom_tank_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_tank_update_pos(core, x);
    contra_rom_disable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x05u;
    contra_play_sound(core, 0x55u); /* tank destroyed */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x00u);
}

/* tank_routine_05 (bank0:6560): walk the demolition table -- blank each
   visible super-tile cell (budget-gated, retried) and burst a two-round 0x89
   explosion there; off-screen cells are skipped. */
static void contra_rom_tank_routine_05(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_TANK_AUTO_SCROLL] = (uint8_t)(ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u);
    contra_rom_tank_update_pos(core, x);
    if (!contra_rom_tank_check_removal(core, x))
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }
    if ((ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x80u) != 0u)
    {
        return; /* demolition finished; waits for check_removal */
    }
    {
        const uint8_t *const e =
            &contra_tank_destroy_tbl[(size_t)ram[CONTRA_RAM_ENEMY_VAR_1 + x] * 5u];
        const unsigned bsum = (unsigned)ram[CONTRA_RAM_ENEMY_X_POS + x] + e[1];
        const uint8_t blank_x = (uint8_t)bsum;
        const uint8_t vis = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] + e[0] +
                                      (uint8_t)(bsum >> 8u));

        if (vis != 0u)
        {
            /* that part of the tank is off-screen: skip its demolition step */
            ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
            return;
        }
        /* the ROM's Y add inherits the visibility adc's carry */
        {
            const unsigned ysum = (unsigned)ram[CONTRA_RAM_ENEMY_Y_POS + x] + e[2] +
                                  (((unsigned)ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] + e[0] +
                                    (bsum >> 8u)) >> 8u);
            const uint8_t blank_y = (uint8_t)ysum;

            if (!contra_rom_level5_draw_nametable_supertile(core, blank_x, blank_y, 0x9Bu))
            {
                return; /* retry next frame */
            }
        }
        contra_rom_create_explosion_at(
            core,
            (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + e[3]),
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + e[4]));
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x04u;
    }
}

/* ===== level-5 boss UFO (0x14) + flying saucer (0x15) + drop bomb (0x16),
   bank0:6652-7160. The carrier is pure background super-tiles: it fades in at
   a random spot, opens its blue top, spawns saucers/bombs on a cadence,
   closes, blanks itself and teleports -- all on the graphics budget. ====== */
static int contra_rom_generate_enemy_at_offset_slot(ContraCore *core, uint8_t gen_slot,
                                                    uint8_t type, uint8_t x_off, uint8_t y_off);

static const uint8_t contra_boss_ufo_x_pos_tbl[4] = {0x40u, 0x60u, 0x80u, 0x80u};
static const uint8_t contra_boss_ufo_supertile_tbl[4] = {0x0Du, 0x0Eu, 0x07u, 0x08u};
/* relative {x,y} of the four carrier quadrants */
static const uint8_t contra_boss_ufo_pos_tbl[8] = {
    0xE4u, 0xE4u, 0x04u, 0xE4u, 0xE4u, 0x04u, 0x04u, 0x04u};
/* {right,left} pairs for the top-bay animation, index = ENEMY_VAR_1 */
static const uint8_t contra_boss_ufo_top_tbl[8] = {
    0x0Bu, 0x05u, 0x0Au, 0x04u, 0x09u, 0x03u, 0x0Du, 0x0Eu};
/* {left,right, half-left,half-right} thruster super-tiles */
static const uint8_t contra_boss_ufo_thruster_tbl[4] = {0x07u, 0x08u, 0x0Cu, 0x06u};
/* {type, rel x, x fast vel, sprite} per generation beat */
static const uint8_t contra_boss_ufo_gen_tbl[16] = {
    0x15u, 0x14u, 0x01u, 0x7Cu,
    0x16u, 0x00u, 0x00u, 0x22u,
    0x15u, 0xECu, 0xFEu, 0x7Cu,
    0x16u, 0x00u, 0x00u, 0x22u};
static const uint8_t contra_boss_ufo_explosion_pos_tbl[10] = {
    0xE0u, 0x00u, 0x00u, 0x20u, 0x20u, 0x00u, 0xF0u, 0xF0u, 0x00u, 0x00u};
/* {x, y, supertile} for the boss-door blow-open after the defeat */
static const uint8_t contra_boss_ufo_door_tbl[9] = {
    0xC0u, 0x80u, 0x96u, 0xC0u, 0xA0u, 0x97u, 0xD0u, 0xC0u, 0x98u};

/* boss_ufo_draw_supertile_a (bank0:6700): draw super-tile `code` at quadrant
   pos_idx, then unconditionally count ENEMY_ANIMATION_DELAY down; when it
   underflows, prep the top-bay animation (VAR_1=2, delay 0x60) and advance. */
static void contra_rom_boss_ufo_draw_supertile_a(
    ContraCore *core, uint8_t x, uint8_t code, uint8_t pos_idx)
{
    uint8_t *const ram = core->ram;
    const uint8_t px = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] +
                                 contra_boss_ufo_pos_tbl[pos_idx * 2u]);
    const uint8_t py = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] +
                                 contra_boss_ufo_pos_tbl[pos_idx * 2u + 1u]);

    (void)contra_rom_level5_draw_nametable_supertile(core, px, py, code);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] & 0x80u) == 0u)
    {
        return; /* more quadrants to draw */
    }
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x02u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x60u);
}

/* boss_ufo_draw_blue_top (bank0:6760): two top quadrants from the bay table. */
static void contra_rom_boss_ufo_draw_blue_top(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t idx = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x03u);

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    contra_rom_boss_ufo_draw_supertile_a(core, x, contra_boss_ufo_top_tbl[idx * 2u], 0u);
    contra_rom_boss_ufo_draw_supertile_a(core, x, contra_boss_ufo_top_tbl[idx * 2u + 1u], 1u);
}

/* boss_ufo_draw_thrusters (bank0:6843): on (delay & 7) == 3 frames, redraw the
   bottom quadrants alternating full/half throttle by delay bit 3; the delay is
   parked at 2 during the two draws so they can never advance the routine. */
static void contra_rom_boss_ufo_draw_thrusters(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t saved = ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x];
    uint8_t idx;

    if ((saved & 0x07u) != 0x03u)
    {
        return;
    }
    idx = ((saved & 0x08u) != 0u) ? 0u : 2u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x02u;
    contra_rom_boss_ufo_draw_supertile_a(core, x, contra_boss_ufo_thruster_tbl[idx], 2u);
    contra_rom_boss_ufo_draw_supertile_a(core, x, contra_boss_ufo_thruster_tbl[idx + 1u], 3u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = saved;
}

static void contra_rom_boss_ufo_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0x10u;
    ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x10u; /* fade-in window */
    contra_load_palettes_color_to_cpu(core, 0x10u);
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_boss_ufo_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t y;

    ram[CONTRA_RAM_ENEMY_X_POS + x] =
        contra_boss_ufo_x_pos_tbl[ram[CONTRA_RAM_RANDOM_NUM] & 0x03u];
    y = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x20u);
    if (y >= 0x71u)
    {
        y = 0x30u;
    }
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = y;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x03u); /* quadrant index */
}

static void contra_rom_boss_ufo_routine_02(ContraCore *core, uint8_t x)
{
    const uint8_t idx = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] & 0x03u);

    contra_rom_boss_ufo_draw_supertile_a(
        core, x, contra_boss_ufo_supertile_tbl[idx], idx);
}

static void contra_rom_boss_ufo_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        contra_rom_boss_ufo_draw_thrusters(core, x);
        return;
    }
    contra_rom_enable_enemy_collision(core, x);
    contra_rom_boss_ufo_draw_blue_top(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x80u) == 0u)
    {
        return;
    }
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x00u);
}

/* boss_ufo_routine_04 (bank0:6790): the 0x100-tick attack window (delay wraps
   0x00->0xFF): every 16 ticks spawn from the generation table (beat picked by
   delay bits 4-5), thrusters animate between beats. */
static void contra_rom_boss_ufo_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t delay;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    delay = ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x];
    if (delay == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x01u;
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u);
        return;
    }
    if ((delay & 0x0Fu) != 0u)
    {
        contra_rom_boss_ufo_draw_thrusters(core, x);
        return;
    }
    {
        const uint8_t off = (uint8_t)((delay >> 2u) & 0x0Cu);
        const uint8_t *const e = &contra_boss_ufo_gen_tbl[off];
        const int slot = contra_rom_generate_enemy_at_offset_slot(
            core, x, e[0], e[1], 0xF4u);

        if (slot < 0)
        {
            return;
        }
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + slot] = e[2];
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + slot] = 0x10u;
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] = 0x02u;
        if ((off & 0x04u) == 0u)
        {
            ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + slot] = 0x80u; /* saucer .5 */
        }
        ram[CONTRA_RAM_ENEMY_SPRITES + slot] = e[3];
    }
}

static void contra_rom_boss_ufo_routine_05(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        contra_rom_boss_ufo_draw_thrusters(core, x);
        return;
    }
    contra_rom_boss_ufo_draw_blue_top(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0x04u)
    {
        return;
    }
    contra_rom_disable_enemy_collision(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x20u);
}

static void contra_rom_boss_ufo_routine_06(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        contra_rom_boss_ufo_draw_thrusters(core, x);
        return;
    }
    ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x18u; /* black-out + fade back in */
    contra_load_palettes_color_to_cpu(core, 0x10u);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x03u);
}

static void contra_rom_boss_ufo_routine_07(ContraCore *core, uint8_t x)
{
    contra_rom_boss_ufo_draw_supertile_a(
        core, x, 0x9Bu, (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] & 0x03u));
}

static void contra_rom_boss_ufo_routine_08(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u); /* back to routine_01 */
}

static void contra_rom_boss_ufo_routine_09(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_init_apu_channels(core);
    contra_play_sound(core, 0x55u);
    contra_rom_disable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x04u;
    contra_rom_destroy_all_enemies(core, (int)x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x10u);
}

static void contra_rom_boss_ufo_routine_0a(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }
    if ((ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x80u) != 0u)
    {
        /* all five blast points done -> the door blow-open */
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x02u;
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x04u);
        return;
    }
    {
        const uint8_t idx = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] * 2u);

        ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(
            ram[CONTRA_RAM_ENEMY_X_POS + x] + contra_boss_ufo_explosion_pos_tbl[idx]);
        ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(
            ram[CONTRA_RAM_ENEMY_Y_POS + x] + contra_boss_ufo_explosion_pos_tbl[idx + 1u]);
        (void)contra_rom_level5_draw_nametable_supertile(
            core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x], 0x9Bu);
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        contra_rom_create_explosion_at(
            core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x]);
    }
}

static void contra_rom_boss_ufo_routine_0b(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }
    if ((ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x80u) != 0u)
    {
        /* level_boss_defeated + set_delay_remove_enemy(0x30) */
        contra_play_sound(core, 0xFFu); /* the ROM's stale A -- the US version's
                                           level_boss_defeated quirk */
        ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0xFFu;
        ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
        ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x30u;
        ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x00u;
        contra_rom_remove_enemy(core, x);
        return;
    }
    {
        const uint8_t *const e =
            &contra_boss_ufo_door_tbl[(size_t)ram[CONTRA_RAM_ENEMY_VAR_1 + x] * 3u];

        ram[CONTRA_RAM_ENEMY_X_POS + x] = e[0];
        ram[CONTRA_RAM_ENEMY_Y_POS + x] = e[1];
        if (!contra_rom_level5_draw_nametable_supertile(core, e[0], e[1], e[2]))
        {
            return; /* retry next frame */
        }
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        contra_rom_create_explosion_at(core, e[0], e[1]);
    }
}

/* update_enemy_x_pos_rem_off_screen / set_enemy_y_vel_rem_off_screen
   (bank7:7780/7796): one-axis sub-pixel moves with the screen-edge removal. */
static bool contra_rom_update_enemy_x_pos_rem_off_screen(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const unsigned f = (unsigned)ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] +
                       ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x];

    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] = (uint8_t)f;
    ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(
        ram[CONTRA_RAM_ENEMY_X_POS + x] + ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] + (f >> 8u));
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
    {
        contra_rom_remove_enemy(core, x);
        return false;
    }
    return true;
}

static bool contra_rom_set_enemy_y_vel_rem_off_screen(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const unsigned f = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] +
                       ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x];

    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = (uint8_t)f;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(
        ram[CONTRA_RAM_ENEMY_Y_POS + x] + ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (f >> 8u));
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xE8u)
    {
        contra_rom_remove_enemy(core, x);
        return false;
    }
    return true;
}

/* set_mini_ufo_sprite (bank0:7100): spin sprites 0x7C..0x7E every 4 ticks. */
static void contra_rom_set_mini_ufo_sprite(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] & 0x03u) != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_SPRITES + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_SPRITES + x] >= 0x7Fu)
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x7Cu;
    }
}

static void contra_rom_mini_ufo_tick_sprite(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    contra_rom_set_mini_ufo_sprite(core, x);
}

static void contra_rom_mini_ufo_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    contra_rom_set_mini_ufo_sprite(core, x);
}

static void contra_rom_mini_ufo_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_mini_ufo_tick_sprite(core, x);
    if (!contra_rom_update_enemy_x_pos_rem_off_screen(core, x))
    {
        return;
    }
    if ((ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0x20u) &&
        (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0xE0u))
    {
        return; /* not yet at a descent edge */
    }
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x80u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0x01u; /* descend at 1.5 */
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_mini_ufo_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_mini_ufo_tick_sprite(core, x);
    if (!contra_rom_set_enemy_y_vel_rem_off_screen(core, x))
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0xA8u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0xA9u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] =
        ((ram[CONTRA_RAM_ENEMY_X_POS + x] & 0x80u) == 0u) ? 0x01u : 0xFEu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x80u;
    contra_rom_set_enemy_y_velocity_to_0(core, x);
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_mini_ufo_routine_03(ContraCore *core, uint8_t x)
{
    contra_rom_mini_ufo_tick_sprite(core, x);
    contra_rom_update_enemy_pos(core, x);
}

/* boss_ufo_bomb_routine_00 (bank0:7128): fall under gravity, blow at y 0xB0. */
static void contra_rom_boss_ufo_bomb_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_a_to_enemy_y_fract_vel(core, x, 0x28u);
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0xB0u)
    {
        contra_rom_update_enemy_pos(core, x);
        return;
    }
    contra_rom_advance_enemy_routine(core, x); /* -> init_explosion */
}
