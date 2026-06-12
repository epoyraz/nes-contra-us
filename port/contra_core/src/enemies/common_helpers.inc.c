/* Enemy spawn data, properties, generic helpers, bullets, and common routines.
   Included by core.c; not compiled as a separate translation unit. */

/* ---------------------------------------------------------------------------
   Faithful real-RAM enemy system.

   A direct port of the ROM's enemy spawn/dispatch onto the real ENEMY_* RAM
   arrays, so the frame-exact harness can validate enemy state against the ROM.
   This is the only enemy system: spawn -> dispatch -> per-type routines ->
   render -> collision, all on real CPU RAM.
   --------------------------------------------------------------------------- */

/* Level 1 scripted enemy data, faithful to level_1_enemy_screen_* (bank2.asm).
   Triples: xx = x position, tt = (repeat << 6) | type, yy = (ypos & 0xF0) |
   attrs; 0xFF terminates a screen. Screen 0x09's capsule has repeat=1, so it
   carries an extra yy byte. */
static const uint8_t contra_l1_enemy_screen_00[] = {
    0x10u, 0x05u, 0x60u, 0x40u, 0x05u, 0x60u, 0x50u, 0x06u, 0xC0u,
    0x60u, 0x02u, 0xA1u, 0x80u, 0x05u, 0x60u, 0xF0u, 0x03u, 0x40u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_01[] = {0x90u, 0x06u, 0xC0u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_02[] = {0x20u, 0x12u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_03[] = {0x40u, 0x12u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_04[] = {
    0x00u, 0x04u, 0xA0u, 0x10u, 0x06u, 0x60u, 0x50u, 0x06u, 0x61u,
    0x60u, 0x03u, 0x43u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_05[] = {
    0x20u, 0x06u, 0x41u, 0x40u, 0x02u, 0xA2u, 0x80u, 0x04u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_06[] = {0x40u, 0x04u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_07[] = {
    0x20u, 0x07u, 0xA0u, 0xA0u, 0x07u, 0x41u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_08[] = {
    0x00u, 0x02u, 0xC3u, 0x50u, 0x06u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_09[] = {
    0x10u, 0x43u, 0x40u, 0xB4u, 0xE0u, 0x07u, 0x81u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_0a[] = {0xC0u, 0x04u, 0xC0u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_0b[] = {
    0x40u, 0x04u, 0xC3u, 0xA8u, 0x10u, 0x81u, 0xB1u, 0x11u, 0xB0u,
    0xB4u, 0x06u, 0x52u, 0xC0u, 0x10u, 0x80u, 0xFFu};
static const uint8_t contra_l1_enemy_screen_0c[] = {0xFFu};

static const uint8_t *const contra_l1_enemy_screen_tbl[] = {
    contra_l1_enemy_screen_00, contra_l1_enemy_screen_01, contra_l1_enemy_screen_02,
    contra_l1_enemy_screen_03, contra_l1_enemy_screen_04, contra_l1_enemy_screen_05,
    contra_l1_enemy_screen_06, contra_l1_enemy_screen_07, contra_l1_enemy_screen_08,
    contra_l1_enemy_screen_09, contra_l1_enemy_screen_0a, contra_l1_enemy_screen_0b,
    contra_l1_enemy_screen_0c};

/* enemy_prop_00 (bank7.asm): 4 bytes/type — STATE_WIDTH, SCORE_COLLISION, HP,
   VAR_A — indexed by ENEMY_TYPE. Level 1 (and shared types < 0x10) use this. */
static const uint8_t contra_enemy_prop_00[][4] = {
    {0x82u, 0x22u, 0x01u, 0x00u}, {0x80u, 0x00u, 0x01u, 0x00u},
    {0x0Fu, 0x32u, 0xF0u, 0x00u}, {0x0Bu, 0x32u, 0x01u, 0x00u},
    {0x8Fu, 0x22u, 0x08u, 0x00u}, {0x83u, 0x10u, 0x01u, 0x00u},
    {0x83u, 0x30u, 0x01u, 0x00u}, {0x8Fu, 0x30u, 0x08u, 0x00u},
    {0x0Fu, 0x52u, 0xF1u, 0x00u}, {0x00u, 0x00u, 0x01u, 0x00u},
    {0x0Fu, 0x42u, 0xF0u, 0x00u}, {0x8Au, 0x05u, 0x01u, 0x00u},
    {0x83u, 0x42u, 0x01u, 0x00u}, {0x00u, 0x00u, 0x01u, 0x00u},
    {0x0Eu, 0x33u, 0x0Au, 0x00u}, {0x80u, 0x01u, 0x01u, 0x00u},
    {0x0Fu, 0x42u, 0x10u, 0x00u}, {0x0Cu, 0x82u, 0x20u, 0x00u},
    {0x89u, 0x00u, 0x01u, 0x00u}};

/* enemy_prop_01/02 level-2/4 entries (bank7:9196), indexed by type-0x10:
   {ENEMY_STATE_WIDTH, ENEMY_SCORE_COLLISION, ENEMY_HP, ENEMY_VAR_A}. The ROM
   selects this via enemy_prop_ptr_tbl[CURRENT_LEVEL] for types >= 0x10, so the
   indoor types get their own init (e.g. the wall turret's HP, the soldiers'
   collision box) instead of the level-1 table. */
static const uint8_t contra_enemy_prop_level2[][4] = {
    {0x8Du, 0x02u, 0x01u, 0x00u}, /* 0x10 boss eye */
    {0x2Fu, 0x22u, 0x05u, 0x00u}, /* 0x11 rollers */
    {0x81u, 0x03u, 0x01u, 0x00u}, /* 0x12 grenades */
    {0x9Fu, 0x35u, 0x04u, 0x00u}, /* 0x13 wall turret (wall cannon) */
    {0x9Fu, 0x05u, 0x01u, 0x00u}, /* 0x14 wall core */
    {0x13u, 0x16u, 0x01u, 0x00u}, /* 0x15 running indoor soldier */
    {0x13u, 0x16u, 0x01u, 0x00u}, /* 0x16 jumping indoor soldier */
    {0x13u, 0x36u, 0x01u, 0x00u}, /* 0x17 seeking guy (grenade launcher) */
    {0x13u, 0x16u, 0x01u, 0x00u}, /* 0x18 group of 4 */
    {0x89u, 0x00u, 0xF1u, 0x00u}, /* 0x19 indoor soldier generator */
    {0x81u, 0x00u, 0xF1u, 0x00u}, /* 0x1A roller generator */
    {0x8Fu, 0x13u, 0x02u, 0x01u}, /* 0x1B boss eye sphere projectile */
    {0x8Fu, 0x02u, 0x01u, 0x00u}, /* 0x1C boss gemini */
    {0x0Au, 0x15u, 0x01u, 0x00u}, /* 0x1D boss gemini spinning bubbles */
    {0x03u, 0x30u, 0x01u, 0x00u}, /* 0x1E blue jumping guy */
    {0x03u, 0x30u, 0x01u, 0x00u}, /* 0x1F red shooting guy */
    {0x81u, 0x00u, 0xF1u, 0x00u}, /* 0x20 red/blue guys generator */
};

/* enemy_prop level-3 entries (bank7:9221), indexed by type-0x10. Note the floating
   rock platform's STATE_WIDTH #$c0 -- bit 6 set marks it "landable", which is what
   lets the player ride it instead of dying. */
static const uint8_t contra_enemy_prop_level3[6][4] = {
    {0xC0u, 0x04u, 0xF0u, 0x00u}, /* 0x10 floating rock platform */
    {0x80u, 0x02u, 0xF0u, 0x00u}, /* 0x11 moving flame */
    {0x81u, 0x00u, 0xF0u, 0x00u}, /* 0x12 rock cave (falling-rock generator) */
    {0x8Fu, 0x31u, 0x05u, 0x00u}, /* 0x13 falling rock */
    {0x8Du, 0x83u, 0xF1u, 0x02u}, /* 0x14 boss mouth */
    {0x0Eu, 0x52u, 0xF1u, 0x00u}, /* 0x15 dragon arm orb */
};

/* enemy_prop level-5 entries (bank7:9230), indexed by type-0x10. */
static const uint8_t contra_enemy_prop_level5[7][4] = {
    {0x81u, 0x00u, 0xF0u, 0x00u}, /* 0x10 ice grenade generator */
    {0x81u, 0x02u, 0xF1u, 0x00u}, /* 0x11 ice grenade */
    {0x85u, 0x79u, 0xF0u, 0x00u}, /* 0x12 tank */
    {0x81u, 0x00u, 0xF0u, 0x00u}, /* 0x13 pipe joint */
    {0x8Du, 0x93u, 0x20u, 0x00u}, /* 0x14 alien carrier */
    {0x02u, 0x20u, 0x01u, 0x00u}, /* 0x15 flying saucer */
    {0x0Au, 0x12u, 0x01u, 0x00u}, /* 0x16 drop bomb */
};

/* enemy_prop level-6 entries (bank7:9241), indexed by type-0x10. */
static const uint8_t contra_enemy_prop_level6[5][4] = {
    {0x81u, 0x0Fu, 0xF0u, 0x00u}, /* 0x10 fire beam down */
    {0x81u, 0x0Fu, 0xF0u, 0x00u}, /* 0x11 fire beam left */
    {0x81u, 0x0Fu, 0xF0u, 0x00u}, /* 0x12 fire beam right */
    {0x04u, 0x9Du, 0x01u, 0x02u}, /* 0x13 boss robot */
    {0x80u, 0x05u, 0x01u, 0x00u}, /* 0x14 spiked disk projectile */
};

/* enemy_prop level-7 entries (bank7:9250), indexed by type-0x10. */
static const uint8_t contra_enemy_prop_level7[9][4] = {
    {0x80u, 0x0Au, 0xF0u, 0x00u}, /* 0x10 mechanical claw */
    {0x8Du, 0x0Fu, 0x10u, 0x00u}, /* 0x11 rising spiked wall */
    {0x0Cu, 0x0Fu, 0x10u, 0x00u}, /* 0x12 spiked wall */
    {0x81u, 0x00u, 0xF0u, 0x00u}, /* 0x13 cart generator */
    {0x6Eu, 0x0Cu, 0x03u, 0x00u}, /* 0x14 moving cart */
    {0x6Eu, 0x0Cu, 0x03u, 0x00u}, /* 0x15 immobile cart */
    {0x0Cu, 0x93u, 0x20u, 0x00u}, /* 0x16 armored door */
    {0x8Fu, 0x72u, 0x08u, 0x00u}, /* 0x17 mortar launcher */
    {0x89u, 0x00u, 0x01u, 0x00u}, /* 0x18 boss soldier generator */
};

/* enemy_prop level-8 entries (bank7:9271), indexed by type-0x10. */
static const uint8_t contra_enemy_prop_level8[7][4] = {
    {0x04u, 0x78u, 0x01u, 0x02u}, /* 0x10 alien guardian */
    {0x06u, 0x22u, 0x01u, 0x01u}, /* 0x11 alien fetus */
    {0x06u, 0x42u, 0x01u, 0x01u}, /* 0x12 alien mouth */
    {0x02u, 0x22u, 0x01u, 0x00u}, /* 0x13 white blob */
    {0x06u, 0x33u, 0x01u, 0x01u}, /* 0x14 alien spider */
    {0x06u, 0x62u, 0x10u, 0x01u}, /* 0x15 spider spawn */
    {0x04u, 0xA7u, 0x01u, 0x03u}, /* 0x16 heart */
};

/* find_next_enemy_slot (bank7.asm:9024): first free slot scanning 15->0, or -1. */
static int contra_rom_find_next_enemy_slot(const ContraCore *core)
{
    int slot;

    for (slot = 0x0F; slot >= 0; --slot)
    {
        if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + (unsigned)slot] == 0u)
        {
            return slot;
        }
    }
    return -1;
}

/* find_bullet_slot (bank7.asm:9037): first slot whose ENEMY_TYPE == 1, else 0. */
static uint8_t contra_rom_find_bullet_slot(const ContraCore *core)
{
    int slot;

    for (slot = 0x0F; slot >= 0; --slot)
    {
        if (core->ram[CONTRA_RAM_ENEMY_TYPE + (unsigned)slot] == 0x01u)
        {
            return (uint8_t)slot;
        }
    }
    return 0u;
}

/* clear_enemy_pt_2..pt_4 (bank7.asm:9077): zero per-slot vars. ENEMY_TYPE/HP and
   ROUTINE/SPRITES are handled by the caller (initialize_enemy). */
static void contra_rom_clear_enemy_pt_2(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_POS + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] = 0u;
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_A + x] = 0u;
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0u;
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = 0u;
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0u;
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = 0u;
}

/* initialize_enemy (bank7.asm:9109): set routine=1, sprite=1, clear vars, then
   load props (width/score/HP/var_a) from enemy_prop_00 by ENEMY_TYPE. Level 1
   uses enemy_prop_00 for both shared (<0x10) and level-specific types. */
static void contra_rom_initialize_enemy(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t type = ram[CONTRA_RAM_ENEMY_TYPE + x];

    ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x01u;
    core->l2_structure_tile[x] = 0u; /* no wall-structure tile drawn yet */
    core->l2_supertile[x] = 0xFFu;   /* no boss-room super-tile drawn yet */
    core->l1_supertile[x] = 0xFFu;   /* no L1 enemy super-tile drawn yet */
    contra_rom_clear_enemy_pt_2(core, x);

    if ((type == 0x12u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x00u))
    {
        core->l1_bridge_gap_count = 0u; /* a fresh bridge -> drop stale collision gaps */
    }

    /* enemy_prop_ptr_tbl (bank7:9152): shared types (< 0x10) use the common
       table; level-specific types (>= 0x10) use the per-level table. */
    if ((type >= 0x10u) &&
        ((ram[CONTRA_RAM_CURRENT_LEVEL] == 0x01u) || (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x03u)))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level2) / sizeof(contra_enemy_prop_level2[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level2[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level2[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level2[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level2[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x02u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level3) / sizeof(contra_enemy_prop_level3[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level3[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level3[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level3[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level3[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x04u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level5) / sizeof(contra_enemy_prop_level5[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level5[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level5[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level5[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level5[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x05u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level6) / sizeof(contra_enemy_prop_level6[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level6[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level6[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level6[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level6[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x06u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level7) / sizeof(contra_enemy_prop_level7[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level7[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level7[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level7[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level7[i][3];
        }
    }
    else if ((type >= 0x10u) && (ram[CONTRA_RAM_CURRENT_LEVEL] == 0x07u))
    {
        const size_t i = (size_t)(type - 0x10u);

        if (i < (sizeof(contra_enemy_prop_level8) / sizeof(contra_enemy_prop_level8[0])))
        {
            ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_level8[i][0];
            ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_level8[i][1];
            ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_level8[i][2];
            ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_level8[i][3];
        }
    }
    else if (type < (sizeof(contra_enemy_prop_00) / sizeof(contra_enemy_prop_00[0])))
    {
        ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = contra_enemy_prop_00[type][0];
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = contra_enemy_prop_00[type][1];
        ram[CONTRA_RAM_ENEMY_HP + x] = contra_enemy_prop_00[type][2];
        ram[CONTRA_RAM_ENEMY_VAR_A + x] = contra_enemy_prop_00[type][3];
    }
}

/* load_screen_enemy_data (bank2.asm:1518), outdoor/horizontal level-1 path:
   spawn scripted enemies as the screen scrolls to their x position. */
/* level_enemy_screen_ptr_ptr_tbl (bank2:1688-1696): per-level CPU address (bank 2)
   of that level's per-screen enemy-data pointer table. Addresses are taken from the
   table's own annotations in bank2.asm. */
static const uint16_t contra_level_enemy_screen_ptr_tbl_addr[8] = {
    0xB82Bu, 0xB8AAu, 0xB90Du, 0xB9AFu, 0xBA48u, 0xBB24u, 0xBBB7u, 0xBCA9u
};

/* Read one byte of the current screen's enemy-data list. Level 1 keeps using the
   verified hardcoded extraction; every other level reads the original bytes from
   bank 2 directly via the pointer chain. */
static uint8_t contra_screen_enemy_byte(const uint8_t *l1data, uint16_t rom_data, uint8_t offset)
{
    return (l1data != NULL)
        ? l1data[offset]
        : contra_rom_read_u8(2u, (uint16_t)(rom_data + offset));
}

/* load_screen_enemy_data (bank2:1518-1614): spawn this screen's scripted enemies as
   the camera reaches each enemy's trigger position. Horizontal levels trigger on
   LEVEL_SCREEN_SCROLL_OFFSET as an x position and place the enemy at the right edge;
   vertical levels (Level 3) trigger on the same offset as the climb distance and
   place the enemy by column (high nibble) with Y set to how far past the trigger the
   camera already is (bank2:1589-1596). The indoor branch is handled separately. */
static void contra_rom_load_screen_enemy_data(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    const uint8_t level = ram[CONTRA_RAM_CURRENT_LEVEL];
    const uint8_t screen = ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER];
    const bool vertical = (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u);
    const uint8_t *l1data = NULL;
    uint16_t rom_data = 0u;
    uint8_t y;
    uint8_t x_raw;
    uint8_t trigger;
    uint8_t scroll;
    uint8_t distance;
    uint8_t type;
    uint8_t repeat;

    if (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u)
    {
        return; /* indoor/base levels use load_enemy_indoor_level */
    }
    if (level >= 8u)
    {
        return;
    }

    if (level == 0u)
    {
        if (screen >= (sizeof(contra_l1_enemy_screen_tbl) / sizeof(contra_l1_enemy_screen_tbl[0])))
        {
            return;
        }
        l1data = contra_l1_enemy_screen_tbl[screen];
    }
    else
    {
        const uint16_t ptr_tbl = contra_level_enemy_screen_ptr_tbl_addr[level];
        rom_data = (uint16_t)(
            (uint16_t)contra_rom_read_u8(2u, (uint16_t)(ptr_tbl + (uint16_t)(screen * 2u))) |
            ((uint16_t)contra_rom_read_u8(2u, (uint16_t)(ptr_tbl + (uint16_t)(screen * 2u) + 1u)) << 8u));
    }

    y = ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET];
    x_raw = contra_screen_enemy_byte(l1data, rom_data, y);
    if (x_raw == 0xFFu)
    {
        return; /* end of screen data */
    }

    trigger = (uint8_t)(x_raw & 0xFEu);
    scroll = ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    if (trigger == scroll)
    {
        distance = 0u;
    }
    else if (trigger > scroll)
    {
        return; /* camera hasn't reached this enemy's trigger yet */
    }
    else
    {
        distance = (uint8_t)((uint8_t)((uint8_t)(trigger - scroll) ^ 0xFFu) + 1u);
    }

    ++y;
    type = (uint8_t)(contra_screen_enemy_byte(l1data, rom_data, y) & 0x3Fu);
    repeat = (uint8_t)((contra_screen_enemy_byte(l1data, rom_data, y) >> 6) & 0x03u);

    for (;;)
    {
        const uint8_t byte3 = contra_screen_enemy_byte(l1data, rom_data, (uint8_t)(y + 1u));
        int slot = contra_rom_find_next_enemy_slot(core);

        ++y;
        if (slot < 0)
        {
            /* no free slot: ROM only steals a bullet slot if the enemy x's low
               bit is set; otherwise it skips placement for this repeat. */
            if ((x_raw & 0x01u) != 0u)
            {
                slot = (int)contra_rom_find_bullet_slot(core);
            }
        }
        if (slot >= 0)
        {
            const uint8_t sx = (uint8_t)slot;

            ram[CONTRA_RAM_ENEMY_TYPE + sx] = type;
            contra_rom_initialize_enemy(core, sx);
            ram[CONTRA_RAM_ENEMY_ATTRIBUTES + sx] = (uint8_t)(byte3 & 0x0Fu);
            if (vertical)
            {
                ram[CONTRA_RAM_ENEMY_X_POS + sx] = (uint8_t)(byte3 & 0xF0u);
                ram[CONTRA_RAM_ENEMY_Y_POS + sx] = distance;
            }
            else
            {
                ram[CONTRA_RAM_ENEMY_Y_POS + sx] = (uint8_t)(byte3 & 0xF0u);
                ram[CONTRA_RAM_ENEMY_X_POS + sx] = (uint8_t)(0xF0u - distance);
            }
        }

        if (repeat == 0u)
        {
            break;
        }
        --repeat;
    }

    ++y;
    ram[CONTRA_RAM_ENEMY_SCREEN_READ_OFFSET] = y;
}

/* --- shared enemy helpers (bank7.asm), real-RAM ports --- */

/* clear_enemy (bank7.asm:9070): zero a slot's state, freeing it. */
static void contra_rom_clear_enemy(ContraCore *core, uint8_t x)
{
    if (core->ram[CONTRA_RAM_ENEMY_TYPE + x] == 0x17u)
    {
        core->ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0u; /* launcher gone -> resume generation */
    }
    core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_HP + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_TYPE + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0u;
    core->l2_structure_tile[x] = 0u;
    core->l2_supertile[x] = 0xFFu;
    core->l1_supertile[x] = 0xFFu;
    contra_rom_clear_enemy_pt_2(core, x);
}

static void contra_rom_remove_enemy(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0u;
}

/* remove_enemy for the scroll-off-screen paths: the ROM keeps ENEMY_TYPE (and
   hp/state) in the husk -- only routine + sprite are cleared -- but the
   native-side render caches must be dropped so the husk can't redraw overlays. */
static void contra_rom_remove_enemy_offscreen(ContraCore *core, uint8_t x)
{
    core->l2_structure_tile[x] = 0u;
    core->l2_supertile[x] = 0xFFu;
    core->l1_supertile[x] = 0xFFu;
    contra_rom_remove_enemy(core, x);
}

static void contra_rom_add_4_to_enemy_y_pos(ContraCore *core, uint8_t x);

/* add_a_to_enemy_y_pos / add_a_to_enemy_x_pos (bank7.asm:8387/8394). */
static void contra_rom_add_a_to_enemy_y_pos(ContraCore *core, uint8_t x, uint8_t a)
{
    core->ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_POS + x] + a);
}

static void contra_rom_add_a_to_enemy_x_pos(ContraCore *core, uint8_t x, uint8_t a)
{
    core->ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_X_POS + x] + a);
}

/* add_scroll_to_enemy_pos (bank7.asm:7824), horizontal: X -= FRAME_SCROLL,
   remove the enemy if it scrolls off the left edge (X < 0x08). */
static void contra_rom_add_scroll_to_enemy_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        /* vertical level (bank7:7827-7832): the enemy is anchored to the terrain, so
           it scrolls DOWN with it; remove it once it passes off the bottom (>= #$e8). */
        const uint8_t new_y = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + ram[CONTRA_RAM_FRAME_SCROLL]);

        ram[CONTRA_RAM_ENEMY_Y_POS + x] = new_y;
        if (new_y >= 0xE8u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* remove_enemy_far keeps type */
        }
        return;
    }

    {
        const uint8_t new_x = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - ram[CONTRA_RAM_FRAME_SCROLL]);

        ram[CONTRA_RAM_ENEMY_X_POS + x] = new_x;
        if (new_x < 0x08u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* remove_enemy keeps type */
        }
    }
}

/* advance_enemy_routine (bank7.asm:7591): ++ENEMY_ROUTINE if non-zero. */
static void contra_rom_advance_enemy_routine(ContraCore *core, uint8_t x)
{
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] != 0u)
    {
        core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] =
            (uint8_t)(core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] + 1u);
    }
}

/* enable_enemy_collision (bank7.asm:8376): ENEMY_STATE_WIDTH &= 0x7E (clear the
   inactive bit 7 and the skip-collision bit 0). */
static void contra_rom_enable_enemy_collision(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Eu);
}

/* enable_bullet_enemy_collision (bank7.asm:8371): ENEMY_STATE_WIDTH &= 0x7F
   (clear bit 7 only, so bullets hit but the body stays non-collidable). */
static void contra_rom_enable_bullet_collision(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] =
        (uint8_t)(core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x7Fu);
}

/* --- enemy bullets (type 0x01), bank0/bank7 --- */
/* quadrant aim direction -> offset into the fractional-velocity table */
static const uint8_t contra_bullet_fract_vel_dir_lookup_tbl[24] = {
    0x00u, 0x02u, 0x04u, 0x06u, 0x08u, 0x0Au, 0x0Cu, 0x0Au, 0x08u, 0x06u, 0x04u, 0x02u,
    0x00u, 0x02u, 0x04u, 0x06u, 0x08u, 0x0Au, 0x0Cu, 0x0Au, 0x08u, 0x06u, 0x04u, 0x02u};
/* {y_fract, x_fract} pairs (fast byte starts 0); bank7 bullet_fract_vel_tbl */
static const uint8_t contra_bullet_fract_vel_tbl[14] = {
    0x00u, 0xFFu, 0x42u, 0xF7u, 0x80u, 0xDDu, 0xB5u, 0xB5u, 0xDDu, 0x80u, 0xF7u, 0x42u, 0xFFu, 0x00u};
static const uint8_t contra_bullet_sprite_tbl[6] = {0x1Eu, 0x21u, 0x21u, 0x1Eu, 0x79u, 0x07u};
static const uint8_t contra_bullet_palette_tbl[6] = {0x01u, 0x02u, 0x02u, 0x01u, 0x01u, 0x02u};
static const uint8_t contra_bullet_collision_code_tbl[6] = {0x01u, 0x05u, 0x05u, 0x01u, 0x02u, 0x00u};
/* cannonball_explosion_sprite_tbl (bank0:498): type-1 bomb ground-explosion frames. */
static const uint8_t contra_cannonball_explosion_sprite_tbl[3] = {0x37u, 0x36u, 0x37u};
/* adjust_bullet_velocity speed scaling reduces to vel*mult/8: 0.5x .. 1.875x */
/* adjust_bullet_velocity (bank7:10086): per-speed-code shift-add cascades.
   NOT vel*mult/8 -- each shift stage truncates at the byte level (the ROM
   comments document that 1.75x/1.87x drop the fast-byte carry), so the low
   bits differ from a clean multiply; the subpixel phase is gameplay-visible
   as the frame each +1px carry lands on. */
static uint16_t contra_rom_adjust_bullet_velocity(uint8_t fract, uint8_t speed)
{
    uint8_t v04 = fract; /* $04 */
    uint8_t v05 = 0u;    /* $05 */
    uint8_t half;        /* lda $05 / lsr / lda $04 / ror */
    uint8_t a;
    unsigned sum;

    switch (speed & 0x07u)
    {
        case 0x00u: /* .5x: lsr $05 / ror $04 */
        case 0x01u: /* .75x: halve in place, then the 1.5x add of the halved value */
        {
            const uint8_t carry = (uint8_t)(v05 & 0x01u);

            v05 >>= 1u;
            v04 = (uint8_t)((v04 >> 1u) | (uint8_t)(carry << 7u));
            if ((speed & 0x07u) == 0x00u)
            {
                break;
            }
        }
        /* fall through */
        case 0x04u: /* 1.5x: add the byte-half */
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            sum = (unsigned)v04 + half;
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;

        case 0x03u: /* 1.25x: add the byte-quarter */
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            sum = (unsigned)v04 + (half >> 1u);
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;

        case 0x02u: /* 1x */
            break;

        case 0x05u: /* 1.62x: v + half + eighth */
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            a = (uint8_t)((uint8_t)((half >> 1u) >> 1u) + half);
            sum = (unsigned)v04 + a;
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;

        case 0x06u: /* 1.75x: v + half + quarter (byte-truncated, ROM quirk) */
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            a = (uint8_t)((uint8_t)(half >> 1u) + half);
            sum = (unsigned)v04 + a;
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;

        case 0x07u: /* 1.87x: v + half + quarter + eighth (byte-truncated) */
        default:
            half = (uint8_t)((v04 >> 1u) | (uint8_t)((v05 & 0x01u) << 7u));
            a = (uint8_t)((uint8_t)(half >> 1u) + half);
            a = (uint8_t)(a + (uint8_t)((uint8_t)(half >> 1u) >> 1u));
            sum = (unsigned)v04 + a;
            v04 = (uint8_t)sum;
            v05 = (uint8_t)(v05 + (sum >> 8u));
            break;
    }
    return (uint16_t)(((uint16_t)v05 << 8u) | v04);
}

/* create_enemy_bullet (bank7): spawn a type-1 enemy bullet at (px,py) aimed by
   (angle, quadrant: bit0 up, bit1 left) at the given speed. */
/* create_enemy_bullet (bank7:9857-9911): spawn type-1 enemy bullet aimed by angle/quadrant. */
static bool contra_rom_create_enemy_bullet(
    ContraCore *core, uint8_t btype, uint8_t angle, uint8_t quadrant, uint8_t speed,
    uint8_t px, uint8_t py)
{
    uint8_t *const ram = core->ram;
    const int slot = contra_rom_find_next_enemy_slot(core);
    uint8_t sx;
    uint8_t idx;
    uint16_t xv;
    uint16_t yv;

    if (slot < 0)
    {
        return false;
    }
    sx = (uint8_t)slot;
    ram[CONTRA_RAM_ENEMY_TYPE + sx] = 0x01u;
    contra_rom_initialize_enemy(core, sx);
    ram[CONTRA_RAM_ENEMY_VAR_1 + sx] = btype;
    if (speed >= 0x07u)
    {
        speed = 0x07u;
    }
    ram[0x06u] = speed;
    ram[CONTRA_RAM_ENEMY_Y_POS + sx] = py;
    ram[CONTRA_RAM_ENEMY_X_POS + sx] = px;

    idx = contra_bullet_fract_vel_dir_lookup_tbl[angle % 24u];
    xv = contra_rom_adjust_bullet_velocity(contra_bullet_fract_vel_tbl[idx + 1u], speed);
    yv = contra_rom_adjust_bullet_velocity(contra_bullet_fract_vel_tbl[idx], speed);
    if ((quadrant & 0x01u) != 0u)
    {
        yv = (uint16_t)(0u - yv); /* up */
    }
    if ((quadrant & 0x02u) != 0u)
    {
        xv = (uint16_t)(0u - xv); /* left */
    }
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + sx] = (uint8_t)(yv >> 8u);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + sx] = (uint8_t)yv;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + sx] = (uint8_t)(xv >> 8u);
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + sx] = (uint8_t)xv;
    return true;
}

/* enemy_bullet_routine_00/01 (bank0:377): set collision code, then each frame
   set the bullet sprite and apply velocity. */
static void contra_rom_enemy_bullet_routine_00(ContraCore *core, uint8_t x)
{
    const uint8_t btype = core->ram[CONTRA_RAM_ENEMY_VAR_1 + x];

    core->ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] =
        contra_bullet_collision_code_tbl[(btype < 6u) ? btype : 0u];
    contra_rom_advance_enemy_routine(core, x);
}

static void contra_rom_enemy_bullet_routine_01(ContraCore *core, uint8_t x)
{
    const uint8_t btype = (core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] < 6u)
        ? core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] : 0u;

    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_bullet_sprite_tbl[btype];
    core->ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = contra_bullet_palette_tbl[btype];
    contra_rom_update_enemy_pos(core, x); /* applies velocity + scroll, removes off-screen */
    /* the ROM continues unconditionally after update_enemy_pos even when the
       bullet was just removed -- the writes below land in the husk, and
       advance_enemy_routine no-ops on routine 0, exactly like the ROM */
    if (core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x00u)
    {
        /* regular bullet (bank0:405-411): on levels that flag solid bullet-bg
           collision (LEVEL_SOLID_BG_COLLISION_CHECK bit 7), remove the bullet
           when it flies into solid background. */
        if (((core->ram[CONTRA_RAM_LEVEL_SOLID_BG_COLLISION_CHECK] & 0x80u) != 0u) &&
            (contra_rom_get_bg_collision_far(
                 core, core->ram[CONTRA_RAM_ENEMY_X_POS + x],
                 core->ram[CONTRA_RAM_ENEMY_Y_POS + x]) == 0x80u))
        {
            contra_rom_remove_enemy(core, x);
        }
        return;
    }
    if (core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x01u)
    {
        /* cannonball_add_gravity_explode (bank0:457): the large cannonball
           (bullet sub-type 1, the L1 boss bomb) arcs under gravity -- add #$14 to
           the 16-bit Y velocity each frame -- and explodes at the ground
           (Y >= 0xD0), advancing to the explosion routine. Without this the bomb
           flies in a straight line. */
        const unsigned f =
            (unsigned)core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] + 0x14u;

        core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)f;
        core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] =
            (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (f >> 8u));
        if (core->ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xD0u)
        {
            core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
            core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x01u;
            contra_rom_advance_enemy_routine(core, x); /* -> enemy_bullet_routine_02 */
        }
    }
    else if (core->ram[CONTRA_RAM_ENEMY_VAR_1 + x] == 0x03u)
    {
        if ((core->ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xB4u) ||
            (core->ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x20u) ||
            (core->ram[CONTRA_RAM_ENEMY_X_POS + x] >= 0xE0u))
        {
            contra_rom_remove_enemy(core, x);
        }
    }
}

/* enemy_bullet_routine_02 (bank0:482): the L1 boss cannonball ground-explosion
   animation -- 3 frames (sprites $37,$36,$37) on an 8-frame step, then advance to
   remove_enemy. */
static void contra_rom_enemy_bullet_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t frame;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return; /* removed by scroll */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    if (frame >= 0x03u)
    {
        contra_rom_advance_enemy_routine(core, x); /* -> remove_enemy */
        return;
    }
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_cannonball_explosion_sprite_tbl[frame];
}

/* --- sniper (enemy type 0x06), bank0.asm:1738 --- */
static const uint8_t contra_sniper_animation_delay_tbl[3] = {0x01u, 0x30u, 0x80u};
/* sniper_animation_delay_2_tbl (bank0.asm:1774): re-hide -> re-stand-up delay
   per sniper type, set when sniper_routine_03 loops back to sniper_routine_01. */
static const uint8_t contra_sniper_animation_delay_2_tbl[3] = {0x01u, 0x60u, 0x80u};
static const uint8_t contra_sniper_frame_tbl[3] = {0x03u, 0x00u, 0x00u};
static const uint8_t contra_sniper_attack_delay_tbl[3] = {0x40u, 0x04u, 0x10u};
static const uint8_t contra_sniper_bullet_attack_count_tbl[3] = {0x03u, 0x01u, 0x03u};
/* sniper_standing_sprite_tbl (bank0:1972): muzzle sprite per vertical aim band
   (up / straight / down). sniper_bullet_y/x_offset (bank0:1976/1980): bullet spawn
   offset for the same band. sniper_bullet_speed (bank0:1987) per sniper type. */
static const uint8_t contra_sniper_standing_sprite_tbl[3] = {0x04u, 0x03u, 0x05u};
static const uint8_t contra_sniper_bullet_y_offset[3] = {0xEEu, 0xF5u, 0x06u};
static const uint8_t contra_sniper_bullet_x_offset[3] = {0xF3u, 0xF1u, 0xF1u};
static const uint8_t contra_sniper_bullet_speed[3] = {0x03u, 0x05u, 0x03u};

/* implemented after the aim helpers (contra_rom_aim_at_player /
   contra_rom_player_enemy_x_dist_idx); forward-declared so sniper_routine_02 can
   fire without reordering the file. */
static void contra_rom_sniper_fire_bullet(ContraCore *core, uint8_t x);

/* sniper sprite codes by ENEMY_FRAME (bank0.asm:2073): regular/hiding vs boss. */
static const uint8_t contra_sniper_sprite_00[7] = {0x44u, 0x45u, 0x46u, 0x43u, 0x42u, 0x41u, 0x29u};
static const uint8_t contra_sniper_sprite_01[7] = {0x44u, 0x45u, 0x46u, 0x2Cu, 0x42u, 0x2Du, 0x29u};

/* set_soldier... sniper_set_sprite (bank0.asm:1709-area): sprite code from
   ENEMY_FRAME and sniper type, flip from firing angle (VAR_2), recoil (VAR_3). */
static void contra_rom_sniper_set_sprite(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t *const tbl =
        (ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] >= 0x02u) ? contra_sniper_sprite_01 : contra_sniper_sprite_00;
    const uint8_t frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    uint8_t attr;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = tbl[(frame < 7u) ? frame : 0u];
    attr = ((ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x01u) == 0u) ? 0x40u : 0x00u;
    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
        attr = (uint8_t)(attr | 0x08u);
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = attr;
}

/* sniper_routine_02 (bank0.asm:1839): render, track scroll, run the attack
   cadence, and fire an aimed bullet at the player on each shot of the burst. */
static void contra_rom_sniper_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t attr;

    contra_rom_sniper_set_sprite(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_4 + x] - 1u);
    if ((int8_t)ram[CONTRA_RAM_ENEMY_VAR_4 + x] >= 0)
    {
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x18u;
        contra_rom_sniper_fire_bullet(core, x);
        return;
    }

    attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    if (attr == 0u)
    {
        /* standing sniper: reset the burst and the between-attack delay */
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = contra_sniper_bullet_attack_count_tbl[0];
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x80u;
    }
    else
    {
        /* crouching / boss sniper: re-hide and advance to the post-attack routine */
        ram[CONTRA_RAM_ENEMY_FRAME + x] = ((attr & 0x01u) != 0u) ? 0x02u : 0x03u;
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x80u;
        contra_rom_advance_enemy_routine(core, x);
    }
}

