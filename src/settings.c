#if !defined(NCMPCPP_SETTINGS_C)
#define NCMPCPP_SETTINGS_C

#include "cbase.h"

#include <mpd/tag.h>

#include "c/ncm_c.h"
#include "configura.h"
#include "settings.h"
#include "title.h"

#define SETTINGS_LINE_CAP 16384

Configuration Config;

typedef int32 (*SettingsApplyFn)(Configuration *config,
                                char *value, int32 value_len,
                                NcmError *ncm_error);

typedef int32 (*SettingsListItemFn)(void *context,
                                   char *item, int32 item_len,
                                   NcmError *ncm_error);

typedef struct SettingsOption {
    char *name;
    char *default_value;
    int32 name_len;
    int32 default_value_len;
    SettingsApplyFn apply;
} SettingsOption;

#define SETTINGS_ASSERT_FIELD_TYPE(NAME, TYPE)                                 \
    _Static_assert(_Generic(&((Configuration *)0)->NAME,                       \
                            TYPE *: 1, default: 0),                            \
                   "generated Configuration field type mismatch")

#define XX_BOOL(NAME, DEFAULT_VALUE)                                           \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, bool);
#define XX_STRING(NAME, DEFAULT_VALUE)                                         \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, char *);                                  \
    SETTINGS_ASSERT_FIELD_TYPE(NAME##_len, int32);
#define XX_PATH(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_DIR(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_INTEGER(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)                      \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, int32);
#define XX_DOUBLE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)                       \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, double);
#define XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER)                           \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, C_TYPE);
#define XX_OPTIONAL_ENUM(                                                      \
    NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD, UNSET_VALUE            \
)                                                                              \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, C_TYPE);                                  \
    SETTINGS_ASSERT_FIELD_TYPE(PRESENT_FIELD, bool);
#define XX_COLOR(NAME, DEFAULT_VALUE)                                          \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcColor);
#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE)                                \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcFormattedColor);
#define XX_BORDER(NAME, DEFAULT_VALUE)                                         \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcBorder);
#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS)                                  \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcmFormatAst);
#define XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING)                          \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcBuffer);
#define XX_BUFFER_WIDTH(NAME, DEFAULT_VALUE, KEEP_EXISTING)                    \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcBuffer);                                \
    SETTINGS_ASSERT_FIELD_TYPE(NAME##_length, int32);
#define XX_LOOK(NAME, DEFAULT_VALUE, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)         \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, StrBuilder);
#define XX_RATIO(NAME, DEFAULT_VALUE, EXPECTED_LEN)                            \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcmInt32Array);
#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT_VALUE)                           \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcmFormattedColorArray);
#define XX_LYRICS_FETCHERS(NAME, DEFAULT_VALUE)                                \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, NcmLyricsFetcherRegistry);
#define XX_SCREEN_LIST(NAME, DEFAULT_VALUE, PREVIOUS_FIELD)                    \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, ScreenTypeArray);                         \
    SETTINGS_ASSERT_FIELD_TYPE(PREVIOUS_FIELD, bool);
#define XX_NAMED_BOOL(NAME, DEFAULT_VALUE, TRUE_VALUE, FALSE_VALUE)            \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, bool);
#define XX_UINT32_CHOICE(NAME, DEFAULT_VALUE, PARSER, UNSET_VALUE)             \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, uint32);
#define XX_COLUMNS(NAME, DEFAULT_VALUE, FORMAT_FIELD)                          \
    SETTINGS_ASSERT_FIELD_TYPE(NAME, ColumnArray);                             \
    SETTINGS_ASSERT_FIELD_TYPE(FORMAT_FIELD, NcmFormatAst);
#include "config_options_pass.h"
#undef SETTINGS_ASSERT_FIELD_TYPE

static int32
settings_error(NcmError *ncm_error, char *message, int32 message_len) {
    return ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE,
                                message, message_len);
}

static int32
settings_invalid_value(NcmError *ncm_error, char *value, int32 value_len) {
    char message[256];
    int32 len = SNPRINTF(message, "invalid value: %.*s", value_len, value);
    return settings_error(ncm_error, message, len);
}

static void
settings_expand_home(StrBuilder *buffer, char *value, int32 value_len) {
    char *home;
    int32 home_len;

    sb_clear(buffer);
    if ((value_len <= 0) || (value[0] != '~')) {
        SB_APPEND(buffer, value, value_len);
        return;
    }

    if ((home = getenv("HOME")) == NULL) {
        SB_APPEND(buffer, value, value_len);
        return;
    }
    home_len = strlen32(home);
    SB_APPEND(buffer, home, home_len);
    if (value_len == 1) {
        return;
    }
    sb_append_byte(buffer, '/');
    if (value[1] == '/') {
        if (value_len > 2) {
            SB_APPEND(buffer, value + 2, value_len - 2);
        }
    } else {
        SB_APPEND(buffer, value + 1, value_len - 1);
    }
    return;
}

