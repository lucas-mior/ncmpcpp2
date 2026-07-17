#include "screens/nc_browser.h"

#include "c/ncm_base.h"
#include "c/ncm_comparators.h"
#include "c/ncm_display.h"
#include "c/ncm_format.h"
#include "c/ncm_fs.h"
#include "c/ncm_path.h"
#include "c/ncm_string.h"
#include "c/ncm_tags.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"
#include "curses/nc_cyclic_buffer.h"
#include "global.h"
#include "settings.h"
#include "screens/screen_switcher.h"
#include "title.h"
#include "ui_state.h"

#include <errno.h>
#include <limits.h>
#include <mpd/client.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

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
static void native_browser_install_menu_callbacks(
    NativeBrowserScreen *screen);
static void native_browser_apply_menu_config(NativeBrowserScreen *screen);
static void native_browser_draw_item(NcMenu *menu, NcWindow *window,
                                     void *item, int64 pos, void *user);
static void native_browser_print_buffer(NcWindow *window, NcBuffer *buffer);
static int32 native_browser_i32_width(int64 width);
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
static void native_browser_sync_display_mode(NativeBrowserScreen *screen);
static int64 native_browser_render_width(NativeBrowserScreen *screen,
                                         int64 available_width,
                                         bool selected, bool highlighted);
static bool native_browser_item_matches(NativeBrowserScreen *screen,
                                        NcmMpdItem *item,
                                        NcmRegex *regex, bool filter);
static bool native_browser_directory_is_root(char *directory,
                                             int32 directory_len);
static bool native_browser_path_is_parent_directory(char *directory,
                                                    int32 directory_len);
static bool native_browser_set_normalized_directory(
    NativeBrowserScreen *screen, char *directory, int32 directory_len);
static bool native_browser_set_parent_of_directory(
    NativeBrowserScreen *screen, char *directory, int32 directory_len);
static bool native_browser_prepare_mpd_reload_directory(
    NativeBrowserScreen *screen);
static bool native_browser_load_mpd_items(NativeBrowserScreen *screen,
                                          NcmMpdItemArray *items);
static bool native_browser_reload_from_local(NativeBrowserScreen *screen,
                                             NcmError *error);
static bool native_browser_prepare_local_reload_directory(
    NativeBrowserScreen *screen, NcmError *error);
static bool native_browser_load_local_entry(NativeBrowserScreen *screen,
                                            NcmFsDirectory *directory,
                                            NcmFsEntry *entry,
                                            NcmError *error);
static bool native_browser_stat_local_path(char *path, int32 path_len,
                                           NcmFsStat *out,
                                           NcmError *error);
static enum NcmFsEntryType native_browser_local_mode_type(mode_t mode);
static bool native_browser_local_path_has_supported_extension(
    NativeBrowserScreen *screen, char *path, int32 path_len);
static bool native_browser_make_local_song(NcmSong *song, char *path,
                                           int32 path_len, time_t mtime);
static bool native_browser_collect_item_songs(
    NativeBrowserScreen *screen, NcmSongArray *songs, NcmMpdItem *item);
static bool native_browser_collect_mpd_directory_songs(
    NcmSongArray *songs, char *path, int32 path_len);
static bool native_browser_collect_local_directory_songs(
    NativeBrowserScreen *screen, NcmSongArray *songs, char *path,
    int32 path_len, NcmError *error);
static bool native_browser_collect_local_entry_songs(
    NativeBrowserScreen *screen, NcmSongArray *songs,
    NcmFsDirectory *directory, NcmFsEntry *entry, NcmError *error);
static bool native_browser_delete_item(NativeBrowserScreen *screen,
                                       NcmMpdClient *client,
                                       NcmMpdItem *item, NcmError *error);
static bool native_browser_delete_directory_item(
    NativeBrowserScreen *screen, NcmMpdItem *item, NcmError *error);
static bool native_browser_delete_song_item(NativeBrowserScreen *screen,
                                            NcmMpdItem *item,
                                            NcmError *error);
static bool native_browser_delete_playlist_item(
    NativeBrowserScreen *screen, NcmMpdClient *client,
    NcmMpdItem *item, NcmError *error);
static bool native_browser_current_directory_item_path(
    NativeBrowserScreen *screen, NcmStringView *path, NcmError *error);
static bool native_browser_current_playlist_item_path(
    NativeBrowserScreen *screen, NcmStringView *path, NcmError *error);
static bool native_browser_load_mpd_song_directory(
    NativeBrowserScreen *screen, NcmMpdClient *client,
    NcmStringView directory, NcmError *error);
static bool native_browser_highlight_song_item(NativeBrowserScreen *screen,
                                               NcmSong *song);
static bool native_browser_item_song_equal(NcmMpdItem *item,
                                           NcmSong *song);
static bool native_browser_rename_real_paths(
    NativeBrowserScreen *screen, NcmStringView old_path,
    NcmStringView new_path, NcmError *error);
static bool native_browser_update_renamed_directory(
    NcmMpdClient *client, NcmStringView old_path, NcmStringView new_path,
    NcmError *error);
static bool native_browser_real_path(NativeBrowserScreen *screen,
                                     NcmStringView path,
                                     NcmBuffer *real_path,
                                     NcmError *error);
static bool native_browser_delete_path_recursive(char *path,
                                                 int32 path_len,
                                                 NcmError *error);
static bool native_browser_remove_directory(char *path, int32 path_len,
                                            NcmError *error);
static void native_browser_set_errno_error(NcmError *error, int32 code,
                                           char *operation, char *path,
                                           int32 path_len);
static bool native_browser_supported_extensions_contains(
    NcmBufferArray *extensions, char *extension, int32 extension_len);
static bool native_browser_supported_extensions_add(
    NcmBufferArray *extensions, char *extension, int32 extension_len);
static int32 native_browser_compare_items(NativeBrowserScreen *screen,
                                          NcmMpdItem *left,
                                          NcmMpdItem *right);
static int32 native_browser_compare_item_values(
    NativeBrowserScreen *screen, NcmMpdItem *left, NcmMpdItem *right);
static int32 native_browser_item_sort_rank(NcmMpdItem *item);
static int32 native_browser_compare_views(NcmStringView left,
                                          NcmStringView right);
static int32 native_browser_compare_times(time_t left, time_t right);
static NcmStringView native_browser_directory_sort_view(
    NcmMpdItem *item);
static NcmStringView native_browser_playlist_sort_view(NcmMpdItem *item);
static NcmStringView native_browser_song_name_sort_view(NcmMpdItem *item);
static bool native_browser_highlight_last_directory(
    NativeBrowserScreen *screen);
