#if !defined(NCMPCPP_NC_BROWSER_H)
#define NCMPCPP_NC_BROWSER_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NativeBrowserScreen {
    NcScreen screen;
    NcBrowserEntryMenu entries;
    NcWindow window;
    NcmBuffer current_directory;
    NcmBuffer search_constraint;
    NcmRegex filter_regex;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;
    int64 lines_scrolled;

    bool mouse_list_scroll_whole_page;
    bool redraw_header;
    bool local_browser;
    bool filter_enabled;
    bool registered;
} NativeBrowserScreen;

void native_browser_screen_init(NativeBrowserScreen *screen,
                                int64 start_x, int64 width,
                                int64 main_start_y, int64 main_height,
                                NcColor color, NcBorder border);
void native_browser_screen_destroy(NativeBrowserScreen *screen);
NcScreen *native_browser_screen_base(NativeBrowserScreen *screen);
NcBrowserEntryMenu *native_browser_screen_entries(
    NativeBrowserScreen *screen);
NcMenu *native_browser_screen_menu(NativeBrowserScreen *screen);
NcWindow *native_browser_screen_window(NativeBrowserScreen *screen);
void native_browser_screen_set_geometry(NativeBrowserScreen *screen,
                                        int64 start_x, int64 width,
                                        int64 main_start_y,
                                        int64 main_height);
void native_browser_screen_set_mouse_config(NativeBrowserScreen *screen,
                                            int64 lines_scrolled,
                                            bool scroll_whole_page);
void native_browser_screen_clear(NativeBrowserScreen *screen);
bool native_browser_screen_add_item_copy(NativeBrowserScreen *screen,
                                         NcmMpdItem *item);
void native_browser_screen_add_item_move(NativeBrowserScreen *screen,
                                         NcmMpdItem *item);
bool native_browser_screen_load_items(NativeBrowserScreen *screen,
                                      NcmMpdItemArray *items);
bool native_browser_screen_reload_from_mpd(NativeBrowserScreen *screen,
                                           NcmMpdClient *client,
                                           NcmError *error);
bool native_browser_screen_set_current_directory(
    NativeBrowserScreen *screen, char *directory, int32 directory_len);
NcmStringView native_browser_screen_current_directory(
    NativeBrowserScreen *screen);
bool native_browser_screen_in_root_directory(NativeBrowserScreen *screen);
void native_browser_screen_set_local(NativeBrowserScreen *screen,
                                     bool local_browser);
bool native_browser_screen_is_local(NativeBrowserScreen *screen);
NcmMpdItem *native_browser_screen_current_item(NativeBrowserScreen *screen);
bool native_browser_screen_current_song(NativeBrowserScreen *screen,
                                        NcmSong *song);
bool native_browser_screen_selected_songs(NativeBrowserScreen *screen,
                                          NcmSongArray *songs);
bool native_browser_screen_enter_directory(NativeBrowserScreen *screen);
bool native_browser_screen_go_to_parent(NativeBrowserScreen *screen);
bool native_browser_screen_apply_filter(NativeBrowserScreen *screen,
                                        char *pattern, int32 pattern_len,
                                        NcmError *error);
void native_browser_screen_clear_filter(NativeBrowserScreen *screen);
bool native_browser_screen_search(NativeBrowserScreen *screen,
                                  char *pattern, int32 pattern_len,
                                  bool forward, bool wrap,
                                  bool skip_current, NcmError *error);
bool native_browser_screen_item_is_parent(NcmMpdItem *item);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_BROWSER_H */
