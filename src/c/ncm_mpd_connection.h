#if !defined(NCM_MPD_CONNECTION_H)
#define NCM_MPD_CONNECTION_H

#include <mpd/client.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_error.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_playlist.h"
#include "c/ncm_song.h"

typedef struct NcmMpdConnection {
    struct mpd_connection *mpd;
    NcmError error;
    enum mpd_error error_code;
    enum mpd_server_error server_error_code;
    bool error_clearable;
} NcmMpdConnection;

typedef struct NcmMpdStats {
    int32 artists;
    int32 albums;
    int32 songs;
    uint64 play_time;
    uint64 uptime;
    uint64 db_update_time;
    uint64 db_play_time;
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

typedef struct NcmMpdString {
    char *value;
    int32 value_len;
} NcmMpdString;

typedef struct NcmMpdStringList {
    NcmMpdString *items;
    int32 count;
    int32 capacity;
} NcmMpdStringList;

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

void ncm_mpd_string_init(NcmMpdString *string);
void ncm_mpd_string_destroy(NcmMpdString *string);
void ncm_mpd_output_init(NcmMpdOutput *output);
void ncm_mpd_output_destroy(NcmMpdOutput *output);

typedef struct NcmMpdPlaylistList {
    NcmPlaylist *items;
    int32 count;
    int32 capacity;
} NcmMpdPlaylistList;

enum NcmMpdReplayGainMode {
    NCM_MPD_REPLAY_GAIN_OFF,
    NCM_MPD_REPLAY_GAIN_TRACK,
    NCM_MPD_REPLAY_GAIN_ALBUM,
};

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
    int32 total_time;
    int32 kbit_rate;
    int32 update_id;
    char error[256];
} NcmMpdStatus;

void ncm_mpd_connection_init(NcmMpdConnection *connection);
void ncm_mpd_connection_destroy(NcmMpdConnection *connection);
bool ncm_mpd_connection_connect(NcmMpdConnection *connection,
                                char *host,
                                uint16 port,
                                int32 timeout_ms);
void ncm_mpd_connection_disconnect(NcmMpdConnection *connection);
bool ncm_mpd_connection_is_connected(NcmMpdConnection *connection);
int32 ncm_mpd_connection_fd(NcmMpdConnection *connection);
bool ncm_mpd_connection_set_timeout(NcmMpdConnection *connection,
                                    int32 timeout_ms);
bool ncm_mpd_connection_noidle(NcmMpdConnection *connection);
bool ncm_mpd_connection_send_idle(NcmMpdConnection *connection,
                                  enum mpd_idle events);
bool ncm_mpd_connection_recv_idle(NcmMpdConnection *connection,
                                  bool disable_timeout,
                                  enum mpd_idle *out_events);
bool ncm_mpd_connection_check_error(NcmMpdConnection *connection);
char *ncm_mpd_connection_error(NcmMpdConnection *connection);
void ncm_mpd_connection_clear_error(NcmMpdConnection *connection);
enum mpd_error ncm_mpd_connection_error_code(
    NcmMpdConnection *connection);
enum mpd_server_error ncm_mpd_connection_server_error_code(
    NcmMpdConnection *connection);
bool ncm_mpd_connection_error_clearable(NcmMpdConnection *connection);
bool ncm_mpd_connection_get_stats(NcmMpdConnection *connection,
                                  NcmMpdStats *stats);
bool ncm_mpd_connection_get_status(NcmMpdConnection *connection,
                                   NcmMpdStatus *status);

int32 ncm_mpd_connection_version(NcmMpdConnection *connection);
bool ncm_mpd_connection_send_password(NcmMpdConnection *connection,
                                      char *password);
bool ncm_mpd_connection_start_command_list(NcmMpdConnection *connection);
bool ncm_mpd_connection_commit_command_list(NcmMpdConnection *connection);
bool ncm_mpd_connection_get_supported_extensions(
    NcmMpdConnection *connection,
    NcmMpdStringList *strings);
bool ncm_mpd_connection_get_replay_gain_mode(
    NcmMpdConnection *connection,
    enum NcmMpdReplayGainMode *mode);
bool ncm_mpd_connection_set_replay_gain_mode(
    NcmMpdConnection *connection,
    enum NcmMpdReplayGainMode mode);
bool ncm_mpd_connection_get_playlists(NcmMpdConnection *connection,
                                      NcmMpdPlaylistList *playlists);
