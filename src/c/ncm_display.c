#if !defined(NCM_DISPLAY_C)
#define NCM_DISPLAY_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "settings.h"

static int32 ncm_display_column_width(Column *column, int32 list_width,
                                      int32 remained_width);
static void ncm_display_append_spaces(StrBuilder *buffer, int32 count);
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
        StrBuilder value = {0};
        Column *column;
        int32 cut_len;
        int32 padding;
        int32 value_width;
        int32 width;

        column = &columns[i];
        width = ncm_display_column_width(column, list_width,
                                         remained_width);
        if (width <= 0) {
            continue;
        }
        if (column != last) {
            width -= 1;
        }
        if (((remained_width - width) < 0) || (width < 0)) {
            break;
        }

        for (int32 j = 0; j < column->type_len; j += 1) {
            enum NcmSongGetter getter =
                ncm_song_getter_from_char(column->type[j]);

            if (getter != NCM_SONG_GETTER_NONE) {
                StrBuilder tag_value = ncm_song_tags_buffer(
                    song, getter, Config.tags_separator,
                    Config.tags_separator_len, Config.show_duplicate_tags);

                if (tag_value.len > 0) {
                    sb_move(&value, &tag_value);
                    sb_free(&tag_value);
                    break;
                }
                sb_free(&tag_value);
            }
        }
        if ((value.len == 0) && column->display_empty_tag && Config.empty_tag_marker
            && (Config.empty_tag_marker_len > 0)) {
            SB_APPEND(&value, Config.empty_tag_marker, Config.empty_tag_marker_len);
        }
        cut_len = utf8_cut_width(value.data, value.len, width);
        value_width = utf8_width(value.data, cut_len);
        padding = width - value_width;
        if (padding < 0) {
            padding = 0;
        }

        if (use_colors && !nc_color_is_default(column->color)) {
            nc_buffer_add_color(buffer, buffer->len,
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
            nc_buffer_add_color(buffer, buffer->len,
                                nc_color_end(), 0);
        }
        sb_free(&value);

        if (column != last) {
            remained_width -= width + 1;
            nc_buffer_append_char(buffer, ' ');
        }
    }
    return;
}

void
ncm_display_column_title(StrBuilder *buffer, struct Column *columns,
                         int32 column_count, int32 list_width) {
    StrBuilder name = {0};
    Column *last;
    int32 remained_width;

    if (buffer == NULL) {
        return;
    }

    sb_clear(buffer);
    if ((columns == NULL) || (column_count <= 0) || (list_width <= 0)) {
        return;
    }

    remained_width = list_width;
    last = &columns[column_count - 1];
    for (int32 i = 0; i < column_count; i += 1) {
        Column *column = &columns[i];
        int32 width = ncm_display_column_width(column,
                                               list_width, remained_width);
        int32 cut_len;
        int32 name_width;
        int32 padding;

        if (width <= 0) {
            continue;
        }
        if (column != last) {
            width -= 1;
        }
        if (((remained_width - width) < 0) || (width < 0)) {
            break;
        }

        sb_clear(&name);
        if (column->name && (column->name_len > 0)) {
            SB_APPEND(&name, column->name, column->name_len);
        } else {
            for (int32 j = 0; j < column->type_len; j += 1) {
                if (j > 0) {
                    sb_append_byte(&name, '/');
                }
                switch (column->type[j]) {
                case 'l':
                    SB_APPEND(&name, STRLIT("Time"));
                    break;
                case 'f':
                    SB_APPEND(&name, STRLIT("Filename"));
                    break;
                case 'D':
                    SB_APPEND(&name, STRLIT("Directory"));
                    break;
                case 'F':
                    SB_APPEND(&name, STRLIT("Filepath"));
                    break;
                case 'a':
                    SB_APPEND(&name, STRLIT("Artist"));
                    break;
                case 'A':
                    SB_APPEND(&name, STRLIT("Album Artist"));
                    break;
                case 't':
                    SB_APPEND(&name, STRLIT("Title"));
                    break;
                case 'b':
                    SB_APPEND(&name, STRLIT("Album"));
                    break;
                case 'y':
                    SB_APPEND(&name, STRLIT("Date"));
                    break;
                case 'n':
                case 'N':
                    SB_APPEND(&name, STRLIT("Track"));
                    break;
                case 'g':
                    SB_APPEND(&name, STRLIT("Genre"));
                    break;
                case 'c':
                    SB_APPEND(&name, STRLIT("Composer"));
                    break;
                case 'p':
                    SB_APPEND(&name, STRLIT("Performer"));
                    break;
                case 'd':
                    SB_APPEND(&name, STRLIT("Disc"));
                    break;
                case 'C':
                    SB_APPEND(&name, STRLIT("Comment"));
                    break;
                case 'P':
                    SB_APPEND(&name, STRLIT("Priority"));
                    break;
                default:
                    SB_APPEND(&name, STRLIT("?"));
                    break;
                }
            }
        }
        cut_len = utf8_cut_width(name.data, name.len, width);
        name_width = utf8_width(name.data, cut_len);
        padding = width - name_width;
        if (padding < 0) {
            padding = 0;
        }
        if (column->right_alignment) {
            ncm_display_append_spaces(buffer, padding);
            SB_APPEND(buffer, name.data, cut_len);
        } else {
            SB_APPEND(buffer, name.data, cut_len);
            ncm_display_append_spaces(buffer, padding);
        }

        if (column != last) {
            remained_width -= width + 1;
            sb_append_byte(buffer, ' ');
        }
    }
    sb_free(&name);
    return;
}

void
ncm_display_directory_row(NcBuffer *buffer, NcmDirectory *directory) {
    NcmStringView path;

    if (!ncm_directory_has_path_view(directory, &path)) {
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

    if (prefix && (prefix_len > 0)) {
        nc_buffer_append_data(buffer, prefix, prefix_len);
    }
    if (!ncm_playlist_has_path_view(playlist, &path)) {
        return;
    }
    ncm_display_append_basename(buffer, path.data, path.len);
    return;
}

static int32
ncm_display_column_width(Column *column,
                         int32 list_width, int32 remained_width) {
    int32 width;

    if (column->stretch_limit >= 0) {
        width = remained_width - column->stretch_limit;
    } else if (column->fixed) {
        width = column->width;
    } else {
        width = column->width*list_width/100;
    }

    return width;
}

static void
ncm_display_append_nc_spaces(NcBuffer *buffer, int32 count) {
    for (int32 i = 0; i < count; i += 1) {
        nc_buffer_append_char(buffer, ' ');
    }
    return;
}

static void
ncm_display_append_spaces(StrBuilder *buffer, int32 count) {
    for (int32 i = 0; i < count; i += 1) {
        sb_append_byte(buffer, ' ');
    }
    return;
}

#endif /* NCM_DISPLAY_C */
