#if !defined(NCM_C_H)
#define NCM_C_H

#include "cbase.h"

#include "configura.h"

#include <mpd/client.h>
#include <mpd/tag.h>
#include <regex.h>

typedef struct NcmStringView {
    char *data;
    int32 len;
} NcmStringView;

typedef struct NcmError {
    char message[256];
    int32 code;
} NcmError;

/*
 * Project-local error codes are positive. Fallible functions return their
 * negative value as status, leaving non-negative values for success payloads.
 */
enum NcmErrorCode {
    NCM_ERROR_OK = 0,
    NCM_ERROR_PROJECT_BASE = 4096,
    NCM_ERROR_INVALID_STATE = NCM_ERROR_PROJECT_BASE,
    NCM_ERROR_NOT_FOUND,
    NCM_ERROR_UNAVAILABLE,
    NCM_ERROR_CANCELLED,
    NCM_ERROR_PARSE,
    NCM_ERROR_MPD,
    NCM_ERROR_TAGLIB,
    NCM_ERROR_NETWORK,
    NCM_ERROR_EXTERNAL_COMMAND,
};

void ncm_error_clear(NcmError *ncm_error);
void ncm_error_set(NcmError *ncm_error, int32 code,
                   char *message, int32 message_len);
bool ncm_error_is_set(NcmError *ncm_error);
int32 ncm_error_code_from_status(int32 status);
int32 ncm_status_from_error_code(int32 code);
int32 ncm_error_set_code(NcmError *ncm_error, int32 code,
                         char *message, int32 message_len);
int32 ncm_error_status(NcmError *ncm_error);
int32 ncm_error_set_status(NcmError *ncm_error, int32 status,
                           char *message, int32 message_len);
int32 ncm_error_ok(NcmError *ncm_error);

void stupid_string_free(char **data, int32 *len, int32 *cap);
void stupid_string_set(char **dest, int32 *dest_len, int32 *dest_cap,
                       char *source, int32 source_len);

#include <mpd/tag.h>

struct mpd_song;

#define ENUM_NAME NcmTagsField
#define ENUM_PREFIX_ NCM_TAGS_FIELD_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                  \
    XX(NCM_TAGS_FIELD_TITLE, Title)                  \
    XX(NCM_TAGS_FIELD_ARTIST, Artist)                \
    XX(NCM_TAGS_FIELD_ALBUM_ARTIST, Album Artist)    \
    XX(NCM_TAGS_FIELD_ALBUM, Album)                  \
    XX(NCM_TAGS_FIELD_DATE, Date)                    \
    XX(NCM_TAGS_FIELD_TRACK, Track)                  \
    XX(NCM_TAGS_FIELD_GENRE, Genre)                  \
    XX(NCM_TAGS_FIELD_COMPOSER, Composer)            \
    XX(NCM_TAGS_FIELD_PERFORMER, Performer)          \
    XX(NCM_TAGS_FIELD_DISC, Disc)                    \
    XX(NCM_TAGS_FIELD_COMMENT, Comment)
#include "cbase/xenums.c"

#define ENUM_NAME NcmTagsReadResult
#define ENUM_PREFIX_ NCM_TAGS_READ_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                      \
    XX(NCM_TAGS_READ_OK)                 \
    XX(NCM_TAGS_READ_OPEN_FAILED)        \
    XX(NCM_TAGS_READ_NOT_FOUND)
#include "cbase/xenums.c"

typedef struct NcmTagsReplayGainInfo {
    NcmStringView reference_loudness;
    NcmStringView track_gain;
    NcmStringView track_peak;
    NcmStringView album_gain;
    NcmStringView album_peak;
} NcmTagsReplayGainInfo;

typedef void NcmTagsValueCallback(char *value, int32 value_len, void *user);
typedef bool NcmTagsGetFieldCallback(enum NcmTagsField field,
                                     int32 idx, NcmStringView *value,
                                     void *user);

void ncm_tags_set_attribute(struct mpd_song *song, char *name, char *value);
enum NcmTagsReadResult ncm_tags_read_lyrics(char *path,
                                            NcmTagsValueCallback *callback,
                                            void *user);
int32 ncm_tags_read_song(struct mpd_song *song);
int32 ncm_tags_write(char *music_dir, char *uri, bool is_from_database,
                     char *directory, char *new_name,
                     NcmTagsGetFieldCallback *callback, void *user);

#include <mpd/tag.h>

#define ENUM_NAME NcmItemType
#define ENUM_PREFIX_ NCM_ITEM_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                         \
    XX(NCM_ITEM_DIRECTORY)                  \
    XX(NCM_ITEM_SONG)                       \
    XX(NCM_ITEM_PLAYLIST)
#include "cbase/xenums.c"

#define ENUM_NAME NcmSongGetter
#define ENUM_PREFIX_ NCM_SONG_GETTER_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                      \
    XX(NCM_SONG_GETTER_NONE, none)                       \
    XX(NCM_SONG_GETTER_LENGTH, Length)                   \
    XX(NCM_SONG_GETTER_DIRECTORY, Directory)             \
    XX(NCM_SONG_GETTER_NAME, Filename)                   \
    XX(NCM_SONG_GETTER_URI, URI)                         \
    XX(NCM_SONG_GETTER_ARTIST, Artist)                   \
    XX(NCM_SONG_GETTER_ALBUM_ARTIST, Album Artist)       \
    XX(NCM_SONG_GETTER_TITLE, Title)                     \
    XX(NCM_SONG_GETTER_ALBUM, Album)                     \
    XX(NCM_SONG_GETTER_DATE, Date)                       \
    XX(NCM_SONG_GETTER_TRACK_NUMBER, Track Number)       \
    XX(NCM_SONG_GETTER_TRACK, Track)                     \
    XX(NCM_SONG_GETTER_GENRE, Genre)                     \
    XX(NCM_SONG_GETTER_COMPOSER, Composer)               \
    XX(NCM_SONG_GETTER_PERFORMER, Performer)             \
    XX(NCM_SONG_GETTER_DISC, Disc)                       \
    XX(NCM_SONG_GETTER_COMMENT, Comment)                 \
    XX(NCM_SONG_GETTER_PRIORITY, Priority)
#include "cbase/xenums.c"

int32 ncm_channels_to_string(int32 channels, char *buffer, int32 buffer_cap);
int32 ncm_color_index_from_char(char c);
char *ncm_tag_type_name(enum mpd_tag_type tag);
enum mpd_tag_type ncm_char_to_tag_type(char c);
enum NcmSongGetter ncm_song_getter_from_char(char c);
enum mpd_tag_type ncm_song_getter_to_tag_type(enum NcmSongGetter getter);
enum NcmTagsField ncm_tags_field_from_char(char c);
enum NcmTagsField ncm_tags_field_from_tag_type(enum mpd_tag_type tag);
enum mpd_tag_type ncm_tags_field_to_tag_type(enum NcmTagsField field);
enum NcmSongGetter ncm_tags_field_to_song_getter(enum NcmTagsField field);
enum NcmTagsField ncm_song_getter_to_tags_field(enum NcmSongGetter getter);

#include <mpd/tag.h>

struct mpd_song;

#define ENUM_NAME NcmSongOwnership
#define ENUM_PREFIX_ NCM_SONG_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                             \
    XX(NCM_SONG_BORROWED)                       \
    XX(NCM_SONG_OWNED)
#include "cbase/xenums.c"

typedef struct NcmSongTag {
    char *value;
    int32 value_len;
    enum mpd_tag_type type;
} NcmSongTag;

typedef struct NcmSong {
    char *uri;
    int32 uri_len;

    NcmSongTag *tags;
    int32 tags_len;
    int32 tags_cap;

    int32 duration;
    int32 position;
    int32 id;
    int32 priority;
    time_t last_modified;
} NcmSong;

void ncm_song_destroy(NcmSong *song);
void ncm_song_move(NcmSong *dest, NcmSong *source);
int32 ncm_song_copy(NcmSong *dest, NcmSong *source);
int32 ncm_song_from_mpd_song(NcmSong *dest, struct mpd_song *source);
int32 ncm_song_from_mpd_song_copy(NcmSong *dest, struct mpd_song *source);
int32 ncm_song_set_uri(NcmSong *song, char *uri, int32 uri_len);
int32 ncm_song_add_tag(NcmSong *song, enum mpd_tag_type type,
                       char *value, int32 value_len);
void ncm_song_set_duration(NcmSong *song, int32 duration);
void ncm_song_set_position(NcmSong *song, int32 position);
void ncm_song_set_id(NcmSong *song, int32 id);
void ncm_song_set_priority(NcmSong *song, int32 priority);
void ncm_song_set_mtime(NcmSong *song, time_t last_modified);
int32 ncm_song_duration(NcmSong *song);
int32 ncm_song_position(NcmSong *song);
int32 ncm_song_id(NcmSong *song);
int32 ncm_song_priority(NcmSong *song);
time_t ncm_song_mtime(NcmSong *song);
bool ncm_song_is_empty(NcmSong *song);

bool ncm_song_has_tag_view(NcmSong *song, enum mpd_tag_type tag,
                           int32 idx, NcmStringView *view);
bool ncm_song_has_uri_view(NcmSong *song, int32 idx, NcmStringView *view);
bool ncm_song_has_name_view(NcmSong *song, int32 idx, NcmStringView *view);
bool ncm_song_has_directory_view(NcmSong *song, int32 idx,
                                 NcmStringView *view);
bool ncm_song_is_from_database(NcmSong *song);
bool ncm_song_is_stream(NcmSong *song);

int32 ncm_song_numeric_tag_len(char *tag, int32 tag_len);
int32 ncm_song_format_numeric_tag(char *buffer, int32 buffer_cap,
                                  char *tag, int32 tag_len);
int32 ncm_song_show_time(int32 length, char *buffer, int32 buffer_cap);
StrBuilder ncm_song_getter_buffer(NcmSong *song,
                                  enum NcmSongGetter getter, int32 idx);
StrBuilder ncm_song_tags_buffer(NcmSong *song, enum NcmSongGetter getter,
                                char *separator, int32 separator_len,
                                bool show_duplicates);
