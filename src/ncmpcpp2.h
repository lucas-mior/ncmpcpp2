#if !defined(NCMPCPP2_H)
#define NCMPCPP2_H

#include "cbase.h"

#define TAG_FLAG_ENUM_FIELDS                   \
  XX(TAG_FLAG_DISPLAY)                         \
  XX(TAG_FLAG_WRITABLE)                        \
  XX(TAG_FLAG_SONG_INFO)                       \
  XX(TAG_FLAG_SEARCH)                          \
  XX(TAG_FLAG_GETTER)                          \
  XX(TAG_FLAG_TAGLIB)                          \
  XX(TAG_FLAG_MPD)                             \
  XX(TAG_FLAG_TAGLIB_NUMBER)                   \
  XX(TAG_FLAGS_FIELD,                          \
     TAG_FLAG_DISPLAY|TAG_FLAG_WRITABLE        \
     |TAG_FLAG_SONG_INFO|TAG_FLAG_GETTER       \
     |TAG_FLAG_TAGLIB)                         \
  XX(TAG_FLAGS_FIELD_SEARCH,                   \
     TAG_FLAGS_FIELD|TAG_FLAG_SEARCH)

#define ENUM_NAME NcmTagMetaFlags
#define ENUM_PREFIX_ TAG_FLAGS_
#define ENUM_BITFLAGS 1
#define ENUM_FIELDS TAG_FLAG_ENUM_FIELDS
#include "cbase/xenums.c"

#define TAG_DISPLAY_NAME(DISP) #DISP
#define TAG_DISPLAY_NAME_LEN(DISP) STRLIT_LEN(#DISP)

#define TAG_FIELD_MPD(XX, suffix, DISP, CHAR)                  \
  XX(suffix, DISP, CHAR, CHAR,                             \
     TAG_FLAGS_FIELD_SEARCH|TAG_FLAG_MPD)

#define TAG_FIELD_MPD_NUM(XX, suffix, DISP, CHAR, getter_char) \
  XX(suffix, DISP, CHAR, getter_char,                          \
     TAG_FLAGS_FIELD|TAG_FLAG_MPD|TAG_FLAG_TAGLIB_NUMBER)

#define TAG_SEARCH_MPD(XX, suffix, DISP)                           \
  XX(suffix, DISP, '\0', '\0',                                     \
     TAG_FLAG_SEARCH|TAG_FLAG_MPD)

#define TAG_NON_DISP(XX, suffix, DISP)                             \
  XX(suffix, DISP, '\0', '\0', TAG_FLAGS_NONE)

#define TAG_FIELD_MPD_NUM_DECLS(XX)                                            \
  TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                                \
  TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')

#define TAG_SEARCH_MPD_DECLS(XX)                                               \
  TAG_SEARCH_MPD(XX, NAME, Filename)

