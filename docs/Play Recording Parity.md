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
| `CONTRA_MESEN_PLAY_DUMP_FRAME` + `CONTRA_MESEN_PLAY_{RAM,OAM,NAMETABLE,PALETTE,FRAMEBUFFER,CHR,PPU,SUPERTILE}_DUMP_PATH` | dump full render/state telemetry at one frame (second pass) |

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
| `CONTRA_NATIVE_PLAY_INPUT_OFFSET` | shift the recording N frames (may be negative); replay length is adjusted so every aligned recording frame is emitted |
| `CONTRA_NATIVE_PLAY_MAX_FRAME` | stop early |
| `CONTRA_NATIVE_PLAY_DUMP_FRAME` + `CONTRA_NATIVE_PLAY_{RAM,OAM,NAMETABLE,PALETTE,FRAMEBUFFER,CHR,PPU,SUPERTILE}_DUMP_PATH` | dump full native render/state telemetry at one frame |

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
guard. If the recorder and native cold-start timelines have a known fixed
offset, pass `--native-frame-offset N`; for example, `2` compares recorded
Mesen frame `f` with native frame `f+2`. Visible OAM is compared by displayed
`y/tile/attribute/x` content, ignoring only the rotating physical OAM slot
number. `--start-frame N` can exclude a known cold-start-only frame before the
aligned gameplay timeline.

### 4. Deep dive at the divergent frame

Once frame N is known, re-run the native replay with
`CONTRA_NATIVE_PLAY_DUMP_FRAME=N` and the dump paths. On the Mesen side,
re-trace the session **headlessly** — Mesen is deterministic, so feeding the
recording's raw inputs back reproduces it bit-exactly (verified on a 39,970
frame session), including DPCM glitches:

```sh
CONTRA_MESEN_PLAY_RECORDING_JSONL=out.jsonl \
CONTRA_MESEN_PLAY_REPLAY_JSONL=recording.jsonl \
CONTRA_MESEN_PLAY_REPLAY_INPUT_OFFSET=-2 \
CONTRA_MESEN_PLAY_DUMP_FRAME=N CONTRA_MESEN_PLAY_RAM_DUMP_PATH=mesen.ram \
Mesen --testRunner --doNotSaveSettings --timeout=900 baserom.nes tools/mesen_play_recorder.lua
```

When the headless test runner starts two frames behind the original power-cycle
recording, pair original frame `f` with headless frame `f+2`, and set
`CONTRA_MESEN_PLAY_REPLAY_INPUT_OFFSET=-2` so the input consumed at that frame is
also aligned. Confirm the structural trace remains identical before trusting
any framebuffer dump.

This also upgrades old recordings to the current schema (new fields are
re-extracted from the replay). Then `tools/ramdiff.py` the two RAM dumps —
`src/ram.asm` maps each diverging address back to a named variable.

Mesen 2.1.1 Lua quirks (field-tested): `emu.setInput` is `(table, port)` — the
documented `(port, table)` order throws; and `setInput(table, 1)` does NOT
drive player 2, it clobbers port 0 (the replay mode skips P2 frames of 0 for
this reason). `Debug/ScriptWindow/AllowIoOsAccess` must be true and
`ScriptTimeout` ~60s in settings.json.

## Renderer-level checks

The gameplay comparator intentionally ignores pixels. For pixel-perfect work,
use the renderer check on the frame(s) you care about:

```sh
cmake --build build --target contra_play_replay_trace
tools/render_check.sh tmp/recordings/play.jsonl 12345 12346
```

`tools/render_check.sh` re-traces the Mesen recording headlessly, replays the
same input through the native core, and writes `tmp/render/<frame>.png`
triptychs (`mesen | native | diff`). The diff is palette-independent: it
compares the rendered structure after deriving a best color bijection, so a
different RGB palette does not mask tile/sprite/scroll mistakes.

For each requested frame it also writes matching sidecar dumps:

| Surface | Mesen dump | Native dump | Notes |
|---|---|---|---|
| CPU RAM | `mesen_ram.bin.<frame>` | `native_ram.bin.<frame>` | 2 KiB CPU RAM |
| OAM | `mesen_oam.bin.<frame>` | `native_oam.bin.<frame>` | Sprite RAM / native latched OAM |
| Nametable | `mesen_nametable.bin.<frame>` | `native_nametable.bin.<frame>` | 2 KiB mirrored nametable model |
| Palette | `mesen_palette.bin.<frame>` | `native_palette.bin.<frame>` | 32-byte palette RAM |
| CHR / pattern | `mesen_chr.bin.<frame>` | `native_chr.bin.<frame>` | 8 KiB pattern-table bytes |
| Full PPU view | `mesen_ppu.bin.<frame>` | `native_ppu.bin.<frame>` | 16 KiB PPU address space view |
| Supertile cache | `mesen_supertile.bin.<frame>` | `native_supertile.bin.<frame>` | ROM `$0600-$067F` vs native screen cache |
| Framebuffer | `mesen_fb.bin.<frame>` | `native_fb.bin.<frame>` | 256x240 little-endian RGBA/u32 |

