#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/nc_tag_editor.h"
#include "screens/nc_tiny_tag_editor.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

_Static_assert(NATIVE_TINY_TAG_EDITOR_ROW_COUNT == 24,
               "tiny tag editor row count changed");

typedef struct TinyEditorTaglibFixture {
    NcmBuffer path;
    NcmTaglibAudioProperties properties;
    int32 open_calls;
    int32 audio_calls;
    int32 extended_calls;
    int32 close_calls;
    bool open_result;
    bool audio_result;
    bool extended_result;
} TinyEditorTaglibFixture;

typedef struct TinyEditorWriteFixture {
    char *music_dir;
    int32 calls;
    bool result;
} TinyEditorWriteFixture;

typedef struct TinyEditorActionFixture {
    NcmBuffer prompt_label;
    NcmBuffer prompt_initial;
    NcmBuffer prompt_response;
    NcmBuffer updated_directory;
    NcmBuffer playlist_title;
    NcmBuffer playlist_new_name;
    NcmBuffer status_messages[4];
    NcScreen *switched_screen;
    char *music_dir;
    int32 prompt_calls;
    int32 status_calls;
    int32 switch_calls;
    int32 write_calls;
    int32 update_directory_calls;
    int32 update_playlist_calls;
    int32 browser_update_calls;
    int32 status_calls_at_side_effect;
    int32 switch_calls_at_side_effect;
    enum NativeTinyTagEditorPromptResult prompt_result;
    bool write_result;
    bool modified_at_side_effect;
} TinyEditorActionFixture;

static NcmStringView tiny_tag_names[NCM_TAGS_FIELD_LAST] = {
    { .data = "Title", .len = STRLIT_LEN("Title") },
    { .data = "Artist", .len = STRLIT_LEN("Artist") },
    {
        .data = "Album Artist",
        .len = STRLIT_LEN("Album Artist"),
    },
    { .data = "Album", .len = STRLIT_LEN("Album") },
    { .data = "Date", .len = STRLIT_LEN("Date") },
    { .data = "Track", .len = STRLIT_LEN("Track") },
    { .data = "Genre", .len = STRLIT_LEN("Genre") },
    { .data = "Composer", .len = STRLIT_LEN("Composer") },
    { .data = "Performer", .len = STRLIT_LEN("Performer") },
    { .data = "Disc", .len = STRLIT_LEN("Disc") },
    { .data = "Comment", .len = STRLIT_LEN("Comment") },
};

static NcmStringView tiny_tag_values[NCM_TAGS_FIELD_LAST] = {
    { .data = "Title value", .len = STRLIT_LEN("Title value") },
    { .data = "Artist value", .len = STRLIT_LEN("Artist value") },
    {
        .data = "Album artist value",
        .len = STRLIT_LEN("Album artist value"),
    },
    { .data = "Album value", .len = STRLIT_LEN("Album value") },
    { .data = "2026", .len = STRLIT_LEN("2026") },
    { .data = "3", .len = STRLIT_LEN("3") },
    { .data = "Genre value", .len = STRLIT_LEN("Genre value") },
    {
        .data = "Composer value",
        .len = STRLIT_LEN("Composer value"),
    },
    {
        .data = "Performer value",
        .len = STRLIT_LEN("Performer value"),
    },
    { .data = "2", .len = STRLIT_LEN("2") },
    {
        .data = "Comment value",
        .len = STRLIT_LEN("Comment value"),
    },
};

static NcmStringView tiny_tag_display_values[NCM_TAGS_FIELD_LAST] = {
    { .data = "Title value", .len = STRLIT_LEN("Title value") },
    { .data = "Artist value", .len = STRLIT_LEN("Artist value") },
    {
        .data = "Album artist value",
        .len = STRLIT_LEN("Album artist value"),
    },
    { .data = "Album value", .len = STRLIT_LEN("Album value") },
    { .data = "2026", .len = STRLIT_LEN("2026") },
    { .data = "03", .len = STRLIT_LEN("03") },
    { .data = "Genre value", .len = STRLIT_LEN("Genre value") },
    {
        .data = "Composer value",
        .len = STRLIT_LEN("Composer value"),
    },
    {
        .data = "Performer value",
        .len = STRLIT_LEN("Performer value"),
    },
    { .data = "2", .len = STRLIT_LEN("2") },
    {
        .data = "Comment value",
        .len = STRLIT_LEN("Comment value"),
    },
};

static void test_mutable_song_tag_changes(void);
static void test_selected_songs(void);
static void test_parser_row_generation(void);
static void test_filename_parser(void);
static void test_tiny_editor_row_layout(void);
static void test_tiny_editor_extended_tag_rows(void);
static void test_tiny_editor_rejects_streams(void);
static void test_tiny_editor_field_updates(void);
static void test_tiny_editor_filename_stem(void);
static void test_tiny_editor_save_results(void);
static void test_tiny_editor_run_tag_action(void);
static void test_tiny_editor_run_filename_action(void);
static void test_tiny_editor_run_save_and_cancel_actions(void);
static void test_tiny_editor_save_side_effects(void);
static void test_tiny_editor_open_song(void);
static void test_tiny_editor_open_failures(void);

static void init_tiny_editor_song(NcmMutableSong *song, char *name,
                                  int32 name_len);
static void init_tiny_editor_native_song(NcmSong *song, char *uri,
                                         int32 uri_len);
static NcMenu *tiny_editor_menu(NativeTinyTagEditorScreen *screen);
static NcBuffer *tiny_editor_row(NativeTinyTagEditorScreen *screen,
                                 int64 row);
static void assert_tiny_editor_row_text(
    NativeTinyTagEditorScreen *screen, int64 row, char *expected,
    int32 expected_len);
static void assert_tiny_editor_tag_row_text(
    NativeTinyTagEditorScreen *screen, enum NcmTagsField field,
    NcmStringView value);
static bool tiny_editor_write_song(void *user, NcmMutableSong *song,
                                   char *music_dir);
static void tiny_editor_action_fixture_init(
    TinyEditorActionFixture *fixture);
