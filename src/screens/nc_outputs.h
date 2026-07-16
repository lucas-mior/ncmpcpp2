#if !defined(NCMPCPP_NC_OUTPUTS_H)
#define NCMPCPP_NC_OUTPUTS_H

#include "curses/nc_menu.h"
#include "screens/nc_scrollpad_screen.h"

typedef struct NcOutputsScreen NcOutputsScreen;

typedef struct NcOutputsHooks {
    void (*fetch_outputs)(void *user, NcOutputsScreen *screen);
    bool (*toggle_output)(void *user, uint32 id, bool enabled,
                          char *name, int32 name_len);
    void (*switch_to)(void *user);
    void (*resize_layout)(void *user, NcOutputsScreen *screen);
    void (*resize_background)(void *user);
    void (*destroy)(void *user);
    void *user;
} NcOutputsHooks;

typedef struct NcOutputsItem {
    char *name;
    int32 name_len;
    uint32 id;
    bool enabled;
} NcOutputsItem;

struct NcOutputsScreen {
    NcScrollpadScreen menu_screen;
    NcWindow window;
    NcMenu menu;
    NcOutputsHooks hooks;
    int64 lines_scrolled;
    bool mouse_scroll_whole_page;
};

void nc_outputs_screen_init(NcOutputsScreen *screen,
                            NcOutputsHooks hooks,
                            int64 start_x, int64 width,
                            int64 main_start_y, int64 main_height,
                            NcColor color, NcBorder border,
                            int64 lines_scrolled,
                            bool mouse_scroll_whole_page);
void nc_outputs_screen_set_geometry(NcOutputsScreen *screen,
                                    int64 start_x, int64 width,
                                    int64 main_start_y,
                                    int64 main_height);
void nc_outputs_screen_set_highlight_prefix(NcOutputsScreen *screen,
                                            NcBuffer *buffer);
void nc_outputs_screen_set_highlight_suffix(NcOutputsScreen *screen,
                                            NcBuffer *buffer);
void nc_outputs_screen_fetch_list(NcOutputsScreen *screen);
void nc_outputs_screen_clear_outputs(NcOutputsScreen *screen);
void nc_outputs_screen_add_output(NcOutputsScreen *screen,
                                  uint32 id,
                                  char *name,
                                  int32 name_len,
                                  bool enabled);
bool nc_outputs_screen_toggle_current(NcOutputsScreen *screen);
NcScreen *nc_outputs_screen_base(NcOutputsScreen *screen);
int64 nc_outputs_screen_start_x(NcOutputsScreen *screen);
int64 nc_outputs_screen_start_y(NcOutputsScreen *screen);
int64 nc_outputs_screen_width(NcOutputsScreen *screen);
int64 nc_outputs_screen_height(NcOutputsScreen *screen);

#endif /* NCMPCPP_NC_OUTPUTS_H */
