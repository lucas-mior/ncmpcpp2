#define CBASE_IMPLEMENT
#include "cbase.h"

#include "c/ncm_c.h"
#if !defined(PROJECT_INCREMENTAL_BUILD)
#include "c/ncm_c.c"
#endif

#include "curses/nc_curses.h"
#if !defined(PROJECT_INCREMENTAL_BUILD)
#include "curses/nc_curses.c"
#endif

#include "screens/nc_screens.h"
#if !defined(PROJECT_INCREMENTAL_BUILD)
#include "screens/nc_screens.c"
#endif

#include "app_legacy_bridge.h"
#include "bindings.h"
#include "configuration.h"
#include "global.h"
#include "settings.h"
#include "title.h"
#include "ui_state.h"

#include "actions.c"
#include "app_controller.c"
#include "app_legacy_bridge.c"
#include "app_state.c"
#include "bindings.c"

#include "configuration.c"
#include "curl_handle.c"

#include "global.c"
#include "helpers.c"
#include "lastfm_service.c"
#include "lyrics_fetcher.c"
#include "screen_actions.c"

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
    Bindings = (NcmBindingsConfiguration){0};
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
    StrBuilder path = {0};

    SB_APPEND(&path, Config.ncmpcpp_directory, Config.ncmpcpp_directory_len);
    SB_APPEND(&path, "error.log");

    if ((app_saved_stderr_fd = dup(STDERR_FILENO)) < 0) {
        sb_free(&path);
        return false;
    }

    app_error_log = freopen(path.data, "a", stderr);
    sb_free(&path);
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
        ncmpcpp_window_destroy(app_header_window);
        app_header_window = NULL;
        ui_state_set_header_window(NULL);
    }
    if (app_footer_window) {
        ncmpcpp_window_destroy(app_footer_window);
        app_footer_window = NULL;
        ui_state_set_footer_window(NULL);
    }
    ncmpcpp_destroy_screen();

    app_restore_stderr();
    app_destroy_state();
    return;
}

static void
app_create_windows(void) {
    ncmpcpp_set_statusbar_visibility_baseline(
        Config.statusbar_visibility);

    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        Config.statusbar_visibility = false;
    }

    ncmpcpp_set_windows_dimensions();
    ncmpcpp_init_screens();

    app_header_window = ncmpcpp_window_create(0, 0,
                                              COLS, ncmpcpp_header_height(),
                                              Config.header_color);
    ui_state_set_header_window(app_header_window);
    if (Config.header_visibility || (Config.design == NCM_DESIGN_ALTERNATIVE)) {
        ncmpcpp_window_display(app_header_window);
    }

    app_footer_window = ncmpcpp_window_create(0, ncmpcpp_footer_start_y(),
                                              COLS, ncmpcpp_footer_height(),
                                              Config.statusbar_color);
    ui_state_set_footer_window(app_footer_window);
    return;
}

static void
app_apply_startup_screen(void) {
    ncmpcpp_playlist_switch_to();

    if (Config.startup_screen_type != ncmpcpp_current_screen_type()) {
        ASSERT_ZERO(ncmpcpp_switch_to_screen_type(Config.startup_screen_type));
    }

    if (Config.has_startup_slave_screen_type) {
        int32 status = ncmpcpp_lock_current_screen();
        enum ScreenType slave_screen_type = Config.startup_slave_screen_type;

        if ((status == 0)
            && (slave_screen_type != ncmpcpp_current_screen_type())) {
            ASSERT_ZERO(ncmpcpp_switch_to_screen_type(slave_screen_type));
            if (!Config.startup_slave_screen_focus) {
                (void)ncmpcpp_execute_action(NCM_ACTION_MASTER_SCREEN);
            }
        }
    }
    return;
}

static void
app_connect_if_due(NcmTimePoint *connect_attempt) {
    if (!ncmpcpp_mpd_is_connected()
        && (global_timer_elapsed_ms(*connect_attempt) > 1000)) {
        *connect_attempt = global_timer;
        ncmpcpp_status_clear();
        nc_window_clear_fd_callbacks(app_footer_window);
        ncmpcpp_connect_or_report();
    }
    return;
}

static void
app_execute_key(NcKey input) {
    NcmBindingSlice bindings;
    bool executed = false;

    if (ncm_bindings_configuration_get(&Bindings, input, &bindings) <= 0) {
        return;
    }

    for (int32 i = 0; i < bindings.len; i += 1) {
        if (ncmpcpp_execute_binding(bindings.data + i) == 0) {
            executed = true;
            break;
        }
    }

    (void)executed;
    return;
}

static bool
app_exit_requested(void) {
    return ncmpcpp_has_exit_request()
           || ncm_action_runtime_exit_requested(NULL);
}

int
main(int32 argc, char **argv) {
    bool key_pressed;
    NcmTimePoint connect_attempt;

    program = argv[0];

    app_init_state();
    setlocale(LC_ALL, "");

    if (configure(argc, argv) <= 0) {
        app_destroy_state();
        exit(EXIT_SUCCESS);
    }
#if CC_GCC || CC_CLANG
    atexit(app_at_exit);
#endif
    if (!app_redirect_stderr()) {
        error2("warning: could not redirect stderr: %s\n", strerror(errno));
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGWINCH, app_signal_handler);

    ncmpcpp_set_noidle_status_callback();
    ncmpcpp_init_screen(Config.colors_enabled, Config.mouse_support);
    app_create_windows();

    (void)global_timer_update(NULL);
    (void)ncm_random_seed_from_time(&global_random, NULL);
    app_apply_startup_screen();

    key_pressed = false;
    connect_attempt.ns = 0;

    while (!app_exit_requested()) {
        NcKey input;

        app_connect_if_due(&connect_attempt);

        if (app_resize_requested) {
            ncmpcpp_resize_screen(true);
            app_resize_requested = 0;
        }

        (void)ncmpcpp_update_environment(!key_pressed, key_pressed, false);

        input = ncm_read_key(app_footer_window);
        key_pressed = input != NC_KEY_NONE;
        if (!key_pressed) {
            continue;
        }

        (void)global_timer_update(NULL);
        app_execute_key(input);
        ncmpcpp_playlist_enable_highlighting_if_current();
    }

    exit(EXIT_SUCCESS);
}
