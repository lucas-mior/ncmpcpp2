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

#include "global.h"
#include "interfaces.h"

class SwitchTo
{
    static void setPreviousScreen(BaseScreen *screen)
    {
        Tabbable *tabbable = dynamic_cast<Tabbable *>(screen);

        if (tabbable == nullptr)
            return;
        if (dynamic_cast<Tabbable *>(Global::myScreen) == nullptr)
            return;
        tabbable->setPreviousScreen(Global::myScreen);
    }

public:
    static void execute(BaseScreen *screen)
    {
        NcScreen *native_screen;
        bool switched;

        native_screen = screen->nativeScreen();
        assert(native_screen != nullptr);

        nc_screen_set_has_to_be_resized(native_screen,
                                        screen->hasToBeResized);
        switched = nc_screen_registry_switch_to(&Global::myScreenRegistry,
                                                native_screen);
        assert(switched);
        (void)switched;
    }

    static void finishNativeSwitch(BaseScreen *screen)
    {
        if (Global::myScreen != screen)
            setPreviousScreen(screen);
        Global::myScreen = screen;
        syncLegacyScreenPointers();
    }
};

#endif // NCMPCPP_SCREEN_SWITCHER_H
