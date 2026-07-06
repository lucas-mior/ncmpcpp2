#include "c/ncm_display.h"

#include "c/ncm_path.h"
#include "cbase/base_macros.h"

static void
ncm_display_append_basename(NcBuffer *buffer, char *path, int32 path_len) {
    int32 basename;

    basename = ncm_path_basename_start(path, path_len);
    nc_buffer_append_data(buffer, path + basename, path_len - basename);
    return;
}

void
ncm_display_song_row(NcBuffer *buffer, NcmFormatAst *format,
                     NcmSong *song, uint32 flags) {
    ncm_format_render_buffer(format, song, buffer, buffer, flags);
    return;
}

void
ncm_display_directory_row(NcBuffer *buffer, NcmDirectory *directory) {
    NcmStringView path;

    if (!ncm_directory_path_view(directory, &path)) {
        return;
    }

    nc_buffer_append_char(buffer, '[');
    ncm_display_append_basename(buffer, path.data, path.len);
    nc_buffer_append_char(buffer, ']');
    return;
}

void
ncm_display_playlist_row(NcBuffer *buffer, NcmPlaylist *playlist,
                         char *prefix, int32 prefix_len) {
    NcmStringView path;

    if ((prefix != NULL) && (prefix_len > 0)) {
        nc_buffer_append_data(buffer, prefix, prefix_len);
    }
    if (!ncm_playlist_path_view(playlist, &path)) {
        return;
    }
    ncm_display_append_basename(buffer, path.data, path.len);
    return;
}

void
ncm_display_output_row(NcBuffer *buffer, NcmMpdOutput *output) {
    if (output == NULL) {
        return;
    }

    if (output->enabled) {
        nc_buffer_append_data(buffer, STRLIT_ARGS("[x] "));
    } else {
        nc_buffer_append_data(buffer, STRLIT_ARGS("[ ] "));
    }
    nc_buffer_append_data(buffer, output->name, output->name_len);
    return;
}

void
ncm_display_tag_row(NcBuffer *buffer, NcmMutableSong *song,
                    enum NcmTagsField field, int32 idx) {
    NcmBuffer tag;

    tag = ncm_mutable_song_get_tag_buffer(song, field, idx);
    nc_buffer_append_data(buffer, tag.data, tag.len);
    ncm_buffer_destroy(&tag);
    return;
}

void
ncm_display_item_row(NcBuffer *buffer, NcmMpdItem *item,
                     NcmFormatAst *song_format, uint32 flags,
                     char *playlist_prefix,
                     int32 playlist_prefix_len) {
    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        ncm_display_directory_row(buffer, ncm_mpd_item_directory(item));
        break;
    case NCM_MPD_ITEM_SONG:
        ncm_display_song_row(buffer, song_format,
                             ncm_mpd_item_song(item), flags);
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        ncm_display_playlist_row(buffer, ncm_mpd_item_playlist(item),
                                 playlist_prefix, playlist_prefix_len);
        break;
    case NCM_MPD_ITEM_UNKNOWN:
        break;
    }
    return;
}
