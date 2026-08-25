#if !defined(NCMPCPP_APP_SCREENS_C)
#define NCMPCPP_APP_SCREENS_C

#include "cbase.h"

#include "actions.h"
#include "app_controller.h"
#include "bindings.h"
#include "c/ncm_c.h"
#include "global.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

struct HelpScreen {
    NcHelpScreen screen;
    bool initialized;
};

struct OutputsScreen {
    NcOutputsScreen screen;
    bool initialized;
};

struct ServerInfoScreen {
    NcServerInfoScreen screen;
    NcmMpdStringList url_handlers;
    NcmMpdStringList tag_types;
    NcmTimePoint timer;
    bool initialized;
};

struct SongInfoScreen {
    NcSongInfoScreen screen;
    NcmSong song;
    bool has_song;
    bool initialized;
};

NcmSongInfoMetadata ncm_song_info_tags[] = {
    {
        .name = "Title",
        .get = NCM_SONG_GETTER_TITLE,
        .field = NCM_TAGS_FIELD_TITLE,
    },
    {
        .name = "Artist",
        .get = NCM_SONG_GETTER_ARTIST,
        .field = NCM_TAGS_FIELD_ARTIST,
    },
    {
        .name = "Album Artist",
        .get = NCM_SONG_GETTER_ALBUM_ARTIST,
        .field = NCM_TAGS_FIELD_ALBUM_ARTIST,
    },
    {
        .name = "Album",
        .get = NCM_SONG_GETTER_ALBUM,
        .field = NCM_TAGS_FIELD_ALBUM,
    },
    {
        .name = "Date",
        .get = NCM_SONG_GETTER_DATE,
        .field = NCM_TAGS_FIELD_DATE,
    },
    {
        .name = "Track",
        .get = NCM_SONG_GETTER_TRACK,
        .field = NCM_TAGS_FIELD_TRACK,
    },
    {
        .name = "Genre",
        .get = NCM_SONG_GETTER_GENRE,
        .field = NCM_TAGS_FIELD_GENRE,
    },
    {
        .name = "Composer",
        .get = NCM_SONG_GETTER_COMPOSER,
        .field = NCM_TAGS_FIELD_COMPOSER,
    },
    {
        .name = "Performer",
        .get = NCM_SONG_GETTER_PERFORMER,
        .field = NCM_TAGS_FIELD_PERFORMER,
    },
    {
        .name = "Disc",
        .get = NCM_SONG_GETTER_DISC,
        .field = NCM_TAGS_FIELD_DISC,
    },
    {
        .name = "Comment",
        .get = NCM_SONG_GETTER_COMMENT,
        .field = NCM_TAGS_FIELD_COMMENT,
    },
    {
        .name = NULL,
        .get = NCM_SONG_GETTER_NONE,
        .field = NCM_TAGS_FIELD_LAST,
    },
};

#define NCM_APP_SCREEN_DECLARE_STORAGE(type, name) \
    static type name;

NCM_APP_SCREEN_DIRECT_STORAGE_TYPES(NCM_APP_SCREEN_DECLARE_STORAGE)
NCM_APP_SCREEN_WRAPPED_STORAGE_TYPES(NCM_APP_SCREEN_DECLARE_STORAGE)

#undef NCM_APP_SCREEN_DECLARE_STORAGE

#define NCM_APP_SCREEN_DECLARE_INIT_FLAG(name) \
    static bool name;

NCM_APP_SCREEN_INIT_FLAGS(NCM_APP_SCREEN_DECLARE_INIT_FLAG)

#undef NCM_APP_SCREEN_DECLARE_INIT_FLAG

#define ENUM_NAME PromptResult
#define ENUM_PREFIX_ PROMPT_RESULT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(PROMPT_RESULT_ERROR) \
    X(PROMPT_RESULT_ABORTED) \
    X(PROMPT_RESULT_ACCEPTED)
#include "cbase/xenums.c"

static void app_request_registered_resize(int32 type);
static void app_screen_register_once(NcScreen *screen);
static void app_screen_register_replacing(NcScreen *screen, int32 type);
static bool app_screen_is_current(NcScreen *screen);
static void app_screen_switch_to(NcScreen *screen);
static void app_screen_toggle_or_switch_to(NcScreen *screen);
static NcBorder no_border(void);
static bool app_register_screen(NcScreen *screen);
static TagEditorHooks tag_editor_hooks(void);
static NcHelpHooks help_hooks(void);
static NcOutputsHooks outputs_hooks(void);
static NcServerInfoHooks server_info_hooks(void);
static NcSongInfoHooks song_info_hooks(void);
static void show_long_time(NcBuffer *buffer, int32 seconds);

#define NCM_APP_SCREEN_DEFINE_DIRECT_ACCESSOR( \
    suffix, type, storage, base_expr \
) \
type * \
app_screen_##suffix(void) { \
    app_screen_##suffix##_init(); \
    return &storage; \
} \
 \
NcScreen * \
app_screen_##suffix##_base(void) { \
    app_screen_##suffix##_init(); \
    return base_expr; \
}

NCM_APP_SCREEN_DIRECT_ACCESSOR_TYPES(NCM_APP_SCREEN_DEFINE_DIRECT_ACCESSOR)

#undef NCM_APP_SCREEN_DEFINE_DIRECT_ACCESSOR

#define NCM_APP_SCREEN_DEFINE_WRAPPED_ACCESSOR(suffix, base_expr) \
    NcScreen * \
    app_screen_##suffix##_base(void) { \
        app_screen_##suffix##_init(); \
        return base_expr; \
    }

NCM_APP_SCREEN_WRAPPED_ACCESSOR_TYPES(NCM_APP_SCREEN_DEFINE_WRAPPED_ACCESSOR)

#undef NCM_APP_SCREEN_DEFINE_WRAPPED_ACCESSOR

#define NCM_APP_SCREEN_DEFINE_TYPED_WRAPPED_ACCESSOR( \
    suffix, function, type, expr \
) \
    type * \
    function(void) { \
        app_screen_##suffix##_init(); \
        return expr; \
    }

NCM_APP_SCREEN_TYPED_WRAPPED_ACCESSOR_TYPES(
    NCM_APP_SCREEN_DEFINE_TYPED_WRAPPED_ACCESSOR)

#undef NCM_APP_SCREEN_DEFINE_TYPED_WRAPPED_ACCESSOR

#define NCM_APP_SCREEN_DEFINE_STANDARD_REGISTER(suffix) \
    void \
    app_screen_##suffix##_register(void) { \
        app_screen_register_once(app_screen_##suffix##_base()); \
        return; \
    }

NCM_APP_SCREEN_STANDARD_REGISTER_TYPES(NCM_APP_SCREEN_DEFINE_STANDARD_REGISTER)

#undef NCM_APP_SCREEN_DEFINE_STANDARD_REGISTER

#define NCM_APP_SCREEN_DEFINE_REPLACE_REGISTER(suffix, type) \
    void \
    app_screen_##suffix##_register(void) { \
        app_screen_register_replacing( \
            app_screen_##suffix##_base(), type); \
        return; \
    }

NCM_APP_SCREEN_REPLACE_REGISTER_TYPES(NCM_APP_SCREEN_DEFINE_REPLACE_REGISTER)

#undef NCM_APP_SCREEN_DEFINE_REPLACE_REGISTER

