#if !defined(NCMPCPP_STATUS_C)
#define NCMPCPP_STATUS_C

#include "status.h"

#include "actions.h"
#include "config.h"

#include <mpd/client.h>
#include <mpd/status.h>

#if defined(HAVE_NETINET_IN_H) && defined(HAVE_NETINET_TCP_H)
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "global.h"
#include "settings.h"
#include "statusbar.h"
#include "app_controller.h"
#include "c/ncm_macro_utilities.h"
#include "c/ncm_format.h"
#include "cbase/utf8.c"
#include "curses/nc_buffer.h"
#include "curses/nc_cyclic_buffer.h"
#include "helpers.h"
#include "screens/native_c_screens.h"
#include "title.h"
#include "ui_state.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool status_initialized;
static char status_consume;
static char status_crossfade;
static char status_db_updating;
static char status_repeat;
static char status_random;
static char status_single;
static int32 status_current_song_id;
static int32 status_current_song_pos;
static uint32 status_elapsed_time;
static uint32 status_kbps;
static enum NcmStatusPlayerState status_player_state;
static uint32 status_playlist_version;
static uint32 status_playlist_length;
static uint32 status_total_time;
static int32 status_volume;

static bool status_hooks_set;
static NcmStatusHooks status_hooks;
static bool status_ui_hooks_set;
static NcmStatusUiHooks status_ui_hooks;
static bool status_init_hooks_set;
static NcmStatusInitHooks status_init_hooks;
static void (*status_notification_observer)(void *user);
static void *status_notification_observer_user;
static void (*status_database_update_observer)(void *user);
static void *status_database_update_observer_user;
static void (*status_playlist_update_observer)(void *user);
static void *status_playlist_update_observer_user;
static NcmTimePoint status_past;
static int64 status_playing_song_scroll_begin;
static int64 status_first_line_scroll_begin;
static int64 status_second_line_scroll_begin;

typedef struct StatusTimeoutContext {
    int32 timeout;
} StatusTimeoutContext;

static void status_run_init_jump_to_now_playing(NcmStatusInitHooks *hooks);
static void status_run_init_set_tcp_nodelay(NcmStatusInitHooks *hooks);
static void
status_run_init_load_browser_supported_extensions(NcmStatusInitHooks *hooks);
static void status_run_init_fetch_outputs(NcmStatusInitHooks *hooks);
static void
status_run_init_setup_visualizer_datasource(NcmStatusInitHooks *hooks);
static void status_run_init_register_mpd_fd_callback(NcmStatusInitHooks *hooks);
static void status_run_init_show_connected_message(NcmStatusInitHooks *hooks);
static int32 status_player_state_string(char *buffer, int32 buffer_cap);
static void status_draw_song_title(NcmSong *song);
static void status_draw_player_state_label(char *state, int32 state_len);
static bool status_current_song_for_change(NcmSong *song);
static void status_call_ui_playlist_changed(uint32 previous_version);
static void status_request_playlist_update(uint32 previous_version);
static void status_call_ui_stored_playlists_changed(void);
static void status_call_ui_database_changed(void);
static void status_request_stored_playlists_update(void);
static void status_request_database_update(void);
static void status_call_ui_player_state_changed(void);
static void status_call_ui_player_stopped(void);
static void status_call_ui_song_id_changed(int32 song_id);
static void status_call_ui_current_song_changed(NcmSong *song);
static void status_run_player_state_command(void);
static void status_reset_visualizer_autoscale(void);
static void status_clear_visible_visualizer(void);
static void status_handle_current_song_changed(NcmSong *song);

static enum NcmStatusPlayerState
status_player_state_from_mpd(enum mpd_state state) {
    switch (state) {
    case MPD_STATE_STOP:
        return NCM_STATUS_PLAYER_STOP;
    case MPD_STATE_PLAY:
        return NCM_STATUS_PLAYER_PLAY;
    case MPD_STATE_PAUSE:
        return NCM_STATUS_PLAYER_PAUSE;
    case MPD_STATE_UNKNOWN:
    default:
        return NCM_STATUS_PLAYER_UNKNOWN;
    }
}

static NcmStatusHooks *
status_active_hooks(NcmStatusHooks *hooks) {
    if (hooks) {
        return hooks;
    }

    if (status_hooks_set) {
        return &status_hooks;
    }

    return NULL;
}

static int32
status_full_event_mask(void) {
    return MPD_IDLE_DATABASE | MPD_IDLE_STORED_PLAYLIST | MPD_IDLE_PLAYLIST
           | MPD_IDLE_PLAYER | MPD_IDLE_MIXER | MPD_IDLE_OUTPUT
           | MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS;
}

void
ncm_status_set_database_update_observer(void (*callback)(void *user),
                                        void *user) {
    status_database_update_observer = callback;
    status_database_update_observer_user = user;
    return;
}

void
ncm_status_set_playlist_update_observer(void (*callback)(void *user),
                                        void *user) {
    status_playlist_update_observer = callback;
    status_playlist_update_observer_user = user;
    return;
}

static void
status_update_timeout_from_screen(NcScreen *screen, void *user) {
    StatusTimeoutContext *context;
    int32 timeout;

    context = user;
    timeout = nc_screen_window_timeout(screen);
    if (timeout < context->timeout) {
        context->timeout = timeout;
    }
    return;
}

static void
status_refresh_footer(NcmStatusHooks *hooks) {
    NcWindow *footer;

    if (hooks && hooks->refresh_footer) {
        hooks->refresh_footer(hooks->user);
        return;
    }

    footer = ui_state_footer_window();
    if (footer) {
        nc_window_refresh(footer);
    }
    return;
}

static void
status_refresh_visible_screens(NcmStatusHooks *hooks) {
    if (hooks && hooks->refresh_visible_screens) {
        hooks->refresh_visible_screens(hooks->user);
        return;
    }

    app_controller_refresh_visible_screens();
    return;
}

static void
status_elapsed_time_changed(NcmStatusHooks *hooks, bool update) {
    if (hooks && hooks->elapsed_time_changed) {
        hooks->elapsed_time_changed(update, hooks->user);
        return;
    }

    ncm_status_changes_elapsed_time(update);
    return;
}

static void
status_print_client_error(char *message, int32 message_len) {
    NcmStringFormatArg arg;

    if (message == NULL) {
        message = "";
    }
    if (message_len < 0) {
        message_len = optional_strlen32(message);
    }

    arg = ncm_string_format_arg_string(message, message_len);
    ncm_statusbar_format((int32)Config.message_delay_time,
                         STRLIT_ARGS("ncmpcpp: %1%"), &arg, 1);
    return;
}

static void
status_print_server_error(char *message, int32 message_len) {
    NcmStringFormatArg arg;

    if (message == NULL) {
        message = "";
    }
    if (message_len < 0) {
        message_len = optional_strlen32(message);
    }

    arg = ncm_string_format_arg_string(message, message_len);
    ncm_statusbar_format((int32)Config.message_delay_time,
                         STRLIT_ARGS("MPD: %1%"), &arg, 1);
    return;
}

static void
status_request_exit(void) {
    ncm_action_runtime_request_exit(NULL);
    return;
}

