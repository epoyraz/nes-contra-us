#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "contra/buttons.h"
#include "contra/core.h"

#define CHECK(condition) \
    do \
    { \
        if (!(condition)) \
        { \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); \
            return false; \
        } \
    } while (0)

typedef struct TestCase
{
    const char *name;
    bool (*run)(void);
} TestCase;

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

static bool framebuffer_region_has_detail(
    const uint32_t *framebuffer,
    unsigned start_x,
    unsigned start_y,
    unsigned width,
    unsigned height,
    unsigned min_unique_colors
)
{
    uint32_t seen[64] = {0u};
    unsigned seen_count = 0u;
    unsigned y;

    for (y = start_y; y < (start_y + height); y += 4u)
    {
        unsigned x;

        for (x = start_x; x < (start_x + width); x += 4u)
        {
            const uint32_t color = framebuffer[(size_t)y * CONTRA_FRAMEBUFFER_WIDTH + (size_t)x];
            bool known = false;
            unsigned index;

            for (index = 0u; index < seen_count; ++index)
            {
                if (seen[index] == color)
                {
                    known = true;
                    break;
                }
            }

            if (!known)
            {
                if (seen_count >= (sizeof(seen) / sizeof(seen[0])))
                {
                    return true;
                }
                seen[seen_count++] = color;
            }
        }
    }

    return seen_count >= min_unique_colors;
}

static bool run_until_gameplay(ContraCore *core, unsigned frame_limit)
{
    unsigned frame;

    for (frame = 0u; frame < frame_limit; ++frame)
    {
        uint8_t input = 0u;

        if ((frame == 5u) || (frame == 20u))
        {
            input = CONTRA_BUTTON_START;
        }

        step_with_input(core, input);
        if ((core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
            (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u) &&
            (core->ram[CONTRA_RAM_PLAYER_STATE] == 0x01u))
        {
            return true;
        }
    }

    return false;
}

static bool run_attract_until_level(ContraCore *core, uint8_t level, unsigned frame_limit)
{
    unsigned frame;

    for (frame = 0u; frame < frame_limit; ++frame)
    {
        step_no_input(core);
        if ((core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u) &&
            (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u) &&
            (core->ram[CONTRA_RAM_CURRENT_LEVEL] == level) &&
            (core->ram[CONTRA_RAM_PLAYER_STATE] == 0x01u))
        {
            return true;
        }
    }

    return false;
}

static bool advance_level2_room_once(ContraCore *core)
{
    unsigned frame;
    const uint8_t initial_screen = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    bool transition_started = false;

    for (frame = 0u; frame < 180u; ++frame)
    {
        step_no_input(core);
        if ((core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u) &&
            (core->ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u))
        {
            break;
        }
    }

    core->ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x01u;

    for (frame = 0u; frame < 420u; ++frame)
    {
        step_with_input(core, CONTRA_BUTTON_UP);
        transition_started = transition_started || (core->ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG] != 0u);

        if ((core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen) ||
            ((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u))
        {
            return transition_started;
        }
    }

    return false;
}

static void force_level2_gameplay(ContraCore *core)
{
    unsigned frame;

    contra_core_init(core);
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = 0x01u;
    core->ram[CONTRA_RAM_PLAYER_MODE_1D] = 0x01u;
    core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core->ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;
    core->ram[CONTRA_RAM_P2_NUM_LIVES] = 0x00u;

    for (frame = 0u; frame < 256u; ++frame)
    {
        step_no_input(core);
        if ((core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
            (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
        {
            return;
        }
    }
}

static bool test_title_start_reaches_level1_gameplay(void)
{
    ContraCore core;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));
    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u);
    CHECK(framebuffer_region_has_detail(contra_core_framebuffer(&core), 0u, 16u, 256u, 208u, 4u));
    return true;
}

static bool test_level1_scrolls_right_under_player_input(void)
{
    ContraCore core;
    unsigned frame;
    uint8_t initial_screen;
    uint8_t initial_scroll;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    initial_screen = core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    initial_scroll = core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];

    for (frame = 0u; frame < 260u; ++frame)
    {
        step_with_input(&core, CONTRA_BUTTON_RIGHT);
    }

    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    CHECK((core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen) ||
          (core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] != initial_scroll));
    CHECK(core.ram[CONTRA_RAM_HORIZONTAL_SCROLL] != 0u);
    return true;
}

