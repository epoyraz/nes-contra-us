/* Level 1 jungle enemy routines and fortress boss-door behavior.
   Included by core.c; not compiled as a separate translation unit. */

/* --- boss bomb turret (enemy type 0x10), bank0.asm --- */
/* super-tile per (recoil state VAR_1: 0 idle / 2 firing) and background variant
   (attr bit0: wall vs jungle), interleaved. */
static const uint8_t contra_boss_bomb_turret_supertile_tbl[6] = {
    0x29u, 0x26u, 0x2Au, 0x27u, 0x2Bu, 0x28u};
static const uint8_t contra_boss_bomb_turret_bomb_velocity_tbl[4] = {0x01u, 0x03u, 0x05u, 0x07u};

/* draw_boss_bomb_turret_y (bank0:2134-2169): draw bomb-turret super-tile at
   index y, jungle bg variant. */
static void contra_rom_draw_boss_bomb_turret_y(ContraCore *core, uint8_t x, uint8_t idx)
{
    uint8_t *const ram = core->ram;
    int draw_x = (int)ram[CONTRA_RAM_ENEMY_X_POS + x];
    uint8_t supertile;

    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u)
    {
        idx = (uint8_t)(idx + 1u); /* jungle background variant */
        draw_x -= 8;               /* jungle super-tile sits 8px left of the enemy */
    }
    supertile = contra_boss_bomb_turret_supertile_tbl[(idx < 6u) ? idx : 0u];
    contra_render_level_1_nametable_update_supertile(
        core, draw_x, (int)ram[CONTRA_RAM_ENEMY_Y_POS + x], supertile);
    contra_cache_level_1_supertile(
        core, x, draw_x, (int)ram[CONTRA_RAM_ENEMY_Y_POS + x], supertile);
}

static void contra_rom_draw_boss_bomb_turret(ContraCore *core, uint8_t x)
{
    contra_rom_draw_boss_bomb_turret_y(core, x, core->ram[CONTRA_RAM_ENEMY_VAR_1 + x]);
}

/* boss_bomb_turret_routine_00/01 (bank0): after a startup delay, alternate
   idle/recoil super-tiles and lob a bomb (a regular type-0 enemy bullet at a
   fixed up-left angle with a random speed) on each firing beat. */
/* boss_bomb_turret_routine_00 (bank0:2093-2096): set attack delay 0x20, advance routine. */
static void contra_rom_boss_bomb_turret_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x20u;
    contra_rom_advance_enemy_routine(core, x);
}

/* boss_bomb_turret_routine_01 (bank0:2101-2128): recoil-animate and lob bomb on firing beat. */
static void contra_rom_boss_bomb_turret_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t was_firing;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    contra_rom_draw_boss_bomb_turret(core, x);
    was_firing = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = (was_firing == 0u) ? 0x28u : 0x08u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(was_firing ^ 0x02u);
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u)
    {
        return; /* recoil frame only, no bomb */
    }
    /* lda #$17; jmp bullet_generation -- the asl inside bullet_generation turns
       #$17 into #$2E, so the bomb is bullet type 1 (large cannonball, sprite
       $21) fired up-left, not a raw type-0 regular bullet. */
    contra_rom_bullet_generation(
        core, 0x17u,
        contra_boss_bomb_turret_bomb_velocity_tbl[ram[CONTRA_RAM_RANDOM_NUM] & 0x03u],
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 0xF8u),
        ram[CONTRA_RAM_ENEMY_Y_POS + x]);
}

/* boss_bomb_turret_routine_02 (bank0:2173-2178), the destroyed routine (set by
   the level-1 nibble table for type 0x10): draw the destroyed-turret super-tile
   (index 4) and advance into the appended explosion trio. */
static void contra_rom_boss_bomb_turret_routine_02(ContraCore *core, uint8_t x)
{
    contra_rom_draw_boss_bomb_turret_y(core, x, 0x04u);
    contra_rom_advance_enemy_routine(core, x);
}

/* --- red turret (enemy type 0x07), bank0.asm:973 --- */
/* red_turret_supertile_1_tbl flows into _2_tbl (11 bytes): emerge frames 0..3
   then the rotation/active frames. */
static const uint8_t contra_red_turret_supertile_tbl[11] = {
    0x16u, 0x14u, 0x18u, 0x11u, 0x17u, 0x15u, 0x18u, 0x11u, 0x11u, 0x12u, 0x13u};

static bool contra_rom_red_turret_load_supertile(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t idx;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return false; /* delay not elapsed -- nothing drawn this frame */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x04u;
    idx = ram[CONTRA_RAM_ENEMY_FRAME + x];
    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u)
    {
        idx = (uint8_t)(idx + 3u); /* alternate background variant */
    }
    {
        const uint8_t supertile = contra_red_turret_supertile_tbl[(idx < 11u) ? idx : 0u];

        contra_render_level_1_nametable_update_supertile(
            core,
            (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
            (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
            supertile);
        contra_cache_level_1_supertile(
            core,
            x,
            (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
            (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
            supertile);
    }
    return true;
}

/* red_turret_routine_00..02 (bank0): aim left, wait for the player to approach,
   then emerge (super-tile animation) and become collidable. The active rotating
   aim + firing + retract (routine_03..05) follow further down, after the shared
   rotating-aim helpers they reuse. */
/* red_turret_routine_00 (bank0:987-991): set aim direction left, advance routine. */
static void contra_rom_red_turret_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x06u; /* face left */
    contra_rom_advance_enemy_routine(core, x);
    /* red_turret_adv_routine FALLS THROUGH into add_scroll_to_enemy_pos
       (bank0:1008): this routine scrolls TWICE on its frame. */
    contra_rom_add_scroll_to_enemy_pos(core, x);
}

/* red_turret_routine_01 (bank0:995-1009): wait for player to approach, then advance. */
static void contra_rom_red_turret_routine_01(ContraCore *core, uint8_t x)
{
    if (!contra_rom_past_trigger_x(core, x, 0xF0u, 0x40u))
    {
        contra_rom_add_scroll_to_enemy_pos(core, x);
        return; /* player not close yet */
    }
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
    contra_rom_advance_enemy_routine(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x); /* the shared fall-through */
}

/* red_turret_routine_02 (bank0:1012-1050): emerge super-tile animation, enable collision. */
static void contra_rom_red_turret_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (!contra_rom_red_turret_load_supertile(core, x))
    {
        contra_rom_add_scroll_to_enemy_pos(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x04u)
    {
        contra_rom_add_scroll_to_enemy_pos(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x02u;
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (ram[CONTRA_RAM_GAME_COMPLETION_COUNT] != 0u) ? 0x08u : 0x28u;
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu); /* enable bullet collision */
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x10u;
    contra_rom_advance_enemy_routine(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x); /* red_turret_adv_routine falls into add_scroll */
}

/* red_turret_routine_03/04/05 (active rotate-and-fire, retract, restore) are
   defined after the shared rotating-aim helpers below, since they reuse them. */
static void contra_rom_red_turret_routine_03(ContraCore *core, uint8_t x);
static void contra_rom_red_turret_routine_04(ContraCore *core, uint8_t x);
static void contra_rom_red_turret_routine_05(ContraCore *core, uint8_t x);

/* --- rotating gun (enemy type 0x04), bank0.asm:742-970 --- */

/* quadrant_aim_dir_00 (bank7:10545): within-quadrant aim nibble for a quadrant
   split into 3 parts [#$00-#$03]; indexed [row = |dy|>>5][col = |dx|>>6], high
   nibble used when bit5 of |dx| is clear, low nibble when set. Used by the
   rotating gun's incremental aim (the 12-direction emerge/rotate). */
static const uint8_t contra_quadrant_aim_dir_00[32] = {
    0x00u, 0x00u, 0x00u, 0x00u, 0x32u, 0x11u, 0x00u, 0x00u,
    0x32u, 0x11u, 0x11u, 0x11u, 0x32u, 0x22u, 0x11u, 0x11u,
    0x33u, 0x22u, 0x11u, 0x11u, 0x33u, 0x22u, 0x22u, 0x11u,
    0x33u, 0x22u, 0x22u, 0x11u, 0x33u, 0x22u, 0x22u, 0x22u};

/* player_enemy_x_dist (bank7:8844 + lda_closer_distance:8885): the index (0/1) of
   the closest normal-state player by |X distance|; non-normal players are pushed
   to max distance (0xFE for p1, 0xFF for p2) so they're never chosen, and a tie
   resolves to player 1. */
static uint8_t contra_rom_player_enemy_x_dist_idx(const ContraCore *core, uint8_t x)
{
    const uint8_t *const ram = core->ram;
    const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t p0 = ram[CONTRA_RAM_SPRITE_X_POS + 0u];
    const uint8_t p1 = ram[CONTRA_RAM_SPRITE_X_POS + 1u];
    uint8_t d0 = (p0 >= ex) ? (uint8_t)(p0 - ex) : (uint8_t)(ex - p0);
    uint8_t d1 = (p1 >= ex) ? (uint8_t)(p1 - ex) : (uint8_t)(ex - p1);

    if (ram[CONTRA_RAM_PLAYER_STATE + 0u] != 0x01u)
    {
        d0 = 0xFEu;
    }
    if (ram[CONTRA_RAM_PLAYER_STATE + 1u] != 0x01u)
    {
        d1 = 0xFFu;
    }
    return (d1 < d0) ? 1u : 0u;
}

/* sniper_routine_02 firing (bank0.asm:1848-1945). The standing sniper (attr 0)
   and boss sniper (attr 2) solve the real aim direction toward the closest player
   via quadrant_aim_dir_01 (get_rotate_01); the crouching sniper (attr 1) fires a
   fixed horizontal shot. aim_at_player returns the within-quadrant aim nibble plus
   the aim quadrant ($07: bit0 = player above, bit1 = player left) -- the same pair
   the boss eye projectile fires with -- which together drive the bullet velocity.
   The full 24-step aim direction (reconstructed by get_rotate_dir) only selects
   the vertical aim band for the muzzle sprite and the bullet spawn offset. */
static void contra_rom_sniper_fire_bullet(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t ey = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    const uint8_t pidx = contra_rom_player_enemy_x_dist_idx(core, x);
    const uint8_t player_x = ram[CONTRA_RAM_SPRITE_X_POS + pidx];
    /* VAR_2: 0 = player to the left of the sniper, 1 = player to the right. */
    const uint8_t firing_right = (player_x >= ex) ? 1u : 0u;
    uint8_t nibble;
    uint8_t quadrant;
    uint8_t dir;
    uint8_t adj;
    uint8_t yidx;
    uint8_t xoff;

    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = firing_right;

    if ((attr & 0x01u) != 0u)
    {
        /* crouching sniper: fixed horizontal shot (nibble 0 = straight along X). */
        nibble = 0x00u;
        quadrant = (firing_right != 0u) ? 0x00u : 0x02u;
    }
    else
    {
        /* standing / boss sniper: aim at the player. The boss sniper aims from
           16px above its position (ROM ldy #$f0 before add_with_enemy_pos). */
        const uint8_t sy = (attr == 0x02u) ? (uint8_t)(ey + 0xF0u) : ey;

        nibble = contra_rom_aim_at_player(core, ex, sy, contra_quadrant_aim_dir_01, &quadrant);
    }

    /* get_rotate_dir (bank7:10236): fold the within-quadrant nibble + quadrant back
       into the absolute 24-step aim direction (quadrant_aim_dir_01: mid 0x0c, max
       0x18). */
    dir = nibble;
    if ((quadrant & 0x02u) != 0u) { dir = (uint8_t)(0x0Cu - dir); } /* player left */
    if ((quadrant & 0x01u) != 0u)                                   /* player above */
    {
        dir = (uint8_t)(0x18u - dir);
        if (dir >= 0x18u) { dir = 0x00u; }
    }

    /* @adjust_bullet_angle: fold the direction into a 0..0x0c half-circle to pick
       the vertical aim band (0 = up, 1 = straight, 2 = down). */
    adj = (uint8_t)((dir + 0x06u) % 0x18u);
    if (adj >= 0x0Cu)
    {
        adj = (uint8_t)(0x18u - adj);
    }
    yidx = (adj < 0x05u) ? 0u : ((adj < 0x08u) ? 1u : 2u);

    /* standing / boss sniper set the firing muzzle sprite; the crouching sniper
       keeps its crouch sprite. */
    if ((attr & 0x01u) == 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = contra_sniper_standing_sprite_tbl[yidx];
    }

    xoff = contra_sniper_bullet_x_offset[yidx];
    if (firing_right != 0u)
    {
        xoff = (uint8_t)(0u - xoff); /* mirror the muzzle offset to the right */
    }

    /* create_enemy_bullet_if_attack_enabled: regular bullets require the flag. */
    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        return;
    }
    if (contra_rom_create_enemy_bullet(
            core, 0u, nibble, quadrant,
            contra_sniper_bullet_speed[(attr < 3u) ? attr : 0u],
            (uint8_t)(ex + xoff),
            (uint8_t)(ey + contra_sniper_bullet_y_offset[yidx])))
    {
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x06u; /* gun recoil */
    }
}

/* get_quadrant_aim_dir_for_player (bank7:10425): pick the within-quadrant aim
   nibble from source (sx,sy) to player_idx; if that player isn't in a normal
   state try the other, and if neither is, aim at screen center-bottom
   (0x80,0xFF). Indoor levels target a fixed player Y (0xB0). *quadrant returns
   $07 (bit0 = player above, bit1 = player left). */
static uint8_t contra_rom_get_quadrant_aim_dir_for_player(
    ContraCore *core, uint8_t sx, uint8_t sy, uint8_t player_idx,
    const uint8_t *tbl, uint8_t *quadrant)
{
    uint8_t *const ram = core->ram;
    uint8_t idx = (uint8_t)(player_idx & 0x01u);
    uint8_t tx;
    uint8_t ty;

    if (ram[CONTRA_RAM_PLAYER_STATE + idx] != 0x01u)
    {
        idx ^= 0x01u;
    }
    if (ram[CONTRA_RAM_PLAYER_STATE + idx] != 0x01u)
    {
        ty = 0xFFu;
        tx = 0x80u;
    }
    else
    {
        tx = ram[CONTRA_RAM_SPRITE_X_POS + idx];
        /* get_quadrant_aim_dir_for_player (bank7:10541) tests location with
           `lsr` -- bit 0 only: indoor rooms (0x01) aim at the fixed 0xB0, but
           the BOSS SCREEN (0x80) takes the outdoor branch and aims at the
           player's real Y. */
        ty = ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x01u) != 0u)
            ? 0xB0u
            : ram[CONTRA_RAM_SPRITE_Y_POS + idx];
    }
    return contra_rom_get_quadrant_aim_dir(sx, sy, tx, ty, tbl, quadrant);
}

