# Native Port Coverage

Last updated: 2026-04-16

This file tracks native-port coverage by ROM bank and subsystem.

Important limits:

- This is not line coverage.
- This is not yet a per-label or per-address translation ledger.
- Status is based on the current native runtime path in `port/contra_core/src/core.c`.
- "ROM-backed" means the native port is consuming original bank data directly, but the original bank logic is not necessarily translated 1:1.

## Status Legend

- `Translated`: native C logic exists and is on the active runtime path.
- `ROM-backed`: original ROM data is being read and used directly.
- `Stubbed`: the hook exists, but behavior is placeholder or no-op.
- `Missing`: not translated yet, or only covered by a narrow one-off path.

## Bank 0

Primary responsibility in the original game: enemy routines.

Current coverage:

- `Translated`: native level 1 enemy update loop.
- `Translated`: native level 1 enemy projectile update loop.
- `Translated`: native level 1 player-vs-enemy collision checks.
- `Translated`: level 1 enemy spawning from screen data once the screen records have been loaded.
- `Translated`: level 1 generated soldier spawning.
- `Translated`: bridge segment state and collision-gap handling for level 1.

Currently covered enemy types in the native level 1 path:

- `0x00`: weapon item drop / pickup actor
- `0x02`: stationary gun emplacement emerge/active cycle
- `0x03`: flying capsule
- `0x04`: turret aiming/firing
- `0x05`: running soldier
- `0x06`: fixed soldier variants
- `0x07`: pop-up turret / rocky background variant
- `0x10`: heavy target / large stationary target
- `0x12`: bridge segments

Major gaps:

- `Missing`: enemy routines for levels 2 through 8.
- `Missing`: most boss logic outside the current level 1 path.
- `Missing`: shared enemy behaviors that still live in original bank 0 or bank 7 for later levels.

## Bank 1

Primary responsibility in the original game: audio engine and sprite data.

Current coverage:

- `ROM-backed`: sprite pointer tables are read directly from the ROM.
- `ROM-backed`: sprite definitions are read directly from the ROM.
- `Translated`: native sprite decode and framebuffer rendering path consumes the ROM sprite data.

Major gaps:

- `Stubbed`: sound command playback (`contra_play_sound`).
- `Stubbed`: APU initialization (`contra_init_apu_channels`).
- `Missing`: actual audio engine translation.
- `Missing`: timing-faithful sound playback and mixing.

## Bank 2

Primary responsibility in the original game: level screen data, player sprite logic, level headers, enemy screen data, soldier generation.

Current coverage:

- `ROM-backed`: level screen supertile pointer tables.
- `ROM-backed`: per-screen RLE supertile data.
- `ROM-backed`: level headers.
- `ROM-backed`: level 1 enemy screen data.
- `ROM-backed`: alternate graphics references.
- `ROM-backed`: demo input tables and demo input pointer table.
- `Translated`: player aim, jump, ledge-fall, landing, and basic movement state.
- `Translated`: player water entry, in-water, and walk-out sprite/state path.
- `Translated`: player bullet spawn/update/render support used by the native gameplay path.
- `Translated`: level 1 screen-enemy loading.
- `Translated`: level 1 soldier generation flow.
- `Translated`: alternate graphics loading hook used by the native port.
- `Translated`: attract-mode demo playback shell that consumes the ROM-backed demo tables.

Major gaps:

- `Missing`: full water/swim parity for all edge cases and later-level interactions.
- `Missing`: complete indoor player sprite/state coverage.
- `Missing`: native consumption of non-level-1 enemy screen data.
- `Missing`: exact parity for outdoor vertical scrolling cases that rely on bank 2 data plus bank 7 scroll/collision logic.

## Bank 3

Primary responsibility in the original game: supertile pattern data, supertile palette data, end-of-level routines.

Current coverage:

- `ROM-backed`: supertile pattern data.
- `ROM-backed`: supertile palette data.
- `Translated`: native background renderer consumes the bank 3 supertile and palette data.
- `Translated`: palette load/cycle support used in the active level path.
- `Translated`: part of the scroll update path for the currently supported gameplay loop.
- `Translated`: Level 1 post-boss end-of-level walk / jump tunnel exit sequence.