static bool test_level1_spawns_native_enemies_while_scrolling(void)
{
    ContraCore core;
    unsigned frame;
    unsigned active_supported_enemies = 0u;
    bool enemy_data_loaded = false;
    bool enemy_seen = false;
    size_t index;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    for (frame = 0u; frame < 760u; ++frame)
    {
        step_with_input(&core, CONTRA_BUTTON_RIGHT);
        enemy_data_loaded = enemy_data_loaded || (core.ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] != 0u);

        for (index = 0u; index < CONTRA_NATIVE_MAX_ENEMIES; ++index)
        {
            if (core.enemies[index].active != 0u)
            {
                enemy_seen = true;
            }
        }
    }

    for (index = 0u; index < CONTRA_NATIVE_MAX_ENEMIES; ++index)
    {
        if (core.enemies[index].active != 0u)
        {
            ++active_supported_enemies;
        }
    }

    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != 0u);
    CHECK(enemy_data_loaded);
    CHECK(enemy_seen || (active_supported_enemies != 0u));
    return true;
}

static bool test_level1_bullet_destroyed_enemy_becomes_explosion(void)
{
    ContraCore core;

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x04u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] = 0x00u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x50u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0xA0u;
    core.ram[CONTRA_RAM_PLAYER_BULLET_SLOT] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = 0x70u;
    core.ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = 0x80u;
    core.enemies[0].active = 0x01u;
    core.enemies[0].type = 0x04u;
    core.enemies[0].hp = 0x01u;
    core.enemies[0].state = 0x02u;
    core.enemies[0].x = 0x70;
    core.enemies[0].y = 0x80;

    step_no_input(&core);

    CHECK(core.ram[CONTRA_RAM_PLAYER_BULLET_SLOT] == 0x00u);
    CHECK(core.enemies[0].active != 0u);
    CHECK(core.enemies[0].type == 0xFEu);
    return true;
}

static bool test_level1_forced_boss_clear_hands_off_to_level2(void)
{
    ContraCore core;
    unsigned frame;
    bool reached_level2 = false;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));
    core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;

    for (frame = 0u; frame < 900u; ++frame)
    {
        step_no_input(&core);
        if (core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u)
        {
            reached_level2 = true;
            break;
        }
    }

    CHECK(reached_level2);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] <= 0x02u);
    return true;
}

static bool test_attract_reaches_level2_gameplay(void)
{
    ContraCore core;
    unsigned frame;

    contra_core_init(&core);
    CHECK(run_attract_until_level(&core, 0x01u, 3800u));
    for (frame = 0u; frame < 16u; ++frame)
    {
        step_no_input(&core);
    }
    CHECK(core.ram[CONTRA_RAM_DEMO_MODE] != 0u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_Y_POS] < 0xE8u);
    return true;
}

static bool test_level2_indoor_floor_landing(void)
{
    ContraCore core;
    unsigned frame;
    bool landed = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u);

    for (frame = 0u; frame < 180u; ++frame)
    {
        step_no_input(&core);
        if ((core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u) &&
            (core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u) &&
            (core.ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u))
        {
            landed = true;
            break;
        }
    }

    CHECK(landed);
    CHECK(core.ram[CONTRA_RAM_SPRITE_Y_POS] == 0xA8u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    CHECK(framebuffer_region_has_detail(contra_core_framebuffer(&core), 0u, 16u, 256u, 208u, 2u));
    return true;
}

