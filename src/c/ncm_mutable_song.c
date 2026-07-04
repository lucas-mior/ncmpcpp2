#include "c/ncm_mutable_song.h"

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/cbase.h"

static void ncm_mutable_song_view_init(NcmStringView *view);
static void ncm_mutable_song_free_string(char **string, int32 *string_len);
static bool ncm_mutable_song_set_string(char **dest, int32 *dest_len,
                                        char *source, int32 source_len);
static bool ncm_mutable_song_string_equal(char *a, int32 a_len,
                                          char *b, int32 b_len);
static NcmMutableSongTag *ncm_mutable_song_find_tag(NcmMutableSong *song,
                                                    enum NcmTagsField field,
                                                    int32 idx);
static NcmMutableSongTag *ncm_mutable_song_add_tag(NcmMutableSong *song,
                                                   enum NcmTagsField field,
                                                   int32 idx);
static bool ncm_mutable_song_grow_tags(NcmMutableSong *song);
static void ncm_mutable_song_tag_init(NcmMutableSongTag *tag);
static void ncm_mutable_song_tag_destroy(NcmMutableSongTag *tag);
static bool ncm_mutable_song_tag_copy(NcmMutableSongTag *dest,
                                      NcmMutableSongTag *source);
static bool ncm_mutable_song_write_callback(enum NcmTagsField field,
                                            int32 idx, NcmStringView *value,
                                            void *user);
static enum NcmSongGetter ncm_mutable_song_field_getter(
    enum NcmTagsField field);

static void
ncm_mutable_song_view_init(NcmStringView *view) {
    if (view == NULL) {
        return;
    }

    view->data = NULL;
    view->len = 0;
    return;
}

static void
ncm_mutable_song_free_string(char **string, int32 *string_len) {
    if (string == NULL) {
        return;
    }
    if (string_len == NULL) {
        return;
    }

    if (*string != NULL) {
        cbase_free(*string, *string_len + 1);
    }
    *string = NULL;
    if (string_len != NULL) {
        *string_len = 0;
    }
    return;
}

static bool
ncm_mutable_song_set_string(char **dest, int32 *dest_len,
                            char *source, int32 source_len) {
    char *copy;

    if (dest == NULL) {
        return false;
    }
    if (dest_len == NULL) {
        return false;
    }
    if (source_len < 0) {
        return false;
    }
    if ((source == NULL) && (source_len > 0)) {
        return false;
    }

    copy = NULL;
    if (source != NULL) {
        copy = (char *)cbase_malloc(source_len + 1);
        if (source_len > 0) {
            cbase_memcpy(copy, source, source_len);
        }
        copy[source_len] = '\0';
    }

    ncm_mutable_song_free_string(dest, dest_len);
    *dest = copy;
    if (copy != NULL) {
        *dest_len = source_len;
    }
    return true;
}

