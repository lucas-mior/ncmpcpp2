#if !defined(NCMPCPP_STATUS_H)
#define NCMPCPP_STATUS_H

#include <stdbool.h>

#include "c/ncm_error.h"
#include "c/ncm_mpd_client.h"

NCM_EXTERN_C_BEGIN

enum NcmStatusPlayerState {
    NCM_STATUS_PLAYER_UNKNOWN,
    NCM_STATUS_PLAYER_STOP,
    NCM_STATUS_PLAYER_PLAY,
    NCM_STATUS_PLAYER_PAUSE,
};

void ncm_status_handle_client_error(NcmMpdClient *client);
void ncm_status_handle_server_error(NcmMpdClient *client);
void ncm_status_trace(NcmMpdClient *client, bool update_timer,
                      bool update_window_timeout, NcmError *error);
bool ncm_status_update(NcmMpdClient *client, int32 event, NcmError *error);
void ncm_status_clear(void);

bool ncm_status_state_consume(void);
bool ncm_status_state_crossfade(void);
bool ncm_status_state_repeat(void);
bool ncm_status_state_random(void);
bool ncm_status_state_single(void);
int32 ncm_status_state_current_song_id(void);
int32 ncm_status_state_current_song_position(void);
uint32 ncm_status_state_playlist_length(void);
uint32 ncm_status_state_elapsed_time(void);
enum NcmStatusPlayerState ncm_status_state_player(void);
uint32 ncm_status_state_total_time(void);
int32 ncm_status_state_volume(void);
void ncm_status_state_sync_from_legacy(enum NcmStatusPlayerState player,
                                       uint32 elapsed_time,
                                       uint32 total_time);

void ncm_status_changes_playlist(uint32 previous_version);
void ncm_status_changes_stored_playlists(void);
void ncm_status_changes_database(void);
void ncm_status_changes_player_state(void);
void ncm_status_changes_song_id(int32 song_id);
void ncm_status_changes_elapsed_time(bool update_elapsed);
void ncm_status_changes_flags(void);
void ncm_status_changes_mixer(void);
void ncm_status_changes_outputs(void);

NCM_EXTERN_C_END

#endif /* NCMPCPP_STATUS_H */
