#include "lyrics_fetcher.h"

#include <errno.h>
#include <string.h>

#if defined(HAVE_TAGLIB_H)
#include "c/ncm_tags.h"
#endif

#include "c/ncm_base.h"
#include "c/ncm_html.h"
#include "c/ncm_regex.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"

#define LYRICS_MSG_NOT_FOUND "Not found"

static NcmLyricsCurlPerformFn lyrics_test_perform;
static NcmLyricsCurlEscapeFn lyrics_test_escape;
static void *lyrics_test_user;

static NcmArrayItemCallbacks lyrics_fetcher_callbacks;

static bool lyrics_string_set(char **data, int32 *len, int32 *cap,
                              char *source, int32 source_len);
static void lyrics_string_destroy(char **data, int32 *len, int32 *cap);
static void lyrics_fetcher_array_init_item(void *item);
static void lyrics_fetcher_array_destroy_item(void *item);
static bool lyrics_fetcher_array_copy_item(void *dest, void *source);
static void lyrics_fetcher_array_move_item(void *dest, void *source);
static bool lyrics_name_to_type(char *name, int32 name_len,
                                enum NcmLyricsFetcherType *type);
static char *lyrics_type_name(enum NcmLyricsFetcherType type, int32 *len);
static char *lyrics_type_site(enum NcmLyricsFetcherType type, int32 *len);
static char *lyrics_type_regex(enum NcmLyricsFetcherType type, int32 *len);
static bool lyrics_type_is_google(enum NcmLyricsFetcherType type);
static bool lyrics_type_is_available(enum NcmLyricsFetcherType type);
static CURLcode lyrics_curl_perform(NcmBuffer *data, char *url,
                                    int32 url_len, char *referer,
                                    int32 referer_len, bool follow_redirect,
                                    int32 timeout_seconds);
static CURLcode lyrics_curl_escape(NcmBuffer *out, char *string,
                                   int32 string_len);
static void lyrics_append_escaped(NcmBuffer *buffer, char *string,
                                  int32 string_len);
static void lyrics_append_url_replaced(NcmBuffer *buffer, char *template,
                                       int32 template_len, char *artist,
                                       int32 artist_len, char *title,
                                       int32 title_len);
static int32 lyrics_find(char *data, int32 data_len, char *needle,
                         int32 needle_len, int32 start);
static bool lyrics_extract_between(NcmBuffer *out, char *data,
                                   int32 data_len, char *start,
                                   int32 start_len, char *end,
                                   int32 end_len);
static bool lyrics_extract_after_until(NcmBuffer *out, char *data,
                                       int32 data_len, char *start,
                                       int32 start_len, char *after,
                                       int32 after_len, char *end,
                                       int32 end_len);
static void lyrics_extract_content(NcmLyricsFetcherDef *fetcher,
                                   NcmBuffer *out, char *data,
                                   int32 data_len);
static bool lyrics_regex_url_match(int32 start, int32 len, void *user);
static bool lyrics_extract_google_url(NcmBuffer *out, char *data,
                                      int32 data_len);
static bool lyrics_url_ok(NcmLyricsFetcherDef *fetcher, char *url,
                          int32 url_len);
static bool lyrics_fetch_direct(NcmLyricsFetcherDef *fetcher,
                                NcmLyricsResult *result, NcmBuffer *url,
                                char *referer, int32 referer_len);
static bool lyrics_fetch_google(NcmLyricsFetcherDef *fetcher,
                                NcmLyricsResult *result, char *artist,
                                int32 artist_len, char *title,
                                int32 title_len, NcmSong *song);
static bool lyrics_fetch_tags(NcmLyricsResult *result, NcmSong *song);
static void lyrics_trim_view(char **data, int32 *len);
static void lyrics_trim_buffer(NcmBuffer *buffer);
static void lyrics_append_clean_lines(NcmBuffer *out, char *data,
                                      int32 data_len);

static NcmArrayItemCallbacks lyrics_fetcher_callbacks = {
    .init = lyrics_fetcher_array_init_item,
    .destroy = lyrics_fetcher_array_destroy_item,
    .copy = lyrics_fetcher_array_copy_item,
    .move = lyrics_fetcher_array_move_item,
};

NCM_ARRAY_DEFINE(ncm_lyrics_fetcher_array,
                 NcmLyricsFetcherArray,
                 NcmLyricsFetcherDef,
                 &lyrics_fetcher_callbacks)

static bool
lyrics_string_set(char **data, int32 *len, int32 *cap,
                  char *source, int32 source_len) {
    char *new_data;
    int32 new_cap;

    lyrics_string_destroy(data, len, cap);
    if ((source == NULL) || (source_len <= 0)) {
        return true;
    }

    new_cap = source_len + 1;
    new_data = ncm_malloc(new_cap);
    ncm_memcpy(new_data, source, source_len);
    new_data[source_len] = '\0';
    *data = new_data;
    *len = source_len;
    *cap = new_cap;
    return true;
}

