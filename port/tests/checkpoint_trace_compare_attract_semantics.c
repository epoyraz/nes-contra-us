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
    TraceRow rows[12];
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

static bool is_attract_row(const TraceRow *row)
{
    return (strcmp(row->scenario, "attract_level1_demo") == 0) ||
        (strcmp(row->scenario, "attract_level2_demo") == 0);
}

static bool load_attract_rows(const char *path, TraceRows *rows)
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

        if (!is_attract_row(&row))
        {
            continue;
        }

        if (rows->count >= (sizeof(rows->rows) / sizeof(rows->rows[0])))
        {
            printf("FAIL too many attract rows in %s\n", path);
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

static unsigned abs_diff(unsigned left, unsigned right)
{
    return (left > right) ? (left - right) : (right - left);
}

static bool compare_exact_field(const char *name, unsigned row, unsigned expected, unsigned actual)
{
    if (expected == actual)
    {
        return true;
    }

    printf("FAIL attract row %u %s expected=%u actual=%u\n", row, name, expected, actual);
    return false;
}

static bool compare_tolerance_field(
    const char *name,
    unsigned row,
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
        "FAIL attract row %u %s expected=%u actual=%u tolerance=%u\n",
        row,
        name,
        expected,
        actual,
        tolerance
    );
    return false;
}

static bool compare_row(const TraceRow *expected, const TraceRow *actual, unsigned row_number)
{
    bool valid = true;
    const unsigned frame_tolerance =
        (strcmp(expected->scenario, "attract_level2_demo") == 0) ? 300u : 0u;
    const unsigned player_x_tolerance =
        (strcmp(expected->scenario, "attract_level2_demo") == 0) ? 48u : 20u;
    const unsigned player_y_tolerance =
        (strcmp(expected->scenario, "attract_level2_demo") == 0) ? 16u : 0u;

    if (strcmp(expected->scenario, actual->scenario) != 0)
    {
        printf("FAIL attract row %u scenario expected=%s actual=%s\n",
               row_number,
               expected->scenario,
               actual->scenario);
        valid = false;
    }

    if (strcmp(expected->name, actual->name) != 0)
    {
        printf("FAIL attract row %u name expected=%s actual=%s\n",
               row_number,
               expected->name,
               actual->name);
        valid = false;
    }

    valid = compare_tolerance_field("frame", row_number, expected->frame, actual->frame, frame_tolerance) && valid;
    valid = compare_exact_field("game_routine", row_number, expected->game_routine, actual->game_routine) && valid;
    valid = compare_exact_field("level_routine", row_number, expected->level_routine, actual->level_routine) && valid;
    valid = compare_exact_field("level", row_number, expected->level, actual->level) && valid;
    valid = compare_exact_field("location_type", row_number, expected->location_type, actual->location_type) && valid;
    valid = compare_exact_field("screen", row_number, expected->screen, actual->screen) && valid;
    valid = compare_tolerance_field("scroll_offset", row_number, expected->scroll_offset, actual->scroll_offset, 20u) && valid;
    valid = compare_exact_field("player_state", row_number, expected->player_state, actual->player_state) && valid;
    valid = compare_tolerance_field("player_x", row_number, expected->player_x, actual->player_x, player_x_tolerance) && valid;
    valid = compare_tolerance_field("player_y", row_number, expected->player_y, actual->player_y, player_y_tolerance) && valid;
    valid = compare_exact_field("lives", row_number, expected->lives, actual->lives) && valid;
    valid = compare_exact_field("game_over", row_number, expected->game_over, actual->game_over) && valid;
    valid = compare_exact_field("demo_end", row_number, expected->demo_end, actual->demo_end) && valid;
    valid = compare_exact_field("indoor_clear", row_number, expected->indoor_clear, actual->indoor_clear) && valid;
    valid = compare_exact_field("wall_core_remaining", row_number, expected->wall_core_remaining, actual->wall_core_remaining) && valid;
    return valid;
}

int main(int argc, char **argv)
{
    TraceRows expected;
    TraceRows actual;
    size_t index;
    unsigned failures = 0u;

    if (argc != 3)
    {
        printf("usage: %s ORIGINAL_TRACE_JSONL NATIVE_TRACE_JSONL\n", argv[0]);
        return 2;
    }

    if (!load_attract_rows(argv[1], &expected) || !load_attract_rows(argv[2], &actual))
    {
        return 1;
    }

    if (expected.count != actual.count)
    {
        printf("FAIL attract trace row count expected=%u actual=%u\n",
               (unsigned)expected.count,
               (unsigned)actual.count);
        return 1;
    }

    for (index = 0u; index < expected.count; ++index)
    {
        if (!compare_row(&expected.rows[index], &actual.rows[index], (unsigned)(index + 1u)))
        {
            ++failures;
        }
    }

    if (failures != 0u)
    {
        return 1;
    }

    printf("PASS attract semantic trace compare rows=%u\n", (unsigned)actual.count);
    return 0;
}
