#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "contra/buttons.h"
#include "contra/core.h"

typedef struct Checkpoint
{
    const char *name;
    unsigned frame;
    uint8_t game_routine;
    uint8_t level_routine;
    uint8_t level;
    uint8_t location_type;
    uint8_t screen;
    uint8_t scroll_offset;
    uint8_t player_state;
    uint8_t player_x;
    uint8_t player_y;
    uint8_t lives;
    uint8_t game_over;
    uint8_t demo_end;
    uint8_t indoor_clear;
    uint8_t wall_core_remaining;
    uint32_t ram_hash;
    uint32_t nametable_hash;
    uint32_t palette_hash;
    uint32_t enemy_hash;
    uint32_t framebuffer_hash;
} Checkpoint;

typedef struct CheckpointCapture
{
    const char *name;
    unsigned frame;
    uint8_t game_routine;
    uint8_t level_routine;
    uint8_t level;
    uint8_t location_type;
    uint8_t screen;
    uint8_t scroll_offset;
    uint8_t player_state;
    uint8_t player_x;
    uint8_t player_y;
    uint8_t lives;
    uint8_t game_over;
    uint8_t demo_end;
    uint8_t indoor_clear;
    uint8_t wall_core_remaining;
    uint32_t ram_hash;
    uint32_t nametable_hash;
    uint32_t palette_hash;
    uint32_t enemy_hash;
    uint32_t framebuffer_hash;
} CheckpointCapture;

typedef struct TraceContext
{
    FILE *jsonl;
} TraceContext;

static const Checkpoint expected_level1_right_run[] = {
    {"level1-title", 180u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x2Cu, 0xA2u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x8796A4EAu, 0xD9F4E514u, 0x84853990u, 0x97B79EC5u, 0xDA4EE732u},
    {"level1-gameplay-start", 900u, 0x02u, 0x04u, 0x00u, 0x00u, 0x01u, 0x40u, 0x01u, 0x3Cu, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x25AC5FFFu, 0xCCCA4E0Bu, 0xF5D5CF3Fu, 0x51FD032Du, 0xD67443A3u},
    {"level1-scroll-mid", 1200u, 0x02u, 0x04u, 0x00u, 0x00u, 0x02u, 0x29u, 0x01u, 0x24u, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x09BD7727u, 0x989ED3E6u, 0xB537F879u, 0x635BAE94u, 0x1D4AF4EAu},
    {"level1-enemy-window", 1500u, 0x02u, 0x04u, 0x00u, 0x00u, 0x03u, 0x32u, 0x01u, 0x37u, 0xD4u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x6318E43Cu, 0xC10D99A1u, 0xB537F879u, 0x97B79EC5u, 0x8E5EF477u}
};

static const Checkpoint expected_attract_level1_demo[] = {
    {"level1-title", 180u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x52EAD0C1u, 0xD9F4E514u, 0x7A1A1897u, 0x97B79EC5u, 0x8577ECE4u},
    {"level1-gameplay-start", 900u, 0x02u, 0x04u, 0x00u, 0x00u, 0x00u, 0x2Du, 0x01u, 0x63u, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0xADC01DDBu, 0x632B9A15u, 0xB537F879u, 0x91C6940Fu, 0xA3A192E3u},
    {"level1-scroll-mid", 1200u, 0x02u, 0x04u, 0x00u, 0x00u, 0x01u, 0x37u, 0x01u, 0x45u, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x57493028u, 0xF67A3E75u, 0xB537F879u, 0x6785D32Fu, 0xC7207706u},
    {"level1-enemy-window", 1500u, 0x02u, 0x04u, 0x00u, 0x00u, 0x02u, 0x20u, 0x01u, 0x24u, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0xB4BF8F54u, 0x45B3429Au, 0xF5D5CF3Fu, 0x635BAE94u, 0x01271FDDu}
};

/* Only the checkpoints the faithful attract demo actually reaches before it
   returns to the title (see run_attract_demo_trace); the room-clear sequence the
   invented core's demo used to hit isn't reproduced here and is covered instead by
   the forced run_level2_room_chain_trace. */
