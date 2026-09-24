#if !defined(NCM_C_H)
#define NCM_C_H

#include "cbase.h"
#include "ncmpcpp2.h"

#include "configura.h"

#include <regex.h>

typedef struct NcmError {
    char message[256];
    int32 message_len;
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

void ncm_error_clear(NcmError *);
void ncm_error_set(NcmError *, int32 code, char *, int32 message_len);
bool ncm_error_is_set(NcmError *);
int32 ncm_error_code_from_status(int32);
int32 ncm_status_from_error_code(int32);
int32 ncm_error_set_code(NcmError *, int32 code, char *, int32 message_len);
int32 ncm_error_status(NcmError *);
int32 ncm_error_set_status(NcmError *, int32 status, char *, int32 message_len);
int32 ncm_error_ok(NcmError *);

void stupid_string_free(char **, int32 *len);
void stupid_string_set(char **, int32 *dest_len, char *, int32);

typedef struct TagsReplayGainInfo {
    StrView reference_loudness;
    StrView track_gain;
    StrView track_peak;
    StrView album_gain;
    StrView album_peak;
} TagsReplayGainInfo;

typedef bool TagsGetTagCallback(enum TagType, int32, StrView *, void *);

int32 ncm_tags_write(char *music_dir, char *uri, bool, char *directory,
                     char *new_name, TagsGetTagCallback *, void *);

#define ENUM_NAME NcmItemType
#define ENUM_PREFIX_ NCM_ITEM_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                         \
    XX(NCM_ITEM_DIRECTORY)                  \
    XX(NCM_ITEM_SONG)                       \
    XX(NCM_ITEM_PLAYLIST)
#include "cbase/xenums.c"

int32 ncm_channels_to_string(int32 channels, char *, int32 buffer_cap);
int32 ncm_color_index_from_char(char);
int32 ncm_tag_type_name_len(enum TagType, char **);
char *ncm_tag_type_name(enum TagType);
enum TagType ncm_char_to_tag_type(char);
enum SongGetter ncm_song_getter_from_char(char);
enum TagType ncm_song_getter_to_tag_type(enum SongGetter);
enum SongGetter ncm_tag_type_to_song_getter(enum TagType);

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
    enum TagType type;
} NcmSongTag;

typedef struct NcmSongProperty {
    char *name;
    char *value;
    int32 name_len;
    int32 value_len;
} NcmSongProperty;

typedef struct NcmSong {
    char *uri;
    int32 uri_len;

    NcmSongTag *tags;
    NcmSongProperty *properties;
    int32 tags_len;
    int32 tags_cap;
    int32 properties_len;
    int32 properties_cap;

    int32 duration;
    int32 position;
    int32 id;
    int32 priority;
    time_t last_modified;
} NcmSong;

void ncm_song_destroy(NcmSong *);
void ncm_song_move(NcmSong *dest, NcmSong *source);
int32 ncm_song_copy(NcmSong *dest, NcmSong *source);
int32 ncm_song_set_uri(NcmSong *, char *, int32);
int32 ncm_song_add_tag(NcmSong *, enum TagType, char *, int32);
int32 ncm_song_add_property(NcmSong *, char *, int32, char *, int32);
void ncm_song_set_duration(NcmSong *, int32);
void ncm_song_set_position(NcmSong *, int32);
void ncm_song_set_id(NcmSong *, int32);
void ncm_song_set_priority(NcmSong *, int32);
void ncm_song_set_mtime(NcmSong *, time_t);
int32 ncm_song_duration(NcmSong *);
int32 ncm_song_position(NcmSong *);
int32 ncm_song_id(NcmSong *);
int32 ncm_song_priority(NcmSong *);
time_t ncm_song_mtime(NcmSong *);
bool ncm_song_is_empty(NcmSong *);

bool ncm_song_has_tag_view(NcmSong *, enum TagType, int32, StrView *);
bool ncm_song_has_uri_view(NcmSong *, int32, StrView *);
bool ncm_song_has_filename_view(NcmSong *, int32, StrView *);
bool ncm_song_has_directory_view(NcmSong *, int32, StrView *);
bool ncm_song_is_from_database(NcmSong *);
bool ncm_song_is_stream(NcmSong *);

int32 ncm_song_numeric_tag_len(char *, int32);
int32 ncm_song_format_numeric_tag(char *buffer, int32 buffer_cap,
                                  char *tag, int32 tag_len);
int32 ncm_song_show_time(int32 length, char *, int32 buffer_cap);
String ncm_song_getter_buffer(NcmSong *, enum SongGetter, int32);
String ncm_song_tags_buffer(NcmSong *, enum SongGetter, char *, int32,
                                bool);
bool ncm_song_is_equal(NcmSong *a, NcmSong *b);

typedef struct MutableSongTag {
    char *original;
    char *value;

    int32 original_len;
    int32 value_len;
    int32 idx;

    enum TagType type;
    bool modified;
} MutableSongTag;

