-- Write watchpoints on the original ROM (Mesen 2, headless).
--
-- Replays a play recording's raw inputs from power-on and logs every CPU write
-- to the watched RAM addresses between two frames: frame, scanline, dot, PC,
-- A/X/Y and the value. Answers "which ROM routine wrote this byte?" for any
-- divergence parity_telemetry reports -- look the PC up in the disassembly
-- (bank = the mapped PRG bank, also logged).
--
--   CONTRA_WATCH_REPLAY_JSONL=/abs/recording.jsonl CONTRA_WATCH_OUT=/abs/out.txt \
--   CONTRA_WATCH_ADDRS=8C-8D,83 CONTRA_WATCH_FROM=940 CONTRA_WATCH_TO=945 \
--   Mesen --testRunner --doNotSaveSettings --timeout=600 baserom.nes tools/mesen_watch.lua
--
-- (tools/watch.sh wraps this.) Addresses are hex CPU addresses ($0000-$07FF).

local replay_path = os.getenv("CONTRA_WATCH_REPLAY_JSONL")
local out_path = os.getenv("CONTRA_WATCH_OUT")
local from_frame = tonumber(os.getenv("CONTRA_WATCH_FROM") or "1")
local to_frame = tonumber(os.getenv("CONTRA_WATCH_TO") or "0")
local addr_spec = os.getenv("CONTRA_WATCH_ADDRS") or ""
local exec_spec = os.getenv("CONTRA_WATCH_EXEC") -- optional: log executions of these PCs

local out = io.open(out_path, "w")
if out == nil or replay_path == nil then
    emu.stop(1)
    return
end

local p1, p2 = {}, {}
for line in io.lines(replay_path) do
    local f = string.match(line, "\"frame\":(%d+)")
    local v = string.match(line, "\"p1_raw\":(%d+)")
    if f ~= nil and v ~= nil then
        p1[tonumber(f)] = tonumber(v)
        p2[tonumber(f)] = tonumber(string.match(line, "\"p2_raw\":(%d+)") or "0")
    end
end

local function buttons(value)
    return {
        a = (value & 0x80) ~= 0, b = (value & 0x40) ~= 0,
        select = (value & 0x20) ~= 0, start = (value & 0x10) ~= 0,
        up = (value & 0x08) ~= 0, down = (value & 0x04) ~= 0,
        left = (value & 0x02) ~= 0, right = (value & 0x01) ~= 0,
    }
end

local frame = 0

emu.addEventCallback(function()
    local v1 = p1[frame + 1] or 0
    local v2 = p2[frame + 1] or 0
    emu.setInput(buttons(v1), 0)
    if v2 ~= 0 then
        emu.setInput(buttons(v2), 1)
    end
end, emu.eventType.inputPolled)

local function log_access(kind, addr, value)
    if frame + 1 < from_frame then
        return
    end
    local st = emu.getState()
    out:write(string.format("frame %d sl %d dot %d %s $%04X=%02X pc=$%04X a=%02X x=%02X y=%02X bank=%02X\n",
        frame + 1, st["ppu.scanline"] or -1, st["ppu.cycle"] or -1, kind, addr, value or 0,
        st["cpu.pc"] or 0, st["cpu.a"] or 0, st["cpu.x"] or 0, st["cpu.y"] or 0,
        emu.read(0x8000, emu.memType.nesMemory, false)))
end

for lo, hi in string.gmatch(addr_spec, "(%x+)%-?(%x*)") do
    local a = tonumber(lo, 16)
    local b = (hi ~= "") and tonumber(hi, 16) or a
    emu.addMemoryCallback(function(addr, value) log_access("W", addr, value) end,
        emu.callbackType.write, a, b, emu.cpuType.nes, emu.memType.nesMemory)
end

if exec_spec ~= nil then
    for lo in string.gmatch(exec_spec, "(%x+)") do
        local a = tonumber(lo, 16)
        emu.addMemoryCallback(function(addr, value) log_access("X", addr, value) end,
            emu.callbackType.exec, a, a, emu.cpuType.nes, emu.memType.nesMemory)
    end
end

emu.addEventCallback(function()
    frame = frame + 1
    if frame >= to_frame then
        out:close()
        emu.stop(0)
    end
end, emu.eventType.endFrame)