static void
settings_parse_string(char **result, int32 *result_len,
                      char *value, int32 value_len) {
    free2(*result, *result_len + 1);
    *result = NULL;
    *result_len = 0;
    if (value_len > 0) {
        *result = xstrndup(value, value_len);
        *result_len = value_len;
    }
    return;
}

static void
settings_parse_path_common(char **result, int32 *result_len,
                           char *value, int32 value_len, bool directory) {
    StrBuilder buffer = {0};

    settings_expand_home(&buffer, value, value_len);
    if (directory) {
        sb_append_byte_if_not(&buffer, '/');
    }

    free2(*result, *result_len + 1);
    *result = NULL;
    *result_len = 0;
    if (buffer.len > 0) {
        *result = sb_steal_exact(&buffer, result_len);
    }
    sb_free(&buffer);
    return;
}

static int32
settings_copy_nc_buffer(NcBuffer *buffer, char *value, int32 value_len,
                        int32 *width, bool keep_existing,
                        NcmError *ncm_error) {
    NcmFormatAst ast = {0};
    NcBuffer tmp = {0};
    int32 status;

    if (keep_existing && !nc_buffer_is_empty(buffer)) {
        return 0;
    }

    status = ncm_format_parse(&ast, value, value_len,
                              NCM_FORMAT_FLAG_COLOR | NCM_FORMAT_FLAG_FORMAT,
                              ncm_error);
    if (status < 0) {
        nc_buffer_destroy(&tmp);
        ncm_format_ast_destroy(&ast);
        return status;
    }

    ncm_format_render_buffer(&ast, NULL, &tmp, NULL,
                             NCM_FORMAT_FLAG_COLOR
                             | NCM_FORMAT_FLAG_FORMAT);
    nc_buffer_destroy(buffer);
    nc_buffer_move(buffer, &tmp);
    if (width) {
        *width = utf8_width(nc_buffer_data(buffer), buffer->len);
    }
    nc_buffer_destroy(&tmp);
    ncm_format_ast_destroy(&ast);
    return 0;
}

static int32
settings_parse_bool(char *value, int32 value_len, bool *result,
                    NcmError *ncm_error) {
    int32 status;

    status = ncm_option_parser_yes_no(value, value_len, result);
    if (status < 0) {
        return settings_invalid_value(ncm_error, value, value_len);
    }
    return 0;
}

static int32
settings_parse_int_range(char *value, int32 value_len, int32 *result,
                         int32 minimum, int32 maximum,
                         NcmError *ncm_error) {
    llong parsed;
    int32 status;

    status = parse_integer(value, value_len, &parsed);
    if (status < 0) {
        return ncm_error_set_status(ncm_error, status,
                                    STRLIT("invalid integer"));
    }
    status = ncm_bounds_check_i64(parsed, minimum, maximum, ncm_error);
    if (status < 0) {
        return status;
    }
    *result = (int32)parsed;
    return 0;
}

static int32
settings_parse_double_range(char *value, int32 value_len, double *result,
                            double minimum, double maximum,
                            NcmError *ncm_error) {
    double parsed;
    int32 status;

    status = ncm_parse_double(value, value_len, &parsed, ncm_error);
    if (status < 0) {
        return status;
    }
    if ((minimum == -HUGE_VAL) && (maximum == HUGE_VAL)) {
        *result = parsed;
        return 0;
    }
    if (maximum == HUGE_VAL) {
        status = ncm_lower_bound_check_f64(parsed, minimum, ncm_error);
    } else {
        status = ncm_bounds_check_f64(parsed, minimum, maximum, ncm_error);
    }
    if (status < 0) {
        return status;
    }
    *result = parsed;
    return 0;
}

static int32
settings_parse_single_color(char *value, int32 value_len, bool background,
                            int16 *result, NcmError *ncm_error) {
    int32 parsed;
    int32 status;

    if (STREQUAL(value, value_len, "black")) {
        *result = COLOR_BLACK;
        return 0;
    }
    if (STREQUAL(value, value_len, "red")) {
        *result = COLOR_RED;
        return 0;
    }
    if (STREQUAL(value, value_len, "green")) {
        *result = COLOR_GREEN;
        return 0;
    }
    if (STREQUAL(value, value_len, "yellow")) {
        *result = COLOR_YELLOW;
        return 0;
    }
    if (STREQUAL(value, value_len, "blue")) {
        *result = COLOR_BLUE;
        return 0;
    }
    if (STREQUAL(value, value_len, "magenta")) {
        *result = COLOR_MAGENTA;
        return 0;
    }
    if (STREQUAL(value, value_len, "cyan")) {
        *result = COLOR_CYAN;
        return 0;
    }
    if (STREQUAL(value, value_len, "white")) {
        *result = COLOR_WHITE;
        return 0;
    }
    if (background && STREQUAL(value, value_len, "transparent")) {
        *result = NC_COLOR_TRANSPARENT;
        return 0;
    }
    if (background && STREQUAL(value, value_len, "current")) {
        *result = NC_COLOR_CURRENT;
        return 0;
    }

    status = ncm_parse_int32(value, value_len, &parsed, ncm_error);
    if (status < 0) {
        return status;
    }
    if (background) {
        if (parsed > 256) {
            return settings_invalid_value(ncm_error, value, value_len);
        }
    } else if ((parsed < 1) || (parsed > 256)) {
        return settings_invalid_value(ncm_error, value, value_len);
    }
    if (parsed == 0) {
        *result = 0;
    } else {
        *result = (int16)(parsed - 1);
    }
    return 0;
}