/* defined later; forward-declared so sniper_routine_03 (the re-hide state) can
   call them without reordering the file. */
static void contra_rom_set_enemy_routine_to_a(ContraCore *core, uint8_t x, uint8_t a);
static void contra_rom_disable_enemy_collision(ContraCore *core, uint8_t x);

/* sniper_routine_03 (bank0.asm:1991): the post-attack RE-HIDE state for the
   crouching/boss sniper. It crouches back down (decrementing ENEMY_FRAME), and
   when the crouch animation completes loops the sniper back to sniper_routine_01
   (ENEMY_ROUTINE = 2) so it pops up and attacks again. CRUCIALLY it ends by
   applying add_scroll_to_enemy_pos (the ROM's `jmp add_scroll_to_enemy_pos`) so
   the hidden sniper stays world-anchored while the screen scrolls. Without this
   state the sniper froze in this routine and drifted with the scroll. */
static void contra_rom_sniper_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_disable_enemy_collision(core, x);
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] == 0u)
        {
            const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
            const uint8_t idx = (attr < 3u) ? attr : 0u;
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = contra_sniper_animation_delay_2_tbl[idx];
            /* -> ENEMY_ROUTINE = 2, i.e. re-run sniper_routine_01 (stand up). */
            contra_rom_set_enemy_routine_to_a(core, x, 0x02u);
        }
        /* @continue: boss sniper (type 0x02) at crouch frame 2 nudges position */
        if (ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] == 0x02u &&
            ram[CONTRA_RAM_ENEMY_FRAME + x] == 0x02u)
        {
            contra_rom_add_a_to_enemy_y_pos(core, x, 0x0Eu);
            contra_rom_add_a_to_enemy_x_pos(core, x, 0xFFu);
        }
    }

    /* @set_sprite_add_scroll_exit */
    contra_rom_sniper_set_sprite(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
}