static const Checkpoint expected_attract_level2_demo[] = {
    {"level2-attract-first-room", 3006u, 0x02u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x028DE9C2u, 0xD2063DC5u, 0xC852DE1Bu, 0x97B79EC5u, 0xA3A72CA9u},
    {"level2-wall-core-loaded", 3007u, 0x02u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x01u, 0x70u, 0x60u, 0x62u, 0x00u, 0x00u, 0x00u, 0x01u, 0xBAA94974u, 0xD2063DC5u, 0xF5E7D5CBu, 0x50B5FA3Cu, 0x5A66DED6u}
};

static const Checkpoint expected_level2_room_chain[] = {
    {"level2-first-room", 236u, 0x05u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x01u, 0x70u, 0x60u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0xC79F8DDDu, 0xD2063DC5u, 0xC852DE1Bu, 0x50B5FA3Cu, 0x71CD3029u},
    {"level2-after-room-1", 403u, 0x05u, 0x04u, 0x01u, 0x01u, 0x01u, 0x00u, 0x02u, 0x94u, 0x78u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0xC54131CCu, 0xD2063DC5u, 0xF5E7D5CBu, 0x522AB080u, 0xA9DDE8DAu},
    {"level2-after-room-4", 956u, 0x05u, 0x04u, 0x01u, 0x01u, 0x04u, 0x00u, 0x01u, 0x9Cu, 0x78u, 0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x70A5CE80u, 0xD2063DC5u, 0xB5360A6Bu, 0xEAA58ABCu, 0x95CC1340u},
    {"level2-boss-state", 1092u, 0x05u, 0x04u, 0x01u, 0x80u, 0x05u, 0x00u, 0x01u, 0xA0u, 0x78u, 0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0xC5164545u, 0xD2063DC5u, 0x63DFFDCCu, 0x81149843u, 0x9269385Cu},
    {"level2-boss-defeated", 1349u, 0x05u, 0x08u, 0x01u, 0x80u, 0x05u, 0x00u, 0x01u, 0x97u, 0xC9u, 0x01u, 0x00u, 0x00u, 0x00u, 0x01u, 0x615597DAu, 0xCC198FF3u, 0xDBC85DFCu, 0x266703E4u, 0xA426ADE6u}
};

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

static uint32_t hash_enemies(const ContraCore *core)
{
    /* The faithful enemy system keeps all enemy state in real CPU RAM. Hash the
       contiguous enemy-object state block (ENEMY_ROUTINE .. end of ENEMY_VAR_4,
       16 slots each) as the enemy signature. */
    return fnv1a_bytes(
        &core->ram[CONTRA_RAM_ENEMY_ROUTINE],
        (CONTRA_RAM_ENEMY_VAR_4 + CONTRA_RAM_ENEMY_SLOT_COUNT) - CONTRA_RAM_ENEMY_ROUTINE);
}

static void step_with_input(ContraCore *core, uint8_t player_1_input)
{
    ContraInputSnapshot input = {{0u, 0u}};

    input.player[0] = player_1_input;
    contra_core_set_input(core, &input);
    contra_core_step_frame(core);
}

static void step_no_input(ContraCore *core)
{
    step_with_input(core, 0u);
}

static void capture_checkpoint(const ContraCore *core, const char *name, unsigned frame, CheckpointCapture *capture)
{
    memset(capture, 0, sizeof(*capture));
    capture->name = name;
    capture->frame = frame;
    capture->game_routine = core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
    capture->level_routine = core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX];
    capture->level = core->ram[CONTRA_RAM_CURRENT_LEVEL];
    capture->location_type = core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE];
    capture->screen = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    capture->scroll_offset = core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    capture->player_state = core->ram[CONTRA_RAM_PLAYER_STATE];
    capture->player_x = core->ram[CONTRA_RAM_SPRITE_X_POS];
    capture->player_y = core->ram[CONTRA_RAM_SPRITE_Y_POS];
    capture->lives = core->ram[CONTRA_RAM_P1_NUM_LIVES];
    capture->game_over = core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS];
    capture->demo_end = core->ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG];
    capture->indoor_clear = core->ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED];
    capture->wall_core_remaining = core->ram[CONTRA_RAM_WALL_CORE_REMAINING];
    capture->ram_hash = fnv1a_bytes(core->ram, sizeof(core->ram));
    capture->nametable_hash = fnv1a_bytes(core->ppu_nametable, sizeof(core->ppu_nametable));
    capture->palette_hash = fnv1a_bytes(core->ppu_palette, sizeof(core->ppu_palette));
    capture->enemy_hash = hash_enemies(core);
    capture->framebuffer_hash = fnv1a_bytes(core->framebuffer, sizeof(core->framebuffer));
}

