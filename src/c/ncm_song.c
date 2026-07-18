#if !defined(NCM_SONG_C)
#define NCM_SONG_C

#include "c/ncm_song.h"

#include <mpd/client.h>
#include <stdio.h>

#include "c/ncm_base.h"
#include "c/ncm_path.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/rapidhash.h"
#include "cbase/util.c"

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

static void
ncm_song_tag_init(NcmSongTag *tag) {
    tag->value = NULL;
    tag->value_len = 0;
    tag->type = MPD_TAG_UNKNOWN;
    return;
}

static void
ncm_song_tag_destroy(NcmSongTag *tag) {
    if (tag == NULL) {
        return;
    }
    if (tag->value != NULL) {
        free2(tag->value, tag->value_len + 1);
    }

    ncm_song_tag_init(tag);
    return;
}

static bool
ncm_song_tag_copy(NcmSongTag *dest, NcmSongTag *source) {
    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }
    if (source->value == NULL) {
        ncm_song_tag_destroy(dest);
        return true;
    }

    ncm_song_tag_destroy(dest);
    dest->value = (char *)malloc2(source->value_len + 1);
    dest->value_len = source->value_len;
    dest->type = source->type;
    memcpy64(dest->value, source->value, source->value_len);
    dest->value[source->value_len] = '\0';
    return true;
}

static bool
ncm_song_grow_tags(NcmSong *song) {
    int32 old_cap;
    int32 new_cap;

    if (song->tags_len < song->tags_cap) {
        return true;
    }

    old_cap = song->tags_cap;
    if (old_cap == 0) {
        new_cap = 8;
    } else {
        new_cap = old_cap*2;
    }

    song->tags = (NcmSongTag *)realloc2(song->tags, old_cap,
                                    new_cap,
                                    SIZEOF(*song->tags));
    for (int32 i = old_cap; i < new_cap; i += 1) {
        ncm_song_tag_init(&song->tags[i]);
    }
    song->tags_cap = new_cap;
    return true;
}

static bool
ncm_song_load_mpd_tag(NcmSong *song, struct mpd_song *source,
                      enum mpd_tag_type type) {
    char *value;

    for (uint32 i = 0; ; i += 1) {
        value = (char *)mpd_song_get_tag(source, type, i);
        if (value == NULL) {
            break;
        }
        if (!ncm_song_add_tag(song, type, value,
                              optional_strlen32(value))) {
            return false;
        }
    }

    return true;
}

void
ncm_song_init(NcmSong *song) {
    song->uri = NULL;
    song->uri_len = 0;
    song->tags = NULL;
    song->tags_len = 0;
    song->tags_cap = 0;
    song->duration = 0;
    song->position = 0;
    song->id = 0;
    song->priority = 0;
    song->last_modified = 0;
    return;
}

void
ncm_song_destroy(NcmSong *song) {
    if (song == NULL) {
        return;
    }

    if (song->uri != NULL) {
        free2(song->uri, song->uri_len + 1);
    }
    for (int32 i = 0; i < song->tags_len; i += 1) {
        ncm_song_tag_destroy(&song->tags[i]);
    }
    if (song->tags != NULL) {
        free2(song->tags, song->tags_cap*SIZEOF(*song->tags));
    }

    ncm_song_init(song);
    return;
}

void
ncm_song_move(NcmSong *dest, NcmSong *source) {
    if (dest == NULL) {
        return;
    }
    if (source == NULL) {
        ncm_song_destroy(dest);
        return;
    }

    ncm_song_destroy(dest);
    *dest = *source;
    ncm_song_init(source);
    return;
}

bool
ncm_song_copy(NcmSong *dest, NcmSong *source) {
    NcmSong replacement;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }

    ncm_song_init(&replacement);
    if (source->uri != NULL) {
        if (!ncm_song_set_uri(&replacement, source->uri,
                              source->uri_len)) {
            ncm_song_destroy(&replacement);
            return false;
        }
    }

    for (int32 i = 0; i < source->tags_len; i += 1) {
        if (!ncm_song_grow_tags(&replacement)) {
            ncm_song_destroy(&replacement);
            return false;
        }
        if (!ncm_song_tag_copy(&replacement.tags[replacement.tags_len],
                               &source->tags[i])) {
            ncm_song_destroy(&replacement);
            return false;
        }
        replacement.tags_len += 1;
    }

    replacement.duration = source->duration;
    replacement.position = source->position;
    replacement.id = source->id;
    replacement.priority = source->priority;
    replacement.last_modified = source->last_modified;

    ncm_song_destroy(dest);
    *dest = replacement;
    return true;
}

