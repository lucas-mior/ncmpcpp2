#include "c/ncm_type_conversions.h"

#include <stdio.h>

int32
ncm_channels_to_string(int32 channels, char *buffer, int32 buffer_cap) {
    int32 result;

    if ((buffer == NULL) || (buffer_cap <= 0)) {
        return 0;
    }

    switch (channels) {
    case 1:
        result = snprintf(buffer, (size_t)buffer_cap, "Mono");
        break;
    case 2:
        result = snprintf(buffer, (size_t)buffer_cap, "Stereo");
        break;
    default:
        result = snprintf(buffer, (size_t)buffer_cap, "%d", channels);
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
ncm_tag_type_name(enum mpd_tag_type tag) {
    switch (tag) {
    case MPD_TAG_ARTIST:
        return "Artist";
    case MPD_TAG_ALBUM:
        return "Album";
    case MPD_TAG_ALBUM_ARTIST:
        return "Album Artist";
    case MPD_TAG_TITLE:
        return "Title";
    case MPD_TAG_TRACK:
        return "Track";
    case MPD_TAG_GENRE:
        return "Genre";
    case MPD_TAG_DATE:
        return "Date";
    case MPD_TAG_COMPOSER:
        return "Composer";
    case MPD_TAG_PERFORMER:
        return "Performer";
    case MPD_TAG_COMMENT:
        return "Comment";
    case MPD_TAG_DISC:
        return "Disc";
    default:
        return "";
    }
}

bool
ncm_char_is_tag_type(char c) {
    return ncm_char_to_tag_type(c) != MPD_TAG_UNKNOWN;
}

enum mpd_tag_type
ncm_char_to_tag_type(char c) {
    switch (c) {
    case 'a':
        return MPD_TAG_ARTIST;
    case 'A':
        return MPD_TAG_ALBUM_ARTIST;
    case 't':
        return MPD_TAG_TITLE;
    case 'b':
        return MPD_TAG_ALBUM;
    case 'y':
        return MPD_TAG_DATE;
    case 'n':
        return MPD_TAG_TRACK;
    case 'g':
        return MPD_TAG_GENRE;
    case 'c':
        return MPD_TAG_COMPOSER;
    case 'p':
        return MPD_TAG_PERFORMER;
    case 'd':
        return MPD_TAG_DISC;
    case 'C':
        return MPD_TAG_COMMENT;
    default:
        return MPD_TAG_UNKNOWN;
    }
}

enum NcmSongGetter
ncm_song_getter_from_char(char c) {
    switch (c) {
    case 'l':
        return NCM_SONG_GETTER_LENGTH;
    case 'D':
        return NCM_SONG_GETTER_DIRECTORY;
    case 'f':
        return NCM_SONG_GETTER_NAME;
    case 'F':
        return NCM_SONG_GETTER_URI;
    case 'a':
        return NCM_SONG_GETTER_ARTIST;
    case 'A':
        return NCM_SONG_GETTER_ALBUM_ARTIST;
    case 't':
        return NCM_SONG_GETTER_TITLE;
    case 'b':
        return NCM_SONG_GETTER_ALBUM;
    case 'y':
        return NCM_SONG_GETTER_DATE;
    case 'n':
        return NCM_SONG_GETTER_TRACK_NUMBER;
    case 'N':
        return NCM_SONG_GETTER_TRACK;
    case 'g':
        return NCM_SONG_GETTER_GENRE;
    case 'c':
        return NCM_SONG_GETTER_COMPOSER;
    case 'p':
        return NCM_SONG_GETTER_PERFORMER;
    case 'd':
        return NCM_SONG_GETTER_DISC;
    case 'C':
        return NCM_SONG_GETTER_COMMENT;
    case 'P':
        return NCM_SONG_GETTER_PRIORITY;
    default:
        return NCM_SONG_GETTER_NONE;
    }
}

enum NcmSongGetter
ncm_song_getter_from_tag_type(enum mpd_tag_type tag) {
    switch (tag) {
    case MPD_TAG_ARTIST:
        return NCM_SONG_GETTER_ARTIST;
    case MPD_TAG_ALBUM:
        return NCM_SONG_GETTER_ALBUM;
    case MPD_TAG_ALBUM_ARTIST:
        return NCM_SONG_GETTER_ALBUM_ARTIST;
    case MPD_TAG_TITLE:
        return NCM_SONG_GETTER_TITLE;
    case MPD_TAG_TRACK:
        return NCM_SONG_GETTER_TRACK;
    case MPD_TAG_GENRE:
        return NCM_SONG_GETTER_GENRE;
    case MPD_TAG_DATE:
        return NCM_SONG_GETTER_DATE;
    case MPD_TAG_COMPOSER:
        return NCM_SONG_GETTER_COMPOSER;
    case MPD_TAG_PERFORMER:
        return NCM_SONG_GETTER_PERFORMER;
    case MPD_TAG_COMMENT:
        return NCM_SONG_GETTER_COMMENT;
    case MPD_TAG_DISC:
        return NCM_SONG_GETTER_DISC;
    default:
        return NCM_SONG_GETTER_NONE;
    }
}

enum mpd_tag_type
ncm_song_getter_to_tag_type(enum NcmSongGetter getter) {
    switch (getter) {
    case NCM_SONG_GETTER_ARTIST:
        return MPD_TAG_ARTIST;
    case NCM_SONG_GETTER_TITLE:
        return MPD_TAG_TITLE;
    case NCM_SONG_GETTER_ALBUM:
        return MPD_TAG_ALBUM;
    case NCM_SONG_GETTER_ALBUM_ARTIST:
        return MPD_TAG_ALBUM_ARTIST;
    case NCM_SONG_GETTER_TRACK:
        return MPD_TAG_TRACK;
    case NCM_SONG_GETTER_DATE:
        return MPD_TAG_DATE;
    case NCM_SONG_GETTER_GENRE:
        return MPD_TAG_GENRE;
    case NCM_SONG_GETTER_COMPOSER:
        return MPD_TAG_COMPOSER;
    case NCM_SONG_GETTER_PERFORMER:
        return MPD_TAG_PERFORMER;
    case NCM_SONG_GETTER_COMMENT:
        return MPD_TAG_COMMENT;
    case NCM_SONG_GETTER_DISC:
        return MPD_TAG_DISC;
    default:
        return MPD_TAG_UNKNOWN;
    }
}

char *
ncm_item_type_name(enum NcmItemType type) {
    switch (type) {
    case NCM_ITEM_DIRECTORY:
        return "directory";
    case NCM_ITEM_SONG:
        return "song";
    case NCM_ITEM_PLAYLIST:
        return "playlist";
    case NCM_ITEM_UNKNOWN:
        return "unknown";
    default:
        return "unknown";
    }
}
