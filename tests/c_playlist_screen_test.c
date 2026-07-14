#include <assert.h>
#include <stdlib.h>

#include "c/ncm_format.h"
#include "c/ncm_string.h"
#include "global.h"
#include "settings.h"
#include "curses/nc_app_menus.h"
#include "screens/nc_playlist.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct PlaylistWindowTrace {
    char title[256];
    char printed[512];

    int32 title_len;
    int32 printed_len;
    int32 display_calls;
    int32 menu_refresh_calls;
    int32 play_calls;
    int32 played_id;
} PlaylistWindowTrace;

typedef struct PlaylistMpdFixture {
    NcmMpdSongList queue;
    NcmMpdSongList changes;

    int32 queue_calls;
    int32 changes_calls;
} PlaylistMpdFixture;

typedef struct PlaylistFormatFixture {
    NcmFormatAst old_classic;
    NcmFormatAst old_columns;
    enum DisplayMode old_mode;
    uint32 old_regex_type;
} PlaylistFormatFixture;

static PlaylistWindowTrace window_trace;
static PlaylistMpdFixture mpd_fixture;

static void reset_window_trace(void);
static void init_screen(NativePlaylistScreen *screen);
static void append_song(NcmMpdSongList *songs, char *uri, int32 uri_len,
                        uint32 position, uint32 id);
static void add_song(NativePlaylistScreen *screen, char *uri,
                     int32 uri_len, uint32 position, uint32 id);
static void assert_song_uri(NcmSong *song, char *uri, int32 uri_len);
static bool display_should_mark_playlist_song(
    NativePlaylistScreen *screen, NcMenu *source, NcmSong *song);
static void begin_formats(PlaylistFormatFixture *fixture);
static void end_formats(PlaylistFormatFixture *fixture);
static void test_playlist_row_ownership(void);
static void test_playlist_selection_range(void);
static void test_native_initialization(void);
static void test_bridge_free_load_filter_search_selection(void);
static void test_bridge_free_mpd_updates_and_membership(void);
static void test_native_storage_is_authoritative(void);
static void test_locate_updates_active_display_menu(void);
static void test_native_display_and_column_title(void);
static void test_native_highlight_timeout(void);
static void test_native_activation_and_mouse(void);
static void test_playlist_updates_current_mutable_song(void);
void __wrap_nc_window_set_title(NcWindow *window, char *title,
                                int32 title_len);

int
main(void) {
    ncm_mpd_song_list_init(&mpd_fixture.queue);
    ncm_mpd_song_list_init(&mpd_fixture.changes);

    test_playlist_row_ownership();
    test_playlist_selection_range();
    test_native_initialization();
    test_bridge_free_load_filter_search_selection();
    test_bridge_free_mpd_updates_and_membership();
    test_native_storage_is_authoritative();
    test_locate_updates_active_display_menu();
    test_native_display_and_column_title();
    test_native_highlight_timeout();
    test_native_activation_and_mouse();
    test_playlist_updates_current_mutable_song();

    ncm_mpd_song_list_destroy(&mpd_fixture.changes);
    ncm_mpd_song_list_destroy(&mpd_fixture.queue);
    exit(EXIT_SUCCESS);
}

static void
reset_window_trace(void) {
    window_trace = (PlaylistWindowTrace){0};
    return;
}

static void
init_screen(NativePlaylistScreen *screen) {
    native_playlist_screen_init(screen, 0, 80, 0, 24,
                                nc_color_default(), nc_border_none());
    return;
}

static void
append_song(NcmMpdSongList *songs, char *uri, int32 uri_len,
            uint32 position, uint32 id) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    ncm_song_set_position(&song, position);
    ncm_song_set_id(&song, id);
    assert(ncm_mpd_song_list_append_copy(songs, &song));
    ncm_song_destroy(&song);
    return;
}

static void
add_song(NativePlaylistScreen *screen, char *uri, int32 uri_len,
         uint32 position, uint32 id) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    ncm_song_set_position(&song, position);
    ncm_song_set_id(&song, id);
    assert(native_playlist_screen_add_song_copy(screen, &song));
    ncm_song_destroy(&song);
    return;
}

