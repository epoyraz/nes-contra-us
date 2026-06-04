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
    {"level1-title", 180u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x2Cu, 0xA2u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x3F497D23u, 0xD9F4E514u, 0x84853990u, 0x20E0D2C5u, 0xB78899A1u},
    {"level1-gameplay-start", 900u, 0x02u, 0x04u, 0x00u, 0x00u, 0x01u, 0x40u, 0x01u, 0x3Cu, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x6E1F7489u, 0xCCCA4E0Bu, 0xB537F879u, 0x7790B0F2u, 0x013A815Du},
    {"level1-scroll-mid", 1200u, 0x02u, 0x04u, 0x00u, 0x00u, 0x02u, 0x29u, 0x01u, 0x24u, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x38171380u, 0x989ED3E6u, 0xB537F879u, 0x98C4DFE1u, 0xA594FA09u},
    {"level1-enemy-window", 1500u, 0x02u, 0x04u, 0x00u, 0x00u, 0x03u, 0x20u, 0x01u, 0x49u, 0xD4u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x45B46E23u, 0x29FF2B00u, 0xB537F879u, 0xDB03FFC0u, 0x5100BBD9u}
};

static const Checkpoint expected_attract_level1_demo[] = {
    {"level1-title", 180u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xF3C87D2Eu, 0xD9F4E514u, 0x7A1A1897u, 0x20E0D2C5u, 0x3D3F5BF9u},
    {"level1-gameplay-start", 900u, 0x02u, 0x04u, 0x00u, 0x00u, 0x00u, 0x2Du, 0x01u, 0x63u, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x2E862FA1u, 0x632B9A15u, 0xB537F879u, 0x9E49A4EDu, 0xF87C9F89u},
    {"level1-scroll-mid", 1200u, 0x02u, 0x04u, 0x00u, 0x00u, 0x01u, 0x37u, 0x01u, 0x45u, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0xCF82270Cu, 0xF67A3E75u, 0xB537F879u, 0x3BF73401u, 0xC7190139u},
    {"level1-enemy-window", 1500u, 0x02u, 0x04u, 0x00u, 0x00u, 0x02u, 0x20u, 0x01u, 0x24u, 0x64u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0xC3FFCF76u, 0x45B3429Au, 0xB537F879u, 0x724DBDECu, 0xF4061BBDu}
};

static const Checkpoint expected_attract_level2_demo[] = {
    {"level2-attract-first-room", 3006u, 0x02u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x102C22D6u, 0xD2063DC5u, 0xC852DE1Bu, 0x20E0D2C5u, 0xF41F4389u},
    {"level2-wall-core-loaded", 3007u, 0x02u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x01u, 0x70u, 0x60u, 0x62u, 0x00u, 0x00u, 0x00u, 0x01u, 0xF2D28A0Eu, 0xD2063DC5u, 0xC852DE1Bu, 0x8458F413u, 0xF41F4389u},
    {"level2-wall-core-destroyed", 3255u, 0x02u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x01u, 0x44u, 0xA8u, 0x62u, 0x00u, 0x00u, 0x00u, 0x00u, 0x1A59D7D8u, 0xD2063DC5u, 0xC852DE1Bu, 0x80A3F5B3u, 0xCC794AE5u},
    {"level2-room-cleared", 3278u, 0x02u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x01u, 0x58u, 0xA8u, 0x62u, 0x00u, 0x00u, 0x01u, 0x00u, 0xC1B87E43u, 0xC03557CDu, 0xC852DE1Bu, 0xD030A1D3u, 0xA672C76Du},
    {"level2-after-room-1", 3655u, 0x02u, 0x04u, 0x01u, 0x01u, 0x01u, 0x00u, 0x01u, 0x72u, 0x9Cu, 0x62u, 0x00u, 0x00u, 0x00u, 0x01u, 0x20F582EFu, 0xC03557CDu, 0xC852DE1Bu, 0x914E30CAu, 0x2FABFB51u},
    {"level2-demo-finished", 4623u, 0x00u, 0x04u, 0x01u, 0x01u, 0x01u, 0x00u, 0x01u, 0x70u, 0xA8u, 0x61u, 0x00u, 0x00u, 0x00u, 0x01u, 0x9A1954AAu, 0xC03557CDu, 0xC852DE1Bu, 0xC5001482u, 0x8B553C0Du}
};

