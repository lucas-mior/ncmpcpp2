#if !defined(NCM_DISPLAY_C)
#define NCM_DISPLAY_C

#include "c/ncm_display.h"

#include "c/ncm_base.h"
#include "c/ncm_path.h"
#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "settings.h"

static NcmBuffer ncm_display_column_value(NcmSong *song, Column *column);
static int32 ncm_display_column_width(Column *column, int32 list_width,
                                      int32 remained_width);
static void ncm_display_append_column_name(NcmBuffer *buffer,
                                           Column *column);
static void ncm_display_append_spaces(NcmBuffer *buffer, int32 count);
static void ncm_display_append_nc_spaces(NcBuffer *buffer, int32 count);

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
ncm_display_song_columns(NcBuffer *buffer, NcmSong *song,
                         struct Column *columns, int32 column_count,
                         int32 list_width, bool use_colors) {
    Column *last;
    int32 remained_width;

    if ((buffer == NULL) || (song == NULL)) {
        return;
    }
    if ((columns == NULL) || (column_count <= 0) || (list_width <= 0)) {
        return;
    }

    remained_width = list_width;
    last = &columns[column_count - 1];
    for (int32 i = 0; i < column_count; i += 1) {
        NcmBuffer value;
        Column *column;
        int32 cut_len;
        int32 padding;
        int32 value_width;
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

        value = ncm_display_column_value(song, column);
        cut_len = utf8_cut_width(value.data, value.len, width);
        value_width = utf8_width(value.data, cut_len);
        padding = width - value_width;
        if (padding < 0) {
            padding = 0;
        }

        if (use_colors && !nc_color_is_default(column->color)) {
            nc_buffer_add_color(buffer, nc_buffer_len(buffer),
                                column->color, 0);
        }
        if (column->right_alignment) {
            ncm_display_append_nc_spaces(buffer, padding);
            nc_buffer_append_data(buffer, value.data, cut_len);
        } else {
            nc_buffer_append_data(buffer, value.data, cut_len);
            ncm_display_append_nc_spaces(buffer, padding);
        }
        if (use_colors && !nc_color_is_default(column->color)) {
            nc_buffer_add_color(buffer, nc_buffer_len(buffer),
                                nc_color_end(), 0);
        }
        ncm_buffer_destroy(&value);

        if (column != last) {
            remained_width -= width + 1;
            nc_buffer_append_char(buffer, ' ');
        }
    }
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
        cut_len = utf8_cut_width(name.data, name.len, width);
        name_width = utf8_width(name.data, cut_len);
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

static NcmBuffer
ncm_display_column_value(NcmSong *song, Column *column) {
    NcmBuffer result;

    ncm_buffer_init(&result);
    if ((song == NULL) || (column == NULL)) {
        return result;
    }

    for (int32 i = 0; i < column->type_len; i += 1) {
        NcmBuffer value;
        enum NcmSongGetter getter;

        getter = ncm_song_getter_from_char(column->type[i]);
        if (getter == NCM_SONG_GETTER_NONE) {
            continue;
        }
        value = ncm_song_tags_buffer(
            song, getter, Config.tags_separator,
            Config.tags_separator_len, Config.show_duplicate_tags);
        if (value.len > 0) {
            ncm_buffer_move(&result, &value);
            ncm_buffer_destroy(&value);
            return result;
        }
        ncm_buffer_destroy(&value);
    }

    if (column->display_empty_tag && (Config.empty_tag != NULL)
        && (Config.empty_tag_len > 0)) {
        ncm_buffer_append(&result, Config.empty_tag,
                          Config.empty_tag_len);
    }
    return result;
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
ncm_display_append_nc_spaces(NcBuffer *buffer, int32 count) {
    for (int32 i = 0; i < count; i += 1) {
        nc_buffer_append_char(buffer, ' ');
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

#endif /* NCM_DISPLAY_C */