#define TAG_EXTENDED_DECLS(XX)                                                 \
  TAG_NON_DISP(XX, MUSICBRAINZ_ARTISTID, Musicbrainz Artist Id)                \
  TAG_NON_DISP(XX, MUSICBRAINZ_ALBUMID, Musicbrainz Album Id)                  \
  TAG_NON_DISP(XX, MUSICBRAINZ_ALBUMARTISTID, Musicbrainz Album Artist Id)     \
  TAG_NON_DISP(XX, MUSICBRAINZ_TRACKID,  Musicbrainz Track Id)                 \
  TAG_NON_DISP(XX, MUSICBRAINZ_RELEASETRACKID, Musicbrainz Release Track Id)   \
  TAG_NON_DISP(XX, ORIGINAL_DATE, Original Date)                               \
  TAG_NON_DISP(XX, ARTIST_SORT, Artist Sort)                                   \
  TAG_NON_DISP(XX, ALBUM_ARTIST_SORT, Album Artist Sort)                       \
  TAG_NON_DISP(XX, ALBUM_SORT, Album Sort)                                     \
  TAG_NON_DISP(XX, LABEL, Label)                                               \
  TAG_NON_DISP(XX, MUSICBRAINZ_WORKID, Musicbrainz Work Id)                    \
  TAG_NON_DISP(XX, GROUPING, Grouping)                                         \
  TAG_NON_DISP(XX, WORK, Work)                                                 \
  TAG_NON_DISP(XX, CONDUCTOR, Conductor)                                       \
  TAG_NON_DISP(XX, COMPOSER_SORT, Composer Sort)                               \
  TAG_NON_DISP(XX, ENSEMBLE, Ensemble)                                         \
  TAG_NON_DISP(XX, MOVEMENT, Movement)                                         \
  TAG_NON_DISP(XX, MOVEMENTNUMBER, Movement Number)                            \
  TAG_NON_DISP(XX, LOCATION, Location)                                         \
  TAG_NON_DISP(XX, MOOD, Mood)                                                 \
  TAG_NON_DISP(XX, TITLE_SORT, Title Sort)                                     \
  TAG_NON_DISP(XX, MUSICBRAINZ_RELEASEGROUPID, Musicbrainz Release Group Id)   \
  TAG_NON_DISP(XX, SHOWMOVEMENT, Show Movement)                                \
  TAG_NON_DISP(XX, DISCSUBTITLE, Disc Subtitle)

#define TAG_NON_DISP_DECLS(XX)                                                 \
  TAG_NON_DISP(XX, UNKNOWN, Unknown)                                           \
  TAG_EXTENDED_DECLS(XX)

#define TAG_DEFS(XX)                                                           \
  TAG_NON_DISP(XX, UNKNOWN, Unknown)                                           \
  TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                       \
  TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                         \
  TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                           \
  TAG_FIELD_MPD(XX, TITLE, Title, 't')                                         \
  TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                                \
  TAG_SEARCH_MPD(XX, NAME, Filename)                                           \
  TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                         \
  TAG_FIELD_MPD(XX, DATE, Date, 'y')                                           \
  TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                                   \
  TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                                 \
  TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')                                     \
  TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                                  \
  TAG_EXTENDED_DECLS(XX)

#define TAG_FIELD_DEFS(XX)                                                     \
  TAG_FIELD_MPD(XX, TITLE, Title, 't')                                         \
  TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                       \
  TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                           \
  TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                         \
  TAG_FIELD_MPD(XX, DATE, Date, 'y')                                           \
  TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                                \
  TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                         \
  TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                                   \
  TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                                 \
  TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                                  \
  TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')

#define TAG_SONG_INFO_DEFS(XX) TAG_FIELD_DEFS(XX)

#define TAG_SEARCH_DEFS(XX)                                                    \
  TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                       \
  TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                           \
  TAG_FIELD_MPD(XX, TITLE, Title, 't')                                         \
  TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                         \
  TAG_SEARCH_MPD(XX, NAME, Filename)                                           \
  TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                                   \
  TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                                 \
  TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                         \
  TAG_FIELD_MPD(XX, DATE, Date, 'y')                                           \
  TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')

#define TAG_PRIMARY_DEFS(XX)                                                   \
  TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                       \
  TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                           \
  TAG_FIELD_MPD(XX, DATE, Date, 'y')                                           \
  TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                         \
  TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                                   \
  TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')

#define NCM_PRIMARY_TAG_DEFAULT_SETTINGS_NAME "artist"

#define TAGLIB_TAG_DEFS(XX) TAG_FIELD_DEFS(XX)

#define TAG_MPD_DEFS(XX)                                                       \
    TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                     \
    TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                         \
    TAG_FIELD_MPD(XX, DATE, Date, 'y')                                         \
    TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                       \
    TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                                 \
    TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                               \
    TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                       \
    TAG_FIELD_MPD(XX, TITLE, Title, 't')                                       \
    TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')                                   \
    TAG_FIELD_MPD_NUM_DECLS(XX)                                                \
    TAG_SEARCH_MPD_DECLS(XX)

