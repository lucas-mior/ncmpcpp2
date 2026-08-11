#if !defined(NCMPCPP_NC_SEL_ITEMS_ADDER_H)
#define NCMPCPP_NC_SEL_ITEMS_ADDER_H

#include "cbase.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

typedef struct PlaylistScreen PlaylistScreen;

#define ENUM_NAME SelectedItemsAdderMenu
#define ENUM_PREFIX_ SELECTED_ITEMS_ADDER_MENU_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(SELECTED_ITEMS_ADDER_MENU_PLAYLISTS) \
    X(SELECTED_ITEMS_ADDER_MENU_POSITIONS)
#include "cbase/xenums.c"

typedef struct SelectedItemsAdderScreen {
    NcScreen screen;
    NcEditorActionMenu playlist_selector;
    NcEditorActionMenu position_selector;
    NcWindow playlist_window;
    NcWindow position_window;
    NcmSongArray selected_songs;
    NcmRegex search_regex;
    StrBuilder search_constraint;
    PlaylistScreen *playlist;
    NcScreen *previous_screen;
    NcmMpdClient *client;

    int32 playlist_width;
    int32 playlist_height;
    int32 position_width;
    int32 position_height;
    int32 active_menu;

    bool local_browser;
    bool search_enabled;
    bool registered;
    bool ready;
} SelectedItemsAdderScreen;

void selected_items_adder_screen_init(
    SelectedItemsAdderScreen *screen, int32 start_x, int32 start_y,
    int32 width, int32 height, NcColor color, NcBorder border);
void selected_items_adder_screen_destroy(
    SelectedItemsAdderScreen *screen);
NcScreen *selected_items_adder_screen_base(
    SelectedItemsAdderScreen *screen);
NcMenu *selected_items_adder_screen_active_menu(
    SelectedItemsAdderScreen *screen);
NcWindow *selected_items_adder_screen_active_window(
    SelectedItemsAdderScreen *screen);
bool selected_items_adder_screen_open(
    SelectedItemsAdderScreen *screen, NcmSongArray *songs,
    PlaylistScreen *playlist, NcmMpdClient *client, NcmError *error);
void selected_items_adder_screen_populate_playlist_selector(
    SelectedItemsAdderScreen *screen, NcmMpdPlaylistList *playlists,
    bool local_browser);
void selected_items_adder_screen_populate_position_selector(
    SelectedItemsAdderScreen *screen);
bool selected_items_adder_screen_run_current(
    SelectedItemsAdderScreen *screen);
bool selected_items_adder_screen_return_to_previous(
    SelectedItemsAdderScreen *screen);
void selected_items_adder_screen_choose_current_playlist(
    SelectedItemsAdderScreen *screen);
bool selected_items_adder_screen_add_to_existing_playlist(
    SelectedItemsAdderScreen *screen, NcmMpdClient *client,
    char *playlist, NcmError *error);
bool selected_items_adder_screen_search(
    SelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, bool forward, bool wrap,
    bool skip_current, NcmError *error);

#endif /* NCMPCPP_NC_SEL_ITEMS_ADDER_H */
