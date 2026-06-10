/* Reproduce the reported bug: defeating the Level 3 boss does not transition to
   Level 4. Start a real Level 3 game (CONTRA_START_LEVEL=2), auto-advance to the
   boss reveal, then force-kill the boss mouth (type 0x14) by injecting player
   bullets onto it -- exactly the trick contra_core_debug_warp_level2_boss uses --
   and report whether CURRENT_LEVEL advances from 0x02 to 0x03. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "contra/buttons.h"
#include "contra/core.h"
#include "contra/ram.h"

static int boss_mouth_slot(const uint8_t *ram)
{
    int i;
    for (i = 0; i < 24; ++i)
    {
        if ((ram[CONTRA_RAM_ENEMY_ROUTINE + i] != 0u) &&
            (ram[CONTRA_RAM_ENEMY_TYPE + i] == 0x14u))
        {
            return i;
        }
    }
    return -1;
}

int main(void)
{
    static ContraCore core;
    const uint8_t *ram;
    unsigned frame;
    int reached_gameplay = 0;
    int saw_boss = 0;
    int killed = 0;
    unsigned boss_first_frame = 0u;

    setenv("CONTRA_START_LEVEL", "2", 1);
    contra_core_init(&core);
    ram = contra_core_ram(&core);

    for (frame = 1u; frame <= 60000u; ++frame)
    {
        ContraInputSnapshot in = {{0u, 0u}};
        uint8_t b = 0u;

        if (!reached_gameplay)
        {
            /* tap START every 16 frames until the game starts (pulse, not hold) */
            if ((frame % 16u) == 0u)
            {
                b = CONTRA_BUTTON_START;
            }
        }
        else if (reached_gameplay)
        {
            b = CONTRA_BUTTON_RIGHT | CONTRA_BUTTON_B; /* run right, fire */
        }
        in.player[0] = b;
        contra_core_set_input(&core, &in);

        /* keep the player alive while probing */
        if (reached_gameplay)
        {
            core.ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
        }

        contra_core_step_frame(&core);

        if (!reached_gameplay &&
            ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u &&
            ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u &&
            ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u)
        {
            reached_gameplay = 1;
            printf("[%u] reached L3 gameplay\n", frame);
        }

        if (reached_gameplay)
        {
            (void)boss_mouth_slot;
            (void)saw_boss;
            (void)boss_first_frame;

            /* Reproduce the post-defeat state directly: the mouth-death special
               case (core.c:15935) sets BOSS_DEFEATED_FLAG=1 with the boss-destroyed
               sound + auto-move delay. Do that once, 60 frames into gameplay, then
               let the level state machine run and watch CURRENT_LEVEL. */
            if (!saw_boss && frame > 700u)
            {
                saw_boss = 1;
                boss_first_frame = frame;
                core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
                printf("[%u] forced BOSS_DEFEATED_FLAG=1 (simulating mouth death)\n", frame);
            }

            if (ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0u && !killed)
            {
                killed = 1;
                printf("[%u] BOSS_DEFEATED_FLAG set = 0x%02X (level=0x%02X, lvl_routine=0x%02X)\n",
                       frame, ram[CONTRA_RAM_BOSS_DEFEATED_FLAG],
                       ram[CONTRA_RAM_CURRENT_LEVEL], ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX]);
            }

            if (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u)
            {
                printf("[%u] *** CURRENT_LEVEL advanced to 0x03 (Level 4) -- TRANSITION OK ***\n", frame);
                return 0;
            }
        }
    }

    printf("---- gave up at 60000 frames ----\n");
    printf("reached_gameplay=%d saw_boss=%d (first@%u) killed_flag=%d final_level=0x%02X lvl_routine=0x%02X end_routine=0x%02X boss_flag=0x%02X arms_destroyed=%u\n",
           reached_gameplay, saw_boss, boss_first_frame, killed,
           ram[CONTRA_RAM_CURRENT_LEVEL], ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
           ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX], ram[CONTRA_RAM_BOSS_DEFEATED_FLAG],
           ram[CONTRA_RAM_BOSS_SCREEN_ENEMIES_DESTROYED]);
    return 1;
}
