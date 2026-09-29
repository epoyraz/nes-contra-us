# Parity Telemetry (RAM → PPU → pixels)

Exhaustive, frame-by-frame, programmatic comparison of the native port against
the original ROM running in Mesen 2, on a recorded human play session. Every
frame compares **all 2 KiB of CPU RAM, nametable RAM, palette RAM, OAM, CHR
RAM, the PPU rendering registers and all 61,440 pixels**. It answers "is the
port identical?" in ~10 s for 15,600 frames, and "where exactly does it stop
being identical?" down to the byte, register and dot.

Status (reference recording `tmp/reference_mesen_v4.jsonl`, frames 1-15600:
intro, Stage 1, Stage 2 including the boss fight and the elevator ending, the
Stage 3 intro): **game RAM, nametables, palette, OAM, CHR, registers and every
pixel identical on every frame.**

## Pipeline

```
Mesen 2 (headless) + tools/mesen_telemetry.lua   -- replays the recording's inputs from power-on
        | raw per-frame records over a FIFO
tools/ctel.py sink                               -- zlib-compressed .ctel archive
        |
contra_parity_telemetry RECORDING.jsonl ARCHIVE  -- replays the same inputs through the native core
                                                    and diffs every layer, every frame
```

### 1. Capture the ground truth (once per recording)

```sh
tools/capture_telemetry.sh tmp/reference_recording_v2.jsonl tmp/telemetry/reference_1_15600.ctel 15600
```

(`reference_recording_v2.jsonl` and `reference_mesen_v4.jsonl` are the same
session; v4 is a re-trace with more fields.) `tools/mesen_telemetry.lua` feeds
the recording's raw pad bits at each input
poll and, at every end of frame (scanline 240), writes: CPU RAM, CIRAM, palette
RAM, OAM, CHR RAM (only when written), the PPU registers (ctrl, mask, v, t,
fine x, w), the framebuffer as canonical palette indices, the log of every PPU
memory write, the log of every register write ($2000-$2003, $2005-$2006,
$4014) with scanline and dot, and frame info (NMI-time RANDOM_NUM, fed input,
poll/NMI counts). `tools/ctel.py info|dump ARCHIVE FRAME [--ppu-log]` inspects
an archive.

### 2. Compare

```sh
cmake --build build-rel --target contra_parity_telemetry
./build-rel/port/contra_parity_telemetry tmp/reference_mesen_v4.jsonl tmp/telemetry/reference_1_15600.ctel \
    --report tmp/r.jsonl --addr-histogram
python3 tools/parity_summary.py tmp/r.jsonl --layer game,fb      # divergent frame runs per stage
```

The summary lists, per layer, frames that differ and the first one; RAM is
split into named regions (from `src/ram.asm`) and classes:

| class | contents | parity metric? |
|---|---|---|
| game | everything the game logic owns | yes |
| scratch | $00-$17 subroutine temporaries | no -- internal junk; where it leaks into game state (zero-page junk chains) it shows up as a game diff |
| sound | the bank-1 sound engine's RAM | no -- the port has no sound engine yet |
| stack | $0197-$01FF | no |
| rng | $34 at end of frame | no -- advanced by the cycle-counted idle loop after the logic |

Frames of a lag burst (the ROM's logic overran the video frame, so the
end-of-frame snapshot is taken mid-logic) are "torn" and excluded from the RAM
tallies; their pixels are still compared (`--report` rows carry `torn`).

### 3. Drill down

| option | use |
|---|---|
| `--from N --to N` | window (the core always replays from power-on) |
| `--stop LAYER` | stop at the first frame where `game`/`nt`/`pal`/`oam`/`chr`/`regs`/`fb`/`any` differs |
| `--detail F[,F-F]` | full two-sided dumps + PNG triptych (Mesen, native, diff) + register/PPU write logs |
| `--watch A[-B],...` | print these RAM bytes every frame, `mesen/native` where they differ, plus the core's stall/wait/lag state |
| `--addr-histogram` | every game-RAM address that ever differs, with frame count and first frame |

`tools/watch.sh ADDRS FROM TO [RECORDING] [EXEC_PCS]` (Mesen,
`tools/mesen_watch.lua`) then answers "which ROM routine wrote this byte?":
every CPU write to the watched addresses with frame, scanline, dot, PC, A/X/Y
and the mapped bank.

## What the replay injects

The port is a C reimplementation, not a cycle-counting emulator. Three values
that depend on CPU cycles the port does not count are taken from the
reference, like a recorded input:

- **Input**: each frame's pad bits are the ROM's own validated read,
  CTRL_KNOWN_GOOD ($F9/$FA): DPCM controller-read glitches already resolved and
  free of scripted input (the end-of-level auto-walk writes CONTROLLER_STATE,
  not CTRL_KNOWN_GOOD). `CONTRA_NATIVE_PLAY_INPUT=raw|latched` uses the
  recording's columns instead.
- **RANDOM_NUM at NMI time** (the idle loop spins it between frames).
  `CONTRA_NATIVE_PLAY_RNG=free` disables.
- **Raster cut timing** (`core.raster_cut_hint`): when a level load calls
  clear_ppu while the picture is being drawn, where that lands depends on the
  NMI entry jitter and the sound engine's cycle count. The port decides
  *whether* a cut happens; the hint gives the dot. Without it the port uses each
  load's typical measured timing. `CONTRA_NATIVE_PLAY_RASTER=free` disables.

## Model notes that were needed for identity

- The NMI is modeled as the ROM runs it: `clear_ppu`, OAM DMA, palette and
  CPU_GRAPHICS_BUFFER flush, PPU_READY-gated mask, scroll, the game loop, then
  `draw_sprites` / `write_0_to_cpu_graphics_buffer` / NMI_CHECK. Multi-frame
  loads (NMI disabled) stall the core and run the rest of the blocked NMI when
  the load finishes. A nested NMI (lag frame) only writes mask and scroll.
- A mid-scanline clear_ppu is rendered as the PPU fetched it: its $2006 writes
  reset v three dots after the second write, and a tile's pattern address is
  fixed at its nametable fetch (`contra_ppu_render_clear_ppu_line`).
- Tests and debug warps that poke RAM to start in a level must use
  `contra_core_boot()`: the power-on sequence runs the ROM's reset code, which
  clears RAM on frame 3.

## ROM behaviours that caused divergences (checklist)

- **Zombie tails**: `jmp remove_enemy` returns to the caller, which keeps
  running on the removed slot (decrements delays, writes sprites, applies
  gravity). Do not `return` after a removal unless the ROM does.
- **`set_enemy_routine_to_a` / `advance_enemy_routine`** do not revive a removed
  enemy (routine 0); they clear its sprite instead (`set_sprite_0`).
- **Table reads past the end**: index the ROM image (e.g. `contra_bank2_read`),
  don't clamp or mask.
- **Bit tests from shifts**: `lsr` ×3 leaves bit 2 in carry, not bit 3.
- **RAM layout**: `ram.asm`'s `.export` comments were wrong once
  (LEVEL_END_LVL_ROUTINE_STATE is $0192); the `.res` layout is authoritative.
- **Which routine table**: generated indoor soldiers explode with
  `shared_enemy_routine_03` (explosion_type_02), the boss room has its own
  player sprite table, player state 3 does not redraw the player.
