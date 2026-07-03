#if !defined(NCM_SONG_H)
#define NCM_SONG_H

#include <mpd/tag.h>

#include "c/ncm_defs.h"

struct mpd_song;

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmSong {
    struct mpd_song *song;
} NcmSong;

void ncm_song_init(NcmSong *song);
void ncm_song_destroy(NcmSong *song);
bool ncm_song_copy(NcmSong *dest, NcmSong *source);
bool ncm_song_from_mpd_song(NcmSong *dest, struct mpd_song *source);
bool ncm_song_empty(NcmSong *song);
struct mpd_song *ncm_song_mpd_song(NcmSong *song);
struct mpd_song *ncm_song_dup_mpd_song(NcmSong *song);

bool ncm_song_name_from_uri(char *uri, int32 uri_len, NcmStringView *view);
bool ncm_song_directory_from_uri(char *uri, int32 uri_len,
                                  NcmStringView *view);
bool ncm_song_uri_is_from_database(char *uri, int32 uri_len);
bool ncm_song_uri_is_stream(char *uri, int32 uri_len);
bool ncm_mpd_song_tag_view(struct mpd_song *song, enum mpd_tag_type tag,
                           uint32 idx, NcmStringView *view);
bool ncm_mpd_song_uri_view(struct mpd_song *song, uint32 idx,
                           NcmStringView *view);
bool ncm_mpd_song_name_view(struct mpd_song *song, uint32 idx,
                            NcmStringView *view);
bool ncm_mpd_song_directory_view(struct mpd_song *song, uint32 idx,
                                 NcmStringView *view);
bool ncm_mpd_song_is_from_database(struct mpd_song *song);
bool ncm_mpd_song_is_stream(struct mpd_song *song);
int32 ncm_song_numeric_tag_len(char *tag, int32 tag_len);
int32 ncm_song_track_number_len(char *tag, int32 tag_len);
int32 ncm_song_format_numeric_tag(char *buffer, int32 buffer_cap,
                                  char *tag, int32 tag_len);
int32 ncm_song_format_track_number(char *buffer, int32 buffer_cap,
                                   char *tag, int32 tag_len);
int32 ncm_song_show_time(uint32 length, char *buffer, int32 buffer_cap);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_SONG_H */
