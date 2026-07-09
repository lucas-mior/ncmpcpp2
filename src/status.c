#include "status.h"

#include <mpd/status.h>

#include "cbase/base_macros.h"
#include "global.h"
#include "settings.h"
#include "statusbar.h"
#include "app_controller.h"
#include "ui_state.h"

#include <limits.h>

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
static void (*status_initialize_hook)(void *user);
static void *status_initialize_hook_user;
static void (*status_notification_observer)(void *user);
static void *status_notification_observer_user;
static NcmTimePoint status_past;

typedef struct StatusTimeoutContext {
    int32 timeout;
} StatusTimeoutContext;

static void status_update_timeout_from_screen(NcScreen *screen, void *user);
static void status_refresh_footer(NcmStatusHooks *hooks);
static void status_elapsed_time_changed(NcmStatusHooks *hooks, bool update);

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
    if (hooks != NULL) {
        return hooks;
    }

    if (status_hooks_set) {
        return &status_hooks;
    }

    return NULL;
}

void
ncm_status_set_hooks(NcmStatusHooks *hooks) {
    if (hooks == NULL) {
        status_hooks_set = false;
        return;
    }

    status_hooks = *hooks;
    status_hooks_set = true;
    return;
}

void
ncm_status_set_initialize_hook(void (*callback)(void *user), void *user) {
    status_initialize_hook = callback;
    status_initialize_hook_user = user;
    return;
}

void
ncm_status_set_notification_observer(void (*callback)(void *user),
                                     void *user) {
    status_notification_observer = callback;
    status_notification_observer_user = user;
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

    if ((hooks != NULL) && (hooks->refresh_footer != NULL)) {
        hooks->refresh_footer(hooks->user);
        return;
    }

    footer = ui_state_footer_window();
    if (footer != NULL) {
        nc_window_refresh(footer);
    }
    return;
}

static void
status_elapsed_time_changed(NcmStatusHooks *hooks, bool update) {
    if ((hooks != NULL) && (hooks->elapsed_time_changed != NULL)) {
        hooks->elapsed_time_changed(update, hooks->user);
        return;
    }

    ncm_status_changes_elapsed_time(update);
    return;
}

void
ncm_status_handle_client_error(NcmMpdClient *client) {
    char *message;
    NcmStringFormatArg arg;

    if ((client != NULL) && !ncm_mpd_client_error_clearable(client)) {
        ncm_mpd_client_disconnect(client);
    }

    if (client != NULL) {
        message = ncm_mpd_client_error_message(client);
        arg = ncm_string_format_arg_cstring(message);
        ncm_statusbar_format((int32)Config.message_delay_time,
                             STRLIT_ARGS("ncmpcpp: %1%"), &arg, 1);
    }
    return;
}

