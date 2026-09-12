#if !defined(TAGS_H)
#define TAGS_H

#include "cbase.h"

#define TAG_DISPLAY_NAME(DISP) #DISP
#define TAG_DISPLAY_NAME_LEN(DISP) STRLIT_LEN(#DISP)

#define TAG_FIELD(XX, SUFFIX, DISP, CHAR)                  \
  XX(SUFFIX, DISP, CHAR, CHAR, false)

#define TAG_FIELD_NUM(XX, SUFFIX, DISP, CHAR, GETTER_CHAR) \
  XX(SUFFIX, DISP, CHAR, GETTER_CHAR, true)

#define TAG_DEFS(XX)                                       \
  TAG_FIELD(XX, ARTIST, Artist, 'a')                       \
  TAG_FIELD(XX, ALBUM_ARTIST, Album Artist, 'A')           \
  TAG_FIELD(XX, ALBUM, Album, 'b')                         \
  TAG_FIELD_NUM(XX, DISC, Disc, 'd', 'd')                  \
  TAG_FIELD_NUM(XX, TRACK, Track, 'n', 'N')                \
  TAG_FIELD(XX, GENRE, Genre, 'g')                         \
  TAG_FIELD(XX, DATE, Date, 'y')                           \
  TAG_FIELD(XX, COMPOSER, Composer, 'c')                   \
  TAG_FIELD(XX, PERFORMER, Performer, 'p')                 \
  TAG_FIELD(XX, TITLE, Title, 't')                         \
  TAG_FIELD(XX, COMMENT, Comment, 'C')

enum {
    TAG_DERIVED_NAME_CAP = 64,
    TAGLIB_PROPERTY_CAP = TAG_DERIVED_NAME_CAP,
    TAGLIB_NAME_CAP = TAG_DERIVED_NAME_CAP,
};

#define TAG_TYPE_ENUM_FIELD(SUFFIX, DISP, CHAR, GETTER_CHAR, TAGLIB_NUM)      \
  XX(CAT(TAG_, SUFFIX), DISP)

#define TAG_TYPE_ENUM_FIELDS                                                   \
  TAG_DEFS(TAG_TYPE_ENUM_FIELD)

#define SONG_GETTER_RECORD_NONE(XX)                                            \
  XX(SONG_GETTER_NONE, none, '\0')

#define SONG_GETTER_RECORD_LENGTH(XX)                                          \
  XX(SONG_GETTER_LENGTH, Length, 'l')

#define SONG_GETTER_RECORD_DIRECTORY(XX)                                       \
  XX(SONG_GETTER_DIRECTORY, Directory, 'D')

#define SONG_GETTER_RECORD_NAME(XX)                                            \
  XX(SONG_GETTER_NAME, Filename, 'f')

#define SONG_GETTER_RECORD_URI(XX)                                             \
  XX(SONG_GETTER_URI, URI, 'F')

#define SONG_GETTER_RECORD_TRACK_NUMBER(XX)                                    \
  XX(SONG_GETTER_TRACK_NUMBER, Track Number, 'n')

#define SONG_GETTER_RECORD_PRIORITY(XX)                                        \
  XX(SONG_GETTER_PRIORITY, Priority, 'P')

#define SONG_GETTER_NON_TAG_DEFS(XX)                                           \
  SONG_GETTER_RECORD_NONE(XX)                                                  \
  SONG_GETTER_RECORD_LENGTH(XX)                                                \
  SONG_GETTER_RECORD_DIRECTORY(XX)                                             \
  SONG_GETTER_RECORD_NAME(XX)                                                  \
  SONG_GETTER_RECORD_URI(XX)                                                   \
  SONG_GETTER_RECORD_TRACK_NUMBER(XX)                                          \
  SONG_GETTER_RECORD_PRIORITY(XX)

#define SONG_GETTER_TAG_HEAD_DEFS(XX)                                          \
  TAG_FIELD(XX, ARTIST, Artist, 'a')                                           \
  TAG_FIELD(XX, ALBUM_ARTIST, Album Artist, 'A')                               \
  TAG_FIELD(XX, TITLE, Title, 't')                                             \
  TAG_FIELD(XX, ALBUM, Album, 'b')                                             \
  TAG_FIELD(XX, DATE, Date, 'y')

#define SONG_GETTER_TAG_TAIL_DEFS(XX)                                          \
  TAG_FIELD_NUM(XX, TRACK, Track, 'n', 'N')                                    \
  TAG_FIELD(XX, GENRE, Genre, 'g')                                             \
  TAG_FIELD(XX, COMPOSER, Composer, 'c')                                       \
  TAG_FIELD(XX, PERFORMER, Performer, 'p')                                     \
  TAG_FIELD_NUM(XX, DISC, Disc, 'd', 'd')                                      \
  TAG_FIELD(XX, COMMENT, Comment, 'C')

#define SONG_GETTER_TAG_DEFS(XX)                                               \
  SONG_GETTER_TAG_HEAD_DEFS(XX)                                                \
  SONG_GETTER_TAG_TAIL_DEFS(XX)

