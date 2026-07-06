#include <stdio.h>
#include <stdlib.h>

#include "c/ncm_song_list.h"
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
static void test_selected_songs(void);
static void test_move_append(void);

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
    if (!ncm_string_equal(actual, actual_len, expected, expected_len)) {
        fprintf(stderr, "%s:%d: %s: expected '%.*s', got '%.*s'\n",
                file, line, name,
                expected_len, expected, actual_len, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
test_selected_songs(void) {
    NcmSong song;
    NcmSongList list;
    NcmSongArray selected;
    NcmStringView view;

    ncm_song_init(&song);
    ncm_song_list_init(&list);
    ncm_song_array_init(&selected);

    REQUIRE(ncm_song_set_uri(&song, LIT_ARGS("one.flac")));
    REQUIRE(ncm_song_list_append_copy(&list, &song,
                                      NCM_SONG_LIST_ITEM_SELECTABLE));
    REQUIRE(ncm_song_set_uri(&song, LIT_ARGS("two.flac")));
    REQUIRE(ncm_song_list_append_copy(&list, &song,
                                      NCM_SONG_LIST_ITEM_SELECTABLE
                                      |NCM_SONG_LIST_ITEM_SELECTED));

    REQUIRE(ncm_song_list_selected_songs(&list, &selected));
    REQUIRE_INT(selected.len, 1);
    REQUIRE(ncm_song_uri_view(&selected.items[0], 0, &view));
    REQUIRE_STRING(view.data, view.len, "two.flac");

    ncm_song_list_item_set_selected(&list.items[1], false);
    REQUIRE(ncm_song_list_set_current(&list, 0));
    REQUIRE(ncm_song_list_selected_songs(&list, &selected));
    REQUIRE_INT(selected.len, 1);
    REQUIRE(ncm_song_uri_view(&selected.items[0], 0, &view));
    REQUIRE_STRING(view.data, view.len, "one.flac");

    ncm_song_array_destroy(&selected);
    ncm_song_list_destroy(&list);
    ncm_song_destroy(&song);
    return;
}

static void
test_move_append(void) {
    NcmSong song;
    NcmSongList list;
    NcmSongListItem *item;
    NcmStringView view;

    ncm_song_init(&song);
    ncm_song_list_init(&list);

    REQUIRE(ncm_song_set_uri(&song, LIT_ARGS("moved.flac")));
    ncm_song_list_append_move(&list, &song,
                              NCM_SONG_LIST_ITEM_SELECTABLE);
    REQUIRE(ncm_song_empty(&song));
    REQUIRE_INT(list.len, 1);

    item = ncm_song_list_current(&list);
    REQUIRE(item != NULL);
    REQUIRE(ncm_song_uri_view(&item->song, 0, &view));
    REQUIRE_STRING(view.data, view.len, "moved.flac");

    ncm_song_list_destroy(&list);
    ncm_song_destroy(&song);
    return;
}

int
main(void) {
    test_selected_songs();
    test_move_append();
    return EXIT_SUCCESS;
}
