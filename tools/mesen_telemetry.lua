-- Exhaustive ground-truth telemetry capture (Mesen 2, headless).
--
-- Replays the raw controller stream of a play recording from power-on (Mesen
-- is deterministic, so this reproduces the recorded session bit-exactly) and
-- writes ONE BINARY RECORD PER FRAME holding the complete machine state the
-- picture is made of:
--
--   CPU RAM (2 KiB)            every byte, at endFrame (scanline 240, dot 0)
--   nametable RAM (2 KiB)      CIRAM, what the background is fetched from
--   palette RAM (32 B)
--   OAM (256 B)                primary sprite RAM (DMA'd at this frame's NMI)
--   CHR RAM (8 KiB)            only on frames where a pattern byte was written
--   PPU registers              ctrl/mask flags, v, t, fine x, write toggle
--   framebuffer (61,440 px)    canonical NES palette index per pixel
--   PPU write log              every $2007 write since the previous endFrame
--                              (PPU address + value, in order)
--   register write log         every CPU write to $2000-$2003/$2005/$2006/$4014 with the
--                              PPU scanline/dot it landed on
--   frame info                 NMI-sampled RANDOM_NUM, inputs, poll count
--
-- tools/telemetry_sink.py compresses the stream into a .ctel archive and
-- port/tools/parity_telemetry.c replays the same inputs through the native
-- core and diffs every layer frame by frame (RAM -> PPU state -> pixels).
--
-- Usage (via tools/capture_telemetry.sh, which sets up the FIFO + sink):
--   CONTRA_TEL_REPLAY_JSONL=/abs/recording.jsonl CONTRA_TEL_OUT=/abs/fifo \
--   Mesen --testRunner --doNotSaveSettings --timeout=1800 baserom.nes tools/mesen_telemetry.lua
--
-- Env (paths MUST be absolute -- Mesen's CWD is not the shell's):
--   CONTRA_TEL_REPLAY_JSONL   recording with per-frame p1_raw/p2_raw (required)
--   CONTRA_TEL_OUT            output path, usually a FIFO read by the sink (required)
--   CONTRA_TEL_MAX_FRAME      stop after N frames (default: recording length)
--   CONTRA_TEL_NO_FB          "1" skips the framebuffer section (faster)
--   CONTRA_TEL_NO_REGTIME     "1" skips scanline/dot sampling of register writes

local replay_path = os.getenv("CONTRA_TEL_REPLAY_JSONL")
local out_path = os.getenv("CONTRA_TEL_OUT")
local max_frame = tonumber(os.getenv("CONTRA_TEL_MAX_FRAME") or "0")
local no_fb = os.getenv("CONTRA_TEL_NO_FB") == "1"
local no_regtime = os.getenv("CONTRA_TEL_NO_REGTIME") == "1"

if replay_path == nil or out_path == nil then
    emu.log("FAIL mesen_telemetry: CONTRA_TEL_REPLAY_JSONL and CONTRA_TEL_OUT are required")
    emu.stop(1)
    return
end

local out = io.open(out_path, "wb")
if out == nil then
    emu.log("FAIL mesen_telemetry: cannot open " .. out_path)
    emu.stop(1)
    return
end
out:setvbuf("full", 1 << 20)

local RAM = emu.memType.nesInternalRam
local CIRAM = emu.memType.nesNametableRam
local PALETTE = emu.memType.nesPaletteRam
local OAM = emu.memType.nesSpriteRam
local CHR = emu.memType.nesChrRam

-- Mesen 2's default NES palette (Nes.UserPalette). Pixels are stored as the
-- CANONICAL index: the lowest palette index rendering the same RGB (all the
-- blacks -> $0D, both whites -> $20). parity_telemetry.c canonicalizes the
-- native indices with the same table, so the comparison is exact visual
-- identity independent of either side's RGB conversion.
local MESEN_PALETTE = {
    0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600, 0x561D00,
    0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000, 0x000000, 0x000000,
    0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC, 0xB71E7B, 0xB53120, 0x994E00,
    0x6B6D00, 0x388700, 0x0C9300, 0x008F32, 0x007C8D, 0x000000, 0x000000, 0x000000,
    0xFFFEFF, 0x64B0FF, 0x9290FF, 0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22,
    0xBCBE00, 0x88D800, 0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000,
    0xFFFEFF, 0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
    0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000, 0x000000,
}
local rgb_to_index = {}
for index = 63, 0, -1 do
    rgb_to_index[MESEN_PALETTE[index + 1]] = index
