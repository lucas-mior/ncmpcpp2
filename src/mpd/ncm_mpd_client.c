#if !defined(NCM_MPD_CLIENT_C)
#define NCM_MPD_CLIENT_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "ncmpcpp2_mpd.h"

#define NCM_CLIENT_TRY(expression) \
    do { \
        int32 status_ = (expression); \
        if (status_ < 0) { \
            return status_; \
        } \
    } while (0)

#define NCM_CLIENT_TRY_MPD(client_, expression, error_) \
    do { \
        int32 status_ = (expression); \
        if (status_ < 0) { \
            ncm_mpd_client_copy_connection_error((client_), (error_)); \
            return status_; \
        } \
    } while (0)

static void
ncm_mpd_client_set_buffer(StrBuilder *buffer, char *string, int32 string_len) {
    sb_clear(buffer);
    SB_APPEND(buffer, string, string_len);
    return;
}

static void
ncm_mpd_client_copy_connection_error(MpdClient *client,
                                     NcmError *ncm_error) {
    enum NcmMpdError code;
    char *message;
    int32 message_len;

    ASSERT(client != NULL);

    message = ncm_mpd_connection_error(&client->connection);
    message_len = optional_strlen32(message);
    code = ncm_mpd_connection_error_code(&client->connection);
    ncm_error_set(ncm_error, (int32)code, message, message_len);
    return;
}

static int32
ncm_mpd_client_require_connected(MpdClient *client, NcmError *ncm_error) {
    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }
    if (!ncm_mpd_connection_is_connected(&client->connection)) {
        return ncm_error_set_status(ncm_error, -NCM_ERROR_INVALID_STATE,
                                    STRLIT("No active MPD connection"));
    }

    return 0;
}

static int32
ncm_mpd_client_noidle_connected(MpdClient *client, int32 *flags,
                                NcmError *ncm_error) {
    int32 events = 0;

    ASSERT(client != NULL);

    if (client->idle) {
        NCM_CLIENT_TRY_MPD(client,
                           ncm_mpd_connection_noidle(&client->connection),
                           ncm_error);
        client->idle = false;
        NCM_CLIENT_TRY_MPD(client,
                           ncm_mpd_connection_recv_idle(&client->connection,
                                                        true,
                                                        &events),
                           ncm_error);
    }

    if (flags != NULL) {
        *flags = (int32)events;
    }
    return ncm_error_ok(ncm_error);
}

static int32
ncm_mpd_client_prechecks_connected(MpdClient *client, NcmError *ncm_error) {
    int32 flags = 0;
    int32 status;

    if ((status = ncm_mpd_client_noidle_connected(client, &flags,
                                                   ncm_error)) < 0) {
        return status;
    }
    if ((flags != 0) && (client->noidle_callback != NULL)) {
        client->noidle_callback(flags, client->noidle_user);
    }

    return 0;
}

static int32
ncm_mpd_client_prechecks(MpdClient *client, NcmError *ncm_error) {
    int32 status;

    if ((status = ncm_mpd_client_require_connected(client, ncm_error)) < 0) {
        return status;
    }

    return ncm_mpd_client_prechecks_connected(client, ncm_error);
}

static int32
ncm_mpd_client_prechecks_no_commands(MpdClient *client,
                                      NcmError *ncm_error) {
    int32 status;

    if ((status = ncm_mpd_client_require_connected(client, ncm_error)) < 0) {
        return status;
    }
    if (client->command_list_active) {
        return ncm_error_set_status(ncm_error, -NCM_ERROR_INVALID_STATE,
                                    STRLIT("MPD command list is already"
                                           " active"));
    }

    return ncm_mpd_client_prechecks_connected(client, ncm_error);
}

static int32
ncm_mpd_client_add_song_ready(MpdClient *client, char *path, int32 pos,
                              int32 *id, NcmError *ncm_error) {
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_add_song(&client->connection,
                                                   path,
                                                   pos,
                                                   client->command_list_active,
                                                   id),
                       ncm_error);
    return ncm_error_ok(ncm_error);
}

