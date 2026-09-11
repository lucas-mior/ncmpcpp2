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
ncm_tag_type_name_len(enum TagType tag, char **out) {
    return ncm_tag_type_display_name_len(tag, out);
}

char *
ncm_tag_type_name(enum TagType tag) {
    char *result;

    ncm_tag_type_name_len(tag, &result);
    return result;
}

enum TagType
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
#define SONG_GETTER_NON_TAG_CHAR_CASE(getter, alias, getter_char)        \
    case getter_char:                                                         \
        return getter;
#define SONG_GETTER_TAG_CHAR_CASE(suffix, display, tag_char,          \
                                      getter_char, flags)                   \
    case getter_char:                                                         \
        return CAT(SONG_GETTER_, suffix);

    SONG_GETTER_RECORD_LENGTH(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_DIRECTORY(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_NAME(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_URI(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_TAG_HEAD_DEFS(SONG_GETTER_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_TRACK_NUMBER(
        SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_TAG_TAIL_DEFS(SONG_GETTER_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_PRIORITY(SONG_GETTER_NON_TAG_CHAR_CASE)

#undef SONG_GETTER_NON_TAG_CHAR_CASE
#undef SONG_GETTER_TAG_CHAR_CASE
    default:
        return SONG_GETTER_NONE;
    }
}

enum TagType
ncm_song_getter_to_tag_type(enum SongGetter getter) {
    switch (getter) {
#define SONG_GETTER_TO_TAG_CASE(suffix, display, tag_char,          \
                                    getter_char, flags)                     \
    case CAT(SONG_GETTER_, suffix):                                           \
        return CAT(TAG_, suffix);

    SONG_GETTER_TAG_DEFS(SONG_GETTER_TO_TAG_CASE)

#undef SONG_GETTER_TO_TAG_CASE
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

enum SongGetter
ncm_tag_type_to_song_getter(enum TagType tag) {
#define NCM_TAG_TO_GETTER_IF(suffix, display, tag_char, getter_char, flags)   \
    if (tag == CAT(TAG_, suffix)) {                                           \
        return CAT(SONG_GETTER_, suffix);                                     \
    }

    TAG_FIELD_DEFS(NCM_TAG_TO_GETTER_IF)

#undef NCM_TAG_TO_GETTER_IF
    return SONG_GETTER_NONE;
}

#endif /* NCM_TYPE_CONVERSIONS_C */