static void
status_prompt_mpd_password(NcmMpdClient *client) {
    enum NcPromptStatus prompt_status;
    NcmError error;
    NcPrompt prompt = {0};
    NcWindow *window;
    char *password;
    bool password_allocated;

    if (client == NULL) {
        return;
    }

    window = ncm_statusbar_put();
    if (window == NULL) {
        return;
    }

    nc_window_print_cstring(window, "Password: ");
    prompt.initial_text = "";
    prompt.width = -1;
    prompt.encrypted = true;
    prompt.remember = false;

    password = NULL;
    prompt_status = nc_window_prompt(window, &prompt, &password);
    if (prompt_status != NC_PROMPT_ACCEPTED) {
        nc_window_prompt_result_destroy(password);
        status_request_exit();
        return;
    }

    password_allocated = password;
    if (password == NULL) {
        password = "";
    }

    ncm_error_clear(&error);
    if (!ncm_mpd_client_set_password(client, password, -1, &error)) {
        status_print_client_error(error.message, -1);
        if (password_allocated) {
            nc_window_prompt_result_destroy(password);
        }
        return;
    }
    if (password_allocated) {
        nc_window_prompt_result_destroy(password);
    }

    if (!ncm_mpd_client_send_password(client, &error)) {
        if (ncm_mpd_client_error_code(client) == MPD_ERROR_SERVER) {
            status_print_server_error(ncm_mpd_client_error_message(client), -1);
        } else {
            ncm_status_handle_client_error(client);
        }
        return;
    }

    ncm_statusbar_print((int32)Config.message_delay_time,
                        STRLIT_ARGS("Password accepted"));
    return;
}

void
ncm_status_handle_client_error_value(NcmMpdClient *client, char *message,
                                     int32 message_len, bool clearable) {
    if (client && !clearable) {
        ncm_mpd_client_disconnect(client);
    }

    status_print_client_error(message, message_len);
    return;
}

void
ncm_status_handle_server_error_value(NcmMpdClient *client, int32 code,
                                     char *message, int32 message_len) {
    status_print_server_error(message, message_len);
    if (code == MPD_SERVER_ERROR_PERMISSION) {
        status_prompt_mpd_password(client);
    }
    return;
}

void
ncm_status_handle_client_error(NcmMpdClient *client) {
    if (client == NULL) {
        return;
    }

    ncm_status_handle_client_error_value(
        client, ncm_mpd_client_error_message(client), -1,
        ncm_mpd_client_error_clearable(client));
    return;
}

void
ncm_status_trace(NcmMpdClient *client, bool update_timer,
                 bool update_window_timeout, NcmError *error) {
    StatusTimeoutContext timeout_context;
    NcmStatusHooks *hooks;
    NcWindow *footer;

    hooks = status_active_hooks(NULL);

    if (update_timer) {
        (void)global_timer_update(error);
    }

    if (client && ncm_mpd_client_connected(client)) {
        if (!status_initialized) {
            (void)ncm_status_initialize_connection(client, error);
            hooks = status_active_hooks(NULL);
        }

        if ((status_player_state == NCM_STATUS_PLAYER_PLAY)
            && (global_timer_elapsed_ms(status_past) > 1000)) {
            status_elapsed_time_changed(hooks, true);
            status_refresh_footer(hooks);
            status_past = global_timer;
        }

        app_controller_update_visible_screens();
        ncm_statusbar_try_redraw();
        (void)ncm_mpd_client_idle(client, error);
    }

    if (update_window_timeout) {
        timeout_context.timeout = INT_MAX;
        app_controller_each_visible_screen(status_update_timeout_from_screen,
                                           &timeout_context);

        footer = ui_state_footer_window();
        if (footer) {
            nc_window_set_timeout(footer, timeout_context.timeout);
        }
    }
    return;
}

static void
status_run_init_hooks(void) {
    NcmStatusInitHooks *hooks;

    hooks = NULL;
    if (status_init_hooks_set) {
        hooks = &status_init_hooks;
    }

    status_run_init_jump_to_now_playing(hooks);
    status_run_init_set_tcp_nodelay(hooks);
    status_run_init_load_browser_supported_extensions(hooks);
    status_run_init_fetch_outputs(hooks);
    status_run_init_setup_visualizer_datasource(hooks);
    status_run_init_register_mpd_fd_callback(hooks);
    status_run_init_show_connected_message(hooks);
    return;
}

static void
status_run_init_jump_to_now_playing(NcmStatusInitHooks *hooks) {
    int32 position;
    bool highlighted;

    if (hooks && hooks->jump_to_now_playing) {
        hooks->jump_to_now_playing(hooks->user);
        return;
    }

    if (!Config.jump_to_now_playing_song_at_start) {
        return;
    }

    position = status_current_song_pos;
    if (position < 0) {
        return;
    }

    highlighted = native_playlist_screen_locate_position(
        native_c_screen_playlist(), (uint32)position);
    if (!highlighted) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    "Song is filtered out");
    }

    {
        NcScreen *playlist_screen2 = native_c_screen_playlist_native();
        if (highlighted && app_controller_is_screen_visible(playlist_screen2)) {
            nc_screen_refresh(playlist_screen2);
        }
    }
    return;
}

static void
status_run_init_set_tcp_nodelay(NcmStatusInitHooks *hooks) {
#if defined(HAVE_NETINET_IN_H) && defined(HAVE_NETINET_TCP_H)
    int32 fd;
    int32 flag;
#endif

    if (hooks && hooks->set_tcp_nodelay) {
        hooks->set_tcp_nodelay(hooks->user);
        return;
    }

#if defined(HAVE_NETINET_IN_H) && defined(HAVE_NETINET_TCP_H)
    fd = ncm_mpd_client_fd(&global_mpd);
    if (fd < 0) {
        return;
    }

    flag = 1;
    (void)setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag,
                     (socklen_t)SIZEOF(flag));
#endif
    return;
}

static void
status_run_init_load_browser_supported_extensions(NcmStatusInitHooks *hooks) {
    if (hooks && hooks->load_browser_supported_extensions) {
        hooks->load_browser_supported_extensions(hooks->user);
        return;
    }
    native_c_screen_browser_fetch_supported_extensions();
    return;
}

static void
status_run_init_fetch_outputs(NcmStatusInitHooks *hooks) {
    if (hooks && hooks->fetch_outputs) {
        hooks->fetch_outputs(hooks->user);
        return;
    }

#if defined(ENABLE_OUTPUTS)
    native_c_screen_outputs_fetch_list();
#endif
    return;
}

static void
status_run_init_setup_visualizer_datasource(NcmStatusInitHooks *hooks) {
    if (hooks && hooks->setup_visualizer_datasource) {
        hooks->setup_visualizer_datasource(hooks->user);
        return;
    }

#if defined(ENABLE_VISUALIZER)
    {
        NativeVisualizerScreen *visualizer2 = native_c_screen_visualizer();
        native_visualizer_screen_close_data_source(visualizer2);
        (void)native_visualizer_screen_open_data_source(visualizer2);
        (void)native_visualizer_screen_find_output_id(visualizer2);
    }
#endif
    return;
}

