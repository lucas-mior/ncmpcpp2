#if !defined(SCREEN_ACTIONS_H)
#define SCREEN_ACTIONS_H

#include "cbase.h"

#include "c/ncm_c.h"

bool current_screen_can_filter(void);
StringView current_screen_current_filter(void);
int32 current_screen_apply_filter(char *, int32, NcmError *);
bool current_screen_can_search(void);
bool current_screen_can_find(void);
StringView current_screen_current_search_constraint(void);
int32 current_screen_search(enum SearchDirection, char *, int32, bool wrap,
                            bool skip_current, NcmError *);
void current_screen_clear_search_constraint(void);

#endif /* SCREEN_ACTIONS_H */
