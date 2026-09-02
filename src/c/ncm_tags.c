#if !defined(NCM_TAGS_C)
#define NCM_TAGS_C

#include "cbase.h"

#include <mpd/client.h>

#include "c/ncm_c.h"

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
    case NCM_TAGS_FIELD_COUNT:
        break;
    default:
        break;
    }

    return NULL;
}

static int32
ncm_tags_write_field(NcmTaglibFile *file, enum NcmTagsField field,
                     NcmTagsGetFieldCallback *callback, void *user) {
    char *property;
    int32 status;

    if ((property = ncm_tags_field_property(field)) == NULL) {
        return -EINVAL;
    }

    if ((status = ncm_taglib_clear_property(file, property)) < 0) {
        return status;
    }
    for (int32 i = 0; ; i += 1) {
        NcmStringView value;

        value = (NcmStringView){0};
        if (!callback(field, i, &value, user)) {
            break;
        }
        if (value.data == NULL) {
            break;
        }
        if (value.len <= 0) {
            break;
        }

        if ((status = ncm_taglib_append_property(file, property,
                                                 value.data)) < 0) {
            return status;
        }
    }

    return 0;
}

static int32
ncm_tags_clear_write_aliases(NcmTaglibFile *file) {
    int32 status;

    if ((status = ncm_taglib_clear_property(file, "ALBUM ARTIST")) < 0) {
        return status;
    }
    if ((status = ncm_taglib_clear_property(file, "TRACK")) < 0) {
        return status;
    }
    if ((status = ncm_taglib_clear_property(file, "DISC")) < 0) {
        return status;
    }
    if ((status = ncm_taglib_clear_property(file, "DESCRIPTION")) < 0) {
        return status;
    }

    return 0;
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
    int32 count;

    if (callback == NULL) {
        return NCM_TAGS_READ_NOT_FOUND;
    }

    file = (NcmTaglibFile){0};
    if (ncm_taglib_file_open(&file, path) < 0) {
        return NCM_TAGS_READ_OPEN_FAILED;
    }

    context.callback = callback;
    context.user = user;
    if ((count = ncm_taglib_read_property(&file, "LYRICS",
                                          ncm_tags_forward_value_callback,
                                          &context)) < 0) {
        ncm_taglib_file_close(&file);
        return NCM_TAGS_READ_OPEN_FAILED;
    }
    if (count == 0) {
        count = ncm_taglib_read_property(&file, "UNSYNCEDLYRICS",
                                         ncm_tags_forward_value_callback,
                                         &context);
        if (count < 0) {
            ncm_taglib_file_close(&file);
            return NCM_TAGS_READ_OPEN_FAILED;
        }
    }

    ncm_taglib_file_close(&file);
    if (count > 0) {
        return NCM_TAGS_READ_OK;
    }
    return NCM_TAGS_READ_NOT_FOUND;
}

int32
ncm_tags_read_song(struct mpd_song *song) {
    NcmTaglibFile file;
    NcmTaglibAudioProperties properties;
    NcmTagsMappedContext context;
    char time_buffer[32];
    int32 written;
    int32 count;
    int32 status;

    if (song == NULL) {
        return -EINVAL;
    }

    file = (NcmTaglibFile){0};
    status = ncm_taglib_file_open(&file, (char *)mpd_song_get_uri(song));
    if (status < 0) {
        return status;
    }

    count = 0;
    if (ncm_taglib_file_audio_properties(&file, &properties) == 0) {
        written = SNPRINTF(time_buffer, "%d", properties.length);
        if (written > 0) {
            ncm_tags_set_attribute(song, "Time", time_buffer);
            count += 1;
        }
    }

    context.song = song;
    status = ncm_taglib_read_mapped_properties(
        &file, ncm_tags_mapped_property_callback, &context);
    if (status < 0) {
        ncm_taglib_file_close(&file);
        ncm_taglib_clear_strings();
        return status;
    }
    count += status;

    ncm_taglib_file_close(&file);
    ncm_taglib_clear_strings();
    return count;
}

int32
ncm_tags_write(char *music_dir, char *uri, bool is_from_database,
               char *directory, char *new_name,
               NcmTagsGetFieldCallback *callback, void *user) {
    NcmTaglibFile file;
    char *old_path;
    char *new_path;
    int32 old_path_len;
    int32 new_path_len;
    int32 status;

    if (callback == NULL) {
        return -EINVAL;
    }

    old_path = ncm_tags_build_file_path(music_dir, uri, is_from_database,
                                        &old_path_len);
    if (old_path == NULL) {
        return -EINVAL;
    }

    file = (NcmTaglibFile){0};
    if ((status = ncm_taglib_file_open(&file, old_path)) < 0) {
        free2(old_path, old_path_len + 1);
        return status;
    }

    if ((status = ncm_tags_clear_write_aliases(&file)) < 0) {
        ncm_taglib_file_close(&file);
        free2(old_path, old_path_len + 1);
        return status;
    }
    for (uint32 i = 0; i < NCM_TAGS_FIELD_COUNT; i += 1) {
        status = ncm_tags_write_field(&file, (enum NcmTagsField)i,
                                      callback, user);
        if (status < 0) {
            ncm_taglib_file_close(&file);
            free2(old_path, old_path_len + 1);
            return status;
        }
    }

    status = ncm_taglib_file_save(&file);
    ncm_taglib_file_close(&file);
    if (status < 0) {
        free2(old_path, old_path_len + 1);
        return status;
    }

    if ((new_name != NULL) && (new_name[0] != '\0')) {
        new_path = ncm_tags_build_renamed_path(music_dir, directory, new_name,
                                               is_from_database,
                                               &new_path_len);
        if (new_path == NULL) {
            free2(old_path, old_path_len + 1);
            return -EINVAL;
        }
        if (rename(old_path, new_path) != 0) {
            status = -errno;
            free2(new_path, new_path_len + 1);
            free2(old_path, old_path_len + 1);
            return status;
        }
        free2(new_path, new_path_len + 1);
    }

    free2(old_path, old_path_len + 1);
    return 0;
}

#endif /* NCM_TAGS_C */
