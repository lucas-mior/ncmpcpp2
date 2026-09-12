#if !defined(TAGS_H)
#define TAGS_H

#include "cbase.h"

#define TAG_DISPLAY_NAME(DISP) #DISP
#define TAG_DISPLAY_NAME_LEN(DISP) STRLIT_LEN(#DISP)

#define TAG_DEFS(XX)                         \
  XX(ARTIST, Artist, 'a')                    \
  XX(ALBUM_ARTIST, Album Artist, 'A')        \
  XX(ALBUM, Album, 'b')                      \
  XX(DISC, Disc, 'd')                        \
  XX(TRACK, Track, 'n')                      \
  XX(GENRE, Genre, 'g')                      \
  XX(DATE, Date, 'y')                        \
  XX(COMPOSER, Composer, 'c')                \
  XX(PERFORMER, Performer, 'p')              \
  XX(TITLE, Title, 't')                      \
  XX(COMMENT, Comment, 'C')

enum {
    TAG_DERIVED_NAME_CAP = 64,
    TAGLIB_PROPERTY_CAP = TAG_DERIVED_NAME_CAP,
    TAGLIB_NAME_CAP = TAG_DERIVED_NAME_CAP,
};

#define TAG_TYPE_ENUM_FIELD(SUFFIX, DISP, CHAR) \
  XX(CAT(TAG_, SUFFIX), DISP)

#define TAG_TYPE_ENUM_FIELDS                   \
  TAG_DEFS(TAG_TYPE_ENUM_FIELD)

#define SONG_GETTER_DEFS(XX)                         \
  XX(SONG_GETTER_NONE, none, '\0')                   \
  XX(SONG_GETTER_LENGTH, Length, 'l')                \
  XX(SONG_GETTER_DIRECTORY, Directory, 'D')          \
  XX(SONG_GETTER_NAME, Filename, 'f')                \
  XX(SONG_GETTER_URI, URI, 'F')                      \
  XX(SONG_GETTER_ARTIST, Artist, 'a')                \
  XX(SONG_GETTER_ALBUM_ARTIST, Album Artist, 'A')    \
  XX(SONG_GETTER_TITLE, Title, 't')                  \
  XX(SONG_GETTER_ALBUM, Album, 'b')                  \
  XX(SONG_GETTER_DATE, Date, 'y')                    \
  XX(SONG_GETTER_TRACK_NUMBER, Track Number, 'n')    \
  XX(SONG_GETTER_TRACK_TOTAL, Total Tracks, 'N')     \
  XX(SONG_GETTER_GENRE, Genre, 'g')                  \
  XX(SONG_GETTER_COMPOSER, Composer, 'c')            \
  XX(SONG_GETTER_PERFORMER, Performer, 'p')          \
  XX(SONG_GETTER_DISC, Disc, 'd')                    \
  XX(SONG_GETTER_COMMENT, Comment, 'C')              \
  XX(SONG_GETTER_PRIORITY, Priority, 'P')

#define SONG_GETTER_ENUM_FIELD(GETTER, DISP, CHAR) \
  XX(GETTER, DISP)

#define SONG_GETTER_ENUM_FIELDS                   \
  SONG_GETTER_DEFS(SONG_GETTER_ENUM_FIELD)

#define ENUM_NAME TagType
#define ENUM_PREFIX_ TAG_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS TAG_TYPE_ENUM_FIELDS
#include "cbase/xenums.c"

#define ENUM_NAME SongGetter
#define ENUM_PREFIX_ SONG_GETTER_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS SONG_GETTER_ENUM_FIELDS
#include "cbase/xenums.c"

static inline int32
ncm_tag_type_display_name_len(enum TagType tag, char **out) {
    if ((uint32)tag >= (uint32)TAG_COUNT) {
        *out = "";
        return 0;
    }

    return TAG_alias_len(tag, out);
}

static inline bool
ncm_tag_type_is_writable(enum TagType tag) {
    return (uint32)tag < TAG_COUNT;
}

static inline enum TagType
ncm_writable_tag_at(int32 idx) {
    if ((idx < 0) || (idx >= (int32)TAG_COUNT)) {
        return TAG_COUNT;
    }
    return (enum TagType)idx;
}

