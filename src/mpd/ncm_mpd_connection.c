#if !defined(NCM_MPD_CONNECTION_C)
#define NCM_MPD_CONNECTION_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"
#include "ncmpcpp2_mpd.h"

#define NCM_MPD_RETURN_IF_ERROR(expression) \
    do { \
        int32 status_ = (expression); \
        if (status_ < 0) { \
            return status_; \
        } \
    } while (0)

static void
ncm_mpd_connection_cstring_copy(char *dst, int32 dst_cap, char *src) {
    int32 src_len;
    int32 len;

    dst[0] = '\0';
    src_len = optional_strlen32(src);
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
ncm_mpd_connection_set_error(MpdConnection *connection,
                             enum mpd_error code,
                             enum mpd_server_error server_code,
                             bool clearable, char *message) {
    int32 message_len;

    switch (code) {
    case MPD_ERROR_SUCCESS:
        connection->error_code = NCM_MPD_ERROR_SUCCESS;
        break;
    case MPD_ERROR_OOM:
        connection->error_code = NCM_MPD_ERROR_OOM;
        break;
    case MPD_ERROR_ARGUMENT:
        connection->error_code = NCM_MPD_ERROR_ARGUMENT;
        break;
    case MPD_ERROR_STATE:
        connection->error_code = NCM_MPD_ERROR_STATE;
        break;
    case MPD_ERROR_TIMEOUT:
        connection->error_code = NCM_MPD_ERROR_TIMEOUT;
        break;
    case MPD_ERROR_SYSTEM:
        connection->error_code = NCM_MPD_ERROR_SYSTEM;
        break;
    case MPD_ERROR_RESOLVER:
        connection->error_code = NCM_MPD_ERROR_RESOLVER;
        break;
    case MPD_ERROR_CLOSED:
        connection->error_code = NCM_MPD_ERROR_CLOSED;
        break;
    case MPD_ERROR_MALFORMED:
        connection->error_code = NCM_MPD_ERROR_MALFORMED;
        break;
    case MPD_ERROR_SERVER:
    default:
        connection->error_code = NCM_MPD_ERROR_SERVER;
        break;
    }

    switch (server_code) {
    case MPD_SERVER_ERROR_EXIST:
        connection->server_error_code = NCM_MPD_SERVER_ERROR_EXIST;
        break;
    case MPD_SERVER_ERROR_NO_EXIST:
        connection->server_error_code = NCM_MPD_SERVER_ERROR_NO_EXIST;
        break;
    case MPD_SERVER_ERROR_PERMISSION:
        connection->server_error_code = NCM_MPD_SERVER_ERROR_PERMISSION;
        break;
    case MPD_SERVER_ERROR_UNK:
        connection->server_error_code = NCM_MPD_SERVER_ERROR_NONE;
        break;
    case MPD_SERVER_ERROR_NOT_LIST:
    case MPD_SERVER_ERROR_ARG:
    case MPD_SERVER_ERROR_PASSWORD:
    case MPD_SERVER_ERROR_UNKNOWN_CMD:
    case MPD_SERVER_ERROR_PLAYLIST_MAX:
    case MPD_SERVER_ERROR_SYSTEM:
    case MPD_SERVER_ERROR_PLAYLIST_LOAD:
    case MPD_SERVER_ERROR_UPDATE_ALREADY:
    case MPD_SERVER_ERROR_PLAYER_SYNC:
    default:
        connection->server_error_code = NCM_MPD_SERVER_ERROR_UNKNOWN;
        break;
    }
    connection->error_clearable = clearable;

    message_len = optional_strlen32(message);
    ncm_error_set(&connection->ncm_error, (int32)connection->error_code,
                  message, message_len);
    return;
}

#define NCM_MPD_CONNECTION_TAG_TYPE_CASE(                                  \
    tag, name, alias, tag_char, field, getter, getter_char,                \
    taglib_property, taglib_name, settings_name, mpd, flags                \
)                                                                          \
    case tag:                                                              \
        return CAT(MPD_TAG_, mpd);

static enum mpd_tag_type
ncm_mpd_connection_tag_type(enum NcmTagType tag) {
    switch ((int32)tag) {
    NCM_MPD_TAG_DEFS(NCM_MPD_CONNECTION_TAG_TYPE_CASE)
    default:
        return MPD_TAG_UNKNOWN;
    }
}

#undef NCM_MPD_CONNECTION_TAG_TYPE_CASE

static int32
ncm_mpd_connection_require_connected(MpdConnection *connection) {
    if (connection == NULL) {
        return -EINVAL;
    }
    if (connection->mpd == NULL) {
        ncm_mpd_connection_set_error(connection,
                                     MPD_ERROR_STATE,
                                     (enum mpd_server_error)0,
                                     false,
                                     "No active MPD connection");
        return -NCM_ERROR_INVALID_STATE;
    }

    return 0;
}

