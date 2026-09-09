#if !defined(NCM_MUTABLE_SONG_C)
#define NCM_MUTABLE_SONG_C

#include "cbase.h"

#include "c/ncm_c.h"

static MutableSongTag *
ncm_mutable_song_find_tag(MutableSong *song, enum NcmTagsField field,
                          int32 idx) {
    ASSERT((song != NULL) && (idx >= 0));

    for (int32 i = 0; i < song->tags_len; i += 1) {
        MutableSongTag *tag = &song->tags[i];

        if ((tag->field == field) && (tag->idx == idx)) {
            return tag;
        }
    }

    return NULL;
}

static MutableSongTag *
ncm_mutable_song_add_tag(MutableSong *song, enum NcmTagsField field,
                         int32 idx) {
    MutableSongTag *tag;

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
            MutableSongTag *new_tag = &song->tags[i];

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
ncm_mutable_song_set_original_tag_unchecked(MutableSong *song,
                                            enum NcmTagsField field, int32 idx,
                                            char *value, int32 value_len) {
    MutableSongTag *tag;

    if ((tag = ncm_mutable_song_find_tag(song, field, idx)) == NULL) {
        tag = ncm_mutable_song_add_tag(song, field, idx);
    }
    stupid_string_set(&tag->original, &tag->original_len, value, value_len);
    return;
}

static void
ncm_mutable_song_set_tag_unchecked(MutableSong *song,
                                   enum NcmTagsField field, int32 idx,
                                   char *value, int32 value_len) {
    MutableSongTag *tag;

    if ((tag = ncm_mutable_song_find_tag(song, field, idx)) == NULL) {
        if (value_len <= 0) {
            return;
        }
        tag = ncm_mutable_song_add_tag(song, field, idx);
    }

    if (optional_strequal(tag->original, tag->original_len, value, value_len)) {
        stupid_string_free(&tag->value, &tag->value_len);
        tag->modified = false;
        return;
    }

    stupid_string_set(&tag->value, &tag->value_len, value, value_len);
    tag->modified = true;
    return;
}

static void
ncm_mutable_song_tag_destroy(MutableSongTag *tag) {
    ASSERT(tag != NULL);

    stupid_string_free(&tag->original, &tag->original_len);
    stupid_string_free(&tag->value, &tag->value_len);
    tag->idx = 0;
    tag->field = NCM_TAGS_FIELD_COUNT;
    tag->modified = false;
    return;
}

static void
ncm_mutable_song_destroy_unchecked(MutableSong *song) {
    stupid_string_free(&song->uri, &song->uri_len);
    stupid_string_free(&song->directory, &song->directory_len);
    stupid_string_free(&song->name, &song->name_len);
    stupid_string_free(&song->new_name, &song->new_name_len);

    for (int32 i = 0; i < song->tags_len; i += 1) {
        ncm_mutable_song_tag_destroy(&song->tags[i]);
    }

    free2(song->tags, song->tags_cap*SIZEOF(*song->tags));
    *song = (MutableSong){0};
    return;
}

static bool
ncm_mutable_song_has_tag_view_unchecked(MutableSong *song,
                                        enum NcmTagsField field, int32 idx,
                                        StringView *view) {
    MutableSongTag *tag;

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
                                StringView *value, void *user) {
    MutableSong *song = user;
    return ncm_mutable_song_has_tag_view_unchecked(song, field, idx, value);
}

void
ncm_mutable_song_destroy(MutableSong *song) {
    if (song == NULL) {
        return;
    }

    ncm_mutable_song_destroy_unchecked(song);
    return;
}

int32
ncm_mutable_song_copy(MutableSong *dest, MutableSong *source) {
    MutableSong copy = {0};

    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        ncm_mutable_song_destroy_unchecked(dest);
        return 0;
    }

    stupid_string_set(&copy.uri, &copy.uri_len, source->uri, source->uri_len);
    stupid_string_set(&copy.directory, &copy.directory_len,
                      source->directory, source->directory_len);
    stupid_string_set(&copy.name, &copy.name_len,
                      source->name, source->name_len);
    stupid_string_set(&copy.new_name, &copy.new_name_len,
                      source->new_name, source->new_name_len);
    copy.mtime = source->mtime;
    copy.duration = source->duration;
    copy.is_from_database = source->is_from_database;

    for (int32 i = 0; i < source->tags_len; i += 1) {
        MutableSongTag *source_tag = &source->tags[i];
        MutableSongTag *tag = ncm_mutable_song_add_tag(&copy,
                                                       source_tag->field,
                                                       source_tag->idx);
        ncm_mutable_song_tag_destroy(tag);
        tag->field = source_tag->field;
        tag->idx = source_tag->idx;
        tag->modified = source_tag->modified;
        stupid_string_set(&tag->original, &tag->original_len,
                          source_tag->original, source_tag->original_len);
        stupid_string_set(&tag->value, &tag->value_len,
                          source_tag->value, source_tag->value_len);
    }

    ncm_mutable_song_destroy_unchecked(dest);
    *dest = copy;
    return 0;
}

