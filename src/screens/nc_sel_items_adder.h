#if !defined(NCMPCPP_NC_SEL_ITEMS_ADDER_H)
#define NCMPCPP_NC_SEL_ITEMS_ADDER_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

typedef struct NativePlaylistScreen NativePlaylistScreen;

enum NativeSelectedItemsAdderMenu {
    NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS,
    NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS,
};

typedef struct NativeSelectedItemsAdderScreen {
    NcScreen screen;
    NcEditorActionMenu playlist_selector;
    NcEditorActionMenu position_selector;
    NcWindow playlist_window;
    NcWindow position_window;
    NcmSongArray selected_songs;
    NcmRegex search_regex;
    NcmBuffer search_constraint;
    NativePlaylistScreen *playlist;
    NcScreen *previous_screen;
    NcmMpdClient *client;

    int64 playlist_width;
    int64 playlist_height;
    int64 position_width;
    int64 position_height;
    int64 active_menu;

    bool local_browser;
    bool search_enabled;
    bool registered;
    bool ready;
} NativeSelectedItemsAdderScreen;

void native_selected_items_adder_screen_init(
    NativeSelectedItemsAdderScreen *screen, int64 start_x, int64 start_y,
    int64 width, int64 height, NcColor color, NcBorder border);
void native_selected_items_adder_screen_destroy(
    NativeSelectedItemsAdderScreen *screen);
NcScreen *native_selected_items_adder_screen_base(
    NativeSelectedItemsAdderScreen *screen);
NcMenu *native_selected_items_adder_screen_active_menu(
    NativeSelectedItemsAdderScreen *screen);
NcWindow *native_selected_items_adder_screen_active_window(
    NativeSelectedItemsAdderScreen *screen);
bool native_selected_items_adder_screen_open(
    NativeSelectedItemsAdderScreen *screen, NcmSongArray *songs,
    NativePlaylistScreen *playlist, NcmMpdClient *client, NcmError *error);
void native_selected_items_adder_screen_populate_playlist_selector(
    NativeSelectedItemsAdderScreen *screen, NcmMpdPlaylistList *playlists,
    bool local_browser);
void native_selected_items_adder_screen_populate_position_selector(
    NativeSelectedItemsAdderScreen *screen);
bool native_selected_items_adder_screen_run_current(
    NativeSelectedItemsAdderScreen *screen);
bool native_selected_items_adder_screen_return_to_previous(
    NativeSelectedItemsAdderScreen *screen);
void native_selected_items_adder_screen_choose_current_playlist(
    NativeSelectedItemsAdderScreen *screen);
bool native_selected_items_adder_screen_add_to_existing_playlist(
    NativeSelectedItemsAdderScreen *screen, NcmMpdClient *client,
    char *playlist, NcmError *error);
bool native_selected_items_adder_screen_search(
    NativeSelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, bool forward, bool wrap,
    bool skip_current, NcmError *error);

#endif /* NCMPCPP_NC_SEL_ITEMS_ADDER_H */
