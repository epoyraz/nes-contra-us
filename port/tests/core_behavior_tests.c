#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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

static uint32_t hash_framebuffer_region(
    const uint32_t *framebuffer,
    unsigned start_x,
    unsigned start_y,
    unsigned width,
    unsigned height
)
{
    uint32_t hash = 2166136261u;
    unsigned y;

    for (y = start_y; y < (start_y + height); ++y)
    {
        unsigned x;

        for (x = start_x; x < (start_x + width); ++x)
        {
            const uint32_t pixel = framebuffer[(size_t)y * CONTRA_FRAMEBUFFER_WIDTH + (size_t)x];
            size_t byte;

            for (byte = 0u; byte < sizeof(pixel); ++byte)
            {
                hash ^= (uint8_t)(pixel >> (byte * 8u));
                hash *= 16777619u;
            }
        }
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

static bool run_until_game_routine(ContraCore *core, uint8_t routine, unsigned frame_limit)
{
    unsigned frame;

    for (frame = 0u; frame < frame_limit; ++frame)
    {
        step_no_input(core);
        if (core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == routine)
        {
            return true;
        }
    }

    return false;
}

static void enter_konami_code(ContraCore *core)
{
    static const uint8_t code[10] = {
        CONTRA_BUTTON_UP, CONTRA_BUTTON_UP,
        CONTRA_BUTTON_DOWN, CONTRA_BUTTON_DOWN,
        CONTRA_BUTTON_LEFT, CONTRA_BUTTON_RIGHT,
        CONTRA_BUTTON_LEFT, CONTRA_BUTTON_RIGHT,
        CONTRA_BUTTON_B, CONTRA_BUTTON_A
    };
    size_t index;

    for (index = 0u; index < sizeof(code); ++index)
    {
        step_with_input(core, code[index]);
        step_no_input(core);
    }
}

/* Enemy-slot accessors used by the level-2 helpers. Level-2 enemies live in real
   CPU RAM (16 slots), so these read the faithful ENEMY_* arrays directly. */
static bool l2_enemy_active(const ContraCore *core, size_t i)
{
    return (i < 16u) && (core->ram[CONTRA_RAM_ENEMY_ROUTINE + i] != 0u);
}
static uint8_t l2_enemy_type(const ContraCore *core, size_t i)
{
    return core->ram[CONTRA_RAM_ENEMY_TYPE + i];
}
static uint8_t l2_enemy_hp(const ContraCore *core, size_t i)
{
    return core->ram[CONTRA_RAM_ENEMY_HP + i];
}
static uint8_t l2_enemy_x(const ContraCore *core, size_t i)
{
    return core->ram[CONTRA_RAM_ENEMY_X_POS + i];
}
static uint8_t l2_enemy_y(const ContraCore *core, size_t i)
{
    return core->ram[CONTRA_RAM_ENEMY_Y_POS + i];
}
/* The wall core is destroyable once it has opened -- bullet collision enabled,
   i.e. ENEMY_STATE_WIDTH bit 7 clear (the faithful analogue of invented state 3).*/
static bool l2_enemy_core_destroyable(const ContraCore *core, size_t i)
{
    return (core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + i] & 0x80u) == 0u;
}
/* The destroyed core runs its destruction chain (routine 5+, the analogue of the
   invented "destroyed" state 0x08), and the back-wall blow-open is tracked in
   l2_blowopen_quadrants (the analogue of the invented per-enemy flags). */
static bool l2_core_destroying(const ContraCore *core, size_t i)
{
    return core->ram[CONTRA_RAM_ENEMY_ROUTINE + i] >= 0x05u;
}
static bool l2_blowopen_started(const ContraCore *core, size_t i)
{
    (void)i;
    return core->l2_blowopen_quadrants != 0u;
}
/* Inject a live player bullet at (x,y): the faithful bullet-vs-enemy collision
   gates on PLAYER_BULLET_SPRITE_CODE != 0 and PLAYER_BULLET_ROUTINE == 1. */
static void l2_inject_player_bullet(ContraCore *core, uint8_t x, uint8_t y)
{
    core->ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE] = 0x01u;
    core->ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE] = 0x01u;
    core->ram[CONTRA_RAM_PLAYER_BULLET_X_POS] = x;
    core->ram[CONTRA_RAM_PLAYER_BULLET_Y_POS] = y;
}
/* Faithful enemy bullets are real-RAM enemy slots of type 0x01 (owner is not
   tracked there), so count those rather than the invented projectile mirror. */
