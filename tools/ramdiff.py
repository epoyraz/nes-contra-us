#!/usr/bin/env python3
"""Labeled diff of two 2KB NES RAM dumps (native vs Mesen), annotating each
differing byte with the nearest ram.h symbol + index, for root-causing L2 demo
divergences. Volatile/derived regions (frame counter, RNG, controller latch,
scroll phase, OAM/sprite DMA buffer) are listed separately so the logic-state
diffs stand out.

Usage: ramdiff.py A.bin B.bin [--all]
"""
import sys

# (offset, name) from contra/ram.h. Array bases are tagged with their stride/len.
SCALARS = {
    0x18: "GAME_ROUTINE", 0x19: "GAME_ROUTINE_INIT", 0x1A: "FRAME_COUNTER",
    0x1B: "NMI_CHECK", 0x1C: "DEMO_MODE", 0x1F: "DEMO_LEVEL_END_FLAG",
    0x20: "PPU_READY", 0x21: "GFX_BUF_OFFSET", 0x25: "PAUSE_STATE", 0x27: "DEMO_LEVEL",
    0x2A: "DELAY_LOW", 0x2B: "DELAY_HIGH", 0x2C: "LEVEL_ROUTINE", 0x2D: "END_LEVEL_ROUTINE",
    0x2E: "DEMO_FIRE_DELAY", 0x2F: "WEAPON_STRENGTH", 0x30: "CURRENT_LEVEL",
    0x32: "P1_LIVES", 0x34: "RANDOM_NUM", 0x35: "OAM_OFFSET", 0x36: "NUM_PAL_LOAD",
    0x37: "INDOOR_SCREEN_CLEARED", 0x38: "P1_GAME_OVER", 0x3B: "BOSS_DEFEATED",
    0x40: "LOCATION_TYPE", 0x41: "SCROLLING_TYPE", 0x48: "ALT_GFX_POS",
    0x58: "STOP_SCROLL", 0x59: "SOLID_BG_COLL", 0x5A: "DEMO_INPUT_NFRAMES",
    0x5C: "DEMO_INPUT_VAL", 0x5E: "DEMO_INPUT_TBL_IDX",
    0x60: "PPU_WR_TILE_OFF", 0x61: "LVL_TRANSITION_TIMER", 0x62: "PPU_WR_ADDR_LO",
    0x63: "PPU_WR_ADDR_HI", 0x64: "SCREEN_NUM", 0x65: "SCREEN_SCROLL_OFF",
    0x66: "ATTR_WR_LO", 0x67: "ATTR_WR_HI", 0x68: "FRAME_SCROLL", 0x69: "SUPERTILE_NT_OFF",
    0x6A: "SPRITE_LOAD_TYPE", 0x71: "ALT_GFX_LOADING", 0x72: "PAL_CYCLE",
    0x73: "INDOOR_SCROLL", 0x74: "BG_PAL_ADJ_TIMER", 0x75: "AUTO_SCROLL00",
    0x76: "AUTO_SCROLL01", 0x77: "TANK_AUTO_SCROLL", 0x79: "SOLDIER_GEN_ROUTINE",
    0x7A: "SOLDIER_GEN_TIMER", 0x82: "ENEMY_SCREEN_READ_OFF", 0x85: "BOSS_ENEMY_DESTROYED",
    0x86: "WALL_CORE_REMAINING", 0x87: "WALL_PLATING_DESTROYED", 0x88: "INDOOR_ATTACK_COUNT",
    0x89: "RED_SOLDIER_CREATED", 0x8A: "GRENADE_LAUNCHER_FLAG", 0x8E: "ENEMY_ATTACK_FLAG",
}
# zero-page two-entry (per-player) arrays at base..base+1
PLAYER2 = {
    0x90: "PLAYER_STATE", 0x92: "IND_TRANS_X_ACCUM", 0x94: "JUMP_COEFF",
    0x96: "IND_TRANS_X_FRACT_VEL", 0x98: "PLAYER_X_VEL", 0x9A: "IND_TRANS_Y_FRACT_VEL",
    0x9C: "IND_TRANS_Y_FAST_VEL", 0x9E: "ANIM_FRAME_TIMER", 0xA0: "JUMP_STATUS",
    0xA2: "PLAYER_FRAME_SCROLL", 0xA4: "EDGE_FALL_CODE", 0xA6: "ANIM_FRAME_IDX",
    0xA8: "INDOOR_ANIM_Y", 0xAA: "CURRENT_WEAPON", 0xAC: "M_WEAPON_FIRE_TIME",
    0xAE: "NEW_LIFE_INVINC", 0xB0: "INVINC_TIMER", 0xB2: "WATER_STATE",
    0xB4: "DEATH_FLAG", 0xB6: "PLAYER_ON_ENEMY", 0xB8: "FALL_X_FREEZE",
    0xBA: "PLAYER_HIDDEN", 0xBC: "SPRITE_SEQUENCE", 0xBE: "INDOOR_ANIM_X",
    0xC0: "AIM_PREV_FRAME", 0xC2: "AIM_DIR", 0xC4: "Y_FRACT_VEL", 0xC6: "Y_FAST_VEL",
    0xC8: "ELECTROCUTED_TIMER", 0xCA: "INDOOR_JUMP_FLAG", 0xCE: "RECOIL_TIMER",
    0xD0: "INDOOR_ADV_FLAG", 0xD2: "SPECIAL_SPRITE_TIMER", 0xD4: "FAST_X_VEL_BOOST",
    0xD6: "PLAYER_SPRITE_CODE", 0xD8: "PLAYER_SPRITE_FLIP", 0xDA: "BG_FLAG_EDGE",
}
SCALARS2 = {
    0xFC: "VERTICAL_SCROLL", 0xFD: "HORIZONTAL_SCROLL", 0xFE: "PPUMASK", 0xFF: "PPUCTRL",
    0xF1: "CONTROLLER", 0xF5: "CONTROLLER_DIFF", 0xF9: "CTRL_KNOWN_GOOD",
}
# 16-wide object arrays (player bullets and enemies share index space differently;
# we tag by base+index). base: name
ARR16 = {
    0x368: "PBUL_SPRITE", 0x378: "PBUL_SPR_ATTR", 0x388: "PBUL_SLOT",
    0x398: "PBUL_YVEL_ACC", 0x3A8: "PBUL_XVEL_ACC", 0x3B8: "PBUL_Y", 0x3C8: "PBUL_X",
    0x3D8: "PBUL_YVEL_FR", 0x3E8: "PBUL_XVEL_FR", 0x3F8: "PBUL_YVEL_FAST",
    0x408: "PBUL_XVEL_FAST", 0x418: "PBUL_TIMER", 0x428: "PBUL_AIM", 0x438: "PBUL_ROUTINE",
    0x448: "PBUL_OWNER", 0x458: "PBUL_FRAPID", 0x468: "PBUL_DIST", 0x478: "PBUL_FSX",
    0x488: "PBUL_FY", 0x498: "PBUL_FSX_ACC", 0x4A8: "PBUL_FY_ACC",
    0x4B8: "ENEMY_ROUTINE", 0x4C8: "ENEMY_YVEL_ACC", 0x4D8: "ENEMY_XVEL_ACC",
    0x4E8: "ENEMY_YVEL_FAST", 0x4F8: "ENEMY_YVEL_FR", 0x508: "ENEMY_XVEL_FAST",
    0x518: "ENEMY_XVEL_FR", 0x528: "ENEMY_TYPE", 0x538: "ENEMY_ANIM_DELAY",
    0x548: "ENEMY_VAR_A", 0x558: "ENEMY_ATK_DELAY/VARB", 0x568: "ENEMY_FRAME",
    0x578: "ENEMY_HP", 0x588: "ENEMY_SCORE_COLL", 0x598: "ENEMY_STATE_WIDTH",
    0x5A8: "ENEMY_ATTRS", 0x5B8: "ENEMY_VAR_1", 0x5C8: "ENEMY_VAR_2",
    0x5D8: "ENEMY_VAR_3", 0x5E8: "ENEMY_VAR_4",
}
# sprite-object position arrays: SPRITE_Y@0x31A, ENEMY_Y@0x324, SPRITE_X@0x334,
# ENEMY_X@0x33E, SPRITE_ATTR@0x34E, ENEMY_SPR_ATTR@0x358, ENEMY_SPRITES@0x30A
SPR = {0x30A: ("ENEMY_SPRITES", 16), 0x31A: ("SPRITE_Y", 10), 0x324: ("ENEMY_Y", 16),
       0x334: ("SPRITE_X", 10), 0x33E: ("ENEMY_X", 16), 0x34E: ("SPRITE_ATTR", 10),
       0x358: ("ENEMY_SPR_ATTR", 16)}

