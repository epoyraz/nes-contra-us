/* Diagnostic: drive the real game into Level 3, drop the player onto a real
   floating-rock platform (type 0x10) using natural physics, then press A and
   observe whether a jump actually starts. No forced player state -- only the
   player's X is nudged over the platform so they fall onto it naturally. */
#include <stdint.h>
#include <stdio.h>

#include "contra/buttons.h"
#include "contra/core.h"

static int find_platform(const uint8_t *ram)
{
    int s;
    for (s = 0; s < 16; ++s)
    {
        if (ram[CONTRA_RAM_ENEMY_TYPE + s] == 0x10u &&
            ram[CONTRA_RAM_ENEMY_ROUTINE + s] != 0u)
        {
            return s;
        }
    }
    return -1;
}

int main(void)
{
    static ContraCore core;
    ContraInputSnapshot input = {{0u, 0u}};
    unsigned frame;
    int phase = 0; /* 0 wait-for-L3, 1 align+fall, 2 settle, 3 jump+observe */
    int plat = -1;
    unsigned phase_start = 0;

    contra_core_init(&core);

    for (frame = 0u; frame < 60000u; ++frame)
    {
        const uint8_t *ram = contra_core_ram(&core);
        input.player[0] = 0u;

        /* boot: press start to leave title/demo */
        if (frame == 20u) input.player[0] = CONTRA_BUTTON_START;

        /* keep the boss-room test progression moving if needed */
        if (ram[CONTRA_RAM_CURRENT_LEVEL] < 2u &&
            ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u &&
            ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u)
        {
            core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
        }

        if (phase == 0)
        {
            if (ram[CONTRA_RAM_CURRENT_LEVEL] == 2u &&
                ram[CONTRA_RAM_PLAYER_STATE + 0u] == 0x01u)
            {
                plat = find_platform(ram);
                if (plat >= 0)
                {
                    /* nudge the player's X to the platform's X so the next fall
                       lands them on it -- but do not touch Y or any state. */
                    core.ram[CONTRA_RAM_SPRITE_X_POS + 0u] = ram[CONTRA_RAM_ENEMY_X_POS + plat];
                    phase = 1;
                    phase_start = frame;
                    printf("[L3] platform slot %d at (x=%u y=%u) sw=0x%02X; player y=%u\n",
                        plat, ram[CONTRA_RAM_ENEMY_X_POS + plat], ram[CONTRA_RAM_ENEMY_Y_POS + plat],
                        ram[CONTRA_RAM_ENEMY_STATE_WIDTH + plat], ram[CONTRA_RAM_SPRITE_Y_POS + 0u]);
                }
            }
        }
        else
        {
            const unsigned d = frame - phase_start;
            /* keep nudging X to stay over the (moving) platform while settling */
            if (plat >= 0 && ram[CONTRA_RAM_ENEMY_TYPE + plat] == 0x10u && phase < 3)
            {
                core.ram[CONTRA_RAM_SPRITE_X_POS + 0u] = ram[CONTRA_RAM_ENEMY_X_POS + plat];
            }

            if (phase == 1 && ram[CONTRA_RAM_PLAYER_ON_ENEMY + 0u] != 0u)
            {
                phase = 2; phase_start = frame;
                printf("[L3] landed on platform at frame d=%u\n", d);
            }
            else if (phase == 2 && d >= 8u)
            {
                phase = 3; phase_start = frame;
                printf("[L3] pressing A now (on_enemy=%u edge=0x%02X jump=0x%02X)\n",
                    ram[CONTRA_RAM_PLAYER_ON_ENEMY], ram[CONTRA_RAM_EDGE_FALL_CODE],
                    ram[CONTRA_RAM_PLAYER_JUMP_STATUS]);
            }

            if (phase == 3)
            {
                if (d < 3u) input.player[0] = CONTRA_BUTTON_A; /* tap jump */
            }
        }

        contra_core_set_input(&core, &input);
        contra_core_step_frame(&core);
        ram = contra_core_ram(&core);

        if (phase >= 2)
        {
            const unsigned d = frame - phase_start;
            printf("ph%d d=%2u y=%3u jump=0x%02X edge=0x%02X on=%u yv=0x%02X freeze=%3u ctrlD=0x%02X platY=%u\n",
                phase, d, ram[CONTRA_RAM_SPRITE_Y_POS + 0u], ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 0u],
                ram[CONTRA_RAM_EDGE_FALL_CODE + 0u], ram[CONTRA_RAM_PLAYER_ON_ENEMY + 0u],
                ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + 0u], ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + 0u],
                ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + 0u],
                plat >= 0 ? ram[CONTRA_RAM_ENEMY_Y_POS + plat] : 0u);
            if (phase == 3 && d >= 30u) break;
        }
    }
    if (phase < 3) printf("[L3] never reached jump phase (phase=%d)\n", phase);
    return 0;
}
