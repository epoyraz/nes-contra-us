/* Level 3 waterfall dragon boss mouth and arm/orb routines.
   Included by core.c; not compiled as a separate translation unit. */

static void contra_rom_enable_bullet_enemy_collision(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] &= 0x7Fu;
}
static void contra_rom_disable_bullet_enemy_collision(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] |= 0x80u;
}

/* boss_mouth_nametable_update_tbl (bank0:4592): {top,bottom} background super-tile
   per animation frame — closed / partially open / fully open. Bit 7 (the #$80 in
   #$a0..#$a5) means "don't repaint the palette"; the data index is the low 7 bits. */
static const uint8_t contra_boss_mouth_nametable_update_tbl[6] = {
    0xA0u, 0xA1u, 0xA2u, 0xA3u, 0xA4u, 0xA5u};
/* mouth_projectile_type_angle (bank0:4643): three orange fireballs, fanned. */
static const uint8_t contra_mouth_projectile_type_angle[3] = {0x88u, 0x86u, 0x84u};
/* boss_mouth_anim_delay_tbl (bank0:4674): closed-mouth delay by arms destroyed
   (2 arms -> #$c0, 1 -> #$70, 0 -> #$20: the boss attacks faster as arms die). */
static const uint8_t contra_boss_mouth_anim_delay_tbl[3] = {0xC0u, 0x70u, 0x20u};

/* boss_mouth_routine_00 (bank0:4512): init the stored HP, the defeat-anim flag, the
   animation frame, and the pre-reveal delay. */
static void contra_rom_boss_mouth_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x20u; /* mouth HP, held while closed */
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x02u; /* defeat-animation delay flag */
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x01u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0xFFu);
}

/* boss_mouth_routine_01 (bank0:4528): wait until the boss-reveal auto-scroll has
   finished, then start the mouth-opening animation. */
static void contra_rom_boss_mouth_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] == 0u)
    {
        return;
    }
    if (ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    contra_rom_advance_enemy_routine(core, x);
}

/* boss_mouth_draw_supertiles_set_delay (bank0:4559): time the open/close animation.
   The mouth's two super-tiles are drawn from ENEMY_FRAME by the per-frame overlay
   redraw, so here only the timer is advanced. Returns true on the frames the ROM's
   carry-clear "drew a new frame" path is taken. */
static bool contra_rom_boss_mouth_anim_step(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return false;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x06u; /* delay between animation frames */
    return true;
}

/* boss_mouth_routine_02 (bank0:4539): animate the mouth opening; once fully open,
   become hittable, set the attack delay, and advance to the firing routine. */
static void contra_rom_boss_mouth_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (!contra_rom_boss_mouth_anim_step(core, x))
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= 0x02u)
    {
        contra_rom_enable_bullet_enemy_collision(core, x);
        ram[CONTRA_RAM_ENEMY_HP + x] = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x06u; /* open-to-fire delay */
        contra_rom_set_enemy_delay_adv_routine(core, x, 0x70u); /* time mouth stays open */
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    }
}

/* boss_mouth_routine_03 (bank0:4597): while open, fire 3 fireballs, then close
   (becoming invincible) and advance to the closing animation. */
static void contra_rom_boss_mouth_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] == 0u)
        {
            const uint8_t px = ram[CONTRA_RAM_ENEMY_X_POS + x];
            const uint8_t py = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x08u);
            const uint8_t speed =
                (ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED] >= 0x02u) ? 0x07u : 0x06u;
            int i;

            for (i = 2; i >= 0; --i)
            {
                contra_rom_create_enemy_bullet_angle_a(
                    core, contra_mouth_projectile_type_angle[i], speed, px, py);
            }
        }
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    contra_rom_disable_bullet_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_ENEMY_HP + x]; /* hold HP while closed */
    ram[CONTRA_RAM_ENEMY_HP + x] = 0xF1u; /* hittable but takes no damage while closed */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x06u);
}

/* boss_mouth_routine_04 (bank0:4646): animate the mouth closing, then loop back to
   the opening routine after a delay that shortens as the dragon arms are destroyed. */
