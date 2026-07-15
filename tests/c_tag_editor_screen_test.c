#include <assert.h>
#include <stdlib.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/nc_tag_editor.h"
#include "settings.h"
#include "ui_state.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct TagEditorWindowTrace {
    int32 display_calls;
    int32 refresh_calls;
} TagEditorWindowTrace;

typedef struct TagEditorBridgeTrace {
    NcWindow active_window;
    char *title;
    MEVENT mouse_event;
    enum NcScroll scroll_where;

    int32 active_window_calls;
    int32 refresh_calls;
    int32 refresh_window_calls;
    int32 scroll_calls;
    int32 action_runnable_calls;
    int32 run_action_calls;
    int32 switch_to_calls;
    int32 resize_calls;
    int32 title_calls;
    int32 update_calls;
    int32 clear_directories_calls;
    int32 mouse_calls;

    bool action_runnable_result;
    bool run_action_result;
} TagEditorBridgeTrace;

static TagEditorWindowTrace window_trace;
static TagEditorBridgeTrace bridge_trace;

static void reset_window_trace(void);
static void reset_bridge_trace(void);
static void init_screen(NativeTagEditorScreen *screen);
static void destroy_screen(NativeTagEditorScreen *screen);
static NativeTagEditorBridge bridge_callbacks(void);
static void append_song(NativeTagEditorScreen *screen, char *uri,
                        int32 uri_len, char *artist, int32 artist_len);
static void append_directory(NcmDirectoryArray *directories, char *path,
                             int32 path_len);
static void assert_menu_string_pair(NcMenuStringPair *pair,
                                    char *first, int32 first_len,
                                    char *second, int32 second_len);
static void assert_song_uri(NcmSong *song, char *uri, int32 uri_len);
static void test_initial_state_and_geometry(void);
static void test_bridge_callback_contract(void);
static void test_bridge_run_action_guard(void);
static void test_directory_filter_and_search_contract(void);
static void test_tag_filter_search_and_selection_contract(void);
static void test_parser_menu_contract(void);
static void test_native_refresh_fallback_contract(void);
static void test_native_state_ownership_without_bridge(void);
static void test_directory_reload_preserves_current_row(void);
static void test_directory_change_clears_stale_tags(void);
static void test_separate_filter_and_search_state(void);

int
main(void) {
    app_controller_init();
    ui_state_set_screen_size(100, 30);
    Config.titles_visibility = true;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;

    test_initial_state_and_geometry();
    test_bridge_callback_contract();
    test_bridge_run_action_guard();
    test_directory_filter_and_search_contract();
    test_tag_filter_search_and_selection_contract();
    test_parser_menu_contract();
    test_native_refresh_fallback_contract();
    test_native_state_ownership_without_bridge();
    test_directory_reload_preserves_current_row();
    test_directory_change_clears_stale_tags();
    test_separate_filter_and_search_state();

    exit(EXIT_SUCCESS);
}

static void
reset_window_trace(void) {
    window_trace = (TagEditorWindowTrace){0};
    return;
}

static void
reset_bridge_trace(void) {
    bridge_trace = (TagEditorBridgeTrace){0};
    bridge_trace.title = (char *)"Legacy tag editor";
    bridge_trace.action_runnable_result = true;
    bridge_trace.run_action_result = true;
    return;
}

static void
init_screen(NativeTagEditorScreen *screen) {
    reset_window_trace();
    native_tag_editor_screen_init(screen, 2, 100, 3, 20,
                                  nc_color_default(), nc_border_none());
    return;
}

static void
destroy_screen(NativeTagEditorScreen *screen) {
    native_tag_editor_screen_destroy(screen);
    return;
}

static void
append_song(NativeTagEditorScreen *screen, char *uri, int32 uri_len,
            char *artist, int32 artist_len) {
    NcmMutableSong song;

    ncm_mutable_song_init(&song);
    assert(ncm_mutable_song_set_uri(&song, uri, uri_len));
    assert(ncm_mutable_song_set_name(&song, uri, uri_len));
    assert(ncm_mutable_song_set_original_tag(
               &song, NCM_TAGS_FIELD_ARTIST, 0, artist, artist_len));
    assert(native_tag_editor_screen_add_mutable_song(screen, &song));
    ncm_mutable_song_destroy(&song);
    return;
}

