#if !defined(NCMPCPP_STATUS_H)
#define NCMPCPP_STATUS_H

#include <stdbool.h>

#include "c/ncm_error.h"
#include "c/ncm_mpd_client.h"

enum NcmStatusPlayerState {
    NCM_STATUS_PLAYER_UNKNOWN,
    NCM_STATUS_PLAYER_STOP,
    NCM_STATUS_PLAYER_PLAY,
    NCM_STATUS_PLAYER_PAUSE,
};

typedef struct NcmStatusHooks {
    void *user;

    void (*playlist_changed)(uint32 previous_version, void *user);
    void (*stored_playlists_changed)(void *user);
    void (*database_changed)(void *user);
    void (*player_state_changed)(void *user);
    void (*song_id_changed)(int32 song_id, void *user);
    void (*elapsed_time_changed)(bool update_elapsed, void *user);
    void (*flags_changed)(void *user);
    void (*mixer_changed)(void *user);
    void (*outputs_changed)(void *user);
    void (*refresh_footer)(void *user);
    void (*refresh_visible_screens)(void *user);
} NcmStatusHooks;

typedef struct NcmStatusUiHooks {
    void *user;

    void (*playlist_changed)(uint32 previous_version, void *user);
    void (*stored_playlists_changed)(void *user);
    void (*database_changed)(void *user);
    void (*player_state_changed)(enum NcmStatusPlayerState state,
                                 void *user);
    void (*player_stopped)(void *user);
    void (*song_id_changed)(int32 song_id, void *user);
    void (*current_song_changed)(NcmSong *song, void *user);
} NcmStatusUiHooks;

typedef struct NcmStatusInitHooks {
    void *user;

    void (*jump_to_now_playing)(void *user);
    void (*set_tcp_nodelay)(void *user);
    void (*load_browser_supported_extensions)(void *user);
    void (*fetch_outputs)(void *user);
    void (*setup_visualizer_datasource)(void *user);
    void (*register_mpd_fd_callback)(void *user);
    void (*show_connected_message)(void *user);
} NcmStatusInitHooks;

void ncm_status_handle_client_error(NcmMpdClient *client);
void ncm_status_handle_client_error_value(NcmMpdClient *client,
                                          char *message,
                                          int32 message_len,
                                          bool clearable);
void ncm_status_handle_server_error_value(NcmMpdClient *client,
                                          int32 code,
                                          char *message,
                                          int32 message_len);
void ncm_status_trace(NcmMpdClient *client, bool update_timer,
                      bool update_window_timeout, NcmError *error);
void ncm_status_set_database_update_observer(void (*callback)(void *user),
                                             void *user);
void ncm_status_set_playlist_update_observer(void (*callback)(void *user),
                                             void *user);
bool ncm_status_apply_mpd_status(NcmMpdStatus *mpd_status, int32 event,
                                 NcmStatusHooks *hooks, NcmError *error);
bool ncm_status_update(NcmMpdClient *client, int32 event, NcmError *error);
bool ncm_status_initialize_from_mpd_status(NcmMpdStatus *mpd_status,
                                           NcmStatusHooks *hooks,
                                           NcmError *error);
bool ncm_status_initialize_connection(NcmMpdClient *client, NcmError *error);
bool ncm_status_update_full(NcmMpdClient *client, NcmStatusHooks *hooks,
                            NcmError *error);
bool ncm_status_update_from_noidle(NcmMpdClient *client,
                                    NcmStatusHooks *hooks,
                                    NcmError *error);
void ncm_status_clear(void);

bool ncm_status_state_consume(void);
bool ncm_status_state_crossfade(void);
bool ncm_status_state_repeat(void);
bool ncm_status_state_random(void);
bool ncm_status_state_single(void);
int32 ncm_status_state_current_song_position(void);
uint32 ncm_status_state_playlist_length(void);
uint32 ncm_status_state_elapsed_time(void);
enum NcmStatusPlayerState ncm_status_state_player(void);
uint32 ncm_status_state_total_time(void);
int32 ncm_status_state_volume(void);

void ncm_status_changes_playlist(uint32 previous_version);
void ncm_status_changes_stored_playlists(void);
void ncm_status_changes_database(void);
void ncm_status_changes_player_state(void);
void ncm_status_changes_song_id(int32 song_id);
void ncm_status_changes_reset_song_scroll(void);
void ncm_status_changes_elapsed_time(bool update_elapsed);
void ncm_status_changes_flags(void);
void ncm_status_changes_mixer(void);
void ncm_status_changes_outputs(void);

#endif /* NCMPCPP_STATUS_H */
