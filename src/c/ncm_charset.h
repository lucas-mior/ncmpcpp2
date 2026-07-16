#if !defined(NCM_CHARSET_H)
#define NCM_CHARSET_H

#include "c/ncm_base.h"

NcmBuffer ncm_charset_to_utf8_from(char *string, int32 string_len,
                                   char *charset, int32 charset_len);
NcmBuffer ncm_charset_from_utf8_to(char *string, int32 string_len,
                                   char *charset, int32 charset_len);
NcmBuffer ncm_charset_utf8_to_locale(char *string, int32 string_len);
NcmBuffer ncm_charset_locale_to_utf8(char *string, int32 string_len);

#endif /* NCM_CHARSET_H */