/* sniper_routine_00 (bank0.asm:1751): init delay/frame from attributes, nudge Y
   down (+4, plus +5 for the crouching sniper), advance. */
static void contra_rom_sniper_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    const uint8_t idx = (attr < 3u) ? attr : 0u;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = contra_sniper_animation_delay_tbl[idx];
    ram[CONTRA_RAM_ENEMY_FRAME + x] = contra_sniper_frame_tbl[idx];
    /* add_4_to_enemy_y_pos is the VERTICAL_SCROLL-snapping variant
       (bank7:8404) -- on the waterfall it seats the sniper on the scroll
       grid; the crouch +5 below is the plain add. */
    contra_rom_add_4_to_enemy_y_pos(core, x);
    if (attr == 0x01u)
    {
        contra_rom_add_a_to_enemy_y_pos(core, x, 0x05u);
    }
    contra_rom_advance_enemy_routine(core, x);
}

/* sniper_routine_01 (bank0.asm:1787): set sprite, track scroll, run the crouch
   animation for hiding snipers, then enable collision and advance to attack. */
static void contra_rom_sniper_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t attr;

    contra_rom_sniper_set_sprite(core, x);
    /* The ROM does NOT bail out when add_scroll_to_enemy_pos removes the
       off-screen sniper here: the rest of the routine runs on the husk (the
       delay still elapses and enable_enemy_collision leaves sw=0x02 on the
       removed slot); only advance_enemy_routine's routine==0 guard stops the
       advance. Faithful husks need the same zombie tail. */
    contra_rom_add_scroll_to_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }

    attr = ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x];
    if (attr != 0u)
    {
        /* crouching / boss sniper: cycle the un-hiding animation */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x03u)
        {
            return;
        }
        if (attr == 0x01u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
            /* dec left frame at 2 (non-zero) -> fall through to enable collision */
        }
        else
        {
            contra_rom_add_a_to_enemy_y_pos(core, x, 0xF2u); /* -14 */
            contra_rom_add_a_to_enemy_x_pos(core, x, 0x01u);
        }
    }

    contra_rom_enable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x30u;
    {
        const uint8_t idx = (attr < 3u) ? attr : 0u;
        ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = contra_sniper_attack_delay_tbl[idx];
        ram[CONTRA_RAM_ENEMY_VAR_4 + x] = contra_sniper_bullet_attack_count_tbl[idx];
    }
    contra_rom_advance_enemy_routine(core, x);
}

