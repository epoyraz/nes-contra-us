local output_path = os.getenv("CONTRA_MESEN_TRACE_JSONL") or "mesen_checkpoint_trace.jsonl"
local output = io.open(output_path, "w")
local debug_path = os.getenv("CONTRA_MESEN_TRACE_DEBUG")
local debug_output = nil

if output == nil then
    emu.log("FAIL could not open trace output " .. output_path)
    emu.stop(1)
    return
end

if debug_path ~= nil then
    debug_output = io.open(debug_path, "w")
end

local frame = 0
local checkpoint_index = 1
local level2_checkpoint_index = 1
local phase = "level1"
local level1_checkpoints = {
    { name = "level1-title", frame = 180 },
    { name = "level1-gameplay-start", frame = 900 },
    { name = "level1-scroll-mid", frame = 1200 },
    { name = "level1-enemy-window", frame = 1500 },
}

local RAM = emu.memType.nesInternalRam
local NAMETABLE = emu.memType.nesNametableRam
local PALETTE = emu.memType.nesPaletteRam

local function band(value, mask)
    return value & mask
end

local function bxor(left, right)
    return left ~ right
end

local function fnv_step(hash, value)
    hash = bxor(hash, band(value, 0xFF))
    return band(hash * 16777619, 0xFFFFFFFF)
end

local function hash_memory(memory_type, start_addr, length)
    local hash = 2166136261

    for offset = 0, length - 1 do
        hash = fnv_step(hash, emu.read(start_addr + offset, memory_type, false))
    end

    return hash
end

local function hash_screen()
    local hash = 2166136261
    local screen = emu.getScreenBuffer()

    for index = 1, #screen do
        local value = screen[index]

        hash = fnv_step(hash, value)
        hash = fnv_step(hash, value >> 8)
        hash = fnv_step(hash, value >> 16)
        hash = fnv_step(hash, value >> 24)
    end

    return hash
end

local function read_ram(addr)
    return emu.read(addr, RAM, false)
end

local function hex32(value)
    return string.format("%08X", band(value, 0xFFFFFFFF))
end

local function debug_log(message)
    if debug_output ~= nil then
        debug_output:write(string.format("%u %s\n", frame, message))
        debug_output:flush()
    end
end

local function capture(scenario, name, capture_frame)
    output:write(string.format(
        "{\"scenario\":\"%s\",\"name\":\"%s\",\"frame\":%u," ..
        "\"game_routine\":%u,\"level_routine\":%u,\"level\":%u," ..
        "\"location_type\":%u,\"screen\":%u,\"scroll_offset\":%u," ..
        "\"player_state\":%u,\"player_x\":%u,\"player_y\":%u," ..
        "\"lives\":%u,\"game_over\":%u," ..
        "\"demo_end\":%u,\"indoor_clear\":%u,\"wall_core_remaining\":%u," ..
        "\"ram_hash\":\"%s\",\"nametable_hash\":\"%s\"," ..
        "\"palette_hash\":\"%s\",\"enemy_hash\":\"%s\"," ..
        "\"framebuffer_hash\":\"%s\"}\n",
        scenario,
        name,
        capture_frame,
        read_ram(0x18),
        read_ram(0x2C),
        read_ram(0x30),
        read_ram(0x40),
        read_ram(0x64),
        read_ram(0x65),
        read_ram(0x90),
        read_ram(0x334),
        read_ram(0x31A),
        read_ram(0x32),
        read_ram(0x38),
        read_ram(0x1F),
        read_ram(0x37),
        read_ram(0x86),
        hex32(hash_memory(RAM, 0x0000, 0x0800)),
        hex32(hash_memory(NAMETABLE, 0x0000, 0x0800)),
        hex32(hash_memory(PALETTE, 0x0000, 0x0020)),
        hex32(2166136261),
        hex32(hash_screen())
    ))
    output:flush()
end

local function input_for_next_frame()
    return {
        up = false,
        down = false,
        left = false,
        right = false,
        select = false,
        start = false,
        a = false,
        b = false,
    }
end

local function on_input_polled()
    emu.setInput(input_for_next_frame(), 0)
end

local function on_end_frame()
    frame = frame + 1

    if phase == "level1" then
        if checkpoint_index <= #level1_checkpoints and frame == level1_checkpoints[checkpoint_index].frame then
            capture("attract_level1_demo", level1_checkpoints[checkpoint_index].name, frame)
            checkpoint_index = checkpoint_index + 1
        end

        if frame >= 1500 then
            phase = "wait_attract_level2"
            debug_log("wait for attract level 2")
        end
        return
    end

    if phase == "wait_attract_level2" then
        if read_ram(0x30) == 0x01 and read_ram(0x40) == 0x01 and read_ram(0x2C) == 0x04 then
            local should_capture = false
            local name = ""

            if level2_checkpoint_index == 1 then
                should_capture = true
                name = "level2-attract-first-room"
            elseif level2_checkpoint_index == 2 then
                should_capture = read_ram(0x86) == 0x01 and read_ram(0x90) == 0x01
                name = "level2-wall-core-loaded"
            elseif level2_checkpoint_index == 3 then
                should_capture =
                    read_ram(0x86) == 0x00 and
                    read_ram(0x37) == 0x00 and
                    read_ram(0x64) == 0x00 and
                    read_ram(0x90) == 0x01
                name = "level2-wall-core-destroyed"
            elseif level2_checkpoint_index == 4 then
                should_capture = read_ram(0x37) == 0x01 and read_ram(0x64) == 0x00
                name = "level2-room-cleared"
            elseif level2_checkpoint_index == 5 then
                should_capture = read_ram(0x64) == 0x01 and read_ram(0x86) == 0x01
                name = "level2-after-room-1"
            end

            if should_capture then
                capture("attract_level2_demo", name, frame)
                level2_checkpoint_index = level2_checkpoint_index + 1
            end

            if level2_checkpoint_index > 5 then
                phase = "wait_level2_demo_end"
                debug_log("wait for level 2 demo end")
            end
        elseif frame >= 12000 then
            output:close()
            emu.stop(1)
        end
        return
    end

    if phase == "wait_level2_demo_end" then
        if read_ram(0x18) ~= 0x02 then
            capture("attract_level2_demo", "level2-demo-finished", frame)
            output:close()
            emu.stop(0)
        elseif frame >= 12000 then
            output:close()
            emu.stop(1)
        end
        return
    end

    if frame >= 12000 then
        output:close()
        emu.stop(1)
    end
end

emu.addEventCallback(on_input_polled, emu.eventType.inputPolled)
emu.addEventCallback(on_end_frame, emu.eventType.endFrame)
