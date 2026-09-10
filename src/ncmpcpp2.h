#if !defined(NCMPCPP2_H)
#define NCMPCPP2_H

#include "cbase.h"

#define NCM_TAG_META_FLAGS_NONE        ((uint32)0)
#define NCM_TAG_META_FLAG_DISPLAY      ((uint32)1 << 0)
#define NCM_TAG_META_FLAG_WRITABLE     ((uint32)1 << 1)
#define NCM_TAG_META_FLAG_SONG_INFO    ((uint32)1 << 2)
#define NCM_TAG_META_FLAG_SEARCH       ((uint32)1 << 3)
#define NCM_TAG_META_FLAG_PRIMARY      ((uint32)1 << 4)
#define NCM_TAG_META_FLAG_GETTER       ((uint32)1 << 5)
#define NCM_TAG_META_FLAG_TAGLIB       ((uint32)1 << 6)
#define NCM_TAG_META_FLAG_MPD          ((uint32)1 << 7)

#define NCM_TAG_META_FLAGS_FIELD                                             \
    (NCM_TAG_META_FLAG_DISPLAY|NCM_TAG_META_FLAG_WRITABLE                    \
     |NCM_TAG_META_FLAG_SONG_INFO|NCM_TAG_META_FLAG_GETTER                   \
     |NCM_TAG_META_FLAG_TAGLIB)

#define NCM_TAG_META_FLAGS_FIELD_SEARCH                                      \
    (NCM_TAG_META_FLAGS_FIELD|NCM_TAG_META_FLAG_SEARCH)

#define NCM_TAG_META_FLAGS_FIELD_PRIMARY                                     \
    (NCM_TAG_META_FLAGS_FIELD_SEARCH|NCM_TAG_META_FLAG_PRIMARY)

#define NCM_TAG_DISPLAY_NAME(display) #display
#define NCM_TAG_DISPLAY_NAME_LEN(display) STRLIT_LEN(#display)

#define NCM_TAG_RECORD_FULL(XX, tag, display, tag_char, field, getter,       \
                            getter_char, taglib_property, taglib_name,      \
                            settings_name, mpd, flags)                     \
    XX(tag, display, tag_char, field, getter, getter_char,                  \
       taglib_property, taglib_name, settings_name, mpd, flags)

