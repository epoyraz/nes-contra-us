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
#include "contra/core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contra/buttons.h"

/* Faithful real-RAM enemy system toggle (see the block before
   contra_run_level_enemy_logic). Defined here too so render code earlier in the
   file can gate on it. Default 1: the faithful real-RAM enemy system is the active
   path; build with -DCONTRA_USE_ROM_ENEMY_SYSTEM=0 for the old invented layer. */
#ifndef CONTRA_USE_ROM_ENEMY_SYSTEM
#define CONTRA_USE_ROM_ENEMY_SYSTEM 1
#endif

static void contra_render_level_1_nametable_update_supertile(
    ContraCore *core, int enemy_x, int enemy_y, uint8_t supertile_index);
static void contra_rom_update_enemy_pos(ContraCore *core, uint8_t x);

/* Faithful enemy types that render as background super-tiles (nametable writes)
   rather than OAM sprites: pill box, rotating gun, red turret, bomb turret,
   plated door, bridge. */
static bool contra_rom_enemy_type_is_supertile(uint8_t type)
{
    switch (type)
    {
        case 0x02u:
        case 0x04u:
        case 0x07u:
        case 0x10u:
        case 0x11u:
        case 0x12u:
            return true;
        default:
            return false;
    }
}

static const uint8_t contra_player_select_cursor_pos[2] = {0xA2u, 0xB2u};
static const uint8_t contra_player_mode_1d_table[2] = {0x01u, 0x07u};
static const uint8_t contra_p2_game_over_status_table[2] = {0x01u, 0x00u};
static const uint8_t contra_konami_code_lookup_table[10] = {
    0x08u, 0x08u, 0x04u, 0x04u, 0x02u, 0x01u, 0x02u, 0x01u, 0x40u, 0x80u
};
static const uint8_t contra_level_spawn_position_index[8] = {0x00u, 0x01u, 0x02u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u};
static const uint8_t contra_vertical_spawn_position[8] = {0x20u, 0x60u, 0x50u, 0x60u, 0x20u, 0x60u, 0x50u, 0x60u};
static const uint8_t contra_horizontal_spawn_position[8] = {0x30u, 0x70u, 0x30u, 0x70u, 0x20u, 0x90u, 0x20u, 0x90u};
static const uint8_t contra_level_vert_scroll_and_song[8][2] = {
    {0xE0u, 0x2Au},
    {0xE8u, 0x3Eu},
    {0x00u, 0x2Eu},
    {0xE8u, 0x3Eu},
    {0xE0u, 0x32u},
    {0xE0u, 0x36u},
    {0xE0u, 0x2Au},
    {0xE0u, 0x3Au}
};
static const uint8_t contra_level_headers[8][0x20] = {
    {
        0x00u, 0x00u, 0x01u, 0x80u, 0x01u, 0x80u, 0x71u, 0x86u,
        0x0Bu, 0x06u, 0xF9u, 0xFFu, 0x05u, 0x08u, 0x05u, 0x08u,
        0x02u, 0x03u, 0x04u, 0x05u, 0x00u, 0x01u, 0x22u, 0x07u,
        0x0Bu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    },
    {
        0x01u, 0x00u, 0x06u, 0x82u, 0x18u, 0x87u, 0x78u, 0x8Eu,
        0x04u, 0x00u, 0xFFu, 0xFFu, 0x24u, 0x28u, 0x29u, 0x28u,
        0x09u, 0x0Au, 0x04u, 0x24u, 0x00u, 0x01u, 0x22u, 0x2Au,
        0x05u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    },
    {
        0x00u, 0x01u, 0xCEu, 0x84u, 0xF8u, 0x8Eu, 0x18u, 0x96u,
        0x07u, 0x07u, 0xFFu, 0xFFu, 0x0Du, 0x0Eu, 0x0Fu, 0x00u,
        0x0Bu, 0x0Cu, 0x04u, 0x0Du, 0x00u, 0x01u, 0x22u, 0x07u,
        0x07u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    },
    {
        0x01u, 0x00u, 0x36u, 0x82u, 0x18u, 0x87u, 0x78u, 0x8Eu,
        0x07u, 0x00u, 0xFFu, 0xFFu, 0x2Eu, 0x2Fu, 0x30u, 0x2Fu,
        0x2Cu, 0x2Du, 0x04u, 0x2Eu, 0x00u, 0x01u, 0x22u, 0x2Au,
        0x08u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    },
    {
        0x00u, 0x00u, 0x8Fu, 0x86u, 0x98u, 0x96u, 0x68u, 0x9Du,
        0x13u, 0x20u, 0xF0u, 0xF0u, 0x62u, 0x63u, 0x62u, 0x63u,
        0x3Du, 0x3Eu, 0x04u, 0x62u, 0x00u, 0x01u, 0x22u, 0x07u,
        0x13u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    },
    {
        0x00u, 0x00u, 0x25u, 0x89u, 0x1Eu, 0x9Eu, 0xFEu, 0xA4u,
        0x0Bu, 0x0Cu, 0xDEu, 0xDEu, 0x35u, 0x36u, 0x37u, 0x38u,
        0x33u, 0x34u, 0x04u, 0x35u, 0x00u, 0x01u, 0x22u, 0x07u,
        0x0Bu, 0x81u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    },
    {
        0x00u, 0x00u, 0x81u, 0x8Bu, 0xAAu, 0xA5u, 0x4Au, 0xADu,
        0x0Du, 0x0Eu, 0xF1u, 0xF1u, 0x47u, 0x57u, 0x47u, 0x58u,
        0x45u, 0x46u, 0x04u, 0x47u, 0x00u, 0x01u, 0x22u, 0x07u,
        0x0Du, 0x81u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    },
    {
        0x00u, 0x00u, 0x47u, 0x8Eu, 0xCAu, 0xADu, 0xFAu, 0xB4u,
        0x09u, 0x05u, 0xEFu, 0xEFu, 0x4Cu, 0x4Du, 0x4Eu, 0x4Fu,
        0x48u, 0x49u, 0x4Au, 0x4Cu, 0x00u, 0x01u, 0x43u, 0x44u,
        0x09u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u
    }
};
static const uint8_t contra_level_graphic_data_lists[13][10] = {
    {0x03u, 0x13u, 0x19u, 0x1Au, 0x14u, 0x16u, 0x05u, 0xFFu, 0xFFu, 0xFFu},
    {0x03u, 0x04u, 0x06u, 0x0Au, 0x0Fu, 0x10u, 0x11u, 0xFFu, 0xFFu, 0xFFu},
    {0x03u, 0x13u, 0x19u, 0x1Au, 0x14u, 0x16u, 0x07u, 0xFFu, 0xFFu, 0xFFu},
    {0x03u, 0x04u, 0x06u, 0x0Au, 0x0Fu, 0x10u, 0x11u, 0x12u, 0xFFu, 0xFFu},
    {0x03u, 0x13u, 0x19u, 0x1Au, 0x15u, 0x16u, 0x0Bu, 0xFFu, 0xFFu, 0xFFu},
    {0x03u, 0x13u, 0x19u, 0x1Au, 0x15u, 0x16u, 0x0Cu, 0xFFu, 0xFFu, 0xFFu},
    {0x03u, 0x13u, 0x19u, 0x1Au, 0x15u, 0x16u, 0x0Du, 0xFFu, 0xFFu, 0xFFu},
    {0x03u, 0x13u, 0x19u, 0x0Eu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu},
    {0x03u, 0x04u, 0x13u, 0x08u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu},
    {0x03u, 0x04u, 0x13u, 0x08u, 0x09u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu},
    {0x01u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu},
    {0x01u, 0x02u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu},
    {0x01u, 0x03u, 0x17u, 0x18u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu}
};

typedef struct ContraGraphicDataRef
{
    uint16_t cpu_addr;
    uint8_t bank_code;
} ContraGraphicDataRef;

static const ContraGraphicDataRef contra_graphic_data_ptrs[27] = {
    {0xCB36u, 0x00u}, {0xAA2Du, 0x04u}, {0x9097u, 0x02u}, {0x8001u, 0x04u}, {0x85AEu, 0x04u},
    {0x8001u, 0x05u}, {0x99FCu, 0x04u}, {0x8A61u, 0x05u}, {0x886Cu, 0x04u}, {0x99CDu, 0x04u},
    {0xA005u, 0x04u}, {0x93E0u, 0x05u}, {0x8001u, 0x06u}, {0x8CDCu, 0x06u}, {0x9BD6u, 0x06u},
    {0xA346u, 0x04u}, {0xA003u, 0x84u}, {0xA3E7u, 0x04u}, {0xA940u, 0x04u}, {0x87A1u, 0x04u},
    {0xA814u, 0x05u}, {0xB07Au, 0x06u}, {0xB15Cu, 0x06u}, {0xADDFu, 0x05u}, {0xB30Du, 0x05u},
    {0xA31Bu, 0x05u}, {0xA500u, 0x05u}
};

static const uint32_t contra_nes_palette_rgba[64] = {
    0x000B4800u, 0x00002A88u, 0x001412A7u, 0x003B00A4u, 0x005C007Eu, 0x006E0040u, 0x006C0600u, 0x00561D00u,
    0x00333500u, 0x000B4800u, 0x00000000u, 0x00004F08u, 0x0000404Du, 0x00000000u, 0x00000000u, 0x00000000u,
    0x00ADADADu, 0x00155FD9u, 0x004240FFu, 0x007527FEu, 0x00A01ACCu, 0x00B71E7Bu, 0x00B53120u, 0x00994E00u,
    0x006B6D00u, 0x00388700u, 0x000C9300u, 0x00008F32u, 0x00007C8Du, 0x00000000u, 0x00000000u, 0x00000000u,
    0x00FFFEFFu, 0x0064B0FFu, 0x009290FFu, 0x00C676FFu, 0x00F36AFFu, 0x00FE6ECCu, 0x00FE8170u, 0x00EA9E22u,
    0x00BCBE00u, 0x0088D800u, 0x005CE430u, 0x0045E082u, 0x0048CDDEu, 0x004F4F4Fu, 0x00000000u, 0x00000000u,
    0x00FFFEFFu, 0x00C0DFFFu, 0x00D3D2FFu, 0x00E8C8FFu, 0x00FBC2FFu, 0x00FEC4EAu, 0x00FECCC5u, 0x00F7D8A5u,
    0x00E4E594u, 0x00CFEF96u, 0x00BDF4ABu, 0x00B3F3CCu, 0x00B5EBF2u, 0x00B8B8B8u, 0x00000000u, 0x00000000u
};
static const uint8_t contra_d_pad_player_aim_tbl[64] = {
    0x02u, 0x02u, 0x07u, 0x02u, 0x04u, 0x03u, 0x06u, 0x02u, 0x00u, 0x01u, 0x08u, 0x02u, 0x07u, 0x02u, 0x07u, 0x02u,
    0x07u, 0x02u, 0x07u, 0x02u, 0x05u, 0x03u, 0x06u, 0x02u, 0x09u, 0x01u, 0x08u, 0x02u, 0x07u, 0x02u, 0x07u, 0x02u,
    0x00u, 0x02u, 0x07u, 0x00u, 0x00u, 0x02u, 0x07u, 0x02u, 0x00u, 0x01u, 0x08u, 0x02u, 0x07u, 0x02u, 0x07u, 0x02u,
    0x00u, 0x02u, 0x07u, 0x00u, 0x0Au, 0x03u, 0x06u, 0x02u, 0x00u, 0x01u, 0x08u, 0x02u, 0x07u, 0x02u, 0x07u, 0x02u
};
static const uint8_t contra_sprite_attr_start_tbl[2] = {0x00u, 0x05u};
static const uint8_t contra_player_effect_xor_tbl[2] = {0x00u, 0xFFu};
static const uint8_t contra_weapon_strength_tbl[5] = {0x00u, 0x02u, 0x01u, 0x03u, 0x02u};
static const uint8_t contra_weapon_bullet_sprite_code_tbl[5] = {0x1Eu, 0x1Fu, 0x22u, 0x1Fu, 0x00u};
static const uint8_t contra_weapon_item_sprite_code_tbl[7] = {0x33u, 0x34u, 0x31u, 0x2Fu, 0x32u, 0x30u, 0x4Eu};
static const uint8_t contra_player_small_seq_sprite_tbl[3] = {0x0Fu, 0x16u, 0x17u};
static const uint8_t contra_player_frame_sprite_type_tbl[10] = {0x00u, 0x02u, 0x00u, 0x03u, 0x00u, 0x00u, 0x03u, 0x00u, 0x02u, 0x00u};
static const uint8_t contra_player_frame_sprite_tbl_00[6] = {0x02u, 0x03u, 0x04u, 0x05u, 0x03u, 0x06u};
static const uint8_t contra_player_frame_sprite_tbl_01[6] = {0x0Du, 0x0Eu, 0x0Fu, 0x0Du, 0x0Eu, 0x0Fu};
static const uint8_t contra_player_frame_sprite_tbl_02[6] = {0x10u, 0x11u, 0x12u, 0x10u, 0x11u, 0x12u};
static const uint8_t contra_player_frame_sprite_tbl_03[6] = {0x13u, 0x14u, 0x15u, 0x13u, 0x14u, 0x15u};
static const uint8_t contra_player_curled_sprite_code_tbl[4] = {0x08u, 0x09u, 0x08u, 0x09u};
static const uint8_t contra_player_death_sprite_tbl[5][2] = {
    {0x0Au, 0x00u}, {0x0Bu, 0x00u}, {0x0Au, 0xC0u}, {0x0Bu, 0xC0u}, {0x0Cu, 0x00u}
};
static const uint8_t contra_player_dead_sequence_tbl[3] = {0x04u, 0x04u, 0x06u};
static const int8_t contra_player_died_x_velocity_tbl[3] = {-1, 0, 0};
static const uint8_t contra_player_water_sprite_tbl[10][2] = {
    {0x19u, 0x00u}, {0x19u, 0x00u}, {0x19u, 0x00u}, {0x18u, 0x00u}, {0x18u, 0x00u},
    {0x18u, 0x40u}, {0x18u, 0x40u}, {0x19u, 0x40u}, {0x19u, 0x40u}, {0x19u, 0x40u}
};
static const uint8_t contra_player_water_firing_sprite_tbl[10][2] = {
    {0x1Bu, 0x00u}, {0x1Cu, 0x00u}, {0x1Du, 0x00u}, {0x18u, 0x00u}, {0x18u, 0x00u},
    {0x18u, 0x40u}, {0x18u, 0x40u}, {0x1Du, 0x40u}, {0x1Cu, 0x40u}, {0x1Bu, 0x40u}
};
static const uint8_t contra_level_2_enemy_gen_screen_00[6] = {
    0x42u, 0x30u, 0x01u, 0x01u, 0x00u, 0xC0u
};
static const uint8_t contra_level_2_enemy_gen_screen_01[12] = {
    0x46u, 0x30u, 0x81u, 0x50u, 0x01u, 0x10u, 0x00u, 0x30u, 0x00u, 0x10u, 0x01u, 0xE0u
};
static const uint8_t contra_level_2_enemy_gen_screen_02[4] = {
    0x00u, 0x30u, 0xC5u, 0xA0u
};
static const uint8_t contra_level_2_enemy_gen_screen_03[6] = {
    0x46u, 0x20u, 0x81u, 0x60u, 0xC3u, 0xE1u
};
static const uint8_t contra_level_2_enemy_gen_screen_04[16] = {
    0x40u, 0x30u, 0x81u, 0x60u, 0x00u, 0x10u, 0x03u, 0x30u,
    0x02u, 0x10u, 0x01u, 0x40u, 0x47u, 0x10u, 0x4Au, 0xE0u
};

typedef struct ContraLevel2GeneratorScript
{
    const uint8_t *data;
    uint8_t length;
} ContraLevel2GeneratorScript;

static const ContraLevel2GeneratorScript contra_level_2_enemy_gen_scripts[5] = {
    {contra_level_2_enemy_gen_screen_00, (uint8_t)sizeof(contra_level_2_enemy_gen_screen_00)},
    {contra_level_2_enemy_gen_screen_01, (uint8_t)sizeof(contra_level_2_enemy_gen_screen_01)},
    {contra_level_2_enemy_gen_screen_02, (uint8_t)sizeof(contra_level_2_enemy_gen_screen_02)},
    {contra_level_2_enemy_gen_screen_03, (uint8_t)sizeof(contra_level_2_enemy_gen_screen_03)},
    {contra_level_2_enemy_gen_screen_04, (uint8_t)sizeof(contra_level_2_enemy_gen_screen_04)}
};

static void contra_set_jump_status_and_y_velocity(ContraCore *core, uint8_t player_index);
static const int8_t contra_bullet_initial_pos_ground[12][2] = {
    {  5, -27}, { 13, -16}, { 16,  -5}, { 13,   6},
    { 16,   9}, {-16,   9}, {-13,   6}, {-16,  -5},
    {-13, -16}, { -5, -27}, { -5, -27}, {  0,  16}
};
static const int8_t contra_bullet_initial_pos_jump[12][2] = {
    {  0, -16}, { 15, -15}, { 16,   0}, { 15,  15},
    {  0,  16}, {  0,  16}, {-15,  15}, {-16,   0},
    {-15, -15}, {  0, -16}, {  0,  16}, {  0,  16}
};
static const int8_t contra_bullet_velocity_fast[12][2] = {
    {  0, -3}, {  2, -3}, {  3,  0}, {  2,  2},
    {  3,  0}, { -3,  0}, { -3,  2}, { -3,  0},
    { -3, -3}, {  0, -3}, {  0, -3}, {  0,  3}
};
static const uint8_t contra_bullet_velocity_fract[12][2] = {
    {0x00u, 0x00u}, {0x1Fu, 0xE1u}, {0x00u, 0x00u}, {0x1Fu, 0x1Fu},
    {0x00u, 0x00u}, {0x00u, 0x00u}, {0xE1u, 0x1Fu}, {0x00u, 0x00u},
    {0xE1u, 0xE1u}, {0x00u, 0x00u}, {0x00u, 0x00u}, {0x00u, 0x00u}
};
static const int8_t contra_bullet_velocity_fast_rapid[12][2] = {
    {  0, -4}, {  2, -3}, {  4,  0}, {  2,  2},
    {  4,  0}, { -4,  0}, { -3,  2}, { -4,  0},
    { -3, -3}, {  0, -4}, {  0, -4}, {  0,  4}
};
static const uint8_t contra_bullet_velocity_fract_rapid[12][2] = {
    {0x00u, 0x00u}, {0xD4u, 0x2Cu}, {0x00u, 0x00u}, {0xD4u, 0xD4u},
    {0x00u, 0x00u}, {0x00u, 0x00u}, {0x2Cu, 0xD4u}, {0x00u, 0x00u},
    {0x2Cu, 0x2Cu}, {0x00u, 0x00u}, {0x00u, 0x00u}, {0x00u, 0x00u}
};
static const int8_t contra_f_bullet_velocity_fast[12][2] = {
    {  0, -2}, {  1, -2}, {  1,  0}, {  1,  1},
    {  1,  0}, { -2,  0}, { -2,  1}, { -2,  0},
    { -2, -2}, {  0, -2}, {  0, -2}, {  0,  1}
};
static const uint8_t contra_f_bullet_velocity_fract[12][2] = {
    {0x00u, 0x80u}, {0x0Fu, 0xF1u}, {0x80u, 0x00u}, {0x0Fu, 0x0Fu},
    {0x80u, 0x00u}, {0x80u, 0x00u}, {0xF1u, 0x0Fu}, {0x80u, 0x00u},
    {0xF1u, 0xF1u}, {0x00u, 0x80u}, {0x00u, 0x80u}, {0x00u, 0x80u}
};
static const int8_t contra_f_bullet_velocity_fast_rapid[12][2] = {
    {  0, -2}, {  1, -2}, {  2,  0}, {  1,  1},
    {  2,  0}, { -2,  0}, { -2,  1}, { -2,  0},
    { -2, -2}, {  0, -2}, {  0, -2}, {  0,  2}
};
static const uint8_t contra_f_bullet_velocity_fract_rapid[12][2] = {
    {0x00u, 0x00u}, {0x6Au, 0x96u}, {0x00u, 0x00u}, {0x6Au, 0x6Au},
    {0x00u, 0x00u}, {0x00u, 0x00u}, {0x96u, 0x6Au}, {0x00u, 0x00u},
    {0x96u, 0x96u}, {0x00u, 0x00u}, {0x00u, 0x00u}, {0x00u, 0x00u}
};
static const uint8_t contra_f_bullet_initial_timer_tbl[12] = {
    0x0Cu, 0x0Eu, 0x00u, 0x02u, 0x00u, 0x08u, 0x06u, 0x08u, 0x0Au, 0x0Cu, 0x0Cu, 0x04u
};
static const int8_t contra_f_bullet_center_offset_tbl[12][2] = {
    {  0, -16}, { 11, -11}, { 16,   0}, { 11,  11},
    { 16,   0}, {-16,   0}, {-11,  11}, {-16,   0},
    {-11, -11}, {  0, -16}, {  0, -16}, {  0,  16}
};
static const int8_t contra_f_bullet_outdoor_y_swirl_amt_tbl[16] = {
      0,  -6, -11, -14, -15, -14, -11,  -6,
      0,   6,  11,  14,  15,  14,  11,   6
};
static const int8_t contra_f_bullet_outdoor_x_swirl_amt_tbl[16] = {
    -15, -14, -11,  -6,   0,   6,  11,  14,
     15,  14,  11,   6,   0,  -6, -11, -14
};
static const uint8_t contra_s_bullet_player_aim_dir_ptr_tbl[12] = {
    0x00u, 0x04u, 0x08u, 0x0Cu, 0x08u, 0x18u, 0x14u, 0x18u, 0x1Cu, 0x00u, 0x00u, 0x10u
};
static const int8_t contra_s_bullet_num_index_modifier_tbl[5] = {0, 1, -1, 2, -2};
static const uint8_t contra_s_bullet_y_velocity_normal[32][2] = {
    {0x03u, 0xFDu}, {0x0Fu, 0xFDu}, {0x3Cu, 0xFDu}, {0x84u, 0xFDu},
    {0xE1u, 0xFDu}, {0x56u, 0xFEu}, {0xDDu, 0xFEu}, {0x6Du, 0xFFu},
    {0x00u, 0x00u}, {0x93u, 0x00u}, {0x23u, 0x01u}, {0xAAu, 0x01u},
    {0x1Fu, 0x02u}, {0x7Cu, 0x02u}, {0xC4u, 0x02u}, {0xF1u, 0x02u},
    {0xFDu, 0x02u}, {0xF1u, 0x02u}, {0xC4u, 0x02u}, {0x7Cu, 0x02u},
    {0x1Fu, 0x02u}, {0xAAu, 0x01u}, {0x23u, 0x01u}, {0x93u, 0x00u},
    {0x00u, 0x00u}, {0x6Du, 0xFFu}, {0xDDu, 0xFEu}, {0x56u, 0xFEu},
    {0xE1u, 0xFDu}, {0x84u, 0xFDu}, {0x3Cu, 0xFDu}, {0x0Fu, 0xFDu}
};
static const uint8_t contra_s_bullet_x_velocity_normal[32][2] = {
    {0x00u, 0x00u}, {0x93u, 0x00u}, {0x23u, 0x01u}, {0xAAu, 0x01u},
    {0x1Fu, 0x02u}, {0x7Cu, 0x02u}, {0xC4u, 0x02u}, {0xF1u, 0x02u},
    {0xFDu, 0x02u}, {0xF1u, 0x02u}, {0xC4u, 0x02u}, {0x7Cu, 0x02u},
    {0x1Fu, 0x02u}, {0xAAu, 0x01u}, {0x23u, 0x01u}, {0x93u, 0x00u},
    {0x00u, 0x00u}, {0x6Du, 0xFFu}, {0xDDu, 0xFEu}, {0x56u, 0xFEu},
    {0xE1u, 0xFDu}, {0x84u, 0xFDu}, {0x3Cu, 0xFDu}, {0x0Fu, 0xFDu},
    {0x03u, 0xFDu}, {0x0Fu, 0xFDu}, {0x3Cu, 0xFDu}, {0x84u, 0xFDu},
    {0xE1u, 0xFDu}, {0x56u, 0xFEu}, {0xDDu, 0xFEu}, {0x6Du, 0xFFu}
};
static const uint8_t contra_s_bullet_y_velocity_rapid[32][2] = {
    {0x84u, 0xFCu}, {0x92u, 0xFCu}, {0xC6u, 0xFCu}, {0x1Au, 0xFDu},
    {0x87u, 0xFDu}, {0x0Fu, 0xFEu}, {0xADu, 0xFEu}, {0x55u, 0xFFu},
    {0x00u, 0x00u}, {0xABu, 0x00u}, {0x53u, 0x01u}, {0xF1u, 0x01u},
    {0x79u, 0x02u}, {0xE6u, 0x02u}, {0x3Au, 0x03u}, {0x6Eu, 0x03u},
    {0x7Cu, 0x03u}, {0x6Eu, 0x03u}, {0x3Au, 0x03u}, {0xE6u, 0x02u},
    {0x79u, 0x02u}, {0xF1u, 0x01u}, {0x53u, 0x01u}, {0xABu, 0x00u},
    {0x00u, 0x00u}, {0x55u, 0xFFu}, {0xADu, 0xFEu}, {0x0Fu, 0xFEu},
    {0x87u, 0xFDu}, {0x1Au, 0xFDu}, {0xC6u, 0xFCu}, {0x92u, 0xFCu}
};
static const uint8_t contra_s_bullet_x_velocity_rapid[32][2] = {
    {0x00u, 0x00u}, {0xABu, 0x00u}, {0x53u, 0x01u}, {0xF1u, 0x01u},
    {0x79u, 0x02u}, {0xE6u, 0x02u}, {0x3Au, 0x03u}, {0x6Eu, 0x03u},
    {0x7Cu, 0x03u}, {0x6Eu, 0x03u}, {0x3Au, 0x03u}, {0xE6u, 0x02u},
    {0x79u, 0x02u}, {0xF1u, 0x01u}, {0x53u, 0x01u}, {0xABu, 0x00u},
    {0x00u, 0x00u}, {0x55u, 0xFFu}, {0xADu, 0xFEu}, {0x0Fu, 0xFEu},
    {0x87u, 0xFDu}, {0x1Au, 0xFDu}, {0xC6u, 0xFCu}, {0x92u, 0xFCu},
    {0x84u, 0xFCu}, {0x92u, 0xFCu}, {0xC6u, 0xFCu}, {0x1Au, 0xFDu},
    {0x87u, 0xFDu}, {0x0Fu, 0xFEu}, {0xADu, 0xFEu}, {0x55u, 0xFFu}
};
static const uint8_t contra_laser_bullet_delay_tbl[4] = {0x0Au, 0x07u, 0x04u, 0x01u};
static const uint8_t contra_laser_bullet_sprite_tbl[12][2] = {
    {0x23u, 0x00u}, {0x25u, 0x80u}, {0x24u, 0x00u}, {0x25u, 0x00u},
    {0x24u, 0x00u}, {0x24u, 0x40u}, {0x25u, 0x40u}, {0x24u, 0x40u},
    {0x25u, 0xC0u}, {0x23u, 0x00u}, {0x23u, 0x80u}, {0x23u, 0x80u}
};

enum
{
    CONTRA_PLAYER_BULLET_COUNT = 16u,
    CONTRA_STANDARD_BULLET_LIMIT = 4u,
    CONTRA_MACHINE_GUN_BULLET_LIMIT = 6u,
    CONTRA_SPRAY_GUN_BULLET_LIMIT = 10u,
    CONTRA_LASER_BULLET_COUNT = 4u
};

enum
{
    CONTRA_NATIVE_LEVEL1_STATE_WAIT = 0u,
    CONTRA_NATIVE_LEVEL1_STATE_EMERGE = 1u,
    CONTRA_NATIVE_LEVEL1_STATE_ACTIVE = 2u,
    CONTRA_NATIVE_LEVEL1_STATE_RETREAT = 3u
};

enum
{
    CONTRA_NATIVE_LEVEL1_TYPE_EXPLOSION = 0xFEu,
    CONTRA_NATIVE_LEVEL1_EXPLOSION_RING = 0u,
    CONTRA_NATIVE_LEVEL1_EXPLOSION_CLOUD = 1u,
    CONTRA_NATIVE_LEVEL1_EXPLOSION_FRAME_DELAY = 0x0Au
};

static const uint8_t contra_level_palette_animation_count[9] = {
    0x04u, 0x04u, 0x03u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u
};
static const uint8_t contra_level_palette_2_index_tbl[4] = {0x04u, 0x5Cu, 0x04u, 0x5Du};
static const uint8_t contra_level_alt_collision_and_palette_tbl[8][15] = {
    {0x06u, 0xA8u, 0xA8u, 0x23u, 0x23u, 0x23u, 0x23u, 0x02u, 0x03u, 0x04u, 0x23u, 0x00u, 0x01u, 0x22u, 0x07u},
    {0x00u, 0xFFu, 0xFFu, 0x16u, 0x17u, 0x18u, 0x17u, 0x11u, 0x12u, 0x13u, 0x16u, 0x00u, 0x01u, 0x22u, 0x21u},
    {0x07u, 0xFFu, 0xFFu, 0x27u, 0x54u, 0x55u, 0x54u, 0x0Bu, 0x25u, 0x26u, 0x27u, 0x00u, 0x01u, 0x22u, 0x07u},
    {0x00u, 0xFFu, 0xFFu, 0x1Eu, 0x1Fu, 0x20u, 0x1Fu, 0x19u, 0x1Au, 0x1Cu, 0x1Eu, 0x00u, 0x01u, 0x22u, 0x2Bu},
    {0x20u, 0xF0u, 0xF0u, 0x42u, 0x42u, 0x42u, 0x42u, 0x3Du, 0x3Eu, 0x40u, 0x42u, 0x00u, 0x01u, 0x22u, 0x06u},
    {0x0Cu, 0xDEu, 0xDEu, 0x3Au, 0x3Bu, 0x3Au, 0x3Cu, 0x39u, 0x39u, 0x04u, 0x3Au, 0x00u, 0x01u, 0x22u, 0x56u},
    {0x0Eu, 0xF1u, 0xF1u, 0x5Au, 0x5Fu, 0x5Au, 0x5Bu, 0x45u, 0x46u, 0x59u, 0x5Fu, 0x00u, 0x01u, 0x22u, 0x07u},
    {0x05u, 0xB6u, 0xB6u, 0x4Bu, 0x50u, 0x4Bu, 0x50u, 0x48u, 0x49u, 0x4Au, 0x4Bu, 0x00u, 0x01u, 0x43u, 0x44u}
};
static const uint8_t contra_level_1_soldier_walk_sprites[6] = {0x3Bu, 0x3Cu, 0x3Du, 0x3Fu, 0x3Cu, 0x3Eu};
static const uint8_t contra_level_1_bridge_destroyed_supertile_tbl[8] = {
    0x00u, 0x1Au, 0x1Bu, 0x1Cu, 0x19u, 0x1Cu, 0x19u, 0x1Du
};
static const int8_t contra_level_1_bridge_cloud_x_offset[4] = {0, -16, 0, 16};
static const int8_t contra_level_1_bridge_cloud_y_offset[4] = {0, 0, -16, 0};
static const uint8_t contra_level_1_bridge_cloud_sprite_tbl[4] = {0x37u, 0x35u, 0x36u, 0x37u};
static const uint8_t contra_level_1_explosion_ring_sprite_tbl[3] = {0x38u, 0x39u, 0x3Au};
static const uint8_t contra_level_1_explosion_cloud_sprite_tbl[4] = {0x37u, 0x35u, 0x36u, 0x37u};
static const uint8_t contra_level_soldier_generation_timer[8] = {0x90u, 0x00u, 0xD8u, 0x00u, 0xD0u, 0xC8u, 0xC0u, 0x00u};
static const uint8_t contra_level_end_level_delay_timer_tbl[8] = {0xA0u, 0xA0u, 0xE0u, 0xA0u, 0xA0u, 0xA0u, 0xA0u, 0xA0u};
static const uint8_t contra_level_1_soldier_generation_screen_attrs[12] = {
    0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x40u, 0x40u, 0x80u, 0xFFu, 0xFFu
};
static const uint8_t contra_gen_soldier_initial_x_pos[16] = {
    0xFAu, 0x0Au, 0xFAu, 0xFAu, 0x0Au, 0xFAu, 0x0Au, 0xFAu,
    0x0Au, 0x0Au, 0x0Au, 0xFAu, 0xFAu, 0x0Au, 0x0Au, 0xFAu
};
static const uint8_t contra_gen_soldier_initial_attr_tbl[28] = {
    0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x04u,
    0x00u, 0x00u, 0x04u, 0x04u,
    0x00u, 0x04u, 0x04u, 0x04u,
    0x04u, 0x04u, 0x04u, 0x04u,
    0x00u, 0x00u, 0x00u, 0x08u,
    0x00u, 0x00u, 0x04u, 0x08u
};
static const uint16_t contra_level_1_enemy_screen_ptr_tbl_addr = 0xB82Bu;
static const uint16_t contra_level_2_enemy_screen_ptr_tbl_addr = 0xB8AAu;
static const uint16_t contra_level_1_nametable_update_supertile_data_addr = 0x83B1u;
static const uint16_t contra_level_1_nametable_update_palette_data_addr = 0x86ACu;
static const uint16_t contra_level_2_nametable_update_supertile_data_addr = 0x88A8u;
static const uint16_t contra_level_2_nametable_update_palette_data_addr = 0x8E91u;
static const uint16_t contra_demo_input_pointer_table_addr = 0xB3D2u;
static const uint8_t contra_level_2_wall_core_update_supertile_tbl[4] = {0x02u, 0x03u, 0x01u, 0x00u};
static const uint8_t contra_level_2_wall_core_update_x_tbl[4] = {0x70u, 0x90u, 0x90u, 0x70u};
static const uint8_t contra_level_2_wall_core_update_y_tbl[4] = {0x78u, 0x78u, 0x58u, 0x58u};

typedef struct ContraAltGraphicDataRef
{
    uint16_t ppu_addr;
    uint16_t cpu_addr;
    uint8_t chunk_count;
} ContraAltGraphicDataRef;

static const ContraAltGraphicDataRef contra_alt_graphic_data_refs[8] = {
    {0x1A80u, 0x9252u, 0x2Cu},
    {0x1000u, 0x9252u, 0x00u},
    {0x1460u, 0x97D2u, 0x5Du},
    {0x1000u, 0x9252u, 0x00u},
    {0x16A0u, 0xA372u, 0x1Du},
    {0x0A80u, 0xA712u, 0x22u},
    {0x1000u, 0x9252u, 0x00u},
    {0x1B60u, 0xAB52u, 0x25u}
};

enum
{
    CONTRA_RAM_ALT_GFX_PPU_ADDR_LO = 0x6Cu,
    CONTRA_RAM_ALT_GFX_PPU_ADDR_HI = 0x6Du,
    CONTRA_RAM_ALT_GFX_READ_ADDR_LO = 0x6Eu,
    CONTRA_RAM_ALT_GFX_READ_ADDR_HI = 0x6Fu,
    CONTRA_RAM_ALT_GFX_CHUNK_COUNT = 0x70u
};

typedef struct ContraRomImage
{
    uint8_t *bytes;
    size_t size;
    bool attempted;
    bool ready;
} ContraRomImage;

static ContraRomImage contra_rom_image = {NULL, 0u, false, false};
static const uint16_t contra_sprite_ptr_tbl_0_addr = 0xB030u;
static const uint16_t contra_sprite_ptr_tbl_1_addr = 0xB12Eu;
static const uint16_t contra_short_text_pointer_table_addr = 0xB262u;

static void contra_clear_memory_3(ContraCore *core);
static void contra_load_palettes_color_to_cpu(ContraCore *core, uint8_t num_colors);
static void contra_load_alternate_graphics(ContraCore *core);
static void contra_load_bank_6_write_text_palette_to_mem(ContraCore *core, uint8_t text_code);
static void contra_init_apu_channels(ContraCore *core);
static void contra_run_level_routine(ContraCore *core);
static void contra_finish_level_graphics_load(ContraCore *core);
static void contra_write_level_1_nametable_update_supertile_to_ppu(
    ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint8_t supertile_index
);
static void contra_calculate_level_1_nametable_update_supertile_ppu_addr(
    const ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint16_t *tile_ppu_addr,
    uint16_t *attr_ppu_addr
);
static void contra_write_level_1_nametable_update_supertile_to_ppu_addr(
    ContraCore *core,
    uint16_t tile_ppu_addr,
    uint16_t attr_ppu_addr,
    uint8_t supertile_index
);

static bool contra_load_rom_image(void)
{
    static const char *const candidate_paths[] = {
        "baserom.nes",
        "../baserom.nes",
        "../../baserom.nes",
        "../../../baserom.nes"
    };
    size_t path_index;

    if (contra_rom_image.attempted)
    {
        return contra_rom_image.ready;
    }

    contra_rom_image.attempted = true;

    for (path_index = 0u; path_index < (sizeof(candidate_paths) / sizeof(candidate_paths[0])); ++path_index)
    {
        const char *const path = candidate_paths[path_index];
        FILE *file = fopen(path, "rb");

        if (file == NULL)
        {
            continue;
        }

        if (fseek(file, 0, SEEK_END) != 0)
        {
            fclose(file);
            continue;
        }

        const long size_long = ftell(file);
        uint8_t *bytes;
        size_t size;

        if (size_long <= 0)
        {
            fclose(file);
            continue;
        }

        size = (size_t)size_long;
        if (fseek(file, 0, SEEK_SET) != 0)
        {
            fclose(file);
            continue;
        }

        bytes = (uint8_t *)malloc(size);
        if (bytes == NULL)
        {
            fclose(file);
            continue;
        }

        if (fread(bytes, 1u, size, file) != size)
        {
            free(bytes);
            fclose(file);
            continue;
        }

        fclose(file);

        if ((size < 16u) ||
            (bytes[0] != 'N') ||
            (bytes[1] != 'E') ||
            (bytes[2] != 'S') ||
            (bytes[3] != 0x1Au))
        {
            free(bytes);
            continue;
        }

        contra_rom_image.bytes = bytes;
        contra_rom_image.size = size;
        contra_rom_image.ready = true;
        return true;
    }

    return false;
}

static size_t contra_prg_rom_offset(uint8_t bank, uint16_t cpu_addr)
{
    const size_t header_size = 16u;
    const size_t bank_size = 0x4000u;
    const size_t cpu_offset = (bank == 7u)
        ? (size_t)(cpu_addr - 0xC000u)
        : (size_t)(cpu_addr - 0x8000u);

    return header_size + ((size_t)bank * bank_size) + cpu_offset;
}

static uint8_t contra_rom_read_u8(uint8_t bank, uint16_t cpu_addr)
{
    const size_t offset = contra_prg_rom_offset(bank, cpu_addr);

    if ((!contra_load_rom_image()) || (offset >= contra_rom_image.size))
    {
        return 0u;
    }

    return contra_rom_image.bytes[offset];
}

static uint16_t contra_rom_read_u16(uint8_t bank, uint16_t cpu_addr)
{
    const uint8_t low = contra_rom_read_u8(bank, cpu_addr);
    const uint8_t high = contra_rom_read_u8(bank, (uint16_t)(cpu_addr + 1u));

    return (uint16_t)((uint16_t)low | ((uint16_t)high << 8u));
}

static uint8_t contra_horizontal_flip_graphic_byte(uint8_t value)
{
    uint8_t flipped = 0u;
    unsigned bit_index;

    for (bit_index = 0u; bit_index < 8u; ++bit_index)
    {
        flipped = (uint8_t)((flipped << 1u) | (value & 0x01u));
        value >>= 1u;
    }

    return flipped;
}

static void contra_write_ppu_byte(ContraCore *core, uint16_t ppu_addr, uint8_t value)
{
    if (ppu_addr < CONTRA_PPU_PATTERN_TABLE_SIZE)
    {
        core->ppu_pattern[ppu_addr] = value;
        return;
    }

    if ((ppu_addr >= 0x2000u) && (ppu_addr < 0x3F00u))
    {
        core->ppu_nametable[(ppu_addr - 0x2000u) & (CONTRA_PPU_NAMETABLE_SIZE - 1u)] = value;
        return;
    }

    if ((ppu_addr >= 0x3F00u) && (ppu_addr < 0x3F20u))
    {
        core->ppu_palette[(ppu_addr - 0x3F00u) & (CONTRA_PPU_PALETTE_SIZE - 1u)] = value;
    }
}

static void contra_write_cpu_graphics_buffer_byte(ContraCore *core, uint8_t value)
{
    const uint8_t offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];

    if (offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
    {
        return;
    }

    core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + offset] = value;
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = (uint8_t)(offset + 1u);
}

static void contra_flush_cpu_graphics_buffer_to_ppu(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    size_t read_offset = 0u;

    if (ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] != 0u)
    {
        return;
    }

    while (read_offset < CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
    {
        const uint8_t increment_mode = ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset++];
        uint16_t ppu_addr;

        if (increment_mode == 0u)
        {
            break;
        }

        if ((read_offset + 1u) >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
        {
            break;
        }

        ppu_addr = (uint16_t)(
            ((uint16_t)ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset] << 8u) |
            (uint16_t)ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset + 1u]
        );
        read_offset += 2u;

        if (increment_mode == 0x03u)
        {
            uint8_t count;

            if (read_offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
            {
                break;
            }

            count = ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset++];
            while ((count-- != 0u) && (read_offset < CONTRA_CPU_GRAPHICS_BUFFER_SIZE))
            {
                contra_write_ppu_byte(core, ppu_addr++, ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset++]);
            }

            continue;
        }

        for (;;)
        {
            const uint16_t increment = (increment_mode == 0x02u) ? 0x20u : 0x01u;
            uint8_t value;

            if (read_offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
            {
                break;
            }

            value = ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset];

            if (value == 0xFFu)
            {
                if ((read_offset + 1u) >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
                {
                    ++read_offset;
                    break;
                }

                if (ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset + 1u] < 0x04u)
                {
                    ++read_offset;
                    break;
                }
            }

            contra_write_ppu_byte(core, ppu_addr, value);
            ppu_addr = (uint16_t)(ppu_addr + increment);
            ++read_offset;

            if (read_offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
            {
                break;
            }
        }
    }

    memset(&ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER], 0, CONTRA_CPU_GRAPHICS_BUFFER_SIZE);
    ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = 0x00u;
}

static void contra_apply_graphic_data_to_ppu(ContraCore *core, uint8_t graphic_index)
{
    const ContraGraphicDataRef ref = contra_graphic_data_ptrs[graphic_index];
    const bool flip_horizontal = (ref.bank_code & 0x80u) != 0u;
    const uint8_t bank = (uint8_t)((ref.bank_code & 0x07u) == 0u ? 7u : (ref.bank_code & 0x07u));
    uint16_t read_addr = ref.cpu_addr;
    uint16_t ppu_addr;

    if (!contra_load_rom_image())
    {
        return;
    }

    ppu_addr = contra_rom_read_u16(bank, read_addr);
    read_addr = (uint16_t)(read_addr + (flip_horizontal ? 4u : 2u));

    for (;;)
    {
        const uint8_t command = contra_rom_read_u8(bank, read_addr++);
        unsigned index;

        if (command == 0xFFu)
        {
            return;
        }

        if (command == 0x7Fu)
        {
            ppu_addr = contra_rom_read_u16(bank, read_addr);
            read_addr = (uint16_t)(read_addr + (flip_horizontal ? 4u : 2u));
            continue;
        }

        if ((command & 0x80u) == 0u)
        {
            uint8_t value = contra_rom_read_u8(bank, read_addr++);

            if (flip_horizontal)
            {
                value = contra_horizontal_flip_graphic_byte(value);
            }

            for (index = 0u; index < command; ++index)
            {
                contra_write_ppu_byte(core, ppu_addr++, value);
            }

            continue;
        }

        for (index = 0u; index < (unsigned)(command & 0x7Fu); ++index)
        {
            uint8_t value = contra_rom_read_u8(bank, read_addr++);

            if (flip_horizontal)
            {
                value = contra_horizontal_flip_graphic_byte(value);
            }

            contra_write_ppu_byte(core, ppu_addr++, value);
        }
    }
}

static void contra_load_graphic_data_list(ContraCore *core, uint8_t list_index)
{
    const uint8_t *const graphic_list = contra_level_graphic_data_lists[list_index];
    unsigned entry_index;

    if (!contra_load_rom_image())
    {
        return;
    }

    memset(core->ppu_pattern, 0, sizeof(core->ppu_pattern));

    for (entry_index = 0u; entry_index < 10u; ++entry_index)
    {
        const uint8_t graphic_index = graphic_list[entry_index];

        if (graphic_index == 0xFFu)
        {
            return;
        }

        contra_apply_graphic_data_to_ppu(core, graphic_index);
    }
}

static uint8_t contra_level_screen_supertile_count(const ContraCore *core)
{
    return (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ? 0x40u : 0x38u;
}

static void contra_decode_level_screen_supertiles(
    ContraCore *core,
    uint8_t screen_number,
    uint8_t *dest,
    uint8_t start_offset
)
{
    const uint16_t ptr_table_addr = (uint16_t)(
        (uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_SUPERTILES_PTR] |
        ((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_SUPERTILES_PTR + 1u] << 8u)
    );
    const uint16_t screen_data_addr = contra_rom_read_u16(2u, (uint16_t)(ptr_table_addr + ((uint16_t)screen_number * 2u)));
    const uint8_t target_end = (uint8_t)(start_offset + contra_level_screen_supertile_count(core));
    uint16_t read_addr = screen_data_addr;
    uint8_t write_offset = start_offset;

    if (!contra_load_rom_image())
    {
        return;
    }

    while (write_offset < target_end)
    {
        uint8_t value = contra_rom_read_u8(2u, read_addr++);
        uint8_t copy_index;

        if (value < 0x80u)
        {
            dest[write_offset++] = value;
            continue;
        }

        if (value < 0xF0u)
        {
            uint8_t repeat_count = (uint8_t)(value & 0x7Fu);
            value = contra_rom_read_u8(2u, read_addr++);

            while ((repeat_count-- != 0u) && (write_offset < target_end))
            {
                dest[write_offset++] = value;
            }

            continue;
        }

        copy_index = (uint8_t)(((value & 0x0Fu) << 3u) | start_offset);
        while ((write_offset < target_end) && (copy_index < CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE))
        {
            dest[write_offset++] = dest[copy_index++];
            if ((uint8_t)(copy_index - (((value & 0x0Fu) << 3u) | start_offset)) >= 8u)
            {
                break;
            }
        }
    }
}

static void contra_load_supertiles_screen_indexes(ContraCore *core, uint8_t screen_number)
{
    contra_decode_level_screen_supertiles(
        core,
        screen_number,
        core->level_screen_supertiles,
        core->ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET]
    );
}

static void contra_write_horizontal_level_column_snapshot_to_ppu(
    ContraCore *core,
    uint16_t ppu_addr,
    uint8_t tile_offset,
    uint8_t supertile_nametable_offset
)
{
    const uint8_t *const ram = core->ram;
    const uint16_t supertile_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u)
    );
    const uint8_t tile_x = tile_offset & 0x1Fu;
    const uint8_t supertile_column = (uint8_t)(tile_x >> 2u);
    const uint8_t tile_x_in_supertile = (uint8_t)(tile_x & 0x03u);
    uint8_t tile_y;

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ||
        (!contra_load_rom_image()))
    {
        return;
    }

    for (tile_y = 0u; tile_y < 28u; ++tile_y)
    {
        const uint8_t supertile_row = (uint8_t)(tile_y >> 2u);
        const uint8_t supertile_offset = (uint8_t)(
            supertile_nametable_offset +
            (uint8_t)(supertile_row * 8u) +
            supertile_column
        );
        const uint8_t supertile_index = core->level_screen_supertiles[supertile_offset];
        const uint16_t supertile_data_addr = (uint16_t)(
            supertile_ptr + ((uint16_t)supertile_index * 16u)
        );
        const uint8_t tile_in_supertile = (uint8_t)(((tile_y & 0x03u) << 2u) | tile_x_in_supertile);
        const uint8_t pattern_index = contra_rom_read_u8(
            3u,
            (uint16_t)(supertile_data_addr + tile_in_supertile)
        );

        contra_write_ppu_byte(core, ppu_addr, pattern_index);
        ppu_addr = (uint16_t)(ppu_addr + 0x20u);
    }
}

static void contra_write_horizontal_level_column_to_ppu(ContraCore *core)
{
    const uint8_t *const ram = core->ram;
    const uint16_t ppu_addr = (uint16_t)(
        ((uint16_t)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] << 8u) |
        (uint16_t)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE]
    );

    contra_write_horizontal_level_column_snapshot_to_ppu(
        core,
        ppu_addr,
        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET],
        ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET]
    );
}

static void contra_write_horizontal_level_column_attributes_snapshot_to_ppu(
    ContraCore *core,
    uint8_t tile_offset,
    uint8_t supertile_nametable_offset,
    uint8_t attr_high
)
{
    const uint8_t *const ram = core->ram;
    const uint16_t palette_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] << 8u)
    );
    const uint8_t attr_col = (uint8_t)((tile_offset >> 2u) & 0x07u);
    uint8_t attr_low = (uint8_t)(0xC0u | attr_col);
    uint8_t attr_row;

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ||
        (!contra_load_rom_image()))
    {
        return;
    }

    for (attr_row = 0u; attr_row < 7u; ++attr_row)
    {
        const uint8_t supertile_offset = (uint8_t)(
            supertile_nametable_offset +
            (uint8_t)(attr_row * 8u) +
            attr_col
        );
        const uint8_t supertile_index = core->level_screen_supertiles[supertile_offset];
        const uint8_t attr = contra_rom_read_u8(3u, (uint16_t)(palette_ptr + supertile_index));
        const uint16_t ppu_addr = (uint16_t)(
            ((uint16_t)attr_high << 8u) |
            (uint16_t)attr_low
        );

        contra_write_ppu_byte(core, ppu_addr, attr);
        attr_low = (uint8_t)(attr_low + 0x08u);
    }
}

static void contra_write_horizontal_level_column_attributes_to_ppu(ContraCore *core)
{
    const uint8_t *const ram = core->ram;

    contra_write_horizontal_level_column_attributes_snapshot_to_ppu(
        core,
        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET],
        ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET],
        ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE]
    );
}

static void contra_schedule_horizontal_level_column_write(ContraCore *core)
{
    const uint8_t *const ram = core->ram;

    core->pending_horizontal_column_write = 0x01u;
    core->pending_horizontal_column_tile_offset = ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET];
    core->pending_horizontal_column_supertile_offset = ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET];
    core->pending_horizontal_column_ppu_addr = (uint16_t)(
        ((uint16_t)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] << 8u) |
        (uint16_t)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE]
    );
}

static void contra_schedule_horizontal_level_column_attributes_write(ContraCore *core)
{
    const uint8_t *const ram = core->ram;

    core->pending_horizontal_attr_write = 0x01u;
    core->pending_horizontal_attr_tile_offset = ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET];
    core->pending_horizontal_attr_supertile_offset = ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET];
    core->pending_horizontal_attr_high = ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE];
}

static void contra_flush_pending_horizontal_level_writes(ContraCore *core)
{
    if (core->pending_horizontal_column_write != 0u)
    {
        contra_write_horizontal_level_column_snapshot_to_ppu(
            core,
            core->pending_horizontal_column_ppu_addr,
            core->pending_horizontal_column_tile_offset,
            core->pending_horizontal_column_supertile_offset
        );
        core->pending_horizontal_column_write = 0x00u;
    }

    if (core->pending_horizontal_attr_write != 0u)
    {
        contra_write_horizontal_level_column_attributes_snapshot_to_ppu(
            core,
            core->pending_horizontal_attr_tile_offset,
            core->pending_horizontal_attr_supertile_offset,
            core->pending_horizontal_attr_high
        );
        core->pending_horizontal_attr_write = 0x00u;
    }
}

static void contra_advance_horizontal_level_ppu_column(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] =
        (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] + 1u);
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] =
        (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] + 1u);

    if (ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] < 0x20u)
    {
        return;
    }

    ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] ^= 0x40u;
    contra_load_supertiles_screen_indexes(core, (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 2u));
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] ^= 0x04u;
    ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] ^= 0x04u;
}

static uint8_t contra_read_nametable_byte(const ContraCore *core, uint8_t nametable_index, uint16_t offset)
{
    const size_t nametable_base = ((size_t)(nametable_index & 0x01u) * 0x400u) + (size_t)offset;

    return core->ppu_nametable[nametable_base & (CONTRA_PPU_NAMETABLE_SIZE - 1u)];
}

static uint8_t contra_read_pattern_color_index(
    const ContraCore *core,
    uint16_t pattern_addr,
    uint8_t pixel_x,
    uint8_t pixel_y
)
{
    const uint8_t plane0 = core->ppu_pattern[pattern_addr + pixel_y];
    const uint8_t plane1 = core->ppu_pattern[pattern_addr + pixel_y + 8u];
    const uint8_t shift = (uint8_t)(7u - pixel_x);

    return (uint8_t)(((plane0 >> shift) & 0x01u) | (((plane1 >> shift) & 0x01u) << 1u));
}

static uint8_t contra_read_nametable_palette_slot(
    const ContraCore *core,
    uint8_t nametable_index,
    uint8_t tile_x,
    uint8_t tile_y
)
{
    const uint16_t attr_offset = (uint16_t)(0x3C0u + (((uint16_t)tile_y >> 2u) * 8u) + ((uint16_t)tile_x >> 2u));
    const uint8_t attr = contra_read_nametable_byte(core, nametable_index, attr_offset);
    const uint8_t shift = (uint8_t)(((tile_y & 0x02u) << 1u) | (tile_x & 0x02u));

    return (uint8_t)((attr >> shift) & 0x03u);
}

static uint32_t contra_background_palette_color_rgba(const ContraCore *core, uint8_t palette_slot, uint8_t color_index)
{
    const size_t slot = (color_index == 0u)
        ? 0u
        : (((size_t)palette_slot * 4u) + (size_t)color_index);
    const uint8_t color_code = core->ppu_palette[slot % CONTRA_PPU_PALETTE_SIZE] & 0x3Fu;

    return contra_nes_palette_rgba[color_code];
}

static uint32_t contra_sprite_palette_color_rgba(const ContraCore *core, uint8_t palette_slot, uint8_t color_index)
{
    const size_t slot = 0x10u + ((size_t)palette_slot * 4u) + (size_t)color_index;
    const uint8_t color_code = core->ppu_palette[slot % CONTRA_PPU_PALETTE_SIZE] & 0x3Fu;

    return contra_nes_palette_rgba[color_code];
}

static void contra_draw_background_tile(
    ContraCore *core,
    int dest_x,
    int dest_y,
    uint8_t pattern_index,
    uint8_t palette_slot
)
{
    const uint16_t pattern_addr = (uint16_t)(0x1000u + ((uint16_t)pattern_index * 16u));
    unsigned pixel_y;

    if ((pattern_addr + 15u) >= sizeof(core->ppu_pattern))
    {
        return;
    }

    for (pixel_y = 0u; pixel_y < 8u; ++pixel_y)
    {
        const int framebuffer_y = dest_y + (int)pixel_y;
        unsigned pixel_x;

        if ((framebuffer_y < 0) || (framebuffer_y >= (int)CONTRA_FRAMEBUFFER_HEIGHT))
        {
            continue;
        }

        for (pixel_x = 0u; pixel_x < 8u; ++pixel_x)
        {
            const int framebuffer_x = dest_x + (int)pixel_x;
            const uint8_t color_index = contra_read_pattern_color_index(core, pattern_addr, (uint8_t)pixel_x, (uint8_t)pixel_y);
            size_t framebuffer_index;

            if ((framebuffer_x < 0) || (framebuffer_x >= (int)CONTRA_FRAMEBUFFER_WIDTH))
            {
                continue;
            }

            framebuffer_index = ((size_t)framebuffer_y * CONTRA_FRAMEBUFFER_WIDTH) + (size_t)framebuffer_x;
            core->framebuffer[framebuffer_index] =
                contra_background_palette_color_rgba(core, palette_slot, color_index);
            core->background_opaque[framebuffer_index] = (uint8_t)(color_index != 0u);
        }
    }
}

static void contra_draw_sprite_pair(
    ContraCore *core,
    uint8_t sprite_x,
    uint8_t sprite_y,
    uint8_t tile_index,
    uint8_t attr,
    uint8_t sprite_order
)
{
    const uint16_t pattern_base = (uint16_t)(((uint16_t)(tile_index & 0x01u) << 12u) + ((uint16_t)(tile_index & 0xFEu) * 16u));
    const uint8_t palette_slot = (uint8_t)(attr & 0x03u);
    const bool priority_behind_bg = (attr & 0x20u) != 0u;
    const bool flip_horizontal = (attr & 0x40u) != 0u;
    const bool flip_vertical = (attr & 0x80u) != 0u;
    unsigned pixel_y;

    if ((pattern_base + 31u) >= sizeof(core->ppu_pattern))
    {
        return;
    }

    for (pixel_y = 0u; pixel_y < 16u; ++pixel_y)
    {
        const unsigned source_row = flip_vertical ? (15u - pixel_y) : pixel_y;
        const uint16_t pattern_addr = (uint16_t)(pattern_base + (((uint16_t)(source_row >> 3u)) * 16u));
        const uint8_t tile_row = (uint8_t)(source_row & 0x07u);
        const int framebuffer_y = (int)sprite_y + (int)pixel_y;
        unsigned pixel_x;

        if ((framebuffer_y < 0) || (framebuffer_y >= (int)CONTRA_FRAMEBUFFER_HEIGHT))
        {
            continue;
        }

        for (pixel_x = 0u; pixel_x < 8u; ++pixel_x)
        {
            const uint8_t source_x = flip_horizontal ? (uint8_t)(7u - pixel_x) : (uint8_t)pixel_x;
            const uint8_t color_index = contra_read_pattern_color_index(core, pattern_addr, source_x, tile_row);
            const int framebuffer_x = (int)sprite_x + (int)pixel_x;
            size_t framebuffer_index;

            if ((color_index == 0u) ||
                (framebuffer_x < 0) ||
                (framebuffer_x >= (int)CONTRA_FRAMEBUFFER_WIDTH))
            {
                continue;
            }

            framebuffer_index = ((size_t)framebuffer_y * CONTRA_FRAMEBUFFER_WIDTH) + (size_t)framebuffer_x;
            if (priority_behind_bg && (core->background_opaque[framebuffer_index] != 0u))
            {
                continue;
            }

            if ((core->sprite_priority[framebuffer_index] != 0xFFu) &&
                (core->sprite_priority[framebuffer_index] < sprite_order))
            {
                continue;
            }

            core->framebuffer[framebuffer_index] =
                contra_sprite_palette_color_rgba(core, palette_slot, color_index);
            core->sprite_priority[framebuffer_index] = sprite_order;
        }
    }
}

static uint16_t contra_read_sprite_ptr(uint8_t sprite_code)
{
    if (sprite_code == 0u)
    {
        return 0u;
    }

    if (sprite_code < 0x80u)
    {
        return contra_rom_read_u16(1u, (uint16_t)(contra_sprite_ptr_tbl_0_addr + (((uint16_t)sprite_code - 1u) * 2u)));
    }

    return contra_rom_read_u16(1u, (uint16_t)(contra_sprite_ptr_tbl_1_addr + (((uint16_t)sprite_code - 0x80u) * 2u)));
}

static void contra_oam_advance_addr(uint8_t *offset)
{
    *offset = (uint8_t)(*offset + 0xC4u);
}

static void contra_write_oam_entry(
    uint8_t *oam,
    uint8_t *offset,
    uint8_t *remaining,
    uint8_t sprite_y,
    uint8_t tile_index,
    uint8_t attr,
    uint8_t sprite_x
)
{
    const uint8_t write_offset = *offset;

    if ((*remaining & 0x80u) != 0u)
    {
        return;
    }

    oam[write_offset + 0u] = sprite_y;
    oam[write_offset + 1u] = tile_index;
    oam[write_offset + 2u] = attr;
    oam[write_offset + 3u] = sprite_x;
    contra_oam_advance_addr(offset);
    *remaining = (uint8_t)(*remaining - 1u);
}

static void contra_write_hud_sprites_to_oam(
    const ContraCore *core,
    uint8_t *oam,
    uint8_t *offset,
    uint8_t *remaining
)
{
    static const uint8_t hud_sprites[8] = {0x0Au, 0x0Au, 0x0Au, 0x0Au, 0x02u, 0x04u, 0x06u, 0x08u};
    static const uint8_t hud_x_offsets[8] = {0x10u, 0x1Cu, 0x28u, 0x34u, 0x10u, 0x1Cu, 0x28u, 0x34u};
    uint8_t player = core->ram[CONTRA_RAM_PLAYER_MODE];

    if (core->ram[CONTRA_RAM_DEMO_MODE] != 0u)
    {
        player = 0x01u;
    }

    for (;;)
    {
        uint8_t sprite_offset = 0x00u;
        uint8_t sprite_count;

        if ((core->ram[CONTRA_RAM_DEMO_MODE] != 0u) ||
            (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + (player & 0x01u)] != 0u))
        {
            sprite_offset = 0x04u;
            sprite_count = 0x04u;
        }
        else
        {
            sprite_count = core->ram[CONTRA_RAM_P1_NUM_LIVES + (player & 0x01u)];
            if (sprite_count >= 0x04u)
            {
                sprite_count = 0x04u;
            }
        }

        while (sprite_count-- != 0u)
        {
            const uint8_t hud_index = sprite_offset++;
            uint8_t sprite_x = hud_x_offsets[hud_index];

            if ((player & 0x01u) != 0u)
            {
                sprite_x = (uint8_t)(sprite_x + 0xB0u);
            }

            contra_write_oam_entry(
                oam,
                offset,
                remaining,
                0x10u,
                hud_sprites[hud_index],
                (uint8_t)(player & 0x01u),
                sprite_x
            );
        }

        if (player == 0u)
        {
            break;
        }
        --player;
    }
}

static void contra_write_cpu_sprite_to_oam(
    const ContraCore *core,
    size_t sprite_index,
    uint8_t *oam,
    uint8_t *offset,
    uint8_t *remaining
)
{
    const uint8_t sprite_code = core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + sprite_index];
    const uint8_t base_attr = core->ram[CONTRA_RAM_SPRITE_ATTR + sprite_index];
    const uint8_t base_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + sprite_index];
    const uint8_t base_x = core->ram[CONTRA_RAM_SPRITE_X_POS + sprite_index];
    uint16_t sprite_addr;

    if ((!contra_load_rom_image()) || (sprite_code == 0u))
    {
        return;
    }

    sprite_addr = contra_read_sprite_ptr(sprite_code);
    if (sprite_addr == 0u)
    {
        return;
    }

    if (contra_rom_read_u8(1u, sprite_addr) == 0xFEu)
    {
        const uint8_t tile = contra_rom_read_u8(1u, (uint16_t)(sprite_addr + 1u));
        const uint8_t attr = (uint8_t)(contra_rom_read_u8(1u, (uint16_t)(sprite_addr + 2u)) | base_attr);
        const uint8_t sprite_y = (uint8_t)(base_y - 0x08u);
        const uint8_t sprite_x = (uint8_t)(base_x - 0x04u);

        contra_write_oam_entry(oam, offset, remaining, sprite_y, tile, attr, sprite_x);
        return;
    }

    {
        uint8_t sprite_tile_count = contra_rom_read_u8(1u, sprite_addr);
        uint16_t read_addr = (uint16_t)(sprite_addr + 1u);
        uint8_t sprite_effect = (uint8_t)(base_attr & 0xC8u);
        const uint8_t attr_mask = (base_attr & 0x04u) != 0u ? 0xFCu : 0xFFu;
        const uint8_t attr_base = (uint8_t)((base_attr & 0x04u) != 0u ? (base_attr & 0x23u) : (base_attr & 0x20u));

        while (sprite_tile_count != 0u)
        {
            uint8_t relative_y = contra_rom_read_u8(1u, read_addr++);

            if (relative_y == 0x80u)
            {
                sprite_effect &= 0xF7u;
                read_addr = contra_rom_read_u16(1u, read_addr);
                continue;
            }

            {
                const uint8_t tile = contra_rom_read_u8(1u, read_addr++);
                const uint8_t tile_attr = contra_rom_read_u8(1u, read_addr++);
                const uint8_t relative_x = contra_rom_read_u8(1u, read_addr++);
                const uint8_t adjusted_relative_y = (uint8_t)(relative_y + ((sprite_effect & 0x08u) != 0u ? 1u : 0u));
                const uint8_t sprite_y = (sprite_effect & 0x80u) != 0u
                    ? (uint8_t)(base_y + (uint8_t)(0xF0u - adjusted_relative_y))
                    : (uint8_t)(base_y + adjusted_relative_y);
                const uint8_t sprite_x = (sprite_effect & 0x40u) != 0u
                    ? (uint8_t)(base_x + (uint8_t)(0xF8u - relative_x))
                    : (uint8_t)(base_x + relative_x);
                const uint8_t attr = (uint8_t)(((tile_attr & attr_mask) | attr_base) ^ sprite_effect);

                contra_write_oam_entry(oam, offset, remaining, sprite_y, tile, attr, sprite_x);
                --sprite_tile_count;
            }
        }
    }
}

static void contra_build_oam_for_next_frame(ContraCore *core)
{
    uint8_t oam[0x100u];
    uint8_t offset = (uint8_t)(core->ram[CONTRA_RAM_OAMDMA_CPU_BUFFER_OFFSET] + 0x4Cu);
    uint8_t remaining = 0x3Fu;
    size_t sprite_index = CONTRA_CPU_SPRITE_RENDER_SLOTS;

    memset(oam, 0xF4, sizeof(oam));
    core->ram[CONTRA_RAM_OAMDMA_CPU_BUFFER_OFFSET] = offset;

    if (core->ram[CONTRA_RAM_SPRITE_LOAD_TYPE] != 0u)
    {
        contra_write_hud_sprites_to_oam(core, oam, &offset, &remaining);
    }

    while (sprite_index-- != 0u)
    {
        /* In the faithful enemy system the enemy slots (10..25) hold real enemy
           state. Background-tile enemy types (pill box, turrets, bridge) draw to
           the nametable, not OAM, so skip them here to avoid a stray sprite. */
        if (CONTRA_USE_ROM_ENEMY_SYSTEM && (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0u) &&
            (sprite_index >= 10u) &&
            contra_rom_enemy_type_is_supertile(core->ram[CONTRA_RAM_ENEMY_TYPE + (sprite_index - 10u)]))
        {
            continue;
        }
        contra_write_cpu_sprite_to_oam(core, sprite_index, oam, &offset, &remaining);
    }

    while ((remaining & 0x80u) == 0u)
    {
        oam[offset] = 0xF4u;
        contra_oam_advance_addr(&offset);
        remaining = (uint8_t)(remaining - 1u);
    }

    memcpy(core->latched_oam, oam, sizeof(core->latched_oam));
    memcpy(&core->ram[CONTRA_RAM_OAMDMA_CPU_BUFFER], oam, sizeof(oam));
}

static bool contra_oam_sprite_visible_on_scanline(const uint8_t *oam, size_t sprite_index, int scanline)
{
    size_t earlier_index;
    unsigned visible_count = 0u;

    for (earlier_index = 0u; earlier_index < sprite_index; ++earlier_index)
    {
        const uint8_t sprite_y = oam[(earlier_index * 4u) + 0u];
        const int top = (int)sprite_y + 1;

        if ((sprite_y < 0xEFu) && (scanline >= top) && (scanline < (top + 16)))
        {
            ++visible_count;
            if (visible_count >= 8u)
            {
                return false;
            }
        }
    }

    return true;
}

static void contra_render_oam_sprite(ContraCore *core, size_t sprite_index)
{
    const size_t oam_offset = sprite_index * 4u;
    const uint8_t sprite_y = core->latched_oam[oam_offset + 0u];
    const uint8_t tile_index = core->latched_oam[oam_offset + 1u];
    const uint8_t attr = core->latched_oam[oam_offset + 2u];
    const uint8_t sprite_x = core->latched_oam[oam_offset + 3u];
    const uint16_t pattern_base = (uint16_t)(((uint16_t)(tile_index & 0x01u) << 12u) + ((uint16_t)(tile_index & 0xFEu) * 16u));
    const uint8_t palette_slot = (uint8_t)(attr & 0x03u);
    const bool priority_behind_bg = (attr & 0x20u) != 0u;
    const bool flip_horizontal = (attr & 0x40u) != 0u;
    const bool flip_vertical = (attr & 0x80u) != 0u;
    unsigned pixel_y;

    if ((sprite_y >= 0xEFu) || ((pattern_base + 31u) >= sizeof(core->ppu_pattern)))
    {
        return;
    }

    for (pixel_y = 0u; pixel_y < 16u; ++pixel_y)
    {
        const unsigned source_row = flip_vertical ? (15u - pixel_y) : pixel_y;
        const uint16_t pattern_addr = (uint16_t)(pattern_base + (((uint16_t)(source_row >> 3u)) * 16u));
        const uint8_t tile_row = (uint8_t)(source_row & 0x07u);
        const int framebuffer_y = (int)sprite_y + 1 + (int)pixel_y;
        unsigned pixel_x;

        if ((framebuffer_y < 0) ||
            (framebuffer_y >= (int)CONTRA_FRAMEBUFFER_HEIGHT) ||
            !contra_oam_sprite_visible_on_scanline(core->latched_oam, sprite_index, framebuffer_y))
        {
            continue;
        }

        for (pixel_x = 0u; pixel_x < 8u; ++pixel_x)
        {
            const uint8_t source_x = flip_horizontal ? (uint8_t)(7u - pixel_x) : (uint8_t)pixel_x;
            const uint8_t color_index = contra_read_pattern_color_index(core, pattern_addr, source_x, tile_row);
            const int framebuffer_x = (int)sprite_x + (int)pixel_x;
            size_t framebuffer_index;

            if ((color_index == 0u) ||
                (framebuffer_x < 0) ||
                (framebuffer_x >= (int)CONTRA_FRAMEBUFFER_WIDTH))
            {
                continue;
            }

            framebuffer_index = ((size_t)framebuffer_y * CONTRA_FRAMEBUFFER_WIDTH) + (size_t)framebuffer_x;
            if (priority_behind_bg && (core->background_opaque[framebuffer_index] != 0u))
            {
                continue;
            }

            if ((core->sprite_priority[framebuffer_index] != 0xFFu) &&
                (core->sprite_priority[framebuffer_index] < sprite_index))
            {
                continue;
            }

            core->framebuffer[framebuffer_index] =
                contra_sprite_palette_color_rgba(core, palette_slot, color_index);
            core->sprite_priority[framebuffer_index] = (uint8_t)sprite_index;
        }
    }
}

static void contra_render_cpu_sprites(ContraCore *core)
{
    size_t sprite_index;

    for (sprite_index = 0u; sprite_index < 64u; ++sprite_index)
    {
        contra_render_oam_sprite(core, sprite_index);
    }
}

static void contra_render_intro_background(ContraCore *core)
{
    const uint8_t base_nametable = (uint8_t)(core->latched_ppuctrl_settings & 0x01u);
    const uint8_t fine_scroll_x = core->latched_horizontal_scroll;
    const uint8_t tile_scroll_x = (uint8_t)(fine_scroll_x >> 3u);
    const uint8_t pixel_scroll_x = (uint8_t)(fine_scroll_x & 0x07u);
    unsigned tile_y;

    for (tile_y = 0u; tile_y < 30u; ++tile_y)
    {
        unsigned screen_tile_x;

        for (screen_tile_x = 0u; screen_tile_x < 33u; ++screen_tile_x)
        {
            const uint8_t world_tile_x = (uint8_t)(tile_scroll_x + (uint8_t)screen_tile_x);
            const uint8_t nametable_index = (uint8_t)(base_nametable ^ ((world_tile_x >> 5u) & 0x01u));
            const uint8_t coarse_x = (uint8_t)(world_tile_x & 0x1Fu);
            const uint16_t tile_offset = (uint16_t)(((uint16_t)tile_y * 32u) + (uint16_t)coarse_x);
            const uint8_t pattern_index = contra_read_nametable_byte(core, nametable_index, tile_offset);
            const uint8_t palette_slot = contra_read_nametable_palette_slot(core, nametable_index, coarse_x, (uint8_t)tile_y);
            const int dest_x = ((int)screen_tile_x * 8) - (int)pixel_scroll_x;

            contra_draw_background_tile(core, dest_x, (int)tile_y * 8, pattern_index, palette_slot);
        }
    }
}

static void contra_render_horizontal_level_background_scrolled(ContraCore *core)
{
    const uint8_t *const ram = core->ram;
    const uint16_t supertile_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u)
    );
    const uint16_t palette_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] << 8u)
    );
    uint8_t current_screen_supertiles[CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE];
    uint8_t next_screen_supertiles[CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE];
    const uint8_t scroll_pixels = core->latched_level_screen_scroll_offset;
    const uint8_t tile_scroll_x = (uint8_t)(scroll_pixels >> 3u);
    const uint8_t pixel_scroll_x = (uint8_t)(scroll_pixels & 0x07u);
    const size_t origin_y = 16u;
    size_t tile_y;

    memset(current_screen_supertiles, 0, sizeof(current_screen_supertiles));
    memset(next_screen_supertiles, 0, sizeof(next_screen_supertiles));
    contra_decode_level_screen_supertiles(core, core->latched_level_screen_number, current_screen_supertiles, 0u);
    contra_decode_level_screen_supertiles(core, (uint8_t)(core->latched_level_screen_number + 1u), next_screen_supertiles, 0u);

    for (tile_y = 0u; tile_y < 28u; ++tile_y)
    {
        size_t screen_tile_x;

        for (screen_tile_x = 0u; screen_tile_x < 33u; ++screen_tile_x)
        {
            const uint8_t world_tile_x = (uint8_t)(tile_scroll_x + (uint8_t)screen_tile_x);
            const uint8_t local_screen = (uint8_t)(world_tile_x >> 5u);
            const uint8_t tile_x_in_screen = (uint8_t)(world_tile_x & 0x1Fu);
            const uint8_t *const screen_supertiles = (local_screen == 0u)
                ? current_screen_supertiles
                : next_screen_supertiles;
            const size_t supertile_column = (size_t)tile_x_in_screen / 4u;
            const size_t supertile_row = tile_y / 4u;
            const size_t supertile_offset = (supertile_row * 8u) + supertile_column;
            const uint8_t supertile_index = screen_supertiles[supertile_offset];
            const size_t supertile_data_addr = (size_t)supertile_index * 16u;
            const uint8_t tile_in_supertile = (uint8_t)(((tile_y & 0x03u) << 2u) | (tile_x_in_screen & 0x03u));
            const uint8_t pattern_index = contra_rom_read_u8(3u, (uint16_t)(supertile_ptr + supertile_data_addr + tile_in_supertile));
            const uint8_t supertile_palette = contra_rom_read_u8(3u, (uint16_t)(palette_ptr + supertile_index));
            const uint8_t palette_shift = (uint8_t)(((tile_y & 0x02u) << 1u) | (tile_x_in_screen & 0x02u));
            const uint8_t palette_slot = (uint8_t)((supertile_palette >> palette_shift) & 0x03u);
            const int dest_x = ((int)screen_tile_x * 8) - (int)pixel_scroll_x;

            contra_draw_background_tile(
                core,
                dest_x,
                (int)(origin_y + (tile_y * 8u)),
                pattern_index,
                palette_slot
            );
        }
    }
}

static void contra_render_level_background(ContraCore *core)
{
    const uint8_t *const ram = core->ram;
    const size_t visible_super_rows = (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ? 8u : 7u;
    const size_t visible_tile_rows = visible_super_rows * 4u;
    const size_t origin_y = (CONTRA_FRAMEBUFFER_HEIGHT > (visible_tile_rows * 8u))
        ? (CONTRA_FRAMEBUFFER_HEIGHT - (visible_tile_rows * 8u))
        : 0u;
    size_t visible_tile_columns = 32u;
    size_t tile_y;

    if (!contra_load_rom_image())
    {
        return;
    }

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u))
    {
        contra_render_horizontal_level_background_scrolled(core);
        return;
    }

    if ((ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x03u) &&
        (ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] == 0x20u))
    {
        visible_tile_columns = ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET];
        if (visible_tile_columns > 32u)
        {
            visible_tile_columns = 32u;
        }
    }

    for (tile_y = 0u; tile_y < visible_tile_rows; ++tile_y)
    {
        size_t tile_x;

        for (tile_x = 0u; tile_x < visible_tile_columns; ++tile_x)
        {
            const size_t supertile_column = tile_x / 4u;
            const size_t supertile_row = tile_y / 4u;
            const size_t supertile_offset = (supertile_row * 8u) + supertile_column;
            const uint8_t supertile_index = core->level_screen_supertiles[supertile_offset];
            const size_t supertile_data_addr = (size_t)supertile_index * 16u;
            const uint16_t supertile_ptr = (uint16_t)(
                (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
                ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u)
            );
            const uint16_t palette_ptr = (uint16_t)(
                (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] |
                ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] << 8u)
            );
            const uint8_t tile_in_supertile = (uint8_t)(((tile_y & 0x03u) << 2u) | (tile_x & 0x03u));
            const uint8_t pattern_index = contra_rom_read_u8(3u, (uint16_t)(supertile_ptr + supertile_data_addr + tile_in_supertile));
            const uint8_t supertile_palette = contra_rom_read_u8(3u, (uint16_t)(palette_ptr + supertile_index));
            const uint8_t palette_shift = (uint8_t)(((tile_y & 0x02u) << 1u) | (tile_x & 0x02u));
            const uint8_t palette_slot = (uint8_t)((supertile_palette >> palette_shift) & 0x03u);

            contra_draw_background_tile(
                core,
                (int)(tile_x * 8u),
                (int)(origin_y + (tile_y * 8u)),
                pattern_index,
                palette_slot
            );
        }
    }
}

static void contra_zero_out_nametables(ContraCore *core)
{
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = 0x00u;
    core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER] = 0x00u;
    memset(&core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER], 0, CONTRA_CPU_GRAPHICS_BUFFER_SIZE);
    memset(core->ppu_nametable, 0, sizeof(core->ppu_nametable));
}

static void contra_load_intro_graphics(ContraCore *core)
{
    contra_init_apu_channels(core);
    contra_clear_memory_3(core);
    core->ram[CONTRA_RAM_PPUMASK_SETTINGS] = 0x1Eu;
    core->ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0x00u;
    core->ram[CONTRA_RAM_DEMO_MODE] = 0x01u;
    contra_load_graphic_data_list(core, 11u);
    contra_load_bank_6_write_text_palette_to_mem(core, 0x06u);
}

static void contra_load_level_intro_screen_graphics(ContraCore *core)
{
    contra_load_graphic_data_list(core, 10u);
}

static void contra_load_bank_6_write_text_palette_to_mem(ContraCore *core, uint8_t text_code)
{
    const bool blank_text = (text_code & 0x80u) != 0u;
    const uint8_t table_index = (uint8_t)(text_code & 0x3Fu);
    uint16_t read_addr;
    uint8_t blank_delay = 0x02u;

    if (!contra_load_rom_image())
    {
        return;
    }

    read_addr = contra_rom_read_u16(
        6u,
        (uint16_t)(contra_short_text_pointer_table_addr + ((uint16_t)table_index * 2u))
    );

    contra_write_cpu_graphics_buffer_byte(core, 0x01u);

    while (core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] < CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
    {
        uint8_t value = contra_rom_read_u8(6u, read_addr++);

        if (value == 0xFFu)
        {
            return;
        }

        if (value == 0xFEu)
        {
            contra_write_cpu_graphics_buffer_byte(core, 0xFFu);
            return;
        }

        if (value == 0xFDu)
        {
            contra_write_cpu_graphics_buffer_byte(core, 0xFFu);
            blank_delay = 0x02u;
            contra_write_cpu_graphics_buffer_byte(core, 0x01u);
            continue;
        }

        if (blank_text)
        {
            if (blank_delay == 0u)
            {
                value = 0x00u;
            }
            else
            {
                blank_delay = (uint8_t)(blank_delay - 1u);
            }
        }

        contra_write_cpu_graphics_buffer_byte(core, value);
    }
}

static void contra_play_sound(ContraCore *core, uint8_t sound_code)
{
    (void)core;
    (void)sound_code;
}

static void contra_init_apu_channels(ContraCore *core)
{
    (void)core;
}

static void contra_patch_cpu_graphics_buffer_byte(ContraCore *core, uint8_t offset, uint8_t value)
{
    if (offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
    {
        return;
    }

    core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + offset] = value;
}

static void contra_patch_cpu_graphics_buffer_from_end(ContraCore *core, uint8_t back_offset, uint8_t value)
{
    const uint8_t offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];

    if ((offset < back_offset) || (offset > CONTRA_CPU_GRAPHICS_BUFFER_SIZE))
    {
        return;
    }

    contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(offset - back_offset), value);
}

static uint8_t contra_calculate_score_digit(uint8_t *low, uint8_t *high)
{
    uint8_t digit = 0x00u;
    uint8_t shift_count = 0x10u;
    bool carry = ((*low & 0x80u) != 0u);

    *low = (uint8_t)(*low << 1u);

    {
        const bool next_carry = ((*high & 0x80u) != 0u);
        *high = (uint8_t)((*high << 1u) | (carry ? 0x01u : 0x00u));
        carry = next_carry;
    }

    do
    {
        const bool digit_carry = ((digit & 0x80u) != 0u);

        digit = (uint8_t)((digit << 1u) | (carry ? 0x01u : 0x00u));
        carry = digit_carry;

        if (digit >= 0x0Au)
        {
            digit = (uint8_t)(digit - 0x0Au);
            carry = true;
        }

        {
            const bool low_carry = ((*low & 0x80u) != 0u);
            *low = (uint8_t)((*low << 1u) | (carry ? 0x01u : 0x00u));
            carry = low_carry;
        }

        {
            const bool high_carry = ((*high & 0x80u) != 0u);
            *high = (uint8_t)((*high << 1u) | (carry ? 0x01u : 0x00u));
            carry = high_carry;
        }
    } while (--shift_count != 0u);

    return digit;
}

static void contra_draw_stage_and_level_name(ContraCore *core)
{
    const uint8_t current_level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Cu);
    contra_patch_cpu_graphics_buffer_from_end(core, 0x02u, (uint8_t)(current_level + 0x31u));
    contra_load_bank_6_write_text_palette_to_mem(core, (uint8_t)(current_level + 0x11u));
}

static void contra_draw_player_num_lives(ContraCore *core)
{
    const uint8_t player_index = (uint8_t)(core->ram[CONTRA_RAM_DRAW_PLAYER_INDEX] & 0x01u);
    uint8_t remaining_lives;
    uint8_t tens_digit = 0x00u;
    uint8_t ones_digit;

    contra_load_bank_6_write_text_palette_to_mem(core, (uint8_t)(0x07u + player_index));

    remaining_lives = (uint8_t)(
        (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] ^ 0x01u) +
        core->ram[CONTRA_RAM_P1_NUM_LIVES + player_index]
    );

    if (remaining_lives == 0u)
    {
        contra_load_bank_6_write_text_palette_to_mem(core, (uint8_t)(0x0Fu + player_index));
        return;
    }

    if ((remaining_lives & 0x80u) != 0u)
    {
        remaining_lives = 0x00u;
    }

    while (remaining_lives >= 0x0Au)
    {
        remaining_lives = (uint8_t)(remaining_lives - 0x0Au);
        ++tens_digit;

        if (tens_digit >= 0x0Au)
        {
            tens_digit = 0x09u;
            remaining_lives = 0x09u;
            break;
        }
    }

    ones_digit = (uint8_t)(remaining_lives | 0x30u);
    if ((tens_digit == 0u) && (ones_digit == 0x30u))
    {
        return;
    }

    contra_patch_cpu_graphics_buffer_from_end(core, 0x02u, ones_digit);
    if (tens_digit != 0u)
    {
        contra_patch_cpu_graphics_buffer_from_end(core, 0x03u, (uint8_t)(tens_digit | 0x30u));
    }
}

static void contra_draw_the_scores(ContraCore *core)
{
    uint8_t score_low;
    uint8_t score_high;
    uint8_t original_offset;
    uint8_t write_offset;
    uint8_t digits_remaining;
    uint8_t digit = 0x00u;
    bool zero_score;

    contra_load_bank_6_write_text_palette_to_mem(core, 0x09u);
    score_low = core->ram[CONTRA_RAM_HIGH_SCORE_LOW];
    score_high = core->ram[CONTRA_RAM_HIGH_SCORE_HIGH];

    if ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x10u) == 0u)
    {
        original_offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];
        write_offset = original_offset;
        digits_remaining = 0x05u;

        do
        {
            digit = contra_calculate_score_digit(&score_low, &score_high);
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(write_offset - 0x04u), (uint8_t)(digit | 0x30u));
            --write_offset;

            if ((uint8_t)(score_low | score_high) == 0u)
            {
                break;
            }

            --digits_remaining;
        } while (digits_remaining != 0u);

        zero_score = (bool)((digits_remaining == 0x05u) && (digit == 0u));
        if (zero_score)
        {
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x04u), 0x00u);
        }

        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x03u), 0x30u);
        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x02u), 0x30u);
    }

    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Au);
    score_low = core->ram[CONTRA_RAM_PLAYER_1_SCORE_LOW];
    score_high = core->ram[CONTRA_RAM_PLAYER_1_SCORE_HIGH];

    if ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x10u) == 0u)
    {
        original_offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];
        write_offset = original_offset;
        digits_remaining = 0x05u;

        do
        {
            digit = contra_calculate_score_digit(&score_low, &score_high);
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(write_offset - 0x04u), (uint8_t)(digit | 0x30u));
            --write_offset;

            if ((uint8_t)(score_low | score_high) == 0u)
            {
                break;
            }

            --digits_remaining;
        } while (digits_remaining != 0u);

        zero_score = (bool)((digits_remaining == 0x05u) && (digit == 0u));
        if (zero_score)
        {
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x04u), 0x00u);
        }

        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x03u), 0x30u);
        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x02u), 0x30u);
    }

    if (core->ram[CONTRA_RAM_PLAYER_MODE] == 0u)
    {
        return;
    }

    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Bu);
    score_low = core->ram[CONTRA_RAM_PLAYER_2_SCORE_LOW];
    score_high = core->ram[CONTRA_RAM_PLAYER_2_SCORE_HIGH];

    if ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x10u) == 0u)
    {
        original_offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];
        write_offset = original_offset;
        digits_remaining = 0x05u;

        do
        {
            digit = contra_calculate_score_digit(&score_low, &score_high);
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(write_offset - 0x04u), (uint8_t)(digit | 0x30u));
            --write_offset;

            if ((uint8_t)(score_low | score_high) == 0u)
            {
                break;
            }

            --digits_remaining;
        } while (digits_remaining != 0u);

        zero_score = (bool)((digits_remaining == 0x05u) && (digit == 0u));
        if (zero_score)
        {
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x04u), 0x00u);
        }

        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x03u), 0x30u);
        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x02u), 0x30u);
    }
}

static void contra_load_level_graphics(ContraCore *core)
{
    uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    if (level > 7u)
    {
        level = 7u;
    }

    contra_load_graphic_data_list(core, level);
}

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
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_ACCUM + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_ACCUM + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FRACT + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FRACT + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] = 0x00u;
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

static void contra_advance_subpixel_position_s16(int16_t *pos, uint8_t *accum, uint8_t fast, uint8_t fract)
{
    const uint16_t sum = (uint16_t)(*accum) + (uint16_t)fract;

    *accum = (uint8_t)sum;
    *pos = (int16_t)((int)(*pos) + (int)(int8_t)fast + (int)(sum >> 8u));
}

static void contra_negate_subpixel_velocity(uint8_t *fast, uint8_t *fract)
{
    const bool borrow = (*fract != 0u);

    *fract = (uint8_t)(0u - (uint16_t)(*fract));
    *fast = (uint8_t)(0u - (uint16_t)(*fast) - (borrow ? 1u : 0u));
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

static void contra_init_player_bullet_position(
    ContraCore *core,
    size_t bullet_slot,
    uint8_t player_index,
    uint8_t aim_dir
)
{
    uint8_t *const ram = core->ram;
    const int8_t (*position_table)[2];

    position_table = (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
        ? contra_bullet_initial_pos_jump
        : contra_bullet_initial_pos_ground;

    ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_slot] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] + position_table[aim_dir][0]);
    ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_slot] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + position_table[aim_dir][1]);
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
    center_x =
        (int)ram[CONTRA_RAM_PLAYER_BULLET_X_POS + (size_t)bullet_slot] + contra_f_bullet_center_offset_tbl[aim_dir][0];
    center_y =
        (int)ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + (size_t)bullet_slot] + contra_f_bullet_center_offset_tbl[aim_dir][1];
    if ((center_y < 0) || (center_y > 0xFF) || (center_x < 0) || (center_x > 0xFF))
    {
        contra_clear_player_bullet(core, (size_t)bullet_slot);
        return false;
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
        contra_init_player_bullet_position(core, (size_t)bullet_slot, player_index, aim_dir);
        contra_set_spray_bullet_velocity(core, (size_t)bullet_slot, aim_dir, bullet_num, rapid_fire);
        ram[CONTRA_RAM_PLAYER_BULLET_S_BULLET_NUM + (size_t)bullet_slot] = bullet_num;
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
        }
        else
        {
            contra_update_machine_gun_fire_time(core, player_index);
        }
        return;
    }

    if (weapon_type == 0x04u)
    {
        if (fire_pressed)
        {
            contra_create_laser_bullets(core, player_index, fire_pressed_this_frame);
        }
        return;
    }

    if (!fire_pressed_this_frame)
    {
        return;
    }

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
}

static void contra_update_shared_player_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;

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

static void contra_update_spray_bullet(ContraCore *core, size_t bullet_index)
{
    uint8_t *const ram = core->ram;
    uint8_t sprite_code = 0x21u;

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

static uint8_t contra_classify_pattern_collision(const ContraCore *core, uint8_t pattern_index)
{
    if (pattern_index == 0u)
    {
        return 0u;
    }

    if (pattern_index < core->ram[CONTRA_RAM_COLLISION_CODE_1_TILE_INDEX])
    {
        return 1u;
    }

    if (pattern_index < core->ram[CONTRA_RAM_COLLISION_CODE_0_TILE_INDEX])
    {
        return 0u;
    }

    if (pattern_index < core->ram[CONTRA_RAM_COLLISION_CODE_2_TILE_INDEX])
    {
        return 2u;
    }

    return 3u;
}

static bool contra_read_level_collision_pattern_index(
    const ContraCore *core,
    uint16_t world_tile_x,
    uint16_t world_tile_y,
    uint8_t *pattern_index
)
{
    const uint16_t supertile_ptr = (uint16_t)(
        (uint16_t)core->ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
        ((uint16_t)core->ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u)
    );
    uint8_t screen_supertiles[CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE];
    uint8_t screen_number;
    uint8_t tile_x_in_screen;
    uint8_t tile_y_in_screen;
    uint8_t supertile_column;
    uint8_t supertile_row;
    uint8_t tile_x_in_supertile;
    uint8_t tile_y_in_supertile;
    size_t supertile_offset;
    size_t tile_offset;
    uint8_t supertile_index;

    if (!contra_load_rom_image())
    {
        return false;
    }

    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        if (world_tile_x >= 32u)
        {
            return false;
        }

        screen_number = (uint8_t)(world_tile_y / 30u);
        tile_x_in_screen = (uint8_t)world_tile_x;
        tile_y_in_screen = (uint8_t)(world_tile_y % 30u);
    }
    else
    {
        screen_number = (uint8_t)(world_tile_x >> 5u);
        tile_x_in_screen = (uint8_t)(world_tile_x & 0x1Fu);
        tile_y_in_screen = (uint8_t)world_tile_y;
        if (tile_y_in_screen >= 28u)
        {
            return false;
        }
    }

    supertile_column = (uint8_t)(tile_x_in_screen >> 2u);
    supertile_row = (uint8_t)(tile_y_in_screen >> 2u);
    tile_x_in_supertile = (uint8_t)((tile_x_in_screen & 0x03u) & 0x02u);
    tile_y_in_supertile = (uint8_t)((tile_y_in_screen & 0x03u) & 0x02u);
    supertile_offset = ((size_t)supertile_row * 8u) + (size_t)supertile_column;
    tile_offset = ((size_t)tile_y_in_supertile * 4u) + (size_t)tile_x_in_supertile;

    if ((supertile_column >= 8u) || (supertile_row >= 8u))
    {
        return false;
    }

    memset(screen_supertiles, 0, sizeof(screen_supertiles));
    contra_decode_level_screen_supertiles((ContraCore *)core, screen_number, screen_supertiles, 0u);
    supertile_index = screen_supertiles[supertile_offset];
    *pattern_index = contra_rom_read_u8(3u, (uint16_t)(supertile_ptr + ((uint16_t)supertile_index * 16u) + (uint16_t)tile_offset));
    return true;
}

static uint8_t contra_level_1_bridge_get_overlay(const ContraNativeEnemy *enemy, unsigned section)
{
    switch (section)
    {
        case 0u:
            return enemy->screen_id;

        case 1u:
            return enemy->sprite_code;

        case 2u:
            return enemy->sprite_attr;

        default:
            return enemy->hp;
    }
}

static void contra_level_1_bridge_set_overlay(ContraNativeEnemy *enemy, unsigned section, uint8_t supertile_index)
{
    switch (section)
    {
        case 0u:
            enemy->screen_id = supertile_index;
            break;

        case 1u:
            enemy->sprite_code = supertile_index;
            break;

        case 2u:
            enemy->sprite_attr = supertile_index;
            break;

        default:
            enemy->hp = supertile_index;
            break;
    }
}

static bool contra_level_1_bridge_has_destroyed_collision_gap(
    const ContraCore *core,
    uint8_t screen_x,
    uint8_t screen_y
)
{
    const uint8_t game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    size_t enemy_index;

    if (!(((game_routine == 0x05u) ||
           ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
          (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
          (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0u) &&
          (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
          (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u)))
    {
        return false;
    }

    for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
    {
        const ContraNativeEnemy *const enemy = &core->enemies[enemy_index];
        const int bridge_top = (int)enemy->y - 12;
        unsigned section;

        if ((enemy->active == 0u) || (enemy->type != 0x12u))
        {
            continue;
        }

        if (((int)screen_y < bridge_top) || ((int)screen_y >= (bridge_top + 32)))
        {
            continue;
        }

        for (section = 0u; section < 4u; ++section)
        {
            const uint8_t overlay = contra_level_1_bridge_get_overlay(enemy, section);
            const int bridge_left = ((int)enemy->x - 12) + (int)(section * 32u);

            if ((overlay == 0u) ||
                ((int)screen_x < bridge_left) ||
                ((int)screen_x >= (bridge_left + 32)))
            {
                continue;
            }

            return true;
        }
    }

    return false;
}

/* Faithful exploding-bridge collision gap: true when (screen_x, screen_y) falls
   in a background super-tile the real-RAM bridge has cleared. The gap is anchored
   in world space (screen<<8 + scroll + x is scroll-invariant), so it persists as
   the level scrolls and after the bridge enemy is removed. When the faithful
   system is off this list stays empty, so the check is a no-op. */
static bool contra_rom_bridge_has_gap(const ContraCore *core, uint8_t screen_x, uint8_t screen_y)
{
    const uint16_t world_x =
        (uint16_t)(((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                   core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + screen_x);
    uint8_t i;

    for (i = 0u; i < core->l1_bridge_gap_count; ++i)
    {
        const uint16_t gx = core->l1_bridge_gap_world_x[i];
        const int gy = (int)core->l1_bridge_gap_screen_y[i];

        if ((world_x >= gx) && (world_x < (uint16_t)(gx + 32u)) &&
            ((int)screen_y >= gy) && ((int)screen_y < (gy + 32)))
        {
            return true;
        }
    }
    return false;
}

static uint8_t contra_get_outdoor_horizontal_bg_collision(
    const ContraCore *core,
    uint8_t screen_x,
    uint8_t screen_y
)
{
    const uint16_t world_pixel_x =
        ((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
        (uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] +
        (uint16_t)screen_x;
    uint8_t pattern_index;
    uint8_t collision_code;

    if (screen_y < 0x10u)
    {
        return 0u;
    }

    if (contra_level_1_bridge_has_destroyed_collision_gap(core, screen_x, screen_y) ||
        contra_rom_bridge_has_gap(core, screen_x, screen_y))
    {
        return 0u;
    }

    if (!contra_read_level_collision_pattern_index(
            core,
            (uint16_t)(world_pixel_x >> 3u),
            (uint8_t)((screen_y - 0x10u) >> 3u),
            &pattern_index))
    {
        return 0u;
    }

    collision_code = contra_classify_pattern_collision(core, pattern_index);
    return (collision_code == 3u) ? 0x80u : collision_code;
}

static uint8_t contra_get_outdoor_bg_collision(
    const ContraCore *core,
    uint8_t screen_x,
    uint8_t screen_y
)
{
    uint8_t pattern_index;
    uint8_t collision_code;

    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u)
    {
        return contra_get_outdoor_horizontal_bg_collision(core, screen_x, screen_y);
    }

    if (!contra_read_level_collision_pattern_index(
            core,
            (uint16_t)(screen_x >> 3u),
            (uint16_t)(
                (((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] * 240u) +
                 (uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] +
                 (uint16_t)screen_y) >> 3u
            ),
            &pattern_index))
    {
        return 0u;
    }

    collision_code = contra_classify_pattern_collision(core, pattern_index);
    return (collision_code == 3u) ? 0x80u : collision_code;
}

static uint8_t contra_get_player_bg_collision_code(const ContraCore *core, uint8_t player_index)
{
    const uint8_t location_type = core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];
    const uint8_t sprite_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    const uint8_t sprite_x = core->ram[CONTRA_RAM_SPRITE_X_POS + player_index];

    if ((location_type & 0x80u) != 0u)
    {
        return (sprite_y > 0xC8u) ? 0x01u : 0x00u;
    }

    if (location_type != 0u)
    {
        return (sprite_y > 0xA0u) ? 0x01u : 0x00u;
    }

    return contra_get_outdoor_bg_collision(core, sprite_x, (uint8_t)(sprite_y + 0x10u));
}

static bool contra_can_player_drop_down(const ContraCore *core, uint8_t player_index)
{
    const uint8_t *const ram = core->ram;
    const uint8_t sample_x = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
    const uint8_t sample_y = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + 0x10u);
    uint16_t row_sample_y;

    if ((ram[CONTRA_RAM_LEVEL_STOP_SCROLL] != 0xFFu) &&
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u))
    {
        return true;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        return false;
    }

    if (contra_get_outdoor_bg_collision(core, sample_x, sample_y) == 0x80u)
    {
        return false;
    }

    row_sample_y = (uint16_t)(sample_y & 0xF0u) + 0x10u;
    while (row_sample_y < 0xE0u)
    {
        if (contra_get_outdoor_bg_collision(core, sample_x, (uint8_t)row_sample_y) != 0u)
        {
            return true;
        }

        row_sample_y += 0x10u;
    }

    return false;
}

static bool contra_player_has_solid_collision_ahead(const ContraCore *core, uint8_t player_index, int delta_x)
{
    const uint8_t location_type = core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];
    const uint8_t scrolling_type = core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE];
    uint8_t sample_x = (uint8_t)(core->ram[CONTRA_RAM_SPRITE_X_POS + player_index] + delta_x);
    uint8_t sample_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    unsigned sample_index;

    if ((location_type != 0u) || (scrolling_type != 0u))
    {
        return false;
    }

    sample_y = (sample_y > 0x0Bu) ? (uint8_t)(sample_y - 0x0Bu) : 0x0Au;
    for (sample_index = 0u; sample_index < 3u; ++sample_index)
    {
        const uint8_t collision = contra_get_outdoor_horizontal_bg_collision(
            core,
            sample_x,
            (uint8_t)(sample_y + (uint8_t)(sample_index * 0x10u))
        );

        if (collision == 0x80u)
        {
            return true;
        }
    }

    return false;
}

static void contra_init_player_attributes(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = 0x00u;
    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = 0x00u;
    ram[CONTRA_RAM_SPRITE_X_POS + player_index] = 0x00u;
    ram[CONTRA_RAM_SPRITE_ATTR + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + player_index] = 0x00u;
    ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_Y + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME + player_index] = 0x00u;
    ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_DEATH_FLAG + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ON_ENEMY + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_X + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_AIM_PREV_FRAME + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_FAST_X_VEL_BOOST + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] = 0x00u;
}

static void contra_init_player_data(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] = 0x00u;
    ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] = 0x00u;
}

static uint8_t contra_get_level_screen_type(const ContraCore *core)
{
    const uint8_t location_type = core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];

    if (location_type == 0u)
    {
        return 0u;
    }

    if ((location_type & 0x80u) != 0u)
    {
        return 1u;
    }

    return 2u;
}

static void contra_set_player_horizontal_flip(ContraCore *core, uint8_t player_index)
{
    uint8_t flip = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x3Fu);

    if (core->ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u)
    {
        flip |= 0x40u;
    }

    core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
}

static void contra_set_player_jump_sprite(ContraCore *core, uint8_t player_index)
{
    uint8_t base_flip = 0x00u;
    uint8_t flip = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x3Fu);
    uint8_t anim_index = core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] & 0x03u;

    if ((core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] & 0x80u) != 0u)
    {
        base_flip = 0x40u;
    }

    if (anim_index >= 0x02u)
    {
        flip |= 0xC0u;
    }

    flip ^= base_flip;
    core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
    core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_curled_sprite_code_tbl[anim_index];

    core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] =
        (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] + 1u);
    if (core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] >= 0x05u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] = 0x00u;
        core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
            (uint8_t)((core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u) & 0x03u);
    }
}

static void contra_set_player_death_sprite(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] == 0x06u)
    {
        if (ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] < 0x1Bu)
        {
            ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] + 1u);
            ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x55u;
        }
        else
        {
            ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x56u;
        }

        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = 0x00u;
        return;
    }

    ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] + 1u);
    if ((ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] & 0x07u) == 0u)
    {
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u);
        if (ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] >= 0x05u)
        {
            ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x04u;
        }
    }

    {
        uint8_t frame_index = ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index];
        uint8_t flip = contra_player_death_sprite_tbl[frame_index][1];

        if ((ram[CONTRA_RAM_PLAYER_DEATH_FLAG + player_index] & 0x02u) != 0u)
        {
            flip ^= 0x40u;
        }

        ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_death_sprite_tbl[frame_index][0];
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
    }
}

static void contra_set_player_water_transition_flip(ContraCore *core, uint8_t player_index)
{
    uint8_t flip = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x3Fu);

    if ((core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] & 0x02u) != 0u)
    {
        flip |= 0x40u;
    }

    core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
}

static void contra_set_player_water_sprite(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t water_state = ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index];
    const uint8_t aim_dir = (uint8_t)(ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] % 10u);

    if ((water_state & 0x04u) == 0u)
    {
        if ((water_state & 0x10u) == 0u)
        {
            ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
            ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x05u;
            ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + 0x10u);
            if (aim_dir >= 0x05u)
            {
                water_state |= 0x02u;
            }
            ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] = 0x10u;
            water_state |= 0x90u;
            ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = water_state;
        }

        ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
        if (ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] != 0u)
        {
            if (ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] < 0x0Cu)
            {
                ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x73u;
            }
            contra_set_player_water_transition_flip(core, player_index);
            ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] =
                (uint8_t)(ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] - 1u);
            return;
        }
    }

    water_state = ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index];
    if ((water_state & 0x08u) != 0u)
    {
        ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
        if (ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] == 0u)
        {
            ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x00u;
            ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
            ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - 0x10u);
            return;
        }

        if (ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] < 0x05u)
        {
            ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x05u;
        }

        contra_set_player_water_transition_flip(core, player_index);
        ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] - 1u);
        return;
    }

    water_state = (uint8_t)((water_state | 0x04u) & 0x7Fu);
    ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = water_state;

    if (ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u)
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_water_firing_sprite_tbl[aim_dir][0];
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] =
            (uint8_t)((ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x0Fu) |
                      contra_player_water_firing_sprite_tbl[aim_dir][1]);
    }
    else
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_water_sprite_tbl[aim_dir][0];
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] =
            (uint8_t)((ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x0Fu) |
                      contra_player_water_sprite_tbl[aim_dir][1]);
    }

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x0Fu) == 0u)
    {
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u);
    }

    if ((ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] & 0x01u) == 0u)
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] |= 0x08u;
    }
    else
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] &= (uint8_t)~0x08u;
    }

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (contra_get_outdoor_bg_collision(
             core,
             ram[CONTRA_RAM_SPRITE_X_POS + player_index],
             ram[CONTRA_RAM_SPRITE_Y_POS + player_index]) != 0x02u))
    {
        water_state = (uint8_t)(ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] | 0x88u);
        if (aim_dir >= 0x05u)
        {
            water_state |= 0x02u;
        }
        else
        {
            water_state &= (uint8_t)~0x02u;
        }

        ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = water_state;
        ram[CONTRA_RAM_PLAYER_WATER_TIMER + player_index] = 0x0Cu;
        ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x1Au;
        contra_set_player_water_transition_flip(core, player_index);
    }
}

static void contra_set_player_sprite(ContraCore *core, uint8_t player_index)
{
    uint8_t sequence = core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index];

    if (core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] != 0u)
    {
        contra_set_player_water_sprite(core, player_index);
        return;
    }

    if (core->ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x05u;
        contra_set_player_horizontal_flip(core, player_index);
        return;
    }

    if (core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
    {
        contra_set_player_jump_sprite(core, player_index);
        return;
    }

    if ((sequence == 0x04u) || (sequence == 0x06u))
    {
        contra_set_player_death_sprite(core, player_index);
        return;
    }

    if (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        if (sequence == 0x01u)
        {
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x50u;
        }
        else if (sequence == 0x02u)
        {
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x54u;
        }
        else if (sequence == 0x05u)
        {
            core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] =
                (uint8_t)(core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] + 1u);
            if (core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] >= 0x0Bu)
            {
                core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
                core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
                    (uint8_t)(core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u);
            }
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] =
                (uint8_t)(0x57u + (core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] & 0x01u));
        }
        else if (sequence == 0x06u)
        {
            contra_set_player_death_sprite(core, player_index);
            return;
        }
        else
        {
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] =
                (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u) ? 0x52u : 0x51u;
        }
        contra_set_player_horizontal_flip(core, player_index);
        return;
    }

    if (sequence < 0x03u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = contra_player_small_seq_sprite_tbl[sequence];
        contra_set_player_horizontal_flip(core, player_index);
        return;
    }

    if (sequence == 0x03u)
    {
        uint8_t frame_type = contra_player_frame_sprite_type_tbl[core->ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] % 10u];
        const uint8_t *frame_table = contra_player_frame_sprite_tbl_00;

        if ((frame_type == 0x00u) && (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u))
        {
            frame_type = 0x01u;
        }

        if (frame_type == 0x01u)
        {
            frame_table = contra_player_frame_sprite_tbl_01;
        }
        else if (frame_type == 0x02u)
        {
            frame_table = contra_player_frame_sprite_tbl_02;
        }
        else if (frame_type == 0x03u)
        {
            frame_table = contra_player_frame_sprite_tbl_03;
        }

        core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] =
            frame_table[core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] % 6u];
        core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] =
            (uint8_t)(core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] + 1u);
        if ((core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] & 0x07u) == 0u)
        {
            core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
                (uint8_t)((core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u) % 6u);
        }

        contra_set_player_horizontal_flip(core, player_index);
        return;
    }

    core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x0Au;
}

static void contra_set_player_sprite_and_attrs(ContraCore *core, uint8_t player_index)
{
    uint8_t sprite_code;
    uint8_t attr;
    uint8_t effect_palette = contra_sprite_attr_start_tbl[player_index];

    contra_set_player_sprite(core, player_index);

    sprite_code = core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index];
    if (core->ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] != 0u)
    {
        sprite_code = 0x00u;
    }
    else if ((core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] != 0u) &&
             ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u))
    {
        sprite_code = 0x00u;
    }

    core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = sprite_code;

    if (core->ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] != 0u)
    {
        effect_palette = 0x02u;
    }
    else if ((core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] != 0u) &&
             (((uint8_t)(core->ram[CONTRA_RAM_FRAME_COUNTER] ^ contra_player_effect_xor_tbl[player_index]) & 0x04u) != 0u))
    {
        effect_palette = 0x05u;
    }

    attr = effect_palette;
    if ((core->ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] & 0x80u) != 0u)
    {
        attr |= 0x20u;
    }

    if (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] >= 0x0Cu)
    {
        attr |= 0x08u;
    }

    attr |= (uint8_t)(core->ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0xC8u);
    core->ram[CONTRA_RAM_SPRITE_ATTR + player_index] = attr;
}

static void contra_apply_gravity_set_player_y(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t previous_y = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    uint8_t visibility_delta = 0x00u;
    uint16_t y_sum;
    uint8_t jump_carry = 0u;

    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] + 0x23u);
    if (ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] < 0x23u)
    {
        ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] + 1u);
    }

    if ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) != 0u)
    {
        visibility_delta = 0xFFu;
    }

    ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] +
                  ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index]);
    if (ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] < ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index])
    {
        jump_carry = 0x01u;
    }

    y_sum = (uint16_t)previous_y +
        (uint16_t)ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] +
        (uint16_t)jump_carry;
    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = (uint8_t)y_sum;
    ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] + visibility_delta + (uint8_t)(y_sum >> 8u));
}

static uint8_t contra_adjust_bg_collision_y(uint8_t screen_y, uint8_t vertical_scroll)
{
    const uint16_t sum = (uint16_t)screen_y + (uint16_t)vertical_scroll;
    uint8_t adjusted = (uint8_t)sum;

    if (((sum >> 8u) != 0u) || (adjusted >= 0xF0u))
    {
        adjusted = (uint8_t)(adjusted + 0x10u);
    }

    return adjusted;
}

static uint8_t contra_get_x_velocity_d_pad_code(const ContraCore *core, uint8_t player_index)
{
    const uint8_t controller = core->ram[CONTRA_RAM_CONTROLLER_STATE + player_index];
    uint8_t motion_code = 0x00u;

    if ((controller & CONTRA_BUTTON_RIGHT) != 0u)
    {
        motion_code = 0x20u;
    }

    if ((controller & CONTRA_BUTTON_LEFT) != 0u)
    {
        motion_code = 0x40u;
    }

    return motion_code;
}

static void contra_set_player_x_velocity_from_code(ContraCore *core, uint8_t player_index, uint8_t motion_code)
{
    const uint8_t controller = core->ram[CONTRA_RAM_CONTROLLER_STATE + player_index];

    if ((motion_code & 0x40u) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0xFFu;
    }
    else if ((motion_code & 0x20u) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x01u;
    }
    else
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
    }

    if ((core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] != 0u) &&
        (((controller & CONTRA_BUTTON_DOWN) != 0u) ||
         ((core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] & 0x80u) != 0u)))
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
    }
}

static void contra_end_indoor_transition(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] = 0x00u;
    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_Y + player_index];
    ram[CONTRA_RAM_SPRITE_X_POS + player_index] = ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_X + player_index];
}

static void contra_set_player_indoor_advancing_velocity(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t sprite_x = ram[CONTRA_RAM_SPRITE_X_POS + player_index];

    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] = 0xFEu;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index] = 0x80u;
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_Y + player_index] = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_X + player_index] = sprite_x;
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = (sprite_x >= 0x80u) ? 0xFFu : 0x01u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x80u;
}

static void contra_start_indoor_room_advance(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    int player_index;

    for (player_index = 1; player_index >= 0; --player_index)
    {
        if ((ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] != 0u) ||
            (ram[CONTRA_RAM_PLAYER_STATE + player_index] != 0x01u) ||
            (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u) ||
            (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] != 0u))
        {
            continue;
        }

        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x05u;
        ram[CONTRA_RAM_INDOOR_SCROLL] = 0x01u;
        ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] = 0x00u;
        ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] = 0x01u;
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
        ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
        contra_set_player_indoor_advancing_velocity(core, (uint8_t)player_index);
    }
}

static bool contra_handle_indoor_player_up_input(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0x01u) ||
        ((ram[CONTRA_RAM_CONTROLLER_STATE + player_index] & CONTRA_BUTTON_UP) == 0u))
    {
        return false;
    }

    ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x01u;
    if (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0u)
    {
        if (ram[CONTRA_RAM_DEMO_MODE] != 0u)
        {
            return true;
        }

        ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] = 0x30u;
        contra_play_sound(core, 0x1Cu);
        return true;
    }

    contra_start_indoor_room_advance(core);
    return true;
}

static bool contra_update_indoor_player_transition(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint16_t sum;

    if (ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + player_index] != 0u)
    {
        ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + player_index] = 0x00u;
        ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] = 0x00u;
        contra_end_indoor_transition(core, player_index);
        contra_set_jump_status_and_y_velocity(core, player_index);
        return true;
    }

    if (ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] != 0u)
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x01u;
        ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] =
            (uint8_t)(ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] - 1u);
        if (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] != 0u)
        {
            ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] = 0x00u;
        }
        return true;
    }

    if (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] == 0u)
    {
        return false;
    }

    ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x05u;
    sum = (uint16_t)ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] +
        (uint16_t)ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index];
    ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] = (uint8_t)sum;
    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] +
                  ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] +
                  (uint8_t)(sum >> 8u));

    if (ram[CONTRA_RAM_INDOOR_SCROLL] >= 0x02u)
    {
        contra_end_indoor_transition(core, player_index);
    }

    return true;
}

/* Snap a screen Y to the standing height the game settles a grounded actor to on
   the outdoor horizontal level: align to the 16px grid (relative to the vertical
   scroll) and add the 4px fine offset that seats feet on the grass surface. */
static uint8_t contra_outdoor_landing_snap_y(const ContraCore *core, uint8_t reference_y)
{
    const uint8_t vertical_offset =
        (uint8_t)((core->ram[CONTRA_RAM_VERTICAL_SCROLL] & 0x0Fu) | 0xF0u);
    uint8_t landing_y = (uint8_t)(reference_y + vertical_offset);

    landing_y &= 0xF0u;
    landing_y = (uint8_t)(landing_y - vertical_offset);
    landing_y = (uint8_t)(landing_y + 0x04u);
    return landing_y;
}

static void contra_set_player_landing_y_offset(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t location_type = ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];

    if ((location_type & 0x80u) != 0u)
    {
        ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = 0xC9u;
        return;
    }

    if (location_type != 0u)
    {
        ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = 0xA8u;
        return;
    }

    ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
        contra_outdoor_landing_snap_y(core, ram[CONTRA_RAM_SPRITE_Y_POS + player_index]);
}

static void contra_land_player_on_ground(ContraCore *core, uint8_t player_index)
{
    contra_init_player_data(core, player_index);
    core->ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
}

static void contra_begin_player_edge_fall(ContraCore *core, uint8_t player_index, uint8_t edge_fall_code)
{
    uint8_t *const ram = core->ram;
    uint8_t freeze_y = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + 0x14u);

    ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] = edge_fall_code;

    if (freeze_y < ram[CONTRA_RAM_SPRITE_Y_POS + player_index])
    {
        freeze_y = 0xFFu;
    }

    ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index] = freeze_y;
}

static void contra_walk_player_off_ledge(ContraCore *core, uint8_t player_index)
{
    contra_begin_player_edge_fall(
        core,
        player_index,
        (core->ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u) ? 0x41u : 0x21u
    );
}

static void contra_check_player_ledge(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t collision_code;

    if ((ram[CONTRA_RAM_PLAYER_ON_ENEMY + player_index] != 0u) ||
        (ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] != 0u) ||
        (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u) ||
        (ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] != 0u) ||
        (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] != 0u))
    {
        return;
    }

    if ((ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] & 0x01u) != 0u)
    {
        ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] = 0x00u;
        return;
    }

    collision_code = contra_get_player_bg_collision_code(core, player_index);
    if (collision_code == 0u)
    {
        contra_walk_player_off_ledge(core, player_index);
        return;
    }

    if (collision_code == 0x02u)
    {
        ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x01u;
        return;
    }

    if (collision_code != 0x02u)
    {
        ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x00u;
    }
}

static void contra_update_player_edge_fall(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t motion_code;

    if (ram[CONTRA_RAM_SPRITE_Y_POS + player_index] >= ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index])
    {
        const uint8_t collision_code = contra_get_player_bg_collision_code(core, player_index);

        if (collision_code != 0u)
        {
            contra_set_player_landing_y_offset(core, player_index);
            contra_land_player_on_ground(core, player_index);
            return;
        }
    }

    contra_apply_gravity_set_player_y(core, player_index);

    if (ram[CONTRA_RAM_SPRITE_Y_POS + player_index] >= ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index])
    {
        motion_code = contra_get_x_velocity_d_pad_code(core, player_index);
        if (motion_code != 0u)
        {
            ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] =
                (uint8_t)((ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] & 0x9Fu) | motion_code);
        }
    }

    contra_set_player_x_velocity_from_code(core, player_index, ram[CONTRA_RAM_EDGE_FALL_CODE + player_index]);
}

static void contra_move_player_horizontally(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const int8_t x_velocity = (int8_t)ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index];
    const uint8_t screen_type = contra_get_level_screen_type(core);
    const uint8_t right_edge = (screen_type == 0u) ? 0xE6u : ((screen_type == 1u) ? 0xE0u : 0xD0u);
    const uint8_t left_edge = (screen_type == 0u) ? 0x1Au : ((screen_type == 1u) ? 0x20u : 0x30u);

    if (x_velocity > 0)
    {
        if ((ram[CONTRA_RAM_SPRITE_X_POS + player_index] < right_edge) &&
            !contra_player_has_solid_collision_ahead(core, player_index, 8))
        {
            ram[CONTRA_RAM_SPRITE_X_POS + player_index] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] + 1u);
        }
    }
    else if ((x_velocity < 0) &&
             (ram[CONTRA_RAM_SPRITE_X_POS + player_index] > left_edge) &&
             !contra_player_has_solid_collision_ahead(core, player_index, -8))
    {
        ram[CONTRA_RAM_SPRITE_X_POS + player_index] =
            (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] - 1u);
    }
}

static void contra_handle_player_fall_out(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const bool demo_mode = ram[CONTRA_RAM_DEMO_MODE] != 0u;
    const bool demo_life_floor_reached =
        demo_mode && (ram[CONTRA_RAM_P1_NUM_LIVES + player_index] <= 0x61u);

    contra_init_player_attributes(core, player_index);
    ram[CONTRA_RAM_PLAYER_STATE + player_index] = 0x00u;

    if (ram[CONTRA_RAM_P1_NUM_LIVES + player_index] == 0u)
    {
        ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] = 0x01u;
    }
    else if (!demo_life_floor_reached)
    {
        ram[CONTRA_RAM_P1_NUM_LIVES + player_index] =
            (uint8_t)(ram[CONTRA_RAM_P1_NUM_LIVES + player_index] - 1u);
    }
}

static void contra_kill_player(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    contra_play_sound(core, 0x52u);
    contra_init_player_data(core, player_index);
    ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] = 0x00u;
    ram[CONTRA_RAM_ELECTROCUTED_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPECIAL_SPRITE_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_DEATH_FLAG + player_index] = 0x01u;
    ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0xFDu;
    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x80u;
    ram[CONTRA_RAM_PLAYER_STATE + player_index] = 0x02u;
}

static void contra_set_player_aim_for_input(ContraCore *core, uint8_t player_index)
{
    uint8_t table_row;
    uint8_t dpad = (uint8_t)(core->ram[CONTRA_RAM_CONTROLLER_STATE + player_index] & 0x0Fu);

    if (core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
    {
        table_row = 0x20u;
    }
    else if (core->ram[CONTRA_RAM_PLAYER_AIM_PREV_FRAME + player_index] >= 0x05u)
    {
        table_row = 0x10u;
    }
    else
    {
        table_row = 0x00u;
    }

    core->ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] = contra_d_pad_player_aim_tbl[table_row + dpad];
}

static void contra_set_jump_status_and_y_velocity(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] =
        (ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u) ? 0x91u : 0x11u;
    ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u)
    {
        ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0xFBu;
        ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0xF0u;
    }
    else
    {
        ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0xFCu;
        ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x90u;
    }
}

static void contra_handle_player_jump(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t current_y_fast_velocity = ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index];
    uint8_t motion_code;

    if ((current_y_fast_velocity & 0x80u) != 0u)
    {
        if ((ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0u) &&
            (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
            (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u))
        {
            const uint8_t sample_y = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - 0x0Au);
            const uint8_t collision = contra_get_outdoor_horizontal_bg_collision(
                core,
                ram[CONTRA_RAM_SPRITE_X_POS + player_index],
                sample_y
            );

            if (collision == 0x80u)
            {
                ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0x00u;
                ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] = 0x00u;
            }
        }
    }
    else
    {
        bool should_check_collision = true;

        if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u)
        {
            if (current_y_fast_velocity < 0x01u)
            {
                should_check_collision = false;
            }
            else if (current_y_fast_velocity < 0x04u)
            {
                const uint8_t adjusted_y = contra_adjust_bg_collision_y(
                    ram[CONTRA_RAM_SPRITE_Y_POS + player_index],
                    ram[CONTRA_RAM_VERTICAL_SCROLL]
                );

                if ((adjusted_y & 0x0Fu) >= 0x08u)
                {
                    should_check_collision = false;
                }
            }
        }

        if (should_check_collision &&
            (contra_get_player_bg_collision_code(core, player_index) != 0u))
        {
            motion_code = contra_get_x_velocity_d_pad_code(core, player_index);
            if (motion_code != 0u)
            {
                ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] =
                    (uint8_t)((ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] & 0x9Fu) | motion_code);
            }

            contra_set_player_x_velocity_from_code(
                core,
                player_index,
                ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index]
            );
            contra_set_player_landing_y_offset(core, player_index);
            contra_land_player_on_ground(core, player_index);
            return;
        }
    }

    contra_apply_gravity_set_player_y(core, player_index);

    motion_code = contra_get_x_velocity_d_pad_code(core, player_index);
    if (motion_code != 0u)
    {
        ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] =
            (uint8_t)((ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] & 0x9Fu) | motion_code);
    }

    contra_set_player_x_velocity_from_code(
        core,
        player_index,
        ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index]
    );
}

static void contra_handle_d_pad(ContraCore *core, uint8_t player_index)
{
    const uint8_t controller = core->ram[CONTRA_RAM_CONTROLLER_STATE + player_index];
    const uint8_t water_state = core->ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index];

    if ((controller & CONTRA_BUTTON_RIGHT) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] =
            (((water_state & 0x80u) != 0u) || ((water_state != 0u) && ((controller & CONTRA_BUTTON_DOWN) != 0u)))
            ? 0x00u
            : 0x01u;
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x03u;
        return;
    }

    if ((controller & CONTRA_BUTTON_LEFT) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] =
            (((water_state & 0x80u) != 0u) || ((water_state != 0u) && ((controller & CONTRA_BUTTON_DOWN) != 0u)))
            ? 0x00u
            : 0xFFu;
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x03u;
        return;
    }

    core->ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
    if ((controller & CONTRA_BUTTON_DOWN) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x02u;
    }
    else if ((controller & CONTRA_BUTTON_UP) != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x01u;
    }
    else
    {
        core->ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x00u;
    }
}

static void contra_run_player_state_routine(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    uint8_t current_level = ram[CONTRA_RAM_CURRENT_LEVEL];

    if (current_level > 7u)
    {
        current_level = 7u;
    }

    switch (ram[CONTRA_RAM_PLAYER_STATE + player_index])
    {
        case 0x00u:
        {
            const uint8_t spawn_index = (uint8_t)(((uint8_t)(player_index << 2u)) + contra_level_spawn_position_index[current_level]);

            contra_init_player_attributes(core, player_index);
            ram[CONTRA_RAM_SPRITE_Y_POS + player_index] = contra_vertical_spawn_position[spawn_index];
            ram[CONTRA_RAM_SPRITE_X_POS + player_index] = contra_horizontal_spawn_position[spawn_index];
            ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] = 0x01u;
            ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x02u;
            ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] = 0x00u;
            ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] = 0x80u;
            ram[CONTRA_RAM_PLAYER_STATE + player_index] = 0x01u;
            break;
        }

        case 0x01u:
        {
            contra_set_player_aim_for_input(core, player_index);
            contra_check_player_ledge(core, player_index);
            contra_check_player_fire(core, player_index);

            if (contra_update_indoor_player_transition(core, player_index))
            {
            }
            else if (ram[CONTRA_RAM_EDGE_FALL_CODE + player_index] != 0u)
            {
                contra_update_player_edge_fall(core, player_index);
            }
            else if (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
            {
                contra_handle_player_jump(core, player_index);
            }
            else
            {
                if (!contra_handle_indoor_player_up_input(core, player_index))
                {
                    contra_handle_d_pad(core, player_index);
                }
                if ((ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] == 0u) &&
                    ((ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_index] & CONTRA_BUTTON_A) != 0u))
                {
                    if ((ram[CONTRA_RAM_CONTROLLER_STATE + player_index] &
                         (CONTRA_BUTTON_DOWN | CONTRA_BUTTON_LEFT | CONTRA_BUTTON_RIGHT)) == CONTRA_BUTTON_DOWN)
                    {
                        if (contra_can_player_drop_down(core, player_index))
                        {
                            contra_begin_player_edge_fall(core, player_index, 0x81u);
                        }
                    }
                    else
                    {
                        contra_set_jump_status_and_y_velocity(core, player_index);
                    }
                }
            }

            if ((ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] & 0x80u) != 0u)
            {
                ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
            }

            contra_move_player_horizontally(core, player_index);

            contra_set_player_sprite_and_attrs(core, player_index);
            ram[CONTRA_RAM_PLAYER_AIM_PREV_FRAME + player_index] = ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index];

            if ((ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] == 0u) &&
                (ram[CONTRA_RAM_SPRITE_Y_POS + player_index] >= 0xE8u))
            {
                contra_kill_player(core, player_index);
            }
            break;
        }

        case 0x02u:
        {
            const uint8_t screen_type = contra_get_level_screen_type(core);

            ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = contra_player_dead_sequence_tbl[screen_type];
            if (ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] != 0u)
            {
                ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] - 1u);
                if (ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] == 0u)
                {
                    contra_handle_player_fall_out(core, player_index);
                    break;
                }
            }
            else
            {
                int8_t x_velocity = contra_player_died_x_velocity_tbl[screen_type];

                ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = (uint8_t)x_velocity;
                if (ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u)
                {
                    ram[CONTRA_RAM_PLAYER_DEATH_FLAG + player_index] |= 0x02u;
                    x_velocity = (int8_t)(-x_velocity);
                    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = (uint8_t)x_velocity;
                }

                if (((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) == 0u) &&
                    (ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] >= 0x02u))
                {
                    if (ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] == 0x01u)
                    {
                        ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x40u;
                    }
                    else
                    {
                        const uint8_t collision_code = contra_get_player_bg_collision_code(core, player_index);

                        if ((collision_code != 0u) && (collision_code != 0x02u))
                        {
                            contra_set_player_landing_y_offset(core, player_index);
                            ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x40u;
                        }
                    }
                }

                contra_apply_gravity_set_player_y(core, player_index);
                contra_move_player_horizontally(core, player_index);
            }

            contra_set_player_sprite_and_attrs(core, player_index);
            break;
        }

        default:
            contra_set_player_sprite_and_attrs(core, player_index);
            break;
    }
}

static void contra_handle_invincibility_and_weapon_strength(ContraCore *core, uint8_t player_index)
{
    contra_run_player_state_routine(core, player_index);

    if (core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] != 0u)
    {
        core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] =
            (uint8_t)(core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] - 1u);
    }
    else if (core->ram[CONTRA_RAM_PLAYER_STATE + player_index] == 0x01u)
    {
        core->ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] = 0x01u;
    }

    if ((core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] != 0u) &&
        ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) == 0u))
    {
        core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] =
            (uint8_t)(core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] - 1u);
    }

    if (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] =
            (uint8_t)(core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] - 1u);
    }

    core->ram[CONTRA_RAM_PLAYER_FAST_X_VEL_BOOST + player_index] = 0x00u;

    {
        const uint8_t weapon = (uint8_t)(core->ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index] & 0x07u);
        const uint8_t strength = contra_weapon_strength_tbl[(weapon < 5u) ? weapon : 0u];

        if (strength >= core->ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH])
        {
            core->ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] = strength;
        }
    }
}

static bool contra_is_native_level_1_active(const ContraCore *core)
{
    const uint8_t game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];

    return ((game_routine == 0x05u) ||
            ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0u) &&
        (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u);
}

static bool contra_is_native_level_2_active(const ContraCore *core)
{
    const uint8_t game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];

    return ((game_routine == 0x05u) ||
            ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) &&
        ((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) ||
         (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u));
}

static bool contra_is_native_combat_active(const ContraCore *core)
{
    return contra_is_native_level_1_active(core) || contra_is_native_level_2_active(core);
}

static bool contra_rects_overlap(
    int left_a,
    int top_a,
    int right_a,
    int bottom_a,
    int left_b,
    int top_b,
    int right_b,
    int bottom_b
)
{
    return (left_a < right_b) && (right_a > left_b) && (top_a < bottom_b) && (bottom_a > top_b);
}

static bool contra_native_level_1_enemy_can_hit_player(const ContraNativeEnemy *enemy)
{
    if (enemy->active == 0u)
    {
        return false;
    }

    switch (enemy->type)
    {
        case 0x02u:
            return enemy->state != CONTRA_NATIVE_LEVEL1_STATE_WAIT;

        case 0x04u:
        case 0x07u:
            return enemy->state == CONTRA_NATIVE_LEVEL1_STATE_ACTIVE;

        case 0x05u:
        case 0x06u:
        case 0x10u:
            return true;

        default:
            return false;
    }
}

static void contra_native_level_1_enemy_bounds(
    const ContraNativeEnemy *enemy,
    int *left,
    int *top,
    int *right,
    int *bottom
)
{
    switch (enemy->type)
    {
        case 0x02u:
        case 0x04u:
        case 0x07u:
        case 0x10u:
            *left = (int)enemy->x - 12;
            *top = (int)enemy->y - 12;
            *right = *left + 32;
            *bottom = *top + 32;
            break;

        case 0x03u:
            *left = (int)enemy->x - 8;
            *top = (int)enemy->y - 8;
            *right = *left + 16;
            *bottom = *top + 16;
            break;

        default:
            *left = (int)enemy->x - 8;
            *top = (int)enemy->y - 16;
            *right = *left + 16;
            *bottom = *top + 24;
            break;
    }
}

static void contra_native_level_1_player_bounds(
    const ContraCore *core,
    uint8_t player_index,
    int *left,
    int *top,
    int *right,
    int *bottom
)
{
    int player_top = (int)core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - 8;
    int player_height = 24;

    if (core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
    {
        player_top += 2;
        player_height = 20;
    }
    else if (core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] == 0x17u)
    {
        player_top += 8;
        player_height = 16;
    }

    *left = (int)core->ram[CONTRA_RAM_SPRITE_X_POS + player_index] - 6;
    *top = player_top;
    *right = *left + 12;
    *bottom = *top + player_height;
}

static void contra_clear_enemy_projectile(ContraNativeProjectile *projectile)
{
    memset(projectile, 0, sizeof(*projectile));
}

static void contra_update_flying_capsule_axis_velocity(
    uint8_t current_pos,
    uint8_t origin_pos,
    int8_t *fast_velocity,
    uint8_t *fract_velocity
)
{
    const int16_t delta = (int16_t)(uint16_t)current_pos - (int16_t)(uint16_t)origin_pos;
    int16_t velocity = (int16_t)(((int16_t)(*fast_velocity) * 256) + (int16_t)(uint16_t)(*fract_velocity));

    velocity = (int16_t)(velocity - (delta << 1));
    *fast_velocity = (int8_t)((uint16_t)velocity >> 8u);
    *fract_velocity = (uint8_t)velocity;
}

static uint8_t contra_apply_flying_capsule_axis_velocity(
    uint8_t position,
    int8_t fast_velocity,
    uint8_t fract_velocity,
    uint8_t *accumulator
)
{
    const uint16_t sum = (uint16_t)(*accumulator) + (uint16_t)fract_velocity;

    *accumulator = (uint8_t)sum;
    return (uint8_t)(
        (uint16_t)position +
        (uint16_t)(uint8_t)fast_velocity +
        (uint16_t)(sum >> 8u)
    );
}

static bool contra_find_level_1_floor_y_below(
    const ContraCore *core,
    uint8_t world_x,
    uint8_t start_y,
    uint8_t *floor_y_out
)
{
    uint8_t floor_test_y;

    for (floor_test_y = 0x10u; floor_test_y < 0xF0u; floor_test_y = (uint8_t)(floor_test_y + 0x10u))
    {
        const uint8_t floor_collision = contra_get_outdoor_horizontal_bg_collision(core, world_x, floor_test_y);
        const uint8_t candidate_y = (uint8_t)(floor_test_y - 0x10u);

        if ((candidate_y < start_y) ||
            (floor_collision == 0u) ||
            (floor_collision == 0x80u) ||
            (candidate_y >= 0xE0u))
        {
            continue;
        }

        *floor_y_out = candidate_y;
        return true;
    }

    return false;
}

/* Find the screen Y of the top of the first solid floor tile at or below
   start_y, scanning at the 8px collision-tile resolution (the coarse finder
   above steps 16px). Returns false if no solid ground is found. */
static bool contra_find_outdoor_floor_surface_y(
    const ContraCore *core,
    uint8_t world_x,
    uint8_t start_y,
    uint8_t *surface_y_out
)
{
    uint8_t test_y = (start_y < 0x10u) ? 0x10u : start_y;

    /* Align the first probe to the 8px tile grid (tiles begin at the 0x10
       status-bar offset) so each step samples a distinct floor tile. */
    test_y = (uint8_t)(((uint8_t)(test_y - 0x10u) & 0xF8u) + 0x10u);

    while (test_y < 0xF0u)
    {
        if (contra_get_outdoor_horizontal_bg_collision(core, world_x, test_y) == 0x01u)
        {
            *surface_y_out = test_y;
            return true;
        }

        test_y = (uint8_t)(test_y + 0x08u);
    }

    return false;
}

/* Seat a grounded enemy at the same height the player would settle to on the
   floor beneath it. The enemy and player share the +0x10 foot anchor, so
   running the detected floor surface through the player's landing snap keeps
   ground enemies aligned with the player instead of sitting below the floor. */
static void contra_snap_native_enemy_to_outdoor_floor(ContraCore *core, ContraNativeEnemy *enemy)
{
    uint8_t surface_y;

    if ((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u))
    {
        return;
    }

    if (contra_find_outdoor_floor_surface_y(
            core,
            (uint8_t)enemy->x,
            (uint8_t)enemy->y,
            &surface_y))
    {
        enemy->y = (int16_t)contra_outdoor_landing_snap_y(core, (uint8_t)(surface_y - 0x10u));
    }
}

static void contra_spawn_level_1_weapon_item(ContraNativeEnemy *enemy)
{
    enemy->type = 0x00u;
    enemy->state = 0x00u;
    enemy->timer = 0x00u;
    enemy->cooldown = 0x00u;
    enemy->flags = 0x00u;
    enemy->hp = 0x01u;
    enemy->sprite_code = contra_weapon_item_sprite_code_tbl[enemy->attrs & 0x07u];
    enemy->sprite_attr = 0x05u;
    enemy->vx = 0;
    enemy->vy = -3;
    enemy->x_frac = 0x80u;
    enemy->y_frac = 0x00u;
    enemy->x_accum = 0x00u;
    enemy->y_accum = 0x00u;
}

static void contra_add_to_enemy_y_fractional_velocity(ContraNativeEnemy *enemy, uint8_t delta)
{
    const uint16_t sum = (uint16_t)enemy->y_frac + (uint16_t)delta;

    enemy->y_frac = (uint8_t)sum;
    enemy->vy = (int8_t)((uint8_t)enemy->vy + (uint8_t)(sum >> 8u));
}

static uint8_t contra_get_level_1_explosion_frame_count(uint8_t explosion_type)
{
    return (explosion_type == CONTRA_NATIVE_LEVEL1_EXPLOSION_CLOUD) ? 4u : 3u;
}

static uint8_t contra_get_level_1_explosion_sprite_code(uint8_t explosion_type, uint8_t frame)
{
    if (explosion_type == CONTRA_NATIVE_LEVEL1_EXPLOSION_CLOUD)
    {
        return contra_level_1_explosion_cloud_sprite_tbl[frame];
    }

    return contra_level_1_explosion_ring_sprite_tbl[frame];
}

static uint8_t contra_get_level_1_enemy_explosion_type(uint8_t enemy_type)
{
    switch (enemy_type)
    {
        case 0x02u:
        case 0x04u:
        case 0x07u:
        case 0x10u:
            return CONTRA_NATIVE_LEVEL1_EXPLOSION_RING;

        default:
            return CONTRA_NATIVE_LEVEL1_EXPLOSION_CLOUD;
    }
}

static void contra_spawn_level_1_enemy_explosion(ContraNativeEnemy *enemy, uint8_t explosion_type)
{
    const int16_t explosion_x = enemy->x;
    const int16_t explosion_y = enemy->y;

    memset(enemy, 0, sizeof(*enemy));
    enemy->active = 0x01u;
    enemy->type = CONTRA_NATIVE_LEVEL1_TYPE_EXPLOSION;
    enemy->flags = explosion_type;
    enemy->timer = CONTRA_NATIVE_LEVEL1_EXPLOSION_FRAME_DELAY;
    enemy->sprite_code = contra_get_level_1_explosion_sprite_code(explosion_type, 0u);
    enemy->x = explosion_x;
    enemy->y = explosion_y;
}

static void contra_handle_level_1_enemy_destroyed(ContraCore *core, ContraNativeEnemy *enemy)
{
    const uint8_t enemy_type = enemy->type;

    if (enemy_type == 0x03u)
    {
        contra_spawn_level_1_weapon_item(enemy);
        return;
    }

    if (enemy_type == 0x02u)
    {
        uint16_t tile_ppu_addr;
        uint16_t attr_ppu_addr;

        contra_calculate_level_1_nametable_update_supertile_ppu_addr(
            core,
            (int)enemy->x,
            (int)enemy->y,
            &tile_ppu_addr,
            &attr_ppu_addr
        );
        contra_write_level_1_nametable_update_supertile_to_ppu_addr(
            core,
            tile_ppu_addr,
            attr_ppu_addr,
            0x02u
        );
        core->level1_weapon_box_restore_timer = 0x3Du;
        core->level1_weapon_box_restore_x = (int16_t)tile_ppu_addr;
        core->level1_weapon_box_restore_y = (int16_t)attr_ppu_addr;
    }

    contra_spawn_level_1_enemy_explosion(enemy, contra_get_level_1_enemy_explosion_type(enemy_type));
}

static void contra_collect_level_1_weapon_item(ContraCore *core, uint8_t player_index, uint8_t item_type)
{
    uint8_t *const ram = core->ram;
    const uint8_t current_weapon = ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index];
    const uint8_t current_weapon_type = (uint8_t)(current_weapon & 0x0Fu);

    item_type &= 0x07u;
    if (item_type == 0x00u)
    {
        ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index] = (uint8_t)(current_weapon | 0x10u);
        return;
    }

    if (item_type < 0x05u)
    {
        const uint8_t rapid_flag = (current_weapon_type == item_type) ? (uint8_t)(current_weapon & 0x10u) : 0x00u;

        ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index] = (uint8_t)(rapid_flag | item_type);
        return;
    }

    if (item_type == 0x05u)
    {
        ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] = 0x80u;
        return;
    }

    {
        size_t enemy_index;

        for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
        {
            if (core->enemies[enemy_index].type != 0x00u)
            {
                memset(&core->enemies[enemy_index], 0, sizeof(core->enemies[enemy_index]));
            }
        }

        memset(core->enemy_projectiles, 0, sizeof(core->enemy_projectiles));
    }
}

static void contra_select_level_1_projectile_velocity(int target_dx, int target_dy, int8_t *vx_out, int8_t *vy_out)
{
    const int abs_dx = abs(target_dx);
    const int abs_dy = abs(target_dy);

    if ((target_dx == 0) && (target_dy == 0))
    {
        *vx_out = -3;
        *vy_out = 0;
        return;
    }

    if (abs_dx >= (abs_dy * 2))
    {
        *vx_out = (int8_t)((target_dx < 0) ? -3 : 3);
        *vy_out = 0;
        return;
    }

    if (abs_dy >= (abs_dx * 2))
    {
        *vx_out = 0;
        *vy_out = (int8_t)((target_dy < 0) ? -3 : 3);
        return;
    }

    *vx_out = (int8_t)((target_dx < 0) ? -2 : 2);
    *vy_out = (int8_t)((target_dy < 0) ? -2 : 2);
}

static bool contra_try_spawn_level_1_enemy_projectile(
    ContraCore *core,
    int spawn_x,
    int spawn_y,
    int8_t vx,
    int8_t vy,
    uint8_t owner,
    uint8_t sprite_code,
    uint8_t sprite_attr
)
{
    size_t projectile_index;

    for (projectile_index = 0u; projectile_index < CONTRA_NATIVE_MAX_ENEMY_PROJECTILES; ++projectile_index)
    {
        ContraNativeProjectile *const projectile = &core->enemy_projectiles[projectile_index];

        if (projectile->active != 0u)
        {
            continue;
        }

        memset(projectile, 0, sizeof(*projectile));
        projectile->active = 0x01u;
        projectile->damage = 0x01u;
        projectile->sprite_code = sprite_code;
        projectile->sprite_attr = sprite_attr;
        projectile->owner = owner;
        projectile->timer = 0x70u;
        projectile->x = (int16_t)spawn_x;
        projectile->y = (int16_t)spawn_y;
        projectile->vx = vx;
        projectile->vy = vy;
        return true;
    }

    return false;
}

static bool contra_try_spawn_level_1_regular_projectile(
    ContraCore *core,
    int spawn_x,
    int spawn_y,
    int8_t vx,
    int8_t vy,
    uint8_t owner
)
{
    return contra_try_spawn_level_1_enemy_projectile(
        core,
        spawn_x,
        spawn_y,
        vx,
        vy,
        owner,
        0x1Eu,
        0x03u
    );
}

static uint8_t contra_level_1_boss_bomb_turret_sprite_code(const ContraNativeEnemy *enemy)
{
    static const uint8_t boss_bomb_turret_supertile_tbl[6] = {
        0x29u, 0x26u, 0x2Au, 0x27u, 0x2Bu, 0x28u
    };
    uint8_t frame_offset = (uint8_t)(enemy->flags & 0x06u);

    if (frame_offset > 0x04u)
    {
        frame_offset = 0x00u;
    }

    return boss_bomb_turret_supertile_tbl[frame_offset + (enemy->attrs & 0x01u)];
}

static int contra_level_1_boss_bomb_turret_render_x(const ContraNativeEnemy *enemy)
{
    return (int)enemy->x - (((enemy->attrs & 0x01u) != 0u) ? 8 : 0);
}

static bool contra_try_spawn_level_1_boss_bomb_turret_projectile(
    ContraCore *core,
    const ContraNativeEnemy *enemy
)
{
    static const int8_t boss_bomb_turret_bomb_velocity_tbl[4] = {-1, -3, -5, -7};
    const int8_t vx = boss_bomb_turret_bomb_velocity_tbl[core->ram[CONTRA_RAM_RANDOM_NUM] & 0x03u];

    return contra_try_spawn_level_1_enemy_projectile(
        core,
        (int)enemy->x - 8,
        (int)enemy->y,
        vx,
        -1,
        enemy->type,
        0x21u,
        0x02u
    );
}

static bool contra_try_fire_level_1_turret(
    ContraCore *core,
    const ContraNativeEnemy *enemy,
    bool has_target,
    int target_dx,
    int target_dy
)
{
    int8_t vx;
    int8_t vy;
    int spawn_x;
    int spawn_y;

    if ((!has_target) || (core->ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] == 0u))
    {
        return false;
    }

    contra_select_level_1_projectile_velocity(target_dx, target_dy, &vx, &vy);

    /*
     * The turret is rendered with its supertile snapped to the world's
     * 8-pixel grid (see contra_render_level_1_nametable_update_supertile).
     * Spawn bullets from the same snapped center so the muzzle position
     * tracks the visible turret instead of the unaligned enemy->x.
     */
    {
        const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
        const int aligned_x = ((((int)enemy->x - 12) + scroll_offset) & ~7) - scroll_offset + 12;
        const int aligned_y = (((int)enemy->y - 12) & ~7) + 12;

        if (enemy->type == 0x07u)
        {
            static const int8_t red_turret_muzzle_offsets[3][2] = {
                {-14, 0},
                {-14, -8},
                {-8, -16}
            };
            uint8_t muzzle_index = 0u;

            if (target_dy < -24)
            {
                muzzle_index = 2u;
            }
            else if (target_dy < -4)
            {
                muzzle_index = 1u;
            }

            spawn_x = aligned_x + red_turret_muzzle_offsets[muzzle_index][0];
            spawn_y = aligned_y + red_turret_muzzle_offsets[muzzle_index][1];
            return contra_try_spawn_level_1_regular_projectile(core, spawn_x, spawn_y, vx, vy, enemy->type);
        }

        spawn_x = aligned_x + 4;
        spawn_y = aligned_y + 8;
    }

    if (vx > 0)
    {
        spawn_x += 8;
    }
    else if (vx < 0)
    {
        spawn_x -= 8;
    }

    if (vy > 0)
    {
        spawn_y += 8;
    }
    else if (vy < 0)
    {
        spawn_y -= 8;
    }

    return contra_try_spawn_level_1_regular_projectile(core, spawn_x, spawn_y, vx, vy, enemy->type);
}

static void contra_update_native_enemy_projectiles(ContraCore *core)
{
    size_t projectile_index;

    if (!contra_is_native_combat_active(core))
    {
        return;
    }

    for (projectile_index = 0u; projectile_index < CONTRA_NATIVE_MAX_ENEMY_PROJECTILES; ++projectile_index)
    {
        ContraNativeProjectile *const projectile = &core->enemy_projectiles[projectile_index];

        if (projectile->active == 0u)
        {
            continue;
        }

        if (core->ram[CONTRA_RAM_FRAME_SCROLL] != 0u)
        {
            projectile->x = (int16_t)(projectile->x - (int)core->ram[CONTRA_RAM_FRAME_SCROLL]);
        }

        projectile->x = (int16_t)((int)projectile->x + projectile->vx);
        projectile->y = (int16_t)((int)projectile->y + projectile->vy);

        if (projectile->timer != 0u)
        {
            projectile->timer = (uint8_t)(projectile->timer - 1u);
        }

        if ((projectile->timer == 0u) ||
            (projectile->x < -16) ||
            (projectile->x > ((int)CONTRA_FRAMEBUFFER_WIDTH + 16)) ||
            (projectile->y < -16) ||
            (projectile->y > ((int)CONTRA_FRAMEBUFFER_HEIGHT + 16)))
        {
            contra_clear_enemy_projectile(projectile);
        }
    }
}

static void contra_check_native_player_collisions(ContraCore *core)
{
    uint8_t player_index;
    const bool level_1_active = contra_is_native_level_1_active(core);

    if (!contra_is_native_combat_active(core))
    {
        return;
    }

    for (player_index = 0u; player_index < 2u; ++player_index)
    {
        const bool new_life_invincible =
            core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + player_index] != 0u;
        const bool barrier_invincible =
            core->ram[CONTRA_RAM_INVINCIBILITY_TIMER + player_index] != 0u;
        int player_left;
        int player_top;
        int player_right;
        int player_bottom;
        size_t enemy_index;
        bool player_hit = false;

        if ((core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] != 0u) ||
            (core->ram[CONTRA_RAM_PLAYER_STATE + player_index] != 0x01u) ||
            (core->ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] != 0u))
        {
            continue;
        }

        contra_native_level_1_player_bounds(
            core,
            player_index,
            &player_left,
            &player_top,
            &player_right,
            &player_bottom
        );

        if (level_1_active)
        {
            for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
            {
                ContraNativeEnemy *const enemy = &core->enemies[enemy_index];
                int enemy_left;
                int enemy_top;
                int enemy_right;
                int enemy_bottom;

                if (enemy->active == 0u)
                {
                    continue;
                }

                contra_native_level_1_enemy_bounds(enemy, &enemy_left, &enemy_top, &enemy_right, &enemy_bottom);
                if (!contra_rects_overlap(
                        player_left,
                        player_top,
                        player_right,
                        player_bottom,
                        enemy_left,
                        enemy_top,
                        enemy_right,
                        enemy_bottom))
                {
                    continue;
                }

                if (enemy->type == 0x00u)
                {
                    contra_collect_level_1_weapon_item(core, player_index, enemy->attrs);
                    memset(enemy, 0, sizeof(*enemy));
                    continue;
                }

                if (!contra_native_level_1_enemy_can_hit_player(enemy))
                {
                    continue;
                }

                if (new_life_invincible)
                {
                    continue;
                }

                if (barrier_invincible)
                {
                    contra_handle_level_1_enemy_destroyed(core, enemy);
                    continue;
                }

                contra_kill_player(core, player_index);
                player_hit = true;
                break;
            }
        }

        if (player_hit)
        {
            continue;
        }

        for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMY_PROJECTILES; ++enemy_index)
        {
            ContraNativeProjectile *const projectile = &core->enemy_projectiles[enemy_index];
            const int projectile_left = (int)projectile->x - 4;
            const int projectile_top = (int)projectile->y - 4;
            const int projectile_right = projectile_left + 8;
            const int projectile_bottom = projectile_top + 8;

            if (projectile->active == 0u)
            {
                continue;
            }

            if (!contra_rects_overlap(
                    player_left,
                    player_top,
                    player_right,
                    player_bottom,
                    projectile_left,
                    projectile_top,
                    projectile_right,
                    projectile_bottom))
            {
                continue;
            }

            contra_clear_enemy_projectile(projectile);
            if ((core->ram[CONTRA_RAM_DEMO_MODE] != 0u) &&
                (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) &&
                (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) &&
                (core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x00u))
            {
                continue;
            }

            if (new_life_invincible || barrier_invincible)
            {
                continue;
            }

            contra_kill_player(core, player_index);
            break;
        }
    }
}

static void contra_load_bank_2_set_players_paused_sprite_attr(ContraCore *core)
{
    contra_set_player_sprite_and_attrs(core, 0u);
    contra_set_player_sprite_and_attrs(core, 1u);
}

static void contra_apply_outdoor_horizontal_frame_scroll(ContraCore *core, uint8_t active_players)
{
    uint8_t *const ram = core->ram;
    const uint8_t trigger_x = (active_players == 0x03u) ? 0xB0u : 0x80u;
    int scroller = -1;

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ||
        ((uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] | ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01]) != 0u) ||
        ((ram[CONTRA_RAM_LEVEL_STOP_SCROLL] & 0x80u) != 0u))
    {
        return;
    }

    if (((active_players & 0x01u) != 0u) &&
        (ram[CONTRA_RAM_PLAYER_STATE + 0u] == 0x01u) &&
        (ram[CONTRA_RAM_PLAYER_X_VELOCITY + 0u] == 0x01u) &&
        (ram[CONTRA_RAM_SPRITE_X_POS + 0u] > trigger_x))
    {
        scroller = 0;
    }

    if (((active_players & 0x02u) != 0u) &&
        (ram[CONTRA_RAM_PLAYER_STATE + 1u] == 0x01u) &&
        (ram[CONTRA_RAM_PLAYER_X_VELOCITY + 1u] == 0x01u) &&
        (ram[CONTRA_RAM_SPRITE_X_POS + 1u] > trigger_x))
    {
        if ((scroller < 0) || (ram[CONTRA_RAM_SPRITE_X_POS + 1u] > ram[CONTRA_RAM_SPRITE_X_POS + (size_t)scroller]))
        {
            scroller = 1;
        }
    }

    if (scroller < 0)
    {
        return;
    }

    if ((active_players == 0x03u) && (ram[CONTRA_RAM_SPRITE_X_POS + (size_t)(scroller ^ 1)] < 0x21u))
    {
        ram[CONTRA_RAM_PLAYER_X_VELOCITY + (size_t)scroller] = 0x00u;
        if (ram[CONTRA_RAM_SPRITE_X_POS + (size_t)scroller] > trigger_x)
        {
            ram[CONTRA_RAM_SPRITE_X_POS + (size_t)scroller] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + (size_t)scroller] - 1u);
        }
        return;
    }

    ram[CONTRA_RAM_FRAME_SCROLL] = 0x01u;
    ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + (size_t)scroller] = 0x01u;
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + (size_t)scroller] = 0x00u;

    if (ram[CONTRA_RAM_SPRITE_X_POS + (size_t)scroller] != 0u)
    {
        ram[CONTRA_RAM_SPRITE_X_POS + (size_t)scroller] =
            (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + (size_t)scroller] - 1u);
    }

    if (active_players == 0x03u)
    {
        const size_t other_player = (size_t)(scroller ^ 1);

        if (ram[CONTRA_RAM_SPRITE_X_POS + other_player] != 0u)
        {
            ram[CONTRA_RAM_SPRITE_X_POS + other_player] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + other_player] - 1u);
        }
    }
}

static void contra_load_bank_3_handle_scroll(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t scroll_pixels = (uint8_t)(ram[CONTRA_RAM_FRAME_SCROLL] + ram[CONTRA_RAM_TANK_AUTO_SCROLL]);

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) &&
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u))
    {
        if (ram[CONTRA_RAM_INDOOR_SCROLL] == 0u)
        {
            return;
        }

        if (ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] == 0u)
        {
            ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
            ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_LOW_BYTE] = 0x00u;
            ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = core->ram[CONTRA_RAM_DEMO_MODE] != 0u ? 0x08u : 0x20u;
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] ^= 0x04u;
            ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] ^= 0x04u;
            contra_load_supertiles_screen_indexes(
                core,
                (uint8_t)((ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] * 4u) + ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET])
            );
            return;
        }

        ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] =
            (uint8_t)(ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] - 1u);
        if (ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] != 0u)
        {
            return;
        }

        ram[CONTRA_RAM_INDOOR_SCROLL] = 0x02u;
        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] =
            (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u);
        if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] >=
            (core->ram[CONTRA_RAM_DEMO_MODE] != 0u ? 0x03u : 0x04u))
        {
            ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x00u;
            ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] = 0x00u;
            ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
            ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
            ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 1u);

            if (ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == ram[CONTRA_RAM_LEVEL_STOP_SCROLL])
            {
                ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x80u;
                ram[CONTRA_RAM_VERTICAL_SCROLL] = 0xE0u;
                contra_load_alternate_graphics(core);
                contra_init_apu_channels(core);
                contra_play_sound(core, 0x42u);
            }
        }

        contra_load_supertiles_screen_indexes(
            core,
            (uint8_t)((ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] * 4u) + ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET])
        );
        ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x0Cu;
        contra_load_palettes_color_to_cpu(core, 0x20u);
        ram[CONTRA_RAM_PPUCTRL_SETTINGS] ^= 0x01u;
        return;
    }

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ||
        (scroll_pixels == 0u))
    {
        return;
    }

    while (scroll_pixels-- != 0u)
    {
        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] =
            (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u);

        if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] == 0u)
        {
            ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 1u);
            ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
            ram[CONTRA_RAM_PPUCTRL_SETTINGS] ^= 0x01u;

            if (ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == ram[CONTRA_RAM_LEVEL_ALT_GRAPHICS_POS])
            {
                ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x01u;
                contra_load_alternate_graphics(core);
            }
        }

        if ((ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] & 0x07u) == 0u)
        {
            contra_schedule_horizontal_level_column_write(core);
            contra_advance_horizontal_level_ppu_column(core);
        }
        else if ((ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] & 0x0Fu) == 0x03u)
        {
            contra_schedule_horizontal_level_column_attributes_write(core);
        }

        ram[CONTRA_RAM_HORIZONTAL_SCROLL] =
            (uint8_t)(ram[CONTRA_RAM_HORIZONTAL_SCROLL] + 1u);
    }
}

static void contra_destroy_level_2_wall_core(ContraCore *core, ContraNativeEnemy *enemy)
{
    if (core->ram[CONTRA_RAM_WALL_CORE_REMAINING] != 0u)
    {
        core->ram[CONTRA_RAM_WALL_CORE_REMAINING] =
            (uint8_t)(core->ram[CONTRA_RAM_WALL_CORE_REMAINING] - 1u);
    }

    if (core->ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0u)
    {
        enemy->state = 0x08u;
        enemy->timer = 0x04u;
        enemy->cooldown = 0x03u;
        enemy->flags = 0x00u;
        enemy->sprite_code = 0x00u;
    }
    else
    {
        memset(enemy, 0, sizeof(*enemy));
    }
}

static void contra_load_bank_0_exe_all_enemy_routine(ContraCore *core)
{
    size_t enemy_index;

    if ((core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) &&
        ((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) ||
         (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u)))
    {
        const bool boss_room = (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u);
        bool grenade_launcher_active = false;

        for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
        {
            const ContraNativeEnemy *const enemy = &core->enemies[enemy_index];

            grenade_launcher_active = grenade_launcher_active ||
                ((enemy->active != 0u) && (enemy->type == 0x17u));
        }

        for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
        {
            ContraNativeEnemy *const enemy = &core->enemies[enemy_index];

            if (enemy->active == 0u)
            {
                continue;
            }

            switch (enemy->type)
            {
                case 0x08u:
                    if (!boss_room)
                    {
                        break;
                    }

                    if (enemy->timer != 0u)
                    {
                        enemy->timer = (uint8_t)(enemy->timer - 1u);
                        break;
                    }

                    if (enemy->state == 0x01u)
                    {
                        static const int8_t cannon_vx[3] = {-2, 0, 2};
                        static const int8_t cannon_vy[3] = {2, 3, 2};
                        static const int8_t cannon_x_offset[3] = {-8, 0, 8};
                        unsigned shot_index;

                        for (shot_index = 0u; shot_index < 3u; ++shot_index)
                        {
                            (void)contra_try_spawn_level_1_regular_projectile(
                                core,
                                (int)enemy->x + cannon_x_offset[shot_index],
                                (int)enemy->y + 8,
                                cannon_vx[shot_index],
                                cannon_vy[shot_index],
                                enemy->type
                            );
                        }

                        enemy->state = 0x02u;
                        enemy->timer = 0x40u;
                    }
                    else
                    {
                        enemy->state = 0x01u;
                        enemy->timer = 0x50u;
                    }
                    break;

                case 0x13u:
                    enemy->sprite_code = 0x84u;
                    break;

                case 0x14u:
                    if (enemy->state == 0x01u)
                    {
                        if (enemy->timer != 0u)
                        {
                            enemy->timer = (uint8_t)(enemy->timer - 1u);
                        }

                        if (enemy->timer == 0u)
                        {
                            enemy->state = 0x03u;
                            enemy->timer = ((core->ram[CONTRA_RAM_DEMO_MODE] != 0u) &&
                                            (core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x00u))
                                ? 0x01u
                                : 0x40u;
                            enemy->sprite_code = 0x87u;
                        }
                    }
                    else if (enemy->state == 0x03u)
                    {
                        if ((core->ram[CONTRA_RAM_DEMO_MODE] != 0u) &&
                            (core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x00u))
                        {
                            if (enemy->timer != 0u)
                            {
                                enemy->timer = (uint8_t)(enemy->timer - 1u);
                            }

                            if (enemy->timer == 0u)
                            {
                                if (enemy->hp != 0u)
                                {
                                    enemy->hp = (uint8_t)(enemy->hp - 1u);
                                }

                                if (enemy->hp == 0u)
                                {
                                    contra_destroy_level_2_wall_core(core, enemy);
                                }
                                else
                                {
                                    enemy->timer = 0x01u;
                                }
                            }
                        }
                    }
                    else if (enemy->state == 0x08u)
                    {
                        if (enemy->timer != 0u)
                        {
                            enemy->timer = (uint8_t)(enemy->timer - 1u);
                        }

                        if (enemy->timer == 0u)
                        {
                            enemy->flags = (uint8_t)(enemy->flags | (uint8_t)(1u << enemy->cooldown));
                            if (enemy->cooldown == 0u)
                            {
                                enemy->state = 0x09u;
                                enemy->timer = 0x10u;
                            }
                            else
                            {
                                enemy->cooldown = (uint8_t)(enemy->cooldown - 1u);
                                enemy->timer = 0x01u;
                            }
                        }
                    }
                    else if (enemy->state == 0x09u)
                    {
                        if (enemy->timer != 0u)
                        {
                            enemy->timer = (uint8_t)(enemy->timer - 1u);
                        }

                        if (enemy->timer == 0u)
                        {
                            core->ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x01u;
                            memset(enemy, 0, sizeof(*enemy));
                        }
                    }
                    break;

                case 0x19u:
                case 0x1Au:
                    if (boss_room || grenade_launcher_active)
                    {
                        break;
                    }

                    if (enemy->timer != 0u)
                    {
                        enemy->timer = (uint8_t)(enemy->timer - 1u);
                    }
                    else
                    {
                        const uint8_t screen = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
                        const ContraLevel2GeneratorScript *script;
                        uint8_t read_offset;
                        uint8_t spawn_attrs;
                        uint8_t delay_byte;
                        uint8_t generated_type;
                        uint8_t generated_count = 1u;
                        uint8_t generated_index;

                        if (screen >= (sizeof(contra_level_2_enemy_gen_scripts) / sizeof(contra_level_2_enemy_gen_scripts[0])))
                        {
                            break;
                        }

                        script = &contra_level_2_enemy_gen_scripts[screen];
                        read_offset = (uint8_t)(enemy->flags % script->length);
                        spawn_attrs = script->data[read_offset];
                        delay_byte = script->data[(uint8_t)(read_offset + 1u)];
                        enemy->flags = (uint8_t)(read_offset + 2u);
                        if (enemy->flags >= script->length)
                        {
                            enemy->flags = 0x00u;
                        }

                        if ((delay_byte & 0x80u) != 0u)
                        {
                            core->ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] =
                                (uint8_t)(core->ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] + 1u);
                            if (core->ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] >= 0x07u)
                            {
                                memset(enemy, 0, sizeof(*enemy));
                                break;
                            }
                        }

                        switch (spawn_attrs >> 6u)
                        {
                            case 0x00u:
                                generated_type = 0x15u;
                                break;

                            case 0x01u:
                                generated_type = 0x16u;
                                break;

                            case 0x02u:
                                generated_type = 0x18u;
                                generated_count = 4u;
                                break;

                            default:
                                generated_type = 0x17u;
                                core->ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0x01u;
                                break;
                        }

                        for (generated_index = 0u; generated_index < generated_count; ++generated_index)
                        {
                            size_t slot_index;

                            for (slot_index = 0u; slot_index < CONTRA_NATIVE_MAX_ENEMIES; ++slot_index)
                            {
                                ContraNativeEnemy *const generated = &core->enemies[slot_index];

                                if (generated->active != 0u)
                                {
                                    continue;
                                }

                                memset(generated, 0, sizeof(*generated));
                                generated->active = 0x01u;
                                generated->type = generated_type;
                                generated->attrs = (uint8_t)(spawn_attrs & 0x3Fu);
                                generated->screen_id = screen;
                                generated->hp = 0x01u;
                                generated->state = 0x01u;
                                generated->timer = (uint8_t)(generated_index * 4u);
                                generated->cooldown = 0x08u;
                                generated->sprite_code = (generated_type == 0x16u) ? 0x97u : 0x93u;
                                generated->sprite_attr = ((spawn_attrs & 0x01u) != 0u) ? 0x00u : 0x40u;
                                generated->x = ((spawn_attrs & 0x01u) != 0u) ? 0x0A : 0xFA;
                                generated->y = (int16_t)(0x70 + (int)(generated_index * 8u));
                                generated->vx = ((spawn_attrs & 0x01u) != 0u) ? 1 : -1;
                                break;
                            }
                        }

                        enemy->timer = (uint8_t)(delay_byte & 0x7Fu);
                    }
                    break;

                case 0x15u:
                case 0x16u:
                case 0x17u:
                case 0x18u:
                    enemy->x = (int16_t)(enemy->x + enemy->vx);
                    enemy->sprite_code = (enemy->type == 0x16u) ? 0x97u : 0x93u;
                    enemy->sprite_attr = (uint8_t)(enemy->vx < 0 ? 0x40u : 0x00u);
                    if ((enemy->x >= 0x68) && (enemy->x < 0x98))
                    {
                        if (enemy->cooldown != 0u)
                        {
                            enemy->cooldown = (uint8_t)(enemy->cooldown - 1u);
                        }
                        else
                        {
                            int target_dx = -32;
                            int target_dy = 0;
                            int8_t projectile_vx;
                            int8_t projectile_vy;
                            unsigned player_index;
                            bool has_target = false;

                            for (player_index = 0u; player_index < 2u; ++player_index)
                            {
                                if (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] != 0u)
                                {
                                    continue;
                                }

                                target_dx = (int)core->ram[CONTRA_RAM_SPRITE_X_POS + player_index] - (int)enemy->x;
                                target_dy = (int)core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - (int)enemy->y;
                                has_target = true;
                                break;
                            }

                            if (has_target)
                            {
                                contra_select_level_1_projectile_velocity(
                                    target_dx,
                                    target_dy,
                                    &projectile_vx,
                                    &projectile_vy
                                );
                                if (contra_try_spawn_level_1_regular_projectile(
                                        core,
                                        (int)enemy->x,
                                        (int)enemy->y,
                                        projectile_vx,
                                        projectile_vy,
                                        enemy->type))
                                {
                                    enemy->cooldown = 0x10u;
                                }
                            }
                        }
                    }
                    if ((enemy->x < -16) || (enemy->x > 0x110))
                    {
                        if (enemy->type == 0x17u)
                        {
                            core->ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0x00u;
                        }
                        memset(enemy, 0, sizeof(*enemy));
                    }
                    break;

                default:
                    break;
            }
        }

        for (enemy_index = 0u; enemy_index < CONTRA_PLAYER_BULLET_COUNT; ++enemy_index)
        {
            const int bullet_x = (int)core->ram[CONTRA_RAM_PLAYER_BULLET_X_POS + enemy_index];
            const int bullet_y = (int)core->ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + enemy_index];
            size_t target_enemy_index;

            if (core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT + enemy_index] == 0u)
            {
                continue;
            }

            for (target_enemy_index = 0u; target_enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++target_enemy_index)
            {
                ContraNativeEnemy *const enemy = &core->enemies[target_enemy_index];
                const int hit_left = (int)enemy->x - 12;
                const int hit_right = hit_left + 32;
                const int hit_top = (int)enemy->y - 12;
                const int hit_bottom = hit_top + 32;

                if (enemy->active == 0u)
                {
                    continue;
                }

                if (!boss_room)
                {
                    if ((enemy->type != 0x14u) || (enemy->state != 0x03u))
                    {
                        continue;
                    }

                    if ((core->ram[CONTRA_RAM_DEMO_MODE] != 0u) &&
                        (core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != 0x00u))
                    {
                        continue;
                    }
                }
                else if ((enemy->type != 0x08u) &&
                         (enemy->type != 0x0Au) &&
                         (enemy->type != 0x10u))
                {
                    continue;
                }

                if ((bullet_x < hit_left) || (bullet_x >= hit_right) ||
                    (bullet_y < hit_top) || (bullet_y >= hit_bottom))
                {
                    continue;
                }

                contra_clear_player_bullet(core, enemy_index);
                if (boss_room &&
                    (enemy->type == 0x10u) &&
                    (core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] < 0x04u))
                {
                    break;
                }

                if (enemy->hp != 0u)
                {
                    enemy->hp = (uint8_t)(enemy->hp - 1u);
                }

                if (enemy->hp == 0u)
                {
                    if (enemy->type == 0x14u)
                    {
                        contra_destroy_level_2_wall_core(core, enemy);
                    }
                    else if (enemy->type == 0x0Au)
                    {
                        core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] =
                            (uint8_t)(core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] + 1u);
                        memset(enemy, 0, sizeof(*enemy));
                    }
                    else if (enemy->type == 0x10u)
                    {
                        core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
                        core->ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x01u;
                        memset(enemy, 0, sizeof(*enemy));
                    }
                    else
                    {
                        memset(enemy, 0, sizeof(*enemy));
                    }
                }

                break;
            }
        }

        return;
    }

    if ((core->ram[CONTRA_RAM_CURRENT_LEVEL] != 0u) ||
        (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u))
    {
        return;
    }

    for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
    {
        ContraNativeEnemy *const enemy = &core->enemies[enemy_index];
        int target_dx = -64;
        int target_dy = 0;
        bool has_target = false;

        if (enemy->active == 0u)
        {
            continue;
        }

        if (core->ram[CONTRA_RAM_FRAME_SCROLL] != 0u)
        {
            enemy->x = (int16_t)(enemy->x - (int16_t)core->ram[CONTRA_RAM_FRAME_SCROLL]);
        }

        {
            unsigned player_index;
            int best_distance = 0;

            for (player_index = 0u; player_index < 2u; ++player_index)
            {
                if (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] != 0u)
                {
                    continue;
                }

                {
                    const int dx = (int)core->ram[CONTRA_RAM_SPRITE_X_POS + player_index] - (int)enemy->x;
                    const int dy = (int)core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - (int)enemy->y;
                    const int distance = abs(dx) + abs(dy);

                    if ((!has_target) || (distance < best_distance))
                    {
                        has_target = true;
                        best_distance = distance;
                        target_dx = dx;
                        target_dy = dy;
                    }
                }
            }
        }

        switch (enemy->type)
        {
            case 0x00u:
            {
                const uint8_t item_type = (uint8_t)(enemy->attrs & 0x07u);

                enemy->sprite_code = contra_weapon_item_sprite_code_tbl[item_type];
                enemy->sprite_attr = (item_type == 0x06u)
                    ? (uint8_t)(0x04u | ((core->ram[CONTRA_RAM_FRAME_COUNTER] >> 3u) & 0x03u))
                    : 0x05u;

                if (enemy->state == 0x00u)
                {
                    uint8_t floor_y;

                    contra_advance_subpixel_position_s16(&enemy->x, &enemy->x_accum, (uint8_t)enemy->vx, enemy->x_frac);
                    contra_advance_subpixel_position_s16(&enemy->y, &enemy->y_accum, (uint8_t)enemy->vy, enemy->y_frac);
                    if ((enemy->vy >= 0) &&
                        contra_find_level_1_floor_y_below(
                            core,
                            (uint8_t)enemy->x,
                            (uint8_t)((enemy->y > 0) ? enemy->y : 0),
                            &floor_y) &&
                        (enemy->y >= floor_y))
                    {
                        enemy->y = floor_y;
                        enemy->state = 0x01u;
                        enemy->vx = 0;
                        enemy->vy = 0;
                        enemy->x_frac = 0x00u;
                        enemy->y_frac = 0x00u;
                        enemy->x_accum = 0x00u;
                        enemy->y_accum = 0x00u;
                    }
                    else
                    {
                        if ((enemy->x >= 0xE8) && (enemy->vx >= 0))
                        {
                            uint8_t vx_fast = (uint8_t)enemy->vx;

                            contra_negate_subpixel_velocity(&vx_fast, &enemy->x_frac);
                            enemy->vx = (int8_t)vx_fast;
                        }
                        else if ((enemy->x < 0x18) && (enemy->vx < 0))
                        {
                            uint8_t vx_fast = (uint8_t)enemy->vx;

                            contra_negate_subpixel_velocity(&vx_fast, &enemy->x_frac);
                            enemy->vx = (int8_t)vx_fast;
                        }

                        contra_add_to_enemy_y_fractional_velocity(enemy, 0x10u);
                    }
                }

                if ((enemy->x < 0x08) || (enemy->x > 0x108) || (enemy->y >= 0xF0))
                {
                    memset(enemy, 0, sizeof(*enemy));
                }
                break;
            }

            case 0x02u:
                if (enemy->x < 0x18)
                {
                    memset(enemy, 0, sizeof(*enemy));
                    break;
                }

                enemy->sprite_code = 0x00u;
                if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_WAIT)
                {
                    if (enemy->timer != 0u)
                    {
                        enemy->timer = (uint8_t)(enemy->timer - 1u);
                    }
                    else
                    {
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_EMERGE;
                        enemy->timer = 0x08u;
                        enemy->flags = 0x01u;
                    }
                }
                else if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_EMERGE)
                {
                    enemy->sprite_code = enemy->flags;
                    if (enemy->timer != 0u)
                    {
                        enemy->timer = (uint8_t)(enemy->timer - 1u);
                    }
                    else if (enemy->flags < 0x02u)
                    {
                        enemy->flags = (uint8_t)(enemy->flags + 1u);
                        enemy->timer = 0x08u;
                    }
                    else
                    {
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_ACTIVE;
                        enemy->sprite_code = 0x02u;
                        contra_snap_native_enemy_to_outdoor_floor(core, enemy);
                    }
                }
                else
                {
                    enemy->sprite_code = 0x02u;
                    contra_snap_native_enemy_to_outdoor_floor(core, enemy);
                }
                break;

            case 0x03u:
                contra_update_flying_capsule_axis_velocity(
                    (uint8_t)enemy->y,
                    enemy->origin_y,
                    &enemy->vy,
                    &enemy->y_frac
                );
                enemy->y = (int16_t)contra_apply_flying_capsule_axis_velocity(
                    (uint8_t)enemy->y,
                    enemy->vy,
                    enemy->y_frac,
                    &enemy->y_accum
                );
                enemy->x = (int16_t)contra_apply_flying_capsule_axis_velocity(
                    (uint8_t)enemy->x,
                    enemy->vx,
                    enemy->x_frac,
                    &enemy->x_accum
                );
                if ((enemy->x > 0x120) || (enemy->x < -16))
                {
                    memset(enemy, 0, sizeof(*enemy));
                }
                break;

            case 0x04u:
                if (enemy->x < 0x18)
                {
                    memset(enemy, 0, sizeof(*enemy));
                    break;
                }

                if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_EMERGE)
                {
                    enemy->sprite_code = (uint8_t)(0x03u + enemy->flags);
                    if (enemy->timer != 0u)
                    {
                        enemy->timer = (uint8_t)(enemy->timer - 1u);
                    }
                    else if (enemy->flags < 0x02u)
                    {
                        enemy->flags = (uint8_t)(enemy->flags + 1u);
                        enemy->timer = 0x08u;
                    }
                    else
                    {
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_ACTIVE;
                        enemy->timer = 0x08u;
                        enemy->cooldown = 0x10u;
                    }
                }
                else
                {
                    if (!has_target)
                    {
                        enemy->sprite_code = 0x05u;
                        break;
                    }

                    if ((target_dx <= 0) && (target_dy <= 0))
                    {
                        if ((-target_dy) >= ((-target_dx) * 2))
                        {
                            enemy->sprite_code = 0x08u;
                        }
                        else if ((-target_dy) >= (-target_dx))
                        {
                            enemy->sprite_code = 0x07u;
                        }
                        else if (((-target_dy) * 2) >= (-target_dx))
                        {
                            enemy->sprite_code = 0x06u;
                        }
                        else
                        {
                            enemy->sprite_code = 0x05u;
                        }
                    }
                    else if ((target_dx >= 0) && (target_dy <= 0))
                    {
                        if ((-target_dy) >= (target_dx * 2))
                        {
                            enemy->sprite_code = 0x08u;
                        }
                        else if ((-target_dy) >= target_dx)
                        {
                            enemy->sprite_code = 0x09u;
                        }
                        else if (((-target_dy) * 2) >= target_dx)
                        {
                            enemy->sprite_code = 0x0Au;
                        }
                        else
                        {
                            enemy->sprite_code = 0x0Bu;
                        }
                    }
                    else if ((target_dx >= 0) && (target_dy >= 0))
                    {
                        if (target_dy >= (target_dx * 2))
                        {
                            enemy->sprite_code = 0x0Eu;
                        }
                        else if (target_dy >= target_dx)
                        {
                            enemy->sprite_code = 0x0Du;
                        }
                        else if ((target_dy * 2) >= target_dx)
                        {
                            enemy->sprite_code = 0x0Cu;
                        }
                        else
                        {
                            enemy->sprite_code = 0x0Bu;
                        }
                    }
                    else
                    {
                        if (target_dy >= ((-target_dx) * 2))
                        {
                            enemy->sprite_code = 0x0Eu;
                        }
                        else if (target_dy >= (-target_dx))
                        {
                            enemy->sprite_code = 0x0Fu;
                        }
                        else if ((target_dy * 2) >= (-target_dx))
                        {
                            enemy->sprite_code = 0x10u;
                        }
                        else
                        {
                            enemy->sprite_code = 0x05u;
                        }
                    }

                    if (enemy->cooldown != 0u)
                    {
                        enemy->cooldown = (uint8_t)(enemy->cooldown - 1u);
                    }
                    else if (contra_try_fire_level_1_turret(core, enemy, has_target, target_dx, target_dy))
                    {
                        enemy->cooldown = 0x28u;
                    }
                }
                break;

            case 0x05u:
            {
                if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_WAIT)
                {
                    enemy->y = (int16_t)(enemy->y + 4);
                    enemy->state = CONTRA_NATIVE_LEVEL1_STATE_EMERGE;
                    enemy->timer = 0x00u;
                    enemy->sprite_code = 0x01u;
                    break;
                }

                if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_EMERGE)
                {
                    if (enemy->timer == 0u)
                    {
                        enemy->x = (int16_t)(enemy->x - enemy->vx);
                        enemy->timer = 0x01u;
                    }
                    enemy->sprite_code = 0x01u;
                    break;
                }

                if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_RETREAT)
                {
                    static const int8_t hit_dx[8] = {0, 0, 0, 1, 0, 0, 1, 0};
                    static const int8_t hit_dy[8] = {0, -4, -3, -3, -3, -2, -3, -2};
                    const uint8_t hit_frame = enemy->timer;
                    const int direction = (enemy->vx < 0) ? 1 : -1;

                    enemy->sprite_code = 0x27u;
                    if (hit_frame < (uint8_t)(sizeof(hit_dx) / sizeof(hit_dx[0])))
                    {
                        enemy->x = (int16_t)(enemy->x + (hit_dx[hit_frame] * direction));
                        enemy->y = (int16_t)(enemy->y + hit_dy[hit_frame]);
                        enemy->timer = (uint8_t)(enemy->timer + 1u);
                    }
                    break;
                }

                {
                    const int16_t next_x = (int16_t)(enemy->x + enemy->vx);
                const uint8_t support_collision =
                    contra_get_outdoor_horizontal_bg_collision(core, (uint8_t)next_x, (uint8_t)(enemy->y + 0x10));

                if ((support_collision != 0x01u) && (support_collision != 0x80u))
                {
                    if ((enemy->attrs & 0x02u) != 0u)
                    {
                        enemy->vx = (int16_t)(-enemy->vx);
                    }
                    else
                    {
                        memset(enemy, 0, sizeof(*enemy));
                        break;
                    }
                }

                enemy->timer = (uint8_t)(enemy->timer + 1u);
                enemy->sprite_code = contra_level_1_soldier_walk_sprites[(enemy->timer >> 3u) % 6u];
                enemy->x = (int16_t)(enemy->x + enemy->vx);
                enemy->sprite_attr = (uint8_t)(enemy->vx < 0 ? 0x40u : 0x00u);
                contra_snap_native_enemy_to_outdoor_floor(core, enemy);
                if ((enemy->x < -16) || (enemy->x > 0x110))
                {
                    memset(enemy, 0, sizeof(*enemy));
                }
                }
                break;
            }

            case 0x06u:
                enemy->sprite_attr = (uint8_t)((has_target && (target_dx > 0)) ? 0x00u : 0x40u);
                if ((enemy->attrs & 0x03u) == 0x00u)
                {
                    contra_snap_native_enemy_to_outdoor_floor(core, enemy);
                }
                if (enemy->x < 0x10)
                {
                    memset(enemy, 0, sizeof(*enemy));
                }
                break;

            case 0x07u:
            {
                const uint8_t rocky_bg = (uint8_t)(((enemy->attrs & 0x01u) == 0u) ? 0x16u : 0x17u);
                const uint8_t half_open = (uint8_t)(((enemy->attrs & 0x01u) == 0u) ? 0x14u : 0x15u);

                if ((enemy->x < 0x30) && (enemy->state != CONTRA_NATIVE_LEVEL1_STATE_RETREAT))
                {
                    enemy->state = CONTRA_NATIVE_LEVEL1_STATE_RETREAT;
                    enemy->timer = 0x01u;
                    enemy->flags = 0x02u;
                }

                if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_EMERGE)
                {
                    static const uint8_t emerge_sequence[4] = {0x00u, 0x01u, 0x02u, 0x03u};
                    const uint8_t sequence_index = emerge_sequence[enemy->flags & 0x03u];

                    enemy->sprite_code = rocky_bg;
                    if (sequence_index == 0x01u)
                    {
                        enemy->sprite_code = half_open;
                    }
                    else if (sequence_index == 0x02u)
                    {
                        enemy->sprite_code = 0x18u;
                    }
                    else if (sequence_index == 0x03u)
                    {
                        enemy->sprite_code = 0x11u;
                    }

                    if (enemy->timer != 0u)
                    {
                        enemy->timer = (uint8_t)(enemy->timer - 1u);
                    }
                    else if (enemy->flags < 0x03u)
                    {
                        enemy->flags = (uint8_t)(enemy->flags + 1u);
                        enemy->timer = 0x04u;
                    }
                    else
                    {
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_ACTIVE;
                        enemy->timer = 0x10u;
                        enemy->cooldown = (uint8_t)(0x18u + ((enemy->attrs & 0x03u) << 2u));
                    }
                }
                else if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_RETREAT)
                {
                    enemy->sprite_code = rocky_bg;
                    if (enemy->flags == 0x02u)
                    {
                        enemy->sprite_code = 0x18u;
                    }
                    else if (enemy->flags == 0x01u)
                    {
                        enemy->sprite_code = half_open;
                    }

                    if (enemy->timer != 0u)
                    {
                        enemy->timer = (uint8_t)(enemy->timer - 1u);
                    }
                    else if (enemy->flags != 0u)
                    {
                        enemy->flags = (uint8_t)(enemy->flags - 1u);
                        enemy->timer = 0x04u;
                    }
                    else
                    {
                        memset(enemy, 0, sizeof(*enemy));
                    }
                }
                else
                {
                    if ((target_dx < 0) && (target_dy < -24))
                    {
                        enemy->sprite_code = 0x13u;
                    }
                    else if ((target_dx < 0) && (target_dy < -4))
                    {
                        enemy->sprite_code = 0x12u;
                    }
                    else
                    {
                        enemy->sprite_code = 0x11u;
                    }

                    if (enemy->cooldown != 0u)
                    {
                        enemy->cooldown = (uint8_t)(enemy->cooldown - 1u);
                    }
                    else if (contra_try_fire_level_1_turret(core, enemy, has_target, target_dx, target_dy))
                    {
                        enemy->cooldown = (uint8_t)(0x30u + ((enemy->attrs & 0x03u) << 3u));
                    }
                }
                break;
            }

            case 0x10u:
                if (enemy->x < 0x18)
                {
                    memset(enemy, 0, sizeof(*enemy));
                    break;
                }

                if (enemy->cooldown != 0u)
                {
                    enemy->cooldown = (uint8_t)(enemy->cooldown - 1u);
                    enemy->sprite_code = contra_level_1_boss_bomb_turret_sprite_code(enemy);
                    break;
                }

                {
                    const uint8_t current_frame = (uint8_t)(enemy->flags & 0x02u);

                    enemy->cooldown = (current_frame == 0u) ? 0x28u : 0x08u;
                    enemy->flags = (uint8_t)(current_frame ^ 0x02u);
                    enemy->sprite_code = contra_level_1_boss_bomb_turret_sprite_code(enemy);
                    if ((enemy->flags & 0x02u) != 0u)
                    {
                        (void)contra_try_spawn_level_1_boss_bomb_turret_projectile(core, enemy);
                    }
                }
                break;

            case 0x12u:
                if (enemy->x < -116)
                {
                    memset(enemy, 0, sizeof(*enemy));
                    break;
                }

                if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_WAIT)
                {
                    if (has_target && (abs(target_dx) < 0x18))
                    {
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_EMERGE;
                        enemy->timer = 0x01u;
                        enemy->flags = 0x00u;
                        enemy->cooldown = 0x00u;
                    }
                }
                else if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_EMERGE)
                {
                    const uint8_t section = (uint8_t)(enemy->flags & 0x03u);

                    if (enemy->timer != 0u)
                    {
                        enemy->timer = (uint8_t)(enemy->timer - 1u);
                        if (enemy->timer != 0u)
                        {
                            break;
                        }
                    }

                    if (enemy->cooldown < 2u)
                    {
                        const uint8_t supertile_index =
                            contra_level_1_bridge_destroyed_supertile_tbl[(section * 2u) + enemy->cooldown];

                        if (supertile_index != 0u)
                        {
                            const unsigned overlay_section = (enemy->cooldown == 0u)
                                ? (unsigned)(section - 1u)
                                : (unsigned)section;

                            contra_level_1_bridge_set_overlay(
                                enemy,
                                overlay_section,
                                supertile_index
                            );
                        }
                    }

                    enemy->cooldown = (uint8_t)(enemy->cooldown + 1u);
                    if (enemy->cooldown >= 4u)
                    {
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_ACTIVE;
                    }
                    else
                    {
                        enemy->timer = 0x04u;
                    }
                }
                else if (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_ACTIVE)
                {
                    enemy->flags = (uint8_t)(enemy->flags + 1u);
                    if (enemy->flags >= 0x04u)
                    {
                        memset(enemy, 0, sizeof(*enemy));
                    }
                    else
                    {
                        enemy->x = (int16_t)(enemy->x + 0x20);
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_EMERGE;
                        enemy->timer = 0x01u;
                        enemy->cooldown = 0x00u;
                    }
                }
                break;

            case CONTRA_NATIVE_LEVEL1_TYPE_EXPLOSION:
                if ((enemy->x < -16) || (enemy->x > 0x110))
                {
                    memset(enemy, 0, sizeof(*enemy));
                    break;
                }

                if (enemy->timer != 0u)
                {
                    enemy->timer = (uint8_t)(enemy->timer - 1u);
                }

                if (enemy->timer == 0u)
                {
                    const uint8_t next_frame = (uint8_t)(enemy->state + 1u);

                    if (next_frame >= contra_get_level_1_explosion_frame_count(enemy->flags))
                    {
                        memset(enemy, 0, sizeof(*enemy));
                        break;
                    }

                    enemy->state = next_frame;
                    enemy->timer = CONTRA_NATIVE_LEVEL1_EXPLOSION_FRAME_DELAY;
                    enemy->sprite_code = contra_get_level_1_explosion_sprite_code(enemy->flags, next_frame);
                }
                break;

            default:
                memset(enemy, 0, sizeof(*enemy));
                break;
        }
    }

    for (enemy_index = 0u; enemy_index < CONTRA_PLAYER_BULLET_COUNT; ++enemy_index)
    {
        const int bullet_x = (int)core->ram[CONTRA_RAM_PLAYER_BULLET_X_POS + enemy_index];
        const int bullet_y = (int)core->ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + enemy_index];
        size_t target_enemy_index;

        if (core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT + enemy_index] == 0u)
        {
            continue;
        }

        for (target_enemy_index = 0u; target_enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++target_enemy_index)
        {
            ContraNativeEnemy *const enemy = &core->enemies[target_enemy_index];
            int hit_left;
            int hit_right;
            int hit_top;
            int hit_bottom;

            if ((enemy->active == 0u) ||
                (enemy->type == 0x00u) ||
                (enemy->type == 0x12u) ||
                (enemy->type == CONTRA_NATIVE_LEVEL1_TYPE_EXPLOSION) ||
                ((enemy->type == 0x02u) && (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_WAIT)) ||
                ((enemy->type == 0x05u) && (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_EMERGE) && (enemy->timer == 0u)) ||
                ((enemy->type == 0x05u) && (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_RETREAT)) ||
                ((enemy->type == 0x07u) && (enemy->state == CONTRA_NATIVE_LEVEL1_STATE_RETREAT)))
            {
                continue;
            }

            if ((enemy->type == 0x02u) ||
                (enemy->type == 0x04u) ||
                (enemy->type == 0x07u) ||
                (enemy->type == 0x10u))
            {
                hit_left = (int)enemy->x - 12;
                hit_right = hit_left + 32;
                hit_top = (int)enemy->y - 12;
                hit_bottom = hit_top + 32;
            }
            else
            {
                hit_left = (int)enemy->x - 8;
                hit_right = hit_left + 16;
                hit_top = (int)enemy->y - 16;
                hit_bottom = hit_top + 24;
            }

            if ((bullet_x < hit_left) || (bullet_x >= hit_right) ||
                (bullet_y < hit_top) || (bullet_y >= hit_bottom))
            {
                continue;
            }

            contra_clear_player_bullet(core, enemy_index);
            if (enemy->type == 0x05u)
            {
                enemy->state = CONTRA_NATIVE_LEVEL1_STATE_RETREAT;
                enemy->timer = 0x00u;
                enemy->sprite_code = 0x01u;
                enemy->hp = 0x00u;
                break;
            }

            if (enemy->hp != 0u)
            {
                enemy->hp = (uint8_t)(enemy->hp - 1u);
            }

            if (enemy->hp == 0u)
            {
                contra_handle_level_1_enemy_destroyed(core, enemy);
            }

            break;
        }
    }
}

static void contra_load_bank_2_load_screen_enemy_data(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint16_t screen_data_addr;
    uint8_t read_offset;

    if (!contra_load_rom_image())
    {
        return;
    }

    if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) &&
        ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) ||
         (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u)))
    {
        static const uint8_t wall_core_hp_tbl[4] = {0x08u, 0x05u, 0x10u, 0x05u};
        static const uint8_t core_opening_delay_tbl[4] = {0x20u, 0x80u, 0xB0u, 0xF0u};
        const bool boss_room = (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u);
        uint8_t read_index = 0x01u;
        int enemy_index = (int)CONTRA_NATIVE_MAX_ENEMIES - 1;

        if (ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] != 0u)
        {
            return;
        }

        screen_data_addr = contra_rom_read_u16(
            2u,
            (uint16_t)(contra_level_2_enemy_screen_ptr_tbl_addr + ((uint16_t)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] * 2u))
        );

        ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x01u;
        ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] = 0x00u;
        ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] = 0x00u;
        ram[CONTRA_RAM_INDOOR_RED_SOLDIER_CREATED] = 0x00u;
        ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0x00u;
        ram[CONTRA_RAM_WALL_CORE_REMAINING] = contra_rom_read_u8(2u, screen_data_addr);
        ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] =
            (ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0u) ? 0x01u : 0x00u;
        memset(core->enemies, 0, sizeof(core->enemies));

        while (enemy_index >= 0)
        {
            const uint8_t position = contra_rom_read_u8(2u, (uint16_t)(screen_data_addr + read_index));
            uint8_t type_flags;
            uint8_t enemy_type;
            uint8_t enemy_attrs;
            ContraNativeEnemy *enemy;

            if (position == 0xFFu)
            {
                break;
            }

            type_flags = contra_rom_read_u8(2u, (uint16_t)(screen_data_addr + read_index + 1u));
            enemy_type = (uint8_t)(type_flags & 0x3Fu);
            enemy_attrs = contra_rom_read_u8(2u, (uint16_t)(screen_data_addr + read_index + 2u));
            read_index = (uint8_t)(read_index + 3u);

            if ((enemy_type != 0x13u) &&
                (enemy_type != 0x14u) &&
                (enemy_type != 0x19u) &&
                (enemy_type != 0x1Au) &&
                (!boss_room ||
                 ((enemy_type != 0x08u) &&
                  (enemy_type != 0x0Au) &&
                  (enemy_type != 0x10u))))
            {
                continue;
            }

            enemy = &core->enemies[enemy_index--];
            memset(enemy, 0, sizeof(*enemy));
            enemy->active = 0x01u;
            enemy->type = enemy_type;
            enemy->attrs = enemy_attrs;
            enemy->screen_id = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
            enemy->x = (int16_t)((uint8_t)(position << 4u));
            enemy->y = (int16_t)(position & 0xF0u);
            if ((type_flags & 0x40u) != 0u)
            {
                enemy->x = (int16_t)(enemy->x + 7);
            }
            if ((type_flags & 0x80u) != 0u)
            {
                enemy->y = (int16_t)(enemy->y + 7);
            }

            switch (enemy_type)
            {
                case 0x08u:
                    enemy->hp = 0x04u;
                    enemy->state = 0x01u;
                    enemy->timer = 0x40u;
                    enemy->sprite_code = 0x84u;
                    break;

                case 0x0Au:
                    enemy->hp = 0x03u;
                    enemy->state = 0x01u;
                    enemy->sprite_code = 0x80u;
                    break;

                case 0x10u:
                    enemy->hp = 0x20u;
                    enemy->state = 0x01u;
                    enemy->timer = 0x60u;
                    enemy->sprite_code = 0x87u;
                    break;

                case 0x13u:
                    enemy->hp = 0x04u;
                    enemy->sprite_code = 0x84u;
                    break;

                case 0x14u:
                {
                    const uint8_t core_kind = (uint8_t)((enemy_attrs >> 2u) & 0x03u);
                    const uint8_t opening_delay_index = (uint8_t)(enemy_attrs & 0x03u);

                    enemy->hp = wall_core_hp_tbl[core_kind];
                    enemy->state = 0x01u;
                    enemy->timer = ((enemy_attrs & 0x04u) != 0u)
                        ? core_opening_delay_tbl[0u]
                        : core_opening_delay_tbl[opening_delay_index];
                    enemy->sprite_code = ((enemy_attrs & 0x04u) != 0u) ? 0x80u : 0x84u;
                    break;
                }

                case 0x19u:
                case 0x1Au:
                    enemy->timer = (enemy_type == 0x1Au) ? 0x50u : 0x40u;
                    break;

                default:
                    break;
            }
        }

        return;
    }

    if ((ram[CONTRA_RAM_CURRENT_LEVEL] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u))
    {
        return;
    }

    screen_data_addr = contra_rom_read_u16(
        2u,
        (uint16_t)(contra_level_1_enemy_screen_ptr_tbl_addr + ((uint16_t)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] * 2u))
    );
    read_offset = ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET];

    for (;;)
    {
        const uint8_t spawn_trigger = contra_rom_read_u8(2u, (uint16_t)(screen_data_addr + read_offset));
        const uint8_t scroll_offset = ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
        uint8_t spawn_delta;
        uint8_t enemy_type_repeat;
        uint8_t enemy_type;
        uint8_t repeat_count;

        if (spawn_trigger == 0xFFu)
        {
            return;
        }

        if ((uint8_t)(spawn_trigger & 0xFEu) > scroll_offset)
        {
            return;
        }

        spawn_delta = (uint8_t)(scroll_offset - (uint8_t)(spawn_trigger & 0xFEu));
        enemy_type_repeat = contra_rom_read_u8(2u, (uint16_t)(screen_data_addr + read_offset + 1u));
        enemy_type = (uint8_t)(enemy_type_repeat & 0x3Fu);
        repeat_count = (uint8_t)(enemy_type_repeat >> 6u);
        read_offset = (uint8_t)(read_offset + 2u);

        do
        {
            const uint8_t enemy_y_attrs = contra_rom_read_u8(2u, (uint16_t)(screen_data_addr + read_offset));
            size_t enemy_index;

            read_offset = (uint8_t)(read_offset + 1u);

            switch (enemy_type)
            {
                case 0x02u:
                case 0x03u:
                case 0x04u:
                case 0x05u:
                case 0x06u:
                case 0x07u:
                case 0x10u:
                case 0x12u:
                    break;

                default:
                    continue;
            }

            for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
            {
                ContraNativeEnemy *const enemy = &core->enemies[enemy_index];

                if (enemy->active != 0u)
                {
                    continue;
                }

                memset(enemy, 0, sizeof(*enemy));
                enemy->active = 0x01u;
                enemy->type = enemy_type;
                enemy->attrs = (uint8_t)(enemy_y_attrs & 0x0Fu);
                enemy->screen_id = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
                enemy->x = (int16_t)(0xF0u - spawn_delta);
                enemy->y = (int16_t)(enemy_y_attrs & 0xF0u);
                enemy->state = CONTRA_NATIVE_LEVEL1_STATE_ACTIVE;
                enemy->hp = 0x01u;

                switch (enemy_type)
                {
                    case 0x02u:
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_WAIT;
                        enemy->timer = 0x20u;
                        enemy->hp = 0x03u;
                        break;

                    case 0x03u:
                        enemy->sprite_code = 0x4Du;
                        enemy->sprite_attr = 0x03u;
                        enemy->origin_x = (uint8_t)enemy->x;
                        enemy->origin_y = (uint8_t)enemy->y;
                        enemy->x = 0x10;
                        enemy->y = (int16_t)((uint8_t)(enemy->origin_y + 0x20u));
                        enemy->vx = 0x01;
                        enemy->vy = 0x00;
                        enemy->x_frac = 0x80u;
                        enemy->y_frac = 0x00u;
                        enemy->x_accum = 0x00u;
                        enemy->y_accum = 0x00u;
                        enemy->hp = 0x01u;
                        break;

                    case 0x04u:
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_EMERGE;
                        enemy->timer = 0x08u;
                        enemy->hp = 0x04u;
                        break;

                    case 0x05u:
                        enemy->vx = (int8_t)(((enemy->attrs & 0x01u) != 0u) ? 1 : -1);
                        enemy->sprite_code = 0x01u;
                        enemy->sprite_attr = (uint8_t)(enemy->vx < 0 ? 0x40u : 0x00u);
                        enemy->hp = 0x01u;
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_WAIT;
                        if ((enemy->attrs & 0x01u) != 0u)
                        {
                            enemy->x = 0x0A;
                        }
                        break;

                    case 0x06u:
                        if ((enemy->attrs & 0x03u) == 0x01u)
                        {
                            enemy->sprite_code = 0x45u;
                            enemy->y = (int16_t)(enemy->y + 9);
                        }
                        else if ((enemy->attrs & 0x03u) == 0x02u)
                        {
                            enemy->sprite_code = 0x2Cu;
                            enemy->y = (int16_t)(enemy->y + 4);
                        }
                        else
                        {
                            enemy->sprite_code = 0x43u;
                            enemy->y = (int16_t)(enemy->y + 4);
                        }
                        enemy->hp = 0x01u;
                        if ((enemy->attrs & 0x03u) == 0x00u)
                        {
                            contra_snap_native_enemy_to_outdoor_floor(core, enemy);
                        }
                        break;

                    case 0x07u:
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_EMERGE;
                        enemy->timer = 0x01u;
                        enemy->hp = 0x04u;
                        break;

                    case 0x10u:
                        enemy->cooldown = 0x20u;
                        enemy->flags = 0x00u;
                        enemy->sprite_code = contra_level_1_boss_bomb_turret_sprite_code(enemy);
                        enemy->hp = 0x08u;
                        break;

                    case 0x12u:
                        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_WAIT;
                        enemy->screen_id = 0x00u;
                        enemy->sprite_code = 0x00u;
                        enemy->sprite_attr = 0x00u;
                        enemy->hp = 0x00u;
                        break;

                    default:
                        break;
                }

                break;
            }
        } while (repeat_count-- != 0u);

        ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = read_offset;
    }
}

static uint8_t contra_get_soldier_generation_delay(const ContraCore *core)
{
    uint8_t delay = contra_level_soldier_generation_timer[core->ram[CONTRA_RAM_CURRENT_LEVEL]];
    uint8_t reductions = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] * 0x05u);
    uint8_t completion_count = core->ram[CONTRA_RAM_GAME_COMPLETION_COUNT];

    if (completion_count > 0x03u)
    {
        completion_count = 0x03u;
    }

    reductions = (uint8_t)(reductions + (uint8_t)(completion_count * 0x28u));
    if (delay <= reductions)
    {
        return 0x08u;
    }

    return (uint8_t)(delay - reductions);
}

static bool contra_find_level_1_soldier_spawn_y(const ContraCore *core, uint8_t spawn_x, uint8_t *spawn_y_out)
{
    uint8_t target_y = 0x80u;
    bool found = false;
    int best_distance = 0x7FFFFFFF;
    unsigned player_index;
    uint8_t floor_test_y;

    for (player_index = 0u; player_index < 2u; ++player_index)
    {
        if (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] != 0u)
        {
            continue;
        }

        target_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
        break;
    }

    for (floor_test_y = 0x10u; floor_test_y < 0xF0u; floor_test_y = (uint8_t)(floor_test_y + 0x10u))
    {
        const uint8_t floor_collision = contra_get_outdoor_horizontal_bg_collision(core, spawn_x, floor_test_y);
        const uint8_t candidate_y = (uint8_t)(floor_test_y - 0x10u);
        const int distance = abs((int)candidate_y - (int)target_y);

        if ((floor_collision != 0x01u) || (candidate_y >= 0xE0u))
        {
            continue;
        }

        if ((!found) || (distance < best_distance))
        {
            found = true;
            best_distance = distance;
            *spawn_y_out = candidate_y;
        }
    }

    return found;
}

static void contra_try_spawn_level_1_generated_soldier(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    const bool tutorial_window =
        (ram[CONTRA_RAM_CURRENT_LEVEL] == 0u) &&
        (ram[CONTRA_RAM_GAME_COMPLETION_COUNT] == 0u) &&
        (ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] < 0x1Eu);
    bool spawn_on_right;
    uint8_t screen_attr;
    uint8_t spawn_x;
    uint8_t spawn_y;
    unsigned player_index;
    size_t enemy_index;

    if ((screen == 0u) || (screen > (sizeof(contra_level_1_soldier_generation_screen_attrs) / sizeof(contra_level_1_soldier_generation_screen_attrs[0]))))
    {
        return;
    }

    screen_attr = contra_level_1_soldier_generation_screen_attrs[screen - 1u];
    if (screen_attr == 0xFFu)
    {
        return;
    }

    if (((screen_attr & 0x80u) != 0u) && ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u))
    {
        return;
    }

    if (((screen_attr & 0x40u) != 0u) && ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u) != 0u))
    {
        return;
    }

    if (tutorial_window)
    {
        spawn_on_right = true;
    }
    else
    {
        spawn_on_right =
            contra_gen_soldier_initial_x_pos[
                (ram[CONTRA_RAM_FRAME_COUNTER] + ram[CONTRA_RAM_RANDOM_NUM]) & 0x0Fu
            ] >= 0x80u;
    }

    for (player_index = 0u; player_index < 2u; ++player_index)
    {
        if (ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] != 0u)
        {
            continue;
        }

        if (spawn_on_right)
        {
            if (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >= 0xC0u)
            {
                return;
            }
        }
        else if (ram[CONTRA_RAM_SPRITE_X_POS + player_index] < 0x40u)
        {
            return;
        }
    }

    spawn_x = spawn_on_right ? 0xFCu : 0x0Au;
    if (!contra_find_level_1_soldier_spawn_y(core, spawn_x, &spawn_y))
    {
        return;
    }

    for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
    {
        ContraNativeEnemy *const enemy = &core->enemies[enemy_index];
        uint8_t soldier_attr;
        uint8_t attr_group = (uint8_t)(screen_attr & 0x3Fu);
        uint8_t attr_index;

        if (enemy->active != 0u)
        {
            continue;
        }

        if (attr_group >= 7u)
        {
            attr_group = 0u;
        }

        attr_index = (uint8_t)((attr_group * 4u) + (ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u));
        soldier_attr = contra_gen_soldier_initial_attr_tbl[attr_index];
        if (((ram[CONTRA_RAM_FRAME_COUNTER] + ram[CONTRA_RAM_RANDOM_NUM]) & 0x02u) != 0u)
        {
            soldier_attr |= 0x02u;
        }

        if (spawn_on_right)
        {
            soldier_attr &= (uint8_t)~0x01u;
        }
        else
        {
            soldier_attr |= 0x01u;
        }

        memset(enemy, 0, sizeof(*enemy));
        enemy->active = 0x01u;
        enemy->type = 0x05u;
        enemy->attrs = soldier_attr;
        enemy->hp = 0x01u;
        enemy->state = CONTRA_NATIVE_LEVEL1_STATE_ACTIVE;
        enemy->screen_id = screen;
        enemy->sprite_code = contra_level_1_soldier_walk_sprites[0u];
        enemy->sprite_attr = spawn_on_right ? 0x40u : 0x00u;
        enemy->x = spawn_x;
        enemy->y = spawn_y;
        enemy->vx = spawn_on_right ? -1 : 1;
        contra_snap_native_enemy_to_outdoor_floor(core, enemy);
        ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] = (uint8_t)(ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] + 1u);
        return;
    }
}

static void contra_load_bank_2_exe_soldier_generation(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t timer_step;

    if ((ram[CONTRA_RAM_CURRENT_LEVEL] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u))
    {
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != ram[CONTRA_RAM_SOLDIER_GEN_SCREEN])
    {
        ram[CONTRA_RAM_SOLDIER_GEN_SCREEN] = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
        ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] = 0x00u;
    }

    if (ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] == 0u)
    {
        ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] = contra_get_soldier_generation_delay(core);
        if (ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] == 0u)
        {
            return;
        }
    }

    timer_step = (((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u) &&
                  (ram[CONTRA_RAM_FRAME_SCROLL] != 0u))
        ? 0x01u
        : 0x02u;

    if (ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] > timer_step)
    {
        ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] =
            (uint8_t)(ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] - timer_step);
        return;
    }

    ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] = contra_get_soldier_generation_delay(core);
    contra_try_spawn_level_1_generated_soldier(core);
}

static void contra_load_palette_indexes(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t cycle_index;

    if (ram[CONTRA_RAM_NUM_PALETTES_TO_LOAD] >= ram[CONTRA_RAM_GAME_ROUTINE_INDEX])
    {
        return;
    }

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) != 0x05u)
    {
        return;
    }

    if (ram[CONTRA_RAM_PAUSE_PALETTE_CYCLE] != 0u)
    {
        return;
    }

    ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE] = (uint8_t)(ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE] + 1u);
    cycle_index = ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE];
    if (cycle_index >= contra_level_palette_animation_count[ram[CONTRA_RAM_CURRENT_LEVEL]])
    {
        cycle_index = 0x00u;
        ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE] = cycle_index;
    }

    ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + 3u] = ram[CONTRA_RAM_LEVEL_PALETTE_CYCLE_INDEXES + cycle_index];

    if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0u) ||
        (((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) == 0u) &&
         (ram[CONTRA_RAM_CURRENT_LEVEL] != 0x07u) &&
         ((ram[CONTRA_RAM_LEVEL_ALT_GRAPHICS_POS] & 0x80u) == 0u)))
    {
        ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + 2u] = contra_level_palette_2_index_tbl[cycle_index];
    }

    contra_load_palettes_color_to_cpu(core, 0x10u);
}

static void contra_load_bank_2_alternate_tile_loading(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t loading_flag = ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG];

    if (loading_flag == 0u)
    {
        return;
    }

    if ((loading_flag & 0x80u) == 0u)
    {
        const uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];
        const ContraAltGraphicDataRef *ref;

        if (level >= 8u)
        {
            ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x00u;
            return;
        }

        ref = &contra_alt_graphic_data_refs[level];
        if (ref->chunk_count == 0u)
        {
            ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x00u;
            return;
        }

        ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_LO] = (uint8_t)(ref->ppu_addr & 0xFFu);
        ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_HI] = (uint8_t)(ref->ppu_addr >> 8u);
        ram[CONTRA_RAM_ALT_GFX_READ_ADDR_LO] = (uint8_t)(ref->cpu_addr & 0xFFu);
        ram[CONTRA_RAM_ALT_GFX_READ_ADDR_HI] = (uint8_t)(ref->cpu_addr >> 8u);
        ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] = ref->chunk_count;
        ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x80u;
    }

    if (ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] == 0u)
    {
        ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x00u;
        return;
    }

    if (contra_load_rom_image())
    {
        const uint16_t ppu_addr = (uint16_t)(
            (uint16_t)ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_LO] |
            ((uint16_t)ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_HI] << 8u)
        );
        const uint16_t read_addr = (uint16_t)(
            (uint16_t)ram[CONTRA_RAM_ALT_GFX_READ_ADDR_LO] |
            ((uint16_t)ram[CONTRA_RAM_ALT_GFX_READ_ADDR_HI] << 8u)
        );
        unsigned chunk_offset;

        for (chunk_offset = 0u; chunk_offset < 0x20u; ++chunk_offset)
        {
            contra_write_ppu_byte(
                core,
                (uint16_t)(ppu_addr + chunk_offset),
                contra_rom_read_u8(2u, (uint16_t)(read_addr + chunk_offset))
            );
        }

        ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_LO] = (uint8_t)((ppu_addr + 0x20u) & 0xFFu);
        ram[CONTRA_RAM_ALT_GFX_PPU_ADDR_HI] = (uint8_t)((ppu_addr + 0x20u) >> 8u);
        ram[CONTRA_RAM_ALT_GFX_READ_ADDR_LO] = (uint8_t)((read_addr + 0x20u) & 0xFFu);
        ram[CONTRA_RAM_ALT_GFX_READ_ADDR_HI] = (uint8_t)((read_addr + 0x20u) >> 8u);
    }

    ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] = (uint8_t)(ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] - 1u);
    if (ram[CONTRA_RAM_ALT_GFX_CHUNK_COUNT] == 0u)
    {
        ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x00u;
    }
}

static void contra_load_palettes_color_to_cpu(ContraCore *core, uint8_t num_colors)
{
    uint8_t palette_index_offset = 0x00u;
    uint8_t write_offset = 0x00u;
    uint8_t palette_buffer[CONTRA_PPU_PALETTE_SIZE];

    core->ram[CONTRA_RAM_NUM_PALETTES_TO_LOAD] = num_colors;
    memcpy(palette_buffer, core->ppu_palette, sizeof(palette_buffer));

    while (write_offset < num_colors)
    {
        const uint8_t palette_index = core->ram[CONTRA_RAM_LEVEL_PALETTE_INDEX + palette_index_offset];
        const uint16_t game_palette_addr = (uint16_t)(0xD227u + ((uint16_t)palette_index * 3u));
        uint8_t color_index;

        palette_buffer[write_offset++] = 0x0Fu;

        for (color_index = 0u; (color_index < 3u) && (write_offset < num_colors); ++color_index)
        {
            uint8_t color = contra_rom_read_u8(7u, (uint16_t)(game_palette_addr + color_index));

            if ((core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] != 0u) &&
                (core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] < 0x09u) &&
                (write_offset >= 4u) &&
                (write_offset < 16u))
            {
                static const uint8_t palette_shift_amounts[8] = {0x00u, 0x00u, 0x10u, 0x10u, 0x20u, 0x20u, 0x30u, 0x30u};
                const uint8_t shift_index = (uint8_t)(core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] - 1u);
                const uint8_t shift = palette_shift_amounts[shift_index];

                color = (color >= shift) ? (uint8_t)(color - shift) : 0x0Fu;
            }

            palette_buffer[write_offset++] = color;
        }

        ++palette_index_offset;
    }

    if ((core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] != 0u) &&
        ((core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] & 0x80u) == 0u))
    {
        core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = (uint8_t)(core->ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] - 1u);
    }

    core->ram[CONTRA_RAM_NUM_PALETTES_TO_LOAD] = 0x00u;
    memcpy(core->pending_palette, palette_buffer, sizeof(core->pending_palette));
    core->pending_palette_count = num_colors;
    core->pending_palette_write = 0x01u;
}

static void contra_load_alternate_graphics(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];

    if (level >= 8u)
    {
        return;
    }

    ram[CONTRA_RAM_LEVEL_ALT_GRAPHICS_POS] = 0xFFu;
    memcpy(
        &ram[CONTRA_RAM_COLLISION_CODE_1_TILE_INDEX],
        contra_level_alt_collision_and_palette_tbl[level],
        sizeof(contra_level_alt_collision_and_palette_tbl[0])
    );
    contra_load_palettes_color_to_cpu(core, 0x20u);
}

static void contra_load_bank_0_load_level_enemies_to_mem(ContraCore *core)
{
    core->ram[CONTRA_RAM_ENEMY_LEVEL_ROUTINES] = core->ram[CONTRA_RAM_CURRENT_LEVEL];
}

static void contra_load_next_supertiles_screen_indexes(ContraCore *core)
{
    contra_load_supertiles_screen_indexes(core, (uint8_t)(core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 1u));
}

static void contra_load_level_header(ContraCore *core)
{
    uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    if (level >= 8u)
    {
        memset(&core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE], 0, 0x20u);
        return;
    }

    memcpy(
        &core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE],
        contra_level_headers[level],
        0x20u
    );
}

static void contra_init_ppu_write_screen_supertiles(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
        ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
        ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] = 0x00u;
        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x1Du;
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0xA0u;
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x23u;
        contra_load_supertiles_screen_indexes(core, ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER]);
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x10u;
        contra_load_palettes_color_to_cpu(core, 0x10u);
        ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x20u;
    }
    else
    {
        ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x30u;
    }

    ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
    ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
    ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x20u;
    ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_LOW_BYTE] = 0xC0u;
    ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] = 0x23u;
    contra_load_supertiles_screen_indexes(core, ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER]);
}

static void contra_set_a_as_current_level_routine(ContraCore *core, uint8_t level_routine)
{
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = level_routine;
    core->ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] = 0x00u;
}

static void contra_set_graphics_zero_mode(ContraCore *core)
{
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = 0x00u;
    memset(&core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER], 0, CONTRA_CPU_GRAPHICS_BUFFER_SIZE);
    contra_init_apu_channels(core);
}

static void contra_clear_level_runtime_memory(ContraCore *core)
{
    memset(&core->ram[0x40u], 0, 0xF0u - 0x40u);
    memset(&core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER], 0, CONTRA_RAM_CPU_GRAPHICS_BUFFER - CONTRA_RAM_CPU_SPRITE_BUFFER);
    memset(core->enemies, 0, sizeof(core->enemies));
    memset(core->enemy_projectiles, 0, sizeof(core->enemy_projectiles));
    core->level1_weapon_box_restore_timer = 0x00u;
    core->level1_weapon_box_restore_x = 0;
    core->level1_weapon_box_restore_y = 0;
    core->pending_horizontal_column_write = 0x00u;
    core->pending_horizontal_attr_write = 0x00u;
}

static bool contra_init_lvl_nametable_animation_elapsed(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] - 0x20u);
        if (ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] > 0xDFu)
        {
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] - 1u);
        }

        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] - 1u);
        if ((ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] & 0x80u) == 0u)
        {
            return false;
        }

        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
        ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
        ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] = 0x40u;
        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x1Du;
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0xA0u;
        ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x23u;
        contra_load_next_supertiles_screen_indexes(core);
        return true;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u)
    {
        contra_write_horizontal_level_column_to_ppu(core);
        contra_write_horizontal_level_column_attributes_to_ppu(core);
    }

    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] + 1u);
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] + 1u);
    ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = (uint8_t)(ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] - 1u);

    if (ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] == 0u)
    {
        return true;
    }

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) || (ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] < 0x20u))
    {
        return false;
    }

    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x24u;
    ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] = 0x27u;
    ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] = 0x40u;
    contra_load_next_supertiles_screen_indexes(core);
    return false;
}

static void contra_apply_controller_state(ContraCore *core)
{
    uint8_t first_read[2];
    uint8_t second_read[2];
    int player_index;

    first_read[0] = core->pending_input.player[0];
    first_read[1] = core->pending_input.player[1];
    second_read[0] = core->pending_input.player[0];
    second_read[1] = core->pending_input.player[1];

    for (player_index = 1; player_index >= 0; --player_index)
    {
        if (second_read[player_index] != first_read[player_index])
        {
            second_read[player_index] = core->ram[CONTRA_RAM_CTRL_KNOWN_GOOD + (size_t)player_index];
        }
    }

    if ((core->ram[CONTRA_RAM_PLAYER_MODE_1D] & 0x04u) == 0u)
    {
        second_read[0] = (uint8_t)(second_read[0] | second_read[1]);
    }

    for (player_index = 1; player_index >= 0; --player_index)
    {
        const size_t state_offset = CONTRA_RAM_CONTROLLER_STATE + (size_t)player_index;
        const size_t diff_offset = CONTRA_RAM_CONTROLLER_STATE_DIFF + (size_t)player_index;
        const size_t known_good_offset = CONTRA_RAM_CTRL_KNOWN_GOOD + (size_t)player_index;
        const uint8_t current = second_read[player_index];
        const uint8_t previous = core->ram[known_good_offset];

        core->ram[diff_offset] = (uint8_t)((current ^ previous) & current);
        core->ram[state_offset] = current;
        core->ram[known_good_offset] = current;
    }
}

static void contra_init_game_routine_flags(ContraCore *core)
{
    core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] = 0x00u;
    core->ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] = 0x00u;
}

static void contra_increment_game_routine(ContraCore *core)
{
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] + 1u);
    contra_init_game_routine_flags(core);
}

static void contra_inc_routine_index_set_timer(ContraCore *core)
{
    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x80u;
    contra_increment_game_routine(core);
}

static void contra_reset_delay_timer(ContraCore *core)
{
    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x40u;
    core->ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x02u;
}

static void contra_set_game_routine_index(ContraCore *core, uint8_t game_routine_index)
{
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = game_routine_index;
    contra_reset_delay_timer(core);
    contra_init_game_routine_flags(core);
}

static bool contra_decrement_delay_timer_elapsed(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if ((uint8_t)(ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] | ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE]) == 0u)
    {
        return true;
    }

    ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = (uint8_t)(ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] - 1u);
    if (ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] != 0u)
    {
        return false;
    }

    if (ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] == 0u)
    {
        return false;
    }

    ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = (uint8_t)(ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] - 1u);
    return false;
}

static void contra_dec_intro_theme_delay(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u)
    {
        return;
    }

    if (ram[CONTRA_RAM_INTRO_THEME_DELAY] == 0u)
    {
        return;
    }

    ram[CONTRA_RAM_INTRO_THEME_DELAY] = (uint8_t)(ram[CONTRA_RAM_INTRO_THEME_DELAY] - 1u);
}

static void contra_load_intro_palette2_play_intro_sound(ContraCore *core)
{
    core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] = 0x00u;
    core->ram[CONTRA_RAM_PPUCTRL_SETTINGS] = 0xB0u;
    core->ram[CONTRA_RAM_INTRO_THEME_DELAY] = 0xA4u;
    contra_load_bank_6_write_text_palette_to_mem(core, 0x04u);
    contra_play_sound(core, 0x26u);
}

static void contra_konami_input_check(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t index = ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT];
    uint8_t input;

    if ((index & 0x80u) != 0u)
    {
        return;
    }

    input = (uint8_t)(ram[CONTRA_RAM_CONTROLLER_STATE_DIFF] & 0xCFu);
    if (input == 0u)
    {
        return;
    }

    if (input != contra_konami_code_lookup_table[index])
    {
        ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] = 0xFFu;
        return;
    }

    index = (uint8_t)(index + 1u);
    ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] = index;
    if (index >= 10u)
    {
        ram[CONTRA_RAM_KONAMI_CODE_STATUS] = 0x01u;
    }
}

static void contra_clear_memory_3(ContraCore *core)
{
    memset(&core->ram[0x28u], 0, 0xF0u - 0x28u);
    memset(&core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER], 0, CONTRA_RAM_CPU_GRAPHICS_BUFFER - CONTRA_RAM_CPU_SPRITE_BUFFER);
}

static void contra_end_demo_level(ContraCore *core)
{
    core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] = 0x01u;
}

static void contra_simulate_demo_input(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    int player_index;

    if ((ram[CONTRA_RAM_CONTROLLER_STATE_DIFF] & (CONTRA_BUTTON_START | CONTRA_BUTTON_SELECT)) != 0u)
    {
        contra_end_demo_level(core);
        return;
    }

    if (ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] != 0xFFu)
    {
        ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] = (uint8_t)(ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] + 1u);
        if (ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] == 0x00u)
        {
            ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] = 0xFFu;
        }
    }

    for (player_index = 1; player_index >= 0; --player_index)
    {
        const size_t player_offset = (size_t)player_index;
        uint8_t input = ram[CONTRA_RAM_DEMO_INPUT_VAL + player_offset];

        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u)
        {
            if (ram[CONTRA_RAM_DEMO_INPUT_NUM_FRAMES + player_offset] == 0u)
            {
                uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];
                const uint16_t table_addr = contra_rom_read_u16(
                    5u,
                    (uint16_t)(contra_demo_input_pointer_table_addr + (((uint16_t)level * 2u + (uint16_t)player_offset) * 2u))
                );
                const uint8_t table_index = ram[CONTRA_RAM_DEMO_INPUT_TBL_INDEX + player_offset];

                input = contra_rom_read_u8(5u, (uint16_t)(table_addr + table_index));
                if (input == 0xFFu)
                {
                    contra_end_demo_level(core);
                    return;
                }

                ram[CONTRA_RAM_DEMO_INPUT_VAL + player_offset] = input;
                ram[CONTRA_RAM_DEMO_INPUT_NUM_FRAMES + player_offset] =
                    contra_rom_read_u8(5u, (uint16_t)(table_addr + table_index + 1u));
                ram[CONTRA_RAM_DEMO_INPUT_TBL_INDEX + player_offset] = (uint8_t)(table_index + 2u);
            }

            ram[CONTRA_RAM_DEMO_INPUT_NUM_FRAMES + player_offset] =
                (uint8_t)(ram[CONTRA_RAM_DEMO_INPUT_NUM_FRAMES + player_offset] - 1u);
            input = ram[CONTRA_RAM_DEMO_INPUT_VAL + player_offset];
        }

        ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_offset] = input;
        ram[CONTRA_RAM_CONTROLLER_STATE + player_offset] = input;

        if (ram[CONTRA_RAM_DEMO_FIRE_DELAY_TIMER] >= 0x50u)
        {
            const uint8_t weapon = (uint8_t)(ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_offset] & 0x0Fu);
            const bool auto_fire = (weapon == 0x01u) || (weapon == 0x04u);

            if (auto_fire)
            {
                ram[CONTRA_RAM_CONTROLLER_STATE + player_offset] |= CONTRA_BUTTON_B;
            }
            else if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x07u) == 0u)
            {
                ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_offset] |= CONTRA_BUTTON_B;
            }
        }
    }
}

static void contra_set_next_demo_level(ContraCore *core)
{
    uint8_t demo_level;

    contra_clear_memory_3(core);
    core->ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x07u;
    core->ram[CONTRA_RAM_FRAME_COUNTER] = 0x00u;
    core->ram[CONTRA_RAM_RANDOM_NUM] = 0x00u;

    demo_level = core->ram[CONTRA_RAM_DEMO_LEVEL];
    if (demo_level >= 0x03u)
    {
        demo_level = 0x00u;
    }

    core->ram[CONTRA_RAM_DEMO_LEVEL] = demo_level;
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = demo_level;
    core->ram[CONTRA_RAM_DEMO_LEVEL] = (uint8_t)(demo_level + 1u);
    core->ram[CONTRA_RAM_P1_NUM_LIVES] = 0x62u;
    core->ram[CONTRA_RAM_P2_NUM_LIVES] = 0x62u;
}

static void contra_init_player_lives(ContraCore *core)
{
    uint8_t player_index;
    uint8_t initial_lives = 0x02u;

    player_index = core->ram[CONTRA_RAM_PLAYER_MODE] & 0x01u;
    core->ram[CONTRA_RAM_PLAYER_MODE_1D] = contra_player_mode_1d_table[player_index];
    core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = contra_p2_game_over_status_table[player_index];
    core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;

    if (core->ram[CONTRA_RAM_KONAMI_CODE_STATUS] != 0u)
    {
        initial_lives = 0x1Du;
    }

    do
    {
        core->ram[CONTRA_RAM_P1_NUM_LIVES + player_index] = initial_lives;
    } while (player_index-- != 0u);

    core->ram[CONTRA_RAM_EXTRA_LIFE_SCORE_LOW] = 0xC8u;
    core->ram[CONTRA_RAM_EXTRA_LIFE_SCORE_HIGH] = 0x00u;
    core->ram[CONTRA_RAM_EXTRA_LIFE_SCORE_HIGH + 1u] = 0xC8u;
}

static void contra_reset_players_score_and_lives(ContraCore *core)
{
    memset(&core->ram[CONTRA_RAM_PLAYER_1_SCORE_LOW], 0, 4u);
    core->ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
    contra_init_player_lives(core);
}

static void contra_init_score_player_lives(ContraCore *core)
{
    contra_clear_memory_3(core);
    core->ram[CONTRA_RAM_DEMO_LEVEL] = 0x00u;
    core->ram[CONTRA_RAM_NUM_CONTINUES] = 0x03u;

    memset(&core->ram[CONTRA_RAM_PLAYER_1_SCORE_LOW], 0, 4u);
    core->ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
    contra_init_player_lives(core);
    core->ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] = 0x00u;
}

static void contra_game_routine_00(ContraCore *core)
{
    contra_zero_out_nametables(core);
    contra_load_intro_graphics(core);

    core->ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] = 0x00u;
    core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] = 0x01u;
    core->ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x02u;
    core->ram[CONTRA_RAM_PPUCTRL_SETTINGS] = 0xB1u;

    contra_inc_routine_index_set_timer(core);
}

static void contra_load_intro_title_sprites(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_SPRITE_X_POS + 0u] = 0x2Cu;
    ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u] = 0xAAu;
    ram[CONTRA_RAM_SPRITE_Y_POS + 0u] = contra_player_select_cursor_pos[ram[CONTRA_RAM_PLAYER_MODE] & 0x01u];
    ram[CONTRA_RAM_SPRITE_ATTR + 0u] = 0x00u;
    ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 1u] = 0xABu;
    ram[CONTRA_RAM_SPRITE_X_POS + 1u] = 0xB3u;
    ram[CONTRA_RAM_SPRITE_Y_POS + 1u] = 0x77u;
    ram[CONTRA_RAM_SPRITE_ATTR + 1u] = 0x00u;
}

static void contra_game_routine_01(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    contra_konami_input_check(core);

    if (ram[CONTRA_RAM_HORIZONTAL_SCROLL] != 0u)
    {
        ram[CONTRA_RAM_HORIZONTAL_SCROLL] = (uint8_t)(ram[CONTRA_RAM_HORIZONTAL_SCROLL] + 1u);
        if (ram[CONTRA_RAM_HORIZONTAL_SCROLL] != 0u)
        {
            return;
        }

        contra_load_intro_palette2_play_intro_sound(core);
    }

    contra_load_intro_title_sprites(core);

    if (contra_decrement_delay_timer_elapsed(core))
    {
        contra_increment_game_routine(core);
    }
}

static void contra_game_routine_02(ContraCore *core)
{
    if (core->ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] == 0u)
    {
        core->ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] = (uint8_t)(core->ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] + 1u);
        contra_set_next_demo_level(core);
        return;
    }

    if (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u)
    {
        contra_simulate_demo_input(core);
    }

    contra_run_level_routine(core);

    if (core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] != 0u)
    {
        core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
        contra_set_game_routine_index(core, 0x00u);
    }
}

static void contra_game_routine_03(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] == 0u)
    {
        ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
        ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x40u;
        ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] = (uint8_t)(ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG] + 1u);
        return;
    }

    contra_dec_intro_theme_delay(core);

    if (ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] != 0u)
    {
        ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = (uint8_t)(ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] - 1u);
    }

    if ((uint8_t)(ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] | ram[CONTRA_RAM_INTRO_THEME_DELAY]) == 0u)
    {
        contra_increment_game_routine(core);
        return;
    }

    {
        uint8_t text_code = (uint8_t)(0x01u + (ram[CONTRA_RAM_PLAYER_MODE] & 0x01u));

        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x08u) != 0u)
        {
            text_code |= 0x80u;
        }

        contra_load_bank_6_write_text_palette_to_mem(core, text_code);
    }
}

static void contra_game_routine_04(ContraCore *core)
{
    contra_init_score_player_lives(core);
    contra_increment_game_routine(core);
}

static void contra_level_routine_00(ContraCore *core)
{
    contra_init_apu_channels(core);
    contra_zero_out_nametables(core);
    memset(core->enemies, 0, sizeof(core->enemies));
    memset(core->enemy_projectiles, 0, sizeof(core->enemy_projectiles));
    core->level1_weapon_box_restore_timer = 0x00u;
    core->level1_weapon_box_restore_x = 0;
    core->level1_weapon_box_restore_y = 0;
    core->pending_horizontal_column_write = 0x00u;
    core->pending_horizontal_attr_write = 0x00u;
    contra_load_bank_6_write_text_palette_to_mem(core, 0x06u);
    core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x00u;
    core->ram[CONTRA_RAM_LEVEL_END_PLAYERS_ALIVE] = 0x00u;
    contra_load_level_header(core);
    contra_init_ppu_write_screen_supertiles(core);
    contra_load_bank_0_load_level_enemies_to_mem(core);
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_level_routine_01(ContraCore *core)
{
    uint8_t delay = core->ram[CONTRA_RAM_DEMO_MODE];

    if (delay == 0u)
    {
        contra_draw_stage_and_level_name(core);
        core->ram[CONTRA_RAM_DRAW_PLAYER_INDEX] = 0x00u;
        contra_draw_player_num_lives(core);

        if (core->ram[CONTRA_RAM_PLAYER_MODE] != 0u)
        {
            core->ram[CONTRA_RAM_DRAW_PLAYER_INDEX] = core->ram[CONTRA_RAM_PLAYER_MODE];
            contra_draw_player_num_lives(core);
        }

        delay = 0xC0u;
    }

    core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = delay;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_level_routine_02(ContraCore *core)
{
    if (!contra_decrement_delay_timer_elapsed(core))
    {
        contra_draw_the_scores(core);
        return;
    }

    contra_zero_out_nametables(core);
    core->level_graphics_wait_frames = 0x08u;
}

static void contra_finish_level_graphics_load(ContraCore *core)
{
    const uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    contra_load_level_graphics(core);
    contra_load_palettes_color_to_cpu(core, 0x20u);

    if (level < 8u)
    {
        core->ram[CONTRA_RAM_VERTICAL_SCROLL] = contra_level_vert_scroll_and_song[level][0];

        if (core->ram[CONTRA_RAM_DEMO_MODE] == 0u)
        {
            contra_play_sound(core, contra_level_vert_scroll_and_song[level][1]);
        }
    }

    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0xFFu;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_level_routine_03(ContraCore *core)
{
    if (!contra_init_lvl_nametable_animation_elapsed(core))
    {
        return;
    }

    core->ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0xFFu;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_set_frame_scroll_draw_player_bullets(ContraCore *core)
{
    uint8_t active_players = 0x00u;

    core->ram[CONTRA_RAM_FRAME_SCROLL] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 0u] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] = 0x00u;

    if (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0u)
    {
        active_players |= 0x01u;
    }

    if (core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS] == 0u)
    {
        active_players |= 0x02u;
    }

    if (active_players != 0u)
    {
        core->ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD] = (uint8_t)(active_players - 1u);
    }
    else
    {
        core->ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD] = 0x00u;
    }

    if ((uint8_t)(core->ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] | core->ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01]) != 0u)
    {
        core->ram[CONTRA_RAM_FRAME_SCROLL] = 0x01u;
    }

    if (active_players == 0x01u)
    {
        contra_handle_invincibility_and_weapon_strength(core, 0u);
    }
    else if (active_players == 0x02u)
    {
        contra_handle_invincibility_and_weapon_strength(core, 1u);
    }
    else if (active_players == 0x03u)
    {
        contra_handle_invincibility_and_weapon_strength(core, 0u);
        contra_handle_invincibility_and_weapon_strength(core, 1u);
    }

    if (core->ram[CONTRA_RAM_INDOOR_SCROLL] >= 0x02u)
    {
        core->ram[CONTRA_RAM_INDOOR_SCROLL] = 0x00u;
    }

    if (core->ram[CONTRA_RAM_FRAME_SCROLL] == 0u)
    {
        contra_apply_outdoor_horizontal_frame_scroll(core, active_players);
    }

    contra_update_player_bullets(core);
    contra_draw_player_bullet_sprites(core);
}

/* ---------------------------------------------------------------------------
   Faithful real-RAM enemy system (work in progress).

   This is the in-progress replacement for the invented core->enemies[] layer:
   a direct port of the ROM's enemy spawn/dispatch onto the real ENEMY_* RAM
   arrays, so the frame-exact harness can validate enemy state against the ROM.

   It is built behind CONTRA_USE_ROM_ENEMY_SYSTEM (default 0) so each piece can
   land and be reviewed without disturbing the working invented layer until the
   replacement (spawn -> dispatch -> per-type routines -> render -> collision)
   is complete and validated. Default 1 (faithful); the first definition above wins.
   --------------------------------------------------------------------------- */
#ifndef CONTRA_USE_ROM_ENEMY_SYSTEM
#define CONTRA_USE_ROM_ENEMY_SYSTEM 1
#endif

/* Level 1 scripted enemy data, faithful to level_1_enemy_screen_* (bank2.asm).
   Triples: xx = x position, tt = (repeat << 6) | type, yy = (ypos & 0xF0) |
   attrs; 0xFF terminates a screen. Screen 0x09's capsule has repeat=1, so it
   carries an extra yy byte. */
static const uint8_t contra_l1_enemy_screen_00[] = {
    0x10u, 0x05u, 0x60u, 0x40u, 0x05u, 0x60u, 0x50u, 0x06u, 0xC0u,
    0x60u, 0x02u, 0xA1u, 0x80u, 0x05u, 0x60u, 0xF0u, 0x03u, 0x40u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_01[] = {0x90u, 0x06u, 0xC0u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_02[] = {0x20u, 0x12u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_03[] = {0x40u, 0x12u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_04[] = {
    0x00u, 0x04u, 0xA0u, 0x10u, 0x06u, 0x60u, 0x50u, 0x06u, 0x61u,
    0x60u, 0x03u, 0x43u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_05[] = {
    0x20u, 0x06u, 0x41u, 0x40u, 0x02u, 0xA2u, 0x80u, 0x04u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_06[] = {0x40u, 0x04u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_07[] = {
    0x20u, 0x07u, 0xA0u, 0xA0u, 0x07u, 0x41u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_08[] = {
    0x00u, 0x02u, 0xC3u, 0x50u, 0x06u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_09[] = {
    0x10u, 0x43u, 0x40u, 0xB4u, 0xE0u, 0x07u, 0x81u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_0a[] = {0xC0u, 0x04u, 0xC0u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_0b[] = {
    0x40u, 0x04u, 0xC3u, 0xA8u, 0x10u, 0x81u, 0xB1u, 0x11u, 0xB0u,
    0xB4u, 0x06u, 0x52u, 0xC0u, 0x10u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_0c[] = {0xFFu};

static const uint8_t *const contra_l1_enemy_screen_tbl[] = {
    contra_l1_enemy_screen_00, contra_l1_enemy_screen_01, contra_l1_enemy_screen_02,
    contra_l1_enemy_screen_03, contra_l1_enemy_screen_04, contra_l1_enemy_screen_05,
    contra_l1_enemy_screen_06, contra_l1_enemy_screen_07, contra_l1_enemy_screen_08,
    contra_l1_enemy_screen_09, contra_l1_enemy_screen_0a, contra_l1_enemy_screen_0b,
    contra_l1_enemy_screen_0c};

/* enemy_prop_00 (bank7.asm): 4 bytes/type — STATE_WIDTH, SCORE_COLLISION, HP,
   VAR_A — indexed by ENEMY_TYPE. Level 1 (and shared types < 0x10) use this. */
static const uint8_t contra_enemy_prop_00[][4] = {
    {0x82u, 0x22u, 0x01u, 0x00u}, {0x80u, 0x00u, 0x01u, 0x00u},
    {0x0Fu, 0x32u, 0xF0u, 0x00u}, {0x0Bu, 0x32u, 0x01u, 0x00u},
    {0x8Fu, 0x22u, 0x08u, 0x00u}, {0x83u, 0x10u, 0x01u, 0x00u},
    {0x83u, 0x30u, 0x01u, 0x00u}, {0x8Fu, 0x30u, 0x08u, 0x00u},
    {0x0Fu, 0x52u, 0xF1u, 0x00u}, {0x00u, 0x00u, 0x01u, 0x00u},
    {0x0Fu, 0x42u, 0xF0u, 0x00u}, {0x8Au, 0x05u, 0x01u, 0x00u},
    {0x83u, 0x42u, 0x01u, 0x00u}, {0x00u, 0x00u, 0x01u, 0x00u},
    {0x0Eu, 0x33u, 0x0Au, 0x00u}, {0x80u, 0x01u, 0x01u, 0x00u},
    {0x0Fu, 0x42u, 0x10u, 0x00u}, {0x0Cu, 0x82u, 0x20u, 0x00u},
    {0x89u, 0x00u, 0x01u, 0x00u}};

/* enemy_prop_01/02 level-2/4 entries (bank7:9196), indexed by type-0x10:
   {ENEMY_STATE_WIDTH, ENEMY_SCORE_COLLISION, ENEMY_HP, ENEMY_VAR_A}. The ROM
   selects this via enemy_prop_ptr_tbl[CURRENT_LEVEL] for types >= 0x10, so the
   indoor types get their own init (e.g. the wall turret's HP, the soldiers'
   collision box) instead of the level-1 table. */
static const uint8_t contra_enemy_prop_level2[][4] = {
    {0x8Du, 0x02u, 0x01u, 0x00u}, /* 0x10 boss eye */
    {0x2Fu, 0x22u, 0x05u, 0x00u}, /* 0x11 rollers */
    {0x81u, 0x03u, 0x01u, 0x00u}, /* 0x12 grenades */
    {0x9Fu, 0x35u, 0x04u, 0x00u}, /* 0x13 wall turret (wall cannon) */
    {0x9Fu, 0x05u, 0x01u, 0x00u}, /* 0x14 wall core */
    {0x13u, 0x16u, 0x01u, 0x00u}, /* 0x15 running indoor soldier */
    {0x13u, 0x16u, 0x01u, 0x00u}, /* 0x16 jumping indoor soldier */
    {0x13u, 0x36u, 0x01u, 0x00u}, /* 0x17 seeking guy (grenade launcher) */
    {0x13u, 0x16u, 0x01u, 0x00u}, /* 0x18 group of 4 */
    {0x89u, 0x00u, 0xF1u, 0x00u}, /* 0x19 indoor soldier generator */
    {0x81u, 0x00u, 0xF1u, 0x00u}, /* 0x1A roller generator */
    {0x8Fu, 0x13u, 0x02u, 0x01u}, /* 0x1B boss eye sphere projectile */
};

/* find_next_enemy_slot (bank7.asm:9024): first free slot scanning 15->0, or -1. */
static int contra_rom_find_next_enemy_slot(const ContraCore *core)
{
    int slot;

    for (slot = 0x0F; slot >= 0; --slot)
    {
        if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + (unsigned)slot] == 0u)
        {
            return slot;
        }
    }
    return -1;
}

/* find_bullet_slot (bank7.asm:9037): first slot whose ENEMY_TYPE == 1, else 0. */
static uint8_t contra_rom_find_bullet_slot(const ContraCore *core)
{
    int slot;

    for (slot = 0x0F; slot >= 0; --slot)
    {
        if (core->ram[CONTRA_RAM_ENEMY_TYPE + (unsigned)slot] == 0x01u)
        {
            return (uint8_t)slot;
        }
    }
    return 0u;
}

/* clear_enemy_pt_2..pt_4 (bank7.asm:9077): zero per-slot vars. ENEMY_TYPE/HP and
   ROUTINE/SPRITES are handled by the caller (initialize_enemy). */
static void contra_rom_clear_enemy_pt_2(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_POS + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] = 0u;
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_A + x] = 0u;
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0u;
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = 0u;
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0u;
}

/* initialize_enemy (bank7.asm:9109): set routine=1, sprite=1, clear vars, then
   load props (width/score/HP/var_a) from enemy_prop_00 by ENEMY_TYPE. Level 1
   uses enemy_prop_00 for both shared (<0x10) and level-specific types. */
static void contra_rom_initialize_enemy(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t type = ram[CONTRA_RAM_ENEMY_TYPE + x];

    ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x01u;
    core->l2_structure_tile[x] = 0u; /* no wall-structure tile drawn yet */
    core->l2_supertile[x] = 0xFFu;   /* no boss-room super-tile drawn yet */
    contra_rom_clear_enemy_pt_2(core, x);

    if ((type == 0x12u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u))
    {
        core->l1_bridge_gap_count = 0u; /* a fresh bridge -> drop stale collision gaps */
    }

    /* enemy_prop_ptr_tbl (bank7:9152): shared types (< 0x10) use the common
       table; level-specific types (>= 0x10) use the per-level table. */
    if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level2) / sizeof(contra_enemy_prop_level2[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level2[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level2[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level2[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level2[i][3];
        }
    }
    else if (type < (sizeof(contra_enemy_prop_00) / sizeof(contra_enemy_prop_00[0])))
    {
        ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_00[type][0];
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_00[type][1];
        ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_00[type][2];
        ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_00[type][3];
    }
}

/* load_screen_enemy_data (bank2.asm:1518), outdoor/horizontal level-1 path:
   spawn scripted enemies as the screen scrolls to their x position. */
static void contra_rom_load_screen_enemy_data(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    const uint8_t *data;
    uint8_t y;
    uint8_t x_raw;
    uint8_t xpos;
    uint8_t scroll;
    uint8_t distance;
    uint8_t type;
    uint8_t repeat;

    if (ram[CONTRA_RAM_CURRENT_LEVEL] != 0u)
    {
        return; /* level 1 only for now */
    }
    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        return; /* outdoor (horizontal) path only for now */
    }
    if (screen >= (sizeof(contra_l1_enemy_screen_tbl) / sizeof(contra_l1_enemy_screen_tbl[0])))
    {
        return;
    }
    data = contra_l1_enemy_screen_tbl[screen];

    y = ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET];
    x_raw = data[y];
    if (x_raw == 0xFFu)
    {
        return; /* end of screen data */
    }

    xpos = (uint8_t)(x_raw & 0xFEu);
    scroll = ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    if (xpos == scroll)
    {
        distance = 0u;
    }
    else if (xpos > scroll)
    {
        return; /* scroll hasn't reached this enemy's position yet */
    }
    else
    {
        distance = (uint8_t)((uint8_t)((uint8_t)(xpos - scroll) ^ 0xFFu) + 1u);
    }

    ++y;
    type = (uint8_t)(data[y] & 0x3Fu);
    repeat = (uint8_t)((data[y] >> 6) & 0x03u);

    for (;;)
    {
        const uint8_t byte3 = data[(uint8_t)(y + 1u)];
        int slot = contra_rom_find_next_enemy_slot(core);

        ++y;
        if (slot < 0)
        {
            /* no free slot: ROM only steals a bullet slot if the enemy x's low
               bit is set; otherwise it skips placement for this repeat. */
            if ((x_raw & 0x01u) != 0u)
            {
                slot = (int)contra_rom_find_bullet_slot(core);
            }
        }
        if (slot >= 0)
        {
            const uint8_t sx = (uint8_t)slot;

            ram[CONTRA_RAM_ENEMY_TYPE + sx] = type;
            contra_rom_initialize_enemy(core, sx);
            ram[CONTRA_RAM_ENEMY_ATTRIBUTES + sx] = (uint8_t)(byte3 & 0x0Fu);
            ram[CONTRA_RAM_ENEMY_Y_POS + sx] = (uint8_t)(byte3 & 0xF0u);
            ram[CONTRA_RAM_ENEMY_X_POS + sx] = (uint8_t)(0xF0u - distance);
        }

        if (repeat == 0u)
        {
            break;
        }
        --repeat;
    }

    ++y;
    ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = y;
}

/* --- shared enemy helpers (bank7.asm), real-RAM ports --- */

/* clear_enemy (bank7.asm:9070): zero a slot's state, freeing it. */
static void contra_rom_clear_enemy(ContraCore *core, uint8_t x)
{
    if (core->ram[CONTRA_RAM_ENEMY_TYPE + x] == 0x17u)
    {
        core->ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0u; /* launcher gone -> resume generation */
    }
    core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_HP + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_TYPE + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0u;
    core->l2_structure_tile[x] = 0u;
    core->l2_supertile[x] = 0xFFu;
    contra_rom_clear_enemy_pt_2(core, x);
}

/* add_a_to_enemy_y_pos / add_a_to_enemy_x_pos (bank7.asm:8387/8394). */
static void contra_rom_add_a_to_enemy_y_pos(ContraCore *core, uint8_t x, uint8_t a)
{
    core->ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_POS + x] + a);
}

static void contra_rom_add_a_to_enemy_x_pos(ContraCore *core, uint8_t x, uint8_t a)
{
    core->ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_X_POS + x] + a);
}

/* add_scroll_to_enemy_pos (bank7.asm:7824), horizontal: X -= FRAME_SCROLL,
   remove the enemy if it scrolls off the left edge (X < 0x08). */
static void contra_rom_add_scroll_to_enemy_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t new_x = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - ram[CONTRA_RAM_FRAME_SCROLL]);

    ram[CONTRA_RAM_ENEMY_X_POS + x] = new_x;
    if (new_x < 0x08u)
    {
        contra_rom_clear_enemy(core, x);
    }
}

/* advance_enemy_routine (bank7.asm:7591): ++ENEMY_ROUTINE if non-zero. */
static void contra_rom_advance_enemy_routine(ContraCore *core, uint8_t x)
{
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] != 0u)
    {
        core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] =
            (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] + 1u);
    }
}

/* enable_enemy_collision (bank7.asm:8376): ENEMY_STATE_WIDTH &= 0x7E (clear the
   inactive bit 7 and the skip-collision bit 0). */
static void contra_rom_enable_enemy_collision(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Eu);
}

/* enable_bullet_enemy_collision (bank7.asm:8371): ENEMY_STATE_WIDTH &= 0x7F
   (clear bit 7 only, so bullets hit but the body stays non-collidable). */
static void contra_rom_enable_bullet_collision(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu);
}

/* --- enemy bullets (type 0x01), bank0/bank7 --- */
/* quadrant aim direction -> offset into the fractional-velocity table */
static const uint8_t contra_bullet_fract_vel_dir_lookup_tbl[24] = {
    0x00u, 0x02u, 0x04u, 0x06u, 0x08u, 0x0Au, 0x0Cu, 0x0Au, 0x08u, 0x06u, 0x04u, 0x02u,
    0x00u, 0x02u, 0x04u, 0x06u, 0x08u, 0x0Au, 0x0Cu, 0x0Au, 0x08u, 0x06u, 0x04u, 0x02u};
/* {y_fract, x_fract} pairs (fast byte starts 0); bank7 bullet_fract_vel_tbl */
static const uint8_t contra_bullet_fract_vel_tbl[14] = {
    0x00u, 0xFFu, 0x42u, 0xF7u, 0x80u, 0xDDu, 0xB5u, 0xB5u, 0xDDu, 0x80u, 0xF7u, 0x42u, 0xFFu, 0x00u};
static const uint8_t contra_bullet_sprite_tbl[6] = {0x1Eu, 0x21u, 0x21u, 0x1Eu, 0x79u, 0x07u};
static const uint8_t contra_bullet_palette_tbl[6] = {0x01u, 0x02u, 0x02u, 0x01u, 0x01u, 0x02u};
static const uint8_t contra_bullet_collision_code_tbl[6] = {0x01u, 0x05u, 0x05u, 0x01u, 0x02u, 0x00u};
/* adjust_bullet_velocity speed scaling reduces to vel*mult/8: 0.5x .. 1.875x */
static const uint8_t contra_bullet_speed_mult[8] = {4u, 6u, 8u, 10u, 12u, 13u, 14u, 15u};

static uint16_t contra_rom_adjust_bullet_velocity(uint8_t fract, uint8_t speed)
{
    return (uint16_t)(((uint16_t)fract * contra_bullet_speed_mult[speed & 0x07u]) >> 3u);
}

/* create_enemy_bullet (bank7): spawn a type-1 enemy bullet at (px,py) aimed by
   (angle, quadrant: bit0 up, bit1 left) at the given speed. */
static bool contra_rom_create_enemy_bullet(
    ContraCore *core, uint8_t btype, uint8_t angle, uint8_t quadrant, uint8_t speed,
    uint8_t px, uint8_t py)
{
    uint8_t *const ram = core->ram;
    const int slot = contra_rom_find_next_enemy_slot(core);
    uint8_t sx;
    uint8_t idx;
    uint16_t xv;
    uint16_t yv;

    if (slot < 0)
    {
        return false;
    }
    sx = (uint8_t)slot;
    ram[CONTRA_RAM_ENEMY_TYPE + sx] = 0x01u;
    contra_rom_initialize_enemy(core, sx);
    ram[CONTRA_RAM_ENEMY_VAR_1 + sx] = btype;
    ram[CONTRA_RAM_ENEMY_Y_POS + sx] = py;
    ram[CONTRA_RAM_ENEMY_X_POS + sx] = px;

    idx = contra_bullet_fract_vel_dir_lookup_tbl[angle % 24u];
    xv = contra_rom_adjust_bullet_velocity(contra_bullet_fract_vel_tbl[idx + 1u], speed);
    yv = contra_rom_adjust_bullet_velocity(contra_bullet_fract_vel_tbl[idx], speed);
    if ((quadrant & 0x01u) != 0u)
    {
        yv = (uint16_t)(0u - yv); /* up */
    }
    if ((quadrant & 0x02u) != 0u)
    {
        xv = (uint16_t)(0u - xv); /* left */
    }
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + sx] = (uint8_t)(yv >> 8u);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + sx] = (uint8_t)yv;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + sx] = (uint8_t)(xv >> 8u);
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + sx] = (uint8_t)xv;
    return true;
}

/* enemy_bullet_routine_00/01 (bank0:377): set collision code, then each frame
   set the bullet sprite and apply velocity. */
static void contra_rom_enemy_bullet_routine_00(ContraCore *core, uint8_t x)
{
    const uint8_t btype = core->ram[CONTRA_RAM_ENEMY_VAR_1 + x];

    core->ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] =
        contra_bullet_collision_code_tbl[(btype < 6u) ? btype : 0u];
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_enemy_bullet_routine_01(ContraCore *core, uint8_t x)
{
    const uint8_t btype = (core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] < 6u)
        ? core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] : 0u;

    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_bullet_sprite_tbl[btype];
    core->ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = contra_bullet_palette_tbl[btype];
    contra_rom_update_enemy_pos(core, x); /* removes itself off-screen */
}

/* --- sniper (enemy type 0x06), bank0.asm:1738 --- */
static const uint8_t contra_sniper_animation_delay_tbl[3] = {0x01u, 0x30u, 0x80u};
static const uint8_t contra_sniper_frame_tbl[3] = {0x03u, 0x00u, 0x00u};
static const uint8_t contra_sniper_attack_delay_tbl[3] = {0x40u, 0x04u, 0x10u};
static const uint8_t contra_sniper_bullet_attack_count_tbl[3] = {0x03u, 0x01u, 0x01u};

/* sniper sprite codes by ENEMY_FRAME (bank0.asm:2073): regular/hiding vs boss. */
static const uint8_t contra_sniper_sprite_00[7] = {0x44u, 0x45u, 0x46u, 0x43u, 0x42u, 0x41u, 0x29u};
static const uint8_t contra_sniper_sprite_01[7] = {0x44u, 0x45u, 0x46u, 0x2Cu, 0x42u, 0x2Du, 0x29u};

/* set_soldier... sniper_set_sprite (bank0.asm:1709-area): sprite code from
   ENEMY_FRAME and sniper type, flip from firing angle (VAR_2), recoil (VAR_3). */
static void contra_rom_sniper_set_sprite(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t *const tbl =
        (ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] >= 0x02u) ? contra_sniper_sprite_01 : contra_sniper_sprite_00;
    const uint8_t frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    uint8_t attr;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = tbl[(frame < 7u) ? frame : 0u];
    attr = ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x01u) == 0u) ? 0x40u : 0x00u;
    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
        attr = (uint8_t)(attr | 0x08u);
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = attr;
}

/* sniper_routine_02 (bank0.asm): render, track scroll, run the attack cadence.
   DEFERRED: the actual bullet creation (no enemy-bullet system yet) — when the
   sniper would fire, the delay is reset but no bullet spawns. */
static void contra_rom_sniper_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t attr;

    contra_rom_sniper_set_sprite(core, x);
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
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
    if ((int8_t)ram[CONTRA_RAM_ENEMY_VAR_4 + x] >= 0)
    {
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x18u;
        if (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] != 0u)
        {
            /* fire a bullet at the nearest player (horizontal aim; the ROM's
               diagonal angle solve is deferred) */
            static const uint8_t sniper_bullet_speed[3] = {0x03u, 0x05u, 0x03u};
            const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
            const uint8_t p0 = ram[CONTRA_RAM_SPRITE_X_POS + 0u];
            const uint8_t p1 = ram[CONTRA_RAM_SPRITE_X_POS + 1u];
            const uint8_t d0 = (p0 >= ex) ? (uint8_t)(p0 - ex) : (uint8_t)(ex - p0);
            const uint8_t d1 = (p1 >= ex) ? (uint8_t)(p1 - ex) : (uint8_t)(ex - p1);
            const uint8_t player_x = ((p1 != 0u) && ((p0 == 0u) || (d1 < d0))) ? p1 : p0;
            const bool firing_left = (player_x < ex);
            const uint8_t idx = (attr < 3u) ? attr : 0u;

            ram[CONTRA_RAM_ENEMY_VAR_2 + x] = firing_left ? 0u : 1u;
            (void)contra_rom_create_enemy_bullet(
                core, 0u, 0u, firing_left ? 0x02u : 0x00u, sniper_bullet_speed[idx],
                (uint8_t)(ex + (firing_left ? 0xF3u : 0x0Du)),
                (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0xEEu));
            ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x06u; /* gun recoil */
        }
        return;
    }

    attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    if (attr == 0u)
    {
        /* standing sniper: reset the burst and the between-attack delay */
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = contra_sniper_bullet_attack_count_tbl[0];
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x80u;
    }
    else
    {
        /* crouching / boss sniper: re-hide and advance to the post-attack routine */
        ram[CONTRA_RAM_ENEMY_FRAME + x] = ((attr & 0x01u) != 0u) ? 0x02u : 0x03u;
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x80u;
        contra_rom_advance_enemy_routine(core, x);
    }
}

/* sniper_routine_00 (bank0.asm:1751): init delay/frame from attributes, nudge Y
   down (+4, plus +5 for the crouching sniper), advance. */
static void contra_rom_sniper_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    const uint8_t idx = (attr < 3u) ? attr : 0u;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = contra_sniper_animation_delay_tbl[idx];
    ram[CONTRA_RAM_ENEMY_FRAME + x] = contra_sniper_frame_tbl[idx];
    contra_rom_add_a_to_enemy_y_pos(core, x, 0x04u);
    if (attr == 0x01u)
    {
        contra_rom_add_a_to_enemy_y_pos(core, x, 0x05u);
    }
    contra_rom_advance_enemy_routine(core, x);
}

/* sniper_routine_01 (bank0.asm:1787): set sprite, track scroll, run the crouch
   animation for hiding snipers, then enable collision and advance to attack. */
static void contra_rom_sniper_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t attr;

    contra_rom_sniper_set_sprite(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return; /* scrolled off and removed */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }

    attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    if (attr != 0u)
    {
        /* crouching / boss sniper: cycle the un-hiding animation */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x03u)
        {
            return;
        }
        if (attr == 0x01u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
            /* dec left frame at 2 (non-zero) -> fall through to enable collision */
        }
        else
        {
            contra_rom_add_a_to_enemy_y_pos(core, x, 0xF2u); /* -14 */
            contra_rom_add_a_to_enemy_x_pos(core, x, 0x01u);
        }
    }

    contra_rom_enable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x30u;
    {
        const uint8_t idx = (attr < 3u) ? attr : 0u;
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = contra_sniper_attack_delay_tbl[idx];
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = contra_sniper_bullet_attack_count_tbl[idx];
    }
    contra_rom_advance_enemy_routine(core, x);
}

/* set_enemy_routine_to_a (bank7.asm:7698): ENEMY_ROUTINE = a. */
static void contra_rom_set_enemy_routine_to_a(ContraCore *core, uint8_t x, uint8_t a)
{
    core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] = a;
}

/* set_enemy_delay_adv_routine (bank7.asm:7585): ENEMY_ANIMATION_DELAY = a; advance. */
static void contra_rom_set_enemy_delay_adv_routine(ContraCore *core, uint8_t x, uint8_t a)
{
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = a;
    contra_rom_advance_enemy_routine(core, x);
}

/* set_carry_if_past_trigger_point (bank0.asm:947), horizontal: true if the
   enemy has scrolled left past trigger_x (ENEMY_X_POS < trigger_x). */
static bool contra_rom_past_trigger_x(const ContraCore *core, uint8_t x, uint8_t trigger_x)
{
    return core->ram[CONTRA_RAM_ENEMY_X_POS + x] < trigger_x;
}

/* set_weapon_box_supertile (bank0.asm:603): draw the pill-box background
   super-tile for ENEMY_FRAME, at the enemy position. Called from the routine on
   each animation step (like the ROM) -- not every render frame -- so the box
   doesn't flicker. */
static const uint8_t contra_weapon_box_supertile_tbl[3] = {0x00u, 0x01u, 0x02u};
static bool contra_rom_set_weapon_box_supertile(ContraCore *core, uint8_t x)
{
    const uint8_t frame = core->ram[CONTRA_RAM_ENEMY_FRAME + x];

    contra_render_level_1_nametable_update_supertile(
        core,
        (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        contra_weapon_box_supertile_tbl[(frame < 3u) ? frame : 0u]);
    return false; /* carry clear == drew successfully */
}

/* weapon_box_routine_00 (bank0.asm:518): init frame, delay, advance. */
static void contra_rom_weapon_box_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x01u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x20u);
}

/* weapon_box_routine_01 (bank0.asm:529): track scroll; once scrolled past the
   activation trigger, start the open animation; close if near the left edge. */
static void contra_rom_weapon_box_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (!contra_rom_past_trigger_x(core, x, 0xF0u))
    {
        return; /* not yet at activation point */
    }
    if (contra_rom_past_trigger_x(core, x, 0x18u))
    {
        contra_rom_set_enemy_routine_to_a(core, x, 0x04u); /* near left edge: close */
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u); /* -> routine_02 */
}

/* weapon_box_routine_02 (bank0.asm:550): open/close animation cycle (HP toggles
   between 0xF0 while open/invulnerable-frame and 0x01 while closed). */
static void contra_rom_weapon_box_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (contra_rom_past_trigger_x(core, x, 0x18u))
    {
        contra_rom_set_enemy_routine_to_a(core, x, 0x04u);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;

    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        /* @open_weapon_box: animate toward open */
        if (contra_rom_set_weapon_box_supertile(core, x))
        {
            return;
        }
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] != 0u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
            return;
        }
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        ram[CONTRA_RAM_ENEMY_HP + x] = 0xF0u;
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
    }
    else
    {
        /* closed: animate toward closed */
        if (contra_rom_set_weapon_box_supertile(core, x))
        {
            return;
        }
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x02u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
            return;
        }
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
        ram[CONTRA_RAM_ENEMY_HP + x] = 0x01u;
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x01u;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x40u;
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u); /* -> routine_01 */
}

/* weapon_box_routine_03 (bank0.asm:617): deactivated; draw closed, then remove. */
static void contra_rom_weapon_box_routine_03(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (contra_rom_set_weapon_box_supertile(core, x))
    {
        return; /* buffer full: drawn next frame */
    }
    contra_rom_clear_enemy(core, x); /* remove_enemy */
}

/* add_4_to_enemy_y_pos (bank7.asm): ENEMY_Y_POS += 4. */
static void contra_rom_add_4_to_enemy_y_pos(ContraCore *core, uint8_t x)
{
    contra_rom_add_a_to_enemy_y_pos(core, x, 0x04u);
}

/* add_y_to_y_pos_get_bg_collision (bank7.asm:8679): bg collision code at
   (ENEMY_X_POS, ENEMY_Y_POS + y_off) without modifying the stored position.
   Codes match the ROM: 0 empty, 1 floor, 2 water, 0x80 solid. */
static uint8_t contra_rom_add_y_to_y_pos_get_bg_collision(const ContraCore *core, uint8_t x, uint8_t y_off)
{
    const uint8_t ey = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_POS + x] + y_off);
    return contra_get_outdoor_horizontal_bg_collision(core, core->ram[CONTRA_RAM_ENEMY_X_POS + x], ey);
}

/* set_enemy_y_velocity_to_0 (bank7.asm). */
static void contra_rom_set_enemy_y_velocity_to_0(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = 0u;
}

/* --- soldier / running man (enemy type 0x05), bank0.asm:1217 --- */
static const uint8_t contra_soldier_initial_anim_delay_tbl[4] = {0x01u, 0x10u, 0x20u, 0x30u};
/* soldier_x_vel_tbl: {fract,fast} for left/right, horizontal then vertical. */
static const uint8_t contra_soldier_x_vel_tbl[8] = {0x00u, 0xFFu, 0x40u, 0x01u, 0x00u, 0xFFu, 0x00u, 0x01u};

/* soldier_set_x_velocity (bank0.asm:1242): set X velocity from ENEMY_VAR_2
   direction (0 left, 1 right) and the level scrolling type. */
static void contra_rom_soldier_set_x_velocity(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t base = (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ? 0x04u : 0x00u;
    const uint8_t off = (uint8_t)((uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] << 1u) + base);

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_soldier_x_vel_tbl[off];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_soldier_x_vel_tbl[off + 1u];
}

/* soldier_stop_y_set_x_velocity (bank0.asm:1237): set X velocity, zero Y. */
static void contra_rom_soldier_stop_y_set_x_velocity(ContraCore *core, uint8_t x)
{
    contra_rom_soldier_set_x_velocity(core, x);
    contra_rom_set_enemy_y_velocity_to_0(core, x);
}

/* soldier_routine_00 (bank0.asm:1217): track scroll, nudge to ground, set the
   initial animation delay from attributes, advance. */
static void contra_rom_soldier_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t idx;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    contra_rom_add_4_to_enemy_y_pos(core, x);
    idx = (uint8_t)((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] >> 4u) & 0x03u);
    contra_rom_set_enemy_delay_adv_routine(core, x, contra_soldier_initial_anim_delay_tbl[idx]);
}

/* soldier_routine_01 (bank0.asm:1275): tick the spawn delay (faithful to the
   ROM's quirky double-decrement for left-runners), then verify there is ground
   under the soldier, enable collision, pick a direction and walk velocity, and
   advance to the walk routine. Removes the soldier if there is no ground (e.g.
   a destroyed bridge). */
static void contra_rom_soldier_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    bool enable_set_vel = false;

    /* horizontal level (level 1) */
    if (ram[CONTRA_RAM_FRAME_SCROLL] == 0u)
    {
        /* @dec_delay_enable_set_vel */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            return;
        }
        enable_set_vel = true;
    }
    else if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u)
    {
        /* running right: only tick on odd frames */
        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) == 0u)
        {
            return;
        }
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            return;
        }
        enable_set_vel = true;
    }
    else
    {
        /* running left: @continue — decrement, and if not yet zero, decrement
           again (the ROM falls through @continue into @dec_delay_enable_set_vel) */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
            if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
            {
                return;
            }
        }
        enable_set_vel = true;
    }

    if (!enable_set_vel)
    {
        return;
    }

    /* @enable_set_vel: require ground #$10 below, else remove */
    if (contra_rom_add_y_to_y_pos_get_bg_collision(core, x, 0x10u) == 0u)
    {
        contra_rom_clear_enemy(core, x); /* no ground (e.g. destroyed bridge) */
        return;
    }
    contra_rom_enable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_X_POS + x] = 0x0Au; /* running right: enter from left */
    }
    contra_rom_soldier_stop_y_set_x_velocity(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x10u); /* -> routine_02 (walk) */
}

/* update_enemy_x_pos / update_enemy_y_pos (bank7.asm:7736-): apply the 16-bit
   fixed-point velocity (FAST.FRACT via the accumulator) to the position. */
static void contra_rom_update_enemy_x_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint16_t accum =
        (uint16_t)ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] + ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x];

    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] = (uint8_t)accum;
    ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] +
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] + (uint8_t)(accum >> 8u));
}

static void contra_rom_update_enemy_y_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint16_t accum =
        (uint16_t)ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] + ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x];

    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = (uint8_t)accum;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] +
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (uint8_t)(accum >> 8u));
}

/* update_enemy_pos (bank7.asm:7736), horizontal: apply X velocity then subtract
   the frame scroll; apply Y velocity; remove if off the left/bottom edge. */
static void contra_rom_update_enemy_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_update_enemy_x_pos(core, x);
    ram[CONTRA_RAM_ENEMY_X_POS + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - ram[CONTRA_RAM_FRAME_SCROLL]);
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
    {
        contra_rom_clear_enemy(core, x);
        return;
    }
    contra_rom_update_enemy_y_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xE8u)
    {
        contra_rom_clear_enemy(core, x);
    }
}

/* add_a_y_to_enemy_pos_get_bg_collision (bank7.asm:8692): bg collision at
   (ENEMY_X_POS + a, ENEMY_Y_POS + y_off), positions unchanged. */
static uint8_t contra_rom_add_a_y_to_enemy_pos_get_bg_collision(
    const ContraCore *core, uint8_t x, uint8_t a, uint8_t y_off)
{
    const uint8_t ex = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_X_POS + x] + a);
    const uint8_t ey = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_POS + x] + y_off);

    return contra_get_outdoor_horizontal_bg_collision(core, ex, ey);
}

/* set_soldier_sprite (bank0.asm:1709): sprite code from ENEMY_FRAME, flip when
   running left, recoil bit while firing. */
static const uint8_t contra_soldier_sprite_codes[12] = {
    0x3Bu, 0x3Cu, 0x3Du, 0x3Fu, 0x3Cu, 0x3Eu, 0x40u, 0x26u, 0x73u, 0x18u, 0x28u, 0x27u};
static void contra_rom_set_soldier_sprite(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    uint8_t attr;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_soldier_sprite_codes[(frame < 12u) ? frame : 0u];
    attr = (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u) ? 0x40u : 0x00u;
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
        attr = (uint8_t)(attr | 0x08u);
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = attr;
}

/* soldier_change_direction (bank0.asm): flip direction, reset X velocity. */
static void contra_rom_soldier_change_direction(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 1u);
    core->ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_VAR_2 + x] ^ 0x01u);
    contra_rom_soldier_set_x_velocity(core, x);
}

/* soldier_routine_02 (bank0.asm:1323): walk. Animate the run cycle, step in the
   facing direction (velocity + scroll), and turn around at an edge.
   DEFERRED (clearly not yet faithful): the RNG-driven jump-off-ledge (uses the
   busy-loop RANDOM_NUM, unreproducible) and firing (routine_03); at an edge the
   soldier simply turns instead of jumping for now. */
static void contra_rom_soldier_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t code;

    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0u; /* TODO: port the jump arc */
    }

    /* @continue_walk_routine: advance the 6-frame run animation every 8 ticks */
    ram[CONTRA_RAM_ENEMY_VAR_A + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_A + x] + 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_A + x] & 0x07u) == 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= 0x06u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = 0u;
        }
    }

    /* @soldier_move: is there ground one step ahead and #$10 below? */
    code = contra_rom_add_a_y_to_enemy_pos_get_bg_collision(
        core, x, ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x], 0x10u);
    if ((code != 0x80u) && (code != 0x01u))
    {
        contra_rom_soldier_change_direction(core, x); /* edge: turn (jump deferred) */
    }

    contra_rom_set_soldier_sprite(core, x);
    contra_rom_update_enemy_pos(core, x);
}

/* --- flying capsule / weapon zeppelin (enemy type 0x03), bank0.asm:680 --- */

/* set_flying_capsule_y_vel + set_flying_capsule_path (bank7.asm:8712/8765):
   harmonic weave -- pull the Y velocity toward the base height VAR_1 by
   subtracting 2*(ENEMY_Y_POS - VAR_1) (16-bit) from the Y velocity. */
static void contra_rom_set_flying_capsule_y_vel(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t pos = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    const uint8_t base = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    uint16_t dist = (uint16_t)(((pos < base) ? 0xFF00u : 0x0000u) | (uint8_t)(pos - base));
    uint16_t vel;

    dist = (uint16_t)(dist << 1u); /* shift count = 1 (outdoor) */
    vel = (uint16_t)(((uint16_t)ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] << 8u) |
                     ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x]);
    vel = (uint16_t)(vel - dist);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = (uint8_t)(vel >> 8u);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)vel;
}

/* flying_capsule_routine_00 (bank0.asm:680): record the base position, enter
   from the left, set the cruise velocity (right + weave), advance. */
static void contra_rom_flying_capsule_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x03u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = ram[CONTRA_RAM_ENEMY_X_POS + x];
    contra_rom_add_a_to_enemy_y_pos(core, x, 0x20u);
    ram[CONTRA_RAM_ENEMY_X_POS + x] = 0x10u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u; /* flying_capsule_vel_tbl[0] */
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x80u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x01u;
    contra_rom_advance_enemy_routine(core, x);
}

/* flying_capsule_routine_01 (bank0.asm): sprite 0x4D, weave the Y velocity,
   apply velocity + scroll. */
static void contra_rom_flying_capsule_routine_01(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x4Du;
    contra_rom_set_flying_capsule_y_vel(core, x);
    contra_rom_update_enemy_pos(core, x);
}

/* enemy_routine_explosion (bank7.asm:7616): animate the explosion sprite
   sequence (explosion_type_00) then remove. A killed enemy becomes a shared
   explosion actor (ENEMY_TYPE 0xFE) running this; that's a small simplification
   of the ROM (which keeps the type and uses per-type explosion routine slots)
   but produces the faithful explosion sprites. */
static void contra_rom_enemy_routine_explosion(ContraCore *core, uint8_t x)
{
    static const uint8_t explosion_type_00[3] = {0x38u, 0x39u, 0x3Au};
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
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= 0x03u)
    {
        contra_rom_clear_enemy(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = explosion_type_00[ram[CONTRA_RAM_ENEMY_FRAME + x]];
}

/* Start the explosion actor on the enemy slot (called from the kill path). */
static void contra_rom_begin_enemy_explosion(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_TYPE + x] == 0x17u)
    {
        ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0u; /* grenade_launcher_routine_06 */
    }
    ram[CONTRA_RAM_ENEMY_TYPE + x] = 0xFEu;
    ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    /* enemy_routine_init_explosion (bank7:7546): ora #$81 -- set bit 7 (bullets
       pass through) AND bit 0 (skip player-body collision), so the explosion of a
       killed enemy can't damage the player who walks into it. */
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] | 0x81u);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x38u;
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x00u;
}

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
static void contra_rom_aim_and_create_enemy_bullet(
    ContraCore *core, uint8_t sx, uint8_t sy, uint8_t btype, uint8_t speed, const uint8_t *tbl)
{
    uint8_t *const ram = core->ram;
    const uint8_t p0 = ram[CONTRA_RAM_SPRITE_X_POS + 0u];
    const uint8_t p1 = ram[CONTRA_RAM_SPRITE_X_POS + 1u];
    const uint8_t d0 = (p0 >= sx) ? (uint8_t)(p0 - sx) : (uint8_t)(sx - p0);
    const uint8_t d1 = (p1 >= sx) ? (uint8_t)(p1 - sx) : (uint8_t)(sx - p1);
    uint8_t idx = ((p1 != 0u) && ((p0 == 0u) || (d1 < d0))) ? 1u : 0u;
    uint8_t tx;
    uint8_t ty;
    uint8_t quadrant;
    uint8_t nibble;

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
        ty = (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
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

static void contra_rom_wall_turret_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u)
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
static void contra_rom_wall_turret_routine_04(ContraCore *core, uint8_t x)
{
    core->l2_structure_tile[x] = 0x83u; /* 'core - destroyed' */
    contra_rom_begin_enemy_explosion(core, x);
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

    if (((attr & 0x08u) == 0u) && (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0u))
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
   happened and the core is open (unplated) and high enough, fire at the player on
   a cadence. The ROM aims diagonally (aim_and_create_enemy_bullet); the exact
   diagonal solve is RNG/aim-table bound, so fire horizontally at the nearest
   player as an approximation (bullet type 0x03, speed code 0x05). */
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

/* wall_core_routine_05/06 (bank7:e737 / enemy_routine_explosion): play the core's
   own explosion (sprites 0x38..0x3a) -- bit7 of STATE_WIDTH stops further bullet
   hits so the chain isn't re-entered -- then advance to the room/blow-open. */
static const uint8_t contra_wall_core_explosion_sprite_tbl[3] = {0x38u, 0x39u, 0x3Au};

static void contra_rom_wall_core_routine_05(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] | 0x80u); /* no more bullet hits */
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0u;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_wall_core_explosion_sprite_tbl[0];
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    contra_rom_advance_enemy_routine(core, x); /* -> routine_06 */
}

static void contra_rom_wall_core_routine_06(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= 0x03u)
    {
        contra_rom_advance_enemy_routine(core, x); /* -> routine_07 */
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        contra_wall_core_explosion_sprite_tbl[ram[CONTRA_RAM_ENEMY_FRAME + x]];
}

/* wall_core_routine_07 (bank0:3333): one fewer core to destroy; if this was the
   last, hide it, wipe the other enemies, and start the back-wall blow-open. */
static void contra_rom_wall_core_routine_07(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    int slot;

    ram[CONTRA_RAM_WALL_CORE_REMAINING] =
        (uint8_t)(ram[CONTRA_RAM_WALL_CORE_REMAINING] - 1u);
    if (ram[CONTRA_RAM_WALL_CORE_REMAINING] != 0u)
    {
        contra_rom_clear_enemy(core, x); /* not the last core -- just remove it */
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x00u; /* hide the core */
    for (slot = 0x0F; slot >= 0; --slot)
    {
        if ((uint8_t)slot != x)
        {
            contra_rom_clear_enemy(core, (uint8_t)slot); /* destroy_all_enemies */
        }
    }
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
        return;
    }
    q = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x03u);
    core->l2_blowopen_quadrants = (uint8_t)(core->l2_blowopen_quadrants | (uint8_t)(1u << q));
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
    contra_rom_clear_enemy(core, x);
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

/* reverse_enemy_x_direction (bank7): negate the 16-bit X velocity. */
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
            contra_rom_clear_enemy(core, x);
            return true;
        }
    }
    else if (newx >= 0xB0u)
    {
        contra_rom_clear_enemy(core, x);
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

/* create_indoor_bullet (bank0): fire a regular bullet aimed by the firing
   soldier's horizontal segment (a type-0x01 enemy, VAR_1=3; the shared
   enemy-bullet routines then animate and move it). */
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

/* roller_routine_00/01 (bank0:2860): start at Y=0x72, then roll down the
   corridor -- the sprite grows (0x99..0x9c) with depth, collision turns on once
   it's close (Y in [0xAC,0xBC)), and it's removed once it rolls past (Y>=0xBC).
   (Level-2 0x11 is the roller; level-1 0x11 is the boss door -- not yet ported,
   so this dispatch case is roller-only for now.) */
static void contra_rom_roller_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0x72u;
    contra_rom_advance_enemy_routine(core, x);
}

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
        contra_rom_clear_enemy(core, x); /* rolled past the player */
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

/* set_enemy_falling_arc_pos (bank0:area): advance the height + ground, place the
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

/* grenade_routine_02 (bank0:3029): explode at the bottom, set the blast's
   player-collision box (mortar_shot_routine_03), then start the explosion. */
static void contra_rom_grenade_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0xACu;
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x0Du;
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0xBEu) | 0x80u);
    contra_rom_begin_enemy_explosion(core, x);
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

static void contra_rom_grenade_launcher_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0x01u;
    contra_rom_set_var2_closest_player(core, x);
    contra_rom_init_indoor_enemy_pos_and_vel(core, x, 3u); /* grenade-kind velocity */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x20u);
}

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
            contra_rom_enemy_launch_grenade(
                core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x]);
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
   weapon DROP on death is deferred -- red soldiers die via the generic explosion. */
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

    /* mid-jump: walk, then follow the jump arc */
    if (contra_rom_apply_indoor_velocity(core, x))
    {
        return; /* walked off-screen and was removed */
    }
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
    if (screen >= 5u)
    {
        return; /* no generation script for this room */
    }
    data = contra_l2_enemy_gen_screen_tbl[screen];
    data_len = contra_l2_enemy_gen_screen_len[screen];

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

static void contra_rom_indoor_roller_gen_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x60u;
    contra_rom_advance_enemy_routine(core, x);
}

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

/* create_enemy_bullet_angle_a (bank7:6928-area): bullet whose quadrant is derived
   from its 24-direction angle (type in bits 7-5, angle in bits 4-0). */
static void contra_rom_create_enemy_bullet_angle_a(
    ContraCore *core, uint8_t type_angle, uint8_t speed, uint8_t px, uint8_t py)
{
    const uint8_t btype = (uint8_t)(type_angle >> 5u);
    const uint8_t angle = (uint8_t)(type_angle & 0x1Fu);
    uint8_t quadrant = 0u;

    if ((angle >= 0x07u) && (angle < 0x12u))
    {
        quadrant = 2u; /* left half */
    }
    if (angle >= 0x0Du)
    {
        quadrant = (uint8_t)(quadrant + 1u); /* upper half */
    }
    (void)contra_rom_create_enemy_bullet(core, btype, angle, quadrant, speed, px, py);
}

/* animate_wall_cannon (bank7:9319): show the current open/close frame super-tile
   and set the 6-frame step delay. */
static void contra_rom_animate_wall_cannon(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x06u;
    core->l2_supertile[x] = core->ram[CONTRA_RAM_ENEMY_FRAME + x];
}

static void contra_rom_wall_cannon_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x08u; /* real HP held in VAR_1 */
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x50u);
}

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
    contra_rom_animate_wall_cannon(core, x); /* draw open frame FRAME */
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

static void contra_rom_wall_cannon_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    contra_rom_animate_wall_cannon(core, x); /* draw closing frame */
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

static void contra_rom_wall_cannon_routine_04(ContraCore *core, uint8_t x)
{
    core->l2_supertile[x] = 0x05u; /* destroyed super-tile, then explode */
    contra_rom_begin_enemy_explosion(core, x);
}

static void contra_rom_wall_plating_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x80u);
}

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
    core->l2_supertile[x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 0x03u); /* deploy frame */
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x02u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_HP + x] = 0x0Au; /* now a destructible target */
    contra_rom_advance_enemy_routine(core, x); /* -> routine_02 (idle target) */
}

static void contra_rom_wall_plating_routine_03(ContraCore *core, uint8_t x)
{
    core->l2_supertile[x] = 0x05u; /* destroyed super-tile, then explode */
    core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] =
        (uint8_t)(core->ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] + 1u);
    contra_rom_begin_enemy_explosion(core, x);
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
static uint8_t contra_rom_aim_at_player(
    ContraCore *core, uint8_t sx, uint8_t sy, const uint8_t *tbl, uint8_t *quadrant)
{
    uint8_t *const ram = core->ram;
    const uint8_t p0 = ram[CONTRA_RAM_SPRITE_X_POS + 0u];
    const uint8_t p1 = ram[CONTRA_RAM_SPRITE_X_POS + 1u];
    const uint8_t d0 = (p0 >= sx) ? (uint8_t)(p0 - sx) : (uint8_t)(sx - p0);
    const uint8_t d1 = (p1 >= sx) ? (uint8_t)(p1 - sx) : (uint8_t)(sx - p1);
    uint8_t idx = ((p1 != 0u) && ((p0 == 0u) || (d1 < d0))) ? 1u : 0u;
    uint8_t tx;
    uint8_t ty;

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

static void contra_rom_boss_eye_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x40u);
}

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
static void contra_rom_boss_eye_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    int slot;

    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0u)
    {
        if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x01u)
        {
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x52u;
        }
        ram[CONTRA_RAM_ENEMY_HP + x] = 0x01u;
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x20u; /* flash red */
        contra_rom_set_enemy_routine_to_a(core, x, 0x03u); /* -> routine_02 (drift) */
        return;
    }
    /* boss_defeated_routine (bank7): mark defeated, wipe the other enemies, explode. */
    ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
    for (slot = 0x0F; slot >= 0; --slot)
    {
        if ((uint8_t)slot != x)
        {
            contra_rom_clear_enemy(core, (uint8_t)slot);
        }
    }
    contra_rom_begin_enemy_explosion(core, x);
}

static const uint8_t contra_eye_projectile_sprite_attr_tbl[4] = {0x00u, 0x40u, 0xC0u, 0x80u};

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

/* --- boss bomb turret (enemy type 0x10), bank0.asm --- */
/* super-tile per (recoil state VAR_1: 0 idle / 2 firing) and background variant
   (attr bit0: wall vs jungle), interleaved. */
static const uint8_t contra_boss_bomb_turret_supertile_tbl[6] = {
    0x29u, 0x26u, 0x2Au, 0x27u, 0x2Bu, 0x28u};
static const uint8_t contra_boss_bomb_turret_bomb_velocity_tbl[4] = {0x01u, 0x03u, 0x05u, 0x07u};

static void contra_rom_draw_boss_bomb_turret(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t idx = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    int draw_x = (int)ram[CONTRA_RAM_ENEMY_X_POS + x];

    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u)
    {
        idx = (uint8_t)(idx + 1u); /* jungle background variant */
        draw_x -= 8;               /* jungle super-tile sits 8px left of the enemy */
    }
    contra_render_level_1_nametable_update_supertile(
        core, draw_x, (int)ram[CONTRA_RAM_ENEMY_Y_POS + x],
        contra_boss_bomb_turret_supertile_tbl[(idx < 6u) ? idx : 0u]);
}

/* boss_bomb_turret_routine_00/01 (bank0): after a startup delay, alternate
   idle/recoil super-tiles and lob a bomb (a regular type-0 enemy bullet at a
   fixed up-left angle with a random speed) on each firing beat. */
static void contra_rom_boss_bomb_turret_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x20u;
    contra_rom_advance_enemy_routine(core, x);
}

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
    (void)contra_rom_create_enemy_bullet(
        core, 0u, 0x17u, 0x01u,
        contra_boss_bomb_turret_bomb_velocity_tbl[ram[CONTRA_RAM_RANDOM_NUM] & 0x03u],
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + 0xF8u),
        ram[CONTRA_RAM_ENEMY_Y_POS + x]);
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
    contra_render_level_1_nametable_update_supertile(
        core,
        (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        contra_red_turret_supertile_tbl[(idx < 11u) ? idx : 0u]);
    return true;
}

/* red_turret_routine_00..02 (bank0): aim left, wait for the player to approach,
   then emerge (super-tile animation) and become collidable. The active rotating
   aim + firing + retract (routine_03..05) are DEFERRED: the turret emerges,
   renders, and can be shot/destroyed (via the kill path -> explosion). */
static void contra_rom_red_turret_routine_00(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x06u; /* face left */
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_red_turret_routine_01(ContraCore *core, uint8_t x)
{
    if (!contra_rom_past_trigger_x(core, x, 0xF0u))
    {
        contra_rom_add_scroll_to_enemy_pos(core, x);
        return; /* player not close yet */
    }
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
    contra_rom_advance_enemy_routine(core, x);
}

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
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x28u;
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu); /* enable bullet collision */
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x10u;
    contra_rom_advance_enemy_routine(core, x);
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
        ty = (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
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
    contra_render_level_1_nametable_update_supertile(
        core, (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x], supertile);
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
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
    return contra_rom_past_trigger_x(core, x, 0x18u);
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
    if (!contra_rom_past_trigger_x(core, x, 0xF0u))
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
    /* draw the next opening super-tile (offset 3 + FRAME); set_delay drops the
       delay to 1 so the animation steps each frame. */
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
    contra_rom_clear_enemy(core, x); /* remove_enemy */
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
    contra_render_level_1_nametable_update_supertile(
        core, (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x], 0x16u);
    contra_rom_begin_enemy_explosion(core, x);
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
    if (contra_rom_past_trigger_x(core, x, 0x30u))
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
    contra_rom_clear_enemy(core, x);
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
    contra_render_level_1_nametable_update_supertile(
        core, (int)ram[CONTRA_RAM_ENEMY_X_POS + x], (int)ram[CONTRA_RAM_ENEMY_Y_POS + x],
        ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u) ? 0x17u : 0x16u);
    contra_rom_begin_enemy_explosion(core, x);
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

/* create_explosion_a / create_explosion_sequence (bank7:8278): spawn an explosion
   actor at (px,py) in a free slot. The ROM uses a type-0x02 enemy running its
   appended init_explosion routine; the native port models every explosion as the
   shared 0xFE actor (contra_rom_enemy_routine_explosion), so set that up here. */
static void contra_rom_create_explosion_at(ContraCore *core, uint8_t px, uint8_t py)
{
    const int slot = contra_rom_find_next_enemy_slot(core);
    uint8_t s;

    if (slot < 0)
    {
        return;
    }
    s = (uint8_t)slot;
    core->ram[CONTRA_RAM_ENEMY_TYPE + s] = 0xFEu;
    core->ram[CONTRA_RAM_ENEMY_ROUTINE + s] = 0x01u;
    core->ram[CONTRA_RAM_ENEMY_FRAME + s] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + s] = 0x0Au;
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + s] = 0x81u; /* bullets pass + no player-body collision */
    core->ram[CONTRA_RAM_ENEMY_SPRITES + s] = 0x38u;
    core->ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + s] = 0x00u;
    core->ram[CONTRA_RAM_ENEMY_Y_POS + s] = py;
    core->ram[CONTRA_RAM_ENEMY_X_POS + s] = px;
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

/* play_explosion_sound (bank0:642): pop an explosion and repurpose this slot into
   a weapon item carrying the source's weapon type (ATTRIBUTES & 0x07). Shared by
   the pill box (weapon_box_routine_04) and the flying capsule
   (flying_capsule_routine_02). Score/sound award is deferred. */
static void contra_rom_play_explosion_sound(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_create_explosion_at(core, ram[CONTRA_RAM_ENEMY_X_POS + x], ram[CONTRA_RAM_ENEMY_Y_POS + x]);
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x07u);
    contra_rom_clear_sprite_clear_enemy_pt_3(core, x);
    ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_TYPE + x] = 0x00u; /* now a weapon item */
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
    contra_render_level_1_nametable_update_supertile(
        core, (int)ram[CONTRA_RAM_ENEMY_X_POS + x], (int)ram[CONTRA_RAM_ENEMY_Y_POS + x],
        contra_weapon_box_destroyed_supertile[y & 0x0Fu]);
    contra_rom_play_explosion_sound(core, x);
}

/* flying_capsule_routine_02 (bank0:737): the weapon zeppelin was destroyed -- it
   has no background tile, so it drops the weapon item directly. */
static void contra_rom_flying_capsule_routine_02(ContraCore *core, uint8_t x)
{
    contra_rom_play_explosion_sound(core, x);
}

/* destroy_all_enemies (bank7:8096): set every live, damageable enemy to its
   destroyed routine -- here the shared 0xFE explosion actor -- skipping the pill
   box (0x02), flying capsule (0x03), and no-damage (HP 0xF0) enemies. */
static void contra_rom_destroy_all_enemies(ContraCore *core)
{
    int s;

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
        core->ram[CONTRA_RAM_ENEMY_ATTRIBUTES + ss] =
            (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ATTRIBUTES + ss] | 0x80u);
        contra_rom_begin_enemy_explosion(core, ss);
    }
}

/* pick_up_weapon_item (bank7:6860): the player touched a weapon item -- apply the
   weapon change for ATTRIBUTES & 0x07 (R adds rapid fire; M/F/S/L replace and drop
   rapid fire unless it's the same weapon; B grants the barrier timer; falcon wipes
   the screen), then remove the item. (Score/sound award is deferred, like the rest
   of the faithful path.) */
static void contra_rom_pick_up_weapon_item(ContraCore *core, uint8_t slot, uint8_t player)
{
    uint8_t *const ram = core->ram;
    const uint8_t attrs = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] & 0x07u);
    uint8_t item_type;
    uint8_t keep_mask;

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
        contra_rom_destroy_all_enemies(core); /* falcon */
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
static void contra_rom_bridge_destroy_supertile(
    ContraCore *core, uint8_t x, uint8_t draw_x_base, uint8_t tile)
{
    uint8_t *const ram = core->ram;
    const uint8_t cell_screen_x = (uint8_t)(draw_x_base - 12u);
    const uint8_t cell_screen_y = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] - 12u);
    const uint16_t world_x =
        (uint16_t)(((uint16_t)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                   ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + cell_screen_x);
    uint8_t i;

    contra_render_level_1_nametable_update_supertile(
        core, (int)draw_x_base, (int)ram[CONTRA_RAM_ENEMY_Y_POS + x], tile);

    for (i = 0u; i < core->l1_bridge_gap_count; ++i)
    {
        if ((core->l1_bridge_gap_world_x[i] == world_x) &&
            (core->l1_bridge_gap_screen_y[i] == cell_screen_y))
        {
            return; /* already recorded */
        }
    }
    if (core->l1_bridge_gap_count < 16u)
    {
        core->l1_bridge_gap_world_x[core->l1_bridge_gap_count] = world_x;
        core->l1_bridge_gap_screen_y[core->l1_bridge_gap_count] = cell_screen_y;
        core->l1_bridge_gap_count = (uint8_t)(core->l1_bridge_gap_count + 1u);
    }
}

/* exploding_bridge_routine_00 (bank0:2273): wait until a player is within 0x18
   pixels, then start the explosion sequence. */
static void contra_rom_exploding_bridge_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t ex = ram[CONTRA_RAM_ENEMY_X_POS + x];
    uint8_t d0 = (ram[CONTRA_RAM_SPRITE_X_POS + 0u] >= ex)
        ? (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + 0u] - ex)
        : (uint8_t)(ex - ram[CONTRA_RAM_SPRITE_X_POS + 0u]);
    uint8_t d1 = (ram[CONTRA_RAM_SPRITE_X_POS + 1u] >= ex)
        ? (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + 1u] - ex)
        : (uint8_t)(ex - ram[CONTRA_RAM_SPRITE_X_POS + 1u]);

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
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
    contra_rom_create_explosion_at(
        core,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] + contra_exploding_bridge_cloud_x_offset[var2 & 0x07u]),
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + contra_exploding_bridge_cloud_y_offset[var2 & 0x03u]));
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
        contra_rom_clear_enemy(core, x); /* all sections gone */
        return;
    }
    nx = (unsigned)ram[CONTRA_RAM_ENEMY_X_POS + x] + 0x20u;
    if (nx > 0xFFu)
    {
        contra_rom_clear_enemy(core, x);
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
    uint8_t *const ram = core->ram;
    const uint8_t max_frames = ((ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x08u) != 0u) ? 4u : 3u;

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
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= max_frames)
    {
        contra_rom_advance_enemy_routine(core, x); /* -> the slot's next routine */
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        explosion_type_00[(ram[CONTRA_RAM_ENEMY_FRAME + x] < 3u) ? ram[CONTRA_RAM_ENEMY_FRAME + x] : 0u];
}

/* dispatch one enemy slot to its type routine by (ENEMY_TYPE, ENEMY_ROUTINE).
   Only ported types act; others hold until their routine is ported. */
static void contra_rom_exe_enemy_type(ContraCore *core, uint8_t x)
{
    const uint8_t type = core->ram[CONTRA_RAM_ENEMY_TYPE + x];
    const uint8_t routine = core->ram[CONTRA_RAM_ENEMY_ROUTINE + x];

    switch (type)
    {
        case 0x10u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u)
            {
                switch (routine) /* level-2 boss eye */
                {
                    case 0x01u: contra_rom_boss_eye_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_eye_routine_01(core, x); break;
                    case 0x03u: contra_rom_boss_eye_routine_02(core, x); break;
                    case 0x04u: contra_rom_boss_eye_routine_03(core, x); break;
                    default: break; /* defeated/explosion handled in routine_03 */
                }
            }
            else
            {
                switch (routine) /* level-1 boss wall bomb turret */
                {
                    case 0x01u: contra_rom_boss_bomb_turret_routine_00(core, x); break;
                    case 0x02u: contra_rom_boss_bomb_turret_routine_01(core, x); break;
                    default: break; /* explosion handled via the kill path */
                }
            }
            break;
        case 0x1Bu: /* level-2 boss eye sphere projectile */
            switch (routine)
            {
                case 0x01u: contra_rom_eye_projectile_routine_00(core, x); break;
                case 0x02u: contra_rom_eye_projectile_routine_01(core, x); break;
                default: break; /* explosion via the 0xFE actor */
            }
            break;
        case 0x13u: /* level-2 wall turret */
            switch (routine)
            {
                case 0x01u: contra_rom_wall_turret_routine_00(core, x); break;
                case 0x02u: contra_rom_wall_turret_routine_01(core, x); break;
                case 0x03u: contra_rom_wall_turret_routine_02(core, x); break;
                case 0x04u: contra_rom_wall_turret_routine_03(core, x); break;
                case 0x05u: contra_rom_wall_turret_routine_04(core, x); break;
                default: break; /* explosion via the 0xFE actor */
            }
            break;
        case 0x14u: /* level-2 wall core */
            switch (routine)
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
            break;
        case 0x15u: /* indoor running soldier */
            switch (routine)
            {
                case 0x01u: contra_rom_indoor_soldier_routine_00(core, x); break;
                case 0x02u: contra_rom_indoor_soldier_routine_01(core, x); break;
                default: break; /* hit/explosion via the 0xFE actor */
            }
            break;
        case 0x16u: /* indoor jumping soldier */
            switch (routine)
            {
                case 0x01u: contra_rom_jumping_soldier_routine_00(core, x); break;
                case 0x02u: contra_rom_jumping_soldier_routine_01(core, x); break;
                default: break; /* hit/explosion via the 0xFE actor */
            }
            break;
        case 0x18u: /* indoor group-of-4 soldier */
            switch (routine)
            {
                case 0x01u: contra_rom_four_soldiers_routine_00(core, x); break;
                case 0x02u: contra_rom_four_soldiers_routine_01(core, x); break;
                case 0x03u: contra_rom_four_soldiers_routine_02(core, x); break;
                default: break; /* hit/explosion via the 0xFE actor */
            }
            break;
        case 0x11u: /* level-2 indoor roller (level-1 0x11 = boss door, not ported) */
            switch (routine)
            {
                case 0x01u: contra_rom_roller_routine_00(core, x); break;
                case 0x02u: contra_rom_roller_routine_01(core, x); break;
                default: break; /* hit/explosion via the 0xFE actor */
            }
            break;
        case 0x1Au: /* indoor roller generator */
            switch (routine)
            {
                case 0x01u: contra_rom_indoor_roller_gen_routine_00(core, x); break;
                case 0x02u: contra_rom_indoor_roller_gen_routine_01(core, x); break;
                default: break; /* removed via clear once the rounds are done */
            }
            break;
        case 0x12u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u)
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
            else
            {
                switch (routine) /* level-2 indoor grenade */
                {
                    case 0x01u: contra_rom_grenade_routine_00(core, x); break;
                    case 0x02u: contra_rom_grenade_routine_01(core, x); break;
                    case 0x03u: contra_rom_grenade_routine_02(core, x); break;
                    default: break; /* explosion via the 0xFE actor */
                }
            }
            break;
        case 0x17u: /* indoor grenade launcher (seeking guy) */
            switch (routine)
            {
                case 0x01u: contra_rom_grenade_launcher_routine_00(core, x); break;
                case 0x02u: contra_rom_grenade_launcher_routine_01(core, x); break;
                default: break; /* hit/explosion via the 0xFE actor (flag cleared there) */
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
                default: break; /* explosion via the 0xFE actor */
            }
            break;
        case 0x0Au: /* boss-room wall plating */
            switch (routine)
            {
                case 0x01u: contra_rom_wall_plating_routine_00(core, x); break;
                case 0x02u: contra_rom_wall_plating_routine_01(core, x); break;
                case 0x03u: break; /* routine_02: idle, just a target */
                case 0x04u: contra_rom_wall_plating_routine_03(core, x); break;
                default: break; /* explosion via the 0xFE actor */
            }
            break;
        case 0x19u: /* indoor soldier generator */
            switch (routine)
            {
                case 0x01u: contra_rom_indoor_soldier_gen_routine_00(core, x); break;
                case 0x02u: contra_rom_indoor_soldier_gen_routine_01(core, x); break;
                default: break; /* removed via clear once the cycles are done */
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
                default: break; /* explosion via the 0xFE actor */
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
                default: break; /* explosion via the 0xFE actor */
            }
            break;
        case 0x01u: /* enemy bullet */
            switch (routine)
            {
                case 0x01u: contra_rom_enemy_bullet_routine_00(core, x); break;
                case 0x02u: contra_rom_enemy_bullet_routine_01(core, x); break;
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
                default: break; /* fire/hit routines not yet ported */
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
                default: break; /* explosion via the 0xFE actor */
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
        case 0x06u: /* sniper */
            switch (routine)
            {
                case 0x01u: contra_rom_sniper_routine_00(core, x); break;
                case 0x02u: contra_rom_sniper_routine_01(core, x); break;
                case 0x03u: contra_rom_sniper_routine_02(core, x); break;
                default: break; /* post-attack/explosion routines not yet ported */
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

/* bullet_enemy_collision_test (bank7.asm:6928) + set_enemy_collision_box +
   bullet_collision_logic, outdoor path: test each live player bullet against the
   enemy's bullet hitbox; on a hit subtract HP (HP >= 0xF0 is invulnerable, e.g.
   the open pill box) and remove the enemy when HP reaches 0.
   DEFERRED: score award, explosion animation (enemy is removed on death for
   now), and laser pass-through (the bullet is consumed on any hit). */
static void contra_rom_bullet_enemy_collision_test(ContraCore *core, uint8_t slot)
{
    uint8_t *const ram = core->ram;
    const uint8_t code = (uint8_t)(ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] & 0x0Fu);
    const uint8_t *box;
    uint8_t box_y;
    uint8_t box_x;
    int bullet;

    if (code >= 15u)
    {
        return; /* collision code 0x0F (fire beams / spiked walls) not used in level 1 */
    }
    box = contra_collision_box_codes_04[code];
    box_y = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + slot] + box[0]);
    box_x = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + slot] + box[1]);

    for (bullet = 0x0F; bullet >= 0; --bullet)
    {
        const unsigned b = (unsigned)bullet;
        uint8_t diff;
        uint8_t hp;

        if ((ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + b] == 0u) ||
            (ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + b] != 0x01u))
        {
            continue;
        }
        /* outdoor box test (the ROM enters with carry clear -> the -1 borrow) */
        diff = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + b] - box_y - 1u);
        if (diff >= box[2])
        {
            continue;
        }
        diff = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_X_POS + b] - box_x - 1u);
        if (diff >= box[3])
        {
            continue;
        }

        /* hit */
        hp = ram[CONTRA_RAM_ENEMY_HP + slot];
        ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + b] = 0u; /* consume bullet */
        if ((hp == 0u) || (hp >= 0xF0u))
        {
            continue; /* already dead, or invulnerable this frame */
        }
        hp = (uint8_t)(hp - 1u);
        ram[CONTRA_RAM_ENEMY_HP + slot] = hp;
        if (hp == 0u)
        {
            /* set_destroyed_enemy_routine (bank7:7977): a killed enemy routes to
               its type's destroyed routine (enemy_destroyed_routine tables) rather
               than always exploding. The cases we cover (RAM routine value =
               nibble+1 from those tables): wall turret/core 0x13/0x14 -> 5
               (their routine_04 plating/destruction); boss-room wall cannon 0x08
               -> 5 (routine_04 destroyed tile); wall plating 0x0A -> 4 (routine_03
               destroyed, bumps the plating count); level-2 boss eye 0x10 -> 4
               (routine_03 dec real HP). Everything else takes the explosion actor. */
            const uint8_t dead_type = ram[CONTRA_RAM_ENEMY_TYPE + slot];
            const bool is_l2 = (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u);
            uint8_t dest_routine = 0u;

            if ((dead_type == 0x13u) || (dead_type == 0x14u) || (dead_type == 0x08u))
            {
                dest_routine = 0x05u;
            }
            else if ((dead_type == 0x0Au) || (is_l2 && (dead_type == 0x10u)))
            {
                dest_routine = 0x04u;
            }
            else if (dead_type == 0x04u)
            {
                dest_routine = 0x07u; /* rotating gun -> routine_06 (restore rock, explode) */
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
            if (dest_routine != 0u)
            {
                contra_rom_set_enemy_routine_to_a(core, slot, dest_routine);
                return;
            }
            contra_rom_begin_enemy_explosion(core, slot); /* TODO: award score */
            return;
        }
    }
}

/* Player collision boxes vs an enemy, by player state: jumping (box 01) and
   standing (box 02), bank7.asm:7240/7259. Water (00) and crouching (03) fall
   back to standing for now. */
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

/* check_players_collision (bank7.asm:6671), outdoor focus: if a normal-state
   player's body overlaps this enemy's collision box, kill the player (barrier
   invincibility destroys the enemy instead; new-life invincibility passes
   through). DEFERRED: water/crouch boxes (use standing), landing on rideable
   enemies, and weapon-item pickup. */
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
        tbl = (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + p] != 0u)
            ? contra_collision_box_codes_01 : contra_collision_box_codes_02;
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
    }
}

/* exe_all_enemy_routine (bank7.asm:7315): run every active enemy's routine, then
   test player-body and player-bullet collision against it, gated like the ROM
   dispatcher (ENEMY_SPRITES set; body collision when STATE_WIDTH bit 0 clear;
   bullet collision when bit 7 clear). */
static void contra_rom_exe_all_enemy_routine(ContraCore *core)
{
    int slot;

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
        if ((core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + sx] & 0x01u) == 0u)
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

/* Faithful level-2 (indoor base) enemy system, gated independently of level 1.
   Default 1: the faithful real-RAM level-2 enemies are the active path; build with
   -DCONTRA_USE_ROM_ENEMY_SYSTEM_L2=0 for the old invented layer. */
#ifndef CONTRA_USE_ROM_ENEMY_SYSTEM_L2
#define CONTRA_USE_ROM_ENEMY_SYSTEM_L2 1
#endif

/* level_2_enemy_screen_* (bank2.asm:2361): each indoor room is a "cores to
   destroy" count, then enemy triples {pos, type(+pos-adjust flags), attrs},
   0xFF-terminated. pos = (hi nibble = Y, lo nibble = X) scaled x16; type byte
   bit7 -> +7 Y, bit6 -> +7 X. Types: 0x13 wall turret, 0x14 wall core,
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

/* load_enemy_indoor_level (bank2.asm): on room entry, clear all slots and load
   the whole room's enemy set at once (indoor levels are room-based, not
   scroll-triggered), setting WALL_CORE_REMAINING from the first byte. */
static void contra_rom_load_indoor_enemy_data(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    const uint8_t *data;
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
    ram[CONTRA_RAM_INDOOR_RED_SOLDIER_CREATED] = 0u;
    ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0u;
    for (slot = 0x0F; slot >= 0; --slot)
    {
        contra_rom_clear_enemy(core, (uint8_t)slot);
    }
    if (screen >= 6u)
    {
        return;
    }
    data = contra_l2_enemy_screen_tbl[screen];
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
        ram[CONTRA_RAM_ENEMY_Y_POS + sx] =
            (uint8_t)((pos & 0xF0u) + (((type_byte & 0x80u) != 0u) ? 0x07u : 0x00u));
        ram[CONTRA_RAM_ENEMY_X_POS + sx] =
            (uint8_t)((uint8_t)(pos << 4u) + (((type_byte & 0x40u) != 0u) ? 0x07u : 0x00u));
        ram[CONTRA_RAM_ENEMY_ATTRIBUTES + sx] = data[y + 2u];
        y += 3u;
    }
}

static void contra_run_level_enemy_logic(ContraCore *core)
{
    contra_load_bank_3_handle_scroll(core);
    if (CONTRA_USE_ROM_ENEMY_SYSTEM_L2 && (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u))
    {
        /* faithful level-2 indoor enemy system (work in progress) */
        contra_rom_exe_all_enemy_routine(core);
        contra_rom_load_indoor_enemy_data(core);
    }
    if (CONTRA_USE_ROM_ENEMY_SYSTEM && (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0u))
    {
        /* faithful real-RAM enemy system (level 1, work in progress) */
        contra_rom_exe_all_enemy_routine(core);
        contra_rom_load_screen_enemy_data(core);
    }
    else
    {
        contra_load_bank_0_exe_all_enemy_routine(core);
        contra_load_bank_2_load_screen_enemy_data(core);
        contra_load_bank_2_exe_soldier_generation(core);
    }

    contra_update_native_enemy_projectiles(core);
    contra_check_native_player_collisions(core);
    contra_load_palette_indexes(core);
    contra_load_bank_2_alternate_tile_loading(core);
}

static void contra_init_game_over(ContraCore *core)
{
    core->ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] = 0x60u;
    contra_set_a_as_current_level_routine(core, 0x0Au);
}

static void contra_set_to_level_routine_05(ContraCore *core)
{
    core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x00u;
    contra_set_a_as_current_level_routine(core, 0x05u);
}

static bool contra_check_game_over_run_enemy_logic(ContraCore *core)
{
    contra_set_frame_scroll_draw_player_bullets(core);
    if ((uint8_t)(core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS] & core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS]) != 0u)
    {
        contra_init_game_over(core);
        return true;
    }

    contra_run_level_enemy_logic(core);
    return false;
}

static void contra_end_level_set_delay_advance(ContraCore *core, uint8_t delay)
{
    core->ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] = delay;
    core->ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] =
        (uint8_t)(core->ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_make_off_screen_player_invisible(ContraCore *core, uint8_t player_index)
{
    const uint8_t sprite_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    const uint8_t sprite_x = core->ram[CONTRA_RAM_SPRITE_X_POS + player_index];

    if ((sprite_y >= 0x08u) && (sprite_x < 0xF8u) && (sprite_x >= 0x04u))
    {
        return;
    }

    core->ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] = 0xFFu;
    core->ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x00u;
    core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = 0x00u;
}

static void contra_run_level_1_end_level_player_routine(ContraCore *core, uint8_t player_index, uint8_t state_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + player_index] =
        (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >= 0x98u) ? 0x80u : 0x00u;

    switch (state_index)
    {
        case 0x00u:
            ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = CONTRA_BUTTON_RIGHT;
            if (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >= 0x90u)
            {
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            }
            break;

        case 0x01u:
            if (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u)
            {
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            }
            else
            {
                ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = (uint8_t)(CONTRA_BUTTON_A | CONTRA_BUTTON_RIGHT);
                ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_index] = (uint8_t)(CONTRA_BUTTON_A | CONTRA_BUTTON_RIGHT);
            }
            break;

        default:
            ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = CONTRA_BUTTON_RIGHT;
            break;
    }
}

static void contra_run_end_level_sequence(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_CONTROLLER_STATE + 0u] = 0x00u;
    ram[CONTRA_RAM_CONTROLLER_STATE + 1u] = 0x00u;
    ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + 0u] = 0x00u;
    ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + 1u] = 0x00u;

    switch (ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX])
    {
        case 0x00u:
        {
            uint8_t merged_jump_status = 0x00u;
            int player_index;

            for (player_index = 1; player_index >= 0; --player_index)
            {
                uint8_t state = 0x00u;

                if (ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] == 0u)
                {
                    merged_jump_status |= ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index];
                    state = 0x01u;
                }

                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] = state;
            }

            if (merged_jump_status != 0u)
            {
                return;
            }

            ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x81u;
            ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] = 0xF0u;
            contra_end_level_set_delay_advance(core, 0x20u);
            break;
        }

        case 0x01u:
            if (ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] != 0u)
            {
                ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] - 1u);
                return;
            }

            {
                int player_index;

                for (player_index = 1; player_index >= 0; --player_index)
                {
                    const uint8_t routine_state = ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index];

                    if (routine_state == 0u)
                    {
                        continue;
                    }

                    if (ram[CONTRA_RAM_CURRENT_LEVEL] == 0u)
                    {
                        contra_run_level_1_end_level_player_routine(core, (uint8_t)player_index, (uint8_t)(routine_state - 1u));
                    }

                    contra_make_off_screen_player_invisible(core, (uint8_t)player_index);
                }
            }

            if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) != 0u)
            {
                ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] - 1u);
                if (ram[CONTRA_RAM_LEVEL_END_SQ_1_TIMER] == 0u)
                {
                    contra_end_level_set_delay_advance(
                        core,
                        contra_level_end_level_delay_timer_tbl[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u]
                    );
                    return;
                }
            }

            if ((uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + 0u] |
                          ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + 1u]) == 0u)
            {
                contra_end_level_set_delay_advance(
                    core,
                    contra_level_end_level_delay_timer_tbl[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u]
                );
            }
            break;

        default:
            ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x02u;
            if (ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] != 0u)
            {
                ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] - 1u);
                if (ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] != 0u)
                {
                    return;
                }
            }

            contra_set_graphics_zero_mode(core);
            contra_set_a_as_current_level_routine(core, 0x05u);
            break;
    }
}

static void contra_show_game_over_screen(ContraCore *core)
{
    if (core->ram[CONTRA_RAM_DEMO_MODE] != 0u)
    {
        core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] = 0x01u;
        return;
    }

    contra_zero_out_nametables(core);
    contra_load_level_intro_screen_graphics(core);
    contra_load_bank_6_write_text_palette_to_mem(core, 0x06u);
    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Du);
    contra_play_sound(core, 0x4Eu);

    core->ram[CONTRA_RAM_NUM_CONTINUES] = (uint8_t)(core->ram[CONTRA_RAM_NUM_CONTINUES] - 1u);
    if ((core->ram[CONTRA_RAM_NUM_CONTINUES] & 0x80u) != 0u)
    {
        contra_set_a_as_current_level_routine(core, 0x07u);
        return;
    }

    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Eu);
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_check_for_pause(ContraCore *core)
{
    uint8_t input = core->ram[CONTRA_RAM_CONTROLLER_STATE_DIFF];

    if ((uint8_t)(core->ram[CONTRA_RAM_DEMO_MODE] | core->ram[0x26u] | core->ram[CONTRA_RAM_PPU_READY]) != 0u)
    {
        return;
    }

    if (core->ram[CONTRA_RAM_PAUSE_STATE] == 0u)
    {
        if ((input & CONTRA_BUTTON_START) == 0u)
        {
            return;
        }

        core->ram[CONTRA_RAM_PAUSE_STATE] = 0x01u;
        contra_play_sound(core, 0x54u);
        return;
    }

    contra_draw_player_bullet_sprites(core);
    contra_load_bank_2_set_players_paused_sprite_attr(core);

    if ((input & CONTRA_BUTTON_START) == 0u)
    {
        return;
    }

    core->ram[CONTRA_RAM_PAUSE_STATE] = 0x00u;
}

static void contra_level_routine_04(ContraCore *core)
{
    contra_check_for_pause(core);
    if (core->ram[CONTRA_RAM_PAUSE_STATE] != 0u)
    {
        return;
    }

    if (core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0u)
    {
        contra_set_a_as_current_level_routine(core, 0x08u);
        return;
    }

    (void)contra_check_game_over_run_enemy_logic(core);
}

static void contra_level_routine_05(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t p1_weapon = ram[CONTRA_RAM_P1_CURRENT_WEAPON];
    const uint8_t p2_weapon = ram[CONTRA_RAM_P2_CURRENT_WEAPON];

    ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0x00u;
    ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x00u;
    contra_clear_level_runtime_memory(core);

    if (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0u)
    {
        contra_show_game_over_screen(core);
        return;
    }

    ram[CONTRA_RAM_P1_CURRENT_WEAPON] = p1_weapon;
    ram[CONTRA_RAM_P2_CURRENT_WEAPON] = p2_weapon;
    ram[CONTRA_RAM_CURRENT_LEVEL] = (uint8_t)(ram[CONTRA_RAM_CURRENT_LEVEL] + 1u);
    if (ram[CONTRA_RAM_CURRENT_LEVEL] >= 0x08u)
    {
        contra_increment_game_routine(core);
        ram[CONTRA_RAM_GAME_COMPLETION_COUNT] = (uint8_t)(ram[CONTRA_RAM_GAME_COMPLETION_COUNT] + 1u);
        ram[CONTRA_RAM_CURRENT_LEVEL] = 0x09u;
        return;
    }

    contra_load_level_intro_screen_graphics(core);
    ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0x00u;
}

static void contra_level_routine_06(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t input_diff = ram[CONTRA_RAM_CONTROLLER_STATE_DIFF];

    if ((input_diff & CONTRA_BUTTON_START) != 0u)
    {
        contra_init_apu_channels(core);
        if (ram[CONTRA_RAM_CONT_END_SELECTION] != 0u)
        {
            contra_set_game_routine_index(core, 0x00u);
            return;
        }

        contra_reset_players_score_and_lives(core);
        ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
        ram[CONTRA_RAM_SPRITE_X_POS + 0u] = 0x00u;
        ram[CONTRA_RAM_SPRITE_Y_POS + 0u] = 0x00u;
        ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u] = 0x00u;
        return;
    }

    if ((input_diff & CONTRA_BUTTON_SELECT) != 0u)
    {
        ram[CONTRA_RAM_CONT_END_SELECTION] ^= 0x01u;
    }

    ram[CONTRA_RAM_SPRITE_X_POS + 0u] = 0x52u;
    ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u] = 0xAAu;
    ram[CONTRA_RAM_SPRITE_Y_POS + 0u] =
        contra_player_select_cursor_pos[ram[CONTRA_RAM_CONT_END_SELECTION] & 0x01u];
    contra_draw_the_scores(core);
}

static void contra_level_routine_07(ContraCore *core)
{
    if ((core->ram[CONTRA_RAM_CONTROLLER_STATE_DIFF] & CONTRA_BUTTON_START) != 0u)
    {
        contra_set_game_routine_index(core, 0x00u);
        return;
    }

    contra_draw_the_scores(core);
}

static void contra_level_routine_08(ContraCore *core)
{
    uint8_t alive_players = 0x00u;
    int player_index;

    if (contra_check_game_over_run_enemy_logic(core))
    {
        return;
    }

    if (core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] != 0u)
    {
        if ((uint8_t)(core->ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] + 1u) == 0u)
        {
            return;
        }

        if (!contra_decrement_delay_timer_elapsed(core))
        {
            return;
        }
    }

    for (player_index = 1; player_index >= 0; --player_index)
    {
        if ((core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] == 0u) &&
            (core->ram[CONTRA_RAM_PLAYER_STATE + player_index] == 0x01u))
        {
            ++alive_players;
        }
    }

    core->ram[CONTRA_RAM_LEVEL_END_PLAYERS_ALIVE] = alive_players;
    if (alive_players == 0u)
    {
        return;
    }

    contra_play_sound(core, 0x46u);
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = (uint8_t)(core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] + 1u);
}

static void contra_level_routine_09(ContraCore *core)
{
    contra_run_end_level_sequence(core);
    contra_set_frame_scroll_draw_player_bullets(core);
    contra_load_bank_3_handle_scroll(core);
    contra_load_bank_0_exe_all_enemy_routine(core);
    contra_load_palette_indexes(core);
}

static void contra_level_routine_0a(ContraCore *core)
{
    contra_run_level_enemy_logic(core);
    core->ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] = (uint8_t)(core->ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] - 1u);
    if (core->ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] != 0u)
    {
        return;
    }

    contra_set_to_level_routine_05(core);
    contra_level_routine_05(core);
}

static void contra_run_level_routine(ContraCore *core)
{
    switch (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX])
    {
        case 0x00u:
            contra_level_routine_00(core);
            break;
        case 0x01u:
            contra_level_routine_01(core);
            break;
        case 0x02u:
            contra_level_routine_02(core);
            break;
        case 0x03u:
            contra_level_routine_03(core);
            break;
        case 0x04u:
            contra_level_routine_04(core);
            break;
        case 0x05u:
            contra_level_routine_05(core);
            break;
        case 0x06u:
            contra_level_routine_06(core);
            break;
        case 0x07u:
            contra_level_routine_07(core);
            break;
        case 0x08u:
            contra_level_routine_08(core);
            break;
        case 0x09u:
            contra_level_routine_09(core);
            break;
        case 0x0Au:
            contra_level_routine_0a(core);
            break;
        default:
            break;
    }
}

static void contra_game_routine_05(ContraCore *core)
{
    contra_run_level_routine(core);
}

static void contra_stop_demo_load_player_select_ui(ContraCore *core)
{
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
    contra_zero_out_nametables(core);
    contra_load_intro_graphics(core);
    contra_load_intro_palette2_play_intro_sound(core);
    contra_set_game_routine_index(core, 0x01u);
}

static void contra_player_mode_change(ContraCore *core)
{
    uint8_t player_mode = (uint8_t)(core->ram[CONTRA_RAM_PLAYER_MODE] + 1u);

    if ((uint8_t)(0x02u - player_mode) == 0u)
    {
        player_mode = 0x00u;
    }

    core->ram[CONTRA_RAM_PLAYER_MODE] = player_mode;
}

static void contra_dec_theme_delay_check_user_input(ContraCore *core)
{
    uint8_t input;

    contra_dec_intro_theme_delay(core);

    input = (uint8_t)(core->ram[CONTRA_RAM_CONTROLLER_STATE_DIFF] & 0x30u);
    if (input == 0u)
    {
        return;
    }

    contra_reset_delay_timer(core);

    if (core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] != 0x01u)
    {
        contra_stop_demo_load_player_select_ui(core);
        return;
    }

    if (core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] != 0u)
    {
        contra_load_intro_palette2_play_intro_sound(core);
        return;
    }

    if ((input & 0x20u) != 0u)
    {
        contra_player_mode_change(core);
        return;
    }

    contra_set_game_routine_index(core, 0x03u);
}

static void contra_exe_game_routine(ContraCore *core)
{
    uint8_t routine;

    /* Faithful (approximate) port of the ROM's RANDOM_NUM generator. Between
       NMIs the ROM spins in a busy loop (bank7.asm:284) repeatedly doing
       RANDOM_NUM += FRAME_COUNTER. The iteration count is CPU-timing dependent
       and cannot be reproduced exactly without cycle-accurate emulation, so we
       model a single iteration per frame using the pre-increment frame counter
       (the value the loop was adding during the gap before this NMI). This
       restores RNG variation — the port previously froze RANDOM_NUM at 0 — but
       does not reproduce the ROM's exact RNG sequence. */
    core->ram[CONTRA_RAM_RANDOM_NUM] =
        (uint8_t)(core->ram[CONTRA_RAM_RANDOM_NUM] + core->ram[CONTRA_RAM_FRAME_COUNTER]);

    core->ram[CONTRA_RAM_FRAME_COUNTER] = (uint8_t)(core->ram[CONTRA_RAM_FRAME_COUNTER] + 1u);

    routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    if ((routine != 0u) && (routine < 0x03u))
    {
        contra_dec_theme_delay_check_user_input(core);
        routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    }

    switch (routine)
    {
        case 0x00u:
            contra_game_routine_00(core);
            break;
        case 0x01u:
            contra_game_routine_01(core);
            break;
        case 0x02u:
            contra_game_routine_02(core);
            break;
        case 0x03u:
            contra_game_routine_03(core);
            break;
        case 0x04u:
            contra_game_routine_04(core);
            break;
        case 0x05u:
            contra_game_routine_05(core);
            break;
        default:
            break;
    }
}

static uint8_t contra_render_native_sprite_code(
    ContraCore *core,
    uint8_t sprite_code,
    uint8_t base_attr,
    uint8_t base_x,
    uint8_t base_y,
    uint8_t sprite_order
)
{
    uint16_t sprite_addr;

    if ((!contra_load_rom_image()) || (sprite_code == 0u))
    {
        return sprite_order;
    }

    sprite_addr = contra_read_sprite_ptr(sprite_code);
    if (sprite_addr == 0u)
    {
        return sprite_order;
    }

    if (contra_rom_read_u8(1u, sprite_addr) == 0xFEu)
    {
        const uint8_t tile = contra_rom_read_u8(1u, (uint16_t)(sprite_addr + 1u));
        const uint8_t attr = (uint8_t)(contra_rom_read_u8(1u, (uint16_t)(sprite_addr + 2u)) | base_attr);
        const uint8_t sprite_y = (uint8_t)(base_y - 0x08u);
        const uint8_t sprite_x = (uint8_t)(base_x - 0x04u);

        contra_draw_sprite_pair(core, sprite_x, sprite_y, tile, attr, sprite_order);
        return (uint8_t)(sprite_order + 1u);
    }

    {
        uint8_t sprite_tile_count = contra_rom_read_u8(1u, sprite_addr);
        uint16_t read_addr = (uint16_t)(sprite_addr + 1u);
        uint8_t sprite_effect = (uint8_t)(base_attr & 0xC8u);
        const uint8_t attr_mask = (base_attr & 0x04u) != 0u ? 0xFCu : 0xFFu;
        const uint8_t attr_base = (uint8_t)((base_attr & 0x04u) != 0u ? (base_attr & 0x23u) : (base_attr & 0x20u));

        while (sprite_tile_count != 0u)
        {
            uint8_t relative_y = contra_rom_read_u8(1u, read_addr++);

            if (relative_y == 0x80u)
            {
                sprite_effect &= 0xF7u;
                read_addr = contra_rom_read_u16(1u, read_addr);
                continue;
            }

            {
                const uint8_t tile = contra_rom_read_u8(1u, read_addr++);
                const uint8_t tile_attr = contra_rom_read_u8(1u, read_addr++);
                const uint8_t relative_x = contra_rom_read_u8(1u, read_addr++);
                const uint8_t adjusted_relative_y = (uint8_t)(relative_y + ((sprite_effect & 0x08u) != 0u ? 1u : 0u));
                const uint8_t sprite_y = (sprite_effect & 0x80u) != 0u
                    ? (uint8_t)(base_y + (uint8_t)(0xF0u - adjusted_relative_y))
                    : (uint8_t)(base_y + adjusted_relative_y);
                const uint8_t sprite_x = (sprite_effect & 0x40u) != 0u
                    ? (uint8_t)(base_x + (uint8_t)(0xF8u - relative_x))
                    : (uint8_t)(base_x + relative_x);
                const uint8_t attr = (uint8_t)(((tile_attr & attr_mask) | attr_base) ^ sprite_effect);

                contra_draw_sprite_pair(core, sprite_x, sprite_y, tile, attr, sprite_order);
                sprite_order = (uint8_t)(sprite_order + 1u);
                --sprite_tile_count;
            }
        }
    }

    return sprite_order;
}

static void contra_render_overlay_supertile(
    ContraCore *core,
    uint16_t supertile_data_addr,
    uint16_t palette_data_addr,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    const uint16_t supertile_addr = (uint16_t)(
        supertile_data_addr + ((uint16_t)supertile_index * 16u)
    );
    const uint8_t supertile_palette = contra_rom_read_u8(
        3u,
        (uint16_t)(palette_data_addr + supertile_index)
    );
    unsigned tile_y;

    for (tile_y = 0u; tile_y < 4u; ++tile_y)
    {
        unsigned tile_x;

        for (tile_x = 0u; tile_x < 4u; ++tile_x)
        {
            const uint8_t pattern_index = contra_rom_read_u8(
                3u,
                (uint16_t)(supertile_addr + ((uint16_t)tile_y * 4u) + tile_x)
            );
            const uint8_t palette_shift = (uint8_t)(((tile_y & 0x02u) << 1u) | (tile_x & 0x02u));
            const uint8_t palette_slot = (uint8_t)((supertile_palette >> palette_shift) & 0x03u);

            contra_draw_background_tile(
                core,
                dest_x + (int)(tile_x * 8u),
                dest_y + (int)(tile_y * 8u),
                pattern_index,
                palette_slot
            );
        }
    }
}

static void contra_calculate_update_supertile_ppu_addr(
    const ContraCore *core,
    int dest_x,
    int dest_y,
    uint16_t *tile_ppu_addr,
    uint16_t *attr_ppu_addr
)
{
    const uint8_t *const ram = core->ram;
    uint8_t rounded_y;
    uint8_t nt_low;
    uint8_t nt_high_bits = 0u;
    uint8_t attr_low_base;
    uint8_t scrolled_x;
    uint8_t nametable_index;
    uint8_t coarse_x;
    uint8_t attr_column_offset;
    unsigned carry;

    {
        const unsigned vertical_sum = (unsigned)(uint8_t)dest_y + (unsigned)ram[CONTRA_RAM_VERTICAL_SCROLL];
        rounded_y = (uint8_t)vertical_sum;
        if ((vertical_sum > 0xFFu) || (rounded_y >= 0xF0u))
        {
            rounded_y = (uint8_t)(rounded_y + 0x10u);
        }
        rounded_y &= 0xF8u;
    }

    nt_low = rounded_y;
    attr_low_base = (uint8_t)((rounded_y >> 2u) & 0x38u);
    carry = (unsigned)(nt_low >> 7u);
    nt_low = (uint8_t)(nt_low << 1u);
    nt_high_bits = (uint8_t)((nt_high_bits << 1u) | (uint8_t)carry);
    carry = (unsigned)(nt_low >> 7u);
    nt_low = (uint8_t)(nt_low << 1u);
    nt_high_bits = (uint8_t)((nt_high_bits << 1u) | (uint8_t)carry);

    {
        const unsigned horizontal_sum = (unsigned)(uint8_t)dest_x + (unsigned)ram[CONTRA_RAM_HORIZONTAL_SCROLL];
        scrolled_x = (uint8_t)horizontal_sum;
        nametable_index = (uint8_t)(ram[CONTRA_RAM_PPUCTRL_SETTINGS] & 0x01u);
        if (horizontal_sum > 0xFFu)
        {
            nametable_index ^= 0x01u;
        }
    }

    coarse_x = (uint8_t)((scrolled_x & 0xF8u) >> 3u);
    attr_column_offset = (uint8_t)((coarse_x >> 2u) | attr_low_base);

    *tile_ppu_addr = (uint16_t)(
        ((uint16_t)((nametable_index == 0u ? 0x20u : 0x24u) | nt_high_bits) << 8u) |
        (uint16_t)(nt_low | coarse_x)
    );
    *attr_ppu_addr = (uint16_t)(
        ((uint16_t)((nametable_index == 0u ? 0x23u : 0x27u) | 0x03u) << 8u) |
        (uint16_t)(0xC0u | attr_column_offset)
    );
}

static void contra_write_overlay_supertile_to_ppu_addr(
    ContraCore *core,
    uint16_t supertile_data_addr,
    uint16_t palette_data_addr,
    uint16_t tile_ppu_addr,
    uint16_t attr_ppu_addr,
    uint8_t supertile_index
)
{
    const bool update_palette = (supertile_index & 0x80u) == 0u;
    const uint8_t stripped_index = (uint8_t)(supertile_index & 0x7Fu);
    const uint16_t supertile_addr = (uint16_t)(
        supertile_data_addr + ((uint16_t)stripped_index * 16u)
    );
    unsigned tile_y;

    if (!contra_load_rom_image())
    {
        return;
    }

    if (update_palette)
    {
        contra_write_ppu_byte(
            core,
            attr_ppu_addr,
            contra_rom_read_u8(3u, (uint16_t)(palette_data_addr + stripped_index))
        );
    }

    for (tile_y = 0u; tile_y < 4u; ++tile_y)
    {
        unsigned tile_x;

        for (tile_x = 0u; tile_x < 4u; ++tile_x)
        {
            const uint8_t pattern_index = contra_rom_read_u8(
                3u,
                (uint16_t)(supertile_addr + ((uint16_t)tile_y * 4u) + tile_x)
            );

            contra_write_ppu_byte(core, (uint16_t)(tile_ppu_addr + tile_x), pattern_index);
        }

        tile_ppu_addr = (uint16_t)(tile_ppu_addr + 0x20u);
    }
}

static void contra_write_overlay_supertile_to_ppu(
    ContraCore *core,
    uint16_t supertile_data_addr,
    uint16_t palette_data_addr,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    uint16_t tile_ppu_addr;
    uint16_t attr_ppu_addr;

    contra_calculate_update_supertile_ppu_addr(core, dest_x, dest_y, &tile_ppu_addr, &attr_ppu_addr);
    contra_write_overlay_supertile_to_ppu_addr(
        core,
        supertile_data_addr,
        palette_data_addr,
        tile_ppu_addr,
        attr_ppu_addr,
        supertile_index
    );
}

static void contra_calculate_level_1_nametable_update_supertile_ppu_addr(
    const ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint16_t *tile_ppu_addr,
    uint16_t *attr_ppu_addr
)
{
    const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    const int aligned_x = (((enemy_x - 12) + scroll_offset) & ~7) - scroll_offset;
    const int aligned_y = (enemy_y - 12) & ~7;

    contra_calculate_update_supertile_ppu_addr(core, aligned_x, aligned_y, tile_ppu_addr, attr_ppu_addr);
}

static void contra_write_level_1_nametable_update_supertile_to_ppu_addr(
    ContraCore *core,
    uint16_t tile_ppu_addr,
    uint16_t attr_ppu_addr,
    uint8_t supertile_index
)
{
    contra_write_overlay_supertile_to_ppu_addr(
        core,
        contra_level_1_nametable_update_supertile_data_addr,
        contra_level_1_nametable_update_palette_data_addr,
        tile_ppu_addr,
        attr_ppu_addr,
        supertile_index
    );
}

static void contra_write_level_1_nametable_update_supertile_to_ppu(
    ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint8_t supertile_index
)
{
    uint16_t tile_ppu_addr;
    uint16_t attr_ppu_addr;

    contra_calculate_level_1_nametable_update_supertile_ppu_addr(
        core,
        enemy_x,
        enemy_y,
        &tile_ppu_addr,
        &attr_ppu_addr
    );
    contra_write_level_1_nametable_update_supertile_to_ppu_addr(
        core,
        tile_ppu_addr,
        attr_ppu_addr,
        supertile_index
    );
}

static void contra_process_level_1_weapon_box_restore(ContraCore *core)
{
    if (core->level1_weapon_box_restore_timer == 0u)
    {
        return;
    }

    core->level1_weapon_box_restore_timer = (uint8_t)(core->level1_weapon_box_restore_timer - 1u);
    if (core->level1_weapon_box_restore_timer != 0u)
    {
        return;
    }

    contra_write_level_1_nametable_update_supertile_to_ppu_addr(
        core,
        (uint16_t)core->level1_weapon_box_restore_x,
        (uint16_t)core->level1_weapon_box_restore_y,
        0x16u
    );
    core->level1_weapon_box_restore_x = 0;
    core->level1_weapon_box_restore_y = 0;
}

static void contra_render_level_1_overlay_supertile(
    ContraCore *core,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    contra_write_overlay_supertile_to_ppu(
        core,
        contra_level_1_nametable_update_supertile_data_addr,
        contra_level_1_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
    contra_render_overlay_supertile(
        core,
        contra_level_1_nametable_update_supertile_data_addr,
        contra_level_1_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
}

static void contra_render_level_2_overlay_supertile(
    ContraCore *core,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    contra_write_overlay_supertile_to_ppu(
        core,
        contra_level_2_nametable_update_supertile_data_addr,
        contra_level_2_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
    contra_render_overlay_supertile(
        core,
        contra_level_2_nametable_update_supertile_data_addr,
        contra_level_2_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
}

static void contra_render_level_1_nametable_update_supertile(
    ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint8_t supertile_index
)
{
    /*
     * The original engine writes the supertile to the nametable rounded down
     * to an 8-pixel grid in *world* space (set_ppu_addresses_in_mem
     * `AND #$F8` after adding HORIZONTAL_SCROLL/VERTICAL_SCROLL). Match that
     * so the supertile's 8x8 tiles line up with the underlying wall tiles
     * instead of leaking the wall pattern through and producing a doubled
     * appearance, while still scrolling smoothly with the level.
     */
    const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    const int aligned_x = (((enemy_x - 12) + scroll_offset) & ~7) - scroll_offset;
    const int aligned_y = (enemy_y - 12) & ~7;

    contra_write_level_1_nametable_update_supertile_to_ppu(core, enemy_x, enemy_y, supertile_index);
    contra_render_level_1_overlay_supertile(core, aligned_x, aligned_y, supertile_index);
}

static void contra_render_level_1_bridge_cloud(
    ContraCore *core,
    const ContraNativeEnemy *enemy,
    uint8_t *sprite_order
)
{
    uint8_t sprite_index;

    if ((enemy->state != CONTRA_NATIVE_LEVEL1_STATE_EMERGE) ||
        (enemy->cooldown == 0u) ||
        (enemy->cooldown >= 4u) ||
        (enemy->timer == 0u))
    {
        return;
    }

    sprite_index = (enemy->timer < 4u) ? (uint8_t)(4u - enemy->timer) : 0u;
    if (sprite_index >= 4u)
    {
        sprite_index = 3u;
    }

    *sprite_order = contra_render_native_sprite_code(
        core,
        contra_level_1_bridge_cloud_sprite_tbl[sprite_index],
        0x00u,
        (uint8_t)((int)enemy->x + contra_level_1_bridge_cloud_x_offset[enemy->cooldown]),
        (uint8_t)(((int)enemy->y + contra_level_1_bridge_cloud_y_offset[enemy->cooldown]) + 0x10),
        *sprite_order
    );
}

static void contra_render_level_2_wall_core_updates(ContraCore *core, const ContraNativeEnemy *enemy)
{
    unsigned phase;

    if ((enemy->type != 0x14u) || ((enemy->state != 0x08u) && (enemy->state != 0x09u)))
    {
        return;
    }

    for (phase = 0u; phase < 4u; ++phase)
    {
        if ((enemy->flags & (uint8_t)(1u << phase)) == 0u)
        {
            continue;
        }

        contra_render_level_2_overlay_supertile(
            core,
            (int)contra_level_2_wall_core_update_x_tbl[phase],
            (int)contra_level_2_wall_core_update_y_tbl[phase],
            contra_level_2_wall_core_update_supertile_tbl[phase]
        );
    }
}

/* level_2_4_tile_animation (bank 3, CPU $86E1): 11 five-byte entries
   {row_flag, tile0, tile1, tile2, tile3}. For level 2/4 the row flag is always 0,
   meaning a 2x2 pattern-tile block (16x16 px): tile0,tile1 on the top row,
   tile2,tile3 below. (data bank3.asm:185; drawn by update_enemy_nametable_tiles
   bank7.asm:8631 / update_nametable_tiles bank7.asm:1643.) */
#define CONTRA_LEVEL_2_4_TILE_ANIMATION_ADDR 0x86E1u

/* Draw the 2x2 level_2_4_tile_animation block for offset anim_offset (0x80..0x8a)
   at the enemy position, as a framebuffer overlay. Mirrors the placement of
   update_enemy_nametable_tiles (top-left = enemy pos - 4, rounded to the 8px tile
   grid in world space so it lines up with the back wall).

   Palette: the wall turret/core write their attribute quadrant when they draw
   (update_nametable_tiles, bank7.asm:1676) by merging the tile_animation entry's
   first byte (the "row flag") into the super-tile palette for that quadrant. For
   every level_2_4_tile_animation entry that first byte is $00, so the structure's
   quadrant is forced to background palette slot 0 -- which is why the metallic
   turret/core stands out against the blue (slot 3) back wall. The open frames
   inherit this with bit 7 set ("leave existing palette"), so they stay slot 0
   too. We therefore draw the whole block at slot 0 rather than inheriting the
   surrounding wall's palette (which would tint the turret blue and wash out the
   head). */
static void contra_render_level_2_tile_animation(
    ContraCore *core,
    int enemy_x,
    int enemy_y,
    uint8_t anim_offset
)
{
    const uint8_t index = (uint8_t)(anim_offset & 0x7Fu);
    const uint16_t entry = (uint16_t)(CONTRA_LEVEL_2_4_TILE_ANIMATION_ADDR + ((uint16_t)index * 5u));
    const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    const int aligned_x = (((enemy_x - 4) + scroll_offset) & ~7) - scroll_offset;
    const int aligned_y = (enemy_y - 4) & ~7;
    unsigned tile_y;

    if (!contra_load_rom_image())
    {
        return;
    }

    for (tile_y = 0u; tile_y < 2u; ++tile_y)
    {
        unsigned tile_x;

        for (tile_x = 0u; tile_x < 2u; ++tile_x)
        {
            const uint8_t pattern_index = contra_rom_read_u8(
                3u, (uint16_t)(entry + 1u + (tile_y * 2u) + tile_x));
            const int px = aligned_x + (int)(tile_x * 8u);
            const int py = aligned_y + (int)(tile_y * 8u);

            contra_draw_background_tile(core, px, py, pattern_index, 0u);
        }
    }
}

/* Faithful level-2 wall turret/core (types 0x13/0x14) live on real RAM. Their
   background appears only via this overlay re-draw each render frame, from the
   per-slot tile cache the routines populate at their draw points. */
static void contra_render_level_2_wall_structures(ContraCore *core)
{
    int slot;

    for (slot = 0; slot < 16; ++slot)
    {
        const uint8_t sx = (uint8_t)slot;
        const uint8_t type = core->ram[CONTRA_RAM_ENEMY_TYPE + sx];
        const uint8_t tile = core->l2_structure_tile[sx];
        const uint8_t supertile = core->l2_supertile[sx];

        if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + sx] == 0u)
        {
            continue;
        }
        /* 0xFE = a killed structure mid-explosion: keep its last (destroyed) tile
           on the wall under the explosion sprite until the room reloads. The
           core/turret draw a 2x2 tile-animation block; the boss-room cannon and
           plating draw a 4x4 super-tile. */
        if ((tile != 0u) && ((type == 0x13u) || (type == 0x14u) || (type == 0xFEu)))
        {
            contra_render_level_2_tile_animation(
                core,
                (int)core->ram[CONTRA_RAM_ENEMY_X_POS + sx],
                (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + sx],
                tile);
        }
        if ((supertile != 0xFFu) && ((type == 0x08u) || (type == 0x0Au) || (type == 0xFEu)))
        {
            const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
            const int ex = (int)core->ram[CONTRA_RAM_ENEMY_X_POS + sx];
            const int ey = (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + sx];
            const int aligned_x = (((ex - 12) + scroll_offset) & ~7) - scroll_offset;
            const int aligned_y = (ey - 12) & ~7;

            contra_render_level_2_overlay_supertile(core, aligned_x, aligned_y, supertile);
        }
    }

    /* back-wall blow-open: 4 destroyed quadrant super-tiles at fixed positions,
       drawn one-by-one by wall_core_routine_08 and persisting until the room
       reloads (same positions the invented path uses). */
    if (core->l2_blowopen_quadrants != 0u)
    {
        unsigned q;

        for (q = 0u; q < 4u; ++q)
        {
            if ((core->l2_blowopen_quadrants & (uint8_t)(1u << q)) != 0u)
            {
                contra_render_level_2_overlay_supertile(
                    core,
                    (int)contra_level_2_wall_core_update_x_tbl[q],
                    (int)contra_level_2_wall_core_update_y_tbl[q],
                    contra_level_2_wall_core_update_supertile_tbl[q]);
            }
        }
    }
}

static void contra_render_native_enemies(ContraCore *core)
{
    uint8_t sprite_order = 0x80u;
    size_t enemy_index;
    const bool level_1_active = contra_is_native_level_1_active(core);
    const bool level_2_active = contra_is_native_level_2_active(core);

    if (!contra_is_native_combat_active(core))
    {
        return;
    }

    if (CONTRA_USE_ROM_ENEMY_SYSTEM && level_1_active)
    {
        /* Faithful enemies render their own super-tiles during their routines
           (set_weapon_box_supertile etc.) and their OAM sprites via the OAM
           build; nothing to do per-frame here. */
        return;
    }

    if (CONTRA_USE_ROM_ENEMY_SYSTEM_L2 && level_2_active)
    {
        /* Faithful indoor enemies live on real RAM: re-draw the wall turret/core
           backgrounds from the per-slot tile cache; soldiers/bullets render via
           the OAM build. Skip the invented core->enemies[] path entirely. */
        contra_render_level_2_wall_structures(core);
        return;
    }

    if (level_1_active)
    {
        for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
        {
            const ContraNativeEnemy *const enemy = &core->enemies[enemy_index];

            if (enemy->active == 0u)
            {
                continue;
            }

            switch (enemy->type)
            {
                case 0x02u:
                case 0x04u:
                case 0x07u:
                case 0x10u:
                    contra_render_level_1_nametable_update_supertile(
                        core,
                        (enemy->type == 0x10u)
                            ? contra_level_1_boss_bomb_turret_render_x(enemy)
                            : (int)enemy->x,
                        (int)enemy->y,
                        enemy->sprite_code
                    );
                    break;

                case 0x12u:
                {
                    unsigned section;

                    for (section = 0u; section < 4u; ++section)
                    {
                        const uint8_t overlay = contra_level_1_bridge_get_overlay(enemy, section);

                        if (overlay == 0u)
                        {
                            continue;
                        }

                        contra_render_level_1_overlay_supertile(
                            core,
                            ((int)enemy->x - 12) + (int)(section * 32u),
                            (int)enemy->y - 12,
                            overlay
                        );
                    }
                    break;
                }

                default:
                    break;
            }
        }
    }
    else if (level_2_active)
    {
        for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
        {
            const ContraNativeEnemy *const enemy = &core->enemies[enemy_index];

            if (enemy->active == 0u)
            {
                continue;
            }

            contra_render_level_2_wall_core_updates(core, enemy);
        }
    }

    for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
    {
        const ContraNativeEnemy *const enemy = &core->enemies[enemy_index];

        if (enemy->active == 0u)
        {
            continue;
        }

        switch (enemy->type)
        {
            case 0x00u:
            case 0x03u:
            case 0x05u:
            case 0x06u:
            case 0x15u:
            case 0x16u:
            case 0x17u:
            case 0x18u:
            case CONTRA_NATIVE_LEVEL1_TYPE_EXPLOSION:
                break;

            case 0x12u:
                contra_render_level_1_bridge_cloud(core, enemy, &sprite_order);
                break;

            default:
                break;
        }
    }

    (void)sprite_order;
}

static bool contra_native_enemy_uses_cpu_sprite_buffer(const ContraNativeEnemy *enemy)
{
    switch (enemy->type)
    {
        case 0x00u:
        case 0x03u:
        case 0x05u:
        case 0x06u:
        case 0x15u:
        case 0x16u:
        case 0x17u:
        case 0x18u:
        case CONTRA_NATIVE_LEVEL1_TYPE_EXPLOSION:
            return enemy->sprite_code != 0u;

        default:
            return false;
    }
}

static void contra_write_cpu_sprite_slot(
    ContraCore *core,
    size_t slot,
    uint8_t sprite_code,
    uint8_t sprite_y,
    uint8_t sprite_x,
    uint8_t sprite_attr
)
{
    core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + slot] = sprite_code;
    core->ram[CONTRA_RAM_SPRITE_Y_POS + slot] = sprite_y;
    core->ram[CONTRA_RAM_SPRITE_X_POS + slot] = sprite_x;
    core->ram[CONTRA_RAM_SPRITE_ATTR + slot] = sprite_attr;
}

static void contra_sync_native_sprite_objects_to_cpu_buffer(ContraCore *core)
{
    int sprite_slot;
    size_t enemy_index;

    if ((CONTRA_USE_ROM_ENEMY_SYSTEM && (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0u)) ||
        (CONTRA_USE_ROM_ENEMY_SYSTEM_L2 && (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u)))
    {
        /* The faithful enemy system maintains the enemy sprite-object slots
           (ENEMY_SPRITES / ENEMY_Y_POS / ENEMY_X_POS = sprite slots 10..25)
           directly in RAM, exactly as the ROM does — they are persistent enemy
           state, not a per-frame rebuild. Don't clear them and rebuild from the
           invented core->enemies[] struct. */
        return;
    }

    for (sprite_slot = (int)CONTRA_CPU_SPRITE_RENDER_SLOTS - 1; sprite_slot >= 10; --sprite_slot)
    {
        contra_write_cpu_sprite_slot(core, (size_t)sprite_slot, 0x00u, 0x00u, 0x00u, 0x00u);
    }

    sprite_slot = (int)CONTRA_CPU_SPRITE_RENDER_SLOTS - 1;
    for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
    {
        const ContraNativeEnemy *const enemy = &core->enemies[enemy_index];

        if ((enemy->active == 0u) || !contra_native_enemy_uses_cpu_sprite_buffer(enemy))
        {
            continue;
        }

        contra_write_cpu_sprite_slot(
            core,
            (size_t)sprite_slot,
            enemy->sprite_code,
            (uint8_t)enemy->y,
            (uint8_t)enemy->x,
            enemy->sprite_attr
        );

        --sprite_slot;
        if (sprite_slot < 10)
        {
            return;
        }
    }

    for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMY_PROJECTILES; ++enemy_index)
    {
        const ContraNativeProjectile *const projectile = &core->enemy_projectiles[enemy_index];

        if ((projectile->active == 0u) || (projectile->sprite_code == 0u))
        {
            continue;
        }

        contra_write_cpu_sprite_slot(
            core,
            (size_t)sprite_slot,
            projectile->sprite_code,
            (uint8_t)projectile->y,
            (uint8_t)projectile->x,
            projectile->sprite_attr
        );

        --sprite_slot;
        if (sprite_slot < 10)
        {
            return;
        }
    }
}

static void contra_latch_cpu_sprite_state(ContraCore *core)
{
    size_t sprite_index;

    contra_sync_native_sprite_objects_to_cpu_buffer(core);

    for (sprite_index = 0u; sprite_index < CONTRA_CPU_SPRITE_RENDER_SLOTS; ++sprite_index)
    {
        core->latched_cpu_sprite_buffer[sprite_index] =
            core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + sprite_index];
        core->latched_sprite_y[sprite_index] =
            core->ram[CONTRA_RAM_SPRITE_Y_POS + sprite_index];
        core->latched_sprite_x[sprite_index] =
            core->ram[CONTRA_RAM_SPRITE_X_POS + sprite_index];
        core->latched_sprite_attr[sprite_index] =
            core->ram[CONTRA_RAM_SPRITE_ATTR + sprite_index];
    }

    core->latched_sprite_load_type = core->ram[CONTRA_RAM_SPRITE_LOAD_TYPE];
    core->latched_demo_mode = core->ram[CONTRA_RAM_DEMO_MODE];
    core->latched_player_mode = core->ram[CONTRA_RAM_PLAYER_MODE];
    core->latched_num_lives[0] = core->ram[CONTRA_RAM_P1_NUM_LIVES];
    core->latched_num_lives[1] = core->ram[CONTRA_RAM_P2_NUM_LIVES];
    core->latched_game_over_status[0] = core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS];
    core->latched_game_over_status[1] = core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS];
    contra_build_oam_for_next_frame(core);
}

static void contra_latch_ppu_render_state(ContraCore *core)
{
    core->latched_horizontal_scroll = core->ram[CONTRA_RAM_HORIZONTAL_SCROLL];
    core->latched_vertical_scroll = core->ram[CONTRA_RAM_VERTICAL_SCROLL];
    core->latched_level_screen_number = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    core->latched_level_screen_scroll_offset = core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    core->latched_ppuctrl_settings = core->ram[CONTRA_RAM_PPUCTRL_SETTINGS];
}

static void contra_apply_pending_palette_write(ContraCore *core)
{
    if (core->pending_palette_write == 0u)
    {
        return;
    }

    memcpy(core->ppu_palette, core->pending_palette, sizeof(core->ppu_palette));
    core->pending_palette_write = 0x00u;
    core->pending_palette_count = 0x00u;
}

static void contra_render_frame(ContraCore *core, bool update_latches)
{
    const uint8_t *const ram = core->ram;
    const uint8_t game_routine = ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    const uint8_t level_routine = ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX];
    const bool demo_level_scene = (game_routine == 0x02u) && (ram[CONTRA_RAM_DEMO_MODE] != 0u);
    const bool gameplay_scene = (game_routine == 0x05u) || demo_level_scene;
    const bool game_over_screen_scene =
        gameplay_scene &&
        (level_routine >= 0x05u) &&
        (level_routine <= 0x07u) &&
        (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0u);
    const uint32_t clear_color = contra_background_palette_color_rgba(core, 0u, 0u);
    size_t pixel_index;

    /*
     * During level_routine_03 the original game is still building the opening
     * gameplay nametable off-screen. Rendering the current PPU state here
     * exposes transient junk or blank frames, so keep the last stage-card
     * frame visible until level_routine_04 begins actual gameplay.
     */
    if (gameplay_scene && (level_routine == 0x03u))
    {
        if (update_latches)
        {
            contra_latch_cpu_sprite_state(core);
            contra_latch_ppu_render_state(core);
            contra_apply_pending_palette_write(core);
        }
        return;
    }

    for (pixel_index = 0; pixel_index < (size_t)(CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT); ++pixel_index)
    {
        core->framebuffer[pixel_index] = clear_color;
        core->background_opaque[pixel_index] = 0u;
        core->sprite_priority[pixel_index] = 0xFFu;
    }

    if ((game_routine <= 0x03u) && !demo_level_scene)
    {
        contra_render_intro_background(core);
        contra_render_cpu_sprites(core);
        if (update_latches)
        {
            contra_latch_cpu_sprite_state(core);
            contra_latch_ppu_render_state(core);
            contra_apply_pending_palette_write(core);
        }
        return;
    }

    /*
     * The level intro / score screen still uses the PPU nametable text path.
     * Keep that view through level_routine_03 because the level background
     * initialization is still in flight there; switching early exposes
     * transient floor-only frames before gameplay begins.
     */
    if (gameplay_scene && ((level_routine < 0x04u) || game_over_screen_scene))
    {
        contra_render_intro_background(core);
    }
    else if (gameplay_scene && (level_routine >= 0x04u))
    {
        contra_render_level_background(core);
    }

    if (!game_over_screen_scene)
    {
        contra_render_native_enemies(core);
    }
    contra_render_cpu_sprites(core);
    if (update_latches)
    {
        contra_latch_cpu_sprite_state(core);
        contra_latch_ppu_render_state(core);
        contra_apply_pending_palette_write(core);
    }
}

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

    contra_flush_pending_horizontal_level_writes(core);
    contra_process_level_1_weapon_box_restore(core);
    contra_apply_controller_state(core);
    contra_exe_game_routine(core);
    contra_flush_cpu_graphics_buffer_to_ppu(core);
    contra_render_frame(core, core->level_graphics_wait_frames == 0u);
}

const uint32_t *contra_core_framebuffer(const ContraCore *core)
{
    return core->framebuffer;
}

const uint8_t *contra_core_ram(const ContraCore *core)
{
    return core->ram;
}
