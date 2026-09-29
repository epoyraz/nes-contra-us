/* Dump the native Level 2 boss-room framebuffer after the same seeded room-chain
 * setup used by the checkpoint trace. This is a visual-comparison probe, not a
 * gameplay shortcut exposed to players. */
#include <stdint.h>
#include <stdio.h>

#include "contra/core.h"
#include "contra/buttons.h"

int main(int argc, char **argv)
{
    ContraCore core;
    ContraInputSnapshot input = {{0u, 0u}};
    const uint32_t *framebuffer;
    FILE *output;
    size_t index;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s OUTPUT_FRAMEBUFFER.bin\n", argv[0]);
        return 2;
    }

    contra_core_boot(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 5u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 1u;
    core.ram[CONTRA_RAM_PLAYER_MODE_1D] = 1u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 1u;
    core.ram[CONTRA_RAM_P1_NUM_LIVES] = 2u;
    for (index = 0u; (index < 3000u) && (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] != 4u); ++index)
    {
        contra_core_set_input(&core, &input);
        contra_core_step_frame(&core);
    }
    /* room chain (checkpoint trace advance_level2_room_once): once the player
       has landed, mark the room cleared and walk Up into the next one */
    for (index = 0u; (index < 16u) && (core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 128u); ++index)
    {
        const uint8_t screen = core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
        unsigned frame;

        input.player[0] = 0u;
        for (frame = 0u; frame < 180u; ++frame)
        {
            contra_core_set_input(&core, &input);
            contra_core_step_frame(&core);
            if ((core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0u) && (core.ram[CONTRA_RAM_EDGE_FALL_CODE] == 0u))
            {
                break;
            }
        }
        core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 1u;
        input.player[0] = CONTRA_BUTTON_UP;
        for (frame = 0u; (frame < 420u) && (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == screen) &&
                         (core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 128u);
             ++frame)
        {
            contra_core_set_input(&core, &input);
            contra_core_step_frame(&core);
        }
    }
    if (core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 128u)
    {
        fprintf(stderr, "FAIL native probe did not reach boss room\n");
        return 1;
    }
    input.player[0] = 0u;
    for (index = 0u; index < 120u; ++index)
    {
        contra_core_set_input(&core, &input);
        contra_core_step_frame(&core);
    }
    if ((core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] != 5u) ||
        (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] != 4u) ||
        (core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 128u) ||
        (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != 5u))
    {
        fprintf(stderr, "FAIL native probe left the active boss arena\n");
        return 1;
    }
    fprintf(
        stderr,
        "native boss probe: routine=%u location=%u screen=%u scroll=%u player=(%u,%u)\n",
        core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
        core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE],
        core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER],
        core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET],
        core.ram[CONTRA_RAM_SPRITE_X_POS],
        core.ram[CONTRA_RAM_SPRITE_Y_POS]);
    framebuffer = contra_core_framebuffer(&core);
    output = fopen(argv[1], "wb");
    if (output == NULL)
    {
        fprintf(stderr, "FAIL could not write %s\n", argv[1]);
        return 1;
    }

    for (index = 0u; index < (CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT); ++index)
    {
        const uint32_t pixel = framebuffer[index];
        const uint8_t bytes[4] = {
            (uint8_t)(pixel & 0xFFu),
            (uint8_t)((pixel >> 8u) & 0xFFu),
            (uint8_t)((pixel >> 16u) & 0xFFu),
            (uint8_t)((pixel >> 24u) & 0xFFu)};

        if (fwrite(bytes, sizeof(bytes), 1u, output) != 1u)
        {
            fclose(output);
            fprintf(stderr, "FAIL could not write %s\n", argv[1]);
            return 1;
        }
    }

    fclose(output);
    return 0;
}
