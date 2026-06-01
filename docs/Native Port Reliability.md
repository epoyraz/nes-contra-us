# Native Port Reliability Plan

This document defines the reliability scaffold for the native Contra port.

The goal is not to rely on prose status claims. The source of truth should be
repeatable tests, coverage output, and a machine-readable routine ledger.

## Reliability Layers

1. Native behavior tests

   The `contra_core_tests` CTest target drives `ContraCore` directly and asserts
   RAM, routine, scroll, player, and framebuffer invariants.

   The `contra_checkpoint_trace_tests` CTest target runs longer deterministic
   input scripts and pins named checkpoints with RAM, nametable, palette, enemy,
   and framebuffer hashes. These hashes are regression baselines for the native
   core today and are shaped to accept original-ROM checkpoints later. The
   `contra_checkpoint_trace_export` CTest target writes the same checkpoints as
   JSONL to `build_win/port/contra_checkpoint_trace.jsonl`, and
   `contra_checkpoint_trace_export_tests` validates that exported schema,
   ordering, frame anchors, scenario names, and hash fields. The
   `contra_checkpoint_trace_compare_self` CTest target keeps the JSONL
   comparator compiled and runnable. When Mesen and `contra.nes` are available,
   `contra_mesen_checkpoint_trace_export` runs the original ROM in Mesen's
   `--testRunner` mode through `tools/mesen_checkpoint_trace.lua`, and
   `contra_mesen_trace_export_tests` validates the original-ROM JSONL schema.
   The original and native JSONL traces are now both machine-readable. The
   Mesen exporter covers level 1 attract/demo checkpoints, the first natural
   level 2 attract/demo checkpoint, and the original-ROM terminal level 2 demo
   state. The terminal row is exported as a drift target but is not included in
   the passing native semantic comparator yet. The
   `contra_checkpoint_trace_compare_attract_semantics` CTest target compares
   the shared native/original attract rows for routine, level, screen, player,
   lives, game-over, demo-end, indoor-clear, and wall-core remaining semantics,
   with explicit tolerances for the remaining frame, scroll, and player-position
   drift. The attract trace now includes the original-ROM level 2 first-room
   wall-core load, wall-core destruction, room-clear flag, screen-1 advance,
   and the later original-ROM demo-finished state.
   Full strict hash comparison remains expected to fail until the native
   timing/state model is brought into parity.

2. C code coverage

   `CONTRA_PORT_ENABLE_COVERAGE=ON` instruments the native C targets with
   GCC/Clang coverage flags. This answers which native implementation code ran.

3. ASM routine coverage ledger

   `port/coverage/asm_routines.csv` records original ASM labels and their port
   status. The `contra_coverage_manifest_tests` target fails if a
   `translated` or `partial` claim lacks both a native symbol and at least one
   registered test reference. It also rejects unknown statuses and missing
   source files, so the ledger cannot drift through stale test-name text or
   dead source paths.

4. Differential testing

   The first differential gate is now active for the shared attract/demo trace.
   A stricter deterministic emulator harness still needs to drive original ROM
   and native level 1/level 2 gameplay frame-by-frame on identical input streams,
   then graduate from semantic fields to RAM, PPU, enemy, and framebuffer hashes.
   Until that exists, confidence is stronger than scenario-only testing but is
   not yet equivalence-proof based.

## Commands

Build and run the normal tests:

```powershell
cmake --build C:\Users\enesp\Desktop\coding\nes-contra-us\build_win --target contra_core_tests contra_checkpoint_trace_tests contra_checkpoint_trace_export_tests contra_checkpoint_trace_compare contra_checkpoint_trace_compare_attract_semantics contra_coverage_manifest_tests contra_mesen_trace_export_tests
ctest --test-dir C:\Users\enesp\Desktop\coding\nes-contra-us\build_win --output-on-failure
```

Create and run a coverage build:

```powershell
cmake -S C:\Users\enesp\Desktop\coding\nes-contra-us -B C:\Users\enesp\Desktop\coding\nes-contra-us\build-coverage -G Ninja -DCONTRA_PORT_BUILD_SDL=OFF -DCONTRA_PORT_ENABLE_COVERAGE=ON
cmake --build C:\Users\enesp\Desktop\coding\nes-contra-us\build-coverage --target contra_core_tests contra_checkpoint_trace_tests contra_checkpoint_trace_export_tests contra_checkpoint_trace_compare contra_checkpoint_trace_compare_attract_semantics contra_coverage_manifest_tests contra_mesen_trace_export_tests
ctest --test-dir C:\Users\enesp\Desktop\coding\nes-contra-us\build-coverage --output-on-failure
gcov -b -c C:\Users\enesp\Desktop\coding\nes-contra-us\build-coverage\port\CMakeFiles\contra_core.dir\contra_core\src\core.c.gcda
```

Export and compare native and original-ROM checkpoint traces manually:

```powershell
ctest --test-dir C:\Users\enesp\Desktop\coding\nes-contra-us\build_win -R "checkpoint_trace_export|mesen_checkpoint_trace_export" --output-on-failure
C:\Users\enesp\Desktop\coding\nes-contra-us\build_win\port\contra_checkpoint_trace_compare_attract_semantics.exe C:\Users\enesp\Desktop\coding\nes-contra-us\build_win\port\mesen_checkpoint_trace.jsonl C:\Users\enesp\Desktop\coding\nes-contra-us\build_win\port\contra_checkpoint_trace.jsonl
C:\Users\enesp\Desktop\coding\nes-contra-us\build_win\port\contra_checkpoint_trace_compare.exe C:\Users\enesp\Desktop\coding\nes-contra-us\build_win\port\mesen_checkpoint_trace.jsonl C:\Users\enesp\Desktop\coding\nes-contra-us\build_win\port\contra_checkpoint_trace.jsonl
```

