#if !defined(NCM_MPD_CLIENT_C)
#define NCM_MPD_CLIENT_C

#include "c/ncm_mpd_client.h"

#include <errno.h>

#include "c/ncm_regex.h"
#include "cbase/base_macros.h"

static void
ncm_mpd_client_set_buffer(NcmBuffer *buffer, char *string,
                          int32 string_len) {
    ncm_buffer_clear(buffer);
    if ((string == NULL) || (string_len <= 0)) {
        ncm_buffer_append_byte(buffer, '\0');
        buffer->len = 0;
        return;
    }

    ncm_buffer_append(buffer, string, string_len);
    ncm_buffer_append_byte(buffer, '\0');
    buffer->len = string_len;
    return;
}

static char *
ncm_mpd_client_buffer_cstr(NcmBuffer *buffer) {
    if (buffer == NULL) {
        return "";
    }
    if (buffer->data == NULL) {
        return "";
    }

    return buffer->data;
}

static void
ncm_mpd_client_copy_connection_error(NcmMpdClient *client,
                                     NcmError *error) {
    char *message;
    int32 message_len;

    if (error == NULL) {
        return;
    }
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return;
    }

    message = ncm_mpd_connection_error(&client->connection);
    message_len = optional_strlen32(message);
    ncm_error_set(error,
                  (int32)ncm_mpd_connection_error_code(&client->connection),
                  message, message_len);
    return;
}

static bool
ncm_mpd_client_require_connected(NcmMpdClient *client, NcmError *error) {
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (!ncm_mpd_connection_is_connected(&client->connection)) {
        ncm_error_set(error, (int32)MPD_ERROR_STATE,
                      STRLIT_ARGS("No active MPD connection"));
        return false;
    }

    return true;
}

static bool
ncm_mpd_client_prechecks(NcmMpdClient *client, NcmError *error) {
    int32 flags;

    if (!ncm_mpd_client_require_connected(client, error)) {
        return false;
    }

    flags = 0;
    if (!ncm_mpd_client_noidle(client, &flags, error)) {
        return false;
    }
    if ((flags != 0) && client->noidle_callback) {
        client->noidle_callback(flags, client->noidle_user);
    }

    return true;
}

static bool
ncm_mpd_client_prechecks_no_commands(NcmMpdClient *client,
                                     NcmError *error) {
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (client->command_list_active) {
        ncm_error_set(error, (int32)MPD_ERROR_STATE,
                      STRLIT_ARGS("MPD command list is already active"));
        return false;
    }

    return ncm_mpd_client_prechecks(client, error);
}

void
ncm_mpd_client_init(NcmMpdClient *client) {
    if (client == NULL) {
        return;
    }

    ncm_mpd_connection_init(&client->connection);
    sb_init(&client->host);
    sb_init(&client->password);
    ncm_mpd_client_set_buffer(&client->host, STRLIT_ARGS("localhost"));
    ncm_mpd_client_set_buffer(&client->password, NULL, 0);
    client->port = 6600;
    client->timeout_ms = 15000;
    client->command_list_active = false;
    client->idle = false;
    client->fd = -1;
    client->noidle_callback = NULL;
    client->noidle_user = NULL;
    return;
}

void
ncm_mpd_client_destroy(NcmMpdClient *client) {
    if (client == NULL) {
        return;
    }

    ncm_mpd_client_disconnect(client);
    ncm_mpd_connection_destroy(&client->connection);
    sb_free(&client->host);
    sb_free(&client->password);
    client->port = 0;
    client->timeout_ms = 0;
    client->command_list_active = false;
    client->idle = false;
    client->fd = -1;
    client->noidle_callback = NULL;
    client->noidle_user = NULL;
    return;
}

char *
ncm_mpd_client_hostname(NcmMpdClient *client) {
    if (client == NULL) {
        return "";
    }

    return ncm_mpd_client_buffer_cstr(&client->host);
}

