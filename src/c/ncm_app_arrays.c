#if !defined(NCM_APP_ARRAYS_C)
#define NCM_APP_ARRAYS_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

#define NCM_ARRAY_TYPE NcmSongArray
#define NCM_ARRAY_ITEM_TYPE NcmSong
#define NCM_ARRAY_PREFIX ncm_song_array
#define NCM_ARRAY_ITEM_DESTROY ncm_song_destroy
#define NCM_ARRAY_ITEM_COPY ncm_song_copy
#define NCM_ARRAY_ITEM_MOVE ncm_song_move
#define NCM_ARRAY_COPY
#define NCM_ARRAY_MOVE
#define NCM_ARRAY_APPEND_COPY
#define NCM_ARRAY_APPEND_MOVE
#include "c/ncm_array_impl_template.h"

#define NCM_ARRAY_TYPE NcmDirectoryArray
#define NCM_ARRAY_ITEM_TYPE NcmDirectory
#define NCM_ARRAY_PREFIX ncm_directory_array
#define NCM_ARRAY_ITEM_DESTROY ncm_directory_destroy
#define NCM_ARRAY_ITEM_COPY ncm_directory_copy
#define NCM_ARRAY_COPY
#define NCM_ARRAY_MOVE
#define NCM_ARRAY_APPEND_COPY
#define NCM_ARRAY_APPEND_MOVE
#include "c/ncm_array_impl_template.h"

#define NCM_ARRAY_TYPE NcmPlaylistArray
#define NCM_ARRAY_ITEM_TYPE NcmPlaylist
#define NCM_ARRAY_PREFIX ncm_playlist_array
#define NCM_ARRAY_ITEM_DESTROY ncm_playlist_destroy
#define NCM_ARRAY_ITEM_COPY ncm_playlist_copy
#define NCM_ARRAY_ITEM_MOVE ncm_playlist_move
#define NCM_ARRAY_MOVE
#define NCM_ARRAY_APPEND_COPY
#define NCM_ARRAY_APPEND_MOVE
#include "c/ncm_array_impl_template.h"

#define NCM_ARRAY_TYPE NcmMpdItemArray
#define NCM_ARRAY_ITEM_TYPE NcmMpdItem
#define NCM_ARRAY_PREFIX ncm_mpd_item_array
#define NCM_ARRAY_ITEM_INIT ncm_mpd_item_init
#define NCM_ARRAY_ITEM_DESTROY ncm_mpd_item_destroy
#define NCM_ARRAY_ITEM_COPY ncm_mpd_item_copy
#define NCM_ARRAY_ITEM_MOVE ncm_mpd_item_move
#define NCM_ARRAY_MOVE
#define NCM_ARRAY_APPEND_COPY
#define NCM_ARRAY_APPEND_MOVE
#include "c/ncm_array_impl_template.h"

#endif /* NCM_APP_ARRAYS_C */
