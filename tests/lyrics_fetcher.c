#if !defined(NCMPCPP_TESTS_LYRICS_FETCHER_C)
#define NCMPCPP_TESTS_LYRICS_FETCHER_C

#include <assert.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "cbase/util.c"
#include "c/ncm_string.h"

bool
ncm_string_starts_with(char *string, int32 string_len, char *prefix,
                       int32 prefix_len) {
    if ((string == NULL) || (prefix == NULL) || (prefix_len < 0)
        || (string_len < prefix_len)) {
        return false;
    }
    return memcmp64(string, prefix, prefix_len) == 0;
}

#include "c/ncm_html.c"
#include "curl_handle.h"

CURLcode
ncm_curl_perform(StrBuilder *data, char *url, int32 url_len, char *referer,
                 int32 referer_len, bool follow_redirect,
                 int32 timeout_seconds) {
    (void)data;
    (void)url;
    (void)url_len;
    (void)referer;
    (void)referer_len;
    (void)follow_redirect;
    (void)timeout_seconds;
    return CURLE_FAILED_INIT;
}

#include "../src/lyrics_fetcher.c"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

typedef struct LyricsFetcherTestCase {
    char *name;
    char *fixture;
    char *direct_url;
    char *bad_page_url;
    char *page_url;
    char *search_url;
    char *expected_text;

    int32 name_len;
    int32 direct_url_len;
    int32 bad_page_url_len;
    int32 page_url_len;
    int32 search_url_len;
    int32 expected_text_len;
    bool success;
} LyricsFetcherTestCase;

typedef struct LyricsFetcherTestContext {
    LyricsFetcherTestCase *test;
    int32 calls;
} LyricsFetcherTestContext;

#define LYRICS_TEST(NAME, FIXTURE, DIRECT_URL, BAD_PAGE_URL, PAGE_URL, \
                    SEARCH_URL, EXPECTED_TEXT, SUCCESS) { \
    .name = NAME, \
    .fixture = FIXTURE, \
    .direct_url = DIRECT_URL, \
    .bad_page_url = BAD_PAGE_URL, \
    .page_url = PAGE_URL, \
    .search_url = SEARCH_URL, \
    .expected_text = EXPECTED_TEXT, \
    .name_len = STRLIT_LEN(NAME), \
    .direct_url_len = STRLIT_LEN(DIRECT_URL), \
    .bad_page_url_len = STRLIT_LEN(BAD_PAGE_URL), \
    .page_url_len = STRLIT_LEN(PAGE_URL), \
    .search_url_len = STRLIT_LEN(SEARCH_URL), \
    .expected_text_len = STRLIT_LEN(EXPECTED_TEXT), \
    .success = SUCCESS, \
}