bool ncm_song_is_equal(NcmSong *a, NcmSong *b);

typedef struct NcmMutableSongTag {
    char *original;
    char *value;

    int32 original_len;
    int32 value_len;
    int32 idx;

    enum NcmTagsField field;
    bool modified;
} NcmMutableSongTag;

typedef struct NcmMutableSong {
    char *uri;
    char *directory;
    char *name;
    char *new_name;

    int32 uri_len;
    int32 directory_len;
    int32 name_len;
    int32 new_name_len;

    int32 mtime;
    int32 duration;
    bool is_from_database;

    NcmMutableSongTag *tags;
    int32 tags_len;
    int32 tags_cap;
} NcmMutableSong;

void ncm_mutable_song_destroy(NcmMutableSong *song);
int32 ncm_mutable_song_copy(NcmMutableSong *dest, NcmMutableSong *source);
void ncm_mutable_song_move(NcmMutableSong *dest, NcmMutableSong *source);

int32 ncm_mutable_song_set_uri(NcmMutableSong *song, char *uri, int32 uri_len);
int32 ncm_mutable_song_set_directory(NcmMutableSong *song, char *directory,
                                     int32 directory_len);
int32 ncm_mutable_song_set_name(NcmMutableSong *song, char *name,
                                int32 name_len);
void ncm_mutable_song_set_from_database(NcmMutableSong *song,
                                        bool is_from_database);

int32 ncm_mutable_song_set_original_tag(NcmMutableSong *song,
                                        enum NcmTagsField field, int32 idx,
                                        char *value, int32 value_len);
int32 ncm_mutable_song_set_tag(NcmMutableSong *song, enum NcmTagsField field,
                               int32 idx, char *value, int32 value_len);
int32 ncm_mutable_song_set_tags(NcmMutableSong *song, enum NcmTagsField field,
                                char *value, int32 value_len,
                                char *separator, int32 separator_len);
bool ncm_mutable_song_has_tag_view(NcmMutableSong *song,
                                   enum NcmTagsField field, int32 idx,
                                   NcmStringView *view);
void ncm_mutable_song_get_tag_buffer(NcmMutableSong *song,
                                     enum NcmTagsField field, int32 idx,
                                     StrBuilder *buffer);
StrBuilder ncm_mutable_song_tags_buffer(NcmMutableSong *song,
                                        enum NcmTagsField field,
                                        char *separator, int32 separator_len,
                                        bool show_duplicates);
int32 ncm_mutable_song_load_originals_from_song(NcmMutableSong *dest,
                                               NcmSong *source);

int32 ncm_mutable_song_set_new_name(NcmMutableSong *song,
                                    char *new_name, int32 new_name_len);
bool ncm_mutable_song_has_new_name_view(NcmMutableSong *song,
                                        NcmStringView *view);

void ncm_mutable_song_set_duration(NcmMutableSong *song, int32 duration);
int32 ncm_mutable_song_duration(NcmMutableSong *song);
void ncm_mutable_song_set_mtime(NcmMutableSong *song, int32 mtime);
int32 ncm_mutable_song_mtime(NcmMutableSong *song);

bool ncm_mutable_song_is_modified(NcmMutableSong *song);
void ncm_mutable_song_clear_modifications(NcmMutableSong *song);
int32 ncm_mutable_song_write(NcmMutableSong *song, char *music_dir);

struct mpd_directory;

typedef struct NcmDirectory {
    char *path;
    int32 path_len;
    time_t last_modified;
} NcmDirectory;

void ncm_directory_destroy(NcmDirectory *directory);
void ncm_directory_move(NcmDirectory *dest, NcmDirectory *source);
int32 ncm_directory_set(NcmDirectory *directory, char *path,
                        int32 path_len, time_t last_modified);
int32 ncm_directory_copy(NcmDirectory *dest, NcmDirectory *source);
bool ncm_directory_has_path_view(NcmDirectory *directory, NcmStringView *view);
time_t ncm_directory_last_modified(NcmDirectory *directory);
int32 ncm_directory_from_mpd_directory(NcmDirectory *dest,
                                       struct mpd_directory *source);

struct mpd_playlist;

typedef struct NcmPlaylist {
    char *path;
    int32 path_len;
    time_t last_modified;
} NcmPlaylist;

void ncm_playlist_destroy(NcmPlaylist *playlist);
void ncm_playlist_move(NcmPlaylist *dest, NcmPlaylist *source);
int32 ncm_playlist_set(NcmPlaylist *playlist, char *path,
                       int32 path_len, time_t last_modified);
int32 ncm_playlist_copy(NcmPlaylist *dest, NcmPlaylist *source);
bool ncm_playlist_has_path_view(NcmPlaylist *playlist, NcmStringView *view);
time_t ncm_playlist_last_modified(NcmPlaylist *playlist);
int32 ncm_playlist_from_mpd_playlist(NcmPlaylist *dest,
                                     struct mpd_playlist *source);

struct mpd_entity;

#define ENUM_NAME NcmMpdItemKind
#define ENUM_PREFIX_ NCM_MPD_ITEM_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(NCM_MPD_ITEM_SONG)                                                      \
    XX(NCM_MPD_ITEM_DIRECTORY)                                                 \
    XX(NCM_MPD_ITEM_PLAYLIST)
#include "cbase/xenums.c"

union NcmMpdItemValue {
    NcmSong song;
    NcmDirectory directory;
    NcmPlaylist playlist;
};

typedef struct NcmMpdItem {
    enum NcmMpdItemKind kind;
    union NcmMpdItemValue value;
} NcmMpdItem;

void ncm_mpd_item_init(NcmMpdItem *item);
void ncm_mpd_item_destroy(NcmMpdItem *item);
void ncm_mpd_item_move(NcmMpdItem *dest, NcmMpdItem *source);
int32 ncm_mpd_item_copy(NcmMpdItem *dest, NcmMpdItem *source);
int32 ncm_mpd_item_set_song(NcmMpdItem *item, NcmSong *source);
int32 ncm_mpd_item_set_directory(NcmMpdItem *item, NcmDirectory *source);
int32 ncm_mpd_item_from_entity_copy(NcmMpdItem *item,
                                   struct mpd_entity *entity);
enum NcmMpdItemKind ncm_mpd_item_kind(NcmMpdItem *item);
NcmSong *ncm_mpd_item_song(NcmMpdItem *item);
NcmDirectory *ncm_mpd_item_directory(NcmMpdItem *item);
NcmPlaylist *ncm_mpd_item_playlist(NcmMpdItem *item);

typedef void NcmArrayItemInitCallback(void *item);
typedef void NcmArrayItemDestroyCallback(void *item);
typedef int32 NcmArrayItemCopyCallback(void *dest, void *source);
typedef void NcmArrayItemMoveCallback(void *dest, void *source);

typedef struct NcmArrayItemCallbacks {
    NcmArrayItemInitCallback *init;
    NcmArrayItemDestroyCallback *destroy;
    NcmArrayItemCopyCallback *copy;
    NcmArrayItemMoveCallback *move;
} NcmArrayItemCallbacks;

#define NCM_ARRAY_DECLARE_TYPE(ARRAY_TYPE, ITEM_TYPE)                          \
    typedef struct ARRAY_TYPE {                                                \
        ITEM_TYPE *items;                                                      \
        int32 len;                                                             \
        int32 cap;                                                             \
    } ARRAY_TYPE;

#define NCM_ARRAY_DECLARE_CLEAR(PREFIX, ARRAY_TYPE)                            \
    void PREFIX##_clear(ARRAY_TYPE *array);

#define NCM_ARRAY_DECLARE_DESTROY(PREFIX, ARRAY_TYPE)                          \
    void PREFIX##_destroy(ARRAY_TYPE *array);

#define NCM_ARRAY_DECLARE_COPY(PREFIX, ARRAY_TYPE)                             \
    int32 PREFIX##_copy(ARRAY_TYPE *dest, ARRAY_TYPE *source);

#define NCM_ARRAY_DECLARE_MOVE(PREFIX, ARRAY_TYPE)                             \
    void PREFIX##_move(ARRAY_TYPE *dest, ARRAY_TYPE *source);

#define NCM_ARRAY_DECLARE_SWAP(PREFIX, ARRAY_TYPE)                             \
    void PREFIX##_swap(ARRAY_TYPE *left, ARRAY_TYPE *right);

#define NCM_ARRAY_DECLARE_RESERVE(PREFIX, ARRAY_TYPE)                          \
    int32 PREFIX##_reserve(ARRAY_TYPE *array, int32 extra);

#define NCM_ARRAY_DECLARE_APPEND(PREFIX, ARRAY_TYPE, ITEM_TYPE)                \
    ITEM_TYPE *PREFIX##_append(ARRAY_TYPE *array);

#define NCM_ARRAY_DECLARE_APPEND_COPY(PREFIX, ARRAY_TYPE, ITEM_TYPE)           \
    int32 PREFIX##_append_copy(ARRAY_TYPE *array, ITEM_TYPE *item);

#define NCM_ARRAY_DECLARE_APPEND_MOVE(PREFIX, ARRAY_TYPE, ITEM_TYPE)           \
    void PREFIX##_append_move(ARRAY_TYPE *array, ITEM_TYPE *item);

#define NCM_ARRAY_DECLARE_REMOVE_ORDERED(PREFIX, ARRAY_TYPE)                   \
    void PREFIX##_remove_ordered(ARRAY_TYPE *array, int32 idx);

#define NCM_ARRAY_DEFINE_CLEAR(PREFIX, ARRAY_TYPE, CALLBACKS)                  \
    void                                                                       \
    PREFIX##_clear(ARRAY_TYPE *array) {                                        \
        NcmArrayItemCallbacks *callbacks;                                      \
                                                                               \
        if (array == NULL) {                                                   \
            return;                                                            \
        }                                                                      \
        callbacks = CALLBACKS;                                                 \
        if (callbacks && callbacks->destroy) {                                 \
            for (int32 i = 0; i < array->len; i += 1) {                        \
                callbacks->destroy(&array->items[i]);                          \
            }                                                                  \
        }                                                                      \
        array->len = 0;                                                        \
        return;                                                                \
    }

