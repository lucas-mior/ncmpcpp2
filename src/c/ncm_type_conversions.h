#if !defined(NCM_TYPE_CONVERSIONS_H)
#define NCM_TYPE_CONVERSIONS_H

#include <stdbool.h>
#include <mpd/tag.h>

#include "c/ncm_defs.h"
#include "c/ncm_tags.h"

#define ENUM_NAME NcmItemType
#define ENUM_PREFIX_ NCM_ITEM_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_ITEM_DIRECTORY) \
    X(NCM_ITEM_SONG) \
    X(NCM_ITEM_PLAYLIST) \
    X(NCM_ITEM_UNKNOWN)
#include "cbase/xenums.c"

#define ENUM_NAME NcmSongGetter
#define ENUM_PREFIX_ NCM_SONG_GETTER_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NCM_SONG_GETTER_NONE) \
    X(NCM_SONG_GETTER_LENGTH) \
    X(NCM_SONG_GETTER_DIRECTORY) \
    X(NCM_SONG_GETTER_NAME) \
    X(NCM_SONG_GETTER_URI) \
    X(NCM_SONG_GETTER_ARTIST) \
    X(NCM_SONG_GETTER_ALBUM_ARTIST) \
    X(NCM_SONG_GETTER_TITLE) \
    X(NCM_SONG_GETTER_ALBUM) \
    X(NCM_SONG_GETTER_DATE) \
    X(NCM_SONG_GETTER_TRACK_NUMBER) \
    X(NCM_SONG_GETTER_TRACK) \
    X(NCM_SONG_GETTER_GENRE) \
    X(NCM_SONG_GETTER_COMPOSER) \
    X(NCM_SONG_GETTER_PERFORMER) \
    X(NCM_SONG_GETTER_DISC) \
    X(NCM_SONG_GETTER_COMMENT) \
    X(NCM_SONG_GETTER_PRIORITY)
#include "cbase/xenums.c"

int32 ncm_channels_to_string(int32 channels, char *buffer, int32 buffer_cap);
int32 ncm_color_index_from_char(char c);
char *ncm_tag_type_name(enum mpd_tag_type tag);
enum mpd_tag_type ncm_char_to_tag_type(char c);
enum NcmSongGetter ncm_song_getter_from_char(char c);
enum mpd_tag_type ncm_song_getter_to_tag_type(enum NcmSongGetter getter);
enum NcmTagsField ncm_tags_field_from_char(char c);
enum NcmTagsField ncm_tags_field_from_tag_type(enum mpd_tag_type tag);
enum mpd_tag_type ncm_tags_field_to_tag_type(enum NcmTagsField field);
enum NcmSongGetter ncm_tags_field_to_song_getter(enum NcmTagsField field);
enum NcmTagsField ncm_song_getter_to_tags_field(enum NcmSongGetter getter);
char *ncm_tags_field_name(enum NcmTagsField field);

#endif /* NCM_TYPE_CONVERSIONS_H */
