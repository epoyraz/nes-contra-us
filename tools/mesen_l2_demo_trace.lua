-- Fast Mesen attract-demo tracer for the Level 2 frame-by-frame comparison.
--
-- Identical JSONL schema and RAM field set to tools/mesen_level1_frame_trace.lua,
-- but with the per-frame region HASHES removed (emitted as constant "00000000").
-- compare_demo_frame_trace.py excludes every *_hash field from the gameplay diff,
-- so dropping them changes nothing in the comparison -- but it removes the
-- ~270k-Lua-op-per-frame framebuffer/CHR/RAM/nametable hashing that throttled the
-- original tracer to ~3 fps, letting Mesen reach the L2 demo window (frames
-- ~2961-4622) quickly. The single-frame dump hooks (RAM/OAM/framebuffer/etc.) are
-- kept for later targeted render/state comparisons.
local output_path = os.getenv("CONTRA_MESEN_LEVEL1_FRAME_JSONL") or "mesen_l2_demo_trace.jsonl"
local output = io.open(output_path, "w")
local dump_frame = tonumber(os.getenv("CONTRA_MESEN_LEVEL1_NAMETABLE_DUMP_FRAME") or "0")
local dump_path = os.getenv("CONTRA_MESEN_LEVEL1_NAMETABLE_DUMP_PATH")
local supertile_dump_path = os.getenv("CONTRA_MESEN_LEVEL1_SUPERTILE_DUMP_PATH")
local enemy_dump_path = os.getenv("CONTRA_MESEN_LEVEL1_ENEMY_DUMP_PATH")
local framebuffer_dump_path = os.getenv("CONTRA_MESEN_LEVEL1_FRAMEBUFFER_DUMP_PATH")
local ram_dump_path = os.getenv("CONTRA_MESEN_LEVEL1_RAM_DUMP_PATH")
local oam_dump_path = os.getenv("CONTRA_MESEN_LEVEL1_OAM_DUMP_PATH")
local palette_dump_path = os.getenv("CONTRA_MESEN_LEVEL1_PALETTE_DUMP_PATH")
local chr_dump_path = os.getenv("CONTRA_MESEN_LEVEL1_CHR_DUMP_PATH")
local max_frame = tonumber(os.getenv("CONTRA_MESEN_LEVEL1_MAX_FRAME") or "4700")
-- Mirror of the native CONTRA_FORCE_DEMO_LEVEL test hook: pin every attract demo
-- to one level by writing DEMO_LEVEL ($27) whenever the game is NOT in demo-play
-- (game_routine $18 != 2), so set_next_demo_level reads the pinned value and the
-- chosen level's demo runs first, clean from boot.
local force_demo = tonumber(os.getenv("CONTRA_MESEN_FORCE_DEMO_LEVEL") or "-1")
local frame = 0

if output == nil then
    emu.log("FAIL could not open level 2 demo frame trace output " .. output_path)
    emu.stop(1)
    return
end

local RAM = emu.memType.nesInternalRam
local NAMETABLE = emu.memType.nesNametableRam
local OAM = emu.memType.nesSpriteRam
local PALETTE = emu.memType.nesPaletteRam
local PPUMEM = emu.memType.nesPpuMemory

