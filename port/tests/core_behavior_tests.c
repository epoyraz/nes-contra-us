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

static uint32_t hash_bytes(const void *data, size_t length)
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

static bool destroy_first_level2_wall_core(ContraCore *core);

static bool advance_level2_room_once(ContraCore *core)
{
    unsigned frame;
    const uint8_t initial_screen = core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    bool transition_started = false;

    core->ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
    for (frame = 0u; frame < 180u; ++frame)
    {
        step_no_input(core);
        if ((core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u) &&
            (core->ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u))
        {
            break;
        }
    }

    if (!destroy_first_level2_wall_core(core))
    {
        return false;
    }
    memset(core->enemy_projectiles, 0, sizeof(core->enemy_projectiles));

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

static bool find_first_active_wall_core(const ContraCore *core, size_t *wall_core_index)
{
    size_t index;

    for (index = 0u; index < CONTRA_NATIVE_MAX_ENEMIES; ++index)
    {
        if ((core->enemies[index].active != 0u) &&
            (core->enemies[index].type == 0x14u))
        {
            *wall_core_index = index;
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

static unsigned count_active_projectiles_from_owner(const ContraCore *core, uint8_t owner)
{
    unsigned count = 0u;
    size_t index;

    for (index = 0u; index < CONTRA_NATIVE_MAX_ENEMY_PROJECTILES; ++index)
    {
        if ((core->enemy_projectiles[index].active != 0u) &&
            (core->enemy_projectiles[index].owner == owner))
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

static bool destroy_first_level2_wall_core(ContraCore *core)
{
    unsigned frame;
    size_t wall_core_index = 0u;

    for (frame = 0u; frame < 320u; ++frame)
    {
        step_no_input(core);
        if (find_first_active_wall_core(core, &wall_core_index) &&
            (core->enemies[wall_core_index].state == 0x03u))
        {
            break;
        }
    }

    if ((frame == 320u) || !find_first_active_wall_core(core, &wall_core_index))
    {
        return false;
    }

    for (frame = 0u;
         (frame < 128u) &&
         (core->enemies[wall_core_index].active != 0u) &&
         (core->enemies[wall_core_index].type == 0x14u) &&
         (core->enemies[wall_core_index].hp != 0u);
         ++frame)
    {
        core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT] = 0x01u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = (uint8_t)core->enemies[wall_core_index].x;
        core->ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = (uint8_t)core->enemies[wall_core_index].y;
        step_no_input(core);
    }

    if ((frame == 128u) ||
        !find_first_active_wall_core(core, &wall_core_index) ||
        (core->enemies[wall_core_index].hp != 0u))
    {
        return false;
    }

    for (frame = 0u; frame < 96u; ++frame)
    {
        step_no_input(core);
        if (core->ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] != 0u)
        {
            return true;
        }
    }

    return false;
}

static bool reach_level2_boss_room(ContraCore *core)
{
    unsigned advance_count;

    force_level2_gameplay(core);
    if (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] != 0x04u)
    {
        return false;
    }

    for (advance_count = 0u; advance_count < 24u; ++advance_count)
    {
        if (!advance_level2_room_once(core))
        {
            return false;
        }

        if ((core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u)
        {
            step_no_input(core);
            return true;
        }
    }

    return false;
}

static bool shoot_enemy_until_removed_or_changed(ContraCore *core, uint8_t enemy_type, unsigned max_hits)
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

        core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT] = 0x01u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = (uint8_t)core->enemies[enemy_index].x;
        core->ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = (uint8_t)core->enemies[enemy_index].y;
        step_no_input(core);

        if (count_active_enemy_type(core, enemy_type) < initial_count)
        {
            return true;
        }
    }

    return count_active_enemy_type(core, enemy_type) < initial_count;
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

static bool test_level1_weapon_item_pickup_changes_weapon_and_bullet(void)
{
    ContraCore core;
    size_t bullet_index;
    bool flame_bullet_seen = false;

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x04u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] = 0x00u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x70u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0x90u;
    core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] = 0x00u;
    core.enemies[0].active = 0x01u;
    core.enemies[0].type = 0x00u;
    core.enemies[0].attrs = 0x02u;
    core.enemies[0].hp = 0x01u;
    core.enemies[0].x = 0x70;
    core.enemies[0].y = 0x90;

    step_no_input(&core);

    CHECK(core.enemies[0].active == 0x00u);
    CHECK((core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] & 0x0Fu) == 0x02u);

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] == 0x01u);

    step_with_input(&core, CONTRA_BUTTON_B);

    for (bullet_index = 0u; bullet_index < 16u; ++bullet_index)
    {
        if ((core.ram[CONTRA_RAM_PLAYER_BULLET_SLOT + bullet_index] == 0x03u) &&
            (core.ram[CONTRA_RAM_PLAYER_BULLET_OWNER + bullet_index] == 0x00u) &&
            (core.ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_index] == 0x22u))
        {
            flame_bullet_seen = true;
            break;
        }
    }

    CHECK(flame_bullet_seen);
    return true;
}

