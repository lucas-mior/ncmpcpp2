#if !defined(NCM_MUTABLE_SONG_C)
#define NCM_MUTABLE_SONG_C

#include "cbase.h"

#include "c/ncm_c.h"

static void
ncm_mutable_song_free_string(char **string, int32 *string_len) {
    ASSERT((string != NULL) && (string_len != NULL));

    free2(*string, *string_len + 1);
    *string = NULL;
    *string_len = 0;
    return;
}

static void
ncm_mutable_song_set_string(char **dest, int32 *dest_len,
                            char *source, int32 source_len) {
    char *copy;

    ASSERT((dest != NULL) && (dest_len != NULL));
    ASSERT(source_len >= 0);
    ASSERT((source != NULL) || (source_len == 0));

    copy = NULL;
    if (source != NULL) {
        copy = (char *)malloc2(source_len + 1);
        if (source_len > 0) {
            memcpy64(copy, source, source_len);
        }
        copy[source_len] = '\0';
    }

    ncm_mutable_song_free_string(dest, dest_len);
    *dest = copy;
    if (copy != NULL) {
        *dest_len = source_len;
    }
    return;
}

static NcmMutableSongTag *
ncm_mutable_song_find_tag(NcmMutableSong *song, enum NcmTagsField field,
                          int32 idx) {
    ASSERT((song != NULL) && (idx >= 0));

    for (int32 i = 0; i < song->tags_len; i += 1) {
        NcmMutableSongTag *tag = &song->tags[i];

        if ((tag->field == field) && (tag->idx == idx)) {
            return tag;
        }
    }

    return NULL;
}

static NcmMutableSongTag *
ncm_mutable_song_add_tag(NcmMutableSong *song, enum NcmTagsField field,
                         int32 idx) {
    NcmMutableSongTag *tag;

    ASSERT((song != NULL) && (idx >= 0));
    if (song->tags_len >= song->tags_cap) {
        int32 new_cap;

        if (song->tags_cap <= 0) {
            new_cap = 16;
        } else {
            new_cap = song->tags_cap*2;
        }

        song->tags = realloc2(song->tags,
                              song->tags_cap, new_cap, SIZEOF(*song->tags));
        for (int32 i = song->tags_cap; i < new_cap; i += 1) {
            NcmMutableSongTag *new_tag;

            new_tag = &song->tags[i];
            new_tag->original = NULL;
            new_tag->value = NULL;
            new_tag->original_len = 0;
            new_tag->value_len = 0;
            new_tag->idx = 0;
            new_tag->field = NCM_TAGS_FIELD_COUNT;
            new_tag->modified = false;
        }
        song->tags_cap = new_cap;
    }

    tag = &song->tags[song->tags_len];
    song->tags_len += 1;
    tag->field = field;
    tag->idx = idx;
    return tag;
}

static void
ncm_mutable_song_set_original_tag_unchecked(NcmMutableSong *song,
                                            enum NcmTagsField field,
                                            int32 idx,
                                            char *value, int32 value_len) {
    NcmMutableSongTag *tag;

    if ((tag = ncm_mutable_song_find_tag(song, field, idx)) == NULL) {
        tag = ncm_mutable_song_add_tag(song, field, idx);
    }
    ncm_mutable_song_set_string(&tag->original, &tag->original_len,
                                value, value_len);
    return;
}

static void
ncm_mutable_song_set_tag_unchecked(NcmMutableSong *song,
                                   enum NcmTagsField field, int32 idx,
                                   char *value, int32 value_len) {
    NcmMutableSongTag *tag;

    if ((tag = ncm_mutable_song_find_tag(song, field, idx)) == NULL) {
        if (value_len <= 0) {
            return;
        }
        tag = ncm_mutable_song_add_tag(song, field, idx);
    }

    if (optional_strequal(tag->original, tag->original_len,
                          value, value_len)) {
        ncm_mutable_song_free_string(&tag->value, &tag->value_len);
        tag->modified = false;
        return;
    }

    ncm_mutable_song_set_string(&tag->value, &tag->value_len,
                                value, value_len);
    tag->modified = true;
    return;
}

