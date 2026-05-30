#include <stdbool.h>
#include <stdio.h>

#include "contra/buttons.h"
#include "contra/core.h"

static void write_u16_le(FILE *file, unsigned value)
{
    fputc((int)(value & 0xFFu), file);
    fputc((int)((value >> 8u) & 0xFFu), file);
}

static void write_u32_le(FILE *file, unsigned value)
{
    write_u16_le(file, value & 0xFFFFu);
    write_u16_le(file, value >> 16u);
}

static bool write_framebuffer_bmp(const char *path, const uint32_t *framebuffer)
{
    const unsigned pixel_data_size = CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT * 4u;
    const unsigned file_size = 14u + 40u + pixel_data_size;
    FILE *file = fopen(path, "wb");
    unsigned y;

    if (file == NULL)
    {
        return false;
    }

    fputc('B', file);
    fputc('M', file);
    write_u32_le(file, file_size);
    write_u16_le(file, 0u);
    write_u16_le(file, 0u);
    write_u32_le(file, 54u);

    write_u32_le(file, 40u);
    write_u32_le(file, CONTRA_FRAMEBUFFER_WIDTH);
    write_u32_le(file, CONTRA_FRAMEBUFFER_HEIGHT);
    write_u16_le(file, 1u);
    write_u16_le(file, 32u);
    write_u32_le(file, 0u);
    write_u32_le(file, pixel_data_size);
    write_u32_le(file, 2835u);
    write_u32_le(file, 2835u);
    write_u32_le(file, 0u);
    write_u32_le(file, 0u);

    for (y = CONTRA_FRAMEBUFFER_HEIGHT; y-- != 0u;)
    {
        unsigned x;

        for (x = 0u; x < CONTRA_FRAMEBUFFER_WIDTH; ++x)
        {
            const uint32_t rgba = framebuffer[(size_t)y * CONTRA_FRAMEBUFFER_WIDTH + (size_t)x];

            fputc((int)(rgba & 0xFFu), file);
            fputc((int)((rgba >> 8u) & 0xFFu), file);
            fputc((int)((rgba >> 16u) & 0xFFu), file);
            fputc((int)((rgba >> 24u) & 0xFFu), file);
        }
    }

    fclose(file);
    return true;
}

static unsigned count_unique_colors_in_region(
    const uint32_t *framebuffer,
    unsigned start_x,
    unsigned start_y,
    unsigned width,
    unsigned height
)
{
    uint32_t seen[64] = {0u};
    unsigned seen_count = 0u;
    unsigned y;

    for (y = start_y; y < (start_y + height); ++y)
    {
        unsigned x;

        for (x = start_x; x < (start_x + width); ++x)
        {
            const uint32_t color = framebuffer[(size_t)y * CONTRA_FRAMEBUFFER_WIDTH + (size_t)x];
            unsigned seen_index;
            bool already_seen = false;

            for (seen_index = 0u; seen_index < seen_count; ++seen_index)
            {
                if (seen[seen_index] == color)
                {
                    already_seen = true;
                    break;
                }
            }

            if ((!already_seen) && (seen_count < 64u))
            {
                seen[seen_count++] = color;
            }
        }
    }

    return seen_count;
}