static void tiny_editor_action_fixture_destroy(
    TinyEditorActionFixture *fixture);
static enum NativeTinyTagEditorPromptResult tiny_editor_prompt(
    void *user, char *label, int32 label_len, NcmStringView initial,
    NcmBuffer *result);
static void tiny_editor_status_message(
    void *user, char *message, int32 message_len);
static void tiny_editor_select_row(
    NativeTinyTagEditorScreen *screen, int64 row);
static void tiny_editor_switch_to_screen(void *user, NcScreen *screen);
static bool tiny_editor_action_write_song(
    void *user, NcmMutableSong *song, char *music_dir);
static void tiny_editor_update_directory(
    void *user, char *directory, int32 directory_len);
static void tiny_editor_update_playlist_song(
    void *user, NcmMutableSong *song);
static void tiny_editor_request_browser_update(void *user);
static bool tiny_editor_taglib_open(void *user, NcmTaglibFile *file,
                                    char *path, int32 path_len);
static bool tiny_editor_taglib_audio_properties(
    void *user, NcmTaglibFile *file,
    NcmTaglibAudioProperties *properties);
static bool tiny_editor_taglib_extended_set_supported(
    void *user, NcmTaglibFile *file);
static void tiny_editor_taglib_close(void *user, NcmTaglibFile *file);

