# Level 1 Enemy Floor Offset — Investigation Notes

**Status:** root cause identified; an experimental (non-faithful) fix is currently
sitting uncommitted in the working tree. Read "Current working-tree state" before
you build.

## Symptom

On level 1, ground enemies (the running/pop-up soldiers) render a few pixels
**below the floor** instead of standing on it. They should sit at the **same
height as the player** when both are on the same ground; they don't.

## TL;DR

The level-1 enemies are **not a port of the original ASM**. They are an invented,
hand-written C state machine living in a side struct (`core->enemies[]`), outside
the emulated NES RAM. Their Y comes from a coarse 16-px spawn grid and is never
settled onto the ground, so they sit low. The faithful pieces of the port
(rendering, spawn-data decode, background origin) are correct and are **not** the
cause.

## What I verified is faithful (i.e. NOT the bug)

| Concern | Port | Original | Verdict |
|---|---|---|---|
| Sprite tile Y math | `contra_render_cpu_sprite` (`core.c:~1019`) | `draw_sprites` (`bank1.asm:3998`) | matches exactly (small `base_y-8`, regular `base_y+rel_y`, vflip `base_y+(0xF0-rel_y)`, `+1` recoil bit) |
| Level-1 enemy spawn Y read | `enemy->y = enemy_y_attrs & 0xF0` (`core.c:~6045`) | `@handle_horizontal` `lda $0c / and #$f0` (`bank2.asm:1598-1601`) | matches |
| Background floor origin | `origin_y = 16` (`core.c:~1144`) | `vertical_scroll = 0xE0` status-bar split (screen row 16 = nametable row 0) | correct |

So per-sprite rendering and the raw spawn coordinate are right.

## Root cause: the enemies are invented, not ported

Evidence in the code:

- Level-1 enemies are stored in **`core->enemies[]`** — a separate
  `ContraNativeEnemy` C struct array (`core.c:2532`, `:4541`, `:5272`, `:5836`,
  `:6097`, …), **not** the emulated NES RAM.
- The real ROM's `ENEMY_Y_POS` / `ENEMY_TYPE` RAM arrays are **never used** by the
  level-1 enemy code (zero references).
- The behavior is a bespoke state machine: `CONTRA_NATIVE_LEVEL1_STATE_WAIT /
  EMERGE / ACTIVE` (`core.c:312`), a custom spawn loop (`core.c:~5976`), and
  hand-tuned constants.
- The soldiers keep their **static, coarse 16-px** spawn Y and never snap to the
  ground. The only native enemy that consults the floor at all is the weapon pod
  (case `0x00`), via `contra_find_level_1_floor_y_below` (`core.c:~4108`) — and
  that scans collision on a coarse **16-px** grid even though the collision map is
  **8-px** (`contra_get_outdoor_horizontal_bg_collision`, `core.c:2565`).

Why it looks "a little below" specifically: the **player** settles onto the floor
through `contra_set_player_landing_y_offset` (`core.c:~3405`), which snaps to a
16-px grid **plus a 4-px fine offset** derived from `VERTICAL_SCROLL`. Player and
soldier share the same `+0x10` foot anchor (player foot sample `core.c:2648`;
soldier foot sample `core.c:~5572`). The player gets the grid+4 snap; the soldier
gets neither → it ends up below the player's standing line.

### Why the Mesen tests don't catch it

Because the enemies live **outside** the emulated RAM, they're outside the
checkpoint state that's compared against the real ROM. The
`contra_mesen_checkpoint_trace_*` tests pass while the enemies are visibly wrong.
That divergence (Mesen-green but visually wrong) is itself the signal that this
subsystem isn't a real port.

## How this came to be

The port faithfully translates the **core engine and rendering** (those trace to
the ASM and are validated against Mesen traces — that's why those tests are
green). The **per-level enemy AI was stubbed** with native C approximations to get
something animating on screen, rather than translated from the original enemy
routines. Net result: a faithful skeleton with invented enemies bolted on the
side.

## The experimental fix currently in the tree (NOT faithful — likely to be reverted)

I added a heuristic that routes grounded soldiers through the player's landing
snap, seeded from an 8-px floor scan:

- `contra_outdoor_landing_snap_y` (`core.c:3391`) — extracted from the player
  landing code (player path unchanged, just refactored to share it).
- `contra_find_outdoor_floor_surface_y` (`core.c:4148`) — 8-px floor-surface scan.
- `contra_snap_native_enemy_to_outdoor_floor` (`core.c:4179`) — seats the enemy at
  the player's standing height on the detected floor.
- Called from the walking soldier (case `0x05`, `core.c:5593`) and the pop-up
  soldier once ACTIVE (case `0x02`, `core.c:5413`, `:5419`).

**This is invented, not derived from any ASM routine.** It makes the picture look
right but is not an exact port. It also breaks 2 golden-master baselines:
`level1-gameplay-start` and `level1-scroll-mid` in `contra_checkpoint_trace_tests`
(+ its export). To restore the known-green state: `git checkout --
port/contra_core/src/core.c`.

## The faithful fix (recommended)

Port the real soldier logic from the disassembly onto the genuine `ENEMY_*` RAM,
so enemy Y comes from the actual algorithm and lands inside the Mesen-validated
state:

- Soldier spawning/cadence: `exe_soldier_generation` + `soldier_generation_00/01/02`
  (`bank2.asm:1698`).
- Per-enemy ground/Y logic: the enemy collision helpers
  `get_bg_collision` (`bank7.asm:6389`), `add_y_to_y_pos_get_bg_collision`
  (`bank7.asm:8679`), `add_a_y_to_enemy_pos_get_bg_collision` (`bank7.asm:8692`).
- See `docs/Enemy Routines.md` for the soldier-generation overview and the enemy
  data-byte format.

Validate by extending the Mesen checkpoint comparison to cover the `ENEMY_*` RAM
once enemies live there, so correctness is provable rather than eyeballed.

## Open decision

1. Revert the heuristic and port the real soldier routines (exact, larger task), or
2. Keep the heuristic as a visual stopgap and update the 2 golden-master baselines,
   porting the real AI later, or
3. Just revert to restore green and decide later.
