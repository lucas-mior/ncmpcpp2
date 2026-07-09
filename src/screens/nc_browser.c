#include "screens/nc_browser.h"

#include "c/ncm_base.h"
#include "c/ncm_fs.h"
#include "c/ncm_string.h"

static NativeBrowserScreen *native_browser_from_screen(NcScreen *screen);
static NcWindow *native_browser_active_window(NcScreen *screen);
static void native_browser_refresh(NcScreen *screen);
static void native_browser_refresh_window(NcScreen *screen);
static void native_browser_scroll(NcScreen *screen, enum NcScroll where);
static void native_browser_switch_to(NcScreen *screen);
static void native_browser_resize(NcScreen *screen);
static char *native_browser_title(NcScreen *screen);
static void native_browser_update(NcScreen *screen);
static void native_browser_mouse_button_pressed(NcScreen *screen,
                                                MEVENT event);
static bool native_browser_is_lockable(NcScreen *screen);
static bool native_browser_is_mergable(NcScreen *screen);
static void native_browser_destroy_callback(NcScreen *screen);
static bool native_browser_filter_item(NcMenu *menu, void *item,
                                       void *user);
static bool native_browser_item_matches(NcmMpdItem *item, NcmRegex *regex);
static bool native_browser_item_label(NcmMpdItem *item, NcmStringView *view);
static bool native_browser_item_is_song(NcmMpdItem *item);
static bool native_browser_copy_selected_song(NativeBrowserScreen *screen,
                                              NcmSongArray *songs,
                                              int64 pos);

static NcScreenCallbacks native_browser_callbacks = {
    .active_window = native_browser_active_window,
    .refresh = native_browser_refresh,
    .refresh_window = native_browser_refresh_window,
    .scroll = native_browser_scroll,
    .switch_to = native_browser_switch_to,
    .resize = native_browser_resize,
    .title = native_browser_title,
    .update = native_browser_update,
    .mouse_button_pressed = native_browser_mouse_button_pressed,
    .is_lockable = native_browser_is_lockable,
    .is_mergable = native_browser_is_mergable,
    .destroy = native_browser_destroy_callback,
};

void
native_browser_screen_init(NativeBrowserScreen *screen,
                           int64 start_x, int64 width,
                           int64 main_start_y, int64 main_height,
                           NcColor color, NcBorder border) {
    nc_browser_entry_menu_init(&screen->entries);
    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    ncm_buffer_init(&screen->current_directory);
    ncm_buffer_init(&screen->filter_constraint);
    ncm_buffer_init(&screen->search_constraint);
    ncm_regex_init(&screen->filter_regex);

    screen->bridge = (NativeBrowserBridge){0};
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->lines_scrolled = 1;
    screen->mouse_list_scroll_whole_page = false;
    screen->redraw_header = true;
    screen->local_browser = false;
    screen->filter_enabled = false;
    screen->registered = false;

    nc_screen_init(&screen->screen, native_browser_callbacks, screen,
                   NC_SCREEN_TYPE_BROWSER);
    return;
}

void
native_browser_screen_destroy(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_buffer_destroy(&screen->filter_constraint);
    ncm_buffer_destroy(&screen->search_constraint);
    ncm_buffer_destroy(&screen->current_directory);
    nc_window_destroy(&screen->window);
    nc_browser_entry_menu_destroy(&screen->entries);
    return;
}

NcScreen *
native_browser_screen_base(NativeBrowserScreen *screen) {
    return &screen->screen;
}

NcBrowserEntryMenu *
native_browser_screen_entries(NativeBrowserScreen *screen) {
    return &screen->entries;
}

NcMenu *
native_browser_screen_menu(NativeBrowserScreen *screen) {
    return nc_browser_entry_menu_base(&screen->entries);
}

NcWindow *
native_browser_screen_window(NativeBrowserScreen *screen) {
    return &screen->window;
}

void
native_browser_screen_set_geometry(NativeBrowserScreen *screen,
                                   int64 start_x, int64 width,
                                   int64 main_start_y,
                                   int64 main_height) {
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    nc_window_move_to(&screen->window, start_x, main_start_y);
    nc_window_resize(&screen->window, width, main_height);
    return;
}