static LyricsFetcherTestCase lyrics_tests[] = {
    LYRICS_TEST(
        "azlyrics",
        "tests/lyrics/azlyrics.html",
        "https://www.azlyrics.com/lyrics/luisfonsi/despacito.html",
        "https://www.azlyrics.com/lyrics/luisfonsi/otra.html",
        "https://www.azlyrics.com/lyrics/luisfonsi/despacito.html",
        "https://www.google.com/search?hl=en&q=site%3Aazlyrics.com+"
        "luis+fonsi+despacito+lyrics",
        "Access denied",
        false),
    LYRICS_TEST(
        "genius",
        "tests/lyrics/genius.html",
        "https://genius.com/luis-fonsi-despacito-lyrics",
        "https://genius.com/Luis-fonsi-otra-lyrics",
        "https://genius.com/Luis-fonsi-despacito-lyrics",
        "https://www.google.com/search?hl=en&q=site%3Agenius.com+"
        "luis+fonsi+despacito+lyrics",
        "Ohne dich",
        true),
    LYRICS_TEST(
        "letras",
        "tests/lyrics/letrasmus.html",
        "https://www.letras.mus.br/luis-fonsi/despacito/",
        "https://www.letras.mus.br/luis-fonsi/outra/",
        "https://www.letras.mus.br/luis-fonsi/despacito/",
        "https://www.google.com/search?hl=en&q=site%3Aletras.mus.br+"
        "luis+fonsi+despacito+lyrics",
        "Ohne dich",
        true),
    LYRICS_TEST(
        "musixmatch",
        "tests/lyrics/musixmatch.html",
        "https://www.musixmatch.com/lyrics/luis-fonsi/despacito",
        "https://www.musixmatch.com/lyrics/Luis-Fonsi/Otra",
        "https://www.musixmatch.com/lyrics/Luis-Fonsi/Despacito",
        "https://www.google.com/search?hl=en&q=site%3Amusixmatch.com+"
        "luis+fonsi+despacito+lyrics",
        "Ohne dich",
        true),
    LYRICS_TEST(
        "tekstowo",
        "tests/lyrics/tekstowo.html",
        "https://www.tekstowo.pl/luis-fonsi/despacito",
        "https://www.tekstowo.pl/luis-fonsi/outra",
        "https://www.tekstowo.pl/luis-fonsi/"
        "despacito-ft-daddy-yankee",
        "https://www.google.com/search?hl=en&q=site%3Atekstowo.pl+"
        "luis+fonsi+despacito+lyrics",
        "Ohne dich",
        true),
    LYRICS_TEST(
        "vagalume",
        "tests/lyrics/vagalume.html",
        "https://www.vagalume.com.br/luis-fonsi/despacito.html",
        "https://www.vagalume.com.br/luis-fonsi/outra.html",
        "https://www.vagalume.com.br/luis-fonsi/"
        "despacito-feat-daddy-yankee.html",
        "https://www.google.com/search?hl=en&q=site%3Avagalume.com.br+"
        "luis+fonsi+despacito+lyrics",
        "Ohne dich",
        true),
};

#undef LYRICS_TEST

static void
lyrics_test_set_download(NcmLyricsCurlPerformFn perform, void *user) {
    lyrics_test_perform = perform;
    lyrics_test_user = user;
    return;
}

static void
lyrics_test_append_fixture(StrBuilder *data, LyricsFetcherTestCase *test) {
    char *html;
    int32 html_len;

    html = read_entire_file(test->fixture, &html_len);
    sb_clear(data);
    sb_append(data, html, html_len);
    free2(html, html_len + 1);
    return;
}

static CURLcode
lyrics_test_direct_download(StrBuilder *data, char *url, int32 url_len,
                            char *referer, int32 referer_len,
                            bool follow_redirect, int32 timeout_seconds,
                            void *user) {
    LyricsFetcherTestContext *context;
    LyricsFetcherTestCase *test;

    context = user;
    assert(context != NULL);
    assert(context->test != NULL);
    test = context->test;
    context->calls += 1;
    assert(context->calls == 1);
    assert(STREQUAL(url, url_len, test->direct_url, test->direct_url_len));
    assert(referer == NULL);
    assert(referer_len == 0);
    assert(follow_redirect);
    assert(timeout_seconds == 15);
    lyrics_test_append_fixture(data, test);
    return CURLE_OK;
}

static void
lyrics_test_append_search_link(StrBuilder *data, char *url, int32 url_len) {
    sb_append(data, STRLIT_ARGS("<a href=\"/url?q=&amp;url="));
    lyrics_append_query(data, url, url_len);
    sb_append(data, STRLIT_ARGS("&amp;sa=U\">lyrics</a>"));
    return;
}

