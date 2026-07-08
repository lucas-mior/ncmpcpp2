#if !defined(NCMPCPP_UI_STATE_H)
#define NCMPCPP_UI_STATE_H

#include "cbase/primitives.h"
#include "curses/nc_window.h"

#if defined(__cplusplus)
extern "C" {
#endif

void ui_state_set_header_window(NcWindow *window);
NcWindow *ui_state_header_window(void);
void ui_state_set_footer_window(NcWindow *window);
NcWindow *ui_state_footer_window(void);
void ui_state_set_header_legacy_window(void *window);
void *ui_state_header_legacy_window(void);
void ui_state_set_footer_legacy_window(void *window);
void *ui_state_footer_legacy_window(void);
void ui_state_set_screen_size(int64 width, int64 height);
int64 ui_state_screen_width(void);
int64 ui_state_screen_height(void);
void ui_state_set_main_geometry(int64 start_y, int64 height);
void ui_state_set_main_start_y(int64 value);
int64 ui_state_main_start_y(void);
void ui_state_set_main_height(int64 value);
int64 ui_state_main_height(void);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_UI_STATE_H */