static void
lyrics_string_destroy(char **data, int32 *len, int32 *cap) {
    if (*data != NULL) {
        ncm_free(*data, *cap);
    }
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}

void
ncm_lyrics_result_init(NcmLyricsResult *result) {
    result->text = NULL;
    result->text_len = 0;
    result->text_cap = 0;
    result->success = false;
    return;
}

void
ncm_lyrics_result_destroy(NcmLyricsResult *result) {
    if (result == NULL) {
        return;
    }
    lyrics_string_destroy(&result->text, &result->text_len,
                          &result->text_cap);
    result->success = false;
    return;
}

void
ncm_lyrics_result_clear(NcmLyricsResult *result) {
    if (result == NULL) {
        return;
    }
    lyrics_string_destroy(&result->text, &result->text_len,
                          &result->text_cap);
    result->success = false;
    return;
}

bool
ncm_lyrics_result_set(NcmLyricsResult *result, bool success,
                      char *text, int32 text_len) {
    if (result == NULL) {
        return false;
    }
    if (!lyrics_string_set(&result->text, &result->text_len,
                           &result->text_cap, text, text_len)) {
        return false;
    }
    result->success = success;
    return true;
}

void
ncm_lyrics_fetcher_def_init(NcmLyricsFetcherDef *fetcher) {
    fetcher->name = NULL;
    fetcher->url_template = NULL;
    fetcher->match_regex = NULL;
    fetcher->name_len = 0;
    fetcher->name_cap = 0;
    fetcher->url_template_len = 0;
    fetcher->url_template_cap = 0;
    fetcher->match_regex_len = 0;
    fetcher->match_regex_cap = 0;
    fetcher->type = NCM_LYRICS_FETCHER_UNKNOWN;
    fetcher->enabled = false;
    return;
}

void
ncm_lyrics_fetcher_def_destroy(NcmLyricsFetcherDef *fetcher) {
    if (fetcher == NULL) {
        return;
    }
    lyrics_string_destroy(&fetcher->name, &fetcher->name_len,
                          &fetcher->name_cap);
    lyrics_string_destroy(&fetcher->url_template,
                          &fetcher->url_template_len,
                          &fetcher->url_template_cap);
    lyrics_string_destroy(&fetcher->match_regex,
                          &fetcher->match_regex_len,
                          &fetcher->match_regex_cap);
    fetcher->type = NCM_LYRICS_FETCHER_UNKNOWN;
    fetcher->enabled = false;
    return;
}

bool
ncm_lyrics_fetcher_def_copy(NcmLyricsFetcherDef *dest,
                            NcmLyricsFetcherDef *source) {
    NcmLyricsFetcherDef tmp;

    if ((dest == NULL) || (source == NULL)) {
        return false;
    }

    ncm_lyrics_fetcher_def_init(&tmp);
    tmp.type = source->type;
    tmp.enabled = source->enabled;
    if (!lyrics_string_set(&tmp.name, &tmp.name_len, &tmp.name_cap,
                           source->name, source->name_len)) {
        ncm_lyrics_fetcher_def_destroy(&tmp);
        return false;
    }
    if (!lyrics_string_set(&tmp.url_template, &tmp.url_template_len,
                           &tmp.url_template_cap, source->url_template,
                           source->url_template_len)) {
        ncm_lyrics_fetcher_def_destroy(&tmp);
        return false;
    }
    if (!lyrics_string_set(&tmp.match_regex, &tmp.match_regex_len,
                           &tmp.match_regex_cap, source->match_regex,
                           source->match_regex_len)) {
        ncm_lyrics_fetcher_def_destroy(&tmp);
        return false;
    }
    ncm_lyrics_fetcher_def_destroy(dest);
    *dest = tmp;
    return true;
}

void
ncm_lyrics_fetcher_def_move(NcmLyricsFetcherDef *dest,
                            NcmLyricsFetcherDef *source) {
    if ((dest == NULL) || (source == NULL)) {
        return;
    }
    ncm_lyrics_fetcher_def_destroy(dest);
    *dest = *source;
    ncm_lyrics_fetcher_def_init(source);
    return;
}

bool
ncm_lyrics_fetcher_def_set_name(NcmLyricsFetcherDef *fetcher,
                                char *name, int32 name_len) {
    enum NcmLyricsFetcherType type;
    char *display_name;
    char *site;
    char *match_regex;
    int32 display_name_len;
    int32 site_len;
    int32 match_regex_len;

    if ((fetcher == NULL) || (name == NULL) || (name_len <= 0)) {
        return false;
    }
    if (!lyrics_name_to_type(name, name_len, &type)) {
        return false;
    }
    if (!lyrics_type_is_available(type)) {
        return false;
    }

    display_name = lyrics_type_name(type, &display_name_len);
    site = lyrics_type_site(type, &site_len);
    match_regex = lyrics_type_regex(type, &match_regex_len);

    ncm_lyrics_fetcher_def_destroy(fetcher);
    ncm_lyrics_fetcher_def_init(fetcher);
    fetcher->type = type;
    fetcher->enabled = true;
    lyrics_string_set(&fetcher->name, &fetcher->name_len,
                      &fetcher->name_cap, display_name, display_name_len);
    lyrics_string_set(&fetcher->url_template, &fetcher->url_template_len,
                      &fetcher->url_template_cap, site, site_len);
    lyrics_string_set(&fetcher->match_regex, &fetcher->match_regex_len,
                      &fetcher->match_regex_cap, match_regex,
                      match_regex_len);
    return true;
}

