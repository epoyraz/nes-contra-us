/* Level 2 and Level 4 indoor base enemies, generators, and boss rooms.
   Included by core.c; not compiled as a separate translation unit. */

/* --- level-2 wall turret (enemy type 0x13), bank0.asm --- */
static const uint8_t contra_wall_turret_initial_delay_tbl[4] = {0x50u, 0x80u, 0xB0u, 0xF0u};

/* quadrant_aim_dir_01 (bank7:10563): the within-quadrant aim direction nibble
   indexed by [row = |dy|>>5][col = |dx|>>6]; the high nibble is used when bit5 of
   |dx| is clear, the low nibble when set. Indoor enemies (wall turret/core,
   jumping soldier) all aim through this table. */
static const uint8_t contra_quadrant_aim_dir_01[32] = {
    0x00u, 0x00u, 0x00u, 0x00u, 0x63u, 0x21u, 0x11u, 0x11u,
    0x64u, 0x32u, 0x21u, 0x11u, 0x65u, 0x43u, 0x22u, 0x22u,
    0x65u, 0x44u, 0x33u, 0x22u, 0x65u, 0x54u, 0x33u, 0x32u,
    0x65u, 0x54u, 0x43u, 0x33u, 0x65u, 0x54u, 0x44u, 0x33u};

/* get_quadrant_aim_dir (bank7:10469): the aim-direction nibble pointing from
   source (sx,sy) toward target (tx,ty) using table tbl; *quadrant returns $07
   (bit0 = source below target -> fire up, bit1 = source right of target -> fire
   left), which the bullet creator turns into velocity sign flips. */
static uint8_t contra_rom_get_quadrant_aim_dir(
    uint8_t sx, uint8_t sy, uint8_t tx, uint8_t ty, const uint8_t *tbl, uint8_t *quadrant)
{
    uint8_t q = 0u;
    uint8_t dy;
    uint8_t dx;
    uint8_t byte;

    if (ty >= sy)
    {
        dy = (uint8_t)(ty - sy);
    }
    else
    {
        dy = (uint8_t)(sy - ty);
        q |= 0x01u; /* source below target */
    }
    if (tx >= sx)
    {
        dx = (uint8_t)(tx - sx);
    }
    else
    {
        dx = (uint8_t)(sx - tx);
        q |= 0x02u; /* source right of target */
    }
    byte = tbl[(size_t)(((dy >> 5) << 2) + (dx >> 6)) & 0x1Fu];
    *quadrant = q;
    return ((dx & 0x20u) != 0u) ? (uint8_t)(byte & 0x0Fu) : (uint8_t)((byte >> 4u) & 0x0Fu);
}

/* aim_and_create_enemy_bullet (bank7): fire a bullet of type btype at the closest
   normal-state player using table tbl. Indoor levels aim at a fixed player Y
   (0xB0), matching the ROM (it ignores indoor jump height); if neither player is
   in a normal state, aim at screen center-bottom (0x80, 0xFF). */
/* aim_and_create_enemy_bullet (bank7:9750-9769): aim enemy bullet at closest normal player, fire. */
static void contra_rom_aim_and_create_enemy_bullet(
    ContraCore *core, uint8_t sx, uint8_t sy, uint8_t btype, uint8_t speed, const uint8_t *tbl)
{
    uint8_t *const ram = core->ram;
    const uint8_t p0 = ram[CONTRA_RAM_SPRITE_X_POS + 0u];
    const uint8_t p1 = ram[CONTRA_RAM_SPRITE_X_POS + 1u];
    /* player_enemy_x_dist (bank7:8844): closest player by |X dist|, non-normal players
       forced to max distance (0xFE/0xFF) via PLAYER_STATE -- not an x==0 absent proxy. */
    uint8_t d0 = (p0 >= sx) ? (uint8_t)(p0 - sx) : (uint8_t)(sx - p0);
    uint8_t d1 = (p1 >= sx) ? (uint8_t)(p1 - sx) : (uint8_t)(sx - p1);
    uint8_t idx;
    uint8_t tx;
    uint8_t ty;
    uint8_t quadrant;
    uint8_t nibble;

    ram[0x06u] = speed;
    if (ram[CONTRA_RAM_PLAYER_STATE + 0u] != 0x01u) { d0 = 0xFEu; }
    if (ram[CONTRA_RAM_PLAYER_STATE + 1u] != 0x01u) { d1 = 0xFFu; }
    idx = (d1 < d0) ? 1u : 0u;

    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        return; /* create_enemy_bullet_if_attack_enabled: only fire when allowed */
    }
    if (ram[CONTRA_RAM_PLAYER_STATE + idx] != 0x01u)
    {
        idx ^= 0x01u; /* closest player isn't in a normal state; try the other */
    }
    if (ram[CONTRA_RAM_PLAYER_STATE + idx] != 0x01u)
    {
        tx = 0x80u;
        ty = 0xFFu;
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
    nibble = contra_rom_get_quadrant_aim_dir(sx, sy, tx, ty, tbl, &quadrant);
    (void)contra_rom_create_enemy_bullet(core, btype, nibble, quadrant, speed, sx, sy);
}

/* wall_turret_routine_00..04 (bank0:3049-3127): deploy, open, fire at the
   player, be destroyable. The nametable tile-animation (update_enemy_nametable_
   tiles from level_2_4_tile_animation) is recorded in l2_structure_tile[x] at the
   ROM draw points and re-drawn each render frame by contra_render_level_2_wall_
   structures; the logic, collision, and firing are faithful. */
static const uint8_t contra_wall_turret_opening_tile_tbl[3] = {0x85u, 0x88u, 0x89u};

static void contra_rom_wall_turret_routine_00(ContraCore *core, uint8_t x)
{
    const uint8_t idx = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x03u);

    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = contra_wall_turret_initial_delay_tbl[idx];
    contra_rom_advance_enemy_routine(core, x);
}

/* wall_turret_routine_01 (bank0:3059-3070): draw closed turret tile, wait, advance. */
/* update_nametable_tiles (bank7:1649) budget for the level-2/4 tile-animation
   draws: the draw fails when GRAPHICS_BUFFER_OFFSET is already >= 0x50 (e.g.
   the fence CHR rewrite plus another structure's draw the same frame); a
   2x2 block with its palette quadrant costs 0x11 buffer bytes. */
static bool contra_rom_tile_animation_draw_budget(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] >= 0x50u)
    {
        return false;
    }
    ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] =
        (uint8_t)(ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] + 0x11u);
    return true;
}

static void contra_rom_wall_turret_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u) &&
        contra_rom_tile_animation_draw_budget(core))
    {
        core->l2_structure_tile[x] = 0x84u; /* draw 'wall turret - closed' */
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 1u;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_wall_turret_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t frame;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    if (!contra_rom_tile_animation_draw_budget(core))
    {
        /* update_nametable_tiles_set_delay: failed draw -> retry next frame */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
        return;
    }
    frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    /* draw the opening frame for the pre-increment FRAME (bank0:3081-3086) */
    core->l2_structure_tile[x] = contra_wall_turret_opening_tile_tbl[(frame < 3u) ? frame : 2u];
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(frame + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x03u)
    {
        return; /* still opening */
    }
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu); /* enable bullet collision */
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x80u;
    contra_rom_advance_enemy_routine(core, x);
}

/* wall_turret_routine_03 (bank0:3106-3117): on timer, aim and fire a bullet. */
static void contra_rom_wall_turret_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x50u;
    contra_rom_aim_and_create_enemy_bullet(
        core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
        0x03u, 0x04u, contra_quadrant_aim_dir_01);
}

/* wall_turret_routine_04 (bank0:3123): the turret's "hit" routine -- draw the
   destroyed tile, then explode (the ROM advances into wall_core_routine_05). */
/* wall_turret_routine_04 (bank0:3046): draw the 'core - destroyed' tiles and
   advance into the shared wall-core explosion tail (wall_core_routine_05 ->
   enemy_routine_explosion -> remove_enemy). A failed draw plain-exits and the
   routine re-runs next frame. */
static void contra_rom_wall_turret_routine_04(ContraCore *core, uint8_t x)
{
    if (!contra_rom_tile_animation_draw_budget(core))
    {
        return;
    }
    core->l2_structure_tile[x] = 0x83u; /* 'core - destroyed' */
    contra_rom_advance_enemy_routine(core, x);
}

/* --- level-2 wall core (enemy type 0x14), bank0.asm:3143 --- */
static const uint8_t contra_wall_core_hp_tbl[4] = {0x08u, 0x05u, 0x10u, 0x05u};
static const uint8_t contra_wall_core_init_dmg_tile_anim_tbl[4] = {0x00u, 0x03u, 0x00u, 0x03u};
static const uint8_t contra_core_opening_delay[4] = {0x20u, 0x80u, 0xB0u, 0xF0u};

/* wall_core_routine_00 (bank0:3143): set HP, score/collision code, the
   destruction-animation offset, and the opening delay, from the core's size +
   plating attributes; advance to the open/expose cycle. */
static void contra_rom_wall_core_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    const uint8_t type_idx = (uint8_t)((attr >> 2u) & 0x03u);
    const bool plated = (attr & 0x04u) != 0u;
    uint8_t delay_idx;

    if (plated)
    {
        ram[CONTRA_RAM_ENEMY_VAR_A + x] = 0x04u;
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x22u;
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x25u;
    }
    ram[CONTRA_RAM_ENEMY_HP + x] = contra_wall_core_hp_tbl[type_idx];
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = contra_wall_core_init_dmg_tile_anim_tbl[type_idx];
    delay_idx = plated ? 0u : (uint8_t)(attr & 0x03u);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = contra_core_opening_delay[delay_idx];
    contra_rom_advance_enemy_routine(core, x);
}

/* offsets into level_2_4_tile_animation for the core opening frames
   (bank0.asm:3261): opening 1 / opening 2 / open. */
static const uint8_t contra_wall_core_opening_tile_tbl[3] = {0x85u, 0x86u, 0x87u};

/* wall_core_routine_01 (bank0:3200): draw the closed/plated tile once, then after
   the opening delay enable collision (plated cores) or advance into the opening
   animation (unplated). A plated core advances the routine twice, skipping the
   open animation -- it stays plated until the plating is shot off. */
static void contra_rom_wall_core_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];

    if (((attr & 0x08u) == 0u) && (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u) &&
        contra_rom_tile_animation_draw_budget(core))
    {
        /* normal core: draw plating (0x80) or closed (0x84); big cores skip this */
        core->l2_structure_tile[x] = ((attr & 0x04u) != 0u) ? 0x80u : 0x84u;
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 1u;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    if ((attr & 0x04u) != 0u)
    {
        /* plated: enable bullet collision and advance past the open animation */
        ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu);
        contra_rom_advance_enemy_routine(core, x);
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x01u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
}

/* wall_core_routine_02 (bank0:3235): play the core opening animation (3 frames,
   8-frame steps), then enable collision and advance to the firing routine. A big
   core has no opening delay and advances immediately. */
static void contra_rom_wall_core_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t frame;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x08u) != 0u)
    {
        ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu);
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    if (!contra_rom_tile_animation_draw_budget(core))
    {
        /* update_nametable_tiles_set_delay: failed draw -> retry next frame */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
        return;
    }
    frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    core->l2_structure_tile[x] = contra_wall_core_opening_tile_tbl[(frame < 3u) ? frame : 2u];
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(frame + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x03u)
    {
        return; /* still opening */
    }
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu);
    contra_rom_advance_enemy_routine(core, x);
}

/* wall_core_routine_03 (bank0:3265): once enough soldier attack rounds have
   happened and the core is open (unplated) and high enough, fire at the closest
   player on a 0x28 cadence through the quadrant aim solve
   (aim_and_create_enemy_bullet, type 3, speed code 5). */
