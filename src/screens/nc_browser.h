#if !defined(NCMPCPP_NC_BROWSER_H)
#define NCMPCPP_NC_BROWSER_H

#include "cbase.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_enums.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

typedef struct BrowserScreen {
    NcScreen screen;
    NcBrowserEntryMenu entries;
    NcWindow window;
    StrBuilder current_directory;
    StrBuilder last_highlighted_directory;
    StrBuilder title_text;
    StrBuilder column_title_text;
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    StrBuilder item_text_buffer;
    StrBuilder path_buffer;
    StrBuilder scratch_buffer;
    StrBuilderArray supported_extensions;
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
} BrowserScreen;

void browser_screen_init(BrowserScreen *screen,
                                int32 start_x, int32 width,
                                int32 main_start_y, int32 main_height,
                                NcColor color, NcBorder border);
void browser_screen_destroy(BrowserScreen *screen);
NcScreen *browser_screen_base(BrowserScreen *screen);
NcBrowserEntryMenu *browser_screen_entries(
    BrowserScreen *screen);
NcMenu *browser_screen_menu(BrowserScreen *screen);
NcWindow *browser_screen_window(BrowserScreen *screen);
void browser_screen_set_geometry(BrowserScreen *screen,
                                        int32 start_x, int32 width,
                                        int32 main_start_y,
                                        int32 main_height);
void browser_screen_set_mouse_config(BrowserScreen *screen,
                                            int32 lines_scrolled,
                                            bool scroll_whole_page);
void browser_screen_clear(BrowserScreen *screen);
bool browser_screen_add_item_copy(BrowserScreen *screen,
                                         NcmMpdItem *item);
void browser_screen_add_item_move(BrowserScreen *screen,
                                         NcmMpdItem *item);
bool browser_screen_reload_from_mpd(BrowserScreen *screen,
                                           NcmMpdClient *client,
                                           NcmError *error);
bool browser_screen_sort(BrowserScreen *screen);
bool browser_screen_set_current_directory(
    BrowserScreen *screen, char *directory, int32 directory_len);
NcmStringView browser_screen_current_directory(
    BrowserScreen *screen);
NcmStringView browser_screen_last_highlighted_directory(
    BrowserScreen *screen);
void browser_screen_update_title_text(BrowserScreen *screen);
void browser_screen_update_column_title(BrowserScreen *screen);
void browser_screen_draw_header(BrowserScreen *screen);
void browser_screen_set_display_mode(BrowserScreen *screen,
                                            enum DisplayMode mode);
bool browser_screen_has_supported_extension(
    BrowserScreen *screen, char *extension, int32 extension_len);
bool browser_screen_fetch_supported_extensions(
    BrowserScreen *screen, NcmMpdClient *client, NcmError *error);
bool browser_screen_update_requested(BrowserScreen *screen);
void browser_screen_clear_update_request(BrowserScreen *screen);
bool browser_screen_in_root_directory(BrowserScreen *screen);
void browser_screen_set_local(BrowserScreen *screen,
                                     bool local_browser);
bool browser_screen_is_local(BrowserScreen *screen);
bool browser_screen_change_browse_mode(BrowserScreen *screen,
                                              NcmMpdClient *client,
                                              NcmError *error);
NcmMpdItem *browser_screen_current_item(BrowserScreen *screen);
bool browser_screen_current_song(BrowserScreen *screen,
                                        NcmSong *song);
bool browser_screen_selected_songs(BrowserScreen *screen,
                                          NcmSongArray *songs);
bool browser_screen_delete_items(BrowserScreen *screen,
                                        NcmMpdClient *client,
                                        NcmError *error);
bool browser_screen_current_directory_path(
    BrowserScreen *screen, NcmStringView *path);
bool browser_screen_current_playlist_path(
    BrowserScreen *screen, NcmStringView *path);
bool browser_screen_rename_directory_available(
    BrowserScreen *screen);
bool browser_screen_rename_playlist_available(
    BrowserScreen *screen);
bool browser_screen_rename_current_directory(
    BrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *error);
bool browser_screen_rename_current_playlist(
    BrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *error);
bool browser_screen_locate_song(BrowserScreen *screen,
                                       NcmSong *song,
                                       NcmMpdClient *client,
                                       NcmError *error);
bool browser_screen_enter_directory(BrowserScreen *screen);
bool browser_screen_go_to_parent(BrowserScreen *screen);
bool browser_screen_apply_filter(BrowserScreen *screen,
                                        char *pattern, int32 pattern_len,
                                        NcmError *error);
void browser_screen_clear_filter(BrowserScreen *screen);
bool browser_screen_search(BrowserScreen *screen,
                                  char *pattern, int32 pattern_len,
                                  bool forward, bool wrap,
                                  bool skip_current, NcmError *error);
bool browser_screen_render_item(BrowserScreen *screen,
                                       NcBuffer *buffer,
                                       NcmMpdItem *item,
                                       int32 available_width,
                                       bool selected,
                                       bool highlighted);
bool browser_screen_item_to_string(BrowserScreen *screen,
                                          NcmMpdItem *item,
                                          StrBuilder *buffer);
void browser_screen_request_update(BrowserScreen *screen);
bool browser_screen_item_is_parent(NcmMpdItem *item);

#endif /* NCMPCPP_NC_BROWSER_H */
