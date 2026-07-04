#include "c/ncm_mpd_connection.h"

#include <stddef.h>

static int32 ncm_mpd_connection_cstring_len(char *string);
static void ncm_mpd_connection_cstring_copy(char *dst, int32 dst_cap,
                                            char *src);
static void ncm_mpd_connection_set_error(NcmMpdConnection *connection,
                                         enum mpd_error code,
                                         enum mpd_server_error server_code,
                                         bool clearable,
                                         char *message);

static int32
ncm_mpd_connection_cstring_len(char *string) {
    int32 len;

    if (string == NULL) {
        return 0;
    }

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }

    return len;
}

static void
ncm_mpd_connection_cstring_copy(char *dst, int32 dst_cap,
                                char *src) {
    int32 src_len;
    int32 len;

    if ((dst == NULL) || (dst_cap <= 0)) {
        return;
    }

    dst[0] = '\0';
    if (src == NULL) {
        return;
    }

    src_len = ncm_mpd_connection_cstring_len(src);
    len = src_len;
    if (len >= dst_cap) {
        len = dst_cap - 1;
    }

    for (int32 i = 0; i < len; i += 1) {
        dst[i] = src[i];
    }
    dst[len] = '\0';
    return;
}

static void
ncm_mpd_connection_set_error(NcmMpdConnection *connection,
                             enum mpd_error code,
                             enum mpd_server_error server_code,
                             bool clearable,
                             char *message) {
    int32 message_len;

    if (connection == NULL) {
        return;
    }

    connection->error_code = code;
    connection->server_error_code = server_code;
    connection->error_clearable = clearable;

    message_len = ncm_mpd_connection_cstring_len(message);
    ncm_error_set(&connection->error, (int32)code, message, message_len);
    return;
}

void
ncm_mpd_connection_init(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return;
    }

    connection->mpd = NULL;
    ncm_mpd_connection_clear_error(connection);
    return;
}

void
ncm_mpd_connection_destroy(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return;
    }

    ncm_mpd_connection_disconnect(connection);
    ncm_mpd_connection_clear_error(connection);
    return;
}

bool
ncm_mpd_connection_connect(NcmMpdConnection *connection,
                           char *host,
                           uint16 port,
                           uint32 timeout_ms) {
    bool ok;

    if (connection == NULL) {
        return false;
    }

    ncm_mpd_connection_disconnect(connection);
    ncm_mpd_connection_clear_error(connection);

    connection->mpd = mpd_connection_new(host, port, timeout_ms);
    if (connection->mpd == NULL) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"Could not create MPD connection");
        return false;
    }

    ok = ncm_mpd_connection_check_error(connection);
    if (!ok) {
        mpd_connection_free(connection->mpd);
        connection->mpd = NULL;
        return false;
    }

    return true;
}

void
ncm_mpd_connection_disconnect(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return;
    }

    if (connection->mpd != NULL) {
        mpd_connection_free(connection->mpd);
    }
    connection->mpd = NULL;
    return;
}

bool
ncm_mpd_connection_is_connected(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return false;
    }

    return connection->mpd != NULL;
}

struct mpd_connection *
ncm_mpd_connection_mpd(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return NULL;
    }

    return connection->mpd;
}

int32
ncm_mpd_connection_fd(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_is_connected(connection)) {
        return -1;
    }

    return mpd_connection_get_fd(connection->mpd);
}

bool
ncm_mpd_connection_check_error(NcmMpdConnection *connection) {
    enum mpd_error code;
    enum mpd_server_error server_code;
    bool clearable;
    char *message;

    if (connection == NULL) {
        return false;
    }
    if (connection->mpd == NULL) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"No active MPD connection");
        return false;
    }

    code = mpd_connection_get_error(connection->mpd);
    if (code == MPD_ERROR_SUCCESS) {
        ncm_mpd_connection_clear_error(connection);
        return true;
    }

    server_code = (enum mpd_server_error)0;
    if (code == MPD_ERROR_SERVER) {
        server_code = mpd_connection_get_server_error(connection->mpd);
    }

    message = (char *)mpd_connection_get_error_message(connection->mpd);
    ncm_mpd_connection_set_error(connection, code, server_code,
                                 false, message);
    clearable = mpd_connection_clear_error(connection->mpd);
    connection->error_clearable = clearable;
    return false;
}

