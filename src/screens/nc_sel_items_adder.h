#if !defined(NCMPCPP_NC_SEL_ITEMS_ADDER_H)
#define NCMPCPP_NC_SEL_ITEMS_ADDER_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NativePlaylistScreen NativePlaylistScreen;

enum NativeSelectedItemsAdderMenu {
    NATIVE_SELECTED_ITEMS_ADDER_MENU_PLAYLISTS,
    NATIVE_SELECTED_ITEMS_ADDER_MENU_POSITIONS,
};

enum NativeSelectedItemsAdderTarget {
    NATIVE_SELECTED_ITEMS_ADDER_TARGET_NONE,
    NATIVE_SELECTED_ITEMS_ADDER_TARGET_CURRENT_PLAYLIST,
    NATIVE_SELECTED_ITEMS_ADDER_TARGET_NEW_PLAYLIST,
    NATIVE_SELECTED_ITEMS_ADDER_TARGET_EXISTING_PLAYLIST,
    NATIVE_SELECTED_ITEMS_ADDER_TARGET_CANCEL,
};

enum NativeSelectedItemsAdderPosition {
    NATIVE_SELECTED_ITEMS_ADDER_POSITION_NONE,
    NATIVE_SELECTED_ITEMS_ADDER_POSITION_END,
    NATIVE_SELECTED_ITEMS_ADDER_POSITION_BEGINNING,
    NATIVE_SELECTED_ITEMS_ADDER_POSITION_AFTER_CURRENT_SONG,
    NATIVE_SELECTED_ITEMS_ADDER_POSITION_AFTER_CURRENT_ALBUM,
    NATIVE_SELECTED_ITEMS_ADDER_POSITION_AFTER_HIGHLIGHTED,
    NATIVE_SELECTED_ITEMS_ADDER_POSITION_CANCEL,
};

typedef struct NativeSelectedItemsAdderAction {
    enum NativeSelectedItemsAdderTarget target;
    enum NativeSelectedItemsAdderPosition position;
    char *playlist;
    int32 playlist_len;
    int32 playlist_cap;
} NativeSelectedItemsAdderAction;

typedef struct NativeSelectedItemsAdderScreen {
    NcScreen screen;
    NcEditorActionMenu playlist_selector;
    NcEditorActionMenu position_selector;
    NcWindow playlist_window;
    NcWindow position_window;
    NcmSongArray selected_songs;
    NcmRegex search_regex;
    NcmBuffer search_constraint;
    NativeSelectedItemsAdderAction last_action;

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

void native_selected_items_adder_action_init(
    NativeSelectedItemsAdderAction *action);
void native_selected_items_adder_action_destroy(
    NativeSelectedItemsAdderAction *action);
bool native_selected_items_adder_action_set(
    NativeSelectedItemsAdderAction *action,
    enum NativeSelectedItemsAdderTarget target,
    enum NativeSelectedItemsAdderPosition position, char *playlist,
    int32 playlist_len);

void native_selected_items_adder_screen_init(
    NativeSelectedItemsAdderScreen *screen, int64 start_x, int64 start_y,
    int64 width, int64 height, NcColor color, NcBorder border);
void native_selected_items_adder_screen_destroy(
    NativeSelectedItemsAdderScreen *screen);
NcScreen *native_selected_items_adder_screen_base(
    NativeSelectedItemsAdderScreen *screen);
NcEditorActionMenu *native_selected_items_adder_screen_playlist_menu(
    NativeSelectedItemsAdderScreen *screen);
NcEditorActionMenu *native_selected_items_adder_screen_position_menu(
    NativeSelectedItemsAdderScreen *screen);
NcMenu *native_selected_items_adder_screen_active_menu(
    NativeSelectedItemsAdderScreen *screen);
NcWindow *native_selected_items_adder_screen_active_window(
    NativeSelectedItemsAdderScreen *screen);
bool native_selected_items_adder_screen_open(
    NativeSelectedItemsAdderScreen *screen, NcmSongArray *songs,
    NativePlaylistScreen *playlist, NcmMpdClient *client, NcmError *error);
bool native_selected_items_adder_screen_set_selected_songs(
    NativeSelectedItemsAdderScreen *screen, NcmSongArray *songs);
bool native_selected_items_adder_screen_selected_songs(
    NativeSelectedItemsAdderScreen *screen, NcmSongArray *songs);
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
bool native_selected_items_adder_screen_apply_search(
    NativeSelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, NcmError *error);
void native_selected_items_adder_screen_clear_search(
    NativeSelectedItemsAdderScreen *screen);
bool native_selected_items_adder_screen_search(
    NativeSelectedItemsAdderScreen *screen, char *pattern,
    int32 pattern_len, uint32 regex_flags, bool forward, bool wrap,
    bool skip_current, NcmError *error);
NativeSelectedItemsAdderAction *native_selected_items_adder_screen_last_action(
    NativeSelectedItemsAdderScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SEL_ITEMS_ADDER_H */
