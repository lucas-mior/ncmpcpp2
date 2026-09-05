#if !defined(NCMPCPP_TITLE_H)
#define NCMPCPP_TITLE_H

#include "cbase.h"

#include "c/ncm_c.h"
#include "curses/nc_curses.h"

void ncm_window_title_configure(bool enabled, bool quiet);
void ncm_window_title_write(char *title, int32 title_len);
void ncm_window_title_set(char *title, int32 title_len);
void ncm_window_title_set_cstring(char *title);
void ncm_title_draw_header_with_config(char *title, int32 title_len,
                                       bool header_visibility,
                                       enum Design design,
                                       NcFormattedColor *volume_color,
                                       NcFormattedColor *separator_color);
void ncm_title_draw_header(char *title, int32 title_len);
void ncm_title_draw_current_header(void);

#endif /* NCMPCPP_TITLE_H */
