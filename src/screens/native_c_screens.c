#include "screens/native_c_screens.h"
#include "screens/nc_search_engine.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "actions.h"
#include "app_controller.h"
#include "bindings.h"
#include "c/ncm_base.h"
#include "c/ncm_display.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "global.h"
#include "settings.h"
#include "screens/screen_switcher.h"
#include "screens/song_info.h"
#include "statusbar.h"
#include "title.h"
#include "ui_state.h"

#if defined(HAVE_TAGLIB_H)
# include "c/ncm_taglib.h"
# include "c/ncm_tags.h"
#endif

struct NativeHelpScreen {
    NcHelpScreen screen;
    bool initialized;
};

struct NativeOutputsScreen {
    NcOutputsScreen screen;
    bool initialized;
};


struct NativeServerInfoScreen {
    NcServerInfoScreen screen;
    NcmMpdStringList url_handlers;
    NcmMpdStringList tag_types;
    NcmTimePoint timer;
    bool initialized;
};

struct NativeSongInfoScreen {
    NcSongInfoScreen screen;
    NcmSong song;
    bool has_song;
    bool initialized;
};

NcmSongInfoMetadata ncm_song_info_tags[] = {
    {"Title", NCM_SONG_GETTER_TITLE, NCM_TAGS_FIELD_TITLE},
    {"Artist", NCM_SONG_GETTER_ARTIST, NCM_TAGS_FIELD_ARTIST},
    {"Album Artist",
     NCM_SONG_GETTER_ALBUM_ARTIST,
     NCM_TAGS_FIELD_ALBUM_ARTIST},
    {"Album", NCM_SONG_GETTER_ALBUM, NCM_TAGS_FIELD_ALBUM},
    {"Date", NCM_SONG_GETTER_DATE, NCM_TAGS_FIELD_DATE},
    {"Track", NCM_SONG_GETTER_TRACK, NCM_TAGS_FIELD_TRACK},
    {"Genre", NCM_SONG_GETTER_GENRE, NCM_TAGS_FIELD_GENRE},
    {"Composer", NCM_SONG_GETTER_COMPOSER,
     NCM_TAGS_FIELD_COMPOSER},
    {"Performer", NCM_SONG_GETTER_PERFORMER,
     NCM_TAGS_FIELD_PERFORMER},
    {"Disc", NCM_SONG_GETTER_DISC, NCM_TAGS_FIELD_DISC},
    {"Comment", NCM_SONG_GETTER_COMMENT,
     NCM_TAGS_FIELD_COMMENT},
    {NULL, NCM_SONG_GETTER_NONE, NCM_TAGS_FIELD_LAST},
};

static NativeBrowserScreen browser_screen;
static bool browser_screen_initialized;
static struct NativeHelpScreen help_screen;
static struct NativeOutputsScreen outputs_screen;
static NativeLastfmScreen lastfm_screen;
static bool lastfm_screen_initialized;
static NativeLyricsScreen lyrics_screen;
static bool lyrics_screen_initialized;
static NativeVisualizerScreen visualizer_screen;
static bool visualizer_screen_initialized;
static NativePlaylistScreen playlist_screen;
static NativePlaylistEditorScreen playlist_editor_screen;
static NativeSelectedItemsAdderScreen selected_items_adder_screen;
static NativeSortPlaylistDialog sort_playlist_dialog;
static NativeSearchEngineScreen search_engine_screen;
static bool playlist_editor_screen_initialized;
static bool selected_items_adder_screen_initialized;
static bool sort_playlist_dialog_initialized;
static bool search_engine_screen_initialized;
static NativeMediaLibraryScreen media_library_screen;
static bool media_library_screen_initialized;
static NativeTagEditorScreen tag_editor_screen;
static bool tag_editor_screen_initialized;
static NativeTinyTagEditorScreen tiny_tag_editor_screen;
static bool tiny_tag_editor_screen_initialized;
static bool playlist_screen_initialized;
static struct NativeServerInfoScreen server_info_screen;
static struct NativeSongInfoScreen song_info_screen;

enum NativePromptResult {
    NATIVE_PROMPT_RESULT_ERROR,
    NATIVE_PROMPT_RESULT_ABORTED,
    NATIVE_PROMPT_RESULT_ACCEPTED,
};

static enum ScreenType native_screen_type_from_native_type(int32 type);
static void native_request_registered_resize(int32 type);
static NcBorder native_no_border(void);
static bool native_register_screen(NcScreen *screen);
static NativeTagEditorHooks native_tag_editor_hooks(void);
static NcHelpHooks native_help_hooks(void);
static NcOutputsHooks native_outputs_hooks(void);
static NcServerInfoHooks native_server_info_hooks(void);
static NcSongInfoHooks native_song_info_hooks(void);
static void native_show_long_time(NcBuffer *buffer, uint64 seconds);


void
native_c_screen_browser_init(void) {
    if (browser_screen_initialized) {
        return;
    }

    native_browser_screen_init(&browser_screen,
                               0,
                               ui_state_screen_width(),
                               ui_state_main_start_y(),
                               ui_state_main_height(),
                               Config.main_color,
                               native_no_border());
    native_browser_screen_set_mouse_config(
        &browser_screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);
    native_browser_screen_set_display_mode(
        &browser_screen, Config.browser_display_mode);
    browser_screen_initialized = true;
    return;
}

void
native_c_screen_browser_register(void) {
    native_c_screen_browser_init();
    assert(native_register_screen(native_c_screen_browser_native()));
    return;
}

void
native_c_screen_browser_switch_to(void) {
    (void)nc_screen_switcher_switch_to(native_c_screen_browser_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_browser_native()));
    return;
}

bool
native_c_screen_browser_is_current(void) {
    return nc_screen_switcher_is_current(native_c_screen_browser_native());
}

void
native_c_screen_browser_fetch_supported_extensions(void) {
    NcmError error;

    ncm_error_clear(&error);
    if (!native_browser_screen_fetch_supported_extensions(
            native_c_screen_browser(), &global_mpd, &error)
        && ncm_error_is_set(&error)) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    error.message);
    }
    return;
}

NativeBrowserScreen *
native_c_screen_browser(void) {
    native_c_screen_browser_init();
    return &browser_screen;
}

NcScreen *
native_c_screen_browser_native(void) {
    native_c_screen_browser_init();
    return native_browser_screen_base(&browser_screen);
}

void
native_c_screen_help_init(void) {
    if (help_screen.initialized) {
        return;
    }

    nc_help_screen_init(&help_screen.screen,
                        native_help_hooks(),
                        0,
                        ui_state_screen_width(),
                        ui_state_main_start_y(),
                        ui_state_main_height(),
                        Config.main_color,
                        native_no_border(),
                        Config.lines_scrolled);
    help_screen.initialized = true;
    (void)nc_help_screen_reload(&help_screen.screen);
    return;
}

void
native_c_screen_help_register(void) {
    native_c_screen_help_init();
    assert(native_register_screen(native_c_screen_help_native()));
    return;
}

void
native_c_screen_help_switch_to(void) {
    (void)nc_screen_switcher_switch_to(native_c_screen_help_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_help_native()));
    return;
}

bool
native_c_screen_help_is_current(void) {
    return nc_screen_switcher_is_current(native_c_screen_help_native());
}

NcHelpScreen *
native_c_screen_help_typed(void) {
    native_c_screen_help_init();
    return &help_screen.screen;
}


void
native_c_screen_lastfm_init(void) {
    if (lastfm_screen_initialized) {
        return;
    }

    native_lastfm_screen_init(&lastfm_screen,
                              0,
                              ui_state_screen_width(),
                              ui_state_main_start_y(),
                              ui_state_main_height(),
                              Config.main_color,
                              native_no_border(),
                              Config.lines_scrolled);
    lastfm_screen_initialized = true;
    return;
}

void
native_c_screen_lastfm_register(void) {
    native_c_screen_lastfm_init();
    assert(native_register_screen(native_c_screen_lastfm_native()));
    return;
}

void
native_c_screen_lastfm_switch_to(void) {
    NcScreen *screen;
    NcScreen *previous;

    native_c_screen_lastfm_register();
    screen = native_c_screen_lastfm_native();
    if (nc_screen_switcher_is_current(screen)) {
        previous = nc_screen_switcher_previous();
        if ((previous != NULL)
            && app_controller_is_screen_registered(previous)) {
            (void)nc_screen_switcher_switch_to(
                previous, nc_screen_has_to_be_resized(previous));
        }
        return;
    }

    (void)nc_screen_switcher_switch_to(screen,
                                       nc_screen_has_to_be_resized(screen));
    return;
}

