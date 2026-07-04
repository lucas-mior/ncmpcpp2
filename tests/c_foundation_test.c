#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <mpd/tag.h>

#include "c/ncm_base.h"
#include "c/ncm_error.h"
#include "c/ncm_html.h"
#include "c/ncm_path.h"
#include "c/ncm_sample_buffer.h"
#include "c/ncm_string.h"
#include "c/ncm_type_conversions.h"
#include "c/ncm_utf8.h"
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

#define REQUIRE_CSTRING(ACTUAL, EXPECTED)                                  \
    require_cstring(__FILE__, __LINE__, (char *)#ACTUAL,                  \
                    ACTUAL, LIT_ARGS(EXPECTED))

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static int32 cstr_len(char *string);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static void require_cstring(char *file, int32 line, char *name,
                            char *actual, char *expected,
                            int32 expected_len);
static void require_buffer_string(char *name, NcmBuffer *buffer,
                                  char *expected, int32 expected_len);
static void test_buffer(void);
static void test_error(void);
static void test_path(void);
static void test_string(void);
static void test_html(void);
static void test_sample_buffer(void);
static void test_type_conversions(void);
static void test_utf8(void);

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

static int32
cstr_len(char *string) {
    int32 len;

    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }

    return len;
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
require_cstring(char *file, int32 line, char *name,
                char *actual, char *expected, int32 expected_len) {
    require_string(file, line, name,
                   actual, cstr_len(actual), expected, expected_len);
    return;
}

static void
require_buffer_string(char *name, NcmBuffer *buffer,
                      char *expected, int32 expected_len) {
    require_string(__FILE__, __LINE__, name,
                   buffer->data, buffer->len, expected, expected_len);
    return;
}

static void
test_buffer(void) {
    NcmBuffer buffer;
    char *stolen;
    int32 stolen_len;
    int32 stolen_cap;

    ncm_buffer_init(&buffer);
    ncm_buffer_append(&buffer, LIT_ARGS("ab"));
    ncm_buffer_append_byte(&buffer, 'c');
    REQUIRE_INT(buffer.len, 3);
    REQUIRE(buffer.cap >= 4);
    REQUIRE_STRING(buffer.data, buffer.len, "abc");

    ncm_buffer_clear(&buffer);
    REQUIRE_INT(buffer.len, 0);
    REQUIRE(buffer.data[0] == '\0');

    ncm_buffer_append(&buffer, LIT_ARGS("value"));
    stolen = ncm_buffer_steal(&buffer, &stolen_len, &stolen_cap);
    REQUIRE_STRING(stolen, stolen_len, "value");
    REQUIRE(stolen_cap >= stolen_len + 1);
    REQUIRE(buffer.data == NULL);
    REQUIRE_INT(buffer.len, 0);
    REQUIRE_INT(buffer.cap, 0);

    ncm_free(stolen, stolen_cap);
    ncm_buffer_destroy(&buffer);
    return;
}

static void
test_error(void) {
    NcmError error;
    char long_message[300];

    ncm_error_clear(&error);
    REQUIRE(!ncm_error_is_set(&error));
    REQUIRE_INT(error.code, 0);
    REQUIRE(error.message[0] == '\0');

    ncm_error_set(&error, 7, LIT_ARGS("failed"));
    REQUIRE(ncm_error_is_set(&error));
    REQUIRE_INT(error.code, 7);
    REQUIRE_STRING(error.message, STRLIT_LEN("failed"), "failed");

    ncm_error_set(&error, 11, NULL, 0);
    REQUIRE(ncm_error_is_set(&error));
    REQUIRE_INT(error.code, 11);
    REQUIRE(error.message[0] == '\0');

    for (int32 i = 0; i < (int32)SIZEOF(long_message); i += 1) {
        long_message[i] = 'x';
    }
    ncm_error_set(&error, 13, long_message, (int32)SIZEOF(long_message));
    REQUIRE(ncm_error_is_set(&error));
    REQUIRE_INT(error.code, 13);
    REQUIRE(error.message[255] == '\0');

    ncm_error_clear(&error);
    REQUIRE(!ncm_error_is_set(&error));
    return;
}

static void
test_path(void) {
    REQUIRE_INT(ncm_path_basename_start(LIT_ARGS("/a/b/song.flac")), 5);
    REQUIRE_INT(ncm_path_basename_start(LIT_ARGS("song.flac")), 0);
    REQUIRE_INT(ncm_path_basename_start(LIT_ARGS("")), 0);

    REQUIRE_INT(ncm_path_parent_directory_len(LIT_ARGS("/a/b/song.flac")), 4);
    REQUIRE_INT(ncm_path_parent_directory_len(LIT_ARGS("song.flac")), 0);
    REQUIRE_INT(ncm_path_parent_directory_len(LIT_ARGS("/")), 0);

    REQUIRE_INT(ncm_path_extension_start(LIT_ARGS("/a/b/song.flac")), 10);
    REQUIRE_INT(ncm_path_extension_start(LIT_ARGS("/a/b/.hidden")), -1);
    REQUIRE_INT(ncm_path_extension_start(LIT_ARGS("/a/b/noext")), -1);

    REQUIRE(ncm_path_has_extension(LIT_ARGS("track.ogg"), LIT_ARGS("ogg")));
    REQUIRE(!ncm_path_has_extension(LIT_ARGS("track.ogg"), LIT_ARGS("mp3")));
    REQUIRE(!ncm_path_has_extension(LIT_ARGS("track"), LIT_ARGS("mp3")));
    return;
}