bool ncm_mpd_connection_list_all_song_uris(NcmMpdConnection *connection,
                                           char *path,
                                           NcmMpdStringList *strings);
bool ncm_mpd_connection_get_url_handlers(NcmMpdConnection *connection,
                                         NcmMpdStringList *strings);
bool ncm_mpd_connection_get_tag_types(NcmMpdConnection *connection,
                                      NcmMpdStringList *strings);

void ncm_mpd_song_list_init(NcmMpdSongList *list);
void ncm_mpd_song_list_destroy(NcmMpdSongList *list);
void ncm_mpd_song_list_clear(NcmMpdSongList *list);
int32 ncm_mpd_song_list_count(NcmMpdSongList *list);
NcmSong *ncm_mpd_song_list_at(NcmMpdSongList *list, int32 idx);
bool ncm_mpd_song_list_append_copy(NcmMpdSongList *list,
                                   NcmSong *song);
bool ncm_mpd_song_list_to_song_array(NcmMpdSongList *list,
                                     NcmSongArray *songs);

void ncm_mpd_item_list_init(NcmMpdItemList *list);
void ncm_mpd_item_list_destroy(NcmMpdItemList *list);
void ncm_mpd_item_list_clear(NcmMpdItemList *list);
bool ncm_mpd_item_list_to_item_array(NcmMpdItemList *list,
                                     NcmMpdItemArray *items);
bool ncm_mpd_item_list_to_directory_array(NcmMpdItemList *list,
                                          NcmDirectoryArray *directories);

void ncm_mpd_string_list_init(NcmMpdStringList *list);
void ncm_mpd_string_list_destroy(NcmMpdStringList *list);
void ncm_mpd_string_list_clear(NcmMpdStringList *list);
int32 ncm_mpd_string_list_count(NcmMpdStringList *list);
NcmMpdString *ncm_mpd_string_list_at(NcmMpdStringList *list,
                                     int32 idx);

void ncm_mpd_output_list_init(NcmMpdOutputList *list);
void ncm_mpd_output_list_destroy(NcmMpdOutputList *list);
void ncm_mpd_output_list_clear(NcmMpdOutputList *list);

void ncm_mpd_playlist_list_init(NcmMpdPlaylistList *list);
void ncm_mpd_playlist_list_destroy(NcmMpdPlaylistList *list);
void ncm_mpd_playlist_list_clear(NcmMpdPlaylistList *list);

bool ncm_mpd_connection_get_current_song(NcmMpdConnection *connection,
                                         NcmSong *song);
bool ncm_mpd_connection_get_queue(NcmMpdConnection *connection,
                                  NcmMpdSongList *songs);
bool ncm_mpd_connection_get_queue_changes(NcmMpdConnection *connection,
                                          int32 version,
                                          NcmMpdSongList *songs);
bool ncm_mpd_connection_get_playlist_content(NcmMpdConnection *connection,
                                             char *path,
                                             NcmMpdSongList *songs);
bool ncm_mpd_connection_get_playlist_content_no_info(
    NcmMpdConnection *connection,
    char *path,
    NcmMpdSongList *songs);

bool ncm_mpd_connection_get_directory(NcmMpdConnection *connection,
                                      char *path,
                                      NcmMpdItemList *items);
bool ncm_mpd_connection_get_directory_recursive(
    NcmMpdConnection *connection,
    char *path,
    NcmMpdSongList *songs);
bool ncm_mpd_connection_get_directory_songs(NcmMpdConnection *connection,
                                            char *path,
                                            NcmMpdSongList *songs);
bool ncm_mpd_connection_list_all_songs(NcmMpdConnection *connection,
                                       char *path,
                                       NcmMpdSongList *songs);
bool ncm_mpd_connection_start_search_songs(NcmMpdConnection *connection,
                                           bool exact_match);
bool ncm_mpd_connection_add_search_tag(NcmMpdConnection *connection,
                                       enum mpd_tag_type tag,
                                       char *value);
bool ncm_mpd_connection_add_search_any(NcmMpdConnection *connection,
                                       char *value);
bool ncm_mpd_connection_add_search_uri(NcmMpdConnection *connection,
                                       char *value);
bool ncm_mpd_connection_commit_search_songs(NcmMpdConnection *connection,
                                            NcmMpdSongList *songs);
