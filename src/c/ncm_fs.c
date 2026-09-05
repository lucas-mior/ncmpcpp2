#if !defined(NCM_FS_C)
#define NCM_FS_C

#include "cbase.h"

#include "c/ncm_c.h"

static void
ncm_fs_path_copy(char *path, int32 path_len, char **copy) {
    *copy = malloc2(path_len + 1);
    memcpy64(*copy, path, path_len);
    (*copy)[path_len] = '\0';
    return;
}

static int32
ncm_fs_set_errno_error(NcmError *ncm_error, int32 code, char *operation,
                       char *path, int32 path_len) {
    char message[256];
    int32 message_len;

    if (path == NULL) {
        message_len = SNPRINTF(message, "%s: %s", operation, strerror(code));
    } else {
        message_len = SNPRINTF(message, "%s '%.*s': %s",
                               operation, path_len, path, strerror(code));
    }
    return ncm_error_set_status(ncm_error, -code, message, message_len);
}

void
ncm_fs_entry_init(NcmFsEntry *entry) {
    entry->name = NULL;
    entry->name_len = 0;
    entry->type = NCM_FS_ENTRY_COUNT;
    return;
}

void
ncm_fs_entry_destroy(NcmFsEntry *entry) {
    free2(entry->name, entry->name_len + 1);
    ncm_fs_entry_init(entry);
    return;
}

int32
ncm_fs_stat(char *path, int32 path_len, NcmFsStat *stat, NcmError *ncm_error) {
    struct stat statbuf;
    char *path_copy;
    int32 code;
    int32 status;

    if (stat == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing stat output"));
    }

    stat->size = 0;
    stat->mtime = 0;
    stat->type = NCM_FS_ENTRY_COUNT;
    stat->exists = false;

    ncm_fs_path_copy(path, path_len, &path_copy);

    if (lstat(path_copy, &statbuf) < 0) {
        code = errno;
        if (code == ENOENT) {
            free2(path_copy, path_len + 1);
            return ncm_error_ok(ncm_error);
        }
        status = ncm_fs_set_errno_error(ncm_error, code,
                                        "stat", path, path_len);
        free2(path_copy, path_len + 1);
        return status;
    }

    stat->size = (int32)statbuf.st_size;
    stat->mtime = (int32)statbuf.st_mtime;
    if (S_ISREG(statbuf.st_mode)) {
        stat->type = NCM_FS_ENTRY_FILE;
    } else if (S_ISDIR(statbuf.st_mode)) {
        stat->type = NCM_FS_ENTRY_DIRECTORY;
    } else if (S_ISLNK(statbuf.st_mode)) {
        stat->type = NCM_FS_ENTRY_SYMLINK;
    }
    stat->exists = true;

    free2(path_copy, path_len + 1);
    return ncm_error_ok(ncm_error);
}

bool
ncm_fs_path_is_existing(char *path, int32 path_len) {
    NcmFsStat stat;

    if (ncm_fs_stat(path, path_len, &stat, NULL) < 0) {
        return false;
    }

    return stat.exists;
}

int32
ncm_fs_unlink(char *path, int32 path_len, NcmError *ncm_error) {
    char *path_copy;
    int32 status;

    ncm_fs_path_copy(path, path_len, &path_copy);

    if (unlink(path_copy) != 0) {
        int32 code = errno;
        if (code == ENOENT) {
            free2(path_copy, path_len + 1);
            return ncm_error_ok(ncm_error);
        }
        status = ncm_fs_set_errno_error(ncm_error, code,
                                        "unlink", path, path_len);
        free2(path_copy, path_len + 1);
        return status;
    }

    free2(path_copy, path_len + 1);
    return ncm_error_ok(ncm_error);
}

int32
ncm_fs_rename(char *old_path, int32 old_path_len,
              char *new_path, int32 new_path_len,
              NcmError *ncm_error) {
    char *old_copy = NULL;
    char *new_copy = NULL;
    int32 code;
    int32 status;

    ncm_fs_path_copy(old_path, old_path_len, &old_copy);
    ncm_fs_path_copy(new_path, new_path_len, &new_copy);

    if (rename(old_copy, new_copy) != 0) {
        code = errno;
        status = ncm_fs_set_errno_error(ncm_error, code,
                                        "rename", old_path, old_path_len);
        free2(new_copy, new_path_len + 1);
        free2(old_copy, old_path_len + 1);
        return status;
    }

    free2(new_copy, new_path_len + 1);
    free2(old_copy, old_path_len + 1);
    return ncm_error_ok(ncm_error);
}

