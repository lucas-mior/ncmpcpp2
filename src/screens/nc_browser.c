#if !defined(NCMPCPP_NC_BROWSER_C)
#define NCMPCPP_NC_BROWSER_C

#include "cbase.h"

#include <mpd/client.h>

#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "global.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "title.h"
#include "ui_state.h"

static void browser_display(BrowserScreen *screen);
static void browser_switch_to(NcScreen *screen);
static void browser_resize(NcScreen *screen);
static char *browser_title(NcScreen *screen);
static void browser_update(NcScreen *screen);
static void browser_mouse_button_pressed(NcScreen *screen, MEVENT event);

#define NC_SCREEN_IMPL_TYPE BrowserScreen
#define NC_SCREEN_IMPL_PREFIX browser
#define NC_SCREEN_IMPL_PUBLIC_PREFIX browser_screen
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_MENU(screen) browser_screen_menu(screen)
#define NC_SCREEN_IMPL_SCROLL_HEIGHT(screen) ((screen)->main_height)
#define NC_SCREEN_IMPL_REFRESH_CALLBACK browser_display
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK browser_switch_to
#define NC_SCREEN_IMPL_RESIZE_CALLBACK browser_resize
#define NC_SCREEN_IMPL_TITLE_CALLBACK browser_title
#define NC_SCREEN_IMPL_UPDATE_CALLBACK browser_update
#define NC_SCREEN_IMPL_MOUSE_CALLBACK browser_mouse_button_pressed
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK browser_screen_destroy
#define NC_SCREEN_IMPL_LOCKABLE true
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.h"

static void browser_locate_last_directory(BrowserScreen *screen);
static bool browser_string_views_matches(NcmStringView left,
                                         NcmStringView right);

typedef struct BrowserSearchContext {
    BrowserScreen *screen;
    NcmRegex *regex;
} BrowserSearchContext;

static void
browser_sync_display_mode(BrowserScreen *screen) {
    browser_screen_set_display_mode(screen, Config.browser_display_mode);
    return;
}

static void
browser_draw_item(NcMenu *menu, NcWindow *window,
                  void *item, int32 pos, void *user) {
    BrowserScreen *screen = user;
    NcBuffer buffer = {0};
    int32 available_width;
    int32 list_width;
    bool highlighted;
    bool selected;
    bool use_colors;

    available_width = nc_window_width(window) - nc_window_get_x(window);
    selected = nc_menu_position_is_selected(menu, pos);
    highlighted = !menu->highlight_disabled && (pos == menu->highlight);

    browser_sync_display_mode(screen);
    nc_buffer_clear(&buffer);
    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        ncm_display_directory_row(&buffer, ncm_mpd_item_directory(item));
        break;
    case NCM_MPD_ITEM_SONG:
        if (screen->active_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
            list_width = available_width;
            if (selected) {
                list_width -= utf8_width(menu->selected_suffix.data,
                                         menu->selected_suffix.len);
            }
            if (highlighted) {
                list_width -= utf8_width(menu->highlight_suffix.data,
                                         menu->highlight_suffix.len);
            }
            if (list_width < 0) {
                list_width = 0;
            }
            use_colors = !Config.discard_colors_if_item_is_selected
                         || !selected;
            ncm_display_song_columns(
                &buffer, ncm_mpd_item_song(item), Config.columns.items,
                Config.columns.len, list_width, use_colors);
        } else {
            ncm_display_song_row(&buffer, &Config.song_list_format,
                                 ncm_mpd_item_song(item),
                                 NCM_FORMAT_FLAG_ALL);
        }
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        ncm_display_playlist_row(
            &buffer, ncm_mpd_item_playlist(item),
            Config.browser_playlist_prefix.data,
            Config.browser_playlist_prefix.len);
        break;
    case NCM_MPD_ITEM_COUNT:
    default:
        break;
    }

    {
        NcBufferProperty *properties = nc_buffer_properties(&buffer);
        char *data = nc_buffer_data(&buffer);
        int32 data_len = nc_buffer_len(&buffer);
        int32 property_count = ARRAY_LEN(buffer.properties);
        int32 property_index = 0;

        for (int32 i = 0;; i += 1) {
            while ((property_index < property_count)
                   && (properties[property_index].position == i)) {
                nc_buffer_apply_property(
                    window, &properties[property_index]);
                property_index += 1;
            }
            if (i >= data_len) {
                break;
            }
            nc_window_print_char(window, data[i]);
        }
    }
    nc_buffer_destroy(&buffer);
    return;
}

static bool
browser_path_is_parent_directory(char *directory,
                                 int32 directory_len) {
    if (directory_len <= 0) {
        return false;
    }
    if (STREQUAL(directory, directory_len, "..")) {
        return true;
    }
    return ENDS_WITH(directory, directory_len, "/..");
}

static bool
browser_directory_is_root(char *directory, int32 directory_len) {
    if (directory_len <= 0) {
        return true;
    }
    return STREQUAL(directory, directory_len, "/");
}

static int32
browser_set_parent_of_directory(BrowserScreen *screen,
                                char *directory,
                                int32 directory_len) {
    int32 parent_len;

    if (browser_directory_is_root(directory, directory_len)) {
        return -ENOENT;
    }

    parent_len = ncm_string_parent_directory_len(directory, directory_len);
    if (parent_len <= 0) {
        browser_screen_set_current_directory(screen, STRLIT("/"));
    } else {
        browser_screen_set_current_directory(screen, directory, parent_len);
    }
    return 0;
}

static int32
browser_set_normalized_directory(BrowserScreen *screen,
                                 char *directory, int32 directory_len) {
    if (browser_path_is_parent_directory(directory, directory_len)) {
        if (STREQUAL(directory, directory_len, "..")) {
            return browser_set_parent_of_directory(
                screen, screen->current_directory.data,
                screen->current_directory.len);
        }
        return browser_set_parent_of_directory(
            screen, directory, directory_len - STRLIT_LEN("/.."));
    }
    browser_screen_set_current_directory(screen, directory, directory_len);
    return 0;
}

static int32
browser_enter_item(BrowserScreen *screen, NcmMpdItem *item) {
    NcmDirectory *directory;

    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_DIRECTORY) {
        return -NCM_ERROR_UNAVAILABLE;
    }

    directory = ncm_mpd_item_directory(item);
    return browser_set_normalized_directory(
        screen, directory->path, directory->path_len);
}

static void
browser_activate_item(NcMenu *menu, void *item, int32 pos, void *user) {
    BrowserScreen *screen = user;

    (void)menu;
    (void)pos;

    if (browser_enter_item(screen, item) == 0) {
        browser_screen_request_update(screen);
    }
    return;
}

static void
browser_set_item_selected(void *item, bool selected, void *user) {
    (void)item;
    (void)selected;
    (void)user;
    return;
}

static bool
browser_item_matches(BrowserScreen *screen, NcmMpdItem *item,
                     NcmRegex *regex, bool filter) {
    NcmStringView path = {0};
    StrBuilder rendered = {0};
    int32 basename;

    if (browser_screen_item_is_parent(item)) {
        return filter;
    }

    browser_sync_display_mode(screen);
    sb_clear(&screen->item_text_buffer);
    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        ncm_directory_has_path_view(ncm_mpd_item_directory(item),
                                    &path);
        basename = ncm_path_basename_start(path.data, path.len);
        sb_append_byte(&screen->item_text_buffer, '[');
        SB_APPEND(&screen->item_text_buffer,
                  path.data + basename, path.len - basename);
        sb_append_byte(&screen->item_text_buffer, ']');
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
        sb_move(&screen->item_text_buffer, &rendered);
        sb_free(&rendered);
        break;
    case NCM_MPD_ITEM_PLAYLIST:
        if (Config.browser_playlist_prefix.data
            && (Config.browser_playlist_prefix.len > 0)) {
            SB_APPEND(&screen->item_text_buffer,
                      Config.browser_playlist_prefix.data,
                      Config.browser_playlist_prefix.len);
        }
        ncm_playlist_has_path_view(ncm_mpd_item_playlist(item),
                                   &path);
        basename = ncm_path_basename_start(path.data, path.len);
        SB_APPEND(&screen->item_text_buffer,
                  path.data + basename, path.len - basename);
        break;
    case NCM_MPD_ITEM_COUNT:
        break;
    default:
        return false;
    }
    return ncm_regex_matches(regex, screen->item_text_buffer.data,
                            screen->item_text_buffer.len);
}

