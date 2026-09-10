#if !defined(NCMPCPP2_H)
#define NCMPCPP2_H

#include "cbase.h"

#define NCM_TAG_META_FLAG_ENUM_FIELDS                      \
  XX(NCM_TAG_META_FLAG_DISPLAY)                            \
  XX(NCM_TAG_META_FLAG_WRITABLE)                           \
  XX(NCM_TAG_META_FLAG_SONG_INFO)                          \
  XX(NCM_TAG_META_FLAG_SEARCH)                             \
  XX(NCM_TAG_META_FLAG_GETTER)                             \
  XX(NCM_TAG_META_FLAG_TAGLIB)                             \
  XX(NCM_TAG_META_FLAG_MPD)                                \
  XX(NCM_TAG_META_FLAG_TAGLIB_NUMBER)                      \
  XX(NCM_TAG_META_FLAGS_FIELD,                             \
     NCM_TAG_META_FLAG_DISPLAY|NCM_TAG_META_FLAG_WRITABLE  \
     |NCM_TAG_META_FLAG_SONG_INFO|NCM_TAG_META_FLAG_GETTER \
     |NCM_TAG_META_FLAG_TAGLIB)                            \
  XX(NCM_TAG_META_FLAGS_FIELD_SEARCH,                      \
     NCM_TAG_META_FLAGS_FIELD|NCM_TAG_META_FLAG_SEARCH)

#define ENUM_NAME NcmTagMetaFlags
#define ENUM_PREFIX_ NCM_TAG_META_FLAGS_
#define ENUM_BITFLAGS 1
#define ENUM_FIELDS NCM_TAG_META_FLAG_ENUM_FIELDS
#include "cbase/xenums.c"

#define NCM_TAG_DISPLAY_NAME(display) #display
#define NCM_TAG_DISPLAY_NAME_LEN(display) STRLIT_LEN(#display)

static inline char
ncm_tag_name_ascii_lower(char c) {
    if ((c >= 'A') && (c <= 'Z')) {
        c = (char)(c - 'A' + 'a');
    }

    return c;
}

static inline char
ncm_tag_name_ascii_upper(char c) {
    if ((c >= 'a') && (c <= 'z')) {
        c = (char)(c - 'a' + 'A');
    }

    return c;
}

static inline int32
ncm_tag_name_to_lower_snake(char *out, int32 out_cap,
                            char *name, int32 name_len) {
    int32 written = 0;

    ASSERT(out != NULL);
    ASSERT(out_cap > 0);
    ASSERT(name_len >= 0);
    ASSERT((name != NULL) || (name_len == 0));

    for (int32 i = 0; i < name_len; i += 1) {
        if (written + 1 >= out_cap) {
            out[0] = '\0';
            return -1;
        }
        if (name[i] == ' ') {
            out[written] = '_';
        } else {
            out[written] = ncm_tag_name_ascii_lower(name[i]);
        }
        written += 1;
    }
    out[written] = '\0';
    return written;
}


static inline int32
ncm_tag_name_to_upper_snake(char *out, int32 out_cap,
                            char *name, int32 name_len) {
    int32 written = 0;

    ASSERT(out != NULL);
    ASSERT(out_cap > 0);
    ASSERT(name_len >= 0);
    ASSERT((name != NULL) || (name_len == 0));

    for (int32 i = 0; i < name_len; i += 1) {
        if (written + 1 >= out_cap) {
            out[0] = '\0';
            return -1;
        }
        if (name[i] == ' ') {
            out[written] = '_';
        } else {
            out[written] = ncm_tag_name_ascii_upper(name[i]);
        }
        written += 1;
    }
    out[written] = '\0';
    return written;
}

static inline int32
ncm_tag_name_to_upper_compact(char *out, int32 out_cap,
                              char *name, int32 name_len) {
    int32 written = 0;

    ASSERT(out != NULL);
    ASSERT(out_cap > 0);
    ASSERT(name_len >= 0);
    ASSERT((name != NULL) || (name_len == 0));

    for (int32 i = 0; i < name_len; i += 1) {
        if (name[i] == ' ') {
            continue;
        }
        if (written + 1 >= out_cap) {
            out[0] = '\0';
            return -1;
        }
        out[written] = ncm_tag_name_ascii_upper(name[i]);
        written += 1;
    }
    out[written] = '\0';
    return written;
}

