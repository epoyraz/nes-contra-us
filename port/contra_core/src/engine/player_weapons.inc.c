/* Player weapon firing, bullet initialization, bullet updates, and bullet sprites.
   Included by core.c; not compiled as a separate translation unit. */


static void contra_clear_player_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_ATTR + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_OWNER + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_AIM_DIR + bullet_index] = 0x00u;
    /* clear_bullet_values (bank6:1724) preserves the X/Y positions and velocity
       sub-pixel accumulators; re-used slots keep the previous bullet's phase. */
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_F_RAPID + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_DIST + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_FS_X + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_F_Y + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_VEL_FS_X_ACCUM + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_VEL_F_Y_ACCUM + bullet_index] = 0x00u;
}

static uint8_t contra_normalize_bullet_direction(uint8_t aim_dir, uint8_t jump_status)
{
    if ((jump_status != 0u) &&
        ((aim_dir == 0x04u) || (aim_dir == 0x05u) || (aim_dir == 0x0Au)))
    {
        return 0x0Bu;
    }

    if (aim_dir <= 0x09u)
    {
        return aim_dir;
    }

    return 0x02u;
}

static bool contra_player_can_fire(const ContraCore *core, uint8_t player_index)
{
    const uint8_t aim_dir = core->ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index];

    if ((core->ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] != 0u) ||
        (core->ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] != 0u))
    {
        return false;
    }

    if ((core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] != 0u) &&
        (aim_dir >= 0x03u) && (aim_dir < 0x07u))
    {
        return false;
    }

    return true;
}

static void contra_advance_subpixel_position_u8(uint8_t *pos, uint8_t *accum, uint8_t fast, uint8_t fract)
{
    const uint16_t sum = (uint16_t)(*accum) + (uint16_t)fract;

    *accum = (uint8_t)sum;
    *pos = (uint8_t)((uint16_t)(*pos) + (uint16_t)fast + (uint16_t)(sum >> 8u));
}

static uint8_t contra_get_player_weapon_type(const uint8_t *ram, uint8_t player_index)
{
    const uint8_t weapon_type = (uint8_t)(ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index] & 0x0Fu);

    return (weapon_type < 5u) ? weapon_type : 0u;
}

static bool contra_player_weapon_has_rapid_fire(const uint8_t *ram, uint8_t player_index)
{
    return (ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index] & 0x10u) != 0u;
}

static char contra_ascii_upper(char ch)
{
    if ((ch >= 'a') && (ch <= 'z'))
    {
        return (char)(ch - ('a' - 'A'));
    }
    return ch;
}

static bool contra_parse_start_weapon(const char *value, uint8_t *weapon)
{
    char first;
    char second;

    if ((value == NULL) || (value[0] == '\0'))
    {
        return false;
    }

    if ((value[0] >= '0') && (value[0] <= '4') && (value[1] == '\0'))
    {
        *weapon = (uint8_t)(value[0] - '0');
        return true;
    }

    first = contra_ascii_upper(value[0]);
    second = contra_ascii_upper(value[1]);
    if ((first == 'S') && ((second == 'T') || (second == 'D')))
    {
        *weapon = 0x00u; /* standard rifle */
        return true;
    }

    switch (first)
    {
        case 'N': *weapon = 0x00u; return true; /* normal */
        case 'M': *weapon = 0x01u; return true;
        case 'F': *weapon = 0x02u; return true;
        case 'S': *weapon = 0x03u; return true; /* spread */
        case 'L': *weapon = 0x04u; return true;
        default: return false;
    }
}

static bool contra_env_flag_enabled(const char *name)
{
    const char *const value = getenv(name);
    char first;

    if ((value == NULL) || (value[0] == '\0'))
    {
        return false;
    }

    first = contra_ascii_upper(value[0]);
    return !(((first == '0') && (value[1] == '\0')) ||
             (first == 'N') ||
             (first == 'F'));
}

static int contra_find_player_bullet_slot(
    const ContraCore *core,
    uint8_t player_index,
    unsigned bullet_limit,
    size_t player_2_start
)
{
    const size_t start = (player_index == 0u) ? 0u : player_2_start;
    size_t slot_offset;

    for (slot_offset = 0u; (slot_offset < bullet_limit) && ((start + slot_offset) < CONTRA_PLAYER_BULLET_COUNT); ++slot_offset)
    {
        const size_t bullet_index = start + slot_offset;

        if (core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_index] == 0u)
        {
            return (int)bullet_index;
        }
    }

    return -1;
}

/* set_indoor_bullet_pos_and_slot (bank6:789): the base/indoor (pseudo-3D) level
   does NOT use the outdoor aim-offset table for the bullet spawn. Standing, the
   bullet spawns y-24 (forward "into the screen") with x +1 when facing right
   (aim 0-3) or -1 otherwise; aiming down (aim 4/5) spawns at y-12 and marks the
   bullet slot bit 7 (crouch). Jumping spawns at the player center, clamped so the
   bullet y never exceeds 0x98. NOT used on the indoor boss screen (location type
   bit 7 set), which keeps the outdoor aim path. */
