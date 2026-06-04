local output_path = os.getenv("CONTRA_MESEN_PALETTE_PROBE_OUTPUT") or "mesen_palette_probe.txt"
local output = io.open(output_path, "w")
local color_code = 0

if output == nil then
    emu.log("FAIL could not open palette probe output " .. output_path)
    emu.stop(1)
    return
end

local PALETTE = emu.memType.nesPaletteRam

local function band(value, mask)
    return value & mask
end

local function sample_screen_color()
    local screen = emu.getScreenBuffer()
    return screen[1] or 0
end

emu.write(0x00, 0x00, PALETTE)

local function on_end_frame()
    if color_code < 64 then
        local color = sample_screen_color()

        output:write(string.format(
            "%02X %02X %02X %02X %02X\n",
            color_code,
            band(color, 0xFF),
            band(color >> 8, 0xFF),
            band(color >> 16, 0xFF),
            band(color >> 24, 0xFF)
        ))
        color_code = color_code + 1
        if color_code < 64 then
            emu.write(0x00, color_code, PALETTE)
        else
            output:close()
            emu.stop(0)
        end
    end
end

emu.addEventCallback(on_end_frame, emu.eventType.endFrame)