static bool
browser_item_matches_filter(NcMenu *menu, void *item, void *user) {
    BrowserScreen *screen = user;

    (void)menu;

    if (!screen->filter_enabled) {
        return true;
    }
    return browser_item_matches(screen, item, &screen->filter_regex, true);
}

static void
browser_install_menu_callbacks(BrowserScreen *screen) {
    NcMenuDisplayCallbacks display_callbacks = {0};
    NcMenuActionCallbacks action_callbacks = {0};
    NcMenu *menu;

    menu = browser_screen_menu(screen);

    display_callbacks.draw = browser_draw_item;
    display_callbacks.matches_filter = browser_item_matches_filter;
    display_callbacks.user = screen;
    nc_menu_set_display_callbacks(menu, display_callbacks);

    action_callbacks.activate = browser_activate_item;
    action_callbacks.set_selected = browser_set_item_selected;
    action_callbacks.user = screen;
    nc_menu_set_action_callbacks(menu, action_callbacks);

    nc_menu_set_highlight_prefix(menu, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(menu, &Config.current_item_suffix);
    nc_menu_set_selected_prefix(menu, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(menu, &Config.selected_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    return;
}

void
browser_screen_init(BrowserScreen *screen,
                    int32 start_x, int32 width,
                    int32 main_start_y, int32 main_height,
                    NcColor color, NcBorder border) {
    nc_browser_entry_menu_init(&screen->entries);
    nc_window_init(&screen->window,
                   start_x, main_start_y, width, main_height,
                   NULL, 0, color, border);

    screen->current_directory = (StrBuilder){0};
    screen->last_highlighted_directory = (StrBuilder){0};
    screen->title_text = (StrBuilder){0};
    screen->column_title_text = (StrBuilder){0};
    screen->filter_constraint = (StrBuilder){0};
    screen->search_constraint = (StrBuilder){0};
    screen->item_text_buffer = (StrBuilder){0};
    screen->path_buffer = (StrBuilder){0};
    screen->scratch_buffer = (StrBuilder){0};

    str_builder_array_init(&screen->supported_extensions);
    screen->filter_regex = (NcmRegex){0};

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

    browser_screen_update_title_text(screen);
    browser_screen_update_column_title(screen);
    browser_install_menu_callbacks(screen);
    nc_screen_init_ops(&screen->screen, browser_ops, screen,
                       NC_SCREEN_TYPE_BROWSER);
    return;
}

void
browser_screen_destroy(BrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }

    ncm_regex_destroy(&screen->filter_regex);
    str_builder_array_destroy(&screen->supported_extensions);

    sb_free(&screen->scratch_buffer);
    sb_free(&screen->path_buffer);
    sb_free(&screen->item_text_buffer);
    sb_free(&screen->filter_constraint);
    sb_free(&screen->search_constraint);
    sb_free(&screen->column_title_text);
    sb_free(&screen->title_text);
    sb_free(&screen->last_highlighted_directory);
    sb_free(&screen->current_directory);

    nc_window_destroy(&screen->window);
    nc_browser_entry_menu_destroy(&screen->entries);
    return;
}

NcBrowserEntryMenu *
browser_screen_entries(BrowserScreen *screen) {
    return &screen->entries;
}

NcMenu *
browser_screen_menu(BrowserScreen *screen) {
    return nc_browser_entry_menu_base(&screen->entries);
}

NcWindow *
browser_screen_window(BrowserScreen *screen) {
    return &screen->window;
}

void
browser_screen_set_mouse_config(BrowserScreen *screen,
                                int32 lines_scrolled,
                                bool scroll_whole_page) {
    screen->lines_scrolled = lines_scrolled;
    screen->mouse_list_scroll_whole_page = scroll_whole_page;
    return;
}

void
browser_screen_clear(BrowserScreen *screen) {
    nc_menu_clear_items(browser_screen_menu(screen));
    screen->redraw_header = true;
    return;
}

void
browser_screen_add_item_move(BrowserScreen *screen,
                             NcmMpdItem *item) {
    NcmMpdItem copy;

    if ((screen == NULL) || (item == NULL)) {
        return;
    }
    ncm_mpd_item_init(&copy);
    ncm_mpd_item_move(&copy, item);
    nc_browser_entry_menu_add(browser_screen_entries(screen), &copy);
    ncm_mpd_item_destroy(&copy);
    return;
}

static int32
browser_item_sort_rank(NcmMpdItem *item) {
    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY:
        return 0;
    case NCM_MPD_ITEM_SONG:
        return 1;
    case NCM_MPD_ITEM_PLAYLIST:
        return 2;
    case NCM_MPD_ITEM_COUNT:
        return 3;
    default:
        break;
    }
    return 3;
}

static int32
browser_compare_views(NcmStringView left, NcmStringView right) {
    return ncm_compare_locale_strings(left.data, left.len, right.data,
                                      right.len,
                                      Config.ignore_leading_the);
}

static int32
browser_compare_times(time_t left, time_t right) {
    if (left > right) {
        return -1;
    }
    if (left < right) {
        return 1;
    }
    return 0;
}

static NcmStringView
browser_directory_sort_view(NcmMpdItem *item) {
    NcmStringView view;

    ncm_string_view_clear(&view);
    ncm_directory_has_path_view(ncm_mpd_item_directory(item), &view);
    return view;
}

static NcmStringView
browser_playlist_sort_view(NcmMpdItem *item) {
    NcmStringView view;

    ncm_string_view_clear(&view);
    ncm_playlist_has_path_view(ncm_mpd_item_playlist(item), &view);
    return view;
}

static NcmStringView
browser_song_name_sort_view(NcmMpdItem *item) {
    NcmStringView view;

    ncm_string_view_clear(&view);
    ncm_song_has_name_view(ncm_mpd_item_song(item), 0, &view);
    return view;
}

int32
browser_screen_sort(BrowserScreen *screen) {
    NcMenu *menu;
    int32 begin;
    int32 count;

    if (screen == NULL) {
        return -EINVAL;
    }
    if (Config.browser_sort_mode == NCM_SORT_MODE_NONE) {
        return 0;
    }

    menu = browser_screen_menu(screen);
    begin = 0;
    count = nc_menu_all_item_count(menu);
    if ((count > 0) && browser_screen_item_is_parent(
        nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, 0))) {
        begin = 1;
    }

    for (int32 i = begin + 1; i < count; i += 1) {
        for (int32 j = i; j > begin; j -= 1) {
            NcmMpdItem *left = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, j - 1);
            NcmMpdItem *right = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, j);
            int32 left_rank = browser_item_sort_rank(right);
            int32 right_rank = browser_item_sort_rank(left);
            int32 comparison;

            if (left_rank < right_rank) {
                comparison = -1;
            } else if (left_rank > right_rank) {
                comparison = 1;
            } else {
                comparison = 0;
                switch (Config.browser_sort_mode) {
                case NCM_SORT_MODE_TYPE:
                    break;
                case NCM_SORT_MODE_NAME:
                    switch (ncm_mpd_item_kind(right)) {
                    case NCM_MPD_ITEM_DIRECTORY:
                        comparison = browser_compare_views(
                            browser_directory_sort_view(right),
                            browser_directory_sort_view(left));
                        break;
                    case NCM_MPD_ITEM_SONG:
                        comparison = browser_compare_views(
                            browser_song_name_sort_view(right),
                            browser_song_name_sort_view(left));
                        break;
                    case NCM_MPD_ITEM_PLAYLIST:
                        comparison = browser_compare_views(
                            browser_playlist_sort_view(right),
                            browser_playlist_sort_view(left));
                        break;
                    case NCM_MPD_ITEM_COUNT:
                    default:
                        break;
                    }
                    break;
                case NCM_SORT_MODE_CUSTOM_FORMAT:
                    switch (ncm_mpd_item_kind(right)) {
                    case NCM_MPD_ITEM_DIRECTORY:
                        comparison = browser_compare_views(
                            browser_directory_sort_view(right),
                            browser_directory_sort_view(left));
                        break;
                    case NCM_MPD_ITEM_PLAYLIST:
                        comparison = browser_compare_views(
                            browser_playlist_sort_view(right),
                            browser_playlist_sort_view(left));
                        break;
                    case NCM_MPD_ITEM_SONG: {
                        StrBuilder right_buffer = ncm_format_render_string(
                            &Config.browser_sort_format,
                            ncm_mpd_item_song(right));
                        StrBuilder left_buffer = ncm_format_render_string(
                            &Config.browser_sort_format,
                            ncm_mpd_item_song(left));

                        comparison = browser_compare_views(
                            ncm_string_view_make(
                                right_buffer.data, right_buffer.len),
                            ncm_string_view_make(
                                left_buffer.data, left_buffer.len));
                        sb_free(&left_buffer);
                        sb_free(&right_buffer);
                        break;
                    }
                    case NCM_MPD_ITEM_COUNT:
                    default:
                        break;
                    }
                    break;
                case NCM_SORT_MODE_MODIFICATION_TIME:
                    switch (ncm_mpd_item_kind(right)) {
                    case NCM_MPD_ITEM_DIRECTORY:
                        comparison = browser_compare_times(
                            ncm_directory_last_modified(
                                ncm_mpd_item_directory(right)),
                            ncm_directory_last_modified(
                                ncm_mpd_item_directory(left)));
                        break;
                    case NCM_MPD_ITEM_PLAYLIST:
                        comparison = browser_compare_times(
                            ncm_playlist_last_modified(
                                ncm_mpd_item_playlist(right)),
                            ncm_playlist_last_modified(
                                ncm_mpd_item_playlist(left)));
                        break;
                    case NCM_MPD_ITEM_SONG:
                        comparison = browser_compare_times(
                            ncm_song_mtime(ncm_mpd_item_song(right)),
                            ncm_song_mtime(ncm_mpd_item_song(left)));
                        break;
                    case NCM_MPD_ITEM_COUNT:
                    default:
                        break;
                    }
                    break;
                case NCM_SORT_MODE_NONE:
                case NCM_SORT_MODE_COUNT:
                default:
                    break;
                }
            }
            if (comparison >= 0) {
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
    return 0;
}

int32
browser_screen_set_current_directory(BrowserScreen *screen,
                                     char *directory,
                                     int32 directory_len) {
    NcmStringView current;
    NcmStringView replacement;

    if (screen == NULL) {
        return -EINVAL;
    }
    if (directory_len < 0) {
        return -EINVAL;
    }
    if ((directory == NULL) && (directory_len > 0)) {
        return -EINVAL;
    }

    current = ncm_string_view_make(screen->current_directory.data,
                                   screen->current_directory.len);
    replacement = ncm_string_view_make(directory, directory_len);
    if (!browser_string_views_matches(current, replacement)) {
        sb_set(&screen->last_highlighted_directory, current.data, current.len);
        screen->title_scroll_beginning = 0;
        screen->redraw_header = true;
    }
    sb_set(&screen->current_directory, directory, directory_len);
    return 0;
}

NcmStringView
browser_screen_current_directory(BrowserScreen *screen) {
    if (screen == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(screen->current_directory.data,
                                screen->current_directory.len);
}

void
browser_screen_update_title_text(BrowserScreen *screen) {
    StrBuilder scroll_buffer = {0};
    NcmStringView directory;
    int32 scroll_beginning;
    int32 scroll_width;
    int32 screen_width;
    char separator[] = " ** ";

    if (screen == NULL) {
        return;
    }

    sb_clear(&screen->title_text);
    SB_APPEND(&screen->title_text, "Browse: ");

    directory = browser_screen_current_directory(screen);
    if (directory.len <= 0) {
        directory = ncm_string_view_make("/", 1);
    }

    screen_width = ui_state_screen_width();
    if (screen_width <= 0) {
        screen_width = screen->width;
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

    scroll_beginning = screen->title_scroll_beginning;
    nc_cyclic_text_write(&scroll_buffer, directory.data, directory.len,
                         &scroll_beginning, scroll_width, separator,
                         SIZEOF(separator) - 1,
                         Config.header_text_scrolling);
    SB_APPEND(&screen->title_text, scroll_buffer.data, scroll_buffer.len);
    screen->title_scroll_beginning = scroll_beginning;
    sb_free(&scroll_buffer);
    return;
}

void
browser_screen_update_column_title(BrowserScreen *screen) {
    int32 width;

    if (screen == NULL) {
        return;
    }

    sb_clear(&screen->column_title_text);
    if ((screen->active_display_mode != NCM_DISPLAY_MODE_COLUMNS)
        || !Config.titles_visibility || (Config.columns.items == NULL)
        || (Config.columns.len <= 0) || (screen->main_height <= 2)) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    width = screen->width;
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
browser_screen_draw_header(BrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }

    browser_screen_update_title_text(screen);
    ncm_title_draw_header(screen->title_text.data, screen->title_text.len);
    screen->redraw_header = false;
    return;
}

void
browser_screen_set_display_mode(BrowserScreen *screen, enum DisplayMode mode) {
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
    browser_screen_update_column_title(screen);
    return;
}

static bool
browser_supported_extensions_contains(StrBuilderArray *extensions,
                                      char *extension,
                                      int32 extension_len) {
    for (int32 i = 0; i < extensions->len; i += 1) {
        StrBuilder *item;

        item = &extensions->items[i];
        if (STREQUAL(item->data, item->len, extension, extension_len)) {
            return true;
        }
    }
    return false;
}

int32
browser_screen_fetch_supported_extensions(BrowserScreen *screen,
                                          NcmMpdClient *client,
                                          NcmError *ncm_error) {
    NcmStringViewList strings = {0};
    StrBuilderArray extensions = {0};
    int32 status;

    if ((screen == NULL) || (client == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing extension state"));
    }

    status = ncm_mpd_client_get_supported_extensions(client, &strings,
                                                     ncm_error);
    if (status < 0) {
        ncm_mpd_string_list_destroy(&strings);
        return status;
    }

    str_builder_array_init(&extensions);
    for (int32 i = 0; i < strings.count; i += 1) {
        NcmStringView *string = &strings.items[i];
        StrBuilder buffer = {0};

        if ((string->len <= 0) || (string->data[0] != '.')) {
            sb_set(&buffer, STRLIT("."));
            SB_APPEND(&buffer, string->data, string->len);
        } else {
            sb_set(&buffer, string->data, string->len);
        }

        if (!browser_supported_extensions_contains(
            &extensions, buffer.data, buffer.len)) {
            str_builder_array_append_copy(&extensions, &buffer);
        }
        sb_free(&buffer);
    }

    str_builder_array_move(&screen->supported_extensions, &extensions);
    str_builder_array_destroy(&extensions);
    ncm_mpd_string_list_destroy(&strings);
    return ncm_error_ok(ncm_error);
}

void
browser_screen_clear_update_request(BrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->update_requested = false;
    nc_screen_clear_update_request(&screen->screen);
    return;
}

bool
browser_screen_is_in_root_directory(BrowserScreen *screen) {
    if (screen == NULL) {
        return true;
    }
    return browser_directory_is_root(screen->current_directory.data,
                                     screen->current_directory.len);
}

void
browser_screen_set_local(BrowserScreen *screen,
                         bool local_browser) {
    if (screen == NULL) {
        return;
    }
    screen->local_browser = local_browser;
    return;
}

bool
browser_screen_is_local(BrowserScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->local_browser;
}

int32
browser_screen_change_browse_mode(BrowserScreen *screen,
                                  NcmMpdClient *client, NcmError *ncm_error) {
    StrBuilder directory = {0};
    char *hostname;
    bool local_browser;
    int32 status;

    if ((screen == NULL) || (client == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser state"));
    }

    if (((hostname = ncm_mpd_client_hostname(client)) == NULL)
        || (hostname[0] != '/')) {
        return ncm_error_set_status(
            ncm_error, -EINVAL,
            STRLIT(
                "local browsing requires an MPD UNIX socket"));
    }

    local_browser = !screen->local_browser;
    if (local_browser) {
        sb_set(&directory, STRLIT("~"));
        status = ncm_path_expand_home(&directory, ncm_error);
        if (status < 0) {
            sb_free(&directory);
            return status;
        }
    } else {
        sb_set(&directory, STRLIT("/"));
    }

    browser_screen_set_current_directory(screen, directory.data, directory.len);
    browser_screen_set_local(screen, local_browser);
    browser_screen_clear(screen);
    browser_screen_request_update(screen);
    if (local_browser && (screen->supported_extensions.len <= 0)) {
        NcmError fetch_error;

        ncm_error_clear(&fetch_error);
        browser_screen_fetch_supported_extensions(screen, client, &fetch_error);
    }

    sb_free(&directory);
    return ncm_error_ok(ncm_error);
}

NcmMpdItem *
browser_screen_current_item(BrowserScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_browser_entry_menu_current(&screen->entries);
}

int32
browser_screen_current_song(BrowserScreen *screen,
                            NcmSong *song) {
    NcmMpdItem *item;

    if (song == NULL) {
        return -EINVAL;
    }
    item = browser_screen_current_item(screen);
    if ((item == NULL) || (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_SONG)) {
        return -ENOENT;
    }
    ncm_song_copy(song, ncm_mpd_item_song(item));
    return 0;
}

static bool
browser_local_path_has_supported_extension(
    BrowserScreen *screen, char *path, int32 path_len
) {
    int32 extension;

    extension = ncm_path_extension_start(path, path_len);
    if (extension <= 0) {
        return false;
    }
    return browser_supported_extensions_contains(
        &screen->supported_extensions, path + extension - 1,
        path_len - extension + 1);
}

static int32
browser_stat_local_path(char *path, int32 path_len, NcmFsStat *out,
                        NcmError *ncm_error) {
    struct stat statbuf;
    char message[256];
    int32 message_len;
    int32 code;

    out->size = 0;
    out->mtime = 0;
    out->type = NCM_FS_ENTRY_COUNT;
    out->exists = false;

    if (stat(path, &statbuf) != 0) {
        code = errno;
        if (code == ENOENT) {
            return ncm_error_ok(ncm_error);
        }
        message_len = SNPRINTF(message, "stat '%.*s': %s",
                                        path_len, path, strerror(code));
        return ncm_error_set_status(ncm_error, -code, message, message_len);
    }

    out->size = (int32)statbuf.st_size;
    out->mtime = (int32)statbuf.st_mtime;
    if (S_ISREG(statbuf.st_mode)) {
        out->type = NCM_FS_ENTRY_FILE;
    } else if (S_ISDIR(statbuf.st_mode)) {
        out->type = NCM_FS_ENTRY_DIRECTORY;
    } else if (S_ISLNK(statbuf.st_mode)) {
        out->type = NCM_FS_ENTRY_SYMLINK;
    }
    out->exists = true;
    return ncm_error_ok(ncm_error);
}

static void
browser_make_local_song(NcmSong *song, char *path, int32 path_len,
                        time_t mtime) {
#if defined(HAVE_TAGLIB_H)
    struct mpd_pair pair;
    struct mpd_song *mpd_song;
#endif

    ncm_song_set_uri(song, path, path_len);
    ncm_song_set_mtime(song, mtime);

#if defined(HAVE_TAGLIB_H)
    pair.name = "file";
    pair.value = path;
    if ((mpd_song = mpd_song_begin(&pair))) {
        if (ncm_tags_read_song(mpd_song) > 0) {
            ncm_song_from_mpd_song(song, mpd_song);
            ncm_song_set_mtime(song, mtime);
        }
        mpd_song_free(mpd_song);
    }
#endif

    return;
}

static int32
browser_collect_local_directory_songs(
    BrowserScreen *screen, NcmSongArray *songs, char *path,
    int32 path_len, NcmError *ncm_error
) {
    NcmFsDirectory directory = {0};
    NcmFsEntry entry = {0};
    int32 read_status;
    int32 status;

    status = ncm_fs_directory_open(&directory, path, path_len, ncm_error);
    if (status < 0) {
        return status;
    }

    ncm_fs_entry_init(&entry);
    while (true) {
        StrBuilder entry_path = {0};
        NcmFsStat stat = {0};

        read_status = ncm_fs_directory_read(&directory, &entry, ncm_error);
        if (read_status < 0) {
            status = read_status;
            break;
        }
        if (read_status == 0) {
            status = 0;
            break;
        }
        if (!Config.local_browser_show_hidden_files
            && (entry.name_len > 0) && (entry.name[0] == '.')) {
            continue;
        }

        ncm_fs_join(&entry_path, directory.path, directory.path_len,
                    entry.name, entry.name_len);

        status = browser_stat_local_path(
            entry_path.data, entry_path.len, &stat, ncm_error);
        if ((status == 0) && stat.exists
            && (stat.type == NCM_FS_ENTRY_DIRECTORY)) {
            status = browser_collect_local_directory_songs(
                screen, songs, entry_path.data, entry_path.len, ncm_error);
        } else if ((status == 0) && stat.exists
                   && (stat.type == NCM_FS_ENTRY_FILE)
                   && browser_local_path_has_supported_extension(
                       screen, entry_path.data, entry_path.len)) {
            NcmSong song = {0};

            browser_make_local_song(
                &song, entry_path.data, entry_path.len, (time_t)stat.mtime);
            ncm_song_array_append_copy(songs, &song);
            ncm_song_destroy(&song);
        }
        sb_free(&entry_path);
        if (status < 0) {
            break;
        }
    }
    ncm_fs_entry_destroy(&entry);
    ncm_fs_directory_close(&directory);
    return status;
}

static int32
browser_collect_item_songs(BrowserScreen *screen,
                           NcmSongArray *songs, NcmMpdItem *item) {
    NcmStringView path;
    int32 status;

    switch (ncm_mpd_item_kind(item)) {
    case NCM_MPD_ITEM_DIRECTORY: {
        NcmMpdSongList source = {0};
        NcmError ncm_error = {0};
        char *directory;

        ncm_directory_has_path_view(ncm_mpd_item_directory(item), &path);
        if (screen->local_browser) {
            ncm_error_clear(&ncm_error);
            return browser_collect_local_directory_songs(
                screen, songs, path.data, path.len, &ncm_error);
        }

        ncm_error_clear(&ncm_error);
        directory = path.data;
        if (path.len <= 0) {
            directory = "/";
        }
        status = ncm_mpd_client_get_directory_recursive(
            &global_mpd, directory, &source, &ncm_error);
        if (status >= 0) {
            for (int32 i = 0; i < source.count; i += 1) {
                ncm_song_array_append_copy(songs, &source.items[i]);
            }
        }
        ncm_mpd_song_list_destroy(&source);
        return status;
    }
    case NCM_MPD_ITEM_SONG:
        ncm_song_array_append_copy(songs, ncm_mpd_item_song(item));
        return 0;
    case NCM_MPD_ITEM_PLAYLIST:
    case NCM_MPD_ITEM_COUNT:
    default:
        return 0;
    }
}

int32
browser_screen_selected_songs(BrowserScreen *screen,
                              NcmSongArray *songs) {
    NcMenu *menu;
    int32 status;

    if (songs == NULL) {
        return -EINVAL;
    }
    ncm_song_array_clear(songs);
    if (screen == NULL) {
        return -EINVAL;
    }

    menu = browser_screen_menu(screen);
    if (nc_menu_is_empty(menu)) {
        return 0;
    }

    if (!nc_menu_has_selected(menu)) {
        return browser_collect_item_songs(
            screen, songs, nc_menu_current_item(menu));
    }

    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        status = browser_collect_item_songs(
            screen, songs, nc_menu_active_item_at(menu, i));
        if (status < 0) {
            return status;
        }
    }
    return 0;
}

static int32
browser_real_path(BrowserScreen *screen, NcmStringView path,
                  StrBuilder *real_path, NcmError *ncm_error) {
    sb_clear(real_path);
    if (screen->local_browser) {
        sb_set(real_path, path.data, path.len);
        return ncm_error_ok(ncm_error);
    }

    if (Config.mpd_music_dir_len <= 0) {
        return ncm_error_set_status(
            ncm_error, -ENOENT,
            STRLIT(
                "Proper mpd_music_dir variable has to be set in "
                "configuration file"));
    }
    ncm_fs_join(real_path, Config.mpd_music_dir, Config.mpd_music_dir_len,
                path.data, path.len);
    return ncm_error_ok(ncm_error);
}

static int32
browser_delete_path_recursive(char *path, int32 path_len,
                              NcmError *ncm_error) {
    NcmFsDirectory directory = {0};
    NcmFsEntry entry = {0};
    NcmFsStat stat = {0};
    int32 read_status;
    int32 status;

    status = ncm_fs_stat(path, path_len, &stat, ncm_error);
    if (status < 0) {
        return status;
    }
    if (!stat.exists) {
        return ncm_error_ok(ncm_error);
    }
    if (stat.type != NCM_FS_ENTRY_DIRECTORY) {
        return ncm_fs_unlink(path, path_len, ncm_error);
    }

    status = ncm_fs_directory_open(&directory, path, path_len, ncm_error);
    if (status < 0) {
        return status;
    }

    ncm_fs_entry_init(&entry);
    while (true) {
        StrBuilder child = {0};

        read_status = ncm_fs_directory_read(&directory, &entry, ncm_error);
        if (read_status < 0) {
            status = read_status;
            break;
        }
        if (read_status == 0) {
            status = 0;
            break;
        }

        ncm_fs_join(&child, directory.path, directory.path_len,
                    entry.name, entry.name_len);
        status = browser_delete_path_recursive(child.data, child.len,
                                               ncm_error);
        sb_free(&child);
        if (status < 0) {
            break;
        }
    }
    ncm_fs_entry_destroy(&entry);
    ncm_fs_directory_close(&directory);

    if (status < 0) {
        return status;
    }
    {
        char message[256];
        char *copy;
        int32 message_len;
        int32 code;

        copy = malloc2(path_len + 1);
        memcpy64(copy, path, path_len);
        copy[path_len] = '\0';
        if (rmdir(copy) != 0) {
            code = errno;
            if (code == ENOENT) {
                free2(copy, path_len + 1);
                return ncm_error_ok(ncm_error);
            }
            message_len = SNPRINTF(message, "rmdir '%.*s': %s",
                                   path_len, path, strerror(code));
            status = ncm_error_set_status(
                ncm_error, -code, message, message_len);
            free2(copy, path_len + 1);
            return status;
        }
        free2(copy, path_len + 1);
    }
    return ncm_error_ok(ncm_error);
}

int32
browser_screen_delete_items(BrowserScreen *screen,
                            NcmMpdClient *client,
                            NcmError *ncm_error) {
    NcMenu *menu;
    int32 count;
    int32 status;
    bool any_selected;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser state"));
    }
    if (!Config.allow_for_physical_item_deletion) {
        return ncm_error_set_status(ncm_error, -EPERM,
                                    STRLIT("physical deletion is forbidden"));
    }

    menu = browser_screen_menu(screen);
    if (nc_menu_is_empty(menu)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("no browser item selected"));
    }
    if (!screen->local_browser && (client == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }

    any_selected = nc_menu_has_selected(menu);
    count = nc_menu_item_count(menu);
    for (int32 i = 0; i < count; i += 1) {
        NcmMpdItem *item;

        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!any_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }

        item = nc_menu_active_item_at(menu, i);
        if (browser_screen_item_is_parent(item)) {
            return ncm_error_set_status(
                ncm_error, -EINVAL,
                STRLIT("deletion of parent directory is forbidden"));
        }

        switch (ncm_mpd_item_kind(item)) {
        case NCM_MPD_ITEM_DIRECTORY: {
            NcmStringView path = {0};
            StrBuilder real_path = {0};

            ncm_directory_has_path_view(ncm_mpd_item_directory(item),
                                        &path);
            status = browser_real_path(screen, path, &real_path, ncm_error);
            if (status == 0) {
                status = browser_delete_path_recursive(
                    real_path.data, real_path.len, ncm_error);
            }
            sb_free(&real_path);
            break;
        }
        case NCM_MPD_ITEM_SONG: {
            NcmStringView path = {0};
            StrBuilder real_path = {0};

            ncm_song_has_uri_view(ncm_mpd_item_song(item), 0, &path);
            status = browser_real_path(screen, path, &real_path, ncm_error);
            if (status == 0) {
                status = ncm_fs_unlink(
                    real_path.data, real_path.len, ncm_error);
            }
            sb_free(&real_path);
            break;
        }
        case NCM_MPD_ITEM_PLAYLIST: {
            NcmStringView path = {0};
            StrBuilder real_path = {0};

            if (client == NULL) {
                return ncm_error_set_status(
                    ncm_error, -EINVAL, STRLIT("missing MPD client"));
            }
            ncm_playlist_has_path_view(ncm_mpd_item_playlist(item),
                                       &path);

            status = ncm_mpd_client_delete_playlist(
                client, path.data, ncm_error);
            if (status == 0) {
                break;
            }
            if (ncm_mpd_client_server_error_code(client)
                != MPD_SERVER_ERROR_NO_EXIST) {
                return status;
            }

            status = browser_real_path(screen, path, &real_path, ncm_error);
            if (status == 0) {
                status = ncm_fs_unlink(
                    real_path.data, real_path.len, ncm_error);
            }
            sb_free(&real_path);
            break;
        }
        case NCM_MPD_ITEM_COUNT:
        default:
            status = ncm_error_set_status(
                ncm_error, -EINVAL, STRLIT("unknown browser item"));
            break;
        }
        if (status < 0) {
            return status;
        }
    }

    if (!screen->local_browser) {
        char *directory;

        directory = screen->current_directory.data;
        if (screen->current_directory.len <= 0) {
            directory = "/";
        }
        status = ncm_mpd_client_update_directory(
            client, directory, NULL, ncm_error);
        if (status < 0) {
            return status;
        }
    }

    browser_screen_request_update(screen);
    return ncm_error_ok(ncm_error);
}

static int32
browser_current_directory_item_path(BrowserScreen *screen,
                                    NcmStringView *path,
                                    NcmError *ncm_error) {
    NcmMpdItem *item;

    ncm_string_view_clear(path);

    if ((item = browser_screen_current_item(screen)) == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser item"));
    }
    if (browser_screen_item_is_parent(item)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("cannot rename parent directory"));
    }
    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_DIRECTORY) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("browser item is not a directory"));
    }
    ncm_directory_has_path_view(ncm_mpd_item_directory(item), path);
    return ncm_error_ok(ncm_error);
}

