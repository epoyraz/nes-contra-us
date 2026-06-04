local output_path = os.getenv("CONTRA_MESEN_LEVEL1_FRAME_JSONL") or "mesen_level1_frame_trace.jsonl"
local output = io.open(output_path, "w")
local dump_frame = tonumber(os.getenv("CONTRA_MESEN_LEVEL1_NAMETABLE_DUMP_FRAME") or "0")
local dump_path = os.getenv("CONTRA_MESEN_LEVEL1_NAMETABLE_DUMP_PATH")
local supertile_dump_path = os.getenv("CONTRA_MESEN_LEVEL1_SUPERTILE_DUMP_PATH")
local frame = 0

if output == nil then
    emu.log("FAIL could not open level 1 frame trace output " .. output_path)
    emu.stop(1)
    return
end

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

local function emit_frame()
    if dump_frame ~= nil and dump_frame ~= 0 and frame == dump_frame and dump_path ~= nil then
        local dump = io.open(dump_path, "wb")

        if dump ~= nil then
            for offset = 0, 0x7FF do
                dump:write(string.char(emu.read(offset, NAMETABLE, false)))
            end
            dump:close()
        end
    end
    if dump_frame ~= nil and dump_frame ~= 0 and frame == dump_frame and supertile_dump_path ~= nil then
        local dump = io.open(supertile_dump_path, "wb")

        if dump ~= nil then
            for offset = 0, 0x7F do
                dump:write(string.char(read_ram(0x600 + offset)))
            end
            dump:close()
        end
    end

    output:write(string.format(
        "{\"frame\":%u,\"game_routine\":%u,\"level_routine\":%u,\"level\":%u," ..
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
        "\"lives\":%u,\"game_over\":%u,\"p2_game_over\":%u,\"demo_end\":%u," ..
        "\"ram_hash\":\"%s\",\"nametable_hash\":\"%s\"," ..
        "\"palette_hash\":\"%s\",\"framebuffer_hash\":\"%s\"}\n",
        frame,
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
        read_ram(0x32),
        read_ram(0x38),
        read_ram(0x39),
        read_ram(0x1F),
        hex32(hash_memory(RAM, 0x0000, 0x0800)),
        hex32(hash_memory(NAMETABLE, 0x0000, 0x0800)),
        hex32(hash_memory(PALETTE, 0x0000, 0x0020)),
        hex32(hash_screen())
    ))
end

local function on_input_polled()
    emu.setInput({
        up = false,
        down = false,
        left = false,
        right = false,
        select = false,
        start = false,
        a = false,
        b = false,
    }, 0)
end

local function on_end_frame()
    frame = frame + 1
    emit_frame()

    if frame >= 1500 then
        output:close()
        emu.stop(0)
    end
end

emu.addEventCallback(on_input_polled, emu.eventType.inputPolled)
emu.addEventCallback(on_end_frame, emu.eventType.endFrame)
