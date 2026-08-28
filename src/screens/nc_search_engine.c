#if !defined(NCMPCPP_NC_SEARCH_ENGINE_C)
#define NCMPCPP_NC_SEARCH_ENGINE_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "ui_state.h"

static void search_display(SearchEngineScreen *screen);
static void search_switch_to(NcScreen *screen);
static void search_resize(NcScreen *screen);
static char *search_title(NcScreen *screen);
static void search_update(NcScreen *screen);
static void search_mouse_button_pressed(NcScreen *screen,
                                        MEVENT event);
static bool search_can_run_current(NcScreen *screen);
static bool search_run_current(NcScreen *screen);
static void search_draw_row(NcMenu *menu, NcWindow *window,
                            void *item, int32 pos, void *user);
static NcMenuDisplayCallbacks search_display_callbacks(
    SearchEngineScreen *screen, bool filtering);
static bool search_row_matches(SearchEngineScreen *screen,
                               NcSearchRow *row, NcmRegex *regex);
static bool search_find_position(NcMenu *menu, int32 pos, void *user);
static bool search_row_label(SearchEngineScreen *screen,
                             NcSearchRow *row, NcmStringView *view);
static void search_draw_classic_song(SearchEngineScreen *screen, NcMenu *menu,
                                     NcWindow *window, NcmSong *song,
                                     int32 pos);
static void search_draw_columns_song(SearchEngineScreen *screen, NcMenu *menu,
                                     NcWindow *window, NcmSong *song,
                                     int32 pos);
static bool search_format_columns(SearchEngineScreen *screen, NcmSong *song,
                                  NcBuffer *buffer, int32 list_width);
static int32 search_screen_width(SearchEngineScreen *screen);
static int32 search_menu_prefix_width(NcMenu *menu, int32 pos);
static int32 search_menu_suffix_width(NcMenu *menu, int32 pos);
static int32 search_buffer_width(NcBuffer *buffer);
static bool search_copy_song_at(SearchEngineScreen *screen,
                                NcmSongArray *songs, int32 pos);
static bool search_insert_buffer_with_flags(SearchEngineScreen *screen,
                                            int32 pos, NcBuffer *buffer,
                                            uint32 flags);
static void search_append_constraint_row(SearchEngineScreen *screen,
                                         int32 idx);
static bool search_set_buffer_row(SearchEngineScreen *screen, int32 pos,
                                  NcBuffer *buffer);
static void search_build_constraint_row(SearchEngineScreen *screen, int32 idx,
                                        NcBuffer *buffer);
static void search_build_search_source_row(SearchEngineScreen *screen,
                                           NcBuffer *buffer);
static void search_build_search_mode_row(SearchEngineScreen *screen,
                                         NcBuffer *buffer);
static void search_append_format(NcBuffer *buffer, enum NcFormat format);
static void search_append_tag_value(NcBuffer *buffer, StrBuilder *value);
static void search_print_buffer(NcWindow *window, NcBuffer *buffer);
static void search_mouse_scroll(SearchEngineScreen *screen,
                                enum NcScroll where);
static void search_print_error(SearchEngineScreen *screen, NcmError *ncm_error);
static bool search_has_constraints(SearchEngineScreen *screen);
static bool search_collect_database_results(SearchEngineScreen *screen,
                                            NcmMpdClient *client,
                                            NcmSongArray *songs,
                                            NcmError *ncm_error);
static bool search_add_database_constraints(SearchEngineScreen *screen,
                                            NcmMpdClient *client,
                                            NcmError *ncm_error);
static bool search_collect_local_results(SearchEngineScreen *screen,
                                         NcmSongArray *source,
                                         NcmSongArray *songs,
                                         NcmError *ncm_error);
static bool search_song_matches(SearchEngineScreen *screen, NcmSong *song,
                                NcmRegex *regexes);
static bool search_song_field_matches(SearchEngineScreen *screen,
                                      NcmSong *song, int32 field,
                                      NcmRegex *regex);
static bool search_song_any_matches(SearchEngineScreen *screen, NcmSong *song,
                                    NcmRegex *regex);
static bool search_song_field_view(NcmSong *song, int32 field,
                                   NcmStringView *view);
static bool search_append_result_rows(SearchEngineScreen *screen,
                                      NcmSongArray *songs);

typedef struct SearchFindContext {
    SearchEngineScreen *screen;
    NcmRegex *regex;
} SearchFindContext;

