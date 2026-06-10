#!/usr/bin/env python3
"""Frame-by-frame comparison of a recorded HUMAN play session: native port vs
real ROM (Mesen).

Pipeline:
  1. tools/mesen_play_recorder.lua  -- play in Mesen, records input + state
  2. port/tools/play_replay_trace.c -- replays the input through the native core
  3. this tool                      -- finds the first divergence

What is compared: the observable GAMEPLAY STATE (level, scroll, player/enemy/
bullet state, controller latch). Region hashes and oam_offset are excluded --
the port is a high-level reimplementation, not byte/cycle-exact, so those never
match and would mask the comparison (same policy as compare_demo_frame_trace.py).

The FIRST divergence is the only one that matters: everything after it cascades.
Three failure classes are distinguished:
  - input divergence: the controller fields split first -> the recording/replay
    is misaligned or a Mesen DPCM controller-read glitch hit that frame; fix the
    replay (CONTRA_NATIVE_PLAY_INPUT_OFFSET / CONTRA_NATIVE_PLAY_INPUT=latched)
    before believing anything downstream.
  - frame shift: native[f..] matches mesen[f+d..] -- a single timing slip
    (one bug), not a wall of divergence.
  - logic divergence: a real port bug; the fields and the per-field summary
    localize it.

Usage:
    compare_play_trace.py NATIVE.jsonl MESEN.jsonl [--frame N] [--list N] [--baseline N]
      --frame N    print the full field diff at frame N and exit
      --list N     show the first N divergent frames (default 5)
      --baseline N exit 1 if the traces diverge before frame N (CI guard)
"""
import json
import sys
from collections import Counter

EXCLUDE = {
    "ram_hash", "pattern_hash", "nametable_hash", "palette_hash",
    "framebuffer_hash", "oam_offset", "frame", "polls", "p1_raw", "p2_raw",
    # RANDOM_NUM is cycle-timing dependent in the ROM (busy-loop between
    # frames) and injected from the recording during replay -- never a signal.
    "rng",
}

INPUT_FIELDS = {"controller", "p2_controller", "controller_diff", "p2_controller_diff"}

SHIFT_RANGE = range(-3, 4)
SHIFT_WINDOW = 60


def load(path):
    frames = {}
    with open(path) as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            try:
                record = json.loads(line)
            except json.JSONDecodeError:
                continue
            if "frame" not in record:
                continue  # meta row
            frames[record["frame"]] = record
    return frames


def gameplay_diff(a, b):
    return [k for k in a if k in b and k not in EXCLUDE and a[k] != b[k]]


def print_frame_diff(native, mesen, frame):
    a = native.get(frame, {})
    b = mesen.get(frame, {})
    keys = gameplay_diff(a, b)
    print(f"\n=== gameplay diff at frame {frame} ({len(keys)} fields) ===")
    for k in keys:
        print(f"  {k:<20} native={a[k]!s:<10} mesen={b[k]}")
    if not keys:
        print("  (identical)")


