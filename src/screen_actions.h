#if !defined(NCMPCPP_SCREEN_ACTIONS_H)
#define NCMPCPP_SCREEN_ACTIONS_H

#include "cbase.h"

#include "c/ncm_c.h"

bool current_screen_allows_filter(void);
NcmStringView current_screen_current_filter(void);
bool current_screen_apply_filter(char *pattern, int32 pattern_len,
                                 NcmError *ncm_error);
bool current_screen_allows_search(void);
NcmStringView current_screen_current_search_constraint(void);
bool current_screen_search(enum SearchDirection direction, char *pattern,
                           int32 pattern_len, bool wrap, bool skip_current,
                           NcmError *ncm_error);
void current_screen_clear_search_constraint(void);

#endif /* NCMPCPP_SCREEN_ACTIONS_H */