/* set_enemy_routine_to_a (bank7.asm:7698): ENEMY_ROUTINE = a. */
static void contra_rom_set_enemy_routine_to_a(ContraCore *core, uint8_t x, uint8_t a)
{
    core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] = a;
}

/* set_enemy_delay_adv_routine (bank7.asm:7585): ENEMY_ANIMATION_DELAY = a; advance. */
static void contra_rom_set_enemy_delay_adv_routine(ContraCore *core, uint8_t x, uint8_t a)
{
    core->ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = a;
    contra_rom_advance_enemy_routine(core, x);
}

/* set_carry_if_past_trigger_point (bank0.asm:947), horizontal: true if the
   enemy has scrolled left past trigger_x (ENEMY_X_POS < trigger_x). */
/* set_carry_if_past_trigger_point (bank0:3870): horizontal levels trigger when
   the enemy X has scrolled left of trigger_x; the vertical level triggers when
   the enemy Y has scrolled DOWN past trigger_y. */
static bool contra_rom_past_trigger_x(const ContraCore *core, uint8_t x,
                                      uint8_t trigger_x, uint8_t trigger_y)
{
    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        return core->ram[CONTRA_RAM_ENEMY_Y_POS + x] >= trigger_y;
    }
    return core->ram[CONTRA_RAM_ENEMY_X_POS + x] < trigger_x;
}