static void contra_set_indoor_bullet_position(
    ContraCore *core, size_t bullet_slot, uint8_t player_index, uint8_t aim_dir)
{
    uint8_t *const ram = core->ram;
    const uint8_t jump_status = ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index];
    uint8_t x_off = 0x00u;
    uint8_t y_off = 0x00u;

    if (jump_status == 0u)
    {
        y_off = 0xF4u; /* -12, assume aiming down (crouch) */
        x_off = 0xFFu; /* -1 */
        if ((aim_dir != 0x04u) && (aim_dir != 0x05u))
        {
            y_off = 0xE8u; /* -24, forward into the screen */
            if (aim_dir < 0x05u)
            {
                x_off = 0x01u; /* aim 0-3: facing right */
            }
        }
    }

    ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_slot] =
        (uint8_t)(y_off + ram[CONTRA_RAM_SPRITE_Y_POS + player_index]);
    ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_slot] =
        (uint8_t)(x_off + ram[CONTRA_RAM_SPRITE_X_POS + player_index]);

    if (y_off == 0xF4u)
    {
        /* crouching while shooting: mark the bullet slot (bank6:821) */
        ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_slot] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_slot] | 0x80u);
    }

    if ((jump_status != 0u) &&
        (ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_slot] >= 0x98u))
    {
        ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_slot] = 0x98u;
    }
}

static void contra_init_player_bullet_position(
    ContraCore *core,
    size_t bullet_slot,
    uint8_t player_index,
    uint8_t aim_dir
)
{
    uint8_t *const ram = core->ram;
    const int8_t (*position_table)[2];

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u)
    {
        /* indoor base level: distinct spawn geometry (bank6:586 chooses this over
           the outdoor table). The boss screen (location type 0x80) falls through. */
        contra_set_indoor_bullet_position(core, bullet_slot, player_index, aim_dir);
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        /* indoor BOSS screen: init_bullet_sprite_pos (bank6:675) picks the
           INDOOR offset tables for any non-zero location type, while the
           velocity below stays outdoor-style. */
        position_table = (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
            ? contra_bullet_initial_pos_indoor_jump
            : contra_bullet_initial_pos_indoor_ground;
    }
    else
    {
        position_table = (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
            ? contra_bullet_initial_pos_jump
            : contra_bullet_initial_pos_ground;
    }

    ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_slot] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] + position_table[aim_dir][0]);
    ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_slot] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + position_table[aim_dir][1]);
}

/* set_vel_for_speed_vars (bank7:4919): rotate `speed` right `shift` bits into a
 * {fast,fract} pair (lsr fast / ror fract), then optionally negate. */
static void contra_speed_code_to_velocity(
    uint8_t speed, uint8_t shift, bool negate, uint8_t *out_fast, uint8_t *out_fract)
{
    uint8_t fast = speed;
    uint8_t fract = 0u;
    uint8_t i;

    for (i = 0u; i < shift; ++i)
    {
        const uint8_t carry = (uint8_t)(fast & 0x01u);

        fast = (uint8_t)(fast >> 1);
        fract = (uint8_t)((fract >> 1) | (uint8_t)(carry << 7));
    }
    if (negate)
    {
        const uint16_t nf = (uint16_t)(0u - (uint16_t)fract);

        fract = (uint8_t)nf;
        fast = (uint8_t)(0u - (uint16_t)fast - ((nf >> 8u) & 1u));
    }
    *out_fast = fast;
    *out_fract = fract;
}

/* set_indoor_bullet_vel (bank6:985): in the base/indoor level every player
 * bullet ignores aim direction. Y is fixed "up" (into the screen); X drifts
 * toward the screen-center vanishing point 0x80 (the pseudo-3D convergence),
 * with magnitude |player_x - 0x80|. shift = 6 normally, 5 with rapid fire. */
static void contra_set_indoor_bullet_velocity(
    ContraCore *core, size_t bullet_slot, uint8_t player_index, bool rapid_fire)
{
    uint8_t *const ram = core->ram;
    const uint8_t shift = rapid_fire ? 5u : 6u;
    const uint8_t player_x = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
    const bool right_of_center = (player_x >= 0x80u);
    const uint8_t x_speed = right_of_center
        ? (uint8_t)(player_x - 0x80u)
        : (uint8_t)(0x80u - player_x);
    uint8_t fast;
    uint8_t fract;

    /* Y velocity: speed code 0x40, always negated (up). */
    contra_speed_code_to_velocity(0x40u, shift, true, &fast, &fract);
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_slot] = fast;
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_slot] = fract;

    /* X velocity: magnitude |player_x - 0x80|, negated (toward center) when the
       player is right of center. */
    contra_speed_code_to_velocity(x_speed, shift, right_of_center, &fast, &fract);
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_slot] = fast;
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_slot] = fract;
}

