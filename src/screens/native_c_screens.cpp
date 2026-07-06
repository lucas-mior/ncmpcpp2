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

#include "screens/native_c_screens.h"

#include "config.h"
#include "screens/help.h"
#include "screens/outputs.h"
#include "screens/screen.h"
#include "screens/screen_legacy.h"
#include "screens/server_info.h"
#include "screens/song_info.h"

void native_c_screen_help_init()
{
    myHelp = new Help;
}

void native_c_screen_help_register()
{
    myHelp->registerNativeScreen();
}

void native_c_screen_help_set_resize()
{
    myHelp->hasToBeResized = 1;
}

void native_c_screen_help_switch_to()
{
    myHelp->switchTo();
}

bool native_c_screen_help_is_current()
{
    return screenLegacyCurrent() == myHelp;
}

BaseScreen *native_c_screen_help_legacy()
{
    return myHelp;
}

NcScreen *native_c_screen_help_native()
{
    return myHelp->nativeScreen();
}

void native_c_screen_song_info_init()
{
    mySongInfo = new SongInfo;
}

void native_c_screen_song_info_register()
{
    mySongInfo->registerNativeScreen();
}

void native_c_screen_song_info_set_resize()
{
    mySongInfo->hasToBeResized = 1;
}

void native_c_screen_song_info_switch_to()
{
    mySongInfo->switchTo();
}

BaseScreen *native_c_screen_song_info_legacy()
{
    return mySongInfo;
}

NcScreen *native_c_screen_song_info_native()
{
    return mySongInfo->nativeScreen();
}

void native_c_screen_server_info_init()
{
    myServerInfo = new ServerInfo;
}

void native_c_screen_server_info_register()
{
    myServerInfo->registerNativeScreen();
}

void native_c_screen_server_info_set_resize()
{
    myServerInfo->hasToBeResized = 1;
}

void native_c_screen_server_info_switch_to()
{
    myServerInfo->switchTo();
}

BaseScreen *native_c_screen_server_info_legacy()
{
    return myServerInfo;
}

NcScreen *native_c_screen_server_info_native()
{
    return myServerInfo->nativeScreen();
}

void native_c_screen_outputs_init()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs = new Outputs;
#endif // ENABLE_OUTPUTS
}

void native_c_screen_outputs_register()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->registerNativeScreen();
#endif // ENABLE_OUTPUTS
}

void native_c_screen_outputs_set_resize()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->hasToBeResized = 1;
#endif // ENABLE_OUTPUTS
}

void native_c_screen_outputs_switch_to()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->switchTo();
#endif // ENABLE_OUTPUTS
}

bool native_c_screen_outputs_is_current()
{
#if defined(ENABLE_OUTPUTS)
    return screenLegacyCurrent() == myOutputs;
#else
    return false;
#endif // ENABLE_OUTPUTS
}

void native_c_screen_outputs_toggle()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->toggleOutput();
#endif // ENABLE_OUTPUTS
}

void native_c_screen_outputs_fetch_list()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->fetchList();
#endif // ENABLE_OUTPUTS
}

void native_c_screen_outputs_refresh_if_visible()
{
#if defined(ENABLE_OUTPUTS)
    if (isVisible(myOutputs)) {
        myOutputs->refreshWindow();
    }
#endif // ENABLE_OUTPUTS
}

BaseScreen *native_c_screen_outputs_legacy()
{
#if defined(ENABLE_OUTPUTS)
    return myOutputs;
#else
    return nullptr;
#endif // ENABLE_OUTPUTS
}

NcScreen *native_c_screen_outputs_native()
{
#if defined(ENABLE_OUTPUTS)
    return myOutputs->nativeScreen();
#else
    return nullptr;
#endif // ENABLE_OUTPUTS
}