static int32
ncm_mpd_client_start_command_list_ready(MpdClient *client,
                                        NcmError *ncm_error) {
    ASSERT(!client->command_list_active);

    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_start_command_list(&client->connection),
        ncm_error);
    client->command_list_active = true;
    return ncm_error_ok(ncm_error);
}

static int32
ncm_mpd_client_commit_command_list_ready(MpdClient *client,
                                         NcmError *ncm_error) {
    int32 status;

    ASSERT(client->command_list_active);

    status = ncm_mpd_connection_commit_command_list(&client->connection);
    client->command_list_active = false;
    if (status < 0) {
        ncm_mpd_client_copy_connection_error(client, ncm_error);
        return status;
    }

    return ncm_error_ok(ncm_error);
}

static void
ncm_mpd_client_disconnect_ready(MpdClient *client) {
    ncm_mpd_connection_disconnect(&client->connection);
    client->fd = -1;
    client->idle = false;
    client->command_list_active = false;
    return;
}

void
ncm_mpd_client_init(MpdClient *client) {
    if (client == NULL) {
        return;
    }

    client->connection = (MpdConnection){0};
    client->host = (StrBuilder){0};
    client->password = (StrBuilder){0};
    ncm_mpd_client_set_buffer(&client->host, STRLIT("localhost"));
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
ncm_mpd_client_destroy(MpdClient *client) {
    if (client == NULL) {
        return;
    }

    ncm_mpd_client_disconnect_ready(client);
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
ncm_mpd_client_hostname(MpdClient *client) {
    if (client == NULL) {
        return "";
    }

    return sb_opt_cstr(&client->host);
}

bool
ncm_mpd_client_is_connected(MpdClient *client) {
    if (client == NULL) {
        return false;
    }

    return ncm_mpd_connection_is_connected(&client->connection);
}

int32
ncm_mpd_client_version(MpdClient *client) {
    if (client == NULL) {
        return 0;
    }

    return ncm_mpd_connection_version(&client->connection);
}

int32
ncm_mpd_client_fd(MpdClient *client) {
    if (client == NULL) {
        return -1;
    }

    return ncm_mpd_connection_fd(&client->connection);
}

void
ncm_mpd_client_set_noidle_callback(MpdClient *client,
                                   NcmMpdNoidleCallback *callback, void *user) {
    if (client == NULL) {
        return;
    }

    client->noidle_callback = callback;
    client->noidle_user = user;
    return;
}

int32
ncm_mpd_client_set_hostname(MpdClient *client, char *host, int32 host_len,
                            NcmError *ncm_error) {
    int32 at;

    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
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

    return ncm_error_ok(ncm_error);
}

void
ncm_mpd_client_set_port(MpdClient *client, uint16 port) {
    if (client == NULL) {
        return;
    }

    client->port = port;
    return;
}

int32
ncm_mpd_client_set_password(MpdClient *client,
                            char *password, int32 password_len,
                            NcmError *ncm_error) {
    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }
    if (password_len < 0) {
        password_len = optional_strlen32(password);
    }

    ncm_mpd_client_set_buffer(&client->password, password, password_len);
    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_set_timeout_ms(MpdClient *client,
                              int32 timeout_ms, NcmError *ncm_error) {
    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }

    client->timeout_ms = timeout_ms;
    if (ncm_mpd_connection_is_connected(&client->connection)) {
        NCM_CLIENT_TRY_MPD(client,
                           ncm_mpd_connection_set_timeout(&client->connection,
                                                          timeout_ms),
                           ncm_error);
    }

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_connect(MpdClient *client, NcmError *ncm_error) {
    char *password;
    int32 status;

    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }

    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_connect(&client->connection,
                                                  sb_opt_cstr(&client->host),
                                                  client->port,
                                                  client->timeout_ms),
                       ncm_error);

    client->fd = ncm_mpd_connection_fd(&client->connection);
    client->idle = false;
    client->command_list_active = false;
    if (client->password.len > 0) {
        password = sb_opt_cstr(&client->password);
        status = ncm_mpd_connection_send_password(&client->connection,
                                                  password);
        if (status < 0) {
            ncm_mpd_client_copy_connection_error(client, ncm_error);
            ncm_mpd_client_disconnect_ready(client);
            return status;
        }
    }

    return ncm_error_ok(ncm_error);
}

