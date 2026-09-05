#if !defined(NCMPCPP_SETTINGS_H)
#define NCMPCPP_SETTINGS_H

#include "cbase.h"

#include <mpd/tag.h>

#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "lyrics_fetcher.h"
#include "screens/nc_screens.h"

typedef struct Column {
    char *name;
    char *type;

    int32 name_len;
    int32 name_cap;
    int32 type_len;
    int32 type_cap;

    int32 width;
    int32 stretch_limit;
    NcColor color;

    bool fixed;
    bool right_alignment;
    bool display_empty_tag;
} Column;

typedef struct ColumnArray {
    Column *items;
    int32 len;
    int32 cap;
} ColumnArray;

typedef struct ScreenTypeArray {
    enum ScreenType *items;
    int32 len;
    int32 cap;
} ScreenTypeArray;

NCM_ARRAY_DECLARE_TYPE(NcmInt32Array, int32)
NCM_ARRAY_DECLARE_CLEAR(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_DESTROY(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_APPEND(ncm_int32_array, NcmInt32Array, int32)

NCM_ARRAY_DECLARE_TYPE(NcmFormattedColorArray, NcFormattedColor)
NCM_ARRAY_DECLARE_CLEAR(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DECLARE_APPEND(ncm_formatted_color_array, NcmFormattedColorArray,
                         NcFormattedColor)

typedef struct Configuration {
#define XX_BOOL(NAME, DEFAULT_VALUE) bool NAME;
#define XX_STRING(NAME, DEFAULT_VALUE) \
    char *NAME; \
    int32 NAME##_len;
#define XX_PATH(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_DIR(NAME, DEFAULT_VALUE) XX_STRING(NAME, DEFAULT_VALUE)
#define XX_INT_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) int32 NAME;
#define XX_DOUBLE_RANGE(NAME, DEFAULT_VALUE, MINIMUM, MAXIMUM) \
    double NAME;
#include "configuration_options.def"
#undef XX_DOUBLE_RANGE
#undef XX_INT_RANGE
#undef XX_DIR
#undef XX_PATH
#undef XX_STRING
#undef XX_BOOL

    char *visualizer_fifo_path;

    int32 visualizer_fifo_path_len;

    StrBuilder progressbar_look;
    StrBuilder visualizer_look;

    NcmFormatAst song_list_format;
    NcmFormatAst song_window_title_format;
    NcmFormatAst song_library_format;
    NcmFormatAst song_columns_mode_format;
    NcmFormatAst browser_sort_format;
    NcmFormatAst song_status_format;
    NcmFormatAst alternative_header_first_line_format;
    NcmFormatAst alternative_header_second_line_format;

    NcmInt32Array playlist_editor_column_width_ratio;
    NcmInt32Array media_library_column_width_ratio_two;
    NcmInt32Array media_library_column_width_ratio_three;
    ColumnArray song_columns_list_format;

    enum DisplayMode playlist_display_mode;
    enum DisplayMode browser_display_mode;
    enum DisplayMode search_engine_display_mode;
    enum DisplayMode playlist_editor_display_mode;

    NcBuffer browser_playlist_prefix;
    NcBuffer selected_item_prefix;
    NcBuffer selected_item_suffix;
    NcBuffer now_playing_prefix;
    NcBuffer now_playing_suffix;
    NcBuffer modified_item_prefix;
    NcBuffer current_item_prefix;
    NcBuffer current_item_suffix;
    NcBuffer current_item_inactive_column_prefix;
    NcBuffer current_item_inactive_column_suffix;

    NcColor header_window_color;
    NcColor main_window_color;
    NcColor statusbar_color;

    NcFormattedColor color1;
    NcFormattedColor color2;
    NcFormattedColor empty_tag_color;
    NcFormattedColor volume_color;
    NcFormattedColor state_line_color;
    NcFormattedColor state_flags_color;
    NcFormattedColor progressbar_color;
    NcFormattedColor progressbar_elapsed_color;
    NcFormattedColor player_state_color;
    NcFormattedColor statusbar_time_color;
    NcFormattedColor alternative_ui_separator_color;
    NcmFormattedColorArray visualizer_color;

    enum VisualizerType visualizer_type;
    NcBorder window_border_color;
    NcBorder active_window_border;
    enum Design user_interface;
    enum SpaceAddMode space_add_mode;
    enum mpd_tag_type media_library_primary_tag;
    enum SortMode browser_sort_mode;

    bool screen_switcher_previous;
    bool default_find_mode;
    bool default_place_to_search_in;
    bool has_startup_slave_screen_type;

    int32 lyrics_db;
    uint32 regular_expressions;

    int32 selected_item_prefix_length;
    int32 selected_item_suffix_length;
    int32 now_playing_prefix_length;
    int32 now_playing_suffix_length;
    int32 current_item_prefix_length;
    int32 current_item_suffix_length;
    int32 current_item_inactive_column_prefix_length;
    int32 current_item_inactive_column_suffix_length;

    enum ScreenType startup_screen;
    enum ScreenType startup_slave_screen;
    ScreenTypeArray screen_switcher_mode;

    NcmLyricsFetcherRegistry lyrics_fetchers;
} Configuration;

void column_init(Column *column);

Column *column_array_append(ColumnArray *array);
void column_array_clear(ColumnArray *array);

enum ScreenType *screen_type_array_append(ScreenTypeArray *array);
void screen_type_array_clear(ScreenTypeArray *array);

void configuration_init(Configuration *config);
void configuration_destroy(Configuration *config);
void configuration_clear(Configuration *config);
int32 configuration_read(Configuration *config,
                         NcmStringViewArray *config_paths,
                         bool ignore_errors, bool quiet,
                         NcmError *ncm_error);

extern Configuration Config;

#endif /* NCMPCPP_SETTINGS_H */
