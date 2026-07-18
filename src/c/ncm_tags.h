#if !defined(NCM_TAGS_H)
#define NCM_TAGS_H

#include <mpd/tag.h>

#include "c/ncm_defs.h"

struct mpd_song;

enum NcmTagsField {
    NCM_TAGS_FIELD_TITLE,
    NCM_TAGS_FIELD_ARTIST,
    NCM_TAGS_FIELD_ALBUM_ARTIST,
    NCM_TAGS_FIELD_ALBUM,
    NCM_TAGS_FIELD_DATE,
    NCM_TAGS_FIELD_TRACK,
    NCM_TAGS_FIELD_GENRE,
    NCM_TAGS_FIELD_COMPOSER,
    NCM_TAGS_FIELD_PERFORMER,
    NCM_TAGS_FIELD_DISC,
    NCM_TAGS_FIELD_COMMENT,
    NCM_TAGS_FIELD_LAST,
};

enum NcmTagsReadResult {
    NCM_TAGS_READ_OK,
    NCM_TAGS_READ_OPEN_FAILED,
    NCM_TAGS_READ_NOT_FOUND,
};

typedef struct NcmTagsReplayGainInfo {
    NcmStringView reference_loudness;
    NcmStringView track_gain;
    NcmStringView track_peak;
    NcmStringView album_gain;
    NcmStringView album_peak;
} NcmTagsReplayGainInfo;

typedef void (*NcmTagsValueCallback)(char *value, int32 value_len,
                                     void *user);
typedef bool (*NcmTagsGetFieldCallback)(enum NcmTagsField field,
                                        int32 idx, NcmStringView *value,
                                        void *user);

void ncm_tags_set_attribute(struct mpd_song *song, char *name, char *value);
enum NcmTagsReadResult ncm_tags_read_lyrics(char *path,
                                            NcmTagsValueCallback callback,
                                            void *user);
bool ncm_tags_read_song(struct mpd_song *song);
bool ncm_tags_write(char *music_dir, char *uri, bool is_from_database,
                    char *directory, char *new_name,
                    NcmTagsGetFieldCallback callback, void *user);

#endif /* NCM_TAGS_H */
