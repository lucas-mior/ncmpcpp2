#if !defined(NCMPCPP_HELPERS_H)
#define NCMPCPP_HELPERS_H

#include <stdbool.h>

#include "c/ncm_base.h"
#include "c/ncm_enums.h"
#include "c/ncm_mpd_connection.h"
#include "curses/nc_menu.h"

enum NcmReapplyFilter {
    NCM_REAPPLY_FILTER_YES,
    NCM_REAPPLY_FILTER_NO,
};

char *ncm_helpers_with_errors(bool success);
int32 ncm_helpers_show_song_time(uint32 length, char *buffer,
                                 int32 buffer_cap);



void ncm_menu_reverse_selection(NcMenu *menu, enum NcMenuItemSource source);
bool ncm_menu_find_selected_range(NcMenu *menu, enum NcMenuItemSource source,
                                  int64 *first, int64 *last);
bool ncm_menu_find_full_selected_range(NcMenu *menu,
                                       enum NcMenuItemSource source,
                                       int64 *first, int64 *last);

#endif /* NCMPCPP_HELPERS_H */