static void contra_rom_wall_core_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] < 0x07u)
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        return; /* still plated */
    }
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0x70u)
    {
        return; /* core too low to attack */
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x28u;
    contra_rom_aim_and_create_enemy_bullet(
        core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
        0x03u, 0x05u, contra_quadrant_aim_dir_01);
}

/* destruction tile sequence (bank0:3329): normal core indices 0-3, big core 4-7
   -- destroyed / open / more-cracks / cracked. */
static const uint8_t contra_wall_core_tile_anim_tbl[8] = {
    0x83u, 0x87u, 0x82u, 0x81u, 0x83u, 0x8Au, 0x82u, 0x81u};

/* wall_core_routine_04 (bank0:3290): the core's "hit" routine (reached when its
   HP hits 0). Draw the next destruction tile and step down one plating layer:
   while plating remains, reset HP and return to firing; once the plating is gone,
   advance into the explosion + back-wall blow-open chain (routine_05..09). */
static void contra_rom_wall_core_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t idx = ram[CONTRA_RAM_ENEMY_VAR_2 + x];

    if (!contra_rom_tile_animation_draw_budget(core))
    {
        return; /* draw failed -- the routine re-runs next frame (bank0:3304) */
    }
    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x08u) != 0u)
    {
        idx = (uint8_t)(idx + 4u); /* big-core tile variant */
    }
    core->l2_structure_tile[x] = contra_wall_core_tile_anim_tbl[idx & 0x07u];

    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x80u) != 0u)
    {
        contra_rom_advance_enemy_routine(core, x); /* plating gone -> routine_05 */
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        /* last plating layer cracked: bare core exposed */
        ram[CONTRA_RAM_ENEMY_VAR_A + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x25u;
        ram[CONTRA_RAM_ENEMY_HP + x] = 0x08u;
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_HP + x] = 0x05u;
    }
    contra_rom_set_enemy_routine_to_a(core, x, 0x04u); /* back to wall_core_routine_03 */
}

/* wall_core_routine_05 (bank7:7527-7531): enemy_routine_init_explosion then FRAME=0.
   Unlike most enemies, the wall core can't convert to the shared type-0xFE explosion
   actor -- it must stay a wall_core to reach routine_07 (the room blow-open) -- so it
   inline-runs the explosion (sprites 0x38..0x3a). Faithful to init_explosion: set
   STATE_WIDTH |= 0x81 (bit7 lets bullets pass, bit0 skips player-body collision) and
   force sprite palette 2 ((attr & 0xFC) | 0x06), the explosion's orange/yellow. */
/* wall_core_routine_05 (bank7:7557): the shared init_explosion (invisible
   sprite, delay 1, sound when sw bit 1), then ENEMY_FRAME overwritten to 0 --
   so the explosion that follows skips its first sprite AND one of its beats
   (31 ticks instead of 41 for the 4-sprite variant). */
static void contra_rom_wall_core_routine_05(ContraCore *core, uint8_t x)
{
    contra_rom_enemy_routine_init_explosion_inplace(core, x);
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
}

/* wall core RAM routine 07 is the SHARED enemy_routine_explosion (bank0:3134). */
static void contra_rom_wall_core_routine_06(ContraCore *core, uint8_t x)
{
    contra_rom_enemy_routine_explosion_inplace(core, x);
}

/* wall_core_routine_07 (bank0:3333): one fewer core to destroy; if this was the
   last, hide it, wipe the other enemies, and start the back-wall blow-open. */
static void contra_rom_wall_core_routine_07(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_WALL_CORE_REMAINING] =
        (uint8_t)(ram[CONTRA_RAM_WALL_CORE_REMAINING] - 1u);
    if (ram[CONTRA_RAM_WALL_CORE_REMAINING] != 0u)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* remove_enemy keeps the husk */
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u; /* hide the core */
    contra_rom_destroy_all_enemies(core, (int)x);
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x03u; /* quadrant index 3..0 */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x04u); /* -> routine_08 */
}

/* wall_core_routine_08 (bank0:3347): blow the back wall open one quadrant at a
   time (4 super-tiles at fixed wall positions, recorded in l2_blowopen_quadrants
   for the render), then advance to mark the screen cleared. */
static void contra_rom_wall_core_routine_08(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t q;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        /* wall_core_wait_play_sound: the small boom on the last waiting tick */
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0x01u)
        {
            contra_play_sound(core, 0x25u);
        }
        return;
    }
    /* move the core to the quadrant being blown open, stamp the destroyed
       super-tile, and pop a single-round 0x89 explosion above it
       (create_explosion_89: the -12px offset for the top row quadrants,
       -4px for the bottom row). */
    q = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x03u);
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = contra_level_2_wall_core_update_y_tbl[q];
    ram[CONTRA_RAM_ENEMY_X_POS + x] = contra_level_2_wall_core_update_x_tbl[q];
    if (ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] >= 0x40u)
    {
        /* draw_enemy_supertile_a failed -- CPU_GRAPHICS_BUFFER already holds
           this frame's fence CHR rewrite (bank7:1353 entry check). The ROM's
           @set_delay_exit retries next frame, with the position already moved. */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
        return;
    }
    ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] =
        (uint8_t)(ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] + 0x21u); /* the super-tile stamp */
    core->l2_blowopen_quadrants = (uint8_t)(core->l2_blowopen_quadrants | (uint8_t)(1u << q));
    contra_rom_create_explosion_sequence(
        core,
        contra_level_2_wall_core_update_x_tbl[q],
        (uint8_t)(contra_level_2_wall_core_update_y_tbl[q] +
                  (((q & 0x02u) != 0u) ? 0xF4u : 0xFCu)),
        0x89u, 0x09u);
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) != 0u)
    {
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x10u); /* blown open -> routine_09 */
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u; /* next quadrant next frame */
}

/* wall_core_routine_09 (bank0:3412): wait out the blast, mark the room cleared
   (advances to the next room), and remove the core. */
static void contra_rom_wall_core_routine_09(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x01u;
    contra_rom_remove_enemy_offscreen(core, x); /* remove_enemy keeps the husk */
}

/* ===== level-2 indoor soldiers (0x15) + their generator (0x19) ==============
   Deterministic (no RANDOM_NUM): soldiers walk in from a fixed side X (0xA8
   right / 0x58 left) at Y=0x6D with a table velocity, and their regular bullet
   is aimed by the soldier's horizontal screen segment. (bank0:2412 generator,
   bank0:3432 soldier, bank7 helpers.) */

/* indoor_soldier_x_velocity_tbl (bank0): {fract, fast} per generator sub-type. */
static const uint8_t contra_indoor_soldier_x_velocity_tbl[4][2] = {
    {0x20u, 0xFFu}, /* running soldier  (-0.875) */
    {0x40u, 0xFFu}, /* jumping soldier  (-0.75) */
    {0x40u, 0xFFu}, /* group of 4       (-0.75) */
    {0x40u, 0xFFu}, /* grenade launcher (-0.75) */
};
/* indoor_bullet_velocity_tbl (bank0): x {fract, fast} per horizontal segment. */
static const uint8_t contra_indoor_bullet_velocity_tbl[7][2] = {
    {0xD4u, 0x00u}, {0x8Du, 0x00u}, {0x46u, 0x00u}, {0x00u, 0x00u},
    {0xBAu, 0xFFu}, {0x73u, 0xFFu}, {0x2Cu, 0xFFu}};
/* far_segment_code_tbl (bank7): X thresholds, segment 6 (far left) .. 0 (right). */
static const uint8_t contra_far_segment_code_tbl[7] = {
    0xFFu, 0x94u, 0x8Cu, 0x84u, 0x7Cu, 0x74u, 0x6Cu};

/* lvl_2_enemy_gen_screen_xx (bank0:2547): 2-byte entries
   {type<<6 | attributes, attack<<7 | delay}. The entry whose delay byte has
   bit7 set ends the cycle: it counts an attack round and restarts at offset 0. */
static const uint8_t contra_l2_enemy_gen_screen_00[] = {0x42u, 0x30u, 0x01u, 0x01u, 0x00u, 0xC0u};
static const uint8_t contra_l2_enemy_gen_screen_01[] = {
    0x46u, 0x30u, 0x81u, 0x50u, 0x01u, 0x10u, 0x00u, 0x30u, 0x00u, 0x10u, 0x01u, 0xE0u};
static const uint8_t contra_l2_enemy_gen_screen_02[] = {0x00u, 0x30u, 0xC5u, 0xA0u};
static const uint8_t contra_l2_enemy_gen_screen_03[] = {0x46u, 0x20u, 0x81u, 0x60u, 0xC3u, 0xE1u};
static const uint8_t contra_l2_enemy_gen_screen_04[] = {
    0x40u, 0x30u, 0x81u, 0x60u, 0x00u, 0x10u, 0x03u, 0x30u,
    0x02u, 0x10u, 0x01u, 0x40u, 0x47u, 0x10u, 0x4Au, 0xE0u};
static const uint8_t *const contra_l2_enemy_gen_screen_tbl[5] = {
    contra_l2_enemy_gen_screen_00, contra_l2_enemy_gen_screen_01, contra_l2_enemy_gen_screen_02,
    contra_l2_enemy_gen_screen_03, contra_l2_enemy_gen_screen_04};
static const size_t contra_l2_enemy_gen_screen_len[5] = {6u, 12u, 4u, 6u, 16u};

/* lvl_4_enemy_gen_screen_* (bank0:2632-2677): Level 4's indoor soldier cycles,
   selected by indoor_enemy_gen_tbl using the generator attributes' level bit. */
static const uint8_t contra_l4_enemy_gen_screen_00[] = {
    0x04u, 0x30u, 0x05u, 0x60u, 0x41u, 0x60u, 0x02u, 0x30u, 0x03u, 0x60u, 0x80u, 0xE0u};
static const uint8_t contra_l4_enemy_gen_screen_01[] = {
    0x4Au, 0x50u, 0xC3u, 0x20u, 0xC2u, 0x20u, 0x04u, 0x20u, 0x05u, 0x50u, 0x47u, 0x50u, 0xC2u, 0xB0u};
static const uint8_t contra_l4_enemy_gen_screen_02[] = {
    0x05u, 0x40u, 0x80u, 0x60u, 0x53u, 0x60u, 0x80u, 0xE0u};
static const uint8_t contra_l4_enemy_gen_screen_03[] = {
    0x57u, 0x60u, 0x40u, 0x60u, 0x41u, 0x60u, 0x40u, 0xE0u};
static const uint8_t contra_l4_enemy_gen_screen_04[] = {
    0x05u, 0x30u, 0x04u, 0x60u, 0x42u, 0xE0u};
static const uint8_t contra_l4_enemy_gen_screen_05[] = {
    0x4Eu, 0x40u, 0x81u, 0x60u, 0x41u, 0x60u, 0x40u, 0xE0u};
static const uint8_t contra_l4_enemy_gen_screen_06[] = {
    0x04u, 0x20u, 0x03u, 0x40u, 0x4Bu, 0x60u, 0x07u, 0x20u, 0x02u, 0x40u, 0x4Bu, 0xE0u};
static const uint8_t contra_l4_enemy_gen_screen_07[] = {
    0x02u, 0x30u, 0x47u, 0x40u, 0x80u, 0x60u, 0x03u, 0x20u, 0x04u, 0xD0u};
static const uint8_t *const contra_l4_enemy_gen_screen_tbl[8] = {
    contra_l4_enemy_gen_screen_00, contra_l4_enemy_gen_screen_01, contra_l4_enemy_gen_screen_02,
    contra_l4_enemy_gen_screen_03, contra_l4_enemy_gen_screen_04, contra_l4_enemy_gen_screen_05,
    contra_l4_enemy_gen_screen_06, contra_l4_enemy_gen_screen_07};
static const size_t contra_l4_enemy_gen_screen_len[8] = {12u, 14u, 8u, 8u, 6u, 8u, 12u, 10u};