local function read_ram(addr)
    return emu.read(addr, RAM, false)
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
    if dump_frame ~= nil and dump_frame ~= 0 and frame == dump_frame and enemy_dump_path ~= nil then
        local dump = io.open(enemy_dump_path, "w")
        if dump ~= nil then
            for slot = 0, 23 do
                local enemy_type = read_ram(0x528 + slot)
                if enemy_type ~= 0 then
                    dump:write(string.format(
                        "%u type=%02X routine=%02X delay=%02X sprite=%02X x=%u y=%u var1=%02X var2=%02X hp=%02X\n",
                        slot,
                        enemy_type,
                        read_ram(0x4B8 + slot),
                        read_ram(0x538 + slot),
                        read_ram(0x30A + slot),
                        read_ram(0x33E + slot),
                        read_ram(0x324 + slot),
                        read_ram(0x5B8 + slot),
                        read_ram(0x5C8 + slot),
                        read_ram(0x548 + slot)
                    ))
                end
            end
            dump:close()
        end
    end
    if dump_frame ~= nil and dump_frame ~= 0 and frame == dump_frame and framebuffer_dump_path ~= nil then
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
    if dump_frame ~= nil and dump_frame ~= 0 and frame == dump_frame and ram_dump_path ~= nil then
        local dump = io.open(ram_dump_path, "wb")
        if dump ~= nil then
            for offset = 0, 0x7FF do
                dump:write(string.char(read_ram(offset)))
            end
            dump:close()
        end
    end
    if dump_frame ~= nil and dump_frame ~= 0 and frame == dump_frame and oam_dump_path ~= nil then
        local dump = io.open(oam_dump_path, "wb")
        if dump ~= nil then
            for offset = 0, 0xFF do
                dump:write(string.char(emu.read(offset, OAM, false)))
            end
            dump:close()
        end
    end
    if dump_frame ~= nil and dump_frame ~= 0 and frame == dump_frame and palette_dump_path ~= nil then
        local dump = io.open(palette_dump_path, "wb")
        if dump ~= nil then
            for offset = 0, 0x1F do
                dump:write(string.char(emu.read(offset, PALETTE, false)))
            end
            dump:close()
        end
    end
    if dump_frame ~= nil and dump_frame ~= 0 and frame == dump_frame and chr_dump_path ~= nil then
        local dump = io.open(chr_dump_path, "wb")
        if dump ~= nil then
            for offset = 0, 0x1FFF do
                dump:write(string.char(emu.read(offset, PPUMEM, false)))
            end
            dump:close()
        end
    end

    -- Compact per-frame enemy digest, matching port/tools/level1_frame_trace.c
    -- byte-for-byte: one "slot:type:routine:x:y:hp:sw|" group per active slot.
    local enemies_str = ""
    for slot = 0, 23 do
        local etype = read_ram(0x528 + slot)
        if etype ~= 0 then
            enemies_str = enemies_str .. string.format(
                "%u:%02X:%02X:%u:%u:%02X:%02X|",
                slot,
                etype,
                read_ram(0x4B8 + slot),
                read_ram(0x33E + slot),
                read_ram(0x324 + slot),
                read_ram(0x578 + slot),
                read_ram(0x598 + slot))
        end
    end

    -- Player-bullet digest, matching level1_frame_trace.c: one
    -- "slot:x:y:aim:xvf:yvf:routine|" group per active bullet (sprite != 0).
    local pbul_str = ""
    for slot = 0, 15 do
        if read_ram(0x368 + slot) ~= 0 then
            pbul_str = pbul_str .. string.format(
                "%u:%u:%u:%X:%02X:%02X:%X|",
                slot,
                read_ram(0x3C8 + slot),
                read_ram(0x3B8 + slot),
                read_ram(0x428 + slot),
                read_ram(0x408 + slot),
                read_ram(0x3F8 + slot),
                read_ram(0x438 + slot))
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
        read_ram(0xBC),
        read_ram(0xBD),
        read_ram(0xD6),
        read_ram(0xD7),
        read_ram(0xAE),
        read_ram(0xAF),
        read_ram(0xB0),
        read_ram(0xB1),
        read_ram(0xB8),
        read_ram(0xB9),
        read_ram(0x32),
        read_ram(0x38),
        read_ram(0x39),
        read_ram(0x1F),
        read_ram(0x35),
        enemies_str,
        pbul_str
    ))
end

local function on_input_polled()
    if force_demo >= 0 and emu.read(0x18, RAM, false) ~= 2 then
        emu.write(0x27, force_demo, RAM)
    end
    emu.setInput({
        up = false, down = false, left = false, right = false,
        select = false, start = false, a = false, b = false,
    }, 0)
end

local function on_end_frame()
    frame = frame + 1
    emit_frame()
    if frame >= max_frame then
        output:close()
        emu.stop(0)
    end
end

emu.addEventCallback(on_input_polled, emu.eventType.inputPolled)
emu.addEventCallback(on_end_frame, emu.eventType.endFrame)
