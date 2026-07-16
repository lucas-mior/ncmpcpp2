#if !defined(NCM_REGEX_H)
#define NCM_REGEX_H

#include <regex.h>

#include "c/ncm_base.h"

#define NCM_REGEX_EXTENDED 0x01u
#define NCM_REGEX_ICASE    0x02u
#define NCM_REGEX_LITERAL  0x04u
#define NCM_REGEX_NOSUB    0x08u

#define NCM_REGEX_BASIC_CASE_INSENSITIVE NCM_REGEX_ICASE
#define NCM_REGEX_EXTENDED_CASE_INSENSITIVE \
    (NCM_REGEX_EXTENDED | NCM_REGEX_ICASE)
#define NCM_REGEX_LITERAL_CASE_INSENSITIVE \
    (NCM_REGEX_LITERAL | NCM_REGEX_EXTENDED | NCM_REGEX_ICASE)

typedef bool (*NcmRegexMatchCallback)(int32 start, int32 len, void *user);

typedef struct NcmRegex {
    regex_t regex;
    bool compiled;
    uint32 flags;
} NcmRegex;

void ncm_regex_init(NcmRegex *regex);
void ncm_regex_destroy(NcmRegex *regex);
void ncm_regex_escape_literal(NcmBuffer *buffer, char *pattern,
                              int32 pattern_len);
bool ncm_regex_compile(NcmRegex *regex, char *pattern, int32 pattern_len,
                       uint32 flags, NcmError *error);
bool ncm_regex_search(NcmRegex *regex, char *string, int32 string_len);
bool ncm_regex_for_each_match(NcmRegex *regex, char *string,
                              int32 string_len,
                              NcmRegexMatchCallback callback, void *user);

#endif /* NCM_REGEX_H */
