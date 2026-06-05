#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "contra/core.h"

static uint32_t fnv1a_bytes(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t hash = 2166136261u;
    size_t index;

    for (index = 0u; index < length; ++index)
    {
        hash ^= bytes[index];
        hash *= 16777619u;
    }

    return hash;
}

static void step_no_input(ContraCore *core)
{
    ContraInputSnapshot input = {{0u, 0u}};

    contra_core_set_input(core, &input);
    contra_core_step_frame(core);
}

int main(void)
{
    ContraCore core;
    const char *const dump_frame_text = getenv("CONTRA_NATIVE_LEVEL1_NAMETABLE_DUMP_FRAME");
    const char *const dump_path = getenv("CONTRA_NATIVE_LEVEL1_NAMETABLE_DUMP_PATH");
    const char *const supertile_dump_path = getenv("CONTRA_NATIVE_LEVEL1_SUPERTILE_DUMP_PATH");
    const char *const framebuffer_dump_path = getenv("CONTRA_NATIVE_LEVEL1_FRAMEBUFFER_DUMP_PATH");
    const char *const ram_dump_path = getenv("CONTRA_NATIVE_LEVEL1_RAM_DUMP_PATH");
    const char *const oam_dump_path = getenv("CONTRA_NATIVE_LEVEL1_OAM_DUMP_PATH");
    const char *const palette_dump_path = getenv("CONTRA_NATIVE_LEVEL1_PALETTE_DUMP_PATH");
    const char *const max_frame_text = getenv("CONTRA_NATIVE_LEVEL1_MAX_FRAME");
    const unsigned dump_frame = (dump_frame_text != NULL) ? (unsigned)strtoul(dump_frame_text, NULL, 10) : 0u;
    const unsigned max_frame = (max_frame_text != NULL) ? (unsigned)strtoul(max_frame_text, NULL, 10) : 1500u;
    unsigned frame;

    contra_core_init(&core);

    for (frame = 1u; frame <= max_frame; ++frame)
    {
        const uint8_t *const ram = core.ram;

        step_no_input(&core);
        if ((dump_frame != 0u) && (frame == dump_frame) && (dump_path != NULL))
        {
            FILE *const dump = fopen(dump_path, "wb");

            if (dump != NULL)
            {
                fwrite(core.ppu_nametable, 1u, sizeof(core.ppu_nametable), dump);
                fclose(dump);
            }
        }
        if ((dump_frame != 0u) && (frame == dump_frame) && (supertile_dump_path != NULL))
        {
            FILE *const dump = fopen(supertile_dump_path, "wb");

            if (dump != NULL)
            {
                fwrite(core.level_screen_supertiles, 1u, sizeof(core.level_screen_supertiles), dump);
                fclose(dump);
            }
        }
        if ((dump_frame != 0u) && (frame == dump_frame) && (framebuffer_dump_path != NULL))
        {
            FILE *const dump = fopen(framebuffer_dump_path, "wb");

            if (dump != NULL)
            {
                fwrite(core.framebuffer, sizeof(core.framebuffer[0]), CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT, dump);
                fclose(dump);
            }
        }
        if ((dump_frame != 0u) && (frame == dump_frame) && (ram_dump_path != NULL))
        {
            FILE *const dump = fopen(ram_dump_path, "wb");

            if (dump != NULL)
            {
                fwrite(core.ram, 1u, sizeof(core.ram), dump);
                fclose(dump);
            }
        }
        if ((dump_frame != 0u) && (frame == dump_frame) && (oam_dump_path != NULL))
        {
            FILE *const dump = fopen(oam_dump_path, "wb");

            if (dump != NULL)
            {
                fwrite(core.latched_oam, 1u, sizeof(core.latched_oam), dump);
                fclose(dump);
            }
        }
        if ((dump_frame != 0u) && (frame == dump_frame) && (palette_dump_path != NULL))
        {
            FILE *const dump = fopen(palette_dump_path, "wb");

            if (dump != NULL)
            {
                fwrite(core.ppu_palette, 1u, sizeof(core.ppu_palette), dump);
                fclose(dump);
            }
        }
        printf(
            "{\"frame\":%u,\"game_routine\":%u,\"level_routine\":%u,\"level\":%u,"
            "\"frame_counter\":%u,\"demo_mode\":%u,\"game_init\":%u,"
            "\"delay_low\":%u,\"delay_high\":%u,"
            "\"location_type\":%u,\"screen\":%u,\"scroll_offset\":%u,"
            "\"horizontal_scroll\":%u,\"vertical_scroll\":%u,"
            "\"frame_scroll\":%u,\"player_frame_scroll\":%u,\"p2_frame_scroll\":%u,"
            "\"player_x_velocity\":%u,\"p2_x_velocity\":%u,"
            "\"auto_scroll_00\":%u,\"auto_scroll_01\":%u,\"tank_auto_scroll\":%u,"
            "\"ppu_tile_offset\":%u,\"ppu_addr_low\":%u,\"ppu_addr_high\":%u,"
            "\"attr_addr_high\":%u,\"supertile_nt_offset\":%u,"
            "\"player_state\":%u,\"p2_state\":%u,\"player_x\":%u,\"p2_x\":%u,\"player_y\":%u,\"p2_y\":%u,"
            "\"controller\":%u,\"p2_controller\":%u,\"controller_diff\":%u,\"p2_controller_diff\":%u,"
            "\"jump\":%u,\"p2_jump\":%u,\"edge_fall\":%u,\"p2_edge_fall\":%u,"
            "\"y_fast\":%u,\"p2_y_fast\":%u,\"y_fract\":%u,\"p2_y_fract\":%u,"
            "\"fall_freeze\":%u,\"p2_fall_freeze\":%u,"
            "\"lives\":%u,\"game_over\":%u,\"p2_game_over\":%u,\"demo_end\":%u,"
            "\"oam_offset\":%u,"
            "\"ram_hash\":\"%08X\",\"pattern_hash\":\"%08X\",\"nametable_hash\":\"%08X\","
            "\"palette_hash\":\"%08X\",\"framebuffer_hash\":\"%08X\"}\n",
            frame,
            (unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
            (unsigned)ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
            (unsigned)ram[CONTRA_RAM_CURRENT_LEVEL],
            (unsigned)ram[CONTRA_RAM_FRAME_COUNTER],
            (unsigned)ram[CONTRA_RAM_DEMO_MODE],
            (unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG],
            (unsigned)ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE],
            (unsigned)ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE],
            (unsigned)ram[CONTRA_RAM_LEVEL_LOCATION_TYPE],
            (unsigned)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER],
            (unsigned)ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET],
            (unsigned)ram[CONTRA_RAM_HORIZONTAL_SCROLL],
            (unsigned)ram[CONTRA_RAM_VERTICAL_SCROLL],
            (unsigned)ram[CONTRA_RAM_FRAME_SCROLL],
            (unsigned)ram[CONTRA_RAM_PLAYER_FRAME_SCROLL],
            (unsigned)ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_X_VELOCITY],
            (unsigned)ram[CONTRA_RAM_PLAYER_X_VELOCITY + 1u],
            (unsigned)ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00],
            (unsigned)ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01],
            (unsigned)ram[CONTRA_RAM_TANK_AUTO_SCROLL],
            (unsigned)ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET],
            (unsigned)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE],
            (unsigned)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE],
            (unsigned)ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE],
            (unsigned)ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET],
            (unsigned)ram[CONTRA_RAM_PLAYER_STATE],
            (unsigned)ram[CONTRA_RAM_PLAYER_STATE + 1u],
            (unsigned)ram[CONTRA_RAM_SPRITE_X_POS],
            (unsigned)ram[CONTRA_RAM_SPRITE_X_POS + 1u],
            (unsigned)ram[CONTRA_RAM_SPRITE_Y_POS],
            (unsigned)ram[CONTRA_RAM_SPRITE_Y_POS + 1u],
            (unsigned)ram[CONTRA_RAM_CONTROLLER_STATE],
            (unsigned)ram[CONTRA_RAM_CONTROLLER_STATE + 1u],
            (unsigned)ram[CONTRA_RAM_CONTROLLER_STATE_DIFF],
            (unsigned)ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_JUMP_STATUS],
            (unsigned)ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 1u],
            (unsigned)ram[CONTRA_RAM_EDGE_FALL_CODE],
            (unsigned)ram[CONTRA_RAM_EDGE_FALL_CODE + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY],
            (unsigned)ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY],
            (unsigned)ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE],
            (unsigned)ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + 1u],
            (unsigned)ram[CONTRA_RAM_P1_NUM_LIVES],
            (unsigned)ram[CONTRA_RAM_P1_GAME_OVER_STATUS],
            (unsigned)ram[CONTRA_RAM_P2_GAME_OVER_STATUS],
            (unsigned)ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG],
            (unsigned)ram[CONTRA_RAM_OAMDMA_CPU_BUFFER_OFFSET],
            fnv1a_bytes(core.ram, sizeof(core.ram)),
            fnv1a_bytes(core.ppu_pattern, sizeof(core.ppu_pattern)),
            fnv1a_bytes(core.ppu_nametable, sizeof(core.ppu_nametable)),
            fnv1a_bytes(core.ppu_palette, sizeof(core.ppu_palette)),
            fnv1a_bytes(core.framebuffer, sizeof(core.framebuffer))
        );
    }

    return 0;
}