char *
ncm_lyrics_fetcher_name(NcmLyricsFetcherDef *fetcher) {
    if ((fetcher == NULL) || (fetcher->name == NULL)) {
        return (char *)"";
    }
    return fetcher->name;
}

int32
ncm_lyrics_fetcher_name_len(NcmLyricsFetcherDef *fetcher) {
    if (fetcher == NULL) {
        return 0;
    }
    return fetcher->name_len;
}

void
ncm_lyrics_fetcher_registry_init(NcmLyricsFetcherRegistry *registry) {
    ncm_lyrics_fetcher_array_init(&registry->fetchers);
    return;
}

void
ncm_lyrics_fetcher_registry_destroy(NcmLyricsFetcherRegistry *registry) {
    if (registry == NULL) {
        return;
    }
    ncm_lyrics_fetcher_array_destroy(&registry->fetchers);
    return;
}

void
ncm_lyrics_fetcher_registry_clear(NcmLyricsFetcherRegistry *registry) {
    if (registry == NULL) {
        return;
    }
    ncm_lyrics_fetcher_array_clear(&registry->fetchers);
    return;
}

NcmLyricsFetcherDef *
ncm_lyrics_fetcher_registry_append(NcmLyricsFetcherRegistry *registry) {
    if (registry == NULL) {
        return NULL;
    }
    return ncm_lyrics_fetcher_array_append(&registry->fetchers);
}

bool
ncm_lyrics_fetcher_registry_append_name(NcmLyricsFetcherRegistry *registry,
                                        char *name, int32 name_len) {
    NcmLyricsFetcherDef *fetcher;

    fetcher = ncm_lyrics_fetcher_registry_append(registry);
    if (fetcher == NULL) {
        return false;
    }
    if (!ncm_lyrics_fetcher_def_set_name(fetcher, name, name_len)) {
        registry->fetchers.len -= 1;
        ncm_lyrics_fetcher_def_destroy(fetcher);
        return false;
    }
    return true;
}

bool
ncm_lyrics_fetcher_registry_add_defaults(NcmLyricsFetcherRegistry *registry) {
#if defined(HAVE_TAGLIB_H)
    if (!ncm_lyrics_fetcher_registry_append_name(registry,
                                                 STRLIT_ARGS("tags"))) {
        return false;
    }
#endif
    if (!ncm_lyrics_fetcher_registry_append_name(registry,
                                                 STRLIT_ARGS("tekstowo"))) {
        return false;
    }
    if (!ncm_lyrics_fetcher_registry_append_name(registry,
                                                 STRLIT_ARGS("plyrics"))) {
        return false;
    }
    if (!ncm_lyrics_fetcher_registry_append_name(
            registry, STRLIT_ARGS("justsomelyrics"))) {
        return false;
    }
    if (!ncm_lyrics_fetcher_registry_append_name(registry,
                                                 STRLIT_ARGS("jahlyrics"))) {
        return false;
    }
    if (!ncm_lyrics_fetcher_registry_append_name(registry,
                                                 STRLIT_ARGS("zeneszoveg"))) {
        return false;
    }
    if (!ncm_lyrics_fetcher_registry_append_name(registry,
                                                 STRLIT_ARGS("internet"))) {
        return false;
    }
    return true;
}

bool
ncm_lyrics_fetcher_build_url(NcmLyricsFetcherDef *fetcher, NcmBuffer *url,
                             char *artist, int32 artist_len,
                             char *title, int32 title_len) {
    if ((fetcher == NULL) || (url == NULL)) {
        return false;
    }
    ncm_buffer_clear(url);
    if (lyrics_type_is_google(fetcher->type)) {
        ncm_buffer_append(url,
                          STRLIT_ARGS("http://www.google.com/search?hl="
                                      "en&ie=UTF-8&oe=UTF-8&q="));
        if (fetcher->type == NCM_LYRICS_FETCHER_INTERNET) {
            ncm_buffer_append(url, STRLIT_ARGS("lyrics"));
        } else {
            ncm_buffer_append(url, STRLIT_ARGS("site:"));
            lyrics_append_escaped(url, fetcher->url_template,
                                  fetcher->url_template_len);
        }
        ncm_buffer_append_byte(url, '+');
        lyrics_append_escaped(url, artist, artist_len);
        ncm_buffer_append_byte(url, '+');
        lyrics_append_escaped(url, title, title_len);
        ncm_buffer_append(url, STRLIT_ARGS("&btnI=I%27m+Feeling+Lucky"));
    } else {
        lyrics_append_url_replaced(url, fetcher->url_template,
                                   fetcher->url_template_len, artist,
                                   artist_len, title, title_len);
    }
    return true;
}

