#if !defined(NCM_TYPE_CONVERSIONS_C)
#define NCM_TYPE_CONVERSIONS_C

#include "cbase.h"

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

char *
ncm_tag_type_name(enum NcmTagType tag) {
    switch (tag) {
    case NCM_TAG_ARTIST:
        return "Artist";
    case NCM_TAG_ALBUM:
        return "Album";
    case NCM_TAG_ALBUM_ARTIST:
        return "Album Artist";
    case NCM_TAG_TITLE:
        return "Title";
    case NCM_TAG_TRACK:
        return "Track";
    case NCM_TAG_GENRE:
        return "Genre";
    case NCM_TAG_DATE:
        return "Date";
    case NCM_TAG_COMPOSER:
        return "Composer";
    case NCM_TAG_PERFORMER:
        return "Performer";
    case NCM_TAG_COMMENT:
        return "Comment";
    case NCM_TAG_DISC:
        return "Disc";
    case NCM_TAG_UNKNOWN:
    case NCM_TAG_NAME:
    case NCM_TAG_MUSICBRAINZ_ARTISTID:
    case NCM_TAG_MUSICBRAINZ_ALBUMID:
    case NCM_TAG_MUSICBRAINZ_ALBUMARTISTID:
    case NCM_TAG_MUSICBRAINZ_TRACKID:
    case NCM_TAG_MUSICBRAINZ_RELEASETRACKID:
    case NCM_TAG_ORIGINAL_DATE:
    case NCM_TAG_ARTIST_SORT:
    case NCM_TAG_ALBUM_ARTIST_SORT:
    case NCM_TAG_ALBUM_SORT:
    case NCM_TAG_LABEL:
    case NCM_TAG_MUSICBRAINZ_WORKID:
    case NCM_TAG_GROUPING:
    case NCM_TAG_WORK:
    case NCM_TAG_CONDUCTOR:
    case NCM_TAG_COMPOSER_SORT:
    case NCM_TAG_ENSEMBLE:
    case NCM_TAG_MOVEMENT:
    case NCM_TAG_MOVEMENTNUMBER:
    case NCM_TAG_LOCATION:
    case NCM_TAG_MOOD:
    case NCM_TAG_TITLE_SORT:
    case NCM_TAG_MUSICBRAINZ_RELEASEGROUPID:
    case NCM_TAG_SHOWMOVEMENT:
    case NCM_TAG_DISCSUBTITLE:
    case NCM_TAG_COUNT:
    default:
        return "";
    }
}

enum NcmTagType
ncm_char_to_tag_type(char c) {
    switch (c) {
    case 'a':
        return NCM_TAG_ARTIST;
    case 'A':
        return NCM_TAG_ALBUM_ARTIST;
    case 't':
        return NCM_TAG_TITLE;
    case 'b':
        return NCM_TAG_ALBUM;
    case 'y':
        return NCM_TAG_DATE;
    case 'n':
        return NCM_TAG_TRACK;
    case 'g':
        return NCM_TAG_GENRE;
    case 'c':
        return NCM_TAG_COMPOSER;
    case 'p':
        return NCM_TAG_PERFORMER;
    case 'd':
        return NCM_TAG_DISC;
    case 'C':
        return NCM_TAG_COMMENT;
    default:
        return NCM_TAG_UNKNOWN;
    }
}

enum SongGetter
ncm_song_getter_from_char(char c) {
    switch (c) {
    case 'l':
        return SONG_GETTER_LENGTH;
    case 'D':
        return SONG_GETTER_DIRECTORY;
    case 'f':
        return SONG_GETTER_NAME;
    case 'F':
        return SONG_GETTER_URI;
    case 'a':
        return SONG_GETTER_ARTIST;
    case 'A':
        return SONG_GETTER_ALBUM_ARTIST;
    case 't':
        return SONG_GETTER_TITLE;
    case 'b':
        return SONG_GETTER_ALBUM;
    case 'y':
        return SONG_GETTER_DATE;
    case 'n':
        return SONG_GETTER_TRACK_NUMBER;
    case 'N':
        return SONG_GETTER_TRACK;
    case 'g':
        return SONG_GETTER_GENRE;
    case 'c':
        return SONG_GETTER_COMPOSER;
    case 'p':
        return SONG_GETTER_PERFORMER;
    case 'd':
        return SONG_GETTER_DISC;
    case 'C':
        return SONG_GETTER_COMMENT;
    case 'P':
        return SONG_GETTER_PRIORITY;
    default:
        return SONG_GETTER_NONE;
    }
}

