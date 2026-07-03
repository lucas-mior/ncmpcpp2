#if !defined(NCM_PATH_H)
#define NCM_PATH_H

#include "c/ncm_defs.h"

#if defined(__cplusplus)
extern "C" {
#endif

int32 ncm_path_basename_start(char *path, int32 path_len);
int32 ncm_path_parent_directory_len(char *path, int32 path_len);
int32 ncm_path_extension_start(char *path, int32 path_len);
bool ncm_path_has_extension(char *path, int32 path_len,
                            char *extension, int32 extension_len);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_PATH_H */
