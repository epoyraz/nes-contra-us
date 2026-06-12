/* ROM image loading, PPU/CHR helpers, HUD drawing, and background rendering.
   Included by core.c; not compiled as a separate translation unit. */


static bool contra_load_rom_image(void)
{
    static const char *const candidate_paths[] = {
        "baserom.nes",
        "../baserom.nes",
        "../../baserom.nes",
        "../../../baserom.nes"
    };
    size_t path_index;

    if (contra_rom_image.attempted)
    {
        return contra_rom_image.ready;
    }

    contra_rom_image.attempted = true;

    for (path_index = 0u; path_index < (sizeof(candidate_paths) / sizeof(candidate_paths[0])); ++path_index)
    {
        const char *const path = candidate_paths[path_index];
        FILE *file = fopen(path, "rb");

        if (file == NULL)
        {
            continue;
        }

        if (fseek(file, 0, SEEK_END) != 0)
        {
            fclose(file);
            continue;
        }

        const long size_long = ftell(file);
        uint8_t *bytes;
        size_t size;

        if (size_long <= 0)
        {
            fclose(file);
            continue;
        }

        size = (size_t)size_long;
        if (fseek(file, 0, SEEK_SET) != 0)
        {
            fclose(file);
            continue;
        }

        bytes = (uint8_t *)malloc(size);
        if (bytes == NULL)
        {
            fclose(file);
            continue;
        }

        if (fread(bytes, 1u, size, file) != size)
        {
            free(bytes);
            fclose(file);
            continue;
        }

        fclose(file);

        if ((size < 16u) ||
            (bytes[0] != 'N') ||
            (bytes[1] != 'E') ||
            (bytes[2] != 'S') ||
            (bytes[3] != 0x1Au))
        {
            free(bytes);
            continue;
        }

        contra_rom_image.bytes = bytes;
        contra_rom_image.size = size;
        contra_rom_image.ready = true;
        return true;
    }

    return false;
}

static size_t contra_prg_rom_offset(uint8_t bank, uint16_t cpu_addr)
{
    const size_t header_size = 16u;
    const size_t bank_size = 0x4000u;
    const size_t cpu_offset = (bank == 7u)
        ? (size_t)(cpu_addr - 0xC000u)
        : (size_t)(cpu_addr - 0x8000u);

    return header_size + ((size_t)bank * bank_size) + cpu_offset;
}

/* PORT HARNESS (read_u8): no single ASM routine -- read a byte from the PRG-ROM image. */
static uint8_t contra_rom_read_u8(uint8_t bank, uint16_t cpu_addr)
{
    const size_t offset = contra_prg_rom_offset(bank, cpu_addr);

    if ((!contra_load_rom_image()) || (offset >= contra_rom_image.size))
    {
        return 0u;
    }

    return contra_rom_image.bytes[offset];
}

/* PORT HARNESS (read_u16): no single ASM routine -- read a little-endian word from the PRG-ROM image. */
static uint16_t contra_rom_read_u16(uint8_t bank, uint16_t cpu_addr)
{
    const uint8_t low = contra_rom_read_u8(bank, cpu_addr);
    const uint8_t high = contra_rom_read_u8(bank, (uint16_t)(cpu_addr + 1u));

    return (uint16_t)((uint16_t)low | ((uint16_t)high << 8u));
}

static uint8_t contra_horizontal_flip_graphic_byte(uint8_t value)
{
    uint8_t flipped = 0u;
    unsigned bit_index;

    for (bit_index = 0u; bit_index < 8u; ++bit_index)
    {
        flipped = (uint8_t)((flipped << 1u) | (value & 0x01u));
        value >>= 1u;
    }

    return flipped;
}

static void contra_write_ppu_byte(ContraCore *core, uint16_t ppu_addr, uint8_t value)
{
    if (ppu_addr < CONTRA_PPU_PATTERN_TABLE_SIZE)
    {
        core->ppu_pattern[ppu_addr] = value;
        return;
    }

    if ((ppu_addr >= 0x2000u) && (ppu_addr < 0x3F00u))
    {
        core->ppu_nametable[(ppu_addr - 0x2000u) & (CONTRA_PPU_NAMETABLE_SIZE - 1u)] = value;
        return;
    }

    if ((ppu_addr >= 0x3F00u) && (ppu_addr < 0x3F20u))
    {
        core->ppu_palette[(ppu_addr - 0x3F00u) & (CONTRA_PPU_PALETTE_SIZE - 1u)] = value;
    }
}

static void contra_write_cpu_graphics_buffer_byte(ContraCore *core, uint8_t value)
{
    const uint8_t offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];

    if (offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
    {
        return;
    }

    core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + offset] = value;
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = (uint8_t)(offset + 1u);
}

static void contra_flush_cpu_graphics_buffer_to_ppu(ContraCore *core)
{
    uint8_t *const ram = core->ram;
    size_t read_offset = 0u;

    if (ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] != 0u)
    {
        return;
    }

    while (read_offset < CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
    {
        const uint8_t increment_mode = ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset++];
        uint16_t ppu_addr;

        if (increment_mode == 0u)
        {
            break;
        }

        if ((read_offset + 1u) >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
        {
            break;
        }

        ppu_addr = (uint16_t)(
            ((uint16_t)ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset] << 8u) |
            (uint16_t)ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset + 1u]
        );
        read_offset += 2u;

        if (increment_mode == 0x03u)
        {
            uint8_t count;

            if (read_offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
            {
                break;
            }

            count = ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset++];
            while ((count-- != 0u) && (read_offset < CONTRA_CPU_GRAPHICS_BUFFER_SIZE))
            {
                contra_write_ppu_byte(core, ppu_addr++, ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset++]);
            }

            continue;
        }

        for (;;)
        {
            const uint16_t increment = (increment_mode == 0x02u) ? 0x20u : 0x01u;
            uint8_t value;

            if (read_offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
            {
                break;
            }

            value = ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset];

            if (value == 0xFFu)
            {
                if ((read_offset + 1u) >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
                {
                    ++read_offset;
                    break;
                }

                if (ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + read_offset + 1u] < 0x04u)
                {
                    ++read_offset;
                    break;
                }
            }

            contra_write_ppu_byte(core, ppu_addr, value);
            ppu_addr = (uint16_t)(ppu_addr + increment);
            ++read_offset;

            if (read_offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
            {
                break;
            }
        }
    }

    memset(&ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER], 0, CONTRA_CPU_GRAPHICS_BUFFER_SIZE);
    ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = 0x00u;
}

