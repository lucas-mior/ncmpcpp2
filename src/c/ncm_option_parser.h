#if !defined(NCM_OPTION_PARSER_H)
#define NCM_OPTION_PARSER_H

#include <stdbool.h>

#include "cbase/primitives.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcmOptionLine {
    char *option;
    char *value;
    int32 option_len;
    int32 value_len;
} NcmOptionLine;

bool ncm_option_parser_parse_line(char *line, int32 line_len,
                                  NcmOptionLine *result);
bool ncm_option_parser_yes_no(char *value, int32 value_len, bool *result);

#if defined(__cplusplus)
}
#endif

#endif /* NCM_OPTION_PARSER_H */
