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

static void contra_render_level_1_nametable_update_supertile(
    ContraCore *core, int enemy_x, int enemy_y, uint8_t supertile_index);
static void contra_rom_update_enemy_pos(ContraCore *core, uint8_t x);
static void contra_rom_bullet_generation(
    ContraCore *core, uint8_t aim, uint8_t speed, uint8_t px, uint8_t py);
static void contra_rom_destroy_all_enemies(ContraCore *core, int keep_slot);
static void contra_rom_add_10_to_enemy_y_fract_vel(ContraCore *core, uint8_t x);
static void contra_rom_add_a_to_enemy_y_fract_vel(ContraCore *core, uint8_t x, uint8_t a);
static uint8_t contra_rom_get_bg_collision_far(const ContraCore *core, uint8_t x, uint8_t y);
static void contra_rom_add_player_score(ContraCore *core, uint8_t player, uint8_t add_lo, uint8_t add_hi);
static void contra_rom_create_explosion_at(ContraCore *core, uint8_t px, uint8_t py);
static void contra_rom_create_explosion_sequence(
    ContraCore *core, uint8_t px, uint8_t py, uint8_t state_width, uint8_t routine);
static void contra_load_next_supertiles_screen_indexes(ContraCore *core);
static void contra_rom_reverse_enemy_x_direction(ContraCore *core, uint8_t x);
static uint8_t contra_rom_player_enemy_x_dist(const ContraCore *core, uint8_t x);

/* Faithful enemy types that render as background super-tiles (nametable writes)
   rather than OAM sprites: pill box, rotating gun, red turret, bomb turret,
   plated door, bridge. */
/* PORT HARNESS (enemy_type_is_supertile): no single ASM routine -- classify enemy types rendered as background super-tiles. */
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
    /* index 0x00 is NES medium gray, not green -- it was duplicating 0x09's value,
       which turned every 0x00 background (e.g. the level-2 base walls) green. */
    0x00666666u, 0x00002A88u, 0x001412A7u, 0x003B00A4u, 0x005C007Eu, 0x006E0040u, 0x006C0600u, 0x00561D00u,
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
/* player_frame_sprite_tbl_04 (bank2:978): the indoor (base) level walk frames. */
static const uint8_t contra_player_frame_sprite_tbl_04[6] = {0x51u, 0x52u, 0x53u, 0x51u, 0x52u, 0x53u};
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
/* bullet_initial_pos_02/03 (bank6:$b626/$b63a): the INDOOR offset tables --
   used by the indoor BOSS screen too (init_bullet_sprite_pos selects them for
   any non-zero location type, while the velocity stays outdoor-style). */
static const int8_t contra_bullet_initial_pos_indoor_ground[10][2] = {
    { -1, -24}, { 15, -16}, { 16,  -6}, { 15,   8}, { 16,  11},
    {-16,  11}, {-15,   8}, {-16,  -6}, {-15, -16}, { -1, -24}
};
static const int8_t contra_bullet_initial_pos_indoor_jump[12][2] = {
    {  0, -16}, { 15, -15}, { 16,   0}, { 15,  15},
    {  0,  16}, {  0,  16}, {-15,  15}, {-16,   0},
    {-15, -15}, {  0, -16}, {  0, -16}, {  0, -16}
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
static const uint8_t contra_level_end_level_delay_timer_tbl[8] = {0xA0u, 0xA0u, 0xE0u, 0xA0u, 0xA0u, 0xA0u, 0xA0u, 0xA0u};
static const uint16_t contra_level_1_nametable_update_supertile_data_addr = 0x83B1u;
static const uint16_t contra_level_1_nametable_update_palette_data_addr = 0x86ACu;
static const uint16_t contra_level_2_nametable_update_supertile_data_addr = 0x88A8u;
static const uint16_t contra_level_2_nametable_update_palette_data_addr = 0x8E91u;
/* Boss-room (LEVEL_LOCATION_TYPE bit 7) destructible overlay set: update_nametable_supertile
   (bank7:1356-1365) forces ptr-table index 8 -> level_2_4_nametable_update_supertile_data
   (bank3:1146, CPU $BA1A) + level_2_4_boss_nametable_update_palette_data (bank3:1212, $BDC4).
   This is the cannon/plating housing graphics, distinct from the corridor set at $88A8. */
static const uint16_t contra_level_2_4_boss_nametable_update_supertile_data_addr = 0xBA1Au;
static const uint16_t contra_level_2_4_boss_nametable_update_palette_data_addr = 0xBDC4u;
/* level_3_nametable_update_supertile_data / _palette_data (bank3, from the cc65
   symbol map): the dragon-boss mouth (and its defeat) supertiles. */
static const uint16_t contra_level_3_nametable_update_supertile_data_addr = 0x9368u;
static const uint16_t contra_level_3_nametable_update_palette_data_addr = 0x965Fu;
static const uint16_t contra_level_7_tile_animation_addr = 0xA56Eu;
static const uint16_t contra_level_7_nametable_update_supertile_data_addr = 0xABEAu;
static const uint16_t contra_level_7_nametable_update_palette_data_addr = 0xAD4Au;
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

/* PORT HARNESS (read_u8): no single ASM routine -- read a byte from the PRG-ROM image. */
static uint8_t contra_rom_read_u8(uint8_t bank, uint16_t cpu_addr)
{
    const size_t offset = contra_prg_rom_offset(bank, cpu_addr);

    if ((!contra_load_rom_image()) || (offset >= contra_rom_image.size))
    {
        return 0u;
    }

    return contra_rom_image.bytes[offset];
}

/* PORT HARNESS (read_u16): no single ASM routine -- read a little-endian word from the PRG-ROM image. */
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
           state. Background-tile enemy types (pill box, turrets, door, bridge)
           draw their *structure* to the nametable, not OAM, so the live structure
           is suppressed here to avoid a stray sprite (its ENEMY_SPRITES is the
           invisible placeholder 0x01). But when such a slot is DESTROYED it runs
           the shared in-slot explosion routine (boss_defeated -> routine 4) which
           sets a real explosion sprite (0x38..) on the SAME slot while keeping its
           type; that death cloud must still render (the ROM's draw_enemy_sprites
           draws every nonzero sprite). So only suppress the invisible placeholder. */
        if ((core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0u) &&
            (sprite_index >= 10u) &&
            (core->ram[CONTRA_RAM_ENEMY_SPRITES + (sprite_index - 10u)] <= 0x01u) &&
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

/* Vertical-level scrolled background. The original camera climbs UP the level: as the
   player ascends, VERTICAL_SCROLL counts DOWN and the *next* screen scrolls in from
   the TOP. LEVEL_SCREEN_SCROLL_OFFSET counts the opposite way (0->0xf0 per screen), so
   the visible window's top sits `240 - scroll_off` pixels into a two-screen column
   whose UPPER half is the next screen (screen_number + 1) and lower half the current
   screen. (contra_vertical_collision_screen_row uses the identical mapping so floors
   stay aligned.) */
static void contra_render_vertical_level_background_scrolled(ContraCore *core)
{
    const uint8_t *const ram = core->ram;
    const uint16_t supertile_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u));
    const uint16_t palette_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] << 8u));
    const uint8_t screen_number = core->latched_level_screen_number;
    const int top_combined = 240 - (int)core->latched_level_screen_scroll_offset;
    uint8_t screen_supertiles[CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE];
    int cached_screen = -1;
    int c;

    for (c = (top_combined & ~7); ; c += 8)
    {
        const int dest_y = c - top_combined;
        const uint8_t data_screen = (c < 240) ? (uint8_t)(screen_number + 1u) : screen_number;
        const uint16_t row_px = (c < 240) ? (uint16_t)c : (uint16_t)(c - 240);
        const uint8_t tile_row = (uint8_t)(row_px >> 3u);       /* 0..29 within the screen */
        const size_t supertile_row = (size_t)(tile_row >> 2u);
        const uint8_t tile_y_in_supertile = (uint8_t)(tile_row & 0x03u);
        size_t tile_x;

        if (dest_y >= (int)CONTRA_FRAMEBUFFER_HEIGHT)
        {
            break;
        }

        if ((int)data_screen != cached_screen)
        {
            memset(screen_supertiles, 0, sizeof(screen_supertiles));
            contra_decode_level_screen_supertiles(core, data_screen, screen_supertiles, 0u);
            cached_screen = (int)data_screen;
        }

        for (tile_x = 0u; tile_x < 32u; ++tile_x)
        {
            const size_t supertile_column = tile_x / 4u;
            const size_t supertile_offset = (supertile_row * 8u) + supertile_column;
            const uint8_t supertile_index = screen_supertiles[supertile_offset];
            const size_t supertile_data_addr = (size_t)supertile_index * 16u;
            const uint8_t tile_in_supertile =
                (uint8_t)((tile_y_in_supertile << 2u) | (tile_x & 0x03u));
            const uint8_t pattern_index =
                contra_rom_read_u8(3u, (uint16_t)(supertile_ptr + supertile_data_addr + tile_in_supertile));
            const uint8_t supertile_palette =
                contra_rom_read_u8(3u, (uint16_t)(palette_ptr + supertile_index));
            const uint8_t palette_shift =
                (uint8_t)(((tile_y_in_supertile & 0x02u) << 1u) | (tile_x & 0x02u));
            const uint8_t palette_slot = (uint8_t)((supertile_palette >> palette_shift) & 0x03u);

            contra_draw_background_tile(core, (int)(tile_x * 8u), dest_y, pattern_index, palette_slot);
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

    /* Boss room: recompose the flat mechanical-wall super-tile layout each frame.
       At boss entry the 3 super-tile pointers are repointed to the boss tables
       ($9013/$b57a/$bd7a) and the wall layout decoded, but the indoor column-advance
       (advance_horizontal_level_ppu_column, LEVEL_SCREEN_NUMBER+2 = out of range for
       the 2-entry boss screen table) clears level_screen_supertiles afterward, so we
       re-decode it here while the boss room is static. */
    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u)
    {
        contra_decode_level_screen_supertiles(
            core, (uint8_t)(ram[CONTRA_RAM_CURRENT_LEVEL] >> 1u),
            core->level_screen_supertiles, 0u);
    }

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u))
    {
        contra_render_horizontal_level_background_scrolled(core);
        return;
    }

    if ((ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) &&
        (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u))
    {
        contra_render_vertical_level_background_scrolled(core);
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

    /* zero_out_nametables falls through to write_graphic_data_to_ppu (bank7),
       which resets both scroll offsets so the freshly drawn screen (game over /
       intro / title) sits at scroll origin instead of the gameplay scroll. */
    core->ram[CONTRA_RAM_VERTICAL_SCROLL] = 0x00u;
    core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] = 0x00u;
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
    /* clear_bullet_values (bank6:1716) clears everything below EXCEPT the X/Y
       velocity sub-pixel accumulators: those keep their phase from the previous
       bullet that used this slot. Zeroing them (as we did) desynced a re-used
       slot's first move -- it crossed a pixel boundary one frame early. */
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
        contra_init_player_bullet_position(core, (size_t)bullet_slot, player_index, aim_dir);
        contra_set_spray_bullet_velocity(core, (size_t)bullet_slot, aim_dir, bullet_num, rapid_fire);
        if (contra_in_indoor_base_level(ram))
        {
            contra_set_indoor_bullet_velocity(core, (size_t)bullet_slot, player_index, rapid_fire);
        }
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

/* Faithful exploding-bridge collision gap: true when (screen_x, screen_y) falls
   in a background super-tile the real-RAM bridge has cleared. The gap is anchored
   in world space (screen<<8 + scroll + x is scroll-invariant), so it persists as
   the level scrolls and after the bridge enemy is removed. When the faithful
   system is off this list stays empty, so the check is a no-op. */
/* PORT HARNESS (supertile_collision_override): no single ASM routine -- look up
   the port-side runtime collision rewrites (bridge gaps via clear_supertile_bg_
   collision, boss-door tunnel cells via set_supertile_bg_collision) at a world
   position. Returns true with the ROM collision code for the queried point:
   the stored nibble holds bits 0-1 = top 16px row, bits 2-3 = bottom row. */
static bool contra_rom_supertile_collision_override(
    const ContraCore *core, uint8_t screen_x, uint8_t screen_y, uint8_t *out_code)
{
    static const uint8_t collision_code_lookup_tbl[4] = {0x00u, 0x01u, 0x02u, 0x80u};
    const uint16_t world_x =
        (uint16_t)(((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                   core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + screen_x);
    uint8_t i;

    for (i = 0u; i < core->l1_bridge_gap_count; ++i)
    {
        /* clear_supertile_bg_collision (bank7:8143) clears the GRID-ALIGNED
           super-tile cell containing the draw point; the stored coordinates
           are the visual draw anchor (a few px inside the cell), so snap to
           the 32px grid here -- the world grid at multiples of 32, the screen
           rows at 0x10 + 32k. Unsnapped, the gap leaked 4px into the intact
           neighbor cell and a soldier stepped off a bridge the ROM still
           considers floor. */
        const uint16_t gx = (uint16_t)(core->l1_bridge_gap_world_x[i] & ~(uint16_t)31u);
        const int gy = ((((int)core->l1_bridge_gap_screen_y[i] - 0x10) & ~31) + 0x10);

        if ((world_x >= gx) && (world_x < (uint16_t)(gx + 32u)) &&
            ((int)screen_y >= gy) && ((int)screen_y < (gy + 32)))
        {
            const uint8_t nibble = core->l1_bridge_gap_coll[i];
            const uint8_t bits = (((int)screen_y - gy) >= 16)
                ? (uint8_t)(nibble >> 2u)
                : nibble;

            *out_code = collision_code_lookup_tbl[bits & 0x03u];
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

    {
        uint8_t override_code;

        if (contra_rom_supertile_collision_override(core, screen_x, screen_y, &override_code))
        {
            return override_code;
        }
    }

    /* BG_COLLISION_DATA granularity (set_tile_collision, bank7:6222): the ROM
       classifies ONE pattern tile per 16x16 quadrant -- the quadrant's top-left
       8x8 tile (odd nametable columns and rows are skipped during the build).
       Snap the probe to that tile or probes in a quadrant's right/bottom 8px
       read a tile the ROM never consults. */
    if (!contra_read_level_collision_pattern_index(
            core,
            (uint16_t)((world_pixel_x >> 3u) & ~(uint16_t)1u),
            (uint8_t)(((screen_y - 0x10u) >> 3u) & 0xFEu),
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

    /* read_bg_collision_byte (bank7:6404): the raw screen Y (pre-scroll, $15)
       past 0xE0 is "past last bg collision row" -> empty. On the waterfall
       this is what lets falling rocks drop off the bottom instead of bouncing
       on the last supertile row. */
    if (screen_y >= 0xE0u)
    {
        return 0u;
    }

    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u)
    {
        return contra_get_outdoor_horizontal_bg_collision(core, screen_x, screen_y);
    }

    {
        /* bg_collision_logic (bank7:6439): $11 = Y + VERTICAL_SCROLL, +0x10 when
           the 8-bit add carries OR lands in 0xF0..0xFF (skipping the attribute
           rows). BG_COLLISION_DATA is a 15-row ring updated as the level streams:
           a row belongs to the NEXT screen once it has fully scrolled in at the
           window top (row*16 >= VERTICAL_SCROLL), and still holds the CURRENT
           screen's stale data otherwise -- including rows that have scrolled off
           the bottom. The waterfall soldier generator depends on the stale rows:
           probes at y=0..6 wrap to ring row 12 (only partially revealed for the
           next screen) and find the current screen's lone left-column floor
           (frame 20968), while freshly streamed top-of-screen soldiers stand on
           fully-revealed next-screen rows (frame 16121). */
        const uint8_t screen_number = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
        const uint8_t vs = core->ram[CONTRA_RAM_VERTICAL_SCROLL];
        /* the next screen's rows stream in at the window top as the level
           scrolls; rows at/below 240-scroll_off have been written. At off=0
           (unscrolled screen) the boundary is 240: no ring row belongs to the
           next screen yet. */
        const uint16_t streamed_boundary =
            (uint16_t)(240u - core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET]);
        const uint16_t sum = (uint16_t)screen_y + vs;
        uint8_t row_px = (uint8_t)sum;

        if ((sum > 0xFFu) || (row_px >= 0xF0u))
        {
            row_px = (uint8_t)(row_px + 0x10u);
        }
        /* quadrant snap: BG_COLLISION_DATA samples only the top-left tile of
           each 16x16 quadrant (set_tile_collision, bank7:6222) */
        {
            const uint8_t data_screen = ((uint16_t)(row_px & 0xF0u) >= streamed_boundary)
                ? (uint8_t)(screen_number + 1u)
                : screen_number;
            const uint16_t world_tile_y =
                (uint16_t)((uint16_t)data_screen * 30u +
                           (((uint16_t)row_px >> 3u) & ~(uint16_t)1u));

            if (!contra_read_level_collision_pattern_index(
                    core, (uint16_t)((screen_x >> 3u) & 0x1Eu), world_tile_y, &pattern_index))
            {
                return 0u;
            }
        }
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
        if (sequence == 0x00u)
        {
            /* facing up / standing (player_sprite_indoor_facing_up, bank2:1321) */
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x50u;
        }
        else if (sequence == 0x01u)
        {
            /* electrocuted by the fence (player_sprite_indoor_electrocuted,
               bank2:1325) -- was wrongly drawn as facing-up (0x50), so the shock
               pose never showed when you pressed Up into the live fence. */
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x55u;
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
        else if ((sequence == 0x03u) || (sequence == 0x04u))
        {
            /* indoor walking animation (player_sprite_indoor_walking_animation,
               bank2:1305 -> set_player_frame_sprite_from_a): cycle the 6-frame walk
               table (player_frame_sprite_tbl_00 normally, _04 while firing),
               advancing one frame every 8 game frames. This is the sideways-walk
               leg animation the indoor branch previously left static. */
            const uint8_t *const frame_table =
                (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER + player_index] != 0u)
                    ? contra_player_frame_sprite_tbl_04
                    : contra_player_frame_sprite_tbl_00;

            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] =
                frame_table[core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] % 6u];
            core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] =
                (uint8_t)(core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] + 1u);
            if ((core->ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] & 0x07u) == 0u)
            {
                core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] =
                    (uint8_t)((core->ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] + 1u) % 6u);
            }
        }
        else
        {
            /* default to facing-up; sequences 4/6 are handled as death above this
               indoor block, so this effectively only catches the standing pose. */
            core->ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x50u;
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

/* apply_gravity (bank7:5115-5123): increment the player's Y fractional velocity by
   #$23, carrying into the fast (whole-pixel) velocity. */
static void contra_apply_gravity(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] + 0x23u);
    if (ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index] < 0x23u)
    {
        ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] + 1u);
    }
}