bool
ncm_lyrics_fetcher_fetch(NcmLyricsFetcherDef *fetcher,
                         NcmLyricsResult *result, char *artist,
                         int32 artist_len, char *title, int32 title_len,
                         NcmSong *song) {
    NcmBuffer url;
    bool ok;

    if ((fetcher == NULL) || (result == NULL)) {
        return false;
    }

    ncm_lyrics_result_clear(result);
    if (fetcher->type == NCM_LYRICS_FETCHER_TAGS) {
        return lyrics_fetch_tags(result, song);
    }
    if (lyrics_type_is_google(fetcher->type)) {
        return lyrics_fetch_google(fetcher, result, artist, artist_len,
                                   title, title_len, song);
    }

    ncm_buffer_init(&url);
    ok = ncm_lyrics_fetcher_build_url(fetcher, &url, artist, artist_len,
                                      title, title_len);
    if (ok) {
        ok = lyrics_fetch_direct(fetcher, result, &url, NULL, 0);
    }
    ncm_buffer_destroy(&url);
    return ok;
}

void
ncm_lyrics_cleanup_html(NcmBuffer *out, char *data, int32 data_len) {
    NcmBuffer unescaped;
    NcmBuffer stripped;

    ncm_buffer_clear(out);
    unescaped = ncm_html_unescape_utf8(data, data_len);
    stripped = ncm_html_strip_tags(unescaped.data, unescaped.len);
    lyrics_append_clean_lines(out, stripped.data, stripped.len);
    lyrics_trim_buffer(out);
    ncm_buffer_destroy(&stripped);
    ncm_buffer_destroy(&unescaped);
    return;
}

void
ncm_lyrics_fetcher_set_io_for_tests(NcmLyricsCurlPerformFn perform,
                                    NcmLyricsCurlEscapeFn escape,
                                    void *user) {
    lyrics_test_perform = perform;
    lyrics_test_escape = escape;
    lyrics_test_user = user;
    return;
}

static void
lyrics_fetcher_array_init_item(void *item) {
    ncm_lyrics_fetcher_def_init(item);
    return;
}

static void
lyrics_fetcher_array_destroy_item(void *item) {
    ncm_lyrics_fetcher_def_destroy(item);
    return;
}

static bool
lyrics_fetcher_array_copy_item(void *dest, void *source) {
    return ncm_lyrics_fetcher_def_copy(dest, source);
}

static void
lyrics_fetcher_array_move_item(void *dest, void *source) {
    ncm_lyrics_fetcher_def_move(dest, source);
    return;
}

static bool
lyrics_name_to_type(char *name, int32 name_len,
                    enum NcmLyricsFetcherType *type) {
    if (ncm_string_equal(name, name_len, STRLIT_ARGS("justsomelyrics"))
        || ncm_string_equal(name, name_len,
                            STRLIT_ARGS("justsomelyrics.com"))) {
        *type = NCM_LYRICS_FETCHER_JUSTSOMELYRICS;
        return true;
    }
    if (ncm_string_equal(name, name_len, STRLIT_ARGS("jahlyrics"))
        || ncm_string_equal(name, name_len, STRLIT_ARGS("jah-lyrics.com"))) {
        *type = NCM_LYRICS_FETCHER_JAHLYRICS;
        return true;
    }
    if (ncm_string_equal(name, name_len, STRLIT_ARGS("plyrics"))
        || ncm_string_equal(name, name_len, STRLIT_ARGS("plyrics.com"))) {
        *type = NCM_LYRICS_FETCHER_PLYRICS;
        return true;
    }
    if (ncm_string_equal(name, name_len, STRLIT_ARGS("azlyrics"))
        || ncm_string_equal(name, name_len, STRLIT_ARGS("azlyrics.com"))) {
        *type = NCM_LYRICS_FETCHER_AZLYRICS;
        return true;
    }
    if (ncm_string_equal(name, name_len, STRLIT_ARGS("tekstowo"))
        || ncm_string_equal(name, name_len, STRLIT_ARGS("tekstowo.pl"))) {
        *type = NCM_LYRICS_FETCHER_TEKSTOWO;
        return true;
    }
    if (ncm_string_equal(name, name_len, STRLIT_ARGS("zeneszoveg"))
        || ncm_string_equal(name, name_len, STRLIT_ARGS("zeneszoveg.hu"))) {
        *type = NCM_LYRICS_FETCHER_ZENESZOVEG;
        return true;
    }
    if (ncm_string_equal(name, name_len, STRLIT_ARGS("internet"))
        || ncm_string_equal(name, name_len, STRLIT_ARGS("the Internet"))) {
        *type = NCM_LYRICS_FETCHER_INTERNET;
        return true;
    }
    if (ncm_string_equal(name, name_len, STRLIT_ARGS("tags"))) {
        *type = NCM_LYRICS_FETCHER_TAGS;
        return true;
    }
    return false;
}

