#!/usr/bin/env python3
"""Frame-by-frame comparison of the attract DEMO between the native port and the
real ROM (Mesen), as an explicit, assertable test -- covering all three demo
levels (stage 1 -> 2 -> 3).

The NES attract demo replays a fixed ROM-recorded input script for levels 0, 1
and 2 (stages 1, 2, 3), returning to the intro between each. Both tracers
(port/tools/level1_frame_trace.c and tools/mesen_level1_frame_trace.lua) run that
demo with NO external input, so on a perfectly faithful port every gameplay field
matches the real ROM frame-for-frame through every demo level.

What is compared: the observable GAMEPLAY STATE (level, scroll, player/enemy
positions, lives, demo flags). The region HASHES and the OAM-buffer offset are
EXCLUDED -- the port is a high-level reimplementation, not a byte/cycle-exact
emulator, so those never match byte-for-byte and would mask the real comparison.

Because the demo's recorded input is position/timing sensitive, the FIRST
single-frame divergence in stage 1 desynchronises everything after it. So this
test's headline number -- "gameplay-identical from frame 1 through N" -- is the
real measure of faithfulness; the per-demo-level section below shows how far into
each stage's demo we get before (or whether) it diverges.

Usage:
    compare_demo_frame_trace.py NATIVE.jsonl MESEN.jsonl [--baseline N] [--frame N]

Exit status: 0 if the port stays gameplay-identical through at least --baseline
frames (default 1536, the current best); 1 if it regresses earlier. With --frame N
it prints the full field diff at frame N and always exits 0.
"""
import json
import sys

# The port is not byte-exact, so these never match and would mask the comparison.
EXCLUDE = {
    "ram_hash", "pattern_hash", "nametable_hash", "palette_hash",
    "framebuffer_hash", "oam_offset",
}

# Human stage name per internal 0-indexed level value.
STAGE_NAME = {0: "stage 1 (jungle)", 1: "stage 2 (base/indoor)", 2: "stage 3 (waterfall)"}


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
            frames[record["frame"]] = record
    return frames


def gameplay_diff(a, b):
    return [k for k in a if k in b and k not in EXCLUDE and a[k] != b[k]]


def demo_level_segments(frames):
    """Return [(level, start_frame, end_frame), ...] for each demo-play run.

    A demo level is actively playing while game_routine == 2 (the level-routine
    execution state, used for both real play and the attract demo). Outdoor stages
    (jungle, waterfall) have location_type == 0, so game_routine -- not
    location_type -- is the portable "is a stage demo running" signal. Each run is
    labelled by the level value it spends the most frames at (the `level` byte
    updates a frame or two after game_routine flips)."""
    from collections import Counter

    runs = []
    cur = None
    for f in sorted(frames):
        playing = frames[f].get("game_routine") == 2
        if playing and cur is None:
            cur = [f, f]
        elif playing:
            cur[1] = f
        elif cur is not None:
            runs.append(tuple(cur))
            cur = None
    if cur is not None:
        runs.append(tuple(cur))

    segments = []
    for lo, hi in runs:
        levels = Counter(frames[f].get("level") for f in range(lo, hi + 1) if f in frames)
        segments.append((levels.most_common(1)[0][0], lo, hi))
    return segments


def field_changes(frames, field, lo, hi):
    """Distinct values a field takes over [lo, hi]."""
    return sorted({frames[f].get(field) for f in frames if lo <= f <= hi})


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    native = load(argv[1])
    mesen = load(argv[2])
    baseline = 1536
    if "--baseline" in argv:
        baseline = int(argv[argv.index("--baseline") + 1])
    want_frame = None
    if "--frame" in argv:
        want_frame = int(argv[argv.index("--frame") + 1])

    common = sorted(set(native) & set(mesen))
    if not common:
        print("FAIL: no common frames between the two traces")
        return 1
    print(f"native frames: {len(native)}  mesen frames: {len(mesen)}  common: {len(common)}")

    if want_frame is not None:
        keys = gameplay_diff(native.get(want_frame, {}), mesen.get(want_frame, {}))
        print(f"\n=== gameplay diff at frame {want_frame} ({len(keys)} fields) ===")
        for k in keys:
            print(f"  {k:<20} native={native[want_frame][k]:<8} mesen={mesen[want_frame][k]}")
        if not keys:
            print("  (identical)")
        return 0

    # Headline: how far the two stay gameplay-identical.
    last_match = 0
    first_div = None
    first_keys = None
    broken = False
    for f in common:
        keys = gameplay_diff(native[f], mesen[f])
        if not keys:
            if not broken:
                last_match = f
            continue
        broken = True
        if first_div is None:
            first_div, first_keys = f, keys

    print(f"\ngameplay-identical from frame 1 through: {last_match}")
    if first_div is not None:
        print(f"first divergence: frame {first_div}  fields={first_keys}")
        for k in first_keys:
            print(f"    {k:<18} native={native[first_div][k]:<8} mesen={mesen[first_div][k]}")
    else:
        print("*** gameplay-identical across the ENTIRE common range. ***")

    # Per-demo-level breakdown (real ROM as ground truth for the windows).
    print("\n=== per demo-level segments (real ROM windows) ===")
    for lvl, lo, hi in demo_level_segments(mesen):
        name = STAGE_NAME.get(lvl, f"level {lvl}")
        seg_match = None
        for f in range(lo, hi + 1):
            if f in native and f in mesen:
                if gameplay_diff(native[f], mesen[f]):
                    break
                seg_match = f
        identical = seg_match == hi
        status = "IDENTICAL" if identical else (
            f"diverges (identical only to f{seg_match})" if seg_match else "diverges immediately")
        print(f"  {name:<22} demo frames {lo}-{hi}: {status}")
        # Stage 3 must scroll vertically upward in the real ROM; surface it.
        if lvl == 2:
            nv = field_changes(native, "vertical_scroll", lo, hi) if native else []
            mv = field_changes(mesen, "vertical_scroll", lo, hi)

            def summarize(vals):
                if not vals:
                    return "(none)"
                if len(vals) == 1:
                    return f"constant {vals[0]}"
                return f"{len(vals)} distinct values, range {min(vals)}..{max(vals)}"

            print(f"      vertical_scroll  native: {summarize(nv)}   mesen: {summarize(mv)}")
            if len(nv) <= 1 < len(mv):
                print("      ^ STAGE 3 DOES NOT SCROLL in the port (real ROM scrolls); see task #28.")

    if last_match < baseline:
        print(f"\nFAIL: regressed -- identical only through {last_match}, below baseline {baseline}.")
        return 1
    print(f"\nPASS: gameplay-identical through {last_match} (>= baseline {baseline}).")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
