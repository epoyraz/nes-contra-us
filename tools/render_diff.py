#!/usr/bin/env python3
"""Palette-independent framebuffer diff: Mesen vs the native port.

Both dumps are 256x240 little-endian u32 pixels (Mesen: the recorder's
FRAMEBUFFER dump; native: CONTRA_NATIVE_PLAY_FRAMEBUFFER_DUMP_PATH). The two
sides use different NES->RGB palettes, so raw pixel equality is meaningless.
Instead each image is relabeled into its CANONICAL form: colors are numbered
by order of first appearance in row-major scan. Two renders of the same scene
match canonically even with completely different palettes; structural bugs
(wrong tiles, missing overlays, displaced sprites) do not.

Caveat: if one side maps two distinct NES colors to the same RGB (palette
collisions, e.g. the multiple blacks) a small false-positive count appears --
judge by the diff image, not by a zero threshold.

Usage:
    render_diff.py MESEN.bin NATIVE.bin [OUT_PREFIX]

Writes OUT_PREFIX.png: a triptych (mesen | native | diff) where diff pixels
are red on the grayscale mesen image. Prints the mismatch count and the
bounding box of the largest mismatch region.
"""
import struct
import sys
import zlib

W, H = 256, 240


def load_u32(path):
    data = open(path, "rb").read()
    if len(data) != W * H * 4:
        raise SystemExit(f"{path}: expected {W*H*4} bytes, got {len(data)}")
    return list(struct.unpack(f"<{W*H}I", data))


def bijection_mismatches(mesen, native):
    """Pixels whose (mesen,native) color pair contradicts the majority color
    bijection. Robust to localized real differences (a first-occurrence
    relabeling would cascade); a palette is a bijection, so consensus pairs
    are the renderer agreeing and minority pairs are structural divergence."""
    from collections import Counter

    pairs = Counter(zip(mesen, native))
    fwd, rev = {}, {}
    for (pm, pn), count in pairs.most_common():
        if pm not in fwd and pn not in rev:
            fwd[pm] = pn
            rev[pn] = pm
    return [i for i, (pm, pn) in enumerate(zip(mesen, native))
            if fwd.get(pm) != pn]


def write_png(path, width, height, rgb_rows):
    def chunk(tag, payload):
        c = tag + payload
        return struct.pack(">I", len(payload)) + c + struct.pack(">I", zlib.crc32(c) & 0xFFFFFFFF)

    raw = b"".join(b"\x00" + row for row in rgb_rows)
    png = (b"\x89PNG\r\n\x1a\n"
           + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
           + chunk(b"IDAT", zlib.compress(raw, 6))
           + chunk(b"IEND", b""))
    open(path, "wb").write(png)


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    mesen_path, native_path = sys.argv[1], sys.argv[2]
    out_prefix = sys.argv[3] if len(sys.argv) > 3 else "render_diff"

    mesen = load_u32(mesen_path)
    native = load_u32(native_path)

    mism = bijection_mismatches(mesen, native)
    print(f"mismatched pixels: {len(mism)} / {W*H} ({100.0*len(mism)/(W*H):.2f}%)")
    if mism:
        xs = [i % W for i in mism]
        ys = [i // W for i in mism]
        print(f"mismatch bbox: x {min(xs)}..{max(xs)}  y {min(ys)}..{max(ys)}")

    # triptych: mesen | native | mesen-grayscale with mismatches in red
    def rgb_of(p):  # u32 -> (r,g,b); channel order irrelevant for viewing
        return ((p >> 16) & 0xFF, (p >> 8) & 0xFF, p & 0xFF)

    mset = set(mism)
    rows = []
    for y in range(H):
        row = bytearray()
        for x in range(W):
            row += bytes(rgb_of(mesen[y * W + x]))
        for x in range(W):
            row += bytes(rgb_of(native[y * W + x]))
        for x in range(W):
            i = y * W + x
            if i in mset:
                row += b"\xff\x00\x00"
            else:
                r, g, b = rgb_of(mesen[i])
                lum = (r * 3 + g * 6 + b) // 10
                row += bytes((lum, lum, lum))
        rows.append(bytes(row))
    write_png(out_prefix + ".png", W * 3, H, rows)
    print(f"wrote {out_prefix}.png  (mesen | native | diff)")
    return 0 if not mism else 1


sys.exit(main())