The semantic attract comparison is part of the passing CTest suite. The strict
comparison is the intended final equivalence gate; today it is not part of the
passing suite because it exposes known native-vs-original differences.

## Current Gates

The native behavior tests currently cover:

- title/start handoff into level 1 gameplay
- level 1 right-scroll behavior under player input
- level 1 native enemy-screen data loading and enemy activation while scrolling
- level 1 bullet collision turning a defeated enemy into an explosion actor
- level 1 weapon item pickup changes weapon RAM, weapon strength, and bullet type
- level 1 bridge destruction reaches the native broken-overlay state
- level 1 forced boss-defeated path exercises end-level subroutine states and
  hands off into level 2 initialization
- attract mode reaching level 2 gameplay
- attract mode level 1 demo consuming ROM-backed demo input, loading enemy data,
  and reaching the first screen milestone before ending
- level 2 indoor floor landing
- level 2 first indoor room rendering has nontrivial framebuffer detail
- level 2 room advance after screen clear and up input
- level 2 up input before screen clear electrocutes without advancing the room
- level 2 ROM-backed indoor enemy screen data loads the first wall core and core
  count
- level 2 wall-core destruction decrements `WALL_CORE_REMAINING`, delays
  `INDOOR_SCREEN_CLEARED`, renders the back-wall quadrant updates, and then
  allows room advance
- level 2 soldier generator consumes the ROM-derived attack script, creates
  indoor soldier actors, and increments `INDOOR_ENEMY_ATTACK_COUNT` only on
  scripted attack-round entries
- generated level 2 indoor soldiers fire native enemy projectiles after entering
  the central attack window
- level 2 room 3 loads the roller generator and produces a native roller row
- level 2 native enemy projectiles move, render through the shared native sprite
  path, and can transition the player into the death state on collision
- level 2 room advance changes decoded room supertiles and framebuffer output
- attract mode reaching level 2 and loading the first wall core without an
  early room-clear flag
- repeated forced level 2 indoor room advances reaching `LEVEL_LOCATION_TYPE=0x80`
  while preserving active player state
- level 2 boss-room enemy data loads the ROM-defined boss eye, wall cannons,
  and wall plating actors at screen 5
- level 2 boss-room wall plating can be destroyed, then the boss eye can be
  defeated and hand off to the end-level routine
- level 2 boss-room wall cannons fire native projectiles, and boss-room
  projectiles can transition the player into the death state on collision
- deterministic checkpoint traces for level 1 right-scroll and level 2 indoor
  room-chain milestones through boss defeat and end-level handoff
- machine-readable checkpoint JSONL export for future native-vs-ROM comparison
- JSONL schema/order/hash validation so exported checkpoints are CI-checkable
- JSONL trace comparator scaffold, currently self-tested against the exported
  native trace until an original-ROM trace is available
- Mesen original-ROM level 1 and level 2 attract checkpoint trace export,
  including the original terminal level 2 demo state, and schema validation
  when `Mesen.exe` and `contra.nes` are available
- native-vs-original attract/demo semantic trace comparison for shared routine,
  level, screen, player, lives, game-over, demo-end, indoor-clear, and wall-core
  remaining fields, with documented timing and position tolerances
- original-ROM and native attract/demo checkpoints for level 2 first-room wall
  core load, wall-core destruction, room clear, and screen-1 advance
- original-ROM-only level 2 demo-finished checkpoint, currently documenting the
  next native timing/death drift target rather than a passing equivalence gate

These are not enough to claim full level 1 or level 2 parity. They are the first
CI-style gates for the areas we are actively stabilizing.

Most recent coverage run for `port/contra_core/src/core.c`:

- lines executed: 80.55% of 4051
- branches executed: 86.21% of 2204
- branches taken at least once: 69.28% of 2204
- calls executed: 78.74% of 508

## Required Next Gates

Level 1 high-confidence gates:

- bridge destruction changes collision and rendering state in an organic level run
- organic boss defeated path reaches end-level sequence and level 2 handoff
  without forcing the boss flag
- selected framebuffer region hashes for stable level 1 moments

Level 2 high-confidence gates:

- level 2 attract demo continues beyond screen 1 without ending early, including
  later wall-core variants
- indoor electrocution only happens before room clear
- precise original projectile physics, remaining generator variants, and
  room-clear variants are native, not forced
- original-vs-native checkpoints for level 2 boss-room gameplay rather than
  native-only boss-room behavior tests

Equivalence gates:

- expand the deterministic original-ROM Mesen runner to level 2 room-chain and
  additional input streams
- make native-vs-original checkpoint comparison a required gate once intentional
  timing/state differences are closed
- compare PPU-facing state before comparing full framebuffer output
- store input recordings for title flow, level 1, level 2, deaths, weapons, and
  transitions

## Equivalence Standard

A translated routine should only be considered equivalent when it has all of
these:

- a row in `port/coverage/asm_routines.csv` with the original label, native
  symbol, status, and test names
- native tests that execute the routine through the public core API instead of
  only direct helper calls
- a coverage run showing the native symbol's implementation is actually reached
- differential checkpoints against the original ROM for the same input stream

Until the differential runner exists, the project can claim regression coverage
for named scenarios, but not proof of original-game equivalence.
