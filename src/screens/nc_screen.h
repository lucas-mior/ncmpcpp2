#if !defined(NCMPCPP_NC_SCREEN_H)
#define NCMPCPP_NC_SCREEN_H

#include <stdbool.h>

#include "cbase/primitives.h"
#include "curses/nc_window.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NC_SCREEN_DEFAULT_WINDOW_TIMEOUT 500

typedef struct NcScreen NcScreen;

typedef struct NcScreenCallbacks {
    NcWindow *(*active_window)(NcScreen *screen);
    void (*refresh)(NcScreen *screen);
    void (*refresh_window)(NcScreen *screen);
    void (*scroll)(NcScreen *screen, enum NcScroll where);
    void (*switch_to)(NcScreen *screen);
    void (*resize)(NcScreen *screen);
    int32 (*window_timeout)(NcScreen *screen);
    char *(*title)(NcScreen *screen);
    void (*update)(NcScreen *screen);
    void (*mouse_button_pressed)(NcScreen *screen, MEVENT event);
    bool (*is_lockable)(NcScreen *screen);
    bool (*is_mergable)(NcScreen *screen);
} NcScreenCallbacks;

struct NcScreen {
    NcScreenCallbacks callbacks;
    void *user;
    int32 type;
    bool has_to_be_resized;
};

void nc_screen_init(NcScreen *screen, NcScreenCallbacks callbacks,
                    void *user, int32 type);
NcWindow *nc_screen_active_window(NcScreen *screen);
void nc_screen_refresh(NcScreen *screen);
void nc_screen_refresh_window(NcScreen *screen);
void nc_screen_scroll(NcScreen *screen, enum NcScroll where);
void nc_screen_switch_to(NcScreen *screen);
void nc_screen_resize(NcScreen *screen);
int32 nc_screen_window_timeout(NcScreen *screen);
char *nc_screen_title(NcScreen *screen);
int32 nc_screen_type(NcScreen *screen);
void nc_screen_update(NcScreen *screen);
void nc_screen_mouse_button_pressed(NcScreen *screen, MEVENT event);
bool nc_screen_is_lockable(NcScreen *screen);
bool nc_screen_is_mergable(NcScreen *screen);
bool nc_screen_has_to_be_resized(NcScreen *screen);
void nc_screen_set_has_to_be_resized(NcScreen *screen,
                                     bool has_to_be_resized);
void *nc_screen_user(NcScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SCREEN_H */
