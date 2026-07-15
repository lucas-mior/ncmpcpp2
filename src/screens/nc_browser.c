#include "screens/nc_browser.h"

#include "c/ncm_base.h"
#include "c/ncm_display.h"
#include "c/ncm_fs.h"
#include "c/ncm_string.h"
#include "c/ncm_utf8.h"
#include "cbase/base_macros.h"
#include "curses/nc_cyclic_buffer.h"
#include "global.h"
#include "settings.h"
#include "screens/screen_switcher.h"
#include "title.h"
#include "ui_state.h"

#include <limits.h>

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
static NcMenuDisplayCallbacks native_browser_display_callbacks(
    NativeBrowserScreen *screen);
static NcMenuActionCallbacks native_browser_action_callbacks(
    NativeBrowserScreen *screen);
static void native_browser_install_menu_callbacks(
    NativeBrowserScreen *screen);
static void native_browser_apply_menu_config(NativeBrowserScreen *screen);
static void native_browser_draw_item(NcMenu *menu, NcWindow *window,
                                     void *item, int64 pos, void *user);
static void native_browser_print_buffer(NcWindow *window, NcBuffer *buffer);
static int32 native_browser_i32_width(int64 width);
static void native_browser_sync_window_title(NativeBrowserScreen *screen);
static void native_browser_draw_header(NativeBrowserScreen *screen);
static void native_browser_mouse_scroll(NativeBrowserScreen *screen,
                                        enum NcScroll where);
static bool native_browser_filter_item(NcMenu *menu, void *item,
                                       void *user);
static void native_browser_activate_item(NcMenu *menu, void *item,
                                         int64 pos, void *user);
static void native_browser_set_item_selected(void *item, bool selected,
                                             void *user);
static bool native_browser_enter_item(NativeBrowserScreen *screen,
                                      NcmMpdItem *item);
static bool native_browser_item_matches(NcmMpdItem *item, NcmRegex *regex);
static bool native_browser_item_label(NcmMpdItem *item, NcmStringView *view);
static bool native_browser_item_is_song(NcmMpdItem *item);
static bool native_browser_copy_selected_song(NativeBrowserScreen *screen,
                                              NcmSongArray *songs,
                                              int64 pos);
