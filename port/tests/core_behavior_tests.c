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

enum
{
    TEST_PLAYER_BULLET_COUNT = 16u
};

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

static bool nametable_has_text(const ContraCore *core, uint16_t ppu_addr, const uint8_t *text, size_t length)
{
    size_t index;
    const size_t start = (size_t)((ppu_addr - 0x2000u) & (CONTRA_PPU_NAMETABLE_SIZE - 1u));

    for (index = 0u; index < length; ++index)
    {
        const size_t nametable_index = (start + index) & (CONTRA_PPU_NAMETABLE_SIZE - 1u);

        if (core->ppu_nametable[nametable_index] != text[index])
        {
            return false;
        }
    }

    return true;
}

static bool run_until_gameplay(ContraCore *core, unsigned frame_limit)
{
    unsigned frame;

    for (frame = 0u; frame < frame_limit; ++frame)
    {
        uint8_t input = 0u;

        if ((frame == 5u) || (frame == 20u) || (frame == 35u))
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

static void clear_player_bullets(ContraCore *core)
{
    size_t index;

    for (index = 0u; index < TEST_PLAYER_BULLET_COUNT; ++index)
    {
        core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT + index] = 0x00u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + index] = 0x00u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_ATTR + index] = 0x00u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + index] = 0x00u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_OWNER + index] = 0x00u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_TIMER + index] = 0x00u;
        core->ram[CONTRA_RAM_PLAYER_BULLET_DIST + index] = 0x00u;
    }
}

static unsigned count_player_bullets_with_slot(const ContraCore *core, uint8_t slot)
{
    unsigned count = 0u;
    size_t index;

    for (index = 0u; index < TEST_PLAYER_BULLET_COUNT; ++index)
    {
        if ((core->ram[CONTRA_RAM_PLAYER_BULLET_SLOT + index] & 0x0Fu) == slot)
        {
            ++count;
        }
    }

    return count;
}

static void prepare_level1_weapon_state(ContraCore *core)
{
    clear_player_bullets(core);
    memset(core->enemy_projectiles, 0, sizeof(core->enemy_projectiles));
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x04u;
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x00u;
    core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] = 0x00u;
    core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core->ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core->ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core->ram[CONTRA_RAM_PLAYER_HIDDEN] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS] = 0x00u;
    core->ram[CONTRA_RAM_PLAYER_WATER_STATE] = 0x00u;
    core->ram[CONTRA_RAM_ELECTROCUTED_TIMER] = 0x00u;
    core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER] = 0x80u;
    core->ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0x80u;
    core->ram[CONTRA_RAM_SPRITE_X_POS] = 0x80u;
    core->ram[CONTRA_RAM_SPRITE_Y_POS] = 0x80u;
    core->ram[CONTRA_RAM_PLAYER_AIM_DIR] = 0x02u;
    core->ram[CONTRA_RAM_PLAYER_AIM_PREV_FRAME] = 0x02u;
}

static void prepare_level1_enemy_matrix_state(ContraCore *core)
{
    prepare_level1_weapon_state(core);
    memset(core->enemies, 0, sizeof(core->enemies));
    memset(core->enemy_projectiles, 0, sizeof(core->enemy_projectiles));
    core->ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER] = 0x00u;
    core->ram[CONTRA_RAM_INVINCIBILITY_TIMER] = 0x80u;
    core->ram[CONTRA_RAM_SPRITE_X_POS] = 0x80u;
    core->ram[CONTRA_RAM_SPRITE_Y_POS] = 0x80u;
}