static bool native_browser_item_is_song(NcmMpdItem *item);
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

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->lines_scrolled = 1;
    screen->title_scroll_beginning = 0;
    screen->active_display_mode = Config.browser_display_mode;
    screen->mouse_list_scroll_whole_page = false;
    screen->redraw_header = true;
    screen->update_requested = true;
    screen->local_browser = false;
    screen->filter_enabled = false;
    screen->registered = false;

    native_browser_screen_update_title_text(screen);
    native_browser_screen_update_column_title(screen);
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
    native_browser_screen_update_column_title(screen);
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
native_browser_screen_sort(NativeBrowserScreen *screen) {
    NcMenu *menu;
    int64 begin;
    int64 count;

    if (screen == NULL) {
        return false;
    }
    if (Config.browser_sort_mode == NCM_SORT_MODE_NONE) {
        return true;
    }

    menu = native_browser_screen_menu(screen);
    begin = 0;
    count = nc_menu_all_item_count(menu);
    if ((count > 0) && native_browser_screen_item_is_parent(
            nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, 0))) {
        begin = 1;
    }

    for (int64 i = begin + 1; i < count; i += 1) {
        for (int64 j = i; j > begin; j -= 1) {
            NcmMpdItem *left;
            NcmMpdItem *right;

            left = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, j - 1);
            right = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, j);
            if (native_browser_compare_items(screen, right, left) >= 0) {
                break;
            }
            nc_menu_swap_item_slots(menu, NC_MENU_ITEMS_ALL, j - 1, j);
        }
    }

    if (screen->filter_enabled) {
        nc_menu_apply_filter(menu);
    } else {
        nc_menu_show_all_items(menu);
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
    if (!native_browser_prepare_mpd_reload_directory(screen)) {
        return false;
    }

    while (true) {
        ncm_mpd_item_array_init(&items);
        result = ncm_mpd_client_get_directory_entries(
            client, screen->current_directory.data, &items, error);
        if (result) {
            result = native_browser_load_mpd_items(screen, &items);
            if (result) {
                native_browser_screen_clear_update_request(screen);
            }
            ncm_mpd_item_array_destroy(&items);
            return result;
        }
        ncm_mpd_item_array_destroy(&items);

        if (ncm_mpd_client_server_error_code(client)
            != MPD_SERVER_ERROR_NO_EXIST) {
            return false;
        }
        if (!native_browser_screen_go_to_parent(screen)) {
            return false;
        }
    }
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
native_browser_screen_update_title_text(NativeBrowserScreen *screen) {
    NcmBuffer scroll_buffer;
    NcmStringView directory;
    int64 scroll_beginning;
    int32 scroll_width;
    int32 screen_width;
    char separator[] = " ** ";

    if (screen == NULL) {
        return;
    }

    ncm_buffer_clear(&screen->title_text);
    ncm_buffer_append(&screen->title_text, STRLIT_ARGS("Browse: "));

    directory = native_browser_screen_current_directory(screen);
    if (directory.len <= 0) {
        directory = ncm_string_view_make("/", 1);
    }

    screen_width = (int32)ui_state_screen_width();
    if (screen_width <= 0) {
        screen_width = native_browser_i32_width(screen->width);
    }

    scroll_width = screen_width - utf8_width(screen->title_text.data,
                                                 screen->title_text.len);
    if (Config.design == NCM_DESIGN_ALTERNATIVE) {
        scroll_width -= 2;
    } else {
        scroll_width -= global_volume_state_len();
    }
    if (scroll_width < 0) {
        scroll_width = 0;
    }

    ncm_buffer_init(&scroll_buffer);
    scroll_beginning = screen->title_scroll_beginning;
    nc_cyclic_text_write(&scroll_buffer, directory.data, directory.len,
                         &scroll_beginning, scroll_width, separator,
                         (int32)SIZEOF(separator) - 1,
                         Config.header_text_scrolling);
    ncm_buffer_append(&screen->title_text, scroll_buffer.data,
                      scroll_buffer.len);
    screen->title_scroll_beginning = scroll_beginning;
    ncm_buffer_destroy(&scroll_buffer);
    return;
}

void
native_browser_screen_update_column_title(NativeBrowserScreen *screen) {
    int32 width;

    if (screen == NULL) {
        return;
    }

    ncm_buffer_clear(&screen->column_title_text);
    if ((screen->active_display_mode != NCM_DISPLAY_MODE_COLUMNS)
        || !Config.titles_visibility || (Config.columns.items == NULL)
        || (Config.columns.len <= 0) || (screen->main_height <= 2)) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    width = native_browser_i32_width(screen->width);
    if (width <= 0) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    ncm_display_column_title(&screen->column_title_text,
                             Config.columns.items,
                             Config.columns.len, width);
    nc_window_set_title(&screen->window, screen->column_title_text.data,
                        screen->column_title_text.len);
    return;
}

void
native_browser_screen_draw_header(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }

    native_browser_screen_update_title_text(screen);
    ncm_title_draw_header(screen->title_text.data, screen->title_text.len);
    screen->redraw_header = false;
    return;
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
    if (screen->active_display_mode == mode) {
        return;
    }
    screen->active_display_mode = mode;
    screen->redraw_header = true;
    native_browser_screen_update_column_title(screen);
    return;
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
    return native_browser_supported_extensions_contains(
        extensions, extension, extension_len);
}

bool
native_browser_screen_fetch_supported_extensions(
    NativeBrowserScreen *screen, NcmMpdClient *client, NcmError *error) {
    NcmMpdStringList strings;
    NcmBufferArray extensions;
    NcmMpdString *string;
    bool result;

    if (screen == NULL || client == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing browser extension state"));
        return false;
    }

    ncm_mpd_string_list_init(&strings);
    if (!ncm_mpd_client_get_supported_extensions(client, &strings, error)) {
        ncm_mpd_string_list_destroy(&strings);
        return false;
    }

    result = true;
    ncm_buffer_array_init(&extensions);
    for (int32 i = 0; result && (i < strings.count); i += 1) {
        string = &strings.items[i];
        result = native_browser_supported_extensions_add(
            &extensions, string->value, string->value_len);
    }

    if (result) {
        ncm_buffer_array_move(&screen->supported_extensions, &extensions);
        ncm_error_clear(error);
    }
    ncm_buffer_array_destroy(&extensions);
    ncm_mpd_string_list_destroy(&strings);
    return result;
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
    return native_browser_directory_is_root(screen->current_directory.data,
                                            screen->current_directory.len);
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

bool
native_browser_screen_change_browse_mode(
    NativeBrowserScreen *screen, NcmMpdClient *client, NcmError *error) {
    NcmBuffer directory;
    char *hostname;
    bool local_browser;
    bool result;

    if (screen == NULL || client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser state"));
        return false;
    }

    hostname = ncm_mpd_client_hostname(client);
    if ((hostname == NULL) || (hostname[0] != '/')) {
        ncm_error_set(
            error, EINVAL,
            STRLIT_ARGS(
                "local browsing requires an MPD UNIX socket"));
        return false;
    }

    ncm_buffer_init(&directory);
    local_browser = !screen->local_browser;
    if (local_browser) {
        if (!ncm_buffer_set(&directory, STRLIT_ARGS("~"))) {
            ncm_buffer_destroy(&directory);
            return false;
        }
        if (!ncm_path_expand_home(&directory, error)) {
            ncm_buffer_destroy(&directory);
            return false;
        }
    } else if (!ncm_buffer_set(&directory, STRLIT_ARGS("/"))) {
        ncm_buffer_destroy(&directory);
        return false;
    }

    result = native_browser_screen_set_current_directory(
        screen, directory.data, directory.len);
    if (result) {
        native_browser_screen_set_local(screen, local_browser);
        native_browser_screen_clear(screen);
        native_browser_screen_request_update(screen);
        if (local_browser && (screen->supported_extensions.len <= 0)) {
            NcmError fetch_error;

            ncm_error_clear(&fetch_error);
            (void)native_browser_screen_fetch_supported_extensions(
                screen, client, &fetch_error);
        }
    }

    ncm_buffer_destroy(&directory);
    return result;
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

    if (songs == NULL) {
        return false;
    }
    ncm_song_array_clear(songs);
    if (screen == NULL) {
        return false;
    }

    menu = native_browser_screen_menu(screen);
    if (nc_menu_empty(menu)) {
        return true;
    }

    any_selected = nc_menu_has_selected(menu);
    if (!any_selected) {
        return native_browser_collect_item_songs(
            screen, songs, nc_menu_current_item(menu));
    }

    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!native_browser_collect_item_songs(
                screen, songs, nc_menu_active_item_at(menu, i))) {
            return false;
        }
    }
    return true;
}

