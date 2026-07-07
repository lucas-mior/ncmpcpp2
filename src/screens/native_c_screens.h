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

#ifndef NCMPCPP_NATIVE_C_SCREENS_H
#define NCMPCPP_NATIVE_C_SCREENS_H

#include <stdbool.h>

#include "screens/nc_screen.h"

struct BaseScreen;

void native_c_screen_help_init();
void native_c_screen_help_register();
void native_c_screen_help_set_resize();
void native_c_screen_help_switch_to();
bool native_c_screen_help_is_current();
struct BaseScreen *native_c_screen_help_legacy();
NcScreen *native_c_screen_help_native();

void native_c_screen_song_info_init();
void native_c_screen_song_info_register();
void native_c_screen_song_info_set_resize();
void native_c_screen_song_info_switch_to();
struct BaseScreen *native_c_screen_song_info_legacy();
NcScreen *native_c_screen_song_info_native();

void native_c_screen_server_info_init();
void native_c_screen_server_info_register();
void native_c_screen_server_info_set_resize();
void native_c_screen_server_info_switch_to();
struct BaseScreen *native_c_screen_server_info_legacy();
NcScreen *native_c_screen_server_info_native();

void native_c_screen_outputs_init();
void native_c_screen_outputs_register();
void native_c_screen_outputs_set_resize();
void native_c_screen_outputs_switch_to();
bool native_c_screen_outputs_is_current();
void native_c_screen_outputs_toggle();
void native_c_screen_outputs_fetch_list();
void native_c_screen_outputs_refresh_if_visible();
struct BaseScreen *native_c_screen_outputs_legacy();
NcScreen *native_c_screen_outputs_native();

#if defined(__cplusplus)

#include "config.h"
#include "screens/help.h"
#include "screens/outputs.h"
#include "screens/screen_cpp_compat.h"
#include "screens/screen_cpp_legacy.h"
#include "screens/server_info.h"
#include "screens/song_info.h"

inline void native_c_screen_help_init()
{
    myHelp = new Help;
}

inline void native_c_screen_help_register()
{
    myHelp->registerNativeScreen();
}

inline void native_c_screen_help_set_resize()
{
    myHelp->hasToBeResized = 1;
}

inline void native_c_screen_help_switch_to()
{
    myHelp->switchTo();
}

inline bool native_c_screen_help_is_current()
{
    return screenLegacyCurrent() == myHelp;
}

inline BaseScreen *native_c_screen_help_legacy()
{
    return myHelp;
}

inline NcScreen *native_c_screen_help_native()
{
    return myHelp->nativeScreen();
}

inline void native_c_screen_song_info_init()
{
    mySongInfo = new SongInfo;
}

inline void native_c_screen_song_info_register()
{
    mySongInfo->registerNativeScreen();
}

inline void native_c_screen_song_info_set_resize()
{
    mySongInfo->hasToBeResized = 1;
}

inline void native_c_screen_song_info_switch_to()
{
    mySongInfo->switchTo();
}

inline BaseScreen *native_c_screen_song_info_legacy()
{
    return mySongInfo;
}

inline NcScreen *native_c_screen_song_info_native()
{
    return mySongInfo->nativeScreen();
}

inline void native_c_screen_server_info_init()
{
    myServerInfo = new ServerInfo;
}

inline void native_c_screen_server_info_register()
{
    myServerInfo->registerNativeScreen();
}

inline void native_c_screen_server_info_set_resize()
{
    myServerInfo->hasToBeResized = 1;
}

inline void native_c_screen_server_info_switch_to()
{
    myServerInfo->switchTo();
}

inline BaseScreen *native_c_screen_server_info_legacy()
{
    return myServerInfo;
}

inline NcScreen *native_c_screen_server_info_native()
{
    return myServerInfo->nativeScreen();
}

inline void native_c_screen_outputs_init()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs = new Outputs;
#endif // ENABLE_OUTPUTS
}

inline void native_c_screen_outputs_register()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->registerNativeScreen();
#endif // ENABLE_OUTPUTS
}

inline void native_c_screen_outputs_set_resize()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->hasToBeResized = 1;
#endif // ENABLE_OUTPUTS
}

inline void native_c_screen_outputs_switch_to()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->switchTo();
#endif // ENABLE_OUTPUTS
}

inline bool native_c_screen_outputs_is_current()
{
#if defined(ENABLE_OUTPUTS)
    return screenLegacyCurrent() == myOutputs;
#else
    return false;
#endif // ENABLE_OUTPUTS
}

inline void native_c_screen_outputs_toggle()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->toggleOutput();
#endif // ENABLE_OUTPUTS
}

inline void native_c_screen_outputs_fetch_list()
{
#if defined(ENABLE_OUTPUTS)
    myOutputs->fetchList();
#endif // ENABLE_OUTPUTS
}

inline void native_c_screen_outputs_refresh_if_visible()
{
#if defined(ENABLE_OUTPUTS)
    if (isVisible(myOutputs))
        myOutputs->refreshWindow();
#endif // ENABLE_OUTPUTS
}

inline BaseScreen *native_c_screen_outputs_legacy()
{
#if defined(ENABLE_OUTPUTS)
    return myOutputs;
#else
    return nullptr;
#endif // ENABLE_OUTPUTS
}

inline NcScreen *native_c_screen_outputs_native()
{
#if defined(ENABLE_OUTPUTS)
    return myOutputs->nativeScreen();
#else
    return nullptr;
#endif // ENABLE_OUTPUTS
}

#endif // defined(__cplusplus)

#endif // NCMPCPP_NATIVE_C_SCREENS_H
