#if !defined(NCMPCPP_SCREEN_ACTIONS_H)
#define NCMPCPP_SCREEN_ACTIONS_H

#include <stdbool.h>

#include "c/ncm_enums.h"
#include "c/ncm_error.h"
#include "c/ncm_string.h"

#if defined(__cplusplus)
extern "C" {
#endif

bool current_screen_allows_filter(void);
NcmStringView current_screen_current_filter(void);
bool current_screen_apply_filter(char *pattern, int32 pattern_len,
                                 NcmError *error);
bool current_screen_allows_search(void);
bool current_screen_search(enum SearchDirection direction,
                           char *pattern, int32 pattern_len,
                           bool wrap, bool skip_current,
                           NcmError *error);
void current_screen_clear_search_constraint(void);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_SCREEN_ACTIONS_H */
