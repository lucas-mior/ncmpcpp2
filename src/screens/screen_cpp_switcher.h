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