static inline int32
ncm_tag_name_to_camel_compact(char *out, int32 out_cap,
                              char *name, int32 name_len) {
    bool capitalize = true;
    int32 written = 0;

    ASSERT(out != NULL);
    ASSERT(out_cap > 0);
    ASSERT(name_len >= 0);
    ASSERT((name != NULL) || (name_len == 0));

    for (int32 i = 0; i < name_len; i += 1) {
        if (name[i] == ' ') {
            capitalize = true;
            continue;
        }
        if (written + 1 >= out_cap) {
            out[0] = '\0';
            return -1;
        }
        if (capitalize) {
            out[written] = ncm_tag_name_ascii_upper(name[i]);
            capitalize = false;
        } else {
            out[written] = ncm_tag_name_ascii_lower(name[i]);
        }
        written += 1;
    }
    out[written] = '\0';
    return written;
}

#define NCM_TAG_FIELD_MPD(XX, suffix, display, tag_char)                     \
  XX(suffix, display, tag_char, tag_char,                                    \
     NCM_TAG_META_FLAGS_FIELD_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_FIELD_MPD_NUM(XX, suffix, display, tag_char, getter_char)    \
  XX(suffix, display, tag_char, getter_char,                                 \
     NCM_TAG_META_FLAGS_FIELD|NCM_TAG_META_FLAG_MPD                          \
     |NCM_TAG_META_FLAG_TAGLIB_NUMBER)

#define NCM_TAG_SEARCH_MPD(XX, suffix, display)                              \
  XX(suffix, display, '\0', '\0',                                            \
     NCM_TAG_META_FLAG_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_NON_DISP(XX, suffix, display)                             \
  XX(suffix, display, '\0', '\0', NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_FIELD_MPD_NUM_DECLS(XX)                                      \
  NCM_TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                          \
  NCM_TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')

#define NCM_TAG_SEARCH_MPD_DECLS(XX)                                         \
  NCM_TAG_SEARCH_MPD(XX, NAME, Filename)

#define NCM_TAG_EXTENDED_DECLS(XX)                                           \
  NCM_TAG_NON_DISP(XX, MUSICBRAINZ_ARTISTID,                            \
                           Musicbrainz Artist Id)                          \
  NCM_TAG_NON_DISP(XX, MUSICBRAINZ_ALBUMID,                             \
                           Musicbrainz Album Id)                           \
  NCM_TAG_NON_DISP(XX, MUSICBRAINZ_ALBUMARTISTID,                       \
                           Musicbrainz Album Artist Id)                    \
  NCM_TAG_NON_DISP(XX, MUSICBRAINZ_TRACKID,                             \
                           Musicbrainz Track Id)                           \
  NCM_TAG_NON_DISP(XX, MUSICBRAINZ_RELEASETRACKID,                      \
                           Musicbrainz Release Track Id)                   \
  NCM_TAG_NON_DISP(XX, ORIGINAL_DATE, Original Date)                    \
  NCM_TAG_NON_DISP(XX, ARTIST_SORT, Artist Sort)                        \
  NCM_TAG_NON_DISP(XX, ALBUM_ARTIST_SORT, Album Artist Sort)            \
  NCM_TAG_NON_DISP(XX, ALBUM_SORT, Album Sort)                          \
  NCM_TAG_NON_DISP(XX, LABEL, Label)                                    \
  NCM_TAG_NON_DISP(XX, MUSICBRAINZ_WORKID, Musicbrainz Work Id)         \
  NCM_TAG_NON_DISP(XX, GROUPING, Grouping)                              \
  NCM_TAG_NON_DISP(XX, WORK, Work)                                      \
  NCM_TAG_NON_DISP(XX, CONDUCTOR, Conductor)                            \
  NCM_TAG_NON_DISP(XX, COMPOSER_SORT, Composer Sort)                    \
  NCM_TAG_NON_DISP(XX, ENSEMBLE, Ensemble)                              \
  NCM_TAG_NON_DISP(XX, MOVEMENT, Movement)                              \
  NCM_TAG_NON_DISP(XX, MOVEMENTNUMBER, Movement Number)                 \
  NCM_TAG_NON_DISP(XX, LOCATION, Location)                              \
  NCM_TAG_NON_DISP(XX, MOOD, Mood)                                      \
  NCM_TAG_NON_DISP(XX, TITLE_SORT, Title Sort)                          \
  NCM_TAG_NON_DISP(XX, MUSICBRAINZ_RELEASEGROUPID,                      \
                           Musicbrainz Release Group Id)                   \
  NCM_TAG_NON_DISP(XX, SHOWMOVEMENT, Show Movement)                     \
  NCM_TAG_NON_DISP(XX, DISCSUBTITLE, Disc Subtitle)

#define NCM_TAG_NON_DISP_DECLS(XX)                                        \
  NCM_TAG_NON_DISP(XX, UNKNOWN, Unknown)                                \
  NCM_TAG_EXTENDED_DECLS(XX)

#define NCM_TAG_DEFS(XX)                                                     \
  NCM_TAG_NON_DISP(XX, UNKNOWN, Unknown)                                \
  NCM_TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                               \
  NCM_TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                 \
  NCM_TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                   \
  NCM_TAG_FIELD_MPD(XX, TITLE, Title, 't')                                 \
  NCM_TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                        \
  NCM_TAG_SEARCH_MPD(XX, NAME, Filename)                                   \
  NCM_TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                 \
  NCM_TAG_FIELD_MPD(XX, DATE, Date, 'y')                                   \
  NCM_TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                           \
  NCM_TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                         \
  NCM_TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')                             \
  NCM_TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                          \
  NCM_TAG_EXTENDED_DECLS(XX)

#define NCM_TAG_FIELD_DEFS(XX)                                               \
  NCM_TAG_FIELD_MPD(XX, TITLE, Title, 't')                                 \
  NCM_TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                               \
  NCM_TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                   \
  NCM_TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                 \
  NCM_TAG_FIELD_MPD(XX, DATE, Date, 'y')                                   \
  NCM_TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                        \
  NCM_TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                 \
  NCM_TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                           \
  NCM_TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                         \
  NCM_TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                          \
  NCM_TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')

#define NCM_TAG_SONG_INFO_DEFS(XX) NCM_TAG_FIELD_DEFS(XX)

#define NCM_TAG_SEARCH_DEFS(XX)                                              \
  NCM_TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                               \
  NCM_TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                   \
  NCM_TAG_FIELD_MPD(XX, TITLE, Title, 't')                                 \
  NCM_TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                 \
  NCM_TAG_SEARCH_MPD(XX, NAME, Filename)                                   \
  NCM_TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                           \
  NCM_TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                         \
  NCM_TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                 \
  NCM_TAG_FIELD_MPD(XX, DATE, Date, 'y')                                   \
  NCM_TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')

#define NCM_TAG_PRIMARY_DEFS(XX)                                             \
  NCM_TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                               \
  NCM_TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                   \
  NCM_TAG_FIELD_MPD(XX, DATE, Date, 'y')                                   \
  NCM_TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                 \
  NCM_TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                           \
  NCM_TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')

#define NCM_PRIMARY_TAG_DEFAULT_SETTINGS_NAME "artist"

#define NCM_TAGLIB_TAG_DEFS(XX) NCM_TAG_FIELD_DEFS(XX)

#define NCM_TAG_MPD_DEFS(XX)                                                 \
    NCM_TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                               \
    NCM_TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                   \
    NCM_TAG_FIELD_MPD(XX, DATE, Date, 'y')                                   \
    NCM_TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                 \
    NCM_TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                           \
    NCM_TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                         \
    NCM_TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                 \
    NCM_TAG_FIELD_MPD(XX, TITLE, Title, 't')                                 \
    NCM_TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')                             \
    NCM_TAG_FIELD_MPD_NUM_DECLS(XX)                                          \
    NCM_TAG_SEARCH_MPD_DECLS(XX)

#define NCM_TAG_SORT_DEFS(XX)                                                \
    NCM_TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                               \
    NCM_TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                   \
    NCM_TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                 \
    NCM_TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                          \
    NCM_TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                        \
    NCM_TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                 \
    NCM_TAG_FIELD_MPD(XX, DATE, Date, 'y')                                   \
    NCM_TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                           \
    NCM_TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                         \
    NCM_TAG_FIELD_MPD(XX, TITLE, Title, 't')

#define NCM_TAG_EDIT_PARSER_DEFS(XX)                                         \
    NCM_TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                               \
    NCM_TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                   \
    NCM_TAG_FIELD_MPD(XX, TITLE, Title, 't')                                 \
    NCM_TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                 \
    NCM_TAG_FIELD_MPD(XX, DATE, Date, 'y')                                   \
    NCM_TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                        \
    NCM_TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                 \
    NCM_TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                           \
    NCM_TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                         \
    NCM_TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                          \
    NCM_TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')

#define NCM_TAG_COUNT_RECORD(suffix, display, tag_char, getter_char,         \
                             flags)                                          \
    + 1


enum {
    NCM_TAG_DERIVED_NAME_CAP = 64,
    NCM_TAG_SETTINGS_NAME_CAP = NCM_TAG_DERIVED_NAME_CAP,
    NCM_TAGLIB_PROPERTY_CAP = NCM_TAG_DERIVED_NAME_CAP,
    NCM_TAGLIB_NAME_CAP = NCM_TAG_DERIVED_NAME_CAP,
    NCM_SONG_INFO_TAG_COUNT = 0
        NCM_TAG_SONG_INFO_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_SEARCH_TAG_COUNT = 0
        NCM_TAG_SEARCH_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_SEARCH_CONSTRAINT_COUNT = NCM_SEARCH_TAG_COUNT + 1,
    NCM_TAGLIB_TAG_COUNT = 0
        NCM_TAGLIB_TAG_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_MPD_TAG_COUNT = 0
        NCM_TAG_MPD_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_PRIMARY_TAG_COUNT = 0
        NCM_TAG_PRIMARY_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_TAG_SORT_COUNT = 0
        NCM_TAG_SORT_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_TAG_EDIT_PARSER_COUNT = 0
        NCM_TAG_EDIT_PARSER_DEFS(NCM_TAG_COUNT_RECORD),
};

#undef NCM_TAG_COUNT_RECORD

#define NCM_TAG_TYPE_ENUM_FIELD(suffix, display, tag_char, getter_char, flags) \
  XX(CAT(NCM_TAG_, suffix))

#define NCM_TAG_TYPE_ENUM_FIELDS                                               \
  NCM_TAG_DEFS(NCM_TAG_TYPE_ENUM_FIELD)

#define NCM_TAGS_FIELD_ENUM_FIELD(suffix, display, tag_char, getter_char, flags) \
  XX(CAT(NCM_TAGS_FIELD_, suffix), display)

#define NCM_TAGS_FIELD_ENUM_FIELDS                                             \
  NCM_TAG_FIELD_DEFS(NCM_TAGS_FIELD_ENUM_FIELD)

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
  NCM_SONG_GETTER_RECORD_NONE(XX)                                            \
  NCM_SONG_GETTER_RECORD_LENGTH(XX)                                          \
  NCM_SONG_GETTER_RECORD_DIRECTORY(XX)                                       \
  NCM_SONG_GETTER_RECORD_NAME(XX)                                            \
  NCM_SONG_GETTER_RECORD_URI(XX)                                             \
  NCM_SONG_GETTER_RECORD_TRACK_NUMBER(XX)                                    \
  NCM_SONG_GETTER_RECORD_PRIORITY(XX)

#define NCM_SONG_GETTER_TAG_HEAD_DEFS(XX)                                      \
  NCM_TAG_FIELD_MPD(XX, ARTIST, Artist, 'a')                                 \
  NCM_TAG_FIELD_MPD(XX, ALBUM_ARTIST, Album Artist, 'A')                     \
  NCM_TAG_FIELD_MPD(XX, TITLE, Title, 't')                                   \
  NCM_TAG_FIELD_MPD(XX, ALBUM, Album, 'b')                                   \
  NCM_TAG_FIELD_MPD(XX, DATE, Date, 'y')

#define NCM_SONG_GETTER_TAG_TAIL_DEFS(XX)                                      \
  NCM_TAG_FIELD_MPD_NUM(XX, TRACK, Track, 'n', 'N')                          \
  NCM_TAG_FIELD_MPD(XX, GENRE, Genre, 'g')                                   \
  NCM_TAG_FIELD_MPD(XX, COMPOSER, Composer, 'c')                             \
  NCM_TAG_FIELD_MPD(XX, PERFORMER, Performer, 'p')                           \
  NCM_TAG_FIELD_MPD_NUM(XX, DISC, Disc, 'd', 'd')                            \
  NCM_TAG_FIELD_MPD(XX, COMMENT, Comment, 'C')

#define NCM_SONG_GETTER_TAG_DEFS(XX)                                           \
  NCM_SONG_GETTER_TAG_HEAD_DEFS(XX)                                          \
  NCM_SONG_GETTER_TAG_TAIL_DEFS(XX)

#define NCM_SONG_GETTER_NON_TAG_ENUM_FIELD(getter, display, getter_char)       \
  XX(getter, display)

#define NCM_SONG_GETTER_TAG_ENUM_FIELD(suffix, display, tag_char,              \
                                        getter_char, flags)                    \
  XX(CAT(SONG_GETTER_, suffix), display)

#define NCM_SONG_GETTER_ENUM_FIELDS                                            \
  NCM_SONG_GETTER_RECORD_NONE(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)            \
  NCM_SONG_GETTER_RECORD_LENGTH(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)          \
  NCM_SONG_GETTER_RECORD_DIRECTORY(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)       \
  NCM_SONG_GETTER_RECORD_NAME(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)            \
  NCM_SONG_GETTER_RECORD_URI(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)             \
  NCM_SONG_GETTER_TAG_HEAD_DEFS(NCM_SONG_GETTER_TAG_ENUM_FIELD)              \
  NCM_SONG_GETTER_RECORD_TRACK_NUMBER(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)    \
  NCM_SONG_GETTER_TAG_TAIL_DEFS(NCM_SONG_GETTER_TAG_ENUM_FIELD)              \
  NCM_SONG_GETTER_RECORD_PRIORITY(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)

#define ENUM_NAME NcmTagType
#define ENUM_PREFIX_ NCM_TAG_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS NCM_TAG_TYPE_ENUM_FIELDS
#include "cbase/xenums.c"

#define ENUM_NAME TagsField
#define ENUM_PREFIX_ NCM_TAGS_FIELD_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS NCM_TAGS_FIELD_ENUM_FIELDS
#include "cbase/xenums.c"

#define ENUM_NAME SongGetter
#define ENUM_PREFIX_ SONG_GETTER_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS NCM_SONG_GETTER_ENUM_FIELDS
#include "cbase/xenums.c"

static inline int32
ncm_tag_type_canonical_name_len(enum NcmTagType tag, char **out) {
    switch (tag) {
#define NCM_TAG_CANONICAL_NAME_CASE(suffix, display, tag_char,                 \
                                    getter_char, flags)                        \
    case CAT(NCM_TAG_, suffix):                                                \
        *out = NCM_TAG_DISPLAY_NAME(display);                                  \
        return NCM_TAG_DISPLAY_NAME_LEN(display);

    NCM_TAG_DEFS(NCM_TAG_CANONICAL_NAME_CASE)

#undef NCM_TAG_CANONICAL_NAME_CASE
    case NCM_TAG_COUNT:
    default:
        *out = "";
        return 0;
    }
}

