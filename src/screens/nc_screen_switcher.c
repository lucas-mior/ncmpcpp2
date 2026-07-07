#include "screens/screen_switcher.h"

#include <stddef.h>

#include "app_controller.h"

bool
nc_screen_switcher_switch_to(NcScreen *screen,
                             bool has_to_be_resized) {
    if (screen == NULL) {
        return false;
    }
    nc_screen_set_has_to_be_resized(screen, has_to_be_resized);
    return app_controller_switch_to_screen(screen);
}

bool
nc_screen_switcher_finish_switch(NcScreen *screen) {
    (void)screen;
    return app_controller_last_switch_changed_screen();
}
