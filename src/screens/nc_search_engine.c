#if !defined(NCMPCPP_NC_SEARCH_ENGINE_C)
#define NCMPCPP_NC_SEARCH_ENGINE_C

#include "screens/nc_search_engine.h"

#include <errno.h>

#include "c/ncm_base.h"
#include "c/ncm_comparators.h"
#include "c/ncm_display.h"
#include "c/ncm_string.h"
#include "cbase/utf8.c"
#include "settings.h"
#include "ui_state.h"

static NcWindow *native_search_active_window(NcScreen *screen);
static void native_search_refresh(NcScreen *screen);
static void native_search_refresh_window(NcScreen *screen);
static void native_search_scroll(NcScreen *screen, enum NcScroll where);
static void native_search_switch_to(NcScreen *screen);
static void native_search_resize(NcScreen *screen);
static char *native_search_title(NcScreen *screen);
static void native_search_update(NcScreen *screen);
static void native_search_mouse_button_pressed(NcScreen *screen,
                                               MEVENT event);
static bool native_search_can_run_current(NcScreen *screen);
static bool native_search_run_current(NcScreen *screen);
static bool native_search_is_lockable(NcScreen *screen);
static bool native_search_is_mergable(NcScreen *screen);
static void native_search_destroy_callback(NcScreen *screen);
static void native_search_draw_row(NcMenu *menu, NcWindow *window,
                                   void *item, int64 pos, void *user);
static NcMenuDisplayCallbacks native_search_display_callbacks(
    NativeSearchEngineScreen *screen, bool filtering);
static bool native_search_row_matches(NativeSearchEngineScreen *screen,
                                      NcSearchRow *row, NcmRegex *regex);
static bool native_search_find_position(NcMenu *menu, int64 pos,
                                        void *user);
static bool native_search_row_label(NativeSearchEngineScreen *screen,
                                    NcSearchRow *row, NcmStringView *view);
static void native_search_draw_classic_song(
    NativeSearchEngineScreen *screen, NcMenu *menu, NcWindow *window,
    NcmSong *song, int64 pos);
static void native_search_draw_columns_song(
    NativeSearchEngineScreen *screen, NcMenu *menu, NcWindow *window,
    NcmSong *song, int64 pos);
static bool native_search_format_columns(
    NativeSearchEngineScreen *screen, NcmSong *song, NcBuffer *buffer,
    int32 list_width);
static int32 native_search_screen_width(NativeSearchEngineScreen *screen);
static int32 native_search_menu_prefix_width(NcMenu *menu, int64 pos);
static int32 native_search_menu_suffix_width(NcMenu *menu, int64 pos);
static int32 native_search_buffer_width(NcBuffer *buffer);
static bool native_search_copy_song_at(NativeSearchEngineScreen *screen,
                                       NcmSongArray *songs, int64 pos);
static bool native_search_insert_buffer_with_flags(
    NativeSearchEngineScreen *screen, int64 pos, NcBuffer *buffer,
    uint32 flags);
static void native_search_append_constraint_row(
    NativeSearchEngineScreen *screen, int32 idx);
static bool native_search_set_buffer_row(
    NativeSearchEngineScreen *screen, int32 pos, NcBuffer *buffer);
static void native_search_build_constraint_row(
    NativeSearchEngineScreen *screen, int32 idx, NcBuffer *buffer);
static void native_search_build_search_source_row(
    NativeSearchEngineScreen *screen, NcBuffer *buffer);
static void native_search_build_search_mode_row(
    NativeSearchEngineScreen *screen, NcBuffer *buffer);
static void native_search_append_format(NcBuffer *buffer,
                                        enum NcFormat format);
static void native_search_append_tag_value(NcBuffer *buffer,
                                           NcmBuffer *value);
static void native_search_print_buffer(NcWindow *window, NcBuffer *buffer);
static void native_search_mouse_scroll(NativeSearchEngineScreen *screen,
                                       enum NcScroll where);
static void native_search_print_error(NativeSearchEngineScreen *screen,
                                      NcmError *error);
static bool native_search_has_constraints(NativeSearchEngineScreen *screen);
static bool native_search_collect_database_results(
    NativeSearchEngineScreen *screen, NcmMpdClient *client,
    NcmSongArray *songs, NcmError *error);
static bool native_search_add_database_constraints(
    NativeSearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *error);
static bool native_search_collect_local_results(
    NativeSearchEngineScreen *screen, NcmSongArray *source,
    NcmSongArray *songs, NcmError *error);
static bool native_search_song_matches(
    NativeSearchEngineScreen *screen, NcmSong *song, NcmRegex *regexes);
static bool native_search_song_field_matches(
    NativeSearchEngineScreen *screen, NcmSong *song, int32 field,
    NcmRegex *regex);
static bool native_search_song_any_matches(
    NativeSearchEngineScreen *screen, NcmSong *song, NcmRegex *regex);
static bool native_search_song_field_view(NcmSong *song, int32 field,
                                          NcmStringView *view);
static bool native_search_append_result_rows(
    NativeSearchEngineScreen *screen, NcmSongArray *songs);

typedef struct NativeSearchFindContext {
    NativeSearchEngineScreen *screen;
    NcmRegex *regex;
} NativeSearchFindContext;

static char *native_search_constraint_names[] = {
    "Any",
    "Artist",
    "Album Artist",
    "Title",
    "Album",
    "Filename",
    "Composer",
    "Performer",
    "Genre",
    "Date",
    "Comment",
};

static int32 native_search_constraint_name_lengths[] = {
    STRLIT_LEN("Any"),
    STRLIT_LEN("Artist"),
    STRLIT_LEN("Album Artist"),
    STRLIT_LEN("Title"),
    STRLIT_LEN("Album"),
    STRLIT_LEN("Filename"),
    STRLIT_LEN("Composer"),
    STRLIT_LEN("Performer"),
    STRLIT_LEN("Genre"),
    STRLIT_LEN("Date"),
    STRLIT_LEN("Comment"),
};

static char *native_search_mode_names[] = {
    "Match if tag contains searched phrase (no regexes)",
    "Match if tag contains searched phrase (regexes supported)",
    "Match only if both values are the same",
};

static char native_search_empty_string[] = "";

static NcScreenCallbacks native_search_callbacks = {
    .active_window = native_search_active_window,
    .refresh = native_search_refresh,
    .refresh_window = native_search_refresh_window,
    .scroll = native_search_scroll,
    .switch_to = native_search_switch_to,
    .resize = native_search_resize,
    .title = native_search_title,
    .update = native_search_update,
    .mouse_button_pressed = native_search_mouse_button_pressed,
    .can_run_current = native_search_can_run_current,
    .run_current = native_search_run_current,
    .is_lockable = native_search_is_lockable,
    .is_mergable = native_search_is_mergable,
    .destroy = native_search_destroy_callback,
};