static const Checkpoint expected_level2_room_chain[] = {
    {"level2-first-room", 236u, 0x05u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x01u, 0x70u, 0x60u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0x63AC3933u, 0xD2063DC5u, 0xC852DE1Bu, 0x8458F413u, 0xF41F4389u},
    {"level2-after-room-1", 403u, 0x05u, 0x04u, 0x01u, 0x01u, 0x01u, 0x00u, 0x01u, 0x94u, 0x78u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0xD3333B48u, 0xD2063DC5u, 0xC852DE1Bu, 0x914E30CAu, 0x8DAA2ADDu},
    {"level2-after-room-4", 811u, 0x05u, 0x04u, 0x01u, 0x01u, 0x04u, 0x00u, 0x01u, 0xA0u, 0x78u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0x75A26579u, 0xD2063DC5u, 0xC852DE1Bu, 0x58853260u, 0xFE7C05E9u},
    {"level2-boss-state", 947u, 0x05u, 0x04u, 0x01u, 0x80u, 0x05u, 0x00u, 0x01u, 0xA0u, 0x78u, 0x02u, 0x00u, 0x00u, 0x00u, 0x01u, 0x1AC1EC59u, 0xD2063DC5u, 0x63DFFDCCu, 0x8EE6E9E0u, 0x9DA1F055u},
    {"level2-boss-defeated", 992u, 0x05u, 0x08u, 0x01u, 0x80u, 0x05u, 0x00u, 0x01u, 0x97u, 0xC9u, 0x02u, 0x00u, 0x00u, 0x01u, 0x01u, 0xC20285D5u, 0xD2063DC5u, 0x63DFFDCCu, 0x58272D65u, 0x9DD192EDu}
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
    return fnv1a_bytes(core->enemies, sizeof(core->enemies));
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

                case 2u:
                    should_capture =
                        (core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x00u) &&
                        (core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u) &&
                        (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x00u) &&
                        (core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u);
                    break;

                case 3u:
                    should_capture =
                        (core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x01u) &&
                        (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x00u);
                    break;

                case 4u:
                    should_capture =
                        (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x01u) &&
                        (core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
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

        if ((level2_checkpoint_index == ((sizeof(expected_attract_level2_demo) / sizeof(expected_attract_level2_demo[0])) - 1u)) &&
            (core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] != 0x02u))
        {
            const Checkpoint *const expected = &expected_attract_level2_demo[level2_checkpoint_index];
            CheckpointCapture capture;

            capture_checkpoint(&core, expected->name, frame, &capture);
            passed = verify_checkpoint(expected, &capture) && passed;
            emit_checkpoint_jsonl(trace, "attract_level2_demo", &capture);
            ++level2_checkpoint_index;
            return passed && (checkpoint_index == (sizeof(expected_attract_level1_demo) / sizeof(expected_attract_level1_demo[0])));
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

static unsigned count_active_enemy_type(const ContraCore *core, uint8_t enemy_type)
{
    unsigned count = 0u;
    size_t index;

    for (index = 0u; index < CONTRA_NATIVE_MAX_ENEMIES; ++index)
    {
        if ((core->enemies[index].active != 0u) &&
            (core->enemies[index].type == enemy_type))
        {
            ++count;
        }
    }

    return count;
}

static bool find_first_active_enemy_type(const ContraCore *core, uint8_t enemy_type, size_t *enemy_index)
{
    size_t index;

    for (index = 0u; index < CONTRA_NATIVE_MAX_ENEMIES; ++index)
    {
        if ((core->enemies[index].active != 0u) &&
            (core->enemies[index].type == enemy_type))
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

static bool shoot_enemy_until_removed_or_changed_counted(
    ContraCore *core,
    uint8_t enemy_type,
    unsigned max_hits,
    unsigned *elapsed_frames
)
{
    unsigned hit;
    size_t enemy_index = 0u;
    const unsigned initial_count = count_active_enemy_type(core, enemy_type);

    for (hit = 0u; hit < max_hits; ++hit)
    {
        if (!find_first_active_enemy_type(core, enemy_type, &enemy_index))
        {
            return true;
        }

        keep_player_invincible(core);
        core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT] = 0x01u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = (uint8_t)core->enemies[enemy_index].x;
        core->ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = (uint8_t)core->enemies[enemy_index].y;
        step_no_input_counted(core, elapsed_frames);

        if (count_active_enemy_type(core, enemy_type) < initial_count)
        {
            return true;
        }
    }

    return count_active_enemy_type(core, enemy_type) < initial_count;
}

static bool destroy_level2_boss_counted(ContraCore *core, unsigned *elapsed_frames)
{
    unsigned frame;

    while (count_active_enemy_type(core, 0x0Au) != 0u)
    {
        if (!shoot_enemy_until_removed_or_changed_counted(core, 0x0Au, 8u, elapsed_frames))
        {
            printf("FAIL level 2 boss wall plating did not break\n");
            return false;
        }
    }

    if (!shoot_enemy_until_removed_or_changed_counted(core, 0x10u, 48u, elapsed_frames))
    {
        printf("FAIL level 2 boss eye did not break\n");
        return false;
    }

    for (frame = 0u; frame < 16u; ++frame)
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