bool
native_browser_screen_delete_items(NativeBrowserScreen *screen,
                                   NcmMpdClient *client,
                                   NcmError *error) {
    NcMenu *menu;
    int64 count;
    bool any_selected;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser state"));
        return false;
    }
    if (!Config.allow_for_physical_item_deletion) {
        ncm_error_set(error, EPERM,
                      STRLIT_ARGS("physical deletion is forbidden"));
        return false;
    }

    menu = native_browser_screen_menu(screen);
    if (nc_menu_empty(menu)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("no browser item selected"));
        return false;
    }
    if (!screen->local_browser && (client == NULL)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }

    any_selected = nc_menu_has_selected(menu);
    count = nc_menu_item_count(menu);
    for (int64 i = 0; i < count; i += 1) {
        NcmMpdItem *item;

        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!any_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }

        item = nc_menu_active_item_at(menu, i);
        if (!native_browser_delete_item(screen, client, item, error)) {
            return false;
        }
    }

    if (!screen->local_browser) {
        char *directory;

        if (client == NULL) {
            ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
            return false;
        }
        directory = screen->current_directory.data;
        if (screen->current_directory.len <= 0) {
            directory = "/";
        }
        if (!ncm_mpd_client_update_directory(
                client, directory, NULL, error)) {
            return false;
        }
    }

    native_browser_screen_request_update(screen);
    ncm_error_clear(error);
    return true;
}

bool
native_browser_screen_current_directory_path(NativeBrowserScreen *screen,
                                             NcmStringView *path) {
    NcmError error;

    ncm_error_clear(&error);
    return native_browser_current_directory_item_path(screen, path, &error);
}

bool
native_browser_screen_current_playlist_path(NativeBrowserScreen *screen,
                                            NcmStringView *path) {
    NcmError error;

    ncm_error_clear(&error);
    return native_browser_current_playlist_item_path(screen, path, &error);
}

bool
native_browser_screen_rename_directory_available(
    NativeBrowserScreen *screen) {
    NcmStringView path;
    NcmError error;

    ncm_error_clear(&error);
    return native_browser_current_directory_item_path(screen, &path, &error)
        && (screen->local_browser || (Config.mpd_music_dir_len > 0));
}

bool
native_browser_screen_rename_playlist_available(
    NativeBrowserScreen *screen) {
    NcmStringView path;
    NcmError error;

    ncm_error_clear(&error);
    return native_browser_current_playlist_item_path(screen, &path, &error);
}

bool
native_browser_screen_rename_current_directory(
    NativeBrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *error) {
    NcmStringView old_path;
    NcmStringView new_path_view;

    if ((new_path == NULL) || (new_path_len <= 0)) {
        ncm_error_clear(error);
        return true;
    }
    if (!native_browser_current_directory_item_path(screen, &old_path,
                                                    error)) {
        return false;
    }
    if (STREQUAL(old_path.data, old_path.len,
                         new_path, new_path_len)) {
        ncm_error_clear(error);
        return true;
    }

    new_path_view = ncm_string_view_make(new_path, new_path_len);
    if (!native_browser_rename_real_paths(screen, old_path, new_path_view,
                                          error)) {
        return false;
    }
    if (!screen->local_browser
        && !native_browser_update_renamed_directory(
            client, old_path, new_path_view, error)) {
        return false;
    }

    native_browser_screen_request_update(screen);
    ncm_error_clear(error);
    return true;
}

bool
native_browser_screen_rename_current_playlist(
    NativeBrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *error) {
    NcmStringView old_path;

    if ((new_path == NULL) || (new_path_len <= 0)) {
        ncm_error_clear(error);
        return true;
    }
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (!native_browser_current_playlist_item_path(screen, &old_path,
                                                   error)) {
        return false;
    }
    if (STREQUAL(old_path.data, old_path.len,
                         new_path, new_path_len)) {
        ncm_error_clear(error);
        return true;
    }

    if (!ncm_mpd_client_rename_playlist(client, old_path.data, new_path,
                                        error)) {
        return false;
    }

    native_browser_screen_request_update(screen);
    ncm_error_clear(error);
    return true;
}

bool
native_browser_screen_locate_song(NativeBrowserScreen *screen,
                                  NcmSong *song, NcmMpdClient *client,
                                  NcmError *error) {
    NcmStringView directory;
    bool local_browser;
    bool result;

    if (screen == NULL || song == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser song"));
        return false;
    }
    if (!ncm_song_directory_view(song, 0, &directory)
        || (directory.len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("song directory is empty"));
        return false;
    }

    native_browser_screen_clear_filter(screen);
    local_browser = !ncm_song_is_from_database(song);
    native_browser_screen_set_local(screen, local_browser);

    if (local_browser) {
        if (!native_browser_screen_set_current_directory(
                screen, directory.data, directory.len)) {
            return false;
        }
        result = native_browser_reload_from_local(screen, error);
    } else {
        if (client == NULL) {
            ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
            return false;
        }
        result = native_browser_load_mpd_song_directory(
            screen, client, directory, error);
        if (!result
            && (ncm_mpd_client_server_error_code(client)
                == MPD_SERVER_ERROR_NO_EXIST)) {
            native_browser_screen_request_update(screen);
            ncm_error_clear(error);
            return true;
        }
    }

    if (!result) {
        return false;
    }

    (void)native_browser_highlight_song_item(screen, song);
    ncm_error_clear(error);
    return true;
}

bool
native_browser_screen_enter_directory(NativeBrowserScreen *screen) {
    if (!native_browser_enter_item(screen,
                                   native_browser_screen_current_item(
                                       screen))) {
        return false;
    }
    native_browser_screen_request_update(screen);
    return true;
}

bool
native_browser_screen_go_to_parent(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (native_browser_screen_in_root_directory(screen)) {
        return false;
    }
    if (!native_browser_set_parent_of_directory(
            screen, screen->current_directory.data,
            screen->current_directory.len)) {
        return false;
    }
    native_browser_screen_request_update(screen);
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
        if (native_browser_item_matches(screen,
                                        nc_menu_active_item_at(menu, pos),
                                        &regex, false)) {
            result = nc_menu_goto_selectable(menu, pos);
            break;
        }
    }

    ncm_regex_destroy(&regex);
    return result;
}

bool
native_browser_screen_render_item(NativeBrowserScreen *screen,
                                  NcBuffer *buffer, NcmMpdItem *item,
                                  int64 available_width, bool selected,
                                  bool highlighted) {
    int64 render_width;
    int32 list_width;
    bool use_colors;

    if ((screen == NULL) || (buffer == NULL) || (item == NULL)) {
        return false;
    }

    native_browser_sync_display_mode(screen);
    nc_buffer_clear(buffer);
    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        ncm_display_directory_row(buffer, ncm_mpd_item_directory(item));
        break;
    case NCM_MPD_ITEM_SONG:
        if (screen->active_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
            render_width = native_browser_render_width(
                screen, available_width, selected, highlighted);
            list_width = native_browser_i32_width(render_width);
            use_colors = !Config.discard_colors_if_item_is_selected
                         || !selected;
            ncm_display_song_columns(buffer, ncm_mpd_item_song(item),
                                     Config.columns.items,
                                     Config.columns.len, list_width,
                                     use_colors);
        } else {
            ncm_display_song_row(buffer, &Config.song_list_format,
                                 ncm_mpd_item_song(item),
                                 NCM_FORMAT_FLAG_ALL);
        }
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        ncm_display_playlist_row(buffer, ncm_mpd_item_playlist(item),
                                 Config.browser_playlist_prefix.data,
                                 Config.browser_playlist_prefix.len);
        break;
    case NCM_MPD_ITEM_UNKNOWN:
        break;
    }
    return true;
}