void
ncm_mpd_client_disconnect(MpdClient *client) {
    if (client == NULL) {
        return;
    }

    ncm_mpd_client_disconnect_ready(client);
    return;
}

int32
ncm_mpd_client_send_password(MpdClient *client, NcmError *ncm_error) {
    char *password;

    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    password = sb_opt_cstr(&client->password);
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_send_password(&client->connection,
                                                        password),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_idle(MpdClient *client, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_require_connected(client, ncm_error));

    if (!client->idle) {
        NCM_CLIENT_TRY_MPD(client,
                           ncm_mpd_connection_send_idle(&client->connection,
                                                        0),
                           ncm_error);
        client->idle = true;
    }

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_noidle(MpdClient *client, int32 *flags, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_require_connected(client, ncm_error));
    return ncm_mpd_client_noidle_connected(client, flags, ncm_error);
}

enum NcmMpdError
ncm_mpd_client_error_code(MpdClient *client) {
    if (client == NULL) {
        return NCM_MPD_ERROR_SUCCESS;
    }

    return ncm_mpd_connection_error_code(&client->connection);
}

enum NcmMpdServerError
ncm_mpd_client_server_error_code(MpdClient *client) {
    if (client == NULL) {
        return NCM_MPD_SERVER_ERROR_NONE;
    }

    return ncm_mpd_connection_server_error_code(&client->connection);
}

bool
ncm_mpd_client_error_is_clearable(MpdClient *client) {
    if (client == NULL) {
        return false;
    }

    return ncm_mpd_connection_error_is_clearable(&client->connection);
}

char *
ncm_mpd_client_error_message(MpdClient *client) {
    if (client == NULL) {
        return "";
    }

    return ncm_mpd_connection_error(&client->connection);
}

#define NCM_CLIENT_CALL_NOARGS(NAME, CONN_CALL) \
int32 \
NAME(MpdClient *client, NcmError *ncm_error) { \
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error)); \
    NCM_CLIENT_TRY_MPD(client, CONN_CALL(&client->connection), ncm_error); \
    return ncm_error_ok(ncm_error); \
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