void
native_search_engine_screen_init(NativeSearchEngineScreen *screen,
                                 int64 start_x, int64 width,
                                 int64 main_start_y, int64 main_height,
                                 NcColor color, NcBorder border) {
    NcMenu *menu;

    nc_search_row_menu_init(&screen->rows);
    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_buffer_init(&screen->constraints[i]);
    }
    ncm_buffer_init(&screen->filter_constraint);
    ncm_buffer_init(&screen->search_constraint);
    ncm_buffer_init(&screen->row_text);
    ncm_buffer_init(&screen->title);
    ncm_buffer_init(&screen->column_title);
    ncm_buffer_append(&screen->title, STRLIT_ARGS("Search engine"));
    ncm_regex_init(&screen->filter_regex);

    screen->hooks = (NativeSearchEngineHooks){0};
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->lines_scrolled = 1;
    screen->result_count = 0;
    screen->search_mode = NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL;
    screen->search_in_database = true;
    screen->mouse_list_scroll_whole_page = false;
    screen->match_to_pattern = false;
    screen->filter_enabled = false;
    screen->prepared = false;
    screen->result_rows_present = false;
    screen->constraints_locked = false;
    screen->registered = false;

    nc_screen_init(&screen->screen, native_search_callbacks, screen,
                   NC_SCREEN_TYPE_SEARCH_ENGINE);
    menu = native_search_engine_screen_menu(screen);
    nc_menu_set_display_callbacks(
        menu, native_search_display_callbacks(screen, false));
    nc_menu_set_selected_prefix(menu, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(menu, &Config.selected_item_suffix);
    nc_menu_set_highlight_prefix(menu, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(menu, &Config.current_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    native_search_engine_screen_update_column_title(screen);
    return;
}

void
native_search_engine_screen_destroy(NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_buffer_destroy(&screen->filter_constraint);
    ncm_buffer_destroy(&screen->row_text);
    ncm_buffer_destroy(&screen->title);
    ncm_buffer_destroy(&screen->column_title);
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
    int64 window_height;
    int64 window_start_y;

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;

    window_start_y = main_start_y;
    window_height = main_height;
    if (screen->column_title.len > 0) {
        window_start_y += 2;
        window_height -= 2;
    }
    if (window_height < 1) {
        window_height = 1;
    }
    nc_window_move_to(&screen->window, start_x, window_start_y);
    nc_window_resize(&screen->window, width, window_height);
    native_search_engine_screen_update_column_title(screen);
    return;
}

void
native_search_engine_screen_set_mouse_config(
    NativeSearchEngineScreen *screen, int64 lines_scrolled,
    bool whole_page) {
    if (screen == NULL) {
        return;
    }
    if (lines_scrolled <= 0) {
        lines_scrolled = 1;
    }
    screen->lines_scrolled = lines_scrolled;
    screen->mouse_list_scroll_whole_page = whole_page;
    return;
}

void
native_search_engine_screen_set_display_mode(
    NativeSearchEngineScreen *screen, enum DisplayMode mode) {
    if ((screen == NULL) || ((mode != NCM_DISPLAY_MODE_CLASSIC)
        && (mode != NCM_DISPLAY_MODE_COLUMNS))) {
        return;
    }
    Config.search_engine_display_mode = mode;
    native_search_engine_screen_update_column_title(screen);
    return;
}

void
native_search_engine_screen_clear(NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(native_search_engine_screen_menu(screen));
    screen->prepared = false;
    screen->result_rows_present = false;
    screen->result_count = 0;
    screen->constraints_locked = false;
    return;
}

char *
native_search_engine_constraint_name(int32 idx) {
    if ((idx < 0) || (idx >= NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT)) {
        return NULL;
    }
    return native_search_constraint_names[idx];
}

char *
native_search_engine_search_mode_name(
    enum NativeSearchEngineSearchMode mode) {
    if ((mode < NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL)
        || (mode >= NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST)) {
        return native_search_mode_names[0];
    }
    return native_search_mode_names[mode];
}

bool
native_search_engine_screen_constraints_locked(
    NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->constraints_locked;
}

bool
native_search_engine_screen_format_song_text(
    NativeSearchEngineScreen *screen, NcmSong *song, NcmBuffer *text) {
    NcBuffer formatted;
    bool result;

    if ((screen == NULL) || (song == NULL) || (text == NULL)) {
        return false;
    }

    nc_buffer_init(&formatted);
    if (Config.search_engine_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        result = native_search_format_columns(
            screen, song, &formatted, native_search_screen_width(screen));
    } else {
        ncm_display_song_row(&formatted, &Config.song_list_format, song,
                             NCM_FORMAT_FLAG_ALL);
        result = true;
    }
    if (result) {
        result = ncm_buffer_set(text, formatted.data, formatted.len);
    }
    nc_buffer_destroy(&formatted);
    return result;
}

void
native_search_engine_screen_update_column_title(
    NativeSearchEngineScreen *screen) {
    int32 list_width;

    if (screen == NULL) {
        return;
    }

    ncm_buffer_clear(&screen->column_title);
    if ((Config.search_engine_display_mode != NCM_DISPLAY_MODE_COLUMNS)
        || !Config.titles_visibility || (Config.columns.items == NULL)
        || (Config.columns.len <= 0) || (screen->main_height <= 2)) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    list_width = native_search_screen_width(screen);
    if (list_width <= 0) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    ncm_display_column_title(&screen->column_title, Config.columns.items,
                             Config.columns.len, list_width);
    nc_window_set_title(&screen->window,
                        screen->column_title.data,
                        screen->column_title.len);
    return;
}

void
native_search_engine_screen_prepare_static_rows(
    NativeSearchEngineScreen *screen) {
    NcBuffer buffer;

    if (screen == NULL) {
        return;
    }

    native_search_engine_screen_clear(screen);
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        native_search_append_constraint_row(screen, i);
    }

    nc_buffer_init(&buffer);
    native_search_engine_screen_add_buffer_with_flags(
        screen, &buffer, NC_MENU_ITEM_SEPARATOR);

    native_search_build_search_source_row(screen, &buffer);
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);

    native_search_build_search_mode_row(screen, &buffer);
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);

    nc_buffer_clear(&buffer);
    native_search_engine_screen_add_buffer_with_flags(
        screen, &buffer, NC_MENU_ITEM_SEPARATOR);

    nc_buffer_append_data(&buffer, STRLIT_ARGS("Search"));
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);

    nc_buffer_clear(&buffer);
    nc_buffer_append_data(&buffer, STRLIT_ARGS("Reset"));
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);
    nc_buffer_destroy(&buffer);

    nc_menu_reset(native_search_engine_screen_menu(screen));
    screen->prepared = true;
    screen->result_rows_present = false;
    screen->result_count = 0;
    screen->constraints_locked = false;
    return;
}

