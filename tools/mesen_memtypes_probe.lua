local output_path = os.getenv("CONTRA_MESEN_MEMTYPES_OUTPUT") or "mesen_memtypes_probe.txt"
local output = io.open(output_path, "w")

if output == nil then
    emu.stop(1)
    return
end

for key, value in pairs(emu.memType) do
    output:write(string.format("%s=%s\n", tostring(key), tostring(value)))
end

output:close()
emu.stop(0)