#define TAG_SORT_DEFS(XX)                                                      \
    TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                     \
    TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                         \
    TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                       \
    TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                                \
    TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                              \
    TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                       \
    TAG_FIELD_MPD(XX, DATE, Date, 'y')                                         \
    TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                                 \
    TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                               \
    TAG_FIELD_MPD(XX, TITLE, Title, 't')

#define TAG_EDIT_PARSER_DEFS(XX)                                               \
    TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                     \
    TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                         \
    TAG_FIELD_MPD(XX, TITLE, Title, 't')                                       \
    TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                       \
    TAG_FIELD_MPD(XX, DATE, Date, 'y')                                         \
    TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                              \
    TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                       \
    TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                                 \
    TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                               \
    TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                                \
    TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')

#define TAG_COUNT_RECORD(suffix, DISP, CHAR, getter_char, flags) + 1


enum {
    TAG_DERIVED_NAME_CAP = 64,
    TAG_SETTINGS_NAME_CAP = TAG_DERIVED_NAME_CAP,
    TAGLIB_PROPERTY_CAP = TAG_DERIVED_NAME_CAP,
    TAGLIB_NAME_CAP = TAG_DERIVED_NAME_CAP,
    NCM_SONG_INFO_TAG_COUNT = 0
        TAG_SONG_INFO_DEFS(TAG_COUNT_RECORD),
    NCM_SEARCH_TAG_COUNT = 0
        TAG_SEARCH_DEFS(TAG_COUNT_RECORD),
    NCM_SEARCH_CONSTRAINT_COUNT = NCM_SEARCH_TAG_COUNT + 1,
    TAGLIB_TAG_COUNT = 0
        TAGLIB_TAG_DEFS(TAG_COUNT_RECORD),
    NCM_MPD_TAG_COUNT = 0
        TAG_MPD_DEFS(TAG_COUNT_RECORD),
    NCM_PRIMARY_TAG_COUNT = 0
        TAG_PRIMARY_DEFS(TAG_COUNT_RECORD),
    TAG_SORT_COUNT = 0
        TAG_SORT_DEFS(TAG_COUNT_RECORD),
    TAG_EDIT_PARSER_COUNT = 0
        TAG_EDIT_PARSER_DEFS(TAG_COUNT_RECORD),
};

#undef TAG_COUNT_RECORD

#define TAG_TYPE_ENUM_FIELD(suffix, DISP, CHAR, getter_char, flags)        \
  XX(CAT(TAG_, suffix))

#define TAG_TYPE_ENUM_FIELDS                                                   \
  TAG_DEFS(TAG_TYPE_ENUM_FIELD)

#define TAGS_FIELD_ENUM_FIELD(suffix, DISP, CHAR, getter_char, flags)      \
  XX(CAT(TAGS_FIELD_, suffix), DISP)

#define TAGS_FIELD_ENUM_FIELDS                                                 \
  TAG_FIELD_DEFS(TAGS_FIELD_ENUM_FIELD)

#define NCM_SONG_GETTER_RECORD_NONE(XX)                                        \
  XX(SONG_GETTER_NONE, none, '\0')

#define NCM_SONG_GETTER_RECORD_LENGTH(XX)                                      \
  XX(SONG_GETTER_LENGTH, Length, 'l')

#define NCM_SONG_GETTER_RECORD_DIRECTORY(XX)                                   \
  XX(SONG_GETTER_DIRECTORY, Directory, 'D')

#define NCM_SONG_GETTER_RECORD_NAME(XX)                                        \
  XX(SONG_GETTER_NAME, Filename, 'f')

#define NCM_SONG_GETTER_RECORD_URI(XX)                                         \
  XX(SONG_GETTER_URI, URI, 'F')

#define NCM_SONG_GETTER_RECORD_TRACK_NUMBER(XX)                                \
  XX(SONG_GETTER_TRACK_NUMBER, Track Number, 'n')

#define NCM_SONG_GETTER_RECORD_PRIORITY(XX)                                    \
  XX(SONG_GETTER_PRIORITY, Priority, 'P')

