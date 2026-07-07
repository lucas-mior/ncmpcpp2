#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "c/ncm_mutable_song.h"
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

static enum NcmTagsField written_field;
static NcmStringView written_first;
static NcmStringView written_second;

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static bool strings_equal(char *a, int32 a_len, char *b, int32 b_len);
static void test_single_tag_edit(void);
static void test_split_tags_and_truncation(void);
static void test_new_name(void);
static void test_copy_and_clear(void);
static void test_write_callback(void);
static void test_load_originals_from_song(void);

bool ncm_tags_write(char *music_dir, char *uri, bool is_from_database,
                    char *directory, char *new_name,
                    NcmTagsGetFieldCallback callback, void *user);

bool
ncm_tags_write(char *music_dir, char *uri, bool is_from_database,
               char *directory, char *new_name,
               NcmTagsGetFieldCallback callback, void *user) {
    (void)music_dir;
    (void)uri;
    (void)is_from_database;
    (void)directory;
    (void)new_name;

    written_field = NCM_TAGS_FIELD_ARTIST;
    callback(written_field, 0, &written_first, user);
    callback(written_field, 1, &written_second, user);
    return true;
}

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

static bool
strings_equal(char *a, int32 a_len, char *b, int32 b_len) {
    if (a_len != b_len) {
        return false;
    }

    for (int32 i = 0; i < a_len; i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

static void
require_string(char *file, int32 line, char *name,
               char *actual, int32 actual_len,
               char *expected, int32 expected_len) {
    if (!strings_equal(actual, actual_len, expected, expected_len)) {
        fprintf(stderr, "%s:%d: %s: expected '%.*s', got '%.*s'\n",
                file, line, name,
                expected_len, expected, actual_len, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
test_single_tag_edit(void) {
    NcmMutableSong song;
    NcmStringView value;

    ncm_mutable_song_init(&song);
    REQUIRE(ncm_mutable_song_set_original_tag(&song, NCM_TAGS_FIELD_TITLE,
                                              0, LIT_ARGS("Old")));
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_TITLE, 0,
                                     &value));
    REQUIRE_STRING(value.data, value.len, "Old");
    REQUIRE(!ncm_mutable_song_is_modified(&song));

    REQUIRE(ncm_mutable_song_set_tag(&song, NCM_TAGS_FIELD_TITLE,
                                     0, LIT_ARGS("New")));
    REQUIRE(ncm_mutable_song_is_modified(&song));
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_TITLE, 0,
                                     &value));
    REQUIRE_STRING(value.data, value.len, "New");

    REQUIRE(ncm_mutable_song_set_tag(&song, NCM_TAGS_FIELD_TITLE,
                                     0, LIT_ARGS("Old")));
    REQUIRE(!ncm_mutable_song_is_modified(&song));
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_TITLE, 0,
                                     &value));
    REQUIRE_STRING(value.data, value.len, "Old");

    ncm_mutable_song_destroy(&song);
    return;
}

static void
test_split_tags_and_truncation(void) {
    NcmMutableSong song;
    NcmStringView value;

    ncm_mutable_song_init(&song);
    REQUIRE(ncm_mutable_song_set_original_tag(&song, NCM_TAGS_FIELD_ARTIST,
                                              0, LIT_ARGS("A")));
    REQUIRE(ncm_mutable_song_set_original_tag(&song, NCM_TAGS_FIELD_ARTIST,
                                              1, LIT_ARGS("B")));

    REQUIRE(ncm_mutable_song_set_tags(&song, NCM_TAGS_FIELD_ARTIST,
                                      LIT_ARGS("X | Y"), LIT_ARGS(" | ")));
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_ARTIST, 0,
                                     &value));
    REQUIRE_STRING(value.data, value.len, "X");
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_ARTIST, 1,
                                     &value));
    REQUIRE_STRING(value.data, value.len, "Y");

    REQUIRE(ncm_mutable_song_set_tags(&song, NCM_TAGS_FIELD_ARTIST,
                                      LIT_ARGS("A"), LIT_ARGS(" | ")));
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_ARTIST, 0,
                                     &value));
    REQUIRE_STRING(value.data, value.len, "A");
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_ARTIST, 1,
                                     &value));
    REQUIRE_INT(value.len, 0);
    REQUIRE(ncm_mutable_song_is_modified(&song));

    ncm_mutable_song_destroy(&song);
    return;
}