#define NCM_APP_SCREEN_DEFINE_SIMPLE_SWITCH(suffix) \
    void \
    app_screen_##suffix##_switch_to(void) { \
        app_screen_switch_to(app_screen_##suffix##_base()); \
        return; \
    }

NCM_APP_SCREEN_SIMPLE_SWITCH_TYPES(NCM_APP_SCREEN_DEFINE_SIMPLE_SWITCH)

#undef NCM_APP_SCREEN_DEFINE_SIMPLE_SWITCH

#define NCM_APP_SCREEN_DEFINE_REGISTER_SWITCH(suffix) \
    void \
    app_screen_##suffix##_switch_to(void) { \
        app_screen_##suffix##_register(); \
        app_screen_switch_to(app_screen_##suffix##_base()); \
        return; \
    }

NCM_APP_SCREEN_REGISTER_SWITCH_TYPES(NCM_APP_SCREEN_DEFINE_REGISTER_SWITCH)

#undef NCM_APP_SCREEN_DEFINE_REGISTER_SWITCH

#define NCM_APP_SCREEN_DEFINE_IS_CURRENT(suffix) \
    bool \
    app_screen_##suffix##_is_current(void) { \
        return app_screen_is_current(app_screen_##suffix##_base()); \
    }

NCM_APP_SCREEN_IS_CURRENT_TYPES(NCM_APP_SCREEN_DEFINE_IS_CURRENT)

#undef NCM_APP_SCREEN_DEFINE_IS_CURRENT

void
app_screen_browser_init(void) {
    if (browser_screen_initialized) {
        return;
    }

    browser_screen_init(&browser_screen,
                        0,
                        ui_state_screen_width(),
                        ui_state_main_start_y(),
                        ui_state_main_height(),
                        Config.main_color,
                        no_border());
    browser_screen_set_mouse_config(
        &browser_screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);
    browser_screen_set_display_mode(
        &browser_screen, Config.browser_display_mode);
    browser_screen_initialized = true;
    return;
}

void
app_screen_browser_fetch_supported_extensions(void) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    if (!browser_screen_fetch_supported_extensions(
        app_screen_browser(), &global_mpd, &ncm_error)
        && ncm_error_is_set(&ncm_error)) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    ncm_error.message);
    }
    return;
}

void
app_screen_help_init(void) {
    if (help_screen.initialized) {
        return;
    }

    nc_help_screen_init(&help_screen.screen,
                        help_hooks(),
                        0,
                        ui_state_screen_width(),
                        ui_state_main_start_y(),
                        ui_state_main_height(),
                        Config.main_color,
                        no_border(),
                        Config.lines_scrolled);
    help_screen.initialized = true;
    (void)nc_help_screen_reload(&help_screen.screen);
    return;
}

void
app_screen_lastfm_init(void) {
    if (lastfm_screen_initialized) {
        return;
    }

    lastfm_screen_init(&lastfm_screen,
                       0,
                       ui_state_screen_width(),
                       ui_state_main_start_y(),
                       ui_state_main_height(),
                       Config.main_color,
                       no_border(),
                       Config.lines_scrolled);
    lastfm_screen_initialized = true;
    return;
}

void
app_screen_lastfm_switch_to(void) {
    app_screen_lastfm_register();
    app_screen_toggle_or_switch_to(app_screen_lastfm_base());
    return;
}

void
app_screen_lyrics_init(void) {
    if (lyrics_screen_initialized) {
        return;
    }

    lyrics_screen_init(&lyrics_screen,
                       0,
                       ui_state_screen_width(),
                       ui_state_main_start_y(),
                       ui_state_main_height(),
                       Config.main_color,
                       no_border(),
                       Config.lines_scrolled);
    lyrics_screen_initialized = true;
    return;
}

void
app_screen_lyrics_set_resize(void) {
    nc_screen_request_resize(app_screen_lyrics_base());
    return;
}

void
app_screen_lyrics_switch_to(void) {
    app_screen_lyrics_register();
    app_screen_toggle_or_switch_to(app_screen_lyrics_base());
    return;
}

void
app_screen_visualizer_init(void) {
#if defined(ENABLE_VISUALIZER)
    VisualizerScreenConfig visualizer_config = {0};

    if (visualizer_screen_initialized) {
        return;
    }

    visualizer_config.source_location = Config.visualizer_data_source;
    visualizer_config.source_location_len =
        Config.visualizer_data_source_len;
    if (Config.visualizer_fifo_path_len > 0) {
        visualizer_config.source_location = Config.visualizer_fifo_path;
        visualizer_config.source_location_len =
            Config.visualizer_fifo_path_len;
    }
    visualizer_config.output_name = Config.visualizer_output_name;
    visualizer_config.output_name_len = Config.visualizer_output_name_len;
    visualizer_config.visualizer_chars = Config.visualizer_chars.data;
    visualizer_config.visualizer_chars_len = Config.visualizer_chars.len;
    visualizer_config.visualizer_colors = Config.visualizer_colors.items;
    visualizer_config.visualizer_colors_len = Config.visualizer_colors.len;
    visualizer_config.fps = Config.visualizer_fps;
    visualizer_config.spectrum_dft_size = Config.visualizer_spectrum_dft_size;
    visualizer_config.spectrum_gain = Config.visualizer_spectrum_gain;
    visualizer_config.spectrum_hz_min = Config.visualizer_spectrum_hz_min;
    visualizer_config.spectrum_hz_max = Config.visualizer_spectrum_hz_max;
    visualizer_config.data_source_hooks =
        visualizer_data_source_system_hooks(&global_mpd);
    visualizer_config.visualization_type =
        (enum VisualizerScreenType)Config.visualizer_type;
    visualizer_config.autoscale = Config.visualizer_autoscale;
    visualizer_config.stereo = Config.visualizer_in_stereo;
    visualizer_config.spectrum_smooth_look =
        Config.visualizer_spectrum_smooth_look;
    visualizer_config.spectrum_smooth_look_legacy_chars =
        Config.visualizer_spectrum_smooth_look_legacy_chars;
    visualizer_config.spectrum_log_scale_x =
        Config.visualizer_spectrum_log_scale_x;
    visualizer_config.spectrum_log_scale_y =
        Config.visualizer_spectrum_log_scale_y;

    visualizer_screen_init(&visualizer_screen,
                           0,
                           ui_state_main_start_y(),
                           ui_state_screen_width(),
                           ui_state_main_height(),
                           Config.main_color,
                           no_border(),
                           &visualizer_config);
    visualizer_screen_initialized = true;
#endif
    return;
}

VisualizerScreen *
app_screen_visualizer(void) {
    app_screen_visualizer_init();
    return &visualizer_screen;
}

NcScreen *
app_screen_visualizer_base(void) {
#if defined(ENABLE_VISUALIZER)
    app_screen_visualizer_init();
    return visualizer_screen_base(&visualizer_screen);
#else
    return NULL;
#endif
}

void
app_screen_playlist_init(void) {
    if (playlist_screen_initialized) {
        return;
    }

    playlist_screen_init(&playlist_screen,
                         0,
                         ui_state_screen_width(),
                         ui_state_main_start_y(),
                         ui_state_main_height(),
                         Config.main_color,
                         no_border());
    playlist_screen_set_mouse_config(
        &playlist_screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);
    playlist_screen_initialized = true;
    return;
}