static void print_capture_as_expected(const CheckpointCapture *capture)
{
    printf(
        "    {\"%s\", %uu, 0x%02Xu, 0x%02Xu, 0x%02Xu, 0x%02Xu, 0x%02Xu, 0x%02Xu, "
        "0x%02Xu, 0x%02Xu, 0x%02Xu, 0x%02Xu, 0x%02Xu, "
        "0x%02Xu, 0x%02Xu, 0x%02Xu, "
        "0x%08Xu, 0x%08Xu, 0x%08Xu, 0x%08Xu, 0x%08Xu},\n",
        capture->name,
        capture->frame,
        capture->game_routine,
        capture->level_routine,
        capture->level,
        capture->location_type,
        capture->screen,
        capture->scroll_offset,
        capture->player_state,
        capture->player_x,
        capture->player_y,
        capture->lives,
        capture->game_over,
        capture->demo_end,
        capture->indoor_clear,
        capture->wall_core_remaining,
        capture->ram_hash,
        capture->nametable_hash,
        capture->palette_hash,
        capture->enemy_hash,
        capture->framebuffer_hash
    );
}

static void emit_checkpoint_jsonl(
    TraceContext *trace,
    const char *scenario,
    const CheckpointCapture *capture
)
{
    if ((trace == NULL) || (trace->jsonl == NULL))
    {
        return;
    }

    fprintf(
        trace->jsonl,
        "{\"scenario\":\"%s\",\"name\":\"%s\",\"frame\":%u,"
        "\"game_routine\":%u,\"level_routine\":%u,\"level\":%u,"
        "\"location_type\":%u,\"screen\":%u,\"scroll_offset\":%u,"
        "\"player_state\":%u,\"player_x\":%u,\"player_y\":%u,"
        "\"lives\":%u,\"game_over\":%u,"
        "\"demo_end\":%u,\"indoor_clear\":%u,\"wall_core_remaining\":%u,"
        "\"ram_hash\":\"%08X\",\"nametable_hash\":\"%08X\","
        "\"palette_hash\":\"%08X\",\"enemy_hash\":\"%08X\","
        "\"framebuffer_hash\":\"%08X\"}\n",
        scenario,
        capture->name,
        capture->frame,
        capture->game_routine,
        capture->level_routine,
        capture->level,
        capture->location_type,
        capture->screen,
        capture->scroll_offset,
        capture->player_state,
        capture->player_x,
        capture->player_y,
        capture->lives,
        capture->game_over,
        capture->demo_end,
        capture->indoor_clear,
        capture->wall_core_remaining,
        capture->ram_hash,
        capture->nametable_hash,
        capture->palette_hash,
        capture->enemy_hash,
        capture->framebuffer_hash
    );
}

static bool checkpoint_matches(const Checkpoint *expected, const CheckpointCapture *actual)
{
    return (strcmp(expected->name, actual->name) == 0) &&
        (expected->frame == actual->frame) &&
        (expected->game_routine == actual->game_routine) &&
        (expected->level_routine == actual->level_routine) &&
        (expected->level == actual->level) &&
        (expected->location_type == actual->location_type) &&
        (expected->screen == actual->screen) &&
        (expected->scroll_offset == actual->scroll_offset) &&
        (expected->player_state == actual->player_state) &&
        (expected->player_x == actual->player_x) &&
        (expected->player_y == actual->player_y) &&
        (expected->lives == actual->lives) &&
        (expected->game_over == actual->game_over) &&
        (expected->demo_end == actual->demo_end) &&
        (expected->indoor_clear == actual->indoor_clear) &&
        (expected->wall_core_remaining == actual->wall_core_remaining) &&
        (expected->ram_hash == actual->ram_hash) &&
        (expected->nametable_hash == actual->nametable_hash) &&
        (expected->palette_hash == actual->palette_hash) &&
        (expected->enemy_hash == actual->enemy_hash) &&
        (expected->framebuffer_hash == actual->framebuffer_hash);
}

static bool verify_checkpoint(const Checkpoint *expected, const CheckpointCapture *actual)
{
    if (checkpoint_matches(expected, actual))
    {
        return true;
    }

    printf("FAIL checkpoint %s changed\nexpected replacement:\n", expected->name);
    print_capture_as_expected(actual);
    return false;
}

