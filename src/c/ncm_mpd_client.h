#if !defined(NCM_MPD_CLIENT_H)
#define NCM_MPD_CLIENT_H

#include "c/ncm_base.h"
#include "c/ncm_mpd_connection.h"
#include "c/ncm_random.h"

typedef void NcmMpdNoidleCallback(int32 flags, void *user);

typedef struct NcmMpdClient {
    NcmMpdConnection connection;
    NcmBuffer host;
    NcmBuffer password;
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
bool ncm_mpd_client_connected(NcmMpdClient *client);
int32 ncm_mpd_client_version(NcmMpdClient *client);
int32 ncm_mpd_client_fd(NcmMpdClient *client);
void ncm_mpd_client_set_noidle_callback(NcmMpdClient *client,
                                        NcmMpdNoidleCallback *callback,
                                        void *user);
bool ncm_mpd_client_set_hostname(NcmMpdClient *client, char *host,
                                 int32 host_len, NcmError *error);
void ncm_mpd_client_set_port(NcmMpdClient *client, uint16 port);
bool ncm_mpd_client_set_password(NcmMpdClient *client, char *password,
                                 int32 password_len, NcmError *error);
bool ncm_mpd_client_set_timeout_ms(NcmMpdClient *client,
                                   int32 timeout_ms,
                                   NcmError *error);
bool ncm_mpd_client_connect(NcmMpdClient *client, NcmError *error);
void ncm_mpd_client_disconnect(NcmMpdClient *client);
bool ncm_mpd_client_send_password(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_idle(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_noidle(NcmMpdClient *client, int32 *flags,
                           NcmError *error);

enum mpd_error ncm_mpd_client_error_code(NcmMpdClient *client);
enum mpd_server_error ncm_mpd_client_server_error_code(
    NcmMpdClient *client);
bool ncm_mpd_client_error_clearable(NcmMpdClient *client);
char *ncm_mpd_client_error_message(NcmMpdClient *client);

bool ncm_mpd_client_get_stats(NcmMpdClient *client, NcmMpdStats *stats,
                              NcmError *error);
bool ncm_mpd_client_get_status(NcmMpdClient *client, NcmMpdStatus *status,
                               NcmError *error);
bool ncm_mpd_client_update_directory(NcmMpdClient *client, char *path,
                                     int32 *id, NcmError *error);

bool ncm_mpd_client_play(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_play_pos(NcmMpdClient *client, int32 pos,
                             NcmError *error);
bool ncm_mpd_client_play_id(NcmMpdClient *client, int32 id,
                            NcmError *error);
bool ncm_mpd_client_toggle_pause(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_stop(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_next(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_previous(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_move(NcmMpdClient *client, int32 from, int32 to,
                         NcmError *error);
bool ncm_mpd_client_swap(NcmMpdClient *client, int32 from, int32 to,
                         NcmError *error);
bool ncm_mpd_client_seek_pos(NcmMpdClient *client, int32 pos,
                             int32 seconds, NcmError *error);
bool ncm_mpd_client_shuffle(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_shuffle_range(NcmMpdClient *client, int32 start,
                                  int32 end, NcmError *error);
bool ncm_mpd_client_clear_queue(NcmMpdClient *client, NcmError *error);

bool ncm_mpd_client_get_queue(NcmMpdClient *client,
                              NcmMpdSongList *songs,
                              NcmError *error);
bool ncm_mpd_client_get_queue_changes(NcmMpdClient *client, int32 version,
                                      NcmMpdSongList *songs,
                                      NcmError *error);
bool ncm_mpd_client_get_current_song(NcmMpdClient *client, NcmSong *song,
                                     NcmError *error);
bool ncm_mpd_client_get_playlist_content(NcmMpdClient *client,
                                         char *path,
                                         NcmMpdSongList *songs,
                                         NcmError *error);
bool ncm_mpd_client_get_playlist_content_no_info(NcmMpdClient *client,
                                                 char *path,
                                                 NcmMpdSongList *songs,
                                                 NcmError *error);
bool ncm_mpd_client_get_supported_extensions(NcmMpdClient *client,
                                             NcmMpdStringList *strings,
                                             NcmError *error);

bool ncm_mpd_client_set_repeat(NcmMpdClient *client, bool mode,
                               NcmError *error);
bool ncm_mpd_client_set_random(NcmMpdClient *client, bool mode,
                               NcmError *error);
bool ncm_mpd_client_set_single(NcmMpdClient *client, bool mode,
                               NcmError *error);
bool ncm_mpd_client_set_consume(NcmMpdClient *client, bool mode,
                                NcmError *error);
bool ncm_mpd_client_set_crossfade(NcmMpdClient *client, int32 seconds,
                                  NcmError *error);
bool ncm_mpd_client_set_volume(NcmMpdClient *client, int32 volume,
                               NcmError *error);
bool ncm_mpd_client_change_volume(NcmMpdClient *client, int32 change,
                                  NcmError *error);
bool ncm_mpd_client_get_replay_gain_mode(
    NcmMpdClient *client,
    enum NcmMpdReplayGainMode *mode,
    NcmError *error);
bool ncm_mpd_client_set_replay_gain_mode(
    NcmMpdClient *client,
    enum NcmMpdReplayGainMode mode,
    NcmError *error);

bool ncm_mpd_client_set_priority_id(NcmMpdClient *client, int32 id,
                                    int32 priority, NcmError *error);
bool ncm_mpd_client_set_priority_song(NcmMpdClient *client,
                                      NcmSong *song, int32 priority,
                                      NcmError *error);
bool ncm_mpd_client_add_song(NcmMpdClient *client, char *path, int32 pos,
                             int32 *id, NcmError *error);
bool ncm_mpd_client_add_song_value(NcmMpdClient *client, NcmSong *song,
                                   int32 pos, int32 *id,
                                   NcmError *error);
bool ncm_mpd_client_add_song_list(NcmMpdClient *client,
                                  NcmMpdSongList *songs, int32 pos,
                                  NcmError *error);
bool ncm_mpd_client_add(NcmMpdClient *client, char *path, bool *added,
                        NcmError *error);
bool ncm_mpd_client_add_random_tag(NcmMpdClient *client,
                                   enum mpd_tag_type tag,
                                   int32 number,
                                   NcmRandom *random,
                                   NcmError *error);
bool ncm_mpd_client_add_random_songs(NcmMpdClient *client,
                                     int32 number,
                                     char *exclude_pattern,
                                     int32 exclude_pattern_len,
                                     NcmRandom *random,
                                     NcmError *error);
bool ncm_mpd_client_delete(NcmMpdClient *client, int32 pos, NcmError *error);
bool ncm_mpd_client_start_command_list(NcmMpdClient *client, NcmError *error);
bool ncm_mpd_client_commit_command_list(NcmMpdClient *client, NcmError *error);

bool ncm_mpd_client_delete_playlist(NcmMpdClient *client, char *name,
                                    NcmError *error);
bool ncm_mpd_client_load_playlist(NcmMpdClient *client, char *name,
                                  bool *loaded, NcmError *error);
bool ncm_mpd_client_save_playlist(NcmMpdClient *client, char *name,
                                  NcmError *error);
bool ncm_mpd_client_clear_playlist(NcmMpdClient *client, char *name,
                                   NcmError *error);
bool ncm_mpd_client_add_to_playlist(NcmMpdClient *client, char *playlist,
                                    char *path, NcmError *error);
bool ncm_mpd_client_add_song_to_playlist(NcmMpdClient *client,
                                         char *playlist, NcmSong *song,
                                         NcmError *error);
bool ncm_mpd_client_playlist_move(NcmMpdClient *client, char *playlist,
                                  int32 from, int32 to,
                                  NcmError *error);
bool ncm_mpd_client_playlist_delete(NcmMpdClient *client, char *playlist,
                                    int32 pos, NcmError *error);
bool ncm_mpd_client_rename_playlist(NcmMpdClient *client, char *from,
                                    char *to, NcmError *error);

bool ncm_mpd_client_start_search(NcmMpdClient *client, bool exact_match,
                                 NcmError *error);
bool ncm_mpd_client_add_search_tag(NcmMpdClient *client,
                                   enum mpd_tag_type tag,
                                   char *value, NcmError *error);
bool ncm_mpd_client_add_search_any(NcmMpdClient *client, char *value,
                                   NcmError *error);
bool ncm_mpd_client_add_search_uri(NcmMpdClient *client, char *value,
                                   NcmError *error);
bool ncm_mpd_client_commit_search_songs(NcmMpdClient *client,
                                        NcmMpdSongList *songs,
                                        NcmError *error);

bool ncm_mpd_client_get_playlists(NcmMpdClient *client,
                                  NcmMpdPlaylistList *playlists,
                                  NcmError *error);
bool ncm_mpd_client_get_list(NcmMpdClient *client, enum mpd_tag_type tag,
                             NcmMpdStringList *strings, NcmError *error);
bool ncm_mpd_client_get_directory(NcmMpdClient *client, char *path,
                                  NcmMpdItemList *items, NcmError *error);
bool ncm_mpd_client_get_directory_recursive(NcmMpdClient *client,
                                            char *path,
                                            NcmMpdSongList *songs,
                                            NcmError *error);
bool ncm_mpd_client_get_songs(NcmMpdClient *client, char *path,
                              NcmMpdSongList *songs, NcmError *error);
bool ncm_mpd_client_get_directory_entries(NcmMpdClient *client,
                                          char *path,
                                          NcmMpdItemArray *items,
                                          NcmError *error);
bool ncm_mpd_client_get_directory_list(NcmMpdClient *client, char *path,
                                       NcmDirectoryArray *directories,
                                       NcmError *error);
bool ncm_mpd_client_get_outputs(NcmMpdClient *client,
                                NcmMpdOutputList *outputs,
                                NcmError *error);
bool ncm_mpd_client_enable_output(NcmMpdClient *client, int32 id,
                                  NcmError *error);
bool ncm_mpd_client_disable_output(NcmMpdClient *client, int32 id,
                                   NcmError *error);
bool ncm_mpd_client_get_url_handlers(NcmMpdClient *client,
                                     NcmMpdStringList *strings,
                                     NcmError *error);
bool ncm_mpd_client_get_tag_types(NcmMpdClient *client,
                                  NcmMpdStringList *strings,
                                  NcmError *error);

#endif /* NCM_MPD_CLIENT_H */