bool
native_search_engine_screen_update_constraint_row(
    NativeSearchEngineScreen *screen, int32 idx) {
    NcBuffer buffer;
    bool result;

    if ((screen == NULL) || !screen->prepared || (idx < 0)
        || (idx >= NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT)) {
        return false;
    }

    nc_buffer_init(&buffer);
    native_search_build_constraint_row(screen, idx, &buffer);
    result = native_search_set_buffer_row(screen, idx, &buffer);
    nc_buffer_destroy(&buffer);
    return result;
}

bool
native_search_engine_screen_update_search_source_row(
    NativeSearchEngineScreen *screen) {
    NcBuffer buffer;
    bool result;

    if ((screen == NULL) || !screen->prepared) {
        return false;
    }

    nc_buffer_init(&buffer);
    native_search_build_search_source_row(screen, &buffer);
    result = native_search_set_buffer_row(
        screen, NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW, &buffer);
    nc_buffer_destroy(&buffer);
    return result;
}

bool
native_search_engine_screen_update_search_mode_row(
    NativeSearchEngineScreen *screen) {
    NcBuffer buffer;
    bool result;

    if ((screen == NULL) || !screen->prepared) {
        return false;
    }

    nc_buffer_init(&buffer);
    native_search_build_search_mode_row(screen, &buffer);
    result = native_search_set_buffer_row(
        screen, NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW, &buffer);
    nc_buffer_destroy(&buffer);
    return result;
}

bool
native_search_engine_screen_add_result_summary(
    NativeSearchEngineScreen *screen, int32 song_count) {
    NcBuffer buffer;
    bool result;

    if ((screen == NULL) || (song_count <= 0)) {
        return false;
    }

    nc_buffer_init(&buffer);
    result = native_search_insert_buffer_with_flags(
        screen, NATIVE_SEARCH_ENGINE_RESULT_SEPARATOR_ROW, &buffer,
        NC_MENU_ITEM_SEPARATOR);
    if (!result) {
        nc_buffer_destroy(&buffer);
        return false;
    }

    nc_buffer_append_cstring(&buffer, "Search results: Found ");
    nc_buffer_append_int32(&buffer, song_count);
    if (song_count == 1) {
        nc_buffer_append_cstring(&buffer, " song");
    } else {
        nc_buffer_append_cstring(&buffer, " songs");
    }
    result = native_search_insert_buffer_with_flags(
        screen, NATIVE_SEARCH_ENGINE_RESULT_SUMMARY_ROW, &buffer,
        NC_MENU_ITEM_INACTIVE);
    if (!result) {
        nc_buffer_destroy(&buffer);
        return false;
    }

    nc_buffer_clear(&buffer);
    result = native_search_insert_buffer_with_flags(
        screen, NATIVE_SEARCH_ENGINE_RESULT_END_SEPARATOR_ROW, &buffer,
        NC_MENU_ITEM_SEPARATOR);
    nc_buffer_destroy(&buffer);
    if (result) {
        screen->result_rows_present = true;
        screen->result_count = song_count;
    }
    return result;
}

void
native_search_engine_screen_set_constraints_locked(
    NativeSearchEngineScreen *screen, bool locked) {
    NcMenu *menu;
    uint32 flags;

    if (screen == NULL) {
        return;
    }
    screen->constraints_locked = locked;
    menu = native_search_engine_screen_menu(screen);
    if (nc_menu_all_item_count(menu)
        <= NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW) {
        return;
    }

    for (int32 i = 0; i <= NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW; i += 1) {
        flags = nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i);
        if (locked) {
            flags |= NC_MENU_ITEM_INACTIVE;
        } else {
            flags &= ~NC_MENU_ITEM_INACTIVE;
        }
        nc_menu_set_item_flags_at(menu, NC_MENU_ITEMS_ALL, i, flags);
    }
    return;
}

void
native_search_engine_screen_reset(NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_buffer_clear(&screen->constraints[i]);
    }
    native_search_engine_screen_clear_filter(screen);
    native_search_engine_screen_clear_find_constraint(screen);
    native_search_engine_screen_prepare_static_rows(screen);
    native_search_engine_screen_status_message(
        screen, STRLIT_ARGS("Search state reset"));
    return;
}

bool
native_search_engine_screen_add_song_copy(NativeSearchEngineScreen *screen,
                                          NcmSong *song) {
    return native_search_engine_screen_add_song_copy_with_flags(
        screen, song, NC_MENU_ITEM_SELECTABLE);
}

bool
native_search_engine_screen_add_song_copy_with_flags(
    NativeSearchEngineScreen *screen, NcmSong *song, uint32 flags) {
    NcSearchRow row;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    nc_search_row_init(&row);
    row.is_song = true;
    if (!ncm_song_copy(&row.song, song)) {
        nc_search_row_destroy(&row);
        return false;
    }
    nc_search_row_menu_add_with_flags(&screen->rows, &row, flags);
    nc_search_row_destroy(&row);
    return true;
}

bool
native_search_engine_screen_add_buffer_with_flags(
    NativeSearchEngineScreen *screen, NcBuffer *buffer, uint32 flags) {
    NcSearchRow row;

    if ((screen == NULL) || (buffer == NULL)) {
        return false;
    }
    nc_search_row_init(&row);
    row.is_song = false;
    nc_buffer_copy(&row.buffer, buffer);
    nc_search_row_menu_add_with_flags(&screen->rows, &row, flags);
    nc_search_row_destroy(&row);
    return true;
}

bool
native_search_engine_screen_set_constraint(NativeSearchEngineScreen *screen,
                                           int32 idx, char *data,
                                           int32 data_len) {
    if ((screen == NULL) || (idx < 0)
        || (idx >= NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT)) {
        return false;
    }
    if (!ncm_buffer_set(&screen->constraints[idx], data, data_len)) {
        return false;
    }
    if (screen->prepared) {
        return native_search_engine_screen_update_constraint_row(
            screen, idx);
    }
    return true;
}

void
native_search_engine_screen_clear_find_constraint(
    NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_buffer_clear(&screen->search_constraint);
    screen->match_to_pattern = false;
    return;
}

bool
native_search_engine_screen_set_search_mode(
    NativeSearchEngineScreen *screen,
    enum NativeSearchEngineSearchMode mode) {
    if ((screen == NULL) || (mode < NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL)
        || (mode >= NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST)) {
        return false;
    }
    screen->search_mode = mode;
    if (screen->prepared) {
        return native_search_engine_screen_update_search_mode_row(screen);
    }
    return true;
}