void
ncm_status_handle_server_error(NcmMpdClient *client) {
    char *message;
    NcmStringFormatArg arg;

    if (client != NULL) {
        message = ncm_mpd_client_error_message(client);
        arg = ncm_string_format_arg_cstring(message);
        ncm_statusbar_format((int32)Config.message_delay_time,
                             STRLIT_ARGS("MPD: %1%"), &arg, 1);
    }
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

    if ((client != NULL) && ncm_mpd_client_connected(client)) {
        if (!status_initialized) {
            if (status_initialize_hook != NULL) {
                status_initialize_hook(status_initialize_hook_user);
                hooks = status_active_hooks(NULL);
            } else {
                (void)ncm_status_update(client, -1, error);
            }
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
        if (footer != NULL) {
            nc_window_set_timeout(footer, timeout_context.timeout);
        }
    }
    return;
}

static void
status_notify_statusbar(void) {
    if (status_notification_observer != NULL) {
        status_notification_observer(status_notification_observer_user);
    }
    return;
}

static void
statusbar_format_cstring(char *format, int32 format_len, char *value) {
    NcmStringFormatArg arg;

    status_notify_statusbar();
    arg = ncm_string_format_arg_cstring(value);
    ncm_statusbar_format((int32)Config.message_delay_time,
                         format, format_len, &arg, 1);
    return;
}

static char *
status_on_off(char status) {
    if (status == 0) {
        return (char *)"off";
    }

    return (char *)"on";
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
        if ((active_hooks != NULL)
            && (active_hooks->database_changed != NULL)) {
            active_hooks->database_changed(active_hooks->user);
        } else {
            ncm_status_changes_database();
        }
    }

    if ((event & MPD_IDLE_STORED_PLAYLIST) != 0) {
        if ((active_hooks != NULL)
            && (active_hooks->stored_playlists_changed != NULL)) {
            active_hooks->stored_playlists_changed(active_hooks->user);
        } else {
            ncm_status_changes_stored_playlists();
        }
    }

    if ((event & MPD_IDLE_PLAYLIST) != 0) {
        previous_playlist_version = status_playlist_version;
        status_playlist_version = mpd_status->queue_version;
        if ((active_hooks != NULL)
            && (active_hooks->playlist_changed != NULL)) {
            active_hooks->playlist_changed(previous_playlist_version,
                                           active_hooks->user);
        } else {
            ncm_status_changes_playlist(previous_playlist_version);
        }
    }

    if ((event & MPD_IDLE_PLAYER) != 0) {
        if ((active_hooks != NULL)
            && (active_hooks->player_state_changed != NULL)) {
            active_hooks->player_state_changed(active_hooks->user);
        } else {
            ncm_status_changes_player_state();
        }

        if (status_current_song_id != mpd_status->song_id) {
            if ((active_hooks != NULL)
                && (active_hooks->song_id_changed != NULL)) {
                active_hooks->song_id_changed(mpd_status->song_id,
                                              active_hooks->user);
            } else {
                ncm_status_changes_song_id(mpd_status->song_id);
            }
            status_current_song_id = mpd_status->song_id;
        }
    }

    if ((event & MPD_IDLE_MIXER) != 0) {
        if ((active_hooks != NULL) && (active_hooks->mixer_changed != NULL)) {
            active_hooks->mixer_changed(active_hooks->user);
        } else {
            ncm_status_changes_mixer();
        }
    }

    if ((event & MPD_IDLE_OUTPUT) != 0) {
        if ((active_hooks != NULL) && (active_hooks->outputs_changed != NULL)) {
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
                                         (char *)"started");
            } else {
                statusbar_format_cstring(STRLIT_ARGS("Database update %1%"),
                                         (char *)"finished");
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
                arg = ncm_string_format_arg_u64(
                    (uint64)mpd_status->crossfade);
                ncm_statusbar_format(
                    (int32)Config.message_delay_time,
                    STRLIT_ARGS("Crossfade set to %1% seconds"), &arg, 1);
            }
        }
    }

    if ((event & (MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS)) != 0) {
        if ((active_hooks != NULL) && (active_hooks->flags_changed != NULL)) {
            active_hooks->flags_changed(active_hooks->user);
        } else {
            ncm_status_changes_flags();
        }
    }

    status_initialized = true;

    if ((event & MPD_IDLE_PLAYER) != 0) {
        if ((active_hooks != NULL) && (active_hooks->refresh_footer != NULL)) {
            active_hooks->refresh_footer(active_hooks->user);
        }
    }

    if ((event & (MPD_IDLE_PLAYLIST
                  |MPD_IDLE_DATABASE
                  |MPD_IDLE_PLAYER)) != 0) {
        if ((active_hooks != NULL)
            && (active_hooks->refresh_visible_screens != NULL)) {
            active_hooks->refresh_visible_screens(active_hooks->user);
        }
    }

    ncm_error_clear(error);
    return true;
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
ncm_status_update_from_noidle(NcmMpdClient *client,
                              NcmStatusHooks *hooks,
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
ncm_status_state_current_song_id(void) {
    return status_current_song_id;
}

int32
ncm_status_state_current_song_position(void) {
    return status_current_song_pos;
}

uint32
ncm_status_state_playlist_length(void) {
    return status_playlist_length;
}

uint32
ncm_status_state_elapsed_time(void) {
    return status_elapsed_time;
}

uint32
ncm_status_state_kbps(void) {
    return status_kbps;
}

enum NcmStatusPlayerState
ncm_status_state_player(void) {
    return status_player_state;
}

uint32
ncm_status_state_playlist_version(void) {
    return status_playlist_version;
}

uint32
ncm_status_state_total_time(void) {
    return status_total_time;
}

int32
ncm_status_state_volume(void) {
    return status_volume;
}

bool
ncm_status_state_database_updating(void) {
    return status_db_updating != 0;
}

void
ncm_status_state_sync_from_legacy(bool initialized,
                                  bool consume,
                                  bool crossfade,
                                  bool database_updating,
                                  bool repeat,
                                  bool random,
                                  bool single,
                                  int32 current_song_id,
                                  int32 current_song_pos,
                                  uint32 elapsed_time,
                                  uint32 kbps,
                                  enum NcmStatusPlayerState player,
                                  uint32 playlist_version,
                                  uint32 playlist_length,
                                  uint32 total_time,
                                  int32 volume) {
    status_initialized = initialized;
    status_consume = 0;
    status_crossfade = 0;
    status_db_updating = 0;
    status_repeat = 0;
    status_random = 0;
    status_single = 0;

    if (consume) {
        status_consume = 'c';
    }
    if (crossfade) {
        status_crossfade = 'x';
    }
    if (database_updating) {
        status_db_updating = 'U';
    }
    if (repeat) {
        status_repeat = 'r';
    }
    if (random) {
        status_random = 'z';
    }
    if (single) {
        status_single = 's';
    }

    status_current_song_id = current_song_id;
    status_current_song_pos = current_song_pos;
    status_elapsed_time = elapsed_time;
    status_kbps = kbps;
    status_player_state = player;
    status_playlist_version = playlist_version;
    status_playlist_length = playlist_length;
    status_total_time = total_time;
    status_volume = volume;
    return;
}

void
ncm_status_changes_playlist(uint32 previous_version) {
    (void)previous_version;
    return;
}

void
ncm_status_changes_stored_playlists(void) {
    return;
}

void
ncm_status_changes_database(void) {
    return;
}

void
ncm_status_changes_player_state(void) {
    return;
}

void
ncm_status_changes_song_id(int32 song_id) {
    status_current_song_id = song_id;
    return;
}

void
ncm_status_changes_elapsed_time(bool update_elapsed) {
    if (update_elapsed) {
        status_elapsed_time += 1;
    }
    return;
}

void
ncm_status_changes_flags(void) {
    return;
}

void
ncm_status_changes_mixer(void) {
    return;
}

void
ncm_status_changes_outputs(void) {
    return;
}