end
-- unknown RGB (emphasis/grayscale, a different palette) -> 0xFF, counted by the comparator
setmetatable(rgb_to_index, { __index = function() return 0xFF end })

-- input replay ---------------------------------------------------------------
local replay_p1, replay_p2 = {}, {}
local replay_last = 0
for line in io.lines(replay_path) do
    local f = string.match(line, "\"frame\":(%d+)")
    local p1 = string.match(line, "\"p1_raw\":(%d+)")
    if f ~= nil and p1 ~= nil then
        f = tonumber(f)
        replay_p1[f] = tonumber(p1)
        replay_p2[f] = tonumber(string.match(line, "\"p2_raw\":(%d+)") or "0")
        if f > replay_last then
            replay_last = f
        end
    end
end
if max_frame == 0 then
    max_frame = replay_last
end

local function buttons_from_byte(value)
    return {
        a = (value & 0x80) ~= 0,
        b = (value & 0x40) ~= 0,
        select = (value & 0x20) ~= 0,
        start = (value & 0x10) ~= 0,
        up = (value & 0x08) ~= 0,
        down = (value & 0x04) ~= 0,
        left = (value & 0x02) ~= 0,
        right = (value & 0x01) ~= 0,
    }
end

local frame = 0
local polls = 0
local fed_p1, fed_p2 = 0, 0
local nmi_rng = 0
local nmi_count = 0

local function on_input_polled()
    local v1 = replay_p1[frame + 1] or 0
    local v2 = replay_p2[frame + 1] or 0
    emu.setInput(buttons_from_byte(v1), 0)
    -- Mesen 2.1.1: setInput(table, 1) clobbers port 0; only drive P2 when used
    if v2 ~= 0 then
        emu.setInput(buttons_from_byte(v2), 1)
    end
    fed_p1, fed_p2 = v1, v2
    polls = polls + 1
end

local function on_nmi()
    -- RANDOM_NUM advances in a cycle-counted busy loop until the NMI; the value
    -- this frame's logic consumes is the one at the NMI
    nmi_rng = emu.read(0x34, RAM, false)
    nmi_count = nmi_count + 1
end

-- write logs -------------------------------------------------------------------
local ppu_log = {}      -- flat: addr, value, addr, value, ...
local reg_log = {}      -- flat: addr, value, scanline, dot, ...
local chr_dirty = true  -- first record always carries CHR

local function on_ppu_write(addr, value)
    local n = #ppu_log
    ppu_log[n + 1] = addr
    ppu_log[n + 2] = value
    if addr < 0x2000 then
        chr_dirty = true
    end
end

local function on_reg_write(addr, value)
    local scanline, dot = -2, 0
    if not no_regtime then
        local st = emu.getState()
        scanline = st["ppu.scanline"] or -2
        dot = st["ppu.cycle"] or 0
    end
    local n = #reg_log
    reg_log[n + 1] = addr
    reg_log[n + 2] = value
    reg_log[n + 3] = scanline
    reg_log[n + 4] = dot
end

-- record building -----------------------------------------------------------------
local function read_block(mem, length)
    local bytes = {}
    for offset = 0, length - 1 do
        bytes[offset + 1] = emu.read(offset, mem, false)
    end
    return string.char(table.unpack(bytes))
end