void
native_search_engine_screen_set_search_source(
    NativeSearchEngineScreen *screen, bool search_in_database) {
    if (screen == NULL) {
        return;
    }
    screen->search_in_database = search_in_database;
    if (screen->prepared) {
        (void)native_search_engine_screen_update_search_source_row(screen);
    }
    return;
}

void
native_search_engine_screen_set_hooks(
    NativeSearchEngineScreen *screen, NativeSearchEngineHooks hooks) {
    if (screen == NULL) {
        return;
    }
    screen->hooks = hooks;
    return;
}

bool
native_search_engine_screen_list_database_songs(
    NativeSearchEngineScreen *screen, NcmSongArray *songs,
    NcmError *error) {
    if ((screen == NULL) || (songs == NULL)
        || (screen->hooks.list_database_songs == NULL)) {
        return false;
    }
    return screen->hooks.list_database_songs(screen->hooks.user,
                                             songs, error);
}

bool
native_search_engine_screen_snapshot_playlist(
    NativeSearchEngineScreen *screen, NcmSongArray *songs,
    NcmError *error) {
    if ((screen == NULL) || (songs == NULL)
        || (screen->hooks.snapshot_playlist == NULL)) {
        return false;
    }
    return screen->hooks.snapshot_playlist(screen->hooks.user,
                                           songs, error);
}

enum NativeSearchEnginePromptResult
native_search_engine_screen_prompt_constraint(
    NativeSearchEngineScreen *screen, int32 idx, NcmBuffer *result) {
    char *label;
    int32 label_len;

    if ((screen == NULL) || (result == NULL) || (idx < 0)
        || (idx >= NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT)
        || (screen->hooks.prompt_constraint == NULL)) {
        return NATIVE_SEARCH_ENGINE_PROMPT_ERROR;
    }
    label = native_search_engine_constraint_name(idx);
    label_len = native_search_constraint_name_lengths[idx];
    return screen->hooks.prompt_constraint(
        screen->hooks.user, label, label_len,
        &screen->constraints[idx], result);
}

void
native_search_engine_screen_status_message(
    NativeSearchEngineScreen *screen, char *message, int32 message_len) {
    if ((screen == NULL) || (message == NULL) || (message_len < 0)
        || (screen->hooks.status_message == NULL)) {
        return;
    }
    screen->hooks.status_message(screen->hooks.user, message, message_len);
    return;
}

bool
native_search_engine_screen_add_song(
    NativeSearchEngineScreen *screen, NcmSong *song, bool play,
    NcmError *error) {
    if ((screen == NULL) || (song == NULL)
        || (screen->hooks.add_song == NULL)) {
        return false;
    }
    return screen->hooks.add_song(screen->hooks.user, song, play, error);
}

bool
native_search_engine_screen_execute_search(
    NativeSearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *error) {
    NcmSongArray source;
    NcmSongArray songs;
    bool result;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing search screen"));
        return false;
    }

    ncm_song_array_init(&source);
    ncm_song_array_init(&songs);
    ncm_error_clear(error);
    native_search_engine_screen_clear_filter(screen);
    native_search_engine_screen_prepare_static_rows(screen);
    native_search_engine_screen_status_message(
        screen, STRLIT_ARGS("Searching..."));

    result = true;
    if (!native_search_has_constraints(screen)) {
        native_search_engine_screen_status_message(
            screen, STRLIT_ARGS("No results found"));
        goto cleanup;
    }

    if (screen->search_in_database
        && ((screen->search_mode
             == NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL)
            || (screen->search_mode
                == NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT))) {
        result = native_search_collect_database_results(
            screen, client, &songs, error);
    } else {
        if (screen->search_in_database) {
            result = native_search_engine_screen_list_database_songs(
                screen, &source, error);
        } else {
            result = native_search_engine_screen_snapshot_playlist(
                screen, &source, error);
        }
        if (result) {
            result = native_search_collect_local_results(
                screen, &source, &songs, error);
        }
    }

    if (!result) {
        native_search_engine_screen_prepare_static_rows(screen);
        native_search_print_error(screen, error);
        goto cleanup;
    }

    if (songs.len <= 0) {
        native_search_engine_screen_status_message(
            screen, STRLIT_ARGS("No results found"));
        goto cleanup;
    }

    if (!native_search_append_result_rows(screen, &songs)) {
        ncm_error_set(error, EIO,
                      STRLIT_ARGS("failed to build search results"));
        native_search_engine_screen_prepare_static_rows(screen);
        native_search_print_error(screen, error);
        result = false;
        goto cleanup;
    }

    native_search_engine_screen_set_constraints_locked(
        screen, Config.block_search_constraints_change);
    nc_menu_scroll_selectable(
        native_search_engine_screen_menu(screen),
        nc_window_height(&screen->window), NC_SCROLL_DOWN);
    nc_menu_scroll_selectable(
        native_search_engine_screen_menu(screen),
        nc_window_height(&screen->window), NC_SCROLL_DOWN);
    native_search_engine_screen_update_column_title(screen);
    native_search_engine_screen_status_message(
        screen, STRLIT_ARGS("Searching finished"));

cleanup:
    ncm_song_array_destroy(&songs);
    ncm_song_array_destroy(&source);
    return result;
}

bool
native_search_engine_screen_can_run_current(
    NativeSearchEngineScreen *screen) {
    NcMenu *menu;
    NcSearchRow *row;
    int64 pos;

    if ((screen == NULL) || !screen->prepared) {
        return false;
    }

    menu = native_search_engine_screen_menu(screen);
    if (nc_menu_empty(menu)) {
        return false;
    }
    pos = nc_menu_highlight(menu);
    if (nc_menu_position_is_separator(menu, pos)
        || nc_menu_position_is_inactive(menu, pos)) {
        return false;
    }

    row = nc_search_row_menu_current(&screen->rows);
    return row && !row->is_song;
}

bool
native_search_engine_screen_run_current(
    NativeSearchEngineScreen *screen) {
    enum NativeSearchEnginePromptResult prompt_status;
    enum NativeSearchEngineSearchMode mode;
    NcmBuffer value;
    NcmError error;
    NcMenu *menu;
    int64 pos;
    uint32 next_mode;
    bool success;

    if (!native_search_engine_screen_can_run_current(screen)) {
        return false;
    }

    menu = native_search_engine_screen_menu(screen);
    pos = nc_menu_highlight(menu);
    if (pos < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT) {
        ncm_buffer_init(&value);
        prompt_status = native_search_engine_screen_prompt_constraint(
            screen, (int32)pos, &value);
        if (prompt_status == NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED) {
            success = native_search_engine_screen_set_constraint(
                screen, (int32)pos, value.data, value.len);
            ncm_buffer_destroy(&value);
            return success;
        }
        ncm_buffer_destroy(&value);
        if (prompt_status == NATIVE_SEARCH_ENGINE_PROMPT_ABORTED) {
            native_search_engine_screen_status_message(
                screen, STRLIT_ARGS("Action aborted"));
        } else {
            native_search_engine_screen_status_message(
                screen, STRLIT_ARGS("Unable to read search constraint"));
        }
        return false;
    }

    if (pos == NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW) {
        screen->search_in_database = !screen->search_in_database;
        Config.search_in_db = screen->search_in_database;
        return native_search_engine_screen_update_search_source_row(
            screen);
    }
    if (pos == NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW) {
        next_mode = (uint32)screen->search_mode + 1;
        if (next_mode >= NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST) {
            next_mode = NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL;
        }
        mode = (enum NativeSearchEngineSearchMode)next_mode;
        return native_search_engine_screen_set_search_mode(screen, mode);
    }
    if (pos == NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW) {
        ncm_error_clear(&error);
        return native_search_engine_screen_start_searching(
            screen, screen->hooks.client, &error);
    }
    if (pos == NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW) {
        native_search_engine_screen_reset(screen);
        return true;
    }
    return false;
}