#define NCM_SONG_GETTER_NON_TAG_DEFS(XX)                                       \
  NCM_SONG_GETTER_RECORD_NONE(XX)                                              \
  NCM_SONG_GETTER_RECORD_LENGTH(XX)                                            \
  NCM_SONG_GETTER_RECORD_DIRECTORY(XX)                                         \
  NCM_SONG_GETTER_RECORD_NAME(XX)                                              \
  NCM_SONG_GETTER_RECORD_URI(XX)                                               \
  NCM_SONG_GETTER_RECORD_TRACK_NUMBER(XX)                                      \
  NCM_SONG_GETTER_RECORD_PRIORITY(XX)

#define NCM_SONG_GETTER_TAG_HEAD_DEFS(XX)                                      \
  TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                       \
  TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                           \
  TAG_FIELD_MPD(XX, TITLE, Title, 't')                                         \
  TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                         \
  TAG_FIELD_MPD(XX, DATE, Date, 'y')

#define NCM_SONG_GETTER_TAG_TAIL_DEFS(XX)                                      \
  TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                                \
  TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                         \
  TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                                   \
  TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                                 \
  TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                                  \
  TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')

#define NCM_SONG_GETTER_TAG_DEFS(XX)                                           \
  NCM_SONG_GETTER_TAG_HEAD_DEFS(XX)                                            \
  NCM_SONG_GETTER_TAG_TAIL_DEFS(XX)

#define NCM_SONG_GETTER_NON_TAG_ENUM_FIELD(getter, DISP, getter_char)          \
  XX(getter, DISP)

#define NCM_SONG_GETTER_TAG_ENUM_FIELD(suffix, DISP, CHAR,                 \
                                        getter_char, flags)                    \
  XX(CAT(SONG_GETTER_, suffix), DISP)

#define NCM_SONG_GETTER_ENUM_FIELDS                                            \
  NCM_SONG_GETTER_RECORD_NONE(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)              \
  NCM_SONG_GETTER_RECORD_LENGTH(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)            \
  NCM_SONG_GETTER_RECORD_DIRECTORY(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)         \
  NCM_SONG_GETTER_RECORD_NAME(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)              \
  NCM_SONG_GETTER_RECORD_URI(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)               \
  NCM_SONG_GETTER_TAG_HEAD_DEFS(NCM_SONG_GETTER_TAG_ENUM_FIELD)                \
  NCM_SONG_GETTER_RECORD_TRACK_NUMBER(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)      \
  NCM_SONG_GETTER_TAG_TAIL_DEFS(NCM_SONG_GETTER_TAG_ENUM_FIELD)                \
  NCM_SONG_GETTER_RECORD_PRIORITY(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)

#define ENUM_NAME NcmTagType
#define ENUM_PREFIX_ TAG_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS TAG_TYPE_ENUM_FIELDS
#include "cbase/xenums.c"

#define ENUM_NAME TagsField
#define ENUM_PREFIX_ TAGS_FIELD_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS TAGS_FIELD_ENUM_FIELDS
#include "cbase/xenums.c"

#define ENUM_NAME SongGetter
#define ENUM_PREFIX_ SONG_GETTER_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS NCM_SONG_GETTER_ENUM_FIELDS
#include "cbase/xenums.c"

static inline int32
ncm_tag_type_canonical_name_len(enum NcmTagType tag, char **out) {
    switch (tag) {
#define TAG_CANONICAL_NAME_CASE(suffix, DISP, CHAR,                        \
                                    getter_char, flags)                        \
    case CAT(TAG_, suffix):                                                    \
        *out = TAG_DISPLAY_NAME(DISP);                                         \
        return TAG_DISPLAY_NAME_LEN(DISP);

    TAG_DEFS(TAG_CANONICAL_NAME_CASE)

#undef TAG_CANONICAL_NAME_CASE
    case TAG_COUNT:
    default:
        *out = "";
        return 0;
    }
}