static char *
lyrics_type_name(enum NcmLyricsFetcherType type, int32 *len) {
    switch (type) {
    case NCM_LYRICS_FETCHER_JUSTSOMELYRICS:
        *len = STRLIT_LEN("justsomelyrics.com");
        return (char *)"justsomelyrics.com";
    case NCM_LYRICS_FETCHER_JAHLYRICS:
        *len = STRLIT_LEN("jah-lyrics.com");
        return (char *)"jah-lyrics.com";
    case NCM_LYRICS_FETCHER_PLYRICS:
        *len = STRLIT_LEN("plyrics.com");
        return (char *)"plyrics.com";
    case NCM_LYRICS_FETCHER_AZLYRICS:
        *len = STRLIT_LEN("azlyrics.com");
        return (char *)"azlyrics.com";
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        *len = STRLIT_LEN("tekstowo.pl");
        return (char *)"tekstowo.pl";
    case NCM_LYRICS_FETCHER_ZENESZOVEG:
        *len = STRLIT_LEN("zeneszoveg.hu");
        return (char *)"zeneszoveg.hu";
    case NCM_LYRICS_FETCHER_INTERNET:
        *len = STRLIT_LEN("the Internet");
        return (char *)"the Internet";
    case NCM_LYRICS_FETCHER_TAGS:
        *len = STRLIT_LEN("tags");
        return (char *)"tags";
    default:
        *len = 0;
        return (char *)"";
    }
}

static char *
lyrics_type_site(enum NcmLyricsFetcherType type, int32 *len) {
    switch (type) {
    case NCM_LYRICS_FETCHER_JUSTSOMELYRICS:
        *len = STRLIT_LEN("justsomelyrics.com");
        return (char *)"justsomelyrics.com";
    case NCM_LYRICS_FETCHER_JAHLYRICS:
        *len = STRLIT_LEN("jah-lyrics.com");
        return (char *)"jah-lyrics.com";
    case NCM_LYRICS_FETCHER_PLYRICS:
        *len = STRLIT_LEN("plyrics.com");
        return (char *)"plyrics.com";
    case NCM_LYRICS_FETCHER_AZLYRICS:
        *len = STRLIT_LEN("azlyrics.com");
        return (char *)"azlyrics.com";
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        *len = STRLIT_LEN("tekstowo.pl");
        return (char *)"tekstowo.pl";
    case NCM_LYRICS_FETCHER_ZENESZOVEG:
        *len = STRLIT_LEN("zeneszoveg.hu");
        return (char *)"zeneszoveg.hu";
    default:
        *len = 0;
        return (char *)"";
    }
}

static char *
lyrics_type_regex(enum NcmLyricsFetcherType type, int32 *len) {
    switch (type) {
    case NCM_LYRICS_FETCHER_JUSTSOMELYRICS:
        *len = STRLIT_LEN("content");
        return (char *)"content";
    case NCM_LYRICS_FETCHER_JAHLYRICS:
        *len = STRLIT_LEN("song-header");
        return (char *)"song-header";
    case NCM_LYRICS_FETCHER_PLYRICS:
        *len = STRLIT_LEN("start of lyrics");
        return (char *)"start of lyrics";
    case NCM_LYRICS_FETCHER_AZLYRICS:
        *len = STRLIT_LEN("Usage of azlyrics.com");
        return (char *)"Usage of azlyrics.com";
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        *len = STRLIT_LEN("inner-text");
        return (char *)"inner-text";
    case NCM_LYRICS_FETCHER_ZENESZOVEG:
        *len = STRLIT_LEN("lyrics-plain-text trans_original");
        return (char *)"lyrics-plain-text trans_original";
    default:
        *len = 0;
        return (char *)"";
    }
}

static bool
lyrics_type_is_google(enum NcmLyricsFetcherType type) {
    return (type == NCM_LYRICS_FETCHER_JUSTSOMELYRICS)
           || (type == NCM_LYRICS_FETCHER_JAHLYRICS)
           || (type == NCM_LYRICS_FETCHER_PLYRICS)
           || (type == NCM_LYRICS_FETCHER_AZLYRICS)
           || (type == NCM_LYRICS_FETCHER_TEKSTOWO)
           || (type == NCM_LYRICS_FETCHER_ZENESZOVEG)
           || (type == NCM_LYRICS_FETCHER_INTERNET);
}

static bool
lyrics_type_is_available(enum NcmLyricsFetcherType type) {
    if (type == NCM_LYRICS_FETCHER_TAGS) {
#if defined(HAVE_TAGLIB_H)
        return true;
#else
        return false;
#endif
    }
    return type != NCM_LYRICS_FETCHER_UNKNOWN;
}

static CURLcode
lyrics_curl_perform(NcmBuffer *data, char *url, int32 url_len,
                    char *referer, int32 referer_len,
                    bool follow_redirect, int32 timeout_seconds) {
    if (lyrics_test_perform != NULL) {
        return lyrics_test_perform(data, url, url_len, referer, referer_len,
                                   follow_redirect, timeout_seconds,
                                   lyrics_test_user);
    }
    return ncm_curl_perform(data, url, url_len, referer, referer_len,
                            follow_redirect, timeout_seconds);
}

