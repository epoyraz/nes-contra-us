/*
 * Emscripten / WebAssembly host for the Contra native port.
 *
 * This mirrors platform_sdl/main.c, but instead of owning a window and an SDL
 * render loop it exposes a tiny C ABI that the browser shell (web/controller.js)
 * drives directly:
 *
 *   contra_web_init()              initialise the core. Loads baserom.nes from
 *                                  the Emscripten virtual filesystem (the build
 *                                  preloads it) for CHR / graphics data.
 *   contra_web_reset()             soft-reset the core.
 *   contra_web_set_input(p1)       latch the player-1 button bitmask
 *   contra_web_set_inputs(p1, p2)  latch both player button bitmasks
 *                                  (CONTRA_BUTTON_* from contra/buttons.h).
 *   contra_web_step()              advance one 60 Hz frame and refresh the
 *                                  canvas-ready RGBA framebuffer.
 *   contra_web_framebuffer()       pointer to a 256x240 RGBA8888 buffer suitable
 *                                  for CanvasRenderingContext2D.putImageData.
 *   contra_web_width()/height()    framebuffer dimensions.
 *   contra_web_warp_level2_boss()  DEBUG warp straight to the Level 2 boss.
 *   contra_web_warp_level4()       DEBUG warp straight to Level 4.
 *
 * The frame-pacing loop lives in JavaScript (requestAnimationFrame) so the page
 * stays in control of timing and can read the framebuffer straight out of wasm
 * linear memory.
 */

#include <stddef.h>
#include <stdint.h>

#include <emscripten/emscripten.h>

#include "contra/buttons.h"
#include "contra/core.h"

#define CONTRA_WEB_PIXELS (CONTRA_FRAMEBUFFER_WIDTH * CONTRA_FRAMEBUFFER_HEIGHT)

static ContraCore g_core;
static uint8_t g_input_player1;
static uint8_t g_input_player2;

/* Canvas-ready RGBA8888 copy of the core's ARGB framebuffer, refreshed each
   step. Static storage so its address is stable across memory growth. */
static uint8_t g_rgba[CONTRA_WEB_PIXELS * 4u];

EMSCRIPTEN_KEEPALIVE void contra_web_init(void)
{
    contra_core_init(&g_core);
    g_input_player1 = 0u;
    g_input_player2 = 0u;
}

EMSCRIPTEN_KEEPALIVE void contra_web_reset(void)
{
    contra_core_reset(&g_core);
    g_input_player1 = 0u;
    g_input_player2 = 0u;
}

EMSCRIPTEN_KEEPALIVE void contra_web_set_input(int player1_mask)
{
    g_input_player1 = (uint8_t)(player1_mask & 0xFF);
}

EMSCRIPTEN_KEEPALIVE void contra_web_set_inputs(int player1_mask, int player2_mask)
{
    g_input_player1 = (uint8_t)(player1_mask & 0xFF);
    g_input_player2 = (uint8_t)(player2_mask & 0xFF);
}

EMSCRIPTEN_KEEPALIVE uint32_t contra_web_state_hash(void)
{
    const uint8_t *const ram = contra_core_ram(&g_core);
    const uint32_t *const framebuffer = contra_core_framebuffer(&g_core);
    uint32_t hash = 2166136261u;
    size_t i;

    for (i = 0u; i < CONTRA_CPU_RAM_SIZE; ++i)
    {
        hash ^= ram[i];
        hash *= 16777619u;
    }

    for (i = 0u; i < (size_t)CONTRA_WEB_PIXELS; ++i)
    {
        const uint32_t pixel = framebuffer[i];
        hash ^= (uint8_t)(pixel & 0xFFu);
        hash *= 16777619u;
        hash ^= (uint8_t)((pixel >> 8) & 0xFFu);
        hash *= 16777619u;
        hash ^= (uint8_t)((pixel >> 16) & 0xFFu);
        hash *= 16777619u;
        hash ^= (uint8_t)((pixel >> 24) & 0xFFu);
        hash *= 16777619u;
    }

    return hash;
}

EMSCRIPTEN_KEEPALIVE void contra_web_step(void)
{
    ContraInputSnapshot input = {{g_input_player1, g_input_player2}};
    const uint32_t *framebuffer;
    size_t i;

    contra_core_set_input(&g_core, &input);
    contra_core_step_frame(&g_core);

    /* The core framebuffer is ARGB8888 (0xAARRGGBB); canvas ImageData expects
       RGBA byte order. Repack per pixel and force opaque alpha. */
    framebuffer = contra_core_framebuffer(&g_core);
    for (i = 0u; i < (size_t)CONTRA_WEB_PIXELS; ++i)
    {
        const uint32_t pixel = framebuffer[i];
        g_rgba[(i * 4u) + 0u] = (uint8_t)((pixel >> 16) & 0xFFu); /* R */
        g_rgba[(i * 4u) + 1u] = (uint8_t)((pixel >> 8) & 0xFFu);  /* G */
        g_rgba[(i * 4u) + 2u] = (uint8_t)(pixel & 0xFFu);         /* B */
        g_rgba[(i * 4u) + 3u] = 0xFFu;                            /* A */
    }
}

EMSCRIPTEN_KEEPALIVE const uint8_t *contra_web_framebuffer(void)
{
    return g_rgba;
}

EMSCRIPTEN_KEEPALIVE int contra_web_width(void)
{
    return (int)CONTRA_FRAMEBUFFER_WIDTH;
}

EMSCRIPTEN_KEEPALIVE int contra_web_height(void)
{
    return (int)CONTRA_FRAMEBUFFER_HEIGHT;
}

EMSCRIPTEN_KEEPALIVE void contra_web_warp_level2_boss(void)
{
    contra_core_debug_warp_level2_boss(&g_core);
}

EMSCRIPTEN_KEEPALIVE void contra_web_warp_level4(void)
{
    contra_core_debug_warp_level4(&g_core);
}

int main(void)
{
    /* Nothing to do at startup: the shell calls contra_web_init() once the
       module (and the preloaded baserom.nes) is ready. The runtime is kept
       alive via -sEXIT_RUNTIME=0 so the exported functions remain callable. */
    return 0;
}