static ContraNativeEnemy *seed_enemy(
    ContraCore *core,
    size_t index,
    uint8_t type,
    uint8_t state,
    int16_t x,
    int16_t y
)
{
    ContraNativeEnemy *const enemy = &core->enemies[index];

    memset(enemy, 0, sizeof(*enemy));
    enemy->active = 0x01u;
    enemy->type = type;
    enemy->state = state;
    enemy->hp = 0x01u;
    enemy->x = x;
    enemy->y = y;
    return enemy;
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

static bool test_level1_generated_soldier_spawns_on_snapped_floor(void)
{
    ContraCore core;
    size_t enemy_index;
    bool soldier_found = false;
    size_t soldier_index = 0u;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    memset(core.enemies, 0, sizeof(core.enemies));
    memset(core.enemy_projectiles, 0, sizeof(core.enemy_projectiles));
    core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x01u;
    core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x00u;
    core.ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
    core.ram[CONTRA_RAM_SOLDIER_GEN_SCREEN] = 0x01u;
    core.ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS] = 0x00u;
    core.ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] = 0x01u;
    core.ram[CONTRA_RAM_FRAME_COUNTER] = 0x01u;
    core.ram[CONTRA_RAM_RANDOM_NUM] = 0x00u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x80u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0xB4u;

    step_no_input(&core);

    for (enemy_index = 0u; enemy_index < CONTRA_NATIVE_MAX_ENEMIES; ++enemy_index)
    {
        if ((core.enemies[enemy_index].active != 0u) &&
            (core.enemies[enemy_index].type == 0x05u) &&
            (core.enemies[enemy_index].x == 0xFC))
        {
            soldier_found = true;
            soldier_index = enemy_index;
            break;
        }
    }

    CHECK(soldier_found);
    CHECK(((uint8_t)core.enemies[soldier_index].y & 0x0Fu) == 0x04u);
    return true;
}

static bool test_level1_rifle_man_stays_seated_on_floor_after_y_drift(void)
{
    ContraCore core;
    ContraNativeEnemy *enemy;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    memset(core.enemies, 0, sizeof(core.enemies));
    memset(core.enemy_projectiles, 0, sizeof(core.enemy_projectiles));
    core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x10u;
    core.ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
    core.ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER] = 0xFFu;
    core.ram[CONTRA_RAM_FRAME_SCROLL] = 0x00u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x80u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0x64u;

    enemy = seed_enemy(&core, 0u, 0x06u, 0x02u, 0x80, 0x69);
    enemy->attrs = 0x00u;
    enemy->sprite_code = 0x43u;

    step_no_input(&core);

    CHECK(core.enemies[0].active != 0u);
    CHECK(core.enemies[0].type == 0x06u);
    CHECK(((uint8_t)core.enemies[0].y & 0x0Fu) == 0x04u);
    return true;
}

static bool test_level1_red_turret_bullet_uses_rom_muzzle_y_offset(void)
{
    ContraCore core;
    ContraNativeEnemy *enemy;
    size_t projectile_index;
    bool projectile_found = false;
    int16_t projectile_y = 0;

    contra_core_init(&core);
    prepare_level1_enemy_matrix_state(&core);
    core.ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] = 0x01u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x40u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0x7Cu;

    enemy = seed_enemy(&core, 0u, 0x07u, 0x02u, 0x7C, 0x7C);
    enemy->attrs = 0x00u;
    enemy->cooldown = 0x00u;
    enemy->flags = 0x03u;

    step_no_input(&core);

    for (projectile_index = 0u; projectile_index < CONTRA_NATIVE_MAX_ENEMY_PROJECTILES; ++projectile_index)
    {
        if ((core.enemy_projectiles[projectile_index].active != 0u) &&
            (core.enemy_projectiles[projectile_index].owner == 0x07u))
        {
            projectile_found = true;
            projectile_y = core.enemy_projectiles[projectile_index].y;
            break;
        }
    }

    CHECK(projectile_found);
    CHECK(projectile_y == 0x7C);
    return true;
}

static bool test_level1_boss_bomb_turret_uses_rom_wall_frame_and_muzzle(void)
{
    ContraCore core;
    ContraNativeEnemy *enemy;
    size_t projectile_index;
    bool projectile_found = false;

    contra_core_init(&core);
    prepare_level1_enemy_matrix_state(&core);
    core.ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] = 0x01u;
    core.ram[CONTRA_RAM_RANDOM_NUM] = 0x00u;
    core.ram[CONTRA_RAM_FRAME_SCROLL] = 0x00u;

    enemy = seed_enemy(&core, 0u, 0x10u, 0x02u, 0xC0, 0x80);
    enemy->attrs = 0x00u;
    enemy->cooldown = 0x00u;
    enemy->flags = 0x00u;
    enemy->hp = 0x08u;
    enemy->sprite_code = 0x26u;

    step_no_input(&core);

    CHECK(core.enemies[0].active != 0u);
    CHECK(core.enemies[0].sprite_code == 0x2Au);

    for (projectile_index = 0u; projectile_index < CONTRA_NATIVE_MAX_ENEMY_PROJECTILES; ++projectile_index)
    {
        const ContraNativeProjectile *const projectile = &core.enemy_projectiles[projectile_index];

        if ((projectile->active != 0u) &&
            (projectile->owner == 0x10u))
        {
            projectile_found = true;
            CHECK(projectile->sprite_code == 0x21u);
            CHECK(projectile->sprite_attr == 0x02u);
            CHECK(projectile->x == (core.enemies[0].x - 10));
            CHECK(projectile->y == (core.enemies[0].y - 1));
            break;
        }
    }

    CHECK(projectile_found);
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

