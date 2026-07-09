#ifndef NCMPCPP_SCREEN_CPP_LEGACY_H
#define NCMPCPP_SCREEN_CPP_LEGACY_H

#include "app_controller.h"
#include "screens/screen_cpp_compat.h"

inline BaseScreen *screenLegacyCurrent()
{
    return BaseScreen::legacyFromNativeScreen(
        app_controller_current_screen());
}

inline BaseScreen *screenLegacyPrevious()
{
    return BaseScreen::legacyFromNativeScreen(
        app_controller_previous_screen());
}

inline BaseScreen *screenLegacyLocked()
{
    return BaseScreen::legacyFromNativeScreen(
        app_controller_locked_screen());
}

inline BaseScreen *screenLegacyInactive()
{
    return BaseScreen::legacyFromNativeScreen(
        app_controller_inactive_screen());
}

inline bool screenLegacyIsCurrent(BaseScreen *screen)
{
    return screenLegacyCurrent() == screen;
}

inline bool screenLegacySwitchChanged()
{
    return app_controller_last_switch_changed_screen();
}

#endif // NCMPCPP_SCREEN_CPP_LEGACY_H