void
app_screen_playlist_editor_init(void) {
    if (playlist_editor_screen_initialized) {
        return;
    }
    playlist_editor_screen_init(&playlist_editor_screen,
                                0,
                                ui_state_screen_width(),
                                ui_state_main_start_y(),
                                ui_state_main_height(),
                                Config.main_color,
                                no_border());
    if ((Config.playlist_editor_column_width_ratio.len >= 2)
        && (Config.playlist_editor_column_width_ratio.items[0] > 0)
        && (Config.playlist_editor_column_width_ratio.items[1] > 0)) {
        playlist_editor_screen_set_column_ratio(
            &playlist_editor_screen,
            Config.playlist_editor_column_width_ratio.items[0],
            Config.playlist_editor_column_width_ratio.items[1]);
    }
    playlist_editor_screen_initialized = true;
    return;
}

void
app_screen_selected_items_adder_init(void) {
    if (selected_items_adder_screen_initialized) {
        return;
    }
    selected_items_adder_screen_init(
        &selected_items_adder_screen, 0, ui_state_main_start_y(),
        ui_state_screen_width(), ui_state_main_height(),
        Config.main_color, Config.window_border);
    selected_items_adder_screen_initialized = true;
    return;
}

bool
app_screen_selected_items_adder_open(NcmSongArray *songs,
                                     NcmError *ncm_error) {
    app_screen_selected_items_adder_register();
    return selected_items_adder_screen_open(
        app_screen_selected_items_adder(), songs,
        app_screen_playlist(), &global_mpd, ncm_error);
}

void
app_screen_sort_playlist_dialog_init(void) {
    if (sort_playlist_dialog_initialized) {
        return;
    }
    sort_playlist_dialog_init(&sort_playlist_dialog, 0,
                              ui_state_main_start_y(),
                              30, ui_state_main_height(),
                              Config.main_color,
                              Config.window_border);
    sort_playlist_dialog_initialized = true;
    return;
}

bool
app_screen_sort_playlist_dialog_switch_to(void) {
    NcmError ncm_error;
    bool success;

    ncm_error_clear(&ncm_error);
    success = sort_playlist_dialog_open(
        app_screen_sort_playlist_dialog(),
        app_screen_playlist(), &global_mpd,
        Config.ignore_leading_the, &ncm_error);
    if (!success && ncm_error_is_set(&ncm_error)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, ncm_error.message);
    }
    return success;
}

static bool
search_list_database_songs(
    void *user, NcmSongArray *songs, NcmError *ncm_error
) {
    NcmMpdSongList source;
    bool result;

    (void)user;
    if (songs == NULL) {
        return false;
    }

    ncm_song_array_clear(songs);
    ncm_mpd_song_list_init(&source);
    result = ncm_mpd_client_get_directory_recursive(
        &global_mpd, "/", &source, ncm_error);
    if (result) {
        if (!(result = ncm_mpd_song_list_to_song_array(&source, songs))) {
            ncm_error_set(ncm_error, EIO,
                          STRLIT("failed to copy database songs"));
        }
    }
    ncm_mpd_song_list_destroy(&source);
    return result;
}

static bool
search_snapshot_playlist(
    void *user, NcmSongArray *songs, NcmError *ncm_error
) {
    PlaylistScreen *playlist;
    NcSongMenu *song_menu;
    NcMenu *menu;
    NcmSong *song;
    int32 count;

    (void)user;
    (void)ncm_error;
    if (songs == NULL) {
        return false;
    }

    ncm_song_array_clear(songs);
    playlist = app_screen_playlist();
    song_menu = playlist_screen_song_menu(playlist);
    menu = nc_song_menu_base(song_menu);
    count = nc_menu_all_item_count(menu);
    for (int32 i = 0; i < count; i += 1) {
        song = nc_song_menu_item_at(song_menu, NC_MENU_ITEMS_ALL, i);
        if (song == NULL) {
            continue;
        }
        if (!ncm_song_array_append_copy(songs, song)) {
            ncm_error_set(ncm_error, EIO,
                          STRLIT("failed to copy playlist songs"));
            return false;
        }
    }
    return true;
}

static bool
search_prompt_hook(char *text, void *user) {
    (void)user;
    return ncm_statusbar_main_hook(text, optional_strlen32(text));
}

static enum SearchEnginePromptResult
search_prompt_constraint(
    void *user, char *label, int32 label_len, StrBuilder *initial,
    StrBuilder *result
) {
    NcmStatusbarScopedLock scoped_lock;
    enum NcPromptStatus status;
    NcPrompt prompt = {0};
    NcWindow *window;
    char *input;
    char *initial_text;
    int32 input_len;
    int32 copied;

    (void)user;
    if ((label == NULL) || (label_len < 0) || (initial == NULL)
        || (result == NULL)) {
        return SEARCH_ENGINE_PROMPT_ERROR;
    }

    input = NULL;
    initial_text = initial->data;
    if (initial_text == NULL) {
        initial_text = "";
    }

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put()) == NULL) {
        ncm_statusbar_scoped_lock_destroy(&scoped_lock);
        return SEARCH_ENGINE_PROMPT_ERROR;
    }
    nc_window_print_data(window, label, label_len);
    nc_window_print_data(window, STRLIT(": "));

    prompt.initial_text = initial_text;
    prompt.width = -1;
    prompt.hook = search_prompt_hook;
    prompt.hook_user_data = NULL;
    prompt.encrypted = false;
    prompt.remember = true;
    status = nc_window_prompt(window, &prompt, &input);
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);

    if ((status != NC_PROMPT_ACCEPTED) || (input == NULL)) {
        nc_window_prompt_result_destroy(input);
        if (status == NC_PROMPT_ABORTED) {
            return SEARCH_ENGINE_PROMPT_ABORTED;
        }
        return SEARCH_ENGINE_PROMPT_ERROR;
    }

    input_len = optional_strlen32(input);
    copied = sb_set(result, input, input_len);
    nc_window_prompt_result_destroy(input);
    if (copied < 0) {
        return SEARCH_ENGINE_PROMPT_ERROR;
    }
    return SEARCH_ENGINE_PROMPT_ACCEPTED;
}

static void
search_status_message(
    void *user, char *message, int32 message_len
) {
    (void)user;
    ncm_statusbar_print(Config.message_delay_time,
                        message, message_len);
    return;
}

static bool
search_add_song(
    void *user, NcmSong *song, bool play, NcmError *ncm_error
) {
    (void)user;
    (void)ncm_error;
    return ncm_action_add_song_to_playlist(song, play, -1);
}

static bool
search_format_song(
    void *user, NcmSong *song, StrBuilder *text
) {
    SearchEngineScreen *screen;

    screen = user;
    if ((screen == NULL) || (song == NULL) || (text == NULL)) {
        return false;
    }
    return search_engine_screen_format_song_text(
        screen, song, text);
}