static CURLcode
lyrics_curl_escape(NcmBuffer *out, char *string, int32 string_len) {
    if (lyrics_test_escape != NULL) {
        return lyrics_test_escape(out, string, string_len, lyrics_test_user);
    }
    return ncm_curl_escape(out, string, string_len);
}

static void
lyrics_append_escaped(NcmBuffer *buffer, char *string, int32 string_len) {
    NcmBuffer escaped;

    ncm_buffer_init(&escaped);
    if (lyrics_curl_escape(&escaped, string, string_len) == CURLE_OK) {
        ncm_buffer_append(buffer, escaped.data, escaped.len);
    }
    ncm_buffer_destroy(&escaped);
    return;
}

static void
lyrics_append_url_replaced(NcmBuffer *buffer, char *template,
                           int32 template_len, char *artist,
                           int32 artist_len, char *title,
                           int32 title_len) {
    int32 i;

    i = 0;
    while (i < template_len) {
        if (ncm_string_starts_with(template + i, template_len - i,
                                   STRLIT_ARGS("%artist%"))) {
            lyrics_append_escaped(buffer, artist, artist_len);
            i += STRLIT_LEN("%artist%");
        } else if (ncm_string_starts_with(template + i, template_len - i,
                                          STRLIT_ARGS("%title%"))) {
            lyrics_append_escaped(buffer, title, title_len);
            i += STRLIT_LEN("%title%");
        } else {
            ncm_buffer_append_byte(buffer, template[i]);
            i += 1;
        }
    }
    return;
}

static int32
lyrics_find(char *data, int32 data_len, char *needle, int32 needle_len,
            int32 start) {
    if ((data == NULL) || (needle == NULL) || (needle_len <= 0)) {
        return -1;
    }
    if (start < 0) {
        start = 0;
    }
    for (int32 i = start; i + needle_len <= data_len; i += 1) {
        if (ncm_string_starts_with(data + i, data_len - i,
                                   needle, needle_len)) {
            return i;
        }
    }
    return -1;
}

static bool
lyrics_extract_between(NcmBuffer *out, char *data, int32 data_len,
                       char *start, int32 start_len, char *end,
                       int32 end_len) {
    int32 a;
    int32 b;

    ncm_buffer_clear(out);
    a = lyrics_find(data, data_len, start, start_len, 0);
    if (a < 0) {
        return false;
    }
    a += start_len;
    b = lyrics_find(data, data_len, end, end_len, a);
    if (b < 0) {
        return false;
    }
    ncm_buffer_append(out, data + a, b - a);
    return true;
}

static bool
lyrics_extract_after_until(NcmBuffer *out, char *data, int32 data_len,
                           char *start, int32 start_len, char *after,
                           int32 after_len, char *end, int32 end_len) {
    int32 a;
    int32 b;

    ncm_buffer_clear(out);
    a = lyrics_find(data, data_len, start, start_len, 0);
    if (a < 0) {
        return false;
    }
    a = lyrics_find(data, data_len, after, after_len, a + start_len);
    if (a < 0) {
        return false;
    }
    a += after_len;
    b = lyrics_find(data, data_len, end, end_len, a);
    if (b < 0) {
        return false;
    }
    ncm_buffer_append(out, data + a, b - a);
    return true;
}

static void
lyrics_extract_content(NcmLyricsFetcherDef *fetcher, NcmBuffer *out,
                       char *data, int32 data_len) {
    switch (fetcher->type) {
    case NCM_LYRICS_FETCHER_JUSTSOMELYRICS:
        lyrics_extract_after_until(out, data, data_len,
                                   STRLIT_ARGS("<div class=\"content"),
                                   STRLIT_ARGS("</div>"),
                                   STRLIT_ARGS("See also"));
        break;
    case NCM_LYRICS_FETCHER_JAHLYRICS:
        lyrics_extract_after_until(out, data, data_len,
                                   STRLIT_ARGS("<div class=\"song-header\">"),
                                   STRLIT_ARGS("</div>"),
                                   STRLIT_ARGS("<p class=\"disclaimer\">"));
        break;
    case NCM_LYRICS_FETCHER_PLYRICS:
        lyrics_extract_between(out, data, data_len,
                               STRLIT_ARGS("<!-- start of lyrics -->"),
                               STRLIT_ARGS("<!-- end of lyrics -->"));
        break;
    case NCM_LYRICS_FETCHER_AZLYRICS:
        lyrics_extract_after_until(out, data, data_len,
                                   STRLIT_ARGS("Usage of azlyrics.com"),
                                   STRLIT_ARGS("-->"),
                                   STRLIT_ARGS("</div>"));
        break;
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        lyrics_extract_between(out, data, data_len,
                               STRLIT_ARGS("<div class=\"inner-text\">"),
                               STRLIT_ARGS("</div>"));
        break;
    case NCM_LYRICS_FETCHER_ZENESZOVEG:
        lyrics_extract_between(
            out, data, data_len,
            STRLIT_ARGS("<div class=\"lyrics-plain-text trans_original\">"),
            STRLIT_ARGS("</div>"));
        break;
    default:
        ncm_buffer_clear(out);
        break;
    }
    return;
}