local function section(id, payload)
    return string.pack("<BI4", id, #payload) .. payload
end

local function ppu_regs_section()
    local st = emu.getState()
    local function flag(name)
        return st[name] and 1 or 0
    end
    local t = st["ppu.tmpVideoRamAddr"] or 0
    local ctrl = ((t >> 10) & 0x03)
        | (flag("ppu.control.verticalWrite") << 2)
        | (((st["ppu.control.spritePatternAddr"] or 0) ~= 0) and 0x08 or 0)
        | (((st["ppu.control.backgroundPatternAddr"] or 0) ~= 0) and 0x10 or 0)
        | (flag("ppu.control.largeSprites") << 5)
        | (flag("ppu.control.nmiOnVerticalBlank") << 7)
    local mask = flag("ppu.mask.grayscale")
        | (flag("ppu.mask.backgroundMask") << 1)
        | (flag("ppu.mask.spriteMask") << 2)
        | (flag("ppu.mask.backgroundEnabled") << 3)
        | (flag("ppu.mask.spritesEnabled") << 4)
        | (flag("ppu.mask.intensifyRed") << 5)
        | (flag("ppu.mask.intensifyGreen") << 6)
        | (flag("ppu.mask.intensifyBlue") << 7)
    return section(6, string.pack("<BBHHBBhH",
        ctrl, mask,
        st["ppu.videoRamAddr"] or 0, t,
        st["ppu.xScroll"] or 0, flag("ppu.writeToggle"),
        st["ppu.scanline"] or -2, st["ppu.cycle"] or 0))
end

local function framebuffer_section()
    local screen = emu.getScreenBuffer()
    local pixels = {}
    for index = 1, 61440 do
        pixels[index] = rgb_to_index[screen[index] & 0xFFFFFF]
    end
    local parts = {}
    for index = 1, 61440, 4096 do
        parts[#parts + 1] = string.char(table.unpack(pixels, index, index + 4095))
    end
    return section(7, table.concat(parts))
end

local function log_section(id, log, fmt, stride)
    local parts = { string.pack("<I4", #log // stride) }
    for index = 1, #log, stride do
        parts[#parts + 1] = string.pack(fmt, table.unpack(log, index, index + stride - 1))
    end
    return section(id, table.concat(parts))
end

local function on_end_frame()
    frame = frame + 1

    local sections = {
        section(10, string.pack("<BBBBBB", nmi_rng, fed_p1, fed_p2, polls & 0xFF, nmi_count & 0xFF, 0)),
        section(1, read_block(RAM, 0x800)),
        section(2, read_block(CIRAM, 0x800)),
        section(3, read_block(PALETTE, 0x20)),
        section(4, read_block(OAM, 0x100)),
        ppu_regs_section(),
        log_section(8, ppu_log, "<HB", 2),
        log_section(9, reg_log, "<HBhH", 4),
    }
    if chr_dirty then
        sections[#sections + 1] = section(5, read_block(CHR, 0x2000))
        chr_dirty = false
    end
    if not no_fb then
        sections[#sections + 1] = framebuffer_section()
    end

    local payload = string.pack("<I4H", frame, #sections) .. table.concat(sections)
    out:write("CTFR", string.pack("<I4", #payload), payload)

    ppu_log = {}
    reg_log = {}
    polls = 0
    nmi_count = 0

    if frame % 1000 == 0 then
        emu.log(string.format("telemetry frame %u", frame))
    end
    if frame >= max_frame then
        out:write("CTND", string.pack("<I4", 0))
        out:close()
        emu.stop(0)
    end
end

emu.addEventCallback(on_nmi, emu.eventType.nmi)
emu.addEventCallback(on_input_polled, emu.eventType.inputPolled)
emu.addEventCallback(on_end_frame, emu.eventType.endFrame)
emu.addMemoryCallback(on_ppu_write, emu.callbackType.write, 0x0000, 0x3FFF,
    emu.cpuType.nes, emu.memType.nesPpuMemory)
-- $2004 is left out: Mesen reports the OAM DMA as 256 $2004 writes, and OAM
-- itself is captured wholesale
emu.addMemoryCallback(on_reg_write, emu.callbackType.write, 0x2000, 0x2003,
    emu.cpuType.nes, emu.memType.nesMemory)
emu.addMemoryCallback(on_reg_write, emu.callbackType.write, 0x2005, 0x2006,
    emu.cpuType.nes, emu.memType.nesMemory)
emu.addMemoryCallback(on_reg_write, emu.callbackType.write, 0x4014, 0x4014,
    emu.cpuType.nes, emu.memType.nesMemory)
emu.log("mesen_telemetry: capturing " .. max_frame .. " frames -> " .. out_path)