static void contra_cache_level_1_supertile(
    ContraCore *core,
    uint8_t slot,
    int enemy_x,
    int enemy_y,
    uint8_t supertile
)
{
    const int scroll_offset = (int)core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET];
    const uint16_t world_base =
        (uint16_t)(((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] << 8u) +
                   core->ram[CONTRA_RAM_LEVEL_SCREEN_SCROLL_OFFSET]);
    const int aligned_x = (((enemy_x - 12) + scroll_offset) & ~7) - scroll_offset;
    const int aligned_y = (enemy_y - 12) & ~7;

    if (slot >= CONTRA_ROM_ENEMY_SLOTS)
    {
        return;
    }

    core->l1_supertile[slot] = supertile;
    core->l1_supertile_world_x[slot] = (uint16_t)(world_base + aligned_x);
    core->l1_supertile_screen_y[slot] = (uint8_t)aligned_y;
}

/* set_weapon_box_supertile (bank0.asm:603): draw the pill-box background
   super-tile for ENEMY_FRAME, at the enemy position. Called from the routine on
   each animation step (like the ROM) -- not every render frame -- so the box
   doesn't flicker. */
static const uint8_t contra_weapon_box_supertile_tbl[3] = {0x00u, 0x01u, 0x02u};
static bool contra_rom_set_weapon_box_supertile(ContraCore *core, uint8_t x)
{
    const uint8_t frame = core->ram[CONTRA_RAM_ENEMY_FRAME + x];
    const uint8_t supertile = contra_weapon_box_supertile_tbl[(frame < 3u) ? frame : 0u];

    contra_render_level_1_nametable_update_supertile(
        core,
        (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        supertile);
    /* The ROM routes this through draw_enemy_supertile_a_set_delay, whose
       nametable write persists. Our background is re-composed from the original
       level layout every frame, so the open/partial super-tile only survives if
       it is registered in the per-frame L1 redraw cache (see
       contra_render_native_enemies). Without this the pill-box door never
       visually opens -- only the closed box baked into the level data shows. */
    contra_cache_level_1_supertile(
        core,
        x,
        (int)core->ram[CONTRA_RAM_ENEMY_X_POS + x],
        (int)core->ram[CONTRA_RAM_ENEMY_Y_POS + x],
        supertile);
    return false; /* carry clear == drew successfully */
}

/* weapon_box_routine_00 (bank0.asm:518): init frame, delay, advance. */
static void contra_rom_weapon_box_routine_00(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x01u;
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x20u);
}

/* weapon_box_routine_01 (bank0.asm:529): track scroll; once scrolled past the
   activation trigger, start the open animation; close if near the left edge. */
static void contra_rom_weapon_box_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (!contra_rom_past_trigger_x(core, x, 0xF0u, 0x30u))
    {
        return; /* not yet at activation point */
    }
    if (contra_rom_past_trigger_x(core, x, 0x18u, 0xC8u))
    {
        contra_rom_set_enemy_routine_to_a(core, x, 0x04u); /* near left edge: close */
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u); /* -> routine_02 */
}

/* weapon_box_routine_02 (bank0.asm:550): open/close animation cycle (HP toggles
   between 0xF0 while open/invulnerable-frame and 0x01 while closed). */
static void contra_rom_weapon_box_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (contra_rom_past_trigger_x(core, x, 0x18u, 0xC8u))
    {
        contra_rom_set_enemy_routine_to_a(core, x, 0x04u);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;

    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        /* @open_weapon_box: animate toward open */
        if (contra_rom_set_weapon_box_supertile(core, x))
        {
            return;
        }
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] != 0u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
            return;
        }
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        ram[CONTRA_RAM_ENEMY_HP + x] = 0xF0u;
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x00u;
    }
    else
    {
        /* closed: animate toward closed */
        if (contra_rom_set_weapon_box_supertile(core, x))
        {
            return;
        }
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] < 0x02u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
            return;
        }
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] - 1u);
        ram[CONTRA_RAM_ENEMY_HP + x] = 0x01u;
        ram[CONTRA_RAM_ENEMY_VAR_2 + x] = 0x01u;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x40u;
    contra_rom_set_enemy_routine_to_a(core, x, 0x02u); /* -> routine_01 */
}

/* weapon_box_routine_03 (bank0.asm:617): deactivated; draw closed, then remove. */
static void contra_rom_weapon_box_routine_03(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (core->ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    if (contra_rom_set_weapon_box_supertile(core, x))
    {
        return; /* buffer full: drawn next frame */
    }
    contra_rom_remove_enemy(core, x); /* keep the cached close tile: the ROM nametable write persists */
}

/* Defined later (used by the weapon-item landing code); forward-declared so
   add_4_to_enemy_y_pos can grid-snap the enemy Y here. */
static void contra_rom_add_a_with_vert_scroll_to_enemy_y_pos(ContraCore *core, uint8_t x, uint8_t a);

/* add_4_to_enemy_y_pos (bank7:8491-8509): a=4, then add_a_with_vert_scroll. The ROM
   falls through into the vert-scroll snap, so this is NOT a plain Y += 4 -- it grid-
   aligns the enemy to the terrain before nudging down. */
static void contra_rom_add_4_to_enemy_y_pos(ContraCore *core, uint8_t x)
{
    contra_rom_add_a_with_vert_scroll_to_enemy_y_pos(core, x, 0x04u);
}

/* add_y_to_y_pos_get_bg_collision (bank7.asm:8679): bg collision code at
   (ENEMY_X_POS, ENEMY_Y_POS + y_off) without modifying the stored position.
   Codes match the ROM: 0 empty, 1 floor, 2 water, 0x80 solid. */
static uint8_t contra_rom_add_y_to_y_pos_get_bg_collision(const ContraCore *core, uint8_t x, uint8_t y_off)
{
    const uint8_t ey = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_POS + x] + y_off);
    return contra_get_outdoor_bg_collision(core, core->ram[CONTRA_RAM_ENEMY_X_POS + x], ey);
}

/* set_enemy_y_velocity_to_0 (bank7:7858-7862): zero only ENEMY_Y_VELOCITY FRACT and
   FAST. The ROM deliberately leaves ENEMY_Y_VEL_ACCUM (the running sub-pixel carry)
   untouched, so the next time Y velocity resumes its accumulator phase continues. */
static void contra_rom_set_enemy_y_velocity_to_0(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0u;
    core->ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0u;
}

/* --- soldier / running man (enemy type 0x05), bank0.asm:1217 --- */
static const uint8_t contra_soldier_initial_anim_delay_tbl[4] = {0x01u, 0x10u, 0x20u, 0x30u};
/* soldier_x_vel_tbl: {fract,fast} for left/right, horizontal then vertical. */
static const uint8_t contra_soldier_x_vel_tbl[8] = {0x00u, 0xFFu, 0x40u, 0x01u, 0x00u, 0xFFu, 0x00u, 0x01u};
static const uint8_t contra_soldier_vel_index_tbl[8] = {0x00u, 0x00u, 0x04u, 0x00u, 0x04u, 0x00u, 0x04u, 0x04u};
static const uint8_t contra_soldier_velocity_tbl[8] = {0x00u, 0xFEu, 0x48u, 0xFFu, 0x00u, 0xFFu, 0x60u, 0xFFu};

