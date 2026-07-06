#if !defined(NCM_APP_ARRAYS_H)
#define NCM_APP_ARRAYS_H

#include "c/ncm_array.h"
#include "c/ncm_directory.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_mutable_song.h"
#include "c/ncm_playlist.h"
#include "c/ncm_sample_buffer.h"
#include "c/ncm_song.h"

#if defined(__cplusplus)
extern "C" {
#endif

NCM_ARRAY_DECLARE(ncm_string_view_array,
                  NcmStringViewArray,
                  NcmStringView)
NCM_ARRAY_DECLARE(ncm_buffer_array,
                  NcmBufferArray,
                  NcmBuffer)
NCM_ARRAY_DECLARE(ncm_song_array,
                  NcmSongArray,
                  NcmSong)
NCM_ARRAY_DECLARE(ncm_mutable_song_array,
                  NcmMutableSongArray,
                  NcmMutableSong)
NCM_ARRAY_DECLARE(ncm_directory_array,
                  NcmDirectoryArray,
                  NcmDirectory)
NCM_ARRAY_DECLARE(ncm_playlist_array,
                  NcmPlaylistArray,
                  NcmPlaylist)
NCM_ARRAY_DECLARE(ncm_mpd_item_array,
                  NcmMpdItemArray,
                  NcmMpdItem)
NCM_ARRAY_DECLARE(ncm_sample_buffer_array,
                  NcmSampleBufferArray,
                  NcmSampleBuffer)

#if defined(__cplusplus)
}
#endif

#endif /* NCM_APP_ARRAYS_H */