/* get_rotate_dir (bank7:10236): given a within-quadrant aim nibble and the
   quadrant ($07), reflect it across the quadrant boundaries into the full
   direction wheel ($0c, target_out), and pick the shortest rotation from the
   current ENEMY_VAR_1. Returns 0x00 = clockwise, 0x01 = counter-clockwise,
   0x80 = already aimed. table_idx selects the wheel size: 0/2 -> 12 directions
   (midway 6, max 0x0c), 1 -> 24 directions (midway 0x0c, max 0x18). */
static uint8_t contra_rom_get_rotate_dir(
    ContraCore *core, uint8_t x, uint8_t aim, uint8_t quadrant, uint8_t table_idx,
    uint8_t *target_out)
{
    const uint8_t var1 = core->ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    const uint8_t midway = ((table_idx & 0x01u) == 0u) ? 0x06u : 0x0Cu;
    const uint8_t max_dir = ((table_idx & 0x01u) == 0u) ? 0x0Cu : 0x18u;
    uint8_t target = aim;
    uint8_t reflected;
    uint8_t wrapped = 0u;
    uint8_t tmp;

    if ((quadrant & 0x02u) != 0u)
    {
        target = (uint8_t)(midway - target); /* player to the left: reflect horizontally */
    }
    if ((quadrant & 0x01u) != 0u)
    {
        tmp = (uint8_t)(max_dir - target); /* player above: reflect across the x-axis */
        target = (tmp < max_dir) ? tmp : 0u;
    }
    *target_out = target;

    /* $0d/$0e: the current aim reflected past the midway, and whether it wrapped */
    tmp = (uint8_t)(var1 + midway);
    if (tmp >= max_dir)
    {
        wrapped = 1u;
        tmp = (uint8_t)(tmp - max_dir);
    }
    reflected = tmp;

    if (target == var1)
    {
        return 0x80u; /* already aiming there */
    }
    if (wrapped == 0u)
    {
        if (target < var1)
        {
            return 0x01u; /* counter-clockwise */
        }
        return (target >= reflected) ? 0x01u : 0x00u;
    }
    if (target >= var1)
    {
        return 0x00u; /* clockwise */
    }
    return (target < reflected) ? 0x00u : 0x01u;
}

/* aim_var_1_for_quadrant_aim_dir_00 (bank7:10128) + rotate_enemy_var_1 (10136):
   step ENEMY_VAR_1 one direction toward the player and return true when it has
   reached the target (the ROM's carry-set "aiming at player" result). */
static bool contra_rom_aim_var_1(ContraCore *core, uint8_t x, uint8_t table_idx, uint8_t player_idx)
{
    uint8_t *const ram = core->ram;
    const uint8_t max_dir = ((table_idx & 0x01u) == 0u) ? 0x0Cu : 0x18u;
    const uint8_t *tbl = (table_idx == 0u) ? contra_quadrant_aim_dir_00 : contra_quadrant_aim_dir_01;
    uint8_t quadrant = 0u;
    uint8_t target = 0u;
    uint8_t rot;
    uint8_t v;

    rot = contra_rom_get_quadrant_aim_dir_for_player(
        core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
        player_idx, tbl, &quadrant);
    rot = contra_rom_get_rotate_dir(core, x, rot, quadrant, table_idx, &target);

    if ((rot & 0x80u) != 0u)
    {
        return true; /* no rotation needed -- already aimed */
    }
    v = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    if (rot != 0u)
    {
        v = (uint8_t)(v - 1u); /* counter-clockwise */
        if ((v & 0x80u) != 0u)
        {
            v = (uint8_t)(max_dir - 1u); /* underflow: wrap to the top */
        }
    }
    else
    {
        v = (uint8_t)(v + 1u); /* clockwise */
        if (v >= max_dir)
        {
            v = 0u; /* wrap back to direction 0 */
        }
    }
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = v;
    return (v == target);
}

/* bullet_generation (bank7:9778): asl the 12-direction aim into the 24-step
   bullet angle, then create_enemy_bullet_if_attack_enabled -- type 0x20 (the
   level-1 boss cannonball) always fires, everything else only when
   ENEMY_ATTACK_FLAG is set. */
static void contra_rom_bullet_generation(
    ContraCore *core, uint8_t aim, uint8_t speed, uint8_t px, uint8_t py)
{
    const uint8_t type_angle = (uint8_t)(aim << 1u);

    if (((type_angle & 0xE0u) != 0x20u) && (core->ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u))
    {
        return;
    }
    contra_rom_create_enemy_bullet_angle_a(core, type_angle, speed, px, py);
}

/* draw_enemy_supertile_a_set_delay (bank7:8527): draw the level-1 background
   super-tile at the enemy and set ANIMATION_DELAY=1 (the native draw never
   fails, so the buffer-full retry path is unreachable). */
static void contra_rom_draw_enemy_supertile_a_set_delay(ContraCore *core, uint8_t x, uint8_t supertile)
{
    /* draw_enemy_supertile_a_set_delay (bank7:8599) sets ANIMATION_DELAY = 1
       only when the draw FAILS (CPU graphics buffer full -> retry next frame).
       The native renderer cannot fail, so the caller's delay always stands --
       the port used to write 1 unconditionally here, which made the rotating
       gun open and rotate every frame instead of on its 8/0x30-frame beats. */
    contra_render_level_1_nametable_update_supertile(
        core, (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x], supertile);
    contra_cache_level_1_supertile(
        core,
        x,
        (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        supertile);
}

static const uint8_t contra_rotating_gun_bullets_per_attack_tbl[4] = {0x01u, 0x02u, 0x03u, 0x03u};
static const uint8_t contra_rotating_gun_rotation_delay_tbl[4] = {0x30u, 0x28u, 0x20u, 0x18u};
static const uint8_t contra_rotating_gun_animation_delay_tbl[4] = {0x80u, 0x60u, 0x40u, 0x30u};
/* rotating_gun_bullet_y_offset_tbl (3 bytes) flows directly into
   rotating_gun_bullet_x_offset_tbl (12 bytes) in the ROM, so the firing code
   reads y_offset = tbl[aim] and x_offset = tbl[aim+3] from this 15-byte block. */
static const uint8_t contra_rotating_gun_bullet_offset_tbl[15] = {
    0x00u, 0x07u, 0x0Cu, 0x0Du, 0x0Cu, 0x07u, 0x00u, 0xF9u,
    0xF4u, 0xF3u, 0xF4u, 0xF9u, 0x00u, 0x07u, 0x0Cu};

/* rotating_gun_should_disable (bank0:865): track scroll, then report whether the
   gun has scrolled into the left 10% of the screen (X < 0x18) and should retract. */
static bool contra_rom_rotating_gun_should_disable(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    return contra_rom_past_trigger_x(core, x, 0x18u, 0xC8u);
}

/* rotating_gun_disable (bank0:811): retract by jumping to routine_05 (ROUTINE 6),
   guarding the removed-by-scroll case like the ROM's set_enemy_routine_to_a. */
static void contra_rom_rotating_gun_disable(ContraCore *core, uint8_t x)
{
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] != 0u)
    {
        contra_rom_set_enemy_routine_to_a(core, x, 0x06u);
    }
}