int
main(void) {
    test_mutable_song_tag_changes();
    test_selected_songs();
    test_parser_row_generation();
    test_filename_parser();
    test_tiny_editor_row_layout();
    test_tiny_editor_extended_tag_rows();
    test_tiny_editor_rejects_streams();
    test_tiny_editor_field_updates();
    test_tiny_editor_filename_stem();
    test_tiny_editor_save_results();
    test_tiny_editor_run_tag_action();
    test_tiny_editor_run_filename_action();
    test_tiny_editor_run_save_and_cancel_actions();
    test_tiny_editor_save_side_effects();
    test_tiny_editor_open_song();
    test_tiny_editor_open_failures();
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
    assert(STREQUAL(view.data, view.len, LIT_ARGS("New")));

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
    assert(STREQUAL(value.data, value.len, LIT_ARGS("two.flac")));
    assert(ncm_song_tag_view(&songs.items[0], MPD_TAG_ARTIST, 0, &value));
    assert(STREQUAL(value.data, value.len,
                            LIT_ARGS("Original two")));
    assert(ncm_song_duration(&songs.items[0]) == 321);
    assert(ncm_song_mtime(&songs.items[0]) == 1234);

    nc_menu_clear_selection(menu);
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
    assert(native_tag_editor_screen_selected_songs(&screen, &songs));
    assert(songs.len == 1);
    assert(ncm_song_uri_view(&songs.items[0], 0, &value));
    assert(STREQUAL(value.data, value.len, LIT_ARGS("one.flac")));
    assert(ncm_song_tag_view(&songs.items[0], MPD_TAG_ARTIST, 0, &value));
    assert(STREQUAL(value.data, value.len,
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
    assert(STREQUAL(view.data, view.len, LIT_ARGS("Artist")));
    assert(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_TITLE,
                                    0, &view));
    assert(STREQUAL(view.data, view.len, LIT_ARGS("Title")));

    ncm_buffer_destroy(&preview);
    ncm_mutable_song_destroy(&song);
    return;
}

static void
test_tiny_editor_row_layout(void) {
    NativeTinyTagEditorScreen screen;
    NcmMutableSong song;
    NcmTaglibAudioProperties properties = {
        .length = 123,
        .bitrate = 320,
        .sample_rate = 44100,
        .channels = 2,
    };
    NcMenu *menu;

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("song.flac"));
    assert(ncm_mutable_song_set_original_tag(
        &song, NCM_TAGS_FIELD_ARTIST, 1, LIT_ARGS("Second artist")));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(&screen,
                                                                 &song));
    assert(native_tiny_tag_editor_screen_reload_rows(
        &screen, &properties, true, LIT_ARGS(" | "), true));
    menu = tiny_editor_menu(&screen);

    assert(nc_menu_all_item_count(menu)
           == NATIVE_TINY_TAG_EDITOR_ROW_COUNT);
    assert(nc_menu_highlight(menu)
           == NATIVE_TINY_TAG_EDITOR_FIRST_TAG_ROW);
    assert(native_tiny_tag_editor_screen_action_runnable(&screen));

    for (int32 row = NATIVE_TINY_TAG_EDITOR_FILE_NAME_INFO_ROW;
         row < NATIVE_TINY_TAG_EDITOR_FIRST_SEPARATOR_ROW;
         row += 1) {
        assert(nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, row)
               == NC_MENU_ITEM_INACTIVE);
    }
    assert(nc_menu_item_flags_at(
               menu, NC_MENU_ITEMS_ALL,
               NATIVE_TINY_TAG_EDITOR_FIRST_SEPARATOR_ROW)
           == NC_MENU_ITEM_SEPARATOR);
    assert(nc_menu_item_flags_at(
               menu, NC_MENU_ITEMS_ALL,
               NATIVE_TINY_TAG_EDITOR_SECOND_SEPARATOR_ROW)
           == NC_MENU_ITEM_SEPARATOR);
    assert(nc_menu_item_flags_at(
               menu, NC_MENU_ITEMS_ALL,
               NATIVE_TINY_TAG_EDITOR_THIRD_SEPARATOR_ROW)
           == NC_MENU_ITEM_SEPARATOR);

    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        assert(nc_menu_item_flags_at(
                   menu, NC_MENU_ITEMS_ALL,
                   NATIVE_TINY_TAG_EDITOR_TAG_ROW(field))
               == NC_MENU_ITEM_SELECTABLE);
    }
    assert(nc_menu_item_flags_at(
               menu, NC_MENU_ITEMS_ALL,
               NATIVE_TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW)
           == NC_MENU_ITEM_SELECTABLE);
    assert(nc_menu_item_flags_at(
               menu, NC_MENU_ITEMS_ALL,
               NATIVE_TINY_TAG_EDITOR_SAVE_ROW)
           == NC_MENU_ITEM_SELECTABLE);
    assert(nc_menu_item_flags_at(
               menu, NC_MENU_ITEMS_ALL,
               NATIVE_TINY_TAG_EDITOR_CANCEL_ROW)
           == NC_MENU_ITEM_SELECTABLE);

    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_FILE_NAME_INFO_ROW,
        LIT_ARGS("Filename: song.flac"));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_DIRECTORY_INFO_ROW,
        LIT_ARGS("Directory: album"));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_UNUSED_INFO_ROW, LIT_ARGS(""));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_LENGTH_INFO_ROW,
        LIT_ARGS("Length: 2:03"));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_BITRATE_INFO_ROW,
        LIT_ARGS("Bitrate: 320 kbps"));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_SAMPLE_RATE_INFO_ROW,
        LIT_ARGS("Sample rate: 44100 Hz"));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_CHANNELS_INFO_ROW,
        LIT_ARGS("Channels: Stereo"));

    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        if (field == NCM_TAGS_FIELD_ARTIST) {
            assert_tiny_editor_row_text(
                &screen, NATIVE_TINY_TAG_EDITOR_TAG_ROW(field),
                LIT_ARGS("Artist: Artist value | Second artist"));
        } else {
            assert_tiny_editor_tag_row_text(
                &screen, (enum NcmTagsField)field,
                tiny_tag_display_values[field]);
        }
    }
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW,
        LIT_ARGS("Filename: song.flac"));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_SAVE_ROW, LIT_ARGS("Save"));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_CANCEL_ROW, LIT_ARGS("Cancel"));

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_tiny_editor_extended_tag_rows(void) {
    NativeTinyTagEditorScreen screen;
    NcmMutableSong song;
    NcmTaglibAudioProperties properties = {0};
    NcMenu *menu;

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("song.flac"));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(&screen,
                                                                 &song));
    assert(native_tiny_tag_editor_screen_reload_rows(
        &screen, &properties, false, LIT_ARGS(" | "), true));
    menu = tiny_editor_menu(&screen);

    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        uint32 expected_flags;

        expected_flags = NC_MENU_ITEM_SELECTABLE;
        if ((field == NCM_TAGS_FIELD_ALBUM_ARTIST)
            || (field == NCM_TAGS_FIELD_COMPOSER)
            || (field == NCM_TAGS_FIELD_PERFORMER)
            || (field == NCM_TAGS_FIELD_DISC)) {
            expected_flags = NC_MENU_ITEM_INACTIVE;
        }
        assert(nc_menu_item_flags_at(
                   menu, NC_MENU_ITEMS_ALL,
                   NATIVE_TINY_TAG_EDITOR_TAG_ROW(field))
               == expected_flags);
    }

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_tiny_editor_rejects_streams(void) {
    NativeTinyTagEditorScreen screen;
    NcmMutableSong mutable_song;
    NcmSong song;

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("https://example.com/live")));
    assert(!native_tiny_tag_editor_screen_set_edited_song(&screen, &song));
    assert(native_tiny_tag_editor_screen_edited(&screen) == NULL);
    ncm_song_destroy(&song);

    ncm_mutable_song_init(&mutable_song);
    assert(ncm_mutable_song_set_uri(
        &mutable_song, LIT_ARGS("http://example.com/live")));
    assert(!native_tiny_tag_editor_screen_set_edited_mutable_song(
        &screen, &mutable_song));
    assert(native_tiny_tag_editor_screen_edited(&screen) == NULL);

    ncm_mutable_song_destroy(&mutable_song);
    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_tiny_editor_field_updates(void) {
    NativeTinyTagEditorScreen screen;
    NcmMutableSong song;
    NcmStringView view;

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("old.flac"));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(&screen,
                                                                 &song));

    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        assert(native_tiny_tag_editor_screen_set_tag_value(
            &screen, (enum NcmTagsField)field,
            LIT_ARGS("Updated value"), NULL, 0));
        assert(ncm_mutable_song_get_tag(
            native_tiny_tag_editor_screen_edited(&screen),
            (enum NcmTagsField)field, 0, &view));
        assert(STREQUAL(view.data, view.len,
                                LIT_ARGS("Updated value")));
    }
    assert(ncm_mutable_song_is_modified(
        native_tiny_tag_editor_screen_edited(&screen)));

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_tiny_editor_filename_stem(void) {
    NativeTinyTagEditorScreen screen;
    NcmMutableSong song;
    NcmStringView view;

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("old.flac"));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(&screen,
                                                                 &song));
    assert(native_tiny_tag_editor_screen_set_filename_stem(
        &screen, LIT_ARGS("new")));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &view));
    assert(STREQUAL(view.data, view.len, LIT_ARGS("new.flac")));

    assert(native_tiny_tag_editor_screen_set_filename_stem(
        &screen, LIT_ARGS("final")));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &view));
    assert(STREQUAL(view.data, view.len, LIT_ARGS("final.flac")));
    assert(!native_tiny_tag_editor_screen_set_filename_stem(
        &screen, LIT_ARGS("")));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &view));
    assert(STREQUAL(view.data, view.len, LIT_ARGS("final.flac")));

    ncm_mutable_song_destroy(&song);
    ncm_mutable_song_init(&song);
    assert(ncm_mutable_song_set_uri(&song, LIT_ARGS("README")));
    assert(ncm_mutable_song_set_name(&song, LIT_ARGS("README")));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(&screen,
                                                                 &song));
    assert(native_tiny_tag_editor_screen_set_filename_stem(
        &screen, LIT_ARGS("COPY")));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &view));
    assert(STREQUAL(view.data, view.len, LIT_ARGS("COPY")));

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_tiny_editor_save_results(void) {
    NativeTinyTagEditorScreen screen;
    NativeTinyTagEditorHooks hooks = {0};
    TinyEditorWriteFixture fixture;
    NcmMutableSong song;
    NcmMutableSong *edited;
    char *music_dir;

    fixture.music_dir = NULL;
    fixture.calls = 0;
    fixture.result = false;
    music_dir = "/music";

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("song.flac"));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(&screen,
                                                                 &song));
    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE, LIT_ARGS("Unsaved"), NULL, 0));
    hooks.write_song = tiny_editor_write_song;
    hooks.user = &fixture;
    native_tiny_tag_editor_screen_set_hooks(&screen, hooks);

    edited = native_tiny_tag_editor_screen_edited(&screen);
    assert(edited != NULL);
    assert(ncm_mutable_song_is_modified(edited));
    assert(!native_tiny_tag_editor_screen_save(&screen, music_dir));
    assert(fixture.calls == 1);
    assert(fixture.music_dir == music_dir);
    assert(ncm_mutable_song_is_modified(edited));

    fixture.result = true;
    assert(native_tiny_tag_editor_screen_save(&screen, music_dir));
    assert(fixture.calls == 2);
    assert(!ncm_mutable_song_is_modified(edited));

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    return;
}

