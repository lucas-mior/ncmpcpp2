#if !defined(NCMPCPP_MAIN_C)
#define NCMPCPP_MAIN_C

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "app_legacy_bridge.h"
#include "bindings.h"
#include "configuration.h"
#include "global.h"
#include "settings.h"
#include "title.h"
#include "ui_state.h"

#include "c/ncm_base.h"
#include "cbase/base_macros.h"

/* Single translation unit source inclusions. */
#include "actions.c"
#include "app_controller.c"
#include "app_legacy_bridge.c"
#include "app_state.c"
#include "bindings.c"
#include "c/ncm_app_arrays.c"
#include "c/ncm_base.c"
#include "c/ncm_charset.c"
#include "c/ncm_comparators.c"
#include "c/ncm_conversion.c"
#include "c/ncm_directory.c"
#include "c/ncm_display.c"
#include "c/ncm_enums.c"
#include "c/ncm_error.c"
#include "c/ncm_format.c"
#include "c/ncm_fs.c"
#include "c/ncm_html.c"
#include "c/ncm_job.c"
#include "c/ncm_macro_utilities.c"
#include "c/ncm_mpd_client.c"
#include "c/ncm_mpd_connection.c"
#include "c/ncm_mpd_item.c"
#include "c/ncm_mutable_song.c"
#include "c/ncm_option_parser.c"
#include "c/ncm_path.c"
#include "c/ncm_playlist.c"
#include "c/ncm_playlist_sort.c"
#include "c/ncm_process.c"
#include "c/ncm_random.c"
#include "c/ncm_regex.c"
#include "c/ncm_search_prompt.c"
#include "c/ncm_sample_buffer.c"
#include "c/ncm_song.c"
#include "c/ncm_string.c"
#include "c/ncm_string_format.c"
#include "c/ncm_taglib.c"
#include "c/ncm_tags.c"
#include "c/ncm_time.c"
#include "c/ncm_type_conversions.c"
#include "configuration.c"
#include "curl_handle.c"
#include "curses/nc_app_menus.c"
#include "curses/nc_buffer.c"
#include "curses/nc_cyclic_buffer.c"
#include "curses/nc_formatted_color.c"
#include "curses/nc_menu.c"
#include "curses/nc_scrollpad.c"
#include "curses/nc_window.c"
#include "global.c"
#include "helpers.c"
#include "lastfm_service.c"
#include "lyrics_fetcher.c"
#include "screen_actions.c"
#include "screens/native_c_screens.c"
#include "screens/nc_browser.c"
#include "screens/nc_help.c"
#include "screens/nc_lastfm.c"
#include "screens/nc_lyrics.c"
#include "screens/nc_media_library.c"
#include "screens/nc_outputs.c"
#include "screens/nc_playlist.c"
#include "screens/nc_playlist_editor.c"
#include "screens/nc_screen.c"
#include "screens/nc_screen_switcher.c"
#include "screens/nc_scrollpad_screen.c"
#include "screens/nc_search_engine.c"
#include "screens/nc_sel_items_adder.c"
#include "screens/nc_server_info.c"
#include "screens/nc_song_info.c"
#include "screens/nc_sort_playlist.c"
#include "screens/nc_tag_editor.c"
#include "screens/nc_tiny_tag_editor.c"
#include "screens/nc_visualizer.c"
#include "screens/screen_type.c"
#include "settings.c"
#include "settings_types.c"
#include "status.c"
#include "statusbar.c"
#include "title.c"
#include "ui_state.c"

static volatile sig_atomic_t app_resize_requested;
static int32 app_saved_stderr_fd = -1;
static FILE *app_error_log;
static NcWindow *app_header_window;
static NcWindow *app_footer_window;

static void
app_signal_handler(int32 signal_number) {
    if (signal_number == SIGWINCH) {
        app_resize_requested = 1;
    }
#if defined(__sun) && defined(__SVR4)
    signal(signal_number, app_signal_handler);
#endif
    return;
}

static void
app_init_state(void) {
    global_state_init();
    configuration_init(&Config);
    ncm_bindings_configuration_init(&Bindings);
    return;
}

static void
app_destroy_state(void) {
    ncm_bindings_configuration_destroy(&Bindings);
    configuration_destroy(&Config);
    global_state_destroy();
    return;
}

static bool
app_redirect_stderr(void) {
    NcmBuffer path;

    ncm_buffer_init(&path);
    ncm_buffer_append(&path, Config.ncmpcpp_directory,
                      Config.ncmpcpp_directory_len);
    ncm_buffer_append(&path, STRLIT_ARGS("error.log"));

    app_saved_stderr_fd = (int32)dup(STDERR_FILENO);
    if (app_saved_stderr_fd < 0) {
        ncm_buffer_destroy(&path);
        return false;
    }

    app_error_log = freopen(path.data, "a", stderr);
    ncm_buffer_destroy(&path);
    return app_error_log;
}

static void
app_restore_stderr(void) {
    if (app_saved_stderr_fd >= 0) {
        fflush(stderr);
        dup2(app_saved_stderr_fd, STDERR_FILENO);
        close(app_saved_stderr_fd);
        app_saved_stderr_fd = -1;
    }
    return;
}

