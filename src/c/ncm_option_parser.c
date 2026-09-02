#if !defined(NCM_OPTION_PARSER_C)
#define NCM_OPTION_PARSER_C

#include "cbase.h"

#include "c/ncm_c.h"

static bool
ncm_option_is_word_char(char c) {
    uint8 u = (uint8)c;

    if (isalnum(u)) {
        return true;
    }
    if (c == '_') {
        return true;
    }
    return false;
}

static int32
ncm_option_trim_start(char *string, int32 string_len) {
    int32 result = 0;

    while (result < string_len) {
        char c = string[result];

        if ((c != ' ') && (c != '\t')) {
            break;
        }
        result += 1;
    }

    return result;
}

static int32
ncm_option_trim_end(char *string, int32 string_len) {
    while (string_len > 0) {
        char c = string[string_len - 1];

        if ((c != ' ') && (c != '\t')) {
            break;
        }
        string_len -= 1;
    }

    return string_len;
}

static int32
ncm_option_comment_start(char *line, int32 line_len) {
    bool quoted = false;

    for (int32 i = 0; i < line_len; i += 1) {
        if (line[i] == '"') {
            quoted = !quoted;
            continue;
        }
        if (!quoted && (line[i] == '#')) {
            return i;
        }
    }

    return line_len;
}

int32
ncm_option_parser_parse_line(char *line, int32 line_len,
                             NcmOptionLine *result, bool *parsed) {
    int32 line_start;
    int32 option_start;
    int32 option_len;
    int32 value_start;
    int32 value_len;

    if ((line == NULL) || (result == NULL) || (parsed == NULL)) {
        return -EINVAL;
    }
    if (line_len < 0) {
        return -EINVAL;
    }

    result->option = NULL;
    result->value = NULL;
    result->option_len = 0;
    result->value_len = 0;
    *parsed = false;

    if (line_len == 0) {
        return 0;
    }

    line_len = ncm_option_comment_start(line, line_len);
    line_len = ncm_option_trim_end(line, line_len);
    line_start = ncm_option_trim_start(line, line_len);
    if (line_start >= line_len) {
        return 0;
    }

    option_start = line_start;
    option_len = 0;
    while ((option_start + option_len < line_len)
           && ncm_option_is_word_char(line[option_start + option_len])) {
        option_len += 1;
    }
    if (option_len <= 0) {
        return -NCM_ERROR_PARSE;
    }

    value_start = option_start + option_len;
    while ((value_start < line_len)
           && ((line[value_start] == ' ') || (line[value_start] == '\t'))) {
        value_start += 1;
    }
    if ((value_start >= line_len) || (line[value_start] != '=')) {
        return -NCM_ERROR_PARSE;
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
            return -NCM_ERROR_PARSE;
        }
        value_len = value_end - value_start;
    } else {
        value_len = ncm_option_trim_end(line + value_start,
                                        line_len - value_start);
    }

    result->option = line + option_start;
    result->option_len = option_len;
    result->value = line + value_start;
    result->value_len = value_len;
    *parsed = true;
    return 0;
}

int32
ncm_option_parser_yes_no(char *value, int32 value_len, bool *result) {
    if ((value == NULL) || (result == NULL) || (value_len < 0)) {
        return -EINVAL;
    }

    if ((value_len == 3)
        && (value[0] == 'y')
        && (value[1] == 'e')
        && (value[2] == 's')) {
        *result = true;
        return 0;
    }
    if ((value_len == 2)
        && (value[0] == 'n')
        && (value[1] == 'o')) {
        *result = false;
        return 0;
    }

    return -NCM_ERROR_PARSE;
}

#endif /* NCM_OPTION_PARSER_C */