static int32
ncm_mpd_song_list_push(NcmMpdSongList *list, NcmSong *song) {
    int32 old_capacity;
    int32 new_capacity;
    int32 index;

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = realloc2(list->items, old_capacity, new_capacity,
                               SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    index = list->count;
    list->items[index] = (NcmSong){0};
    ncm_song_move(&list->items[index], song);
    list->count += 1;
    return index;
}

static void
ncm_mpd_string_list_push(StringViewList *list, char *value) {
    int32 old_capacity;
    int32 new_capacity;
    int32 value_len;
    int32 index;
    StringView *string;

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = realloc2(list->items, old_capacity, new_capacity,
                               SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    index = list->count;
    value_len = optional_strlen32(value);
    string = &list->items[index];
    string->data = malloc2(value_len + 1);
    string->len = value_len;
    ncm_mpd_connection_cstring_copy(string->data, value_len + 1, value);
    list->count += 1;
    return;
}

static char *
ncm_mpd_connection_mpd_directory(char *directory) {
    if (directory == NULL) {
        return NULL;
    }
    if ((directory[0] == '/') && (directory[1] == '\0')) {
        return "";
    }

    return directory;
}

static int32
ncm_mpd_connection_recv_song(MpdConnection *connection, NcmSong *song) {
    struct mpd_song *mpd_song;
    int32 status;

    if ((mpd_song = mpd_recv_song(connection->mpd)) == NULL) {
        return 0;
    }

    status = ncm_mpd_item_song_from_mpd_song_copy(song, mpd_song);
    mpd_song_free(mpd_song);
    if (status < 0) {
        ncm_mpd_connection_set_error(connection,
                                     MPD_ERROR_STATE,
                                     (enum mpd_server_error)0,
                                     false,
                                     "Could not read MPD song");
        return -NCM_ERROR_MPD;
    }

    return 0;
}

static int32
ncm_mpd_connection_recv_song_list(MpdConnection *connection,
                                  NcmMpdSongList *songs) {
    NcmSong song = {0};

    ncm_mpd_song_list_clear(songs);
    while (true) {
        song = (NcmSong){0};
        if (ncm_mpd_connection_recv_song(connection, &song) < 0) {
            ncm_song_destroy(&song);
            mpd_response_finish(connection->mpd);
            return -NCM_ERROR_MPD;
        }
        if (ncm_song_is_empty(&song)) {
            ncm_song_destroy(&song);
            break;
        }
        ncm_mpd_song_list_push(songs, &song);
        ncm_song_destroy(&song);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

static int32
ncm_mpd_connection_recv_pair_list(MpdConnection *connection, char *name,
                                  StringViewList *strings) {
    struct mpd_pair *pair;

    ncm_mpd_string_list_clear(strings);
    while (true) {
        if ((pair = mpd_recv_pair_named(connection->mpd, name)) == NULL) {
            break;
        }
        ncm_mpd_string_list_push(strings, (char *)pair->value);
        mpd_return_pair(connection->mpd, pair);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

void
ncm_mpd_song_list_destroy(NcmMpdSongList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_song_list_clear(list);
    free2(list->items, list->capacity*SIZEOF(*list->items));
    *list = (NcmMpdSongList){0};

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

int32
ncm_mpd_song_list_count(NcmMpdSongList *list) {
    if (list == NULL) {
        return 0;
    }

    return list->count;
}

NcmSong *
ncm_mpd_song_list_at(NcmMpdSongList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= list->count)) {
        return NULL;
    }

    return &list->items[idx];
}

int32
ncm_mpd_song_list_append_copy(NcmMpdSongList *list, NcmSong *song) {
    NcmSong copy = {0};
    int32 index;

    if ((list == NULL) || (song == NULL)) {
        return -EINVAL;
    }

    ncm_song_copy(&copy, song);
    index = ncm_mpd_song_list_push(list, &copy);
    ncm_song_destroy(&copy);
    return index;
}

int32
ncm_mpd_song_list_to_song_array(NcmMpdSongList *list, NcmSongArray *songs) {
    NcmSongArray replacement = {0};

    if (songs == NULL) {
        return -EINVAL;
    }

    if (list) {
        for (int32 i = 0; i < list->count; i += 1) {
            ncm_song_array_append_copy(&replacement, &list->items[i]);
        }
    }

    ncm_song_array_move(songs, &replacement);
    return songs->len;
}

void
ncm_mpd_item_list_destroy(NcmMpdItemList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_item_list_clear(list);
    free2(list->items, list->capacity*SIZEOF(*list->items));
    *list = (NcmMpdItemList){0};

    return;
}

void
ncm_mpd_item_list_clear(NcmMpdItemList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->count; i += 1) {
        ncm_mpd_item_destroy(&list->items[i]);
    }
    list->count = 0;
    return;
}

int32
ncm_mpd_item_list_to_item_array(NcmMpdItemList *list, NcmMpdItemArray *items) {
    NcmMpdItemArray replacement = {0};

    if (items == NULL) {
        return -EINVAL;
    }

    if (list) {
        for (int32 i = 0; i < list->count; i += 1) {
            ncm_mpd_item_array_append_copy(&replacement, &list->items[i]);
        }
    }

    ncm_mpd_item_array_move(items, &replacement);
    return items->len;
}

int32
ncm_mpd_item_list_to_directory_array(NcmMpdItemList *list,
                                     NcmDirectoryArray *directories) {
    NcmDirectoryArray replacement = {0};

    if (directories == NULL) {
        return -EINVAL;
    }

    if (list) {
        for (int32 i = 0; i < list->count; i += 1) {
            NcmDirectory *directory;

            if (ncm_mpd_item_kind(&list->items[i]) != NCM_MPD_ITEM_DIRECTORY) {
                continue;
            }

            directory = ncm_mpd_item_directory(&list->items[i]);
            ncm_directory_array_append_copy(&replacement, directory);
        }
    }

    ncm_directory_array_move(directories, &replacement);
    return directories->len;
}

void
ncm_mpd_string_list_destroy(StringViewList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_string_list_clear(list);
    free2(list->items, list->capacity*SIZEOF(*list->items));
    *list = (StringViewList){0};

    return;
}

void
ncm_mpd_string_list_clear(StringViewList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->count; i += 1) {
        free2(list->items[i].data, list->items[i].len + 1);
        list->items[i] = (StringView){0};
    }
    list->count = 0;
    return;
}

int32
ncm_mpd_string_list_count(StringViewList *list) {
    if (list == NULL) {
        return 0;
    }

    return list->count;
}

StringView *
ncm_mpd_string_list_at(StringViewList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= list->count)) {
        return NULL;
    }

    return &list->items[idx];
}

void
ncm_mpd_output_list_destroy(NcmMpdOutputList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_output_list_clear(list);
    free2(list->items, list->capacity*SIZEOF(*list->items));
    *list = (NcmMpdOutputList){0};

    return;
}

void
ncm_mpd_output_list_clear(NcmMpdOutputList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->count; i += 1) {
        free2(list->items[i].name, list->items[i].name_len + 1);
        list->items[i] = (NcmMpdOutput){0};
    }
    list->count = 0;
    return;
}