static void contra_rom_boss_mouth_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t idx;

    if (!contra_rom_boss_mouth_anim_step(core, x))
    {
        return;
    }
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
        return;
    }
    idx = ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED];
    if (idx > 2u)
    {
        idx = 2u;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = contra_boss_mouth_anim_delay_tbl[idx];
    contra_rom_set_enemy_routine_to_a(core, x, 0x03u); /* -> boss_mouth_routine_02 */
}

/* boss_mouth_routine_08 (bank0:4682): the dragon-defeated set piece -- every
   other frame (ENEMY_VAR_3 toggling) walk 14 fixed positions, draw the
   destroyed background super-tile (budget-gated, retried on failure) and spawn
   a two-round 0x89 explosion there; after all 14, set the level-end delay to
   0x60 and remove. (The super-tile pixels themselves are cosmetic for the
   native renderer; the budget byte cost and the slot's X/Y walk are the
   structural effects.) */
static void contra_rom_boss_mouth_routine_08(ContraCore *core, uint8_t x)
{
    static const uint8_t y_tbl[14] = {
        0x20u, 0x20u, 0x20u, 0x20u, 0x40u, 0x40u, 0x60u, 0x60u,
        0x80u, 0x80u, 0xA0u, 0xA0u, 0xC0u, 0xC0u};
    static const uint8_t x_tbl[14] = {
        0x50u, 0xB0u, 0x70u, 0x90u, 0x70u, 0x90u, 0x70u, 0x90u,
        0x70u, 0x90u, 0x70u, 0x90u, 0x70u, 0x90u};
    uint8_t *const ram = core->ram;
    uint8_t i;

    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x01u;

    i = ram[CONTRA_RAM_ENEMY_VAR_2 + x];
    if (i >= 14u)
    {
        i = 13u; /* unreachable in ROM data; guard the table read */
    }
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = y_tbl[i];
    ram[CONTRA_RAM_ENEMY_X_POS + x] = x_tbl[i];
    if (!contra_rom_enemy_supertile_draw_budget(core))
    {
        return; /* draw failed: retry next toggle */
    }
    contra_rom_create_explosion_at(core, x_tbl[i], y_tbl[i]);

    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] >= 0x0Eu)
    {
        ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x60u; /* set_delay_remove_enemy */
        ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x00u;
        contra_rom_remove_enemy(core, x);
    }
}

/* dragon_arm_orb_set_sprite (bank0:4943): the tip (its child link is the #$ff
   terminator) is the red hand orb (sprite_7b); every other orb is gray (sprite_7a). */
static void contra_rom_dragon_arm_orb_set_sprite(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        ((core->ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) != 0u) ? 0x7Bu : 0x7Au;
}

/* dragon_arm_orb_routine_00 (bank0:4746): initialise a shoulder orb -- choose the
   side (bit 0 of the attribute), seed the position index, nudge the X anchor, mark
   it as the shoulder (VAR_4 = #$ff), and queue 4 child orbs to spawn. */
static void contra_rom_dragon_arm_orb_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const bool left = (ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u;
    const uint8_t pos = left ? 0x28u : 0x38u;
    const uint8_t x_adj = left ? 0xF8u : 0x08u;

    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = pos;
    ram[CONTRA_RAM_ENEMY_VAR_A + x] = pos;
    contra_rom_add_a_to_enemy_x_pos(core, x, x_adj);
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0xFFu; /* shoulder marker */
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x04u; /* child orbs to spawn */
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = x;     /* chain tail starts at self */
    contra_rom_advance_enemy_routine(core, x);
}

/* dragon_arm_orb_routine_01 (bank0:4768): once the reveal scroll has finished, the
   shoulder spawns one child orb per frame, threading a doubly-linked chain
   (VAR_3 = next/outer, VAR_4 = prev/inner). The 4th (tip) child becomes the red
   "hand" orb, then every orb's routine is advanced to the extend animation. */
