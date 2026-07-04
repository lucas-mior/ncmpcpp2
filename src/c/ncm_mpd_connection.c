#include "c/ncm_mpd_connection.h"

#include <stddef.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"

static int32 ncm_mpd_connection_cstring_len(char *string);
static void ncm_mpd_connection_cstring_copy(char *dst, int32 dst_cap,
                                            char *src);
static void ncm_mpd_connection_set_error(NcmMpdConnection *connection,
                                         enum mpd_error code,
                                         enum mpd_server_error server_code,
                                         bool clearable,
                                         char *message);
static bool ncm_mpd_connection_require_connected(
    NcmMpdConnection *connection);
static bool ncm_mpd_song_list_push(NcmMpdSongList *list,
                                   NcmSong *song);
static bool ncm_mpd_connection_recv_song(NcmMpdConnection *connection,
                                         NcmSong *song);
static bool ncm_mpd_connection_recv_song_list(NcmMpdConnection *connection,
                                              NcmMpdSongList *songs);

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

static bool
ncm_mpd_connection_require_connected(NcmMpdConnection *connection) {
    if (connection == NULL) {
        return false;
    }
    if (connection->mpd == NULL) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"No active MPD connection");
        return false;
    }

    return true;
}

static bool
ncm_mpd_song_list_push(NcmMpdSongList *list, NcmSong *song) {
    int32 old_capacity;
    int32 new_capacity;

    if (list == NULL) {
        return false;
    }
    if (song == NULL) {
        return false;
    }

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = (NcmSong *)ncm_realloc_array(
            list->items, old_capacity, new_capacity, SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    ncm_song_init(&list->items[list->count]);
    ncm_song_move(&list->items[list->count], song);
    list->count += 1;
    return true;
}

static bool
ncm_mpd_connection_recv_song(NcmMpdConnection *connection,
                             NcmSong *song) {
    struct mpd_song *mpd_song;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (song == NULL) {
        return false;
    }

    mpd_song = mpd_recv_song(connection->mpd);
    if (mpd_song == NULL) {
        return true;
    }

    ok = ncm_song_from_mpd_song_copy(song, mpd_song);
    mpd_song_free(mpd_song);
    if (!ok) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"Could not read MPD song");
        return false;
    }

    return true;
}

static bool
ncm_mpd_connection_recv_song_list(NcmMpdConnection *connection,
                                  NcmMpdSongList *songs) {
    NcmSong song;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (songs == NULL) {
        return false;
    }

    ncm_mpd_song_list_clear(songs);
    for (;;) {
        ncm_song_init(&song);
        if (!ncm_mpd_connection_recv_song(connection, &song)) {
            ncm_song_destroy(&song);
            mpd_response_finish(connection->mpd);
            return false;
        }
        if (ncm_song_empty(&song)) {
            ncm_song_destroy(&song);
            break;
        }
        if (!ncm_mpd_song_list_push(songs, &song)) {
            ncm_song_destroy(&song);
            mpd_response_finish(connection->mpd);
            return false;
        }
        ncm_song_destroy(&song);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

void
ncm_mpd_song_list_init(NcmMpdSongList *list) {
    if (list == NULL) {
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return;
}

void
ncm_mpd_song_list_destroy(NcmMpdSongList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_song_list_clear(list);
    if (list->items != NULL) {
        ncm_free(list->items, list->capacity*SIZEOF(*list->items));
    }

    ncm_mpd_song_list_init(list);
    return;
}

void
ncm_mpd_song_list_clear(NcmMpdSongList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->count; i += 1) {
        ncm_song_destroy(&list->items[i]);
    }
    list->count = 0;
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

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (out_stats == NULL) {
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

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (out_status == NULL) {
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

bool
ncm_mpd_connection_get_current_song(NcmMpdConnection *connection,
                                    NcmSong *song) {
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (song == NULL) {
        return false;
    }

    if (!mpd_send_current_song(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    ok = ncm_mpd_connection_recv_song(connection, song);
    mpd_response_finish(connection->mpd);
    if (!ok) {
        return false;
    }

    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_get_song(NcmMpdConnection *connection,
                            char *path,
                            NcmSong *song) {
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (song == NULL) {
        return false;
    }

    if (!mpd_send_list_all_meta(connection->mpd, path)) {
        return ncm_mpd_connection_check_error(connection);
    }

    ok = ncm_mpd_connection_recv_song(connection, song);
    mpd_response_finish(connection->mpd);
    if (!ok) {
        return false;
    }

    if (ncm_song_empty(song)) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"Could not get MPD song");
        return false;
    }

    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_get_queue_changes(NcmMpdConnection *connection,
                                     uint32 version,
                                     NcmMpdSongList *songs) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_queue_changes_meta(connection->mpd, version)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

bool
ncm_mpd_connection_get_playlist_content(NcmMpdConnection *connection,
                                        char *path,
                                        NcmMpdSongList *songs) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_list_playlist_meta(connection->mpd, path)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

bool
ncm_mpd_connection_get_playlist_content_no_info(
    NcmMpdConnection *connection,
    char *path,
    NcmMpdSongList *songs) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_list_playlist(connection->mpd, path)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

bool
ncm_mpd_connection_play(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_play(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_play_pos(NcmMpdConnection *connection, int32 pos) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_play_pos(connection->mpd, (unsigned)pos);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_play_id(NcmMpdConnection *connection, int32 id) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_play_id(connection->mpd, (unsigned)id);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_pause(NcmMpdConnection *connection, bool state) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_pause(connection->mpd, state);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_toggle_pause(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_toggle_pause(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_stop(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_stop(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_next(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_next(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_previous(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_previous(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_seek_pos(NcmMpdConnection *connection,
                            uint32 pos,
                            uint32 seconds) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_seek_pos(connection->mpd, pos, seconds);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_set_repeat(NcmMpdConnection *connection, bool mode) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_repeat(connection->mpd, mode);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_set_random(NcmMpdConnection *connection, bool mode) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_random(connection->mpd, mode);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_set_single(NcmMpdConnection *connection, bool mode) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_single(connection->mpd, mode);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_set_consume(NcmMpdConnection *connection, bool mode) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_consume(connection->mpd, mode);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_set_crossfade(NcmMpdConnection *connection,
                                 uint32 seconds) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_crossfade(connection->mpd, seconds);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_set_volume(NcmMpdConnection *connection, uint32 vol) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_set_volume(connection->mpd, vol);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_change_volume(NcmMpdConnection *connection,
                                 int32 change) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_change_volume(connection->mpd, change);
    return ncm_mpd_connection_check_error(connection);
}
