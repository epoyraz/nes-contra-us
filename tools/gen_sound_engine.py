#!/usr/bin/env python3
"""Generate port/contra_core/src/sound_engine.c from src/bank1.asm.

The bank-1 sound engine is dense, hand-tuned 6502 (cross-routine jumps,
flag-carrying exits, stack juggling). Rather than re-deriving it by hand, this
translates it instruction by instruction: every 6502 instruction becomes one
C statement with the same register/flag effects, branches and jmps become
gotos, jsr/rts use a return stack, indirect jmps dispatch on the real label
addresses, and data tables are read from the ROM image at their real CPU
addresses. The result is a literal port of the disassembly.

Usage: tools/gen_sound_engine.py [contra.dbg]   (needs a ROM built by build.sh)
"""
import re
import sys

ASM = "src/bank1.asm"
OUT = "port/contra_core/src/sound_engine.c"
FIRST = "mute_pulse_channel"
END = "dpcm_sample_data_tbl"
# exported entry points (bank7 calls these)
ENTRIES = ["handle_sound_slots", "init_sound_code_vars", "init_pulse_and_noise_channels"]
# routines outside bank 1 called with jsr: name -> C call
EXTERNAL = {"play_sound": "contra_sound_ext_play_sound(core, &a, &n, &z)"}


def load_symbols(dbg_path):
    syms = {}
    for line in open(dbg_path):
        if not line.startswith("sym"):
            continue
        m = re.search(r'name="([^"]+)"', line)
        v = re.search(r'val=0x([0-9A-Fa-f]+)', line)
        t = re.search(r'type=(\w+)', line)
        if not (m and v and t) or t.group(1) not in ("lab", "equ"):
            continue
        name = m.group(1)
        seg = re.search(r'seg=(\d+)', line)
        syms.setdefault(name, []).append((int(v.group(1), 16), seg.group(1) if seg else None))
    return syms


