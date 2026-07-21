#if !defined(NCM_DISPLAY_H)
#define NCM_DISPLAY_H

#include "c/ncm_directory.h"
#include "c/ncm_format.h"
#include "c/ncm_mpd_connection.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_mutable_song.h"
#include "c/ncm_playlist.h"
#include "curses/nc_buffer.h"

struct Column;

void ncm_display_song_row(NcBuffer *buffer, NcmFormatAst *format,
                          NcmSong *song, uint32 flags);
void ncm_display_song_columns(NcBuffer *buffer, NcmSong *song,
                              struct Column *columns, int32 column_count,
                              int32 list_width, bool use_colors);
void ncm_display_column_title(StrBuilder *buffer,
                              struct Column *columns,
                              int32 column_count, int32 list_width);
void ncm_display_directory_row(NcBuffer *buffer, NcmDirectory *directory);
void ncm_display_playlist_row(NcBuffer *buffer, NcmPlaylist *playlist,
                              char *prefix, int32 prefix_len);

#endif /* NCM_DISPLAY_H */