static inline int32
ncm_tag_type_display_name_len(enum NcmTagType tag, char **out) {
    switch (tag) {
#define NCM_TAG_DISPLAY_NAME_CASE(suffix, display, tag_char,                   \
                                  getter_char, flags)                          \
    case CAT(NCM_TAG_, suffix):                                                \
        if (((flags) & NCM_TAG_META_FLAG_DISPLAY) == 0) {                      \
            *out = "";                                                         \
            return 0;                                                          \
        }                                                                      \
        *out = NCM_TAG_DISPLAY_NAME(display);                                  \
        return NCM_TAG_DISPLAY_NAME_LEN(display);

    NCM_TAG_DEFS(NCM_TAG_DISPLAY_NAME_CASE)

#undef NCM_TAG_DISPLAY_NAME_CASE
    case NCM_TAG_COUNT:
    default:
        *out = "";
        return 0;
    }
}

static inline int32
ncm_tag_type_settings_name_len(enum NcmTagType tag, char *out,
                               int32 out_cap) {
    switch ((int32)tag) {
#define NCM_TAG_SETTINGS_NAME_CASE(suffix, display, tag_char,                  \
                                   getter_char, flags)                         \
    case CAT(NCM_TAG_, suffix):                                                \
        return ncm_tag_name_to_lower_snake(out, out_cap,                       \
                                           NCM_TAG_DISPLAY_NAME(display),      \
                                           NCM_TAG_DISPLAY_NAME_LEN(display));

    NCM_TAG_PRIMARY_DEFS(NCM_TAG_SETTINGS_NAME_CASE)

#undef NCM_TAG_SETTINGS_NAME_CASE
    case NCM_TAG_COUNT:
    default:
        if (out_cap > 0) {
            out[0] = '\0';
        }
        return -1;
    }
}

