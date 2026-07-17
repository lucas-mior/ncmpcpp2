#if !defined(NCM_SONG_H)
#define NCM_SONG_H

#include <mpd/tag.h>
#include <time.h>

#include "c/ncm_defs.h"
#include "c/ncm_type_conversions.h"

struct mpd_song;

enum NcmSongOwnership {
    NCM_SONG_BORROWED,
    NCM_SONG_OWNED,
};

typedef struct NcmSongTag {
    char *value;
    int32 value_len;
    enum mpd_tag_type type;
} NcmSongTag;

typedef struct NcmSong {
    char *uri;
    int32 uri_len;

    NcmSongTag *tags;
    int32 tags_len;
    int32 tags_cap;

    uint32 duration;
    uint32 position;
    uint32 id;
    uint32 priority;
    time_t last_modified;
} NcmSong;

void ncm_song_init(NcmSong *song);
void ncm_song_destroy(NcmSong *song);
void ncm_song_move(NcmSong *dest, NcmSong *source);
bool ncm_song_copy(NcmSong *dest, NcmSong *source);
bool ncm_song_from_mpd_song(NcmSong *dest, struct mpd_song *source);
bool ncm_song_from_mpd_song_copy(NcmSong *dest, struct mpd_song *source);
bool ncm_song_set_uri(NcmSong *song, char *uri, int32 uri_len);
bool ncm_song_add_tag(NcmSong *song, enum mpd_tag_type type,
                      char *value, int32 value_len);
void ncm_song_set_duration(NcmSong *song, uint32 duration);
void ncm_song_set_position(NcmSong *song, uint32 position);
void ncm_song_set_id(NcmSong *song, uint32 id);
void ncm_song_set_priority(NcmSong *song, uint32 priority);
void ncm_song_set_mtime(NcmSong *song, time_t last_modified);
uint32 ncm_song_duration(NcmSong *song);
uint32 ncm_song_position(NcmSong *song);
uint32 ncm_song_id(NcmSong *song);
uint32 ncm_song_priority(NcmSong *song);
time_t ncm_song_mtime(NcmSong *song);
bool ncm_song_empty(NcmSong *song);

bool ncm_song_tag_view(NcmSong *song, enum mpd_tag_type tag,
                       uint32 idx, NcmStringView *view);
bool ncm_song_uri_view(NcmSong *song, uint32 idx, NcmStringView *view);
bool ncm_song_name_view(NcmSong *song, uint32 idx, NcmStringView *view);
bool ncm_song_directory_view(NcmSong *song, uint32 idx,
                             NcmStringView *view);
bool ncm_song_is_from_database(NcmSong *song);
bool ncm_song_is_stream(NcmSong *song);

bool ncm_song_name_from_uri(char *uri, int32 uri_len, NcmStringView *view);
bool ncm_song_directory_from_uri(char *uri, int32 uri_len,
                                  NcmStringView *view);
bool ncm_song_uri_is_from_database(char *uri, int32 uri_len);
bool ncm_song_uri_is_stream(char *uri, int32 uri_len);
int32 ncm_song_numeric_tag_len(char *tag, int32 tag_len);
int32 ncm_song_track_number_len(char *tag, int32 tag_len);
int32 ncm_song_format_numeric_tag(char *buffer, int32 buffer_cap,
                                  char *tag, int32 tag_len);
int32 ncm_song_format_track_number(char *buffer, int32 buffer_cap,
                                   char *tag, int32 tag_len);
int32 ncm_song_show_time(uint32 length, char *buffer, int32 buffer_cap);
NcmBuffer ncm_song_getter_buffer(NcmSong *song,
                                 enum NcmSongGetter getter, uint32 idx);
NcmBuffer ncm_song_tags_buffer(NcmSong *song, enum NcmSongGetter getter,
                               char *separator, int32 separator_len,
                               bool show_duplicates);
bool ncm_song_equal(NcmSong *a, NcmSong *b);

#endif /* NCM_SONG_H */