bool
browser_screen_has_current_directory_path(BrowserScreen *screen,
                                          NcmStringView *path) {
    NcmError ncm_error;

    if ((screen == NULL) || (path == NULL)) {
        return false;
    }
    ncm_error_clear(&ncm_error);
    return browser_current_directory_item_path(screen, path, &ncm_error) == 0;
}

static int32
browser_current_playlist_item_path(BrowserScreen *screen,
                                   NcmStringView *path,
                                   NcmError *ncm_error) {
    NcmMpdItem *item;

    ncm_string_view_clear(path);

    if ((item = browser_screen_current_item(screen)) == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser item"));
    }
    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_PLAYLIST) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("browser item is not a playlist"));
    }
    ncm_playlist_has_path_view(ncm_mpd_item_playlist(item), path);
    return ncm_error_ok(ncm_error);
}

bool
browser_screen_has_current_playlist_path(BrowserScreen *screen,
                                         NcmStringView *path) {
    NcmError ncm_error;

    if ((screen == NULL) || (path == NULL)) {
        return false;
    }
    ncm_error_clear(&ncm_error);
    return browser_current_playlist_item_path(screen, path, &ncm_error) == 0;
}

bool
browser_screen_can_rename_directory(BrowserScreen *screen) {
    NcmStringView path;
    NcmError ncm_error;

    if (screen == NULL) {
        return false;
    }

    ncm_error_clear(&ncm_error);
    return (browser_current_directory_item_path(screen, &path, &ncm_error) == 0)
        && (screen->local_browser || (Config.mpd_music_dir_len > 0));
}