int main(void)
{
    static ContraCore core;
    static ContraCore intro_core;
    static ContraCore late_intro_core;
    static ContraCore title_select_core;
    static ContraCore hud_core;
    static ContraCore gameplay_core;
    static ContraCore jump_core;
    static ContraCore bullet_core;
    static ContraCore game_over_core;
    ContraInputSnapshot input = {{0u, 0u}};
    const uint8_t *intro_ram;
    const uint8_t *late_intro_ram;
    const uint8_t *title_select_ram;
    const uint8_t *hud_ram;
    const uint8_t *gameplay_ram;
    const uint8_t *jump_ram;
    const uint8_t *ram;
    const uint8_t *bullet_ram;
    const uint8_t *game_over_ram;
    const uint32_t *intro_framebuffer;
    const uint32_t *late_intro_framebuffer;
    const uint32_t *title_select_framebuffer;
    const uint32_t *hud_framebuffer;
    const uint32_t *gameplay_framebuffer;
    const uint32_t *jump_framebuffer;
    const uint32_t *framebuffer;
    unsigned frame;
    unsigned bullet_index;
    unsigned previous_game_routine = 0xFFu;
    unsigned previous_level_routine = 0xFFu;
    unsigned forced_boss_frame = 0u;
    unsigned next_level_frame = 0u;
    unsigned stage2_intro_frame = 0u;

    contra_core_init(&intro_core);
    for (frame = 0u; frame < 272u; ++frame)
    {
        contra_core_set_input(&intro_core, &input);
        contra_core_step_frame(&intro_core);
    }

    intro_ram = contra_core_ram(&intro_core);
    intro_framebuffer = contra_core_framebuffer(&intro_core);
    printf(
        "intro frame=%u game_routine=%u hscroll=%u title_region_colors=%u cursor_region_colors=%u image=%s\n",
        (unsigned)intro_ram[CONTRA_RAM_FRAME_COUNTER],
        (unsigned)intro_ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
        (unsigned)intro_ram[CONTRA_RAM_HORIZONTAL_SCROLL],
        count_unique_colors_in_region(intro_framebuffer, 160u, 88u, 88u, 96u),
        count_unique_colors_in_region(intro_framebuffer, 36u, 154u, 24u, 24u),
        write_framebuffer_bmp("tmp/contra_intro.bmp", intro_framebuffer) ? "saved" : "failed"
    );

    contra_core_init(&late_intro_core);
    for (frame = 0u; frame < 400u; ++frame)
    {
        contra_core_set_input(&late_intro_core, &input);
        contra_core_step_frame(&late_intro_core);
    }

    late_intro_ram = contra_core_ram(&late_intro_core);
    late_intro_framebuffer = contra_core_framebuffer(&late_intro_core);
    printf(
        "lateintro frame=%u game_routine=%u sprite0=%02X sprite1=%02X title_region_colors=%u cursor_region_colors=%u image=%s\n",
        (unsigned)late_intro_ram[CONTRA_RAM_FRAME_COUNTER],
        (unsigned)late_intro_ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
        (unsigned)late_intro_ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u],
        (unsigned)late_intro_ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 1u],
        count_unique_colors_in_region(late_intro_framebuffer, 160u, 88u, 88u, 96u),
        count_unique_colors_in_region(late_intro_framebuffer, 36u, 154u, 24u, 24u),
        write_framebuffer_bmp("tmp/contra_intro_late.bmp", late_intro_framebuffer) ? "saved" : "failed"
    );

    contra_core_init(&title_select_core);
    {
        static const uint8_t expected_2_players[9] = {0x32u, 0x00u, 0x50u, 0x4Cu, 0x41u, 0x59u, 0x45u, 0x52u, 0x53u};
        unsigned visible_frames = 0u;
        unsigned blank_frames = 0u;
        unsigned title_select_capture_frame = 0u;

        input.player[0] = 0u;
        for (frame = 0u; frame < 48u; ++frame)
        {
            if (frame == 5u)
            {
                input.player[0] = CONTRA_BUTTON_START;
            }
            else if (frame == 6u)
            {
                input.player[0] = 0u;
            }
            else if (frame == 10u)
            {
                input.player[0] = CONTRA_BUTTON_SELECT;
            }
            else if (frame == 11u)
            {
                input.player[0] = 0u;
            }
            else if (frame == 20u)
            {
                input.player[0] = CONTRA_BUTTON_START;
            }
            else if (frame == 21u)
            {
                input.player[0] = 0u;
            }

            contra_core_set_input(&title_select_core, &input);
            contra_core_step_frame(&title_select_core);

            title_select_ram = contra_core_ram(&title_select_core);
            if (title_select_ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x03u)
            {
                const uint8_t *const title_text = &title_select_core.ppu_nametable[0x2C7u];
                unsigned char_index;
                bool matches_visible = true;
                bool matches_blank = true;

                for (char_index = 0u; char_index < 9u; ++char_index)
                {
                    if (title_text[char_index] != expected_2_players[char_index])
                    {
                        matches_visible = false;
                    }

                    if (title_text[char_index] != 0x00u)
                    {
                        matches_blank = false;
                    }
                }

                if (matches_visible)
                {
                    ++visible_frames;
                    title_select_capture_frame = frame + 1u;
                }

                if (matches_blank)
                {
                    ++blank_frames;
                }
            }
        }

        title_select_ram = contra_core_ram(&title_select_core);
        title_select_framebuffer = contra_core_framebuffer(&title_select_core);
        printf(
            "titleselect frame=%u game_routine=%u player_mode=%u play_select=%02X 2p=%02X/%02X/%02X visible_frames=%u blank_frames=%u image=%s\n",
            title_select_capture_frame,
            (unsigned)title_select_ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
            (unsigned)title_select_ram[CONTRA_RAM_PLAYER_MODE],
            (unsigned)title_select_core.ppu_nametable[0x28Au],
            (unsigned)title_select_core.ppu_nametable[0x2C7u],
            (unsigned)title_select_core.ppu_nametable[0x2C8u],
            (unsigned)title_select_core.ppu_nametable[0x2CFu],
            visible_frames,
            blank_frames,
            write_framebuffer_bmp("tmp/contra_title_select.bmp", title_select_framebuffer) ? "saved" : "failed"
        );
    }

    contra_core_init(&hud_core);
    {
        bool scores_seeded = false;

        input.player[0] = 0u;
        for (frame = 0u; frame < 380u; ++frame)
        {
            if (frame == 5u)
            {
                input.player[0] = CONTRA_BUTTON_START;
            }
            else if (frame == 6u)
            {
                input.player[0] = 0u;
            }
            else if (frame == 10u)
            {
                input.player[0] = CONTRA_BUTTON_SELECT;
            }
            else if (frame == 11u)
            {
                input.player[0] = 0u;
            }
            else if (frame == 20u)
            {
                input.player[0] = CONTRA_BUTTON_START;
            }
            else if (frame == 21u)
            {
                input.player[0] = 0u;
            }

            contra_core_set_input(&hud_core, &input);
            contra_core_step_frame(&hud_core);

            hud_ram = contra_core_ram(&hud_core);
            if ((!scores_seeded) &&
                (hud_ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
                (hud_ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x01u))
            {
                hud_core.ram[CONTRA_RAM_HIGH_SCORE_LOW] = 0x03u;
                hud_core.ram[CONTRA_RAM_HIGH_SCORE_HIGH] = 0x00u;
                hud_core.ram[CONTRA_RAM_PLAYER_1_SCORE_LOW] = 0x01u;
                hud_core.ram[CONTRA_RAM_PLAYER_1_SCORE_HIGH] = 0x00u;
                hud_core.ram[CONTRA_RAM_PLAYER_2_SCORE_LOW] = 0x02u;
                hud_core.ram[CONTRA_RAM_PLAYER_2_SCORE_HIGH] = 0x00u;
                scores_seeded = true;
            }

            if ((hud_ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
                (hud_ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x02u) &&
                ((hud_ram[CONTRA_RAM_FRAME_COUNTER] & 0x10u) == 0u))
            {
                break;
            }
        }

        hud_ram = contra_core_ram(&hud_core);
        hud_framebuffer = contra_core_framebuffer(&hud_core);
        printf(
            "hud frame=%u game_routine=%u level_routine=%u stage=%02X/%02X stage_num=%02X level_name=%02X lives=%02X/%02X hi=%02X/%02X/%02X p1=%02X/%02X/%02X p2=%02X/%02X/%02X image=%s\n",
            frame + 1u,
            (unsigned)hud_ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
            (unsigned)hud_ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
            (unsigned)hud_core.ppu_nametable[0x20Cu],
            (unsigned)hud_core.ppu_nametable[0x210u],
            (unsigned)hud_core.ppu_nametable[0x212u],
            (unsigned)hud_core.ppu_nametable[0x24Cu],
            (unsigned)hud_core.ppu_nametable[0x0C8u],
            (unsigned)hud_core.ppu_nametable[0x0D8u],
            (unsigned)hud_core.ppu_nametable[0x131u],
            (unsigned)hud_core.ppu_nametable[0x132u],
            (unsigned)hud_core.ppu_nametable[0x133u],
            (unsigned)hud_core.ppu_nametable[0x089u],
            (unsigned)hud_core.ppu_nametable[0x08Au],
            (unsigned)hud_core.ppu_nametable[0x08Bu],
            (unsigned)hud_core.ppu_nametable[0x099u],
            (unsigned)hud_core.ppu_nametable[0x09Au],
            (unsigned)hud_core.ppu_nametable[0x09Bu],
            write_framebuffer_bmp("tmp/contra_hud_transition.bmp", hud_framebuffer) ? "saved" : "failed"
        );
    }

    contra_core_init(&gameplay_core);
    {
        unsigned gameplay_start_frame = 0u;
        unsigned initial_x = 0u;
        unsigned initial_y = 0u;
        unsigned final_x = 0u;
        unsigned final_y = 0u;
        unsigned sprite_code = 0u;
        unsigned initial_screen = 0u;
        unsigned initial_scroll = 0u;
        unsigned final_screen = 0u;
        unsigned final_scroll = 0u;
        unsigned final_hscroll = 0u;
        unsigned scroll_start_frame = 0u;
        unsigned screen_advance_frame = 0u;
        unsigned final_enemy_read_offset = 0u;
        bool reached_gameplay = false;
        bool started_scrolling = false;
        bool advanced_screen = false;

        input.player[0] = 0u;
        for (frame = 0u; frame < 1500u; ++frame)
        {
            if (frame == 5u)
            {
                input.player[0] = CONTRA_BUTTON_START;
            }
            else if (frame == 6u)
            {
                input.player[0] = 0u;
            }
            else if (frame == 20u)
            {
                input.player[0] = CONTRA_BUTTON_START;
            }
            else if (frame == 21u)
            {
                input.player[0] = 0u;
            }

            gameplay_ram = contra_core_ram(&gameplay_core);
            if ((gameplay_ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
                (gameplay_ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x04u))
            {
                if (!reached_gameplay)
                {
                    reached_gameplay = true;
                    gameplay_start_frame = frame;
                    initial_x = gameplay_ram[CONTRA_RAM_SPRITE_X_POS + 0u];
                    initial_y = gameplay_ram[CONTRA_RAM_SPRITE_Y_POS + 0u];
                    initial_screen = gameplay_ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
                    initial_scroll = gameplay_ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
                }

                if ((frame - gameplay_start_frame) < 512u)
                {
                    input.player[0] = CONTRA_BUTTON_RIGHT;
                }
                else if ((frame - gameplay_start_frame) < 544u)
                {
                    input.player[0] = 0u;
                }
                else
                {
                    input.player[0] = 0u;
                    break;
                }
            }

            contra_core_set_input(&gameplay_core, &input);
            contra_core_step_frame(&gameplay_core);

            if (reached_gameplay && !started_scrolling)
            {
                gameplay_ram = contra_core_ram(&gameplay_core);

                if ((gameplay_ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen) ||
                    (gameplay_ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET] != initial_scroll) ||
                    (gameplay_ram[CONTRA_RAM_HORIZONTAL_SCROLL] != 0u))
                {
                    started_scrolling = true;
                    scroll_start_frame = frame + 1u;
                }
            }

            if (reached_gameplay && !advanced_screen)
            {
                gameplay_ram = contra_core_ram(&gameplay_core);
                if (gameplay_ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] != initial_screen)
                {
                    advanced_screen = true;
                    screen_advance_frame = frame + 1u;
                }
            }
        }

        gameplay_ram = contra_core_ram(&gameplay_core);
        gameplay_framebuffer = contra_core_framebuffer(&gameplay_core);
        final_x = gameplay_ram[CONTRA_RAM_SPRITE_X_POS + 0u];
        final_y = gameplay_ram[CONTRA_RAM_SPRITE_Y_POS + 0u];
        sprite_code = gameplay_ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u];
        final_screen = gameplay_ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
        final_scroll = gameplay_ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
        final_hscroll = gameplay_ram[CONTRA_RAM_HORIZONTAL_SCROLL];
        final_enemy_read_offset = gameplay_ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET];
        printf(
            "gameplay frame=%u start=%u scroll_start=%u screen_advance=%u routine=%u/%u p1state=%u sprite=%02X pos=(%u,%u)->(%u,%u) "
            "screen=%u->%u scroll=%u->%u hscroll=%u enemy_read=%u aim=%u jump=%u image=%s\n",
            frame + 1u,
            gameplay_start_frame,
            scroll_start_frame,
            screen_advance_frame,
            (unsigned)gameplay_ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
            (unsigned)gameplay_ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
            (unsigned)gameplay_ram[CONTRA_RAM_PLAYER_STATE + 0u],
            sprite_code,
            initial_x,
            initial_y,
            final_x,
            final_y,
            initial_screen,
            final_screen,
            initial_scroll,
            final_scroll,
            final_hscroll,
            final_enemy_read_offset,
            (unsigned)gameplay_ram[CONTRA_RAM_PLAYER_AIM_DIR + 0u],
            (unsigned)gameplay_ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 0u],
            write_framebuffer_bmp("tmp/contra_gameplay_scroll.bmp", gameplay_framebuffer) ? "saved" : "failed"
        );
    }

    contra_core_init(&jump_core);
    {
        unsigned jump_start_frame = 0u;
        unsigned jump_trigger_frame = 0u;
        unsigned landing_frame = 0u;
        unsigned jump_initial_y = 0u;
        unsigned jump_final_y = 0u;
        unsigned hidden_during_jump = 0u;
        unsigned jump_final_state = 0u;
        unsigned jump_final_lives = 0u;
        unsigned jump_final_game_over = 0u;
        bool reached_gameplay = false;
        bool reached_grounded_state = false;
        bool jump_started = false;

        input.player[0] = 0u;
        for (frame = 0u; frame < 1200u; ++frame)
        {
            if (frame == 5u)
            {
                input.player[0] = CONTRA_BUTTON_START;
            }
            else if (frame == 6u)
            {
                input.player[0] = 0u;
            }
            else if (frame == 20u)
            {
                input.player[0] = CONTRA_BUTTON_START;
            }
            else if (frame == 21u)
            {
                input.player[0] = 0u;
            }

            jump_ram = contra_core_ram(&jump_core);
            if ((jump_ram[CONTRA_RAM_GAME_ROUTINE_INDEX] == 0x05u) &&
                (jump_ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u))
            {
                if (!reached_gameplay)
                {
                    reached_gameplay = true;
                    jump_start_frame = frame;
                }

                if (!reached_grounded_state &&
                    (jump_ram[CONTRA_RAM_PLAYER_STATE + 0u] == 0x01u) &&
                    (jump_ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 0u] == 0x00u) &&
                    (jump_ram[CONTRA_RAM_EDGE_FALL_CODE + 0u] == 0x00u))
                {
                    reached_grounded_state = true;
                    jump_initial_y = jump_ram[CONTRA_RAM_SPRITE_Y_POS + 0u];
                    landing_frame = frame;
                }

                if (reached_grounded_state && ((frame - landing_frame) == 8u))
                {
                    input.player[0] = CONTRA_BUTTON_A;
                    jump_trigger_frame = frame + 1u;
                }
                else if (reached_grounded_state && ((frame - landing_frame) == 9u))
                {
                    input.player[0] = 0u;
                }
            }

            contra_core_set_input(&jump_core, &input);
            contra_core_step_frame(&jump_core);

            if (reached_gameplay)
            {
                jump_ram = contra_core_ram(&jump_core);
                if ((jump_trigger_frame != 0u) &&
                    (jump_ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 0u] != 0u))
                {
                    jump_started = true;
                }

                if (jump_started)
                {
                    if (jump_ram[CONTRA_RAM_PLAYER_HIDDEN + 0u] > hidden_during_jump)
                    {
                        hidden_during_jump = jump_ram[CONTRA_RAM_PLAYER_HIDDEN + 0u];
                    }

                    if ((jump_ram[CONTRA_RAM_PLAYER_STATE + 0u] != 0x01u) ||
                        ((jump_ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 0u] == 0u) &&
                         (jump_ram[CONTRA_RAM_EDGE_FALL_CODE + 0u] == 0u) &&
                         ((frame + 1u) > jump_trigger_frame)))
                    {
                        break;
                    }

                    if (((frame + 1u) > jump_trigger_frame) &&
                        (((frame + 1u) - jump_trigger_frame) > 120u))
                    {
                        break;
                    }
                }

            }
        }

        jump_ram = contra_core_ram(&jump_core);
        jump_framebuffer = contra_core_framebuffer(&jump_core);
        jump_final_y = jump_ram[CONTRA_RAM_SPRITE_Y_POS + 0u];
        jump_final_state = jump_ram[CONTRA_RAM_PLAYER_STATE + 0u];
        jump_final_lives = jump_ram[CONTRA_RAM_P1_NUM_LIVES];
        jump_final_game_over = jump_ram[CONTRA_RAM_P1_GAME_OVER_STATUS];
        printf(
            "jump frame=%u gameplay_start=%u grounded=%u jump_start=%u hidden=%u y=%u->%u state=%u lives=%u gameover=%u sprite=%02X image=%s\n",
            frame + 1u,
            jump_start_frame,
            landing_frame,
            jump_trigger_frame,
            hidden_during_jump,
            jump_initial_y,
            jump_final_y,
            jump_final_state,
            jump_final_lives,
            jump_final_game_over,
            (unsigned)jump_ram[CONTRA_RAM_CPU_SPRITE_BUFFER + 0u],
            write_framebuffer_bmp("tmp/contra_gameplay_jump.bmp", jump_framebuffer) ? "saved" : "failed"
        );
    }

    contra_core_init(&core);
    input.player[0] = 0u;

    for (frame = 0u; frame < 1400u; ++frame)
    {
        if (frame == 5u)
        {
            input.player[0] = CONTRA_BUTTON_START; /* Skip the intro scroll. */
        }
        else if (frame == 6u)
        {
            input.player[0] = 0u;
        }
        else if (frame == 10u)
        {
            input.player[0] = CONTRA_BUTTON_SELECT; /* Toggle to 2-player mode on the select screen. */
        }
        else if (frame == 11u)
        {
            input.player[0] = 0u;
        }
        else if (frame == 20u)
        {
            input.player[0] = CONTRA_BUTTON_START; /* Start the game from game_routine_01. */
        }
        else if (frame == 21u)
        {
            input.player[0] = 0u;
        }

        if (frame == 579u)
        {
            core.ram[CONTRA_RAM_BOSS_DEFEATED_FLAG] = 0x01u; /* Force the first level_routine_04 branch. */
            forced_boss_frame = frame + 1u;
        }

        contra_core_set_input(&core, &input);
        contra_core_step_frame(&core);

        ram = contra_core_ram(&core);
        if ((next_level_frame == 0u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u))
        {
            next_level_frame = frame + 1u;
        }

        if ((stage2_intro_frame == 0u) &&
            (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) &&
            (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x02u))
        {
            stage2_intro_frame = frame + 1u;
            break;
        }

        if (((unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INDEX] != previous_game_routine) ||
            ((unsigned)ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] != previous_level_routine))
        {
            printf(
                "step frame=%u game_routine=%u level_routine=%u current_level=%u loc=%u scroll_type=%u delay=(%u,%u) gfx=%u vscroll=%u transition=%u sprite_load=%u boss=%u\n",
                frame + 1u,
                (unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
                (unsigned)ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
                (unsigned)ram[CONTRA_RAM_CURRENT_LEVEL],
                (unsigned)ram[CONTRA_RAM_LEVEL_LOCATION_TYPE],
                (unsigned)ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE],
                (unsigned)ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE],
                (unsigned)ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE],
                (unsigned)ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE],
                (unsigned)ram[CONTRA_RAM_VERTICAL_SCROLL],
                (unsigned)ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER],
                (unsigned)ram[CONTRA_RAM_SPRITE_LOAD_TYPE],
                (unsigned)ram[CONTRA_RAM_BOSS_DEFEATED_FLAG]
            );
            previous_game_routine = (unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INDEX];
            previous_level_routine = (unsigned)ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX];
        }
    }

    ram = contra_core_ram(&core);

    printf(
        "bossclear forced=%u next_level=%u stage2_intro=%u\n",
        forced_boss_frame,
        next_level_frame,
        stage2_intro_frame
    );

    printf(
        "frame=%u game_routine=%u level_routine=%u init=%u current_level=%u loc=%u scroll_type=%u player_mode=%u p1d=%u lives=(%u,%u) continues=%u scroll=(%u,%u) intro=%u delay=(%u,%u) gfx=%u transition=%u sprite_load=%u boss=%u ppu=(%u,%u)\n",
        (unsigned)ram[CONTRA_RAM_FRAME_COUNTER],
        (unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
        (unsigned)ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
        (unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG],
        (unsigned)ram[CONTRA_RAM_CURRENT_LEVEL],
        (unsigned)ram[CONTRA_RAM_LEVEL_LOCATION_TYPE],
        (unsigned)ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE],
        (unsigned)ram[CONTRA_RAM_PLAYER_MODE],
        (unsigned)ram[CONTRA_RAM_PLAYER_MODE_1D],
        (unsigned)ram[CONTRA_RAM_P1_NUM_LIVES],
        (unsigned)ram[CONTRA_RAM_P2_NUM_LIVES],
        (unsigned)ram[CONTRA_RAM_NUM_CONTINUES],
        (unsigned)ram[CONTRA_RAM_HORIZONTAL_SCROLL],
        (unsigned)ram[CONTRA_RAM_VERTICAL_SCROLL],
        (unsigned)ram[CONTRA_RAM_INTRO_THEME_DELAY],
        (unsigned)ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE],
        (unsigned)ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE],
        (unsigned)ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE],
        (unsigned)ram[CONTRA_RAM_LEVEL_TRANSITION_TIMER],
        (unsigned)ram[CONTRA_RAM_SPRITE_LOAD_TYPE],
        (unsigned)ram[CONTRA_RAM_BOSS_DEFEATED_FLAG],
        (unsigned)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE],
        (unsigned)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE]
    );

    framebuffer = contra_core_framebuffer(&core);
    {
        unsigned sampled_colors = 0u;
        unsigned non_black = 0u;
        unsigned total = 0u;
        uint32_t seen[256] = {0u};
        unsigned y;

        for (y = 24u; y < 216u; y += 8u)
        {
            unsigned x;

            for (x = 24u; x < 232u; x += 8u)
            {
                const uint32_t color = framebuffer[(size_t)y * CONTRA_FRAMEBUFFER_WIDTH + (size_t)x];
                unsigned seen_index;
                bool already_seen = false;

                for (seen_index = 0u; seen_index < sampled_colors; ++seen_index)
                {
                    if (seen[seen_index] == color)
                    {
                        already_seen = true;
                        break;
                    }
                }

                if ((!already_seen) && (sampled_colors < 256u))
                {
                    seen[sampled_colors++] = color;
                }

                if ((color & 0x00FFFFFFu) != 0u)
                {
                    ++non_black;
                }

                ++total;
            }
        }

        printf(
            "framebuffer sampled_colors=%u nonblack_ratio=%u/%u\n",
            sampled_colors,
            non_black,
            total
        );
    }

    contra_core_init(&bullet_core);
    bullet_core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    bullet_core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x04u;
    bullet_core.ram[CONTRA_RAM_PPU_READY] = 0x00u;
    bullet_core.ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
    bullet_core.ram[CONTRA_RAM_PAUSE_STATE] = 0x00u;
    bullet_core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x00u;
    bullet_core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;
    bullet_core.ram[CONTRA_RAM_INDOOR_SCROLL] = 0x02u;
    bullet_core.ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00] = 0x01u;
    bullet_core.ram[CONTRA_RAM_FRAME_COUNTER] = 0x01u; /* Will advance to an even frame before bullets are copied. */

    for (bullet_index = 0u; bullet_index < 16u; ++bullet_index)
    {
        bullet_core.ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + bullet_index] = (uint8_t)(0x40u + bullet_index);
        bullet_core.ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_ATTR + bullet_index] = (uint8_t)(0x80u + bullet_index);
        bullet_core.ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + bullet_index] = (uint8_t)(0x20u + bullet_index);
        bullet_core.ram[CONTRA_RAM_PLAYER_BULLET_X_POS + bullet_index] = (uint8_t)(0x60u + bullet_index);
    }

    contra_core_set_input(&bullet_core, &input);
    contra_core_step_frame(&bullet_core);
    bullet_ram = contra_core_ram(&bullet_core);

    printf(
        "bullet frame=%u frame_scroll=%u indoor_scroll=%u attack=%u weapon=%u bitfield=%u codes=",
        (unsigned)bullet_ram[CONTRA_RAM_FRAME_COUNTER],
        (unsigned)bullet_ram[CONTRA_RAM_FRAME_SCROLL],
        (unsigned)bullet_ram[CONTRA_RAM_INDOOR_SCROLL],
        (unsigned)bullet_ram[CONTRA_RAM_ENEMY_ATTACK_FLAG],
        (unsigned)bullet_ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH],
        (unsigned)bullet_ram[CONTRA_RAM_PLAYER_GAME_OVER_BIT_FIELD]
    );

    for (bullet_index = 2u; bullet_index < 10u; ++bullet_index)
    {
        printf(
            "%s%02X/%02X/%02X/%02X",
            bullet_index == 2u ? "" : ",",
            (unsigned)bullet_ram[CONTRA_RAM_CPU_SPRITE_BUFFER + bullet_index],
            (unsigned)bullet_ram[CONTRA_RAM_SPRITE_ATTR + bullet_index],
            (unsigned)bullet_ram[CONTRA_RAM_SPRITE_Y_POS + bullet_index],
            (unsigned)bullet_ram[CONTRA_RAM_SPRITE_X_POS + bullet_index]
        );
    }

    printf("\n");

    contra_core_init(&game_over_core);
    game_over_core.ram[CONTRA_RAM_GAME_ROUTINE_INDEX] = 0x05u;
    game_over_core.ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] = 0x04u;
    game_over_core.ram[CONTRA_RAM_PPU_READY] = 0x00u;
    game_over_core.ram[CONTRA_RAM_DEMO_MODE] = 0x00u;
    game_over_core.ram[CONTRA_RAM_PAUSE_STATE] = 0x00u;
    game_over_core.ram[CONTRA_RAM_P1_GAME_OVER_STATUS] = 0x01u;
    game_over_core.ram[CONTRA_RAM_P2_GAME_OVER_STATUS] = 0x01u;

    contra_core_set_input(&game_over_core, &input);
    contra_core_step_frame(&game_over_core);

    for (frame = 0u; frame < 96u; ++frame)
    {
        contra_core_set_input(&game_over_core, &input);
        contra_core_step_frame(&game_over_core);
    }

    game_over_ram = contra_core_ram(&game_over_core);
    printf(
        "gameover frame=%u level_routine=%u game_over_delay=%u boss=%u end_level=%u\n",
        (unsigned)game_over_ram[CONTRA_RAM_FRAME_COUNTER],
        (unsigned)game_over_ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
        (unsigned)game_over_ram[CONTRA_RAM_GAME_OVER_DELAY_TIMER],
        (unsigned)game_over_ram[CONTRA_RAM_BOSS_DEFEATED_FLAG],
        (unsigned)game_over_ram[CONTRA_RAM_END_LEVEL_ROUTINE_INDEX]
    );

    return 0;
}
