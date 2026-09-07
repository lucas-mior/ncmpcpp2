#if !defined(TITLE_H)
#define TITLE_H

#include "cbase.h"

#include "c/ncm_c.h"
#include "curses/nc_curses.h"

void ncm_window_title_configure(bool enabled, bool quiet);
void ncm_window_title_write(char *, int32);
void ncm_window_title_set(char *, int32);
void ncm_window_title_set_cstring(char *);
void ncm_title_draw_header_with_config(char *, int32, bool, enum Design,
                                       NcFormattedColor *volume_color,
                                       NcFormattedColor *separator_color);
void ncm_title_draw_header(char *, int32);
void ncm_title_draw_current_header(void);

#endif /* TITLE_H */