/* player_jumping_set_y_pos (bank7:5093-5112): advance the jump coefficient by the
   fractional velocity and move the player's Y position by the fast velocity (plus
   the coefficient carry), updating the off-screen HIDDEN counter. */
static void contra_player_jumping_set_y_pos(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t previous_y = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    const uint8_t visibility_delta =
        ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) != 0u) ? 0xFFu : 0x00u;
    uint8_t jump_carry = 0u;
    uint16_t y_sum;

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

/* The common @set_y_pos path (bank7:4730-4740): gravity then move the player by
   velocity. Used by the edge-fall and indoor paths that never vertical-scroll. */
static void contra_apply_gravity_set_player_y(ContraCore *core, uint8_t player_index)
{
    contra_apply_gravity(core, player_index);
    contra_player_jumping_set_y_pos(core, player_index);
}

/* set_boss_auto_scroll (bank7:6528-6548): once the player reaches the boss screen
   (LEVEL_STOP_SCROLL == LEVEL_SCREEN_NUMBER) and has scrolled past the trigger
   offset, start the auto-scroll that reveals the boss (AUTO_SCROLL_TIMER_00) and
   latch LEVEL_STOP_SCROLL to #$ff. Returns true when the boss auto-scroll is active
   (the caller then skips the player-driven scroll). */
static bool contra_rom_set_boss_auto_scroll(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    static const uint8_t scroll_trigger_tbl[2] = {0xA0u, 0xC0u};   /* bank7:6553 */
    static const uint8_t auto_scroll_timer_tbl[2] = {0x60u, 0x40u}; /* bank7:6559 */
    const uint8_t st = (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ? 1u : 0u;

    if (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0u)
    {
        return false; /* boss defeated: resume normal scroll */
    }
    if ((ram[CONTRA_RAM_LEVEL_STOP_SCROLL] & 0x80u) != 0u)
    {
        return true; /* auto-scroll already started */
    }
    if (ram[CONTRA_RAM_LEVEL_STOP_SCROLL] != ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER])
    {
        return false; /* not yet on the boss screen */
    }
    if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] < scroll_trigger_tbl[st])
    {
        return false; /* not yet far enough into the boss screen */
    }
    ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] = auto_scroll_timer_tbl[st];
    ram[CONTRA_RAM_LEVEL_STOP_SCROLL] = 0xFFu;
    return true;
}

/* set_vertical_level_frame_scroll (bank7:5213-5237): on a vertical level, when the
   player is jumping up (negative fast velocity) and high on the screen (y < #$50),
   scroll the screen up by the player's would-be upward movement instead of moving
   the player sprite. Mirrors player_jumping_set_y_pos' coefficient update but
   stores the negated Y delta to FRAME_SCROLL. Returns true when it scrolled, so
   the caller skips the player Y-position update (the `bcs` at bank7:4736). */
static bool contra_set_vertical_level_frame_scroll(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t y = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    uint8_t coeff;
    uint8_t carry;
    uint8_t new_y;

    if (y >= 0x50u)
    {
        return false;
    }
    if ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) == 0u)
    {
        return false; /* falling (or stationary): don't scroll */
    }
    if (contra_rom_set_boss_auto_scroll(core))
    {
        return false; /* boss-reveal auto-scroll takes over (bank7:5220-5221) */
    }

    coeff = (uint8_t)(ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] +
                      ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index]);
    carry = (coeff < ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index]) ? 1u : 0u;
    ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] = coeff;
    new_y = (uint8_t)((uint16_t)y +
                      (uint16_t)ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] +
                      (uint16_t)carry);

    ram[CONTRA_RAM_FRAME_SCROLL] = (uint8_t)(y - new_y);
    ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + player_index] = 0x01u;
    return true;
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

/* set_player_advancing_vel (bank7:4882): the walk-into-the-screen velocities.
   A speed code rotates 7 bits right into a fast:fract pair (i.e. code/128
   px/frame). Y always uses code 0x58 negated (-0x00B0, a slow upward drift);
   X uses the player's distance to the screen center |x - 0x80|, negated when
   starting right of center -- so both players converge on the door. A
   game-over player 2 at x=0 gets exactly +1 px/frame (code 0x80). */
static void contra_set_player_indoor_advancing_velocity(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t sprite_x = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
    const uint8_t dx = (uint8_t)(sprite_x - 0x80u);
    const uint8_t mag = (sprite_x >= 0x80u) ? dx : (uint8_t)(0u - dx);
    uint16_t v = (uint16_t)((uint16_t)mag << 1u);

    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FAST_VEL + player_index] = 0xFFu;
    ram[CONTRA_RAM_INDOOR_TRANSITION_Y_FRACT_VEL + player_index] = 0x50u;
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_Y + player_index] = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    ram[CONTRA_RAM_PLAYER_INDOOR_ANIM_X + player_index] = sprite_x;

    if (sprite_x >= 0x80u)
    {
        v = (uint16_t)(0u - v); /* right of center: walk left */
    }
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = (uint8_t)(v >> 8u);
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = (uint8_t)v;
}

/* The Up-press on a cleared indoor screen (bank7:4602 @indoor_screen_cleared):
   if the OTHER player is alive but not standing ready (state 1, not jumping),
   the press only sets the presser's standing sprite. Otherwise BOTH player
   slots get the walking sequence, and any slot not already advancing (a
   game-over player 2 included!) is armed with the advance flag + velocities.
   The walk restarts this way for each of the 4 background screens (the held
   Up re-triggers after every swap ends the segment). */
static void contra_start_indoor_room_advance(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t other = (uint8_t)(player_index ^ 1u);
    int x;

    if ((ram[CONTRA_RAM_P1_GAME_OVER_STATUS + other] == 0u) &&
        ((ram[CONTRA_RAM_PLAYER_STATE + other] != 0x01u) ||
         (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + other] != 0u)))
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x00u;
        return;
    }

    for (x = 1; x >= 0; --x)
    {
        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + x] = 0x05u;
        if (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + x] != 0u)
        {
            continue;
        }
        ram[CONTRA_RAM_INDOOR_SCROLL] = 0x01u;
        ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + x] = 0x01u;
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + x] = 0x00u;
        ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + x] = 0x00u;
        contra_set_player_indoor_advancing_velocity(core, (uint8_t)x);
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
    /* handle_d_pad (bank7:4561) shifts RIGHT, LEFT, DOWN out of the pad first;
       d_pad_up_pressed only runs when UP is the lone direction. UP+LEFT keeps
       walking and does NOT arm the room advance (frame 29017). */
    if ((ram[CONTRA_RAM_CONTROLLER_STATE + player_index] &
         (CONTRA_BUTTON_RIGHT | CONTRA_BUTTON_LEFT | CONTRA_BUTTON_DOWN)) != 0u)
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

    contra_start_indoor_room_advance(core, player_index);
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
    uint8_t *const ram = core->ram;

    /* player_land_on_ground (bank7:4847). When arriving from a jump or edge
       fall: reset the walk animation phase and the fall X freeze, and play the
       landing sound. PLAYER_SPECIAL_SPRITE_TIMER is NOT reset -- the somersault
       spin phase carries across landings into the next jump. */
    if ((uint8_t)(ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] |
                  ram[CONTRA_RAM_EDGE_FALL_CODE + player_index]) != 0u)
    {
        ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;
        ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
        ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + player_index] = 0x00u;
        contra_play_sound(core, 0x03u); /* sound_03: landing */
    }

    /* @check_aim_dir: re-derive the horizontal flip from the aim direction */
    {
        uint8_t flip = (uint8_t)(ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] & 0x3Fu);

        if (ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u)
        {
            flip |= 0x40u;
        }
        ram[CONTRA_RAM_PLAYER_SPRITE_FLIP + player_index] = flip;
    }

    contra_init_player_data(core, player_index);
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

/* set_frame_scroll_if_appropriate (bank7:5141-5266), per player, called from
   inside calc_player_x_vel between the d-pad velocity and the move: when the
   player is at/past the scroll point, CONSUME the X velocity into FRAME_SCROLL
   (so the move that follows applies zero and the player stays put). */
static void contra_set_frame_scroll_if_appropriate(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    static const uint8_t horizontal_scroll_point_tbl[3] = {0x80u, 0x80u, 0xB0u};
    unsigned accum;

    if (((uint8_t)(ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] |
                   ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] |
                   ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01]) != 0u) ||
        (ram[CONTRA_RAM_PLAYER_STATE + player_index] != 0x01u))
    {
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        /* set_vertical_level_frame_scroll (bank7:5236): while jumping up past
           Y 0x50, scroll the screen by the jump velocity instead of moving the
           player. Reached from the horizontal move on vertical levels too --
           the ROM's double-application "platform skip" quirk is faithful. */
        unsigned coeff;
        uint8_t new_y;

        if ((ram[CONTRA_RAM_SPRITE_Y_POS + player_index] >= 0x50u) ||
            ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) == 0u))
        {
            return;
        }
        if (contra_rom_set_boss_auto_scroll(core))
        {
            return;
        }
        coeff = (unsigned)ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] +
            ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + player_index];
        ram[CONTRA_RAM_PLAYER_JUMP_COEFFICIENT + player_index] = (uint8_t)coeff;
        new_y = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] +
                          ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] +
                          (uint8_t)(coeff >> 8u));
        ram[CONTRA_RAM_FRAME_SCROLL] =
            (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - new_y);
        ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + player_index] = 0x01u;
        return;
    }

    if ((ram[CONTRA_RAM_LEVEL_STOP_SCROLL] & 0x80u) != 0u)
    {
        return; /* boss auto-scroll has latched */
    }
    if (ram[CONTRA_RAM_SPRITE_X_POS + player_index] <
        horizontal_scroll_point_tbl[ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD] % 3u])
    {
        return;
    }
    if (ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD] == 0x02u)
    {
        /* both players active: the other player at the left edge blocks the
           scroll AND stops the scroller (@stop_player_x_velocity) */
        if (ram[CONTRA_RAM_SPRITE_X_POS + (player_index ^ 1u)] < 0x21u)
        {
            ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x00u;
            ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
            return;
        }
    }
    if (contra_rom_set_boss_auto_scroll(core))
    {
        return;
    }

    /* @set_horizontal_level_frame_scroll: consume the velocity into the scroll */
    accum = (unsigned)ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] +
        ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index];
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] = (uint8_t)accum;
    ram[CONTRA_RAM_FRAME_SCROLL] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] + (accum >> 8u));
    ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + player_index] = 0x01u;
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
}

/* calc_player_x_vel (bank7:4357-4438): apply the X velocity. The scroll check
   runs INSIDE the rightward branch (after the solid/right-edge/boss-max gates)
   and may consume the velocity, in which case the add below applies zero.
   Once the level boss is defeated (BOSS_DEFEATED_FLAG bit 7, the scripted
   end-of-level walk) the geometry gates are skipped so the player can walk
   past the boss-screen solids. */
static void contra_move_player_horizontally(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    static const uint8_t lvl_boss_max_x_scroll_tbl[8] = {
        0x90u, 0xFFu, 0xFFu, 0xFFu, 0xA0u, 0xD0u, 0xB0u, 0xB0u};
    const uint8_t screen_type = contra_get_level_screen_type(core);
    const uint8_t right_edge = (screen_type == 0u) ? 0xE6u : ((screen_type == 1u) ? 0xE0u : 0xD0u);
    const uint8_t left_edge = (screen_type == 0u) ? 0x1Au : ((screen_type == 1u) ? 0x20u : 0x30u);
    const bool boss_defeated_walk = (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] & 0x80u) != 0u;
    uint8_t velocity;
    unsigned accum;

    /* fold in the velocity boost from riding a moving platform */
    ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] =
        (uint8_t)(ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] +
                  ram[CONTRA_RAM_PLAYER_FAST_X_VEL_BOOST + player_index]);
    velocity = ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index];

    if ((uint8_t)(velocity | ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index]) == 0u)
    {
        return;
    }

    if ((velocity & 0x80u) == 0u)
    {
        if (!boss_defeated_walk)
        {
            if (contra_player_has_solid_collision_ahead(core, player_index, 8) ||
                (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >= right_edge))
            {
                return;
            }
            if ((ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] != 0u) &&
                (ram[CONTRA_RAM_SPRITE_X_POS + player_index] >=
                 lvl_boss_max_x_scroll_tbl[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u]))
            {
                return;
            }
        }
        contra_set_frame_scroll_if_appropriate(core, player_index);
        velocity = ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index];
    }
    else
    {
        if (contra_player_has_solid_collision_ahead(core, player_index, -8))
        {
            return;
        }
        if (!boss_defeated_walk &&
            (ram[CONTRA_RAM_SPRITE_X_POS + player_index] < left_edge))
        {
            return;
        }
    }

    /* @apply_vel_to_player_x_pos */
    accum = (unsigned)ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] +
        ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index];
    ram[CONTRA_RAM_INDOOR_TRANSITION_X_ACCUM + player_index] = (uint8_t)accum;
    ram[CONTRA_RAM_SPRITE_X_POS + player_index] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] + velocity + (accum >> 8u));
}

/* auto_scroll_player (bank7:4279-4296), the tail of player_state_routine_01
   and the head of player_state_routine_02: while a boss-reveal auto-scroll
   timer runs, push the player back against the scroll every frame -- 1px left
   on horizontal levels (stopping at the outdoor left edge 0x1A), 1px down on
   the vertical level. */
static void contra_auto_scroll_player(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    if ((uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] |
                  ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01]) == 0u)
    {
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
            (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] + 1u);
        return;
    }

    if (ram[CONTRA_RAM_SPRITE_X_POS + player_index] < 0x1Au)
    {
        return;
    }

    ram[CONTRA_RAM_SPRITE_X_POS + player_index] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] - 1u);
}

static void contra_handle_player_fall_out(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const bool demo_mode = ram[CONTRA_RAM_DEMO_MODE] != 0u;
    const bool demo_life_floor_reached =
        demo_mode && (ram[CONTRA_RAM_P1_NUM_LIVES + player_index] <= 0x61u);

    /* init_player_and_weapon (bank7:5314): reset current weapon to the default
       rifle on death, then fall through to init_player_attributes. */
    ram[CONTRA_RAM_P1_CURRENT_WEAPON + player_index] = 0x00u;
    if ((player_index == 0u) && contra_env_flag_enabled("CONTRA_KEEP_START_WEAPON"))
    {
        uint8_t start_weapon;

        if (contra_parse_start_weapon(getenv("CONTRA_START_WEAPON"), &start_weapon))
        {
            ram[CONTRA_RAM_P1_CURRENT_WEAPON] = start_weapon;
        }
    }

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
    uint8_t *const ram = core->ram;
    uint8_t table_row;
    uint8_t aim_context;
    uint8_t dpad = (uint8_t)(ram[CONTRA_RAM_CONTROLLER_STATE + player_index] & 0x0Fu);

    /* bank7:4965-4990 set_player_aim_for_input
     * Default to the jump-aim rows ($20 standing / $30 jumping). These rows are
     * only actually used on the indoor (base) boss screen; for every other
     * screen the row is overridden below with the facing-direction rows
     * ($00/$10) regardless of jump status. */
    table_row = (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] != 0u) ? 0x30u : 0x20u;

    /* bank7:4110-4119 run_player_state_routine computes $08: it is #$03 on the
     * indoor (base) boss screen (LEVEL_LOCATION_TYPE bit 7 set), otherwise the
     * level_spawn_position_index entry (values 0/1/2 only, never 3). Only the
     * indoor-boss case keeps the jump-aim row. */
    aim_context = ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u)
        ? 0x03u
        : contra_level_spawn_position_index[ram[CONTRA_RAM_CURRENT_LEVEL] & 0x07u];

    if (aim_context != 0x03u)
    {
        table_row = (ram[CONTRA_RAM_PLAYER_AIM_PREV_FRAME + player_index] >= 0x05u)
            ? 0x10u
            : 0x00u;
    }

    ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] = contra_d_pad_player_aim_tbl[table_row + dpad];
}

