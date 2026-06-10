# Play Recording Parity

Frame-by-frame comparison of the native port against the real ROM on **arbitrary
human play** — not just the attract demo. Play the game in Mesen until a bug
shows; the pipeline replays your exact inputs through the native core and names
the first frame and field where the port diverges. That turns "the sprite looks
wrong around the bridge" into "`player_x` splits at frame 2471", which is a
fixable bug instead of an archaeology project.

This extends the demo-parity tooling (`tools/mesen_level1_frame_trace.lua`,
`tools/mesen_l2_demo_trace.lua`, `tools/compare_demo_frame_trace.py`) to
recorded input. All three tools share the same JSONL schema and comparison
policy: semantic gameplay fields are compared; region hashes and `oam_offset`
are excluded because the port is a high-level reimplementation, not a
byte/cycle-exact emulator.

## Pipeline

### 1. Record — `tools/mesen_play_recorder.lua` (Mesen 2)

Launch Mesen **from a shell** (so it inherits the env vars) with the ROM and
the script on the command line — the script auto-loads and records from
power-on, so frame 1 of the recording equals `contra_core_init` + first
`contra_core_step_frame` on the native side. It records one JSONL row per
frame: the raw controller input (`p1_raw`/`p2_raw`, from `emu.getInput`), the
latched state the game consumed (`controller` = `$F1`), and the full gameplay
field set. Then just play until you've reproduced the bug, and stop the script.

To restart a recording (or start one mid-session), use **System > Power
Cycle** — the script auto-restarts at frame 0 and begins a fresh file. The
script refuses to record from a mid-session state: Mesen's Lua API only offers
a soft reset, and Contra warm-boots through soft reset (RAM survives, the boot
skips the cold-init timeline), which can never be replayed against the
cold-booted native core.

Also start a Mesen **movie recording** (Tools > Movies > Record) before
playing. The movie makes the session repeatable: you can re-trace it later with
different probes without reproducing the bug by hand.

| Env var | Meaning |
|---|---|
| `CONTRA_MESEN_PLAY_RECORDING_JSONL` | output path (default `contra_play_recording.jsonl`, in Mesen's CWD) |
| `CONTRA_MESEN_PLAY_MAX_FRAME` | auto-stop after N frames (default 0 = run until script unload) |
| `CONTRA_MESEN_PLAY_NO_RESET` | `1` skips the power cycle — required when re-tracing a movie playback |
| `CONTRA_MESEN_PLAY_DUMP_FRAME` + `CONTRA_MESEN_PLAY_{RAM,OAM,NAMETABLE,PALETTE,FRAMEBUFFER}_DUMP_PATH` | dump full state at one frame (second pass) |

### 2. Replay — `contra_play_replay_trace`

```sh
cmake --build build --target contra_play_replay_trace
./build/port/contra_play_replay_trace contra_play_recording.jsonl > native_play_trace.jsonl
```

Feeds the recorded raw input through `contra_core_set_input` frame-by-frame and
emits the identical schema. It verifies alignment using the game's own NMI
counter (`$1A`): if the recording is shifted relative to power-on, stderr names
the exact `CONTRA_NATIVE_PLAY_INPUT_OFFSET` that repairs it. **Fix any
alignment warning before trusting the comparison.**

| Env var | Meaning |
|---|---|
| `CONTRA_NATIVE_PLAY_INPUT` | `raw` (default) or `latched` — feed `$F1/$F2` instead; use if a Mesen DPCM controller-read glitch corrupted a raw frame |
| `CONTRA_NATIVE_PLAY_INPUT_OFFSET` | shift the recording N frames (may be negative) |
| `CONTRA_NATIVE_PLAY_MAX_FRAME` | stop early |
| `CONTRA_NATIVE_PLAY_DUMP_FRAME` + `CONTRA_NATIVE_PLAY_{RAM,OAM,NAMETABLE,PALETTE,FRAMEBUFFER,CHR,SUPERTILE}_DUMP_PATH` | dump full native state at one frame |

### 3. Compare — `tools/compare_play_trace.py`

```sh
python3 tools/compare_play_trace.py native_play_trace.jsonl contra_play_recording.jsonl
```

Reports how far the traces stay gameplay-identical and details the **first**
divergence (everything after it cascades). It distinguishes three classes:

- **Input divergence** — the controller fields split before any gameplay does:
  the replay is misaligned or a DPCM glitch hit; fix per the hints printed.
- **Frame shift** — from frame f the native trace matches Mesen offset by ±d:
  one timing slip (one bug), not hundreds of divergent frames.
- **Logic divergence** — a real port bug; the field list and per-field summary
  localize it.

`--frame N` prints the full field diff at frame N; `--baseline N` makes it a CI
guard.

### 4. Deep dive at the divergent frame

Once frame N is known, re-run the native replay with
`CONTRA_NATIVE_PLAY_DUMP_FRAME=N` and the dump paths. On the Mesen side,
re-trace the session **headlessly** — Mesen is deterministic, so feeding the
recording's raw inputs back reproduces it bit-exactly (verified on a 39,970
frame session), including DPCM glitches:

```sh
CONTRA_MESEN_PLAY_RECORDING_JSONL=out.jsonl \
CONTRA_MESEN_PLAY_REPLAY_JSONL=recording.jsonl \
CONTRA_MESEN_PLAY_DUMP_FRAME=N CONTRA_MESEN_PLAY_RAM_DUMP_PATH=mesen.ram \
Mesen --testRunner --doNotSaveSettings --timeout=900 baserom.nes tools/mesen_play_recorder.lua
```

This also upgrades old recordings to the current schema (new fields are
re-extracted from the replay). Then `tools/ramdiff.py` the two RAM dumps —
`src/ram.asm` maps each diverging address back to a named variable.

Mesen 2.1.1 Lua quirks (field-tested): `emu.setInput` is `(table, port)` — the
documented `(port, table)` order throws; and `setInput(table, 1)` does NOT
drive player 2, it clobbers port 0 (the replay mode skips P2 frames of 0 for
this reason). `Debug/ScriptWindow/AllowIoOsAccess` must be true and
`ScriptTimeout` ~60s in settings.json.

## Caveats

- Record from power-on (the script handles this). Mid-session recordings can't
  be replayed: the native core can't be seeded with mid-game state.
- OAM, nametable, palette and scroll fields are the visual ground truth.
  Mid-frame raster effects (the status-bar scroll split) can't diverge in this
  comparison even when rendering differs — those still need eyeballing.
- During attract demo segments the game generates its own input; recorded
  input is ignored there by both sides, so demos stay comparable.

## Validation

The loop is self-checked without Mesen: replaying a native trace's own echoed
inputs reproduces it byte-for-byte (determinism + input round-trip), and the
comparator was exercised against synthetic logic-divergence, frame-shift and
misaligned-recording cases.