static char *search_constraint_names[] = {
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

static int32 search_constraint_name_lengths[] = {
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

static char *search_mode_names[] = {
    "Match if tag contains searched phrase (no regexes)",
    "Match if tag contains searched phrase (regexes supported)",
    "Match only if both values are the same",
};

static char search_empty_string[] = "";

#define NC_SCREEN_IMPL_TYPE SearchEngineScreen
#define NC_SCREEN_IMPL_PREFIX search
#define NC_SCREEN_IMPL_PUBLIC_PREFIX search_engine_screen
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_MENU(screen) search_engine_screen_menu(screen)
#define NC_SCREEN_IMPL_REFRESH_CALLBACK search_display
#define NC_SCREEN_IMPL_CAN_RUN_CURRENT_CALLBACK search_can_run_current
#define NC_SCREEN_IMPL_RUN_CURRENT_CALLBACK search_run_current
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK search_switch_to
#define NC_SCREEN_IMPL_RESIZE_CALLBACK search_resize
#define NC_SCREEN_IMPL_TITLE_CALLBACK search_title
#define NC_SCREEN_IMPL_UPDATE_CALLBACK search_update
#define NC_SCREEN_IMPL_MOUSE_CALLBACK search_mouse_button_pressed
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK \
    search_engine_screen_destroy
#define NC_SCREEN_IMPL_LOCKABLE true
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.h"

void
search_engine_screen_init(SearchEngineScreen *screen,
                          int32 start_x, int32 width,
                          int32 main_start_y, int32 main_height,
                          NcColor color, NcBorder border) {
    NcMenu *menu;

    nc_search_row_menu_init(&screen->rows);
    nc_window_init(&screen->window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    for (int32 i = 0; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        sb_init(&screen->constraints[i]);
    }
    sb_init(&screen->filter_constraint);
    sb_init(&screen->search_constraint);
    sb_init(&screen->row_text);
    sb_init(&screen->title);
    sb_init(&screen->column_title);
    SB_APPEND(&screen->title, STRLIT("Search engine"));
    ncm_regex_init(&screen->filter_regex);

    screen->hooks = (SearchEngineHooks){0};
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->lines_scrolled = 1;
    screen->result_count = 0;
    screen->search_mode = SEARCH_ENGINE_SEARCH_MODE_LITERAL;
    screen->search_in_database = true;
    screen->mouse_list_scroll_whole_page = false;
    screen->match_to_pattern = false;
    screen->filter_enabled = false;
    screen->prepared = false;
    screen->result_rows_present = false;
    screen->constraints_locked = false;
    screen->registered = false;

    nc_screen_init_ops(&screen->screen, search_ops, screen,
                       NC_SCREEN_TYPE_SEARCH_ENGINE);
    menu = search_engine_screen_menu(screen);
    nc_menu_set_display_callbacks(menu,
                                  search_display_callbacks(screen, false));
    nc_menu_set_selected_prefix(menu, &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(menu, &Config.selected_item_suffix);
    nc_menu_set_highlight_prefix(menu, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(menu, &Config.current_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    search_engine_screen_update_column_title(screen);
    return;
}

void
search_engine_screen_destroy(SearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    sb_free(&screen->filter_constraint);
    sb_free(&screen->row_text);
    sb_free(&screen->title);
    sb_free(&screen->column_title);
    sb_free(&screen->search_constraint);
    for (int32 i = 0; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        sb_free(&screen->constraints[i]);
    }
    nc_window_destroy(&screen->window);
    nc_search_row_menu_destroy(&screen->rows);
    return;
}

NcMenu *
search_engine_screen_menu(SearchEngineScreen *screen) {
    return nc_search_row_menu_base(&screen->rows);
}

NcWindow *
search_engine_screen_window(SearchEngineScreen *screen) {
    return &screen->window;
}

void
search_engine_screen_set_geometry(SearchEngineScreen *screen,
                                  int32 start_x, int32 width,
                                  int32 main_start_y, int32 main_height) {
    int32 window_height;
    int32 window_start_y;

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
    search_engine_screen_update_column_title(screen);
    return;
}

void
search_engine_screen_set_mouse_config(SearchEngineScreen *screen,
                                      int32 lines_scrolled,
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
search_engine_screen_set_display_mode(SearchEngineScreen *screen,
                                      enum DisplayMode mode) {
    if ((screen == NULL) || ((mode != NCM_DISPLAY_MODE_CLASSIC)
                             && (mode != NCM_DISPLAY_MODE_COLUMNS))) {
        return;
    }
    Config.search_engine_display_mode = mode;
    search_engine_screen_update_column_title(screen);
    return;
}

void
search_engine_screen_clear(SearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(search_engine_screen_menu(screen));
    screen->prepared = false;
    screen->result_rows_present = false;
    screen->result_count = 0;
    screen->constraints_locked = false;
    return;
}

char *
search_engine_constraint_name(int32 idx) {
    if ((idx < 0) || (idx >= SEARCH_ENGINE_CONSTRAINT_COUNT)) {
        return NULL;
    }
    return search_constraint_names[idx];
}

char *
search_engine_search_mode_name(enum SearchEngineSearchMode mode) {
    if ((mode < SEARCH_ENGINE_SEARCH_MODE_LITERAL)
        || (mode >= SEARCH_ENGINE_SEARCH_MODE_COUNT)) {
        return search_mode_names[0];
    }
    return search_mode_names[mode];
}

bool
search_engine_screen_constraints_locked(SearchEngineScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->constraints_locked;
}

bool
search_engine_screen_format_song_text(SearchEngineScreen *screen,
                                      NcmSong *song, StrBuilder *text) {
    NcBuffer formatted;
    bool result;

    if ((screen == NULL) || (song == NULL) || (text == NULL)) {
        return false;
    }

    nc_buffer_init(&formatted);
    if (Config.search_engine_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        result = search_format_columns(screen, song, &formatted,
                                       search_screen_width(screen));
    } else {
        ncm_display_song_row(&formatted, &Config.song_list_format, song,
                             NCM_FORMAT_FLAG_ALL);
        result = true;
    }
    if (result) {
        result = sb_set(text, formatted.data, formatted.len) >= 0;
    }
    nc_buffer_destroy(&formatted);
    return result;
}

void
search_engine_screen_update_column_title(SearchEngineScreen *screen) {
    int32 list_width;

    if (screen == NULL) {
        return;
    }

    sb_clear(&screen->column_title);
    if ((Config.search_engine_display_mode != NCM_DISPLAY_MODE_COLUMNS)
        || !Config.titles_visibility || (Config.columns.items == NULL)
        || (Config.columns.len <= 0) || (screen->main_height <= 2)) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    list_width = search_screen_width(screen);
    if (list_width <= 0) {
        nc_window_set_title(&screen->window, NULL, 0);
        return;
    }

    ncm_display_column_title(&screen->column_title, Config.columns.items,
                             Config.columns.len, list_width);
    nc_window_set_title(&screen->window, screen->column_title.data,
                        screen->column_title.len);
    return;
}

void
search_engine_screen_prepare_static_rows(SearchEngineScreen *screen) {
    NcBuffer buffer;

    if (screen == NULL) {
        return;
    }

    search_engine_screen_clear(screen);
    for (int32 i = 0; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        search_append_constraint_row(screen, i);
    }

    nc_buffer_init(&buffer);
    search_engine_screen_add_buffer_with_flags(
        screen, &buffer, NC_MENU_ITEM_SEPARATOR);

    search_build_search_source_row(screen, &buffer);
    search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);

    search_build_search_mode_row(screen, &buffer);
    search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);

    nc_buffer_clear(&buffer);
    search_engine_screen_add_buffer_with_flags(
        screen, &buffer, NC_MENU_ITEM_SEPARATOR);

    nc_buffer_append_data(&buffer, STRLIT("Search"));
    search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);

    nc_buffer_clear(&buffer);
    nc_buffer_append_data(&buffer, STRLIT("Reset"));
    search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);
    nc_buffer_destroy(&buffer);

    nc_menu_reset(search_engine_screen_menu(screen));
    screen->prepared = true;
    screen->result_rows_present = false;
    screen->result_count = 0;
    screen->constraints_locked = false;
    return;
}

bool
search_engine_screen_update_constraint_row(SearchEngineScreen *screen,
                                           int32 idx) {
    NcBuffer buffer;
    bool result;

    if ((screen == NULL) || !screen->prepared || (idx < 0)
        || (idx >= SEARCH_ENGINE_CONSTRAINT_COUNT)) {
        return false;
    }

    nc_buffer_init(&buffer);
    search_build_constraint_row(screen, idx, &buffer);
    result = search_set_buffer_row(screen, idx, &buffer);
    nc_buffer_destroy(&buffer);
    return result;
}

bool
search_engine_screen_update_search_source_row(SearchEngineScreen *screen) {
    NcBuffer buffer;
    bool result;

    if ((screen == NULL) || !screen->prepared) {
        return false;
    }

    nc_buffer_init(&buffer);
    search_build_search_source_row(screen, &buffer);
    result = search_set_buffer_row(
        screen, SEARCH_ENGINE_SEARCH_SOURCE_ROW, &buffer);
    nc_buffer_destroy(&buffer);
    return result;
}

bool
search_engine_screen_update_search_mode_row(SearchEngineScreen *screen) {
    NcBuffer buffer;
    bool result;

    if ((screen == NULL) || !screen->prepared) {
        return false;
    }

    nc_buffer_init(&buffer);
    search_build_search_mode_row(screen, &buffer);
    result = search_set_buffer_row(
        screen, SEARCH_ENGINE_SEARCH_MODE_ROW, &buffer);
    nc_buffer_destroy(&buffer);
    return result;
}

bool
search_engine_screen_add_result_summary(SearchEngineScreen *screen,
                                        int32 song_count) {
    NcBuffer buffer;
    bool result;

    if ((screen == NULL) || (song_count <= 0)) {
        return false;
    }

    nc_buffer_init(&buffer);
    result = search_insert_buffer_with_flags(
        screen, SEARCH_ENGINE_RESULT_SEPARATOR_ROW, &buffer,
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
    result = search_insert_buffer_with_flags(
        screen, SEARCH_ENGINE_RESULT_SUMMARY_ROW, &buffer,
        NC_MENU_ITEM_INACTIVE);
    if (!result) {
        nc_buffer_destroy(&buffer);
        return false;
    }

    nc_buffer_clear(&buffer);
    result = search_insert_buffer_with_flags(
        screen, SEARCH_ENGINE_RESULT_END_SEPARATOR_ROW, &buffer,
        NC_MENU_ITEM_SEPARATOR);
    nc_buffer_destroy(&buffer);
    if (result) {
        screen->result_rows_present = true;
        screen->result_count = song_count;
    }
    return result;
}

void
search_engine_screen_set_constraints_locked(SearchEngineScreen *screen,
                                            bool locked) {
    NcMenu *menu;
    uint32 flags;

    if (screen == NULL) {
        return;
    }
    screen->constraints_locked = locked;
    menu = search_engine_screen_menu(screen);
    if (nc_menu_all_item_count(menu)
        <= SEARCH_ENGINE_SEARCH_BUTTON_ROW) {
        return;
    }

    for (int32 i = 0; i <= SEARCH_ENGINE_SEARCH_BUTTON_ROW; i += 1) {
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
search_engine_screen_reset(SearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    for (int32 i = 0; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        sb_clear(&screen->constraints[i]);
    }
    search_engine_screen_clear_filter(screen);
    search_engine_screen_clear_find_constraint(screen);
    search_engine_screen_prepare_static_rows(screen);
    search_engine_screen_status_message(screen, STRLIT("Search state reset"));
    return;
}

bool
search_engine_screen_add_song_copy(SearchEngineScreen *screen,
                                   NcmSong *song) {
    return search_engine_screen_add_song_copy_with_flags(
        screen, song, NC_MENU_ITEM_SELECTABLE);
}

bool
search_engine_screen_add_song_copy_with_flags(SearchEngineScreen *screen,
                                              NcmSong *song, uint32 flags) {
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
search_engine_screen_add_buffer_with_flags(SearchEngineScreen *screen,
                                           NcBuffer *buffer, uint32 flags) {
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
search_engine_screen_set_constraint(SearchEngineScreen *screen,
                                    int32 idx, char *data,
                                    int32 data_len) {
    if ((screen == NULL) || (idx < 0)
        || (idx >= SEARCH_ENGINE_CONSTRAINT_COUNT)) {
        return false;
    }
    if (sb_set(&screen->constraints[idx], data, data_len) < 0) {
        return false;
    }
    if (screen->prepared) {
        return search_engine_screen_update_constraint_row(screen, idx);
    }
    return true;
}

void
search_engine_screen_clear_find_constraint(SearchEngineScreen *screen) {
    if (screen == NULL) {
        return;
    }
    sb_clear(&screen->search_constraint);
    screen->match_to_pattern = false;
    return;
}

bool
search_engine_screen_set_search_mode(SearchEngineScreen *screen,
                                     enum SearchEngineSearchMode mode) {
    if ((screen == NULL) || (mode < SEARCH_ENGINE_SEARCH_MODE_LITERAL)
        || (mode >= SEARCH_ENGINE_SEARCH_MODE_COUNT)) {
        return false;
    }
    screen->search_mode = mode;
    if (screen->prepared) {
        return search_engine_screen_update_search_mode_row(screen);
    }
    return true;
}

void
search_engine_screen_set_search_source(SearchEngineScreen *screen,
                                       bool search_in_database) {
    if (screen == NULL) {
        return;
    }
    screen->search_in_database = search_in_database;
    if (screen->prepared) {
        (void)search_engine_screen_update_search_source_row(screen);
    }
    return;
}

void
search_engine_screen_set_hooks(SearchEngineScreen *screen,
                               SearchEngineHooks hooks) {
    if (screen == NULL) {
        return;
    }
    screen->hooks = hooks;
    return;
}

bool
search_engine_screen_list_database_songs(SearchEngineScreen *screen,
                                         NcmSongArray *songs,
                                         NcmError *ncm_error) {
    if ((screen == NULL) || (songs == NULL)
        || (screen->hooks.list_database_songs == NULL)) {
        return false;
    }
    return screen->hooks.list_database_songs(screen->hooks.user,
                                             songs, ncm_error);
}

bool
search_engine_screen_snapshot_playlist(SearchEngineScreen *screen,
                                       NcmSongArray *songs,
                                       NcmError *ncm_error) {
    if ((screen == NULL) || (songs == NULL)
        || (screen->hooks.snapshot_playlist == NULL)) {
        return false;
    }
    return screen->hooks.snapshot_playlist(screen->hooks.user,
                                           songs, ncm_error);
}

enum SearchEnginePromptResult
search_engine_screen_prompt_constraint(SearchEngineScreen *screen, int32 idx,
                                       StrBuilder *result) {
    char *label;
    int32 label_len;
    if ((screen == NULL) || (result == NULL) || (idx < 0)
        || (idx >= SEARCH_ENGINE_CONSTRAINT_COUNT)
        || (screen->hooks.prompt_constraint == NULL)) {
        return SEARCH_ENGINE_PROMPT_ERROR;
    }

    label = search_engine_constraint_name(idx);
    label_len = search_constraint_name_lengths[idx];

    return screen->hooks.prompt_constraint(screen->hooks.user,
                                           label, label_len,
                                           &screen->constraints[idx], result);
}

void
search_engine_screen_status_message(SearchEngineScreen *screen, char *message,
                                    int32 message_len) {
    if ((screen == NULL) || (message == NULL) || (message_len < 0)
        || (screen->hooks.status_message == NULL)) {
        return;
    }
    screen->hooks.status_message(screen->hooks.user, message, message_len);
    return;
}

bool
search_engine_screen_add_song(SearchEngineScreen *screen, NcmSong *song,
                              bool play, NcmError *ncm_error) {
    if ((screen == NULL) || (song == NULL)
        || (screen->hooks.add_song == NULL)) {
        return false;
    }
    return screen->hooks.add_song(screen->hooks.user, song, play, ncm_error);
}

bool
search_engine_screen_execute_search(SearchEngineScreen *screen,
                                    NcmMpdClient *client, NcmError *ncm_error) {
    NcmSongArray source;
    NcmSongArray songs;
    bool result;

    if (screen == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing search screen"));
        return false;
    }

    ncm_song_array_init(&source);
    ncm_song_array_init(&songs);
    ncm_error_clear(ncm_error);
    search_engine_screen_clear_filter(screen);
    search_engine_screen_prepare_static_rows(screen);
    search_engine_screen_status_message(screen, STRLIT("Searching..."));

    result = true;
    if (!search_has_constraints(screen)) {
        search_engine_screen_status_message(screen, STRLIT("No results found"));
        goto cleanup;
    }

    if (screen->search_in_database
        && ((screen->search_mode
             == SEARCH_ENGINE_SEARCH_MODE_LITERAL)
            || (screen->search_mode
                == SEARCH_ENGINE_SEARCH_MODE_EXACT))) {
        result = search_collect_database_results(
            screen, client, &songs, ncm_error);
    } else {
        if (screen->search_in_database) {
            result = search_engine_screen_list_database_songs(
                screen, &source, ncm_error);
        } else {
            result = search_engine_screen_snapshot_playlist(
                screen, &source, ncm_error);
        }
        if (result) {
            result = search_collect_local_results(screen, &source, &songs,
                                                  ncm_error);
        }
    }

    if (!result) {
        search_engine_screen_prepare_static_rows(screen);
        search_print_error(screen, ncm_error);
        goto cleanup;
    }

    if (songs.len <= 0) {
        search_engine_screen_status_message(screen, STRLIT("No results found"));
        goto cleanup;
    }

    if (!search_append_result_rows(screen, &songs)) {
        ncm_error_set(ncm_error, EIO,
                      STRLIT("failed to build search results"));
        search_engine_screen_prepare_static_rows(screen);
        search_print_error(screen, ncm_error);
        result = false;
        goto cleanup;
    }

    search_engine_screen_set_constraints_locked(
        screen, Config.block_search_constraints_change);
    nc_menu_scroll_selectable(
        search_engine_screen_menu(screen),
        nc_window_height(&screen->window), NC_SCROLL_DOWN);
    nc_menu_scroll_selectable(
        search_engine_screen_menu(screen),
        nc_window_height(&screen->window), NC_SCROLL_DOWN);
    search_engine_screen_update_column_title(screen);
    search_engine_screen_status_message(screen, STRLIT("Searching finished"));

cleanup:
    ncm_song_array_destroy(&songs);
    ncm_song_array_destroy(&source);
    return result;
}

bool
search_engine_screen_can_run_current(SearchEngineScreen *screen) {
    NcMenu *menu;
    NcSearchRow *row;
    int32 pos;

    if ((screen == NULL) || !screen->prepared) {
        return false;
    }

    menu = search_engine_screen_menu(screen);
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
search_engine_screen_run_current(SearchEngineScreen *screen) {
    enum SearchEnginePromptResult prompt_status;
    enum SearchEngineSearchMode mode;
    StrBuilder value = {0};
    NcmError ncm_error;
    NcMenu *menu;
    int32 pos;
    uint32 next_mode;
    bool success;

    if (!search_engine_screen_can_run_current(screen)) {
        return false;
    }

    menu = search_engine_screen_menu(screen);
    pos = nc_menu_highlight(menu);
    if (pos < SEARCH_ENGINE_CONSTRAINT_COUNT) {
        prompt_status = search_engine_screen_prompt_constraint(
            screen, pos, &value);
        if (prompt_status == SEARCH_ENGINE_PROMPT_ACCEPTED) {
            success = search_engine_screen_set_constraint(
                screen, pos, value.data, value.len);
            sb_free(&value);
            return success;
        }
        sb_free(&value);
        if (prompt_status == SEARCH_ENGINE_PROMPT_ABORTED) {
            search_engine_screen_status_message(
                screen, STRLIT("Action aborted"));
        } else {
            search_engine_screen_status_message(
                screen, STRLIT("Unable to read search constraint"));
        }
        return false;
    }

    if (pos == SEARCH_ENGINE_SEARCH_SOURCE_ROW) {
        screen->search_in_database = !screen->search_in_database;
        Config.search_in_db = screen->search_in_database;
        return search_engine_screen_update_search_source_row(screen);
    }
    if (pos == SEARCH_ENGINE_SEARCH_MODE_ROW) {
        next_mode = (uint32)screen->search_mode + 1;
        if (next_mode >= SEARCH_ENGINE_SEARCH_MODE_COUNT) {
            next_mode = SEARCH_ENGINE_SEARCH_MODE_LITERAL;
        }
        mode = (enum SearchEngineSearchMode)next_mode;
        return search_engine_screen_set_search_mode(screen, mode);
    }
    if (pos == SEARCH_ENGINE_SEARCH_BUTTON_ROW) {
        ncm_error_clear(&ncm_error);
        return search_engine_screen_start_searching(screen,
                                                    screen->hooks.client,
                                                    &ncm_error);
    }
    if (pos == SEARCH_ENGINE_RESET_BUTTON_ROW) {
        search_engine_screen_reset(screen);
        return true;
    }
    return false;
}

bool
search_engine_screen_start_searching(SearchEngineScreen *screen,
                                     NcmMpdClient *client, NcmError *ncm_error) {
    NcMenu *menu;

    if ((screen == NULL) || screen->constraints_locked) {
        return false;
    }
    if (!screen->prepared) {
        search_engine_screen_prepare_static_rows(screen);
    }

    menu = search_engine_screen_menu(screen);
    nc_menu_highlight_position(
        menu, SEARCH_ENGINE_SEARCH_BUTTON_ROW,
        nc_window_height(&screen->window));
    return search_engine_screen_execute_search(screen, client, ncm_error);
}

enum DisplayMode
search_engine_screen_toggle_display_mode(SearchEngineScreen *screen) {
    enum DisplayMode mode = Config.search_engine_display_mode;

    if (screen == NULL) {
        return Config.search_engine_display_mode;
    }

    if (mode == NCM_DISPLAY_MODE_CLASSIC) {
        mode = NCM_DISPLAY_MODE_COLUMNS;
    } else {
        mode = NCM_DISPLAY_MODE_CLASSIC;
    }
    search_engine_screen_set_display_mode(screen, mode);
    return mode;
}

bool
search_engine_screen_allows_search(SearchEngineScreen *screen) {
    NcMenu *menu;
    NcSearchRow *row;
    int32 count;

    if (screen == NULL) {
        return false;
    }
    menu = search_engine_screen_menu(screen);
    count = nc_menu_item_count(menu);
    if (count <= 0) {
        return false;
    }
    if ((row = nc_menu_active_item_at(menu, count - 1)) == NULL) {
        return false;
    }
    return row->is_song;
}

bool
search_engine_screen_current_song(SearchEngineScreen *screen,
                                  NcmSong *song) {
    NcSearchRow *row;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    if (((row = nc_search_row_menu_current(&screen->rows)) == NULL)
        || !row->is_song) {
        return false;
    }
    return ncm_song_copy(song, &row->song);
}

bool
search_engine_screen_selected_songs(SearchEngineScreen *screen,
                                    NcmSongArray *songs) {
    NcMenu *menu;

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }
    menu = search_engine_screen_menu(screen);
    if (!nc_menu_has_selected(menu)) {
        return search_copy_song_at(screen, songs, nc_menu_highlight(menu));
    }
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!search_copy_song_at(screen, songs, i)) {
            return false;
        }
    }
    return true;
}

bool
search_engine_screen_apply_filter(SearchEngineScreen *screen,
                                  char *pattern, int32 pattern_len,
                                  NcmError *ncm_error) {
    NcMenuDisplayCallbacks callbacks;

    if (screen == NULL) {
        return false;
    }
    if (pattern_len <= 0) {
        search_engine_screen_clear_filter(screen);
        return true;
    }
    if (!ncm_regex_compile(&screen->filter_regex, pattern, pattern_len,
                           NCM_REGEX_LITERAL_CASE_INSENSITIVE, ncm_error)) {
        return false;
    }
    if (sb_set(&screen->filter_constraint, pattern, pattern_len) < 0) {
        return false;
    }
    callbacks = search_display_callbacks(screen, true);
    nc_menu_set_display_callbacks(search_engine_screen_menu(screen),
                                  callbacks);
    screen->filter_enabled = true;
    nc_menu_apply_filter(search_engine_screen_menu(screen));
    return true;
}

void
search_engine_screen_clear_filter(SearchEngineScreen *screen) {
    NcMenuDisplayCallbacks callbacks;

    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->filter_regex);
    ncm_regex_init(&screen->filter_regex);
    sb_clear(&screen->filter_constraint);
    screen->filter_enabled = false;
    callbacks = search_display_callbacks(screen, false);
    nc_menu_set_display_callbacks(search_engine_screen_menu(screen),
                                  callbacks);
    nc_menu_show_all_items(search_engine_screen_menu(screen));
    return;
}

bool
search_engine_screen_search(SearchEngineScreen *screen,
                            char *pattern, int32 pattern_len,
                            bool forward, bool wrap, bool skip_current,
                            NcmError *ncm_error) {
    SearchFindContext context;
    NcmRegex regex;
    NcMenu *menu;
    bool result;

    if ((screen == NULL) || (pattern == NULL) || (pattern_len <= 0)) {
        return false;
    }

    ncm_regex_init(&regex);
    if (!ncm_regex_compile(&regex, pattern, pattern_len,
                           NCM_REGEX_LITERAL_CASE_INSENSITIVE, ncm_error)) {
        ncm_regex_destroy(&regex);
        return false;
    }

    menu = search_engine_screen_menu(screen);
    context.screen = screen;
    context.regex = &regex;
    result = nc_menu_search_selectable(menu, screen->main_height, forward,
                                       wrap, skip_current,
                                       search_find_position,
                                       &context, NULL);

    ncm_regex_destroy(&regex);
    return result;
}

static bool
search_find_position(NcMenu *menu, int32 pos, void *user) {
    SearchFindContext *context = user;

    return search_row_matches(context->screen,
                              nc_menu_active_item_at(menu, pos),
                              context->regex);
}

static void
search_display(SearchEngineScreen *search) {
    NcMenu *menu;
    NcWindow *window;
    if (!search->prepared) {
        search_engine_screen_prepare_static_rows(search);
    }
    search_engine_screen_update_column_title(search);

    menu = search_engine_screen_menu(search);
    window = search_engine_screen_window(search);

    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
search_switch_to(NcScreen *screen) {
    SearchEngineScreen *search = search_from_screen(screen);

    if (!search->prepared) {
        search_engine_screen_prepare_static_rows(search);
    }
    search_engine_screen_update_column_title(search);
    return;
}

static void
search_resize(NcScreen *screen) {
    SearchEngineScreen *search = search_from_screen(screen);
    int32 start_x;
    int32 width;

    nc_screen_get_resize_params(screen, &start_x, &width);
    search_engine_screen_set_geometry(
        search, start_x, width, ui_state_main_start_y(),
        ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
search_title(NcScreen *screen) {
    SearchEngineScreen *search = search_from_screen(screen);

    return search->title.data;
}

static void
search_update(NcScreen *screen) {
    nc_screen_clear_update_request(screen);
    return;
}

static void
search_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    SearchEngineScreen *search = search_from_screen(screen);
    NcMenu *menu;
    NcSearchRow *row;
    NcWindow *window = search_engine_screen_window(search);
    NcmError ncm_error;
    int32 x = event.x;
    int32 y = event.y;
    bool play;

    if (!nc_window_has_coords(window, &x, &y)) {
        return;
    }

    menu = search_engine_screen_menu(search);
    if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
        if (!nc_menu_goto_selectable(menu, y)) {
            return;
        }
        if ((row = nc_search_row_menu_current(&search->rows)) == NULL) {
            return;
        }
        if (!row->is_song) {
            if (event.bstate & BUTTON3_PRESSED) {
                (void)nc_screen_run_current(screen);
            }
            return;
        }

        play = (event.bstate & BUTTON3_PRESSED) != 0;
        ncm_error_clear(&ncm_error);
        if (!search_engine_screen_add_song(
            search, &row->song, play, &ncm_error)) {
            search_print_error(search, &ncm_error);
        }
        return;
    }

    if (event.bstate & BUTTON5_PRESSED) {
        search_mouse_scroll(search, NC_SCROLL_DOWN);
    } else if (event.bstate & BUTTON4_PRESSED) {
        search_mouse_scroll(search, NC_SCROLL_UP);
    }
    return;
}

static bool
search_can_run_current(NcScreen *screen) {
    return search_engine_screen_can_run_current(search_from_screen(screen));
}

static bool
search_run_current(NcScreen *screen) {
    return search_engine_screen_run_current(search_from_screen(screen));
}

static bool
search_filter_row(NcMenu *menu, void *item, void *user) {
    SearchEngineScreen *screen = user;
    NcSearchRow *row = item;

    (void)menu;
    if ((row == NULL) || !row->is_song) {
        return true;
    }
    return search_row_matches(screen, row, &screen->filter_regex);
}

static bool
search_row_matches(SearchEngineScreen *screen,
                   NcSearchRow *row, NcmRegex *regex) {
    NcmStringView view;

    if ((row == NULL) || !row->is_song
        || !search_row_label(screen, row, &view)) {
        return false;
    }
    return ncm_regex_search(regex, view.data, view.len);
}

static bool
search_row_label(SearchEngineScreen *screen,
                 NcSearchRow *row, NcmStringView *view) {
    ASSERT(screen != NULL);
    if ((row == NULL) || (view == NULL)) {
        return false;
    }
    if (row->is_song) {
        if (screen->hooks.format_song) {
            sb_clear(&screen->row_text);
            if (!screen->hooks.format_song(
                screen->hooks.user, &row->song, &screen->row_text)) {
                return false;
            }
            *view = ncm_string_view_make(screen->row_text.data,
                                         screen->row_text.len);
            return true;
        }
        if (!search_engine_screen_format_song_text(
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
search_insert_buffer_with_flags(SearchEngineScreen *screen,
                                int32 pos, NcBuffer *buffer,
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
search_append_constraint_row(SearchEngineScreen *screen, int32 idx) {
    NcBuffer buffer;

    nc_buffer_init(&buffer);
    search_build_constraint_row(screen, idx, &buffer);
    search_engine_screen_add_buffer_with_flags(screen, &buffer, 0);
    nc_buffer_destroy(&buffer);
    return;
}

static bool
search_set_buffer_row(SearchEngineScreen *screen, int32 pos,
                      NcBuffer *buffer) {
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
search_build_constraint_row(SearchEngineScreen *screen, int32 idx,
                            NcBuffer *buffer) {
    nc_buffer_clear(buffer);
    search_append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_cstring(buffer, search_constraint_names[idx]);
    while (nc_buffer_len(buffer) < 13) {
        nc_buffer_append_char(buffer, ' ');
    }
    search_append_format(buffer, NC_FORMAT_NO_BOLD);
    nc_buffer_append_data(buffer, STRLIT(": "));
    search_append_tag_value(buffer, &screen->constraints[idx]);
    return;
}

static void
search_build_search_source_row(SearchEngineScreen *screen, NcBuffer *buffer) {
    nc_buffer_clear(buffer);
    search_append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_data(buffer, STRLIT("Search in:"));
    search_append_format(buffer, NC_FORMAT_NO_BOLD);
    nc_buffer_append_char(buffer, ' ');
    if (screen->search_in_database) {
        nc_buffer_append_data(buffer, STRLIT("Database"));
    } else {
        nc_buffer_append_data(buffer, STRLIT("Current playlist"));
    }
    return;
}

static void
search_build_search_mode_row(SearchEngineScreen *screen, NcBuffer *buffer) {
    nc_buffer_clear(buffer);
    search_append_format(buffer, NC_FORMAT_BOLD);
    nc_buffer_append_data(buffer, STRLIT("Search mode:"));
    search_append_format(buffer, NC_FORMAT_NO_BOLD);
    nc_buffer_append_char(buffer, ' ');
    nc_buffer_append_cstring(
        buffer, search_engine_search_mode_name(screen->search_mode));
    return;
}

static void
search_append_format(NcBuffer *buffer, enum NcFormat format) {
    nc_buffer_add_format(buffer, nc_buffer_len(buffer), format, 0);
    return;
}

static void
search_append_tag_value(NcBuffer *buffer, StrBuilder *value) {
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
search_display_callbacks(SearchEngineScreen *screen,
                         bool filtering) {
    NcMenuDisplayCallbacks callbacks;

    callbacks = (NcMenuDisplayCallbacks){0};
    callbacks.draw = search_draw_row;
    if (filtering) {
        callbacks.filter = search_filter_row;
    }
    callbacks.user = screen;
    return callbacks;
}

static void
search_draw_row(NcMenu *menu, NcWindow *window, void *item,
                int32 pos, void *user) {
    SearchEngineScreen *screen = user;
    NcSearchRow *row = item;

    if ((screen == NULL) || (window == NULL) || (row == NULL)) {
        return;
    }

    if (!row->is_song) {
        search_print_buffer(window, &row->buffer);
        return;
    }

    if (Config.search_engine_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        search_draw_columns_song(screen, menu, window, &row->song, pos);
    } else {
        search_draw_classic_song(screen, menu, window, &row->song, pos);
    }
    return;
}

static void
search_draw_classic_song(SearchEngineScreen *screen, NcMenu *menu,
                         NcWindow *window, NcmSong *song, int32 pos) {
    NcBuffer left;
    NcBuffer right;
    int32 right_width;
    int32 right_x;
    int32 y;

    nc_buffer_init(&left);
    nc_buffer_init(&right);
    ncm_format_render_buffer(&Config.song_list_format, song,
                             &left, &right, NCM_FORMAT_FLAG_ALL);
    search_print_buffer(window, &left);

    if (right.len > 0) {
        right_width = search_buffer_width(&right);
        right_x = search_screen_width(screen)
                  - search_menu_suffix_width(menu, pos)
                  - right_width;
        if (right_x < 0) {
            right_x = 0;
        }
        y = nc_window_get_y(window);
        nc_window_go_to_xy(window, right_x, y);
        search_print_buffer(window, &right);
    }

    nc_buffer_destroy(&left);
    nc_buffer_destroy(&right);
    return;
}

static void
search_draw_columns_song(SearchEngineScreen *screen, NcMenu *menu,
                         NcWindow *window, NcmSong *song, int32 pos) {
    NcBuffer buffer;
    int32 width;

    width = search_screen_width(screen)
            - search_menu_prefix_width(menu, pos)
            - search_menu_suffix_width(menu, pos);
    if (width < 0) {
        width = 0;
    }

    nc_buffer_init(&buffer);
    (void)search_format_columns(screen, song, &buffer, width);
    search_print_buffer(window, &buffer);
    nc_buffer_destroy(&buffer);
    return;
}

static bool
search_format_columns(SearchEngineScreen *screen, NcmSong *song,
                      NcBuffer *buffer, int32 list_width) {
    ASSERT(screen != NULL);
    if ((song == NULL) || (buffer == NULL)) {
        return false;
    }

    ncm_display_song_columns(buffer, song, Config.columns.items,
                             Config.columns.len, list_width, true);
    return true;
}

static int32
search_screen_width(SearchEngineScreen *screen) {
    ASSERT(screen != NULL);
    if (screen->width <= 0) {
        return 0;
    }
    if (screen->width > INT32_MAX) {
        return INT32_MAX;
    }
    return screen->width;
}

static int32
search_menu_prefix_width(NcMenu *menu, int32 pos) {
    int32 width;

    if (menu == NULL) {
        return 0;
    }

    width = 0;
    if (menu->highlight_enabled && (pos == menu->highlight)) {
        width += search_buffer_width(&menu->highlight_prefix);
    }
    if (nc_menu_position_is_selected(menu, pos)) {
        width += search_buffer_width(&menu->selected_prefix);
    }
    return width;
}

static int32
search_menu_suffix_width(NcMenu *menu, int32 pos) {
    int32 width;

    if (menu == NULL) {
        return 0;
    }

    width = 0;
    if (nc_menu_position_is_selected(menu, pos)) {
        width += search_buffer_width(&menu->selected_suffix);
    }
    if (menu->highlight_enabled && (pos == menu->highlight)) {
        width += search_buffer_width(&menu->highlight_suffix);
    }
    return width;
}

static int32
search_buffer_width(NcBuffer *buffer) {
    if ((buffer == NULL) || (buffer->len <= 0)) {
        return 0;
    }
    return utf8_width(buffer->data, buffer->len);
}

static void
search_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties = nc_buffer_properties(buffer);
    char *data = nc_buffer_data(buffer);
    int32 property_count = nc_buffer_property_count(buffer);
    int32 property_index;
    int32 len = nc_buffer_len(buffer);

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
search_mouse_scroll(SearchEngineScreen *screen,
                    enum NcScroll where) {
    NcMenu *menu = search_engine_screen_menu(screen);
    enum NcScroll effective = where;
    int32 count = screen->lines_scrolled;

    if (screen->mouse_list_scroll_whole_page) {
        count = 1;
        if (where == NC_SCROLL_UP) {
            effective = NC_SCROLL_PAGE_UP;
        } else {
            effective = NC_SCROLL_PAGE_DOWN;
        }
    }

    for (int32 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(menu, nc_window_height(&screen->window),
                                  effective);
    }
    return;
}

static bool
search_has_constraints(SearchEngineScreen *screen) {
    ASSERT(screen != NULL);

    for (int32 i = 0; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        if (screen->constraints[i].len > 0) {
            return true;
        }
    }
    return false;
}

static bool
search_collect_database_results(SearchEngineScreen *screen,
                                NcmMpdClient *client,
                                NcmSongArray *songs, NcmError *ncm_error) {
    NcmMpdSongList result;
    bool ok;
    bool exact_match;

    if ((screen == NULL) || (client == NULL) || (songs == NULL)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing database search state"));
        return false;
    }

    exact_match = screen->search_mode == SEARCH_ENGINE_SEARCH_MODE_EXACT;
    ncm_mpd_song_list_init(&result);
    if ((ok = ncm_mpd_client_start_search(client, exact_match, ncm_error))) {
        ok = search_add_database_constraints(
            screen, client, ncm_error);
    }
    if (ok) {
        ok = ncm_mpd_client_commit_search_songs(
            client, &result, ncm_error);
    }
    if (ok) {
        if (!(ok = ncm_mpd_song_list_to_song_array(&result, songs))) {
            ncm_error_set(ncm_error, EIO,
                          STRLIT("failed to copy search results"));
        }
    }
    ncm_mpd_song_list_destroy(&result);
    return ok;
}

static bool
search_add_database_constraints(SearchEngineScreen *screen,
                                NcmMpdClient *client, NcmError *ncm_error) {
    StrBuilder *constraint;

    constraint = &screen->constraints[0];
    if ((constraint->len > 0)
        && !ncm_mpd_client_add_search_any(
            client, constraint->data, ncm_error)) {
        return false;
    }

    for (int32 i = 1; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        enum mpd_tag_type tag;

        constraint = &screen->constraints[i];
        if (constraint->len <= 0) {
            continue;
        }
        if (i == 5) {
            if (!ncm_mpd_client_add_search_uri(
                client, constraint->data, ncm_error)) {
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
            ncm_error_set(ncm_error, EINVAL,
                          STRLIT("invalid search constraint"));
            return false;
        }
        if (!ncm_mpd_client_add_search_tag(
            client, tag, constraint->data, ncm_error)) {
            return false;
        }
    }
    return true;
}

static bool
search_collect_local_results(SearchEngineScreen *screen, NcmSongArray *source,
                             NcmSongArray *songs, NcmError *ncm_error) {
    NcmRegex regexes[SEARCH_ENGINE_CONSTRAINT_COUNT];
    NcmError regex_error;
    bool ok;
    bool exact_match;

    if ((screen == NULL) || (source == NULL) || (songs == NULL)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing local search state"));
        return false;
    }

    exact_match = screen->search_mode == SEARCH_ENGINE_SEARCH_MODE_EXACT;
    for (int32 i = 0; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_regex_init(&regexes[i]);
    }

    if (!exact_match) {
        for (int32 i = 0;
             i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
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
        if (!search_song_matches(
            screen, &source->items[i], regexes)) {
            continue;
        }
        if (!ncm_song_array_append_copy(songs, &source->items[i])) {
            ncm_error_set(ncm_error, EIO,
                          STRLIT("failed to copy matching song"));
            ok = false;
            break;
        }
    }

    for (int32 i = 0; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        ncm_regex_destroy(&regexes[i]);
    }
    if (ok) {
        ncm_error_clear(ncm_error);
    }
    return ok;
}

static bool
search_song_matches(SearchEngineScreen *screen, NcmSong *song,
                    NcmRegex *regexes) {
    ASSERT(screen != NULL);
    if ((song == NULL) || (regexes == NULL)) {
        return false;
    }

    if ((screen->constraints[0].len > 0)
        && !search_song_any_matches(screen, song, &regexes[0])) {
        return false;
    }
    for (int32 i = 1; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        if (screen->constraints[i].len <= 0) {
            continue;
        }
        if (!search_song_field_matches(screen, song, i, &regexes[i])) {
            return false;
        }
    }
    return true;
}

static bool
search_song_field_matches(SearchEngineScreen *screen, NcmSong *song,
                          int32 field, NcmRegex *regex) {
    NcmStringView value;
    StrBuilder *constraint = &screen->constraints[field];

    if (!search_song_field_view(song, field, &value)) {
        value = ncm_string_view_make(search_empty_string, 0);
    }
    if (screen->search_mode == SEARCH_ENGINE_SEARCH_MODE_EXACT) {
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
search_song_any_matches(SearchEngineScreen *screen, NcmSong *song,
                        NcmRegex *regex) {
    NcmStringView value;
    StrBuilder *constraint = &screen->constraints[0];

    if ((screen->search_mode != SEARCH_ENGINE_SEARCH_MODE_EXACT)
        && ((regex == NULL) || !regex->compiled)) {
        return true;
    }

    for (int32 i = 1; i < SEARCH_ENGINE_CONSTRAINT_COUNT; i += 1) {
        if (!search_song_field_view(song, i, &value)) {
            value = ncm_string_view_make(search_empty_string, 0);
        }
        if (screen->search_mode
            == SEARCH_ENGINE_SEARCH_MODE_EXACT) {
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
search_song_field_view(NcmSong *song, int32 field,
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
search_append_result_rows(SearchEngineScreen *screen, NcmSongArray *songs) {
    ASSERT(screen != NULL);
    if ((songs == NULL) || (songs->len <= 0)) {
        return false;
    }

    for (int32 i = 0; i < songs->len; i += 1) {
        if (!search_engine_screen_add_song_copy(screen, &songs->items[i])) {
            return false;
        }
    }
    return search_engine_screen_add_result_summary(screen, songs->len);
}

static void
search_print_error(SearchEngineScreen *screen, NcmError *ncm_error) {
    int32 len;

    if ((screen == NULL) || (screen->hooks.status_message == NULL)
        || (ncm_error == NULL)) {
        return;
    }
    len = optional_strlen32(ncm_error->message);
    if (len > 0) {
        screen->hooks.status_message(screen->hooks.user, ncm_error->message,
                                     len);
    }
    return;
}

static bool
search_copy_song_at(SearchEngineScreen *screen,
                    NcmSongArray *songs, int32 pos) {
    NcSearchRow *row;

    row = nc_menu_active_item_at(search_engine_screen_menu(screen),
                                 pos);
    if ((row == NULL) || !row->is_song) {
        return true;
    }
    return ncm_song_array_append_copy(songs, &row->song);
}

#endif /* NCMPCPP_NC_SEARCH_ENGINE_C */
