#if !defined(TAGLIB_C)
#define TAGLIB_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "configura.h"

#if defined(HAVE_TAGLIB_H)
#include <tag_c.h>
#endif

#include "c/ncm_c.h"

#if defined(HAVE_TAGLIB_H)

static bool ncm_taglib_is_initialized;

static TagLib_File *
ncm_taglib_handle(TaglibFile *file) {
    return (TagLib_File *)file->handle;
}

static bool
ncm_taglib_value_is_empty(char *value) {
    return value[0] == '\0';
}

#endif

int32
ncm_taglib_file_open(TaglibFile *file, char *path) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    ASSERT(file != NULL);
    ASSERT(path != NULL);

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
ncm_taglib_file_close(TaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
#endif

    ASSERT(file != NULL);

#if defined(HAVE_TAGLIB_H)
    if ((handle = ncm_taglib_handle(file))) {
        taglib_file_free(handle);
    }
#endif
    file->handle = NULL;
    return;
}

int32
ncm_taglib_file_save(TaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    ASSERT(file != NULL);
    handle = ncm_taglib_handle(file);
    ASSERT(handle != NULL);
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
ncm_taglib_file_audio_properties(TaglibFile *file,
                                 TaglibAudioProperties *properties) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
    TagLib_AudioProperties *audio;

    ASSERT(file != NULL);
    ASSERT(properties != NULL);

    properties->length = 0;
    properties->bitrate = 0;
    properties->sample_rate = 0;
    properties->channels = 0;

    handle = ncm_taglib_handle(file);
    ASSERT(handle != NULL);

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
ncm_taglib_read_mapped_properties(TaglibFile *file,
                                  TaglibPairCallback *callback, void *user) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;
    int32 count;

    ASSERT(file != NULL);
    handle = ncm_taglib_handle(file);
    ASSERT(handle != NULL);
    ASSERT(callback != NULL);

    count = 0;
    for (uint32 i = 0; i < TAG_COUNT; i += 1) {
        char property[TAGLIB_PROPERTY_CAP];
        char name[TAGLIB_NAME_CAP];
        char **values;
        int32 property_len;
        int32 name_len;

        property_len = ncm_tag_type_taglib_property_len(
            (enum TagType)i, property, LENGTH(property));
        name_len = ncm_tag_type_taglib_name_len((enum TagType)i,
                                                name, LENGTH(name));
        ASSERT(property_len > 0);
        ASSERT(name_len > 0);

        if ((values = taglib_property_get(handle, property)) == NULL) {
            continue;
        }

        for (int32 j = 0; values[j] != NULL; j += 1) {
            if (!ncm_taglib_value_is_empty(values[j])) {
                callback(name, values[j], user);
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
ncm_taglib_clear_property(TaglibFile *file, char *property) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    ASSERT(file != NULL);
    handle = ncm_taglib_handle(file);
    ASSERT(handle != NULL);
    ASSERT(property != NULL);

    taglib_property_set(handle, property, NULL);
    return 0;
#else
    (void)file;
    (void)property;
    return -NCM_ERROR_TAGLIB;
#endif
}

int32
ncm_taglib_append_property(TaglibFile *file, char *property, char *value) {
#if defined(HAVE_TAGLIB_H)
    TagLib_File *handle;

    ASSERT(file != NULL);
    handle = ncm_taglib_handle(file);
    ASSERT(handle != NULL);
    ASSERT(property != NULL);
    ASSERT(value != NULL);

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
ncm_taglib_file_can_set_extended_tags(TaglibFile *file) {
#if defined(HAVE_TAGLIB_H)
    ASSERT(file != NULL);
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

#endif /* TAGLIB_C */
