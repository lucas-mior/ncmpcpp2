#include "screens/nc_search_engine.h"

#include "c/ncm_base.h"
#include "c/ncm_string.h"

static NativeSearchEngineScreen *native_search_from_screen(NcScreen *screen);
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
static bool native_search_filter_row(NcMenu *menu, void *item, void *user);
static bool native_search_row_matches(NativeSearchEngineScreen *screen,
                                      NcSearchRow *row, NcmRegex *regex);
static bool native_search_row_label(NativeSearchEngineScreen *screen,
                                    NcSearchRow *row, NcmStringView *view);
static bool native_search_copy_song_at(NativeSearchEngineScreen *screen,
                                       NcmSongArray *songs, int64 pos);
static bool native_search_insert_buffer_with_flags(
    NativeSearchEngineScreen *screen, int64 pos, NcBuffer *buffer,
    uint32 flags);
static void native_search_append_constraint_row(
    NativeSearchEngineScreen *screen, int32 idx);

static char *native_search_constraint_names[] = {
    (char *)"Any",
    (char *)"Artist",
    (char *)"Album Artist",
    (char *)"Title",
    (char *)"Album",
    (char *)"Filename",
    (char *)"Composer",
    (char *)"Performer",
    (char *)"Genre",
    (char *)"Date",
    (char *)"Comment",
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
    (char *)"Match if tag contains searched phrase (no regexes)",
    (char *)"Match if tag contains searched phrase (regexes supported)",
    (char *)"Match only if both values are the same",
};

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

    screen->bridge = (NativeSearchEngineBridge){0};
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
native_search_engine_screen_is_prepared(
    NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->prepared;
}

void
native_search_engine_screen_set_prepared(
    NativeSearchEngineScreen *screen, bool prepared) {
    if (screen == NULL) {
        return;
    }
    screen->prepared = prepared;
    if (!prepared) {
        screen->result_rows_present = false;
        screen->result_count = 0;
        screen->constraints_locked = false;
    }
    return;
}

bool
native_search_engine_screen_has_result_rows(
    NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->result_rows_present;
}

int32
native_search_engine_screen_result_count(
    NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return 0;
    }
    return screen->result_count;
}

