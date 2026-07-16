#ifndef NCMPCPP_NATIVE_C_SCREENS_H
#define NCMPCPP_NATIVE_C_SCREENS_H

#include <stdbool.h>

#include "c/ncm_defs.h"
#include "screens/nc_browser.h"
#include "screens/nc_help.h"
#include "screens/nc_lastfm.h"
#include "screens/nc_lyrics.h"
#include "screens/nc_outputs.h"
#include "screens/nc_media_library.h"
#include "screens/nc_playlist.h"
#include "screens/nc_playlist_editor.h"
#include "screens/nc_sel_items_adder.h"
#include "screens/nc_sort_playlist.h"
#include "screens/nc_tag_editor.h"
#include "screens/nc_tiny_tag_editor.h"
#include "screens/nc_visualizer.h"
#include "screens/nc_server_info.h"
#include "screens/nc_song_info.h"
#include "screens/nc_screen.h"
#include "screens/screen_type.h"

NCM_EXTERN_C_BEGIN

typedef struct NativeHelpScreen NativeHelpScreen;
typedef struct NativeOutputsScreen NativeOutputsScreen;
typedef struct NativeSearchEngineScreen NativeSearchEngineScreen;
typedef struct NativeServerInfoScreen NativeServerInfoScreen;
typedef struct NativeSongInfoScreen NativeSongInfoScreen;


void native_c_screens_init_all(void);
void native_c_screens_register_native_only(void);
void native_c_screens_request_registered_resize(void);
bool native_c_screens_is_registered_type(enum ScreenType screen_type);
NcScreen *native_c_screens_find_type(enum ScreenType screen_type);
bool native_c_screens_switch_to_type(enum ScreenType screen_type);
bool native_c_screens_lock_current(void);
enum ScreenType native_c_screens_current_type(void);

void native_c_screen_browser_init(void);
void native_c_screen_browser_register(void);
void native_c_screen_browser_set_resize(void);
void native_c_screen_browser_switch_to(void);
bool native_c_screen_browser_is_current(void);
void native_c_screen_browser_fetch_supported_extensions(void);
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

void native_c_screen_lastfm_init(void);
void native_c_screen_lastfm_register(void);
void native_c_screen_lastfm_set_resize(void);
void native_c_screen_lastfm_switch_to(void);
bool native_c_screen_lastfm_is_current(void);
NativeLastfmScreen *native_c_screen_lastfm(void);
NcLastfmScreen *native_c_screen_lastfm_typed(void);
NcScreen *native_c_screen_lastfm_native(void);

void native_c_screen_lyrics_init(void);
void native_c_screen_lyrics_register(void);
void native_c_screen_lyrics_set_resize(void);
void native_c_screen_lyrics_switch_to(void);
bool native_c_screen_lyrics_is_current(void);
NativeLyricsScreen *native_c_screen_lyrics(void);
NcLyricsScreen *native_c_screen_lyrics_typed(void);
NcScreen *native_c_screen_lyrics_native(void);

void native_c_screen_visualizer_init(void);
void native_c_screen_visualizer_register(void);
void native_c_screen_visualizer_set_resize(void);
void native_c_screen_visualizer_switch_to(void);
bool native_c_screen_visualizer_is_current(void);
NativeVisualizerScreen *native_c_screen_visualizer(void);
NcScreen *native_c_screen_visualizer_native(void);

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
bool native_c_screen_selected_items_adder_open(
    NcmSongArray *songs, NcmError *error);
bool native_c_screen_selected_items_adder_is_current(void);
NativeSelectedItemsAdderScreen *native_c_screen_selected_items_adder(void);
NcScreen *native_c_screen_selected_items_adder_native(void);

void native_c_screen_sort_playlist_dialog_init(void);
void native_c_screen_sort_playlist_dialog_register(void);
void native_c_screen_sort_playlist_dialog_set_resize(void);
bool native_c_screen_sort_playlist_dialog_switch_to(void);
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


void native_c_screen_tag_editor_init(void);
void native_c_screen_tag_editor_register(void);
void native_c_screen_tag_editor_set_resize(void);
void native_c_screen_tag_editor_switch_to(void);
bool native_c_screen_tag_editor_is_current(void);
NativeTagEditorScreen *native_c_screen_tag_editor(void);
NcScreen *native_c_screen_tag_editor_native(void);

void native_c_screen_tiny_tag_editor_init(void);
void native_c_screen_tiny_tag_editor_register(void);
void native_c_screen_tiny_tag_editor_set_resize(void);
void native_c_screen_tiny_tag_editor_switch_to(void);
bool native_c_screen_tiny_tag_editor_is_current(void);
NativeTinyTagEditorScreen *native_c_screen_tiny_tag_editor(void);
NcScreen *native_c_screen_tiny_tag_editor_native(void);

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