static void
assert_song_uri(NcmSong *song, char *uri, int32 uri_len) {
    NcmStringView view;

    assert(ncm_song_uri_view(song, 0, &view));
    assert(ncm_string_equal(view.data, view.len, uri, uri_len));
    return;
}

static bool
display_should_mark_playlist_song(NativePlaylistScreen *screen,
                                  NcMenu *source, NcmSong *song) {
    bool is_playlist_menu;

    is_playlist_menu = source == nc_playlist_screen_menu(
        native_playlist_screen_playlist(screen));
    return !is_playlist_menu
        && native_playlist_screen_contains_song(screen, song);
}

static void
begin_formats(PlaylistFormatFixture *fixture) {
    NcmError error;

    fixture->old_classic = Config.song_list_format;
    fixture->old_columns = Config.song_columns_mode_format;
    fixture->old_mode = Config.playlist_display_mode;
    fixture->old_regex_type = Config.regex_type;

    ncm_format_ast_init(&Config.song_list_format);
    ncm_format_ast_init(&Config.song_columns_mode_format);
    ncm_error_clear(&error);
    assert(ncm_format_parse(&Config.song_list_format, LIT_ARGS("%F"),
                            NCM_FORMAT_FLAG_ALL, &error));
    assert(ncm_format_parse(&Config.song_columns_mode_format,
                            LIT_ARGS("%F"), NCM_FORMAT_FLAG_ALL, &error));
    Config.playlist_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;
    return;
}

static void
end_formats(PlaylistFormatFixture *fixture) {
    ncm_format_ast_destroy(&Config.song_list_format);
    ncm_format_ast_destroy(&Config.song_columns_mode_format);
    Config.song_list_format = fixture->old_classic;
    Config.song_columns_mode_format = fixture->old_columns;
    Config.playlist_display_mode = fixture->old_mode;
    Config.regex_type = fixture->old_regex_type;
    return;
}

static void
test_playlist_row_ownership(void) {
    NcSongMenu songs;
    NcmSong song;
    NcmSong *stored;

    nc_song_menu_init(&songs);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("owned-one.flac")));
    nc_song_menu_add(&songs, &song);
    ncm_song_destroy(&song);
    ncm_song_init(&song);

    stored = nc_song_menu_item_at(&songs, NC_MENU_ITEMS_ALL, 0);
    assert(stored != NULL);
    assert_song_uri(stored, LIT_ARGS("owned-one.flac"));

    ncm_song_destroy(&song);
    nc_song_menu_destroy(&songs);
    return;
}

static void
test_playlist_selection_range(void) {
    NcPlaylistScreen screen;
    NcScreenCallbacks callbacks = {0};
    NcSongMenu songs;
    NcmSong song;

    nc_song_menu_init(&songs);
    ncm_song_init(&song);
    nc_playlist_screen_init(&screen, callbacks, NULL,
                            nc_song_menu_base(&songs), 0, 80, 0, 10);

    for (int32 i = 0; i < 4; i += 1) {
        assert(ncm_song_set_uri(&song, LIT_ARGS("range.flac")));
        ncm_song_set_position(&song, (uint32)i);
        nc_song_menu_add(&songs, &song);
    }

    assert(nc_menu_select_range(nc_song_menu_base(&songs), 1, 3, true));
    assert(!nc_menu_position_is_selected(nc_song_menu_base(&songs), 0));
    assert(nc_menu_position_is_selected(nc_song_menu_base(&songs), 1));
    assert(nc_menu_position_is_selected(nc_song_menu_base(&songs), 2));
    assert(nc_menu_position_is_selected(nc_song_menu_base(&songs), 3));

    ncm_song_destroy(&song);
    nc_song_menu_destroy(&songs);
    return;
}

