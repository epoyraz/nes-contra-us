#!/usr/bin/env python3
"""Per-stage STRUCTURAL first-divergence report for play-parity traces.

Structural = everything except the cosmetic/by-construction-different fields:
sprite/seq (player pose codes; known cosmetic bursts), the region hashes,
oam/rampg/bgcol (inspection-only side data), and the raw input/rng feeds.

Rows are aligned by their "frame" field. Lag-burst rows (the real NES's
slowdown frames -- FRAME_COUNTER equal to either neighbor's) are skipped:
the recorder's snapshot of a lag frame is torn mid-iteration state.

Usage:
    structural_check.py [NATIVE.jsonl [MESEN.jsonl]]      per-stage report
    structural_check.py --frame N [NATIVE [MESEN]]        full field diff at N

The report auto-prints a per-slot diff of the first divergent enemies/pbul
field so the frontier inspection does not need a follow-up script.
"""
import json
import sys

EXCLUDE = {
    "ram_hash", "pattern_hash", "nametable_hash", "palette_hash",
    "framebuffer_hash", "oam_offset", "frame", "polls", "p1_raw", "p2_raw",
    # RANDOM_NUM is injected from the recording during replay -- never a signal
    "rng",
    # v5 inspection-only side data: rampg (per-page RAM hashes) and oam (the
    # rotating OAM shadow) never match a high-level port byte-for-byte; bgcol
    # is only emitted by the recorder until the port mirrors the ring table
    "rampg", "bgcol", "oam",
    # zp ($06-$0F) is mid-frame ROM scratch -- end-of-frame values depend on
    # which subroutine ran last; inspection-only like ram_hash
    "zp",
}

COSMETIC_PREFIXES = ("sprite", "seq")

STAGES = {
    "stage1": (594, 7441),
    "stage2": (7442, 15507),
    "stage3": (15508, 26680),
    "stage4": (26681, 39645),
}

DEFAULT_NATIVE = "tmp/reference_native_latest.jsonl"
DEFAULT_MESEN = "tmp/reference_mesen_v4.jsonl"


def load(path):
    rows, order = {}, []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            r = json.loads(line)
            if "frame" not in r:
                continue
            rows[r["frame"]] = r
            order.append(r["frame"])
    return rows, order


def slot_diff(name, m_val, v_val):
    """Print a per-slot diff for the pipe-separated digest fields."""
    def parse(s):
        out = {}
        for part in (s or "").split("|"):
            if part:
                out[part.split(":")[0]] = part
        return out

    m, v = parse(m_val), parse(v_val)
    for slot in sorted(set(m) | set(v), key=lambda s: int(s)):
        if m.get(slot) != v.get(slot):
            print(f"    {name} slot {slot}: mesen={m.get(slot)}  native={v.get(slot)}")


def main():
    args = [a for a in sys.argv[1:]]
    frame_at = None
    if args and args[0] == "--frame":
        frame_at = int(args[1])
        args = args[2:]
    native_path = args[0] if len(args) > 0 else DEFAULT_NATIVE
    mesen_path = args[1] if len(args) > 1 else DEFAULT_MESEN

    mesen, morder = load(mesen_path)
    native, _ = load(native_path)
    common = [f for f in morder if f in native]

    if frame_at is not None:
        m, v = mesen[frame_at], native[frame_at]
        # intersection: fields one schema doesn't record are not divergence
        keys = (set(m) & set(v)) - EXCLUDE
        for k in sorted(keys):
            if m.get(k) != v.get(k):
                print(f"{k:24s} mesen={m.get(k)!r}  native={v.get(k)!r}")
                if k in ("enemies", "pbul"):
                    slot_diff(k, m.get(k), v.get(k))
        return

    def lag(idx):
        fc = mesen[common[idx]].get("frame_counter")
        if idx > 0 and mesen[common[idx - 1]].get("frame_counter") == fc:
            return True
        if idx + 1 < len(common) and mesen[common[idx + 1]].get("frame_counter") == fc:
            return True
        return False

    stage_first, first_struct = {}, None
    for idx, f in enumerate(common):
        if lag(idx):
            continue
        m, v = mesen[f], native[f]
        # intersection: fields one schema doesn't record are not divergence
        keys = (set(m) & set(v)) - EXCLUDE
        structural = [k for k in sorted(keys)
                      if m.get(k) != v.get(k)
                      and not k.startswith(COSMETIC_PREFIXES)]
        if structural:
            if first_struct is None:
                first_struct = (f, structural)
            for name, (a, b) in STAGES.items():
                if a <= f <= b and name not in stage_first:
                    stage_first[name] = (f, structural[:8])

    print(f"common frames: {len(common)}")
    if first_struct:
        f, fields = first_struct
        print(f"FIRST STRUCTURAL: {f} {fields[:12]}")
        m, v = mesen[f], native[f]
        for k in fields:
            if k in ("enemies", "pbul"):
                slot_diff(k, m.get(k), v.get(k))
            else:
                print(f"    {k}: mesen={m.get(k)!r} native={v.get(k)!r}")
    else:
        print("ALL STRUCTURALLY GREEN")
    for name in STAGES:
        if name in stage_first:
            f, fields = stage_first[name]
            a, b = STAGES[name]
            pct = 100.0 * (f - a) / (b - a)
            print(f"  {name}: first structural at {f} ({pct:.1f}%) {fields}")
        else:
            print(f"  {name}: GREEN")


main()