bool
native_browser_screen_item_to_string(NativeBrowserScreen *screen,
                                     NcmMpdItem *item,
                                     NcmBuffer *buffer) {
    NcmStringView path;
    NcmBuffer rendered;
    int32 basename;

    if ((screen == NULL) || (item == NULL) || (buffer == NULL)) {
        return false;
    }

    native_browser_sync_display_mode(screen);
    ncm_buffer_clear(buffer);
    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        if (!ncm_directory_path_view(ncm_mpd_item_directory(item), &path)) {
            return false;
        }
        basename = ncm_path_basename_start(path.data, path.len);
        ncm_buffer_append_byte(buffer, '[');
        ncm_buffer_append(buffer, path.data + basename,
                          path.len - basename);
        ncm_buffer_append_byte(buffer, ']');
        break;
    case NCM_MPD_ITEM_SONG:
        if (screen->active_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
            rendered = ncm_format_render_string(
                &Config.song_columns_mode_format,
                ncm_mpd_item_song(item));
        } else {
            rendered = ncm_format_render_string(
                &Config.song_list_format, ncm_mpd_item_song(item));
        }
        ncm_buffer_move(buffer, &rendered);
        ncm_buffer_destroy(&rendered);
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        if ((Config.browser_playlist_prefix.data != NULL)
            && (Config.browser_playlist_prefix.len > 0)) {
            ncm_buffer_append(buffer, Config.browser_playlist_prefix.data,
                              Config.browser_playlist_prefix.len);
        }
        if (!ncm_playlist_path_view(ncm_mpd_item_playlist(item), &path)) {
            return false;
        }
        basename = ncm_path_basename_start(path.data, path.len);
        ncm_buffer_append(buffer, path.data + basename,
                          path.len - basename);
        break;
    case NCM_MPD_ITEM_UNKNOWN:
        break;
    }
    return true;
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
    return native_browser_path_is_parent_directory(view.data, view.len);
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
    native_browser_screen_update_column_title(browser);
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
    if (nc_menu_empty(native_browser_screen_menu(browser))) {
        native_browser_screen_request_update(browser);
    }
    browser->redraw_header = true;
    native_browser_screen_draw_header(browser);
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
    browser->redraw_header = true;
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
native_browser_title(NcScreen *screen) {
    NativeBrowserScreen *browser;

    browser = native_browser_from_screen(screen);
    native_browser_screen_update_title_text(browser);
    return browser->title_text.data;
}

static void
native_browser_update(NcScreen *screen) {
    NativeBrowserScreen *browser;
    NcmError error;

    browser = native_browser_from_screen(screen);
    if (native_browser_screen_update_requested(browser)) {
        ncm_error_clear(&error);
        if (native_browser_screen_is_local(browser)) {
            (void)native_browser_reload_from_local(browser, &error);
        } else {
            (void)native_browser_screen_reload_from_mpd(
                browser, &global_mpd, &error);
        }
    }
    if (browser->redraw_header) {
        native_browser_screen_draw_header(browser);
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
    bool highlighted;
    bool selected;

    screen = user;
    if ((menu == NULL) || (window == NULL) || (item == NULL)) {
        return;
    }

    available_width = nc_window_width(window) - nc_window_get_x(window);
    selected = nc_menu_position_is_selected(menu, pos);
    highlighted = menu->highlight_enabled && (pos == menu->highlight);

    nc_buffer_init(&buffer);
    if (native_browser_screen_render_item(screen, &buffer, item,
                                          available_width, selected,
                                          highlighted)) {
        native_browser_print_buffer(window, &buffer);
    }
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
    return native_browser_item_matches(screen, item, &screen->filter_regex,
                                       true);
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
    return native_browser_set_normalized_directory(
        screen, directory->path, directory->path_len);
}

static void
native_browser_sync_display_mode(NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    native_browser_screen_set_display_mode(screen, Config.browser_display_mode);
    return;
}

static int64
native_browser_render_width(NativeBrowserScreen *screen,
                            int64 available_width,
                            bool selected, bool highlighted) {
    NcMenu *menu;
    int64 result;

    result = available_width;
    menu = native_browser_screen_menu(screen);
    if (selected) {
        result -= utf8_width(menu->selected_suffix.data,
                                 menu->selected_suffix.len);
    }
    if (highlighted) {
        result -= utf8_width(menu->highlight_suffix.data,
                                 menu->highlight_suffix.len);
    }
    if (result < 0) {
        result = 0;
    }
    return result;
}

static bool
native_browser_item_matches(NativeBrowserScreen *screen, NcmMpdItem *item,
                            NcmRegex *regex, bool filter) {
    if ((screen == NULL) || (regex == NULL)) {
        return false;
    }
    if (native_browser_screen_item_is_parent(item)) {
        return filter;
    }
    if (!native_browser_screen_item_to_string(screen, item,
                                              &screen->item_text_buffer)) {
        return false;
    }
    return ncm_regex_search(regex, screen->item_text_buffer.data,
                            screen->item_text_buffer.len);
}

static bool
native_browser_directory_is_root(char *directory, int32 directory_len) {
    if (directory_len <= 0) {
        return true;
    }
    return STREQUAL(directory, directory_len, STRLIT_ARGS("/"));
}

static bool
native_browser_path_is_parent_directory(char *directory,
                                        int32 directory_len) {
    if (directory_len <= 0) {
        return false;
    }
    if (STREQUAL(directory, directory_len, STRLIT_ARGS(".."))) {
        return true;
    }
    return ncm_string_ends_with(directory, directory_len, STRLIT_ARGS("/.."));
}

static bool
native_browser_set_normalized_directory(NativeBrowserScreen *screen,
                                        char *directory,
                                        int32 directory_len) {
    if (screen == NULL) {
        return false;
    }
    if (directory_len < 0) {
        return false;
    }
    if ((directory == NULL) && (directory_len > 0)) {
        return false;
    }
    if (native_browser_path_is_parent_directory(directory, directory_len)) {
        if (STREQUAL(directory, directory_len, STRLIT_ARGS(".."))) {
            return native_browser_set_parent_of_directory(
                screen, screen->current_directory.data,
                screen->current_directory.len);
        }
        return native_browser_set_parent_of_directory(
            screen, directory, directory_len - STRLIT_LEN("/.."));
    }
    return native_browser_screen_set_current_directory(
        screen, directory, directory_len);
}

static bool
native_browser_set_parent_of_directory(NativeBrowserScreen *screen,
                                       char *directory,
                                       int32 directory_len) {
    int32 parent_len;

    if (screen == NULL) {
        return false;
    }
    if (directory_len < 0) {
        return false;
    }
    if ((directory == NULL) && (directory_len > 0)) {
        return false;
    }
    if (native_browser_directory_is_root(directory, directory_len)) {
        return false;
    }

    parent_len = ncm_string_parent_directory_len(directory, directory_len);
    if (parent_len <= 0) {
        return native_browser_screen_set_current_directory(
            screen, STRLIT_ARGS("/"));
    }
    return native_browser_screen_set_current_directory(
        screen, directory, parent_len);
}

static bool
native_browser_prepare_mpd_reload_directory(
    NativeBrowserScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->current_directory.len <= 0) {
        return native_browser_screen_set_current_directory(
            screen, STRLIT_ARGS("/"));
    }
    if (native_browser_path_is_parent_directory(
            screen->current_directory.data, screen->current_directory.len)) {
        return native_browser_set_normalized_directory(
            screen, screen->current_directory.data,
            screen->current_directory.len);
    }
    return true;
}

static bool
native_browser_add_parent_directory_item(
    NativeBrowserScreen *screen) {
    NcmDirectory directory;
    NcmMpdItem item;
    bool result;

    if (screen == NULL) {
        return false;
    }
    if (native_browser_screen_in_root_directory(screen)) {
        return true;
    }

    ncm_buffer_clear(&screen->scratch_buffer);
    ncm_buffer_append(&screen->scratch_buffer,
                      screen->current_directory.data,
                      screen->current_directory.len);
    ncm_buffer_append(&screen->scratch_buffer, STRLIT_ARGS("/.."));

    ncm_directory_init(&directory);
    ncm_mpd_item_init(&item);
    result = ncm_directory_set(&directory, screen->scratch_buffer.data,
                               screen->scratch_buffer.len, 0)
             && ncm_mpd_item_set_directory(&item, &directory);
    if (result) {
        native_browser_screen_add_item_move(screen, &item);
    }
    ncm_mpd_item_destroy(&item);
    ncm_directory_destroy(&directory);
    return result;
}

