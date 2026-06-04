#include "contra/core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contra/buttons.h"

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

    if (contra_level_1_bridge_has_destroyed_collision_gap(core, screen_x, screen_y))
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

static void contra_run_level_enemy_logic(ContraCore *core)
{
    contra_load_bank_3_handle_scroll(core);
    contra_load_bank_0_exe_all_enemy_routine(core);
    contra_load_bank_2_load_screen_enemy_data(core);
    contra_load_bank_2_exe_soldier_generation(core);

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
