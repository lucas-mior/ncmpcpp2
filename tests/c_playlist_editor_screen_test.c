#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#include "actions.h"
#include "app_controller.h"
#include "c/ncm_format.h"
#include "global.h"
#include "settings.h"
#include "c/ncm_string.h"
#include "screens/nc_playlist_editor.h"
#include "status.h"
#include "ui_state.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define COMMAND_TEXT_CAPACITY 128

typedef struct PlaylistEditorWindowTrace {
    char printed[1024];

    int64 last_separator_x;
    int32 printed_len;
    int32 display_calls;
    int32 menu_refresh_calls;
    int32 separator_calls;
} PlaylistEditorWindowTrace;

typedef struct PlaylistEditorMpdFixture {
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcmMpdSongList alternate_content;
    char alternate_content_path[COMMAND_TEXT_CAPACITY];
    char failing_content_path[COMMAND_TEXT_CAPACITY];
    char content_path[COMMAND_TEXT_CAPACITY];
    char no_info_content_path[COMMAND_TEXT_CAPACITY];
    char command_playlist[COMMAND_TEXT_CAPACITY];
    char command_target[COMMAND_TEXT_CAPACITY];
    char added_song_uri[COMMAND_TEXT_CAPACITY];
    char status_message[COMMAND_TEXT_CAPACITY];
    int32 get_playlists_calls;
    int32 get_content_calls;
    int32 get_content_no_info_calls;
    int32 status_calls;
    int32 save_calls;
    int32 rename_calls;
    int32 delete_calls;
    int32 load_calls;
    int32 add_song_calls;
    int32 status_update_full_calls;
    int32 add_song_position;
    bool get_playlists_result;
    bool get_content_result;
    bool load_result;
    bool add_song_play;
} PlaylistEditorMpdFixture;

typedef struct PlaylistEditorFormatFixture {
    NcmFormatAst old_song_list_format;
    NcmFormatAst old_song_columns_mode_format;
    ColumnArray old_columns;
    enum DisplayMode old_playlist_display_mode;
    enum DisplayMode old_playlist_editor_display_mode;
    bool old_titles_visibility;
} PlaylistEditorFormatFixture;

static PlaylistEditorWindowTrace window_trace;
static PlaylistEditorMpdFixture mpd_fixture;

static void reset_window_trace(void);
static void init_mpd_fixture(void);
static void destroy_mpd_fixture(void);
static void reset_mpd_calls(void);
static void begin_render_formats(PlaylistEditorFormatFixture *fixture,
                                 Column *columns);
static void end_render_formats(PlaylistEditorFormatFixture *fixture);
static void init_screen(NativePlaylistEditorScreen *screen);
static void append_playlist(NcmMpdPlaylistList *playlists,
                            char *path, int32 path_len);
static void append_song(NcmMpdSongList *songs, char *uri,
                        int32 uri_len, char *title, int32 title_len);
static void assert_playlist_path(NcmPlaylist *playlist,
                                 char *path, int32 path_len);
static void assert_song_uri(NcmSong *song, char *uri, int32 uri_len);
static void assert_array_song_uri(NcmSongArray *songs, int32 pos,
                                  char *uri, int32 uri_len);
static int32 cstring_len(char *string);
static void copy_cstring(char *dest, int32 dest_cap, char *source);
static void test_initial_state_and_geometry(void);
static void test_layout_and_menu_configuration(void);
static void test_fetch_timer_and_timeout_state(void);
static void test_owned_playlist_and_content_rows(void);
static void test_column_navigation(void);
static void test_filter_and_search_are_column_local(void);
static void test_legacy_rendered_filter_and_search_text(void);
static void test_invalid_regex_preserves_constraints(void);
static void test_playlist_filter_clear_restores_current_row(void);
static void test_selected_song_contract(void);
static void test_playlist_column_selected_songs_fetches_playlists(void);
static void test_selected_playlist_fetch_failure_clears_result(void);
static void test_column_availability_uses_unfiltered_menus(void);
static void test_playlist_change_clears_content_selection(void);
static void test_native_selection_helpers(void);
static void test_mpd_reload_and_current_items(void);
static void test_update_callback_uses_native_mpd_logic(void);
static void test_delayed_update_and_empty_content_cache(void);
static void test_native_scroll_fetches_new_playlist_content(void);
static void test_mouse_right_click_actions(void);
static void test_finish_list_change_callback_and_mouse_wheel(void);
static void test_locate_playlist_fetches_content_and_switches(void);
static void test_locate_song_searches_current_playlist_first(void);
static void test_locate_song_uses_legacy_playlist_order(void);
static void test_locate_song_reports_not_found(void);
static void test_update_error_reporting_and_flags(void);
static void test_playlist_commands(void);
static void test_native_rendering_callbacks(void);
static void test_native_refresh_and_mouse_fallback(void);
void __wrap_nc_window_set_title(NcWindow *window, char *title,
                                int32 title_len);

int
main(void) {
    app_controller_init();
    ui_state_set_screen_size(100, 30);
    Config.titles_visibility = true;
    init_mpd_fixture();

    test_initial_state_and_geometry();
    test_layout_and_menu_configuration();
    test_fetch_timer_and_timeout_state();
    test_owned_playlist_and_content_rows();
    test_column_navigation();
    test_filter_and_search_are_column_local();
    test_legacy_rendered_filter_and_search_text();
    test_invalid_regex_preserves_constraints();
    test_playlist_filter_clear_restores_current_row();
    test_selected_song_contract();
    test_playlist_column_selected_songs_fetches_playlists();
    test_selected_playlist_fetch_failure_clears_result();
    test_column_availability_uses_unfiltered_menus();
    test_playlist_change_clears_content_selection();
    test_native_selection_helpers();
    test_mpd_reload_and_current_items();
    test_update_callback_uses_native_mpd_logic();
    test_delayed_update_and_empty_content_cache();
    test_native_scroll_fetches_new_playlist_content();
    test_mouse_right_click_actions();
    test_finish_list_change_callback_and_mouse_wheel();
    test_locate_playlist_fetches_content_and_switches();
    test_locate_song_searches_current_playlist_first();
    test_locate_song_uses_legacy_playlist_order();
    test_locate_song_reports_not_found();
    test_update_error_reporting_and_flags();
    test_playlist_commands();
    test_native_rendering_callbacks();
    test_native_refresh_and_mouse_fallback();

    destroy_mpd_fixture();
    exit(EXIT_SUCCESS);
}

static void
reset_window_trace(void) {
    window_trace = (PlaylistEditorWindowTrace){0};
    return;
}

static void
begin_render_formats(PlaylistEditorFormatFixture *fixture,
                     Column *columns) {
    NcmError error;

    fixture->old_song_list_format = Config.song_list_format;
    fixture->old_song_columns_mode_format =
        Config.song_columns_mode_format;
    fixture->old_columns = Config.columns;
    fixture->old_playlist_display_mode = Config.playlist_display_mode;
    fixture->old_playlist_editor_display_mode =
        Config.playlist_editor_display_mode;
    fixture->old_titles_visibility = Config.titles_visibility;

    ncm_format_ast_init(&Config.song_list_format);
    ncm_format_ast_init(&Config.song_columns_mode_format);
    ncm_error_clear(&error);
    assert(ncm_format_parse(&Config.song_list_format,
                            LIT_ARGS("classic:%F"),
                            NCM_FORMAT_FLAG_ALL, &error));
    ncm_error_clear(&error);
    assert(ncm_format_parse(&Config.song_columns_mode_format,
                            LIT_ARGS("columns:%a"),
                            NCM_FORMAT_FLAG_ALL, &error));

    columns[0] = (Column){0};
    columns[0].name = "Artist";
    columns[0].name_len = STRLIT_LEN("Artist");
    columns[0].type = "a";
    columns[0].type_len = STRLIT_LEN("a");
    columns[0].width = 6;
    columns[0].stretch_limit = -1;
    columns[0].color = nc_color_default();
    columns[0].fixed = true;

    columns[1] = (Column){0};
    columns[1].name = "Title";
    columns[1].name_len = STRLIT_LEN("Title");
    columns[1].type = "t";
    columns[1].type_len = STRLIT_LEN("t");
    columns[1].width = 6;
    columns[1].stretch_limit = -1;
    columns[1].color = nc_color_default();
    columns[1].fixed = true;
    columns[1].display_empty_tag = true;

    Config.columns.items = columns;
    Config.columns.len = 2;
    Config.columns.cap = 2;
    Config.playlist_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    Config.titles_visibility = true;
    return;
}

static void
end_render_formats(PlaylistEditorFormatFixture *fixture) {
    ncm_format_ast_destroy(&Config.song_columns_mode_format);
    ncm_format_ast_destroy(&Config.song_list_format);
    Config.song_list_format = fixture->old_song_list_format;
    Config.song_columns_mode_format =
        fixture->old_song_columns_mode_format;
    Config.columns = fixture->old_columns;
    Config.playlist_display_mode = fixture->old_playlist_display_mode;
    Config.playlist_editor_display_mode =
        fixture->old_playlist_editor_display_mode;
    Config.titles_visibility = fixture->old_titles_visibility;
    return;
}

static void
init_mpd_fixture(void) {
    mpd_fixture = (PlaylistEditorMpdFixture){0};
    ncm_mpd_playlist_list_init(&mpd_fixture.playlists);
    ncm_mpd_song_list_init(&mpd_fixture.content);
    ncm_mpd_song_list_init(&mpd_fixture.alternate_content);
    mpd_fixture.get_playlists_result = true;
    mpd_fixture.get_content_result = true;
    mpd_fixture.load_result = true;
    mpd_fixture.add_song_play = false;
    return;
}

static void
destroy_mpd_fixture(void) {
    ncm_mpd_song_list_destroy(&mpd_fixture.alternate_content);
    ncm_mpd_song_list_destroy(&mpd_fixture.content);
    ncm_mpd_playlist_list_destroy(&mpd_fixture.playlists);
    mpd_fixture = (PlaylistEditorMpdFixture){0};
    return;
}

