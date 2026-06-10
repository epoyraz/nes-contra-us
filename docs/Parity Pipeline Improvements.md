# Parity Pipeline Improvements (backlog — not yet implemented)

Ideas collected while working the stage-3 frontier (June 2026). Each item exists
because a concrete divergence cost real time that the improvement would have
avoided. Nothing here is implemented yet; this is the pick-up-later plan.

The current pipeline these improve on: record human play in Mesen
(`docs/lua_scripts` recorder → `tmp/reference_mesen_v4.jsonl`), replay through
the native core (`CONTRA_NATIVE_PLAY_INPUT=latched
./build/port/contra_play_replay_trace tmp/reference_mesen_v4.jsonl >
tmp/reference_native_latest.jsonl`), diff for the first structural divergence
(`tools/compare_play_trace.py`, plus the per-stage structural script currently
in `tmp/structural_check.py`).

## 1. Next recording: capture more state

Motivating failure: the frame-20968 soldier-generator desync. The generator's
timer/routine are not recorded, so a phase slip was invisible until a spawn
landed 4 frames off — locating the cause required reverse-engineering the
generator's phase arithmetic instead of reading a diff.

- **Engine-globals block (~64 bytes/frame).** At minimum:
  `SOLDIER_GENERATION_TIMER/ROUTINE/X_POS/Y_POS`, `SOLDIER_GEN_SCREEN`,
  `SCREEN_GEN_SOLDIERS`, `PLAYER_WEAPON_STRENGTH`, `ENEMY_ATTACK_FLAG`,
  `LEVEL_SCREEN_NUMBER`, `LEVEL_SCREEN_SCROLL_OFFSET` (both currently `None`
  in the v4 trace), auto-scroll timers, `LEVEL_STOP_SCROLL`,
  `BOSS_AUTO_SCROLL_COMPLETE`, and the zero-page temps `$06-$0F` (the ROM
  leaks them between systems; the port already reproduces the `$06` junk
  chain for generated-soldier attributes).
- **`BG_COLLISION_DATA` raw (128 bytes/frame, $0680-$06FF).** The whole
  collision bug class (the raw-Y>=0xE0 empty guard, quadrant-granularity tile
  sampling, vertical screen-seam mapping) would each have been a one-line
  byte diff if the recording carried the ROM's actual collision array. The
  port can build its own equivalent and compare byte-for-byte.
- **Richer per-slot enemy digest.** Add `ENEMY_VAR_1..4`, velocities
  (fract/fast), `ENEMY_ANIMATION_DELAY`, `ENEMY_ATTACK_DELAY`, `ENEMY_HP`.
  Today divergences surface when *position* drifts, often many frames after
  the causal VAR/velocity/delay diverged.
- **Player bullet full state.** Velocity fract bytes, `PLAYER_BULLET_TIMER`,
  `PLAYER_BULLET_FS_X`/`F_Y` (the F-weapon swirl center).
- **Per-page RAM hashes** (16 hashes instead of the single excluded
  `ram_hash`). When every digest field matches but state has silently
  desynced, the first diverging page localizes the search immediately.
- **Explicit lag-frame marker from the emulator** instead of inferring lag
  from a frozen `FRAME_COUNTER`. Removes the torn-row ambiguity that needed
  the "skip rows whose frame_counter equals either neighbor's" comparator
  heuristic and the replay-side lag schedule.

## 2. Mesen-side debugging hooks

The asymmetry is the core problem: native RAM can be dumped at any frame
(`CONTRA_NATIVE_PLAY_DUMP_FRAME`), Mesen's side is frozen at recording time.

- **Movie + on-demand RAM dump script:** a Lua script that deterministically
  re-runs the recorded movie and dumps full RAM (and PPU nametables) at a
  requested list of frames. Every divergence then becomes a direct two-sided
  RAM diff via `tools/ramdiff.py`.
- **Periodic savestates** (every ~1000 frames) during recording so a targeted
  re-run to frame N is seconds, not a full-movie replay.

## 3. Faster iteration loop

- **Core snapshots (biggest win).** `ContraCore` is one flat struct —
  serialize it every ~1000 frames during a green run. After a code change,
  replay from the last green snapshot before the frontier instead of from
  frame 0: the 30-60s cycle becomes ~2s at a 20k-frame frontier. Full replay
  still runs before every commit (a change can regress earlier frames).
  Caveat: `contra_rom_image` statics and any non-struct state must be
  re-initialized on load; verify a snapshot round-trips to identical traces
  before trusting it.
- **`make frontier` target** (or `tools/frontier.sh`): build + replay +
  structural compare in one command. Promote `tmp/structural_check.py` into
  `tools/` (it is the per-stage structural report with lag-burst skip;
  `tmp/` is not durable). Have it auto-print the first divergent enemy/bullet
  slot's field diff — the manual inspect-python adds a minute per cycle,
  dozens of times a day. Guard against the two known foot-guns in one place:
  the replay tool writes to STDOUT (argv[2] is ignored — redirect, never
  pass an output path), and the input must be the mesen_v4 recording (older
  recordings lack the `weapon` field and break digest_v2).
- **Permanent env-gated trace registry.** `CONTRA_TRACE=soldier_gen,collision`
  style switches compiled in permanently instead of ad-hoc probe edits that
  get added, rebuilt, and reverted around every investigation (and risk
  leaking into commits — today's session has exactly such uncommitted probe
  code in `contra_rom_gen_soldier_find_pos` / `soldier_generation_02`).
- **ctags/index over `src/*.asm`** so routine-name → file:line is instant.

## 4. Strategy-level changes

- **Mirror the ROM's data pipelines, not its queries.** Four collision bugs
  so far all came from reconstructing collision per-probe from pattern
  indexes instead of building `BG_COLLISION_DATA` once during supertile
  streaming the way bank7's `set_tile_collision` does (one classified tile
  per 16x16 quadrant, two rows per supertile, written during
  `load_level_supertile_data`). Building the table makes every consumer
  (player ground checks, enemy probes, soldier-generation searches,
  `get_bg_collision_far`) faithful for free, including the nametable-half
  selection and wrap quirks. Apply the same principle to any small RAM table
  the ROM precomputes.
- **Per-level recordings + init-from-snapshot.** One 40k-frame recording
  serializes the work: stage 4 cannot be attacked until stage 3 is green
  because everything cascades. Short per-level recordings, with a core mode
  that initializes from a recorded RAM snapshot at the level boundary, would
  let levels be greened independently — including in parallel.
- **CI baseline guard.** `tools/compare_play_trace.py --baseline N` already
  exists; wire it (or the structural variant) into CI on every push so a
  regression in an already-green stage is caught at commit time rather than
  at the next manual full verify.

## Known state when this was written

- Stages 1-2 structurally green; stage 3 frontier at frame 20968 (48.9%).
- In-flight diagnosis: the port's outdoor collision lookup reads level-3
  screen 5/6's left column (x=0x0A) as all-empty at VERTICAL_SCROLL=0xC9
  while the ROM finds floor there — this flips two soldier-generation search
  outcomes and shifts the spawn phase by 4 frames. The quadrant-snap fix
  (sample the 16x16 quadrant's top-left tile, matching `set_tile_collision`)
  is already applied and kept stages 1-2 green but did not resolve the
  left-column reads; the screen-seam/nametable mapping of the vertical
  lookup is the remaining suspect. Probe instrumentation (env
  `CONTRA_PROBE_SOLDIER_GEN`) is in the working tree, uncommitted.