VOLATILE = {0x1A, 0x34, 0x35, 0x2E, 0x5C, 0x5E, 0x5A, 0x5B, 0x5F,
            0x60, 0x62, 0x63, 0x66, 0x67, 0x68, 0x69, 0x61,
            0xFC, 0xFD, 0xF1, 0xF2, 0xF5, 0xF6, 0xF9, 0xFA}


def label(off):
    if off in SCALARS: return SCALARS[off]
    if off in SCALARS2: return SCALARS2[off]
    for b, n in PLAYER2.items():
        if b <= off <= b + 1: return f"{n}[{off-b}]"
    for b, (n, ln) in SPR.items():
        if b <= off < b + ln: return f"{n}[{off-b}]"
    for b, n in ARR16.items():
        if b <= off < b + 16: return f"{n}[{off-b}]"
    if 0x200 <= off < 0x300: return f"OAM_DMA[{off-0x200}]"
    if 0x300 <= off < 0x30A: return f"CPU_SPR_BUF[{off-0x300}]"
    if 0x190 <= off < 0x1A0: return f"LVLEND[{off-0x190:#x}]"
    if 0x7E0 <= off <= 0x7E5: return f"SCORE[{off-0x7E0}]"
    return None


def main(argv):
    a = open(argv[1], "rb").read()
    b = open(argv[2], "rb").read()
    show_all = "--all" in argv
    n = min(len(a), len(b))
    logic, volat, render, unknown = [], [], [], []
    for off in range(n):
        if a[off] == b[off]:
            continue
        lab = label(off)
        row = f"  {off:#05x} {lab or '?':<22} A={a[off]:#04x}({a[off]:>3})  B={b[off]:#04x}({b[off]:>3})"
        if off in VOLATILE:
            volat.append(row)
        elif lab is None:
            unknown.append(row)
        elif lab.startswith(("OAM_DMA", "CPU_SPR_BUF", "SPRITE_Y", "SPRITE_X", "SPRITE_ATTR", "ENEMY_SPRITES", "ENEMY_SPR_ATTR")):
            render.append(row)
        else:
            logic.append(row)
    print(f"=== LOGIC-STATE diffs ({len(logic)}) [A={argv[1]}  B={argv[2]}] ===")
    print("\n".join(logic) or "  (none)")
    print(f"\n=== sprite/render-position diffs ({len(render)}) ===")
    print("\n".join(render) or "  (none)")
    if show_all:
        print(f"\n=== volatile ({len(volat)}) ===");  print("\n".join(volat) or "  (none)")
        print(f"\n=== unknown ({len(unknown)}) ===");  print("\n".join(unknown) or "  (none)")
    else:
        print(f"\n(volatile diffs: {len(volat)}, unknown-offset diffs: {len(unknown)} -- use --all to show)")


if __name__ == "__main__":
    main(sys.argv)