bool
native_c_screen_lastfm_is_current(void) {
    return nc_screen_switcher_is_current(native_c_screen_lastfm_native());
}

NativeLastfmScreen *
native_c_screen_lastfm(void) {
    native_c_screen_lastfm_init();
    return &lastfm_screen;
}

NcScreen *
native_c_screen_lastfm_native(void) {
    native_c_screen_lastfm_init();
    return native_lastfm_screen_base(&lastfm_screen);
}

void
native_c_screen_lyrics_init(void) {
    if (lyrics_screen_initialized) {
        return;
    }

    native_lyrics_screen_init(&lyrics_screen,
                              0,
                              ui_state_screen_width(),
                              ui_state_main_start_y(),
                              ui_state_main_height(),
                              Config.main_color,
                              native_no_border(),
                              Config.lines_scrolled);
    lyrics_screen_initialized = true;
    return;
}

void
native_c_screen_lyrics_register(void) {
    native_c_screen_lyrics_init();
    assert(native_register_screen(native_c_screen_lyrics_native()));
    return;
}

void
native_c_screen_lyrics_set_resize(void) {
    nc_screen_request_resize(native_c_screen_lyrics_native());
    return;
}

void
native_c_screen_lyrics_switch_to(void) {
    NcScreen *screen;
    NcScreen *previous;

    native_c_screen_lyrics_register();
    screen = native_c_screen_lyrics_native();
    if (nc_screen_switcher_is_current(screen)) {
        previous = nc_screen_switcher_previous();
        if ((previous != NULL)
            && app_controller_is_screen_registered(previous)) {
            (void)nc_screen_switcher_switch_to(
                previous, nc_screen_has_to_be_resized(previous));
        }
        return;
    }

    (void)nc_screen_switcher_switch_to(screen,
                                       nc_screen_has_to_be_resized(screen));
    return;
}

bool
native_c_screen_lyrics_is_current(void) {
    return nc_screen_switcher_is_current(native_c_screen_lyrics_native());
}

NativeLyricsScreen *
native_c_screen_lyrics(void) {
    native_c_screen_lyrics_init();
    return &lyrics_screen;
}

NcScreen *
native_c_screen_lyrics_native(void) {
    native_c_screen_lyrics_init();
    return native_lyrics_screen_base(&lyrics_screen);
}

void
native_c_screen_visualizer_init(void) {
#if defined(ENABLE_VISUALIZER)
    NativeVisualizerScreenConfig visualizer_config = {0};

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
    visualizer_config.fps = (int32)Config.visualizer_fps;
    visualizer_config.spectrum_dft_size =
        Config.visualizer_spectrum_dft_size;
    visualizer_config.spectrum_gain = Config.visualizer_spectrum_gain;
    visualizer_config.spectrum_hz_min = Config.visualizer_spectrum_hz_min;
    visualizer_config.spectrum_hz_max = Config.visualizer_spectrum_hz_max;
    visualizer_config.data_source_hooks =
        native_visualizer_data_source_system_hooks(&global_mpd);
    visualizer_config.visualization_type =
        (enum NativeVisualizerType)Config.visualizer_type;
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

    native_visualizer_screen_init(&visualizer_screen,
                                  0,
                                  ui_state_main_start_y(),
                                  ui_state_screen_width(),
                                  ui_state_main_height(),
                                  Config.main_color,
                                  native_no_border(),
                                  &visualizer_config);
    visualizer_screen_initialized = true;
#endif
    return;
}

void
native_c_screen_visualizer_register(void) {
#if defined(ENABLE_VISUALIZER)
    native_c_screen_visualizer_init();
    assert(native_register_screen(native_c_screen_visualizer_native()));
#endif
    return;
}

bool
native_c_screen_visualizer_is_current(void) {
#if defined(ENABLE_VISUALIZER)
    return nc_screen_switcher_is_current(native_c_screen_visualizer_native());
#else
    return false;
#endif
}

NativeVisualizerScreen *
native_c_screen_visualizer(void) {
    native_c_screen_visualizer_init();
    return &visualizer_screen;
}

NcScreen *
native_c_screen_visualizer_native(void) {
    native_c_screen_visualizer_init();
    return native_visualizer_screen_base(&visualizer_screen);
}

NcScreen *
native_c_screen_help_native(void) {
    native_c_screen_help_init();
    return nc_help_screen_base(&help_screen.screen);
}

void
native_c_screen_playlist_init(void) {
    if (playlist_screen_initialized) {
        return;
    }

    native_playlist_screen_init(&playlist_screen,
                                0,
                                ui_state_screen_width(),
                                ui_state_main_start_y(),
                                ui_state_main_height(),
                                Config.main_color,
                                native_no_border());
    native_playlist_screen_set_mouse_config(
        &playlist_screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);
    playlist_screen_initialized = true;
    return;
}

void
native_c_screen_playlist_register(void) {
    native_c_screen_playlist_init();
    assert(native_register_screen(native_c_screen_playlist_native()));
    return;
}

void
native_c_screen_playlist_switch_to(void) {
    (void)nc_screen_switcher_switch_to(native_c_screen_playlist_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_playlist_native()));
    return;
}

bool
native_c_screen_playlist_is_current(void) {
    return nc_screen_switcher_is_current(native_c_screen_playlist_native());
}

NativePlaylistScreen *
native_c_screen_playlist(void) {
    native_c_screen_playlist_init();
    return &playlist_screen;
}

NcScreen *
native_c_screen_playlist_native(void) {
    native_c_screen_playlist_init();
    return native_playlist_screen_base(&playlist_screen);
}

void
native_c_screen_playlist_editor_init(void) {
    if (playlist_editor_screen_initialized) {
        return;
    }
    native_playlist_editor_screen_init(&playlist_editor_screen,
                                       0,
                                       ui_state_screen_width(),
                                       ui_state_main_start_y(),
                                       ui_state_main_height(),
                                       Config.main_color,
                                       native_no_border());
    if (Config.playlist_editor_column_width_ratio.len >= 2
        && Config.playlist_editor_column_width_ratio.items[0] > 0
        && Config.playlist_editor_column_width_ratio.items[1] > 0) {
        native_playlist_editor_screen_set_column_ratio(
            &playlist_editor_screen,
            Config.playlist_editor_column_width_ratio.items[0],
            Config.playlist_editor_column_width_ratio.items[1]);
    }
    playlist_editor_screen_initialized = true;
    return;
}

void
native_c_screen_playlist_editor_register(void) {
    native_c_screen_playlist_editor_init();
    assert(native_register_screen(native_c_screen_playlist_editor_native()));
    return;
}

void
native_c_screen_playlist_editor_switch_to(void) {
    (void)nc_screen_switcher_switch_to(
        native_c_screen_playlist_editor_native(),
        nc_screen_has_to_be_resized(
            native_c_screen_playlist_editor_native()));
    return;
}

NativePlaylistEditorScreen *
native_c_screen_playlist_editor(void) {
    native_c_screen_playlist_editor_init();
    return &playlist_editor_screen;
}

NcScreen *
native_c_screen_playlist_editor_native(void) {
    native_c_screen_playlist_editor_init();
    return native_playlist_editor_screen_base(&playlist_editor_screen);
}

void
native_c_screen_selected_items_adder_init(void) {
    if (selected_items_adder_screen_initialized) {
        return;
    }
    native_selected_items_adder_screen_init(
        &selected_items_adder_screen, 0, ui_state_main_start_y(),
        ui_state_screen_width(), ui_state_main_height(),
        Config.main_color, Config.window_border);
    selected_items_adder_screen_initialized = true;
    return;
}

void
native_c_screen_selected_items_adder_register(void) {
    NcScreen *registered;
    NcScreen *screen;
    bool success;

    native_c_screen_selected_items_adder_init();
    screen = native_c_screen_selected_items_adder_native();
    registered = app_controller_find_screen_type(
        NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER);
    if ((registered != NULL) && (registered != screen)) {
        success = app_controller_unregister_screen(registered);
        assert(success);
        if (!success) {
            return;
        }
    }
    success = native_register_screen(screen);
    assert(success);
    (void)success;
    return;
}

void
native_c_screen_selected_items_adder_switch_to(void) {
    (void)nc_screen_switcher_switch_to(
        native_c_screen_selected_items_adder_native(),
        nc_screen_has_to_be_resized(
            native_c_screen_selected_items_adder_native()));
    return;
}