static bool native_browser_string_views_equal(NcmStringView left,
                                              NcmStringView right);

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
    ncm_buffer_init(&screen->last_highlighted_directory);
    ncm_buffer_init(&screen->title_text);
    ncm_buffer_init(&screen->column_title_text);
    ncm_buffer_init(&screen->filter_constraint);
    ncm_buffer_init(&screen->search_constraint);
    ncm_buffer_init(&screen->item_text_buffer);
    ncm_buffer_init(&screen->path_buffer);
    ncm_buffer_init(&screen->scratch_buffer);
    ncm_buffer_array_init(&screen->supported_extensions);
    ncm_regex_init(&screen->filter_regex);

    screen->bridge = (NativeBrowserBridge){0};
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->lines_scrolled = 1;
    screen->title_scroll_beginning = 0;
    screen->active_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    screen->mouse_list_scroll_whole_page = false;
    screen->redraw_header = true;
    screen->update_requested = true;
    screen->local_browser = false;
    screen->filter_enabled = false;
    screen->registered = false;

    native_browser_install_menu_callbacks(screen);
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
    ncm_buffer_array_destroy(&screen->supported_extensions);
    ncm_buffer_destroy(&screen->scratch_buffer);
    ncm_buffer_destroy(&screen->path_buffer);
    ncm_buffer_destroy(&screen->item_text_buffer);
    ncm_buffer_destroy(&screen->filter_constraint);
    ncm_buffer_destroy(&screen->search_constraint);
    ncm_buffer_destroy(&screen->column_title_text);
    ncm_buffer_destroy(&screen->title_text);
    ncm_buffer_destroy(&screen->last_highlighted_directory);
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
    NcmStringView current;
    NcmStringView replacement;

    if (screen == NULL) {
        return false;
    }
    if (directory_len < 0) {
        return false;
    }
    if ((directory == NULL) && (directory_len > 0)) {
        return false;
    }

    current = ncm_string_view_make(screen->current_directory.data,
                                   screen->current_directory.len);
    replacement = ncm_string_view_make(directory, directory_len);
    if (!native_browser_string_views_equal(current, replacement)) {
        if (!ncm_buffer_set(&screen->last_highlighted_directory,
                            current.data, current.len)) {
            return false;
        }
        screen->title_scroll_beginning = 0;
        screen->redraw_header = true;
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
native_browser_screen_set_last_highlighted_directory(
    NativeBrowserScreen *screen, char *directory, int32 directory_len) {
    if (screen == NULL) {
        return false;
    }
    return ncm_buffer_set(&screen->last_highlighted_directory, directory,
                          directory_len);
}

NcmStringView
native_browser_screen_last_highlighted_directory(
    NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(screen->last_highlighted_directory.data,
                                screen->last_highlighted_directory.len);
}

void
native_browser_screen_clear_last_highlighted_directory(
    NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_buffer_clear(&screen->last_highlighted_directory);
    return;
}

bool
native_browser_screen_set_title_text(NativeBrowserScreen *screen,
                                     char *title, int32 title_len) {
    if (screen == NULL) {
        return false;
    }
    return ncm_buffer_set(&screen->title_text, title, title_len);
}

NcmStringView
native_browser_screen_title_text(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(screen->title_text.data,
                                screen->title_text.len);
}

bool
native_browser_screen_set_column_title_text(NativeBrowserScreen *screen,
                                            char *title,
                                            int32 title_len) {
    if (screen == NULL) {
        return false;
    }
    return ncm_buffer_set(&screen->column_title_text, title, title_len);
}

NcmStringView
native_browser_screen_column_title_text(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(screen->column_title_text.data,
                                screen->column_title_text.len);
}

void
native_browser_screen_set_display_mode(NativeBrowserScreen *screen,
                                        enum DisplayMode mode) {
    if (screen == NULL) {
        return;
    }
    if ((mode != NCM_DISPLAY_MODE_CLASSIC)
        && (mode != NCM_DISPLAY_MODE_COLUMNS)) {
        return;
    }
    screen->active_display_mode = mode;
    screen->redraw_header = true;
    return;
}

enum DisplayMode
native_browser_screen_display_mode(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return NCM_DISPLAY_MODE_CLASSIC;
    }
    return screen->active_display_mode;
}

void
native_browser_screen_clear_supported_extensions(
    NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_buffer_array_clear(&screen->supported_extensions);
    return;
}

bool
native_browser_screen_add_supported_extension(NativeBrowserScreen *screen,
                                              char *extension,
                                              int32 extension_len) {
    NcmBuffer buffer;
    bool result;

    if (screen == NULL) {
        return false;
    }

    ncm_buffer_init(&buffer);
    result = ncm_buffer_set(&buffer, extension, extension_len)
             && ncm_buffer_array_append_copy(
                 &screen->supported_extensions, &buffer);
    ncm_buffer_destroy(&buffer);
    return result;
}

bool
native_browser_screen_has_supported_extension(NativeBrowserScreen *screen,
                                              char *extension,
                                              int32 extension_len) {
    NcmBufferArray *extensions;

    if (screen == NULL) {
        return false;
    }
    extensions = &screen->supported_extensions;
    for (int32 i = 0; i < extensions->len; i += 1) {
        NcmBuffer *item;

        item = &extensions->items[i];
        if (ncm_string_equal(item->data, item->len, extension,
                             extension_len)) {
            return true;
        }
    }
    return false;
}

NcmBufferArray *
native_browser_screen_supported_extensions(
    NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->supported_extensions;
}

void
native_browser_screen_clear_temp_buffers(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_buffer_clear(&screen->item_text_buffer);
    ncm_buffer_clear(&screen->path_buffer);
    ncm_buffer_clear(&screen->scratch_buffer);
    return;
}

bool
native_browser_screen_update_requested(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->update_requested;
}

void
native_browser_screen_clear_update_request(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->update_requested = false;
    nc_screen_clear_update_request(&screen->screen);
    return;
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
    return native_browser_enter_item(screen,
                                     native_browser_screen_current_item(
                                         screen));
}

