#if !defined(NCM_STRING_H)
#define NCM_STRING_H

#include "c/ncm_defs.h"

#if defined(__cplusplus)
extern "C" {
#endif

void ncm_string_lowercase_ascii(char *string, int32 string_len);
bool ncm_string_equal(char *left, int32 left_len,
                      char *right, int32 right_len);
bool ncm_string_starts_with(char *string, int32 string_len,
                            char *prefix, int32 prefix_len);
bool ncm_string_ends_with(char *string, int32 string_len,
                          char *suffix, int32 suffix_len);
int32 ncm_string_find_char(char *string, int32 string_len, char needle);
bool ncm_string_contains_char(char *string, int32 string_len, char needle);
void ncm_string_remove_chars(char *string, int32 *string_len,
                             char *chars, int32 chars_len);
void ncm_string_append_shell_escaped_single_quotes(NcmBuffer *buffer,
                                                   char *string,
                                                   int32 string_len);
int32 ncm_string_basename_start(char *path, int32 path_len);
int32 ncm_string_parent_directory_len(char *path, int32 path_len);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_STRING_H */