bool
browser_screen_can_rename_playlist(
    BrowserScreen *screen
) {
    NcmStringView path;
    NcmError ncm_error;

    if (screen == NULL) {
        return false;
    }
    ncm_error_clear(&ncm_error);
    return browser_current_playlist_item_path(screen, &path, &ncm_error) == 0;
}

int32
browser_screen_rename_current_directory(BrowserScreen *screen,
                                        char *new_path, int32 new_path_len,
                                        NcmMpdClient *client,
                                        NcmError *ncm_error) {
    NcmStringView old_path;
    NcmStringView new_path_view;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser state"));
    }
    if ((new_path == NULL) || (new_path_len <= 0)) {
        return ncm_error_ok(ncm_error);
    }
    if ((status = browser_current_directory_item_path(
        screen, &old_path, ncm_error)) < 0) {
        return status;
    }
    if (STREQUAL(old_path.data, old_path.len, new_path, new_path_len)) {
        return ncm_error_ok(ncm_error);
    }

    new_path_view = ncm_string_view_make(new_path, new_path_len);
    {
        StrBuilder old_real_path = {0};
        StrBuilder new_real_path = {0};

        status = browser_real_path(screen, old_path, &old_real_path, ncm_error);
        if (status == 0) {
            status = browser_real_path(screen, new_path_view, &new_real_path,
                                       ncm_error);
        }
        if (status == 0) {
            status = ncm_fs_rename(
                old_real_path.data, old_real_path.len,
                new_real_path.data, new_real_path.len, ncm_error);
        }
        sb_free(&new_real_path);
        sb_free(&old_real_path);
    }
    if (status < 0) {
        return status;
    }

    if (!screen->local_browser) {
        StrBuilder shared;
        char *directory;

        if (client == NULL) {
            return ncm_error_set_status(ncm_error, -EINVAL,
                                        STRLIT("missing MPD client"));
        }

        shared = ncm_string_shared_directory(old_path.data, old_path.len,
                                             new_path_view.data,
                                             new_path_view.len);
        directory = shared.data;
        if (shared.len <= 0) {
            directory = "/";
        }
        status = ncm_mpd_client_update_directory(
            client, directory, NULL, ncm_error);
        sb_free(&shared);
        if (status < 0) {
            return status;
        }
    }

    browser_screen_request_update(screen);
    return ncm_error_ok(ncm_error);
}