void
native_browser_screen_set_mouse_config(NativeBrowserScreen *screen,
                                       int64 lines_scrolled,
                                       bool scroll_whole_page) {
    screen->lines_scrolled = lines_scrolled;
    screen->mouse_list_scroll_whole_page = scroll_whole_page;
    return;
}

void
native_browser_screen_clear(NativeBrowserScreen *screen) {
    nc_menu_clear_items(native_browser_screen_menu(screen));
    screen->redraw_header = true;
    return;
}

bool
native_browser_screen_add_item_copy(NativeBrowserScreen *screen,
                                    NcmMpdItem *item) {
    if (screen == NULL || item == NULL) {
        return false;
    }
    nc_browser_entry_menu_add(native_browser_screen_entries(screen), item);
    return true;
}

void
native_browser_screen_add_item_move(NativeBrowserScreen *screen,
                                    NcmMpdItem *item) {
    NcmMpdItem copy;

    if (screen == NULL || item == NULL) {
        return;
    }
    ncm_mpd_item_init(&copy);
    ncm_mpd_item_move(&copy, item);
    nc_browser_entry_menu_add(native_browser_screen_entries(screen), &copy);
    ncm_mpd_item_destroy(&copy);
    return;
}

bool
native_browser_screen_load_items(NativeBrowserScreen *screen,
                                 NcmMpdItemArray *items) {
    if (screen == NULL || items == NULL) {
        return false;
    }
    native_browser_screen_clear(screen);
    for (int32 i = 0; i < items->len; i += 1) {
        if (!native_browser_screen_add_item_copy(screen, &items->items[i])) {
            return false;
        }
    }
    return true;
}

bool
native_browser_screen_reload_from_mpd(NativeBrowserScreen *screen,
                                      NcmMpdClient *client,
                                      NcmError *error) {
    NcmMpdItemArray items;
    bool result;

    if (screen == NULL || client == NULL) {
        return false;
    }

    ncm_mpd_item_array_init(&items);
    result = ncm_mpd_client_get_directory_entries(
        client, screen->current_directory.data, &items, error);
    if (result) {
        result = native_browser_screen_load_items(screen, &items);
    }
    ncm_mpd_item_array_destroy(&items);
    return result;
}

bool
native_browser_screen_set_current_directory(NativeBrowserScreen *screen,
                                            char *directory,
                                            int32 directory_len) {
    if (screen == NULL) {
        return false;
    }
    return ncm_buffer_set(&screen->current_directory, directory,
                          directory_len);
}

NcmStringView
native_browser_screen_current_directory(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(screen->current_directory.data,
                                screen->current_directory.len);
}

bool
native_browser_screen_in_root_directory(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return true;
    }
    return screen->current_directory.len <= 0;
}

void
native_browser_screen_set_local(NativeBrowserScreen *screen,
                                bool local_browser) {
    if (screen == NULL) {
        return;
    }
    screen->local_browser = local_browser;
    return;
}

bool
native_browser_screen_is_local(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->local_browser;
}

NcmMpdItem *
native_browser_screen_current_item(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_browser_entry_menu_current(&screen->entries);
}

bool
native_browser_screen_current_song(NativeBrowserScreen *screen,
                                   NcmSong *song) {
    NcmMpdItem *item;

    if (song == NULL) {
        return false;
    }
    item = native_browser_screen_current_item(screen);
    if (!native_browser_item_is_song(item)) {
        return false;
    }
    return ncm_song_copy(song, ncm_mpd_item_song(item));
}

bool
native_browser_screen_selected_songs(NativeBrowserScreen *screen,
                                     NcmSongArray *songs) {
    NcMenu *menu;
    bool any_selected;

    if (screen == NULL || songs == NULL) {
        return false;
    }

    menu = native_browser_screen_menu(screen);
    any_selected = nc_menu_has_selected(menu);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!native_browser_copy_selected_song(screen, songs, i)) {
            return false;
        }
    }
    return true;
}

bool
native_browser_screen_enter_directory(NativeBrowserScreen *screen) {
    NcmMpdItem *item;
    NcmDirectory *directory;
    NcmPlaylist *playlist;

    item = native_browser_screen_current_item(screen);
    if (item == NULL) {
        return false;
    }
    if (ncm_mpd_item_kind(item) == NCM_MPD_ITEM_DIRECTORY) {
        directory = ncm_mpd_item_directory(item);
        return native_browser_screen_set_current_directory(
            screen, directory->path, directory->path_len);
    }
    if (ncm_mpd_item_kind(item) == NCM_MPD_ITEM_PLAYLIST) {
        playlist = ncm_mpd_item_playlist(item);
        return native_browser_screen_set_current_directory(
            screen, playlist->path, playlist->path_len);
    }
    return false;
}

