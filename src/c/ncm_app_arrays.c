#include "c/ncm_app_arrays.h"

static void ncm_app_array_buffer_init(void *item);
static void ncm_app_array_buffer_destroy(void *item);
static bool ncm_app_array_buffer_copy(void *dest, void *source);
static void ncm_app_array_buffer_move(void *dest, void *source);
static void ncm_app_array_song_init(void *item);
static void ncm_app_array_song_destroy(void *item);
static bool ncm_app_array_song_copy(void *dest, void *source);
static void ncm_app_array_song_move(void *dest, void *source);
static void ncm_app_array_mutable_song_init(void *item);
static void ncm_app_array_mutable_song_destroy(void *item);
static bool ncm_app_array_mutable_song_copy(void *dest, void *source);
static void ncm_app_array_mutable_song_move(void *dest, void *source);
static void ncm_app_array_directory_init(void *item);
static void ncm_app_array_directory_destroy(void *item);
static bool ncm_app_array_directory_copy(void *dest, void *source);
static void ncm_app_array_directory_move(void *dest, void *source);
static void ncm_app_array_playlist_init(void *item);
static void ncm_app_array_playlist_destroy(void *item);
static bool ncm_app_array_playlist_copy(void *dest, void *source);
static void ncm_app_array_playlist_move(void *dest, void *source);
static void ncm_app_array_mpd_item_init(void *item);
static void ncm_app_array_mpd_item_destroy(void *item);
static bool ncm_app_array_mpd_item_copy(void *dest, void *source);
static void ncm_app_array_mpd_item_move(void *dest, void *source);
static void ncm_app_array_sample_buffer_init(void *item);
static void ncm_app_array_sample_buffer_destroy(void *item);
static bool ncm_app_array_sample_buffer_copy(void *dest, void *source);
static void ncm_app_array_sample_buffer_move(void *dest, void *source);

static NcmArrayItemCallbacks ncm_app_array_no_callbacks = {0};
static NcmArrayItemCallbacks ncm_app_array_buffer_callbacks = {
    .init = ncm_app_array_buffer_init,
    .destroy = ncm_app_array_buffer_destroy,
    .copy = ncm_app_array_buffer_copy,
    .move = ncm_app_array_buffer_move,
};
static NcmArrayItemCallbacks ncm_app_array_song_callbacks = {
    .init = ncm_app_array_song_init,
    .destroy = ncm_app_array_song_destroy,
    .copy = ncm_app_array_song_copy,
    .move = ncm_app_array_song_move,
};
static NcmArrayItemCallbacks ncm_app_array_mutable_song_callbacks = {
    .init = ncm_app_array_mutable_song_init,
    .destroy = ncm_app_array_mutable_song_destroy,
    .copy = ncm_app_array_mutable_song_copy,
    .move = ncm_app_array_mutable_song_move,
};
static NcmArrayItemCallbacks ncm_app_array_directory_callbacks = {
    .init = ncm_app_array_directory_init,
    .destroy = ncm_app_array_directory_destroy,
    .copy = ncm_app_array_directory_copy,
    .move = ncm_app_array_directory_move,
};
static NcmArrayItemCallbacks ncm_app_array_playlist_callbacks = {
    .init = ncm_app_array_playlist_init,
    .destroy = ncm_app_array_playlist_destroy,
    .copy = ncm_app_array_playlist_copy,
    .move = ncm_app_array_playlist_move,
};
static NcmArrayItemCallbacks ncm_app_array_mpd_item_callbacks = {
    .init = ncm_app_array_mpd_item_init,
    .destroy = ncm_app_array_mpd_item_destroy,
    .copy = ncm_app_array_mpd_item_copy,
    .move = ncm_app_array_mpd_item_move,
};
static NcmArrayItemCallbacks ncm_app_array_sample_buffer_callbacks = {
    .init = ncm_app_array_sample_buffer_init,
    .destroy = ncm_app_array_sample_buffer_destroy,
    .copy = ncm_app_array_sample_buffer_copy,
    .move = ncm_app_array_sample_buffer_move,
};

