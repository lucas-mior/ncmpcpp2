#if !defined(NCMPCPP_SETTINGS_C)
#define NCMPCPP_SETTINGS_C

#include "settings.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpd/tag.h>

#include "config.h"
#include "c/ncm_base.h"
#include "c/ncm_conversion.h"
#include "c/ncm_fs.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_option_parser.h"
#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "global.h"

#define SETTINGS_LINE_CAP 16384

Configuration Config;

static bool settings_quiet;

typedef bool (*SettingsApplyFn)(Configuration *config, char *value,
                                int32 value_len, NcmError *error);

typedef struct SettingsOption {
    char *name;
    char *default_value;
    int32 name_len;
    int32 default_value_len;
    SettingsApplyFn apply;
    bool used;
} SettingsOption;

#define SETTINGS_OPTION(NAME, DEFAULT_VALUE, APPLY) \
    { \
        .name = (char *)NAME, \
        .default_value = (char *)DEFAULT_VALUE, \
        .name_len = STRLIT_LEN(NAME), \
        .default_value_len = STRLIT_LEN(DEFAULT_VALUE), \
        .apply = APPLY, \
        .used = false, \
    }

static bool apply_ncmpcpp_directory(Configuration *config, char *value,
                                    int32 value_len, NcmError *error);
static bool apply_lyrics_directory(Configuration *config, char *value,
                                   int32 value_len, NcmError *error);
static bool apply_mpd_music_dir(Configuration *config, char *value,
                                int32 value_len, NcmError *error);
static bool apply_random_exclude_pattern(Configuration *config, char *value,
                                         int32 value_len, NcmError *error);
static bool apply_visualizer_data_source(Configuration *config, char *value,
                                         int32 value_len, NcmError *error);
static bool apply_visualizer_output_name(Configuration *config, char *value,
                                         int32 value_len, NcmError *error);
static bool apply_visualizer_in_stereo(Configuration *config, char *value,
                                       int32 value_len, NcmError *error);
static bool apply_visualizer_autoscale(Configuration *config, char *value,
                                       int32 value_len, NcmError *error);
static bool apply_visualizer_spectrum_smooth_look(Configuration *config,
                                                  char *value, int32 value_len,
                                                  NcmError *error);
static bool apply_visualizer_spectrum_smooth_look_legacy_chars(
    Configuration *config, char *value, int32 value_len, NcmError *error);
static bool apply_visualizer_spectrum_log_scale_x(Configuration *config,
                                                  char *value, int32 value_len,
                                                  NcmError *error);
static bool apply_visualizer_spectrum_log_scale_y(Configuration *config,
                                                  char *value, int32 value_len,
                                                  NcmError *error);
static bool apply_message_delay_time(Configuration *config, char *value,
                                     int32 value_len, NcmError *error);
static bool apply_execute_on_song_change(Configuration *config, char *value,
                                         int32 value_len, NcmError *error);
static bool apply_execute_on_player_state_change(Configuration *config,
                                                 char *value, int32 value_len,
                                                 NcmError *error);
static bool apply_playlist_show_mpd_host(Configuration *config, char *value,
                                         int32 value_len, NcmError *error);
static bool apply_playlist_show_remaining_time(Configuration *config,
                                               char *value, int32 value_len,
                                               NcmError *error);
static bool apply_playlist_shorten_total_times(Configuration *config,
                                               char *value, int32 value_len,
                                               NcmError *error);
static bool apply_playlist_separate_albums(Configuration *config, char *value,
                                           int32 value_len, NcmError *error);
static bool apply_discard_colors_if_item_is_selected(Configuration *config,
                                                     char *value,
                                                     int32 value_len,
                                                     NcmError *error);
static bool apply_show_duplicate_tags(Configuration *config, char *value,
                                      int32 value_len, NcmError *error);
static bool apply_incremental_seeking(Configuration *config, char *value,
                                      int32 value_len, NcmError *error);
static bool apply_seek_time(Configuration *config, char *value, int32 value_len,
                            NcmError *error);
static bool apply_volume_change_step(Configuration *config, char *value,
                                     int32 value_len, NcmError *error);
static bool apply_autocenter_mode(Configuration *config, char *value,
                                  int32 value_len, NcmError *error);
static bool apply_centered_cursor(Configuration *config, char *value,
                                  int32 value_len, NcmError *error);
static bool apply_data_fetching_delay(Configuration *config, char *value,
                                      int32 value_len, NcmError *error);
static bool apply_media_library_hide_album_dates(Configuration *config,
                                                 char *value, int32 value_len,
                                                 NcmError *error);
static bool apply_media_library_albums_split_by_date(Configuration *config,
                                                     char *value,
                                                     int32 value_len,
                                                     NcmError *error);
static bool apply_default_tag_editor_pattern(Configuration *config, char *value,
                                             int32 value_len, NcmError *error);
static bool apply_header_visibility(Configuration *config, char *value,
                                    int32 value_len, NcmError *error);
static bool apply_statusbar_visibility(Configuration *config, char *value,
                                       int32 value_len, NcmError *error);
static bool apply_connected_message_on_startup(Configuration *config,
                                               char *value, int32 value_len,
                                               NcmError *error);
static bool apply_titles_visibility(Configuration *config, char *value,
                                    int32 value_len, NcmError *error);
static bool apply_header_text_scrolling(Configuration *config, char *value,
                                        int32 value_len, NcmError *error);
static bool apply_cyclic_scrolling(Configuration *config, char *value,
                                   int32 value_len, NcmError *error);
static bool apply_follow_now_playing_lyrics(Configuration *config, char *value,
                                            int32 value_len, NcmError *error);
static bool apply_fetch_lyrics_background(Configuration *config, char *value,
                                          int32 value_len, NcmError *error);
static bool apply_store_lyrics_in_song_dir(Configuration *config, char *value,
                                           int32 value_len, NcmError *error);
static bool apply_generate_win32_compatible_filenames(Configuration *config,
                                                      char *value,
                                                      int32 value_len,
                                                      NcmError *error);
static bool apply_allow_for_physical_item_deletion(Configuration *config,
                                                   char *value, int32 value_len,
                                                   NcmError *error);
static bool apply_lastfm_preferred_language(Configuration *config, char *value,
                                            int32 value_len, NcmError *error);
static bool apply_show_hidden_files_in_local_browser(Configuration *config,
                                                     char *value,
                                                     int32 value_len,
                                                     NcmError *error);
static bool apply_startup_slave_screen_focus(Configuration *config, char *value,
                                             int32 value_len, NcmError *error);
static bool apply_ask_for_locked_screen_width_part(Configuration *config,
                                                   char *value, int32 value_len,
                                                   NcmError *error);
static bool apply_jump_to_now_playing_song_at_start(Configuration *config,
                                                    char *value,
                                                    int32 value_len,
                                                    NcmError *error);
static bool apply_ask_before_clearing_playlists(Configuration *config,
                                                char *value, int32 value_len,
                                                NcmError *error);
static bool apply_ask_before_shuffling_playlists(Configuration *config,
                                                 char *value, int32 value_len,
                                                 NcmError *error);
static bool apply_display_volume_level(Configuration *config, char *value,
                                       int32 value_len, NcmError *error);
static bool apply_display_bitrate(Configuration *config, char *value,
                                  int32 value_len, NcmError *error);
static bool apply_display_remaining_time(Configuration *config, char *value,
                                         int32 value_len, NcmError *error);
static bool apply_ignore_leading_the(Configuration *config, char *value,
                                     int32 value_len, NcmError *error);
static bool apply_block_search_constraints_change(Configuration *config,
                                                  char *value, int32 value_len,
                                                  NcmError *error);
static bool apply_mouse_support(Configuration *config, char *value,
                                int32 value_len, NcmError *error);
static bool apply_mouse_list_scroll_whole_page(Configuration *config,
                                               char *value, int32 value_len,
                                               NcmError *error);
static bool apply_lines_scrolled(Configuration *config, char *value,
                                 int32 value_len, NcmError *error);
static bool apply_empty_tag_marker(Configuration *config, char *value,
                                   int32 value_len, NcmError *error);
static bool apply_tags_separator(Configuration *config, char *value,
                                 int32 value_len, NcmError *error);
static bool apply_tag_editor_extended_numeration(Configuration *config,
                                                 char *value, int32 value_len,
                                                 NcmError *error);
static bool apply_media_library_sort_by_mtime(Configuration *config,
                                              char *value, int32 value_len,
                                              NcmError *error);
static bool apply_external_editor(Configuration *config, char *value,
                                  int32 value_len, NcmError *error);
static bool apply_use_console_editor(Configuration *config, char *value,
                                     int32 value_len, NcmError *error);
static bool apply_colors_enabled(Configuration *config, char *value,
                                 int32 value_len, NcmError *error);

static void
settings_error(NcmError *error, char *message, int32 message_len) {
    ncm_error_set(error, EINVAL, message, message_len);
    return;
}

static void
settings_invalid_value(NcmError *error, char *value, int32 value_len) {
    char message[256];
    int32 len;

    len = SNPRINTF(message, "invalid value: %.*s", value_len, value);
    settings_error(error, message, len);
    return;
}

