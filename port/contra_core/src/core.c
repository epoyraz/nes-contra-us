/* Contra native port — core runtime.
 *
 * Faithful-port coverage by ROM bank: see docs/Native Port Coverage.md and run
 *   python3 tools/port_coverage.py
 * for a per-bank, line-weighted bar chart. Convention: when a function
 * faithfully ports an original routine, cite that routine's ASM line RANGE in a
 * comment as bank<N>:<from>-<to> (e.g. "bank7:7315-7352 exe_all_enemy_routine"),
 * so the coverage ledger credits the right number of assembly lines. */

/* Faithful-port ledger — engine routines translated from the named original
 * labels. The attract-demo frame harness validates this engine path frame-exact
 * through f1070 (boot, demo, level-1 game/level routines, scroll, player,
 * scoring, render). The invented level-1 enemy AI is intentionally NOT listed
 * here (it is not a faithful port). Counted by tools/port_coverage.py.
 *   bank2:772-776    set_player_sprite_and_attrs
 *   bank2:865-900    set_player_sprite
 *   bank2:1027-1040  set_player_horizontal_flip
 *   bank2:1040-1082  set_player_jump_sprite
 *   bank5:179-186    end_demo_level
 *   bank6:290-334    check_player_fire
 *   bank7:513-527    play_sound
 *   bank7:616-624    load_bank_3_handle_scroll
 *   bank7:630-635    load_bank_2_alternate_tile_loading
 *   bank7:635-639    load_level_graphics
 *   bank7:648-659    load_bank_2_set_players_paused_sprite_attr
 *   bank7:659-670    load_bank_6_write_text_palette_to_mem
 *   bank7:688-698    exe_game_routine
 *   bank7:715-739    game_routine_00
 *   bank7:749-771    game_routine_01
 *   bank7:809-825    game_routine_02
 *   bank7:825-858    game_routine_03
 *   bank7:858-862    game_routine_04
 *   bank7:875-879    inc_routine_index_set_timer
 *   bank7:879-883    increment_game_routine
 *   bank7:883-891    init_game_routine_flags
 *   bank7:920-929    reset_delay_timer
 *   bank7:929-941    konami_input_check
 *   bank7:1049-1067  dec_theme_delay_check_user_input
 *   bank7:1067-1075  player_mode_change
 *   bank7:1090-1109  load_intro_palette2_play_intro_sound
 *   bank7:1109-1118  dec_intro_theme_delay
 *   bank7:1118-1142  set_next_demo_level
 *   bank7:1142-1148  init_score_player_lives
 *   bank7:1164-1172  init_player_lives
 *   bank7:1197-1203  clear_memory_3
 *   bank7:1950-1960  calculate_score_digit
 *   bank7:2014-2030  load_intro_graphics
 *   bank7:2244-2248  zero_out_nametables
 *   bank7:2364-2385  horizontal_flip_graphic_byte
 *   bank7:2400-2457  draw_player_num_lives
 *   bank7:2464-2476  draw_stage_and_level_name
 *   bank7:2476-2554  draw_the_scores
 *   bank7:3019-3024  game_routine_05
 *   bank7:3024-3029  run_level_routine
 *   bank7:3044-3082  level_routine_00
 *   bank7:3082-3093  load_level_header
 *   bank7:3099-3114  level_routine_01
 *   bank7:3122-3144  level_routine_02
 *   bank7:3164-3173  level_routine_03
 *   bank7:3173-3184  level_routine_04
 *   bank7:3209-3215  init_game_over
 *   bank7:3215-3221  set_to_level_routine_05
 *   bank7:3221-3227  set_graphics_zero_mode
 *   bank7:3230-3237  set_a_as_current_level_routine
 *   bank7:3237-3266  level_routine_05
 *   bank7:3278-3308  show_game_over_screen
 *   bank7:3308-3342  level_routine_06
 *   bank7:3348-3355  level_routine_07
 *   bank7:3355-3392  level_routine_08
 *   bank7:3396-3406  level_routine_09
 *   bank7:3406-3414  level_routine_0a
 *   bank7:3414-3443  check_for_pause
 *   bank7:3453-3479  load_alternate_graphics
 *   bank7:3479-3497  load_palettes_color_to_cpu
 *   bank7:3587-3621  load_palette_indexes
 *   bank7:3857-3877  set_frame_scroll_draw_player_bullets
 *   bank7:3877-3907  draw_player_bullet_sprites
 *   bank7:4053-4061  handle_invincibility_and_weapon_strength
 *   bank7:4110-4125  run_player_state_routine
 *   bank7:4233-4256  kill_player
 *   bank7:4516-4543  set_jump_status_and_y_velocity
 *   bank7:4543-4557  handle_d_pad
 *   bank7:4768-4783  get_x_velocity_d_pad_code
 *   bank7:4852-4865  init_player_data
 *   bank7:4939-4961  set_player_landing_y_offset
 *   bank7:4965-5012  set_player_aim_for_input
 *   bank7:5019-5042  check_player_ledge
 *   bank7:5057-5089  get_player_bg_collision_code
 *   bank7:5252-5272  can_player_drop_down
 *   bank7:5318-5366  init_player_attributes
 *   bank7:5452-5461  init_ppu_write_screen_supertiles
 *   bank7:6570-6577  load_next_supertiles_screen_indexes
 *   bank7:6586-6603  load_supertiles_screen_indexes */

