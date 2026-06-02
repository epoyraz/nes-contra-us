#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ExpectedTraceRow
{
    const char *scenario;
    const char *name;
    unsigned frame;
    unsigned level;
    unsigned location_type;
    unsigned screen;
    unsigned demo_end;
    unsigned indoor_clear;
    unsigned wall_core_remaining;
} ExpectedTraceRow;

static const ExpectedTraceRow expected_rows[] = {
    {"attract_level1_demo", "level1-title", 180u, 0u, 0u, 0u, 0u, 0u, 0u},
    {"attract_level1_demo", "level1-gameplay-start", 900u, 0u, 0u, 0u, 0u, 0u, 0u},
    {"attract_level1_demo", "level1-scroll-mid", 1200u, 0u, 0u, 1u, 0u, 0u, 0u},
    {"attract_level1_demo", "level1-enemy-window", 1500u, 0u, 0u, 2u, 0u, 0u, 0u},
    {"attract_level2_demo", "level2-attract-first-room", 3015u, 1u, 1u, 0u, 0u, 0u, 0u},
    {"attract_level2_demo", "level2-wall-core-loaded", 3016u, 1u, 1u, 0u, 0u, 0u, 1u},
    {"attract_level2_demo", "level2-wall-core-destroyed", 3502u, 1u, 1u, 0u, 0u, 0u, 0u},
    {"attract_level2_demo", "level2-room-cleared", 3526u, 1u, 1u, 0u, 0u, 1u, 0u},
    {"attract_level2_demo", "level2-after-room-1", 3770u, 1u, 1u, 1u, 0u, 0u, 1u},
    {"attract_level2_demo", "level2-demo-finished", 4632u, 1u, 1u, 1u, 0u, 0u, 1u},
    {"level2_room_chain", "level2-first-room", 4935u, 1u, 1u, 0u, 0u, 0u, 1u},
    {"level2_room_chain", "level2-after-room-1", 5071u, 1u, 1u, 1u, 0u, 0u, 1u},
    {"level2_room_chain", "level2-after-room-4", 5625u, 1u, 1u, 4u, 0u, 0u, 1u},
    {"level2_room_chain", "level2-boss-state", 5809u, 1u, 128u, 5u, 0u, 0u, 1u}
};

static bool contains_string_field(const char *line, const char *key, const char *value)
{
    char needle[256];

    if (snprintf(needle, sizeof(needle), "\"%s\":\"%s\"", key, value) >= (int)sizeof(needle))
    {
        return false;
    }

    return strstr(line, needle) != NULL;
}

static bool contains_unsigned_field(const char *line, const char *key, unsigned value)
{
    char needle[128];

    if (snprintf(needle, sizeof(needle), "\"%s\":%u", key, value) >= (int)sizeof(needle))
    {
        return false;
    }

    return strstr(line, needle) != NULL;
}

static bool is_hex_digit(char value)
{
    return ((value >= '0') && (value <= '9')) ||
        ((value >= 'A') && (value <= 'F'));
}

static bool contains_hash_field(const char *line, const char *key)
{
    char needle[64];
    const char *value;
    size_t index;

    if (snprintf(needle, sizeof(needle), "\"%s\":\"", key) >= (int)sizeof(needle))
    {
        return false;
    }

    value = strstr(line, needle);
    if (value == NULL)
    {
        return false;
    }

    value += strlen(needle);
    for (index = 0u; index < 8u; ++index)
    {
        if (!is_hex_digit(value[index]))
        {
            return false;
        }
    }

    return value[8u] == '"';
}

static bool validate_line(const char *line, const ExpectedTraceRow *expected, unsigned line_number)
{
    static const char *hash_fields[] = {
        "ram_hash",
        "nametable_hash",
        "palette_hash",
        "enemy_hash",
        "framebuffer_hash"
    };
    size_t index;
    bool valid = true;

    if (!contains_string_field(line, "scenario", expected->scenario))
    {
        printf("FAIL Mesen trace row %u scenario mismatch\n", line_number);
        valid = false;
    }

    if (!contains_string_field(line, "name", expected->name))
    {
        printf("FAIL Mesen trace row %u name mismatch\n", line_number);
        valid = false;
    }

    if ((expected->frame != 0u) && !contains_unsigned_field(line, "frame", expected->frame))
    {
        printf("FAIL Mesen trace row %u frame mismatch\n", line_number);
        valid = false;
    }

    if (!contains_unsigned_field(line, "level", expected->level))
    {
        printf("FAIL Mesen trace row %u level mismatch\n", line_number);
        valid = false;
    }

    if (!contains_unsigned_field(line, "location_type", expected->location_type))
    {
        printf("FAIL Mesen trace row %u location_type mismatch\n", line_number);
        valid = false;
    }

    if (!contains_unsigned_field(line, "screen", expected->screen))
    {
        printf("FAIL Mesen trace row %u screen mismatch\n", line_number);
        valid = false;
    }

    if (!contains_unsigned_field(line, "demo_end", expected->demo_end))
    {
        printf("FAIL Mesen trace row %u demo_end mismatch\n", line_number);
        valid = false;
    }

    if (!contains_unsigned_field(line, "indoor_clear", expected->indoor_clear))
    {
        printf("FAIL Mesen trace row %u indoor_clear mismatch\n", line_number);
        valid = false;
    }

    if (!contains_unsigned_field(line, "wall_core_remaining", expected->wall_core_remaining))
    {
        printf("FAIL Mesen trace row %u wall_core_remaining mismatch\n", line_number);
        valid = false;
    }

    for (index = 0u; index < (sizeof(hash_fields) / sizeof(hash_fields[0])); ++index)
    {
        if (!contains_hash_field(line, hash_fields[index]))
        {
            printf("FAIL Mesen trace row %u missing valid %s\n", line_number, hash_fields[index]);
            valid = false;
        }
    }

    return valid;
}

int main(int argc, char **argv)
{
    FILE *file;
    char line[2048];
    unsigned line_number = 0u;
    unsigned failures = 0u;

    if (argc != 2)
    {
        printf("usage: %s MESEN_TRACE_JSONL\n", argv[0]);
        return 2;
    }

    file = fopen(argv[1], "r");
    if (file == NULL)
    {
        printf("FAIL could not open Mesen checkpoint trace %s\n", argv[1]);
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (line_number >= (sizeof(expected_rows) / sizeof(expected_rows[0])))
        {
            printf("FAIL Mesen trace has extra row %u\n", line_number + 1u);
            ++failures;
            break;
        }

        if (!validate_line(line, &expected_rows[line_number], line_number + 1u))
        {
            ++failures;
        }

        ++line_number;
    }

    if (ferror(file) != 0)
    {
        printf("FAIL could not read Mesen checkpoint trace %s\n", argv[1]);
        ++failures;
    }

    fclose(file);

    if (line_number != (sizeof(expected_rows) / sizeof(expected_rows[0])))
    {
        printf("FAIL Mesen trace row count expected=%u actual=%u\n",
               (unsigned)(sizeof(expected_rows) / sizeof(expected_rows[0])),
               line_number);
        ++failures;
    }

    if (failures != 0u)
    {
        return 1;
    }

    printf("PASS Mesen checkpoint trace export rows=%u\n", line_number);
    return 0;
}
