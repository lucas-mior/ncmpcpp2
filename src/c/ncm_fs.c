#include "c/ncm_fs.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "c/ncm_string.h"
#include "cbase/base_macros.h"

static bool ncm_fs_path_copy(char *path, int32 path_len, char **copy,
                             NcmError *error);
static void ncm_fs_set_errno_error(NcmError *error, int32 code,
                                   char *operation, char *path,
                                   int32 path_len);
static enum NcmFsEntryType ncm_fs_mode_type(mode_t mode);
static enum NcmFsEntryType ncm_fs_dirent_type(int32 type);
static bool ncm_fs_append_app_name(NcmBuffer *buffer, char *app_name,
                                   int32 app_name_len);
static bool ncm_fs_xdg_dir(NcmBuffer *buffer, char *xdg_name,
                           char *fallback_suffix,
                           char *app_name, int32 app_name_len,
                           NcmError *error);

static bool
ncm_fs_path_copy(char *path, int32 path_len, char **copy, NcmError *error) {
    if (copy == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing path copy output"));
        return false;
    }
    *copy = NULL;

    if (path == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing path"));
        return false;
    }
    if (path_len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("negative path length"));
        return false;
    }

    *copy = ncm_malloc(path_len + 1);
    ncm_memcpy(*copy, path, path_len);
    (*copy)[path_len] = '\0';
    return true;
}

static void
ncm_fs_set_errno_error(NcmError *error, int32 code, char *operation,
                       char *path, int32 path_len) {
    char message[256];
    int32 message_len;

    if (path == NULL) {
        message_len = snprintf(message, SIZEOF(message), "%s: %s",
                               operation, strerror(code));
    } else {
        message_len = snprintf(message, SIZEOF(message), "%s '%.*s': %s",
                               operation, path_len, path, strerror(code));
    }
    ncm_error_set(error, code, message, message_len);
    return;
}

static enum NcmFsEntryType
ncm_fs_mode_type(mode_t mode) {
    if (S_ISREG(mode)) {
        return NCM_FS_ENTRY_FILE;
    }
    if (S_ISDIR(mode)) {
        return NCM_FS_ENTRY_DIRECTORY;
    }
    if (S_ISLNK(mode)) {
        return NCM_FS_ENTRY_SYMLINK;
    }

    return NCM_FS_ENTRY_UNKNOWN;
}

static enum NcmFsEntryType
ncm_fs_dirent_type(int32 type) {
    switch (type) {
#if defined(DT_REG)
    case DT_REG:
        return NCM_FS_ENTRY_FILE;
#endif
#if defined(DT_DIR)
    case DT_DIR:
        return NCM_FS_ENTRY_DIRECTORY;
#endif
#if defined(DT_LNK)
    case DT_LNK:
        return NCM_FS_ENTRY_SYMLINK;
#endif
    default:
        break;
    }

    return NCM_FS_ENTRY_UNKNOWN;
}

static bool
ncm_fs_append_app_name(NcmBuffer *buffer, char *app_name,
                       int32 app_name_len) {
    if ((app_name == NULL) || (app_name_len <= 0)) {
        return true;
    }

    ncm_fs_join(buffer, buffer->data, buffer->len, app_name, app_name_len);
    return true;
}

static bool
ncm_fs_xdg_dir(NcmBuffer *buffer, char *xdg_name, char *fallback_suffix,
               char *app_name, int32 app_name_len, NcmError *error) {
    char *env;
    char *home;
    int32 env_len;
    int32 home_len;
    int32 fallback_len;

    if (buffer == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing output buffer"));
        return false;
    }

    ncm_buffer_clear(buffer);
    if ((env = getenv(xdg_name))) {
        env_len = (int32)strlen(env);
        ncm_buffer_append(buffer, env, env_len);
        ncm_fs_append_app_name(buffer, app_name, app_name_len);
        ncm_error_clear(error);
        return true;
    }

    if ((home = getenv("HOME")) == NULL) {
        ncm_error_set(error, ENOENT, STRLIT_ARGS("HOME is not set"));
        return false;
    }

    home_len = (int32)strlen(home);
    fallback_len = (int32)strlen(fallback_suffix);
    ncm_buffer_append(buffer, home, home_len);
    ncm_fs_join(buffer, buffer->data, buffer->len,
                fallback_suffix, fallback_len);
    ncm_fs_append_app_name(buffer, app_name, app_name_len);
    ncm_error_clear(error);
    return true;
}