static void
status_run_init_register_mpd_fd_callback(NcmStatusInitHooks *hooks) {
    NcWindow *footer;
    int32 fd;

    if (hooks && hooks->register_mpd_fd_callback) {
        hooks->register_mpd_fd_callback(hooks->user);
        return;
    }

    footer = ui_state_footer_window();
    fd = ncm_mpd_client_fd(&global_mpd);
    if ((footer == NULL) || (fd < 0)) {
        return;
    }

    nc_window_add_fd_callback(footer, fd, ncm_statusbar_mpd_idle_callback);
    return;
}

static void
status_run_init_show_connected_message(NcmStatusInitHooks *hooks) {
    NcmStringFormatArg arg;

    if (hooks && hooks->show_connected_message) {
        hooks->show_connected_message(hooks->user);
        return;
    }

    if (!Config.connected_message_on_startup) {
        return;
    }

    arg = ncm_string_format_arg_cstring(ncm_mpd_client_hostname(&global_mpd));
    ncm_statusbar_format((int32)Config.message_delay_time,
                         STRLIT_ARGS("Connected to %1%"), &arg, 1);
    return;
}

static void
status_notify_statusbar(void) {
    if (status_notification_observer) {
        status_notification_observer(status_notification_observer_user);
    }
    return;
}

static void
statusbar_format_cstring(char *format, int32 format_len, char *value) {
    NcmStringFormatArg arg;

    status_notify_statusbar();
    arg = ncm_string_format_arg_cstring(value);
    ncm_statusbar_format((int32)Config.message_delay_time, format, format_len,
                         &arg, 1);
    return;
}

static char *
status_on_off(char status) {
    if (status == 0) {
        return "off";
    }

    return "on";
}

