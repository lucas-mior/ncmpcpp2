#if !defined(NCM_STRING_H)
#define NCM_STRING_H

#include "c/ncm_defs.h"

void ncm_string_view_init(NcmStringView *view);
NcmStringView ncm_string_view_make(char *data, int32 len);
void ncm_string_view_set(NcmStringView *view, char *data, int32 len);
void ncm_string_view_clear(NcmStringView *view);

void ncm_string_lowercase_ascii(char *string, int32 string_len);
bool ncm_string_starts_with(char *string, int32 string_len,
                            char *prefix, int32 prefix_len);
bool ncm_string_ends_with(char *string, int32 string_len,
                          char *suffix, int32 suffix_len);
int32 ncm_string_find_char(char *string, int32 string_len, char needle);
bool ncm_string_contains_char(char *string, int32 string_len, char needle);
NcmBuffer ncm_string_shared_directory(char *left, int32 left_len,
                                      char *right, int32 right_len);
NcmBuffer ncm_string_get_enclosed(char *string, int32 string_len,
                                  char open, char close,
                                  int32 start, int32 *pos);
void ncm_string_remove_chars(char *string, int32 *string_len,
                             char *chars, int32 chars_len);
void ncm_string_remove_invalid_filename_chars(char *filename,
                                              int32 *filename_len,
                                              bool win32_compatible);
void ncm_string_append_shell_escaped_single_quotes(NcmBuffer *buffer,
                                                   char *string,
                                                   int32 string_len);
int32 ncm_string_basename_start(char *path, int32 path_len);
int32 ncm_string_parent_directory_len(char *path, int32 path_len);

#endif /* NCM_STRING_H */