/* set_indoor_bullet_delay (bank6:659): indoor player bullets despawn on a frame
   TIMER (they fly "into the screen" and vanish at the horizon), not off-screen.
   0x2a frames normally; 0x15 with rapid fire, except the laser which is 0x2a. */
static void contra_set_indoor_bullet_delay(
    ContraCore *core, size_t bullet_slot, uint8_t weapon_type, bool rapid_fire)
{
    core->ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_slot] =
        ((weapon_type != 0x04u) && rapid_fire) ? 0x15u : 0x2Au;
}

/* LEVEL_LOCATION_TYPE == 0x01: indoor base level (NOT the indoor boss screen,
   whose location type has bit 7 set and uses the outdoor aim path). */
static bool contra_in_indoor_base_level(const uint8_t *ram)
{
    return ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u;
}

static void contra_set_player_bullet_velocity(
    ContraCore *core,
    size_t bullet_slot,
    const int8_t velocity_fast[12][2],
    const uint8_t velocity_fract[12][2],
    uint8_t aim_dir
)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_slot] = (uint8_t)velocity_fast[aim_dir][0];
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_slot] = (uint8_t)velocity_fast[aim_dir][1];
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_slot] = velocity_fract[aim_dir][0];
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_slot] = velocity_fract[aim_dir][1];
}

static void contra_init_player_bullet_common(
    ContraCore *core,
    size_t bullet_slot,
    uint8_t player_index,
    uint8_t weapon_type,
    uint8_t aim_dir,
    uint8_t sprite_code,
    uint8_t routine
)
{
    uint8_t *const ram = core->ram;

    contra_clear_player_bullet(core, bullet_slot);
    ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] = 0x0Fu;
    ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_slot] = (uint8_t)(weapon_type + 1u);
    ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_slot] = routine;
    ram[CONTRA_RAM_PLAYER_BULLET_OWNER + bullet_slot] = player_index;
    ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_slot] = sprite_code;
    ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_ATTR + bullet_slot] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_AIM_DIR + bullet_slot] = aim_dir;
}

static bool contra_create_standard_or_machine_gun_bullet(
    ContraCore *core,
    uint8_t player_index,
    uint8_t weapon_type,
    unsigned bullet_limit
)
{
    uint8_t *const ram = core->ram;
    const int bullet_slot = contra_find_player_bullet_slot(core, player_index, bullet_limit, 0x0Au);
    const bool rapid_fire = contra_player_weapon_has_rapid_fire(ram, player_index);
    const uint8_t aim_dir = contra_normalize_bullet_direction(
        ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index],
        ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index]
    );

    if (bullet_slot < 0)
    {
        return false;
    }

    contra_init_player_bullet_common(
        core,
        (size_t)bullet_slot,
        player_index,
        weapon_type,
        aim_dir,
        contra_weapon_bullet_sprite_code_tbl[weapon_type],
        0x00u
    );
    contra_init_player_bullet_position(core, (size_t)bullet_slot, player_index, aim_dir);
    contra_set_player_bullet_velocity(
        core,
        (size_t)bullet_slot,
        rapid_fire ? contra_bullet_velocity_fast_rapid : contra_bullet_velocity_fast,
        rapid_fire ? contra_bullet_velocity_fract_rapid : contra_bullet_velocity_fract,
        aim_dir
    );
    if (contra_in_indoor_base_level(ram))
    {
        contra_set_indoor_bullet_velocity(core, (size_t)bullet_slot, player_index, rapid_fire);
        contra_set_indoor_bullet_delay(core, (size_t)bullet_slot, weapon_type, rapid_fire);
    }
    contra_play_sound(core, 0x0Au);
    return true;
}

