#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static void trim_line_end(char *line)
{
    size_t length = strlen(line);

    while ((length != 0u) &&
           ((line[length - 1u] == '\n') || (line[length - 1u] == '\r')))
    {
        line[--length] = '\0';
    }
}

static bool read_trace_line(FILE *file, const char *path, char *line, size_t line_size, bool *has_line)
{
    *has_line = fgets(line, (int)line_size, file) != NULL;
    if (*has_line)
    {
        trim_line_end(line);
        return true;
    }

    if (ferror(file) != 0)
    {
        printf("FAIL could not read checkpoint trace %s\n", path);
        return false;
    }

    return true;
}

int main(int argc, char **argv)
{
    FILE *expected_file;
    FILE *actual_file;
    char expected_line[2048];
    char actual_line[2048];
    unsigned line_number = 0u;
    unsigned failures = 0u;

    if (argc != 3)
    {
        printf("usage: %s EXPECTED_JSONL ACTUAL_JSONL\n", argv[0]);
        return 2;
    }

    expected_file = fopen(argv[1], "r");
    if (expected_file == NULL)
    {
        printf("FAIL could not open expected checkpoint trace %s\n", argv[1]);
        return 1;
    }

    actual_file = fopen(argv[2], "r");
    if (actual_file == NULL)
    {
        printf("FAIL could not open actual checkpoint trace %s\n", argv[2]);
        fclose(expected_file);
        return 1;
    }

    for (;;)
    {
        bool has_expected = false;
        bool has_actual = false;

        if (!read_trace_line(expected_file, argv[1], expected_line, sizeof(expected_line), &has_expected) ||
            !read_trace_line(actual_file, argv[2], actual_line, sizeof(actual_line), &has_actual))
        {
            ++failures;
            break;
        }

        if (!has_expected && !has_actual)
        {
            break;
        }

        ++line_number;
        if (has_expected != has_actual)
        {
            printf("FAIL checkpoint trace length mismatch at row %u\n", line_number);
            ++failures;
            break;
        }

        if (strcmp(expected_line, actual_line) != 0)
        {
            printf("FAIL checkpoint trace row %u mismatch\n", line_number);
            printf("expected: %s\n", expected_line);
            printf("actual:   %s\n", actual_line);
            ++failures;
        }
    }

    fclose(actual_file);
    fclose(expected_file);

    if (failures != 0u)
    {
        return 1;
    }

    printf("PASS checkpoint trace compare rows=%u\n", line_number);
    return 0;
}