static void contra_set_jump_status_and_y_velocity(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] =
        (ram[CONTRA_RAM_PLAYER_AIM_DIR + player_index] >= 0x05u) ? 0x91u : 0x11u;
    ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_ANIMATION_FRAME_INDEX + player_index] = 0x00u;

    /* the ROM tests only BIT 0 of the location type (`lsr`, bank7:4954): the
       indoor BOSS room (0x80) jumps with the outdoor -5.94, not the indoor
       -4.56 -- that's the jump into the boss room from the last corridor. */
    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x01u) == 0u)
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

    /* @apply_gravity (bank7:4730-4740): gravity always runs; on a vertical level a
       climbing player scrolls the screen up instead of moving the sprite up. When
       the scroll is taken, player_jumping_set_y_pos is skipped (the `bcs` branch). */
    contra_apply_gravity(core, player_index);
    if ((ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u) ||
        !contra_set_vertical_level_frame_scroll(core, player_index))
    {
        contra_player_jumping_set_y_pos(core, player_index);
    }

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

/* player_state_routine_00's landing test (bank7:4205 @check_if_floor_exit +
   check_collision_below bank7:5296): a spawn column is landable when there is
   no solid at the spawn row, no solid at y+0x20, and SOME collision (floor,
   water or solid) in the 16px rows below it down to row 0x0E. */
static bool contra_rom_spawn_spot_landable(ContraCore *core, uint8_t player_index)
{
    uint8_t *const ram = core->ram;
    const uint8_t x = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
    const uint8_t y = ram[CONTRA_RAM_SPRITE_Y_POS + player_index];
    uint8_t code = contra_get_player_bg_collision_code(core, player_index);
    uint8_t row;

    if ((code & 0x80u) != 0u)
    {
        return false; /* solid object at the spawn row */
    }
    code = contra_rom_get_bg_collision_far(core, x, (uint8_t)(y + 0x20u));
    if ((code & 0x80u) != 0u)
    {
        return false;
    }
    for (row = (uint8_t)(((uint8_t)(y + 0x20u) >> 4u) + 1u); row < 0x0Eu; ++row)
    {
        if (contra_get_outdoor_bg_collision(core, x, (uint8_t)(row << 4u)) != 0u)
        {
            return true; /* something to land on below */
        }
    }
    return false;
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

            /* player_state_routine_00 (bank7:4166-4188), outdoor only: if the
               table spawn column has nothing to land on, restart at x=0x10 and
               walk right in 0x10 steps; past 0xE0 fall back to x=0x30. */
            if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
                !contra_rom_spawn_spot_landable(core, player_index))
            {
                ram[CONTRA_RAM_SPRITE_X_POS + player_index] = 0x10u;
                for (;;)
                {
                    uint8_t sx;

                    if (contra_rom_spawn_spot_landable(core, player_index))
                    {
                        break;
                    }
                    sx = (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] + 0x10u);
                    ram[CONTRA_RAM_SPRITE_X_POS + player_index] = sx;
                    if (sx >= 0xE0u)
                    {
                        ram[CONTRA_RAM_SPRITE_X_POS + player_index] = 0x30u;
                        break;
                    }
                }
            }
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

            /* handle_player_state_calc_x_vel (bank7:4359): the per-frame X
               velocity reset is STATE-1 ONLY -- a dead player's corpse keeps
               its last velocity byte (the ROM never clears it). */
            if (ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG + player_index] == 0u)
            {
                ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
                ram[CONTRA_RAM_INDOOR_TRANSITION_X_FRACT_VEL + player_index] = 0x00u;
            }

            /* handle_player_state (bank7:4455) defaults the sprite sequence to
               3 (walking/curled-jump) every frame; the d-pad branches overwrite
               it for standing/aiming/crouching, so it survives exactly while
               jumping or falling -- including the spawn drop-in. */
            ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x03u;

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
                const bool jump_pressed =
                    (ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] == 0u) &&
                    ((ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_index] & CONTRA_BUTTON_A) != 0u);

                if (jump_pressed)
                {
                    const uint8_t directional =
                        (uint8_t)(ram[CONTRA_RAM_CONTROLLER_STATE + player_index] &
                                  (CONTRA_BUTTON_DOWN | CONTRA_BUTTON_LEFT | CONTRA_BUTTON_RIGHT));

                    if (directional == CONTRA_BUTTON_DOWN)
                    {
                        ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + player_index] = 0x02u;
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
                else if (!contra_handle_indoor_player_up_input(core, player_index))
                {
                    contra_handle_d_pad(core, player_index);
                }
            }

            if ((ram[CONTRA_RAM_PLAYER_WATER_STATE + player_index] & 0x80u) != 0u)
            {
                ram[CONTRA_RAM_PLAYER_X_VELOCITY + player_index] = 0x00u;
            }

            contra_move_player_horizontally(core, player_index);
            contra_auto_scroll_player(core, player_index);

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

            contra_auto_scroll_player(core, player_index);
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
    const uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    return ((game_routine == 0x05u) ||
            ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        ((level == 0x01u) || (level == 0x03u)) &&
        ((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) ||
         (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u));
}

static bool contra_is_native_level_7_active(const ContraCore *core)
{
    const uint8_t game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];

    return ((game_routine == 0x05u) ||
            ((game_routine == 0x02u) && (core->ram[CONTRA_RAM_DEMO_MODE] != 0u))) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u) &&
        (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u);
}

static bool contra_is_native_combat_active(const ContraCore *core)
{
    return contra_is_native_level_1_active(core) ||
        contra_is_native_level_2_active(core) ||
        contra_is_native_level_7_active(core);
}

/* Find the screen Y of the top of the first solid floor tile at or below
   start_y, scanning at the 8px collision-tile resolution (the coarse finder
   above steps 16px). Returns false if no solid ground is found. */
/* Seat a grounded enemy at the same height the player would settle to on the
   floor beneath it. The enemy and player share the +0x10 foot anchor, so
   running the detected floor surface through the player's landing snap keeps
   ground enemies aligned with the player instead of sitting below the floor. */
static void contra_load_bank_2_set_players_paused_sprite_attr(ContraCore *core)
{
    contra_set_player_sprite_and_attrs(core, 0u);
    contra_set_player_sprite_and_attrs(core, 1u);
}

static void contra_scroll_vertical_non_scrolling_player(ContraCore *core, uint8_t active_players)
{
    uint8_t *const ram = core->ram;
    size_t scrolled_player;

    if ((active_players != 0x03u) ||
        (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u) ||
        (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] != 0u) ||
        (ram[CONTRA_RAM_FRAME_SCROLL] == 0u) ||
        (ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 0u] == ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u]))
    {
        return;
    }

    scrolled_player = (ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 0u] < ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u]) ? 0u : 1u;
    ram[CONTRA_RAM_SPRITE_Y_POS + scrolled_player] =
        (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + scrolled_player] + ram[CONTRA_RAM_FRAME_SCROLL]);
    if (ram[CONTRA_RAM_SPRITE_Y_POS + scrolled_player] < ram[CONTRA_RAM_FRAME_SCROLL])
    {
        ram[CONTRA_RAM_PLAYER_HIDDEN + scrolled_player] =
            (uint8_t)(ram[CONTRA_RAM_PLAYER_HIDDEN + scrolled_player] + 1u);
    }
}

/* animate_indoor_fence (bank7:2917): the electric fence isn't in the nametable --
   the ROM rebuilds the 4 CHR pattern tiles at PPU $1FC0 (pattern-table-1 tiles
   0xFC-0xFF, which the indoor room super-tiles reference) every animation frame by
   OR-ing a fixed tile "background" shape with a frame-cycled "electricity" overlay.
   The native renderer composes the background straight from ppu_pattern, so writing
   the animated CHR there is what makes the fence appear and flicker. Once the screen
   is cleared the blank overlay is written and the fence vanishes. */
static const uint8_t contra_pattern_tile_bg_00[0x40] = {
    0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu, 0xFFu,
    0xFEu, 0xFCu, 0xF8u, 0xF0u, 0xE0u, 0xC0u, 0x80u, 0x00u, 0x00u, 0x01u, 0x03u, 0x07u, 0x0Fu, 0x1Fu, 0x3Fu, 0x7Fu,
    0x7Fu, 0x3Fu, 0x1Fu, 0x0Fu, 0x07u, 0x03u, 0x01u, 0x00u, 0x00u, 0x80u, 0xC0u, 0xE0u, 0xF0u, 0xF8u, 0xFCu, 0xFEu};
static const uint8_t contra_pattern_tile_fence_tbl[0x28] = {
    0x00u, 0x00u, 0x04u, 0x44u, 0xEBu, 0x32u, 0x20u, 0x00u,
    0x00u, 0x00u, 0x10u, 0x30u, 0xEBu, 0x6Au, 0x44u, 0x00u,
    0x00u, 0x00u, 0x08u, 0x0Cu, 0xD7u, 0x56u, 0x22u, 0x00u,
    0x00u, 0x00u, 0x20u, 0x22u, 0xD7u, 0x4Cu, 0x04u, 0x00u,
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u}; /* blank: no fence */

static void contra_animate_indoor_fence(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t base;
    unsigned y;

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0x01u)
    {
        return; /* indoor boss screen (0x80) / non-indoor: no fence */
    }

    if (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0u)
    {
        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u) != 0u)
        {
            return; /* electricity is redrawn only every 4th frame */
        }
        base = (uint8_t)((ram[CONTRA_RAM_FRAME_COUNTER] & 0x0Cu) << 1u); /* 0x00/0x08/0x10/0x18 */
    }
    else if ((ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] & 0x80u) == 0u)
    {
        base = 0x20u; /* screen cleared: blank overlay removes the fence (once) */
        ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x80u;
    }
    else
    {
        return; /* fence already removed */
    }

    /* the fence CHR rewrite occupies 0x45 bytes of CPU_GRAPHICS_BUFFER
       (header 5 + 0x40 pattern bytes) -- on these frames a full enemy
       super-tile stamp finds the buffer past its 0x40 entry check and must
       retry next frame (bank7:1353). */
    ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] =
        (uint8_t)(ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] + 0x45u);

    for (y = 0u; y < 0x40u; ++y)
    {
        core->ppu_pattern[0x1FC0u + y] =
            (uint8_t)(contra_pattern_tile_bg_00[y] |
                      contra_pattern_tile_fence_tbl[base + (y & 0x07u)]);
    }
}

static void contra_load_bank_3_handle_scroll(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    uint8_t scroll_pixels = (uint8_t)(ram[CONTRA_RAM_FRAME_SCROLL] + ram[CONTRA_RAM_TANK_AUTO_SCROLL]);

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) &&
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u))
    {
        contra_animate_indoor_fence(core); /* runs every frame, regardless of scroll */

        if ((ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] & 0x80u) != 0u)
        {
            /* @indoor_screen_transition (bank7:5845): the 0x20-frame background
               segment finished last frame -- swap to the next of the 4 corridor
               screens. The walking player sees INDOOR_SCROLL=2 this frame and
               ends the segment (restoring position); the still-held Up press
               re-arms the next segment. */
        }
        else if (ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] != 0u)
        {
            /* @write_column_tiles_exit: stream one nametable column per frame */
            ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] =
                (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] + 1u);
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] =
                (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] + 1u);
            ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] - 1u);
            if (ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] == 0u)
            {
                ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x80u; /* swap next frame */
            }
            return;
        }
        else
        {
            if (ram[CONTRA_RAM_INDOOR_SCROLL] == 0u)
            {
                return;
            }
            /* segment init (bank7:5786): reset the streaming counters, flip the
               write nametable, load the NEXT corridor screen's super-tiles
               (screen*4 + offset + 1, note the SEC), then stream the FIRST
               column the same frame. */
            ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
            ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_LOW_BYTE] = 0x00u;
            ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x20u;
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] ^= 0x04u;
            ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] ^= 0x04u;
            contra_load_supertiles_screen_indexes(
                core,
                (uint8_t)((ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] * 4u) +
                          ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u)
            );
            ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x01u;
            ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x01u;
            ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x1Fu;
            return;
        }

        ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER] = 0x00u;
        ram[CONTRA_RAM_INDOOR_SCROLL] =
            (uint8_t)(ram[CONTRA_RAM_INDOOR_SCROLL] + 1u); /* -> 2: ends the walk segment */
        ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] =
            (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u);
        if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] >= 0x04u)
        {
            /* all 4 corridor screens shown: jump both players into the room */
            ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + 0u] =
                (uint8_t)(ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + 0u] + 1u);
            ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + 1u] =
                (uint8_t)(ram[CONTRA_RAM_INDOOR_PLAYER_JUMP_FLAG + 1u] + 1u);
            ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x00u;
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
                /* load the boss-room CHR set: lda CURRENT_LEVEL; lsr; ora #$08;
                   jsr load_A_offset_graphic_data (bank7:5845-5848) -- list 8 for L2
                   (level_2_boss_graphic_data $03,$04,$13,$08), list 9 for L4. Without
                   this the cannon/plating/wall tiles render from the wrong CHR. */
                contra_load_graphic_data_list(
                    core, (uint8_t)(0x08u | (ram[CONTRA_RAM_CURRENT_LEVEL] >> 1u)));
                /* Compose the flat mechanical boss wall: repoint the super-tile /
                   tile-data / palette pointers to the boss tables (handle_indoor_scroll
                   pointer swap, bank7:5772-5787, source level_2_4_boss_graphics_data
                   bank7:5870) and reload the boss-wall layout. Screen index 0 = L2 wall,
                   1 = L4 wall (CURRENT_LEVEL>>1). Without this the boss room composes the
                   generic corridor layout (now drawn with boss CHR -> garbled). */
                ram[CONTRA_RAM_LEVEL_SCREEN_SUPERTILES_PTR] = 0x13u; /* $9013 boss screen ptr tbl */
                ram[CONTRA_RAM_LEVEL_SCREEN_SUPERTILES_PTR + 1u] = 0x90u;
                ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] = 0x7Au; /* $b57a boss super-tile data */
                ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] = 0xB5u;
                ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] = 0x7Au; /* $bd7a boss palette data */
                ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] = 0xBDu;
                contra_decode_level_screen_supertiles(
                    core, (uint8_t)(ram[CONTRA_RAM_CURRENT_LEVEL] >> 1u),
                    core->level_screen_supertiles, 0u);
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

    /* handle_vertical_scroll (bank7:5667-5747): advance the vertical-level camera.
       The original streams nametable rows into the PPU as it scrolls; the port's
       framebuffer renderer redraws from LEVEL_SCREEN_NUMBER / SCROLL_OFFSET each
       frame, so only the scroll bookkeeping is ported here (the PPU write-address
       and supertile-streaming steps are unnecessary). */
    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        uint8_t vertical_scroll_pixels;

        /* boss-reveal auto-scroll (bank7:5668-5676) */
        if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] != 0u)
        {
            ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x10u;
            ram[CONTRA_RAM_FRAME_SCROLL] = 0x01u;
            ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] =
                (uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] - 1u);
            if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] == 0u)
            {
                ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] =
                    (uint8_t)(ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] + 1u);
            }
        }

        vertical_scroll_pixels = ram[CONTRA_RAM_FRAME_SCROLL]; /* @init_loop, bank7:5678-5681 */
        while (vertical_scroll_pixels-- != 0u)                 /* @frame_scroll_loop */
        {
            ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + 1u);
            if (ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] >= 0xF0u) /* bank7:5685-5699 */
            {
                ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
                ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
                ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 1u);
                if (ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == ram[CONTRA_RAM_LEVEL_ALT_GRAPHICS_POS])
                {
                    ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x01u;
                    ram[CONTRA_RAM_BG_PALETTE_ADJ_TIMER] = 0x80u;
                    contra_load_alternate_graphics(core);
                }
            }

            if ((ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] & 0x07u) == 0u)
            {
                const uint8_t old_low = ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE];

                ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] =
                    (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] - 0x20u);
                if (old_low < 0x20u)
                {
                    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] =
                        (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] - 1u);
                }

                ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] =
                    (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] - 1u);
                if ((ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] & 0x80u) != 0u)
                {
                    ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] ^= 0x40u;
                    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x1Du;
                    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0xA0u;
                    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] = 0x23u;
                    contra_load_next_supertiles_screen_indexes(core);
                }
            }

            /* @dec_scroll_continue (bank7:5726-5732): VERTICAL_SCROLL counts down
               one PPU line per scrolled pixel, wrapping #$ff back to #$ef. */
            ram[CONTRA_RAM_VERTICAL_SCROLL] =
                (uint8_t)(ram[CONTRA_RAM_VERTICAL_SCROLL] - 1u);
            if (ram[CONTRA_RAM_VERTICAL_SCROLL] == 0xFFu)
            {
                ram[CONTRA_RAM_VERTICAL_SCROLL] = 0xEFu;
            }
        }
        return;
    }

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        return;
    }

    /* @handle_outdoor_level (bank7:5575-5598): advance the boss-reveal auto-scroll.
       Tick AUTO_SCROLL_TIMER_01 then _00; while either is running force a 1px frame
       scroll, and when _00 elapses set BOSS_AUTO_SCROLL_COMPLETE so the boss
       routines (which gate on it) can start. Then recompute the scroll amount. */
    {
        bool force_frame_scroll = false;

        if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] != 0u)
        {
            ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] =
                (uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] - 1u);
            if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01] != 0u)
            {
                force_frame_scroll = true;
            }
        }
        if (!force_frame_scroll && (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] != 0u))
        {
            ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] =
                (uint8_t)(ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] - 1u);
            if (ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] == 0u)
            {
                ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] =
                    (uint8_t)(ram[CONTRA_RAM_BOSS_AUTO_SCROLL_COMPLETE] + 1u);
            }
            force_frame_scroll = true;
        }
        if (force_frame_scroll)
        {
            ram[CONTRA_RAM_FRAME_SCROLL] = 0x01u;
        }
        scroll_pixels = (uint8_t)(ram[CONTRA_RAM_FRAME_SCROLL] + ram[CONTRA_RAM_TANK_AUTO_SCROLL]);
    }

    if (scroll_pixels == 0u)
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

        /* load_palette_colors_to_cpu (bank7:3520) leaves the game_palettes
           pointer low byte in the $06 zero-page temp each slot. The leftover
           from the LAST slot is consumed as junk by create_default_soldiers'
           ledge-handling bit on the next frame, so the temp must be mirrored. */
        core->ram[0x06u] = (uint8_t)((uint8_t)(palette_index * 3u) + 0x27u);

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
    core->level1_weapon_box_restore_timer = 0x00u;
    core->level1_weapon_box_restore_x = 0;
    core->level1_weapon_box_restore_y = 0;
    core->pending_horizontal_column_write = 0x00u;
    core->pending_horizontal_attr_write = 0x00u;
    core->l7_tile_update_count = 0x00u;
    core->l7_supertile_update_count = 0x00u;
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

    /* TEST HOOK (not part of the faithful port): CONTRA_FORCE_DEMO_LEVEL=<index>
       pins every attract demo to one level (0-based), so the chosen level's demo
       runs FIRST, clean from boot -- isolating it from upstream demo-level desync
       for the frame-exact comparison harness. */
    {
        const char *const force = getenv("CONTRA_FORCE_DEMO_LEVEL");

        if (force != NULL)
        {
            const int level = atoi(force);

            if ((level >= 0) && (level <= 2))
            {
                core->ram[CONTRA_RAM_CURRENT_LEVEL] = (uint8_t)level;
                core->ram[CONTRA_RAM_DEMO_LEVEL] = (uint8_t)level;
            }
        }
    }
}

