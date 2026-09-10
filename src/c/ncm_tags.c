#if !defined(NCM_TAGS_C)
#define NCM_TAGS_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

int32
ncm_tags_write(char *music_dir, char *uri, bool is_from_database,
               char *directory, char *new_name,
               NcmTagsGetFieldCallback *callback, void *user) {
    NcmTaglibFile file = {0};
    char *old_path;
    char *new_path;
    int32 old_path_len;
    int32 new_path_len;
    int32 status;
    char property[NCM_TAGLIB_PROPERTY_CAP];

    if (callback == NULL) {
        return -EINVAL;
    }
    if (uri == NULL) {
        return -EINVAL;
    }

    {
        int32 music_dir_len = 0;
        int32 uri_len = optional_strlen32(uri);

        if (is_from_database) {
            music_dir_len = optional_strlen32(music_dir);
        }
        old_path_len = music_dir_len + uri_len;
        old_path = malloc2(old_path_len + 1);
        if (music_dir_len > 0) {
            memcpy64(old_path, music_dir, music_dir_len);
        }
        memcpy64(old_path + music_dir_len, uri, uri_len + 1);
    }

    if ((status = ncm_taglib_file_open(&file, old_path)) < 0) {
        free2(old_path, old_path_len + 1);
        return status;
    }

    if ((status = ncm_taglib_clear_property(&file, "ALBUM ARTIST")) < 0) {
        ncm_taglib_file_close(&file);
        free2(old_path, old_path_len + 1);
        return status;
    }
    if ((status = ncm_taglib_clear_property(&file, "TRACK")) < 0) {
        ncm_taglib_file_close(&file);
        free2(old_path, old_path_len + 1);
        return status;
    }
    if ((status = ncm_taglib_clear_property(&file, "DISC")) < 0) {
        ncm_taglib_file_close(&file);
        free2(old_path, old_path_len + 1);
        return status;
    }
    if ((status = ncm_taglib_clear_property(&file, "DESCRIPTION")) < 0) {
        ncm_taglib_file_close(&file);
        free2(old_path, old_path_len + 1);
        return status;
    }

    for (uint32 i = 0; i < NCM_TAGS_FIELD_COUNT; i += 1) {
        enum TagsField field = (enum TagsField)i;
        int32 property_len;

        property_len = ncm_tags_field_taglib_property_len(
            field, property, LENGTH(property));
        ASSERT(property_len > 0);

        if ((status = ncm_taglib_clear_property(&file, property)) < 0) {
            ncm_taglib_file_close(&file);
            free2(old_path, old_path_len + 1);
            return status;
        }
        for (int32 value_i = 0; ; value_i += 1) {
            StringView value = {0};

            if (!callback(field, value_i, &value, user)) {
                break;
            }
            if (value.data == NULL) {
                break;
            }
            if (value.len <= 0) {
                break;
            }

            if ((status = ncm_taglib_append_property(&file, property,
                                                     value.data)) < 0) {
                ncm_taglib_file_close(&file);
                free2(old_path, old_path_len + 1);
                return status;
            }
        }
    }

    status = ncm_taglib_file_save(&file);
    ncm_taglib_file_close(&file);
    if (status < 0) {
        free2(old_path, old_path_len + 1);
        return status;
    }

    if ((new_name != NULL) && (new_name[0] != '\0')) {
        int32 music_dir_len = 0;
        int32 directory_len = optional_strlen32(directory);
        int32 new_name_len = optional_strlen32(new_name);
        int32 offset = 0;

        if (is_from_database) {
            music_dir_len = optional_strlen32(music_dir);
        }
        new_path_len = music_dir_len + directory_len + 1 + new_name_len;
        new_path = malloc2(new_path_len + 1);
        if (music_dir_len > 0) {
            memcpy64(new_path + offset, music_dir, music_dir_len);
            offset += music_dir_len;
        }
        if (directory_len > 0) {
            memcpy64(new_path + offset, directory, directory_len);
            offset += directory_len;
        }
        new_path[offset] = '/';
        offset += 1;
        memcpy64(new_path + offset, new_name, new_name_len + 1);

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