Use the sidecars in this order when a rendered frame differs:

1. Compare `CHR`, `nametable`, and `palette` first. If these differ, the port's
   PPU-facing state is wrong before composition.
2. Compare `OAM` next. If only sprites differ, this usually identifies missing
   OAM build, sprite tile, attribute, or priority behavior.
3. Compare `framebuffer` plus the PNG triptych last. If PPU-facing state and
   OAM agree but pixels differ, the native compositor is wrong.
4. Use `RAM`/`supertile` to explain why the PPU-facing state diverged.

Remaining blind spot: these are end-of-frame dumps. They do not prove
mid-frame raster timing, ordered `$2000-$2007` writes, `$4014` OAM DMA timing,
or the status-bar scroll split. If a frame differs while the end-of-frame PPU
state looks identical, the next tool to add is an ordered PPU/register write
trace with frame/scanline/cycle/PC/value.

## Schema v6

The recorder emits `"schema":6` in the meta row. Schema v6 adds a compared
`voam` field containing only the visible PPU OAM entries that produced the
frame (`index:y:tile:attr:x`). This is deliberately different from the legacy
inspection-only `oam` field: `oam` is the CPU shadow buffer prepared for a
future DMA, while `voam` is lag-corrected displayed sprite state. Hidden OAM
entries are omitted because the ROM only changes their Y byte and leaves the
other bytes stale. The PPU's unused OAM attribute bits 2–4 are masked out.

This makes sprite code, attributes, and final rendered X/Y placement part of
the normal frame-by-frame comparison, including moving-enemy baseline errors.
All older field names are unchanged, so older traces and tools keep working —
the comparators diff the field intersection across schemas.

Schema v5 added these per-frame fields on top of v4:

| Field | Source | Compared? |
|---|---|---|
| `score`, `p2_score` | $07E2/3, $07E4/5 (16-bit) | structural |
| `wstr` | PLAYER_WEAPON_STRENGTH $2F | structural |
| `atkflag` | ENEMY_ATTACK_FLAG $8E | structural |
| `gen` | soldier generator `timer:routine:x:y:genscreen:count` ($7A,$79,$7B,$7C,$195,$196) | structural |
| `lag` | FRAME_COUNTER froze vs previous row | structural |
| `zp` | $06–$0F hex | inspection only (mid-frame ROM scratch) |
| `bgcol` | BG_COLLISION_DATA $0680–$06FF hex | inspection only (native has no mirror yet) |
| `rampg` | 16× FNV-1a over 128-byte RAM pages | inspection only |
| `oam` | OAM shadow $0200–$02FF hex | inspection only (rotating buffer) |
| `voam` | visible displayed PPU OAM as `index:y:tile:attr:x` groups | structural |

`bgcol`/`rampg`/`oam` are "heavy" (~2400 extra reads/frame): always on in
headless re-trace mode, opt-in for live recording via
`CONTRA_MESEN_PLAY_HEAVY=1`. `CONTRA_MESEN_PLAY_DUMP_FRAMES=f1,f2,...` dumps
state at multiple frames in one run (files get a `.<frame>` suffix).

## Fast iteration

- `tools/frontier.sh [RECORDING [OUT]]` — build + replay + structural report
  in one command; it owns the stdout redirection and refuses raw recordings
  (the two historical foot-guns).
- `tools/structural_check.py` — per-stage structural report with an automatic
  per-slot diff of the first divergent `enemies`/`pbul` field;
  `--frame N` prints the full diff at one frame.
- Core snapshots: `CONTRA_NATIVE_PLAY_SNAPSHOT_EVERY=N` +
  `CONTRA_NATIVE_PLAY_SNAPSHOT_DIR=dir` write the flat `ContraCore` during a
  replay; `CONTRA_NATIVE_PLAY_RESUME=dir/core_F.bin` resumes mid-recording so
  a deep frontier iterates in seconds (pair with `CONTRA_NATIVE_PLAY_MAX_FRAME`).
  Snapshots are invalidated by any `ContraCore` layout change (size-checked).
  Always run the FULL replay before trusting/committing a fix.

## Caveats

- Record from power-on (the script handles this). Mid-session recordings can't
  be replayed: the native core can't be seeded with mid-game state.
- **The comparison is frame-by-frame over GAME STATE, not pixels.** The
  framebuffer/nametable/pattern/palette hashes are excluded by design (the
  port renders through its own compositor), so background rendering bugs are
  invisible to this pipeline even at ALL GREEN. The v5 `oam` field narrows the
  gap on the sprite side (inspection); use `tools/render_check.sh` for
  renderer-level checks.
- Mid-frame raster effects (the status-bar scroll split) can't diverge in this
  comparison even when rendering differs — those still need eyeballing.
- During attract demo segments the game generates its own input; recorded
  input is ignored there by both sides, so demos stay comparable.

## Validation

The loop is self-checked without Mesen: replaying a native trace's own echoed
inputs reproduces it byte-for-byte (determinism + input round-trip), and the
comparator was exercised against synthetic logic-divergence, frame-shift and
misaligned-recording cases.