static void
reset_mpd_calls(void) {
    mpd_fixture.content_path[0] = '\0';
    mpd_fixture.no_info_content_path[0] = '\0';
    mpd_fixture.alternate_content_path[0] = '\0';
    mpd_fixture.failing_content_path[0] = '\0';
    mpd_fixture.command_playlist[0] = '\0';
    mpd_fixture.command_target[0] = '\0';
    mpd_fixture.added_song_uri[0] = '\0';
    mpd_fixture.get_playlists_calls = 0;
    mpd_fixture.get_content_calls = 0;
    mpd_fixture.get_content_no_info_calls = 0;
    mpd_fixture.save_calls = 0;
    mpd_fixture.rename_calls = 0;
    mpd_fixture.delete_calls = 0;
    mpd_fixture.load_calls = 0;
    mpd_fixture.add_song_calls = 0;
    mpd_fixture.status_update_full_calls = 0;
    mpd_fixture.add_song_position = 0;
    mpd_fixture.status_calls = 0;
    mpd_fixture.status_message[0] = '\0';
    mpd_fixture.get_playlists_result = true;
    mpd_fixture.get_content_result = true;
    mpd_fixture.load_result = true;
    mpd_fixture.add_song_play = false;
    return;
}

static void
init_screen(NativePlaylistEditorScreen *screen) {
    native_playlist_editor_screen_init(
        screen, 0, 80, 0, 24, nc_color_default(), nc_border_none());
    return;
}

static void
append_playlist(NcmMpdPlaylistList *playlists, char *path,
                int32 path_len) {
    NcmPlaylist playlist;

    ncm_playlist_init(&playlist);
    assert(ncm_playlist_set(&playlist, path, path_len, 0));
    assert(ncm_mpd_playlist_list_append_copy(playlists, &playlist));
    ncm_playlist_destroy(&playlist);
    return;
}

static void
append_song(NcmMpdSongList *songs, char *uri, int32 uri_len,
            char *title, int32 title_len) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    if ((title != NULL) && (title_len > 0)) {
        assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, title, title_len));
    }
    assert(ncm_mpd_song_list_append_copy(songs, &song));
    ncm_song_destroy(&song);
    return;
}

static void
assert_playlist_path(NcmPlaylist *playlist, char *path,
                     int32 path_len) {
    assert(playlist != NULL);
    assert(ncm_string_equal(playlist->path, playlist->path_len,
                            path, path_len));
    return;
}

static void
assert_song_uri(NcmSong *song, char *uri, int32 uri_len) {
    NcmStringView view;

    assert(song != NULL);
    assert(ncm_song_uri_view(song, 0, &view));
    assert(ncm_string_equal(view.data, view.len, uri, uri_len));
    return;
}

static void
assert_array_song_uri(NcmSongArray *songs, int32 pos, char *uri,
                      int32 uri_len) {
    assert(pos >= 0);
    assert(pos < songs->len);
    assert_song_uri(&songs->items[pos], uri, uri_len);
    return;
}

static int32
cstring_len(char *string) {
    int32 len;

    len = 0;
    if (string == NULL) {
        return 0;
    }
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
copy_cstring(char *dest, int32 dest_cap, char *source) {
    int32 source_len;

    assert(dest != NULL);
    assert(dest_cap > 0);
    source_len = cstring_len(source);
    assert(source_len < dest_cap);
    if (source_len > 0) {
        cbase_memcpy(dest, source, source_len);
    }
    dest[source_len] = '\0';
    return;
}

static void
test_initial_state_and_geometry(void) {
    NativePlaylistEditorScreen screen;
    NcScreen *base;

    init_screen(&screen);
    base = native_playlist_editor_screen_base(&screen);

    assert(base != NULL);
    assert(nc_screen_type(base) == NC_SCREEN_TYPE_PLAYLIST_EDITOR);
    assert(native_playlist_editor_screen_active_menu(&screen)
           == nc_playlist_entry_menu_base(&screen.playlists));
    assert(native_playlist_editor_screen_active_window(&screen)
           == &screen.playlists_window);
    assert(screen.active_column
           == NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS);
    assert(screen.start_x == 0);
    assert(screen.width == 80);
    assert(screen.left_width == 39);
    assert(screen.right_start_x == 40);
    assert(screen.right_width == 40);
    assert(screen.playlists_update_requested);
    assert(screen.content_update_requested);
    assert(screen.fetching_delay_ms == -1);
    assert(screen.window_timeout_ms == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);
    assert(screen.last_playlist_highlight == -1);
    assert(screen.last_known_content_count == -1);
    assert(!screen.displayed_playlist_valid);
    assert(!screen.observed_playlist_valid);
    assert(!screen.playlist_search_enabled);
    assert(!screen.content_search_enabled);
    assert(nc_screen_is_lockable(base));
    assert(nc_screen_is_mergable(base));
    assert(nc_screen_window_timeout(base)
           == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);
    assert(ncm_string_equal(nc_screen_title(base),
                            STRLIT_LEN("Playlist editor"),
                            LIT_ARGS("Playlist editor")));
    assert(ncm_string_equal(screen.playlists_title.data,
                            screen.playlists_title.len,
                            LIT_ARGS("Playlists")));
    assert(ncm_string_equal(screen.content_title.data,
                            screen.content_title.len,
                            LIT_ARGS("Content")));
    assert(ncm_string_equal(screen.playlists_window.title,
                            screen.playlists_window.title_len,
                            LIT_ARGS("Playlists")));
    assert(ncm_string_equal(screen.content_window.title,
                            screen.content_window.title_len,
                            LIT_ARGS("Content")));

    native_playlist_editor_screen_set_column_ratio(&screen, 2, 1);
    assert(screen.left_width == 52);
    assert(screen.right_start_x == 53);
    assert(screen.right_width == 27);
    assert(screen.column_ratio_left == 2);
    assert(screen.column_ratio_right == 1);

    native_playlist_editor_screen_set_geometry(&screen, 5, 60, 3, 18);
    assert(screen.start_x == 5);
    assert(screen.width == 60);
    assert(screen.main_start_y == 3);
    assert(screen.main_height == 18);
    assert(screen.left_width == 39);
    assert(screen.right_start_x == 45);
    assert(screen.right_width == 20);

    native_playlist_editor_screen_set_geometry(&screen, 5, 2, 3, 18);
    assert(screen.left_width == 1);
    assert(screen.right_start_x == 6);
    assert(screen.right_width == 1);

    native_playlist_editor_screen_set_geometry(&screen, 5, 1, 3, 18);
    assert(screen.left_width == 1);
    assert(screen.right_width == 1);

    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_layout_and_menu_configuration(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcMenu *playlists_menu;
    NcMenu *content_menu;
    NcBuffer old_selected_prefix;
    NcBuffer old_selected_suffix;
    NcBuffer old_current_prefix;
    NcBuffer old_current_suffix;
    NcBuffer old_inactive_prefix;
    NcBuffer old_inactive_suffix;
    bool old_titles_visibility;
    bool old_cyclic_scrolling;
    bool old_centered_cursor;

    old_selected_prefix = Config.selected_item_prefix;
    old_selected_suffix = Config.selected_item_suffix;
    old_current_prefix = Config.current_item_prefix;
    old_current_suffix = Config.current_item_suffix;
    old_inactive_prefix = Config.current_item_inactive_column_prefix;
    old_inactive_suffix = Config.current_item_inactive_column_suffix;
    old_titles_visibility = Config.titles_visibility;
    old_cyclic_scrolling = Config.use_cyclic_scrolling;
    old_centered_cursor = Config.centered_cursor;

    nc_buffer_init(&Config.selected_item_prefix);
    nc_buffer_append_data(&Config.selected_item_prefix,
                          LIT_ARGS("<selected>"));
    nc_buffer_init(&Config.selected_item_suffix);
    nc_buffer_append_data(&Config.selected_item_suffix,
                          LIT_ARGS("</selected>"));
    nc_buffer_init(&Config.current_item_prefix);
    nc_buffer_append_data(&Config.current_item_prefix,
                          LIT_ARGS("<active>"));
    nc_buffer_init(&Config.current_item_suffix);
    nc_buffer_append_data(&Config.current_item_suffix,
                          LIT_ARGS("</active>"));
    nc_buffer_init(&Config.current_item_inactive_column_prefix);
    nc_buffer_append_data(
        &Config.current_item_inactive_column_prefix,
        LIT_ARGS("<inactive>"));
    nc_buffer_init(&Config.current_item_inactive_column_suffix);
    nc_buffer_append_data(
        &Config.current_item_inactive_column_suffix,
        LIT_ARGS("</inactive>"));
    Config.titles_visibility = true;
    Config.use_cyclic_scrolling = true;
    Config.centered_cursor = true;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    append_playlist(&playlists, LIT_ARGS("Current"));
    append_song(&content, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    playlists_menu = nc_playlist_entry_menu_base(&screen.playlists);
    content_menu = nc_song_menu_base(&screen.content);

    assert(playlists_menu->cyclic_scroll_enabled);
    assert(content_menu->cyclic_scroll_enabled);
    assert(playlists_menu->autocenter_cursor);
    assert(content_menu->autocenter_cursor);
    assert(nc_buffer_equal(&playlists_menu->selected_prefix,
                           &Config.selected_item_prefix));
    assert(nc_buffer_equal(&content_menu->selected_suffix,
                           &Config.selected_item_suffix));
    assert(nc_buffer_equal(&playlists_menu->highlight_prefix,
                           &Config.current_item_prefix));
    assert(nc_buffer_equal(&playlists_menu->highlight_suffix,
                           &Config.current_item_suffix));
    assert(nc_buffer_equal(
        &content_menu->highlight_prefix,
        &Config.current_item_inactive_column_prefix));
    assert(nc_buffer_equal(
        &content_menu->highlight_suffix,
        &Config.current_item_inactive_column_suffix));

    native_playlist_editor_screen_next_column(&screen);
    assert(nc_buffer_equal(
        &playlists_menu->highlight_prefix,
        &Config.current_item_inactive_column_prefix));
    assert(nc_buffer_equal(&content_menu->highlight_prefix,
                           &Config.current_item_prefix));

    native_playlist_editor_screen_destroy(&screen);
    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);

    Config.titles_visibility = false;
    init_screen(&screen);
    assert(screen.playlists_title.len == 0);
    assert(screen.content_title.len == 0);
    assert(screen.playlists_window.title_len == 0);
    assert(screen.content_window.title_len == 0);
    native_playlist_editor_screen_destroy(&screen);

    nc_buffer_destroy(&Config.selected_item_prefix);
    nc_buffer_destroy(&Config.selected_item_suffix);
    nc_buffer_destroy(&Config.current_item_prefix);
    nc_buffer_destroy(&Config.current_item_suffix);
    nc_buffer_destroy(&Config.current_item_inactive_column_prefix);
    nc_buffer_destroy(&Config.current_item_inactive_column_suffix);
    Config.selected_item_prefix = old_selected_prefix;
    Config.selected_item_suffix = old_selected_suffix;
    Config.current_item_prefix = old_current_prefix;
    Config.current_item_suffix = old_current_suffix;
    Config.current_item_inactive_column_prefix = old_inactive_prefix;
    Config.current_item_inactive_column_suffix = old_inactive_suffix;
    Config.titles_visibility = old_titles_visibility;
    Config.use_cyclic_scrolling = old_cyclic_scrolling;
    Config.centered_cursor = old_centered_cursor;
    return;
}

static void
test_fetch_timer_and_timeout_state(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList songs;
    NcmTimePoint old_global_timer;
    bool old_data_fetching_delay;

    old_global_timer = global_timer;
    old_data_fetching_delay = Config.data_fetching_delay;
    global_timer.ns = 1000000000ll;
    Config.data_fetching_delay = true;

    init_screen(&screen);
    assert(screen.fetching_delay_ms == NATIVE_PLAYLIST_EDITOR_FETCH_DELAY_MS);
    assert(screen.window_timeout_ms == NATIVE_PLAYLIST_EDITOR_FETCH_DELAY_MS);
    assert(nc_screen_window_timeout(native_playlist_editor_screen_base(
               &screen)) == NATIVE_PLAYLIST_EDITOR_FETCH_DELAY_MS);

    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&songs);
    append_playlist(&playlists, LIT_ARGS("Delayed"));
    append_song(&songs, LIT_ARGS("delayed.flac"), LIT_ARGS("Delayed"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &songs));
    assert(nc_screen_window_timeout(native_playlist_editor_screen_base(
               &screen)) == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);

    global_timer.ns = 2000000000ll;
    native_playlist_editor_screen_request_content_update(&screen);
    assert(screen.timer.ns == global_timer.ns);
    assert(screen.content_update_requested);

    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);

    Config.data_fetching_delay = old_data_fetching_delay;
    global_timer = old_global_timer;
    return;
}