static unsigned count_active_projectiles_from_owner(const ContraCore *core, uint8_t owner)
{
    unsigned count = 0u;
    size_t i;

    (void)owner;
    for (i = 0u; i < 16u; ++i)
    {
        if ((core->ram[CONTRA_RAM_ENEMY_ROUTINE + i] != 0u) &&
            (core->ram[CONTRA_RAM_ENEMY_TYPE + i] == 0x01u))
        {
            ++count;
        }
    }
    return count;
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

static void force_level4_gameplay(ContraCore *core)
{
    unsigned frame;

    contra_core_init(core);
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = 0x03u;
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

static void force_level5_gameplay(ContraCore *core)
{
    unsigned frame;

    contra_core_init(core);
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = 0x04u;
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

static void force_level6_gameplay(ContraCore *core)
{
    unsigned frame;

    contra_core_init(core);
    core->ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x00u;
    core->ram[CONTRA_RAM_CURRENT_LEVEL] = 0x05u;
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
        if (l2_enemy_active(core, index) && (l2_enemy_type(core, index) == 0x14u))
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
        if (l2_enemy_active(core, index) && (l2_enemy_type(core, index) == enemy_type))
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
        if (l2_enemy_active(core, index) && (l2_enemy_type(core, index) == enemy_type))
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
            l2_enemy_core_destroyable(core, wall_core_index))
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
         l2_enemy_active(core, wall_core_index) &&
         (l2_enemy_type(core, wall_core_index) == 0x14u) &&
         (l2_enemy_hp(core, wall_core_index) != 0u);
         ++frame)
    {
        l2_inject_player_bullet(core, l2_enemy_x(core, wall_core_index), l2_enemy_y(core, wall_core_index));
        step_no_input(core);
    }

    if ((frame == 128u) ||
        !find_first_active_wall_core(core, &wall_core_index) ||
        (l2_enemy_hp(core, wall_core_index) != 0u))
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

/* Only the (retired-under-faithful) boss-room plating test uses this. */

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

static bool test_attract_level1_bridge_demo_keeps_p2_on_rom_route(void)
{
    ContraCore core;
    unsigned frame;

    contra_core_init(&core);

    for (frame = 1u; frame <= 1601u; ++frame)
    {
        step_no_input(&core);

        if (frame == 1597u)
        {
            CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u);
            CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x02u);
            CHECK(core.ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + 1u] == 0x00u);
            CHECK(core.ram[CONTRA_RAM_EDGE_FALL_CODE + 1u] == 0x00u);
            CHECK(core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 1u] == 0x00u);
            CHECK(core.ram[CONTRA_RAM_SPRITE_Y_POS + 1u] == 0x64u);
        }
    }

    CHECK(core.ram[CONTRA_RAM_PLAYER_BG_FLAG_EDGE_DETECT + 1u] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_EDGE_FALL_CODE + 1u] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 1u] == 0x11u);
    CHECK(core.ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + 1u] == 0xFBu);
    CHECK(core.ram[CONTRA_RAM_SPRITE_Y_POS + 1u] == 0x64u);
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

static bool test_konami_code_in_intro_grants_30_lives(void)
{
    ContraCore core;

    contra_core_init(&core);
    CHECK(run_until_game_routine(&core, 0x01u, 32u));

    enter_konami_code(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_KONAMI_CODE_STATUS] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] == 0x0Au);

    CHECK(run_until_gameplay(&core, 900u));
    CHECK(core.ram[CONTRA_RAM_P1_NUM_LIVES] == 0x1Du);
    CHECK(core.ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] == 0x00u);
    return true;
}

static bool test_konami_code_ignored_after_intro_check_window(void)
{
    ContraCore core;

    contra_core_init(&core);
    CHECK(run_until_game_routine(&core, 0x02u, 900u));

    enter_konami_code(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_KONAMI_CODE_STATUS] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] == 0x00u);
    return true;
}

static bool test_konami_code_ignored_during_gameplay(void)
{
    ContraCore core;

    force_level4_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    enter_konami_code(&core);
    CHECK(core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u);
    CHECK(core.ram[CONTRA_RAM_KONAMI_CODE_STATUS] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_KONAMI_CODE_NUM_CORRECT] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_P1_NUM_LIVES] == 0x02u);
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
    /* The faithful wall core takes its HP in routine_00 (one frame after spawn),
       where the invented system set it on spawn; step until it has initialized. */
    for (frame = 0u; frame < 8u; ++frame)
    {
        step_no_input(&core);
        if (find_first_active_wall_core(&core, &wall_core_index) &&
            (l2_enemy_hp(&core, wall_core_index) == 0x08u))
        {
            break;
        }
    }
    CHECK(find_first_active_wall_core(&core, &wall_core_index));
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
    CHECK(l2_enemy_hp(&core, wall_core_index) == 0x08u);
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
            l2_enemy_core_destroyable(&core, wall_core_index))
        {
            break;
        }
    }

    CHECK(frame < 320u);
    CHECK(l2_enemy_hp(&core, wall_core_index) == 0x08u);
    while (l2_enemy_hp(&core, wall_core_index) != 0u)
    {
        l2_inject_player_bullet(&core, l2_enemy_x(&core, wall_core_index), l2_enemy_y(&core, wall_core_index));
        step_no_input(&core);
    }

    CHECK(l2_enemy_active(&core, wall_core_index));
    CHECK(l2_enemy_type(&core, wall_core_index) == 0x14u);
    CHECK(l2_core_destroying(&core, wall_core_index));
    initial_framebuffer_hash = hash_bytes(core.framebuffer, sizeof(core.framebuffer));

    /* the destruction chain registers the kill (WALL_CORE_REMAINING) and blows the
       back wall open over the next frames (the faithful chain is multi-frame, where
       the invented path did it immediately) */
    for (frame = 0u; frame < 48u; ++frame)
    {
        step_no_input(&core);
        if (l2_blowopen_started(&core, wall_core_index))
        {
            saw_quadrant_update = true;
            break;
        }
    }

    CHECK(saw_quadrant_update);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x00u);
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

    /* The faithful generator's per-round cadence differs from the invented one, so
       running soldiers can arrive in a later round; widen the window and assert the
       intent (both soldier kinds spawn and attack rounds increment). */
    for (frame = 0u; frame < 360u; ++frame)
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
    CHECK(core.ram[CONTRA_RAM_INDOOR_ENEMY_ATTACK_COUNT] >= 0x01u);
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
    CHECK(l2_enemy_type(&core, wall_core_index) == 0x14u);
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
    return true;
}