static void contra_apply_start_player_env(ContraCore *core)
{
    const char *const lives_env = getenv("CONTRA_START_LIVES");
    const char *const weapon_env = getenv("CONTRA_START_WEAPON");
    uint8_t weapon;

    if (lives_env != NULL)
    {
        char *end = NULL;
        const long total_lives = strtol(lives_env, &end, 10);

        if ((end != lives_env) && (end != NULL) && (*end == '\0') &&
            (total_lives >= 1) && (total_lives <= 256))
        {
            /* P1_NUM_LIVES stores lives remaining after the current life.
               CONTRA_START_LIVES is user-facing total lives, so 30 -> 0x1d. */
            core->ram[CONTRA_RAM_P1_NUM_LIVES] = (uint8_t)(total_lives - 1);
        }
    }

    if (contra_parse_start_weapon(weapon_env, &weapon))
    {
        core->ram[CONTRA_RAM_P1_CURRENT_WEAPON] = weapon;
    }
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

    /* TEST HOOK (not part of the faithful port): CONTRA_START_LEVEL=<index> starts
       the game on a chosen level. Index is 0-based, so CONTRA_START_LEVEL=2 begins
       directly on Level 3. Applied once, here at game start, after clear_memory_3
       has zeroed CURRENT_LEVEL; level_routine_00 then loads that level's header. */
    {
        const char *const start_level = getenv("CONTRA_START_LEVEL");

        if (start_level != NULL)
        {
            const int level = atoi(start_level);

            if ((level >= 0) && (level <= 7))
            {
                core->ram[CONTRA_RAM_CURRENT_LEVEL] = (uint8_t)level;
            }
        }
    }
    contra_apply_start_player_env(core);
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

    {
        /* bank7.asm game_routine_03: `dec DELAY_TIME_LOW_BYTE` decrements memory
           while A keeps the PRE-decrement value, and the `ora INTRO_THEME_DELAY`
           elapsed test runs on that stale A -- the ROM advances on the frame
           AFTER the timer reaches 0, not the frame it reaches 0. */
        const uint8_t delay_before = ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE];

        if (delay_before != 0u)
        {
            ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = (uint8_t)(delay_before - 1u);
        }

        if ((uint8_t)(delay_before | ram[CONTRA_RAM_INTRO_THEME_DELAY]) == 0u)
        {
            contra_increment_game_routine(core);
            return;
        }
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
    if (core->level_transition_pending != 0u)
    {
        /* mid-game level transition only (the boot path is already timed by
           startup_wait_frames): the ROM spends one extra video frame flushing
           this routine's PPU writes (reference frame 7449). */
        core->level_transition_pending = 0u;
        core->frame_stall_frames = 0x01u;
    }
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
    /* the ROM's level graphics load spans more video frames for the indoor
       base levels (reference recording: level 1 = 8 frozen frames at boot,
       level 2 = 10 at frames 7643-7652) */
    core->level_graphics_wait_frames =
        (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ? 0x0Au : 0x08u;
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
        contra_scroll_vertical_non_scrolling_player(core, active_players);
    }

    if (core->ram[CONTRA_RAM_INDOOR_SCROLL] >= 0x02u)
    {
        core->ram[CONTRA_RAM_INDOOR_SCROLL] = 0x00u;
    }

    /* bank7:3879-3884: the auto-scroll FRAME_SCROLL=1 is set AFTER the player
       routines (a latch this frame counts) and BEFORE the bullet routines, so
       in-flight bullets ride the scroll on the latch frame itself. */
    if ((uint8_t)(core->ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] | core->ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01]) != 0u)
    {
        core->ram[CONTRA_RAM_FRAME_SCROLL] = 0x01u;
    }

    contra_update_player_bullets(core);
    contra_draw_player_bullet_sprites(core);
}

/* ---------------------------------------------------------------------------
   Faithful real-RAM enemy system.

   A direct port of the ROM's enemy spawn/dispatch onto the real ENEMY_* RAM
   arrays, so the frame-exact harness can validate enemy state against the ROM.
   This is the only enemy system: spawn -> dispatch -> per-type routines ->
   render -> collision, all on real CPU RAM.
   --------------------------------------------------------------------------- */

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
    {0x8Fu, 0x02u, 0x01u, 0x00u}, /* 0x1C boss gemini */
    {0x0Au, 0x15u, 0x01u, 0x00u}, /* 0x1D boss gemini spinning bubbles */
    {0x03u, 0x30u, 0x01u, 0x00u}, /* 0x1E blue jumping guy */
    {0x03u, 0x30u, 0x01u, 0x00u}, /* 0x1F red shooting guy */
    {0x81u, 0x00u, 0xF1u, 0x00u}, /* 0x20 red/blue guys generator */
};

/* enemy_prop level-3 entries (bank7:9221), indexed by type-0x10. Note the floating
   rock platform's STATE_WIDTH #$c0 -- bit 6 set marks it "landable", which is what
   lets the player ride it instead of dying. */
static const uint8_t contra_enemy_prop_level3[6][4] = {
    {0xC0u, 0x04u, 0xF0u, 0x00u}, /* 0x10 floating rock platform */
    {0x80u, 0x02u, 0xF0u, 0x00u}, /* 0x11 moving flame */
    {0x81u, 0x00u, 0xF0u, 0x00u}, /* 0x12 rock cave (falling-rock generator) */
    {0x8Fu, 0x31u, 0x05u, 0x00u}, /* 0x13 falling rock */
    {0x8Du, 0x83u, 0xF1u, 0x02u}, /* 0x14 boss mouth */
    {0x0Eu, 0x52u, 0xF1u, 0x00u}, /* 0x15 dragon arm orb */
};

/* enemy_prop level-5 entries (bank7:9230), indexed by type-0x10. */
static const uint8_t contra_enemy_prop_level5[7][4] = {
    {0x81u, 0x00u, 0xF0u, 0x00u}, /* 0x10 ice grenade generator */
    {0x81u, 0x02u, 0xF1u, 0x00u}, /* 0x11 ice grenade */
    {0x85u, 0x79u, 0xF0u, 0x00u}, /* 0x12 tank */
    {0x81u, 0x00u, 0xF0u, 0x00u}, /* 0x13 pipe joint */
    {0x8Du, 0x93u, 0x20u, 0x00u}, /* 0x14 alien carrier */
    {0x02u, 0x20u, 0x01u, 0x00u}, /* 0x15 flying saucer */
    {0x0Au, 0x12u, 0x01u, 0x00u}, /* 0x16 drop bomb */
};

/* enemy_prop level-6 entries (bank7:9241), indexed by type-0x10. */
static const uint8_t contra_enemy_prop_level6[5][4] = {
    {0x81u, 0x0Fu, 0xF0u, 0x00u}, /* 0x10 fire beam down */
    {0x81u, 0x0Fu, 0xF0u, 0x00u}, /* 0x11 fire beam left */
    {0x81u, 0x0Fu, 0xF0u, 0x00u}, /* 0x12 fire beam right */
    {0x04u, 0x9Du, 0x01u, 0x02u}, /* 0x13 boss robot */
    {0x80u, 0x05u, 0x01u, 0x00u}, /* 0x14 spiked disk projectile */
};

/* enemy_prop level-7 entries (bank7:9250), indexed by type-0x10. */
static const uint8_t contra_enemy_prop_level7[9][4] = {
    {0x80u, 0x0Au, 0xF0u, 0x00u}, /* 0x10 mechanical claw */
    {0x8Du, 0x0Fu, 0x10u, 0x00u}, /* 0x11 rising spiked wall */
    {0x0Cu, 0x0Fu, 0x10u, 0x00u}, /* 0x12 spiked wall */
    {0x81u, 0x00u, 0xF0u, 0x00u}, /* 0x13 cart generator */
    {0x6Eu, 0x0Cu, 0x03u, 0x00u}, /* 0x14 moving cart */
    {0x6Eu, 0x0Cu, 0x03u, 0x00u}, /* 0x15 immobile cart */
    {0x0Cu, 0x93u, 0x20u, 0x00u}, /* 0x16 armored door */
    {0x8Fu, 0x72u, 0x08u, 0x00u}, /* 0x17 mortar launcher */
    {0x89u, 0x00u, 0x01u, 0x00u}, /* 0x18 boss soldier generator */
};