static bool test_level1_bridge_destruction_changes_render_state(void)
{
    ContraCore core;
    unsigned frame;
    uint32_t initial_overlay_hash;
    uint32_t active_overlay_hash;
    bool bridge_active = false;

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x04u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] = 0x00u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT] = 0x01u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x70u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0x90u;
    core.enemies[0].active = 0x01u;
    core.enemies[0].type = 0x12u;
    core.enemies[0].x = 0x70;
    core.enemies[0].y = 0x90;
    core.enemies[0].state = 0x00u;
    initial_overlay_hash =
        ((uint32_t)core.enemies[0].screen_id << 24u) |
        ((uint32_t)core.enemies[0].sprite_code << 16u) |
        ((uint32_t)core.enemies[0].sprite_attr << 8u) |
        (uint32_t)core.enemies[0].hp;

    for (frame = 0u; frame < 96u; ++frame)
    {
        step_no_input(&core);
        if (core.enemies[0].state == 0x02u)
        {
            bridge_active = true;
            break;
        }
    }

    CHECK(bridge_active);
    CHECK(core.enemies[0].active != 0x00u);
    CHECK(core.enemies[0].type == 0x12u);
    CHECK(core.enemies[0].screen_id == 0x1Bu);
    CHECK(core.enemies[0].sprite_code == 0x19u);
    CHECK(core.enemies[0].sprite_attr == 0x19u);
    CHECK(core.enemies[0].hp == 0x1Du);
    active_overlay_hash =
        ((uint32_t)core.enemies[0].screen_id << 24u) |
        ((uint32_t)core.enemies[0].sprite_code << 16u) |
        ((uint32_t)core.enemies[0].sprite_attr << 8u) |
        (uint32_t)core.enemies[0].hp;
    CHECK(active_overlay_hash != initial_overlay_hash);

    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT] = 0x00u;
    core.ram[CONTRA_RAM_EDGE_FALL_CODE] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x90u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0x90u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_EDGE_FALL_CODE] != 0x00u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level1_organic_bridge_load_changes_collision(void)
{
    ContraCore core;
    unsigned frame;
    size_t bridge_index = 0u;
    bool bridge_loaded = false;
    bool bridge_active = false;
    uint32_t initial_overlay_hash;
    uint32_t active_overlay_hash;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    for (frame = 0u; frame < 6000u; ++frame)
    {
        size_t enemy_index;

        core.ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER] = 0x80u;
        step_with_input(&core, CONTRA_BUTTON_RIGHT);

        for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
        {
            if ((core.enemies[enemy_index].active != 0u) &&
                (core.enemies[enemy_index].type == 0x12u))
            {
                bridge_index = enemy_index;
                bridge_loaded = true;
                break;
            }
        }

        if (bridge_loaded)
        {
            break;
        }
    }

    CHECK(bridge_loaded);
    initial_overlay_hash =
        ((uint32_t)core.enemies[bridge_index].screen_id << 24u) |
        ((uint32_t)core.enemies[bridge_index].sprite_code << 16u) |
        ((uint32_t)core.enemies[bridge_index].sprite_attr << 8u) |
        (uint32_t)core.enemies[bridge_index].hp;

    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_EDGE_FALL_CODE] = 0x00u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = (uint8_t)core.enemies[bridge_index].x;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = (uint8_t)core.enemies[bridge_index].y;

    for (frame = 0u; frame < 96u; ++frame)
    {
        step_no_input(&core);
        if (core.enemies[bridge_index].state == 0x02u)
        {
            bridge_active = true;
            break;
        }
    }

    CHECK(bridge_active);
    active_overlay_hash =
        ((uint32_t)core.enemies[bridge_index].screen_id << 24u) |
        ((uint32_t)core.enemies[bridge_index].sprite_code << 16u) |
        ((uint32_t)core.enemies[bridge_index].sprite_attr << 8u) |
        (uint32_t)core.enemies[bridge_index].hp;
    CHECK(active_overlay_hash != initial_overlay_hash);

    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT] = 0x00u;
    core.ram[CONTRA_RAM_EDGE_FALL_CODE] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = (uint8_t)(core.enemies[bridge_index].x + 0x20);
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = (uint8_t)core.enemies[bridge_index].y;
    step_no_input(&core);

    CHECK(core.ram[CONTRA_RAM_EDGE_FALL_CODE] != 0x00u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level1_forced_boss_clear_hands_off_to_level2(void)
{
    ContraCore core;
    unsigned frame;
    bool reached_level2 = false;
    bool reached_routine_08 = false;
    bool reached_routine_09 = false;
    bool reached_end_index_01 = false;
    bool reached_end_index_02 = false;
    bool saw_boss_flag_81 = false;
    bool saw_boss_flag_02 = false;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));
    core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;

    for (frame = 0u; frame < 1200u; ++frame)
    {
        step_no_input(&core);
        reached_routine_08 = reached_routine_08 || (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x08u);
        reached_routine_09 = reached_routine_09 || (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x09u);
        reached_end_index_01 = reached_end_index_01 || (core.ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] == 0x01u);
        reached_end_index_02 = reached_end_index_02 || (core.ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] == 0x02u);
        saw_boss_flag_81 = saw_boss_flag_81 || (core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0x81u);
        saw_boss_flag_02 = saw_boss_flag_02 || (core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0x02u);

        if (core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u)
        {
            reached_level2 = true;
            break;
        }
    }

    CHECK(reached_routine_08);
    CHECK(reached_routine_09);
    CHECK(reached_end_index_01);
    CHECK(reached_end_index_02);
    CHECK(saw_boss_flag_81);
    CHECK(saw_boss_flag_02);
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