static CURLcode
lyrics_test_download(StrBuilder *data, char *url, int32 url_len, char *referer,
                     int32 referer_len, bool follow_redirect,
                     int32 timeout_seconds, void *user) {
    LyricsFetcherTestContext *context;
    LyricsFetcherTestCase *test;

    context = user;
    assert(context != NULL);
    assert(context->test != NULL);
    test = context->test;
    context->calls += 1;
    assert(follow_redirect);
    assert(timeout_seconds == 15);

    if (context->calls == 1) {
        assert(STREQUAL(url, url_len, test->direct_url,
                        test->direct_url_len));
        assert(referer == NULL);
        assert(referer_len == 0);
        return CURLE_HTTP_RETURNED_ERROR;
    }
    if (context->calls == 2) {
        assert(STREQUAL(url, url_len, test->search_url,
                        test->search_url_len));
        assert(referer == NULL);
        assert(referer_len == 0);
        sb_clear(data);
        sb_append(data,
                  STRLIT_ARGS("<a href=\"https://example.com/luis-fonsi/"
                              "despacito\">wrong domain</a>"));
        lyrics_test_append_search_link(data, test->bad_page_url,
                                       test->bad_page_url_len);
        lyrics_test_append_search_link(data, test->bad_page_url,
                                       test->bad_page_url_len);
        lyrics_test_append_search_link(data, test->page_url,
                                       test->page_url_len);
        return CURLE_OK;
    }

    assert(context->calls == 3);
    assert(STREQUAL(url, url_len, test->page_url, test->page_url_len));
    assert(STREQUAL(referer, referer_len, test->search_url,
                    test->search_url_len));
    lyrics_test_append_fixture(data, test);
    return CURLE_OK;
}

static CURLcode
lyrics_test_direct_candidate_download(
    StrBuilder *data, char *url, int32 url_len, char *referer,
    int32 referer_len, bool follow_redirect, int32 timeout_seconds, void *user
) {
    LyricsFetcherTestContext *context;

    context = user;
    assert(context != NULL);
    assert(context->test != NULL);
    context->calls += 1;
    assert(follow_redirect);
    assert(timeout_seconds == 15);
    assert(referer == NULL);
    assert(referer_len == 0);

    if (context->calls == 1) {
        assert(STREQUAL(
            url, url_len,
            STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben/"
                        "que-nega-e-essa.html")));
        return CURLE_HTTP_RETURNED_ERROR;
    }

    assert(context->calls == 2);
    assert(STREQUAL(
        url, url_len,
        STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben/"
                    "que-nega-%C3%A9-essa.html")));
    lyrics_test_append_fixture(data, context->test);
    return CURLE_OK;
}

static CURLcode
lyrics_test_jorge_ben_search_download(
    StrBuilder *data, char *url, int32 url_len, char *referer,
    int32 referer_len, bool follow_redirect, int32 timeout_seconds, void *user
) {
    LyricsFetcherTestContext *context;

    context = user;
    assert(context != NULL);
    assert(context->test != NULL);
    context->calls += 1;
    assert(follow_redirect);
    assert(timeout_seconds == 15);

    if (context->calls == 1) {
        assert(STREQUAL(
            url, url_len,
            STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben/"
                        "que-nega-e-essa.html")));
        assert(referer == NULL);
        assert(referer_len == 0);
        return CURLE_HTTP_RETURNED_ERROR;
    }
    if (context->calls == 2) {
        assert(STREQUAL(
            url, url_len,
            STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben/"
                        "que-nega-%C3%A9-essa.html")));
        assert(referer == NULL);
        assert(referer_len == 0);
        return CURLE_HTTP_RETURNED_ERROR;
    }
    if (context->calls == 3) {
        assert(STREQUAL(
            url, url_len,
            STRLIT_ARGS("https://www.google.com/search?hl=en&q=site%3A"
                        "vagalume.com.br+Jorge+Ben+Que+nega+%C3%A9+"
                        "essa+lyrics")));
        assert(referer == NULL);
        assert(referer_len == 0);
        sb_clear(data);
        lyrics_test_append_search_link(
            data,
            STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben-jor/"
                        "mas-que-nada.html"));
        lyrics_test_append_search_link(
            data,
            STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben-jor/"
                        "que-nega-e-essa.html"));
        return CURLE_OK;
    }

    assert(context->calls == 4);
    assert(STREQUAL(
        url, url_len,
        STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben-jor/"
                    "que-nega-e-essa.html")));
    assert(STREQUAL(
        referer, referer_len,
        STRLIT_ARGS("https://www.google.com/search?hl=en&q=site%3A"
                    "vagalume.com.br+Jorge+Ben+Que+nega+%C3%A9+"
                    "essa+lyrics")));
    lyrics_test_append_fixture(data, context->test);
    return CURLE_OK;
}