/* enemy_prop level-8 entries (bank7:9271), indexed by type-0x10. */
static const uint8_t contra_enemy_prop_level8[7][4] = {
    {0x04u, 0x78u, 0x01u, 0x02u}, /* 0x10 alien guardian */
    {0x06u, 0x22u, 0x01u, 0x01u}, /* 0x11 alien fetus */
    {0x06u, 0x42u, 0x01u, 0x01u}, /* 0x12 alien mouth */
    {0x02u, 0x22u, 0x01u, 0x00u}, /* 0x13 white blob */
    {0x06u, 0x33u, 0x01u, 0x01u}, /* 0x14 alien spider */
    {0x06u, 0x62u, 0x10u, 0x01u}, /* 0x15 spider spawn */
    {0x04u, 0xA7u, 0x01u, 0x03u}, /* 0x16 heart */
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
    core->l1_supertile[x] = 0xFFu;   /* no L1 enemy super-tile drawn yet */
    contra_rom_clear_enemy_pt_2(core, x);

    if ((type == 0x12u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u))
    {
        core->l1_bridge_gap_count = 0u; /* a fresh bridge -> drop stale collision gaps */
    }

    /* enemy_prop_ptr_tbl (bank7:9152): shared types (< 0x10) use the common
       table; level-specific types (>= 0x10) use the per-level table. */
    if ((type >= 0x10u) &&
        ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) || (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u)))
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
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level3) / sizeof(contra_enemy_prop_level3[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level3[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level3[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level3[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level3[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level5) / sizeof(contra_enemy_prop_level5[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level5[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level5[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level5[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level5[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level6) / sizeof(contra_enemy_prop_level6[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level6[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level6[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level6[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level6[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level7) / sizeof(contra_enemy_prop_level7[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level7[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level7[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level7[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level7[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level8) / sizeof(contra_enemy_prop_level8[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level8[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level8[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level8[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level8[i][3];
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
/* level_enemy_screen_ptr_ptr_tbl (bank2:1688-1696): per-level CPU address (bank 2)
   of that level's per-screen enemy-data pointer table. Addresses are taken from the
   table's own annotations in bank2.asm. */
static const uint16_t contra_level_enemy_screen_ptr_tbl_addr[8] = {
    0xB82Bu, 0xB8AAu, 0xB90Du, 0xB9AFu, 0xBA48u, 0xBB24u, 0xBBB7u, 0xBCA9u
};

/* Read one byte of the current screen's enemy-data list. Level 1 keeps using the
   verified hardcoded extraction; every other level reads the original bytes from
   bank 2 directly via the pointer chain. */
static uint8_t contra_screen_enemy_byte(const uint8_t *l1data, uint16_t rom_data, uint8_t offset)
{
    return (l1data != NULL)
        ? l1data[offset]
        : contra_rom_read_u8(2u, (uint16_t)(rom_data + offset));
}

/* load_screen_enemy_data (bank2:1518-1614): spawn this screen's scripted enemies as
   the camera reaches each enemy's trigger position. Horizontal levels trigger on
   LEVEL_SCREEN_SCROLL_OFFSET as an x position and place the enemy at the right edge;
   vertical levels (Level 3) trigger on the same offset as the climb distance and
   place the enemy by column (high nibble) with Y set to how far past the trigger the
   camera already is (bank2:1589-1596). The indoor branch is handled separately. */
static void contra_rom_load_screen_enemy_data(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];
    const uint8_t screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    const bool vertical = (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u);
    const uint8_t *l1data = NULL;
    uint16_t rom_data = 0u;
    uint8_t y;
    uint8_t x_raw;
    uint8_t trigger;
    uint8_t scroll;
    uint8_t distance;
    uint8_t type;
    uint8_t repeat;

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        return; /* indoor/base levels use load_enemy_indoor_level */
    }
    if (level >= 8u)
    {
        return;
    }

    if (level == 0u)
    {
        if (screen >= (sizeof(contra_l1_enemy_screen_tbl) / sizeof(contra_l1_enemy_screen_tbl[0])))
        {
            return;
        }
        l1data = contra_l1_enemy_screen_tbl[screen];
    }
    else
    {
        const uint16_t ptr_tbl = contra_level_enemy_screen_ptr_tbl_addr[level];
        rom_data = (uint16_t)(
            (uint16_t)contra_rom_read_u8(2u, (uint16_t)(ptr_tbl + (uint16_t)(screen * 2u))) |
            ((uint16_t)contra_rom_read_u8(2u, (uint16_t)(ptr_tbl + (uint16_t)(screen * 2u) + 1u)) << 8u));
    }

    y = ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET];
    x_raw = contra_screen_enemy_byte(l1data, rom_data, y);
    if (x_raw == 0xFFu)
    {
        return; /* end of screen data */
    }

    trigger = (uint8_t)(x_raw & 0xFEu);
    scroll = ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    if (trigger == scroll)
    {
        distance = 0u;
    }
    else if (trigger > scroll)
    {
        return; /* camera hasn't reached this enemy's trigger yet */
    }
    else
    {
        distance = (uint8_t)((uint8_t)((uint8_t)(trigger - scroll) ^ 0xFFu) + 1u);
    }

    ++y;
    type = (uint8_t)(contra_screen_enemy_byte(l1data, rom_data, y) & 0x3Fu);
    repeat = (uint8_t)((contra_screen_enemy_byte(l1data, rom_data, y) >> 6) & 0x03u);

    for (;;)
    {
        const uint8_t byte3 = contra_screen_enemy_byte(l1data, rom_data, (uint8_t)(y + 1u));
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
            if (vertical)
            {
                ram[CONTRA_RAM_ENEMY_X_POS + sx] = (uint8_t)(byte3 & 0xF0u);
                ram[CONTRA_RAM_ENEMY_Y_POS + sx] = distance;
            }
            else
            {
                ram[CONTRA_RAM_ENEMY_Y_POS + sx] = (uint8_t)(byte3 & 0xF0u);
                ram[CONTRA_RAM_ENEMY_X_POS + sx] = (uint8_t)(0xF0u - distance);
            }
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
    core->l1_supertile[x] = 0xFFu;
    contra_rom_clear_enemy_pt_2(core, x);
}

static void contra_rom_remove_enemy(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0u;
}

/* remove_enemy for the scroll-off-screen paths: the ROM keeps ENEMY_TYPE (and
   hp/state) in the husk -- only routine + sprite are cleared -- but the
   native-side render caches must be dropped so the husk can't redraw overlays. */
static void contra_rom_remove_enemy_offscreen(ContraCore *core, uint8_t x)
{
    core->l2_structure_tile[x] = 0u;
    core->l2_supertile[x] = 0xFFu;
    core->l1_supertile[x] = 0xFFu;
    contra_rom_remove_enemy(core, x);
}

static void contra_rom_add_4_to_enemy_y_pos(ContraCore *core, uint8_t x);

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

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        /* vertical level (bank7:7827-7832): the enemy is anchored to the terrain, so
           it scrolls DOWN with it; remove it once it passes off the bottom (>= #$e8). */
        const uint8_t new_y = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + ram[CONTRA_RAM_FRAME_SCROLL]);

        ram[CONTRA_RAM_ENEMY_Y_POS + x] = new_y;
        if (new_y >= 0xE8u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* remove_enemy_far keeps type */
        }
        return;
    }

    {
        const uint8_t new_x = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - ram[CONTRA_RAM_FRAME_SCROLL]);

        ram[CONTRA_RAM_ENEMY_X_POS + x] = new_x;
        if (new_x < 0x08u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* remove_enemy keeps type */
        }
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
/* cannonball_explosion_sprite_tbl (bank0:498): type-1 bomb ground-explosion frames. */
static const uint8_t contra_cannonball_explosion_sprite_tbl[3] = {0x37u, 0x36u, 0x37u};
/* adjust_bullet_velocity speed scaling reduces to vel*mult/8: 0.5x .. 1.875x */
/* adjust_bullet_velocity (bank7:10086): per-speed-code shift-add cascades.
   NOT vel*mult/8 -- each shift stage truncates at the byte level (the ROM
   comments document that 1.75x/1.87x drop the fast-byte carry), so the low
   bits differ from a clean multiply; the subpixel phase is gameplay-visible
   as the frame each +1px carry lands on. */
static uint16_t contra_rom_adjust_bullet_velocity(uint8_t fract, uint8_t speed)
{
    uint8_t v04 = fract; /* $04 */
    uint8_t v05 = 0u;    /* $05 */
    uint8_t half;        /* lda $05 / lsr / lda $04 / ror */
    uint8_t a;
    unsigned sum;

    switch (speed & 0x07u)
    {
        case 0x00u: /* .5x: lsr $05 / ror $04 */
        case 0x01u: /* .75x: halve in place, then the 1.5x add of the halved value */
        {
            const uint8_t carry = (uint8_t)(v05 & 0x01u);

            v05 >>= 1u;
            v04 = (uint8_t)((v04 >> 1u) | (uint8_t)(carry << 7u));
            if ((speed & 0x07u) == 0x00u)
            {
                break;
            }
        }
        /* fall through */
        case 0x04u: /* 1.5x: add the byte-half */
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            sum = (unsigned)v04 + half;
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;

        case 0x03u: /* 1.25x: add the byte-quarter */
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            sum = (unsigned)v04 + (half >> 1u);
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;

        case 0x02u: /* 1x */
            break;

        case 0x05u: /* 1.62x: v + half + eighth */
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            a = (uint8_t)((uint8_t)((half >> 1u) >> 1u) + half);
            sum = (unsigned)v04 + a;
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;

        case 0x06u: /* 1.75x: v + half + quarter (byte-truncated, ROM quirk) */
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            a = (uint8_t)((uint8_t)(half >> 1u) + half);
            sum = (unsigned)v04 + a;
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;

        case 0x07u: /* 1.87x: v + half + quarter + eighth (byte-truncated) */
        default:
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            a = (uint8_t)((uint8_t)(half >> 1u) + half);
            a = (uint8_t)(a + (uint8_t)((uint8_t)(half >> 1u) >> 1u));
            sum = (unsigned)v04 + a;
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;
    }
    return (uint16_t)(((uint16_t)v05 << 8u) | v04);
}

/* create_enemy_bullet (bank7): spawn a type-1 enemy bullet at (px,py) aimed by
   (angle, quadrant: bit0 up, bit1 left) at the given speed. */
/* create_enemy_bullet (bank7:9857-9911): spawn type-1 enemy bullet aimed by angle/quadrant. */
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
    contra_rom_update_enemy_pos(core, x); /* applies velocity + scroll, removes off-screen */
    /* the ROM continues unconditionally after update_enemy_pos even when the
       bullet was just removed -- the writes below land in the husk, and
       advance_enemy_routine no-ops on routine 0, exactly like the ROM */
    if (core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x00u)
    {
        /* regular bullet (bank0:405-411): on levels that flag solid bullet-bg
           collision (LEVEL_SOLID_BG_COLLISION_CHECK bit 7), remove the bullet
           when it flies into solid background. */
        if (((core->ram[CONTRA_RAM_LEVEL_SOLID_BG_COLLISION_CHECK] & 0x80u) != 0u) &&
            (contra_rom_get_bg_collision_far(
                 core, core->ram[CONTRA_RAM_ENEMY_X_POS + x],
                 core->ram[CONTRA_RAM_ENEMY_Y_POS + x]) == 0x80u))
        {
            contra_rom_remove_enemy(core, x);
        }
        return;
    }
    if (core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x01u)
    {
        /* cannonball_add_gravity_explode (bank0:457): the large cannonball
           (bullet sub-type 1, the L1 boss bomb) arcs under gravity -- add #$14 to
           the 16-bit Y velocity each frame -- and explodes at the ground
           (Y >= 0xD0), advancing to the explosion routine. Without this the bomb
           flies in a straight line. */
        const unsigned f =
            (unsigned)core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] + 0x14u;

        core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)f;
        core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
            (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (f >> 8u));
        if (core->ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xD0u)
        {
            core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
            core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
            contra_rom_advance_enemy_routine(core, x); /* -> enemy_bullet_routine_02 */
        }
    }
    else if (core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x03u)
    {
        if ((core->ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xB4u) ||
            (core->ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x20u) ||
            (core->ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xE0u))
        {
            contra_rom_remove_enemy(core, x);
        }
    }
}

/* enemy_bullet_routine_02 (bank0:482): the L1 boss cannonball ground-explosion
   animation -- 3 frames (sprites $37,$36,$37) on an 8-frame step, then advance to
   remove_enemy. */
static void contra_rom_enemy_bullet_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t frame;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return; /* removed by scroll */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    if (frame >= 0x03u)
    {
        contra_rom_advance_enemy_routine(core, x); /* -> remove_enemy */
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_cannonball_explosion_sprite_tbl[frame];
}

/* --- sniper (enemy type 0x06), bank0.asm:1738 --- */
static const uint8_t contra_sniper_animation_delay_tbl[3] = {0x01u, 0x30u, 0x80u};
/* sniper_animation_delay_2_tbl (bank0.asm:1774): re-hide -> re-stand-up delay
   per sniper type, set when sniper_routine_03 loops back to sniper_routine_01. */
static const uint8_t contra_sniper_animation_delay_2_tbl[3] = {0x01u, 0x60u, 0x80u};
static const uint8_t contra_sniper_frame_tbl[3] = {0x03u, 0x00u, 0x00u};
static const uint8_t contra_sniper_attack_delay_tbl[3] = {0x40u, 0x04u, 0x10u};
static const uint8_t contra_sniper_bullet_attack_count_tbl[3] = {0x03u, 0x01u, 0x01u};
/* sniper_standing_sprite_tbl (bank0:1972): muzzle sprite per vertical aim band
   (up / straight / down). sniper_bullet_y/x_offset (bank0:1976/1980): bullet spawn
   offset for the same band. sniper_bullet_speed (bank0:1987) per sniper type. */
static const uint8_t contra_sniper_standing_sprite_tbl[3] = {0x04u, 0x03u, 0x05u};
static const uint8_t contra_sniper_bullet_y_offset[3] = {0xEEu, 0xF5u, 0x06u};
static const uint8_t contra_sniper_bullet_x_offset[3] = {0xF3u, 0xF1u, 0xF1u};
static const uint8_t contra_sniper_bullet_speed[3] = {0x03u, 0x05u, 0x03u};

/* implemented after the aim helpers (contra_rom_aim_at_player /
   contra_rom_player_enemy_x_dist_idx); forward-declared so sniper_routine_02 can
   fire without reordering the file. */
static void contra_rom_sniper_fire_bullet(ContraCore *core, uint8_t x);

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

/* sniper_routine_02 (bank0.asm:1839): render, track scroll, run the attack
   cadence, and fire an aimed bullet at the player on each shot of the burst. */
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
        contra_rom_sniper_fire_bullet(core, x);
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

/* defined later; forward-declared so sniper_routine_03 (the re-hide state) can
   call them without reordering the file. */
static void contra_rom_set_enemy_routine_to_a(ContraCore *core, uint8_t x, uint8_t a);
static void contra_rom_disable_enemy_collision(ContraCore *core, uint8_t x);

/* sniper_routine_03 (bank0.asm:1991): the post-attack RE-HIDE state for the
   crouching/boss sniper. It crouches back down (decrementing ENEMY_FRAME), and
   when the crouch animation completes loops the sniper back to sniper_routine_01
   (ENEMY_ROUTINE = 2) so it pops up and attacks again. CRUCIALLY it ends by
   applying add_scroll_to_enemy_pos (the ROM's `jmp add_scroll_to_enemy_pos`) so
   the hidden sniper stays world-anchored while the screen scrolls. Without this
   state the sniper froze in this routine and drifted with the scroll. */
static void contra_rom_sniper_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_disable_enemy_collision(core, x);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] == 0u)
        {
            const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
            const uint8_t idx = (attr < 3u) ? attr : 0u;
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = contra_sniper_animation_delay_2_tbl[idx];
            /* -> ENEMY_ROUTINE = 2, i.e. re-run sniper_routine_01 (stand up). */
            contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
        }
        /* @continue: boss sniper (type 0x02) at crouch frame 2 nudges position */
        if (ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] == 0x02u &&
            ram[CONTRA_RAM_ENEMY_FRAME + x] == 0x02u)
        {
            contra_rom_add_a_to_enemy_y_pos(core, x, 0x0Eu);
            contra_rom_add_a_to_enemy_x_pos(core, x, 0xFFu);
        }
    }

    /* @set_sprite_add_scroll_exit */
    contra_rom_sniper_set_sprite(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
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
    /* add_4_to_enemy_y_pos is the VERTICAL_SCROLL-snapping variant
       (bank7:8404) -- on the waterfall it seats the sniper on the scroll
       grid; the crouch +5 below is the plain add. */
    contra_rom_add_4_to_enemy_y_pos(core, x);
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
    /* The ROM does NOT bail out when add_scroll_to_enemy_pos removes the
       off-screen sniper here: the rest of the routine runs on the husk (the
       delay still elapses and enable_enemy_collision leaves sw=0x02 on the
       removed slot); only advance_enemy_routine's routine==0 guard stops the
       advance. Faithful husks need the same zombie tail. */
    contra_rom_add_scroll_to_enemy_pos(core, x);
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
/* set_carry_if_past_trigger_point (bank0:3870): horizontal levels trigger when
   the enemy X has scrolled left of trigger_x; the vertical level triggers when
   the enemy Y has scrolled DOWN past trigger_y. */
static bool contra_rom_past_trigger_x(const ContraCore *core, uint8_t x,
                                      uint8_t trigger_x, uint8_t trigger_y)
{
    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        return core->ram[CONTRA_RAM_ENEMY_Y_POS + x] >= trigger_y;
    }
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
    const uint8_t supertile = contra_weapon_box_supertile_tbl[(frame < 3u) ? frame : 0u];

    contra_render_level_1_nametable_update_supertile(
        core,
        (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        supertile);
    /* The ROM routes this through draw_enemy_supertile_a_set_delay, whose
       nametable write persists. Our background is re-composed from the original
       level layout every frame, so the open/partial super-tile only survives if
       it is registered in the per-frame L1 redraw cache (see
       contra_render_native_enemies). Without this the pill-box door never
       visually opens -- only the closed box baked into the level data shows. */
    core->l1_supertile[x] = supertile;
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
    if (!contra_rom_past_trigger_x(core, x, 0xF0u, 0x30u))
    {
        return; /* not yet at activation point */
    }
    if (contra_rom_past_trigger_x(core, x, 0x18u, 0xC8u))
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
    if (contra_rom_past_trigger_x(core, x, 0x18u, 0xC8u))
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
    contra_rom_remove_enemy_offscreen(core, x); /* ROM remove_enemy keeps the husk */
}

/* Defined later (used by the weapon-item landing code); forward-declared so
   add_4_to_enemy_y_pos can grid-snap the enemy Y here. */
static void contra_rom_add_a_with_vert_scroll_to_enemy_y_pos(ContraCore *core, uint8_t x, uint8_t a);

/* add_4_to_enemy_y_pos (bank7:8491-8509): a=4, then add_a_with_vert_scroll. The ROM
   falls through into the vert-scroll snap, so this is NOT a plain Y += 4 -- it grid-
   aligns the enemy to the terrain before nudging down. */
static void contra_rom_add_4_to_enemy_y_pos(ContraCore *core, uint8_t x)
{
    contra_rom_add_a_with_vert_scroll_to_enemy_y_pos(core, x, 0x04u);
}

/* add_y_to_y_pos_get_bg_collision (bank7.asm:8679): bg collision code at
   (ENEMY_X_POS, ENEMY_Y_POS + y_off) without modifying the stored position.
   Codes match the ROM: 0 empty, 1 floor, 2 water, 0x80 solid. */
static uint8_t contra_rom_add_y_to_y_pos_get_bg_collision(const ContraCore *core, uint8_t x, uint8_t y_off)
{
    const uint8_t ey = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_POS + x] + y_off);
    return contra_get_outdoor_bg_collision(core, core->ram[CONTRA_RAM_ENEMY_X_POS + x], ey);
}

/* set_enemy_y_velocity_to_0 (bank7:7858-7862): zero only ENEMY_Y_VELOCITY FRACT and
   FAST. The ROM deliberately leaves ENEMY_Y_VEL_ACCUM (the running sub-pixel carry)
   untouched, so the next time Y velocity resumes its accumulator phase continues. */
static void contra_rom_set_enemy_y_velocity_to_0(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0u;
}

/* --- soldier / running man (enemy type 0x05), bank0.asm:1217 --- */
static const uint8_t contra_soldier_initial_anim_delay_tbl[4] = {0x01u, 0x10u, 0x20u, 0x30u};
/* soldier_x_vel_tbl: {fract,fast} for left/right, horizontal then vertical. */
static const uint8_t contra_soldier_x_vel_tbl[8] = {0x00u, 0xFFu, 0x40u, 0x01u, 0x00u, 0xFFu, 0x00u, 0x01u};
static const uint8_t contra_soldier_vel_index_tbl[8] = {0x00u, 0x00u, 0x04u, 0x00u, 0x04u, 0x00u, 0x04u, 0x04u};
static const uint8_t contra_soldier_velocity_tbl[8] = {0x00u, 0xFEu, 0x48u, 0xFFu, 0x00u, 0xFFu, 0x60u, 0xFFu};

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

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        /* Vertical levels first anchor the soldier to terrain scroll, then use
           the normal one-decrement spawn delay path. The guard must check the
           ROUTINE (the husk-keeping remove_enemy leaves the TYPE in place). */
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
        enable_set_vel = true;
    }
    else if (ram[CONTRA_RAM_FRAME_SCROLL] == 0u)
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
        contra_rom_remove_enemy_offscreen(core, x); /* ROM remove_enemy keeps the husk */
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

/* update_enemy_y_pos (bank7:7881-7890): apply Y velocity (accum+fract, Y += fast + carry). */
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

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        /* vertical level (bank7:7737-7748): apply Y velocity AND the screen scroll so
           the enemy stays anchored to the terrain (update_enemy_y_pos_with_scroll),
           then apply X velocity with no scroll. */
        contra_rom_update_enemy_y_pos(core, x);
        ram[CONTRA_RAM_ENEMY_Y_POS + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + ram[CONTRA_RAM_FRAME_SCROLL]);
        if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xE8u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* ROM: remove_enemy keeps type */
            return;
        }
        contra_rom_update_enemy_x_pos(core, x);
        if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* ROM: remove_enemy keeps type */
        }
        return;
    }

    contra_rom_update_enemy_x_pos(core, x);
    ram[CONTRA_RAM_ENEMY_X_POS + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - ram[CONTRA_RAM_FRAME_SCROLL]);
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* ROM: remove_enemy keeps type */
        return;
    }
    contra_rom_update_enemy_y_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xE8u)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* ROM: remove_enemy keeps type */
    }
}

/* add_a_y_to_enemy_pos_get_bg_collision (bank7.asm:8692): bg collision at
   (ENEMY_X_POS + a, ENEMY_Y_POS + y_off), positions unchanged. */
static uint8_t contra_rom_add_a_y_to_enemy_pos_get_bg_collision(
    const ContraCore *core, uint8_t x, uint8_t a, uint8_t y_off)
{
    const uint8_t ex = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_X_POS + x] + a);
    const uint8_t ey = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_POS + x] + y_off);

    return contra_get_outdoor_bg_collision(core, ex, ey);
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
/* soldier_change_direction (bank0:1477-1483): flip soldier direction, reset X velocity. */
static void contra_rom_soldier_change_direction(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 1u);
    core->ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_VAR_2 + x] ^ 0x01u);
    contra_rom_soldier_set_x_velocity(core, x);
}

