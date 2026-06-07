#define SDL_MAIN_HANDLED

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>

#include "contra/buttons.h"
#include "contra/core.h"

static ContraInputSnapshot contra_collect_keyboard_input(void)
{
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    ContraInputSnapshot input = {{0u, 0u}};

    if (keys[SDL_SCANCODE_RIGHT] != 0u)
    {
        input.player[0] |= CONTRA_BUTTON_RIGHT;
    }
    if (keys[SDL_SCANCODE_LEFT] != 0u)
    {
        input.player[0] |= CONTRA_BUTTON_LEFT;
    }
    if (keys[SDL_SCANCODE_DOWN] != 0u)
    {
        input.player[0] |= CONTRA_BUTTON_DOWN;
    }
    if (keys[SDL_SCANCODE_UP] != 0u)
    {
        input.player[0] |= CONTRA_BUTTON_UP;
    }
    if (keys[SDL_SCANCODE_RETURN] != 0u)
    {
        input.player[0] |= CONTRA_BUTTON_START;
    }
    if (keys[SDL_SCANCODE_RSHIFT] != 0u)
    {
        input.player[0] |= CONTRA_BUTTON_SELECT;
    }
    if ((keys[SDL_SCANCODE_LCTRL] != 0u) ||
        (keys[SDL_SCANCODE_Z] != 0u) ||
        (keys[SDL_SCANCODE_Y] != 0u) ||
        (keys[SDL_SCANCODE_S] != 0u))
    {
        input.player[0] |= CONTRA_BUTTON_B;
    }
    if ((keys[SDL_SCANCODE_SPACE] != 0u) ||
        (keys[SDL_SCANCODE_X] != 0u) ||
        (keys[SDL_SCANCODE_A] != 0u))
    {
        input.player[0] |= CONTRA_BUTTON_A;
    }

    return input;
}

int main(int argc, char **argv)
{
    ContraCore core;
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;
    uint64_t previous_counter;
    double counter_frequency;
    double accumulator_seconds = 0.0;
    const double frame_step_seconds = 1.0 / 60.0;
    bool running = true;

    (void)argc;
    (void)argv;

    SDL_SetMainReady();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

    window = SDL_CreateWindow(
        "Contra Native Port Scaffold",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        (int)(CONTRA_FRAMEBUFFER_WIDTH * 3u),
        (int)(CONTRA_FRAMEBUFFER_HEIGHT * 3u),
        SDL_WINDOW_SHOWN
    );

    if (window == NULL)
    {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    if (renderer == NULL)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, (int)CONTRA_FRAMEBUFFER_WIDTH, (int)CONTRA_FRAMEBUFFER_HEIGHT);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        (int)CONTRA_FRAMEBUFFER_WIDTH,
        (int)CONTRA_FRAMEBUFFER_HEIGHT
    );

    if (texture == NULL)
    {
        fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    contra_core_init(&core);

    /* DEBUG: CONTRA_START_LEVEL2_BOSS=1 warps straight to the Level 2 boss fight. */
    if (getenv("CONTRA_START_LEVEL2_BOSS") != NULL)
    {
        contra_core_debug_warp_level2_boss(&core);
    }
    else if (getenv("CONTRA_START_LEVEL4") != NULL)
    {
        contra_core_debug_warp_level4(&core);
    }

    previous_counter = (uint64_t)SDL_GetPerformanceCounter();
    counter_frequency = (double)SDL_GetPerformanceFrequency();

    while (running)
    {
        SDL_Event event;
        const uint64_t current_counter = (uint64_t)SDL_GetPerformanceCounter();
        const double delta_seconds = (double)(current_counter - previous_counter) / counter_frequency;
        ContraInputSnapshot input;
        int sim_steps = 0;

        previous_counter = current_counter;
        accumulator_seconds += delta_seconds;

        while (SDL_PollEvent(&event) != 0)
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if ((event.type == SDL_KEYDOWN) && (event.key.keysym.sym == SDLK_ESCAPE))
            {
                running = false;
            }
        }

        input = contra_collect_keyboard_input();
        contra_core_set_input(&core, &input);

        while ((accumulator_seconds >= frame_step_seconds) && (sim_steps < 5))
        {
            contra_core_step_frame(&core);
            accumulator_seconds -= frame_step_seconds;
            ++sim_steps;
        }

        SDL_UpdateTexture(
            texture,
            NULL,
            contra_core_framebuffer(&core),
            (int)(CONTRA_FRAMEBUFFER_WIDTH * sizeof(uint32_t))
        );

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        if (sim_steps == 0)
        {
            SDL_Delay(1);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