static inline bool
ncm_tag_type_is_primary(enum NcmTagType tag) {
    switch ((int32)tag) {
#define NCM_TAG_IS_PRIMARY_CASE(suffix, display, tag_char, getter_char,        \
                                flags)                                         \
    case CAT(NCM_TAG_, suffix):                                                \
        return true;

    NCM_TAG_PRIMARY_DEFS(NCM_TAG_IS_PRIMARY_CASE)

#undef NCM_TAG_IS_PRIMARY_CASE
    case NCM_TAG_COUNT:
    default:
        return false;
    }
}

static inline bool
ncm_tag_type_parse_settings_name(char *value, int32 value_len,
                                 enum NcmTagType *result) {
    char normalized[NCM_TAG_SETTINGS_NAME_CAP];
    enum NcmTagType parsed;
    int32 normalized_len;

    ASSERT(result != NULL);

    normalized_len = ncm_tag_name_to_upper_snake(normalized,
                                                LENGTH(normalized),
                                                value, value_len);
    if (normalized_len <= 0) {
        return false;
    }
    if (BEGINS_WITH(normalized, normalized_len, "NCM_TAG_")) {
        return false;
    }

    parsed = NCM_TAG_parse(normalized, normalized_len);
    if (!ncm_tag_type_is_primary(parsed)) {
        return false;
    }

    *result = parsed;
    return true;
}

