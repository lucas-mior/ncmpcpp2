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