typedef struct LyricsRegexUrlState {
    NcmBuffer *out;
    char *data;
    int32 data_len;
    bool found;
} LyricsRegexUrlState;

static bool
lyrics_regex_url_match(int32 start, int32 len, void *user) {
    LyricsRegexUrlState *state;
    char *match;
    int32 match_len;
    int32 prefix_len;
    int32 suffix_len;

    state = user;
    if (state->found) {
        return false;
    }
    match = state->data + start;
    match_len = len;
    prefix_len = STRLIT_LEN("<A HREF=\"http://www.google.com/url?q=");
    suffix_len = STRLIT_LEN("\">here</A>");
    if (match_len > prefix_len + suffix_len) {
        ncm_buffer_append(state->out, match + prefix_len,
                          match_len - prefix_len - suffix_len);
        state->found = true;
    }
    return false;
}

static bool
lyrics_extract_google_url(NcmBuffer *out, char *data, int32 data_len) {
    NcmRegex regex;
    NcmError error;
    LyricsRegexUrlState state;
    bool matched;

    ncm_buffer_clear(out);
    ncm_error_clear(&error);
    ncm_regex_init(&regex);
    if (!ncm_regex_compile(
            &regex,
            STRLIT_ARGS("<A HREF=\"http://www\\.google\\.com/url\\?q=[^\"]*\">here</A>"),
            NCM_REGEX_EXTENDED, &error)) {
            ncm_regex_destroy(&regex);
        return false;
    }

    state.out = out;
    state.data = data;
    state.data_len = data_len;
    state.found = false;
    matched = ncm_regex_for_each_match(&regex, data, data_len,
                                       lyrics_regex_url_match, &state);
    ncm_regex_destroy(&regex);
    return matched && state.found;
}

static bool
lyrics_url_ok(NcmLyricsFetcherDef *fetcher, char *url, int32 url_len) {
    if (fetcher->type == NCM_LYRICS_FETCHER_INTERNET) {
        return false;
    }
    return lyrics_find(url, url_len, fetcher->url_template,
                       fetcher->url_template_len, 0) >= 0;
}

static bool
lyrics_fetch_direct(NcmLyricsFetcherDef *fetcher, NcmLyricsResult *result,
                    NcmBuffer *url, char *referer, int32 referer_len) {
    CURLcode code;
    NcmBuffer data;
    NcmBuffer lyrics;
    NcmBuffer cleaned;
    bool ok;

    ncm_buffer_init(&data);
    ncm_buffer_init(&lyrics);
    ncm_buffer_init(&cleaned);
    code = lyrics_curl_perform(&data, url->data, url->len, referer,
                               referer_len, true, 10);
    if (code != CURLE_OK) {
        char *message = (char *)curl_easy_strerror(code);
        ncm_lyrics_result_set(result, false, message, (int32)strlen(message));
        ok = true;
        goto cleanup;
    }

    lyrics_extract_content(fetcher, &lyrics, data.data, data.len);
    if (lyrics.len <= 0) {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS(LYRICS_MSG_NOT_FOUND));
        ok = true;
        goto cleanup;
    }

    ncm_lyrics_cleanup_html(&cleaned, lyrics.data, lyrics.len);
    if (cleaned.len <= 0) {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS(LYRICS_MSG_NOT_FOUND));
        ok = true;
        goto cleanup;
    }
    ncm_lyrics_result_set(result, true, cleaned.data, cleaned.len);
    ok = true;

cleanup:
    ncm_buffer_destroy(&cleaned);
    ncm_buffer_destroy(&lyrics);
    ncm_buffer_destroy(&data);
    return ok;
}

static bool
lyrics_fetch_google(NcmLyricsFetcherDef *fetcher, NcmLyricsResult *result,
                    char *artist, int32 artist_len, char *title,
                    int32 title_len, NcmSong *song) {
    NcmBuffer google_url;
    NcmBuffer data;
    NcmBuffer url;
    NcmBuffer unescaped;
    CURLcode code;
    bool ok;

    (void)song;
    ncm_buffer_init(&google_url);
    ncm_buffer_init(&data);
    ncm_buffer_init(&url);
    ncm_buffer_init(&unescaped);
    ok = ncm_lyrics_fetcher_build_url(fetcher, &google_url, artist,
                                      artist_len, title, title_len);
    if (!ok) {
        goto cleanup;
    }

    code = lyrics_curl_perform(&data, google_url.data, google_url.len,
                               google_url.data, google_url.len, false, 10);
    if (code != CURLE_OK) {
        char *message = (char *)curl_easy_strerror(code);
        ncm_lyrics_result_set(result, false, message, (int32)strlen(message));
        ok = true;
        goto cleanup;
    }
    if (!lyrics_extract_google_url(&url, data.data, data.len)) {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS(LYRICS_MSG_NOT_FOUND));
        ok = true;
        goto cleanup;
    }

    unescaped = ncm_html_unescape_utf8(url.data, url.len);
    if (!lyrics_url_ok(fetcher, unescaped.data, unescaped.len)) {
        if (fetcher->type == NCM_LYRICS_FETCHER_INTERNET) {
            NcmBuffer message;

            ncm_buffer_init(&message);
            ncm_buffer_append(&message,
                              STRLIT_ARGS("The following site may contain "
                                          "lyrics for this song: "));
            ncm_buffer_append(&message, unescaped.data, unescaped.len);
            ncm_lyrics_result_set(result, false, message.data, message.len);
            ncm_buffer_destroy(&message);
        } else {
            ncm_lyrics_result_set(result, false,
                                  STRLIT_ARGS(LYRICS_MSG_NOT_FOUND));
        }
        ok = true;
        goto cleanup;
    }

    ok = lyrics_fetch_direct(fetcher, result, &unescaped,
                             google_url.data, google_url.len);