static bool
native_browser_load_mpd_items(NativeBrowserScreen *screen,
                              NcmMpdItemArray *items) {
    NcMenu *menu;

    if (screen == NULL || items == NULL) {
        return false;
    }

    menu = native_browser_screen_menu(screen);
    screen->title_scroll_beginning = 0;
    nc_menu_show_all_items(menu);
    native_browser_screen_clear(screen);
    if (!native_browser_add_parent_directory_item(screen)) {
        return false;
    }
    for (int32 i = 0; i < items->len; i += 1) {
        if (!native_browser_screen_add_item_copy(screen, &items->items[i])) {
            return false;
        }
    }
    if (!native_browser_screen_sort(screen)) {
        return false;
    }

    if (screen->filter_enabled) {
        nc_menu_apply_filter(menu);
    } else {
        nc_menu_show_all_items(menu);
    }
    (void)native_browser_highlight_last_directory(screen);
    screen->redraw_header = true;
    return true;
}

static bool
native_browser_reload_from_local(NativeBrowserScreen *screen,
                                 NcmError *error) {
    NcmFsDirectory directory;
    NcmFsEntry entry;
    NcMenu *menu;
    bool result;

    if (screen == NULL) {
        return false;
    }
    if (!native_browser_prepare_local_reload_directory(screen, error)) {
        return false;
    }

    if (!ncm_fs_directory_open(&directory, screen->current_directory.data,
                               screen->current_directory.len, error)) {
        return false;
    }

    result = true;
    menu = native_browser_screen_menu(screen);
    screen->title_scroll_beginning = 0;
    nc_menu_show_all_items(menu);
    native_browser_screen_clear(screen);
    if (!native_browser_add_parent_directory_item(screen)) {
        result = false;
    }

    ncm_fs_entry_init(&entry);
    while (result && ncm_fs_directory_read(&directory, &entry, error)) {
        if (!native_browser_load_local_entry(screen, &directory, &entry,
                                             error)) {
            result = false;
        }
    }
    if (ncm_error_is_set(error)) {
        result = false;
    }
    ncm_fs_entry_destroy(&entry);
    ncm_fs_directory_close(&directory);

    if (result
        && ((Config.browser_sort_mode == NCM_SORT_MODE_NONE)
            || (Config.browser_sort_mode == NCM_SORT_MODE_TYPE))) {
        Config.browser_sort_mode = NCM_SORT_MODE_NAME;
    }
    if (result && !native_browser_screen_sort(screen)) {
        result = false;
    }

    if (result) {
        if (screen->filter_enabled) {
            nc_menu_apply_filter(menu);
        } else {
            nc_menu_show_all_items(menu);
        }
        (void)native_browser_highlight_last_directory(screen);
        screen->redraw_header = true;
        native_browser_screen_clear_update_request(screen);
    }
    return result;
}

static bool
native_browser_prepare_local_reload_directory(NativeBrowserScreen *screen,
                                              NcmError *error) {
    if (screen == NULL) {
        return false;
    }
    if (screen->current_directory.len <= 0) {
        if (!ncm_buffer_set(&screen->current_directory, STRLIT_ARGS("~"))) {
            return false;
        }
        if (!ncm_path_expand_home(&screen->current_directory, error)) {
            return false;
        }
        return true;
    }
    if (native_browser_path_is_parent_directory(
            screen->current_directory.data, screen->current_directory.len)) {
        return native_browser_set_normalized_directory(
            screen, screen->current_directory.data,
            screen->current_directory.len);
    }
    ncm_error_clear(error);
    return true;
}

static bool
native_browser_add_local_directory_item(NativeBrowserScreen *screen,
                                        NcmBuffer *path, time_t mtime) {
    NcmDirectory directory;
    NcmMpdItem item;
    bool result;

    ncm_directory_init(&directory);
    ncm_mpd_item_init(&item);
    result = ncm_directory_set(&directory, path->data, path->len, mtime)
             && ncm_mpd_item_set_directory(&item, &directory);
    if (result) {
        native_browser_screen_add_item_move(screen, &item);
    }
    ncm_mpd_item_destroy(&item);
    ncm_directory_destroy(&directory);
    return result;
}

static bool
native_browser_add_local_song_item(NativeBrowserScreen *screen,
                                   NcmBuffer *path, time_t mtime) {
    NcmSong song;
    NcmMpdItem item;
    bool result;

    ncm_song_init(&song);
    ncm_mpd_item_init(&item);
    result = native_browser_make_local_song(&song, path->data, path->len,
                                            mtime);
    if (result) {
        result = ncm_mpd_item_set_song(&item, &song);
    }
    if (result) {
        native_browser_screen_add_item_move(screen, &item);
    }
    ncm_mpd_item_destroy(&item);
    ncm_song_destroy(&song);
    return result;
}

static bool
native_browser_load_local_entry(NativeBrowserScreen *screen,
                                NcmFsDirectory *directory,
                                NcmFsEntry *entry, NcmError *error) {
    NcmBuffer path;
    NcmFsStat stat;
    bool result;

    if (screen == NULL || directory == NULL || entry == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing local entry"));
        return false;
    }
    if (!Config.local_browser_show_hidden_files
        && (entry->name_len > 0) && (entry->name[0] == '.')) {
        return true;
    }

    ncm_buffer_init(&path);
    if (!ncm_fs_join(&path, directory->path, directory->path_len,
                     entry->name, entry->name_len)) {
        ncm_buffer_destroy(&path);
        return false;
    }

    if (!native_browser_stat_local_path(path.data, path.len,
                                        &stat, error)) {
        ncm_buffer_destroy(&path);
        return false;
    }
    if (!stat.exists) {
        ncm_buffer_destroy(&path);
        return true;
    }

    result = true;
    if (stat.type == NCM_FS_ENTRY_DIRECTORY) {
        result = native_browser_add_local_directory_item(
            screen, &path, (time_t)stat.mtime);
    } else if ((stat.type == NCM_FS_ENTRY_FILE)
               && native_browser_local_path_has_supported_extension(
                   screen, path.data, path.len)) {
        result = native_browser_add_local_song_item(
            screen, &path, (time_t)stat.mtime);
    }

    ncm_buffer_destroy(&path);
    return result;
}

static bool
native_browser_stat_local_path(char *path, int32 path_len, NcmFsStat *out,
                               NcmError *error) {
    struct stat statbuf;
    char message[256];
    int32 message_len;

    if (out == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing stat output"));
        return false;
    }
    out->size = 0;
    out->mtime = 0;
    out->type = NCM_FS_ENTRY_UNKNOWN;
    out->exists = false;

    if (path == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing path"));
        return false;
    }
    if (path_len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("negative path length"));
        return false;
    }

    if (stat(path, &statbuf) != 0) {
        if (errno == ENOENT) {
            ncm_error_clear(error);
            return true;
        }
        message_len = snprintf(message, SIZEOF(message), "stat '%.*s': %s",
                               path_len, path, strerror(errno));
        ncm_error_set(error, errno, message, message_len);
        return false;
    }

    out->size = (int64)statbuf.st_size;
    out->mtime = (int64)statbuf.st_mtime;
    out->type = native_browser_local_mode_type(statbuf.st_mode);
    out->exists = true;
    ncm_error_clear(error);
    return true;
}

static enum NcmFsEntryType
native_browser_local_mode_type(mode_t mode) {
    if (S_ISREG(mode)) {
        return NCM_FS_ENTRY_FILE;
    }
    if (S_ISDIR(mode)) {
        return NCM_FS_ENTRY_DIRECTORY;
    }
    if (S_ISLNK(mode)) {
        return NCM_FS_ENTRY_SYMLINK;
    }
    return NCM_FS_ENTRY_UNKNOWN;
}

static bool
native_browser_local_path_has_supported_extension(
    NativeBrowserScreen *screen, char *path, int32 path_len) {
    int32 extension;

    extension = ncm_path_extension_start(path, path_len);
    if (extension <= 0) {
        return false;
    }
    return native_browser_screen_has_supported_extension(
        screen, path + extension - 1, path_len - extension + 1);
}

