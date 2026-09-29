#!/usr/bin/env python3
"""Summarize a contra_parity_telemetry --report JSONL.

For each layer, prints the divergent frame RUNS (first..last, length, total
count) per stage, so the frontier and the shape of each problem (one-frame
blip vs. sustained drift) is visible at a glance.

Usage:
    parity_summary.py REPORT.jsonl [--layer fb] [--runs 20] [--stage stage2]
"""
import json
import sys
from collections import defaultdict


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        sys.exit(2)
    path = args[0]
    layers = None
    max_runs = 12
    only_stage = None
    i = 1
    while i < len(args):
        if args[i] == "--layer":
            layers = args[i + 1].split(",")
            i += 2
        elif args[i] == "--runs":
            max_runs = int(args[i + 1])
            i += 2
        elif args[i] == "--stage":
            only_stage = args[i + 1]
            i += 2
        else:
            i += 1

    rows = [json.loads(line) for line in open(path)]
    if layers is None:
        layers = ["game", "nt", "pal", "oam", "chr", "regs", "fb"]

    for layer in layers:
        runs = defaultdict(list)
        for r in rows:
            if r.get("torn") or not r.get(layer):
                continue
            if only_stage and r["stage"] != only_stage:
                continue
            stage_runs = runs[r["stage"]]
            if stage_runs and stage_runs[-1][1] == r["f"] - 1:
                stage_runs[-1][1] = r["f"]
                stage_runs[-1][2] += r[layer]
            else:
                stage_runs.append([r["f"], r["f"], r[layer]])
        print(f"== {layer}")
        if not runs:
            print("   IDENTICAL")
        for stage, stage_runs in runs.items():
            frames = sum(b - a + 1 for a, b, _ in stage_runs)
            total = sum(c for _, _, c in stage_runs)
            print(f"   {stage}: {frames} frames in {len(stage_runs)} runs, total {total}")
            for a, b, c in stage_runs[:max_runs]:
                print(f"      {a}..{b} ({b - a + 1}f) {c}")
            if len(stage_runs) > max_runs:
                print(f"      ... {len(stage_runs) - max_runs} more runs")


if __name__ == "__main__":
    main()
