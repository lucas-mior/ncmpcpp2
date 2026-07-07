#if !defined(NCMPCPP_SCREEN_LEGACY_H)
#define NCMPCPP_SCREEN_LEGACY_H

#include "app_controller.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

static inline NcScreen *
screen_legacy_current(void) {
    return app_controller_current_screen();
}

static inline NcScreen *
screen_legacy_previous(void) {
    return app_controller_previous_screen();
}

static inline NcScreen *
screen_legacy_locked(void) {
    return app_controller_locked_screen();
}

static inline NcScreen *
screen_legacy_inactive(void) {
    return app_controller_inactive_screen();
}

static inline bool
screen_legacy_is_current(NcScreen *screen) {
    return app_controller_current_screen() == screen;
}

static inline bool
screen_legacy_switch_changed(void) {
    return app_controller_last_switch_changed_screen();
}

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_SCREEN_LEGACY_H */
