#if !defined(NCMPCPP_TITLE_H)
#define NCMPCPP_TITLE_H

#include "c/ncm_defs.h"

NCM_EXTERN_C_BEGIN

void ncm_window_title_set(char *title, int32 title_len);
void ncm_window_title_set_cstring(char *title);
void ncm_title_draw_header(char *title, int32 title_len);

NCM_EXTERN_C_END

#endif /* NCMPCPP_TITLE_H */