int32
browser_screen_rename_current_playlist(
    BrowserScreen *screen, char *new_path, int32 new_path_len,
    NcmMpdClient *client, NcmError *ncm_error
) {
    NcmStringView old_path;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser state"));
    }
    if ((new_path == NULL) || (new_path_len <= 0)) {
        return ncm_error_ok(ncm_error);
    }
    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }
    if ((status = browser_current_playlist_item_path(
        screen, &old_path, ncm_error)) < 0) {
        return status;
    }
    if (STREQUAL(old_path.data, old_path.len, new_path, new_path_len)) {
        return ncm_error_ok(ncm_error);
    }

    if ((status = ncm_mpd_client_rename_playlist(
        client, old_path.data, new_path, ncm_error)) < 0) {
        return status;
    }

    browser_screen_request_update(screen);
    return ncm_error_ok(ncm_error);
}

static void
browser_add_parent_directory_item(BrowserScreen *screen) {
    NcmDirectory directory = {0};
    NcmMpdItem item;

    if (browser_screen_is_in_root_directory(screen)) {
        return;
    }

    sb_clear(&screen->scratch_buffer);
    SB_APPEND(&screen->scratch_buffer,
              screen->current_directory.data, screen->current_directory.len);
    SB_APPEND(&screen->scratch_buffer, "/..");

    ncm_mpd_item_init(&item);
    ncm_directory_set(&directory, screen->scratch_buffer.data,
                      screen->scratch_buffer.len, 0);
    ncm_mpd_item_set_directory(&item, &directory);
    browser_screen_add_item_move(screen, &item);
    ncm_mpd_item_destroy(&item);
    ncm_directory_destroy(&directory);
    return;
}