bool
native_browser_screen_go_to_parent(NativeBrowserScreen *screen) {
    int32 parent_len;

    if (screen == NULL) {
        return false;
    }
    if (screen->current_directory.len <= 0) {
        return false;
    }
    parent_len = ncm_string_parent_directory_len(
        screen->current_directory.data, screen->current_directory.len);
    screen->current_directory.len = parent_len;
    if (screen->current_directory.data != NULL) {
        screen->current_directory.data[parent_len] = '\0';
    }
    return true;
}

bool
native_browser_screen_apply_filter(NativeBrowserScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   NcmError *error) {
    NcMenuDisplayCallbacks callbacks = {0};

    if (screen == NULL) {
        return false;
    }
    if (pattern_len <= 0) {
        native_browser_screen_clear_filter(screen);
        return true;
    }
    if (!ncm_regex_compile(&screen->filter_regex, pattern, pattern_len,
                           NCM_REGEX_LITERAL_CASE_INSENSITIVE, error)) {
        return false;
    }
    if (!ncm_buffer_set(&screen->filter_constraint, pattern, pattern_len)) {
        return false;
    }
    callbacks.filter = native_browser_filter_item;
    callbacks.user = screen;
    nc_menu_set_display_callbacks(native_browser_screen_menu(screen),
                                  callbacks);
    screen->filter_enabled = true;
    nc_menu_apply_filter(native_browser_screen_menu(screen));
    return true;
}

void
native_browser_screen_clear_filter(NativeBrowserScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_regex_init(&screen->filter_regex);
    ncm_buffer_clear(&screen->filter_constraint);
    screen->filter_enabled = false;
    nc_menu_set_display_callbacks(native_browser_screen_menu(screen),
                                  callbacks);
    nc_menu_show_all_items(native_browser_screen_menu(screen));
    return;
}

bool
native_browser_screen_search(NativeBrowserScreen *screen,
                             char *pattern, int32 pattern_len,
                             bool forward, bool wrap,
                             bool skip_current, NcmError *error) {
    NcmRegex regex;
    NcMenu *menu;
    int64 count;
    int64 start;
    bool result = false;

    if (screen == NULL || pattern == NULL || pattern_len <= 0) {
        return false;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len,
                           NCM_REGEX_LITERAL_CASE_INSENSITIVE, error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    menu = native_browser_screen_menu(screen);
    count = nc_menu_item_count(menu);
    start = nc_menu_highlight(menu);
    if (skip_current) {
        if (forward) {
            start += 1;
        } else {
            start -= 1;
        }
    }

    for (int64 i = 0; i < count; i += 1) {
        int64 pos;
        if (forward) {
            pos = start + i;
        } else {
            pos = start - i;
        }
        if (wrap) {
            while (pos < 0) {
                pos += count;
            }
            while (pos >= count) {
                pos -= count;
            }
        }
        if (pos < 0 || pos >= count) {
            continue;
        }
        if (native_browser_item_matches(nc_menu_active_item_at(menu, pos),
                                        &regex)) {
            result = nc_menu_goto_selectable(menu, pos);
            break;
        }
    }

    ncm_regex_destroy(&regex);
    return result;
}

bool
native_browser_screen_item_is_parent(NcmMpdItem *item) {
    NcmStringView view;

    if (item == NULL) {
        return false;
    }
    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_DIRECTORY) {
        return false;
    }
    if (!ncm_directory_path_view(ncm_mpd_item_directory(item), &view)) {
        return false;
    }
    return ncm_string_equal(view.data, view.len, (char *)"..", 2);
}

void
native_browser_screen_request_update(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    if (screen->bridge.request_update != NULL) {
        screen->bridge.request_update(screen->bridge.user);
        return;
    }
    nc_screen_request_update(&screen->screen);
    return;
}

void
native_browser_screen_set_bridge(NativeBrowserScreen *screen,
                                 NativeBrowserBridge bridge) {
    if (screen == NULL) {
        return;
    }
    screen->bridge = bridge;
    return;
}