static CURLcode
lyrics_test_timeout(StrBuilder *data, char *url, int32 url_len, char *referer,
                    int32 referer_len, bool follow_redirect,
                    int32 timeout_seconds, void *user) {
    int32 *calls;

    (void)data;
    (void)url;
    (void)url_len;
    (void)referer;
    (void)referer_len;
    (void)follow_redirect;
    (void)timeout_seconds;
    calls = user;
    *calls += 1;
    return CURLE_OPERATION_TIMEDOUT;
}

static void
lyrics_test_assert_result(LyricsFetcherTestCase *test,
                          NcmLyricsResult *result) {
    assert(result->success == test->success);
    assert(result->text != NULL);
    assert(memmem64(result->text, result->text_len, test->expected_text,
                    test->expected_text_len)
           != NULL);
    if (result->success) {
        assert(result->text_len > 100);
        assert(memmem64(result->text, result->text_len,
                        STRLIT_ARGS("<div"))
               == NULL);
        assert(memmem64(result->text, result->text_len,
                        STRLIT_ARGS("<br"))
               == NULL);
    }
    return;
}

static void
lyrics_test_assert_first_direct_url(
    NcmLyricsFetcherDef *fetcher, char *artist, int32 artist_len,
    char *title, int32 title_len, char *expected, int32 expected_len
) {
    StrBuilderArray urls;

    str_builder_array_init(&urls);
    assert(lyrics_collect_direct_urls(fetcher, &urls, artist, artist_len,
                                      title, title_len));
    assert(urls.len > 0);
    assert(STREQUAL(urls.items[0].data, urls.items[0].len,
                    expected, expected_len));
    str_builder_array_destroy(&urls);
    return;
}

static void
test_registry_has_only_supported_fetchers(void) {
    NcmLyricsFetcherRegistry registry;
    char *removed[] = {
        "justsomelyrics",
        "jahlyrics",
        "plyrics",
        "zeneszoveg",
        "tags",
    };

    ncm_lyrics_fetcher_registry_init(&registry);
    for (int32 i = 0; i < LENGTH(lyrics_tests); i += 1) {
        assert(ncm_lyrics_fetcher_registry_append_name(
            &registry, lyrics_tests[i].name, lyrics_tests[i].name_len));
    }
    assert(ncm_lyrics_fetcher_registry_append_name(
        &registry, STRLIT_ARGS("internet")));
    assert(registry.fetchers.len == LENGTH(lyrics_tests) + 1);

    for (int32 i = 0; i < LENGTH(removed); i += 1) {
        assert(!ncm_lyrics_fetcher_registry_append_name(
            &registry, removed[i], strlen32(removed[i])));
    }
    assert(registry.fetchers.len == LENGTH(lyrics_tests) + 1);
    ncm_lyrics_fetcher_registry_destroy(&registry);
    return;
}

static void
test_site_fetchers_direct_download_and_parse_fixtures(void) {
    for (int32 i = 0; i < LENGTH(lyrics_tests); i += 1) {
        LyricsFetcherTestContext context;
        NcmLyricsFetcherDef fetcher;
        NcmLyricsResult result;

        context.test = &lyrics_tests[i];
        context.calls = 0;
        ncm_lyrics_fetcher_def_init(&fetcher);
        ncm_lyrics_result_init(&result);
        assert(ncm_lyrics_fetcher_def_set_name(
            &fetcher, context.test->name, context.test->name_len));

        lyrics_test_set_download(lyrics_test_direct_download, &context);
        assert(ncm_lyrics_fetcher_fetch(
            &fetcher, &result, STRLIT_ARGS("luis fonsi"),
            STRLIT_ARGS("despacito")));
        assert(context.calls == 1);
        lyrics_test_assert_result(context.test, &result);

        ncm_lyrics_result_destroy(&result);
        ncm_lyrics_fetcher_def_destroy(&fetcher);
    }
    lyrics_test_set_download(NULL, NULL);
    return;
}