static void
browser_load_mpd_items(BrowserScreen *screen, NcmMpdItemArray *items) {
    NcMenu *menu = browser_screen_menu(screen);

    screen->title_scroll_beginning = 0;
    nc_menu_show_all_items(menu);
    browser_screen_clear(screen);
    browser_add_parent_directory_item(screen);
    for (int32 i = 0; i < items->len; i += 1) {
        nc_browser_entry_menu_add(browser_screen_entries(screen),
                                  &items->items[i]);
    }
    browser_screen_sort(screen);

    if (screen->filter_enabled) {
        nc_menu_apply_filter(menu);
    } else {
        nc_menu_show_all_items(menu);
    }
    browser_locate_last_directory(screen);
    screen->redraw_header = true;
    return;
}

static int32
browser_reload_from_local(BrowserScreen *screen, NcmError *ncm_error) {
    NcmFsDirectory directory = {0};
    NcmFsEntry entry = {0};
    NcMenu *menu;
    int32 read_status;
    int32 status;

    if (screen->current_directory.len <= 0) {
        sb_set(&screen->current_directory, STRLIT("~"));
        status = ncm_path_expand_home(&screen->current_directory, ncm_error);
        if (status < 0) {
            return status;
        }
    } else if (browser_path_is_parent_directory(
        screen->current_directory.data, screen->current_directory.len)) {
        status = browser_set_normalized_directory(
            screen, screen->current_directory.data,
            screen->current_directory.len);
        if (status < 0) {
            return ncm_error_set_status(ncm_error, status,
                                        STRLIT("invalid local directory"));
        }
    }
    ncm_error_ok(ncm_error);

    status = ncm_fs_directory_open(&directory, screen->current_directory.data,
                                   screen->current_directory.len, ncm_error);
    if (status < 0) {
        return status;
    }

    menu = browser_screen_menu(screen);
    screen->title_scroll_beginning = 0;
    nc_menu_show_all_items(menu);
    browser_screen_clear(screen);
    browser_add_parent_directory_item(screen);

    status = 0;
    ncm_fs_entry_init(&entry);
    while (status == 0) {
        StrBuilder path = {0};
        NcmFsStat stat = {0};

        read_status = ncm_fs_directory_read(&directory, &entry, ncm_error);
        if (read_status < 0) {
            status = read_status;
            break;
        }
        if (read_status == 0) {
            break;
        }
        if (!Config.local_browser_show_hidden_files
            && (entry.name_len > 0) && (entry.name[0] == '.')) {
            continue;
        }

        ncm_fs_join(&path, directory.path, directory.path_len,
                    entry.name, entry.name_len);

        status = browser_stat_local_path(
            path.data, path.len, &stat, ncm_error);
        if ((status < 0) || !stat.exists) {
            sb_free(&path);
            if (status < 0) {
                break;
            }
            continue;
        }

        if (stat.type == NCM_FS_ENTRY_DIRECTORY) {
            NcmDirectory local_directory = {0};
            NcmMpdItem item = {0};

            ncm_mpd_item_init(&item);
            ncm_directory_set(&local_directory, path.data, path.len,
                              (time_t)stat.mtime);
            ncm_mpd_item_set_directory(&item, &local_directory);
            browser_screen_add_item_move(screen, &item);
            ncm_mpd_item_destroy(&item);
            ncm_directory_destroy(&local_directory);
        } else if ((stat.type == NCM_FS_ENTRY_FILE)
                   && browser_local_path_has_supported_extension(
                       screen, path.data, path.len)) {
            NcmSong song = {0};
            NcmMpdItem item = {0};

            ncm_mpd_item_init(&item);
            browser_make_local_song(&song, path.data, path.len,
                                    (time_t)stat.mtime);
            ncm_mpd_item_set_song(&item, &song);
            browser_screen_add_item_move(screen, &item);
            ncm_mpd_item_destroy(&item);
            ncm_song_destroy(&song);
        }
        sb_free(&path);
    }
    ncm_fs_entry_destroy(&entry);
    ncm_fs_directory_close(&directory);

    if ((status == 0)
        && ((Config.browser_sort_mode == NCM_SORT_MODE_NONE)
            || (Config.browser_sort_mode == NCM_SORT_MODE_TYPE))) {
        Config.browser_sort_mode = NCM_SORT_MODE_NAME;
    }
    if (status == 0) {
        browser_screen_sort(screen);
    }

    if (status == 0) {
        if (screen->filter_enabled) {
            nc_menu_apply_filter(menu);
        } else {
            nc_menu_show_all_items(menu);
        }
        browser_locate_last_directory(screen);
        screen->redraw_header = true;
        browser_screen_clear_update_request(screen);
    }
    return status;
}