static void
append_directory(NcmDirectoryArray *directories, char *path,
                 int32 path_len) {
    NcmDirectory directory;

    ncm_directory_init(&directory);
    assert(ncm_directory_set(&directory, path, path_len, 0));
    assert(ncm_directory_array_append_copy(directories, &directory));
    ncm_directory_destroy(&directory);
    return;
}

static void
assert_menu_string_pair(NcMenuStringPair *pair, char *first,
                        int32 first_len, char *second,
                        int32 second_len) {
    assert(pair != NULL);
    assert(ncm_string_equal(pair->first, pair->first_len,
                            first, first_len));
    assert(ncm_string_equal(pair->second, pair->second_len,
                            second, second_len));
    return;
}

static void
assert_song_uri(NcmSong *song, char *uri, int32 uri_len) {
    NcmStringView view;

    assert(ncm_song_uri_view(song, 0, &view));
    assert(ncm_string_equal(view.data, view.len, uri, uri_len));
    return;
}

static NcWindow *
bridge_active_window(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->active_window_calls += 1;
    return &trace->active_window;
}

static void
bridge_refresh(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->refresh_calls += 1;
    return;
}

static void
bridge_refresh_window(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->refresh_window_calls += 1;
    return;
}

static void
bridge_scroll(void *user, enum NcScroll where) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->scroll_calls += 1;
    trace->scroll_where = where;
    return;
}

static bool
bridge_action_runnable(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->action_runnable_calls += 1;
    return trace->action_runnable_result;
}

static bool
bridge_run_action(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->run_action_calls += 1;
    return trace->run_action_result;
}

static void
bridge_switch_to(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->switch_to_calls += 1;
    return;
}

static void
bridge_resize(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->resize_calls += 1;
    return;
}

static char *
bridge_title(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->title_calls += 1;
    return trace->title;
}

static void
bridge_update(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->update_calls += 1;
    return;
}

static void
bridge_clear_directories(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->clear_directories_calls += 1;
    return;
}

static void
bridge_mouse_button_pressed(void *user, MEVENT event) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->mouse_calls += 1;
    trace->mouse_event = event;
    return;
}

static NativeTagEditorBridge
bridge_callbacks(void) {
    NativeTagEditorBridge bridge;

    bridge = (NativeTagEditorBridge){0};
    bridge.active_window = bridge_active_window;
    bridge.refresh = bridge_refresh;
    bridge.refresh_window = bridge_refresh_window;
    bridge.scroll = bridge_scroll;
    bridge.action_runnable = bridge_action_runnable;
    bridge.run_action = bridge_run_action;
    bridge.switch_to = bridge_switch_to;
    bridge.resize = bridge_resize;
    bridge.title = bridge_title;
    bridge.update = bridge_update;
    bridge.clear_directories = bridge_clear_directories;
    bridge.mouse_button_pressed = bridge_mouse_button_pressed;
    bridge.user = &bridge_trace;
    return bridge;
}

static void
test_initial_state_and_geometry(void) {
    NativeTagEditorScreen screen;
    NcmStringView current_dir;

    init_screen(&screen);

    assert(nc_screen_type(native_tag_editor_screen_base(&screen))
           == NC_SCREEN_TYPE_TAG_EDITOR);
    assert(native_tag_editor_screen_active_menu(&screen)
           == nc_editor_pair_menu_base(&screen.directories));
    assert(native_tag_editor_screen_active_window(&screen)
           == &screen.directories_window);
    assert(screen.active_column == NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES);
    assert(native_tag_editor_screen_current_dir(&screen, &current_dir));
    assert(ncm_string_equal(current_dir.data, current_dir.len,
                            STRLIT_ARGS("/")));

    assert(screen.left_width == 37);
    assert(screen.middle_start_x == 40);
    assert(screen.middle_width == 26);
    assert(screen.right_start_x == 67);
    assert(screen.right_width == 35);

    assert(nc_screen_is_lockable(native_tag_editor_screen_base(&screen)));
    assert(nc_screen_is_mergable(native_tag_editor_screen_base(&screen)));
    assert(ncm_string_equal(nc_screen_title(
                                native_tag_editor_screen_base(&screen)),
                            STRLIT_LEN("Tag editor"),
                            STRLIT_ARGS("Tag editor")));

    assert(nc_menu_item_count(nc_editor_string_menu_base(
               native_tag_editor_screen_parser_rows(&screen))) == 3);

    destroy_screen(&screen);
    return;
}