typedef struct MutableSong {
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

    MutableSongTag *tags;
    int32 tags_len;
    int32 tags_cap;
} MutableSong;

void mutable_song_destroy(MutableSong *);
int32 mutable_song_copy(MutableSong *dest, MutableSong *source);
void mutable_song_move(MutableSong *dest, MutableSong *source);

int32 mutable_song_set_tag(MutableSong *, enum TagType, int32 idx,
                           char *, int32 value_len);
int32 mutable_song_set_tags(MutableSong *, enum TagType,
                            char *value, int32 value_len,
                            char *separator, int32 separator_len);
bool mutable_song_has_tag_view(MutableSong *, enum TagType, int32, StrView *);
void mutable_song_get_tag_buffer(MutableSong *, enum TagType,
                                 int32, String *);
String mutable_song_tags_buffer(MutableSong *, enum TagType,
                                    char *, int32, bool);
int32 mutable_song_load_originals_from_song(MutableSong *, NcmSong *);

int32 mutable_song_set_new_name(MutableSong *, char *, int32);
bool mutable_song_has_new_name_view(MutableSong *, StrView *);

int32 mutable_song_duration(MutableSong *);
int32 mutable_song_mtime(MutableSong *);

bool mutable_song_is_modified(MutableSong *);
void mutable_song_clear_modifications(MutableSong *);
int32 mutable_song_write(MutableSong *, char *);

typedef struct NcmDirectory {
    char *path;
    int32 path_len;
    time_t last_modified;
} NcmDirectory;

void ncm_directory_destroy(NcmDirectory *);
void ncm_directory_move(NcmDirectory *dest, NcmDirectory *source);
int32 ncm_directory_set(NcmDirectory *, char *, int32, time_t);
int32 ncm_directory_copy(NcmDirectory *dest, NcmDirectory *source);
bool ncm_directory_has_path_view(NcmDirectory *, StrView *);
time_t ncm_directory_last_modified(NcmDirectory *);
typedef struct NcmPlaylist {
    char *path;
    int32 path_len;
    time_t last_modified;
} NcmPlaylist;

void ncm_playlist_destroy(NcmPlaylist *);
void ncm_playlist_move(NcmPlaylist *dest, NcmPlaylist *source);
int32 ncm_playlist_set(NcmPlaylist *, char *, int32, time_t);
int32 ncm_playlist_copy(NcmPlaylist *dest, NcmPlaylist *source);
bool ncm_playlist_has_path_view(NcmPlaylist *, StrView *);
time_t ncm_playlist_last_modified(NcmPlaylist *);

#define ENUM_NAME NcmMpdItemKind
#define ENUM_PREFIX_ NCM_MPD_ITEM_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                     \
    XX(NCM_MPD_ITEM_SONG)               \
    XX(NCM_MPD_ITEM_DIRECTORY)          \
    XX(NCM_MPD_ITEM_PLAYLIST)
#include "cbase/xenums.c"

typedef struct NcmMpdItem {
    enum NcmMpdItemKind kind;
    union {
        NcmSong song;
        NcmDirectory directory;
        NcmPlaylist playlist;
    };
} NcmMpdItem;

void ncm_mpd_item_init(NcmMpdItem *);
void ncm_mpd_item_destroy(NcmMpdItem *);
void ncm_mpd_item_move(NcmMpdItem *dest, NcmMpdItem *source);
int32 ncm_mpd_item_copy(NcmMpdItem *dest, NcmMpdItem *source);
int32 ncm_mpd_item_set_song(NcmMpdItem *, NcmSong *);
int32 ncm_mpd_item_set_directory(NcmMpdItem *, NcmDirectory *);
int32 ncm_mpd_item_from_entity_copy(NcmMpdItem *, void *);
int32 ncm_mpd_item_song_from_mpd_song_copy(NcmSong *, void *);
int32 ncm_mpd_item_song_from_mpd_song_copy_with_properties(NcmSong *, void *);
int32 ncm_mpd_item_playlist_from_mpd_playlist(NcmPlaylist *, void *);
void ncm_mpd_item_local_song(NcmSong *, char *path, int32 path_len, time_t);
enum NcmMpdItemKind ncm_mpd_item_kind(NcmMpdItem *);
NcmSong *ncm_mpd_item_song(NcmMpdItem *);
NcmDirectory *ncm_mpd_item_directory(NcmMpdItem *);
NcmPlaylist *ncm_mpd_item_playlist(NcmMpdItem *);

typedef struct NcmSampleBuffer {
    int16 *data;
    int32 len;
    int32 cap;
} NcmSampleBuffer;

void ncm_sample_buffer_destroy(NcmSampleBuffer *);
int32 ncm_sample_buffer_put(NcmSampleBuffer *, int16 *, int32);
int32 ncm_sample_buffer_get_clamped(NcmSampleBuffer *, int32 samples_len,
                                    int16 *, int32 dest_len);
void ncm_sample_buffer_resize(NcmSampleBuffer *, int32);

