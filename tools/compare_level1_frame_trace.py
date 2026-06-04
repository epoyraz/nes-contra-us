#!/usr/bin/env python3
"""Compare a native level-1 frame trace against the Mesen (real-ROM) trace.

The two traces share an identical per-frame JSON schema (see
port/tools/level1_frame_trace.c and tools/mesen_level1_frame_trace.lua). Both
run the level-1 attract demo with no input, so on a faithful port every field
and every region hash should match the real ROM frame-for-frame.

This is the steering wheel for the faithful port: it reports the FIRST frame
where the port diverges from the ROM and localizes the divergence to specific
fields, so the responsible 6502 routine can be found and ported.

Usage:
    compare_level1_frame_trace.py NATIVE.jsonl MESEN.jsonl [--frame N] [--max-rows K]

Without --frame: prints the first divergence overall, the first divergence of
each hash region, and a per-field table at the first divergent frame.
With --frame N: prints the full field-by-field diff at frame N.
"""
import json
import sys

HASH_FIELDS = [
    "ram_hash", "pattern_hash", "nametable_hash", "palette_hash", "framebuffer_hash",
]


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


def diff_keys(a, b):
    """Keys present in both records whose values differ, in schema order."""
    return [k for k in a if k in b and a[k] != b[k]]


def fmt_value(value):
    if isinstance(value, str):
        return value
    return str(value)


def print_field_table(native, mesen, keys):
    width = max((len(k) for k in keys), default=0)
    print(f"  {'field'.ljust(width)}   native      mesen")
    print(f"  {'-' * width}   ----------  ----------")
    for key in keys:
        n = fmt_value(native.get(key))
        m = fmt_value(mesen.get(key))
        print(f"  {key.ljust(width)}   {n:<10}  {m:<10}")


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    native_path, mesen_path = argv[1], argv[2]
    want_frame = None
    if "--frame" in argv:
        want_frame = int(argv[argv.index("--frame") + 1])

    native = load(native_path)
    mesen = load(mesen_path)
    common = sorted(set(native) & set(mesen))
    if not common:
        print("No common frames between the two traces.")
        return 1

    print(f"native frames: {len(native)}   mesen frames: {len(mesen)}   common: {len(common)}")

    if want_frame is not None:
        if want_frame not in native or want_frame not in mesen:
            print(f"frame {want_frame} missing from one trace")
            return 1
        keys = diff_keys(native[want_frame], mesen[want_frame])
        print(f"\n=== full diff at frame {want_frame} ({len(keys)} differing fields) ===")
        if keys:
            print_field_table(native[want_frame], mesen[want_frame], keys)
        else:
            print("  (identical)")
        return 0

    # First divergence of each hash region, and first overall divergence.
    first_overall = None
    first_overall_keys = None
    first_hash = {h: None for h in HASH_FIELDS}
    last_all_match = 0
    streak_broken = False

    for frame in common:
        keys = diff_keys(native[frame], mesen[frame])
        if not keys:
            if not streak_broken:
                last_all_match = frame
            continue
        streak_broken = True
        if first_overall is None:
            first_overall = frame
            first_overall_keys = keys
        for h in HASH_FIELDS:
            if first_hash[h] is None and native[frame].get(h) != mesen[frame].get(h):
                first_hash[h] = frame

    print(f"\nframes identical from 1 through: {last_all_match}")

    if first_overall is None:
        print("\n*** ALL COMMON FRAMES MATCH EXACTLY — port is frame-exact. ***")
        return 0

    scalar_keys = [k for k in first_overall_keys if k not in HASH_FIELDS]
    print(f"\nfirst divergence: frame {first_overall}")
    print(f"  differing scalar fields: {scalar_keys or '(none — hash-only divergence)'}")
    print("  first divergence per hash region:")
    for h in HASH_FIELDS:
        print(f"    {h:<16} frame {first_hash[h]}")

    print(f"\n=== field table at first divergent frame {first_overall} ===")
    print_field_table(native[first_overall], mesen[first_overall], first_overall_keys)

    # Context: show the frame just before, to confirm the last good state.
    prev = [f for f in common if f < first_overall]
    if prev:
        pf = prev[-1]
        pk = diff_keys(native[pf], mesen[pf])
        print(f"\nframe {pf} (just before): {'identical' if not pk else 'differs in ' + str(pk)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