static void contra_rom_dragon_arm_orb_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    int child;
    uint8_t prev;
    uint8_t slot;
    uint8_t cur;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] == 0u)
    {
        return;
    }
    if (ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] != 0u)
    {
        return;
    }
    if ((ram[CONTRA_RAM_ENEMY_VAR_4 + x] & 0x80u) == 0u)
    {
        return; /* only the shoulder spawns the chain */
    }

    child = contra_rom_find_next_enemy_slot(core);
    if (child < 0)
    {
        return;
    }
    slot = (uint8_t)child;
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x15u;
    contra_rom_initialize_enemy(core, slot);

    /* @init_child_dragon_arm_orb (bank0:4825) */
    ram[CONTRA_RAM_ENEMY_ROUTINE + slot] = 0x02u; /* dragon_arm_orb_routine_01 */
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot] = 0x8Cu;
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] = 0x52u;
    ram[CONTRA_RAM_ENEMY_HP + slot] = 0xF1u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + slot] = 0x00u;
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    ram[CONTRA_RAM_ENEMY_Y_POS + slot] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    ram[CONTRA_RAM_ENEMY_X_POS + slot] = ram[CONTRA_RAM_ENEMY_X_POS + x];
    prev = ram[CONTRA_RAM_ENEMY_VAR_2 + x]; /* current chain tail */
    ram[CONTRA_RAM_ENEMY_VAR_4 + slot] = prev;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = slot;
    ram[CONTRA_RAM_ENEMY_VAR_3 + prev] = slot;

    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] != 0u)
    {
        return; /* more children to spawn on later frames */
    }

    /* the tip child becomes the red hand orb (bank0:4796-4807) */
    ram[CONTRA_RAM_ENEMY_VAR_3 + slot] = 0xFFu;
    ram[CONTRA_RAM_ENEMY_HP + slot] = 0x10u;
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot] = 0x0Cu;
    ram[CONTRA_RAM_ENEMY_VAR_2 + slot] = 0x01u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + slot] = 0x20u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = slot; /* shoulder remembers the hand */

    /* advance every orb (shoulder..hand) to dragon_arm_orb_routine_02 (bank0:4809) */
    cur = x;
    for (;;)
    {
        const uint8_t next = ram[CONTRA_RAM_ENEMY_VAR_3 + cur];

        contra_rom_advance_enemy_routine(core, cur);
        if ((next & 0x80u) != 0u)
        {
            break; /* just advanced the hand orb */
        }
        cur = next;
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
}

/* dragon_arm_orb_pos_tbl (bank0:5411): 80-byte signed offset table. Consecutive
   entries trace a circle, so accumulating a per-orb position index and indexing this
   for the Y offset (and index+16 for the X offset, a quarter-turn phase shift) bends
   the chain into the arm's curve. Rows 2/3 negate rows 0/1; the 5th row repeats
   row 0 so index+16 stays in range for indices up to 0x3f. */
static const uint8_t contra_dragon_arm_orb_pos_tbl[80] = {
    0x00u,0x01u,0x03u,0x04u,0x06u,0x07u,0x08u,0x0Au,0x0Bu,0x0Cu,0x0Du,0x0Eu,0x0Eu,0x0Fu,0x0Fu,0x0Fu,
    0x0Fu,0x0Fu,0x0Fu,0x0Fu,0x0Eu,0x0Eu,0x0Du,0x0Cu,0x0Bu,0x0Au,0x08u,0x07u,0x06u,0x04u,0x03u,0x01u,
    0x00u,0xFFu,0xFDu,0xFCu,0xFAu,0xF9u,0xF8u,0xF6u,0xF5u,0xF4u,0xF3u,0xF2u,0xF2u,0xF1u,0xF1u,0xF1u,
    0xF1u,0xF1u,0xF1u,0xF1u,0xF2u,0xF2u,0xF3u,0xF4u,0xF5u,0xF6u,0xF8u,0xF9u,0xFAu,0xFCu,0xFDu,0xFFu,
    0x00u,0x01u,0x03u,0x04u,0x06u,0x07u,0x08u,0x0Au,0x0Bu,0x0Cu,0x0Du,0x0Eu,0x0Eu,0x0Fu,0x0Fu,0x0Fu};

/* dragon_arm_open_anim_tbl (bank0:4938): {y vel accum, y pos, x vel accum, x pos}
   per side (right, left) for the per-orb arm-extend "grow out" animation. */