/* reverse_enemy_x_direction (bank7): negate the 16-bit X velocity. */
/* reverse_enemy_x_direction (bank7:7919-7927): negate 16-bit enemy X velocity. */
static void contra_rom_reverse_enemy_x_direction(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint16_t v = (uint16_t)(((uint16_t)ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] << 8u) |
                                  ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x]);
    const uint16_t neg = (uint16_t)(0u - (unsigned)v);

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = (uint8_t)(neg & 0xFFu);
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = (uint8_t)(neg >> 8u);
}

/* find_far_segment_for_a (bank7): 6 (far left) .. 0 (far right) for X position. */
/* find_far_segment (bank7:8931-8948): X position to far-segment code 6..0. */
static uint8_t contra_rom_find_far_segment(uint8_t xpos)
{
    int y;

    for (y = 6; y >= 0; --y)
    {
        if (xpos < contra_far_segment_code_tbl[y])
        {
            return (uint8_t)y;
        }
    }
    return 0u;
}

/* init_indoor_enemy_pos_and_vel (bank0): fixed spawn position + table velocity;
   ENEMY_ATTRIBUTES bit0 set = arrives from the left (reverse direction, X=0x58). */
/* init_indoor_enemy_pos_and_vel (bank0:4073-4089): seed indoor enemy spawn X/Y and velocity. */
static void contra_rom_init_indoor_enemy_pos_and_vel(ContraCore *core, uint8_t x, uint8_t kind)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_indoor_soldier_x_velocity_tbl[kind][0];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_indoor_soldier_x_velocity_tbl[kind][1];
    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u)
    {
        contra_rom_reverse_enemy_x_direction(core, x);
        ram[CONTRA_RAM_ENEMY_X_POS + x] = 0x58u; /* from the left */
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_X_POS + x] = 0xA8u; /* from the right */
    }
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0x6Du;
}

/* init_sprite_from_frame (bank0:3479): 3-frame walk cycle (advance every 4th
   FRAME_COUNTER tick), sprite 0x93+frame, flip horizontally when moving left. */
static void contra_rom_init_sprite_from_frame(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    uint8_t attr;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u) == 0u)
    {
        frame = (uint8_t)(frame + 1u);
        if (frame >= 0x03u)
        {
            frame = 0u;
        }
        ram[CONTRA_RAM_ENEMY_FRAME + x] = frame;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(0x93u + frame);
    attr = ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x];
    if ((ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] & 0x80u) != 0u)
    {
        attr = (uint8_t)(attr | 0x40u); /* moving left: flip horizontally */
    }
    else
    {
        attr = (uint8_t)(attr & 0xBFu);
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = attr;
}

/* apply_enemy_velocity_set_bg_priority (bank7): integrate X velocity, remove the
   enemy once it walks off the indoor screen (X>=0xB0 moving right, X<0x50 moving
   left), and set the sprite bg-priority bit by X. Returns true if removed. */
/* apply_indoor_velocity (bank0:4104-4143): integrate X velocity, off-screen removal, bg priority. */
static bool contra_rom_apply_indoor_velocity(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const unsigned accum = (unsigned)ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] +
                           ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x];
    const uint8_t fast = ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x];
    uint8_t newx;
    uint8_t attr;

    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] = (uint8_t)accum;
    newx = (uint8_t)((unsigned)ram[CONTRA_RAM_ENEMY_X_POS + x] + fast + (accum >> 8u));
    ram[CONTRA_RAM_ENEMY_X_POS + x] = newx;

    if ((fast & 0x80u) != 0u)
    {
        if (newx < 0x50u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* remove_enemy keeps the husk */
            return true;
        }
    }
    else if (newx >= 0xB0u)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* remove_enemy keeps the husk */
        return true;
    }

    attr = ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x];
    if ((newx >= 0x60u) && (newx < 0xA0u))
    {
        attr = (uint8_t)(attr & 0xDFu); /* draw in front of background */
    }
    else
    {
        attr = (uint8_t)(attr | 0x20u); /* draw behind background */
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = attr;
    return false;
}

static void contra_rom_apply_indoor_hit_y_velocity(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const unsigned accum = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] +
                           ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x];

    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = (uint8_t)accum;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(
        (unsigned)ram[CONTRA_RAM_ENEMY_Y_POS + x] +
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] +
        (accum >> 8u));
}

static void contra_rom_shared_indoor_soldier_hit_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_disable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x96u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x80u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xFDu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x10u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_shared_indoor_soldier_hit_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const unsigned yv = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] + 0x38u;

    if (contra_rom_apply_indoor_velocity(core, x))
    {
        return;
    }
    contra_rom_apply_indoor_hit_y_velocity(core, x);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)yv;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (yv >> 8u));
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
    }
}

/* create_indoor_bullet (bank0): fire a regular bullet aimed by the firing
   soldier's horizontal segment (a type-0x01 enemy, VAR_1=3; the shared
   enemy-bullet routines then animate and move it). */
/* create_indoor_bullet (bank0:4226-4257): spawn aimed indoor enemy bullet (type 1). */
static void contra_rom_create_indoor_bullet(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t ey = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    uint8_t seg;
    int slot;

    if ((ex >= 0xA0u) || (ex < 0x60u))
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        return;
    }
    slot = contra_rom_find_next_enemy_slot(core);
    if (slot < 0)
    {
        return;
    }
    seg = contra_rom_find_far_segment(ex);
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x01u;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    ram[CONTRA_RAM_ENEMY_Y_POS + slot] = ey;
    ram[CONTRA_RAM_ENEMY_X_POS + slot] = ex;
    ram[CONTRA_RAM_ENEMY_VAR_1 + slot] = 0x03u; /* indoor regular bullet */
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + slot] = contra_indoor_bullet_velocity_tbl[seg][0];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + slot] = contra_indoor_bullet_velocity_tbl[seg][1];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + slot] = 0x40u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + slot] = 0x01u;
}

/* roller (0x11) Y->sprite-size cutoffs (bank0:2895) and per-segment X velocity
   (roller_vel_code_tbl, bank0:4185). */
static const uint8_t contra_roller_sprite_y_cutoff_tbl[3] = {0x7Cu, 0x8Cu, 0x9Cu};
static const uint8_t contra_roller_vel_code_tbl[7][2] = {
    {0x55u, 0x00u}, {0x38u, 0x00u}, {0x1Cu, 0x00u}, {0x00u, 0x00u},
    {0xE4u, 0xFFu}, {0xC8u, 0xFFu}, {0xABu, 0xFFu}};

/* create_roller_with_segment_a (bank0:4158): spawn a roller (type 0x11) at
   (px, py) rolling down toward the player, X velocity from horizontal segment
   seg, Y velocity +0.5. Gated by ENEMY_ATTACK_FLAG. */
static void contra_rom_create_roller_with_segment(
    ContraCore *core, uint8_t px, uint8_t py, uint8_t seg, uint8_t attrs)
{
    uint8_t *const ram = core->ram;
    int slot;

    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        return;
    }
    slot = contra_rom_find_next_enemy_slot(core);
    if (slot < 0)
    {
        return;
    }
    seg = (uint8_t)(seg % 7u);
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x11u;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    ram[CONTRA_RAM_ENEMY_Y_POS + slot] = py;
    ram[CONTRA_RAM_ENEMY_X_POS + slot] = px;
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = attrs;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + slot] = contra_roller_vel_code_tbl[seg][0];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + slot] = contra_roller_vel_code_tbl[seg][1];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + slot] = 0x80u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + slot] = 0x00u;
}

/* create_roller (bank0:4150): roller aimed by its own X position's segment. */
static void contra_rom_create_roller(ContraCore *core, uint8_t px, uint8_t py, uint8_t attrs)
{
    contra_rom_create_roller_with_segment(core, px, py, contra_rom_find_far_segment(px), attrs);
}

/* roller_routine_04 (bank7:7611): show_explosion_a with explosion_type_03
   ({0x36,0x37}) and only 2 sprites; collision disabled on the last one. */
static void contra_rom_roller_routine_explosion(ContraCore *core, uint8_t x)
{
    static const uint8_t explosion_type_03[2] = {0x36u, 0x37u};
    uint8_t *const ram = core->ram;
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
    if (frame >= 2u)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    if ((uint8_t)(frame + 1u) >= 2u)
    {
        contra_rom_disable_enemy_collision(core, x);
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = explosion_type_03[frame];
}

/* roller_routine_00/01 (bank0:2860): start at Y=0x72, then roll down the
   corridor -- the sprite grows (0x99..0x9c) with depth, collision turns on once
   it's close (Y in [0xAC,0xBC)), and it's removed once it rolls past (Y>=0xBC).
   (Level-2 0x11 is the roller; level-1 0x11 is the fortress boss door, which
   has its own dispatch branch.) */
static void contra_rom_roller_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0x72u;
    contra_rom_advance_enemy_routine(core, x);
}

/* roller_routine_01 (bank0:2866-2900): roller sprite size, collision enable, Y removal. */
static void contra_rom_roller_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t ypos = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    uint8_t size = 0u;
    uint8_t ny;

    if (ypos >= contra_roller_sprite_y_cutoff_tbl[2]) { size = 3u; }
    else if (ypos >= contra_roller_sprite_y_cutoff_tbl[1]) { size = 2u; }
    else if (ypos >= contra_roller_sprite_y_cutoff_tbl[0]) { size = 1u; }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(0x99u + size);
    if (size >= 2u)
    {
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x2Eu;
    }
    contra_rom_update_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return; /* rolled off the side and was removed */
    }
    ny = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    if (ny >= 0xBCu)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* rolled past: remove_enemy keeps the husk */
    }
    else if (ny >= 0xACu)
    {
        ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Eu); /* enable player collision */
    }
}

/* ===== level-2 indoor grenade (0x12) ======================================
   A thrown grenade follows a pseudo-3D arc: a height accumulator (VAR_2/VAR_3)
   driven by a height velocity (VAR_4/VAR_B) is layered on a ground-Y (VAR_1)
   that travels toward the player; the on-screen Y is VAR_1 + VAR_3. (ENEMY_VAR_B
   IS ENEMY_ATTACK_DELAY -- the same $0558, so routine_00 seeding ATTACK_DELAY=
   0xFD is the -3 upward throw, and routine_01 adding gravity to it is the arc.)
   (level-1 0x12 is the exploding bridge, a separate future port.) */

/* set_enemy_falling_arc_pos (bank7:8957-8980): advance the height + ground, place the
   sprite at VAR_1+VAR_3, and remove the grenade if it falls off the bottom/left. */
static void contra_rom_set_enemy_falling_arc_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    unsigned acc;
    uint8_t var1;

    acc = (unsigned)ram[CONTRA_RAM_ENEMY_VAR_2 + x] + ram[CONTRA_RAM_ENEMY_VAR_4 + x];
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)acc;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] + ram[CONTRA_RAM_ENEMY_VAR_B + x] + (acc >> 8u));
    acc = (unsigned)ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] + ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x];
    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = (uint8_t)acc;
    var1 = (uint8_t)(
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] + ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (acc >> 8u));
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = var1;
    if (var1 >= 0xF0u)
    {
        contra_rom_clear_enemy(core, x); /* fell below the screen */
        return;
    }
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(var1 + ram[CONTRA_RAM_ENEMY_VAR_3 + x]);
    contra_rom_update_enemy_x_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
    {
        contra_rom_clear_enemy(core, x); /* off the left edge */
    }
}

/* grenade sprite codes by depth size (0..2): {codes..., attrs...} pairs,
   length per size from len_tbl (bank0:3008/2998). */
static const uint8_t contra_grenade_sprite_y_cutoff_tbl[2] = {0x80u, 0x90u};
static const uint8_t contra_grenade_sprite_codes_len_tbl[3] = {4u, 8u, 8u};
static const uint8_t contra_grenade_sprite_codes_00[8] = {
    0xA8u, 0xA9u, 0xA6u, 0xA9u, 0x00u, 0x00u, 0x00u, 0xC0u};