static void contra_rom_soldier_start_jump(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t closest = contra_rom_player_enemy_x_dist(core, x);
    uint8_t y_delta = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + closest] - ram[CONTRA_RAM_ENEMY_Y_POS + x]);
    uint8_t base = 0x04u;
    uint8_t vel_off;

    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 1u);
    if ((ram[CONTRA_RAM_SPRITE_Y_POS + closest] < ram[CONTRA_RAM_ENEMY_Y_POS + x]) ||
        (y_delta < 0x10u))
    {
        if (ram[CONTRA_RAM_SPRITE_Y_POS + closest] < ram[CONTRA_RAM_ENEMY_Y_POS + x])
        {
            y_delta = (uint8_t)(0u - y_delta);
        }
        (void)y_delta;
        base = 0x00u;
    }

    vel_off = contra_soldier_vel_index_tbl[(uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] & 0x03u) + base)];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = contra_soldier_velocity_tbl[vel_off + 0u];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = contra_soldier_velocity_tbl[vel_off + 1u];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_soldier_velocity_tbl[vel_off + 2u];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_soldier_velocity_tbl[vel_off + 3u];
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        contra_rom_reverse_enemy_x_direction(core, x);
    }
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
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x0Au;
        if ((ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] & 0x80u) == 0u)
        {
            code = contra_rom_add_y_to_y_pos_get_bg_collision(core, x, 0x10u);
            if ((code == 0x80u) || (code == 0x01u))
            {
                ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
                ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
                contra_rom_add_4_to_enemy_y_pos(core, x);
                contra_rom_soldier_stop_y_set_x_velocity(core, x);
            }
            else if (code == 0x02u)
            {
                contra_rom_set_enemy_routine_to_a(core, x, 0x0Au);
            }
        }
        if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
        {
            contra_rom_add_10_to_enemy_y_fract_vel(core, x);
        }
        contra_rom_set_soldier_sprite(core, x);
        contra_rom_update_enemy_pos(core, x);
        return;
    }

    /* @continue (bank0:1356): a firing soldier (attribute bits 2-3) starts an
       attack round when its delay elapses -- the bullet count comes from
       get_soldier_num_bullets (RNG + weapon strength). */
    if (((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x0Cu) != 0u) &&
        (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] != 0u))
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
        {
            static const uint8_t soldier_num_bullets_tbl[8] = {
                0x01u, 0x01u, 0x02u, 0x01u, 0x02u, 0x01u, 0x02u, 0x02u};
            const uint8_t idx = (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] & 0x03u) +
                ((ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] & 0x02u) << 1u));

            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x80u;
            ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x08u;
            ram[CONTRA_RAM_ENEMY_VAR_3 + x] = soldier_num_bullets_tbl[idx & 0x07u];
            contra_rom_advance_enemy_routine(core, x); /* -> soldier_routine_03 */
            contra_rom_set_soldier_sprite(core, x);
            contra_rom_update_enemy_pos(core, x);
            return;
        }
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
        if ((ram[CONTRA_RAM_ENEMY_VAR_4 + x] >= 0x02u) ||
            ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x02u) == 0u))
        {
            contra_rom_soldier_start_jump(core, x);
        }
        else
        {
            contra_rom_soldier_change_direction(core, x);
        }
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

/* flying_capsule_routine_00 (bank0.asm:680): record the base position, then
   horizontal levels enter from the left (X=0x10, cruise right + Y weave) while
   the vertical waterfall rises from the bottom (X+=0x20, Y=0xE0, -1.5 up). */
static void contra_rom_flying_capsule_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x03u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = ram[CONTRA_RAM_ENEMY_X_POS + x];
    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        contra_rom_add_a_to_enemy_x_pos(core, x, 0x20u);
        ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0xE0u;
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x80u; /* flying_capsule_vel_tbl[4] */
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xFEu;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    }
    else
    {
        contra_rom_add_a_to_enemy_y_pos(core, x, 0x20u);
        ram[CONTRA_RAM_ENEMY_X_POS + x] = 0x10u;
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u; /* flying_capsule_vel_tbl[0] */
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x80u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x01u;
    }
    contra_rom_advance_enemy_routine(core, x);
}

/* set_flying_capsule_x_vel + set_flying_capsule_path (bank7:8739/8765): the vertical-
   level weave -- pull X velocity toward the base column VAR_2 by subtracting
   2*(ENEMY_X_POS - VAR_2) (16-bit) from the X velocity. Mirror of the Y weave. */
static void contra_rom_set_flying_capsule_x_vel(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t pos = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t base = ram[CONTRA_RAM_ENEMY_VAR_2 + x];
    uint16_t dist = (uint16_t)(((pos < base) ? 0xFF00u : 0x0000u) | (uint8_t)(pos - base));
    uint16_t vel;

    dist = (uint16_t)(dist << 1u); /* shift count = 1 (vertical) */
    vel = (uint16_t)(((uint16_t)ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] << 8u) |
                     ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x]);
    vel = (uint16_t)(vel - dist);
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = (uint8_t)(vel >> 8u);
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = (uint8_t)vel;
}

/* flying_capsule_routine_01 (bank0:720-734): sprite 0x4D; the weave axis depends on
   LEVEL_SCROLLING_TYPE -- vertical levels weave X (set_flying_capsule_x_vel),
   horizontal/indoor weave Y -- then apply velocity + scroll. The port previously
   always wove Y, so a capsule on a vertical level drifted on the wrong axis. */
static void contra_rom_flying_capsule_routine_01(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x4Du;
    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        contra_rom_set_flying_capsule_x_vel(core, x);
    }
    else
    {
        contra_rom_set_flying_capsule_y_vel(core, x);
    }
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
    /* enemy_routine_init_explosion (bank7:7544): the death burst forces sprite
       palette 2 ((attr & 0xFC) | 0x06) -- the orange/yellow explosion colors. The
       port hardcoded palette 0, which tinted the explosion with each level's
       palette-0 colors and made L2 deaths look unlike L1's. */
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0xFCu) | 0x06u);
}

/* --- soldier death (bank0.asm soldier_routine_04/05, plus the shared bank7
   explosion routines run IN PLACE on the soldier's own slot). The ROM keeps
   ENEMY_TYPE 0x05 through the whole death sequence (arc -> hide -> explosion
   -> remove leaves type set with routine 0), unlike the shared 0xFE actor
   above, so the soldier ports the shared routines without the type swap. --- */

/* init_soldier_hit_vel (bank0.asm:1657): the shared corpse launch -- fly up
   and away from the facing direction (-3.5 Y, 0.375 X; X zeroed at the screen
   edges, reversed by ENEMY_VAR_2), collision off, 16-frame arc. The sniper's
   death (sniper_routine_04) jmp's into this too. */
static void contra_rom_init_soldier_hit_vel(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t x_pos = ram[CONTRA_RAM_ENEMY_X_POS + x];

    contra_rom_disable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x80u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xFCu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x60u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    if ((x_pos < 0x10u) || (x_pos >= 0xF0u))
    {
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    }
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        contra_rom_reverse_enemy_x_direction(core, x);
    }
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x10u);
}

/* apply_gravity_to_destroyed_soldier (bank0.asm:1694): the shared corpse arc --
   gravity (+0x30 fract per frame) against the upward velocity; advance to the
   explosion when the timer elapses or the corpse clears the top of the screen. */
static void contra_rom_apply_gravity_to_destroyed_soldier(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_a_to_enemy_y_fract_vel(core, x, 0x30u);
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x08u)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    contra_rom_update_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
    }
}

/* soldier_routine_03 (bank0:1521): the attack round. Stand (attrs&0x0C < 5) or
   crouch (>= 5, collision box 0x1B) and fire ENEMY_VAR_3+1 bullets on a 0x10
   beat, skipping shots whose muzzle would start off-screen; then restore the
   collision box and walk again. */
static void contra_rom_soldier_routine_03(ContraCore *core, uint8_t x)
{
    static const uint8_t soldier_bullet_y_offset[4] = {0xF7u, 0xF7u, 0x0Au, 0x0Au};
    static const uint8_t soldier_bullet_x_offset[4] = {0xF0u, 0x10u, 0xF0u, 0x10u};
    static const uint8_t soldier_bullet_type_tbl[2] = {0x06u, 0x00u};
    uint8_t *const ram = core->ram;
    uint8_t yidx;
    uint8_t bullet_x;
    unsigned sum;

    if ((uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x0Cu) < 0x05u)
    {
        yidx = 0u; /* standing shot */
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x06u;
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x1Bu; /* crouching box */
        yidx = 2u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x07u;
    }

    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        goto sprite_scroll_exit;
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) != 0u)
    {
        /* fired all bullets: stand back up and walk again */
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x10u;
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
        contra_rom_set_soldier_sprite(core, x);
        contra_rom_add_scroll_to_enemy_pos(core, x);
        contra_rom_set_enemy_routine_to_a(core, x, 0x03u); /* -> soldier_routine_02 */
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x10u;
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        yidx = (uint8_t)(yidx + 1u); /* running right */
    }

    sum = (unsigned)soldier_bullet_x_offset[yidx] + ram[CONTRA_RAM_ENEMY_X_POS + x];
    bullet_x = (uint8_t)sum;
    if ((soldier_bullet_x_offset[yidx] & 0x80u) != 0u)
    {
        if ((sum < 0x100u) || (bullet_x < 0x08u))
        {
            goto sprite_scroll_exit; /* muzzle off-screen to the left */
        }
    }
    else if (sum >= 0x100u)
    {
        goto sprite_scroll_exit; /* muzzle off-screen to the right */
    }
    contra_rom_bullet_generation(
        core,
        soldier_bullet_type_tbl[ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x01u],
        0x06u,
        bullet_x,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + soldier_bullet_y_offset[yidx]));
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x06u; /* gun recoil timer */

sprite_scroll_exit:
    contra_rom_set_soldier_sprite(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
}

/* soldier_routine_04 (bank0.asm:1650): hit -- corpse sprite (frame 0x0B), launch. */
static void contra_rom_soldier_routine_04(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x0Bu;
    contra_rom_set_soldier_sprite(core, x);
    contra_rom_init_soldier_hit_vel(core, x);
}

/* soldier_routine_05 (bank0.asm:1689): the corpse arc. */
static void contra_rom_soldier_routine_05(ContraCore *core, uint8_t x)
{
    contra_rom_set_soldier_sprite(core, x);
    contra_rom_apply_gravity_to_destroyed_soldier(core, x);
}

/* sniper_routine_04 (bank0.asm:2021): hit -- corpse sprite (frame 0x06), launch. */
static void contra_rom_sniper_routine_04(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x06u;
    contra_rom_sniper_set_sprite(core, x);
    contra_rom_init_soldier_hit_vel(core, x);
}

/* sniper_routine_05 (bank0.asm:2028): the corpse arc. */
static void contra_rom_sniper_routine_05(ContraCore *core, uint8_t x)
{
    contra_rom_sniper_set_sprite(core, x);
    contra_rom_apply_gravity_to_destroyed_soldier(core, x);
}

/* explosion_sound_hide_enemy (bank7:7589): the shared tail of enemy_routine_
   init_explosion and mortar_shot_routine_03 -- store the updated state width,
   explosion sound if bit 1 allows, force sprite palette 2, hide the sprite for
   one frame, advance. */
static void contra_rom_explosion_sound_hide_enemy(ContraCore *core, uint8_t x, uint8_t sw)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = sw;
    if ((sw & 0x02u) != 0u)
    {
        contra_play_sound(core, 0x19u); /* sound_19: enemy destroyed */
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0xFCu) | 0x06u);
    if (ram[CONTRA_RAM_ENEMY_SPRITES + x] == 0u)
    {
        contra_rom_remove_enemy(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0xFFu;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x01u; /* invisible sprite for one frame */
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
}

/* enemy_routine_init_explosion (bank7:7572) on the enemy's own slot. */
static void contra_rom_enemy_routine_init_explosion_inplace(ContraCore *core, uint8_t x)
{
    contra_rom_explosion_sound_hide_enemy(
        core, x, (uint8_t)(core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] | 0x81u));
}

/* mortar_shot_routine_03 (bank7:7582): a split mortar round hit the ground --
   score/collision code 0x0D, strip the player-enemy collision bits (0 and 6),
   let bullets pass through (bit 7), then the shared explosion tail. */
static void contra_rom_mortar_shot_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x0Du;
    contra_rom_explosion_sound_hide_enemy(
        core, x, (uint8_t)((ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0xBEu) | 0x80u));
}

/* enemy_routine_explosion (bank7:7616) on the enemy's own slot: 3 sprites
   (explosion_type_00; 4 from explosion_type_01 when ENEMY_STATE_WIDTH bit 3 is
   set), 10 frames apart, then advance to the remove routine. */
static void contra_rom_enemy_routine_explosion_inplace(ContraCore *core, uint8_t x)
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
        contra_rom_advance_enemy_routine(core, x);
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

/* enemy_routine_remove_enemy (bank7:7706): scroll-track one last frame, then
   clear routine + sprite. ENEMY_TYPE intentionally stays set -- the ROM leaves
   it in the slot until a new spawn reuses it. */
static void contra_rom_enemy_routine_remove_inplace(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_remove_enemy(core, x);
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
   (Level-2 0x11 is the roller; level-1 0x11 is the boss door -- not yet ported,
   so this dispatch case is roller-only for now.) */
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
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Eu);
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
            uint8_t delay = contra_boss_gemini_attack_delay_tbl[
                (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]) & 0x03u)];
            const uint8_t strength_delta = (uint8_t)(ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] << 3u);

            ram[CONTRA_RAM_RANDOM_NUM] =
                (uint8_t)(ram[CONTRA_RAM_RANDOM_NUM] + ram[CONTRA_RAM_FRAME_COUNTER]);
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
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Eu);
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

/* boss_gemini_routine_04/06 (bank0:5665-5683): decrement the two-helmet count;
   the last helmet runs the boss-defeated path, otherwise it explodes/removes. */
static void contra_rom_boss_gemini_routine_04(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_WALL_CORE_REMAINING] = (uint8_t)(ram[CONTRA_RAM_WALL_CORE_REMAINING] - 1u);
    if (ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0u)
    {
        contra_init_apu_channels(core);
        contra_play_sound(core, 0x57u);
        ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
        contra_rom_destroy_all_enemies(core, (int)x);
        contra_rom_begin_enemy_explosion(core, x);
        return;
    }
    contra_rom_begin_enemy_explosion(core, x);
}

static const uint8_t contra_spinning_bubbles_speed_tbl[4] = {0x01u, 0x03u, 0x04u, 0x05u};
static const uint8_t contra_spinning_bullet_spin_tbl[4] = {0x08u, 0x06u, 0x04u, 0x02u};
static const uint8_t contra_spinning_bullet_vel_tbl[60] = {
    0x00u, 0x00u, 0x63u, 0x00u, 0xC0u, 0x00u, 0x0Fu, 0x01u, 0x4Bu, 0x01u, 0x72u, 0x01u,
    0x7Eu, 0x01u, 0x72u, 0x01u, 0x4Bu, 0x01u, 0x0Fu, 0x01u, 0xC0u, 0x00u, 0x63u, 0x00u,
    0x00u, 0x00u, 0x9Du, 0xFFu, 0x40u, 0xFFu, 0xF1u, 0xFEu, 0xB5u, 0xFEu, 0x8Eu, 0xFEu,
    0x82u, 0xFEu, 0x8Eu, 0xFEu, 0xB5u, 0xFEu, 0xF1u, 0xFEu, 0x40u, 0xFFu, 0x9Du, 0xFFu,
    0x00u, 0x00u, 0x63u, 0x00u, 0xC0u, 0x00u, 0x0Fu, 0x01u, 0x4Bu, 0x01u, 0x72u, 0x01u};

static uint8_t contra_rom_spinning_bubble_aim_dir(ContraCore *core, uint8_t x)
{
    uint8_t quadrant;
    uint8_t aim = contra_rom_aim_at_player(
        core,
        core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        contra_quadrant_aim_dir_01,
        &quadrant);

    aim = (uint8_t)(aim % 12u);
    if ((quadrant & 0x02u) != 0u)
    {
        aim = (uint8_t)((12u - aim) % 12u);
    }
    return aim;
}

static void contra_rom_set_spinning_bubble_velocity_from_var1(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t dir = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] % 12u);
    const size_t y = (size_t)dir * 2u;
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

    ram[CONTRA_RAM_ENEMY_VAR_2 + x] =
        (ram[CONTRA_RAM_SPRITE_X_POS] <= ram[CONTRA_RAM_ENEMY_X_POS + x]) ? 0u : 1u;
    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = (uint8_t)(ram[CONTRA_RAM_FRAME_COUNTER] & 0x03u);
    ram[CONTRA_RAM_ENEMY_VAR_A + x] =
        contra_spinning_bubbles_speed_tbl[ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x03u];
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = contra_rom_spinning_bubble_aim_dir(core, x);
    contra_rom_set_spinning_bubble_velocity_from_var1(core, x);
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
    {
        const uint8_t target = contra_rom_spinning_bubble_aim_dir(core, x);

        if (target != ram[CONTRA_RAM_ENEMY_VAR_1 + x])
        {
            ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] | 0x03u);
            if (((target - ram[CONTRA_RAM_ENEMY_VAR_1 + x]) & 0x0Fu) < 6u)
            {
                ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)((ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 1u) % 12u);
            }
            else
            {
                ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)((ram[CONTRA_RAM_ENEMY_VAR_1 + x] + 11u) % 12u);
            }
            contra_rom_set_spinning_bubble_velocity_from_var1(core, x);
        }
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
        contra_rom_clear_enemy(core, x);
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

    if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u)
    {
        idx = (uint8_t)(idx + 1u); /* jungle background variant */
        draw_x -= 8;               /* jungle super-tile sits 8px left of the enemy */
    }
    contra_render_level_1_nametable_update_supertile(
        core, draw_x, (int)ram[CONTRA_RAM_ENEMY_Y_POS + x],
        contra_boss_bomb_turret_supertile_tbl[(idx < 6u) ? idx : 0u]);
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
    core->l1_supertile[x] =
        contra_red_turret_supertile_tbl[(idx < 11u) ? idx : 0u];
    contra_render_level_1_nametable_update_supertile(
        core,
        (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        core->l1_supertile[x]);
    return true;
}