def detect_shift(native, mesen, start):
    """If native[f] == mesen[f+d] over a window after the divergence, the port
    slipped d frame(s) at one point rather than diverging in logic."""
    for d in SHIFT_RANGE:
        if d == 0:
            continue
        compared = 0
        for f in range(start, start + SHIFT_WINDOW):
            if f not in native or (f + d) not in mesen:
                continue
            compared += 1
            if gameplay_diff(native[f], mesen[f + d]):
                break
        else:
            if compared >= SHIFT_WINDOW // 2:
                return d
    return None


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2

    native = load(argv[1])
    mesen = load(argv[2])
    want_frame = int(argv[argv.index("--frame") + 1]) if "--frame" in argv else None
    list_count = int(argv[argv.index("--list") + 1]) if "--list" in argv else 5
    baseline = int(argv[argv.index("--baseline") + 1]) if "--baseline" in argv else None

    common = sorted(set(native) & set(mesen))
    if not common:
        print("FAIL: no common frames between the two traces")
        return 1
    print(f"native frames: {len(native)}  mesen frames: {len(mesen)}  "
          f"common: {len(common)} (frame {common[0]}..{common[-1]})")

    if want_frame is not None:
        print_frame_diff(native, mesen, want_frame)
        return 0

    divergent = []
    last_match = 0
    broken = False
    field_counts = Counter()
    for f in common:
        # A mesen row whose FRAME_COUNTER equals the previous row's is a real-NES
        # lag frame: the game loop never finished, so the recorder's snapshot is
        # a torn mid-iteration state and the replay skips stepping that frame
        # (the lag schedule). Skip comparing it.
        prev = mesen.get(f - 1)
        if prev is not None and prev.get("frame_counter") == mesen[f].get("frame_counter"):
            continue
        keys = gameplay_diff(native[f], mesen[f])
        if not keys:
            if not broken:
                last_match = f
            continue
        broken = True
        divergent.append((f, keys))
        field_counts.update(keys)

    print(f"\ngameplay-identical from frame {common[0]} through: {last_match}")

    if not divergent:
        print("*** gameplay-identical across the ENTIRE common range. ***")
        return 0

    first_frame, first_keys = divergent[0]

    # Did the game-consumed input split before (or at) the first divergence?
    # If so the comparison downstream is meaningless until the input is fixed.
    if set(first_keys) <= INPUT_FIELDS:
        print(f"\n!!! INPUT divergence at frame {first_frame}: {first_keys}")
        print("    The input the game consumed differs before any gameplay does.")
        print("    Check the replay tool's stderr for an alignment warning")
        print("    (CONTRA_NATIVE_PLAY_INPUT_OFFSET), or retry with")
        print("    CONTRA_NATIVE_PLAY_INPUT=latched (Mesen DPCM read glitch).")

    print(f"\nfirst divergence: frame {first_frame}  fields={first_keys}")
    for k in first_keys:
        print(f"    {k:<18} native={native[first_frame][k]!s:<10} mesen={mesen[first_frame][k]}")

    shift = detect_shift(native, mesen, first_frame)
    if shift is not None:
        print(f"\n>>> FRAME SHIFT: from frame {first_frame} the native trace matches the")
        print(f">>> mesen trace offset by {shift:+d} frame(s). This is ONE timing slip at")
        print(f">>> ~frame {first_frame} (a frame gained/lost), not {len(divergent)} separate bugs.")

    if len(divergent) > 1:
        print(f"\nfirst {min(list_count, len(divergent))} divergent frames "
              f"(of {len(divergent)} total):")
        for f, keys in divergent[:list_count]:
            shown = ", ".join(keys[:6]) + (" ..." if len(keys) > 6 else "")
            print(f"  frame {f}: {shown}")

        print("\nfields diverging most often (cascade included):")
        for field, count in field_counts.most_common(12):
            print(f"  {field:<22} {count} frames")

    # Per-level breakdown (segments from the real ROM's `level` byte). The
    # replay is ONE timeline: a divergence inside level N contaminates every
    # later level, so work top-down -- the first dirty level is the only
    # actionable one.
    print("\n=== per-level breakdown (fix top-down; lower levels cascade into higher) ===")
    segments = []
    for f in common:
        lvl = mesen[f].get("level")
        if segments and segments[-1][0] == lvl:
            segments[-1][2] = f
        else:
            segments.append([lvl, f, f])
    frontier_passed = False
    for lvl, lo, hi in segments:
        if hi - lo < 120:
            continue  # transition blip
        seg_frames = [f for f in range(lo, hi + 1) if f in native and f in mesen]
        seg_div = [f for f in seg_frames if gameplay_diff(native[f], mesen[f])]
        if not seg_div:
            status = "CLEAN"
        else:
            pct = 100.0 * (seg_div[0] - lo) / max(1, hi - lo)
            status = f"diverges at frame {seg_div[0]} ({pct:.0f}% in), {len(seg_div)} divergent frames"
            if frontier_passed:
                status += "  [cascade-contaminated]"
            frontier_passed = True
        print(f"  stage {lvl + 1} (frames {lo:>6}-{hi:>6}): {status}")

    print(f"\n(use --frame {first_frame} for the full diff there; re-run the replay with")
    print(f" CONTRA_NATIVE_PLAY_DUMP_FRAME={first_frame} and the *_DUMP_PATH vars, plus the")
    print(" matching Mesen dump hooks, then tools/ramdiff.py for a byte-level look)")

    if baseline is not None and last_match < baseline:
        print(f"\nFAIL: diverged at {first_frame}, before baseline {baseline}.")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