/* soldier_set_x_velocity (bank0.asm:1242): set X velocity from ENEMY_VAR_2
   direction (0 left, 1 right) and the level scrolling type. */
static void contra_rom_soldier_set_x_velocity(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t base = (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ? 0x04u : 0x00u;
    const uint8_t off = (uint8_t)((uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_2 + x] << 1u) + base);

    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_soldier_x_vel_tbl[off];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_soldier_x_vel_tbl[off + 1u];
}

/* soldier_stop_y_set_x_velocity (bank0.asm:1237): set X velocity, zero Y. */
static void contra_rom_soldier_stop_y_set_x_velocity(ContraCore *core, uint8_t x)
{
    contra_rom_soldier_set_x_velocity(core, x);
    contra_rom_set_enemy_y_velocity_to_0(core, x);
}

/* soldier_routine_00 (bank0.asm:1217): track scroll, nudge to ground, set the
   initial animation delay from attributes, advance. */
static void contra_rom_soldier_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t idx;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    contra_rom_add_4_to_enemy_y_pos(core, x);
    idx = (uint8_t)((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] >> 4u) & 0x03u);
    contra_rom_set_enemy_delay_adv_routine(core, x, contra_soldier_initial_anim_delay_tbl[idx]);
}

/* soldier_routine_01 (bank0.asm:1275): tick the spawn delay (faithful to the
   ROM's quirky double-decrement for left-runners), then verify there is ground
   under the soldier, enable collision, pick a direction and walk velocity, and
   advance to the walk routine. Removes the soldier if there is no ground (e.g.
   a destroyed bridge). */
static void contra_rom_soldier_routine_01(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    bool enable_set_vel = false;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        /* Vertical levels first anchor the soldier to terrain scroll, then use
           the normal one-decrement spawn delay path. The guard must check the
           ROUTINE (the husk-keeping remove_enemy leaves the TYPE in place). */
        contra_rom_add_scroll_to_enemy_pos(core, x);
        if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
        {
            return;
        }
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            return;
        }
        enable_set_vel = true;
    }
    else if (ram[CONTRA_RAM_FRAME_SCROLL] == 0u)
    {
        /* @dec_delay_enable_set_vel */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            return;
        }
        enable_set_vel = true;
    }
    else if ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u) != 0u)
    {
        /* running right: only tick on odd frames */
        if ((ram[CONTRA_RAM_FRAME_COUNTER] & 0x01u) == 0u)
        {
            return;
        }
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            return;
        }
        enable_set_vel = true;
    }
    else
    {
        /* running left: @continue — decrement, and if not yet zero, decrement
           again (the ROM falls through @continue into @dec_delay_enable_set_vel) */
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
        {
            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
                (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
            if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
            {
                return;
            }
        }
        enable_set_vel = true;
    }

    if (!enable_set_vel)
    {
        return;
    }

    /* @enable_set_vel: require ground #$10 below, else remove */
    if (contra_rom_add_y_to_y_pos_get_bg_collision(core, x, 0x10u) == 0u)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* ROM remove_enemy keeps the husk */
        return;
    }
    contra_rom_enable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x01u);
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_X_POS + x] = 0x0Au; /* running right: enter from left */
    }
    contra_rom_soldier_stop_y_set_x_velocity(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x10u); /* -> routine_02 (walk) */
}

/* update_enemy_x_pos / update_enemy_y_pos (bank7.asm:7736-): apply the 16-bit
   fixed-point velocity (FAST.FRACT via the accumulator) to the position. */
static void contra_rom_update_enemy_x_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint16_t accum =
        (uint16_t)ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] + ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x];

    ram[CONTRA_RAM_ENEMY_X_VEL_ACCUM + x] = (uint8_t)accum;
    ram[CONTRA_RAM_ENEMY_X_POS + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] +
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] + (uint8_t)(accum >> 8u));
}

/* update_enemy_y_pos (bank7:7881-7890): apply Y velocity (accum+fract, Y += fast + carry). */
static void contra_rom_update_enemy_y_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint16_t accum =
        (uint16_t)ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] + ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x];

    ram[CONTRA_RAM_ENEMY_Y_VEL_ACCUM + x] = (uint8_t)accum;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] +
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] + (uint8_t)(accum >> 8u));
}

/* update_enemy_pos (bank7.asm:7736), horizontal: apply X velocity then subtract
   the frame scroll; apply Y velocity; remove if off the left/bottom edge. */
static void contra_rom_update_enemy_pos(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        /* vertical level (bank7:7737-7748): apply Y velocity AND the screen scroll so
           the enemy stays anchored to the terrain (update_enemy_y_pos_with_scroll),
           then apply X velocity with no scroll. */
        contra_rom_update_enemy_y_pos(core, x);
        ram[CONTRA_RAM_ENEMY_Y_POS + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + ram[CONTRA_RAM_FRAME_SCROLL]);
        if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xE8u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* ROM: remove_enemy keeps type */
            return;
        }
        contra_rom_update_enemy_x_pos(core, x);
        if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
        {
            contra_rom_remove_enemy_offscreen(core, x); /* ROM: remove_enemy keeps type */
        }
        return;
    }

    contra_rom_update_enemy_x_pos(core, x);
    ram[CONTRA_RAM_ENEMY_X_POS + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_X_POS + x] - ram[CONTRA_RAM_FRAME_SCROLL]);
    if (ram[CONTRA_RAM_ENEMY_X_POS + x] < 0x08u)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* ROM: remove_enemy keeps type */
        return;
    }
    contra_rom_update_enemy_y_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] >= 0xE8u)
    {
        contra_rom_remove_enemy_offscreen(core, x); /* ROM: remove_enemy keeps type */
    }
}

/* add_a_y_to_enemy_pos_get_bg_collision (bank7.asm:8692): bg collision at
   (ENEMY_X_POS + a, ENEMY_Y_POS + y_off), positions unchanged. */
static uint8_t contra_rom_add_a_y_to_enemy_pos_get_bg_collision(
    const ContraCore *core, uint8_t x, uint8_t a, uint8_t y_off)
{
    const uint8_t ex = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_X_POS + x] + a);
    const uint8_t ey = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_Y_POS + x] + y_off);

    return contra_get_outdoor_bg_collision(core, ex, ey);
}

/* set_soldier_sprite (bank0.asm:1709): sprite code from ENEMY_FRAME, flip when
   running left, recoil bit while firing. */
static const uint8_t contra_soldier_sprite_codes[12] = {
    0x3Bu, 0x3Cu, 0x3Du, 0x3Fu, 0x3Cu, 0x3Eu, 0x40u, 0x26u, 0x73u, 0x18u, 0x28u, 0x27u};
static void contra_rom_set_soldier_sprite(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t frame = ram[CONTRA_RAM_ENEMY_FRAME + x];
    uint8_t attr;

    ram[CONTRA_RAM_ENEMY_SPRITES + x] = contra_soldier_sprite_codes[(frame < 12u) ? frame : 0u];
    attr = (ram[CONTRA_RAM_ENEMY_VAR_2 + x] == 0u) ? 0x40u : 0x00u;
    if (ram[CONTRA_RAM_ENEMY_VAR_1 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_VAR_1 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_1 + x] - 1u);
        attr = (uint8_t)(attr | 0x08u);
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = attr;
}

/* soldier_change_direction (bank0.asm): flip direction, reset X velocity. */
/* soldier_change_direction (bank0:1477-1483): flip soldier direction, reset X velocity. */
static void contra_rom_soldier_change_direction(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_VAR_4 + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_VAR_4 + x] + 1u);
    core->ram[CONTRA_RAM_ENEMY_VAR_2 + x] = (uint8_t)(core->ram[CONTRA_RAM_ENEMY_VAR_2 + x] ^ 0x01u);
    contra_rom_soldier_set_x_velocity(core, x);
}

static void contra_rom_soldier_start_jump(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t closest = contra_rom_player_enemy_x_dist(core, x);
    uint8_t y_delta = (uint8_t)(ram[CONTRA_RAM_SPRITE_Y_POS + closest] - ram[CONTRA_RAM_ENEMY_Y_POS + x]);
    uint8_t base = 0x04u;
    uint8_t vel_off;

    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] + 1u);
    if ((ram[CONTRA_RAM_SPRITE_Y_POS + closest] < ram[CONTRA_RAM_ENEMY_Y_POS + x]) ||
        (y_delta < 0x10u))
    {
        if (ram[CONTRA_RAM_SPRITE_Y_POS + closest] < ram[CONTRA_RAM_ENEMY_Y_POS + x])
        {
            y_delta = (uint8_t)(0u - y_delta);
        }
        (void)y_delta;
        base = 0x00u;
    }

    vel_off = contra_soldier_vel_index_tbl[(uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] & 0x03u) + base)];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = contra_soldier_velocity_tbl[vel_off + 0u];
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = contra_soldier_velocity_tbl[vel_off + 1u];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = contra_soldier_velocity_tbl[vel_off + 2u];
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = contra_soldier_velocity_tbl[vel_off + 3u];
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        contra_rom_reverse_enemy_x_direction(core, x);
    }
}

/* soldier_routine_02 (bank0.asm:1323): walk. Animate the run cycle, step in the
   facing direction (velocity + scroll), turn or jump at an edge (the jump pick
   draws on the injected RANDOM_NUM), and start an attack round (routine_03)
   when the fire pick lands. */