static inline int32
ncm_tag_type_display_name_len(enum NcmTagType tag, char **out) {
    switch (tag) {
#define TAG_DISPLAY_NAME_CASE(suffix, DISP, CHAR,                          \
                                  getter_char, flags)                          \
    case CAT(TAG_, suffix):                                                    \
        if (((flags) & TAG_FLAG_DISPLAY) == 0) {                               \
            *out = "";                                                         \
            return 0;                                                          \
        }                                                                      \
        *out = TAG_DISPLAY_NAME(DISP);                                         \
        return TAG_DISPLAY_NAME_LEN(DISP);

    TAG_DEFS(TAG_DISPLAY_NAME_CASE)

#undef TAG_DISPLAY_NAME_CASE
    case TAG_COUNT:
    default:
        *out = "";
        return 0;
    }
}

static inline int32
ncm_tag_type_settings_name_len(enum NcmTagType tag, char *out,
                               int32 cap) {
    switch ((int32)tag) {
#define TAG_SETTINGS_NAME_CASE(suffix, DISP, CHAR,                         \
                                   getter_char, flags)                         \
    case CAT(TAG_, suffix):                                                    \
        return ascii_normalize_lower_snake(out, cap,                           \
                                           TAG_DISPLAY_NAME(DISP),             \
                                           TAG_DISPLAY_NAME_LEN(DISP));

    TAG_PRIMARY_DEFS(TAG_SETTINGS_NAME_CASE)

#undef TAG_SETTINGS_NAME_CASE
    case TAG_COUNT:
    default:
        if (cap > 0) {
            out[0] = '\0';
        }
        return -1;
    }
}

static inline bool
ncm_tag_type_is_primary(enum NcmTagType tag) {
    switch ((int32)tag) {
#define TAG_IS_PRIMARY_CASE(suffix, DISP, CHAR, getter_char,               \
                                flags)                                         \
    case CAT(TAG_, suffix):                                                    \
        return true;

    TAG_PRIMARY_DEFS(TAG_IS_PRIMARY_CASE)

#undef TAG_IS_PRIMARY_CASE
    case TAG_COUNT:
    default:
        return false;
    }
}

static inline bool
ncm_tag_type_parse_settings_name(char *value, int32 value_len,
                                 enum NcmTagType *result) {
    char normalized[TAG_SETTINGS_NAME_CAP];
    enum NcmTagType parsed;
    int32 normalized_len;

    ASSERT(result != NULL);

    normalized_len = ascii_normalize_upper_snake(normalized,
                                                LENGTH(normalized),
                                                value, value_len);
    if (normalized_len <= 0) {
        return false;
    }
    if (BEGINS_WITH(normalized, normalized_len, "TAG_")) {
        return false;
    }

    parsed = TAG_parse(normalized, normalized_len);
    if (!ncm_tag_type_is_primary(parsed)) {
        return false;
    }

    *result = parsed;
    return true;
}

static inline enum NcmTagType
ncm_primary_tag_next(enum NcmTagType tag) {
    static enum NcmTagType tags[NCM_PRIMARY_TAG_COUNT] = {
#define NCM_PRIMARY_TAG_ARRAY_ENTRY(suffix, DISP, CHAR,                    \
                                    getter_char, flags)                        \
        CAT(TAG_, suffix),

        TAG_PRIMARY_DEFS(NCM_PRIMARY_TAG_ARRAY_ENTRY)

#undef NCM_PRIMARY_TAG_ARRAY_ENTRY
    };

    for (int32 i = 0; i < NCM_PRIMARY_TAG_COUNT; i += 1) {
        if (tags[i] == tag) {
            if (i + 1 >= NCM_PRIMARY_TAG_COUNT) {
                return tags[0];
            }
            return tags[i + 1];
        }
    }
    return tags[0];
}

static inline int32
ncm_song_getter_display_name_len(enum SongGetter getter, char **out) {
    return SONG_GETTER_alias_len(getter, out);
}

