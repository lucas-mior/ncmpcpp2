#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/nc_tag_editor.h"
#include "settings.h"
#include "ui_state.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct TagEditorWindowTrace {
    char printed[4096];
    int64 separator_x[4];

    int32 printed_len;
    int32 display_calls;
    int32 refresh_calls;
    int32 separator_calls;
    int32 print_data_calls;
    int32 print_char_calls;
    int32 property_calls;
} TagEditorWindowTrace;

typedef struct TagEditorMpdTrace {
    NcmDirectoryArray directories;
    NcmMpdSongList songs;
    char directory_path[256];
    char songs_path[256];
    char status_message[256];

    int32 directory_path_len;
    int32 songs_path_len;
    int32 status_message_len;
    int32 get_directory_list_calls;
    int32 get_songs_calls;
    int32 status_calls;

    bool get_directory_list_result;
    bool get_songs_result;
} TagEditorMpdTrace;

typedef struct TagEditorBridgeTrace {
    NcWindow active_window;
    char *title;
    MEVENT mouse_event;
    enum NcScroll scroll_where;

    int32 active_window_calls;
    int32 sync_calls;
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
static TagEditorMpdTrace mpd_trace;

void __wrap_nc_window_print_char(NcWindow *window, char ch);

static void reset_window_trace(void);
static void reset_bridge_trace(void);
static void reset_mpd_trace(void);
static void destroy_mpd_trace(void);
static void init_screen(NativeTagEditorScreen *screen);
static void destroy_screen(NativeTagEditorScreen *screen);
static NativeTagEditorBridge bridge_callbacks(void);
static void append_song(NativeTagEditorScreen *screen, char *uri,
                        int32 uri_len, char *artist, int32 artist_len);
static void append_song_with_title(NativeTagEditorScreen *screen, char *uri,
                                   int32 uri_len, char *artist,
                                   int32 artist_len, char *title,
                                   int32 title_len);
static void append_directory(NcmDirectoryArray *directories, char *path,
                             int32 path_len);
static void append_mpd_song(NcmMpdSongList *songs, char *uri,
                            int32 uri_len);
static void assert_menu_string_pair(NcMenuStringPair *pair,
                                    char *first, int32 first_len,
                                    char *second, int32 second_len);
static void assert_song_uri(NcmSong *song, char *uri, int32 uri_len);
static void set_test_buffer(NcBuffer *buffer, char *data, int32 data_len);
static void assert_tag_editor_menu_config(NcMenu *menu,
                                          NcBuffer *highlight_prefix,
                                          NcBuffer *highlight_suffix);
static void assert_printed_equals(char *expected, int32 expected_len);
static void assert_printed_contains(char *needle, int32 needle_len);
static void set_rendering_config(NcBuffer *old_modified_prefix,
                                 char **old_empty_tag,
                                 int32 *old_empty_tag_len,
                                 char **old_tags_separator,
                                 int32 *old_tags_separator_len);
static void restore_rendering_config(NcBuffer *old_modified_prefix,
                                     char *old_empty_tag,
                                     int32 old_empty_tag_len,
                                     char *old_tags_separator,
                                     int32 old_tags_separator_len);
static void test_initial_state_and_geometry(void);
static void test_menu_configuration_and_highlights(void);
static void test_title_visibility_configuration(void);
static void test_native_resize_preserves_state(void);
static void test_small_geometry_bounds(void);
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
static void test_native_directory_and_tag_type_rendering(void);
static void test_native_tag_rendering_for_tag_fields(void);
static void test_native_tag_rendering_for_filename_rows(void);
static void test_native_refresh_does_not_delegate_rendering(void);
static void test_native_update_fetches_directories_and_songs(void);
static void test_native_update_reports_fetch_failures(void);
static void test_directory_filter_keeps_control_rows(void);
static void test_tag_filter_uses_selected_rendered_field(void);
static void test_tag_search_uses_modified_and_filename_rows(void);
static void test_invalid_regex_preserves_previous_constraints(void);
static void test_tag_type_column_is_not_searchable(void);
static void test_tag_editor_selected_songs_use_active_tags_column(void);
static void test_tag_editor_selection_helpers(void);
static void test_tag_editor_active_menu_selection_actions(void);
static void test_tag_editor_enter_directory_invalidates_tags(void);
static void test_tag_editor_mouse_directory_click_enters_and_clears(void);
static void test_tag_editor_mouse_tag_type_and_tag_clicks(void);
static void test_tag_editor_mouse_wheel_directory_change(void);
static void test_tag_editor_tag_type_scroll_refreshes_tags(void);
static void test_tag_editor_search_result_directory_change(void);
static void test_tag_editor_parser_mouse_rows(void);

int
main(void) {
    app_controller_init();
    ui_state_set_screen_size(100, 30);
    Config.titles_visibility = true;
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;

    test_initial_state_and_geometry();
    test_menu_configuration_and_highlights();
    test_title_visibility_configuration();
    test_native_resize_preserves_state();
    test_small_geometry_bounds();
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
    test_native_directory_and_tag_type_rendering();
    test_native_tag_rendering_for_tag_fields();
    test_native_tag_rendering_for_filename_rows();
    test_native_refresh_does_not_delegate_rendering();
    test_native_update_fetches_directories_and_songs();
    test_native_update_reports_fetch_failures();
    test_directory_filter_keeps_control_rows();
    test_tag_filter_uses_selected_rendered_field();
    test_tag_search_uses_modified_and_filename_rows();
    test_invalid_regex_preserves_previous_constraints();
    test_tag_type_column_is_not_searchable();
    test_tag_editor_selected_songs_use_active_tags_column();
    test_tag_editor_selection_helpers();
    test_tag_editor_active_menu_selection_actions();
    test_tag_editor_enter_directory_invalidates_tags();
    test_tag_editor_mouse_directory_click_enters_and_clears();
    test_tag_editor_mouse_tag_type_and_tag_clicks();
    test_tag_editor_mouse_wheel_directory_change();
    test_tag_editor_tag_type_scroll_refreshes_tags();
    test_tag_editor_search_result_directory_change();
    test_tag_editor_parser_mouse_rows();

    destroy_mpd_trace();
    exit(EXIT_SUCCESS);
}

static void
reset_window_trace(void) {
    window_trace = (TagEditorWindowTrace){0};
    return;
}

static void
reset_mpd_trace(void) {
    destroy_mpd_trace();
    ncm_directory_array_init(&mpd_trace.directories);
    ncm_mpd_song_list_init(&mpd_trace.songs);
    mpd_trace.get_directory_list_result = true;
    mpd_trace.get_songs_result = true;
    return;
}

static void
destroy_mpd_trace(void) {
    ncm_directory_array_destroy(&mpd_trace.directories);
    ncm_mpd_song_list_destroy(&mpd_trace.songs);
    mpd_trace = (TagEditorMpdTrace){0};
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
append_song_with_title(NativeTagEditorScreen *screen, char *uri,
                       int32 uri_len, char *artist, int32 artist_len,
                       char *title, int32 title_len) {
    NcmMutableSong song;

    ncm_mutable_song_init(&song);
    assert(ncm_mutable_song_set_uri(&song, uri, uri_len));
    assert(ncm_mutable_song_set_name(&song, uri, uri_len));
    assert(ncm_mutable_song_set_original_tag(
               &song, NCM_TAGS_FIELD_ARTIST, 0, artist, artist_len));
    assert(ncm_mutable_song_set_original_tag(
               &song, NCM_TAGS_FIELD_TITLE, 0, title, title_len));
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
append_mpd_song(NcmMpdSongList *songs, char *uri, int32 uri_len) {
    NcmSong song;

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, uri, uri_len));
    assert(ncm_mpd_song_list_append_copy(songs, &song));
    ncm_song_destroy(&song);
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
set_test_buffer(NcBuffer *buffer, char *data, int32 data_len) {
    nc_buffer_init(buffer);
    nc_buffer_append_data(buffer, data, data_len);
    return;
}

static void
assert_tag_editor_menu_config(NcMenu *menu, NcBuffer *highlight_prefix,
                              NcBuffer *highlight_suffix) {
    assert(menu->cyclic_scroll_enabled == Config.use_cyclic_scrolling);
    assert(menu->autocenter_cursor == Config.centered_cursor);
    assert(nc_buffer_equal(&menu->selected_prefix,
                           &Config.selected_item_prefix));
    assert(nc_buffer_equal(&menu->selected_suffix,
                           &Config.selected_item_suffix));
    assert(nc_buffer_equal(&menu->highlight_prefix, highlight_prefix));
    assert(nc_buffer_equal(&menu->highlight_suffix, highlight_suffix));
    return;
}

static void
assert_song_uri(NcmSong *song, char *uri, int32 uri_len) {
    NcmStringView view;

    assert(ncm_song_uri_view(song, 0, &view));
    assert(ncm_string_equal(view.data, view.len, uri, uri_len));
    return;
}

static void
assert_printed_equals(char *expected, int32 expected_len) {
    assert(window_trace.printed_len == expected_len);
    assert(ncm_string_equal(window_trace.printed, window_trace.printed_len,
                            expected, expected_len));
    return;
}

static void
assert_printed_contains(char *needle, int32 needle_len) {
    bool found;

    found = false;
    for (int32 i = 0; i + needle_len <= window_trace.printed_len; i += 1) {
        if (ncm_string_equal(window_trace.printed + i, needle_len,
                             needle, needle_len)) {
            found = true;
            break;
        }
    }
    assert(found);
    return;
}

static void
set_rendering_config(NcBuffer *old_modified_prefix, char **old_empty_tag,
                     int32 *old_empty_tag_len, char **old_tags_separator,
                     int32 *old_tags_separator_len) {
    *old_modified_prefix = Config.modified_item_prefix;
    *old_empty_tag = Config.empty_tag;
    *old_empty_tag_len = Config.empty_tag_len;
    *old_tags_separator = Config.tags_separator;
    *old_tags_separator_len = Config.tags_separator_len;

    set_test_buffer(&Config.modified_item_prefix, LIT_ARGS("modified: "));
    Config.empty_tag = (char *)"<empty>";
    Config.empty_tag_len = STRLIT_LEN("<empty>");
    Config.tags_separator = (char *)" | ";
    Config.tags_separator_len = STRLIT_LEN(" | ");
    return;
}

static void
restore_rendering_config(NcBuffer *old_modified_prefix, char *old_empty_tag,
                         int32 old_empty_tag_len, char *old_tags_separator,
                         int32 old_tags_separator_len) {
    nc_buffer_destroy(&Config.modified_item_prefix);
    Config.modified_item_prefix = *old_modified_prefix;
    Config.empty_tag = old_empty_tag;
    Config.empty_tag_len = old_empty_tag_len;
    Config.tags_separator = old_tags_separator;
    Config.tags_separator_len = old_tags_separator_len;
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
bridge_sync(void *user) {
    TagEditorBridgeTrace *trace;

    trace = user;
    trace->sync_calls += 1;
    return;
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
    bridge.sync = bridge_sync;
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
    assert(screen.parser_dialog_start_x == 37);
    assert(screen.parser_dialog_start_y == 10);
    assert(screen.parser_dialog_width == 30);
    assert(screen.parser_dialog_height == 5);
    assert(screen.parser_start_x == 7);
    assert(screen.parser_start_y == 3);
    assert(screen.parser_width == 90);
    assert(screen.parser_width_one == 45);
    assert(screen.parser_width_two == 45);
    assert(screen.parser_height == 20);
    assert(screen.parser_helper_start_x == 52);
    assert(screen.directories_window.start_x == 2);
    assert(screen.directories_window.width == 37);
    assert(screen.tag_types_window.start_x == 40);
    assert(screen.tag_types_window.width == 26);
    assert(screen.tags_window.start_x == 67);
    assert(screen.tags_window.width == 35);
    assert(screen.parser_dialog_window.start_x == 37);
    assert(screen.parser_dialog_window.start_y == 10);
    assert(screen.parser_dialog_window.width == 30);
    assert(screen.parser_dialog_window.height == 5);
    assert(screen.parser_window.start_x == 7);
    assert(screen.parser_window.width == 45);
    assert(screen.parser_helper_window.start_x == 52);
    assert(screen.parser_helper_window.width == 45);

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
test_menu_configuration_and_highlights(void) {
    NativeTagEditorScreen screen;
    NcBuffer old_selected_prefix;
    NcBuffer old_selected_suffix;
    NcBuffer old_current_prefix;
    NcBuffer old_current_suffix;
    NcBuffer old_inactive_prefix;
    NcBuffer old_inactive_suffix;
    bool old_cyclic_scrolling;
    bool old_centered_cursor;
    NcMenu *directories;
    NcMenu *tag_types;
    NcMenu *tags;
    NcMenu *parser_dialog;
    NcMenu *parser_rows;

    old_selected_prefix = Config.selected_item_prefix;
    old_selected_suffix = Config.selected_item_suffix;
    old_current_prefix = Config.current_item_prefix;
    old_current_suffix = Config.current_item_suffix;
    old_inactive_prefix = Config.current_item_inactive_column_prefix;
    old_inactive_suffix = Config.current_item_inactive_column_suffix;
    old_cyclic_scrolling = Config.use_cyclic_scrolling;
    old_centered_cursor = Config.centered_cursor;

    set_test_buffer(&Config.selected_item_prefix, LIT_ARGS("<sel>"));
    set_test_buffer(&Config.selected_item_suffix, LIT_ARGS("</sel>"));
    set_test_buffer(&Config.current_item_prefix, LIT_ARGS("<cur>"));
    set_test_buffer(&Config.current_item_suffix, LIT_ARGS("</cur>"));
    set_test_buffer(&Config.current_item_inactive_column_prefix,
                    LIT_ARGS("<inactive>"));
    set_test_buffer(&Config.current_item_inactive_column_suffix,
                    LIT_ARGS("</inactive>"));
    Config.use_cyclic_scrolling = true;
    Config.centered_cursor = true;

    init_screen(&screen);
    directories = nc_editor_pair_menu_base(&screen.directories);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    tags = nc_tag_row_menu_base(&screen.tags);
    parser_dialog = nc_editor_string_menu_base(&screen.parser_dialog);
    parser_rows = nc_editor_string_menu_base(&screen.parser_rows);

    assert_tag_editor_menu_config(directories, &Config.current_item_prefix,
                                  &Config.current_item_suffix);
    assert_tag_editor_menu_config(tag_types,
                                  &Config.current_item_inactive_column_prefix,
                                  &Config.current_item_inactive_column_suffix);
    assert_tag_editor_menu_config(tags,
                                  &Config.current_item_inactive_column_prefix,
                                  &Config.current_item_inactive_column_suffix);
    assert_tag_editor_menu_config(parser_dialog,
                                  &parser_dialog->highlight_prefix,
                                  &parser_dialog->highlight_suffix);
    assert_tag_editor_menu_config(parser_rows, &parser_rows->highlight_prefix,
                                  &parser_rows->highlight_suffix);

    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    native_tag_editor_screen_next_column(&screen);
    assert_tag_editor_menu_config(directories,
                                  &Config.current_item_inactive_column_prefix,
                                  &Config.current_item_inactive_column_suffix);
    assert_tag_editor_menu_config(tag_types, &Config.current_item_prefix,
                                  &Config.current_item_suffix);
    assert_tag_editor_menu_config(tags,
                                  &Config.current_item_inactive_column_prefix,
                                  &Config.current_item_inactive_column_suffix);

    native_tag_editor_screen_next_column(&screen);
    assert_tag_editor_menu_config(tags, &Config.current_item_prefix,
                                  &Config.current_item_suffix);

    destroy_screen(&screen);
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
    Config.use_cyclic_scrolling = old_cyclic_scrolling;
    Config.centered_cursor = old_centered_cursor;
    return;
}

static void
test_title_visibility_configuration(void) {
    NativeTagEditorScreen screen;
    bool old_visibility;

    old_visibility = Config.titles_visibility;
    Config.titles_visibility = false;
    init_screen(&screen);

    assert(screen.directories_title.len == 0);
    assert(screen.tag_types_title.len == 0);
    assert(screen.tags_title.len == 0);
    assert(screen.parser_title.len == 0);
    assert(screen.parser_helper_title.len == 0);
    assert(screen.directories_window.title_len == 0);
    assert(screen.tag_types_window.title_len == 0);
    assert(screen.tags_window.title_len == 0);

    Config.titles_visibility = true;
    native_tag_editor_screen_set_geometry(&screen, 2, 100, 3, 20);
    assert(ncm_string_equal(screen.directories_window.title,
                            screen.directories_window.title_len,
                            STRLIT_ARGS("Directories")));
    assert(ncm_string_equal(screen.tag_types_window.title,
                            screen.tag_types_window.title_len,
                            STRLIT_ARGS("Tag types")));
    assert(ncm_string_equal(screen.tags_window.title,
                            screen.tags_window.title_len,
                            STRLIT_ARGS("Tags")));

    destroy_screen(&screen);
    Config.titles_visibility = old_visibility;
    return;
}

static void
test_native_resize_preserves_state(void) {
    NativeTagEditorScreen screen;
    NcMenu *directories;
    NcMenu *tags;
    NcmError error;

    init_screen(&screen);
    ncm_error_clear(&error);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Beta"), STRLIT_ARGS("/Beta")));
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    append_song_with_title(&screen, STRLIT_ARGS("two.flac"),
                           STRLIT_ARGS("Beta Artist"),
                           STRLIT_ARGS("Beta"));
    assert(native_tag_editor_screen_apply_directory_filter(
               &screen, STRLIT_ARGS("Alpha"), Config.regex_type, &error));
    assert(native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Alpha"), true, true, false, &error));
    native_tag_editor_screen_clear_filters(&screen);
    native_tag_editor_screen_next_column(&screen);
    native_tag_editor_screen_next_column(&screen);
    assert(native_tag_editor_screen_apply_tag_filter(
               &screen, STRLIT_ARGS("Beta"), Config.regex_type, &error));
    screen.parser_mode = NATIVE_TAG_EDITOR_PARSER_RENAME_FILES;

    directories = nc_editor_pair_menu_base(&screen.directories);
    tags = nc_tag_row_menu_base(&screen.tags);
    assert(nc_menu_goto_selectable(directories, 1));
    directories->beginning = 1;
    tags->beginning = 0;

    native_tag_editor_screen_set_geometry(&screen, 4, 80, 5, 15);

    assert(screen.parser_mode == NATIVE_TAG_EDITOR_PARSER_RENAME_FILES);
    assert(nc_menu_highlight(directories) == 1);
    assert(nc_menu_highlight(tags) == 0);
    assert(directories->beginning == 1);
    assert(tags->beginning == 0);
    assert(screen.directory_filter_constraint.len == 0);
    assert(ncm_string_equal(screen.directory_search_constraint.data,
                            screen.directory_search_constraint.len,
                            STRLIT_ARGS("Alpha")));
    assert(ncm_string_equal(screen.tag_filter_constraint.data,
                            screen.tag_filter_constraint.len,
                            STRLIT_ARGS("Beta")));

    assert(screen.left_width == 27);
    assert(screen.middle_start_x == 32);
    assert(screen.middle_width == 26);
    assert(screen.right_start_x == 59);
    assert(screen.right_width == 25);
    assert(screen.parser_dialog_start_x == 29);
    assert(screen.parser_dialog_start_y == 10);
    assert(screen.parser_start_x == 8);
    assert(screen.parser_start_y == 5);
    assert(screen.parser_width == 72);
    assert(screen.parser_width_one == 36);
    assert(screen.parser_width_two == 36);
    assert(screen.parser_helper_start_x == 44);

    destroy_screen(&screen);
    return;
}

static void
test_small_geometry_bounds(void) {
    NativeTagEditorScreen screen;

    init_screen(&screen);
    native_tag_editor_screen_set_geometry(&screen, 0, 2, 0, 0);

    assert(screen.width == 2);
    assert(screen.main_height == 1);
    assert(screen.left_width >= 0);
    assert(screen.middle_width >= 1);
    assert(screen.right_width >= 1);
    assert(screen.parser_dialog_width >= 1);
    assert(screen.parser_dialog_height >= 1);
    assert(screen.parser_width >= 1);
    assert(screen.parser_width_one >= 1);
    assert(screen.parser_width_two >= 1);
    assert(screen.parser_height >= 1);
    assert(screen.directories_window.height == 1);
    assert(screen.tag_types_window.height == 1);
    assert(screen.tags_window.height == 1);

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

    assert(nc_screen_active_window(base) == &screen.directories_window);
    assert(bridge_trace.active_window_calls == 0);

    nc_screen_refresh(base);
    assert(bridge_trace.sync_calls == 0);
    assert(bridge_trace.refresh_calls == 0);
    assert(window_trace.display_calls == 3);

    nc_screen_refresh_window(base);
    assert(bridge_trace.sync_calls == 0);
    assert(bridge_trace.refresh_window_calls == 0);
    assert(window_trace.refresh_calls == 4);

    nc_screen_scroll(base, NC_SCROLL_PAGE_DOWN);
    assert(bridge_trace.scroll_calls == 0);

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

    reset_mpd_trace();
    nc_screen_request_update(base);
    assert(nc_screen_has_to_be_updated(base));
    nc_screen_update(base);
    assert(bridge_trace.update_calls == 0);
    assert(bridge_trace.sync_calls == 0);
    assert(mpd_trace.get_directory_list_calls == 1);
    assert(mpd_trace.get_songs_calls == 1);
    assert(!nc_screen_has_to_be_updated(base));

    native_tag_editor_screen_clear_directories(&screen);
    assert(bridge_trace.clear_directories_calls == 1);

    event = (MEVENT){0};
    event.x = 11;
    event.y = 12;
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(base, event);
    assert(bridge_trace.mouse_calls == 0);

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
    assert(nc_menu_item_count(menu) == 3);
    pair = nc_menu_active_item_at(menu, 0);
    assert_menu_string_pair(pair, STRLIT_ARGS("."),
                            STRLIT_ARGS("/"));
    pair = nc_menu_active_item_at(menu, 1);
    assert_menu_string_pair(pair, STRLIT_ARGS(".."),
                            STRLIT_ARGS("/parent"));
    pair = nc_menu_active_item_at(menu, 2);
    assert_menu_string_pair(pair, STRLIT_ARGS("Alpha"),
                            STRLIT_ARGS("/Alpha"));
    assert(nc_menu_goto_selectable(menu, 2));

    native_tag_editor_screen_clear_filters(&screen);
    assert(nc_menu_item_count(menu) == 4);
    assert(nc_menu_highlight(menu) == 2);
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
    assert(nc_menu_goto_selectable(nc_editor_string_menu_base(
               &screen.tag_types), 1));
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
    assert(window_trace.refresh_calls == 3);
    assert(window_trace.separator_calls == 2);
    assert(window_trace.separator_x[0] == 39);
    assert(window_trace.separator_x[1] == 66);

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
    assert(nc_menu_goto_selectable(nc_editor_string_menu_base(
               &screen.tag_types), 1));
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


static void
test_native_directory_and_tag_type_rendering(void) {
    NativeTagEditorScreen screen;
    NcMenu *tag_types;

    init_screen(&screen);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Álpha"), STRLIT_ARGS("/Álpha")));
    assert(native_tag_editor_screen_add_directory(
               &screen, "bad\xff", 4, STRLIT_ARGS("/bad")));

    reset_window_trace();
    nc_screen_refresh_window(native_tag_editor_screen_base(&screen));
    assert_printed_equals("Álpha\nbad\xff\n", 12);

    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    native_tag_editor_screen_next_column(&screen);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    assert(nc_menu_goto_selectable(tag_types, 12));
    reset_window_trace();
    nc_screen_refresh_window(native_tag_editor_screen_base(&screen));
    assert_printed_contains(STRLIT_ARGS("Filename"));

    destroy_screen(&screen);
    return;
}

static void
test_native_tag_rendering_for_tag_fields(void) {
    NativeTagEditorScreen screen;
    NcMenu *tag_types;
    NcBuffer old_modified_prefix;
    char *old_empty_tag;
    char *old_tags_separator;
    int32 old_empty_tag_len;
    int32 old_tags_separator_len;

    set_rendering_config(&old_modified_prefix, &old_empty_tag,
                         &old_empty_tag_len, &old_tags_separator,
                         &old_tags_separator_len);
    init_screen(&screen);
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS(""));
    append_song(&screen, STRLIT_ARGS("two.flac"), STRLIT_ARGS("Beta"));
    assert(native_tag_editor_screen_apply_tag_to_selection(
               &screen, NCM_TAGS_FIELD_ARTIST, STRLIT_ARGS("Changed"),
               Config.tags_separator, Config.tags_separator_len));
    native_tag_editor_screen_next_column(&screen);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    assert(nc_menu_goto_selectable(tag_types, 1));
    native_tag_editor_screen_next_column(&screen);

    reset_window_trace();
    nc_screen_refresh_window(native_tag_editor_screen_base(&screen));
    assert_printed_equals("modified: Changed\nmodified: Changed\n", 36);
    assert(window_trace.property_calls == 0);

    destroy_screen(&screen);
    restore_rendering_config(&old_modified_prefix, old_empty_tag,
                             old_empty_tag_len, old_tags_separator,
                             old_tags_separator_len);
    return;
}

static void
test_native_tag_rendering_for_filename_rows(void) {
    NativeTagEditorScreen screen;
    NcMenu *tag_types;
    NcmMutableSong *song;
    NcBuffer old_modified_prefix;
    char *old_empty_tag;
    char *old_tags_separator;
    int32 old_empty_tag_len;
    int32 old_tags_separator_len;

    set_rendering_config(&old_modified_prefix, &old_empty_tag,
                         &old_empty_tag_len, &old_tags_separator,
                         &old_tags_separator_len);
    init_screen(&screen);
    append_song(&screen, STRLIT_ARGS("old.flac"), STRLIT_ARGS("Artist"));
    song = nc_tag_row_menu_current(&screen.tags);
    assert(song != NULL);
    assert(ncm_mutable_song_set_new_name(song, STRLIT_ARGS("new.flac")));

    native_tag_editor_screen_next_column(&screen);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    assert(nc_menu_goto_selectable(tag_types, 12));
    native_tag_editor_screen_next_column(&screen);

    reset_window_trace();
    nc_screen_refresh_window(native_tag_editor_screen_base(&screen));
    assert_printed_equals("modified: old.flac -> new.flac\n", 31);
    assert(window_trace.property_calls == 2);

    destroy_screen(&screen);
    restore_rendering_config(&old_modified_prefix, old_empty_tag,
                             old_empty_tag_len, old_tags_separator,
                             old_tags_separator_len);
    return;
}

static void
test_native_refresh_does_not_delegate_rendering(void) {
    NativeTagEditorScreen screen;

    init_screen(&screen);
    reset_bridge_trace();
    native_tag_editor_screen_set_bridge(&screen, bridge_callbacks());
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));

    reset_window_trace();
    nc_screen_refresh(native_tag_editor_screen_base(&screen));
    assert(bridge_trace.sync_calls == 0);
    assert(bridge_trace.refresh_calls == 0);
    assert(window_trace.display_calls == 3);
    assert(window_trace.refresh_calls == 3);
    assert(window_trace.printed_len > 0);

    reset_window_trace();
    nc_screen_refresh_window(native_tag_editor_screen_base(&screen));
    assert(bridge_trace.sync_calls == 0);
    assert(bridge_trace.refresh_window_calls == 0);
    assert(window_trace.refresh_calls == 1);

    destroy_screen(&screen);
    return;
}

static void
test_native_update_fetches_directories_and_songs(void) {
    NativeTagEditorScreen screen;
    NcMenu *directories;
    NcMenu *tags;
    NcMenuStringPair *pair;
    NcmMutableSong *song;

    reset_mpd_trace();
    append_directory(&mpd_trace.directories, STRLIT_ARGS("/The Beta"));
    append_directory(&mpd_trace.directories, STRLIT_ARGS("/Alpha"));
    append_mpd_song(&mpd_trace.songs, STRLIT_ARGS("zeta.flac"));
    append_mpd_song(&mpd_trace.songs, STRLIT_ARGS("alpha.flac"));

    init_screen(&screen);
    reset_bridge_trace();
    native_tag_editor_screen_set_bridge(&screen, bridge_callbacks());
    nc_screen_request_update(native_tag_editor_screen_base(&screen));
    nc_screen_update(native_tag_editor_screen_base(&screen));

    assert(bridge_trace.update_calls == 0);
    assert(bridge_trace.sync_calls == 0);
    assert(mpd_trace.get_directory_list_calls == 1);
    assert(mpd_trace.get_songs_calls == 1);
    assert(ncm_string_equal(mpd_trace.directory_path,
                            mpd_trace.directory_path_len,
                            STRLIT_ARGS("/")));
    assert(ncm_string_equal(mpd_trace.songs_path,
                            mpd_trace.songs_path_len,
                            STRLIT_ARGS("/")));

    directories = nc_editor_pair_menu_base(&screen.directories);
    assert(nc_menu_item_count(directories) == 3);
    pair = nc_menu_active_item_at(directories, 0);
    assert_menu_string_pair(pair, STRLIT_ARGS("."), STRLIT_ARGS("/"));
    pair = nc_menu_active_item_at(directories, 1);
    assert_menu_string_pair(pair, STRLIT_ARGS("Alpha"),
                            STRLIT_ARGS("/Alpha"));
    pair = nc_menu_active_item_at(directories, 2);
    assert_menu_string_pair(pair, STRLIT_ARGS("The Beta"),
                            STRLIT_ARGS("/The Beta"));
    assert(!screen.directories_update_requested);

    tags = nc_tag_row_menu_base(&screen.tags);
    assert(nc_menu_item_count(tags) == 2);
    song = nc_menu_active_item_at(tags, 0);
    assert(song != NULL);
    assert(ncm_string_equal(song->uri, song->uri_len,
                            STRLIT_ARGS("alpha.flac")));
    song = nc_menu_active_item_at(tags, 1);
    assert(song != NULL);
    assert(ncm_string_equal(song->uri, song->uri_len,
                            STRLIT_ARGS("zeta.flac")));
    assert(!screen.tags_update_requested);
    assert(screen.displayed_dir_valid);
    assert(ncm_string_equal(screen.displayed_dir.data,
                            screen.displayed_dir.len,
                            STRLIT_ARGS("/")));

    destroy_screen(&screen);
    return;
}

static void
test_native_update_reports_fetch_failures(void) {
    NativeTagEditorScreen screen;

    reset_mpd_trace();
    mpd_trace.get_directory_list_result = false;
    init_screen(&screen);
    reset_bridge_trace();
    native_tag_editor_screen_set_bridge(&screen, bridge_callbacks());

    nc_screen_request_update(native_tag_editor_screen_base(&screen));
    nc_screen_update(native_tag_editor_screen_base(&screen));
    assert(bridge_trace.update_calls == 0);
    assert(mpd_trace.get_directory_list_calls == 1);
    assert(mpd_trace.status_calls == 1);
    assert(ncm_string_equal(mpd_trace.status_message,
                            mpd_trace.status_message_len,
                            STRLIT_ARGS("Could not fetch directories: "
                                        "directory failure")));
    assert(!screen.directories_update_requested);

    destroy_screen(&screen);

    reset_mpd_trace();
    append_directory(&mpd_trace.directories, STRLIT_ARGS("/Alpha"));
    mpd_trace.get_songs_result = false;
    init_screen(&screen);
    nc_screen_request_update(native_tag_editor_screen_base(&screen));
    nc_screen_update(native_tag_editor_screen_base(&screen));
    assert(mpd_trace.get_directory_list_calls == 1);
    assert(mpd_trace.get_songs_calls == 1);
    assert(mpd_trace.status_calls == 1);
    assert(ncm_string_equal(mpd_trace.status_message,
                            mpd_trace.status_message_len,
                            STRLIT_ARGS("Could not fetch songs: "
                                        "songs failure")));
    assert(!screen.tags_update_requested);

    destroy_screen(&screen);
    return;
}

static void
test_directory_filter_keeps_control_rows(void) {
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

    menu = nc_editor_pair_menu_base(&screen.directories);
    assert(native_tag_editor_screen_apply_directory_filter(
               &screen, STRLIT_ARGS("Alpha"), Config.regex_type, &error));
    assert(nc_menu_item_count(menu) == 3);
    pair = nc_menu_active_item_at(menu, 0);
    assert_menu_string_pair(pair, STRLIT_ARGS("."), STRLIT_ARGS("/"));
    pair = nc_menu_active_item_at(menu, 1);
    assert_menu_string_pair(pair, STRLIT_ARGS(".."),
                            STRLIT_ARGS("/parent"));
    pair = nc_menu_active_item_at(menu, 2);
    assert_menu_string_pair(pair, STRLIT_ARGS("Alpha"),
                            STRLIT_ARGS("/Alpha"));

    native_tag_editor_screen_clear_filters(&screen);
    assert(nc_menu_goto_selectable(menu, 0));
    assert(!native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("."), true, true, false, &error));
    assert(nc_menu_highlight(menu) == 0);

    destroy_screen(&screen);
    return;
}

static void
test_tag_filter_uses_selected_rendered_field(void) {
    NativeTagEditorScreen screen;
    NcMenu *tag_types;
    NcMenu *tags;
    NcmError error;

    init_screen(&screen);
    ncm_error_clear(&error);
    append_song_with_title(&screen, STRLIT_ARGS("one.flac"),
                           STRLIT_ARGS("Alpha"),
                           STRLIT_ARGS("Needle title"));
    append_song_with_title(&screen, STRLIT_ARGS("two.flac"),
                           STRLIT_ARGS("Beta"),
                           STRLIT_ARGS("Other title"));
    native_tag_editor_screen_next_column(&screen);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    assert(nc_menu_goto_selectable(tag_types, 1));
    native_tag_editor_screen_next_column(&screen);
    tags = nc_tag_row_menu_base(&screen.tags);

    assert(native_tag_editor_screen_apply_tag_filter(
               &screen, STRLIT_ARGS("Needle title"), Config.regex_type,
               &error));
    assert(nc_menu_item_count(tags) == 0);

    native_tag_editor_screen_clear_filters(&screen);
    native_tag_editor_screen_previous_column(&screen);
    assert(nc_menu_goto_selectable(tag_types, 0));
    native_tag_editor_screen_next_column(&screen);
    assert(native_tag_editor_screen_apply_tag_filter(
               &screen, STRLIT_ARGS("Needle title"), Config.regex_type,
               &error));
    assert(nc_menu_item_count(tags) == 1);

    destroy_screen(&screen);
    return;
}

static void
test_tag_search_uses_modified_and_filename_rows(void) {
    NativeTagEditorScreen screen;
    NcMenu *tag_types;
    NcMenu *tags;
    NcmMutableSong *song;
    NcmError error;

    init_screen(&screen);
    ncm_error_clear(&error);
    append_song(&screen, STRLIT_ARGS("old.flac"), STRLIT_ARGS("Alpha"));
    append_song(&screen, STRLIT_ARGS("second.flac"), STRLIT_ARGS("Beta"));
    tags = nc_tag_row_menu_base(&screen.tags);
    song = nc_menu_active_item_at(tags, 0);
    assert(song != NULL);
    assert(ncm_mutable_song_set_tag(
               song, NCM_TAGS_FIELD_ARTIST, 0, STRLIT_ARGS("Changed")));
    assert(ncm_mutable_song_set_new_name(song, STRLIT_ARGS("new.flac")));

    native_tag_editor_screen_next_column(&screen);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    assert(nc_menu_goto_selectable(tag_types, 1));
    native_tag_editor_screen_next_column(&screen);
    assert(native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Changed"), true, true, false,
               &error));
    assert(nc_menu_highlight(tags) == 0);
    assert(!native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Alpha"), true, true, false,
               &error));

    native_tag_editor_screen_previous_column(&screen);
    assert(nc_menu_goto_selectable(tag_types, 12));
    native_tag_editor_screen_next_column(&screen);
    assert(native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("old.flac -> new.flac"), true,
               true, false, &error));
    assert(nc_menu_highlight(tags) == 0);

    destroy_screen(&screen);
    return;
}