/* rotating_gun_routine_00 (bank0:756): face left, advance. */
static void contra_rom_rotating_gun_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x06u; /* aim direction = left */
    contra_rom_advance_enemy_routine(core, x);
}

/* rotating_gun_routine_01 (bank0:764): wait until the gun has scrolled past the
   activation trigger (X < 0xF0), then start the opening animation. */
static void contra_rom_rotating_gun_routine_01(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (!contra_rom_past_trigger_x(core, x, 0xF0u, 0x30u))
    {
        return; /* not yet at the activation point */
    }
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u); /* -> routine_02 */
}

/* rotating_gun_routine_02 (bank0:784): play the 3-frame opening animation (gun
   emerges from the wall), then become bullet-collidable and advance to aim. */
static void contra_rom_rotating_gun_routine_02(ContraCore *core, uint8_t x)
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
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u; /* next opening step in 8 frames */
    contra_rom_draw_enemy_supertile_a_set_delay(
        core, x, (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 0x03u));
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x03u)
    {
        return; /* gun not fully open yet */
    }
    contra_rom_enable_bullet_collision(core, x); /* STATE_WIDTH &= 0x7F */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u); /* -> routine_03 */
}

/* rotating_gun_routine_03 (bank0:805): retract if scrolled off; otherwise rotate
   the gun one step toward the player each beat, and when it lines up load the
   per-attribute burst count and advance to fire. */
static void contra_rom_rotating_gun_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    bool aimed;
    uint8_t supertile;
    uint8_t idx;

    if (contra_rom_rotating_gun_should_disable(core, x))
    {
        contra_rom_rotating_gun_disable(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        contra_rotating_gun_rotation_delay_tbl[ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] & 0x03u];
    aimed = contra_rom_aim_var_1(core, x, 0u, contra_rom_player_enemy_x_dist_idx(core, x));
    /* draw the gun super-tile for the new aim direction: ((VAR_1 + 6) % 12) + 5 */
    supertile = (uint8_t)(((ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 0x06u) % 12u) + 0x05u);
    contra_rom_draw_enemy_supertile_a_set_delay(core, x, supertile);
    if (!aimed)
    {
        return; /* still rotating toward the player */
    }
    idx = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x03u);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = contra_rotating_gun_bullets_per_attack_tbl[idx];
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u); /* -> routine_04 */
}

/* rotating_gun_routine_04 (bank0:873): fire VAR_2 bullets at the aimed direction
   (one per 0x10-frame beat), then return to routine_03 to re-aim. */
static void contra_rom_rotating_gun_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t aim;
    uint8_t bx;
    uint8_t by;

    if (contra_rom_rotating_gun_should_disable(core, x))
    {
        contra_rom_rotating_gun_disable(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    aim = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    by = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + contra_rotating_gun_bullet_offset_tbl[aim]);
    bx = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + contra_rotating_gun_bullet_offset_tbl[aim + 3u]);
    contra_rom_bullet_generation(core, aim, 0x04u, bx, by);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x10u; /* delay between bullets */
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        return; /* more bullets in this burst */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        contra_rotating_gun_animation_delay_tbl[ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] & 0x03u];
    contra_rom_set_enemy_routine_to_a(core, x, 0x04u); /* -> routine_03 */
}

/* rotating_gun_routine_05 (bank0:924): retract -- draw the closed super-tile (3)
   and remove the enemy. */
static void contra_rom_rotating_gun_routine_05(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    contra_render_level_1_nametable_update_supertile(
        core, (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x], 0x03u);
    contra_cache_level_1_supertile(
        core,
        x,
        (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        0x03u);
    contra_rom_remove_enemy(core, x); /* keep the cached close tile: the ROM nametable write persists */
}

/* rotating_gun_routine_06 (bank0:933): destroyed -- restore the rock background
   super-tile (0x16) then start the explosion actor (the ROM advances to
   enemy_routine_init_explosion). */
static void contra_rom_rotating_gun_routine_06(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    /* draw_enemy_supertile_a + cache the rock so the per-frame L1 redraw shows
       it; the ROM then advances into its own appended explosion routines,
       keeping ENEMY_TYPE 0x04 (not the shared 0xFE actor). */
    contra_rom_draw_enemy_supertile_a_set_delay(core, x, 0x16u);
    contra_rom_advance_enemy_routine(core, x); /* -> enemy_routine_init_explosion */
}

/* --- red turret active rotate-and-fire (enemy type 0x07), bank0.asm:1072-1199 ---
   The red turret only aims left / up-left, so ENEMY_VAR_1 is clamped to [6,8]. */
static const uint8_t contra_red_turret_supertile_2_tbl[9] = {
    0x18u, 0x11u, 0x17u, 0x15u, 0x18u, 0x11u, 0x11u, 0x12u, 0x13u};
/* red_turret_bullet_offset_tbl (bank0:1158), split into the y/x offsets the ROM
   reads through its two overlapping label views, indexed by ENEMY_VAR_1 - 6. */
static const uint8_t contra_red_turret_bullet_y_off[3] = {0x00u, 0xF8u, 0xF0u};
static const uint8_t contra_red_turret_bullet_x_off[3] = {0xF2u, 0xF2u, 0xF8u};

/* disable_enemy_collision (bank7:8371-area): ENEMY_STATE_WIDTH |= 0x81. */
static void contra_rom_disable_enemy_collision(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] | 0x81u);
}

/* |distance| from a player to the enemy along one axis, with the ROM's
   state-adjusted sentinels (0xFE for a non-normal p1, 0xFF for p2). */
static void contra_rom_axis_dists(
    const ContraCore *core, uint8_t epos, uint16_t player_base, uint8_t *d0, uint8_t *d1)
{
    const uint8_t *const ram = core->ram;
    const uint8_t p0 = ram[player_base + 0u];
    const uint8_t p1 = ram[player_base + 1u];

    *d0 = (p0 >= epos) ? (uint8_t)(p0 - epos) : (uint8_t)(epos - p0);
    *d1 = (p1 >= epos) ? (uint8_t)(p1 - epos) : (uint8_t)(epos - p1);
    if (ram[CONTRA_RAM_PLAYER_STATE + 0u] != 0x01u)
    {
        *d0 = 0xFEu;
    }
    if (ram[CONTRA_RAM_PLAYER_STATE + 1u] != 0x01u)
    {
        *d1 = 0xFFu;
    }
}

/* red_turret_find_target_player (bank7:8803): pick the player who is farther in
   the dominant axis (the ROM's curious targeting). Returns the player index. */
static uint8_t contra_rom_red_turret_find_target_player(const ContraCore *core, uint8_t x)
{
    uint8_t xd[2];
    uint8_t yd[2];
    uint8_t farther_x;
    uint8_t farther_y;

    contra_rom_axis_dists(core, core->ram[CONTRA_RAM_ENEMY_X_POS + x], CONTRA_RAM_SPRITE_X_POS, &xd[0], &xd[1]);
    farther_x = (xd[1] < xd[0]) ? 0u : 1u; /* closest_x ^ 1 */
    contra_rom_axis_dists(core, core->ram[CONTRA_RAM_ENEMY_Y_POS + x], CONTRA_RAM_SPRITE_Y_POS, &yd[0], &yd[1]);
    farther_y = (yd[1] < yd[0]) ? 0u : 1u; /* closest_y ^ 1 */
    return (yd[farther_y] < xd[farther_x]) ? farther_y : farther_x;
}

/* check_red_turret_firing_range (bank0:1189): true when the turret is below-or-
   level with the target player AND to the player's right (it fires up-left). */
static bool contra_rom_check_red_turret_firing_range(const ContraCore *core, uint8_t x, uint8_t p)
{
    const uint8_t *const ram = core->ram;

    if ((uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x20u) < ram[CONTRA_RAM_SPRITE_Y_POS + p])
    {
        return false; /* turret above the player */
    }
    return ram[CONTRA_RAM_ENEMY_X_POS + x] >= ram[CONTRA_RAM_SPRITE_X_POS + p];
}

/* get_rotate_00 (bank7:10180): rotation direction toward the player using
   quadrant_aim_dir_00 (0x00 CW, 0x01 CCW, 0x80 already aimed). */
static uint8_t contra_rom_get_rotate_00(ContraCore *core, uint8_t x, uint8_t player_idx)
{
    uint8_t quadrant = 0u;
    uint8_t target = 0u;
    const uint8_t aim = contra_rom_get_quadrant_aim_dir_for_player(
        core, core->ram[CONTRA_RAM_ENEMY_X_POS + x], core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        player_idx, contra_quadrant_aim_dir_00, &quadrant);

    return contra_rom_get_rotate_dir(core, x, aim, quadrant, 0u, &target);
}

/* red_turret_routine_03 (bank0:1072): retract once scrolled to the left edge,
   otherwise rotate the gun toward the target within [6,8] and fire 3-bullet
   bursts up-left when the player is in range. */
static void contra_rom_red_turret_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t target;
    uint8_t player_idx;
    uint8_t rot;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (contra_rom_past_trigger_x(core, x, 0x30u, 0xC0u))
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x02u; /* retract animation start */
        contra_rom_disable_enemy_collision(core, x);
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u); /* -> routine_04 */
        return;
    }
    target = contra_rom_red_turret_find_target_player(core, x);
    player_idx = contra_rom_check_red_turret_firing_range(core, x, target)
        ? target : (uint8_t)(target ^ 0x01u);
    rot = contra_rom_get_rotate_00(core, x, player_idx);

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        const uint8_t var1 = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
        bool rotated = false;

        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x10u;
        if ((rot & 0x80u) != 0u)
        {
            /* already aimed -> hold and fire */
        }
        else if (rot != 0u)
        {
            if (var1 != 0x06u) /* counter-clockwise toward "left" */
            {
                ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(var1 - 1u);
                rotated = true;
            }
        }
        else if (var1 != 0x08u) /* clockwise toward "up" */
        {
            ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(var1 + 1u);
            rotated = true;
        }
        if (rotated)
        {
            const uint8_t v = ram[CONTRA_RAM_ENEMY_VAR_1 + x];

            contra_rom_draw_enemy_supertile_a_set_delay(
                core, x, contra_red_turret_supertile_2_tbl[(v < 9u) ? v : 0u]);
        }
    }
    /* @dec_attack_delay_fire_bullet */
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x80u) != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x02u;      /* burst of 3 again */
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x50u; /* longer pause between bursts */
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x10u; /* between bullets in a burst */
    }
    if (!contra_rom_check_red_turret_firing_range(core, x, 0u)) /* fire check vs P1 ($0f=0) */
    {
        return;
    }
    {
        const uint8_t v = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
        const uint8_t vi = (uint8_t)(((v >= 6u) && (v <= 8u)) ? (v - 6u) : 0u);
        const uint8_t by = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + contra_red_turret_bullet_y_off[vi]);
        const uint8_t bx = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + contra_red_turret_bullet_x_off[vi]);

        contra_rom_bullet_generation(core, v, 0x05u, bx, by);
    }
}

