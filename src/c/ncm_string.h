#if !defined(NCM_STRING_H)
#define NCM_STRING_H

#include "cbase/primitives.h"

#if defined(__cplusplus)
extern "C" {
#endif

void ncm_string_lowercase_ascii(char *string, int32 string_len);
int32 ncm_string_basename_start(char *path, int32 path_len);
int32 ncm_string_parent_directory_len(char *path, int32 path_len);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_STRING_H */