static bool test_level4_first_room_loads_rom_enemy_data(void)
{
    ContraCore core;
    size_t wall_core_index = 0u;

    force_level4_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);
    step_no_input(&core);

    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_STOP_SCROLL] == 0x08u);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x01u);
    CHECK(count_active_enemy_type(&core, 0x19u) == 1u);
    CHECK(count_active_enemy_type(&core, 0x14u) == 1u);
    CHECK(count_active_enemy_type(&core, 0x13u) == 2u);
    CHECK(find_first_active_wall_core(&core, &wall_core_index));
    CHECK(l2_enemy_x(&core, wall_core_index) == 0x80u);
    CHECK(l2_enemy_y(&core, wall_core_index) == 0x68u);
    return true;
}

static bool test_level4_first_room_renders_wall_core_target(void)
{
    ContraCore core;
    size_t wall_core_index = 0u;
    uint32_t target_hash;
    uint32_t suppressed_hash;
    unsigned frame;

    force_level4_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    for (frame = 0u; frame < 80u; ++frame)
    {
        step_no_input(&core);
        if (find_first_active_wall_core(&core, &wall_core_index) &&
            (core.l2_structure_tile[wall_core_index] != 0u))
        {
            break;
        }
    }

    CHECK(frame < 80u);
    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u);
    CHECK(core.l2_structure_tile[wall_core_index] != 0u);

    target_hash = hash_framebuffer_region(
        contra_core_framebuffer(&core),
        (unsigned)(l2_enemy_x(&core, wall_core_index) - 8u),
        (unsigned)(l2_enemy_y(&core, wall_core_index) - 8u),
        24u,
        24u);

    core.l2_structure_tile[wall_core_index] = 0u;
    step_no_input(&core);

    suppressed_hash = hash_framebuffer_region(
        contra_core_framebuffer(&core),
        (unsigned)(l2_enemy_x(&core, wall_core_index) - 8u),
        (unsigned)(l2_enemy_y(&core, wall_core_index) - 8u),
        24u,
        24u);

    CHECK(target_hash != suppressed_hash);
    return true;
}

static bool test_level4_debug_env_lives_and_weapon(void)
{
    ContraCore core;
    bool ok;

    setenv("CONTRA_START_LIVES", "30", 1);
    setenv("CONTRA_START_WEAPON", "S", 1);
    contra_core_init(&core);
    contra_core_debug_warp_level4(&core);

    ok = (core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u) &&
        (core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u) &&
        (core.ram[CONTRA_RAM_P1_NUM_LIVES] == 0x1Du) &&
        (core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] == 0x03u);

    unsetenv("CONTRA_START_LIVES");
    unsetenv("CONTRA_START_WEAPON");

    CHECK(ok);
    return true;
}

static uint8_t level4_respawn_weapon_with_env(bool keep_weapon)
{
    ContraCore core;

    if (keep_weapon)
    {
        setenv("CONTRA_KEEP_START_WEAPON", "1", 1);
    }
    else
    {
        unsetenv("CONTRA_KEEP_START_WEAPON");
    }
    setenv("CONTRA_START_WEAPON", "S", 1);

    force_level4_gameplay(&core);
    core.ram[CONTRA_RAM_P1_CURRENT_WEAPON] = 0x01u;
    core.ram[CONTRA_RAM_P1_NUM_LIVES] = 0x02u;
    core.ram[CONTRA_RAM_PLAYER_STATE] = 0x02u;
    core.ram[CONTRA_RAM_PLAYER_ANIM_FRAME_TIMER] = 0x01u;
    step_no_input(&core);

    unsetenv("CONTRA_KEEP_START_WEAPON");
    unsetenv("CONTRA_START_WEAPON");
    return core.ram[CONTRA_RAM_P1_CURRENT_WEAPON];
}

