#include "c/ncm_display.h"

#include "c/ncm_base.h"
#include "c/ncm_path.h"
#include "c/ncm_string.h"
#include "c/ncm_utf8.h"
#include "cbase/base_macros.h"
#include "settings.h"

static void ncm_display_append_basename(NcBuffer *buffer, char *path,
                                        int32 path_len);
static int32 ncm_display_column_width(Column *column, int32 list_width,
                                      int32 remained_width);
static NcmStringView ncm_display_column_type_name(char type);
static void ncm_display_append_column_name(NcmBuffer *buffer,
                                           Column *column);
static void ncm_display_append_spaces(NcmBuffer *buffer, int32 count);

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
ncm_display_column_title(NcmBuffer *buffer, struct Column *columns,
                         int32 column_count, int32 list_width) {
    NcmBuffer name;
    Column *last;
    int32 remained_width;

    if (buffer == NULL) {
        return;
    }

    ncm_buffer_clear(buffer);
    if ((columns == NULL) || (column_count <= 0) || (list_width <= 0)) {
        return;
    }

    remained_width = list_width;
    last = &columns[column_count - 1];
    ncm_buffer_init(&name);
    for (int32 i = 0; i < column_count; i += 1) {
        Column *column;
        int32 cut_len;
        int32 name_width;
        int32 padding;
        int32 width;

        column = &columns[i];
        width = ncm_display_column_width(column, list_width,
                                         remained_width);
        if (width == 0) {
            continue;
        }
        if (column != last) {
            width -= 1;
        }
        if (((remained_width - width) < 0) || (width < 0)) {
            break;
        }

        ncm_buffer_clear(&name);
        ncm_display_append_column_name(&name, column);
        cut_len = ncm_utf8_cut_width(name.data, name.len, width);
        name_width = ncm_utf8_width(name.data, cut_len);
        padding = width - name_width;
        if (padding < 0) {
            padding = 0;
        }
        if (column->right_alignment) {
            ncm_display_append_spaces(buffer, padding);
            ncm_buffer_append(buffer, name.data, cut_len);
        } else {
            ncm_buffer_append(buffer, name.data, cut_len);
            ncm_display_append_spaces(buffer, padding);
        }

        if (column != last) {
            remained_width -= width + 1;
            ncm_buffer_append_byte(buffer, ' ');
        }
    }
    ncm_buffer_destroy(&name);
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

static int32
ncm_display_column_width(Column *column, int32 list_width,
                         int32 remained_width) {
    int32 width;

    if (column->stretch_limit >= 0) {
        width = remained_width - column->stretch_limit;
    } else if (column->fixed) {
        width = column->width;
    } else {
        width = (int32)((int64)column->width*list_width/100);
    }
    return width;
}

static NcmStringView
ncm_display_column_type_name(char type) {
    switch (type) {
    case 'l':
        return ncm_string_view_make(STRLIT_ARGS("Time"));
    case 'f':
        return ncm_string_view_make(STRLIT_ARGS("Filename"));
    case 'D':
        return ncm_string_view_make(STRLIT_ARGS("Directory"));
    case 'F':
        return ncm_string_view_make(STRLIT_ARGS("Filepath"));
    case 'a':
        return ncm_string_view_make(STRLIT_ARGS("Artist"));
    case 'A':
        return ncm_string_view_make(STRLIT_ARGS("Album Artist"));
    case 't':
        return ncm_string_view_make(STRLIT_ARGS("Title"));
    case 'b':
        return ncm_string_view_make(STRLIT_ARGS("Album"));
    case 'y':
        return ncm_string_view_make(STRLIT_ARGS("Date"));
    case 'n':
    case 'N':
        return ncm_string_view_make(STRLIT_ARGS("Track"));
    case 'g':
        return ncm_string_view_make(STRLIT_ARGS("Genre"));
    case 'c':
        return ncm_string_view_make(STRLIT_ARGS("Composer"));
    case 'p':
        return ncm_string_view_make(STRLIT_ARGS("Performer"));
    case 'd':
        return ncm_string_view_make(STRLIT_ARGS("Disc"));
    case 'C':
        return ncm_string_view_make(STRLIT_ARGS("Comment"));
    case 'P':
        return ncm_string_view_make(STRLIT_ARGS("Priority"));
    default:
        return ncm_string_view_make(STRLIT_ARGS("?"));
    }
}

static void
ncm_display_append_column_name(NcmBuffer *buffer, Column *column) {
    NcmStringView name;

    if ((column->name != NULL) && (column->name_len > 0)) {
        ncm_buffer_append(buffer, column->name, column->name_len);
        return;
    }

    for (int32 i = 0; i < column->type_len; i += 1) {
        if (i > 0) {
            ncm_buffer_append_byte(buffer, '/');
        }
        name = ncm_display_column_type_name(column->type[i]);
        ncm_buffer_append(buffer, name.data, name.len);
    }
    return;
}

static void
ncm_display_append_spaces(NcmBuffer *buffer, int32 count) {
    for (int32 i = 0; i < count; i += 1) {
        ncm_buffer_append_byte(buffer, ' ');
    }
    return;
}
