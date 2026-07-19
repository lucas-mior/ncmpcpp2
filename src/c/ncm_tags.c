#if !defined(NCM_TAGS_C)
#define NCM_TAGS_C

#include "c/ncm_tags.h"

#include <mpd/client.h>
#include <stdio.h>

#include "c/ncm_taglib.h"
#include "cbase/util.c"

typedef struct NcmTagsFirstPropertyContext {
    NcmStringView *value;
    bool found;
} NcmTagsFirstPropertyContext;

typedef struct NcmTagsForwardContext {
    NcmTagsValueCallback *callback;
    void *user;
} NcmTagsForwardContext;

typedef struct NcmTagsMappedContext {
    struct mpd_song *song;
} NcmTagsMappedContext;

static void
ncm_tags_view_init(NcmStringView *view) {
    if (view == NULL) {
        return;
    }

    view->data = NULL;
    view->len = 0;
    return;
}

static void
ncm_tags_forward_value_callback(char *value, void *user) {
    NcmTagsForwardContext *context;

    context = (NcmTagsForwardContext *)user;
    if (context == NULL) {
        return;
    }
    if (context->callback == NULL) {
        return;
    }

    context->callback(value, optional_strlen32(value), context->user);
    return;
}

static void
ncm_tags_mapped_property_callback(char *name, char *value, void *user) {
    NcmTagsMappedContext *context;

    context = (NcmTagsMappedContext *)user;
    if (context == NULL) {
        return;
    }

    ncm_tags_set_attribute(context->song, name, value);
    return;
}

static char *
ncm_tags_build_file_path(char *music_dir, char *uri, bool is_from_database,
                         int32 *path_len) {
    char *path;
    int32 music_dir_len;
    int32 uri_len;
    int32 len;

    if (uri == NULL) {
        return NULL;
    }

    music_dir_len = 0;
    if (is_from_database) {
        music_dir_len = optional_strlen32(music_dir);
    }
    uri_len = optional_strlen32(uri);
    len = music_dir_len + uri_len;

    path = (char *)malloc2(len + 1);
    if (music_dir_len > 0) {
        memcpy64(path, music_dir, music_dir_len);
    }
    memcpy64(path + music_dir_len, uri, uri_len + 1);

    if (path_len) {
        *path_len = len;
    }
    return path;
}

static char *
ncm_tags_build_renamed_path(char *music_dir, char *directory, char *new_name,
                            bool is_from_database, int32 *path_len) {
    char *path;
    int32 music_dir_len;
    int32 directory_len;
    int32 new_name_len;
    int32 len;
    int32 offset;

    if (new_name == NULL) {
        return NULL;
    }

    music_dir_len = 0;
    if (is_from_database) {
        music_dir_len = optional_strlen32(music_dir);
    }
    directory_len = optional_strlen32(directory);
    new_name_len = optional_strlen32(new_name);
    len = music_dir_len + directory_len + 1 + new_name_len;

    path = (char *)malloc2(len + 1);
    offset = 0;
    if (music_dir_len > 0) {
        memcpy64(path + offset, music_dir, music_dir_len);
        offset += music_dir_len;
    }
    if (directory_len > 0) {
        memcpy64(path + offset, directory, directory_len);
        offset += directory_len;
    }
    path[offset] = '/';
    offset += 1;
    memcpy64(path + offset, new_name, new_name_len + 1);

    if (path_len) {
        *path_len = len;
    }
    return path;
}

static char *
ncm_tags_field_property(enum NcmTagsField field) {
    switch (field) {
    case NCM_TAGS_FIELD_TITLE:
        return "TITLE";
    case NCM_TAGS_FIELD_ARTIST:
        return "ARTIST";
    case NCM_TAGS_FIELD_ALBUM_ARTIST:
        return "ALBUMARTIST";
    case NCM_TAGS_FIELD_ALBUM:
        return "ALBUM";
    case NCM_TAGS_FIELD_DATE:
        return "DATE";
    case NCM_TAGS_FIELD_TRACK:
        return "TRACKNUMBER";
    case NCM_TAGS_FIELD_GENRE:
        return "GENRE";
    case NCM_TAGS_FIELD_COMPOSER:
        return "COMPOSER";
    case NCM_TAGS_FIELD_PERFORMER:
        return "PERFORMER";
    case NCM_TAGS_FIELD_DISC:
        return "DISCNUMBER";
    case NCM_TAGS_FIELD_COMMENT:
        return "COMMENT";
    case NCM_TAGS_FIELD_LAST:
        break;
    default:
        break;
    }

    return NULL;
}