bool
native_c_screen_selected_items_adder_open(NcmSongArray *songs,
                                          NcmError *error) {
    native_c_screen_selected_items_adder_register();
    return native_selected_items_adder_screen_open(
        native_c_screen_selected_items_adder(), songs,
        native_c_screen_playlist(), &global_mpd, error);
}

NativeSelectedItemsAdderScreen *
native_c_screen_selected_items_adder(void) {
    native_c_screen_selected_items_adder_init();
    return &selected_items_adder_screen;
}

NcScreen *
native_c_screen_selected_items_adder_native(void) {
    native_c_screen_selected_items_adder_init();
    return native_selected_items_adder_screen_base(
        &selected_items_adder_screen);
}

void
native_c_screen_sort_playlist_dialog_init(void) {
    if (sort_playlist_dialog_initialized) {
        return;
    }
    native_sort_playlist_dialog_init(&sort_playlist_dialog, 0,
                                     ui_state_main_start_y(),
                                     30, ui_state_main_height(),
                                     Config.main_color,
                                     Config.window_border);
    sort_playlist_dialog_initialized = true;
    return;
}

void
native_c_screen_sort_playlist_dialog_register(void) {
    NcScreen *registered;
    NcScreen *screen;
    bool success;

    native_c_screen_sort_playlist_dialog_init();
    screen = native_c_screen_sort_playlist_dialog_native();
    registered = app_controller_find_screen_type(
        NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG);
    if ((registered != NULL) && (registered != screen)) {
        success = app_controller_unregister_screen(registered);
        assert(success);
        if (!success) {
            return;
        }
    }
    success = native_register_screen(screen);
    assert(success);
    (void)success;
    return;
}

bool
native_c_screen_sort_playlist_dialog_switch_to(void) {
    NcmError error;
    bool success;

    ncm_error_clear(&error);
    success = native_sort_playlist_dialog_open(
        native_c_screen_sort_playlist_dialog(),
        native_c_screen_playlist(), &global_mpd,
        Config.ignore_leading_the, &error);
    if (!success && ncm_error_is_set(&error)) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time, error.message);
    }
    return success;
}

NativeSortPlaylistDialog *
native_c_screen_sort_playlist_dialog(void) {
    native_c_screen_sort_playlist_dialog_init();
    return &sort_playlist_dialog;
}

NcScreen *
native_c_screen_sort_playlist_dialog_native(void) {
    native_c_screen_sort_playlist_dialog_init();
    return native_sort_playlist_dialog_base(&sort_playlist_dialog);
}

static bool
native_search_list_database_songs(
    void *user, NcmSongArray *songs, NcmError *error) {
    NcmMpdSongList source;
    bool result;

    (void)user;
    if (songs == NULL) {
        return false;
    }

    ncm_song_array_clear(songs);
    ncm_mpd_song_list_init(&source);
    result = ncm_mpd_client_get_directory_recursive(
        &global_mpd, "/", &source, error);
    if (result) {
        result = ncm_mpd_song_list_to_song_array(&source, songs);
        if (!result) {
            ncm_error_set(error, EIO,
                          STRLIT_ARGS("failed to copy database songs"));
        }
    }
    ncm_mpd_song_list_destroy(&source);
    return result;
}

static bool
native_search_snapshot_playlist(
    void *user, NcmSongArray *songs, NcmError *error) {
    NativePlaylistScreen *playlist;
    NcSongMenu *song_menu;
    NcMenu *menu;
    NcmSong *song;
    int64 count;

    (void)user;
    (void)error;
    if (songs == NULL) {
        return false;
    }

    ncm_song_array_clear(songs);
    playlist = native_c_screen_playlist();
    song_menu = native_playlist_screen_song_menu(playlist);
    menu = nc_song_menu_base(song_menu);
    count = nc_menu_all_item_count(menu);
    for (int64 i = 0; i < count; i += 1) {
        song = nc_song_menu_item_at(song_menu, NC_MENU_ITEMS_ALL, i);
        if (song == NULL) {
            continue;
        }
        if (!ncm_song_array_append_copy(songs, song)) {
            ncm_error_set(error, EIO,
                          STRLIT_ARGS("failed to copy playlist songs"));
            return false;
        }
    }
    return true;
}

static bool
native_search_prompt_hook(char *text, void *user) {
    (void)user;
    return ncm_statusbar_main_hook(text, optional_strlen32(text));
}

static enum NativeSearchEnginePromptResult
native_search_prompt_constraint(
    void *user, char *label, int32 label_len, NcmBuffer *initial,
    NcmBuffer *result) {
    NcmStatusbarScopedLock lock;
    enum NcPromptStatus status;
    NcPrompt prompt = {0};
    NcWindow *window;
    char *input;
    char *initial_text;
    int32 input_len;
    bool copied;

    (void)user;
    if ((label == NULL) || (label_len < 0) || (initial == NULL)
        || (result == NULL)) {
        return NATIVE_SEARCH_ENGINE_PROMPT_ERROR;
    }

    input = NULL;
    initial_text = initial->data;
    if (initial_text == NULL) {
        initial_text = "";
    }

    ncm_statusbar_scoped_lock_init(&lock);
    window = ncm_statusbar_put();
    if (window == NULL) {
        ncm_statusbar_scoped_lock_destroy(&lock);
        return NATIVE_SEARCH_ENGINE_PROMPT_ERROR;
    }
    nc_window_print_data(window, label, label_len);
    nc_window_print_data(window, STRLIT_ARGS(": "));

    prompt.initial_text = initial_text;
    prompt.width = -1;
    prompt.hook = native_search_prompt_hook;
    prompt.hook_user_data = NULL;
    prompt.encrypted = false;
    prompt.remember = true;
    status = nc_window_prompt(window, &prompt, &input);
    ncm_statusbar_scoped_lock_destroy(&lock);

    if ((status != NC_PROMPT_ACCEPTED) || (input == NULL)) {
        nc_window_prompt_result_destroy(input);
        if (status == NC_PROMPT_ABORTED) {
            return NATIVE_SEARCH_ENGINE_PROMPT_ABORTED;
        }
        return NATIVE_SEARCH_ENGINE_PROMPT_ERROR;
    }

    input_len = optional_strlen32(input);
    copied = ncm_buffer_set(result, input, input_len);
    nc_window_prompt_result_destroy(input);
    if (!copied) {
        return NATIVE_SEARCH_ENGINE_PROMPT_ERROR;
    }
    return NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED;
}

static void
native_search_status_message(
    void *user, char *message, int32 message_len) {
    (void)user;
    ncm_statusbar_print((int32)Config.message_delay_time,
                        message, message_len);
    return;
}

static bool
native_search_add_song(
    void *user, NcmSong *song, bool play, NcmError *error) {
    (void)user;
    (void)error;
    return ncm_action_add_song_to_playlist(song, play, -1);
}

static bool
native_search_format_song(
    void *user, NcmSong *song, NcmBuffer *text) {
    NativeSearchEngineScreen *screen;

    screen = user;
    if ((screen == NULL) || (song == NULL) || (text == NULL)) {
        return false;
    }
    return native_search_engine_screen_format_song_text(
        screen, song, text);
}

void
native_c_screen_search_engine_init(void) {
    NativeSearchEngineHooks hooks = {0};
    enum NativeSearchEngineSearchMode mode;

    if (search_engine_screen_initialized) {
        return;
    }

    native_search_engine_screen_init(&search_engine_screen,
                                     0,
                                     ui_state_screen_width(),
                                     ui_state_main_start_y(),
                                     ui_state_main_height(),
                                     Config.main_color,
                                     native_no_border());

    mode = NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL;
    if (Config.search_engine_default_search_mode
        < NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST) {
        mode = (enum NativeSearchEngineSearchMode)
            Config.search_engine_default_search_mode;
    }
    (void)native_search_engine_screen_set_search_mode(
        &search_engine_screen, mode);
    native_search_engine_screen_set_search_source(
        &search_engine_screen, Config.search_in_db);

    hooks.client = &global_mpd;
    hooks.list_database_songs = native_search_list_database_songs;
    hooks.snapshot_playlist = native_search_snapshot_playlist;
    hooks.prompt_constraint = native_search_prompt_constraint;
    hooks.status_message = native_search_status_message;
    hooks.add_song = native_search_add_song;
    hooks.format_song = native_search_format_song;
    hooks.user = &search_engine_screen;
    native_search_engine_screen_set_hooks(&search_engine_screen, hooks);
    native_search_engine_screen_set_mouse_config(
        &search_engine_screen, Config.lines_scrolled,
        Config.mouse_list_scroll_whole_page);

    search_engine_screen_initialized = true;
    return;
}