/* red_turret_routine_00..02 (bank0): aim left, wait for the player to approach,
   then emerge (super-tile animation) and become collidable. The active rotating
   aim + firing + retract (routine_03..05) are DEFERRED: the turret emerges,
   renders, and can be shot/destroyed (via the kill path -> explosion). */
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
    /* draw_enemy_supertile_a_set_delay (bank7:8599) sets ANIMATION_DELAY = 1
       only when the draw FAILS (CPU graphics buffer full -> retry next frame).
       The native renderer cannot fail, so the caller's delay always stands --
       the port used to write 1 unconditionally here, which made the rotating
       gun open and rotate every frame instead of on its 8/0x30-frame beats. */
    contra_render_level_1_nametable_update_supertile(
        core, (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x], supertile);
    core->l1_supertile[x] = supertile; /* persist for the per-frame L1 redraw */
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
    contra_rom_remove_enemy_offscreen(core, x); /* ROM remove_enemy keeps the husk */
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
static void contra_rom_record_supertile_collision_override(
    ContraCore *core, uint8_t cell_screen_x, uint8_t cell_screen_y, uint8_t tile, uint8_t coll)
{
    uint8_t *const ram = core->ram;
    const uint16_t world_x =
        (uint16_t)(((uint16_t)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                   ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] + cell_screen_x);
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
            core->l1_bridge_gap_coll[i] = coll;
            return; /* already recorded */
        }
    }
    if (core->l1_bridge_gap_count < 24u)
    {
        core->l1_bridge_gap_world_x[core->l1_bridge_gap_count] = world_x;
        core->l1_bridge_gap_screen_y[core->l1_bridge_gap_count] = cell_screen_y;
        core->l1_bridge_gap_tile[core->l1_bridge_gap_count] = tile;
        core->l1_bridge_gap_coll[core->l1_bridge_gap_count] = coll;
        core->l1_bridge_gap_count = (uint8_t)(core->l1_bridge_gap_count + 1u);
    }
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
static void contra_rom_generate_enemy_at_offset(ContraCore *core, uint8_t gen_slot,
                                                uint8_t type, uint8_t x_off, uint8_t y_off)
{
    uint8_t *const ram = core->ram;
    const int slot = contra_rom_find_next_enemy_slot(core);

    if (slot < 0)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_TYPE + slot] = type;
    contra_rom_initialize_enemy(core, (uint8_t)slot);
    ram[CONTRA_RAM_ENEMY_Y_POS + slot] = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + gen_slot] + y_off);
    ram[CONTRA_RAM_ENEMY_X_POS + slot] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + gen_slot] + x_off);
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
   scroll flag is set it moves left by the frame scroll, otherwise it is terrain-
   anchored through add_scroll_to_enemy_pos. */
static void contra_rom_ice_separator_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0xC4u;
    if (ram[CONTRA_RAM_TANK_AUTO_SCROLL] != 0u)
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

static bool contra_rom_fire_beam_step_anim(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        return false;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x30u) ? 0x00u : 0x01u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] | ram[CONTRA_RAM_ENEMY_VAR_4 + x]);
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
        !contra_rom_fire_beam_step_anim(core, x))
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
        !contra_rom_fire_beam_step_anim(core, x))
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
        !contra_rom_fire_beam_step_anim(core, x))
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
        !contra_rom_fire_beam_step_anim(core, x))
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
        !contra_rom_fire_beam_step_anim(core, x))
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
        !contra_rom_fire_beam_step_anim(core, x))
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

/* ===== level-7 hangar and level-8 alien enemies (bank0.asm:8027-10385) ===== */

static int contra_screen_tile_aligned_x(const ContraCore *core, int x)
{
    const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];

    return ((x + scroll_offset) & ~7) - scroll_offset;
}

static int contra_screen_tile_aligned_y(int y)
{
    return y & ~7;
}