static void
test_native_initialization(void) {
    NativePlaylistScreen screen;
    NcMenu *menu;
    bool old_centered;
    bool old_cyclic;
    bool old_scroll_whole_page;
    uint32 old_lines_scrolled;

    old_centered = Config.centered_cursor;
    old_cyclic = Config.use_cyclic_scrolling;
    old_scroll_whole_page = Config.mouse_list_scroll_whole_page;
    old_lines_scrolled = Config.lines_scrolled;
    Config.centered_cursor = true;
    Config.use_cyclic_scrolling = true;
    Config.mouse_list_scroll_whole_page = true;
    Config.lines_scrolled = 7;

    init_screen(&screen);
    menu = native_playlist_screen_menu(&screen);
    assert(menu == nc_song_menu_base(&screen.songs));
    assert(screen.screen.menu == menu);
    assert(menu->autocenter_cursor);
    assert(menu->cyclic_scroll_enabled);
    assert(menu->display_callbacks.draw != NULL);
    assert(menu->action_callbacks.activate != NULL);
    assert(screen.screen.lines_scrolled == 7);
    assert(screen.screen.mouse_list_scroll_whole_page);
    assert(screen.sync == NULL);
    assert(screen.bridge.active_window == NULL);
    assert(screen.bridge.refresh == NULL);
    assert(screen.bridge.update_song == NULL);

    native_playlist_screen_destroy(&screen);
    Config.centered_cursor = old_centered;
    Config.use_cyclic_scrolling = old_cyclic;
    Config.mouse_list_scroll_whole_page = old_scroll_whole_page;
    Config.lines_scrolled = old_lines_scrolled;
    return;
}

static void
test_bridge_free_load_filter_search_selection(void) {
    NativePlaylistScreen screen;
    PlaylistFormatFixture formats;
    NcmMpdSongList songs;
    NcmSongArray selected;
    NcmSong current;
    NcmSong now_playing;
    NcmError error;
    NcMenu *menu;

    begin_formats(&formats);
    init_screen(&screen);
    menu = native_playlist_screen_menu(&screen);
    ncm_mpd_song_list_init(&songs);
    ncm_song_init(&current);
    ncm_song_init(&now_playing);
    append_song(&songs, LIT_ARGS("keep-one.flac"), 0, 10);
    append_song(&songs, LIT_ARGS("drop.flac"), 1, 11);
    append_song(&songs, LIT_ARGS("keep-two.flac"), 2, 12);

    assert(native_playlist_screen_load_song_list(&screen, &songs));
    assert(native_playlist_screen_song_count(&screen) == 3);
    assert(native_playlist_screen_locate_position(&screen, 2));
    assert(nc_menu_highlight(menu) == 2);
    assert(nc_menu_set_position_selected(menu, 1, true));

    ncm_error_clear(&error);
    assert(native_playlist_screen_apply_filter(
        &screen, LIT_ARGS("keep"), &error));
    assert(nc_menu_is_filtered(menu));
    assert(nc_menu_item_count(menu) == 2);
    assert(!native_playlist_screen_locate_position(&screen, 1));
    assert(native_playlist_screen_locate_position(&screen, 2));
    assert(nc_menu_highlight(menu) == 1);
    assert(native_playlist_screen_current_song(&screen, &current));
    assert_song_uri(&current, LIT_ARGS("keep-two.flac"));
    assert(native_playlist_screen_now_playing_song(
        &screen, 1, &now_playing));
    assert_song_uri(&now_playing, LIT_ARGS("drop.flac"));
    assert(!native_playlist_screen_now_playing_song(
        &screen, 99, &now_playing));

    assert(native_playlist_screen_search(
        &screen, LIT_ARGS("keep-one"), true, true, true, &error));
    assert(nc_menu_highlight(menu) == 0);
    assert(native_playlist_screen_search(
        &screen, LIT_ARGS("keep-two"), false, true, true, &error));
    assert(nc_menu_highlight(menu) == 1);

    native_playlist_screen_clear_filter(&screen);
    assert(!nc_menu_is_filtered(menu));
    assert(nc_menu_item_count(menu) == 3);
    assert(nc_menu_position_is_selected(menu, 1));

    ncm_song_array_init(&selected);
    assert(native_playlist_screen_selected_songs(&screen, &selected));
    assert(selected.len == 1);
    assert_song_uri(&selected.items[0], LIT_ARGS("drop.flac"));
    ncm_song_array_destroy(&selected);

    ncm_song_destroy(&now_playing);
    ncm_song_destroy(&current);
    ncm_mpd_song_list_destroy(&songs);
    native_playlist_screen_destroy(&screen);
    end_formats(&formats);
    return;
}