static bool
native_browser_make_local_song(NcmSong *song, char *path, int32 path_len,
                               time_t mtime) {
#if defined(HAVE_TAGLIB_H)
    struct mpd_pair pair;
    struct mpd_song *mpd_song;
#endif
    bool result;

    if (song == NULL || path == NULL || path_len < 0) {
        return false;
    }

    result = ncm_song_set_uri(song, path, path_len);
    if (result) {
        ncm_song_set_mtime(song, mtime);
    }

#if defined(HAVE_TAGLIB_H)
    if (result) {
        pair.name = "file";
        pair.value = path;
        mpd_song = mpd_song_begin(&pair);
        if (mpd_song != NULL) {
            if (ncm_tags_read_song(mpd_song)) {
                result = ncm_song_from_mpd_song(song, mpd_song);
                if (result) {
                    ncm_song_set_mtime(song, mtime);
                }
            }
            mpd_song_free(mpd_song);
        }
    }
#endif

    return result;
}

static bool
native_browser_collect_item_songs(NativeBrowserScreen *screen,
                                  NcmSongArray *songs, NcmMpdItem *item) {
    NcmStringView path;

    if (screen == NULL || songs == NULL || item == NULL) {
        return false;
    }

    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        if (!ncm_directory_path_view(ncm_mpd_item_directory(item), &path)) {
            return false;
        }
        if (screen->local_browser) {
            NcmError error;

            ncm_error_clear(&error);
            return native_browser_collect_local_directory_songs(
                screen, songs, path.data, path.len, &error);
        }
        return native_browser_collect_mpd_directory_songs(
            songs, path.data, path.len);
    case NCM_MPD_ITEM_SONG:
        return ncm_song_array_append_copy(songs,
                                          ncm_mpd_item_song(item));
    case NCM_MPD_ITEM_PLAYLIST:
    case NCM_MPD_ITEM_UNKNOWN:
        return true;
    }
    return true;
}

static bool
native_browser_collect_mpd_directory_songs(
    NcmSongArray *songs, char *path, int32 path_len) {
    NcmMpdSongList source;
    NcmError error;
    char *directory;
    bool result;

    if (songs == NULL || path == NULL || path_len < 0) {
        return false;
    }

    ncm_mpd_song_list_init(&source);
    ncm_error_clear(&error);
    directory = path;
    if (path_len <= 0) {
        directory = "/";
    }
    result = ncm_mpd_client_get_directory_recursive(
        &global_mpd, directory, &source, &error);
    for (int32 i = 0; result && (i < source.count); i += 1) {
        if (!ncm_song_array_append_copy(songs, &source.items[i])) {
            result = false;
        }
    }
    ncm_mpd_song_list_destroy(&source);
    return result;
}

static bool
native_browser_collect_local_directory_songs(
    NativeBrowserScreen *screen, NcmSongArray *songs, char *path,
    int32 path_len, NcmError *error) {
    NcmFsDirectory directory;
    NcmFsEntry entry;
    bool result;

    if (screen == NULL || songs == NULL || path == NULL || path_len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing local directory"));
        return false;
    }

    if (!ncm_fs_directory_open(&directory, path, path_len, error)) {
        return false;
    }

    result = true;
    ncm_fs_entry_init(&entry);
    while (result && ncm_fs_directory_read(&directory, &entry, error)) {
        result = native_browser_collect_local_entry_songs(
            screen, songs, &directory, &entry, error);
    }
    if (ncm_error_is_set(error)) {
        result = false;
    }
    ncm_fs_entry_destroy(&entry);
    ncm_fs_directory_close(&directory);
    return result;
}

static bool
native_browser_collect_local_entry_songs(
    NativeBrowserScreen *screen, NcmSongArray *songs,
    NcmFsDirectory *directory, NcmFsEntry *entry, NcmError *error) {
    NcmBuffer path;
    NcmFsStat stat;
    NcmSong song;
    bool result;

    if ((screen == NULL) || (songs == NULL) || (directory == NULL)
        || (entry == NULL)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing local entry"));
        return false;
    }
    if (!Config.local_browser_show_hidden_files
        && (entry->name_len > 0) && (entry->name[0] == '.')) {
        return true;
    }

    ncm_buffer_init(&path);
    if (!ncm_fs_join(&path, directory->path, directory->path_len,
                     entry->name, entry->name_len)) {
        ncm_buffer_destroy(&path);
        return false;
    }

    result = native_browser_stat_local_path(path.data, path.len,
                                            &stat, error);
    if (result && stat.exists && (stat.type == NCM_FS_ENTRY_DIRECTORY)) {
        result = native_browser_collect_local_directory_songs(
            screen, songs, path.data, path.len, error);
    } else if (result && stat.exists && (stat.type == NCM_FS_ENTRY_FILE)
               && native_browser_local_path_has_supported_extension(
                   screen, path.data, path.len)) {
        ncm_song_init(&song);
        result = native_browser_make_local_song(&song, path.data, path.len,
                                                (time_t)stat.mtime)
                 && ncm_song_array_append_copy(songs, &song);
        ncm_song_destroy(&song);
    }

    ncm_buffer_destroy(&path);
    return result;
}

static bool
native_browser_delete_item(NativeBrowserScreen *screen,
                           NcmMpdClient *client,
                           NcmMpdItem *item, NcmError *error) {
    if (item == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser item"));
        return false;
    }
    if (native_browser_screen_item_is_parent(item)) {
        ncm_error_set(
            error, EINVAL,
            STRLIT_ARGS("deletion of parent directory is forbidden"));
        return false;
    }

    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        return native_browser_delete_directory_item(screen, item, error);
    case NCM_MPD_ITEM_SONG:
        return native_browser_delete_song_item(screen, item, error);
    case NCM_MPD_ITEM_PLAYLIST:
        return native_browser_delete_playlist_item(screen, client, item,
                                                   error);
    case NCM_MPD_ITEM_UNKNOWN:
        break;
    }

    ncm_error_set(error, EINVAL, STRLIT_ARGS("unknown browser item"));
    return false;
}

static bool
native_browser_delete_directory_item(NativeBrowserScreen *screen,
                                     NcmMpdItem *item, NcmError *error) {
    NcmStringView path;
    NcmBuffer real_path;
    bool result;

    if (!ncm_directory_path_view(ncm_mpd_item_directory(item), &path)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing directory path"));
        return false;
    }

    ncm_buffer_init(&real_path);
    result = native_browser_real_path(screen, path, &real_path, error)
             && native_browser_delete_path_recursive(
                 real_path.data, real_path.len, error);
    ncm_buffer_destroy(&real_path);
    return result;
}

static bool
native_browser_delete_song_item(NativeBrowserScreen *screen,
                                NcmMpdItem *item, NcmError *error) {
    NcmStringView path;
    NcmBuffer real_path;
    bool result;

    if (!ncm_song_uri_view(ncm_mpd_item_song(item), 0, &path)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing song path"));
        return false;
    }

    ncm_buffer_init(&real_path);
    result = native_browser_real_path(screen, path, &real_path, error)
             && ncm_fs_unlink(real_path.data, real_path.len, error);
    ncm_buffer_destroy(&real_path);
    return result;
}

static bool
native_browser_delete_playlist_item(NativeBrowserScreen *screen,
                                    NcmMpdClient *client,
                                    NcmMpdItem *item, NcmError *error) {
    NcmStringView path;
    NcmBuffer real_path;
    bool result;

    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    if (!ncm_playlist_path_view(ncm_mpd_item_playlist(item), &path)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing playlist path"));
        return false;
    }

    if (ncm_mpd_client_delete_playlist(client, path.data, error)) {
        return true;
    }
    if (ncm_mpd_client_server_error_code(client)
        != MPD_SERVER_ERROR_NO_EXIST) {
        return false;
    }

    ncm_buffer_init(&real_path);
    result = native_browser_real_path(screen, path, &real_path, error)
             && ncm_fs_unlink(real_path.data, real_path.len, error);
    ncm_buffer_destroy(&real_path);
    return result;
}