bool
native_browser_screen_activate_current(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return nc_menu_activate_current(native_browser_screen_menu(screen));
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
    if (!ncm_buffer_set(&screen->last_highlighted_directory,
                        screen->current_directory.data,
                        screen->current_directory.len)) {
        return false;
    }
    parent_len = ncm_string_parent_directory_len(
        screen->current_directory.data, screen->current_directory.len);
    screen->current_directory.len = parent_len;
    if (screen->current_directory.data != NULL) {
        screen->current_directory.data[parent_len] = '\0';
    }
    screen->title_scroll_beginning = 0;
    screen->redraw_header = true;
    return true;
}

bool
native_browser_screen_apply_filter(NativeBrowserScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   NcmError *error) {
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
    screen->filter_enabled = true;
    native_browser_install_menu_callbacks(screen);
    nc_menu_apply_filter(native_browser_screen_menu(screen));
    return true;
}

void
native_browser_screen_clear_filter(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_regex_init(&screen->filter_regex);
    ncm_buffer_clear(&screen->filter_constraint);
    screen->filter_enabled = false;
    native_browser_install_menu_callbacks(screen);
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
    if (!ncm_buffer_set(&screen->search_constraint, pattern, pattern_len)) {
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
    screen->update_requested = true;
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
    return &browser->window;
}

static void
native_browser_refresh(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    native_browser_sync_window_title(browser);
    nc_menu_refresh(native_browser_screen_menu(browser), &browser->window,
                    browser->width, browser->main_height);
    return;
}

static void
native_browser_refresh_window(NcScreen *screen) {
    native_browser_refresh(screen);
    return;
}

static void
native_browser_scroll(NcScreen *screen, enum NcScroll where) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    nc_menu_scroll_selectable(native_browser_screen_menu(browser),
                              browser->main_height, where);
    return;
}

static void
native_browser_switch_to(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    (void)nc_screen_switcher_finish_switch(screen);
    browser->redraw_header = true;
    native_browser_draw_header(browser);
    return;
}

static void
native_browser_resize(NcScreen *screen) {
    NativeBrowserScreen *browser;
    int64 x;
    int64 width;

    browser = native_browser_from_screen(screen);
    x = browser->start_x;
    width = browser->width;
    nc_screen_switcher_get_resize_params(screen, &x, &width, true);
    native_browser_screen_set_geometry(browser, x, width,
                                       ui_state_main_start_y(),
                                       ui_state_main_height());
    native_browser_sync_window_title(browser);
    browser->redraw_header = true;
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
native_browser_title(NcScreen *screen) {
    NativeBrowserScreen *browser;
    NcmBuffer scroll_buffer;
    NcmStringView directory;
    int64 scroll_beginning;
    int32 scroll_width;
    int32 screen_width;
    char separator[] = " ** ";

    browser = native_browser_from_screen(screen);
    ncm_buffer_clear(&browser->title_text);
    ncm_buffer_append(&browser->title_text, STRLIT_ARGS("Browse: "));

    directory = native_browser_screen_current_directory(browser);
    if (directory.len <= 0) {
        directory = ncm_string_view_make((char *)"/", 1);
    }

    screen_width = (int32)ui_state_screen_width();
    if (screen_width <= 0) {
        screen_width = native_browser_i32_width(browser->width);
    }
    scroll_width = screen_width - ncm_utf8_width(browser->title_text.data,
                                                 browser->title_text.len);
    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        scroll_width -= 2;
    } else {
        scroll_width -= global_volume_state_len();
    }
    if (scroll_width < 0) {
        scroll_width = 0;
    }

    ncm_buffer_init(&scroll_buffer);
    scroll_beginning = browser->title_scroll_beginning;
    nc_cyclic_text_write(&scroll_buffer, directory.data, directory.len,
                         &scroll_beginning, scroll_width, separator,
                         (int32)SIZEOF(separator) - 1,
                         Config.header_text_scrolling);
    ncm_buffer_append(&browser->title_text, scroll_buffer.data,
                      scroll_buffer.len);
    browser->title_scroll_beginning = scroll_beginning;
    ncm_buffer_destroy(&scroll_buffer);
    return browser->title_text.data;
}