static const uint8_t contra_dragon_arm_open_anim_tbl[2][4] = {
    {0x4Bu, 0xFFu, 0xB5u, 0x00u}, {0x4Bu, 0xFFu, 0x4Bu, 0xFFu}};

/* dragon_arm_orb_set_positions (bank0:5365): walk the chain from the shoulder out,
   positioning each orb relative to the previous one by an accumulated position index
   into dragon_arm_orb_pos_tbl. This is what gives the arm its curved shape. */
static void contra_rom_dragon_arm_orb_set_positions(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t prev = x;

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    for (;;)
    {
        const uint8_t cur = ram[CONTRA_RAM_ENEMY_VAR_3 + prev];
        uint8_t idx;

        if ((cur & 0x80u) != 0u)
        {
            break; /* reached past the hand */
        }
        idx = (uint8_t)((ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + prev] +
                         ram[CONTRA_RAM_ENEMY_VAR_1 + cur]) & 0x3Fu);
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + cur] = idx;
        ram[CONTRA_RAM_ENEMY_Y_POS + cur] =
            (uint8_t)(contra_dragon_arm_orb_pos_tbl[idx] + ram[CONTRA_RAM_ENEMY_Y_POS + prev]);
        ram[CONTRA_RAM_ENEMY_X_POS + cur] =
            (uint8_t)(contra_dragon_arm_orb_pos_tbl[idx + 16u] + ram[CONTRA_RAM_ENEMY_X_POS + prev]);
        prev = cur;
    }
}

/* @set_pos_add_accum (bank0:4911): nudge one orb along the extend direction for its
   side, accumulating sub-pixel velocity into its position. */
static void contra_rom_dragon_arm_orb_extend_step(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t side = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u);
    const uint8_t *t = contra_dragon_arm_open_anim_tbl[side];
    uint16_t sum;

    sum = (uint16_t)ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] + t[0];
    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = (uint8_t)sum;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] =
        (uint8_t)((uint16_t)ram[CONTRA_RAM_ENEMY_Y_POS + x] + t[1] + (sum >> 8u));
    sum = (uint16_t)ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] + t[2];
    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] = (uint8_t)sum;
    ram[CONTRA_RAM_ENEMY_X_POS + x] =
        (uint8_t)((uint16_t)ram[CONTRA_RAM_ENEMY_X_POS + x] + t[3] + (sum >> 8u));
}

/* dragon_arm_orb_routine_02 (bank0:4853): the arm extends out one orb at a time,
   from the hand inward. When an orb finishes its #$10-step extend it hands off to its
   parent; once the innermost orb finishes, every orb advances to the attack routine. */
static void contra_rom_dragon_arm_orb_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t parent;
    uint8_t cur;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) == 0u)
        {
            return; /* the ROM ticks this delay on odd frames only */
        }
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return;
    }

    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        return; /* this orb isn't extending yet */
    }
    contra_rom_dragon_arm_orb_set_sprite(core, x);
    contra_rom_dragon_arm_orb_extend_step(core, x);
    if ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x80u) != 0u)
    {
        return; /* already finished extending */
    }
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] < 0x10u)
    {
        return; /* still extending */
    }

    /* this orb is fully extended -- hand off to the parent (bank0:4876) */
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0xFFu;
    parent = ram[CONTRA_RAM_ENEMY_VAR_4 + x];
    ram[CONTRA_RAM_ENEMY_VAR_2 + parent] = 0x01u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + parent] = 0x00u;
    if ((ram[CONTRA_RAM_ENEMY_VAR_4 + parent] & 0x80u) == 0u)
    {
        return; /* parent is a normal orb; it extends next */
    }

    /* the parent is the shoulder: the whole arm is extended, advance every orb to
       the attack routine (bank0:4888). */
    cur = parent;
    for (;;)
    {
        const uint8_t next = ram[CONTRA_RAM_ENEMY_VAR_3 + cur];

        contra_rom_advance_enemy_routine(core, cur);
        ram[CONTRA_RAM_ENEMY_VAR_2 + cur] = 0x00u;
        if ((next & 0x80u) != 0u)
        {
            break;
        }
        cur = next;
    }
    ram[CONTRA_RAM_ENEMY_FRAME + parent] = 0x00u;
}