static void contra_apply_graphic_data_to_ppu(ContraCore *core, uint8_t graphic_index)
{
    const ContraGraphicDataRef ref = contra_graphic_data_ptrs[graphic_index];
    const bool flip_horizontal = (ref.bank_code & 0x80u) != 0u;
    const uint8_t bank = (uint8_t)((ref.bank_code & 0x07u) == 0u ? 7u : (ref.bank_code & 0x07u));
    uint16_t read_addr = ref.cpu_addr;
    uint16_t ppu_addr;

    if (!contra_load_rom_image())
    {
        return;
    }

    ppu_addr = contra_rom_read_u16(bank, read_addr);
    read_addr = (uint16_t)(read_addr + (flip_horizontal ? 4u : 2u));

    for (;;)
    {
        const uint8_t command = contra_rom_read_u8(bank, read_addr++);
        unsigned index;

        if (command == 0xFFu)
        {
            return;
        }

        if (command == 0x7Fu)
        {
            ppu_addr = contra_rom_read_u16(bank, read_addr);
            read_addr = (uint16_t)(read_addr + (flip_horizontal ? 4u : 2u));
            continue;
        }

        if ((command & 0x80u) == 0u)
        {
            uint8_t value = contra_rom_read_u8(bank, read_addr++);

            if (flip_horizontal)
            {
                value = contra_horizontal_flip_graphic_byte(value);
            }

            for (index = 0u; index < command; ++index)
            {
                contra_write_ppu_byte(core, ppu_addr++, value);
            }

            continue;
        }

        for (index = 0u; index < (unsigned)(command & 0x7Fu); ++index)
        {
            uint8_t value = contra_rom_read_u8(bank, read_addr++);

            if (flip_horizontal)
            {
                value = contra_horizontal_flip_graphic_byte(value);
            }

            contra_write_ppu_byte(core, ppu_addr++, value);
        }
    }
}

static void contra_load_graphic_data_list(ContraCore *core, uint8_t list_index)
{
    const uint8_t *const graphic_list = contra_level_graphic_data_lists[list_index];
    unsigned entry_index;

    if (!contra_load_rom_image())
    {
        return;
    }

    memset(core->ppu_pattern, 0, sizeof(core->ppu_pattern));

    for (entry_index = 0u; entry_index < 10u; ++entry_index)
    {
        const uint8_t graphic_index = graphic_list[entry_index];

        if (graphic_index == 0xFFu)
        {
            return;
        }

        contra_apply_graphic_data_to_ppu(core, graphic_index);
    }
}

static uint8_t contra_level_screen_supertile_count(const ContraCore *core)
{
    return (core->ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ? 0x40u : 0x38u;
}

static void contra_decode_level_screen_supertiles(
    ContraCore *core,
    uint8_t screen_number,
    uint8_t *dest,
    uint8_t start_offset
)
{
    const uint16_t ptr_table_addr = (uint16_t)(
        (uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_SUPERTILES_PTR] |
        ((uint16_t)core->ram[CONTRA_RAM_LEVEL_SCREEN_SUPERTILES_PTR + 1u] << 8u)
    );
    const uint16_t screen_data_addr = contra_rom_read_u16(2u, (uint16_t)(ptr_table_addr + ((uint16_t)screen_number * 2u)));
    const uint8_t target_end = (uint8_t)(start_offset + contra_level_screen_supertile_count(core));
    uint16_t read_addr = screen_data_addr;
    uint8_t write_offset = start_offset;

    if (!contra_load_rom_image())
    {
        return;
    }

    while (write_offset < target_end)
    {
        uint8_t value = contra_rom_read_u8(2u, read_addr++);
        uint8_t copy_index;

        if (value < 0x80u)
        {
            dest[write_offset++] = value;
            continue;
        }

        if (value < 0xF0u)
        {
            uint8_t repeat_count = (uint8_t)(value & 0x7Fu);
            value = contra_rom_read_u8(2u, read_addr++);

            while ((repeat_count-- != 0u) && (write_offset < target_end))
            {
                dest[write_offset++] = value;
            }

            continue;
        }

        copy_index = (uint8_t)(((value & 0x0Fu) << 3u) | start_offset);
        while ((write_offset < target_end) && (copy_index < CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE))
        {
            dest[write_offset++] = dest[copy_index++];
            if ((uint8_t)(copy_index - (((value & 0x0Fu) << 3u) | start_offset)) >= 8u)
            {
                break;
            }
        }
    }
}

static void contra_load_supertiles_screen_indexes(ContraCore *core, uint8_t screen_number)
{
    contra_decode_level_screen_supertiles(
        core,
        screen_number,
        core->level_screen_supertiles,
        core->ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET]
    );
}

static void contra_write_horizontal_level_column_snapshot_to_ppu(
    ContraCore *core,
    uint16_t ppu_addr,
    uint8_t tile_offset,
    uint8_t supertile_nametable_offset
)
{
    const uint8_t *const ram = core->ram;
    const uint16_t supertile_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u)
    );
    const uint8_t tile_x = tile_offset & 0x1Fu;
    const uint8_t supertile_column = (uint8_t)(tile_x >> 2u);
    const uint8_t tile_x_in_supertile = (uint8_t)(tile_x & 0x03u);
    uint8_t tile_y;

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ||
        (!contra_load_rom_image()))
    {
        return;
    }

    for (tile_y = 0u; tile_y < 28u; ++tile_y)
    {
        const uint8_t supertile_row = (uint8_t)(tile_y >> 2u);
        const uint8_t supertile_offset = (uint8_t)(
            supertile_nametable_offset +
            (uint8_t)(supertile_row * 8u) +
            supertile_column
        );
        const uint8_t supertile_index = core->level_screen_supertiles[supertile_offset];
        const uint16_t supertile_data_addr = (uint16_t)(
            supertile_ptr + ((uint16_t)supertile_index * 16u)
        );
        const uint8_t tile_in_supertile = (uint8_t)(((tile_y & 0x03u) << 2u) | tile_x_in_supertile);
        const uint8_t pattern_index = contra_rom_read_u8(
            3u,
            (uint16_t)(supertile_data_addr + tile_in_supertile)
        );

        contra_write_ppu_byte(core, ppu_addr, pattern_index);
        ppu_addr = (uint16_t)(ppu_addr + 0x20u);
    }
}

static void contra_write_horizontal_level_column_to_ppu(ContraCore *core)
{
    const uint8_t *const ram = core->ram;
    const uint16_t ppu_addr = (uint16_t)(
        ((uint16_t)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] << 8u) |
        (uint16_t)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE]
    );

    contra_write_horizontal_level_column_snapshot_to_ppu(
        core,
        ppu_addr,
        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET],
        ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET]
    );
}