static void
native_browser_update(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    native_browser_screen_clear_update_request(browser);
    if (browser->redraw_header) {
        native_browser_draw_header(browser);
    }
    return;
}

static void
native_browser_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NativeBrowserScreen *browser;
    NcMenu *menu;
    NcmMpdItem *item;
    NcWindow *window;
    int32 x;
    int32 y;

    browser = native_browser_from_screen(screen);
    menu = native_browser_screen_menu(browser);
    if (nc_menu_empty(menu)) {
        return;
    }

    window = native_browser_screen_window(browser);
    x = event.x;
    y = event.y;
    if (!nc_window_has_coords(window, &x, &y)) {
        return;
    }

    if ((event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) != 0) {
        if (!nc_menu_goto_selectable(menu, y)) {
            return;
        }
        item = native_browser_screen_current_item(browser);
        if ((event.bstate & BUTTON1_PRESSED) && (item != NULL)) {
            (void)nc_menu_activate_current(menu);
        }
        return;
    }

    if ((event.bstate & BUTTON5_PRESSED) != 0) {
        if (browser->mouse_list_scroll_whole_page) {
            native_browser_scroll(screen, NC_SCROLL_PAGE_DOWN);
        } else {
            native_browser_mouse_scroll(browser, NC_SCROLL_DOWN);
        }
    } else if ((event.bstate & BUTTON4_PRESSED) != 0) {
        if (browser->mouse_list_scroll_whole_page) {
            native_browser_scroll(screen, NC_SCROLL_PAGE_UP);
        } else {
            native_browser_mouse_scroll(browser, NC_SCROLL_UP);
        }
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

static NcMenuDisplayCallbacks
native_browser_display_callbacks(NativeBrowserScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = native_browser_draw_item;
    callbacks.filter = native_browser_filter_item;
    callbacks.user = screen;
    return callbacks;
}

static NcMenuActionCallbacks
native_browser_action_callbacks(NativeBrowserScreen *screen) {
    NcMenuActionCallbacks callbacks = {0};

    callbacks.activate = native_browser_activate_item;
    callbacks.set_selected = native_browser_set_item_selected;
    callbacks.user = screen;
    return callbacks;
}

static void
native_browser_install_menu_callbacks(NativeBrowserScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }

    menu = native_browser_screen_menu(screen);
    nc_menu_set_display_callbacks(menu,
                                  native_browser_display_callbacks(screen));
    nc_menu_set_action_callbacks(menu,
                                 native_browser_action_callbacks(screen));
    native_browser_apply_menu_config(screen);
    return;
}