/* wave/spin trigger + timer tables (bank0:5019-5088). */
static const uint8_t contra_wave_direction_up_change_tbl[2] = {0x14u, 0x0Cu};
static const uint8_t contra_wave_direction_down_change_tbl[2] = {0x2Cu, 0x34u};
static const uint8_t contra_dragon_arm_orb_pattern_timer_tbl[3] = {0x40u, 0xC0u, 0x40u};
static const uint8_t contra_dragon_arm_frame_02_tbl[2] = {0x08u, 0x38u};
static const uint8_t contra_dragon_arm_delay_tbl[4] = {0x40u, 0x60u, 0x30u, 0x70u};

/* dragon_arm_orb_fire_projectile (bank0:5124): on the shoulder's VAR_A cadence,
   aim a bullet from the hand orb at the nearest player. */
static void contra_rom_dragon_arm_orb_fire_projectile(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t hand;

    ram[CONTRA_RAM_ENEMY_VAR_A + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_A + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_A + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_A + x] = 0x90u;
    hand = ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x]; /* shoulder stored the hand slot */
    /* the ROM byte #$80 is TYPE<<5: bullet type 4, the dragon fireball --
       its larger collision box (code 2) and sprite_79 hang off VAR_1 */
    contra_rom_aim_and_create_enemy_bullet(
        core, ram[CONTRA_RAM_ENEMY_X_POS + hand], ram[CONTRA_RAM_ENEMY_Y_POS + hand],
        0x04u, 0x05u, contra_quadrant_aim_dir_01);
}

/* @timer_logic (bank0:5284): apply one orb's rotation-timer adjustment to its
   position index, carrying an accumulator between orbs. Returns the updated
   accumulator ($08 += $0d). */
static uint8_t contra_rom_dragon_arm_timer_logic(ContraCore *core, uint8_t x, uint8_t adj, uint8_t accum)
{
    uint8_t *const ram = core->ram;
    const uint8_t d0c = adj;
    uint8_t d0b = (uint8_t)(adj + accum);
    int d0d = 0;

    if (d0b == 0u)
    {
        return accum;
    }
    if ((d0b & 0x80u) != 0u)
    {
        do /* @inc_timer_loop */
        {
            bool dec_var1 = true;

            if (((ram[CONTRA_RAM_ENEMY_VAR_4 + x] & 0x80u) == 0u) &&
                (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x38u))
            {
                if ((d0c & 0x80u) == 0u) /* adj 0 or positive */
                {
                    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
                    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
                }
                else
                {
                    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
                }
                dec_var1 = false;
            }
            if (dec_var1)
            {
                ++d0d;
                ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
                    (uint8_t)((ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u) & 0x3Fu);
            }
            ++d0b;
        } while (d0b != 0u);
    }
    else
    {
        do /* @enemy_var_2_loop */
        {
            bool inc_var1 = true;

            if (((ram[CONTRA_RAM_ENEMY_VAR_4 + x] & 0x80u) == 0u) &&
                (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x08u))
            {
                if ((d0c == 0u) || ((d0c & 0x80u) != 0u)) /* adj 0 or negative */
                {
                    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
                    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
                }
                else
                {
                    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
                }
                inc_var1 = false;
            }
            if (inc_var1)
            {
                --d0d;
                ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
                    (uint8_t)((ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u) & 0x3Fu);
            }
            --d0b;
        } while (d0b != 0u);
    }
    return (uint8_t)(accum + (uint8_t)d0d);
}

/* @check_delay_run_timer (bank0:5265): advance one orb's rotation timer and apply it. */
static uint8_t contra_rom_dragon_arm_check_delay_run_timer(ContraCore *core, uint8_t x, uint8_t accum)
{
    uint8_t *const ram = core->ram;
    uint8_t adj;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        adj = 0x00u;
    }
    else if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u)
    {
        adj = 0x00u;
    }
    else if ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x80u) != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] + 1u);
        adj = 0xFFu;
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] - 1u);
        adj = 0x01u;
    }
    return contra_rom_dragon_arm_timer_logic(core, x, adj, accum);
}

