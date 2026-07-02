#if !defined(NCM_STRING_H)
#define NCM_STRING_H

#include <stdbool.h>

#include "cbase/primitives.h"

#if defined(__cplusplus)
extern "C" {
#endif

void ncm_string_lowercase_ascii(char *string, int32 string_len);
bool ncm_string_equal(char *left, int32 left_len,
                      char *right, int32 right_len);
bool ncm_string_starts_with(char *string, int32 string_len,
                            char *prefix, int32 prefix_len);
int32 ncm_string_basename_start(char *path, int32 path_len);
int32 ncm_string_parent_directory_len(char *path, int32 path_len);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_STRING_H */