static bool
ncm_tags_write_field(NcmTaglibFile *file, enum NcmTagsField field,
                     NcmTagsGetFieldCallback *callback, void *user) {
    char *property;

    property = ncm_tags_field_property(field);
    if (property == NULL) {
        return false;
    }

    ncm_taglib_clear_property(file, property);
    for (int32 i = 0; ; i += 1) {
        NcmStringView value;
        bool has_value;

        ncm_tags_view_init(&value);
        has_value = callback(field, i, &value, user);
        if (!has_value) {
            break;
        }
        if (value.data == NULL) {
            break;
        }
        if (value.len <= 0) {
            break;
        }

        ncm_taglib_append_property(file, property, value.data);
    }

    return true;
}

static void
ncm_tags_clear_write_aliases(NcmTaglibFile *file) {
    ncm_taglib_clear_property(file, "ALBUM ARTIST");
    ncm_taglib_clear_property(file, "TRACK");
    ncm_taglib_clear_property(file, "DISC");
    ncm_taglib_clear_property(file, "DESCRIPTION");
    return;
}

void
ncm_tags_set_attribute(struct mpd_song *song, char *name, char *value) {
    struct mpd_pair pair;

    if (song == NULL) {
        return;
    }
    if (name == NULL) {
        return;
    }
    if (value == NULL) {
        return;
    }

    pair.name = name;
    pair.value = value;
    mpd_song_feed(song, &pair);
    return;
}

enum NcmTagsReadResult
ncm_tags_read_lyrics(char *path, NcmTagsValueCallback *callback,
                     void *user) {
    NcmTaglibFile file;
    NcmTagsForwardContext context;
    bool found;

    if (callback == NULL) {
        return NCM_TAGS_READ_NOT_FOUND;
    }

    ncm_taglib_file_init(&file);
    if (!ncm_taglib_file_open(&file, path)) {
        return NCM_TAGS_READ_OPEN_FAILED;
    }

    context.callback = callback;
    context.user = user;
    found = ncm_taglib_read_property(&file, "LYRICS",
                                      ncm_tags_forward_value_callback,
                                      &context);
    if (!found) {
        found = ncm_taglib_read_property(&file, "UNSYNCEDLYRICS",
                                          ncm_tags_forward_value_callback,
                                          &context);
    }

    ncm_taglib_file_close(&file);
    if (found) {
        return NCM_TAGS_READ_OK;
    }
    return NCM_TAGS_READ_NOT_FOUND;
}

bool
ncm_tags_read_song(struct mpd_song *song) {
    NcmTaglibFile file;
    NcmTaglibAudioProperties properties;
    NcmTagsMappedContext context;
    char time_buffer[32];
    int32 written;
    bool found;

    if (song == NULL) {
        return false;
    }

    ncm_taglib_file_init(&file);
    if (!ncm_taglib_file_open(&file, (char *)mpd_song_get_uri(song))) {
        return false;
    }

    found = false;
    if (ncm_taglib_file_audio_properties(&file, &properties)) {
        written = SNPRINTF(time_buffer, "%d",
                           properties.length);
        if (written > 0) {
            ncm_tags_set_attribute(song, "Time", time_buffer);
            found = true;
        }
    }

    context.song = song;
    if (ncm_taglib_read_mapped_properties(&file,
                                          ncm_tags_mapped_property_callback,
                                          &context)) {
        found = true;
    }

    ncm_taglib_file_close(&file);
    ncm_taglib_clear_strings();
    return found;
}

bool
ncm_tags_write(char *music_dir, char *uri, bool is_from_database,
               char *directory, char *new_name,
               NcmTagsGetFieldCallback *callback, void *user) {
    NcmTaglibFile file;
    char *old_path;
    char *new_path;
    int32 old_path_len;
    int32 new_path_len;
    bool saved;

    if (callback == NULL) {
        return false;
    }

    old_path = ncm_tags_build_file_path(music_dir, uri, is_from_database,
                                        &old_path_len);
    if (old_path == NULL) {
        return false;
    }

    ncm_taglib_file_init(&file);
    if (!ncm_taglib_file_open(&file, old_path)) {
        free2(old_path, old_path_len + 1);
        return false;
    }

    ncm_tags_clear_write_aliases(&file);
    for (uint32 i = 0; i < NCM_TAGS_FIELD_LAST; i += 1) {
        ncm_tags_write_field(&file, (enum NcmTagsField)i, callback, user);
    }

    saved = ncm_taglib_file_save(&file);
    ncm_taglib_file_close(&file);
    if (!saved) {
        free2(old_path, old_path_len + 1);
        return false;
    }

    if (new_name && (new_name[0] != '\0')) {
        new_path = ncm_tags_build_renamed_path(music_dir, directory, new_name,
                                               is_from_database,
                                               &new_path_len);
        if (new_path == NULL) {
            free2(old_path, old_path_len + 1);
            return false;
        }
        if (rename(old_path, new_path) != 0) {
            free2(new_path, new_path_len + 1);
            free2(old_path, old_path_len + 1);
            return false;
        }
        free2(new_path, new_path_len + 1);
    }

    free2(old_path, old_path_len + 1);
    return true;
}

#endif /* NCM_TAGS_C */
