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
    const char *const enemy_dump_path = getenv("CONTRA_NATIVE_LEVEL1_ENEMY_DUMP_PATH");
    const unsigned dump_frame = (dump_frame_text != NULL) ? (unsigned)strtoul(dump_frame_text, NULL, 10) : 0u;
    unsigned frame;

    contra_core_init(&core);

    for (frame = 1u; frame <= 1500u; ++frame)
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
        if ((dump_frame != 0u) && (frame == dump_frame) && (enemy_dump_path != NULL))
        {
            FILE *const dump = fopen(enemy_dump_path, "w");
            size_t enemy_index;

            if (dump != NULL)
            {
                for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
                {
                    const ContraNativeEnemy *const enemy = &core.enemies[enemy_index];

                    if (enemy->active == 0u)
                    {
                        continue;
                    }

                    fprintf(
                        dump,
                        "%zu active=%u type=%02X attrs=%02X hp=%02X state=%02X timer=%02X cooldown=%02X "
                        "flags=%02X screen_id=%02X sprite_code=%02X sprite_attr=%02X x=%d y=%d vx=%d vy=%d\n",
                        enemy_index,
                        (unsigned)enemy->active,
                        (unsigned)enemy->type,
                        (unsigned)enemy->attrs,
                        (unsigned)enemy->hp,
                        (unsigned)enemy->state,
                        (unsigned)enemy->timer,
                        (unsigned)enemy->cooldown,
                        (unsigned)enemy->flags,
                        (unsigned)enemy->screen_id,
                        (unsigned)enemy->sprite_code,
                        (unsigned)enemy->sprite_attr,
                        (int)enemy->x,
                        (int)enemy->y,
                        (int)enemy->vx,
                        (int)enemy->vy
                    );
                }

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
            "\"lives\":%u,\"game_over\":%u,\"p2_game_over\":%u,\"demo_end\":%u,"
            "\"ram_hash\":\"%08X\",\"nametable_hash\":\"%08X\","
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
            (unsigned)ram[CONTRA_RAM_P1_NUM_LIVES],
            (unsigned)ram[CONTRA_RAM_P1_GAME_OVER_STATUS],
            (unsigned)ram[CONTRA_RAM_P2_GAME_OVER_STATUS],
            (unsigned)ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG],
            fnv1a_bytes(core.ram, sizeof(core.ram)),
            fnv1a_bytes(core.ppu_nametable, sizeof(core.ppu_nametable)),
            fnv1a_bytes(core.ppu_palette, sizeof(core.ppu_palette)),
            fnv1a_bytes(core.framebuffer, sizeof(core.framebuffer))
        );
    }

    return 0;
}
