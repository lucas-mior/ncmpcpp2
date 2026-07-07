#include "screens/nc_search_engine.h"

#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "screens/screen_switcher.h"

static NativeSearchEngineScreen *native_search_from_screen(NcScreen *screen);
static NcWindow *native_search_active_window(NcScreen *screen);
static void native_search_refresh(NcScreen *screen);
static void native_search_refresh_window(NcScreen *screen);
static void native_search_scroll(NcScreen *screen, enum NcScroll where);
static void native_search_switch_to(NcScreen *screen);
static void native_search_resize(NcScreen *screen);
static char *native_search_title(NcScreen *screen);
static void native_search_update(NcScreen *screen);
static bool native_search_is_lockable(NcScreen *screen);
static bool native_search_is_mergable(NcScreen *screen);
static void native_search_destroy_callback(NcScreen *screen);
static bool native_search_filter_row(NcMenu *menu, void *item, void *user);
static bool native_search_row_matches(NcSearchRow *row, NcmRegex *regex);
static bool native_search_row_label(NcSearchRow *row, NcmStringView *view);
static bool native_search_copy_song_at(NativeSearchEngineScreen *screen,
                                       NcmSongArray *songs, int64 pos);

static NcScreenCallbacks native_search_callbacks = {
    .active_window = native_search_active_window,
    .refresh = native_search_refresh,
    .refresh_window = native_search_refresh_window,
    .scroll = native_search_scroll,
    .switch_to = native_search_switch_to,
    .resize = native_search_resize,
    .title = native_search_title,
    .update = native_search_update,
    .is_lockable = native_search_is_lockable,
    .is_mergable = native_search_is_mergable,
    .destroy = native_search_destroy_callback,
};

void
native_search_engine_screen_init(NativeSearchEngineScreen *screen,
                                 int64 start_x, int64 width,
                                 int64 main_start_y, int64 main_height,
                                 NcColor color, NcBorder border) {
    nc_search_row_menu_init(&screen->rows);
    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_buffer_init(&screen->constraints[i]);
    }
    ncm_buffer_init(&screen->search_constraint);
    ncm_regex_init(&screen->filter_regex);

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->lines_scrolled = 1;
    screen->mouse_list_scroll_whole_page = false;
    screen->match_to_pattern = false;
    screen->filter_enabled = false;
    screen->registered = false;

    nc_screen_init(&screen->screen, native_search_callbacks, screen,
                   NC_SCREEN_TYPE_SEARCH_ENGINE);
    return;
}

void
native_search_engine_screen_destroy(NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_buffer_destroy(&screen->search_constraint);
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_buffer_destroy(&screen->constraints[i]);
    }
    nc_window_destroy(&screen->window);
    nc_search_row_menu_destroy(&screen->rows);
    return;
}

NcScreen *
native_search_engine_screen_base(NativeSearchEngineScreen *screen) {
    return &screen->screen;
}

NcSearchRowMenu *
native_search_engine_screen_rows(NativeSearchEngineScreen *screen) {
    return &screen->rows;
}

NcMenu *
native_search_engine_screen_menu(NativeSearchEngineScreen *screen) {
    return nc_search_row_menu_base(&screen->rows);
}

NcWindow *
native_search_engine_screen_window(NativeSearchEngineScreen *screen) {
    return &screen->window;
}

void
native_search_engine_screen_set_geometry(NativeSearchEngineScreen *screen,
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
native_search_engine_screen_clear(NativeSearchEngineScreen *screen) {
    nc_menu_clear_items(native_search_engine_screen_menu(screen));
    return;
}

void
native_search_engine_screen_reset(NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    native_search_engine_screen_clear(screen);
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_buffer_clear(&screen->constraints[i]);
    }
    native_search_engine_screen_clear_filter(screen);
    return;
}

bool
native_search_engine_screen_add_song_copy(NativeSearchEngineScreen *screen,
                                          NcmSong *song) {
    NcSearchRow row;

    if (screen == NULL || song == NULL) {
        return false;
    }
    nc_search_row_init(&row);
    row.is_song = true;
    if (!ncm_song_copy(&row.song, song)) {
        nc_search_row_destroy(&row);
        return false;
    }
    nc_search_row_menu_add(&screen->rows, &row);
    nc_search_row_destroy(&row);
    return true;
}

bool
native_search_engine_screen_add_buffer(NativeSearchEngineScreen *screen,
                                       NcBuffer *buffer) {
    NcSearchRow row;

    if (screen == NULL || buffer == NULL) {
        return false;
    }
    nc_search_row_init(&row);
    row.is_song = false;
    nc_buffer_copy(&row.buffer, buffer);
    nc_search_row_menu_add(&screen->rows, &row);
    nc_search_row_destroy(&row);
    return true;
}

bool
native_search_engine_screen_set_constraint(NativeSearchEngineScreen *screen,
                                           int32 idx, char *data,
                                           int32 data_len) {
    if (screen == NULL || idx < 0
        || idx >= NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT) {
        return false;
    }
    return ncm_buffer_set(&screen->constraints[idx], data, data_len);
}

NcmStringView
native_search_engine_screen_constraint(NativeSearchEngineScreen *screen,
                                       int32 idx) {
    if (screen == NULL || idx < 0
        || idx >= NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(screen->constraints[idx].data,
                                screen->constraints[idx].len);
}

bool
native_search_engine_screen_build_query(NativeSearchEngineScreen *screen,
                                        NcmBuffer *query) {
    bool first;

    if (screen == NULL || query == NULL) {
        return false;
    }

    ncm_buffer_clear(query);
    first = true;
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        if (screen->constraints[i].len <= 0) {
            continue;
        }
        if (!first) {
            ncm_buffer_append(query, (char *)" ", 1);
        }
        ncm_buffer_append(query, screen->constraints[i].data,
                          screen->constraints[i].len);
        first = false;
    }
    return true;
}