int32
ncm_fs_mkdir_all(char *path, int32 path_len, NcmError *ncm_error) {
    char *copy;
    int32 status;

    ncm_fs_path_copy(path, path_len, &copy);

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
        if (mkdir(copy, 0700) < 0) {
            int32 code = errno;
            if (code != EEXIST) {
                status = ncm_fs_set_errno_error(
                    ncm_error, code, "mkdir", copy, i);
                free2(copy, path_len + 1);
                return status;
            }
        }
        if (i < path_len) {
            copy[i] = path[i];
        }
    }

    free2(copy, path_len + 1);
    return ncm_error_ok(ncm_error);
}

int32
ncm_fs_directory_open(NcmFsDirectory *directory,
                      char *path, int32 path_len,
                      NcmError *ncm_error) {
    DIR *dir;
    char *path_copy;
    int32 code;
    int32 status;

    if (directory == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing directory"));
    }
    directory->dir = NULL;
    directory->path = NULL;
    directory->path_len = 0;

    ncm_fs_path_copy(path, path_len, &path_copy);

    if ((dir = opendir(path_copy)) == NULL) {
        code = errno;
        status = ncm_fs_set_errno_error(ncm_error, code,
                                        "opendir", path, path_len);
        free2(path_copy, path_len + 1);
        return status;
    }

    directory->dir = dir;
    directory->path = path_copy;
    directory->path_len = path_len;
    return ncm_error_ok(ncm_error);
}

int32
ncm_fs_directory_read(NcmFsDirectory *directory, NcmFsEntry *entry,
                      NcmError *ncm_error) {
    struct dirent *dirent;
    DIR *dir;
    int32 name_len;
    int32 code;

    if (entry == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing directory entry"));
    }
    ncm_fs_entry_destroy(entry);

    if ((directory == NULL) || (directory->dir == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("directory is not open"));
    }

    dir = directory->dir;
    errno = 0;
    while ((dirent = readdir(dir)) != NULL) {
        if (strequal(dirent->d_name, ".")) {
            continue;
        }
        if (strequal(dirent->d_name, "..")) {
            continue;
        }

        name_len = strlen32(dirent->d_name);
        entry->name = malloc2(name_len + 1);
        entry->name_len = name_len;
        switch (dirent->d_type) {
#if defined(DT_REG)
        case DT_REG:
            entry->type = NCM_FS_ENTRY_FILE;
            break;
#endif
#if defined(DT_DIR)
        case DT_DIR:
            entry->type = NCM_FS_ENTRY_DIRECTORY;
            break;
#endif
#if defined(DT_LNK)
        case DT_LNK:
            entry->type = NCM_FS_ENTRY_SYMLINK;
            break;
#endif
        default:
            entry->type = NCM_FS_ENTRY_COUNT;
            break;
        }
        memcpy64(entry->name, dirent->d_name, name_len + 1);

        ncm_error_clear(ncm_error);
        return 1;
    }

    if (errno != 0) {
        code = errno;
        return ncm_fs_set_errno_error(ncm_error, code,
                                      "readdir", directory->path,
                                      directory->path_len);
    }

    return ncm_error_ok(ncm_error);
}

void
ncm_fs_directory_close(NcmFsDirectory *directory) {
    if (directory->dir != NULL) {
        closedir((DIR *)directory->dir);
    }
    free2(directory->path, directory->path_len + 1);

    directory->dir = NULL;
    directory->path = NULL;
    directory->path_len = 0;
    return;
}

int32
ncm_fs_join(StrBuilder *buffer,
            char *left, int32 left_len, char *right, int32 right_len) {
    StrBuilder result = {0};

    if (buffer == NULL) {
        return -EINVAL;
    }
    if (right == NULL) {
        return -EINVAL;
    }
    if ((left_len < 0) || (right_len < 0)) {
        return -EINVAL;
    }

    if ((left != NULL) && (left_len > 0)) {
        SB_APPEND(&result, left, left_len);
        sb_append_byte_if_not(&result, '/');
    }
    while ((right_len > 0) && (right[0] == '/')) {
        right += 1;
        right_len -= 1;
    }
    SB_APPEND(&result, right, right_len);

    sb_clear(buffer);
    SB_APPEND(buffer, result.data, result.len);
    sb_free(&result);

    return 0;
}

#endif /* NCM_FS_C */