static void step_no_input_counted(ContraCore *core, unsigned *frame)
{
    step_no_input(core);
    ++(*frame);
}

static void step_with_input_counted(ContraCore *core, uint8_t player_1_input, unsigned *frame)
{
    step_with_input(core, player_1_input);
    ++(*frame);
}

static bool run_until_level_routine(
    ContraCore *core,
    uint8_t level,
    uint8_t level_routine,
    unsigned frame_limit,
    unsigned *elapsed_frames
)
{
    unsigned frame;

    for (frame = 0u; frame < frame_limit; ++frame)
    {
        step_no_input_counted(core, elapsed_frames);
        if ((core->ram[CONTRA_RAM_CURRENT_LEVEL] == level) &&
            (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == level_routine))
        {
            return true;
        }
    }

    return false;
}

static bool run_level1_right_trace(TraceContext *trace)
{
    ContraCore core;
    unsigned frame;
    size_t checkpoint_index = 0u;
    bool gameplay_reached = false;
    bool passed = true;

    contra_core_init(&core);

    for (frame = 1u; frame <= 1500u; ++frame)
    {
        uint8_t input = 0u;

        if ((frame == 5u) || (frame == 20u))
        {
            input = CONTRA_BUTTON_START;
        }
        else if (gameplay_reached)
        {
            input = CONTRA_BUTTON_RIGHT;
        }

        step_with_input(&core, input);
        gameplay_reached = gameplay_reached ||
            ((core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
             (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u) &&
             (core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u));

        if ((checkpoint_index < (sizeof(expected_level1_right_run) / sizeof(expected_level1_right_run[0]))) &&
            (frame == expected_level1_right_run[checkpoint_index].frame))
        {
            CheckpointCapture capture;

            capture_checkpoint(&core, expected_level1_right_run[checkpoint_index].name, frame, &capture);
            passed = verify_checkpoint(&expected_level1_right_run[checkpoint_index], &capture) && passed;
            emit_checkpoint_jsonl(trace, "level1_right_run", &capture);
            ++checkpoint_index;
        }
    }

    return passed && (checkpoint_index == (sizeof(expected_level1_right_run) / sizeof(expected_level1_right_run[0])));
}

static bool run_attract_demo_trace(TraceContext *trace)
{
    ContraCore core;
    unsigned frame;
    size_t checkpoint_index = 0u;
    size_t level2_checkpoint_index = 0u;
    bool passed = true;

    contra_core_init(&core);

    for (frame = 1u; frame <= 1500u; ++frame)
    {
        step_no_input(&core);

        if ((checkpoint_index < (sizeof(expected_attract_level1_demo) / sizeof(expected_attract_level1_demo[0]))) &&
            (frame == expected_attract_level1_demo[checkpoint_index].frame))
        {
            CheckpointCapture capture;

            capture_checkpoint(&core, expected_attract_level1_demo[checkpoint_index].name, frame, &capture);
            passed = verify_checkpoint(&expected_attract_level1_demo[checkpoint_index], &capture) && passed;
            emit_checkpoint_jsonl(trace, "attract_level1_demo", &capture);
            ++checkpoint_index;
        }
    }

    /* The level-2 attract demo plays a fixed input recording. Under the faithful
       enemy timing it reaches the first indoor room and loads the wall core, but
       the recording does not destroy it -- the demo returns to the title before
       clearing the room. Whether the real ROM's demo clears L2 room 1 is unknown
       without Mesen ground-truth, so we only assert the two checkpoints the demo
       actually reaches; boss clear and room advancement are covered
       deterministically by run_level2_room_chain_trace. */
    for (; frame <= 12000u; ++frame)
    {
        step_no_input(&core);

        if ((level2_checkpoint_index < (sizeof(expected_attract_level2_demo) / sizeof(expected_attract_level2_demo[0]))) &&
            (core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) &&
            (core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u) &&
            (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
        {
            bool should_capture = false;

            switch (level2_checkpoint_index)
            {
                case 0u:
                    should_capture = true;
                    break;

                case 1u:
                    should_capture =
                        (core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u) &&
                        (core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u);
                    break;

                default:
                    break;
            }

            if (should_capture)
            {
                const Checkpoint *const expected = &expected_attract_level2_demo[level2_checkpoint_index];
                CheckpointCapture capture;

                capture_checkpoint(&core, expected->name, frame, &capture);
                passed = verify_checkpoint(expected, &capture) && passed;
                emit_checkpoint_jsonl(trace, "attract_level2_demo", &capture);
                ++level2_checkpoint_index;
            }

            if (level2_checkpoint_index == (sizeof(expected_attract_level2_demo) / sizeof(expected_attract_level2_demo[0])))
            {
                return passed && (checkpoint_index == (sizeof(expected_attract_level1_demo) / sizeof(expected_attract_level1_demo[0])));
            }
        }
    }

    {
        CheckpointCapture capture;

        capture_checkpoint(&core, "level2-attract-final", frame, &capture);
        printf("FAIL attract demo level 2 checkpoints incomplete\nfinal state:\n");
        print_capture_as_expected(&capture);
    }
    return false;
}

static bool advance_level2_room_once(ContraCore *core, unsigned *elapsed_frames)
{
    unsigned frame;
    const uint8_t initial_screen = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];

    for (frame = 0u; frame < 180u; ++frame)
    {
        step_no_input_counted(core, elapsed_frames);
        if ((core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u) &&
            (core->ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u))
        {
            break;
        }
    }

    core->ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x01u;

    for (frame = 0u; frame < 420u; ++frame)
    {
        step_with_input_counted(core, CONTRA_BUTTON_UP, elapsed_frames);
        if ((core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen) ||
            ((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u))
        {
            return true;
        }
    }

    return false;
}

/* The faithful enemy system lives on real RAM (16 slots): a slot is live when its
   ENEMY_ROUTINE is non-zero, and its type is ENEMY_TYPE. */
static bool find_first_active_enemy_type(const ContraCore *core, uint8_t enemy_type, size_t *enemy_index)
{
    size_t index;

    for (index = 0u; index < 16u; ++index)
    {
        if ((core->ram[CONTRA_RAM_ENEMY_ROUTINE + index] != 0u) &&
            (core->ram[CONTRA_RAM_ENEMY_TYPE + index] == enemy_type))
        {
            *enemy_index = index;
            return true;
        }
    }

    return false;
}

static void keep_player_invincible(ContraCore *core)
{
    core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER] = 0x80u;
    core->ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0x80u;
}

/* Defeat the faithful level-2 boss: the back wall mounts four armor platings
   (type 0x0A, HP 10 each) and two invulnerable cannons (0x08) shielding the boss
   eye (0x10, ENEMY_STATE_WIDTH bit 7 set so bullets pass). Pump bullets into the
   first live plating until every plating is gone, then into the now-exposed eye;
   boss_eye_routine_03 sets BOSS_DEFEATED_FLAG and the level routine advances to
   0x08. (The invented boss used a per-hit count heuristic with an 8-shot cap,
   which can't break a 10-HP plating -- this drives the real RAM directly.) */
static bool destroy_level2_boss_counted(ContraCore *core, unsigned *elapsed_frames)
{
    unsigned frame;

    for (frame = 0u; frame < 2000u; ++frame)
    {
        size_t idx = 0u;

        if (!find_first_active_enemy_type(core, 0x0Au, &idx) &&
            !find_first_active_enemy_type(core, 0x10u, &idx))
        {
            break; /* nothing left to shoot */
        }
        keep_player_invincible(core);
        /* faithful player bullet: gated on SPRITE_CODE != 0 and ROUTINE == 1 */
        core->ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE] = 0x01u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE] = 0x01u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = core->ram[CONTRA_RAM_ENEMY_X_POS + idx];
        core->ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = core->ram[CONTRA_RAM_ENEMY_Y_POS + idx];
        step_no_input_counted(core, elapsed_frames);
        if (core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0u)
        {
            break;
        }
    }

    if (core->ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0u)
    {
        printf("FAIL level 2 boss was not defeated\n");
        return false;
    }

    for (frame = 0u; frame < 64u; ++frame)
    {
        keep_player_invincible(core);
        step_no_input_counted(core, elapsed_frames);
        if (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x08u)
        {
            return true;
        }
    }

    printf("FAIL level 2 boss defeat did not advance level routine\n");
    return false;
}