static const uint8_t contra_grenade_sprite_codes_01[16] = {
    0xA4u, 0xA5u, 0xA6u, 0xA5u, 0xA4u, 0xA7u, 0xA6u, 0xA7u,
    0x00u, 0x00u, 0x00u, 0xC0u, 0xC0u, 0x00u, 0x00u, 0xC0u};
static const uint8_t contra_grenade_sprite_codes_02[16] = {
    0xA0u, 0xA1u, 0xA2u, 0xA1u, 0xA0u, 0xA3u, 0xA2u, 0xA3u,
    0x00u, 0x00u, 0x00u, 0xC0u, 0xC0u, 0x00u, 0x00u, 0xC0u};
static const uint8_t *const contra_grenade_sprite_codes_tbl[3] = {
    contra_grenade_sprite_codes_00, contra_grenade_sprite_codes_01, contra_grenade_sprite_codes_02};

/* enemy_launch_grenade (bank0:4195): throw a grenade (type 0x12) from (px, py),
   X velocity by horizontal segment, ground Y velocity +0.5. */
static void contra_rom_enemy_launch_grenade(ContraCore *core, uint8_t px, uint8_t py)
{
    uint8_t *const ram = core->ram;
    const uint8_t seg = (uint8_t)(contra_rom_find_far_segment(px) % 7u);
    int slot;

    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        return;
    }
    slot = contra_rom_find_next_enemy_slot(core);
    if (slot < 0)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x12u;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    ram[CONTRA_RAM_ENEMY_Y_POS + slot] = py;
    ram[CONTRA_RAM_ENEMY_X_POS + slot] = px;
    /* grenade_vel_code_tbl is identical to roller_vel_code_tbl */
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + slot] = contra_roller_vel_code_tbl[seg][0];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + slot] = contra_roller_vel_code_tbl[seg][1];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + slot] = 0x80u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + slot] = 0x00u;
}

/* grenade_routine_00 (bank0:2900): VAR_1 = throw-origin ground Y, VAR_4 = 0,
   VAR_B (= ATTACK_DELAY) = 0xFD (the -3 upward throw). */
static void contra_rom_grenade_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_VAR_B + x] = 0xFDu;
    contra_rom_advance_enemy_routine(core, x);
}

/* grenade_routine_01 (bank0:2932): tumble (sprite scaled by ground-Y depth,
   animated every 8 frames), apply gravity to the height velocity, follow the
   arc, and advance to the explosion once the height passes ground (VAR_3 >= 0). */
static void contra_rom_grenade_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t ground = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    uint8_t size = 0u;
    uint8_t len;
    uint8_t frame;
    unsigned acc;

    if (ground >= contra_grenade_sprite_y_cutoff_tbl[1]) { size = 2u; }
    else if (ground >= contra_grenade_sprite_y_cutoff_tbl[0]) { size = 1u; }
    len = contra_grenade_sprite_codes_len_tbl[size];

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) == 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    }
    frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    if (frame >= len)
    {
        frame = 0u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0u;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_grenade_sprite_codes_tbl[size][frame];
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = contra_grenade_sprite_codes_tbl[size][frame + len];

    acc = (unsigned)ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 0x0Cu; /* gravity on the height velocity */
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)acc;
    ram[CONTRA_RAM_ENEMY_VAR_B + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_B + x] + (acc >> 8u));

    contra_rom_set_enemy_falling_arc_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return; /* removed off-screen */
    }
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) == 0u)
    {
        contra_rom_advance_enemy_routine(core, x); /* landed -> explode */
    }
}

/* grenade_routine_02 (bank0:3029): explode at the bottom -- sound, Y=0xAC, the
   blast's player-collision box via mortar_shot_routine_03 (whose shared tail
   ALSO advances), then advance again: the landing frame ends at the explosion
   routine (RAM 3 -> 5), skipping a visible init_explosion frame. */
static void contra_rom_grenade_routine_02(ContraCore *core, uint8_t x)
{
    contra_play_sound(core, 0x24u);
    core->ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0xACu;
    contra_rom_mortar_shot_routine_03(core, x); /* advances 3 -> 4 */
    contra_rom_advance_enemy_routine(core, x);  /* and 4 -> 5 */
}

/* ===== level-2 grenade launcher (0x17, "seeking guy"), bank0:3680 ==========
   Walks the corridor tracking the closest player; at the player's horizontal
   segment (or a corridor edge) it stops in a firing pose and lobs a burst of
   grenades, then resumes seeking. Holds GRENADE_LAUNCHER_FLAG while alive so the
   generator pauses (cleared when it dies, in begin_enemy_explosion/clear_enemy). */
static const uint8_t contra_indoor_close_segment_tbl[7] = {
    0xFFu, 0xBCu, 0xA4u, 0x8Cu, 0x74u, 0x5Cu, 0x44u};

/* find_close_segment (bank0:4043): 6 (far left) .. 0 (far right) for a player X. */
static uint8_t contra_rom_find_close_segment(uint8_t player_x)
{
    int y;

    for (y = 6; y >= 0; --y)
    {
        if (player_x < contra_indoor_close_segment_tbl[y])
        {
            return (uint8_t)y;
        }
    }
    return 0u;
}

/* set_enemy_var_2_to_closest_x_player (bank0:3787): closest normal-state player
   index -> ENEMY_VAR_2; returns it. */
static uint8_t contra_rom_set_var2_closest_player(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t p0 = ram[CONTRA_RAM_SPRITE_X_POS + 0u];
    const uint8_t p1 = ram[CONTRA_RAM_SPRITE_X_POS + 1u];
    const uint8_t d0 = (p0 >= ex) ? (uint8_t)(p0 - ex) : (uint8_t)(ex - p0);
    const uint8_t d1 = (p1 >= ex) ? (uint8_t)(p1 - ex) : (uint8_t)(ex - p1);
    uint8_t idx = ((p1 != 0u) && ((p0 == 0u) || (d1 < d0))) ? 1u : 0u;

    if (ram[CONTRA_RAM_PLAYER_STATE + idx] != 0x01u)
    {
        idx ^= 0x01u;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = idx;
    return idx;
}

/* grenade_launcher_apply_vel_aim (bank0:3732): walk; at a corridor edge or when
   the walk timer elapses, line up the player's segment and drop into the firing
   pose, queueing grenades only when in the player's segment. */
static void contra_rom_grenade_launcher_apply_vel_aim(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    bool realign;

    contra_rom_init_sprite_from_frame(core, x);
    if ((ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] & 0x80u) != 0u)
    {
        realign = (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x60u); /* left edge */
    }
    else
    {
        realign = (ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xA0u); /* right edge */
    }
    if (!realign)
    {
        if (contra_rom_apply_indoor_velocity(core, x))
        {
            return; /* removed off-screen */
        }
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            return; /* keep walking */
        }
    }
    {
        const uint8_t enemy_seg = contra_rom_find_far_segment(ram[CONTRA_RAM_ENEMY_X_POS + x]);
        const uint8_t pidx = ram[CONTRA_RAM_ENEMY_VAR_2 + x];
        const uint8_t player_seg = contra_rom_find_close_segment(ram[CONTRA_RAM_SPRITE_X_POS + pidx]);
        const bool same_seg = (player_seg == enemy_seg);

        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = same_seg ? 0x38u : 0x18u;
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 1u);
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x04u;
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
            same_seg ? (uint8_t)((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] >> 1u) & 0x03u) : 0u;
    }
}

/* grenade_launcher_routine_00 (bank0:3680-3688): spawn grenade launcher, set flag/vel/delay. */
static void contra_rom_grenade_launcher_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0x01u;
    contra_rom_set_var2_closest_player(core, x);
    contra_rom_init_indoor_enemy_pos_and_vel(core, x, 3u); /* grenade-kind velocity */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x20u);
}

/* grenade_launcher_routine_01 (bank0:3689-3731): grenade launcher posed firing / seek-turn logic. */
static void contra_rom_grenade_launcher_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] == 0u)
    {
        contra_rom_grenade_launcher_apply_vel_aim(core, x);
        return;
    }
    /* firing pose */
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x96u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        /* lob queued grenades on a cadence while posed */
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u)
        {
            return;
        }
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
        {
            return;
        }
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x14u;
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
        contra_rom_enemy_launch_grenade(
            core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x]);
        return;
    }
    /* pose over: resume seeking, turning toward the player */
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
    {
        const uint8_t enemy_seg = contra_rom_find_far_segment(ram[CONTRA_RAM_ENEMY_X_POS + x]);
        const uint8_t pidx = contra_rom_set_var2_closest_player(core, x);
        const uint8_t player_seg = contra_rom_find_close_segment(ram[CONTRA_RAM_SPRITE_X_POS + pidx]);
        const uint8_t want = (player_seg < enemy_seg) ? 0x00u : 0x80u;

        if (((want ^ ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x]) & 0x80u) != 0u)
        {
            contra_rom_reverse_enemy_x_direction(core, x);
        }
    }
}

/* indoor_soldier_routine_00/01 (bank0:3432): a running indoor soldier walks in
   from a side, animates, and fires on a cadence while inside the central firing
   band -- a regular bullet (weapon 0), a grenade (weapon 1), or a roller
   (weapon 2/3). */
static void contra_rom_indoor_soldier_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_init_indoor_enemy_pos_and_vel(core, x, 0u);
    core->ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x08u;
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_indoor_soldier_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_init_sprite_from_frame(core, x);
    if (contra_rom_apply_indoor_velocity(core, x))
    {
        return; /* walked off-screen and was removed */
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x10u;
    if ((ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x68u) || (ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0x98u))
    {
        return; /* outside the firing band */
    }
    {
        const uint8_t weapon = (uint8_t)((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] >> 1u) & 0x03u);

        if (weapon == 0u)
        {
            contra_rom_create_indoor_bullet(core, x); /* regular bullet */
        }
        else if (weapon == 1u)
        {
            /* bank0:3461: inc VAR_1, throw only on ODD counts -- every other
               beat, and the count advances even when the launch itself is
               blocked (e.g. ENEMY_ATTACK_FLAG still 0 after a respawn) */
            ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u);
            if ((ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x01u) != 0u)
            {
                contra_rom_enemy_launch_grenade(
                    core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x]);
            }
        }
        else
        {
            /* roller, spawned 8px below the soldier (bank0:3469) */
            contra_rom_create_roller(
                core, ram[CONTRA_RAM_ENEMY_X_POS + x],
                (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 8u), 0x00u);
        }
    }
}

/* jumping_soldier_y_vel_tbl (bank0:3648): signed per-frame Y deltas over one jump
   arc (up, hang, then down). */
static const int8_t contra_jumping_soldier_y_vel_tbl[20] = {
    -3, -3, -2, -2, -2, -1, -1, -1, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3};

/* jumping_soldier_routine_00 (bank0:3548): decide whether this is the room's one
   red weapon-dropping soldier (only after the first attack round, one per room;
   others get their red bit cleared), then init position/velocity. The red
   weapon drop on death is jumping_soldier_routine_04 (mid-room kills become
   the carried weapon item via play_explosion_sound). */
static void contra_rom_jumping_soldier_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x02u) != 0u)
    {
        if ((ram[CONTRA_RAM_INDOOR_RED_SOLDIER_CREATED] != 0u) ||
            (ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] == 0u))
        {
            ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0xFDu); /* not the red one */
        }
        else
        {
            ram[CONTRA_RAM_INDOOR_RED_SOLDIER_CREATED] = 0x01u;
        }
    }
    contra_rom_init_indoor_enemy_pos_and_vel(core, x, 1u); /* jumping velocity entry */
    contra_rom_advance_enemy_routine(core, x);
}

