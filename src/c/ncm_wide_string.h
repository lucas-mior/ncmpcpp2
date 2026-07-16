#if !defined(NCM_WIDE_STRING_H)
#define NCM_WIDE_STRING_H

#include <wchar.h>

#include "c/ncm_defs.h"

typedef struct NcmWideString {
    wchar_t *data;
    int32 len;
    int32 cap;
} NcmWideString;

void ncm_wide_string_init(NcmWideString *string);
void ncm_wide_string_destroy(NcmWideString *string);
void ncm_wide_string_clear(NcmWideString *string);
void ncm_wide_string_reserve(NcmWideString *string, int32 extra);
void ncm_wide_string_append_char(NcmWideString *string, wchar_t ch);
bool ncm_wide_string_from_utf8(NcmWideString *out,
                               char *string, int32 string_len);

#endif /* NCM_WIDE_STRING_H */