/* dragon_arm_animate (bank0:5242): roll every orb's rotation timer forward, bending
   the arm by nudging the per-orb position indices, and merge the timers so the
   shoulder knows when a spin pattern has finished. */
static void contra_rom_dragon_arm_animate(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t accum = 0u;
    uint8_t merged = 0u;
    uint8_t cur = x;

    for (;;)
    {
        const uint8_t next = ram[CONTRA_RAM_ENEMY_VAR_3 + cur];

        accum = contra_rom_dragon_arm_check_delay_run_timer(core, cur, accum);
        merged = (uint8_t)(merged | ram[CONTRA_RAM_ENEMY_VAR_2 + cur]);
        if ((next & 0x80u) != 0u)
        {
            break;
        }
        cur = next;
    }
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = merged;
}

/* quadrant_aim_dir_02 (bank7:10578): the within-quadrant aim nibble table used only
   by the dragon arm orbs when seeking the player (selector $0f == 2). */
static const uint8_t contra_quadrant_aim_dir_02[32] = {
    0x80u,0x00u,0x00u,0x00u,
    0xF8u,0x53u,0x32u,0x21u,
    0xFBu,0x86u,0x54u,0x33u,
    0xFDu,0xA8u,0x75u,0x54u,
    0xFEu,0xB9u,0x87u,0x65u,
    0xFEu,0xCBu,0x98u,0x76u,
    0xFEu,0xDBu,0xA9u,0x87u,
    0xFFu,0xDCu,0xBAu,0x98u};

/* dragon_arm_orb_seek_should_move (bank7:10341): for orb `x`, compute the aim
   direction toward player `player_idx` and compare it to the next orb's accumulated
   position index. Returns 0x80 = orb already aimed (don't move), 0x00 = move by
   incrementing its position index, 0x01 = move by decrementing. */
static uint8_t contra_rom_dragon_arm_orb_seek_should_move(
    ContraCore *core, uint8_t x, uint8_t player_idx)
{
    uint8_t *const ram = core->ram;
    const uint8_t sx = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t sy = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    const uint8_t next = ram[CONTRA_RAM_ENEMY_VAR_3 + x]; /* next orb, farther from body */
    uint8_t quadrant;
    uint8_t aim = contra_rom_get_quadrant_aim_dir_for_player(
        core, sx, sy, player_idx, contra_quadrant_aim_dir_02, &quadrant);
    uint8_t next_pos;
    uint8_t adj;       /* $0d */
    uint8_t last_row;  /* $0e */

    if ((quadrant & 0x02u) != 0u)
    {
        aim = (uint8_t)(0x20u - aim); /* player to the left */
    }
    if ((quadrant & 0x01u) != 0u)
    {
        aim = (uint8_t)((0x40u - aim) & 0x3Fu); /* player above (does not happen for the arm) */
    }

    next_pos = ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + next];
    last_row = 0u;
    adj = (uint8_t)(next_pos + 0x20u);
    if (adj >= 0x40u)
    {
        last_row = 1u;
        adj = (uint8_t)(adj - 0x40u);
    }

    if (aim == next_pos)
    {
        return 0x80u; /* @set_negative_exit */
    }
    if (last_row == 0u)
    {
        if (aim < next_pos)
        {
            return 0x01u; /* @clear_zero_exit */
        }
        return (aim >= adj) ? 0x01u : 0x00u;
    }
    if (aim >= next_pos)
    {
        return 0x00u; /* @loop */
    }
    return (aim < adj) ? 0x00u : 0x01u;
}

/* @inc_position (bank0:5170): bump the moving orb's position index up. If it already
   sits at 0x08 walk inward past the run of 0x08 orbs, and at the boundary nudge the
   child orb down so the arm bends smoothly. */
