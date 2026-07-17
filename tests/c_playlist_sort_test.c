#include "c/ncm_playlist_sort.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)
#define TEXT(S) (char *)S
#define TEST_MAX_SWAPS 32

typedef struct TestMpdState {
    NcmPlaylistSortSwap swaps[TEST_MAX_SWAPS];
    int32 swaps_len;
    int32 start_calls;
    int32 commit_calls;
    int32 fail_swap;
    bool fail_start;
    bool fail_commit;
} TestMpdState;

static TestMpdState test_mpd;

static int32 test_cstrlen32(char *string);
static void test_mpd_reset(void);
static void test_song_set(NcmSong *song, char *uri, uint32 position,
                          char *artist, char *album, char *title,
                          char *track);
static void test_plan_key_order_and_absolute_swaps(void);
static void test_plan_uses_position_for_equal_and_duplicate_songs(void);
static void test_plan_honors_ignore_leading_the_and_none(void);
static void test_plan_validates_inputs(void);
static void test_execute_uses_one_command_list(void);
static void test_execute_preserves_first_error(void);
static void test_noop_avoids_mpd_traffic(void);

bool
__wrap_ncm_mpd_client_start_command_list(NcmMpdClient *client,
                                         NcmError *error) {
    test_mpd.start_calls += 1;
    if (test_mpd.fail_start) {
        ncm_error_set(error, EIO, STRLIT_ARGS("start failure"));
        return false;
    }
    client->command_list_active = true;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_swap(NcmMpdClient *client, uint32 from,
                           uint32 to, NcmError *error) {
    (void)client;
    if (test_mpd.swaps_len == test_mpd.fail_swap) {
        ncm_error_set(error, EIO, STRLIT_ARGS("swap failure"));
        return false;
    }
    assert(test_mpd.swaps_len < TEST_MAX_SWAPS);
    test_mpd.swaps[test_mpd.swaps_len].from = from;
    test_mpd.swaps[test_mpd.swaps_len].to = to;
    test_mpd.swaps_len += 1;
    ncm_error_clear(error);
    return true;
}

bool
__wrap_ncm_mpd_client_commit_command_list(NcmMpdClient *client,
                                          NcmError *error) {
    test_mpd.commit_calls += 1;
    client->command_list_active = false;
    if (test_mpd.fail_commit) {
        ncm_error_set(error, EIO, STRLIT_ARGS("commit failure"));
        return false;
    }
    ncm_error_clear(error);
    return true;
}

int
main(void) {
    test_plan_key_order_and_absolute_swaps();
    test_plan_uses_position_for_equal_and_duplicate_songs();
    test_plan_honors_ignore_leading_the_and_none();
    test_plan_validates_inputs();
    test_execute_uses_one_command_list();
    test_execute_preserves_first_error();
    test_noop_avoids_mpd_traffic();
    return EXIT_SUCCESS;
}

static int32
test_cstrlen32(char *string) {
    int32 len;

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static void
test_mpd_reset(void) {
    test_mpd = (TestMpdState){0};
    test_mpd.fail_swap = -1;
    return;
}

static void
test_song_set(NcmSong *song, char *uri, uint32 position,
              char *artist, char *album, char *title,
              char *track) {
    ncm_song_init(song);
    assert(ncm_song_set_uri(song, uri, test_cstrlen32(uri)));
    ncm_song_set_position(song, position);
    if (artist != NULL) {
        assert(ncm_song_add_tag(song, MPD_TAG_ARTIST, artist,
                                test_cstrlen32(artist)));
    }
    if (album != NULL) {
        assert(ncm_song_add_tag(song, MPD_TAG_ALBUM, album,
                                test_cstrlen32(album)));
    }
    if (title != NULL) {
        assert(ncm_song_add_tag(song, MPD_TAG_TITLE, title,
                                test_cstrlen32(title)));
    }
    if (track != NULL) {
        assert(ncm_song_add_tag(song, MPD_TAG_TRACK, track,
                                test_cstrlen32(track)));
    }
    return;
}

static void
test_plan_key_order_and_absolute_swaps(void) {
    enum NcmSongGetter getters[] = {
        NCM_SONG_GETTER_ARTIST,
        NCM_SONG_GETTER_ALBUM,
        NCM_SONG_GETTER_TRACK,
    };
    NcmPlaylistSortPlan plan;
    NcmSongArray songs;
    NcmSong song;
    NcmError error;

    ncm_playlist_sort_plan_init(&plan);
    ncm_song_array_init(&songs);
    test_song_set(&song, TEXT("one.flac"), 10,
                  TEXT("Beta"), TEXT("Second"),
                  TEXT("One"), TEXT("1"));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("two.flac"), 11,
                  TEXT("Alpha"), TEXT("First"),
                  TEXT("Two"), TEXT("10"));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("three.flac"), 12,
                  TEXT("Alpha"), TEXT("First"),
                  TEXT("Three"), TEXT("2"));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);

    assert(ncm_playlist_sort_plan_build(
        &plan, &songs, 40, getters, NCM_ARRAY_LEN(getters),
        false, &error));
    assert(plan.len == 1);
    assert(plan.items[0].from == 40);
    assert(plan.items[0].to == 42);
    assert(ncm_song_position(&songs.items[0]) == 10);
    assert(ncm_song_position(&songs.items[1]) == 11);
    assert(ncm_song_position(&songs.items[2]) == 12);

    ncm_song_array_destroy(&songs);
    ncm_playlist_sort_plan_destroy(&plan);
    return;
}

