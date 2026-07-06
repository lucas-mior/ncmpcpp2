#if !defined(NCM_FS_H)
#define NCM_FS_H

#include <stdbool.h>

#include "c/ncm_base.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum NcmFsEntryType {
    NCM_FS_ENTRY_UNKNOWN,
    NCM_FS_ENTRY_FILE,
    NCM_FS_ENTRY_DIRECTORY,
    NCM_FS_ENTRY_SYMLINK,
};

typedef struct NcmFsStat {
    int64 size;
    int64 mtime;
    enum NcmFsEntryType type;
    bool exists;
} NcmFsStat;

typedef struct NcmFsEntry {
    char *name;
    int32 name_len;
    enum NcmFsEntryType type;
} NcmFsEntry;

typedef struct NcmFsDirectory {
    void *dir;
    char *path;
    int32 path_len;
} NcmFsDirectory;

void ncm_fs_entry_init(NcmFsEntry *entry);
void ncm_fs_entry_destroy(NcmFsEntry *entry);
bool ncm_fs_stat(char *path, int32 path_len, NcmFsStat *stat,
                 NcmError *error);
bool ncm_fs_exists(char *path, int32 path_len);
bool ncm_fs_is_regular_file(char *path, int32 path_len);
bool ncm_fs_is_directory(char *path, int32 path_len);
bool ncm_fs_unlink(char *path, int32 path_len, NcmError *error);
bool ncm_fs_mkdir_all(char *path, int32 path_len, NcmError *error);
bool ncm_fs_directory_open(NcmFsDirectory *directory, char *path,
                           int32 path_len, NcmError *error);
bool ncm_fs_directory_read(NcmFsDirectory *directory, NcmFsEntry *entry,
                           NcmError *error);
void ncm_fs_directory_close(NcmFsDirectory *directory);
bool ncm_fs_join(NcmBuffer *buffer, char *left, int32 left_len,
                 char *right, int32 right_len);
bool ncm_fs_user_config_dir(NcmBuffer *buffer, char *app_name,
                            int32 app_name_len, NcmError *error);
bool ncm_fs_user_cache_dir(NcmBuffer *buffer, char *app_name,
                           int32 app_name_len, NcmError *error);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_FS_H */