int32
ncm_mpd_client_get_stats(MpdClient *client, NcmMpdStats *stats,
                         NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_get_stats(&client->connection, stats),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_status(MpdClient *client, NcmMpdStatus *status,
                          NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_get_status(&client->connection,
                                                     status),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_update_directory(MpdClient *client, char *path,
                                int32 *id, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_update_database(&client->connection,
                                                          path,
                                                          id),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_play_pos(MpdClient *client, int32 pos, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_play_pos(&client->connection, pos),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_play_id(MpdClient *client, int32 id, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_play_id(&client->connection, id),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_move(MpdClient *client, int32 from, int32 to,
                    NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_move(&client->connection,
                                               from,
                                               to,
                                               client->command_list_active),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_swap(MpdClient *client, int32 from, int32 to,
                    NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_swap(&client->connection,
                                               from,
                                               to,
                                               client->command_list_active),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_seek_pos(MpdClient *client, int32 pos, int32 seconds,
                        NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_seek_pos(&client->connection,
                                                   pos,
                                                   seconds),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_shuffle_range(MpdClient *client, int32 start, int32 end,
                             NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_shuffle_range(&client->connection,
                                                        start,
                                                        end),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

#define NCM_CLIENT_LIST_CALL(NAME, LIST_TYPE, CONN_CALL) \
int32 \
NAME(MpdClient *client, LIST_TYPE *list, NcmError *ncm_error) { \
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error)); \
    NCM_CLIENT_TRY_MPD(client, \
                       CONN_CALL(&client->connection, list), \
                       ncm_error); \
    return ncm_error_ok(ncm_error); \
}

NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_supported_extensions,
                     StringViewList,
                     ncm_mpd_connection_get_supported_extensions)
NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_playlists,
                     NcmMpdPlaylistList, ncm_mpd_connection_get_playlists)
NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_outputs,
                     NcmMpdOutputList, ncm_mpd_connection_get_outputs)
NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_url_handlers,
                     StringViewList, ncm_mpd_connection_get_url_handlers)
NCM_CLIENT_LIST_CALL(ncm_mpd_client_get_tag_types,
                     StringViewList, ncm_mpd_connection_get_tag_types)

#undef NCM_CLIENT_LIST_CALL

int32
ncm_mpd_client_get_queue(MpdClient *client,
                         NcmMpdSongList *songs, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_get_queue(&client->connection, songs),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_queue_changes(MpdClient *client, int32 version,
                                 NcmMpdSongList *songs, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_get_queue_changes(&client->connection,
                                                            version,
                                                            songs),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_current_song(MpdClient *client, NcmSong *song,
                                NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_get_current_song(&client->connection,
                                                           song),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_playlist_content(MpdClient *client, char *path,
                                    NcmMpdSongList *songs,
                                    NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_get_playlist_content(&client->connection,
                                                path, songs),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_playlist_content_no_info(MpdClient *client, char *path,
                                            NcmMpdSongList *songs,
                                            NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_get_playlist_content_no_info(&client->connection,
                                                        path, songs),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

#define NCM_CLIENT_MODE_CALL(NAME, CONN_CALL) \
int32 \
NAME(MpdClient *client, bool mode, NcmError *ncm_error) { \
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error)); \
    NCM_CLIENT_TRY_MPD(client, \
                       CONN_CALL(&client->connection, mode), \
                       ncm_error); \
    return ncm_error_ok(ncm_error); \
}

NCM_CLIENT_MODE_CALL(ncm_mpd_client_set_repeat, ncm_mpd_connection_set_repeat)
NCM_CLIENT_MODE_CALL(ncm_mpd_client_set_random, ncm_mpd_connection_set_random)
NCM_CLIENT_MODE_CALL(ncm_mpd_client_set_single, ncm_mpd_connection_set_single)
NCM_CLIENT_MODE_CALL(ncm_mpd_client_set_consume, ncm_mpd_connection_set_consume)

#undef NCM_CLIENT_MODE_CALL

int32
ncm_mpd_client_set_crossfade(MpdClient *client, int32 seconds,
                             NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_set_crossfade(&client->connection,
                                                        seconds),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_set_volume(MpdClient *client, int32 volume,
                          NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_set_volume(&client->connection,
                                                     volume),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_change_volume(MpdClient *client, int32 change,
                             NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_change_volume(&client->connection,
                                                        change),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_replay_gain_mode(MpdClient *client,
                                    enum NcmMpdReplayGainMode *mode,
                                    NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_get_replay_gain_mode(&client->connection, mode),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_set_replay_gain_mode(MpdClient *client,
                                    enum NcmMpdReplayGainMode mode,
                                    NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_set_replay_gain_mode(&client->connection, mode),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_set_priority_song(MpdClient *client, NcmSong *song,
                                 int32 priority, NcmError *ncm_error) {
    if (song == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD song"));
    }

    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_set_priority_id(&client->connection,
                                           ncm_song_id(song), priority,
                                           client->command_list_active),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_add_song_value(MpdClient *client, NcmSong *song,
                              int32 pos, int32 *id, NcmError *ncm_error) {
    StringView uri;

    if (song == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD song"));
    }
    if (!ncm_song_has_uri_view(song, 0, &uri)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("MPD song has no URI"));
    }

    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    return ncm_mpd_client_add_song_ready(client, uri.data, pos, id, ncm_error);
}

int32
ncm_mpd_client_add_song_list(MpdClient *client,
                             NcmMpdSongList *songs, int32 pos,
                             NcmError *ncm_error) {
    bool started;
    int32 insert_pos;
    int32 status;

    if (songs == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD song list"));
    }
    if (songs->count <= 0) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("empty MPD song list"));
    }
    if ((status = ncm_mpd_client_prechecks(client, ncm_error)) < 0) {
        return status;
    }

    started = false;
    if (!client->command_list_active) {
        if ((status = ncm_mpd_client_start_command_list_ready(client,
                                                              ncm_error)) < 0) {
            return status;
        }
        started = true;
    }

    for (int32 i = 0; i < songs->count; i += 1) {
        StringView uri;

        if (!ncm_song_has_uri_view(&songs->items[i], 0, &uri)) {
            if (started) {
                client->command_list_active = false;
            }
            return ncm_error_set_status(ncm_error, -EINVAL,
                                        STRLIT("MPD song has no URI"));
        }

        insert_pos = -1;
        if (pos >= 0) {
            insert_pos = pos + i;
        }
        ncm_mpd_client_add_song_ready(client, uri.data, insert_pos, NULL,
                                      ncm_error);
    }

    if (started) {
        return ncm_mpd_client_commit_command_list_ready(client, ncm_error);
    }

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_add(MpdClient *client, char *path, bool *added,
                   NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_add(&client->connection,
                                              path,
                                              client->command_list_active,
                                              added),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_delete(MpdClient *client, int32 pos, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_delete(&client->connection,
                                                 pos,
                                                 client->command_list_active),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_start_command_list(MpdClient *client, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    return ncm_mpd_client_start_command_list_ready(client, ncm_error);
}

int32
ncm_mpd_client_commit_command_list(MpdClient *client, NcmError *ncm_error) {
    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }
    if (!client->command_list_active) {
        return ncm_error_set_status(ncm_error, -NCM_ERROR_INVALID_STATE,
                                    STRLIT("No active MPD command list"));
    }

    return ncm_mpd_client_commit_command_list_ready(client, ncm_error);
}

int32
ncm_mpd_client_delete_playlist(MpdClient *client, char *name,
                               NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_delete_playlist(&client->connection,
                                                          name),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_load_playlist(MpdClient *client, char *name,
                             bool *loaded, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_load_playlist(&client->connection,
                                                        name,
                                                        loaded),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_save_playlist(MpdClient *client, char *name,
                             NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_save_playlist(&client->connection,
                                                        name),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_clear_playlist(MpdClient *client, char *name,
                              NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_clear_playlist(&client->connection,
                                                         name),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_add_song_to_playlist(MpdClient *client,
                                    char *playlist, NcmSong *song,
                                    NcmError *ncm_error) {
    StringView uri;

    if (song == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD song"));
    }
    if (!ncm_song_has_uri_view(song, 0, &uri)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("MPD song has no URI"));
    }

    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_add_to_playlist(&client->connection,
                                           playlist, uri.data,
                                           client->command_list_active),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_playlist_move(MpdClient *client, char *playlist,
                             int32 from, int32 to, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_playlist_move(&client->connection,
                                         playlist, from, to,
                                         client->command_list_active),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_playlist_delete(MpdClient *client, char *playlist,
                               int32 pos, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_playlist_delete(&client->connection,
                                           playlist, pos,
                                           client->command_list_active),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_rename_playlist(MpdClient *client, char *from,
                               char *to, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_rename_playlist(&client->connection,
                                                          from,
                                                          to),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_start_search(MpdClient *client, bool exact_match,
                            NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_start_search_songs(&client->connection, exact_match),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_add_search_tag(MpdClient *client, enum NcmTagType tag,
                              char *value, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_require_connected(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_add_search_tag(&client->connection,
                                                         tag,
                                                         value),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_add_search_any(MpdClient *client, char *value,
                              NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_require_connected(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_add_search_any(&client->connection,
                                                         value),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_add_search_uri(MpdClient *client, char *value,
                              NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_require_connected(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_add_search_uri(&client->connection,
                                                         value),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_commit_search_songs(MpdClient *client, NcmMpdSongList *songs,
                                   NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_commit_search_songs(&client->connection, songs),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_list(MpdClient *client, enum NcmTagType tag,
                        StringViewList *strings, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_list_tag_values(&client->connection,
                                                          tag,
                                                          strings),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_directory_recursive(MpdClient *client, char *path,
                                       NcmMpdSongList *songs,
                                       NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_list_all_songs(&client->connection,
                                                         path,
                                                         songs),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_songs(MpdClient *client, char *path,
                         NcmMpdSongList *songs, NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
        ncm_mpd_connection_get_directory_songs(&client->connection,
                                              path, songs),
        ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_get_directory_entries(MpdClient *client, char *path,
                                     NcmMpdItemArray *items,
                                     NcmError *ncm_error) {
    NcmMpdItemList list;
    int32 status;

    if (items == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD item array"));
    }
    if ((status = ncm_mpd_client_prechecks_no_commands(client,
                                                       ncm_error)) < 0) {
        return status;
    }

    list = (NcmMpdItemList){0};
    status = ncm_mpd_connection_get_directory(&client->connection, path, &list);
    if (status < 0) {
        ncm_mpd_client_copy_connection_error(client, ncm_error);
    } else {
        ncm_mpd_item_list_to_item_array(&list, items);
        status = ncm_error_ok(ncm_error);
    }
    ncm_mpd_item_list_destroy(&list);
    return status;
}

int32
ncm_mpd_client_get_directory_list(MpdClient *client, char *path,
                                  NcmDirectoryArray *directories,
                                  NcmError *ncm_error) {
    NcmMpdItemList items;
    int32 status;

    if (directories == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD directory array"));
    }
    if ((status = ncm_mpd_client_prechecks_no_commands(client,
                                                       ncm_error)) < 0) {
        return status;
    }

    items = (NcmMpdItemList){0};
    status = ncm_mpd_connection_get_directory(&client->connection, path,
                                               &items);
    if (status < 0) {
        ncm_mpd_client_copy_connection_error(client, ncm_error);
    } else {
        ncm_mpd_item_list_to_directory_array(&items, directories);
        status = ncm_error_ok(ncm_error);
    }
    ncm_mpd_item_list_destroy(&items);
    return status;
}

int32
ncm_mpd_client_enable_output(MpdClient *client, int32 id,
                             NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_enable_output(&client->connection,
                                                        id),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_disable_output(MpdClient *client, int32 id,
                              NcmError *ncm_error) {
    NCM_CLIENT_TRY(ncm_mpd_client_prechecks_no_commands(client, ncm_error));
    NCM_CLIENT_TRY_MPD(client,
                       ncm_mpd_connection_disable_output(&client->connection,
                                                         id),
                       ncm_error);

    return ncm_error_ok(ncm_error);
}

int32
ncm_mpd_client_add_random_tag(MpdClient *client, enum NcmTagType tag,
                              int32 number, NcmError *ncm_error) {
    StringViewList tags;
    NcmMpdSongList songs;
    int32 status;

    if (number < 0) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("negative random count"));
    }
    if ((status = ncm_mpd_client_prechecks_no_commands(client,
                                                       ncm_error)) < 0) {
        return status;
    }

    tags = (StringViewList){0};
    songs = (NcmMpdSongList){0};
    status = ncm_mpd_connection_list_tag_values(&client->connection, tag,
                                                 &tags);
    if (status < 0) {
        ncm_mpd_client_copy_connection_error(client, ncm_error);
        goto cleanup;
    }
    if (number > tags.count) {
        status = ncm_error_set_status(ncm_error, -NCM_ERROR_UNAVAILABLE,
                                      STRLIT("not enough MPD tag values"));
        goto cleanup;
    }

    rand_shuffle(tags.items, tags.count, SIZEOF(*tags.items));
    for (int32 i = 0; i < number; i += 1) {
        status = ncm_mpd_connection_start_search_songs(&client->connection,
                                                        true);
        if (status < 0) {
            ncm_mpd_client_copy_connection_error(client, ncm_error);
            goto cleanup;
        }
        status = ncm_mpd_connection_add_search_tag(&client->connection,
                                                   tag,
                                                   tags.items[i].data);
        if (status < 0) {
            ncm_mpd_client_copy_connection_error(client, ncm_error);
            goto cleanup;
        }
        status = ncm_mpd_connection_commit_search_songs(&client->connection,
                                                         &songs);
        if (status < 0) {
            ncm_mpd_client_copy_connection_error(client, ncm_error);
            goto cleanup;
        }
        if ((status = ncm_mpd_client_start_command_list_ready(client,
                                                              ncm_error)) < 0) {
            goto cleanup;
        }
        for (int32 j = 0; j < songs.count; j += 1) {
            ncm_mpd_client_add_song_ready(client, songs.items[j].uri, -1,
                                          NULL, ncm_error);
        }
        if ((status =
             ncm_mpd_client_commit_command_list_ready(client, ncm_error)) < 0) {
            goto cleanup;
        }
        ncm_mpd_song_list_clear(&songs);
    }

    status = ncm_error_ok(ncm_error);

cleanup:
    if (client->command_list_active) {
        client->command_list_active = false;
    }
    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_string_list_destroy(&tags);
    return status;
}

int32
ncm_mpd_client_add_random_songs(MpdClient *client, int32 number,
                                char *exclude_pattern,
                                int32 exclude_pattern_len,
                                NcmError *ncm_error) {
    StringViewList files;
    NcmRegex regex;
    bool have_regex;
    int32 added;
    int32 status;

    if (number < 0) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("negative random count"));
    }
    if (exclude_pattern_len < 0) {
        exclude_pattern_len = optional_strlen32(exclude_pattern);
    }
    if ((status = ncm_mpd_client_prechecks_no_commands(client,
                                                       ncm_error)) < 0) {
        return status;
    }

    files = (StringViewList){0};
    regex = (NcmRegex){0};
    have_regex = false;

    status = ncm_mpd_connection_list_all_song_uris(&client->connection,
                                                   "/", &files);
    if (status < 0) {
        ncm_mpd_client_copy_connection_error(client, ncm_error);
        goto cleanup;
    }
    if (number > files.count) {
        status = ncm_error_set_status(ncm_error, -NCM_ERROR_UNAVAILABLE,
                                      STRLIT("not enough MPD songs"));
        goto cleanup;
    }

    if ((exclude_pattern != NULL) && (exclude_pattern_len > 0)) {
        status = ncm_regex_compile(&regex, exclude_pattern, exclude_pattern_len,
                                   NCM_REGEX_EXTENDED | NCM_REGEX_NOSUB,
                                   ncm_error);
        if (status < 0) {
            goto cleanup;
        }
        have_regex = true;
    }

    rand_shuffle(files.items, files.count, SIZEOF(*files.items));
    if ((status = ncm_mpd_client_start_command_list_ready(client,
                                                          ncm_error)) < 0) {
        goto cleanup;
    }

    added = 0;
    for (int32 i = 0; (i < files.count) && (added < number); i += 1) {
        if (have_regex
            && ncm_regex_matches(&regex,
                                 files.items[i].data, files.items[i].len)) {
            continue;
        }
        ncm_mpd_client_add_song_ready(client, files.items[i].data, -1, NULL,
                                      ncm_error);
        added += 1;
    }
    if ((status = ncm_mpd_client_commit_command_list_ready(client,
                                                           ncm_error)) < 0) {
        goto cleanup;
    }

    if (added != number) {
        status = ncm_error_set_status(ncm_error, -NCM_ERROR_UNAVAILABLE,
                                      STRLIT("not enough MPD songs"));
    } else {
        status = ncm_error_ok(ncm_error);
    }

cleanup:
    if (client->command_list_active) {
        client->command_list_active = false;
    }
    ncm_regex_destroy(&regex);
    ncm_mpd_string_list_destroy(&files);
    return status;
}

#endif /* NCM_MPD_CLIENT_C */
