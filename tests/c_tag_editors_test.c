#include <assert.h>
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

typedef struct TinyEditorWriteFixture {
    char *music_dir;
    int32 calls;
    bool result;
} TinyEditorWriteFixture;

static NcmStringView tiny_tag_names[NCM_TAGS_FIELD_LAST] = {
    { .data = (char *)"Title", .len = STRLIT_LEN("Title") },
    { .data = (char *)"Artist", .len = STRLIT_LEN("Artist") },
    {
        .data = (char *)"Album Artist",
        .len = STRLIT_LEN("Album Artist"),
    },
    { .data = (char *)"Album", .len = STRLIT_LEN("Album") },
    { .data = (char *)"Date", .len = STRLIT_LEN("Date") },
    { .data = (char *)"Track", .len = STRLIT_LEN("Track") },
    { .data = (char *)"Genre", .len = STRLIT_LEN("Genre") },
    { .data = (char *)"Composer", .len = STRLIT_LEN("Composer") },
    { .data = (char *)"Performer", .len = STRLIT_LEN("Performer") },
    { .data = (char *)"Disc", .len = STRLIT_LEN("Disc") },
    { .data = (char *)"Comment", .len = STRLIT_LEN("Comment") },
};

static NcmStringView tiny_tag_values[NCM_TAGS_FIELD_LAST] = {
    { .data = (char *)"Title value", .len = STRLIT_LEN("Title value") },
    { .data = (char *)"Artist value", .len = STRLIT_LEN("Artist value") },
    {
        .data = (char *)"Album artist value",
        .len = STRLIT_LEN("Album artist value"),
    },
    { .data = (char *)"Album value", .len = STRLIT_LEN("Album value") },
    { .data = (char *)"2026", .len = STRLIT_LEN("2026") },
    { .data = (char *)"3", .len = STRLIT_LEN("3") },
    { .data = (char *)"Genre value", .len = STRLIT_LEN("Genre value") },
    {
        .data = (char *)"Composer value",
        .len = STRLIT_LEN("Composer value"),
    },
    {
        .data = (char *)"Performer value",
        .len = STRLIT_LEN("Performer value"),
    },
    { .data = (char *)"2", .len = STRLIT_LEN("2") },
    {
        .data = (char *)"Comment value",
        .len = STRLIT_LEN("Comment value"),
    },
};

static NcmStringView tiny_tag_display_values[NCM_TAGS_FIELD_LAST] = {
    { .data = (char *)"Title value", .len = STRLIT_LEN("Title value") },
    { .data = (char *)"Artist value", .len = STRLIT_LEN("Artist value") },
    {
        .data = (char *)"Album artist value",
        .len = STRLIT_LEN("Album artist value"),
    },
    { .data = (char *)"Album value", .len = STRLIT_LEN("Album value") },
    { .data = (char *)"2026", .len = STRLIT_LEN("2026") },
    { .data = (char *)"03", .len = STRLIT_LEN("03") },
    { .data = (char *)"Genre value", .len = STRLIT_LEN("Genre value") },
    {
        .data = (char *)"Composer value",
        .len = STRLIT_LEN("Composer value"),
    },
    {
        .data = (char *)"Performer value",
        .len = STRLIT_LEN("Performer value"),
    },
    { .data = (char *)"2", .len = STRLIT_LEN("2") },
    {
        .data = (char *)"Comment value",
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

static void init_tiny_editor_song(NcmMutableSong *song, char *name,
                                  int32 name_len);
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
    nc_menu_highlight_position(menu, 0, nc_menu_item_count(menu));
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
        assert(ncm_string_equal(view.data, view.len,
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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("new.flac")));

    assert(native_tiny_tag_editor_screen_set_filename_stem(
        &screen, LIT_ARGS("final")));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("final.flac")));
    assert(!native_tiny_tag_editor_screen_set_filename_stem(
        &screen, LIT_ARGS("")));
    assert(ncm_mutable_song_get_new_name(
        native_tiny_tag_editor_screen_edited(&screen), &view));
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("final.flac")));

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
    assert(ncm_string_equal(view.data, view.len, LIT_ARGS("COPY")));

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
    music_dir = (char *)"/music";

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
    if (!ncm_string_equal(buffer->data, buffer->len,
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