static void
ncm_mutable_song_tag_destroy(NcmMutableSongTag *tag) {
    ASSERT(tag != NULL);

    ncm_mutable_song_free_string(&tag->original, &tag->original_len);
    ncm_mutable_song_free_string(&tag->value, &tag->value_len);
    tag->idx = 0;
    tag->field = NCM_TAGS_FIELD_COUNT;
    tag->modified = false;
    return;
}

static void
ncm_mutable_song_destroy_unchecked(NcmMutableSong *song) {
    ncm_mutable_song_free_string(&song->uri, &song->uri_len);
    ncm_mutable_song_free_string(&song->directory, &song->directory_len);
    ncm_mutable_song_free_string(&song->name, &song->name_len);
    ncm_mutable_song_free_string(&song->new_name, &song->new_name_len);

    for (int32 i = 0; i < song->tags_len; i += 1) {
        ncm_mutable_song_tag_destroy(&song->tags[i]);
    }

    free2(song->tags, song->tags_cap*SIZEOF(*song->tags));
    *song = (NcmMutableSong){0};
    return;
}

static bool
ncm_mutable_song_has_tag_view_unchecked(NcmMutableSong *song,
                                        enum NcmTagsField field, int32 idx,
                                        NcmStringView *view) {
    NcmMutableSongTag *tag;

    if ((tag = ncm_mutable_song_find_tag(song, field, idx)) == NULL) {
        return false;
    }
    if (tag->modified) {
        ncm_string_view_set(view, tag->value, tag->value_len);
        return true;
    }

    ncm_string_view_set(view, tag->original, tag->original_len);
    return true;
}

static bool
ncm_mutable_song_write_callback(enum NcmTagsField field, int32 idx,
                                NcmStringView *value, void *user) {
    NcmMutableSong *song;

    song = (NcmMutableSong *)user;
    return ncm_mutable_song_has_tag_view_unchecked(song, field, idx, value);
}

void
ncm_mutable_song_destroy(NcmMutableSong *song) {
    if (song == NULL) {
        return;
    }

    ncm_mutable_song_destroy_unchecked(song);
    return;
}

int32
ncm_mutable_song_copy(NcmMutableSong *dest, NcmMutableSong *source) {
    NcmMutableSong copy = {0};

    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        ncm_mutable_song_destroy_unchecked(dest);
        return 0;
    }

    ncm_mutable_song_set_string(&copy.uri, &copy.uri_len,
                                source->uri, source->uri_len);
    ncm_mutable_song_set_string(&copy.directory, &copy.directory_len,
                                source->directory, source->directory_len);
    ncm_mutable_song_set_string(&copy.name, &copy.name_len,
                                source->name, source->name_len);
    ncm_mutable_song_set_string(&copy.new_name, &copy.new_name_len,
                                source->new_name, source->new_name_len);
    copy.mtime = source->mtime;
    copy.duration = source->duration;
    copy.is_from_database = source->is_from_database;

    for (int32 i = 0; i < source->tags_len; i += 1) {
        NcmMutableSongTag *source_tag = &source->tags[i];
        NcmMutableSongTag *tag = ncm_mutable_song_add_tag(&copy,
                                                          source_tag->field,
                                                          source_tag->idx);
        ncm_mutable_song_tag_destroy(tag);
        tag->field = source_tag->field;
        tag->idx = source_tag->idx;
        tag->modified = source_tag->modified;
        ncm_mutable_song_set_string(&tag->original, &tag->original_len,
                                    source_tag->original,
                                    source_tag->original_len);
        ncm_mutable_song_set_string(&tag->value, &tag->value_len,
                                    source_tag->value,
                                    source_tag->value_len);
    }

    ncm_mutable_song_destroy_unchecked(dest);
    *dest = copy;
    return 0;
}

