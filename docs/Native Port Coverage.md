# Native Port Coverage

Last updated: 2026-06-12 (after splitting the native core into source fragments)

This file tracks native-port coverage by ROM bank and subsystem.

## Headline

**Every game system has a running, faithful implementation**: all 8 levels'
enemies and bosses, all level flows (intro cards, gameplay, boss defeats,
end-of-level walks), the full game-end sequence (screen melt, helicopter
island escape, credits crawl, second-loop handoff), scoring, and the demo.
The one deliberately unported subsystem is the **bank 1 APU sound engine**
(the port records sound codes; it does not synthesize audio).

Verification status:

- **Stages 1-4 (levels 1-4, 39,970 frames)**: continuously verified by the
  play-parity pipeline (`tools/frontier.sh`) — zero structural divergence
  against the Mesen ground-truth recording.
- **Levels 5-8 and the ending**: ported statically from the assembly (build +
  unit smoke tests + the regression staying green). **No recording covers them
  yet** — recording stages 5-8 with the v5 pipeline is the next verification
  step. Treat their behavior as ported-but-unverified.
- The v4 reference trace has no `score` field, so score correctness in stages
  1-4 is not continuously checked (it was spot-verified once via the score
  side-channel traces). The v5 schema makes score structural.

## Visualization (line-weighted)

```
python3 tools/port_coverage.py             # per-bank line-weighted % bar chart
python3 tools/port_coverage.py --uncited 7 # bank-7 routines not yet cited as ported
```

It sums the ASM line ranges that the native core sources cite (convention
`bank<N>:<from>-<to>`; bare `bank<N>:<line>` citations auto-expand to the
enclosing routine) and divides by each bank's total lines. `core.c` carries the
consolidated ledger, while included fragments carry local routine citations.
Cite the ASM range whenever you faithfully port a routine.

Current numbers (2026-06-12):

```
bank0  92.6%  ( 9738/10511)  enemy routines
bank1  43.6%  ( 3138/ 7196)  sprite data + decode (the sound engine is the 0%)
bank2  98.4%  ( 3026/ 3074)  level/spawn data, player sprites, soldier gen
bank3  96.9%  ( 1541/ 1590)  supertile data, end-of-level routines
bank4  95.2%  (  767/  806)  graphic data, game-end/credits
bank5  89.0%  (  235/  264)  graphic data, demo input
bank6  98.3%  ( 1886/ 1919)  text data, player weapons/bullets
bank7  77.1%  ( 8322/10800)  engine core, collision, dispatch
TOTAL  79.2%  (28653/36160 assembly lines)
```

The total excluding the out-of-scope bank 1 sound engine (lines 1-4058) is
**~85%**; the remainder is NMI/PPU plumbing modeled by the native renderer at
a different layer, plus the deferred items below.

## Status legend

- `Ported`: faithful native C translation on the active runtime path.
- `ROM-backed`: the port reads the original data bytes from the ROM image at
  runtime (the correct way to "port" data).
- `Modeled`: the game-visible effect is reproduced through a native mechanism
  (e.g. the renderer) rather than a 1:1 translation of the PPU plumbing.
- `Not ported`: listed under Known gaps.

## Bank 0 — enemy routines (92.5%)

`Ported`: every enemy and boss for all 8 levels, including this milestone's
L5 tank, L5 boss UFO set (carrier/saucers/bombs), L8 alien guardian, and all
shared helpers (explosion trios, husk-keeping removes, the destroyed-routine
nibble routing, supertile stamping on the graphics budget).

Known gaps (the missing ~7%):

- Small uncited leaf helpers inside already-ported systems.

## Bank 1 — audio engine + sprite data (43.6%)

- `ROM-backed`: all sprite definitions and pointer tables (read at $B030/$B12E+).
- `Ported`: the multi-tile sprite decode (`load_sprite_to_cpu_mem`) and HUD
  sprites, as the native OAM builder.
- `Not ported` (**by design**): the APU sound engine (lines 1-4058).
  `contra_play_sound` records sound codes; there is no audio synthesis. This
  is the entire remaining 56% of the bank.

## Bank 2 — level data, player sprites, soldier generation (98.4%)

- `ROM-backed`: all levels' super-tile screen data, level headers, enemy
  screen data, alt-graphics refs, demo tables.