static bool test_respawn_resets_weapon_without_keep_start_weapon(void)
{
    CHECK(level4_respawn_weapon_with_env(false) == 0x00u);
    return true;
}

static bool test_respawn_restores_start_weapon_with_keep_start_weapon(void)
{
    CHECK(level4_respawn_weapon_with_env(true) == 0x03u);
    return true;
}

static bool test_level5_ice_grenade_generator_uses_rom_props(void)
{
    ContraCore core;
    size_t generator_index = 0u;

    force_level5_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x20u;
    core.ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
    step_no_input(&core);

    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u);
    CHECK(find_first_active_enemy_type(&core, 0x10u, &generator_index));
    CHECK(core.ram[CONTRA_RAM_ENEMY_STATE_WIDTH + generator_index] == 0x81u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + generator_index] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_HP + generator_index] == 0xF0u);
    CHECK(l2_enemy_x(&core, generator_index) == 0xF0u);
    CHECK(l2_enemy_y(&core, generator_index) == 0x60u);
    return true;
}

static bool test_level5_ice_grenade_generator_spawns_ice_grenade(void)
{
    ContraCore core;
    size_t generator_index = 0u;

    force_level5_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x00u;
    core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x20u;
    core.ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
    step_no_input(&core);
    CHECK(find_first_active_enemy_type(&core, 0x10u, &generator_index));

    core.ram[CONTRA_RAM_ENEMY_X_POS + generator_index] = 0xC7u;
    core.ram[CONTRA_RAM_ENEMY_ROUTINE + generator_index] = 0x01u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + generator_index] == 0x02u);

    core.ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + generator_index] = 0x01u;
    step_no_input(&core);
    CHECK(find_first_active_enemy_type(&core, 0x11u, &generator_index));
    CHECK(core.ram[CONTRA_RAM_ENEMY_STATE_WIDTH + generator_index] == 0x81u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + generator_index] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_HP + generator_index] == 0xF1u);
    return true;
}

static bool test_level5_specific_types_do_not_use_indoor_handlers(void)
{
    ContraCore core;
    const size_t slot = 0u;

    force_level5_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    core.ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x13u;
    core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] = 0x01u;
    core.ram[CONTRA_RAM_ENEMY_SPRITES + slot] = 0x00u;
    core.ram[CONTRA_RAM_ENEMY_X_POS + slot] = 0x80u;
    core.ram[CONTRA_RAM_ENEMY_Y_POS + slot] = 0xB0u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x13u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_SPRITES + slot] == 0xC4u);

    core.ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x12u;
    core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] = 0x01u;
    core.ram[CONTRA_RAM_ENEMY_SPRITES + slot] = 0x00u;
    core.ram[CONTRA_RAM_ENEMY_X_POS + slot] = 0x80u;
    core.ram[CONTRA_RAM_ENEMY_Y_POS + slot] = 0x30u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x12u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_SPRITES + slot] == 0x00u);
    return true;
}

static bool test_level6_fire_beam_uses_rom_props_and_init(void)
{
    ContraCore core;
    size_t beam_index = 0u;

    force_level6_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x03u;
    core.ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] = 0x48u;
    core.ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x03u; /* skip screen-3 pill box */
    step_no_input(&core);

    CHECK(core.ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u);
    CHECK(find_first_active_enemy_type(&core, 0x10u, &beam_index));
    CHECK(core.ram[CONTRA_RAM_ENEMY_STATE_WIDTH + beam_index] == 0x81u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + beam_index] == 0x0Fu);
    CHECK(core.ram[CONTRA_RAM_ENEMY_HP + beam_index] == 0xF0u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + beam_index] == 0x01u);

    step_no_input(&core);
    CHECK(find_first_active_enemy_type(&core, 0x10u, &beam_index));
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + beam_index] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_FRAME + beam_index] == 0x04u);
    CHECK((core.ram[CONTRA_RAM_ENEMY_ATTRIBUTES + beam_index] & 0x80u) != 0u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_Y_POS + beam_index] == 0x28u);
    return true;
}

static bool test_level6_left_and_right_beams_do_not_use_old_handlers(void)
{
    ContraCore core;
    const size_t slot = 0u;

    force_level6_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    core.ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x11u;
    core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] = 0x01u;
    core.ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = 0x00u;
    core.ram[CONTRA_RAM_ENEMY_X_POS + slot] = 0x80u;
    core.ram[CONTRA_RAM_ENEMY_Y_POS + slot] = 0x58u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x11u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] == 0x40u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_Y_POS + slot] == 0x60u);

    core.ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x12u;
    core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] = 0x01u;
    core.ram[CONTRA_RAM_ENEMY_ATTRIBUTES + slot] = 0x00u;
    core.ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + slot] = 0x00u;
    core.ram[CONTRA_RAM_ENEMY_X_POS + slot] = 0x80u;
    core.ram[CONTRA_RAM_ENEMY_Y_POS + slot] = 0x68u;
    step_no_input(&core);
    CHECK(core.ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x12u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] == 0x02u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + slot] == 0x40u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_Y_POS + slot] == 0x70u);
    return true;
}