static int32
settings_trim_start(char *value, int32 value_len) {
    int32 result;

    result = 0;
    while (result < value_len) {
        uint8 c = (uint8)value[result];

        if (!isspace(c)) {
            break;
        }
        result += 1;
    }

    return result;
}

static int32
settings_trim_end(char *value, int32 value_len) {
    int32 result;

    result = value_len;
    while (result > 0) {
        uint8 c = (uint8)value[result - 1];

        if (!isspace(c)) {
            break;
        }
        result -= 1;
    }

    return result;
}

static bool
settings_string_set(char **data, int32 *len, int32 *cap, char *value,
                    int32 value_len) {
    char *new_data;
    int32 new_cap;

    if (value_len < 0) {
        value_len = 0;
    }
    new_cap = value_len + 1;
    new_data = malloc2(new_cap);
    if (value_len > 0) {
        memcpy64(new_data, value, value_len);
    }
    new_data[value_len] = '\0';

    if (*data) {
        free2(*data, *cap);
    }
    *data = new_data;
    *len = value_len;
    *cap = new_cap;
    return true;
}

static bool
settings_expand_home(NcmBuffer *buffer, char *value, int32 value_len) {
    char *home;
    int32 home_len;

    ncm_buffer_clear(buffer);
    if ((value_len <= 0) || (value[0] != '~')) {
        ncm_buffer_append(buffer, value, value_len);
        return true;
    }

    if ((home = getenv("HOME")) == NULL) {
        ncm_buffer_append(buffer, value, value_len);
        return true;
    }
    home_len = strlen32(home);
    ncm_buffer_append(buffer, home, home_len);
    if (value_len == 1) {
        return true;
    }
    ncm_buffer_append_byte(buffer, '/');
    if (value[1] == '/') {
        if (value_len > 2) {
            ncm_buffer_append(buffer, value + 2, value_len - 2);
        }
    } else {
        ncm_buffer_append(buffer, value + 1, value_len - 1);
    }
    return true;
}

static bool
settings_string_set_expanded(char **data, int32 *len, int32 *cap, char *value,
                             int32 value_len) {
    StrBuilder buffer;
    bool result;

    sb_init(&buffer);
    if ((result = settings_expand_home(&buffer, value, value_len))) {
        result = settings_string_set(data, len, cap, buffer.data, buffer.len);
    }
    sb_free(&buffer);
    return result;
}

static bool
settings_string_set_directory(char **data, int32 *len, int32 *cap, char *value,
                              int32 value_len) {
    StrBuilder buffer;
    bool result;

    sb_init(&buffer);
    if ((result = settings_expand_home(&buffer, value, value_len))
        && ((buffer.len <= 0) || (buffer.data[buffer.len - 1] != '/'))) {
        sb_append_byte(&buffer, '/');
    }
    if (result) {
        result = settings_string_set(data, len, cap, buffer.data, buffer.len);
    }
    sb_free(&buffer);
    return result;
}

static bool
settings_copy_buffer(NcmBuffer *buffer, char *value, int32 value_len) {
    ncm_buffer_clear(buffer);
    ncm_buffer_append(buffer, value, value_len);
    return true;
}

static bool
settings_copy_nc_buffer(NcBuffer *buffer, char *value, int32 value_len,
                        int32 *width, bool keep_existing) {
    NcmFormatAst ast;
    NcBuffer tmp;
    NcmError error;
    bool result;

    if (keep_existing && !nc_buffer_empty(buffer)) {
        return true;
    }

    ncm_error_clear(&error);
    ncm_format_ast_init(&ast);
    nc_buffer_init(&tmp);
    result = ncm_format_parse(&ast, value, value_len,
                              NCM_FORMAT_FLAG_COLOR | NCM_FORMAT_FLAG_FORMAT,
                              &error);
    if (result) {
        ncm_format_render_buffer(&ast, NULL, &tmp, NULL,
                                 NCM_FORMAT_FLAG_COLOR
                                     | NCM_FORMAT_FLAG_FORMAT);
        nc_buffer_destroy(buffer);
        nc_buffer_move(buffer, &tmp);
        if (width) {
            *width = utf8_width(nc_buffer_data(buffer), nc_buffer_len(buffer));
        }
    }
    nc_buffer_destroy(&tmp);
    ncm_format_ast_destroy(&ast);
    return result;
}

static bool
settings_parse_bool(char *value, int32 value_len, bool *result,
                    NcmError *error) {
    if (!ncm_option_parser_yes_no(value, value_len, result)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
settings_parse_int32(char *value, int32 value_len, int32 *result,
                     NcmError *error) {
    return ncm_parse_int32(value, value_len, result, error);
}

static bool
settings_parse_double(char *value, int32 value_len, double *result,
                      NcmError *error) {
    return ncm_parse_double(value, value_len, result, error);
}

static bool
settings_color_name(char *value, int32 value_len, bool background,
                    int16 *result) {
    if (STREQUAL(value, value_len, STRLIT_ARGS("black"))) {
        *result = COLOR_BLACK;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("red"))) {
        *result = COLOR_RED;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("green"))) {
        *result = COLOR_GREEN;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("yellow"))) {
        *result = COLOR_YELLOW;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("blue"))) {
        *result = COLOR_BLUE;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("magenta"))) {
        *result = COLOR_MAGENTA;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("cyan"))) {
        *result = COLOR_CYAN;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("white"))) {
        *result = COLOR_WHITE;
        return true;
    }
    if (background && STREQUAL(value, value_len, STRLIT_ARGS("transparent"))) {
        *result = NC_COLOR_TRANSPARENT;
        return true;
    }
    if (background && STREQUAL(value, value_len, STRLIT_ARGS("current"))) {
        *result = NC_COLOR_CURRENT;
        return true;
    }

    return false;
}

static bool
settings_parse_single_color(char *value, int32 value_len, bool background,
                            int16 *result, NcmError *error) {
    int32 parsed;

    if (settings_color_name(value, value_len, background, result)) {
        return true;
    }
    if (!settings_parse_int32(value, value_len, &parsed, error)) {
        return false;
    }
    if (background) {
        if (parsed > 256) {
            settings_invalid_value(error, value, value_len);
            return false;
        }
    } else if ((parsed < 1) || (parsed > 256)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    if (parsed == 0) {
        *result = 0;
    } else {
        *result = (int16)(parsed - 1);
    }
    return true;
}

static bool
settings_parse_color(char *value, int32 value_len, NcColor *color,
                     NcmError *error) {
    int32 underscore;
    int16 foreground;
    int16 background;

    if (STREQUAL(value, value_len, STRLIT_ARGS("default"))) {
        *color = nc_color_default();
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("end"))) {
        *color = nc_color_end();
        return true;
    }

    underscore = ncm_string_find_char(value, value_len, '_');
    if (underscore < 0) {
        if (!settings_parse_single_color(value, value_len, false, &foreground,
                                         error)) {
            return false;
        }
        *color = nc_color_make(foreground, NC_COLOR_CURRENT, false, false);
        return true;
    }

    if (!settings_parse_single_color(value, underscore, false, &foreground,
                                     error)) {
        return false;
    }
    if (!settings_parse_single_color(value + underscore + 1,
                                     value_len - underscore - 1, true,
                                     &background, error)) {
        return false;
    }
    *color = nc_color_make(foreground, background, false, false);
    return true;
}

static bool
settings_parse_formatted_color(char *value, int32 value_len,
                               NcFormattedColor *color, NcmError *error) {
    int32 colon;
    NcColor base;
    NcFormattedColor tmp;

    colon = ncm_string_find_char(value, value_len, ':');
    if (colon < 0) {
        colon = value_len;
    }
    if (!settings_parse_color(value, colon, &base, error)) {
        return false;
    }
    if (nc_color_is_end(base)) {
        settings_invalid_value(error, value, value_len);
        return false;
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
            settings_invalid_value(error, value, value_len);
            return false;
        }
    }

    nc_formatted_color_destroy(color);
    nc_formatted_color_move(color, &tmp);
    return true;
}

