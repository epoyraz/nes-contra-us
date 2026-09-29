#!/usr/bin/env python3
"""Contra telemetry archive (.ctel) -- sink, reader and inspector.

The ground-truth side of the parity pipeline: tools/mesen_telemetry.lua streams
one raw record per frame; `ctel.py sink` compresses the stream into a .ctel
archive that port/tools/parity_telemetry.c (and this module) read back.

Archive layout (little endian):
    "CTEL" u32 version=1
    repeated: u32 frame, u32 raw_len, u32 comp_len, zlib(record payload)

Record payload (exactly what the Lua script emits after its "CTFR" framing):
    u32 frame, u16 section_count, then sections: u8 id, u32 len, bytes
      1 CPU RAM (2048)          2 nametable RAM / CIRAM (2048)
      3 palette RAM (32)        4 OAM (256)
      5 CHR RAM (8192, only on frames where CHR was written)
      6 PPU regs: u8 ctrl, u8 mask, u16 v, u16 t, u8 fine_x, u8 w, i16 scanline, u16 dot
      7 framebuffer: 61440 canonical palette indices (0xFF = unknown color)
      8 PPU write log: u32 n, n x (u16 addr, u8 value)
      9 register write log: u32 n, n x (u16 addr, u8 value, i16 scanline, u16 dot)
     10 frame info: u8 nmi_rng, u8 p1, u8 p2, u8 polls, u8 nmi_count, u8 reserved

Usage:
    ctel.py sink IN_STREAM OUT.ctel          compress a raw stream (FIFO or file)
    ctel.py info ARCHIVE                     frame range, sizes, CHR frames
    ctel.py dump ARCHIVE FRAME [--ppu-log]   human-readable record summary
"""
import struct
import sys
import zlib

MAGIC = b"CTEL"
VERSION = 1

SEC_RAM, SEC_CIRAM, SEC_PALETTE, SEC_OAM, SEC_CHR = 1, 2, 3, 4, 5
SEC_REGS, SEC_FB, SEC_PPU_LOG, SEC_REG_LOG, SEC_INFO = 6, 7, 8, 9, 10


def sink(in_path, out_path):
    frames = 0
    raw_total = 0
    comp_total = 0
    with open(in_path, "rb") as src, open(out_path, "wb") as dst:
        dst.write(MAGIC + struct.pack("<I", VERSION))
        while True:
            header = src.read(8)
            if len(header) < 8:
                break
            tag, length = header[:4], struct.unpack("<I", header[4:])[0]
            if tag == b"CTND":
                break
            if tag != b"CTFR":
                raise SystemExit(f"sink: bad record tag {tag!r} after {frames} frames")
            payload = src.read(length)
            if len(payload) != length:
                raise SystemExit(f"sink: truncated record after {frames} frames")
            frame = struct.unpack_from("<I", payload, 0)[0]
            comp = zlib.compress(payload, 6)
            dst.write(struct.pack("<III", frame, len(payload), len(comp)))
            dst.write(comp)
            frames += 1
            raw_total += len(payload)
            comp_total += len(comp)
    print(f"sink: {frames} frames, raw {raw_total/1e6:.1f} MB -> {comp_total/1e6:.1f} MB",
          file=sys.stderr)


def parse_record(payload):
    frame, count = struct.unpack_from("<IH", payload, 0)
    pos = 6
    sections = {}
    for _ in range(count):
        sid, length = struct.unpack_from("<BI", payload, pos)
        pos += 5
        sections[sid] = payload[pos:pos + length]
        pos += length
    return frame, sections


def iter_records(path, want=None):
    """Yield (frame, sections) for every record (or only frames in `want`)."""
    with open(path, "rb") as src:
        head = src.read(8)
        if head[:4] != MAGIC:
            raise SystemExit(f"{path}: not a .ctel archive")
        while True:
            header = src.read(12)
            if len(header) < 12:
                return
            frame, raw_len, comp_len = struct.unpack("<III", header)
            if want is not None and frame not in want:
                src.seek(comp_len, 1)
                continue
            payload = zlib.decompress(src.read(comp_len))
            yield parse_record(payload)


def regs(sections):
    ctrl, mask, v, t, x, w, scanline, dot = struct.unpack("<BBHHBBhH", sections[SEC_REGS])
    return dict(ctrl=ctrl, mask=mask, v=v, t=t, x=x, w=w, scanline=scanline, dot=dot)


def ppu_log(sections):
    data = sections.get(SEC_PPU_LOG, b"\0\0\0\0")
    n = struct.unpack_from("<I", data, 0)[0]
    return [struct.unpack_from("<HB", data, 4 + 3 * i) for i in range(n)]


def reg_log(sections):
    data = sections.get(SEC_REG_LOG, b"\0\0\0\0")
    n = struct.unpack_from("<I", data, 0)[0]
    return [struct.unpack_from("<HBhH", data, 4 + 7 * i) for i in range(n)]


def info(path):
    first = last = None
    count = 0
    chr_frames = []
    for frame, sections in iter_records(path):
        if first is None:
            first = frame
        last = frame
        count += 1
        if SEC_CHR in sections:
            chr_frames.append(frame)
    print(f"{path}: {count} frames ({first}..{last}); CHR written on {len(chr_frames)} frames")
    print("  CHR frames:", " ".join(map(str, chr_frames[:60])), "..." if len(chr_frames) > 60 else "")


def dump(path, frame, show_log):
    for f, sections in iter_records(path, want={frame}):
        r = regs(sections)
        rng, p1, p2, polls, nmis, _ = struct.unpack("<BBBBBB", sections[SEC_INFO])
        print(f"frame {f}: rng={rng:02X} p1={p1:02X} p2={p2:02X} polls={polls} nmis={nmis}")
        print("  regs:", " ".join(f"{k}={v:X}" if k not in ("scanline", "dot") else f"{k}={v}"
                                  for k, v in r.items()))
        print("  sections:", sorted(sections))
        log = ppu_log(sections)
        print(f"  ppu writes: {len(log)}")
        if show_log:
            print("   ", " ".join(f"{a:04X}={v:02X}" for a, v in log))
        for a, v, sl, dot in reg_log(sections):
            print(f"  reg ${a:04X}={v:02X} @ scanline {sl} dot {dot}")
        return
    raise SystemExit(f"frame {frame} not in {path}")


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(2)
    cmd = sys.argv[1]
    if cmd == "sink":
        sink(sys.argv[2], sys.argv[3])
    elif cmd == "info":
        info(sys.argv[2])
    elif cmd == "dump":
        dump(sys.argv[2], int(sys.argv[3]), "--ppu-log" in sys.argv)
    else:
        print(__doc__)
        sys.exit(2)


if __name__ == "__main__":
    main()