static void contra_write_horizontal_level_column_attributes_snapshot_to_ppu(
    ContraCore *core,
    uint8_t tile_offset,
    uint8_t supertile_nametable_offset,
    uint8_t attr_high
)
{
    const uint8_t *const ram = core->ram;
    const uint16_t palette_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] << 8u)
    );
    const uint8_t attr_col = (uint8_t)((tile_offset >> 2u) & 0x07u);
    uint8_t attr_low = (uint8_t)(0xC0u | attr_col);
    uint8_t attr_row;

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] != 0u) ||
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ||
        (!contra_load_rom_image()))
    {
        return;
    }

    for (attr_row = 0u; attr_row < 7u; ++attr_row)
    {
        const uint8_t supertile_offset = (uint8_t)(
            supertile_nametable_offset +
            (uint8_t)(attr_row * 8u) +
            attr_col
        );
        const uint8_t supertile_index = core->level_screen_supertiles[supertile_offset];
        const uint8_t attr = contra_rom_read_u8(3u, (uint16_t)(palette_ptr + supertile_index));
        const uint16_t ppu_addr = (uint16_t)(
            ((uint16_t)attr_high << 8u) |
            (uint16_t)attr_low
        );

        contra_write_ppu_byte(core, ppu_addr, attr);
        attr_low = (uint8_t)(attr_low + 0x08u);
    }
}

static void contra_write_horizontal_level_column_attributes_to_ppu(ContraCore *core)
{
    const uint8_t *const ram = core->ram;

    contra_write_horizontal_level_column_attributes_snapshot_to_ppu(
        core,
        ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET],
        ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET],
        ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE]
    );
}

static void contra_schedule_horizontal_level_column_write(ContraCore *core)
{
    const uint8_t *const ram = core->ram;

    core->pending_horizontal_column_write = 0x01u;
    core->pending_horizontal_column_tile_offset = ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET];
    core->pending_horizontal_column_supertile_offset = ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET];
    core->pending_horizontal_column_ppu_addr = (uint16_t)(
        ((uint16_t)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] << 8u) |
        (uint16_t)ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE]
    );
}

static void contra_schedule_horizontal_level_column_attributes_write(ContraCore *core)
{
    const uint8_t *const ram = core->ram;

    core->pending_horizontal_attr_write = 0x01u;
    core->pending_horizontal_attr_tile_offset = ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET];
    core->pending_horizontal_attr_supertile_offset = ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET];
    core->pending_horizontal_attr_high = ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE];
}

static void contra_flush_pending_horizontal_level_writes(ContraCore *core)
{
    if (core->pending_horizontal_column_write != 0u)
    {
        contra_write_horizontal_level_column_snapshot_to_ppu(
            core,
            core->pending_horizontal_column_ppu_addr,
            core->pending_horizontal_column_tile_offset,
            core->pending_horizontal_column_supertile_offset
        );
        core->pending_horizontal_column_write = 0x00u;
    }

    if (core->pending_horizontal_attr_write != 0u)
    {
        contra_write_horizontal_level_column_attributes_snapshot_to_ppu(
            core,
            core->pending_horizontal_attr_tile_offset,
            core->pending_horizontal_attr_supertile_offset,
            core->pending_horizontal_attr_high
        );
        core->pending_horizontal_attr_write = 0x00u;
    }
}

static void contra_advance_horizontal_level_ppu_column(ContraCore *core)
{
    uint8_t *const ram = core->ram;

    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] =
        (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] + 1u);
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] =
        (uint8_t)(ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] + 1u);

    if (ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] < 0x20u)
    {
        return;
    }

    ram[CONTRA_RAM_SUPERTILE_NAMETABLE_OFFSET] ^= 0x40u;
    contra_load_supertiles_screen_indexes(core, (uint8_t)(ram[CONTRA_RAM_LEVEL_SCREEN_NUMBER] + 2u));
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_LOW_BYTE] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET] = 0x00u;
    ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] ^= 0x04u;
    ram[CONTRA_RAM_ATTRIBUTE_TBL_WRITE_HIGH_BYTE] ^= 0x04u;
}

static uint8_t contra_read_nametable_byte(const ContraCore *core, uint8_t nametable_index, uint16_t offset)
{
    const size_t nametable_base = ((size_t)(nametable_index & 0x01u) * 0x400u) + (size_t)offset;

    return core->ppu_nametable[nametable_base & (CONTRA_PPU_NAMETABLE_SIZE - 1u)];
}

static uint8_t contra_read_pattern_color_index(
    const ContraCore *core,
    uint16_t pattern_addr,
    uint8_t pixel_x,
    uint8_t pixel_y
)
{
    const uint8_t plane0 = core->ppu_pattern[pattern_addr + pixel_y];
    const uint8_t plane1 = core->ppu_pattern[pattern_addr + pixel_y + 8u];
    const uint8_t shift = (uint8_t)(7u - pixel_x);

    return (uint8_t)(((plane0 >> shift) & 0x01u) | (((plane1 >> shift) & 0x01u) << 1u));
}

static uint8_t contra_read_nametable_palette_slot(
    const ContraCore *core,
    uint8_t nametable_index,
    uint8_t tile_x,
    uint8_t tile_y
)
{
    const uint16_t attr_offset = (uint16_t)(0x3C0u + (((uint16_t)tile_y >> 2u) * 8u) + ((uint16_t)tile_x >> 2u));
    const uint8_t attr = contra_read_nametable_byte(core, nametable_index, attr_offset);
    const uint8_t shift = (uint8_t)(((tile_y & 0x02u) << 1u) | (tile_x & 0x02u));

    return (uint8_t)((attr >> shift) & 0x03u);
}

static uint32_t contra_background_palette_color_rgba(const ContraCore *core, uint8_t palette_slot, uint8_t color_index)
{
    const size_t slot = (color_index == 0u)
        ? 0u
        : (((size_t)palette_slot * 4u) + (size_t)color_index);
    const uint8_t color_code = core->ppu_palette[slot % CONTRA_PPU_PALETTE_SIZE] & 0x3Fu;

    return contra_nes_palette_rgba[color_code];
}

static uint32_t contra_sprite_palette_color_rgba(const ContraCore *core, uint8_t palette_slot, uint8_t color_index)
{
    const size_t slot = 0x10u + ((size_t)palette_slot * 4u) + (size_t)color_index;
    const uint8_t color_code = core->ppu_palette[slot % CONTRA_PPU_PALETTE_SIZE] & 0x3Fu;

    return contra_nes_palette_rgba[color_code];
}