static void
test_plan_uses_position_for_equal_and_duplicate_songs(void) {
    enum NcmSongGetter getters[] = {
        NCM_SONG_GETTER_TITLE,
    };
    NcmPlaylistSortPlan plan;
    NcmSongArray songs;
    NcmSong song;
    NcmError error;

    ncm_playlist_sort_plan_init(&plan);
    ncm_song_array_init(&songs);
    test_song_set(&song, TEXT("same.flac"), 103,
                  NULL, NULL, TEXT("Same"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("same.flac"), 100,
                  NULL, NULL, TEXT("Same"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("same.flac"), 102,
                  NULL, NULL, TEXT("Same"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("same.flac"), 101,
                  NULL, NULL, TEXT("Same"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);

    assert(ncm_playlist_sort_plan_build(
        &plan, &songs, 500, getters, NCM_ARRAY_LEN(getters),
        false, &error));
    assert(plan.len == 2);
    assert(plan.items[0].from == 500);
    assert(plan.items[0].to == 501);
    assert(plan.items[1].from == 501);
    assert(plan.items[1].to == 503);
    for (int32 i = 0; i < plan.len; i += 1) {
        assert(plan.items[i].from >= 500);
        assert(plan.items[i].from < 504);
        assert(plan.items[i].to >= 500);
        assert(plan.items[i].to < 504);
    }

    ncm_song_array_destroy(&songs);
    ncm_playlist_sort_plan_destroy(&plan);
    return;
}

static void
test_plan_honors_ignore_leading_the_and_none(void) {
    enum NcmSongGetter getters[] = {
        NCM_SONG_GETTER_ARTIST,
        NCM_SONG_GETTER_NONE,
        NCM_SONG_GETTER_TITLE,
    };
    NcmPlaylistSortPlan plan;
    NcmSongArray songs;
    NcmSong song;
    NcmError error;

    ncm_playlist_sort_plan_init(&plan);
    ncm_song_array_init(&songs);
    test_song_set(&song, TEXT("first.flac"), 20,
                  TEXT("The Beatles"), NULL,
                  TEXT("A"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("second.flac"), 21,
                  TEXT("Cars"), NULL,
                  TEXT("Z"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);

    assert(ncm_playlist_sort_plan_build(
        &plan, &songs, 7, getters, NCM_ARRAY_LEN(getters),
        false, &error));
    assert(plan.len == 1);
    assert(plan.items[0].from == 7);
    assert(plan.items[0].to == 8);

    assert(ncm_playlist_sort_plan_build(
        &plan, &songs, 7, getters, NCM_ARRAY_LEN(getters),
        true, &error));
    assert(plan.len == 0);

    ncm_song_array_clear(&songs);
    test_song_set(&song, TEXT("third.flac"), 30,
                  TEXT("Same"), NULL, TEXT("Z"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("fourth.flac"), 31,
                  TEXT("Same"), NULL, TEXT("A"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    assert(ncm_playlist_sort_plan_build(
        &plan, &songs, 7, getters, NCM_ARRAY_LEN(getters),
        false, &error));
    assert(plan.len == 0);

    ncm_song_array_destroy(&songs);
    ncm_playlist_sort_plan_destroy(&plan);
    return;
}

static void
test_plan_validates_inputs(void) {
    enum NcmSongGetter invalid_getter;
    NcmPlaylistSortPlan plan;
    NcmSongArray songs;
    NcmSong song;
    NcmError error;

    ncm_playlist_sort_plan_init(&plan);
    ncm_song_array_init(&songs);
    invalid_getter = (enum NcmSongGetter)999;

    assert(!ncm_playlist_sort_plan_build(
        NULL, &songs, 0, NULL, 0, false, &error));
    assert(error.code == EINVAL);
    assert(!ncm_playlist_sort_plan_build(
        &plan, NULL, 0, NULL, 0, false, &error));
    assert(error.code == EINVAL);
    assert(!ncm_playlist_sort_plan_build(
        &plan, &songs, 0, NULL, 1, false, &error));
    assert(error.code == EINVAL);
    assert(!ncm_playlist_sort_plan_build(
        &plan, &songs, 0, &invalid_getter, 1, false, &error));
    assert(error.code == EINVAL);

    test_song_set(&song, TEXT("overflow.flac"), 0,
                  NULL, NULL, NULL, NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    assert(!ncm_playlist_sort_plan_build(
        &plan, &songs, UINT32_MAX, NULL, 0, false, &error));
    assert(error.code == EOVERFLOW);

    ncm_song_array_destroy(&songs);
    ncm_playlist_sort_plan_destroy(&plan);
    return;
}

static void
test_execute_uses_one_command_list(void) {
    enum NcmSongGetter getters[] = {
        NCM_SONG_GETTER_TITLE,
    };
    NcmMpdClient client = {0};
    NcmSongArray songs;
    NcmSong song;
    NcmError error;

    test_mpd_reset();
    ncm_song_array_init(&songs);
    test_song_set(&song, TEXT("c.flac"), 0,
                  NULL, NULL, TEXT("C"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("a.flac"), 1,
                  NULL, NULL, TEXT("A"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);
    test_song_set(&song, TEXT("b.flac"), 2,
                  NULL, NULL, TEXT("B"), NULL);
    assert(ncm_song_array_append_copy(&songs, &song));
    ncm_song_destroy(&song);

    assert(ncm_playlist_sort_range(
        &songs, 30, getters, NCM_ARRAY_LEN(getters), false,
        &client, &error));
    assert(test_mpd.start_calls == 1);
    assert(test_mpd.commit_calls == 1);
    assert(test_mpd.swaps_len == 2);
    assert(test_mpd.swaps[0].from == 30);
    assert(test_mpd.swaps[0].to == 31);
    assert(test_mpd.swaps[1].from == 31);
    assert(test_mpd.swaps[1].to == 32);
    assert(!client.command_list_active);

    ncm_song_array_destroy(&songs);
    return;
}

static void
test_execute_preserves_first_error(void) {
    NcmPlaylistSortPlan plan;
    NcmMpdClient client = {0};
    NcmError error;

    ncm_playlist_sort_plan_init(&plan);
    plan.items = malloc2(2*SIZEOF(*plan.items));
    plan.len = 2;
    plan.cap = 2;
    plan.items[0].from = 10;
    plan.items[0].to = 11;
    plan.items[1].from = 11;
    plan.items[1].to = 12;

    test_mpd_reset();
    test_mpd.fail_start = true;
    client.command_list_active = true;
    assert(!ncm_playlist_sort_plan_execute(&plan, &client, &error));
    assert(error.code == EIO);
    assert(STREQUAL(error.message, STRLIT_LEN("start failure"),
                            LIT_ARGS("start failure")));
    assert(test_mpd.swaps_len == 0);
    assert(test_mpd.commit_calls == 0);
    assert(client.command_list_active);

    client.command_list_active = false;
    test_mpd_reset();
    test_mpd.fail_swap = 1;
    assert(!ncm_playlist_sort_plan_execute(&plan, &client, &error));
    assert(error.code == EIO);
    assert(STREQUAL(error.message, STRLIT_LEN("swap failure"),
                            LIT_ARGS("swap failure")));
    assert(test_mpd.start_calls == 1);
    assert(test_mpd.swaps_len == 1);
    assert(test_mpd.commit_calls == 0);
    assert(!client.command_list_active);

    test_mpd_reset();
    test_mpd.fail_commit = true;
    assert(!ncm_playlist_sort_plan_execute(&plan, &client, &error));
    assert(error.code == EIO);
    assert(STREQUAL(error.message, STRLIT_LEN("commit failure"),
                            LIT_ARGS("commit failure")));
    assert(test_mpd.start_calls == 1);
    assert(test_mpd.swaps_len == 2);
    assert(test_mpd.commit_calls == 1);

    ncm_playlist_sort_plan_destroy(&plan);
    return;
}

static void
test_noop_avoids_mpd_traffic(void) {
    NcmMpdClient client = {0};
    NcmSongArray songs;
    NcmError error;

    test_mpd_reset();
    ncm_song_array_init(&songs);
    assert(ncm_playlist_sort_range(
        &songs, 0, NULL, 0, false, &client, &error));
    assert(test_mpd.start_calls == 0);
    assert(test_mpd.swaps_len == 0);
    assert(test_mpd.commit_calls == 0);
    ncm_song_array_destroy(&songs);
    return;
}