bool ncm_mpd_connection_list_tag_values(NcmMpdConnection *connection,
                                        enum mpd_tag_type tag,
                                        NcmMpdStringList *strings);

bool ncm_mpd_connection_update_database(NcmMpdConnection *connection,
                                        char *path,
                                        int32 *id);
bool ncm_mpd_connection_get_outputs(NcmMpdConnection *connection,
                                    NcmMpdOutputList *outputs);
bool ncm_mpd_connection_enable_output(NcmMpdConnection *connection,
                                      int32 id);
bool ncm_mpd_connection_disable_output(NcmMpdConnection *connection,
                                       int32 id);

bool ncm_mpd_connection_play(NcmMpdConnection *connection);
bool ncm_mpd_connection_play_pos(NcmMpdConnection *connection, int32 pos);
bool ncm_mpd_connection_play_id(NcmMpdConnection *connection, int32 id);
bool ncm_mpd_connection_toggle_pause(NcmMpdConnection *connection);
bool ncm_mpd_connection_stop(NcmMpdConnection *connection);
bool ncm_mpd_connection_next(NcmMpdConnection *connection);
bool ncm_mpd_connection_previous(NcmMpdConnection *connection);
bool ncm_mpd_connection_seek_pos(NcmMpdConnection *connection,
                                 int32 pos,
                                 int32 seconds);
bool ncm_mpd_connection_set_repeat(NcmMpdConnection *connection, bool mode);
bool ncm_mpd_connection_set_random(NcmMpdConnection *connection, bool mode);
bool ncm_mpd_connection_set_single(NcmMpdConnection *connection, bool mode);
bool ncm_mpd_connection_set_consume(NcmMpdConnection *connection, bool mode);
bool ncm_mpd_connection_set_crossfade(NcmMpdConnection *connection,
                                      int32 seconds);
bool ncm_mpd_connection_set_volume(NcmMpdConnection *connection, int32 vol);
bool ncm_mpd_connection_change_volume(NcmMpdConnection *connection,
                                      int32 change);

bool ncm_mpd_connection_move(NcmMpdConnection *connection,
                             int32 from,
                             int32 to,
                             bool command_list_active);
bool ncm_mpd_connection_swap(NcmMpdConnection *connection,
                             int32 from,
                             int32 to,
                             bool command_list_active);
bool ncm_mpd_connection_shuffle(NcmMpdConnection *connection);
bool ncm_mpd_connection_shuffle_range(NcmMpdConnection *connection,
                                      int32 start,
                                      int32 end);
bool ncm_mpd_connection_clear_queue(NcmMpdConnection *connection);
bool ncm_mpd_connection_set_priority_id(NcmMpdConnection *connection,
                                        int32 id,
                                        int32 prio,
                                        bool command_list_active);
bool ncm_mpd_connection_add_song(NcmMpdConnection *connection,
                                 char *path,
                                 int32 pos,
                                 bool command_list_active,
                                 int32 *id);
bool ncm_mpd_connection_add(NcmMpdConnection *connection,
                            char *path,
                            bool command_list_active,
                            bool *added);
bool ncm_mpd_connection_delete(NcmMpdConnection *connection,
                               int32 pos,
                               bool command_list_active);
bool ncm_mpd_connection_clear_playlist(NcmMpdConnection *connection,
                                       char *playlist);
bool ncm_mpd_connection_add_to_playlist(NcmMpdConnection *connection,
                                        char *playlist,
                                        char *path,
                                        bool command_list_active);
bool ncm_mpd_connection_playlist_move(NcmMpdConnection *connection,
                                      char *playlist,
                                      int32 from,
                                      int32 to,
                                      bool command_list_active);
bool ncm_mpd_connection_playlist_delete(NcmMpdConnection *connection,
                                        char *playlist,
                                        int32 pos,
                                        bool command_list_active);
bool ncm_mpd_connection_rename_playlist(NcmMpdConnection *connection,
                                        char *from,
                                        char *to);
bool ncm_mpd_connection_delete_playlist(NcmMpdConnection *connection,
                                        char *playlist);
bool ncm_mpd_connection_load_playlist(NcmMpdConnection *connection,
                                      char *playlist,
                                      bool *loaded);
bool ncm_mpd_connection_save_playlist(NcmMpdConnection *connection,
                                      char *playlist);

#endif /* NCM_MPD_CONNECTION_H */