static bool test_attract_level1_matches_rom_startup_timing(void)
{
    ContraCore core;
    unsigned frame;

    contra_core_init(&core);

    for (frame = 1u; frame <= 3u; ++frame)
    {
        step_no_input(&core);
        CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x00u);
        CHECK(core.ram[CONTRA_RAM_FRAME_COUNTER] == 0x00u);
        CHECK(core.ram[CONTRA_RAM_DEMO_MODE] == 0x00u);
        CHECK(core.ram[CONTRA_RAM_HORIZONTAL_SCROLL] == 0x00u);
    }

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_FRAME_COUNTER] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_DEMO_MODE] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_HORIZONTAL_SCROLL] == 0x00u);

    for (frame = 5u; frame <= 9u; ++frame)
    {
        step_no_input(&core);
        CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x00u);
        CHECK(core.ram[CONTRA_RAM_FRAME_COUNTER] == 0x01u);
        CHECK(core.ram[CONTRA_RAM_DEMO_MODE] == 0x01u);
        CHECK(core.ram[CONTRA_RAM_HORIZONTAL_SCROLL] == 0x00u);
    }

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_FRAME_COUNTER] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_DEMO_MODE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_HORIZONTAL_SCROLL] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] == 0x80u);
    CHECK(core.ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] == 0x02u);
    return true;
}

static bool test_attract_level1_matches_rom_graphics_load_stall(void)
{
    ContraCore core;
    unsigned frame;

    contra_core_init(&core);

    for (frame = 1u; frame <= 653u; ++frame)
    {
        step_no_input(&core);
    }

    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_FRAME_COUNTER] == 0x03u);
    CHECK(core.ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_VERTICAL_SCROLL] == 0x00u);

    for (frame = 654u; frame <= 661u; ++frame)
    {
        step_no_input(&core);
        CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u);
        CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x02u);
        CHECK(core.ram[CONTRA_RAM_FRAME_COUNTER] == 0x04u);
        CHECK(core.ram[CONTRA_RAM_VERTICAL_SCROLL] == 0x00u);
    }

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x03u);
    CHECK(core.ram[CONTRA_RAM_FRAME_COUNTER] == 0x04u);
    CHECK(core.ram[CONTRA_RAM_VERTICAL_SCROLL] == 0xE0u);
    return true;
}

static bool test_attract_level1_matches_rom_gameplay_entry_frame(void)
{
    ContraCore core;
    unsigned frame;

    contra_core_init(&core);

    for (frame = 1u; frame <= 709u; ++frame)
    {
        step_no_input(&core);
        CHECK(!((core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u) &&
                (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u)));
    }

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_FRAME_COUNTER] == 0x34u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_STATE] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_Y_POS] == 0x00u);
    return true;
}

static bool test_attract_level1_matches_rom_first_scroll_frame(void)
{
    ContraCore core;
    unsigned frame;

    contra_core_init(&core);

    for (frame = 1u; frame <= 854u; ++frame)
    {
        step_no_input(&core);
    }

    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_HORIZONTAL_SCROLL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_FRAME_SCROLL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS] == 0x62u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS + 1u] == 0xAFu);

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_HORIZONTAL_SCROLL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_FRAME_SCROLL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS] == 0x63u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS + 1u] == 0xB0u);

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_HORIZONTAL_SCROLL] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_FRAME_SCROLL] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_X_VELOCITY + 1u] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS] == 0x63u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS + 1u] == 0xB0u);
    return true;
}

static bool test_attract_level1_blocks_p2_scroll_when_p1_at_left_edge(void)
{
    ContraCore core;
    unsigned frame;

    contra_core_init(&core);

    for (frame = 1u; frame <= 1295u; ++frame)
    {
        step_no_input(&core);
    }

    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] == 0x96u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS] == 0x20u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS + 1u] == 0xB0u);

    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] == 0x96u);
    CHECK(core.ram[CONTRA_RAM_FRAME_SCROLL] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_X_VELOCITY + 1u] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS] == 0x20u);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS + 1u] == 0xB0u);
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
    uint32_t initial_framebuffer_hash;
    bool room_advanced = false;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    CHECK(destroy_first_level2_wall_core(&core));
    CHECK(framebuffer_region_has_detail(contra_core_framebuffer(&core), 0u, 16u, 256u, 208u, 2u));
    initial_screen = core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
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