static void
test_owned_playlist_and_content_rows(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList songs;
    NcMenu *playlist_menu;
    NcMenu *content_menu;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&songs);
    append_playlist(&playlists, LIT_ARGS("Road trip"));
    for (int32 i = 0; i < 128; i += 1) {
        append_playlist(&playlists, LIT_ARGS("Repeated playlist"));
    }
    append_song(&songs, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    append_song(&songs, LIT_ARGS("two.flac"), LIT_ARGS("Two"));

    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &songs));
    ncm_mpd_playlist_list_destroy(&playlists);
    ncm_mpd_song_list_destroy(&songs);

    playlist_menu = nc_playlist_entry_menu_base(&screen.playlists);
    content_menu = nc_song_menu_base(&screen.content);
    assert(nc_menu_all_item_count(playlist_menu) == 129);
    assert(nc_menu_all_item_count(content_menu) == 2);
    assert_playlist_path(nc_playlist_entry_menu_item_at(
                             &screen.playlists, NC_MENU_ITEMS_ALL, 0),
                         LIT_ARGS("Road trip"));
    assert_song_uri(nc_song_menu_item_at(
                        &screen.content, NC_MENU_ITEMS_ALL, 0),
                    LIT_ARGS("one.flac"));
    assert_song_uri(nc_song_menu_item_at(
                        &screen.content, NC_MENU_ITEMS_ALL, 1),
                    LIT_ARGS("two.flac"));
    assert(!screen.playlists_update_requested);
    assert(!screen.content_update_requested);
    assert(screen.observed_playlist_valid);
    assert(screen.displayed_playlist_valid);
    assert(screen.last_playlist_highlight == 0);
    assert(screen.last_known_content_count == 2);
    assert(ncm_string_equal(screen.displayed_playlist_path.data,
                            screen.displayed_playlist_path.len,
                            LIT_ARGS("Road trip")));
    assert(ncm_string_equal(screen.content_title.data,
                            screen.content_title.len,
                            LIT_ARGS("Content (2 items)")));

    native_playlist_editor_screen_clear(&screen);
    assert(nc_menu_all_item_count(playlist_menu) == 0);
    assert(nc_menu_all_item_count(content_menu) == 0);
    assert(screen.playlists_update_requested);
    assert(screen.content_update_requested);
    assert(!screen.displayed_playlist_valid);
    assert(!screen.observed_playlist_valid);
    assert(screen.last_known_content_count == -1);
    assert(ncm_string_equal(screen.content_title.data,
                            screen.content_title.len,
                            LIT_ARGS("Content")));

    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_column_navigation(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList songs;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&songs);
    append_playlist(&playlists, LIT_ARGS("Current"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));

    assert(!native_playlist_editor_screen_previous_column_available(
        &screen));
    assert(!native_playlist_editor_screen_next_column_available(&screen));
    native_playlist_editor_screen_next_column(&screen);
    assert(screen.active_column
           == NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS);

    append_song(&songs, LIT_ARGS("song.flac"), LIT_ARGS("Song"));
    assert(native_playlist_editor_screen_load_content(&screen, &songs));
    assert(native_playlist_editor_screen_next_column_available(&screen));
    native_playlist_editor_screen_next_column(&screen);
    assert(screen.active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT);
    assert(native_playlist_editor_screen_previous_column_available(
        &screen));
    assert(native_playlist_editor_screen_active_menu(&screen)
           == nc_song_menu_base(&screen.content));
    assert(native_playlist_editor_screen_active_window(&screen)
           == &screen.content_window);

    native_playlist_editor_screen_previous_column(&screen);
    assert(screen.active_column
           == NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS);

    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_filter_and_search_are_column_local(void) {
    NativePlaylistEditorScreen screen;
    PlaylistEditorFormatFixture formats;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList songs;
    NcmError error;
    NcMenu *playlist_menu;
    NcMenu *content_menu;
    Column columns[2];

    begin_render_formats(&formats, columns);
    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&songs);
    append_playlist(&playlists, LIT_ARGS("Road trip"));
    append_playlist(&playlists, LIT_ARGS("Study"));
    append_playlist(&playlists, LIT_ARGS("Road archive"));
    append_song(&songs, LIT_ARGS("one.flac"), LIT_ARGS("First song"));
    append_song(&songs, LIT_ARGS("two.flac"), LIT_ARGS("Needle title"));
    append_song(&songs, LIT_ARGS("needle-uri.flac"),
                LIT_ARGS("Third song"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &songs));
    playlist_menu = nc_playlist_entry_menu_base(&screen.playlists);
    content_menu = nc_song_menu_base(&screen.content);

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("road"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, &error));
    assert(screen.playlist_filter_enabled);
    assert(nc_menu_item_count(playlist_menu) == 2);
    assert(nc_menu_item_count(content_menu) == 3);
    assert(ncm_string_equal(screen.playlist_filter_constraint.data,
                            screen.playlist_filter_constraint.len,
                            LIT_ARGS("road")));

    native_playlist_editor_screen_next_column(&screen);
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("needle"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, &error));
    assert(screen.content_filter_enabled);
    assert(nc_menu_item_count(content_menu) == 1);
    assert(nc_menu_item_count(playlist_menu) == 2);

    nc_menu_highlight_position(content_menu, 0, 24);
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("needle-uri"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, true, true, false, &error));
    assert(nc_menu_highlight(content_menu) == 0);
    assert(screen.content_search_enabled);
    assert(ncm_string_equal(screen.content_search_constraint.data,
                            screen.content_search_constraint.len,
                            LIT_ARGS("needle-uri")));
    ncm_error_clear(&error);
    assert(!native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("Needle title"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, false, true, false, &error));
    assert(nc_menu_highlight(content_menu) == 0);
    assert(screen.content_search_enabled);
    assert(ncm_string_equal(screen.content_search_constraint.data,
                            screen.content_search_constraint.len,
                            LIT_ARGS("Needle title")));

    native_playlist_editor_screen_set_geometry(&screen, 3, 70, 2, 12);
    assert(screen.active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT);
    assert(screen.left_width == 34);
    assert(screen.right_start_x == 38);
    assert(screen.right_width == 35);
    assert(screen.playlist_filter_enabled);
    assert(screen.content_filter_enabled);
    assert(screen.content_search_enabled);
    assert(nc_menu_item_count(playlist_menu) == 2);
    assert(nc_menu_item_count(content_menu) == 1);
    assert(nc_menu_highlight(content_menu) == 0);

    native_playlist_editor_screen_clear_active_filter(&screen);
    assert(!screen.content_filter_enabled);
    assert(nc_menu_item_count(content_menu) == 3);
    assert(nc_menu_item_count(playlist_menu) == 2);

    nc_menu_highlight_position(content_menu, 0, 24);
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("two.flac"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, false, true, false, &error));
    assert(nc_menu_highlight(content_menu) == 1);

    native_playlist_editor_screen_previous_column(&screen);
    native_playlist_editor_screen_clear_active_filter(&screen);
    assert(!screen.playlist_filter_enabled);
    assert(nc_menu_item_count(playlist_menu) == 3);

    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    end_render_formats(&formats);
    return;
}

static void
test_legacy_rendered_filter_and_search_text(void) {
    NativePlaylistEditorScreen screen;
    PlaylistEditorFormatFixture formats;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList songs;
    NcmError error;
    NcMenu *content_menu;
    Column columns[2];

    begin_render_formats(&formats, columns);
    Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&songs);
    append_playlist(&playlists, LIT_ARGS("Rendered"));
    append_song(&songs, LIT_ARGS("raw-needle.flac"),
                LIT_ARGS("Formatted Title"));
    assert(ncm_song_add_tag(&songs.items[0], MPD_TAG_ARTIST,
                            LIT_ARGS("Column Artist")));
    append_song(&songs, LIT_ARGS("other.flac"), LIT_ARGS("Other"));
    assert(ncm_song_add_tag(&songs.items[1], MPD_TAG_ARTIST,
                            LIT_ARGS("Other Artist")));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &songs));
    assert(native_playlist_editor_screen_next_column_available(&screen));
    native_playlist_editor_screen_next_column(&screen);
    content_menu = nc_song_menu_base(&screen.content);

    Config.playlist_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("raw-needle"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, &error));
    assert(nc_menu_item_count(content_menu) == 1);
    native_playlist_editor_screen_clear_active_filter(&screen);
    ncm_error_clear(&error);
    assert(!native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("Column Artist"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, true, true, false, &error));

    Config.playlist_display_mode = NCM_DISPLAY_MODE_COLUMNS;
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("Column Artist"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, &error));
    assert(nc_menu_item_count(content_menu) == 1);
    native_playlist_editor_screen_clear_active_filter(&screen);
    ncm_error_clear(&error);
    assert(!native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("raw-needle"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, true, true, false, &error));
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("Other Artist"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, true, true, true, &error));
    assert(nc_menu_highlight(content_menu) == 1);

    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    end_render_formats(&formats);
    return;
}