static void contra_draw_background_tile(
    ContraCore *core,
    int dest_x,
    int dest_y,
    uint8_t pattern_index,
    uint8_t palette_slot
)
{
    const uint16_t pattern_addr = (uint16_t)(0x1000u + ((uint16_t)pattern_index * 16u));
    unsigned pixel_y;

    if ((pattern_addr + 15u) >= sizeof(core->ppu_pattern))
    {
        return;
    }

    for (pixel_y = 0u; pixel_y < 8u; ++pixel_y)
    {
        const int framebuffer_y = dest_y + (int)pixel_y;
        unsigned pixel_x;

        if ((framebuffer_y < 0) || (framebuffer_y >= (int)CONTRA_FRAMEBUFFER_HEIGHT))
        {
            continue;
        }

        for (pixel_x = 0u; pixel_x < 8u; ++pixel_x)
        {
            const int framebuffer_x = dest_x + (int)pixel_x;
            const uint8_t color_index = contra_read_pattern_color_index(core, pattern_addr, (uint8_t)pixel_x, (uint8_t)pixel_y);
            size_t framebuffer_index;

            if ((framebuffer_x < 0) || (framebuffer_x >= (int)CONTRA_FRAMEBUFFER_WIDTH))
            {
                continue;
            }

            framebuffer_index = ((size_t)framebuffer_y * CONTRA_FRAMEBUFFER_WIDTH) + (size_t)framebuffer_x;
            core->framebuffer[framebuffer_index] =
                contra_background_palette_color_rgba(core, palette_slot, color_index);
            core->background_opaque[framebuffer_index] = (uint8_t)(color_index != 0u);
        }
    }
}

static uint16_t contra_read_sprite_ptr(uint8_t sprite_code)
{
    if (sprite_code == 0u)
    {
        return 0u;
    }

    if (sprite_code < 0x80u)
    {
        return contra_rom_read_u16(1u, (uint16_t)(contra_sprite_ptr_tbl_0_addr + (((uint16_t)sprite_code - 1u) * 2u)));
    }

    return contra_rom_read_u16(1u, (uint16_t)(contra_sprite_ptr_tbl_1_addr + (((uint16_t)sprite_code - 0x80u) * 2u)));
}

static void contra_oam_advance_addr(uint8_t *offset)
{
    *offset = (uint8_t)(*offset + 0xC4u);
}

static void contra_write_oam_entry(
    uint8_t *oam,
    uint8_t *offset,
    uint8_t *remaining,
    uint8_t sprite_y,
    uint8_t tile_index,
    uint8_t attr,
    uint8_t sprite_x
)
{
    const uint8_t write_offset = *offset;

    if ((*remaining & 0x80u) != 0u)
    {
        return;
    }

    oam[write_offset + 0u] = sprite_y;
    oam[write_offset + 1u] = tile_index;
    oam[write_offset + 2u] = attr;
    oam[write_offset + 3u] = sprite_x;
    contra_oam_advance_addr(offset);
    *remaining = (uint8_t)(*remaining - 1u);
}

static void contra_write_hud_sprites_to_oam(
    const ContraCore *core,
    uint8_t *oam,
    uint8_t *offset,
    uint8_t *remaining
)
{
    static const uint8_t hud_sprites[8] = {0x0Au, 0x0Au, 0x0Au, 0x0Au, 0x02u, 0x04u, 0x06u, 0x08u};
    static const uint8_t hud_x_offsets[8] = {0x10u, 0x1Cu, 0x28u, 0x34u, 0x10u, 0x1Cu, 0x28u, 0x34u};
    uint8_t player = core->ram[CONTRA_RAM_PLAYER_MODE];

    if (core->ram[CONTRA_RAM_DEMO_MODE] != 0u)
    {
        player = 0x01u;
    }

    for (;;)
    {
        uint8_t sprite_offset = 0x00u;
        uint8_t sprite_count;

        if ((core->ram[CONTRA_RAM_DEMO_MODE] != 0u) ||
            (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + (player & 0x01u)] != 0u))
        {
            sprite_offset = 0x04u;
            sprite_count = 0x04u;
        }
        else
        {
            sprite_count = core->ram[CONTRA_RAM_P1_NUM_LIVES + (player & 0x01u)];
            if (sprite_count >= 0x04u)
            {
                sprite_count = 0x04u;
            }
        }

        while (sprite_count-- != 0u)
        {
            const uint8_t hud_index = sprite_offset++;
            uint8_t sprite_x = hud_x_offsets[hud_index];

            if ((player & 0x01u) != 0u)
            {
                sprite_x = (uint8_t)(sprite_x + 0xB0u);
            }

            contra_write_oam_entry(
                oam,
                offset,
                remaining,
                0x10u,
                hud_sprites[hud_index],
                (uint8_t)(player & 0x01u),
                sprite_x
            );
        }

        if (player == 0u)
        {
            break;
        }
        --player;
    }
}

static void contra_write_cpu_sprite_to_oam(
    const ContraCore *core,
    size_t sprite_index,
    uint8_t *oam,
    uint8_t *offset,
    uint8_t *remaining
)
{
    const uint8_t sprite_code = core->ram[CONTRA_RAM_CPU_SPRITE_BUFFER + sprite_index];
    const uint8_t base_attr = core->ram[CONTRA_RAM_SPRITE_ATTR + sprite_index];
    const uint8_t base_y = core->ram[CONTRA_RAM_SPRITE_Y_POS + sprite_index];
    const uint8_t base_x = core->ram[CONTRA_RAM_SPRITE_X_POS + sprite_index];
    uint16_t sprite_addr;

    if ((!contra_load_rom_image()) || (sprite_code == 0u))
    {
        return;
    }

    sprite_addr = contra_read_sprite_ptr(sprite_code);
    if (sprite_addr == 0u)
    {
        return;
    }

    if (contra_rom_read_u8(1u, sprite_addr) == 0xFEu)
    {
        const uint8_t tile = contra_rom_read_u8(1u, (uint16_t)(sprite_addr + 1u));
        const uint8_t attr = (uint8_t)(contra_rom_read_u8(1u, (uint16_t)(sprite_addr + 2u)) | base_attr);
        const uint8_t sprite_y = (uint8_t)(base_y - 0x08u);
        const uint8_t sprite_x = (uint8_t)(base_x - 0x04u);

        contra_write_oam_entry(oam, offset, remaining, sprite_y, tile, attr, sprite_x);
        return;
    }

    {
        uint8_t sprite_tile_count = contra_rom_read_u8(1u, sprite_addr);
        uint16_t read_addr = (uint16_t)(sprite_addr + 1u);
        uint8_t sprite_effect = (uint8_t)(base_attr & 0xC8u);
        const uint8_t attr_mask = (base_attr & 0x04u) != 0u ? 0xFCu : 0xFFu;
        const uint8_t attr_base = (uint8_t)((base_attr & 0x04u) != 0u ? (base_attr & 0x23u) : (base_attr & 0x20u));

        while (sprite_tile_count != 0u)
        {
            uint8_t relative_y = contra_rom_read_u8(1u, read_addr++);

            if (relative_y == 0x80u)
            {
                sprite_effect &= 0xF7u;
                read_addr = contra_rom_read_u16(1u, read_addr);
                continue;
            }

            {
                const uint8_t tile = contra_rom_read_u8(1u, read_addr++);
                const uint8_t tile_attr = contra_rom_read_u8(1u, read_addr++);
                const uint8_t relative_x = contra_rom_read_u8(1u, read_addr++);
                const uint8_t adjusted_relative_y = (uint8_t)(relative_y + ((sprite_effect & 0x08u) != 0u ? 1u : 0u));
                const uint8_t sprite_y = (sprite_effect & 0x80u) != 0u
                    ? (uint8_t)(base_y + (uint8_t)(0xF0u - adjusted_relative_y))
                    : (uint8_t)(base_y + adjusted_relative_y);
                const uint8_t sprite_x = (sprite_effect & 0x40u) != 0u
                    ? (uint8_t)(base_x + (uint8_t)(0xF8u - relative_x))
                    : (uint8_t)(base_x + relative_x);
                const uint8_t attr = (uint8_t)(((tile_attr & attr_mask) | attr_base) ^ sprite_effect);

                contra_write_oam_entry(oam, offset, remaining, sprite_y, tile, attr, sprite_x);
                --sprite_tile_count;
            }
        }
    }
}