static bool contra_create_flame_bullet(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const int bullet_slot = contra_find_player_bullet_slot(core, player_index, CONTRA_STANDARD_BULLET_LIMIT, 0x0Au);
    const bool rapid_fire = contra_player_weapon_has_rapid_fire(ram, player_index);
    const uint8_t aim_dir = contra_normalize_bullet_direction(
        ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index],
        ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index]
    );
    int center_x;
    int center_y;

    if (bullet_slot < 0)
    {
        return false;
    }

    contra_init_player_bullet_common(
        core,
        (size_t)bullet_slot,
        player_index,
        0x02u,
        aim_dir,
        contra_weapon_bullet_sprite_code_tbl[0x02u],
        0x00u
    );
    contra_init_player_bullet_position(core, (size_t)bullet_slot, player_index, aim_dir);
    contra_set_player_bullet_velocity(
        core,
        (size_t)bullet_slot,
        rapid_fire ? contra_f_bullet_velocity_fast_rapid : contra_f_bullet_velocity_fast,
        rapid_fire ? contra_f_bullet_velocity_fract_rapid : contra_f_bullet_velocity_fract,
        aim_dir
    );
    /* f_bullet_outdoor_init_center (bank6:1028): the X center is a plain 8-bit
       add (it WRAPS -- a left shot near the screen edge gets FS_X=0xFF, never
       killed); only the Y center is screened, with the 6502 carry: a positive
       offset kills on overflow past the bottom, a negative offset kills when
       the add does NOT carry (shot off the top). */
    center_x = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_X_POS + (size_t)bullet_slot] +
                         (uint8_t)contra_f_bullet_center_offset_tbl[aim_dir][0]);
    {
        const int8_t off_y = contra_f_bullet_center_offset_tbl[aim_dir][1];
        const uint16_t sum_y = (uint16_t)ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + (size_t)bullet_slot] +
                               (uint8_t)off_y;
        const bool carry = sum_y > 0xFFu;

        if ((off_y >= 0) ? carry : !carry)
        {
            contra_clear_player_bullet(core, (size_t)bullet_slot);
            return false;
        }
        center_y = (uint8_t)sum_y;
    }

    ram[CONTRA_RAM_PLAYER_BULLET_TIMER + (size_t)bullet_slot] = contra_f_bullet_initial_timer_tbl[aim_dir];
    ram[CONTRA_RAM_PLAYER_BULLET_FS_X + (size_t)bullet_slot] = (uint8_t)center_x;
    ram[CONTRA_RAM_PLAYER_BULLET_F_Y + (size_t)bullet_slot] = (uint8_t)center_y;
    contra_play_sound(core, 0x0Au);
    return true;
}

static void contra_set_spray_bullet_velocity(
    ContraCore *core,
    size_t bullet_slot,
    uint8_t aim_dir,
    uint8_t bullet_num,
    bool rapid_fire
)
{
    uint8_t *const ram = core->ram;
    const uint8_t base_index = contra_s_bullet_player_aim_dir_ptr_tbl[aim_dir];
    const uint8_t velocity_index = (uint8_t)(
        (base_index + (uint8_t)contra_s_bullet_num_index_modifier_tbl[bullet_num]) & 0x1Fu
    );
    const uint8_t (*const x_velocity)[2] =
        rapid_fire ? contra_s_bullet_x_velocity_rapid : contra_s_bullet_x_velocity_normal;
    const uint8_t (*const y_velocity)[2] =
        rapid_fire ? contra_s_bullet_y_velocity_rapid : contra_s_bullet_y_velocity_normal;

    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_slot] = x_velocity[velocity_index][0];
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_slot] = x_velocity[velocity_index][1];
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_slot] = y_velocity[velocity_index][0];
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_slot] = y_velocity[velocity_index][1];
}

static void contra_create_spray_bullets(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const bool rapid_fire = contra_player_weapon_has_rapid_fire(ram, player_index);
    const uint8_t aim_dir = contra_normalize_bullet_direction(
        ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index],
        ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index]
    );
    uint8_t bullet_num;

    for (bullet_num = 0u; bullet_num < 5u; ++bullet_num)
    {
        const int bullet_slot = contra_find_player_bullet_slot(core, player_index, CONTRA_SPRAY_GUN_BULLET_LIMIT, 0x06u);

        if (bullet_slot < 0)
        {
            return;
        }

        contra_init_player_bullet_common(
            core,
            (size_t)bullet_slot,
                player_index,
                0x03u,
                aim_dir,
                contra_weapon_bullet_sprite_code_tbl[0x03u],
                0x00u
            );
        ram[CONTRA_RAM_PLAYER_BULLET_S_BULLET_NUM + (size_t)bullet_slot] = bullet_num;
        contra_init_player_bullet_position(core, (size_t)bullet_slot, player_index, aim_dir);
        if (contra_in_indoor_base_level(ram))
        {
            /* init_s_bullet_pos_and_vel indoor branch (bank6:520-526): indoor S
               bullets use the indoor pos/vel, seed FS_X from the spawn X for the
               pseudo-3D spread, record the rapid flag, and -- critically -- get a
               despawn delay. Without the delay the bullet TIMER stays 0, wraps to
               0xFF on its first update, and the bullet lingers ~255 frames; two
               presses then jam every spray slot and the gun stops firing. */
            contra_set_indoor_bullet_velocity(core, (size_t)bullet_slot, player_index, rapid_fire);
            ram[CONTRA_RAM_PLAYER_BULLET_FS_X + (size_t)bullet_slot] =
                ram[CONTRA_RAM_PLAYER_BULLET_X_POS + (size_t)bullet_slot];
            ram[CONTRA_RAM_PLAYER_BULLET_S_RAPID + (size_t)bullet_slot] = rapid_fire ? 0x01u : 0x00u;
            contra_set_indoor_bullet_delay(core, (size_t)bullet_slot, 0x03u, rapid_fire);
        }
        else
        {
            contra_set_spray_bullet_velocity(core, (size_t)bullet_slot, aim_dir, bullet_num, rapid_fire);
        }
    }

    contra_play_sound(core, 0x0Au);
}