#define NCM_ARRAY_DEFINE_DESTROY(PREFIX, ARRAY_TYPE)                           \
    void                                                                       \
    PREFIX##_destroy(ARRAY_TYPE *array) {                                      \
        if (array == NULL) {                                                   \
            return;                                                            \
        }                                                                      \
        PREFIX##_clear(array);                                                 \
        if (array->items) {                                                    \
            free2(array->items,                                                \
                  array->cap*SIZEOF(*array->items));                           \
        }                                                                      \
        *array = (ARRAY_TYPE){0};                                              \
        return;                                                                \
    }

#define NCM_ARRAY_DEFINE_COPY(PREFIX, ARRAY_TYPE)                              \
    int32                                                                      \
    PREFIX##_copy(ARRAY_TYPE *dest, ARRAY_TYPE *source) {                      \
        ARRAY_TYPE replacement;                                                \
        int32 err;                                                             \
                                                                               \
        if (dest == NULL) {                                                    \
            return -EINVAL;                                                    \
        }                                                                      \
        if (dest == source) {                                                  \
            return dest->len;                                                  \
        }                                                                      \
                                                                               \
        replacement = (ARRAY_TYPE){0};                                         \
        if (source) {                                                          \
            if ((err = PREFIX##_reserve(                                       \
                     &replacement, source->len)) < 0) {                        \
                PREFIX##_destroy(&replacement);                                \
                return err;                                                    \
            }                                                                  \
            for (int32 i = 0; i < source->len; i += 1) {                       \
                if ((err = PREFIX##_append_copy(                               \
                         &replacement, &source->items[i])) < 0) {              \
                    PREFIX##_destroy(&replacement);                            \
                    return err;                                                \
                }                                                              \
            }                                                                  \
        }                                                                      \
                                                                               \
        PREFIX##_destroy(dest);                                                \
        *dest = replacement;                                                   \
        return dest->len;                                                      \
    }

#define NCM_ARRAY_DEFINE_MOVE(PREFIX, ARRAY_TYPE)                              \
    void                                                                       \
    PREFIX##_move(ARRAY_TYPE *dest, ARRAY_TYPE *source) {                      \
        if (dest == NULL) {                                                    \
            return;                                                            \
        }                                                                      \
        if (dest == source) {                                                  \
            return;                                                            \
        }                                                                      \
                                                                               \
        PREFIX##_destroy(dest);                                                \
        if (source == NULL) {                                                  \
            *dest = (ARRAY_TYPE){0};                                           \
            return;                                                            \
        }                                                                      \
        *dest = *source;                                                       \
        *source = (ARRAY_TYPE){0};                                             \
        return;                                                                \
    }

#define NCM_ARRAY_DEFINE_SWAP(PREFIX, ARRAY_TYPE)                              \
    void                                                                       \
    PREFIX##_swap(ARRAY_TYPE *left, ARRAY_TYPE *right) {                       \
        ARRAY_TYPE temp;                                                       \
                                                                               \
        if (left == NULL) {                                                    \
            return;                                                            \
        }                                                                      \
        if (right == NULL) {                                                   \
            return;                                                            \
        }                                                                      \
        temp = *left;                                                          \
        *left = *right;                                                        \
        *right = temp;                                                         \
        return;                                                                \
    }

#define NCM_ARRAY_DEFINE_RESERVE(PREFIX, ARRAY_TYPE)                           \
    int32                                                                      \
    PREFIX##_reserve(ARRAY_TYPE *array, int32 extra) {                         \
        int64 needed;                                                          \
        int32 old_cap;                                                         \
        int32 new_cap;                                                         \
                                                                               \
        if (array == NULL) {                                                   \
            return -EINVAL;                                                    \
        }                                                                      \
        if (extra < 0) {                                                       \
            return -EINVAL;                                                    \
        }                                                                      \
        if (extra == 0) {                                                      \
            return array->cap;                                                 \
        }                                                                      \
                                                                               \
        needed = (int64)array->len + extra;                                    \
        if (needed <= array->cap) {                                            \
            return array->cap;                                                 \
        }                                                                      \
        if (needed >= MAXOF(array->cap)) {                                     \
            error("Array only supports fewer than 2GB items.\n");              \
            fatal(EXIT_FAILURE);                                               \
        }                                                                      \
                                                                               \
        old_cap = array->cap;                                                  \
        new_cap = array->cap;                                                  \
        if (new_cap <= 0) {                                                    \
            new_cap = 8;                                                       \
        }                                                                      \
        if (needed >= (MAXOF(new_cap)/2)) {                                    \
            new_cap = (int32)needed;                                           \
        } else {                                                               \
            while (new_cap < needed) {                                         \
                new_cap *= 2;                                                  \
            }                                                                  \
        }                                                                      \
                                                                               \
        array->items = realloc2(                                               \
            array->items, old_cap, new_cap, SIZEOF(*array->items));            \
        array->cap = new_cap;                                                  \
        return array->cap;                                                     \
    }

#define NCM_ARRAY_DEFINE_APPEND(PREFIX, ARRAY_TYPE, ITEM_TYPE,                 \
                                CALLBACKS)                                     \
    ITEM_TYPE *                                                                \
    PREFIX##_append(ARRAY_TYPE *array) {                                       \
        ITEM_TYPE *item;                                                       \
        NcmArrayItemCallbacks *callbacks;                                      \
                                                                               \
        if (PREFIX##_reserve(array, 1) < 0) {                                  \
            return NULL;                                                       \
        }                                                                      \
        callbacks = CALLBACKS;                                                 \
        item = &array->items[array->len];                                      \
        array->len += 1;                                                       \
        if (callbacks && callbacks->init) {                                    \
            callbacks->init(item);                                             \
        } else {                                                               \
            char *bytes = (char *)item;                                        \
            for (int32 i = 0; i < (int32)SIZEOF(*item); i += 1) {              \
                bytes[i] = 0;                                                  \
            }                                                                  \
        }                                                                      \
        return item;                                                           \
    }

#define NCM_ARRAY_DEFINE_APPEND_COPY(PREFIX, ARRAY_TYPE, ITEM_TYPE,            \
                                     CALLBACKS)                                \
    int32                                                                      \
    PREFIX##_append_copy(ARRAY_TYPE *array, ITEM_TYPE *item) {                 \
        ITEM_TYPE *dest;                                                       \
        NcmArrayItemCallbacks *callbacks;                                      \
        int32 err;                                                             \
        int32 index;                                                           \
                                                                               \
        if ((array == NULL) || (item == NULL)) {                               \
            return -EINVAL;                                                    \
        }                                                                      \
        if ((err = PREFIX##_reserve(array, 1)) < 0) {                          \
            return err;                                                        \
        }                                                                      \
        index = array->len;                                                    \
        dest = &array->items[index];                                           \
        array->len += 1;                                                       \
        callbacks = CALLBACKS;                                                 \
        if (callbacks && callbacks->init) {                                    \
            callbacks->init(dest);                                             \
        } else {                                                               \
            *dest = (ITEM_TYPE){0};                                            \
        }                                                                      \
        if (callbacks && callbacks->copy) {                                    \
            if ((err = callbacks->copy(dest, item)) < 0) {                     \
                array->len -= 1;                                               \
                if (callbacks->destroy) {                                      \
                    callbacks->destroy(dest);                                  \
                }                                                              \
                return err;                                                    \
            }                                                                  \
        } else {                                                               \
            *dest = *item;                                                     \
        }                                                                      \
        return index;                                                          \
    }

#define NCM_ARRAY_DEFINE_APPEND_MOVE(PREFIX, ARRAY_TYPE, ITEM_TYPE,            \
                                     CALLBACKS)                                \
    void                                                                       \
    PREFIX##_append_move(ARRAY_TYPE *array, ITEM_TYPE *item) {                 \
        ITEM_TYPE *dest;                                                       \
        NcmArrayItemCallbacks *callbacks;                                      \
                                                                               \
        if (item == NULL) {                                                    \
            return;                                                            \
        }                                                                      \
        dest = PREFIX##_append(array);                                         \
        if (dest == NULL) {                                                    \
            return;                                                            \
        }                                                                      \
        callbacks = CALLBACKS;                                                 \
        if (callbacks && callbacks->move) {                                    \
            callbacks->move(dest, item);                                       \
        } else {                                                               \
            *dest = *item;                                                     \
        }                                                                      \
        return;                                                                \
    }

#define NCM_ARRAY_DEFINE_REMOVE_ORDERED(PREFIX, ARRAY_TYPE, CALLBACKS)         \
    void                                                                       \
    PREFIX##_remove_ordered(ARRAY_TYPE *array, int32 idx) {                    \
        NcmArrayItemCallbacks *callbacks;                                      \
                                                                               \
        if (array == NULL) {                                                   \
            return;                                                            \
        }                                                                      \
        if ((idx < 0) || (idx >= array->len)) {                                \
            return;                                                            \
        }                                                                      \
                                                                               \
        callbacks = CALLBACKS;                                                 \
        if (callbacks && callbacks->destroy) {                                 \
            callbacks->destroy(&array->items[idx]);                            \
        }                                                                      \
        if (idx + 1 < array->len) {                                            \
            memmove64(                                                         \
                &array->items[idx],                                            \
                &array->items[idx + 1],                                        \
                (array->len - idx - 1)*SIZEOF(*array->items));                 \
        }                                                                      \
        array->len -= 1;                                                       \
        return;                                                                \
    }