static void
test_invalid_regex_preserves_previous_constraints(void) {
    NativeTagEditorScreen screen;
    NcMenu *tag_types;
    NcmError error;

    init_screen(&screen);
    ncm_error_clear(&error);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_apply_directory_filter(
               &screen, STRLIT_ARGS("Alpha"), NCM_REGEX_EXTENDED, &error));
    assert(!native_tag_editor_screen_apply_directory_filter(
               &screen, STRLIT_ARGS("("), NCM_REGEX_EXTENDED, &error));
    assert(screen.directory_filter_enabled);
    assert(ncm_string_equal(screen.directory_filter_constraint.data,
                            screen.directory_filter_constraint.len,
                            STRLIT_ARGS("Alpha")));

    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Beta"));
    native_tag_editor_screen_next_column(&screen);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    assert(nc_menu_goto_selectable(tag_types, 1));
    native_tag_editor_screen_next_column(&screen);
    Config.regex_type = NCM_REGEX_EXTENDED;
    assert(native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Beta"), true, true, false, &error));
    assert(!native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("("), true, true, false, &error));
    assert(screen.tag_search_enabled);
    assert(ncm_string_equal(screen.tag_search_constraint.data,
                            screen.tag_search_constraint.len,
                            STRLIT_ARGS("Beta")));
    Config.regex_type = NCM_REGEX_LITERAL_CASE_INSENSITIVE;

    destroy_screen(&screen);
    return;
}