static int32
settings_parse_color(char *value, int32 value_len, NcColor *color,
                     NcmError *ncm_error) {
    int32 underscore;
    int32 status;
    int16 foreground;
    int16 background;

    if (STREQUAL(value, value_len, "default")) {
        *color = nc_color_default();
        return 0;
    }
    if (STREQUAL(value, value_len, "end")) {
        *color = nc_color_end();
        return 0;
    }

    underscore = ncm_string_find_char(value, value_len, '_');
    if (underscore < 0) {
        status = settings_parse_single_color(value, value_len, false,
                                             &foreground, ncm_error);
        if (status < 0) {
            return status;
        }
        *color = nc_color_make(foreground, NC_COLOR_CURRENT, false, false);
        return 0;
    }

    status = settings_parse_single_color(value, underscore, false, &foreground,
                                         ncm_error);
    if (status < 0) {
        return status;
    }
    status = settings_parse_single_color(value + underscore + 1,
                                         value_len - underscore - 1, true,
                                         &background, ncm_error);
    if (status < 0) {
        return status;
    }
    *color = nc_color_make(foreground, background, false, false);
    return 0;
}

static int32
settings_parse_formatted_color(char *value, int32 value_len,
                               NcFormattedColor *color,
                               NcmError *ncm_error) {
    int32 colon;
    int32 status;
    NcColor base;
    NcFormattedColor tmp;

    colon = ncm_string_find_char(value, value_len, ':');
    if (colon < 0) {
        colon = value_len;
    }
    status = settings_parse_color(value, colon, &base, ncm_error);
    if (status < 0) {
        return status;
    }
    if (nc_color_is_end(base)) {
        return settings_invalid_value(ncm_error, value, value_len);
    }

    nc_formatted_color_init_color(&tmp, base);
    for (int32 i = colon + 1; i < value_len; i += 1) {
        switch (value[i]) {
        case 'b':
            nc_formatted_color_add_format(&tmp, NC_FORMAT_BOLD);
            break;
        case 'u':
            nc_formatted_color_add_format(&tmp, NC_FORMAT_UNDERLINE);
            break;
        case 'r':
            nc_formatted_color_add_format(&tmp, NC_FORMAT_REVERSE);
            break;
        case 'a':
            nc_formatted_color_add_format(&tmp, NC_FORMAT_ALT_CHARSET);
            break;
        case 'i':
            nc_formatted_color_add_format(&tmp, NC_FORMAT_ITALIC);
            break;
        default:
            nc_formatted_color_destroy(&tmp);
            return settings_invalid_value(ncm_error, value, value_len);
        }
    }

    nc_formatted_color_destroy(color);
    nc_formatted_color_move(color, &tmp);
    return 0;
}

static int32
settings_parse_border(char *value, int32 value_len, NcBorder *border,
                      NcmError *ncm_error) {
    NcColor color;
    int32 status;

    status = settings_parse_color(value, value_len, &color, ncm_error);
    if (status < 0) {
        return status;
    }
    *border = nc_border_make(color);
    return 0;
}

static void
settings_next_list_item(char *value, int32 value_len, int32 *pos, char **item,
                        int32 *item_len, bool *found) {
    int32 start;
    int32 end;
    bool quoted;

    if (*pos > value_len) {
        *found = false;
        return;
    }
    start = *pos;
    end = start;
    quoted = false;
    while (end < value_len) {
        if (value[end] == '"') {
            quoted = !quoted;
        }
        if (!quoted && (value[end] == ',')) {
            break;
        }
        end += 1;
    }

    while (start < end) {
        uint8 c = (uint8)value[start];

        if (!isspace(c)) {
            break;
        }
        start += 1;
    }
    while (end > start) {
        uint8 c = (uint8)value[end - 1];

        if (!isspace(c)) {
            break;
        }
        end -= 1;
    }
    if ((end - start >= 2) && (value[start] == '"')
        && (value[end - 1] == '"')) {
        start += 1;
        end -= 1;
    }

    *item = value + start;
    *item_len = end - start;
    if (end < value_len) {
        *pos = end + 1;
    } else {
        *pos = value_len + 1;
    }
    *found = true;
    return;
}

static int32
settings_parse_list(char *value, int32 value_len, void *context,
                    SettingsListItemFn append_item, NcmError *ncm_error) {
    int32 pos;
    bool added;

    ASSERT(context != NULL);
    ASSERT(append_item != NULL);

    pos = 0;
    added = false;
    while (pos <= value_len) {
        char *item;
        int32 item_len;
        int32 status;
        bool found;

        settings_next_list_item(value, value_len, &pos,
                                &item, &item_len, &found);
        if (!found) {
            break;
        }
        if (item_len <= 0) {
            continue;
        }
        status = append_item(context, item, item_len, ncm_error);
        if (status < 0) {
            return status;
        }
        added = true;
    }
    if (!added) {
        return settings_invalid_value(ncm_error, value, value_len);
    }
    return 0;
}