static void
test_site_fetchers_search_download_and_parse_fixtures(void) {
    for (int32 i = 0; i < LENGTH(lyrics_tests); i += 1) {
        LyricsFetcherTestContext context;
        NcmLyricsFetcherDef fetcher;
        NcmLyricsResult result;
        StrBuilder search_url;

        context.test = &lyrics_tests[i];
        context.calls = 0;
        ncm_lyrics_fetcher_def_init(&fetcher);
        ncm_lyrics_result_init(&result);
        sb_init(&search_url);

        assert(ncm_lyrics_fetcher_def_set_name(
            &fetcher, context.test->name, context.test->name_len));
        lyrics_test_assert_first_direct_url(
            &fetcher, STRLIT_ARGS("luis fonsi"), STRLIT_ARGS("despacito"),
            context.test->direct_url, context.test->direct_url_len);
        assert(ncm_lyrics_fetcher_build_url(
            &fetcher, &search_url, STRLIT_ARGS("luis fonsi"),
            STRLIT_ARGS("despacito")));
        assert(STREQUAL(search_url.data, search_url.len,
                        context.test->search_url,
                        context.test->search_url_len));

        lyrics_test_set_download(lyrics_test_download, &context);
        assert(ncm_lyrics_fetcher_fetch(
            &fetcher, &result, STRLIT_ARGS("luis fonsi"),
            STRLIT_ARGS("despacito")));
        assert(context.calls == 3);
        lyrics_test_assert_result(context.test, &result);

        sb_free(&search_url);
        ncm_lyrics_result_destroy(&result);
        ncm_lyrics_fetcher_def_destroy(&fetcher);
    }
    lyrics_test_set_download(NULL, NULL);
    return;
}

static void
test_provider_aware_slug_normalization(void) {
    NcmLyricsFetcherDef fetcher;

    ncm_lyrics_fetcher_def_init(&fetcher);

    assert(ncm_lyrics_fetcher_def_set_name(&fetcher,
                                           STRLIT_ARGS("vagalume")));
    lyrics_test_assert_first_direct_url(
        &fetcher, STRLIT_ARGS("Jorge Ben"),
        STRLIT_ARGS("Que nega é essa"),
        STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben/"
                    "que-nega-e-essa.html"));
    assert(lyrics_search_candidate_score(
        &fetcher,
        STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben-jor/"
                    "que-nega-e-essa.html"),
        STRLIT_ARGS("Jorge Ben"), STRLIT_ARGS("Que nega é essa")) > 0);
    assert(lyrics_search_candidate_score(
        &fetcher,
        STRLIT_ARGS("https://www.vagalume.com.br/jorge-ben-jor/"
                    "mas-que-nada.html"),
        STRLIT_ARGS("Jorge Ben"), STRLIT_ARGS("Que nega é essa")) == 0);

    assert(ncm_lyrics_fetcher_def_set_name(&fetcher,
                                           STRLIT_ARGS("azlyrics")));
    lyrics_test_assert_first_direct_url(
        &fetcher, STRLIT_ARGS("Beyoncé & Jay-Z"), STRLIT_ARGS("Déjà Vu"),
        STRLIT_ARGS("https://www.azlyrics.com/lyrics/beyoncejayz/"
                    "dejavu.html"));

    ncm_lyrics_fetcher_def_destroy(&fetcher);
    return;
}

static void
test_site_fetcher_tries_multiple_direct_urls(void) {
    LyricsFetcherTestContext context;
    NcmLyricsFetcherDef fetcher;
    NcmLyricsResult result;

    context.test = &lyrics_tests[5];
    context.calls = 0;
    ncm_lyrics_fetcher_def_init(&fetcher);
    ncm_lyrics_result_init(&result);

    assert(ncm_lyrics_fetcher_def_set_name(&fetcher,
                                           STRLIT_ARGS("vagalume")));
    lyrics_test_set_download(lyrics_test_direct_candidate_download, &context);
    assert(ncm_lyrics_fetcher_fetch(
        &fetcher, &result, STRLIT_ARGS("Jorge Ben"),
        STRLIT_ARGS("Que nega é essa")));
    assert(context.calls == 2);
    lyrics_test_assert_result(context.test, &result);

    lyrics_test_set_download(NULL, NULL);
    ncm_lyrics_result_destroy(&result);
    ncm_lyrics_fetcher_def_destroy(&fetcher);
    return;
}