void
ncm_fs_entry_init(NcmFsEntry *entry) {
    entry->name = NULL;
    entry->name_len = 0;
    entry->type = NCM_FS_ENTRY_UNKNOWN;
    return;
}

void
ncm_fs_entry_destroy(NcmFsEntry *entry) {
    if (entry->name) {
        ncm_free(entry->name, entry->name_len + 1);
    }
    ncm_fs_entry_init(entry);
    return;
}

bool
ncm_fs_stat(char *path, int32 path_len, NcmFsStat *stat,
            NcmError *error) {
    struct stat statbuf;
    char *path_copy;

    if (stat == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing stat output"));
        return false;
    }

    stat->size = 0;
    stat->mtime = 0;
    stat->type = NCM_FS_ENTRY_UNKNOWN;
    stat->exists = false;

    if (!ncm_fs_path_copy(path, path_len, &path_copy, error)) {
        return false;
    }

    if (lstat(path_copy, &statbuf) != 0) {
        if (errno == ENOENT) {
            ncm_free(path_copy, path_len + 1);
            ncm_error_clear(error);
            return true;
        }
        ncm_fs_set_errno_error(error, errno, (char *)"stat", path, path_len);
        ncm_free(path_copy, path_len + 1);
        return false;
    }

    stat->size = (int64)statbuf.st_size;
    stat->mtime = (int64)statbuf.st_mtime;
    stat->type = ncm_fs_mode_type(statbuf.st_mode);
    stat->exists = true;
    ncm_free(path_copy, path_len + 1);
    ncm_error_clear(error);
    return true;
}

bool
ncm_fs_exists(char *path, int32 path_len) {
    NcmFsStat stat;

    if (!ncm_fs_stat(path, path_len, &stat, NULL)) {
        return false;
    }

    return stat.exists;
}

bool
ncm_fs_is_regular_file(char *path, int32 path_len) {
    NcmFsStat stat;

    if (!ncm_fs_stat(path, path_len, &stat, NULL)) {
        return false;
    }

    return stat.exists && (stat.type == NCM_FS_ENTRY_FILE);
}

bool
ncm_fs_is_directory(char *path, int32 path_len) {
    NcmFsStat stat;

    if (!ncm_fs_stat(path, path_len, &stat, NULL)) {
        return false;
    }

    return stat.exists && (stat.type == NCM_FS_ENTRY_DIRECTORY);
}

bool
ncm_fs_unlink(char *path, int32 path_len, NcmError *error) {
    char *path_copy;

    if (!ncm_fs_path_copy(path, path_len, &path_copy, error)) {
        return false;
    }

    if (unlink(path_copy) != 0) {
        if (errno == ENOENT) {
            ncm_free(path_copy, path_len + 1);
            ncm_error_clear(error);
            return true;
        }
        ncm_fs_set_errno_error(error, errno, (char *)"unlink", path,
                               path_len);
        ncm_free(path_copy, path_len + 1);
        return false;
    }

    ncm_free(path_copy, path_len + 1);
    ncm_error_clear(error);
    return true;
}

bool
ncm_fs_mkdir_all(char *path, int32 path_len, NcmError *error) {
    char *copy;

    if (!ncm_fs_path_copy(path, path_len, &copy, error)) {
        return false;
    }

    for (int32 i = 1; i <= path_len; i += 1) {
        if ((copy[i] != '/') && (copy[i] != '\0')) {
            continue;
        }
        if (i <= 0) {
            continue;
        }
        if (copy[i - 1] == '/') {
            continue;
        }

        copy[i] = '\0';
        if ((mkdir(copy, 0700) != 0) && (errno != EEXIST)) {
            ncm_fs_set_errno_error(error, errno, (char *)"mkdir", copy, i);
            ncm_free(copy, path_len + 1);
            return false;
        }
        if (i < path_len) {
            copy[i] = path[i];
        }
    }

    ncm_free(copy, path_len + 1);
    ncm_error_clear(error);
    return true;
}