void
native_search_engine_screen_set_result_state(
    NativeSearchEngineScreen *screen, bool rows_present,
    int32 result_count) {
    if (screen == NULL) {
        return;
    }
    if (result_count < 0) {
        result_count = 0;
    }
    if (!rows_present) {
        result_count = 0;
    }
    screen->result_rows_present = rows_present;
    screen->result_count = result_count;
    return;
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
native_search_engine_screen_set_title(
    NativeSearchEngineScreen *screen, char *title, int32 title_len) {
    if ((screen == NULL) || (title_len < 0)
        || ((title == NULL) && (title_len > 0))) {
        return false;
    }
    return ncm_buffer_set(&screen->title, title, title_len);
}

NcmStringView
native_search_engine_screen_title(NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(screen->title.data, screen->title.len);
}

bool
native_search_engine_screen_set_column_title(
    NativeSearchEngineScreen *screen, char *title, int32 title_len) {
    if ((screen == NULL) || (title_len < 0)
        || ((title == NULL) && (title_len > 0))) {
        return false;
    }
    return ncm_buffer_set(&screen->column_title, title, title_len);
}

NcmStringView
native_search_engine_screen_column_title(
    NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return ncm_string_view_make(NULL, 0);
    }
    return ncm_string_view_make(screen->column_title.data,
                                screen->column_title.len);
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
    nc_buffer_append_cstring(&buffer, (char *)"Search in: ");
    if (screen->search_in_database) {
        nc_buffer_append_cstring(&buffer, (char *)"Database");
    } else {
        nc_buffer_append_cstring(&buffer, (char *)"Current playlist");
    }
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);

    nc_buffer_clear(&buffer);
    nc_buffer_append_cstring(&buffer, (char *)"Search mode: ");
    nc_buffer_append_cstring(
        &buffer,
        native_search_engine_search_mode_name(screen->search_mode));
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);

    nc_buffer_clear(&buffer);
    native_search_engine_screen_add_buffer_with_flags(
        screen, &buffer, NC_MENU_ITEM_SEPARATOR);
    nc_buffer_append_cstring(&buffer, (char *)"Search");
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);
    nc_buffer_clear(&buffer);
    nc_buffer_append_cstring(&buffer, (char *)"Reset");
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
native_search_engine_screen_add_result_summary(
    NativeSearchEngineScreen *screen, int32 song_count) {
    NcBuffer buffer;
    bool result;

    if (screen == NULL || song_count <= 0) {
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

    nc_buffer_append_cstring(&buffer, (char *)"Search results: Found ");
    nc_buffer_append_int32(&buffer, song_count);
    if (song_count == 1) {
        nc_buffer_append_cstring(&buffer, (char *)" song");
    } else {
        nc_buffer_append_cstring(&buffer, (char *)" songs");
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
    native_search_engine_screen_prepare_static_rows(screen);
    return;
}

bool
native_search_engine_screen_add_song_copy(NativeSearchEngineScreen *screen,
                                          NcmSong *song) {
    return native_search_engine_screen_add_song_copy_with_flags(
        screen, song, NC_MENU_ITEM_SELECTABLE);
}

bool
native_search_engine_screen_add_buffer(NativeSearchEngineScreen *screen,
                                       NcBuffer *buffer) {
    return native_search_engine_screen_add_buffer_with_flags(
        screen, buffer, NC_MENU_ITEM_SELECTABLE);
}

bool
native_search_engine_screen_add_song_copy_with_flags(
    NativeSearchEngineScreen *screen, NcmSong *song, uint32 flags) {
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
    nc_search_row_menu_add_with_flags(&screen->rows, &row, flags);
    nc_search_row_destroy(&row);
    return true;
}

bool
native_search_engine_screen_add_buffer_with_flags(
    NativeSearchEngineScreen *screen, NcBuffer *buffer, uint32 flags) {
    NcSearchRow row;

    if (screen == NULL || buffer == NULL) {
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
native_search_engine_screen_set_search_mode(
    NativeSearchEngineScreen *screen,
    enum NativeSearchEngineSearchMode mode) {
    if (screen == NULL || mode < NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL
        || mode >= NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST) {
        return false;
    }
    screen->search_mode = mode;
    return true;
}

enum NativeSearchEngineSearchMode
native_search_engine_screen_search_mode(
    NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL;
    }
    return screen->search_mode;
}

void
native_search_engine_screen_set_search_source(
    NativeSearchEngineScreen *screen, bool search_in_database) {
    if (screen == NULL) {
        return;
    }
    screen->search_in_database = search_in_database;
    return;
}

bool
native_search_engine_screen_searches_database(
    NativeSearchEngineScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->search_in_database;
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
native_search_engine_screen_collect_results(
    NativeSearchEngineScreen *screen, NcmSongArray *songs, NcmError *error) {
    bool has_constraint;

    if (screen == NULL || songs == NULL) {
        return false;
    }

    has_constraint = false;
    for (int32 i = 0; i < NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        if (screen->constraints[i].len > 0) {
            has_constraint = true;
            break;
        }
    }
    if (!has_constraint) {
        return true;
    }
    if (screen->hooks.collect_results == NULL) {
        return false;
    }
    return screen->hooks.collect_results(
        screen->hooks.user, screen->search_in_database,
        screen->search_mode, screen->constraints,
        NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT, songs, error);
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
    if (!ncm_buffer_set(&screen->filter_constraint, pattern, pattern_len)) {
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
    ncm_buffer_clear(&screen->filter_constraint);
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
        if (native_search_row_matches(
                screen, nc_menu_active_item_at(menu, pos), &regex)) {
            result = nc_menu_goto_selectable(menu, pos);
            break;
        }
    }

    ncm_regex_destroy(&regex);
    return result;
}

void
native_search_engine_screen_set_bridge(NativeSearchEngineScreen *screen,
                                       NativeSearchEngineBridge bridge) {
    if (screen == NULL) {
        return;
    }
    screen->bridge = bridge;
    return;
}

static NativeSearchEngineScreen *
native_search_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
native_search_active_window(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.active_window != NULL) {
        return search->bridge.active_window(search->bridge.user);
    }
    return &search->window;
}

static void
native_search_refresh(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.refresh != NULL) {
        search->bridge.refresh(search->bridge.user);
        return;
    }
    nc_menu_prepare_refresh(native_search_engine_screen_menu(search),
                            search->main_height, NULL, NULL);
    return;
}

static void
native_search_refresh_window(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.refresh_window != NULL) {
        search->bridge.refresh_window(search->bridge.user);
        return;
    }
    native_search_refresh(screen);
    return;
}

static void
native_search_scroll(NcScreen *screen, enum NcScroll where) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.scroll != NULL) {
        search->bridge.scroll(search->bridge.user, where);
        return;
    }
    nc_menu_scroll_selectable(native_search_engine_screen_menu(search),
                              search->main_height, where);
    return;
}