static bool test_game_over_delay_expiry_loads_screen_without_glitch(void)
{
    static const uint8_t game_over_text[9] = {0x47u, 0x41u, 0x4Du, 0x45u, 0x00u, 0x4Fu, 0x56u, 0x45u, 0x52u};
    static const uint8_t continue_text[8] = {0x43u, 0x4Fu, 0x4Eu, 0x54u, 0x49u, 0x4Eu, 0x55u, 0x45u};
    static const uint8_t end_text[3] = {0x45u, 0x4Eu, 0x44u};
    ContraCore core;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x0Au;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x00u;
    core.ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] = 0x01u;
    core.ram[CONTRA_RAM_NUM_CONTINUES] = 0x01u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.enemies[0].active = 0x01u;
    core.enemies[0].type = 0x10u;
    core.enemies[0].sprite_code = 0x29u;
    core.enemies[0].x = 0x80;
    core.enemies[0].y = 0x80;
    core.enemy_projectiles[0].active = 0x01u;
    core.enemy_projectiles[0].owner = 0x10u;
    core.enemy_projectiles[0].sprite_code = 0x21u;
    core.enemy_projectiles[0].x = 0x80;
    core.enemy_projectiles[0].y = 0x80;

    step_no_input(&core);

    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x06u);
    CHECK(core.ram[CONTRA_RAM_NUM_CONTINUES] == 0x00u);
    CHECK(count_active_enemy_type(&core, 0x10u) == 0u);
    CHECK(count_active_projectiles_from_owner(&core, 0x10u) == 0u);
    CHECK(nametable_has_text(&core, 0x222Au, game_over_text, sizeof(game_over_text)));
    CHECK(nametable_has_text(&core, 0x228Cu, continue_text, sizeof(continue_text)));
    CHECK(nametable_has_text(&core, 0x22CCu, end_text, sizeof(end_text)));
    CHECK(framebuffer_region_has_detail(contra_core_framebuffer(&core), 72u, 128u, 112u, 40u, 2u));
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

static bool test_broad_weapon_gameover_and_alt_graphics_matrix(void)
{
    ContraCore core;
    uint32_t initial_pattern_hash;
    bool laser_visible = false;
    bool alt_graphics_finished = false;
    unsigned frame;
    size_t index;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] = 0x01u;
    for (frame = 0u; frame < 12u; ++frame)
    {
        step_with_input(&core, CONTRA_BUTTON_B);
    }
    CHECK(count_player_bullets_with_slot(&core, 0x02u) != 0u);
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_PLAYER_M_WEAPON_FIRE_TIME] != 0u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] = 0x12u;
    core.ram[CONTRA_RAM_PLAYER_AIM_DIR] = 0x02u;
    step_with_input(&core, CONTRA_BUTTON_B);
    CHECK(count_player_bullets_with_slot(&core, 0x03u) == 1u);
    for (frame = 0u; frame < 4u; ++frame)
    {
        step_no_input(&core);
    }
    CHECK(core.ram[CONTRA_RAM_PLAYER_BULLET_TIMER] != 0u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] = 0x02u;
    core.ram[CONTRA_RAM_PLAYER_AIM_DIR] = 0x06u;
    step_with_input(&core, CONTRA_BUTTON_B);
    CHECK(count_player_bullets_with_slot(&core, 0x03u) == 1u);
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_PLAYER_BULLET_TIMER] != 0u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] = 0x13u;
    core.ram[CONTRA_RAM_PLAYER_AIM_DIR] = 0x02u;
    step_with_input(&core, CONTRA_BUTTON_B);
    CHECK(count_player_bullets_with_slot(&core, 0x04u) >= 5u);
    for (frame = 0u; frame < 34u; ++frame)
    {
        step_no_input(&core);
    }
    CHECK(core.ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE] >= 0x20u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] = 0x04u;
    core.ram[CONTRA_RAM_PLAYER_AIM_DIR] = 0x02u;
    step_with_input(&core, CONTRA_BUTTON_B);
    CHECK(count_player_bullets_with_slot(&core, 0x05u) >= 2u);
    for (frame = 0u; frame < 12u; ++frame)
    {
        step_no_input(&core);
    }
    for (index = 0u; index < TEST_PLAYER_BULLET_COUNT; ++index)
    {
        if (((core.ram[CONTRA_RAM_PLAYER_BULLET_SLOT + index] & 0x0Fu) == 0x05u) &&
            (core.ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + index] == 0x01u) &&
            (core.ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + index] != 0u))
        {
            laser_visible = true;
        }
    }
    CHECK(laser_visible);
    step_with_input(&core, CONTRA_BUTTON_B);
    CHECK(count_player_bullets_with_slot(&core, 0x05u) >= 2u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_PLAYER_WATER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_AIM_DIR] = 0x06u;
    step_no_input(&core);
    CHECK((core.ram[CONTRA_RAM_PLAYER_WATER_STATE] & 0x80u) != 0u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_WATER_TIMER] != 0u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] = 0x01u;
    initial_pattern_hash = hash_bytes(core.ppu_pattern, sizeof(core.ppu_pattern));
    for (frame = 0u; frame < 128u; ++frame)
    {
        step_no_input(&core);
        if (core.ram[CONTRA_RAM_ALT_GRAPHIC_DATA_LOADING_FLAG] == 0x00u)
        {
            alt_graphics_finished = true;
            break;
        }
    }
    CHECK(alt_graphics_finished);
    CHECK(hash_bytes(core.ppu_pattern, sizeof(core.ppu_pattern)) != initial_pattern_hash);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x00u;
    core.ram[CONTRA_RAM_NUM_CONTINUES] = 0x01u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x06u);
    CHECK(core.ram[CONTRA_RAM_NUM_CONTINUES] == 0x00u);
    step_with_input(&core, CONTRA_BUTTON_SELECT);
    CHECK(core.ram[CONTRA_RAM_CONT_END_SELECTION] == 0x01u);
    step_with_input(&core, CONTRA_BUTTON_START);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x00u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x00u;
    core.ram[CONTRA_RAM_NUM_CONTINUES] = 0x00u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x07u);
    step_with_input(&core, CONTRA_BUTTON_START);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x00u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x07u;
    core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x06u);
    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x09u);
    return true;
}

