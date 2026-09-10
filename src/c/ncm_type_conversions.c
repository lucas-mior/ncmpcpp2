#if !defined(NCM_TYPE_CONVERSIONS_C)
#define NCM_TYPE_CONVERSIONS_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

int32
ncm_channels_to_string(int32 channels, char *buffer, int32 buffer_cap) {
    int32 result;

    if ((buffer == NULL) || (buffer_cap <= 0)) {
        return 0;
    }

    switch (channels) {
    case 1:
        result = snprintf2(buffer, buffer_cap, "Mono");
        break;
    case 2:
        result = snprintf2(buffer, buffer_cap, "Stereo");
        break;
    default:
        result = snprintf2(buffer, buffer_cap, "%d", channels);
        break;
    }

    if (result < 0) {
        buffer[0] = '\0';
        return 0;
    }
    if (result >= buffer_cap) {
        result = buffer_cap - 1;
    }

    return result;
}

int32
ncm_color_index_from_char(char c) {
    switch (c) {
    case '0':
        return 0;
    case '1':
        return 1;
    case '2':
        return 2;
    case '3':
        return 3;
    case '4':
        return 4;
    case '5':
        return 5;
    case '6':
        return 6;
    case '7':
        return 7;
    case '8':
        return 8;
    case '9':
        return 9;
    default:
        return -1;
    }
}

int32
ncm_tag_type_name_len(enum NcmTagType tag, char **out) {
    return ncm_tag_type_display_name_len(tag, out);
}

char *
ncm_tag_type_name(enum NcmTagType tag) {
    char *result;

    ncm_tag_type_name_len(tag, &result);
    return result;
}

enum NcmTagType
ncm_char_to_tag_type(char c) {
    switch (c) {
#define TAG_CHAR_CASE(suffix, display, tag_char, getter_char,        \
                          flags)                                             \
    case tag_char:                                                            \
        return CAT(TAG_, suffix);

    TAG_FIELD_DEFS(TAG_CHAR_CASE)

#undef TAG_CHAR_CASE
    default:
        return TAG_UNKNOWN;
    }
}

