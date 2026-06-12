/* Replays a Mesen play recording (tools/mesen_play_recorder.lua) through the
 * native core and emits the same per-frame JSONL schema as
 * level1_frame_trace.c, so tools/compare_play_trace.py can diff the native
 * port against the real ROM frame-by-frame on arbitrary human input.
 *
 * Usage: contra_play_replay_trace RECORDING.jsonl > native_play_trace.jsonl
 *
 * Env:
 *   CONTRA_NATIVE_PLAY_INPUT         "raw" (default, hardware input) or
 *                                    "latched" ($F1/$F2 the game consumed;
 *                                    use when a DPCM controller-read glitch
 *                                    corrupted a raw frame in Mesen)
 *   CONTRA_NATIVE_PLAY_INPUT_OFFSET  add N (may be negative) to the recording
 *                                    frame used for each native frame, for
 *                                    recordings that did not start exactly at
 *                                    power-on frame 1
 *   CONTRA_NATIVE_PLAY_RNG           "free" disables injecting the recorded
 *                                    RANDOM_NUM ($34) value each frame (the
 *                                    ROM's between-frame busy-loop RNG is
 *                                    cycle-dependent and unreproducible, so
 *                                    injection is the default)
 *   CONTRA_NATIVE_PLAY_MAX_FRAME     stop after N frames (default: recording length)
 *   CONTRA_NATIVE_PLAY_DUMP_FRAME    with the paths below, dump full state at one frame
 *   CONTRA_NATIVE_PLAY_RAM_DUMP_PATH / OAM / NAMETABLE / PALETTE / FRAMEBUFFER /
 *   CHR / PPU / SUPERTILE _DUMP_PATH
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contra/core.h"

typedef struct PlayInputRow
{
    uint8_t p1_raw;
    uint8_t p2_raw;
    uint8_t p1_latched;
    uint8_t p2_latched;
    uint8_t frame_counter;
    uint8_t rng;
    uint8_t has_row;
    uint8_t has_raw;
    uint8_t has_frame_counter;
    uint8_t has_rng;
} PlayInputRow;

enum
{
    ALIGNMENT_WINDOW = 240,
    ALIGNMENT_MAX_SHIFT = 3,
    NATIVE_PPU_DUMP_SIZE = 0x4000u
};

#define SNAPSHOT_MAGIC 0x504E5343u /* "CSNP" little-endian */

typedef struct SnapshotHeader
{
    uint32_t magic;
    uint32_t version;
    uint32_t frame;
    uint32_t core_size;
} SnapshotHeader;

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

static void fill_ppu_dump(uint8_t *dump, const ContraCore *core)
{
    unsigned addr;

    for (addr = 0u; addr < NATIVE_PPU_DUMP_SIZE; ++addr)
    {
        if (addr < CONTRA_PPU_PATTERN_TABLE_SIZE)
        {
            dump[addr] = core->ppu_pattern[addr];
        }
        else if (addr < 0x3F00u)
        {
            dump[addr] = core->ppu_nametable[
                (addr - 0x2000u) & (CONTRA_PPU_NAMETABLE_SIZE - 1u)];
        }
        else
        {
            dump[addr] = core->ppu_palette[
                (addr - 0x3F00u) & (CONTRA_PPU_PALETTE_SIZE - 1u)];
        }
    }
}

static bool extract_unsigned(const char *line, const char *key, unsigned *out)
{
    char needle[64];
    const char *pos;

    if (snprintf(needle, sizeof(needle), "\"%s\":", key) >= (int)sizeof(needle))
    {
        return false;
    }

    pos = strstr(line, needle);
    if (pos == NULL)
    {
        return false;
    }

    *out = (unsigned)strtoul(pos + strlen(needle), NULL, 10);
    return true;
}