/* jumping_soldier_routine_01 (bank0:3572): sprite by jump phase, walk
   horizontally, follow the parabolic jump arc, and -- during the landing pause,
   if not red -- fire once at the player via the diagonal aim. */
static void contra_rom_jumping_soldier_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t delay = ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x];
    const uint8_t pal = ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x02u) != 0u) ? 0x05u : 0x00u;
    uint8_t attr;

    if (delay == 0u)
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x97u; /* in the air */
    }
    else if (delay < 0x04u)
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x93u;
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x98u;
    }

    attr = ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x];
    if ((ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] & 0x80u) != 0u)
    {
        attr = (uint8_t)(attr | 0x40u); /* moving left: flip horizontally */
    }
    else
    {
        attr = (uint8_t)(attr & 0xBFu);
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = (uint8_t)((attr & 0xF8u) | pal);

    if (delay != 0u)
    {
        /* landing pause: count down; fire once (non-red) on the firing beat */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = (uint8_t)(delay - 1u);
        if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x02u) != 0u)
        {
            return; /* the red soldier doesn't fire */
        }
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0x08u)
        {
            contra_rom_aim_and_create_enemy_bullet(
                core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
                0x03u, 0x04u, contra_quadrant_aim_dir_01);
        }
        return;
    }

    /* mid-jump: walk, then follow the jump arc. The ROM's @apply_y_vel
       (bank0:3618) steps the Y arc even when the walk just removed the
       soldier -- the husk's frozen Y carries one extra arc step. */
    (void)contra_rom_apply_indoor_velocity(core, x);
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(
        (int)ram[CONTRA_RAM_ENEMY_Y_POS + x] +
        contra_jumping_soldier_y_vel_tbl[ram[CONTRA_RAM_ENEMY_VAR_1 + x] % 20u]);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] >= 0x14u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0u;
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x10u; /* land, pause before next jump */
    }
}

/* group-of-4 (0x18) delay tables (bank0:3856/3899), indexed by
   VAR_2 (fire round 0..2) * 4 + VAR_1 (soldier index 0..3). */
static const uint8_t contra_four_soldiers_delay_running_tbl[12] = {
    0x3Fu, 0x39u, 0x33u, 0x2Du, 0x18u, 0x10u, 0x10u, 0x18u, 0xFFu, 0xFFu, 0xFFu, 0xFFu};
static const uint8_t contra_four_soldiers_firing_delay_tbl[12] = {
    0x01u, 0x07u, 0x0Du, 0x13u, 0x18u, 0x18u, 0x18u, 0x18u, 0x10u, 0x18u, 0x18u, 0x10u};

static uint8_t contra_rom_four_soldiers_delay_offset(const ContraCore *core, uint8_t x)
{
    return (uint8_t)(((core->ram[CONTRA_RAM_ENEMY_VAR_2 + x] << 2) +
                      core->ram[CONTRA_RAM_ENEMY_VAR_1 + x]) % 12u);
}

/* four_soldiers_set_firing_delay (bank0:3874-3883): set group-of-4 firing animation delay from table. */
static void contra_rom_four_soldiers_set_firing_delay(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        contra_four_soldiers_firing_delay_tbl[contra_rom_four_soldiers_delay_offset(core, x)];
}

/* four_soldiers_routine_00/01/02 (bank0:3820): a member of a group of four runs
   in, periodically stops in a firing pose to shoot a segment-aimed bullet, and
   the back two of the four split off in the opposite direction after the first
   shot. */
static void contra_rom_four_soldiers_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_init_indoor_enemy_pos_and_vel(core, x, 2u); /* group velocity entry */
    contra_rom_four_soldiers_set_firing_delay(core, x);
    contra_rom_advance_enemy_routine(core, x);
}

/* four_soldiers_routine_01 (bank0:3827-3849): wait delay, fire on beat, split rear pair. */
static void contra_rom_four_soldiers_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0x04u)
        {
            contra_rom_create_indoor_bullet(core, x); /* fire on the firing beat */
        }
        return;
    }
    /* delay elapsed: the back two soldiers reverse after the first round */
    if ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0x01u) && (ram[CONTRA_RAM_ENEMY_VAR_1 + x] >= 0x02u))
    {
        contra_rom_reverse_enemy_x_direction(core, x);
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        contra_four_soldiers_delay_running_tbl[contra_rom_four_soldiers_delay_offset(core, x)];
    contra_rom_advance_enemy_routine(core, x); /* -> routine_02 (run) */
}

/* four_soldiers_routine_02 (bank0:3862-3873): run, then stop into firing pose, reset. */
static void contra_rom_four_soldiers_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_init_sprite_from_frame(core, x);
    if (contra_rom_apply_indoor_velocity(core, x))
    {
        return; /* ran off-screen and was removed */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x96u; /* stop in the firing pose */
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
    contra_rom_four_soldiers_set_firing_delay(core, x);
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u); /* back to four_soldiers_routine_01 */
}

/* create the generator sub-type soldier (bank0:2485): running (0x15), jumping
   (0x16), group-of-4 (4x 0x18), grenade launcher (0x17). Running/jumping/group
   are ported; the grenade launcher is skipped -- its script entry still
   advances, so the generator cadence and attack-round count stay faithful. */
static void contra_rom_create_indoor_soldier(ContraCore *core, uint8_t stype, uint8_t attrs)
{
    uint8_t type;
    int slot;

    if (stype == 0x02u)
    {
        /* group of four: spawn 4 type-0x18 soldiers, labelled VAR_1 = 3..0
           (@create_group_of_4, bank0:2505). */
        int i;

        for (i = 3; i >= 0; --i)
        {
            slot = contra_rom_find_next_enemy_slot(core);
            if (slot < 0)
            {
                return;
            }
            core->ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x18u;
            contra_rom_initialize_enemy(core, (uint8_t)slot);
            core->ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = attrs;
            core->ram[CONTRA_RAM_ENEMY_VAR_1 + slot] = (uint8_t)i;
        }
        return;
    }

    switch (stype)
    {
        case 0x00u: type = 0x15u; break; /* running indoor soldier */
        case 0x01u: type = 0x16u; break; /* jumping indoor soldier */
        case 0x03u: type = 0x17u; break; /* grenade launcher (seeking guy) */
        default: return;
    }
    slot = contra_rom_find_next_enemy_slot(core);
    if (slot < 0)
    {
        return;
    }
    core->ram[CONTRA_RAM_ENEMY_TYPE + slot] = type;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    core->ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = attrs;
}

/* indoor_soldier_gen_routine_00/01 (bank0:2412): the per-room enemy-cycle
   generator. On odd frames, after its delay elapses, read the next script entry
   and spawn that soldier; the cycle-ending entry counts an attack round and
   restarts, and the generator removes itself after 7 rounds. */
static void contra_rom_indoor_soldier_gen_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x40u;
    contra_rom_advance_enemy_routine(core, x);
}

/* indoor_soldier_gen_routine_01 (bank0:2422-2501): per-room soldier generator reads spawn script. */
static void contra_rom_indoor_soldier_gen_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    const uint8_t *data;
    size_t data_len;
    uint8_t offset;
    uint8_t byte0;
    uint8_t byte1;
    uint8_t attrs;
    uint8_t stype;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) == 0u)
    {
        return; /* generate only on odd frames */
    }
    if (ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] != 0u)
    {
        return; /* a grenade launcher is already on screen */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u)
    {
        if (screen >= (sizeof(contra_l4_enemy_gen_screen_tbl) / sizeof(contra_l4_enemy_gen_screen_tbl[0])))
        {
            return;
        }
        data = contra_l4_enemy_gen_screen_tbl[screen];
        data_len = contra_l4_enemy_gen_screen_len[screen];
    }
    else if (screen < (sizeof(contra_l2_enemy_gen_screen_tbl) / sizeof(contra_l2_enemy_gen_screen_tbl[0])))
    {
        data = contra_l2_enemy_gen_screen_tbl[screen];
        data_len = contra_l2_enemy_gen_screen_len[screen];
    }
    else
    {
        return; /* no generation script for this room */
    }

    offset = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    if ((size_t)(offset + 1u) >= data_len)
    {
        offset = 0u; /* safety: stay in-bounds */
    }
    byte0 = data[offset];
    byte1 = data[offset + 1u];
    attrs = (uint8_t)(byte0 & 0x3Fu);
    stype = (uint8_t)(byte0 >> 6u);
    offset = (uint8_t)(offset + 2u);

    if ((byte1 & 0x80u) != 0u)
    {
        offset = 0u; /* end of cycle: restart and count an attack round */
        ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] =
            (uint8_t)(ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] + 1u);
        if (ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] >= 0x07u)
        {
            contra_rom_clear_enemy(core, x); /* generator removed after 7 rounds */
            return;
        }
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = (uint8_t)(byte1 & 0x7Fu);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = offset;
    contra_rom_create_indoor_soldier(core, stype, attrs);
}

/* roller generator (0x1A), bank0:3905. roller_initial_x_pos_tbl (bank0:3975) maps
   a pattern's x-index to a spawn X; the patterns (roller_gen_init_00/01,
   bank0:3993/4025) are {x-index<<4 | attrs, delay} entries, 0xFF = wrap. A delay
   of 0 spawns the next roller the same frame (a burst across the corridor). */
static const uint8_t contra_roller_initial_x_pos_tbl[7] = {
    0x98u, 0x90u, 0x88u, 0x80u, 0x78u, 0x70u, 0x68u};
static const uint8_t contra_roller_gen_init_00[] = {
    0x00u, 0x00u, 0x10u, 0x00u, 0x20u, 0x00u, 0x30u, 0x00u, 0x40u, 0x00u, 0x50u, 0x00u, 0x60u, 0xF0u,
    0x01u, 0x00u, 0x11u, 0x00u, 0x21u, 0x00u, 0x31u, 0x00u, 0x41u, 0x00u, 0x51u, 0x00u, 0x61u, 0xF0u,
    0x30u, 0x10u, 0x20u, 0x00u, 0x40u, 0x10u, 0x10u, 0x00u, 0x50u, 0x10u, 0x00u, 0x00u, 0x60u, 0xF0u,
    0x00u, 0x00u, 0x60u, 0x10u, 0x10u, 0x00u, 0x50u, 0x10u, 0x20u, 0x00u, 0x40u, 0x10u, 0x30u, 0xF0u,
    0xFFu};
static const uint8_t contra_roller_gen_init_01[] = {
    0x00u, 0x00u, 0x20u, 0x00u, 0x40u, 0x00u, 0x60u, 0xF0u,
    0x10u, 0x00u, 0x30u, 0x00u, 0x50u, 0xF0u, 0xFFu};
static const uint8_t *const contra_roller_gen_init_tbl[2] = {
    contra_roller_gen_init_00, contra_roller_gen_init_01};
static const size_t contra_roller_gen_init_len[2] = {
    sizeof(contra_roller_gen_init_00), sizeof(contra_roller_gen_init_01)};

/* indoor_roller_gen_routine_00 (bank0:3910-3913): roller generator init, set 0x60 delay. */
static void contra_rom_indoor_roller_gen_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x60u;
    contra_rom_advance_enemy_routine(core, x);
}