bool
ncm_status_apply_mpd_status(NcmMpdStatus *mpd_status, int32 event,
                            NcmStatusHooks *hooks, NcmError *error) {
    NcmStringFormatArg arg;
    uint32 previous_playlist_version;
    char new_consume;
    char new_crossfade;
    char new_random;
    char new_repeat;
    char new_single;
    NcmStatusHooks *active_hooks;

    if (mpd_status == NULL) {
        ncm_error_set(error, -1, STRLIT_ARGS("MPD status is NULL"));
        return false;
    }

    active_hooks = status_active_hooks(hooks);

    status_current_song_pos = mpd_status->song_pos;
    status_elapsed_time = mpd_status->elapsed_time;
    status_kbps = mpd_status->kbit_rate;
    status_player_state = status_player_state_from_mpd(mpd_status->state);
    status_playlist_length = mpd_status->queue_length;
    status_total_time = mpd_status->total_time;
    status_volume = mpd_status->volume;

    if ((event & MPD_IDLE_DATABASE) != 0) {
        if (active_hooks
            && active_hooks->database_changed) {
            active_hooks->database_changed(active_hooks->user);
        } else {
            ncm_status_changes_database();
        }
    }

    if ((event & MPD_IDLE_STORED_PLAYLIST) != 0) {
        if (active_hooks
            && active_hooks->stored_playlists_changed) {
            active_hooks->stored_playlists_changed(active_hooks->user);
        } else {
            ncm_status_changes_stored_playlists();
        }
    }

    if ((event & MPD_IDLE_PLAYLIST) != 0) {
        previous_playlist_version = status_playlist_version;
        status_playlist_version = mpd_status->queue_version;
        if (active_hooks
            && active_hooks->playlist_changed) {
            active_hooks->playlist_changed(previous_playlist_version,
                                           active_hooks->user);
        } else {
            ncm_status_changes_playlist(previous_playlist_version);
        }
    }

    if ((event & MPD_IDLE_PLAYER) != 0) {
        if (active_hooks
            && active_hooks->player_state_changed) {
            active_hooks->player_state_changed(active_hooks->user);
        } else {
            ncm_status_changes_player_state();
        }

        if (status_current_song_id != mpd_status->song_id) {
            if (active_hooks
                && active_hooks->song_id_changed) {
                active_hooks->song_id_changed(mpd_status->song_id,
                                              active_hooks->user);
            } else {
                ncm_status_changes_song_id(mpd_status->song_id);
            }
            status_current_song_id = mpd_status->song_id;
        }
    }

    if ((event & MPD_IDLE_MIXER) != 0) {
        if (active_hooks && active_hooks->mixer_changed) {
            active_hooks->mixer_changed(active_hooks->user);
        } else {
            ncm_status_changes_mixer();
        }
    }

    if ((event & MPD_IDLE_OUTPUT) != 0) {
        if (active_hooks && active_hooks->outputs_changed) {
            active_hooks->outputs_changed(active_hooks->user);
        } else {
            ncm_status_changes_outputs();
        }
    }

    if ((event & MPD_IDLE_UPDATE) != 0) {
        status_db_updating = 0;
        if (mpd_status->update_id != 0) {
            status_db_updating = 'U';
        }

        if (status_initialized) {
            if (status_db_updating) {
                statusbar_format_cstring(STRLIT_ARGS("Database update %1%"),
                                         "started");
            } else {
                statusbar_format_cstring(STRLIT_ARGS("Database update %1%"),
                                         "finished");
            }
        }
    }

    if ((event & MPD_IDLE_OPTIONS) != 0) {
        new_repeat = 0;
        if (mpd_status->repeat) {
            new_repeat = 'r';
        }
        if (new_repeat != status_repeat) {
            status_repeat = new_repeat;
            if (status_initialized) {
                statusbar_format_cstring(STRLIT_ARGS("Repeat mode is %1%"),
                                         status_on_off(status_repeat));
            }
        }

        new_random = 0;
        if (mpd_status->random) {
            new_random = 'z';
        }
        if (new_random != status_random) {
            status_random = new_random;
            if (status_initialized) {
                statusbar_format_cstring(STRLIT_ARGS("Random mode is %1%"),
                                         status_on_off(status_random));
            }
        }

        new_single = 0;
        if (mpd_status->single) {
            new_single = 's';
        }
        if (new_single != status_single) {
            status_single = new_single;
            if (status_initialized) {
                statusbar_format_cstring(STRLIT_ARGS("Single mode is %1%"),
                                         status_on_off(status_single));
            }
        }

        new_consume = 0;
        if (mpd_status->consume) {
            new_consume = 'c';
        }
        if (new_consume != status_consume) {
            status_consume = new_consume;
            if (status_initialized) {
                statusbar_format_cstring(STRLIT_ARGS("Consume mode is %1%"),
                                         status_on_off(status_consume));
            }
        }

        new_crossfade = 0;
        if (mpd_status->crossfade != 0) {
            new_crossfade = 'x';
        }
        if (new_crossfade != status_crossfade) {
            status_crossfade = new_crossfade;
            if (status_initialized) {
                status_notify_statusbar();
                arg = ncm_string_format_arg_u64((uint64)mpd_status->crossfade);
                ncm_statusbar_format(
                    (int32)Config.message_delay_time,
                    STRLIT_ARGS("Crossfade set to %1% seconds"), &arg, 1);
            }
        }
    }

    if ((event & (MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS)) != 0) {
        if (active_hooks && active_hooks->flags_changed) {
            active_hooks->flags_changed(active_hooks->user);
        } else {
            ncm_status_changes_flags();
        }
    }

    status_initialized = true;

    if ((event & MPD_IDLE_PLAYER) != 0) {
        status_refresh_footer(active_hooks);
    }

    if ((event & (MPD_IDLE_PLAYLIST | MPD_IDLE_DATABASE | MPD_IDLE_PLAYER))
        != 0) {
        status_refresh_visible_screens(active_hooks);
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_status_initialize_from_mpd_status(NcmMpdStatus *mpd_status,
                                      NcmStatusHooks *hooks, NcmError *error) {
    if (!ncm_status_apply_mpd_status(mpd_status, status_full_event_mask(),
                                     hooks, error)) {
        return false;
    }

    status_run_init_hooks();
    return true;
}

bool
ncm_status_initialize_connection(NcmMpdClient *client, NcmError *error) {
    NcmMpdStatus mpd_status;

    if (client == NULL) {
        ncm_error_set(error, -1, STRLIT_ARGS("MPD client is NULL"));
        return false;
    }

    if (!ncm_mpd_client_get_status(client, &mpd_status, error)) {
        return false;
    }

    return ncm_status_initialize_from_mpd_status(&mpd_status, NULL, error);
}

bool
ncm_status_update(NcmMpdClient *client, int32 event, NcmError *error) {
    NcmMpdStatus mpd_status;

    if (client == NULL) {
        ncm_error_set(error, -1, STRLIT_ARGS("MPD client is NULL"));
        return false;
    }

    if (!ncm_mpd_client_get_status(client, &mpd_status, error)) {
        return false;
    }

    return ncm_status_apply_mpd_status(&mpd_status, event, NULL, error);
}

bool
ncm_status_update_full(NcmMpdClient *client, NcmStatusHooks *hooks,
                       NcmError *error) {
    NcmMpdStatus mpd_status;

    if (client == NULL) {
        ncm_error_set(error, -1, STRLIT_ARGS("MPD client is NULL"));
        return false;
    }

    if (!ncm_mpd_client_get_status(client, &mpd_status, error)) {
        return false;
    }

    return ncm_status_apply_mpd_status(&mpd_status, status_full_event_mask(),
                                       hooks, error);
}

bool
ncm_status_update_from_noidle(NcmMpdClient *client, NcmStatusHooks *hooks,
                              NcmError *error) {
    NcmMpdStatus mpd_status;
    int32 flags;

    if (client == NULL) {
        ncm_error_set(error, -1, STRLIT_ARGS("MPD client is NULL"));
        return false;
    }

    flags = 0;
    if (!ncm_mpd_client_noidle(client, &flags, error)) {
        return false;
    }

    if (!ncm_mpd_client_get_status(client, &mpd_status, error)) {
        return false;
    }

    return ncm_status_apply_mpd_status(&mpd_status, flags, hooks, error);
}

void
ncm_status_clear(void) {
    status_initialized = false;
    status_consume = 0;
    status_crossfade = 0;
    status_db_updating = 0;
    status_repeat = 0;
    status_random = 0;
    status_single = 0;
    status_current_song_id = -1;
    status_current_song_pos = -1;
    status_elapsed_time = 0;
    status_kbps = 0;
    status_player_state = NCM_STATUS_PLAYER_UNKNOWN;
    status_playlist_version = 0;
    status_playlist_length = 0;
    status_total_time = 0;
    status_volume = -1;
    return;
}

bool
ncm_status_state_consume(void) {
    return status_consume != 0;
}

bool
ncm_status_state_crossfade(void) {
    return status_crossfade != 0;
}

bool
ncm_status_state_repeat(void) {
    return status_repeat != 0;
}

bool
ncm_status_state_random(void) {
    return status_random != 0;
}

bool
ncm_status_state_single(void) {
    return status_single != 0;
}

int32
ncm_status_state_current_song_position(void) {
    return status_current_song_pos;
}

int32
ncm_status_state_playlist_length(void) {
    return status_playlist_length;
}

int32
ncm_status_state_elapsed_time(void) {
    return status_elapsed_time;
}

enum NcmStatusPlayerState
ncm_status_state_player(void) {
    return status_player_state;
}

int32
ncm_status_state_total_time(void) {
    return status_total_time;
}

int32
ncm_status_state_volume(void) {
    return status_volume;
}

void
ncm_status_changes_playlist(uint32 previous_version) {
    status_request_playlist_update(previous_version);
    status_call_ui_playlist_changed(previous_version);
    return;
}

void
ncm_status_changes_stored_playlists(void) {
    status_request_stored_playlists_update();
    status_call_ui_stored_playlists_changed();
    return;
}

void
ncm_status_changes_database(void) {
    status_request_database_update();
    status_call_ui_database_changed();
    return;
}

void
ncm_status_changes_player_state(void) {
    NcmSong song;
    char player_state[32];
    int32 player_state_len;
    NcWindow *header;

    status_run_player_state_command();
    status_call_ui_player_state_changed();

    switch (status_player_state) {
    case NCM_STATUS_PLAYER_PLAY:
        ncm_song_init(&song);
        if (native_playlist_screen_now_playing_song(
                native_c_screen_playlist(), status_current_song_pos, &song)) {
            status_draw_song_title(&song);
        }
        ncm_song_destroy(&song);
        native_playlist_screen_reload_remaining(native_c_screen_playlist());
        break;
    case NCM_STATUS_PLAYER_STOP:
        ncm_window_title_set(STRLIT_ARGS("ncmpcpp " VERSION));
        if (ncm_progressbar_is_unlocked()) {
            ncm_progressbar_draw(0, 0);
        }
        native_playlist_screen_reload_remaining(native_c_screen_playlist());
        if (Config.design == NCM_DESIGN_ALTERNATIVE) {
            header = ui_state_header_window();
            if (header) {
                nc_window_go_to_xy(header, 0, 0);
                nc_window_apply_term_manip(header, NC_TERM_CLEAR_TO_EOL);
                nc_window_go_to_xy(header, 0, 1);
                nc_window_apply_term_manip(header, NC_TERM_CLEAR_TO_EOL);
            }
            ncm_status_changes_mixer();
            ncm_status_changes_flags();
        }
        status_call_ui_player_stopped();
        status_clear_visible_visualizer();
        break;
    case NCM_STATUS_PLAYER_PAUSE:
    case NCM_STATUS_PLAYER_UNKNOWN:
        break;
    default:
        break;
    }

    player_state_len
        = status_player_state_string(player_state, (int32)SIZEOF(player_state));
    status_draw_player_state_label(player_state, player_state_len);
    ncm_status_changes_elapsed_time(false);
    return;
}

void
ncm_status_changes_reset_song_scroll(void) {
    status_playing_song_scroll_begin = 0;
    status_first_line_scroll_begin = 0;
    status_second_line_scroll_begin = 0;
    return;
}

void
ncm_status_changes_song_id(int32 song_id) {
    NcmSong song;

    native_playlist_screen_reload_remaining(native_c_screen_playlist());
    ncm_status_changes_reset_song_scroll();
    status_reset_visualizer_autoscale();
    status_call_ui_song_id_changed(song_id);

    if (status_player_state != NCM_STATUS_PLAYER_STOP) {
        ncm_song_init(&song);
        if (status_current_song_for_change(&song)) {
            status_handle_current_song_changed(&song);
            status_call_ui_current_song_changed(&song);
            status_draw_song_title(&song);
        }
        ncm_song_destroy(&song);
    }

    status_current_song_id = song_id;
    ncm_status_changes_elapsed_time(false);
    return;
}

static void
status_apply_formatted_color(NcWindow *window, NcFormattedColor *color) {
    enum NcFormat *formats;
    int32 count;

    if ((window == NULL) || (color == NULL)) {
        return;
    }

    nc_window_push_color(window, color->color);
    formats = nc_formatted_color_formats(color);
    count = nc_formatted_color_format_count(color);
    for (int32 i = 0; i < count; i += 1) {
        nc_window_apply_format(window, formats[i]);
    }
    return;
}

static void
status_apply_formatted_color_end(NcWindow *window, NcFormattedColor *color) {
    enum NcFormat *formats;
    int32 count;

    if ((window == NULL) || (color == NULL)) {
        return;
    }

    if (!nc_color_is_default(color->color)) {
        nc_window_push_color(window, nc_color_end());
    }
    formats = nc_formatted_color_formats(color);
    count = nc_formatted_color_format_count(color);
    for (int32 i = count - 1; i >= 0; i -= 1) {
        nc_window_apply_format(window, nc_format_reverse(formats[i]));
    }
    return;
}

static int32
status_song_time_string(uint32 length, char *buffer, int32 buffer_cap) {
    return ncm_helpers_show_song_time(length, buffer, buffer_cap);
}

static int32
status_player_state_string(char *buffer, int32 buffer_cap) {
    char *string;
    int32 len;

    string = "";
    switch (status_player_state) {
    case NCM_STATUS_PLAYER_UNKNOWN:
        if (Config.design == NCM_DESIGN_ALTERNATIVE) {
            string = "[unknown]";
        }
        break;
    case NCM_STATUS_PLAYER_PLAY:
        if (Config.design == NCM_DESIGN_ALTERNATIVE) {
            string = "[playing]";
        } else {
            string = "Playing:";
        }
        break;
    case NCM_STATUS_PLAYER_PAUSE:
        if (Config.design == NCM_DESIGN_ALTERNATIVE) {
            string = "[paused]";
        } else {
            string = "Paused:";
        }
        break;
    case NCM_STATUS_PLAYER_STOP:
        if (Config.design == NCM_DESIGN_ALTERNATIVE) {
            string = "[stopped]";
        }
        break;
    default:
        break;
    }

    len = optional_strlen32(string);
    if (len >= buffer_cap) {
        len = buffer_cap - 1;
    }
    if (len > 0) {
        memcpy64(buffer, string, len);
    }
    buffer[len] = 0;
    return len;
}

static void
status_draw_song_title(NcmSong *song) {
    NcmBuffer title;

    title = ncm_format_render_string(&Config.song_window_title_format, song);
    ncm_window_title_set(title.data, title.len);
    ncm_buffer_destroy(&title);
    return;
}

static void
status_call_ui_playlist_changed(uint32 previous_version) {
    if (status_ui_hooks_set && status_ui_hooks.playlist_changed) {
        status_ui_hooks.playlist_changed(previous_version,
                                         status_ui_hooks.user);
    }
    return;
}

static void
status_request_playlist_update(uint32 previous_version) {
    NcmError error;

    ncm_error_clear(&error);
    if (!native_playlist_screen_reload_from_mpd(
            native_c_screen_playlist(), &global_mpd, previous_version,
            status_playlist_length, &error)) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    error.message);
    } else if (status_playlist_update_observer) {
        status_playlist_update_observer(status_playlist_update_observer_user);
    }
    ncm_error_clear(&error);
    return;
}

