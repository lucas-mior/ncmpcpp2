/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#ifndef NCMPCPP_SCREEN_SWITCHER_H
#define NCMPCPP_SCREEN_SWITCHER_H

#include <cassert>
#include <type_traits>

#include "global.h"
#include "interfaces.h"

class SwitchTo
{
    template <bool ToBeExecuted, typename ScreenT> struct TabbableAction_ {
        static void execute(ScreenT *) { }
    };
    template <typename ScreenT> struct TabbableAction_<true, ScreenT> {
        static void execute(ScreenT *screen) {
            using Global::myScreen;
            // It has to work only if both current and previous screens
            // are Tabbable.
            if (dynamic_cast<Tabbable *>(myScreen))
                screen->setPreviousScreen(myScreen);
        }
    };

public:
    template <typename ScreenT>
    static void execute(ScreenT *screen)
    {
        NcScreen *native_screen;

        native_screen = screen->nativeScreen();
        if (native_screen)
        {
            nc_screen_set_has_to_be_resized(native_screen,
                                            screen->hasToBeResized);
            bool switched = nc_screen_registry_switch_to(
                &Global::myScreenRegistry, native_screen);
            assert(switched);
            (void)switched;
            return;
        }

        executeLegacy(screen);
    }

    template <typename ScreenT>
    static void finishNativeSwitch(ScreenT *screen)
    {
        using Global::myScreen;

        const bool isScreenTabbable = std::is_base_of<Tabbable, ScreenT>::value;

        if (myScreen != screen)
            TabbableAction_<isScreenTabbable, ScreenT>::execute(screen);
        myScreen = screen;
        syncLegacyScreenPointers();
    }

    template <typename ScreenT>
    static void executeLegacy(ScreenT *screen)
    {
        using Global::myScreen;
        using Global::myLockedScreen;

        const bool isScreenMergable = screen->isMergable() && myLockedScreen;
        const bool isScreenTabbable = std::is_base_of<Tabbable, ScreenT>::value;

        assert(myScreen != screen);
        if (isScreenMergable)
            updateInactiveScreen(screen);
        if (screen->hasToBeResized || isScreenMergable)
            screen->resize();
        TabbableAction_<isScreenTabbable, ScreenT>::execute(screen);
        myScreen = screen;
    }
};

#endif // NCMPCPP_SCREEN_SWITCHER_H