static bool run_level2_room_chain_trace(TraceContext *trace)
{
    ContraCore core;
    CheckpointCapture capture;
    bool passed = true;
    unsigned room_index;
    unsigned elapsed_frames = 0u;

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;

    if (!run_until_level_routine(&core, 0x01u, 0x04u, 256u, &elapsed_frames))
    {
        printf("FAIL level 2 did not reach gameplay setup\n");
        return false;
    }

    while ((core.ram[CONTRA_RAM_PLAYER_STATE] != 0x01u) ||
           (core.ram[CONTRA_RAM_WALL_CORE_REMAINING] != 0x01u))
    {
        if (elapsed_frames > 360u)
        {
            printf("FAIL level 2 first room did not finish actor setup\n");
            return false;
        }

        step_no_input_counted(&core, &elapsed_frames);
    }

    capture_checkpoint(&core, expected_level2_room_chain[0].name, elapsed_frames, &capture);
    passed = verify_checkpoint(&expected_level2_room_chain[0], &capture) && passed;
    emit_checkpoint_jsonl(trace, "level2_room_chain", &capture);

    for (room_index = 1u; room_index <= 4u; ++room_index)
    {
        if (!advance_level2_room_once(&core, &elapsed_frames))
        {
            printf("FAIL level 2 room advance %u did not complete\n", room_index);
            return false;
        }

        if ((room_index == 1u) || (room_index == 4u))
        {
            const size_t expected_index = (room_index == 1u) ? 1u : 2u;

            capture_checkpoint(
                &core,
                expected_level2_room_chain[expected_index].name,
                elapsed_frames,
                &capture
            );
            passed = verify_checkpoint(&expected_level2_room_chain[expected_index], &capture) && passed;
            emit_checkpoint_jsonl(trace, "level2_room_chain", &capture);
        }
    }

    while ((core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) == 0u)
    {
        if (!advance_level2_room_once(&core, &elapsed_frames))
        {
            printf("FAIL level 2 did not reach boss state\n");
            return false;
        }
    }

    capture_checkpoint(&core, expected_level2_room_chain[3].name, elapsed_frames, &capture);
    passed = verify_checkpoint(&expected_level2_room_chain[3], &capture) && passed;
    emit_checkpoint_jsonl(trace, "level2_room_chain", &capture);

    if (!destroy_level2_boss_counted(&core, &elapsed_frames))
    {
        return false;
    }

    capture_checkpoint(&core, expected_level2_room_chain[4].name, elapsed_frames, &capture);
    passed = verify_checkpoint(&expected_level2_room_chain[4], &capture) && passed;
    emit_checkpoint_jsonl(trace, "level2_room_chain", &capture);
    return passed;
}

