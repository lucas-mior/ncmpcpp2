#if !defined(HELPERS_H)
#define HELPERS_H

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"
#include "curses/nc_curses.h"

enum NcmReapplyFilter {
    NCM_REAPPLY_FILTER_YES,
    NCM_REAPPLY_FILTER_NO,
};

char *ncm_helpers_with_errors(bool);
int32 ncm_helpers_show_song_time(int32 length, char *, int32 buffer_cap);

void ncm_menu_reverse_selection(NcMenu *, enum NcMenuItemSource);
int32 ncm_menu_find_selected_range(NcMenu *, enum NcMenuItemSource,
                                   int32 *first, int32 *last);
int32 ncm_menu_find_full_selected_range(NcMenu *, enum NcMenuItemSource,
                                        int32 *first, int32 *last);

#endif /* HELPERS_H */
