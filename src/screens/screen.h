#if !defined(NCMPCPP_SCREEN_H)
#define NCMPCPP_SCREEN_H

#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

static inline NcWindow *
screen_active_window(NcScreen *screen) {
    return nc_screen_active_window(screen);
}

static inline void
screen_refresh(NcScreen *screen) {
    nc_screen_refresh(screen);
    return;
}

static inline void
screen_refresh_window(NcScreen *screen) {
    nc_screen_refresh_window(screen);
    return;
}

static inline void
screen_scroll(NcScreen *screen, enum NcScroll where) {
    nc_screen_scroll(screen, where);
    return;
}

static inline void
screen_switch_to(NcScreen *screen) {
    nc_screen_switch_to(screen);
    return;
}

static inline void
screen_resize(NcScreen *screen) {
    nc_screen_resize(screen);
    return;
}

static inline int32
screen_window_timeout(NcScreen *screen) {
    return nc_screen_window_timeout(screen);
}

static inline char *
screen_title(NcScreen *screen) {
    return nc_screen_title(screen);
}

static inline int32
screen_type(NcScreen *screen) {
    return nc_screen_type(screen);
}

static inline void
screen_set_type(NcScreen *screen, int32 type) {
    nc_screen_set_type(screen, type);
    return;
}

static inline void
screen_update(NcScreen *screen) {
    nc_screen_update(screen);
    return;
}

static inline void
screen_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    nc_screen_mouse_button_pressed(screen, event);
    return;
}

static inline bool
screen_is_lockable(NcScreen *screen) {
    return nc_screen_is_lockable(screen);
}

static inline bool
screen_is_mergable(NcScreen *screen) {
    return nc_screen_is_mergable(screen);
}

static inline bool
screen_has_to_be_resized(NcScreen *screen) {
    return nc_screen_has_to_be_resized(screen);
}

static inline void
screen_set_has_to_be_resized(NcScreen *screen, bool has_to_be_resized) {
    nc_screen_set_has_to_be_resized(screen, has_to_be_resized);
    return;
}

static inline void
screen_destroy(NcScreen *screen) {
    nc_screen_destroy(screen);
    return;
}

static inline void *
screen_user(NcScreen *screen) {
    return nc_screen_user(screen);
}

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_SCREEN_H */