void
app_screen_search_engine_init(void) {
    SearchEngineHooks hooks = {0};
    enum SearchEngineSearchMode mode;

    if (search_engine_screen_initialized) {
        return;
    }

    search_engine_screen_init(&search_engine_screen,
                              0,
                              ui_state_screen_width(),
                              ui_state_main_start_y(),
                              ui_state_main_height(),
                              Config.main_color,
                              no_border());

    mode = SEARCH_ENGINE_SEARCH_MODE_LITERAL;
    if (Config.search_engine_default_search_mode
        < (int32)SEARCH_ENGINE_SEARCH_MODE_LAST) {
        mode = (enum SearchEngineSearchMode)
            Config.search_engine_default_search_mode;
    }
    (void)search_engine_screen_set_search_mode(
        &search_engine_screen, mode);
    search_engine_screen_set_search_source(
        &search_engine_screen, Config.search_in_db);

    hooks.client = &global_mpd;
    hooks.list_database_songs = search_list_database_songs;
    hooks.snapshot_playlist = search_snapshot_playlist;
    hooks.prompt_constraint = search_prompt_constraint;
    hooks.status_message = search_status_message;
    hooks.add_song = search_add_song;
    hooks.format_song = search_format_song;
    hooks.user = &search_engine_screen;
    search_engine_screen_set_hooks(&search_engine_screen, hooks);
    search_engine_screen_set_mouse_config(
        &search_engine_screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);

    search_engine_screen_initialized = true;
    return;
}

void
app_screen_media_library_init(void) {
    MediaLibraryHooks hooks;

    if (media_library_screen_initialized) {
        return;
    }

    hooks = media_library_mpd_hooks(&global_mpd);
    media_library_screen_init(&media_library_screen, hooks,
                              0,
                              ui_state_screen_width(),
                              ui_state_main_start_y(),
                              ui_state_main_height(),
                              Config.main_color,
                              no_border());
    media_library_screen_initialized = true;
    return;
}

void
app_screen_tag_editor_init(void) {
    TagEditorHooks hooks;

    if (tag_editor_screen_initialized) {
        return;
    }

    tag_editor_screen_init(&tag_editor_screen, 0,
                           ui_state_screen_width(),
                           ui_state_main_start_y(),
                           ui_state_main_height(),
                           Config.main_color,
                           no_border());
    hooks = tag_editor_hooks();
    tag_editor_screen_set_hooks(&tag_editor_screen, hooks);
    tag_editor_screen_initialized = true;
    return;
}

static bool
statusbar_prompt_hook(char *text, void *user) {
    (void)user;
    return ncm_statusbar_main_hook(text, optional_strlen32(text));
}

static enum PromptResult
prompt_buffer(char *label, int32 label_len,
              NcmStringView initial, StrBuilder *result,
              bool bold_label) {
    NcmStatusbarScopedLock scoped_lock;
    enum NcPromptStatus status;
    NcPrompt prompt = {0};
    NcWindow *window;
    char *input;
    char *initial_text;
    int32 input_len;
    int32 copied;

    if ((label == NULL) || (label_len < 0) || (initial.len < 0)
        || (result == NULL)
        || ((initial.data == NULL) && (initial.len > 0))) {
        return PROMPT_RESULT_ERROR;
    }

    input = NULL;
    initial_text = initial.data;
    if (initial_text == NULL) {
        initial_text = "";
    }

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put()) == NULL) {
        ncm_statusbar_scoped_lock_destroy(&scoped_lock);
        return PROMPT_RESULT_ERROR;
    }
    if (bold_label) {
        nc_window_apply_format(window, NC_FORMAT_BOLD);
    }
    nc_window_print_data(window, label, label_len);
    nc_window_print_data(window, STRLIT(": "));
    if (bold_label) {
        nc_window_apply_format(window, NC_FORMAT_NO_BOLD);
    }

    prompt.initial_text = initial_text;
    prompt.width = -1;
    prompt.hook = statusbar_prompt_hook;
    prompt.hook_user_data = NULL;
    prompt.encrypted = false;
    prompt.remember = true;
    status = nc_window_prompt(window, &prompt, &input);
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);

    if ((status != NC_PROMPT_ACCEPTED) || (input == NULL)) {
        nc_window_prompt_result_destroy(input);
        if (status == NC_PROMPT_ABORTED) {
            return PROMPT_RESULT_ABORTED;
        }
        return PROMPT_RESULT_ERROR;
    }

    input_len = optional_strlen32(input);
    copied = sb_set(result, input, input_len);
    nc_window_prompt_result_destroy(input);
    if (copied < 0) {
        return PROMPT_RESULT_ERROR;
    }
    return PROMPT_RESULT_ACCEPTED;
}

static enum TagEditorPromptResult
tag_editor_hook_prompt(
    void *user, char *label, int32 label_len, NcmStringView initial,
    StrBuilder *result
) {
    enum PromptResult prompt_result;

    (void)user;
    prompt_result = prompt_buffer(label, label_len, initial,
                                  result, true);
    if (prompt_result == PROMPT_RESULT_ACCEPTED) {
        return TAG_EDITOR_PROMPT_ACCEPTED;
    }
    if (prompt_result == PROMPT_RESULT_ABORTED) {
        return TAG_EDITOR_PROMPT_ABORTED;
    }
    return TAG_EDITOR_PROMPT_ERROR;
}

static bool
tag_editor_hook_confirm(
    void *user, char *message, int32 message_len
) {
    NcmStatusbarScopedLock scoped_lock;
    NcWindow *window;
    char values[2];
    char answer;
    bool prompted;

    (void)user;
    if ((message == NULL) || (message_len < 0)) {
        return false;
    }

    values[0] = 'y';
    values[1] = 'n';
    answer = 'n';
    prompted = false;

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    if ((window = ncm_statusbar_put())) {
        nc_window_print_data(window, message, message_len);
        nc_window_print_data(window, STRLIT(" [y/n] "));
        prompted = ncm_statusbar_prompt_return_one_of(
            window, values, LENGTH(values), &answer);
    }
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);

    if (!prompted || (answer != 'y')) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, "Action cancelled");
        return false;
    }
    return true;
}

static void
tag_editor_hook_status_message(
    void *user, char *message, int32 message_len
) {
    (void)user;
    ncm_statusbar_print(Config.message_delay_time,
                        message, message_len);
    return;
}

static void
tag_editor_hook_update_directory(
    void *user, char *directory, int32 directory_len
) {
    NcmError ncm_error = {0};

    (void)user;
    (void)directory_len;
    if (!ncm_mpd_client_update_directory(
        &global_mpd, directory, NULL, &ncm_error)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, ncm_error.message);
    }
    return;
}

static TagEditorHooks
tag_editor_hooks(void) {
    TagEditorHooks hooks = {0};

    hooks.prompt = tag_editor_hook_prompt;
    hooks.confirm = tag_editor_hook_confirm;
    hooks.status_message = tag_editor_hook_status_message;
    hooks.update_directory = tag_editor_hook_update_directory;
    return hooks;
}

static enum TinyTagEditorPromptResult
tiny_tag_editor_prompt(
    void *user, char *label, int32 label_len, NcmStringView initial,
    StrBuilder *result
) {
    enum PromptResult prompt_result;

    (void)user;
    prompt_result = prompt_buffer(label, label_len, initial,
                                  result, true);
    if (prompt_result == PROMPT_RESULT_ACCEPTED) {
        return TINY_TAG_EDITOR_PROMPT_ACCEPTED;
    }
    if (prompt_result == PROMPT_RESULT_ABORTED) {
        return TINY_TAG_EDITOR_PROMPT_ABORTED;
    }
    return TINY_TAG_EDITOR_PROMPT_ERROR;
}

static void
tiny_tag_editor_status_message(
    void *user, char *message, int32 message_len
) {
    (void)user;
    ncm_statusbar_print(Config.message_delay_time,
                        message, message_len);
    return;
}