static PlayInputRow *load_recording(const char *path, unsigned *out_last_frame, bool *out_digest_v2)
{
    static char line[16384];
    FILE *const fp = fopen(path, "r");
    PlayInputRow *rows = NULL;
    size_t capacity = 0u;
    unsigned last_frame = 0u;

    if (fp == NULL)
    {
        fprintf(stderr, "FAIL could not open recording %s\n", path);
        return NULL;
    }

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        unsigned frame;
        unsigned value;
        PlayInputRow row;

        if (!extract_unsigned(line, "frame", &frame) || (frame == 0u))
        {
            continue;
        }

        /* Recordings that carry the weapon field also list active type-0x00
           (weapon item) slots in the enemies digest; emit the same digest. */
        if (strstr(line, "\"weapon\":") != NULL)
        {
            *out_digest_v2 = true;
        }

        memset(&row, 0, sizeof(row));
        row.has_row = 1u;

        if (extract_unsigned(line, "p1_raw", &value))
        {
            row.p1_raw = (uint8_t)value;
            row.has_raw = 1u;
            if (extract_unsigned(line, "p2_raw", &value))
            {
                row.p2_raw = (uint8_t)value;
            }
        }
        if (extract_unsigned(line, "controller", &value))
        {
            row.p1_latched = (uint8_t)value;
            if (extract_unsigned(line, "p2_controller", &value))
            {
                row.p2_latched = (uint8_t)value;
            }
        }
        if (extract_unsigned(line, "frame_counter", &value))
        {
            row.frame_counter = (uint8_t)value;
            row.has_frame_counter = 1u;
        }
        if (extract_unsigned(line, "rng", &value))
        {
            row.rng = (uint8_t)value;
            row.has_rng = 1u;
        }

        if ((size_t)frame >= capacity)
        {
            size_t new_capacity = (capacity == 0u) ? 4096u : capacity;
            PlayInputRow *grown;

            while ((size_t)frame >= new_capacity)
            {
                new_capacity *= 2u;
            }

            grown = (PlayInputRow *)realloc(rows, new_capacity * sizeof(*rows));
            if (grown == NULL)
            {
                fprintf(stderr, "FAIL out of memory loading recording\n");
                free(rows);
                fclose(fp);
                return NULL;
            }
            memset(grown + capacity, 0, (new_capacity - capacity) * sizeof(*rows));
            rows = grown;
            capacity = new_capacity;
        }

        rows[frame] = row;
        if (frame > last_frame)
        {
            last_frame = frame;
        }
    }

    fclose(fp);

    if (last_frame == 0u)
    {
        fprintf(stderr, "FAIL recording %s contains no frame rows\n", path);
        free(rows);
        return NULL;
    }

    *out_last_frame = last_frame;
    return rows;
}

/* The game's own NMI counter ($1A) ticks identically from power-on on both
 * sides, so a constant mismatch in the first frames means the recording is
 * shifted relative to power-on, not that the port diverged. Detect the shift
 * and tell the user which CONTRA_NATIVE_PLAY_INPUT_OFFSET repairs it. */
static void report_alignment(
    const PlayInputRow *rows,
    unsigned last_frame,
    long input_offset,
    const uint8_t *native_fc,
    unsigned native_fc_count)
{
    long best_shift = 0;
    unsigned best_matches = 0u;
    unsigned zero_matches = 0u;
    unsigned checked = 0u;
    long shift;

    for (shift = -ALIGNMENT_MAX_SHIFT; shift <= ALIGNMENT_MAX_SHIFT; ++shift)
    {
        unsigned matches = 0u;
        unsigned considered = 0u;
        unsigned frame;

        for (frame = 1u; frame <= native_fc_count; ++frame)
        {
            const long source = (long)frame + input_offset + shift;

            if ((source < 1) || (source > (long)last_frame))
            {
                continue;
            }
            if (!rows[source].has_row || !rows[source].has_frame_counter)
            {
                continue;
            }

            ++considered;
            if (rows[source].frame_counter == native_fc[frame - 1u])
            {
                ++matches;
            }
        }

        if (shift == 0)
        {
            zero_matches = matches;
            checked = considered;
        }
        if (matches > best_matches)
        {
            best_matches = matches;
            best_shift = shift;
        }
    }

    if (checked < (ALIGNMENT_WINDOW / 2u))
    {
        return; /* recording has no frame_counter data to check against */
    }

    if (zero_matches >= ((checked * 9u) / 10u))
    {
        return;
    }

    fprintf(
        stderr,
        "WARNING frame alignment: only %u/%u frame_counter matches in the first %u frames",
        zero_matches, checked, native_fc_count);
    if ((best_shift != 0) && (best_matches >= ((checked * 9u) / 10u)))
    {
        fprintf(
            stderr,
            "; recording looks shifted by %ld frame(s) -- retry with CONTRA_NATIVE_PLAY_INPUT_OFFSET=%ld",
            best_shift, input_offset + best_shift);
    }
    fprintf(stderr, "\n");
}