static void
test_invalid_regex_preserves_constraints(void) {
    NativePlaylistEditorScreen screen;
    PlaylistEditorFormatFixture formats;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList songs;
    NcmError error;
    Column columns[2];

    begin_render_formats(&formats, columns);
    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&songs);
    append_playlist(&playlists, LIT_ARGS("Road trip"));
    append_song(&songs, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &songs));

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("Road"), NCM_REGEX_EXTENDED, &error));
    ncm_error_clear(&error);
    assert(!native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("("), NCM_REGEX_EXTENDED, &error));
    assert(ncm_error_is_set(&error));
    assert(screen.playlist_filter_enabled);
    assert(ncm_string_equal(screen.playlist_filter_constraint.data,
                            screen.playlist_filter_constraint.len,
                            LIT_ARGS("Road")));

    native_playlist_editor_screen_next_column(&screen);
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("one"), NCM_REGEX_EXTENDED,
        true, true, false, &error));
    ncm_error_clear(&error);
    assert(!native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("("), NCM_REGEX_EXTENDED,
        true, true, false, &error));
    assert(ncm_error_is_set(&error));
    assert(screen.content_search_enabled);
    assert(ncm_string_equal(screen.content_search_constraint.data,
                            screen.content_search_constraint.len,
                            LIT_ARGS("one")));

    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    end_render_formats(&formats);
    return;
}

static void
test_playlist_filter_clear_restores_current_row(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcMenu *playlist_menu;
    NcmError error;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    append_playlist(&playlists, LIT_ARGS("Alpha"));
    append_playlist(&playlists, LIT_ARGS("Road trip"));
    append_playlist(&playlists, LIT_ARGS("Study"));
    append_playlist(&playlists, LIT_ARGS("Road archive"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    playlist_menu = nc_playlist_entry_menu_base(&screen.playlists);

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("road"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, &error));
    assert(nc_menu_item_count(playlist_menu) == 2);
    nc_menu_highlight_position(playlist_menu, 1, 24);
    native_playlist_editor_screen_clear_active_filter(&screen);
    assert(nc_menu_item_count(playlist_menu) == 4);
    assert(nc_menu_highlight(playlist_menu) == 3);

    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_selected_song_contract(void) {
    NativePlaylistEditorScreen screen;
    PlaylistEditorFormatFixture formats;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcmSongArray selected;
    NcmError error;
    NcMenu *menu;
    Column columns[2];

    begin_render_formats(&formats, columns);
    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    append_playlist(&playlists, LIT_ARGS("Current"));
    append_song(&content, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    append_song(&content, LIT_ARGS("two.flac"), LIT_ARGS("Two"));
    append_song(&content, LIT_ARGS("three.flac"), LIT_ARGS("Three"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));

    ncm_song_array_init(&selected);
    assert(native_playlist_editor_screen_selected_songs(&screen,
                                                         &selected));
    assert(selected.len == 3);
    assert_array_song_uri(&selected, 0, LIT_ARGS("one.flac"));
    assert_array_song_uri(&selected, 1, LIT_ARGS("two.flac"));
    assert_array_song_uri(&selected, 2, LIT_ARGS("three.flac"));
    ncm_song_array_clear(&selected);

    native_playlist_editor_screen_next_column(&screen);
    menu = nc_song_menu_base(&screen.content);
    nc_menu_highlight_position(menu, 1, 24);
    assert(native_playlist_editor_screen_selected_songs(&screen,
                                                         &selected));
    assert(selected.len == 1);
    assert_array_song_uri(&selected, 0, LIT_ARGS("two.flac"));
    ncm_song_array_clear(&selected);

    assert(nc_menu_select_range(menu, 0, 2, true));
    assert(native_playlist_editor_screen_selected_songs(&screen,
                                                         &selected));
    assert(selected.len == 3);
    assert_array_song_uri(&selected, 0, LIT_ARGS("one.flac"));
    assert_array_song_uri(&selected, 1, LIT_ARGS("two.flac"));
    assert_array_song_uri(&selected, 2, LIT_ARGS("three.flac"));
    ncm_song_array_clear(&selected);

    nc_menu_clear_selection(menu);
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("Three"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, &error));
    assert(nc_menu_item_count(menu) == 1);
    assert(native_playlist_editor_screen_selected_songs(&screen,
                                                         &selected));
    assert(selected.len == 1);
    assert_array_song_uri(&selected, 0, LIT_ARGS("three.flac"));

    ncm_song_array_destroy(&selected);
    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    end_render_formats(&formats);
    return;
}


static void
test_playlist_column_selected_songs_fetches_playlists(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmSongArray selected;
    NcMenu *menu;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_clear(&mpd_fixture.content);
    ncm_mpd_song_list_clear(&mpd_fixture.alternate_content);
    append_playlist(&playlists, LIT_ARGS("Alpha"));
    append_playlist(&playlists, LIT_ARGS("Beta"));
    append_song(&mpd_fixture.content, LIT_ARGS("alpha-1.flac"),
                LIT_ARGS("Alpha 1"));
    append_song(&mpd_fixture.content, LIT_ARGS("alpha-2.flac"),
                LIT_ARGS("Alpha 2"));
    append_song(&mpd_fixture.alternate_content,
                LIT_ARGS("beta-1.flac"), LIT_ARGS("Beta 1"));
    copy_cstring(mpd_fixture.alternate_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.alternate_content_path),
                 "Beta");
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));

    menu = nc_playlist_entry_menu_base(&screen.playlists);
    assert(nc_menu_set_position_selected(menu, 0, true));
    assert(nc_menu_set_position_selected(menu, 1, true));
    assert(native_playlist_editor_screen_selected_playlist_count(
               &screen) == 2);

    reset_mpd_calls();
    copy_cstring(mpd_fixture.alternate_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.alternate_content_path),
                 "Beta");
    ncm_song_array_init(&selected);
    assert(native_playlist_editor_screen_selected_songs(&screen,
                                                         &selected));
    assert(mpd_fixture.get_content_calls == 2);
    assert(selected.len == 3);
    assert_array_song_uri(&selected, 0, LIT_ARGS("alpha-1.flac"));
    assert_array_song_uri(&selected, 1, LIT_ARGS("alpha-2.flac"));
    assert_array_song_uri(&selected, 2, LIT_ARGS("beta-1.flac"));

    ncm_song_array_destroy(&selected);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_selected_playlist_fetch_failure_clears_result(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmSongArray selected;
    NcMenu *menu;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_clear(&mpd_fixture.content);
    ncm_mpd_song_list_clear(&mpd_fixture.alternate_content);
    append_playlist(&playlists, LIT_ARGS("Alpha"));
    append_playlist(&playlists, LIT_ARGS("Beta"));
    append_song(&mpd_fixture.content, LIT_ARGS("alpha.flac"),
                LIT_ARGS("Alpha"));
    append_song(&mpd_fixture.alternate_content, LIT_ARGS("beta.flac"),
                LIT_ARGS("Beta"));
    copy_cstring(mpd_fixture.alternate_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.alternate_content_path),
                 "Beta");
    copy_cstring(mpd_fixture.failing_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.failing_content_path),
                 "Beta");
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    menu = nc_playlist_entry_menu_base(&screen.playlists);
    assert(nc_menu_select_range(menu, 0, 1, true));

    reset_mpd_calls();
    copy_cstring(mpd_fixture.alternate_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.alternate_content_path),
                 "Beta");
    copy_cstring(mpd_fixture.failing_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.failing_content_path),
                 "Beta");
    ncm_song_array_init(&selected);
    assert(!native_playlist_editor_screen_selected_songs(&screen,
                                                          &selected));
    assert(mpd_fixture.get_content_calls == 2);
    assert(selected.len == 0);
    assert(mpd_fixture.status_calls == 1);

    ncm_song_array_destroy(&selected);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_column_availability_uses_unfiltered_menus(void) {
    NativePlaylistEditorScreen screen;
    PlaylistEditorFormatFixture formats;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcmError error;
    Column columns[2];

    begin_render_formats(&formats, columns);
    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    append_playlist(&playlists, LIT_ARGS("Only"));
    append_song(&content, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    assert(native_playlist_editor_screen_next_column_available(&screen));
    native_playlist_editor_screen_next_column(&screen);

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("not present"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, &error));
    assert(native_playlist_editor_screen_active_menu_empty(&screen));
    assert(native_playlist_editor_screen_previous_column_available(
               &screen));
    native_playlist_editor_screen_previous_column(&screen);
    assert(native_playlist_editor_screen_next_column_available(&screen));

    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    end_render_formats(&formats);
    return;
}