static inline enum NcmTagType
ncm_primary_tag_next(enum NcmTagType tag) {
    static enum NcmTagType tags[NCM_PRIMARY_TAG_COUNT] = {
#define NCM_PRIMARY_TAG_ARRAY_ENTRY(suffix, display, tag_char,                 \
                                    getter_char, flags)                        \
        CAT(NCM_TAG_, suffix),

        NCM_TAG_PRIMARY_DEFS(NCM_PRIMARY_TAG_ARRAY_ENTRY)

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
#define NCM_SONG_GETTER_TAG_NAME_CASE(suffix, display, tag_char,               \
                                       getter_char, flags)                     \
    case CAT(SONG_GETTER_, suffix):                                            \
        *out = NCM_TAG_DISPLAY_NAME(display);                                  \
        return NCM_TAG_DISPLAY_NAME_LEN(display);

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
#define NCM_SONG_GETTER_TAG_TITLE_CASE(suffix, display, tag_char,              \
                                        getter_char, flags)                    \
    case CAT(SONG_GETTER_, suffix):                                            \
        *out = NCM_TAG_DISPLAY_NAME(display);                                  \
        return NCM_TAG_DISPLAY_NAME_LEN(display);

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
ncm_tag_type_taglib_property_len(enum NcmTagType tag, char *out,
                                 int32 out_cap) {
    int32 result;

    ASSERT(out != NULL);
    ASSERT(out_cap > 0);

    switch ((int32)tag) {
#define NCM_TAGLIB_PROPERTY_CASE(suffix, display, tag_char,                    \
                                  getter_char, flags)                          \
    case CAT(NCM_TAG_, suffix):                                                \
        result = ncm_tag_name_to_upper_compact(                                \
            out, out_cap, NCM_TAG_DISPLAY_NAME(display),                       \
            NCM_TAG_DISPLAY_NAME_LEN(display));                                \
        if (result < 0) {                                                      \
            return result;                                                     \
        }                                                                      \
        if (((flags) & NCM_TAG_META_FLAG_TAGLIB_NUMBER) == 0) {                \
            return result;                                                     \
        }                                                                      \
        if (result + STRLIT_LEN("NUMBER") >= out_cap) {                        \
            out[0] = '\0';                                                     \
            return -1;                                                         \
        }                                                                      \
        memcpy64(out + result, STRLIT("NUMBER") + 1);                          \
        return result + STRLIT_LEN("NUMBER");

    NCM_TAGLIB_TAG_DEFS(NCM_TAGLIB_PROPERTY_CASE)

#undef NCM_TAGLIB_PROPERTY_CASE
    case NCM_TAG_COUNT:
    default:
        if (out_cap > 0) {
            out[0] = '\0';
        }
        return -1;
    }
}