static void
native_search_switch_to(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.switch_to != NULL) {
        search->bridge.switch_to(search->bridge.user);
    }
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
    if (search->bridge.resize != NULL) {
        search->bridge.resize(search->bridge.user);
    }
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
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.update != NULL) {
        search->bridge.update(search->bridge.user);
    }
    nc_screen_clear_update_request(screen);
    return;
}

static void
native_search_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.mouse_button_pressed != NULL) {
        search->bridge.mouse_button_pressed(search->bridge.user, event);
    }
    return;
}

static bool
native_search_can_run_current(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.can_run_current == NULL) {
        return false;
    }
    return search->bridge.can_run_current(search->bridge.user);
}

static bool
native_search_run_current(NcScreen *screen) {
    NativeSearchEngineScreen *search;

    search = native_search_from_screen(screen);
    if (search->bridge.run_current == NULL) {
        return false;
    }
    return search->bridge.run_current(search->bridge.user);
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
    return native_search_row_matches(
        screen, item, &screen->filter_regex);
}

static bool
native_search_row_matches(NativeSearchEngineScreen *screen,
                          NcSearchRow *row, NcmRegex *regex) {
    NcmStringView view;

    if (!native_search_row_label(screen, row, &view)) {
        return false;
    }
    return ncm_regex_search(regex, view.data, view.len);
}

static bool
native_search_row_label(NativeSearchEngineScreen *screen,
                        NcSearchRow *row, NcmStringView *view) {
    if (screen == NULL || row == NULL || view == NULL) {
        return false;
    }
    if (row->is_song) {
        if (screen->hooks.format_song != NULL) {
            ncm_buffer_clear(&screen->row_text);
            if (!screen->hooks.format_song(
                    screen->hooks.user, &row->song, &screen->row_text)) {
                return false;
            }
            *view = ncm_string_view_make(screen->row_text.data,
                                         screen->row_text.len);
            return true;
        }
        return ncm_song_uri_view(&row->song, 0, view);
    }
    *view = ncm_string_view_make(row->buffer.data, row->buffer.len);
    return true;
}

static bool
native_search_insert_buffer_with_flags(
    NativeSearchEngineScreen *screen, int64 pos, NcBuffer *buffer,
    uint32 flags) {
    NcSearchRow row;

    if (screen == NULL || buffer == NULL) {
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
    nc_buffer_append_cstring(&buffer, native_search_constraint_names[idx]);
    while (nc_buffer_len(&buffer) < 13) {
        nc_buffer_append_char(&buffer, ' ');
    }
    nc_buffer_append_cstring(&buffer, (char *)": ");
    nc_buffer_append_data(&buffer, screen->constraints[idx].data,
                          screen->constraints[idx].len);
    native_search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);
    nc_buffer_destroy(&buffer);
    return;
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
