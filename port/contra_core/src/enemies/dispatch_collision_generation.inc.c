/* Enemy dispatch, collision checks, indoor loading, and outdoor soldier generation.
   Included by core.c; not compiled as a separate translation unit. */

static void contra_rom_exe_enemy_type(ContraCore *core, uint8_t x)
{
    const uint8_t type = core->ram[CONTRA_RAM_ENEMY_TYPE + x];
    const uint8_t routine = core->ram[CONTRA_RAM_ENEMY_ROUTINE + x];

    switch (type)
    {
        case 0x10u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
            {
                switch (routine) /* level-8 alien guardian (bank0:8969) */
                {
                    case 0x01u: contra_rom_alien_guardian_routine_00(core, x); break;
                    case 0x02u: contra_rom_alien_guardian_routine_01(core, x); break;
                    case 0x03u: contra_rom_alien_guardian_routine_02(core, x); break;
                    case 0x04u: contra_rom_alien_guardian_routine_03(core, x); break;
                    case 0x05u: contra_rom_alien_guardian_routine_04(core, x); break;
                    case 0x06u: contra_rom_alien_guardian_routine_05(core, x); break;
                    case 0x07u: contra_rom_alien_guardian_routine_06(core, x); break;
                    case 0x08u: contra_rom_alien_guardian_routine_07(core, x); break;
                    case 0x09u: contra_rom_alien_guardian_routine_08(core, x); break;
                    case 0x0Au: contra_rom_alien_guardian_routine_09(core, x); break;
                    case 0x0Bu: contra_rom_alien_guardian_routine_0a(core, x); break;
                    case 0x0Cu: contra_rom_alien_guardian_routine_0b(core, x); break;
                    /* table tail (bank0:8982): remove_enemy ($e809, no scroll) */
                    case 0x0Du: contra_rom_remove_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 mechanical claw */
                {
                    case 0x01u: contra_rom_claw_routine_00(core, x); break;
                    case 0x02u: contra_rom_claw_routine_01(core, x); break;
                    case 0x03u: contra_rom_claw_routine_02(core, x); break;
                    case 0x04u: contra_rom_claw_routine_03(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u)
            {
                switch (routine) /* level-6 fire beam down */
                {
                    case 0x01u: contra_rom_fire_beam_down_routine_00(core, x); break;
                    case 0x02u: contra_rom_fire_beam_down_routine_01(core, x); break;
                    case 0x03u: contra_rom_fire_beam_down_routine_02(core, x); break;
                    case 0x04u: contra_rom_fire_beam_down_routine_03(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u)
            {
                switch (routine) /* level-5 ice grenade generator */
                {
                    case 0x01u: contra_rom_ice_grenade_generator_routine_00(core, x); break;
                    case 0x02u: contra_rom_ice_grenade_generator_routine_01(core, x); break;
                    case 0x03u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u)
            {
                switch (routine) /* level-2 boss eye */
                {
                    case 0x01u: contra_rom_boss_eye_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_eye_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_eye_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_eye_routine_03(core, x); break;
                    /* table tail (bank0:2670-2672): boss_defeated_routine,
                       explosion, boss_eye_routine_06 */
                    case 0x05u: contra_rom_boss_eye_defeated_routine(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x07u: contra_rom_boss_eye_routine_06(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
            {
                switch (routine) /* level-3 floating rock platform */
                {
                    case 0x01u: contra_rom_floating_rock_routine_00(core, x); break;
                    case 0x02u: contra_rom_floating_rock_routine_01(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* level-1 boss wall bomb turret */
                {
                    case 0x01u: contra_rom_boss_bomb_turret_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_bomb_turret_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_bomb_turret_routine_02(core, x); break;
                    /* appended shared explosion trio (bank0:2086-2088) */
                    case 0x04u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            break;
        case 0x11u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
            {
                switch (routine) /* level-8 alien fetus */
                {
                    case 0x01u: contra_rom_alien_fetus_routine_00(core, x); break;
                    case 0x02u: contra_rom_alien_fetus_routine_01(core, x); break;
                    case 0x03u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x05u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 rising spiked wall */
                {
                    case 0x01u: contra_rom_rising_spiked_wall_routine_00(core, x); break;
                    case 0x02u: contra_rom_rising_spiked_wall_routine_01(core, x); break;
                    case 0x03u: contra_rom_rising_spiked_wall_routine_02(core, x); break;
                    case 0x04u: contra_rom_add_scroll_to_enemy_pos(core, x); break;
                    case 0x05u: contra_rom_spiked_wall_destroyed_routine(core, x); break;
                    case 0x06u: contra_rom_spiked_wall_destroy_anim(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u)
            {
                switch (routine) /* level-6 fire beam left */
                {
                    case 0x01u: contra_rom_fire_beam_left_routine_00(core, x); break;
                    case 0x02u: contra_rom_fire_beam_left_routine_01(core, x); break;
                    case 0x03u: contra_rom_fire_beam_left_routine_02(core, x); break;
                    case 0x04u: contra_rom_fire_beam_left_routine_03(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u)
            {
                switch (routine) /* level-5 ice grenade */
                {
                    case 0x01u: contra_rom_ice_grenade_routine_00(core, x); break;
                    case 0x02u: contra_rom_ice_grenade_routine_01(core, x); break;
                    case 0x03u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x05u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u)
            {
                switch (routine) /* level-1 fortress boss door */
                {
                    case 0x01u: contra_rom_boss_door_routine_00(core, x); break;
                    case 0x02u: contra_rom_add_scroll_to_enemy_pos(core, x); break;
                    case 0x03u: contra_rom_boss_door_routine_02(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x05u: contra_rom_boss_door_routine_clear_sprite(core, x); break;
                    case 0x06u: contra_rom_boss_door_routine_05(core, x); break;
                    case 0x07u: contra_rom_boss_door_routine_06(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
            {
                switch (routine) /* level-3 moving flame */
                {
                    case 0x01u: contra_rom_floating_rock_routine_00(core, x); break;
                    case 0x02u: contra_rom_moving_flame_routine_01(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* level-2 indoor roller */
                {
                    case 0x01u: contra_rom_roller_routine_00(core, x); break;
                    case 0x02u: contra_rom_roller_routine_01(core, x); break;
                    /* table tail (bank0:2852-2854): init_explosion,
                       roller_routine_04 (explosion_type_03), remove_enemy */
                    case 0x03u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                    case 0x04u: contra_rom_roller_routine_explosion(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            break;
        case 0x1Bu: /* level-2 boss eye sphere projectile */
            switch (routine)
            {
                case 0x01u: contra_rom_eye_projectile_routine_00(core, x); break;
                case 0x02u: contra_rom_eye_projectile_routine_01(core, x); break;
                /* table tail (bank0:2804-2806): the in-place explosion trio */
                case 0x03u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x04u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x05u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x1Cu: /* level-4 boss gemini */
            switch (routine)
            {
                case 0x01u: contra_rom_boss_gemini_routine_00(core, x); break;
                case 0x02u: contra_rom_boss_gemini_routine_01(core, x); break;
                case 0x03u: contra_rom_boss_gemini_routine_02(core, x); break;
                case 0x04u: contra_rom_boss_gemini_routine_03(core, x); break;
                case 0x05u: contra_rom_boss_gemini_routine_04(core, x); break;
                case 0x06u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x07u: contra_rom_boss_gemini_routine_06(core, x); break;
                default: break;
            }
            break;
        case 0x1Du: /* level-4 boss gemini spinning bubbles */
            switch (routine)
            {
                case 0x01u: contra_rom_spinning_bubbles_routine_00(core, x); break;
                case 0x02u: contra_rom_spinning_bubbles_routine_01(core, x); break;
                case 0x03u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x04u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                /* enemy_routine_remove_enemy: keep the husk */
                case 0x05u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x1Eu: /* level-4 boss blue soldier */
            switch (routine)
            {
                case 0x01u: contra_rom_red_blue_soldier_routine_00(core, x); break;
                case 0x02u: contra_rom_blue_soldier_routine_01(core, x); break;
                case 0x03u: contra_rom_blue_soldier_routine_02(core, x); break;
                case 0x04u: contra_rom_blue_soldier_routine_03(core, x); break;
                case 0x05u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x06u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                /* enemy_routine_remove_enemy: the husk keeps its TYPE */
                case 0x07u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x1Fu: /* level-4 boss red soldier */
            switch (routine)
            {
                case 0x01u: contra_rom_red_blue_soldier_routine_00(core, x); break;
                case 0x02u: contra_rom_red_soldier_routine_01(core, x); break;
                case 0x03u: contra_rom_red_soldier_routine_02(core, x); break;
                case 0x04u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x05u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                /* enemy_routine_remove_enemy: the husk keeps its TYPE */
                case 0x06u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x20u: /* level-4 boss red/blue soldier generator */
            switch (routine)
            {
                case 0x01u: contra_rom_red_blue_soldier_gen_routine_00(core, x); break;
                case 0x02u: contra_rom_red_blue_soldier_gen_routine_01(core, x); break;
                /* table tail is remove_enemy (bank0:6085): keep the husk */
                case 0x03u: contra_rom_remove_enemy(core, x); break;
                default: break;
            }
            break;
        case 0x13u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
            {
                switch (routine) /* level-8 white blob */
                {
                    case 0x01u: contra_rom_white_blob_routine_00(core, x); break;
                    case 0x02u: contra_rom_white_blob_routine_01(core, x); break;
                    case 0x03u: contra_rom_white_blob_routine_02(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x06u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 cart generator */
                {
                    case 0x01u: contra_rom_mine_cart_generator_routine_00(core, x); break;
                    case 0x02u: contra_rom_mine_cart_generator_routine_01(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u)
            {
                switch (routine) /* level-6 boss giant soldier */
                {
                    case 0x01u: contra_rom_boss_giant_soldier_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_giant_soldier_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_giant_soldier_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_giant_soldier_routine_03(core, x); break;
                    case 0x05u: contra_rom_boss_giant_soldier_routine_04(core, x); break;
                    case 0x06u: contra_rom_boss_giant_soldier_routine_05(core, x); break;
                    case 0x07u: contra_rom_boss_giant_soldier_routine_06(core, x); break;
                    case 0x08u: contra_rom_boss_giant_soldier_routine_07(core, x); break;
                    case 0x09u: contra_rom_boss_giant_soldier_routine_08(core, x); break;
                    case 0x0Au: contra_rom_boss_giant_soldier_routine_09(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u)
            {
                switch (routine) /* level-5 pipe joint */
                {
                    case 0x01u: contra_rom_ice_separator_routine_00(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
            {
                switch (routine) /* level-3 falling rock */
                {
                    case 0x01u: contra_rom_falling_rock_routine_00(core, x); break;
                    case 0x02u: contra_rom_falling_rock_routine_01(core, x); break;
                    case 0x03u: contra_rom_falling_rock_routine_02(core, x); break;
                    /* table tail (bank0:4397-4399): the in-place explosion trio */
                    case 0x04u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* level-2 wall turret */
                {
                    case 0x01u: contra_rom_wall_turret_routine_00(core, x); break;
                    case 0x02u: contra_rom_wall_turret_routine_01(core, x); break;
                    case 0x03u: contra_rom_wall_turret_routine_02(core, x); break;
                    case 0x04u: contra_rom_wall_turret_routine_03(core, x); break;
                    case 0x05u: contra_rom_wall_turret_routine_04(core, x); break;
                    /* table tail (bank0:3041-3043): the wall-core explosion trio */
                    case 0x06u: contra_rom_wall_core_routine_05(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x08u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            break;
        case 0x14u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
            {
                switch (routine) /* level-8 alien spider */
                {
                    case 0x01u: contra_rom_alien_spider_routine_00(core, x); break;
                    case 0x02u: contra_rom_alien_spider_routine_01(core, x); break;
                    case 0x03u: contra_rom_alien_spider_routine_02(core, x); break;
                    case 0x04u: contra_rom_alien_spider_routine_03(core, x); break;
                    case 0x05u: contra_rom_alien_spider_routine_04(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x08u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 moving cart */
                {
                    case 0x01u:
                    case 0x02u:
                    case 0x03u: contra_rom_moving_cart_routine_00(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x06u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u)
            {
                switch (routine) /* level-6 spiked disk projectile */
                {
                    case 0x01u: contra_rom_boss_giant_projectile_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_giant_projectile_routine_01(core, x); break;
                    case 0x03u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u)
            {
                switch (routine) /* level-5 boss UFO (Guldaf), bank0:6653 */
                {
                    case 0x01u: contra_rom_boss_ufo_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_ufo_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_ufo_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_ufo_routine_03(core, x); break;
                    case 0x05u: contra_rom_boss_ufo_routine_04(core, x); break;
                    case 0x06u: contra_rom_boss_ufo_routine_05(core, x); break;
                    case 0x07u: contra_rom_boss_ufo_routine_06(core, x); break;
                    case 0x08u: contra_rom_boss_ufo_routine_07(core, x); break;
                    case 0x09u: contra_rom_boss_ufo_routine_08(core, x); break;
                    case 0x0Au: contra_rom_boss_ufo_routine_09(core, x); break;
                    case 0x0Bu: contra_rom_boss_ufo_routine_0a(core, x); break;
                    case 0x0Cu: contra_rom_boss_ufo_routine_0b(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
            {
                switch (routine) /* level-3 dragon boss mouth */
                {
                    case 0x01u: contra_rom_boss_mouth_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_mouth_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_mouth_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_mouth_routine_03(core, x); break;
                    case 0x05u: contra_rom_boss_mouth_routine_04(core, x); break;
                    /* table tail (bank0:4646): boss_defeated_routine, explosion,
                       clear_sprite, then the 14-explosion set piece */
                    case 0x06u: contra_rom_boss_door_routine_02(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x08u: contra_rom_boss_door_routine_clear_sprite(core, x); break;
                    case 0x09u: contra_rom_boss_mouth_routine_08(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* level-2 wall core */
                {
                    case 0x01u: contra_rom_wall_core_routine_00(core, x); break;
                    case 0x02u: contra_rom_wall_core_routine_01(core, x); break;
                    case 0x03u: contra_rom_wall_core_routine_02(core, x); break;
                    case 0x04u: contra_rom_wall_core_routine_03(core, x); break;
                    case 0x05u: contra_rom_wall_core_routine_04(core, x); break;
                    case 0x06u: contra_rom_wall_core_routine_05(core, x); break;
                    case 0x07u: contra_rom_wall_core_routine_06(core, x); break;
                    case 0x08u: contra_rom_wall_core_routine_07(core, x); break;
                    case 0x09u: contra_rom_wall_core_routine_08(core, x); break;
                    case 0x0Au: contra_rom_wall_core_routine_09(core, x); break;
                    default: break;
                }
            }
            break;
        case 0x15u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
            {
                switch (routine) /* level-8 alien spider spawn */
                {
                    case 0x01u: contra_rom_alien_spider_spawn_routine_00(core, x); break;
                    case 0x02u: contra_rom_alien_spider_spawn_routine_01(core, x); break;
                    case 0x03u: contra_rom_alien_spider_spawn_routine_02(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x06u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 immobile cart */
                {
                    case 0x01u: contra_rom_immobile_cart_generator_routine_00(core, x); break;
                    case 0x02u: contra_rom_immobile_cart_generator_routine_01(core, x); break;
                    case 0x03u: contra_rom_moving_cart_routine_00(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x06u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u)
            {
                switch (routine) /* level-5 flying saucer (mini UFO), bank0:7045 */
                {
                    case 0x01u: contra_rom_mini_ufo_routine_00(core, x); break;
                    case 0x02u: contra_rom_mini_ufo_routine_01(core, x); break;
                    case 0x03u: contra_rom_mini_ufo_routine_02(core, x); break;
                    case 0x04u: contra_rom_mini_ufo_routine_03(core, x); break;
                    /* table tail: init_explosion, explosion, remove_enemy */
                    case 0x05u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
            {
                switch (routine) /* level-3 dragon arm orb */
                {
                    case 0x01u: contra_rom_dragon_arm_orb_routine_00(core, x); break;
                    case 0x02u: contra_rom_dragon_arm_orb_routine_01(core, x); break;
                    case 0x03u: contra_rom_dragon_arm_orb_routine_02(core, x); break;
                    case 0x04u: contra_rom_dragon_arm_orb_routine_03(core, x); break;
                    case 0x05u: contra_rom_dragon_arm_orb_routine_04(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x08u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* level-2 indoor running soldier */
                {
                    case 0x01u: contra_rom_indoor_soldier_routine_00(core, x); break;
                    case 0x02u: contra_rom_indoor_soldier_routine_01(core, x); break;
                    case 0x03u: contra_rom_shared_indoor_soldier_hit_routine_00(core, x); break;
                    case 0x04u: contra_rom_shared_indoor_soldier_hit_routine_01(core, x); break;
                    /* table tail (bank0:3423-3425): init_explosion,
                       shared_enemy_routine_03 (explosion_type_02), remove_enemy */
                    case 0x05u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            break;
        case 0x16u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
            {
                switch (routine) /* level-8 boss heart */
                {
                    case 0x01u: contra_rom_boss_heart_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_heart_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_heart_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_heart_routine_03(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x06u: contra_rom_boss_heart_routine_05(core, x); break;
                    case 0x07u: contra_rom_boss_heart_routine_06(core, x); break;
                    case 0x08u: contra_rom_alien_guardian_routine_0b(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 armored door */
                {
                    case 0x01u: contra_rom_level7_boss_door_routine_00(core, x); break;
                    case 0x02u: contra_rom_level7_boss_door_routine_01(core, x); break;
                    case 0x03u: contra_rom_level7_boss_door_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_heart_routine_03(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x06u: contra_rom_level7_boss_door_routine_05(core, x); break;
                    case 0x07u: contra_rom_level7_boss_door_routine_06(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u)
            {
                switch (routine) /* level-5 boss UFO drop bomb, bank0:7132 */
                {
                    case 0x01u: contra_rom_boss_ufo_bomb_routine_00(core, x); break;
                    /* table tail: init_explosion, explosion, remove_enemy */
                    case 0x02u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                    case 0x03u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* indoor jumping soldier */
                {
                    case 0x01u: contra_rom_jumping_soldier_routine_00(core, x); break;
                    case 0x02u: contra_rom_jumping_soldier_routine_01(core, x); break;
                    case 0x03u: contra_rom_shared_indoor_soldier_hit_routine_00(core, x); break;
                    case 0x04u: contra_rom_shared_indoor_soldier_hit_routine_01(core, x); break;
                    case 0x05u: contra_rom_jumping_soldier_routine_04(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x08u: contra_rom_enemy_routine_remove_inplace(core, x); break; /* remove_enemy keeps the husk */
                    default: break; /* hit/explosion via the 0xFE actor */
                }
            }
            break;
        case 0x18u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 boss soldier generator */
                {
                    case 0x01u: contra_rom_boss_soldier_generator_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_soldier_generator_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_soldier_generator_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_soldier_generator_routine_03(core, x); break;
                    case 0x05u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* indoor group-of-4 soldier */
                {
                    case 0x01u: contra_rom_four_soldiers_routine_00(core, x); break;
                    case 0x02u: contra_rom_four_soldiers_routine_01(core, x); break;
                    case 0x03u: contra_rom_four_soldiers_routine_02(core, x); break;
                    case 0x04u: contra_rom_shared_indoor_soldier_hit_routine_00(core, x); break;
                    case 0x05u: contra_rom_shared_indoor_soldier_hit_routine_01(core, x); break;
                    /* table tail (bank0:3812-3814) */
                    case 0x06u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x08u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            break;
        case 0x1Au: /* indoor roller generator */
            switch (routine)
            {
                case 0x01u: contra_rom_indoor_roller_gen_routine_00(core, x); break;
                case 0x02u: contra_rom_indoor_roller_gen_routine_01(core, x); break;
                /* table entry 2 (bank0:3906) is remove_enemy itself */
                case 0x03u: contra_rom_remove_enemy_offscreen(core, x); break;
                default: break;
            }
            break;
        case 0x12u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
            {
                switch (routine) /* level-8 alien mouth */
                {
                    case 0x01u: contra_rom_alien_mouth_routine_00(core, x); break;
                    case 0x02u: contra_rom_alien_mouth_routine_01(core, x); break;
                    case 0x03u: contra_rom_alien_mouth_routine_02(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x06u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 spiked wall */
                {
                    case 0x01u: contra_rom_spiked_wall_routine_00(core, x); break;
                    case 0x02u: contra_rom_add_scroll_to_enemy_pos(core, x); break;
                    case 0x03u: contra_rom_spiked_wall_destroyed_routine(core, x); break;
                    case 0x04u: contra_rom_spiked_wall_destroy_anim(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u)
            {
                switch (routine) /* level-6 fire beam right */
                {
                    case 0x01u: contra_rom_fire_beam_right_routine_00(core, x); break;
                    case 0x02u: contra_rom_fire_beam_right_routine_01(core, x); break;
                    case 0x03u: contra_rom_fire_beam_right_routine_02(core, x); break;
                    case 0x04u: contra_rom_fire_beam_right_routine_03(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u)
            {
                switch (routine) /* level-5 tank (bank0:6250) */
                {
                    case 0x01u: contra_rom_tank_routine_00(core, x); break;
                    case 0x02u: contra_rom_tank_routine_01(core, x); break;
                    case 0x03u: contra_rom_tank_routine_02(core, x); break;
                    case 0x04u: contra_rom_tank_routine_03(core, x); break;
                    case 0x05u: contra_rom_tank_routine_04(core, x); break;
                    case 0x06u: contra_rom_tank_routine_05(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u)
            {
                switch (routine) /* level-1 exploding bridge */
                {
                    case 0x01u: contra_rom_exploding_bridge_routine_00(core, x); break;
                    case 0x02u: contra_rom_exploding_bridge_routine_01(core, x); break;
                    case 0x03u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x04u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x05u: contra_rom_exploding_bridge_routine_04(core, x); break;
                    default: break;
                }
            }
            else if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
            {
                switch (routine) /* level-3 rock cave (falling-rock generator) */
                {
                    case 0x01u: contra_rom_rock_cave_routine_00(core, x); break;
                    case 0x02u: contra_rom_rock_cave_routine_01(core, x); break;
                    case 0x03u: contra_rom_rock_cave_routine_02(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* level-2 indoor grenade */
                {
                    case 0x01u: contra_rom_grenade_routine_00(core, x); break;
                    case 0x02u: contra_rom_grenade_routine_01(core, x); break;
                    case 0x03u: contra_rom_grenade_routine_02(core, x); break;
                    /* appended trio: destroy_all (e.g. the boss core dying) raises
                       a mid-air grenade to 4 via its nibble and the explosion runs
                       in place */
                    case 0x04u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                    default: break;
                }
            }
            break;
        case 0x17u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u)
            {
                switch (routine) /* level-7 boss mortar launcher */
                {
                    case 0x01u: contra_rom_boss_mortar_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_mortar_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_mortar_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_mortar_routine_03(core, x); break;
                    case 0x05u: contra_rom_boss_mortar_routine_04(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x07u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x08u: contra_rom_clear_enemy(core, x); break;
                    default: break;
                }
            }
            else
            {
                switch (routine) /* indoor grenade launcher (seeking guy) */
                {
                    case 0x01u: contra_rom_grenade_launcher_routine_00(core, x); break;
                    case 0x02u: contra_rom_grenade_launcher_routine_01(core, x); break;
                    /* table tail (bank0:3477-3481): hit pair, explosion, then
                       grenade_launcher_routine_06 = remove + clear the
                       generation-blocking flag */
                    case 0x03u: contra_rom_shared_indoor_soldier_hit_routine_00(core, x); break;
                    case 0x04u: contra_rom_shared_indoor_soldier_hit_routine_01(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                    case 0x06u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x07u:
                        contra_rom_enemy_routine_remove_inplace(core, x);
                        core->ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0x00u;
                        break;
                    default: break;
                }
            }
            break;
        case 0x08u: /* boss-room wall cannon (triple cannon) */
            switch (routine)
            {
                case 0x01u: contra_rom_wall_cannon_routine_00(core, x); break;
                case 0x02u: contra_rom_wall_cannon_routine_01(core, x); break;
                case 0x03u: contra_rom_wall_cannon_routine_02(core, x); break;
                case 0x04u: contra_rom_wall_cannon_routine_03(core, x); break;
                case 0x05u: contra_rom_wall_cannon_routine_04(core, x); break;
                /* table tail (bank7:9351-9353): the in-place explosion trio */
                case 0x06u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x07u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x08u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x0Au: /* boss-room wall plating */
            switch (routine)
            {
                case 0x01u: contra_rom_wall_plating_routine_00(core, x); break;
                case 0x02u: contra_rom_wall_plating_routine_01(core, x); break;
                case 0x03u: break; /* routine_02: idle, just a target */
                case 0x04u: contra_rom_wall_plating_routine_03(core, x); break;
                /* table tail (bank7:9483-9485): the in-place explosion trio */
                case 0x05u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x06u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x07u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x19u: /* indoor soldier generator */
            switch (routine)
            {
                case 0x01u: contra_rom_indoor_soldier_gen_routine_00(core, x); break;
                case 0x02u: contra_rom_indoor_soldier_gen_routine_01(core, x); break;
                /* table entry 2 (bank0:2409) is remove_enemy itself */
                case 0x03u: contra_rom_remove_enemy_offscreen(core, x); break;
                default: break;
            }
            break;
        case 0x07u: /* red turret */
            switch (routine)
            {
                case 0x01u: contra_rom_red_turret_routine_00(core, x); break;
                case 0x02u: contra_rom_red_turret_routine_01(core, x); break;
                case 0x03u: contra_rom_red_turret_routine_02(core, x); break;
                case 0x04u: contra_rom_red_turret_routine_03(core, x); break;
                case 0x05u: contra_rom_red_turret_routine_04(core, x); break;
                case 0x06u: contra_rom_red_turret_routine_05(core, x); break;
                /* routines 7-9: the appended shared explosion trio, type kept */
                case 0x07u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x08u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x09u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x04u: /* rotating gun */
            switch (routine)
            {
                case 0x01u: contra_rom_rotating_gun_routine_00(core, x); break;
                case 0x02u: contra_rom_rotating_gun_routine_01(core, x); break;
                case 0x03u: contra_rom_rotating_gun_routine_02(core, x); break;
                case 0x04u: contra_rom_rotating_gun_routine_03(core, x); break;
                case 0x05u: contra_rom_rotating_gun_routine_04(core, x); break;
                case 0x06u: contra_rom_rotating_gun_routine_05(core, x); break;
                case 0x07u: contra_rom_rotating_gun_routine_06(core, x); break;
                /* routines 8-10: the appended shared explosion trio, type kept */
                case 0x08u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x09u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x0Au: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x01u: /* enemy bullet */
            switch (routine)
            {
                case 0x01u: contra_rom_enemy_bullet_routine_00(core, x); break;
                case 0x02u: contra_rom_enemy_bullet_routine_01(core, x); break;
                case 0x03u: contra_rom_enemy_bullet_routine_02(core, x); break;
                case 0x04u: contra_rom_remove_enemy_offscreen(core, x); break; /* remove_enemy keeps the husk */
                default: break;
            }
            break;
        case 0xFEu: /* explosion actor (killed enemy) */
            contra_rom_enemy_routine_explosion(core, x);
            break;
        case 0x03u: /* flying capsule / weapon zeppelin */
            switch (routine)
            {
                case 0x01u: contra_rom_flying_capsule_routine_00(core, x); break;
                case 0x02u: contra_rom_flying_capsule_routine_01(core, x); break;
                case 0x03u: contra_rom_flying_capsule_routine_02(core, x); break;
                default: break; /* picked up / removed */
            }
            break;
        case 0x05u: /* soldier / running man */
            switch (routine)
            {
                case 0x01u: contra_rom_soldier_routine_00(core, x); break;
                case 0x02u: contra_rom_soldier_routine_01(core, x); break;
                case 0x03u: contra_rom_soldier_routine_02(core, x); break;
                case 0x04u: contra_rom_soldier_routine_03(core, x); break;
                case 0x05u: contra_rom_soldier_routine_04(core, x); break;
                case 0x06u: contra_rom_soldier_routine_05(core, x); break;
                case 0x07u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x08u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x09u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                case 0x0Au: contra_rom_soldier_routine_09(core, x); break;
                case 0x0Bu: contra_rom_soldier_routine_0a(core, x); break;
                default: break;
            }
            break;
        case 0x02u: /* pill box / weapon box */
            switch (routine)
            {
                case 0x01u: contra_rom_weapon_box_routine_00(core, x); break;
                case 0x02u: contra_rom_weapon_box_routine_01(core, x); break;
                case 0x03u: contra_rom_weapon_box_routine_02(core, x); break;
                case 0x04u: contra_rom_weapon_box_routine_03(core, x); break;
                case 0x05u: contra_rom_weapon_box_routine_04(core, x); break;
                /* routines 6-8 and 9-11: the appended shared explosion trio,
                   also run by spawned explosion actors (create_explosion_sequence). */
                case 0x06u: case 0x09u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x07u: case 0x0Au: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x08u: case 0x0Bu: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x00u: /* weapon item (power-up drop) */
            switch (routine)
            {
                case 0x01u: contra_rom_weapon_item_routine_00(core, x); break;
                case 0x02u: contra_rom_weapon_item_routine_01(core, x); break;
                case 0x03u: contra_rom_weapon_item_routine_02(core, x); break;
                default: break; /* picked up / removed */
            }
            break;
        case 0x0Cu: /* scuba diver (water mortar soldier) */
            switch (routine)
            {
                case 0x01u: contra_rom_scuba_soldier_routine_00(core, x); break;
                case 0x02u: contra_rom_scuba_soldier_routine_01(core, x); break;
                case 0x03u: contra_rom_scuba_soldier_routine_02(core, x); break;
                case 0x04u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x05u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x06u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x0Bu: /* mortar shot */
            switch (routine)
            {
                case 0x01u: contra_rom_mortar_shot_routine_00(core, x); break;
                case 0x02u: contra_rom_mortar_shot_routine_01(core, x); break;
                case 0x03u: contra_rom_mortar_shot_routine_02(core, x); break;
                /* 4-6: the initial shot's explosion trio; 7-9: a split round's
                   ground-hit trio (mortar_shot_routine_03 entry). */
                case 0x04u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x07u: contra_rom_mortar_shot_routine_03(core, x); break;
                case 0x05u: case 0x08u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x06u: case 0x09u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        case 0x06u: /* sniper */
            switch (routine)
            {
                case 0x01u: contra_rom_sniper_routine_00(core, x); break;
                case 0x02u: contra_rom_sniper_routine_01(core, x); break;
                case 0x03u: contra_rom_sniper_routine_02(core, x); break;
                case 0x04u: contra_rom_sniper_routine_03(core, x); break;
                case 0x05u: contra_rom_sniper_routine_04(core, x); break;
                case 0x06u: contra_rom_sniper_routine_05(core, x); break;
                case 0x07u: contra_rom_enemy_routine_init_explosion_inplace(core, x); break;
                case 0x08u: contra_rom_enemy_routine_explosion_inplace(core, x); break;
                case 0x09u: contra_rom_enemy_routine_remove_inplace(core, x); break;
                default: break;
            }
            break;
        default:
            break; /* type not yet ported */
    }
}

/* collision_box_codes_04 (bank7.asm:7297): the bullet-vs-enemy hitbox per enemy
   collision code (4 bytes: y offset, x offset, height, width). */
static const uint8_t contra_collision_box_codes_04[15][4] = {
    {0xEEu, 0xF5u, 0x24u, 0x16u}, {0xFCu, 0xFCu, 0x08u, 0x08u},
    {0xF5u, 0xF5u, 0x16u, 0x16u}, {0xEFu, 0xEFu, 0x22u, 0x22u},
    {0xE0u, 0xF0u, 0x08u, 0x20u}, {0xFAu, 0xFAu, 0x0Cu, 0x0Cu},
    {0xF3u, 0xFAu, 0x16u, 0x0Cu}, {0xE4u, 0xE4u, 0x38u, 0x38u},
    {0xEEu, 0xE4u, 0x24u, 0x38u}, {0xE1u, 0xD5u, 0x44u, 0x56u},
    {0xFDu, 0xF8u, 0x12u, 0x12u}, {0x05u, 0xF3u, 0x0Eu, 0x1Au},
    {0xF3u, 0xF3u, 0x1Au, 0x1Au}, {0xE7u, 0xEEu, 0x33u, 0x24u},
    {0xF2u, 0xF2u, 0x13u, 0x1Cu}};

/* add_player_score (bank7:1247-1339): add a 2-byte score (units of 100 game
   points) to the player's score, award the 30,000-point extra lives, and track
   the high score. Scores are interleaved 2 bytes per player at $07E2; the
   extra-life thresholds live at $3C (start 0xC8 = 20,000 points, +0x12C per
   life, capped once the high byte reaches 0x75). The special falcon score
   (add_hi != 0, 0x1388 = 500,000) skips the threshold compare AND the cap:
   it always grants one life and advances the threshold by the same 0x1388. */
static void contra_rom_add_player_score(ContraCore *core, uint8_t player, uint8_t add_lo, uint8_t add_hi)
{
    uint8_t *const ram = core->ram;
    const unsigned score = CONTRA_RAM_PLAYER_1_SCORE_LOW + ((unsigned)player * 2u);
    const unsigned thr = CONTRA_RAM_EXTRA_LIFE_SCORE_LOW + ((unsigned)player * 2u);
    const unsigned lo = (unsigned)ram[score] + add_lo;
    const unsigned hi = (unsigned)ram[score + 1u] + add_hi + (lo >> 8u);
    bool award;

    if (hi >= 0x100u)
    {
        ram[score] = 0xFFu; /* maxed out at 9,999,900 */
        ram[score + 1u] = 0xFFu;
    }
    else
    {
        ram[score] = (uint8_t)lo;
        ram[score + 1u] = (uint8_t)hi;
    }

    if (add_hi != 0u)
    {
        award = true; /* falcon score: unconditional */
    }
    else if ((ram[score + 1u] > ram[thr + 1u]) ||
             ((ram[score + 1u] == ram[thr + 1u]) && (ram[score] >= ram[thr])))
    {
        award = ram[thr + 1u] < 0x75u; /* past 2,995,200 no more lives */
    }
    else
    {
        award = false;
    }

    if (award)
    {
        const unsigned step_lo = (add_hi != 0u) ? 0x88u : 0x2Cu;
        const unsigned step_hi = (add_hi != 0u) ? 0x13u : 0x01u;
        const unsigned tlo = (unsigned)ram[thr] + step_lo;
        const unsigned thi = (unsigned)ram[thr + 1u] + step_hi + (tlo >> 8u);
        uint8_t lives;

        if (thi >= 0x100u)
        {
            ram[thr] = 0xFFu;
            ram[thr + 1u] = 0xFFu;
        }
        else
        {
            ram[thr] = (uint8_t)tlo;
            ram[thr + 1u] = (uint8_t)thi;
        }

        lives = (uint8_t)(ram[CONTRA_RAM_P1_NUM_LIVES + player] + 1u);
        if (lives >= 0x63u)
        {
            lives = 0x63u;
        }
        ram[CONTRA_RAM_P1_NUM_LIVES + player] = lives;
        if (add_hi == 0u)
        {
            contra_play_sound(core, 0x20u); /* sound_20: extra life */
        }
    }

    /* @set_if_new_high_score */
    if ((ram[score + 1u] > ram[CONTRA_RAM_HIGH_SCORE_HIGH]) ||
        ((ram[score + 1u] == ram[CONTRA_RAM_HIGH_SCORE_HIGH]) &&
         (ram[score] >= ram[CONTRA_RAM_HIGH_SCORE_LOW])))
    {
        ram[CONTRA_RAM_HIGH_SCORE_LOW] = ram[score];
        ram[CONTRA_RAM_HIGH_SCORE_HIGH] = ram[score + 1u];
    }
}

/* add_enemy_score_set_enemy_routine (bank7:7998-8027), the scoring half: pull
   the score code from the high nibble of ENEMY_SCORE_COLLISION, add the points
   (code 0x0A is the 2-byte falcon score and bypasses the demo-mode gate, which
   lives in add_player_low_score), then clear the score nibble. */
static void contra_rom_add_enemy_score(ContraCore *core, uint8_t slot, uint8_t player)
{
    static const uint8_t score_codes_tbl[10] = {
        0x00u, 0x01u, 0x03u, 0x05u, 0x0Au, 0x14u, 0x1Eu, 0x32u, 0x64u, 0x96u};
    uint8_t *const ram = core->ram;
    const uint8_t code = (uint8_t)(ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] >> 4u);

    if (code == 0x0Au)
    {
        contra_rom_add_player_score(core, player, 0x88u, 0x13u); /* 500,000 */
    }
    else if ((code < 10u) && (score_codes_tbl[code] != 0u) &&
             (ram[CONTRA_RAM_DEMO_MODE] == 0u))
    {
        contra_rom_add_player_score(core, player, score_codes_tbl[code], 0x00u);
    }
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] & 0x0Fu);
}

/* bullet_enemy_collision_test (bank7.asm:6928) + set_enemy_collision_box +
   bullet_collision_logic, outdoor path: test each live player bullet against the
   enemy's bullet hitbox; on a hit subtract HP (HP >= 0xF0 is invulnerable, e.g.
   the open pill box) and remove the enemy when HP reaches 0. Laser hits consume
   the beam's oldest segment (pass-through). */
/* set_enemy_collision_box @collision_code_f (bank7:7136): the variable-size
   bullet hitbox used by the fire beams and rising/standing spiked walls
   (collision code 0x0F). The box grows with ENEMY_VAR_1 (negated when
   ENEMY_ATTRIBUTES bit 6 is set); the adj-table placeholder bytes 0xFF/0xFE are
   replaced by +/-(VAR_1 + 8). Without this the L6/L7 spiked walls were never
   hit-tested and could not be destroyed. */
static void contra_rom_collision_code_f_bullet_box(
    const ContraCore *core, uint8_t slot,
    uint8_t *box_y, uint8_t *box_x, uint8_t *box_h, uint8_t *box_w)
{
    static const uint8_t base[4] = {0xFEu, 0xFEu, 0x04u, 0x04u}; /* bullet box (table 4) */
    static const uint8_t adj[4][4] = {
        {0xFAu, 0xF8u, 0x0Cu, 0xFFu}, /* variable width, fixed x (growing right) */
        {0xFAu, 0xFEu, 0x0Cu, 0xFFu}, /* variable width, variable x (growing left) */
        {0xF8u, 0xFAu, 0xFFu, 0x0Cu}, /* variable height, fixed y (growing down) */
        {0xFEu, 0xF6u, 0xFFu, 0x14u}, /* variable height, variable y (growing up) */
    };
    const uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot];
    const uint8_t var1 = ram[CONTRA_RAM_ENEMY_VAR_1 + slot];
    const uint8_t ctrl = (uint8_t)(((attr & 0x40u) ? (uint8_t)(0u - var1) : var1) + 0x08u);
    const uint8_t *const a = adj[((attr >> 4u) & 0x0Cu) >> 2u];
    uint8_t v[4];
    int i;

    for (i = 0; i < 4; ++i)
    {
        const uint8_t raw = a[i];
        v[i] = (raw < 0xFEu) ? raw : ((raw == 0xFFu) ? ctrl : (uint8_t)(0u - ctrl));
    }
    *box_y = (uint8_t)(v[0] + base[0] + ram[CONTRA_RAM_ENEMY_Y_POS + slot]);
    *box_x = (uint8_t)(v[1] + base[1] + ram[CONTRA_RAM_ENEMY_X_POS + slot]);
    *box_h = (uint8_t)(v[2] + base[2]);
    *box_w = (uint8_t)(v[3] + base[3]);
}

static void contra_rom_bullet_enemy_collision_test(ContraCore *core, uint8_t slot)
{
    uint8_t *const ram = core->ram;
    const uint8_t code = (uint8_t)(ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] & 0x0Fu);
    uint8_t box_y;
    uint8_t box_x;
    uint8_t box_h;
    uint8_t box_w;
    int bullet;

    if (code >= 15u)
    {
        /* collision code 0x0F: variable box (fire beams / spiked walls) */
        contra_rom_collision_code_f_bullet_box(core, slot, &box_y, &box_x, &box_h, &box_w);
    }
    else
    {
        const uint8_t *const box = contra_collision_box_codes_04[code];

        box_y = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + slot] + box[0]);
        box_x = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + slot] + box[1]);
        box_h = box[2];
        box_w = box[3];
    }

    for (bullet = 0x0F; bullet >= 0; --bullet)
    {
        const unsigned b = (unsigned)bullet;
        uint8_t borrow = 1u;
        uint8_t diff;
        uint8_t hp;

        if ((ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + b] == 0u) ||
            (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + b] != 0x01u))
        {
            continue;
        }
        if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
        {
            const uint8_t indoor_box_gate =
                (uint8_t)((ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot] & 0x30u) << 2u);

            if ((indoor_box_gate & 0x80u) == 0u)
            {
                if (ram[CONTRA_RAM_PLAYER_BULLET_TIMER + b] >= 0x02u)
                {
                    continue;
                }
            }
            else
            {
                if ((ram[CONTRA_RAM_PLAYER_BULLET_SLOT + b] & 0x80u) == 0u)
                {
                    continue;
                }
                /* indoor floor enemies reach the box test through the SLOT
                   bit-7 path, where the 6502 carry is still SET from the
                   location-type lsr (bank7:6973) -- no -1 borrow. The outdoor
                   path (carry clear from the lsr) and the indoor wall-enemy
                   path (carry clear from `cmp #$02` with timer < 2) keep it. */
                borrow = 0u;
            }
        }
        diff = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + b] - box_y - borrow);
        if (diff >= box_h)
        {
            continue;
        }
        /* the X sbc's borrow is always 1: its carry comes from the Y `cmp`,
           which is clear whenever the Y test passed (bank7:6990) */
        diff = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_X_POS + b] - box_x - 1u);
        if (diff >= box_w)
        {
            continue;
        }

        /* hit -- the ROM's loop RTSes after the first overlapping bullet
           (bank7:7035), so an enemy takes at most ONE bullet per frame; the
           bullet is consumed (routine 2) even against a dead/invulnerable
           enemy. */
        hp = ram[CONTRA_RAM_ENEMY_HP + slot];
        if (ram[CONTRA_RAM_PLAYER_BULLET_SLOT + b] == 0x05u)
        {
            /* laser (bank7:7013-7032): consume the beam's OLDEST in-flight
               segment (the owner's first slot in routine 1) instead of the
               hit one -- the beam collapses from the tail while a long
               overlap keeps damaging each frame (the pass-through). */
            unsigned s = (b < 0x0Au) ? 0u : 0x0Au;

            while ((s != b) && (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + s] != 0x01u))
            {
                ++s;
            }
            ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + s] = 0x02u;
            ram[CONTRA_RAM_PLAYER_BULLET_TIMER + s] = 0x06u;
        }
        else
        {
            ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + b] = 0x02u;
            ram[CONTRA_RAM_PLAYER_BULLET_TIMER + b] = 0x06u;
        }
        if ((hp == 0u) || (hp >= 0xF0u))
        {
            return; /* already dead, or invulnerable this frame */
        }
        hp = (uint8_t)(hp - 1u);
        ram[CONTRA_RAM_ENEMY_HP + slot] = hp;
        if (hp == 0u)
        {
            /* bullet_collision_logic (bank7:7078): the kill awards the bullet
               owner's score (and possibly an extra life) before the destroyed
               routine is dispatched. */
            contra_rom_add_enemy_score(core, slot, ram[CONTRA_RAM_PLAYER_BULLET_OWNER + b]);
            /* set_destroyed_enemy_routine (bank7:7977): a killed enemy routes to
               its type's destroyed routine (enemy_destroyed_routine tables) rather
               than always exploding. The cases we cover (RAM routine value =
               nibble+1 from those tables): wall turret/core 0x13/0x14 -> 5
               (their routine_04 plating/destruction); boss-room wall cannon 0x08
               -> 5 (routine_04 destroyed tile); wall plating 0x0A -> 4 (routine_03
               destroyed, bumps the plating count); level-2 boss eye 0x10 -> 4
               (routine_03 dec real HP). Everything else takes the explosion actor. */
            const uint8_t dead_type = ram[CONTRA_RAM_ENEMY_TYPE + slot];
            const bool is_indoor_base =
                (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) || (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u);
            uint8_t dest_routine = 0u;

            if ((dead_type == 0x14u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u))
            {
                /* dragon boss mouth defeated: the nibble (high of $65, bank7
                   L3 table) only RAISES the routine to 6 = boss_defeated_routine,
                   which runs NEXT frame (sound, DELAY=0xFF, destroy_all). */
                contra_rom_set_enemy_routine_to_a(core, slot, 0x06u);
                return;
            }
            if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u) && (dead_type >= 0x10u))
            {
                /* L8 nibbles (bank7:8143-8146): $43 guardian/fetus, $34
                   mouth/blob, $63 spider/spawn, $40 heart -- each an entry in
                   its own per-type routine table. */
                static const uint8_t l8_kill_routine[7] = {
                    0x04u, 0x03u, 0x03u, 0x04u, 0x06u, 0x03u, 0x04u};

                if (dead_type <= 0x16u)
                {
                    dest_routine = l8_kill_routine[dead_type - 0x10u];
                }
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u) &&
                     (dead_type >= 0x11u) && (dead_type <= 0x18u))
            {
                /* L7 nibbles (bank7:8138-8142): rising spiked wall $05 low,
                   spiked wall $30 high, moving/immobile cart $44, armored
                   door $35 high, mortar $35 low, soldier generator $50 high */
                static const uint8_t l7_kill_routine[8] = {
                    0x05u, 0x03u, 0x00u, 0x04u, 0x04u, 0x03u, 0x05u, 0x05u};

                dest_routine = l7_kill_routine[dead_type - 0x11u];
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u) && (dead_type == 0x13u))
            {
                dest_routine = 0x07u; /* L6 giant robot: nibble $07 low (bank7:8137)
                                         -> boss_giant_soldier_routine_06 */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u) && (dead_type == 0x14u))
            {
                dest_routine = 0x03u; /* L6 spiked disk: nibble $30 high -> clear */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u) && (dead_type == 0x13u))
            {
                dest_routine = 0x04u; /* L3 falling rock: nibble $24 low (bank7:8133)
                                         -> its appended init_explosion */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u) && (dead_type == 0x14u))
            {
                dest_routine = 0x0Au; /* L5 boss UFO: nibble $a5 high (bank7:8129)
                                         -> boss_ufo_routine_09 (destroyed) */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u) && (dead_type == 0x15u))
            {
                dest_routine = 0x05u; /* L5 flying saucer: nibble $a5 low -> its
                                         table's appended init_explosion */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u) && (dead_type == 0x16u))
            {
                dest_routine = 0x02u; /* L5 drop bomb: nibble $20 high -> its
                                         table's appended init_explosion */
            }
            else if ((dead_type == 0x13u) || (dead_type == 0x14u) || (dead_type == 0x08u))
            {
                dest_routine = 0x05u;
            }
            else if ((is_indoor_base != 0) &&
                     ((dead_type == 0x15u) || (dead_type == 0x16u) ||
                      (dead_type == 0x17u)))
            {
                dest_routine = 0x03u;
            }
            else if ((is_indoor_base != 0) && (dead_type == 0x18u))
            {
                dest_routine = 0x04u; /* group of 4: nibble $43 high (bank7:8110)
                                         -- straight to the knockback animation */
            }
            else if ((is_indoor_base != 0) && (dead_type == 0x11u))
            {
                dest_routine = 0x03u; /* roller: nibble $43 low -> init_explosion */
            }
            else if ((is_indoor_base != 0) && (dead_type == 0x1Bu))
            {
                dest_routine = 0x03u; /* eye sphere: nibble $33 low -> init_explosion */
            }
            else if ((dead_type == 0x15u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u))
            {
                dest_routine = 0x05u; /* level-3 dragon arm orb -> dragon_arm_orb_routine_04.
                                         On level 2, type 0x15 is the indoor running soldier,
                                         whose dispatch has no routine 5 -- it must fall through
                                         to the 0xFE explosion actor or the killed soldier freezes
                                         in front of the core and blocks the room. */
            }
            else if ((dead_type == 0x0Au) ||
                     ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) && (dead_type == 0x10u)))
            {
                dest_routine = 0x04u;
            }
            else if ((dead_type == 0x0Bu) || (dead_type == 0x0Cu))
            {
                dest_routine = 0x04u; /* mortar shot / scuba diver: nibbles $44/$4x
                                         (bank7:8104-8105) -> init_explosion */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u) && (dead_type == 0x10u))
            {
                dest_routine = 0x03u; /* level-1 boss bomb turret -> routine_02
                                         (nibble $33 high, bank7:8104) */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u) && (dead_type == 0x1Cu))
            {
                dest_routine = 0x04u; /* boss_gemini_routine_03 */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u) &&
                     ((dead_type == 0x1Eu) || (dead_type == 0x1Fu)))
            {
                /* L4 boss-screen blue/red soldiers: nibble $54 (bank7:8119) --
                   each lands on its table's appended init_explosion */
                dest_routine = (dead_type == 0x1Eu) ? 0x05u : 0x04u;
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u) && (dead_type == 0x1Du))
            {
                dest_routine = 0x03u; /* spinning bubble: nibble $43 low -> its
                                         appended init_explosion */
            }
            else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u) && (dead_type == 0x12u))
            {
                dest_routine = 0x05u; /* L5 tank: nibble $50 high (bank7:8128)
                                         -> tank_routine_04 (demolition) */
            }
            else if (dead_type == 0x04u)
            {
                dest_routine = 0x07u; /* rotating gun -> routine_06 (restore rock, explode) */
            }
            else if ((dead_type == 0x05u) || (dead_type == 0x06u))
            {
                dest_routine = 0x05u; /* soldier/sniper -> routine_04 (corpse death arc) */
            }
            else if (dead_type == 0x02u)
            {
                dest_routine = 0x05u; /* pill box -> routine_04 (restore bg, drop weapon item) */
            }
            else if (dead_type == 0x07u)
            {
                dest_routine = 0x06u; /* red turret -> routine_05 (restore bg, explode) */
            }
            else if (dead_type == 0x03u)
            {
                dest_routine = 0x03u; /* flying capsule -> routine_02 (drop weapon item) */
            }
            else if ((dead_type == 0x11u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u))
            {
                dest_routine = 0x03u; /* L1 fortress boss door -> boss_defeated_routine */
            }
            if (dest_routine != 0u)
            {
                contra_rom_set_enemy_routine_to_a(core, slot, dest_routine);
                return;
            }
            contra_rom_begin_enemy_explosion(core, slot);
            return;
        }
        return; /* survived the hit: at most one bullet per enemy per frame */
    }
}

/* Player collision boxes vs an enemy, selected by player state (4 bytes each:
   y offset, x offset, height, width). The ROM's collision_box_codes_tbl
   (bank7.asm:7211) indexes these tables by the player-state offset computed in
   check_players_collision (@set_collision_code_offset, bank7.asm:6728-6747):
     0 = in water    (collision_box_codes_00, bank7.asm:7221)
     1 = jumping     (collision_box_codes_01, bank7.asm:7240)
     2 = crouching   (collision_box_codes_02, bank7.asm:7259) -- lower/shorter
     3 = standing    (collision_box_codes_03, bank7.asm:7278) -- tall
   The crouch box is lower and shorter than the standing box, so a crouched
   player ducks under head-height bullets. */
static const uint8_t contra_collision_box_codes_00[15][4] = {
    {0xF1u, 0xF7u, 0x28u, 0x12u}, {0xFEu, 0xF9u, 0x14u, 0x14u},
    {0xF8u, 0xF3u, 0x1Au, 0x1Au}, {0xF2u, 0xEDu, 0x26u, 0x26u},
    {0xE0u, 0xF0u, 0x08u, 0x20u}, {0xFDu, 0xF8u, 0x10u, 0x10u},
    {0xF5u, 0xF7u, 0x20u, 0x12u}, {0xE7u, 0xE2u, 0x3Cu, 0x3Cu},
    {0xF1u, 0xE2u, 0x28u, 0x3Cu}, {0xE4u, 0xD3u, 0x48u, 0x5Au},
    {0x00u, 0xF6u, 0x16u, 0x16u}, {0x08u, 0xF1u, 0x12u, 0x1Eu},
    {0xF5u, 0xF1u, 0x20u, 0x1Eu}, {0xEAu, 0xECu, 0x37u, 0x28u},
    {0xF3u, 0xF3u, 0x11u, 0x1Au}};
static const uint8_t contra_collision_box_codes_01[15][4] = {
    {0xEAu, 0xF8u, 0x1Eu, 0x10u}, {0xF6u, 0xFAu, 0x0Cu, 0x0Cu},
    {0xF1u, 0xF5u, 0x12u, 0x16u}, {0xEAu, 0xEEu, 0x24u, 0x24u},
    {0xE0u, 0xF0u, 0x08u, 0x20u}, {0xF5u, 0xF9u, 0x0Eu, 0x0Eu},
    {0xEDu, 0xF8u, 0x1Eu, 0x10u}, {0xDFu, 0xE3u, 0x3Au, 0x3Au},
    {0xE9u, 0xE3u, 0x26u, 0x3Au}, {0xDCu, 0xD4u, 0x46u, 0x58u},
    {0xF8u, 0xF7u, 0x14u, 0x14u}, {0x00u, 0xF2u, 0x10u, 0x1Cu},
    {0xEDu, 0xF2u, 0x1Eu, 0x1Cu}, {0xE2u, 0xEDu, 0x35u, 0x26u},
    {0xF3u, 0xF1u, 0x12u, 0x1Eu}};
static const uint8_t contra_collision_box_codes_02[15][4] = {
    {0xF4u, 0xF1u, 0x0Du, 0x1Eu}, {0xF3u, 0xF4u, 0x04u, 0x18u},
    {0xECu, 0xEDu, 0x10u, 0x26u}, {0xE6u, 0xE7u, 0x1Cu, 0x32u},
    {0xE0u, 0xF0u, 0x08u, 0x20u}, {0xF1u, 0xF2u, 0x06u, 0x1Cu},
    {0xE9u, 0xF1u, 0x16u, 0x1Eu}, {0xDBu, 0xDCu, 0x32u, 0x48u},
    {0xE5u, 0xDCu, 0x1Eu, 0x48u}, {0xD8u, 0xCDu, 0x3Eu, 0x66u},
    {0xF4u, 0xF0u, 0x19u, 0x22u}, {0xFCu, 0xEBu, 0x08u, 0x2Au},
    {0xE3u, 0xF2u, 0x1Au, 0x1Cu}, {0xDEu, 0xE6u, 0x2Du, 0x34u},
    {0xE9u, 0xEAu, 0x0Du, 0x2Cu}};
static const uint8_t contra_collision_box_codes_03[15][4] = {
    {0xF3u, 0xF8u, 0x24u, 0x10u}, {0xF0u, 0xFBu, 0x1Fu, 0x0Au},
    {0xEAu, 0xF5u, 0x2Bu, 0x16u}, {0xE3u, 0xEEu, 0x39u, 0x24u},
    {0xE0u, 0xF0u, 0x08u, 0x20u}, {0xEEu, 0xF9u, 0x23u, 0x0Eu},
    {0xE6u, 0xF8u, 0x33u, 0x10u}, {0xD8u, 0xE3u, 0x4Fu, 0x3Au},
    {0xE2u, 0xE3u, 0x3Bu, 0x3Au}, {0xD5u, 0xD4u, 0x5Bu, 0x58u},
    {0xF1u, 0xF7u, 0x29u, 0x14u}, {0xF9u, 0xF2u, 0x25u, 0x1Cu},
    {0xE4u, 0xF2u, 0x35u, 0x1Cu}, {0xDAu, 0xEDu, 0x4Au, 0x26u},
    {0xE6u, 0xF1u, 0x2Au, 0x1Eu}};

/* check_players_collision (bank7.asm:6671): if a normal-state player's body
   overlaps this enemy's collision box, kill the player (barrier invincibility
   destroys the enemy instead; new-life invincibility passes through). The
   collision box is selected by the player's state -- water/jumping/crouching/
   standing -- and on indoor levels a crouching player is immune to bullets
   (ducks under them). Rideable enemies (floating rocks, mining carts) are
   landed on and ridden instead of colliding. */
static void contra_rom_check_players_collision(ContraCore *core, uint8_t slot)
{
    uint8_t *const ram = core->ram;
    const uint8_t code = (uint8_t)(ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] & 0x0Fu);
    int player;

    if (code >= 15u)
    {
        return;
    }
    for (player = 1; player >= 0; --player)
    {
        const unsigned p = (unsigned)player;
        const uint8_t (*tbl)[4];
        const uint8_t *box;
        uint8_t box_y;
        uint8_t box_x;

        if (ram[CONTRA_RAM_PLAYER_STATE + p] != 0x01u)
        {
            continue;
        }
        /* @handle_outdoor_level / @check_player_jumping (bank7:6694-6726): for a
           landable enemy (STATE_WIDTH bit 6 set) with bit 5 clear -- the floating
           rock platform -- a player who is jumping UP (Y fast velocity negative) or
           at the apex (velocity 0) passes straight through. This is exactly what
           lets the player jump OFF the platform instead of being re-pinned to it.
           With bit 5 SET (the mining carts) the ROM instead re-aims the
           land/collide gate (STATE_WIDTH bit 4) each frame: set (a side hit
           kills) when the player is below or within 8px of the top, cleared
           (landable) when the player is 8+ px above. */
        {
            const uint8_t sw = ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot];

            if (((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x01u) == 0u) &&
                ((sw & 0x40u) != 0u) && ((sw & 0x20u) != 0u))
            {
                const uint8_t ey = ram[CONTRA_RAM_ENEMY_Y_POS + slot];
                const uint8_t py = ram[CONTRA_RAM_SPRITE_Y_POS + p];

                if ((ey >= py) && ((uint8_t)(ey - py) >= 0x08u))
                {
                    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot] = (uint8_t)(sw & 0xEFu);
                }
                else
                {
                    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot] = (uint8_t)(sw | 0x10u);
                }
            }
            else if (((sw & 0x40u) != 0u) && ((sw & 0x20u) == 0u) &&
                     (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + p] != 0u))
            {
                const uint8_t yv = ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + p];

                if ((yv == 0u) || ((yv & 0x80u) != 0u))
                {
                    continue; /* ascending through the platform */
                }
            }
        }
        /* Indoor levels (LEVEL_LOCATION_TYPE bit 0 set): a crouching player is
           immune to enemy bullets -- they duck underneath (bank7.asm:6682-6692).
           PLAYER_SPRITE_SEQUENCE == 2 is the crouch animation; ENEMY_TYPE 1 is a
           bullet. */
        if (((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x01u) != 0u) &&
            (ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x01u) &&
            (ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + p] == 0x02u))
        {
            continue;
        }
        /* Select the player-state collision box (bank7.asm:6728-6747):
           water(0)/jumping(1)/crouching(2)/standing(3). */
        if (ram[CONTRA_RAM_PLAYER_WATER_STATE + p] != 0u)
        {
            if ((ram[CONTRA_RAM_CONTROLLER_STATE + p] & 0x04u) != 0u)
            {
                continue; /* invisible while crouching in water -- no collision */
            }
            tbl = contra_collision_box_codes_00; /* in water */
        }
        else if (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + p] != 0u)
        {
            tbl = contra_collision_box_codes_01; /* jumping */
        }
        else if (ram[CONTRA_RAM_PLAYER_SPRITE_CODE + p] == 0x17u)
        {
            tbl = contra_collision_box_codes_02; /* crouching (lower/shorter box) */
        }
        else
        {
            tbl = contra_collision_box_codes_03; /* standing */
        }
        box = tbl[code];
        box_y = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + slot] + box[0]);
        box_x = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + slot] + box[1]);
        if ((uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + p] - box_y) >= box[2])
        {
            continue;
        }
        if ((uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + p] - box_x) >= box[3])
        {
            continue; /* outside the box */
        }
        /* overlap */

        /* @check_can_land_on / @land_on_enemy (bank7:6822-6810): a "landable" enemy
           (ENEMY_STATE_WIDTH bit 6 set, bit 4 clear) -- the floating rock platform
           and mining carts -- is ridden, not collided with. The player lands on top
           when descending onto it (Y >= PLAYER_FALL_X_FREEZE) and passes through it
           from below. */
        {
            const uint8_t sw = ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot];
            const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + slot];
            const uint8_t pxc = ram[CONTRA_RAM_SPRITE_X_POS + p];
            const uint8_t dx = (ex >= pxc) ? (uint8_t)(ex - pxc) : (uint8_t)(pxc - ex);

            if ((dx < 0x80u) && ((sw & 0x40u) != 0u) && ((sw & 0x10u) == 0u))
            {
                if (ram[CONTRA_RAM_SPRITE_Y_POS + p] >= ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + p])
                {
                    const uint8_t landing = ((sw & 0x20u) != 0u) ? 0xE8u : 0xE4u;
                    const uint8_t carry =
                        (((uint16_t)ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + slot] +
                          ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + slot]) > 0xFFu) ? 1u : 0u;

                    ram[CONTRA_RAM_ENEMY_FRAME + slot] = 0x01u; /* start the platform moving */
                    ram[CONTRA_RAM_PLAYER_FAST_X_VEL_BOOST + p] =
                        (uint8_t)(ram[CONTRA_RAM_PLAYER_FAST_X_VEL_BOOST + p] +
                                  ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + slot] + carry);
                    ram[CONTRA_RAM_SPRITE_Y_POS + p] =
                        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + slot] + landing);
                    ram[CONTRA_RAM_PLAYER_ON_ENEMY + p] = 0x01u;
                    contra_land_player_on_ground(core, (uint8_t)p);
                }
                continue; /* landed on, or passed through, the platform */
            }
        }

        if (ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x00u)
        {
            /* weapon item: pick it up (the ROM checks this before invincibility,
               so even a barrier player collects it). */
            contra_rom_pick_up_weapon_item(core, slot, (uint8_t)p);
            return; /* item removed -- done with this enemy */
        }
        if (ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + p] != 0u)
        {
            continue; /* invincible just after respawn -- walk through */
        }
        if (ram[CONTRA_RAM_INVINCIBILITY_TIMER + p] != 0u)
        {
            const uint8_t hp = ram[CONTRA_RAM_ENEMY_HP + slot];

            if ((hp != 0u) && (hp < 0xF0u))
            {
                contra_rom_begin_enemy_explosion(core, slot); /* barrier: destroy enemy */
            }
            continue;
        }
        contra_kill_player(core, (uint8_t)p);
        /* bank7:6863-6866: the enemy BULLET that hit the player is removed
           (remove_current_enemy keeps the type-01 husk); other enemy types
           survive the collision. */
        if (ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x01u)
        {
            contra_rom_remove_enemy(core, (uint8_t)slot);
        }
    }
}

/* exe_all_enemy_routine (bank7.asm:7315): run every active enemy's routine, then
   test player-body and player-bullet collision against it, gated like the ROM
   dispatcher (ENEMY_SPRITES set; body collision when STATE_WIDTH bit 0 clear;
   bullet collision when bit 7 clear). */
static void contra_rom_exe_all_enemy_routine(ContraCore *core)
{
    int slot;

    /* clear the "player is riding a non-dangerous enemy" flags (bank7:7317); they
       are re-set this frame only if the player is still standing on a platform. */
    core->ram[CONTRA_RAM_PLAYER_ON_ENEMY + 0u] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_ON_ENEMY + 1u] = 0x00u;

    for (slot = 0x0F; slot >= 0; --slot)
    {
        const uint8_t sx = (uint8_t)slot;

        if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + sx] == 0u)
        {
            continue;
        }
        contra_rom_exe_enemy_type(core, sx);
        if ((core->ram[CONTRA_RAM_ENEMY_ROUTINE + sx] == 0u) ||
            (core->ram[CONTRA_RAM_ENEMY_SPRITES + sx] == 0u))
        {
            continue;
        }
        /* On a regular indoor level (LEVEL_LOCATION_TYPE bit 0 set; 0x80 indoor-boss
           takes the outdoor path), an enemy high on the pseudo-3D corridor
           (ENEMY_Y_POS < 0x9C) is too far up to reach the player, so the player-body
           collision is skipped (@handle_outdoor, bank7:7330-7340). Without this, a
           bullet still descending toward a player (y < 0x9C) registers a hit one
           frame early -- e.g. it killed demo P2 before it could crouch under the shot.
           The bullet-vs-enemy test below still runs (the ROM's @continue path). */
        if ((((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x01u) == 0u) ||
             (core->ram[CONTRA_RAM_ENEMY_Y_POS + sx] >= 0x9Cu)) &&
            ((core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + sx] & 0x01u) == 0u))
        {
            contra_rom_check_players_collision(core, sx);
        }
        if ((core->ram[CONTRA_RAM_ENEMY_ROUTINE + sx] != 0u) &&
            (core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + sx] & 0x80u) == 0u)
        {
            contra_rom_bullet_enemy_collision_test(core, sx);
        }
    }
}

/* Faithful level-2/4 (indoor base) enemy system. The faithful real-RAM indoor
   enemies are the only path. */

/* level_2_enemy_screen_* / level_4_enemy_screen_* (bank2.asm): each indoor room is a "cores to
   destroy" count, then enemy triples {pos, type(+pos-adjust flags), attrs},
   0xFF-terminated. pos = (hi nibble = Y, lo nibble = X) scaled x16; type byte
   bit7 -> +8 Y, bit6 -> +8 X (see the carry note at the decode below). Types:
   0x13 wall turret, 0x14 wall core,
   0x19 soldier generator, 0x1A roller generator, plus the room-05 boss set. */
static const uint8_t contra_l2_enemy_screen_00[] = {0x01u, 0x11u, 0x19u, 0x00u, 0x68u, 0x94u, 0x03u, 0xFFu};
static const uint8_t contra_l2_enemy_screen_01[] = {
    0x01u, 0x11u, 0x59u, 0x00u, 0x66u, 0xD4u, 0x03u, 0x69u, 0xD3u, 0x00u, 0xFFu};
static const uint8_t contra_l2_enemy_screen_02[] = {
    0x01u, 0x11u, 0x59u, 0x00u, 0x78u, 0x14u, 0x03u, 0x66u, 0xD3u, 0x00u, 0x69u, 0xD3u, 0x00u, 0xFFu};
static const uint8_t contra_l2_enemy_screen_03[] = {
    0x01u, 0x11u, 0x59u, 0x00u, 0x11u, 0x1Au, 0x00u, 0x58u, 0x93u, 0x00u, 0x68u, 0x94u, 0x03u, 0xFFu};
static const uint8_t contra_l2_enemy_screen_04[] = {
    0x01u, 0x11u, 0x59u, 0x00u, 0x68u, 0x94u, 0x0Bu, 0x66u, 0xD3u, 0x00u,
    0x69u, 0xD3u, 0x00u, 0x58u, 0x13u, 0x00u, 0xFFu};
static const uint8_t contra_l2_enemy_screen_05[] = {
    0x01u, 0x48u, 0x10u, 0x00u, 0x65u, 0x08u, 0x00u, 0x68u, 0x0Au, 0x01u, 0x6Bu, 0x08u, 0x00u,
    0x95u, 0x0Au, 0x00u, 0x98u, 0x0Au, 0x00u, 0x9Bu, 0x0Au, 0x00u, 0xFFu};
static const uint8_t *const contra_l2_enemy_screen_tbl[6] = {
    contra_l2_enemy_screen_00, contra_l2_enemy_screen_01, contra_l2_enemy_screen_02,
    contra_l2_enemy_screen_03, contra_l2_enemy_screen_04, contra_l2_enemy_screen_05};

/* level_4_enemy_screen_* (bank2:2530-2607): Base 2 uses the same indoor loader
   format as level 2 but has eight core rooms and a Gemini boss room. */
static const uint8_t contra_l4_enemy_screen_00[] = {
    0x01u, 0x11u, 0x19u, 0x01u, 0x68u, 0x94u, 0x04u, 0x66u, 0xD3u, 0x00u,
    0x69u, 0xD3u, 0x00u, 0xFFu};
static const uint8_t contra_l4_enemy_screen_01[] = {
    0x04u, 0x11u, 0x19u, 0x01u, 0x76u, 0x54u, 0x03u, 0x77u, 0x54u, 0x01u,
    0x78u, 0x54u, 0x01u, 0x79u, 0x54u, 0x03u, 0xFFu};
static const uint8_t contra_l4_enemy_screen_02[] = {
    0x02u, 0x11u, 0x19u, 0x01u, 0x67u, 0xD4u, 0x04u, 0x68u, 0xD4u, 0x04u,
    0x58u, 0x93u, 0x00u, 0xFFu};
static const uint8_t contra_l4_enemy_screen_03[] = {
    0x02u, 0x11u, 0x19u, 0x01u, 0x68u, 0x13u, 0x00u, 0x77u, 0x54u, 0x03u,
    0x78u, 0x54u, 0x03u, 0xFFu};
static const uint8_t contra_l4_enemy_screen_04[] = {
    0x02u, 0x11u, 0x19u, 0x01u, 0x66u, 0xD4u, 0x03u, 0x68u, 0x93u, 0x00u,
    0x69u, 0xD4u, 0x03u, 0xFFu};
static const uint8_t contra_l4_enemy_screen_05[] = {
    0x01u, 0x11u, 0x1Au, 0x01u, 0x11u, 0x19u, 0x01u, 0x68u, 0x94u, 0x03u, 0xFFu};
static const uint8_t contra_l4_enemy_screen_06[] = {
    0x01u, 0x11u, 0x19u, 0x01u, 0x58u, 0x94u, 0x03u, 0x68u, 0x93u, 0x00u, 0xFFu};
static const uint8_t contra_l4_enemy_screen_07[] = {
    0x01u, 0x11u, 0x19u, 0x01u, 0x58u, 0x13u, 0x00u, 0x66u, 0xD3u, 0x00u,
    0x68u, 0x94u, 0x0Bu, 0x69u, 0xD3u, 0x00u, 0xFFu};
static const uint8_t contra_l4_enemy_screen_08[] = {
    0x02u, 0x11u, 0x20u, 0x00u, 0x36u, 0x5Cu, 0x00u, 0x39u, 0x5Cu, 0x00u,
    0x58u, 0x08u, 0x00u, 0x85u, 0x0Au, 0x00u, 0x88u, 0x0Au, 0x01u,
    0x8Bu, 0x0Au, 0x00u, 0xFFu};
static const uint8_t *const contra_l4_enemy_screen_tbl[9] = {
    contra_l4_enemy_screen_00, contra_l4_enemy_screen_01, contra_l4_enemy_screen_02,
    contra_l4_enemy_screen_03, contra_l4_enemy_screen_04, contra_l4_enemy_screen_05,
    contra_l4_enemy_screen_06, contra_l4_enemy_screen_07, contra_l4_enemy_screen_08};

static bool contra_get_indoor_enemy_screen_count(const ContraCore *core, size_t *screen_count)
{
    switch (core->ram[CONTRA_RAM_CURRENT_LEVEL])
    {
        case 0x01u:
            *screen_count = sizeof(contra_l2_enemy_screen_tbl) / sizeof(contra_l2_enemy_screen_tbl[0]);
            return true;
        case 0x03u:
            *screen_count = sizeof(contra_l4_enemy_screen_tbl) / sizeof(contra_l4_enemy_screen_tbl[0]);
            return true;
        default:
            *screen_count = 0u;
            return false;
    }
}

/* load_enemy_indoor_level (bank2.asm): on room entry, clear all slots and load
   the whole room's enemy set at once (indoor levels are room-based, not
   scroll-triggered), setting WALL_CORE_REMAINING from the first byte. */
static void contra_rom_load_indoor_enemy_data(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    const uint8_t *data;
    size_t screen_count;
    size_t y;
    int slot;

    if (ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] != 0u)
    {
        return; /* already loaded this room */
    }
    ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 1u;
    ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] = 0u;
    ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] = 0u;
    core->l2_blowopen_quadrants = 0u;
    core->l2_destroyed_struct_count = 0u;
    ram[CONTRA_RAM_INDOOR_RED_SOLDIER_CREATED] = 0u;
    ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0u;
    for (slot = 0x0F; slot >= 0; --slot)
    {
        /* remove_all_enemies (bank7:8157): husk-keeping remove_enemy per slot */
        contra_rom_remove_enemy_offscreen(core, (uint8_t)slot);
    }
    if (!contra_get_indoor_enemy_screen_count(core, &screen_count) || (screen >= screen_count))
    {
        return;
    }
    data = (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u)
        ? contra_l4_enemy_screen_tbl[screen]
        : contra_l2_enemy_screen_tbl[screen];
    if (data[0] == 0xFFu)
    {
        return;
    }
    ram[CONTRA_RAM_WALL_CORE_REMAINING] = data[0];
    y = 1u;
    for (slot = 0x0F; slot >= 0; --slot)
    {
        const uint8_t pos = data[y];
        uint8_t type_byte;
        uint8_t sx;

        if (pos == 0xFFu)
        {
            break;
        }
        type_byte = data[y + 1u];
        sx = (uint8_t)slot;
        ram[CONTRA_RAM_ENEMY_TYPE + sx] = (uint8_t)(type_byte & 0x3Fu);
        contra_rom_initialize_enemy(core, sx);
        /* bank2:1662-1675: the ROM tests each pos-adjust flag with `asl $08`,
           which shifts that flag bit OUT into the carry; the adjust is then
           `adc #$07`, and because the branch is only NOT taken when the flag bit
           was 1, the carry is always 1 here -- so the effective add is $07 + 1 =
           $08, not $07. (mesen ground truth: pos $68 -> Y $60 + 8 = $68 = 104.) */
        ram[CONTRA_RAM_ENEMY_Y_POS + sx] =
            (uint8_t)((pos & 0xF0u) + (((type_byte & 0x80u) != 0u) ? 0x08u : 0x00u));
        ram[CONTRA_RAM_ENEMY_X_POS + sx] =
            (uint8_t)((uint8_t)(pos << 4u) + (((type_byte & 0x40u) != 0u) ? 0x08u : 0x00u));
        ram[CONTRA_RAM_ENEMY_ATTRIBUTES + sx] = data[y + 2u];
        y += 3u;
    }
}

/* --- exe_soldier_generation (bank2.asm:1697): the outdoor random soldier
   spawner. A per-level timer (adjusted by game completions and weapon
   strength) expires into a terrain search for a spawn point seeded by
   FRAME_COUNTER + RANDOM_NUM, then gates (top-25%, attack flag, per-screen
   behavior bytes with skip probabilities, player-edge protection) decide
   whether to spawn one attribute-randomized soldier -- or, 1/3 of the time
   while scrolling, a three-soldier wave with staggered spawn delays.
   The 6502 carry chains through the `adc RANDOM_NUM` sites are modeled
   explicitly; with the recorded-RNG injection the whole system replays
   frame-exactly. --- */

static const uint8_t contra_level_soldier_generation_timer[8] = {
    0x90u, 0x00u, 0xD8u, 0x00u, 0xD0u, 0xC8u, 0xC0u, 0x00u};

/* always either left edge (0x0A) or right edge (0xFA) */
static const uint8_t contra_gen_soldier_initial_x_pos[16] = {
    0xFAu, 0x0Au, 0xFAu, 0xFAu, 0x0Au, 0xFAu, 0x0Au, 0xFAu,
    0x0Au, 0x0Au, 0x0Au, 0xFAu, 0xFAu, 0x0Au, 0x0Au, 0xFAu};

/* bit 0 = direction, bit 1 = ledge handling, bit 2 = shoots, bit 3 = ? */
static const uint8_t contra_gen_soldier_initial_attr_tbl[28] = {
    0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x04u,
    0x00u, 0x00u, 0x04u, 0x04u,
    0x00u, 0x04u, 0x04u, 0x04u,
    0x04u, 0x04u, 0x04u, 0x04u,
    0x00u, 0x00u, 0x00u, 0x08u,
    0x00u, 0x00u, 0x04u, 0x08u};

/* per-screen soldier behavior bytes; 0xFF = no generation on that screen */
static const uint8_t contra_soldier_level_attributes_00[12] = {
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x40u, 0x40u, 0x80u, 0xFFu, 0xFFu};
static const uint8_t contra_soldier_level_attributes_01[9] = {
    0x00u, 0x00u, 0x00u, 0x01u, 0x00u, 0xFFu, 0x01u, 0xFFu, 0xFFu};
static const uint8_t contra_soldier_level_attributes_02[20] = {
    0x01u, 0x02u, 0x03u, 0x04u, 0x03u, 0x03u, 0x03u, 0x02u, 0xFFu, 0x04u,
    0x02u, 0x03u, 0xFFu, 0x02u, 0x03u, 0x04u, 0x02u, 0xFFu, 0xFFu, 0xFFu};
static const uint8_t contra_soldier_level_attributes_03[12] = {
    0x00u, 0x05u, 0x02u, 0xFFu, 0xFFu, 0x80u, 0x05u, 0x03u, 0x82u, 0xFFu, 0xFFu, 0xFFu};
static const uint8_t contra_soldier_level_attributes_04[15] = {
    0x80u, 0x05u, 0x06u, 0x80u, 0x80u, 0x05u, 0x07u, 0x80u, 0x80u, 0x04u, 0xFFu, 0x04u, 0x04u, 0xFFu, 0xFFu};

/* soldier_level_attributes_ptr_tbl (bank2:2190) order: 00,00,01,00,02,03,04,00 */
static const uint8_t *const contra_soldier_level_attributes_tbl[8] = {
    contra_soldier_level_attributes_00, contra_soldier_level_attributes_00,
    contra_soldier_level_attributes_01, contra_soldier_level_attributes_00,
    contra_soldier_level_attributes_02, contra_soldier_level_attributes_03,
    contra_soldier_level_attributes_04, contra_soldier_level_attributes_00};

/* find_next_enemy_slot_6_to_0 (bank7:9094): slots 0-6 are reserved for
   generated soldiers; scan 6 down to 0 for a slot with ENEMY_ROUTINE == 0. */
static int contra_rom_find_next_enemy_slot_6_to_0(const ContraCore *core)
{
    int slot;

    for (slot = 6; slot >= 0; --slot)
    {
        if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + (unsigned)slot] == 0u)
        {
            return slot;
        }
    }
    return -1;
}

/* get_bg_collision_far (bank7:6336): the collision code at (x,y), except a
   floor with a solid one collision row (16px) below reports solid. */
static uint8_t contra_rom_get_bg_collision_far(const ContraCore *core, uint8_t x, uint8_t y)
{
    const uint8_t code = contra_get_outdoor_bg_collision(core, x, y);

    if ((code == 0x01u) &&
        (contra_get_outdoor_bg_collision(core, x, (uint8_t)(y + 0x10u)) == 0x80u))
    {
        return 0x80u;
    }
    return code;
}

/* The looped 8-bit `sec/sbc` of adjust_generation_timer: subtract step `count`
   times, stopping (without the store) on borrow. Returns the exit carry. */
static bool contra_rom_soldier_timer_sub_loop(ContraCore *core, uint8_t step, uint8_t count)
{
    uint8_t *const ram = core->ram;

    while (count != 0u)
    {
        const uint8_t value = ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER];

        if (value < step)
        {
            return false; /* borrow -> bcc @exit with carry clear */
        }
        ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] = (uint8_t)(value - step);
        --count;
    }
    return true;
}

/* adjust_generation_timer (bank2:1737): TIMER = per-level base, minus
   0x28 * min(GAME_COMPLETION_COUNT, 3), minus 5 * PLAYER_WEAPON_STRENGTH.
   Returns the 6502 carry at exit -- gen_soldier_find_pos's `adc RANDOM_NUM`
   consumes it. When BOTH adjustments are skipped (first playthrough, weapon
   strength 0 -- i.e. after a death before any pickup) nothing in the routine
   touches the carry, so the exit carry is the CALLER'S: clear from
   soldier_generation_01's borrowing `sbc #$02`, set from the odd-frame
   scrolling path's `ror` (FC bit 0 = 1). A wrong carry flips the player pick
   and the spawn column. */
static bool contra_rom_adjust_soldier_generation_timer(ContraCore *core, bool carry_in)
{
    uint8_t *const ram = core->ram;
    const uint8_t completions = ram[CONTRA_RAM_GAME_COMPLETION_COUNT];
    bool carry = carry_in;

    ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] =
        contra_level_soldier_generation_timer[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u];
    if (completions != 0u)
    {
        carry = contra_rom_soldier_timer_sub_loop(
            core, 0x28u, (completions < 4u) ? completions : 3u);
    }
    if (ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] != 0u)
    {
        carry = contra_rom_soldier_timer_sub_loop(
            core, 0x05u, ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH]);
    }
    return carry;
}

/* get_x_pos_check_bg_collision's X selection (bank2:1888): nibble of
   FRAME_COUNTER + RANDOM_NUM (+ chained carry) picks an edge, except level 1
   forces the right edge until 0x1E soldiers have spawned on the screen. */
static uint8_t contra_rom_gen_soldier_pick_x(ContraCore *core, bool carry_in)
{
    uint8_t *const ram = core->ram;
    const unsigned sum = (unsigned)ram[CONTRA_RAM_FRAME_COUNTER] +
        (unsigned)ram[CONTRA_RAM_RANDOM_NUM] + (carry_in ? 1u : 0u);
    uint8_t x = contra_gen_soldier_initial_x_pos[sum & 0x0Fu];

    if ((ram[CONTRA_RAM_GAME_COMPLETION_COUNT] == 0u) &&
        (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u) &&
        (ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] < 0x1Eu))
    {
        x = 0xFCu;
    }
    return x;
}

/* check_gen_soldier_bg_collision (bank2:1915): a floor at the test point
   records the spawn position (one super-tile above the floor) and advances to
   soldier_generation_02 (once). */
static bool contra_rom_check_gen_soldier_bg_collision(ContraCore *core, uint8_t test_x, uint8_t test_y)
{
    uint8_t *const ram = core->ram;

    if (contra_get_outdoor_bg_collision(core, test_x, test_y) != 0x01u)
    {
        return false;
    }
    ram[CONTRA_RAM_SOLDIER_GENERATION_X_POS] = test_x;
    ram[CONTRA_RAM_SOLDIER_GENERATION_Y_POS] = (uint8_t)(test_y - 0x10u);
    if (ram[CONTRA_RAM_SOLDIER_GENERATION_ROUTINE] != 0x02u)
    {
        ram[CONTRA_RAM_SOLDIER_GENERATION_ROUTINE] =
            (uint8_t)(ram[CONTRA_RAM_SOLDIER_GENERATION_ROUTINE] + 1u);
    }
    return true;
}

/* gen_soldier_find_pos (bank2:1803): pick a player by (FRAME_COUNTER +
   RANDOM_NUM) parity, then search for a floor every 16px -- 1/4 of frames
   top-down, 1/4 bottom-up, 1/2 upward from the player's Y. The initial probe
   at the start position runs unconditionally (its result is ignored but its
   side effects are real -- faithful to the ROM). */
static void contra_rom_gen_soldier_find_pos(ContraCore *core, bool tick_carry)
{
    uint8_t *const ram = core->ram;
    const bool carry0 = contra_rom_adjust_soldier_generation_timer(core, tick_carry);
    const unsigned pick = (unsigned)ram[CONTRA_RAM_FRAME_COUNTER] +
        (unsigned)ram[CONTRA_RAM_RANDOM_NUM] + (carry0 ? 1u : 0u);
    const bool adc_carry = pick > 0xFFu;
    /* ROM quirk kept: when the picked player has no sprite (Y == 0), the code
       means to switch players but re-reads the same zero, so the start stays 0. */
    const uint8_t player_y = ram[CONTRA_RAM_SPRITE_Y_POS + (pick & 0x01u)];
    const uint8_t mode = (uint8_t)(ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u);
    const bool top_down = (mode == 0u);
    const uint8_t start_y = top_down ? 0x00u : ((mode == 1u) ? 0xF0u : player_y);
    const uint8_t test_x = contra_rom_gen_soldier_pick_x(core, adc_carry);
    uint8_t test_y = start_y;

    (void)contra_rom_check_gen_soldier_bg_collision(core, test_x, test_y);
    for (;;)
    {
        test_y = top_down ? (uint8_t)(test_y + 0x10u) : (uint8_t)(test_y - 0x10u);
        if (test_y == start_y)
        {
            return; /* wrapped around the screen without finding ground */
        }
        if (contra_rom_check_gen_soldier_bg_collision(core, test_x, test_y))
        {
            return;
        }
    }
}

/* soldier_generation_00 (bank2:1718): arm the timer (skipped on levels whose
   base timer is 0 -- the indoor levels and the alien lair). */
static void contra_rom_soldier_generation_00(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (contra_level_soldier_generation_timer[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u] == 0u)
    {
        return;
    }
    (void)contra_rom_adjust_soldier_generation_timer(core, true);
    ram[CONTRA_RAM_SOLDIER_GENERATION_ROUTINE] =
        (uint8_t)(ram[CONTRA_RAM_SOLDIER_GENERATION_ROUTINE] + 1u);
}

/* soldier_generation_01 (bank2:1786): tick the timer (-2/frame; only -1 on odd
   frames while the screen scrolls) and search for a spawn point on expiry. */
static void contra_rom_soldier_generation_01(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t timer = ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER];

    if (((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u) &&
        (ram[CONTRA_RAM_FRAME_SCROLL] != 0u))
    {
        ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] = (uint8_t)(timer - 1u);
        if (ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] != 0u)
        {
            return;
        }
        /* entry carry: the `ror` that routed here put FC bit 0 (= 1) in it */
        contra_rom_gen_soldier_find_pos(core, true);
        return;
    }
    ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] = (uint8_t)(timer - 2u);
    if (timer >= 2u)
    {
        return; /* no borrow */
    }
    /* entry carry: clear -- the expiring `sbc #$02` borrowed */
    contra_rom_gen_soldier_find_pos(core, false);
}

/* create_default_soldiers (bank2:2092): the 1/3-while-scrolling wave -- THREE
   soldiers in one frame at the found spawn point, with staggered spawn delays
   (ENEMY_ATTRIBUTES bits 4-5 = 2,1,0 -> soldier_initial_anim_delay_tbl). */
static void contra_rom_create_default_soldiers(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t dir = (ram[CONTRA_RAM_SOLDIER_GENERATION_X_POS] < 0x80u) ? 0x01u : 0x00u;
    int y;

    ram[0x06u] = 0x00u;
    ram[0x07u] = dir; /* the ROM keeps the direction in the $07 temp */
    for (y = 2; y >= 0; --y)
    {
        const int slot = contra_rom_find_next_enemy_slot_6_to_0(core);
        uint8_t s;
        uint8_t attr;

        ram[0x06u] = (uint8_t)(ram[0x06u] + 1u);
        if (slot < 0)
        {
            break;
        }
        s = (uint8_t)slot;
        ram[CONTRA_RAM_ENEMY_TYPE + s] = 0x05u;
        contra_rom_initialize_enemy(core, s);
        ram[CONTRA_RAM_ENEMY_Y_POS + s] = ram[CONTRA_RAM_SOLDIER_GENERATION_Y_POS];
        ram[CONTRA_RAM_ENEMY_X_POS + s] = ram[CONTRA_RAM_SOLDIER_GENERATION_X_POS];
        attr = (uint8_t)((uint8_t)((uint8_t)y << 4u) + (uint8_t)(ram[0x06u] & 0x02u));
        ram[CONTRA_RAM_ENEMY_ATTRIBUTES + s] = (uint8_t)(attr | dir);
    }
    ram[CONTRA_RAM_SOLDIER_GENERATION_ROUTINE] = 0x00u;
}

/* soldier_generation_02 (bank2:1965): the spawn gates and the spawn itself. */
static void contra_rom_soldier_generation_02(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];
    const uint8_t gen_x = ram[CONTRA_RAM_SOLDIER_GENERATION_X_POS];
    const uint8_t gen_y = ram[CONTRA_RAM_SOLDIER_GENERATION_Y_POS];
    const uint8_t screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    const bool right_side = (gen_x >= 0x80u);
    uint8_t attr_byte;
    uint8_t probe_x;
    int slot;

    /* never the top 25% of the screen (except the vertical waterfall), never
       the very bottom, and only while the enemies are allowed to attack */
    if (((level != 0x02u) && (gen_y < 0x40u)) ||
        (gen_y >= 0xE0u) ||
        (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u))
    {
        goto soldier_gen_exit;
    }
    /* snow field's final screens spawn from the right only */
    if ((level == 0x04u) && (screen >= 0x11u) && !right_side)
    {
        goto soldier_gen_exit;
    }
    if ((contra_rom_get_bg_collision_far(core, gen_x, gen_y) & 0x80u) != 0u)
    {
        goto soldier_gen_exit;
    }
    probe_x = (uint8_t)((right_side ? (uint8_t)(gen_x - 0x10u) : gen_x) + 0x08u);
    if ((contra_rom_get_bg_collision_far(core, probe_x, gen_y) & 0x80u) != 0u)
    {
        goto soldier_gen_exit;
    }
    slot = contra_rom_find_next_enemy_slot_6_to_0(core);
    if ((slot < 0) || (screen == 0u))
    {
        goto soldier_gen_exit; /* no slot, or the first screen never generates */
    }
    attr_byte = contra_soldier_level_attributes_tbl[level & 0x07u][screen - 1u];
    if (attr_byte == 0xFFu)
    {
        goto soldier_gen_exit;
    }
    if (((attr_byte & 0x80u) != 0u) && ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u))
    {
        goto soldier_gen_exit; /* bit 7: 50% skip */
    }
    if (((attr_byte & 0x40u) != 0u) && ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u) != 0u))
    {
        goto soldier_gen_exit; /* bit 6: 75% skip */
    }
    /* player_edge_check (bank2:2144): before 0x1E soldiers (first playthrough
       only), don't spawn next to a player hugging that screen edge */
    if ((ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] < 0x1Eu) &&
        (ram[CONTRA_RAM_GAME_COMPLETION_COUNT] == 0u))
    {
        int player;

        for (player = 1; player >= 0; --player)
        {
            if (ram[CONTRA_RAM_P1_GAME_OVER_STATUS + (unsigned)player] != 0u)
            {
                continue;
            }
            if (right_side
                    ? (ram[CONTRA_RAM_SPRITE_X_POS + (unsigned)player] >= 0xC0u)
                    : (ram[CONTRA_RAM_SPRITE_X_POS + (unsigned)player] < 0x40u))
            {
                goto soldier_gen_exit;
            }
        }
    }
    /* init_and_generate_soldier: while scrolling (except the waterfall),
       RANDOM_NUM & 3 == 0 -> the three-soldier wave */
    if ((level != 0x02u) && (ram[CONTRA_RAM_FRAME_SCROLL] != 0u) &&
        ((ram[CONTRA_RAM_RANDOM_NUM] & 0x03u) == 0u))
    {
        contra_rom_create_default_soldiers(core);
        return;
    }
    {
        const uint8_t s = (uint8_t)slot;
        const unsigned base = ((unsigned)(attr_byte & 0x3Fu)) << 2u;
        const unsigned idx = base + (ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u);
        uint8_t attrs = (idx < sizeof(contra_gen_soldier_initial_attr_tbl))
            ? contra_gen_soldier_initial_attr_tbl[idx]
            : 0x00u; /* attr code 7 (snow field) indexes past the ROM table */

        ram[CONTRA_RAM_ENEMY_TYPE + s] = 0x05u;
        contra_rom_initialize_enemy(core, s);
        /* the adc chain: (attr*4 + frame bits) cannot carry, so the
           RANDOM_NUM + FRAME_COUNTER add below has carry-in 0 */
        if ((((unsigned)ram[CONTRA_RAM_RANDOM_NUM] +
              (unsigned)ram[CONTRA_RAM_FRAME_COUNTER]) & 0x02u) != 0u)
        {
            attrs = (uint8_t)(attrs | 0x02u); /* random ledge-handling bit */
        }
        ram[CONTRA_RAM_ENEMY_ATTRIBUTES + s] = attrs;
        ram[CONTRA_RAM_ENEMY_Y_POS + s] = gen_y;
        ram[CONTRA_RAM_ENEMY_X_POS + s] = gen_x;
        if (!right_side)
        {
            ram[CONTRA_RAM_ENEMY_ATTRIBUTES + s] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + s] + 1u); /* run right */
        }
        ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] =
            (uint8_t)(ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] + 1u);
    }

soldier_gen_exit:
    ram[CONTRA_RAM_SOLDIER_GENERATION_ROUTINE] = 0x00u;
}

/* exe_soldier_generation (bank2:1697). */
static void contra_rom_exe_soldier_generation(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != ram[CONTRA_RAM_SOLDIER_GEN_SCREEN])
    {
        ram[CONTRA_RAM_SOLDIER_GEN_SCREEN] = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
        ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] = 0x00u;
    }
    switch (ram[CONTRA_RAM_SOLDIER_GENERATION_ROUTINE])
    {
        case 0x00u: contra_rom_soldier_generation_00(core); break;
        case 0x01u: contra_rom_soldier_generation_01(core); break;
        case 0x02u: contra_rom_soldier_generation_02(core); break;
        default: break;
    }
}