static void contra_level7_record_tile_update(ContraCore *core, int x, int y, uint8_t tile_index)
{
    const int aligned_x = contra_screen_tile_aligned_x(core, x);
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
    const int aligned_x = contra_screen_tile_aligned_x(core, x);
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
static const uint8_t contra_claw_tile_code_tbl[8][8] = {
    {0x86u, 0x86u, 0x86u, 0x86u, 0x00u, 0x00u, 0x00u, 0x00u},
    {0x80u, 0x80u, 0x82u, 0x84u, 0x00u, 0x00u, 0x00u, 0x00u},
    {0x86u, 0x86u, 0x86u, 0x86u, 0x00u, 0x00u, 0x00u, 0x00u},
    {0x80u, 0x80u, 0x82u, 0x84u, 0x00u, 0x00u, 0x00u, 0x00u},
    {0x86u, 0x86u, 0x86u, 0x86u, 0x86u, 0x86u, 0x86u, 0x86u},
    {0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u, 0x80u},
    {0x86u, 0x86u, 0x86u, 0x86u, 0x00u, 0x00u, 0x00u, 0x00u},
    {0x80u, 0x80u, 0x82u, 0x84u, 0x00u, 0x00u, 0x00u, 0x00u},
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

static void contra_rom_alien_guardian_routine_0b(ContraCore *core, uint8_t x)
{
    contra_rom_destroy_all_enemies(core, (int)x);
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0xA0u;
    contra_rom_clear_enemy(core, x);
}

/* exe_enemy_type (bank7:7360-7474): dispatch enemy type/routine to handler. */
static void contra_rom_exe_enemy_type(ContraCore *core, uint8_t x)
{
    const uint8_t type = core->ram[CONTRA_RAM_ENEMY_TYPE + x];
    const uint8_t routine = core->ram[CONTRA_RAM_ENEMY_ROUTINE + x];

    switch (type)
    {
        case 0x10u:
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u)
            {
                switch (routine) /* level-8 alien guardian */
                {
                    case 0x01u: contra_rom_advance_enemy_routine(core, x); break;
                    case 0x02u: contra_rom_add_scroll_to_enemy_pos(core, x); break;
                    case 0x03u: contra_rom_add_scroll_to_enemy_pos(core, x); break;
                    case 0x04u: contra_play_sound(core, 0x55u); contra_rom_begin_enemy_explosion(core, x); break;
                    case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                    case 0x06u: contra_rom_advance_enemy_routine(core, x); break;
                    case 0x07u: contra_rom_advance_enemy_routine(core, x); break;
                    case 0x08u: contra_rom_advance_enemy_routine(core, x); break;
                    case 0x09u: contra_rom_advance_enemy_routine(core, x); break;
                    case 0x0Au: contra_rom_advance_enemy_routine(core, x); break;
                    case 0x0Bu: contra_rom_advance_enemy_routine(core, x); break;
                    case 0x0Cu: contra_rom_alien_guardian_routine_0b(core, x); break;
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
                case 0x06u: contra_rom_enemy_routine_explosion_step(core, x); break;
                case 0x07u: contra_rom_clear_enemy(core, x); break;
                default: break;
            }
            break;
        case 0x1Du: /* level-4 boss gemini spinning bubbles */
            switch (routine)
            {
                case 0x01u: contra_rom_spinning_bubbles_routine_00(core, x); break;
                case 0x02u: contra_rom_spinning_bubbles_routine_01(core, x); break;
                case 0x03u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                case 0x04u: contra_rom_enemy_routine_explosion_step(core, x); break;
                case 0x05u: contra_rom_clear_enemy(core, x); break;
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
                case 0x05u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                case 0x06u: contra_rom_enemy_routine_explosion_step(core, x); break;
                case 0x07u: contra_rom_clear_enemy(core, x); break;
                default: break;
            }
            break;
        case 0x1Fu: /* level-4 boss red soldier */
            switch (routine)
            {
                case 0x01u: contra_rom_red_blue_soldier_routine_00(core, x); break;
                case 0x02u: contra_rom_red_soldier_routine_01(core, x); break;
                case 0x03u: contra_rom_red_soldier_routine_02(core, x); break;
                case 0x04u: contra_rom_enemy_routine_init_explosion_step(core, x); break;
                case 0x05u: contra_rom_enemy_routine_explosion_step(core, x); break;
                case 0x06u: contra_rom_clear_enemy(core, x); break;
                default: break;
            }
            break;
        case 0x20u: /* level-4 boss red/blue soldier generator */
            switch (routine)
            {
                case 0x01u: contra_rom_red_blue_soldier_gen_routine_00(core, x); break;
                case 0x02u: contra_rom_red_blue_soldier_gen_routine_01(core, x); break;
                case 0x03u: contra_rom_clear_enemy(core, x); break;
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
                    default: break; /* explosion via the 0xFE actor */
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
                /* TODO: port level-5 boss UFO (bank0:6647). Do not route it through
                   level-3 mouth or level-2 wall core behavior. */
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
                /* TODO: port level-5 tank (bank0:6250). Do not route it through
                   level-3 rock cave or level-2 grenade behavior. */
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
                default: break; /* 0x04 fire not yet ported; 0x0A/0x0B water splash */
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
   the open pill box) and remove the enemy when HP reaches 0.
   DEFERRED: laser pass-through (the bullet is consumed on any hit). */
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
        if (diff >= box[2])
        {
            continue;
        }
        /* the X sbc's borrow is always 1: its carry comes from the Y `cmp`,
           which is clear whenever the Y test passed (bank7:6990) */
        diff = (uint8_t)(ram[CONTRA_RAM_PLAYER_BULLET_X_POS + b] - box_x - 1u);
        if (diff >= box[3])
        {
            continue;
        }

        /* hit -- the ROM's loop RTSes after the first overlapping bullet
           (bank7:7035), so an enemy takes at most ONE bullet per frame; the
           bullet is consumed (routine 2) even against a dead/invulnerable
           enemy. */
        hp = ram[CONTRA_RAM_ENEMY_HP + slot];
        ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + b] = 0x02u;
        ram[CONTRA_RAM_PLAYER_BULLET_TIMER + b] = 0x06u;
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
            if ((dead_type == 0x13u) || (dead_type == 0x14u) || (dead_type == 0x08u))
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
            else if ((dead_type == 0x11u) && !is_indoor_base)
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
   (ducks under them). DEFERRED: landing on rideable enemies. */
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
           lets the player jump OFF the platform instead of being re-pinned to it. */
        {
            const uint8_t sw = ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot];

            if (((sw & 0x40u) != 0u) && ((sw & 0x20u) == 0u) &&
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
   (ENEMY_ATTRIBUTES bits 4-5 = 2,1,0 -> soldier_initial_anim_delay_tbl). The
   ledge-handling bit (bit 1) comes from incrementing the $06 zero-page temp,
   whose entry value is whatever the previous frame's logic left there (usually
   the palette-pointer low byte, mirrored in contra_load_palettes_color_to_cpu);
   the temp lives in ram[0x06] so the junk chain is reproduced. */
static void contra_rom_create_default_soldiers(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t dir = (ram[CONTRA_RAM_SOLDIER_GENERATION_X_POS] < 0x80u) ? 0x01u : 0x00u;
    int y;

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

static void contra_run_level_enemy_logic(ContraCore *core)
{
    contra_load_bank_3_handle_scroll(core);
    contra_rom_exe_all_enemy_routine(core);
    if ((core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) ||
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u))
    {
        contra_rom_load_indoor_enemy_data(core);
    }
    else
    {
        contra_rom_load_screen_enemy_data(core);
    }
    contra_rom_exe_soldier_generation(core);
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

/* end_of_lvl_routine_lvl_3 (bank3:1439): two states -- walk to the dragon gate at
   screen center, then jump into it and vanish behind the wall. */
static void contra_run_level_3_end_level_player_routine(ContraCore *core, uint8_t player_index, uint8_t state_index)
{
    uint8_t *const ram = core->ram;

    if (state_index == 0u)
    {
        const uint8_t px = ram[CONTRA_RAM_SPRITE_X_POS + player_index];
        const uint8_t dist = (px >= 0x80u) ? (uint8_t)(px - 0x80u) : (uint8_t)(0x80u - px);

        ram[CONTRA_RAM_CONTROLLER_STATE + player_index] =
            (px < 0x80u) ? CONTRA_BUTTON_RIGHT : CONTRA_BUTTON_LEFT;
        if (dist < 0x08u)
        {
            ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
        }
        return;
    }

    /* end_of_lvl_routine_lvl_3_01 (bank3:1466): jump into the gate. */
    ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + player_index] = CONTRA_BUTTON_A;
    if (ram[CONTRA_RAM_PLAYER_JUMP_STATUS + player_index] == 0u)
    {
        return; /* the jump is just starting */
    }
    if ((ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + player_index] & 0x80u) != 0u)
    {
        return; /* still rising */
    }
    if (ram[CONTRA_RAM_SPRITE_Y_POS + player_index] < 0xB0u)
    {
        return; /* not yet fallen to the top of the wall below the gate */
    }
    /* make_player_invisible (bank3:1550): disappear behind the wall. */
    ram[CONTRA_RAM_PLAYER_HIDDEN + player_index] = 0xFFu;
    ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] = 0x00u;
    ram[CONTRA_RAM_PLAYER_SPRITE_CODE + player_index] = 0x00u;
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

/* end_of_lvl_routine_indoor (bank3:1385): walk each player to their elevator
   (P1 left to x=0x0C, P2 right to x=0xF4), mount it (state 3 = can't move,
   sprite_91), wait for the other player, then ride up. */
static void contra_run_indoor_end_level_player_routine(ContraCore *core, uint8_t player_index, uint8_t state_index)
{
    static const uint8_t indoor_lvl_end_input_tbl[2] = {CONTRA_BUTTON_LEFT, CONTRA_BUTTON_RIGHT};
    static const uint8_t indoor_lvl_elevator_pos_tbl[2] = {0x0Cu, 0xF4u};
    static const uint8_t indoor_lvl_elevator_attr_tbl[2] = {0x00u, 0x45u};
    uint8_t *const ram = core->ram;

    switch (state_index)
    {
        case 0x00u:
        {
            const uint8_t dx = (uint8_t)(ram[CONTRA_RAM_SPRITE_X_POS + player_index] -
                                         indoor_lvl_elevator_pos_tbl[player_index]);
            const uint8_t dist = ((dx & 0x80u) != 0u) ? (uint8_t)(0u - dx) : dx;

            ram[CONTRA_RAM_CONTROLLER_STATE + player_index] = indoor_lvl_end_input_tbl[player_index];
            if (dist < 0x02u)
            {
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            }
            break;
        }

        case 0x01u:
            ram[CONTRA_RAM_PLAYER_STATE + player_index] = 0x03u; /* can't move */
            ram[CONTRA_RAM_SPRITE_ATTR + player_index] = indoor_lvl_elevator_attr_tbl[player_index];
            ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = 0x91u; /* on the elevator */
            ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            break;

        case 0x02u:
        {
            const uint8_t other_state =
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + (player_index ^ 1u)];

            if ((other_state == 0u) || (other_state >= 0x03u))
            {
                ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] =
                    (uint8_t)(ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE + player_index] + 1u);
            }
            break;
        }

        default:
            ram[CONTRA_RAM_SPRITE_Y_POS + player_index] =
                (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + player_index] - 1u); /* ride up */
            ram[CONTRA_RAM_CPU_SPRITE_BUFFER + player_index] = 0x91u;
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
                    else if ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) ||
                             (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u))
                    {
                        contra_run_indoor_end_level_player_routine(core, (uint8_t)player_index, (uint8_t)(routine_state - 1u));
                    }
                    else if (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
                    {
                        contra_run_level_3_end_level_player_routine(core, (uint8_t)player_index, (uint8_t)(routine_state - 1u));
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
    ram[CONTRA_RAM_VERTICAL_SCROLL] = 0x00u; /* intro screen scrolls from 0 */
    /* The ROM busy-writes the next stage's intro graphics across several
       video frames (FRAME_COUNTER frozen, LEVEL_ROUTINE_INDEX still 5) and
       flips the routine index to 0 only as the load finishes. Measured from
       the reference recording: entering an indoor base (levels 2/4) freezes
       5 frames and the level_routine_00 that follows costs one more flush
       frame (frames 7443-7449); entering an outdoor stage freezes 4 with no
       extra flush frame (frames 15509-15513). */
    if ((ram[CONTRA_RAM_CURRENT_LEVEL] & 0x01u) != 0u)
    {
        core->frame_stall_frames = 0x05u;
        core->level_transition_pending = 0x01u;
    }
    else
    {
        core->frame_stall_frames = 0x04u;
        core->level_transition_pending = 0x00u;
    }
    core->frame_stall_routine_reset = 0x01u;
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
    contra_rom_exe_all_enemy_routine(core);
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

static void contra_render_overlay_supertile(
    ContraCore *core,
    uint16_t supertile_data_addr,
    uint16_t palette_data_addr,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    const uint8_t stripped_index = (uint8_t)(supertile_index & 0x7Fu);
    const uint16_t supertile_addr = (uint16_t)(
        supertile_data_addr + ((uint16_t)stripped_index * 16u)
    );
    const uint8_t supertile_palette = contra_rom_read_u8(
        3u,
        (uint16_t)(palette_data_addr + stripped_index)
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
    /* update_nametable_supertile (bank7:1356-1365) forces the boss-room overlay table
       when LEVEL_LOCATION_TYPE bit 7 is set, so the wall cannon/plating housings draw
       from level_2_4_nametable_update_supertile_data ($BA1A) not the corridor set. */
    const int boss = (core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u;
    const uint16_t supertile_addr = boss
        ? contra_level_2_4_boss_nametable_update_supertile_data_addr
        : contra_level_2_nametable_update_supertile_data_addr;
    const uint16_t palette_addr = boss
        ? contra_level_2_4_boss_nametable_update_palette_data_addr
        : contra_level_2_nametable_update_palette_data_addr;

    contra_write_overlay_supertile_to_ppu(
        core,
        supertile_addr,
        palette_addr,
        dest_x,
        dest_y,
        supertile_index
    );
    contra_render_overlay_supertile(
        core,
        supertile_addr,
        palette_addr,
        dest_x,
        dest_y,
        supertile_index
    );
}

static void contra_render_level_3_overlay_supertile(
    ContraCore *core,
    int dest_x,
    int dest_y,
    uint8_t supertile_index
)
{
    contra_write_overlay_supertile_to_ppu(
        core,
        contra_level_3_nametable_update_supertile_data_addr,
        contra_level_3_nametable_update_palette_data_addr,
        dest_x,
        dest_y,
        supertile_index
    );
    contra_render_overlay_supertile(
        core,
        contra_level_3_nametable_update_supertile_data_addr,
        contra_level_3_nametable_update_palette_data_addr,
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

    /* draw_enemy_supertile_10 (bank7:8552) draws from the CURRENT level's
       nametable update data. Level 3 spawns the same shared enemies that restore
       a background super-tile (pill box 0x02, rotating gun 0x04, red turret 0x07),
       so on level 3 read from the level-3 data, not level-1. */
    if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
    {
        contra_write_overlay_supertile_to_ppu(
            core, contra_level_3_nametable_update_supertile_data_addr,
            contra_level_3_nametable_update_palette_data_addr,
            aligned_x, aligned_y, supertile_index);
        contra_render_overlay_supertile(
            core, contra_level_3_nametable_update_supertile_data_addr,
            contra_level_3_nametable_update_palette_data_addr,
            aligned_x, aligned_y, supertile_index);
        return;
    }

    contra_write_level_1_nametable_update_supertile_to_ppu(core, enemy_x, enemy_y, supertile_index);
    contra_render_level_1_overlay_supertile(core, aligned_x, aligned_y, supertile_index);
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

            /* row-flag $00 = keep the underlying super-tile palette; the back wall
               the core/turret sits on uses the cycling 4th BG palette (slot 3), so
               the structure flashes red/blue with the wall as in the ROM (not the
               forced gray slot 0 that washed the target out). */
            contra_draw_background_tile(core, px, py, pattern_index, 3u);
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
       reloads (same positions the invented path uses). Only valid while the room is
       static -- during the walk-into-screen transition (INDOOR_SCROLL != 0) the room
       re-composes at the perspective scroll offsets, so these fixed-position quadrants
       would float as a misaligned block over the receding back wall. */
    if ((core->l2_blowopen_quadrants != 0u) &&
        (core->ram[CONTRA_RAM_INDOOR_SCROLL] == 0u))
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

    /* destroyed boss-room cannon/plating housings: redraw the destroyed super-tile
       (index 5) at each recorded position so it persists after the enemy explodes and
       its slot is freed (the ROM's nametable write persists; the port re-composes). */
    {
        unsigned d;

        for (d = 0u; d < core->l2_destroyed_struct_count; ++d)
        {
            const int ex = (int)core->l2_destroyed_struct_x[d];
            const int ey = (int)core->l2_destroyed_struct_y[d];

            contra_render_level_2_overlay_supertile(
                core, (ex - 12) & ~7, (ey - 12) & ~7, 0x05u);
        }
    }
}

/* Faithful level-1 exploding bridge: the destroyed super-tiles the bridge writes
   to the nametable must be re-drawn over the re-composed background every frame
   (the native background ignores one-shot nametable writes), the same way the
   level-2 wall structures persist. Each recorded gap cell keeps its destroyed
   super-tile index and a scroll-invariant world X; map that back to the current
   screen X and re-draw it with the same -0x0c / 8px alignment the bridge used. */
static void contra_render_level_1_bridge_gaps(ContraCore *core)
{
    const uint8_t *const ram = core->ram;
    const int scroll_offset = (int)ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    const uint16_t world_base =
        (uint16_t)(((uint16_t)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                   ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET]);
    uint8_t i;

    for (i = 0u; i < core->l1_bridge_gap_count; ++i)
    {
        const uint8_t tile = core->l1_bridge_gap_tile[i];
        /* cell_screen_x already has the -0x0c super-tile offset folded in (it was
           recorded as draw_x_base - 12), so the alignment matches the draw path. */
        const int cell_screen_x =
            (int)core->l1_bridge_gap_world_x[i] - (int)world_base;
        const int aligned_x = ((cell_screen_x + scroll_offset) & ~7) - scroll_offset;
        const int aligned_y = (int)core->l1_bridge_gap_screen_y[i] & ~7;

        if (tile == 0u)
        {
            continue;
        }
        /* Framebuffer overlay only: the destroyed super-tile was already written
           to the PPU nametable model once at explosion time (the ROM's one-shot
           load_bank_3_update_nametable_supertile). Re-writing it here every frame
           is unnecessary and would perturb the nametable; only the visible
           framebuffer needs the persistent redraw. */
        contra_render_overlay_supertile(
            core,
            contra_level_1_nametable_update_supertile_data_addr,
            contra_level_1_nametable_update_palette_data_addr,
            aligned_x,
            aligned_y,
            tile);
    }
}

static uint8_t contra_read_screen_palette_slot_at(const ContraCore *core, int px, int py)
{
    const unsigned horizontal_sum = (unsigned)(uint8_t)px + (unsigned)core->ram[CONTRA_RAM_HORIZONTAL_SCROLL];
    const unsigned vertical_sum = (unsigned)(uint8_t)py + (unsigned)core->ram[CONTRA_RAM_VERTICAL_SCROLL];
    uint8_t nametable_index = (uint8_t)(core->ram[CONTRA_RAM_PPUCTRL_SETTINGS] & 0x01u);
    const uint8_t coarse_x = (uint8_t)(((uint8_t)horizontal_sum & 0xF8u) >> 3u);
    const uint8_t coarse_y = (uint8_t)(((uint8_t)vertical_sum & 0xF8u) >> 3u);

    if (horizontal_sum > 0xFFu)
    {
        nametable_index ^= 0x01u;
    }

    return contra_read_nametable_palette_slot(core, nametable_index, coarse_x, coarse_y);
}

static void contra_render_level_7_tile_animation(ContraCore *core, int x, int y, uint8_t tile_index)
{
    const uint8_t index = (uint8_t)(tile_index & 0x7Fu);
    const uint16_t entry = (uint16_t)(contra_level_7_tile_animation_addr + ((uint16_t)index * 5u));
    uint8_t row_count;
    uint16_t read_addr;
    uint8_t row;

    if (!contra_load_rom_image())
    {
        return;
    }

    row_count = 0x02u;
    if ((contra_rom_read_u8(3u, entry) & 0x80u) != 0u)
    {
        row_count = (uint8_t)(contra_rom_read_u8(3u, entry) & 0x07u);
    }
    read_addr = (uint16_t)(entry + 1u);

    for (row = 0u; row < row_count; ++row)
    {
        uint8_t col;

        for (col = 0u; col < 2u; ++col)
        {
            const int px = x + (int)(col * 8u);
            const int py = y + (int)(row * 8u);
            const uint8_t pattern_index = contra_rom_read_u8(3u, read_addr++);

            contra_draw_background_tile(
                core,
                px,
                py,
                pattern_index,
                contra_read_screen_palette_slot_at(core, px, py));
        }
    }
}

static void contra_render_level_7_nametable_writes(ContraCore *core)
{
    uint8_t i;

    for (i = 0u; i < core->l7_supertile_update_count; ++i)
    {
        const int x = (int)core->l7_supertile_update_x[i];
        const int y = (int)core->l7_supertile_update_y[i];

        contra_write_overlay_supertile_to_ppu(
            core,
            contra_level_7_nametable_update_supertile_data_addr,
            contra_level_7_nametable_update_palette_data_addr,
            x,
            y,
            core->l7_supertile_update_index[i]);
        contra_render_overlay_supertile(
            core,
            contra_level_7_nametable_update_supertile_data_addr,
            contra_level_7_nametable_update_palette_data_addr,
            x,
            y,
            core->l7_supertile_update_index[i]);
    }

    for (i = 0u; i < core->l7_tile_update_count; ++i)
    {
        contra_render_level_7_tile_animation(
            core,
            (int)core->l7_tile_update_x[i],
            (int)core->l7_tile_update_y[i],
            core->l7_tile_update_index[i]);
    }
}

static void contra_render_native_enemies(ContraCore *core)
{
    /* Level 3 (vertical outdoor) also draws background-overlay super-tiles -- the
       dragon boss mouth -- so let its gameplay through to the level-1/3 branch. */
    const bool level3_active =
        (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u) &&
        (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u) &&
        (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u);

    if (!contra_is_native_combat_active(core) && !level3_active)
    {
        return;
    }
    if (contra_is_native_level_7_active(core))
    {
        contra_render_level_7_nametable_writes(core);
        return;
    }
    if (contra_is_native_level_2_active(core))
    {
        /* faithful indoor wall turret/core backgrounds; L1 + soldiers/bullets
           render via their routines and the OAM build */
        contra_render_level_2_wall_structures(core);
    }
    else
    {
        int slot;

        /* faithful level-1 background super-tile enemies (red turret 0x07,
           rotating gun 0x04, door tunnel, ...) must be re-drawn over the
           re-composed background every frame, exactly like the level-2 wall
           structures: the routines' one-shot nametable write is wiped by the
           background recompose, so without this redraw the turret is invisible
           even though its logic runs and it fires. The per-slot cache is only
           populated by the two super-tile draw helpers (red_turret_load_supertile
           and draw_enemy_supertile_a_set_delay), so the bridge gaps below are not
           double-drawn. */
        for (slot = 0; slot < CONTRA_NATIVE_MAX_ENEMIES; ++slot)
        {
            const uint8_t sx = (uint8_t)slot;
            const uint8_t st = core->l1_supertile[sx];
            const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
            int aligned_x;
            int aligned_y;

            if ((st == 0xFFu) || (core->ram[CONTRA_RAM_ENEMY_ROUTINE + sx] == 0u) ||
                !contra_rom_enemy_type_is_supertile(core->ram[CONTRA_RAM_ENEMY_TYPE + sx]))
            {
                continue;
            }
            /* same -0x0c / 8px world-space alignment the draw path uses
               (contra_render_level_1_nametable_update_supertile). */
            aligned_x = ((((int)core->ram[CONTRA_RAM_ENEMY_X_POS + sx] - 12) + scroll_offset) & ~7) - scroll_offset;
            aligned_y = ((int)core->ram[CONTRA_RAM_ENEMY_Y_POS + sx] - 12) & ~7;
            /* level 3 stores its restored super-tiles in the level-3 data (see
               contra_render_level_1_nametable_update_supertile). */
            if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
            {
                contra_render_level_3_overlay_supertile(core, aligned_x, aligned_y, st);
            }
            else
            {
                contra_render_level_1_overlay_supertile(core, aligned_x, aligned_y, st);
            }
        }

        /* faithful level-1 destroyed-bridge super-tiles persist over the
           re-composed background */
        contra_render_level_1_bridge_gaps(core);

        /* level-3 dragon boss mouth: two stacked background super-tiles drawn from
           the mouth's current animation frame (closed/partial/open), redrawn over
           the recomposed background like the level-1 super-tile enemies. */
        if (core->ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
        {
            int slot;

            for (slot = 0; slot < CONTRA_NATIVE_MAX_ENEMIES; ++slot)
            {
                const uint8_t sx = (uint8_t)slot;
                uint8_t frame;
                int ex;
                int ey;

                if ((core->ram[CONTRA_RAM_ENEMY_TYPE + sx] != 0x14u) ||
                    (core->ram[CONTRA_RAM_ENEMY_ROUTINE + sx] < 0x03u))
                {
                    continue; /* not the mouth, or still hidden in the wall (pre routine_02) */
                }
                frame = core->ram[CONTRA_RAM_ENEMY_FRAME + sx];
                if (frame > 2u)
                {
                    frame = 2u;
                }
                ex = (int)core->ram[CONTRA_RAM_ENEMY_X_POS + sx];
                ey = (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + sx];
                contra_render_level_3_overlay_supertile(
                    core, ex - 12, ey - 12,
                    (uint8_t)(contra_boss_mouth_nametable_update_tbl[frame * 2u] & 0x7Fu));
                contra_render_level_3_overlay_supertile(
                    core, ex - 12, ey + 20,
                    (uint8_t)(contra_boss_mouth_nametable_update_tbl[(frame * 2u) + 1u] & 0x7Fu));
            }
        }
    }
}

static void contra_sync_native_sprite_objects_to_cpu_buffer(ContraCore *core)
{
    /* The faithful enemy system maintains the enemy sprite-object slots
       (ENEMY_SPRITES / ENEMY_Y_POS / ENEMY_X_POS = sprite slots 10..25)
       directly in RAM, exactly as the ROM does — they are persistent enemy
       state, not a per-frame rebuild. Nothing to do here. */
    (void)core;
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

    if (core->frame_stall_frames != 0u)
    {
        core->frame_stall_frames = (uint8_t)(core->frame_stall_frames - 1u);
        if ((core->frame_stall_frames == 0u) && (core->frame_stall_routine_reset != 0u))
        {
            core->frame_stall_routine_reset = 0u;
            core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
        }
        contra_render_frame(core, true);
        return;
    }

    /* the NMI flushed CPU_GRAPHICS_BUFFER at the end of the previous frame:
       this frame's writers (fence animation, enemy super-tile stamps) start
       from an empty buffer. Only the BUDGET is modeled (ram[$21]); the actual
       pixels go straight to the native renderer. */
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = 0x00u;

    contra_flush_pending_horizontal_level_writes(core);
    contra_process_level_1_weapon_box_restore(core);
    contra_apply_controller_state(core);
    contra_exe_game_routine(core);
    contra_flush_cpu_graphics_buffer_to_ppu(core);
    contra_render_frame(core, core->level_graphics_wait_frames == 0u);
}

/* DEBUG (NOT part of the faithful port): warp an initialized core straight to the
   Level 2 boss room so the front-end can launch "just the L2 boss fight". Sets up a
   single-player L2 game, runs it to gameplay, then replays the room-advance loop --
   shoot down each room's wall cores (enemy type 0x14) by injecting a player bullet at
   them, then hold Up to walk into the next room -- until LEVEL_LOCATION_TYPE bit 7
   (boss room) is set. Mirrors the reach_level2_boss_room behavior test. */
void contra_core_debug_warp_level2_boss(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    ContraInputSnapshot none = {{0u, 0u}};
    ContraInputSnapshot up = {{0u, 0u}};
    unsigned room;
    unsigned frame;

    up.player[0] = CONTRA_BUTTON_UP;

    /* force a single-player Level 2 game and run it to gameplay (level_routine 4) */
    ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_CURRENT_LEVEL] = 0x01u;
    ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;
    ram[CONTRA_RAM_P2_NUM_LIVES] = 0x00u;
    contra_apply_start_player_env(core);
    for (frame = 0u; frame < 600u; ++frame)
    {
        contra_core_set_input(core, &none);
        contra_core_step_frame(core);
        if ((ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
            (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
        {
            break;
        }
    }

    for (room = 0u; room < 24u; ++room)
    {
        const uint8_t initial_screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
        size_t i;
        int found;

        if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u)
        {
            break; /* boss room reached */
        }

        /* settle until grounded after the previous room transition */
        ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
        for (frame = 0u; frame < 180u; ++frame)
        {
            contra_core_set_input(core, &none);
            contra_core_step_frame(core);
            if ((ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0u) &&
                (ram[CONTRA_RAM_EDGE_FALL_CODE] == 0u))
            {
                break;
            }
        }

        /* shoot down every wall core in this room, then wait for the room to clear */
        for (frame = 0u; frame < 600u; ++frame)
        {
            found = 0;
            for (i = 0u; i < 16u; ++i)
            {
                if ((ram[CONTRA_RAM_ENEMY_ROUTINE + i] != 0u) &&
                    (ram[CONTRA_RAM_ENEMY_TYPE + i] == 0x14u))
                {
                    ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE] = 0x01u;
                    ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE] = 0x01u;
                    ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = ram[CONTRA_RAM_ENEMY_X_POS + i];
                    ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = ram[CONTRA_RAM_ENEMY_Y_POS + i];
                    found = 1;
                }
            }
            ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
            contra_core_set_input(core, &none);
            contra_core_step_frame(core);
            if ((found == 0) && (ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] != 0u))
            {
                break;
            }
        }

        /* hold Up to walk into the next room */
        for (frame = 0u; frame < 420u; ++frame)
        {
            ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
            contra_core_set_input(core, &up);
            contra_core_step_frame(core);
            if ((ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen) ||
                ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u))
            {
                break;
            }
        }
    }

    /* leave the player alive and not mid-jump for the handover to live input */
    ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0x00u;
    contra_apply_start_player_env(core);
}

void contra_core_debug_warp_level4(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    ContraInputSnapshot none = {{0u, 0u}};
    unsigned frame;

    ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    ram[CONTRA_RAM_CURRENT_LEVEL] = 0x03u;
    ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;
    ram[CONTRA_RAM_P2_NUM_LIVES] = 0x00u;
    contra_apply_start_player_env(core);

    for (frame = 0u; frame < 600u; ++frame)
    {
        contra_core_set_input(core, &none);
        contra_core_step_frame(core);
        if ((ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
            (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
        {
            break;
        }
    }
    contra_apply_start_player_env(core);
}

const uint32_t *contra_core_framebuffer(const ContraCore *core)
{
    return core->framebuffer;
}

const uint8_t *contra_core_ram(const ContraCore *core)
{
    return core->ram;
}