void
ncm_mutable_song_move(NcmMutableSong *dest, NcmMutableSong *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mutable_song_destroy_unchecked(dest);
    if (source == NULL) {
        *dest = (NcmMutableSong){0};
        return;
    }

    *dest = *source;
    *source = (NcmMutableSong){0};
    return;
}

int32
ncm_mutable_song_set_uri(NcmMutableSong *song, char *uri, int32 uri_len) {
    if (song == NULL) {
        return -EINVAL;
    }
    if (uri_len < 0) {
        return -EINVAL;
    }
    if ((uri == NULL) && (uri_len > 0)) {
        return -EINVAL;
    }

    ncm_mutable_song_set_string(&song->uri, &song->uri_len, uri, uri_len);
    return 0;
}

int32
ncm_mutable_song_set_directory(NcmMutableSong *song, char *directory,
                                int32 directory_len) {
    if (song == NULL) {
        return -EINVAL;
    }
    if (directory_len < 0) {
        return -EINVAL;
    }
    if ((directory == NULL) && (directory_len > 0)) {
        return -EINVAL;
    }

    ncm_mutable_song_set_string(&song->directory, &song->directory_len,
                                directory, directory_len);
    return 0;
}

int32
ncm_mutable_song_set_name(NcmMutableSong *song, char *name, int32 name_len) {
    if (song == NULL) {
        return -EINVAL;
    }
    if (name_len < 0) {
        return -EINVAL;
    }
    if ((name == NULL) && (name_len > 0)) {
        return -EINVAL;
    }

    ncm_mutable_song_set_string(&song->name, &song->name_len, name, name_len);
    return 0;
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

int32
ncm_mutable_song_set_original_tag(NcmMutableSong *song,
                                  enum NcmTagsField field, int32 idx,
                                  char *value, int32 value_len) {
    if (song == NULL) {
        return -EINVAL;
    }
    if (idx < 0) {
        return -EINVAL;
    }
    if (field >= NCM_TAGS_FIELD_COUNT) {
        return -EINVAL;
    }
    if (value_len < 0) {
        return -EINVAL;
    }
    if ((value == NULL) && (value_len > 0)) {
        return -EINVAL;
    }

    ncm_mutable_song_set_original_tag_unchecked(song, field, idx,
                                                value, value_len);
    return 0;
}

int32
ncm_mutable_song_set_tag(NcmMutableSong *song, enum NcmTagsField field,
                         int32 idx, char *value, int32 value_len) {
    if (song == NULL) {
        return -EINVAL;
    }
    if (idx < 0) {
        return -EINVAL;
    }
    if (field >= NCM_TAGS_FIELD_COUNT) {
        return -EINVAL;
    }
    if (value_len < 0) {
        return -EINVAL;
    }
    if ((value == NULL) && (value_len > 0)) {
        return -EINVAL;
    }

    ncm_mutable_song_set_tag_unchecked(song, field, idx, value, value_len);
    return 0;
}

int32
ncm_mutable_song_set_tags(NcmMutableSong *song, enum NcmTagsField field,
                          char *value, int32 value_len, char *separator,
                          int32 separator_len) {
    int32 begin;
    int32 idx;

    if (song == NULL) {
        return -EINVAL;
    }
    if (value == NULL) {
        return -EINVAL;
    }
    if (value_len < 0) {
        return -EINVAL;
    }
    if (field >= NCM_TAGS_FIELD_COUNT) {
        return -EINVAL;
    }

    if ((separator == NULL) || (separator_len <= 0)) {
        ncm_mutable_song_set_tag_unchecked(song, field, 0, value, value_len);
        ncm_mutable_song_set_tag_unchecked(song, field, 1, "", 0);
        return 0;
    }

    begin = 0;
    idx = 0;
    for (int32 i = 0; i <= value_len; i += 1) {
        bool at_end = (i == value_len);
        bool at_separator = false;

        if (!at_end && (i + separator_len <= value_len)) {
            at_separator = optional_strequal(
                value + i, separator_len, separator, separator_len);
        }

        if (at_end || at_separator) {
            ncm_mutable_song_set_tag_unchecked(song, field, idx,
                                               value + begin, i - begin);
            idx += 1;
            if (at_separator) {
                i += separator_len - 1;
            }
            begin = i + 1;
        }
    }

    ncm_mutable_song_set_tag_unchecked(song, field, idx, "", 0);
    return 0;
}

bool
ncm_mutable_song_has_tag_view(NcmMutableSong *song,
                              enum NcmTagsField field, int32 idx,
                              NcmStringView *view) {
    if (view == NULL) {
        return false;
    }
    ncm_string_view_clear(view);
    if (song == NULL) {
        return false;
    }
    if (idx < 0) {
        return false;
    }
    if (field >= NCM_TAGS_FIELD_COUNT) {
        return false;
    }

    return ncm_mutable_song_has_tag_view_unchecked(song, field, idx, view);
}

static void
ncm_mutable_song_get_tag_buffer_unchecked(NcmMutableSong *song,
                                          enum NcmTagsField field, int32 idx,
                                          StrBuilder *buffer) {
    NcmStringView view;

    sb_clear(buffer);
    if (field == NCM_TAGS_FIELD_TRACK) {
        int32 len;

        if (!ncm_mutable_song_has_tag_view_unchecked(song, field, idx, &view)) {
            return;
        }

        len = ncm_song_numeric_tag_len(view.data, view.len);
        sb_reserve(buffer, len);
        buffer->len = ncm_song_format_numeric_tag(buffer->data, buffer->cap,
                                                  view.data, view.len);
        return;
    }
    if (!ncm_mutable_song_has_tag_view_unchecked(song, field, idx, &view)) {
        return;
    }

    SB_APPEND(buffer, view.data, view.len);
    return;
}

void
ncm_mutable_song_get_tag_buffer(NcmMutableSong *song,
                                enum NcmTagsField field, int32 idx,
                                StrBuilder *buffer) {
    if (buffer == NULL) {
        return;
    }
    if (song == NULL) {
        sb_clear(buffer);
        return;
    }
    if (idx < 0) {
        sb_clear(buffer);
        return;
    }
    if (field >= NCM_TAGS_FIELD_COUNT) {
        sb_clear(buffer);
        return;
    }

    ncm_mutable_song_get_tag_buffer_unchecked(song, field, idx, buffer);
    return;
}

StrBuilder
ncm_mutable_song_tags_buffer(NcmMutableSong *song,
                             enum NcmTagsField field,
                             char *separator, int32 separator_len,
                             bool show_duplicates) {
    StrBuilder result = {0};

    if (song == NULL) {
        return result;
    }
    if (field >= NCM_TAGS_FIELD_COUNT) {
        return result;
    }
    if ((separator == NULL) || (separator_len < 0)) {
        separator = "";
        separator_len = 0;
    }

    for (int32 i = 0; ; i += 1) {
        StrBuilder tag = {0};
        bool already_present;

        ncm_mutable_song_get_tag_buffer_unchecked(song, field, i, &tag);
        if (tag.len <= 0) {
            sb_free(&tag);
            break;
        }

        already_present = false;
        if (!show_duplicates) {
            for (int32 j = 0; j < i; j += 1) {
                StrBuilder previous = {0};

                ncm_mutable_song_get_tag_buffer_unchecked(
                    song, field, j, &previous);
                if (optional_strequal(previous.data, previous.len,
                                      tag.data, tag.len)) {
                    already_present = true;
                }

                sb_free(&previous);
                if (already_present) {
                    break;
                }
            }
        }

        if (!already_present) {
            if (result.len > 0) {
                SB_APPEND(&result, separator, separator_len);
            }
            SB_APPEND(&result, tag.data, tag.len);
        }
        sb_free(&tag);
    }

    return result;
}

int32
ncm_mutable_song_load_originals_from_song(NcmMutableSong *dest,
                                          NcmSong *source) {
    NcmStringView view;

    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }
    if (!ncm_song_has_uri_view(source, 0, &view)) {
        return -NCM_ERROR_NOT_FOUND;
    }
    ncm_mutable_song_set_string(&dest->uri, &dest->uri_len,
                                view.data, view.len);
    if (ncm_song_has_directory_view(source, 0, &view)) {
        ncm_mutable_song_set_string(&dest->directory, &dest->directory_len,
                                    view.data, view.len);
    } else {
        ncm_mutable_song_set_string(&dest->directory, &dest->directory_len,
                                    "", 0);
    }
    if (ncm_song_has_name_view(source, 0, &view)) {
        ncm_mutable_song_set_string(&dest->name, &dest->name_len,
                                    view.data, view.len);
    } else {
        ncm_mutable_song_set_string(&dest->name, &dest->name_len, "", 0);
    }
    dest->is_from_database = ncm_song_is_from_database(source);

    for (uint32 field = 0; field < NCM_TAGS_FIELD_COUNT; field += 1) {
        enum NcmSongGetter getter = ncm_tags_field_to_song_getter(field);

        if (getter == NCM_SONG_GETTER_NONE) {
            continue;
        }
        for (int32 i = 0; ; i += 1) {
            StrBuilder buffer = ncm_song_getter_buffer(source, getter, i);

            if (buffer.len <= 0) {
                sb_free(&buffer);
                break;
            }
            ncm_mutable_song_set_original_tag_unchecked(
                dest, (enum NcmTagsField)field, i, buffer.data, buffer.len);
            sb_free(&buffer);
        }
    }

    return 0;
}