bool
ncm_mpd_client_connected(NcmMpdClient *client) {
    if (client == NULL) {
        return false;
    }

    return ncm_mpd_connection_is_connected(&client->connection);
}

int32
ncm_mpd_client_version(NcmMpdClient *client) {
    if (client == NULL) {
        return 0;
    }

    return ncm_mpd_connection_version(&client->connection);
}

int32
ncm_mpd_client_fd(NcmMpdClient *client) {
    if (client == NULL) {
        return -1;
    }

    return ncm_mpd_connection_fd(&client->connection);
}

void
ncm_mpd_client_set_noidle_callback(NcmMpdClient *client,
                                   NcmMpdNoidleCallback *callback,
                                   void *user) {
    if (client == NULL) {
        return;
    }

    client->noidle_callback = callback;
    client->noidle_user = user;
    return;
}

bool
ncm_mpd_client_set_hostname(NcmMpdClient *client, char *host,
                            int32 host_len, NcmError *error) {
    int32 at;

    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (host == NULL) {
        host = "";
        host_len = 0;
    }
    if (host_len < 0) {
        host_len = optional_strlen32(host);
    }

    at = -1;
    for (int32 i = 0; i < host_len; i += 1) {
        if (host[i] == '@') {
            at = i;
            break;
        }
    }

    if (at > 0) {
        ncm_mpd_client_set_buffer(&client->password, host, at);
        ncm_mpd_client_set_buffer(&client->host, host + at + 1,
                                  host_len - at - 1);
    } else {
        ncm_mpd_client_set_buffer(&client->host, host, host_len);
    }

    ncm_error_clear(error);
    return true;
}

void
ncm_mpd_client_set_port(NcmMpdClient *client, uint16 port) {
    if (client == NULL) {
        return;
    }

    client->port = port;
    return;
}