static int32
settings_parse_ratio(NcmInt32Array *array, char *value, int32 value_len,
                     int32 expected_len, NcmError *ncm_error) {
    int32 start;
    int32 total;

    ncm_int32_array_clear(array);
    start = 0;
    total = 0;
    while (start <= value_len) {
        int32 end;
        int32 parsed;
        int32 status;
        int32 *slot;

        end = start;
        while ((end < value_len) && (value[end] != ':')) {
            end += 1;
        }
        status = ncm_parse_int32(value + start, end - start, &parsed,
                                 ncm_error);
        if (status < 0) {
            return status;
        }
        slot = ncm_int32_array_append(array);
        *slot = parsed;
        total += parsed;
        if (end >= value_len) {
            break;
        }
        start = end + 1;
    }
    if ((array->len != expected_len) || (total == 0)) {
        return settings_invalid_value(ncm_error, value, value_len);
    }
    return 0;
}

static int32
settings_parse_browser_sort_mode(char *value, int32 value_len,
                                 enum SortMode *result) {
    if (STREQUAL(value, value_len, "noop")) {
        value = "none";
        value_len = STRLIT_LEN("none");
    }
    return ncm_sort_mode_parse(value, value_len, result);
}

static int32
settings_parse_media_library_primary_tag(char *value, int32 value_len,
                                         enum mpd_tag_type *result) {
    if (STREQUAL(value, value_len, "artist")) {
        *result = MPD_TAG_ARTIST;
        return 0;
    }
    if (STREQUAL(value, value_len, "album_artist")) {
        *result = MPD_TAG_ALBUM_ARTIST;
        return 0;
    }
    if (STREQUAL(value, value_len, "date")) {
        *result = MPD_TAG_DATE;
        return 0;
    }
    if (STREQUAL(value, value_len, "genre")) {
        *result = MPD_TAG_GENRE;
        return 0;
    }
    if (STREQUAL(value, value_len, "composer")) {
        *result = MPD_TAG_COMPOSER;
        return 0;
    }
    if (STREQUAL(value, value_len, "performer")) {
        *result = MPD_TAG_PERFORMER;
        return 0;
    }
    return -NCM_ERROR_PARSE;
}

static int32
settings_append_formatted_color(void *context, char *item, int32 item_len,
                                NcmError *ncm_error) {
    NcmFormattedColorArray *array = context;
    NcFormattedColor *dest;

    dest = ncm_formatted_color_array_append(array);
    return settings_parse_formatted_color(item, item_len, dest, ncm_error);
}

static int32
settings_parse_formatted_color_list(NcmFormattedColorArray *array,
                                    char *value, int32 value_len,
                                    NcmError *ncm_error) {
    ncm_formatted_color_array_clear(array);
    return settings_parse_list(value, value_len, array,
                               settings_append_formatted_color, ncm_error);
}

static int32
settings_parse_format(NcmFormatAst *format, char *value, int32 value_len,
                      uint32 flags, NcmError *ncm_error) {
    ncm_format_ast_clear(format);
    return ncm_format_parse(format, value, value_len, flags, ncm_error);
}