static bool test_level6_boss_robot_not_misrouted_to_level3_or_level2(void)
{
    ContraCore core;
    const size_t slot = 0u;

    force_level6_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    core.ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x13u;
    core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] = 0x01u;
    core.ram[CONTRA_RAM_ENEMY_SPRITES + slot] = 0x00u;
    core.ram[CONTRA_RAM_ENEMY_X_POS + slot] = 0xB0u;
    core.ram[CONTRA_RAM_ENEMY_Y_POS + slot] = 0xA0u;
    step_no_input(&core);

    CHECK(core.ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x13u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] == 0x01u);
    CHECK(core.ram[CONTRA_RAM_ENEMY_SPRITES + slot] == 0x00u);
    return true;
}

static bool test_level4_multi_core_room_loads_rom_enemy_data(void)
{
    ContraCore core;

    force_level4_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u);

    core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x01u;
    core.ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
    step_no_input(&core);

    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x04u);
    CHECK(count_active_enemy_type(&core, 0x19u) == 1u);
    CHECK(count_active_enemy_type(&core, 0x14u) == 4u);
    CHECK(count_active_enemy_type(&core, 0x13u) == 0u);
    return true;
}

static bool load_level4_boss_room(ContraCore *core)
{
    force_level4_gameplay(core);
    if (core->ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] != 0x04u)
    {
        return false;
    }

    core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] = 0x08u;
    core->ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] = 0x80u;
    core->ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = 0x00u;
    step_no_input(core);
    return true;
}

static bool test_level4_boss_room_loads_rom_enemy_data(void)
{
    ContraCore core;

    CHECK(load_level4_boss_room(&core));

    CHECK(core.ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0x80u);
    CHECK(core.ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] == 0x08u);
    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x02u);
    CHECK(count_active_enemy_type(&core, 0x20u) == 1u);
    CHECK(count_active_enemy_type(&core, 0x1Cu) == 2u);
    CHECK(count_active_enemy_type(&core, 0x08u) == 1u);
    CHECK(count_active_enemy_type(&core, 0x0Au) == 3u);
    return true;
}

static bool test_level4_boss_red_blue_generator_spawns_soldiers(void)
{
    ContraCore core;
    unsigned frame;
    bool spawned_red = false;

    CHECK(load_level4_boss_room(&core));
    CHECK(count_active_enemy_type(&core, 0x20u) == 1u);
    CHECK(count_active_enemy_type(&core, 0x1Fu) == 0u);
    core.ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] = 0x00u;

    for (frame = 0u; frame < 360u; ++frame)
    {
        step_no_input(&core);
        if (count_active_enemy_type(&core, 0x1Fu) != 0u)
        {
            spawned_red = true;
            break;
        }
    }

    CHECK(spawned_red);
    CHECK(count_active_enemy_type(&core, 0x20u) == 1u);
    return true;
}

static bool test_level4_boss_gemini_defeat_sets_boss_flag(void)
{
    ContraCore core;
    size_t gemini_index = 0u;
    unsigned frame;

    CHECK(load_level4_boss_room(&core));
    CHECK(find_first_active_enemy_type(&core, 0x1Cu, &gemini_index));
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + gemini_index] == 0x01u);

    core.ram[CONTRA_RAM_WALL_PLATING_DESTROYED_COUNT] = 0x03u;
    core.ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] = 0x00u;
    for (frame = 0u; frame < 96u; ++frame)
    {
        step_no_input(&core);
    }

    CHECK(count_active_enemy_type(&core, 0x1Cu) == 2u);
    CHECK(find_first_active_enemy_type(&core, 0x1Cu, &gemini_index));
    CHECK(core.ram[CONTRA_RAM_ENEMY_VAR_4 + gemini_index] == 0x0Au);
    CHECK(core.ram[CONTRA_RAM_ENEMY_ROUTINE + gemini_index] >= 0x03u);

    while (find_first_active_enemy_type(&core, 0x1Cu, &gemini_index))
    {
        core.ram[CONTRA_RAM_ENEMY_VAR_4 + gemini_index] = 0x01u;
        core.ram[CONTRA_RAM_ENEMY_HP + gemini_index] = 0x01u;
        core.ram[CONTRA_RAM_ENEMY_STATE_WIDTH + gemini_index] =
            (uint8_t)(core.ram[CONTRA_RAM_ENEMY_STATE_WIDTH + gemini_index] & 0x7Eu);
        l2_inject_player_bullet(
            &core,
            core.ram[CONTRA_RAM_ENEMY_X_POS + gemini_index],
            core.ram[CONTRA_RAM_ENEMY_Y_POS + gemini_index]);
        for (frame = 0u; frame < 8u; ++frame)
        {
            step_no_input(&core);
        }
        if (core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0u)
        {
            break;
        }
    }

    CHECK(core.ram[CONTRA_RAM_WALL_CORE_REMAINING] == 0x00u);
    CHECK(core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] != 0u);
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