/* red_turret_routine_04 (bank0:1163): play the retract animation backward, then
   remove the turret. */
static void contra_rom_red_turret_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (!contra_rom_red_turret_load_supertile(core, x))
    {
        return; /* animation delay not elapsed */
    }
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_FRAME + x] & 0x80u) == 0u)
    {
        return; /* more frames to play */
    }
    contra_rom_remove_enemy_offscreen(core, x); /* ROM remove_enemy keeps the husk */
}

/* red_turret_routine_05 (bank0:1172): destroyed -- restore the rocky/metal
   background super-tile then start the explosion actor. */
static void contra_rom_red_turret_routine_05(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    contra_rom_draw_enemy_supertile_a_set_delay(
        core, x, ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u) ? 0x17u : 0x16u);
    /* the ROM advances into its own appended explosion routines, type kept */
    contra_rom_advance_enemy_routine(core, x);
}

/* --- weapon item (enemy type 0x00), bank0.asm:144-356 --- */

/* set_weapon_item_y_vel_enemy_frame (bank7:7802): apply the Y velocity to
   ENEMY_Y_POS and advance ENEMY_FRAME by frame_incr + the position carry,
   pre-decrementing the increment when the item moves up so a normal up-step
   doesn't trip the off-screen check. Returns true when ENEMY_FRAME wrapped to 1
   (the item ran off the top/bottom and should be removed). */
static bool contra_rom_set_weapon_item_y_vel_enemy_frame(
    ContraCore *core, uint8_t x, uint8_t y_fast, uint8_t frame_incr)
{
    uint8_t *const ram = core->ram;
    unsigned acc;
    unsigned ypos;
    unsigned frame;

    if ((y_fast & 0x80u) != 0u)
    {
        frame_incr = (uint8_t)(frame_incr - 1u); /* moving up: dey */
    }
    acc = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] + ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x];
    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = (uint8_t)acc;
    ypos = (unsigned)ram[CONTRA_RAM_ENEMY_Y_POS + x] + y_fast + (acc >> 8u);
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)ypos;
    frame = (unsigned)ram[CONTRA_RAM_ENEMY_FRAME + x] + frame_incr + (ypos >> 8u);
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)frame;
    return ((uint8_t)frame == 0x01u);
}

/* set_outdoor_weapon_item_vel (bank7:7770): apply the item's velocity each frame
   in outdoor levels (horizontal = scroll-adjusted X then Y; vertical = scrolled Y
   then X) and remove it once it leaves the screen. */
static void contra_rom_set_outdoor_weapon_item_vel(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        const uint8_t yv =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + ram[CONTRA_RAM_FRAME_SCROLL]);

        if (contra_rom_set_weapon_item_y_vel_enemy_frame(core, x, yv, 0x00u))
        {
            contra_rom_clear_enemy(core, x);
            return;
        }
        contra_rom_update_enemy_x_pos(core, x);
        if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
        {
            contra_rom_clear_enemy(core, x);
        }
        return;
    }
    /* horizontal: update_enemy_x_pos_with_scroll */
    contra_rom_update_enemy_x_pos(core, x);
    ram[CONTRA_RAM_ENEMY_X_POS + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - ram[CONTRA_RAM_FRAME_SCROLL]);
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
    {
        contra_rom_clear_enemy(core, x);
        return;
    }
    if (contra_rom_set_weapon_item_y_vel_enemy_frame(
            core, x, ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x], 0x00u))
    {
        contra_rom_clear_enemy(core, x);
    }
}

/* weapon_item_check_bg_collision (bank0:293): bg collision code at ENEMY_X+dx,
   testing at Y=0x10 (or the item Y when it's the live frame and Y>=0x10). Returns
   0 when the level has no solid bg-collision (LEVEL_SOLID_BG_COLLISION_CHECK==0).*/
static uint8_t contra_rom_weapon_item_check_bg_collision(ContraCore *core, uint8_t x, uint8_t dx)
{
    uint8_t *const ram = core->ram;
    const uint8_t test_x = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + dx);
    uint8_t test_y = 0x10u;

    if (ram[CONTRA_RAM_LEVEL_SOLID_BG_COLLISION_CHECK] == 0u)
    {
        return 0u;
    }
    if ((ram[CONTRA_RAM_ENEMY_FRAME + x] == 0u) && (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0x10u))
    {
        test_y = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    }
    return contra_get_outdoor_horizontal_bg_collision(core, test_x, test_y);
}

/* check_weapon_item_collision (bank0:270): true when the item is falling (not
   ascending / not the explosion frame) and there's bg collision 8px below it. */
static bool contra_rom_check_weapon_item_collision(const ContraCore *core, uint8_t x)
{
    const uint8_t *const ram = core->ram;

    if (((ram[CONTRA_RAM_ENEMY_FRAME + x] | ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x]) & 0x80u) != 0u)
    {
        return false; /* ascending or explosion frame -- no landing check */
    }
    return contra_rom_add_y_to_y_pos_get_bg_collision(core, x, 0x08u) != 0u;
}

/* add_a_with_vert_scroll_to_enemy_y_pos (bank7:8495): snap the item Y to a 16px
   grid (accounting for VERTICAL_SCROLL) and add a. */
static void contra_rom_add_a_with_vert_scroll_to_enemy_y_pos(ContraCore *core, uint8_t x, uint8_t a)
{
    uint8_t *const ram = core->ram;
    const uint8_t scroll = (uint8_t)((ram[CONTRA_RAM_VERTICAL_SCROLL] & 0x0Fu) | 0xF0u);
    uint8_t y = (uint8_t)((uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + scroll) & 0xF0u);

    y = (uint8_t)(y - scroll);
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(y + a);
}

/* set_enemy_velocity_to_0 (bank7:7854): zero both X and Y fast/fract velocity. */
static void contra_rom_set_enemy_velocity_to_0(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0u;
}

/* add_10_to_enemy_y_fract_vel (bank7:8429): gravity -- +0x10 to the Y fractional
   velocity, carrying into the fast byte. */
static void contra_rom_add_10_to_enemy_y_fract_vel(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const unsigned f = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] + 0x10u;

    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)f;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (f >> 8u));
}

/* add_a_to_enemy_y_fract_vel (bank7.asm:8358): 16-bit add of a into the Y
   velocity (carry from the fractional byte into the fast byte). */
static void contra_rom_add_a_to_enemy_y_fract_vel(ContraCore *core, uint8_t x, uint8_t a)
{
    uint8_t *const ram = core->ram;
    const unsigned f = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] + a;

    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)f;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (f >> 8u));
}

/* weapon_item_indoor_vel_tbl (bank7:9004): {fract,fast} X velocity by far segment
   0 (far left, drift right) .. 6 (far right, drift left). */
static const uint8_t contra_weapon_item_indoor_vel_tbl[14] = {
    0xAAu, 0x00u, 0x71u, 0x00u, 0x38u, 0x00u, 0x00u, 0x00u,
    0xC8u, 0xFFu, 0x8Fu, 0xFFu, 0x56u, 0xFFu};

/* set_weapon_item_indoor_velocity (bank7:8986): X velocity from the item's far
   segment, fixed downward Y velocity (fast 1). */
static void contra_rom_set_weapon_item_indoor_velocity(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t seg = contra_rom_find_far_segment(ram[CONTRA_RAM_ENEMY_X_POS + x]);

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_weapon_item_indoor_vel_tbl[(seg * 2u) & 0x0Fu];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_weapon_item_indoor_vel_tbl[((seg * 2u) + 1u) & 0x0Fu];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0x01u;
}

/* weapon_item_sprite_code_tbl (bank0:366) is already defined above (shared with
   the invented sprite path): {R,M,F,S,L,B,Falcon} = 33,34,31,2F,32,30,4E. */

/* set_weapon_item_sprite (bank0:334): show the weapon-type sprite (invisible on
   non-live frames); the falcon flashes its palette via FRAME_COUNTER. */
static void contra_rom_set_weapon_item_sprite(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t wtype;

    if (ram[CONTRA_RAM_ENEMY_FRAME + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u; /* not the visible frame */
        return;
    }
    wtype = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x07u);
    if (wtype == 0x06u)
    {
        ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
            (uint8_t)(((ram[CONTRA_RAM_FRAME_COUNTER] >> 3u) & 0x03u) | 0x04u);
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_weapon_item_sprite_code_tbl[(wtype < 7u) ? wtype : 0u];
}

/* weapon_item_init_vel_tbl (bank0:195): {y_fract,y_fast,x_fract,x_fast} for the
   outdoor horizontal / vertical-left / vertical-right launch arcs. */
static const uint8_t contra_weapon_item_init_vel_tbl[12] = {
    0x00u, 0xFDu, 0x80u, 0x00u, 0x00u, 0xFDu, 0x40u, 0x00u, 0x00u, 0xFDu, 0xC0u, 0xFFu};

/* weapon_item_routine_00 (bank0:144): set the collision/sprite attrs, then the
   launch velocity (outdoor table) or the indoor arc setup. */
static void contra_rom_weapon_item_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t y;

    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = 0x80u;     /* bullets pass through */
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x22u; /* score 2, collision box 2 */
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x05u;
    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_ENEMY_Y_POS + x]; /* arc origin Y */
        contra_rom_set_weapon_item_indoor_velocity(core, x);
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x80u;
        ram[CONTRA_RAM_ENEMY_VAR_B + x] = 0xFDu;
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    y = 0u;
    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        y = (ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0x80u) ? 8u : 4u;
    }
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = contra_weapon_item_init_vel_tbl[y + 0u];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = contra_weapon_item_init_vel_tbl[y + 1u];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_weapon_item_init_vel_tbl[y + 2u];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_weapon_item_init_vel_tbl[y + 3u];
    contra_rom_advance_enemy_routine(core, x);
}

/* weapon_item_routine_01 (bank0:203): fall. Indoor follows the pseudo-3D arc and
   lands at Y=0xA4; outdoor applies velocity + gravity, bounces off walls, and
   lands when it hits the ground. */