static bool test_attract_level1_demo_reaches_screen_milestones(void)
{
    ContraCore core;
    unsigned frame;
    bool reached_screen_1 = false;
    bool consumed_demo_input = false;
    bool loaded_enemy_data = false;

    contra_core_init(&core);
    CHECK(run_attract_until_level(&core, 0x00u, 1200u));
    CHECK(core.ram[CONTRA_RAM_DEMO_MODE] != 0u);
    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x00u);

    for (frame = 0u; frame < 2200u; ++frame)
    {
        step_no_input(&core);
        consumed_demo_input = consumed_demo_input ||
            (core.ram[CONTRA_RAM_DEMO_INPUT_TBL_INDEX] != 0u);
        loaded_enemy_data = loaded_enemy_data ||
            (core.ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] != 0u);
        reached_screen_1 = reached_screen_1 ||
            (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] >= 0x01u);

        if (reached_screen_1 && loaded_enemy_data)
        {
            break;
        }

        CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u);
        CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u);
        CHECK(core.ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG] == 0x00u);
    }

    CHECK(consumed_demo_input);
    CHECK(loaded_enemy_data);
    CHECK(reached_screen_1);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    CHECK(framebuffer_region_has_detail(contra_core_framebuffer(&core), 0u, 16u, 256u, 208u, 4u));
    return true;
}

static bool test_level2_indoor_floor_landing(void)
{
    ContraCore core;
    unsigned frame;
    bool landed = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    core.ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER] = 0x00u;
    core.ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0x00u;
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