static bool
ncm_mutable_song_string_equal(char *a, int32 a_len, char *b, int32 b_len) {
    if (a_len != b_len) {
        return false;
    }
    if (a_len == 0) {
        return true;
    }
    if ((a == NULL) || (b == NULL)) {
        return false;
    }

    for (int32 i = 0; i < a_len; i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

static NcmMutableSongTag *
ncm_mutable_song_find_tag(NcmMutableSong *song, enum NcmTagsField field,
                          int32 idx) {
    if (song == NULL) {
        return NULL;
    }
    if (idx < 0) {
        return NULL;
    }

    for (int32 i = 0; i < song->tags_len; i += 1) {
        NcmMutableSongTag *tag;

        tag = &song->tags[i];
        if ((tag->field == field) && (tag->idx == idx)) {
            return tag;
        }
    }

    return NULL;
}

static bool
ncm_mutable_song_grow_tags(NcmMutableSong *song) {
    int32 new_cap;

    if (song == NULL) {
        return false;
    }
    if (song->tags_len < song->tags_cap) {
        return true;
    }

    if (song->tags_cap <= 0) {
        new_cap = 16;
    } else {
        new_cap = song->tags_cap*2;
    }

    song->tags = (NcmMutableSongTag *)cbase_realloc_array(
        song->tags, song->tags_cap, new_cap, SIZEOF(*song->tags));
    for (int32 i = song->tags_cap; i < new_cap; i += 1) {
        ncm_mutable_song_tag_init(&song->tags[i]);
    }
    song->tags_cap = new_cap;
    return true;
}

static NcmMutableSongTag *
ncm_mutable_song_add_tag(NcmMutableSong *song, enum NcmTagsField field,
                         int32 idx) {
    NcmMutableSongTag *tag;

    if (song == NULL) {
        return NULL;
    }
    if (idx < 0) {
        return NULL;
    }
    if (!ncm_mutable_song_grow_tags(song)) {
        return NULL;
    }

    tag = &song->tags[song->tags_len];
    song->tags_len += 1;
    tag->field = field;
    tag->idx = idx;
    return tag;
}

static void
ncm_mutable_song_tag_init(NcmMutableSongTag *tag) {
    if (tag == NULL) {
        return;
    }

    tag->original = NULL;
    tag->value = NULL;
    tag->original_len = 0;
    tag->value_len = 0;
    tag->idx = 0;
    tag->field = NCM_TAGS_FIELD_LAST;
    tag->modified = false;
    return;
}

static void
ncm_mutable_song_tag_destroy(NcmMutableSongTag *tag) {
    if (tag == NULL) {
        return;
    }

    ncm_mutable_song_free_string(&tag->original, &tag->original_len);
    ncm_mutable_song_free_string(&tag->value, &tag->value_len);
    tag->idx = 0;
    tag->field = NCM_TAGS_FIELD_LAST;
    tag->modified = false;
    return;
}

static bool
ncm_mutable_song_tag_copy(NcmMutableSongTag *dest,
                          NcmMutableSongTag *source) {
    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }

    ncm_mutable_song_tag_destroy(dest);
    dest->field = source->field;
    dest->idx = source->idx;
    dest->modified = source->modified;
    if (!ncm_mutable_song_set_string(&dest->original, &dest->original_len,
                                     source->original,
                                     source->original_len)) {
        return false;
    }
    if (!ncm_mutable_song_set_string(&dest->value, &dest->value_len,
                                     source->value, source->value_len)) {
        return false;
    }

    return true;
}

static enum NcmSongGetter
ncm_mutable_song_field_getter(enum NcmTagsField field) {
    switch (field) {
    case NCM_TAGS_FIELD_TITLE:
        return NCM_SONG_GETTER_TITLE;
    case NCM_TAGS_FIELD_ARTIST:
        return NCM_SONG_GETTER_ARTIST;
    case NCM_TAGS_FIELD_ALBUM_ARTIST:
        return NCM_SONG_GETTER_ALBUM_ARTIST;
    case NCM_TAGS_FIELD_ALBUM:
        return NCM_SONG_GETTER_ALBUM;
    case NCM_TAGS_FIELD_DATE:
        return NCM_SONG_GETTER_DATE;
    case NCM_TAGS_FIELD_TRACK:
        return NCM_SONG_GETTER_TRACK;
    case NCM_TAGS_FIELD_GENRE:
        return NCM_SONG_GETTER_GENRE;
    case NCM_TAGS_FIELD_COMPOSER:
        return NCM_SONG_GETTER_COMPOSER;
    case NCM_TAGS_FIELD_PERFORMER:
        return NCM_SONG_GETTER_PERFORMER;
    case NCM_TAGS_FIELD_DISC:
        return NCM_SONG_GETTER_DISC;
    case NCM_TAGS_FIELD_COMMENT:
        return NCM_SONG_GETTER_COMMENT;
    case NCM_TAGS_FIELD_LAST:
    default:
        return NCM_SONG_GETTER_NONE;
    }
}

static bool
ncm_mutable_song_write_callback(enum NcmTagsField field, int32 idx,
                                NcmStringView *value, void *user) {
    NcmMutableSong *song;

    song = (NcmMutableSong *)user;
    return ncm_mutable_song_get_tag(song, field, idx, value);
}

void
ncm_mutable_song_init(NcmMutableSong *song) {
    if (song == NULL) {
        return;
    }

    song->uri = NULL;
    song->directory = NULL;
    song->name = NULL;
    song->new_name = NULL;
    song->uri_len = 0;
    song->directory_len = 0;
    song->name_len = 0;
    song->new_name_len = 0;
    song->mtime = 0;
    song->duration = 0;
    song->is_from_database = false;
    song->tags = NULL;
    song->tags_len = 0;
    song->tags_cap = 0;
    return;
}

void
ncm_mutable_song_destroy(NcmMutableSong *song) {
    if (song == NULL) {
        return;
    }

    ncm_mutable_song_free_string(&song->uri, &song->uri_len);
    ncm_mutable_song_free_string(&song->directory, &song->directory_len);
    ncm_mutable_song_free_string(&song->name, &song->name_len);
    ncm_mutable_song_free_string(&song->new_name, &song->new_name_len);
    for (int32 i = 0; i < song->tags_len; i += 1) {
        ncm_mutable_song_tag_destroy(&song->tags[i]);
    }
    if (song->tags != NULL) {
        cbase_free(song->tags, song->tags_cap*SIZEOF(*song->tags));
    }
    ncm_mutable_song_init(song);
    return;
}

bool
ncm_mutable_song_copy(NcmMutableSong *dest, NcmMutableSong *source) {
    NcmMutableSong copy;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        ncm_mutable_song_destroy(dest);
        return true;
    }

    ncm_mutable_song_init(&copy);
    if (!ncm_mutable_song_set_uri(&copy, source->uri, source->uri_len)) {
        ncm_mutable_song_destroy(&copy);
        return false;
    }
    if (!ncm_mutable_song_set_directory(&copy, source->directory,
                                        source->directory_len)) {
        ncm_mutable_song_destroy(&copy);
        return false;
    }
    if (!ncm_mutable_song_set_name(&copy, source->name, source->name_len)) {
        ncm_mutable_song_destroy(&copy);
        return false;
    }
    if (!ncm_mutable_song_set_string(&copy.new_name, &copy.new_name_len,
                                     source->new_name,
                                     source->new_name_len)) {
        ncm_mutable_song_destroy(&copy);
        return false;
    }
    copy.mtime = source->mtime;
    copy.duration = source->duration;
    copy.is_from_database = source->is_from_database;

    for (int32 i = 0; i < source->tags_len; i += 1) {
        NcmMutableSongTag *tag;

        tag = ncm_mutable_song_add_tag(&copy, source->tags[i].field,
                                       source->tags[i].idx);
        if (tag == NULL) {
            ncm_mutable_song_destroy(&copy);
            return false;
        }
        if (!ncm_mutable_song_tag_copy(tag, &source->tags[i])) {
            ncm_mutable_song_destroy(&copy);
            return false;
        }
    }

    ncm_mutable_song_destroy(dest);
    *dest = copy;
    return true;
}