static void contra_rom_dragon_arm_inc_position(ContraCore *core, uint8_t x, uint8_t slot11)
{
    uint8_t *const ram = core->ram;

    for (;;)
    {
        const uint8_t parent = ram[CONTRA_RAM_ENEMY_VAR_4 + x];
        if ((parent & 0x80u) != 0u)
        {
            break; /* parent is the shoulder */
        }
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0x08u)
        {
            break;
        }
        x = parent;
    }
    if (x == slot11)
    {
        const uint8_t child = ram[CONTRA_RAM_ENEMY_VAR_3 + x];
        if ((child & 0x80u) == 0u)
        {
            ram[CONTRA_RAM_ENEMY_VAR_1 + child] =
                (uint8_t)((ram[CONTRA_RAM_ENEMY_VAR_1 + child] - 1u) & 0x3Fu);
        }
        x = slot11;
    }
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u) & 0x3Fu);
}

/* @dec_position (bank0:5200): mirror of @inc_position -- find the orb at index 0x38
   and decrement it, nudging the boundary child orb up. */
static void contra_rom_dragon_arm_dec_position(ContraCore *core, uint8_t x, uint8_t slot11)
{
    uint8_t *const ram = core->ram;

    for (;;)
    {
        const uint8_t parent = ram[CONTRA_RAM_ENEMY_VAR_4 + x];
        if ((parent & 0x80u) != 0u)
        {
            break;
        }
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0x38u)
        {
            break;
        }
        x = parent;
    }
    if (x == slot11)
    {
        const uint8_t child = ram[CONTRA_RAM_ENEMY_VAR_3 + x];
        if ((child & 0x80u) == 0u)
        {
            ram[CONTRA_RAM_ENEMY_VAR_1 + child] =
                (uint8_t)((ram[CONTRA_RAM_ENEMY_VAR_1 + child] + 1u) & 0x3Fu);
        }
        x = slot11;
    }
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u) & 0x3Fu);
}

/* dragon_arm_seek_player_logic (bank0:5145): the FRAME #$04 attack -- walk the chain
   from the hand inward, find the first orb that should rotate to point the arm at the
   closest player, and nudge that orb's position index toward the aim. */
static void contra_rom_dragon_arm_seek_player_logic(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t hand = ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x];
    const uint8_t player_idx = contra_rom_player_enemy_x_dist_idx(core, hand);
    uint8_t cur = hand;

    for (;;)
    {
        const uint8_t slot11 = cur; /* $11: the orb whose index gets adjusted */
        const uint8_t parent = ram[CONTRA_RAM_ENEMY_VAR_4 + cur];
        uint8_t mv;

        if ((parent & 0x80u) != 0u)
        {
            break; /* @exit: reached the shoulder */
        }
        mv = contra_rom_dragon_arm_orb_seek_should_move(core, parent, player_idx);
        if ((mv & 0x80u) != 0u)
        {
            cur = parent; /* @enemy_orb_loop: this orb is fine, try the next inward */
            continue;
        }
        if (mv != 0u)
        {
            contra_rom_dragon_arm_dec_position(core, slot11, slot11);
        }
        else
        {
            contra_rom_dragon_arm_inc_position(core, slot11, slot11);
        }
        break;
    }
}

static void contra_rom_dragon_arm_orb_pat_1_2_3_or_4(ContraCore *core, uint8_t x, uint8_t frame);

/* dragon_arm_orb_attack_pat (bank0:4974): pattern #$00 -- wave the arm up and down,
   firing on cadence, flipping direction at the trigger indices, and advancing to the
   spin pattern after a few sweeps. */
static void contra_rom_dragon_arm_orb_attack_pat(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    uint8_t side;

    if (frame != 0u)
    {
        contra_rom_dragon_arm_orb_pat_1_2_3_or_4(core, x, frame);
        return;
    }

    contra_rom_dragon_arm_orb_fire_projectile(core, x);
    side = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u);
    if (ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] != 0u)
    {
        /* waving up */
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != contra_wave_direction_down_change_tbl[side])
        {
            ram[CONTRA_RAM_ENEMY_VAR_2 + x] = contra_dragon_arm_orb_pattern_timer_tbl[side + 1u];
            return;
        }
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] + 1u);
        if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] == 0x03u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u); /* -> spin toward center */
        }
    }
    else
    {
        /* waving down */
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != contra_wave_direction_up_change_tbl[side])
        {
            ram[CONTRA_RAM_ENEMY_VAR_2 + x] = contra_dragon_arm_orb_pattern_timer_tbl[side];
            return;
        }
    }
    /* @set_delay_swap_dir */
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x03u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] ^= 0x01u;
}