static void contra_rom_weapon_item_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_set_weapon_item_sprite(core, x);
    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        const unsigned v4 = (unsigned)ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 0x12u;

        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)v4;
        ram[CONTRA_RAM_ENEMY_VAR_B + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_B + x] + (v4 >> 8u));
        contra_rom_set_enemy_falling_arc_pos(core, x);
        if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
        {
            return; /* arc carried it off-screen */
        }
        if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) != 0u)
        {
            return; /* still falling */
        }
        ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0xA4u; /* land on the indoor floor */
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    /* outdoor */
    contra_rom_set_outdoor_weapon_item_vel(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return; /* removed off-screen */
    }
    if ((ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0x20u) && contra_rom_check_weapon_item_collision(core, x))
    {
        contra_rom_add_a_with_vert_scroll_to_enemy_y_pos(core, x, 0x0Au); /* land */
        contra_rom_set_enemy_velocity_to_0(core, x);
        contra_rom_advance_enemy_routine(core, x); /* -> routine_02 */
        return;
    }
    {
        const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
        const bool moving_left = (ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] & 0x80u) != 0u;
        bool reverse;

        if (!moving_left)
        {
            reverse = (ex >= 0xE8u) ||
                ((contra_rom_weapon_item_check_bg_collision(core, x, 0x0Au) & 0x80u) != 0u);
        }
        else
        {
            reverse = (ex < 0x18u) ||
                ((contra_rom_weapon_item_check_bg_collision(core, x, 0xF6u) & 0x80u) != 0u);
        }
        if (reverse)
        {
            contra_rom_reverse_enemy_x_direction(core, x);
        }
        contra_rom_add_10_to_enemy_y_fract_vel(core, x); /* gravity */
    }
}

/* weapon_item_routine_02 (bank0:315): rest on the ground -- keep the sprite, and
   drop back to routine_01 if the ground disappears (or remove on indoor scroll).*/
static void contra_rom_weapon_item_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_set_weapon_item_sprite(core, x);
    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] != 0u)
        {
            contra_rom_clear_enemy(core, x); /* indoor room scrolled */
        }
        return;
    }
    contra_rom_set_outdoor_weapon_item_vel(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (contra_rom_check_weapon_item_collision(core, x))
    {
        return; /* still resting on the ground */
    }
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u); /* ground gone -> routine_01 */
}

/* create_explosion_sequence (bank7:8353): spawn an explosion actor at (px,py)
   in a free slot. The ROM uses a type-0x02 enemy (pill box) purely because its
   routine table has the shared init_explosion/explosion/remove trio appended at
   routines 6-8 and 9-11; state_width carries the explosion type (0x89 = the
   big two-round burst, 0x08 = small) and routine picks the starting entry. */
static void contra_rom_create_explosion_sequence(
    ContraCore *core, uint8_t px, uint8_t py, uint8_t state_width, uint8_t routine)
{
    const int slot = contra_rom_find_next_enemy_slot(core);
    uint8_t s;

    if (slot < 0)
    {
        return;
    }
    s = (uint8_t)slot;
    core->ram[CONTRA_RAM_ENEMY_TYPE + s] = 0x02u;
    contra_rom_initialize_enemy(core, s);
    core->ram[CONTRA_RAM_ENEMY_ROUTINE + s] = routine;
    core->ram[CONTRA_RAM_ENEMY_SPRITES + s] = 0x01u; /* blank until the explosion shows */
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + s] = state_width;
    core->ram[CONTRA_RAM_ENEMY_Y_POS + s] = py;
    core->ram[CONTRA_RAM_ENEMY_X_POS + s] = px;
}

/* create_two_explosion_89 (bank7:8332). */
static void contra_rom_create_explosion_at(ContraCore *core, uint8_t px, uint8_t py)
{
    contra_rom_create_explosion_sequence(core, px, py, 0x89u, 0x06u);
}

/* clear_sprite_clear_enemy_pt_3 (bank7:9060 -> clear_enemy_pt_3/pt_4): zero the
   sprite and per-slot working state but KEEP ENEMY_ATTRIBUTES, X/Y_POS, ROUTINE,
   TYPE -- used to repurpose the pill-box slot into a weapon item. */
static void contra_rom_clear_sprite_clear_enemy_pt_3(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0u;
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_A + x] = 0u;
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0u; /* = VAR_B */
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0u;
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = 0u;
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0u;
}

/* weapon_box_destroyed_supertile (bank0:663): post-destruction background tile per
   level (low bit of attrs picks the variant). */
static const uint8_t contra_weapon_box_destroyed_supertile[16] = {
    0x16u, 0x16u, 0x16u, 0x16u, 0x16u, 0x16u, 0x16u, 0x16u,
    0x19u, 0x1Au, 0x03u, 0x04u, 0x09u, 0x09u, 0x16u, 0x16u};

/* play_explosion_sound (bank0:642-658): sound $19, pop an explosion, and
   repurpose this slot into a weapon item carrying the source's weapon type
   (ATTRIBUTES & 0x07). Shared by the pill box (weapon_box_routine_04) and the
   flying capsule (flying_capsule_routine_02). */
static void contra_rom_play_explosion_sound(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_play_sound(core, 0x19u); /* enemy destroyed */
    contra_rom_create_explosion_at(core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x]);
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x07u);
    contra_rom_clear_sprite_clear_enemy_pt_3(core, x);
    ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_TYPE + x] = 0x00u; /* now a weapon item */
}

/* jumping_soldier_routine_04 (bank0:3637): a red jumping soldier that dies
   mid-room (0x64 <= x < 0x9C, attribute bit 7 clear) pops the explosion and
   becomes the weapon item it carries (attributes >> 2 -> play_explosion_sound);
   otherwise it just advances into the shared explosion tail. */
static void contra_rom_jumping_soldier_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attrs = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];

    if (((attrs & 0x02u) != 0u) &&
        (ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0x64u) &&
        (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x9Cu) &&
        ((attrs & 0x80u) == 0u))
    {
        ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = (uint8_t)(attrs >> 2u);
        contra_rom_play_explosion_sound(core, x);
        return;
    }
    contra_rom_advance_enemy_routine(core, x);
}

/* weapon_box_routine_04 (bank0:627): the pill box was destroyed -- draw the
   restored background super-tile, then drop a weapon item via play_explosion_sound. */
static void contra_rom_weapon_box_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t y;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    y = (uint8_t)(ram[CONTRA_RAM_CURRENT_LEVEL] * 2u);
    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x08u) != 0u)
    {
        y = (uint8_t)(y + 1u);
    }
    {
        const uint8_t supertile = contra_weapon_box_destroyed_supertile[y & 0x0Fu];

        contra_render_level_1_nametable_update_supertile(
            core, (int)ram[CONTRA_RAM_ENEMY_X_POS + x], (int)ram[CONTRA_RAM_ENEMY_Y_POS + x],
            supertile);
        contra_cache_level_1_supertile(
            core, x, (int)ram[CONTRA_RAM_ENEMY_X_POS + x], (int)ram[CONTRA_RAM_ENEMY_Y_POS + x],
            supertile);
    }
    contra_rom_play_explosion_sound(core, x);
}

/* flying_capsule_routine_02 (bank0:737): the weapon zeppelin was destroyed -- it
   has no background tile, so it drops the weapon item directly. */
static void contra_rom_flying_capsule_routine_02(ContraCore *core, uint8_t x)
{
    contra_rom_play_explosion_sound(core, x);
}

/* destroy_all_enemies (bank7:8096): set every live, damageable enemy to its
   destroyed routine via set_destroyed_enemy_routine -- skipping the pill box
   (0x02), flying capsule (0x03), and no-damage (HP 0xF0) enemies. The caller's
   own slot needs no exemption (keep_slot kept for the call sites' clarity):
   its destroyed nibble is <= its current routine, so the `>=`-only rule below
   leaves it running its own cascade, exactly like the ROM. */

/* set_destroyed_enemy_routine (bank7:8029): raise ENEMY_ROUTINE to the type's
   destroyed-routine nibble. The per-level tables (bank7:8097-8146) are entry
   points into ONE continuous nibble array -- a level's rows deliberately run
   into the next label's bytes. Types < 0x10 use the common (level-1) base. */
static void contra_rom_set_destroyed_enemy_routine(ContraCore *core, uint8_t x)
{
    static const uint8_t nibbles[38] = {
        /* @0  routine_00 (level 1 + common types < 0x10) */
        0x04u, 0x53u,
        /* @2  routine_01 (levels 2 and 4) */
        0x75u, 0x56u, 0x50u, 0x44u, 0x44u, 0x43u, 0x33u, 0x20u, 0x43u,
        /* @11 routine_02 (level 3) */
        0x45u, 0x53u, 0x33u,
        /* @14 routine_03 (level 5) */
        0x43u, 0x33u, 0x43u, 0x54u,
        /* @18 routine_04 (level 6) */
        0x30u, 0x22u, 0x24u,
        /* @21 routine_05 (level 7) */
        0x65u, 0x33u, 0x50u, 0xA5u, 0x20u,
        /* @26 routine_06 (level 8) */
        0x00u, 0x07u, 0x30u, 0x05u, 0x30u, 0x44u, 0x35u, 0x50u,
        0x43u, 0x34u, 0x63u, 0x40u};
    static const uint8_t level_base[8] = {0u, 2u, 11u, 2u, 14u, 18u, 21u, 26u};
    uint8_t *const ram = core->ram;
    const uint8_t type = ram[CONTRA_RAM_ENEMY_TYPE + x];
    const uint8_t base = (type < 0x10u)
        ? 0u
        : level_base[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u];
    const unsigned idx = (unsigned)base + (type >> 1u);
    uint8_t nib;

    if (idx >= sizeof nibbles)
    {
        return;
    }
    nib = ((type & 0x01u) != 0u)
        ? (uint8_t)(nibbles[idx] & 0x0Fu)
        : (uint8_t)(nibbles[idx] >> 4u);
    if (nib >= ram[CONTRA_RAM_ENEMY_ROUTINE + x])
    {
        ram[CONTRA_RAM_ENEMY_ROUTINE + x] = nib;
    }
}

static void contra_rom_destroy_all_enemies(ContraCore *core, int keep_slot)
{
    int s;

    (void)keep_slot;
    for (s = 0x0F; s >= 0; --s)
    {
        const uint8_t ss = (uint8_t)s;
        const uint8_t type = core->ram[CONTRA_RAM_ENEMY_TYPE + ss];

        if ((core->ram[CONTRA_RAM_ENEMY_ROUTINE + ss] == 0u) ||
            (core->ram[CONTRA_RAM_ENEMY_SPRITES + ss] == 0u) ||
            (type == 0x02u) || (type == 0x03u) ||
            (core->ram[CONTRA_RAM_ENEMY_HP + ss] == 0xF0u))
        {
            continue;
        }
        contra_rom_set_destroyed_enemy_routine(core, ss);
        core->ram[CONTRA_RAM_ENEMY_ATTRIBUTES + ss] =
            (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ATTRIBUTES + ss] | 0x80u);
    }
}