int32
browser_screen_locate_song(BrowserScreen *screen,
                           NcmSong *song, NcmMpdClient *client,
                           NcmError *ncm_error) {
    NcmStringView directory;
    bool local_browser;
    int32 status;

    if ((screen == NULL) || (song == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser song"));
    }
    if (!ncm_song_has_directory_view(song, 0, &directory)
        || (directory.len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("song directory is empty"));
    }

    browser_screen_clear_filter(screen);
    local_browser = !ncm_song_is_from_database(song);
    browser_screen_set_local(screen, local_browser);

    if (local_browser) {
        browser_screen_set_current_directory(screen, directory.data,
                                             directory.len);
        status = browser_reload_from_local(screen, ncm_error);
    } else {
        NcmMpdItemArray items = {0};
        StrBuilder path = {0};

        if (client == NULL) {
            return ncm_error_set_status(ncm_error, -EINVAL,
                                        STRLIT("missing MPD client"));
        }

        if (directory.len <= 0) {
            sb_set(&path, STRLIT("/"));
        } else {
            sb_set(&path, directory.data, directory.len);
        }
        status = ncm_mpd_client_get_directory_entries(
            client, path.data, &items, ncm_error);
        if (status == 0) {
            browser_screen_set_current_directory(screen, path.data, path.len);
            browser_load_mpd_items(screen, &items);
            browser_screen_clear_update_request(screen);
        }
        ncm_mpd_item_array_destroy(&items);
        sb_free(&path);

        if ((status < 0)
            && (ncm_mpd_client_server_error_code(client)
                == MPD_SERVER_ERROR_NO_EXIST)) {
            browser_screen_request_update(screen);
            return ncm_error_ok(ncm_error);
        }
    }

    if (status < 0) {
        return status;
    }

    {
        NcMenu *menu = browser_screen_menu(screen);

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            NcmMpdItem *item = nc_menu_active_item_at(menu, i);

            if ((ncm_mpd_item_kind(item) != NCM_MPD_ITEM_SONG)
                || !ncm_song_is_equal(ncm_mpd_item_song(item), song)) {
                continue;
            }
            nc_menu_highlight_position(menu, i, screen->main_height);
            break;
        }
    }
    return ncm_error_ok(ncm_error);
}

int32
browser_screen_enter_directory(BrowserScreen *screen) {
    NcmMpdItem *item;
    int32 status;

    if (screen == NULL) {
        return -EINVAL;
    }
    item = browser_screen_current_item(screen);
    if (item == NULL) {
        return -EINVAL;
    }
    if ((status = browser_enter_item(screen, item)) < 0) {
        return status;
    }
    browser_screen_request_update(screen);
    return 0;
}

int32
browser_screen_go_to_parent(BrowserScreen *screen) {
    if (screen == NULL) {
        return -EINVAL;
    }
    if (browser_screen_is_in_root_directory(screen)) {
        return -ENOENT;
    }
    browser_set_parent_of_directory(screen, screen->current_directory.data,
                                    screen->current_directory.len);
    browser_screen_request_update(screen);
    return 0;
}

int32
browser_screen_apply_filter(BrowserScreen *screen,
                            char *pattern, int32 pattern_len,
                            NcmError *ncm_error) {
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser screen"));
    }
    if (pattern_len <= 0) {
        browser_screen_clear_filter(screen);
        return ncm_error_ok(ncm_error);
    }
    if (pattern == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing filter pattern"));
    }
    if ((status = ncm_regex_compile(
        &screen->filter_regex, pattern, pattern_len,
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, ncm_error)) < 0) {
        return status;
    }
    sb_set(&screen->filter_constraint, pattern, pattern_len);
    screen->filter_enabled = true;
    browser_install_menu_callbacks(screen);
    nc_menu_apply_filter(browser_screen_menu(screen));
    return ncm_error_ok(ncm_error);
}