void
ncm_mutable_song_move(NcmMutableSong *dest, NcmMutableSong *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mutable_song_destroy(dest);
    if (source == NULL) {
        ncm_mutable_song_init(dest);
        return;
    }

    *dest = *source;
    ncm_mutable_song_init(source);
    return;
}

bool
ncm_mutable_song_set_uri(NcmMutableSong *song, char *uri, int32 uri_len) {
    if (song == NULL) {
        return false;
    }

    return ncm_mutable_song_set_string(&song->uri, &song->uri_len, uri,
                                       uri_len);
}

bool
ncm_mutable_song_set_directory(NcmMutableSong *song, char *directory,
                               int32 directory_len) {
    if (song == NULL) {
        return false;
    }

    return ncm_mutable_song_set_string(&song->directory, &song->directory_len,
                                       directory, directory_len);
}

bool
ncm_mutable_song_set_name(NcmMutableSong *song, char *name, int32 name_len) {
    if (song == NULL) {
        return false;
    }

    return ncm_mutable_song_set_string(&song->name, &song->name_len, name,
                                       name_len);
}

void
ncm_mutable_song_set_from_database(NcmMutableSong *song,
                                   bool is_from_database) {
    if (song == NULL) {
        return;
    }

    song->is_from_database = is_from_database;
    return;
}

bool
ncm_mutable_song_set_original_tag(NcmMutableSong *song,
                                  enum NcmTagsField field, int32 idx,
                                  char *value, int32 value_len) {
    NcmMutableSongTag *tag;

    if (song == NULL) {
        return false;
    }
    if (idx < 0) {
        return false;
    }
    if (field >= NCM_TAGS_FIELD_LAST) {
        return false;
    }

    tag = ncm_mutable_song_find_tag(song, field, idx);
    if (tag == NULL) {
        tag = ncm_mutable_song_add_tag(song, field, idx);
    }
    if (tag == NULL) {
        return false;
    }

    return ncm_mutable_song_set_string(&tag->original, &tag->original_len,
                                       value, value_len);
}