/* pick_up_weapon_item (bank7:6884): the player touched a weapon item -- award
   1,000 points (#$0A via add_player_low_score, demo-gated), then apply the
   weapon change for ATTRIBUTES & 0x07 (R adds rapid fire; M/F/S/L replace and
   drop rapid fire unless it's the same weapon; B grants the barrier timer;
   falcon wipes the screen), then remove the item. */
static void contra_rom_pick_up_weapon_item(ContraCore *core, uint8_t slot, uint8_t player)
{
    uint8_t *const ram = core->ram;
    const uint8_t attrs = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] & 0x07u);
    uint8_t item_type;
    uint8_t keep_mask;

    if (ram[CONTRA_RAM_DEMO_MODE] == 0u)
    {
        contra_rom_add_player_score(core, player, 0x0Au, 0x00u); /* 1,000 points */
    }
    contra_play_sound(core, 0x1Fu); /* sound_1f: weapon item taken */

    if (attrs == 0u)
    {
        item_type = 0x10u; /* R: set rapid fire, keep the current weapon */
        keep_mask = 0xFFu;
    }
    else if (attrs < 0x05u)
    {
        item_type = attrs; /* M/F/S/L */
        keep_mask = (((attrs ^ ram[CONTRA_RAM_P1_CURRENT_WEAPON + player]) & 0x0Fu) == 0u)
            ? 0xF0u  /* same weapon: keep rapid fire */
            : 0xE0u; /* different weapon: drop rapid fire */
    }
    else if (attrs == 0x05u)
    {
        ram[CONTRA_RAM_INVINCIBILITY_TIMER + player] =
            (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u) ? 0x90u : 0x80u; /* barrier */
        contra_rom_clear_enemy(core, slot);
        return;
    }
    else
    {
        contra_rom_destroy_all_enemies(core, (int)slot); /* falcon (item slot cleared below) */
        ram[CONTRA_RAM_FALCON_FLASH_TIMER] = 0x20u;
        contra_rom_clear_enemy(core, slot);
        return;
    }
    ram[CONTRA_RAM_P1_CURRENT_WEAPON + player] =
        (uint8_t)((ram[CONTRA_RAM_P1_CURRENT_WEAPON + player] & keep_mask) | item_type);
    contra_rom_clear_enemy(core, slot);
}

/* --- exploding bridge (enemy type 0x12, level 1), bank0.asm:2265-2403 --- */

/* exploding_bridge_destroyed_supertile_tbl (bank0:2362), with the +1 overflow
   into the cloud-y table (0x1D) that the ROM relies on for the last section. */
static const uint8_t contra_exploding_bridge_destroyed_supertile_tbl[8] = {
    0x00u, 0x1Au, 0x1Bu, 0x1Cu, 0x19u, 0x1Cu, 0x19u, 0x1Du};
static const uint8_t contra_exploding_bridge_cloud_y_offset[4] = {0x1Du, 0x00u, 0xF0u, 0x00u};
static const uint8_t contra_exploding_bridge_cloud_x_offset[5] = {0x10u, 0xF0u, 0x00u, 0x10u, 0x00u};

/* clear_supertile_bg_collision (bank7:8143) on the native model: draw the
   destroyed bridge super-tile and record its world position so the outdoor
   collision lookup reports "empty" there (the player falls through). draw_x_base
   is the enemy X (or X-0x20 for the trailing tile); the L1 render helper applies
   the -0x0c super-tile offset, so the recorded screen X matches the drawn tile. */
static void contra_rom_record_supertile_collision_override_world(
    ContraCore *core, uint16_t world_x, uint8_t cell_screen_y, uint8_t tile, uint8_t coll_packed)
{
    uint8_t i;

    for (i = 0u; i < core->l1_bridge_gap_count; ++i)
    {
        if ((core->l1_bridge_gap_world_x[i] == world_x) &&
            (core->l1_bridge_gap_screen_y[i] == cell_screen_y))
        {
            /* A later step re-draws this cell with a more-destroyed super-tile
               (e.g. 0x1a -> 0x1b); keep the latest so the persistent per-frame
               redraw matches what the ROM left on the nametable. */
            core->l1_bridge_gap_tile[i] = tile;
            core->l1_bridge_gap_coll[i] = coll_packed;
            return; /* already recorded */
        }
    }
    if (core->l1_bridge_gap_count <
        (uint8_t)(sizeof(core->l1_bridge_gap_world_x) / sizeof(core->l1_bridge_gap_world_x[0])))
    {
        core->l1_bridge_gap_world_x[core->l1_bridge_gap_count] = world_x;
        core->l1_bridge_gap_screen_y[core->l1_bridge_gap_count] = cell_screen_y;
        core->l1_bridge_gap_tile[core->l1_bridge_gap_count] = tile;
        core->l1_bridge_gap_coll[core->l1_bridge_gap_count] = coll_packed;
        core->l1_bridge_gap_count = (uint8_t)(core->l1_bridge_gap_count + 1u);
    }
}

static void contra_rom_record_supertile_collision_override(
    ContraCore *core, uint8_t cell_screen_x, uint8_t cell_screen_y, uint8_t tile, uint8_t coll)
{
    uint8_t *const ram = core->ram;
    const uint16_t world_x =
        (uint16_t)(((uint16_t)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                   ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + cell_screen_x);

    /* whole-cell code: the same nibble applies to both 16px halves */
    contra_rom_record_supertile_collision_override_world(
        core, world_x, cell_screen_y, tile,
        (uint8_t)((coll & 0x0Fu) | (uint8_t)(coll << 4u)));
}

/* set_supertile_bg_collisions / clear_supertile_bg_collision (bank7:8218-8317 /
   8143-8181) for runtime super-tile stamps on the native model: record an
   override for the GRID-ALIGNED cell containing the draw point, with separate
   left/right 16px-half nibbles (A/Y in the ROM call). Snapping to the cell grid
   here (unlike the bridge recorder, which keeps the draw anchor for its redraw)
   makes the rising spiked wall's 16px-step redraws dedupe into one entry per
   cell, so the destroy animation's clear replaces the risen wall's solid code
   instead of being shadowed by it. tile 0 = collision-only entry (the L7 walls
   draw through the L7 overlay cache, not the bridge redraw). */
static void contra_rom_set_supertile_bg_collisions(
    ContraCore *core, uint8_t draw_x, uint8_t draw_y, uint8_t coll_left, uint8_t coll_right)
{
    uint8_t *const ram = core->ram;
    const uint16_t world_x =
        (uint16_t)((uint16_t)(((uint16_t)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                              ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + draw_x) &
                   ~(uint16_t)31u);
    const uint8_t cell_screen_y =
        (uint8_t)(((uint8_t)((uint8_t)(draw_y - 0x10u) & 0xE0u)) + 0x10u);

    contra_rom_record_supertile_collision_override_world(
        core, world_x, cell_screen_y, 0u,
        (uint8_t)((coll_left & 0x0Fu) | (uint8_t)(coll_right << 4u)));
}

static void contra_rom_bridge_destroy_supertile(
    ContraCore *core, uint8_t x, uint8_t draw_x_base, uint8_t tile)
{
    uint8_t *const ram = core->ram;

    contra_render_level_1_nametable_update_supertile(
        core, (int)draw_x_base, (int)ram[CONTRA_RAM_ENEMY_Y_POS + x], tile);
    contra_rom_record_supertile_collision_override(
        core, (uint8_t)(draw_x_base - 12u),
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] - 12u), tile, 0x00u);
}

/* exploding_bridge_routine_00 (bank0:2273): wait until a player is within 0x18
   pixels, then start the explosion sequence. */
static void contra_rom_exploding_bridge_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t ex;
    uint8_t d0;
    uint8_t d1;

    /* the ROM scrolls FIRST, then measures the distance from the updated X --
       measuring pre-scroll triggered the bridge one frame late */
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
    d0 = (ram[CONTRA_RAM_SPRITE_X_POS + 0u] >= ex)
        ? (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + 0u] - ex)
        : (uint8_t)(ex - ram[CONTRA_RAM_SPRITE_X_POS + 0u]);
    d1 = (ram[CONTRA_RAM_SPRITE_X_POS + 1u] >= ex)
        ? (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + 1u] - ex)
        : (uint8_t)(ex - ram[CONTRA_RAM_SPRITE_X_POS + 1u]);
    if (ram[CONTRA_RAM_PLAYER_STATE + 0u] != 0x01u)
    {
        d0 = 0xFEu;
    }
    if (ram[CONTRA_RAM_PLAYER_STATE + 1u] != 0x01u)
    {
        d1 = 0xFFu;
    }
    if (((d1 < d0) ? d1 : d0) >= 0x18u)
    {
        return; /* no player close enough yet */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
    contra_rom_advance_enemy_routine(core, x); /* -> routine_01 */
}

/* exploding_bridge_routine_01 (bank0:2289): per beat, draw a destroyed super-tile
   (clearing its bg collision) for the first two cloud steps, then pop an explosion
   cloud; after four clouds advance to routine_04 (next section). */
static void contra_rom_exploding_bridge_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t var2;

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
    var2 = ram[CONTRA_RAM_ENEMY_VAR_2 + x];
    if (var2 < 0x02u)
    {
        const uint8_t section = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
        const uint8_t tile =
            contra_exploding_bridge_destroyed_supertile_tbl[(uint8_t)((section * 2u) + var2) & 0x07u];

        if (tile != 0u)
        {
            /* VAR_2 even -> the trailing (previous) super-tile at X-0x20 */
            const uint8_t draw_x = ((var2 & 0x01u) == 0u)
                ? (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 0x20u)
                : ram[CONTRA_RAM_ENEMY_X_POS + x];

            contra_rom_bridge_destroy_supertile(core, x, draw_x, tile);
        }
    }
    var2 = (uint8_t)(var2 + 1u);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = var2;
    if (var2 >= 0x04u)
    {
        contra_rom_advance_enemy_routine(core, x); /* -> routine_04 */
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x04u;
    /* the bridge clouds use create_enemy_for_explosion (bank0:2352): the SMALL
       single-round explosion (state_width 0x08), not the big 0x89 burst */
    contra_rom_create_explosion_sequence(
        core,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + contra_exploding_bridge_cloud_x_offset[var2 & 0x07u]),
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + contra_exploding_bridge_cloud_y_offset[var2 & 0x03u]),
        0x08u, 0x06u);
}

/* exploding_bridge_routine_04 (bank0:2385): advance to the next bridge section
   (X += 0x20) and loop back to routine_01, or remove the bridge after section 4. */