static void
tiny_tag_editor_update_directory(
    void *user, char *directory, int32 directory_len
) {
    NcmError ncm_error = {0};

    (void)user;
    (void)directory_len;
    if (!ncm_mpd_client_update_directory(
        &global_mpd, directory, NULL, &ncm_error)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, ncm_error.message);
    }
    return;
}

static void
tiny_tag_editor_update_playlist_song(
    void *user, NcmMutableSong *song
) {
    (void)user;
    (void)playlist_screen_update_current_mutable_song(
        app_screen_playlist(), song);
    return;
}

static void
tiny_tag_editor_request_browser_update(void *user) {
    (void)user;
    browser_screen_request_update(app_screen_browser());
    return;
}

void
app_screen_tiny_tag_editor_init(void) {
    TinyTagEditorHooks hooks = {0};

    if (tiny_tag_editor_screen_initialized) {
        return;
    }

    tiny_tag_editor_screen_init(&tiny_tag_editor_screen, 0,
                                ui_state_screen_width(),
                                ui_state_main_start_y(),
                                ui_state_main_height(),
                                Config.main_color,
                                no_border());
    hooks.prompt = tiny_tag_editor_prompt;
    hooks.status_message = tiny_tag_editor_status_message;
    hooks.update_directory = tiny_tag_editor_update_directory;
    hooks.update_playlist_song =
        tiny_tag_editor_update_playlist_song;
    hooks.request_browser_update =
        tiny_tag_editor_request_browser_update;
    tiny_tag_editor_screen_set_hooks(
        &tiny_tag_editor_screen, hooks);
    tiny_tag_editor_screen_initialized = true;
    return;
}

void
app_screen_song_info_init(void) {
    if (song_info_screen.initialized) {
        return;
    }

    ncm_song_init(&song_info_screen.song);
    nc_song_info_screen_init(&song_info_screen.screen,
                             song_info_hooks(),
                             0,
                             ui_state_screen_width(),
                             ui_state_main_start_y(),
                             ui_state_main_height(),
                             Config.main_color,
                             no_border(),
                             Config.lines_scrolled);
    song_info_screen.initialized = true;
    return;
}

void
app_screen_server_info_init(void) {
    if (server_info_screen.initialized) {
        return;
    }

    ncm_mpd_string_list_init(&server_info_screen.url_handlers);
    ncm_mpd_string_list_init(&server_info_screen.tag_types);
    nc_server_info_screen_init(&server_info_screen.screen,
                               server_info_hooks(),
                               ui_state_screen_width(),
                               ui_state_screen_height(),
                               ui_state_main_start_y(),
                               ui_state_main_height(),
                               Config.main_color,
                               Config.window_border);
    server_info_screen.initialized = true;
    return;
}

void
app_screen_outputs_init(void) {
#if defined(ENABLE_OUTPUTS)
    NcBuffer prefix;
    NcBuffer suffix;

    if (outputs_screen.initialized) {
        return;
    }

    nc_outputs_screen_init(&outputs_screen.screen,
                           outputs_hooks(),
                           0,
                           ui_state_screen_width(),
                           ui_state_main_start_y(),
                           ui_state_main_height(),
                           Config.main_color,
                           Config.window_border,
                           Config.lines_scrolled,
                           Config.mouse_list_scroll_whole_page);
    nc_buffer_init(&prefix);
    nc_buffer_init(&suffix);
    nc_buffer_copy(&prefix, &Config.current_item_prefix);
    nc_buffer_copy(&suffix, &Config.current_item_suffix);
    nc_outputs_screen_set_highlight_prefix(&outputs_screen.screen, &prefix);
    nc_outputs_screen_set_highlight_suffix(&outputs_screen.screen, &suffix);
    nc_buffer_destroy(&prefix);
    nc_buffer_destroy(&suffix);
    outputs_screen.initialized = true;
#endif
    return;
}

void
app_screen_outputs_toggle(void) {
#if defined(ENABLE_OUTPUTS)
    (void)nc_outputs_screen_toggle_current(&outputs_screen.screen);
#endif
    return;
}

void
app_screen_outputs_fetch_list(void) {
#if defined(ENABLE_OUTPUTS)
    app_screen_outputs_init();
    nc_outputs_screen_fetch_list(&outputs_screen.screen);
#endif
    return;
}

void
app_screen_outputs_refresh_if_visible(void) {
#if defined(ENABLE_OUTPUTS)
    if (nc_screen_switcher_is_visible(app_screen_outputs_base())) {
        nc_screen_refresh_window(app_screen_outputs_base());
    }
#endif
    return;
}

NcScreen *
app_screen_outputs_base(void) {
#if defined(ENABLE_OUTPUTS)
    app_screen_outputs_init();
    return nc_outputs_screen_base(&outputs_screen.screen);
#else
    return NULL;
#endif
}

void
app_screens_init_all(void) {
    #define NCM_APP_SCREEN_INIT_SCREEN(suffix) \
        app_screen_##suffix##_init();

    NCM_APP_SCREEN_INIT_ALL_TYPES(NCM_APP_SCREEN_INIT_SCREEN)

    #undef NCM_APP_SCREEN_INIT_SCREEN
    return;
}

void
app_screens_register_initial(void) {
    #define NCM_APP_SCREEN_REGISTER_SCREEN(suffix) \
        app_screen_##suffix##_register();

    NCM_APP_SCREEN_REGISTER_INITIAL_TYPES(NCM_APP_SCREEN_REGISTER_SCREEN)

    #undef NCM_APP_SCREEN_REGISTER_SCREEN
    return;
}

void
app_screens_request_registered_resize(void) {
    #define NCM_APP_SCREEN_REQUEST_RESIZE(suffix, type) \
        app_request_registered_resize(type);

    NCM_APP_SCREEN_RESIZE_REQUEST_TYPES(NCM_APP_SCREEN_REQUEST_RESIZE)

    #undef NCM_APP_SCREEN_REQUEST_RESIZE
    return;
}

NcScreen *
app_screens_find_type(enum ScreenType screen_type) {
    int32 type;

    type = screen_type_to_nc_type(screen_type);
    if (type == NC_SCREEN_TYPE_UNKNOWN) {
        return NULL;
    }
    return app_controller_find_screen_type(type);
}

bool
app_screens_switch_to_type(enum ScreenType screen_type) {
    NcScreen *screen;

    if ((screen = app_screens_find_type(screen_type)) == NULL) {
        return false;
    }
    return nc_screen_switcher_switch_to(screen,
                                        nc_screen_has_to_be_resized(screen));
}

bool
app_screens_lock_current(void) {
    return app_controller_lock_current_screen();
}

enum ScreenType
app_screens_current_type(void) {
    NcScreen *screen;

    if ((screen = app_controller_current_screen()) == NULL) {
        return NCM_SCREEN_TYPE_LAST;
    }
    return screen_type_from_nc_type(nc_screen_type(screen));
}

static void
app_request_registered_resize(int32 type) {
    NcScreen *screen;

    if ((screen = app_controller_find_screen_type(type))) {
        nc_screen_request_resize(screen);
    }
    return;
}

static void
app_screen_register_once(NcScreen *screen) {
    ASSERT(screen != NULL);
    ASSERT(app_register_screen(screen));
    return;
}

static void
app_screen_register_replacing(NcScreen *screen, int32 type) {
    NcScreen *registered;
    bool success;

    ASSERT(screen != NULL);

    registered = app_controller_find_screen_type(type);
    if (registered && (registered != screen)) {
        success = app_controller_unregister_screen(registered);
        ASSERT(success);
        if (!success) {
            return;
        }
    }
    success = app_register_screen(screen);
    ASSERT(success);
    (void)success;
    return;
}