static void
ncm_app_array_buffer_init(void *item) {
    ncm_buffer_init(item);
    return;
}

static void
ncm_app_array_buffer_destroy(void *item) {
    ncm_buffer_destroy(item);
    return;
}

static bool
ncm_app_array_buffer_copy(void *dest, void *source) {
    NcmBuffer *dest_buffer;
    NcmBuffer *source_buffer;

    dest_buffer = dest;
    source_buffer = source;
    ncm_buffer_clear(dest_buffer);
    ncm_buffer_append(dest_buffer, source_buffer->data, source_buffer->len);
    return true;
}

static void
ncm_app_array_buffer_move(void *dest, void *source) {
    NcmBuffer *dest_buffer;
    NcmBuffer *source_buffer;

    dest_buffer = dest;
    source_buffer = source;
    ncm_buffer_destroy(dest_buffer);
    *dest_buffer = *source_buffer;
    ncm_buffer_init(source_buffer);
    return;
}

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
ncm_app_array_mutable_song_init(void *item) {
    ncm_mutable_song_init(item);
    return;
}

static void
ncm_app_array_mutable_song_destroy(void *item) {
    ncm_mutable_song_destroy(item);
    return;
}

static bool
ncm_app_array_mutable_song_copy(void *dest, void *source) {
    return ncm_mutable_song_copy(dest, source);
}

static void
ncm_app_array_mutable_song_move(void *dest, void *source) {
    ncm_mutable_song_move(dest, source);
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
ncm_app_array_directory_move(void *dest, void *source) {
    ncm_directory_move(dest, source);
    return;
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
ncm_app_array_playlist_move(void *dest, void *source) {
    ncm_playlist_move(dest, source);
    return;
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

static void
ncm_app_array_mpd_item_move(void *dest, void *source) {
    ncm_mpd_item_move(dest, source);
    return;
}

static void
ncm_app_array_sample_buffer_init(void *item) {
    ncm_sample_buffer_init(item);
    return;
}

static void
ncm_app_array_sample_buffer_destroy(void *item) {
    ncm_sample_buffer_destroy(item);
    return;
}

static bool
ncm_app_array_sample_buffer_copy(void *dest, void *source) {
    ncm_sample_buffer_copy(dest, source);
    return true;
}

static void
ncm_app_array_sample_buffer_move(void *dest, void *source) {
    ncm_sample_buffer_move(dest, source);
    return;
}

NCM_ARRAY_DEFINE(ncm_string_view_array,
                 NcmStringViewArray,
                 NcmStringView,
                 &ncm_app_array_no_callbacks)
NCM_ARRAY_DEFINE(ncm_buffer_array,
                 NcmBufferArray,
                 NcmBuffer,
                 &ncm_app_array_buffer_callbacks)
NCM_ARRAY_DEFINE(ncm_song_array,
                 NcmSongArray,
                 NcmSong,
                 &ncm_app_array_song_callbacks)
NCM_ARRAY_DEFINE(ncm_mutable_song_array,
                 NcmMutableSongArray,
                 NcmMutableSong,
                 &ncm_app_array_mutable_song_callbacks)
NCM_ARRAY_DEFINE(ncm_directory_array,
                 NcmDirectoryArray,
                 NcmDirectory,
                 &ncm_app_array_directory_callbacks)
NCM_ARRAY_DEFINE(ncm_playlist_array,
                 NcmPlaylistArray,
                 NcmPlaylist,
                 &ncm_app_array_playlist_callbacks)
NCM_ARRAY_DEFINE(ncm_mpd_item_array,
                 NcmMpdItemArray,
                 NcmMpdItem,
                 &ncm_app_array_mpd_item_callbacks)
NCM_ARRAY_DEFINE(ncm_sample_buffer_array,
                 NcmSampleBufferArray,
                 NcmSampleBuffer,
                 &ncm_app_array_sample_buffer_callbacks)
