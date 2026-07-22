#if !defined(CBASE_H)
#define CBASE_H

#include <stdbool.h>

#include "primitives.h"

void cbase_application_mode_link_anchor(void);

void *cbase_malloc(int64 size);
void *cbase_realloc_array(void *old, int64 old_capacity,
                          int64 new_capacity, int64 obj_size);
void cbase_free(void *pointer, int64 size);
void cbase_memset(void *buffer, int value, int64 n);
void cbase_memcpy(void *dest, void *source, int64 n);
void cbase_memmove(void *dest, void *source, int64 n);
int cbase_memcmp(void *left, void *right, int64 n);
int32 strlen32(char *string);

void cbase_string_lowercase_ascii(char *string, int32 string_len);
int32 cbase_string_last_index_of(char *string, int32 string_len, char needle);
bool cbase_strequal(const char *s1, const char *s2);

int32 cbase_utf8_decode_rune(char *string, uint32 *rune, int32 string_len);
int32 cbase_utf8_encode_rune(uint32 rune, char *buffer, int32 buffer_capacity);
int32 cbase_utf8_char_width(uint32 rune);

#endif /* CBASE_H */