static inline char *
ncm_song_getter_display_name(enum SongGetter getter) {
    return SONG_GETTER_alias(getter);
}

static inline int32
ncm_song_getter_tag_name_len(enum SongGetter getter, char **out) {
    switch (getter) {
#define NCM_SONG_GETTER_TAG_NAME_CASE(suffix, DISP, CHAR, getter_char, flags) \
    case CAT(SONG_GETTER_, suffix):                                           \
        *out = TAG_DISPLAY_NAME(DISP);                                        \
        return TAG_DISPLAY_NAME_LEN(DISP);

    NCM_SONG_GETTER_TAG_DEFS(NCM_SONG_GETTER_TAG_NAME_CASE)

#undef NCM_SONG_GETTER_TAG_NAME_CASE
    case SONG_GETTER_NONE:
    case SONG_GETTER_LENGTH:
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_URI:
    case SONG_GETTER_TRACK_NUMBER:
    case SONG_GETTER_PRIORITY:
    case SONG_GETTER_COUNT:
    default:
        *out = "";
        return 0;
    }
}

static inline int32
ncm_song_getter_column_title_len(enum SongGetter getter, char **out) {
    switch (getter) {
    case SONG_GETTER_LENGTH:
        *out = "Time";
        return STRLIT_LEN("Time");
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_PRIORITY:
        return ncm_song_getter_display_name_len(getter, out);
    case SONG_GETTER_URI:
        *out = "Filepath";
        return STRLIT_LEN("Filepath");
    case SONG_GETTER_TRACK_NUMBER:
        *out = "Track";
        return STRLIT_LEN("Track");
#define NCM_SONG_GETTER_TAG_TITLE_CASE(suffix, DISP, CHAR, getter_char, flags) \
    case CAT(SONG_GETTER_, suffix):                                            \
        *out = TAG_DISPLAY_NAME(DISP);                                         \
        return TAG_DISPLAY_NAME_LEN(DISP);

    NCM_SONG_GETTER_TAG_DEFS(NCM_SONG_GETTER_TAG_TITLE_CASE)

#undef NCM_SONG_GETTER_TAG_TITLE_CASE
    case SONG_GETTER_NONE:
    case SONG_GETTER_COUNT:
    default:
        *out = "?";
        return STRLIT_LEN("?");
    }
}

static inline int32
ncm_song_getter_sort_label_len(enum SongGetter getter, char **out) {
    int32 len;

    if (getter == SONG_GETTER_URI) {
        return ncm_song_getter_display_name_len(SONG_GETTER_NAME, out);
    }

    len = ncm_song_getter_tag_name_len(getter, out);
    if (len > 0) {
        return len;
    }

    *out = "";
    return 0;
}

static inline int32
ncm_tag_type_taglib_property_len(enum NcmTagType tag, char *out, int32 cap) {
    int32 result;

    ASSERT(out != NULL);
    ASSERT(cap > 0);

    switch ((int32)tag) {
#define TAGLIB_PROPERTY_CASE(suffix, DISP, CHAR, getter_char, flags) \
    case CAT(TAG_, suffix):                                          \
        result = ascii_normalize_upper_compact(                      \
            out, cap, TAG_DISPLAY_NAME(DISP),                        \
            TAG_DISPLAY_NAME_LEN(DISP));                             \
        if (result < 0) {                                            \
            return result;                                           \
        }                                                            \
        if (((flags) & TAG_FLAG_TAGLIB_NUMBER) == 0) {               \
            return result;                                           \
        }                                                            \
        if (result + STRLIT_LEN("NUMBER") >= cap) {                  \
            out[0] = '\0';                                           \
            return -1;                                               \
        }                                                            \
        memcpy64(out + result, STRLIT("NUMBER") + 1);                \
        return result + STRLIT_LEN("NUMBER");

    TAGLIB_TAG_DEFS(TAGLIB_PROPERTY_CASE)

#undef TAGLIB_PROPERTY_CASE
    case TAG_COUNT:
    default:
        if (cap > 0) {
            out[0] = '\0';
        }
        return -1;
    }
}

