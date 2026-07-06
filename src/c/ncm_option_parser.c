#include "c/ncm_option_parser.h"

#include <ctype.h>
#include <stddef.h>

static bool ncm_option_is_word_char(char c);
static int32 ncm_option_trim_end(char *string, int32 string_len);

static bool
ncm_option_is_word_char(char c) {
    unsigned char u;

    u = (unsigned char)c;
    if (isalnum(u)) {
        return true;
    }
    if (c == '_') {
        return true;
    }
    return false;
}

static int32
ncm_option_trim_end(char *string, int32 string_len) {
    while (string_len > 0) {
        char c;

        c = string[string_len - 1];
        if ((c != ' ') && (c != '\t')) {
            break;
        }
        string_len -= 1;
    }

    return string_len;
}

bool
ncm_option_parser_parse_line(char *line, int32 line_len,
                             NcmOptionLine *result) {
    int32 option_len;
    int32 value_start;
    int32 value_len;

    result->option = NULL;
    result->value = NULL;
    result->option_len = 0;
    result->value_len = 0;

    option_len = 0;
    while ((option_len < line_len)
           && ncm_option_is_word_char(line[option_len])) {
        option_len += 1;
    }
    if (option_len <= 0) {
        return false;
    }

    value_start = option_len;
    while ((value_start < line_len)
           && ((line[value_start] == ' ') || (line[value_start] == '\t'))) {
        value_start += 1;
    }
    if ((value_start >= line_len) || (line[value_start] != '=')) {
        return false;
    }

    value_start += 1;
    while ((value_start < line_len)
           && ((line[value_start] == ' ') || (line[value_start] == '\t'))) {
        value_start += 1;
    }

    if ((value_start < line_len) && (line[value_start] == '"')) {
        int32 value_end;

        value_start += 1;
        value_end = line_len - 1;
        while ((value_end >= value_start) && (line[value_end] != '"')) {
            value_end -= 1;
        }
        if (value_end < value_start) {
            return false;
        }
        value_len = value_end - value_start;
    } else {
        value_len = ncm_option_trim_end(line + value_start,
                                        line_len - value_start);
    }

    result->option = line;
    result->option_len = option_len;
    result->value = line + value_start;
    result->value_len = value_len;
    return true;
}

bool
ncm_option_parser_yes_no(char *value, int32 value_len, bool *result) {
    if ((value_len == 3)
        && (value[0] == 'y')
        && (value[1] == 'e')
        && (value[2] == 's')) {
        *result = true;
        return true;
    }
    if ((value_len == 2)
        && (value[0] == 'n')
        && (value[1] == 'o')) {
        *result = false;
        return true;
    }

    return false;
}