int32
ncm_mutable_song_set_new_name(NcmMutableSong *song, char *new_name,
                              int32 new_name_len) {
    if (song == NULL) {
        return -EINVAL;
    }
    if (new_name_len < 0) {
        return -EINVAL;
    }
    if ((new_name == NULL) && (new_name_len > 0)) {
        return -EINVAL;
    }

    if (new_name_len <= 0) {
        ncm_mutable_song_free_string(&song->new_name, &song->new_name_len);
        return 0;
    }
    if (optional_strequal(song->name, song->name_len, new_name,
                          new_name_len)) {
        ncm_mutable_song_free_string(&song->new_name, &song->new_name_len);
        return 0;
    }

    ncm_mutable_song_set_string(&song->new_name, &song->new_name_len,
                                new_name, new_name_len);
    return 0;
}

bool
ncm_mutable_song_has_new_name_view(NcmMutableSong *song,
                                   NcmStringView *view) {
    if (view == NULL) {
        return false;
    }
    ncm_string_view_clear(view);
    if (song == NULL) {
        return false;
    }
    if (song->new_name == NULL) {
        return false;
    }

    ncm_string_view_set(view, song->new_name, song->new_name_len);
    return true;
}

void
ncm_mutable_song_set_duration(NcmMutableSong *song, int32 duration) {
    if (song == NULL) {
        return;
    }

    song->duration = duration;
    return;
}

int32
ncm_mutable_song_duration(NcmMutableSong *song) {
    if (song == NULL) {
        return 0;
    }

    return song->duration;
}

void
ncm_mutable_song_set_mtime(NcmMutableSong *song, int32 mtime) {
    if (song == NULL) {
        return;
    }

    song->mtime = mtime;
    return;
}

int32
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
    if (song->new_name) {
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

int32
ncm_mutable_song_write(NcmMutableSong *song, char *music_dir) {
    if (song == NULL) {
        return -EINVAL;
    }
    if (song->uri == NULL) {
        return -EINVAL;
    }

    return ncm_tags_write(music_dir, song->uri, song->is_from_database,
                          song->directory, song->new_name,
                          ncm_mutable_song_write_callback, song);
}

#endif /* NCM_MUTABLE_SONG_C */