static void
test_bridge_callback_contract(void) {
    NativeTagEditorScreen screen;
    NcScreen *base;
    MEVENT event;

    init_screen(&screen);
    reset_bridge_trace();
    native_tag_editor_screen_set_bridge(&screen, bridge_callbacks());
    base = native_tag_editor_screen_base(&screen);

    assert(nc_screen_active_window(base) == &bridge_trace.active_window);
    assert(bridge_trace.active_window_calls == 1);

    nc_screen_refresh(base);
    assert(bridge_trace.refresh_calls == 1);
    assert(window_trace.display_calls == 0);

    nc_screen_refresh_window(base);
    assert(bridge_trace.refresh_window_calls == 1);

    nc_screen_scroll(base, NC_SCROLL_PAGE_DOWN);
    assert(bridge_trace.scroll_calls == 1);
    assert(bridge_trace.scroll_where == NC_SCROLL_PAGE_DOWN);

    assert(nc_screen_can_run_current(base));
    assert(bridge_trace.action_runnable_calls == 1);
    assert(nc_screen_run_current(base));
    assert(bridge_trace.action_runnable_calls == 3);
    assert(bridge_trace.run_action_calls == 1);

    nc_screen_switch_to(base);
    assert(bridge_trace.switch_to_calls == 1);

    nc_screen_request_resize(base);
    assert(nc_screen_has_to_be_resized(base));
    nc_screen_resize(base);
    assert(bridge_trace.resize_calls == 1);
    assert(!nc_screen_has_to_be_resized(base));

    assert(ncm_string_equal(nc_screen_title(base),
                            STRLIT_LEN("Legacy tag editor"),
                            STRLIT_ARGS("Legacy tag editor")));
    assert(bridge_trace.title_calls == 1);

    nc_screen_request_update(base);
    assert(nc_screen_has_to_be_updated(base));
    nc_screen_update(base);
    assert(bridge_trace.update_calls == 1);
    assert(!nc_screen_has_to_be_updated(base));

    native_tag_editor_screen_clear_directories(&screen);
    assert(bridge_trace.clear_directories_calls == 1);

    event = (MEVENT){0};
    event.x = 11;
    event.y = 12;
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(base, event);
    assert(bridge_trace.mouse_calls == 1);
    assert(bridge_trace.mouse_event.x == 11);
    assert(bridge_trace.mouse_event.y == 12);
    assert(bridge_trace.mouse_event.bstate == BUTTON3_PRESSED);

    destroy_screen(&screen);
    return;
}

static void
test_bridge_run_action_guard(void) {
    NativeTagEditorScreen screen;
    NcScreen *base;

    init_screen(&screen);
    reset_bridge_trace();
    bridge_trace.action_runnable_result = false;
    native_tag_editor_screen_set_bridge(&screen, bridge_callbacks());
    base = native_tag_editor_screen_base(&screen);

    assert(!nc_screen_can_run_current(base));
    assert(!nc_screen_run_current(base));
    assert(bridge_trace.action_runnable_calls == 2);
    assert(bridge_trace.run_action_calls == 0);

    destroy_screen(&screen);
    return;
}

