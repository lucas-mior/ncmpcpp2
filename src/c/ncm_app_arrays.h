#if !defined(NCM_APP_ARRAYS_H)
#define NCM_APP_ARRAYS_H

#include "c/ncm_array.h"
#include "c/ncm_directory.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_mutable_song.h"
#include "c/ncm_playlist.h"
#include "c/ncm_sample_buffer.h"
#include "c/ncm_song.h"

NCM_ARRAY_DECLARE_TYPE(NcmStringViewArray, NcmStringView)
NCM_ARRAY_DECLARE_INIT(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DECLARE_CLEAR(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DECLARE_APPEND(ncm_string_view_array,
                         NcmStringViewArray,
                         NcmStringView)

typedef StrBuilderArray NcmBufferArray;
NCM_ARRAY_DECLARE_INIT(ncm_buffer_array, NcmBufferArray)
NCM_ARRAY_DECLARE_CLEAR(ncm_buffer_array, NcmBufferArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_buffer_array, NcmBufferArray)
NCM_ARRAY_DECLARE_COPY(ncm_buffer_array, NcmBufferArray)
NCM_ARRAY_DECLARE_MOVE(ncm_buffer_array, NcmBufferArray)
NCM_ARRAY_DECLARE_SWAP(ncm_buffer_array, NcmBufferArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_buffer_array, NcmBufferArray)
NCM_ARRAY_DECLARE_APPEND(ncm_buffer_array, NcmBufferArray, NcmBuffer)
NCM_ARRAY_DECLARE_APPEND_COPY(ncm_buffer_array,
                              NcmBufferArray,
                              NcmBuffer)

NCM_ARRAY_DECLARE_TYPE(NcmSongArray, NcmSong)
NCM_ARRAY_DECLARE_INIT(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_CLEAR(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_COPY(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_MOVE(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_song_array, NcmSongArray)
NCM_ARRAY_DECLARE_APPEND(ncm_song_array, NcmSongArray, NcmSong)
NCM_ARRAY_DECLARE_APPEND_COPY(ncm_song_array, NcmSongArray, NcmSong)
NCM_ARRAY_DECLARE_APPEND_MOVE(ncm_song_array, NcmSongArray, NcmSong)

NCM_ARRAY_DECLARE_TYPE(NcmDirectoryArray, NcmDirectory)
NCM_ARRAY_DECLARE_INIT(ncm_directory_array, NcmDirectoryArray)
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
NCM_ARRAY_DECLARE_INIT(ncm_playlist_array, NcmPlaylistArray)
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
NCM_ARRAY_DECLARE_INIT(ncm_mpd_item_array, NcmMpdItemArray)
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

#endif /* NCM_APP_ARRAYS_H */