static unsigned contra_collect_laser_bullet_slots(
    const ContraCore *core,
    uint8_t player_index,
    bool allow_reuse,
    size_t bullet_slots[CONTRA_LASER_BULLET_COUNT]
)
{
    unsigned count = 0u;
    size_t bullet_index;

    for (bullet_index = 0u; bullet_index < CONTRA_PLAYER_BULLET_COUNT; ++bullet_index)
    {
        if (core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_index] == 0u)
        {
            bullet_slots[count++] = bullet_index;
            if (count == CONTRA_LASER_BULLET_COUNT)
            {
                return count;
            }
        }
    }

    if (!allow_reuse)
    {
        return count;
    }

    for (bullet_index = 0u; bullet_index < CONTRA_PLAYER_BULLET_COUNT; ++bullet_index)
    {
        if ((core->ram[CONTRA_RAM_PLAYER_BULLET_OWNER + bullet_index] == player_index) &&
            ((core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_index] & 0x0Fu) == 0x05u))
        {
            bullet_slots[count++] = bullet_index;
            if (count == CONTRA_LASER_BULLET_COUNT)
            {
                return count;
            }
        }
    }

    return count;
}

static bool contra_player_has_active_laser_bullets(const ContraCore *core, uint8_t player_index)
{
    size_t bullet_index;

    for (bullet_index = 0u; bullet_index < CONTRA_PLAYER_BULLET_COUNT; ++bullet_index)
    {
        if ((core->ram[CONTRA_RAM_PLAYER_BULLET_OWNER + bullet_index] == player_index) &&
            ((core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_index] & 0x0Fu) == 0x05u))
        {
            return true;
        }
    }

    return false;
}

static void contra_create_laser_bullets(ContraCore *core, uint8_t player_index, bool pressed_this_frame)
{
    uint8_t *const ram = core->ram;
    size_t bullet_slots[CONTRA_LASER_BULLET_COUNT];
    const bool can_reuse = pressed_this_frame;
    const uint8_t aim_dir = contra_normalize_bullet_direction(
        ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index],
        ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index]
    );
    const unsigned bullet_count = contra_collect_laser_bullet_slots(core, player_index, can_reuse, bullet_slots);
    unsigned bullet_num;

    if (!pressed_this_frame && contra_player_has_active_laser_bullets(core, player_index))
    {
        return;
    }

    if (bullet_count < CONTRA_LASER_BULLET_COUNT)
    {
        return;
    }

    for (bullet_num = 0u; bullet_num < CONTRA_LASER_BULLET_COUNT; ++bullet_num)
    {
        const size_t bullet_slot = bullet_slots[bullet_num];

        contra_init_player_bullet_common(core, bullet_slot, player_index, 0x04u, aim_dir, 0x00u, 0x00u);
        contra_init_player_bullet_position(core, bullet_slot, player_index, aim_dir);
        contra_set_player_bullet_velocity(
            core,
            bullet_slot,
            contra_bullet_velocity_fast_rapid,
            contra_bullet_velocity_fract_rapid,
            aim_dir
        );
        if (contra_in_indoor_base_level(ram))
        {
            contra_set_indoor_bullet_velocity(core, bullet_slot, player_index, true);
        }
        ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_slot] = contra_laser_bullet_delay_tbl[bullet_num];
    }

    contra_play_sound(core, 0x0Au);
}

static void contra_update_machine_gun_fire_time(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t fire_time = ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME + player_index];

    fire_time = (uint8_t)(fire_time & 0x0Fu);
    if (fire_time < 0x07u)
    {
        fire_time = (uint8_t)(fire_time + 1u);
    }

    ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME + player_index] = fire_time;
}

static void contra_fire_machine_gun(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t fire_time = (uint8_t)(ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME + player_index] + 1u);
    const uint8_t threshold = (fire_time < 0x60u) ? 0x08u : 0x0Fu;

    ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME + player_index] = fire_time;
    if ((fire_time & 0x0Fu) < threshold)
    {
        return;
    }

    fire_time = (uint8_t)(fire_time + 0x10u);
    if (fire_time >= 0x70u)
    {
        fire_time = 0x00u;
    }
    else
    {
        fire_time = (uint8_t)(fire_time & 0xF0u);
    }

    if (!contra_create_standard_or_machine_gun_bullet(
            core,
            player_index,
            0x01u,
            CONTRA_MACHINE_GUN_BULLET_LIMIT))
    {
        ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME + player_index] = 0x07u;
        return;
    }

    ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME + player_index] = fire_time;
}