enum SongGetter
ncm_song_getter_from_char(char c) {
    switch (c) {
#define NCM_SONG_GETTER_NON_TAG_CHAR_CASE(getter, alias, getter_char)        \
    case getter_char:                                                         \
        return getter;
#define NCM_SONG_GETTER_TAG_CHAR_CASE(suffix, display, tag_char,          \
                                      getter_char, flags)                   \
    case getter_char:                                                         \
        return CAT(SONG_GETTER_, suffix);

    NCM_SONG_GETTER_RECORD_LENGTH(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_DIRECTORY(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_NAME(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_URI(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_TAG_HEAD_DEFS(NCM_SONG_GETTER_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_TRACK_NUMBER(
        NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_TAG_TAIL_DEFS(NCM_SONG_GETTER_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_PRIORITY(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)

#undef NCM_SONG_GETTER_NON_TAG_CHAR_CASE
#undef NCM_SONG_GETTER_TAG_CHAR_CASE
    default:
        return SONG_GETTER_NONE;
    }
}

enum NcmTagType
ncm_song_getter_to_tag_type(enum SongGetter getter) {
    switch (getter) {
#define NCM_SONG_GETTER_TO_TAG_CASE(suffix, display, tag_char,          \
                                    getter_char, flags)                     \
    case CAT(SONG_GETTER_, suffix):                                           \
        return CAT(TAG_, suffix);

    NCM_SONG_GETTER_TAG_DEFS(NCM_SONG_GETTER_TO_TAG_CASE)

#undef NCM_SONG_GETTER_TO_TAG_CASE
    case SONG_GETTER_NONE:
    case SONG_GETTER_LENGTH:
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_URI:
    case SONG_GETTER_TRACK_NUMBER:
    case SONG_GETTER_PRIORITY:
    case SONG_GETTER_COUNT:
    default:
        return TAG_UNKNOWN;
    }
}

enum TagsField
ncm_tags_field_from_tag_type(enum NcmTagType tag) {
    switch (tag) {
#define TAG_TO_FIELD_CASE(suffix, display, tag_char, getter_char,    \
                              flags)                                         \
    case CAT(TAG_, suffix):                                               \
        return CAT(TAGS_FIELD_, suffix);

    TAG_FIELD_DEFS(TAG_TO_FIELD_CASE)

#undef TAG_TO_FIELD_CASE
    case TAG_UNKNOWN:
    case TAG_NAME:
    case TAG_MUSICBRAINZ_ARTISTID:
    case TAG_MUSICBRAINZ_ALBUMID:
    case TAG_MUSICBRAINZ_ALBUMARTISTID:
    case TAG_MUSICBRAINZ_TRACKID:
    case TAG_MUSICBRAINZ_RELEASETRACKID:
    case TAG_ORIGINAL_DATE:
    case TAG_ARTIST_SORT:
    case TAG_ALBUM_ARTIST_SORT:
    case TAG_ALBUM_SORT:
    case TAG_LABEL:
    case TAG_MUSICBRAINZ_WORKID:
    case TAG_GROUPING:
    case TAG_WORK:
    case TAG_CONDUCTOR:
    case TAG_COMPOSER_SORT:
    case TAG_ENSEMBLE:
    case TAG_MOVEMENT:
    case TAG_MOVEMENTNUMBER:
    case TAG_LOCATION:
    case TAG_MOOD:
    case TAG_TITLE_SORT:
    case TAG_MUSICBRAINZ_RELEASEGROUPID:
    case TAG_SHOWMOVEMENT:
    case TAG_DISCSUBTITLE:
    case TAG_COUNT:
    default:
        return TAGS_FIELD_COUNT;
    }
}

enum TagsField
ncm_tags_field_from_char(char c) {
    return ncm_tags_field_from_tag_type(ncm_char_to_tag_type(c));
}

enum NcmTagType
ncm_tags_field_to_tag_type(enum TagsField field) {
    switch (field) {
#define NCM_FIELD_TO_TAG_CASE(suffix, display, tag_char, getter_char,    \
                              flags)                                         \
    case CAT(TAGS_FIELD_, suffix):                                        \
        return CAT(TAG_, suffix);

    TAG_FIELD_DEFS(NCM_FIELD_TO_TAG_CASE)

#undef NCM_FIELD_TO_TAG_CASE
    case TAGS_FIELD_COUNT:
    default:
        return TAG_UNKNOWN;
    }
}

enum SongGetter
ncm_tags_field_to_song_getter(enum TagsField field) {
    switch (field) {
#define NCM_FIELD_TO_GETTER_CASE(suffix, display, tag_char, getter_char, \
                                 flags)                                      \
    case CAT(TAGS_FIELD_, suffix):                                        \
        return CAT(SONG_GETTER_, suffix);

    TAG_FIELD_DEFS(NCM_FIELD_TO_GETTER_CASE)

#undef NCM_FIELD_TO_GETTER_CASE
    case TAGS_FIELD_COUNT:
    default:
        return SONG_GETTER_NONE;
    }
}

enum TagsField
ncm_song_getter_to_tags_field(enum SongGetter getter) {
    switch (getter) {
#define NCM_GETTER_TO_FIELD_CASE(suffix, display, tag_char, getter_char, \
                                 flags)                                      \
    case CAT(SONG_GETTER_, suffix):                                           \
        return CAT(TAGS_FIELD_, suffix);

    TAG_FIELD_DEFS(NCM_GETTER_TO_FIELD_CASE)

#undef NCM_GETTER_TO_FIELD_CASE
    case SONG_GETTER_TRACK_NUMBER:
        return TAGS_FIELD_TRACK;
    case SONG_GETTER_NONE:
    case SONG_GETTER_LENGTH:
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_URI:
    case SONG_GETTER_PRIORITY:
    case SONG_GETTER_COUNT:
    default:
        return TAGS_FIELD_COUNT;
    }
}

#endif /* NCM_TYPE_CONVERSIONS_C */
