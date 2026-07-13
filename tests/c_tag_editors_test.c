#include <assert.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/nc_tag_editor.h"
#include "screens/nc_tiny_tag_editor.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct TinyEditorActionContext {
    int32 run_count;
    bool runnable;
} TinyEditorActionContext;

typedef struct TagEditorActionContext {
    int32 run_count;
    bool runnable;
} TagEditorActionContext;

static void test_mutable_song_tag_changes(void);
static void test_selected_songs(void);
static void test_parser_row_generation(void);
static void test_filename_parser(void);
static void test_tag_editor_run_action_bridge(void);
static void test_tiny_editor_field_updates(void);
static void test_tiny_editor_run_action_bridge(void);
static void test_save_failure_keeps_state_destroyable(void);
static bool tag_editor_action_runnable(void *user);
static bool tag_editor_run_action(void *user);
static bool tiny_editor_action_runnable(void *user);
static bool tiny_editor_run_action(void *user);

int
main(void) {
    test_mutable_song_tag_changes();
    test_selected_songs();
    test_parser_row_generation();
    test_filename_parser();
    test_tag_editor_run_action_bridge();
    test_tiny_editor_field_updates();
    test_tiny_editor_run_action_bridge();
    test_save_failure_keeps_state_destroyable();
    return EXIT_SUCCESS;
}