bool
ncm_song_from_mpd_song(NcmSong *dest, struct mpd_song *source) {
    return ncm_song_from_mpd_song_copy(dest, source);
}

bool
ncm_song_from_mpd_song_copy(NcmSong *dest, struct mpd_song *source) {
    NcmSong replacement;
    char *uri;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }

    uri = (char *)mpd_song_get_uri(source);
    if (uri == NULL) {
        return false;
    }

    ncm_song_init(&replacement);
    if (!ncm_song_set_uri(&replacement, uri, optional_strlen32(uri))) {
        ncm_song_destroy(&replacement);
        return false;
    }

    if (!ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_ARTIST)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_ALBUM)
        || !ncm_song_load_mpd_tag(&replacement, source,
                                  MPD_TAG_ALBUM_ARTIST)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_TITLE)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_TRACK)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_NAME)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_GENRE)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_DATE)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_COMPOSER)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_PERFORMER)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_COMMENT)
        || !ncm_song_load_mpd_tag(&replacement, source, MPD_TAG_DISC)) {
        ncm_song_destroy(&replacement);
        return false;
    }

    replacement.duration = mpd_song_get_duration(source);
    replacement.position = mpd_song_get_pos(source);
    replacement.id = mpd_song_get_id(source);
    replacement.priority = mpd_song_get_prio(source);
    replacement.last_modified = mpd_song_get_last_modified(source);

    ncm_song_destroy(dest);
    *dest = replacement;
    return true;
}

bool
ncm_song_set_uri(NcmSong *song, char *uri, int32 uri_len) {
    char *copy;

    if (song == NULL) {
        return false;
    }
    if (uri == NULL) {
        return false;
    }
    if (uri_len < 0) {
        return false;
    }

    copy = (char *)malloc2(uri_len + 1);
    memcpy64(copy, uri, uri_len);
    copy[uri_len] = '\0';

    if (song->uri != NULL) {
        free2(song->uri, song->uri_len + 1);
    }
    song->uri = copy;
    song->uri_len = uri_len;
    return true;
}

bool
ncm_song_add_tag(NcmSong *song, enum mpd_tag_type type,
                 char *value, int32 value_len) {
    NcmSongTag *tag;

    if (song == NULL) {
        return false;
    }
    if (value == NULL) {
        return false;
    }
    if (value_len < 0) {
        return false;
    }
    if (type == MPD_TAG_UNKNOWN) {
        return false;
    }
    if (!ncm_song_grow_tags(song)) {
        return false;
    }

    tag = &song->tags[song->tags_len];
    ncm_song_tag_destroy(tag);
    tag->value = (char *)malloc2(value_len + 1);
    tag->value_len = value_len;
    tag->type = type;
    memcpy64(tag->value, value, value_len);
    tag->value[value_len] = '\0';
    song->tags_len += 1;
    return true;
}

void
ncm_song_set_duration(NcmSong *song, int32 duration) {
    if (song != NULL) {
        song->duration = duration;
    }
    return;
}

void
ncm_song_set_position(NcmSong *song, int32 position) {
    if (song != NULL) {
        song->position = position;
    }
    return;
}

void
ncm_song_set_id(NcmSong *song, int32 id) {
    if (song != NULL) {
        song->id = id;
    }
    return;
}

void
ncm_song_set_priority(NcmSong *song, int32 priority) {
    if (song != NULL) {
        song->priority = priority;
    }
    return;
}

void
ncm_song_set_mtime(NcmSong *song, time_t last_modified) {
    if (song != NULL) {
        song->last_modified = last_modified;
    }
    return;
}

int32
ncm_song_duration(NcmSong *song) {
    if (song == NULL) {
        return 0;
    }
    return song->duration;
}

int32
ncm_song_position(NcmSong *song) {
    if (song == NULL) {
        return 0;
    }
    return song->position;
}

int32
ncm_song_id(NcmSong *song) {
    if (song == NULL) {
        return 0;
    }
    return song->id;
}

int32
ncm_song_priority(NcmSong *song) {
    if (song == NULL) {
        return 0;
    }
    return song->priority;
}

time_t
ncm_song_mtime(NcmSong *song) {
    if (song == NULL) {
        return 0;
    }
    return song->last_modified;
}

bool
ncm_song_empty(NcmSong *song) {
    if (song == NULL) {
        return true;
    }

    return song->uri == NULL;
}

bool
ncm_song_tag_view(NcmSong *song, enum mpd_tag_type tag, int32 idx,
                  NcmStringView *view) {
    int32 seen;

    ncm_string_view_clear(view);
    if (song == NULL) {
        return false;
    }

    seen = 0;
    for (int32 i = 0; i < song->tags_len; i += 1) {
        if (song->tags[i].type != tag) {
            continue;
        }
        if (seen == idx) {
            ncm_string_view_set(view, song->tags[i].value,
                              song->tags[i].value_len);
            return true;
        }
        seen += 1;
    }

    return false;
}