#define NCM_ARRAY_TYPE NcmSongArray
#define NCM_ARRAY_ITEM_TYPE NcmSong
#define NCM_ARRAY_PREFIX ncm_song_array
#define NCM_ARRAY_COPY
#define NCM_ARRAY_MOVE
#define NCM_ARRAY_APPEND_COPY
#define NCM_ARRAY_APPEND_MOVE
#include "c/ncm_array_decl_template.h"

#define NCM_ARRAY_TYPE NcmDirectoryArray
#define NCM_ARRAY_ITEM_TYPE NcmDirectory
#define NCM_ARRAY_PREFIX ncm_directory_array
#define NCM_ARRAY_COPY
#define NCM_ARRAY_MOVE
#define NCM_ARRAY_APPEND_COPY
#define NCM_ARRAY_APPEND_MOVE
#include "c/ncm_array_decl_template.h"

#define NCM_ARRAY_TYPE NcmPlaylistArray
#define NCM_ARRAY_ITEM_TYPE NcmPlaylist
#define NCM_ARRAY_PREFIX ncm_playlist_array
#define NCM_ARRAY_MOVE
#define NCM_ARRAY_APPEND_COPY
#define NCM_ARRAY_APPEND_MOVE
#include "c/ncm_array_decl_template.h"

#define NCM_ARRAY_TYPE NcmMpdItemArray
#define NCM_ARRAY_ITEM_TYPE NcmMpdItem
#define NCM_ARRAY_PREFIX ncm_mpd_item_array
#define NCM_ARRAY_MOVE
#define NCM_ARRAY_APPEND_COPY
#define NCM_ARRAY_APPEND_MOVE
#include "c/ncm_array_decl_template.h"

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

typedef bool NcmRegexMatchCallback(int32 start, int32 len, void *);

typedef struct NcmRegex {
    regex_t regex;
    bool compiled;
    uint32 flags;
} NcmRegex;

void ncm_regex_destroy(NcmRegex *);
int32 ncm_regex_compile(NcmRegex *, char *, int32, uint32, NcmError *);
bool ncm_regex_matches(NcmRegex *, char *, int32);
int32 ncm_regex_for_each_match(NcmRegex *, char *, int32,
                               NcmRegexMatchCallback *, void *);

enum NcmMpdError {
    NCM_MPD_ERROR_SUCCESS = 0,
    NCM_MPD_ERROR_OOM,
    NCM_MPD_ERROR_ARGUMENT,
    NCM_MPD_ERROR_STATE,
    NCM_MPD_ERROR_TIMEOUT,
    NCM_MPD_ERROR_SYSTEM,
    NCM_MPD_ERROR_RESOLVER,
    NCM_MPD_ERROR_CLOSED,
    NCM_MPD_ERROR_MALFORMED,
    NCM_MPD_ERROR_SERVER,
};

enum NcmMpdServerError {
    NCM_MPD_SERVER_ERROR_NONE = 0,
    NCM_MPD_SERVER_ERROR_EXIST,
    NCM_MPD_SERVER_ERROR_NO_EXIST,
    NCM_MPD_SERVER_ERROR_PERMISSION,
    NCM_MPD_SERVER_ERROR_UNKNOWN,
};

#define ENUM_NAME NcmMpdIdle
#define ENUM_PREFIX_ NCM_MPD_IDLE_
#define ENUM_BITFLAGS 1
#define ENUM_FIELDS                         \
    XX(NCM_MPD_IDLE_DATABASE)               \
    XX(NCM_MPD_IDLE_STORED_PLAYLIST)        \
    XX(NCM_MPD_IDLE_PLAYLIST)               \
    XX(NCM_MPD_IDLE_PLAYER)                 \
    XX(NCM_MPD_IDLE_MIXER)                  \
    XX(NCM_MPD_IDLE_OUTPUT)                 \
    XX(NCM_MPD_IDLE_UPDATE)                 \
    XX(NCM_MPD_IDLE_OPTIONS)
#include "cbase/xenums.c"

enum NcmMpdState {
    NCM_MPD_STATE_UNKNOWN = 0,
    NCM_MPD_STATE_STOP,
    NCM_MPD_STATE_PLAY,
    NCM_MPD_STATE_PAUSE,
};

typedef struct MpdConnection {
    void *mpd;
    NcmError ncm_error;
    enum NcmMpdError error_code;
    enum NcmMpdServerError server_error_code;
    bool error_clearable;
} MpdConnection;

typedef struct NcmMpdStats {
    int32 artists;
    int32 albums;
    int32 songs;
    int32 play_time;
    int32 uptime;
    int32 db_update_time;
    int32 db_play_time;
} NcmMpdStats;

typedef struct NcmMpdOutput {
    int32 id;
    char *name;
    int32 name_len;
    bool enabled;
} NcmMpdOutput;

typedef struct NcmMpdOutputList {
    NcmMpdOutput *items;
    int32 len;
    int32 capacity;
} NcmMpdOutputList;