static void contra_build_oam_for_next_frame(ContraCore *core)
{
    uint8_t oam[0x100u];
    uint8_t offset = (uint8_t)(core->ram[CONTRA_RAM_OAMDMA_CPU_BUFFER_OFFSET] + 0x4Cu);
    uint8_t remaining = 0x3Fu;
    size_t sprite_index = CONTRA_CPU_SPRITE_RENDER_SLOTS;

    memset(oam, 0xF4, sizeof(oam));
    core->ram[CONTRA_RAM_OAMDMA_CPU_BUFFER_OFFSET] = offset;

    if (core->ram[CONTRA_RAM_SPRITE_LOAD_TYPE] != 0u)
    {
        contra_write_hud_sprites_to_oam(core, oam, &offset, &remaining);
    }

    while (sprite_index-- != 0u)
    {
        contra_write_cpu_sprite_to_oam(core, sprite_index, oam, &offset, &remaining);
    }

    while ((remaining & 0x80u) == 0u)
    {
        oam[offset] = 0xF4u;
        contra_oam_advance_addr(&offset);
        remaining = (uint8_t)(remaining - 1u);
    }

    memcpy(core->latched_oam, oam, sizeof(core->latched_oam));
    memcpy(&core->ram[CONTRA_RAM_OAMDMA_CPU_BUFFER], oam, sizeof(oam));
}

static bool contra_oam_sprite_visible_on_scanline(const uint8_t *oam, size_t sprite_index, int scanline)
{
    size_t earlier_index;
    unsigned visible_count = 0u;

    for (earlier_index = 0u; earlier_index < sprite_index; ++earlier_index)
    {
        const uint8_t sprite_y = oam[(earlier_index * 4u) + 0u];
        const int top = (int)sprite_y + 1;

        if ((sprite_y < 0xEFu) && (scanline >= top) && (scanline < (top + 16)))
        {
            ++visible_count;
            if (visible_count >= 8u)
            {
                return false;
            }
        }
    }

    return true;
}

static void contra_render_oam_sprite(ContraCore *core, size_t sprite_index)
{
    const size_t oam_offset = sprite_index * 4u;
    const uint8_t sprite_y = core->latched_oam[oam_offset + 0u];
    const uint8_t tile_index = core->latched_oam[oam_offset + 1u];
    const uint8_t attr = core->latched_oam[oam_offset + 2u];
    const uint8_t sprite_x = core->latched_oam[oam_offset + 3u];
    const uint16_t pattern_base = (uint16_t)(((uint16_t)(tile_index & 0x01u) << 12u) + ((uint16_t)(tile_index & 0xFEu) * 16u));
    const uint8_t palette_slot = (uint8_t)(attr & 0x03u);
    const bool priority_behind_bg = (attr & 0x20u) != 0u;
    const bool flip_horizontal = (attr & 0x40u) != 0u;
    const bool flip_vertical = (attr & 0x80u) != 0u;
    unsigned pixel_y;

    if ((sprite_y >= 0xEFu) || ((pattern_base + 31u) >= sizeof(core->ppu_pattern)))
    {
        return;
    }

    for (pixel_y = 0u; pixel_y < 16u; ++pixel_y)
    {
        const unsigned source_row = flip_vertical ? (15u - pixel_y) : pixel_y;
        const uint16_t pattern_addr = (uint16_t)(pattern_base + (((uint16_t)(source_row >> 3u)) * 16u));
        const uint8_t tile_row = (uint8_t)(source_row & 0x07u);
        const int framebuffer_y = (int)sprite_y + 1 + (int)pixel_y;
        unsigned pixel_x;

        if ((framebuffer_y < 0) ||
            (framebuffer_y >= (int)CONTRA_FRAMEBUFFER_HEIGHT) ||
            !contra_oam_sprite_visible_on_scanline(core->latched_oam, sprite_index, framebuffer_y))
        {
            continue;
        }

        for (pixel_x = 0u; pixel_x < 8u; ++pixel_x)
        {
            const uint8_t source_x = flip_horizontal ? (uint8_t)(7u - pixel_x) : (uint8_t)pixel_x;
            const uint8_t color_index = contra_read_pattern_color_index(core, pattern_addr, source_x, tile_row);
            const int framebuffer_x = (int)sprite_x + (int)pixel_x;
            size_t framebuffer_index;

            if ((color_index == 0u) ||
                (framebuffer_x < 0) ||
                (framebuffer_x >= (int)CONTRA_FRAMEBUFFER_WIDTH))
            {
                continue;
            }

            framebuffer_index = ((size_t)framebuffer_y * CONTRA_FRAMEBUFFER_WIDTH) + (size_t)framebuffer_x;
            if (priority_behind_bg && (core->background_opaque[framebuffer_index] != 0u))
            {
                continue;
            }

            if ((core->sprite_priority[framebuffer_index] != 0xFFu) &&
                (core->sprite_priority[framebuffer_index] < sprite_index))
            {
                continue;
            }

            core->framebuffer[framebuffer_index] =
                contra_sprite_palette_color_rgba(core, palette_slot, color_index);
            core->sprite_priority[framebuffer_index] = (uint8_t)sprite_index;
        }
    }
}

static void contra_render_cpu_sprites(ContraCore *core)
{
    size_t sprite_index;

    for (sprite_index = 0u; sprite_index < 64u; ++sprite_index)
    {
        contra_render_oam_sprite(core, sprite_index);
    }
}