bool
ncm_mpd_client_set_password(NcmMpdClient *client, char *password,
                            int32 password_len, NcmError *error) {
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (password_len < 0) {
        password_len = optional_strlen32(password);
    }

    ncm_mpd_client_set_buffer(&client->password, password, password_len);
    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_set_timeout_ms(NcmMpdClient *client, int32 timeout_ms,
                              NcmError *error) {
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }

    client->timeout_ms = timeout_ms;
    if (ncm_mpd_client_connected(client)) {
        if (!ncm_mpd_connection_set_timeout(&client->connection,
                                            timeout_ms)) {
            ncm_mpd_client_copy_connection_error(client, error);
            return false;
        }
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_connect(NcmMpdClient *client, NcmError *error) {
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }

    if (!ncm_mpd_connection_connect(&client->connection,
                                    ncm_mpd_client_buffer_cstr(&client->host),
                                    client->port, client->timeout_ms)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    client->fd = ncm_mpd_connection_fd(&client->connection);
    client->idle = false;
    client->command_list_active = false;
    if (client->password.len > 0) {
        if (!ncm_mpd_client_send_password(client, error)) {
            ncm_mpd_client_disconnect(client);
            return false;
        }
    }

    ncm_error_clear(error);
    return true;
}

void
ncm_mpd_client_disconnect(NcmMpdClient *client) {
    if (client == NULL) {
        return;
    }

    ncm_mpd_connection_disconnect(&client->connection);
    client->fd = -1;
    client->idle = false;
    client->command_list_active = false;
    return;
}

bool
ncm_mpd_client_send_password(NcmMpdClient *client, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }

    if (!ncm_mpd_connection_send_password(
            &client->connection,
            ncm_mpd_client_buffer_cstr(&client->password))) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_idle(NcmMpdClient *client, NcmError *error) {
    if (!ncm_mpd_client_require_connected(client, error)) {
        return false;
    }

    if (!client->idle) {
        if (!ncm_mpd_connection_send_idle(&client->connection,
                                          (enum mpd_idle)0)) {
            ncm_mpd_client_copy_connection_error(client, error);
            return false;
        }
        client->idle = true;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_noidle(NcmMpdClient *client, int32 *flags,
                      NcmError *error) {
    enum mpd_idle events;

    if (!ncm_mpd_client_require_connected(client, error)) {
        return false;
    }

    events = (enum mpd_idle)0;
    if (client->idle) {
        if (!ncm_mpd_connection_noidle(&client->connection)) {
            ncm_mpd_client_copy_connection_error(client, error);
            return false;
        }
        client->idle = false;
        if (!ncm_mpd_connection_recv_idle(&client->connection,
                                          true, &events)) {
            ncm_mpd_client_copy_connection_error(client, error);
            return false;
        }
    }

    if (flags) {
        *flags = (int32)events;
    }
    ncm_error_clear(error);
    return true;
}

enum mpd_error
ncm_mpd_client_error_code(NcmMpdClient *client) {
    if (client == NULL) {
        return MPD_ERROR_SUCCESS;
    }

    return ncm_mpd_connection_error_code(&client->connection);
}

enum mpd_server_error
ncm_mpd_client_server_error_code(NcmMpdClient *client) {
    if (client == NULL) {
        return (enum mpd_server_error)0;
    }

    return ncm_mpd_connection_server_error_code(&client->connection);
}

bool
ncm_mpd_client_error_clearable(NcmMpdClient *client) {
    if (client == NULL) {
        return false;
    }

    return ncm_mpd_connection_error_clearable(&client->connection);
}

char *
ncm_mpd_client_error_message(NcmMpdClient *client) {
    if (client == NULL) {
        return "";
    }

    return ncm_mpd_connection_error(&client->connection);
}

#define NCM_CLIENT_CALL_NOARGS(NAME, CONN_CALL) \
bool \
NAME(NcmMpdClient *client, NcmError *error) { \
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) { \
        return false; \
    } \
    if (!CONN_CALL(&client->connection)) { \
        ncm_mpd_client_copy_connection_error(client, error); \
        return false; \
    } \
    ncm_error_clear(error); \
    return true; \
}

NCM_CLIENT_CALL_NOARGS(ncm_mpd_client_play, ncm_mpd_connection_play)
NCM_CLIENT_CALL_NOARGS(ncm_mpd_client_toggle_pause,
                       ncm_mpd_connection_toggle_pause)
NCM_CLIENT_CALL_NOARGS(ncm_mpd_client_stop, ncm_mpd_connection_stop)
NCM_CLIENT_CALL_NOARGS(ncm_mpd_client_next, ncm_mpd_connection_next)
NCM_CLIENT_CALL_NOARGS(ncm_mpd_client_previous, ncm_mpd_connection_previous)
NCM_CLIENT_CALL_NOARGS(ncm_mpd_client_shuffle, ncm_mpd_connection_shuffle)
NCM_CLIENT_CALL_NOARGS(ncm_mpd_client_clear_queue,
                       ncm_mpd_connection_clear_queue)

#undef NCM_CLIENT_CALL_NOARGS

bool
ncm_mpd_client_get_stats(NcmMpdClient *client, NcmMpdStats *stats,
                         NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_stats(&client->connection, stats)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_status(NcmMpdClient *client, NcmMpdStatus *status,
                          NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_status(&client->connection, status)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_update_directory(NcmMpdClient *client, char *path,
                                int32 *id, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_update_database(&client->connection, path, id)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_play_pos(NcmMpdClient *client, int32 pos, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_play_pos(&client->connection, pos)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_play_id(NcmMpdClient *client, int32 id, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_play_id(&client->connection, id)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_move(NcmMpdClient *client, int32 from, int32 to,
                    NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_move(&client->connection, from, to,
                                 client->command_list_active)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_swap(NcmMpdClient *client, int32 from, int32 to,
                    NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_swap(&client->connection, from, to,
                                 client->command_list_active)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_seek_pos(NcmMpdClient *client, int32 pos, int32 seconds,
                        NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_seek_pos(&client->connection, pos, seconds)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_shuffle_range(NcmMpdClient *client, int32 start,
                             int32 end, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_shuffle_range(&client->connection, start, end)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

#define NCM_CLIENT_LIST_CALL(NAME, LIST_TYPE, CONN_CALL) \
bool \
NAME(NcmMpdClient *client, LIST_TYPE *list, NcmError *error) { \
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) { \
        return false; \
    } \
    if (!CONN_CALL(&client->connection, list)) { \
        ncm_mpd_client_copy_connection_error(client, error); \
        return false; \
    } \
    ncm_error_clear(error); \
    return true; \
}

NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_supported_extensions,
                     NcmMpdStringList,
                     ncm_mpd_connection_get_supported_extensions)
NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_playlists,
                     NcmMpdPlaylistList,
                     ncm_mpd_connection_get_playlists)
NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_outputs,
                     NcmMpdOutputList,
                     ncm_mpd_connection_get_outputs)
NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_url_handlers,
                     NcmMpdStringList,
                     ncm_mpd_connection_get_url_handlers)
NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_tag_types,
                     NcmMpdStringList,
                     ncm_mpd_connection_get_tag_types)

#undef NCM_CLIENT_LIST_CALL

bool
ncm_mpd_client_get_queue(NcmMpdClient *client,
                         NcmMpdSongList *songs, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_queue(&client->connection, songs)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_queue_changes(NcmMpdClient *client, int32 version,
                                 NcmMpdSongList *songs, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_queue_changes(&client->connection,
                                              version, songs)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_current_song(NcmMpdClient *client, NcmSong *song,
                                NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_current_song(&client->connection, song)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_playlist_content(NcmMpdClient *client, char *path,
                                    NcmMpdSongList *songs,
                                    NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_playlist_content(&client->connection,
                                                 path, songs)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_playlist_content_no_info(NcmMpdClient *client,
                                            char *path,
                                            NcmMpdSongList *songs,
                                            NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_playlist_content_no_info(
            &client->connection, path, songs)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

#define NCM_CLIENT_MODE_CALL(NAME, CONN_CALL) \
bool \
NAME(NcmMpdClient *client, bool mode, NcmError *error) { \
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) { \
        return false; \
    } \
    if (!CONN_CALL(&client->connection, mode)) { \
        ncm_mpd_client_copy_connection_error(client, error); \
        return false; \
    } \
    ncm_error_clear(error); \
    return true; \
}

NCM_CLIENT_MODE_CALL(ncm_mpd_client_set_repeat,
                     ncm_mpd_connection_set_repeat)
NCM_CLIENT_MODE_CALL(ncm_mpd_client_set_random,
                     ncm_mpd_connection_set_random)
NCM_CLIENT_MODE_CALL(ncm_mpd_client_set_single,
                     ncm_mpd_connection_set_single)
NCM_CLIENT_MODE_CALL(ncm_mpd_client_set_consume,
                     ncm_mpd_connection_set_consume)

#undef NCM_CLIENT_MODE_CALL

bool
ncm_mpd_client_set_crossfade(NcmMpdClient *client, int32 seconds,
                             NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_set_crossfade(&client->connection, seconds)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_set_volume(NcmMpdClient *client, int32 volume, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_set_volume(&client->connection, volume)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_change_volume(NcmMpdClient *client, int32 change,
                             NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_change_volume(&client->connection, change)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_replay_gain_mode(NcmMpdClient *client,
                                    enum NcmMpdReplayGainMode *mode,
                                    NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_replay_gain_mode(&client->connection, mode)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_set_replay_gain_mode(NcmMpdClient *client,
                                    enum NcmMpdReplayGainMode mode,
                                    NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_set_replay_gain_mode(&client->connection,
                                                 mode)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_set_priority_id(NcmMpdClient *client, int32 id,
                               int32 priority, NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_set_priority_id(&client->connection, id,
                                            priority,
                                            client->command_list_active)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_set_priority_song(NcmMpdClient *client, NcmSong *song,
                                 int32 priority, NcmError *error) {
    if (song == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD song"));
        return false;
    }

    return ncm_mpd_client_set_priority_id(client, ncm_song_id(song),
                                          priority, error);
}

bool
ncm_mpd_client_add_song(NcmMpdClient *client, char *path, int32 pos,
                        int32 *id, NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_add_song(&client->connection, path, pos,
                                     client->command_list_active, id)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_add_song_value(NcmMpdClient *client, NcmSong *song,
                              int32 pos, int32 *id, NcmError *error) {
    NcmStringView uri;

    if (song == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD song"));
        return false;
    }
    if (!ncm_song_uri_view(song, 0, &uri)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("MPD song has no URI"));
        return false;
    }

    return ncm_mpd_client_add_song(client, uri.data, pos, id, error);
}

bool
ncm_mpd_client_add_song_list(NcmMpdClient *client,
                             NcmMpdSongList *songs, int32 pos,
                             NcmError *error) {
    bool started;
    int32 insert_pos;

    if (songs == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD song list"));
        return false;
    }
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (songs->count <= 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("empty MPD song list"));
        return false;
    }

    started = false;
    if (!client->command_list_active) {
        if (!ncm_mpd_client_start_command_list(client, error)) {
            return false;
        }
        started = true;
    }

    for (int32 i = 0; i < songs->count; i += 1) {
        insert_pos = -1;
        if (pos >= 0) {
            insert_pos = pos + i;
        }
        if (!ncm_mpd_client_add_song_value(client, &songs->items[i],
                                           insert_pos, NULL, error)) {
            if (started) {
                client->command_list_active = false;
            }
            return false;
        }
    }

    if (started) {
        return ncm_mpd_client_commit_command_list(client, error);
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_add(NcmMpdClient *client, char *path, bool *added,
                   NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_add(&client->connection, path,
                                client->command_list_active, added)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_delete(NcmMpdClient *client, int32 pos, NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_delete(&client->connection, pos,
                                   client->command_list_active)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_start_command_list(NcmMpdClient *client, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_start_command_list(&client->connection)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    client->command_list_active = true;
    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_commit_command_list(NcmMpdClient *client, NcmError *error) {
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (!client->command_list_active) {
        ncm_error_set(error, (int32)MPD_ERROR_STATE,
                      STRLIT_ARGS("No active MPD command list"));
        return false;
    }
    if (!ncm_mpd_connection_commit_command_list(&client->connection)) {
        client->command_list_active = false;
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    client->command_list_active = false;
    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_delete_playlist(NcmMpdClient *client, char *name,
                               NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_delete_playlist(&client->connection, name)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_load_playlist(NcmMpdClient *client, char *name,
                             bool *loaded, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_load_playlist(&client->connection, name,
                                          loaded)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_save_playlist(NcmMpdClient *client, char *name,
                             NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_save_playlist(&client->connection, name)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_clear_playlist(NcmMpdClient *client, char *name,
                              NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_clear_playlist(&client->connection, name)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_add_to_playlist(NcmMpdClient *client, char *playlist,
                               char *path, NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_add_to_playlist(&client->connection, playlist,
                                            path,
                                            client->command_list_active)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_add_song_to_playlist(NcmMpdClient *client,
                                    char *playlist, NcmSong *song,
                                    NcmError *error) {
    NcmStringView uri;

    if (song == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD song"));
        return false;
    }
    if (!ncm_song_uri_view(song, 0, &uri)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("MPD song has no URI"));
        return false;
    }

    return ncm_mpd_client_add_to_playlist(client, playlist, uri.data,
                                          error);
}

bool
ncm_mpd_client_playlist_move(NcmMpdClient *client, char *playlist,
                             int32 from, int32 to, NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_playlist_move(&client->connection, playlist,
                                          from, to,
                                          client->command_list_active)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_playlist_delete(NcmMpdClient *client, char *playlist,
                               int32 pos, NcmError *error) {
    if (!ncm_mpd_client_prechecks(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_playlist_delete(&client->connection, playlist,
                                            pos,
                                            client->command_list_active)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_rename_playlist(NcmMpdClient *client, char *from,
                               char *to, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_rename_playlist(&client->connection,
                                            from, to)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_start_search(NcmMpdClient *client, bool exact_match,
                            NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_start_search_songs(&client->connection,
                                               exact_match)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_add_search_tag(NcmMpdClient *client, enum mpd_tag_type tag,
                              char *value, NcmError *error) {
    if (!ncm_mpd_client_require_connected(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_add_search_tag(&client->connection, tag,
                                           value)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_add_search_any(NcmMpdClient *client, char *value,
                              NcmError *error) {
    if (!ncm_mpd_client_require_connected(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_add_search_any(&client->connection, value)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_add_search_uri(NcmMpdClient *client, char *value,
                              NcmError *error) {
    if (!ncm_mpd_client_require_connected(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_add_search_uri(&client->connection, value)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_commit_search_songs(NcmMpdClient *client,
                                   NcmMpdSongList *songs,
                                   NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_commit_search_songs(&client->connection,
                                                songs)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_list(NcmMpdClient *client, enum mpd_tag_type tag,
                        NcmMpdStringList *strings, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_list_tag_values(&client->connection, tag,
                                            strings)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_directory(NcmMpdClient *client, char *path,
                             NcmMpdItemList *items, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_directory(&client->connection, path,
                                          items)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_directory_recursive(NcmMpdClient *client, char *path,
                                       NcmMpdSongList *songs,
                                       NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_list_all_songs(&client->connection, path,
                                           songs)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_songs(NcmMpdClient *client, char *path,
                         NcmMpdSongList *songs, NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_get_directory_songs(&client->connection,
                                                path, songs)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_get_directory_entries(NcmMpdClient *client, char *path,
                                     NcmMpdItemArray *items,
                                     NcmError *error) {
    NcmMpdItemList list;
    bool ok;

    if (items == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD item array"));
        return false;
    }

    ncm_mpd_item_list_init(&list);
    if ((ok = ncm_mpd_client_get_directory(client, path, &list, error))) {
        ok = ncm_mpd_item_list_to_item_array(&list, items);
    }
    ncm_mpd_item_list_destroy(&list);
    return ok;
}

bool
ncm_mpd_client_get_directory_list(NcmMpdClient *client, char *path,
                                  NcmDirectoryArray *directories,
                                  NcmError *error) {
    NcmMpdItemList items;
    bool ok;

    if (directories == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing MPD directory array"));
        return false;
    }

    ncm_mpd_item_list_init(&items);
    if ((ok = ncm_mpd_client_get_directory(client, path, &items, error))) {
        ok = ncm_mpd_item_list_to_directory_array(&items, directories);
    }
    ncm_mpd_item_list_destroy(&items);
    return ok;
}

bool
ncm_mpd_client_enable_output(NcmMpdClient *client, int32 id,
                             NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_enable_output(&client->connection, id)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_disable_output(NcmMpdClient *client, int32 id,
                              NcmError *error) {
    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        return false;
    }
    if (!ncm_mpd_connection_disable_output(&client->connection, id)) {
        ncm_mpd_client_copy_connection_error(client, error);
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
ncm_mpd_client_add_random_tag(NcmMpdClient *client,
                              enum mpd_tag_type tag,
                              int32 number,
                              NcmRandom *random,
                              NcmError *error) {
    NcmMpdStringList tags;
    NcmMpdSongList songs;
    bool ok = false;

    if (number < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("negative random count"));
        return false;
    }

    ncm_mpd_string_list_init(&tags);
    ncm_mpd_song_list_init(&songs);
    if (!ncm_mpd_client_get_list(client, tag, &tags, error)) {
        goto cleanup;
    }
    if (number > tags.count) {
        ok = false;
        goto cleanup;
    }

    ncm_random_shuffle(random, tags.items, tags.count, SIZEOF(*tags.items));
    for (int32 i = 0; i < number; i += 1) {
        if (!ncm_mpd_client_start_search(client, true, error)) {
            goto cleanup;
        }
        if (!ncm_mpd_client_add_search_tag(client, tag,
                                           tags.items[i].value, error)) {
            goto cleanup;
        }
        if (!ncm_mpd_client_commit_search_songs(client, &songs, error)) {
            goto cleanup;
        }
        if (!ncm_mpd_client_start_command_list(client, error)) {
            goto cleanup;
        }
        for (int32 j = 0; j < songs.count; j += 1) {
            if (!ncm_mpd_client_add_song(client, songs.items[j].uri,
                                         -1, NULL, error)) {
                goto cleanup;
            }
        }
        if (!ncm_mpd_client_commit_command_list(client, error)) {
            goto cleanup;
        }
        ncm_mpd_song_list_clear(&songs);
    }

    ok = true;

cleanup:
    if (client && client->command_list_active) {
        client->command_list_active = false;
    }
    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_string_list_destroy(&tags);
    return ok;
}

bool
ncm_mpd_client_add_random_songs(NcmMpdClient *client,
                                int32 number,
                                char *exclude_pattern,
                                int32 exclude_pattern_len,
                                NcmRandom *random,
                                NcmError *error) {
    NcmMpdStringList files;
    NcmRegex regex;
    bool have_regex;
    int32 added;
    bool ok = false;

    if (number < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("negative random count"));
        return false;
    }
    if (exclude_pattern_len < 0) {
        exclude_pattern_len = optional_strlen32(exclude_pattern);
    }

    ncm_mpd_string_list_init(&files);
    ncm_regex_init(&regex);
    have_regex = false;

    if (!ncm_mpd_client_prechecks_no_commands(client, error)) {
        goto cleanup;
    }
    if (!ncm_mpd_connection_list_all_song_uris(&client->connection,
                                               "/", &files)) {
        ncm_mpd_client_copy_connection_error(client, error);
        goto cleanup;
    }
    if (number > files.count) {
        goto cleanup;
    }

    if (exclude_pattern && (exclude_pattern_len > 0)) {
        if (!ncm_regex_compile(&regex, exclude_pattern,
                               exclude_pattern_len,
                               NCM_REGEX_EXTENDED | NCM_REGEX_NOSUB,
                               error)) {
            goto cleanup;
        }
        have_regex = true;
    }

    ncm_random_shuffle(random, files.items, files.count, SIZEOF(*files.items));
    if (!ncm_mpd_client_start_command_list(client, error)) {
        goto cleanup;
    }

    added = 0;
    for (int32 i = 0; (i < files.count) && (added < number); i += 1) {
        if (have_regex
            && ncm_regex_search(&regex, files.items[i].value,
                                files.items[i].value_len)) {
            continue;
        }
        if (!ncm_mpd_client_add_song(client, files.items[i].value,
                                     -1, NULL, error)) {
            goto cleanup;
        }
        added += 1;
    }
    if (!ncm_mpd_client_commit_command_list(client, error)) {
        goto cleanup;
    }

    ok = added == number;

cleanup:
    if (client && client->command_list_active) {
        client->command_list_active = false;
    }
    ncm_regex_destroy(&regex);
    ncm_mpd_string_list_destroy(&files);
    return ok;
}

#endif /* NCM_MPD_CLIENT_C */