static void
test_playlist_change_clears_content_selection(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcMenu *content_menu;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    append_playlist(&playlists, LIT_ARGS("First"));
    append_playlist(&playlists, LIT_ARGS("Second"));
    append_song(&content, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    native_playlist_editor_screen_next_column(&screen);
    content_menu = nc_song_menu_base(&screen.content);
    assert(nc_menu_set_position_selected(content_menu, 0, true));
    assert(native_playlist_editor_screen_selected_content_count(
               &screen) == 1);

    native_playlist_editor_screen_previous_column(&screen);
    nc_screen_scroll(native_playlist_editor_screen_base(&screen),
                     NC_SCROLL_DOWN);
    assert(nc_menu_all_item_count(content_menu) == 0);
    assert(native_playlist_editor_screen_selected_content_count(
               &screen) == 0);
    assert(!native_playlist_editor_screen_next_column_available(&screen));

    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_native_selection_helpers(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcmSong song;
    char *path;
    int32 path_len;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    ncm_song_init(&song);
    append_playlist(&playlists, LIT_ARGS("Helper"));
    append_song(&content, LIT_ARGS("helper.flac"), LIT_ARGS("Helper"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));

    assert(native_playlist_editor_screen_current_playlist_path(
        &screen, &path, &path_len));
    assert(ncm_string_equal(path, path_len, LIT_ARGS("Helper")));
    assert(!native_playlist_editor_screen_active_menu_empty(&screen));
    assert(native_playlist_editor_screen_selected_playlist_count(
               &screen) == 0);
    assert(native_playlist_editor_screen_selected_content_count(
               &screen) == 0);

    native_playlist_editor_screen_next_column(&screen);
    assert(native_playlist_editor_screen_current_content_song(&screen,
                                                              &song));
    assert_song_uri(&song, LIT_ARGS("helper.flac"));

    ncm_song_destroy(&song);
    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_mpd_reload_and_current_items(void) {
    NativePlaylistEditorScreen screen;
    NcmPlaylist playlist;
    NcmSong song;
    NcmError error;

    ncm_mpd_playlist_list_clear(&mpd_fixture.playlists);
    ncm_mpd_song_list_clear(&mpd_fixture.content);
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("Fetched"));
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("Second"));
    append_song(&mpd_fixture.content, LIT_ARGS("first.flac"),
                LIT_ARGS("First"));
    append_song(&mpd_fixture.content, LIT_ARGS("second.flac"),
                LIT_ARGS("Second"));
    reset_mpd_calls();
    init_screen(&screen);
    ncm_playlist_init(&playlist);
    ncm_song_init(&song);

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_reload_playlists_from_mpd(
        &screen, NULL, &error));
    assert(mpd_fixture.get_playlists_calls == 1);
    assert(native_playlist_editor_screen_current_playlist(&screen,
                                                           &playlist));
    assert_playlist_path(&playlist, LIT_ARGS("Fetched"));

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_reload_content_from_mpd(
        &screen, NULL, &error));
    assert(mpd_fixture.get_content_calls == 1);
    assert(ncm_string_equal(mpd_fixture.content_path,
                            cstring_len(mpd_fixture.content_path),
                            LIT_ARGS("Fetched")));
    assert(native_playlist_editor_screen_current_song(&screen, &song));
    assert_song_uri(&song, LIT_ARGS("first.flac"));

    ncm_song_destroy(&song);
    ncm_playlist_destroy(&playlist);
    native_playlist_editor_screen_destroy(&screen);

    init_screen(&screen);
    ncm_error_clear(&error);
    assert(!native_playlist_editor_screen_reload_content_from_mpd(
        &screen, NULL, &error));
    assert(error.code == EINVAL);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_update_callback_uses_native_mpd_logic(void) {
    NativePlaylistEditorScreen screen;
    NcMenu *playlists;
    NcMenu *content;
    NcmPlaylist *playlist;
    bool old_ignore_leading_the;
    bool old_data_fetching_delay;

    old_ignore_leading_the = Config.ignore_leading_the;
    old_data_fetching_delay = Config.data_fetching_delay;
    Config.ignore_leading_the = true;
    Config.data_fetching_delay = false;

    ncm_mpd_playlist_list_clear(&mpd_fixture.playlists);
    ncm_mpd_song_list_clear(&mpd_fixture.content);
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("The Beta"));
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("Alpha"));
    append_song(&mpd_fixture.content, LIT_ARGS("alpha-one.flac"),
                LIT_ARGS("Alpha one"));
    reset_mpd_calls();

    init_screen(&screen);
    nc_screen_update(native_playlist_editor_screen_base(&screen));

    playlists = nc_playlist_entry_menu_base(&screen.playlists);
    content = nc_song_menu_base(&screen.content);
    assert(mpd_fixture.get_playlists_calls == 1);
    assert(mpd_fixture.get_content_calls == 1);
    assert(!screen.playlists_update_requested);
    assert(!screen.content_update_requested);
    assert(nc_menu_all_item_count(playlists) == 2);
    assert(nc_menu_all_item_count(content) == 1);

    playlist = nc_playlist_entry_menu_item_at(&screen.playlists,
                                              NC_MENU_ITEMS_ALL, 0);
    assert_playlist_path(playlist, LIT_ARGS("Alpha"));
    assert(ncm_string_equal(mpd_fixture.content_path,
                            cstring_len(mpd_fixture.content_path),
                            LIT_ARGS("Alpha")));

    native_playlist_editor_screen_destroy(&screen);
    Config.ignore_leading_the = old_ignore_leading_the;
    Config.data_fetching_delay = old_data_fetching_delay;
    return;
}

static void
test_delayed_update_and_empty_content_cache(void) {
    NativePlaylistEditorScreen screen;
    NcmTimePoint old_global_timer;
    bool old_data_fetching_delay;

    old_global_timer = global_timer;
    old_data_fetching_delay = Config.data_fetching_delay;
    Config.data_fetching_delay = true;
    global_timer.ns = 1000000000ll;

    ncm_mpd_playlist_list_clear(&mpd_fixture.playlists);
    ncm_mpd_song_list_clear(&mpd_fixture.content);
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("Empty"));
    reset_mpd_calls();

    init_screen(&screen);
    nc_screen_update(native_playlist_editor_screen_base(&screen));
    assert(mpd_fixture.get_playlists_calls == 1);
    assert(mpd_fixture.get_content_calls == 0);
    assert(screen.content_update_requested);
    assert(nc_screen_window_timeout(native_playlist_editor_screen_base(
               &screen)) == NATIVE_PLAYLIST_EDITOR_FETCH_DELAY_MS);

    global_timer.ns +=
        (NATIVE_PLAYLIST_EDITOR_FETCH_DELAY_MS + 1) * 1000000ll;
    nc_screen_update(native_playlist_editor_screen_base(&screen));
    assert(mpd_fixture.get_content_calls == 1);
    assert(!screen.content_update_requested);
    assert(screen.displayed_playlist_valid);
    assert(screen.last_known_content_count == 0);
    assert(nc_screen_window_timeout(native_playlist_editor_screen_base(
               &screen)) == NC_SCREEN_DEFAULT_WINDOW_TIMEOUT);

    nc_screen_update(native_playlist_editor_screen_base(&screen));
    assert(mpd_fixture.get_content_calls == 1);

    native_playlist_editor_screen_destroy(&screen);
    Config.data_fetching_delay = old_data_fetching_delay;
    global_timer = old_global_timer;
    return;
}

static void
test_native_scroll_fetches_new_playlist_content(void) {
    NativePlaylistEditorScreen screen;
    NcMenu *content;
    bool old_data_fetching_delay;

    old_data_fetching_delay = Config.data_fetching_delay;
    Config.data_fetching_delay = false;

    ncm_mpd_playlist_list_clear(&mpd_fixture.playlists);
    ncm_mpd_song_list_clear(&mpd_fixture.content);
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("First"));
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("Second"));
    append_song(&mpd_fixture.content, LIT_ARGS("selected.flac"),
                LIT_ARGS("Selected"));
    reset_mpd_calls();

    init_screen(&screen);
    nc_screen_update(native_playlist_editor_screen_base(&screen));
    assert(mpd_fixture.get_content_calls == 1);
    assert(ncm_string_equal(mpd_fixture.content_path,
                            cstring_len(mpd_fixture.content_path),
                            LIT_ARGS("First")));

    nc_screen_scroll(native_playlist_editor_screen_base(&screen),
                     NC_SCROLL_DOWN);
    content = nc_song_menu_base(&screen.content);
    assert(nc_menu_all_item_count(content) == 0);
    assert(screen.content_update_requested);

    nc_screen_update(native_playlist_editor_screen_base(&screen));
    assert(mpd_fixture.get_content_calls == 2);
    assert(ncm_string_equal(mpd_fixture.content_path,
                            cstring_len(mpd_fixture.content_path),
                            LIT_ARGS("Second")));
    assert(nc_menu_all_item_count(content) == 1);

    native_playlist_editor_screen_destroy(&screen);
    Config.data_fetching_delay = old_data_fetching_delay;
    return;
}