static void contra_render_intro_background(ContraCore *core)
{
    const uint8_t base_nametable = (uint8_t)(core->latched_ppuctrl_settings & 0x01u);
    const uint8_t fine_scroll_x = core->latched_horizontal_scroll;
    const uint8_t tile_scroll_x = (uint8_t)(fine_scroll_x >> 3u);
    const uint8_t pixel_scroll_x = (uint8_t)(fine_scroll_x & 0x07u);
    /* Vertical scroll (the ending credits crawl): the nametable repeats every
       240 lines (the $2800 writes mirror onto $2000). Zero everywhere else,
       so the intro screens render as before. */
    const uint8_t fine_scroll_y = core->latched_vertical_scroll;
    const unsigned tile_scroll_y = (unsigned)(fine_scroll_y >> 3u);
    const unsigned pixel_scroll_y = (unsigned)(fine_scroll_y & 0x07u);
    const unsigned visible_rows = (pixel_scroll_y != 0u) ? 31u : 30u;
    unsigned tile_y;

    for (tile_y = 0u; tile_y < visible_rows; ++tile_y)
    {
        const unsigned src_tile_y = (tile_scroll_y + tile_y) % 30u;
        unsigned screen_tile_x;

        for (screen_tile_x = 0u; screen_tile_x < 33u; ++screen_tile_x)
        {
            const uint8_t world_tile_x = (uint8_t)(tile_scroll_x + (uint8_t)screen_tile_x);
            const uint8_t nametable_index = (uint8_t)(base_nametable ^ ((world_tile_x >> 5u) & 0x01u));
            const uint8_t coarse_x = (uint8_t)(world_tile_x & 0x1Fu);
            const uint16_t tile_offset = (uint16_t)(((uint16_t)src_tile_y * 32u) + (uint16_t)coarse_x);
            const uint8_t pattern_index = contra_read_nametable_byte(core, nametable_index, tile_offset);
            const uint8_t palette_slot = contra_read_nametable_palette_slot(core, nametable_index, coarse_x, (uint8_t)src_tile_y);
            const int dest_x = ((int)screen_tile_x * 8) - (int)pixel_scroll_x;

            contra_draw_background_tile(core, dest_x, ((int)tile_y * 8) - (int)pixel_scroll_y, pattern_index, palette_slot);
        }
    }
}

static void contra_render_horizontal_level_background_scrolled(ContraCore *core)
{
    contra_render_intro_background(core);
}

/* Vertical-level scrolled background. The original camera climbs UP the level: as the
   player ascends, VERTICAL_SCROLL counts DOWN and the *next* screen scrolls in from
   the TOP. LEVEL_SCREEN_SCROLL_OFFSET counts the opposite way (0->0xf0 per screen), so
   the visible window's top sits `240 - scroll_off` pixels into a two-screen column
   whose UPPER half is the next screen (screen_number + 1) and lower half the current
   screen. (contra_vertical_collision_screen_row uses the identical mapping so floors
   stay aligned.) */
static void contra_render_vertical_level_background_scrolled(ContraCore *core)
{
    const uint8_t *const ram = core->ram;
    const uint16_t supertile_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u));
    const uint16_t palette_ptr = (uint16_t)(
        (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] |
        ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] << 8u));
    const uint8_t screen_number = core->latched_level_screen_number;
    const int top_combined = 240 - (int)core->latched_level_screen_scroll_offset;
    uint8_t screen_supertiles[CONTRA_LEVEL_SCREEN_SUPERTILES_SIZE];
    int cached_screen = -1;
    int c;

    for (c = (top_combined & ~7); ; c += 8)
    {
        const int dest_y = c - top_combined;
        const uint8_t data_screen = (c < 240) ? (uint8_t)(screen_number + 1u) : screen_number;
        const uint16_t row_px = (c < 240) ? (uint16_t)c : (uint16_t)(c - 240);
        const uint8_t tile_row = (uint8_t)(row_px >> 3u);       /* 0..29 within the screen */
        const size_t supertile_row = (size_t)(tile_row >> 2u);
        const uint8_t tile_y_in_supertile = (uint8_t)(tile_row & 0x03u);
        size_t tile_x;

        if (dest_y >= (int)CONTRA_FRAMEBUFFER_HEIGHT)
        {
            break;
        }

        if ((int)data_screen != cached_screen)
        {
            memset(screen_supertiles, 0, sizeof(screen_supertiles));
            contra_decode_level_screen_supertiles(core, data_screen, screen_supertiles, 0u);
            cached_screen = (int)data_screen;
        }

        for (tile_x = 0u; tile_x < 32u; ++tile_x)
        {
            const size_t supertile_column = tile_x / 4u;
            const size_t supertile_offset = (supertile_row * 8u) + supertile_column;
            const uint8_t supertile_index = screen_supertiles[supertile_offset];
            const size_t supertile_data_addr = (size_t)supertile_index * 16u;
            const uint8_t tile_in_supertile =
                (uint8_t)((tile_y_in_supertile << 2u) | (tile_x & 0x03u));
            const uint8_t pattern_index =
                contra_rom_read_u8(3u, (uint16_t)(supertile_ptr + supertile_data_addr + tile_in_supertile));
            const uint8_t supertile_palette =
                contra_rom_read_u8(3u, (uint16_t)(palette_ptr + supertile_index));
            const uint8_t palette_shift =
                (uint8_t)(((tile_y_in_supertile & 0x02u) << 1u) | (tile_x & 0x02u));
            const uint8_t palette_slot = (uint8_t)((supertile_palette >> palette_shift) & 0x03u);

            contra_draw_background_tile(core, (int)(tile_x * 8u), dest_y, pattern_index, palette_slot);
        }
    }
}