/* screen-0 wall core: ROM enemy data pos byte 0x68 (Y base 0x60, X base 0x80)
   with the +Y pos-adjust flag set. The ROM tests that flag with `asl`, which
   leaves carry=1, so the subsequent `adc #$07` adds 8 -- the spawn Y must be 0x68
   (104), not 0x67 (the off-by-one the port had). */
static bool test_level2_indoor_enemy_spawn_y_pos_adjust(void)
{
    ContraCore core;
    size_t idx = 0u;
    unsigned frame;

    force_level2_gameplay(&core);
    for (frame = 0u; frame < 8u; ++frame)
    {
        step_no_input(&core);
        if (find_first_active_wall_core(&core, &idx))
        {
            break;
        }
    }
    CHECK(find_first_active_wall_core(&core, &idx));
    CHECK(l2_enemy_y(&core, idx) == 0x68u); /* 0x60 + 8 (the +8, not +7) */
    CHECK(l2_enemy_x(&core, idx) == 0x80u); /* 0x80, no X adjust */
    return true;
}

/* Fire one standard P1 bullet on the indoor level once the player has landed, and
   return its bullet slot (or -1). B is pressed for a single frame so it registers
   as a newly-pressed shot. */
static int l2_fire_one_player_bullet(ContraCore *core)
{
    unsigned frame;
    int slot;

    for (frame = 0u; frame < 120u; ++frame)
    {
        if ((core->ram[CONTRA_RAM_PLAYER_STATE] == 0x01u) &&
            (core->ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u) &&
            (core->ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u) &&
            (core->ram[CONTRA_RAM_PLAYER_RECOIL_TIMER] == 0x00u))
        {
            break;
        }
        step_no_input(core);
    }
    step_no_input(core);                       /* B released the prior frame */
    step_with_input(core, CONTRA_BUTTON_B);    /* newly-pressed fire */

    for (slot = 0; slot < 16; ++slot)
    {
        if (core->ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + (size_t)slot] != 0u)
        {
            return slot;
        }
    }
    return -1;
}

/* Indoor (pseudo-3D) player bullets spawn from set_indoor_bullet_pos_and_slot, NOT
   the outdoor aim-offset table: y is -24 (forward into the screen) or -12 (aiming
   down), and x is +/-1 -- never the outdoor +16/-5 geometry. */
static bool test_level2_indoor_player_bullet_spawn_geometry(void)
{
    ContraCore core;
    int slot;
    int dy;
    int dx;

    force_level2_gameplay(&core);
    slot = l2_fire_one_player_bullet(&core);
    CHECK(slot >= 0);

    dy = (int)core.ram[CONTRA_RAM_SPRITE_Y_POS] - (int)core.ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + (size_t)slot];
    dx = (int)core.ram[CONTRA_RAM_PLAYER_BULLET_X_POS + (size_t)slot] - (int)core.ram[CONTRA_RAM_SPRITE_X_POS];
    if (dx < 0)
    {
        dx = -dx;
    }
    CHECK((dy == 12) || (dy == 24)); /* indoor forward/down, never outdoor -5 */
    CHECK(dx == 1);                  /* indoor +/-1, never outdoor +16 */
    return true;
}

/* Indoor player bullets despawn on PLAYER_BULLET_TIMER (0x2a frames, then a 6-frame
   routine-2 ring), not by flying off-screen. A bullet that still moved at the
   outdoor velocity would remain on-screen well past frame 50; the indoor bullet
   must be gone, having reached routine 2 while still on the visible playfield. */
static bool test_level2_indoor_player_bullet_despawns_on_timer(void)
{
    ContraCore core;
    int slot;
    unsigned frame;
    bool reached_routine_2 = false;
    bool despawned = false;

    force_level2_gameplay(&core);
    slot = l2_fire_one_player_bullet(&core);
    CHECK(slot >= 0);
    CHECK(core.ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + (size_t)slot] <= 0x01u);

    for (frame = 0u; frame < 60u; ++frame)
    {
        step_no_input(&core);
        if (core.ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + (size_t)slot] == 0u)
        {
            despawned = true;
            break;
        }
        if (core.ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + (size_t)slot] >= 0x02u)
        {
            reached_routine_2 = true;
            /* still on the visible playfield -- so this is the timer, not off-screen */
            CHECK(core.ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + (size_t)slot] >= 0x40u);
        }
    }

    CHECK(reached_routine_2);
    CHECK(despawned);
    return true;
}

/* The indoor electric fence is drawn by animating the CHR pattern tiles at PPU
   $1FC0 (animate_indoor_fence); the port never did this, so the fence was
   invisible. On the indoor base level with the screen not yet cleared, those tiles
   must be written non-blank and must change frame-to-frame as the electricity
   animates. */
