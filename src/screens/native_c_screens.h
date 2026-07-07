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
#include "screens/nc_browser.h"
#include "screens/nc_help.h"
#include "screens/nc_outputs.h"
#include "screens/nc_media_library.h"
#include "screens/nc_playlist.h"
#include "screens/nc_playlist_editor.h"
#include "screens/nc_sel_items_adder.h"
#include "screens/nc_sort_playlist.h"
#include "screens/nc_search_engine.h"
#include "screens/nc_server_info.h"
#include "screens/nc_song_info.h"
#include "screens/nc_screen.h"

NCM_EXTERN_C_BEGIN

typedef struct NativeHelpScreen NativeHelpScreen;
typedef struct NativeOutputsScreen NativeOutputsScreen;
typedef struct NativeServerInfoScreen NativeServerInfoScreen;
typedef struct NativeSongInfoScreen NativeSongInfoScreen;


void native_c_screen_browser_init(void);
void native_c_screen_browser_register(void);
void native_c_screen_browser_set_resize(void);
void native_c_screen_browser_switch_to(void);
bool native_c_screen_browser_is_current(void);
NativeBrowserScreen *native_c_screen_browser(void);
NcScreen *native_c_screen_browser_native(void);

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

void native_c_screen_playlist_editor_init(void);
void native_c_screen_playlist_editor_register(void);
void native_c_screen_playlist_editor_set_resize(void);
void native_c_screen_playlist_editor_switch_to(void);
bool native_c_screen_playlist_editor_is_current(void);
NativePlaylistEditorScreen *native_c_screen_playlist_editor(void);
NcScreen *native_c_screen_playlist_editor_native(void);

void native_c_screen_selected_items_adder_init(void);
void native_c_screen_selected_items_adder_register(void);
void native_c_screen_selected_items_adder_set_resize(void);
void native_c_screen_selected_items_adder_switch_to(void);
bool native_c_screen_selected_items_adder_is_current(void);
NativeSelectedItemsAdderScreen *native_c_screen_selected_items_adder(void);
NcScreen *native_c_screen_selected_items_adder_native(void);

void native_c_screen_sort_playlist_dialog_init(void);
void native_c_screen_sort_playlist_dialog_register(void);
void native_c_screen_sort_playlist_dialog_set_resize(void);
void native_c_screen_sort_playlist_dialog_switch_to(void);
bool native_c_screen_sort_playlist_dialog_is_current(void);
NativeSortPlaylistDialog *native_c_screen_sort_playlist_dialog(void);
NcScreen *native_c_screen_sort_playlist_dialog_native(void);

void native_c_screen_search_engine_init(void);
void native_c_screen_search_engine_register(void);
void native_c_screen_search_engine_set_resize(void);
void native_c_screen_search_engine_switch_to(void);
bool native_c_screen_search_engine_is_current(void);
NativeSearchEngineScreen *native_c_screen_search_engine(void);
NcScreen *native_c_screen_search_engine_native(void);

void native_c_screen_media_library_init(void);
void native_c_screen_media_library_register(void);
void native_c_screen_media_library_set_resize(void);
void native_c_screen_media_library_switch_to(void);
bool native_c_screen_media_library_is_current(void);
NativeMediaLibraryScreen *native_c_screen_media_library(void);
NcScreen *native_c_screen_media_library_native(void);

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