static void contra_render_level_background(ContraCore *core)
{
    const uint8_t *const ram = core->ram;
    const size_t visible_super_rows = (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) ? 8u : 7u;
    const size_t visible_tile_rows = visible_super_rows * 4u;
    const size_t origin_y = (CONTRA_FRAMEBUFFER_HEIGHT > (visible_tile_rows * 8u))
        ? (CONTRA_FRAMEBUFFER_HEIGHT - (visible_tile_rows * 8u))
        : 0u;
    size_t visible_tile_columns = 32u;
    size_t tile_y;

    if (!contra_load_rom_image())
    {
        return;
    }

    /* Boss room: recompose the flat mechanical-wall super-tile layout each frame.
       At boss entry the 3 super-tile pointers are repointed to the boss tables
       ($9013/$b57a/$bd7a) and the wall layout decoded, but the indoor column-advance
       (advance_horizontal_level_ppu_column, LEVEL_SCREEN_NUMBER+2 = out of range for
       the 2-entry boss screen table) clears level_screen_supertiles afterward, so we
       re-decode it here while the boss room is static. */
    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] & 0x80u) != 0u)
    {
        contra_decode_level_screen_supertiles(
            core, (uint8_t)(ram[CONTRA_RAM_CURRENT_LEVEL] >> 1u),
            core->level_screen_supertiles, 0u);
    }

    if ((ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u))
    {
        contra_render_horizontal_level_background_scrolled(core);
        return;
    }

    if ((ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] != 0u) &&
        (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] >= 0x04u))
    {
        contra_render_vertical_level_background_scrolled(core);
        return;
    }

    if ((ram[CONTRA_RAM_LEVEL_SCROLLING_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_LOCATION_TYPE] == 0u) &&
        (ram[CONTRA_RAM_LEVEL_ROUTINE_INDEX] == 0x03u) &&
        (ram[CONTRA_RAM_PPU_WRITE_ADDRESS_HIGH_BYTE] == 0x20u))
    {
        visible_tile_columns = ram[CONTRA_RAM_PPU_WRITE_TILE_OFFSET];
        if (visible_tile_columns > 32u)
        {
            visible_tile_columns = 32u;
        }
    }

    for (tile_y = 0u; tile_y < visible_tile_rows; ++tile_y)
    {
        size_t tile_x;

        for (tile_x = 0u; tile_x < visible_tile_columns; ++tile_x)
        {
            const size_t supertile_column = tile_x / 4u;
            const size_t supertile_row = tile_y / 4u;
            const size_t supertile_offset = (supertile_row * 8u) + supertile_column;
            const uint8_t supertile_index = core->level_screen_supertiles[supertile_offset];
            const size_t supertile_data_addr = (size_t)supertile_index * 16u;
            const uint16_t supertile_ptr = (uint16_t)(
                (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR] |
                ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_DATA_PTR + 1u] << 8u)
            );
            const uint16_t palette_ptr = (uint16_t)(
                (uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA] |
                ((uint16_t)ram[CONTRA_RAM_LEVEL_SUPERTILE_PALETTE_DATA + 1u] << 8u)
            );
            const uint8_t tile_in_supertile = (uint8_t)(((tile_y & 0x03u) << 2u) | (tile_x & 0x03u));
            const uint8_t pattern_index = contra_rom_read_u8(3u, (uint16_t)(supertile_ptr + supertile_data_addr + tile_in_supertile));
            const uint8_t supertile_palette = contra_rom_read_u8(3u, (uint16_t)(palette_ptr + supertile_index));
            const uint8_t palette_shift = (uint8_t)(((tile_y & 0x02u) << 1u) | (tile_x & 0x02u));
            const uint8_t palette_slot = (uint8_t)((supertile_palette >> palette_shift) & 0x03u);

            contra_draw_background_tile(
                core,
                (int)(tile_x * 8u),
                (int)(origin_y + (tile_y * 8u)),
                pattern_index,
                palette_slot
            );
        }
    }
}

static void contra_zero_out_nametables(ContraCore *core)
{
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_MODE] = 0x00u;
    core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] = 0x00u;
    core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER] = 0x00u;
    memset(&core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER], 0, CONTRA_CPU_GRAPHICS_BUFFER_SIZE);
    memset(core->ppu_nametable, 0, sizeof(core->ppu_nametable));

    /* zero_out_nametables falls through to write_graphic_data_to_ppu (bank7),
       which resets both scroll offsets so the freshly drawn screen (game over /
       intro / title) sits at scroll origin instead of the gameplay scroll. */
    core->ram[CONTRA_RAM_VERTICAL_SCROLL] = 0x00u;
    core->ram[CONTRA_RAM_HORIZONTAL_SCROLL] = 0x00u;
}

static void contra_load_intro_graphics(ContraCore *core)
{
    contra_init_apu_channels(core);
    contra_clear_memory_3(core);
    core->ram[CONTRA_RAM_PPUMASK_SETTINGS] = 0x1Eu;
    core->ram[CONTRA_RAM_SPRITE_LOAD_TYPE] = 0x00u;
    core->ram[CONTRA_RAM_DEMO_MODE] = 0x01u;
    contra_load_graphic_data_list(core, 11u);
    contra_load_bank_6_write_text_palette_to_mem(core, 0x06u);
}

static void contra_load_level_intro_screen_graphics(ContraCore *core)
{
    contra_load_graphic_data_list(core, 10u);
}

static void contra_load_bank_6_write_text_palette_to_mem(ContraCore *core, uint8_t text_code)
{
    const bool blank_text = (text_code & 0x80u) != 0u;
    const uint8_t table_index = (uint8_t)(text_code & 0x3Fu);
    uint16_t read_addr;
    uint8_t blank_delay = 0x02u;

    if (!contra_load_rom_image())
    {
        return;
    }

    read_addr = contra_rom_read_u16(
        6u,
        (uint16_t)(contra_short_text_pointer_table_addr + ((uint16_t)table_index * 2u))
    );

    contra_write_cpu_graphics_buffer_byte(core, 0x01u);

    while (core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET] < CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
    {
        uint8_t value = contra_rom_read_u8(6u, read_addr++);

        if (value == 0xFFu)
        {
            return;
        }

        if (value == 0xFEu)
        {
            contra_write_cpu_graphics_buffer_byte(core, 0xFFu);
            return;
        }

        if (value == 0xFDu)
        {
            contra_write_cpu_graphics_buffer_byte(core, 0xFFu);
            blank_delay = 0x02u;
            contra_write_cpu_graphics_buffer_byte(core, 0x01u);
            continue;
        }

        if (blank_text)
        {
            if (blank_delay == 0u)
            {
                value = 0x00u;
            }
            else
            {
                blank_delay = (uint8_t)(blank_delay - 1u);
            }
        }

        contra_write_cpu_graphics_buffer_byte(core, value);
    }
}

static void contra_play_sound(ContraCore *core, uint8_t sound_code)
{
    (void)core;
    (void)sound_code;
}

static void contra_init_apu_channels(ContraCore *core)
{
    (void)core;
}

static void contra_patch_cpu_graphics_buffer_byte(ContraCore *core, uint8_t offset, uint8_t value)
{
    if (offset >= CONTRA_CPU_GRAPHICS_BUFFER_SIZE)
    {
        return;
    }

    core->ram[CONTRA_RAM_CPU_GRAPHICS_BUFFER + offset] = value;
}

static void contra_patch_cpu_graphics_buffer_from_end(ContraCore *core, uint8_t back_offset, uint8_t value)
{
    const uint8_t offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];

    if ((offset < back_offset) || (offset > CONTRA_CPU_GRAPHICS_BUFFER_SIZE))
    {
        return;
    }

    contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(offset - back_offset), value);
}