#define NCM_TAG_RECORD_FIELD_PRIMARY_MPD(XX, suffix, display, tag_char,      \
                                         taglib_property, taglib_name,       \
                                         settings_name)                     \
    NCM_TAG_RECORD_FULL(XX, CAT(NCM_TAG_, suffix), display, tag_char,        \
                        suffix, suffix, tag_char, taglib_property,          \
                        taglib_name, settings_name, suffix,                 \
                        NCM_TAG_META_FLAGS_FIELD_PRIMARY                    \
                        |NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_FIELD_SEARCH_MPD(XX, suffix, display, tag_char,       \
                                        taglib_property, taglib_name)        \
    NCM_TAG_RECORD_FULL(XX, CAT(NCM_TAG_, suffix), display, tag_char,        \
                        suffix, suffix, tag_char, taglib_property,          \
                        taglib_name, NULL, suffix,                          \
                        NCM_TAG_META_FLAGS_FIELD_SEARCH                     \
                        |NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_FIELD_MPD(XX, suffix, display, tag_char, getter_char, \
                                 taglib_property, taglib_name)              \
    NCM_TAG_RECORD_FULL(XX, CAT(NCM_TAG_, suffix), display, tag_char,        \
                        suffix, suffix, getter_char, taglib_property,       \
                        taglib_name, NULL, suffix,                          \
                        NCM_TAG_META_FLAGS_FIELD|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_SEARCH_MPD(XX, suffix, display)                      \
    NCM_TAG_RECORD_FULL(XX, CAT(NCM_TAG_, suffix), display, '\0', NONE,     \
                        NONE, '\0', NULL, NULL, NULL, suffix,              \
                        NCM_TAG_META_FLAG_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_NON_DISPLAY(XX, suffix, display)                     \
    NCM_TAG_RECORD_FULL(XX, CAT(NCM_TAG_, suffix), display, '\0', NONE,     \
                        NONE, '\0', NULL, NULL, NULL, NONE,                \
                        NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_UNKNOWN_NON_DISPLAY(XX)                              \
    NCM_TAG_RECORD_NON_DISPLAY(XX, UNKNOWN, Unknown)

#define NCM_TAG_RECORD_UNKNOWN(XX)                                           \
    XX(NCM_TAG_UNKNOWN, Unknown, '\0', NONE, NONE, '\0', NULL, NULL,        \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ARTIST(XX)                                            \
    XX(NCM_TAG_ARTIST, Artist, 'a', ARTIST, ARTIST, 'a',                    \
       "ARTIST", "Artist", "artist", ARTIST,                              \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_ALBUM(XX)                                             \
    XX(NCM_TAG_ALBUM, Album, 'b', ALBUM, ALBUM, 'b',                        \
       "ALBUM", "Album", NULL, ALBUM,                                      \
       NCM_TAG_META_FLAGS_FIELD_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                      \
    XX(NCM_TAG_ALBUM_ARTIST, Album Artist, 'A', ALBUM_ARTIST,               \
       ALBUM_ARTIST, 'A', "ALBUMARTIST", "AlbumArtist",                    \
       "album_artist", ALBUM_ARTIST,                                        \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_TITLE(XX)                                             \
    XX(NCM_TAG_TITLE, Title, 't', TITLE, TITLE, 't',                         \
       "TITLE", "Title", NULL, TITLE,                                      \
       NCM_TAG_META_FLAGS_FIELD_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_TRACK(XX)                                             \
    XX(NCM_TAG_TRACK, Track, 'n', TRACK, TRACK, 'N',                         \
       "TRACKNUMBER", "Track", NULL, TRACK,                                \
       NCM_TAG_META_FLAGS_FIELD|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_NAME(XX)                                              \
    XX(NCM_TAG_NAME, Filename, '\0', NONE, NONE, '\0', NULL, NULL,          \
       NULL, NAME, NCM_TAG_META_FLAG_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_GENRE(XX)                                             \
    XX(NCM_TAG_GENRE, Genre, 'g', GENRE, GENRE, 'g',                         \
       "GENRE", "Genre", "genre", GENRE,                                  \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_DATE(XX)                                              \
    XX(NCM_TAG_DATE, Date, 'y', DATE, DATE, 'y',                             \
       "DATE", "Date", "date", DATE,                                      \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_COMPOSER(XX)                                          \
    XX(NCM_TAG_COMPOSER, Composer, 'c', COMPOSER, COMPOSER, 'c',            \
       "COMPOSER", "Composer", "composer", COMPOSER,                     \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_PERFORMER(XX)                                         \
    XX(NCM_TAG_PERFORMER, Performer, 'p', PERFORMER, PERFORMER, 'p',        \
       "PERFORMER", "Performer", "performer", PERFORMER,                 \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_COMMENT(XX)                                           \
    XX(NCM_TAG_COMMENT, Comment, 'C', COMMENT, COMMENT, 'C',                \
       "COMMENT", "Comment", NULL, COMMENT,                                \
       NCM_TAG_META_FLAGS_FIELD_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_DISC(XX)                                              \
    XX(NCM_TAG_DISC, Disc, 'd', DISC, DISC, 'd',                             \
       "DISCNUMBER", "Disc", NULL, DISC,                                   \
       NCM_TAG_META_FLAGS_FIELD|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_MUSICBRAINZ_ARTISTID(XX)                              \
    XX(NCM_TAG_MUSICBRAINZ_ARTISTID, Musicbrainz Artist Id, '\0',           \
       NONE, NONE, '\0', NULL, NULL, NULL, NONE,                            \
       NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_ALBUMID(XX)                               \
    XX(NCM_TAG_MUSICBRAINZ_ALBUMID, Musicbrainz Album Id, '\0',             \
       NONE, NONE, '\0', NULL, NULL, NULL, NONE,                            \
       NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_ALBUMARTISTID(XX)                         \
    XX(NCM_TAG_MUSICBRAINZ_ALBUMARTISTID,                                   \
       Musicbrainz Album Artist Id, '\0', NONE, NONE, '\0', NULL, NULL,    \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_TRACKID(XX)                               \
    XX(NCM_TAG_MUSICBRAINZ_TRACKID, Musicbrainz Track Id, '\0',             \
       NONE, NONE, '\0', NULL, NULL, NULL, NONE,                            \
       NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_RELEASETRACKID(XX)                        \
    XX(NCM_TAG_MUSICBRAINZ_RELEASETRACKID,                                  \
       Musicbrainz Release Track Id, '\0', NONE, NONE, '\0', NULL, NULL,   \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ORIGINAL_DATE(XX)                                     \
    XX(NCM_TAG_ORIGINAL_DATE, Original Date, '\0', NONE, NONE, '\0',        \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ARTIST_SORT(XX)                                       \
    XX(NCM_TAG_ARTIST_SORT, Artist Sort, '\0', NONE, NONE, '\0',            \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ALBUM_ARTIST_SORT(XX)                                 \
    XX(NCM_TAG_ALBUM_ARTIST_SORT, Album Artist Sort, '\0', NONE, NONE,      \
       '\0', NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ALBUM_SORT(XX)                                        \
    XX(NCM_TAG_ALBUM_SORT, Album Sort, '\0', NONE, NONE, '\0',              \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_LABEL(XX)                                             \
    XX(NCM_TAG_LABEL, Label, '\0', NONE, NONE, '\0', NULL, NULL,           \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_WORKID(XX)                                \
    XX(NCM_TAG_MUSICBRAINZ_WORKID, Musicbrainz Work Id, '\0',               \
       NONE, NONE, '\0', NULL, NULL, NULL, NONE,                            \
       NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_GROUPING(XX)                                          \
    XX(NCM_TAG_GROUPING, Grouping, '\0', NONE, NONE, '\0', NULL, NULL,      \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_WORK(XX)                                              \
    XX(NCM_TAG_WORK, Work, '\0', NONE, NONE, '\0', NULL, NULL, NULL,       \
       NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_CONDUCTOR(XX)                                         \
    XX(NCM_TAG_CONDUCTOR, Conductor, '\0', NONE, NONE, '\0', NULL,          \
       NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_COMPOSER_SORT(XX)                                     \
    XX(NCM_TAG_COMPOSER_SORT, Composer Sort, '\0', NONE, NONE, '\0',        \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ENSEMBLE(XX)                                          \
    XX(NCM_TAG_ENSEMBLE, Ensemble, '\0', NONE, NONE, '\0', NULL, NULL,      \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MOVEMENT(XX)                                          \
    XX(NCM_TAG_MOVEMENT, Movement, '\0', NONE, NONE, '\0', NULL, NULL,      \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MOVEMENTNUMBER(XX)                                    \
    XX(NCM_TAG_MOVEMENTNUMBER, Movement Number, '\0', NONE, NONE, '\0',     \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_LOCATION(XX)                                          \
    XX(NCM_TAG_LOCATION, Location, '\0', NONE, NONE, '\0', NULL, NULL,      \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MOOD(XX)                                              \
    XX(NCM_TAG_MOOD, Mood, '\0', NONE, NONE, '\0', NULL, NULL, NULL,        \
       NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_TITLE_SORT(XX)                                        \
    XX(NCM_TAG_TITLE_SORT, Title Sort, '\0', NONE, NONE, '\0',              \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_RELEASEGROUPID(XX)                        \
    XX(NCM_TAG_MUSICBRAINZ_RELEASEGROUPID,                                  \
       Musicbrainz Release Group Id, '\0', NONE, NONE, '\0', NULL, NULL,   \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_SHOWMOVEMENT(XX)                                      \
    XX(NCM_TAG_SHOWMOVEMENT, Show Movement, '\0', NONE, NONE, '\0',         \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_DISCSUBTITLE(XX)                                      \
    XX(NCM_TAG_DISCSUBTITLE, Disc Subtitle, '\0', NONE, NONE, '\0',         \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_DEFS(XX)                                                     \
    NCM_TAG_RECORD_UNKNOWN(XX)                                               \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM(XX)                                                 \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_TITLE(XX)                                                 \
    NCM_TAG_RECORD_TRACK(XX)                                                 \
    NCM_TAG_RECORD_NAME(XX)                                                  \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_DATE(XX)                                                  \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)                                             \
    NCM_TAG_RECORD_COMMENT(XX)                                               \
    NCM_TAG_RECORD_DISC(XX)                                                  \
    NCM_TAG_RECORD_MUSICBRAINZ_ARTISTID(XX)                                  \
    NCM_TAG_RECORD_MUSICBRAINZ_ALBUMID(XX)                                   \
    NCM_TAG_RECORD_MUSICBRAINZ_ALBUMARTISTID(XX)                             \
    NCM_TAG_RECORD_MUSICBRAINZ_TRACKID(XX)                                   \
    NCM_TAG_RECORD_MUSICBRAINZ_RELEASETRACKID(XX)                            \
    NCM_TAG_RECORD_ORIGINAL_DATE(XX)                                         \
    NCM_TAG_RECORD_ARTIST_SORT(XX)                                           \
    NCM_TAG_RECORD_ALBUM_ARTIST_SORT(XX)                                     \
    NCM_TAG_RECORD_ALBUM_SORT(XX)                                            \
    NCM_TAG_RECORD_LABEL(XX)                                                 \
    NCM_TAG_RECORD_MUSICBRAINZ_WORKID(XX)                                    \
    NCM_TAG_RECORD_GROUPING(XX)                                              \
    NCM_TAG_RECORD_WORK(XX)                                                  \
    NCM_TAG_RECORD_CONDUCTOR(XX)                                             \
    NCM_TAG_RECORD_COMPOSER_SORT(XX)                                         \
    NCM_TAG_RECORD_ENSEMBLE(XX)                                              \
    NCM_TAG_RECORD_MOVEMENT(XX)                                              \
    NCM_TAG_RECORD_MOVEMENTNUMBER(XX)                                        \
    NCM_TAG_RECORD_LOCATION(XX)                                              \
    NCM_TAG_RECORD_MOOD(XX)                                                  \
    NCM_TAG_RECORD_TITLE_SORT(XX)                                            \
    NCM_TAG_RECORD_MUSICBRAINZ_RELEASEGROUPID(XX)                            \
    NCM_TAG_RECORD_SHOWMOVEMENT(XX)                                          \
    NCM_TAG_RECORD_DISCSUBTITLE(XX)

#define NCM_TAG_FIELD_DEFS(XX)                                               \
    NCM_TAG_RECORD_TITLE(XX)                                                 \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_ALBUM(XX)                                                 \
    NCM_TAG_RECORD_DATE(XX)                                                  \
    NCM_TAG_RECORD_TRACK(XX)                                                 \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)                                             \
    NCM_TAG_RECORD_DISC(XX)                                                  \
    NCM_TAG_RECORD_COMMENT(XX)

#define NCM_TAG_SONG_INFO_DEFS(XX) NCM_TAG_FIELD_DEFS(XX)

#define NCM_TAG_SEARCH_DEFS(XX)                                              \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_TITLE(XX)                                                 \
    NCM_TAG_RECORD_ALBUM(XX)                                                 \
    NCM_TAG_RECORD_NAME(XX)                                                  \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)                                             \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_DATE(XX)                                                  \
    NCM_TAG_RECORD_COMMENT(XX)

#define NCM_TAG_PRIMARY_DEFS(XX)                                             \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_DATE(XX)                                                  \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)

#define NCM_TAG_SETTINGS_NAME_VALUE(tag, display, tag_char, field, getter,   \
                                    getter_char, taglib_property,            \
                                    taglib_name, settings_name, mpd, flags)  \
    settings_name

#define NCM_PRIMARY_TAG_DEFAULT_SETTINGS_NAME                                 \
    NCM_TAG_RECORD_ARTIST(NCM_TAG_SETTINGS_NAME_VALUE)

#define NCM_TAGLIB_TAG_DEFS(XX)                                              \
    NCM_TAG_RECORD_TITLE(XX)                                                 \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_ALBUM(XX)                                                 \
    NCM_TAG_RECORD_DATE(XX)                                                  \
    NCM_TAG_RECORD_TRACK(XX)                                                 \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)                                             \
    NCM_TAG_RECORD_DISC(XX)                                                  \
    NCM_TAG_RECORD_COMMENT(XX)

#define NCM_MPD_TAG_DEFS(XX)                                                 \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM(XX)                                                 \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_TITLE(XX)                                                 \
    NCM_TAG_RECORD_TRACK(XX)                                                 \
    NCM_TAG_RECORD_NAME(XX)                                                  \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_DATE(XX)                                                  \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)                                             \
    NCM_TAG_RECORD_COMMENT(XX)                                               \
    NCM_TAG_RECORD_DISC(XX)

#define NCM_TAG_SORT_DEFS(XX)                                                \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_ALBUM(XX)                                                 \
    NCM_TAG_RECORD_DISC(XX)                                                  \
    NCM_TAG_RECORD_TRACK(XX)                                                 \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_DATE(XX)                                                  \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)                                             \
    NCM_TAG_RECORD_TITLE(XX)

#define NCM_TAG_EDIT_PARSER_DEFS(XX)                                         \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_TITLE(XX)                                                 \
    NCM_TAG_RECORD_ALBUM(XX)                                                 \
    NCM_TAG_RECORD_DATE(XX)                                                  \
    NCM_TAG_RECORD_TRACK(XX)                                                 \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)                                             \
    NCM_TAG_RECORD_DISC(XX)                                                  \
    NCM_TAG_RECORD_COMMENT(XX)

#define NCM_TAG_COUNT_RECORD(tag, display, tag_char, field, getter,          \
                             getter_char, taglib_property, taglib_name,     \
                             settings_name, mpd, flags)                    \
    + 1

enum {
    NCM_SONG_INFO_TAG_COUNT = 0
        NCM_TAG_SONG_INFO_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_SEARCH_TAG_COUNT = 0
        NCM_TAG_SEARCH_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_SEARCH_CONSTRAINT_COUNT = NCM_SEARCH_TAG_COUNT + 1,
    NCM_TAGLIB_TAG_COUNT = 0
        NCM_TAGLIB_TAG_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_MPD_TAG_COUNT = 0
        NCM_MPD_TAG_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_PRIMARY_TAG_COUNT = 0
        NCM_TAG_PRIMARY_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_TAG_SORT_COUNT = 0
        NCM_TAG_SORT_DEFS(NCM_TAG_COUNT_RECORD),
    NCM_TAG_EDIT_PARSER_COUNT = 0
        NCM_TAG_EDIT_PARSER_DEFS(NCM_TAG_COUNT_RECORD),
};

#undef NCM_TAG_COUNT_RECORD

#define NCM_TAG_TYPE_ENUM_FIELD(tag, display, tag_char, field, getter,       \
                                getter_char, taglib_property, taglib_name,  \
                                settings_name, mpd, flags)                 \
    XX(tag)

#define NCM_TAG_TYPE_ENUM_FIELDS                                             \
    NCM_TAG_DEFS(NCM_TAG_TYPE_ENUM_FIELD)

#define NCM_TAGS_FIELD_ENUM_FIELD(tag, display, tag_char, field, getter,     \
                                  getter_char, taglib_property,             \
                                  taglib_name, settings_name, mpd, flags)   \
    XX(CAT(NCM_TAGS_FIELD_, field), display)

#define NCM_TAGS_FIELD_ENUM_FIELDS                                           \
    NCM_TAG_FIELD_DEFS(NCM_TAGS_FIELD_ENUM_FIELD)

#define NCM_SONG_GETTER_RECORD_NONE(XX)                                      \
    XX(SONG_GETTER_NONE, none, '\0')

#define NCM_SONG_GETTER_RECORD_LENGTH(XX)                                    \
    XX(SONG_GETTER_LENGTH, Length, 'l')

#define NCM_SONG_GETTER_RECORD_DIRECTORY(XX)                                 \
    XX(SONG_GETTER_DIRECTORY, Directory, 'D')

#define NCM_SONG_GETTER_RECORD_NAME(XX)                                      \
    XX(SONG_GETTER_NAME, Filename, 'f')

#define NCM_SONG_GETTER_RECORD_URI(XX)                                       \
    XX(SONG_GETTER_URI, URI, 'F')

#define NCM_SONG_GETTER_RECORD_TRACK_NUMBER(XX)                              \
    XX(SONG_GETTER_TRACK_NUMBER, Track Number, 'n')

#define NCM_SONG_GETTER_RECORD_PRIORITY(XX)                                  \
    XX(SONG_GETTER_PRIORITY, Priority, 'P')

#define NCM_SONG_GETTER_NON_TAG_DEFS(XX)                                     \
    NCM_SONG_GETTER_RECORD_NONE(XX)                                          \
    NCM_SONG_GETTER_RECORD_LENGTH(XX)                                        \
    NCM_SONG_GETTER_RECORD_DIRECTORY(XX)                                     \
    NCM_SONG_GETTER_RECORD_NAME(XX)                                          \
    NCM_SONG_GETTER_RECORD_URI(XX)                                           \
    NCM_SONG_GETTER_RECORD_TRACK_NUMBER(XX)                                  \
    NCM_SONG_GETTER_RECORD_PRIORITY(XX)

#define NCM_SONG_GETTER_TAG_HEAD_DEFS(XX)                                    \
    NCM_TAG_RECORD_ARTIST(XX)                                                \
    NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                          \
    NCM_TAG_RECORD_TITLE(XX)                                                 \
    NCM_TAG_RECORD_ALBUM(XX)                                                 \
    NCM_TAG_RECORD_DATE(XX)

#define NCM_SONG_GETTER_TAG_TAIL_DEFS(XX)                                    \
    NCM_TAG_RECORD_TRACK(XX)                                                 \
    NCM_TAG_RECORD_GENRE(XX)                                                 \
    NCM_TAG_RECORD_COMPOSER(XX)                                              \
    NCM_TAG_RECORD_PERFORMER(XX)                                             \
    NCM_TAG_RECORD_DISC(XX)                                                  \
    NCM_TAG_RECORD_COMMENT(XX)

#define NCM_SONG_GETTER_TAG_DEFS(XX)                                         \
    NCM_SONG_GETTER_TAG_HEAD_DEFS(XX)                                        \
    NCM_SONG_GETTER_TAG_TAIL_DEFS(XX)

#define NCM_SONG_GETTER_NON_TAG_ENUM_FIELD(getter, display, getter_char)     \
    XX(getter, display)

#define NCM_SONG_GETTER_TAG_ENUM_FIELD(tag, display, tag_char, field, getter,\
                                       getter_char, taglib_property,         \
                                       taglib_name, settings_name, mpd,      \
                                       flags)                                \
    XX(CAT(SONG_GETTER_, getter), display)

#define NCM_SONG_GETTER_ENUM_FIELDS                                          \
    NCM_SONG_GETTER_RECORD_NONE(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)          \
    NCM_SONG_GETTER_RECORD_LENGTH(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)        \
    NCM_SONG_GETTER_RECORD_DIRECTORY(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)     \
    NCM_SONG_GETTER_RECORD_NAME(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)          \
    NCM_SONG_GETTER_RECORD_URI(NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)           \
    NCM_SONG_GETTER_TAG_HEAD_DEFS(NCM_SONG_GETTER_TAG_ENUM_FIELD)            \
    NCM_SONG_GETTER_RECORD_TRACK_NUMBER(                                     \
        NCM_SONG_GETTER_NON_TAG_ENUM_FIELD)                                  \
    NCM_SONG_GETTER_TAG_TAIL_DEFS(NCM_SONG_GETTER_TAG_ENUM_FIELD)            \
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
#define NCM_TAG_CANONICAL_NAME_CASE(tag, display, tag_char, field, getter,   \
                                    getter_char, taglib_property,            \
                                    taglib_name, settings_name, mpd, flags)  \
    case tag:                                                                 \
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
#define NCM_TAG_DISPLAY_NAME_CASE(tag, display, tag_char, field, getter,      \
                                  getter_char, taglib_property,              \
                                  taglib_name, settings_name, mpd, flags)    \
    case tag:                                                                 \
        if (((flags) & NCM_TAG_META_FLAG_DISPLAY) == 0) {                     \
            *out = "";                                                        \
            return 0;                                                         \
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

static inline bool
ncm_tag_type_parse_settings_name(char *value, int32 value_len,
                                 enum NcmTagType *result) {
#define NCM_TAG_SETTINGS_NAME_CASE(tag, display, tag_char, field, getter,    \
                                   getter_char, taglib_property,             \
                                   taglib_name, settings_name, mpd, flags)   \
    if (STREQUAL(value, value_len, settings_name)) {                         \
        *result = tag;                                                       \
        return true;                                                         \
    }

    NCM_TAG_PRIMARY_DEFS(NCM_TAG_SETTINGS_NAME_CASE)

#undef NCM_TAG_SETTINGS_NAME_CASE
    return false;
}

static inline enum NcmTagType
ncm_primary_tag_next(enum NcmTagType tag) {
    static enum NcmTagType tags[NCM_PRIMARY_TAG_COUNT] = {
#define NCM_PRIMARY_TAG_ARRAY_ENTRY(tag, display, tag_char, field, getter,   \
                                    getter_char, taglib_property,            \
                                    taglib_name, settings_name, mpd, flags)  \
        tag,

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
#define NCM_SONG_GETTER_TAG_NAME_CASE(tag, display, tag_char, field, getter, \
                                      getter_char, taglib_property,          \
                                      taglib_name, settings_name, mpd,       \
                                      flags)                                 \
    case CAT(SONG_GETTER_, getter):                                           \
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
#define NCM_SONG_GETTER_TAG_TITLE_CASE(tag, display, tag_char, field, getter,\
                                       getter_char, taglib_property,         \
                                       taglib_name, settings_name, mpd,      \
                                       flags)                                \
    case CAT(SONG_GETTER_, getter):                                           \
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

static inline char *
ncm_tags_field_taglib_property(enum TagsField field) {
    switch (field) {
#define NCM_TAGLIB_FIELD_PROPERTY_CASE(tag, display, tag_char, field_id,     \
                                       getter, getter_char, taglib_property, \
                                       taglib_name, settings_name, mpd,      \
                                       flags)                                \
    case CAT(NCM_TAGS_FIELD_, field_id):                                     \
        return taglib_property;

    NCM_TAGLIB_TAG_DEFS(NCM_TAGLIB_FIELD_PROPERTY_CASE)

#undef NCM_TAGLIB_FIELD_PROPERTY_CASE
    case NCM_TAGS_FIELD_COUNT:
    default:
        return NULL;
    }
}

static inline char
ncm_tags_field_format_char(enum TagsField field) {
    switch (field) {
#define NCM_TAGS_FIELD_FORMAT_CHAR_CASE(tag, display, tag_char, field_id,    \
                                        getter, getter_char,                 \
                                        taglib_property, taglib_name,        \
                                        settings_name, mpd, flags)           \
    case CAT(NCM_TAGS_FIELD_, field_id):                                      \
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
#define NCM_SONG_GETTER_NON_TAG_CHAR_CASE(getter, display, getter_char)      \
    case getter:                                                               \
        return getter_char;
#define NCM_SONG_GETTER_TAG_CHAR_CASE(tag, display, tag_char, field, getter, \
                                      getter_char, taglib_property,          \
                                      taglib_name, settings_name, mpd,       \
                                      flags)                                 \
    case CAT(SONG_GETTER_, getter):                                            \
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
