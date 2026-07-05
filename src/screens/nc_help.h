#if !defined(NCMPCPP_NC_HELP_H)
#define NCMPCPP_NC_HELP_H

#include "curses/nc_buffer.h"
#include "curses/nc_scrollpad.h"
#include "screens/nc_scrollpad_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcHelpScreen NcHelpScreen;

typedef struct NcHelpHooks {
    bool (*render)(void *user, NcBuffer *buffer);
    void (*switch_to)(void *user);
    void (*resize_layout)(void *user, NcHelpScreen *screen);
    void (*resize_background)(void *user);
    void (*destroy)(void *user);
    void *user;
} NcHelpHooks;

struct NcHelpScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    NcHelpHooks hooks;

    int64 lines_scrolled;
};

void nc_help_screen_init(NcHelpScreen *screen,
                         NcHelpHooks hooks,
                         int64 start_x, int64 width,
                         int64 main_start_y, int64 main_height,
                         NcColor color, NcBorder border,
                         int64 lines_scrolled);
void nc_help_screen_destroy(NcHelpScreen *screen);
void nc_help_screen_set_geometry(NcHelpScreen *screen,
                                 int64 start_x, int64 width,
                                 int64 main_start_y,
                                 int64 main_height);
bool nc_help_screen_reload(NcHelpScreen *screen);
NcScreen *nc_help_screen_base(NcHelpScreen *screen);
NcWindow *nc_help_screen_window(NcHelpScreen *screen);
int64 nc_help_screen_start_x(NcHelpScreen *screen);
int64 nc_help_screen_start_y(NcHelpScreen *screen);
int64 nc_help_screen_width(NcHelpScreen *screen);
int64 nc_help_screen_height(NcHelpScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_HELP_H */
