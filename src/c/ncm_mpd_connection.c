#include "c/ncm_mpd_connection.h"

#include <stddef.h>
#include <string.h>

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
static bool ncm_mpd_item_list_push(NcmMpdItemList *list,
                                   NcmMpdItem *item);
static bool ncm_mpd_string_list_push(NcmMpdStringList *list,
                                     char *value);
static bool ncm_mpd_output_list_push(NcmMpdOutputList *list,
                                     struct mpd_output *output);
static bool ncm_mpd_playlist_list_push(NcmMpdPlaylistList *list,
                                       struct mpd_playlist *playlist);
static char *ncm_mpd_connection_mpd_directory(char *directory);
static bool ncm_mpd_connection_recv_song(NcmMpdConnection *connection,
                                         NcmSong *song);
static bool ncm_mpd_connection_recv_song_list(NcmMpdConnection *connection,
                                              NcmMpdSongList *songs);
static bool ncm_mpd_connection_recv_entity_song_list(
    NcmMpdConnection *connection,
    NcmMpdSongList *songs);
static bool ncm_mpd_connection_recv_item_list(NcmMpdConnection *connection,
                                              NcmMpdItemList *items);
static bool ncm_mpd_connection_recv_string_list_tag(
    NcmMpdConnection *connection,
    enum mpd_tag_type tag,
    NcmMpdStringList *strings);
static bool ncm_mpd_connection_recv_pair_list(
    NcmMpdConnection *connection,
    char *name,
    NcmMpdStringList *strings);
static char *ncm_mpd_replay_gain_mode_name(
    enum NcmMpdReplayGainMode mode);
