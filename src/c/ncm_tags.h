#if !defined(NCM_TAGS_H)
#define NCM_TAGS_H

#include <mpd/tag.h>

#include "c/ncm_defs.h"

struct mpd_song;

#define ENUM_NAME NcmTagsField
#define ENUM_PREFIX_ NCM_TAGS_FIELD_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_TAGS_FIELD_TITLE) \
    X(NCM_TAGS_FIELD_ARTIST) \
    X(NCM_TAGS_FIELD_ALBUM_ARTIST) \
    X(NCM_TAGS_FIELD_ALBUM) \
    X(NCM_TAGS_FIELD_DATE) \
    X(NCM_TAGS_FIELD_TRACK) \
    X(NCM_TAGS_FIELD_GENRE) \
    X(NCM_TAGS_FIELD_COMPOSER) \
    X(NCM_TAGS_FIELD_PERFORMER) \
    X(NCM_TAGS_FIELD_DISC) \
    X(NCM_TAGS_FIELD_COMMENT)
#include "cbase/xenums.c"

#define ENUM_NAME NcmTagsReadResult
#define ENUM_PREFIX_ NCM_TAGS_READ_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_TAGS_READ_OK) \
    X(NCM_TAGS_READ_OPEN_FAILED) \
    X(NCM_TAGS_READ_NOT_FOUND)
#include "cbase/xenums.c"

typedef struct NcmTagsReplayGainInfo {
    NcmStringView reference_loudness;
    NcmStringView track_gain;
    NcmStringView track_peak;
    NcmStringView album_gain;
    NcmStringView album_peak;
} NcmTagsReplayGainInfo;

typedef void NcmTagsValueCallback(char *value, int32 value_len,
                                  void *user);
typedef bool NcmTagsGetFieldCallback(enum NcmTagsField field,
                                     int32 idx, NcmStringView *value,
                                     void *user);

void ncm_tags_set_attribute(struct mpd_song *song, char *name, char *value);
enum NcmTagsReadResult ncm_tags_read_lyrics(char *path,
                                            NcmTagsValueCallback *callback,
                                            void *user);
bool ncm_tags_read_song(struct mpd_song *song);
bool ncm_tags_write(char *music_dir, char *uri, bool is_from_database,
                    char *directory, char *new_name,
                    NcmTagsGetFieldCallback *callback, void *user);

#endif /* NCM_TAGS_H */