- `Ported`: player sprite/state code (pause, death, water entry/swim/exit,
  indoor walks, boss-screen aiming), enemy screen-data loaders for all levels,
  and the full soldier-generation system (timer carry chains, find-pos ring
  semantics — both verified frame-exact).

## Bank 3 — supertile data + end-of-level (96.9%)

- `ROM-backed`: every level's super-tile, nametable-update, palette, and
  tile-animation data (consumed by the renderer and the overlay caches).
- `Ported`: the end-level sequence and all eight per-level end-of-level
  routines (L1 tunnel jump, indoor elevators, L3 gate, L5/6/7 walk-right with
  background-priority triggers, L8 timer).

## Bank 4 — graphic data + game end (95.2%)

- `ROM-backed`: graphic data blobs; ending credits text (read live by the
  credits crawl).
- `Ported`: the full game-end flow — screen melt, island scene, helicopter
  escape, destroyed-island redraw, credits crawl, the 2nd-loop handoff.

## Bank 5 — graphic data + demo input (89.0%)

- `ROM-backed`: graphic data, demo input tables.
- `Ported`: demo input playback (attract mode is frame-exact vs the ROM).

## Bank 6 — text + player weapons/bullets (98.3%)

- `ROM-backed`: short text tables, transition/intro palettes, graphic data
  (the text writer reads bank 6 bytes at runtime).
- `Ported`: the entire player weapon and bullet system (fire gates, per-weapon
  spawn/velocity/update, F swirl, S spread, L laser) — continuously verified
  through the `pbul` digest field across all four recorded stages.

## Bank 7 — engine core (76.1%)

- `Ported`: game/level routine state machines (00-0A + game_routine_06),
  controller reading (including the DPCM latch glitch), scoring, timers,
  collision (outdoor horizontal + the vertical BG_COLLISION_DATA ring
  semantics + the world-anchored override list for runtime clears), enemy
  dispatch and all shared enemy helpers, aim/rotation helpers and bullet
  velocity solves, palette pipeline, graphics decompressor, text writer.
- `Modeled`: the PPU write plumbing (supertile stamp pipeline, attribute
  writers, column streaming) — the native renderer reproduces the visible
  output from the same data; PPU addr math mirrors `set_ppu_addresses_in_mem`.

Known gaps (the missing 24%):

- `Not ported`: NMI/IRQ entry and sound dispatch plumbing (232-441).
- `Not ported`: the NMI graphics-buffer group-format drain
  (`write_cpu_graphics_buffer_to_ppu`, 2666-2785) — gameplay nametable writes
  are modeled natively instead.
- `Modeled differently`: the BG_COLLISION_DATA ring **writer**
  (`set_supertile_bg_collisions`, 8218-8317) — runtime collision rewrites go
  through the override list.
- Unused/dead ROM labels and small uncited helpers.

## Known gaps, consolidated (the honest backlog)

(2026-06-11 sweep: the laser pass-through, rideable-enemy landing and the
carts' land/collide gate, soldier water splash, L6 fire-beam tile draw, L3/
L6/L7 kill-tail nibble routing, the 2P horizontal scroll_player branch, and
the wall-core aim were all closed; the items below are what remains.)

1. Bank 1 sound engine (out of scope by design — the port records sound
   codes only).
2. RNG is an approximation by design (the busy-loop iteration count is not
   reproducible without cycle accuracy); the replay pipeline injects the
   recorded RNG, so parity testing is unaffected.
3. Three renderer findings from the frame-diff sweep (frame 3000 corner +
   weapon-box supertile, frame 6500 phantom pill-box overlay) — see the
   memory/pipeline notes; game state is identical, pixels differ.
4. bank7 PPU plumbing modeled natively rather than translated: the NMI
   graphics-buffer group drain and the BG_COLLISION_DATA ring writer
   (runtime collision rewrites go through the world-anchored override list).

## Tracking plan

- When you faithfully port a routine, cite `bank<N>:<from>-<to>` in a comment
  (the `core.c` header ledger or at the function in the relevant fragment).
- When a recording of stages 5-8 exists, move L5-L8 systems from
  "ported-but-unverified" to "verified" here.
- Keep the Known-gaps list in sync with the `DEFERRED`/`not ported` comments
  in the native core sources — they are cross-referenced.