static void
test_tiny_editor_run_tag_action(void) {
    NativeTinyTagEditorScreen screen;
    NativeTinyTagEditorHooks hooks = {0};
    TinyEditorActionFixture fixture;
    NcmTaglibAudioProperties properties = {0};
    NcmMutableSong song;
    NcmStringView value;
    NcScreen *base;

    tiny_editor_action_fixture_init(&fixture);
    fixture.prompt_result = NATIVE_TINY_TAG_EDITOR_PROMPT_ACCEPTED;
    assert(ncm_buffer_set(&fixture.prompt_response,
                          LIT_ARGS("New title")));

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("song.flac"));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(
        &screen, &song));
    assert(native_tiny_tag_editor_screen_reload_rows(
        &screen, &properties, true, LIT_ARGS(" | "), true));
    hooks.prompt = tiny_editor_prompt;
    hooks.status_message = tiny_editor_status_message;
    hooks.user = &fixture;
    native_tiny_tag_editor_screen_set_hooks(&screen, hooks);
    tiny_editor_select_row(
        &screen, NATIVE_TINY_TAG_EDITOR_TAG_ROW(NCM_TAGS_FIELD_TITLE));

    base = native_tiny_tag_editor_screen_base(&screen);
    assert(nc_screen_active_window(base) == &screen.window);
    assert(STREQUAL(nc_screen_title(base),
                            STRLIT_LEN("Tiny tag editor"),
                            LIT_ARGS("Tiny tag editor")));
    assert(nc_screen_can_run_current(base));
    assert(nc_screen_run_current(base));
    assert(fixture.prompt_calls == 1);
    assert(STREQUAL(fixture.prompt_label.data,
                            fixture.prompt_label.len,
                            LIT_ARGS("Title")));
    assert(STREQUAL(fixture.prompt_initial.data,
                            fixture.prompt_initial.len,
                            LIT_ARGS("Title value")));
    assert(ncm_mutable_song_get_tag(
        native_tiny_tag_editor_screen_edited(&screen),
        NCM_TAGS_FIELD_TITLE, 0, &value));
    assert(STREQUAL(value.data, value.len,
                            LIT_ARGS("New title")));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_TAG_ROW(NCM_TAGS_FIELD_TITLE),
        LIT_ARGS("Title: New title"));

    tiny_editor_select_row(
        &screen, NATIVE_TINY_TAG_EDITOR_TAG_ROW(NCM_TAGS_FIELD_ARTIST));
    fixture.prompt_result = NATIVE_TINY_TAG_EDITOR_PROMPT_ABORTED;
    assert(!nc_screen_run_current(base));
    assert(fixture.prompt_calls == 2);
    assert(fixture.status_calls == 1);
    assert(STREQUAL(fixture.status_messages[0].data,
                            fixture.status_messages[0].len,
                            LIT_ARGS("Action aborted")));
    assert(ncm_mutable_song_get_tag(
        native_tiny_tag_editor_screen_edited(&screen),
        NCM_TAGS_FIELD_ARTIST, 0, &value));
    assert(STREQUAL(value.data, value.len,
                            LIT_ARGS("Artist value")));

    nc_menu_highlight_position(
        tiny_editor_menu(&screen),
        NATIVE_TINY_TAG_EDITOR_FIRST_SEPARATOR_ROW, screen.main_height);
    assert(!nc_screen_can_run_current(base));
    assert(!nc_screen_run_current(base));

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    tiny_editor_action_fixture_destroy(&fixture);
    return;
}

static void
test_tiny_editor_run_filename_action(void) {
    NativeTinyTagEditorScreen screen;
    NativeTinyTagEditorHooks hooks = {0};
    TinyEditorActionFixture fixture;
    NcmTaglibAudioProperties properties = {0};
    NcmMutableSong song;
    NcmStringView name;

    tiny_editor_action_fixture_init(&fixture);
    fixture.prompt_result = NATIVE_TINY_TAG_EDITOR_PROMPT_ACCEPTED;
    assert(ncm_buffer_set(&fixture.prompt_response,
                          LIT_ARGS("renamed")));

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("song.flac"));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(
        &screen, &song));
    assert(native_tiny_tag_editor_screen_reload_rows(
        &screen, &properties, true, LIT_ARGS(" | "), true));
    hooks.prompt = tiny_editor_prompt;
    hooks.user = &fixture;
    native_tiny_tag_editor_screen_set_hooks(&screen, hooks);
    tiny_editor_select_row(
        &screen, NATIVE_TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW);

    assert(native_tiny_tag_editor_screen_run_current(&screen));
    assert(STREQUAL(fixture.prompt_label.data,
                            fixture.prompt_label.len,
                            LIT_ARGS("Filename")));
    assert(STREQUAL(fixture.prompt_initial.data,
                            fixture.prompt_initial.len,
                            LIT_ARGS("song")));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &name));
    assert(STREQUAL(name.data, name.len,
                            LIT_ARGS("renamed.flac")));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW,
        LIT_ARGS("Filename: renamed.flac"));

    ncm_buffer_clear(&fixture.prompt_response);
    assert(native_tiny_tag_editor_screen_run_current(&screen));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &name));
    assert(STREQUAL(name.data, name.len,
                            LIT_ARGS("renamed.flac")));

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    tiny_editor_action_fixture_destroy(&fixture);
    return;
}