static void
status_call_ui_stored_playlists_changed(void) {
    if (status_ui_hooks_set
        && status_ui_hooks.stored_playlists_changed) {
        status_ui_hooks.stored_playlists_changed(status_ui_hooks.user);
    }
    return;
}

static void
status_call_ui_database_changed(void) {
    if (status_ui_hooks_set && status_ui_hooks.database_changed) {
        status_ui_hooks.database_changed(status_ui_hooks.user);
    }
    return;
}

static void
status_request_stored_playlists_update(void) {
    NativeBrowserScreen *browser;
    NativePlaylistEditorScreen *editor;

    editor = native_c_screen_playlist_editor();
    native_playlist_editor_screen_request_playlists_update(editor);
    native_playlist_editor_screen_request_content_update(editor);

    browser = native_c_screen_browser();
    if (browser && !native_browser_screen_is_local(browser)
        && native_browser_screen_in_root_directory(browser)) {
        native_browser_screen_request_update(browser);
    }
    return;
}

static void
status_request_database_update(void) {
    native_browser_screen_request_update(native_c_screen_browser());
#if defined(HAVE_TAGLIB_H)
    native_tag_editor_screen_clear_directories(native_c_screen_tag_editor());
#endif
    native_media_library_screen_request_tags_update(
        native_c_screen_media_library());
    native_media_library_screen_request_albums_update(
        native_c_screen_media_library());
    native_media_library_screen_request_songs_update(
        native_c_screen_media_library());
    if (status_database_update_observer) {
        status_database_update_observer(status_database_update_observer_user);
    }
    return;
}

static void
status_call_ui_player_state_changed(void) {
    if (status_ui_hooks_set && status_ui_hooks.player_state_changed) {
        status_ui_hooks.player_state_changed(status_player_state,
                                             status_ui_hooks.user);
    }
    return;
}

static void
status_call_ui_player_stopped(void) {
    if (status_ui_hooks_set && status_ui_hooks.player_stopped) {
        status_ui_hooks.player_stopped(status_ui_hooks.user);
    }
    return;
}

static void
status_call_ui_song_id_changed(int32 song_id) {
    if (status_ui_hooks_set && status_ui_hooks.song_id_changed) {
        status_ui_hooks.song_id_changed(song_id, status_ui_hooks.user);
    }
    return;
}

static void
status_call_ui_current_song_changed(NcmSong *song) {
    if ((song == NULL) || ncm_song_empty(song)) {
        return;
    }
    if (status_ui_hooks_set && status_ui_hooks.current_song_changed) {
        status_ui_hooks.current_song_changed(song, status_ui_hooks.user);
    }
    return;
}

static char *
status_player_state_env(void) {
    switch (status_player_state) {
    case NCM_STATUS_PLAYER_PLAY:
        return "play";
    case NCM_STATUS_PLAYER_STOP:
        return "stop";
    case NCM_STATUS_PLAYER_PAUSE:
        return "pause";
    case NCM_STATUS_PLAYER_UNKNOWN:
        return "unknown";
    default:
        break;
    }

    return "unknown";
}