static void
test_directory_filter_and_search_contract(void) {
    NativeTagEditorScreen screen;
    NcMenu *menu;
    NcMenuStringPair *pair;
    NcmError error;

    init_screen(&screen);
    ncm_error_clear(&error);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("."), STRLIT_ARGS("/")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS(".."), STRLIT_ARGS("/parent")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Beta"), STRLIT_ARGS("/Beta")));

    menu = nc_editor_pair_menu_base(
        native_tag_editor_screen_directories(&screen));
    assert(nc_menu_item_count(menu) == 4);
    assert(native_tag_editor_screen_apply_directory_filter(
               &screen, STRLIT_ARGS("Alpha"), Config.regex_type, &error));
    assert(nc_menu_item_count(menu) == 1);
    pair = nc_menu_active_item_at(menu, 0);
    assert_menu_string_pair(pair, STRLIT_ARGS("Alpha"),
                            STRLIT_ARGS("/Alpha"));

    native_tag_editor_screen_clear_filters(&screen);
    assert(nc_menu_item_count(menu) == 4);
    assert(screen.directory_filter_constraint.len == 0);
    assert(screen.directory_search_constraint.len == 0);

    assert(native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Beta"), true, true, false, &error));
    assert(nc_menu_highlight(menu) == 3);

    destroy_screen(&screen);
    return;
}

static void
test_tag_filter_search_and_selection_contract(void) {
    NativeTagEditorScreen screen;
    NcMenu *menu;
    NcmSongArray songs;
    NcmError error;

    init_screen(&screen);
    ncm_song_array_init(&songs);
    ncm_error_clear(&error);
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    append_song(&screen, STRLIT_ARGS("two.flac"), STRLIT_ARGS("Beta"));
    native_tag_editor_screen_next_column(&screen);
    native_tag_editor_screen_next_column(&screen);

    menu = nc_tag_row_menu_base(native_tag_editor_screen_tags(&screen));
    assert(native_tag_editor_screen_active_menu(&screen) == menu);
    assert(native_tag_editor_screen_apply_tag_filter(
               &screen, STRLIT_ARGS("Alpha"), Config.regex_type, &error));
    assert(nc_menu_item_count(menu) == 1);
    assert(native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert_song_uri(&songs.items[0], STRLIT_ARGS("one.flac"));

    native_tag_editor_screen_clear_filters(&screen);
    ncm_song_array_clear(&songs);
    assert(nc_menu_goto_selectable(menu, 0));
    assert(native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Beta"), true, true, true, &error));
    assert(nc_menu_highlight(menu) == 1);
    assert(nc_menu_set_position_selected(menu, 1, true));
    assert(native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert_song_uri(&songs.items[0], STRLIT_ARGS("two.flac"));

    ncm_song_array_destroy(&songs);
    destroy_screen(&screen);
    return;
}

static void
test_parser_menu_contract(void) {
    NativeTagEditorScreen screen;
    NcMenu *menu;
    NcMenuString *row;

    init_screen(&screen);
    assert(native_tag_editor_screen_prepare_parser_rows(
               &screen, NATIVE_TAG_EDITOR_PARSER_TAGS_FROM_FILENAME,
               STRLIT_ARGS("%a - %t")));
    menu = nc_editor_string_menu_base(
        native_tag_editor_screen_parser_rows(&screen));
    assert(nc_menu_item_count(menu) == 8);

    row = nc_menu_active_item_at(menu, 0);
    assert(ncm_string_equal(row->data, row->len,
                            STRLIT_ARGS("Get tags from filename")));
    row = nc_menu_active_item_at(menu, 1);
    assert(ncm_string_equal(row->data, row->len,
                            STRLIT_ARGS("Rename files")));
    row = nc_menu_active_item_at(menu, 2);
    assert(ncm_string_equal(row->data, row->len, STRLIT_ARGS("Cancel")));
    row = nc_menu_active_item_at(menu, 3);
    assert(ncm_string_equal(row->data, row->len, STRLIT_ARGS("Pattern")));
    row = nc_menu_active_item_at(menu, 4);
    assert(ncm_string_equal(row->data, row->len, STRLIT_ARGS("Preview")));
    row = nc_menu_active_item_at(menu, 5);
    assert(ncm_string_equal(row->data, row->len, STRLIT_ARGS("Legend")));
    row = nc_menu_active_item_at(menu, 6);
    assert(ncm_string_equal(row->data, row->len, STRLIT_ARGS("Proceed")));
    row = nc_menu_active_item_at(menu, 7);
    assert(ncm_string_equal(row->data, row->len, STRLIT_ARGS("Cancel")));

    destroy_screen(&screen);
    return;
}

static void
test_native_refresh_fallback_contract(void) {
    NativeTagEditorScreen screen;

    init_screen(&screen);
    reset_window_trace();
    nc_screen_refresh(native_tag_editor_screen_base(&screen));
    assert(window_trace.display_calls == 3);
    assert(window_trace.refresh_calls == 1);

    destroy_screen(&screen);
    return;
}

static void
test_native_state_ownership_without_bridge(void) {
    NativeTagEditorScreen screen;
    NcmStringView current_dir;

    init_screen(&screen);

    assert(screen.bridge.user == NULL);
    assert(native_tag_editor_screen_current_dir(&screen, &current_dir));
    assert(ncm_string_equal(current_dir.data, current_dir.len,
                            STRLIT_ARGS("/")));
    assert(ncm_string_equal(screen.directories_title.data,
                            screen.directories_title.len,
                            STRLIT_ARGS("Directories")));
    assert(ncm_string_equal(screen.tag_types_title.data,
                            screen.tag_types_title.len,
                            STRLIT_ARGS("Tag types")));
    assert(ncm_string_equal(screen.tags_title.data, screen.tags_title.len,
                            STRLIT_ARGS("Tags")));
    assert(ncm_string_equal(screen.parser_title.data,
                            screen.parser_title.len,
                            STRLIT_ARGS("Pattern")));
    assert(ncm_string_equal(screen.parser_helper_title.data,
                            screen.parser_helper_title.len,
                            STRLIT_ARGS("Preview")));
    assert(screen.last_known_directory_count == 0);
    assert(screen.last_known_tag_count == 0);
    assert(!screen.directory_filter_enabled);
    assert(!screen.tag_filter_enabled);
    assert(!screen.directory_search_enabled);
    assert(!screen.tag_search_enabled);
    assert(!screen.displayed_dir_valid);
    assert(!screen.observed_dir_valid);

    destroy_screen(&screen);
    return;
}

static void
test_directory_reload_preserves_current_row(void) {
    NativeTagEditorScreen screen;
    NcmDirectoryArray directories;
    NcMenu *menu;
    NcMenuStringPair *pair;

    init_screen(&screen);
    ncm_directory_array_init(&directories);
    append_directory(&directories, STRLIT_ARGS("/Alpha"));
    append_directory(&directories, STRLIT_ARGS("/Beta"));
    append_directory(&directories, STRLIT_ARGS("/Gamma"));
    assert(native_tag_editor_screen_load_directories(&screen,
                                                     &directories));

    menu = nc_editor_pair_menu_base(&screen.directories);
    assert(nc_menu_goto_selectable(menu, 1));
    pair = nc_menu_active_item_at(menu, 1);
    assert_menu_string_pair(pair, STRLIT_ARGS("Beta"),
                            STRLIT_ARGS("/Beta"));

    ncm_directory_array_clear(&directories);
    append_directory(&directories, STRLIT_ARGS("/Gamma"));
    append_directory(&directories, STRLIT_ARGS("/Beta"));
    append_directory(&directories, STRLIT_ARGS("/Alpha"));
    assert(native_tag_editor_screen_load_directories(&screen,
                                                     &directories));
    assert(nc_menu_highlight(menu) == 1);
    pair = nc_menu_active_item_at(menu, 1);
    assert_menu_string_pair(pair, STRLIT_ARGS("Beta"),
                            STRLIT_ARGS("/Beta"));
    assert(screen.last_known_directory_count == 3);
    assert(!screen.directories_update_requested);

    ncm_directory_array_destroy(&directories);
    destroy_screen(&screen);
    return;
}

static void
test_directory_change_clears_stale_tags(void) {
    NativeTagEditorScreen screen;
    NcmSongArray songs;
    NcmSong song;
    NcMenu *directory_menu;
    NcMenu *tag_menu;
    NcmStringView displayed;
    NcmStringView observed;

    init_screen(&screen);
    ncm_song_array_init(&songs);
    ncm_song_init(&song);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Beta"), STRLIT_ARGS("/Beta")));
    assert(ncm_song_set_uri(&song, STRLIT_ARGS("alpha.flac")));
    assert(ncm_song_array_append_copy(&songs, &song));
    assert(native_tag_editor_screen_load_songs(&screen, &songs));

    tag_menu = nc_tag_row_menu_base(&screen.tags);
    assert(nc_menu_item_count(tag_menu) == 1);
    assert(native_tag_editor_screen_displayed_dir(&screen, &displayed));
    assert(ncm_string_equal(displayed.data, displayed.len,
                            STRLIT_ARGS("/Alpha")));

    directory_menu = nc_editor_pair_menu_base(&screen.directories);
    assert(nc_menu_goto_selectable(directory_menu, 1));
    native_tag_editor_screen_finish_directory_change(&screen);
    assert(nc_menu_item_count(tag_menu) == 0);
    assert(!native_tag_editor_screen_displayed_dir(&screen, &displayed));
    assert(native_tag_editor_screen_observed_dir(&screen, &observed));
    assert(ncm_string_equal(observed.data, observed.len,
                            STRLIT_ARGS("/Beta")));
    assert(screen.tags_update_requested);
    assert(screen.last_known_tag_count == 0);

    ncm_song_destroy(&song);
    ncm_song_array_destroy(&songs);
    destroy_screen(&screen);
    return;
}

static void
test_separate_filter_and_search_state(void) {
    NativeTagEditorScreen screen;
    NcmError error;

    init_screen(&screen);
    ncm_error_clear(&error);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Beta"), STRLIT_ARGS("/Beta")));
    assert(native_tag_editor_screen_apply_directory_filter(
               &screen, STRLIT_ARGS("Alpha"), Config.regex_type, &error));
    assert(screen.directory_filter_enabled);
    assert(ncm_string_equal(screen.directory_filter_constraint.data,
                            screen.directory_filter_constraint.len,
                            STRLIT_ARGS("Alpha")));
    assert(screen.directory_search_constraint.len == 0);

    native_tag_editor_screen_clear_filters(&screen);
    assert(native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Beta"), true, true, false, &error));
    assert(screen.directory_search_enabled);
    assert(ncm_string_equal(screen.directory_search_constraint.data,
                            screen.directory_search_constraint.len,
                            STRLIT_ARGS("Beta")));
    assert(screen.directory_filter_constraint.len == 0);

    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    append_song(&screen, STRLIT_ARGS("two.flac"), STRLIT_ARGS("Beta"));
    native_tag_editor_screen_next_column(&screen);
    native_tag_editor_screen_next_column(&screen);
    assert(native_tag_editor_screen_apply_tag_filter(
               &screen, STRLIT_ARGS("Alpha"), Config.regex_type, &error));
    assert(ncm_string_equal(screen.tag_filter_constraint.data,
                            screen.tag_filter_constraint.len,
                            STRLIT_ARGS("Alpha")));
    native_tag_editor_screen_clear_filters(&screen);
    assert(native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Beta"), true, true, false, &error));
    assert(screen.tag_search_enabled);
    assert(ncm_string_equal(screen.tag_search_constraint.data,
                            screen.tag_search_constraint.len,
                            STRLIT_ARGS("Beta")));
    assert(ncm_string_equal(screen.directory_search_constraint.data,
                            screen.directory_search_constraint.len,
                            STRLIT_ARGS("Beta")));

    destroy_screen(&screen);
    return;
}

void
__wrap_nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
                      int64 width, int64 height, char *title,
                      int32 title_len, NcColor color,
                      NcBorder border) {
    (void)color;
    *window = (NcWindow){0};
    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
    window->title = title;
    window->title_len = title_len;
    window->border = border;
    return;
}

void
__wrap_nc_window_destroy(NcWindow *window) {
    *window = (NcWindow){0};
    return;
}

void
__wrap_nc_window_move_to(NcWindow *window, int64 new_x, int64 new_y) {
    window->start_x = new_x;
    window->start_y = new_y;
    return;
}

void
__wrap_nc_window_resize(NcWindow *window, int64 width, int64 height) {
    window->width = width;
    window->height = height;
    return;
}

void
__wrap_nc_window_set_title(NcWindow *window, char *title,
                           int32 title_len) {
    window->title = title;
    window->title_len = title_len;
    return;
}

void
__wrap_nc_window_display(NcWindow *window) {
    (void)window;
    window_trace.display_calls += 1;
    return;
}

void
__wrap_nc_menu_refresh(NcMenu *menu, NcWindow *window, int64 width,
                       int64 height) {
    (void)menu;
    (void)window;
    (void)width;
    (void)height;
    window_trace.refresh_calls += 1;
    return;
}