bool
native_search_engine_screen_start_searching(
    NativeSearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *error) {
    NcMenu *menu;

    if ((screen == NULL) || screen->constraints_locked) {
        return false;
    }
    if (!screen->prepared) {
        native_search_engine_screen_prepare_static_rows(screen);
    }

    menu = native_search_engine_screen_menu(screen);
    nc_menu_highlight_position(
        menu, NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW,
        nc_window_height(&screen->window));
    return native_search_engine_screen_execute_search(screen, client, error);
}

enum DisplayMode
native_search_engine_screen_toggle_display_mode(
    NativeSearchEngineScreen *screen) {
    enum DisplayMode mode;

    if (screen == NULL) {
        return Config.search_engine_display_mode;
    }
    mode = Config.search_engine_display_mode;
    if (mode == NCM_DISPLAY_MODE_CLASSIC) {
        mode = NCM_DISPLAY_MODE_COLUMNS;
    } else {
        mode = NCM_DISPLAY_MODE_CLASSIC;
    }
    native_search_engine_screen_set_display_mode(screen, mode);
    return mode;
}

bool
native_search_engine_screen_allows_search(
    NativeSearchEngineScreen *screen) {
    NcMenu *menu;
    NcSearchRow *row;
    int64 count;

    if (screen == NULL) {
        return false;
    }
    menu = native_search_engine_screen_menu(screen);
    count = nc_menu_item_count(menu);
    if (count <= 0) {
        return false;
    }
    row = nc_menu_active_item_at(menu, count - 1);
    if (row == NULL) {
        return false;
    }
    return row->is_song;
}

bool
native_search_engine_screen_current_song(NativeSearchEngineScreen *screen,
                                         NcmSong *song) {
    NcSearchRow *row;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    row = nc_search_row_menu_current(&screen->rows);
    if ((row == NULL) || !row->is_song) {
        return false;
    }
    return ncm_song_copy(song, &row->song);
}

bool
native_search_engine_screen_selected_songs(NativeSearchEngineScreen *screen,
                                           NcmSongArray *songs) {
    NcMenu *menu;
    bool any_selected;

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }
    menu = native_search_engine_screen_menu(screen);
    any_selected = nc_menu_has_selected(menu);
    if (!any_selected) {
        return native_search_copy_song_at(
            screen, songs, nc_menu_highlight(menu));
    }
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!nc_menu_position_is_selected(menu, i)) {
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
    NcMenuDisplayCallbacks callbacks;

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
    if (!ncm_buffer_set(&screen->filter_constraint, pattern, pattern_len)) {
        return false;
    }
    callbacks = native_search_display_callbacks(screen, true);
    nc_menu_set_display_callbacks(native_search_engine_screen_menu(screen),
                                  callbacks);
    screen->filter_enabled = true;
    nc_menu_apply_filter(native_search_engine_screen_menu(screen));
    return true;
}

void
native_search_engine_screen_clear_filter(NativeSearchEngineScreen *screen) {
    NcMenuDisplayCallbacks callbacks;

    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_regex_init(&screen->filter_regex);
    ncm_buffer_clear(&screen->filter_constraint);
    screen->filter_enabled = false;
    callbacks = native_search_display_callbacks(screen, false);
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
    NativeSearchFindContext context;
    NcmRegex regex;
    NcMenu *menu;
    bool result;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len,
                           NCM_REGEX_LITERAL_CASE_INSENSITIVE, error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    menu = native_search_engine_screen_menu(screen);
    context.screen = screen;
    context.regex = &regex;
    result = nc_menu_search_selectable(menu, screen->main_height, forward,
                                       wrap, skip_current,
                                       native_search_find_position,
                                       &context, NULL);

    ncm_regex_destroy(&regex);
    return result;
}

static bool
native_search_find_position(NcMenu *menu, int64 pos,
                            void *user) {
    NativeSearchFindContext *context;

    context = user;
    return native_search_row_matches(
        context->screen, nc_menu_active_item_at(menu, pos),
        context->regex);
}

static NativeSearchEngineScreen *
native_search_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
native_search_active_window(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    return &search->window;
}

static void
native_search_refresh(NcScreen *screen) {
    NativeSearchEngineScreen *search;
    NcMenu *menu;
    NcWindow *window;

    search = native_search_from_screen(screen);
    if (!search->prepared) {
        native_search_engine_screen_prepare_static_rows(search);
    }
    native_search_engine_screen_update_column_title(search);
    menu = native_search_engine_screen_menu(search);
    window = native_search_engine_screen_window(search);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
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
                              nc_window_height(&search->window), where);
    return;
}

static void
native_search_switch_to(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (!search->prepared) {
        native_search_engine_screen_prepare_static_rows(search);
    }
    native_search_engine_screen_update_column_title(search);
    return;
}

static void
native_search_resize(NcScreen *screen) {
    NativeSearchEngineScreen *search;
    int64 start_x;
    int64 width;

    search = native_search_from_screen(screen);
    nc_screen_get_resize_params(screen, &start_x, &width);
    native_search_engine_screen_set_geometry(
        search, start_x, width, ui_state_main_start_y(),
        ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
native_search_title(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    return search->title.data;
}

static void
native_search_update(NcScreen *screen) {
    nc_screen_clear_update_request(screen);
    return;
}

static void
native_search_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NativeSearchEngineScreen *search;
    NcMenu *menu;
    NcSearchRow *row;
    NcWindow *window;
    NcmError error;
    int32 x;
    int32 y;
    bool play;

    search = native_search_from_screen(screen);
    window = native_search_engine_screen_window(search);
    x = event.x;
    y = event.y;
    if (!nc_window_has_coords(window, &x, &y)) {
        return;
    }

    menu = native_search_engine_screen_menu(search);
    if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
        if (!nc_menu_goto_selectable(menu, y)) {
            return;
        }
        row = nc_search_row_menu_current(&search->rows);
        if (row == NULL) {
            return;
        }
        if (!row->is_song) {
            if (event.bstate & BUTTON3_PRESSED) {
                (void)nc_screen_run_current(screen);
            }
            return;
        }

        play = (event.bstate & BUTTON3_PRESSED) != 0;
        ncm_error_clear(&error);
        if (!native_search_engine_screen_add_song(
                search, &row->song, play, &error)) {
            native_search_print_error(search, &error);
        }
        return;
    }

    if (event.bstate & BUTTON5_PRESSED) {
        native_search_mouse_scroll(search, NC_SCROLL_DOWN);
    } else if (event.bstate & BUTTON4_PRESSED) {
        native_search_mouse_scroll(search, NC_SCROLL_UP);
    }
    return;
}