static void
test_tiny_editor_run_save_and_cancel_actions(void) {
    NativeTinyTagEditorScreen screen;
    NativeTinyTagEditorHooks hooks = {0};
    TinyEditorActionFixture fixture;
    NcmTaglibAudioProperties properties = {0};
    NcmMutableSong song;
    NcScreen previous = {0};

    tiny_editor_action_fixture_init(&fixture);
    fixture.write_result = true;
    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("song.flac"));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(
        &screen, &song));
    assert(native_tiny_tag_editor_screen_reload_rows(
        &screen, &properties, true, LIT_ARGS(" | "), true));
    assert(ncm_buffer_set(&screen.music_dir, LIT_ARGS("/music")));
    screen.previous_screen = &previous;
    ncm_mutable_song_set_from_database(&screen.edited, true);
    hooks.status_message = tiny_editor_status_message;
    hooks.write_song = tiny_editor_action_write_song;
    hooks.update_directory = tiny_editor_update_directory;
    hooks.switch_to_screen = tiny_editor_switch_to_screen;
    hooks.user = &fixture;
    native_tiny_tag_editor_screen_set_hooks(&screen, hooks);

    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE, LIT_ARGS("Changed"), NULL, 0));
    assert(native_tiny_tag_editor_screen_run_row(
        &screen, NATIVE_TINY_TAG_EDITOR_SAVE_ROW));
    assert(fixture.write_calls == 1);
    assert(fixture.music_dir == screen.music_dir.data);
    assert(fixture.status_calls == 2);
    assert(STREQUAL(fixture.status_messages[0].data,
                            fixture.status_messages[0].len,
                            LIT_ARGS("Updating tags...")));
    assert(STREQUAL(fixture.status_messages[1].data,
                            fixture.status_messages[1].len,
                            LIT_ARGS("Tags updated")));
    assert(fixture.switch_calls == 1);
    assert(fixture.switched_screen == &previous);
    assert(fixture.update_directory_calls == 1);
    assert(STREQUAL(fixture.updated_directory.data,
                            fixture.updated_directory.len,
                            LIT_ARGS("album")));
    assert(fixture.status_calls_at_side_effect == 2);
    assert(fixture.switch_calls_at_side_effect == 0);
    assert(!ncm_mutable_song_is_modified(
        native_tiny_tag_editor_screen_edited(&screen)));

    fixture.write_result = false;
    ncm_mutable_song_set_from_database(&screen.edited, false);
    errno = EACCES;
    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE, LIT_ARGS("Unsaved"), NULL, 0));
    assert(native_tiny_tag_editor_screen_run_row(
        &screen, NATIVE_TINY_TAG_EDITOR_SAVE_ROW));
    assert(fixture.write_calls == 2);
    assert(fixture.status_calls == 4);
    assert(STREQUAL(
        fixture.status_messages[2].data,
        fixture.status_messages[2].len,
        LIT_ARGS("Updating tags...")));
    assert(ncm_string_starts_with(
        fixture.status_messages[3].data,
        fixture.status_messages[3].len,
        LIT_ARGS("Error while writing tags: ")));
    assert(fixture.switch_calls == 2);
    assert(fixture.update_directory_calls == 1);
    assert(fixture.update_playlist_calls == 0);
    assert(fixture.browser_update_calls == 0);
    assert(ncm_mutable_song_is_modified(
        native_tiny_tag_editor_screen_edited(&screen)));

    assert(native_tiny_tag_editor_screen_run_row(
        &screen, NATIVE_TINY_TAG_EDITOR_CANCEL_ROW));
    assert(fixture.write_calls == 2);
    assert(fixture.switch_calls == 3);

    screen.previous_screen = NULL;
    assert(native_tiny_tag_editor_screen_run_row(
        &screen, NATIVE_TINY_TAG_EDITOR_CANCEL_ROW));
    assert(fixture.write_calls == 2);
    assert(fixture.switch_calls == 3);

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    tiny_editor_action_fixture_destroy(&fixture);
    return;
}

static void
test_tiny_editor_save_side_effects(void) {
    NativeTinyTagEditorScreen screen;
    NativeTinyTagEditorHooks hooks = {0};
    TinyEditorActionFixture fixture;
    NcmMutableSong song;
    NcScreen previous = {0};

    tiny_editor_action_fixture_init(&fixture);
    fixture.write_result = true;
    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    init_tiny_editor_song(&song, LIT_ARGS("song.flac"));
    assert(native_tiny_tag_editor_screen_set_edited_mutable_song(
        &screen, &song));
    assert(ncm_buffer_set(&screen.music_dir, LIT_ARGS("/music")));
    hooks.write_song = tiny_editor_action_write_song;
    hooks.update_directory = tiny_editor_update_directory;
    hooks.update_playlist_song = tiny_editor_update_playlist_song;
    hooks.request_browser_update = tiny_editor_request_browser_update;
    hooks.user = &fixture;
    native_tiny_tag_editor_screen_set_hooks(&screen, hooks);

    previous.type = NC_SCREEN_TYPE_PLAYLIST;
    screen.previous_screen = &previous;
    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE,
        LIT_ARGS("Playlist title"), NULL, 0));
    assert(native_tiny_tag_editor_screen_set_filename(
        &screen, LIT_ARGS("renamed.flac")));
    assert(native_tiny_tag_editor_screen_save(
        &screen, screen.music_dir.data));
    assert(fixture.update_directory_calls == 0);
    assert(fixture.update_playlist_calls == 1);
    assert(fixture.browser_update_calls == 0);
    assert(fixture.modified_at_side_effect);
    assert(STREQUAL(fixture.playlist_title.data,
                            fixture.playlist_title.len,
                            LIT_ARGS("Playlist title")));
    assert(STREQUAL(fixture.playlist_new_name.data,
                            fixture.playlist_new_name.len,
                            LIT_ARGS("renamed.flac")));

    previous.type = NC_SCREEN_TYPE_BROWSER;
    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE,
        LIT_ARGS("Browser title"), NULL, 0));
    assert(native_tiny_tag_editor_screen_save(
        &screen, screen.music_dir.data));
    assert(fixture.update_directory_calls == 0);
    assert(fixture.update_playlist_calls == 1);
    assert(fixture.browser_update_calls == 1);

    screen.previous_screen = NULL;
    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE,
        LIT_ARGS("No previous screen"), NULL, 0));
    assert(native_tiny_tag_editor_screen_save(
        &screen, screen.music_dir.data));
    assert(fixture.update_directory_calls == 0);
    assert(fixture.update_playlist_calls == 1);
    assert(fixture.browser_update_calls == 1);

    fixture.write_result = false;
    previous.type = NC_SCREEN_TYPE_BROWSER;
    screen.previous_screen = &previous;
    assert(native_tiny_tag_editor_screen_set_tag_value(
        &screen, NCM_TAGS_FIELD_TITLE,
        LIT_ARGS("Write failed"), NULL, 0));
    assert(!native_tiny_tag_editor_screen_save(
        &screen, screen.music_dir.data));
    assert(fixture.update_directory_calls == 0);
    assert(fixture.update_playlist_calls == 1);
    assert(fixture.browser_update_calls == 1);
    assert(ncm_mutable_song_is_modified(
        native_tiny_tag_editor_screen_edited(&screen)));

    ncm_mutable_song_destroy(&song);
    native_tiny_tag_editor_screen_destroy(&screen);
    tiny_editor_action_fixture_destroy(&fixture);
    return;
}