static void
test_bridge_free_mpd_updates_and_membership(void) {
    NativePlaylistScreen screen;
    NcmMpdClient client = {0};
    NcmError error;
    NcSongMenu display;
    NcSongMenu external;
    NcmSong duplicate;
    NcmSong *stored;
    NcMenu *display_menu;
    NcMenu *external_menu;
    NcMenu *menu;

    ncm_mpd_song_list_clear(&mpd_fixture.queue);
    ncm_mpd_song_list_clear(&mpd_fixture.changes);
    mpd_fixture.queue_calls = 0;
    mpd_fixture.changes_calls = 0;
    append_song(&mpd_fixture.queue, LIT_ARGS("duplicate.flac"), 0, 20);
    append_song(&mpd_fixture.queue, LIT_ARGS("duplicate.flac"), 1, 21);

    init_screen(&screen);
    nc_song_menu_init(&display);
    nc_song_menu_init(&external);
    display_menu = nc_song_menu_base(&display);
    external_menu = nc_song_menu_base(&external);
    native_playlist_screen_set_display_menu(&screen, display_menu);
    menu = native_playlist_screen_menu(&screen);
    ncm_error_clear(&error);
    assert(native_playlist_screen_reload_from_mpd(
        &screen, &client, 0, 2, &error));
    assert(mpd_fixture.queue_calls == 1);
    assert(mpd_fixture.changes_calls == 0);
    assert(native_playlist_screen_song_count(&screen) == 2);

    ncm_song_init(&duplicate);
    assert(ncm_song_set_uri(&duplicate, LIT_ARGS("duplicate.flac")));
    assert(native_playlist_screen_contains_song(&screen, &duplicate));
    assert(!display_should_mark_playlist_song(
        &screen, display_menu, &duplicate));
    assert(display_should_mark_playlist_song(
        &screen, external_menu, &duplicate));
    assert(nc_menu_set_position_selected(menu, 1, true));

    append_song(&mpd_fixture.changes,
                LIT_ARGS("replacement.flac"), 1, 31);
    assert(native_playlist_screen_reload_from_mpd(
        &screen, &client, 1, 2, &error));
    assert(mpd_fixture.changes_calls == 1);
    stored = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, 1);
    assert_song_uri(stored, LIT_ARGS("replacement.flac"));
    assert(nc_menu_position_is_selected(menu, 1));
    assert(native_playlist_screen_contains_song(&screen, &duplicate));

    ncm_mpd_song_list_clear(&mpd_fixture.changes);
    assert(native_playlist_screen_reload_from_mpd(
        &screen, &client, 2, 1, &error));
    assert(native_playlist_screen_song_count(&screen) == 1);
    assert(native_playlist_screen_contains_song(&screen, &duplicate));
    assert(display_should_mark_playlist_song(
        &screen, external_menu, &duplicate));
    assert(native_playlist_screen_reload_from_mpd(
        &screen, &client, 3, 0, &error));
    assert(native_playlist_screen_empty(&screen));
    assert(!native_playlist_screen_contains_song(&screen, &duplicate));
    assert(!display_should_mark_playlist_song(
        &screen, external_menu, &duplicate));

    ncm_song_destroy(&duplicate);
    native_playlist_screen_set_display_menu(&screen, NULL);
    nc_song_menu_destroy(&external);
    nc_song_menu_destroy(&display);
    native_playlist_screen_destroy(&screen);
    return;
}

static void
test_native_storage_is_authoritative(void) {
    NativePlaylistScreen screen;
    NcSongMenu display;
    NcmSong found;
    NcmSong song;

    init_screen(&screen);
    nc_song_menu_init(&display);
    ncm_song_init(&found);
    ncm_song_init(&song);
    native_playlist_screen_set_display_menu(
        &screen, nc_song_menu_base(&display));

    assert(ncm_song_set_uri(&song, LIT_ARGS("storage.flac")));
    ncm_song_set_position(&song, 7);
    assert(native_playlist_screen_add_song_copy(&screen, &song));
    assert(native_playlist_screen_song_count(&screen) == 1);
    assert(nc_menu_all_item_count(nc_song_menu_base(&display)) == 0);
    assert(native_playlist_screen_now_playing_song(&screen, 7, &found));
    assert_song_uri(&found, LIT_ARGS("storage.flac"));

    ncm_song_destroy(&song);
    ncm_song_destroy(&found);
    native_playlist_screen_destroy(&screen);
    nc_song_menu_destroy(&display);
    return;
}

