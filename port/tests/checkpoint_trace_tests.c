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
    uint32_t ram_hash;
    uint32_t nametable_hash;
    uint32_t palette_hash;
    uint32_t enemy_hash;
    uint32_t framebuffer_hash;
} CheckpointCapture;

static const Checkpoint expected_level1_right_run[] = {
    {"level1-title", 180u, 0x03u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x2Cu, 0xA2u, 0x00u, 0x00u, 0x12A9B9A6u, 0xD9F4E514u, 0x84853990u, 0x20E0D2C5u, 0xB78899A1u},
    {"level1-gameplay-start", 900u, 0x05u, 0x04u, 0x00u, 0x00u, 0x00u, 0x42u, 0x01u, 0x7Eu, 0x64u, 0x01u, 0x00u, 0x8B854068u, 0xD2063DC5u, 0xB537F879u, 0xD87FCF0Fu, 0x9189637Du},
    {"level1-scroll-mid", 1200u, 0x05u, 0x04u, 0x00u, 0x00u, 0x00u, 0xBAu, 0x01u, 0x7Fu, 0x64u, 0x00u, 0x00u, 0x60AC39C8u, 0xD2063DC5u, 0xB537F879u, 0x7618CF7Fu, 0xF7ECC0B9u},
    {"level1-enemy-window", 1500u, 0x05u, 0x04u, 0x00u, 0x00u, 0x01u, 0xE6u, 0x01u, 0x7Fu, 0x64u, 0x00u, 0x00u, 0x093EB530u, 0xD2063DC5u, 0xB537F879u, 0xCB4056E4u, 0xDAC71F55u}
};

static const Checkpoint expected_level2_room_chain[] = {
    {"level2-first-room", 227u, 0x05u, 0x04u, 0x01u, 0x01u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x02u, 0x00u, 0x0DEA0819u, 0xD2063DC5u, 0xC852DE1Bu, 0x20E0D2C5u, 0x3DDE1DC5u},
    {"level2-after-room-1", 395u, 0x05u, 0x04u, 0x01u, 0x01u, 0x01u, 0x00u, 0x01u, 0x94u, 0x78u, 0x02u, 0x00u, 0x0E9C3618u, 0xD2063DC5u, 0xC852DE1Bu, 0x20E0D2C5u, 0x8DAA2ADDu},
    {"level2-after-room-4", 803u, 0x05u, 0x04u, 0x01u, 0x01u, 0x04u, 0x00u, 0x01u, 0xA0u, 0x78u, 0x02u, 0x00u, 0x98561B49u, 0xD2063DC5u, 0xC852DE1Bu, 0x20E0D2C5u, 0xFE7C05E9u},
    {"level2-boss-state", 939u, 0x05u, 0x04u, 0x01u, 0x80u, 0x05u, 0x00u, 0x01u, 0xA0u, 0x78u, 0x02u, 0x00u, 0x1052BBA9u, 0xD2063DC5u, 0x63DFFDCCu, 0x20E0D2C5u, 0x9DA1F055u}
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

static bool run_level1_right_trace(void)
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
            ++checkpoint_index;
        }
    }

    return passed && (checkpoint_index == (sizeof(expected_level1_right_run) / sizeof(expected_level1_right_run[0])));
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

static bool run_level2_room_chain_trace(void)
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

    capture_checkpoint(&core, expected_level2_room_chain[0].name, elapsed_frames, &capture);
    passed = verify_checkpoint(&expected_level2_room_chain[0], &capture) && passed;

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
    return passed;
}

int main(void)
{
    unsigned failures = 0u;

    if (!run_level1_right_trace())
    {
        ++failures;
    }
    else
    {
        printf("PASS checkpoint trace level1_right_run\n");
    }

    if (!run_level2_room_chain_trace())
    {
        ++failures;
    }
    else
    {
        printf("PASS checkpoint trace level2_room_chain\n");
    }

    if (failures != 0u)
    {
        return 1;
    }

    return 0;
}
