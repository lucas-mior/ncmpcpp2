#include "status.h"

#include <mpd/status.h>

#include "cbase/base_macros.h"
#include "global.h"
#include "settings.h"
#include "statusbar.h"

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
    (void)update_window_timeout;

    if (update_timer) {
        (void)global_timer_update(error);
    }

    if ((client != NULL) && ncm_mpd_client_connected(client)) {
        if (!status_initialized) {
            (void)ncm_status_update(client, -1, error);
        }
        ncm_statusbar_try_redraw();
        (void)ncm_mpd_client_idle(client, error);
    }
    return;
}

static void
statusbar_format_cstring(char *format, int32 format_len, char *value) {
    NcmStringFormatArg arg;

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

    if (mpd_status == NULL) {
        ncm_error_set(error, -1, STRLIT_ARGS("MPD status is NULL"));
        return false;
    }

    status_current_song_pos = mpd_status->song_pos;
    status_elapsed_time = mpd_status->elapsed_time;
    status_kbps = mpd_status->kbit_rate;
    status_player_state = status_player_state_from_mpd(mpd_status->state);
    status_playlist_length = mpd_status->queue_length;
    status_total_time = mpd_status->total_time;
    status_volume = mpd_status->volume;

    if ((event & MPD_IDLE_DATABASE) != 0) {
        if ((hooks != NULL) && (hooks->database_changed != NULL)) {
            hooks->database_changed(hooks->user);
        } else {
            ncm_status_changes_database();
        }
    }

    if ((event & MPD_IDLE_STORED_PLAYLIST) != 0) {
        if ((hooks != NULL) && (hooks->stored_playlists_changed != NULL)) {
            hooks->stored_playlists_changed(hooks->user);
        } else {
            ncm_status_changes_stored_playlists();
        }
    }

    if ((event & MPD_IDLE_PLAYLIST) != 0) {
        previous_playlist_version = status_playlist_version;
        if ((hooks != NULL) && (hooks->playlist_changed != NULL)) {
            hooks->playlist_changed(previous_playlist_version, hooks->user);
        } else {
            ncm_status_changes_playlist(previous_playlist_version);
        }
        status_playlist_version = mpd_status->queue_version;
    }

    if ((event & MPD_IDLE_PLAYER) != 0) {
        if ((hooks != NULL) && (hooks->player_state_changed != NULL)) {
            hooks->player_state_changed(hooks->user);
        } else {
            ncm_status_changes_player_state();
        }

        if (status_current_song_id != mpd_status->song_id) {
            if ((hooks != NULL) && (hooks->song_id_changed != NULL)) {
                hooks->song_id_changed(mpd_status->song_id, hooks->user);
            } else {
                ncm_status_changes_song_id(mpd_status->song_id);
            }
            status_current_song_id = mpd_status->song_id;
        }
    }

    if ((event & MPD_IDLE_MIXER) != 0) {
        if ((hooks != NULL) && (hooks->mixer_changed != NULL)) {
            hooks->mixer_changed(hooks->user);
        } else {
            ncm_status_changes_mixer();
        }
    }

    if ((event & MPD_IDLE_OUTPUT) != 0) {
        if ((hooks != NULL) && (hooks->outputs_changed != NULL)) {
            hooks->outputs_changed(hooks->user);
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
                arg = ncm_string_format_arg_u64(
                    (uint64)mpd_status->crossfade);
                ncm_statusbar_format(
                    (int32)Config.message_delay_time,
                    STRLIT_ARGS("Crossfade set to %1% seconds"), &arg, 1);
            }
        }
    }

    if ((event & (MPD_IDLE_UPDATE | MPD_IDLE_OPTIONS)) != 0) {
        if ((hooks != NULL) && (hooks->flags_changed != NULL)) {
            hooks->flags_changed(hooks->user);
        } else {
            ncm_status_changes_flags();
        }
    }

    status_initialized = true;

    if ((event & MPD_IDLE_PLAYER) != 0) {
        if ((hooks != NULL) && (hooks->refresh_footer != NULL)) {
            hooks->refresh_footer(hooks->user);
        }
    }

    if ((event & (MPD_IDLE_PLAYLIST
                  |MPD_IDLE_DATABASE
                  |MPD_IDLE_PLAYER)) != 0) {
        if ((hooks != NULL) && (hooks->refresh_visible_screens != NULL)) {
            hooks->refresh_visible_screens(hooks->user);
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
ncm_status_state_sync_from_legacy(enum NcmStatusPlayerState player,
                                  uint32 elapsed_time,
                                  uint32 total_time) {
    status_player_state = player;
    status_elapsed_time = elapsed_time;
    status_total_time = total_time;
    return;
}

void
ncm_status_changes_playlist(uint32 previous_version) {
    status_playlist_version = previous_version;
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
