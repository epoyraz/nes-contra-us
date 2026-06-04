#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct TraceRow
{
    char scenario[64];
    char name[64];
    unsigned frame;
    unsigned game_routine;
    unsigned level_routine;
    unsigned level;
    unsigned location_type;
    unsigned screen;
    unsigned scroll_offset;
    unsigned player_state;
    unsigned player_x;
    unsigned player_y;
    unsigned lives;
    unsigned game_over;
    unsigned demo_end;
    unsigned indoor_clear;
    unsigned wall_core_remaining;
} TraceRow;

typedef struct TraceRows
{
    TraceRow rows[8];
    size_t count;
} TraceRows;

static bool parse_row(const char *line, TraceRow *row)
{
    char ram_hash[16];
    char nametable_hash[16];
    char palette_hash[16];
    char enemy_hash[16];
    char framebuffer_hash[16];

    memset(row, 0, sizeof(*row));
    return sscanf(
        line,
        "{\"scenario\":\"%63[^\"]\",\"name\":\"%63[^\"]\",\"frame\":%u,"
        "\"game_routine\":%u,\"level_routine\":%u,\"level\":%u,"
        "\"location_type\":%u,\"screen\":%u,\"scroll_offset\":%u,"
        "\"player_state\":%u,\"player_x\":%u,\"player_y\":%u,"
        "\"lives\":%u,\"game_over\":%u,"
        "\"demo_end\":%u,\"indoor_clear\":%u,\"wall_core_remaining\":%u,"
        "\"ram_hash\":\"%15[^\"]\",\"nametable_hash\":\"%15[^\"]\","
        "\"palette_hash\":\"%15[^\"]\",\"enemy_hash\":\"%15[^\"]\","
        "\"framebuffer_hash\":\"%15[^\"]\"}",
        row->scenario,
        row->name,
        &row->frame,
        &row->game_routine,
        &row->level_routine,
        &row->level,
        &row->location_type,
        &row->screen,
        &row->scroll_offset,
        &row->player_state,
        &row->player_x,
        &row->player_y,
        &row->lives,
        &row->game_over,
        &row->demo_end,
        &row->indoor_clear,
        &row->wall_core_remaining,
        ram_hash,
        nametable_hash,
        palette_hash,
        enemy_hash,
        framebuffer_hash
    ) == 22;
}

static bool load_room_chain_rows(const char *path, TraceRows *rows)
{
    FILE *file;
    char line[2048];
    unsigned line_number = 0u;

    memset(rows, 0, sizeof(*rows));
    file = fopen(path, "r");
    if (file == NULL)
    {
        printf("FAIL could not open trace %s\n", path);
        return false;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        TraceRow row;
        ++line_number;

        if (!parse_row(line, &row))
        {
            printf("FAIL could not parse trace row %u from %s\n", line_number, path);
            fclose(file);
            return false;
        }

        if (strcmp(row.scenario, "level2_room_chain") != 0)
        {
            continue;
        }

        if (rows->count >= (sizeof(rows->rows) / sizeof(rows->rows[0])))
        {
            printf("FAIL too many level2_room_chain rows in %s\n", path);
            fclose(file);
            return false;
        }

        rows->rows[rows->count] = row;
        ++rows->count;
    }

    if (ferror(file) != 0)
    {
        printf("FAIL could not read trace %s\n", path);
        fclose(file);
        return false;
    }

    fclose(file);
    return true;
}

static const TraceRow *find_row_by_name(const TraceRows *rows, const char *name)
{
    size_t index;

    for (index = 0u; index < rows->count; ++index)
    {
        if (strcmp(rows->rows[index].name, name) == 0)
        {
            return &rows->rows[index];
        }
    }

    return NULL;
}

static unsigned abs_diff(unsigned left, unsigned right)
{
    return (left > right) ? (left - right) : (right - left);
}

static bool compare_exact_field(const char *name, const char *row, unsigned expected, unsigned actual)
{
    if (expected == actual)
    {
        return true;
    }

    printf("FAIL level2_room_chain %s %s expected=%u actual=%u\n", row, name, expected, actual);
    return false;
}

static bool compare_tolerance_field(
    const char *name,
    const char *row,
    unsigned expected,
    unsigned actual,
    unsigned tolerance
)
{
    if (abs_diff(expected, actual) <= tolerance)
    {
        return true;
    }

    printf(
        "FAIL level2_room_chain %s %s expected=%u actual=%u tolerance=%u\n",
        row,
        name,
        expected,
        actual,
        tolerance
    );
    return false;
}