static void contra_check_player_fire(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t weapon_type = contra_get_player_weapon_type(ram, player_index);
    const bool fire_pressed = (ram[CONTRA_RAM_CONTROLLER_STATE + player_index] & CONTRA_BUTTON_B) != 0u;
    const bool fire_pressed_this_frame = (ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_index] & CONTRA_BUTTON_B) != 0u;

    if (!contra_player_can_fire(core, player_index))
    {
        return;
    }

    if (weapon_type == 0x01u)
    {
        if (fire_pressed)
        {
            contra_fire_machine_gun(core, player_index);
            return;
        }
    }
    else if (weapon_type == 0x04u)
    {
        if (fire_pressed)
        {
            contra_create_laser_bullets(core, player_index, fire_pressed_this_frame);
            return;
        }
    }
    else if (fire_pressed_this_frame)
    {
        switch (weapon_type)
        {
            case 0x02u:
                (void)contra_create_flame_bullet(core, player_index);
                break;

            case 0x03u:
                contra_create_spray_bullets(core, player_index);
                break;

            default:
                (void)contra_create_standard_or_machine_gun_bullet(
                    core,
                    player_index,
                    0x00u,
                    CONTRA_STANDARD_BULLET_LIMIT
                );
                break;
        }
        return;
    }

    /* bank6 @continue (bank6.asm:322): on every frame WITHOUT a fire attempt --
       for EVERY weapon, not just the M gun -- PLAYER_M_WEAPON_FIRE_TIME's low
       nibble climbs to its 0x07 cap. A freshly picked-up M gun therefore fires
       on the very first B press; without this the port swallowed the first
       half-second of machine-gun fire after a pickup. */
    contra_update_machine_gun_fire_time(core, player_index);
}

static void contra_add_scroll_to_player_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_FRAME_SCROLL] != 0u)
    {
        if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u)
        {
            ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] - ram[CONTRA_RAM_FRAME_SCROLL]);
        }
        else
        {
            ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] + ram[CONTRA_RAM_FRAME_SCROLL]);
        }
    }
}

/* player_shared_indoor_bullet_routine_01 + player_bullet_collision_routine
   (bank6:1493/1739): on the indoor base level standard/M bullets fly by velocity
   while PLAYER_BULLET_TIMER counts down (routine 1); at 0 they advance to routine 2
   with TIMER 0x06, turn into a hollow-ring sprite (0x47), linger 6 frames, then
   despawn -- or clear immediately once the screen is cleared. No off-screen test. */
static void contra_update_indoor_shared_player_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] >= 0x02u)
    {
        if (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] != 0u)
        {
            contra_clear_player_bullet(core, bullet_index);
            return;
        }
        ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_index] = 0x47u; /* hollow ring */
        contra_add_scroll_to_player_bullet(core, bullet_index);
        ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] - 1u);
        if (ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] == 0u)
        {
            contra_clear_player_bullet(core, bullet_index);
        }
        return;
    }

    contra_advance_subpixel_position_u8(
        &ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index],
        &ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_ACCUM + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_index]
    );
    contra_advance_subpixel_position_u8(
        &ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index],
        &ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_ACCUM + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_index]
    );

    ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] - 1u);
    if (ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] == 0u)
    {
        ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] = 0x02u;
        ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] = 0x06u;
    }
}

static void contra_update_shared_player_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;

    if (contra_in_indoor_base_level(ram))
    {
        contra_update_indoor_shared_player_bullet(core, bullet_index);
        return;
    }

    /* player_bullet_collision_routine (bank6:1739, $bc1e), shared by every
       bullet type: a bullet that hit something freezes at the impact point as
       the hollow-ring sprite (0x47), tracks scroll only, and despawns when the
       6-frame timer set by set_bullet_routine_to_2 runs out. Without this the
       dying bullet kept flying and could register extra hits. */
    if (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] >= 0x02u)
    {
        if (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] != 0u)
        {
            contra_clear_player_bullet(core, bullet_index);
            return;
        }
        ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_index] = 0x47u;
        contra_add_scroll_to_player_bullet(core, bullet_index);
        ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] - 1u);
        if (ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] == 0u)
        {
            contra_clear_player_bullet(core, bullet_index);
        }
        return;
    }

    contra_advance_subpixel_position_u8(
        &ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index],
        &ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_ACCUM + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_index]
    );
    contra_advance_subpixel_position_u8(
        &ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index],
        &ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_ACCUM + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_index]
    );

    contra_add_scroll_to_player_bullet(core, bullet_index);

    if ((ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] < 0x05u) ||
        (ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] >= 0xFBu) ||
        (ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] < 0x05u) ||
        (ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] >= 0xE8u))
    {
        contra_clear_player_bullet(core, bullet_index);
    }
}