static void
test_mouse_right_click_actions(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcMenu *playlist_menu;
    NcMenu *content_menu;
    MEVENT event;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    append_playlist(&playlists, LIT_ARGS("First"));
    append_playlist(&playlists, LIT_ARGS("Second"));
    append_song(&content, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    append_song(&content, LIT_ARGS("two.flac"), LIT_ARGS("Two"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    playlist_menu = nc_playlist_entry_menu_base(&screen.playlists);
    content_menu = nc_song_menu_base(&screen.content);

    reset_mpd_calls();
    event = (MEVENT){0};
    event.x = 1;
    event.y = 2;
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(
        native_playlist_editor_screen_base(&screen), event);
    assert(screen.active_column
           == NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS);
    assert(nc_menu_highlight(playlist_menu) == 0);
    assert(mpd_fixture.load_calls == 1);
    assert(ncm_string_equal(mpd_fixture.command_playlist,
                            cstring_len(mpd_fixture.command_playlist),
                            LIT_ARGS("First")));
    assert(mpd_fixture.status_update_full_calls == 1);
    assert(ncm_string_equal(mpd_fixture.status_message,
                            cstring_len(mpd_fixture.status_message),
                            LIT_ARGS("Playlist \"First\" loaded")));

    reset_mpd_calls();
    event = (MEVENT){0};
    event.x = 41;
    event.y = 3;
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(
        native_playlist_editor_screen_base(&screen), event);
    assert(screen.active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT);
    assert(nc_menu_highlight(content_menu) == 1);
    assert(mpd_fixture.add_song_calls == 1);
    assert(mpd_fixture.add_song_play);
    assert(mpd_fixture.add_song_position == -1);
    assert(ncm_string_equal(mpd_fixture.added_song_uri,
                            cstring_len(mpd_fixture.added_song_uri),
                            LIT_ARGS("two.flac")));

    reset_mpd_calls();
    event = (MEVENT){0};
    event.x = 1;
    event.y = 3;
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(
        native_playlist_editor_screen_base(&screen), event);
    assert(screen.active_column
           == NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS);
    assert(nc_menu_highlight(playlist_menu) == 1);
    assert(nc_menu_all_item_count(content_menu) == 0);
    assert(screen.content_update_requested);
    assert(mpd_fixture.load_calls == 1);
    assert(ncm_string_equal(mpd_fixture.command_playlist,
                            cstring_len(mpd_fixture.command_playlist),
                            LIT_ARGS("Second")));

    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_finish_list_change_callback_and_mouse_wheel(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcMenu *playlist_menu;
    NcMenu *content_menu;
    MEVENT event;
    NcmError error;
    uint32 old_lines_scrolled;
    bool old_whole_page;

    old_lines_scrolled = Config.lines_scrolled;
    old_whole_page = Config.mouse_list_scroll_whole_page;
    Config.lines_scrolled = 1;
    Config.mouse_list_scroll_whole_page = false;

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    append_playlist(&playlists, LIT_ARGS("First"));
    append_playlist(&playlists, LIT_ARGS("Second"));
    append_playlist(&playlists, LIT_ARGS("Third"));
    append_song(&content, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    append_song(&content, LIT_ARGS("two.flac"), LIT_ARGS("Two"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    playlist_menu = nc_playlist_entry_menu_base(&screen.playlists);
    content_menu = nc_song_menu_base(&screen.content);

    event = (MEVENT){0};
    event.x = 1;
    event.y = 2;
    event.bstate = BUTTON5_PRESSED;
    nc_screen_mouse_button_pressed(
        native_playlist_editor_screen_base(&screen), event);
    assert(nc_menu_highlight(playlist_menu) == 1);
    assert(nc_menu_all_item_count(content_menu) == 0);
    assert(screen.content_update_requested);
    assert(!screen.displayed_playlist_valid);

    assert(native_playlist_editor_screen_load_content(&screen, &content));
    assert(nc_menu_all_item_count(content_menu) == 2);
    nc_menu_highlight_position(playlist_menu, 2, screen.main_height);
    nc_screen_finish_list_change(native_playlist_editor_screen_base(
        &screen));
    assert(nc_menu_all_item_count(content_menu) == 0);
    assert(screen.content_update_requested);

    assert(native_playlist_editor_screen_load_content(&screen, &content));
    nc_menu_highlight_position(playlist_menu, 0, screen.main_height);
    native_playlist_editor_screen_finish_list_change(&screen);
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_search_active(
        &screen, LIT_ARGS("Second"), NCM_REGEX_LITERAL_CASE_INSENSITIVE,
        true, true, false, &error));
    assert(nc_menu_highlight(playlist_menu) == 1);
    assert(nc_menu_all_item_count(content_menu) == 0);
    assert(screen.content_update_requested);

    Config.lines_scrolled = old_lines_scrolled;
    Config.mouse_list_scroll_whole_page = old_whole_page;
    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}


static void
test_locate_playlist_fetches_content_and_switches(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList initial;
    NcmMpdSongList initial_content;
    NcmError error;
    NcmSong song;
    char *path;
    int32 path_len;

    ncm_mpd_playlist_list_clear(&mpd_fixture.playlists);
    ncm_mpd_song_list_clear(&mpd_fixture.content);
    ncm_mpd_song_list_clear(&mpd_fixture.alternate_content);
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("Beta"));
    append_playlist(&mpd_fixture.playlists, LIT_ARGS("Alpha"));
    append_song(&mpd_fixture.content, LIT_ARGS("alpha.flac"),
                LIT_ARGS("Alpha"));
    reset_mpd_calls();

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&initial);
    ncm_mpd_song_list_init(&initial_content);
    ncm_song_init(&song);
    append_playlist(&initial, LIT_ARGS("Beta"));
    append_playlist(&initial, LIT_ARGS("Alpha"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &initial));
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("Beta"), NCM_REGEX_LITERAL_CASE_INSENSITIVE,
        &error));
    assert(nc_menu_item_count(nc_playlist_entry_menu_base(
               &screen.playlists)) == 1);

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_locate_playlist(
        &screen, NULL, LIT_ARGS("Alpha"), &error));
    assert(mpd_fixture.get_playlists_calls == 1);
    assert(mpd_fixture.get_content_calls == 1);
    assert(mpd_fixture.get_content_no_info_calls == 0);
    assert(app_controller_is_current_screen(
               native_playlist_editor_screen_base(&screen)));
    assert(native_playlist_editor_screen_current_playlist_path(
        &screen, &path, &path_len));
    assert(ncm_string_equal(path, path_len, LIT_ARGS("Alpha")));
    assert(nc_menu_item_count(nc_playlist_entry_menu_base(
               &screen.playlists)) == 2);
    assert(!screen.playlist_filter_enabled);
    assert(native_playlist_editor_screen_current_content_song(&screen,
                                                              &song));
    assert_song_uri(&song, LIT_ARGS("alpha.flac"));
    assert(ncm_string_equal(mpd_fixture.content_path,
                            cstring_len(mpd_fixture.content_path),
                            LIT_ARGS("Alpha")));

    ncm_song_destroy(&song);
    ncm_mpd_song_list_destroy(&initial_content);
    ncm_mpd_playlist_list_destroy(&initial);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_locate_song_searches_current_playlist_first(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcmSong target;
    NcmSong current;
    NcmError error;

    reset_mpd_calls();
    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    ncm_song_init(&target);
    ncm_song_init(&current);
    append_playlist(&playlists, LIT_ARGS("Current"));
    append_song(&content, LIT_ARGS("current.flac"),
                LIT_ARGS("Current"));
    append_song(&content, LIT_ARGS("target.flac"),
                LIT_ARGS("Target"));
    assert(ncm_song_set_uri(&target, LIT_ARGS("target.flac")));
    assert(ncm_song_add_tag(&target, MPD_TAG_TITLE,
                            LIT_ARGS("Target")));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    native_playlist_editor_screen_next_column(&screen);

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_locate_song(&screen, NULL,
                                                     &target, &error));
    assert(mpd_fixture.get_content_no_info_calls == 0);
    assert(mpd_fixture.get_content_calls == 0);
    assert(mpd_fixture.status_calls == 0);
    assert(native_playlist_editor_screen_current_content_song(&screen,
                                                              &current));
    assert_song_uri(&current, LIT_ARGS("target.flac"));
    assert(screen.active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT);

    ncm_song_destroy(&current);
    ncm_song_destroy(&target);
    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_locate_song_uses_legacy_playlist_order(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcmSong target;
    NcmSong current;
    NcmError error;
    char *path;
    int32 path_len;

    ncm_mpd_song_list_clear(&mpd_fixture.content);
    ncm_mpd_song_list_clear(&mpd_fixture.alternate_content);
    append_song(&mpd_fixture.content, LIT_ARGS("other.flac"),
                LIT_ARGS("Other"));
    append_song(&mpd_fixture.alternate_content, LIT_ARGS("noise.flac"),
                LIT_ARGS("Noise"));
    append_song(&mpd_fixture.alternate_content, LIT_ARGS("target.flac"),
                LIT_ARGS("Target"));
    copy_cstring(mpd_fixture.alternate_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.alternate_content_path),
                 "Target");
    reset_mpd_calls();
    copy_cstring(mpd_fixture.alternate_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.alternate_content_path),
                 "Target");

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    ncm_song_init(&target);
    ncm_song_init(&current);
    append_playlist(&playlists, LIT_ARGS("Current"));
    append_playlist(&playlists, LIT_ARGS("Miss"));
    append_playlist(&playlists, LIT_ARGS("Target"));
    append_song(&content, LIT_ARGS("before.flac"), LIT_ARGS("Before"));
    append_song(&content, LIT_ARGS("current.flac"),
                LIT_ARGS("Current"));
    append_song(&content, LIT_ARGS("later.flac"), LIT_ARGS("Later"));
    assert(ncm_song_set_uri(&target, LIT_ARGS("target.flac")));
    assert(ncm_song_add_tag(&target, MPD_TAG_TITLE,
                            LIT_ARGS("Target")));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    native_playlist_editor_screen_next_column(&screen);
    nc_menu_highlight_position(nc_song_menu_base(&screen.content), 1,
                               screen.main_height);

    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_locate_song(&screen, NULL,
                                                     &target, &error));
    assert(mpd_fixture.status_calls == 1);
    assert(ncm_string_equal(mpd_fixture.status_message,
                            cstring_len(mpd_fixture.status_message),
                            LIT_ARGS("Jumping to song...")));
    assert(mpd_fixture.get_content_no_info_calls == 2);
    assert(mpd_fixture.get_content_calls == 1);
    assert(ncm_string_equal(mpd_fixture.no_info_content_path,
                            cstring_len(mpd_fixture.no_info_content_path),
                            LIT_ARGS("Target")));
    assert(ncm_string_equal(mpd_fixture.content_path,
                            cstring_len(mpd_fixture.content_path),
                            LIT_ARGS("Target")));
    assert(native_playlist_editor_screen_current_playlist_path(
        &screen, &path, &path_len));
    assert(ncm_string_equal(path, path_len, LIT_ARGS("Target")));
    assert(native_playlist_editor_screen_current_content_song(&screen,
                                                              &current));
    assert_song_uri(&current, LIT_ARGS("target.flac"));
    assert(screen.active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT);

    ncm_song_destroy(&current);
    ncm_song_destroy(&target);
    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_locate_song_reports_not_found(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcmSong target;
    NcmError error;

    ncm_mpd_song_list_clear(&mpd_fixture.content);
    ncm_mpd_song_list_clear(&mpd_fixture.alternate_content);
    append_song(&mpd_fixture.content, LIT_ARGS("other.flac"),
                LIT_ARGS("Other"));
    reset_mpd_calls();

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    ncm_song_init(&target);
    append_playlist(&playlists, LIT_ARGS("Current"));
    append_playlist(&playlists, LIT_ARGS("Later"));
    append_song(&content, LIT_ARGS("current.flac"),
                LIT_ARGS("Current"));
    assert(ncm_song_set_uri(&target, LIT_ARGS("missing.flac")));
    assert(ncm_song_add_tag(&target, MPD_TAG_TITLE,
                            LIT_ARGS("Missing")));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    native_playlist_editor_screen_next_column(&screen);

    ncm_error_clear(&error);
    assert(!native_playlist_editor_screen_locate_song(&screen, NULL,
                                                      &target, &error));
    assert(!ncm_error_is_set(&error));
    assert(mpd_fixture.get_content_no_info_calls == 1);
    assert(mpd_fixture.status_calls == 2);
    assert(ncm_string_equal(mpd_fixture.status_message,
                            cstring_len(mpd_fixture.status_message),
                            LIT_ARGS("Song was not found in playlists")));

    ncm_song_destroy(&target);
    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_update_error_reporting_and_flags(void) {
    NativePlaylistEditorScreen screen;
    bool old_data_fetching_delay;

    old_data_fetching_delay = Config.data_fetching_delay;
    Config.data_fetching_delay = false;

    ncm_mpd_playlist_list_clear(&mpd_fixture.playlists);
    ncm_mpd_song_list_clear(&mpd_fixture.content);
    reset_mpd_calls();
    mpd_fixture.get_playlists_result = false;

    init_screen(&screen);
    nc_screen_update(native_playlist_editor_screen_base(&screen));
    assert(mpd_fixture.get_playlists_calls == 1);
    assert(!screen.playlists_update_requested);
    assert(screen.content_update_requested);
    assert(mpd_fixture.status_calls == 1);
    assert(ncm_string_equal(mpd_fixture.status_message,
                            cstring_len(mpd_fixture.status_message),
                            LIT_ARGS("Could not fetch playlists: "
                                     "playlist failure")));
    native_playlist_editor_screen_destroy(&screen);

    append_playlist(&mpd_fixture.playlists, LIT_ARGS("Broken"));
    reset_mpd_calls();
    mpd_fixture.get_content_result = false;
    init_screen(&screen);
    nc_screen_update(native_playlist_editor_screen_base(&screen));
    assert(mpd_fixture.get_playlists_calls == 1);
    assert(mpd_fixture.get_content_calls == 1);
    assert(!screen.content_update_requested);
    assert(mpd_fixture.status_calls == 1);
    assert(ncm_string_equal(mpd_fixture.status_message,
                            cstring_len(mpd_fixture.status_message),
                            LIT_ARGS("Could not fetch playlist content: "
                                     "content failure")));
    native_playlist_editor_screen_destroy(&screen);

    Config.data_fetching_delay = old_data_fetching_delay;
    return;
}

static void
test_playlist_commands(void) {
    NativePlaylistEditorScreen screen;
    NativePlaylistEditorCommand command;
    NcmMpdPlaylistList playlists;
    NcmError error;
    bool loaded;

    init_screen(&screen);
    native_playlist_editor_command_init(&command);
    ncm_mpd_playlist_list_init(&playlists);
    append_playlist(&playlists, LIT_ARGS("Original"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    reset_mpd_calls();

    assert(native_playlist_editor_screen_prepare_playlist_command(
        &screen, NATIVE_PLAYLIST_EDITOR_COMMAND_SAVE, NULL, 0,
        &command));
    ncm_error_clear(&error);
    assert(native_playlist_editor_command_execute(&command, NULL, &error));
    assert(mpd_fixture.save_calls == 1);
    assert(ncm_string_equal(mpd_fixture.command_playlist,
                            cstring_len(mpd_fixture.command_playlist),
                            LIT_ARGS("Original")));

    assert(native_playlist_editor_screen_prepare_playlist_command(
        &screen, NATIVE_PLAYLIST_EDITOR_COMMAND_RENAME,
        LIT_ARGS("Renamed"), &command));
    ncm_error_clear(&error);
    assert(native_playlist_editor_command_execute(&command, NULL, &error));
    assert(mpd_fixture.rename_calls == 1);
    assert(ncm_string_equal(mpd_fixture.command_playlist,
                            cstring_len(mpd_fixture.command_playlist),
                            LIT_ARGS("Original")));
    assert(ncm_string_equal(mpd_fixture.command_target,
                            cstring_len(mpd_fixture.command_target),
                            LIT_ARGS("Renamed")));

    assert(native_playlist_editor_screen_prepare_playlist_command(
        &screen, NATIVE_PLAYLIST_EDITOR_COMMAND_DELETE, NULL, 0,
        &command));
    ncm_error_clear(&error);
    assert(native_playlist_editor_command_execute(&command, NULL, &error));
    assert(mpd_fixture.delete_calls == 1);

    assert(native_playlist_editor_screen_prepare_playlist_command(
        &screen, NATIVE_PLAYLIST_EDITOR_COMMAND_LOAD, NULL, 0,
        &command));
    ncm_error_clear(&error);
    assert(native_playlist_editor_command_execute(&command, NULL, &error));
    assert(mpd_fixture.load_calls == 1);

    loaded = false;
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_load_current_playlist(
        &screen, NULL, &loaded, &error));
    assert(loaded);
    assert(mpd_fixture.load_calls == 2);

    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_command_destroy(&command);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

static void
test_native_rendering_callbacks(void) {
    NativePlaylistEditorScreen screen;
    PlaylistEditorFormatFixture formats;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcmError error;
    NcMenu *playlist_menu;
    NcMenu *content_menu;
    NcmPlaylist *playlist;
    NcmSong *song;
    Column columns[2];
    char invalid_path[] = {
        'b', 'a', 'd', (char)0xff, '-', 'p', 'a', 't', 'h',
    };

    begin_render_formats(&formats, columns);
    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    append_playlist(&playlists,
                    LIT_ARGS("Very long road trip playlist name"));
    append_playlist(&playlists, invalid_path, NCM_ARRAY_LEN(invalid_path));
    append_song(&content,
                LIT_ARGS("very-long-empty-metadata-file-name.flac"),
                NULL, 0);
    append_song(&content, LIT_ARGS("tagged.flac"), LIT_ARGS("TitleLong"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    assert(ncm_string_equal(screen.content_title.data,
                            screen.content_title.len,
                            LIT_ARGS("Content (2 items)")));

    playlist_menu = nc_playlist_entry_menu_base(&screen.playlists);
    content_menu = nc_song_menu_base(&screen.content);
    assert(playlist_menu->display_callbacks.draw != NULL);
    assert(content_menu->display_callbacks.draw != NULL);

    playlist = nc_playlist_entry_menu_item_at(&screen.playlists,
                                              NC_MENU_ITEMS_ALL, 0);
    reset_window_trace();
    playlist_menu->display_callbacks.draw(
        playlist_menu, &screen.playlists_window, playlist, 0,
        playlist_menu->display_callbacks.user);
    assert(ncm_string_equal(window_trace.printed,
                            window_trace.printed_len,
                            LIT_ARGS("Very long road trip playlist name")));

    playlist = nc_playlist_entry_menu_item_at(&screen.playlists,
                                              NC_MENU_ITEMS_ALL, 1);
    reset_window_trace();
    playlist_menu->display_callbacks.draw(
        playlist_menu, &screen.playlists_window, playlist, 1,
        playlist_menu->display_callbacks.user);
    assert(ncm_string_equal(window_trace.printed,
                            window_trace.printed_len,
                            invalid_path,
                            NCM_ARRAY_LEN(invalid_path)));

    song = nc_song_menu_item_at(&screen.content, NC_MENU_ITEMS_ALL, 0);
    reset_window_trace();
    content_menu->display_callbacks.draw(
        content_menu, &screen.content_window, song, 0,
        content_menu->display_callbacks.user);
    assert(ncm_string_equal(
        window_trace.printed, window_trace.printed_len,
        LIT_ARGS("classic:very-long-empty-metadata-file-name.flac")));

    native_playlist_editor_screen_next_column(&screen);
    ncm_error_clear(&error);
    assert(native_playlist_editor_screen_apply_active_filter(
        &screen, LIT_ARGS("tagged"),
        NCM_REGEX_LITERAL_CASE_INSENSITIVE, &error));
    assert(nc_menu_item_count(content_menu) == 1);
    assert(ncm_string_equal(screen.content_title.data,
                            screen.content_title.len,
                            LIT_ARGS("Content (1 item)")));

    song = nc_song_menu_item_at(&screen.content, NC_MENU_ITEMS_ALL, 1);
    assert(ncm_song_add_tag(song, MPD_TAG_ARTIST,
                            LIT_ARGS("ArtistLong")));
    Config.playlist_editor_display_mode = NCM_DISPLAY_MODE_COLUMNS;
    screen.content_window.width = 10;
    reset_window_trace();
    content_menu->display_callbacks.draw(
        content_menu, &screen.content_window, song, 0,
        content_menu->display_callbacks.user);
    assert(ncm_string_equal(window_trace.printed,
                            window_trace.printed_len,
                            LIT_ARGS("Artis ")));

    native_playlist_editor_screen_clear_active_filter(&screen);
    assert(ncm_string_equal(screen.content_title.data,
                            screen.content_title.len,
                            LIT_ARGS("Content (2 items)")));

    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    end_render_formats(&formats);
    return;
}

static void
test_native_refresh_and_mouse_fallback(void) {
    NativePlaylistEditorScreen screen;
    NcmMpdPlaylistList playlists;
    NcmMpdSongList content;
    NcMenu *playlist_menu;
    NcMenu *content_menu;
    MEVENT event = {0};

    init_screen(&screen);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_mpd_song_list_init(&content);
    append_playlist(&playlists, LIT_ARGS("First"));
    append_playlist(&playlists, LIT_ARGS("Second"));
    append_song(&content, LIT_ARGS("one.flac"), LIT_ARGS("One"));
    append_song(&content, LIT_ARGS("two.flac"), LIT_ARGS("Two"));
    assert(native_playlist_editor_screen_load_playlists(&screen,
                                                         &playlists));
    assert(native_playlist_editor_screen_load_content(&screen, &content));
    playlist_menu = nc_playlist_entry_menu_base(&screen.playlists);
    content_menu = nc_song_menu_base(&screen.content);
    reset_window_trace();

    nc_screen_refresh(native_playlist_editor_screen_base(&screen));
    assert(window_trace.display_calls == 2);
    assert(window_trace.menu_refresh_calls == 2);
    assert(window_trace.separator_calls == 1);
    assert(window_trace.last_separator_x == 39);
    nc_screen_refresh_window(native_playlist_editor_screen_base(&screen));
    assert(window_trace.display_calls == 3);
    assert(window_trace.menu_refresh_calls == 3);
    assert(window_trace.separator_calls == 1);

    event.x = 1;
    event.y = 3;
    event.bstate = BUTTON1_PRESSED;
    nc_screen_mouse_button_pressed(
        native_playlist_editor_screen_base(&screen), event);
    assert(screen.active_column
           == NATIVE_PLAYLIST_EDITOR_COLUMN_PLAYLISTS);
    assert(nc_menu_highlight(playlist_menu) == 1);
    assert(nc_menu_all_item_count(content_menu) == 0);
    assert(screen.content_update_requested);
    assert(!screen.displayed_playlist_valid);
    assert(screen.last_known_content_count == -1);
    assert(ncm_string_equal(screen.content_title.data,
                            screen.content_title.len,
                            LIT_ARGS("Content")));

    assert(native_playlist_editor_screen_load_content(&screen, &content));
    event.x = 41;
    event.y = 3;
    nc_screen_mouse_button_pressed(
        native_playlist_editor_screen_base(&screen), event);
    assert(screen.active_column == NATIVE_PLAYLIST_EDITOR_COLUMN_CONTENT);
    assert(nc_menu_highlight(content_menu) == 1);

    ncm_mpd_song_list_destroy(&content);
    ncm_mpd_playlist_list_destroy(&playlists);
    native_playlist_editor_screen_destroy(&screen);
    return;
}

void
__wrap_nc_screen_draw_vertical_separator(int64 x) {
    window_trace.separator_calls += 1;
    window_trace.last_separator_x = x;
    return;
}

void
__wrap_nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
                      int64 width, int64 height, char *title,
                      int32 title_len, NcColor color, NcBorder border) {
    (void)color;
    *window = (NcWindow){0};
    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
    window->border = border;
    __wrap_nc_window_set_title(window, title, title_len);
    return;
}

void
__wrap_nc_window_destroy(NcWindow *window) {
    *window = (NcWindow){0};
    return;
}

void
__wrap_nc_window_set_title(NcWindow *window, char *title,
                           int32 title_len) {
    bool had_title;
    bool has_title;

    had_title = window->title_len > 0;
    has_title = title_len > 0;
    if (has_title && !had_title) {
        window->start_y += 2;
        window->height -= 2;
    } else if (!has_title && had_title) {
        window->start_y -= 2;
        window->height += 2;
    }
    window->title = title;
    window->title_len = title_len;
    return;
}

void
__wrap_nc_window_move_to(NcWindow *window, int64 new_x, int64 new_y) {
    window->start_x = new_x;
    window->start_y = new_y;
    if (window->border.enabled) {
        window->start_x += 1;
        window->start_y += 1;
    }
    if (window->title_len > 0) {
        window->start_y += 2;
    }
    return;
}

void
__wrap_nc_window_resize(NcWindow *window, int64 width, int64 height) {
    if (window->border.enabled) {
        width -= 2;
        height -= 2;
    }
    if (window->title_len > 0) {
        height -= 2;
    }
    window->width = width;
    window->height = height;
    return;
}

void
__wrap_nc_window_display(NcWindow *window) {
    (void)window;
    window_trace.display_calls += 1;
    return;
}

bool
__wrap_nc_window_has_coords(NcWindow *window, int32 *x, int32 *y) {
    *x -= (int32)window->start_x;
    *y -= (int32)window->start_y;
    return (*x >= 0) && (*x < window->width)
        && (*y >= 0) && (*y < window->height);
}

int32
__wrap_nc_window_get_x(NcWindow *window) {
    (void)window;
    return window_trace.printed_len;
}

void
__wrap_nc_window_print_data(NcWindow *window, char *string,
                            int32 string_len) {
    (void)window;
    if ((string == NULL) || (string_len <= 0)) {
        return;
    }
    assert((window_trace.printed_len + string_len)
           < (int32)sizeof(window_trace.printed));
    cbase_memcpy(window_trace.printed + window_trace.printed_len,
               string, string_len);
    window_trace.printed_len += string_len;
    window_trace.printed[window_trace.printed_len] = '\0';
    return;
}

void
__wrap_nc_window_print_char(NcWindow *window, char ch) {
    (void)window;
    assert(window_trace.printed_len
           < (int32)sizeof(window_trace.printed) - 1);
    window_trace.printed[window_trace.printed_len] = ch;
    window_trace.printed_len += 1;
    window_trace.printed[window_trace.printed_len] = '\0';
    return;
}

void
__wrap_nc_menu_refresh(NcMenu *menu, NcWindow *window, int64 width,
                       int64 height) {
    (void)menu;
    (void)window;
    (void)width;
    (void)height;
    window_trace.menu_refresh_calls += 1;
    return;
}

bool
__wrap_ncm_mpd_client_get_playlists(NcmMpdClient *client,
                                    NcmMpdPlaylistList *playlists,
                                    NcmError *error) {
    (void)client;
    mpd_fixture.get_playlists_calls += 1;
    ncm_error_clear(error);
    if (!mpd_fixture.get_playlists_result) {
        ncm_error_set(error, EIO, STRLIT_ARGS("playlist failure"));
        return false;
    }
    return ncm_mpd_playlist_list_copy(playlists,
                                      &mpd_fixture.playlists);
}

bool
__wrap_ncm_mpd_client_get_playlist_content(
    NcmMpdClient *client, char *path, NcmMpdSongList *songs,
    NcmError *error) {
    (void)client;
    mpd_fixture.get_content_calls += 1;
    copy_cstring(mpd_fixture.content_path,
                 NCM_ARRAY_LEN(mpd_fixture.content_path), path);
    ncm_error_clear(error);
    if (!mpd_fixture.get_content_result
        || ((mpd_fixture.failing_content_path[0] != '\0')
            && ncm_string_equal(
                path, cstring_len(path),
                mpd_fixture.failing_content_path,
                cstring_len(mpd_fixture.failing_content_path)))) {
        ncm_error_set(error, EIO, STRLIT_ARGS("content failure"));
        return false;
    }
    if ((mpd_fixture.alternate_content_path[0] != '\0')
        && ncm_string_equal(path, cstring_len(path),
                            mpd_fixture.alternate_content_path,
                            cstring_len(
                                mpd_fixture.alternate_content_path))) {
        return ncm_mpd_song_list_copy(songs,
                                      &mpd_fixture.alternate_content);
    }
    return ncm_mpd_song_list_copy(songs, &mpd_fixture.content);
}

bool
__wrap_ncm_mpd_client_get_playlist_content_no_info(
    NcmMpdClient *client, char *path, NcmMpdSongList *songs,
    NcmError *error) {
    (void)client;
    mpd_fixture.get_content_no_info_calls += 1;
    copy_cstring(mpd_fixture.no_info_content_path,
                 NCM_ARRAY_LEN(mpd_fixture.no_info_content_path), path);
    ncm_error_clear(error);
    if (!mpd_fixture.get_content_result
        || ((mpd_fixture.failing_content_path[0] != '\0')
            && ncm_string_equal(
                path, cstring_len(path),
                mpd_fixture.failing_content_path,
                cstring_len(mpd_fixture.failing_content_path)))) {
        ncm_error_set(error, EIO, STRLIT_ARGS("content failure"));
        return false;
    }
    if ((mpd_fixture.alternate_content_path[0] != '\0')
        && ncm_string_equal(path, cstring_len(path),
                            mpd_fixture.alternate_content_path,
                            cstring_len(
                                mpd_fixture.alternate_content_path))) {
        return ncm_mpd_song_list_copy(songs,
                                      &mpd_fixture.alternate_content);
    }
    return ncm_mpd_song_list_copy(songs, &mpd_fixture.content);
}

void
__wrap_ncm_statusbar_print(int32 delay_seconds, char *message,
                           int32 message_len) {
    (void)delay_seconds;
    assert(message_len < NCM_ARRAY_LEN(mpd_fixture.status_message));
    mpd_fixture.status_calls += 1;
    if (message_len > 0) {
        cbase_memcpy(mpd_fixture.status_message, message, message_len);
    }
    mpd_fixture.status_message[message_len] = '\0';
    return;
}

void
__wrap_ncm_statusbar_print_cstring(int32 delay_seconds, char *message) {
    (void)delay_seconds;
    mpd_fixture.status_calls += 1;
    copy_cstring(mpd_fixture.status_message,
                 NCM_ARRAY_LEN(mpd_fixture.status_message), message);
    return;
}

bool
__wrap_ncm_status_update_full(NcmMpdClient *client,
                              NcmStatusHooks *hooks,
                              NcmError *error) {
    (void)client;
    (void)hooks;
    mpd_fixture.status_update_full_calls += 1;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_action_add_song_to_playlist(NcmSong *song, bool play,
                                       int32 position) {
    NcmStringView uri;

    mpd_fixture.add_song_calls += 1;
    mpd_fixture.add_song_play = play;
    mpd_fixture.add_song_position = position;
    assert(ncm_song_uri_view(song, 0, &uri));
    assert(uri.len < NCM_ARRAY_LEN(mpd_fixture.added_song_uri));
    if (uri.len > 0) {
        cbase_memcpy(mpd_fixture.added_song_uri, uri.data, uri.len);
    }
    mpd_fixture.added_song_uri[uri.len] = '\0';
    return true;
}

bool
__wrap_ncm_mpd_client_save_playlist(NcmMpdClient *client, char *name,
                                    NcmError *error) {
    (void)client;
    mpd_fixture.save_calls += 1;
    copy_cstring(mpd_fixture.command_playlist,
                 NCM_ARRAY_LEN(mpd_fixture.command_playlist), name);
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_rename_playlist(NcmMpdClient *client, char *from,
                                      char *to, NcmError *error) {
    (void)client;
    mpd_fixture.rename_calls += 1;
    copy_cstring(mpd_fixture.command_playlist,
                 NCM_ARRAY_LEN(mpd_fixture.command_playlist), from);
    copy_cstring(mpd_fixture.command_target,
                 NCM_ARRAY_LEN(mpd_fixture.command_target), to);
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_delete_playlist(NcmMpdClient *client, char *name,
                                      NcmError *error) {
    (void)client;
    mpd_fixture.delete_calls += 1;
    copy_cstring(mpd_fixture.command_playlist,
                 NCM_ARRAY_LEN(mpd_fixture.command_playlist), name);
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_load_playlist(NcmMpdClient *client, char *name,
                                    bool *loaded, NcmError *error) {
    (void)client;
    mpd_fixture.load_calls += 1;
    copy_cstring(mpd_fixture.command_playlist,
                 NCM_ARRAY_LEN(mpd_fixture.command_playlist), name);
    if (loaded != NULL) {
        *loaded = mpd_fixture.load_result;
    }
    ncm_error_clear(error);
    return true;
}
