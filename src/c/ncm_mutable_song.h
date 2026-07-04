#if !defined(NCM_MUTABLE_SONG_H)
#define NCM_MUTABLE_SONG_H

#include "c/ncm_defs.h"
#include "c/ncm_song.h"
#include "c/ncm_tags.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmMutableSongTag {
    char *original;
    char *value;

    int32 original_len;
    int32 value_len;
    int32 idx;

    enum NcmTagsField field;
    bool modified;
} NcmMutableSongTag;

typedef struct NcmMutableSong {
    char *uri;
    char *directory;
    char *name;
    char *new_name;

    int32 uri_len;
    int32 directory_len;
    int32 name_len;
    int32 new_name_len;

    int64 mtime;
    uint32 duration;
    bool is_from_database;

    NcmMutableSongTag *tags;
    int32 tags_len;
    int32 tags_cap;
} NcmMutableSong;

void ncm_mutable_song_init(NcmMutableSong *song);
void ncm_mutable_song_destroy(NcmMutableSong *song);
bool ncm_mutable_song_copy(NcmMutableSong *dest, NcmMutableSong *source);
void ncm_mutable_song_move(NcmMutableSong *dest, NcmMutableSong *source);

bool ncm_mutable_song_set_uri(NcmMutableSong *song, char *uri, int32 uri_len);
bool ncm_mutable_song_set_directory(NcmMutableSong *song, char *directory,
                                    int32 directory_len);
bool ncm_mutable_song_set_name(NcmMutableSong *song, char *name,
                               int32 name_len);
void ncm_mutable_song_set_from_database(NcmMutableSong *song,
                                        bool is_from_database);

bool ncm_mutable_song_set_original_tag(NcmMutableSong *song,
                                       enum NcmTagsField field, int32 idx,
                                       char *value, int32 value_len);
bool ncm_mutable_song_set_tag(NcmMutableSong *song, enum NcmTagsField field,
                              int32 idx, char *value, int32 value_len);
bool ncm_mutable_song_set_tags(NcmMutableSong *song, enum NcmTagsField field,
                               char *value, int32 value_len,
                               char *separator, int32 separator_len);
bool ncm_mutable_song_get_tag(NcmMutableSong *song, enum NcmTagsField field,
                              int32 idx, NcmStringView *view);
NcmBuffer ncm_mutable_song_get_numeric_tag_buffer(NcmMutableSong *song,
                                                  enum NcmTagsField field,
                                                  int32 idx);
bool ncm_mutable_song_load_originals_from_song(NcmMutableSong *dest,
                                               NcmSong *source);

bool ncm_mutable_song_set_new_name(NcmMutableSong *song, char *new_name,
                                   int32 new_name_len);
bool ncm_mutable_song_get_new_name(NcmMutableSong *song, NcmStringView *view);

void ncm_mutable_song_set_duration(NcmMutableSong *song, uint32 duration);
uint32 ncm_mutable_song_duration(NcmMutableSong *song);
void ncm_mutable_song_set_mtime(NcmMutableSong *song, int64 mtime);
int64 ncm_mutable_song_mtime(NcmMutableSong *song);

bool ncm_mutable_song_is_modified(NcmMutableSong *song);
void ncm_mutable_song_clear_modifications(NcmMutableSong *song);
bool ncm_mutable_song_write(NcmMutableSong *song, char *music_dir);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_MUTABLE_SONG_H */