bool
ncm_mutable_song_set_tag(NcmMutableSong *song, enum NcmTagsField field,
                         int32 idx, char *value, int32 value_len) {
    NcmMutableSongTag *tag;

    if (song == NULL) {
        return false;
    }
    if (idx < 0) {
        return false;
    }
    if (field >= NCM_TAGS_FIELD_LAST) {
        return false;
    }
    if (value_len < 0) {
        return false;
    }

    tag = ncm_mutable_song_find_tag(song, field, idx);
    if (tag == NULL) {
        if (value_len == 0) {
            return true;
        }
        tag = ncm_mutable_song_add_tag(song, field, idx);
    }
    if (tag == NULL) {
        return false;
    }

    if (ncm_mutable_song_string_equal(tag->original, tag->original_len,
                                      value, value_len)) {
        ncm_mutable_song_free_string(&tag->value, &tag->value_len);
        tag->modified = false;
        return true;
    }

    if (!ncm_mutable_song_set_string(&tag->value, &tag->value_len, value,
                                     value_len)) {
        return false;
    }
    tag->modified = true;
    return true;
}

bool
ncm_mutable_song_set_tags(NcmMutableSong *song, enum NcmTagsField field,
                          char *value, int32 value_len, char *separator,
                          int32 separator_len) {
    int32 begin;
    int32 idx;

    if (song == NULL) {
        return false;
    }
    if (value == NULL) {
        return false;
    }
    if (value_len < 0) {
        return false;
    }
    if (field >= NCM_TAGS_FIELD_LAST) {
        return false;
    }

    if ((separator == NULL) || (separator_len <= 0)) {
        if (!ncm_mutable_song_set_tag(song, field, 0, value, value_len)) {
            return false;
        }
        return ncm_mutable_song_set_tag(song, field, 1, "", 0);
    }

    begin = 0;
    idx = 0;
    for (int32 i = 0; i <= value_len; i += 1) {
        bool at_end;
        bool at_separator;

        at_end = i == value_len;
        at_separator = false;
        if (!at_end && (i + separator_len <= value_len)) {
            at_separator = ncm_mutable_song_string_equal(
                value + i, separator_len, separator, separator_len);
        }

        if (at_end || at_separator) {
            if (!ncm_mutable_song_set_tag(song, field, idx, value + begin,
                                          i - begin)) {
                return false;
            }
            idx += 1;
            if (at_separator) {
                i += separator_len - 1;
            }
            begin = i + 1;
        }
    }

    return ncm_mutable_song_set_tag(song, field, idx, "", 0);
}

bool
ncm_mutable_song_get_tag(NcmMutableSong *song, enum NcmTagsField field,
                         int32 idx, NcmStringView *view) {
    NcmMutableSongTag *tag;

    ncm_mutable_song_view_init(view);
    if (song == NULL) {
        return false;
    }
    if (idx < 0) {
        return false;
    }
    if (field >= NCM_TAGS_FIELD_LAST) {
        return false;
    }

    tag = ncm_mutable_song_find_tag(song, field, idx);
    if (tag == NULL) {
        return false;
    }
    if (tag->modified) {
        view->data = tag->value;
        view->len = tag->value_len;
        return true;
    }

    view->data = tag->original;
    view->len = tag->original_len;
    return true;
}

NcmBuffer
ncm_mutable_song_get_numeric_tag_buffer(NcmMutableSong *song,
                                        enum NcmTagsField field,
                                        int32 idx) {
    NcmBuffer buffer;
    NcmStringView view;
    int32 len;

    ncm_buffer_init(&buffer);
    if (!ncm_mutable_song_get_tag(song, field, idx, &view)) {
        return buffer;
    }

    len = ncm_song_numeric_tag_len(view.data, view.len);
    ncm_buffer_reserve(&buffer, len);
    buffer.len = ncm_song_format_numeric_tag(buffer.data, buffer.cap,
                                             view.data, view.len);
    return buffer;
}