#define NCM_ARRAY_DECLARE(PREFIX, ARRAY_TYPE, ITEM_TYPE)                       \
    NCM_ARRAY_DECLARE_TYPE(ARRAY_TYPE, ITEM_TYPE)                              \
    NCM_ARRAY_DECLARE_CLEAR(PREFIX, ARRAY_TYPE)                                \
    NCM_ARRAY_DECLARE_DESTROY(PREFIX, ARRAY_TYPE)                              \
    NCM_ARRAY_DECLARE_COPY(PREFIX, ARRAY_TYPE)                                 \
    NCM_ARRAY_DECLARE_MOVE(PREFIX, ARRAY_TYPE)                                 \
    NCM_ARRAY_DECLARE_SWAP(PREFIX, ARRAY_TYPE)                                 \
    NCM_ARRAY_DECLARE_RESERVE(PREFIX, ARRAY_TYPE)                              \
    NCM_ARRAY_DECLARE_APPEND(PREFIX, ARRAY_TYPE, ITEM_TYPE)                    \
    NCM_ARRAY_DECLARE_APPEND_COPY(PREFIX, ARRAY_TYPE, ITEM_TYPE)               \
    NCM_ARRAY_DECLARE_APPEND_MOVE(PREFIX, ARRAY_TYPE, ITEM_TYPE)               \
    NCM_ARRAY_DECLARE_REMOVE_ORDERED(PREFIX, ARRAY_TYPE)

#define NCM_ARRAY_DEFINE(PREFIX, ARRAY_TYPE, ITEM_TYPE, CALLBACKS)             \
    NCM_ARRAY_DEFINE_CLEAR(PREFIX, ARRAY_TYPE, CALLBACKS)                      \
    NCM_ARRAY_DEFINE_DESTROY(PREFIX, ARRAY_TYPE)                               \
    NCM_ARRAY_DEFINE_COPY(PREFIX, ARRAY_TYPE)                                  \
    NCM_ARRAY_DEFINE_MOVE(PREFIX, ARRAY_TYPE)                                  \
    NCM_ARRAY_DEFINE_SWAP(PREFIX, ARRAY_TYPE)                                  \
    NCM_ARRAY_DEFINE_RESERVE(PREFIX, ARRAY_TYPE)                               \
    NCM_ARRAY_DEFINE_APPEND(PREFIX, ARRAY_TYPE, ITEM_TYPE, CALLBACKS)          \
    NCM_ARRAY_DEFINE_APPEND_COPY(PREFIX, ARRAY_TYPE, ITEM_TYPE,                \
                                 CALLBACKS)                                    \
    NCM_ARRAY_DEFINE_APPEND_MOVE(PREFIX, ARRAY_TYPE, ITEM_TYPE,                \
                                 CALLBACKS)                                    \
    NCM_ARRAY_DEFINE_REMOVE_ORDERED(PREFIX, ARRAY_TYPE, CALLBACKS)

typedef struct NcmSampleBuffer {
    int16 *data;
    int32 len;
    int32 cap;
} NcmSampleBuffer;

void ncm_sample_buffer_destroy(NcmSampleBuffer *buffer);
int32 ncm_sample_buffer_put(NcmSampleBuffer *buffer,
                            int16 *samples, int32 samples_len);
int32 ncm_sample_buffer_get_clamped(NcmSampleBuffer *buffer,
                                    int32 samples_len,
                                    int16 *dest, int32 dest_len);
void ncm_sample_buffer_resize(NcmSampleBuffer *buffer, int32 cap);
void ncm_sample_buffer_clear(NcmSampleBuffer *buffer);
int32 ncm_sample_buffer_capacity(NcmSampleBuffer *buffer);