static inline int32
ncm_tag_type_taglib_name_len(enum NcmTagType tag, char *out,
                             int32 out_cap) {
    ASSERT(out != NULL);
    ASSERT(out_cap > 0);

    switch ((int32)tag) {
#define NCM_TAGLIB_NAME_CASE(suffix, display, tag_char,                        \
                              getter_char, flags)                              \
    case CAT(NCM_TAG_, suffix):                                                \
        return ncm_tag_name_to_camel_compact(                                  \
            out, out_cap, NCM_TAG_DISPLAY_NAME(display),                       \
            NCM_TAG_DISPLAY_NAME_LEN(display));

    NCM_TAGLIB_TAG_DEFS(NCM_TAGLIB_NAME_CASE)

#undef NCM_TAGLIB_NAME_CASE
    case NCM_TAG_COUNT:
    default:
        if (out_cap > 0) {
            out[0] = '\0';
        }
        return -1;
    }
}

static inline int32
ncm_tags_field_taglib_property_len(enum TagsField field, char *out,
                                   int32 out_cap) {
    ASSERT(out != NULL);
    ASSERT(out_cap > 0);

    switch (field) {
#define NCM_TAGLIB_FIELD_PROPERTY_CASE(suffix, display, tag_char,              \
                                        getter_char, flags)                    \
    case CAT(NCM_TAGS_FIELD_, suffix):                                         \
        return ncm_tag_type_taglib_property_len(CAT(NCM_TAG_, suffix),         \
                                                out, out_cap);

    NCM_TAGLIB_TAG_DEFS(NCM_TAGLIB_FIELD_PROPERTY_CASE)

#undef NCM_TAGLIB_FIELD_PROPERTY_CASE
    case NCM_TAGS_FIELD_COUNT:
    default:
        if (out_cap > 0) {
            out[0] = '\0';
        }
        return -1;
    }
}