static bool test_broad_enemy_pause_and_player_state_matrix(void)
{
    ContraCore core;
    ContraNativeEnemy *enemy;
    unsigned projectile_count;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    prepare_level1_enemy_matrix_state(&core);
    enemy = seed_enemy(&core, 0u, 0x00u, 0x00u, 0xE8, 0x60);
    enemy->attrs = 0x06u;
    enemy->vx = 1;
    enemy->vy = 1;
    enemy->x_frac = 0x80u;
    enemy = seed_enemy(&core, 1u, 0x02u, 0x00u, 0x40, 0x70);
    enemy->timer = 0x00u;
    enemy = seed_enemy(&core, 2u, 0x02u, 0x01u, 0x48, 0x70);
    enemy->flags = 0x02u;
    enemy = seed_enemy(&core, 3u, 0x03u, 0x00u, 0x130, 0x70);
    enemy->origin_x = 0x80u;
    enemy->origin_y = 0x70u;
    enemy = seed_enemy(&core, 4u, 0x04u, 0x01u, 0x58, 0x70);
    enemy->flags = 0x02u;
    enemy = seed_enemy(&core, 5u, 0x04u, 0x02u, 0x70, 0x70);
    enemy->cooldown = 0x00u;
    seed_enemy(&core, 6u, 0x04u, 0x02u, 0x10, 0x70);
    enemy = seed_enemy(&core, 7u, 0x05u, 0x02u, 0x20, 0x20);
    enemy->attrs = 0x02u;
    enemy->vx = 1;
    seed_enemy(&core, 8u, 0x06u, 0x02u, 0x08, 0x70);
    enemy = seed_enemy(&core, 9u, 0x07u, 0x01u, 0x50, 0x70);
    enemy->flags = 0x03u;
    enemy = seed_enemy(&core, 10u, 0x07u, 0x02u, 0x20, 0x70);
    enemy->cooldown = 0x00u;
    enemy = seed_enemy(&core, 11u, 0x07u, 0x03u, 0x50, 0x70);
    enemy->flags = 0x00u;
    enemy->timer = 0x00u;
    seed_enemy(&core, 12u, 0x10u, 0x02u, 0x10, 0x70);
    seed_enemy(&core, 13u, 0x12u, 0x00u, -120, 0x90);
    enemy = seed_enemy(&core, 14u, 0xFEu, 0x02u, 0x70, 0x70);
    enemy->flags = 0x00u;
    enemy->timer = 0x00u;
    seed_enemy(&core, 15u, 0x55u, 0x00u, 0x70, 0x70);

    step_no_input(&core);
    CHECK(core.enemies[1].state == 0x01u);
    CHECK(core.enemies[2].state == 0x02u);
    CHECK(core.enemies[4].state == 0x02u);
    CHECK(core.enemies[6].active == 0x00u);
    CHECK(core.enemies[8].active == 0x00u);
    CHECK(core.enemies[10].state == 0x03u);
    CHECK(core.enemies[11].active == 0x00u);
    CHECK(core.enemies[12].active == 0x00u);
    CHECK(core.enemies[13].active == 0x00u);
    CHECK(core.enemies[14].active == 0x00u);
    CHECK(core.enemies[15].active == 0x00u);

    prepare_level1_enemy_matrix_state(&core);
    enemy = seed_enemy(&core, 0u, 0x03u, 0x00u, 0x70, 0x70);
    enemy->hp = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_BULLET_SLOT] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = 0x70u;
    core.ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = 0x70u;
    step_no_input(&core);
    CHECK(core.enemies[0].type == 0x00u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_BULLET_SLOT] == 0x00u);
    core.enemies[0].x = (int16_t)core.ram[CONTRA_RAM_SPRITE_X_POS];
    core.enemies[0].y = (int16_t)core.ram[CONTRA_RAM_SPRITE_Y_POS];
    core.enemies[0].attrs = 0x05u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_INVINCIBILITY_TIMER] >= 0x7Fu);

    force_level2_gameplay(&core);
    memset(core.enemies, 0, sizeof(core.enemies));
    memset(core.enemy_projectiles, 0, sizeof(core.enemy_projectiles));
    core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x80u;
    core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x05u;
    enemy = seed_enemy(&core, 0u, 0x08u, 0x01u, 0x70, 0x60);
    enemy->timer = 0x00u;
    seed_enemy(&core, 1u, 0x13u, 0x00u, 0x80, 0x80);
    enemy = seed_enemy(&core, 2u, 0x14u, 0x08u, 0x80, 0x80);
    enemy->cooldown = 0x02u;
    enemy = seed_enemy(&core, 3u, 0x14u, 0x09u, 0x88, 0x80);
    enemy->timer = 0x00u;
    projectile_count = count_active_projectiles_from_owner(&core, 0x08u);
    step_no_input(&core);
    CHECK(count_active_projectiles_from_owner(&core, 0x08u) > projectile_count);
    CHECK(core.enemies[3].active == 0x00u);

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));
    core.ram[CONTRA_RAM_PPU_READY] = 0x00u;
    step_with_input(&core, CONTRA_BUTTON_START);
    CHECK(core.ram[CONTRA_RAM_PAUSE_STATE] == 0x01u);
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_PAUSE_STATE] == 0x01u);
    step_with_input(&core, CONTRA_BUTTON_START);
    CHECK(core.ram[CONTRA_RAM_PAUSE_STATE] == 0x00u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x01u;
    core.ram[CONTRA_RAM_INTRO_THEME_DELAY] = 0x01u;
    step_with_input(&core, CONTRA_BUTTON_SELECT);
    CHECK(core.ram[CONTRA_RAM_PLAYER_MODE] == 0x01u);
    step_no_input(&core);
    step_with_input(&core, CONTRA_BUTTON_SELECT);
    CHECK(core.ram[CONTRA_RAM_PLAYER_MODE] == 0x00u);
    step_with_input(&core, CONTRA_BUTTON_START);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x03u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x02u;
    core.ram[CONTRA_RAM_DEMO_MODE] = 0x01u;
    core.ram[CONTRA_RAM_INTRO_THEME_DELAY] = 0x01u;
    step_with_input(&core, CONTRA_BUTTON_START);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x01u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x0Au;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core.ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] = 0x01u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x07u);
    return true;
}