#define SONG_GETTER_NON_TAG_ENUM_FIELD(getter, DISP, GETTER_CHAR)              \
  XX(getter, DISP)

#define SONG_GETTER_TAG_ENUM_FIELD(SUFFIX, DISP, CHAR, GETTER_CHAR, TAGLIB_NUM) \
  XX(CAT(SONG_GETTER_, SUFFIX), DISP)

#define SONG_GETTER_ENUM_FIELDS                                                \
  SONG_GETTER_RECORD_NONE(SONG_GETTER_NON_TAG_ENUM_FIELD)                      \
  SONG_GETTER_RECORD_LENGTH(SONG_GETTER_NON_TAG_ENUM_FIELD)                    \
  SONG_GETTER_RECORD_DIRECTORY(SONG_GETTER_NON_TAG_ENUM_FIELD)                 \
  SONG_GETTER_RECORD_NAME(SONG_GETTER_NON_TAG_ENUM_FIELD)                      \
  SONG_GETTER_RECORD_URI(SONG_GETTER_NON_TAG_ENUM_FIELD)                       \
  SONG_GETTER_TAG_HEAD_DEFS(SONG_GETTER_TAG_ENUM_FIELD)                        \
  SONG_GETTER_RECORD_TRACK_NUMBER(SONG_GETTER_NON_TAG_ENUM_FIELD)              \
  SONG_GETTER_TAG_TAIL_DEFS(SONG_GETTER_TAG_ENUM_FIELD)                        \
  SONG_GETTER_RECORD_PRIORITY(SONG_GETTER_NON_TAG_ENUM_FIELD)

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
    case SONG_GETTER_PRIORITY:
        return SONG_GETTER_alias_len(getter, out);
#define SONG_GETTER_TAG_TITLE(SUFFIX, DISP, CHAR, GETTER_CHAR, TAGLIB_NUM) \
    case CAT(SONG_GETTER_, SUFFIX):                                        \
        return SONG_GETTER_alias_len(getter, out);

    SONG_GETTER_TAG_DEFS(SONG_GETTER_TAG_TITLE)

#undef SONG_GETTER_TAG_TITLE
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
#define SONG_GETTER_SORT_LABEL(SUFFIX, DISP, CHAR, GETTER_CHAR, TAGLIB_NUM) \
    case CAT(SONG_GETTER_, SUFFIX):                                         \
        return SONG_GETTER_alias_len(getter, out);

    SONG_GETTER_TAG_DEFS(SONG_GETTER_SORT_LABEL)

#undef SONG_GETTER_SORT_LABEL
    case SONG_GETTER_NONE:
    case SONG_GETTER_LENGTH:
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_TRACK_NUMBER:
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
    bool append_number;
    int32 alias_len;
    int32 result;

    ASSERT(out != NULL);
    ASSERT_POSITIVE(cap);

    switch ((int32)tag) {
#define TAGLIB_PROPERTY_CASE(SUFFIX, DISP, CHAR, GETTER_CHAR, TAGLIB_NUM)      \
    case CAT(TAG_, SUFFIX):                                                    \
        append_number = TAGLIB_NUM;                                            \
        break;

    TAG_DEFS(TAGLIB_PROPERTY_CASE)

#undef TAGLIB_PROPERTY_CASE
    case TAG_COUNT:
    default:
        out[0] = '\0';
        return -1;
    }

    alias_len = TAG_alias_len(tag, &alias);
    if (alias_len >= cap) {
        out[0] = '\0';
        return -1;
    }
    result = ascii_normalize_upper_compact(out, alias, alias_len);
    if (!append_number) {
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
#define TAG_TYPE_FORMAT_CHAR_CASE(SUFFIX, DISP, CHAR, GETTER_CHAR, TAGLIB_NUM) \
    case CAT(TAG_, SUFFIX):                                                    \
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
#define SONG_GETTER_NON_TAG_CHAR_CASE(getter, DISP, GETTER_CHAR)               \
    case getter:                                                               \
        return GETTER_CHAR;
#define SONG_GETTER_TAG_CHAR_CASE(SUFFIX, DISP, CHAR, GETTER_CHAR, TAGLIB_NUM) \
    case CAT(SONG_GETTER_, SUFFIX):                                            \
        return GETTER_CHAR;

    SONG_GETTER_RECORD_NONE(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_LENGTH(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_DIRECTORY(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_NAME(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_URI(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_TAG_HEAD_DEFS(SONG_GETTER_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_TRACK_NUMBER(SONG_GETTER_NON_TAG_CHAR_CASE)
    SONG_GETTER_TAG_TAIL_DEFS(SONG_GETTER_TAG_CHAR_CASE)
    SONG_GETTER_RECORD_PRIORITY(SONG_GETTER_NON_TAG_CHAR_CASE)

#undef SONG_GETTER_NON_TAG_CHAR_CASE
#undef SONG_GETTER_TAG_CHAR_CASE
    case SONG_GETTER_COUNT:
    default:
        return '\0';
    }
}

#endif /* TAGS_H */