#define ENUM_NAME NcmMpdReplayGainMode
#define ENUM_PREFIX_ NCM_MPD_REPLAY_GAIN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                        \
    XX(NCM_MPD_REPLAY_GAIN_OFF, off)       \
    XX(NCM_MPD_REPLAY_GAIN_TRACK, track)   \
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
    enum NcmMpdState state;
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

void ncm_mpd_connection_destroy(MpdConnection *);
int32 ncm_mpd_connection_connect(MpdConnection *, char *, uint16, int32);
void ncm_mpd_connection_disconnect(MpdConnection *);
bool ncm_mpd_connection_is_connected(MpdConnection *);
int32 ncm_mpd_connection_fd(MpdConnection *);
int32 ncm_mpd_connection_set_timeout(MpdConnection *, int32);
int32 ncm_mpd_connection_noidle(MpdConnection *);
int32 ncm_mpd_connection_send_idle(MpdConnection *, uint32);
int32 ncm_mpd_connection_recv_idle(MpdConnection *, bool, uint32 *);
int32 ncm_mpd_connection_check_error(MpdConnection *);
char *ncm_mpd_connection_error(MpdConnection *);
void ncm_mpd_connection_clear_error(MpdConnection *);
enum NcmMpdError ncm_mpd_connection_error_code(MpdConnection *);
enum NcmMpdServerError ncm_mpd_connection_server_error_code(MpdConnection *);
bool ncm_mpd_connection_error_is_clearable(MpdConnection *);
int32 ncm_mpd_connection_get_stats(MpdConnection *, NcmMpdStats *);
int32 ncm_mpd_connection_get_status(MpdConnection *, NcmMpdStatus *);

int32 ncm_mpd_connection_version(MpdConnection *);
int32 ncm_mpd_connection_send_password(MpdConnection *, char *);
int32 ncm_mpd_connection_start_command_list(MpdConnection *);
int32 ncm_mpd_connection_commit_command_list(MpdConnection *);
int32 ncm_mpd_connection_get_supported_extensions(MpdConnection *,
                                                  StrFlexList *);
int32 ncm_mpd_connection_get_replay_gain_mode(MpdConnection *,
                                              enum NcmMpdReplayGainMode *);
int32 ncm_mpd_connection_set_replay_gain_mode(MpdConnection *,
                                              enum NcmMpdReplayGainMode);
int32 ncm_mpd_connection_get_playlists(MpdConnection *,
                                       NcmPlaylistArray *);
int32 ncm_mpd_connection_list_all_song_uris(MpdConnection *, char *,
                                            StrFlexList *);
int32 ncm_mpd_connection_get_url_handlers(MpdConnection *,
                                          StrFlexList *);
int32 ncm_mpd_connection_get_tag_types(MpdConnection *, StrFlexList *);

int32 ncm_mpd_item_array_to_directory_array(NcmMpdItemArray *,
                                            NcmDirectoryArray *);

void ncm_mpd_output_list_destroy(NcmMpdOutputList *);
void ncm_mpd_output_list_clear(NcmMpdOutputList *);

int32 ncm_mpd_connection_get_current_song(MpdConnection *, NcmSong *);
int32 ncm_mpd_connection_get_queue(MpdConnection *, NcmSongArray *);
int32 ncm_mpd_connection_get_queue_changes(MpdConnection *, int32,
                                           NcmSongArray *);
int32 ncm_mpd_connection_get_playlist_content(MpdConnection *, char *,
                                              NcmSongArray *);
int32 ncm_mpd_connection_get_playlist_content_no_info(MpdConnection *,
                                                      char *, NcmSongArray *);

int32 ncm_mpd_connection_get_directory(MpdConnection *, char *,
                                       NcmMpdItemArray *);
int32 ncm_mpd_connection_get_directory_songs(MpdConnection *, char *,
                                             NcmSongArray *);
int32 ncm_mpd_connection_list_all_songs(MpdConnection *, char *,
                                        NcmSongArray *);
int32 ncm_mpd_connection_start_search_songs(MpdConnection *, bool);
int32 ncm_mpd_connection_add_search_tag(MpdConnection *, enum TagType,
                                        char *);
int32 ncm_mpd_connection_add_search_any(MpdConnection *, char *);
int32 ncm_mpd_connection_add_search_uri(MpdConnection *, char *);
int32 ncm_mpd_connection_commit_search_songs(MpdConnection *,
                                             NcmSongArray *);
int32 ncm_mpd_connection_list_tag_values(MpdConnection *, enum TagType,
                                         StrFlexList *);

int32 ncm_mpd_connection_update_database(MpdConnection *, char *, int32 *);
int32 ncm_mpd_connection_get_outputs(MpdConnection *, NcmMpdOutputList *);
int32 ncm_mpd_connection_enable_output(MpdConnection *, int32);
int32 ncm_mpd_connection_disable_output(MpdConnection *, int32);