static void
test_new_name(void) {
    NcmMutableSong song;
    NcmStringView value;

    ncm_mutable_song_init(&song);
    REQUIRE(ncm_mutable_song_set_name(&song, LIT_ARGS("old.flac")));
    REQUIRE(ncm_mutable_song_set_new_name(&song, LIT_ARGS("old.flac")));
    REQUIRE(!ncm_mutable_song_is_modified(&song));
    REQUIRE(!ncm_mutable_song_get_new_name(&song, &value));

    REQUIRE(ncm_mutable_song_set_new_name(&song, LIT_ARGS("new.flac")));
    REQUIRE(ncm_mutable_song_is_modified(&song));
    REQUIRE(ncm_mutable_song_get_new_name(&song, &value));
    REQUIRE_STRING(value.data, value.len, "new.flac");

    REQUIRE(ncm_mutable_song_set_new_name(&song, LIT_ARGS("")));
    REQUIRE(!ncm_mutable_song_is_modified(&song));

    ncm_mutable_song_destroy(&song);
    return;
}

static void
test_copy_and_clear(void) {
    NcmMutableSong song;
    NcmMutableSong copy;
    NcmStringView value;

    ncm_mutable_song_init(&song);
    ncm_mutable_song_init(&copy);
    REQUIRE(ncm_mutable_song_set_original_tag(&song, NCM_TAGS_FIELD_GENRE,
                                              0, LIT_ARGS("Jazz")));
    REQUIRE(ncm_mutable_song_set_tag(&song, NCM_TAGS_FIELD_GENRE,
                                     0, LIT_ARGS("Fusion")));
    REQUIRE(ncm_mutable_song_copy(&copy, &song));
    REQUIRE(ncm_mutable_song_get_tag(&copy, NCM_TAGS_FIELD_GENRE, 0,
                                     &value));
    REQUIRE_STRING(value.data, value.len, "Fusion");

    ncm_mutable_song_clear_modifications(&copy);
    REQUIRE(!ncm_mutable_song_is_modified(&copy));
    REQUIRE(ncm_mutable_song_get_tag(&copy, NCM_TAGS_FIELD_GENRE, 0,
                                     &value));
    REQUIRE_STRING(value.data, value.len, "Jazz");

    ncm_mutable_song_destroy(&copy);
    ncm_mutable_song_destroy(&song);
    return;
}

static void
test_write_callback(void) {
    NcmMutableSong song;

    ncm_mutable_song_init(&song);
    ncm_mutable_song_set_uri(&song, LIT_ARGS("dir/file.flac"));
    ncm_mutable_song_set_directory(&song, LIT_ARGS("dir"));
    ncm_mutable_song_set_from_database(&song, true);
    ncm_mutable_song_set_original_tag(&song, NCM_TAGS_FIELD_ARTIST,
                                      0, LIT_ARGS("A"));
    ncm_mutable_song_set_tag(&song, NCM_TAGS_FIELD_ARTIST,
                             1, LIT_ARGS("B"));

    REQUIRE(ncm_mutable_song_write(&song, (char *)"/music/"));
    REQUIRE_INT((int32)written_field, NCM_TAGS_FIELD_ARTIST);
    REQUIRE_STRING(written_first.data, written_first.len, "A");
    REQUIRE_STRING(written_second.data, written_second.len, "B");

    ncm_mutable_song_destroy(&song);
    return;
}

static void
test_load_originals_from_song(void) {
    NcmSong source;
    NcmMutableSong song;
    NcmStringView value;

    ncm_song_init(&source);
    ncm_mutable_song_init(&song);

    REQUIRE(ncm_song_set_uri(&source, LIT_ARGS("dir/file.flac")));
    REQUIRE(ncm_song_add_tag(&source, MPD_TAG_ARTIST, LIT_ARGS("Artist")));
    REQUIRE(ncm_song_add_tag(&source, MPD_TAG_TITLE, LIT_ARGS("Title")));
    ncm_song_set_duration(&source, 99);

    REQUIRE(ncm_mutable_song_load_originals_from_song(&song, &source));
    REQUIRE_STRING(song.uri, song.uri_len, "dir/file.flac");
    REQUIRE_STRING(song.directory, song.directory_len, "dir");
    REQUIRE_STRING(song.name, song.name_len, "file.flac");
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_ARTIST,
                                     0, &value));
    REQUIRE_STRING(value.data, value.len, "Artist");
    REQUIRE(ncm_mutable_song_get_tag(&song, NCM_TAGS_FIELD_TITLE,
                                     0, &value));
    REQUIRE_STRING(value.data, value.len, "Title");
    REQUIRE_INT((int32)ncm_mutable_song_duration(&song), 0);
    REQUIRE(!ncm_mutable_song_is_modified(&song));

    ncm_mutable_song_destroy(&song);
    ncm_song_destroy(&source);
    return;
}

int
main(void) {
    test_single_tag_edit();
    test_split_tags_and_truncation();
    test_new_name();
    test_copy_and_clear();
    test_write_callback();
    test_load_originals_from_song();
    return EXIT_SUCCESS;
}
