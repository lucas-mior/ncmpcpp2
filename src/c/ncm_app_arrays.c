#if !defined(NCM_APP_ARRAYS_C)
#define NCM_APP_ARRAYS_C

#include "c/ncm_app_arrays.h"

static void ncm_app_array_song_init(void *item);
static void ncm_app_array_song_destroy(void *item);
static bool ncm_app_array_song_copy(void *dest, void *source);
static void ncm_app_array_song_move(void *dest, void *source);
static void ncm_app_array_directory_init(void *item);
static void ncm_app_array_directory_destroy(void *item);
static bool ncm_app_array_directory_copy(void *dest, void *source);
static void ncm_app_array_playlist_init(void *item);
static void ncm_app_array_playlist_destroy(void *item);
static bool ncm_app_array_playlist_copy(void *dest, void *source);
static void ncm_app_array_mpd_item_init(void *item);
static void ncm_app_array_mpd_item_destroy(void *item);
static bool ncm_app_array_mpd_item_copy(void *dest, void *source);

static NcmArrayItemCallbacks ncm_app_array_no_callbacks = {0};
static NcmArrayItemCallbacks ncm_app_array_song_callbacks = {
    .init = ncm_app_array_song_init,
    .destroy = ncm_app_array_song_destroy,
    .copy = ncm_app_array_song_copy,
    .move = ncm_app_array_song_move,
};
static NcmArrayItemCallbacks ncm_app_array_directory_callbacks = {
    .init = ncm_app_array_directory_init,
    .destroy = ncm_app_array_directory_destroy,
    .copy = ncm_app_array_directory_copy,
};
static NcmArrayItemCallbacks ncm_app_array_playlist_callbacks = {
    .init = ncm_app_array_playlist_init,
    .destroy = ncm_app_array_playlist_destroy,
    .copy = ncm_app_array_playlist_copy,
};
static NcmArrayItemCallbacks ncm_app_array_mpd_item_callbacks = {
    .init = ncm_app_array_mpd_item_init,
    .destroy = ncm_app_array_mpd_item_destroy,
    .copy = ncm_app_array_mpd_item_copy,
};

static void
ncm_app_array_song_init(void *item) {
    ncm_song_init(item);
    return;
}

static void
ncm_app_array_song_destroy(void *item) {
    ncm_song_destroy(item);
    return;
}

static bool
ncm_app_array_song_copy(void *dest, void *source) {
    return ncm_song_copy(dest, source);
}

static void
ncm_app_array_song_move(void *dest, void *source) {
    ncm_song_move(dest, source);
    return;
}

static void
ncm_app_array_directory_init(void *item) {
    ncm_directory_init(item);
    return;
}

static void
ncm_app_array_directory_destroy(void *item) {
    ncm_directory_destroy(item);
    return;
}

static bool
ncm_app_array_directory_copy(void *dest, void *source) {
    return ncm_directory_copy(dest, source);
}

static void
ncm_app_array_playlist_init(void *item) {
    ncm_playlist_init(item);
    return;
}

static void
ncm_app_array_playlist_destroy(void *item) {
    ncm_playlist_destroy(item);
    return;
}

static bool
ncm_app_array_playlist_copy(void *dest, void *source) {
    return ncm_playlist_copy(dest, source);
}

static void
ncm_app_array_mpd_item_init(void *item) {
    ncm_mpd_item_init(item);
    return;
}

static void
ncm_app_array_mpd_item_destroy(void *item) {
    ncm_mpd_item_destroy(item);
    return;
}

static bool
ncm_app_array_mpd_item_copy(void *dest, void *source) {
    return ncm_mpd_item_copy(dest, source);
}