static bool test_level2_indoor_room_rendering_has_detail(void)
{
    ContraCore core;
    unsigned frame;
    uint32_t framebuffer_hash;
    bool landed = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
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
    framebuffer_hash = hash_bytes(core.framebuffer, sizeof(core.framebuffer));

    CHECK(framebuffer_region_has_detail(contra_core_framebuffer(&core), 0u, 16u, 256u, 208u, 2u));
    CHECK(framebuffer_hash != 0x00000000u);
    CHECK(framebuffer_hash != 0x811C9DC5u);
    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
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

static bool test_level2_wall_core_destroy_allows_room_advance(void)
{
    ContraCore core;
    unsigned frame;
    uint8_t initial_screen;
    bool room_advanced = false;
    size_t wall_core_index = 0u;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    step_no_input(&core);
    CHECK(find_first_active_wall_core(&core, &wall_core_index));
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
    CHECK(core.enemies[wall_core_index].hp == 0x08u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);

    CHECK(destroy_first_level2_wall_core(&core));
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_ELECTROCUTED_TIMER] == 0x00u);
    initial_screen = core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];

    for (frame = 0u; frame < 520u; ++frame)
    {
        step_with_input(&core, CONTRA_BUTTON_UP);
        if (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen)
        {
            room_advanced = true;
            break;
        }
    }

    CHECK(room_advanced);
    CHECK(core.ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level2_wall_core_destroy_updates_back_wall_quadrants(void)
{
    ContraCore core;
    unsigned frame;
    size_t wall_core_index = 0u;
    uint32_t initial_framebuffer_hash;
    bool saw_quadrant_update = false;

    force_level2_gameplay(&core);
    core.ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;
    for (frame = 0u; frame < 320u; ++frame)
    {
        step_no_input(&core);
        if (find_first_active_wall_core(&core, &wall_core_index) &&
            (core.enemies[wall_core_index].state == 0x03u))
        {
            break;
        }
    }

    CHECK(frame < 320u);
    CHECK(core.enemies[wall_core_index].hp == 0x08u);
    while (core.enemies[wall_core_index].hp != 0u)
    {
        core.ram[CONTRA_RAM_PLAYER_BULLET_SLOT] = 0x01u;
        core.ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = (uint8_t)core.enemies[wall_core_index].x;
        core.ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = (uint8_t)core.enemies[wall_core_index].y;
        step_no_input(&core);
    }

    CHECK(core.enemies[wall_core_index].active != 0u);
    CHECK(core.enemies[wall_core_index].type == 0x14u);
    CHECK(core.enemies[wall_core_index].state == 0x08u);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
    CHECK(core.enemies[wall_core_index].flags == 0x00u);
    initial_framebuffer_hash = hash_bytes(core.framebuffer, sizeof(core.framebuffer));

    for (frame = 0u; frame < 24u; ++frame)
    {
        step_no_input(&core);
        if (core.enemies[wall_core_index].flags != 0x00u)
        {
            saw_quadrant_update = true;
            break;
        }
    }

    CHECK(saw_quadrant_update);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
    CHECK(hash_bytes(core.framebuffer, sizeof(core.framebuffer)) != initial_framebuffer_hash);

    for (frame = 0u; frame < 96u; ++frame)
    {
        step_no_input(&core);
        if (core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] != 0u)
        {
            return true;
        }
    }

    return false;
}

