#include <stdio.h>
#include <stdlib.h>

#include "c/ncm_base.h"
#include "c/ncm_directory.h"
#include "c/ncm_mpd_connection.h"
#include "c/ncm_mpd_item.h"
#include "c/ncm_playlist.h"
#include "c/ncm_song.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define REQUIRE(COND) do {                                                \
    if (!(COND)) {                                                        \
        fail(__FILE__, __LINE__, (char *)#COND);                         \
    }                                                                     \
} while (0)

#define REQUIRE_INT(ACTUAL, EXPECTED)                                     \
    require_int(__FILE__, __LINE__, (char *)#ACTUAL, ACTUAL, EXPECTED)

#define REQUIRE_STRING(ACTUAL, ACTUAL_LEN, EXPECTED)                      \
    require_string(__FILE__, __LINE__, (char *)#ACTUAL,                   \
                   ACTUAL, ACTUAL_LEN, LIT_ARGS(EXPECTED))

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static void test_directory(void);
static void test_playlist(void);
static void test_song_empty_copy(void);
static void test_song_storage_copy(void);
static void test_song_uri_helpers(void);
static void test_song_format_helpers(void);
static void test_song_getter_buffers_and_hash(void);
static void test_item_unknown(void);
static void test_item_directory_copy(void);
static void test_item_playlist_replacement(void);
static void test_item_empty_song_copy(void);
static void test_item_song_null_constructors(void);
static void test_mpd_song_list_copy_and_array(void);
static void test_mpd_item_list_directory_array(void);
static void test_mpd_string_output_and_playlist_lists(void);

static void
fail(char *file, int32 line, char *condition) {
    fprintf(stderr, "%s:%d: requirement failed: %s\n",
            file, line, condition);
    exit(EXIT_FAILURE);
}

static void
require_int(char *file, int32 line, char *name,
            int32 actual, int32 expected) {
    if (actual != expected) {
        fprintf(stderr, "%s:%d: %s: expected %d, got %d\n",
                file, line, name, expected, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
require_string(char *file, int32 line, char *name,
               char *actual, int32 actual_len,
               char *expected, int32 expected_len) {
    if (!STREQUAL(actual, actual_len, expected, expected_len)) {
        fprintf(stderr, "%s:%d: %s: expected '%.*s', got '%.*s'\n",
                file, line, name,
                expected_len, expected, actual_len, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
test_directory(void) {
    NcmDirectory directory;
    NcmDirectory copy;
    NcmDirectory moved;
    NcmStringView view;

    ncm_directory_init(&directory);
    ncm_directory_init(&copy);
    ncm_directory_init(&moved);

    REQUIRE(ncm_directory_set(&directory, LIT_ARGS("Jazz/Live"), 123));
    REQUIRE_STRING(directory.path, directory.path_len, "Jazz/Live");
    REQUIRE_INT((int32)directory.last_modified, 123);
    REQUIRE(ncm_directory_path_view(&directory, &view));
    REQUIRE_STRING(view.data, view.len, "Jazz/Live");
    REQUIRE_INT((int32)ncm_directory_last_modified(&directory), 123);

    REQUIRE(ncm_directory_copy(&copy, &directory));
    REQUIRE_STRING(copy.path, copy.path_len, "Jazz/Live");
    REQUIRE_INT((int32)copy.last_modified, 123);
    REQUIRE(ncm_directory_equal(&copy, &directory));

    REQUIRE(ncm_directory_set(&directory, LIT_ARGS("Other"), 456));
    REQUIRE_STRING(directory.path, directory.path_len, "Other");
    REQUIRE_STRING(copy.path, copy.path_len, "Jazz/Live");
    REQUIRE(!ncm_directory_equal(&copy, &directory));

    ncm_directory_move(&moved, &copy);
    REQUIRE(ncm_directory_path_view(&moved, &view));
    REQUIRE_STRING(view.data, view.len, "Jazz/Live");
    REQUIRE(!ncm_directory_path_view(&copy, &view));

    ncm_directory_destroy(&moved);
    ncm_directory_destroy(&copy);
    ncm_directory_destroy(&directory);
    return;
}

static void
test_playlist(void) {
    NcmPlaylist playlist;
    NcmPlaylist copy;

    ncm_playlist_init(&playlist);
    ncm_playlist_init(&copy);

    REQUIRE(ncm_playlist_set(&playlist, LIT_ARGS("favorites"), 321));
    REQUIRE_STRING(playlist.path, playlist.path_len, "favorites");
    REQUIRE_INT((int32)playlist.last_modified, 321);

    REQUIRE(ncm_playlist_copy(&copy, &playlist));
    REQUIRE_STRING(copy.path, copy.path_len, "favorites");
    REQUIRE_INT((int32)copy.last_modified, 321);

    REQUIRE(ncm_playlist_set(&playlist, LIT_ARGS("recent"), 654));
    REQUIRE_STRING(playlist.path, playlist.path_len, "recent");
    REQUIRE_STRING(copy.path, copy.path_len, "favorites");

    ncm_playlist_destroy(&copy);
    ncm_playlist_destroy(&playlist);
    return;
}

static void
test_song_empty_copy(void) {
    NcmSong song;
    NcmSong copy;

    ncm_song_init(&song);
    ncm_song_init(&copy);

    REQUIRE(ncm_song_empty(&song));
    REQUIRE(!ncm_song_owns_mpd_song(&song));
    REQUIRE(!ncm_song_borrows_mpd_song(&song));
    REQUIRE(ncm_song_copy(&copy, &song));
    REQUIRE(ncm_song_empty(&copy));
    REQUIRE(!ncm_song_owns_mpd_song(&copy));
    REQUIRE(!ncm_song_borrows_mpd_song(&copy));
    REQUIRE(ncm_song_mpd_song(&copy) == NULL);
    REQUIRE(ncm_song_dup_mpd_song(&copy) == NULL);

    ncm_song_destroy(&copy);
    ncm_song_destroy(&song);
    return;
}



static void
test_song_storage_copy(void) {
    NcmSong song;
    NcmSong copy;
    NcmStringView view;

    ncm_song_init(&song);
    ncm_song_init(&copy);

    REQUIRE(ncm_song_set_uri(&song, LIT_ARGS("dir/song.flac")));
    REQUIRE(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("One")));
    REQUIRE(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Two")));
    REQUIRE(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Title")));
    ncm_song_set_duration(&song, 123);
    ncm_song_set_position(&song, 7);
    ncm_song_set_id(&song, 8);
    ncm_song_set_priority(&song, 9);
    ncm_song_set_mtime(&song, 10);

    REQUIRE(ncm_song_copy(&copy, &song));
    REQUIRE(!ncm_song_empty(&copy));
    REQUIRE(ncm_song_uri_view(&copy, 0, &view));
    REQUIRE_STRING(view.data, view.len, "dir/song.flac");
    REQUIRE(ncm_song_tag_view(&copy, MPD_TAG_ARTIST, 0, &view));
    REQUIRE_STRING(view.data, view.len, "One");
    REQUIRE(ncm_song_tag_view(&copy, MPD_TAG_ARTIST, 1, &view));
    REQUIRE_STRING(view.data, view.len, "Two");
    REQUIRE(ncm_song_tag_view(&copy, MPD_TAG_TITLE, 0, &view));
    REQUIRE_STRING(view.data, view.len, "Title");
    REQUIRE_INT((int32)ncm_song_duration(&copy), 123);
    REQUIRE_INT((int32)ncm_song_position(&copy), 7);
    REQUIRE_INT((int32)ncm_song_id(&copy), 8);
    REQUIRE_INT((int32)ncm_song_priority(&copy), 9);
    REQUIRE_INT((int32)ncm_song_mtime(&copy), 10);

    REQUIRE(ncm_song_set_uri(&song, LIT_ARGS("changed.flac")));
    REQUIRE(ncm_song_uri_view(&copy, 0, &view));
    REQUIRE_STRING(view.data, view.len, "dir/song.flac");

    ncm_song_destroy(&copy);
    ncm_song_destroy(&song);
    return;
}

static void
test_song_uri_helpers(void) {
    NcmStringView view;

    REQUIRE(ncm_song_name_from_uri(LIT_ARGS("dir/song.flac"), &view));
    REQUIRE_STRING(view.data, view.len, "song.flac");

    REQUIRE(ncm_song_name_from_uri(LIT_ARGS("song.flac"), &view));
    REQUIRE_STRING(view.data, view.len, "song.flac");

    REQUIRE(ncm_song_directory_from_uri(LIT_ARGS("dir/song.flac"), &view));
    REQUIRE_STRING(view.data, view.len, "dir");

    REQUIRE(ncm_song_directory_from_uri(LIT_ARGS("song.flac"), &view));
    REQUIRE_STRING(view.data, view.len, "/");

    REQUIRE(ncm_song_directory_from_uri(LIT_ARGS("/music/song.flac"),
                                        &view));
    REQUIRE_STRING(view.data, view.len, "/music");

    REQUIRE(ncm_song_uri_is_from_database(LIT_ARGS("dir/song.flac")));
    REQUIRE(ncm_song_uri_is_from_database(LIT_ARGS("song.flac")));
    REQUIRE(!ncm_song_uri_is_from_database(LIT_ARGS("/tmp/song.flac")));

    REQUIRE(ncm_song_uri_is_stream(LIT_ARGS("http://example/stream")));
    REQUIRE(ncm_song_uri_is_stream(LIT_ARGS("https://example/stream")));
    REQUIRE(!ncm_song_uri_is_stream(LIT_ARGS("ftp://example/stream")));
    return;
}

static void
test_song_format_helpers(void) {
    char buffer[32];
    int32 len;

    REQUIRE_INT(ncm_song_numeric_tag_len(LIT_ARGS("1")), 2);
    len = ncm_song_format_numeric_tag(buffer, NCM_ARRAY_LEN(buffer),
                                      LIT_ARGS("1"));
    REQUIRE_STRING(buffer, len, "01");

    len = ncm_song_format_numeric_tag(buffer, NCM_ARRAY_LEN(buffer),
                                      LIT_ARGS("0"));
    REQUIRE_STRING(buffer, len, "0");

    len = ncm_song_format_numeric_tag(buffer, NCM_ARRAY_LEN(buffer),
                                      LIT_ARGS("1/12"));
    REQUIRE_STRING(buffer, len, "01/12");

    len = ncm_song_format_numeric_tag(buffer, NCM_ARRAY_LEN(buffer),
                                      LIT_ARGS("10/12"));
    REQUIRE_STRING(buffer, len, "10/12");

    REQUIRE_INT(ncm_song_track_number_len(LIT_ARGS("1/12")), 2);
    len = ncm_song_format_track_number(buffer, NCM_ARRAY_LEN(buffer),
                                       LIT_ARGS("1/12"));
    REQUIRE_STRING(buffer, len, "01");

    len = ncm_song_format_track_number(buffer, NCM_ARRAY_LEN(buffer),
                                       LIT_ARGS("10/12"));
    REQUIRE_STRING(buffer, len, "10");

    len = ncm_song_show_time(65, buffer, NCM_ARRAY_LEN(buffer));
    REQUIRE_STRING(buffer, len, "1:05");

    len = ncm_song_show_time(3661, buffer, NCM_ARRAY_LEN(buffer));
    REQUIRE_STRING(buffer, len, "1:01:01");
    return;
}

static void
test_song_getter_buffers_and_hash(void) {
    NcmSong song;
    NcmSong copy;
    NcmBuffer buffer;
    uint64 hash;

    ncm_song_init(&song);
    ncm_song_init(&copy);

    REQUIRE(ncm_song_hash(NULL) == ncm_song_hash(&song));
    REQUIRE(ncm_song_set_uri(&song, LIT_ARGS("dir/song.flac")));
    REQUIRE(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Title")));
    REQUIRE(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Dup")));
    REQUIRE(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Dup")));
    REQUIRE(ncm_song_add_tag(&song, MPD_TAG_TRACK, LIT_ARGS("1/12")));
    ncm_song_set_duration(&song, 65);
    ncm_song_set_priority(&song, 4);

    buffer = ncm_song_getter_buffer(&song, NCM_SONG_GETTER_TITLE, 0);
    REQUIRE_STRING(buffer.data, buffer.len, "Title");
    ncm_buffer_destroy(&buffer);

    buffer = ncm_song_getter_buffer(&song, NCM_SONG_GETTER_DIRECTORY, 0);
    REQUIRE_STRING(buffer.data, buffer.len, "dir");
    ncm_buffer_destroy(&buffer);

    buffer = ncm_song_getter_buffer(&song, NCM_SONG_GETTER_NAME, 0);
    REQUIRE_STRING(buffer.data, buffer.len, "song.flac");
    ncm_buffer_destroy(&buffer);

    buffer = ncm_song_getter_buffer(&song, NCM_SONG_GETTER_TRACK, 0);
    REQUIRE_STRING(buffer.data, buffer.len, "01/12");
    ncm_buffer_destroy(&buffer);

    buffer = ncm_song_getter_buffer(&song, NCM_SONG_GETTER_TRACK_NUMBER, 0);
    REQUIRE_STRING(buffer.data, buffer.len, "01");
    ncm_buffer_destroy(&buffer);

    buffer = ncm_song_getter_buffer(&song, NCM_SONG_GETTER_LENGTH, 0);
    REQUIRE_STRING(buffer.data, buffer.len, "1:05");
    ncm_buffer_destroy(&buffer);

    buffer = ncm_song_getter_buffer(&song, NCM_SONG_GETTER_PRIORITY, 0);
    REQUIRE_STRING(buffer.data, buffer.len, "4");
    ncm_buffer_destroy(&buffer);

    buffer = ncm_song_tags_buffer(&song, NCM_SONG_GETTER_ARTIST,
                                  LIT_ARGS(", "), false);
    REQUIRE_STRING(buffer.data, buffer.len, "Dup");
    ncm_buffer_destroy(&buffer);

    buffer = ncm_song_tags_buffer(&song, NCM_SONG_GETTER_ARTIST,
                                  LIT_ARGS(", "), true);
    REQUIRE_STRING(buffer.data, buffer.len, "Dup, Dup");
    ncm_buffer_destroy(&buffer);

    hash = ncm_song_hash(&song);
    REQUIRE(ncm_song_copy(&copy, &song));
    REQUIRE(ncm_song_equal(&copy, &song));
    REQUIRE(ncm_song_hash(&copy) == hash);

    REQUIRE(ncm_song_add_tag(&copy, MPD_TAG_COMMENT, LIT_ARGS("Other")));
    REQUIRE(ncm_song_equal(&copy, &song));
    REQUIRE(ncm_song_hash(&copy) == hash);

    REQUIRE(ncm_song_set_uri(&copy, LIT_ARGS("dir/other.flac")));
    REQUIRE(!ncm_song_equal(&copy, &song));
    REQUIRE(ncm_song_hash(&copy) != hash);

    ncm_song_destroy(&copy);
    ncm_song_destroy(&song);
    return;
}

static void
test_item_unknown(void) {
    NcmMpdItem item;

    ncm_mpd_item_init(&item);
    REQUIRE_INT((int32)ncm_mpd_item_kind(&item), NCM_MPD_ITEM_UNKNOWN);
    REQUIRE(ncm_mpd_item_directory(&item) == NULL);
    REQUIRE(ncm_mpd_item_playlist(&item) == NULL);
    REQUIRE(ncm_mpd_item_song(&item) == NULL);

    ncm_mpd_item_destroy(&item);
    REQUIRE_INT((int32)ncm_mpd_item_kind(&item), NCM_MPD_ITEM_UNKNOWN);
    return;
}

static void
test_item_directory_copy(void) {
    NcmMpdItem item;
    NcmMpdItem copy;
    NcmDirectory *directory;

    ncm_mpd_item_init(&item);
    ncm_mpd_item_init(&copy);

    item.kind = NCM_MPD_ITEM_DIRECTORY;
    ncm_directory_init(&item.value.directory);
    REQUIRE(ncm_directory_set(&item.value.directory,
                              LIT_ARGS("Albums"), 111));

    REQUIRE(ncm_mpd_item_copy(&copy, &item));
    REQUIRE_INT((int32)ncm_mpd_item_kind(&copy), NCM_MPD_ITEM_DIRECTORY);
    directory = ncm_mpd_item_directory(&copy);
    REQUIRE(directory != NULL);
    REQUIRE_STRING(directory->path, directory->path_len, "Albums");
    REQUIRE_INT((int32)directory->last_modified, 111);
    REQUIRE(ncm_mpd_item_playlist(&copy) == NULL);

    REQUIRE(ncm_directory_set(&item.value.directory,
                              LIT_ARGS("Changed"), 222));
    REQUIRE_STRING(directory->path, directory->path_len, "Albums");

    ncm_mpd_item_destroy(&copy);
    ncm_mpd_item_destroy(&item);
    return;
}

static void
test_item_playlist_replacement(void) {
    NcmMpdItem item;
    NcmMpdItem replacement;
    NcmPlaylist *playlist;

    ncm_mpd_item_init(&item);
    ncm_mpd_item_init(&replacement);

    item.kind = NCM_MPD_ITEM_DIRECTORY;
    ncm_directory_init(&item.value.directory);
    REQUIRE(ncm_directory_set(&item.value.directory,
                              LIT_ARGS("Old"), 10));

    replacement.kind = NCM_MPD_ITEM_PLAYLIST;
    ncm_playlist_init(&replacement.value.playlist);
    REQUIRE(ncm_playlist_set(&replacement.value.playlist,
                             LIT_ARGS("Mix"), 20));

    REQUIRE(ncm_mpd_item_copy(&item, &replacement));
    REQUIRE_INT((int32)ncm_mpd_item_kind(&item), NCM_MPD_ITEM_PLAYLIST);
    playlist = ncm_mpd_item_playlist(&item);
    REQUIRE(playlist != NULL);
    REQUIRE_STRING(playlist->path, playlist->path_len, "Mix");
    REQUIRE_INT((int32)playlist->last_modified, 20);
    REQUIRE(ncm_mpd_item_directory(&item) == NULL);

    ncm_mpd_item_destroy(&replacement);
    ncm_mpd_item_destroy(&item);
    return;
}

static void
test_item_empty_song_copy(void) {
    NcmMpdItem item;
    NcmMpdItem copy;
    NcmSong *song;

    ncm_mpd_item_init(&item);
    ncm_mpd_item_init(&copy);

    item.kind = NCM_MPD_ITEM_SONG;
    ncm_song_init(&item.value.song);

    REQUIRE(ncm_mpd_item_copy(&copy, &item));
    REQUIRE_INT((int32)ncm_mpd_item_kind(&copy), NCM_MPD_ITEM_SONG);
    song = ncm_mpd_item_song(&copy);
    REQUIRE(song != NULL);
    REQUIRE(ncm_song_empty(song));
    REQUIRE(ncm_mpd_item_directory(&copy) == NULL);

    ncm_mpd_item_destroy(&copy);
    ncm_mpd_item_destroy(&item);
    return;
}

static void
test_item_song_null_constructors(void) {
    NcmMpdItem item;

    ncm_mpd_item_init(&item);

    REQUIRE(!ncm_mpd_item_from_mpd_song_copy(&item, NULL));
    REQUIRE_INT((int32)ncm_mpd_item_kind(&item), NCM_MPD_ITEM_UNKNOWN);
    REQUIRE(!ncm_mpd_item_from_mpd_song_borrow(&item, NULL));
    REQUIRE_INT((int32)ncm_mpd_item_kind(&item), NCM_MPD_ITEM_UNKNOWN);

    ncm_mpd_item_destroy(&item);
    return;
}

static void
test_mpd_song_list_copy_and_array(void) {
    NcmSong song;
    NcmMpdSongList list;
    NcmMpdSongList copy;
    NcmSongArray array;
    NcmStringView view;

    ncm_song_init(&song);
    ncm_mpd_song_list_init(&list);
    ncm_mpd_song_list_init(&copy);
    ncm_song_array_init(&array);

    REQUIRE(ncm_song_set_uri(&song, LIT_ARGS("one.flac")));
    REQUIRE(ncm_mpd_song_list_append_copy(&list, &song));
    REQUIRE(ncm_song_set_uri(&song, LIT_ARGS("two.flac")));
    ncm_mpd_song_list_append_move(&list, &song);
    REQUIRE(ncm_song_empty(&song));
    REQUIRE_INT(ncm_mpd_song_list_count(&list), 2);

    REQUIRE(ncm_mpd_song_list_copy(&copy, &list));
    REQUIRE_INT(ncm_mpd_song_list_count(&copy), 2);
    REQUIRE(ncm_song_uri_view(ncm_mpd_song_list_at(&copy, 1), 0, &view));
    REQUIRE_STRING(view.data, view.len, "two.flac");

    REQUIRE(ncm_mpd_song_list_to_song_array(&copy, &array));
    REQUIRE_INT(array.len, 2);
    REQUIRE(ncm_song_uri_view(&array.items[0], 0, &view));
    REQUIRE_STRING(view.data, view.len, "one.flac");

    ncm_song_array_destroy(&array);
    ncm_mpd_song_list_destroy(&copy);
    ncm_mpd_song_list_destroy(&list);
    ncm_song_destroy(&song);
    return;
}

static void
test_mpd_item_list_directory_array(void) {
    NcmDirectory directory;
    NcmMpdItem item;
    NcmMpdItemList list;
    NcmMpdItemArray items;
    NcmDirectoryArray directories;

    ncm_directory_init(&directory);
    ncm_mpd_item_init(&item);
    ncm_mpd_item_list_init(&list);
    ncm_mpd_item_array_init(&items);
    ncm_directory_array_init(&directories);

    REQUIRE(ncm_directory_set(&directory, LIT_ARGS("Albums"), 77));
    REQUIRE(ncm_mpd_item_set_directory(&item, &directory));
    REQUIRE(ncm_mpd_item_list_append_copy(&list, &item));
    REQUIRE_INT(ncm_mpd_item_list_count(&list), 1);
    REQUIRE(ncm_mpd_item_list_to_item_array(&list, &items));
    REQUIRE_INT(items.len, 1);
    REQUIRE(ncm_mpd_item_list_to_directory_array(&list, &directories));
    REQUIRE_INT(directories.len, 1);
    REQUIRE_STRING(directories.items[0].path,
                   directories.items[0].path_len, "Albums");

    ncm_directory_array_destroy(&directories);
    ncm_mpd_item_array_destroy(&items);
    ncm_mpd_item_list_destroy(&list);
    ncm_mpd_item_destroy(&item);
    ncm_directory_destroy(&directory);
    return;
}

static void
test_mpd_string_output_and_playlist_lists(void) {
    NcmMpdString string;
    NcmMpdStringList strings;
    NcmBufferArray buffers;
    NcmMpdOutput output;
    NcmMpdOutputList outputs;
    NcmMpdOutputList outputs_copy;
    NcmPlaylist playlist;
    NcmMpdPlaylistList playlists;
    NcmPlaylistArray playlist_array;

    ncm_mpd_string_init(&string);
    ncm_mpd_string_list_init(&strings);
    ncm_buffer_array_init(&buffers);
    ncm_mpd_output_init(&output);
    ncm_mpd_output_list_init(&outputs);
    ncm_mpd_output_list_init(&outputs_copy);
    ncm_playlist_init(&playlist);
    ncm_mpd_playlist_list_init(&playlists);
    ncm_playlist_array_init(&playlist_array);

    REQUIRE(ncm_mpd_string_set(&string, LIT_ARGS("handler")));
    REQUIRE_STRING(string.value, string.value_len, "handler");
    REQUIRE(ncm_mpd_string_list_append(&strings, LIT_ARGS("alpha")));
    REQUIRE(ncm_mpd_string_list_append(&strings, LIT_ARGS("beta")));
    REQUIRE_INT(ncm_mpd_string_list_count(&strings), 2);
    REQUIRE(ncm_mpd_string_list_to_buffer_array(&strings, &buffers));
    REQUIRE_INT(buffers.len, 2);
    REQUIRE_STRING(buffers.items[1].data, buffers.items[1].len, "beta");

    REQUIRE(ncm_mpd_output_set(&output, 42, LIT_ARGS("speakers"), true));
    REQUIRE(ncm_mpd_output_list_append_copy(&outputs, &output));
    REQUIRE(ncm_mpd_output_list_copy(&outputs_copy, &outputs));
    REQUIRE_INT(ncm_mpd_output_list_count(&outputs_copy), 1);
    REQUIRE_INT((int32)ncm_mpd_output_list_at(&outputs_copy, 0)->id, 42);
    REQUIRE_STRING(outputs_copy.items[0].name,
                   outputs_copy.items[0].name_len, "speakers");
    REQUIRE(outputs_copy.items[0].enabled);

    REQUIRE(ncm_playlist_set(&playlist, LIT_ARGS("favorites"), 5));
    REQUIRE(ncm_mpd_playlist_list_append_copy(&playlists, &playlist));
    REQUIRE(ncm_mpd_playlist_list_to_playlist_array(&playlists,
                                                    &playlist_array));
    REQUIRE_INT(playlist_array.len, 1);
    REQUIRE_STRING(playlist_array.items[0].path,
                   playlist_array.items[0].path_len, "favorites");

    ncm_playlist_array_destroy(&playlist_array);
    ncm_mpd_playlist_list_destroy(&playlists);
    ncm_playlist_destroy(&playlist);
    ncm_mpd_output_list_destroy(&outputs_copy);
    ncm_mpd_output_list_destroy(&outputs);
    ncm_mpd_output_destroy(&output);
    ncm_buffer_array_destroy(&buffers);
    ncm_mpd_string_list_destroy(&strings);
    ncm_mpd_string_destroy(&string);
    return;
}

int
main(void) {
    test_directory();
    test_playlist();
    test_song_empty_copy();
    test_song_storage_copy();
    test_song_uri_helpers();
    test_song_format_helpers();
    test_song_getter_buffers_and_hash();
    test_item_unknown();
    test_item_directory_copy();
    test_item_playlist_replacement();
    test_item_empty_song_copy();
    test_item_song_null_constructors();
    test_mpd_song_list_copy_and_array();
    test_mpd_item_list_directory_array();
    test_mpd_string_output_and_playlist_lists();

    exit(EXIT_SUCCESS);
}