static void
status_run_player_state_command(void) {
    NcmError error;

    if (Config.execute_on_player_state_change_len <= 0) {
        return;
    }

    (void)setenv("MPD_PLAYER_STATE", status_player_state_env(), 1);
    ncm_error_clear(&error);
    (void)ncm_macro_run_external_command(
        Config.execute_on_player_state_change,
        Config.execute_on_player_state_change_len, true, &error);
    ncm_error_clear(&error);
    (void)unsetenv("MPD_PLAYER_STATE");
    return;
}

static void
status_run_song_change_command(void) {
    NcmError error;

    if (Config.execute_on_song_change_len <= 0) {
        return;
    }

    ncm_error_clear(&error);
    (void)ncm_macro_run_external_command(Config.execute_on_song_change,
                                         Config.execute_on_song_change_len,
                                         true, &error);
    ncm_error_clear(&error);
    return;
}

static void
status_reset_visualizer_autoscale(void) {
#if defined(ENABLE_VISUALIZER)
    native_visualizer_screen_reset_auto_scale_multiplier(
        native_c_screen_visualizer());
#endif
    return;
}

static void
status_clear_visible_visualizer(void) {
#if defined(ENABLE_VISUALIZER)
    NcScreen *visualizer;

    visualizer = native_c_screen_visualizer_native();
    if (app_controller_is_screen_visible(visualizer)) {
        native_visualizer_screen_clear(native_c_screen_visualizer());
    }
#endif
    return;
}

static void
status_fetch_background_lyrics(NcmSong *song) {
    NcmError error;

    if (!Config.fetch_lyrics_in_background) {
        return;
    }

    ncm_error_clear(&error);
    (void)native_lyrics_screen_fetch_in_background(native_c_screen_lyrics(),
                                                   song, false, &error);
    ncm_error_clear(&error);
    return;
}

static void
status_autocenter_playlist(NcmSong *song) {
    if (!Config.autocenter_mode) {
        return;
    }

    (void)native_playlist_screen_locate_position(native_c_screen_playlist(),
                                                 ncm_song_position(song));
    return;
}

static void
status_fetch_now_playing_lyrics(NcmSong *song) {
    NcmError error;

    if (!Config.now_playing_lyrics) {
        return;
    }
    if (!app_controller_is_screen_visible(native_c_screen_lyrics_native())) {
        return;
    }
    if (app_controller_previous_screen() != native_c_screen_playlist_native()) {
        return;
    }

    ncm_error_clear(&error);
    (void)native_lyrics_screen_fetch(native_c_screen_lyrics(), song, NULL,
                                     &error);
    ncm_error_clear(&error);
    return;
}

static void
status_handle_current_song_changed(NcmSong *song) {
    if ((song == NULL) || ncm_song_empty(song)) {
        return;
    }

    status_run_song_change_command();
    status_fetch_background_lyrics(song);
    status_autocenter_playlist(song);
    status_fetch_now_playing_lyrics(song);
    return;
}

static void
status_draw_player_state_label(char *state, int32 state_len) {
    NcWindow *window;

    switch (Config.design) {
    case NCM_DESIGN_ALTERNATIVE:
        window = ui_state_header_window();
        if (window == NULL) {
            return;
        }
        nc_window_go_to_xy(window, 0, 1);
        nc_window_apply_format(window, NC_FORMAT_BOLD);
        nc_window_print_data(window, state, state_len);
        nc_window_apply_format(window, NC_FORMAT_NO_BOLD);
        nc_window_refresh(window);
        break;
    case NCM_DESIGN_CLASSIC:
        window = ui_state_footer_window();
        if ((window == NULL) || !ncm_statusbar_is_unlocked()
            || !Config.statusbar_visibility) {
            return;
        }
        nc_window_go_to_xy(window, 0, 1);
        if (state_len == 0) {
            nc_window_apply_term_manip(window, NC_TERM_CLEAR_TO_EOL);
        } else {
            nc_window_apply_format(window, NC_FORMAT_BOLD);
            nc_window_print_data(window, state, state_len);
            nc_window_apply_format(window, NC_FORMAT_NO_BOLD);
        }
        break;
    case NCM_DESIGN_LAST:
        break;
    default:
        break;
    }
    return;
}

static bool
status_current_song_for_change(NcmSong *song) {
    NcmError error;

    if (song == NULL) {
        return false;
    }

    if (native_playlist_screen_now_playing_song(
            native_c_screen_playlist(), status_current_song_pos, song)) {
        return true;
    }

    if (!ncm_mpd_client_connected(&global_mpd)) {
        return false;
    }

    ncm_error_clear(&error);
    if (!ncm_mpd_client_get_current_song(&global_mpd, song, &error)) {
        return false;
    }
    return !ncm_song_empty(song);
}

static void
status_buffer_append_char(NcmBuffer *buffer, char ch) {
    ncm_buffer_append_byte(buffer, ch);
    return;
}

static void
status_buffer_append_uint32(NcmBuffer *buffer, uint32 value) {
    char tmp[32];
    int32 len;

    len = SNPRINTF(tmp, "%u", value);
    if (len < 0) {
        return;
    }
    if (len >= (int32)SIZEOF(tmp)) {
        len = (int32)SIZEOF(tmp) - 1;
    }
    ncm_buffer_append(buffer, tmp, len);
    return;
}

static void
status_tracklength_buffer(NcmBuffer *buffer) {
    char time_buffer[64];
    int32 time_len;

    ncm_buffer_clear(buffer);
    if ((Config.display_bitrate) && (status_kbps != 0)
        && (Config.design == NCM_DESIGN_CLASSIC)) {
        status_buffer_append_char(buffer, '(');
        status_buffer_append_uint32(buffer, status_kbps);
        ncm_buffer_append(buffer, STRLIT_ARGS(" kbps) "));
    }

    if (Config.design == NCM_DESIGN_CLASSIC) {
        status_buffer_append_char(buffer, '[');
    }

    if ((Config.display_remaining_time) && (status_total_time != 0)) {
        status_buffer_append_char(buffer, '-');
        if (status_elapsed_time < status_total_time) {
            time_len = status_song_time_string(
                status_total_time - status_elapsed_time, time_buffer,
                (int32)SIZEOF(time_buffer));
        } else {
            time_len = status_song_time_string(0, time_buffer,
                                               (int32)SIZEOF(time_buffer));
        }
    } else {
        time_len = status_song_time_string(status_elapsed_time, time_buffer,
                                           (int32)SIZEOF(time_buffer));
    }
    ncm_buffer_append(buffer, time_buffer, time_len);

    if (status_total_time != 0) {
        status_buffer_append_char(buffer, '/');
        time_len = status_song_time_string(status_total_time, time_buffer,
                                           (int32)SIZEOF(time_buffer));
        ncm_buffer_append(buffer, time_buffer, time_len);
    }

    if (Config.design == NCM_DESIGN_CLASSIC) {
        status_buffer_append_char(buffer, ']');
    } else if ((Config.display_bitrate) && (status_kbps != 0)) {
        ncm_buffer_append(buffer, STRLIT_ARGS(" ("));
        status_buffer_append_uint32(buffer, status_kbps);
        ncm_buffer_append(buffer, STRLIT_ARGS(" kbps)"));
    }
    return;
}