bool
ncm_fs_directory_open(NcmFsDirectory *directory, char *path,
                      int32 path_len, NcmError *error) {
    DIR *dir;
    char *path_copy;

    if (directory == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing directory"));
        return false;
    }
    directory->dir = NULL;
    directory->path = NULL;
    directory->path_len = 0;

    if (!ncm_fs_path_copy(path, path_len, &path_copy, error)) {
        return false;
    }

    if ((dir = opendir(path_copy)) == NULL) {
        ncm_fs_set_errno_error(error, errno, (char *)"opendir", path,
                               path_len);
        ncm_free(path_copy, path_len + 1);
        return false;
    }

    directory->dir = dir;
    directory->path = path_copy;
    directory->path_len = path_len;
    ncm_error_clear(error);
    return true;
}

bool
ncm_fs_directory_read(NcmFsDirectory *directory, NcmFsEntry *entry,
                      NcmError *error) {
    struct dirent *dirent;
    DIR *dir;
    int32 name_len;

    if (entry == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing directory entry"));
        return false;
    }
    ncm_fs_entry_destroy(entry);

    if ((directory == NULL) || (directory->dir == NULL)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("directory is not open"));
        return false;
    }

    dir = (DIR *)directory->dir;
    errno = 0;
    while ((dirent = readdir(dir))) {
        if (strcmp(dirent->d_name, ".") == 0) {
            continue;
        }
        if (strcmp(dirent->d_name, "..") == 0) {
            continue;
        }

        name_len = (int32)strlen(dirent->d_name);
        entry->name = ncm_malloc(name_len + 1);
        entry->name_len = name_len;
        entry->type = ncm_fs_dirent_type(dirent->d_type);
        ncm_memcpy(entry->name, dirent->d_name, name_len + 1);
        ncm_error_clear(error);
        return true;
    }

    if (errno) {
        ncm_fs_set_errno_error(error, errno, (char *)"readdir",
                               directory->path, directory->path_len);
    } else {
        ncm_error_clear(error);
    }

    return false;
}

void
ncm_fs_directory_close(NcmFsDirectory *directory) {
    if (directory == NULL) {
        return;
    }
    if (directory->dir) {
        closedir((DIR *)directory->dir);
    }
    if (directory->path) {
        ncm_free(directory->path, directory->path_len + 1);
    }

    directory->dir = NULL;
    directory->path = NULL;
    directory->path_len = 0;
    return;
}

bool
ncm_fs_join(NcmBuffer *buffer, char *left, int32 left_len,
            char *right, int32 right_len) {
    NcmBuffer result;

    if (buffer == NULL) {
        return false;
    }
    if (right == NULL) {
        return false;
    }
    if ((left_len < 0) || (right_len < 0)) {
        return false;
    }

    ncm_buffer_init(&result);
    if ((left != NULL) && (left_len > 0)) {
        ncm_buffer_append(&result, left, left_len);
        if ((result.len > 0) && (result.data[result.len - 1] != '/')) {
            ncm_buffer_append_byte(&result, '/');
        }
    }
    while ((right_len > 0) && (right[0] == '/')) {
        right += 1;
        right_len -= 1;
    }
    ncm_buffer_append(&result, right, right_len);

    ncm_buffer_clear(buffer);
    ncm_buffer_append(buffer, result.data, result.len);
    ncm_buffer_destroy(&result);
    return true;
}

bool
ncm_fs_user_config_dir(NcmBuffer *buffer, char *app_name,
                       int32 app_name_len, NcmError *error) {
    return ncm_fs_xdg_dir(buffer, (char *)"XDG_CONFIG_HOME",
                          (char *)".config", app_name, app_name_len,
                          error);
}

bool
ncm_fs_user_cache_dir(NcmBuffer *buffer, char *app_name,
                      int32 app_name_len, NcmError *error) {
    return ncm_fs_xdg_dir(buffer, (char *)"XDG_CACHE_HOME",
                          (char *)".cache", app_name, app_name_len,
                          error);
}