int main(int argc, char **argv)
{
    ContraCore core;
    const char *const input_mode_text = getenv("CONTRA_NATIVE_PLAY_INPUT");
    const char *const input_offset_text = getenv("CONTRA_NATIVE_PLAY_INPUT_OFFSET");
    const char *const max_frame_text = getenv("CONTRA_NATIVE_PLAY_MAX_FRAME");
    const char *const dump_frame_text = getenv("CONTRA_NATIVE_PLAY_DUMP_FRAME");
    const char *const ram_dump_path = getenv("CONTRA_NATIVE_PLAY_RAM_DUMP_PATH");
    const char *const oam_dump_path = getenv("CONTRA_NATIVE_PLAY_OAM_DUMP_PATH");
    const char *const nametable_dump_path = getenv("CONTRA_NATIVE_PLAY_NAMETABLE_DUMP_PATH");
    const char *const palette_dump_path = getenv("CONTRA_NATIVE_PLAY_PALETTE_DUMP_PATH");
    const char *const framebuffer_dump_path = getenv("CONTRA_NATIVE_PLAY_FRAMEBUFFER_DUMP_PATH");
    const char *const chr_dump_path = getenv("CONTRA_NATIVE_PLAY_CHR_DUMP_PATH");
    const char *const ppu_dump_path = getenv("CONTRA_NATIVE_PLAY_PPU_DUMP_PATH");
    const char *const supertile_dump_path = getenv("CONTRA_NATIVE_PLAY_SUPERTILE_DUMP_PATH");
    const char *const rng_mode_text = getenv("CONTRA_NATIVE_PLAY_RNG");
    /* side-channel score trace ("frame score16" per line, P1 score at $07E2/3);
       pairs with the recorder's CONTRA_MESEN_PLAY_SCORE_TRACE_PATH */
    const char *const score_trace_path = getenv("CONTRA_NATIVE_PLAY_SCORE_TRACE_PATH");
    FILE *score_trace = (score_trace_path != NULL) ? fopen(score_trace_path, "w") : NULL;
    /* core snapshots: write the flat ContraCore every N frames; resume from a
       snapshot to iterate at a deep frontier in seconds instead of replaying
       from power-on (full replay still required before trusting/committing) */
    const char *const snapshot_every_text = getenv("CONTRA_NATIVE_PLAY_SNAPSHOT_EVERY");
    const char *const snapshot_dir = getenv("CONTRA_NATIVE_PLAY_SNAPSHOT_DIR");
    const char *const resume_path = getenv("CONTRA_NATIVE_PLAY_RESUME");
    const unsigned snapshot_every =
        (snapshot_every_text != NULL) ? (unsigned)strtoul(snapshot_every_text, NULL, 10) : 0u;
    /* CONTRA_NATIVE_PLAY_DUMP_FRAMES: comma-separated list; each dump file
       gets a ".<frame>" suffix (mirrors the recorder's DUMP_FRAMES) */
    const char *const dump_frames_text = getenv("CONTRA_NATIVE_PLAY_DUMP_FRAMES");
    unsigned dump_frames[64];
    unsigned dump_frames_count = 0u;
    unsigned resume_frame = 0u;
    uint8_t prev_fc = 0u;
    bool prev_fc_valid = false;
    const bool rng_free = (rng_mode_text != NULL) && (strcmp(rng_mode_text, "free") == 0);
    const bool use_latched = (input_mode_text != NULL) && (strcmp(input_mode_text, "latched") == 0);
    const long input_offset = (input_offset_text != NULL) ? strtol(input_offset_text, NULL, 10) : 0;
    const unsigned dump_frame = (dump_frame_text != NULL) ? (unsigned)strtoul(dump_frame_text, NULL, 10) : 0u;
    uint8_t native_fc[ALIGNMENT_WINDOW];
    unsigned native_fc_count = 0u;
    PlayInputRow *rows;
    bool digest_v2 = false;
    unsigned last_frame;
    unsigned max_frame;
    unsigned frame;

    if (argc < 2)
    {
        fprintf(stderr, "usage: %s RECORDING.jsonl > native_play_trace.jsonl\n", argv[0]);
        return 2;
    }

    rows = load_recording(argv[1], &last_frame, &digest_v2);
    if (rows == NULL)
    {
        return 1;
    }

    max_frame = last_frame;
    if (max_frame_text != NULL)
    {
        const unsigned limit = (unsigned)strtoul(max_frame_text, NULL, 10);

        if ((limit != 0u) && (limit < max_frame))
        {
            max_frame = limit;
        }
    }

    if (dump_frames_text != NULL)
    {
        const char *p = dump_frames_text;

        while ((*p != '\0') && (dump_frames_count < 64u))
        {
            char *end;
            const unsigned long v = strtoul(p, &end, 10);

            if (end == p)
            {
                break;
            }
            dump_frames[dump_frames_count++] = (unsigned)v;
            p = (*end == ',') ? (end + 1) : end;
        }
    }

    contra_core_init(&core);

    if (resume_path != NULL)
    {
        FILE *const snap = fopen(resume_path, "rb");
        SnapshotHeader header;

        if ((snap == NULL) ||
            (fread(&header, sizeof(header), 1u, snap) != 1u) ||
            (header.magic != SNAPSHOT_MAGIC) || (header.version != 1u) ||
            (header.core_size != (uint32_t)sizeof(core)) ||
            (fread(&core, sizeof(core), 1u, snap) != 1u))
        {
            fprintf(stderr, "FAIL: cannot resume from %s (missing/version/size mismatch -- "
                            "snapshots are invalidated by ContraCore layout changes)\n",
                    resume_path);
            if (snap != NULL)
            {
                fclose(snap);
            }
            free(rows);
            return 1;
        }
        fclose(snap);
        resume_frame = header.frame;
        /* skip the power-on alignment report -- it needs the first frames */
        native_fc_count = ALIGNMENT_WINDOW;
        fprintf(stderr, "resumed from snapshot at frame %u\n", resume_frame);
    }

    for (frame = resume_frame + 1u; frame <= max_frame; ++frame)
    {
        const long source = (long)frame + input_offset;
        const uint8_t *const ram = core.ram;
        ContraInputSnapshot input = {{0u, 0u}};
        char enemies[1024] = {0};
        size_t enemies_len = 0u;
        char pbullets[512] = {0};
        size_t pbullets_len = 0u;
        uint8_t p1_fed;
        uint8_t p2_fed;
        unsigned slot;

        /* LAG SCHEDULE: a recording row whose FRAME_COUNTER equals the previous
           row's marks a real-NES lag frame (the ROM's main loop overran the
           video frame -- e.g. the six-roller room): the game logic never ran
           and the controller was never polled. Skip stepping the core for that
           video frame -- unless the native side is in one of its own modeled
           stalls (level loads / transitions), which freeze the counter on both
           sides in lockstep already. The recorder's snapshot of a lag frame is
           a torn mid-iteration state, so the comparator skips those rows. */
        const bool lag_frame =
            (source >= 2) && (source <= (long)last_frame) &&
            rows[source].has_row && rows[source].has_frame_counter &&
            rows[source - 1].has_row && rows[source - 1].has_frame_counter &&
            (rows[source].frame_counter == rows[source - 1].frame_counter) &&
            (core.startup_wait_frames == 0u) &&
            (core.level_graphics_wait_frames == 0u) &&
            (core.frame_stall_frames == 0u);

        if (!lag_frame && (source >= 1) && (source <= (long)last_frame) && rows[source].has_row)
        {
            const PlayInputRow *const row = &rows[source];

            if (use_latched || !row->has_raw)
            {
                input.player[0] = row->p1_latched;
                input.player[1] = row->p2_latched;
            }
            else
            {
                input.player[0] = row->p1_raw;
                input.player[1] = row->p2_raw;
            }
            /* The ROM advances RANDOM_NUM ($34) in a cycle-count-dependent busy
               loop between frames, which a high-level port cannot reproduce.
               The recorder samples it at the NMI (the exact value this frame's
               logic consumes); injecting it removes RNG-rooted false
               divergence. The core's exe_game_routine applies its own
               approximate busy-loop advance (+= FRAME_COUNTER) at the start of
               the step, so inject the recorded value MINUS the pre-increment
               frame counter and let that advance restore it exactly.
               CONTRA_NATIVE_PLAY_RNG=free disables. */
            if (row->has_rng && !rng_free)
            {
                core.ram[CONTRA_RAM_RANDOM_NUM] =
                    (uint8_t)(row->rng - core.ram[CONTRA_RAM_FRAME_COUNTER]);
            }
        }
        p1_fed = input.player[0];
        p2_fed = input.player[1];

        if (!lag_frame)
        {
            contra_core_set_input(&core, &input);
            contra_core_step_frame(&core);
        }

        if (native_fc_count < ALIGNMENT_WINDOW)
        {
            native_fc[native_fc_count] = ram[CONTRA_RAM_FRAME_COUNTER];
            ++native_fc_count;
            if (native_fc_count == ALIGNMENT_WINDOW)
            {
                report_alignment(rows, last_frame, input_offset, native_fc, native_fc_count);
            }
        }

        /* Compact per-frame enemy digest: one "slot:type:routine:x:y:hp:sw|" group
           per active enemy slot (type != 0). Must match level1_frame_trace.c and
           mesen_play_recorder.lua byte-for-byte. */
        /* 16 slots: the ROM's enemy arrays are 16 wide; 16-23 alias other
           arrays. v2 recordings use 16; legacy ones compared 24 aliased. */
        for (slot = 0u; slot < (digest_v2 ? 16u : 24u); ++slot)
        {
            if ((ram[CONTRA_RAM_ENEMY_TYPE + slot] != 0u) ||
                (digest_v2 && (ram[CONTRA_RAM_ENEMY_ROUTINE + slot] != 0u)))
            {
                enemies_len += (size_t)snprintf(
                    enemies + enemies_len, sizeof(enemies) - enemies_len,
                    "%u:%02X:%02X:%u:%u:%02X:%02X|",
                    slot,
                    (unsigned)ram[CONTRA_RAM_ENEMY_TYPE + slot],
                    (unsigned)ram[CONTRA_RAM_ENEMY_ROUTINE + slot],
                    (unsigned)ram[CONTRA_RAM_ENEMY_X_POS + slot],
                    (unsigned)ram[CONTRA_RAM_ENEMY_Y_POS + slot],
                    (unsigned)ram[CONTRA_RAM_ENEMY_HP + slot],
                    (unsigned)ram[CONTRA_RAM_ENEMY_STATE_WIDTH + slot]);
            }
        }

        /* Player-bullet digest: "slot:x:y:aim:xvf:yvf:routine|" per active bullet. */
        for (slot = 0u; slot < 16u; ++slot)
        {
            if (ram[CONTRA_RAM_PLAYER_BULLET_SPRITE_CODE + slot] != 0u)
            {
                pbullets_len += (size_t)snprintf(
                    pbullets + pbullets_len, sizeof(pbullets) - pbullets_len,
                    "%u:%u:%u:%X:%02X:%02X:%X|",
                    slot,
                    (unsigned)ram[CONTRA_RAM_PLAYER_BULLET_X_POS + slot],
                    (unsigned)ram[CONTRA_RAM_PLAYER_BULLET_Y_POS + slot],
                    (unsigned)ram[CONTRA_RAM_PLAYER_BULLET_AIM_DIR + slot],
                    (unsigned)ram[CONTRA_RAM_PLAYER_BULLET_X_VEL_FAST + slot],
                    (unsigned)ram[CONTRA_RAM_PLAYER_BULLET_Y_VEL_FAST + slot],
                    (unsigned)ram[CONTRA_RAM_PLAYER_BULLET_ROUTINE + slot]);
            }
        }

        {
            const bool want_plain = (dump_frame != 0u) && (frame == dump_frame);
            bool want_suffixed = false;
            unsigned di;

            for (di = 0u; di < dump_frames_count; ++di)
            {
                if (dump_frames[di] == frame)
                {
                    want_suffixed = true;
                    break;
                }
            }
            if (want_plain || want_suffixed)
            {
                uint8_t ppu_dump[NATIVE_PPU_DUMP_SIZE];
                const struct
                {
                    const char *path;
                    const void *data;
                    size_t size;
                } dumps[] = {
                    {ram_dump_path, core.ram, sizeof(core.ram)},
                    {oam_dump_path, core.latched_oam, sizeof(core.latched_oam)},
                    {nametable_dump_path, core.ppu_nametable, sizeof(core.ppu_nametable)},
                    {palette_dump_path, core.ppu_palette, sizeof(core.ppu_palette)},
                    {framebuffer_dump_path, core.framebuffer, sizeof(core.framebuffer)},
                    {chr_dump_path, core.ppu_pattern, sizeof(core.ppu_pattern)},
                    {ppu_dump_path, ppu_dump, sizeof(ppu_dump)},
                    {supertile_dump_path, core.level_screen_supertiles, sizeof(core.level_screen_supertiles)},
                };
                size_t dump_index;

                if (ppu_dump_path != NULL)
                {
                    fill_ppu_dump(ppu_dump, &core);
                }

                for (dump_index = 0u; dump_index < (sizeof(dumps) / sizeof(dumps[0])); ++dump_index)
                {
                    if (dumps[dump_index].path != NULL)
                    {
                        char path_buf[1024];
                        FILE *dump;

                        if (want_suffixed)
                        {
                            snprintf(path_buf, sizeof(path_buf), "%s.%u",
                                     dumps[dump_index].path, frame);
                        }
                        else
                        {
                            snprintf(path_buf, sizeof(path_buf), "%s", dumps[dump_index].path);
                        }
                        dump = fopen(path_buf, "wb");
                        if (dump != NULL)
                        {
                            fwrite(dumps[dump_index].data, 1u, dumps[dump_index].size, dump);
                            fclose(dump);
                        }
                    }
                }
            }
        }

        printf(
            "{\"frame\":%u,\"p1_raw\":%u,\"p2_raw\":%u,\"rng\":%u,"
            "\"weapon\":%u,\"p2_weapon\":%u,"
            "\"game_routine\":%u,\"level_routine\":%u,\"level\":%u,"
            "\"frame_counter\":%u,\"demo_mode\":%u,\"game_init\":%u,"
            "\"delay_low\":%u,\"delay_high\":%u,"
            "\"location_type\":%u,\"screen\":%u,\"scroll_offset\":%u,"
            "\"horizontal_scroll\":%u,\"vertical_scroll\":%u,"
            "\"frame_scroll\":%u,\"player_frame_scroll\":%u,\"p2_frame_scroll\":%u,"
            "\"player_x_velocity\":%u,\"p2_x_velocity\":%u,"
            "\"auto_scroll_00\":%u,\"auto_scroll_01\":%u,\"tank_auto_scroll\":%u,"
            "\"ppu_tile_offset\":%u,\"ppu_addr_low\":%u,\"ppu_addr_high\":%u,"
            "\"attr_addr_high\":%u,\"supertile_nt_offset\":%u,"
            "\"player_state\":%u,\"p2_state\":%u,\"player_x\":%u,\"p2_x\":%u,\"player_y\":%u,\"p2_y\":%u,"
            "\"controller\":%u,\"p2_controller\":%u,\"controller_diff\":%u,\"p2_controller_diff\":%u,"
            "\"jump\":%u,\"p2_jump\":%u,\"edge_fall\":%u,\"p2_edge_fall\":%u,"
            "\"y_fast\":%u,\"p2_y_fast\":%u,\"y_fract\":%u,\"p2_y_fract\":%u,"
            "\"fall_freeze\":%u,\"p2_fall_freeze\":%u,"
            "\"seq\":%u,\"p2_seq\":%u,\"sprite\":%u,\"p2_sprite\":%u,"
            "\"new_life\":%u,\"p2_new_life\":%u,\"inv\":%u,\"p2_inv\":%u,"
            "\"lives\":%u,\"game_over\":%u,\"p2_game_over\":%u,\"demo_end\":%u,"
            "\"oam_offset\":%u,\"enemies\":\"%s\",\"pbul\":\"%s\","
            "\"ram_hash\":\"%08X\",\"pattern_hash\":\"%08X\",\"nametable_hash\":\"%08X\","
            "\"palette_hash\":\"%08X\",\"framebuffer_hash\":\"%08X\"",
            frame,
            (unsigned)p1_fed,
            (unsigned)p2_fed,
            (unsigned)ram[CONTRA_RAM_RANDOM_NUM],
            (unsigned)ram[CONTRA_RAM_P1_CURRENT_WEAPON],
            (unsigned)ram[CONTRA_RAM_P2_CURRENT_WEAPON],
            (unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INDEX],
            (unsigned)ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX],
            (unsigned)ram[CONTRA_RAM_CURRENT_LEVEL],
            (unsigned)ram[CONTRA_RAM_FRAME_COUNTER],
            (unsigned)ram[CONTRA_RAM_DEMO_MODE],
            (unsigned)ram[CONTRA_RAM_GAME_ROUTINE_INIT_FLAG],
            (unsigned)ram[CONTRA_RAM_DELAY_TIME_LOW_BYTE],
            (unsigned)ram[CONTRA_RAM_DELAY_TIME_HIGH_BYTE],
            (unsigned)ram[CONTRA_RAM_LEVEL_LOCATION_TYPE],
            (unsigned)ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER],
            (unsigned)ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET],
            (unsigned)ram[CONTRA_RAM_HORIZONTAL_SCROLL],
            (unsigned)ram[CONTRA_RAM_VERTICAL_SCROLL],
            (unsigned)ram[CONTRA_RAM_FRAME_SCROLL],
            (unsigned)ram[CONTRA_RAM_PLAYER_FRAME_SCROLL],
            (unsigned)ram[CONTRA_RAM_PLAYER_FRAME_SCROLL + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_X_VELOCITY],
            (unsigned)ram[CONTRA_RAM_PLAYER_X_VELOCITY + 1u],
            (unsigned)ram[CONTRA_RAM_AUTO_SCROLL_TIMER_00],
            (unsigned)ram[CONTRA_RAM_AUTO_SCROLL_TIMER_01],
            (unsigned)ram[CONTRA_RAM_TANK_AUTO_SCROLL],
            (unsigned)ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET],
            (unsigned)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE],
            (unsigned)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE],
            (unsigned)ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE],
            (unsigned)ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET],
            (unsigned)ram[CONTRA_RAM_PLAYER_STATE],
            (unsigned)ram[CONTRA_RAM_PLAYER_STATE + 1u],
            (unsigned)ram[CONTRA_RAM_SPRITE_X_POS],
            (unsigned)ram[CONTRA_RAM_SPRITE_X_POS + 1u],
            (unsigned)ram[CONTRA_RAM_SPRITE_Y_POS],
            (unsigned)ram[CONTRA_RAM_SPRITE_Y_POS + 1u],
            (unsigned)ram[CONTRA_RAM_CONTROLLER_STATE],
            (unsigned)ram[CONTRA_RAM_CONTROLLER_STATE + 1u],
            (unsigned)ram[CONTRA_RAM_CONTROLLER_STATE_DIFF],
            (unsigned)ram[CONTRA_RAM_CONTROLLER_STATE_DIFF + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_JUMP_STATUS],
            (unsigned)ram[CONTRA_RAM_PLAYER_JUMP_STATUS + 1u],
            (unsigned)ram[CONTRA_RAM_EDGE_FALL_CODE],
            (unsigned)ram[CONTRA_RAM_EDGE_FALL_CODE + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY],
            (unsigned)ram[CONTRA_RAM_PLAYER_Y_FAST_VELOCITY + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY],
            (unsigned)ram[CONTRA_RAM_PLAYER_Y_FRACT_VELOCITY + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE],
            (unsigned)ram[CONTRA_RAM_PLAYER_FALL_X_FREEZE + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE],
            (unsigned)ram[CONTRA_RAM_PLAYER_SPRITE_SEQUENCE + 1u],
            (unsigned)ram[CONTRA_RAM_PLAYER_SPRITE_CODE],
            (unsigned)ram[CONTRA_RAM_PLAYER_SPRITE_CODE + 1u],
            (unsigned)ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER],
            (unsigned)ram[CONTRA_RAM_NEW_LIFE_INVINCIBILITY_TIMER + 1u],
            (unsigned)ram[CONTRA_RAM_INVINCIBILITY_TIMER],
            (unsigned)ram[CONTRA_RAM_INVINCIBILITY_TIMER + 1u],
            (unsigned)ram[CONTRA_RAM_P1_NUM_LIVES],
            (unsigned)ram[CONTRA_RAM_P1_GAME_OVER_STATUS],
            (unsigned)ram[CONTRA_RAM_P2_GAME_OVER_STATUS],
            (unsigned)ram[CONTRA_RAM_DEMO_LEVEL_END_FLAG],
            (unsigned)ram[CONTRA_RAM_OAMDMA_CPU_BUFFER_OFFSET],
            enemies,
            pbullets,
            fnv1a_bytes(core.ram, sizeof(core.ram)),
            fnv1a_bytes(core.ppu_pattern, sizeof(core.ppu_pattern)),
            fnv1a_bytes(core.ppu_nametable, sizeof(core.ppu_nametable)),
            fnv1a_bytes(core.ppu_palette, sizeof(core.ppu_palette)),
            fnv1a_bytes(core.framebuffer, sizeof(core.framebuffer))
        );

        /* schema v5 tail: mirror of the recorder's v5 additions. bgcol is
           empty -- the native core computes collision from level data and has
           no BG_COLLISION_DATA mirror (yet); the comparator excludes it. */
        {
            const uint8_t fc_now = ram[CONTRA_RAM_FRAME_COUNTER];
            unsigned i;

            printf(",\"score\":%u,\"p2_score\":%u,\"wstr\":%u,\"atkflag\":%u,"
                   "\"gen\":\"%02X:%02X:%u:%u:%02X:%02X\",\"zp\":\"",
                   (unsigned)ram[0x7E2u] + ((unsigned)ram[0x7E3u] << 8u),
                   (unsigned)ram[0x7E4u] + ((unsigned)ram[0x7E5u] << 8u),
                   (unsigned)ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH],
                   (unsigned)ram[CONTRA_RAM_ENEMY_ATTACK_FLAG],
                   (unsigned)ram[CONTRA_RAM_SOLDIER_GENERATION_TIMER],
                   (unsigned)ram[0x79u],
                   (unsigned)ram[0x7Bu],
                   (unsigned)ram[0x7Cu],
                   (unsigned)ram[CONTRA_RAM_SOLDIER_GEN_SCREEN],
                   (unsigned)ram[CONTRA_RAM_SCREEN_GEN_SOLDIERS]);
            for (i = 0x06u; i <= 0x0Fu; ++i)
            {
                printf("%02X", (unsigned)ram[i]);
            }
            printf("\",\"lag\":%u,\"bgcol\":\"\",\"rampg\":\"",
                   (prev_fc_valid && (fc_now == prev_fc)) ? 1u : 0u);
            for (i = 0u; i < 16u; ++i)
            {
                printf("%s%08x", (i != 0u) ? ":" : "",
                       fnv1a_bytes(core.ram + (i * 0x80u), 0x80u));
            }
            printf("\",\"oam\":\"");
            for (i = 0u; i < 0x100u; ++i)
            {
                printf("%02X", (unsigned)core.latched_oam[i]);
            }
            printf("\"}\n");
            prev_fc = fc_now;
            prev_fc_valid = true;
        }

        if (score_trace != NULL)
        {
            fprintf(score_trace, "%u %u\n", frame,
                    (unsigned)ram[0x7E2u] + ((unsigned)ram[0x7E3u] << 8u));
        }

        if ((snapshot_every != 0u) && (snapshot_dir != NULL) &&
            ((frame % snapshot_every) == 0u))
        {
            char snap_path[1024];
            FILE *snap;

            snprintf(snap_path, sizeof(snap_path), "%s/core_%u.bin", snapshot_dir, frame);
            snap = fopen(snap_path, "wb");
            if (snap != NULL)
            {
                const SnapshotHeader header = {SNAPSHOT_MAGIC, 1u, frame, (uint32_t)sizeof(core)};

                fwrite(&header, sizeof(header), 1u, snap);
                fwrite(&core, sizeof(core), 1u, snap);
                fclose(snap);
            }
        }
    }

    if (score_trace != NULL)
    {
        fclose(score_trace);
    }

    if ((native_fc_count > 0u) && (native_fc_count < ALIGNMENT_WINDOW))
    {
        report_alignment(rows, last_frame, input_offset, native_fc, native_fc_count);
    }

    free(rows);
    return 0;
}
