#if !defined(NCMPCPP_SETTINGS_H)
#define NCMPCPP_SETTINGS_H

#include <stdbool.h>

#include <mpd/tag.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_array.h"
#include "c/ncm_enums.h"
#include "c/ncm_error.h"
#include "c/ncm_format.h"
#include "c/ncm_regex.h"
#include "c/ncm_song.h"
#include "lyrics_fetcher.h"
#include "curses/nc_buffer.h"
#include "curses/nc_formatted_color.h"
#include "curses/nc_window.h"
#include "screens/screen_type.h"

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
NCM_ARRAY_DECLARE_INIT(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_CLEAR(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_DESTROY(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_RESERVE(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DECLARE_APPEND(ncm_int32_array, NcmInt32Array, int32)

NCM_ARRAY_DECLARE_TYPE(NcmFormattedColorArray, NcFormattedColor)
NCM_ARRAY_DECLARE_INIT(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DECLARE_CLEAR(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DECLARE_APPEND(ncm_formatted_color_array, NcmFormattedColorArray,
                         NcFormattedColor)

typedef struct Configuration {
    char *ncmpcpp_directory;
    char *lyrics_directory;
    char *mpd_music_dir;
    char *visualizer_fifo_path;
    char *visualizer_data_source;
    char *visualizer_output_name;
    char *empty_tag;
    char *external_editor;
    char *system_encoding;
    char *execute_on_song_change;
    char *execute_on_player_state_change;
    char *lastfm_preferred_language;
    char *pattern;
    char *random_exclude_pattern;
    char *tags_separator;

    int32 ncmpcpp_directory_len;
    int32 lyrics_directory_len;
    int32 mpd_music_dir_len;
    int32 visualizer_fifo_path_len;
    int32 visualizer_data_source_len;
    int32 visualizer_output_name_len;
    int32 empty_tag_len;
    int32 external_editor_len;
    int32 system_encoding_len;
    int32 execute_on_song_change_len;
    int32 execute_on_player_state_change_len;
    int32 lastfm_preferred_language_len;
    int32 pattern_len;
    int32 random_exclude_pattern_len;
    int32 tags_separator_len;

    int32 ncmpcpp_directory_cap;
    int32 lyrics_directory_cap;
    int32 mpd_music_dir_cap;
    int32 visualizer_fifo_path_cap;
    int32 visualizer_data_source_cap;
    int32 visualizer_output_name_cap;
    int32 empty_tag_cap;
    int32 external_editor_cap;
    int32 system_encoding_cap;
    int32 execute_on_song_change_cap;
    int32 execute_on_player_state_change_cap;
    int32 lastfm_preferred_language_cap;
    int32 pattern_cap;
    int32 random_exclude_pattern_cap;
    int32 tags_separator_cap;

    NcmBuffer progressbar;
    NcmBuffer visualizer_chars;

    NcmFormatAst song_list_format;
    NcmFormatAst song_window_title_format;
    NcmFormatAst song_library_format;
    NcmFormatAst song_columns_mode_format;
    NcmFormatAst browser_sort_format;
    NcmFormatAst song_status_format;
    NcmFormatAst new_header_first_line;
    NcmFormatAst new_header_second_line;

    NcmInt32Array playlist_editor_column_width_ratio;
    NcmInt32Array media_library_column_width_ratio_two;
    NcmInt32Array media_library_column_width_ratio_three;
    ColumnArray columns;

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

    NcColor header_color;
    NcColor main_color;
    NcColor statusbar_color;

    NcFormattedColor color1;
    NcFormattedColor color2;
    NcFormattedColor empty_tags_color;
    NcFormattedColor volume_color;
    NcFormattedColor state_line_color;
    NcFormattedColor state_flags_color;
    NcFormattedColor progressbar_color;
    NcFormattedColor progressbar_elapsed_color;
    NcFormattedColor player_state_color;
    NcFormattedColor statusbar_time_color;
    NcFormattedColor alternative_ui_separator_color;
    NcmFormattedColorArray visualizer_colors;

    enum VisualizerType visualizer_type;
    NcBorder window_border;
    NcBorder active_window_border;
    enum Design design;
    enum SpaceAddMode space_add_mode;
    enum mpd_tag_type media_lib_primary_tag;
    enum SortMode browser_sort_mode;

    bool colors_enabled;
    bool playlist_show_mpd_host;
    bool playlist_show_remaining_time;
    bool playlist_shorten_total_times;
    bool playlist_separate_albums;
    bool set_window_title;
    bool header_visibility;
    bool header_text_scrolling;
    bool statusbar_visibility;
    bool connected_message_on_startup;
    bool titles_visibility;
    bool centered_cursor;
    bool screen_switcher_previous;
    bool autocenter_mode;
    bool wrapped_search;
    bool incremental_seeking;
    bool now_playing_lyrics;
    bool fetch_lyrics_in_background;
    bool local_browser_show_hidden_files;
    bool search_in_db;
    bool jump_to_now_playing_song_at_start;
    bool display_volume_level;
    bool display_bitrate;
    bool display_remaining_time;
    bool ignore_leading_the;
    bool block_search_constraints_change;
    bool use_console_editor;
    bool use_cyclic_scrolling;
    bool ask_before_clearing_playlists;
    bool ask_before_shuffling_playlists;
    bool mouse_support;
    bool mouse_list_scroll_whole_page;
    bool visualizer_in_stereo;
    bool visualizer_autoscale;
    bool visualizer_spectrum_smooth_look;
    bool visualizer_spectrum_smooth_look_legacy_chars;
    bool visualizer_spectrum_log_scale_x;
    bool visualizer_spectrum_log_scale_y;
    bool data_fetching_delay;
    bool media_library_sort_by_mtime;
    bool media_lib_hide_album_dates;
    bool tag_editor_extended_numeration;
    bool discard_colors_if_item_is_selected;
    bool store_lyrics_in_song_dir;
    bool generate_win32_compatible_filenames;
    bool ask_for_locked_screen_width_part;
    bool allow_for_physical_item_deletion;
    bool show_duplicate_tags;
    bool media_library_albums_split_by_date;
    bool startup_slave_screen_focus;
    bool has_startup_slave_screen_type;

    uint32 mpd_connection_timeout;
    uint32 crossfade_time;
    uint32 seek_time;
    uint32 volume_change_step;
    uint32 message_delay_time;
    uint32 lyrics_db;
    uint32 lines_scrolled;
    uint32 search_engine_default_search_mode;
    uint32 visualizer_fps;
    uint32 visualizer_spectrum_dft_size;
    uint32 playlist_disable_highlight_delay_seconds;
    uint32 regex_type;

    double visualizer_spectrum_gain;
    double visualizer_spectrum_hz_min;
    double visualizer_spectrum_hz_max;
    double locked_screen_width_part;

    int32 selected_item_prefix_length;
    int32 selected_item_suffix_length;
    int32 now_playing_prefix_length;
    int32 now_playing_suffix_length;
    int32 current_item_prefix_length;
    int32 current_item_suffix_length;
    int32 current_item_inactive_column_prefix_length;
    int32 current_item_inactive_column_suffix_length;

    enum ScreenType startup_screen_type;
    enum ScreenType startup_slave_screen_type;
    ScreenTypeArray screen_sequence;

    NcmLyricsFetcherRegistry lyrics_fetchers;
} Configuration;

void column_init(Column *column);
void column_destroy(Column *column);

void column_array_init(ColumnArray *array);
void column_array_destroy(ColumnArray *array);
Column *column_array_append(ColumnArray *array);
void column_array_clear(ColumnArray *array);

void screen_type_array_init(ScreenTypeArray *array);
void screen_type_array_destroy(ScreenTypeArray *array);
enum ScreenType *screen_type_array_append(ScreenTypeArray *array);
void screen_type_array_clear(ScreenTypeArray *array);

void configuration_init(Configuration *config);
void configuration_destroy(Configuration *config);
void configuration_clear(Configuration *config);
bool configuration_read(Configuration *config, NcmStringViewArray *config_paths,
                        bool ignore_errors, bool quiet, NcmError *error);

extern Configuration Config;

#endif /* NCMPCPP_SETTINGS_H */
