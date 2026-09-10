#if !defined(APP_SCREENS_C)
#define APP_SCREENS_C

#include "cbase.h"
#include "ncmpcpp2.h"

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
    StringViewList url_handlers;
    StringViewList tag_types;
    int64 timer;
    bool initialized;
};

struct SongInfoScreen {
    NcSongInfoScreen screen;
    NcmSong song;
    bool has_song;
    bool initialized;
};

#define NCM_SONG_INFO_TAG_ENTRY(suffix, display, tag_char, getter_char,    \
                                flags)                                      \
    {                                                                       \
        .name = NCM_TAG_DISPLAY_NAME(display),                              \
        .name_len = NCM_TAG_DISPLAY_NAME_LEN(display),                      \
        .get = CAT(SONG_GETTER_, suffix),                                   \
        .field = CAT(NCM_TAGS_FIELD_, suffix),                              \
    },

NcmSongInfoMetadata ncm_song_info_tags[] = {
    NCM_TAG_SONG_INFO_DEFS(NCM_SONG_INFO_TAG_ENTRY)
    {
        .name = NULL,
        .name_len = 0,
        .get = SONG_GETTER_NONE,
        .field = NCM_TAGS_FIELD_COUNT,
    },
};

#undef NCM_SONG_INFO_TAG_ENTRY
static void
app_request_registered_resize(enum NcScreenType type) {
    NcScreen *screen;

    if ((screen = app_controller_find_screen_type(type))) {
        nc_screen_request_resize(screen);
    }
    return;
}

#define APP_SCREEN_DECLARE_STORAGE(type, name)                             \
    static type name;

APP_SCREEN_DIRECT_STORAGE_TYPES(APP_SCREEN_DECLARE_STORAGE)
APP_SCREEN_WRAPPED_STORAGE_TYPES(APP_SCREEN_DECLARE_STORAGE)

#undef APP_SCREEN_DECLARE_STORAGE

#undef APP_SCREEN_DECLARE_INIT_FLAG

#define ENUM_NAME PromptResult
#define ENUM_PREFIX_ PROMPT_RESULT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS               \
    XX(PROMPT_RESULT_ERROR)       \
    XX(PROMPT_RESULT_ABORTED)     \
    XX(PROMPT_RESULT_ACCEPTED)
#include "cbase/xenums.c"

#define APP_SCREEN_DIRECT_ACCESSOR(suffix, type, storage, base_expr) \
    type *                                                               \
    app_screen_##suffix(void) {                                          \
        app_screen_##suffix##_init();                                    \
        return &storage;                                                 \
    }                                                                    \
                                                                         \
    NcScreen *                                                           \
    app_screen_##suffix##_base(void) {                                   \
        app_screen_##suffix##_init();                                    \
        return base_expr;                                                \
    }

static void
app_register_screen(NcScreen *screen) {
    if (!app_controller_is_screen_registered(screen)) {
        ASSERT(app_controller_register_screen(screen) == 0);
    }
    return;
}

static void
app_screen_register_once(NcScreen *screen) {
    app_register_screen(screen);
    return;
}

static void
app_screen_register_replacing(NcScreen *screen, enum NcScreenType type) {
    NcScreen *registered;

    registered = app_controller_find_screen_type(type);
    if (registered && (registered != screen)) {
        ASSERT(app_controller_unregister_screen(registered) == 0);
    }
    app_register_screen(screen);
    return;
}

static bool
app_screen_is_current(NcScreen *screen) {
    return nc_screen_switcher_is_current(screen);
}

static void
app_screen_switch_to(NcScreen *screen) {
    nc_screen_switcher_switch_to(screen, screen->has_to_be_resized);
    return;
}

static NcBorder
no_border(void) {
    NcBorder border = {0};

    return border;
}

APP_SCREEN_DIRECT_ACCESSOR_TYPES(APP_SCREEN_DIRECT_ACCESSOR)

#undef APP_SCREEN_DIRECT_ACCESSOR

#define APP_SCREEN_DEFINE_WRAPPED_ACCESSOR(suffix, base_expr)           \
    NcScreen *                                                              \
    app_screen_##suffix##_base(void) {                                      \
        app_screen_##suffix##_init();                                       \
        return base_expr;                                                   \
    }

APP_SCREEN_WRAPPED_ACCESSOR_TYPES(APP_SCREEN_DEFINE_WRAPPED_ACCESSOR)

#undef APP_SCREEN_DEFINE_WRAPPED_ACCESSOR

#define APP_SCREEN_TYPED_WRAPPED_ACCESSOR(suffix, function, type, expr) \
    type *                                                                  \
    function(void) {                                                        \
        app_screen_##suffix##_init();                                       \
        return expr;                                                        \
    }

APP_SCREEN_TYPED_WRAPPED_ACCESSOR_TYPES(APP_SCREEN_TYPED_WRAPPED_ACCESSOR)

#undef APP_SCREEN_TYPED_WRAPPED_ACCESSOR

#define APP_SCREEN_DEFINE_STANDARD_REGISTER(suffix)                        \
    void                                                                       \
    app_screen_##suffix##_register(void) {                                     \
        app_screen_register_once(app_screen_##suffix##_base());                \
        return;                                                                \
    }

APP_SCREEN_STANDARD_REGISTER_TYPES(APP_SCREEN_DEFINE_STANDARD_REGISTER)

#undef APP_SCREEN_DEFINE_STANDARD_REGISTER

#define APP_SCREEN_DEFINE_REPLACE_REGISTER(suffix, type)                   \
    void                                                                       \
    app_screen_##suffix##_register(void) {                                     \
        app_screen_register_replacing(app_screen_##suffix##_base(), type);     \
        return;                                                                \
    }

APP_SCREEN_REPLACE_REGISTER_TYPES(APP_SCREEN_DEFINE_REPLACE_REGISTER)

#undef APP_SCREEN_DEFINE_REPLACE_REGISTER

#define APP_SCREEN_DEFINE_SIMPLE_SWITCH(suffix)                            \
    void                                                                       \
    app_screen_##suffix##_switch_to(void) {                                    \
        app_screen_switch_to(app_screen_##suffix##_base());                    \
        return;                                                                \
    }

APP_SCREEN_SIMPLE_SWITCH_TYPES(APP_SCREEN_DEFINE_SIMPLE_SWITCH)

#undef APP_SCREEN_DEFINE_SIMPLE_SWITCH

#define APP_SCREEN_DEFINE_REGISTER_SWITCH(suffix)                          \
    void                                                                       \
    app_screen_##suffix##_switch_to(void) {                                    \
        app_screen_##suffix##_register();                                      \
        app_screen_switch_to(app_screen_##suffix##_base());                    \
        return;                                                                \
    }