enum NcmTagType
ncm_song_getter_to_tag_type(enum SongGetter getter) {
    switch (getter) {
    case SONG_GETTER_ARTIST:
        return NCM_TAG_ARTIST;
    case SONG_GETTER_TITLE:
        return NCM_TAG_TITLE;
    case SONG_GETTER_ALBUM:
        return NCM_TAG_ALBUM;
    case SONG_GETTER_ALBUM_ARTIST:
        return NCM_TAG_ALBUM_ARTIST;
    case SONG_GETTER_TRACK:
        return NCM_TAG_TRACK;
    case SONG_GETTER_DATE:
        return NCM_TAG_DATE;
    case SONG_GETTER_GENRE:
        return NCM_TAG_GENRE;
    case SONG_GETTER_COMPOSER:
        return NCM_TAG_COMPOSER;
    case SONG_GETTER_PERFORMER:
        return NCM_TAG_PERFORMER;
    case SONG_GETTER_COMMENT:
        return NCM_TAG_COMMENT;
    case SONG_GETTER_DISC:
        return NCM_TAG_DISC;
    case SONG_GETTER_NONE:
    case SONG_GETTER_LENGTH:
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_URI:
    case SONG_GETTER_TRACK_NUMBER:
    case SONG_GETTER_PRIORITY:
    case SONG_GETTER_COUNT:
    default:
        return NCM_TAG_UNKNOWN;
    }
}

enum TagsField
ncm_tags_field_from_char(char c) {
    return ncm_tags_field_from_tag_type(ncm_char_to_tag_type(c));
}

enum TagsField
ncm_tags_field_from_tag_type(enum NcmTagType tag) {
    switch (tag) {
    case NCM_TAG_TITLE:
        return NCM_TAGS_FIELD_TITLE;
    case NCM_TAG_ARTIST:
        return NCM_TAGS_FIELD_ARTIST;
    case NCM_TAG_ALBUM_ARTIST:
        return NCM_TAGS_FIELD_ALBUM_ARTIST;
    case NCM_TAG_ALBUM:
        return NCM_TAGS_FIELD_ALBUM;
    case NCM_TAG_DATE:
        return NCM_TAGS_FIELD_DATE;
    case NCM_TAG_TRACK:
        return NCM_TAGS_FIELD_TRACK;
    case NCM_TAG_GENRE:
        return NCM_TAGS_FIELD_GENRE;
    case NCM_TAG_COMPOSER:
        return NCM_TAGS_FIELD_COMPOSER;
    case NCM_TAG_PERFORMER:
        return NCM_TAGS_FIELD_PERFORMER;
    case NCM_TAG_DISC:
        return NCM_TAGS_FIELD_DISC;
    case NCM_TAG_COMMENT:
        return NCM_TAGS_FIELD_COMMENT;
    case NCM_TAG_UNKNOWN:
    case NCM_TAG_NAME:
    case NCM_TAG_MUSICBRAINZ_ARTISTID:
    case NCM_TAG_MUSICBRAINZ_ALBUMID:
    case NCM_TAG_MUSICBRAINZ_ALBUMARTISTID:
    case NCM_TAG_MUSICBRAINZ_TRACKID:
    case NCM_TAG_MUSICBRAINZ_RELEASETRACKID:
    case NCM_TAG_ORIGINAL_DATE:
    case NCM_TAG_ARTIST_SORT:
    case NCM_TAG_ALBUM_ARTIST_SORT:
    case NCM_TAG_ALBUM_SORT:
    case NCM_TAG_LABEL:
    case NCM_TAG_MUSICBRAINZ_WORKID:
    case NCM_TAG_GROUPING:
    case NCM_TAG_WORK:
    case NCM_TAG_CONDUCTOR:
    case NCM_TAG_COMPOSER_SORT:
    case NCM_TAG_ENSEMBLE:
    case NCM_TAG_MOVEMENT:
    case NCM_TAG_MOVEMENTNUMBER:
    case NCM_TAG_LOCATION:
    case NCM_TAG_MOOD:
    case NCM_TAG_TITLE_SORT:
    case NCM_TAG_MUSICBRAINZ_RELEASEGROUPID:
    case NCM_TAG_SHOWMOVEMENT:
    case NCM_TAG_DISCSUBTITLE:
    case NCM_TAG_COUNT:
    default:
        return NCM_TAGS_FIELD_COUNT;
    }
}

