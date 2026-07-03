#include "c/ncm_song.h"

#include <mpd/client.h>
#include <stdio.h>

#include "cbase/base_macros.h"
#include "c/ncm_path.h"
#include "c/ncm_string.h"


static int32 ncm_song_cstring_len(char *string);
static void ncm_song_clear_view(NcmStringView *view);
static void ncm_song_set_view(NcmStringView *view, char *data, int32 len);
static bool ncm_song_needs_numeric_zero(char *tag, int32 tag_len);
static int32 ncm_song_format_numeric_tag_prefix(char *buffer,
                                                int32 buffer_cap,
                                                char *tag,
                                                int32 tag_len,
                                                int32 copy_len);

static int32
ncm_song_cstring_len(char *string) {
    int32 len;

    if (string == NULL) {
        return 0;
    }

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }

    return len;
}

static void
ncm_song_clear_view(NcmStringView *view) {
    if (view == NULL) {
        return;
    }

    view->data = NULL;
    view->len = 0;
    return;
}

static void
ncm_song_set_view(NcmStringView *view, char *data, int32 len) {
    if (view == NULL) {
        return;
    }

    view->data = data;
    view->len = len;
    return;
}

static bool
ncm_song_needs_numeric_zero(char *tag, int32 tag_len) {
    if (tag == NULL) {
        return false;
    }
    if ((tag_len == 1) && (tag[0] != '0')) {
        return true;
    }
    if ((tag_len > 3) && (tag[1] == '/')) {
        return true;
    }

    return false;
}

static int32
ncm_song_format_numeric_tag_prefix(char *buffer, int32 buffer_cap,
                                   char *tag, int32 tag_len,
                                   int32 copy_len) {
    int32 out;
    bool add_zero;

    if ((tag == NULL) || (tag_len < 0)) {
        if ((buffer != NULL) && (buffer_cap > 0)) {
            buffer[0] = '\0';
        }
        return 0;
    }
    if (copy_len > tag_len) {
        copy_len = tag_len;
    }
    if (copy_len < 0) {
        copy_len = 0;
    }

    out = 0;
    add_zero = ncm_song_needs_numeric_zero(tag, tag_len);
    if (add_zero) {
        if ((buffer != NULL) && (out + 1 < buffer_cap)) {
            buffer[out] = '0';
        }
        out += 1;
    }

    for (int32 i = 0; i < copy_len; i += 1) {
        if ((buffer != NULL) && (out + 1 < buffer_cap)) {
            buffer[out] = tag[i];
        }
        out += 1;
    }

    if ((buffer != NULL) && (buffer_cap > 0)) {
        if (out < buffer_cap) {
            buffer[out] = '\0';
        } else {
            buffer[buffer_cap - 1] = '\0';
            out = buffer_cap - 1;
        }
    }

    return out;
}

void
ncm_song_init(NcmSong *song) {
    song->song = NULL;
    return;
}

void
ncm_song_destroy(NcmSong *song) {
    if (song->song) {
        mpd_song_free(song->song);
    }

    song->song = NULL;
    return;
}

bool
ncm_song_copy(NcmSong *dest, NcmSong *source) {
    struct mpd_song *copy;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }
    if (source->song == NULL) {
        ncm_song_destroy(dest);
        return true;
    }

    copy = mpd_song_dup(source->song);
    if (copy == NULL) {
        return false;
    }

    ncm_song_destroy(dest);
    dest->song = copy;
    return true;
}

bool
ncm_song_from_mpd_song(NcmSong *dest, struct mpd_song *source) {
    NcmSong replacement;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }

    if (mpd_song_get_uri(source) == NULL) {
        return false;
    }

    ncm_song_init(&replacement);
    replacement.song = mpd_song_dup(source);
    if (replacement.song == NULL) {
        return false;
    }

    ncm_song_destroy(dest);
    *dest = replacement;
    return true;
}

bool
ncm_song_empty(NcmSong *song) {
    if (song == NULL) {
        return true;
    }

    return song->song == NULL;
}

struct mpd_song *
ncm_song_mpd_song(NcmSong *song) {
    if (song == NULL) {
        return NULL;
    }

    return song->song;
}

struct mpd_song *
ncm_song_dup_mpd_song(NcmSong *song) {
    if (song == NULL) {
        return NULL;
    }
    if (song->song == NULL) {
        return NULL;
    }

    return mpd_song_dup(song->song);
}

bool
ncm_song_name_from_uri(char *uri, int32 uri_len, NcmStringView *view) {
    int32 basename;

    ncm_song_clear_view(view);
    if (uri == NULL) {
        return false;
    }
    if (uri_len < 0) {
        return false;
    }

    basename = ncm_path_basename_start(uri, uri_len);
    ncm_song_set_view(view, uri + basename, uri_len - basename);
    return true;
}

bool
ncm_song_directory_from_uri(char *uri, int32 uri_len,
                            NcmStringView *view) {
    int32 basename;

    ncm_song_clear_view(view);
    if (uri == NULL) {
        return false;
    }
    if (uri_len < 0) {
        return false;
    }

    basename = ncm_path_basename_start(uri, uri_len);
    if (basename == 0) {
        ncm_song_set_view(view, STRLIT_ARGS("/"));
    } else {
        ncm_song_set_view(view, uri, basename - 1);
    }

    return true;
}