void
native_c_screen_search_engine_register(void) {
    native_c_screen_search_engine_init();
    assert(native_register_screen(native_c_screen_search_engine_native()));
    return;
}

void
native_c_screen_search_engine_switch_to(void) {
    (void)nc_screen_switcher_switch_to(
        native_c_screen_search_engine_native(),
        nc_screen_has_to_be_resized(native_c_screen_search_engine_native()));
    return;
}

NativeSearchEngineScreen *
native_c_screen_search_engine(void) {
    native_c_screen_search_engine_init();
    return &search_engine_screen;
}

NcScreen *
native_c_screen_search_engine_native(void) {
    native_c_screen_search_engine_init();
    return native_search_engine_screen_base(&search_engine_screen);
}

void
native_c_screen_media_library_init(void) {
    NativeMediaLibraryHooks hooks;

    if (media_library_screen_initialized) {
        return;
    }

    hooks = native_media_library_mpd_hooks(&global_mpd);
    native_media_library_screen_init(&media_library_screen, hooks,
                                     0,
                                     ui_state_screen_width(),
                                     ui_state_main_start_y(),
                                     ui_state_main_height(),
                                     Config.main_color,
                                     native_no_border());
    media_library_screen_initialized = true;
    return;
}

void
native_c_screen_media_library_register(void) {
    native_c_screen_media_library_init();
    assert(native_register_screen(native_c_screen_media_library_native()));
    return;
}

void
native_c_screen_media_library_switch_to(void) {
    (void)nc_screen_switcher_switch_to(
        native_c_screen_media_library_native(),
        nc_screen_has_to_be_resized(native_c_screen_media_library_native()));
    return;
}

NativeMediaLibraryScreen *
native_c_screen_media_library(void) {
    native_c_screen_media_library_init();
    return &media_library_screen;
}

NcScreen *
native_c_screen_media_library_native(void) {
    native_c_screen_media_library_init();
    return native_media_library_screen_base(&media_library_screen);
}

void
native_c_screen_tag_editor_init(void) {
    NativeTagEditorHooks hooks;

    if (tag_editor_screen_initialized) {
        return;
    }

    native_tag_editor_screen_init(&tag_editor_screen, 0,
                                  ui_state_screen_width(),
                                  ui_state_main_start_y(),
                                  ui_state_main_height(),
                                  Config.main_color,
                                  native_no_border());
    hooks = native_tag_editor_hooks();
    native_tag_editor_screen_set_hooks(&tag_editor_screen, hooks);
    tag_editor_screen_initialized = true;
    return;
}

void
native_c_screen_tag_editor_register(void) {
    native_c_screen_tag_editor_init();
    assert(native_register_screen(native_c_screen_tag_editor_native()));
    return;
}

void
native_c_screen_tag_editor_switch_to(void) {
    (void)nc_screen_switcher_switch_to(
        native_c_screen_tag_editor_native(),
        nc_screen_has_to_be_resized(native_c_screen_tag_editor_native()));
    return;
}

NativeTagEditorScreen *
native_c_screen_tag_editor(void) {
    native_c_screen_tag_editor_init();
    return &tag_editor_screen;
}

NcScreen *
native_c_screen_tag_editor_native(void) {
    native_c_screen_tag_editor_init();
    return native_tag_editor_screen_base(&tag_editor_screen);
}

static bool
native_statusbar_prompt_hook(char *text, void *user) {
    (void)user;
    return ncm_statusbar_main_hook(text, optional_strlen32(text));
}

static enum NativePromptResult
native_prompt_buffer(char *label, int32 label_len,
                     NcmStringView initial, NcmBuffer *result,
                     bool bold_label) {
    NcmStatusbarScopedLock lock;
    enum NcPromptStatus status;
    NcPrompt prompt = {0};
    NcWindow *window;
    char *input;
    char *initial_text;
    int32 input_len;
    bool copied;

    if ((label == NULL) || (label_len < 0) || (initial.len < 0)
        || (result == NULL)
        || ((initial.data == NULL) && (initial.len > 0))) {
        return NATIVE_PROMPT_RESULT_ERROR;
    }

    input = NULL;
    initial_text = initial.data;
    if (initial_text == NULL) {
        initial_text = "";
    }

    ncm_statusbar_scoped_lock_init(&lock);
    window = ncm_statusbar_put();
    if (window == NULL) {
        ncm_statusbar_scoped_lock_destroy(&lock);
        return NATIVE_PROMPT_RESULT_ERROR;
    }
    if (bold_label) {
        nc_window_apply_format(window, NC_FORMAT_BOLD);
    }
    nc_window_print_data(window, label, label_len);
    nc_window_print_data(window, STRLIT_ARGS(": "));
    if (bold_label) {
        nc_window_apply_format(window, NC_FORMAT_NO_BOLD);
    }

    prompt.initial_text = initial_text;
    prompt.width = -1;
    prompt.hook = native_statusbar_prompt_hook;
    prompt.hook_user_data = NULL;
    prompt.encrypted = false;
    prompt.remember = true;
    status = nc_window_prompt(window, &prompt, &input);
    ncm_statusbar_scoped_lock_destroy(&lock);

    if ((status != NC_PROMPT_ACCEPTED) || (input == NULL)) {
        nc_window_prompt_result_destroy(input);
        if (status == NC_PROMPT_ABORTED) {
            return NATIVE_PROMPT_RESULT_ABORTED;
        }
        return NATIVE_PROMPT_RESULT_ERROR;
    }

    input_len = optional_strlen32(input);
    copied = ncm_buffer_set(result, input, input_len);
    nc_window_prompt_result_destroy(input);
    if (!copied) {
        return NATIVE_PROMPT_RESULT_ERROR;
    }
    return NATIVE_PROMPT_RESULT_ACCEPTED;
}

static enum NativeTagEditorPromptResult
native_tag_editor_prompt(
    void *user, char *label, int32 label_len, NcmStringView initial,
    NcmBuffer *result) {
    enum NativePromptResult prompt_result;

    (void)user;
    prompt_result = native_prompt_buffer(label, label_len, initial,
                                         result, true);
    if (prompt_result == NATIVE_PROMPT_RESULT_ACCEPTED) {
        return NATIVE_TAG_EDITOR_PROMPT_ACCEPTED;
    }
    if (prompt_result == NATIVE_PROMPT_RESULT_ABORTED) {
        return NATIVE_TAG_EDITOR_PROMPT_ABORTED;
    }
    return NATIVE_TAG_EDITOR_PROMPT_ERROR;
}

static bool
native_tag_editor_confirm(
    void *user, char *message, int32 message_len) {
    NcmStatusbarScopedLock lock;
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

    ncm_statusbar_scoped_lock_init(&lock);
    window = ncm_statusbar_put();
    if (window != NULL) {
        nc_window_print_data(window, message, message_len);
        nc_window_print_data(window, STRLIT_ARGS(" [y/n] "));
        prompted = ncm_statusbar_prompt_return_one_of(
            window, values, NCM_ARRAY_LEN(values), &answer);
    }
    ncm_statusbar_scoped_lock_destroy(&lock);

    if (!prompted || (answer != 'y')) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time, "Action cancelled");
        return false;
    }
    return true;
}

static void
native_tag_editor_status_message(
    void *user, char *message, int32 message_len) {
    (void)user;
    ncm_statusbar_print((int32)Config.message_delay_time,
                        message, message_len);
    return;
}

static void
native_tag_editor_update_directory(
    void *user, char *directory, int32 directory_len) {
    NcmError error = {0};

    (void)user;
    (void)directory_len;
    if (!ncm_mpd_client_update_directory(
            &global_mpd, directory, NULL, &error)) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time, error.message);
    }
    return;
}

static NativeTagEditorHooks
native_tag_editor_hooks(void) {
    NativeTagEditorHooks hooks = {0};

    hooks.prompt = native_tag_editor_prompt;
    hooks.confirm = native_tag_editor_confirm;
    hooks.status_message = native_tag_editor_status_message;
    hooks.update_directory = native_tag_editor_update_directory;
    return hooks;
}

static enum NativeTinyTagEditorPromptResult
native_tiny_tag_editor_prompt(
    void *user, char *label, int32 label_len, NcmStringView initial,
    NcmBuffer *result) {
    enum NativePromptResult prompt_result;

    (void)user;
    prompt_result = native_prompt_buffer(label, label_len, initial,
                                         result, true);
    if (prompt_result == NATIVE_PROMPT_RESULT_ACCEPTED) {
        return NATIVE_TINY_TAG_EDITOR_PROMPT_ACCEPTED;
    }
    if (prompt_result == NATIVE_PROMPT_RESULT_ABORTED) {
        return NATIVE_TINY_TAG_EDITOR_PROMPT_ABORTED;
    }
    return NATIVE_TINY_TAG_EDITOR_PROMPT_ERROR;
}

