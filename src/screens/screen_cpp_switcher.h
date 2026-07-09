#ifndef NCMPCPP_SCREEN_CPP_SWITCHER_H
#define NCMPCPP_SCREEN_CPP_SWITCHER_H

#include <cassert>

#include "interfaces.h"
#include "screens/screen_cpp_compat.h"
#include "screens/screen_switcher.h"

class SwitchTo
{
public:
    static void execute(BaseScreen *screen)
    {
        NcScreen *native_screen;
        bool switched;

        native_screen = screen->nativeScreen();
        assert(native_screen != nullptr);

        switched = nc_screen_switcher_switch_to(native_screen,
                                                screen->hasToBeResized);
        assert(switched);
        (void)switched;
    }

    static void finishNativeSwitch(BaseScreen *screen)
    {
        if (nc_screen_switcher_finish_switch(screen->nativeScreen()))
            screen_compat::set_tab_previous_screen(screen);
        syncLegacyScreenPointers();
    }
};

#endif // NCMPCPP_SCREEN_CPP_SWITCHER_H
