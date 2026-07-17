#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "lyrics_fetcher.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define REQUIRE(COND) do {                                                \
    if (!(COND)) {                                                        \
        fail(__FILE__, __LINE__, (char *)#COND);                          \
    }                                                                     \
} while (0)

#define REQUIRE_STRING(ACTUAL, ACTUAL_LEN, EXPECTED)                      \
    require_string(__FILE__, __LINE__, (char *)#ACTUAL,                   \
                   ACTUAL, ACTUAL_LEN, LIT_ARGS(EXPECTED))

typedef struct LyricsTestIo {
    CURLcode code;
    char *response;
    int32 response_len;
} LyricsTestIo;

static void fail(char *file, int32 line, char *condition);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static CURLcode test_perform(NcmBuffer *data, char *url, int32 url_len,
                             char *referer, int32 referer_len,
                             bool follow_redirect, int32 timeout_seconds,
                             void *user);
static CURLcode test_escape(NcmBuffer *out, char *string, int32 string_len,
                            void *user);
static bool buffer_contains(NcmBuffer *buffer, char *needle,
                            int32 needle_len);
static void test_provider_registration(void);
static void test_url_construction(void);
static void test_html_cleanup(void);
static void test_failed_fetch(void);

static void
fail(char *file, int32 line, char *condition) {
    fprintf(stderr, "%s:%d: requirement failed: %s\n",
            file, line, condition);
    exit(EXIT_FAILURE);
}

static void
require_string(char *file, int32 line, char *name,
               char *actual, int32 actual_len,
               char *expected, int32 expected_len) {
    if (!STREQUAL(actual, actual_len, expected, expected_len)) {
        fprintf(stderr, "%s:%d: %s: expected '%.*s', got '%.*s'\n",
                file, line, name, expected_len, expected,
                actual_len, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static CURLcode
test_perform(NcmBuffer *data, char *url, int32 url_len,
             char *referer, int32 referer_len, bool follow_redirect,
             int32 timeout_seconds, void *user) {
    LyricsTestIo *io;

    (void)url;
    (void)url_len;
    (void)referer;
    (void)referer_len;
    (void)follow_redirect;
    (void)timeout_seconds;
    io = user;
    ncm_buffer_clear(data);
    if ((io->code == CURLE_OK) && (io->response != NULL)) {
        ncm_buffer_append(data, io->response, io->response_len);
    }
    return io->code;
}

static CURLcode
test_escape(NcmBuffer *out, char *string, int32 string_len, void *user) {
    (void)user;
    ncm_buffer_clear(out);
    for (int32 i = 0; i < string_len; i += 1) {
        if (string[i] == ' ') {
            ncm_buffer_append_byte(out, '+');
        } else {
            ncm_buffer_append_byte(out, string[i]);
        }
    }
    return CURLE_OK;
}

static bool
buffer_contains(NcmBuffer *buffer, char *needle, int32 needle_len) {
    for (int32 i = 0; i + needle_len <= buffer->len; i += 1) {
        if (ncm_string_starts_with(buffer->data + i, buffer->len - i,
                                   needle, needle_len)) {
            return true;
        }
    }
    return false;
}

static void
test_provider_registration(void) {
    NcmLyricsFetcherRegistry registry;

    ncm_lyrics_fetcher_registry_init(&registry);
    REQUIRE(ncm_lyrics_fetcher_registry_append_name(
        &registry, STRLIT_ARGS("tekstowo")));
    REQUIRE(ncm_lyrics_fetcher_registry_append_name(
        &registry, STRLIT_ARGS("internet")));
    REQUIRE(registry.fetchers.len == 2);
    REQUIRE_STRING(registry.fetchers.items[0].name,
                   registry.fetchers.items[0].name_len,
                   "tekstowo.pl");
    REQUIRE_STRING(registry.fetchers.items[1].name,
                   registry.fetchers.items[1].name_len,
                   "the Internet");
    ncm_lyrics_fetcher_registry_clear(&registry);
    REQUIRE(ncm_lyrics_fetcher_registry_add_defaults(&registry));
    REQUIRE(registry.fetchers.len > 0);
    ncm_lyrics_fetcher_registry_destroy(&registry);
    return;
}

static void
test_url_construction(void) {
    LyricsTestIo io;
    NcmLyricsFetcherDef fetcher;
    NcmBuffer url;

    io.code = CURLE_OK;
    io.response = NULL;
    io.response_len = 0;
    ncm_lyrics_fetcher_set_io_for_tests(test_perform, test_escape, &io);
    ncm_lyrics_fetcher_def_init(&fetcher);
    ncm_buffer_init(&url);
    REQUIRE(ncm_lyrics_fetcher_def_set_name(&fetcher,
                                            STRLIT_ARGS("justsomelyrics")));
    REQUIRE(ncm_lyrics_fetcher_build_url(&fetcher, &url,
                                         STRLIT_ARGS("A B"),
                                         STRLIT_ARGS("C D")));
    REQUIRE(buffer_contains(&url, STRLIT_ARGS("site:justsomelyrics.com")));
    REQUIRE(buffer_contains(&url, STRLIT_ARGS("A+B")));
    REQUIRE(buffer_contains(&url, STRLIT_ARGS("C+D")));
    ncm_buffer_destroy(&url);
    ncm_lyrics_fetcher_def_destroy(&fetcher);
    ncm_lyrics_fetcher_set_io_for_tests(NULL, NULL, NULL);
    return;
}

static void
test_html_cleanup(void) {
    NcmBuffer out;

    ncm_buffer_init(&out);
    ncm_lyrics_cleanup_html(
        &out,
        STRLIT_ARGS("<p>  a&nbsp; b  </p><br>\n\n <span>c</span>"));
    REQUIRE_STRING(out.data, out.len, "a  b\n\nc");
    ncm_buffer_destroy(&out);
    return;
}

static void
test_failed_fetch(void) {
    LyricsTestIo io;
    NcmLyricsFetcherDef fetcher;
    NcmLyricsResult result;

    io.code = CURLE_FAILED_INIT;
    io.response = NULL;
    io.response_len = 0;
    ncm_lyrics_fetcher_set_io_for_tests(test_perform, test_escape, &io);
    ncm_lyrics_fetcher_def_init(&fetcher);
    ncm_lyrics_result_init(&result);
    REQUIRE(ncm_lyrics_fetcher_def_set_name(&fetcher,
                                            STRLIT_ARGS("plyrics")));
    REQUIRE(ncm_lyrics_fetcher_fetch(&fetcher, &result,
                                     STRLIT_ARGS("artist"),
                                     STRLIT_ARGS("title"), NULL));
    REQUIRE(!result.success);
    REQUIRE(result.text_len > 0);
    ncm_lyrics_result_destroy(&result);
    ncm_lyrics_fetcher_def_destroy(&fetcher);
    ncm_lyrics_fetcher_set_io_for_tests(NULL, NULL, NULL);
    return;
}

int
main(void) {
    test_provider_registration();
    test_url_construction();
    test_html_cleanup();
    test_failed_fetch();
    return EXIT_SUCCESS;
}