static void
test_tiny_editor_open_song(void) {
    NativeTinyTagEditorScreen screen;
    NativeTinyTagEditorHooks hooks = {0};
    TinyEditorTaglibFixture fixture = {0};
    NcmMutableSong *edited;
    NcmBuffer path;
    NcmSong song;
    enum NativeTinyTagEditorOpenResult result;

    ncm_buffer_init(&fixture.path);
    fixture.properties.length = 245;
    fixture.properties.bitrate = 320;
    fixture.properties.sample_rate = 48000;
    fixture.properties.channels = 2;
    fixture.open_result = true;
    fixture.audio_result = true;
    fixture.extended_result = false;

    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    hooks.taglib_open = tiny_editor_taglib_open;
    hooks.taglib_audio_properties = tiny_editor_taglib_audio_properties;
    hooks.taglib_extended_set_supported =
        tiny_editor_taglib_extended_set_supported;
    hooks.taglib_close = tiny_editor_taglib_close;
    hooks.user = &fixture;
    native_tiny_tag_editor_screen_set_hooks(&screen, hooks);
    init_tiny_editor_native_song(&song, LIT_ARGS("album/song.flac"));
    ncm_buffer_init(&path);
    result = native_tiny_tag_editor_screen_open_song(
        &screen, &song, LIT_ARGS("/music/"), LIT_ARGS(" | "), true,
        &path);
    assert(result == NATIVE_TINY_TAG_EDITOR_OPEN_SUCCESS);
    assert(STREQUAL(path.data, path.len,
                            LIT_ARGS("/music/album/song.flac")));
    assert(STREQUAL(fixture.path.data, fixture.path.len,
                            path.data, path.len));
    assert(fixture.open_calls == 1);
    assert(fixture.audio_calls == 1);
    assert(fixture.extended_calls == 1);
    assert(fixture.close_calls == 1);
    edited = native_tiny_tag_editor_screen_edited(&screen);
    assert(edited != NULL);
    assert(ncm_mutable_song_duration(edited) == 245);
    assert(ncm_mutable_song_mtime(edited) == 1234);
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_BITRATE_INFO_ROW,
        LIT_ARGS("Bitrate: 320 kbps"));
    assert_tiny_editor_row_text(
        &screen, NATIVE_TINY_TAG_EDITOR_SAMPLE_RATE_INFO_ROW,
        LIT_ARGS("Sample rate: 48000 Hz"));
    assert(nc_menu_item_flags_at(
        tiny_editor_menu(&screen), NC_MENU_ITEMS_ALL,
        NATIVE_TINY_TAG_EDITOR_TAG_ROW(NCM_TAGS_FIELD_ALBUM_ARTIST))
        & NC_MENU_ITEM_INACTIVE);

    ncm_song_destroy(&song);
    ncm_buffer_destroy(&path);
    native_tiny_tag_editor_screen_destroy(&screen);
    ncm_buffer_destroy(&fixture.path);
    return;
}