static void
test_mutable_song_tag_changes(void) {
    NativeTagEditorScreen screen;
    NcmMutableSong song;
    NcmMutableSong current;
    NcmStringView view;

    native_tag_editor_screen_init(&screen, 0, 90, 0, 24,
                                  nc_color_default(), nc_border_none());
    ncm_mutable_song_init(&song);
    ncm_mutable_song_init(&current);

    assert(ncm_mutable_song_set_uri(&song, LIT_ARGS("one.flac")));
    assert(ncm_mutable_song_set_name(&song, LIT_ARGS("one.flac")));
    assert(ncm_mutable_song_set_original_tag(&song,
                                             NCM_TAGS_FIELD_ARTIST, 0,
                                             LIT_ARGS("Old")));
    assert(native_tag_editor_screen_add_mutable_song(&screen, &song));
    assert(native_tag_editor_screen_apply_tag_to_selection(
        &screen, NCM_TAGS_FIELD_ARTIST, LIT_ARGS("New"), NULL, 0));
    assert(native_tag_editor_screen_current_song(&screen, &current));
    assert(ncm_mutable_song_get_tag(&current, NCM_TAGS_FIELD_ARTIST,
                                    0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("New")));

    ncm_mutable_song_destroy(&current);
    ncm_mutable_song_destroy(&song);
    native_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_selected_songs(void) {
    NativeTagEditorScreen screen;
    NcmMutableSong first;
    NcmMutableSong second;
    NcmSongArray songs;
    NcmStringView value;
    NcMenu *menu;

    native_tag_editor_screen_init(&screen, 0, 90, 0, 24,
                                  nc_color_default(), nc_border_none());
    ncm_mutable_song_init(&first);
    ncm_mutable_song_init(&second);
    ncm_song_array_init(&songs);

    assert(ncm_mutable_song_set_uri(&first, LIT_ARGS("one.flac")));
    assert(ncm_mutable_song_set_original_tag(
        &first, NCM_TAGS_FIELD_ARTIST, 0, LIT_ARGS("Original one")));
    assert(ncm_mutable_song_set_tag(
        &first, NCM_TAGS_FIELD_ARTIST, 0, LIT_ARGS("Edited one")));
    assert(ncm_mutable_song_set_uri(&second, LIT_ARGS("two.flac")));
    ncm_mutable_song_set_duration(&second, 321);
    ncm_mutable_song_set_mtime(&second, 1234);
    assert(ncm_mutable_song_set_original_tag(
        &second, NCM_TAGS_FIELD_ARTIST, 0, LIT_ARGS("Original two")));
    assert(native_tag_editor_screen_add_mutable_song(&screen, &first));
    assert(native_tag_editor_screen_add_mutable_song(&screen, &second));

    assert(!native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 0);

    native_tag_editor_screen_next_column(&screen);
    native_tag_editor_screen_next_column(&screen);
    menu = nc_tag_row_menu_base(native_tag_editor_screen_tags(&screen));
    assert(nc_menu_set_position_selected(menu, 1, true));
    assert(native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert(ncm_song_uri_view(&songs.items[0], 0, &value));
    assert(ncm_string_equal(value.data, value.len, LIT_ARGS("two.flac")));
    assert(ncm_song_tag_view(&songs.items[0], MPD_TAG_ARTIST, 0, &value));
    assert(ncm_string_equal(value.data, value.len,
                            LIT_ARGS("Original two")));
    assert(ncm_song_duration(&songs.items[0]) == 321);
    assert(ncm_song_mtime(&songs.items[0]) == 1234);

    nc_menu_clear_selection(menu);
    nc_menu_highlight_position(
        menu, 0, nc_menu_item_count(menu));
    assert(native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert(ncm_song_uri_view(&songs.items[0], 0, &value));
    assert(ncm_string_equal(value.data, value.len, LIT_ARGS("one.flac")));
    assert(ncm_song_tag_view(&songs.items[0], MPD_TAG_ARTIST, 0, &value));
    assert(ncm_string_equal(value.data, value.len,
                            LIT_ARGS("Original one")));

    ncm_song_array_destroy(&songs);
    ncm_mutable_song_destroy(&second);
    ncm_mutable_song_destroy(&first);
    native_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_parser_row_generation(void) {
    NativeTagEditorScreen screen;
    NcMenu *menu;

    native_tag_editor_screen_init(&screen, 0, 90, 0, 24,
                                  nc_color_default(), nc_border_none());
    assert(native_tag_editor_screen_prepare_parser_rows(
        &screen, NATIVE_TAG_EDITOR_PARSER_TAGS_FROM_FILENAME,
        LIT_ARGS("%a - %t")));
    menu = nc_editor_string_menu_base(
        native_tag_editor_screen_parser_rows(&screen));
    assert(nc_menu_item_count(menu) >= 8);

    native_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_filename_parser(void) {
    NcmMutableSong song;
    NcmBuffer preview;
    NcmStringView view;

    ncm_mutable_song_init(&song);
    ncm_buffer_init(&preview);
    assert(ncm_mutable_song_set_name(&song,
                                     LIT_ARGS("Artist - Title.flac")));
    assert(native_tag_editor_parse_filename(&song, LIT_ARGS("%a - %t"),
                                            true, &preview));
    assert(preview.len > 0);
    assert(native_tag_editor_parse_filename(&song, LIT_ARGS("%a - %t"),
                                            false, NULL));
    assert(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_ARTIST,
                                    0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("Artist")));
    assert(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_TITLE,
                                    0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("Title")));

    ncm_buffer_destroy(&preview);
    ncm_mutable_song_destroy(&song);
    return;
}

static void
test_tag_editor_run_action_bridge(void) {
    NativeTagEditorScreen screen;
    NativeTagEditorBridge bridge = {0};
    TagEditorActionContext context = {0};
    NcScreen *native_screen;

    native_tag_editor_screen_init(&screen, 0, 90, 0, 24,
                                  nc_color_default(), nc_border_none());
    native_screen = native_tag_editor_screen_base(&screen);
    assert(!nc_screen_can_run_current(native_screen));

    bridge.action_runnable = tag_editor_action_runnable;
    bridge.run_action = tag_editor_run_action;
    bridge.user = &context;
    native_tag_editor_screen_set_bridge(&screen, bridge);

    assert(!nc_screen_can_run_current(native_screen));
    context.runnable = true;
    assert(nc_screen_can_run_current(native_screen));
    assert(nc_screen_run_current(native_screen));
    assert(context.run_count == 1);

    native_tag_editor_screen_destroy(&screen);
    return;
}

static bool
tag_editor_action_runnable(void *user) {
    TagEditorActionContext *context;

    context = user;
    return context->runnable;
}

static bool
tag_editor_run_action(void *user) {
    TagEditorActionContext *context;

    context = user;
    context->run_count += 1;
    return true;
}

static void
test_tiny_editor_field_updates(void) {
    NativeTinyTagEditorScreen screen;
    NcmMutableSong song;
    NcmStringView view;
    NcmTaglibAudioProperties properties = {0};

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    ncm_mutable_song_init(&song);
    assert(ncm_mutable_song_set_uri(&song, LIT_ARGS("old.flac")));
    assert(ncm_mutable_song_set_name(&song, LIT_ARGS("old.flac")));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(&screen,
                                                                 &song));
    assert(native_tiny_tag_editor_screen_reload_rows(&screen, &properties,
                                                     true));
    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE, LIT_ARGS("New title"), NULL, 0));
    assert(native_tiny_tag_editor_screen_set_filename(
        &screen, LIT_ARGS("new.flac")));
    assert(ncm_mutable_song_get_tag(
        native_tiny_tag_editor_screen_edited(&screen),
        NCM_TAGS_FIELD_TITLE, 0, &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("New title")));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("new.flac")));

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_tiny_editor_run_action_bridge(void) {
    NativeTinyTagEditorScreen screen;
    NativeTinyTagEditorBridge bridge = {0};
    TinyEditorActionContext context = {0};
    NcScreen *native_screen;

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    native_screen = native_tiny_tag_editor_screen_base(&screen);
    assert(!nc_screen_can_run_current(native_screen));

    bridge.action_runnable = tiny_editor_action_runnable;
    bridge.run_action = tiny_editor_run_action;
    bridge.user = &context;
    native_tiny_tag_editor_screen_set_bridge(&screen, bridge);

    assert(!nc_screen_can_run_current(native_screen));
    context.runnable = true;
    assert(nc_screen_can_run_current(native_screen));
    assert(nc_screen_run_current(native_screen));
    assert(context.run_count == 1);

    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}

static bool
tiny_editor_action_runnable(void *user) {
    TinyEditorActionContext *context;

    context = user;
    return context->runnable;
}

static bool
tiny_editor_run_action(void *user) {
    TinyEditorActionContext *context;

    context = user;
    context->run_count += 1;
    return true;
}

static void
test_save_failure_keeps_state_destroyable(void) {
    NativeTinyTagEditorScreen screen;
    NcmMutableSong song;

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    ncm_mutable_song_init(&song);
    assert(ncm_mutable_song_set_name(&song, LIT_ARGS("missing-uri.flac")));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(&screen,
                                                                 &song));
    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE, LIT_ARGS("Unsaved"), NULL, 0));
    assert(!native_tiny_tag_editor_screen_save(&screen, (char *)"/music"));
    assert(native_tiny_tag_editor_screen_edited(&screen) != NULL);

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}