static void
test_string(void) {
    NcmBuffer escaped;
    char lowercase[] = "AbC-123-Ç";
    char invalid[] = "a/b:c*d?e";
    int32 invalid_len;

    ncm_string_lowercase_ascii(lowercase, STRLIT_LEN("AbC-123-Ç"));
    REQUIRE_STRING(lowercase, STRLIT_LEN("abc-123-Ç"), "abc-123-Ç");

    REQUIRE(ncm_string_equal(LIT_ARGS("same"), LIT_ARGS("same")));
    REQUIRE(!ncm_string_equal(LIT_ARGS("same"), LIT_ARGS("other")));
    REQUIRE(ncm_string_starts_with(LIT_ARGS("prefix"), LIT_ARGS("pre")));
    REQUIRE(!ncm_string_starts_with(LIT_ARGS("pre"), LIT_ARGS("prefix")));
    REQUIRE(ncm_string_ends_with(LIT_ARGS("track.flac"), LIT_ARGS("flac")));
    REQUIRE(!ncm_string_ends_with(LIT_ARGS("track.flac"), LIT_ARGS("mp3")));
    REQUIRE_INT(ncm_string_find_char(LIT_ARGS("abc"), 'b'), 1);
    REQUIRE_INT(ncm_string_find_char(LIT_ARGS("abc"), 'z'), -1);

    invalid_len = STRLIT_LEN("a/b:c*d?e");
    ncm_string_remove_chars(invalid, &invalid_len, LIT_ARGS("/:*?"));
    REQUIRE_STRING(invalid, invalid_len, "abcde");

    ncm_buffer_init(&escaped);
    ncm_string_append_shell_escaped_single_quotes(&escaped,
                                                  LIT_ARGS("can't stop"));
    require_buffer_string((char *)"escaped", &escaped,
                          LIT_ARGS("can'\\''t stop"));
    ncm_buffer_destroy(&escaped);
    return;
}

static void
test_html(void) {
    NcmBuffer result;

    result = ncm_html_unescape_entities(LIT_ARGS("a&amp;b&quot;c&nbsp;d"));
    require_buffer_string((char *)"entities", &result, LIT_ARGS("a&b\"c d"));
    ncm_buffer_destroy(&result);

    result = ncm_html_unescape_entities(
        LIT_ARGS("&apos;&lt;&gt;&ndash;&mdash;"));
    require_buffer_string((char *)"extra entities", &result,
                          LIT_ARGS("'<>–—"));
    ncm_buffer_destroy(&result);

    result = ncm_html_unescape_utf8(LIT_ARGS("&#65; &#x20ac; &#bad;"));
    require_buffer_string((char *)"numeric utf8", &result,
                          LIT_ARGS("A € &#bad;"));
    ncm_buffer_destroy(&result);

    result = ncm_html_unescape_utf8(LIT_ARGS("&#x110000; &#1114112;"));
    require_buffer_string((char *)"invalid numeric utf8", &result,
                          LIT_ARGS("&#x110000; &#1114112;"));
    ncm_buffer_destroy(&result);

    result = ncm_html_strip_tags(LIT_ARGS("a<p>b<br>c</p>&amp;"));
    require_buffer_string((char *)"strip tags", &result,
                          LIT_ARGS("a\nb\nc\n&"));
    ncm_buffer_destroy(&result);

    result = ncm_html_strip_tags(LIT_ARGS("<p class=x>a<br />b&lt;c"));
    require_buffer_string((char *)"strip tag variants", &result,
                          LIT_ARGS("\na\nb<c"));
    ncm_buffer_destroy(&result);

    result = ncm_html_strip_tags(LIT_ARGS("a <broken b\nc"));
    require_buffer_string((char *)"malformed tag", &result,
                          LIT_ARGS("a <broken bc"));
    ncm_buffer_destroy(&result);
    return;
}

