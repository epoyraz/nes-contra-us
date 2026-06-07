#!/usr/bin/env python3
"""Isolated frame-by-frame comparison of the LEVEL 2 attract demo, native vs real
ROM (Mesen), immune to upstream (L1) demo desync.

The full-demo comparison (compare_demo_frame_trace.py) aligns by absolute frame,
so any Level 1 demo desync shifts when the L2 demo starts and makes the L2 window
incomparable. This tool instead ANCHORS each trace on its own L2-demo event and
compares in anchor-relative time, isolating "is the L2 demo itself faithful".

Anchor: the first frame with level==1, game_routine==2 (attract play) and
level_routine >= ANCHOR_ROUTINE. Default ANCHOR_ROUTINE=4 (gameplay running), so
the benign 1-frame level-load/init timing difference does not count against L2.
Use --anchor 0 to align at level load instead.

Region hashes and oam_offset are excluded (the port is a high-level reimpl, not
byte-exact). Usage:
    compare_l2_demo.py NATIVE.jsonl MESEN.jsonl [--anchor N] [--max N] [--at K]
      --anchor N : align at first L2 frame with level_routine >= N (default 4)
      --max N    : stop after N aligned frames (default: all common)
      --at K     : print the full field diff at aligned frame K and exit
"""
import json
import sys

EXCLUDE = {
    "ram_hash", "pattern_hash", "nametable_hash", "palette_hash",
    "framebuffer_hash", "oam_offset", "frame", "frame_counter",
}


def load(path):
    frames = []
    with open(path) as handle:
        for line in handle:
            line = line.strip()
            if line:
                frames.append(json.loads(line))
    frames.sort(key=lambda r: r["frame"])
    return frames


def l2_anchor(frames, anchor_routine):
    for i, r in enumerate(frames):
        if (r.get("level") == 1 and r.get("game_routine") == 2
                and r.get("level_routine", 0) >= anchor_routine):
            return i
    return None


def diff_keys(a, b):
    return [k for k in a if k in b and k not in EXCLUDE and a[k] != b[k]]


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    native = load(argv[1])
    mesen = load(argv[2])
    anchor_routine = int(argv[argv.index("--anchor") + 1]) if "--anchor" in argv else 4
    want_max = int(argv[argv.index("--max") + 1]) if "--max" in argv else None
    want_at = int(argv[argv.index("--at") + 1]) if "--at" in argv else None

    na = l2_anchor(native, anchor_routine)
    ma = l2_anchor(mesen, anchor_routine)
    if na is None or ma is None:
        print(f"FAIL: L2 anchor (level_routine>={anchor_routine}) not found "
              f"(native={na}, mesen={ma})")
        return 1
    print(f"native L2 anchor: abs frame {native[na]['frame']} (index {na})")
    print(f"mesen  L2 anchor: abs frame {mesen[ma]['frame']} (index {ma})")

    n = min(len(native) - na, len(mesen) - ma)
    if want_max is not None:
        n = min(n, want_max)

    if want_at is not None:
        a, b = native[na + want_at], mesen[ma + want_at]
        print(f"\n=== aligned frame {want_at} "
              f"(native abs {a['frame']}, mesen abs {b['frame']}) ===")
        keys = diff_keys(a, b)
        for k in keys:
            print(f"  {k:<20} native={a[k]:<8} mesen={b[k]}")
        if not keys:
            print("  (identical)")
        return 0

    first = None
    last_ok = -1
    for k in range(n):
        keys = diff_keys(native[na + k], mesen[ma + k])
        if keys:
            first = (k, keys)
            break
        last_ok = k

    print(f"\nL2 demo identical (anchor-relative) for {last_ok + 1} frames "
          f"(aligned 0..{last_ok}).")
    if first is None:
        print(f"*** L2 demo IDENTICAL across all {n} compared frames. ***")
        return 0
    k, keys = first
    a, b = native[na + k], mesen[ma + k]
    print(f"first L2 divergence at aligned frame {k} "
          f"(native abs {a['frame']}, mesen abs {b['frame']}):")
    for key in keys:
        print(f"    {key:<18} native={a[key]:<8} mesen={b[key]}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