static int32
settings_parse_columns(ColumnArray *columns, NcmFormatAst *format,
                       char *value, int32 value_len,
                       NcmError *ncm_error) {
    int32 pos;
    int32 last_relative;
    int32 stretch_limit;
    int32 status;

    column_array_clear(columns);
    ncm_format_ast_clear(format);
    pos = 0;
    while (pos < value_len) {
        StrBuilder width = {0};
        StrBuilder color = {0};
        StrBuilder tag = {0};
        Column *column;
        int32 next;
        int32 parsed_width;

        width = ncm_string_get_enclosed(value, value_len, '(', ')', pos,
                                        &next);
        if (width.len <= 0) {
            sb_free(&width);
            sb_free(&color);
            sb_free(&tag);
            break;
        }
        pos = next;
        color = ncm_string_get_enclosed(value, value_len, '[', ']', pos,
                                        &next);
        pos = next;
        tag = ncm_string_get_enclosed(value, value_len, '{', '}', pos, &next);
        pos = next;
        column = column_array_append(columns);
        if ((width.len > 0) && (width.data[width.len - 1] == 'f')) {
            column->fixed = true;
            width.len -= 1;
            width.data[width.len] = '\0';
        }
        status = ncm_parse_int32(width.data, width.len, &parsed_width,
                                 ncm_error);
        if (status < 0) {
            sb_free(&width);
            sb_free(&color);
            sb_free(&tag);
            return status;
        }
        column->width = parsed_width;
        if (color.len > 0) {
            status = settings_parse_color(color.data, color.len,
                                          &column->color, ncm_error);
            if (status < 0) {
                sb_free(&width);
                sb_free(&color);
                sb_free(&tag);
                return status;
            }
        }
        if (tag.len > 0) {
            int32 colon;
            int32 type_len;

            colon = ncm_string_find_char(tag.data, tag.len, ':');
            type_len = tag.len;
            if (colon >= 0) {
                stupid_string_set(&column->name, &column->name_len,
                                    &column->name_cap, tag.data + colon + 1,
                                    tag.len - colon - 1);
                type_len = colon;
            }
            for (int32 i = 0; i < type_len; i += 1) {
                char ch = tag.data[i];

                switch (ch) {
                case 'r':
                    column->right_alignment = true;
                    break;
                case 'E':
                    column->display_empty_tag = false;
                    break;
                case '|':
                    break;
                default:
                    if (ncm_song_getter_from_char(ch) != NCM_SONG_GETTER_NONE) {
                        int32 new_len = column->type_len + 1;
                        int32 new_cap = new_len + 1;
                        char *new_data = malloc2(new_cap);

                        if (column->type_len > 0) {
                            memcpy64(new_data, column->type,
                                     column->type_len);
                        }
                        new_data[column->type_len] = ch;
                        new_data[new_len] = '\0';

                        free2(column->type, column->type_cap);

                        column->type = new_data;
                        column->type_len = new_len;
                        column->type_cap = new_cap;
                    }
                    break;
                }
            }
        } else {
            column->display_empty_tag = false;
        }
        sb_free(&width);
        sb_free(&color);
        sb_free(&tag);
    }

    if (columns->len <= 0) {
        return settings_invalid_value(ncm_error, value, value_len);
    }

    last_relative = -1;
    stretch_limit = 0;
    for (int32 i = columns->len - 1; i >= 0; i -= 1) {
        if (columns->items[i].fixed) {
            stretch_limit += columns->items[i].width;
        } else {
            last_relative = i;
            break;
        }
    }
    if (last_relative >= 0) {
        Column *column = &columns->items[last_relative];

        column->stretch_limit = stretch_limit;
    }

    for (int32 i = 0; i < columns->len; i += 1) {
        Column *column = &columns->items[i];

        ncm_format_ast_append_column_types(format,
                                           column->type, column->type_len);
    }
    return 0;
}

static int32
settings_parse_look(StrBuilder *look, char *value, int32 value_len,
                    int32 min_chars, int32 max_chars, bool pad_to_max,
                    NcmError *ncm_error) {
    int32 characters = utf8_characters(value, value_len);

    if ((characters < min_chars) || (characters > max_chars)) {
        return settings_invalid_value(ncm_error, value, value_len);
    }
    sb_clear(look);
    SB_APPEND(look, value, value_len);
    if (pad_to_max) {
        char zero = '\0';

        for (int32 i = characters; i < max_chars; i += 1) {
            sb_append(look, &zero, 1);
        }
    }
    return 0;
}

static int32
settings_parse_named_bool(char *value, int32 value_len, bool *result,
                          char *true_value, int32 true_value_len,
                          char *false_value, int32 false_value_len,
                          NcmError *ncm_error) {
    if (STREQUAL(value, value_len, true_value, true_value_len)) {
        *result = true;
        return 0;
    }
    if (STREQUAL(value, value_len, false_value, false_value_len)) {
        *result = false;
        return 0;
    }
    return settings_invalid_value(ncm_error, value, value_len);
}

static int32
settings_append_lyrics_fetcher(void *context, char *item, int32 item_len,
                               NcmError *ncm_error) {
    NcmLyricsFetcherRegistry *registry = context;
    int32 status;

    status = ncm_lyrics_fetcher_registry_append_name(registry,
                                                      item, item_len);
    if (status < 0) {
        return settings_error(ncm_error, STRLIT("unknown lyrics fetcher"));
    }
    return 0;
}

static int32
settings_parse_lyrics_fetchers(NcmLyricsFetcherRegistry *registry,
                               char *value, int32 value_len,
                               NcmError *ncm_error) {
    ncm_lyrics_fetcher_registry_clear(registry);
    return settings_parse_list(value, value_len, registry,
                               settings_append_lyrics_fetcher, ncm_error);
}

static int32
settings_append_screen(void *context, char *item, int32 item_len,
                       NcmError *ncm_error) {
    ScreenTypeArray *array = context;
    enum ScreenType *slot;
    enum ScreenType screen;
    int32 status;

    status = screen_type_parse_startup(item, item_len, &screen);
    if (status < 0) {
        return settings_invalid_value(ncm_error, item, item_len);
    }
    slot = screen_type_array_append(array);
    *slot = screen;
    return 0;
}

static int32
settings_parse_screen_list(ScreenTypeArray *array, bool *previous,
                           char *value, int32 value_len,
                           NcmError *ncm_error) {
    if (STREQUAL(value, value_len, "previous")) {
        *previous = true;
        screen_type_array_clear(array);
        return 0;
    }
    *previous = false;
    screen_type_array_clear(array);
    return settings_parse_list(value, value_len, array,
                               settings_append_screen, ncm_error);
}