/* dragon_arm_orb_pat_1_2_3_or_4 (bank0:5032): patterns #$01 spin toward center,
   #$02 spin away, #$03 hook, #$04 seek player. */
static void contra_rom_dragon_arm_orb_pat_1_2_3_or_4(ContraCore *core, uint8_t x, uint8_t frame)
{
    uint8_t *const ram = core->ram;

    if (frame == 0x01u)
    {
        contra_rom_dragon_arm_orb_fire_projectile(core, x);
        if (ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] != 0u)
        {
            return; /* spins still winding down */
        }
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        return;
    }
    if (frame == 0x02u)
    {
        const uint8_t side = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u);
        uint8_t cur = ram[CONTRA_RAM_ENEMY_VAR_3 + x];

        contra_rom_dragon_arm_orb_fire_projectile(core, x);
        for (;;)
        {
            if (ram[CONTRA_RAM_ENEMY_VAR_1 + cur] != contra_dragon_arm_frame_02_tbl[side])
            {
                ram[CONTRA_RAM_ENEMY_VAR_2 + cur] = contra_dragon_arm_orb_pattern_timer_tbl[side];
                return;
            }
            if ((ram[CONTRA_RAM_ENEMY_VAR_3 + cur] & 0x80u) != 0u)
            {
                /* whole arm reached the spin-out index: random delay, advance to
                   hook. The ROM's `adc FRAME_COUNTER` (bank0:5077) carries in 1:
                   this path is only reached through the equality cmp against
                   dragon_arm_frame_02_tbl, which leaves the carry SET. */
                const uint8_t idx = (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] +
                                               ram[CONTRA_RAM_FRAME_COUNTER] + 1u) & 0x03u);
                ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = contra_dragon_arm_delay_tbl[idx];
                ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
                return;
            }
            cur = ram[CONTRA_RAM_ENEMY_VAR_3 + cur];
        }
    }
    if (frame == 0x03u)
    {
        contra_rom_dragon_arm_orb_fire_projectile(core, x);
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
        {
            return;
        }
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0xC0u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        return;
    }
    /* frame == 0x04: seek player */
    contra_rom_dragon_arm_seek_player_logic(core, x);
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u; /* back to wave */
}

/* dragon_arm_orb_routine_03 (bank0:4954): run the shoulder's attack pattern, roll the
   rotation animation (except while seeking), then recompute every orb's position. */
static void contra_rom_dragon_arm_orb_routine_03(ContraCore *core, uint8_t x)
{
    contra_rom_dragon_arm_orb_set_sprite(core, x);
    if ((core->ram[CONTRA_RAM_ENEMY_VAR_4 + x] & 0x80u) == 0u)
    {
        return; /* only the shoulder runs the pattern logic */
    }
    contra_rom_dragon_arm_orb_attack_pat(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_FRAME + x] != 0x04u)
    {
        contra_rom_dragon_arm_animate(core, x);
    }
    contra_rom_dragon_arm_orb_set_positions(core, x);
}

/* dragon_arm_orb_routine_04 (bank0:5419): when the red hand is destroyed, count
   the arm and route every parent up the chain through set_destroyed_enemy_
   routine (their nibble is 5 = this routine, so the cascade re-runs in place one
   frame later and each orb advances into the explosion trio); the orb itself
   just advances. */
static void contra_rom_dragon_arm_orb_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) != 0u)
    {
        uint8_t cur = x;

        ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED] =
            (uint8_t)(ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED] + 1u);
        for (;;)
        {
            const uint8_t parent = ram[CONTRA_RAM_ENEMY_VAR_4 + cur];

            if ((parent & 0x80u) != 0u)
            {
                break; /* reached the shoulder */
            }
            cur = parent;
            contra_rom_set_destroyed_enemy_routine(core, cur);
        }
    }
    contra_rom_advance_enemy_routine(core, x);
}