static void
native_browser_apply_menu_config(NativeBrowserScreen *screen) {
    NcMenu *menu;

    menu = native_browser_screen_menu(screen);
    nc_menu_set_highlight_prefix(menu, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(menu, &Config.current_item_suffix);
    nc_menu_set_selected_prefix(menu, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(menu, &Config.selected_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    return;
}

static void
native_browser_draw_item(NcMenu *menu, NcWindow *window,
                         void *item, int64 pos, void *user) {
    NativeBrowserScreen *screen;
    NcBuffer buffer;
    int64 available_width;
    int32 list_width;
    bool use_colors;

    screen = user;
    if ((menu == NULL) || (window == NULL) || (item == NULL)) {
        return;
    }

    nc_buffer_init(&buffer);
    if ((screen != NULL)
        && (screen->active_display_mode == NCM_DISPLAY_MODE_COLUMNS)
        && (ncm_mpd_item_kind(item) == NCM_MPD_ITEM_SONG)) {
        available_width = nc_window_width(window) - nc_window_get_x(window);
        if (nc_menu_position_is_selected(menu, pos)) {
            available_width -= ncm_utf8_width(
                menu->selected_suffix.data, menu->selected_suffix.len);
        }
        if (menu->highlight_enabled && (pos == menu->highlight)) {
            available_width -= ncm_utf8_width(
                menu->highlight_suffix.data, menu->highlight_suffix.len);
        }
        if (available_width < 0) {
            available_width = 0;
        }
        list_width = native_browser_i32_width(available_width);
        use_colors = !Config.discard_colors_if_item_is_selected
                     || !nc_menu_position_is_selected(menu, pos);
        ncm_display_song_columns(&buffer, ncm_mpd_item_song(item),
                                 Config.columns.items, Config.columns.len,
                                 list_width, use_colors);
    } else {
        ncm_display_item_row(&buffer, item, &Config.song_list_format,
                             NCM_FORMAT_FLAG_ALL,
                             Config.browser_playlist_prefix.data,
                             Config.browser_playlist_prefix.len);
    }
    native_browser_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);
    return;
}

static void
native_browser_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties;
    char *data;
    int32 property_count;
    int32 property_index;
    int32 len;

    data = nc_buffer_data(buffer);
    len = nc_buffer_len(buffer);
    properties = nc_buffer_properties(buffer);
    property_count = nc_buffer_property_count(buffer);
    property_index = 0;

    for (int32 i = 0;; i += 1) {
        while ((property_index < property_count)
               && (properties[property_index].position == i)) {
            nc_buffer_apply_property(window, &properties[property_index]);
            property_index += 1;
        }
        if (i >= len) {
            break;
        }
        nc_window_print_char(window, data[i]);
    }
    return;
}

static int32
native_browser_i32_width(int64 width) {
    if (width <= 0) {
        return 0;
    }
    if (width > INT32_MAX) {
        return INT32_MAX;
    }
    return (int32)width;
}

static void
native_browser_sync_window_title(NativeBrowserScreen *screen) {
    int32 width;

    if (screen == NULL) {
        return;
    }

    if ((screen->active_display_mode == NCM_DISPLAY_MODE_COLUMNS)
        && Config.titles_visibility) {
        width = native_browser_i32_width(screen->width);
        ncm_display_column_title(&screen->column_title_text,
                                 Config.columns.items, Config.columns.len,
                                 width);
        nc_window_set_title(&screen->window, screen->column_title_text.data,
                            screen->column_title_text.len);
    } else {
        ncm_buffer_clear(&screen->column_title_text);
        nc_window_set_title(&screen->window, NULL, 0);
    }
    return;
}

static void
native_browser_draw_header(NativeBrowserScreen *screen) {
    char *title;
    int32 len;

    if (screen == NULL) {
        return;
    }

    title = native_browser_title(&screen->screen);
    len = 0;
    if (title != NULL) {
        while (title[len] != '\0') {
            len += 1;
        }
    }
    ncm_title_draw_header(title, len);
    screen->redraw_header = false;
    return;
}

static void
native_browser_mouse_scroll(NativeBrowserScreen *screen,
                            enum NcScroll where) {
    for (int64 i = 0; i < screen->lines_scrolled; i += 1) {
        nc_menu_scroll_selectable(native_browser_screen_menu(screen),
                                  screen->main_height, where);
    }
    return;
}

static bool
native_browser_filter_item(NcMenu *menu, void *item, void *user) {
    NativeBrowserScreen *screen;

    (void)menu;
    screen = user;
    if ((screen == NULL) || !screen->filter_enabled) {
        return true;
    }
    return native_browser_item_matches(item, &screen->filter_regex);
}

static void
native_browser_activate_item(NcMenu *menu, void *item, int64 pos,
                             void *user) {
    NativeBrowserScreen *screen;

    (void)menu;
    (void)pos;
    screen = user;
    if (screen == NULL || item == NULL) {
        return;
    }
    if (native_browser_enter_item(screen, item)) {
        native_browser_screen_request_update(screen);
    }
    return;
}

static void
native_browser_set_item_selected(void *item, bool selected, void *user) {
    (void)item;
    (void)selected;
    (void)user;
    return;
}

static bool
native_browser_enter_item(NativeBrowserScreen *screen, NcmMpdItem *item) {
    NcmDirectory *directory;

    if (screen == NULL || item == NULL) {
        return false;
    }
    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_DIRECTORY) {
        return false;
    }

    directory = ncm_mpd_item_directory(item);
    return native_browser_screen_set_current_directory(
        screen, directory->path, directory->path_len);
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

static bool
native_browser_string_views_equal(NcmStringView left,
                                  NcmStringView right) {
    return ncm_string_equal(left.data, left.len, right.data, right.len);
}