static bool test_level2_room_advance_after_clear_and_up(void)
{
    ContraCore core;
    unsigned frame;
    uint8_t initial_step;
    bool transition_started = false;
    bool transition_step_advanced = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    for (frame = 0u; frame < 180u; ++frame)
    {
        step_no_input(&core);
        if ((core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u) &&
            (core.ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u))
        {
            break;
        }
    }

    core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] = 0x01u;
    initial_step = core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];

    for (frame = 0u; frame < 320u; ++frame)
    {
        step_with_input(&core, CONTRA_BUTTON_UP);
        transition_started = transition_started || (core.ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG] != 0u);
        transition_step_advanced = transition_step_advanced ||
            (core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] != initial_step);

        if (transition_step_advanced)
        {
            break;
        }
    }

    CHECK(transition_started);
    CHECK(transition_step_advanced);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level2_up_before_clear_electrocutes_without_advancing(void)
{
    ContraCore core;
    unsigned frame;
    uint8_t initial_screen;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    for (frame = 0u; frame < 180u; ++frame)
    {
        step_no_input(&core);
        if ((core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u) &&
            (core.ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u))
        {
            break;
        }
    }

    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
    initial_screen = core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];

    step_with_input(&core, CONTRA_BUTTON_UP);

    CHECK(core.ram[CONTRA_RAM_ELECTROCUTED_TIMER] != 0x00u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_PLAYER_ADV_FLAG] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCROLL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == initial_screen);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_attract_level2_advances_indoor_room(void)
{
    ContraCore core;
    unsigned frame;
    bool reached_level2 = false;
    bool room_advanced = false;

    contra_core_init(&core);

    for (frame = 0u; frame < 5200u; ++frame)
    {
        step_no_input(&core);

        if ((core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u) &&
            (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u) &&
            (core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u))
        {
            reached_level2 = true;
            room_advanced = room_advanced ||
                (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != 0u);
        }

        if (room_advanced)
        {
            break;
        }
    }

    CHECK(reached_level2);
    CHECK(room_advanced);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level2_repeated_room_advances_reach_boss_state(void)
{
    ContraCore core;
    unsigned advance_count;
    bool reached_boss_state = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    for (advance_count = 0u; advance_count < 24u; ++advance_count)
    {
        CHECK(advance_level2_room_once(&core));
        if ((core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u)
        {
            reached_boss_state = true;
            break;
        }
    }

    CHECK(reached_boss_state);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == core.ram[CONTRA_RAM_LEVEL_STOP_SCROLL]);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

int main(void)
{
    const TestCase tests[] = {
        {"title_start_reaches_level1_gameplay", test_title_start_reaches_level1_gameplay},
        {"level1_scrolls_right_under_player_input", test_level1_scrolls_right_under_player_input},
        {"level1_spawns_native_enemies_while_scrolling", test_level1_spawns_native_enemies_while_scrolling},
        {"level1_bullet_destroyed_enemy_becomes_explosion", test_level1_bullet_destroyed_enemy_becomes_explosion},
        {"level1_forced_boss_clear_hands_off_to_level2", test_level1_forced_boss_clear_hands_off_to_level2},
        {"attract_reaches_level2_gameplay", test_attract_reaches_level2_gameplay},
        {"level2_indoor_floor_landing", test_level2_indoor_floor_landing},
        {"level2_room_advance_after_clear_and_up", test_level2_room_advance_after_clear_and_up},
        {"level2_up_before_clear_electrocutes_without_advancing", test_level2_up_before_clear_electrocutes_without_advancing},
        {"attract_level2_advances_indoor_room", test_attract_level2_advances_indoor_room},
        {"level2_repeated_room_advances_reach_boss_state", test_level2_repeated_room_advances_reach_boss_state}
    };
    const size_t test_count = sizeof(tests) / sizeof(tests[0]);
    size_t index;
    size_t failed = 0u;

    for (index = 0u; index < test_count; ++index)
    {
        const bool passed = tests[index].run();

        printf("%s %s\n", passed ? "PASS" : "FAIL", tests[index].name);
        if (!passed)
        {
            ++failed;
        }
    }

    if (failed != 0u)
    {
        printf("%u/%u tests failed\n", (unsigned)failed, (unsigned)test_count);
        return 1;
    }

    printf("%u tests passed\n", (unsigned)test_count);
    return 0;
}