static void
app_at_exit(void) {
    ncm_mpd_client_disconnect(&global_mpd);
    ncm_window_title_set_cstring("");

    if (app_header_window) {
        ncmpcpp_legacy_window_destroy(app_header_window);
        app_header_window = NULL;
        ui_state_set_header_window(NULL);
    }
    if (app_footer_window) {
        ncmpcpp_legacy_window_destroy(app_footer_window);
        app_footer_window = NULL;
        ui_state_set_footer_window(NULL);
    }
    ncmpcpp_legacy_destroy_screen();

    app_restore_stderr();
    app_destroy_state();
    return;
}

static void
app_create_windows(void) {
    ncmpcpp_legacy_set_statusbar_visibility_baseline(
        Config.statusbar_visibility);

    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        Config.statusbar_visibility = false;
    }

    ncmpcpp_legacy_set_windows_dimensions();
    ncmpcpp_legacy_initialize_screens();

    app_header_window = ncmpcpp_legacy_window_create(
        0, 0, COLS, ncmpcpp_legacy_header_height(), Config.header_color);
    ui_state_set_header_window(app_header_window);
    if (Config.header_visibility || (Config.design == NCM_DESIGN_ALTERNATIVE)) {
        ncmpcpp_legacy_window_display(app_header_window);
    }

    app_footer_window = ncmpcpp_legacy_window_create(
        0, ncmpcpp_legacy_footer_start_y(), COLS,
        ncmpcpp_legacy_footer_height(), Config.statusbar_color);
    ui_state_set_footer_window(app_footer_window);
    return;
}

static void
app_apply_startup_screen(void) {
    ncmpcpp_legacy_playlist_switch_to();

    if (Config.startup_screen_type != ncmpcpp_legacy_current_screen_type()) {
        assert(
            ncmpcpp_legacy_switch_to_screen_type(Config.startup_screen_type));
    }

    if (Config.has_startup_slave_screen_type) {
        bool screen_locked;
        enum ScreenType slave_screen_type;

        slave_screen_type = Config.startup_slave_screen_type;
        screen_locked = ncmpcpp_legacy_lock_current_screen();
        if (screen_locked
            && (slave_screen_type != ncmpcpp_legacy_current_screen_type())) {
            assert(ncmpcpp_legacy_switch_to_screen_type(slave_screen_type));
            if (!Config.startup_slave_screen_focus) {
                ncmpcpp_legacy_execute_action(NCM_ACTION_MASTER_SCREEN);
            }
        }
    }
    return;
}

static void
app_connect_if_due(NcmTimePoint *connect_attempt) {
    if (!ncmpcpp_legacy_mpd_connected()
        && (global_timer_elapsed_ms(*connect_attempt) > 1000)) {
        *connect_attempt = global_timer;
        ncmpcpp_legacy_status_clear();
        nc_window_clear_fd_callbacks(app_footer_window);
        ncmpcpp_legacy_connect_or_report();
    }
    return;
}

static void
app_execute_key(NcKey input) {
    NcmBindingSlice bindings;
    bool executed;

    bindings = ncm_bindings_configuration_get(&Bindings, input);
    executed = false;
    for (int32 i = 0; i < bindings.len; i += 1) {
        if (ncmpcpp_legacy_execute_binding(bindings.data + i)) {
            executed = true;
            break;
        }
    }

    (void)executed;
    return;
}

static bool
app_exit_requested(void) {
    return ncmpcpp_legacy_exit_requested()
           || ncm_action_runtime_exit_requested(NULL);
}

int
main(int32 argc, char **argv) {
    bool key_pressed;
    NcKey input;
    NcmTimePoint connect_attempt;

    app_init_state();
    setlocale(LC_ALL, "");

    if (!configure(argc, argv)) {
        app_destroy_state();
        exit(EXIT_SUCCESS);
    }
    atexit(app_at_exit);
    if (!app_redirect_stderr()) {
        fprintf(stderr, "warning: could not redirect stderr: %s\n",
                strerror(errno));
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGWINCH, app_signal_handler);

    ncmpcpp_legacy_set_noidle_status_callback();
    ncmpcpp_legacy_init_screen(Config.colors_enabled, Config.mouse_support);
    app_create_windows();

    global_timer_update(NULL);
    ncm_random_seed_from_time(&global_random, NULL);
    app_apply_startup_screen();

    key_pressed = false;
    connect_attempt.ns = 0;

    while (!app_exit_requested()) {
        app_connect_if_due(&connect_attempt);

        if (app_resize_requested) {
            ncmpcpp_legacy_resize_screen(true);
            app_resize_requested = 0;
        }

        ncmpcpp_legacy_update_environment(!key_pressed, key_pressed, false);

        input = ncm_read_key(app_footer_window);
        key_pressed = input != NC_KEY_NONE;
        if (!key_pressed) {
            continue;
        }

        global_timer_update(NULL);
        app_execute_key(input);
        ncmpcpp_legacy_playlist_enable_highlighting_if_current();
    }

    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_MAIN_C */