Major gaps:

- `Missing`: non-Level-1 end-of-level routine translation.
- `Missing`: exact vertical outdoor nametable update parity.
- `Missing`: broader bank 3 logic outside the currently exercised background and palette path.

## Bank 4

Primary responsibility in the original game: compressed graphics data, ending scene, ending credits logic.

Current coverage:

- `ROM-backed`: graphics data consumed by native graphics-loading code.

Major gaps:

- `Missing`: ending scene logic.
- `Missing`: ending credits logic.
- `Missing`: bank 4 non-graphics runtime code.

## Bank 5

Primary responsibility in the original game: compressed graphics data and demo input data.

Current coverage:

- `ROM-backed`: graphics data consumed by native graphics-loading code.
- `ROM-backed`: demo input data consumed by attract-mode playback.
- `Translated`: attract-mode sequencing shell in the native core.

Major gaps:

- `Missing`: any bank-5-specific runtime logic not already reduced to data consumption.

## Bank 6

Primary responsibility in the original game: short text, weapon logic, bullet logic, additional graphics data.

Current coverage:

- `ROM-backed`: short text tables used by the UI/text writer.
- `ROM-backed`: graphics data consumed by native graphics-loading code.
- `Translated`: native text/palette writes for title, stage name, menu text, and player/lives UI.
- `Translated`: player firing gate checks.
- `Translated`: player bullet spawn, movement, and draw path.
- `Translated`: weapon pickup state updates and repeat-fire timing.

Major gaps:

- `Missing`: full confidence on parity for every weapon-specific behavior.
- `Missing`: bank 6 behavior that depends on not-yet-ported audio or later-level interactions.

## Bank 7

Primary responsibility in the original game: core engine, dispatch, controller, scrolling, collision, shared enemy logic, rendering support, game flow.

Current coverage:

- `Translated`: native RAM model mirrors the original CPU RAM layout.
- `Translated`: controller state and edge-diff handling.
- `Translated`: high-level game routine dispatch.
- `Translated`: game routines `00` through `05` on the current native path.
- `Translated`: game-over/continue flow for the current native path.
- `Translated`: level routines `00` through `0A` on the current Level 1 native path, including post-boss handoff.
- `Translated`: intro/title/player-select/game-start flow currently used by the port.
- `Translated`: level header loading, graphics setup, palette loading, and early level bring-up.
- `Translated`: outdoor horizontal collision lookup path.
- `Translated`: data-backed outdoor vertical collision lookup for the active native path.
- `Translated`: player core state loop for the currently supported gameplay path.
- `Translated`: frame scroll integration for the currently supported horizontal outdoor gameplay path.
- `Translated`: native sprite buffer render path and framebuffer composition.
- `Translated`: demo level stepping and demo input playback support.

Major gaps:

- `Missing`: exact outdoor vertical collision parity with the original BG collision memory semantics.
- `Missing`: exact outdoor vertical scroll/render parity.
- `Missing`: remaining game and level routines that are not yet on the broader native runtime path, especially post-Level-1 content.
- `Missing`: large parts of shared enemy logic for later levels.
- `Missing`: exact NMI/IRQ/APU behavior.
- `Missing`: full semantics coverage outside the currently supported level 1 focused gameplay loop.

## Highest-Risk Gaps Right Now

These are the gaps most likely to produce visible gameplay mismatches:

- Water/swim logic.
- Outdoor vertical collision and scrolling parity, especially level 3.
- Audio, because bank 1 sound hooks are still stubbed.
- Non-level-1 enemy and boss coverage.

## Tracking Plan

For now, updates to this file should do two things:

- move items between `Missing`, `Stubbed`, `ROM-backed`, and `Translated`
- name the exact subsystem or enemy types that changed

The next improvement to this tracker should be a per-bank label/routine ledger with original symbol names and a native status for each one.