bool
ncm_song_uri_is_from_database(char *uri, int32 uri_len) {
    if (uri == NULL) {
        return false;
    }
    if (uri_len <= 0) {
        return true;
    }
    if (uri[0] != '/') {
        return true;
    }

    return ncm_string_find_char(uri, uri_len, '/') < 0;
}

bool
ncm_song_uri_is_stream(char *uri, int32 uri_len) {
    if (uri == NULL) {
        return false;
    }

    return ncm_string_starts_with(uri, uri_len, STRLIT_ARGS("http://"))
           || ncm_string_starts_with(uri, uri_len, STRLIT_ARGS("https://"));
}

bool
ncm_mpd_song_tag_view(struct mpd_song *song, enum mpd_tag_type tag,
                      uint32 idx, NcmStringView *view) {
    char *value;

    ncm_song_clear_view(view);
    if (song == NULL) {
        return false;
    }

    value = (char *)mpd_song_get_tag(song, tag, idx);
    if (value == NULL) {
        return false;
    }

    ncm_song_set_view(view, value, ncm_song_cstring_len(value));
    return true;
}

bool
ncm_mpd_song_uri_view(struct mpd_song *song, uint32 idx,
                      NcmStringView *view) {
    char *uri;

    ncm_song_clear_view(view);
    if (song == NULL) {
        return false;
    }
    if (idx > 0) {
        return false;
    }

    uri = (char *)mpd_song_get_uri(song);
    if (uri == NULL) {
        return false;
    }

    ncm_song_set_view(view, uri, ncm_song_cstring_len(uri));
    return true;
}

bool
ncm_mpd_song_name_view(struct mpd_song *song, uint32 idx,
                       NcmStringView *view) {
    NcmStringView uri;

    if (ncm_mpd_song_tag_view(song, MPD_TAG_NAME, idx, view)) {
        return true;
    }
    if (idx > 0) {
        return false;
    }
    if (!ncm_mpd_song_uri_view(song, 0, &uri)) {
        ncm_song_clear_view(view);
        return false;
    }

    return ncm_song_name_from_uri(uri.data, uri.len, view);
}

bool
ncm_mpd_song_directory_view(struct mpd_song *song, uint32 idx,
                            NcmStringView *view) {
    NcmStringView uri;

    ncm_song_clear_view(view);
    if (idx > 0) {
        return false;
    }
    if (ncm_mpd_song_is_stream(song)) {
        return false;
    }
    if (!ncm_mpd_song_uri_view(song, 0, &uri)) {
        return false;
    }

    return ncm_song_directory_from_uri(uri.data, uri.len, view);
}

bool
ncm_mpd_song_is_from_database(struct mpd_song *song) {
    NcmStringView uri;

    if (!ncm_mpd_song_uri_view(song, 0, &uri)) {
        return false;
    }

    return ncm_song_uri_is_from_database(uri.data, uri.len);
}

bool
ncm_mpd_song_is_stream(struct mpd_song *song) {
    NcmStringView uri;

    if (!ncm_mpd_song_uri_view(song, 0, &uri)) {
        return false;
    }

    return ncm_song_uri_is_stream(uri.data, uri.len);
}

int32
ncm_song_numeric_tag_len(char *tag, int32 tag_len) {
    if ((tag == NULL) || (tag_len < 0)) {
        return 0;
    }
    if (ncm_song_needs_numeric_zero(tag, tag_len)) {
        return tag_len + 1;
    }

    return tag_len;
}

int32
ncm_song_track_number_len(char *tag, int32 tag_len) {
    int32 slash;
    int32 extra;

    if ((tag == NULL) || (tag_len < 0)) {
        return 0;
    }

    slash = ncm_string_find_char(tag, tag_len, '/');
    if (ncm_song_needs_numeric_zero(tag, tag_len)) {
        extra = 1;
    } else {
        extra = 0;
    }
    if (slash >= 0) {
        return slash + extra;
    }

    return tag_len + extra;
}

int32
ncm_song_format_numeric_tag(char *buffer, int32 buffer_cap,
                            char *tag, int32 tag_len) {
    return ncm_song_format_numeric_tag_prefix(buffer, buffer_cap,
                                              tag, tag_len, tag_len);
}

int32
ncm_song_format_track_number(char *buffer, int32 buffer_cap,
                             char *tag, int32 tag_len) {
    int32 slash;
    int32 copy_len;

    if ((tag == NULL) || (tag_len < 0)) {
        return ncm_song_format_numeric_tag_prefix(buffer, buffer_cap,
                                                  tag, tag_len, 0);
    }

    slash = ncm_string_find_char(tag, tag_len, '/');
    if (slash >= 0) {
        copy_len = slash;
    } else {
        copy_len = tag_len;
    }

    return ncm_song_format_numeric_tag_prefix(buffer, buffer_cap,
                                              tag, tag_len, copy_len);
}

int32
ncm_song_show_time(uint32 length, char *buffer, int32 buffer_cap) {
    uint32 hours;
    uint32 minutes;
    uint32 seconds;
    int32 result;

    if ((buffer == NULL) || (buffer_cap <= 0)) {
        return 0;
    }

    hours = length/3600;
    length -= hours*3600;
    minutes = length/60;
    length -= minutes*60;
    seconds = length;

    if (hours > 0) {
        result = snprintf(buffer, (size_t)buffer_cap, "%u:%02u:%02u",
                          hours, minutes, seconds);
    } else {
        result = snprintf(buffer, (size_t)buffer_cap, "%u:%02u",
                          minutes, seconds);
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