static void
test_tiny_editor_open_failures(void) {
    NativeTinyTagEditorScreen screen;
    NativeTinyTagEditorHooks hooks = {0};
    TinyEditorTaglibFixture fixture = {0};
    NcmBuffer path;
    NcmSong song;
    enum NativeTinyTagEditorOpenResult result;

    ncm_buffer_init(&fixture.path);
    fixture.open_result = true;
    fixture.audio_result = true;
    fixture.extended_result = true;
    native_tiny_tag_editor_screen_init(&screen, 0, 80, 0, 24,
                                       nc_color_default(),
                                       nc_border_none());
    hooks.taglib_open = tiny_editor_taglib_open;
    hooks.taglib_audio_properties = tiny_editor_taglib_audio_properties;
    hooks.taglib_extended_set_supported =
        tiny_editor_taglib_extended_set_supported;
    hooks.taglib_close = tiny_editor_taglib_close;
    hooks.user = &fixture;
    native_tiny_tag_editor_screen_set_hooks(&screen, hooks);
    ncm_buffer_init(&path);

    init_tiny_editor_native_song(&song,
                                 LIT_ARGS("https://example/song.flac"));
    result = native_tiny_tag_editor_screen_open_song(
        &screen, &song, LIT_ARGS("/music/"), LIT_ARGS(" | "), true,
        &path);
    assert(result == NATIVE_TINY_TAG_EDITOR_OPEN_STREAM);
    assert(path.len == 0);
    assert(fixture.open_calls == 0);
    ncm_song_destroy(&song);

    init_tiny_editor_native_song(&song, LIT_ARGS("album/song.flac"));
    result = native_tiny_tag_editor_screen_open_song(
        &screen, &song, NULL, 0, LIT_ARGS(" | "), true, &path);
    assert(result
           == NATIVE_TINY_TAG_EDITOR_OPEN_MISSING_MUSIC_DIRECTORY);
    assert(path.len == 0);
    assert(fixture.open_calls == 0);

    fixture.open_result = false;
    result = native_tiny_tag_editor_screen_open_song(
        &screen, &song, LIT_ARGS("/music/"), LIT_ARGS(" | "), true,
        &path);
    assert(result == NATIVE_TINY_TAG_EDITOR_OPEN_UNREADABLE_FILE);
    assert(STREQUAL(path.data, path.len,
                            LIT_ARGS("/music/album/song.flac")));
    assert(fixture.open_calls == 1);
    assert(fixture.audio_calls == 0);
    assert(fixture.extended_calls == 0);
    assert(fixture.close_calls == 1);
    assert(native_tiny_tag_editor_screen_edited(&screen) == NULL);

    fixture.open_result = true;
    ncm_song_destroy(&song);

    ncm_song_init(&song);
    result = native_tiny_tag_editor_screen_open_song(
        &screen, &song, LIT_ARGS("/music/"), LIT_ARGS(" | "), true,
        &path);
    assert(result == NATIVE_TINY_TAG_EDITOR_OPEN_PREPARE_FAILED);
    assert(fixture.open_calls == 1);
    ncm_song_destroy(&song);

    init_tiny_editor_native_song(&song, LIT_ARGS("/tmp/song.flac"));
    result = native_tiny_tag_editor_screen_open_song(
        &screen, &song, NULL, 0, NULL, 0, true, &path);
    assert(result == NATIVE_TINY_TAG_EDITOR_OPEN_SUCCESS);
    assert(STREQUAL(path.data, path.len,
                            LIT_ARGS("/tmp/song.flac")));
    assert(fixture.open_calls == 2);
    assert(fixture.close_calls == 2);
    ncm_song_destroy(&song);

    ncm_buffer_destroy(&path);
    native_tiny_tag_editor_screen_destroy(&screen);
    ncm_buffer_destroy(&fixture.path);
    return;
}

static void
init_tiny_editor_song(NcmMutableSong *song, char *name, int32 name_len) {
    ncm_mutable_song_init(song);
    assert(ncm_mutable_song_set_uri(song, LIT_ARGS("album/song.flac")));
    assert(ncm_mutable_song_set_directory(song, LIT_ARGS("album")));
    assert(ncm_mutable_song_set_name(song, name, name_len));
    ncm_mutable_song_set_duration(song, 123);

    for (uint32 field = 0; field < NCM_TAGS_FIELD_LAST; field += 1) {
        assert(ncm_mutable_song_set_original_tag(
            song, (enum NcmTagsField)field, 0,
            tiny_tag_values[field].data, tiny_tag_values[field].len));
    }
    return;
}

static void
init_tiny_editor_native_song(NcmSong *song, char *uri,
                             int32 uri_len) {
    ncm_song_init(song);
    assert(ncm_song_set_uri(song, uri, uri_len));
    assert(ncm_song_add_tag(song, MPD_TAG_TITLE,
                            LIT_ARGS("Song title")));
    ncm_song_set_duration(song, 245);
    ncm_song_set_mtime(song, 1234);
    return;
}

static NcMenu *
tiny_editor_menu(NativeTinyTagEditorScreen *screen) {
    return nc_editor_buffer_menu_base(
        native_tiny_tag_editor_screen_rows(screen));
}

static NcBuffer *
tiny_editor_row(NativeTinyTagEditorScreen *screen, int64 row) {
    return nc_editor_buffer_menu_item_at(
        native_tiny_tag_editor_screen_rows(screen), NC_MENU_ITEMS_ALL, row);
}

static void
assert_tiny_editor_row_text(NativeTinyTagEditorScreen *screen, int64 row,
                            char *expected, int32 expected_len) {
    NcBuffer *buffer;

    buffer = tiny_editor_row(screen, row);
    assert(buffer != NULL);
    if (!STREQUAL(buffer->data, buffer->len,
                          expected, expected_len)) {
        fprintf(stderr,
                "tiny editor row %lld mismatch: expected [%.*s], "
                "actual [%.*s]\n",
                (llong)row, expected_len, expected,
                buffer->len, buffer->data);
        assert(false);
    }
    return;
}

static void
assert_tiny_editor_tag_row_text(NativeTinyTagEditorScreen *screen,
                                enum NcmTagsField field,
                                NcmStringView value) {
    NcmBuffer expected;

    ncm_buffer_init(&expected);
    ncm_buffer_append(&expected, tiny_tag_names[field].data,
                      tiny_tag_names[field].len);
    ncm_buffer_append(&expected, STRLIT_ARGS(": "));
    ncm_buffer_append(&expected, value.data, value.len);
    assert_tiny_editor_row_text(
        screen, NATIVE_TINY_TAG_EDITOR_TAG_ROW(field),
        expected.data, expected.len);
    ncm_buffer_destroy(&expected);
    return;
}

static bool
tiny_editor_taglib_open(void *user, NcmTaglibFile *file, char *path,
                        int32 path_len) {
    TinyEditorTaglibFixture *fixture;

    fixture = user;
    fixture->open_calls += 1;
    assert(ncm_buffer_set(&fixture->path, path, path_len));
    if (fixture->open_result) {
        file->handle = fixture;
    }
    return fixture->open_result;
}

static bool
tiny_editor_taglib_audio_properties(
    void *user, NcmTaglibFile *file,
    NcmTaglibAudioProperties *properties) {
    TinyEditorTaglibFixture *fixture;

    fixture = user;
    fixture->audio_calls += 1;
    assert(file->handle == fixture);
    if (fixture->audio_result) {
        *properties = fixture->properties;
    }
    return fixture->audio_result;
}

static bool
tiny_editor_taglib_extended_set_supported(
    void *user, NcmTaglibFile *file) {
    TinyEditorTaglibFixture *fixture;

    fixture = user;
    fixture->extended_calls += 1;
    assert(file->handle == fixture);
    return fixture->extended_result;
}

