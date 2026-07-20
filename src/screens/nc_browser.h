#if !defined(NCMPCPP_NC_BROWSER_H)
#define NCMPCPP_NC_BROWSER_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_enums.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

typedef struct NativeBrowserScreen {
    NcScreen screen;
    NcBrowserEntryMenu entries;
    NcWindow window;
    NcmBuffer current_directory;
    NcmBuffer last_highlighted_directory;
    NcmBuffer title_text;
    NcmBuffer column_title_text;
    NcmBuffer filter_constraint;
    NcmBuffer search_constraint;
    NcmBuffer item_text_buffer;
    NcmBuffer path_buffer;
    NcmBuffer scratch_buffer;
    NcmBufferArray supported_extensions;
    NcmRegex filter_regex;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 lines_scrolled;
    int32 title_scroll_beginning;

    enum DisplayMode active_display_mode;

    bool mouse_list_scroll_whole_page;
    bool redraw_header;
    bool update_requested;
    bool local_browser;
    bool filter_enabled;
    bool registered;
} NativeBrowserScreen;

void native_browser_screen_init(NativeBrowserScreen *screen,
                                int32 start_x, int32 width,
                                int32 main_start_y, int32 main_height,
                                NcColor color, NcBorder border);
void native_browser_screen_destroy(NativeBrowserScreen *screen);
NcScreen *native_browser_screen_base(NativeBrowserScreen *screen);
NcBrowserEntryMenu *native_browser_screen_entries(
    NativeBrowserScreen *screen);
NcMenu *native_browser_screen_menu(NativeBrowserScreen *screen);
NcWindow *native_browser_screen_window(NativeBrowserScreen *screen);
void native_browser_screen_set_geometry(NativeBrowserScreen *screen,
                                        int32 start_x, int32 width,
                                        int32 main_start_y,
                                        int32 main_height);
void native_browser_screen_set_mouse_config(NativeBrowserScreen *screen,
                                            int32 lines_scrolled,
                                            bool scroll_whole_page);
void native_browser_screen_clear(NativeBrowserScreen *screen);
bool native_browser_screen_add_item_copy(NativeBrowserScreen *screen,
                                         NcmMpdItem *item);
void native_browser_screen_add_item_move(NativeBrowserScreen *screen,
                                         NcmMpdItem *item);
bool native_browser_screen_reload_from_mpd(NativeBrowserScreen *screen,
                                           NcmMpdClient *client,
                                           NcmError *error);
bool native_browser_screen_sort(NativeBrowserScreen *screen);
bool native_browser_screen_set_current_directory(
    NativeBrowserScreen *screen, char *directory, int32 directory_len);
NcmStringView native_browser_screen_current_directory(
    NativeBrowserScreen *screen);
NcmStringView native_browser_screen_last_highlighted_directory(
    NativeBrowserScreen *screen);
void native_browser_screen_update_title_text(NativeBrowserScreen *screen);
void native_browser_screen_update_column_title(NativeBrowserScreen *screen);
void native_browser_screen_draw_header(NativeBrowserScreen *screen);
void native_browser_screen_set_display_mode(NativeBrowserScreen *screen,
                                            enum DisplayMode mode);
bool native_browser_screen_has_supported_extension(
    NativeBrowserScreen *screen, char *extension, int32 extension_len);
bool native_browser_screen_fetch_supported_extensions(
    NativeBrowserScreen *screen, NcmMpdClient *client, NcmError *error);
bool native_browser_screen_update_requested(NativeBrowserScreen *screen);
void native_browser_screen_clear_update_request(NativeBrowserScreen *screen);
bool native_browser_screen_in_root_directory(NativeBrowserScreen *screen);
void native_browser_screen_set_local(NativeBrowserScreen *screen,
                                     bool local_browser);
bool native_browser_screen_is_local(NativeBrowserScreen *screen);
bool native_browser_screen_change_browse_mode(NativeBrowserScreen *screen,
                                              NcmMpdClient *client,
                                              NcmError *error);
NcmMpdItem *native_browser_screen_current_item(NativeBrowserScreen *screen);
bool native_browser_screen_current_song(NativeBrowserScreen *screen,
                                        NcmSong *song);
bool native_browser_screen_selected_songs(NativeBrowserScreen *screen,
                                          NcmSongArray *songs);
bool native_browser_screen_delete_items(NativeBrowserScreen *screen,
                                        NcmMpdClient *client,
                                        NcmError *error);
bool native_browser_screen_current_directory_path(
    NativeBrowserScreen *screen, NcmStringView *path);
bool native_browser_screen_current_playlist_path(
    NativeBrowserScreen *screen, NcmStringView *path);
bool native_browser_screen_rename_directory_available(
    NativeBrowserScreen *screen);
bool native_browser_screen_rename_playlist_available(
    NativeBrowserScreen *screen);
bool native_browser_screen_rename_current_directory(
    NativeBrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *error);
bool native_browser_screen_rename_current_playlist(
    NativeBrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *error);
bool native_browser_screen_locate_song(NativeBrowserScreen *screen,
                                       NcmSong *song,
                                       NcmMpdClient *client,
                                       NcmError *error);
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
bool native_browser_screen_render_item(NativeBrowserScreen *screen,
                                       NcBuffer *buffer,
                                       NcmMpdItem *item,
                                       int32 available_width,
                                       bool selected,
                                       bool highlighted);
bool native_browser_screen_item_to_string(NativeBrowserScreen *screen,
                                          NcmMpdItem *item,
                                          NcmBuffer *buffer);
void native_browser_screen_request_update(NativeBrowserScreen *screen);
bool native_browser_screen_item_is_parent(NcmMpdItem *item);

#endif /* NCMPCPP_NC_BROWSER_H */