/* Faithful-port ledger, backfill (2026-06-11, after the all-stages-green
 * milestone): ranges verified ported (in-body citations elsewhere name the
 * routine but not always the line range) or ROM-backed (the port reads the
 * original data bytes from the ROM image at runtime). Known NOT ported and
 * deliberately absent from this ledger: the bank1 APU sound engine (1-4058);
 * bank7 NMI/sound plumbing (232-441), the NMI graphics-buffer
 * drain (write_cpu_graphics_buffer_to_ppu, 2666-2785), and the
 * BG_COLLISION_DATA ring writer (set_supertile_bg_collisions, 8218-8317 --
 * modeled by the world-anchored collision-override list instead).
 *
 *   bank0:391-453    enemy_bullet_routine_01 + indoor_bullet_offscreen_check
 *   bank0:499-517    cannonball_explosion_sprite_tbl + weapon_box_routine_ptr_tbl
 *   bank0:715-719    flying_capsule_vel_tbl
 *   bank0:1063-1067  red_turret_supertile_1_tbl
 *   bank0:1202-1216  soldier_routine_ptr_tbl
 *   bank0:1232-1236  soldier_initial_anim_delay_tbl
 *   bank0:1267-1274  soldier_x_vel_tbl
 *   bank0:1443-1472  soldier_apply_vel_check_solid_collision
 *   bank0:1488-1506  get_soldier_num_bullets + soldier_num_bullets_tbl
 *   bank0:1580-1614  soldier_fired_all_bullets + bullet offset/type tbls
 *   bank0:1658-1688  init_soldier_hit_vel
 *   bank0:2034-2065  sniper_set_sprite
 *   bank0:2082-2129  bomb_turret_routine_ptr_tbl + routines 00-01
 *   bank0:2527-2545  indoor_enemy_gen_tbl + lvl_2_enemy_gen_tbl
 *   bank0:2572-2615  lvl_2/lvl_4 enemy gen screen tables
 *   bank0:2796-2801  boss_eye_routine_06
 *   bank0:2918-2925  grenade_routine_00
 *   bank0:3047-3052  wall_turret_routine_00
 *   bank0:3184-3193  wall_core_hp_tbl
 *   bank0:3385-3397  wall_core_wait_play_sound
 *   bank0:3419-3429  indoor_soldier_routine_ptr_tbl
 *   bank0:3535-3545  jumping_soldier_routine_ptr_tbl
 *   bank0:3650-3677  jumping_soldier_routine_04 + grenade_launcher_routine_ptr_tbl
 *   bank0:3799-3817  grenade_launcher_routine_06 + four_soldiers_routine_ptr_tbl
 *   bank0:3825-3847  four_soldiers_routine_01
 *   bank0:3908-3911  indoor_roller_gen_routine_00
 *   bank0:3976-4040  roller_gen_init_tbl + roller_gen_init_01
 *   bank0:4061-4103  indoor_close_segment_tbl + init_indoor_enemy_pos_and_vel
 *   bank0:4222-4225  grenade_vel_code_tbl
 *   bank0:4260-4279  indoor_bullet_velocity_tbl + init_enemy_set_type_and_pos
 *   bank0:4445-4448  falling_rock_set_sprite
 *   bank0:4500-4511  boss_mouth_routine_ptr_tbl
 *   bank0:4647-4649  mouth_projectile_type_angle
 *   bank0:4678-4681  boss_mouth_anim_delay_tbl
 *   bank0:4978-5022  dragon_arm_orb_attack_pat
 *   bank0:5096-5106  dragon_arm_orb_pat_3_or_4
 *   bank0:5423-5451  dragon_arm_orb_routine_04 + boss_gemini_routine_ptr_tbl
 *   bank0:5678-5689  boss_gemini_routine_06
 *   bank0:5788-5820  spinning_bullet_vel_tbl
 *   bank0:5825-6173  L4 boss blue/red soldiers + red_blue_soldier_gen (full block)
 *   bank0:7183-7236  fire_beam_add_pos_set_delay + begin_fire_beam_attack
 *   bank0:7246-7388  fire beam down/left/right routines 01-03
 *   bank0:7463-7479  animate_small_flame
 *   bank0:10412-10466 boss_heart_routine_06 + alien_guardian_routine_0b
 *   bank0:10490-10510 set_nametable_x_pos/pos_for_alien_guardian
 *   bank1:4059-4217  load_sprite_to_cpu_mem (multi-tile sprite decode -> OAM builder)
 *   bank1:4218-4301  draw_hud_sprites + hud_sprites + OAMDMA addr helpers
 *   bank1:4302-7196  sprite_ptr_tbl_0/1 + all sprite definitions (ROM-backed)
 *   bank2:48-758     level super-tile screen data + graphic/alt-graphic refs (ROM-backed)
 *   bank2:759-1388   player sprite/state code (pause, death, water, indoor, boss)
 *   bank2:1389-1516  level headers (ROM-backed)
 *   bank2:1517-1696  enemy screen-data loaders (outdoor + indoor)
 *   bank2:1697-2245  soldier generation system + attribute tables
 *   bank2:2246-3073  enemy screen data, all levels (ROM-backed)
 *   bank3:50-1227    super-tile / nametable-update / palette / tile-animation data (ROM-backed)
 *   bank3:1228-1590  end-level sequence + per-level end-of-level routines (all 8 levels)
 *   bank4:40-117     graphic data refs (ROM-backed; intro + ending pattern data)
 *   bank4:617-806    ending credits text data (ROM-backed, read by the credits crawl)
 *   bank5:29-263     graphic data refs + demo input system + demo input tables
 *   bank6:33-289     graphic data refs + short text tables + intro/transition palettes (ROM-backed)
 *   bank6:290-1918   player weapon + bullet system (continuously verified via the pbul digest)
 *   bank7:442-461    clear_memory
 *   bank7:548-566    load_bank_3_update_nametable_supertile
 *   bank7:640-642    load_A_offset_graphic_data
 *   bank7:671-676    game_routine_06
 *   bank7:899-920    decrement_delay_timer
 *   bank7:970-1049   ensure_input_valid + set_player_input + read_controller_button
 *   bank7:1204-1229  clear_memory_starting_at_x
 *   bank7:1240-1252  add_player_low_score
 *   bank7:1389-1602  write_update_supertile_to_cpu + update_supertile_palette
 *   bank7:1614-1633  nametable_update_data_ptr_tbl
 *   bank7:1789-1897  palette_mask_tbl + set_ppu_addresses_in_mem
 *   bank7:1898-1935  set_graphics_buffer_header + run_routine_from_tbl_below
 *   bank7:1968-1986  shift_and_check_digit_carry
 *   bank7:2000-2022  advance_graphic_read_addr
 *   bank7:2048-2087  load_level_graphic_data + level_graphic_data_tbl
 *   bank7:2114-2140  level_2_boss_graphic_data + ending_graphic_data
 *   bank7:2141-2252  graphic_data_ptr_tbl
 *   bank7:2253-2343  zero_out_nametables + write_graphic_data_to_ppu + sequence writer
 *   bank7:2580-2642  write_text_palette_to_mem
 *   bank7:2786-2817  write_palette_colors_to_ppu
 *   bank7:2822-2881  alternate_tile_loading + set_alt_graphics_cpu_buffer
 *   bank7:2928-3000  animate_indoor_fence
 *   bank7:3030-3039  game_routine_05 + run_level_routine
 *   bank7:3539-3594  write_palette_color_a_to_cpu_mem + shift_bg_palette_color
 *   bank7:3703-3720  lvl_alt_collision_and_palette_tbl
 *   bank7:3726-3869  game_palettes
 *   bank7:3920-4050  frame scroll/weapon strength + player_state_routine_03 + scroll_player
 *   bank7:4066-4073  handle_invincibility_and_weapon_strength
 *   bank7:4147-4151  level_spawn_position_index
 *   bank7:4231-4245  player_state_routine_01
 *   bank7:4278-4348  auto_scroll_player + player_state_routine_02
 *   bank7:4651-4668  set_player_x_vel_to_a
 *   bank7:4681-4707  set_x_velocity_for_edge_fall_code
 *   bank7:4785-4799  get_x_velocity_d_pad_code
 *   bank7:4818-4835  indoor_transition_set_pos
 *   bank7:4869-4881  init_player_data
 *   bank7:5114-5149  player_jumping_set_y_pos + apply_gravity
 *   bank7:5276-5295  can_player_drop_down
 *   bank7:5390-5475  check_player_solid_bg_collision
 *   bank7:5476-5501  init_ppu_write_screen_supertiles + config_horizontal_scrolling
 *   bank7:5525-5587  init_lvl_nametable_animation
 *   bank7:5894-5901  level_2_4_boss_graphics_data
 *   bank7:5902-6012  load_column_of_tiles_to_cpu_buffer + load_level_supertile_data
 *   bank7:6013-6221  set_vert_lvl_super_tiles + col/row attribute writers
 *   bank7:6316-6399  vertical-level collision + floor finders
 *   bank7:6501-6540  read_bg_collision_byte
 *   bank7:6583-6586  auto_scroll_timer_tbl
 *   bank7:6610-6657  load_supertiles_screen_indexes (+ starting_at_y)
 *   bank7:6947-6953  remove_current_enemy
 *   bank7:7042-7052  set_bullet_routine_to_2
 *   bank7:7241-7250  collision_box_codes_tbl
 *   bank7:7442-7530  enemy_routine per-level dispatch tables
 *   bank7:7574-7581  enemy_routine_init_explosion
 *   bank7:7621-7662  advance_enemy_routine + shared explosion routines
 *   bank7:7712-7779  remove_enemy + set_sprite_0 + explosion type tables
 *   bank7:7796-7800  set_enemy_y_vel_rem_off_screen
 *   bank7:7826-7836  remove_enemy_far
 *   bank7:7893-7907  set_enemy_(y_)velocity_to_0
 *   bank7:7931-7970  update_enemy_y/x_pos (+scroll)
 *   bank7:8065-8095  score_codes_tbl + enemy_destroyed_routine_ptr_tbl
 *   bank7:8182-8217  remove_all_enemies + clear/set_supertile_bg_collision
 *   bank7:8322-8346  create_explosion_89 + create_enemy_for_explosion
 *   bank7:8393-8413  set_delay_remove_enemy + disable_enemy_collision
 *   bank7:8430-8432  enable_enemy_collision
 *   bank7:8454-8492  add_a_to_enemy_x_pos + add_with_enemy_pos
 *   bank7:8560-8598  enemy Y adders + update_nametable_tiles_set_delay
 *   bank7:8615-8623  draw_enemy_supertile_a
 *   bank7:8755-8784  bg-collision probe adders
 *   bank7:8816-8832  set_flying_capsule_x_vel
 *   bank7:8880-8945  red_turret_find_target_player + player_enemy_x_dist
 *   bank7:9027-9033  far_segment_code_tbl
 *   bank7:9081-9104  weapon_item_indoor_vel_tbl + find_next_enemy_slot
 *   bank7:9114-9173  find_bullet_slot + clear_sprite/enemy helpers
 *   bank7:9400-9477  animate_wall_cannon + wall_cannon_routine_02/03/04
 *   bank7:9488-9522  wall_plating_routine_00/01/03
 *   bank7:9615-9623  scuba_soldier_routine_00
 *   bank7:9831-9858  aim_and_create_enemy_bullet
 *   bank7:9913-9975  create_enemy_bullet_if_attack_enabled
 *   bank7:9992-10057 bullet_gen_exit + calc_bullet_velocities
 *   bank7:10067-10085 bullet_fract_vel_tbl
 *   bank7:10205-10230 aim_var_1 quadrant helpers
 *   bank7:10275-10330 get_rotate_00/01 + get_rotate_dir_for_index
 *   bank7:10640-10685 quadrant_aim_dir_00/01/02 */
#include "contra/core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contra/buttons.h"

/* The native core is kept as one translation unit while the implementation
 * is organized by subsystem and ROM/level domain. This preserves the many
 * file-local helpers/static tables from the porting phase while making the
 * level implementations navigable. */
#include "engine/tables_and_forwards.inc.c"
#include "engine/graphics_ppu.inc.c"
#include "engine/player_weapons.inc.c"
#include "engine/player_movement.inc.c"
#include "engine/game_flow.inc.c"
#include "enemies/common_helpers.inc.c"
#include "levels/level2_4_indoor_bases.inc.c"
#include "levels/level1_jungle.inc.c"
#include "levels/level3_waterfall.inc.c"
#include "levels/level5_snowfield.inc.c"
#include "levels/level6_energy_zone.inc.c"
#include "levels/level7_hangar_and_level8_alien_lair.inc.c"
#include "enemies/dispatch_collision_generation.inc.c"
#include "engine/level_loop.inc.c"
#include "engine/render_overlays.inc.c"
#include "engine/public_api.inc.c"