APP_SCREEN_REGISTER_SWITCH_TYPES(APP_SCREEN_DEFINE_REGISTER_SWITCH)

#undef APP_SCREEN_DEFINE_REGISTER_SWITCH

#define APP_SCREEN_DEFINE_SIMPLE_SHOW(suffix)                             \
    static int32                                                        \
    app_screen_##suffix##_show(void) {                                        \
        app_screen_##suffix##_register();                                     \
        app_screen_##suffix##_switch_to();                                    \
        return 0;                                                             \
    }

APP_SCREEN_SIMPLE_SWITCH_TYPES(APP_SCREEN_DEFINE_SIMPLE_SHOW)

#undef APP_SCREEN_DEFINE_SIMPLE_SHOW

#define APP_SCREEN_DEFINE_REGISTER_SHOW(suffix)                           \
    static int32                                                        \
    app_screen_##suffix##_show(void) {                                        \
        app_screen_##suffix##_switch_to();                                    \
        return 0;                                                             \
    }

APP_SCREEN_REGISTER_SWITCH_TYPES(APP_SCREEN_DEFINE_REGISTER_SHOW)

#undef APP_SCREEN_DEFINE_REGISTER_SHOW

#define APP_SCREEN_DEFINE_IS_CURRENT(suffix)                               \
    bool                                                                       \
    app_screen_##suffix##_is_current(void) {                                   \
        return app_screen_is_current(app_screen_##suffix##_base());            \
    }

APP_SCREEN_IS_CURRENT_TYPES(APP_SCREEN_DEFINE_IS_CURRENT)

#undef APP_SCREEN_DEFINE_IS_CURRENT

void
app_screen_browser_init(void) {
    static bool browser_screen_initialized = false;
    if (browser_screen_initialized) {
        return;
    }

    browser_screen_init(&browser_screen, 0,
                        ui_state_screen_width(), ui_state_main_start_y(),
                        ui_state_main_height(), Config.main_window_color,
                        no_border());
    browser_screen_set_mouse_config(&browser_screen, Config.lines_scrolled,
                                    Config.mouse_list_scroll_whole_page);
    browser_screen_set_display_mode(&browser_screen,
                                    Config.browser_display_mode);

    browser_screen_initialized = true;
    return;
}

void
app_screen_browser_fetch_supported_extensions(void) {
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    if ((browser_screen_fetch_supported_extensions(app_screen_browser(),
                                                  &global_mpd,
                                                  &ncm_error) < 0)
        && ncm_error_is_set(&ncm_error)) {
        ncm_statusbar_print(Config.message_delay_time,
                            ncm_error.message, ncm_error.message_len);
    }
    return;
}

void
app_screen_lastfm_init(void) {
    static bool lastfm_screen_initialized = false;
    if (lastfm_screen_initialized) {
        return;
    }

    lastfm_screen_init(&lastfm_screen, 0,
                       ui_state_screen_width(), ui_state_main_start_y(),
                       ui_state_main_height(), Config.main_window_color,
                       no_border(), Config.lines_scrolled);
    lastfm_screen_initialized = true;
    return;
}