def main():
    dbg = sys.argv[1] if len(sys.argv) > 1 else "contra.dbg"
    syms = load_symbols(dbg)
    bank1_seg = None
    for line in open(dbg):
        if line.startswith("seg") and 'name="BANK_1"' in line:
            bank1_seg = re.search(r"id=(\d+)", line).group(1)

    def sym(name):
        cands = syms.get(name)
        if not cands:
            raise SystemExit("unknown symbol " + name)
        for val, seg in cands:  # prefer the bank-1 definition of a label
            if seg == bank1_seg:
                return val
        return cands[0][0]

    lines = open(ASM).read().split("\n")
    start = next(i for i, l in enumerate(lines) if l.startswith(FIRST + ":"))
    end = next(i for i, l in enumerate(lines) if l.startswith(END + ":"))

    # pass 1: statements with labels (local labels scoped to the last global)
    items = []  # (kind, value, src_line)
    scope = None
    for i in range(start, end):
        raw = lines[i].split(";")[0].rstrip()
        if not raw.strip():
            continue
        m = re.match(r"^(@?\w+):\s*(.*)$", raw.strip())
        if m:
            name = m.group(1)
            if name.startswith("@"):
                name = scope + "__" + name[1:]
            else:
                scope = name
            items.append(("label", name, i + 1))
            raw = m.group(2)
            if not raw.strip():
                continue
        s = raw.strip()
        if s.startswith("."):
            items.append(("data", s, i + 1))
            continue
        items.append(("insn", s, i + 1))

    def cname(label):
        return "L_" + label

    def resolve_label(op, cur_scope):
        if op.startswith("@"):
            return cur_scope + "__" + op[1:]
        return op

    labels = {name for kind, name, _ in items if kind == "label"}

    def expr(tok):
        """numeric value of an operand expression like SOUND_CODE+4 or $e4"""
        tok = tok.strip()
        total = 0
        for part in re.split(r"(?=[+-])", tok):
            part = part.strip()
            if not part:
                continue
            sign = -1 if part.startswith("-") else 1
            part = part.lstrip("+-").strip()
            if part.startswith("$"):
                v = int(part[1:], 16)
            elif part.startswith("%"):
                v = int(part[1:], 2)
            elif part.isdigit():
                v = int(part)
            elif part.startswith("<") or part.startswith(">"):
                v = expr(part[1:])
                v = (v & 0xFF) if part[0] == "<" else (v >> 8)
            else:
                v = sym(part)
            total += sign * v
        return total & 0xFFFF

    out = []
    emit = out.append
    ret_sites = []
    cur_scope = None
    pc_labels = {}  # label -> address, for indirect jumps

    def mem_operand(op):
        """returns C expression for the effective address, or None for imm/acc"""
        op = op.strip()
        m = re.match(r"^\((.+)\),\s*y$", op, re.I)
        if m:
            zp = expr(m.group(1))
            return "(uint16_t)((ram[0x%02Xu] | (ram[0x%02Xu] << 8)) + y)" % (zp, (zp + 1) & 0xFF)
        m = re.match(r"^(.+),\s*([xy])$", op, re.I)
        if m:
            base = expr(m.group(1))
            reg = m.group(2).lower()
            if base < 0x100 and reg == "x":
                return "(uint16_t)((0x%02Xu + x) & 0xFFu)" % base
            return "(uint16_t)(0x%04Xu + %s)" % (base, reg)
        return "0x%04Xu" % expr(op)

    def operand_value(op):
        op = op.strip()
        if op.startswith("#"):
            return "0x%02Xu" % (expr(op[1:]) & 0xFF)
        return "RD(%s)" % mem_operand(op)

    def setnz(reg):
        return "n = (uint8_t)(%s & 0x80u); z = (uint8_t)(%s == 0u);" % (reg, reg)

    for kind, val, srcline in items:
        if kind == "label":
            if not val.startswith(cur_scope or "\x00") or "__" not in val:
                if "__" not in val:
                    cur_scope = val
            emit("%s: /* %s */" % (cname(val), val))
            continue
        if kind == "data":
            emit("    /* %d: data %s -- read from the ROM image */" % (srcline, val.split()[0]))
            continue
        parts = val.split(None, 1)
        mn = parts[0].lower()
        op = parts[1].strip() if len(parts) > 1 else ""
        c = "/* %d: %s */ " % (srcline, val)
        if mn in ("lda", "ldx", "ldy"):
            r = mn[2]
            stmt = "%s = %s; %s" % (r, operand_value(op), setnz(r))
        elif mn in ("sta", "stx", "sty"):
            stmt = "WR(%s, %s);" % (mem_operand(op), mn[2])
        elif mn in ("tax", "tay", "txa", "tya"):
            src, dst = mn[1], mn[2]
            stmt = "%s = %s; %s" % (dst, src, setnz(dst))
        elif mn in ("inx", "iny", "dex", "dey"):
            r = mn[2]
            d = "+ 1u" if mn[0] == "i" else "- 1u"
            stmt = "%s = (uint8_t)(%s %s); %s" % (r, r, d, setnz(r))
        elif mn in ("inc", "dec"):
            d = "+ 1u" if mn == "inc" else "- 1u"
            stmt = "{ const uint16_t ea = %s; t = (uint8_t)(RD(ea) %s); WR(ea, t); %s }" % (mem_operand(op), d, setnz("t"))
        elif mn in ("asl", "lsr", "rol", "ror"):
            acc = op.lower() in ("", "a")
            if acc:
                load, store = "t = a;", "a = t;"
            else:
                load, store = "{ const uint16_t ea = %s; t = RD(ea);" % mem_operand(op), "WR(ea, t); }"
            if mn == "asl":
                body = "c = (uint8_t)(t >> 7); t = (uint8_t)(t << 1);"
            elif mn == "lsr":
                body = "c = (uint8_t)(t & 1u); t = (uint8_t)(t >> 1);"
            elif mn == "rol":
                body = "t2 = (uint8_t)(t >> 7); t = (uint8_t)((t << 1) | c); c = t2;"
            else:
                body = "t2 = (uint8_t)(t & 1u); t = (uint8_t)((t >> 1) | (c << 7)); c = t2;"
            stmt = "%s %s %s %s" % (load, body, setnz("t"), store)
        elif mn in ("and", "ora", "eor"):
            o = {"and": "&", "ora": "|", "eor": "^"}[mn]
            stmt = "a = (uint8_t)(a %s %s); %s" % (o, operand_value(op), setnz("a"))
        elif mn == "adc":
            stmt = ("{ const uint8_t m = %s; const unsigned s = (unsigned)a + m + c; "
                    "v = (uint8_t)((~(a ^ m) & (a ^ s) & 0x80u) != 0u); c = (uint8_t)(s > 0xFFu); a = (uint8_t)s; %s }"
                    % (operand_value(op), setnz("a")))
        elif mn == "sbc":
            stmt = ("{ const uint8_t m = (uint8_t)~%s; const unsigned s = (unsigned)a + m + c; "
                    "v = (uint8_t)((~(a ^ m) & (a ^ s) & 0x80u) != 0u); c = (uint8_t)(s > 0xFFu); a = (uint8_t)s; %s }"
                    % (operand_value(op), setnz("a")))
        elif mn in ("cmp", "cpx", "cpy"):
            r = {"cmp": "a", "cpx": "x", "cpy": "y"}[mn]
            stmt = ("{ const uint8_t m = %s; t = (uint8_t)(%s - m); c = (uint8_t)(%s >= m); %s }"
                    % (operand_value(op), r, r, setnz("t")))
        elif mn == "bit":
            stmt = "t = %s; z = (uint8_t)((a & t) == 0u); n = (uint8_t)(t & 0x80u); v = (uint8_t)((t & 0x40u) != 0u);" % operand_value(op)
        elif mn in ("beq", "bne", "bcc", "bcs", "bmi", "bpl", "bvc", "bvs"):
            cond = {"beq": "z", "bne": "!z", "bcs": "c", "bcc": "!c", "bmi": "n", "bpl": "!n", "bvs": "v", "bvc": "!v"}[mn]
            stmt = "if (%s) { goto %s; }" % (cond, cname(resolve_label(op, cur_scope)))
        elif mn == "jmp":
            if op.startswith("("):
                zp = expr(op.strip("()"))
                stmt = "target = (uint16_t)(RD(0x%04Xu) | (RD(0x%04Xu) << 8)); goto indirect;" % (zp, zp + 1)
            else:
                stmt = "goto %s;" % cname(resolve_label(op, cur_scope))
        elif mn == "jsr":
            target = resolve_label(op, cur_scope)
            if target in EXTERNAL:
                stmt = EXTERNAL[target] + ";"
            else:
                rid = len(ret_sites)
                ret_sites.append(rid)
                stmt = "PUSH_RET(%d); goto %s; R_%d:" % (rid, cname(target), rid)
        elif mn == "rts":
            stmt = "goto do_rts;"
        elif mn == "pha":
            stmt = "stack[sp++] = a;"
        elif mn == "pla":
            stmt = "a = stack[--sp]; %s" % setnz("a")
        elif mn == "php":
            stmt = "stack[sp++] = (uint8_t)((n ? 0x80u : 0u) | (v ? 0x40u : 0u) | (z ? 0x02u : 0u) | c);"
        elif mn == "plp":
            stmt = "t = stack[--sp]; n = (uint8_t)(t & 0x80u); v = (uint8_t)((t >> 6) & 1u); z = (uint8_t)((t >> 1) & 1u); c = (uint8_t)(t & 1u);"
        elif mn in ("clc", "sec"):
            stmt = "c = %s;" % ("0u" if mn == "clc" else "1u")
        elif mn == "nop":
            stmt = ";"
        else:
            raise SystemExit("unsupported instruction %r at line %d" % (val, srcline))
        emit("    " + c + stmt)

    # every label's CPU address (for indirect jumps and entries)
    for name in labels:
        if "__" in name:
            continue
        try:
            pc_labels[name] = sym(name)
        except SystemExit:
            pass

    hdr = [
        "/* GENERATED by tools/gen_sound_engine.py from src/bank1.asm -- do not edit.",
        " *",
        " * The Contra bank-1 sound engine, translated instruction by instruction:",
        " * each 6502 instruction is one C statement with the same register and flag",
        " * effects (source line numbers in the comments). Sound RAM lives in",
        " * core->ram exactly where the ROM keeps it; APU register writes go to the",
        " * APU model; data tables are read from the ROM image. */",
        "#include \"sound_engine.h\"",
        "",
        "#include <stdlib.h>",
        "",
        "#if defined(__GNUC__) || defined(__clang__)",
        "#pragma GCC diagnostic ignored \"-Wunused-label\"",
        "#endif",
        "",
        "#define RD(addr) contra_sound_read(core, (addr))",
        "#define WR(addr, val) contra_sound_write(core, (addr), (val))",
        "#define PUSH_RET(id) (rstack[rsp++] = (id))",
        "",
        "void contra_sound_exec(ContraCore *core, ContraSoundEntry entry, uint8_t *pa, uint8_t *px, uint8_t *py)",
        "{",
        "    uint8_t *const ram = core->ram;",
        "    uint8_t a = *pa, x = *px, y = *py;",
        "    uint8_t n = 0u, z = 0u, c = 0u, v = 0u, t = 0u, t2 = 0u;",
        "    uint8_t stack[64];",
        "    unsigned sp = 0u;",
        "    int rstack[64];",
        "    unsigned rsp = 0u;",
        "    uint16_t target = 0u;",
        "",
        "    (void)ram; (void)t2; (void)target; (void)v;",
        "    switch (entry)",
        "    {",
    ]
    for e in ENTRIES:
        hdr.append("        case CONTRA_SOUND_%s: goto %s;" % (e.upper(), cname(e)))
    hdr += ["        default: return;", "    }", ""]

    tail = ["", "do_rts:", "    if (rsp == 0u)", "    {", "        *pa = a; *px = x; *py = y;", "        return;", "    }",
            "    switch (rstack[--rsp])", "    {"]
    for rid in ret_sites:
        tail.append("        case %d: goto R_%d;" % (rid, rid))
    tail += ["        default: abort();", "    }", "", "indirect:", "    switch (target)", "    {"]
    for name, addr in sorted(pc_labels.items(), key=lambda kv: kv[1]):
        tail.append("        case 0x%04Xu: goto %s;" % (addr, cname(name)))
    tail += ["        default: abort();", "    }", "}", ""]

    with open(OUT, "w") as f:
        f.write("\n".join(hdr + out + tail))
    print("wrote %s: %d instructions, %d call sites, %d labels" % (OUT, sum(1 for k, _, _ in items if k == "insn"), len(ret_sites), len(labels)))


if __name__ == "__main__":
    main()