char *
ncm_mpd_connection_error(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return NULL;
    }

    return connection->error.message;
}

void
ncm_mpd_connection_clear_error(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return;
    }

    ncm_error_clear(&connection->error);
    connection->error_code = MPD_ERROR_SUCCESS;
    connection->server_error_code = (enum mpd_server_error)0;
    connection->error_clearable = false;
    return;
}

enum mpd_error
ncm_mpd_connection_error_code(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return MPD_ERROR_SUCCESS;
    }

    return connection->error_code;
}

enum mpd_server_error
ncm_mpd_connection_server_error_code(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return (enum mpd_server_error)0;
    }

    return connection->server_error_code;
}

bool
ncm_mpd_connection_error_clearable(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return false;
    }

    return connection->error_clearable;
}

bool
ncm_mpd_connection_get_stats(NcmMpdConnection *connection,
                             NcmMpdStats *out_stats) {
    struct mpd_stats *stats;
    bool ok;

    if (connection == NULL) {
        return false;
    }
    if (out_stats == NULL) {
        return false;
    }
    if (connection->mpd == NULL) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"No active MPD connection");
        return false;
    }

    if ((stats = mpd_run_stats(connection->mpd)) == NULL) {
        if (ncm_mpd_connection_check_error(connection)) {
            ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                         (enum mpd_server_error)0, false,
                                         (char *)"Could not get MPD stats");
        }
        return false;
    }

    out_stats->artists = mpd_stats_get_number_of_artists(stats);
    out_stats->albums = mpd_stats_get_number_of_albums(stats);
    out_stats->songs = mpd_stats_get_number_of_songs(stats);
    out_stats->play_time = mpd_stats_get_play_time(stats);
    out_stats->uptime = mpd_stats_get_uptime(stats);
    out_stats->db_update_time = mpd_stats_get_db_update_time(stats);
    out_stats->db_play_time = mpd_stats_get_db_play_time(stats);

    mpd_stats_free(stats);
    ok = ncm_mpd_connection_check_error(connection);
    return ok;
}

bool
ncm_mpd_connection_get_status(NcmMpdConnection *connection,
                              NcmMpdStatus *out_status) {
    struct mpd_status *status;
    char *error;
    bool ok;

    if (connection == NULL) {
        return false;
    }
    if (out_status == NULL) {
        return false;
    }
    if (connection->mpd == NULL) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"No active MPD connection");
        return false;
    }

    if ((status = mpd_run_status(connection->mpd)) == NULL) {
        if (ncm_mpd_connection_check_error(connection)) {
            ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                         (enum mpd_server_error)0, false,
                                         (char *)"Could not get MPD status");
        }
        return false;
    }

    out_status->volume = mpd_status_get_volume(status);
    out_status->repeat = mpd_status_get_repeat(status);
    out_status->random = mpd_status_get_random(status);
    out_status->single = mpd_status_get_single(status);
    out_status->consume = mpd_status_get_consume(status);
    out_status->queue_length = mpd_status_get_queue_length(status);
    out_status->queue_version = mpd_status_get_queue_version(status);
    out_status->state = mpd_status_get_state(status);
    out_status->crossfade = mpd_status_get_crossfade(status);
    out_status->song_pos = mpd_status_get_song_pos(status);
    out_status->song_id = mpd_status_get_song_id(status);
    out_status->next_song_pos = mpd_status_get_next_song_pos(status);
    out_status->next_song_id = mpd_status_get_next_song_id(status);
    out_status->elapsed_time = mpd_status_get_elapsed_time(status);
    out_status->total_time = mpd_status_get_total_time(status);
    out_status->kbit_rate = mpd_status_get_kbit_rate(status);
    out_status->update_id = mpd_status_get_update_id(status);

    error = (char *)mpd_status_get_error(status);
    ncm_mpd_connection_cstring_copy(out_status->error,
                                    NCM_ARRAY_LEN(out_status->error),
                                    error);

    mpd_status_free(status);
    ok = ncm_mpd_connection_check_error(connection);
    return ok;
}
