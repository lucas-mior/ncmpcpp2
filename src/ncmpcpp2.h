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

#define NCM_TAG_RECORD_UNKNOWN(XX)                                           \
    XX(NCM_TAG_UNKNOWN, "", Unknown, '\0', NONE, NONE, '\0', NULL, NULL,   \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ARTIST(XX)                                            \
    XX(NCM_TAG_ARTIST, "Artist", Artist, 'a', ARTIST, ARTIST, 'a',          \
       "ARTIST", "Artist", "artist", ARTIST,                              \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_ALBUM(XX)                                             \
    XX(NCM_TAG_ALBUM, "Album", Album, 'b', ALBUM, ALBUM, 'b',               \
       "ALBUM", "Album", NULL, ALBUM,                                      \
       NCM_TAG_META_FLAGS_FIELD_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_ALBUM_ARTIST(XX)                                      \
    XX(NCM_TAG_ALBUM_ARTIST, "Album Artist", Album Artist, 'A',             \
       ALBUM_ARTIST, ALBUM_ARTIST, 'A', "ALBUMARTIST", "AlbumArtist",      \
       "album_artist", ALBUM_ARTIST,                                        \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_TITLE(XX)                                             \
    XX(NCM_TAG_TITLE, "Title", Title, 't', TITLE, TITLE, 't',               \
       "TITLE", "Title", NULL, TITLE,                                      \
       NCM_TAG_META_FLAGS_FIELD_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_TRACK(XX)                                             \
    XX(NCM_TAG_TRACK, "Track", Track, 'n', TRACK, TRACK, 'N',               \
       "TRACKNUMBER", "Track", NULL, TRACK,                                \
       NCM_TAG_META_FLAGS_FIELD|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_NAME(XX)                                              \
    XX(NCM_TAG_NAME, "", Filename, '\0', NONE, NONE, '\0', NULL, NULL,     \
       NULL, NAME, NCM_TAG_META_FLAG_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_GENRE(XX)                                             \
    XX(NCM_TAG_GENRE, "Genre", Genre, 'g', GENRE, GENRE, 'g',               \
       "GENRE", "Genre", "genre", GENRE,                                  \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_DATE(XX)                                              \
    XX(NCM_TAG_DATE, "Date", Date, 'y', DATE, DATE, 'y',                   \
       "DATE", "Date", "date", DATE,                                      \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_COMPOSER(XX)                                          \
    XX(NCM_TAG_COMPOSER, "Composer", Composer, 'c', COMPOSER, COMPOSER,    \
       'c', "COMPOSER", "Composer", "composer", COMPOSER,                 \
       NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_PERFORMER(XX)                                         \
    XX(NCM_TAG_PERFORMER, "Performer", Performer, 'p', PERFORMER,           \
       PERFORMER, 'p', "PERFORMER", "Performer", "performer",             \
       PERFORMER, NCM_TAG_META_FLAGS_FIELD_PRIMARY|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_COMMENT(XX)                                           \
    XX(NCM_TAG_COMMENT, "Comment", Comment, 'C', COMMENT, COMMENT, 'C',     \
       "COMMENT", "Comment", NULL, COMMENT,                                \
       NCM_TAG_META_FLAGS_FIELD_SEARCH|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_DISC(XX)                                              \
    XX(NCM_TAG_DISC, "Disc", Disc, 'd', DISC, DISC, 'd',                   \
       "DISCNUMBER", "Disc", NULL, DISC,                                   \
       NCM_TAG_META_FLAGS_FIELD|NCM_TAG_META_FLAG_MPD)

#define NCM_TAG_RECORD_MUSICBRAINZ_ARTISTID(XX)                              \
    XX(NCM_TAG_MUSICBRAINZ_ARTISTID, "", Musicbrainz Artist Id, '\0',      \
       NONE, NONE, '\0', NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_ALBUMID(XX)                               \
    XX(NCM_TAG_MUSICBRAINZ_ALBUMID, "", Musicbrainz Album Id, '\0',        \
       NONE, NONE, '\0', NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_ALBUMARTISTID(XX)                         \
    XX(NCM_TAG_MUSICBRAINZ_ALBUMARTISTID, "",                              \
       Musicbrainz Album Artist Id, '\0', NONE, NONE, '\0', NULL, NULL,     \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_TRACKID(XX)                               \
    XX(NCM_TAG_MUSICBRAINZ_TRACKID, "", Musicbrainz Track Id, '\0',        \
       NONE, NONE, '\0', NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_RELEASETRACKID(XX)                        \
    XX(NCM_TAG_MUSICBRAINZ_RELEASETRACKID, "",                             \
       Musicbrainz Release Track Id, '\0', NONE, NONE, '\0', NULL, NULL,    \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ORIGINAL_DATE(XX)                                     \
    XX(NCM_TAG_ORIGINAL_DATE, "", Original Date, '\0', NONE, NONE, '\0',   \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ARTIST_SORT(XX)                                       \
    XX(NCM_TAG_ARTIST_SORT, "", Artist Sort, '\0', NONE, NONE, '\0',       \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ALBUM_ARTIST_SORT(XX)                                 \
    XX(NCM_TAG_ALBUM_ARTIST_SORT, "", Album Artist Sort, '\0', NONE, NONE, \
       '\0', NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ALBUM_SORT(XX)                                        \
    XX(NCM_TAG_ALBUM_SORT, "", Album Sort, '\0', NONE, NONE, '\0',         \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_LABEL(XX)                                             \
    XX(NCM_TAG_LABEL, "", Label, '\0', NONE, NONE, '\0', NULL, NULL,       \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_WORKID(XX)                                \
    XX(NCM_TAG_MUSICBRAINZ_WORKID, "", Musicbrainz Work Id, '\0',          \
       NONE, NONE, '\0', NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_GROUPING(XX)                                          \
    XX(NCM_TAG_GROUPING, "", Grouping, '\0', NONE, NONE, '\0', NULL, NULL, \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_WORK(XX)                                              \
    XX(NCM_TAG_WORK, "", Work, '\0', NONE, NONE, '\0', NULL, NULL, NULL,   \
       NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_CONDUCTOR(XX)                                         \
    XX(NCM_TAG_CONDUCTOR, "", Conductor, '\0', NONE, NONE, '\0', NULL,     \
       NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_COMPOSER_SORT(XX)                                     \
    XX(NCM_TAG_COMPOSER_SORT, "", Composer Sort, '\0', NONE, NONE, '\0',   \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_ENSEMBLE(XX)                                          \
    XX(NCM_TAG_ENSEMBLE, "", Ensemble, '\0', NONE, NONE, '\0', NULL, NULL, \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MOVEMENT(XX)                                          \
    XX(NCM_TAG_MOVEMENT, "", Movement, '\0', NONE, NONE, '\0', NULL, NULL, \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MOVEMENTNUMBER(XX)                                    \
    XX(NCM_TAG_MOVEMENTNUMBER, "", Movement Number, '\0', NONE, NONE,      \
       '\0', NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_LOCATION(XX)                                          \
    XX(NCM_TAG_LOCATION, "", Location, '\0', NONE, NONE, '\0', NULL, NULL, \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MOOD(XX)                                              \
    XX(NCM_TAG_MOOD, "", Mood, '\0', NONE, NONE, '\0', NULL, NULL, NULL,   \
       NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_TITLE_SORT(XX)                                        \
    XX(NCM_TAG_TITLE_SORT, "", Title Sort, '\0', NONE, NONE, '\0',         \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_MUSICBRAINZ_RELEASEGROUPID(XX)                        \
    XX(NCM_TAG_MUSICBRAINZ_RELEASEGROUPID, "",                             \
       Musicbrainz Release Group Id, '\0', NONE, NONE, '\0', NULL, NULL,    \
       NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_SHOWMOVEMENT(XX)                                      \
    XX(NCM_TAG_SHOWMOVEMENT, "", Show Movement, '\0', NONE, NONE, '\0',    \
       NULL, NULL, NULL, NONE, NCM_TAG_META_FLAGS_NONE)

#define NCM_TAG_RECORD_DISCSUBTITLE(XX)                                      \
    XX(NCM_TAG_DISCSUBTITLE, "", Disc Subtitle, '\0', NONE, NONE, '\0',    \
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

#define NCM_TAG_TYPE_ENUM_FIELD(tag, name, alias, tag_char, field, getter,   \
                                getter_char, taglib_property, taglib_name,   \
                                settings_name, mpd, flags)                  \
    XX(tag)

#define NCM_TAG_TYPE_ENUM_FIELDS                                             \
    NCM_TAG_DEFS(NCM_TAG_TYPE_ENUM_FIELD)

#define NCM_TAGS_FIELD_ENUM_FIELD(tag, name, alias, tag_char, field, getter, \
                                  getter_char, taglib_property, taglib_name, \
                                  settings_name, mpd, flags)                \
    XX(CAT(NCM_TAGS_FIELD_, field), alias)

#define NCM_TAGS_FIELD_ENUM_FIELDS                                           \
    NCM_TAG_FIELD_DEFS(NCM_TAGS_FIELD_ENUM_FIELD)

#define NCM_SONG_GETTER_TAG_ENUM_FIELD(tag, name, alias, tag_char, field,    \
                                       getter, getter_char, taglib_property, \
                                       taglib_name, settings_name, mpd,      \
                                       flags)                                \
    XX(CAT(SONG_GETTER_, getter), alias)

#define NCM_SONG_GETTER_ENUM_FIELDS                                          \
    XX(SONG_GETTER_NONE, none)                                               \
    XX(SONG_GETTER_LENGTH, Length)                                           \
    XX(SONG_GETTER_DIRECTORY, Directory)                                     \
    XX(SONG_GETTER_NAME, Filename)                                           \
    XX(SONG_GETTER_URI, URI)                                                 \
    NCM_TAG_RECORD_ARTIST(NCM_SONG_GETTER_TAG_ENUM_FIELD)                    \
    NCM_TAG_RECORD_ALBUM_ARTIST(NCM_SONG_GETTER_TAG_ENUM_FIELD)              \
    NCM_TAG_RECORD_TITLE(NCM_SONG_GETTER_TAG_ENUM_FIELD)                     \
    NCM_TAG_RECORD_ALBUM(NCM_SONG_GETTER_TAG_ENUM_FIELD)                     \
    NCM_TAG_RECORD_DATE(NCM_SONG_GETTER_TAG_ENUM_FIELD)                      \
    XX(SONG_GETTER_TRACK_NUMBER, Track Number)                               \
    NCM_TAG_RECORD_TRACK(NCM_SONG_GETTER_TAG_ENUM_FIELD)                     \
    NCM_TAG_RECORD_GENRE(NCM_SONG_GETTER_TAG_ENUM_FIELD)                     \
    NCM_TAG_RECORD_COMPOSER(NCM_SONG_GETTER_TAG_ENUM_FIELD)                  \
    NCM_TAG_RECORD_PERFORMER(NCM_SONG_GETTER_TAG_ENUM_FIELD)                 \
    NCM_TAG_RECORD_DISC(NCM_SONG_GETTER_TAG_ENUM_FIELD)                      \
    NCM_TAG_RECORD_COMMENT(NCM_SONG_GETTER_TAG_ENUM_FIELD)                   \
    XX(SONG_GETTER_PRIORITY, Priority)

#endif /* NCMPCPP2_H */