static bool
settings_next_list_item(char *value, int32 value_len, int32 *pos, char **item,
                        int32 *item_len) {
    int32 start;
    int32 end;
    bool quoted;

    if (*pos > value_len) {
        return false;
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

    start += settings_trim_start(value + start, end - start);
    end = start + settings_trim_end(value + start, end - start);
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
    return true;
}

static bool
settings_parse_formatted_color_list(NcmFormattedColorArray *array, char *value,
                                    int32 value_len, NcmError *error) {
    int32 pos;
    bool added;

    ncm_formatted_color_array_clear(array);
    pos = 0;
    added = false;
    while (pos <= value_len) {
        char *item;
        int32 item_len;
        NcFormattedColor *dest;

        if (!settings_next_list_item(value, value_len, &pos, &item,
                                     &item_len)) {
            break;
        }
        if (item_len <= 0) {
            continue;
        }
        if ((dest = ncm_formatted_color_array_append(array)) == NULL) {
            settings_error(error, STRLIT_ARGS("failed to append color"));
            return false;
        }
        if (!settings_parse_formatted_color(item, item_len, dest, error)) {
            return false;
        }
        added = true;
    }
    if (!added) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
settings_parse_ratio(NcmInt32Array *array, char *value, int32 value_len,
                     int32 expected_len, NcmError *error) {
    int32 start;
    int32 total;

    ncm_int32_array_clear(array);
    start = 0;
    total = 0;
    while (start <= value_len) {
        int32 end;
        int32 parsed;
        int32 *slot;

        end = start;
        while ((end < value_len) && (value[end] != ':')) {
            end += 1;
        }
        if (!settings_parse_int32(value + start, end - start, &parsed, error)) {
            return false;
        }
        if ((slot = ncm_int32_array_append(array)) == NULL) {
            settings_error(error, STRLIT_ARGS("failed to append ratio"));
            return false;
        }
        *slot = parsed;
        total += parsed;
        if (end >= value_len) {
            break;
        }
        start = end + 1;
    }
    if ((array->len != expected_len) || (total == 0)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
settings_column_append_type(Column *column, char ch) {
    char *new_data;
    int32 new_len;
    int32 new_cap;

    new_len = column->type_len + 1;
    new_cap = new_len + 1;
    new_data = malloc2(new_cap);
    if (column->type_len > 0) {
        memcpy64(new_data, column->type, column->type_len);
    }
    new_data[column->type_len] = ch;
    new_data[new_len] = '\0';
    if (column->type) {
        free2(column->type, column->type_cap);
    }
    column->type = new_data;
    column->type_len = new_len;
    column->type_cap = new_cap;
    return true;
}

static bool
settings_parse_columns(Configuration *config, char *value, int32 value_len,
                       NcmError *error) {
    int32 pos;
    int32 last_relative;
    int32 stretch_limit;

    column_array_clear(&config->columns);
    ncm_format_ast_clear(&config->song_columns_mode_format);
    pos = 0;
    while (pos < value_len) {
        StrBuilder width;
        StrBuilder color;
        StrBuilder tag;
        Column *column;
        int32 next;
        int32 parsed_width;

        sb_init(&width);
        sb_init(&color);
        sb_init(&tag);
        width = ncm_string_get_enclosed(value, value_len, '(', ')', pos, &next);
        if (width.len <= 0) {
            sb_free(&width);
            sb_free(&color);
            sb_free(&tag);
            break;
        }
        pos = next;
        color = ncm_string_get_enclosed(value, value_len, '[', ']', pos, &next);
        pos = next;
        tag = ncm_string_get_enclosed(value, value_len, '{', '}', pos, &next);
        pos = next;
        if ((column = column_array_append(&config->columns)) == NULL) {
            settings_error(error, STRLIT_ARGS("failed to append column"));
            sb_free(&width);
            sb_free(&color);
            sb_free(&tag);
            return false;
        }
        if ((width.len > 0) && (width.data[width.len - 1] == 'f')) {
            column->fixed = true;
            width.len -= 1;
            width.data[width.len] = '\0';
        }
        if (!settings_parse_int32(width.data, width.len, &parsed_width,
                                   error)) {
            sb_free(&width);
            sb_free(&color);
            sb_free(&tag);
            return false;
        }
        column->width = parsed_width;
        if (color.len > 0) {
            if (!settings_parse_color(color.data, color.len, &column->color,
                                      error)) {
                sb_free(&width);
                sb_free(&color);
                sb_free(&tag);
                return false;
            }
        }
        if (tag.len > 0) {
            int32 colon;
            int32 type_len;

            colon = ncm_string_find_char(tag.data, tag.len, ':');
            type_len = tag.len;
            if (colon >= 0) {
                settings_string_set(&column->name, &column->name_len,
                                    &column->name_cap, tag.data + colon + 1,
                                    tag.len - colon - 1);
                type_len = colon;
            }
            for (int32 i = 0; i < type_len; i += 1) {
                char ch;

                ch = tag.data[i];
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
                        settings_column_append_type(column, ch);
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

    if (config->columns.len <= 0) {
        settings_invalid_value(error, value, value_len);
        return false;
    }

    last_relative = -1;
    stretch_limit = 0;
    for (int32 i = config->columns.len - 1; i >= 0; i -= 1) {
        if (config->columns.items[i].fixed) {
            stretch_limit += config->columns.items[i].width;
        } else {
            last_relative = i;
            break;
        }
    }
    if (last_relative >= 0) {
        config->columns.items[last_relative].stretch_limit = stretch_limit;
    }

    for (int32 i = 0; i < config->columns.len; i += 1) {
        Column *column;

        column = &config->columns.items[i];
        if (!ncm_format_ast_append_column_types(
                &config->song_columns_mode_format, column->type,
                column->type_len)) {
            settings_error(error, STRLIT_ARGS("failed to build column format"));
            return false;
        }
    }
    return true;
}

static bool
settings_parse_screen_list(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    int32 pos;
    bool added;

    screen_type_array_clear(&config->screen_sequence);
    pos = 0;
    added = false;
    while (pos <= value_len) {
        char *item;
        int32 item_len;
        enum ScreenType *slot;
        enum ScreenType screen;

        if (!settings_next_list_item(value, value_len, &pos, &item,
                                     &item_len)) {
            break;
        }
        if (item_len <= 0) {
            continue;
        }
        if (!screen_type_parse_startup(item, item_len, &screen)) {
            settings_invalid_value(error, item, item_len);
            return false;
        }
        slot = screen_type_array_append(&config->screen_sequence);
        if (slot == NULL) {
            settings_error(error, STRLIT_ARGS("failed to append screen"));
            return false;
        }
        *slot = screen;
        added = true;
    }
    if (!added) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
settings_parse_lyrics_fetchers(Configuration *config, char *value,
                               int32 value_len, NcmError *error) {
    int32 pos;
    bool added;

    ncm_lyrics_fetcher_registry_clear(&config->lyrics_fetchers);
    pos = 0;
    added = false;
    while (pos <= value_len) {
        char *item;
        int32 item_len;
        if (!settings_next_list_item(value, value_len, &pos, &item,
                                     &item_len)) {
            break;
        }
        if (item_len <= 0) {
            continue;
        }
        if (!ncm_lyrics_fetcher_registry_append_name(&config->lyrics_fetchers,
                                                     item, item_len)) {
            settings_error(error, STRLIT_ARGS("unknown lyrics fetcher"));
            return false;
        }
        added = true;
    }
    if (!added) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

#define APPLY_STRING_DIR(FUNC, FIELD) \
    static bool \
    FUNC(Configuration *config, char *value, int32 value_len, \
         NcmError *error) { \
        (void)error; \
        return settings_string_set_directory(&config->FIELD, \
                                             &config->FIELD##_len, \
                                             &config->FIELD##_cap, \
                                             value, value_len); \
    }

#define APPLY_STRING_PATH(FUNC, FIELD) \
    static bool \
    FUNC(Configuration *config, char *value, int32 value_len, \
         NcmError *error) { \
        (void)error; \
        return settings_string_set_expanded(&config->FIELD, \
                                            &config->FIELD##_len, \
                                            &config->FIELD##_cap, \
                                            value, value_len); \
    }

#define APPLY_STRING(FUNC, FIELD) \
    static bool \
    FUNC(Configuration *config, char *value, int32 value_len, \
         NcmError *error) { \
        (void)error; \
        return settings_string_set(&config->FIELD, &config->FIELD##_len, \
                                   &config->FIELD##_cap, value, value_len); \
    }

#define APPLY_BOOL(FUNC, FIELD) \
    static bool \
    FUNC(Configuration *config, char *value, int32 value_len, \
         NcmError *error) { \
        return settings_parse_bool(value, value_len, &config->FIELD, error); \
    }

#define APPLY_UINT(FUNC, FIELD) \
    static bool \
    FUNC(Configuration *config, char *value, int32 value_len, \
         NcmError *error) { \
        return settings_parse_int32(value, value_len, &config->FIELD, \
                                     error); \
    }

APPLY_STRING_DIR(apply_ncmpcpp_directory, ncmpcpp_directory)
APPLY_STRING_DIR(apply_lyrics_directory, lyrics_directory)
APPLY_STRING_DIR(apply_mpd_music_dir, mpd_music_dir)
APPLY_STRING(apply_random_exclude_pattern, random_exclude_pattern)
APPLY_STRING_PATH(apply_visualizer_data_source, visualizer_data_source)
APPLY_STRING(apply_visualizer_output_name, visualizer_output_name)
APPLY_BOOL(apply_visualizer_in_stereo, visualizer_in_stereo)
APPLY_BOOL(apply_visualizer_autoscale, visualizer_autoscale)
APPLY_BOOL(apply_visualizer_spectrum_smooth_look,
           visualizer_spectrum_smooth_look)
APPLY_BOOL(apply_visualizer_spectrum_smooth_look_legacy_chars,
           visualizer_spectrum_smooth_look_legacy_chars)
APPLY_BOOL(apply_visualizer_spectrum_log_scale_x,
           visualizer_spectrum_log_scale_x)
APPLY_BOOL(apply_visualizer_spectrum_log_scale_y,
           visualizer_spectrum_log_scale_y)
APPLY_UINT(apply_message_delay_time, message_delay_time)
APPLY_STRING_PATH(apply_execute_on_song_change, execute_on_song_change)
APPLY_STRING_PATH(apply_execute_on_player_state_change,
                  execute_on_player_state_change)
APPLY_BOOL(apply_playlist_show_mpd_host, playlist_show_mpd_host)
APPLY_BOOL(apply_playlist_show_remaining_time, playlist_show_remaining_time)
APPLY_BOOL(apply_playlist_shorten_total_times, playlist_shorten_total_times)
APPLY_BOOL(apply_playlist_separate_albums, playlist_separate_albums)
APPLY_BOOL(apply_discard_colors_if_item_is_selected,
           discard_colors_if_item_is_selected)
APPLY_BOOL(apply_show_duplicate_tags, show_duplicate_tags)
APPLY_BOOL(apply_incremental_seeking, incremental_seeking)
APPLY_UINT(apply_seek_time, seek_time)
APPLY_UINT(apply_volume_change_step, volume_change_step)
APPLY_BOOL(apply_autocenter_mode, autocenter_mode)
APPLY_BOOL(apply_centered_cursor, centered_cursor)
APPLY_BOOL(apply_data_fetching_delay, data_fetching_delay)
APPLY_BOOL(apply_media_library_hide_album_dates, media_lib_hide_album_dates)
APPLY_BOOL(apply_media_library_albums_split_by_date,
           media_library_albums_split_by_date)
APPLY_STRING(apply_default_tag_editor_pattern, pattern)
APPLY_BOOL(apply_header_visibility, header_visibility)
APPLY_BOOL(apply_statusbar_visibility, statusbar_visibility)
APPLY_BOOL(apply_connected_message_on_startup, connected_message_on_startup)
APPLY_BOOL(apply_titles_visibility, titles_visibility)
APPLY_BOOL(apply_header_text_scrolling, header_text_scrolling)
APPLY_BOOL(apply_cyclic_scrolling, use_cyclic_scrolling)
APPLY_BOOL(apply_follow_now_playing_lyrics, now_playing_lyrics)
APPLY_BOOL(apply_fetch_lyrics_background, fetch_lyrics_in_background)
APPLY_BOOL(apply_store_lyrics_in_song_dir, store_lyrics_in_song_dir)
APPLY_BOOL(apply_generate_win32_compatible_filenames,
           generate_win32_compatible_filenames)
APPLY_BOOL(apply_allow_for_physical_item_deletion,
           allow_for_physical_item_deletion)
APPLY_STRING(apply_lastfm_preferred_language, lastfm_preferred_language)
APPLY_BOOL(apply_show_hidden_files_in_local_browser,
           local_browser_show_hidden_files)
APPLY_BOOL(apply_startup_slave_screen_focus, startup_slave_screen_focus)
APPLY_BOOL(apply_ask_for_locked_screen_width_part,
           ask_for_locked_screen_width_part)
APPLY_BOOL(apply_jump_to_now_playing_song_at_start,
           jump_to_now_playing_song_at_start)
APPLY_BOOL(apply_ask_before_clearing_playlists, ask_before_clearing_playlists)
APPLY_BOOL(apply_ask_before_shuffling_playlists, ask_before_shuffling_playlists)
APPLY_BOOL(apply_display_volume_level, display_volume_level)
APPLY_BOOL(apply_display_bitrate, display_bitrate)
APPLY_BOOL(apply_display_remaining_time, display_remaining_time)
APPLY_BOOL(apply_ignore_leading_the, ignore_leading_the)
APPLY_BOOL(apply_block_search_constraints_change,
           block_search_constraints_change)
APPLY_BOOL(apply_mouse_support, mouse_support)
APPLY_BOOL(apply_mouse_list_scroll_whole_page, mouse_list_scroll_whole_page)
APPLY_UINT(apply_lines_scrolled, lines_scrolled)
APPLY_STRING(apply_empty_tag_marker, empty_tag)
APPLY_STRING(apply_tags_separator, tags_separator)
APPLY_BOOL(apply_tag_editor_extended_numeration, tag_editor_extended_numeration)
APPLY_BOOL(apply_media_library_sort_by_mtime, media_library_sort_by_mtime)
APPLY_STRING_PATH(apply_external_editor, external_editor)
APPLY_BOOL(apply_use_console_editor, use_console_editor)
APPLY_BOOL(apply_colors_enabled, colors_enabled)

static bool
apply_mpd_host(Configuration *config, char *value, int32 value_len,
               NcmError *error) {
    StrBuilder host;
    bool result;

    (void)config;
    sb_init(&host);
    if ((result = settings_expand_home(&host, value, value_len))) {
        result = ncm_mpd_client_set_hostname(&global_mpd, host.data, host.len,
                                             error);
    }
    sb_free(&host);
    return result;
}

static bool
apply_mpd_port(Configuration *config, char *value, int32 value_len,
               NcmError *error) {
    int32 port;

    (void)config;
    if (!settings_parse_int32(value, value_len, &port, error)) {
        return false;
    }
    if (port > 65535) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    ncm_mpd_client_set_port(&global_mpd, (uint16)port);
    return true;
}

static bool
apply_mpd_password(Configuration *config, char *value, int32 value_len,
                   NcmError *error) {
    (void)config;
    if (value_len <= 0) {
        return true;
    }
    return ncm_mpd_client_set_password(&global_mpd, value, value_len, error);
}

static bool
apply_mpd_connection_timeout(Configuration *config, char *value,
                             int32 value_len, NcmError *error) {
    if (!settings_parse_int32(value, value_len,
                               &config->mpd_connection_timeout, error)) {
        return false;
    }
    return ncm_mpd_client_set_timeout_ms(
        &global_mpd, config->mpd_connection_timeout*1000, error);
}

static bool
apply_mpd_crossfade_time(Configuration *config, char *value, int32 value_len,
                         NcmError *error) {
    return settings_parse_int32(value, value_len, &config->crossfade_time,
                                 error);
}

static bool
apply_visualizer_type(Configuration *config, char *value, int32 value_len,
                      NcmError *error) {
    if (!ncm_visualizer_type_parse(value, value_len,
                                   &config->visualizer_type)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_visualizer_look(Configuration *config, char *value, int32 value_len,
                      NcmError *error) {
    if (utf8_characters(value, value_len) != 2) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return settings_copy_buffer(&config->visualizer_chars, value, value_len);
}

static bool
apply_visualizer_fps(Configuration *config, char *value, int32 value_len,
                     NcmError *error) {
    if (!settings_parse_int32(value, value_len, &config->visualizer_fps,
                               error)) {
        return false;
    }
    return ncm_bounds_check_i64(config->visualizer_fps, 30, 1000, error);
}

static bool
apply_visualizer_spectrum_dft_size(Configuration *config, char *value,
                                   int32 value_len, NcmError *error) {
    if (!settings_parse_int32(value, value_len,
                               &config->visualizer_spectrum_dft_size, error)) {
        return false;
    }
    return ncm_bounds_check_i64(config->visualizer_spectrum_dft_size, 1, 5,
                                error);
}

static bool
apply_visualizer_spectrum_gain(Configuration *config, char *value,
                               int32 value_len, NcmError *error) {
    if (!settings_parse_double(value, value_len,
                               &config->visualizer_spectrum_gain, error)) {
        return false;
    }
    return ncm_bounds_check_f64(config->visualizer_spectrum_gain, 0, 100,
                                error);
}

static bool
apply_visualizer_spectrum_hz_min(Configuration *config, char *value,
                                 int32 value_len, NcmError *error) {
    if (!settings_parse_double(value, value_len,
                               &config->visualizer_spectrum_hz_min, error)) {
        return false;
    }
    return ncm_lower_bound_check_f64(config->visualizer_spectrum_hz_min, 1,
                                     error);
}

static bool
apply_visualizer_spectrum_hz_max(Configuration *config, char *value,
                                 int32 value_len, NcmError *error) {
    if (!settings_parse_double(value, value_len,
                               &config->visualizer_spectrum_hz_max, error)) {
        return false;
    }
    return ncm_lower_bound_check_f64(config->visualizer_spectrum_hz_max,
                                     config->visualizer_spectrum_hz_min + 1,
                                     error);
}

static bool
apply_visualizer_color(Configuration *config, char *value, int32 value_len,
                       NcmError *error) {
    return settings_parse_formatted_color_list(&config->visualizer_colors,
                                               value, value_len, error);
}

static bool
settings_parse_format(NcmFormatAst *format, char *value, int32 value_len,
                      uint32 flags, NcmError *error) {
    ncm_format_ast_clear(format);
    return ncm_format_parse(format, value, value_len, flags, error);
}

static bool
apply_system_encoding(Configuration *config, char *value, int32 value_len,
                      NcmError *error) {
    (void)value;
    (void)value_len;
    (void)error;
    return settings_string_set(&config->system_encoding,
                               &config->system_encoding_len,
                               &config->system_encoding_cap, "", 0);
}

static bool
apply_playlist_disable_highlight_delay(Configuration *config, char *value,
                                       int32 value_len, NcmError *error) {
    return settings_parse_int32(
        value, value_len, &config->playlist_disable_highlight_delay_seconds,
        error);
}

static bool
apply_song_list_format(Configuration *config, char *value, int32 value_len,
                       NcmError *error) {
    return settings_parse_format(&config->song_list_format, value, value_len,
                                 NCM_FORMAT_FLAG_ALL, error);
}

static bool
apply_song_status_format(Configuration *config, char *value, int32 value_len,
                         NcmError *error) {
    return settings_parse_format(
        &config->song_status_format, value, value_len,
        NCM_FORMAT_FLAG_ALL ^ NCM_FORMAT_FLAG_OUTPUT_SWITCH, error);
}

static bool
apply_song_library_format(Configuration *config, char *value, int32 value_len,
                          NcmError *error) {
    return settings_parse_format(&config->song_library_format, value, value_len,
                                 NCM_FORMAT_FLAG_ALL, error);
}

static bool
apply_header_first_line_format(Configuration *config, char *value,
                               int32 value_len, NcmError *error) {
    return settings_parse_format(
        &config->new_header_first_line, value, value_len,
        NCM_FORMAT_FLAG_ALL ^ NCM_FORMAT_FLAG_OUTPUT_SWITCH, error);
}

static bool
apply_header_second_line_format(Configuration *config, char *value,
                                int32 value_len, NcmError *error) {
    return settings_parse_format(
        &config->new_header_second_line, value, value_len,
        NCM_FORMAT_FLAG_ALL ^ NCM_FORMAT_FLAG_OUTPUT_SWITCH, error);
}

static bool
apply_current_item_prefix(Configuration *config, char *value, int32 value_len,
                          NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(&config->current_item_prefix, value,
                                   value_len,
                                   &config->current_item_prefix_length, true);
}

static bool
apply_current_item_suffix(Configuration *config, char *value, int32 value_len,
                          NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(&config->current_item_suffix, value,
                                   value_len,
                                   &config->current_item_suffix_length, true);
}

static bool
apply_current_item_inactive_column_prefix(Configuration *config, char *value,
                                          int32 value_len, NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(
        &config->current_item_inactive_column_prefix, value, value_len,
        &config->current_item_inactive_column_prefix_length, true);
}

static bool
apply_current_item_inactive_column_suffix(Configuration *config, char *value,
                                          int32 value_len, NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(
        &config->current_item_inactive_column_suffix, value, value_len,
        &config->current_item_inactive_column_suffix_length, true);
}

static bool
apply_now_playing_prefix(Configuration *config, char *value, int32 value_len,
                         NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(&config->now_playing_prefix, value,
                                   value_len,
                                   &config->now_playing_prefix_length, false);
}

static bool
apply_now_playing_suffix(Configuration *config, char *value, int32 value_len,
                         NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(&config->now_playing_suffix, value,
                                   value_len,
                                   &config->now_playing_suffix_length, false);
}

static bool
apply_browser_playlist_prefix(Configuration *config, char *value,
                              int32 value_len, NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(&config->browser_playlist_prefix, value,
                                   value_len, NULL, false);
}

static bool
apply_selected_item_prefix(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(&config->selected_item_prefix, value,
                                   value_len,
                                   &config->selected_item_prefix_length, false);
}

static bool
apply_selected_item_suffix(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(&config->selected_item_suffix, value,
                                   value_len,
                                   &config->selected_item_suffix_length, false);
}

static bool
apply_modified_item_prefix(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    (void)error;
    return settings_copy_nc_buffer(&config->modified_item_prefix, value,
                                   value_len, NULL, false);
}

static bool
apply_song_window_title_format(Configuration *config, char *value,
                               int32 value_len, NcmError *error) {
    return settings_parse_format(&config->song_window_title_format, value,
                                 value_len, NCM_FORMAT_FLAG_TAG, error);
}

static bool
apply_browser_sort_mode(Configuration *config, char *value, int32 value_len,
                        NcmError *error) {
    if (STREQUAL(value, value_len, STRLIT_ARGS("noop"))) {
        value = "none";
        value_len = STRLIT_LEN("none");
    }
    if (!ncm_sort_mode_parse(value, value_len, &config->browser_sort_mode)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_browser_sort_format(Configuration *config, char *value, int32 value_len,
                          NcmError *error) {
    return settings_parse_format(&config->browser_sort_format, value, value_len,
                                 NCM_FORMAT_FLAG_TAG, error);
}

static bool
apply_song_columns_list_format(Configuration *config, char *value,
                               int32 value_len, NcmError *error) {
    return settings_parse_columns(config, value, value_len, error);
}

static bool
apply_playlist_display_mode(Configuration *config, char *value, int32 value_len,
                            NcmError *error) {
    if (!ncm_display_mode_parse(value, value_len,
                                &config->playlist_display_mode)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_browser_display_mode(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    if (!ncm_display_mode_parse(value, value_len,
                                &config->browser_display_mode)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_search_engine_display_mode(Configuration *config, char *value,
                                 int32 value_len, NcmError *error) {
    if (!ncm_display_mode_parse(value, value_len,
                                &config->search_engine_display_mode)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_playlist_editor_display_mode(Configuration *config, char *value,
                                   int32 value_len, NcmError *error) {
    if (!ncm_display_mode_parse(value, value_len,
                                &config->playlist_editor_display_mode)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_progressbar_look(Configuration *config, char *value, int32 value_len,
                       NcmError *error) {
    int32 characters;

    characters = utf8_characters(value, value_len);
    if ((characters < 2) || (characters > 3)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    sb_clear(&config->progressbar);
    sb_append(&config->progressbar, value, value_len);
    if (characters == 2) {
        sb_append_byte(&config->progressbar, '\0');
    }
    return true;
}

static bool
apply_default_place_to_search_in(Configuration *config, char *value,
                                 int32 value_len, NcmError *error) {
    if (STREQUAL(value, value_len, STRLIT_ARGS("database"))) {
        config->search_in_db = true;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("playlist"))) {
        config->search_in_db = false;
        return true;
    }
    settings_invalid_value(error, value, value_len);
    return false;
}

static bool
apply_user_interface(Configuration *config, char *value, int32 value_len,
                     NcmError *error) {
    if (!ncm_design_parse(value, value_len, &config->design)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_media_library_primary_tag(Configuration *config, char *value,
                                int32 value_len, NcmError *error) {
    if (STREQUAL(value, value_len, STRLIT_ARGS("artist"))) {
        config->media_lib_primary_tag = MPD_TAG_ARTIST;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("album_artist"))) {
        config->media_lib_primary_tag = MPD_TAG_ALBUM_ARTIST;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("date"))) {
        config->media_lib_primary_tag = MPD_TAG_DATE;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("genre"))) {
        config->media_lib_primary_tag = MPD_TAG_GENRE;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("composer"))) {
        config->media_lib_primary_tag = MPD_TAG_COMPOSER;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("performer"))) {
        config->media_lib_primary_tag = MPD_TAG_PERFORMER;
        return true;
    }
    settings_invalid_value(error, value, value_len);
    return false;
}

static bool
apply_default_find_mode(Configuration *config, char *value, int32 value_len,
                        NcmError *error) {
    if (STREQUAL(value, value_len, STRLIT_ARGS("wrapped"))) {
        config->wrapped_search = true;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("normal"))) {
        config->wrapped_search = false;
        return true;
    }
    settings_invalid_value(error, value, value_len);
    return false;
}

static bool
apply_lyrics_fetchers(Configuration *config, char *value, int32 value_len,
                      NcmError *error) {
    return settings_parse_lyrics_fetchers(config, value, value_len, error);
}

static bool
apply_space_add_mode(Configuration *config, char *value, int32 value_len,
                     NcmError *error) {
    if (!ncm_space_add_mode_parse(value, value_len, &config->space_add_mode)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_screen_switcher_mode(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    if (STREQUAL(value, value_len, STRLIT_ARGS("previous"))) {
        config->screen_switcher_previous = true;
        screen_type_array_clear(&config->screen_sequence);
        return true;
    }
    config->screen_switcher_previous = false;
    return settings_parse_screen_list(config, value, value_len, error);
}

static bool
apply_startup_screen(Configuration *config, char *value, int32 value_len,
                     NcmError *error) {
    if (!screen_type_parse_startup(value, value_len,
                                   &config->startup_screen_type)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    return true;
}

static bool
apply_startup_slave_screen(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    if (value_len <= 0) {
        config->has_startup_slave_screen_type = false;
        config->startup_slave_screen_type = NCM_SCREEN_TYPE_UNKNOWN;
        return true;
    }
    if (!screen_type_parse_startup(value, value_len,
                                   &config->startup_slave_screen_type)) {
        settings_invalid_value(error, value, value_len);
        return false;
    }
    config->has_startup_slave_screen_type = true;
    return true;
}

static bool
apply_locked_screen_width_part(Configuration *config, char *value,
                               int32 value_len, NcmError *error) {
    if (!settings_parse_double(value, value_len,
                               &config->locked_screen_width_part, error)) {
        return false;
    }
    config->locked_screen_width_part /= 100.0;
    return true;
}

static bool
apply_media_library_column_width_ratio_two(Configuration *config, char *value,
                                           int32 value_len, NcmError *error) {
    return settings_parse_ratio(&config->media_library_column_width_ratio_two,
                                value, value_len, 2, error);
}

static bool
apply_media_library_column_width_ratio_three(Configuration *config, char *value,
                                             int32 value_len, NcmError *error) {
    return settings_parse_ratio(&config->media_library_column_width_ratio_three,
                                value, value_len, 3, error);
}

static bool
apply_playlist_editor_column_width_ratio(Configuration *config, char *value,
                                         int32 value_len, NcmError *error) {
    return settings_parse_ratio(&config->playlist_editor_column_width_ratio,
                                value, value_len, 2, error);
}

static bool
apply_regular_expressions(Configuration *config, char *value, int32 value_len,
                          NcmError *error) {
    if (STREQUAL(value, value_len, STRLIT_ARGS("none"))) {
        config->regex_flags = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("basic"))) {
        config->regex_flags = NCM_REGEX_BASIC_CASE_INSENSITIVE;
        return true;
    }
    if (STREQUAL(value, value_len, STRLIT_ARGS("extended"))) {
        config->regex_flags = NCM_REGEX_EXTENDED_CASE_INSENSITIVE;
        return true;
    }
    settings_invalid_value(error, value, value_len);
    return false;
}

static bool
apply_enable_window_title(Configuration *config, char *value, int32 value_len,
                          NcmError *error) {
    char *term;
    int32 term_len;
    bool unsupported;

    term = getenv("TERM");
    unsupported = term == NULL;
    if (!unsupported) {
        term_len = strlen32(term);
        unsupported = memmem64(term, term_len, STRLIT_ARGS("linux"))
                      || BEGINS_WITH(term, term_len, "eterm");
    }
    if (unsupported) {
        config->set_window_title = false;
        if (!settings_quiet) {
            fprintf(stderr, "Terminal doesn't support window title, skipping "
                            "'enable_window_title'.\n");
        }
        return true;
    }
    return settings_parse_bool(value, value_len, &config->set_window_title,
                               error);
}

static bool
apply_search_engine_default_search_mode(Configuration *config, char *value,
                                        int32 value_len, NcmError *error) {
    int32 mode;

    if (!settings_parse_int32(value, value_len, &mode, error)) {
        return false;
    }
    if (!ncm_bounds_check_i64(mode, 1, 3, error)) {
        return false;
    }
    config->search_engine_default_search_mode = mode - 1;
    return true;
}

static bool
apply_empty_tag_color(Configuration *config, char *value, int32 value_len,
                      NcmError *error) {
    return settings_parse_formatted_color(value, value_len,
                                          &config->empty_tags_color, error);
}

static bool
apply_header_window_color(Configuration *config, char *value, int32 value_len,
                          NcmError *error) {
    return settings_parse_color(value, value_len, &config->header_color, error);
}

static bool
apply_volume_color(Configuration *config, char *value, int32 value_len,
                   NcmError *error) {
    return settings_parse_formatted_color(value, value_len,
                                          &config->volume_color, error);
}

static bool
apply_state_line_color(Configuration *config, char *value, int32 value_len,
                       NcmError *error) {
    return settings_parse_formatted_color(value, value_len,
                                          &config->state_line_color, error);
}

static bool
apply_state_flags_color(Configuration *config, char *value, int32 value_len,
                        NcmError *error) {
    return settings_parse_formatted_color(value, value_len,
                                          &config->state_flags_color, error);
}

static bool
apply_main_window_color(Configuration *config, char *value, int32 value_len,
                        NcmError *error) {
    return settings_parse_color(value, value_len, &config->main_color, error);
}

static bool
apply_color1(Configuration *config, char *value, int32 value_len,
             NcmError *error) {
    return settings_parse_formatted_color(value, value_len, &config->color1,
                                          error);
}

static bool
apply_color2(Configuration *config, char *value, int32 value_len,
             NcmError *error) {
    return settings_parse_formatted_color(value, value_len, &config->color2,
                                          error);
}

static bool
apply_progressbar_color(Configuration *config, char *value, int32 value_len,
                        NcmError *error) {
    return settings_parse_formatted_color(value, value_len,
                                          &config->progressbar_color, error);
}

static bool
apply_progressbar_elapsed_color(Configuration *config, char *value,
                                int32 value_len, NcmError *error) {
    return settings_parse_formatted_color(
        value, value_len, &config->progressbar_elapsed_color, error);
}

static bool
apply_statusbar_color(Configuration *config, char *value, int32 value_len,
                      NcmError *error) {
    return settings_parse_color(value, value_len, &config->statusbar_color,
                                error);
}

static bool
apply_statusbar_time_color(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    return settings_parse_formatted_color(value, value_len,
                                          &config->statusbar_time_color, error);
}

static bool
apply_player_state_color(Configuration *config, char *value, int32 value_len,
                         NcmError *error) {
    return settings_parse_formatted_color(value, value_len,
                                          &config->player_state_color, error);
}

static bool
apply_alternative_ui_separator_color(Configuration *config, char *value,
                                     int32 value_len, NcmError *error) {
    return settings_parse_formatted_color(
        value, value_len, &config->alternative_ui_separator_color, error);
}

static bool
apply_window_border_color(Configuration *config, char *value, int32 value_len,
                          NcmError *error) {
    NcColor color;

    if (!settings_parse_color(value, value_len, &color, error)) {
        return false;
    }
    config->window_border = nc_border_make(color);
    return true;
}

static bool
apply_active_window_border(Configuration *config, char *value, int32 value_len,
                           NcmError *error) {
    NcColor color;

    if (!settings_parse_color(value, value_len, &color, error)) {
        return false;
    }
    config->active_window_border = nc_border_make(color);
    return true;
}

static SettingsOption *
settings_find_option(SettingsOption *options, int32 option_count, char *name,
                     int32 name_len) {
    for (int32 i = 0; i < option_count; i += 1) {
        if (STREQUAL(name, name_len, options[i].name, options[i].name_len)) {
            return &options[i];
        }
    }
    return NULL;
}

static void
settings_set_option_error(NcmError *error, bool default_value,
                          SettingsOption *option, NcmError *cause) {
    char message[256];
    char *phase;
    char *detail;
    int32 detail_len;
    int32 len;

    if (default_value) {
        phase = "initializing";
    } else {
        phase = "processing";
    }
    detail = "invalid value";
    detail_len = STRLIT_LEN("invalid value");
    if (ncm_error_is_set(cause)) {
        detail = cause->message;
        detail_len = strlen32(cause->message);
    }

    len = SNPRINTF(message, "error while %s option \"%.*s\": %.*s", phase,
                   option->name_len, option->name, detail_len, detail);
    if (len < 0) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("error while processing option"));
        return;
    }
    if (len >= SIZEOF(message)) {
        len = SIZEOF(message) - 1;
    }
    if (ncm_error_is_set(cause)) {
        ncm_error_set(error, cause->code, message, len);
    } else {
        ncm_error_set(error, EINVAL, message, len);
    }
    return;
}

static void
settings_set_unknown_option_error(NcmError *error, char *option,
                                  int32 option_len) {
    char message[256];
    int32 len;

    len = SNPRINTF(message, "unknown option: %.*s", option_len, option);
    if (len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("unknown option"));
        return;
    }
    if (len >= SIZEOF(message)) {
        len = SIZEOF(message) - 1;
    }
    ncm_error_set(error, EINVAL, message, len);
    return;
}

static void
settings_set_duplicate_option_error(NcmError *error, SettingsOption *option) {
    char message[256];
    int32 len;

    len = SNPRINTF(message,
                   "error while processing option \"%.*s\": "
                   "option already set",
                   option->name_len, option->name);
    ncm_error_set(error, EINVAL, message, len);
    return;
}

static bool
settings_report_or_ignore(NcmError *error, bool ignore_errors) {
    if (ncm_error_is_set(error)) {
        fprintf(stderr, "%s\n", error->message);
    }
    if (!ignore_errors) {
        return false;
    }
    ncm_error_clear(error);
    return true;
}

static bool
settings_apply_option(Configuration *config, SettingsOption *option,
                      char *value, int32 value_len, bool default_value,
                      bool ignore_errors, NcmError *error) {
    NcmError cause;

    ncm_error_clear(&cause);
    if (!option->apply(config, value, value_len, &cause)) {
        settings_set_option_error(error, default_value, option, &cause);
        return settings_report_or_ignore(error, ignore_errors);
    }
    return true;
}

static bool
settings_read_file(Configuration *config, SettingsOption *options,
                   int32 option_count, char *path, int32 path_len,
                   bool ignore_errors, bool quiet, NcmError *error) {
    FILE *file;
    StrBuilder path_buffer;
    char line[SETTINGS_LINE_CAP];

    if (!ncm_fs_exists(path, path_len)) {
        return true;
    }

    sb_init(&path_buffer);
    sb_append(&path_buffer, path, path_len);
    if ((file = fopen(path_buffer.data, "r")) == NULL) {
        char message[256];
        int32 len;

        len = SNPRINTF(message, "failed to open configuration file '%.*s': %s",
                       path_len, path, strerror(errno));
        if (len < 0) {
            ncm_error_set(error, errno,
                          STRLIT_ARGS("failed to open configuration file"));
        } else {
            if (len >= SIZEOF(message)) {
                len = SIZEOF(message) - 1;
            }
            ncm_error_set(error, errno, message, len);
        }
        sb_free(&path_buffer);
        return settings_report_or_ignore(error, ignore_errors);
    }

    if (!quiet) {
        fprintf(stderr, "Reading configuration from %s...\n", path_buffer.data);
    }
    while (fgets(line, SIZEOF(line), file)) {
        int32 line_len;
        NcmOptionLine parsed;
        SettingsOption *option;

        line_len = strlen32(line);
        while (
            (line_len > 0)
            && ((line[line_len - 1] == '\n') || (line[line_len - 1] == '\r'))) {
            line_len -= 1;
            line[line_len] = '\0';
        }
        if (!ncm_option_parser_parse_line(line, line_len, &parsed)) {
            continue;
        }
        option = settings_find_option(options, option_count, parsed.option,
                                      parsed.option_len);
        if (option == NULL) {
            settings_set_unknown_option_error(error, parsed.option,
                                              parsed.option_len);
            if (!settings_report_or_ignore(error, ignore_errors)) {
                fclose(file);
                sb_free(&path_buffer);
                return false;
            }
            continue;
        }
        if (option->used) {
            settings_set_duplicate_option_error(error, option);
            if (!settings_report_or_ignore(error, ignore_errors)) {
                fclose(file);
                sb_free(&path_buffer);
                return false;
            }
            continue;
        }
        option->used = true;
        if (!settings_apply_option(config, option, parsed.value,
                                   parsed.value_len, false, ignore_errors,
                                   error)) {
            fclose(file);
            sb_free(&path_buffer);
            return false;
        }
    }
    fclose(file);
    sb_free(&path_buffer);
    return true;
}

static bool
settings_initialize_defaults(Configuration *config, SettingsOption *options,
                             int32 option_count, bool ignore_errors,
                             NcmError *error) {
    for (int32 i = 0; i < option_count; i += 1) {
        if (options[i].used) {
            continue;
        }
        if (!settings_apply_option(
                config, &options[i], options[i].default_value,
                options[i].default_value_len, true, ignore_errors, error)) {
            return false;
        }
    }
    return true;
}

bool
configuration_read(Configuration *config, NcmStringViewArray *config_paths,
                   bool ignore_errors, bool quiet, NcmError *error) {
    SettingsOption options[] = {
        SETTINGS_OPTION("ncmpcpp_directory", "~/.config/ncmpcpp/",
                        apply_ncmpcpp_directory),
        SETTINGS_OPTION("lyrics_directory", "~/.lyrics/",
                        apply_lyrics_directory),
        SETTINGS_OPTION("mpd_host", "localhost", apply_mpd_host),
        SETTINGS_OPTION("mpd_port", "6600", apply_mpd_port),
        SETTINGS_OPTION("mpd_password", "", apply_mpd_password),
        SETTINGS_OPTION("mpd_music_dir", "~/music", apply_mpd_music_dir),
        SETTINGS_OPTION("mpd_connection_timeout", "5",
                        apply_mpd_connection_timeout),
        SETTINGS_OPTION("mpd_crossfade_time", "5", apply_mpd_crossfade_time),
        SETTINGS_OPTION("random_exclude_pattern", "",
                        apply_random_exclude_pattern),
        SETTINGS_OPTION("visualizer_data_source", "/tmp/mpd.fifo",
                        apply_visualizer_data_source),
        SETTINGS_OPTION("visualizer_output_name", "Visualizer feed",
                        apply_visualizer_output_name),
        SETTINGS_OPTION("visualizer_in_stereo", "yes",
                        apply_visualizer_in_stereo),
#if defined(HAVE_FFTW3_H)
        SETTINGS_OPTION("visualizer_type", "spectrum", apply_visualizer_type),
#else
        SETTINGS_OPTION("visualizer_type", "ellipse", apply_visualizer_type),
#endif
        SETTINGS_OPTION("visualizer_look", "●▮", apply_visualizer_look),
        SETTINGS_OPTION("visualizer_fps", "60", apply_visualizer_fps),
        SETTINGS_OPTION("visualizer_autoscale", "no",
                        apply_visualizer_autoscale),
        SETTINGS_OPTION("visualizer_spectrum_smooth_look", "yes",
                        apply_visualizer_spectrum_smooth_look),
        SETTINGS_OPTION("visualizer_spectrum_smooth_look_legacy_chars", "yes",
                        apply_visualizer_spectrum_smooth_look_legacy_chars),
        SETTINGS_OPTION("visualizer_spectrum_dft_size", "2",
                        apply_visualizer_spectrum_dft_size),
        SETTINGS_OPTION("visualizer_spectrum_gain", "10",
                        apply_visualizer_spectrum_gain),
        SETTINGS_OPTION("visualizer_spectrum_hz_min", "20",
                        apply_visualizer_spectrum_hz_min),
        SETTINGS_OPTION("visualizer_spectrum_hz_max", "20000",
                        apply_visualizer_spectrum_hz_max),
        SETTINGS_OPTION("visualizer_spectrum_log_scale_x", "yes",
                        apply_visualizer_spectrum_log_scale_x),
        SETTINGS_OPTION("visualizer_spectrum_log_scale_y", "yes",
                        apply_visualizer_spectrum_log_scale_y),
        SETTINGS_OPTION("visualizer_color",
                        "blue, cyan, green, yellow, magenta, red",
                        apply_visualizer_color),
        SETTINGS_OPTION("system_encoding", "", apply_system_encoding),
        SETTINGS_OPTION("playlist_disable_highlight_delay", "5",
                        apply_playlist_disable_highlight_delay),
        SETTINGS_OPTION("message_delay_time", "5", apply_message_delay_time),
        SETTINGS_OPTION("song_list_format", "{%a - }{%t}|{$8%f$9}$R{$3%l$9}",
                        apply_song_list_format),
        SETTINGS_OPTION("song_status_format",
                        "{{%a{ \"%b\"{ (%y)}} - }{%t}}|{%f}",
                        apply_song_status_format),
        SETTINGS_OPTION("song_library_format", "{%n - }{%t}|{%f}",
                        apply_song_library_format),
        SETTINGS_OPTION("alternative_header_first_line_format",
                        "$b$1$aqqu$/a$9 {%t}|{%f} $1$atqq$/a$9$/b",
                        apply_header_first_line_format),
        SETTINGS_OPTION("alternative_header_second_line_format",
                        "{{$4$b%a$/b$9}{ - $7%b$9}{ ($4%y$9)}}|{%D}",
                        apply_header_second_line_format),
        SETTINGS_OPTION("current_item_prefix", "$(yellow)$r",
                        apply_current_item_prefix),
        SETTINGS_OPTION("current_item_suffix", "$/r$(end)",
                        apply_current_item_suffix),
        SETTINGS_OPTION("current_item_inactive_column_prefix", "$(white)$r",
                        apply_current_item_inactive_column_prefix),
        SETTINGS_OPTION("current_item_inactive_column_suffix", "$/r$(end)",
                        apply_current_item_inactive_column_suffix),
        SETTINGS_OPTION("now_playing_prefix", "$b", apply_now_playing_prefix),
        SETTINGS_OPTION("now_playing_suffix", "$/b", apply_now_playing_suffix),
        SETTINGS_OPTION("browser_playlist_prefix", "$2playlist$9 ",
                        apply_browser_playlist_prefix),
        SETTINGS_OPTION("selected_item_prefix", "$6",
                        apply_selected_item_prefix),
        SETTINGS_OPTION("selected_item_suffix", "$9",
                        apply_selected_item_suffix),
        SETTINGS_OPTION("modified_item_prefix", "$3>$9 ",
                        apply_modified_item_prefix),
        SETTINGS_OPTION("song_window_title_format", "{%a - }{%t}|{%f}",
                        apply_song_window_title_format),
        SETTINGS_OPTION("browser_sort_mode", "type", apply_browser_sort_mode),
        SETTINGS_OPTION("browser_sort_format", "{%a - }{%t}|{%f} {%l}",
                        apply_browser_sort_format),
        SETTINGS_OPTION("song_columns_list_format",
                        "(20)[]{a} (6f)[green]{NE} "
                        "(50)[white]{t|f:Title} (20)[cyan]{b} "
                        "(7f)[magenta]{l}",
                        apply_song_columns_list_format),
        SETTINGS_OPTION("execute_on_song_change", "",
                        apply_execute_on_song_change),
        SETTINGS_OPTION("execute_on_player_state_change", "",
                        apply_execute_on_player_state_change),
        SETTINGS_OPTION("playlist_show_mpd_host", "no",
                        apply_playlist_show_mpd_host),
        SETTINGS_OPTION("playlist_show_remaining_time", "no",
                        apply_playlist_show_remaining_time),
        SETTINGS_OPTION("playlist_shorten_total_times", "no",
                        apply_playlist_shorten_total_times),
        SETTINGS_OPTION("playlist_separate_albums", "no",
                        apply_playlist_separate_albums),
        SETTINGS_OPTION("playlist_display_mode", "columns",
                        apply_playlist_display_mode),
        SETTINGS_OPTION("browser_display_mode", "classic",
                        apply_browser_display_mode),
        SETTINGS_OPTION("search_engine_display_mode", "classic",
                        apply_search_engine_display_mode),
        SETTINGS_OPTION("playlist_editor_display_mode", "classic",
                        apply_playlist_editor_display_mode),
        SETTINGS_OPTION("discard_colors_if_item_is_selected", "yes",
                        apply_discard_colors_if_item_is_selected),
        SETTINGS_OPTION("show_duplicate_tags", "yes",
                        apply_show_duplicate_tags),
        SETTINGS_OPTION("incremental_seeking", "yes",
                        apply_incremental_seeking),
        SETTINGS_OPTION("seek_time", "1", apply_seek_time),
        SETTINGS_OPTION("volume_change_step", "2", apply_volume_change_step),
        SETTINGS_OPTION("autocenter_mode", "no", apply_autocenter_mode),
        SETTINGS_OPTION("centered_cursor", "no", apply_centered_cursor),
        SETTINGS_OPTION("progressbar_look", "=>", apply_progressbar_look),
        SETTINGS_OPTION("default_place_to_search_in", "database",
                        apply_default_place_to_search_in),
        SETTINGS_OPTION("user_interface", "classic", apply_user_interface),
        SETTINGS_OPTION("data_fetching_delay", "yes",
                        apply_data_fetching_delay),
        SETTINGS_OPTION("media_library_hide_album_dates", "no",
                        apply_media_library_hide_album_dates),
        SETTINGS_OPTION("media_library_primary_tag", "artist",
                        apply_media_library_primary_tag),
        SETTINGS_OPTION("media_library_albums_split_by_date", "yes",
                        apply_media_library_albums_split_by_date),
        SETTINGS_OPTION("default_find_mode", "wrapped",
                        apply_default_find_mode),
        SETTINGS_OPTION("default_tag_editor_pattern", "%n - %t",
                        apply_default_tag_editor_pattern),
        SETTINGS_OPTION("header_visibility", "yes", apply_header_visibility),
        SETTINGS_OPTION("statusbar_visibility", "yes",
                        apply_statusbar_visibility),
        SETTINGS_OPTION("connected_message_on_startup", "yes",
                        apply_connected_message_on_startup),
        SETTINGS_OPTION("titles_visibility", "yes", apply_titles_visibility),
        SETTINGS_OPTION("header_text_scrolling", "yes",
                        apply_header_text_scrolling),
        SETTINGS_OPTION("cyclic_scrolling", "no", apply_cyclic_scrolling),
        SETTINGS_OPTION("lyrics_fetchers",
                        "azlyrics, genius, letras, musixmatch, tekstowo, "
                        "vagalume, internet",
                        apply_lyrics_fetchers),
        SETTINGS_OPTION("follow_now_playing_lyrics", "no",
                        apply_follow_now_playing_lyrics),
        SETTINGS_OPTION("fetch_lyrics_for_current_song_in_background", "no",
                        apply_fetch_lyrics_background),
        SETTINGS_OPTION("store_lyrics_in_song_dir", "no",
                        apply_store_lyrics_in_song_dir),
        SETTINGS_OPTION("generate_win32_compatible_filenames", "yes",
                        apply_generate_win32_compatible_filenames),
        SETTINGS_OPTION("allow_for_physical_item_deletion", "no",
                        apply_allow_for_physical_item_deletion),
        SETTINGS_OPTION("lastfm_preferred_language", "en",
                        apply_lastfm_preferred_language),
        SETTINGS_OPTION("space_add_mode", "add_remove", apply_space_add_mode),
        SETTINGS_OPTION("show_hidden_files_in_local_browser", "no",
                        apply_show_hidden_files_in_local_browser),
        SETTINGS_OPTION("screen_switcher_mode", "playlist, browser",
                        apply_screen_switcher_mode),
        SETTINGS_OPTION("startup_screen", "playlist", apply_startup_screen),
        SETTINGS_OPTION("startup_slave_screen", "", apply_startup_slave_screen),
        SETTINGS_OPTION("startup_slave_screen_focus", "no",
                        apply_startup_slave_screen_focus),
        SETTINGS_OPTION("locked_screen_width_part", "50",
                        apply_locked_screen_width_part),
        SETTINGS_OPTION("ask_for_locked_screen_width_part", "yes",
                        apply_ask_for_locked_screen_width_part),
        SETTINGS_OPTION("media_library_column_width_ratio_two", "1:1",
                        apply_media_library_column_width_ratio_two),
        SETTINGS_OPTION("media_library_column_width_ratio_three", "1:1:1",
                        apply_media_library_column_width_ratio_three),
        SETTINGS_OPTION("playlist_editor_column_width_ratio", "1:2",
                        apply_playlist_editor_column_width_ratio),
        SETTINGS_OPTION("jump_to_now_playing_song_at_start", "yes",
                        apply_jump_to_now_playing_song_at_start),
        SETTINGS_OPTION("ask_before_clearing_playlists", "yes",
                        apply_ask_before_clearing_playlists),
        SETTINGS_OPTION("ask_before_shuffling_playlists", "yes",
                        apply_ask_before_shuffling_playlists),
        SETTINGS_OPTION("display_volume_level", "yes",
                        apply_display_volume_level),
        SETTINGS_OPTION("display_bitrate", "no", apply_display_bitrate),
        SETTINGS_OPTION("display_remaining_time", "no",
                        apply_display_remaining_time),
        SETTINGS_OPTION("regular_expressions", "extended",
                        apply_regular_expressions),
        SETTINGS_OPTION("ignore_leading_the", "no", apply_ignore_leading_the),
        SETTINGS_OPTION("block_search_constraints_change_if_items_found", "yes",
                        apply_block_search_constraints_change),
        SETTINGS_OPTION("mouse_support", "yes", apply_mouse_support),
        SETTINGS_OPTION("mouse_list_scroll_whole_page", "no",
                        apply_mouse_list_scroll_whole_page),
        SETTINGS_OPTION("lines_scrolled", "5", apply_lines_scrolled),
        SETTINGS_OPTION("empty_tag_marker", "<empty>", apply_empty_tag_marker),
        SETTINGS_OPTION("tags_separator", " | ", apply_tags_separator),
        SETTINGS_OPTION("tag_editor_extended_numeration", "no",
                        apply_tag_editor_extended_numeration),
        SETTINGS_OPTION("media_library_sort_by_mtime", "no",
                        apply_media_library_sort_by_mtime),
        SETTINGS_OPTION("enable_window_title", "yes",
                        apply_enable_window_title),
        SETTINGS_OPTION("search_engine_default_search_mode", "1",
                        apply_search_engine_default_search_mode),
        SETTINGS_OPTION("external_editor", "nano", apply_external_editor),
        SETTINGS_OPTION("use_console_editor", "yes", apply_use_console_editor),
        SETTINGS_OPTION("colors_enabled", "yes", apply_colors_enabled),
        SETTINGS_OPTION("empty_tag_color", "cyan", apply_empty_tag_color),
        SETTINGS_OPTION("header_window_color", "default",
                        apply_header_window_color),
        SETTINGS_OPTION("volume_color", "default", apply_volume_color),
        SETTINGS_OPTION("state_line_color", "default", apply_state_line_color),
        SETTINGS_OPTION("state_flags_color", "default:b",
                        apply_state_flags_color),
        SETTINGS_OPTION("main_window_color", "yellow", apply_main_window_color),
        SETTINGS_OPTION("color1", "white", apply_color1),
        SETTINGS_OPTION("color2", "green", apply_color2),
        SETTINGS_OPTION("progressbar_color", "black:b",
                        apply_progressbar_color),
        SETTINGS_OPTION("progressbar_elapsed_color", "green:b",
                        apply_progressbar_elapsed_color),
        SETTINGS_OPTION("statusbar_color", "default", apply_statusbar_color),
        SETTINGS_OPTION("statusbar_time_color", "default:b",
                        apply_statusbar_time_color),
        SETTINGS_OPTION("player_state_color", "default:b",
                        apply_player_state_color),
        SETTINGS_OPTION("alternative_ui_separator_color", "black:b",
                        apply_alternative_ui_separator_color),
        SETTINGS_OPTION("window_border_color", "green",
                        apply_window_border_color),
        SETTINGS_OPTION("active_window_border", "red",
                        apply_active_window_border),
    };
    int32 option_count;

    configuration_clear(config);
    settings_quiet = quiet;
    option_count = LENGTH(options);
    if (config_paths) {
        for (int32 i = 0; i < config_paths->len; i += 1) {
            NcmStringView path;

            path = config_paths->items[i];
            if (!settings_read_file(config, options, option_count, path.data,
                                    path.len, ignore_errors, quiet, error)) {
                return false;
            }
        }
    }
    return settings_initialize_defaults(config, options, option_count,
                                        ignore_errors, error);
}

#endif /* NCMPCPP_SETTINGS_C */