static void
status_draw_classic_elapsed_time(NcWindow *footer, NcmSong *song,
                                 char *player_state, int32 player_state_len) {
    NcBuffer rendered_song;
    NcmBuffer tracklength;
    int32 text_width;
    int32 track_x;
    char separator[] = " ** ";

    if ((footer == NULL) || !Config.statusbar_visibility) {
        return;
    }
    if (!ncm_statusbar_is_unlocked()) {
        return;
    }

    nc_buffer_init(&rendered_song);
    ncm_buffer_init(&tracklength);
    status_tracklength_buffer(&tracklength);
    ncm_format_render_buffer(&Config.song_status_format, song, &rendered_song,
                             &rendered_song, NCM_FORMAT_FLAG_ALL);

    nc_window_go_to_xy(footer, 0, 1);
    nc_window_apply_term_manip(footer, NC_TERM_CLEAR_TO_EOL);
    status_apply_formatted_color(footer, &Config.player_state_color);
    nc_window_print_data(footer, player_state, player_state_len);
    status_apply_formatted_color_end(footer, &Config.player_state_color);
    nc_window_print_char(footer, ' ');

    text_width = (int32)nc_window_width(footer) - player_state_len;
    text_width -= tracklength.len;
    text_width -= 2;
    if (text_width < 0) {
        text_width = 0;
    }
    nc_cyclic_buffer_write(&rendered_song, footer,
                           &status_playing_song_scroll_begin, text_width,
                           separator, STRLIT_LEN(" ** "));

    track_x = (int32)nc_window_width(footer) - tracklength.len;
    if (track_x < 0) {
        track_x = 0;
    }
    nc_window_go_to_xy(footer, track_x, 1);
    status_apply_formatted_color(footer, &Config.statusbar_time_color);
    nc_window_print_data(footer, tracklength.data, tracklength.len);
    status_apply_formatted_color_end(footer, &Config.statusbar_time_color);

    ncm_buffer_destroy(&tracklength);
    nc_buffer_destroy(&rendered_song);
    return;
}

static void
status_draw_alternative_elapsed_time(NcWindow *header, NcmSong *song,
                                     char *player_state,
                                     int32 player_state_len) {
    NcBuffer first;
    NcBuffer second;
    NcmBuffer tracklength;
    int32 first_len;
    int32 first_margin;
    int32 first_start;
    int32 second_len;
    int32 second_margin;
    int32 second_start;
    int32 text_width;
    int32 volume_x;
    char separator[] = " ** ";

    if (header == NULL) {
        return;
    }

    nc_buffer_init(&first);
    nc_buffer_init(&second);
    ncm_buffer_init(&tracklength);
    status_tracklength_buffer(&tracklength);

    ncm_format_render_buffer(&Config.new_header_first_line, song, &first,
                             &first, NCM_FORMAT_FLAG_ALL);
    ncm_format_render_buffer(&Config.new_header_second_line, song, &second,
                             &second, NCM_FORMAT_FLAG_ALL);

    first_len = utf8_width(first.data, first.len);
    first_margin = tracklength.len + 1;
    if (first_margin < (global_volume_state_len() + 1)) {
        first_margin = global_volume_state_len() + 1;
    }
    first_margin *= 2;
    first_start = tracklength.len + 1;
    if (first_len < (COLS - first_margin)) {
        first_start = (COLS - first_len) / 2;
    }

    second_len = utf8_width(second.data, second.len);
    second_margin = player_state_len;
    if (second_margin < 8) {
        second_margin = 8;
    }
    second_margin = (second_margin + 1)*2;
    second_start = player_state_len + 1;
    if (second_len < (COLS - second_margin)) {
        second_start = (COLS - second_len) / 2;
    }

    if (!global_seeking_in_progress) {
        nc_window_go_to_xy(header, 0, 0);
        nc_window_apply_term_manip(header, NC_TERM_CLEAR_TO_EOL);
        status_apply_formatted_color(header, &Config.statusbar_time_color);
        nc_window_print_data(header, tracklength.data, tracklength.len);
        status_apply_formatted_color_end(header, &Config.statusbar_time_color);
    }

    nc_window_go_to_xy(header, first_start, 0);
    text_width = COLS - tracklength.len - global_volume_state_len() - 1;
    if (text_width < 0) {
        text_width = 0;
    }
    nc_cyclic_buffer_write(&first, header, &status_first_line_scroll_begin,
                           text_width, separator, STRLIT_LEN(" ** "));

    nc_window_go_to_xy(header, 0, 1);
    nc_window_apply_term_manip(header, NC_TERM_CLEAR_TO_EOL);
    status_apply_formatted_color(header, &Config.player_state_color);
    nc_window_print_data(header, player_state, player_state_len);
    status_apply_formatted_color_end(header, &Config.player_state_color);
    nc_window_go_to_xy(header, second_start, 1);

    text_width = COLS - player_state_len - 10;
    if (text_width < 0) {
        text_width = 0;
    }
    nc_cyclic_buffer_write(&second, header, &status_second_line_scroll_begin,
                           text_width, separator, STRLIT_LEN(" ** "));

    volume_x = (int32)nc_window_width(header) - global_volume_state_len();
    if (volume_x < 0) {
        volume_x = 0;
    }
    nc_window_go_to_xy(header, volume_x, 0);
    status_apply_formatted_color(header, &Config.volume_color);
    nc_window_print_data(header, global_volume_state_cstr(),
                         global_volume_state_len());
    status_apply_formatted_color_end(header, &Config.volume_color);

    ncm_status_changes_flags();
    ncm_buffer_destroy(&tracklength);
    nc_buffer_destroy(&second);
    nc_buffer_destroy(&first);
    return;
}

static void
status_update_elapsed_from_mpd(void) {
    NcmMpdStatus mpd_status;
    NcmError error;

    if (!ncm_mpd_client_connected(&global_mpd)) {
        status_elapsed_time += 1;
        return;
    }

    ncm_error_clear(&error);
    if (!ncm_mpd_client_get_status(&global_mpd, &mpd_status, &error)) {
        status_elapsed_time += 1;
        return;
    }

    status_elapsed_time = mpd_status.elapsed_time;
    status_kbps = mpd_status.kbit_rate;
    return;
}