enum NcmTagType
ncm_tags_field_to_tag_type(enum TagsField field) {
    switch (field) {
    case NCM_TAGS_FIELD_TITLE:
        return NCM_TAG_TITLE;
    case NCM_TAGS_FIELD_ARTIST:
        return NCM_TAG_ARTIST;
    case NCM_TAGS_FIELD_ALBUM_ARTIST:
        return NCM_TAG_ALBUM_ARTIST;
    case NCM_TAGS_FIELD_ALBUM:
        return NCM_TAG_ALBUM;
    case NCM_TAGS_FIELD_DATE:
        return NCM_TAG_DATE;
    case NCM_TAGS_FIELD_TRACK:
        return NCM_TAG_TRACK;
    case NCM_TAGS_FIELD_GENRE:
        return NCM_TAG_GENRE;
    case NCM_TAGS_FIELD_COMPOSER:
        return NCM_TAG_COMPOSER;
    case NCM_TAGS_FIELD_PERFORMER:
        return NCM_TAG_PERFORMER;
    case NCM_TAGS_FIELD_DISC:
        return NCM_TAG_DISC;
    case NCM_TAGS_FIELD_COMMENT:
        return NCM_TAG_COMMENT;
    case NCM_TAGS_FIELD_COUNT:
    default:
        return NCM_TAG_UNKNOWN;
    }
}

enum SongGetter
ncm_tags_field_to_song_getter(enum TagsField field) {
    switch (field) {
    case NCM_TAGS_FIELD_TITLE:
        return SONG_GETTER_TITLE;
    case NCM_TAGS_FIELD_ARTIST:
        return SONG_GETTER_ARTIST;
    case NCM_TAGS_FIELD_ALBUM_ARTIST:
        return SONG_GETTER_ALBUM_ARTIST;
    case NCM_TAGS_FIELD_ALBUM:
        return SONG_GETTER_ALBUM;
    case NCM_TAGS_FIELD_DATE:
        return SONG_GETTER_DATE;
    case NCM_TAGS_FIELD_TRACK:
        return SONG_GETTER_TRACK;
    case NCM_TAGS_FIELD_GENRE:
        return SONG_GETTER_GENRE;
    case NCM_TAGS_FIELD_COMPOSER:
        return SONG_GETTER_COMPOSER;
    case NCM_TAGS_FIELD_PERFORMER:
        return SONG_GETTER_PERFORMER;
    case NCM_TAGS_FIELD_DISC:
        return SONG_GETTER_DISC;
    case NCM_TAGS_FIELD_COMMENT:
        return SONG_GETTER_COMMENT;
    case NCM_TAGS_FIELD_COUNT:
    default:
        return SONG_GETTER_NONE;
    }
}

enum TagsField
ncm_song_getter_to_tags_field(enum SongGetter getter) {
    switch (getter) {
    case SONG_GETTER_TITLE:
        return NCM_TAGS_FIELD_TITLE;
    case SONG_GETTER_ARTIST:
        return NCM_TAGS_FIELD_ARTIST;
    case SONG_GETTER_ALBUM_ARTIST:
        return NCM_TAGS_FIELD_ALBUM_ARTIST;
    case SONG_GETTER_ALBUM:
        return NCM_TAGS_FIELD_ALBUM;
    case SONG_GETTER_DATE:
        return NCM_TAGS_FIELD_DATE;
    case SONG_GETTER_TRACK:
    case SONG_GETTER_TRACK_NUMBER:
        return NCM_TAGS_FIELD_TRACK;
    case SONG_GETTER_GENRE:
        return NCM_TAGS_FIELD_GENRE;
    case SONG_GETTER_COMPOSER:
        return NCM_TAGS_FIELD_COMPOSER;
    case SONG_GETTER_PERFORMER:
        return NCM_TAGS_FIELD_PERFORMER;
    case SONG_GETTER_DISC:
        return NCM_TAGS_FIELD_DISC;
    case SONG_GETTER_COMMENT:
        return NCM_TAGS_FIELD_COMMENT;
    case SONG_GETTER_NONE:
    case SONG_GETTER_LENGTH:
    case SONG_GETTER_DIRECTORY:
    case SONG_GETTER_NAME:
    case SONG_GETTER_URI:
    case SONG_GETTER_PRIORITY:
    case SONG_GETTER_COUNT:
    default:
        return NCM_TAGS_FIELD_COUNT;
    }
}


#endif /* NCM_TYPE_CONVERSIONS_C */