static bool test_level2_soldier_generator_uses_scripted_attack_rounds(void)
{
    ContraCore core;
    unsigned frame;
    bool saw_jumping_soldier = false;
    bool saw_running_soldier = false;
    bool attack_round_incremented = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    step_no_input(&core);
    CHECK(count_active_enemy_type(&core, 0x19u) == 1u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] == 0x00u);

    for (frame = 0u; frame < 180u; ++frame)
    {
        step_no_input(&core);
        saw_jumping_soldier = saw_jumping_soldier || (count_active_enemy_type(&core, 0x16u) != 0u);
        saw_running_soldier = saw_running_soldier || (count_active_enemy_type(&core, 0x15u) != 0u);
        attack_round_incremented = attack_round_incremented ||
            (core.ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] != 0u);

        if (saw_jumping_soldier && saw_running_soldier && attack_round_incremented)
        {
            break;
        }
    }

    CHECK(saw_jumping_soldier);
    CHECK(saw_running_soldier);
    CHECK(attack_round_incremented);
    CHECK(core.ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level2_generated_soldiers_fire_projectiles(void)
{
    ContraCore core;
    unsigned frame;
    bool fired_projectile = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    step_no_input(&core);

    for (frame = 0u; frame < 260u; ++frame)
    {
        step_no_input(&core);
        fired_projectile = fired_projectile ||
            (count_active_projectiles_from_owner(&core, 0x15u) != 0u) ||
            (count_active_projectiles_from_owner(&core, 0x16u) != 0u);

        if (fired_projectile)
        {
            break;
        }
    }

    CHECK(fired_projectile);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level2_roller_generator_spawns_roller_row(void)
{
    ContraCore core;
    unsigned advance_count;
    unsigned frame;
    bool spawned_rollers = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    for (advance_count = 0u; advance_count < 3u; ++advance_count)
    {
        CHECK(advance_level2_room_once(&core));
    }

    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x03u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u);
    CHECK(count_active_enemy_type(&core, 0x1Au) == 1u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);

    for (frame = 0u; frame < 220u; ++frame)
    {
        step_no_input(&core);
        if (count_active_enemy_type(&core, 0x18u) >= 4u)
        {
            spawned_rollers = true;
            break;
        }
    }

    CHECK(spawned_rollers);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x03u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level2_projectiles_move_and_hit_player(void)
{
    ContraCore core;
    int16_t initial_x;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    core.enemy_projectiles[0].active = 0x01u;
    core.enemy_projectiles[0].damage = 0x01u;
    core.enemy_projectiles[0].sprite_code = 0x1Eu;
    core.enemy_projectiles[0].sprite_attr = 0x03u;
    core.enemy_projectiles[0].owner = 0x15u;
    core.enemy_projectiles[0].timer = 0x20u;
    core.enemy_projectiles[0].x = 0x20;
    core.enemy_projectiles[0].y = 0x40;
    core.enemy_projectiles[0].vx = 2;
    core.enemy_projectiles[0].vy = 0;
    initial_x = core.enemy_projectiles[0].x;

    step_no_input(&core);
    CHECK(core.enemy_projectiles[0].active != 0u);
    CHECK(core.enemy_projectiles[0].x == (int16_t)(initial_x + 2));

    memset(core.enemy_projectiles, 0, sizeof(core.enemy_projectiles));
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_HIDDEN] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER] = 0x00u;
    core.ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0x00u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x80u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0x78u;
    core.enemy_projectiles[0].active = 0x01u;
    core.enemy_projectiles[0].damage = 0x01u;
    core.enemy_projectiles[0].sprite_code = 0x1Eu;
    core.enemy_projectiles[0].sprite_attr = 0x03u;
    core.enemy_projectiles[0].owner = 0x15u;
    core.enemy_projectiles[0].timer = 0x20u;
    core.enemy_projectiles[0].x = (int16_t)core.ram[CONTRA_RAM_SPRITE_X_POS];
    core.enemy_projectiles[0].y = (int16_t)core.ram[CONTRA_RAM_SPRITE_Y_POS];
    core.enemy_projectiles[0].vx = 0;
    core.enemy_projectiles[0].vy = 0;

    step_no_input(&core);
    CHECK(core.enemy_projectiles[0].active == 0u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_DEATH_FLAG] == 0x01u);
    return true;
}