static void contra_update_flame_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t aim_dir = ram[CONTRA_RAM_PLAYER_BULLET_AIM_DIR + bullet_index];
    const uint8_t swirl_index = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] & 0x0Fu);

    if (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] >= 0x02u)
    {
        /* consumed: the shared player_bullet_collision_routine freezes the
           impact ring -- without this the dead fireball kept swirling */
        contra_update_shared_player_bullet(core, bullet_index);
        return;
    }

    if (ram[CONTRA_RAM_FRAME_SCROLL] != 0u)
    {
        if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u)
        {
            ram[CONTRA_RAM_PLAYER_BULLET_FS_X + bullet_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_FS_X + bullet_index] - ram[CONTRA_RAM_FRAME_SCROLL]);
        }
        else
        {
            ram[CONTRA_RAM_PLAYER_BULLET_F_Y + bullet_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_F_Y + bullet_index] + ram[CONTRA_RAM_FRAME_SCROLL]);
        }
    }

    contra_advance_subpixel_position_u8(
        &ram[CONTRA_RAM_PLAYER_BULLET_FS_X + bullet_index],
        &ram[CONTRA_RAM_PLAYER_BULLET_VEL_FS_X_ACCUM + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_index]
    );
    contra_advance_subpixel_position_u8(
        &ram[CONTRA_RAM_PLAYER_BULLET_F_Y + bullet_index],
        &ram[CONTRA_RAM_PLAYER_BULLET_VEL_F_Y_ACCUM + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_index]
    );

    ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] = (uint8_t)(
        ram[CONTRA_RAM_PLAYER_BULLET_FS_X + bullet_index] + contra_f_bullet_outdoor_x_swirl_amt_tbl[swirl_index]
    );
    ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] = (uint8_t)(
        ram[CONTRA_RAM_PLAYER_BULLET_F_Y + bullet_index] + contra_f_bullet_outdoor_y_swirl_amt_tbl[swirl_index]
    );

    if ((aim_dir == 0x0Au) || (aim_dir < 0x05u))
    {
        ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] + 1u);
    }
    else
    {
        ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] - 1u);
    }

    if ((ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] < 0x05u) ||
        (ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] >= 0xFBu) ||
        (ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] < 0x05u) ||
        (ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] >= 0xE8u))
    {
        contra_clear_player_bullet(core, bullet_index);
    }
}

/* s_bullet_pos_mod_tbl (bank6:1861): per-frame sideways drift {accum-add,
   indoor-adj-add} for each S bullet number; entries 0-4 normal, 5-9 rapid fire.
   Bullet 0 stays centred while 1/3 drift right and 2/4 drift left, fanning the
   spread out across the pseudo-3D corridor. */
static const uint8_t contra_s_bullet_pos_mod_tbl[10][2] = {
    {0x00u, 0x00u}, {0x20u, 0x00u}, {0xE0u, 0xFFu}, {0x40u, 0x00u}, {0xC0u, 0xFFu},
    {0x00u, 0x00u}, {0x40u, 0x00u}, {0xC0u, 0xFFu}, {0x80u, 0x00u}, {0x80u, 0xFFu},
};

/* player_s_indoor_bullet_routine_01 + update_s_bullet_indoor_pos (bank6:1501,
   1829): indoor spread bullets fly "into the screen" (Y by velocity) while each
   bullet number accumulates its own sideways offset (S_INDOOR_ADJ) from the
   pos-mod table, producing the spread fan in the corridor view. The sprite grows
   (0x1F->0x20->0x21) as the despawn TIMER counts down toward the vanishing point.
   The ROM also writes a throwaway VEL_FS_X_ACCUM + S_ADJ_ACCUM sum into
   X_VEL_ACCUM; later bullets reusing the slot observe that stale phase. */
static void contra_update_s_indoor_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t timer = ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index];
    uint8_t sprite = 0x1Fu;
    uint8_t mod_index;
    unsigned accum_sum;

    if (timer < 0x1Au)
    {
        sprite = (timer < 0x0Au) ? 0x21u : 0x20u;
    }
    ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_index] = sprite;

    /* FS_X += {X_VEL_FAST, X_VEL_FRACT}: the convergence toward centre 0x80 */
    contra_advance_subpixel_position_u8(
        &ram[CONTRA_RAM_PLAYER_BULLET_FS_X + bullet_index],
        &ram[CONTRA_RAM_PLAYER_BULLET_VEL_FS_X_ACCUM + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_index]);
    /* Y_POS += {Y_VEL_FAST, Y_VEL_FRACT}: up, into the screen */
    contra_advance_subpixel_position_u8(
        &ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index],
        &ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_ACCUM + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_index],
        ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_index]);

    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_ACCUM + bullet_index] = (uint8_t)(
        ram[CONTRA_RAM_PLAYER_BULLET_VEL_FS_X_ACCUM + bullet_index] +
        ram[CONTRA_RAM_PLAYER_BULLET_S_ADJ_ACCUM + bullet_index]);

    /* X_POS = FS_X + accumulated sideways spread */
    ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] = (uint8_t)(
        ram[CONTRA_RAM_PLAYER_BULLET_FS_X + bullet_index] +
        ram[CONTRA_RAM_PLAYER_BULLET_S_INDOOR_ADJ + bullet_index]);

    /* advance {S_INDOOR_ADJ:S_ADJ_ACCUM} by this bullet's pos-mod entry */
    mod_index = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_S_BULLET_NUM + bullet_index] +
                          (ram[CONTRA_RAM_PLAYER_BULLET_S_RAPID + bullet_index] ? 5u : 0u));
    if (mod_index > 9u)
    {
        mod_index = 9u;
    }
    accum_sum = (unsigned)ram[CONTRA_RAM_PLAYER_BULLET_S_ADJ_ACCUM + bullet_index] +
                contra_s_bullet_pos_mod_tbl[mod_index][0];
    ram[CONTRA_RAM_PLAYER_BULLET_S_ADJ_ACCUM + bullet_index] = (uint8_t)accum_sum;
    ram[CONTRA_RAM_PLAYER_BULLET_S_INDOOR_ADJ + bullet_index] = (uint8_t)(
        ram[CONTRA_RAM_PLAYER_BULLET_S_INDOOR_ADJ + bullet_index] +
        contra_s_bullet_pos_mod_tbl[mod_index][1] + (accum_sum >> 8u));

    /* dec_bullet_delay_possibly_adv_routine: at 0, become the impact ring */
    ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] = (uint8_t)(timer - 1u);
    if (ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] == 0u)
    {
        ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] = 0x02u;
        ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] = 0x06u;
    }
}