int32 ncm_mpd_connection_play(MpdConnection *);
int32 ncm_mpd_connection_play_pos(MpdConnection *, int32);
int32 ncm_mpd_connection_play_id(MpdConnection *, int32);
int32 ncm_mpd_connection_toggle_pause(MpdConnection *);
int32 ncm_mpd_connection_stop(MpdConnection *);
int32 ncm_mpd_connection_next(MpdConnection *);
int32 ncm_mpd_connection_previous(MpdConnection *);
int32 ncm_mpd_connection_seek_pos(MpdConnection *, int32 pos, int32 seconds);
int32 ncm_mpd_connection_set_repeat(MpdConnection *, bool);
int32 ncm_mpd_connection_set_random(MpdConnection *, bool);
int32 ncm_mpd_connection_set_single(MpdConnection *, bool);
int32 ncm_mpd_connection_set_consume(MpdConnection *, bool);
int32 ncm_mpd_connection_set_crossfade(MpdConnection *, int32);
int32 ncm_mpd_connection_set_volume(MpdConnection *, int32);
int32 ncm_mpd_connection_change_volume(MpdConnection *, int32);

int32 ncm_mpd_connection_move(MpdConnection *, int32 from, int32 to, bool);
int32 ncm_mpd_connection_swap(MpdConnection *, int32 from, int32 to, bool);
int32 ncm_mpd_connection_shuffle(MpdConnection *);
int32 ncm_mpd_connection_shuffle_range(MpdConnection *, int32 start, int32 end);
int32 ncm_mpd_connection_clear_queue(MpdConnection *);
int32 ncm_mpd_connection_set_priority_id(MpdConnection *, int32 id,
                                         int32 prio, bool);
int32 ncm_mpd_connection_add_song(MpdConnection *, char *, int32, bool,
                                  int32 *);
int32 ncm_mpd_connection_add(MpdConnection *, char *, bool, bool *);
int32 ncm_mpd_connection_delete(MpdConnection *, int32, bool);
int32 ncm_mpd_connection_clear_playlist(MpdConnection *, char *);
int32 ncm_mpd_connection_add_to_playlist(MpdConnection *, char *playlist,
                                         char *path, bool);
int32 ncm_mpd_connection_playlist_move(MpdConnection *, char *, int32 from,
                                       int32 to, bool);
int32 ncm_mpd_connection_playlist_delete(MpdConnection *, char *, int32, bool);
int32 ncm_mpd_connection_rename_playlist(MpdConnection *, char *from, char *to);
int32 ncm_mpd_connection_delete_playlist(MpdConnection *, char *);
int32 ncm_mpd_connection_load_playlist(MpdConnection *, char *, bool *);
int32 ncm_mpd_connection_save_playlist(MpdConnection *, char *);

typedef void NcmMpdNoidleCallback(uint32, void *);

typedef struct MpdClient {
    MpdConnection connection;
    String host;
    String password;
    uint16 port;
    int32 timeout_ms;
    bool command_list_active;
    bool idle;
    int32 fd;
    NcmMpdNoidleCallback *noidle_callback;
    void *noidle_user;
} MpdClient;

void ncm_mpd_client_init(MpdClient *);
void ncm_mpd_client_destroy(MpdClient *);
char *ncm_mpd_client_hostname(MpdClient *);
bool ncm_mpd_client_is_connected(MpdClient *);
int32 ncm_mpd_client_version(MpdClient *);
int32 ncm_mpd_client_fd(MpdClient *);
void ncm_mpd_client_set_noidle_callback(MpdClient *, NcmMpdNoidleCallback *,
                                        void *);