static void
native_tiny_tag_editor_status_message(
    void *user, char *message, int32 message_len) {
    (void)user;
    ncm_statusbar_print((int32)Config.message_delay_time,
                        message, message_len);
    return;
}

static void
native_tiny_tag_editor_update_directory(
    void *user, char *directory, int32 directory_len) {
    NcmError error = {0};

    (void)user;
    (void)directory_len;
    if (!ncm_mpd_client_update_directory(
            &global_mpd, directory, NULL, &error)) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time, error.message);
    }
    return;
}

static void
native_tiny_tag_editor_update_playlist_song(
    void *user, NcmMutableSong *song) {
    (void)user;
    (void)native_playlist_screen_update_current_mutable_song(
        native_c_screen_playlist(), song);
    return;
}

static void
native_tiny_tag_editor_request_browser_update(void *user) {
    (void)user;
    native_browser_screen_request_update(native_c_screen_browser());
    return;
}

void
native_c_screen_tiny_tag_editor_init(void) {
    NativeTinyTagEditorHooks hooks = {0};

    if (tiny_tag_editor_screen_initialized) {
        return;
    }

    native_tiny_tag_editor_screen_init(&tiny_tag_editor_screen, 0,
                                       ui_state_screen_width(),
                                       ui_state_main_start_y(),
                                       ui_state_main_height(),
                                       Config.main_color,
                                       native_no_border());
    hooks.prompt = native_tiny_tag_editor_prompt;
    hooks.status_message = native_tiny_tag_editor_status_message;
    hooks.update_directory = native_tiny_tag_editor_update_directory;
    hooks.update_playlist_song =
        native_tiny_tag_editor_update_playlist_song;
    hooks.request_browser_update =
        native_tiny_tag_editor_request_browser_update;
    native_tiny_tag_editor_screen_set_hooks(
        &tiny_tag_editor_screen, hooks);
    tiny_tag_editor_screen_initialized = true;
    return;
}

void
native_c_screen_tiny_tag_editor_register(void) {
    native_c_screen_tiny_tag_editor_init();
    assert(native_register_screen(native_c_screen_tiny_tag_editor_native()));
    return;
}

void
native_c_screen_tiny_tag_editor_switch_to(void) {
    native_c_screen_tiny_tag_editor_register();
    (void)nc_screen_switcher_switch_to(
        native_c_screen_tiny_tag_editor_native(),
        nc_screen_has_to_be_resized(
            native_c_screen_tiny_tag_editor_native()));
    return;
}

NativeTinyTagEditorScreen *
native_c_screen_tiny_tag_editor(void) {
    native_c_screen_tiny_tag_editor_init();
    return &tiny_tag_editor_screen;
}

NcScreen *
native_c_screen_tiny_tag_editor_native(void) {
    native_c_screen_tiny_tag_editor_init();
    return native_tiny_tag_editor_screen_base(&tiny_tag_editor_screen);
}

void
native_c_screen_song_info_init(void) {
    if (song_info_screen.initialized) {
        return;
    }

    ncm_song_init(&song_info_screen.song);
    nc_song_info_screen_init(&song_info_screen.screen,
                             native_song_info_hooks(),
                             0,
                             ui_state_screen_width(),
                             ui_state_main_start_y(),
                             ui_state_main_height(),
                             Config.main_color,
                             native_no_border(),
                             Config.lines_scrolled);
    song_info_screen.initialized = true;
    return;
}

void
native_c_screen_song_info_register(void) {
    native_c_screen_song_info_init();
    assert(native_register_screen(native_c_screen_song_info_native()));
    return;
}

void
native_c_screen_song_info_switch_to(void) {
    (void)nc_screen_switcher_switch_to(native_c_screen_song_info_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_song_info_native()));
    return;
}

NcScreen *
native_c_screen_song_info_native(void) {
    native_c_screen_song_info_init();
    return nc_song_info_screen_base(&song_info_screen.screen);
}