static bool test_level2_room_advance_changes_render_state(void)
{
    ContraCore core;
    unsigned frame;
    uint8_t initial_screen;
    uint32_t initial_supertiles_hash;
    uint32_t initial_framebuffer_hash;
    bool room_advanced = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    CHECK(destroy_first_level2_wall_core(&core));
    CHECK(framebuffer_region_has_detail(contra_core_framebuffer(&core), 0u, 16u, 256u, 208u, 2u));
    initial_screen = core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    initial_supertiles_hash = hash_bytes(core.level_screen_supertiles, sizeof(core.level_screen_supertiles));
    initial_framebuffer_hash = hash_bytes(core.framebuffer, sizeof(core.framebuffer));

    for (frame = 0u; frame < 520u; ++frame)
    {
        step_with_input(&core, CONTRA_BUTTON_UP);
        if (core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen)
        {
            room_advanced = true;
            break;
        }
    }

    CHECK(room_advanced);
    CHECK(hash_bytes(core.level_screen_supertiles, sizeof(core.level_screen_supertiles)) != initial_supertiles_hash);
    CHECK(hash_bytes(core.framebuffer, sizeof(core.framebuffer)) != initial_framebuffer_hash);
    CHECK(framebuffer_region_has_detail(contra_core_framebuffer(&core), 0u, 16u, 256u, 208u, 2u));
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_attract_level2_loads_wall_core_without_early_clear(void)
{
    ContraCore core;
    unsigned frame;
    bool reached_level2 = false;
    bool wall_core_loaded = false;
    size_t wall_core_index = 0u;

    contra_core_init(&core);

    for (frame = 0u; frame < 5200u; ++frame)
    {
        step_no_input(&core);

        if ((core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u) &&
            (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u) &&
            (core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u))
        {
            reached_level2 = true;
            wall_core_loaded = wall_core_loaded ||
                find_first_active_wall_core(&core, &wall_core_index);
        }

        if (wall_core_loaded)
        {
            break;
        }
    }

    CHECK(reached_level2);
    CHECK(wall_core_loaded);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
    CHECK(core.enemies[wall_core_index].type == 0x14u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_attract_level2_demo_does_not_consume_multiple_lives_before_rom_terminal(void)
{
    ContraCore core;
    unsigned frame;
    const unsigned original_terminal_frame = 4632u;
    bool terminal_reached = false;

    contra_core_init(&core);
    for (frame = 0u; frame < (original_terminal_frame + 300u); ++frame)
    {
        step_no_input(&core);
        if ((frame > 3000u) && (core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] != 0x02u))
        {
            terminal_reached = true;
            break;
        }
    }

    CHECK(terminal_reached);
    CHECK(frame <= (original_terminal_frame + 300u));
    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_P1_NUM_LIVES] >= 0x61u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
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
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == core.ram[CONTRA_RAM_LEVEL_STOP_SCROLL]);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level2_boss_room_loads_rom_enemy_data(void)
{
    ContraCore core;

    CHECK(reach_level2_boss_room(&core));

    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x05u);
    CHECK(count_active_enemy_type(&core, 0x10u) == 1u);
    CHECK(count_active_enemy_type(&core, 0x08u) == 2u);
    CHECK(count_active_enemy_type(&core, 0x0Au) == 4u);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);
    return true;
}

static bool test_level2_boss_room_plating_and_eye_can_be_destroyed(void)
{
    ContraCore core;
    unsigned frame;

    CHECK(reach_level2_boss_room(&core));
    CHECK(count_active_enemy_type(&core, 0x0Au) == 4u);
    CHECK(count_active_enemy_type(&core, 0x10u) == 1u);
    CHECK(core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] == 0x00u);

    while (count_active_enemy_type(&core, 0x0Au) != 0u)
    {
        CHECK(shoot_enemy_until_removed_or_changed(&core, 0x0Au, 8u));
    }

    CHECK(core.ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] == 0x04u);
    CHECK(count_active_enemy_type(&core, 0x08u) == 2u);
    CHECK(count_active_enemy_type(&core, 0x10u) == 1u);

    CHECK(shoot_enemy_until_removed_or_changed(&core, 0x10u, 48u));
    CHECK(count_active_enemy_type(&core, 0x10u) == 0u);
    CHECK(core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0x00u);

    for (frame = 0u; frame < 16u; ++frame)
    {
        step_no_input(&core);
        if (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x08u)
        {
            return true;
        }
    }

    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x08u);
    return true;
}