static bool
app_screen_is_current(NcScreen *screen) {
    ASSERT(screen != NULL);
    return nc_screen_switcher_is_current(screen);
}

static void
app_screen_switch_to(NcScreen *screen) {
    ASSERT(screen != NULL);
    (void)nc_screen_switcher_switch_to(screen,
                                       nc_screen_has_to_be_resized(screen));
    return;
}

static void
app_screen_toggle_or_switch_to(NcScreen *screen) {
    NcScreen *previous;

    ASSERT(screen != NULL);
    if (nc_screen_switcher_is_current(screen)) {
        previous = nc_screen_switcher_previous();
        if (previous && app_controller_is_screen_registered(previous)) {
            app_screen_switch_to(previous);
        }
        return;
    }
    app_screen_switch_to(screen);
    return;
}

static NcBorder
no_border(void) {
    NcBorder border = {0};

    return border;
}

static void
draw_screen_header(NcScreen *screen) {
    char *title;

    title = nc_screen_title(screen);
    ncm_title_draw_header(title, optional_strlen32(title));
    return;
}

static bool
app_register_screen(NcScreen *screen) {
    if (app_controller_is_screen_registered(screen)) {
        return true;
    }
    return app_controller_register_screen(screen);
}

static void
resize_main_area(NcScreen *base, int32 *x, int32 *width) {
    nc_screen_switcher_get_resize_params(base, x, width, true);
    return;
}

static void
append_cstring(NcBuffer *buffer, char *string) {
    nc_buffer_append_cstring(buffer, string);
    return;
}

static void
append_data(NcBuffer *buffer, char *string, int32 len) {
    if (string && (len > 0)) {
        nc_buffer_append_data(buffer, string, len);
    }
    return;
}

static void
append_format(NcBuffer *buffer, enum NcFormat format) {
    nc_buffer_add_format(buffer, nc_buffer_len(buffer), format, 0);
    return;
}

static void
append_formatted_color(NcBuffer *buffer,
                       NcFormattedColor *color) {
    nc_buffer_add_formatted_color(buffer, nc_buffer_len(buffer), color, 0);
    return;
}

static void
append_formatted_color_end(NcBuffer *buffer,
                           NcFormattedColor *color) {
    nc_buffer_add_formatted_color_end(buffer, nc_buffer_len(buffer), color, 0);
    return;
}

static void
append_bold_label(NcBuffer *buffer, char *label) {
    append_format(buffer, NC_FORMAT_BOLD);
    append_cstring(buffer, label);
    append_format(buffer, NC_FORMAT_NO_BOLD);
    return;
}

static void
append_song_tag(NcBuffer *buffer, StrBuilder *tag) {
    if ((tag == NULL) || (tag->len <= 0)) {
        append_formatted_color(buffer, &Config.empty_tags_color);
        append_data(buffer, Config.empty_tag, Config.empty_tag_len);
        append_formatted_color_end(buffer, &Config.empty_tags_color);
        return;
    }
    append_data(buffer, tag->data, tag->len);
    return;
}

static void
append_song_key_value(NcBuffer *buffer, char *key,
                      StrBuilder *value,
                      bool empty_as_missing) {
    append_format(buffer, NC_FORMAT_BOLD);
    append_formatted_color(buffer, &Config.color1);
    append_cstring(buffer, key);
    append_cstring(buffer, ":");
    append_formatted_color_end(buffer, &Config.color1);
    append_format(buffer, NC_FORMAT_NO_BOLD);
    append_cstring(buffer, " ");
    append_formatted_color(buffer, &Config.color2);
    if (empty_as_missing) {
        append_song_tag(buffer, value);
    } else if (value) {
        append_data(buffer, value->data, value->len);
    }
    append_formatted_color_end(buffer, &Config.color2);
    append_cstring(buffer, "\n");
    return;
}

static int32
format_action_key_name(NcKey key, char *buffer, int32 buffer_cap) {
    int32 len;

    len = ncm_bindings_key_name(key, buffer, buffer_cap);
    if (len < 0) {
        return 0;
    }
    return len;
}

static void
append_action_keys(NcBuffer *buffer, enum NcmActionType type) {
    int32 column_start;
    int32 width;

    column_start = nc_buffer_len(buffer);
    width = 0;
    for (int32 i = 0; i < Bindings.keys_len; i += 1) {
        NcmKeyBindings *key_bindings;
        char key_name[64];
        int32 key_len;

        key_bindings = Bindings.keys + i;
        for (int32 j = 0; j < key_bindings->bindings_len; j += 1) {
            NcmBinding *binding;

            binding = key_bindings->bindings + j;
            if (!ncm_binding_is_single_action_type(binding, type)) {
                continue;
            }
            key_len = format_action_key_name(
                key_bindings->key, key_name, SIZEOF(key_name));
            if (key_len <= 0) {
                continue;
            }
            if (width > 0) {
                append_cstring(buffer, " ");
                width += 1;
            }
            append_data(buffer, key_name, key_len);
            width += key_len;
        }
    }
    while ((nc_buffer_len(buffer) - column_start) < 20) {
        nc_buffer_append_char(buffer, ' ');
    }
    return;
}

static void
append_help_line(NcBuffer *buffer, enum NcmActionType type,
                 char *description) {
    append_cstring(buffer, "    ");
    append_action_keys(buffer, type);
    append_cstring(buffer, " : ");
    append_cstring(buffer, description);
    append_cstring(buffer, "\n");
    return;
}