static bool parse_args(int argc, char **argv, const char **jsonl_path)
{
    if (argc == 1)
    {
        *jsonl_path = NULL;
        return true;
    }

    if ((argc == 3) && (strcmp(argv[1], "--emit-jsonl") == 0))
    {
        *jsonl_path = argv[2];
        return true;
    }

    printf("usage: %s [--emit-jsonl PATH]\n", argv[0]);
    return false;
}

int main(int argc, char **argv)
{
    TraceContext trace = {NULL};
    const char *jsonl_path = NULL;
    unsigned failures = 0u;

    if (!parse_args(argc, argv, &jsonl_path))
    {
        return 2;
    }

    if (jsonl_path != NULL)
    {
        trace.jsonl = fopen(jsonl_path, "w");
        if (trace.jsonl == NULL)
        {
            printf("FAIL could not open checkpoint trace output %s\n", jsonl_path);
            return 1;
        }
    }

    if (!run_level1_right_trace(&trace))
    {
        ++failures;
    }
    else
    {
        printf("PASS checkpoint trace level1_right_run\n");
    }

    if (!run_attract_demo_trace(&trace))
    {
        ++failures;
    }
    else
    {
        printf("PASS checkpoint trace attract_demo\n");
    }

    if (!run_level2_room_chain_trace(&trace))
    {
        ++failures;
    }
    else
    {
        printf("PASS checkpoint trace level2_room_chain\n");
    }

    if ((trace.jsonl != NULL) && (fclose(trace.jsonl) != 0))
    {
        printf("FAIL could not close checkpoint trace output %s\n", jsonl_path);
        ++failures;
    }

    if (failures != 0u)
    {
        return 1;
    }

    return 0;
}
