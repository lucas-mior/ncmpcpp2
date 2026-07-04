#if !defined(NCM_UTF8_H)
#define NCM_UTF8_H

#include "cbase/primitives.h"

#if defined(__cplusplus)
extern "C" {
#endif

int32 ncm_utf8_decode(char *string, int32 string_len, uint32 *rune);
int32 ncm_utf8_encode(uint32 rune, char *buffer, int32 buffer_capacity);
int32 ncm_utf8_char_width(uint32 rune);
int32 ncm_utf8_characters(char *string, int32 string_len);
int32 ncm_utf8_byte_position(char *string, int32 string_len,
                             int32 character);
int32 ncm_utf8_next_position(char *string, int32 string_len, int32 byte);
int32 ncm_utf8_suffix_width_position(char *string, int32 string_len,
                                     int32 max_width);
int32 ncm_utf8_width(char *string, int32 string_len);
int32 ncm_utf8_cut_width(char *string, int32 string_len, int32 max_width);
int32 ncm_utf8_capitalize_first_letters(char *string, int32 string_len,
                                        char *buffer,
                                        int32 buffer_capacity);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_UTF8_H */