static void
test_locate_updates_active_display_menu(void) {
    NativePlaylistScreen screen;
    NcmMpdSongList songs;
    NcSongMenu display;
    NcMenu *display_menu;
    NcMenu *storage_menu;
    NcmSong *song;

    init_screen(&screen);
    ncm_mpd_song_list_init(&songs);
    nc_song_menu_init(&display);
    append_song(&songs, LIT_ARGS("zero.flac"), 0, 40);
    append_song(&songs, LIT_ARGS("one.flac"), 1, 41);
    append_song(&songs, LIT_ARGS("two.flac"), 2, 42);
    assert(native_playlist_screen_load_song_list(&screen, &songs));

    storage_menu = native_playlist_screen_menu(&screen);
    display_menu = nc_song_menu_base(&display);
    for (int64 i = 0; i < nc_menu_all_item_count(storage_menu); i += 1) {
        song = nc_menu_item_at(storage_menu, NC_MENU_ITEMS_ALL, i);
        nc_song_menu_add(&display, song);
    }
    native_playlist_screen_set_display_menu(&screen, display_menu);

    nc_menu_clear_filtered_items(storage_menu);
    song = nc_menu_item_at(storage_menu, NC_MENU_ITEMS_ALL, 0);
    nc_menu_add_filtered_item_ref(storage_menu, song);
    song = nc_menu_item_at(storage_menu, NC_MENU_ITEMS_ALL, 2);
    nc_menu_add_filtered_item_ref(storage_menu, song);
    nc_menu_show_filtered_items(storage_menu);

    nc_menu_clear_filtered_items(display_menu);
    song = nc_menu_item_at(display_menu, NC_MENU_ITEMS_ALL, 0);
    nc_menu_add_filtered_item_ref(display_menu, song);
    song = nc_menu_item_at(display_menu, NC_MENU_ITEMS_ALL, 2);
    nc_menu_add_filtered_item_ref(display_menu, song);
    nc_menu_show_filtered_items(display_menu);

    assert(native_playlist_screen_locate_position(&screen, 2));
    assert(nc_menu_highlight(storage_menu) == 1);
    assert(nc_menu_highlight(display_menu) == 1);

    native_playlist_screen_set_display_menu(&screen, NULL);
    nc_song_menu_destroy(&display);
    ncm_mpd_song_list_destroy(&songs);
    native_playlist_screen_destroy(&screen);
    return;
}

