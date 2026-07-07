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

#include "c/ncm_defs.h"
#include "screens/nc_help.h"
#include "screens/nc_outputs.h"
#include "screens/nc_playlist.h"
#include "screens/nc_server_info.h"
#include "screens/nc_song_info.h"
#include "screens/nc_screen.h"

NCM_EXTERN_C_BEGIN

typedef struct NativeHelpScreen NativeHelpScreen;
typedef struct NativeOutputsScreen NativeOutputsScreen;
typedef struct NativeServerInfoScreen NativeServerInfoScreen;
typedef struct NativeSongInfoScreen NativeSongInfoScreen;

void native_c_screen_help_init(void);
void native_c_screen_help_register(void);
void native_c_screen_help_set_resize(void);
void native_c_screen_help_switch_to(void);
bool native_c_screen_help_is_current(void);
NativeHelpScreen *native_c_screen_help(void);
NcHelpScreen *native_c_screen_help_typed(void);
NcScreen *native_c_screen_help_native(void);

void native_c_screen_playlist_init(void);
void native_c_screen_playlist_register(void);
void native_c_screen_playlist_set_resize(void);
void native_c_screen_playlist_switch_to(void);
bool native_c_screen_playlist_is_current(void);
NativePlaylistScreen *native_c_screen_playlist(void);
NcPlaylistScreen *native_c_screen_playlist_typed(void);
NcScreen *native_c_screen_playlist_native(void);

void native_c_screen_song_info_init(void);
void native_c_screen_song_info_register(void);
void native_c_screen_song_info_set_resize(void);
void native_c_screen_song_info_switch_to(void);
NativeSongInfoScreen *native_c_screen_song_info(void);
NcSongInfoScreen *native_c_screen_song_info_typed(void);
NcScreen *native_c_screen_song_info_native(void);

void native_c_screen_server_info_init(void);
void native_c_screen_server_info_register(void);
void native_c_screen_server_info_set_resize(void);
void native_c_screen_server_info_switch_to(void);
NativeServerInfoScreen *native_c_screen_server_info(void);
NcServerInfoScreen *native_c_screen_server_info_typed(void);
NcScreen *native_c_screen_server_info_native(void);

void native_c_screen_outputs_init(void);
void native_c_screen_outputs_register(void);
void native_c_screen_outputs_set_resize(void);
void native_c_screen_outputs_switch_to(void);
bool native_c_screen_outputs_is_current(void);
void native_c_screen_outputs_toggle(void);
void native_c_screen_outputs_fetch_list(void);
void native_c_screen_outputs_refresh_if_visible(void);
NativeOutputsScreen *native_c_screen_outputs(void);
NcOutputsScreen *native_c_screen_outputs_typed(void);
NcScreen *native_c_screen_outputs_native(void);

NCM_EXTERN_C_END

#endif /* NCMPCPP_NATIVE_C_SCREENS_H */
