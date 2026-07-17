#include "settings.h"

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

static NcmArrayItemCallbacks settings_no_callbacks = {0};

static void settings_formatted_color_array_init_item(void *item);
static void settings_formatted_color_array_destroy_item(void *item);

static NcmArrayItemCallbacks settings_formatted_color_callbacks = {
    .init = settings_formatted_color_array_init_item,
    .destroy = settings_formatted_color_array_destroy_item,
};

NCM_ARRAY_DEFINE_INIT(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DEFINE_CLEAR(ncm_int32_array,
                       NcmInt32Array,
                       &settings_no_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DEFINE_RESERVE(ncm_int32_array, NcmInt32Array)
NCM_ARRAY_DEFINE_APPEND(ncm_int32_array,
                        NcmInt32Array,
                        int32,
                        &settings_no_callbacks)

NCM_ARRAY_DEFINE_INIT(ncm_formatted_color_array, NcmFormattedColorArray)
NCM_ARRAY_DEFINE_CLEAR(ncm_formatted_color_array,
                       NcmFormattedColorArray,
                       &settings_formatted_color_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_formatted_color_array,
                         NcmFormattedColorArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_formatted_color_array,
                         NcmFormattedColorArray)
NCM_ARRAY_DEFINE_APPEND(ncm_formatted_color_array,
                        NcmFormattedColorArray,
                        NcFormattedColor,
                        &settings_formatted_color_callbacks)

static void
settings_string_destroy(char **data, int32 *len, int32 *cap) {
    if (*data != NULL) {
        free2(*data, *cap);
    }
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}

void
column_init(Column *column) {
    column->name = NULL;
    column->type = NULL;
    column->name_len = 0;
    column->name_cap = 0;
    column->type_len = 0;
    column->type_cap = 0;
    column->width = 0;
    column->stretch_limit = -1;
    column->color = nc_color_default();
    column->fixed = false;
    column->right_alignment = false;
    column->display_empty_tag = true;
    return;
}

void
column_destroy(Column *column) {
    if (column == NULL) {
        return;
    }
    settings_string_destroy(&column->name, &column->name_len,
                            &column->name_cap);
    settings_string_destroy(&column->type, &column->type_len,
                            &column->type_cap);
    column_init(column);
    return;
}

void
column_array_init(ColumnArray *array) {
    array->items = NULL;
    array->len = 0;
    array->cap = 0;
    return;
}

void
column_array_clear(ColumnArray *array) {
    if (array == NULL) {
        return;
    }
    for (int32 i = 0; i < array->len; i += 1) {
        column_destroy(&array->items[i]);
    }
    array->len = 0;
    return;
}

void
column_array_destroy(ColumnArray *array) {
    if (array == NULL) {
        return;
    }
    column_array_clear(array);
    if (array->items) {
        free2(array->items, array->cap*SIZEOF(*array->items));
    }
    column_array_init(array);
    return;
}

static bool
column_array_reserve(ColumnArray *array, int32 extra) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    if (array == NULL) {
        return false;
    }
    if (extra <= 0) {
        return true;
    }

    needed = array->len + extra;
    if (needed <= array->cap) {
        return true;
    }

    old_cap = array->cap;
    new_cap = array->cap;
    if (new_cap <= 0) {
        new_cap = 8;
    }
    while (new_cap < needed) {
        new_cap *= 2;
    }

    array->items = realloc2(array->items, old_cap, new_cap,
                          SIZEOF(*array->items));
    array->cap = new_cap;
    return true;
}

Column *
column_array_append(ColumnArray *array) {
    Column *column;

    if (!column_array_reserve(array, 1)) {
        return NULL;
    }

    column = &array->items[array->len];
    array->len += 1;
    column_init(column);
    return column;
}

void
screen_type_array_init(ScreenTypeArray *array) {
    array->items = NULL;
    array->len = 0;
    array->cap = 0;
    return;
}

void
screen_type_array_clear(ScreenTypeArray *array) {
    if (array == NULL) {
        return;
    }
    array->len = 0;
    return;
}

void
screen_type_array_destroy(ScreenTypeArray *array) {
    if (array == NULL) {
        return;
    }
    if (array->items) {
        free2(array->items, array->cap*SIZEOF(*array->items));
    }
    screen_type_array_init(array);
    return;
}

static bool
screen_type_array_reserve(ScreenTypeArray *array, int32 extra) {
    int32 needed;
    int32 old_cap;
    int32 new_cap;

    if (array == NULL) {
        return false;
    }
    if (extra <= 0) {
        return true;
    }

    needed = array->len + extra;
    if (needed <= array->cap) {
        return true;
    }

    old_cap = array->cap;
    new_cap = array->cap;
    if (new_cap <= 0) {
        new_cap = 8;
    }
    while (new_cap < needed) {
        new_cap *= 2;
    }

    array->items = realloc2(array->items, old_cap, new_cap,
                          SIZEOF(*array->items));
    array->cap = new_cap;
    return true;
}

enum ScreenType *
screen_type_array_append(ScreenTypeArray *array) {
    enum ScreenType *screen_type;

    if (!screen_type_array_reserve(array, 1)) {
        return NULL;
    }
    screen_type = &array->items[array->len];
    array->len += 1;
    *screen_type = NCM_SCREEN_TYPE_PLAYLIST;
    return screen_type;
}

static void
settings_formatted_color_array_init_item(void *item) {
    nc_formatted_color_init(item);
    return;
}

static void
settings_formatted_color_array_destroy_item(void *item) {
    nc_formatted_color_destroy(item);
    return;
}

static void
configuration_init_strings(Configuration *config) {
#define NCM_CONFIG_STRING_INIT(name) \
    config->name = NULL; \
    config->name##_len = 0; \
    config->name##_cap = 0

    NCM_CONFIG_STRING_INIT(ncmpcpp_directory);
    NCM_CONFIG_STRING_INIT(lyrics_directory);
    NCM_CONFIG_STRING_INIT(mpd_music_dir);
    NCM_CONFIG_STRING_INIT(visualizer_fifo_path);
    NCM_CONFIG_STRING_INIT(visualizer_data_source);
    NCM_CONFIG_STRING_INIT(visualizer_output_name);
    NCM_CONFIG_STRING_INIT(empty_tag);
    NCM_CONFIG_STRING_INIT(external_editor);
    NCM_CONFIG_STRING_INIT(system_encoding);
    NCM_CONFIG_STRING_INIT(execute_on_song_change);
    NCM_CONFIG_STRING_INIT(execute_on_player_state_change);
    NCM_CONFIG_STRING_INIT(lastfm_preferred_language);
    NCM_CONFIG_STRING_INIT(pattern);
    NCM_CONFIG_STRING_INIT(random_exclude_pattern);
    NCM_CONFIG_STRING_INIT(tags_separator);

#undef NCM_CONFIG_STRING_INIT
    return;
}

static void
configuration_destroy_strings(Configuration *config) {
#define NCM_CONFIG_STRING_DESTROY(name) \
    settings_string_destroy(&config->name, &config->name##_len, \
                            &config->name##_cap)

    NCM_CONFIG_STRING_DESTROY(ncmpcpp_directory);
    NCM_CONFIG_STRING_DESTROY(lyrics_directory);
    NCM_CONFIG_STRING_DESTROY(mpd_music_dir);
    NCM_CONFIG_STRING_DESTROY(visualizer_fifo_path);
    NCM_CONFIG_STRING_DESTROY(visualizer_data_source);
    NCM_CONFIG_STRING_DESTROY(visualizer_output_name);
    NCM_CONFIG_STRING_DESTROY(empty_tag);
    NCM_CONFIG_STRING_DESTROY(external_editor);
    NCM_CONFIG_STRING_DESTROY(system_encoding);
    NCM_CONFIG_STRING_DESTROY(execute_on_song_change);
    NCM_CONFIG_STRING_DESTROY(execute_on_player_state_change);
    NCM_CONFIG_STRING_DESTROY(lastfm_preferred_language);
    NCM_CONFIG_STRING_DESTROY(pattern);
    NCM_CONFIG_STRING_DESTROY(random_exclude_pattern);
    NCM_CONFIG_STRING_DESTROY(tags_separator);

#undef NCM_CONFIG_STRING_DESTROY
    return;
}

static void
configuration_init_buffers(Configuration *config) {
    ncm_buffer_init(&config->progressbar);
    ncm_buffer_init(&config->visualizer_chars);
    nc_buffer_init(&config->browser_playlist_prefix);
    nc_buffer_init(&config->selected_item_prefix);
    nc_buffer_init(&config->selected_item_suffix);
    nc_buffer_init(&config->now_playing_prefix);
    nc_buffer_init(&config->now_playing_suffix);
    nc_buffer_init(&config->modified_item_prefix);
    nc_buffer_init(&config->current_item_prefix);
    nc_buffer_init(&config->current_item_suffix);
    nc_buffer_init(&config->current_item_inactive_column_prefix);
    nc_buffer_init(&config->current_item_inactive_column_suffix);
    return;
}

static void
configuration_destroy_buffers(Configuration *config) {
    ncm_buffer_destroy(&config->progressbar);
    ncm_buffer_destroy(&config->visualizer_chars);
    nc_buffer_destroy(&config->browser_playlist_prefix);
    nc_buffer_destroy(&config->selected_item_prefix);
    nc_buffer_destroy(&config->selected_item_suffix);
    nc_buffer_destroy(&config->now_playing_prefix);
    nc_buffer_destroy(&config->now_playing_suffix);
    nc_buffer_destroy(&config->modified_item_prefix);
    nc_buffer_destroy(&config->current_item_prefix);
    nc_buffer_destroy(&config->current_item_suffix);
    nc_buffer_destroy(&config->current_item_inactive_column_prefix);
    nc_buffer_destroy(&config->current_item_inactive_column_suffix);
    return;
}

static void
configuration_init_formats(Configuration *config) {
    ncm_format_ast_init(&config->song_list_format);
    ncm_format_ast_init(&config->song_window_title_format);
    ncm_format_ast_init(&config->song_library_format);
    ncm_format_ast_init(&config->song_columns_mode_format);
    ncm_format_ast_init(&config->browser_sort_format);
    ncm_format_ast_init(&config->song_status_format);
    ncm_format_ast_init(&config->new_header_first_line);
    ncm_format_ast_init(&config->new_header_second_line);
    return;
}

static void
configuration_destroy_formats(Configuration *config) {
    ncm_format_ast_destroy(&config->song_list_format);
    ncm_format_ast_destroy(&config->song_window_title_format);
    ncm_format_ast_destroy(&config->song_library_format);
    ncm_format_ast_destroy(&config->song_columns_mode_format);
    ncm_format_ast_destroy(&config->browser_sort_format);
    ncm_format_ast_destroy(&config->song_status_format);
    ncm_format_ast_destroy(&config->new_header_first_line);
    ncm_format_ast_destroy(&config->new_header_second_line);
    return;
}

static void
configuration_init_colors(Configuration *config) {
    config->header_color = nc_color_default();
    config->main_color = nc_color_default();
    config->statusbar_color = nc_color_default();
    nc_formatted_color_init(&config->color1);
    nc_formatted_color_init(&config->color2);
    nc_formatted_color_init(&config->empty_tags_color);
    nc_formatted_color_init(&config->volume_color);
    nc_formatted_color_init(&config->state_line_color);
    nc_formatted_color_init(&config->state_flags_color);
    nc_formatted_color_init(&config->progressbar_color);
    nc_formatted_color_init(&config->progressbar_elapsed_color);
    nc_formatted_color_init(&config->player_state_color);
    nc_formatted_color_init(&config->statusbar_time_color);
    nc_formatted_color_init(&config->alternative_ui_separator_color);
    config->window_border = nc_border_none();
    config->active_window_border = nc_border_none();
    return;
}

static void
configuration_destroy_colors(Configuration *config) {
    nc_formatted_color_destroy(&config->color1);
    nc_formatted_color_destroy(&config->color2);
    nc_formatted_color_destroy(&config->empty_tags_color);
    nc_formatted_color_destroy(&config->volume_color);
    nc_formatted_color_destroy(&config->state_line_color);
    nc_formatted_color_destroy(&config->state_flags_color);
    nc_formatted_color_destroy(&config->progressbar_color);
    nc_formatted_color_destroy(&config->progressbar_elapsed_color);
    nc_formatted_color_destroy(&config->player_state_color);
    nc_formatted_color_destroy(&config->statusbar_time_color);
    nc_formatted_color_destroy(&config->alternative_ui_separator_color);
    return;
}

static void
configuration_init_arrays(Configuration *config) {
    ncm_int32_array_init(&config->playlist_editor_column_width_ratio);
    ncm_int32_array_init(&config->media_library_column_width_ratio_two);
    ncm_int32_array_init(&config->media_library_column_width_ratio_three);
    column_array_init(&config->columns);
    ncm_formatted_color_array_init(&config->visualizer_colors);
    screen_type_array_init(&config->screen_sequence);
    ncm_lyrics_fetcher_registry_init(&config->lyrics_fetchers);
    return;
}

static void
configuration_destroy_arrays(Configuration *config) {
    ncm_int32_array_destroy(&config->playlist_editor_column_width_ratio);
    ncm_int32_array_destroy(&config->media_library_column_width_ratio_two);
    ncm_int32_array_destroy(&config->media_library_column_width_ratio_three);
    column_array_destroy(&config->columns);
    ncm_formatted_color_array_destroy(&config->visualizer_colors);
    screen_type_array_destroy(&config->screen_sequence);
    ncm_lyrics_fetcher_registry_destroy(&config->lyrics_fetchers);
    return;
}

void
configuration_init(Configuration *config) {
    if (config == NULL) {
        return;
    }

    configuration_init_strings(config);
    configuration_init_buffers(config);
    configuration_init_formats(config);
    configuration_init_colors(config);
    configuration_init_arrays(config);

    config->playlist_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    config->browser_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    config->search_engine_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    config->playlist_editor_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    config->visualizer_type = NCM_VISUALIZER_TYPE_WAVE;
    config->design = NCM_DESIGN_CLASSIC;
    config->space_add_mode = NCM_SPACE_ADD_MODE_ADD_REMOVE;
    config->media_lib_primary_tag = MPD_TAG_ARTIST;
    config->browser_sort_mode = NCM_SORT_MODE_TYPE;

    config->colors_enabled = false;
    config->playlist_show_mpd_host = false;
    config->playlist_show_remaining_time = false;
    config->playlist_shorten_total_times = false;
    config->playlist_separate_albums = false;
    config->set_window_title = false;
    config->header_visibility = false;
    config->header_text_scrolling = false;
    config->statusbar_visibility = false;
    config->connected_message_on_startup = false;
    config->titles_visibility = false;
    config->centered_cursor = false;
    config->screen_switcher_previous = false;
    config->autocenter_mode = false;
    config->wrapped_search = false;
    config->incremental_seeking = false;
    config->now_playing_lyrics = false;
    config->fetch_lyrics_in_background = false;
    config->local_browser_show_hidden_files = false;
    config->search_in_db = false;
    config->jump_to_now_playing_song_at_start = false;
    config->display_volume_level = false;
    config->display_bitrate = false;
    config->display_remaining_time = false;
    config->ignore_leading_the = false;
    config->block_search_constraints_change = false;
    config->use_console_editor = false;
    config->use_cyclic_scrolling = false;
    config->ask_before_clearing_playlists = false;
    config->ask_before_shuffling_playlists = false;
    config->mouse_support = false;
    config->mouse_list_scroll_whole_page = false;
    config->visualizer_in_stereo = false;
    config->visualizer_autoscale = false;
    config->visualizer_spectrum_smooth_look = false;
    config->visualizer_spectrum_smooth_look_legacy_chars = false;
    config->visualizer_spectrum_log_scale_x = false;
    config->visualizer_spectrum_log_scale_y = false;
    config->data_fetching_delay = false;
    config->media_library_sort_by_mtime = false;
    config->media_lib_hide_album_dates = false;
    config->tag_editor_extended_numeration = false;
    config->discard_colors_if_item_is_selected = false;
    config->store_lyrics_in_song_dir = false;
    config->generate_win32_compatible_filenames = false;
    config->ask_for_locked_screen_width_part = false;
    config->allow_for_physical_item_deletion = false;
    config->show_duplicate_tags = false;
    config->media_library_albums_split_by_date = false;
    config->startup_slave_screen_focus = false;
    config->has_startup_slave_screen_type = false;

    config->mpd_connection_timeout = 0;
    config->crossfade_time = 0;
    config->seek_time = 0;
    config->volume_change_step = 0;
    config->message_delay_time = 0;
    config->lyrics_db = 0;
    config->lines_scrolled = 0;
    config->search_engine_default_search_mode = 0;
    config->visualizer_fps = 0;
    config->visualizer_spectrum_dft_size = 0;
    config->playlist_disable_highlight_delay_seconds = 0;
    config->regex_type = NCM_REGEX_EXTENDED_CASE_INSENSITIVE;

    config->visualizer_spectrum_gain = 0;
    config->visualizer_spectrum_hz_min = 0;
    config->visualizer_spectrum_hz_max = 0;
    config->locked_screen_width_part = 0;

    config->selected_item_prefix_length = 0;
    config->selected_item_suffix_length = 0;
    config->now_playing_prefix_length = 0;
    config->now_playing_suffix_length = 0;
    config->current_item_prefix_length = 0;
    config->current_item_suffix_length = 0;
    config->current_item_inactive_column_prefix_length = 0;
    config->current_item_inactive_column_suffix_length = 0;

    config->startup_screen_type = NCM_SCREEN_TYPE_PLAYLIST;
    config->startup_slave_screen_type = NCM_SCREEN_TYPE_UNKNOWN;
    return;
}

void
configuration_destroy(Configuration *config) {
    if (config == NULL) {
        return;
    }

    configuration_destroy_strings(config);
    configuration_destroy_buffers(config);
    configuration_destroy_formats(config);
    configuration_destroy_colors(config);
    configuration_destroy_arrays(config);
    configuration_init(config);
    return;
}

void
configuration_clear(Configuration *config) {
    configuration_destroy(config);
    return;
}

