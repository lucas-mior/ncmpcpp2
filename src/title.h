#if !defined(NCMPCPP_TITLE_H)
#define NCMPCPP_TITLE_H

#include "c/ncm_defs.h"
#include "c/ncm_enums.h"
#include "curses/nc_formatted_color.h"

NCM_EXTERN_C_BEGIN

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

NCM_EXTERN_C_END

#endif /* NCMPCPP_TITLE_H */