static void
test_native_display_and_column_title(void) {
    NativePlaylistScreen screen;
    PlaylistFormatFixture formats;
    NcmError error;
    ColumnArray old_columns;
    Column columns[2];
    NcMenu *menu;
    NcmSong *song;
    bool old_titles_visibility;

    begin_formats(&formats);
    ncm_format_ast_destroy(&Config.song_list_format);
    ncm_format_ast_destroy(&Config.song_columns_mode_format);
    ncm_format_ast_init(&Config.song_list_format);
    ncm_format_ast_init(&Config.song_columns_mode_format);
    ncm_error_clear(&error);
    assert(ncm_format_parse(&Config.song_list_format,
                            LIT_ARGS("classic:%F"),
                            NCM_FORMAT_FLAG_ALL, &error));
    assert(ncm_format_parse(&Config.song_columns_mode_format,
                            LIT_ARGS("columns:%F"),
                            NCM_FORMAT_FLAG_ALL, &error));

    old_columns = Config.columns;
    old_titles_visibility = Config.titles_visibility;
    columns[0] = (Column){0};
    columns[0].name = (char *)"Artist";
    columns[0].name_len = STRLIT_LEN("Artist");
    columns[0].type = (char *)"a";
    columns[0].type_len = STRLIT_LEN("a");
    columns[0].width = 10;
    columns[0].stretch_limit = -1;
    columns[0].fixed = true;
    columns[1] = (Column){0};
    columns[1].type = (char *)"t";
    columns[1].type_len = STRLIT_LEN("t");
    columns[1].width = 10;
    columns[1].stretch_limit = -1;
    columns[1].fixed = true;
    Config.columns.items = columns;
    Config.columns.len = 2;
    Config.columns.cap = 2;
    Config.titles_visibility = true;
    Config.playlist_display_mode = NCM_DISPLAY_MODE_COLUMNS;

    reset_window_trace();
    native_playlist_screen_init(&screen, 0, 20, 0, 24,
                                nc_color_default(), nc_border_none());
    assert(ncm_string_equal(screen.column_title.data,
                            screen.column_title.len,
                            LIT_ARGS("Artist    Title     ")));
    assert(screen.window.start_y == 2);
    assert(screen.window.height == 22);

    add_song(&screen, LIT_ARGS("song.flac"), 0, 40);
    menu = native_playlist_screen_menu(&screen);
    song = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, 0);
    reset_window_trace();
    menu->display_callbacks.draw(menu, &screen.window, song, 0,
                                 menu->display_callbacks.user);
    assert(ncm_string_equal(window_trace.printed,
                            window_trace.printed_len,
                            LIT_ARGS("columns:song.flac")));

    Config.playlist_display_mode = NCM_DISPLAY_MODE_CLASSIC;
    native_playlist_screen_update_column_title(&screen);
    assert(screen.column_title.len == 0);
    assert(screen.window.start_y == 0);
    assert(screen.window.height == 24);
    reset_window_trace();
    menu->display_callbacks.draw(menu, &screen.window, song, 0,
                                 menu->display_callbacks.user);
    assert(ncm_string_equal(window_trace.printed,
                            window_trace.printed_len,
                            LIT_ARGS("classic:song.flac")));

    Config.playlist_display_mode = NCM_DISPLAY_MODE_COLUMNS;
    native_playlist_screen_set_geometry(&screen, 0, 20, 0, 24);
    assert(screen.column_title.len > 0);
    assert(screen.window.start_y == 2);
    assert(screen.window.height == 22);

    native_playlist_screen_destroy(&screen);
    Config.columns = old_columns;
    Config.titles_visibility = old_titles_visibility;
    end_formats(&formats);
    return;
}

static void
test_native_highlight_timeout(void) {
    NativePlaylistScreen screen;
    uint32 old_delay;
    NcmTimePoint old_timer;

    old_delay = Config.playlist_disable_highlight_delay_seconds;
    old_timer = global_timer;
    Config.playlist_disable_highlight_delay_seconds = 2;
    global_timer.ns = 1000000000;
    reset_window_trace();
    init_screen(&screen);

    native_playlist_screen_request_highlighting(&screen);
    assert(native_playlist_screen_highlighting(&screen));
    assert(screen.highlight_timer.ns == global_timer.ns);

    global_timer.ns = 3000000000;
    nc_screen_update(native_playlist_screen_base(&screen));
    assert(native_playlist_screen_highlighting(&screen));
    assert(window_trace.display_calls == 0);

    global_timer.ns = 4000000001;
    nc_screen_update(native_playlist_screen_base(&screen));
    assert(!native_playlist_screen_highlighting(&screen));
    assert(window_trace.menu_refresh_calls == 1);
    assert(window_trace.display_calls == 1);

    native_playlist_screen_destroy(&screen);
    global_timer = old_timer;
    Config.playlist_disable_highlight_delay_seconds = old_delay;
    return;
}

static void
test_native_activation_and_mouse(void) {
    NativePlaylistScreen screen;
    MEVENT event = {0};

    reset_window_trace();
    init_screen(&screen);
    add_song(&screen, LIT_ARGS("play.flac"), 0, 77);

    assert(nc_playlist_screen_activate_position(&screen.screen, 0));
    assert(window_trace.play_calls == 1);
    assert(window_trace.played_id == 77);

    event.x = 0;
    event.y = 0;
    event.bstate = BUTTON3_PRESSED;
    nc_playlist_screen_mouse_button_pressed(&screen.screen, event);
    assert(window_trace.play_calls == 2);
    assert(window_trace.played_id == 77);

    native_playlist_screen_destroy(&screen);
    return;
}