static void
test_sample_buffer(void) {
    NcmSampleBuffer buffer;
    int16 first_samples[] = {1, 2, 3, 4};
    int16 second_samples[] = {5, 6, 7};
    int16 too_many_samples[] = {8, 9, 10, 11, 12};
    int16 dest[] = {-1, -1, -1};
    int16 copy_dest[] = {0, 0, 0, 0};
    int32 copied;

    ncm_sample_buffer_init(&buffer);
    ncm_sample_buffer_resize(&buffer, 4);
    REQUIRE_INT(ncm_sample_buffer_capacity(&buffer), 4);
    REQUIRE_INT(ncm_sample_buffer_size(&buffer), 0);

    REQUIRE(ncm_sample_buffer_put(&buffer, first_samples,
                                  NCM_ARRAY_LEN(first_samples)));
    REQUIRE_INT(ncm_sample_buffer_size(&buffer), 4);
    REQUIRE(!ncm_sample_buffer_put(&buffer, too_many_samples,
                                   NCM_ARRAY_LEN(too_many_samples)));
    REQUIRE_INT(ncm_sample_buffer_size(&buffer), 4);

    copied = ncm_sample_buffer_get(&buffer, 2, dest, NCM_ARRAY_LEN(dest));
    REQUIRE_INT(copied, 2);
    REQUIRE_INT(dest[0], -1);
    REQUIRE_INT(dest[1], 1);
    REQUIRE_INT(dest[2], 2);
    REQUIRE_INT(ncm_sample_buffer_size(&buffer), 2);

    REQUIRE(ncm_sample_buffer_put(&buffer, second_samples,
                                  NCM_ARRAY_LEN(second_samples)));
    REQUIRE_INT(ncm_sample_buffer_size(&buffer), 4);
    copied = ncm_sample_buffer_get(&buffer, 1000,
                                   copy_dest, NCM_ARRAY_LEN(copy_dest));
    REQUIRE_INT(copied, 4);
    REQUIRE_INT(copy_dest[0], 4);
    REQUIRE_INT(copy_dest[1], 5);
    REQUIRE_INT(copy_dest[2], 6);
    REQUIRE_INT(copy_dest[3], 7);
    REQUIRE_INT(ncm_sample_buffer_size(&buffer), 0);

    ncm_sample_buffer_destroy(&buffer);
    return;
}

static void
test_type_conversions(void) {
    char buffer[16];
    int32 len;

    len = ncm_channels_to_string(1, buffer, NCM_ARRAY_LEN(buffer));
    REQUIRE_STRING(buffer, len, "Mono");
    len = ncm_channels_to_string(2, buffer, NCM_ARRAY_LEN(buffer));
    REQUIRE_STRING(buffer, len, "Stereo");
    len = ncm_channels_to_string(6, buffer, NCM_ARRAY_LEN(buffer));
    REQUIRE_STRING(buffer, len, "6");
    len = ncm_channels_to_string(123456, buffer, 4);
    REQUIRE_STRING(buffer, len, "123");

    REQUIRE_INT(ncm_color_index_from_char('0'), 0);
    REQUIRE_INT(ncm_color_index_from_char('9'), 9);
    REQUIRE_INT(ncm_color_index_from_char('x'), -1);

    REQUIRE_CSTRING(ncm_tag_type_name(MPD_TAG_ARTIST), "Artist");
    REQUIRE_CSTRING(ncm_tag_type_name(MPD_TAG_ALBUM_ARTIST),
                    "Album Artist");
    REQUIRE_CSTRING(ncm_tag_type_name((enum mpd_tag_type)-999), "");

    REQUIRE(ncm_char_is_tag_type('a'));
    REQUIRE(!ncm_char_is_tag_type('x'));
    REQUIRE(ncm_char_to_tag_type('a') == MPD_TAG_ARTIST);
    REQUIRE(ncm_char_to_tag_type('A') == MPD_TAG_ALBUM_ARTIST);
    REQUIRE(ncm_char_to_tag_type('C') == MPD_TAG_COMMENT);

    REQUIRE_CSTRING(ncm_item_type_name(NCM_ITEM_DIRECTORY), "directory");
    REQUIRE_CSTRING(ncm_item_type_name(NCM_ITEM_SONG), "song");
    REQUIRE_CSTRING(ncm_item_type_name((enum NcmItemType)-1), "unknown");
    return;
}

static void
test_utf8(void) {
    char buffer[8];
    uint32 rune;
    int32 len;

    len = ncm_utf8_encode(0x20acu, buffer, NCM_ARRAY_LEN(buffer));
    REQUIRE_INT(len, 3);
    REQUIRE_STRING(buffer, len, "€");
    REQUIRE_INT(ncm_utf8_decode(buffer, len, &rune), 3);
    REQUIRE(rune == 0x20acu);

    REQUIRE_INT(ncm_utf8_characters(LIT_ARGS("a€b")), 3);
    REQUIRE_INT(ncm_utf8_next_position(LIT_ARGS("a€b"), 0), 1);
    REQUIRE_INT(ncm_utf8_next_position(LIT_ARGS("a€b"), 1), 4);
    REQUIRE_INT(ncm_utf8_cut_width(LIT_ARGS("abcdef"), 3), 3);
    return;
}

int
main(void) {
    test_buffer();
    test_error();
    test_path();
    test_string();
    test_html();
    test_sample_buffer();
    test_type_conversions();
    test_utf8();
    return EXIT_SUCCESS;
}