static bool
native_search_can_run_current(NcScreen *screen) {
    return native_search_engine_screen_can_run_current(
        native_search_from_screen(screen));
}

static bool
native_search_run_current(NcScreen *screen) {
    return native_search_engine_screen_run_current(
        native_search_from_screen(screen));
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
    NcSearchRow *row;

    (void)menu;
    screen = user;
    row = item;
    if ((row == NULL) || !row->is_song) {
        return true;
    }
    return native_search_row_matches(screen, row, &screen->filter_regex);
}

static bool
native_search_row_matches(NativeSearchEngineScreen *screen,
                          NcSearchRow *row, NcmRegex *regex) {
    NcmStringView view;

    if ((row == NULL) || !row->is_song
        || !native_search_row_label(screen, row, &view)) {
        return false;
    }
    return ncm_regex_search(regex, view.data, view.len);
}

static bool
native_search_row_label(NativeSearchEngineScreen *screen,
                        NcSearchRow *row, NcmStringView *view) {
    if ((screen == NULL) || (row == NULL) || (view == NULL)) {
        return false;
    }
    if (row->is_song) {
        if (screen->hooks.format_song) {
            ncm_buffer_clear(&screen->row_text);
            if (!screen->hooks.format_song(
                    screen->hooks.user, &row->song, &screen->row_text)) {
                return false;
            }
            *view = ncm_string_view_make(screen->row_text.data,
                                         screen->row_text.len);
            return true;
        }
        if (!native_search_engine_screen_format_song_text(
                screen, &row->song, &screen->row_text)) {
            return false;
        }
        *view = ncm_string_view_make(screen->row_text.data,
                                     screen->row_text.len);
        return true;
    }
    *view = ncm_string_view_make(row->buffer.data, row->buffer.len);
    return true;
}

static bool
native_search_insert_buffer_with_flags(
    NativeSearchEngineScreen *screen, int64 pos, NcBuffer *buffer,
    uint32 flags) {
    NcSearchRow row;

    if ((screen == NULL) || (buffer == NULL)) {
        return false;
    }
    nc_search_row_init(&row);
    nc_buffer_copy(&row.buffer, buffer);
    nc_search_row_menu_insert_with_flags(&screen->rows, pos, &row, flags);
    nc_search_row_destroy(&row);
    return true;
}

static void
native_search_append_constraint_row(
    NativeSearchEngineScreen *screen, int32 idx) {
    NcBuffer buffer;

    nc_buffer_init(&buffer);
    native_search_build_constraint_row(screen, idx, &buffer);
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);
    nc_buffer_destroy(&buffer);
    return;
}

static bool
native_search_set_buffer_row(
    NativeSearchEngineScreen *screen, int32 pos, NcBuffer *buffer) {
    NcSearchRow *row;

    row = nc_search_row_menu_item_at(
        &screen->rows, NC_MENU_ITEMS_ALL, pos);
    if ((row == NULL) || row->is_song) {
        return false;
    }
    nc_buffer_destroy(&row->buffer);
    nc_buffer_copy(&row->buffer, buffer);
    return true;
}

static void
native_search_build_constraint_row(
    NativeSearchEngineScreen *screen, int32 idx, NcBuffer *buffer) {
    nc_buffer_clear(buffer);
    native_search_append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_cstring(buffer, native_search_constraint_names[idx]);
    while (nc_buffer_len(buffer) < 13) {
        nc_buffer_append_char(buffer, ' ');
    }
    native_search_append_format(buffer, NC_FORMAT_NO_BOLD);
    nc_buffer_append_data(buffer, STRLIT_ARGS(": "));
    native_search_append_tag_value(buffer, &screen->constraints[idx]);
    return;
}

static void
native_search_build_search_source_row(
    NativeSearchEngineScreen *screen, NcBuffer *buffer) {
    nc_buffer_clear(buffer);
    native_search_append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_data(buffer, STRLIT_ARGS("Search in:"));
    native_search_append_format(buffer, NC_FORMAT_NO_BOLD);
    nc_buffer_append_char(buffer, ' ');
    if (screen->search_in_database) {
        nc_buffer_append_data(buffer, STRLIT_ARGS("Database"));
    } else {
        nc_buffer_append_data(buffer, STRLIT_ARGS("Current playlist"));
    }
    return;
}

static void
native_search_build_search_mode_row(
    NativeSearchEngineScreen *screen, NcBuffer *buffer) {
    nc_buffer_clear(buffer);
    native_search_append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_data(buffer, STRLIT_ARGS("Search mode:"));
    native_search_append_format(buffer, NC_FORMAT_NO_BOLD);
    nc_buffer_append_char(buffer, ' ');
    nc_buffer_append_cstring(
        buffer, native_search_engine_search_mode_name(screen->search_mode));
    return;
}

static void
native_search_append_format(NcBuffer *buffer, enum NcFormat format) {
    nc_buffer_add_format(buffer, nc_buffer_len(buffer), format, 0);
    return;
}

static void
native_search_append_tag_value(NcBuffer *buffer, NcmBuffer *value) {
    if (value->len > 0) {
        nc_buffer_append_data(buffer, value->data, value->len);
        return;
    }
    if ((Config.empty_tag == NULL) || (Config.empty_tag_len <= 0)) {
        return;
    }
    nc_buffer_add_formatted_color(
        buffer, nc_buffer_len(buffer), &Config.empty_tags_color, 0);
    nc_buffer_append_data(buffer, Config.empty_tag, Config.empty_tag_len);
    nc_buffer_add_formatted_color_end(
        buffer, nc_buffer_len(buffer), &Config.empty_tags_color, 0);
    return;
}

static NcMenuDisplayCallbacks
native_search_display_callbacks(NativeSearchEngineScreen *screen,
                                bool filtering) {
    NcMenuDisplayCallbacks callbacks;

    callbacks = (NcMenuDisplayCallbacks){0};
    callbacks.draw = native_search_draw_row;
    if (filtering) {
        callbacks.filter = native_search_filter_row;
    }
    callbacks.user = screen;
    return callbacks;
}