static bool compare_row(
    const TraceRow *expected,
    const TraceRow *actual,
    unsigned expected_base_frame,
    unsigned actual_base_frame
)
{
    bool valid = true;
    const unsigned expected_delta = expected->frame - expected_base_frame;
    const unsigned actual_delta = actual->frame - actual_base_frame;

    valid = compare_tolerance_field("relative_frame", expected->name, expected_delta, actual_delta, 220u) && valid;
    valid = compare_exact_field("game_routine", expected->name, expected->game_routine, actual->game_routine) && valid;
    valid = compare_exact_field("level_routine", expected->name, expected->level_routine, actual->level_routine) && valid;
    valid = compare_exact_field("level", expected->name, expected->level, actual->level) && valid;
    valid = compare_exact_field("location_type", expected->name, expected->location_type, actual->location_type) && valid;
    valid = compare_exact_field("screen", expected->name, expected->screen, actual->screen) && valid;
    valid = compare_exact_field("scroll_offset", expected->name, expected->scroll_offset, actual->scroll_offset) && valid;
    valid = compare_exact_field("player_state", expected->name, expected->player_state, actual->player_state) && valid;
    valid = compare_tolerance_field("player_x", expected->name, expected->player_x, actual->player_x, 48u) && valid;
    valid = compare_tolerance_field("player_y", expected->name, expected->player_y, actual->player_y, 80u) && valid;
    valid = compare_exact_field("lives", expected->name, expected->lives, actual->lives) && valid;
    valid = compare_exact_field("game_over", expected->name, expected->game_over, actual->game_over) && valid;
    valid = compare_exact_field("demo_end", expected->name, expected->demo_end, actual->demo_end) && valid;
    valid = compare_exact_field("indoor_clear", expected->name, expected->indoor_clear, actual->indoor_clear) && valid;
    valid = compare_exact_field("wall_core_remaining", expected->name, expected->wall_core_remaining, actual->wall_core_remaining) && valid;
    return valid;
}

int main(int argc, char **argv)
{
    static const char *required_rows[] = {
        "level2-first-room",
        "level2-after-room-1",
        "level2-after-room-4",
        "level2-boss-state",
        "level2-boss-defeated"
    };
    TraceRows expected;
    TraceRows actual;
    size_t index;
    unsigned failures = 0u;
    unsigned expected_base_frame;
    unsigned actual_base_frame;

    if (argc != 3)
    {
        printf("usage: %s ORIGINAL_TRACE_JSONL NATIVE_TRACE_JSONL\n", argv[0]);
        return 2;
    }

    if (!load_room_chain_rows(argv[1], &expected) || !load_room_chain_rows(argv[2], &actual))
    {
        return 1;
    }

    if (expected.count != (sizeof(required_rows) / sizeof(required_rows[0])))
    {
        printf("FAIL original level2_room_chain row count expected=%u actual=%u\n",
               (unsigned)(sizeof(required_rows) / sizeof(required_rows[0])),
               (unsigned)expected.count);
        return 1;
    }

    if (actual.count < expected.count)
    {
        printf("FAIL native level2_room_chain row count expected_at_least=%u actual=%u\n",
               (unsigned)expected.count,
               (unsigned)actual.count);
        return 1;
    }

    expected_base_frame = expected.rows[0].frame;
    actual_base_frame = actual.rows[0].frame;

    for (index = 0u; index < (sizeof(required_rows) / sizeof(required_rows[0])); ++index)
    {
        const TraceRow *const expected_row = find_row_by_name(&expected, required_rows[index]);
        const TraceRow *const actual_row = find_row_by_name(&actual, required_rows[index]);

        if (expected_row == NULL)
        {
            printf("FAIL original level2_room_chain missing %s\n", required_rows[index]);
            ++failures;
            continue;
        }

        if (actual_row == NULL)
        {
            printf("FAIL native level2_room_chain missing %s\n", required_rows[index]);
            ++failures;
            continue;
        }

        if (!compare_row(expected_row, actual_row, expected_base_frame, actual_base_frame))
        {
            ++failures;
        }
    }

    if (failures != 0u)
    {
        return 1;
    }

    return 0;
}