static bool test_broad_player_ui_and_end_level_matrix(void)
{
    ContraCore core;
    unsigned frame;

    contra_core_init(&core);
    CHECK(run_until_gameplay(&core, 900u));

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_LEVEL_STOP_SCROLL] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] = 0x01u;
    step_with_input(&core, (uint8_t)(CONTRA_BUTTON_A | CONTRA_BUTTON_DOWN));
    CHECK(core.ram[CONTRA_RAM_EDGE_FALL_CODE] != 0x00u);
    step_no_input(&core);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x01u;
    core.ram[CONTRA_RAM_LEVEL_STOP_SCROLL] = 0xFFu;
    core.ram[CONTRA_RAM_EDGE_FALL_CODE] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT] = 0x01u;
    step_with_input(&core, (uint8_t)(CONTRA_BUTTON_A | CONTRA_BUTTON_DOWN));
    CHECK(core.ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_WATER_STATE] = 0x04u;
    core.ram[CONTRA_RAM_PLAYER_WATER_TIMER] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_RECOIL_TIMER] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_AIM_DIR] = 0x04u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_PLAYER_SPRITE_CODE] != 0x00u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_WATER_STATE] = 0x04u;
    core.ram[CONTRA_RAM_PLAYER_WATER_TIMER] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_RECOIL_TIMER] = 0x00u;
    core.ram[CONTRA_RAM_FRAME_COUNTER] = 0x00u;
    step_no_input(&core);
    CHECK((core.ram[CONTRA_RAM_PLAYER_WATER_STATE] & 0x04u) != 0x00u);
    core.ram[CONTRA_RAM_FRAME_COUNTER] = 0x01u;
    step_no_input(&core);
    CHECK((core.ram[CONTRA_RAM_PLAYER_WATER_STATE] & 0x04u) != 0x00u);

    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_PLAYER_WATER_STATE] = 0x0Cu;
    core.ram[CONTRA_RAM_PLAYER_WATER_TIMER] = 0x03u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_PLAYER_WATER_TIMER] < 0x03u);
    prepare_level1_weapon_state(&core);
    core.ram[CONTRA_RAM_PLAYER_WATER_STATE] = 0x0Cu;
    core.ram[CONTRA_RAM_PLAYER_WATER_TIMER] = 0x00u;
    step_no_input(&core);
    CHECK((core.ram[CONTRA_RAM_PLAYER_WATER_STATE] & 0x08u) == 0x00u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x06u;
    core.ram[CONTRA_RAM_CONT_END_SELECTION] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_MODE] = 0x01u;
    core.ram[CONTRA_RAM_HIGH_SCORE_LOW] = 0x99u;
    core.ram[CONTRA_RAM_HIGH_SCORE_HIGH] = 0x09u;
    core.ram[CONTRA_RAM_PLAYER_1_SCORE_LOW] = 0x56u;
    core.ram[CONTRA_RAM_PLAYER_1_SCORE_HIGH] = 0x34u;
    core.ram[CONTRA_RAM_PLAYER_2_SCORE_LOW] = 0x78u;
    core.ram[CONTRA_RAM_PLAYER_2_SCORE_HIGH] = 0x12u;
    core.ram[CONTRA_RAM_FRAME_COUNTER] = 0x00u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_SPRITE_X_POS] == 0x52u);
    CHECK(core.ram[CONTRA_RAM_CPU_SPRITE_BUFFER] == 0xAAu);
    step_with_input(&core, CONTRA_BUTTON_START);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_1_SCORE_LOW] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_P1_NUM_LIVES] != 0x00u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x07u;
    core.ram[CONTRA_RAM_PLAYER_MODE] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_2_SCORE_LOW] = 0x21u;
    core.ram[CONTRA_RAM_PLAYER_2_SCORE_HIGH] = 0x43u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x04u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] = 0x00u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x0Au);
    CHECK(core.ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER] != 0x00u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x08u;
    core.ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0xFFu;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x08u);
    core.ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE] = 0x01u;
    core.ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE] = 0x00u;
    for (frame = 0u; (frame < 4u) && (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x08u); ++frame)
    {
        step_no_input(&core);
    }
    CHECK(core.ram[CONTRA_RAM_LEVEL_END_PLAYERS_ALIVE] != 0x00u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x09u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x09u;
    core.ram[CONTRA_RAM_CURRENT_LEVEL] = 0x00u;
    core.ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX] = 0x01u;
    core.ram[CONTRA_RAM_LEVEL_END_DELAY_TIMER] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE] = 0x01u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x01u;
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x80u;
    core.ram[CONTRA_RAM_SPRITE_Y_POS] = 0x80u;
    step_no_input(&core);
    CHECK((core.ram[CONTRA_RAM_CONTROLLER_STATE] & CONTRA_BUTTON_RIGHT) != 0x00u);
    core.ram[CONTRA_RAM_SPRITE_X_POS] = 0x90u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE] >= 0x02u);
    core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] = 0x00u;
    step_no_input(&core);
    CHECK((core.ram[CONTRA_RAM_CONTROLLER_STATE] & CONTRA_BUTTON_A) != 0x00u);
    core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] = 0x01u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_END_LVL_ROUTINE_STATE] >= 0x03u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x01u;
    core.ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_MODE] = 0x00u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x01u;
    core.ram[CONTRA_RAM_P1_NUM_LIVES] = 0x00u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x02u);

    contra_core_init(&core);
    core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x01u;
    core.ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
    core.ram[CONTRA_RAM_PLAYER_MODE] = 0x01u;
    core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P1_NUM_LIVES] = 0x2Au;
    core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x00u;
    core.ram[CONTRA_RAM_P2_NUM_LIVES] = 0xFFu;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x02u);

    return true;
}

