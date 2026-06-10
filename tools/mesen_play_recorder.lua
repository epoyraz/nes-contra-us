-- Interactive play recorder for native-port parity work (Mesen 2).
--
-- Load this script in Mesen while contra.nes is open, then play. It power
-- cycles the console so the recording starts at power-on frame 1 (matching
-- contra_core_init), and writes one JSONL row per frame: the RAW controller
-- input (what the hardware would read), the LATCHED controller state the game
-- consumed ($F1/$F2 after read_controller_state), and the same gameplay field
-- set as tools/mesen_l2_demo_trace.lua / port/tools/level1_frame_trace.c.
--
-- Replay the recording through the port with port/tools/play_replay_trace.c
-- and diff the two traces with tools/compare_play_trace.py.
--
-- Tip: also record a Mesen movie (Tools > Movies > Record) while playing. The
-- movie replays the exact session later, so once compare_play_trace.py names
-- the divergent frame, load this script during movie playback with
-- CONTRA_MESEN_PLAY_NO_RESET=1 and the dump hooks below to extract full
-- RAM/OAM/nametable state at that frame for tools/ramdiff.py.
--
-- Env (inherited from the shell that launched Mesen):
--   CONTRA_MESEN_PLAY_RECORDING_JSONL  output path (default contra_play_recording.jsonl)
--   CONTRA_MESEN_PLAY_MAX_FRAME        stop after N frames (default 0 = until script unload)
--   CONTRA_MESEN_PLAY_NO_RESET         "1" to skip the power cycle (movie playback re-trace)
--   CONTRA_MESEN_PLAY_REPLAY_JSONL     replay the p1_raw/p2_raw inputs of an earlier
--                                      recording instead of live input (headless
--                                      re-trace: Mesen is deterministic, so this
--                                      reproduces the session exactly -- including
--                                      DPCM controller glitches -- with the current
--                                      schema's extra fields)
--   CONTRA_MESEN_PLAY_DUMP_FRAME       with the paths below, dump state at one frame
--   CONTRA_MESEN_PLAY_RAM_DUMP_PATH / OAM / NAMETABLE / PALETTE / FRAMEBUFFER _DUMP_PATH

local output_path = os.getenv("CONTRA_MESEN_PLAY_RECORDING_JSONL") or "contra_play_recording.jsonl"
local max_frame = tonumber(os.getenv("CONTRA_MESEN_PLAY_MAX_FRAME") or "0")
local no_reset = os.getenv("CONTRA_MESEN_PLAY_NO_RESET") == "1"
local replay_path = os.getenv("CONTRA_MESEN_PLAY_REPLAY_JSONL")
local dump_frame = tonumber(os.getenv("CONTRA_MESEN_PLAY_DUMP_FRAME") or "0")
local ram_dump_path = os.getenv("CONTRA_MESEN_PLAY_RAM_DUMP_PATH")
local oam_dump_path = os.getenv("CONTRA_MESEN_PLAY_OAM_DUMP_PATH")
local nametable_dump_path = os.getenv("CONTRA_MESEN_PLAY_NAMETABLE_DUMP_PATH")
local palette_dump_path = os.getenv("CONTRA_MESEN_PLAY_PALETTE_DUMP_PATH")
local framebuffer_dump_path = os.getenv("CONTRA_MESEN_PLAY_FRAMEBUFFER_DUMP_PATH")
local output = io.open(output_path, "w")

if output == nil then
    emu.log("FAIL could not open play recording output " .. output_path)
    emu.stop(1)
    return
end

local RAM = emu.memType.nesInternalRam
local NAMETABLE = emu.memType.nesNametableRam
local OAM = emu.memType.nesSpriteRam
local PALETTE = emu.memType.nesPaletteRam

local frame = 0
local pending_raw = { 0, 0 }
local pending_rng = 0
local polls_this_frame = 0
local raw_source = "getInput"
local rng_from_nmi = false

-- Call an emu API that may not exist in every Mesen build; nil on failure.
local function try(fn, ...)
    if type(fn) == "function" then
        local ok, result = pcall(fn, ...)
        if ok then
            return result
        end
    end
    return nil
end

local function read_ram(addr)
    return emu.read(addr, RAM, false)
end

-- Contra controller bit order ($F1 layout, src/bank7.asm read_controller_state):
-- A=0x80 B=0x40 Select=0x20 Start=0x10 Up=0x08 Down=0x04 Left=0x02 Right=0x01
local function input_byte(port)
    local state = try(emu.getInput, port)

    if state == nil then
        return nil
    end

    local value = 0
    if state.a then value = value | 0x80 end
    if state.b then value = value | 0x40 end
    if state.select then value = value | 0x20 end
    if state.start then value = value | 0x10 end
    if state.up then value = value | 0x08 end
    if state.down then value = value | 0x04 end
    if state.left then value = value | 0x02 end
    if state.right then value = value | 0x01 end
    return value
end

-- Input-replay mode: feed an earlier recording's raw input stream.
local replay_p1 = nil
local replay_p2 = nil
if replay_path ~= nil then
    replay_p1 = {}
    replay_p2 = {}
    for line in io.lines(replay_path) do
        local f = string.match(line, "\"frame\":(%d+)")
        local p1 = string.match(line, "\"p1_raw\":(%d+)")
        if f ~= nil and p1 ~= nil then
            replay_p1[tonumber(f)] = tonumber(p1)
            replay_p2[tonumber(f)] = tonumber(string.match(line, "\"p2_raw\":(%d+)") or "0")
        end
    end
    if max_frame == 0 then
        max_frame = #replay_p1
    end
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

local function on_input_polled()
    if replay_p1 ~= nil then
        -- the poll belongs to the frame whose endFrame has not fired yet
        local v1 = replay_p1[frame + 1] or 0
        local v2 = replay_p2[frame + 1] or 0

        emu.setInput(buttons_from_byte(v1), 0)
        -- Mesen 2.1.1 quirk (field-tested): emu.setInput(table, 1) does NOT
        -- drive player 2 -- it CLOBBERS the port-0 input just set above. Only
        -- attempt P2 when the recording actually has P2 presses; 2-player
        -- replay needs the correct 2.1.1 port/subport binding worked out first.
        if v2 ~= 0 then
            emu.setInput(buttons_from_byte(v2), 1)
        end
        if not rng_from_nmi then
            pending_rng = read_ram(0x34)
        end
        pending_raw[1] = v1
        pending_raw[2] = v2
        polls_this_frame = polls_this_frame + 1
        return
    end

    local p1 = input_byte(0)
    local p2 = input_byte(1)

    -- fallback RNG sample (see on_nmi -- the precise sample point); only used
    -- when the nmi event hook is unavailable
    if not rng_from_nmi then
        pending_rng = read_ram(0x34)
    end

    if p1 == nil then
        -- emu.getInput unavailable: raw falls back to the latched state at end
        -- of frame (loses raw fidelity on DPCM-glitch frames only).
        raw_source = "latched"
        return
    end

    pending_raw[1] = p1
    pending_raw[2] = p2 or 0
    polls_this_frame = polls_this_frame + 1
end

local function dump_region(path, memory_type, length)
    if path == nil then
        return
    end

    local dump = io.open(path, "wb")
    if dump == nil then
        return
    end

    for offset = 0, length - 1 do
        dump:write(string.char(emu.read(offset, memory_type, false)))
    end
    dump:close()
end

local function dump_state()
    dump_region(ram_dump_path, RAM, 0x800)
    dump_region(oam_dump_path, OAM, 0x100)
    dump_region(nametable_dump_path, NAMETABLE, 0x800)
    dump_region(palette_dump_path, PALETTE, 0x20)

    if framebuffer_dump_path ~= nil then
        local dump = io.open(framebuffer_dump_path, "wb")

        if dump ~= nil then
            local screen = emu.getScreenBuffer()

            for index = 1, #screen do
                local value = screen[index]

                dump:write(string.char(value & 0xFF))
                dump:write(string.char((value >> 8) & 0xFF))
                dump:write(string.char((value >> 16) & 0xFF))
                dump:write(string.char((value >> 24) & 0xFF))
            end
            dump:close()
        end
    end
end

local function emit_frame()
    local enemies = {}
    local pbullets = {}

    if dump_frame ~= 0 and frame == dump_frame then
        dump_state()
    end

    -- Per-frame enemy / player-bullet digests; must match
    -- port/tools/level1_frame_trace.c and tools/mesen_l2_demo_trace.lua
    -- byte-for-byte.
    -- 16 slots: the ROM's enemy arrays are 16 wide; indexes 16-23 alias the
    -- next arrays (ENEMY_ANIMATION_DELAY etc.) and are pure noise
    for slot = 0, 15 do
        local enemy_type = read_ram(0x528 + slot)

        -- type 0x00 is the weapon item, so an active slot (routine set) must be
        -- listed even when its type is 0 or weapon-item divergence is invisible
        if enemy_type ~= 0 or read_ram(0x4B8 + slot) ~= 0 then
            enemies[#enemies + 1] = string.format(
                "%u:%02X:%02X:%u:%u:%02X:%02X|",
                slot,
                enemy_type,
                read_ram(0x4B8 + slot),
                read_ram(0x33E + slot),
                read_ram(0x324 + slot),
                read_ram(0x578 + slot),
                read_ram(0x598 + slot)
            )
        end
    end

    for slot = 0, 15 do
        if read_ram(0x368 + slot) ~= 0 then
            pbullets[#pbullets + 1] = string.format(
                "%u:%u:%u:%X:%02X:%02X:%X|",
                slot,
                read_ram(0x3C8 + slot),
                read_ram(0x3B8 + slot),
                read_ram(0x428 + slot),
                read_ram(0x408 + slot),
                read_ram(0x3F8 + slot),
                read_ram(0x438 + slot)
            )
        end
    end

    if raw_source == "latched" then
        pending_raw[1] = read_ram(0xF1)
        pending_raw[2] = read_ram(0xF2)
    end

    if polls_this_frame == 0 then
        pending_rng = read_ram(0x34)
    end

    output:write(string.format(
        "{\"frame\":%u,\"p1_raw\":%u,\"p2_raw\":%u,\"polls\":%u,\"rng\":%u," ..
        "\"weapon\":%u,\"p2_weapon\":%u," ..
        "\"game_routine\":%u,\"level_routine\":%u,\"level\":%u," ..
        "\"frame_counter\":%u,\"demo_mode\":%u,\"game_init\":%u," ..
        "\"delay_low\":%u,\"delay_high\":%u," ..
        "\"location_type\":%u,\"screen\":%u,\"scroll_offset\":%u," ..
        "\"horizontal_scroll\":%u,\"vertical_scroll\":%u," ..
        "\"frame_scroll\":%u,\"player_frame_scroll\":%u,\"p2_frame_scroll\":%u," ..
        "\"player_x_velocity\":%u,\"p2_x_velocity\":%u," ..
        "\"auto_scroll_00\":%u,\"auto_scroll_01\":%u,\"tank_auto_scroll\":%u," ..
        "\"ppu_tile_offset\":%u,\"ppu_addr_low\":%u,\"ppu_addr_high\":%u," ..
        "\"attr_addr_high\":%u,\"supertile_nt_offset\":%u," ..
        "\"player_state\":%u,\"p2_state\":%u,\"player_x\":%u,\"p2_x\":%u,\"player_y\":%u,\"p2_y\":%u," ..
        "\"controller\":%u,\"p2_controller\":%u,\"controller_diff\":%u,\"p2_controller_diff\":%u," ..
        "\"jump\":%u,\"p2_jump\":%u,\"edge_fall\":%u,\"p2_edge_fall\":%u," ..
        "\"y_fast\":%u,\"p2_y_fast\":%u,\"y_fract\":%u,\"p2_y_fract\":%u," ..
        "\"fall_freeze\":%u,\"p2_fall_freeze\":%u," ..
        "\"seq\":%u,\"p2_seq\":%u,\"sprite\":%u,\"p2_sprite\":%u," ..
        "\"new_life\":%u,\"p2_new_life\":%u,\"inv\":%u,\"p2_inv\":%u," ..
        "\"lives\":%u,\"game_over\":%u,\"p2_game_over\":%u,\"demo_end\":%u," ..
        "\"oam_offset\":%u,\"enemies\":\"%s\",\"pbul\":\"%s\"," ..
        "\"ram_hash\":\"00000000\",\"pattern_hash\":\"00000000\",\"nametable_hash\":\"00000000\"," ..
        "\"palette_hash\":\"00000000\",\"framebuffer_hash\":\"00000000\"}\n",
        frame,
        pending_raw[1],
        pending_raw[2],
        polls_this_frame,
        pending_rng,
        read_ram(0xAA),
        read_ram(0xAB),
        read_ram(0x18),
        read_ram(0x2C),
        read_ram(0x30),
        read_ram(0x1A),
        read_ram(0x1C),
        read_ram(0x19),
        read_ram(0x2A),
        read_ram(0x2B),
        read_ram(0x40),
        read_ram(0x64),
        read_ram(0x65),
        read_ram(0xFD),
        read_ram(0xFC),
        read_ram(0x68),
        read_ram(0xA2),
        read_ram(0xA3),
        read_ram(0x98),
        read_ram(0x99),
        read_ram(0x75),
        read_ram(0x76),
        read_ram(0x77),
        read_ram(0x60),
        read_ram(0x62),
        read_ram(0x63),
        read_ram(0x67),
        read_ram(0x69),
        read_ram(0x90),
        read_ram(0x91),
        read_ram(0x334),
        read_ram(0x335),
        read_ram(0x31A),
        read_ram(0x31B),
        read_ram(0xF1),
        read_ram(0xF2),
        read_ram(0xF5),
        read_ram(0xF6),
        read_ram(0xA0),
        read_ram(0xA1),
        read_ram(0xA4),
        read_ram(0xA5),
        read_ram(0xC6),
        read_ram(0xC7),
        read_ram(0xC4),
        read_ram(0xC5),
        read_ram(0xB8),
        read_ram(0xB9),
        read_ram(0xBC),
        read_ram(0xBD),
        read_ram(0xD6),
        read_ram(0xD7),
        read_ram(0xAE),
        read_ram(0xAF),
        read_ram(0xB0),
        read_ram(0xB1),
        read_ram(0x32),
        read_ram(0x38),
        read_ram(0x39),
        read_ram(0x1F),
        read_ram(0x35),
        table.concat(enemies),
        table.concat(pbullets)
    ))
    -- live recording flushes every frame for crash-safety; the headless
    -- input-replay re-trace flushes sparsely for speed
    if replay_path == nil or frame % 600 == 0 then
        output:flush()
    end
end

local function on_end_frame()
    frame = frame + 1
    emit_frame()
    polls_this_frame = 0

    if frame % 600 == 0 then
        try(emu.displayMessage, "recorder", string.format("recording frame %u -> %s", frame, output_path))
    end

    if max_frame > 0 and frame >= max_frame then
        output:close()
        emu.stop(0)
    end
end

-- Recording must start at POWER-ON so frame 1 equals contra_core_init + first
-- contra_core_step_frame on the native side. emu.reset() is NOT good enough:
-- it is a soft reset, and Contra warm-boots through it (RAM survives, the boot
-- skips the cold-init timeline -- field-tested: demo_mode stayed set and the
-- title appeared at frame 6 instead of ~180), which can never be replayed
-- against the cold-booted native core. So if the console is already
-- mid-session, REFUSE to record and ask for a real power cycle; Mesen's
-- System > Power Cycle restarts this script at frame 0
-- (AutoRestartScriptAfterPowerCycle), which lands in the clean branch below.
-- Skipped during movie-playback re-tracing (the movie starts from power-on).
local state = try(emu.getState)
local booted_frames = (type(state) == "table") and tonumber(state.frameCount) or nil
if not no_reset and (booted_frames == nil or booted_frames > 10) then
    emu.log("recorder: console is mid-session; use System > Power Cycle to start a power-on recording")
    output:write("{\"meta\":1,\"error\":\"console was mid-session at script load; recording requires a power cycle\"}\n")
    output:close()

    local function nag()
        frame = frame + 1
        if frame % 60 == 1 then
            try(emu.displayMessage, "recorder", "NOT RECORDING -- use System > Power Cycle to start")
        end
    end

    emu.addEventCallback(nag, emu.eventType.endFrame)
    try(emu.displayMessage, "recorder", "NOT RECORDING -- use System > Power Cycle to start")
    return
end

output:write(string.format(
    "{\"meta\":1,\"recorder\":\"mesen_play_recorder\",\"raw_source\":\"%s\"}\n",
    raw_source
))
output:flush()

-- RANDOM_NUM ($34) advances in a CPU busy loop right up to the NMI
-- (cycle-count dependent, unreproducible by the port). The inputPolled event
-- fires at Mesen's own input poll -- many busy-loop iterations BEFORE the NMI
-- -- so the exact sample point for the value the frame's logic consumes is the
-- NMI itself ($34 is stable inside the handler).
local function on_nmi()
    pending_rng = read_ram(0x34)
end

rng_from_nmi = pcall(emu.addEventCallback, on_nmi, emu.eventType.nmi)

emu.addEventCallback(on_input_polled, emu.eventType.inputPolled)
emu.addEventCallback(on_end_frame, emu.eventType.endFrame)
emu.log("play recorder writing " .. output_path)
try(emu.displayMessage, "recorder", "recording to " .. output_path)