static uint8_t contra_calculate_score_digit(uint8_t *low, uint8_t *high)
{
    uint8_t digit = 0x00u;
    uint8_t shift_count = 0x10u;
    bool carry = ((*low & 0x80u) != 0u);

    *low = (uint8_t)(*low << 1u);

    {
        const bool next_carry = ((*high & 0x80u) != 0u);
        *high = (uint8_t)((*high << 1u) | (carry ? 0x01u : 0x00u));
        carry = next_carry;
    }

    do
    {
        const bool digit_carry = ((digit & 0x80u) != 0u);

        digit = (uint8_t)((digit << 1u) | (carry ? 0x01u : 0x00u));
        carry = digit_carry;

        if (digit >= 0x0Au)
        {
            digit = (uint8_t)(digit - 0x0Au);
            carry = true;
        }

        {
            const bool low_carry = ((*low & 0x80u) != 0u);
            *low = (uint8_t)((*low << 1u) | (carry ? 0x01u : 0x00u));
            carry = low_carry;
        }

        {
            const bool high_carry = ((*high & 0x80u) != 0u);
            *high = (uint8_t)((*high << 1u) | (carry ? 0x01u : 0x00u));
            carry = high_carry;
        }
    } while (--shift_count != 0u);

    return digit;
}

static void contra_draw_stage_and_level_name(ContraCore *core)
{
    const uint8_t current_level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Cu);
    contra_patch_cpu_graphics_buffer_from_end(core, 0x02u, (uint8_t)(current_level + 0x31u));
    contra_load_bank_6_write_text_palette_to_mem(core, (uint8_t)(current_level + 0x11u));
}

static void contra_draw_player_num_lives(ContraCore *core)
{
    const uint8_t player_index = (uint8_t)(core->ram[CONTRA_RAM_DRAW_PLAYER_INDEX] & 0x01u);
    uint8_t remaining_lives;
    uint8_t tens_digit = 0x00u;
    uint8_t ones_digit;

    contra_load_bank_6_write_text_palette_to_mem(core, (uint8_t)(0x07u + player_index));

    remaining_lives = (uint8_t)(
        (core->ram[CONTRA_RAM_P1_GAME_OVER_STATUS + player_index] ^ 0x01u) +
        core->ram[CONTRA_RAM_P1_NUM_LIVES + player_index]
    );

    if (remaining_lives == 0u)
    {
        contra_load_bank_6_write_text_palette_to_mem(core, (uint8_t)(0x0Fu + player_index));
        return;
    }

    if ((remaining_lives & 0x80u) != 0u)
    {
        remaining_lives = 0x00u;
    }

    while (remaining_lives >= 0x0Au)
    {
        remaining_lives = (uint8_t)(remaining_lives - 0x0Au);
        ++tens_digit;

        if (tens_digit >= 0x0Au)
        {
            tens_digit = 0x09u;
            remaining_lives = 0x09u;
            break;
        }
    }

    ones_digit = (uint8_t)(remaining_lives | 0x30u);
    if ((tens_digit == 0u) && (ones_digit == 0x30u))
    {
        return;
    }

    contra_patch_cpu_graphics_buffer_from_end(core, 0x02u, ones_digit);
    if (tens_digit != 0u)
    {
        contra_patch_cpu_graphics_buffer_from_end(core, 0x03u, (uint8_t)(tens_digit | 0x30u));
    }
}

static void contra_draw_the_scores(ContraCore *core)
{
    uint8_t score_low;
    uint8_t score_high;
    uint8_t original_offset;
    uint8_t write_offset;
    uint8_t digits_remaining;
    uint8_t digit = 0x00u;
    bool zero_score;

    contra_load_bank_6_write_text_palette_to_mem(core, 0x09u);
    score_low = core->ram[CONTRA_RAM_HIGH_SCORE_LOW];
    score_high = core->ram[CONTRA_RAM_HIGH_SCORE_HIGH];

    if ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x10u) == 0u)
    {
        original_offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];
        write_offset = original_offset;
        digits_remaining = 0x05u;

        do
        {
            digit = contra_calculate_score_digit(&score_low, &score_high);
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(write_offset - 0x04u), (uint8_t)(digit | 0x30u));
            --write_offset;

            if ((uint8_t)(score_low | score_high) == 0u)
            {
                break;
            }

            --digits_remaining;
        } while (digits_remaining != 0u);

        zero_score = (bool)((digits_remaining == 0x05u) && (digit == 0u));
        if (zero_score)
        {
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x04u), 0x00u);
        }

        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x03u), 0x30u);
        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x02u), 0x30u);
    }

    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Au);
    score_low = core->ram[CONTRA_RAM_PLAYER_1_SCORE_LOW];
    score_high = core->ram[CONTRA_RAM_PLAYER_1_SCORE_HIGH];

    if ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x10u) == 0u)
    {
        original_offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];
        write_offset = original_offset;
        digits_remaining = 0x05u;

        do
        {
            digit = contra_calculate_score_digit(&score_low, &score_high);
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(write_offset - 0x04u), (uint8_t)(digit | 0x30u));
            --write_offset;

            if ((uint8_t)(score_low | score_high) == 0u)
            {
                break;
            }

            --digits_remaining;
        } while (digits_remaining != 0u);

        zero_score = (bool)((digits_remaining == 0x05u) && (digit == 0u));
        if (zero_score)
        {
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x04u), 0x00u);
        }

        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x03u), 0x30u);
        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x02u), 0x30u);
    }

    if (core->ram[CONTRA_RAM_PLAYER_MODE] == 0u)
    {
        return;
    }

    contra_load_bank_6_write_text_palette_to_mem(core, 0x0Bu);
    score_low = core->ram[CONTRA_RAM_PLAYER_2_SCORE_LOW];
    score_high = core->ram[CONTRA_RAM_PLAYER_2_SCORE_HIGH];

    if ((core->ram[CONTRA_RAM_FRAME_COUNTER] & 0x10u) == 0u)
    {
        original_offset = core->ram[CONTRA_RAM_GRAPHICS_BUFFER_OFFSET];
        write_offset = original_offset;
        digits_remaining = 0x05u;

        do
        {
            digit = contra_calculate_score_digit(&score_low, &score_high);
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(write_offset - 0x04u), (uint8_t)(digit | 0x30u));
            --write_offset;

            if ((uint8_t)(score_low | score_high) == 0u)
            {
                break;
            }

            --digits_remaining;
        } while (digits_remaining != 0u);

        zero_score = (bool)((digits_remaining == 0x05u) && (digit == 0u));
        if (zero_score)
        {
            contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x04u), 0x00u);
        }

        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x03u), 0x30u);
        contra_patch_cpu_graphics_buffer_byte(core, (uint8_t)(original_offset - 0x02u), 0x30u);
    }
}

static void contra_load_level_graphics(ContraCore *core)
{
    uint8_t level = core->ram[CONTRA_RAM_CURRENT_LEVEL];

    if (level > 7u)
    {
        level = 7u;
    }

    contra_load_graphic_data_list(core, level);
}