NCM_ARRAY_DEFINE_INIT(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DEFINE_CLEAR(ncm_string_view_array,
                       NcmStringViewArray,
                       &ncm_app_array_no_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_string_view_array, NcmStringViewArray)
NCM_ARRAY_DEFINE_APPEND(ncm_string_view_array,
                        NcmStringViewArray,
                        NcmStringView,
                        &ncm_app_array_no_callbacks)

void
ncm_buffer_array_init(NcmBufferArray *array) {
    str_builder_array_init(array);
    return;
}

void
ncm_buffer_array_clear(NcmBufferArray *array) {
    str_builder_array_clear(array);
    return;
}

void
ncm_buffer_array_destroy(NcmBufferArray *array) {
    str_builder_array_destroy(array);
    return;
}

bool
ncm_buffer_array_copy(NcmBufferArray *dest, NcmBufferArray *source) {
    return str_builder_array_copy(dest, source);
}

void
ncm_buffer_array_move(NcmBufferArray *dest, NcmBufferArray *source) {
    str_builder_array_move(dest, source);
    return;
}

void
ncm_buffer_array_swap(NcmBufferArray *left, NcmBufferArray *right) {
    str_builder_array_swap(left, right);
    return;
}

bool
ncm_buffer_array_reserve(NcmBufferArray *array, int32 extra) {
    return str_builder_array_reserve(array, extra);
}

NcmBuffer *
ncm_buffer_array_append(NcmBufferArray *array) {
    return str_builder_array_append(array);
}

bool
ncm_buffer_array_append_copy(NcmBufferArray *array, NcmBuffer *item) {
    return str_builder_array_append_copy(array, item);
}

NCM_ARRAY_DEFINE_INIT(ncm_song_array, NcmSongArray)
NCM_ARRAY_DEFINE_CLEAR(ncm_song_array,
                       NcmSongArray,
                       &ncm_app_array_song_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_song_array, NcmSongArray)
NCM_ARRAY_DEFINE_COPY(ncm_song_array, NcmSongArray)
NCM_ARRAY_DEFINE_MOVE(ncm_song_array, NcmSongArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_song_array, NcmSongArray)
NCM_ARRAY_DEFINE_APPEND(ncm_song_array,
                        NcmSongArray,
                        NcmSong,
                        &ncm_app_array_song_callbacks)
NCM_ARRAY_DEFINE_APPEND_COPY(ncm_song_array,
                             NcmSongArray,
                             NcmSong,
                             &ncm_app_array_song_callbacks)
NCM_ARRAY_DEFINE_APPEND_MOVE(ncm_song_array,
                             NcmSongArray,
                             NcmSong,
                             &ncm_app_array_song_callbacks)

NCM_ARRAY_DEFINE_INIT(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DEFINE_CLEAR(ncm_directory_array,
                       NcmDirectoryArray,
                       &ncm_app_array_directory_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DEFINE_COPY(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DEFINE_MOVE(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_directory_array, NcmDirectoryArray)
NCM_ARRAY_DEFINE_APPEND(ncm_directory_array,
                        NcmDirectoryArray,
                        NcmDirectory,
                        &ncm_app_array_directory_callbacks)
NCM_ARRAY_DEFINE_APPEND_COPY(ncm_directory_array,
                             NcmDirectoryArray,
                             NcmDirectory,
                             &ncm_app_array_directory_callbacks)

NCM_ARRAY_DEFINE_INIT(ncm_playlist_array, NcmPlaylistArray)
NCM_ARRAY_DEFINE_CLEAR(ncm_playlist_array,
                       NcmPlaylistArray,
                       &ncm_app_array_playlist_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_playlist_array, NcmPlaylistArray)
NCM_ARRAY_DEFINE_MOVE(ncm_playlist_array, NcmPlaylistArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_playlist_array, NcmPlaylistArray)
NCM_ARRAY_DEFINE_APPEND(ncm_playlist_array,
                        NcmPlaylistArray,
                        NcmPlaylist,
                        &ncm_app_array_playlist_callbacks)
NCM_ARRAY_DEFINE_APPEND_COPY(ncm_playlist_array,
                             NcmPlaylistArray,
                             NcmPlaylist,
                             &ncm_app_array_playlist_callbacks)

NCM_ARRAY_DEFINE_INIT(ncm_mpd_item_array, NcmMpdItemArray)
NCM_ARRAY_DEFINE_CLEAR(ncm_mpd_item_array,
                       NcmMpdItemArray,
                       &ncm_app_array_mpd_item_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_mpd_item_array, NcmMpdItemArray)
NCM_ARRAY_DEFINE_MOVE(ncm_mpd_item_array, NcmMpdItemArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_mpd_item_array, NcmMpdItemArray)
NCM_ARRAY_DEFINE_APPEND(ncm_mpd_item_array,
                        NcmMpdItemArray,
                        NcmMpdItem,
                        &ncm_app_array_mpd_item_callbacks)
NCM_ARRAY_DEFINE_APPEND_COPY(ncm_mpd_item_array,
                             NcmMpdItemArray,
                             NcmMpdItem,
                             &ncm_app_array_mpd_item_callbacks)

#endif /* NCM_APP_ARRAYS_C */
