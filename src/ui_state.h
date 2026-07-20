#if !defined(NCMPCPP_UI_STATE_H)
#define NCMPCPP_UI_STATE_H

#include <stdbool.h>

#include "cbase/primitives.h"
#include "curses/nc_window.h"

void ui_state_set_header_window(NcWindow *window);
NcWindow *ui_state_header_window(void);
void ui_state_set_footer_window(NcWindow *window);
NcWindow *ui_state_footer_window(void);
void ui_state_set_screen_size(int32 width, int32 height);
int32 ui_state_screen_width(void);
int32 ui_state_screen_height(void);
void ui_state_set_main_geometry(int32 start_y, int32 height);
int32 ui_state_main_start_y(void);
int32 ui_state_main_height(void);
void ui_state_set_header_height(int32 value);
int32 ui_state_header_height(void);
void ui_state_set_footer_height(int32 value);
int32 ui_state_footer_height(void);
void ui_state_set_footer_start_y(int32 value);
int32 ui_state_footer_start_y(void);
void ui_state_set_statusbar_visibility_baseline(bool value);
bool ui_state_statusbar_visibility_baseline(void);

#endif /* NCMPCPP_UI_STATE_H */
