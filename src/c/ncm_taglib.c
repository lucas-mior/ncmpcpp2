#if !defined(NCM_TAGLIB_C)
#define NCM_TAGLIB_C

#include "cbase.h"

#include "config.h"

#if defined(HAVE_TAGLIB_H)
#include <tag_c.h>
#endif

#include "c/ncm_c.h"

#if defined(HAVE_TAGLIB_H)

typedef struct NcmTaglibPropertyMap {
    char *property;
    char *name;
} NcmTaglibPropertyMap;

static bool ncm_taglib_is_initialized;

static NcmTaglibPropertyMap ncm_taglib_properties[] = {
    {
        .property = "TITLE",
        .name = "Title",
    },
    {
        .property = "ARTIST",
        .name = "Artist",
    },
    {
        .property = "ALBUMARTIST",
        .name = "AlbumArtist",
    },
    {
        .property = "ALBUM",
        .name = "Album",
    },
    {
        .property = "DATE",
        .name = "Date",
    },
    {
        .property = "TRACKNUMBER",
        .name = "Track",
    },
    {
        .property = "GENRE",
        .name = "Genre",
    },
    {
        .property = "COMPOSER",
        .name = "Composer",
    },
    {
        .property = "PERFORMER",
        .name = "Performer",
    },
    {
        .property = "DISCNUMBER",
        .name = "Disc",
    },
    {
        .property = "COMMENT",
        .name = "Comment",
    },
};

static TagLib_File *
ncm_taglib_handle(NcmTaglibFile *file) {
    return (TagLib_File *)file->handle;
}

static bool
ncm_taglib_value_is_empty(char *value) {
    return value[0] == '\0';
}

#endif

int32
ncm_taglib_file_open(NcmTaglibFile *file, char *path) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if (file == NULL) {
        return -EINVAL;
    }
    if (path == NULL) {
        return -EINVAL;
    }

    if (!ncm_taglib_is_initialized) {
        taglib_set_strings_unicode(1);
        taglib_id3v2_set_default_text_encoding(TagLib_ID3v2_UTF8);
        ncm_taglib_is_initialized = true;
    }

    ncm_taglib_file_close(file);

    if ((handle = taglib_file_new(path)) == NULL) {
        return -NCM_ERROR_TAGLIB;
    }
    file->handle = handle;
    return 0;
#else
    (void)file;
    (void)path;
    return -NCM_ERROR_TAGLIB;
#endif
}

void
ncm_taglib_file_close(NcmTaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
#endif

    if (file == NULL) {
        return;
    }

#if defined(HAVE_TAGLIB_H)
    if ((handle = ncm_taglib_handle(file))) {
        taglib_file_free(handle);
    }
#endif
    file->handle = NULL;
    return;
}

int32
ncm_taglib_file_save(NcmTaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if (file == NULL) {
        return -EINVAL;
    }
    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return -EINVAL;
    }
    if (taglib_file_save(handle) == 0) {
        return -NCM_ERROR_TAGLIB;
    }

    return 0;
#else
    (void)file;
    return -NCM_ERROR_TAGLIB;
#endif
}

int32
ncm_taglib_file_audio_properties(NcmTaglibFile *file,
                                 NcmTaglibAudioProperties *properties) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
    TagLib_AudioProperties *audio;

    if (file == NULL) {
        return -EINVAL;
    }
    if (properties == NULL) {
        return -EINVAL;
    }

    properties->length = 0;
    properties->bitrate = 0;
    properties->sample_rate = 0;
    properties->channels = 0;

    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return -EINVAL;
    }

    audio = (TagLib_AudioProperties *)taglib_file_audioproperties(handle);
    if (audio == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }

    properties->length = (int32)taglib_audioproperties_length(audio);
    properties->bitrate = (int32)taglib_audioproperties_bitrate(audio);
    properties->sample_rate = (int32)taglib_audioproperties_samplerate(audio);
    properties->channels = (int32)taglib_audioproperties_channels(audio);
    return 0;
#else
    (void)file;
    (void)properties;
    return -NCM_ERROR_TAGLIB;
#endif
}

int32
ncm_taglib_read_property(NcmTaglibFile *file, char *property,
                         NcmTaglibValueCallback *callback, void *user) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
    char **values;
    int32 count;

    if (file == NULL) {
        return -EINVAL;
    }
    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return -EINVAL;
    }
    if (property == NULL) {
        return -EINVAL;
    }
    if (callback == NULL) {
        return -EINVAL;
    }

    if ((values = taglib_property_get(handle, property)) == NULL) {
        return 0;
    }

    count = 0;
    for (int32 i = 0; values[i] != NULL; i += 1) {
        if (!ncm_taglib_value_is_empty(values[i])) {
            callback(values[i], user);
            count += 1;
        }
    }

    taglib_property_free(values);
    return count;
#else
    (void)file;
    (void)property;
    (void)callback;
    (void)user;
    return -NCM_ERROR_TAGLIB;
#endif
}

int32
ncm_taglib_read_mapped_properties(NcmTaglibFile *file,
                                  NcmTaglibPairCallback *callback,
                                  void *user) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
    int32 count;

    if (file == NULL) {
        return -EINVAL;
    }
    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return -EINVAL;
    }
    if (callback == NULL) {
        return -EINVAL;
    }

    count = 0;
    for (int32 i = 0; i < LENGTH(ncm_taglib_properties); i += 1) {
        char **values;

        if ((values = taglib_property_get(handle,
                                          ncm_taglib_properties[i].property))
            == NULL) {
            continue;
        }

        for (int32 j = 0; values[j] != NULL; j += 1) {
            if (!ncm_taglib_value_is_empty(values[j])) {
                callback(ncm_taglib_properties[i].name, values[j], user);
                count += 1;
            }
        }

        taglib_property_free(values);
    }

    return count;
#else
    (void)file;
    (void)callback;
    (void)user;
    return -NCM_ERROR_TAGLIB;
#endif
}

int32
ncm_taglib_clear_property(NcmTaglibFile *file, char *property) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if (file == NULL) {
        return -EINVAL;
    }
    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return -EINVAL;
    }
    if (property == NULL) {
        return -EINVAL;
    }

    taglib_property_set(handle, property, NULL);
    return 0;
#else
    (void)file;
    (void)property;
    return -NCM_ERROR_TAGLIB;
#endif
}

int32
ncm_taglib_append_property(NcmTaglibFile *file, char *property, char *value) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if (file == NULL) {
        return -EINVAL;
    }
    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return -EINVAL;
    }
    if (property == NULL) {
        return -EINVAL;
    }
    if (value == NULL) {
        return -EINVAL;
    }

    taglib_property_set_append(handle, property, value);
    return 0;
#else
    (void)file;
    (void)property;
    (void)value;
    return -NCM_ERROR_TAGLIB;
#endif
}

bool
ncm_taglib_file_can_set_extended_tags(NcmTaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    if (file == NULL) {
        return false;
    }
    return ncm_taglib_handle(file) != NULL;
#else
    (void)file;
    return false;
#endif
}

void
ncm_taglib_clear_strings(void) {
#if defined(HAVE_TAGLIB_H)
    taglib_tag_free_strings();
#endif
    return;
}

#endif /* NCM_TAGLIB_C */