void
native_c_screen_server_info_init(void) {
    if (server_info_screen.initialized) {
        return;
    }

    ncm_mpd_string_list_init(&server_info_screen.url_handlers);
    ncm_mpd_string_list_init(&server_info_screen.tag_types);
    nc_server_info_screen_init(&server_info_screen.screen,
                               native_server_info_hooks(),
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
native_c_screen_server_info_register(void) {
    native_c_screen_server_info_init();
    assert(native_register_screen(native_c_screen_server_info_native()));
    return;
}

void
native_c_screen_server_info_switch_to(void) {
    NcScreen *screen;

    screen = native_c_screen_server_info_native();
    (void)nc_screen_switcher_switch_to(screen,
                                       nc_screen_has_to_be_resized(screen));
    return;
}

NcScreen *
native_c_screen_server_info_native(void) {
    native_c_screen_server_info_init();
    return nc_server_info_screen_base(&server_info_screen.screen);
}

void
native_c_screen_outputs_init(void) {
#if defined(ENABLE_OUTPUTS)
    NcBuffer prefix;
    NcBuffer suffix;

    if (outputs_screen.initialized) {
        return;
    }

    nc_outputs_screen_init(&outputs_screen.screen,
                           native_outputs_hooks(),
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
native_c_screen_outputs_register(void) {
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
    assert(native_register_screen(native_c_screen_outputs_native()));
#endif
    return;
}

void
native_c_screen_outputs_switch_to(void) {
#if defined(ENABLE_OUTPUTS)
    (void)nc_screen_switcher_switch_to(native_c_screen_outputs_native(),
                                       nc_screen_has_to_be_resized(
                                           native_c_screen_outputs_native()));
#endif
    return;
}

void
native_c_screen_outputs_toggle(void) {
#if defined(ENABLE_OUTPUTS)
    (void)nc_outputs_screen_toggle_current(&outputs_screen.screen);
#endif
    return;
}

void
native_c_screen_outputs_fetch_list(void) {
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
    nc_outputs_screen_fetch_list(&outputs_screen.screen);
#endif
    return;
}

void
native_c_screen_outputs_refresh_if_visible(void) {
#if defined(ENABLE_OUTPUTS)
    if (nc_screen_switcher_is_visible(native_c_screen_outputs_native())) {
        nc_screen_refresh_window(native_c_screen_outputs_native());
    }
#endif
    return;
}

NcScreen *
native_c_screen_outputs_native(void) {
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
    return nc_outputs_screen_base(&outputs_screen.screen);
#else
    return NULL;
#endif
}

void
native_c_screens_init_all(void) {
    native_c_screen_browser_init();
    native_c_screen_help_init();
    native_c_screen_lastfm_init();
    native_c_screen_lyrics_init();
    native_c_screen_media_library_init();
    native_c_screen_playlist_init();
    native_c_screen_playlist_editor_init();
    native_c_screen_search_engine_init();
    native_c_screen_selected_items_adder_init();
    native_c_screen_server_info_init();
    native_c_screen_song_info_init();
    native_c_screen_sort_playlist_dialog_init();
#if defined(HAVE_TAGLIB_H)
    native_c_screen_tag_editor_init();
    native_c_screen_tiny_tag_editor_init();
#endif
#if defined(ENABLE_VISUALIZER)
    native_c_screen_visualizer_init();
#endif
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_init();
#endif
    return;
}

void
native_c_screens_register_native_only(void) {
    native_c_screen_browser_register();
    native_c_screen_help_register();
    native_c_screen_lastfm_register();
    native_c_screen_media_library_register();
    native_c_screen_search_engine_register();
    native_c_screen_selected_items_adder_register();
    native_c_screen_song_info_register();
    native_c_screen_server_info_register();
#if defined(ENABLE_VISUALIZER)
    native_c_screen_visualizer_register();
#endif
#if defined(HAVE_TAGLIB_H)
    native_c_screen_tag_editor_register();
    native_c_screen_tiny_tag_editor_register();
#endif
#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_register();
#endif
    native_c_screen_playlist_register();
    native_c_screen_playlist_editor_register();
    return;
}

void
native_c_screens_request_registered_resize(void) {
    native_request_registered_resize(NC_SCREEN_TYPE_BROWSER);
    native_request_registered_resize(NC_SCREEN_TYPE_HELP);
    native_request_registered_resize(NC_SCREEN_TYPE_LASTFM);
    native_request_registered_resize(NC_SCREEN_TYPE_LYRICS);
    native_request_registered_resize(NC_SCREEN_TYPE_MEDIA_LIBRARY);
    native_request_registered_resize(NC_SCREEN_TYPE_PLAYLIST);
    native_request_registered_resize(NC_SCREEN_TYPE_PLAYLIST_EDITOR);
    native_request_registered_resize(NC_SCREEN_TYPE_SEARCH_ENGINE);
    native_request_registered_resize(NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER);
    native_request_registered_resize(NC_SCREEN_TYPE_SERVER_INFO);
    native_request_registered_resize(NC_SCREEN_TYPE_SONG_INFO);
    native_request_registered_resize(NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG);
#if defined(HAVE_TAGLIB_H)
    native_request_registered_resize(NC_SCREEN_TYPE_TAG_EDITOR);
    native_request_registered_resize(NC_SCREEN_TYPE_TINY_TAG_EDITOR);
#endif
#if defined(ENABLE_VISUALIZER)
    native_request_registered_resize(NC_SCREEN_TYPE_VISUALIZER);
#endif
#if defined(ENABLE_OUTPUTS)
    native_request_registered_resize(NC_SCREEN_TYPE_OUTPUTS);
#endif
    return;
}

NcScreen *
native_c_screens_find_type(enum ScreenType screen_type) {
    int32 type;

    type = screen_type_to_native_type(screen_type);
    if (type == NC_SCREEN_TYPE_UNKNOWN) {
        return NULL;
    }
    return app_controller_find_screen_type(type);
}

bool
native_c_screens_switch_to_type(enum ScreenType screen_type) {
    NcScreen *screen;

    screen = native_c_screens_find_type(screen_type);
    if (screen == NULL) {
        return false;
    }
    return nc_screen_switcher_switch_to(screen,
                                       nc_screen_has_to_be_resized(screen));
}

bool
native_c_screens_lock_current(void) {
    return app_controller_lock_current_screen();
}

enum ScreenType
native_c_screens_current_type(void) {
    NcScreen *screen;

    screen = app_controller_current_screen();
    if (screen == NULL) {
        return NCM_SCREEN_TYPE_UNKNOWN;
    }
    return native_screen_type_from_native_type(nc_screen_type(screen));
}

static enum ScreenType
native_screen_type_from_native_type(int32 type) {
    switch (type) {
    case NC_SCREEN_TYPE_BROWSER:
        return NCM_SCREEN_TYPE_BROWSER;
    case NC_SCREEN_TYPE_HELP:
        return NCM_SCREEN_TYPE_HELP;
    case NC_SCREEN_TYPE_LASTFM:
        return NCM_SCREEN_TYPE_LASTFM;
    case NC_SCREEN_TYPE_LYRICS:
        return NCM_SCREEN_TYPE_LYRICS;
    case NC_SCREEN_TYPE_MEDIA_LIBRARY:
        return NCM_SCREEN_TYPE_MEDIA_LIBRARY;
#if defined(ENABLE_OUTPUTS)
    case NC_SCREEN_TYPE_OUTPUTS:
        return NCM_SCREEN_TYPE_OUTPUTS;
#endif
    case NC_SCREEN_TYPE_PLAYLIST:
        return NCM_SCREEN_TYPE_PLAYLIST;
    case NC_SCREEN_TYPE_PLAYLIST_EDITOR:
        return NCM_SCREEN_TYPE_PLAYLIST_EDITOR;
    case NC_SCREEN_TYPE_SEARCH_ENGINE:
        return NCM_SCREEN_TYPE_SEARCH_ENGINE;
    case NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER:
        return NCM_SCREEN_TYPE_SELECTED_ITEMS_ADDER;
    case NC_SCREEN_TYPE_SERVER_INFO:
        return NCM_SCREEN_TYPE_SERVER_INFO;
    case NC_SCREEN_TYPE_SONG_INFO:
        return NCM_SCREEN_TYPE_SONG_INFO;
    case NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG:
        return NCM_SCREEN_TYPE_SORT_PLAYLIST_DIALOG;
#if defined(HAVE_TAGLIB_H)
    case NC_SCREEN_TYPE_TAG_EDITOR:
        return NCM_SCREEN_TYPE_TAG_EDITOR;
    case NC_SCREEN_TYPE_TINY_TAG_EDITOR:
        return NCM_SCREEN_TYPE_TINY_TAG_EDITOR;
#endif
#if defined(ENABLE_VISUALIZER)
    case NC_SCREEN_TYPE_VISUALIZER:
        return NCM_SCREEN_TYPE_VISUALIZER;
#endif
    case NC_SCREEN_TYPE_UNKNOWN:
        break;
    default:
        break;
    }
    return NCM_SCREEN_TYPE_UNKNOWN;
}

static void
native_request_registered_resize(int32 type) {
    NcScreen *screen;

    screen = app_controller_find_screen_type(type);
    if (screen != NULL) {
        nc_screen_request_resize(screen);
    }
    return;
}

static NcBorder
native_no_border(void) {
    NcBorder border = {0};

    return border;
}

static void
native_draw_screen_header(NcScreen *screen) {
    char *title;

    title = nc_screen_title(screen);
    ncm_title_draw_header(title, optional_strlen32(title));
    return;
}

static bool
native_register_screen(NcScreen *screen) {
    if (app_controller_is_screen_registered(screen)) {
        return true;
    }
    return app_controller_register_screen(screen);
}

static void
native_resize_main_area(NcScreen *base, int64 *x, int64 *width) {
    nc_screen_switcher_get_resize_params(base, x, width, true);
    return;
}

static void
native_append_cstring(NcBuffer *buffer, char *string) {
    nc_buffer_append_cstring(buffer, string);
    return;
}

static void
native_append_data(NcBuffer *buffer, char *string, int32 len) {
    if ((string != NULL) && (len > 0)) {
        nc_buffer_append_data(buffer, string, len);
    }
    return;
}

static void
native_append_format(NcBuffer *buffer, enum NcFormat format) {
    nc_buffer_add_format(buffer, nc_buffer_len(buffer), format, 0);
    return;
}

static void
native_append_formatted_color(NcBuffer *buffer,
                              NcFormattedColor *color) {
    nc_buffer_add_formatted_color(buffer, nc_buffer_len(buffer), color, 0);
    return;
}

static void
native_append_formatted_color_end(NcBuffer *buffer,
                                  NcFormattedColor *color) {
    nc_buffer_add_formatted_color_end(buffer, nc_buffer_len(buffer), color, 0);
    return;
}

static void
native_append_bold_label(NcBuffer *buffer, char *label) {
    native_append_format(buffer, NC_FORMAT_BOLD);
    native_append_cstring(buffer, label);
    native_append_format(buffer, NC_FORMAT_NO_BOLD);
    return;
}

static void
native_append_song_tag(NcBuffer *buffer, NcmBuffer *tag) {
    if ((tag == NULL) || (tag->len <= 0)) {
        native_append_formatted_color(buffer, &Config.empty_tags_color);
        native_append_data(buffer, Config.empty_tag, Config.empty_tag_len);
        native_append_formatted_color_end(buffer, &Config.empty_tags_color);
        return;
    }
    native_append_data(buffer, tag->data, tag->len);
    return;
}

static void
native_append_song_key_value(NcBuffer *buffer, char *key,
                             NcmBuffer *value,
                             bool empty_as_missing) {
    native_append_format(buffer, NC_FORMAT_BOLD);
    native_append_formatted_color(buffer, &Config.color1);
    native_append_cstring(buffer, key);
    native_append_cstring(buffer, ":");
    native_append_formatted_color_end(buffer, &Config.color1);
    native_append_format(buffer, NC_FORMAT_NO_BOLD);
    native_append_cstring(buffer, " ");
    native_append_formatted_color(buffer, &Config.color2);
    if (empty_as_missing) {
        native_append_song_tag(buffer, value);
    } else if (value != NULL) {
        native_append_data(buffer, value->data, value->len);
    }
    native_append_formatted_color_end(buffer, &Config.color2);
    native_append_cstring(buffer, "\n");
    return;
}

static int32
native_key_name(NcKey key, char *buffer, int32 buffer_cap) {
    int32 len;

    len = ncm_bindings_key_name(key, buffer, buffer_cap);
    if (len < 0) {
        return 0;
    }
    return len;
}

static void
native_append_action_keys(NcBuffer *buffer, enum NcmActionType type) {
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
            key_len = native_key_name(key_bindings->key, key_name,
                                      (int32)sizeof(key_name));
            if (key_len <= 0) {
                continue;
            }
            if (width > 0) {
                native_append_cstring(buffer, " ");
                width += 1;
            }
            native_append_data(buffer, key_name, key_len);
            width += key_len;
        }
    }
    while ((nc_buffer_len(buffer) - column_start) < 20) {
        nc_buffer_append_char(buffer, ' ');
    }
    return;
}

static void
native_append_help_line(NcBuffer *buffer, enum NcmActionType type,
                        char *description) {
    native_append_cstring(buffer, "    ");
    native_append_action_keys(buffer, type);
    native_append_cstring(buffer, " : ");
    native_append_cstring(buffer, description);
    native_append_cstring(buffer, "\n");
    return;
}

static bool
native_help_render(void *user, NcBuffer *buffer) {
    (void)user;

    native_append_format(buffer, NC_FORMAT_BOLD);
    native_append_cstring(buffer, "\n  Keys - Movement\n\n");
    native_append_format(buffer, NC_FORMAT_NO_BOLD);
    native_append_help_line(buffer, NCM_ACTION_SCROLL_UP, "Move cursor up");
    native_append_help_line(buffer, NCM_ACTION_SCROLL_DOWN,
                            "Move cursor down");
    native_append_help_line(buffer, NCM_ACTION_PAGE_UP, "Page up");
    native_append_help_line(buffer, NCM_ACTION_PAGE_DOWN, "Page down");
    native_append_help_line(buffer, NCM_ACTION_MOVE_HOME, "Home");
    native_append_help_line(buffer, NCM_ACTION_MOVE_END, "End");
    native_append_help_line(buffer, NCM_ACTION_NEXT_SCREEN, "Next screen");
    native_append_help_line(buffer, NCM_ACTION_PREVIOUS_SCREEN,
                            "Previous screen");
    native_append_help_line(buffer, NCM_ACTION_SHOW_HELP, "Show help");
    native_append_help_line(buffer, NCM_ACTION_SHOW_PLAYLIST,
                            "Show playlist");
    native_append_help_line(buffer, NCM_ACTION_SHOW_BROWSER, "Show browser");
    native_append_help_line(buffer, NCM_ACTION_SHOW_SEARCH_ENGINE,
                            "Show search engine");
    native_append_help_line(buffer, NCM_ACTION_SHOW_MEDIA_LIBRARY,
                            "Show media library");
    native_append_help_line(buffer, NCM_ACTION_SHOW_PLAYLIST_EDITOR,
                            "Show playlist editor");
    native_append_help_line(buffer, NCM_ACTION_SHOW_SERVER_INFO,
                            "Show server info");
#if defined(ENABLE_OUTPUTS)
    native_append_help_line(buffer, NCM_ACTION_SHOW_OUTPUTS,
                            "Show outputs");
#endif
#if defined(ENABLE_VISUALIZER)
    native_append_help_line(buffer, NCM_ACTION_SHOW_VISUALIZER,
                            "Show music visualizer");
#endif
#if defined(HAVE_TAGLIB_H)
    native_append_help_line(buffer, NCM_ACTION_SHOW_TAG_EDITOR,
                            "Show tag editor");
#endif

    native_append_format(buffer, NC_FORMAT_BOLD);
    native_append_cstring(buffer, "\n  Keys - Global\n\n");
    native_append_format(buffer, NC_FORMAT_NO_BOLD);
    native_append_help_line(buffer, NCM_ACTION_PLAY, "Play");
    native_append_help_line(buffer, NCM_ACTION_STOP, "Stop");
    native_append_help_line(buffer, NCM_ACTION_PAUSE, "Pause");
    native_append_help_line(buffer, NCM_ACTION_NEXT, "Next track");
    native_append_help_line(buffer, NCM_ACTION_PREVIOUS, "Previous track");
    native_append_help_line(buffer, NCM_ACTION_VOLUME_DOWN,
                            "Decrease volume");
    native_append_help_line(buffer, NCM_ACTION_VOLUME_UP,
                            "Increase volume");
    native_append_help_line(buffer, NCM_ACTION_TOGGLE_REPEAT,
                            "Toggle repeat mode");
    native_append_help_line(buffer, NCM_ACTION_TOGGLE_RANDOM,
                            "Toggle random mode");
    native_append_help_line(buffer, NCM_ACTION_TOGGLE_SINGLE,
                            "Toggle single mode");
    native_append_help_line(buffer, NCM_ACTION_TOGGLE_CONSUME,
                            "Toggle consume mode");
    native_append_help_line(buffer, NCM_ACTION_UPDATE_DATABASE,
                            "Start music database update");
    native_append_help_line(buffer, NCM_ACTION_EXECUTE_COMMAND,
                            "Execute command");
    native_append_help_line(buffer, NCM_ACTION_QUIT, "Quit");
    return true;
}

static void
native_help_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(native_c_screen_help_native());
    native_draw_screen_header(native_c_screen_help_native());
    return;
}

static void
native_help_resize(void *user, NcHelpScreen *screen) {
    int64 x;
    int64 width;

    (void)user;
    native_resize_main_area(nc_help_screen_base(screen), &x, &width);
    nc_help_screen_set_geometry(screen,
                                x,
                                width,
                                ui_state_main_start_y(),
                                ui_state_main_height());
    return;
}

static void
native_help_destroy(void *user) {
    NativeHelpScreen *owner;

    owner = user;
    owner->initialized = false;
    return;
}

static NcHelpHooks
native_help_hooks(void) {
    NcHelpHooks hooks = {0};

    hooks.render = native_help_render;
    hooks.switch_to = native_help_switch_to;
    hooks.resize_layout = native_help_resize;
    hooks.destroy = native_help_destroy;
    hooks.user = &help_screen;
    return hooks;
}

static void
native_outputs_fetch(void *user, NcOutputsScreen *screen) {
#if defined(ENABLE_OUTPUTS)
    NcmMpdOutputList outputs;
    NcmError error;

    (void)user;
    ncm_error_clear(&error);
    ncm_mpd_output_list_init(&outputs);
    if (!ncm_mpd_client_get_outputs(&global_mpd, &outputs, &error)) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(5,
                             STRLIT_ARGS("Could not fetch outputs: %1"),
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
native_outputs_toggle(void *user, uint32 id, bool enabled,
                      char *name, int32 name_len) {
#if defined(ENABLE_OUTPUTS)
    NcmError error;
    bool ok;

    (void)user;
    ncm_error_clear(&error);
    if (enabled) {
        ok = ncm_mpd_client_disable_output(&global_mpd, id, &error);
    } else {
        ok = ncm_mpd_client_enable_output(&global_mpd, id, &error);
    }
    if (!ok) {
        NcmStringFormatArg args[2];

        args[0] = ncm_string_format_arg_string(name, name_len);
        args[1] = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(5,
                             STRLIT_ARGS("Could not toggle output %1: %2"),
                             args,
                             2);
        return false;
    }

    if (enabled) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_string(name, name_len);
        ncm_statusbar_format(3,
                             STRLIT_ARGS("Output %1 disabled"),
                             &arg,
                             1);
    } else {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_string(name, name_len);
        ncm_statusbar_format(3,
                             STRLIT_ARGS("Output %1 enabled"),
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
native_outputs_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(native_c_screen_outputs_native());
    native_draw_screen_header(native_c_screen_outputs_native());
    return;
}

static void
native_outputs_resize(void *user, NcOutputsScreen *screen) {
    int64 x;
    int64 width;

    (void)user;
    native_resize_main_area(nc_outputs_screen_base(screen), &x, &width);
    nc_outputs_screen_set_geometry(screen,
                                   x,
                                   width,
                                   ui_state_main_start_y(),
                                   ui_state_main_height());
    return;
}

static void
native_outputs_destroy(void *user) {
#if defined(ENABLE_OUTPUTS)
    NativeOutputsScreen *owner;

    owner = user;
    owner->initialized = false;
#else
    (void)user;
#endif
    return;
}

static NcOutputsHooks
native_outputs_hooks(void) {
    NcOutputsHooks hooks = {0};

    hooks.fetch_outputs = native_outputs_fetch;
    hooks.toggle_output = native_outputs_toggle;
    hooks.switch_to = native_outputs_switch_to;
    hooks.resize_layout = native_outputs_resize;
    hooks.destroy = native_outputs_destroy;
    hooks.user = &outputs_screen;
    return hooks;
}

static void
native_server_info_load_lists(void *user) {
    NativeServerInfoScreen *owner;
    NcmError error;

    owner = user;
    ncm_error_clear(&error);
    (void)ncm_mpd_client_get_url_handlers(&global_mpd,
                                           &owner->url_handlers,
                                           &error);
    ncm_error_clear(&error);
    (void)ncm_mpd_client_get_tag_types(&global_mpd,
                                        &owner->tag_types,
                                        &error);
    return;
}

static bool
native_server_info_render(void *user, NcBuffer *buffer) {
    NativeServerInfoScreen *owner;
    NcmMpdStats stats;
    NcmError error;
    char time_buffer[64];

    owner = user;
    if (global_timer_elapsed_ms(owner->timer) < 1000) {
        return false;
    }
    owner->timer = global_timer;

    ncm_error_clear(&error);
    if (!ncm_mpd_client_get_stats(&global_mpd, &stats, &error)) {
        return false;
    }

    native_append_bold_label(buffer, "Version: ");
    native_append_cstring(buffer, "0.");
    nc_buffer_append_uint32(buffer, ncm_mpd_client_version(&global_mpd));
    native_append_cstring(buffer, ".*\n");

    native_append_bold_label(buffer, "Uptime: ");
    native_show_long_time(buffer, stats.uptime);
    native_append_cstring(buffer, "\n");

    native_append_bold_label(buffer, "Time playing: ");
    ncm_song_show_time((uint32)stats.play_time,
                       time_buffer,
                       (int32)sizeof(time_buffer));
    native_append_cstring(buffer, time_buffer);
    native_append_cstring(buffer, "\n\n");

    native_append_bold_label(buffer, "Total playtime: ");
    native_show_long_time(buffer, stats.db_play_time);
    native_append_cstring(buffer, "\n");

    native_append_bold_label(buffer, "Artist names: ");
    nc_buffer_append_uint32(buffer, stats.artists);
    native_append_cstring(buffer, "\n");

    native_append_bold_label(buffer, "Album names: ");
    nc_buffer_append_uint32(buffer, stats.albums);
    native_append_cstring(buffer, "\n");

    native_append_bold_label(buffer, "Songs in database: ");
    nc_buffer_append_uint32(buffer, stats.songs);
    native_append_cstring(buffer, "\n\n");

    native_append_bold_label(buffer, "URL Handlers:");
    for (int32 i = 0; i < owner->url_handlers.count; i += 1) {
        NcmMpdString *handler;

        handler = owner->url_handlers.items + i;
        native_append_cstring(buffer, i == 0 ? " " : ", ");
        native_append_data(buffer, handler->value, handler->value_len);
    }
    native_append_cstring(buffer, "\n\n");

    native_append_bold_label(buffer, "Tag Types:");
    for (int32 i = 0; i < owner->tag_types.count; i += 1) {
        NcmMpdString *tag;

        tag = owner->tag_types.items + i;
        native_append_cstring(buffer, i == 0 ? " " : ", ");
        native_append_data(buffer, tag->value, tag->value_len);
    }
    return true;
}

static void
native_server_info_switch_to(void *user) {
    (void)user;
    (void)nc_screen_switcher_finish_switch(
        native_c_screen_server_info_native());
    native_draw_screen_header(native_c_screen_server_info_native());
    return;
}

static void
native_server_info_resize(void *user) {
    (void)user;
    nc_server_info_screen_set_dimensions(&server_info_screen.screen,
                                         ui_state_screen_width(),
                                         ui_state_screen_height(),
                                         ui_state_main_start_y(),
                                         ui_state_main_height());
    return;
}

static char *
native_server_info_title(void *user) {
    (void)user;
    return "Server info";
}

static void
native_server_info_destroy(void *user) {
    NativeServerInfoScreen *owner;

    owner = user;
    ncm_mpd_string_list_destroy(&owner->url_handlers);
    ncm_mpd_string_list_destroy(&owner->tag_types);
    owner->initialized = false;
    return;
}

static NcServerInfoHooks
native_server_info_hooks(void) {
    NcServerInfoHooks hooks = {0};

    hooks.load_lists = native_server_info_load_lists;
    hooks.render = native_server_info_render;
    hooks.switch_to = native_server_info_switch_to;
    hooks.resize_layout = native_server_info_resize;
    hooks.title = native_server_info_title;
    hooks.destroy = native_server_info_destroy;
    hooks.user = &server_info_screen;
    return hooks;
}

static bool
native_song_info_render(void *user, NcSongInfoScreen *screen,
                        NcBuffer *buffer) {
    NativeSongInfoScreen *owner;
    NcmBuffer value;

    (void)screen;
    owner = user;
    if (!owner->has_song) {
        return false;
    }

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_NAME,
                                   0);
    native_append_song_key_value(buffer, "Filename", &value, false);
    ncm_buffer_destroy(&value);

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_DIRECTORY,
                                   0);
    native_append_song_key_value(buffer, "Directory", &value, true);
    ncm_buffer_destroy(&value);
    native_append_cstring(buffer, "\n");

    value = ncm_song_getter_buffer(&owner->song,
                                   NCM_SONG_GETTER_LENGTH,
                                   0);
    native_append_song_key_value(buffer, "Length", &value, false);
    ncm_buffer_destroy(&value);

    for (int32 i = 0; ncm_song_info_tags[i].name != NULL; i += 1) {
        native_append_format(buffer, NC_FORMAT_BOLD);
        native_append_cstring(buffer, "\n");
        native_append_cstring(buffer, ncm_song_info_tags[i].name);
        native_append_cstring(buffer, ":");
        native_append_format(buffer, NC_FORMAT_NO_BOLD);
        native_append_cstring(buffer, " ");
        value = ncm_song_tags_buffer(&owner->song,
                                     ncm_song_info_tags[i].get,
                                     Config.tags_separator,
                                     Config.tags_separator_len,
                                     Config.show_duplicate_tags);
        native_append_song_tag(buffer, &value);
        ncm_buffer_destroy(&value);
    }
    return true;
}

static void
native_song_info_switch_to(void *user, NcSongInfoScreen *screen) {
    NativeSongInfoScreen *owner;
    NcmError error;

    owner = user;
    ncm_error_clear(&error);
    ncm_song_destroy(&owner->song);
    ncm_song_init(&owner->song);
    owner->has_song = ncm_mpd_client_get_current_song(&global_mpd,
                                                      &owner->song,
                                                      &error);
    if (!owner->has_song) {
        NcmStringFormatArg arg;

        arg = ncm_string_format_arg_cstring(error.message);
        ncm_statusbar_format(5,
                             STRLIT_ARGS("Could not fetch current song: %1"),
                             &arg,
                             1);
        return;
    }

    (void)nc_screen_switcher_finish_switch(native_c_screen_song_info_native());
    (void)nc_song_info_screen_prepare_current(screen);
    native_draw_screen_header(native_c_screen_song_info_native());
    return;
}

static void
native_song_info_resize(void *user, NcSongInfoScreen *screen) {
    int64 x;
    int64 width;

    (void)user;
    native_resize_main_area(nc_song_info_screen_base(screen), &x, &width);
    nc_song_info_screen_set_geometry(screen,
                                     x,
                                     width,
                                     ui_state_main_start_y(),
                                     ui_state_main_height());
    return;
}

static void
native_song_info_destroy(void *user) {
    NativeSongInfoScreen *owner;

    owner = user;
    ncm_song_destroy(&owner->song);
    owner->has_song = false;
    owner->initialized = false;
    return;
}

static NcSongInfoHooks
native_song_info_hooks(void) {
    NcSongInfoHooks hooks = {0};

    hooks.render = native_song_info_render;
    hooks.switch_to = native_song_info_switch_to;
    hooks.resize_layout = native_song_info_resize;
    hooks.destroy = native_song_info_destroy;
    hooks.user = &song_info_screen;
    return hooks;
}

static void
native_show_long_time(NcBuffer *buffer, uint64 seconds) {
    uint64 days;
    uint64 hours;
    uint64 minutes;

    days = seconds / 86400;
    seconds -= days*86400;
    hours = seconds / 3600;
    seconds -= hours*3600;
    minutes = seconds / 60;
    seconds -= minutes*60;

    if (days > 0) {
        nc_buffer_append_uint64(buffer, days);
        native_append_cstring(buffer, "d ");
    }
    if ((days > 0) || (hours > 0)) {
        nc_buffer_append_uint64(buffer, hours);
        native_append_cstring(buffer, "h ");
    }
    if ((days > 0) || (hours > 0) || (minutes > 0)) {
        nc_buffer_append_uint64(buffer, minutes);
        native_append_cstring(buffer, "m ");
    }
    nc_buffer_append_uint64(buffer, seconds);
    native_append_cstring(buffer, "s");
    return;
}
