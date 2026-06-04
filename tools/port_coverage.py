#!/usr/bin/env python3
"""Per-bank faithful-port coverage for the Contra native port, weighted by
assembly LINE COUNT (not routine count) so a big routine counts more than a tiny
one.

The native port (port/contra_core/src/core.c) is a hand reimplementation, not a
line-by-line transpile, so there is no automatic ASM<->C mapping. Instead the
port self-reports what it faithfully ports by citing the original ASM line range
in a comment, using the convention:

    bank<N>:<from>-<to>      e.g.  bank7:7315-7352   exe_all_enemy_routine

This tool scans port/contra_core/src/core.c for those citations, takes the union
of cited line ranges per bank (so overlaps aren't double counted), and divides
by the bank's total line count to get a line-weighted coverage percentage.

A bare single-line citation (bank7:7315) is auto-expanded to the enclosing
routine's span (from its label line to the next label), so older single-line
references are still credited; prefer explicit from-to ranges for precision.

    python3 tools/port_coverage.py            # per-bank line-weighted bar chart
    python3 tools/port_coverage.py --uncited 7  # routines in bank 7 not yet cited
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PORT_C = os.path.join(ROOT, "port", "contra_core", "src", "core.c")
LABEL_RE = re.compile(r"^([a-zA-Z_][a-zA-Z0-9_]*):")
# Matches bank7:7315  or  bank7:7315-7352  or  bank2.asm:1518-1614
CITE_RE = re.compile(r"\bbank(\d+)(?:\.asm)?:(\d+)(?:-(\d+))?\b")

BANK_ROLE = {
    "bank0": "enemy routines",
    "bank1": "graphics / sprite / sound",
    "bank2": "enemy spawn data, soldier gen, level data",
    "bank3": "nametable / scroll / background",
    "bank4": "level data",
    "bank5": "misc data",
    "bank6": "audio / misc",
    "bank7": "engine core, NMI, enemy helpers, collision",
}


def bank_path(bank):
    return os.path.join(ROOT, "src", bank + ".asm")


def bank_total_lines(bank):
    with open(bank_path(bank)) as handle:
        return sum(1 for _ in handle)


def bank_label_lines(bank):
    """Sorted list of (line_number, name) for top-level labels in a bank."""
    out = []
    with open(bank_path(bank)) as handle:
        for idx, line in enumerate(handle, start=1):
            m = LABEL_RE.match(line)
            if m:
                out.append((idx, m.group(1)))
    return out


def enclosing_span(label_lines, line):
    """Span [start, end) of the routine whose label encloses `line`."""
    start = line
    end = line + 1
    for i, (lab_line, _name) in enumerate(label_lines):
        if lab_line <= line:
            start = lab_line
            end = label_lines[i + 1][0] if i + 1 < len(label_lines) else lab_line + 1
        else:
            break
    return start, end


def union_length(ranges):
    """Total length covered by a set of [start, end) ranges (union)."""
    if not ranges:
        return 0, []
    ranges = sorted(ranges)
    merged = [list(ranges[0])]
    for s, e in ranges[1:]:
        if s <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], e)
        else:
            merged.append([s, e])
    return sum(e - s for s, e in merged), merged


def collect():
    core = open(PORT_C).read()
    per_bank = {}
    for m in CITE_RE.finditer(core):
        bank = "bank" + m.group(1)
        start = int(m.group(2))
        end = int(m.group(3)) + 1 if m.group(3) else None
        per_bank.setdefault(bank, []).append((start, end))
    return per_bank


def main(argv):
    if "--uncited" in argv:
        bank = "bank" + argv[argv.index("--uncited") + 1]
        cites = collect().get(bank, [])
        label_lines = bank_label_lines(bank)
        covered = set()
        for start, end in cites:
            if end is None:
                start, end = enclosing_span(label_lines, start)
            covered.update(range(start, end))
        print(f"{bank} routines with no faithful-port citation:")
        for i, (line, name) in enumerate(label_lines):
            nxt = label_lines[i + 1][0] if i + 1 < len(label_lines) else line + 1
            if not any(l in covered for l in range(line, nxt)):
                print(f"  {line:6d}  {name}  ({nxt - line} lines)")
        return 0

    per_bank = collect()
    banks = sorted(b[:-4] for b in os.listdir(os.path.join(ROOT, "src"))
                   if re.fullmatch(r"bank\d+\.asm", b))

    print("Contra native-port FAITHFUL coverage, weighted by assembly line count")
    print("(cited ASM ranges in core.c / total bank lines; an honest lower bound)\n")
    width = 30
    tot_cov = 0
    tot_all = 0
    for bank in banks:
        total = bank_total_lines(bank)
        label_lines = bank_label_lines(bank)
        ranges = []
        for start, end in per_bank.get(bank, []):
            ranges.append(enclosing_span(label_lines, start) if end is None else (start, end))
        covered, _ = union_length(ranges)
        covered = min(covered, total)
        tot_cov += covered
        tot_all += total
        pct = 100.0 * covered / total if total else 0.0
        filled = int(round(pct / 100.0 * width))
        bar = "#" * filled + "-" * (width - filled)
        print(f"  {bank}  {bar} {pct:5.1f}%  ({covered:5d}/{total:5d} lines)  {BANK_ROLE.get(bank, '')}")

    pct = 100.0 * tot_cov / tot_all if tot_all else 0.0
    filled = int(round(pct / 100.0 * width))
    bar = "#" * filled + "-" * (width - filled)
    print(f"\n  TOTAL {bar} {pct:5.1f}%  ({tot_cov}/{tot_all} assembly lines)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