static void
test_tag_type_column_is_not_searchable(void) {
    NativeTagEditorScreen screen;
    NcmError error;

    init_screen(&screen);
    ncm_error_clear(&error);
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    native_tag_editor_screen_next_column(&screen);
    assert(screen.active_column == NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES);
    assert(!native_tag_editor_screen_search(
               &screen, STRLIT_ARGS("Artist"), true, true, false,
               &error));
    assert(!screen.directory_search_enabled);
    assert(!screen.tag_search_enabled);

    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_selected_songs_use_active_tags_column(void) {
    NativeTagEditorScreen screen;
    NcmSongArray songs;
    NcMenu *tags;

    init_screen(&screen);
    ncm_song_array_init(&songs);
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    append_song(&screen, STRLIT_ARGS("two.flac"), STRLIT_ARGS("Beta"));

    assert(!native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 0);
    native_tag_editor_screen_next_column(&screen);
    assert(!native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 0);
    native_tag_editor_screen_next_column(&screen);

    assert(native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert_song_uri(&songs.items[0], STRLIT_ARGS("one.flac"));
    ncm_song_array_clear(&songs);

    tags = nc_tag_row_menu_base(&screen.tags);
    assert(nc_menu_set_position_selected(tags, 1, true));
    assert(native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert_song_uri(&songs.items[0], STRLIT_ARGS("two.flac"));

    ncm_song_array_destroy(&songs);
    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_selection_helpers(void) {
    NativeTagEditorScreen screen;
    NcmStringView view;
    NcmMutableSong song;
    enum NcmTagsField field;
    NcMenu *tag_types;
    NcMenu *tags;

    init_screen(&screen);
    ncm_mutable_song_init(&song);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_current_directory_path(&screen, &view));
    assert(ncm_string_equal(view.data, view.len, STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_active_menu_empty(&screen) == false);

    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    assert(native_tag_editor_screen_next_column_available(&screen));
    native_tag_editor_screen_next_column(&screen);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    assert(native_tag_editor_screen_current_tag_field(&screen, &field));
    assert(field == NCM_TAGS_FIELD_TITLE);
    assert(native_tag_editor_screen_current_tag_type_editable(&screen));
    assert(native_tag_editor_screen_current_tag_type_actionable(&screen));
    assert(nc_menu_goto_selectable(tag_types, 1));
    assert(native_tag_editor_screen_current_tag_field(&screen, &field));
    assert(field == NCM_TAGS_FIELD_ARTIST);

    native_tag_editor_screen_next_column(&screen);
    tags = nc_tag_row_menu_base(&screen.tags);
    assert(native_tag_editor_screen_current_tag_song(&screen, &song));
    assert(ncm_string_equal(song.uri, song.uri_len, STRLIT_ARGS("one.flac")));
    assert(native_tag_editor_screen_selected_tag_song_count(&screen) == 0);
    assert(nc_menu_set_current_selected(tags, true));
    assert(native_tag_editor_screen_selected_tag_song_count(&screen) == 1);
    assert(!native_tag_editor_screen_active_menu_empty(&screen));

    ncm_mutable_song_destroy(&song);
    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_active_menu_selection_actions(void) {
    NativeTagEditorScreen screen;
    NcMenu *directories;
    NcMenu *tag_types;
    NcMenu *tags;

    init_screen(&screen);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    append_song(&screen, STRLIT_ARGS("two.flac"), STRLIT_ARGS("Beta"));

    directories = nc_editor_pair_menu_base(&screen.directories);
    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    tags = nc_tag_row_menu_base(&screen.tags);

    assert(nc_menu_toggle_current_selected(
               native_tag_editor_screen_active_menu(&screen)));
    assert(nc_menu_selected_count(directories) == 1);
    assert(nc_menu_selected_count(tag_types) == 0);
    assert(nc_menu_selected_count(tags) == 0);

    native_tag_editor_screen_next_column(&screen);
    assert(nc_menu_toggle_current_selected(
               native_tag_editor_screen_active_menu(&screen)));
    assert(nc_menu_selected_count(directories) == 1);
    assert(nc_menu_selected_count(tag_types) == 1);
    assert(nc_menu_selected_count(tags) == 0);

    native_tag_editor_screen_next_column(&screen);
    assert(nc_menu_toggle_current_selected(
               native_tag_editor_screen_active_menu(&screen)));
    assert(nc_menu_selected_count(directories) == 1);
    assert(nc_menu_selected_count(tag_types) == 1);
    assert(nc_menu_selected_count(tags) == 1);
    nc_menu_clear_selection(native_tag_editor_screen_active_menu(&screen));
    assert(nc_menu_selected_count(directories) == 1);
    assert(nc_menu_selected_count(tag_types) == 1);
    assert(nc_menu_selected_count(tags) == 0);

    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_enter_directory_invalidates_tags(void) {
    NativeTagEditorScreen screen;
    NcmStringView view;
    NcMenu *tags;

    init_screen(&screen);
    assert(native_tag_editor_screen_set_current_dir(
               &screen, STRLIT_ARGS("/")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Alpha"));
    tags = nc_tag_row_menu_base(&screen.tags);
    assert(nc_menu_set_current_selected(tags, true));

    assert(native_tag_editor_screen_enter_directory(&screen));
    assert(native_tag_editor_screen_current_dir(&screen, &view));
    assert(ncm_string_equal(view.data, view.len, STRLIT_ARGS("/Alpha")));
    assert(nc_menu_empty(tags));
    assert(native_tag_editor_screen_selected_tag_song_count(&screen) == 0);
    assert(screen.tags_update_requested);

    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_mouse_directory_click_enters_and_clears(void) {
    NativeTagEditorScreen screen;
    NcmStringView current_dir;
    MEVENT event;

    init_screen(&screen);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Beta"), STRLIT_ARGS("/Beta")));
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Artist"));
    reset_bridge_trace();
    native_tag_editor_screen_set_bridge(&screen, bridge_callbacks());

    event = (MEVENT){0};
    event.x = (int32)screen.directories_window.start_x + 1;
    event.y = (int32)screen.directories_window.start_y + 1;
    event.bstate = BUTTON1_PRESSED;
    nc_screen_mouse_button_pressed(native_tag_editor_screen_base(&screen),
                                   event);

    assert(screen.active_column == NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES);
    assert(native_tag_editor_screen_current_dir(&screen, &current_dir));
    assert(ncm_string_equal(current_dir.data, current_dir.len,
                            STRLIT_ARGS("/Beta")));
    assert(nc_menu_empty(nc_editor_pair_menu_base(&screen.directories)));
    assert(nc_menu_empty(nc_tag_row_menu_base(&screen.tags)));
    assert(screen.directories_update_requested);
    assert(screen.tags_update_requested);
    assert(bridge_trace.mouse_calls == 0);

    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_mouse_tag_type_and_tag_clicks(void) {
    NativeTagEditorScreen screen;
    NcMenu *tag_types;
    NcMenu *tags;
    MEVENT event;

    init_screen(&screen);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    append_song_with_title(&screen, STRLIT_ARGS("one.flac"),
                           STRLIT_ARGS("Artist"), STRLIT_ARGS("Title"));
    append_song_with_title(&screen, STRLIT_ARGS("two.flac"),
                           STRLIT_ARGS("Artist"), STRLIT_ARGS("Other"));
    reset_bridge_trace();
    native_tag_editor_screen_set_bridge(&screen, bridge_callbacks());

    tag_types = nc_editor_string_menu_base(&screen.tag_types);
    tags = nc_tag_row_menu_base(&screen.tags);

    event = (MEVENT){0};
    event.x = (int32)screen.tag_types_window.start_x + 1;
    event.y = (int32)screen.tag_types_window.start_y + 1;
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(native_tag_editor_screen_base(&screen),
                                   event);
    assert(screen.active_column == NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES);
    assert(nc_menu_highlight(tag_types) == 1);
    assert(bridge_trace.run_action_calls == 1);
    assert(bridge_trace.mouse_calls == 0);
    assert(nc_menu_item_count(tags) == 2);

    event = (MEVENT){0};
    event.x = (int32)screen.tags_window.start_x + 1;
    event.y = (int32)screen.tags_window.start_y + 1;
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(native_tag_editor_screen_base(&screen),
                                   event);
    assert(screen.active_column == NATIVE_TAG_EDITOR_COLUMN_TAGS);
    assert(nc_menu_highlight(tags) == 1);
    assert(bridge_trace.run_action_calls == 2);
    assert(bridge_trace.mouse_calls == 0);

    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_mouse_wheel_directory_change(void) {
    NativeTagEditorScreen screen;
    NcMenu *directories;
    MEVENT event;
    NcmStringView observed;
    uint32 old_lines_scrolled;
    bool old_whole_page;

    init_screen(&screen);
    old_lines_scrolled = Config.lines_scrolled;
    old_whole_page = Config.mouse_list_scroll_whole_page;
    Config.lines_scrolled = 1;
    Config.mouse_list_scroll_whole_page = false;

    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Beta"), STRLIT_ARGS("/Beta")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Gamma"), STRLIT_ARGS("/Gamma")));
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Artist"));
    directories = nc_editor_pair_menu_base(&screen.directories);
    assert(nc_menu_highlight(directories) == 0);
    assert(nc_menu_item_count(nc_tag_row_menu_base(&screen.tags)) == 1);

    event = (MEVENT){0};
    event.x = (int32)screen.directories_window.start_x + 1;
    event.y = (int32)screen.directories_window.start_y;
    event.bstate = BUTTON5_PRESSED;
    nc_screen_mouse_button_pressed(native_tag_editor_screen_base(&screen),
                                   event);

    assert(nc_menu_highlight(directories) == 1);
    assert(nc_menu_empty(nc_tag_row_menu_base(&screen.tags)));
    assert(native_tag_editor_screen_observed_dir(&screen, &observed));
    assert(ncm_string_equal(observed.data, observed.len,
                            STRLIT_ARGS("/Beta")));
    assert(screen.tags_update_requested);

    Config.lines_scrolled = old_lines_scrolled;
    Config.mouse_list_scroll_whole_page = old_whole_page;
    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_tag_type_scroll_refreshes_tags(void) {
    NativeTagEditorScreen screen;
    NcMenu *tags;

    init_screen(&screen);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    append_song_with_title(&screen, STRLIT_ARGS("one.flac"),
                           STRLIT_ARGS("Artist"), STRLIT_ARGS("Title"));
    append_song_with_title(&screen, STRLIT_ARGS("two.flac"),
                           STRLIT_ARGS("Artist"), STRLIT_ARGS("Other"));
    assert(native_tag_editor_screen_next_column_available(&screen));
    native_tag_editor_screen_next_column(&screen);
    tags = nc_tag_row_menu_base(&screen.tags);
    assert(nc_menu_set_position_selected(tags, 1, true));

    reset_window_trace();
    nc_screen_scroll(native_tag_editor_screen_base(&screen),
                     NC_SCROLL_DOWN);
    assert(screen.active_column == NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES);
    assert(nc_menu_highlight(nc_editor_string_menu_base(&screen.tag_types))
           == 1);
    assert(nc_menu_item_count(tags) == 2);
    assert(nc_menu_position_is_selected(tags, 1));
    assert(window_trace.refresh_calls >= 1);

    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_search_result_directory_change(void) {
    NativeTagEditorScreen screen;
    NcmError error;
    NcmStringView observed;

    init_screen(&screen);
    ncm_error_clear(&error);
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Alpha"), STRLIT_ARGS("/Alpha")));
    assert(native_tag_editor_screen_add_directory(
               &screen, STRLIT_ARGS("Beta"), STRLIT_ARGS("/Beta")));
    append_song(&screen, STRLIT_ARGS("one.flac"), STRLIT_ARGS("Artist"));

    assert(native_tag_editor_screen_search(&screen, STRLIT_ARGS("Beta"),
                                           true, true, true, &error));
    assert(nc_menu_empty(nc_tag_row_menu_base(&screen.tags)));
    assert(native_tag_editor_screen_observed_dir(&screen, &observed));
    assert(ncm_string_equal(observed.data, observed.len,
                            STRLIT_ARGS("/Beta")));
    assert(screen.tags_update_requested);

    destroy_screen(&screen);
    return;
}

static void
test_tag_editor_parser_mouse_rows(void) {
    NativeTagEditorScreen screen;
    NcMenu *parser_rows;
    MEVENT event;

    init_screen(&screen);
    assert(native_tag_editor_screen_prepare_parser_rows(
               &screen, NATIVE_TAG_EDITOR_PARSER_TAGS_FROM_FILENAME,
               STRLIT_ARGS("%a - %t")));
    reset_bridge_trace();
    native_tag_editor_screen_set_bridge(&screen, bridge_callbacks());
    parser_rows = nc_editor_string_menu_base(&screen.parser_rows);

    event = (MEVENT){0};
    event.x = (int32)screen.parser_window.start_x + 1;
    event.y = (int32)screen.parser_window.start_y + 4;
    event.bstate = BUTTON3_PRESSED;
    nc_screen_mouse_button_pressed(native_tag_editor_screen_base(&screen),
                                   event);

    assert(nc_menu_highlight(parser_rows) == 4);
    assert(bridge_trace.run_action_calls == 1);
    assert(bridge_trace.mouse_calls == 0);

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

bool
__wrap_nc_window_has_coords(NcWindow *window, int32 *x, int32 *y) {
    if ((window == NULL) || (x == NULL) || (y == NULL)) {
        return false;
    }
    if ((*x < window->start_x) || (*x >= window->start_x + window->width)) {
        return false;
    }
    if ((*y < window->start_y) || (*y >= window->start_y + window->height)) {
        return false;
    }
    *x -= (int32)window->start_x;
    *y -= (int32)window->start_y;
    return true;
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
    int64 count;

    (void)width;
    window_trace.refresh_calls += 1;
    if (menu == NULL || window == NULL) {
        return;
    }
    count = nc_menu_item_count(menu);
    if (count > height) {
        count = height;
    }
    for (int64 i = 0; i < count; i += 1) {
        void *item;

        if (nc_menu_position_is_separator(menu, i)) {
            continue;
        }
        item = nc_menu_active_item_at(menu, i);
        if (menu->display_callbacks.draw != NULL) {
            menu->display_callbacks.draw(menu, window, item, i,
                                         menu->display_callbacks.user);
            __wrap_nc_window_print_char(window, '\n');
        }
    }
    return;
}

void
__wrap_nc_window_print_data(NcWindow *window, char *string,
                            int32 string_len) {
    (void)window;
    window_trace.print_data_calls += 1;
    if ((string == NULL) || (string_len <= 0)) {
        return;
    }
    assert(window_trace.printed_len + string_len
           < (int32)SIZEOF(window_trace.printed));
    ncm_memcpy(window_trace.printed + window_trace.printed_len, string,
               string_len);
    window_trace.printed_len += string_len;
    window_trace.printed[window_trace.printed_len] = '\0';
    return;
}

void
__wrap_nc_window_print_char(NcWindow *window, char ch) {
    (void)window;
    window_trace.print_char_calls += 1;
    assert(window_trace.printed_len + 1
           < (int32)SIZEOF(window_trace.printed));
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
    window_trace.property_calls += 1;
    return;
}

void
__wrap_nc_screen_draw_vertical_separator(int64 x) {
    if (window_trace.separator_calls < LENGTH(window_trace.separator_x)) {
        window_trace.separator_x[window_trace.separator_calls] = x;
    }
    window_trace.separator_calls += 1;
    return;
}

bool
__wrap_ncm_mpd_client_get_directory_list(NcmMpdClient *client, char *path,
                                         NcmDirectoryArray *directories,
                                         NcmError *error) {
    (void)client;
    mpd_trace.get_directory_list_calls += 1;
    mpd_trace.directory_path_len = 0;
    if (path != NULL) {
        mpd_trace.directory_path_len = (int32)strlen(path);
        assert(mpd_trace.directory_path_len
               < (int32)SIZEOF(mpd_trace.directory_path));
        ncm_memcpy(mpd_trace.directory_path, path,
                   mpd_trace.directory_path_len);
        mpd_trace.directory_path[mpd_trace.directory_path_len] = '\0';
    }
    if (!mpd_trace.get_directory_list_result) {
        ncm_error_set(error, EIO, STRLIT_ARGS("directory failure"));
        return false;
    }
    assert(ncm_directory_array_copy(directories, &mpd_trace.directories));
    return true;
}

bool
__wrap_ncm_mpd_client_get_songs(NcmMpdClient *client, char *path,
                                NcmMpdSongList *songs, NcmError *error) {
    (void)client;
    mpd_trace.get_songs_calls += 1;
    mpd_trace.songs_path_len = 0;
    if (path != NULL) {
        mpd_trace.songs_path_len = (int32)strlen(path);
        assert(mpd_trace.songs_path_len
               < (int32)SIZEOF(mpd_trace.songs_path));
        ncm_memcpy(mpd_trace.songs_path, path, mpd_trace.songs_path_len);
        mpd_trace.songs_path[mpd_trace.songs_path_len] = '\0';
    }
    if (!mpd_trace.get_songs_result) {
        ncm_error_set(error, EIO, STRLIT_ARGS("songs failure"));
        return false;
    }
    assert(ncm_mpd_song_list_copy(songs, &mpd_trace.songs));
    return true;
}

void
__wrap_ncm_statusbar_print_cstring(int32 delay_seconds, char *message) {
    (void)delay_seconds;
    mpd_trace.status_calls += 1;
    mpd_trace.status_message_len = 0;
    if (message != NULL) {
        mpd_trace.status_message_len = (int32)strlen(message);
        assert(mpd_trace.status_message_len
               < (int32)SIZEOF(mpd_trace.status_message));
        ncm_memcpy(mpd_trace.status_message, message,
                   mpd_trace.status_message_len);
        mpd_trace.status_message[mpd_trace.status_message_len] = '\0';
    }
    return;
}