static void
native_search_draw_row(NcMenu *menu, NcWindow *window, void *item,
                       int64 pos, void *user) {
    NativeSearchEngineScreen *screen;
    NcSearchRow *row;

    screen = user;
    row = item;
    if ((screen == NULL) || (window == NULL) || (row == NULL)) {
        return;
    }

    if (!row->is_song) {
        native_search_print_buffer(window, &row->buffer);
        return;
    }

    if (Config.search_engine_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        native_search_draw_columns_song(
            screen, menu, window, &row->song, pos);
    } else {
        native_search_draw_classic_song(
            screen, menu, window, &row->song, pos);
    }
    return;
}

static void
native_search_draw_classic_song(
    NativeSearchEngineScreen *screen, NcMenu *menu, NcWindow *window,
    NcmSong *song, int64 pos) {
    NcBuffer left;
    NcBuffer right;
    int32 right_width;
    int32 right_x;
    int32 y;

    nc_buffer_init(&left);
    nc_buffer_init(&right);
    ncm_format_render_buffer(&Config.song_list_format, song,
                             &left, &right, NCM_FORMAT_FLAG_ALL);
    native_search_print_buffer(window, &left);

    if (right.len > 0) {
        right_width = native_search_buffer_width(&right);
        right_x = native_search_screen_width(screen)
                  - native_search_menu_suffix_width(menu, pos)
                  - right_width;
        if (right_x < 0) {
            right_x = 0;
        }
        y = nc_window_get_y(window);
        nc_window_go_to_xy(window, right_x, y);
        native_search_print_buffer(window, &right);
    }

    nc_buffer_destroy(&left);
    nc_buffer_destroy(&right);
    return;
}

static void
native_search_draw_columns_song(
    NativeSearchEngineScreen *screen, NcMenu *menu, NcWindow *window,
    NcmSong *song, int64 pos) {
    NcBuffer buffer;
    int32 width;

    width = native_search_screen_width(screen)
            - native_search_menu_prefix_width(menu, pos)
            - native_search_menu_suffix_width(menu, pos);
    if (width < 0) {
        width = 0;
    }

    nc_buffer_init(&buffer);
    (void)native_search_format_columns(screen, song, &buffer, width);
    native_search_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);
    return;
}

static bool
native_search_format_columns(
    NativeSearchEngineScreen *screen, NcmSong *song, NcBuffer *buffer,
    int32 list_width) {
    if ((screen == NULL) || (song == NULL) || (buffer == NULL)) {
        return false;
    }

    ncm_display_song_columns(buffer, song, Config.columns.items,
                             Config.columns.len, list_width, true);
    return true;
}

static int32
native_search_screen_width(NativeSearchEngineScreen *screen) {
    if ((screen == NULL) || (screen->width <= 0)) {
        return 0;
    }
    if (screen->width > INT32_MAX) {
        return INT32_MAX;
    }
    return (int32)screen->width;
}

static int32
native_search_menu_prefix_width(NcMenu *menu, int64 pos) {
    int32 width;

    if (menu == NULL) {
        return 0;
    }

    width = 0;
    if (menu->highlight_enabled && (pos == menu->highlight)) {
        width += native_search_buffer_width(&menu->highlight_prefix);
    }
    if (nc_menu_position_is_selected(menu, pos)) {
        width += native_search_buffer_width(&menu->selected_prefix);
    }
    return width;
}

static int32
native_search_menu_suffix_width(NcMenu *menu, int64 pos) {
    int32 width;

    if (menu == NULL) {
        return 0;
    }

    width = 0;
    if (nc_menu_position_is_selected(menu, pos)) {
        width += native_search_buffer_width(&menu->selected_suffix);
    }
    if (menu->highlight_enabled && (pos == menu->highlight)) {
        width += native_search_buffer_width(&menu->highlight_suffix);
    }
    return width;
}

static int32
native_search_buffer_width(NcBuffer *buffer) {
    if ((buffer == NULL) || (buffer->len <= 0)) {
        return 0;
    }
    return utf8_width(buffer->data, buffer->len);
}

static void
native_search_print_buffer(NcWindow *window, NcBuffer *buffer) {
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

static void
native_search_mouse_scroll(NativeSearchEngineScreen *screen,
                           enum NcScroll where) {
    NcMenu *menu;
    enum NcScroll effective;
    int64 count;

    menu = native_search_engine_screen_menu(screen);
    effective = where;
    count = screen->lines_scrolled;
    if (screen->mouse_list_scroll_whole_page) {
        count = 1;
        if (where == NC_SCROLL_UP) {
            effective = NC_SCROLL_PAGE_UP;
        } else {
            effective = NC_SCROLL_PAGE_DOWN;
        }
    }

    for (int64 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(menu, nc_window_height(&screen->window),
                                  effective);
    }
    return;
}

static bool
native_search_has_constraints(NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return false;
    }

    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        if (screen->constraints[i].len > 0) {
            return true;
        }
    }
    return false;
}