bool
ncm_song_uri_view(NcmSong *song, int32 idx, NcmStringView *view) {
    ncm_string_view_clear(view);
    if (song == NULL) {
        return false;
    }
    if (idx > 0) {
        return false;
    }
    if (song->uri == NULL) {
        return false;
    }

    ncm_string_view_set(view, song->uri, song->uri_len);
    return true;
}

bool
ncm_song_name_view(NcmSong *song, int32 idx, NcmStringView *view) {
    NcmStringView uri;

    if (ncm_song_tag_view(song, MPD_TAG_NAME, idx, view)) {
        return true;
    }
    if (idx > 0) {
        return false;
    }
    if (!ncm_song_uri_view(song, 0, &uri)) {
        ncm_string_view_clear(view);
        return false;
    }

    return ncm_song_name_from_uri(uri.data, uri.len, view);
}

bool
ncm_song_directory_view(NcmSong *song, int32 idx, NcmStringView *view) {
    NcmStringView uri;

    ncm_string_view_clear(view);
    if (idx > 0) {
        return false;
    }
    if (ncm_song_is_stream(song)) {
        return false;
    }
    if (!ncm_song_uri_view(song, 0, &uri)) {
        return false;
    }

    return ncm_song_directory_from_uri(uri.data, uri.len, view);
}

bool
ncm_song_is_from_database(NcmSong *song) {
    NcmStringView uri;

    if (!ncm_song_uri_view(song, 0, &uri)) {
        return false;
    }

    return ncm_song_uri_is_from_database(uri.data, uri.len);
}

bool
ncm_song_is_stream(NcmSong *song) {
    NcmStringView uri;

    if (!ncm_song_uri_view(song, 0, &uri)) {
        return false;
    }

    return ncm_song_uri_is_stream(uri.data, uri.len);
}

bool
ncm_song_name_from_uri(char *uri, int32 uri_len, NcmStringView *view) {
    int32 basename;

    ncm_string_view_clear(view);
    if (uri == NULL) {
        return false;
    }
    if (uri_len < 0) {
        return false;
    }

    basename = ncm_path_basename_start(uri, uri_len);
    ncm_string_view_set(view, uri + basename, uri_len - basename);
    return true;
}