static void contra_update_spray_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;
    uint8_t sprite_code = 0x21u;

    if (contra_in_indoor_base_level(ram))
    {
        if (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] < 0x02u)
        {
            contra_update_s_indoor_bullet(core, bullet_index);
            return;
        }
        contra_update_shared_player_bullet(core, bullet_index);
        return;
    }

    ram[CONTRA_RAM_PLAYER_BULLET_DIST + bullet_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_DIST + bullet_index] + 1u);
    if (ram[CONTRA_RAM_PLAYER_BULLET_DIST + bullet_index] < 0x10u)
    {
        sprite_code = 0x1Fu;
    }
    else if (ram[CONTRA_RAM_PLAYER_BULLET_DIST + bullet_index] < 0x20u)
    {
        sprite_code = 0x20u;
    }

    ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_index] = sprite_code;
    contra_update_shared_player_bullet(core, bullet_index);
}

static void contra_update_laser_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t aim_dir = ram[CONTRA_RAM_PLAYER_BULLET_AIM_DIR + bullet_index];

    if (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] == 0x00u)
    {
        if (ram[CONTRA_RAM_FRAME_SCROLL] != 0u)
        {
            if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u)
            {
                ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] =
                    (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] - ram[CONTRA_RAM_FRAME_SCROLL]);
            }
            else
            {
                ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] =
                    (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] + ram[CONTRA_RAM_FRAME_SCROLL]);
            }
        }

        if (ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] != 0u)
        {
            ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] - 1u);
        }

        if (ram[CONTRA_RAM_PLAYER_BULLET_TIMER + bullet_index] == 0u)
        {
            ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] = 0x01u;
            ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_index] = contra_laser_bullet_sprite_tbl[aim_dir][0];
            ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_ATTR + bullet_index] = contra_laser_bullet_sprite_tbl[aim_dir][1];
        }
        return;
    }

    contra_update_shared_player_bullet(core, bullet_index);
}

static void contra_update_player_bullets(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    size_t bullet_index;

    for (bullet_index = 0u; bullet_index < CONTRA_PLAYER_BULLET_COUNT; ++bullet_index)
    {
        const uint8_t bullet_slot = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_index] & 0x0Fu);

        if (bullet_slot == 0u)
        {
            continue;
        }

        if ((bullet_slot != 0x05u) && (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] == 0u))
        {
            ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + bullet_index] = 0x01u;
            continue;
        }

        switch (bullet_slot)
        {
            case 0x03u:
                contra_update_flame_bullet(core, bullet_index);
                break;

            case 0x04u:
                contra_update_spray_bullet(core, bullet_index);
                break;

            case 0x05u:
                contra_update_laser_bullet(core, bullet_index);
                break;

            default:
                contra_update_shared_player_bullet(core, bullet_index);
                break;
        }
    }
}

static void contra_draw_player_bullet_sprites(ContraCore *core)
{
    const uint8_t frame_parity = (uint8_t)(core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u);
    int bullet_index;

    for (bullet_index = 7; bullet_index >= 0; --bullet_index)
    {
        const size_t source_index = (size_t)(((uint8_t)bullet_index << 1u) | frame_parity);
        const size_t sprite_index = (size_t)bullet_index + 2u;

        core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + sprite_index] =
            core->ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + source_index];
        core->ram[CONTRA_RAM_SPRITE_ATTR + sprite_index] =
            core->ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_ATTR + source_index];
        core->ram[CONTRA_RAM_SPRITE_Y_POS + sprite_index] =
            core->ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + source_index];
        core->ram[CONTRA_RAM_SPRITE_X_POS + sprite_index] =
            core->ram[CONTRA_RAM_PLAYER_BULLET_X_POS + source_index];
    }
}