/* indoor_roller_gen_routine_01 (bank0:3914-3973): roller generator spawns pattern bursts per delay. */
static void contra_rom_indoor_roller_gen_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t *pattern;
    size_t pattern_len;
    uint8_t pidx;
    uint8_t off;
    unsigned guard;

    if (ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] >= 0x07u)
    {
        contra_rom_clear_enemy(core, x); /* rollers stop after 7 rounds */
        return;
    }
    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) == 0u)
    {
        return; /* odd frames only */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    pidx = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x07u);
    if (pidx >= 2u)
    {
        pidx = 0u; /* only two patterns exist */
    }
    pattern = contra_roller_gen_init_tbl[pidx];
    pattern_len = contra_roller_gen_init_len[pidx];

    off = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    for (guard = 0u; guard < 32u; ++guard) /* burst until a non-zero delay */
    {
        uint8_t posattr;
        uint8_t roller_attr;
        uint8_t xidx;
        uint8_t delay;

        if ((off + 1u >= pattern_len) || (pattern[off] == 0xFFu))
        {
            off = 0u; /* wrap to the start of the pattern */
        }
        posattr = pattern[off];
        roller_attr = (uint8_t)(posattr & 0x0Fu);
        xidx = (uint8_t)((posattr >> 4u) % 7u);
        delay = pattern[off + 1u];
        off = (uint8_t)(off + 2u);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = delay;
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = off;
        contra_rom_create_roller_with_segment(
            core, contra_roller_initial_x_pos_tbl[xidx], 0x70u, xidx, roller_attr);
        if (delay != 0u)
        {
            break; /* wait before the next roller */
        }
    }
}

/* ===== level-2 boss room: wall cannon (0x08) + wall plating (0x0A) ==========
   The fortress targets (shared with the level-1 boss). Both draw as 4x4 super-
   tiles cached in l2_supertile and re-drawn each render frame. The cannon opens,
   fires a 3-bullet downward spread, and closes on a loop; the 4 platings are the
   shields -- each destroyed plating bumps WALL_PLATING_DESTROYED_COUNT, and once
   all 4 are gone the boss eye wakes. (bank7:9269/9397.) */

/* create_enemy_bullet_angle_a (bank7:9796): bullet whose quadrant is derived from
   its 24-direction angle (type in bits 7-5, angle in bits 4-0). Falls through to
   create_enemy_bullet_if_attack_enabled: the level-1 boss cannonball (type 1)
   always fires, every other bullet only when ENEMY_ATTACK_FLAG is set. Returns
   true if a bullet was actually created. */
static bool contra_rom_create_enemy_bullet_angle_a(
    ContraCore *core, uint8_t type_angle, uint8_t speed, uint8_t px, uint8_t py)
{
    const uint8_t btype = (uint8_t)(type_angle >> 5u);
    const uint8_t angle = (uint8_t)(type_angle & 0x1Fu);
    uint8_t quadrant = 0u;

    core->ram[0x06u] = speed;
    if ((angle >= 0x07u) && (angle < 0x12u))
    {
        quadrant = 2u; /* left half */
    }
    if (angle >= 0x0Du)
    {
        quadrant = (uint8_t)(quadrant + 1u); /* upper half */
    }
    if ((btype != 0x01u) && (core->ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u))
    {
        return false;
    }
    return contra_rom_create_enemy_bullet(core, btype, angle, quadrant, speed, px, py);
}

/* animate_wall_cannon (bank7:9319): show the current open/close frame super-tile
   and set the 6-frame step delay. */
/* draw_enemy_supertile_a (bank7:1351 update_nametable_supertile) budget: a
   full 32x32 super-tile stamp fails when GRAPHICS_BUFFER_OFFSET is already
   >= 0x40 and costs 0x21 bytes -- the boss-room platings/cannons sharing a
   deploy beat stagger off each other this way. */
static bool contra_rom_enemy_supertile_draw_budget(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] >= 0x40u)
    {
        return false;
    }
    ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] =
        (uint8_t)(ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] + 0x21u);
    return true;
}

/* animate_wall_cannon (bank7:9399): draw_enemy_supertile_a_set_delay -- a
   failed draw forces ANIMATION_DELAY=1 (retry next frame) and reports it. */
static bool contra_rom_animate_wall_cannon(ContraCore *core, uint8_t x)
{
    if (!contra_rom_enemy_supertile_draw_budget(core))
    {
        core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
        return false;
    }
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x06u;
    core->l2_supertile[x] = core->ram[CONTRA_RAM_ENEMY_FRAME + x];
    return true;
}

static void contra_rom_wall_cannon_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x08u; /* real HP held in VAR_1 */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x50u);
}

/* wall_cannon_routine_01 (bank7:9295-9317): open cannon: animate, become hittable + arm attack. */
static void contra_rom_wall_cannon_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    if (!contra_rom_animate_wall_cannon(core, x)) /* draw open frame FRAME */
    {
        return; /* buffer full: retry next frame */
    }
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x02u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        return;
    }
    /* fully open: become hittable, queue the attack */
    ram[CONTRA_RAM_ENEMY_HP + x] = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x04u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x40u);
}

static const uint8_t contra_wall_cannon_bullet_x_offset[3] = {0xF8u, 0x00u, 0x08u};
static const uint8_t contra_wall_cannon_bullet_type_angle[3] = {0x48u, 0x46u, 0x44u};

/* wall_cannon_routine_02 (bank7:9325-9353): fire 3-bullet downward spread, then start closing. */
static void contra_rom_wall_cannon_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] == 0u)
        {
            int i;

            for (i = 2; i >= 0; --i) /* 3-bullet downward spread */
            {
                contra_rom_create_enemy_bullet_angle_a(
                    core, contra_wall_cannon_bullet_type_angle[i], 0x07u,
                    (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + contra_wall_cannon_bullet_x_offset[i]),
                    (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x08u));
            }
        }
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_ENEMY_HP + x]; /* save HP */
    ram[CONTRA_RAM_ENEMY_HP + x] = 0xF1u; /* hittable but invulnerable while closing */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x06u);
}

/* wall_cannon_routine_03 (bank7:9363-9385): animate closing, weapon-scaled reopen delay. */
static void contra_rom_wall_cannon_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    if (!contra_rom_animate_wall_cannon(core, x)) /* draw closing frame */
    {
        return; /* buffer full: retry next frame */
    }
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
        return;
    }
    /* fully closed: wait (longer vs a weaker weapon), then reopen */
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] < 0x02u) ? 0xC0u : 0x60u;
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u); /* -> wall_cannon_routine_01 */
}

static void contra_rom_record_destroyed_structure(ContraCore *core, uint8_t x);

/* wall_cannon_routine_04 (bank7:9387-9391): draw destroyed super-tile 0x05,
   then advance into the appended in-place explosion trio (bank7:9351-9353). */
static void contra_rom_wall_cannon_routine_04(ContraCore *core, uint8_t x)
{
    if (!contra_rom_enemy_supertile_draw_budget(core))
    {
        return; /* draw failed: the routine re-runs next frame */
    }
    core->l2_supertile[x] = 0x05u; /* destroyed super-tile */
    contra_rom_record_destroyed_structure(core, x);
    contra_rom_advance_enemy_routine(core, x);
}

/* wall_plating_routine_00 (bank7:9407-9409): set 0x80 deploy delay and advance routine. */
static void contra_rom_wall_plating_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x80u);
}

/* wall_plating_routine_01 (bank7:9412-9428): deploy plating frames, then HP=0x0A target. */
static void contra_rom_wall_plating_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x04u;
    if (!contra_rom_enemy_supertile_draw_budget(core))
    {
        /* draw_enemy_supertile_a_set_delay failure: retry next frame -- this is
           what splits the four platings' shared deploy beat into two staggered
           pairs (only two 0x21-byte stamps fit under the 0x40 check). */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
        return;
    }
    core->l2_supertile[x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 0x03u); /* deploy frame */
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x02u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_HP + x] = 0x0Au; /* now a destructible target */
    contra_rom_advance_enemy_routine(core, x); /* -> routine_02 (idle target) */
}

/* wall_plating_routine_03 (bank7:9436-9443): destroyed super-tile, bump destroyed count, explode. */
/* Remember a destroyed boss-room housing's wall position so the destroyed super-tile
   (index 5) keeps being drawn after the enemy explodes and its slot is freed -- the ROM
   leaves the destroyed tile on the nametable, but the port re-composes the wall each
   frame, so it must redraw it from this record. */
static void contra_rom_record_destroyed_structure(ContraCore *core, uint8_t x)
{
    if (core->l2_destroyed_struct_count < 8u)
    {
        core->l2_destroyed_struct_x[core->l2_destroyed_struct_count] =
            core->ram[CONTRA_RAM_ENEMY_X_POS + x];
        core->l2_destroyed_struct_y[core->l2_destroyed_struct_count] =
            core->ram[CONTRA_RAM_ENEMY_Y_POS + x];
        core->l2_destroyed_struct_count = (uint8_t)(core->l2_destroyed_struct_count + 1u);
    }
}

static void contra_rom_wall_plating_routine_03(ContraCore *core, uint8_t x)
{
    if (!contra_rom_enemy_supertile_draw_budget(core))
    {
        return; /* draw failed: the routine re-runs next frame */
    }
    core->l2_supertile[x] = 0x05u; /* destroyed super-tile */
    contra_rom_record_destroyed_structure(core, x);
    core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] =
        (uint8_t)(core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] + 1u);
    /* bank7:9523: advance into the appended in-place explosion trio */
    contra_rom_advance_enemy_routine(core, x);
}

/* ===== level-2 boss room: boss eye (0x10) + eye projectile (0x1B) ===========
   Once all 4 platings are gone the eye wakes, drifts across the corridor (HP=1
   per hit, real HP 0x10 in VAR_1), and lobs aimed sphere projectiles (0x1B) that
   grow + become hittable as they near the player. */

/* set_bullet_velocities (bank7:9893): set an existing enemy slot's X/Y velocity
   from a 24-direction angle + quadrant (the velocity half of create_enemy_bullet). */
static void contra_rom_set_bullet_velocities(
    ContraCore *core, uint8_t slot, uint8_t angle, uint8_t quadrant, uint8_t speed)
{
    uint8_t *const ram = core->ram;
    const uint8_t idx = contra_bullet_fract_vel_dir_lookup_tbl[angle % 24u];
    uint16_t xv = contra_rom_adjust_bullet_velocity(contra_bullet_fract_vel_tbl[idx + 1u], speed);
    uint16_t yv = contra_rom_adjust_bullet_velocity(contra_bullet_fract_vel_tbl[idx], speed);

    if ((quadrant & 0x01u) != 0u) { yv = (uint16_t)(0u - yv); }
    if ((quadrant & 0x02u) != 0u) { xv = (uint16_t)(0u - xv); }
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + slot] = (uint8_t)(yv >> 8u);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + slot] = (uint8_t)yv;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + slot] = (uint8_t)(xv >> 8u);
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + slot] = (uint8_t)xv;
}

/* get_quadrant_aim_dir_for_player (bank7): aim-direction nibble from (sx,sy)
   toward the closest normal-state player (indoor Y fixed at 0xB0; screen-center
   if neither is normal); *quadrant gets $07. */