static int32
settings_parse_regular_expressions(char *value, int32 value_len,
                                   uint32 *result) {
    if (STREQUAL(value, value_len, "none")) {
        *result = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
        return 0;
    }
    if (STREQUAL(value, value_len, "basic")) {
        *result = NCM_REGEX_BASIC_CASE_INSENSITIVE;
        return 0;
    }
    if (STREQUAL(value, value_len, "extended")) {
        *result = NCM_REGEX_EXTENDED_CASE_INSENSITIVE;
        return 0;
    }
    return -NCM_ERROR_PARSE;
}

static int32
settings_report_or_ignore(NcmError *ncm_error, bool ignore_errors) {
    ASSERT(ncm_error_is_set(ncm_error));
    error2("%s\n", ncm_error->message);
    if (!ignore_errors) {
        return ncm_error_status(ncm_error);
    }
    ncm_error_clear(ncm_error);
    return 0;
}

static int32
settings_apply_option(Configuration *config, SettingsOption option,
                      char *value, int32 value_len, bool default_value,
                      bool ignore_errors, NcmError *ncm_error) {
    NcmError cause;
    char message[256];
    char *phase;
    char *detail;
    int32 detail_len;
    int32 len;
    int32 status;

    ncm_error_clear(&cause);
    status = option.apply(config, value, value_len, &cause);
    if (status < 0) {
        if (default_value) {
            phase = "initializing";
        } else {
            phase = "processing";
        }
        detail = "invalid value";
        detail_len = STRLIT_LEN("invalid value");
        if (ncm_error_is_set(&cause)) {
            detail = cause.message;
            detail_len = strlen32(cause.message);
        }

        len = SNPRINTF(message, "error while %s option \"%.*s\": %.*s",
                       phase, option.name_len, option.name,
                       detail_len, detail);
        if (len < 0) {
            ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE,
                                 STRLIT("error while processing option"));
        } else {
            if (len >= SIZEOF(message)) {
                len = SIZEOF(message) - 1;
            }
            if (ncm_error_is_set(&cause)) {
                ncm_error_set(ncm_error, cause.code, message, len);
            } else {
                ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE,
                                     message, len);
            }
        }
        return settings_report_or_ignore(ncm_error, ignore_errors);
    }
    return 0;
}

int32
configuration_validate(Configuration *config, NcmError *ncm_error) {
    if (config == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing configuration"));
    }

    if (config->visualizer_spectrum_hz_max
        <= config->visualizer_spectrum_hz_min) {
        return settings_error(
            ncm_error,
            STRLIT("visualizer_spectrum_hz_max must be greater than "
                   "visualizer_spectrum_hz_min"));
    }
    return ncm_error_ok(ncm_error);
}

int32
configuration_apply_runtime(Configuration *config, NcmMpdClient *client,
                            bool quiet, NcmError *ncm_error) {
    int32 status;

    if ((config == NULL) || (client == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing runtime configuration"));
    }

    status = ncm_mpd_client_set_hostname(
        client, config->mpd_host, config->mpd_host_len, ncm_error);
    if (status < 0) {
        return status;
    }
    ncm_mpd_client_set_port(client, (uint16)config->mpd_port);
    if (config->mpd_password_len > 0) {
        status = ncm_mpd_client_set_password(
            client, config->mpd_password, config->mpd_password_len, ncm_error);
        if (status < 0) {
            return status;
        }
    }
    status = ncm_mpd_client_set_timeout_ms(
        client, config->mpd_connection_timeout*1000, ncm_error);
    if (status < 0) {
        return status;
    }

    ncm_window_title_configure(config->enable_window_title, quiet);
    return ncm_error_ok(ncm_error);
}

#define XX_DIR(NAME, DEFAULT_VALUE)                                            \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error UNUSED) {                                 \
        settings_parse_path_common(&config->NAME, &config->NAME##_len,         \
                                   value, value_len, true);                    \
        return 0;                                                              \
    }

#define XX_PATH(NAME, DEFAULT_VALUE)                                           \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error UNUSED) {                                 \
        settings_parse_path_common(&config->NAME, &config->NAME##_len,         \
                                   value, value_len, false);                   \
        return 0;                                                              \
    }

#define XX_STRING(NAME, DEFAULT_VALUE)                                         \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error UNUSED) {                                 \
        settings_parse_string(&config->NAME, &config->NAME##_len,              \
                              value, value_len);                               \
        return 0;                                                              \
    }

#define XX_BOOL(NAME, DEFAULT_VALUE)                                           \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_bool(value, value_len, &config->NAME,            \
                                   ncm_error);                                 \
    }

#define XX_INTEGER(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)                      \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_int_range(value, value_len, &config->NAME,       \
                                        MINIMUM, MAXIMUM, ncm_error);          \
    }

#define XX_DOUBLE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM)                       \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_double_range(value, value_len, &config->NAME,    \
                                           MINIMUM, MAXIMUM, ncm_error);       \
    }

#define XX_ENUM(NAME, C_TYPE, DEFAULT_VALUE, PARSER)                           \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        int32 status;                                                          \
        status = PARSER(value, value_len, &config->NAME);                      \
        if (status < 0) {                                                      \
            return settings_invalid_value(ncm_error, value, value_len);        \
        }                                                                      \
        return 0;                                                              \
    }