static bool
native_search_collect_database_results(
    NativeSearchEngineScreen *screen, NcmMpdClient *client,
    NcmSongArray *songs, NcmError *error) {
    NcmMpdSongList result;
    bool exact_match;
    bool ok;

    if ((screen == NULL) || (client == NULL) || (songs == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing database search state"));
        return false;
    }

    exact_match = screen->search_mode
        == NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT;
    ncm_mpd_song_list_init(&result);
    ok = ncm_mpd_client_start_search(client, exact_match, error);
    if (ok) {
        ok = native_search_add_database_constraints(
            screen, client, error);
    }
    if (ok) {
        ok = ncm_mpd_client_commit_search_songs(
            client, &result, error);
    }
    if (ok) {
        ok = ncm_mpd_song_list_to_song_array(&result, songs);
        if (!ok) {
            ncm_error_set(error, EIO,
                          STRLIT_ARGS("failed to copy search results"));
        }
    }
    ncm_mpd_song_list_destroy(&result);
    return ok;
}

static bool
native_search_add_database_constraints(
    NativeSearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *error) {
    NcmBuffer *constraint;

    constraint = &screen->constraints[0];
    if ((constraint->len > 0)
        && !ncm_mpd_client_add_search_any(
            client, constraint->data, error)) {
        return false;
    }

    for (int32 i = 1; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        enum mpd_tag_type tag;

        constraint = &screen->constraints[i];
        if (constraint->len <= 0) {
            continue;
        }
        if (i == 5) {
            if (!ncm_mpd_client_add_search_uri(
                    client, constraint->data, error)) {
                return false;
            }
            continue;
        }

        switch (i) {
        case 1:
            tag = MPD_TAG_ARTIST;
            break;
        case 2:
            tag = MPD_TAG_ALBUM_ARTIST;
            break;
        case 3:
            tag = MPD_TAG_TITLE;
            break;
        case 4:
            tag = MPD_TAG_ALBUM;
            break;
        case 6:
            tag = MPD_TAG_COMPOSER;
            break;
        case 7:
            tag = MPD_TAG_PERFORMER;
            break;
        case 8:
            tag = MPD_TAG_GENRE;
            break;
        case 9:
            tag = MPD_TAG_DATE;
            break;
        case 10:
            tag = MPD_TAG_COMMENT;
            break;
        default:
            ncm_error_set(error, EINVAL,
                          STRLIT_ARGS("invalid search constraint"));
            return false;
        }
        if (!ncm_mpd_client_add_search_tag(
                client, tag, constraint->data, error)) {
            return false;
        }
    }
    return true;
}

static bool
native_search_collect_local_results(
    NativeSearchEngineScreen *screen, NcmSongArray *source,
    NcmSongArray *songs, NcmError *error) {
    NcmRegex regexes[NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT];
    NcmError regex_error;
    bool exact_match;
    bool ok;

    if ((screen == NULL) || (source == NULL) || (songs == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing local search state"));
        return false;
    }

    exact_match = screen->search_mode
        == NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT;
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_regex_init(&regexes[i]);
    }

    if (!exact_match) {
        for (int32 i = 0;
             i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
            if (screen->constraints[i].len <= 0) {
                continue;
            }
            ncm_error_clear(&regex_error);
            (void)ncm_regex_compile(
                &regexes[i], screen->constraints[i].data,
                screen->constraints[i].len, Config.regex_flags,
                &regex_error);
        }
    }

    ok = true;
    for (int32 i = 0; i < source->len; i += 1) {
        if (!native_search_song_matches(
                screen, &source->items[i], regexes)) {
            continue;
        }
        if (!ncm_song_array_append_copy(songs, &source->items[i])) {
            ncm_error_set(error, EIO,
                          STRLIT_ARGS("failed to copy matching song"));
            ok = false;
            break;
        }
    }

    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_regex_destroy(&regexes[i]);
    }
    if (ok) {
        ncm_error_clear(error);
    }
    return ok;
}

static bool
native_search_song_matches(
    NativeSearchEngineScreen *screen, NcmSong *song, NcmRegex *regexes) {
    if ((screen == NULL) || (song == NULL) || (regexes == NULL)) {
        return false;
    }

    if ((screen->constraints[0].len > 0)
        && !native_search_song_any_matches(screen, song, &regexes[0])) {
        return false;
    }
    for (int32 i = 1; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        if (screen->constraints[i].len <= 0) {
            continue;
        }
        if (!native_search_song_field_matches(
                screen, song, i, &regexes[i])) {
            return false;
        }
    }
    return true;
}

static bool
native_search_song_field_matches(
    NativeSearchEngineScreen *screen, NcmSong *song, int32 field,
    NcmRegex *regex) {
    NcmStringView value;
    NcmBuffer *constraint;

    constraint = &screen->constraints[field];
    if (!native_search_song_field_view(song, field, &value)) {
        value = ncm_string_view_make(native_search_empty_string, 0);
    }
    if (screen->search_mode == NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT) {
        return ncm_compare_locale_strings(
                   value.data, value.len, constraint->data,
                   constraint->len, Config.ignore_leading_the) == 0;
    }
    if ((regex == NULL) || !regex->compiled) {
        return true;
    }
    return ncm_regex_search(regex, value.data, value.len);
}

static bool
native_search_song_any_matches(
    NativeSearchEngineScreen *screen, NcmSong *song, NcmRegex *regex) {
    NcmStringView value;
    NcmBuffer *constraint;

    constraint = &screen->constraints[0];
    if ((screen->search_mode != NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT)
        && ((regex == NULL) || !regex->compiled)) {
        return true;
    }

    for (int32 i = 1; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        if (!native_search_song_field_view(song, i, &value)) {
            value = ncm_string_view_make(native_search_empty_string, 0);
        }
        if (screen->search_mode
            == NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT) {
            if (ncm_compare_locale_strings(
                    value.data, value.len, constraint->data,
                    constraint->len, Config.ignore_leading_the) == 0) {
                return true;
            }
        } else if (ncm_regex_search(regex, value.data, value.len)) {
            return true;
        }
    }
    return false;
}

static bool
native_search_song_field_view(NcmSong *song, int32 field,
                              NcmStringView *view) {
    enum mpd_tag_type tag;

    if ((song == NULL) || (view == NULL)) {
        return false;
    }
    if (field == 5) {
        return ncm_song_name_view(song, 0, view);
    }

    switch (field) {
    case 1:
        tag = MPD_TAG_ARTIST;
        break;
    case 2:
        tag = MPD_TAG_ALBUM_ARTIST;
        break;
    case 3:
        tag = MPD_TAG_TITLE;
        break;
    case 4:
        tag = MPD_TAG_ALBUM;
        break;
    case 6:
        tag = MPD_TAG_COMPOSER;
        break;
    case 7:
        tag = MPD_TAG_PERFORMER;
        break;
    case 8:
        tag = MPD_TAG_GENRE;
        break;
    case 9:
        tag = MPD_TAG_DATE;
        break;
    case 10:
        tag = MPD_TAG_COMMENT;
        break;
    default:
        return false;
    }
    return ncm_song_tag_view(song, tag, 0, view);
}

static bool
native_search_append_result_rows(
    NativeSearchEngineScreen *screen, NcmSongArray *songs) {
    if ((screen == NULL) || (songs == NULL) || (songs->len <= 0)) {
        return false;
    }

    for (int32 i = 0; i < songs->len; i += 1) {
        if (!native_search_engine_screen_add_song_copy(
                screen, &songs->items[i])) {
            return false;
        }
    }
    return native_search_engine_screen_add_result_summary(
        screen, songs->len);
}

static void
native_search_print_error(NativeSearchEngineScreen *screen,
                          NcmError *error) {
    int32 len;

    if ((screen == NULL) || (screen->hooks.status_message == NULL)
        || (error == NULL)) {
        return;
    }
    len = optional_strlen32(error->message);
    if (len > 0) {
        screen->hooks.status_message(screen->hooks.user, error->message,
                                     len);
    }
    return;
}

static bool
native_search_copy_song_at(NativeSearchEngineScreen *screen,
                           NcmSongArray *songs, int64 pos) {
    NcSearchRow *row;

    row = nc_menu_active_item_at(native_search_engine_screen_menu(screen),
                                 pos);
    if ((row == NULL) || !row->is_song) {
        return true;
    }
    return ncm_song_array_append_copy(songs, &row->song);
}

#endif /* NCMPCPP_NC_SEARCH_ENGINE_C */