bool
ncm_song_directory_from_uri(char *uri, int32 uri_len,
                            NcmStringView *view) {
    int32 basename;

    ncm_string_view_clear(view);
    if (uri == NULL) {
        return false;
    }
    if (uri_len < 0) {
        return false;
    }

    basename = ncm_path_basename_start(uri, uri_len);
    if (basename == 0) {
        ncm_string_view_set(view, STRLIT_ARGS("/"));
    } else {
        ncm_string_view_set(view, uri, basename - 1);
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
ncm_song_show_time(int32 length, char *buffer, int32 buffer_cap) {
    int32 hours;
    int32 minutes;
    int32 seconds;
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
        result = snprintf2(buffer, buffer_cap, "%u:%02u:%02u",
                          hours, minutes, seconds);
    } else {
        result = snprintf2(buffer, buffer_cap, "%u:%02u",
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

NcmBuffer
ncm_song_getter_buffer(NcmSong *song, enum NcmSongGetter getter, int32 idx) {
    NcmBuffer buffer;
    NcmStringView view;
    char number_buffer[32];
    int32 len;
    enum mpd_tag_type tag;

    ncm_buffer_init(&buffer);
    if (song == NULL) {
        return buffer;
    }

    switch (getter) {
    case NCM_SONG_GETTER_LENGTH:
        if (idx > 0) {
            return buffer;
        }
        if (song->duration > 0) {
            len = ncm_song_show_time(song->duration, number_buffer,
                                     NCM_ARRAY_LEN(number_buffer));
            ncm_buffer_append(&buffer, number_buffer, len);
        } else {
            ncm_buffer_append(&buffer, STRLIT_ARGS("-:--"));
        }
        return buffer;
    case NCM_SONG_GETTER_DIRECTORY:
        if (ncm_song_directory_view(song, idx, &view)) {
            ncm_buffer_append(&buffer, view.data, view.len);
        }
        return buffer;
    case NCM_SONG_GETTER_NAME:
        if (ncm_song_name_view(song, idx, &view)) {
            ncm_buffer_append(&buffer, view.data, view.len);
        }
        return buffer;
    case NCM_SONG_GETTER_URI:
        if (ncm_song_uri_view(song, idx, &view)) {
            ncm_buffer_append(&buffer, view.data, view.len);
        }
        return buffer;
    case NCM_SONG_GETTER_TRACK:
        if (ncm_song_tag_view(song, MPD_TAG_TRACK, idx, &view)) {
            len = ncm_song_numeric_tag_len(view.data, view.len);
            ncm_buffer_reserve(&buffer, len);
            buffer.len = ncm_song_format_numeric_tag(buffer.data,
                                                     buffer.cap,
                                                     view.data, view.len);
        }
        return buffer;
    case NCM_SONG_GETTER_TRACK_NUMBER:
        if (ncm_song_tag_view(song, MPD_TAG_TRACK, idx, &view)) {
            len = ncm_song_track_number_len(view.data, view.len);
            ncm_buffer_reserve(&buffer, len);
            buffer.len = ncm_song_format_track_number(buffer.data,
                                                      buffer.cap,
                                                      view.data, view.len);
        }
        return buffer;
    case NCM_SONG_GETTER_DISC:
        if (ncm_song_tag_view(song, MPD_TAG_DISC, idx, &view)) {
            len = ncm_song_numeric_tag_len(view.data, view.len);
            ncm_buffer_reserve(&buffer, len);
            buffer.len = ncm_song_format_numeric_tag(buffer.data,
                                                     buffer.cap,
                                                     view.data, view.len);
        }
        return buffer;
    case NCM_SONG_GETTER_PRIORITY:
        if (idx > 0) {
            return buffer;
        }
        len = SNPRINTF(number_buffer,
                       "%u", song->priority);
        if (len > 0) {
            if (len >= NCM_ARRAY_LEN(number_buffer)) {
                len = NCM_ARRAY_LEN(number_buffer) - 1;
            }
            ncm_buffer_append(&buffer, number_buffer, len);
        }
        return buffer;
    case NCM_SONG_GETTER_ARTIST:
    case NCM_SONG_GETTER_ALBUM_ARTIST:
    case NCM_SONG_GETTER_TITLE:
    case NCM_SONG_GETTER_ALBUM:
    case NCM_SONG_GETTER_DATE:
    case NCM_SONG_GETTER_GENRE:
    case NCM_SONG_GETTER_COMPOSER:
    case NCM_SONG_GETTER_PERFORMER:
    case NCM_SONG_GETTER_COMMENT:
        tag = ncm_song_getter_to_tag_type(getter);
        if (ncm_song_tag_view(song, tag, idx, &view)) {
            ncm_buffer_append(&buffer, view.data, view.len);
        }
        return buffer;
    case NCM_SONG_GETTER_NONE:
    default:
        return buffer;
    }
}

NcmBuffer
ncm_song_tags_buffer(NcmSong *song, enum NcmSongGetter getter,
                     char *separator, int32 separator_len,
                     bool show_duplicates) {
    NcmBuffer result;
    NcmBuffer tag;

    ncm_buffer_init(&result);
    if (song == NULL) {
        return result;
    }
    if (getter == NCM_SONG_GETTER_NONE) {
        return result;
    }
    if ((separator == NULL) || (separator_len < 0)) {
        separator = "";
        separator_len = 0;
    }

    for (uint32 i = 0; ; i += 1) {
        bool already_present;

        tag = ncm_song_getter_buffer(song, getter, i);
        if (tag.len == 0) {
            ncm_buffer_destroy(&tag);
            break;
        }

        already_present = false;
        if (!show_duplicates) {
            for (uint32 j = 0; j < i; j += 1) {
                NcmBuffer previous;

                previous = ncm_song_getter_buffer(song, getter, j);
                if (STREQUAL(previous.data, previous.len,
                                     tag.data, tag.len)) {
                    already_present = true;
                }
                ncm_buffer_destroy(&previous);
                if (already_present) {
                    break;
                }
            }
        }

        if (!already_present) {
            if (result.len > 0) {
                ncm_buffer_append(&result, separator, separator_len);
            }
            ncm_buffer_append(&result, tag.data, tag.len);
        }
        ncm_buffer_destroy(&tag);
    }

    return result;
}

bool
ncm_song_equal(NcmSong *a, NcmSong *b) {
    NcmStringView a_uri;
    NcmStringView b_uri;

    if ((a == NULL) || (b == NULL)) {
        return a == b;
    }
    if (!ncm_song_uri_view(a, 0, &a_uri)) {
        return !ncm_song_uri_view(b, 0, &b_uri);
    }
    if (!ncm_song_uri_view(b, 0, &b_uri)) {
        return false;
    }

    return STREQUAL(a_uri.data, a_uri.len, b_uri.data, b_uri.len);
}

#endif /* NCM_SONG_C */