static void
tiny_editor_taglib_close(void *user, NcmTaglibFile *file) {
    TinyEditorTaglibFixture *fixture;

    fixture = user;
    fixture->close_calls += 1;
    file->handle = NULL;
    return;
}

static void
tiny_editor_action_fixture_init(TinyEditorActionFixture *fixture) {
    ncm_buffer_init(&fixture->prompt_label);
    ncm_buffer_init(&fixture->prompt_initial);
    ncm_buffer_init(&fixture->prompt_response);
    ncm_buffer_init(&fixture->updated_directory);
    ncm_buffer_init(&fixture->playlist_title);
    ncm_buffer_init(&fixture->playlist_new_name);
    for (int32 i = 0; i < 4; i += 1) {
        ncm_buffer_init(&fixture->status_messages[i]);
    }
    fixture->switched_screen = NULL;
    fixture->music_dir = NULL;
    fixture->prompt_calls = 0;
    fixture->status_calls = 0;
    fixture->switch_calls = 0;
    fixture->write_calls = 0;
    fixture->update_directory_calls = 0;
    fixture->update_playlist_calls = 0;
    fixture->browser_update_calls = 0;
    fixture->status_calls_at_side_effect = -1;
    fixture->switch_calls_at_side_effect = -1;
    fixture->prompt_result = NATIVE_TINY_TAG_EDITOR_PROMPT_ERROR;
    fixture->write_result = false;
    fixture->modified_at_side_effect = false;
    return;
}

static void
tiny_editor_action_fixture_destroy(TinyEditorActionFixture *fixture) {
    ncm_buffer_destroy(&fixture->prompt_label);
    ncm_buffer_destroy(&fixture->prompt_initial);
    ncm_buffer_destroy(&fixture->prompt_response);
    ncm_buffer_destroy(&fixture->updated_directory);
    ncm_buffer_destroy(&fixture->playlist_title);
    ncm_buffer_destroy(&fixture->playlist_new_name);
    for (int32 i = 0; i < 4; i += 1) {
        ncm_buffer_destroy(&fixture->status_messages[i]);
    }
    return;
}

static enum NativeTinyTagEditorPromptResult
tiny_editor_prompt(
    void *user, char *label, int32 label_len, NcmStringView initial,
    NcmBuffer *result) {
    TinyEditorActionFixture *fixture;

    fixture = user;
    fixture->prompt_calls += 1;
    assert(ncm_buffer_set(&fixture->prompt_label, label, label_len));
    assert(ncm_buffer_set(&fixture->prompt_initial,
                          initial.data, initial.len));
    if (fixture->prompt_result
        == NATIVE_TINY_TAG_EDITOR_PROMPT_ACCEPTED) {
        assert(ncm_buffer_copy(result, &fixture->prompt_response));
    }
    return fixture->prompt_result;
}

static void
tiny_editor_status_message(
    void *user, char *message, int32 message_len) {
    TinyEditorActionFixture *fixture;

    fixture = user;
    assert(fixture->status_calls < 4);
    assert(ncm_buffer_set(
        &fixture->status_messages[fixture->status_calls],
        message, message_len));
    fixture->status_calls += 1;
    return;
}

static void
tiny_editor_select_row(NativeTinyTagEditorScreen *screen, int64 row) {
    nc_menu_highlight_position(tiny_editor_menu(screen), row,
                               screen->main_height);
    assert(nc_menu_highlight(tiny_editor_menu(screen)) == row);
    return;
}

static void
tiny_editor_switch_to_screen(void *user, NcScreen *screen) {
    TinyEditorActionFixture *fixture;

    fixture = user;
    fixture->switched_screen = screen;
    fixture->switch_calls += 1;
    return;
}

static bool
tiny_editor_action_write_song(
    void *user, NcmMutableSong *song, char *music_dir) {
    TinyEditorActionFixture *fixture;

    fixture = user;
    assert(song != NULL);
    fixture->music_dir = music_dir;
    fixture->write_calls += 1;
    return fixture->write_result;
}

static void
tiny_editor_update_directory(
    void *user, char *directory, int32 directory_len) {
    TinyEditorActionFixture *fixture;

    fixture = user;
    assert(ncm_buffer_set(&fixture->updated_directory,
                          directory, directory_len));
    fixture->update_directory_calls += 1;
    fixture->status_calls_at_side_effect = fixture->status_calls;
    fixture->switch_calls_at_side_effect = fixture->switch_calls;
    fixture->modified_at_side_effect = true;
    return;
}

static void
tiny_editor_update_playlist_song(
    void *user, NcmMutableSong *song) {
    TinyEditorActionFixture *fixture;
    NcmStringView new_name;
    NcmStringView title;

    fixture = user;
    assert(song != NULL);
    assert(ncm_mutable_song_get_tag(
        song, NCM_TAGS_FIELD_TITLE, 0, &title));
    assert(ncm_buffer_set(&fixture->playlist_title,
                          title.data, title.len));
    ncm_buffer_clear(&fixture->playlist_new_name);
    if (ncm_mutable_song_get_new_name(song, &new_name)) {
        assert(ncm_buffer_set(&fixture->playlist_new_name,
                              new_name.data, new_name.len));
    }
    fixture->update_playlist_calls += 1;
    fixture->status_calls_at_side_effect = fixture->status_calls;
    fixture->switch_calls_at_side_effect = fixture->switch_calls;
    fixture->modified_at_side_effect = ncm_mutable_song_is_modified(song);
    return;
}

static void
tiny_editor_request_browser_update(void *user) {
    TinyEditorActionFixture *fixture;

    fixture = user;
    fixture->browser_update_calls += 1;
    fixture->status_calls_at_side_effect = fixture->status_calls;
    fixture->switch_calls_at_side_effect = fixture->switch_calls;
    return;
}

static bool
tiny_editor_write_song(void *user, NcmMutableSong *song,
                       char *music_dir) {
    TinyEditorWriteFixture *fixture;

    fixture = user;
    fixture->calls += 1;
    assert(song != NULL);
    assert(music_dir != NULL);
    fixture->music_dir = music_dir;
    return fixture->result;
}