void
browser_screen_clear_filter(BrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    screen->filter_regex = (NcmRegex){0};
    sb_clear(&screen->filter_constraint);
    screen->filter_enabled = false;
    browser_install_menu_callbacks(screen);
    nc_menu_show_all_items(browser_screen_menu(screen));
    return;
}

static bool
browser_position_matches_search(NcMenu *menu, int32 pos, void *user) {
    BrowserSearchContext *context = user;

    return browser_item_matches(context->screen,
                                nc_menu_active_item_at(menu, pos),
                                context->regex, false);
}

int32
browser_screen_search(BrowserScreen *screen,
                      char *pattern, int32 pattern_len,
                      bool forward, bool wrap,
                      bool skip_current, NcmError *ncm_error) {
    BrowserSearchContext context;
    NcmRegex regex;
    NcMenu *menu;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing browser screen"));
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing search pattern"));
    }

    regex = (NcmRegex){0};
    status = ncm_regex_compile(&regex, pattern, pattern_len,
                               NCM_REGEX_LITERAL_CASE_INSENSITIVE,
                               ncm_error);
    if (status < 0) {
        ncm_regex_destroy(&regex);
        return status;
    }
    sb_set(&screen->search_constraint, pattern, pattern_len);

    menu = browser_screen_menu(screen);
    context.screen = screen;
    context.regex = &regex;
    status = nc_menu_search_selectable(menu, screen->main_height, forward,
                                       wrap, skip_current,
                                       browser_position_matches_search,
                                       &context, NULL);
    ncm_regex_destroy(&regex);
    if (status == 0) {
        return 1;
    }
    if (status == -NCM_ERROR_NOT_FOUND) {
        return 0;
    }
    return status;
}

bool
browser_screen_item_is_parent(NcmMpdItem *item) {
    NcmStringView view;

    if (item == NULL) {
        return false;
    }
    if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_DIRECTORY) {
        return false;
    }
    if (!ncm_directory_has_path_view(ncm_mpd_item_directory(item), &view)) {
        return false;
    }
    return browser_path_is_parent_directory(view.data, view.len);
}

void
browser_screen_request_update(BrowserScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->update_requested = true;
    nc_screen_request_update(&screen->screen);
    return;
}

static void
browser_display(BrowserScreen *browser) {
    browser_screen_update_column_title(browser);
    nc_menu_refresh(browser_screen_menu(browser), &browser->window,
                    browser->width, browser->main_height);
    return;
}

static void
browser_switch_to(NcScreen *screen) {
    BrowserScreen *browser = browser_from_screen(screen);

    nc_screen_switcher_finish_switch(screen);
    if (nc_menu_is_empty(browser_screen_menu(browser))) {
        browser_screen_request_update(browser);
    }
    browser->redraw_header = true;
    browser_screen_draw_header(browser);
    return;
}

static void
browser_resize(NcScreen *screen) {
    BrowserScreen *browser = browser_from_screen(screen);
    int32 x = browser->start_x;
    int32 width = browser->width;

    nc_screen_switcher_get_resize_params(screen, &x, &width, true);
    browser->start_x = x;
    browser->width = width;
    browser->main_start_y = ui_state_main_start_y();
    browser->main_height = ui_state_main_height();
    nc_window_move_to(&browser->window, x, browser->main_start_y);
    nc_window_resize(&browser->window, width, browser->main_height);
    browser_screen_update_column_title(browser);
    browser->redraw_header = true;
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
browser_title(NcScreen *screen) {
    BrowserScreen *browser = browser_from_screen(screen);
    browser_screen_update_title_text(browser);
    return browser->title_text.data;
}

static void
browser_update(NcScreen *screen) {
    BrowserScreen *browser = browser_from_screen(screen);
    NcmError ncm_error = {0};

    if (browser->update_requested) {
        ncm_error_clear(&ncm_error);
        if (browser_screen_is_local(browser)) {
            browser_reload_from_local(browser, &ncm_error);
        } else {
            int32 status = 0;

            if (browser->current_directory.len <= 0) {
                browser_screen_set_current_directory(browser, STRLIT("/"));
            } else if (browser_path_is_parent_directory(
                browser->current_directory.data,
                browser->current_directory.len)) {
                status = browser_set_normalized_directory(
                    browser, browser->current_directory.data,
                    browser->current_directory.len);
            }
            if (status < 0) {
                ncm_error_set_status(
                    &ncm_error, status, STRLIT("invalid browser directory"));
            }

            while (status >= 0) {
                NcmMpdItemArray items = {0};

                status = ncm_mpd_client_get_directory_entries(
                    &global_mpd, browser->current_directory.data,
                    &items, &ncm_error);
                if (status == 0) {
                    browser_load_mpd_items(browser, &items);
                    browser_screen_clear_update_request(browser);
                    ncm_mpd_item_array_destroy(&items);
                    break;
                }
                ncm_mpd_item_array_destroy(&items);

                if (ncm_mpd_client_server_error_code(&global_mpd)
                    != MPD_SERVER_ERROR_NO_EXIST) {
                    break;
                }
                status = browser_screen_go_to_parent(browser);
            }
        }
    }
    if (browser->redraw_header) {
        browser_screen_draw_header(browser);
    }
    return;
}

static void
browser_mouse_scroll(BrowserScreen *screen, enum NcScroll where) {
    for (int32 i = 0; i < screen->lines_scrolled; i += 1) {
        nc_menu_scroll_selectable(browser_screen_menu(screen),
                                  screen->main_height, where);
    }
    return;
}

static void
browser_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    BrowserScreen *browser = browser_from_screen(screen);
    NcMenu *menu = browser_screen_menu(browser);
    NcWindow *window = browser_screen_window(browser);
    int32 x = event.x;
    int32 y = event.y;

    if (nc_menu_is_empty(menu)) {
        return;
    }

    if (!nc_window_has_coords(window, &x, &y)) {
        return;
    }

    if ((event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) != 0) {
        if (nc_menu_goto_selectable(menu, y) < 0) {
            return;
        }
        if ((event.bstate & BUTTON1_PRESSED) != 0) {
            nc_menu_activate_current(menu);
        }
        return;
    }

    if ((event.bstate & BUTTON5_PRESSED) != 0) {
        if (browser->mouse_list_scroll_whole_page) {
            browser_scroll(screen, NC_SCROLL_PAGE_DOWN);
        } else {
            browser_mouse_scroll(browser, NC_SCROLL_DOWN);
        }
    } else if ((event.bstate & BUTTON4_PRESSED) != 0) {
        if (browser->mouse_list_scroll_whole_page) {
            browser_scroll(screen, NC_SCROLL_PAGE_UP);
        } else {
            browser_mouse_scroll(browser, NC_SCROLL_UP);
        }
    }
    return;
}

static void
browser_locate_last_directory(BrowserScreen *screen) {
    NcmStringView target;
    NcmStringView path;
    NcMenu *menu;

    target = ncm_string_view_make(
        screen->last_highlighted_directory.data,
        screen->last_highlighted_directory.len);
    if (target.len <= 0) {
        return;
    }

    menu = browser_screen_menu(screen);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmMpdItem *item = nc_menu_active_item_at(menu, i);

        if (ncm_mpd_item_kind(item) != NCM_MPD_ITEM_DIRECTORY) {
            continue;
        }
        ncm_directory_has_path_view(ncm_mpd_item_directory(item), &path);
        if (browser_string_views_matches(path, target)) {
            nc_menu_highlight_position(menu, i, screen->main_height);
            return;
        }
    }
    return;
}

static bool
browser_string_views_matches(NcmStringView left,
                             NcmStringView right) {
    return STREQUAL(left.data, left.len, right.data, right.len);
}

#endif /* NCMPCPP_NC_BROWSER_C */