static void contra_rom_soldier_routine_02(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    uint8_t code;

    if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x0Au;
        if ((ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] & 0x80u) == 0u)
        {
            code = contra_rom_add_y_to_y_pos_get_bg_collision(core, x, 0x10u);
            if ((code == 0x80u) || (code == 0x01u))
            {
                ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
                ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
                contra_rom_add_4_to_enemy_y_pos(core, x);
                contra_rom_soldier_stop_y_set_x_velocity(core, x);
            }
            else if (code == 0x02u)
            {
                contra_rom_set_enemy_routine_to_a(core, x, 0x0Au);
            }
        }
        if (ram[CONTRA_RAM_ENEMY_VAR_3 + x] != 0u)
        {
            contra_rom_add_10_to_enemy_y_fract_vel(core, x);
        }
        contra_rom_set_soldier_sprite(core, x);
        contra_rom_update_enemy_pos(core, x);
        return;
    }

    /* @continue (bank0:1356): a firing soldier (attribute bits 2-3) starts an
       attack round when its delay elapses -- the bullet count comes from
       get_soldier_num_bullets (RNG + weapon strength). */
    if (((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x0Cu) != 0u) &&
        (ram[CONTRA_RAM_ENEMY_ATTACK_FLAG] != 0u))
    {
        ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
            (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
        if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
        {
            static const uint8_t soldier_num_bullets_tbl[8] = {
                0x01u, 0x01u, 0x02u, 0x01u, 0x02u, 0x01u, 0x02u, 0x02u};
            const uint8_t idx = (uint8_t)((ram[CONTRA_RAM_RANDOM_NUM] & 0x03u) +
                ((ram[CONTRA_RAM_PLAYER_WEAPON_STRENGTH] & 0x02u) << 1u));

            ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x80u;
            ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x08u;
            ram[CONTRA_RAM_ENEMY_VAR_3 + x] = soldier_num_bullets_tbl[idx & 0x07u];
            contra_rom_advance_enemy_routine(core, x); /* -> soldier_routine_03 */
            contra_rom_set_soldier_sprite(core, x);
            contra_rom_update_enemy_pos(core, x);
            return;
        }
    }

    /* @continue_walk_routine: advance the 6-frame run animation every 8 ticks */
    ram[CONTRA_RAM_ENEMY_VAR_A + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_A + x] + 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_A + x] & 0x07u) == 0u)
    {
        ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
        if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= 0x06u)
        {
            ram[CONTRA_RAM_ENEMY_FRAME + x] = 0u;
        }
    }

    /* @soldier_move: is there ground one step ahead and #$10 below? */
    code = contra_rom_add_a_y_to_enemy_pos_get_bg_collision(
        core, x, ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x], 0x10u);
    if ((code != 0x80u) && (code != 0x01u))
    {
        if ((ram[CONTRA_RAM_ENEMY_VAR_4 + x] >= 0x02u) ||
            ((ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x02u) == 0u))
        {
            contra_rom_soldier_start_jump(core, x);
        }
        else
        {
            contra_rom_soldier_change_direction(core, x);
        }
    }

    contra_rom_set_soldier_sprite(core, x);
    contra_rom_update_enemy_pos(core, x);
}

/* --- flying capsule / weapon zeppelin (enemy type 0x03), bank0.asm:680 --- */

/* set_flying_capsule_y_vel + set_flying_capsule_path (bank7.asm:8712/8765):
   harmonic weave -- pull the Y velocity toward the base height VAR_1 by
   subtracting 2*(ENEMY_Y_POS - VAR_1) (16-bit) from the Y velocity. */
static void contra_rom_set_flying_capsule_y_vel(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t pos = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    const uint8_t base = ram[CONTRA_RAM_ENEMY_VAR_1 + x];
    uint16_t dist = (uint16_t)(((pos < base) ? 0xFF00u : 0x0000u) | (uint8_t)(pos - base));
    uint16_t vel;

    dist = (uint16_t)(dist << 1u); /* shift count = 1 (outdoor) */
    vel = (uint16_t)(((uint16_t)ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] << 8u) |
                     ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x]);
    vel = (uint16_t)(vel - dist);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = (uint8_t)(vel >> 8u);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = (uint8_t)vel;
}

/* flying_capsule_routine_00 (bank0.asm:680): record the base position, then
   horizontal levels enter from the left (X=0x10, cruise right + Y weave) while
   the vertical waterfall rises from the bottom (X+=0x20, Y=0xE0, -1.5 up). */
static void contra_rom_flying_capsule_routine_00(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] = 0x03u;
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = ram[CONTRA_RAM_ENEMY_Y_POS + x];
    ram[CONTRA_RAM_ENEMY_VAR_2 + x] = ram[CONTRA_RAM_ENEMY_X_POS + x];
    if (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        contra_rom_add_a_to_enemy_x_pos(core, x, 0x20u);
        ram[CONTRA_RAM_ENEMY_Y_POS + x] = 0xE0u;
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x80u; /* flying_capsule_vel_tbl[4] */
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xFEu;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    }
    else
    {
        contra_rom_add_a_to_enemy_y_pos(core, x, 0x20u);
        ram[CONTRA_RAM_ENEMY_X_POS + x] = 0x10u;
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x00u; /* flying_capsule_vel_tbl[0] */
        ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x80u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x01u;
    }
    contra_rom_advance_enemy_routine(core, x);
}

/* set_flying_capsule_x_vel + set_flying_capsule_path (bank7:8739/8765): the vertical-
   level weave -- pull X velocity toward the base column VAR_2 by subtracting
   2*(ENEMY_X_POS - VAR_2) (16-bit) from the X velocity. Mirror of the Y weave. */
static void contra_rom_set_flying_capsule_x_vel(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t pos = ram[CONTRA_RAM_ENEMY_X_POS + x];
    const uint8_t base = ram[CONTRA_RAM_ENEMY_VAR_2 + x];
    uint16_t dist = (uint16_t)(((pos < base) ? 0xFF00u : 0x0000u) | (uint8_t)(pos - base));
    uint16_t vel;

    dist = (uint16_t)(dist << 1u); /* shift count = 1 (vertical) */
    vel = (uint16_t)(((uint16_t)ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] << 8u) |
                     ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x]);
    vel = (uint16_t)(vel - dist);
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = (uint8_t)(vel >> 8u);
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = (uint8_t)vel;
}

/* flying_capsule_routine_01 (bank0:720-734): sprite 0x4D; the weave axis depends on
   LEVEL_SCROLLING_TYPE -- vertical levels weave X (set_flying_capsule_x_vel),
   horizontal/indoor weave Y -- then apply velocity + scroll. The port previously
   always wove Y, so a capsule on a vertical level drifted on the wrong axis. */
static void contra_rom_flying_capsule_routine_01(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x4Du;
    if (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u)
    {
        contra_rom_set_flying_capsule_x_vel(core, x);
    }
    else
    {
        contra_rom_set_flying_capsule_y_vel(core, x);
    }
    contra_rom_update_enemy_pos(core, x);
}

/* enemy_routine_explosion (bank7.asm:7616): animate the explosion sprite
   sequence (explosion_type_00) then remove. A killed enemy becomes a shared
   explosion actor (ENEMY_TYPE 0xFE) running this; that's a small simplification
   of the ROM (which keeps the type and uses per-type explosion routine slots)
   but produces the faithful explosion sprites. */
static void contra_rom_enemy_routine_explosion(ContraCore *core, uint8_t x)
{
    static const uint8_t explosion_type_00[3] = {0x38u, 0x39u, 0x3Au};
    uint8_t *const ram = core->ram;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= 0x03u)
    {
        contra_rom_clear_enemy(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = explosion_type_00[ram[CONTRA_RAM_ENEMY_FRAME + x]];
}

/* Start the explosion actor on the enemy slot (called from the kill path). */
static void contra_rom_begin_enemy_explosion(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    if (ram[CONTRA_RAM_ENEMY_TYPE + x] == 0x17u)
    {
        ram[CONTRA_RAM_GRENADE_LAUNCHER_FLAG] = 0u; /* grenade_launcher_routine_06 */
    }
    ram[CONTRA_RAM_ENEMY_TYPE + x] = 0xFEu;
    ram[CONTRA_RAM_ENEMY_ROUTINE + x] = 0x01u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    /* enemy_routine_init_explosion (bank7:7546): ora #$81 -- set bit 7 (bullets
       pass through) AND bit 0 (skip player-body collision), so the explosion of a
       killed enemy can't damage the player who walks into it. */
    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] | 0x81u);
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x38u;
    /* enemy_routine_init_explosion (bank7:7544): the death burst forces sprite
       palette 2 ((attr & 0xFC) | 0x06) -- the orange/yellow explosion colors. The
       port hardcoded palette 0, which tinted the explosion with each level's
       palette-0 colors and made L2 deaths look unlike L1's. */
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0xFCu) | 0x06u);
}

/* --- soldier death (bank0.asm soldier_routine_04/05, plus the shared bank7
   explosion routines run IN PLACE on the soldier's own slot). The ROM keeps
   ENEMY_TYPE 0x05 through the whole death sequence (arc -> hide -> explosion
   -> remove leaves type set with routine 0), unlike the shared 0xFE actor
   above, so the soldier ports the shared routines without the type swap. --- */

/* init_soldier_hit_vel (bank0.asm:1657): the shared corpse launch -- fly up
   and away from the facing direction (-3.5 Y, 0.375 X; X zeroed at the screen
   edges, reversed by ENEMY_VAR_2), collision off, 16-frame arc. The sniper's
   death (sniper_routine_04) jmp's into this too. */
static void contra_rom_init_soldier_hit_vel(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;
    const uint8_t x_pos = ram[CONTRA_RAM_ENEMY_X_POS + x];

    contra_rom_disable_enemy_collision(core, x);
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FRACT + x] = 0x80u;
    ram[CONTRA_RAM_ENEMY_Y_VELOCITY_FAST + x] = 0xFCu;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x60u;
    ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    if ((x_pos < 0x10u) || (x_pos >= 0xF0u))
    {
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FRACT + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_X_VELOCITY_FAST + x] = 0x00u;
    }
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        contra_rom_reverse_enemy_x_direction(core, x);
    }
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x10u);
}

/* apply_gravity_to_destroyed_soldier (bank0.asm:1694): the shared corpse arc --
   gravity (+0x30 fract per frame) against the upward velocity; advance to the
   explosion when the timer elapses or the corpse clears the top of the screen. */
