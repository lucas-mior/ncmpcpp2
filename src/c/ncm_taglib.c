#if !defined(NCM_TAGLIB_C)
#define NCM_TAGLIB_C

#include "c/ncm_taglib.h"

#include "config.h"

#if defined(HAVE_TAGLIB_H)
#include <tag_c.h>
#endif

#include <stdbool.h>
#include <stddef.h>

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
    if (file == NULL) {
        return NULL;
    }

    return (TagLib_File *)file->handle;
}

static bool
ncm_taglib_value_is_empty(char *value) {
    if (value == NULL) {
        return true;
    }

    return value[0] == '\0';
}

#endif

void
ncm_taglib_init(void) {
#if defined(HAVE_TAGLIB_H)
    if (ncm_taglib_is_initialized) {
        return;
    }

    taglib_set_strings_unicode(1);
    taglib_id3v2_set_default_text_encoding(TagLib_ID3v2_UTF8);
    ncm_taglib_is_initialized = true;
#endif
    return;
}

void
ncm_taglib_file_init(NcmTaglibFile *file) {
    if (file == NULL) {
        return;
    }

    file->handle = NULL;
    return;
}

bool
ncm_taglib_file_open(NcmTaglibFile *file, char *path) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if (file == NULL) {
        return false;
    }
    if (path == NULL) {
        return false;
    }

    ncm_taglib_init();
    ncm_taglib_file_close(file);

    if ((handle = taglib_file_new(path)) == NULL) {
        return false;
    }
    file->handle = handle;
    return true;
#else
    (void)file;
    (void)path;
    return false;
#endif
}

void
ncm_taglib_file_close(NcmTaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if ((handle = ncm_taglib_handle(file))) {
        taglib_file_free(handle);
    }
#endif
    if (file) {
        file->handle = NULL;
    }
    return;
}

bool
ncm_taglib_file_save(NcmTaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return false;
    }

    return taglib_file_save(handle) != 0;
#else
    (void)file;
    return false;
#endif
}

bool
ncm_taglib_file_audio_properties(NcmTaglibFile *file,
                                 NcmTaglibAudioProperties *properties) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
    TagLib_AudioProperties *audio;

    if (properties == NULL) {
        return false;
    }

    properties->length = 0;
    properties->bitrate = 0;
    properties->sample_rate = 0;
    properties->channels = 0;

    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return false;
    }

    audio = (TagLib_AudioProperties *)taglib_file_audioproperties(handle);
    if (audio == NULL) {
        return false;
    }

    properties->length = (int32)taglib_audioproperties_length(audio);
    properties->bitrate = (int32)taglib_audioproperties_bitrate(audio);
    properties->sample_rate = (int32)taglib_audioproperties_samplerate(audio);
    properties->channels = (int32)taglib_audioproperties_channels(audio);
    return true;
#else
    (void)file;
    (void)properties;
    return false;
#endif
}

bool
ncm_taglib_read_property(NcmTaglibFile *file, char *property,
                         NcmTaglibValueCallback *callback, void *user) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
    char **values;
    bool found;

    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return false;
    }
    if (property == NULL) {
        return false;
    }
    if (callback == NULL) {
        return false;
    }

    if ((values = taglib_property_get(handle, property)) == NULL) {
        return false;
    }

    found = false;
    for (int32 i = 0; values[i]; i += 1) {
        if (!ncm_taglib_value_is_empty(values[i])) {
            callback(values[i], user);
            found = true;
        }
    }

    taglib_property_free(values);
    return found;
#else
    (void)file;
    (void)property;
    (void)callback;
    (void)user;
    return false;
#endif
}

bool
ncm_taglib_read_mapped_properties(NcmTaglibFile *file,
                                  NcmTaglibPairCallback *callback,
                                  void *user) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
    bool found;

    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return false;
    }
    if (callback == NULL) {
        return false;
    }

    found = false;
    for (int32 i = 0; i < NCM_ARRAY_LEN(ncm_taglib_properties); i += 1) {
        char **values;

        values = taglib_property_get(handle, ncm_taglib_properties[i].property);
        if (values == NULL) {
            continue;
        }

        for (int32 j = 0; values[j]; j += 1) {
            if (!ncm_taglib_value_is_empty(values[j])) {
                callback(ncm_taglib_properties[i].name, values[j], user);
                found = true;
            }
        }

        taglib_property_free(values);
    }

    return found;
#else
    (void)file;
    (void)callback;
    (void)user;
    return false;
#endif
}

void
ncm_taglib_clear_property(NcmTaglibFile *file, char *property) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return;
    }
    if (property == NULL) {
        return;
    }

    taglib_property_set(handle, property, NULL);
#else
    (void)file;
    (void)property;
#endif
    return;
}

void
ncm_taglib_append_property(NcmTaglibFile *file, char *property, char *value) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    if ((handle = ncm_taglib_handle(file)) == NULL) {
        return;
    }
    if (property == NULL) {
        return;
    }
    if (value == NULL) {
        return;
    }

    taglib_property_set_append(handle, property, value);
#else
    (void)file;
    (void)property;
    (void)value;
#endif
    return;
}

bool
ncm_taglib_extended_set_supported(NcmTaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    return ncm_taglib_handle(file);
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