static NativeBrowserScreen *
native_browser_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
native_browser_active_window(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    if (browser->bridge.active_window != NULL) {
        return browser->bridge.active_window(browser->bridge.user);
    }
    return &browser->window;
}

static void
native_browser_refresh(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    if (browser->bridge.refresh != NULL) {
        browser->bridge.refresh(browser->bridge.user);
        return;
    }
    nc_menu_prepare_refresh(native_browser_screen_menu(browser),
                            browser->main_height, NULL, NULL);
    return;
}

static void
native_browser_refresh_window(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    if (browser->bridge.refresh_window != NULL) {
        browser->bridge.refresh_window(browser->bridge.user);
        return;
    }
    native_browser_refresh(screen);
    return;
}

static void
native_browser_scroll(NcScreen *screen, enum NcScroll where) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    if (browser->bridge.scroll != NULL) {
        browser->bridge.scroll(browser->bridge.user, where);
        return;
    }
    nc_menu_scroll_selectable(native_browser_screen_menu(browser),
                              browser->main_height, where);
    return;
}

static void
native_browser_switch_to(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    if (browser->bridge.switch_to != NULL) {
        browser->bridge.switch_to(browser->bridge.user);
    }
    return;
}

static void
native_browser_resize(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    native_browser_screen_set_geometry(browser, browser->start_x,
                                       browser->width,
                                       browser->main_start_y,
                                       browser->main_height);
    if (browser->bridge.resize != NULL) {
        browser->bridge.resize(browser->bridge.user);
    }
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
native_browser_title(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    if (browser->bridge.title != NULL) {
        return browser->bridge.title(browser->bridge.user);
    }
    return (char *)"Browser";
}

static void
native_browser_update(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    if (browser->bridge.update != NULL) {
        browser->bridge.update(browser->bridge.user);
        nc_screen_clear_update_request(screen);
        return;
    }
    nc_screen_clear_update_request(screen);
    return;
}

static void
native_browser_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    if (browser->bridge.mouse_button_pressed != NULL) {
        browser->bridge.mouse_button_pressed(browser->bridge.user, event);
    }
    return;
}

static bool
native_browser_is_lockable(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
native_browser_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
native_browser_destroy_callback(NcScreen *screen) {
    native_browser_screen_destroy(native_browser_from_screen(screen));
    return;
}

static bool
native_browser_filter_item(NcMenu *menu, void *item, void *user) {
    NativeBrowserScreen *screen;

    (void)menu;
    screen = user;
    return native_browser_item_matches(item, &screen->filter_regex);
}

static bool
native_browser_item_matches(NcmMpdItem *item, NcmRegex *regex) {
    NcmStringView view;

    if (!native_browser_item_label(item, &view)) {
        return false;
    }
    return ncm_regex_search(regex, view.data, view.len);
}

static bool
native_browser_item_label(NcmMpdItem *item, NcmStringView *view) {
    NcmDirectory *directory;
    NcmPlaylist *playlist;
    NcmSong *song;

    if (item == NULL || view == NULL) {
        return false;
    }

    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_SONG:
        song = ncm_mpd_item_song(item);
        return ncm_song_uri_view(song, 0, view);
    case NCM_MPD_ITEM_DIRECTORY:
        directory = ncm_mpd_item_directory(item);
        return ncm_directory_path_view(directory, view);
    case NCM_MPD_ITEM_PLAYLIST:
        playlist = ncm_mpd_item_playlist(item);
        return ncm_playlist_path_view(playlist, view);
    default:
        break;
    }
    return false;
}

static bool
native_browser_item_is_song(NcmMpdItem *item) {
    if (item == NULL) {
        return false;
    }
    return ncm_mpd_item_kind(item) == NCM_MPD_ITEM_SONG;
}

static bool
native_browser_copy_selected_song(NativeBrowserScreen *screen,
                                  NcmSongArray *songs, int64 pos) {
    NcmMpdItem *item;
    NcMenu *menu;

    menu = native_browser_screen_menu(screen);
    item = nc_menu_active_item_at(menu, pos);
    if (!native_browser_item_is_song(item)) {
        return true;
    }
    return ncm_song_array_append_copy(songs, ncm_mpd_item_song(item));
}