NCM_ARRAY_DECLARE_TYPE(NcmStringViewArray, NcmStringView)
NCM_ARRAY_DECLARE_CLEAR(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DECLARE_APPEND(ncm_string_view_array,
                         NcmStringViewArray,
                         NcmStringView)

NCM_ARRAY_DECLARE_TYPE(NcmSongArray, NcmSong)
NCM_ARRAY_DECLARE_CLEAR(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_COPY(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_MOVE(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_APPEND(ncm_song_array, NcmSongArray, NcmSong)
NCM_ARRAY_DECLARE_APPEND_COPY(ncm_song_array, NcmSongArray, NcmSong)
NCM_ARRAY_DECLARE_APPEND_MOVE(ncm_song_array, NcmSongArray, NcmSong)

NCM_ARRAY_DECLARE_TYPE(NcmDirectoryArray, NcmDirectory)
NCM_ARRAY_DECLARE_CLEAR(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DECLARE_COPY(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DECLARE_MOVE(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DECLARE_APPEND(ncm_directory_array,
                         NcmDirectoryArray,
                         NcmDirectory)
NCM_ARRAY_DECLARE_APPEND_COPY(ncm_directory_array,
                              NcmDirectoryArray,
                              NcmDirectory)

NCM_ARRAY_DECLARE_TYPE(NcmPlaylistArray, NcmPlaylist)
NCM_ARRAY_DECLARE_CLEAR(ncm_playlist_array, NcmPlaylistArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_playlist_array, NcmPlaylistArray)
NCM_ARRAY_DECLARE_MOVE(ncm_playlist_array, NcmPlaylistArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_playlist_array, NcmPlaylistArray)
NCM_ARRAY_DECLARE_APPEND(ncm_playlist_array,
                         NcmPlaylistArray,
                         NcmPlaylist)
NCM_ARRAY_DECLARE_APPEND_COPY(ncm_playlist_array,
                              NcmPlaylistArray,
                              NcmPlaylist)

NCM_ARRAY_DECLARE_TYPE(NcmMpdItemArray, NcmMpdItem)
NCM_ARRAY_DECLARE_CLEAR(ncm_mpd_item_array, NcmMpdItemArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_mpd_item_array, NcmMpdItemArray)
NCM_ARRAY_DECLARE_MOVE(ncm_mpd_item_array, NcmMpdItemArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_mpd_item_array, NcmMpdItemArray)
NCM_ARRAY_DECLARE_APPEND(ncm_mpd_item_array,
                         NcmMpdItemArray,
                         NcmMpdItem)
NCM_ARRAY_DECLARE_APPEND_COPY(ncm_mpd_item_array,
                              NcmMpdItemArray,
                              NcmMpdItem)

#include <regex.h>

#define NCM_REGEX_EXTENDED 0x01u
#define NCM_REGEX_ICASE    0x02u
#define NCM_REGEX_LITERAL  0x04u
#define NCM_REGEX_NOSUB    0x08u

#define NCM_REGEX_BASIC_CASE_INSENSITIVE NCM_REGEX_ICASE
#define NCM_REGEX_EXTENDED_CASE_INSENSITIVE                                    \
    (NCM_REGEX_EXTENDED | NCM_REGEX_ICASE)
#define NCM_REGEX_LITERAL_CASE_INSENSITIVE                                     \
    (NCM_REGEX_LITERAL | NCM_REGEX_EXTENDED | NCM_REGEX_ICASE)

typedef bool NcmRegexMatchCallback(int32 start, int32 len, void *user);

typedef struct NcmRegex {
    regex_t regex;
    bool compiled;
    uint32 flags;
} NcmRegex;

void ncm_regex_destroy(NcmRegex *regex);
int32 ncm_regex_compile(NcmRegex *regex, char *pattern, int32 pattern_len,
                        uint32 flags, NcmError *ncm_error);
bool ncm_regex_matches(NcmRegex *regex, char *string, int32 string_len);
int32 ncm_regex_for_each_match(NcmRegex *regex,
                               char *string, int32 string_len,
                               NcmRegexMatchCallback *callback, void *user);

#include <mpd/client.h>

typedef struct NcmMpdConnection {
    struct mpd_connection *mpd;
    NcmError ncm_error;
    enum mpd_error error_code;
    enum mpd_server_error server_error_code;
    bool error_clearable;
} NcmMpdConnection;

typedef struct NcmMpdStats {
    int32 artists;
    int32 albums;
    int32 songs;
    int32 play_time;
    int32 uptime;
    int32 db_update_time;
    int32 db_play_time;
} NcmMpdStats;

typedef struct NcmMpdSongList {
    NcmSong *items;
    int32 count;
    int32 capacity;
} NcmMpdSongList;

typedef struct NcmMpdItemList {
    NcmMpdItem *items;
    int32 count;
    int32 capacity;
} NcmMpdItemList;

typedef struct NcmStringViewList {
    NcmStringView *items;
    int32 count;
    int32 capacity;
} NcmStringViewList;

typedef struct NcmMpdOutput {
    int32 id;
    char *name;
    int32 name_len;
    bool enabled;
} NcmMpdOutput;

typedef struct NcmMpdOutputList {
    NcmMpdOutput *items;
    int32 count;
    int32 capacity;
} NcmMpdOutputList;

typedef struct NcmMpdPlaylistList {
    NcmPlaylist *items;
    int32 count;
    int32 capacity;
} NcmMpdPlaylistList;

#define ENUM_NAME NcmMpdReplayGainMode
#define ENUM_PREFIX_ NCM_MPD_REPLAY_GAIN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(NCM_MPD_REPLAY_GAIN_OFF, off)                                           \
    XX(NCM_MPD_REPLAY_GAIN_TRACK, track)                                       \
    XX(NCM_MPD_REPLAY_GAIN_ALBUM, album)
#include "cbase/xenums.c"

typedef struct NcmMpdStatus {
    int32 volume;
    bool repeat;
    bool random;
    bool single;
    bool consume;
    int32 queue_length;
    int32 queue_version;
    enum mpd_state state;
    int32 crossfade;
    int32 song_pos;
    int32 song_id;
    int32 next_song_pos;
    int32 next_song_id;
    int32 elapsed_time;
    int64 elapsed_time_ms;
    int32 total_time;
    int32 kbit_rate;
    int32 update_id;
    char error[256];
} NcmMpdStatus;

void ncm_mpd_connection_destroy(NcmMpdConnection *connection);
int32 ncm_mpd_connection_connect(NcmMpdConnection *connection,
                                char *host,
                                uint16 port,
                                int32 timeout_ms);
void ncm_mpd_connection_disconnect(NcmMpdConnection *connection);
bool ncm_mpd_connection_is_connected(NcmMpdConnection *connection);
int32 ncm_mpd_connection_fd(NcmMpdConnection *connection);
int32 ncm_mpd_connection_set_timeout(NcmMpdConnection *connection,
                                    int32 timeout_ms);
int32 ncm_mpd_connection_noidle(NcmMpdConnection *connection);
int32 ncm_mpd_connection_send_idle(NcmMpdConnection *connection,
                                  enum mpd_idle events);
int32 ncm_mpd_connection_recv_idle(NcmMpdConnection *connection,
                                  bool disable_timeout,
                                  enum mpd_idle *out_events);
int32 ncm_mpd_connection_check_error(NcmMpdConnection *connection);
char *ncm_mpd_connection_error(NcmMpdConnection *connection);
void ncm_mpd_connection_clear_error(NcmMpdConnection *connection);
enum mpd_error ncm_mpd_connection_error_code(
    NcmMpdConnection *connection);
enum mpd_server_error ncm_mpd_connection_server_error_code(
    NcmMpdConnection *connection);
bool ncm_mpd_connection_error_is_clearable(NcmMpdConnection *connection);
int32 ncm_mpd_connection_get_stats(NcmMpdConnection *connection,
                                  NcmMpdStats *stats);
int32 ncm_mpd_connection_get_status(NcmMpdConnection *connection,
                                   NcmMpdStatus *status);

int32 ncm_mpd_connection_version(NcmMpdConnection *connection);
int32 ncm_mpd_connection_send_password(NcmMpdConnection *connection,
                                      char *password);
int32 ncm_mpd_connection_start_command_list(NcmMpdConnection *connection);
int32 ncm_mpd_connection_commit_command_list(NcmMpdConnection *connection);
int32 ncm_mpd_connection_get_supported_extensions(
    NcmMpdConnection *connection,
    NcmStringViewList *strings);
int32 ncm_mpd_connection_get_replay_gain_mode(
    NcmMpdConnection *connection,
    enum NcmMpdReplayGainMode *mode);
int32 ncm_mpd_connection_set_replay_gain_mode(
    NcmMpdConnection *connection,
    enum NcmMpdReplayGainMode mode);
int32 ncm_mpd_connection_get_playlists(NcmMpdConnection *connection,
                                      NcmMpdPlaylistList *playlists);
int32 ncm_mpd_connection_list_all_song_uris(NcmMpdConnection *connection,
                                           char *path,
                                           NcmStringViewList *strings);
int32 ncm_mpd_connection_get_url_handlers(NcmMpdConnection *connection,
                                         NcmStringViewList *strings);
int32 ncm_mpd_connection_get_tag_types(NcmMpdConnection *connection,
                                      NcmStringViewList *strings);

void ncm_mpd_song_list_destroy(NcmMpdSongList *list);
void ncm_mpd_song_list_clear(NcmMpdSongList *list);
int32 ncm_mpd_song_list_count(NcmMpdSongList *list);
NcmSong *ncm_mpd_song_list_at(NcmMpdSongList *list, int32 idx);
int32 ncm_mpd_song_list_append_copy(
    NcmMpdSongList *list, NcmSong *song);
int32 ncm_mpd_song_list_to_song_array(
    NcmMpdSongList *list, NcmSongArray *songs);

void ncm_mpd_item_list_destroy(NcmMpdItemList *list);
void ncm_mpd_item_list_clear(NcmMpdItemList *list);
int32 ncm_mpd_item_list_to_item_array(
    NcmMpdItemList *list, NcmMpdItemArray *items);
int32 ncm_mpd_item_list_to_directory_array(
    NcmMpdItemList *list, NcmDirectoryArray *directories);

void ncm_mpd_string_list_destroy(NcmStringViewList *list);
void ncm_mpd_string_list_clear(NcmStringViewList *list);
int32 ncm_mpd_string_list_count(NcmStringViewList *list);
NcmStringView *ncm_mpd_string_list_at(NcmStringViewList *list,
                                     int32 idx);

void ncm_mpd_output_list_destroy(NcmMpdOutputList *list);
void ncm_mpd_output_list_clear(NcmMpdOutputList *list);

void ncm_mpd_playlist_list_destroy(NcmMpdPlaylistList *list);
void ncm_mpd_playlist_list_clear(NcmMpdPlaylistList *list);

int32 ncm_mpd_connection_get_current_song(NcmMpdConnection *connection,
                                         NcmSong *song);
int32 ncm_mpd_connection_get_queue(NcmMpdConnection *connection,
                                  NcmMpdSongList *songs);
int32 ncm_mpd_connection_get_queue_changes(NcmMpdConnection *connection,
                                          int32 version,
                                          NcmMpdSongList *songs);
int32 ncm_mpd_connection_get_playlist_content(NcmMpdConnection *connection,
                                             char *path,
                                             NcmMpdSongList *songs);
int32 ncm_mpd_connection_get_playlist_content_no_info(
    NcmMpdConnection *connection,
    char *path,
    NcmMpdSongList *songs);

int32 ncm_mpd_connection_get_directory(NcmMpdConnection *connection,
                                      char *path,
                                      NcmMpdItemList *items);
int32 ncm_mpd_connection_get_directory_songs(NcmMpdConnection *connection,
                                            char *path,
                                            NcmMpdSongList *songs);
int32 ncm_mpd_connection_list_all_songs(NcmMpdConnection *connection,
                                       char *path,
                                       NcmMpdSongList *songs);
int32 ncm_mpd_connection_start_search_songs(NcmMpdConnection *connection,
                                           bool exact_match);
int32 ncm_mpd_connection_add_search_tag(NcmMpdConnection *connection,
                                       enum mpd_tag_type tag,
                                       char *value);
int32 ncm_mpd_connection_add_search_any(NcmMpdConnection *connection,
                                       char *value);
int32 ncm_mpd_connection_add_search_uri(NcmMpdConnection *connection,
                                       char *value);
int32 ncm_mpd_connection_commit_search_songs(NcmMpdConnection *connection,
                                            NcmMpdSongList *songs);
int32 ncm_mpd_connection_list_tag_values(NcmMpdConnection *connection,
                                        enum mpd_tag_type tag,
                                        NcmStringViewList *strings);

int32 ncm_mpd_connection_update_database(NcmMpdConnection *connection,
                                        char *path,
                                        int32 *id);
int32 ncm_mpd_connection_get_outputs(NcmMpdConnection *connection,
                                    NcmMpdOutputList *outputs);
int32 ncm_mpd_connection_enable_output(NcmMpdConnection *connection,
                                      int32 id);
int32 ncm_mpd_connection_disable_output(NcmMpdConnection *connection,
                                       int32 id);

int32 ncm_mpd_connection_play(NcmMpdConnection *connection);
int32 ncm_mpd_connection_play_pos(NcmMpdConnection *connection, int32 pos);
int32 ncm_mpd_connection_play_id(NcmMpdConnection *connection, int32 id);
int32 ncm_mpd_connection_toggle_pause(NcmMpdConnection *connection);
int32 ncm_mpd_connection_stop(NcmMpdConnection *connection);
int32 ncm_mpd_connection_next(NcmMpdConnection *connection);
int32 ncm_mpd_connection_previous(NcmMpdConnection *connection);
int32 ncm_mpd_connection_seek_pos(NcmMpdConnection *connection,
                                 int32 pos,
                                 int32 seconds);
int32 ncm_mpd_connection_set_repeat(NcmMpdConnection *connection, bool mode);
int32 ncm_mpd_connection_set_random(NcmMpdConnection *connection, bool mode);
int32 ncm_mpd_connection_set_single(NcmMpdConnection *connection, bool mode);
int32 ncm_mpd_connection_set_consume(NcmMpdConnection *connection, bool mode);
int32 ncm_mpd_connection_set_crossfade(NcmMpdConnection *connection,
                                      int32 seconds);
int32 ncm_mpd_connection_set_volume(NcmMpdConnection *connection, int32 vol);
int32 ncm_mpd_connection_change_volume(NcmMpdConnection *connection,
                                      int32 change);

int32 ncm_mpd_connection_move(NcmMpdConnection *connection,
                             int32 from,
                             int32 to,
                             bool command_list_active);
int32 ncm_mpd_connection_swap(NcmMpdConnection *connection,
                             int32 from,
                             int32 to,
                             bool command_list_active);
int32 ncm_mpd_connection_shuffle(NcmMpdConnection *connection);
int32 ncm_mpd_connection_shuffle_range(NcmMpdConnection *connection,
                                      int32 start,
                                      int32 end);
int32 ncm_mpd_connection_clear_queue(NcmMpdConnection *connection);
int32 ncm_mpd_connection_set_priority_id(NcmMpdConnection *connection,
                                        int32 id,
                                        int32 prio,
                                        bool command_list_active);
int32 ncm_mpd_connection_add_song(NcmMpdConnection *connection,
                                 char *path,
                                 int32 pos,
                                 bool command_list_active,
                                 int32 *id);
int32 ncm_mpd_connection_add(NcmMpdConnection *connection,
                            char *path,
                            bool command_list_active,
                            bool *added);
int32 ncm_mpd_connection_delete(NcmMpdConnection *connection,
                               int32 pos,
                               bool command_list_active);
int32 ncm_mpd_connection_clear_playlist(NcmMpdConnection *connection,
                                       char *playlist);
int32 ncm_mpd_connection_add_to_playlist(NcmMpdConnection *connection,
                                        char *playlist,
                                        char *path,
                                        bool command_list_active);
int32 ncm_mpd_connection_playlist_move(NcmMpdConnection *connection,
                                      char *playlist,
                                      int32 from,
                                      int32 to,
                                      bool command_list_active);
int32 ncm_mpd_connection_playlist_delete(NcmMpdConnection *connection,
                                        char *playlist,
                                        int32 pos,
                                        bool command_list_active);
int32 ncm_mpd_connection_rename_playlist(NcmMpdConnection *connection,
                                        char *from,
                                        char *to);
int32 ncm_mpd_connection_delete_playlist(NcmMpdConnection *connection,
                                        char *playlist);
int32 ncm_mpd_connection_load_playlist(NcmMpdConnection *connection,
                                      char *playlist,
                                      bool *loaded);
int32 ncm_mpd_connection_save_playlist(NcmMpdConnection *connection,
                                      char *playlist);

typedef void NcmMpdNoidleCallback(int32 flags, void *user);

typedef struct NcmMpdClient {
    NcmMpdConnection connection;
    StrBuilder host;
    StrBuilder password;
    uint16 port;
    int32 timeout_ms;
    bool command_list_active;
    bool idle;
    int32 fd;
    NcmMpdNoidleCallback *noidle_callback;
    void *noidle_user;
} NcmMpdClient;

void ncm_mpd_client_init(NcmMpdClient *client);
void ncm_mpd_client_destroy(NcmMpdClient *client);
char *ncm_mpd_client_hostname(NcmMpdClient *client);
bool ncm_mpd_client_is_connected(NcmMpdClient *client);
int32 ncm_mpd_client_version(NcmMpdClient *client);
int32 ncm_mpd_client_fd(NcmMpdClient *client);
void ncm_mpd_client_set_noidle_callback(NcmMpdClient *client,
                                        NcmMpdNoidleCallback *callback,
                                        void *user);
int32 ncm_mpd_client_set_hostname(NcmMpdClient *client, char *host,
                                 int32 host_len, NcmError *ncm_error);
void ncm_mpd_client_set_port(NcmMpdClient *client, uint16 port);
int32 ncm_mpd_client_set_password(NcmMpdClient *client, char *password,
                                 int32 password_len, NcmError *ncm_error);
int32 ncm_mpd_client_set_timeout_ms(NcmMpdClient *client,
                                   int32 timeout_ms,
                                   NcmError *ncm_error);
int32 ncm_mpd_client_connect(NcmMpdClient *client, NcmError *ncm_error);
void ncm_mpd_client_disconnect(NcmMpdClient *client);
int32 ncm_mpd_client_send_password(NcmMpdClient *client, NcmError *ncm_error);
int32 ncm_mpd_client_idle(NcmMpdClient *client, NcmError *ncm_error);
int32 ncm_mpd_client_noidle(NcmMpdClient *client, int32 *flags,
                           NcmError *ncm_error);

enum mpd_error ncm_mpd_client_error_code(NcmMpdClient *client);
enum mpd_server_error ncm_mpd_client_server_error_code(
    NcmMpdClient *client);
bool ncm_mpd_client_error_is_clearable(NcmMpdClient *client);
char *ncm_mpd_client_error_message(NcmMpdClient *client);

int32 ncm_mpd_client_get_stats(NcmMpdClient *client, NcmMpdStats *stats,
                              NcmError *ncm_error);
int32 ncm_mpd_client_get_status(NcmMpdClient *client, NcmMpdStatus *status,
                               NcmError *ncm_error);
int32 ncm_mpd_client_update_directory(NcmMpdClient *client, char *path,
                                     int32 *id, NcmError *ncm_error);

int32 ncm_mpd_client_play(NcmMpdClient *client, NcmError *ncm_error);
int32 ncm_mpd_client_play_pos(NcmMpdClient *client, int32 pos,
                             NcmError *ncm_error);
int32 ncm_mpd_client_play_id(NcmMpdClient *client, int32 id,
                            NcmError *ncm_error);
int32 ncm_mpd_client_toggle_pause(NcmMpdClient *client, NcmError *ncm_error);
int32 ncm_mpd_client_stop(NcmMpdClient *client, NcmError *ncm_error);
int32 ncm_mpd_client_next(NcmMpdClient *client, NcmError *ncm_error);
int32 ncm_mpd_client_previous(NcmMpdClient *client, NcmError *ncm_error);
int32 ncm_mpd_client_move(NcmMpdClient *client, int32 from, int32 to,
                         NcmError *ncm_error);
int32 ncm_mpd_client_swap(NcmMpdClient *client, int32 from, int32 to,
                         NcmError *ncm_error);
int32 ncm_mpd_client_seek_pos(NcmMpdClient *client, int32 pos,
                             int32 seconds, NcmError *ncm_error);
int32 ncm_mpd_client_shuffle(NcmMpdClient *client, NcmError *ncm_error);
int32 ncm_mpd_client_shuffle_range(NcmMpdClient *client, int32 start,
                                  int32 end, NcmError *ncm_error);
int32 ncm_mpd_client_clear_queue(NcmMpdClient *client, NcmError *ncm_error);

int32 ncm_mpd_client_get_queue(NcmMpdClient *client,
                              NcmMpdSongList *songs,
                              NcmError *ncm_error);
int32 ncm_mpd_client_get_queue_changes(NcmMpdClient *client, int32 version,
                                      NcmMpdSongList *songs,
                                      NcmError *ncm_error);
int32 ncm_mpd_client_get_current_song(NcmMpdClient *client, NcmSong *song,
                                     NcmError *ncm_error);
int32 ncm_mpd_client_get_playlist_content(NcmMpdClient *client,
                                         char *path,
                                         NcmMpdSongList *songs,
                                         NcmError *ncm_error);
int32 ncm_mpd_client_get_playlist_content_no_info(NcmMpdClient *client,
                                                 char *path,
                                                 NcmMpdSongList *songs,
                                                 NcmError *ncm_error);
int32 ncm_mpd_client_get_supported_extensions(NcmMpdClient *client,
                                             NcmStringViewList *strings,
                                             NcmError *ncm_error);

int32 ncm_mpd_client_set_repeat(NcmMpdClient *client, bool mode,
                               NcmError *ncm_error);
int32 ncm_mpd_client_set_random(NcmMpdClient *client, bool mode,
                               NcmError *ncm_error);
int32 ncm_mpd_client_set_single(NcmMpdClient *client, bool mode,
                               NcmError *ncm_error);
int32 ncm_mpd_client_set_consume(NcmMpdClient *client, bool mode,
                                NcmError *ncm_error);
int32 ncm_mpd_client_set_crossfade(NcmMpdClient *client, int32 seconds,
                                  NcmError *ncm_error);
int32 ncm_mpd_client_set_volume(NcmMpdClient *client, int32 volume,
                               NcmError *ncm_error);
int32 ncm_mpd_client_change_volume(NcmMpdClient *client, int32 change,
                                  NcmError *ncm_error);
int32 ncm_mpd_client_get_replay_gain_mode(
    NcmMpdClient *client,
    enum NcmMpdReplayGainMode *mode,
    NcmError *ncm_error);
int32 ncm_mpd_client_set_replay_gain_mode(
    NcmMpdClient *client,
    enum NcmMpdReplayGainMode mode,
    NcmError *ncm_error);

int32 ncm_mpd_client_set_priority_id(NcmMpdClient *client, int32 id,
                                    int32 priority, NcmError *ncm_error);
int32 ncm_mpd_client_set_priority_song(NcmMpdClient *client,
                                      NcmSong *song, int32 priority,
                                      NcmError *ncm_error);
int32 ncm_mpd_client_add_song(NcmMpdClient *client, char *path, int32 pos,
                             int32 *id, NcmError *ncm_error);
int32 ncm_mpd_client_add_song_value(NcmMpdClient *client, NcmSong *song,
                                   int32 pos, int32 *id,
                                   NcmError *ncm_error);
int32 ncm_mpd_client_add_song_list(NcmMpdClient *client,
                                  NcmMpdSongList *songs, int32 pos,
                                  NcmError *ncm_error);
int32 ncm_mpd_client_add(NcmMpdClient *client, char *path, bool *added,
                        NcmError *ncm_error);
int32 ncm_mpd_client_add_random_tag(NcmMpdClient *client,
                                   enum mpd_tag_type tag,
                                   int32 number,
                                   NcmError *ncm_error);
int32 ncm_mpd_client_add_random_songs(NcmMpdClient *client,
                                     int32 number,
                                     char *exclude_pattern,
                                     int32 exclude_pattern_len,
                                     NcmError *ncm_error);
int32 ncm_mpd_client_delete(NcmMpdClient *client, int32 pos,
                           NcmError *ncm_error);
int32 ncm_mpd_client_start_command_list(NcmMpdClient *client,
                                       NcmError *ncm_error);
int32 ncm_mpd_client_commit_command_list(NcmMpdClient *client,
                                        NcmError *ncm_error);

int32 ncm_mpd_client_delete_playlist(NcmMpdClient *client, char *name,
                                    NcmError *ncm_error);
int32 ncm_mpd_client_load_playlist(NcmMpdClient *client, char *name,
                                  bool *loaded, NcmError *ncm_error);
int32 ncm_mpd_client_save_playlist(NcmMpdClient *client, char *name,
                                  NcmError *ncm_error);
int32 ncm_mpd_client_clear_playlist(NcmMpdClient *client, char *name,
                                   NcmError *ncm_error);
int32 ncm_mpd_client_add_to_playlist(NcmMpdClient *client, char *playlist,
                                    char *path, NcmError *ncm_error);
int32 ncm_mpd_client_add_song_to_playlist(NcmMpdClient *client,
                                         char *playlist, NcmSong *song,
                                         NcmError *ncm_error);
int32 ncm_mpd_client_playlist_move(NcmMpdClient *client, char *playlist,
                                  int32 from, int32 to,
                                  NcmError *ncm_error);
int32 ncm_mpd_client_playlist_delete(NcmMpdClient *client, char *playlist,
                                    int32 pos, NcmError *ncm_error);
int32 ncm_mpd_client_rename_playlist(NcmMpdClient *client, char *from,
                                    char *to, NcmError *ncm_error);

int32 ncm_mpd_client_start_search(NcmMpdClient *client, bool exact_match,
                                 NcmError *ncm_error);
int32 ncm_mpd_client_add_search_tag(NcmMpdClient *client,
                                   enum mpd_tag_type tag,
                                   char *value, NcmError *ncm_error);
int32 ncm_mpd_client_add_search_any(NcmMpdClient *client, char *value,
                                   NcmError *ncm_error);
int32 ncm_mpd_client_add_search_uri(NcmMpdClient *client, char *value,
                                   NcmError *ncm_error);
int32 ncm_mpd_client_commit_search_songs(NcmMpdClient *client,
                                        NcmMpdSongList *songs,
                                        NcmError *ncm_error);

int32 ncm_mpd_client_get_playlists(NcmMpdClient *client,
                                  NcmMpdPlaylistList *playlists,
                                  NcmError *ncm_error);
int32 ncm_mpd_client_get_list(NcmMpdClient *client, enum mpd_tag_type tag,
                             NcmStringViewList *strings, NcmError *ncm_error);
int32 ncm_mpd_client_get_directory(NcmMpdClient *client, char *path,
                                  NcmMpdItemList *items, NcmError *ncm_error);
int32 ncm_mpd_client_get_directory_recursive(NcmMpdClient *client,
                                            char *path,
                                            NcmMpdSongList *songs,
                                            NcmError *ncm_error);
int32 ncm_mpd_client_get_songs(NcmMpdClient *client, char *path,
                              NcmMpdSongList *songs, NcmError *ncm_error);
int32 ncm_mpd_client_get_directory_entries(NcmMpdClient *client,
                                          char *path,
                                          NcmMpdItemArray *items,
                                          NcmError *ncm_error);
int32 ncm_mpd_client_get_directory_list(NcmMpdClient *client, char *path,
                                       NcmDirectoryArray *directories,
                                       NcmError *ncm_error);
int32 ncm_mpd_client_get_outputs(NcmMpdClient *client,
                                NcmMpdOutputList *outputs,
                                NcmError *ncm_error);
int32 ncm_mpd_client_enable_output(NcmMpdClient *client, int32 id,
                                  NcmError *ncm_error);
int32 ncm_mpd_client_disable_output(NcmMpdClient *client, int32 id,
                                   NcmError *ncm_error);
int32 ncm_mpd_client_get_url_handlers(NcmMpdClient *client,
                                     NcmStringViewList *strings,
                                     NcmError *ncm_error);
int32 ncm_mpd_client_get_tag_types(NcmMpdClient *client,
                                  NcmStringViewList *strings,
                                  NcmError *ncm_error);

#include "configura.h"

#define ENUM_NAME SearchDirection
#define ENUM_PREFIX_ NCM_SEARCH_DIRECTION_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                      \
    XX(NCM_SEARCH_DIRECTION_BACKWARD, backward)          \
    XX(NCM_SEARCH_DIRECTION_FORWARD, forward)
#include "cbase/xenums.c"

#define ENUM_NAME SpaceAddMode
#define ENUM_PREFIX_ NCM_SPACE_ADD_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                    \
    XX(NCM_SPACE_ADD_MODE_ADD_REMOVE, add_remove)      \
    XX(NCM_SPACE_ADD_MODE_ALWAYS_ADD, always_add)
#include "cbase/xenums.c"

#define ENUM_NAME SortMode
#define ENUM_PREFIX_ NCM_SORT_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                    \
    XX(NCM_SORT_MODE_TYPE, type)                       \
    XX(NCM_SORT_MODE_NAME, name)                       \
    XX(NCM_SORT_MODE_MODIFICATION_TIME, mtime)         \
    XX(NCM_SORT_MODE_CUSTOM_FORMAT, format)            \
    XX(NCM_SORT_MODE_NONE, none)
#include "cbase/xenums.c"

#define ENUM_NAME DisplayMode
#define ENUM_PREFIX_ NCM_DISPLAY_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                        \
    XX(NCM_DISPLAY_MODE_CLASSIC, classic)  \
    XX(NCM_DISPLAY_MODE_COLUMNS, columns)
#include "cbase/xenums.c"

#define ENUM_NAME Design
#define ENUM_PREFIX_ NCM_DESIGN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                        \
    XX(NCM_DESIGN_CLASSIC, classic)        \
    XX(NCM_DESIGN_ALTERNATIVE, alternative)
#include "cbase/xenums.c"

#define ENUM_NAME VisualizerType
#define ENUM_PREFIX_ NCM_VISUALIZER_TYPE_
#define ENUM_BITFLAGS 0
#if defined(HAVE_FFTW3_H)
#define ENUM_FIELDS                                    \
    XX(NCM_VISUALIZER_TYPE_WAVE, wave)                 \
    XX(NCM_VISUALIZER_TYPE_WAVE_FILLED, wave_filled)   \
    XX(NCM_VISUALIZER_TYPE_SPECTRUM, spectrum)         \
    XX(NCM_VISUALIZER_TYPE_ELLIPSE, ellipse)
#else
#define ENUM_FIELDS                                    \
    XX(NCM_VISUALIZER_TYPE_WAVE, wave)                 \
    XX(NCM_VISUALIZER_TYPE_WAVE_FILLED, wave_filled)   \
    XX(NCM_VISUALIZER_TYPE_ELLIPSE, ellipse)
#endif
#include "cbase/xenums.c"

char *ncm_search_direction_str(enum SearchDirection value);
int32 ncm_space_add_mode_parse(char *string, int32 string_len,
                               enum SpaceAddMode *value);
int32 ncm_sort_mode_parse(char *string, int32 string_len,
                          enum SortMode *value);
char *ncm_display_mode_str(enum DisplayMode value);
int32 ncm_display_mode_parse(char *string, int32 string_len,
                             enum DisplayMode *value);
char *ncm_design_str(enum Design value);
int32 ncm_design_parse(char *string, int32 string_len, enum Design *value);
int32 ncm_visualizer_type_parse(char *string, int32 string_len,
                                enum VisualizerType *value);

int32 ncm_compare_locale_strings(char *left, int32 left_len,
                                 char *right, int32 right_len,
                                 bool ignore_the);

int32 ncm_parse_int32(char *source, int32 source_len, int32 *out,
                      NcmError *ncm_error);
int32 ncm_parse_int64(char *source, int32 source_len, int32 *out,
                      NcmError *ncm_error);
int32 ncm_parse_double(char *source, int32 source_len,
                       double *out, NcmError *ncm_error);

int32 ncm_bounds_check_i64(int64 value, int64 lbound, int64 ubound,
                           NcmError *ncm_error);

int32 ncm_bounds_check_f64(double value, double lbound, double ubound,
                           NcmError *ncm_error);
int32 ncm_lower_bound_check_f64(double value, double lbound,
                                NcmError *ncm_error);

#define ENUM_NAME NcmFsEntryType
#define ENUM_PREFIX_ NCM_FS_ENTRY_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                     \
    XX(NCM_FS_ENTRY_FILE)               \
    XX(NCM_FS_ENTRY_DIRECTORY)          \
    XX(NCM_FS_ENTRY_SYMLINK)
#include "cbase/xenums.c"

typedef struct NcmFsStat {
    int32 size;
    int32 mtime;
    enum NcmFsEntryType type;
    bool exists;
} NcmFsStat;

typedef struct NcmFsEntry {
    char *name;
    int32 name_len;
    enum NcmFsEntryType type;
} NcmFsEntry;

typedef struct NcmFsDirectory {
    DIR *dir;
    char *path;
    int32 path_len;
} NcmFsDirectory;

void ncm_fs_entry_init(NcmFsEntry *entry);
void ncm_fs_entry_destroy(NcmFsEntry *entry);
int32 ncm_fs_stat(char *path, int32 path_len, NcmFsStat *stat,
                  NcmError *ncm_error);
bool ncm_fs_path_is_existing(char *path, int32 path_len);
int32 ncm_fs_unlink(char *path, int32 path_len, NcmError *ncm_error);
int32 ncm_fs_rename(char *old_path, int32 old_path_len,
                    char *new_path, int32 new_path_len,
                    NcmError *ncm_error);
int32 ncm_fs_mkdir_all(char *path, int32 path_len, NcmError *ncm_error);
int32 ncm_fs_directory_open(NcmFsDirectory *directory, char *path,
                            int32 path_len, NcmError *ncm_error);
int32 ncm_fs_directory_read(NcmFsDirectory *directory, NcmFsEntry *entry,
                            NcmError *ncm_error);
void ncm_fs_directory_close(NcmFsDirectory *directory);
int32 ncm_fs_join(StrBuilder *buffer, char *left, int32 left_len,
                  char *right, int32 right_len);

StrBuilder ncm_html_unescape_utf8(char *data, int32 data_len);
StrBuilder ncm_html_unescape_entities(char *data, int32 data_len);
StrBuilder ncm_html_strip_tags(char *data, int32 data_len);

typedef int32 NcmJobRunCallback(void *user, NcmError *ncm_error);
typedef void NcmJobCompleteCallback(int32 status, NcmError *ncm_error,
                                    void *user);
typedef void NcmJobDestroyCallback(void *user);

typedef struct NcmJob {
    NcmJobRunCallback *run;
    NcmJobCompleteCallback *complete;
    NcmJobDestroyCallback *destroy;
    void *user;
    NcmError ncm_error;
    int32 status;
} NcmJob;

typedef struct NcmJobQueue {
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    NcmJob *pending;
    NcmJob *completed;

    int32 pending_len;
    int32 pending_cap;
    int32 completed_len;
    int32 completed_cap;

    bool started;
    bool stopping;
} NcmJobQueue;

void ncm_job_queue_init(NcmJobQueue *queue);
int32 ncm_job_queue_start(NcmJobQueue *queue, NcmError *ncm_error);
int32 ncm_job_queue_push(NcmJobQueue *queue, NcmJob job, NcmError *ncm_error);
int32 ncm_job_queue_dispatch_completed(NcmJobQueue *queue);
void ncm_job_queue_destroy(NcmJobQueue *queue);
int32 ncm_job_queue_pending_count(NcmJobQueue *queue);
int32 ncm_job_queue_completed_count(NcmJobQueue *queue);

#define NCM_LRC_NO_BUFFER_POSITION (-1)

typedef struct NcmLrcEntry {
    int32 time_ms;
    int32 text_start;
    int32 text_len;
    int32 buffer_start;
    int32 buffer_end;
    int32 source_order;
    int32 blank_lines_before;
} NcmLrcEntry;

typedef struct NcmLrcDocument {
    StrBuilder text;
    NcmLrcEntry *entries;

    int32 entries_len;
    int32 entries_cap;
    int32 offset_ms;
    bool has_offset;
} NcmLrcDocument;

typedef struct NcmLrcRenderTarget {
    void *user;
    int32 (*position)(void *user);
    void (*append)(void *user, char *data, int32 data_len);
} NcmLrcRenderTarget;

void ncm_lrc_document_clear(NcmLrcDocument *document);
void ncm_lrc_document_destroy(NcmLrcDocument *document);
int32 ncm_lrc_parse(NcmLrcDocument *document,
                    char *data, int32 data_len,
                    NcmError *ncm_error);
NcmStringView ncm_lrc_entry_text(NcmLrcDocument *document,
                                 NcmLrcEntry *entry);
int32 ncm_lrc_document_render_plain(NcmLrcDocument *document,
                                     NcmLrcRenderTarget *target);
int32 ncm_lrc_document_entry_at_time(NcmLrcDocument *document,
                                     int64 elapsed_ms);
int32 ncm_lrc_document_next_entry_after_time(NcmLrcDocument *document,
                                             int64 elapsed_ms);

int32 ncm_run_external_command(char *command, int32 command_len, bool block,
                               NcmError *ncm_error);
int32 ncm_run_external_console_command(char *command, int32 command_len,
                                       NcmError *ncm_error);

typedef struct NcmOptionLine {
    char *option;
    char *value;
    int32 option_len;
    int32 value_len;
} NcmOptionLine;

int32 ncm_option_parser_parse_line(char *line, int32 line_len,
                                   NcmOptionLine *result, bool *parsed);
int32 ncm_option_parser_yes_no(char *value, int32 value_len, bool *result);

int32 ncm_path_expand_home(StrBuilder *path, NcmError *ncm_error);
int32 ncm_path_basename_start(char *path, int32 path_len);
int32 ncm_path_parent_directory_len(char *path, int32 path_len);
int32 ncm_path_extension_start(char *path, int32 path_len);

typedef struct NcmMpdClient NcmMpdClient;
typedef struct NcmSongArray NcmSongArray;

typedef struct NcmPlaylistSortSwap {
    int32 from;
    int32 to;
} NcmPlaylistSortSwap;

typedef struct NcmPlaylistSortPlan {
    NcmPlaylistSortSwap *items;
    int32 len;
} NcmPlaylistSortPlan;

int32 ncm_playlist_sort_range(
    NcmSongArray *songs, int32 start_position,
    enum NcmSongGetter *getters, int32 getters_len,
    bool ignore_leading_the, NcmMpdClient *client,
    NcmError *ncm_error);

typedef struct NcmSearchPromptState {
    enum SearchDirection direction;
    StrBuilder last_text;
    int32 start_position;

    bool has_start_position;
    bool has_last_result;
    bool last_found;
} NcmSearchPromptState;

void ncm_search_prompt_state_init(NcmSearchPromptState *state,
                                  enum SearchDirection direction);
void ncm_search_prompt_state_destroy(NcmSearchPromptState *state);
void ncm_search_prompt_state_set_start_position(
    NcmSearchPromptState *state, int32 position);
bool ncm_search_prompt_state_has_cached_result(NcmSearchPromptState *state,
                                               char *text, int32 text_len,
                                               bool *found);
int32 ncm_search_prompt_state_finish_result(NcmSearchPromptState *state,
                                            char *text, int32 text_len,
                                            bool search_ok, bool found);

NcmStringView ncm_string_view_make(char *data, int32 len);
void ncm_string_view_set(NcmStringView *view, char *data, int32 len);
void ncm_string_view_clear(NcmStringView *view);

void ncm_string_lowercase_ascii(char *string, int32 string_len);
int32 ncm_string_find_char(char *string, int32 string_len, char needle);
bool ncm_string_contains_char(char *string, int32 string_len, char needle);
StrBuilder ncm_string_shared_directory(char *left, int32 left_len,
                                       char *right, int32 right_len);
StrBuilder ncm_string_get_enclosed(char *string, int32 string_len,
                                   char open, char close,
                                   int32 start, int32 *pos);
void ncm_string_remove_chars(char *string, int32 *string_len,
                             char *chars, int32 chars_len);
void ncm_string_remove_invalid_filename_chars(char *filename,
                                              int32 *filename_len,
                                              bool win32_compatible);
void ncm_string_append_shell_escaped_single_quotes(StrBuilder *buffer,
                                                   char *string,
                                                   int32 string_len);
int32 ncm_string_basename_start(char *path, int32 path_len);
int32 ncm_string_parent_directory_len(char *path, int32 path_len);

typedef struct NcmTaglibFile {
    void *handle;
} NcmTaglibFile;

typedef struct NcmTaglibAudioProperties {
    int32 length;
    int32 bitrate;
    int32 sample_rate;
    int32 channels;
} NcmTaglibAudioProperties;

typedef void NcmTaglibPairCallback(char *name, char *value, void *user);
typedef void NcmTaglibValueCallback(char *value, void *user);

int32 ncm_taglib_file_open(NcmTaglibFile *file, char *path);
void ncm_taglib_file_close(NcmTaglibFile *file);
int32 ncm_taglib_file_save(NcmTaglibFile *file);
int32 ncm_taglib_file_audio_properties(NcmTaglibFile *file,
                                       NcmTaglibAudioProperties *properties);
int32 ncm_taglib_read_mapped_properties(NcmTaglibFile *file,
                                        NcmTaglibPairCallback *callback,
                                        void *user);
int32 ncm_taglib_read_property(NcmTaglibFile *file, char *property,
                               NcmTaglibValueCallback *callback, void *user);
int32 ncm_taglib_clear_property(NcmTaglibFile *file, char *property);
int32 ncm_taglib_append_property(NcmTaglibFile *file, char *property,
                                 char *value);
bool ncm_taglib_file_can_set_extended_tags(NcmTaglibFile *file);
void ncm_taglib_clear_strings(void);

#include "curses/nc_curses.h"

#define ENUM_NAME NcmFormatFlags
#define ENUM_PREFIX_ NCM_FORMAT_FLAG_
#define ENUM_BITFLAGS 1
#define ENUM_FIELDS                         \
    XX(NCM_FORMAT_FLAG_COLOR)               \
    XX(NCM_FORMAT_FLAG_FORMAT)              \
    XX(NCM_FORMAT_FLAG_OUTPUT_SWITCH)       \
    XX(NCM_FORMAT_FLAG_TAG)
#include "cbase/xenums.c"

#define NCM_FORMAT_FLAG_ALL                 \
    (NCM_FORMAT_FLAG_COLOR                  \
     |NCM_FORMAT_FLAG_FORMAT                \
     |NCM_FORMAT_FLAG_OUTPUT_SWITCH         \
     |NCM_FORMAT_FLAG_TAG)

#define ENUM_NAME NcmFormatExprType
#define ENUM_PREFIX_ NCM_FORMAT_EXPR_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                         \
    XX(NCM_FORMAT_EXPR_TEXT)                \
    XX(NCM_FORMAT_EXPR_COLOR)               \
    XX(NCM_FORMAT_EXPR_FORMAT)              \
    XX(NCM_FORMAT_EXPR_OUTPUT_SWITCH)       \
    XX(NCM_FORMAT_EXPR_SONG_TAG)            \
    XX(NCM_FORMAT_EXPR_GROUP)               \
    XX(NCM_FORMAT_EXPR_FIRST_OF)
#include "cbase/xenums.c"

#define ENUM_NAME NcmFormatResult
#define ENUM_PREFIX_ NCM_FORMAT_RESULT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                         \
    XX(NCM_FORMAT_RESULT_EMPTY)             \
    XX(NCM_FORMAT_RESULT_MISSING)           \
    XX(NCM_FORMAT_RESULT_OK)
#include "cbase/xenums.c"

typedef struct NcmFormatSongTag {
    enum NcmSongGetter getter;
    uint32 delimiter;
} NcmFormatSongTag;

typedef struct NcmFormatExpr NcmFormatExpr;

typedef struct NcmFormatExprList {
    NcmFormatExpr *items;
    int32 len;
    int32 cap;
} NcmFormatExprList;

struct NcmFormatExpr {
    enum NcmFormatExprType type;
    union {
        StrBuilder text;
        NcColor color;
        enum NcFormat format;
        NcmFormatSongTag song_tag;
        NcmFormatExprList list;
    } value;
};

typedef struct NcmFormatAst {
    NcmFormatExprList root;
} NcmFormatAst;

typedef struct NcmFormatCallbacks {
    void (*text)(void *user, char *data, int32 data_len, NcmFormatSongTag *tag);
    void (*color)(void *user, NcColor color);
    void (*format)(void *user, enum NcFormat format);
} NcmFormatCallbacks;

void ncm_format_expr_list_destroy(NcmFormatExprList *list);
void ncm_format_expr_list_clear(NcmFormatExprList *list);
void ncm_format_expr_list_move(NcmFormatExprList *dest,
                               NcmFormatExprList *source);
NcmFormatExpr *ncm_format_expr_list_append(NcmFormatExprList *list);

void ncm_format_ast_destroy(NcmFormatAst *ast);
void ncm_format_ast_clear(NcmFormatAst *ast);
void ncm_format_ast_move(NcmFormatAst *dest, NcmFormatAst *source);
int32 ncm_format_ast_append_column_types(NcmFormatAst *ast,
                                         char *types, int32 types_len);

int32 ncm_format_parse(NcmFormatAst *ast, char *data, int32 data_len,
                       uint32 flags, NcmError *ncm_error);

void ncm_format_render(NcmFormatAst *ast, NcmSong *song,
                       NcmFormatCallbacks *callbacks, void *output,
                       void *second_output, uint32 flags);
void ncm_format_render_buffer(NcmFormatAst *ast, NcmSong *song,
                              NcBuffer *buffer, NcBuffer *right_aligned,
                              uint32 flags);
StrBuilder ncm_format_render_string(NcmFormatAst *ast, NcmSong *song);
StrBuilder ncm_format_render_tag(NcmSong *song, NcmFormatSongTag *tag);

struct Column;

void ncm_display_song_row(NcBuffer *buffer, NcmFormatAst *format,
                          NcmSong *song, uint32 flags);
void ncm_display_song_columns(NcBuffer *buffer, NcmSong *song,
                              struct Column *columns, int32 column_count,
                              int32 list_width, bool use_colors);
void ncm_display_column_title(StrBuilder *buffer,
                              struct Column *columns,
                              int32 column_count, int32 list_width);
void ncm_display_directory_row(NcBuffer *buffer, NcmDirectory *directory);
void ncm_display_playlist_row(NcBuffer *buffer, NcmPlaylist *playlist,
                              char *prefix, int32 prefix_len);

#endif /* NCM_C_H */