cleanup:
    ncm_buffer_destroy(&unescaped);
    ncm_buffer_destroy(&url);
    ncm_buffer_destroy(&data);
    ncm_buffer_destroy(&google_url);
    return ok;
}

#if defined(HAVE_TAGLIB_H)
typedef struct TagsLyricsState {
    NcmBuffer *buffer;
} TagsLyricsState;

static void
tags_append_lyrics(char *value, int32 value_len, void *user) {
    TagsLyricsState *state;

    state = user;
    if (state->buffer->len > 0) {
        ncm_buffer_append(state->buffer, STRLIT_ARGS("\n\n"));
    }
    ncm_buffer_append(state->buffer, value, value_len);
    return;
}
#endif

static bool
lyrics_fetch_tags(NcmLyricsResult *result, NcmSong *song) {
#if defined(HAVE_TAGLIB_H)
    NcmBuffer lyrics;
    NcmStringView uri;
    TagsLyricsState state;
    enum NcmTagsReadResult read_result;

    if ((song == NULL) || !ncm_song_uri_view(song, 0, &uri)) {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS("No lyrics in tags"));
        return true;
    }

    ncm_buffer_init(&lyrics);
    state.buffer = &lyrics;
    read_result = ncm_tags_read_lyrics(uri.data, tags_append_lyrics, &state);
    if (read_result == NCM_TAGS_READ_OPEN_FAILED) {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS("Could not open file"));
    } else if (read_result == NCM_TAGS_READ_OK) {
        ncm_lyrics_result_set(result, true, lyrics.data, lyrics.len);
    } else {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS("No lyrics in tags"));
    }
    ncm_buffer_destroy(&lyrics);
    return true;
#else
    (void)song;
    ncm_lyrics_result_set(result, false, STRLIT_ARGS("No lyrics in tags"));
    return true;
#endif
}

static void
lyrics_trim_view(char **data, int32 *len) {
    char *text;
    int32 text_len;

    text = *data;
    text_len = *len;
    while ((text_len > 0)
           && ((text[0] == ' ') || (text[0] == '\t')
               || (text[0] == '\r') || (text[0] == '\n'))) {
        text += 1;
        text_len -= 1;
    }
    while ((text_len > 0)
           && ((text[text_len - 1] == ' ')
               || (text[text_len - 1] == '\t')
               || (text[text_len - 1] == '\r')
               || (text[text_len - 1] == '\n'))) {
        text_len -= 1;
    }
    *data = text;
    *len = text_len;
    return;
}

static void
lyrics_trim_buffer(NcmBuffer *buffer) {
    char *text;
    int32 text_len;
    NcmBuffer tmp;

    text = buffer->data;
    text_len = buffer->len;
    lyrics_trim_view(&text, &text_len);
    ncm_buffer_init(&tmp);
    ncm_buffer_append(&tmp, text, text_len);
    ncm_buffer_clear(buffer);
    ncm_buffer_append(buffer, tmp.data, tmp.len);
    ncm_buffer_destroy(&tmp);
    return;
}

static void
lyrics_append_clean_lines(NcmBuffer *out, char *data, int32 data_len) {
    int32 line_start;
    bool previous_empty;

    line_start = 0;
    previous_empty = true;
    for (int32 i = 0; i <= data_len; i += 1) {
        if ((i == data_len) || (data[i] == '\n') || (data[i] == '\r')) {
            char *line;
            int32 line_len;

            line = data + line_start;
            line_len = i - line_start;
            lyrics_trim_view(&line, &line_len);
            if (line_len > 0) {
                if (out->len > 0) {
                    ncm_buffer_append_byte(out, '\n');
                }
                ncm_buffer_append(out, line, line_len);
                previous_empty = false;
            } else if (!previous_empty) {
                ncm_buffer_append_byte(out, '\n');
                previous_empty = true;
            }
            line_start = i + 1;
        }
    }
    lyrics_trim_buffer(out);
    return;
}