#define XX_OPTIONAL_ENUM(                                                      \
    NAME, C_TYPE, DEFAULT_VALUE, PARSER, PRESENT_FIELD, UNSET_VALUE            \
)                                                                              \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        int32 status;                                                          \
        if (value_len <= 0) {                                                  \
            config->PRESENT_FIELD = false;                                     \
            config->NAME = (C_TYPE)(UNSET_VALUE);                              \
            return 0;                                                          \
        }                                                                      \
        status = PARSER(value, value_len, &config->NAME);                      \
        if (status < 0) {                                                      \
            return settings_invalid_value(ncm_error, value, value_len);        \
        }                                                                      \
        config->PRESENT_FIELD = true;                                          \
        return 0;                                                              \
    }

#define XX_COLOR(NAME, DEFAULT_VALUE)                                          \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_color(value, value_len, &config->NAME,           \
                                    ncm_error);                                \
    }

#define XX_FORMATTED_COLOR(NAME, DEFAULT_VALUE)                                \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_formatted_color(value, value_len,                \
                                              &config->NAME, ncm_error);       \
    }

#define XX_BORDER(NAME, DEFAULT_VALUE)                                         \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_border(value, value_len, &config->NAME,          \
                                     ncm_error);                               \
    }

#define XX_FORMAT(NAME, DEFAULT_VALUE, FLAGS)                                  \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_format(&config->NAME, value, value_len,          \
                                     FLAGS, ncm_error);                        \
    }

#define XX_BUFFER(NAME, DEFAULT_VALUE, KEEP_EXISTING)                          \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_copy_nc_buffer(&config->NAME, value, value_len, NULL,  \
                                       KEEP_EXISTING, ncm_error);              \
    }

#define XX_BUFFER_WIDTH(NAME, DEFAULT_VALUE, KEEP_EXISTING)                    \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_copy_nc_buffer(                                        \
            &config->NAME, value, value_len, &config->NAME##_length,           \
            KEEP_EXISTING, ncm_error);                                         \
    }

#define XX_LOOK(NAME, DEFAULT_VALUE, MIN_CHARS, MAX_CHARS, PAD_TO_MAX)         \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_look(&config->NAME, value, value_len,            \
                                   MIN_CHARS, MAX_CHARS, PAD_TO_MAX,           \
                                   ncm_error);                                 \
    }

#define XX_RATIO(NAME, DEFAULT_VALUE, EXPECTED_LEN)                            \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_ratio(&config->NAME, value, value_len,           \
                                    EXPECTED_LEN, ncm_error);                  \
    }

#define XX_FORMATTED_COLOR_LIST(NAME, DEFAULT_VALUE)                           \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_formatted_color_list(                            \
            &config->NAME, value, value_len, ncm_error);                       \
    }

#define XX_LYRICS_FETCHERS(NAME, DEFAULT_VALUE)                                \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_lyrics_fetchers(                                 \
            &config->NAME, value, value_len, ncm_error);                       \
    }

#define XX_SCREEN_LIST(NAME, DEFAULT_VALUE, PREVIOUS_FIELD)                    \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_screen_list(                                     \
            &config->NAME, &config->PREVIOUS_FIELD, value, value_len,          \
            ncm_error);                                                        \
    }

#define XX_NAMED_BOOL(NAME, DEFAULT_VALUE, TRUE_VALUE, FALSE_VALUE)            \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_named_bool(                                      \
            value, value_len, &config->NAME, TRUE_VALUE,                       \
            STRLIT_LEN(TRUE_VALUE), FALSE_VALUE, STRLIT_LEN(FALSE_VALUE),      \
            ncm_error);                                                        \
    }

#define XX_UINT32_CHOICE(NAME, DEFAULT_VALUE, PARSER, UNSET_VALUE)             \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        int32 status;                                                          \
        status = PARSER(value, value_len, &config->NAME);                      \
        if (status < 0) {                                                      \
            return settings_invalid_value(ncm_error, value, value_len);        \
        }                                                                      \
        return 0;                                                              \
    }

#define XX_COLUMNS(NAME, DEFAULT_VALUE, FORMAT_FIELD)                          \
    static int32                                                               \
    apply_##NAME(Configuration *config, char *value, int32 value_len,          \
                 NcmError *ncm_error) {                                        \
        return settings_parse_columns(&config->NAME,                           \
                                      &config->FORMAT_FIELD,                   \
                                      value, value_len, ncm_error);            \
    }

#include "config_options_pass.h"