static bool test_level2_indoor_fence_animates_chr(void)
{
    ContraCore core;
    unsigned frame;
    bool nonblank = false;
    bool changed = false;
    uint8_t first[0x40];
    size_t i;

    force_level2_gameplay(&core);
    CHECK(core.ram[CONTRA_RAM_INDOOR_SCREEN_CLEARED] == 0x00u); /* fence active */

    for (frame = 0u; frame < 16u; ++frame)
    {
        step_no_input(&core);
        for (i = 0u; i < 0x40u; ++i)
        {
            if (core.ppu_pattern[0x1FC0u + i] != 0u)
            {
                nonblank = true;
                break;
            }
        }
        if (nonblank)
        {
            break;
        }
    }
    CHECK(nonblank);
    memcpy(first, &core.ppu_pattern[0x1FC0u], 0x40u);

    for (frame = 0u; frame < 20u; ++frame)
    {
        step_no_input(&core);
        if (memcmp(first, &core.ppu_pattern[0x1FC0u], 0x40u) != 0)
        {
            changed = true;
            break;
        }
    }
    CHECK(changed);
    return true;
}

/* A killed level-2 indoor running soldier (type 0x15) must die via the shared
   explosion actor (0xFE) and clear, NOT get routed to routine 5 -- on L2 the type
   has no routine-5 handler, so it would freeze in place (still collidable) in front
   of the core and block the room (a soft-lock). (Routine 5 is correct only for the
   level-3 dragon arm orb, which also uses type 0x15.) */
static bool test_level2_killed_indoor_soldier_explodes_not_freezes(void)
{
    ContraCore core;
    const size_t slot = 10u;
    unsigned frame;
    bool frozen = false;
    bool cleared = false;

    force_level2_gameplay(&core);
    core.ram[CONTRA_RAM_ENEMY_TYPE + slot] = 0x15u;        /* indoor running soldier */
    core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] = 0x02u;     /* running */
    core.ram[CONTRA_RAM_ENEMY_HP + slot] = 0x01u;
    core.ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot] = 0x00u; /* collidable */
    core.ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + slot] = 0x01u; /* small centered box */
    core.ram[CONTRA_RAM_ENEMY_X_POS + slot] = 0x80u;
    core.ram[CONTRA_RAM_ENEMY_Y_POS + slot] = 0x6Du;
    core.ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + slot] = 0x00u;
    core.ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + slot] = 0x00u;

    l2_inject_player_bullet(&core, 0x80u, 0x6Du);

    for (frame = 0u; frame < 48u; ++frame)
    {
        step_no_input(&core);
        if ((core.ram[CONTRA_RAM_ENEMY_TYPE + slot] == 0x15u) &&
            (core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] == 0x05u))
        {
            frozen = true; /* the soft-lock: stuck as a live type-0x15 at routine 5 */
        }
        if (core.ram[CONTRA_RAM_ENEMY_ROUTINE + slot] == 0x00u)
        {
            cleared = true;
        }
    }
    CHECK(!frozen);
    CHECK(cleared);
    return true;
}

/* Pressing Up into the live electric fence (screen not cleared) must show the
   electric-shock sprite 0x55 while ELECTROCUTED_TIMER runs -- the indoor sprite
   branch previously drew the facing-up sprite, so the shock never appeared. */
static bool test_level2_electrocution_shows_shock_sprite(void)
{
    ContraCore core;
    unsigned frame;
    bool shock = false;

    force_level2_gameplay(&core);
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

    for (frame = 0u; frame < 60u; ++frame)
    {
        step_with_input(&core, CONTRA_BUTTON_UP);
        if ((core.ram[CONTRA_RAM_ELECTROCUTED_TIMER] != 0u) &&
            (core.ram[CONTRA_RAM_PLAYER_SPRITE_CODE] == 0x55u))
        {
            shock = true;
            break;
        }
    }
    CHECK(shock);
    return true;
}

/* Walking sideways on the indoor (base) level must animate the player's legs
   (PLAYER_SPRITE_SEQUENCE 0x03 -> player_sprite_indoor_walking_animation), cycling
   the walk frames -- the indoor sprite branch previously left it on a static
   sprite. */
static bool test_level2_indoor_player_walk_animates(void)
{
    ContraCore core;
    unsigned frame;
    uint8_t seen[8];
    unsigned seen_count = 0u;

    force_level2_gameplay(&core);
    for (frame = 0u; frame < 90u; ++frame)
    {
        if ((core.ram[CONTRA_RAM_PLAYER_STATE] == 0x01u) &&
            (core.ram[CONTRA_RAM_PLAYER_JUMP_STATUS] == 0x00u) &&
            (core.ram[CONTRA_RAM_EDGE_FALL_CODE] == 0x00u))
        {
            break;
        }
        step_no_input(&core);
    }

    for (frame = 0u; frame < 48u; ++frame)
    {
        uint8_t sprite;
        unsigned i;
        bool known = false;

        step_with_input(&core, CONTRA_BUTTON_RIGHT);
        sprite = core.ram[CONTRA_RAM_PLAYER_SPRITE_CODE];
        for (i = 0u; i < seen_count; ++i)
        {
            if (seen[i] == sprite)
            {
                known = true;
            }
        }
        if (!known && (seen_count < (sizeof(seen) / sizeof(seen[0]))))
        {
            seen[seen_count++] = sprite;
        }
    }
    CHECK(seen_count >= 2u); /* the walk cycles through multiple frames */
    return true;
}

