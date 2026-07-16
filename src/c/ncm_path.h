#if !defined(NCM_PATH_H)
#define NCM_PATH_H

#include "c/ncm_defs.h"
#include "c/ncm_error.h"

bool ncm_path_expand_home(NcmBuffer *path, NcmError *error);
int32 ncm_path_basename_start(char *path, int32 path_len);
int32 ncm_path_parent_directory_len(char *path, int32 path_len);
int32 ncm_path_extension_start(char *path, int32 path_len);
bool ncm_path_has_extension(char *path, int32 path_len,
                            char *extension, int32 extension_len);

#endif /* NCM_PATH_H */