static inline int32
ncm_song_getter_column_title_len(enum SongGetter getter, char **out) {
    switch (getter) {
    case SONG_GETTER_LENGTH:
        *out = "Time";
        return STRLIT_LEN("Time");
    case SONG_GETTER_URI:
        *out = "Filepath";
        return STRLIT_LEN("Filepath");
    case SONG_GETTER_TRACK_NUMBER:
        *out = "Track";
        return STRLIT_LEN("Track");
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_ARTIST:
    case SONG_GETTER_ALBUM_ARTIST:
    case SONG_GETTER_TITLE:
    case SONG_GETTER_ALBUM:
    case SONG_GETTER_DATE:
    case SONG_GETTER_TRACK_TOTAL:
    case SONG_GETTER_GENRE:
    case SONG_GETTER_COMPOSER:
    case SONG_GETTER_PERFORMER:
    case SONG_GETTER_DISC:
    case SONG_GETTER_COMMENT:
    case SONG_GETTER_PRIORITY:
        return SONG_GETTER_alias_len(getter, out);
    case SONG_GETTER_NONE:
    case SONG_GETTER_COUNT:
    default:
        *out = "?";
        return STRLIT_LEN("?");
    }
}

static inline int32
ncm_song_getter_sort_label_len(enum SongGetter getter, char **out) {
    switch (getter) {
    case SONG_GETTER_URI:
        return SONG_GETTER_alias_len(SONG_GETTER_NAME, out);
    case SONG_GETTER_TRACK_NUMBER:
        return TAG_alias_len(TAG_TRACK, out);
    case SONG_GETTER_ARTIST:
    case SONG_GETTER_ALBUM_ARTIST:
    case SONG_GETTER_TITLE:
    case SONG_GETTER_ALBUM:
    case SONG_GETTER_DATE:
    case SONG_GETTER_GENRE:
    case SONG_GETTER_COMPOSER:
    case SONG_GETTER_PERFORMER:
    case SONG_GETTER_DISC:
    case SONG_GETTER_COMMENT:
        return SONG_GETTER_alias_len(getter, out);
    case SONG_GETTER_NONE:
    case SONG_GETTER_LENGTH:
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_TRACK_TOTAL:
    case SONG_GETTER_PRIORITY:
    case SONG_GETTER_COUNT:
    default:
        *out = "";
        return 0;
    }
}

static inline int32
ncm_tag_type_taglib_property_len(enum TagType tag, char *out, int32 cap) {
    char *alias;
    int32 alias_len;
    int32 result;

    ASSERT(out != NULL);
    ASSERT_POSITIVE(cap);

    if ((uint32)tag >= TAG_COUNT) {
        out[0] = '\0';
        return -1;
    }

    alias_len = TAG_alias_len(tag, &alias);
    if (alias_len >= cap) {
        out[0] = '\0';
        return -1;
    }
    result = ascii_normalize_upper_compact(out, alias, alias_len);
    if ((tag != TAG_TRACK) && (tag != TAG_DISC)) {
        return result;
    }
    if (result + STRLIT_LEN("NUMBER") >= cap) {
        out[0] = '\0';
        return -1;
    }

    memcpy64(out + result, STRLIT("NUMBER") + 1);
    return result + STRLIT_LEN("NUMBER");
}

static inline int32
ncm_tag_type_taglib_name_len(enum TagType tag, char *out, int32 cap) {
    char *alias;
    int32 alias_len;

    ASSERT(out != NULL);
    ASSERT_POSITIVE(cap);

    if ((uint32)tag >= TAG_COUNT) {
        out[0] = '\0';
        return -1;
    }

    alias_len = TAG_alias_len(tag, &alias);
    if (alias_len >= cap) {
        out[0] = '\0';
        return -1;
    }

    return ascii_normalize_camel_compact(out, alias, alias_len);
}

static inline char
ncm_tag_type_format_char(enum TagType tag) {
    switch (tag) {
#define TAG_TYPE_FORMAT_CHAR_CASE(SUFFIX, DISP, CHAR) \
    case CAT(TAG_, SUFFIX):                            \
        return CHAR;

    TAG_DEFS(TAG_TYPE_FORMAT_CHAR_CASE)

#undef TAG_TYPE_FORMAT_CHAR_CASE
    case TAG_COUNT:
    default:
        return '\0';
    }
}

static inline int32
ncm_tag_type_parser_name_len(enum TagType tag, char **out) {
    if (tag == TAG_TRACK) {
        return SONG_GETTER_alias_len(SONG_GETTER_TRACK_NUMBER, out);
    }
    if (!ncm_tag_type_is_writable(tag)) {
        *out = "";
        return 0;
    }

    return TAG_alias_len(tag, out);
}

static inline char
ncm_song_getter_format_char(enum SongGetter getter) {
    switch (getter) {
#define SONG_GETTER_CHAR_CASE(GETTER, DISP, CHAR) \
    case GETTER:                                   \
        return CHAR;

    SONG_GETTER_DEFS(SONG_GETTER_CHAR_CASE)

#undef SONG_GETTER_CHAR_CASE
    case SONG_GETTER_COUNT:
    default:
        return '\0';
    }
}

#endif /* TAGS_H */