/* aim_at_player (bank7:10425-10455): quadrant aim nibble toward closest normal player. */
static uint8_t contra_rom_aim_at_player(
    ContraCore *core, uint8_t sx, uint8_t sy, const uint8_t *tbl, uint8_t *quadrant)
{
    uint8_t *const ram = core->ram;
    const uint8_t p0 = ram[CONTRA_RAM_SPRITE_X_POS + 0u];
    const uint8_t p1 = ram[CONTRA_RAM_SPRITE_X_POS + 1u];
    /* player_enemy_x_dist (bank7:8844): closest player by |X dist| from the source,
       with non-normal players forced to max distance (0xFE/0xFF) via PLAYER_STATE so
       they're never chosen -- NOT an x==0 "absent" proxy, which mis-picked a normal
       player legitimately standing at screen x==0. */
    uint8_t d0 = (p0 >= sx) ? (uint8_t)(p0 - sx) : (uint8_t)(sx - p0);
    uint8_t d1 = (p1 >= sx) ? (uint8_t)(p1 - sx) : (uint8_t)(sx - p1);
    uint8_t idx;
    uint8_t tx;
    uint8_t ty;

    if (ram[CONTRA_RAM_PLAYER_STATE + 0u] != 0x01u) { d0 = 0xFEu; }
    if (ram[CONTRA_RAM_PLAYER_STATE + 1u] != 0x01u) { d1 = 0xFFu; }
    idx = (d1 < d0) ? 1u : 0u;

    if (ram[CONTRA_RAM_PLAYER_STATE + idx] != 0x01u) { idx ^= 0x01u; }
    if (ram[CONTRA_RAM_PLAYER_STATE + idx] != 0x01u)
    {
        tx = 0x80u;
        ty = 0xFFu;
    }
    else
    {
        tx = ram[CONTRA_RAM_SPRITE_X_POS + idx];
        ty = (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ? 0xB0u : ram[CONTRA_RAM_SPRITE_Y_POS + idx];
    }
    return contra_rom_get_quadrant_aim_dir(sx, sy, tx, ty, tbl, quadrant);
}

/* generate_enemy_at_pos (bank7:8448): spawn an enemy of `type` at the generating
   enemy's position. */
static void contra_rom_generate_enemy_at_pos(ContraCore *core, uint8_t gen_slot, uint8_t type)
{
    uint8_t *const ram = core->ram;
    const int slot = contra_rom_find_next_enemy_slot(core);

    if (slot < 0)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = type;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    ram[CONTRA_RAM_ENEMY_Y_POS + slot] = ram[CONTRA_RAM_ENEMY_Y_POS + gen_slot];
    ram[CONTRA_RAM_ENEMY_X_POS + slot] = ram[CONTRA_RAM_ENEMY_X_POS + gen_slot];
}

static const uint8_t contra_boss_eye_sprite_code_tbl[8] = {
    0x5Du, 0x5Eu, 0x5Fu, 0x5Eu, 0x60u, 0x61u, 0x62u, 0x61u};
static const uint8_t contra_boss_eye_attack_delay_tbl[4] = {0x70u, 0x50u, 0x40u, 0x28u};

/* boss_eye_routine_00 (bank0:2678-2690): set animation delay 0x40, advance routine. */
static void contra_rom_boss_eye_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x40u);
}

/* boss_eye_routine_01 (bank0:2691-2710): after platings destroyed, init boss vel/HP/collision. */
static void contra_rom_boss_eye_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] < 0x04u)
    {
        return; /* shields still up */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x40u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x10u; /* real HP */
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Eu); /* enable collision */
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x20u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0xC0u);
}

/* boss_eye_routine_02 (bank0:2712-2761): animate sprite, drift/bounce, fire eye projectile. */
static void contra_rom_boss_eye_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t base = 0u;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x08u) != 0u)
        {
            base = 4u; /* flash the hit frames */
        }
    }
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        contra_boss_eye_sprite_code_tbl[(base + ((ram[CONTRA_RAM_ENEMY_FRAME + x] >> 3u) & 0x03u)) & 0x07u];

    contra_rom_update_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    {
        const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
        const bool moving_left = (ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] & 0x80u) != 0u;

        if (moving_left ? (ex < 0x50u) : (ex >= 0xB0u))
        {
            contra_rom_reverse_enemy_x_direction(core, x); /* bounce off the edges */
        }
    }
    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    {
        const uint8_t ws = ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH];

        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = contra_boss_eye_attack_delay_tbl[(ws < 4u) ? ws : 3u];
    }
    contra_rom_generate_enemy_at_pos(core, x, 0x1Bu); /* fire an eye projectile */
}

/* boss_eye_routine_03 (bank0:2779): reached on every hit (eye HP=1). Decrement
   the real HP (VAR_1); if it's gone the boss is defeated, otherwise flash and
   resume drifting. */
/* boss_eye_routine_03 (bank0:2737): one "kill" = one real-HP tick (VAR_1); the
   metal ting, HP back to 1, flash, and back to the drift -- or, exhausted,
   advance to boss_defeated_routine. */
static void contra_rom_boss_eye_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0u)
    {
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x01u)
        {
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x52u;
        }
        contra_play_sound(core, 0x16u); /* bullet on metal */
        ram[CONTRA_RAM_ENEMY_HP + x] = 0x01u;
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x20u; /* flash red */
        contra_rom_set_enemy_routine_to_a(core, x, 0x03u); /* -> routine_02 (drift) */
        return;
    }
    contra_rom_advance_enemy_routine(core, x); /* -> boss_defeated_routine */
}

/* boss_defeated_routine (bank7:7566), the eye's RAM routine 05: APU init,
   sound_57, level_boss_defeated (DELAY_TIME_LOW=0xFF + BOSS_DEFEATED_FLAG),
   the faithful destroy_all_enemies, then fall into the in-place
   init_explosion. */
static void contra_rom_boss_eye_defeated_routine(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_init_apu_channels(core);
    contra_play_sound(core, 0x57u); /* sound_57: boss destroyed */
    ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0xFFu;
    ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
    contra_rom_destroy_all_enemies(core, (int)x);
    contra_rom_enemy_routine_init_explosion_inplace(core, x);
}

/* boss_eye_routine_06 (bank0:2818): hide the sprite, set the level-end delay
   to 0x60, and remove the eye (husk kept). */
static void contra_rom_boss_eye_routine_06(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u;
    ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x60u;
    ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x00u;
    contra_rom_remove_enemy_offscreen(core, x);
}

static const uint8_t contra_boss_gemini_sprite_tbl[6] = {0x68u, 0x69u, 0x6Au, 0x68u, 0x6Bu, 0x6Cu};
static const uint8_t contra_boss_gemini_attack_delay_tbl[4] = {0x8Au, 0xA9u, 0x63u, 0xD7u};

/* boss_gemini_routine_00 (bank0:5448-5470): seed real HP, base X, movement
   fraction, initial attack delay, and the pre-appearance delay. */
static void contra_rom_boss_gemini_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0x0Au;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_ENEMY_X_POS + x];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x80u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x80u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x40u);
}

/* boss_gemini_routine_01 (bank0:5472-5486): wait for all three Level 4 boss
   platings, count down the startup delay, enable collision, and start moving. */
static void contra_rom_boss_gemini_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] < 0x03u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0xA0u;
    /* enable_bullet_enemy_collision: clears ONLY bit 7 -- bit 0 (player
       collision skip) stays set (sw 0x8F -> 0x0F) */
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x20u);
}

/* boss_gemini_routine_02 (bank0:5488-5614): animate the helmets, fire spinning
   bubbles when the attack delay elapses, move the paired helmets away/together,
   and pause at the endpoints with collision state matching the ROM. */
static void contra_rom_boss_gemini_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t sprite_offset_flag;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) == 0u)
    {
        uint8_t frame = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);

        if (frame >= 0x03u)
        {
            frame = 0u;
        }
        ram[CONTRA_RAM_ENEMY_FRAME + x] = frame;
    }

    sprite_offset_flag = ram[CONTRA_RAM_ENEMY_VAR_3 + x];
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
        if ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x01u) != 0u)
        {
            sprite_offset_flag ^= 0x01u;
        }
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_boss_gemini_sprite_tbl[
        (ram[CONTRA_RAM_ENEMY_FRAME + x] + ((sprite_offset_flag != 0u) ? 3u : 0u)) % 6u];

    if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] == 0u)
        {
            /* bank0:5520: RN += FC (re-randomize), then the pick is the NEW
               value >> 1 & 3 (the lsr runs on the stored sum) */
            const uint8_t sum =
                (uint8_t)(ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]);
            uint8_t delay = contra_boss_gemini_attack_delay_tbl[(sum >> 1u) & 0x03u];
            const uint8_t strength_delta = (uint8_t)(ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] << 3u);

            ram[CONTRA_RAM_RANDOM_NUM] = sum;
            delay = (uint8_t)(delay - strength_delta);
            ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = delay;
            contra_rom_generate_enemy_at_pos(core, x, 0x1Du);
        }
    }

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            goto set_x_pos;
        }
        ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] | 0x81u);
    }

    {
        const unsigned frac =
            (unsigned)ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] +
            ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x];
        uint8_t offset;

        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)frac;
        offset = (uint8_t)(
            ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] +
            ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] +
            (frac >> 8u));
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = offset;
        if ((ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] & 0x80u) == 0u)
        {
            if (offset >= 0x30u)
            {
                ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x20u;
                contra_rom_reverse_enemy_x_direction(core, x);
            }
        }
        else if ((offset & 0x80u) != 0u)
        {
            ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0x00u;
            ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u;
            /* enable_bullet_enemy_collision: bit 7 only (sw 0x8F -> 0x0F) */
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu);
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x30u;
            contra_rom_reverse_enemy_x_direction(core, x);
        }
    }

set_x_pos:
    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) == 0u)
    {
        ram[CONTRA_RAM_ENEMY_X_POS + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] + ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x]);
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_X_POS + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x]);
    }
}

/* boss_gemini_routine_03 (bank0:5631-5660): each hit decrements VAR_4 real HP;
   nonfinal hits reset HP to 1, flash/low-HP state, play metal hit, and resume. */
static void contra_rom_boss_gemini_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    }
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] == 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] < 0x07u)
    {
        if (ram[CONTRA_RAM_ENEMY_VAR_4 + x] == 0x01u)
        {
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x52u;
        }
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x01u;
    }
    ram[CONTRA_RAM_ENEMY_HP + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x10u;
    contra_play_sound(core, 0x16u);
    contra_rom_set_enemy_routine_to_a(core, x, 0x03u);
}

static void contra_rom_boss_door_routine_02(ContraCore *core, uint8_t x);
static void contra_rom_boss_door_routine_clear_sprite(ContraCore *core, uint8_t x);

/* boss_gemini_routine_04 (bank0:5665): decrement the two-helmet count; the
   LAST helmet jumps to boss_defeated_routine (sound 0x57, DELAY=0xFF,
   destroy_all, then init_explosion); an earlier one runs init_explosion in
   place. Both end up ADVANCED to the explosion routine. */
static void contra_rom_boss_gemini_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_WALL_CORE_REMAINING] = (uint8_t)(ram[CONTRA_RAM_WALL_CORE_REMAINING] - 1u);
    if (ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0u)
    {
        contra_rom_boss_door_routine_02(core, x); /* boss_defeated_routine */
        return;
    }
    contra_rom_enemy_routine_init_explosion_inplace(core, x);
}

/* boss_gemini_routine_06 (bank0:5677): one helmet left -> plain remove (husk);
   both destroyed -> clear sprite and set the level-end auto-move delay 0x60. */
static void contra_rom_boss_gemini_routine_06(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_WALL_CORE_REMAINING] != 0u)
    {
        contra_rom_remove_enemy(core, x);
        return;
    }
    contra_rom_boss_door_routine_clear_sprite(core, x);
    ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x60u; /* set_delay_remove_enemy */
    ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x00u;
    contra_rom_remove_enemy(core, x);
}

static const uint8_t contra_spinning_bubbles_speed_tbl[4] = {0x01u, 0x03u, 0x04u, 0x05u};
static const uint8_t contra_spinning_bullet_spin_tbl[4] = {0x08u, 0x06u, 0x04u, 0x02u};
static const uint8_t contra_spinning_bullet_vel_tbl[60] = {
    0x00u, 0x00u, 0x63u, 0x00u, 0xC0u, 0x00u, 0x0Fu, 0x01u, 0x4Bu, 0x01u, 0x72u, 0x01u,
    0x7Eu, 0x01u, 0x72u, 0x01u, 0x4Bu, 0x01u, 0x0Fu, 0x01u, 0xC0u, 0x00u, 0x63u, 0x00u,
    0x00u, 0x00u, 0x9Du, 0xFFu, 0x40u, 0xFFu, 0xF1u, 0xFEu, 0xB5u, 0xFEu, 0x8Eu, 0xFEu,
    0x82u, 0xFEu, 0x8Eu, 0xFEu, 0xB5u, 0xFEu, 0xF1u, 0xFEu, 0x40u, 0xFFu, 0x9Du, 0xFFu,
    0x00u, 0x00u, 0x63u, 0x00u, 0xC0u, 0x00u, 0x0Fu, 0x01u, 0x4Bu, 0x01u, 0x72u, 0x01u};