static bool
help_render(void *user, NcBuffer *buffer) {
    (void)user;

    append_format(buffer, NC_FORMAT_BOLD);
    append_cstring(buffer, "\n  Keys - Movement\n\n");
    append_format(buffer, NC_FORMAT_NO_BOLD);
    append_help_line(buffer, NCM_ACTION_SCROLL_UP, "Move cursor up");
    append_help_line(buffer, NCM_ACTION_SCROLL_DOWN,
                            "Move cursor down");
    append_help_line(buffer, NCM_ACTION_PAGE_UP, "Page up");
    append_help_line(buffer, NCM_ACTION_PAGE_DOWN, "Page down");
    append_help_line(buffer, NCM_ACTION_MOVE_HOME, "Home");
    append_help_line(buffer, NCM_ACTION_MOVE_END, "End");
    append_help_line(buffer, NCM_ACTION_NEXT_SCREEN, "Next screen");
    append_help_line(buffer, NCM_ACTION_PREVIOUS_SCREEN,
                            "Previous screen");
    append_help_line(buffer, NCM_ACTION_SHOW_HELP, "Show help");
    append_help_line(buffer, NCM_ACTION_SHOW_PLAYLIST,
                            "Show playlist");
    append_help_line(buffer, NCM_ACTION_SHOW_BROWSER, "Show browser");
    append_help_line(buffer, NCM_ACTION_SHOW_SEARCH_ENGINE,
                            "Show search engine");
    append_help_line(buffer, NCM_ACTION_SHOW_MEDIA_LIBRARY,
                            "Show media library");
    append_help_line(buffer, NCM_ACTION_SHOW_PLAYLIST_EDITOR,
                            "Show playlist editor");
    append_help_line(buffer, NCM_ACTION_SHOW_SERVER_INFO,
                            "Show server info");
#if defined(ENABLE_OUTPUTS)
    append_help_line(buffer, NCM_ACTION_SHOW_OUTPUTS,
                            "Show outputs");
#endif
#if defined(ENABLE_VISUALIZER)
    append_help_line(buffer, NCM_ACTION_SHOW_VISUALIZER,
                            "Show music visualizer");
#endif
#if defined(HAVE_TAGLIB_H)
    append_help_line(buffer, NCM_ACTION_SHOW_TAG_EDITOR,
                            "Show tag editor");
#endif

    append_format(buffer, NC_FORMAT_BOLD);
    append_cstring(buffer, "\n  Keys - Global\n\n");
    append_format(buffer, NC_FORMAT_NO_BOLD);
    append_help_line(buffer, NCM_ACTION_PLAY, "Play");
    append_help_line(buffer, NCM_ACTION_STOP, "Stop");
    append_help_line(buffer, NCM_ACTION_PAUSE, "Pause");
    append_help_line(buffer, NCM_ACTION_NEXT, "Next track");
    append_help_line(buffer, NCM_ACTION_PREVIOUS, "Previous track");
    append_help_line(buffer, NCM_ACTION_VOLUME_DOWN,
                            "Decrease volume");
    append_help_line(buffer, NCM_ACTION_VOLUME_UP,
                            "Increase volume");
    append_help_line(buffer, NCM_ACTION_TOGGLE_REPEAT,
                            "Toggle repeat mode");
    append_help_line(buffer, NCM_ACTION_TOGGLE_RANDOM,
                            "Toggle random mode");
    append_help_line(buffer, NCM_ACTION_TOGGLE_SINGLE,
                            "Toggle single mode");
    append_help_line(buffer, NCM_ACTION_TOGGLE_CONSUME,
                            "Toggle consume mode");
    append_help_line(buffer, NCM_ACTION_UPDATE_DATABASE,
                            "Start music database update");
    append_help_line(buffer, NCM_ACTION_EXECUTE_COMMAND,
                            "Execute command");
    append_help_line(buffer, NCM_ACTION_QUIT, "Quit");
    return true;
}

static void
help_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(app_screen_help_base());
    draw_screen_header(app_screen_help_base());
    return;
}

static void
help_resize(void *user, NcHelpScreen *screen) {
    int32 x;
    int32 width;

    (void)user;
    resize_main_area(nc_help_screen_base(screen), &x, &width);
    nc_help_screen_set_geometry(screen,
                                x,
                                width,
                                ui_state_main_start_y(),
                                ui_state_main_height());
    return;
}

static void
help_destroy(void *user) {
    HelpScreen *owner;

    owner = user;
    owner->initialized = false;
    return;
}

static NcHelpHooks
help_hooks(void) {
    NcHelpHooks hooks = {0};

    hooks.render = help_render;
    hooks.switch_to = help_switch_to;
    hooks.resize_layout = help_resize;
    hooks.destroy = help_destroy;
    hooks.user = &help_screen;
    return hooks;
}

static void
outputs_fetch(void *user, NcOutputsScreen *screen) {
#if defined(ENABLE_OUTPUTS)
    NcmMpdOutputList outputs;
    NcmError ncm_error;

    (void)user;
    ncm_error_clear(&ncm_error);
    ncm_mpd_output_list_init(&outputs);
    if (!ncm_mpd_client_get_outputs(&global_mpd, &outputs, &ncm_error)) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(ncm_error.message);
        ncm_statusbar_format(5,
                             STRLIT("Could not fetch outputs: %1"),
                             &arg,
                             1);
        ncm_mpd_output_list_destroy(&outputs);
        return;
    }

    for (int32 i = 0; i < outputs.count; i += 1) {
        NcmMpdOutput *output;

        output = outputs.items + i;
        nc_outputs_screen_add_output(screen,
                                     output->id,
                                     output->name,
                                     output->name_len,
                                     output->enabled);
    }
    ncm_mpd_output_list_destroy(&outputs);
#else
    (void)user;
    (void)screen;
#endif
    return;
}

static bool
outputs_toggle(void *user, int32 id, bool enabled,
               char *name, int32 name_len) {
#if defined(ENABLE_OUTPUTS)
    NcmError ncm_error;
    bool ok;

    (void)user;
    ncm_error_clear(&ncm_error);
    if (enabled) {
        ok = ncm_mpd_client_disable_output(&global_mpd, id, &ncm_error);
    } else {
        ok = ncm_mpd_client_enable_output(&global_mpd, id, &ncm_error);
    }
    if (!ok) {
        NcmStringFormatArg args[2];

        args[0] = ncm_string_format_arg_string(name, name_len);
        args[1] = ncm_string_format_arg_cstring(ncm_error.message);
        ncm_statusbar_format(5,
                             STRLIT("Could not toggle output %1: %2"),
                             args,
                             2);
        return false;
    }

    if (enabled) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_string(name, name_len);
        ncm_statusbar_format(3,
                             STRLIT("Output %1 disabled"),
                             &arg,
                             1);
    } else {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_string(name, name_len);
        ncm_statusbar_format(3,
                             STRLIT("Output %1 enabled"),
                             &arg,
                             1);
    }
    return true;
#else
    (void)user;
    (void)id;
    (void)enabled;
    (void)name;
    (void)name_len;
    return false;
#endif
}

static void
outputs_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(app_screen_outputs_base());
    draw_screen_header(app_screen_outputs_base());
    return;
}

static void
outputs_resize(void *user, NcOutputsScreen *screen) {
    int32 x;
    int32 width;

    (void)user;
    resize_main_area(nc_outputs_screen_base(screen), &x, &width);
    nc_outputs_screen_set_geometry(screen,
                                   x,
                                   width,
                                   ui_state_main_start_y(),
                                   ui_state_main_height());
    return;
}

static void
outputs_destroy(void *user) {
#if defined(ENABLE_OUTPUTS)
    OutputsScreen *owner;

    owner = user;
    owner->initialized = false;
#else
    (void)user;
#endif
    return;
}

static NcOutputsHooks
outputs_hooks(void) {
    NcOutputsHooks hooks = {0};

    hooks.fetch_outputs = outputs_fetch;
    hooks.toggle_output = outputs_toggle;
    hooks.switch_to = outputs_switch_to;
    hooks.resize_layout = outputs_resize;
    hooks.destroy = outputs_destroy;
    hooks.user = &outputs_screen;

    return hooks;
}

static void
server_info_load_lists(void *user) {
    ServerInfoScreen *owner;
    NcmError ncm_error;

    owner = user;
    ncm_error_clear(&ncm_error);
    (void)ncm_mpd_client_get_url_handlers(&global_mpd,
                                          &owner->url_handlers,
                                          &ncm_error);
    ncm_error_clear(&ncm_error);
    (void)ncm_mpd_client_get_tag_types(&global_mpd,
                                       &owner->tag_types,
                                       &ncm_error);
    return;
}

