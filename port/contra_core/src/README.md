# Contra Core Source Map

`core.c` is intentionally still one C translation unit. The implementation is
split into included fragments so the port can keep file-local ROM tables and
helpers `static` while making the code navigable.

## Engine

- `engine/tables_and_forwards.inc.c` — shared ROM-backed tables, constants, and forward declarations needed by later fragments.
- `engine/graphics_ppu.inc.c` — ROM loading, PPU writes, graphics decoding, nametable rendering, HUD, and score drawing.
- `engine/player_weapons.inc.c` — player bullets and weapon firing/update logic.
- `engine/player_movement.inc.c` — collision sampling, player sprites, movement states, death, jumping, water, and scrolling.
- `engine/game_flow.inc.c` — controller input, intro/demo/player-select flow, level header loading, palette/scroll setup, and early level routines.
- `engine/level_loop.inc.c` — gameplay loop, enemy/spawn execution, end-level flow, game-over flow, pause, and ending sequence.
- `engine/render_overlays.inc.c` — persistent background overlay rendering and final frame assembly.
- `engine/public_api.inc.c` — public `contra_core_*` API entry points and debug warps.

## Enemies

- `enemies/common_helpers.inc.c` — enemy spawn data, property tables, generic enemy helpers, bullets, shared soldiers, and common enemy types.
- `enemies/dispatch_collision_generation.inc.c` — enemy dispatch, bullet/enemy collision, player/enemy collision, indoor room data, and outdoor soldier generation.

## Levels

- `levels/level1_jungle.inc.c` — Level 1 jungle enemy routines and fortress boss door.
- `levels/level2_4_indoor_bases.inc.c` — Level 2 and Level 4 indoor base enemies, room generators, and boss rooms.
- `levels/level3_waterfall.inc.c` — Level 3 waterfall dragon boss mouth and arm/orb routines.
- `levels/level5_snowfield.inc.c` — Level 5 snowfield grenade, tank, UFO, saucer, and bomb routines.
- `levels/level6_energy_zone.inc.c` — Level 6 fire beams, giant soldier boss, and spiked disk routines.
- `levels/level7_hangar_and_level8_alien_lair.inc.c` — Level 7 hangar mechanisms/boss plus Level 8 alien lair enemies.