static uint8_t contra_rom_get_quadrant_aim_dir_for_player(
    ContraCore *core, uint8_t sx, uint8_t sy, uint8_t player_idx,
    const uint8_t *tbl, uint8_t *quadrant);
static uint8_t contra_rom_get_rotate_dir(
    ContraCore *core, uint8_t x, uint8_t aim, uint8_t quadrant, uint8_t table_idx,
    uint8_t *target_out);
static bool contra_rom_aim_var_1(ContraCore *core, uint8_t x, uint8_t table_idx, uint8_t player_idx);

/* spinning_bullet_vel_tbl (bank0:5784): the X velocity reads the SAME table 12
   bytes (6 directions = 90 degrees) ahead of the Y read -- a shared sine table.
   The wheel is 24 directions (ENEMY_VAR_1 0-23), so the table runs 0x3C bytes. */
static void contra_rom_set_spinning_bubble_velocity_from_var1(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const size_t y = (size_t)ram[CONTRA_RAM_ENEMY_VAR_1 + x] * 2u;
    const size_t xoff = y + 12u;

    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = contra_spinning_bullet_vel_tbl[y];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = contra_spinning_bullet_vel_tbl[y + 1u];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_spinning_bullet_vel_tbl[xoff];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_spinning_bullet_vel_tbl[xoff + 1u];
}

/* spinning_bubbles_routine_00 (bank0:5693-5720): choose spin cadence from frame
   low bits, aim at the closest player, seed velocity, and start adjustment delay. */
static void contra_rom_spinning_bubbles_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t player = contra_rom_player_enemy_x_dist(core, x);
    const uint8_t speed =
        contra_spinning_bubbles_speed_tbl[ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u];
    uint8_t quadrant = 0u;
    uint8_t aim;
    uint8_t target = 0u;

    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = player;
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = (uint8_t)(ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u);
    aim = contra_rom_get_quadrant_aim_dir_for_player(
        core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
        player, contra_quadrant_aim_dir_01, &quadrant);
    /* set_bullet_velocities (bank7:9976): the aim nibble's base velocity scaled
       by the speed code (0.75x..1.62x), negated per quadrant */
    {
        const uint8_t idx = contra_bullet_fract_vel_dir_lookup_tbl[aim % 24u];
        uint16_t xv = contra_rom_adjust_bullet_velocity(contra_bullet_fract_vel_tbl[idx + 1u], speed);
        uint16_t yv = contra_rom_adjust_bullet_velocity(contra_bullet_fract_vel_tbl[idx], speed);

        if ((quadrant & 0x01u) != 0u) { yv = (uint16_t)(0u - yv); }
        if ((quadrant & 0x02u) != 0u) { xv = (uint16_t)(0u - xv); }
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = (uint8_t)(yv >> 8u);
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)yv;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = (uint8_t)(xv >> 8u);
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = (uint8_t)xv;
    }
    /* get_rotate_dir folds the nibble + quadrant into the 24-step wheel; only
       the target direction ($0c) is kept as the spin origin */
    (void)contra_rom_get_rotate_dir(core, x, aim, quadrant, 1u, &target);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = target;
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x20u;
    contra_rom_advance_enemy_routine(core, x);
}

/* spinning_bubbles_routine_01 (bank0:5726-5781): spin sprite 0x6D..0x72, move,
   and periodically nudge aim toward the originally selected player. */
static void contra_rom_spinning_bubbles_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t spin_idx = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x03u);

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] >= contra_spinning_bullet_spin_tbl[spin_idx])
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)((ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u) % 6u);
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(0x6Du + ram[CONTRA_RAM_ENEMY_FRAME + x]);
    contra_rom_update_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u) || (ram[CONTRA_RAM_ENEMY_VAR_3 + x] >= 0x14u))
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x08u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 1u);
    /* aim_var_1_for_quadrant_aim_dir_01: rotate VAR_1 one step on the 24-dir
       wheel toward the ORIGINAL closest player (VAR_2). The ROM's carry exits
       without retuning when no rotation was needed OR the step landed exactly
       on the target; only an in-progress rotation re-reads the sine table. */
    if (!contra_rom_aim_var_1(core, x, 1u, ram[CONTRA_RAM_ENEMY_VAR_2 + x]))
    {
        ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] | 0x03u);
        contra_rom_set_spinning_bubble_velocity_from_var1(core, x);
    }
}

static const uint8_t contra_red_blue_soldier_init_pos_tbl[8] = {
    0x9Cu, 0xF0u, 0x9Cu, 0x10u, 0x61u, 0xF0u, 0x61u, 0x10u};
static const uint8_t contra_red_blue_soldier_init_vel_tbl[4] = {0x00u, 0xFFu, 0x00u, 0x01u};
static const uint8_t contra_blue_soldier_jmp_x_vel_tbl[4] = {0xC0u, 0xFFu, 0x40u, 0x00u};
static const uint8_t contra_red_blue_soldier_data_tbl[28] = {
    0x00u, 0x01u, 0x02u, 0x03u, 0xD0u, 0x06u, 0x07u, 0xA0u,
    0x04u, 0x05u, 0xC0u, 0x00u, 0x01u, 0xB0u, 0x02u, 0x03u,
    0xD0u, 0x04u, 0x05u, 0x06u, 0x07u, 0xD0u, 0x00u, 0x01u,
    0x02u, 0x03u, 0xFEu, 0xFFu};

static uint8_t contra_rom_player_enemy_x_distance(const ContraCore *core, uint8_t x, uint8_t *player_index)
{
    const uint8_t ex = core->ram[CONTRA_RAM_ENEMY_X_POS + x];
    uint8_t best = 0xFFu;
    uint8_t best_index = 0u;
    uint8_t p;

    for (p = 0u; p < 2u; ++p)
    {
        if (core->ram[CONTRA_RAM_PLAYER_STATE + p] == 0x01u)
        {
            const uint8_t px = core->ram[CONTRA_RAM_SPRITE_X_POS + p];
            const uint8_t dist = (px >= ex) ? (uint8_t)(px - ex) : (uint8_t)(ex - px);

            if (dist < best)
            {
                best = dist;
                best_index = p;
            }
        }
    }
    *player_index = best_index;
    return best;
}

static void contra_rom_red_blue_soldier_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x03u);
    const uint8_t pos_idx = (uint8_t)(attr * 2u);
    const uint8_t vel_idx = (uint8_t)((attr & 0x01u) * 2u);

    ram[CONTRA_RAM_ENEMY_Y_POS + x] = contra_red_blue_soldier_init_pos_tbl[pos_idx];
    ram[CONTRA_RAM_ENEMY_X_POS + x] = contra_red_blue_soldier_init_pos_tbl[pos_idx + 1u];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_red_blue_soldier_init_vel_tbl[vel_idx];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_red_blue_soldier_init_vel_tbl[vel_idx + 1u];
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_red_blue_soldier_set_run_frame(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u) == 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)((ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u) % 3u);
    }
}

static void contra_rom_red_blue_soldier_set_bg_priority(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t attr = (uint8_t)(ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0xDFu);

    if ((ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xDCu) || (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x24u))
    {
        attr = (uint8_t)(attr | 0x20u);
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = attr;
}

static void contra_rom_blue_soldier_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t player_index;

    contra_rom_red_blue_soldier_set_run_frame(core, x);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(0x85u + ram[CONTRA_RAM_ENEMY_FRAME + x]);
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u) ? 0x07u : 0x47u;
    contra_rom_red_blue_soldier_set_bg_priority(core, x);
    contra_rom_update_enemy_pos(core, x);
    if ((ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xD8u) || (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x28u))
    {
        return;
    }
    if (contra_rom_player_enemy_x_distance(core, x, &player_index) < 0x10u)
    {
        (void)player_index;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
    }
}

static void contra_rom_blue_soldier_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(0x88u + ram[CONTRA_RAM_ENEMY_FRAME + x]);
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x03u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0xDFu);
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Eu);
    {
        const uint8_t vel_idx = (uint8_t)((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) * 2u);

        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_blue_soldier_jmp_x_vel_tbl[vel_idx];
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_blue_soldier_jmp_x_vel_tbl[vel_idx + 1u];
    }
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xFFu;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x10u);
}

static void contra_rom_blue_soldier_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x8Au;
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x8Bu;
    }
    contra_rom_add_10_to_enemy_y_fract_vel(core, x);
    contra_rom_update_enemy_pos(core, x);
}

static void contra_rom_red_soldier_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t player_index;
    uint8_t attack_dist;

    contra_rom_red_blue_soldier_set_run_frame(core, x);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = (uint8_t)(0x8Cu + ram[CONTRA_RAM_ENEMY_FRAME + x]);
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u) ? 0x06u : 0x46u;
    contra_rom_red_blue_soldier_set_bg_priority(core, x);
    contra_rom_update_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        return;
    }
    if ((ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xD8u) || (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x28u))
    {
        return;
    }
    attack_dist = ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x02u) != 0u) ? 0x30u : 0x10u;
    if (contra_rom_player_enemy_x_distance(core, x, &player_index) < attack_dist)
    {
        (void)player_index;
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x8Fu;
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x03u;
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x10u;
        contra_rom_advance_enemy_routine(core, x);
    }
}

static void contra_rom_red_soldier_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] == 0x2Cu)
        {
            ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0xF7u);
        }
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x90u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_1 + x] & 0x80u) != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
        contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x30u;
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] | 0x08u);
    contra_rom_aim_and_create_enemy_bullet(
        core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x],
        0x00u, 0x04u, contra_quadrant_aim_dir_01);
}

static void contra_rom_red_blue_soldier_gen_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x80u);
}

static void contra_rom_spawn_red_blue_soldier(ContraCore *core, uint8_t data_byte)
{
    uint8_t *const ram = core->ram;
    const int slot = contra_rom_find_next_enemy_slot(core);

    if (slot < 0)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = ((data_byte >> 2u) & 0x01u) != 0u ? 0x1Eu : 0x1Fu;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = (uint8_t)(data_byte & 0x03u);
}

static void contra_rom_red_blue_soldier_gen_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] >= 0x03u)
    {
        contra_rom_remove_enemy(core, x); /* jmp remove_enemy: keep the husk */
        return;
    }
    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    for (;;)
    {
        uint8_t offset = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
        uint8_t data = contra_red_blue_soldier_data_tbl[offset];

        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(offset + 1u);
        if (data == 0xFFu)
        {
            ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x00u;
            continue;
        }
        if ((data & 0x80u) != 0u)
        {
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = (uint8_t)(data << 1u);
            return;
        }
        contra_rom_spawn_red_blue_soldier(core, data);
    }
}

static const uint8_t contra_eye_projectile_sprite_attr_tbl[4] = {0x00u, 0x40u, 0xC0u, 0x80u};

/* eye_projectile_routine_00 (bank0:2811-2824): aim ring projectile at player, advance routine. */
static void contra_rom_eye_projectile_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t sx = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t sy = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    uint8_t quadrant;
    const uint8_t nibble = contra_rom_aim_at_player(core, sx, sy, contra_quadrant_aim_dir_01, &quadrant);

    contra_rom_set_bullet_velocities(core, x, nibble, quadrant, 0x06u);
    contra_rom_advance_enemy_routine(core, x);
}

/* eye_projectile_routine_01 (bank0:2826-2845): grow/enable collision, mirror sprite attr, move. */
static void contra_rom_eye_projectile_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t attr;

    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x48u)
    {
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x63u; /* far/small, not yet hittable */
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Eu); /* enable collision */
        ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x64u; /* near/big, hittable */
    }
    attr = (uint8_t)(ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0x3Fu);
    attr = (uint8_t)(attr | contra_eye_projectile_sprite_attr_tbl[(ram[CONTRA_RAM_FRAME_COUNTER] >> 2u) & 0x03u]);
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = attr;
    contra_rom_update_enemy_pos(core, x);
}
