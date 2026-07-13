#ifndef NCMPCPP_SCREEN_CPP_LEGACY_H
#define NCMPCPP_SCREEN_CPP_LEGACY_H

#include "app_controller.h"
#include "screens/screen_cpp_compat.h"

inline BaseScreen *screenLegacyCurrent()
{
    return BaseScreen::legacyFromNativeScreen(
        app_controller_current_screen());
}

inline BaseScreen *screenLegacyLocked()
{
    return BaseScreen::legacyFromNativeScreen(
        app_controller_locked_screen());
}

#endif // NCMPCPP_SCREEN_CPP_LEGACY_H