static void
test_vagalume_search_finds_jorge_ben_jor_alias(void) {
    LyricsFetcherTestContext context;
    NcmLyricsFetcherDef fetcher;
    NcmLyricsResult result;

    context.test = &lyrics_tests[5];
    context.calls = 0;
    ncm_lyrics_fetcher_def_init(&fetcher);
    ncm_lyrics_result_init(&result);

    assert(ncm_lyrics_fetcher_def_set_name(&fetcher,
                                           STRLIT_ARGS("vagalume")));
    lyrics_test_set_download(lyrics_test_jorge_ben_search_download, &context);
    assert(ncm_lyrics_fetcher_fetch(
        &fetcher, &result, STRLIT_ARGS("Jorge Ben"),
        STRLIT_ARGS("Que nega é essa")));
    assert(context.calls == 4);
    lyrics_test_assert_result(context.test, &result);

    lyrics_test_set_download(NULL, NULL);
    ncm_lyrics_result_destroy(&result);
    ncm_lyrics_fetcher_def_destroy(&fetcher);
    return;
}

static void
test_internet_fetcher_returns_search_url_without_download(void) {
    NcmLyricsFetcherDef fetcher;
    NcmLyricsResult result;
    StrBuilder url;

    ncm_lyrics_fetcher_def_init(&fetcher);
    ncm_lyrics_result_init(&result);
    sb_init(&url);
    assert(ncm_lyrics_fetcher_def_set_name(&fetcher,
                                           STRLIT_ARGS("internet")));
    assert(ncm_lyrics_fetcher_build_url(
        &fetcher, &url, STRLIT_ARGS("luis fonsi"),
        STRLIT_ARGS("despacito")));
    assert(STREQUAL(
        url.data, url.len,
        STRLIT_ARGS(
            "https://www.google.com/search?hl=en&q=lyrics+luis+fonsi+"
            "despacito")));
    assert(ncm_lyrics_fetcher_fetch(
        &fetcher, &result, STRLIT_ARGS("luis fonsi"),
        STRLIT_ARGS("despacito")));
    assert(!result.success);
    assert(memmem64(result.text, result.text_len, url.data, url.len) != NULL);

    sb_free(&url);
    ncm_lyrics_result_destroy(&result);
    ncm_lyrics_fetcher_def_destroy(&fetcher);
    return;
}

static void
test_transport_error_is_reported(void) {
    NcmLyricsFetcherDef fetcher;
    NcmLyricsResult result;
    int32 calls;

    calls = 0;
    ncm_lyrics_fetcher_def_init(&fetcher);
    ncm_lyrics_result_init(&result);
    assert(ncm_lyrics_fetcher_def_set_name(&fetcher,
                                           STRLIT_ARGS("genius")));
    lyrics_test_set_download(lyrics_test_timeout, &calls);
    assert(ncm_lyrics_fetcher_fetch(
        &fetcher, &result, STRLIT_ARGS("luis fonsi"),
        STRLIT_ARGS("despacito")));
    assert(calls == 1);
    assert(!result.success);
    assert(STREQUAL(result.text, result.text_len,
                    STRLIT_ARGS("Request timed out")));

    lyrics_test_set_download(NULL, NULL);
    ncm_lyrics_result_destroy(&result);
    ncm_lyrics_fetcher_def_destroy(&fetcher);
    return;
}

int
main(void) {
    test_registry_has_only_supported_fetchers();
    test_site_fetchers_direct_download_and_parse_fixtures();
    test_site_fetchers_search_download_and_parse_fixtures();
    test_provider_aware_slug_normalization();
    test_site_fetcher_tries_multiple_direct_urls();
    test_vagalume_search_finds_jorge_ben_jor_alias();
    test_internet_fetcher_returns_search_url_without_download();
    test_transport_error_is_reported();
    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_TESTS_LYRICS_FETCHER_C */