static inline int32
ncm_tag_type_taglib_name_len(enum NcmTagType tag, char *out, int32 cap) {
    ASSERT(out != NULL);
    ASSERT(cap > 0);

    switch ((int32)tag) {
#define TAGLIB_NAME_CASE(suffix, DISP, CHAR,                               \
                              getter_char, flags)                          \
    case CAT(TAG_, suffix):                                                \
        return ascii_normalize_camel_compact(                              \
            out, cap, TAG_DISPLAY_NAME(DISP),                              \
            TAG_DISPLAY_NAME_LEN(DISP));

    TAGLIB_TAG_DEFS(TAGLIB_NAME_CASE)

#undef TAGLIB_NAME_CASE
    case TAG_COUNT:
    default:
        if (cap > 0) {
            out[0] = '\0';
        }
        return -1;
    }
}

static inline int32
ncm_tags_field_taglib_property_len(enum TagsField field, char *out, int32 cap) {
    ASSERT(out != NULL);
    ASSERT(cap > 0);

    switch (field) {
#define TAGLIB_FIELD_PROPERTY_CASE(suffix, DISP, CHAR, getter_char, flags)    \
    case CAT(TAGS_FIELD_, suffix):                                            \
        return ncm_tag_type_taglib_property_len(CAT(TAG_, suffix), out, cap);

    TAGLIB_TAG_DEFS(TAGLIB_FIELD_PROPERTY_CASE)

#undef TAGLIB_FIELD_PROPERTY_CASE
    case TAGS_FIELD_COUNT:
    default:
        if (cap > 0) {
            out[0] = '\0';
        }
        return -1;
    }
}

static inline char
ncm_tags_field_format_char(enum TagsField field) {
    switch (field) {
#define TAGS_FIELD_FORMAT_CHAR_CASE(suffix, DISP, CHAR, getter_char, flags) \
    case CAT(TAGS_FIELD_, suffix):                                          \
        return CHAR;

    TAG_FIELD_DEFS(TAGS_FIELD_FORMAT_CHAR_CASE)

#undef TAGS_FIELD_FORMAT_CHAR_CASE
    case TAGS_FIELD_COUNT:
    default:
        return '\0';
    }
}

static inline int32
ncm_tags_field_parser_name_len(enum TagsField field, char **out) {
    if (field == TAGS_FIELD_TRACK) {
        return ncm_song_getter_display_name_len(SONG_GETTER_TRACK_NUMBER, out);
    }
    if (field == TAGS_FIELD_COUNT) {
        *out = "";
        return 0;
    }

    return TAGS_FIELD_alias_len(field, out);
}

static inline char
ncm_song_getter_format_char(enum SongGetter getter) {
    switch (getter) {
#define NCM_SONG_GETTER_NON_TAG_CHAR_CASE(getter, DISP, getter_char)           \
    case getter:                                                               \
        return getter_char;
#define NCM_SONG_GETTER_TAG_CHAR_CASE(suffix, DISP, CHAR, getter_char, flags)  \
    case CAT(SONG_GETTER_, suffix):                                            \
        return getter_char;

    NCM_SONG_GETTER_RECORD_NONE(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_LENGTH(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_DIRECTORY(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_NAME(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_URI(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_TAG_HEAD_DEFS(NCM_SONG_GETTER_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_TRACK_NUMBER(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)
    NCM_SONG_GETTER_TAG_TAIL_DEFS(NCM_SONG_GETTER_TAG_CHAR_CASE)
    NCM_SONG_GETTER_RECORD_PRIORITY(NCM_SONG_GETTER_NON_TAG_CHAR_CASE)

#undef NCM_SONG_GETTER_NON_TAG_CHAR_CASE
#undef NCM_SONG_GETTER_TAG_CHAR_CASE
    case SONG_GETTER_COUNT:
    default:
        return '\0';
    }
}

#endif /* NCMPCPP2_H */
