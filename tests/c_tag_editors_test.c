#include <assert.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/nc_tag_editor.h"
#include "screens/nc_tiny_tag_editor.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

static void test_mutable_song_tag_changes(void);
static void test_parser_row_generation(void);
static void test_filename_parser(void);
static void test_tiny_editor_field_updates(void);
static void test_save_failure_keeps_state_destroyable(void);

int
main(void) {
    test_mutable_song_tag_changes();
    test_parser_row_generation();
    test_filename_parser();
    test_tiny_editor_field_updates();
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