#define OPT(NAME, DEFAULT_VALUE)                               \
    {                                                          \
        .name = #NAME,                                         \
        .default_value = DEFAULT_VALUE,                        \
        .name_len = STRLIT_LEN(#NAME),                         \
        .default_value_len = STRLIT_LEN(DEFAULT_VALUE),        \
        .apply = apply_##NAME,                                 \
    }

static const SettingsOption ncmpcpp_options[] = {
#define XX_OPTION(NAME, DEFAULT_VALUE, ...)                    \
    [SETTINGS_OPTION_##NAME] = OPT(NAME, DEFAULT_VALUE),
#include "config_options_pass.h"
};

_Static_assert(LENGTH(ncmpcpp_options) == SETTINGS_OPTION_COUNT,
               "generated settings option table size mismatch");

#undef OPT

int32
configuration_read(Configuration *config, NcmStringViewArray *config_paths,
                   bool ignore_errors, bool quiet, NcmError *ncm_error) {
    bool used[SETTINGS_OPTION_COUNT] = {0};
    int32 status;

    configuration_clear(config);
    for (int32 i = 0; i < config_paths->len; i += 1) {
        NcmStringView path = config_paths->items[i];
        FILE *file;
        StrBuilder path_buffer = {0};
        char line[SETTINGS_LINE_CAP];

        if (!ncm_fs_path_is_existing(path.data, path.len)) {
            continue;
        }

        SB_APPEND(&path_buffer, path.data, path.len);
        if ((file = fopen(path_buffer.data, "r")) == NULL) {
            char message[256];
            int32 saved_errno;
            int32 len;

            saved_errno = errno;
            len = SNPRINTF(
                message,
                "failed to open configuration file '%.*s': %s",
                path.len, path.data, strerror(saved_errno));
            if (len < 0) {
                ncm_error_set_status(
                    ncm_error, -saved_errno,
                    STRLIT("failed to open configuration file"));
            } else {
                if (len >= SIZEOF(message)) {
                    len = SIZEOF(message) - 1;
                }
                ncm_error_set_status(ncm_error, -saved_errno,
                                     message, len);
            }
            status = settings_report_or_ignore(ncm_error, ignore_errors);
            sb_free(&path_buffer);
            if (status < 0) {
                return status;
            }
            continue;
        }

        if (!quiet) {
            error2("Reading configuration from %s...\n",
                   path_buffer.data);
        }
        while (fgets(line, SIZEOF(line), file)) {
            NcmOptionLine parsed;
            int32 line_len = strlen32(line);
            uint32 option_index = SETTINGS_OPTION_COUNT;
            bool has_option;

            while (
                (line_len > 0)
                && ((line[line_len - 1] == '\n')
                    || (line[line_len - 1] == '\r'))) {
                line_len -= 1;
                line[line_len] = '\0';
            }
            status = ncm_option_parser_parse_line(line, line_len, &parsed,
                                                  &has_option);
            if (status < 0) {
                settings_invalid_value(ncm_error, line, line_len);
                status = settings_report_or_ignore(ncm_error,
                                                   ignore_errors);
                if (status < 0) {
                    fclose(file);
                    sb_free(&path_buffer);
                    return status;
                }
                continue;
            }
            if (!has_option) {
                continue;
            }

            for (uint32 j = 0; j < SETTINGS_OPTION_COUNT; j += 1) {
                if (STREQUAL(
                    parsed.option, parsed.option_len,
                    ncmpcpp_options[j].name, ncmpcpp_options[j].name_len)) {
                    option_index = j;
                    break;
                }
            }
            if (option_index == SETTINGS_OPTION_COUNT) {
                char message[256];
                int32 len;

                len = SNPRINTF(message, "unknown option: %.*s",
                               parsed.option_len, parsed.option);
                if (len < 0) {
                    ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE,
                                         STRLIT("unknown option"));
                } else {
                    if (len >= SIZEOF(message)) {
                        len = SIZEOF(message) - 1;
                    }
                    ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE,
                                         message, len);
                }
                status = settings_report_or_ignore(ncm_error,
                                                   ignore_errors);
                if (status < 0) {
                    fclose(file);
                    sb_free(&path_buffer);
                    return status;
                }
                continue;
            }
            if (used[option_index]) {
                char message[256];
                int32 len;

                len = SNPRINTF(
                    message,
                    "error while processing option \"%.*s\": "
                    "option already set",
                    ncmpcpp_options[option_index].name_len,
                    ncmpcpp_options[option_index].name);
                ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE,
                                     message, len);
                status = settings_report_or_ignore(ncm_error,
                                                   ignore_errors);
                if (status < 0) {
                    fclose(file);
                    sb_free(&path_buffer);
                    return status;
                }
                continue;
            }
            used[option_index] = true;
            status = settings_apply_option(
                config, ncmpcpp_options[option_index],
                parsed.value, parsed.value_len,
                false, ignore_errors, ncm_error);
            if (status < 0) {
                fclose(file);
                sb_free(&path_buffer);
                return status;
            }
        }
        fclose(file);
        sb_free(&path_buffer);
    }

    for (uint32 i = 0; i < SETTINGS_OPTION_COUNT; i += 1) {
        if (used[i]) {
            continue;
        }
        status = settings_apply_option(
            config, ncmpcpp_options[i], ncmpcpp_options[i].default_value,
            ncmpcpp_options[i].default_value_len,
            true, ignore_errors, ncm_error);
        if (status < 0) {
            return status;
        }
    }

    status = configuration_validate(config, ncm_error);
    if (status < 0) {
        return settings_report_or_ignore(ncm_error, ignore_errors);
    }
    return 0;
}

#endif /* NCMPCPP_SETTINGS_C */
