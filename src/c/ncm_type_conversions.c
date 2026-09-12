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
#define TAG_CHAR_CASE(SUFFIX, DISP, CHAR) \
    case CHAR:                             \
        return CAT(TAG_, SUFFIX);

    TAG_DEFS(TAG_CHAR_CASE)

#undef TAG_CHAR_CASE
    default:
        return TAG_COUNT;
    }
}

enum SongGetter
ncm_song_getter_from_char(char c) {
    switch (c) {
#define SONG_GETTER_CHAR_CASE(GETTER, DISP, CHAR) \
    case CHAR:                                     \
        return GETTER;

    SONG_GETTER_DEFS(SONG_GETTER_CHAR_CASE)

#undef SONG_GETTER_CHAR_CASE
    default:
        return SONG_GETTER_COUNT;
    }
}

enum TagType
ncm_song_getter_to_tag_type(enum SongGetter getter) {
    switch (getter) {
    case SONG_GETTER_ARTIST:
        return TAG_ARTIST;
    case SONG_GETTER_ALBUM_ARTIST:
        return TAG_ALBUM_ARTIST;
    case SONG_GETTER_TITLE:
        return TAG_TITLE;
    case SONG_GETTER_ALBUM:
        return TAG_ALBUM;
    case SONG_GETTER_DATE:
        return TAG_DATE;
    case SONG_GETTER_GENRE:
        return TAG_GENRE;
    case SONG_GETTER_COMPOSER:
        return TAG_COMPOSER;
    case SONG_GETTER_PERFORMER:
        return TAG_PERFORMER;
    case SONG_GETTER_DISC:
        return TAG_DISC;
    case SONG_GETTER_COMMENT:
        return TAG_COMMENT;
    case SONG_GETTER_LENGTH:
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_URI:
    case SONG_GETTER_TRACK_NUMBER:
    case SONG_GETTER_TRACK_TOTAL:
    case SONG_GETTER_PRIORITY:
    case SONG_GETTER_COUNT:
    default:
        return TAG_COUNT;
    }
}

enum SongGetter
ncm_tag_type_to_song_getter(enum TagType tag) {
    switch (tag) {
    case TAG_ARTIST:
        return SONG_GETTER_ARTIST;
    case TAG_ALBUM_ARTIST:
        return SONG_GETTER_ALBUM_ARTIST;
    case TAG_ALBUM:
        return SONG_GETTER_ALBUM;
    case TAG_DISC:
        return SONG_GETTER_DISC;
    case TAG_TRACK:
        return SONG_GETTER_TRACK_NUMBER;
    case TAG_GENRE:
        return SONG_GETTER_GENRE;
    case TAG_DATE:
        return SONG_GETTER_DATE;
    case TAG_COMPOSER:
        return SONG_GETTER_COMPOSER;
    case TAG_PERFORMER:
        return SONG_GETTER_PERFORMER;
    case TAG_TITLE:
        return SONG_GETTER_TITLE;
    case TAG_COMMENT:
        return SONG_GETTER_COMMENT;
    case TAG_COUNT:
    default:
        return SONG_GETTER_COUNT;
    }
}

#endif /* NCM_TYPE_CONVERSIONS_C */