static inline char
ncm_tags_field_format_char(enum TagsField field) {
    switch (field) {
#define NCM_TAGS_FIELD_FORMAT_CHAR_CASE(suffix, display, tag_char,             \
                                         getter_char, flags)                   \
    case CAT(NCM_TAGS_FIELD_, suffix):                                         \
        return tag_char;

    NCM_TAG_FIELD_DEFS(NCM_TAGS_FIELD_FORMAT_CHAR_CASE)

#undef NCM_TAGS_FIELD_FORMAT_CHAR_CASE
    case NCM_TAGS_FIELD_COUNT:
    default:
        return '\0';
    }
}

static inline int32
ncm_tags_field_parser_name_len(enum TagsField field, char **out) {
    if (field == NCM_TAGS_FIELD_TRACK) {
        return ncm_song_getter_display_name_len(SONG_GETTER_TRACK_NUMBER,
                                                out);
    }
    if (field == NCM_TAGS_FIELD_COUNT) {
        *out = "";
        return 0;
    }

    return NCM_TAGS_FIELD_alias_len(field, out);
}

static inline char
ncm_song_getter_format_char(enum SongGetter getter) {
    switch (getter) {
#define NCM_SONG_GETTER_NON_TAG_CHAR_CASE(getter, display, getter_char)        \
    case getter:                                                               \
        return getter_char;
#define NCM_SONG_GETTER_TAG_CHAR_CASE(suffix, display, tag_char,               \
                                       getter_char, flags)                     \
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
