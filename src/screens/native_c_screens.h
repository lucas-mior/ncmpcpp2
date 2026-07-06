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

#endif // NCMPCPP_NATIVE_C_SCREENS_H