void
ncm_mpd_playlist_list_destroy(NcmMpdPlaylistList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_playlist_list_clear(list);
    free2(list->items, list->capacity*SIZEOF(*list->items));
    *list = (NcmMpdPlaylistList){0};

    return;
}

void
ncm_mpd_playlist_list_clear(NcmMpdPlaylistList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->count; i += 1) {
        ncm_playlist_destroy(&list->items[i]);
    }
    list->count = 0;
    return;
}

void
ncm_mpd_connection_destroy(MpdConnection *connection) {
    if (connection == NULL) {
        return;
    }

    ncm_mpd_connection_disconnect(connection);
    ncm_mpd_connection_clear_error(connection);
    return;
}

int32
ncm_mpd_connection_connect(MpdConnection *connection,
                           char *host, uint16 port, int32 timeout_ms) {
    int32 status;

    if (connection == NULL) {
        return -EINVAL;
    }

    ncm_mpd_connection_disconnect(connection);
    ncm_mpd_connection_clear_error(connection);

    connection->mpd = mpd_connection_new(host, port, (uint32)timeout_ms);
    if (connection->mpd == NULL) {
        ncm_mpd_connection_set_error(connection,
                                     MPD_ERROR_STATE,
                                     (enum mpd_server_error)0,
                                     false,
                                     "Could not create MPD connection");
        return -NCM_ERROR_MPD;
    }

    if ((status = ncm_mpd_connection_check_error(connection)) < 0) {
        mpd_connection_free(connection->mpd);
        connection->mpd = NULL;
        return status;
    }

    return 0;
}

void
ncm_mpd_connection_disconnect(MpdConnection *connection) {
    if (connection == NULL) {
        return;
    }

    if (connection->mpd) {
        mpd_connection_free(connection->mpd);
    }
    connection->mpd = NULL;
    return;
}

bool
ncm_mpd_connection_is_connected(MpdConnection *connection) {
    if (connection == NULL) {
        return false;
    }

    return connection->mpd;
}

int32
ncm_mpd_connection_fd(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    return mpd_connection_get_fd(connection->mpd);
}