int main(void)
{
    const TestCase tests[] = {
        {"title_start_reaches_level1_gameplay", test_title_start_reaches_level1_gameplay},
        {"level1_scrolls_right_under_player_input", test_level1_scrolls_right_under_player_input},
        {"attract_level1_bridge_demo_keeps_p2_on_rom_route", test_attract_level1_bridge_demo_keeps_p2_on_rom_route},
        {"level1_forced_boss_clear_hands_off_to_level2", test_level1_forced_boss_clear_hands_off_to_level2},
        {"attract_reaches_level2_gameplay", test_attract_reaches_level2_gameplay},
        {"konami_code_in_intro_grants_30_lives", test_konami_code_in_intro_grants_30_lives},
        {"konami_code_ignored_after_intro_check_window", test_konami_code_ignored_after_intro_check_window},
        {"konami_code_ignored_during_gameplay", test_konami_code_ignored_during_gameplay},
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
        {"level2_room_advance_changes_render_state", test_level2_room_advance_changes_render_state},
        {"attract_level2_loads_wall_core_without_early_clear", test_attract_level2_loads_wall_core_without_early_clear},
        {"game_over_delay_expiry_loads_screen_without_glitch", test_game_over_delay_expiry_loads_screen_without_glitch},
        {"level2_repeated_room_advances_reach_boss_state", test_level2_repeated_room_advances_reach_boss_state},
        {"level2_boss_room_loads_rom_enemy_data", test_level2_boss_room_loads_rom_enemy_data},
        {"level2_boss_room_wall_cannons_fire_projectiles", test_level2_boss_room_wall_cannons_fire_projectiles},
        {"level4_first_room_loads_rom_enemy_data", test_level4_first_room_loads_rom_enemy_data},
        {"level4_first_room_renders_wall_core_target", test_level4_first_room_renders_wall_core_target},
        {"level4_debug_env_lives_and_weapon", test_level4_debug_env_lives_and_weapon},
        {"respawn_resets_weapon_without_keep_start_weapon", test_respawn_resets_weapon_without_keep_start_weapon},
        {"respawn_restores_start_weapon_with_keep_start_weapon", test_respawn_restores_start_weapon_with_keep_start_weapon},
        {"level4_multi_core_room_loads_rom_enemy_data", test_level4_multi_core_room_loads_rom_enemy_data},
        {"level4_boss_room_loads_rom_enemy_data", test_level4_boss_room_loads_rom_enemy_data},
        {"level4_boss_red_blue_generator_spawns_soldiers", test_level4_boss_red_blue_generator_spawns_soldiers},
        {"level4_boss_gemini_defeat_sets_boss_flag", test_level4_boss_gemini_defeat_sets_boss_flag},
        {"level5_ice_grenade_generator_uses_rom_props", test_level5_ice_grenade_generator_uses_rom_props},
        {"level5_ice_grenade_generator_spawns_ice_grenade", test_level5_ice_grenade_generator_spawns_ice_grenade},
        {"level5_specific_types_do_not_use_indoor_handlers", test_level5_specific_types_do_not_use_indoor_handlers},
        {"level6_fire_beam_uses_rom_props_and_init", test_level6_fire_beam_uses_rom_props_and_init},
        {"level6_left_and_right_beams_do_not_use_old_handlers", test_level6_left_and_right_beams_do_not_use_old_handlers},
        {"level6_boss_robot_not_misrouted_to_level3_or_level2", test_level6_boss_robot_not_misrouted_to_level3_or_level2},
        {"level2_indoor_enemy_spawn_y_pos_adjust", test_level2_indoor_enemy_spawn_y_pos_adjust},
        {"level2_indoor_player_bullet_spawn_geometry", test_level2_indoor_player_bullet_spawn_geometry},
        {"level2_indoor_player_bullet_despawns_on_timer", test_level2_indoor_player_bullet_despawns_on_timer},
        {"level2_indoor_fence_animates_chr", test_level2_indoor_fence_animates_chr},
        {"level2_killed_indoor_soldier_explodes_not_freezes", test_level2_killed_indoor_soldier_explodes_not_freezes},
        {"level2_electrocution_shows_shock_sprite", test_level2_electrocution_shows_shock_sprite},
        {"level2_indoor_player_walk_animates", test_level2_indoor_player_walk_animates},
        {"broad_weapon_gameover_and_alt_graphics_matrix", test_broad_weapon_gameover_and_alt_graphics_matrix},
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
