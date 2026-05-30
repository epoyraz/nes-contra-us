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
   core today and are shaped to accept original-ROM checkpoints later.

2. C code coverage

   `CONTRA_PORT_ENABLE_COVERAGE=ON` instruments the native C targets with
   GCC/Clang coverage flags. This answers which native implementation code ran.

3. ASM routine coverage ledger

   `port/coverage/asm_routines.csv` records original ASM labels and their port
   status. The `contra_coverage_manifest_tests` target fails if a
   `translated` or `partial` claim lacks both a native symbol and at least one
   test reference.

4. Differential testing

   This is the next reliability layer to add. A deterministic emulator harness
   should run the original ROM and the native core frame-by-frame on the same
   input stream, then compare RAM and rendering checkpoints. Until this exists,
   confidence is scenario-based rather than equivalence-proof based.

## Commands

Build and run the normal tests:

```powershell
cmake --build C:\Users\enesp\Desktop\coding\nes-contra-us\build_win --target contra_core_tests contra_checkpoint_trace_tests contra_coverage_manifest_tests
ctest --test-dir C:\Users\enesp\Desktop\coding\nes-contra-us\build_win --output-on-failure
```

Create and run a coverage build:

```powershell
cmake -S C:\Users\enesp\Desktop\coding\nes-contra-us -B C:\Users\enesp\Desktop\coding\nes-contra-us\build-coverage -G Ninja -DCONTRA_PORT_BUILD_SDL=OFF -DCONTRA_PORT_ENABLE_COVERAGE=ON
cmake --build C:\Users\enesp\Desktop\coding\nes-contra-us\build-coverage --target contra_core_tests contra_checkpoint_trace_tests contra_coverage_manifest_tests
ctest --test-dir C:\Users\enesp\Desktop\coding\nes-contra-us\build-coverage --output-on-failure
gcov -b -c C:\Users\enesp\Desktop\coding\nes-contra-us\build-coverage\port\CMakeFiles\contra_core.dir\contra_core\src\core.c.gcda
```

## Current Gates

The native behavior tests currently cover:

- title/start handoff into level 1 gameplay
- level 1 right-scroll behavior under player input
- level 1 native enemy-screen data loading and enemy activation while scrolling
- level 1 bullet collision turning a defeated enemy into an explosion actor
- level 1 boss-defeated handoff into level 2 initialization
- attract mode reaching level 2 gameplay
- level 2 indoor floor landing
- level 2 room advance after screen clear and up input
- level 2 up input before screen clear electrocutes without advancing the room
- attract mode advancing level 2 to the next indoor room
- repeated forced level 2 indoor room advances reaching the boss-room state
- deterministic checkpoint traces for level 1 right-scroll and level 2 indoor
  room-chain milestones

These are not enough to claim full level 1 or level 2 parity. They are the first
CI-style gates for the areas we are actively stabilizing.

Most recent coverage run for `port/contra_core/src/core.c`:

- lines executed: 77.24% of 3712
- branches executed: 82.23% of 1947
- branches taken at least once: 64.66% of 1947
- calls executed: 75.41% of 488

## Required Next Gates

Level 1 high-confidence gates:

- level 1 demo reaches expected screen milestones before ending
- bridge destruction changes collision and rendering state
- weapon capsule pickup changes weapon RAM and bullet behavior
- organic boss defeated path reaches end-level sequence and level 2 handoff
- selected framebuffer region hashes for stable level 1 moments

Level 2 high-confidence gates:

- level 2 attract demo advances through indoor room steps without ending early
- indoor room rendering has nontrivial framebuffer detail
- indoor electrocution only happens before room clear
- enemy wall cores and room-clear state are native, not forced
- boss-room handoff sets `LEVEL_LOCATION_TYPE=0x80` and preserves player state

Equivalence gates:

- add a deterministic original-ROM emulator runner
- compare native and original RAM checkpoints for recorded input streams
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