static void
test_playlist_updates_current_mutable_song(void) {
    NativePlaylistScreen screen;
    NcmMutableSong edited;
    NcmSong result;
    NcmSong song;
    NcmStringView view;

    init_screen(&screen);
    ncm_mutable_song_init(&edited);
    ncm_song_init(&result);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("/music/song.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE,
                            LIT_ARGS("Old title")));
    assert(ncm_song_add_tag(&song, MPD_TAG_NAME,
                            LIT_ARGS("Preserved name")));
    ncm_song_set_duration(&song, 321);
    ncm_song_set_position(&song, 7);
    ncm_song_set_id(&song, 19);
    ncm_song_set_priority(&song, 4);
    ncm_song_set_mtime(&song, 1234);
    assert(native_playlist_screen_add_song_copy(&screen, &song));
    assert(ncm_mutable_song_load_originals_from_song(&edited, &song));
    assert(ncm_mutable_song_set_tag(
        &edited, NCM_TAGS_FIELD_TITLE, 0, LIT_ARGS("New title")));
    assert(ncm_mutable_song_set_new_name(
        &edited, LIT_ARGS("renamed.flac")));

    assert(native_playlist_screen_update_current_mutable_song(
        &screen, &edited));
    assert(native_playlist_screen_current_song(&screen, &result));
    assert_song_uri(&result, LIT_ARGS("/music/renamed.flac"));
    assert(ncm_song_tag_view(&result, MPD_TAG_TITLE, 0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("New title")));
    assert(ncm_song_tag_view(&result, MPD_TAG_NAME, 0, &view));
    assert(ncm_string_equal(view.data, view.len,
                            LIT_ARGS("Preserved name")));
    assert(ncm_song_duration(&result) == 321);
    assert(ncm_song_position(&result) == 7);
    assert(ncm_song_id(&result) == 19);
    assert(ncm_song_priority(&result) == 4);
    assert(ncm_song_mtime(&result) == 1234);

    ncm_song_destroy(&song);
    ncm_song_destroy(&result);
    ncm_mutable_song_destroy(&edited);
    native_playlist_screen_destroy(&screen);
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

    window_trace.title[0] = '\0';
    window_trace.title_len = title_len;
    if ((title != NULL) && (title_len > 0)) {
        assert(title_len < (int32)sizeof(window_trace.title));
        for (int32 i = 0; i < title_len; i += 1) {
            window_trace.title[i] = title[i];
        }
        window_trace.title[title_len] = '\0';
        window->title = title;
    } else {
        window->title = NULL;
    }
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

void
__wrap_nc_window_print_char(NcWindow *window, char ch) {
    (void)window;
    assert(window_trace.printed_len
           < (int32)sizeof(window_trace.printed)-1);
    window_trace.printed[window_trace.printed_len] = ch;
    window_trace.printed_len += 1;
    window_trace.printed[window_trace.printed_len] = '\0';
    return;
}

void
__wrap_nc_buffer_apply_property(NcWindow *window,
                                NcBufferProperty *property) {
    (void)window;
    (void)property;
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
__wrap_ncm_mpd_client_get_queue(NcmMpdClient *client,
                                NcmMpdSongList *songs,
                                NcmError *error) {
    (void)client;
    mpd_fixture.queue_calls += 1;
    ncm_error_clear(error);
    return ncm_mpd_song_list_copy(songs, &mpd_fixture.queue);
}

bool
__wrap_ncm_mpd_client_get_queue_changes(NcmMpdClient *client,
                                        uint32 version,
                                        NcmMpdSongList *songs,
                                        NcmError *error) {
    (void)client;
    (void)version;
    mpd_fixture.changes_calls += 1;
    ncm_error_clear(error);
    return ncm_mpd_song_list_copy(songs, &mpd_fixture.changes);
}

bool
__wrap_ncm_mpd_client_play_id(NcmMpdClient *client, int32 id,
                              NcmError *error) {
    (void)client;
    window_trace.play_calls += 1;
    window_trace.played_id = id;
    ncm_error_clear(error);
    return true;
}