static void contra_rom_apply_gravity_to_destroyed_soldier(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    contra_rom_add_a_to_enemy_y_fract_vel(core, x, 0x30u);
    if (ram[CONTRA_RAM_ENEMY_Y_POS + x] < 0x08u)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    contra_rom_update_enemy_pos(core, x);
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] == 0u)
    {
        contra_rom_advance_enemy_routine(core, x);
    }
}

/* soldier_routine_03 (bank0:1521): the attack round. Stand (attrs&0x0C < 5) or
   crouch (>= 5, collision box 0x1B) and fire ENEMY_VAR_3+1 bullets on a 0x10
   beat, skipping shots whose muzzle would start off-screen; then restore the
   collision box and walk again. */
static void contra_rom_soldier_routine_03(ContraCore *core, uint8_t x)
{
    static const uint8_t soldier_bullet_y_offset[4] = {0xF7u, 0xF7u, 0x0Au, 0x0Au};
    static const uint8_t soldier_bullet_x_offset[4] = {0xF0u, 0x10u, 0xF0u, 0x10u};
    static const uint8_t soldier_bullet_type_tbl[2] = {0x06u, 0x00u};
    uint8_t *const ram = core->ram;
    uint8_t yidx;
    uint8_t bullet_x;
    unsigned sum;

    if ((uint8_t)(ram[CONTRA_RAM_ENEMY_ATTRIBUTES + x] & 0x0Cu) < 0x05u)
    {
        yidx = 0u; /* standing shot */
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x06u;
    }
    else
    {
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x1Bu; /* crouching box */
        yidx = 2u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x07u;
    }

    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] != 0u)
    {
        goto sprite_scroll_exit;
    }
    ram[CONTRA_RAM_ENEMY_VAR_3 + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_VAR_3 + x] - 1u);
    if ((ram[CONTRA_RAM_ENEMY_VAR_3 + x] & 0x80u) != 0u)
    {
        /* fired all bullets: stand back up and walk again */
        ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x10u;
        ram[CONTRA_RAM_ENEMY_VAR_3 + x] = 0x00u;
        ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x00u;
        contra_rom_set_soldier_sprite(core, x);
        contra_rom_add_scroll_to_enemy_pos(core, x);
        contra_rom_set_enemy_routine_to_a(core, x, 0x03u); /* -> soldier_routine_02 */
        return;
    }
    ram[CONTRA_RAM_ENEMY_ATTACK_DELAY + x] = 0x10u;
    if (ram[CONTRA_RAM_ENEMY_VAR_2 + x] != 0u)
    {
        yidx = (uint8_t)(yidx + 1u); /* running right */
    }

    sum = (unsigned)soldier_bullet_x_offset[yidx] + ram[CONTRA_RAM_ENEMY_X_POS + x];
    bullet_x = (uint8_t)sum;
    if ((soldier_bullet_x_offset[yidx] & 0x80u) != 0u)
    {
        if ((sum < 0x100u) || (bullet_x < 0x08u))
        {
            goto sprite_scroll_exit; /* muzzle off-screen to the left */
        }
    }
    else if (sum >= 0x100u)
    {
        goto sprite_scroll_exit; /* muzzle off-screen to the right */
    }
    contra_rom_bullet_generation(
        core,
        soldier_bullet_type_tbl[ram[CONTRA_RAM_ENEMY_VAR_2 + x] & 0x01u],
        0x06u,
        bullet_x,
        (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + soldier_bullet_y_offset[yidx]));
    ram[CONTRA_RAM_ENEMY_VAR_1 + x] = 0x06u; /* gun recoil timer */

sprite_scroll_exit:
    contra_rom_set_soldier_sprite(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
}

/* soldier_routine_04 (bank0.asm:1650): hit -- corpse sprite (frame 0x0B), launch. */
static void contra_rom_soldier_routine_04(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x0Bu;
    contra_rom_set_soldier_sprite(core, x);
    contra_rom_init_soldier_hit_vel(core, x);
}

/* soldier_routine_05 (bank0.asm:1689): the corpse arc. */
static void contra_rom_soldier_routine_05(ContraCore *core, uint8_t x)
{
    contra_rom_set_soldier_sprite(core, x);
    contra_rom_apply_gravity_to_destroyed_soldier(core, x);
}

/* soldier_routine_09 (bank0:1615-1626): landed in water -- splash sprite
   (frame 8), sink 0x10, then the splash animation. */
static void contra_rom_soldier_routine_09(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x08u;
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x10u);
    contra_rom_set_soldier_sprite(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x08u);
}

/* soldier_routine_0a (bank0:1627-1640): step the splash to the puddle (frame 9,
   sinking 8 more) on an 8-frame beat, then remove the soldier. */
static void contra_rom_soldier_routine_0a(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        contra_rom_set_soldier_sprite(core, x);
        contra_rom_add_scroll_to_enemy_pos(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x08u;
    ram[CONTRA_RAM_ENEMY_FRAME + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    if (ram[CONTRA_RAM_ENEMY_FRAME + x] >= 0x0Au)
    {
        contra_rom_remove_enemy(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_Y_POS + x] = (uint8_t)(ram[CONTRA_RAM_ENEMY_Y_POS + x] + 0x08u);
    contra_rom_set_soldier_sprite(core, x);
    contra_rom_add_scroll_to_enemy_pos(core, x);
}

/* sniper_routine_04 (bank0.asm:2021): hit -- corpse sprite (frame 0x06), launch. */
static void contra_rom_sniper_routine_04(ContraCore *core, uint8_t x)
{
    core->ram[CONTRA_RAM_ENEMY_FRAME + x] = 0x06u;
    contra_rom_sniper_set_sprite(core, x);
    contra_rom_init_soldier_hit_vel(core, x);
}

/* sniper_routine_05 (bank0.asm:2028): the corpse arc. */
static void contra_rom_sniper_routine_05(ContraCore *core, uint8_t x)
{
    contra_rom_sniper_set_sprite(core, x);
    contra_rom_apply_gravity_to_destroyed_soldier(core, x);
}

/* explosion_sound_hide_enemy (bank7:7589): the shared tail of enemy_routine_
   init_explosion and mortar_shot_routine_03 -- store the updated state width,
   explosion sound if bit 1 allows, force sprite palette 2, hide the sprite for
   one frame, advance. */
static void contra_rom_explosion_sound_hide_enemy(ContraCore *core, uint8_t x, uint8_t sw)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] = sw;
    if ((sw & 0x02u) != 0u)
    {
        contra_play_sound(core, 0x19u); /* sound_19: enemy destroyed */
    }
    ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] =
        (uint8_t)((ram[CONTRA_RAM_ENEMY_SPRITE_ATTR + x] & 0xFCu) | 0x06u);
    if (ram[CONTRA_RAM_ENEMY_SPRITES + x] == 0u)
    {
        contra_rom_remove_enemy(core, x);
        return;
    }
    ram[CONTRA_RAM_ENEMY_FRAME + x] = 0xFFu;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] = 0x01u; /* invisible sprite for one frame */
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_set_enemy_delay_adv_routine(core, x, 0x01u);
}

/* enemy_routine_init_explosion (bank7:7572) on the enemy's own slot. */
static void contra_rom_enemy_routine_init_explosion_inplace(ContraCore *core, uint8_t x)
{
    contra_rom_explosion_sound_hide_enemy(
        core, x, (uint8_t)(core->ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] | 0x81u));
}

/* mortar_shot_routine_03 (bank7:7582): a split mortar round hit the ground --
   score/collision code 0x0D, strip the player-enemy collision bits (0 and 6),
   let bullets pass through (bit 7), then the shared explosion tail. */
static void contra_rom_mortar_shot_routine_03(ContraCore *core, uint8_t x)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_ENEMY_SCORE_COLLISION + x] = 0x0Du;
    contra_rom_explosion_sound_hide_enemy(
        core, x, (uint8_t)((ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0xBEu) | 0x80u));
}

/* enemy_routine_explosion (bank7:7616) on the enemy's own slot: 3 sprites
   (explosion_type_00; 4 from explosion_type_01 when ENEMY_STATE_WIDTH bit 3 is
   set), 10 frames apart, then advance to the remove routine. */
static void contra_rom_enemy_routine_explosion_inplace(ContraCore *core, uint8_t x)
{
    static const uint8_t explosion_type_00[3] = {0x38u, 0x39u, 0x3Au};
    static const uint8_t explosion_type_01[4] = {0x37u, 0x35u, 0x36u, 0x37u};
    uint8_t *const ram = core->ram;
    const bool large = (ram[CONTRA_RAM_ENEMY_STATE_WIDTH + x] & 0x08u) != 0u;
    const uint8_t max_frames = large ? 4u : 3u;
    uint8_t frame;

    contra_rom_add_scroll_to_enemy_pos(core, x);
    if (ram[CONTRA_RAM_ENEMY_ROUTINE + x] == 0u)
    {
        return;
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] =
        (uint8_t)(ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] - 1u);
    if (ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] != 0u)
    {
        return;
    }
    frame = (uint8_t)(ram[CONTRA_RAM_ENEMY_FRAME + x] + 1u);
    ram[CONTRA_RAM_ENEMY_FRAME + x] = frame;
    if (frame >= max_frames)
    {
        contra_rom_advance_enemy_routine(core, x);
        return;
    }
    if ((uint8_t)(frame + 1u) >= max_frames)
    {
        contra_rom_disable_enemy_collision(core, x); /* last sprite */
    }
    ram[CONTRA_RAM_ENEMY_ANIMATION_DELAY + x] = 0x0Au;
    ram[CONTRA_RAM_ENEMY_SPRITES + x] =
        large ? explosion_type_01[frame] : explosion_type_00[frame];
}

/* enemy_routine_remove_enemy (bank7:7706): scroll-track one last frame, then
   clear routine + sprite. ENEMY_TYPE intentionally stays set -- the ROM leaves
   it in the slot until a new spawn reuses it. */
static void contra_rom_enemy_routine_remove_inplace(ContraCore *core, uint8_t x)
{
    contra_rom_add_scroll_to_enemy_pos(core, x);
    contra_rom_remove_enemy(core, x);
}