static void contra_rom_exploding_bridge_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    unsigned nx;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] >= 0x04u)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* all sections gone; ROM remove_enemy keeps type */
        return;
    }
    nx = (unsigned)ram[CONTRA_RAM_ENEMY_X_POS + x] + 0x20u;
    if (nx > 0xFFu)
    {
        contra_rom_remove_enemy_offscreen(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)nx;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x01u;     /* drop the previous cloud sprite */
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u); /* -> routine_01 */
}

/* enemy_routine_init_explosion (bank7:7544): mark the slot as exploding, recolor
   to the explosion palette, hide the sprite, and advance. The bridge runs these
   shared explosion routines as routine *steps* (between sections) rather than
   swapping the slot to the 0xFE actor, then continues to its routine_04. */
static void contra_rom_enemy_routine_init_explosion_step(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] | 0x81u);
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0xFCu) | 0x06u);
    if (ram[CONTRA_RAM_ENEMY_SPRITES + x] == 0u)
    {
        contra_rom_clear_enemy(core, x); /* nothing on screen -> remove */
        return;
    }
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0xFFu;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x01u; /* hide while the cloud animates */
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
}

/* enemy_routine_explosion (bank7:7616): step the explosion sprite sequence (3
   frames for type 0, 4 for type 1), then advance to the slot's next routine. */
static void contra_rom_enemy_routine_explosion_step(ContraCore *core, uint8_t x)
{
    static const uint8_t explosion_type_00[3] = {0x38u, 0x39u, 0x3Au};
    static const uint8_t explosion_type_01[4] = {0x37u, 0x35u, 0x36u, 0x37u};
    uint8_t *const ram = core->ram;
    const bool large = (ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x08u) != 0u;
    const uint8_t max_frames = large ? 4u : 3u;
    uint8_t frame;

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
    frame = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    ram[CONTRA_RAM_ENEMY_FRAME + x] = frame;
    if (frame >= max_frames)
    {
        contra_rom_advance_enemy_routine(core, x); /* -> the slot's next routine */
        return;
    }
    if ((uint8_t)(frame + 1u) >= max_frames)
    {
        contra_rom_disable_enemy_collision(core, x); /* last sprite */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        large ? explosion_type_01[frame] : explosion_type_00[frame];
}

/* --- level-1 fortress boss door (enemy type 0x11), bank0.asm:2184 ---
   The plated door is the level-1 boss target. boss_wall_plated_door_routine_ptr_tbl:
     RAM 1  boss_wall_plated_door_routine_00   siren, advance
     RAM 2  add_scroll_to_enemy_pos            wait here, killable
     RAM 3  boss_defeated_routine              the set_destroyed_enemy_routine target
     RAM 4  enemy_routine_explosion
     RAM 5  shared_enemy_routine_clear_sprite
     RAM 6  boss_wall_plated_door_routine_05   arm the tunnel-open sequence
     RAM 7  boss_wall_plated_door_routine_06   blast the tunnel super-tiles open
   When the player shoots the door to 0 HP, set_destroyed_enemy_routine routes it
   to RAM routine 3 (enemy_destroyed_routine_00 byte $33, door = low nibble = 3),
   which sets BOSS_DEFEATED_FLAG + the auto-move delay, wipes the other enemies,
   then cascades through the explosion and tunnel-open. */

/* boss_wall_plated_door_routine_00 (bank0:2194): play the jungle-boss siren and
   advance to the wait/killable routine. */
static void contra_rom_boss_door_routine_00(ContraCore *core, uint8_t x)
{
    contra_play_sound(core, 0x1Bu); /* sound_1b: level-1 boss siren */
    contra_rom_advance_enemy_routine(core, x);
}

/* boss_defeated_routine (bank7:7536): init the APU, play the boss-destroyed sound,
   set BOSS_DEFEATED_FLAG + the auto-move delay (level_boss_defeated), destroy all
   the other enemies, then fall through to enemy_routine_init_explosion (hide the
   door and advance to its explosion routine). */
static void contra_rom_boss_door_routine_02(ContraCore *core, uint8_t x)
{
    contra_init_apu_channels(core);
    contra_play_sound(core, 0x57u);                    /* sound_57: boss destroyed */
    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0xFFu; /* auto-move delay */
    core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
    contra_rom_destroy_all_enemies(core, (int)x); /* keep the door's own slot alive */
    contra_rom_enemy_routine_init_explosion_step(core, x); /* -> RAM routine 4 */
}

/* shared_enemy_routine_clear_sprite (bank7): blank the sprite, advance. */
/* boss_door_routine_clear_sprite (bank7:7691-7693): blank sprite code, advance routine. */
static void contra_rom_boss_door_routine_clear_sprite(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0u; /* set_sprite_0 */
    contra_rom_advance_enemy_routine(core, x);
}

/* boss_wall_plated_door_routine_05 (bank0:2200): arm the tunnel-blast loop. */
static void contra_rom_boss_door_routine_05(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x00u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u);
}

/* tunnel super-tiles (bank0:2257), per-cell {y,x} move offsets (bank0:2245, the
   8th entry's 0xFF terminates), and collision codes (bank0:2261). */
static const uint8_t contra_door_tunnel_supertile_tbl[8] = {
    0x1Eu, 0x22u, 0x1Fu, 0x23u, 0x20u, 0x24u, 0x21u, 0x25u};
static const uint8_t contra_door_tunnel_collision_tbl[8] = {
    0x00u, 0x00u, 0x00u, 0x04u, 0x00u, 0x04u, 0x00u, 0x04u};
static const int8_t contra_door_tunnel_offset_tbl[8][2] = {
    {(int8_t)0xF0, (int8_t)0xF0}, {0x20, 0x00}, {(int8_t)0xE0, 0x20}, {0x20, 0x00},
    {(int8_t)0xE0, 0x20}, {0x20, 0x00}, {(int8_t)0xE0, 0x20}, {0x20, 0x00}};

/* boss_wall_plated_door_routine_06 (bank0:2207): every 8th frame stamp the next
   tunnel super-tile and pop an explosion at the door position; on the in-between
   tick move the door to the next tunnel cell; after the 8 cells, remove the door.
   The ROM also rewrites bg collision per cell (set_supertile_bg_collision,
   wall_plated_door_collision_code_tbl) -- the tunnel floor opens up and the
   walking-off player falls INTO the blasted hole; recorded in the runtime
   collision-override list like the exploding bridge. */
static void contra_rom_boss_door_routine_06(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t delay = (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    uint8_t idx;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = delay;
    if (delay != 0u)
    {
        /* @create_tunnel_explosion: only the final tick before the next stamp
           moves the door (or, once the offset table hits its 0xFF, removes it). */
        if (delay != 0x01u)
        {
            return;
        }
        idx = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
        if (idx >= 8u) /* wall_plated_door_explosion_offset_tbl terminator */
        {
            ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x30u;
            ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x00u;
            contra_rom_remove_enemy_offscreen(core, x); /* set_delay_remove_enemy -> remove_enemy keeps the husk */
            return;
        }
        ram[CONTRA_RAM_ENEMY_Y_POS + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + (uint8_t)contra_door_tunnel_offset_tbl[idx][0]);
        ram[CONTRA_RAM_ENEMY_X_POS + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + (uint8_t)contra_door_tunnel_offset_tbl[idx][1]);
        return;
    }
    /* delay reached 0: stamp this cell's tunnel super-tile + explosion, advance.
       The ROM sets the 8-frame delay before the draw; setting it after is
       equivalent now that the draw helper no longer touches the delay. */
    idx = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    contra_rom_draw_enemy_supertile_a_set_delay(core, x, contra_door_tunnel_supertile_tbl[idx]);
    contra_rom_record_supertile_collision_override(
        core, (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 12u),
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] - 12u),
        contra_door_tunnel_supertile_tbl[idx],
        contra_door_tunnel_collision_tbl[idx]);
    /* the ROM tail is `jmp create_enemy_for_explosion` (bank0:2230): the SMALL
       single-round 0x08 explosion, not the big 0x89 burst */
    contra_rom_create_explosion_sequence(
        core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
        0x08u, 0x06u);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(idx + 1u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
}

/* dispatch one enemy slot to its type routine by (ENEMY_TYPE, ENEMY_ROUTINE).
   Only ported types act; others hold until their routine is ported. */
/* rock_moving_flame_init_vel_tbl (bank0:4304): {x fract vel, x fast vel} indexed by
   ENEMY_ATTRIBUTES — slow rock, fast rock, flame-left, flame-right. */
static const uint8_t contra_rock_moving_flame_init_vel_tbl[4][2] = {
    {0x80u, 0xFFu}, {0xC0u, 0x00u}, {0x80u, 0xFFu}, {0x80u, 0x00u}};
/* rock_moving_flame_boundaries_tbl (bank0:4313): {left X barrier, right X barrier}. */
static const uint8_t contra_rock_moving_flame_boundaries_tbl[4][2] = {
    {0x50u, 0xB0u}, {0x70u, 0xC0u}, {0x48u, 0xB8u}, {0x48u, 0xB8u}};

/* floating_rock_routine_00 (bank0:4286): shared by the Level 3 rock platform and
   moving flame. Seed x velocity/direction and the turn-around barriers from the
   enemy attribute, fold in scroll, then advance to the active routine. */
static void contra_rom_floating_rock_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t idx = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x03u);

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_rock_moving_flame_init_vel_tbl[idx][0];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_rock_moving_flame_init_vel_tbl[idx][1];
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = contra_rock_moving_flame_boundaries_tbl[idx][0];
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = contra_rock_moving_flame_boundaries_tbl[idx][1];
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_advance_enemy_routine(core, x);
}

/* update_pos_turn_around_if_needed (bank0:4326): move by velocity, then reverse x
   direction when the platform/flame reaches its left (VAR_2) or right (VAR_1)
   barrier. */
static void contra_rom_update_pos_turn_around_if_needed(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_update_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] & 0x80u) != 0u)
    {
        if (ram[CONTRA_RAM_ENEMY_X_POS + x] < ram[CONTRA_RAM_ENEMY_VAR_2 + x])
        {
            contra_rom_reverse_enemy_x_direction(core, x);
        }
    }
    else if (ram[CONTRA_RAM_ENEMY_X_POS + x] >= ram[CONTRA_RAM_ENEMY_VAR_1 + x])
    {
        contra_rom_reverse_enemy_x_direction(core, x);
    }
}

/* floating_rock_routine_01 (bank0:4321): the moving rock platform. */
static void contra_rom_floating_rock_routine_01(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x48u; /* sprite_48 floating rock */
    contra_rom_update_pos_turn_around_if_needed(core, x);
}

/* moving_flame_routine_01 (bank0:4353): the bridge flame — same motion as the rock
   platform plus a palette flash every 8 frames. */
static void contra_rom_moving_flame_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x49u; /* sprite_49 bridge fire */
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x08u) != 0u) ? 0x40u : 0x00u;
    contra_rom_update_pos_turn_around_if_needed(core, x);
}