static void
app_screen_toggle_or_switch_to(NcScreen *screen) {
    NcScreen *previous;

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

void
app_screen_lastfm_switch_to(void) {
    app_screen_lastfm_register();
    app_screen_toggle_or_switch_to(app_screen_lastfm_base());
    return;
}

static int32
app_screen_lastfm_show(void) {
    app_screen_lastfm_switch_to();
    return 0;
}

void
app_screen_lyrics_init(void) {
    static bool lyrics_screen_initialized = false;
    if (lyrics_screen_initialized) {
        return;
    }

    lyrics_screen_init(&lyrics_screen, 0,
                       ui_state_screen_width(), ui_state_main_start_y(),
                       ui_state_main_height(), Config.main_window_color,
                       no_border(), Config.lines_scrolled);
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

static int32
app_screen_lyrics_show(void) {
    app_screen_lyrics_switch_to();
    return 0;
}

void
app_screen_visualizer_init(void) {
#if defined(ENABLE_VISUALIZER)
    static bool visualizer_screen_initialized = false;
    VisualizerScreenConfig conf = {0};

    if (visualizer_screen_initialized) {
        return;
    }

    conf.source_location = Config.visualizer_data_source;
    conf.source_location_len = Config.visualizer_data_source_len;
    conf.output_name = Config.visualizer_output_name;
    conf.output_name_len = Config.visualizer_output_name_len;
    conf.visualizer_chars = Config.visualizer_look.data;
    conf.visualizer_chars_len = Config.visualizer_look.len;
    conf.visualizer_colors = Config.visualizer_color.items;
    conf.visualizer_colors_len = Config.visualizer_color.len;
    conf.fps = Config.visualizer_fps;
    conf.spectrum_dft_size = Config.visualizer_spectrum_dft_size;
    conf.spectrum_gain = Config.visualizer_spectrum_gain;
    conf.spectrum_hz_min = Config.visualizer_spectrum_hz_min;
    conf.spectrum_hz_max = Config.visualizer_spectrum_hz_max;
    conf.data_source_hooks = visualizer_data_source_system_hooks(&global_mpd);
    conf.visualization_type = Config.visualizer_type;
    conf.autoscale = Config.visualizer_autoscale;
    conf.stereo = Config.visualizer_in_stereo;
    conf.spectrum_smooth_look = Config.visualizer_spectrum_smooth_look;
    conf.spectrum_smooth_look_legacy_chars =
        Config.visualizer_spectrum_smooth_look_legacy_chars;
    conf.spectrum_log_scale_x = Config.visualizer_spectrum_log_scale_x;
    conf.spectrum_log_scale_y = Config.visualizer_spectrum_log_scale_y;

    visualizer_screen_init(&visualizer_screen, 0, ui_state_main_start_y(),
                           ui_state_screen_width(), ui_state_main_height(),
                           Config.main_window_color, no_border(),
                           &conf);
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

static int32
app_screen_visualizer_show(void) {
#if defined(ENABLE_VISUALIZER)
    app_screen_visualizer_register();
    return app_screens_switch_to_type(SCREEN_TYPE_VISUALIZER);
#else
    return -NCM_ERROR_UNAVAILABLE;
#endif
}

void
app_screen_playlist_init(void) {
    static bool playlist_screen_initialized;
    if (playlist_screen_initialized) {
        return;
    }

    playlist_screen_init(&playlist_screen, 0, ui_state_screen_width(),
                         ui_state_main_start_y(), ui_state_main_height(),
                         Config.main_window_color, no_border());
    playlist_screen_set_mouse_config(&playlist_screen, Config.lines_scrolled,
                                     Config.mouse_list_scroll_whole_page);
    playlist_screen_initialized = true;
    return;
}

void
app_screen_playlist_edit_init(void) {
    static bool playlist_edit_screen_initialized = false;
    if (playlist_edit_screen_initialized) {
        return;
    }
    playlist_edit_screen_init(&playlist_edit_screen, 0, ui_state_screen_width(),
                              ui_state_main_start_y(), ui_state_main_height(),
                              Config.main_window_color, no_border());
    if ((Config.playlist_edit_column_width_ratio.len >= 2)
        && (Config.playlist_edit_column_width_ratio.items[0] > 0)
        && (Config.playlist_edit_column_width_ratio.items[1] > 0)) {
        int32 first_ratio;
        int32 second_ratio;

        first_ratio = Config.playlist_edit_column_width_ratio.items[0];
        second_ratio = Config.playlist_edit_column_width_ratio.items[1];
        playlist_edit_screen_set_column_ratio(&playlist_edit_screen,
                                              first_ratio, second_ratio);
    }
    playlist_edit_screen_initialized = true;
    return;
}

void
app_screen_selected_items_adder_init(void) {
    static bool selected_items_adder_screen_initialized = false;
    if (selected_items_adder_screen_initialized) {
        return;
    }
    selected_items_adder_screen_init(&selected_items_adder_screen, 0,
                                     ui_state_main_start_y(),
                                     ui_state_screen_width(),
                                     ui_state_main_height(),
                                     Config.main_window_color,
                                     Config.window_border_color);
    selected_items_adder_screen_initialized = true;
    return;
}

int32
app_screen_selected_items_adder_open(NcmSongArray *songs, NcmError *ncm_error) {
    app_screen_selected_items_adder_register();
    return selected_items_adder_screen_open(app_screen_selected_items_adder(),
                                            songs, app_screen_playlist(),
                                            &global_mpd, ncm_error);
}

void
app_screen_sort_playlist_dialog_init(void) {
    static bool sort_playlist_dialog_initialized = false;
    if (sort_playlist_dialog_initialized) {
        return;
    }
    sort_playlist_dialog_init(&sort_playlist_dialog, 0, ui_state_main_start_y(),
                              30, ui_state_main_height(),
                              Config.main_window_color,
                              Config.window_border_color);
    sort_playlist_dialog_initialized = true;
    return;
}

int32
app_screen_sort_playlist_dialog_switch_to(void) {
    NcmError ncm_error;
    int32 status;

    ncm_error_clear(&ncm_error);
    status = sort_playlist_dialog_open(app_screen_sort_playlist_dialog(),
                                       app_screen_playlist(), &global_mpd,
                                       Config.ignore_leading_the,
                                       &ncm_error);
    if ((status < 0) && ncm_error_is_set(&ncm_error)) {
        ncm_statusbar_print(Config.message_delay_time,
                            ncm_error.message, ncm_error.message_len);
    }
    return status;
}

static int32
app_screen_sort_playlist_dialog_show(void) {
    app_screen_sort_playlist_dialog_register();
    return app_screen_sort_playlist_dialog_switch_to();
}

static int32
search_list_database_songs(void *user, NcmSongArray *songs,
                           NcmError *ncm_error) {
    NcmMpdSongList source = {0};
    int32 status;

    (void)user;

    ncm_song_array_clear(songs);
    status = ncm_mpd_client_get_directory_recursive(&global_mpd,
                                                    "/", &source, ncm_error);
    if (status >= 0) {
        ncm_mpd_song_list_to_song_array(&source, songs);
        status = 0;
    }
    ncm_mpd_song_list_destroy(&source);
    return status;
}

static int32
search_snapshot_playlist(void *user, NcmSongArray *songs, NcmError *ncm_error) {
    PlaylistScreen *playlist;
    NcSongMenu *song_menu;
    NcMenu *menu;
    NcmSong *song;
    int32 count;

    (void)user;
    (void)ncm_error;

    ncm_song_array_clear(songs);
    playlist = app_screen_playlist();
    song_menu = playlist_screen_song_menu(playlist);
    menu = nc_song_menu_base(song_menu);
    count = nc_menu_all_item_count(menu);
    for (int32 i = 0; i < count; i += 1) {
        song = nc_song_menu_item_at(song_menu, NC_MENU_ITEMS_ALL, i);
        ncm_song_array_append_copy(songs, song);
    }
    return 0;
}

static bool
search_prompt_should_continue(char *text, void *user) {
    (void)user;
    return ncm_statusbar_prompt_should_continue(text, optional_strlen32(text));
}

static enum SearchEnginePromptResult
search_prompt_constraint(void *user, char *label, int32 label_len,
                         StrBuilder *initial, StrBuilder *result) {
    NcmStatusbarScopedLock scoped_lock;
    enum NcPromptStatus status;
    NcPrompt prompt = {0};
    NcWindow *window;
    char *input;
    char *initial_text;
    int32 input_len;

    (void)user;
    input = NULL;
    initial_text = initial->data;
    if (initial_text == NULL) {
        initial_text = "";
    }

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    window = ncm_statusbar_put();
    nc_window_print_data(window, label, label_len);
    nc_window_print_data(window, STRLIT(": "));

    prompt.initial_text = initial_text;
    prompt.width = -1;
    prompt.should_continue = search_prompt_should_continue;
    prompt.should_continue_user_data = NULL;
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
    sb_set(result, input, input_len);
    nc_window_prompt_result_destroy(input);
    return SEARCH_ENGINE_PROMPT_ACCEPTED;
}

static void
search_status_message(void *user, char *message, int32 message_len) {
    (void)user;
    ncm_statusbar_print(Config.message_delay_time, message, message_len);
    return;
}

static int32
search_add_song(void *user, NcmSong *song, bool play, NcmError *ncm_error) {
    (void)user;
    (void)ncm_error;
    return ncm_action_add_song_to_playlist(song, play, -1);
}

static int32
search_format_song(void *user, NcmSong *song, StrBuilder *text) {
    SearchEngineScreen *screen;

    screen = user;
    search_engine_screen_format_song_text(screen, song, text);
    return 0;
}

void
app_screen_search_engine_init(void) {
    SearchEngineHooks hooks = {0};
    enum SearchEngineSearchMode mode;
    static bool search_engine_screen_initialized = false;

    if (search_engine_screen_initialized) {
        return;
    }

    search_engine_screen_init(&search_engine_screen, 0,
                              ui_state_screen_width(), ui_state_main_start_y(),
                              ui_state_main_height(), Config.main_window_color,
                              no_border());

    mode = config_search_engine_default_mode(&Config);
    search_engine_screen_set_search_mode(&search_engine_screen, mode);
    search_engine_screen_set_search_source(&search_engine_screen,
                                        Config.default_place_to_search_in
                                        == NCM_DEFAULT_SEARCH_SOURCE_DATABASE);

    hooks.client = &global_mpd;
    hooks.list_database_songs = search_list_database_songs;
    hooks.snapshot_playlist = search_snapshot_playlist;
    hooks.prompt_constraint = search_prompt_constraint;
    hooks.status_message = search_status_message;
    hooks.add_song = search_add_song;
    hooks.format_song = search_format_song;
    hooks.user = &search_engine_screen;
    search_engine_screen_set_hooks(&search_engine_screen, hooks);
    search_engine_screen_set_mouse_config(&search_engine_screen,
                                          Config.lines_scrolled,
                                          Config.mouse_list_scroll_whole_page);

    search_engine_screen_initialized = true;
    return;
}

void
app_screen_media_library_init(void) {
    MediaLibraryHooks hooks;
    static bool media_library_screen_initialized = false;

    if (media_library_screen_initialized) {
        return;
    }

    hooks = media_library_mpd_hooks(&global_mpd);
    media_library_screen_init(&media_library_screen, hooks, 0,
                              ui_state_screen_width(), ui_state_main_start_y(),
                              ui_state_main_height(), Config.main_window_color,
                              no_border());
    media_library_screen_initialized = true;
    return;
}

static bool
statusbar_prompt_should_continue(char *text, void *user) {
    (void)user;
    return ncm_statusbar_prompt_should_continue(text, optional_strlen32(text));
}

static enum PromptResult
prompt_buffer(char *label, int32 label_len,
              StringView initial, StrBuilder *result, bool bold_label) {
    NcmStatusbarScopedLock scoped_lock;
    enum NcPromptStatus status;
    NcPrompt prompt = {0};
    NcWindow *window;
    char *input;
    char *initial_text;
    int32 input_len;

    input = NULL;
    initial_text = initial.data;
    if (initial_text == NULL) {
        initial_text = "";
    }

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    window = ncm_statusbar_put();
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
    prompt.should_continue = statusbar_prompt_should_continue;
    prompt.should_continue_user_data = NULL;
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
    sb_set(result, input, input_len);
    nc_window_prompt_result_destroy(input);
    return PROMPT_RESULT_ACCEPTED;
}

static enum TagEditPromptResult
tag_edit_hook_prompt(void *user, char *label, int32 label_len,
                     StringView initial, StrBuilder *result) {
    enum PromptResult prompt_result;

    (void)user;
    prompt_result = prompt_buffer(label, label_len, initial, result, true);
    if (prompt_result == PROMPT_RESULT_ACCEPTED) {
        return TAG_EDIT_PROMPT_ACCEPTED;
    }
    if (prompt_result == PROMPT_RESULT_ABORTED) {
        return TAG_EDIT_PROMPT_ABORTED;
    }
    return TAG_EDIT_PROMPT_ERROR;
}

static bool
tag_edit_hook_confirm(void *user, char *message, int32 message_len) {
    NcmStatusbarScopedLock scoped_lock;
    NcWindow *window;
    char values[2];
    char answer;
    int32 status;

    (void)user;
    values[0] = 'y';
    values[1] = 'n';
    answer = 'n';

    ncm_statusbar_scoped_lock_init(&scoped_lock);
    window = ncm_statusbar_put();
    nc_window_print_data(window, message, message_len);
    nc_window_print_data(window, STRLIT(" [y/n] "));
    status = ncm_statusbar_prompt_return_one_of(window, values,
                                                LENGTH(values), &answer);
    ncm_statusbar_scoped_lock_destroy(&scoped_lock);

    if ((status == 0) || (answer != 'y')) {
        ncm_statusbar_print(Config.message_delay_time,
                            STRLIT("Action cancelled"));
        return false;
    }
    return true;
}

static void
tag_edit_hook_status_message(void *user, char *message, int32 message_len) {
    (void)user;
    ncm_statusbar_print(Config.message_delay_time, message, message_len);
    return;
}

static void
tag_edit_hook_update_directory(void *user, char *directory,
                               int32 directory_len) {
    NcmError ncm_error = {0};

    (void)user;
    (void)directory_len;
    if (ncm_mpd_client_update_directory(&global_mpd, directory, NULL,
                                        &ncm_error) < 0) {
        ncm_statusbar_print(Config.message_delay_time,
                            ncm_error.message, ncm_error.message_len);
    }
    return;
}

void
app_screen_tag_edit_init(void) {
    TagEditHooks hooks = {0};
    static bool tag_edit_screen_initialized = false;

    if (tag_edit_screen_initialized) {
        return;
    }

    tag_edit_screen_init(&tag_edit_screen, 0, ui_state_screen_width(),
                           ui_state_main_start_y(), ui_state_main_height(),
                           Config.main_window_color, no_border());
    hooks.prompt = tag_edit_hook_prompt;
    hooks.confirm = tag_edit_hook_confirm;
    hooks.status_message = tag_edit_hook_status_message;
    hooks.update_directory = tag_edit_hook_update_directory;
    tag_edit_screen_set_hooks(&tag_edit_screen, hooks);
    tag_edit_screen_initialized = true;
    return;
}

static enum TinyTagEditPromptResult
tiny_tag_edit_prompt(void *user, char *label, int32 label_len,
                     StringView initial, StrBuilder *result) {
    enum PromptResult prompt_result;

    (void)user;
    prompt_result = prompt_buffer(label, label_len, initial, result, true);
    if (prompt_result == PROMPT_RESULT_ACCEPTED) {
        return TINY_TAG_EDIT_PROMPT_ACCEPTED;
    }
    if (prompt_result == PROMPT_RESULT_ABORTED) {
        return TINY_TAG_EDIT_PROMPT_ABORTED;
    }
    return TINY_TAG_EDIT_PROMPT_ERROR;
}

static void
tiny_tag_edit_status_message(void *user, char *message, int32 message_len) {
    (void)user;
    ncm_statusbar_print(Config.message_delay_time, message, message_len);
    return;
}

static void
tiny_tag_edit_update_directory(void *user,
                               char *directory, int32 directory_len) {
    NcmError ncm_error = {0};

    (void)user;
    (void)directory_len;
    if (ncm_mpd_client_update_directory(&global_mpd, directory, NULL,
                                        &ncm_error) < 0) {
        ncm_statusbar_print(Config.message_delay_time,
                            ncm_error.message, ncm_error.message_len);
    }
    return;
}

static void
tiny_tag_edit_update_playlist_song(void *user, MutableSong *song) {
    (void)user;
    playlist_screen_update_current_mutable_song(app_screen_playlist(), song);
    return;
}

static void
tiny_tag_edit_request_browser_update(void *user) {
    (void)user;
    browser_screen_request_update(app_screen_browser());
    return;
}

void
app_screen_tiny_tag_edit_init(void) {
    TinyTagEditHooks hooks = {0};
    static bool tiny_tag_edit_screen_initialized = false;

    if (tiny_tag_edit_screen_initialized) {
        return;
    }

    tiny_tag_edit_screen_init(&tiny_tag_edit_screen, 0,
                              ui_state_screen_width(),
                              ui_state_main_start_y(), ui_state_main_height(),
                              Config.main_window_color, no_border());
    hooks.prompt = tiny_tag_edit_prompt;
    hooks.status_message = tiny_tag_edit_status_message;
    hooks.update_directory = tiny_tag_edit_update_directory;
    hooks.update_playlist_song = tiny_tag_edit_update_playlist_song;
    hooks.request_browser_update = tiny_tag_edit_request_browser_update;
    tiny_tag_edit_screen_set_hooks(&tiny_tag_edit_screen, hooks);
    tiny_tag_edit_screen_initialized = true;
    return;
}

void
app_screen_outputs_toggle(void) {
#if defined(ENABLE_OUTPUTS)
    nc_outputs_screen_toggle_current(&outputs_screen.screen);
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
    #define APP_SCREEN_INIT_SCREEN(suffix)                                 \
        app_screen_##suffix##_init();

    APP_SCREEN_INIT_ALL_TYPES(APP_SCREEN_INIT_SCREEN)

    #undef APP_SCREEN_INIT_SCREEN
    return;
}

void
app_screens_register_initial(void) {
    #define APP_SCREEN_REGISTER_SCREEN(suffix)                             \
        app_screen_##suffix##_register();

    APP_SCREEN_REGISTER_INITIAL_TYPES(APP_SCREEN_REGISTER_SCREEN)

    #undef APP_SCREEN_REGISTER_SCREEN
    return;
}

void
app_screens_request_registered_resize(void) {
    #define APP_SCREEN_REQUEST_RESIZE(suffix, type)                        \
        app_request_registered_resize(type);

    APP_SCREEN_RESIZE_REQUEST_TYPES(APP_SCREEN_REQUEST_RESIZE)

    #undef APP_SCREEN_REQUEST_RESIZE
    return;
}

NcScreen *
app_screens_find_type(enum ScreenType screen_type) {
    enum NcScreenType type;

    type = screen_type_to_nc_type(screen_type);
    if (type == NC_SCREEN_TYPE_UNKNOWN) {
        return NULL;
    }
    return app_controller_find_screen_type(type);
}

int32
app_screens_switch_to_type(enum ScreenType screen_type) {
    NcScreen *screen;

    if ((screen = app_screens_find_type(screen_type)) == NULL) {
        return -ENOENT;
    }
    return nc_screen_switcher_switch_to(screen, screen->has_to_be_resized);
}

int32
app_screens_switch_or_open_type(enum ScreenType screen_type) {
    switch (screen_type) {
    #define APP_SCREEN_SWITCH_OR_OPEN_CASE(screen_type_value, nc_type, \
                                           nc_value, alias, flags, suffix) \
        case screen_type_value:                                     \
            return app_screen_##suffix##_show();

    SCREEN_TYPES(APP_SCREEN_SWITCH_OR_OPEN_CASE)

    #undef APP_SCREEN_SWITCH_OR_OPEN_CASE
    case SCREEN_TYPE_COUNT:
        break;
    default:
        break;
    }

    return -NCM_ERROR_UNAVAILABLE;
}

int32
app_screens_lock_current(void) {
    return app_controller_lock_current_screen();
}

enum ScreenType
app_screens_current_type(void) {
    NcScreen *screen;

    if ((screen = app_controller_current_screen()) == NULL) {
        return SCREEN_TYPE_COUNT;
    }
    return screen_type_from_nc_type(nc_screen_type(screen));
}

static void
draw_screen_header(NcScreen *screen) {
    char *title = nc_screen_title(screen);
    ncm_title_draw_header(title, optional_strlen32(title));
    return;
}

static void
resize_main_area(NcScreen *base, int32 *x, int32 *width) {
    nc_screen_switcher_get_resize_params(base, x, width, true);
    return;
}

static void
append_data(NcBuffer *buffer, char *string, int32 len) {
    if (len > 0) {
        nc_buffer_append_data(buffer, string, len);
    }
    return;
}

static void
append_format(NcBuffer *buffer, enum NcFormat format) {
    nc_buffer_add_format(buffer, buffer->len, format, 0);
    return;
}

static void
append_formatted_color(NcBuffer *buffer, NcFormattedColor *color) {
    nc_buffer_add_formatted_color(buffer, buffer->len, color, 0);
    return;
}

static void
append_formatted_color_end(NcBuffer *buffer, NcFormattedColor *color) {
    nc_buffer_add_formatted_color_end(buffer, buffer->len, color, 0);
    return;
}

static void
append_bold_label(NcBuffer *buffer, char *label) {
    append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_data(buffer, label, strlen32(label));
    append_format(buffer, NC_FORMAT_NO_BOLD);
    return;
}

static void
append_song_tag(NcBuffer *buffer, StrBuilder *tag) {
    if (tag->len <= 0) {
        append_formatted_color(buffer, &Config.empty_tag_color);
        append_data(buffer,
                    Config.empty_tag_marker, Config.empty_tag_marker_len);
        append_formatted_color_end(buffer, &Config.empty_tag_color);
        return;
    }
    append_data(buffer, tag->data, tag->len);
    return;
}

static void
append_song_key_value(NcBuffer *buffer, char *key, int32 key_len,
                      StrBuilder *value, bool empty_as_missing) {
    append_format(buffer, NC_FORMAT_BOLD);
    append_formatted_color(buffer, &Config.color1);
    nc_buffer_append_data(buffer, key, key_len);
    nc_buffer_append_data(buffer, STRLIT(":"));
    append_formatted_color_end(buffer, &Config.color1);
    append_format(buffer, NC_FORMAT_NO_BOLD);
    nc_buffer_append_data(buffer, STRLIT(" "));
    append_formatted_color(buffer, &Config.color2);
    if (empty_as_missing) {
        append_song_tag(buffer, value);
    } else {
        append_data(buffer, value->data, value->len);
    }
    append_formatted_color_end(buffer, &Config.color2);
    nc_buffer_append_data(buffer, STRLIT("\n"));
    return;
}

static void
append_help(NcBuffer *buffer, enum ActionType type, char *description) {
    int32 column_start;
    int32 width;

    nc_buffer_append_data(buffer, STRLIT("    "));
    column_start = buffer->len;
    width = 0;
    for (int32 i = 0; i < Bindings.keys_len; i += 1) {
        NcmKeyBindings *key_bindings = &Bindings.keys[i];
        char key_name[64];
        int32 key_len;

        for (int32 j = 0; j < key_bindings->bindings_len; j += 1) {
            Binding *binding = &key_bindings->bindings[j];

            if (!binding_is_single_action_type(binding, type)) {
                continue;
            }
            key_len = bindings_key_name(key_bindings->key, key_name,
                                            SIZEOF(key_name));
            if (key_len <= 0) {
                continue;
            }
            if (width > 0) {
                nc_buffer_append_data(buffer, STRLIT(" "));
                width += 1;
            }
            append_data(buffer, key_name, key_len);
            width += key_len;
        }
    }
    while ((buffer->len - column_start) < 20) {
        nc_buffer_append_char(buffer, ' ');
    }
    nc_buffer_append_data(buffer, STRLIT(" : "));
    nc_buffer_append_data(buffer, description, strlen32(description));
    nc_buffer_append_data(buffer, STRLIT("\n"));
    return;
}

static int32
help_render(void *user, NcBuffer *buffer) {
    (void)user;

    append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_data(buffer, STRLIT("\n  Keys - Movement\n\n"));
    append_format(buffer, NC_FORMAT_NO_BOLD);
    append_help(buffer, ACTION_SCROLL_UP, "Move cursor up");
    append_help(buffer, ACTION_SCROLL_DOWN, "Move cursor down");
    append_help(buffer, ACTION_PAGE_UP, "Page up");
    append_help(buffer, ACTION_PAGE_DOWN, "Page down");
    append_help(buffer, ACTION_MOVE_HOME, "Home");
    append_help(buffer, ACTION_MOVE_END, "End");
    append_help(buffer, ACTION_NEXT_SCREEN, "Next screen");
    append_help(buffer, ACTION_PREVIOUS_SCREEN, "Previous screen");
    append_help(buffer, ACTION_SHOW_HELP, "Show help");
    append_help(buffer, ACTION_SHOW_PLAYLIST, "Show playlist");
    append_help(buffer, ACTION_SHOW_BROWSER, "Show browser");
    append_help(buffer, ACTION_SHOW_SEARCH_ENGINE, "Show search engine");
    append_help(buffer, ACTION_SHOW_MEDIA_LIBRARY, "Show media library");
    append_help(buffer, ACTION_SHOW_PLAYLIST_EDITOR,
                "Show playlist editor");
    append_help(buffer, ACTION_SHOW_SERVER_INFO, "Show server info");
#if defined(ENABLE_OUTPUTS)
    append_help(buffer, ACTION_SHOW_OUTPUTS, "Show outputs");
#endif
#if defined(ENABLE_VISUALIZER)
    append_help(buffer, ACTION_SHOW_VISUALIZER, "Show music visualizer");
#endif
#if defined(HAVE_TAGLIB_H)
    append_help(buffer, ACTION_SHOW_TAG_EDIT, "Show tag editor");
#endif

    append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_data(buffer, STRLIT("\n  Keys - Global\n\n"));
    append_format(buffer, NC_FORMAT_NO_BOLD);

    append_help(buffer, ACTION_PLAY, "Play");
    append_help(buffer, ACTION_STOP, "Stop");
    append_help(buffer, ACTION_PAUSE, "Pause");
    append_help(buffer, ACTION_NEXT, "Next track");
    append_help(buffer, ACTION_PREVIOUS, "Previous track");
    append_help(buffer, ACTION_VOLUME_DOWN, "Decrease volume");
    append_help(buffer, ACTION_VOLUME_UP, "Increase volume");
    append_help(buffer, ACTION_TOGGLE_REPEAT, "Toggle repeat mode");
    append_help(buffer, ACTION_TOGGLE_RANDOM, "Toggle random mode");
    append_help(buffer, ACTION_TOGGLE_SINGLE, "Toggle single mode");
    append_help(buffer, ACTION_TOGGLE_CONSUME, "Toggle consume mode");
    append_help(buffer, ACTION_UPDATE_DATABASE, "Start database update");
    append_help(buffer, ACTION_EXECUTE_COMMAND, "Execute command");
    append_help(buffer, ACTION_QUIT, "Quit");

    return 1;
}

static void
help_switch_to(void *user) {
    (void)user;
    nc_screen_switcher_finish_switch(app_screen_help_base());
    draw_screen_header(app_screen_help_base());
    return;
}

static void
help_resize(void *user, NcHelpScreen *screen) {
    int32 x;
    int32 width;

    (void)user;
    resize_main_area(nc_help_screen_base(screen), &x, &width);
    nc_help_screen_set_geometry(screen, x, width, ui_state_main_start_y(),
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

void
app_screen_help_init(void) {
    NcHelpHooks hooks = {0};

    if (help_screen.initialized) {
        return;
    }

    hooks.render = help_render;
    hooks.switch_to = help_switch_to;
    hooks.resize_layout = help_resize;
    hooks.destroy = help_destroy;
    hooks.user = &help_screen;
    nc_help_screen_init(&help_screen.screen, hooks, 0, ui_state_screen_width(),
                        ui_state_main_start_y(), ui_state_main_height(),
                        Config.main_window_color, no_border(),
                        Config.lines_scrolled);
    help_screen.initialized = true;
    nc_help_screen_reload(&help_screen.screen);
    return;
}

static void
outputs_fetch(void *user, NcOutputsScreen *screen) {
#if defined(ENABLE_OUTPUTS)
    NcmMpdOutputList outputs = {0};
    NcmError ncm_error;

    (void)user;
    ncm_error_clear(&ncm_error);

    if (ncm_mpd_client_get_outputs(&global_mpd, &outputs, &ncm_error) < 0) {
        StrBuilder message = {0};

        SB_APPEND(&message, "Could not fetch outputs: ");
        SB_APPEND(&message,
                  ncm_error.message, ncm_error.message_len);
        ncm_statusbar_print(5, message.data, message.len);
        sb_free(&message);
        ncm_mpd_output_list_destroy(&outputs);
        return;
    }

    for (int32 i = 0; i < outputs.count; i += 1) {
        NcmMpdOutput *output;

        output = outputs.items + i;
        nc_outputs_screen_add_output(screen, output->id,
                                     output->name, output->name_len,
                                     output->enabled);
    }
    ncm_mpd_output_list_destroy(&outputs);
#else
    (void)user;
    (void)screen;
#endif
    return;
}

static int32
outputs_toggle(void *user, int32 id, bool enabled, char *name, int32 name_len) {
#if defined(ENABLE_OUTPUTS)
    NcmError ncm_error;
    int32 status;

    (void)user;
    ncm_error_clear(&ncm_error);
    if (enabled) {
        status = ncm_mpd_client_disable_output(&global_mpd, id, &ncm_error);
    } else {
        status = ncm_mpd_client_enable_output(&global_mpd, id, &ncm_error);
    }
    if (status < 0) {
        StrBuilder message = {0};

        SB_APPEND(&message, "Could not toggle output ");
        SB_APPEND(&message, name, name_len);
        SB_APPEND(&message, ": ");
        SB_APPEND(&message, ncm_error.message,
                  ncm_error.message_len);
        ncm_statusbar_print(5, message.data, message.len);
        sb_free(&message);
        return status;
    }

    {
        StrBuilder message = {0};

        SB_APPEND(&message, "Output ");
        SB_APPEND(&message, name, name_len);
        if (enabled) {
            SB_APPEND(&message, " disabled");
        } else {
            SB_APPEND(&message, " enabled");
        }
        ncm_statusbar_print(3, message.data, message.len);
        sb_free(&message);
    }
    return 0;
#else
    (void)user;
    (void)id;
    (void)enabled;
    (void)name;
    (void)name_len;
    return -NCM_ERROR_UNAVAILABLE;
#endif
}

static void
outputs_switch_to(void *user) {
    (void)user;
    nc_screen_switcher_finish_switch(app_screen_outputs_base());
    draw_screen_header(app_screen_outputs_base());
    return;
}

static void
outputs_resize(void *user, NcOutputsScreen *screen) {
    int32 x;
    int32 width;

    (void)user;
    resize_main_area(nc_outputs_screen_base(screen), &x, &width);
    nc_outputs_screen_set_geometry(screen, x, width, ui_state_main_start_y(),
                                   ui_state_main_height());
    return;
}

static void
outputs_destroy(void *user) {
#if defined(ENABLE_OUTPUTS)
    OutputsScreen *owner = user;
    owner->initialized = false;
#else
    (void)user;
#endif
    return;
}

void
app_screen_outputs_init(void) {
#if defined(ENABLE_OUTPUTS)
    NcOutputsHooks hooks = {0};
    NcBuffer prefix;
    NcBuffer suffix;

    if (outputs_screen.initialized) {
        return;
    }

    hooks.fetch_outputs = outputs_fetch;
    hooks.toggle_output = outputs_toggle;
    hooks.switch_to = outputs_switch_to;
    hooks.resize_layout = outputs_resize;
    hooks.destroy = outputs_destroy;
    hooks.user = &outputs_screen;
    nc_outputs_screen_init(&outputs_screen.screen, hooks,
                           0, ui_state_screen_width(),
                           ui_state_main_start_y(), ui_state_main_height(),
                           Config.main_window_color, Config.window_border_color,
                           Config.lines_scrolled,
                           Config.mouse_list_scroll_whole_page);
    prefix = (NcBuffer){0};
    suffix = (NcBuffer){0};
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

static void
server_info_load_lists(void *user) {
    ServerInfoScreen *owner;
    NcmError ncm_error;

    owner = user;
    ncm_error_clear(&ncm_error);
    ncm_mpd_client_get_url_handlers(&global_mpd, &owner->url_handlers,
                                    &ncm_error);
    ncm_error_clear(&ncm_error);
    ncm_mpd_client_get_tag_types(&global_mpd, &owner->tag_types, &ncm_error);
    return;
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
        nc_buffer_append_data(buffer, STRLIT("d "));
    }
    if ((days > 0) || (hours > 0)) {
        nc_buffer_append_int64(buffer, hours);
        nc_buffer_append_data(buffer, STRLIT("h "));
    }
    if ((days > 0) || (hours > 0) || (minutes > 0)) {
        nc_buffer_append_int64(buffer, minutes);
        nc_buffer_append_data(buffer, STRLIT("m "));
    }
    nc_buffer_append_int64(buffer, seconds);
    nc_buffer_append_data(buffer, STRLIT("s"));
    return;
}

static int32
server_info_render(void *user, NcBuffer *buffer) {
    ServerInfoScreen *owner;
    NcmMpdStats stats;
    NcmError ncm_error;
    char time_buffer[64];
    int32 status;

    owner = user;
    if (global_timer_elapsed_ms(owner->timer) < 1000) {
        return 0;
    }
    owner->timer = global_timer;

    ncm_error_clear(&ncm_error);
    status = ncm_mpd_client_get_stats(&global_mpd, &stats, &ncm_error);
    if (status < 0) {
        return status;
    }

    append_bold_label(buffer, "Version: ");
    nc_buffer_append_data(buffer, STRLIT("0."));
    nc_buffer_append_int64(buffer, ncm_mpd_client_version(&global_mpd));
    nc_buffer_append_data(buffer, STRLIT(".*\n"));

    append_bold_label(buffer, "Uptime: ");
    show_long_time(buffer, stats.uptime);
    nc_buffer_append_data(buffer, STRLIT("\n"));

    append_bold_label(buffer, "Time playing: ");
    ncm_song_show_time(stats.play_time, time_buffer, SIZEOF(time_buffer));
    nc_buffer_append_data(buffer, time_buffer, strlen32(time_buffer));
    nc_buffer_append_data(buffer, STRLIT("\n\n"));

    append_bold_label(buffer, "Total playtime: ");
    show_long_time(buffer, stats.db_play_time);
    nc_buffer_append_data(buffer, STRLIT("\n"));

    append_bold_label(buffer, "Artist names: ");
    nc_buffer_append_int64(buffer, stats.artists);
    nc_buffer_append_data(buffer, STRLIT("\n"));

    append_bold_label(buffer, "Album names: ");
    nc_buffer_append_int64(buffer, stats.albums);
    nc_buffer_append_data(buffer, STRLIT("\n"));

    append_bold_label(buffer, "Songs in database: ");
    nc_buffer_append_int64(buffer, stats.songs);
    nc_buffer_append_data(buffer, STRLIT("\n\n"));

    append_bold_label(buffer, "URL Handlers:");
    for (int32 i = 0; i < owner->url_handlers.count; i += 1) {
        StringView *handler = &owner->url_handlers.items[i];

        if (i == 0) {
            nc_buffer_append_data(buffer, STRLIT(" "));
        } else {
            nc_buffer_append_data(buffer, STRLIT(", "));
        }
        append_data(buffer, handler->data, handler->len);
    }
    nc_buffer_append_data(buffer, STRLIT("\n\n"));

    append_bold_label(buffer, "Tag Types:");
    for (int32 i = 0; i < owner->tag_types.count; i += 1) {
        StringView *tag = &owner->tag_types.items[i];

        if (i == 0) {
            nc_buffer_append_data(buffer, STRLIT(" "));
        } else {
            nc_buffer_append_data(buffer, STRLIT(", "));
        }

        append_data(buffer, tag->data, tag->len);
    }
    return 1;
}

static void
server_info_switch_to(void *user) {
    (void)user;
    nc_screen_switcher_finish_switch(app_screen_server_info_base());
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
    ServerInfoScreen *owner = user;

    ncm_mpd_string_list_destroy(&owner->url_handlers);
    ncm_mpd_string_list_destroy(&owner->tag_types);
    owner->initialized = false;

    return;
}

void
app_screen_server_info_init(void) {
    NcServerInfoHooks hooks = {0};

    if (server_info_screen.initialized) {
        return;
    }

    hooks.load_lists = server_info_load_lists;
    hooks.render = server_info_render;
    hooks.switch_to = server_info_switch_to;
    hooks.resize_layout = server_info_resize;
    hooks.title = server_info_title;
    hooks.destroy = server_info_destroy;
    hooks.user = &server_info_screen;
    server_info_screen.url_handlers = (StringViewList){0};
    server_info_screen.tag_types = (StringViewList){0};
    nc_server_info_screen_init(&server_info_screen.screen, hooks,
                               ui_state_screen_width(),
                               ui_state_screen_height(),
                               ui_state_main_start_y(), ui_state_main_height(),
                               Config.main_window_color,
                               Config.window_border_color);
    server_info_screen.initialized = true;
    return;
}

static int32
song_info_render(void *user, NcSongInfoScreen *screen, NcBuffer *buffer) {
    SongInfoScreen *owner = user;
    StrBuilder value;

    (void)screen;
    if (!owner->has_song) {
        return 0;
    }

    {
        char *name;
        int32 name_len;

        value = ncm_song_getter_buffer(&owner->song, SONG_GETTER_NAME, 0);
        name_len = ncm_song_getter_display_name_len(SONG_GETTER_NAME, &name);
        append_song_key_value(buffer, name, name_len, &value, false);
        sb_free(&value);

        value = ncm_song_getter_buffer(&owner->song, SONG_GETTER_DIRECTORY, 0);
        name_len = ncm_song_getter_display_name_len(SONG_GETTER_DIRECTORY,
                                                    &name);
        append_song_key_value(buffer, name, name_len, &value, true);
        sb_free(&value);
        nc_buffer_append_data(buffer, STRLIT("\n"));

        value = ncm_song_getter_buffer(&owner->song, SONG_GETTER_LENGTH, 0);
        name_len = ncm_song_getter_display_name_len(SONG_GETTER_LENGTH, &name);
        append_song_key_value(buffer, name, name_len, &value, false);
        sb_free(&value);
    }

    for (int32 i = 0; i < NCM_SONG_INFO_TAG_COUNT; i += 1) {
        append_format(buffer, NC_FORMAT_BOLD);
        nc_buffer_append_data(buffer, STRLIT("\n"));
        nc_buffer_append_data(buffer, ncm_song_info_tags[i].name,
                              ncm_song_info_tags[i].name_len);
        nc_buffer_append_data(buffer, STRLIT(":"));
        append_format(buffer, NC_FORMAT_NO_BOLD);
        nc_buffer_append_data(buffer, STRLIT(" "));
        value = ncm_song_tags_buffer(&owner->song, ncm_song_info_tags[i].get,
                                     Config.tags_separator,
                                     Config.tags_separator_len,
                                     Config.show_duplicate_tags);
        append_song_tag(buffer, &value);
        sb_free(&value);
    }
    return 1;
}

static void
song_info_switch_to(void *user, NcSongInfoScreen *screen) {
    SongInfoScreen *owner = user;
    NcmError ncm_error;

    ncm_error_clear(&ncm_error);
    ncm_song_destroy(&owner->song);
    owner->song = (NcmSong){0};
    owner->has_song = ncm_mpd_client_get_current_song(&global_mpd, &owner->song,
                                                      &ncm_error) == 0;
    if (!owner->has_song) {
        StrBuilder message = {0};

        SB_APPEND(&message, "Could not fetch current song: ");
        SB_APPEND(&message, ncm_error.message,
                  ncm_error.message_len);
        ncm_statusbar_print(5, message.data, message.len);
        sb_free(&message);
        return;
    }

    nc_screen_switcher_finish_switch(app_screen_song_info_base());
    nc_song_info_screen_prepare_current(screen);
    draw_screen_header(app_screen_song_info_base());
    return;
}

static void
song_info_resize(void *user, NcSongInfoScreen *screen) {
    int32 x;
    int32 width;

    (void)user;
    resize_main_area(nc_song_info_screen_base(screen), &x, &width);
    nc_song_info_screen_set_geometry(screen, x, width, ui_state_main_start_y(),
                                     ui_state_main_height());
    return;
}

static void
song_info_destroy(void *user) {
    SongInfoScreen *owner = user;

    ncm_song_destroy(&owner->song);
    owner->has_song = false;
    owner->initialized = false;

    return;
}

void
app_screen_song_info_init(void) {
    NcSongInfoHooks hooks = {0};

    if (song_info_screen.initialized) {
        return;
    }

    hooks.render = song_info_render;
    hooks.switch_to = song_info_switch_to;
    hooks.resize_layout = song_info_resize;
    hooks.destroy = song_info_destroy;
    hooks.user = &song_info_screen;
    song_info_screen.song = (NcmSong){0};
    nc_song_info_screen_init(&song_info_screen.screen, hooks,
                             0, ui_state_screen_width(),
                             ui_state_main_start_y(), ui_state_main_height(),
                             Config.main_window_color, no_border(),
                             Config.lines_scrolled);
    song_info_screen.initialized = true;
    return;
}

#endif /* APP_SCREENS_C */