bool
ncm_mutable_song_load_originals_from_song(NcmMutableSong *dest,
                                          NcmSong *source) {
    NcmStringView view;

    if (dest == NULL) {
        return false;
    }
    if (source == NULL) {
        return false;
    }
    if (!ncm_song_uri_view(source, 0, &view)) {
        return false;
    }
    if (!ncm_mutable_song_set_uri(dest, view.data, view.len)) {
        return false;
    }
    if (ncm_song_directory_view(source, 0, &view)) {
        if (!ncm_mutable_song_set_directory(dest, view.data, view.len)) {
            return false;
        }
    } else if (!ncm_mutable_song_set_directory(dest, "", 0)) {
        return false;
    }
    if (ncm_song_name_view(source, 0, &view)) {
        if (!ncm_mutable_song_set_name(dest, view.data, view.len)) {
            return false;
        }
    } else if (!ncm_mutable_song_set_name(dest, "", 0)) {
        return false;
    }
    ncm_mutable_song_set_from_database(dest,
                                       ncm_song_is_from_database(source));

    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        enum NcmSongGetter getter;

        getter = ncm_mutable_song_field_getter((enum NcmTagsField)field);
        if (getter == NCM_SONG_GETTER_NONE) {
            continue;
        }
        for (uint32 i = 0; ; i += 1) {
            NcmBuffer buffer;

            buffer = ncm_song_getter_buffer(source, getter, i);
            if (buffer.len == 0) {
                ncm_buffer_destroy(&buffer);
                break;
            }
            if (!ncm_mutable_song_set_original_tag(
                    dest, (enum NcmTagsField)field, (int32)i,
                    buffer.data, buffer.len)) {
                ncm_buffer_destroy(&buffer);
                return false;
            }
            ncm_buffer_destroy(&buffer);
        }
    }

    return true;
}

bool
ncm_mutable_song_set_new_name(NcmMutableSong *song, char *new_name,
                              int32 new_name_len) {
    if (song == NULL) {
        return false;
    }
    if (new_name_len < 0) {
        return false;
    }

    if (new_name_len == 0) {
        ncm_mutable_song_free_string(&song->new_name, &song->new_name_len);
        return true;
    }
    if (ncm_mutable_song_string_equal(song->name, song->name_len, new_name,
                                      new_name_len)) {
        ncm_mutable_song_free_string(&song->new_name, &song->new_name_len);
        return true;
    }

    return ncm_mutable_song_set_string(&song->new_name, &song->new_name_len,
                                       new_name, new_name_len);
}

bool
ncm_mutable_song_get_new_name(NcmMutableSong *song, NcmStringView *view) {
    ncm_mutable_song_view_init(view);
    if (song == NULL) {
        return false;
    }

    view->data = song->new_name;
    view->len = song->new_name_len;
    return song->new_name != NULL;
}

void
ncm_mutable_song_set_duration(NcmMutableSong *song, uint32 duration) {
    if (song == NULL) {
        return;
    }

    song->duration = duration;
    return;
}

uint32
ncm_mutable_song_duration(NcmMutableSong *song) {
    if (song == NULL) {
        return 0;
    }

    return song->duration;
}

void
ncm_mutable_song_set_mtime(NcmMutableSong *song, int64 mtime) {
    if (song == NULL) {
        return;
    }

    song->mtime = mtime;
    return;
}

int64
ncm_mutable_song_mtime(NcmMutableSong *song) {
    if (song == NULL) {
        return 0;
    }

    return song->mtime;
}

bool
ncm_mutable_song_is_modified(NcmMutableSong *song) {
    if (song == NULL) {
        return false;
    }
    if (song->new_name != NULL) {
        return true;
    }

    for (int32 i = 0; i < song->tags_len; i += 1) {
        if (song->tags[i].modified) {
            return true;
        }
    }

    return false;
}

void
ncm_mutable_song_clear_modifications(NcmMutableSong *song) {
    if (song == NULL) {
        return;
    }

    ncm_mutable_song_free_string(&song->new_name, &song->new_name_len);
    for (int32 i = 0; i < song->tags_len; i += 1) {
        ncm_mutable_song_free_string(&song->tags[i].value,
                                     &song->tags[i].value_len);
        song->tags[i].modified = false;
    }
    return;
}

bool
ncm_mutable_song_write(NcmMutableSong *song, char *music_dir) {
    if (song == NULL) {
        return false;
    }
    if (song->uri == NULL) {
        return false;
    }

    return ncm_tags_write(music_dir, song->uri, song->is_from_database,
                          song->directory, song->new_name,
                          ncm_mutable_song_write_callback, song);
}