static bool test_level2_boss_room_wall_cannons_fire_projectiles(void)
{
    ContraCore core;
    unsigned frame;
    bool fired_projectile = false;

    CHECK(reach_level2_boss_room(&core));
    CHECK(count_active_enemy_type(&core, 0x08u) == 2u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] == 0x00u);

    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x80u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0x90u;
    core.ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0xFFu;

    for (frame = 0u; frame < 240u; ++frame)
    {
        step_no_input(&core);
        fired_projectile = fired_projectile ||
            (count_active_projectiles_from_owner(&core, 0x08u) != 0u);

        if (fired_projectile)
        {
            break;
        }
    }

    CHECK(fired_projectile);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u);

    memset(core.enemy_projectiles, 0, sizeof(core.enemy_projectiles));
    core.ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0x00u;
    core.enemy_projectiles[0].active = 0x01u;
    core.enemy_projectiles[0].damage = 0x01u;
    core.enemy_projectiles[0].sprite_code = 0x1Eu;
    core.enemy_projectiles[0].sprite_attr = 0x03u;
    core.enemy_projectiles[0].owner = 0x08u;
    core.enemy_projectiles[0].timer = 0x20u;
    core.enemy_projectiles[0].x = (int16_t)core.ram[CONTRA_RAM_SPRITE_X_POS];
    core.enemy_projectiles[0].y = (int16_t)core.ram[CONTRA_RAM_SPRITE_Y_POS];
    core.enemy_projectiles[0].vx = 0;
    core.enemy_projectiles[0].vy = 0;

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x02u);
    CHECK(core.enemy_projectiles[0].active == 0u);
    return true;
}

int main(void)
{
    const TestCase tests[] = {
        {"title_start_reaches_level1_gameplay", test_title_start_reaches_level1_gameplay},
        {"level1_scrolls_right_under_player_input", test_level1_scrolls_right_under_player_input},
        {"level1_spawns_native_enemies_while_scrolling", test_level1_spawns_native_enemies_while_scrolling},
        {"level1_bullet_destroyed_enemy_becomes_explosion", test_level1_bullet_destroyed_enemy_becomes_explosion},
        {"level1_weapon_item_pickup_changes_weapon_and_bullet", test_level1_weapon_item_pickup_changes_weapon_and_bullet},
        {"level1_bridge_destruction_reaches_overlay_state", test_level1_bridge_destruction_changes_render_state},
        {"level1_organic_bridge_load_changes_collision", test_level1_organic_bridge_load_changes_collision},
        {"level1_forced_boss_clear_hands_off_to_level2", test_level1_forced_boss_clear_hands_off_to_level2},
        {"attract_reaches_level2_gameplay", test_attract_reaches_level2_gameplay},
        {"attract_level1_demo_reaches_screen_milestones", test_attract_level1_demo_reaches_screen_milestones},
        {"level2_indoor_floor_landing", test_level2_indoor_floor_landing},
        {"level2_indoor_room_rendering_has_detail", test_level2_indoor_room_rendering_has_detail},
        {"level2_room_advance_after_clear_and_up", test_level2_room_advance_after_clear_and_up},
        {"level2_up_before_clear_electrocutes_without_advancing", test_level2_up_before_clear_electrocutes_without_advancing},
        {"level2_wall_core_destroy_allows_room_advance", test_level2_wall_core_destroy_allows_room_advance},
        {"level2_wall_core_destroy_updates_back_wall_quadrants", test_level2_wall_core_destroy_updates_back_wall_quadrants},
        {"level2_soldier_generator_uses_scripted_attack_rounds", test_level2_soldier_generator_uses_scripted_attack_rounds},
        {"level2_generated_soldiers_fire_projectiles", test_level2_generated_soldiers_fire_projectiles},
        {"level2_roller_generator_spawns_roller_row", test_level2_roller_generator_spawns_roller_row},
        {"level2_projectiles_move_and_hit_player", test_level2_projectiles_move_and_hit_player},
        {"level2_room_advance_changes_render_state", test_level2_room_advance_changes_render_state},
        {"attract_level2_loads_wall_core_without_early_clear", test_attract_level2_loads_wall_core_without_early_clear},
        {"attract_level2_demo_does_not_consume_multiple_lives_before_rom_terminal", test_attract_level2_demo_does_not_consume_multiple_lives_before_rom_terminal},
        {"level2_repeated_room_advances_reach_boss_state", test_level2_repeated_room_advances_reach_boss_state},
        {"level2_boss_room_loads_rom_enemy_data", test_level2_boss_room_loads_rom_enemy_data},
        {"level2_boss_room_plating_and_eye_can_be_destroyed", test_level2_boss_room_plating_and_eye_can_be_destroyed},
        {"level2_boss_room_wall_cannons_fire_projectiles", test_level2_boss_room_wall_cannons_fire_projectiles}
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