int32 ncm_mpd_client_set_hostname(MpdClient *, char *, int32, NcmError *);
void ncm_mpd_client_set_port(MpdClient *, uint16);
int32 ncm_mpd_client_set_password(MpdClient *, char *, int32, NcmError *);
int32 ncm_mpd_client_set_timeout_ms(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_connect(MpdClient *, NcmError *);
void ncm_mpd_client_disconnect(MpdClient *);
int32 ncm_mpd_client_send_password(MpdClient *, NcmError *);
int32 ncm_mpd_client_idle(MpdClient *, NcmError *);
int32 ncm_mpd_client_noidle(MpdClient *, uint32 *, NcmError *);

enum NcmMpdError ncm_mpd_client_error_code(MpdClient *);
enum NcmMpdServerError ncm_mpd_client_server_error_code(MpdClient *);
bool ncm_mpd_client_error_is_clearable(MpdClient *);
char *ncm_mpd_client_error_message(MpdClient *);

int32 ncm_mpd_client_get_stats(MpdClient *, NcmMpdStats *, NcmError *);
int32 ncm_mpd_client_get_status(MpdClient *, NcmMpdStatus *, NcmError *);
int32 ncm_mpd_client_update_directory(MpdClient *, char *, int32 *,
                                      NcmError *);

int32 ncm_mpd_client_play(MpdClient *, NcmError *);
int32 ncm_mpd_client_play_pos(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_play_id(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_toggle_pause(MpdClient *, NcmError *);
int32 ncm_mpd_client_stop(MpdClient *, NcmError *);
int32 ncm_mpd_client_next(MpdClient *, NcmError *);
int32 ncm_mpd_client_previous(MpdClient *, NcmError *);
int32 ncm_mpd_client_move(MpdClient *, int32 from, int32 to, NcmError *);
int32 ncm_mpd_client_swap(MpdClient *, int32 from, int32 to, NcmError *);
int32 ncm_mpd_client_seek_pos(MpdClient *, int32 pos, int32 seconds,
                              NcmError *);
int32 ncm_mpd_client_shuffle(MpdClient *, NcmError *);
int32 ncm_mpd_client_shuffle_range(MpdClient *, int32 start, int32 end,
                                   NcmError *);
int32 ncm_mpd_client_clear_queue(MpdClient *, NcmError *);

int32 ncm_mpd_client_get_queue(MpdClient *, NcmSongArray *, NcmError *);
int32 ncm_mpd_client_get_queue_changes(MpdClient *, int32, NcmSongArray *,
                                       NcmError *);
int32 ncm_mpd_client_get_current_song(MpdClient *, NcmSong *, NcmError *);
int32 ncm_mpd_client_get_playlist_content(MpdClient *, char *,
                                          NcmSongArray *, NcmError *);
int32 ncm_mpd_client_get_playlist_content_no_info(MpdClient *, char *,
                                                  NcmSongArray *, NcmError *);
int32 ncm_mpd_client_get_supported_extensions(MpdClient *,
                                              StrFlexList *, NcmError *);

int32 ncm_mpd_client_set_repeat(MpdClient *, bool, NcmError *);
int32 ncm_mpd_client_set_random(MpdClient *, bool, NcmError *);
int32 ncm_mpd_client_set_single(MpdClient *, bool, NcmError *);
int32 ncm_mpd_client_set_consume(MpdClient *, bool, NcmError *);
int32 ncm_mpd_client_set_crossfade(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_set_volume(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_change_volume(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_get_replay_gain_mode(MpdClient *,
                                          enum NcmMpdReplayGainMode *,
                                          NcmError *);
int32 ncm_mpd_client_set_replay_gain_mode(MpdClient *,
                                          enum NcmMpdReplayGainMode,
                                          NcmError *);

int32 ncm_mpd_client_set_priority_song(MpdClient *, NcmSong *, int32,
                                       NcmError *);
int32 ncm_mpd_client_add_song_value(MpdClient *, NcmSong *, int32, int32 *,
                                    NcmError *);
int32 ncm_mpd_client_add_song_array(MpdClient *, NcmSongArray *, int32,
                                   NcmError *);
int32 ncm_mpd_client_add(MpdClient *, char *, bool *, NcmError *);
int32 ncm_mpd_client_add_random_tag(MpdClient *, enum TagType, int32,
                                    NcmError *);
int32 ncm_mpd_client_add_random_songs(MpdClient *, int32 number, char *,
                                      int32 exclude_pattern_len, NcmError *);
int32 ncm_mpd_client_delete(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_start_command_list(MpdClient *, NcmError *);
int32 ncm_mpd_client_commit_command_list(MpdClient *, NcmError *);

int32 ncm_mpd_client_delete_playlist(MpdClient *, char *, NcmError *);
int32 ncm_mpd_client_load_playlist(MpdClient *, char *, bool *, NcmError *);
int32 ncm_mpd_client_save_playlist(MpdClient *, char *, NcmError *);
int32 ncm_mpd_client_clear_playlist(MpdClient *, char *, NcmError *);
int32 ncm_mpd_client_add_song_to_playlist(MpdClient *, char *, NcmSong *,
                                          NcmError *);
int32 ncm_mpd_client_playlist_move(MpdClient *, char *, int32 from,
                                   int32 to, NcmError *);
int32 ncm_mpd_client_playlist_delete(MpdClient *, char *, int32, NcmError *);
int32 ncm_mpd_client_rename_playlist(MpdClient *, char *from, char *to,
                                     NcmError *);

int32 ncm_mpd_client_start_search(MpdClient *, bool, NcmError *);
int32 ncm_mpd_client_add_search_tag(MpdClient *, enum TagType, char *,
                                    NcmError *);
int32 ncm_mpd_client_add_search_any(MpdClient *, char *, NcmError *);
int32 ncm_mpd_client_add_search_uri(MpdClient *, char *, NcmError *);
int32 ncm_mpd_client_commit_search_songs(MpdClient *, NcmSongArray *,
                                         NcmError *);

int32 ncm_mpd_client_get_playlists(MpdClient *, NcmPlaylistArray *,
                                   NcmError *);
int32 ncm_mpd_client_get_list(MpdClient *, enum TagType,
                              StrFlexList *, NcmError *);
int32 ncm_mpd_client_get_directory_recursive(MpdClient *, char *,
                                             NcmSongArray *, NcmError *);
int32 ncm_mpd_client_get_songs(MpdClient *, char *, NcmSongArray *,
                               NcmError *);
int32 ncm_mpd_client_get_directory_entries(MpdClient *, char *,
                                           NcmMpdItemArray *, NcmError *);
int32 ncm_mpd_client_get_directory_list(MpdClient *, char *,
                                        NcmDirectoryArray *, NcmError *);
int32 ncm_mpd_client_get_outputs(MpdClient *, NcmMpdOutputList *,
                                 NcmError *);
int32 ncm_mpd_client_enable_output(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_disable_output(MpdClient *, int32, NcmError *);
int32 ncm_mpd_client_get_url_handlers(MpdClient *, StrFlexList *,
                                      NcmError *);
int32 ncm_mpd_client_get_tag_types(MpdClient *, StrFlexList *, NcmError *);

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

int32 ncm_compare_locale_strings(char *left, int32 left_len, char *right,
                                 int32 right_len, bool);

int32 ncm_parse_double(char *, int32, double *, NcmError *);

int32 ncm_bounds_check_i64(int64 value, int64 min, int64 max, NcmError *);
int32 ncm_bounds_check_f64(double value, double min, double max, NcmError *);

int32 ncm_lower_bound_check_f64(double value, double lbound, NcmError *);

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

void ncm_fs_entry_init(NcmFsEntry *);
void ncm_fs_entry_destroy(NcmFsEntry *);
int32 ncm_fs_stat(char *, int32, NcmFsStat *, NcmError *);
bool ncm_fs_path_is_existing(char *, int32);
int32 ncm_fs_unlink(char *, int32, NcmError *);
int32 ncm_fs_rename(char *old_path, int32 old_path_len, char *new_path,
                    int32 new_path_len, NcmError *);
int32 ncm_fs_mkdir_all(char *, int32, NcmError *);
int32 ncm_fs_directory_open(NcmFsDirectory *, char *, int32, NcmError *);
int32 ncm_fs_directory_read(NcmFsDirectory *, NcmFsEntry *, NcmError *);
void ncm_fs_directory_close(NcmFsDirectory *);
int32 ncm_fs_join(String *, char *left, int32 left_len, char *right,
                  int32 right_len);

String ncm_html_unescape_utf8(char *, int32);
String ncm_html_unescape_entities(char *, int32);
String ncm_html_strip_tags(char *, int32);

typedef int32 NcmJobRunCallback(void *, NcmError *);
typedef void NcmJobCompleteCallback(int32, NcmError *, void *);
typedef void NcmJobDestroyCallback(void *);

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

void ncm_job_queue_init(NcmJobQueue *);
int32 ncm_job_queue_start(NcmJobQueue *, NcmError *);
int32 ncm_job_queue_push(NcmJobQueue *, NcmJob, NcmError *);
int32 ncm_job_queue_dispatch_completed(NcmJobQueue *);
void ncm_job_queue_destroy(NcmJobQueue *);
int32 ncm_job_queue_pending_len(NcmJobQueue *);
int32 ncm_job_queue_completed_len(NcmJobQueue *);

#define NCM_LRC_NO_BUFFER_POSITION (-1)

typedef struct LrcEntry {
    int32 time_ms;
    int32 text_start;
    int32 text_len;
    int32 buffer_start;
    int32 buffer_end;
    int32 source_order;
    int32 blank_lines_before;
} LrcEntry;

typedef struct LrcDocument {
    String text;
    LrcEntry *entries;

    int32 offset_ms;
    bool has_offset;
} LrcDocument;

typedef struct LrcRenderTarget {
    void *user;
    int32 (*position)(void *);
    void (*append)(void *, char *, int32);
} LrcRenderTarget;

void lrc_document_clear(LrcDocument *);
void lrc_document_destroy(LrcDocument *);
int32 lrc_parse(LrcDocument *, char *, int32, NcmError *);
int32 lrc_document_render_plain(LrcDocument *, LrcRenderTarget *);
int32 lrc_document_entry_at_time(LrcDocument *, int64);
int32 lrc_document_next_entry_after_time(LrcDocument *, int64);

int32 ncm_run_external_command(char *, int32, bool, NcmError *);
int32 ncm_run_external_console_command(char *, int32, NcmError *);

typedef struct NcmOptionLine {
    char *option;
    char *value;
    int32 option_len;
    int32 value_len;
} NcmOptionLine;

int32 ncm_option_parser_parse_line(char *, int32, NcmOptionLine *, bool *);
int32 ncm_option_parser_yes_no(char *, int32, bool *);

int32 ncm_path_expand_home(String *, NcmError *);
int32 ncm_path_basename_start(char *, int32);
int32 ncm_path_parent_directory_len(char *, int32);
int32 ncm_path_extension_start(char *, int32);

typedef struct MpdClient MpdClient;
typedef struct NcmSongArray NcmSongArray;

typedef struct NcmPlaylistSortSwap {
    int32 from;
    int32 to;
} NcmPlaylistSortSwap;

typedef struct NcmPlaylistSortPlan {
    NcmPlaylistSortSwap *items;
    int32 len;
} NcmPlaylistSortPlan;

int32 ncm_playlist_sort_range(NcmSongArray *, int32 start_position,
                              enum SongGetter *, int32 getters_len, bool,
                              MpdClient *, NcmError *);

typedef struct SearchPromptState {
    enum SearchDirection direction;
    String last_text;
    int32 start_position;

    bool has_start_position;
    bool has_last_result;
    bool last_found;
} SearchPromptState;

void search_prompt_state_init(SearchPromptState *, enum SearchDirection);
void search_prompt_state_destroy(SearchPromptState *);
void search_prompt_state_set_start_position(SearchPromptState *, int32);
bool search_prompt_state_has_cached_result(SearchPromptState *,
                                               char *, int32, bool *);
int32 search_prompt_state_finish_result(SearchPromptState *,
                                            char *, int32,
                                            bool search_ok, bool found);

StrView ncm_string_view(char *, int32);
void ncm_string_view_set(StrView *, char *, int32);

void ncm_string_lowercase_ascii(char *, int32);
int32 ncm_string_find_char(char *, int32, char);
bool ncm_string_contains_char(char *, int32, char);
String ncm_string_shared_directory(char *left, int32 left_len, char *right,
                                       int32 right_len);
String ncm_string_get_enclosed(char *, int32 string_len, char open,
                                   char close, int32 start, int32 *);
void ncm_string_remove_chars(char *string, int32 *, char *chars, int32);
void ncm_string_remove_invalid_filename_chars(char *, int32 *, bool);
void ncm_string_append_shell_escaped_single_quotes(String *, char *, int32);
int32 ncm_path_basename_start(char *, int32);
int32 ncm_string_parent_directory_len(char *, int32);

typedef struct TaglibFile {
    void *handle;
} TaglibFile;

typedef struct TaglibAudioProperties {
    int32 length;
    int32 bitrate;
    int32 sample_rate;
    int32 channels;
} TaglibAudioProperties;

typedef void TaglibPairCallback(char *name, char *value, void *);
int32 ncm_taglib_file_open(TaglibFile *, char *);
void ncm_taglib_file_close(TaglibFile *);
int32 ncm_taglib_file_save(TaglibFile *);
int32 ncm_taglib_file_audio_properties(TaglibFile *,
                                       TaglibAudioProperties *);
int32 ncm_taglib_read_mapped_properties(TaglibFile *,
                                        TaglibPairCallback *, void *);
int32 ncm_taglib_clear_property(TaglibFile *, char *);
int32 ncm_taglib_append_property(TaglibFile *, char *property, char *value);
bool ncm_taglib_file_can_set_extended_tags(TaglibFile *);
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
    enum SongGetter getter;
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
        String text;
        NcColor color;
        enum NcFormat format;
        NcmFormatSongTag song_tag;
        NcmFormatExprList list;
    };
};

typedef struct NcmFormatAst {
    NcmFormatExprList root;
} NcmFormatAst;

typedef struct NcmFormatCallbacks {
    void (*text)(void *, char *, int32, NcmFormatSongTag *);
    void (*color)(void *, NcColor);
    void (*format)(void *, enum NcFormat);
} NcmFormatCallbacks;

void ncm_format_expr_list_move(NcmFormatExprList *dest,
                               NcmFormatExprList *source);
NcmFormatExpr *ncm_format_expr_list_append(NcmFormatExprList *);

void ncm_format_ast_destroy(NcmFormatAst *);
void ncm_format_ast_clear(NcmFormatAst *);
void ncm_format_ast_move(NcmFormatAst *dest, NcmFormatAst *source);
int32 ncm_format_ast_append_column_types(NcmFormatAst *, char *, int32);

int32 ncm_format_parse(NcmFormatAst *, char *, int32, uint32, NcmError *);

void ncm_format_render(NcmFormatAst *, NcmSong *, NcmFormatCallbacks *,
                       void *output, void *second_output, uint32);
void ncm_format_render_buffer(NcmFormatAst *, NcmSong *, NcBuffer *buffer,
                              NcBuffer *right_aligned, uint32);
String ncm_format_render_string(NcmFormatAst *, NcmSong *);

struct Column;

void ncm_display_song_row(NcBuffer *, NcmFormatAst *, NcmSong *, uint32);
void ncm_display_song_columns(NcBuffer *, NcmSong *, struct Column *,
                              int32 column_len, int32 list_width, bool);
void ncm_display_column_title(String *, struct Column *,
                              int32 column_len, int32 list_width);
void ncm_display_directory_row(NcBuffer *, NcmDirectory *);
void ncm_display_playlist_row(NcBuffer *, NcmPlaylist *, char *, int32);

#endif /* NCM_C_H */