bool
native_search_engine_screen_current_song(NativeSearchEngineScreen *screen,
                                         NcmSong *song) {
    NcSearchRow *row;

    if (screen == NULL || song == NULL) {
        return false;
    }
    row = nc_search_row_menu_current(&screen->rows);
    if (row == NULL || !row->is_song) {
        return false;
    }
    return ncm_song_copy(song, &row->song);
}

bool
native_search_engine_screen_selected_songs(NativeSearchEngineScreen *screen,
                                           NcmSongArray *songs) {
    NcMenu *menu;
    bool any_selected;

    if (screen == NULL || songs == NULL) {
        return false;
    }
    menu = native_search_engine_screen_menu(screen);
    any_selected = nc_menu_has_selected(menu);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!native_search_copy_song_at(screen, songs, i)) {
            return false;
        }
    }
    return true;
}

bool
native_search_engine_screen_apply_filter(NativeSearchEngineScreen *screen,
                                         char *pattern, int32 pattern_len,
                                         NcmError *error) {
    NcMenuDisplayCallbacks callbacks = {0};

    if (screen == NULL) {
        return false;
    }
    if (pattern_len <= 0) {
        native_search_engine_screen_clear_filter(screen);
        return true;
    }
    if (!ncm_regex_compile(&screen->filter_regex, pattern, pattern_len,
                           NCM_REGEX_LITERAL_CASE_INSENSITIVE, error)) {
        return false;
    }
    if (!ncm_buffer_set(&screen->search_constraint, pattern, pattern_len)) {
        return false;
    }
    callbacks.filter = native_search_filter_row;
    callbacks.user = screen;
    nc_menu_set_display_callbacks(native_search_engine_screen_menu(screen),
                                  callbacks);
    screen->filter_enabled = true;
    nc_menu_apply_filter(native_search_engine_screen_menu(screen));
    return true;
}

void
native_search_engine_screen_clear_filter(NativeSearchEngineScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_regex_init(&screen->filter_regex);
    ncm_buffer_clear(&screen->search_constraint);
    screen->filter_enabled = false;
    nc_menu_set_display_callbacks(native_search_engine_screen_menu(screen),
                                  callbacks);
    nc_menu_show_all_items(native_search_engine_screen_menu(screen));
    return;
}

bool
native_search_engine_screen_search(NativeSearchEngineScreen *screen,
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

    menu = native_search_engine_screen_menu(screen);
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
        if (native_search_row_matches(nc_menu_active_item_at(menu, pos),
                                      &regex)) {
            result = nc_menu_goto_selectable(menu, pos);
            break;
        }
    }

    ncm_regex_destroy(&regex);
    return result;
}

static NativeSearchEngineScreen *
native_search_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
native_search_active_window(NcScreen *screen) {
    return &native_search_from_screen(screen)->window;
}

static void
native_search_refresh(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    nc_menu_prepare_refresh(native_search_engine_screen_menu(search),
                            search->main_height, NULL, NULL);
    return;
}

static void
native_search_refresh_window(NcScreen *screen) {
    native_search_refresh(screen);
    return;
}

static void
native_search_scroll(NcScreen *screen, enum NcScroll where) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    nc_menu_scroll_selectable(native_search_engine_screen_menu(search),
                              search->main_height, where);
    return;
}

static void
native_search_switch_to(NcScreen *screen) {
    (void)nc_screen_switcher_switch_to(screen,
                                       nc_screen_has_to_be_resized(screen));
    return;
}

static void
native_search_resize(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    native_search_engine_screen_set_geometry(search, search->start_x,
                                             search->width,
                                             search->main_start_y,
                                             search->main_height);
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
native_search_title(NcScreen *screen) {
    (void)screen;
    return (char *)"Search engine";
}

static void
native_search_update(NcScreen *screen) {
    nc_screen_clear_update_request(screen);
    return;
}

static bool
native_search_is_lockable(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
native_search_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
native_search_destroy_callback(NcScreen *screen) {
    native_search_engine_screen_destroy(native_search_from_screen(screen));
    return;
}

static bool
native_search_filter_row(NcMenu *menu, void *item, void *user) {
    NativeSearchEngineScreen *screen;

    (void)menu;
    screen = user;
    return native_search_row_matches(item, &screen->filter_regex);
}

static bool
native_search_row_matches(NcSearchRow *row, NcmRegex *regex) {
    NcmStringView view;

    if (!native_search_row_label(row, &view)) {
        return false;
    }
    return ncm_regex_search(regex, view.data, view.len);
}

static bool
native_search_row_label(NcSearchRow *row, NcmStringView *view) {
    if (row == NULL || view == NULL) {
        return false;
    }
    if (row->is_song) {
        return ncm_song_uri_view(&row->song, 0, view);
    }
    *view = ncm_string_view_make(row->buffer.data, row->buffer.len);
    return true;
}

static bool
native_search_copy_song_at(NativeSearchEngineScreen *screen,
                           NcmSongArray *songs, int64 pos) {
    NcSearchRow *row;

    row = nc_menu_active_item_at(native_search_engine_screen_menu(screen),
                                 pos);
    if (row == NULL || !row->is_song) {
        return true;
    }
    return ncm_song_array_append_copy(songs, &row->song);
}