int main(void)
{
    const TestCase tests[] = {
        {"title_start_reaches_level1_gameplay", test_title_start_reaches_level1_gameplay},
        {"level1_scrolls_right_under_player_input", test_level1_scrolls_right_under_player_input},
        {"level1_spawns_native_enemies_while_scrolling", test_level1_spawns_native_enemies_while_scrolling},
        {"level1_generated_soldier_spawns_on_snapped_floor", test_level1_generated_soldier_spawns_on_snapped_floor},
        {"level1_rifle_man_stays_seated_on_floor_after_y_drift", test_level1_rifle_man_stays_seated_on_floor_after_y_drift},
        {"level1_red_turret_bullet_uses_rom_muzzle_y_offset", test_level1_red_turret_bullet_uses_rom_muzzle_y_offset},
        {"level1_boss_bomb_turret_uses_rom_wall_frame_and_muzzle", test_level1_boss_bomb_turret_uses_rom_wall_frame_and_muzzle},
        {"level1_bullet_destroyed_enemy_becomes_explosion", test_level1_bullet_destroyed_enemy_becomes_explosion},
        {"level1_weapon_item_pickup_changes_weapon_and_bullet", test_level1_weapon_item_pickup_changes_weapon_and_bullet},
        {"level1_bridge_destruction_reaches_overlay_state", test_level1_bridge_destruction_changes_render_state},
        {"level1_organic_bridge_load_changes_collision", test_level1_organic_bridge_load_changes_collision},
        {"level1_forced_boss_clear_hands_off_to_level2", test_level1_forced_boss_clear_hands_off_to_level2},
        {"attract_reaches_level2_gameplay", test_attract_reaches_level2_gameplay},
        {"attract_level1_matches_rom_startup_timing", test_attract_level1_matches_rom_startup_timing},
        {"attract_level1_matches_rom_graphics_load_stall", test_attract_level1_matches_rom_graphics_load_stall},
        {"attract_level1_matches_rom_gameplay_entry_frame", test_attract_level1_matches_rom_gameplay_entry_frame},
        {"attract_level1_matches_rom_first_scroll_frame", test_attract_level1_matches_rom_first_scroll_frame},
        {"attract_level1_blocks_p2_scroll_when_p1_at_left_edge", test_attract_level1_blocks_p2_scroll_when_p1_at_left_edge},
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
        {"game_over_delay_expiry_loads_screen_without_glitch", test_game_over_delay_expiry_loads_screen_without_glitch},
        {"attract_level2_demo_does_not_consume_multiple_lives_before_rom_terminal", test_attract_level2_demo_does_not_consume_multiple_lives_before_rom_terminal},
        {"level2_repeated_room_advances_reach_boss_state", test_level2_repeated_room_advances_reach_boss_state},
        {"level2_boss_room_loads_rom_enemy_data", test_level2_boss_room_loads_rom_enemy_data},
        {"level2_boss_room_plating_and_eye_can_be_destroyed", test_level2_boss_room_plating_and_eye_can_be_destroyed},
        {"level2_boss_room_wall_cannons_fire_projectiles", test_level2_boss_room_wall_cannons_fire_projectiles},
        {"broad_weapon_gameover_and_alt_graphics_matrix", test_broad_weapon_gameover_and_alt_graphics_matrix},
        {"broad_enemy_pause_and_player_state_matrix", test_broad_enemy_pause_and_player_state_matrix},
        {"broad_player_ui_and_end_level_matrix", test_broad_player_ui_and_end_level_matrix}
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