static bool
native_browser_current_directory_item_path(NativeBrowserScreen *screen,
                                           NcmStringView *path,
                                           NcmError *error) {
    NcmMpdItem *item;

    if (path == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing path output"));
        return false;
    }
    ncm_string_view_clear(path);
    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser state"));
        return false;
    }

    item = native_browser_screen_current_item(screen);
    if (item == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser item"));
        return false;
    }
    if (native_browser_screen_item_is_parent(item)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("cannot rename parent directory"));
        return false;
    }
    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_DIRECTORY) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("browser item is not a directory"));
        return false;
    }
    if (!ncm_directory_path_view(ncm_mpd_item_directory(item), path)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing directory path"));
        return false;
    }
    ncm_error_clear(error);
    return true;
}

static bool
native_browser_current_playlist_item_path(NativeBrowserScreen *screen,
                                          NcmStringView *path,
                                          NcmError *error) {
    NcmMpdItem *item;

    if (path == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing path output"));
        return false;
    }
    ncm_string_view_clear(path);
    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser state"));
        return false;
    }

    item = native_browser_screen_current_item(screen);
    if (item == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser item"));
        return false;
    }
    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_PLAYLIST) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("browser item is not a playlist"));
        return false;
    }
    if (!ncm_playlist_path_view(ncm_mpd_item_playlist(item), path)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing playlist path"));
        return false;
    }
    ncm_error_clear(error);
    return true;
}

static bool
native_browser_load_mpd_song_directory(
    NativeBrowserScreen *screen, NcmMpdClient *client,
    NcmStringView directory, NcmError *error) {
    NcmMpdItemArray items;
    NcmBuffer path;
    bool result;

    if (screen == NULL || client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing browser state"));
        return false;
    }

    ncm_buffer_init(&path);
    if (directory.len <= 0) {
        result = ncm_buffer_set(&path, STRLIT_ARGS("/"));
    } else {
        result = ncm_buffer_set(&path, directory.data, directory.len);
    }
    if (!result) {
        ncm_buffer_destroy(&path);
        return false;
    }

    ncm_mpd_item_array_init(&items);
    result = ncm_mpd_client_get_directory_entries(client, path.data,
                                                  &items, error);
    if (result) {
        result = native_browser_screen_set_current_directory(
            screen, path.data, path.len)
                 && native_browser_load_mpd_items(screen, &items);
        if (result) {
            native_browser_screen_clear_update_request(screen);
        }
    }

    ncm_mpd_item_array_destroy(&items);
    ncm_buffer_destroy(&path);
    return result;
}

static bool
native_browser_highlight_song_item(NativeBrowserScreen *screen,
                                   NcmSong *song) {
    NcMenu *menu;

    if (screen == NULL || song == NULL) {
        return false;
    }

    menu = native_browser_screen_menu(screen);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!native_browser_item_song_equal(
                nc_menu_active_item_at(menu, i), song)) {
            continue;
        }
        nc_menu_highlight_position(menu, i, screen->main_height);
        return true;
    }
    return false;
}

static bool
native_browser_item_song_equal(NcmMpdItem *item, NcmSong *song) {
    if (item == NULL || song == NULL) {
        return false;
    }
    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_SONG) {
        return false;
    }
    return ncm_song_equal(ncm_mpd_item_song(item), song);
}

static bool
native_browser_rename_real_paths(NativeBrowserScreen *screen,
                                 NcmStringView old_path,
                                 NcmStringView new_path,
                                 NcmError *error) {
    NcmBuffer old_real_path;
    NcmBuffer new_real_path;
    bool result;

    ncm_buffer_init(&old_real_path);
    ncm_buffer_init(&new_real_path);
    result = native_browser_real_path(screen, old_path, &old_real_path,
                                      error)
             && native_browser_real_path(screen, new_path, &new_real_path,
                                         error)
             && ncm_fs_rename(old_real_path.data, old_real_path.len,
                              new_real_path.data, new_real_path.len,
                              error);
    ncm_buffer_destroy(&new_real_path);
    ncm_buffer_destroy(&old_real_path);
    return result;
}

static bool
native_browser_update_renamed_directory(NcmMpdClient *client,
                                        NcmStringView old_path,
                                        NcmStringView new_path,
                                        NcmError *error) {
    NcmBuffer shared;
    char *directory;
    bool result;

    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }

    shared = ncm_string_shared_directory(old_path.data, old_path.len,
                                         new_path.data, new_path.len);
    directory = shared.data;
    if (shared.len <= 0) {
        directory = "/";
    }
    result = ncm_mpd_client_update_directory(client, directory, NULL,
                                             error);
    ncm_buffer_destroy(&shared);
    return result;
}

static bool
native_browser_real_path(NativeBrowserScreen *screen, NcmStringView path,
                         NcmBuffer *real_path, NcmError *error) {
    if (real_path == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing real path output"));
        return false;
    }
    if (path.len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("negative path length"));
        return false;
    }
    if ((path.data == NULL) && (path.len > 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing path"));
        return false;
    }

    ncm_buffer_clear(real_path);
    if ((screen != NULL) && screen->local_browser) {
        return ncm_buffer_set(real_path, path.data, path.len);
    }

    if (Config.mpd_music_dir_len <= 0) {
        ncm_error_set(
            error, ENOENT,
            STRLIT_ARGS(
                "Proper mpd_music_dir variable has to be set in "
                "configuration file"));
        return false;
    }
    return ncm_fs_join(real_path, Config.mpd_music_dir,
                       Config.mpd_music_dir_len, path.data, path.len);
}

static bool
native_browser_delete_path_recursive(char *path, int32 path_len,
                                     NcmError *error) {
    NcmFsDirectory directory;
    NcmFsEntry entry;
    NcmFsStat stat;
    bool result;

    if (!ncm_fs_stat(path, path_len, &stat, error)) {
        return false;
    }
    if (!stat.exists) {
        return true;
    }
    if (stat.type != NCM_FS_ENTRY_DIRECTORY) {
        return ncm_fs_unlink(path, path_len, error);
    }

    if (!ncm_fs_directory_open(&directory, path, path_len, error)) {
        return false;
    }

    result = true;
    ncm_fs_entry_init(&entry);
    while (result && ncm_fs_directory_read(&directory, &entry, error)) {
        NcmBuffer child;

        ncm_buffer_init(&child);
        result = ncm_fs_join(&child, directory.path, directory.path_len,
                             entry.name, entry.name_len)
                 && native_browser_delete_path_recursive(
                     child.data, child.len, error);
        ncm_buffer_destroy(&child);
    }
    if (ncm_error_is_set(error)) {
        result = false;
    }
    ncm_fs_entry_destroy(&entry);
    ncm_fs_directory_close(&directory);

    if (!result) {
        return false;
    }
    return native_browser_remove_directory(path, path_len, error);
}

static bool
native_browser_remove_directory(char *path, int32 path_len,
                                NcmError *error) {
    char *copy;

    if (path == NULL || path_len < 0) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("invalid directory path"));
        return false;
    }

    copy = malloc2(path_len + 1);
    memcpy64(copy, path, path_len);
    copy[path_len] = '\0';
    if (rmdir(copy) != 0) {
        if (errno == ENOENT) {
            free2(copy, path_len + 1);
            ncm_error_clear(error);
            return true;
        }
        native_browser_set_errno_error(error, errno, "rmdir", path,
                                       path_len);
        free2(copy, path_len + 1);
        return false;
    }

    free2(copy, path_len + 1);
    ncm_error_clear(error);
    return true;
}

static void
native_browser_set_errno_error(NcmError *error, int32 code,
                               char *operation, char *path,
                               int32 path_len) {
    char message[256];
    int32 message_len;

    message_len = snprintf(message, SIZEOF(message), "%s '%.*s': %s",
                           operation, path_len, path, strerror(code));
    ncm_error_set(error, code, message, message_len);
    return;
}

