-- Drive the real ROM to its Level 2 boss room (mirrors contra_core_debug_warp_level2_boss
-- in the port): force a single-player L2 game, then per-room shoot down the wall cores
-- (enemy type 0x14) by injecting a player bullet at slot 0 and hold Up to advance, until
-- LEVEL_LOCATION_TYPE bit 7 (boss room). Then settle and dump framebuffer/nametable/
-- palette/CHR for comparison against the native port. All dump paths are absolute.
local FB   = os.getenv("CONTRA_BOSS_FB")        -- framebuffer (BGRA, 256x240)
local NT   = os.getenv("CONTRA_BOSS_NT")        -- nametable RAM (0x800)
local PAL  = os.getenv("CONTRA_BOSS_PAL")       -- palette RAM (0x20)
local CHR  = os.getenv("CONTRA_BOSS_CHR")       -- PPU pattern tables (0x2000)

local RAM = emu.memType.nesInternalRam
local NAMETABLE = emu.memType.nesNametableRam
local PALETTE = emu.memType.nesPaletteRam
local PPUMEM = emu.memType.nesPpuMemory
local function r(a) return emu.read(a, RAM, false) end
local function w(a,v) emu.write(a, v, RAM) end

-- RAM map
local GAME_ROUTINE,LEVEL_ROUTINE,CURRENT_LEVEL,PLAYER_MODE_1D = 0x18,0x2C,0x30,0x1D
local P1_GO,P2_GO,P1_LIVES,P2_LIVES = 0x38,0x39,0x32,0x33
local LOC,SCREEN,SCREEN_CLEARED,JUMP,EDGE,INVINC = 0x40,0x64,0x37,0xA0,0xA4,0xB0
local E_ROUTINE,E_TYPE,E_X,E_Y = 0x4B8,0x528,0x33E,0x324
local B_SPRITE,B_ROUTINE,B_X,B_Y = 0x368,0x438,0x3C8,0x3B8

local phase, room, initial_screen, pf, boss_frame = "title", 0, 0, 0, 0
local hold_up = false
local press_start = false
local frame = 0
local LOG = io.open((os.getenv("CONTRA_BOSS_LOG") or "boss_warp.log"), "w")
local function plog(s) if LOG ~= nil then LOG:write(s.."\n"); LOG:flush() end end

local function dump_all()
    if FB ~= nil then
        local f = io.open(FB, "wb"); local s = emu.getScreenBuffer()
        for i=1,#s do local v=s[i]
            f:write(string.char(v&0xFF, (v>>8)&0xFF, (v>>16)&0xFF, (v>>24)&0xFF)) end
        f:close()
    end
    if NT ~= nil then local f=io.open(NT,"wb"); for o=0,0x7FF do f:write(string.char(emu.read(o,NAMETABLE,false))) end; f:close() end
    if PAL ~= nil then local f=io.open(PAL,"wb"); for o=0,0x1F do f:write(string.char(emu.read(o,PALETTE,false))) end; f:close() end
    if CHR ~= nil then local f=io.open(CHR,"wb"); for o=0,0x1FFF do f:write(string.char(emu.read(o,PPUMEM,false))) end; f:close() end
    emu.log(string.format("BOSS DUMP done: loc=%02X screen=%u", r(LOC), r(SCREEN)))
end

local last_phase = ""
local function on_frame()
    frame = frame + 1
    hold_up = false
    press_start = false
    if phase ~= "title" then w(0x25, 0x00) end  -- never let a stray Start press pause us
    if phase ~= last_phase or (frame % 300 == 0) then
        plog(string.format("f=%u phase=%s room=%u gr=%02X lr=%02X loc=%02X scr=%u cleared=%02X dm=%02X dl=%02X dh=%02X e0t=%02X",
            frame, phase, room, r(GAME_ROUTINE), r(LEVEL_ROUTINE), r(LOC), r(SCREEN), r(SCREEN_CLEARED), r(0x1C), r(0x2A), r(0x2B), r(E_TYPE)))
        last_phase = phase
    end
    if phase == "title" then
        -- pulse Start to get through title + player-select into a real L1 game
        press_start = (frame % 16) < 8
        if r(GAME_ROUTINE)==0x05 and r(LEVEL_ROUTINE)>=0x04 then
            -- real game running with clean init + enemy spawner; reload as Level 2
            w(CURRENT_LEVEL,0x01); w(LEVEL_ROUTINE,0x00)
            w(0x2A,0x00); w(0x2B,0x00)  -- clear DELAY_TIME low/high (leftover L1 timer)
            pf=0; phase="wait_gameplay"
        end
        if frame > 1200 then plog("FAIL no L1 game"); dump_all(); emu.stop(1) end
    elseif phase == "wait_gameplay" then
        w(0x2B,0x00)  -- keep DELAY_TIME_HIGH clear so the stage-name delay can elapse
        pf=pf+1
        if pf>30 and r(GAME_ROUTINE)==0x05 and r(LEVEL_ROUTINE)>=0x04 and r(CURRENT_LEVEL)==0x01 then
            room=0; phase="room_start"
        end
        if frame > 2400 then plog("FAIL no L2 gameplay"); dump_all(); emu.stop(1) end
    elseif phase == "room_start" then
        if (r(LOC) & 0x80) ~= 0 then phase="boss_settle"; boss_frame=0
        elseif room >= 24 then emu.log("FAIL too many rooms"); emu.stop(1)
        else initial_screen=r(SCREEN); w(INVINC,0xFF); pf=0; phase="settle" end
    elseif phase == "settle" then
        w(INVINC,0xFF); pf=pf+1
        if (r(JUMP)==0 and r(EDGE)==0) or pf>=180 then pf=0; phase="destroy" end
    elseif phase == "destroy" then
        w(INVINC,0xFF)
        local found=false
        for i=0,15 do
            if r(E_ROUTINE+i)~=0 and r(E_TYPE+i)==0x14 then
                w(B_SPRITE,0x01); w(B_ROUTINE,0x01); w(B_X,r(E_X+i)); w(B_Y,r(E_Y+i)); found=true
            end
        end
        pf=pf+1
        if (not found and r(SCREEN_CLEARED)~=0) or pf>=600 then pf=0; phase="walkup" end
    elseif phase == "walkup" then
        w(INVINC,0xFF); hold_up=true; pf=pf+1
        if r(SCREEN)~=initial_screen or (r(LOC)&0x80)~=0 or pf>=420 then room=room+1; phase="room_start" end
    elseif phase == "boss_settle" then
        boss_frame=boss_frame+1
        if boss_frame>=120 then dump_all(); phase="done"; emu.stop(0) end
    end
end

local function on_input()
    emu.setInput({ up=hold_up, down=false, left=false, right=false,
                   select=false, start=press_start, a=false, b=false }, 0)
end

emu.addEventCallback(on_input, emu.eventType.inputPolled)
emu.addEventCallback(on_frame, emu.eventType.endFrame)