static bool
server_info_render(void *user, NcBuffer *buffer) {
    ServerInfoScreen *owner;
    NcmMpdStats stats;
    NcmError ncm_error;
    char time_buffer[64];

    owner = user;
    if (global_timer_elapsed_ms(owner->timer) < 1000) {
        return false;
    }
    owner->timer = global_timer;

    ncm_error_clear(&ncm_error);
    if (!ncm_mpd_client_get_stats(&global_mpd, &stats, &ncm_error)) {
        return false;
    }

    append_bold_label(buffer, "Version: ");
    append_cstring(buffer, "0.");
    nc_buffer_append_int32(buffer, ncm_mpd_client_version(&global_mpd));
    append_cstring(buffer, ".*\n");

    append_bold_label(buffer, "Uptime: ");
    show_long_time(buffer, stats.uptime);
    append_cstring(buffer, "\n");

    append_bold_label(buffer, "Time playing: ");
    ncm_song_show_time(stats.play_time, time_buffer, SIZEOF(time_buffer));
    append_cstring(buffer, time_buffer);
    append_cstring(buffer, "\n\n");

    append_bold_label(buffer, "Total playtime: ");
    show_long_time(buffer, stats.db_play_time);
    append_cstring(buffer, "\n");

    append_bold_label(buffer, "Artist names: ");
    nc_buffer_append_int32(buffer, stats.artists);
    append_cstring(buffer, "\n");

    append_bold_label(buffer, "Album names: ");
    nc_buffer_append_int32(buffer, stats.albums);
    append_cstring(buffer, "\n");

    append_bold_label(buffer, "Songs in database: ");
    nc_buffer_append_int32(buffer, stats.songs);
    append_cstring(buffer, "\n\n");

    append_bold_label(buffer, "URL Handlers:");
    for (int32 i = 0; i < owner->url_handlers.count; i += 1) {
        NcmMpdString *handler;

        handler = owner->url_handlers.items + i;
        if (i == 0) {
            append_cstring(buffer, " ");
        } else {
            append_cstring(buffer, ", ");
        }
        append_data(buffer, handler->value, handler->value_len);
    }
    append_cstring(buffer, "\n\n");

    append_bold_label(buffer, "Tag Types:");
    for (int32 i = 0; i < owner->tag_types.count; i += 1) {
        NcmMpdString *tag;

        tag = owner->tag_types.items + i;
        if (i == 0) {
            append_cstring(buffer, " ");
        } else {
            append_cstring(buffer, ", ");
        }
        append_data(buffer, tag->value, tag->value_len);
    }
    return true;
}

static void
server_info_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(
        app_screen_server_info_base());
    draw_screen_header(app_screen_server_info_base());
    return;
}

static void
server_info_resize(void *user) {
    (void)user;
    nc_server_info_screen_set_dimensions(&server_info_screen.screen,
                                         ui_state_screen_width(),
                                         ui_state_screen_height(),
                                         ui_state_main_start_y(),
                                         ui_state_main_height());
    return;
}

static char *
server_info_title(void *user) {
    (void)user;
    return "Server info";
}

static void
server_info_destroy(void *user) {
    ServerInfoScreen *owner;

    owner = user;
    ncm_mpd_string_list_destroy(&owner->url_handlers);
    ncm_mpd_string_list_destroy(&owner->tag_types);
    owner->initialized = false;
    return;
}

static NcServerInfoHooks
server_info_hooks(void) {
    NcServerInfoHooks hooks = {0};

    hooks.load_lists = server_info_load_lists;
    hooks.render = server_info_render;
    hooks.switch_to = server_info_switch_to;
    hooks.resize_layout = server_info_resize;
    hooks.title = server_info_title;
    hooks.destroy = server_info_destroy;
    hooks.user = &server_info_screen;
    return hooks;
}

static bool
song_info_render(void *user, NcSongInfoScreen *screen,
                 NcBuffer *buffer) {
    SongInfoScreen *owner;
    StrBuilder value;

    (void)screen;
    owner = user;
    if (!owner->has_song) {
        return false;
    }

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_NAME,
                                   0);
    append_song_key_value(buffer, "Filename", &value, false);
    sb_free(&value);

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_DIRECTORY,
                                   0);
    append_song_key_value(buffer, "Directory", &value, true);
    sb_free(&value);
    append_cstring(buffer, "\n");

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_LENGTH,
                                   0);
    append_song_key_value(buffer, "Length", &value, false);
    sb_free(&value);

    for (int32 i = 0; ncm_song_info_tags[i].name; i += 1) {
        append_format(buffer, NC_FORMAT_BOLD);
        append_cstring(buffer, "\n");
        append_cstring(buffer, ncm_song_info_tags[i].name);
        append_cstring(buffer, ":");
        append_format(buffer, NC_FORMAT_NO_BOLD);
        append_cstring(buffer, " ");
        value = ncm_song_tags_buffer(&owner->song,
                                     ncm_song_info_tags[i].get,
                                     Config.tags_separator,
                                     Config.tags_separator_len,
                                     Config.show_duplicate_tags);
        append_song_tag(buffer, &value);
        sb_free(&value);
    }
    return true;
}

static void
song_info_switch_to(void *user, NcSongInfoScreen *screen) {
    SongInfoScreen *owner;
    NcmError ncm_error;

    owner = user;
    ncm_error_clear(&ncm_error);
    ncm_song_destroy(&owner->song);
    ncm_song_init(&owner->song);
    owner->has_song = ncm_mpd_client_get_current_song(&global_mpd,
                                                      &owner->song,
                                                      &ncm_error);
    if (!owner->has_song) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(ncm_error.message);
        ncm_statusbar_format(5,
                             STRLIT("Could not fetch current song: %1"),
                             &arg,
                             1);
        return;
    }

    (void)nc_screen_switcher_finish_switch(app_screen_song_info_base());
    (void)nc_song_info_screen_prepare_current(screen);
    draw_screen_header(app_screen_song_info_base());
    return;
}

static void
song_info_resize(void *user, NcSongInfoScreen *screen) {
    int32 x;
    int32 width;

    (void)user;
    resize_main_area(nc_song_info_screen_base(screen), &x, &width);
    nc_song_info_screen_set_geometry(screen,
                                     x,
                                     width,
                                     ui_state_main_start_y(),
                                     ui_state_main_height());
    return;
}

static void
song_info_destroy(void *user) {
    SongInfoScreen *owner;

    owner = user;
    ncm_song_destroy(&owner->song);
    owner->has_song = false;
    owner->initialized = false;
    return;
}

static NcSongInfoHooks
song_info_hooks(void) {
    NcSongInfoHooks hooks = {0};

    hooks.render = song_info_render;
    hooks.switch_to = song_info_switch_to;
    hooks.resize_layout = song_info_resize;
    hooks.destroy = song_info_destroy;
    hooks.user = &song_info_screen;
    return hooks;
}

static void
show_long_time(NcBuffer *buffer, int32 seconds) {
    int32 days;
    int32 hours;
    int32 minutes;

    days = seconds / 86400;
    seconds -= days*86400;
    hours = seconds / 3600;
    seconds -= hours*3600;
    minutes = seconds / 60;
    seconds -= minutes*60;

    if (days > 0) {
        nc_buffer_append_int64(buffer, days);
        append_cstring(buffer, "d ");
    }
    if ((days > 0) || (hours > 0)) {
        nc_buffer_append_int64(buffer, hours);
        append_cstring(buffer, "h ");
    }
    if ((days > 0) || (hours > 0) || (minutes > 0)) {
        nc_buffer_append_int64(buffer, minutes);
        append_cstring(buffer, "m ");
    }
    nc_buffer_append_int64(buffer, seconds);
    append_cstring(buffer, "s");
    return;
}

#endif /* NCMPCPP_APP_SCREENS_C */