void
ncm_mutable_song_move(MutableSong *dest, MutableSong *source) {
    if (dest == NULL) {
        return;
    }
    if (dest == source) {
        return;
    }

    ncm_mutable_song_destroy_unchecked(dest);
    if (source == NULL) {
        *dest = (MutableSong){0};
        return;
    }

    *dest = *source;
    *source = (MutableSong){0};
    return;
}

int32
ncm_mutable_song_set_tag(MutableSong *song, enum NcmTagsField field,
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
ncm_mutable_song_set_tags(MutableSong *song, enum NcmTagsField field,
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
            at_separator = optional_strequal(value + i, separator_len,
                                             separator, separator_len);
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
ncm_mutable_song_has_tag_view(MutableSong *song,
                              enum NcmTagsField field, int32 idx,
                              StringView *view) {
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
ncm_mutable_song_get_tag_buffer_unchecked(MutableSong *song,
                                          enum NcmTagsField field, int32 idx,
                                          StrBuilder *buffer) {
    StringView view;

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
ncm_mutable_song_get_tag_buffer(MutableSong *song,
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
ncm_mutable_song_tags_buffer(MutableSong *song, enum NcmTagsField field,
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

                ncm_mutable_song_get_tag_buffer_unchecked(song, field, j,
                                                          &previous);
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
ncm_mutable_song_load_originals_from_song(MutableSong *dest,
                                          NcmSong *source) {
    StringView view;

    if (dest == NULL) {
        return -EINVAL;
    }
    if (source == NULL) {
        return -EINVAL;
    }
    if (!ncm_song_has_uri_view(source, 0, &view)) {
        return -NCM_ERROR_NOT_FOUND;
    }
    stupid_string_set(&dest->uri, &dest->uri_len, view.data, view.len);
    if (ncm_song_has_directory_view(source, 0, &view)) {
        stupid_string_set(&dest->directory, &dest->directory_len,
                          view.data, view.len);
    } else {
        stupid_string_set(&dest->directory, &dest->directory_len, "", 0);
    }
    if (ncm_song_has_name_view(source, 0, &view)) {
        stupid_string_set(&dest->name, &dest->name_len, view.data, view.len);
    } else {
        stupid_string_set(&dest->name, &dest->name_len, "", 0);
    }
    dest->is_from_database = ncm_song_is_from_database(source);

    for (uint32 field = 0; field < NCM_TAGS_FIELD_COUNT; field += 1) {
        enum SongGetter getter = ncm_tags_field_to_song_getter(field);

        if (getter == SONG_GETTER_NONE) {
            continue;
        }
        for (int32 i = 0; ; i += 1) {
            StrBuilder buffer = ncm_song_getter_buffer(source, getter, i);

            if (buffer.len <= 0) {
                sb_free(&buffer);
                break;
            }
            ncm_mutable_song_set_original_tag_unchecked(dest,
                (enum NcmTagsField)field, i, buffer.data, buffer.len);
            sb_free(&buffer);
        }
    }

    return 0;
}

int32
ncm_mutable_song_set_new_name(MutableSong *song, char *new_name,
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
        stupid_string_free(&song->new_name, &song->new_name_len);
        return 0;
    }
    if (optional_strequal(song->name, song->name_len, new_name, new_name_len)) {
        stupid_string_free(&song->new_name, &song->new_name_len);
        return 0;
    }

    stupid_string_set(&song->new_name, &song->new_name_len,
                      new_name, new_name_len);
    return 0;
}

bool
ncm_mutable_song_has_new_name_view(MutableSong *song, StringView *view) {
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

int32
ncm_mutable_song_duration(MutableSong *song) {
    if (song == NULL) {
        return 0;
    }

    return song->duration;
}

int32
ncm_mutable_song_mtime(MutableSong *song) {
    if (song == NULL) {
        return 0;
    }

    return song->mtime;
}

bool
ncm_mutable_song_is_modified(MutableSong *song) {
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
ncm_mutable_song_clear_modifications(MutableSong *song) {
    if (song == NULL) {
        return;
    }

    stupid_string_free(&song->new_name, &song->new_name_len);
    for (int32 i = 0; i < song->tags_len; i += 1) {
        stupid_string_free(&song->tags[i].value, &song->tags[i].value_len);
        song->tags[i].modified = false;
    }
    return;
}

int32
ncm_mutable_song_write(MutableSong *song, char *music_dir) {
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