static bool ncm_mpd_replay_gain_mode_parse(
    char *name,
    enum NcmMpdReplayGainMode *mode);

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
ncm_mpd_item_list_push(NcmMpdItemList *list, NcmMpdItem *item) {
    int32 old_capacity;
    int32 new_capacity;

    if (list == NULL) {
        return false;
    }
    if (item == NULL) {
        return false;
    }

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = (NcmMpdItem *)ncm_realloc_array(
            list->items, old_capacity, new_capacity, SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    ncm_mpd_item_init(&list->items[list->count]);
    ncm_mpd_item_move(&list->items[list->count], item);
    list->count += 1;
    return true;
}

static bool
ncm_mpd_string_list_push(NcmMpdStringList *list, char *value) {
    int32 old_capacity;
    int32 new_capacity;
    int32 value_len;
    NcmMpdString *string;

    if (list == NULL) {
        return false;
    }
    if (value == NULL) {
        return false;
    }

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = (NcmMpdString *)ncm_realloc_array(
            list->items, old_capacity, new_capacity, SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    value_len = ncm_mpd_connection_cstring_len(value);
    string = &list->items[list->count];
    string->value = (char *)ncm_malloc(value_len + 1);
    string->value_len = value_len;
    ncm_mpd_connection_cstring_copy(string->value, value_len + 1, value);
    list->count += 1;
    return true;
}


static bool
ncm_mpd_output_list_push(NcmMpdOutputList *list,
                         struct mpd_output *output) {
    int32 old_capacity;
    int32 new_capacity;
    char *name;
    int32 name_len;
    NcmMpdOutput *item;

    if (list == NULL) {
        return false;
    }
    if (output == NULL) {
        return false;
    }

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = (NcmMpdOutput *)ncm_realloc_array(
            list->items, old_capacity, new_capacity, SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    name = (char *)mpd_output_get_name(output);
    name_len = ncm_mpd_connection_cstring_len(name);

    item = &list->items[list->count];
    item->id = mpd_output_get_id(output);
    item->name = (char *)ncm_malloc(name_len + 1);
    item->name_len = name_len;
    item->enabled = mpd_output_get_enabled(output);
    ncm_mpd_connection_cstring_copy(item->name, name_len + 1, name);

    list->count += 1;
    return true;
}


static bool
ncm_mpd_playlist_list_push(NcmMpdPlaylistList *list,
                           struct mpd_playlist *playlist) {
    int32 old_capacity;
    int32 new_capacity;
    NcmPlaylist item;

    if (list == NULL) {
        return false;
    }
    if (playlist == NULL) {
        return false;
    }

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = (NcmPlaylist *)ncm_realloc_array(
            list->items, old_capacity, new_capacity, SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    ncm_playlist_init(&item);
    if (!ncm_playlist_from_mpd_playlist(&item, playlist)) {
        ncm_playlist_destroy(&item);
        return false;
    }

    ncm_playlist_init(&list->items[list->count]);
    ncm_playlist_move(&list->items[list->count], &item);
    ncm_playlist_destroy(&item);
    list->count += 1;
    return true;
}

static char *
ncm_mpd_connection_mpd_directory(char *directory) {
    if (directory == NULL) {
        return NULL;
    }
    if ((directory[0] == '/') && (directory[1] == '\0')) {
        return (char *)"";
    }

    return directory;
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

static bool
ncm_mpd_connection_recv_entity_song_list(NcmMpdConnection *connection,
                                         NcmMpdSongList *songs) {
    struct mpd_entity *entity;
    struct mpd_song *mpd_song;
    NcmSong song;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (songs == NULL) {
        return false;
    }

    ncm_mpd_song_list_clear(songs);
    for (;;) {
        entity = mpd_recv_entity(connection->mpd);
        if (entity == NULL) {
            break;
        }

        if (mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            mpd_song = (struct mpd_song *)mpd_entity_get_song(entity);
            if (mpd_song == NULL) {
                ncm_mpd_connection_set_error(
                    connection, MPD_ERROR_STATE,
                    (enum mpd_server_error)0, false,
                    (char *)"MPD song entity has no song");
                mpd_entity_free(entity);
                mpd_response_finish(connection->mpd);
                return false;
            }

            ncm_song_init(&song);
            ok = ncm_song_from_mpd_song_copy(&song, mpd_song);
            if (ok) {
                ok = ncm_mpd_song_list_push(songs, &song);
            }
            ncm_song_destroy(&song);
            if (!ok) {
                ncm_mpd_connection_set_error(
                    connection, MPD_ERROR_STATE,
                    (enum mpd_server_error)0, false,
                    (char *)"Could not read MPD song entity");
                mpd_entity_free(entity);
                mpd_response_finish(connection->mpd);
                return false;
            }
        }

        mpd_entity_free(entity);
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

static bool
ncm_mpd_connection_recv_item_list(NcmMpdConnection *connection,
                                  NcmMpdItemList *items) {
    struct mpd_entity *entity;
    NcmMpdItem item;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (items == NULL) {
        return false;
    }

    ncm_mpd_item_list_clear(items);
    for (;;) {
        entity = mpd_recv_entity(connection->mpd);
        if (entity == NULL) {
            break;
        }

        ncm_mpd_item_init(&item);
        ok = ncm_mpd_item_from_entity_copy(&item, entity);
        mpd_entity_free(entity);
        if (ok) {
            ok = ncm_mpd_item_list_push(items, &item);
        }
        ncm_mpd_item_destroy(&item);
        if (!ok) {
            ncm_mpd_connection_set_error(
                connection, MPD_ERROR_STATE,
                (enum mpd_server_error)0, false,
                (char *)"Could not read MPD directory item");
            mpd_response_finish(connection->mpd);
            return false;
        }
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

static bool
ncm_mpd_connection_recv_string_list_tag(NcmMpdConnection *connection,
                                        enum mpd_tag_type tag,
                                        NcmMpdStringList *strings) {
    struct mpd_pair *pair;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (strings == NULL) {
        return false;
    }

    ncm_mpd_string_list_clear(strings);
    for (;;) {
        pair = mpd_recv_pair_tag(connection->mpd, tag);
        if (pair == NULL) {
            break;
        }

        ok = ncm_mpd_string_list_push(strings, (char *)pair->value);
        mpd_return_pair(connection->mpd, pair);
        if (!ok) {
            ncm_mpd_connection_set_error(
                connection, MPD_ERROR_STATE,
                (enum mpd_server_error)0, false,
                (char *)"Could not read MPD tag value");
            mpd_response_finish(connection->mpd);
            return false;
        }
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}


static bool
ncm_mpd_connection_recv_pair_list(NcmMpdConnection *connection,
                                  char *name,
                                  NcmMpdStringList *strings) {
    struct mpd_pair *pair;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (strings == NULL) {
        return false;
    }

    ncm_mpd_string_list_clear(strings);
    for (;;) {
        pair = mpd_recv_pair_named(connection->mpd, name);
        if (pair == NULL) {
            break;
        }
        ok = ncm_mpd_string_list_push(strings, (char *)pair->value);
        mpd_return_pair(connection->mpd, pair);
        if (!ok) {
            mpd_response_finish(connection->mpd);
            return false;
        }
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

static char *
ncm_mpd_replay_gain_mode_name(enum NcmMpdReplayGainMode mode) {
    switch (mode) {
    case NCM_MPD_REPLAY_GAIN_OFF:
        return (char *)"off";
    case NCM_MPD_REPLAY_GAIN_TRACK:
        return (char *)"track";
    case NCM_MPD_REPLAY_GAIN_ALBUM:
        return (char *)"album";
    }

    return (char *)"off";
}

static bool
ncm_mpd_replay_gain_mode_parse(char *name,
                               enum NcmMpdReplayGainMode *mode) {
    if (mode == NULL) {
        return false;
    }
    if (name == NULL) {
        return false;
    }

    if (strcmp(name, "off") == 0) {
        *mode = NCM_MPD_REPLAY_GAIN_OFF;
        return true;
    }
    if (strcmp(name, "track") == 0) {
        *mode = NCM_MPD_REPLAY_GAIN_TRACK;
        return true;
    }
    if (strcmp(name, "album") == 0) {
        *mode = NCM_MPD_REPLAY_GAIN_ALBUM;
        return true;
    }

    return false;
}

void
ncm_mpd_string_init(NcmMpdString *string) {
    if (string == NULL) {
        return;
    }

    string->value = NULL;
    string->value_len = 0;
    return;
}

void
ncm_mpd_string_destroy(NcmMpdString *string) {
    if (string == NULL) {
        return;
    }

    if (string->value != NULL) {
        ncm_free(string->value, string->value_len + 1);
    }
    ncm_mpd_string_init(string);
    return;
}

bool
ncm_mpd_string_set(NcmMpdString *string, char *value, int32 value_len) {
    if (string == NULL) {
        return false;
    }
    if (value == NULL) {
        value = (char *)"";
        value_len = 0;
    }
    if (value_len < 0) {
        value_len = ncm_mpd_connection_cstring_len(value);
    }

    ncm_mpd_string_destroy(string);
    string->value = (char *)ncm_malloc(value_len + 1);
    string->value_len = value_len;
    if (value_len > 0) {
        ncm_memcpy(string->value, value, value_len);
    }
    string->value[value_len] = '\0';
    return true;
}

bool
ncm_mpd_string_copy(NcmMpdString *dest, NcmMpdString *source) {
    if (dest == NULL) {
        return false;
    }
    if (dest == source) {
        return true;
    }
    if (source == NULL) {
        ncm_mpd_string_destroy(dest);
        return true;
    }

    return ncm_mpd_string_set(dest, source->value, source->value_len);
}

void
ncm_mpd_string_move(NcmMpdString *dest, NcmMpdString *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mpd_string_destroy(dest);
    if (source == NULL) {
        ncm_mpd_string_init(dest);
        return;
    }
    *dest = *source;
    ncm_mpd_string_init(source);
    return;
}

void
ncm_mpd_output_init(NcmMpdOutput *output) {
    if (output == NULL) {
        return;
    }

    output->id = 0;
    output->name = NULL;
    output->name_len = 0;
    output->enabled = false;
    return;
}

void
ncm_mpd_output_destroy(NcmMpdOutput *output) {
    if (output == NULL) {
        return;
    }

    if (output->name != NULL) {
        ncm_free(output->name, output->name_len + 1);
    }
    ncm_mpd_output_init(output);
    return;
}

bool
ncm_mpd_output_set(NcmMpdOutput *output, uint32 id, char *name,
                   int32 name_len, bool enabled) {
    if (output == NULL) {
        return false;
    }
    if (name == NULL) {
        name = (char *)"";
        name_len = 0;
    }
    if (name_len < 0) {
        name_len = ncm_mpd_connection_cstring_len(name);
    }

    ncm_mpd_output_destroy(output);
    output->id = id;
    output->name = (char *)ncm_malloc(name_len + 1);
    output->name_len = name_len;
    output->enabled = enabled;
    if (name_len > 0) {
        ncm_memcpy(output->name, name, name_len);
    }
    output->name[name_len] = '\0';
    return true;
}

bool
ncm_mpd_output_copy(NcmMpdOutput *dest, NcmMpdOutput *source) {
    if (dest == NULL) {
        return false;
    }
    if (dest == source) {
        return true;
    }
    if (source == NULL) {
        ncm_mpd_output_destroy(dest);
        return true;
    }

    return ncm_mpd_output_set(dest, source->id, source->name,
                              source->name_len, source->enabled);
}

void
ncm_mpd_output_move(NcmMpdOutput *dest, NcmMpdOutput *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mpd_output_destroy(dest);
    if (source == NULL) {
        ncm_mpd_output_init(dest);
        return;
    }
    *dest = *source;
    ncm_mpd_output_init(source);
    return;
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

bool
ncm_mpd_song_list_copy(NcmMpdSongList *dest, NcmMpdSongList *source) {
    NcmMpdSongList replacement;

    if (dest == NULL) {
        return false;
    }
    if (dest == source) {
        return true;
    }

    ncm_mpd_song_list_init(&replacement);
    if (source != NULL) {
        for (int32 i = 0; i < source->count; i += 1) {
            if (!ncm_mpd_song_list_append_copy(&replacement,
                                               &source->items[i])) {
                ncm_mpd_song_list_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_mpd_song_list_destroy(dest);
    *dest = replacement;
    return true;
}

void
ncm_mpd_song_list_move(NcmMpdSongList *dest, NcmMpdSongList *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mpd_song_list_destroy(dest);
    if (source == NULL) {
        ncm_mpd_song_list_init(dest);
        return;
    }
    *dest = *source;
    ncm_mpd_song_list_init(source);
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

bool
ncm_mpd_song_list_append_copy(NcmMpdSongList *list, NcmSong *song) {
    NcmSong copy;
    bool ok;

    ncm_song_init(&copy);
    ok = ncm_song_copy(&copy, song);
    if (ok) {
        ok = ncm_mpd_song_list_push(list, &copy);
    }
    ncm_song_destroy(&copy);
    return ok;
}

void
ncm_mpd_song_list_append_move(NcmMpdSongList *list, NcmSong *song) {
    ncm_mpd_song_list_push(list, song);
    return;
}

bool
ncm_mpd_song_list_to_song_array(NcmMpdSongList *list,
                                NcmSongArray *songs) {
    NcmSongArray replacement;

    if (songs == NULL) {
        return false;
    }

    ncm_song_array_init(&replacement);
    if (list != NULL) {
        for (int32 i = 0; i < list->count; i += 1) {
            if (!ncm_song_array_append_copy(&replacement,
                                            &list->items[i])) {
                ncm_song_array_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_song_array_move(songs, &replacement);
    return true;
}

void
ncm_mpd_item_list_init(NcmMpdItemList *list) {
    if (list == NULL) {
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return;
}

void
ncm_mpd_item_list_destroy(NcmMpdItemList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_item_list_clear(list);
    if (list->items != NULL) {
        ncm_free(list->items, list->capacity*SIZEOF(*list->items));
    }

    ncm_mpd_item_list_init(list);
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

bool
ncm_mpd_item_list_copy(NcmMpdItemList *dest, NcmMpdItemList *source) {
    NcmMpdItemList replacement;

    if (dest == NULL) {
        return false;
    }
    if (dest == source) {
        return true;
    }

    ncm_mpd_item_list_init(&replacement);
    if (source != NULL) {
        for (int32 i = 0; i < source->count; i += 1) {
            if (!ncm_mpd_item_list_append_copy(&replacement,
                                               &source->items[i])) {
                ncm_mpd_item_list_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_mpd_item_list_destroy(dest);
    *dest = replacement;
    return true;
}

void
ncm_mpd_item_list_move(NcmMpdItemList *dest, NcmMpdItemList *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mpd_item_list_destroy(dest);
    if (source == NULL) {
        ncm_mpd_item_list_init(dest);
        return;
    }
    *dest = *source;
    ncm_mpd_item_list_init(source);
    return;
}

int32
ncm_mpd_item_list_count(NcmMpdItemList *list) {
    if (list == NULL) {
        return 0;
    }

    return list->count;
}

NcmMpdItem *
ncm_mpd_item_list_at(NcmMpdItemList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= list->count)) {
        return NULL;
    }

    return &list->items[idx];
}

bool
ncm_mpd_item_list_append_copy(NcmMpdItemList *list, NcmMpdItem *item) {
    NcmMpdItem copy;
    bool ok;

    ncm_mpd_item_init(&copy);
    ok = ncm_mpd_item_copy(&copy, item);
    if (ok) {
        ok = ncm_mpd_item_list_push(list, &copy);
    }
    ncm_mpd_item_destroy(&copy);
    return ok;
}

void
ncm_mpd_item_list_append_move(NcmMpdItemList *list, NcmMpdItem *item) {
    ncm_mpd_item_list_push(list, item);
    return;
}

bool
ncm_mpd_item_list_to_item_array(NcmMpdItemList *list,
                                NcmMpdItemArray *items) {
    NcmMpdItemArray replacement;

    if (items == NULL) {
        return false;
    }

    ncm_mpd_item_array_init(&replacement);
    if (list != NULL) {
        for (int32 i = 0; i < list->count; i += 1) {
            if (!ncm_mpd_item_array_append_copy(&replacement,
                                                &list->items[i])) {
                ncm_mpd_item_array_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_mpd_item_array_move(items, &replacement);
    return true;
}

bool
ncm_mpd_item_list_to_directory_array(NcmMpdItemList *list,
                                     NcmDirectoryArray *directories) {
    NcmDirectoryArray replacement;
    NcmDirectory *directory;

    if (directories == NULL) {
        return false;
    }

    ncm_directory_array_init(&replacement);
    if (list != NULL) {
        for (int32 i = 0; i < list->count; i += 1) {
            if (ncm_mpd_item_kind(&list->items[i])
                != NCM_MPD_ITEM_DIRECTORY) {
                continue;
            }
            directory = ncm_mpd_item_directory(&list->items[i]);
            if (!ncm_directory_array_append_copy(&replacement,
                                                 directory)) {
                ncm_directory_array_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_directory_array_move(directories, &replacement);
    return true;
}

void
ncm_mpd_string_list_init(NcmMpdStringList *list) {
    if (list == NULL) {
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return;
}

void
ncm_mpd_string_list_destroy(NcmMpdStringList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_string_list_clear(list);
    if (list->items != NULL) {
        ncm_free(list->items, list->capacity*SIZEOF(*list->items));
    }

    ncm_mpd_string_list_init(list);
    return;
}

void
ncm_mpd_string_list_clear(NcmMpdStringList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->count; i += 1) {
        ncm_mpd_string_destroy(&list->items[i]);
    }
    list->count = 0;
    return;
}

bool
ncm_mpd_string_list_copy(NcmMpdStringList *dest,
                         NcmMpdStringList *source) {
    NcmMpdStringList replacement;

    if (dest == NULL) {
        return false;
    }
    if (dest == source) {
        return true;
    }

    ncm_mpd_string_list_init(&replacement);
    if (source != NULL) {
        for (int32 i = 0; i < source->count; i += 1) {
            if (!ncm_mpd_string_list_append(&replacement,
                                            source->items[i].value,
                                            source->items[i].value_len)) {
                ncm_mpd_string_list_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_mpd_string_list_destroy(dest);
    *dest = replacement;
    return true;
}

void
ncm_mpd_string_list_move(NcmMpdStringList *dest,
                         NcmMpdStringList *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mpd_string_list_destroy(dest);
    if (source == NULL) {
        ncm_mpd_string_list_init(dest);
        return;
    }
    *dest = *source;
    ncm_mpd_string_list_init(source);
    return;
}

int32
ncm_mpd_string_list_count(NcmMpdStringList *list) {
    if (list == NULL) {
        return 0;
    }

    return list->count;
}

NcmMpdString *
ncm_mpd_string_list_at(NcmMpdStringList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= list->count)) {
        return NULL;
    }

    return &list->items[idx];
}

bool
ncm_mpd_string_list_append(NcmMpdStringList *list, char *value,
                           int32 value_len) {
    int32 old_capacity;
    int32 new_capacity;

    if (list == NULL) {
        return false;
    }
    if (value == NULL) {
        value = (char *)"";
        value_len = 0;
    }
    if (value_len < 0) {
        value_len = ncm_mpd_connection_cstring_len(value);
    }

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = (NcmMpdString *)ncm_realloc_array(
            list->items, old_capacity, new_capacity, SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    ncm_mpd_string_init(&list->items[list->count]);
    if (!ncm_mpd_string_set(&list->items[list->count], value, value_len)) {
        return false;
    }
    list->count += 1;
    return true;
}

bool
ncm_mpd_string_list_to_buffer_array(NcmMpdStringList *list,
                                    NcmBufferArray *strings) {
    NcmBufferArray replacement;
    NcmBuffer *buffer;

    if (strings == NULL) {
        return false;
    }

    ncm_buffer_array_init(&replacement);
    if (list != NULL) {
        for (int32 i = 0; i < list->count; i += 1) {
            buffer = ncm_buffer_array_append(&replacement);
            if (buffer == NULL) {
                ncm_buffer_array_destroy(&replacement);
                return false;
            }
            if (!ncm_buffer_set(buffer, list->items[i].value,
                                list->items[i].value_len)) {
                ncm_buffer_array_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_buffer_array_move(strings, &replacement);
    return true;
}

void
ncm_mpd_output_list_init(NcmMpdOutputList *list) {
    if (list == NULL) {
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return;
}

void
ncm_mpd_output_list_destroy(NcmMpdOutputList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_output_list_clear(list);
    if (list->items != NULL) {
        ncm_free(list->items, list->capacity*SIZEOF(*list->items));
    }

    ncm_mpd_output_list_init(list);
    return;
}

void
ncm_mpd_output_list_clear(NcmMpdOutputList *list) {
    if (list == NULL) {
        return;
    }

    for (int32 i = 0; i < list->count; i += 1) {
        ncm_mpd_output_destroy(&list->items[i]);
    }
    list->count = 0;
    return;
}

bool
ncm_mpd_output_list_copy(NcmMpdOutputList *dest,
                         NcmMpdOutputList *source) {
    NcmMpdOutputList replacement;

    if (dest == NULL) {
        return false;
    }
    if (dest == source) {
        return true;
    }

    ncm_mpd_output_list_init(&replacement);
    if (source != NULL) {
        for (int32 i = 0; i < source->count; i += 1) {
            if (!ncm_mpd_output_list_append_copy(&replacement,
                                                 &source->items[i])) {
                ncm_mpd_output_list_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_mpd_output_list_destroy(dest);
    *dest = replacement;
    return true;
}

void
ncm_mpd_output_list_move(NcmMpdOutputList *dest,
                         NcmMpdOutputList *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mpd_output_list_destroy(dest);
    if (source == NULL) {
        ncm_mpd_output_list_init(dest);
        return;
    }
    *dest = *source;
    ncm_mpd_output_list_init(source);
    return;
}

int32
ncm_mpd_output_list_count(NcmMpdOutputList *list) {
    if (list == NULL) {
        return 0;
    }

    return list->count;
}

NcmMpdOutput *
ncm_mpd_output_list_at(NcmMpdOutputList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= list->count)) {
        return NULL;
    }

    return &list->items[idx];
}

bool
ncm_mpd_output_list_append_copy(NcmMpdOutputList *list,
                                NcmMpdOutput *output) {
    NcmMpdOutput copy;
    bool ok;

    if (list == NULL) {
        return false;
    }
    if (output == NULL) {
        return false;
    }

    ncm_mpd_output_init(&copy);
    ok = ncm_mpd_output_copy(&copy, output);
    if (ok) {
        ncm_mpd_output_list_append_move(list, &copy);
    }
    ncm_mpd_output_destroy(&copy);
    return ok;
}

void
ncm_mpd_output_list_append_move(NcmMpdOutputList *list,
                                NcmMpdOutput *output) {
    int32 old_capacity;
    int32 new_capacity;

    if (list == NULL) {
        return;
    }
    if (output == NULL) {
        return;
    }

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = (NcmMpdOutput *)ncm_realloc_array(
            list->items, old_capacity, new_capacity, SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    ncm_mpd_output_init(&list->items[list->count]);
    ncm_mpd_output_move(&list->items[list->count], output);
    list->count += 1;
    return;
}

void
ncm_mpd_playlist_list_init(NcmMpdPlaylistList *list) {
    if (list == NULL) {
        return;
    }

    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
    return;
}

void
ncm_mpd_playlist_list_destroy(NcmMpdPlaylistList *list) {
    if (list == NULL) {
        return;
    }

    ncm_mpd_playlist_list_clear(list);
    if (list->items != NULL) {
        ncm_free(list->items, list->capacity*SIZEOF(*list->items));
    }

    ncm_mpd_playlist_list_init(list);
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

bool
ncm_mpd_playlist_list_copy(NcmMpdPlaylistList *dest,
                           NcmMpdPlaylistList *source) {
    NcmMpdPlaylistList replacement;

    if (dest == NULL) {
        return false;
    }
    if (dest == source) {
        return true;
    }

    ncm_mpd_playlist_list_init(&replacement);
    if (source != NULL) {
        for (int32 i = 0; i < source->count; i += 1) {
            if (!ncm_mpd_playlist_list_append_copy(&replacement,
                                                   &source->items[i])) {
                ncm_mpd_playlist_list_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_mpd_playlist_list_destroy(dest);
    *dest = replacement;
    return true;
}

void
ncm_mpd_playlist_list_move(NcmMpdPlaylistList *dest,
                           NcmMpdPlaylistList *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mpd_playlist_list_destroy(dest);
    if (source == NULL) {
        ncm_mpd_playlist_list_init(dest);
        return;
    }
    *dest = *source;
    ncm_mpd_playlist_list_init(source);
    return;
}

int32
ncm_mpd_playlist_list_count(NcmMpdPlaylistList *list) {
    if (list == NULL) {
        return 0;
    }

    return list->count;
}

NcmPlaylist *
ncm_mpd_playlist_list_at(NcmMpdPlaylistList *list, int32 idx) {
    if (list == NULL) {
        return NULL;
    }
    if ((idx < 0) || (idx >= list->count)) {
        return NULL;
    }

    return &list->items[idx];
}

bool
ncm_mpd_playlist_list_append_copy(NcmMpdPlaylistList *list,
                                  NcmPlaylist *playlist) {
    NcmPlaylist copy;
    bool ok;

    if (list == NULL) {
        return false;
    }
    if (playlist == NULL) {
        return false;
    }

    ncm_playlist_init(&copy);
    ok = ncm_playlist_copy(&copy, playlist);
    if (ok) {
        ncm_mpd_playlist_list_append_move(list, &copy);
    }
    ncm_playlist_destroy(&copy);
    return ok;
}

void
ncm_mpd_playlist_list_append_move(NcmMpdPlaylistList *list,
                                  NcmPlaylist *playlist) {
    int32 old_capacity;
    int32 new_capacity;

    if (list == NULL) {
        return;
    }
    if (playlist == NULL) {
        return;
    }

    if (list->count >= list->capacity) {
        old_capacity = list->capacity;
        new_capacity = old_capacity*2;
        if (new_capacity < 8) {
            new_capacity = 8;
        }

        list->items = (NcmPlaylist *)ncm_realloc_array(
            list->items, old_capacity, new_capacity, SIZEOF(*list->items));
        list->capacity = new_capacity;
    }

    ncm_playlist_init(&list->items[list->count]);
    ncm_playlist_move(&list->items[list->count], playlist);
    list->count += 1;
    return;
}

bool
ncm_mpd_playlist_list_to_playlist_array(NcmMpdPlaylistList *list,
                                        NcmPlaylistArray *playlists) {
    NcmPlaylistArray replacement;

    if (playlists == NULL) {
        return false;
    }

    ncm_playlist_array_init(&replacement);
    if (list != NULL) {
        for (int32 i = 0; i < list->count; i += 1) {
            if (!ncm_playlist_array_append_copy(&replacement,
                                                &list->items[i])) {
                ncm_playlist_array_destroy(&replacement);
                return false;
            }
        }
    }

    ncm_playlist_array_move(playlists, &replacement);
    return true;
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
ncm_mpd_connection_get_fd(NcmMpdConnection *connection, int32 *fd) {
    if (fd == NULL) {
        return false;
    }

    *fd = ncm_mpd_connection_fd(connection);
    return *fd >= 0;
}

bool
ncm_mpd_connection_set_timeout(NcmMpdConnection *connection,
                               uint32 timeout_ms) {
    if (connection == NULL) {
        return false;
    }
    if (connection->mpd == NULL) {
        return true;
    }

    mpd_connection_set_timeout(connection->mpd, timeout_ms);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_send_idle(NcmMpdConnection *connection,
                             enum mpd_idle events) {
    (void)events;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_idle(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return true;
}

bool
ncm_mpd_connection_recv_idle(NcmMpdConnection *connection,
                             bool disable_timeout,
                             enum mpd_idle *out_events) {
    enum mpd_idle events;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    events = (enum mpd_idle)mpd_recv_idle(connection->mpd, disable_timeout);
    mpd_response_finish(connection->mpd);
    ok = ncm_mpd_connection_check_error(connection);
    if (out_events != NULL) {
        *out_events = events;
    }

    return ok;
}

bool
ncm_mpd_connection_idle(NcmMpdConnection *connection,
                        enum mpd_idle events,
                        enum mpd_idle *out_events) {
    if (!ncm_mpd_connection_send_idle(connection, events)) {
        return false;
    }

    return ncm_mpd_connection_recv_idle(connection, true, out_events);
}

bool
ncm_mpd_connection_noidle(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_noidle(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return true;
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


uint32
ncm_mpd_connection_version(NcmMpdConnection *connection) {
    unsigned *version;

    if (!ncm_mpd_connection_is_connected(connection)) {
        return 0;
    }

    version = (unsigned *)mpd_connection_get_server_version(connection->mpd);
    if (version == NULL) {
        return 0;
    }

    return version[1];
}

bool
ncm_mpd_connection_send_password(NcmMpdConnection *connection,
                                 char *password) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (password == NULL) {
        return true;
    }
    if (password[0] == '\0') {
        return true;
    }

    if (!mpd_run_password(connection->mpd, password)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_start_command_list(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_command_list_begin(connection->mpd, true)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_commit_command_list(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_command_list_end(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_get_supported_extensions(NcmMpdConnection *connection,
                                            NcmMpdStringList *strings) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_command(connection->mpd, "decoders", NULL)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_pair_list(connection,
                                             (char *)"suffix", strings);
}

bool
ncm_mpd_connection_get_replay_gain_mode(NcmMpdConnection *connection,
                                        enum NcmMpdReplayGainMode *mode) {
    struct mpd_pair *pair;
    bool parsed;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (mode == NULL) {
        return false;
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

    parsed = ncm_mpd_replay_gain_mode_parse((char *)pair->value, mode);
    mpd_return_pair(connection->mpd, pair);
    mpd_response_finish(connection->mpd);
    if (!parsed) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"Unknown replay gain mode");
        return false;
    }

    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_set_replay_gain_mode(NcmMpdConnection *connection,
                                        enum NcmMpdReplayGainMode mode) {
    char *name;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    name = ncm_mpd_replay_gain_mode_name(mode);
    if (!mpd_send_command(connection->mpd, "replay_gain_mode", name, NULL)) {
        return ncm_mpd_connection_check_error(connection);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_get_playlists(NcmMpdConnection *connection,
                                 NcmMpdPlaylistList *playlists) {
    struct mpd_playlist *playlist;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (playlists == NULL) {
        return false;
    }

    if (!mpd_send_list_playlists(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    ncm_mpd_playlist_list_clear(playlists);
    for (;;) {
        playlist = mpd_recv_playlist(connection->mpd);
        if (playlist == NULL) {
            break;
        }
        ok = ncm_mpd_playlist_list_push(playlists, playlist);
        mpd_playlist_free(playlist);
        if (!ok) {
            mpd_response_finish(connection->mpd);
            return false;
        }
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_list_all_song_uris(NcmMpdConnection *connection,
                                      char *path,
                                      NcmMpdStringList *strings) {
    char *directory;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (strings == NULL) {
        return false;
    }

    directory = ncm_mpd_connection_mpd_directory(path);
    if (!mpd_send_list_all(connection->mpd, directory)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_pair_list(connection,
                                             (char *)"file", strings);
}

bool
ncm_mpd_connection_get_url_handlers(NcmMpdConnection *connection,
                                    NcmMpdStringList *strings) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_list_url_schemes(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_pair_list(connection,
                                             (char *)"handler", strings);
}

bool
ncm_mpd_connection_get_tag_types(NcmMpdConnection *connection,
                                 NcmMpdStringList *strings) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_list_tag_types(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_pair_list(connection,
                                             (char *)"tagtype", strings);
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
ncm_mpd_connection_get_queue(NcmMpdConnection *connection,
                             NcmMpdSongList *songs) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_send_list_queue_meta(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
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
ncm_mpd_connection_get_directory(NcmMpdConnection *connection,
                                 char *path,
                                 NcmMpdItemList *items) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (items == NULL) {
        return false;
    }

    if (!mpd_send_list_meta(connection->mpd,
                            ncm_mpd_connection_mpd_directory(path))) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_item_list(connection, items);
}

bool
ncm_mpd_connection_get_directory_recursive(NcmMpdConnection *connection,
                                           char *path,
                                           NcmMpdSongList *songs) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (songs == NULL) {
        return false;
    }

    if (!mpd_send_list_all_meta(connection->mpd,
                                ncm_mpd_connection_mpd_directory(path))) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_entity_song_list(connection, songs);
}

bool
ncm_mpd_connection_get_directory_songs(NcmMpdConnection *connection,
                                       char *path,
                                       NcmMpdSongList *songs) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (songs == NULL) {
        return false;
    }

    if (!mpd_send_list_meta(connection->mpd,
                            ncm_mpd_connection_mpd_directory(path))) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

bool
ncm_mpd_connection_search_songs(NcmMpdConnection *connection,
                                bool exact_match,
                                enum mpd_tag_type tag,
                                char *value,
                                NcmMpdSongList *songs) {
    if (!ncm_mpd_connection_start_search_songs(connection, exact_match)) {
        return false;
    }
    if (!ncm_mpd_connection_add_search_tag(connection, tag, value)) {
        return false;
    }

    return ncm_mpd_connection_commit_search_songs(connection, songs);
}

bool
ncm_mpd_connection_find_songs(NcmMpdConnection *connection,
                              enum mpd_tag_type tag,
                              char *value,
                              NcmMpdSongList *songs) {
    return ncm_mpd_connection_search_songs(connection, true, tag, value, songs);
}

bool
ncm_mpd_connection_list_all_songs(NcmMpdConnection *connection,
                                  char *path,
                                  NcmMpdSongList *songs) {
    return ncm_mpd_connection_get_directory_recursive(connection, path, songs);
}

bool
ncm_mpd_connection_start_search_songs(NcmMpdConnection *connection,
                                      bool exact_match) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_search_db_songs(connection->mpd, exact_match)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return true;
}

bool
ncm_mpd_connection_start_search_tags(NcmMpdConnection *connection,
                                     enum mpd_tag_type tag) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_search_db_tags(connection->mpd, tag)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return true;
}

bool
ncm_mpd_connection_add_search_tag(NcmMpdConnection *connection,
                                  enum mpd_tag_type tag,
                                  char *value) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_search_add_tag_constraint(connection->mpd, MPD_OPERATOR_DEFAULT,
                                       tag, value)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return true;
}

bool
ncm_mpd_connection_add_search_any(NcmMpdConnection *connection,
                                  char *value) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_search_add_any_tag_constraint(connection->mpd,
                                           MPD_OPERATOR_DEFAULT, value)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return true;
}

bool
ncm_mpd_connection_add_search_uri(NcmMpdConnection *connection,
                                  char *value) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (!mpd_search_add_uri_constraint(connection->mpd, MPD_OPERATOR_DEFAULT,
                                       value)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return true;
}

bool
ncm_mpd_connection_commit_search_songs(NcmMpdConnection *connection,
                                       NcmMpdSongList *songs) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (songs == NULL) {
        return false;
    }

    if (!mpd_search_commit(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_song_list(connection, songs);
}

bool
ncm_mpd_connection_list_tag_values(NcmMpdConnection *connection,
                                   enum mpd_tag_type tag,
                                   NcmMpdStringList *strings) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (strings == NULL) {
        return false;
    }

    if (!mpd_search_db_tags(connection->mpd, tag)) {
        return ncm_mpd_connection_check_error(connection);
    }
    if (!mpd_search_commit(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_string_list_tag(connection, tag, strings);
}

bool
ncm_mpd_connection_commit_search_tags(NcmMpdConnection *connection,
                                      enum mpd_tag_type tag,
                                      NcmMpdStringList *strings) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (strings == NULL) {
        return false;
    }

    if (!mpd_search_commit(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    return ncm_mpd_connection_recv_string_list_tag(connection, tag, strings);
}

bool
ncm_mpd_connection_update_database(NcmMpdConnection *connection,
                                   char *path,
                                   uint32 *id) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (id != NULL) {
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
    if (id != NULL) {
        *id = mpd_recv_update_id(connection->mpd);
    } else {
        mpd_recv_update_id(connection->mpd);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_rescan_database(NcmMpdConnection *connection,
                                   char *path,
                                   uint32 *id) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (id != NULL) {
        *id = 0;
    }

    if (!mpd_send_rescan(connection->mpd, path)) {
        return ncm_mpd_connection_check_error(connection);
    }
    if (id != NULL) {
        *id = mpd_recv_update_id(connection->mpd);
    } else {
        mpd_recv_update_id(connection->mpd);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}


bool
ncm_mpd_connection_get_outputs(NcmMpdConnection *connection,
                               NcmMpdOutputList *outputs) {
    struct mpd_output *output;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }
    if (outputs == NULL) {
        return false;
    }

    ncm_mpd_output_list_clear(outputs);
    if (!mpd_send_outputs(connection->mpd)) {
        return ncm_mpd_connection_check_error(connection);
    }

    for (;;) {
        output = mpd_recv_output(connection->mpd);
        if (output == NULL) {
            break;
        }

        ok = ncm_mpd_output_list_push(outputs, output);
        mpd_output_free(output);
        if (!ok) {
            ncm_mpd_connection_set_error(
                connection, MPD_ERROR_STATE,
                (enum mpd_server_error)0, false,
                (char *)"Could not read MPD output");
            mpd_response_finish(connection->mpd);
            return false;
        }
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_enable_output(NcmMpdConnection *connection,
                                 uint32 id) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_enable_output(connection->mpd, id);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_disable_output(NcmMpdConnection *connection,
                                  uint32 id) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_disable_output(connection->mpd, id);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_toggle_output(NcmMpdConnection *connection,
                                 uint32 id) {
    NcmMpdOutputList outputs;
    bool found;
    bool enabled;
    bool ok;

    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    ncm_mpd_output_list_init(&outputs);
    ok = ncm_mpd_connection_get_outputs(connection, &outputs);
    found = false;
    enabled = false;
    if (ok) {
        for (int32 i = 0; i < outputs.count; i += 1) {
            if (outputs.items[i].id == id) {
                found = true;
                enabled = outputs.items[i].enabled;
                break;
            }
        }
    }
    ncm_mpd_output_list_destroy(&outputs);
    if (!ok) {
        return false;
    }
    if (!found) {
        ncm_mpd_connection_set_error(connection, MPD_ERROR_STATE,
                                     (enum mpd_server_error)0, false,
                                     (char *)"MPD output not found");
        return false;
    }

    if (enabled) {
        return ncm_mpd_connection_disable_output(connection, id);
    }
    return ncm_mpd_connection_enable_output(connection, id);
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

bool
ncm_mpd_connection_move(NcmMpdConnection *connection,
                        uint32 from,
                        uint32 to,
                        bool command_list_active) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (command_list_active) {
        mpd_send_move(connection->mpd, from, to);
        return true;
    }

    mpd_run_move(connection->mpd, from, to);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_swap(NcmMpdConnection *connection,
                        uint32 from,
                        uint32 to,
                        bool command_list_active) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (command_list_active) {
        mpd_send_swap(connection->mpd, from, to);
        return true;
    }

    mpd_run_swap(connection->mpd, from, to);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_shuffle(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_shuffle(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_shuffle_range(NcmMpdConnection *connection,
                                  uint32 start,
                                  uint32 end) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_shuffle_range(connection->mpd, start, end);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_clear_queue(NcmMpdConnection *connection) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_clear(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_set_priority_id(NcmMpdConnection *connection,
                                   uint32 id,
                                   int32 prio,
                                   bool command_list_active) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (command_list_active) {
        mpd_send_prio_id(connection->mpd, prio, id);
        return true;
    }

    mpd_run_prio_id(connection->mpd, prio, id);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_add_song(NcmMpdConnection *connection,
                            char *path,
                            int32 pos,
                            bool command_list_active,
                            int32 *id) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (id != NULL) {
        *id = 0;
    }

    if (pos < 0) {
        mpd_send_add_id(connection->mpd, path);
    } else {
        mpd_send_add_id_to(connection->mpd, path, (uint32)pos);
    }

    if (command_list_active) {
        return true;
    }

    if (id != NULL) {
        *id = mpd_recv_song_id(connection->mpd);
    } else {
        mpd_recv_song_id(connection->mpd);
    }
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_add(NcmMpdConnection *connection,
                       char *path,
                       bool command_list_active,
                       bool *added) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (added != NULL) {
        *added = false;
    }

    if (command_list_active) {
        if (added != NULL) {
            *added = mpd_send_add(connection->mpd, path);
        } else {
            mpd_send_add(connection->mpd, path);
        }
        return true;
    }

    if (added != NULL) {
        *added = mpd_run_add(connection->mpd, path);
    } else {
        mpd_run_add(connection->mpd, path);
    }
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_delete(NcmMpdConnection *connection,
                          uint32 pos,
                          bool command_list_active) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_send_delete(connection->mpd, pos);
    if (command_list_active) {
        return true;
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_delete_range(NcmMpdConnection *connection,
                                uint32 begin,
                                uint32 end,
                                bool command_list_active) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_send_delete_range(connection->mpd, begin, end);
    if (command_list_active) {
        return true;
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_clear_playlist(NcmMpdConnection *connection,
                                  char *playlist) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_playlist_clear(connection->mpd, playlist);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_add_to_playlist(NcmMpdConnection *connection,
                                   char *playlist,
                                   char *path,
                                   bool command_list_active) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (command_list_active) {
        mpd_send_playlist_add(connection->mpd, playlist, path);
        return true;
    }

    mpd_run_playlist_add(connection->mpd, playlist, path);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_playlist_move(NcmMpdConnection *connection,
                                 char *playlist,
                                 uint32 from,
                                 uint32 to,
                                 bool command_list_active) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_send_playlist_move(connection->mpd, playlist, from, to);
    if (command_list_active) {
        return true;
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_playlist_delete(NcmMpdConnection *connection,
                                   char *playlist,
                                   uint32 pos,
                                   bool command_list_active) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_send_playlist_delete(connection->mpd, playlist, pos);
    if (command_list_active) {
        return true;
    }

    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_rename_playlist(NcmMpdConnection *connection,
                                   char *from,
                                   char *to) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_rename(connection->mpd, from, to);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_delete_playlist(NcmMpdConnection *connection,
                                   char *playlist) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_run_rm(connection->mpd, playlist);
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_load_playlist(NcmMpdConnection *connection,
                                 char *playlist,
                                 bool *loaded) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    if (loaded != NULL) {
        *loaded = mpd_run_load(connection->mpd, playlist);
    } else {
        mpd_run_load(connection->mpd, playlist);
    }
    return ncm_mpd_connection_check_error(connection);
}

bool
ncm_mpd_connection_save_playlist(NcmMpdConnection *connection,
                                 char *playlist) {
    if (!ncm_mpd_connection_require_connected(connection)) {
        return false;
    }

    mpd_send_save(connection->mpd, playlist);
    mpd_response_finish(connection->mpd);
    return ncm_mpd_connection_check_error(connection);
}