/* rock_cave_routine_00 (bank0:4375): fold in scroll, advance. */
static void contra_rom_rock_cave_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_advance_enemy_routine(core, x);
}

/* rock_cave_routine_01 (bank0:4379): fold in scroll, set the delay before the
   first falling rock, advance. */
static void contra_rom_rock_cave_routine_01(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u);
}

/* rock_cave_routine_02 (bank0:4384): the active generator — every #$e0 frames
   spawn a falling rock (type 0x13) at the cave's position. */
static void contra_rom_rock_cave_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_update_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0xE0u;
    contra_rom_generate_enemy_at_pos(core, x, 0x13u);
}

/* falling_rock_routine_00 (bank0:4402): wait the initial fall delay, advance. */
static void contra_rom_falling_rock_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x40u);
}

/* falling_rock_set_sprite_and_attr (bank0:4434): tumble the boulder by cycling the
   sprite flip bits every 4 frames. */
static void contra_rom_falling_rock_set_sprite_and_attr(ContraCore *core, uint8_t x)
{
    static const uint8_t flip_tbl[4] = {0x00u, 0x40u, 0xC0u, 0x80u};
    uint8_t *const ram = core->ram;
    const uint8_t idx = (uint8_t)((ram[CONTRA_RAM_FRAME_COUNTER] >> 2u) & 0x03u);

    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0x3Fu) | flip_tbl[idx]);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x4Au; /* sprite_4a boulder */
}

/* falling_rock_routine_01 (bank0:4407): the boulder wobbles left/right while the
   pre-fall delay counts down, then enables collision and advances to the fall. */
static void contra_rom_falling_rock_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x4Au; /* falling_rock_set_sprite */
    contra_rom_update_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u) == 0u)
    {
        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x04u) != 0u)
        {
            ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 1u);
        }
        else
        {
            ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - 1u);
        }
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    contra_rom_enable_enemy_collision(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
}

/* falling_rock_routine_02 (bank0:4453): the boulder falls under gravity and bounces
   when it meets the ground (ENEMY_VAR_1 tracks the ground Y it landed on). */
static void contra_rom_falling_rock_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_falling_rock_set_sprite_and_attr(core, x);
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= ram[CONTRA_RAM_ENEMY_VAR_1 + x])
    {
        if (contra_rom_add_y_to_y_pos_get_bg_collision(core, x, 0x08u) != 0u)
        {
            const uint16_t ground = (uint16_t)ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x10u;

            contra_play_sound(core, 0x05u); /* rock hitting ground */
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x40u;
            ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (ground > 0xFFu) ? 0xFFu : (uint8_t)ground;
            ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0xC0u;
            ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xFEu; /* bounce up at -1.25 */
        }
    }

    contra_rom_add_10_to_enemy_y_fract_vel(core, x); /* gravity */
    {
        const uint16_t var1 = (uint16_t)ram[CONTRA_RAM_ENEMY_VAR_1 + x] + ram[CONTRA_RAM_FRAME_SCROLL];
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (var1 > 0xFFu) ? 0xFFu : (uint8_t)var1;
    }
    contra_rom_update_enemy_pos(core, x);
}

/* player_enemy_x_dist (bank7:8844): return the index (0/1) of the player closest in
   X to enemy x, treating a non-normal player as infinitely far (0xFE/0xFF). */
static uint8_t contra_rom_player_enemy_x_dist(const ContraCore *core, uint8_t x)
{
    const uint8_t *const ram = core->ram;
    const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t p0 = ram[CONTRA_RAM_SPRITE_X_POS + 0u];
    const uint8_t p1 = ram[CONTRA_RAM_SPRITE_X_POS + 1u];
    uint8_t d0 = (p0 >= ex) ? (uint8_t)(p0 - ex) : (uint8_t)(ex - p0);
    uint8_t d1 = (p1 >= ex) ? (uint8_t)(p1 - ex) : (uint8_t)(ex - p1);

    if (ram[CONTRA_RAM_PLAYER_STATE + 0u] != 0x01u) { d0 = 0xFEu; }
    if (ram[CONTRA_RAM_PLAYER_STATE + 1u] != 0x01u) { d1 = 0xFFu; }
    return (d1 < d0) ? 1u : 0u;
}

/* generate_enemy_at_pos (bank7:8676) with a relative (x,y) offset from the
   generating enemy's position. */
static int contra_rom_generate_enemy_at_offset_slot(ContraCore *core, uint8_t gen_slot,
                                                    uint8_t type, uint8_t x_off, uint8_t y_off)
{
    uint8_t *const ram = core->ram;
    const int slot = contra_rom_find_next_enemy_slot(core);

    if (slot < 0)
    {
        return -1;
    }
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = type;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    ram[CONTRA_RAM_ENEMY_Y_POS + slot] = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + gen_slot] + y_off);
    ram[CONTRA_RAM_ENEMY_X_POS + slot] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + gen_slot] + x_off);
    return slot;
}

static void contra_rom_generate_enemy_at_offset(ContraCore *core, uint8_t gen_slot,
                                                uint8_t type, uint8_t x_off, uint8_t y_off)
{
    (void)contra_rom_generate_enemy_at_offset_slot(core, gen_slot, type, x_off, y_off);
}

/* scuba_soldier_routine_00 (bank7:9534): wait the initial pre-attack delay. */
static void contra_rom_scuba_soldier_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x80u);
}

/* scuba_soldier_routine_01 (bank7:9543): hide in the water bobbing up and down,
   then activate once low enough on screen (Y >= #$b8 on the vertical level). */
static void contra_rom_scuba_soldier_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x4Bu; /* sprite_4b hiding */
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        ((ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] & 0x10u) != 0u) ? 0x00u : 0x08u;
    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xB8u)
    {
        contra_rom_enable_enemy_collision(core, x);
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x10u;
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x30u);
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x10u; /* re-check soon */
    }
}

/* scuba_soldier_routine_02 (bank7:9579): surface and fire a mortar shot, then dive
   back to the hiding routine. */
static void contra_rom_scuba_soldier_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x4Cu; /* sprite_4c surfaced, shooting up */
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
        ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x08u; /* gun recoil */
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x00u;
    }

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        /* vulnerability window over: dive and loop back to the hide routine */
        contra_rom_add_scroll_to_enemy_pos(core, x);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0xC0u;
        contra_rom_disable_enemy_collision(core, x);
        contra_rom_set_enemy_routine_to_a(core, x, 0x02u); /* -> scuba_soldier_routine_01 */
        return;
    }

    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] == 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x07u; /* recoil timer */
        contra_rom_generate_enemy_at_offset(core, x, 0x0Bu, 0x05u, 0xE8u); /* mortar */
    }
    contra_rom_add_scroll_to_enemy_pos(core, x);
}

/* mortar_shot_velocity_tbl (bank7:9671): {y fract, y fast, x fract, x fast} indexed
   by ENEMY_ATTRIBUTES (0 = initial straight-up shot; 1/2/3 = the up/right/left split
   rounds; 4-7 = the hangar-zone aimed launches). */
static const uint8_t contra_mortar_shot_velocity_tbl[8][4] = {
    {0x00u, 0xFBu, 0x00u, 0x00u}, {0x00u, 0xFEu, 0x00u, 0x00u},
    {0x40u, 0xFEu, 0x90u, 0x00u}, {0x40u, 0xFEu, 0x70u, 0xFFu},
    {0x00u, 0xFBu, 0xC0u, 0xFFu}, {0x00u, 0xFBu, 0x80u, 0xFFu},
    {0x00u, 0xFBu, 0x40u, 0xFFu}, {0x00u, 0xFBu, 0x00u, 0xFFu}};

/* mortar_shot_routine_00 (bank7:9627): set the explosion mode, sprite, palette and
   launch velocity from the mortar's attribute. */
static void contra_rom_mortar_shot_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    uint8_t idx;

    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = (attr != 0u) ? 0x80u : 0x8Au;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x20u; /* sprite_20 mortar */
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x06u; /* palette 2 */

    if (attr != 0u)
    {
        idx = attr;
    }
    else if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0u)
    {
        idx = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 3u);
    }
    else
    {
        idx = 0u;
    }
    if (idx > 7u) { idx = 0u; }

    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = contra_mortar_shot_velocity_tbl[idx][0];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = contra_mortar_shot_velocity_tbl[idx][1];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_mortar_shot_velocity_tbl[idx][2];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_mortar_shot_velocity_tbl[idx][3];
    contra_rom_advance_enemy_routine(core, x);
}

/* mortar_shot_routine_02 (bank7:9714): the initial shot splits into 3 rounds (with
   attributes 3/2/1) at its position, then becomes an explosion. */
static void contra_rom_mortar_shot_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t count;

    contra_rom_update_enemy_pos(core, x);
    for (count = 3u; count != 0u; --count)
    {
        const int slot = contra_rom_find_next_enemy_slot(core);

        if (slot < 0)
        {
            break;
        }
        ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x0Bu;
        contra_rom_initialize_enemy(core, (uint8_t)slot);
        ram[CONTRA_RAM_ENEMY_X_POS + slot] = ram[CONTRA_RAM_ENEMY_X_POS + x];
        ram[CONTRA_RAM_ENEMY_Y_POS + slot] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
        ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = count;
    }
    contra_rom_advance_enemy_routine(core, x); /* -> enemy_routine_init_explosion */
}

/* mortar_shot_routine_01 (bank7:9684): fly under gravity. The initial shot splits at
   its apex; a split round explodes when it falls onto the floor below a player. */
static void contra_rom_mortar_shot_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_10_to_enemy_y_fract_vel(core, x); /* gravity */
    contra_rom_update_enemy_pos(core, x);

    if (ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] == 0u)
    {
        /* initial shot: advance to the split routine once falling or past apex */
        if (((ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] & 0x80u) == 0u) ||
            (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x30u))
        {
            contra_rom_advance_enemy_routine(core, x);
        }
        return;
    }

    /* split round */
    if ((ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] & 0x80u) != 0u)
    {
        return; /* still rising */
    }
    {
        const uint8_t closest = contra_rom_player_enemy_x_dist(core, x);

        if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < ram[CONTRA_RAM_SPRITE_Y_POS + closest])
        {
            return; /* still above the nearest player */
        }
    }
    if (contra_rom_add_y_to_y_pos_get_bg_collision(core, x, 0x00u) == 0u)
    {
        return; /* no floor yet */
    }
    contra_play_sound(core, 0x24u);
    contra_rom_set_enemy_routine_to_a(core, x, 0x07u); /* -> mortar_shot_routine_03 */
}

/* enable/disable_bullet_enemy_collision (bank7:8371/8349): bit 7 of ENEMY_STATE_WIDTH
   gates whether bullets collide (set = pass through). */