int32
ncm_mpd_connection_set_timeout(MpdConnection *connection, int32 timeout_ms) {
    if (connection == NULL) {
        return -EINVAL;
    }
    if (connection->mpd == NULL) {
        return 0;
    }

    mpd_connection_set_timeout(connection->mpd, (uint32)timeout_ms);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_send_idle(MpdConnection *connection, int32 events) {
    enum mpd_idle mpd_events = (enum mpd_idle)0;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (events == 0) {
        if (!mpd_send_idle(connection->mpd)) {
            return ncm_mpd_connection_check_error(connection);
        }
        return 0;
    }

    if ((events & NCM_MPD_IDLE_DATABASE) != 0) {
        mpd_events |= MPD_IDLE_DATABASE;
    }
    if ((events & NCM_MPD_IDLE_STORED_PLAYLIST) != 0) {
        mpd_events |= MPD_IDLE_STORED_PLAYLIST;
    }
    if ((events & NCM_MPD_IDLE_PLAYLIST) != 0) {
        mpd_events |= MPD_IDLE_PLAYLIST;
    }
    if ((events & NCM_MPD_IDLE_PLAYER) != 0) {
        mpd_events |= MPD_IDLE_PLAYER;
    }
    if ((events & NCM_MPD_IDLE_MIXER) != 0) {
        mpd_events |= MPD_IDLE_MIXER;
    }
    if ((events & NCM_MPD_IDLE_OUTPUT) != 0) {
        mpd_events |= MPD_IDLE_OUTPUT;
    }
    if ((events & NCM_MPD_IDLE_UPDATE) != 0) {
        mpd_events |= MPD_IDLE_UPDATE;
    }
    if ((events & NCM_MPD_IDLE_OPTIONS) != 0) {
        mpd_events |= MPD_IDLE_OPTIONS;
    }

    if (!mpd_send_idle_mask(connection->mpd, mpd_events)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return 0;
}

int32
ncm_mpd_connection_recv_idle(MpdConnection *connection, bool disable_timeout,
                             int32 *out_events) {
    enum mpd_idle mpd_events;
    int32 events;
    int32 status;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_events = (enum mpd_idle)mpd_recv_idle(connection->mpd, disable_timeout);
    mpd_response_finish(connection->mpd);
    status = ncm_mpd_connection_check_error(connection);

    events = 0;
    if ((mpd_events & MPD_IDLE_DATABASE) != 0) {
        events |= NCM_MPD_IDLE_DATABASE;
    }
    if ((mpd_events & MPD_IDLE_STORED_PLAYLIST) != 0) {
        events |= NCM_MPD_IDLE_STORED_PLAYLIST;
    }
    if ((mpd_events & MPD_IDLE_PLAYLIST) != 0) {
        events |= NCM_MPD_IDLE_PLAYLIST;
    }
    if ((mpd_events & MPD_IDLE_PLAYER) != 0) {
        events |= NCM_MPD_IDLE_PLAYER;
    }
    if ((mpd_events & MPD_IDLE_MIXER) != 0) {
        events |= NCM_MPD_IDLE_MIXER;
    }
    if ((mpd_events & MPD_IDLE_OUTPUT) != 0) {
        events |= NCM_MPD_IDLE_OUTPUT;
    }
    if ((mpd_events & MPD_IDLE_UPDATE) != 0) {
        events |= NCM_MPD_IDLE_UPDATE;
    }
    if ((mpd_events & MPD_IDLE_OPTIONS) != 0) {
        events |= NCM_MPD_IDLE_OPTIONS;
    }
    if (out_events) {
        *out_events = events;
    }

    return status;
}

int32
ncm_mpd_connection_noidle(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (!mpd_send_noidle(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return 0;
}

int32
ncm_mpd_connection_check_error(MpdConnection *connection) {
    enum mpd_error code;
    enum mpd_server_error server_code;
    bool clearable;
    char *message;

    if (connection == NULL) {
        return -EINVAL;
    }
    if (connection->mpd == NULL) {
        ncm_mpd_connection_set_error(connection,
                                     MPD_ERROR_STATE,
                                     (enum mpd_server_error)0,
                                     false,
                                     "No active MPD connection");
        return -NCM_ERROR_INVALID_STATE;
    }

    code = mpd_connection_get_error(connection->mpd);
    if (code == MPD_ERROR_SUCCESS) {
        ncm_mpd_connection_clear_error(connection);
        return 0;
    }

    server_code = (enum mpd_server_error)0;
    if (code == MPD_ERROR_SERVER) {
        server_code = mpd_connection_get_server_error(connection->mpd);
    }

    message = (char *)mpd_connection_get_error_message(connection->mpd);
    ncm_mpd_connection_set_error(connection, code, server_code, false, message);
    clearable = mpd_connection_clear_error(connection->mpd);
    connection->error_clearable = clearable;

    switch (code) {
    case MPD_ERROR_SUCCESS:
        return 0;
    case MPD_ERROR_OOM:
        return -ENOMEM;
    case MPD_ERROR_ARGUMENT:
        return -EINVAL;
    case MPD_ERROR_STATE:
        return -NCM_ERROR_INVALID_STATE;
    case MPD_ERROR_TIMEOUT:
        return -ETIMEDOUT;
    case MPD_ERROR_SYSTEM:
        return -EIO;
    case MPD_ERROR_RESOLVER:
    case MPD_ERROR_CLOSED:
        return -NCM_ERROR_NETWORK;
    case MPD_ERROR_MALFORMED:
        return -NCM_ERROR_PARSE;
    case MPD_ERROR_SERVER:
        return -NCM_ERROR_MPD;
    default:
        return -NCM_ERROR_MPD;
    }
}

char *
ncm_mpd_connection_error(MpdConnection *connection) {
    if (connection == NULL) {
        return NULL;
    }

    return connection->ncm_error.message;
}

void
ncm_mpd_connection_clear_error(MpdConnection *connection) {
    if (connection == NULL) {
        return;
    }

    ncm_error_clear(&connection->ncm_error);
    connection->error_code = NCM_MPD_ERROR_SUCCESS;
    connection->server_error_code = NCM_MPD_SERVER_ERROR_NONE;
    connection->error_clearable = false;
    return;
}

enum NcmMpdError
ncm_mpd_connection_error_code(MpdConnection *connection) {
    if (connection == NULL) {
        return NCM_MPD_ERROR_SUCCESS;
    }

    return connection->error_code;
}

enum NcmMpdServerError
ncm_mpd_connection_server_error_code(MpdConnection *connection) {
    if (connection == NULL) {
        return NCM_MPD_SERVER_ERROR_NONE;
    }

    return connection->server_error_code;
}

bool
ncm_mpd_connection_error_is_clearable(MpdConnection *connection) {
    if (connection == NULL) {
        return false;
    }

    return connection->error_clearable;
}

int32
ncm_mpd_connection_get_stats(MpdConnection *connection,
                             NcmMpdStats *out_stats) {
    struct mpd_stats *stats;
    int32 status;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (out_stats == NULL) {
        return -EINVAL;
    }

    if ((stats = mpd_run_stats(connection->mpd)) == NULL) {
        status = ncm_mpd_connection_check_error(connection);
        if (status >= 0) {
            ncm_mpd_connection_set_error(connection,
                                         MPD_ERROR_STATE,
                                         (enum mpd_server_error)0,
                                         false,
                                         "Could not get MPD stats");
            status = -NCM_ERROR_MPD;
        }
        return status;
    }

    out_stats->artists = (int32)mpd_stats_get_number_of_artists(stats);
    out_stats->albums = (int32)mpd_stats_get_number_of_albums(stats);
    out_stats->songs = (int32)mpd_stats_get_number_of_songs(stats);
    out_stats->play_time = (int32)mpd_stats_get_play_time(stats);
    out_stats->uptime = (int32)mpd_stats_get_uptime(stats);
    out_stats->db_update_time = (int32)mpd_stats_get_db_update_time(stats);
    out_stats->db_play_time = (int32)mpd_stats_get_db_play_time(stats);

    mpd_stats_free(stats);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_get_status(MpdConnection *connection,
                              NcmMpdStatus *out_status) {
    struct mpd_status *mpd_status;
    char *error;
    int32 error_cap;
    int32 status;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (out_status == NULL) {
        return -EINVAL;
    }

    if ((mpd_status = mpd_run_status(connection->mpd)) == NULL) {
        status = ncm_mpd_connection_check_error(connection);
        if (status >= 0) {
            ncm_mpd_connection_set_error(connection,
                                         MPD_ERROR_STATE,
                                         (enum mpd_server_error)0,
                                         false,
                                         "Could not get MPD status");
            status = -NCM_ERROR_MPD;
        }
        return status;
    }

    out_status->volume = mpd_status_get_volume(mpd_status);
    out_status->repeat = mpd_status_get_repeat(mpd_status);
    out_status->random = mpd_status_get_random(mpd_status);
    out_status->single = mpd_status_get_single(mpd_status);
    out_status->consume = mpd_status_get_consume(mpd_status);
    out_status->queue_length = (int32)mpd_status_get_queue_length(mpd_status);
    out_status->queue_version = (int32)mpd_status_get_queue_version(mpd_status);
    switch (mpd_status_get_state(mpd_status)) {
    case MPD_STATE_STOP:
        out_status->state = NCM_MPD_STATE_STOP;
        break;
    case MPD_STATE_PLAY:
        out_status->state = NCM_MPD_STATE_PLAY;
        break;
    case MPD_STATE_PAUSE:
        out_status->state = NCM_MPD_STATE_PAUSE;
        break;
    case MPD_STATE_UNKNOWN:
    default:
        out_status->state = NCM_MPD_STATE_UNKNOWN;
        break;
    }
    out_status->crossfade = (int32)mpd_status_get_crossfade(mpd_status);
    out_status->song_pos = mpd_status_get_song_pos(mpd_status);
    out_status->song_id = mpd_status_get_song_id(mpd_status);
    out_status->next_song_pos = mpd_status_get_next_song_pos(mpd_status);
    out_status->next_song_id = mpd_status_get_next_song_id(mpd_status);
    out_status->elapsed_time = (int32)mpd_status_get_elapsed_time(mpd_status);
    out_status->elapsed_time_ms = (int64)mpd_status_get_elapsed_ms(mpd_status);
    out_status->total_time = (int32)mpd_status_get_total_time(mpd_status);
    out_status->kbit_rate = (int32)mpd_status_get_kbit_rate(mpd_status);
    out_status->update_id = (int32)mpd_status_get_update_id(mpd_status);

    error = (char *)mpd_status_get_error(mpd_status);
    error_cap = LENGTH(out_status->error);
    ncm_mpd_connection_cstring_copy(out_status->error, error_cap, error);

    mpd_status_free(mpd_status);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_version(MpdConnection *connection) {
    uint *version;

    if (!ncm_mpd_connection_is_connected(connection)) {
        return 0;
    }

    version = (uint *)mpd_connection_get_server_version(connection->mpd);
    if (version == NULL) {
        return 0;
    }

    return (int32)version[1];
}

int32
ncm_mpd_connection_send_password(MpdConnection *connection, char *password) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (password == NULL) {
        return 0;
    }
    if (password[0] == '\0') {
        return 0;
    }

    if (!mpd_run_password(connection->mpd, password)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_start_command_list(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (!mpd_command_list_begin(connection->mpd, true)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_commit_command_list(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (!mpd_command_list_end(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_get_supported_extensions(MpdConnection *connection,
                                            StringViewList *strings) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (strings == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_command(connection->mpd, "decoders", NULL)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_pair_list(connection, "suffix", strings);
}

int32
ncm_mpd_connection_get_replay_gain_mode(MpdConnection *connection,
                                        enum NcmMpdReplayGainMode *mode) {
    struct mpd_pair *pair;
    char *name;
    int32 status;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (mode == NULL) {
        return -EINVAL;
    }

    *mode = NCM_MPD_REPLAY_GAIN_OFF;
    if (!mpd_send_command(connection->mpd, "replay_gain_status", NULL)) {
        return ncm_mpd_connection_check_error(connection);
    }

    pair = mpd_recv_pair_named(connection->mpd, "replay_gain_mode");
    if (pair == NULL) {
        mpd_response_finish(connection->mpd);
        return ncm_mpd_connection_check_error(connection);
    }

    name = (char *)pair->value;
    if (name == NULL) {
        status = -EINVAL;
    } else if (strequal(name, "off")) {
        *mode = NCM_MPD_REPLAY_GAIN_OFF;
        status = 0;
    } else if (strequal(name, "track")) {
        *mode = NCM_MPD_REPLAY_GAIN_TRACK;
        status = 0;
    } else if (strequal(name, "album")) {
        *mode = NCM_MPD_REPLAY_GAIN_ALBUM;
        status = 0;
    } else {
        status = -NCM_ERROR_PARSE;
    }

    mpd_return_pair(connection->mpd, pair);
    mpd_response_finish(connection->mpd);
    if (status < 0) {
        ncm_mpd_connection_set_error(connection,
                                     MPD_ERROR_STATE,
                                     (enum mpd_server_error)0,
                                     false,
                                     "Unknown replay gain mode");
        return status;
    }

    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_set_replay_gain_mode(MpdConnection *connection,
                                        enum NcmMpdReplayGainMode mode) {
    char *name;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    switch (mode) {
    case NCM_MPD_REPLAY_GAIN_OFF:
        name = "off";
        break;
    case NCM_MPD_REPLAY_GAIN_TRACK:
        name = "track";
        break;
    case NCM_MPD_REPLAY_GAIN_ALBUM:
        name = "album";
        break;
    case NCM_MPD_REPLAY_GAIN_COUNT:
    default:
        name = "off";
        break;
    }

    if (!mpd_send_command(connection->mpd, "replay_gain_mode", name, NULL)) {
        return ncm_mpd_connection_check_error(connection);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_get_playlists(MpdConnection *connection,
                                 NcmMpdPlaylistList *playlists) {
    struct mpd_playlist *playlist;
    NcmPlaylist item = {0};
    int32 old_capacity;
    int32 new_capacity;
    int32 index;
    int32 err;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (playlists == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_playlists(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    ncm_mpd_playlist_list_clear(playlists);
    while (true) {
        if ((playlist = mpd_recv_playlist(connection->mpd)) == NULL) {
            break;
        }

        if (playlists->count >= playlists->capacity) {
            old_capacity = playlists->capacity;
            new_capacity = old_capacity*2;
            if (new_capacity < 8) {
                new_capacity = 8;
            }

            playlists->items = realloc2(playlists->items,
                                        old_capacity, new_capacity,
                                        SIZEOF(*playlists->items));
            playlists->capacity = new_capacity;
        }

        item = (NcmPlaylist){0};
        err = ncm_mpd_item_playlist_from_mpd_playlist(&item, playlist);
        mpd_playlist_free(playlist);
        if (err < 0) {
            ncm_playlist_destroy(&item);
            mpd_response_finish(connection->mpd);
            return -NCM_ERROR_MPD;
        }

        index = playlists->count;
        playlists->items[index] = (NcmPlaylist){0};
        ncm_playlist_move(&playlists->items[index], &item);
        playlists->count += 1;
        ncm_playlist_destroy(&item);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_list_all_song_uris(MpdConnection *connection, char *path,
                                      StringViewList *strings) {
    char *directory;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (strings == NULL) {
        return -EINVAL;
    }

    directory = ncm_mpd_connection_mpd_directory(path);
    if (!mpd_send_list_all(connection->mpd, directory)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_pair_list(connection, "file", strings);
}

int32
ncm_mpd_connection_get_url_handlers(MpdConnection *connection,
                                    StringViewList *strings) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (strings == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_url_schemes(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_pair_list(connection, "handler", strings);
}

int32
ncm_mpd_connection_get_tag_types(MpdConnection *connection,
                                 StringViewList *strings) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (strings == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_tag_types(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_pair_list(connection, "tagtype", strings);
}

int32
ncm_mpd_connection_get_current_song(MpdConnection *connection,
                                    NcmSong *song) {
    int32 status;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (song == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_current_song(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    status = ncm_mpd_connection_recv_song(connection, song);
    mpd_response_finish(connection->mpd);
    if (status < 0) {
        return status;
    }

    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_get_queue(MpdConnection *connection,
                             NcmMpdSongList *songs) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (songs == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_queue_meta(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

int32
ncm_mpd_connection_get_queue_changes(MpdConnection *connection,
                                     int32 version, NcmMpdSongList *songs) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (songs == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_queue_changes_meta(connection->mpd, (uint32)version)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

int32
ncm_mpd_connection_get_playlist_content(MpdConnection *connection,
                                        char *path, NcmMpdSongList *songs) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (songs == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_playlist_meta(connection->mpd, path)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

int32
ncm_mpd_connection_get_playlist_content_no_info(MpdConnection *connection,
    char *path, NcmMpdSongList *songs) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (songs == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_playlist(connection->mpd, path)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

int32
ncm_mpd_connection_get_directory(MpdConnection *connection, char *path,
                                 NcmMpdItemList *items) {
    struct mpd_entity *entity;
    NcmMpdItem item = {0};
    int32 old_capacity;
    int32 new_capacity;
    int32 index;
    int32 err;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (items == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_meta(connection->mpd,
                            ncm_mpd_connection_mpd_directory(path))) {
        return ncm_mpd_connection_check_error(connection);
    }

    ncm_mpd_item_list_clear(items);
    while (true) {
        if ((entity = mpd_recv_entity(connection->mpd)) == NULL) {
            break;
        }

        ncm_mpd_item_init(&item);
        err = ncm_mpd_item_from_entity_copy(&item, entity);
        mpd_entity_free(entity);
        if (err < 0) {
            ncm_mpd_item_destroy(&item);
            ncm_mpd_connection_set_error(connection,
                                         MPD_ERROR_STATE,
                                         (enum mpd_server_error)0,
                                         false,
                                         "Could not read MPD directory item");
            mpd_response_finish(connection->mpd);
            return -NCM_ERROR_MPD;
        }

        if (items->count >= items->capacity) {
            old_capacity = items->capacity;
            new_capacity = old_capacity*2;
            if (new_capacity < 8) {
                new_capacity = 8;
            }

            items->items = realloc2(items->items, old_capacity, new_capacity,
                                    SIZEOF(*items->items));
            items->capacity = new_capacity;
        }

        index = items->count;
        ncm_mpd_item_init(&items->items[index]);
        ncm_mpd_item_move(&items->items[index], &item);
        items->count += 1;
        ncm_mpd_item_destroy(&item);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_get_directory_songs(MpdConnection *connection, char *path,
                                       NcmMpdSongList *songs) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (songs == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_meta(connection->mpd,
                            ncm_mpd_connection_mpd_directory(path))) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

int32
ncm_mpd_connection_list_all_songs(MpdConnection *connection, char *path,
                                  NcmMpdSongList *songs) {
    struct mpd_entity *entity;
    struct mpd_song *mpd_song;
    NcmSong song = {0};
    int32 err;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (songs == NULL) {
        return -EINVAL;
    }

    if (!mpd_send_list_all_meta(connection->mpd,
                                ncm_mpd_connection_mpd_directory(path))) {
        return ncm_mpd_connection_check_error(connection);
    }

    ncm_mpd_song_list_clear(songs);
    while (true) {
        if ((entity = mpd_recv_entity(connection->mpd)) == NULL) {
            break;
        }

        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            mpd_song = (struct mpd_song *)mpd_entity_get_song(entity);
            song = (NcmSong){0};
            err = ncm_mpd_item_song_from_mpd_song_copy(&song, mpd_song);
            if (err < 0) {
                ncm_song_destroy(&song);
                ncm_mpd_connection_set_error(connection,
                                             MPD_ERROR_STATE,
                                             (enum mpd_server_error)0,
                                             false,
                                             "Could not read MPD song entity");
                mpd_entity_free(entity);
                mpd_response_finish(connection->mpd);
                return -NCM_ERROR_MPD;
            }

            ncm_mpd_song_list_push(songs, &song);
            ncm_song_destroy(&song);
        }

        mpd_entity_free(entity);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_start_search_songs(MpdConnection *connection,
                                      bool exact_match) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (!mpd_search_db_songs(connection->mpd, exact_match)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return 0;
}

int32
ncm_mpd_connection_add_search_tag(MpdConnection *connection,
                                  enum NcmTagType tag, char *value) {
    enum mpd_tag_type mpd_tag;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_tag = ncm_mpd_connection_tag_type(tag);
    if (mpd_tag == MPD_TAG_UNKNOWN) {
        return -EINVAL;
    }

    if (!mpd_search_add_tag_constraint(connection->mpd, MPD_OPERATOR_DEFAULT,
                                       mpd_tag, value)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return 0;
}

int32
ncm_mpd_connection_add_search_any(MpdConnection *connection, char *value) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (!mpd_search_add_any_tag_constraint(connection->mpd,
                                           MPD_OPERATOR_DEFAULT, value)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return 0;
}

int32
ncm_mpd_connection_add_search_uri(MpdConnection *connection, char *value) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (!mpd_search_add_uri_constraint(connection->mpd, MPD_OPERATOR_DEFAULT,
                                       value)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return 0;
}

int32
ncm_mpd_connection_commit_search_songs(MpdConnection *connection,
                                       NcmMpdSongList *songs) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (songs == NULL) {
        return -EINVAL;
    }

    if (!mpd_search_commit(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

int32
ncm_mpd_connection_list_tag_values(MpdConnection *connection,
                                   enum NcmTagType tag,
                                   StringViewList *strings) {
    struct mpd_pair *pair;
    enum mpd_tag_type mpd_tag;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (strings == NULL) {
        return -EINVAL;
    }

    mpd_tag = ncm_mpd_connection_tag_type(tag);
    if (mpd_tag == MPD_TAG_UNKNOWN) {
        return -EINVAL;
    }

    if (!mpd_search_db_tags(connection->mpd, mpd_tag)) {
        return ncm_mpd_connection_check_error(connection);
    }
    if (!mpd_search_commit(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    ncm_mpd_string_list_clear(strings);
    while (true) {
        if ((pair = mpd_recv_pair_tag(connection->mpd, mpd_tag)) == NULL) {
            break;
        }

        ncm_mpd_string_list_push(strings, (char *)pair->value);
        mpd_return_pair(connection->mpd, pair);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_update_database(MpdConnection *connection, char *path,
                                   int32 *id) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (id) {
        *id = 0;
    }

    /*
     * Use send/receive instead of mpd_run_update because mpd_run_update does
     * not call mpd_response_finish if MPD returns update id 0, which breaks
     * Mopidy.
     */
    if (!mpd_send_update(connection->mpd, path)) {
        return ncm_mpd_connection_check_error(connection);
    }
    if (id) {
        *id = (int32)mpd_recv_update_id(connection->mpd);
    } else {
        mpd_recv_update_id(connection->mpd);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_get_outputs(MpdConnection *connection,
                               NcmMpdOutputList *outputs) {
    struct mpd_output *output;
    NcmMpdOutput *item;
    char *name;
    int32 old_capacity;
    int32 new_capacity;
    int32 name_len;
    int32 index;

    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));
    if (outputs == NULL) {
        return -EINVAL;
    }

    ncm_mpd_output_list_clear(outputs);
    if (!mpd_send_outputs(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    while (true) {
        if ((output = mpd_recv_output(connection->mpd)) == NULL) {
            break;
        }

        if (outputs->count >= outputs->capacity) {
            old_capacity = outputs->capacity;
            new_capacity = old_capacity*2;
            if (new_capacity < 8) {
                new_capacity = 8;
            }

            outputs->items = realloc2(outputs->items,
                                      old_capacity, new_capacity,
                                      SIZEOF(*outputs->items));
            outputs->capacity = new_capacity;
        }

        index = outputs->count;
        name = (char *)mpd_output_get_name(output);
        name_len = optional_strlen32(name);

        item = &outputs->items[index];
        item->id = (int32)mpd_output_get_id(output);
        item->name = malloc2(name_len + 1);
        item->name_len = name_len;
        item->enabled = mpd_output_get_enabled(output);

        ncm_mpd_connection_cstring_copy(item->name, name_len + 1, name);
        outputs->count += 1;

        mpd_output_free(output);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_enable_output(MpdConnection *connection, int32 id) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_enable_output(connection->mpd, (uint32)id);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_disable_output(MpdConnection *connection, int32 id) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_disable_output(connection->mpd, (uint32)id);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_play(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_play(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_play_pos(MpdConnection *connection, int32 pos) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_play_pos(connection->mpd, (uint)pos);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_play_id(MpdConnection *connection, int32 id) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_play_id(connection->mpd, (uint)id);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_toggle_pause(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_toggle_pause(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_stop(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_stop(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_next(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_next(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_previous(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_previous(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_seek_pos(MpdConnection *connection,
                            int32 pos, int32 seconds) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_seek_pos(connection->mpd, (uint32)pos, (uint32)seconds);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_set_repeat(MpdConnection *connection, bool mode) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_repeat(connection->mpd, mode);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_set_random(MpdConnection *connection, bool mode) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_random(connection->mpd, mode);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_set_single(MpdConnection *connection, bool mode) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_single(connection->mpd, mode);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_set_consume(MpdConnection *connection, bool mode) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_consume(connection->mpd, mode);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_set_crossfade(MpdConnection *connection, int32 seconds) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_crossfade(connection->mpd, (uint32)seconds);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_set_volume(MpdConnection *connection, int32 vol) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_set_volume(connection->mpd, (uint32)vol);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_change_volume(MpdConnection *connection, int32 change) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_change_volume(connection->mpd, change);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_move(MpdConnection *connection, int32 from, int32 to,
                        bool command_list_active) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (command_list_active) {
        mpd_send_move(connection->mpd, (uint32)from, (uint32)to);
        return 0;
    }

    mpd_run_move(connection->mpd, (uint32)from, (uint32)to);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_swap(MpdConnection *connection, int32 from, int32 to,
                        bool command_list_active) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (command_list_active) {
        mpd_send_swap(connection->mpd, (uint32)from, (uint32)to);
        return 0;
    }

    mpd_run_swap(connection->mpd, (uint32)from, (uint32)to);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_shuffle(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_shuffle(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_shuffle_range(MpdConnection *connection,
                                 int32 start, int32 end) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_shuffle_range(connection->mpd, (uint32)start, (uint32)end);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_clear_queue(MpdConnection *connection) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_clear(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_set_priority_id(MpdConnection *connection,
                                   int32 id, int32 prio,
                                   bool command_list_active) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (command_list_active) {
        mpd_send_prio_id(connection->mpd, (uint32)prio, (uint32)id);
        return 0;
    }

    mpd_run_prio_id(connection->mpd, (uint32)prio, (uint32)id);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_add_song(MpdConnection *connection, char *path,
                            int32 pos, bool command_list_active, int32 *id) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (id) {
        *id = 0;
    }

    if (pos < 0) {
        mpd_send_add_id(connection->mpd, path);
    } else {
        mpd_send_add_id_to(connection->mpd, path, (uint32)pos);
    }

    if (command_list_active) {
        return 0;
    }

    if (id) {
        *id = mpd_recv_song_id(connection->mpd);
    } else {
        mpd_recv_song_id(connection->mpd);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_add(MpdConnection *connection, char *path,
                       bool command_list_active, bool *added) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (added) {
        *added = false;
    }

    if (command_list_active) {
        if (added) {
            *added = mpd_send_add(connection->mpd, path);
        } else {
            mpd_send_add(connection->mpd, path);
        }
        return 0;
    }

    if (added) {
        *added = mpd_run_add(connection->mpd, path);
    } else {
        mpd_run_add(connection->mpd, path);
    }
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_delete(MpdConnection *connection,
                          int32 pos, bool command_list_active) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_send_delete(connection->mpd, (uint32)pos);
    if (command_list_active) {
        return 0;
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_clear_playlist(MpdConnection *connection,
                                  char *playlist) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_playlist_clear(connection->mpd, playlist);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_add_to_playlist(MpdConnection *connection,
                                   char *playlist, char *path,
                                   bool command_list_active) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (command_list_active) {
        mpd_send_playlist_add(connection->mpd, playlist, path);
        return 0;
    }

    mpd_run_playlist_add(connection->mpd, playlist, path);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_playlist_move(MpdConnection *connection, char *playlist,
                                 int32 from, int32 to,
                                 bool command_list_active) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_send_playlist_move(connection->mpd, playlist, (uint32)from, (uint32)to);
    if (command_list_active) {
        return 0;
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_playlist_delete(MpdConnection *connection, char *playlist,
                                   int32 pos, bool command_list_active) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_send_playlist_delete(connection->mpd, playlist, (uint32)pos);
    if (command_list_active) {
        return 0;
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_rename_playlist(MpdConnection *connection,
                                   char *from, char *to) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_rename(connection->mpd, from, to);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_delete_playlist(MpdConnection *connection,
                                   char *playlist) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_run_rm(connection->mpd, playlist);
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_load_playlist(MpdConnection *connection, char *playlist,
                                 bool *loaded) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    if (loaded) {
        *loaded = mpd_run_load(connection->mpd, playlist);
    } else {
        mpd_run_load(connection->mpd, playlist);
    }
    return ncm_mpd_connection_check_error(connection);
}

int32
ncm_mpd_connection_save_playlist(MpdConnection *connection, char *playlist) {
    NCM_MPD_RETURN_IF_ERROR(ncm_mpd_connection_require_connected(connection));

    mpd_send_save(connection->mpd, playlist);
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

#undef NCM_MPD_RETURN_IF_ERROR

#endif /* NCM_MPD_CONNECTION_C */