static bool
native_browser_supported_extensions_contains(NcmBufferArray *extensions,
                                             char *extension,
                                             int32 extension_len) {
    if (extensions == NULL) {
        return false;
    }
    if (extension == NULL) {
        extension = "";
        extension_len = 0;
    }
    if (extension_len < 0) {
        extension_len = strlen32(extension);
    }

    for (int32 i = 0; i < extensions->len; i += 1) {
        NcmBuffer *item;

        item = &extensions->items[i];
        if (STREQUAL(item->data, item->len, extension,
                             extension_len)) {
            return true;
        }
    }
    return false;
}

static bool
native_browser_supported_extensions_add(NcmBufferArray *extensions,
                                        char *extension,
                                        int32 extension_len) {
    NcmBuffer buffer;
    bool result;

    if (extensions == NULL) {
        return false;
    }
    if (extension == NULL) {
        extension = "";
        extension_len = 0;
    }
    if (extension_len < 0) {
        extension_len = strlen32(extension);
    }

    ncm_buffer_init(&buffer);
    if ((extension_len <= 0) || (extension[0] != '.')) {
        result = ncm_buffer_set(&buffer, STRLIT_ARGS("."));
        if (result) {
            ncm_buffer_append(&buffer, extension, extension_len);
        }
    } else {
        result = ncm_buffer_set(&buffer, extension, extension_len);
    }

    if (result
        && !native_browser_supported_extensions_contains(
            extensions, buffer.data, buffer.len)) {
        result = ncm_buffer_array_append_copy(extensions, &buffer);
    }
    ncm_buffer_destroy(&buffer);
    return result;
}

static bool
native_browser_highlight_last_directory(
    NativeBrowserScreen *screen) {
    NcmStringView target;
    NcmStringView path;
    NcMenu *menu;

    if (screen == NULL) {
        return false;
    }

    target = native_browser_screen_last_highlighted_directory(screen);
    if (target.len <= 0) {
        return false;
    }

    menu = native_browser_screen_menu(screen);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMpdItem *item;

        item = nc_menu_active_item_at(menu, i);
        if (item == NULL) {
            continue;
        }
        if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_DIRECTORY) {
            continue;
        }
        if (!ncm_directory_path_view(ncm_mpd_item_directory(item), &path)) {
            continue;
        }
        if (native_browser_string_views_equal(path, target)) {
            nc_menu_highlight_position(menu, i, screen->main_height);
            return true;
        }
    }
    return false;
}

static int32
native_browser_compare_items(NativeBrowserScreen *screen,
                             NcmMpdItem *left, NcmMpdItem *right) {
    int32 left_rank;
    int32 right_rank;

    left_rank = native_browser_item_sort_rank(left);
    right_rank = native_browser_item_sort_rank(right);
    if (left_rank < right_rank) {
        return -1;
    }
    if (left_rank > right_rank) {
        return 1;
    }
    return native_browser_compare_item_values(screen, left, right);
}

static int32
native_browser_compare_item_values(NativeBrowserScreen *screen,
                                   NcmMpdItem *left,
                                   NcmMpdItem *right) {
    NcmBuffer left_buffer;
    NcmBuffer right_buffer;
    int32 result;

    (void)screen;
    switch (Config.browser_sort_mode) {
    case NCM_SORT_MODE_TYPE:
        return 0;
    case NCM_SORT_MODE_NAME:
        switch (ncm_mpd_item_kind(left)) {
        case NCM_MPD_ITEM_DIRECTORY:
            return native_browser_compare_views(
                native_browser_directory_sort_view(left),
                native_browser_directory_sort_view(right));
        case NCM_MPD_ITEM_SONG:
            return native_browser_compare_views(
                native_browser_song_name_sort_view(left),
                native_browser_song_name_sort_view(right));
        case NCM_MPD_ITEM_PLAYLIST:
            return native_browser_compare_views(
                native_browser_playlist_sort_view(left),
                native_browser_playlist_sort_view(right));
        case NCM_MPD_ITEM_UNKNOWN:
            return 0;
        }
        break;
    case NCM_SORT_MODE_CUSTOM_FORMAT:
        switch (ncm_mpd_item_kind(left)) {
        case NCM_MPD_ITEM_DIRECTORY:
            return native_browser_compare_views(
                native_browser_directory_sort_view(left),
                native_browser_directory_sort_view(right));
        case NCM_MPD_ITEM_PLAYLIST:
            return native_browser_compare_views(
                native_browser_playlist_sort_view(left),
                native_browser_playlist_sort_view(right));
        case NCM_MPD_ITEM_SONG:
            left_buffer = ncm_format_render_string(
                &Config.browser_sort_format, ncm_mpd_item_song(left));
            right_buffer = ncm_format_render_string(
                &Config.browser_sort_format, ncm_mpd_item_song(right));
            result = native_browser_compare_views(
                ncm_string_view_make(left_buffer.data, left_buffer.len),
                ncm_string_view_make(right_buffer.data, right_buffer.len));
            ncm_buffer_destroy(&right_buffer);
            ncm_buffer_destroy(&left_buffer);
            return result;
        case NCM_MPD_ITEM_UNKNOWN:
            return 0;
        }
        break;
    case NCM_SORT_MODE_MODIFICATION_TIME:
        switch (ncm_mpd_item_kind(left)) {
        case NCM_MPD_ITEM_DIRECTORY:
            return native_browser_compare_times(
                ncm_directory_last_modified(
                    ncm_mpd_item_directory(left)),
                ncm_directory_last_modified(
                    ncm_mpd_item_directory(right)));
        case NCM_MPD_ITEM_PLAYLIST:
            return native_browser_compare_times(
                ncm_playlist_last_modified(
                    ncm_mpd_item_playlist(left)),
                ncm_playlist_last_modified(
                    ncm_mpd_item_playlist(right)));
        case NCM_MPD_ITEM_SONG:
            return native_browser_compare_times(
                ncm_song_mtime(ncm_mpd_item_song(left)),
                ncm_song_mtime(ncm_mpd_item_song(right)));
        case NCM_MPD_ITEM_UNKNOWN:
            return 0;
        }
        break;
    case NCM_SORT_MODE_NONE:
    case NCM_SORT_MODE_LAST:
        return 0;
    }
    return 0;
}

static int32
native_browser_item_sort_rank(NcmMpdItem *item) {
    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        return 0;
    case NCM_MPD_ITEM_SONG:
        return 1;
    case NCM_MPD_ITEM_PLAYLIST:
        return 2;
    case NCM_MPD_ITEM_UNKNOWN:
        return 3;
    }
    return 3;
}

static int32
native_browser_compare_views(NcmStringView left, NcmStringView right) {
    return ncm_compare_locale_strings(left.data, left.len, right.data,
                                      right.len,
                                      Config.ignore_leading_the);
}

static int32
native_browser_compare_times(time_t left, time_t right) {
    if (left > right) {
        return -1;
    }
    if (left < right) {
        return 1;
    }
    return 0;
}

static NcmStringView
native_browser_directory_sort_view(NcmMpdItem *item) {
    NcmStringView view;

    ncm_string_view_clear(&view);
    (void)ncm_directory_path_view(ncm_mpd_item_directory(item), &view);
    return view;
}

static NcmStringView
native_browser_playlist_sort_view(NcmMpdItem *item) {
    NcmStringView view;

    ncm_string_view_clear(&view);
    (void)ncm_playlist_path_view(ncm_mpd_item_playlist(item), &view);
    return view;
}

static NcmStringView
native_browser_song_name_sort_view(NcmMpdItem *item) {
    NcmStringView view;

    ncm_string_view_clear(&view);
    (void)ncm_song_name_view(ncm_mpd_item_song(item), 0, &view);
    return view;
}

static bool
native_browser_item_is_song(NcmMpdItem *item) {
    if (item == NULL) {
        return false;
    }
    return ncm_mpd_item_kind(item) == NCM_MPD_ITEM_SONG;
}

static bool
native_browser_string_views_equal(NcmStringView left,
                                  NcmStringView right) {
    return STREQUAL(left.data, left.len, right.data, right.len);
}