void
ncm_status_changes_elapsed_time(bool update_elapsed) {
    NcmSong song;
    NcWindow *footer;
    NcWindow *header;
    char player_state[32];
    int32 player_state_len;

    if (update_elapsed) {
        status_update_elapsed_from_mpd();
    }

    if ((status_player_state == NCM_STATUS_PLAYER_STOP)
        || (status_player_state == NCM_STATUS_PLAYER_UNKNOWN)) {
        footer = ui_state_footer_window();
        if (footer && ncm_statusbar_is_unlocked()
            && Config.statusbar_visibility) {
            nc_window_go_to_xy(footer, 0, 1);
            nc_window_apply_term_manip(footer, NC_TERM_CLEAR_TO_EOL);
        }
        if (ncm_progressbar_is_unlocked()) {
            ncm_progressbar_draw(0, 0);
        }
        return;
    }

    ncm_song_init(&song);
    if (!native_playlist_screen_now_playing_song(
            native_c_screen_playlist(), status_current_song_pos, &song)) {
        ncm_song_destroy(&song);
        footer = ui_state_footer_window();
        if (footer && ncm_statusbar_is_unlocked()
            && Config.statusbar_visibility) {
            nc_window_go_to_xy(footer, 0, 1);
            nc_window_apply_term_manip(footer, NC_TERM_CLEAR_TO_EOL);
        }
        if (ncm_progressbar_is_unlocked()) {
            ncm_progressbar_draw(0, 0);
        }
        return;
    }

    player_state_len
        = status_player_state_string(player_state, (int32)SIZEOF(player_state));
    status_draw_song_title(&song);

    switch (Config.design) {
    case NCM_DESIGN_CLASSIC:
        status_draw_classic_elapsed_time(ui_state_footer_window(), &song,
                                         player_state, player_state_len);
        break;
    case NCM_DESIGN_ALTERNATIVE:
        header = ui_state_header_window();
        status_draw_alternative_elapsed_time(header, &song, player_state,
                                             player_state_len);
        break;
    case NCM_DESIGN_LAST:
        break;
    default:
        break;
    }

    if (ncm_progressbar_is_unlocked()) {
        ncm_progressbar_draw(status_elapsed_time, status_total_time);
    }

    ncm_song_destroy(&song);
    return;
}

void
ncm_status_changes_flags(void) {
    NcWindow *header;
    NcmBuffer switch_state;
    int32 flags_x;

    if (!Config.header_visibility && (Config.design == NCM_DESIGN_CLASSIC)) {
        return;
    }

    header = ui_state_header_window();
    if (header == NULL) {
        return;
    }

    ncm_buffer_init(&switch_state);
    switch (Config.design) {
    case NCM_DESIGN_CLASSIC:
        if (status_repeat) {
            status_buffer_append_char(&switch_state, status_repeat);
        }
        if (status_random) {
            status_buffer_append_char(&switch_state, status_random);
        }
        if (status_single) {
            status_buffer_append_char(&switch_state, status_single);
        }
        if (status_consume) {
            status_buffer_append_char(&switch_state, status_consume);
        }
        if (status_crossfade) {
            status_buffer_append_char(&switch_state, status_crossfade);
        }
        if (status_db_updating) {
            status_buffer_append_char(&switch_state, status_db_updating);
        }

        status_apply_formatted_color(header, &Config.state_line_color);
        mvwhline(nc_window_raw(header), 1, 0, 0, COLS);
        status_apply_formatted_color_end(header, &Config.state_line_color);

        if (switch_state.len > 0) {
            flags_x = COLS - switch_state.len - 3;
            if (flags_x < 0) {
                flags_x = 0;
            }
            nc_window_go_to_xy(header, flags_x, 1);
            status_apply_formatted_color(header, &Config.state_line_color);
            nc_window_print_char(header, '[');
            status_apply_formatted_color_end(header, &Config.state_line_color);
            status_apply_formatted_color(header, &Config.state_flags_color);
            nc_window_print_data(header, switch_state.data, switch_state.len);
            status_apply_formatted_color_end(header, &Config.state_flags_color);
            status_apply_formatted_color(header, &Config.state_line_color);
            nc_window_print_char(header, ']');
            status_apply_formatted_color_end(header, &Config.state_line_color);
        }
        break;
    case NCM_DESIGN_ALTERNATIVE:
        status_buffer_append_char(&switch_state, '[');
        if (status_repeat) {
            status_buffer_append_char(&switch_state, status_repeat);
        } else {
            status_buffer_append_char(&switch_state, '-');
        }
        if (status_random) {
            status_buffer_append_char(&switch_state, status_random);
        } else {
            status_buffer_append_char(&switch_state, '-');
        }
        if (status_single) {
            status_buffer_append_char(&switch_state, status_single);
        } else {
            status_buffer_append_char(&switch_state, '-');
        }
        if (status_consume) {
            status_buffer_append_char(&switch_state, status_consume);
        } else {
            status_buffer_append_char(&switch_state, '-');
        }
        if (status_crossfade) {
            status_buffer_append_char(&switch_state, status_crossfade);
        } else {
            status_buffer_append_char(&switch_state, '-');
        }
        if (status_db_updating) {
            status_buffer_append_char(&switch_state, status_db_updating);
        } else {
            status_buffer_append_char(&switch_state, '-');
        }
        status_buffer_append_char(&switch_state, ']');

        flags_x = COLS - switch_state.len;
        if (flags_x < 0) {
            flags_x = 0;
        }
        nc_window_go_to_xy(header, flags_x, 1);
        status_apply_formatted_color(header, &Config.state_flags_color);
        nc_window_print_data(header, switch_state.data, switch_state.len);
        status_apply_formatted_color_end(header, &Config.state_flags_color);
        if (!Config.header_visibility) {
            status_apply_formatted_color(
                header, &Config.alternative_ui_separator_color);
            mvwhline(nc_window_raw(header), 2, 0, 0, COLS);
            status_apply_formatted_color_end(
                header, &Config.alternative_ui_separator_color);
        }
        break;
    case NCM_DESIGN_LAST:
        break;
    default:
        break;
    }

    nc_window_refresh(header);
    ncm_buffer_destroy(&switch_state);
    return;
}

void
ncm_status_changes_mixer(void) {
    NcWindow *header;
    char volume[32];
    int32 volume_len;
    int32 volume_x;

    if (!Config.display_volume_level
        || (!Config.header_visibility
            && (Config.design == NCM_DESIGN_CLASSIC))) {
        return;
    }

    header = ui_state_header_window();
    if (header == NULL) {
        return;
    }

    switch (Config.design) {
    case NCM_DESIGN_CLASSIC:
        global_volume_state_set(" Volume: ", STRLIT_LEN(" Volume: "));
        break;
    case NCM_DESIGN_ALTERNATIVE:
        global_volume_state_set(" Vol: ", STRLIT_LEN(" Vol: "));
        break;
    case NCM_DESIGN_LAST:
        break;
    default:
        break;
    }

    if (status_volume < 0) {
        global_volume_state_append("n/a", STRLIT_LEN("n/a"));
    } else {
        volume_len = SNPRINTF(volume, "%d", status_volume);
        if (volume_len < 0) {
            volume_len = 0;
        }
        global_volume_state_append(volume, volume_len);
        global_volume_state_append("%", STRLIT_LEN("%"));
    }

    volume_x = (int32)nc_window_width(header) - global_volume_state_len();
    if (volume_x < 0) {
        volume_x = 0;
    }
    nc_window_go_to_xy(header, volume_x, 0);
    status_apply_formatted_color(header, &Config.volume_color);
    nc_window_print_data(header, global_volume_state_cstr(),
                         global_volume_state_len());
    status_apply_formatted_color_end(header, &Config.volume_color);
    nc_window_refresh(header);
    return;
}

void
ncm_status_changes_outputs(void) {
#if ENABLE_OUTPUTS
    native_c_screen_outputs_fetch_list();
    native_c_screen_outputs_refresh_if_visible();
#endif
    return;
}

#endif /* NCMPCPP_STATUS_C */
