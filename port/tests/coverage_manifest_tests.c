#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum
{
    MAX_LINE_LENGTH = 1024,
    MAX_FIELDS = 6,
    MAX_FILE_BYTES = 1024u * 1024u
};

static bool field_is_empty(const char *field)
{
    return (field == NULL) || (field[0] == '\0') || (field[0] == '\n') || (field[0] == '\r');
}

static void trim_newline(char *text)
{
    size_t length = strlen(text);

    while ((length != 0u) && ((text[length - 1u] == '\n') || (text[length - 1u] == '\r')))
    {
        text[--length] = '\0';
    }
}

static size_t split_csv_line(char *line, char *fields[MAX_FIELDS])
{
    size_t count = 0u;
    char *cursor = line;

    while ((count < MAX_FIELDS) && (cursor != NULL))
    {
        char *next = strchr(cursor, ',');

        if (next != NULL)
        {
            *next = '\0';
            ++next;
        }

        fields[count++] = cursor;
        cursor = next;
    }

    return count;
}

static bool status_requires_evidence(const char *status)
{
    return (strcmp(status, "translated") == 0) || (strcmp(status, "partial") == 0);
}

static bool load_text_file(const char *path, char *buffer, size_t buffer_size)
{
    FILE *file = fopen(path, "rb");
    size_t bytes_read;

    if (file == NULL)
    {
        printf("FAIL could not open %s\n", path);
        return false;
    }

    bytes_read = fread(buffer, 1u, buffer_size - 1u, file);
    if (ferror(file) != 0)
    {
        printf("FAIL could not read %s\n", path);
        fclose(file);
        return false;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return true;
}

static bool source_contains_token(const char *source, const char *token)
{
    return (!field_is_empty(token)) && (strstr(source, token) != NULL);
}

static bool all_test_refs_exist(const char *tests_source, char *tests_field)
{
    char *cursor = tests_field;

    while (cursor != NULL)
    {
        char *next = strchr(cursor, '|');

        if (next != NULL)
        {
            *next = '\0';
            ++next;
        }

        if (!source_contains_token(tests_source, cursor))
        {
            return false;
        }

        cursor = next;
    }

    return true;
}

int main(void)
{
    FILE *file = fopen("port/coverage/asm_routines.csv", "r");
    static char core_source[MAX_FILE_BYTES];
    static char tests_source[MAX_FILE_BYTES];
    char line[MAX_LINE_LENGTH];
    unsigned line_number = 0u;
    unsigned checked_rows = 0u;
    unsigned failures = 0u;

    if (!load_text_file("port/contra_core/src/core.c", core_source, sizeof(core_source)) ||
        !load_text_file("port/tests/core_behavior_tests.c", tests_source, sizeof(tests_source)))
    {
        return 1;
    }

    if (file == NULL)
    {
        printf("FAIL could not open port/coverage/asm_routines.csv\n");
        return 1;
    }

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char *fields[MAX_FIELDS] = {0};
        size_t field_count;

        ++line_number;
        trim_newline(line);

        if ((line[0] == '\0') || (line[0] == '#'))
        {
            continue;
        }

        field_count = split_csv_line(line, fields);
        if (line_number == 1u)
        {
            continue;
        }

        if (field_count != MAX_FIELDS)
        {
            printf("FAIL coverage row %u has %u fields\n", line_number, (unsigned)field_count);
            ++failures;
            continue;
        }

        if (field_is_empty(fields[0]) || field_is_empty(fields[1]) || field_is_empty(fields[2]))
        {
            printf("FAIL coverage row %u is missing source_file label or status\n", line_number);
            ++failures;
            continue;
        }

        if (status_requires_evidence(fields[2]))
        {
            if (field_is_empty(fields[3]))
            {
                printf("FAIL coverage row %u has %s status without native_symbol\n", line_number, fields[2]);
                ++failures;
            }
            else if (!source_contains_token(core_source, fields[3]))
            {
                printf("FAIL coverage row %u native_symbol %s was not found in core.c\n", line_number, fields[3]);
                ++failures;
            }

            if (field_is_empty(fields[4]))
            {
                printf("FAIL coverage row %u has %s status without tests\n", line_number, fields[2]);
                ++failures;
            }
            else if (!all_test_refs_exist(tests_source, fields[4]))
            {
                printf("FAIL coverage row %u references a missing test in %s\n", line_number, fields[4]);
                ++failures;
            }
        }

        ++checked_rows;
    }

    fclose(file);

    if (failures != 0u)
    {
        printf("FAIL coverage manifest rows=%u failures=%u\n", checked_rows, failures);
        return 1;
    }

    printf("PASS coverage manifest rows=%u\n", checked_rows);
    return 0;
}
