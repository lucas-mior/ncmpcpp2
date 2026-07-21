#if !defined(NCMPCPP_LYRICS_FETCHER_C)
#define NCMPCPP_LYRICS_FETCHER_C

#include "lyrics_fetcher.h"

#include "c/ncm_base.h"
#include "c/ncm_html.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/utf8.c"
#include "cbase/util.c"
#include "curl_handle.h"

#define LYRICS_MSG_ACCESS_DENIED "Access denied"
#define LYRICS_MSG_DOWNLOAD_FAILED "Download failed"
#define LYRICS_MSG_INVALID_FETCHER "Invalid lyrics fetcher"
#define LYRICS_MSG_NOT_FOUND "Not found"
#define LYRICS_MSG_TIMED_OUT "Request timed out"

#define LYRICS_SEARCH_MAX_CANDIDATES 8

static NcmLyricsCurlPerformFn lyrics_test_perform;
static void *lyrics_test_user;

static NcmArrayItemCallbacks lyrics_fetcher_callbacks;

typedef enum LyricsSlugProfile {
    LYRICS_SLUG_PROFILE_NONE,
    LYRICS_SLUG_PROFILE_COMPACT_FOLDED,
    LYRICS_SLUG_PROFILE_HYPHEN_FOLDED,
    LYRICS_SLUG_PROFILE_COMPACT_PERCENT,
    LYRICS_SLUG_PROFILE_HYPHEN_PERCENT,
} LyricsSlugProfile;

typedef struct LyricsDirectSlugPair {
    LyricsSlugProfile artist;
    LyricsSlugProfile title;
} LyricsDirectSlugPair;

static void lyrics_string_destroy(char **data, int32 *len, int32 *cap);
static void lyrics_fetcher_array_init_item(void *item);
static void lyrics_fetcher_array_destroy_item(void *item);
static bool lyrics_name_to_type(char *name, int32 name_len,
                                enum NcmLyricsFetcherType *type);
static char *lyrics_type_name(enum NcmLyricsFetcherType type, int32 *len);
static char *lyrics_type_domain(enum NcmLyricsFetcherType type, int32 *len);
static LyricsSlugProfile lyrics_slug_profile(enum NcmLyricsFetcherType type);
static LyricsSlugProfile lyrics_legacy_slug_profile(LyricsSlugProfile profile);
static bool lyrics_slug_profile_compact(LyricsSlugProfile profile);
static bool lyrics_slug_profile_folded(LyricsSlugProfile profile);
static bool lyrics_fold_latin_rune(uint32 rune, char *folded,
                                   int32 *folded_len);
static bool lyrics_skip_slug_rune(uint32 rune);
static bool lyrics_slug_separator_rune(uint32 rune);
static bool lyrics_append_slug_profile(StrBuilder *buffer,
                                       LyricsSlugProfile profile, char *string,
                                       int32 string_len);
static bool lyrics_append_slug(StrBuilder *buffer,
                               enum NcmLyricsFetcherType type, char *string,
                               int32 string_len);
static void lyrics_append_query(StrBuilder *buffer,
                                char *string, int32 string_len);
static int32 lyrics_hex_value(char ch);
static bool lyrics_url_collected(StrBuilderArray *urls, char *url,
                                 int32 url_len);
static bool lyrics_append_url_if_new(StrBuilderArray *urls, char *url,
                                     int32 url_len);
static bool lyrics_build_direct_url_profiles(
    NcmLyricsFetcherDef *fetcher, StrBuilder *url, char *artist,
    int32 artist_len, char *title, int32 title_len,
    LyricsSlugProfile artist_profile, LyricsSlugProfile title_profile
);
static bool lyrics_collect_direct_urls(NcmLyricsFetcherDef *fetcher,
                                       StrBuilderArray *urls, char *artist,
                                       int32 artist_len, char *title,
                                       int32 title_len);
static bool lyrics_fetch_page(NcmLyricsFetcherDef *fetcher,
                              NcmLyricsResult *result, StrBuilder *url,
                              char *referer, int32 referer_len,
                              bool *retry);
static bool lyrics_fetch_search(NcmLyricsFetcherDef *fetcher,
                                NcmLyricsResult *result, char *artist,
                                int32 artist_len, char *title,
                                int32 title_len);
static bool lyrics_collect_search_urls(NcmLyricsFetcherDef *fetcher,
                                       StrBuilderArray *out, char *data,
                                       int32 data_len, char *artist,
                                       int32 artist_len, char *title,
                                       int32 title_len);
static bool lyrics_fetch_internet(NcmLyricsFetcherDef *fetcher,
                                  NcmLyricsResult *result, char *artist,
                                  int32 artist_len, char *title,
                                  int32 title_len);
static void lyrics_trim_buffer(StrBuilder *buffer);
static void lyrics_append_clean_lines(StrBuilder *out, char *data,
                                      int32 data_len);

static NcmArrayItemCallbacks lyrics_fetcher_callbacks = {
    .init = lyrics_fetcher_array_init_item,
    .destroy = lyrics_fetcher_array_destroy_item,
};

NCM_ARRAY_DEFINE_INIT(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DEFINE_CLEAR(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray,
                       &lyrics_fetcher_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DEFINE_APPEND(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray,
                        NcmLyricsFetcherDef, &lyrics_fetcher_callbacks)

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
    new_data = malloc2(new_cap);
    memcpy64(new_data, source, source_len);
    new_data[source_len] = '\0';

    *data = new_data;
    *len = source_len;
    *cap = new_cap;

    return true;
}

static void
lyrics_string_destroy(char **data, int32 *len, int32 *cap) {
    if (*data) {
        free2(*data, *cap);
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
    lyrics_string_destroy(&result->text, &result->text_len, &result->text_cap);
    result->success = false;
    return;
}

void
ncm_lyrics_result_clear(NcmLyricsResult *result) {
    if (result == NULL) {
        return;
    }
    lyrics_string_destroy(&result->text, &result->text_len, &result->text_cap);
    result->success = false;
    return;
}

bool
ncm_lyrics_result_set(NcmLyricsResult *result, bool success,
                      char *text, int32 text_len) {
    if (result == NULL) {
        return false;
    }
    if (!lyrics_string_set(&result->text, &result->text_len, &result->text_cap,
                           text, text_len)) {
        return false;
    }
    result->success = success;
    return true;
}

void
ncm_lyrics_fetcher_def_init(NcmLyricsFetcherDef *fetcher) {
    fetcher->name = NULL;
    fetcher->name_len = 0;
    fetcher->name_cap = 0;
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
    fetcher->type = NCM_LYRICS_FETCHER_UNKNOWN;
    fetcher->enabled = false;
    return;
}

bool
ncm_lyrics_fetcher_def_set_name(NcmLyricsFetcherDef *fetcher,
                                char *name, int32 name_len) {
    enum NcmLyricsFetcherType type;
    char *display_name;
    int32 display_name_len;

    if ((fetcher == NULL) || (name == NULL) || (name_len <= 0)) {
        return false;
    }
    if (!lyrics_name_to_type(name, name_len, &type)) {
        return false;
    }

    display_name = lyrics_type_name(type, &display_name_len);
    ncm_lyrics_fetcher_def_destroy(fetcher);
    ncm_lyrics_fetcher_def_init(fetcher);
    fetcher->type = type;
    fetcher->enabled = true;
    return lyrics_string_set(&fetcher->name, &fetcher->name_len,
                             &fetcher->name_cap, display_name,
                             display_name_len);
}

char *
ncm_lyrics_fetcher_name(NcmLyricsFetcherDef *fetcher) {
    if ((fetcher == NULL) || (fetcher->name == NULL)) {
        return "";
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

    if ((fetcher = ncm_lyrics_fetcher_registry_append(registry)) == NULL) {
        return false;
    }
    if (!ncm_lyrics_fetcher_def_set_name(fetcher, name, name_len)) {
        registry->fetchers.len -= 1;
        ncm_lyrics_fetcher_def_destroy(fetcher);
        return false;
    }
    return true;
}

static bool
lyrics_url_collected(StrBuilderArray *urls, char *url, int32 url_len) {
    if ((urls == NULL) || (url == NULL) || (url_len <= 0)) {
        return false;
    }

    for (int32 i = 0; i < urls->len; i += 1) {
        if (STREQUAL(urls->items[i].data, urls->items[i].len,
                     url, url_len)) {
            return true;
        }
    }
    return false;
}

static bool
lyrics_append_url_if_new(StrBuilderArray *urls, char *url, int32 url_len) {
    StrBuilder *item;

    if ((urls == NULL) || (url == NULL) || (url_len <= 0)) {
        return false;
    }
    if (lyrics_url_collected(urls, url, url_len)) {
        return true;
    }

    item = str_builder_array_append(urls);
    if (item == NULL) {
        return false;
    }
    sb_append(item, url, url_len);
    return true;
}

static bool
lyrics_build_direct_url_profiles(
    NcmLyricsFetcherDef *fetcher, StrBuilder *url, char *artist,
    int32 artist_len, char *title, int32 title_len,
    LyricsSlugProfile artist_profile, LyricsSlugProfile title_profile
) {
    bool valid;

    if ((fetcher == NULL) || (url == NULL) || !fetcher->enabled
        || (artist == NULL) || (artist_len <= 0) || (title == NULL)
        || (title_len <= 0)) {
        return false;
    }

    sb_clear(url);
    switch (fetcher->type) {
    case NCM_LYRICS_FETCHER_AZLYRICS:
        sb_append(url, STRLIT_ARGS("https://www.azlyrics.com/lyrics/"));
        valid = lyrics_append_slug_profile(url, artist_profile,
                                           artist, artist_len);
        sb_append_byte(url, '/');
        valid = valid
                && lyrics_append_slug_profile(url, title_profile,
                                              title, title_len);
        sb_append(url, STRLIT_ARGS(".html"));
        break;
    case NCM_LYRICS_FETCHER_GENIUS:
        sb_append(url, STRLIT_ARGS("https://genius.com/"));
        valid = lyrics_append_slug_profile(url, artist_profile,
                                           artist, artist_len);
        sb_append_byte(url, '-');
        valid = valid
                && lyrics_append_slug_profile(url, title_profile,
                                              title, title_len);
        sb_append(url, STRLIT_ARGS("-lyrics"));
        break;
    case NCM_LYRICS_FETCHER_LETRASMUS:
        sb_append(url, STRLIT_ARGS("https://www.letras.mus.br/"));
        valid = lyrics_append_slug_profile(url, artist_profile,
                                           artist, artist_len);
        sb_append_byte(url, '/');
        valid = valid
                && lyrics_append_slug_profile(url, title_profile,
                                              title, title_len);
        sb_append_byte(url, '/');
        break;
    case NCM_LYRICS_FETCHER_MUSICA:
        valid = false;
        break;
    case NCM_LYRICS_FETCHER_PAROLES:
        sb_append(url, STRLIT_ARGS("https://www.paroles.net/"));
        valid = lyrics_append_slug_profile(url, artist_profile,
                                           artist, artist_len);
        sb_append(url, STRLIT_ARGS("/paroles-"));
        valid = valid
                && lyrics_append_slug_profile(url, title_profile,
                                              title, title_len);
        break;
    case NCM_LYRICS_FETCHER_MUSIXMATCH:
        sb_append(url, STRLIT_ARGS("https://www.musixmatch.com/lyrics/"));
        valid = lyrics_append_slug_profile(url, artist_profile,
                                           artist, artist_len);
        sb_append_byte(url, '/');
        valid = valid
                && lyrics_append_slug_profile(url, title_profile,
                                              title, title_len);
        break;
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        sb_append(url, STRLIT_ARGS("https://www.tekstowo.pl/"));
        valid = lyrics_append_slug_profile(url, artist_profile,
                                           artist, artist_len);
        sb_append_byte(url, '/');
        valid = valid
                && lyrics_append_slug_profile(url, title_profile,
                                              title, title_len);
        break;
    case NCM_LYRICS_FETCHER_VAGALUME:
        sb_append(url, STRLIT_ARGS("https://www.vagalume.com.br/"));
        valid = lyrics_append_slug_profile(url, artist_profile,
                                           artist, artist_len);
        sb_append_byte(url, '/');
        valid = valid
                && lyrics_append_slug_profile(url, title_profile,
                                              title, title_len);
        sb_append(url, STRLIT_ARGS(".html"));
        break;
    case NCM_LYRICS_FETCHER_UNKNOWN:
    case NCM_LYRICS_FETCHER_INTERNET:
    case NCM_LYRICS_FETCHER_LAST:
    default:
        valid = false;
        break;
    }

    if (!valid) {
        sb_clear(url);
    }
    return valid;
}

static bool
lyrics_collect_direct_urls(NcmLyricsFetcherDef *fetcher,
                           StrBuilderArray *urls, char *artist,
                           int32 artist_len, char *title, int32 title_len) {
    LyricsSlugProfile profile;
    LyricsSlugProfile legacy_profile;
    LyricsDirectSlugPair pairs[4];
    StrBuilder candidate;
    bool ok;

    if ((fetcher == NULL) || (urls == NULL) || !fetcher->enabled
        || (artist == NULL) || (artist_len <= 0) || (title == NULL)
        || (title_len <= 0)) {
        return false;
    }

    profile = lyrics_slug_profile(fetcher->type);
    if (profile == LYRICS_SLUG_PROFILE_NONE) {
        str_builder_array_clear(urls);
        return fetcher->type == NCM_LYRICS_FETCHER_MUSICA;
    }
    legacy_profile = lyrics_legacy_slug_profile(profile);
    pairs[0] = (LyricsDirectSlugPair){profile, profile};
    pairs[1] = (LyricsDirectSlugPair){legacy_profile, legacy_profile};
    pairs[2] = (LyricsDirectSlugPair){profile, legacy_profile};
    pairs[3] = (LyricsDirectSlugPair){legacy_profile, profile};

    str_builder_array_clear(urls);
    sb_init(&candidate);
    ok = true;
    for (int32 i = 0; i < LENGTH(pairs); i += 1) {
        if (!lyrics_build_direct_url_profiles(
                fetcher, &candidate, artist, artist_len, title, title_len,
                pairs[i].artist, pairs[i].title)) {
            ok = false;
            break;
        }
        if (!lyrics_append_url_if_new(urls, candidate.data, candidate.len)) {
            ok = false;
            break;
        }
    }
    sb_free(&candidate);
    return ok && (urls->len > 0);
}

bool
ncm_lyrics_fetcher_build_url(NcmLyricsFetcherDef *fetcher, StrBuilder *url,
                             char *artist, int32 artist_len, char *title,
                             int32 title_len) {
    char *domain;
    int32 domain_len;

    if ((fetcher == NULL) || (url == NULL) || !fetcher->enabled
        || (artist == NULL) || (artist_len <= 0) || (title == NULL)
        || (title_len <= 0)) {
        return false;
    }

    sb_clear(url);
    sb_append(url,
                      STRLIT_ARGS("https://www.google.com/search?hl=en&q="));
    if (fetcher->type == NCM_LYRICS_FETCHER_INTERNET) {
        sb_append(url, STRLIT_ARGS("lyrics+"));
    } else {
        domain = lyrics_type_domain(fetcher->type, &domain_len);
        if (domain_len <= 0) {
            sb_clear(url);
            return false;
        }
        lyrics_append_query(url, STRLIT_ARGS("site:"));
        lyrics_append_query(url, domain, domain_len);
        sb_append_byte(url, '+');
    }
    lyrics_append_query(url, artist, artist_len);
    sb_append_byte(url, '+');
    lyrics_append_query(url, title, title_len);
    if (fetcher->type != NCM_LYRICS_FETCHER_INTERNET) {
        sb_append(url, STRLIT_ARGS("+lyrics"));
    }
    return true;
}

bool
ncm_lyrics_fetcher_fetch(NcmLyricsFetcherDef *fetcher, NcmLyricsResult *result,
                         char *artist, int32 artist_len, char *title,
                         int32 title_len) {
    StrBuilderArray direct_urls;
    bool retry = false;
    bool ok;

    if ((fetcher == NULL) || (result == NULL)) {
        return false;
    }

    ncm_lyrics_result_clear(result);
    if (!fetcher->enabled
        || (fetcher->type <= NCM_LYRICS_FETCHER_UNKNOWN)
        || (fetcher->type >= NCM_LYRICS_FETCHER_LAST)) {
        ncm_lyrics_result_set(result, false,
                              STRLIT_ARGS(LYRICS_MSG_INVALID_FETCHER));
        return false;
    }
    if (fetcher->type == NCM_LYRICS_FETCHER_INTERNET) {
        return lyrics_fetch_internet(fetcher, result, artist, artist_len,
                                     title, title_len);
    }

    str_builder_array_init(&direct_urls);
    ok = lyrics_collect_direct_urls(fetcher, &direct_urls, artist, artist_len,
                                    title, title_len);
    if (ok) {
        if (direct_urls.len == 0) {
            retry = true;
        }
        for (int32 i = 0; i < direct_urls.len; i += 1) {
            ok = lyrics_fetch_page(fetcher, result, &direct_urls.items[i],
                                   NULL, 0, &retry);
            if (!ok || result->success || !retry) {
                break;
            }
        }
    }
    str_builder_array_destroy(&direct_urls);
    if (ok && retry) {
        ok = lyrics_fetch_search(fetcher, result, artist, artist_len, title,
                                 title_len);
    }
    return ok;
}

void
ncm_lyrics_cleanup_html(StrBuilder *out, char *data, int32 data_len) {
    StrBuilder unescaped;
    StrBuilder stripped;

    sb_clear(out);
    unescaped = ncm_html_unescape_utf8(data, data_len);
    stripped = ncm_html_strip_tags(unescaped.data, unescaped.len);
    lyrics_append_clean_lines(out, stripped.data, stripped.len);
    sb_free(&stripped);
    sb_free(&unescaped);
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
lyrics_name_to_type(char *name, int32 name_len,
                    enum NcmLyricsFetcherType *type) {
    if (STREQUAL(name, name_len, STRLIT_ARGS("azlyrics"))
        || STREQUAL(name, name_len, STRLIT_ARGS("azlyrics.com"))) {
        *type = NCM_LYRICS_FETCHER_AZLYRICS;
        return true;
    }
    if (STREQUAL(name, name_len, STRLIT_ARGS("genius"))
        || STREQUAL(name, name_len, STRLIT_ARGS("genius.com"))) {
        *type = NCM_LYRICS_FETCHER_GENIUS;
        return true;
    }
    if (STREQUAL(name, name_len, STRLIT_ARGS("letras"))
        || STREQUAL(name, name_len, STRLIT_ARGS("letrasmus"))
        || STREQUAL(name, name_len, STRLIT_ARGS("letras.mus.br"))) {
        *type = NCM_LYRICS_FETCHER_LETRASMUS;
        return true;
    }
    if (STREQUAL(name, name_len, STRLIT_ARGS("musica"))
        || STREQUAL(name, name_len, STRLIT_ARGS("musica.com"))) {
        *type = NCM_LYRICS_FETCHER_MUSICA;
        return true;
    }
    if (STREQUAL(name, name_len, STRLIT_ARGS("paroles"))
        || STREQUAL(name, name_len, STRLIT_ARGS("paroles.net"))) {
        *type = NCM_LYRICS_FETCHER_PAROLES;
        return true;
    }
    if (STREQUAL(name, name_len, STRLIT_ARGS("musixmatch"))
        || STREQUAL(name, name_len, STRLIT_ARGS("musixmatch.com"))) {
        *type = NCM_LYRICS_FETCHER_MUSIXMATCH;
        return true;
    }
    if (STREQUAL(name, name_len, STRLIT_ARGS("tekstowo"))
        || STREQUAL(name, name_len, STRLIT_ARGS("tekstowo.pl"))) {
        *type = NCM_LYRICS_FETCHER_TEKSTOWO;
        return true;
    }
    if (STREQUAL(name, name_len, STRLIT_ARGS("vagalume"))
        || STREQUAL(name, name_len, STRLIT_ARGS("vagalume.com.br"))) {
        *type = NCM_LYRICS_FETCHER_VAGALUME;
        return true;
    }
    if (STREQUAL(name, name_len, STRLIT_ARGS("internet"))
        || STREQUAL(name, name_len, STRLIT_ARGS("the Internet"))) {
        *type = NCM_LYRICS_FETCHER_INTERNET;
        return true;
    }
    return false;
}

static char *
lyrics_type_name(enum NcmLyricsFetcherType type, int32 *len) {
    switch (type) {
    case NCM_LYRICS_FETCHER_AZLYRICS:
        *len = STRLIT_LEN("azlyrics.com");
        return "azlyrics.com";
    case NCM_LYRICS_FETCHER_GENIUS:
        *len = STRLIT_LEN("genius.com");
        return "genius.com";
    case NCM_LYRICS_FETCHER_LETRASMUS:
        *len = STRLIT_LEN("letras.mus.br");
        return "letras.mus.br";
    case NCM_LYRICS_FETCHER_MUSICA:
        *len = STRLIT_LEN("musica.com");
        return "musica.com";
    case NCM_LYRICS_FETCHER_PAROLES:
        *len = STRLIT_LEN("paroles.net");
        return "paroles.net";
    case NCM_LYRICS_FETCHER_MUSIXMATCH:
        *len = STRLIT_LEN("musixmatch.com");
        return "musixmatch.com";
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        *len = STRLIT_LEN("tekstowo.pl");
        return "tekstowo.pl";
    case NCM_LYRICS_FETCHER_VAGALUME:
        *len = STRLIT_LEN("vagalume.com.br");
        return "vagalume.com.br";
    case NCM_LYRICS_FETCHER_INTERNET:
        *len = STRLIT_LEN("the Internet");
        return "the Internet";
    case NCM_LYRICS_FETCHER_UNKNOWN:
    case NCM_LYRICS_FETCHER_LAST:
    default:
        *len = 0;
        return "";
    }
}

static char *
lyrics_type_domain(enum NcmLyricsFetcherType type, int32 *len) {
    switch (type) {
    case NCM_LYRICS_FETCHER_AZLYRICS:
        *len = STRLIT_LEN("azlyrics.com");
        return "azlyrics.com";
    case NCM_LYRICS_FETCHER_GENIUS:
        *len = STRLIT_LEN("genius.com");
        return "genius.com";
    case NCM_LYRICS_FETCHER_LETRASMUS:
        *len = STRLIT_LEN("letras.mus.br");
        return "letras.mus.br";
    case NCM_LYRICS_FETCHER_MUSICA:
        *len = STRLIT_LEN("musica.com");
        return "musica.com";
    case NCM_LYRICS_FETCHER_PAROLES:
        *len = STRLIT_LEN("paroles.net");
        return "paroles.net";
    case NCM_LYRICS_FETCHER_MUSIXMATCH:
        *len = STRLIT_LEN("musixmatch.com");
        return "musixmatch.com";
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        *len = STRLIT_LEN("tekstowo.pl");
        return "tekstowo.pl";
    case NCM_LYRICS_FETCHER_VAGALUME:
        *len = STRLIT_LEN("vagalume.com.br");
        return "vagalume.com.br";
    case NCM_LYRICS_FETCHER_UNKNOWN:
    case NCM_LYRICS_FETCHER_INTERNET:
    case NCM_LYRICS_FETCHER_LAST:
    default:
        *len = 0;
        return "";
    }
}

static bool
lyrics_ascii_alnum(char ch) {
    return ((ch >= 'a') && (ch <= 'z'))
           || ((ch >= 'A') && (ch <= 'Z'))
           || ((ch >= '0') && (ch <= '9'));
}

static char
lyrics_ascii_lower(char ch) {
    if ((ch >= 'A') && (ch <= 'Z')) {
        return (char)(ch - 'A' + 'a');
    }
    return ch;
}

static char
lyrics_hex_digit(uint8 value) {
    if (value < 10) {
        return (char)('0' + value);
    }
    return (char)('A' + value - 10);
}

static void
lyrics_append_percent_byte(StrBuilder *buffer, uint8 value) {
    sb_append_byte(buffer, '%');
    sb_append_byte(buffer, lyrics_hex_digit(value >> 4));
    sb_append_byte(buffer, lyrics_hex_digit(value & 0x0f));
    return;
}

static LyricsSlugProfile
lyrics_slug_profile(enum NcmLyricsFetcherType type) {
    switch (type) {
    case NCM_LYRICS_FETCHER_AZLYRICS:
        return LYRICS_SLUG_PROFILE_COMPACT_FOLDED;
    case NCM_LYRICS_FETCHER_GENIUS:
    case NCM_LYRICS_FETCHER_LETRASMUS:
    case NCM_LYRICS_FETCHER_PAROLES:
    case NCM_LYRICS_FETCHER_MUSIXMATCH:
    case NCM_LYRICS_FETCHER_TEKSTOWO:
    case NCM_LYRICS_FETCHER_VAGALUME:
        return LYRICS_SLUG_PROFILE_HYPHEN_FOLDED;
    case NCM_LYRICS_FETCHER_MUSICA:
    case NCM_LYRICS_FETCHER_UNKNOWN:
    case NCM_LYRICS_FETCHER_INTERNET:
    case NCM_LYRICS_FETCHER_LAST:
    default:
        return LYRICS_SLUG_PROFILE_NONE;
    }
}

static LyricsSlugProfile
lyrics_legacy_slug_profile(LyricsSlugProfile profile) {
    switch (profile) {
    case LYRICS_SLUG_PROFILE_COMPACT_FOLDED:
        return LYRICS_SLUG_PROFILE_COMPACT_PERCENT;
    case LYRICS_SLUG_PROFILE_HYPHEN_FOLDED:
        return LYRICS_SLUG_PROFILE_HYPHEN_PERCENT;
    case LYRICS_SLUG_PROFILE_COMPACT_PERCENT:
    case LYRICS_SLUG_PROFILE_HYPHEN_PERCENT:
    case LYRICS_SLUG_PROFILE_NONE:
    default:
        return profile;
    }
}

static bool
lyrics_slug_profile_compact(LyricsSlugProfile profile) {
    switch (profile) {
    case LYRICS_SLUG_PROFILE_COMPACT_FOLDED:
    case LYRICS_SLUG_PROFILE_COMPACT_PERCENT:
        return true;
    case LYRICS_SLUG_PROFILE_HYPHEN_FOLDED:
    case LYRICS_SLUG_PROFILE_HYPHEN_PERCENT:
    case LYRICS_SLUG_PROFILE_NONE:
    default:
        return false;
    }
}

static bool
lyrics_slug_profile_folded(LyricsSlugProfile profile) {
    switch (profile) {
    case LYRICS_SLUG_PROFILE_COMPACT_FOLDED:
    case LYRICS_SLUG_PROFILE_HYPHEN_FOLDED:
        return true;
    case LYRICS_SLUG_PROFILE_COMPACT_PERCENT:
    case LYRICS_SLUG_PROFILE_HYPHEN_PERCENT:
    case LYRICS_SLUG_PROFILE_NONE:
    default:
        return false;
    }
}

static bool
lyrics_fold_latin_rune(uint32 rune, char *folded, int32 *folded_len) {
    switch (rune) {
    case 0x00aa:
    case 0x00c0:
    case 0x00c1:
    case 0x00c2:
    case 0x00c3:
    case 0x00c4:
    case 0x00c5:
    case 0x00e0:
    case 0x00e1:
    case 0x00e2:
    case 0x00e3:
    case 0x00e4:
    case 0x00e5:
    case 0x0100:
    case 0x0101:
    case 0x0102:
    case 0x0103:
    case 0x0104:
    case 0x0105:
        folded[0] = 'a';
        *folded_len = 1;
        return true;
    case 0x00c6:
    case 0x00e6:
        folded[0] = 'a';
        folded[1] = 'e';
        *folded_len = 2;
        return true;
    case 0x00c7:
    case 0x00e7:
    case 0x0106:
    case 0x0107:
    case 0x0108:
    case 0x0109:
    case 0x010a:
    case 0x010b:
    case 0x010c:
    case 0x010d:
        folded[0] = 'c';
        *folded_len = 1;
        return true;
    case 0x00d0:
    case 0x00f0:
    case 0x010e:
    case 0x010f:
    case 0x0110:
    case 0x0111:
        folded[0] = 'd';
        *folded_len = 1;
        return true;
    case 0x00c8:
    case 0x00c9:
    case 0x00ca:
    case 0x00cb:
    case 0x00e8:
    case 0x00e9:
    case 0x00ea:
    case 0x00eb:
    case 0x0112:
    case 0x0113:
    case 0x0114:
    case 0x0115:
    case 0x0116:
    case 0x0117:
    case 0x0118:
    case 0x0119:
    case 0x011a:
    case 0x011b:
        folded[0] = 'e';
        *folded_len = 1;
        return true;
    case 0x011c:
    case 0x011d:
    case 0x011e:
    case 0x011f:
    case 0x0120:
    case 0x0121:
    case 0x0122:
    case 0x0123:
        folded[0] = 'g';
        *folded_len = 1;
        return true;
    case 0x0124:
    case 0x0125:
    case 0x0126:
    case 0x0127:
        folded[0] = 'h';
        *folded_len = 1;
        return true;
    case 0x00cc:
    case 0x00cd:
    case 0x00ce:
    case 0x00cf:
    case 0x00ec:
    case 0x00ed:
    case 0x00ee:
    case 0x00ef:
    case 0x0128:
    case 0x0129:
    case 0x012a:
    case 0x012b:
    case 0x012c:
    case 0x012d:
    case 0x012e:
    case 0x012f:
    case 0x0130:
    case 0x0131:
        folded[0] = 'i';
        *folded_len = 1;
        return true;
    case 0x0134:
    case 0x0135:
        folded[0] = 'j';
        *folded_len = 1;
        return true;
    case 0x0136:
    case 0x0137:
        folded[0] = 'k';
        *folded_len = 1;
        return true;
    case 0x0139:
    case 0x013a:
    case 0x013b:
    case 0x013c:
    case 0x013d:
    case 0x013e:
    case 0x013f:
    case 0x0140:
    case 0x0141:
    case 0x0142:
        folded[0] = 'l';
        *folded_len = 1;
        return true;
    case 0x00d1:
    case 0x00f1:
    case 0x0143:
    case 0x0144:
    case 0x0145:
    case 0x0146:
    case 0x0147:
    case 0x0148:
    case 0x014a:
    case 0x014b:
        folded[0] = 'n';
        *folded_len = 1;
        return true;
    case 0x00ba:
    case 0x00d2:
    case 0x00d3:
    case 0x00d4:
    case 0x00d5:
    case 0x00d6:
    case 0x00d8:
    case 0x00f2:
    case 0x00f3:
    case 0x00f4:
    case 0x00f5:
    case 0x00f6:
    case 0x00f8:
    case 0x014c:
    case 0x014d:
    case 0x014e:
    case 0x014f:
    case 0x0150:
    case 0x0151:
        folded[0] = 'o';
        *folded_len = 1;
        return true;
    case 0x0152:
    case 0x0153:
        folded[0] = 'o';
        folded[1] = 'e';
        *folded_len = 2;
        return true;
    case 0x0154:
    case 0x0155:
    case 0x0156:
    case 0x0157:
    case 0x0158:
    case 0x0159:
        folded[0] = 'r';
        *folded_len = 1;
        return true;
    case 0x015a:
    case 0x015b:
    case 0x015c:
    case 0x015d:
    case 0x015e:
    case 0x015f:
    case 0x0160:
    case 0x0161:
        folded[0] = 's';
        *folded_len = 1;
        return true;
    case 0x00df:
        folded[0] = 's';
        folded[1] = 's';
        *folded_len = 2;
        return true;
    case 0x00de:
    case 0x00fe:
        folded[0] = 't';
        folded[1] = 'h';
        *folded_len = 2;
        return true;
    case 0x0162:
    case 0x0163:
    case 0x0164:
    case 0x0165:
    case 0x0166:
    case 0x0167:
        folded[0] = 't';
        *folded_len = 1;
        return true;
    case 0x00d9:
    case 0x00da:
    case 0x00db:
    case 0x00dc:
    case 0x00f9:
    case 0x00fa:
    case 0x00fb:
    case 0x00fc:
    case 0x0168:
    case 0x0169:
    case 0x016a:
    case 0x016b:
    case 0x016c:
    case 0x016d:
    case 0x016e:
    case 0x016f:
    case 0x0170:
    case 0x0171:
    case 0x0172:
    case 0x0173:
        folded[0] = 'u';
        *folded_len = 1;
        return true;
    case 0x0174:
    case 0x0175:
        folded[0] = 'w';
        *folded_len = 1;
        return true;
    case 0x00dd:
    case 0x00fd:
    case 0x00ff:
    case 0x0176:
    case 0x0177:
    case 0x0178:
        folded[0] = 'y';
        *folded_len = 1;
        return true;
    case 0x0179:
    case 0x017a:
    case 0x017b:
    case 0x017c:
    case 0x017d:
    case 0x017e:
        folded[0] = 'z';
        *folded_len = 1;
        return true;
    default:
        return false;
    }
}

static bool
lyrics_skip_slug_rune(uint32 rune) {
    return (rune == '\'') || (rune == '`') || (rune == 0x2018)
           || (rune == 0x2019) || (rune == 0x02bc);
}

static bool
lyrics_slug_separator_rune(uint32 rune) {
    return (rune == 0x00a0) || (rune == 0x1680)
           || ((rune >= 0x2000) && (rune <= 0x200a))
           || (rune == 0x202f) || (rune == 0x205f)
           || (rune == 0x3000)
           || ((rune >= 0x2010) && (rune <= 0x2015))
           || (rune == 0x2212);
}

static bool
lyrics_append_slug_profile(StrBuilder *buffer, LyricsSlugProfile profile,
                           char *string, int32 string_len) {
    bool compact;
    bool folded_profile;
    bool pending_separator;
    bool wrote;

    if (profile == LYRICS_SLUG_PROFILE_NONE) {
        return false;
    }
    compact = lyrics_slug_profile_compact(profile);
    folded_profile = lyrics_slug_profile_folded(profile);
    pending_separator = false;
    wrote = false;
    for (int32 i = 0; i < string_len; i += 1) {
        uint8 byte;
        uint32 rune;
        int32 rune_len;
        char folded[2];
        int32 folded_len;

        byte = (uint8)string[i];
        if (lyrics_ascii_alnum(string[i])) {
            if (pending_separator && wrote && !compact) {
                sb_append_byte(buffer, '-');
            }
            sb_append_byte(buffer, lyrics_ascii_lower(string[i]));
            pending_separator = false;
            wrote = true;
            continue;
        }
        if (byte < 0x80) {
            if (lyrics_skip_slug_rune((uint32)string[i])) {
                continue;
            }
            if (wrote && !compact) {
                pending_separator = true;
            }
            continue;
        }

        rune_len = utf8_decode(string + i, string_len - i, &rune);
        if (lyrics_skip_slug_rune(rune)) {
            i += rune_len - 1;
            continue;
        }
        if (lyrics_slug_separator_rune(rune)) {
            if (wrote && !compact) {
                pending_separator = true;
            }
            i += rune_len - 1;
            continue;
        }
        if (pending_separator && wrote && !compact) {
            sb_append_byte(buffer, '-');
        }
        if (folded_profile
            && lyrics_fold_latin_rune(rune, folded, &folded_len)) {
            sb_append(buffer, folded, folded_len);
        } else {
            for (int32 j = 0; j < rune_len; j += 1) {
                lyrics_append_percent_byte(buffer, (uint8)string[i + j]);
            }
        }
        pending_separator = false;
        wrote = true;
        i += rune_len - 1;
    }
    return wrote;
}

static bool
lyrics_append_slug(StrBuilder *buffer, enum NcmLyricsFetcherType type,
                   char *string, int32 string_len) {
    return lyrics_append_slug_profile(buffer, lyrics_slug_profile(type),
                                      string, string_len);
}

static bool
lyrics_query_safe(char ch) {
    return lyrics_ascii_alnum(ch) || (ch == '-') || (ch == '.')
           || (ch == '_') || (ch == '~');
}

static void
lyrics_append_query(StrBuilder *buffer, char *string, int32 string_len) {
    for (int32 i = 0; i < string_len; i += 1) {
        uint8 byte;

        byte = (uint8)string[i];
        if (lyrics_query_safe(string[i])) {
            sb_append_byte(buffer, string[i]);
        } else if ((string[i] == ' ') || (string[i] == '\t')) {
            sb_append_byte(buffer, '+');
        } else {
            lyrics_append_percent_byte(buffer, byte);
        }
    }
    return;
}

static bool
lyrics_ascii_equal_ignore_case(char left, char right) {
    return lyrics_ascii_lower(left) == lyrics_ascii_lower(right);
}

static bool
lyrics_starts_with_ignore_case(char *string, int32 string_len, char *prefix,
                               int32 prefix_len) {
    if ((string == NULL) || (prefix == NULL) || (prefix_len < 0)
        || (string_len < prefix_len)) {
        return false;
    }
    for (int32 i = 0; i < prefix_len; i += 1) {
        if (!lyrics_ascii_equal_ignore_case(string[i], prefix[i])) {
            return false;
        }
    }
    return true;
}

static int32
lyrics_find_ignore_case(char *data, int32 data_len, char *needle,
                        int32 needle_len, int32 start) {
    if ((data == NULL) || (needle == NULL) || (needle_len <= 0)) {
        return -1;
    }
    if (start < 0) {
        start = 0;
    }
    for (int32 i = start; i + needle_len <= data_len; i += 1) {
        if (lyrics_starts_with_ignore_case(data + i, data_len - i, needle,
                                           needle_len)) {
            return i;
        }
    }
    return -1;
}

static void
lyrics_percent_decode(StrBuilder *out, char *data, int32 data_len) {
    sb_clear(out);
    for (int32 i = 0; i < data_len; i += 1) {
        if ((data[i] == '%') && (i + 2 < data_len)) {
            int32 high;
            int32 low;

            high = lyrics_hex_value(data[i + 1]);
            low = lyrics_hex_value(data[i + 2]);
            if ((high >= 0) && (low >= 0)) {
                sb_append_byte(out, (char)((high << 4) | low));
                i += 2;
                continue;
            }
        }
        sb_append_byte(out, data[i]);
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
        if (ncm_string_starts_with(data + i, data_len - i, needle,
                                   needle_len)) {
            return i;
        }
    }
    return -1;
}

static int32
lyrics_find_char(char *data, int32 data_len, char needle, int32 start) {
    if (start < 0) {
        start = 0;
    }
    for (int32 i = start; i < data_len; i += 1) {
        if (data[i] == needle) {
            return i;
        }
    }
    return -1;
}

static bool
lyrics_extract_after_until(StrBuilder *out, char *data, int32 data_len,
                           char *start, int32 start_len, char *after,
                           int32 after_len, char *end, int32 end_len) {
    int32 a;
    int32 b;

    sb_clear(out);
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
    sb_append(out, data + a, b - a);
    return true;
}

static int32
lyrics_find_tag_end(char *data, int32 data_len, int32 start) {
    char quote;

    quote = '\0';
    for (int32 i = start; i < data_len; i += 1) {
        if (quote != '\0') {
            if (data[i] == quote) {
                quote = '\0';
            }
        } else if ((data[i] == '\'') || (data[i] == '"')) {
            quote = data[i];
        } else if (data[i] == '>') {
            return i;
        }
    }
    return -1;
}

static int32
lyrics_find_matching_div(char *data, int32 data_len, int32 content_start,
                         int32 *close_end) {
    int32 depth;
    int32 pos;

    depth = 1;
    pos = content_start;
    while (pos < data_len) {
        int32 open;
        int32 close;

        open = lyrics_find(data, data_len, STRLIT_ARGS("<div"), pos);
        close = lyrics_find(data, data_len, STRLIT_ARGS("</div"), pos);
        if (close < 0) {
            return -1;
        }
        if ((open >= 0) && (open < close)) {
            int32 tag_end;

            tag_end = lyrics_find_tag_end(data, data_len,
                                          open + STRLIT_LEN("<div"));
            if (tag_end < 0) {
                return -1;
            }
            depth += 1;
            pos = tag_end + 1;
        } else {
            int32 tag_end;

            tag_end = lyrics_find_tag_end(data, data_len,
                                          close + STRLIT_LEN("</div"));
            if (tag_end < 0) {
                return -1;
            }
            depth -= 1;
            if (depth == 0) {
                *close_end = tag_end + 1;
                return close;
            }
            pos = tag_end + 1;
        }
    }
    return -1;
}

static bool
lyrics_extract_divs(StrBuilder *out, char *data, int32 data_len, char *marker,
                     int32 marker_len, bool append_all) {
    int32 pos;
    bool found;

    sb_clear(out);
    pos = 0;
    found = false;
    while (pos < data_len) {
        int32 open;
        int32 open_end;
        int32 close;
        int32 close_end;

        open = lyrics_find(data, data_len, STRLIT_ARGS("<div"), pos);
        if (open < 0) {
            break;
        }
        open_end = lyrics_find_tag_end(data, data_len,
                                       open + STRLIT_LEN("<div"));
        if (open_end < 0) {
            break;
        }
        if (lyrics_find(data + open, open_end - open + 1, marker, marker_len,
                        0)
            < 0) {
            pos = open_end + 1;
            continue;
        }

        close = lyrics_find_matching_div(data, data_len, open_end + 1,
                                         &close_end);
        if (close < 0) {
            break;
        }
        if (found) {
            sb_append(out, STRLIT_ARGS("\n\n"));
        }
        sb_append(out, data + open_end + 1, close - open_end - 1);
        found = true;
        pos = close_end;
        if (!append_all) {
            break;
        }
    }
    return found;
}

static int32
lyrics_hex_value(char ch) {
    if ((ch >= '0') && (ch <= '9')) {
        return ch - '0';
    }
    if ((ch >= 'a') && (ch <= 'f')) {
        return ch - 'a' + 10;
    }
    if ((ch >= 'A') && (ch <= 'F')) {
        return ch - 'A' + 10;
    }
    return -1;
}

static bool
lyrics_url_domain_matches(char *url, int32 url_len, char *domain,
                          int32 domain_len) {
    int32 host_start;
    int32 host_end;
    int32 suffix_start;

    host_start = lyrics_find_ignore_case(url, url_len, STRLIT_ARGS("://"), 0);
    if (host_start < 0) {
        return false;
    }
    host_start += STRLIT_LEN("://");
    host_end = host_start;
    while ((host_end < url_len) && (url[host_end] != '/')
           && (url[host_end] != '?') && (url[host_end] != '#')
           && (url[host_end] != ':')) {
        host_end += 1;
    }
    if (host_end - host_start < domain_len) {
        return false;
    }

    suffix_start = host_end - domain_len;
    if (!lyrics_starts_with_ignore_case(url + suffix_start, domain_len,
                                        domain, domain_len)) {
        return false;
    }
    return (suffix_start == host_start) || (url[suffix_start - 1] == '.');
}

static int32
lyrics_url_path_start(char *url, int32 url_len) {
    int32 scheme;
    int32 path_start;

    scheme = lyrics_find_ignore_case(url, url_len, STRLIT_ARGS("://"), 0);
    if (scheme < 0) {
        return -1;
    }

    path_start = scheme + STRLIT_LEN("://");
    while ((path_start < url_len) && (url[path_start] != '/')
           && (url[path_start] != '?') && (url[path_start] != '#')) {
        path_start += 1;
    }
    if ((path_start >= url_len) || (url[path_start] != '/')) {
        return -1;
    }
    return path_start;
}

static int32
lyrics_url_path_end(char *url, int32 url_len, int32 path_start) {
    int32 path_end;

    path_end = path_start;
    while ((path_end < url_len) && (url[path_end] != '?')
           && (url[path_end] != '#')) {
        path_end += 1;
    }
    return path_end;
}

static bool
lyrics_url_path_starts_with(char *url, int32 url_len, int32 path_start,
                            char *prefix, int32 prefix_len) {
    int32 path_end;

    path_end = lyrics_url_path_end(url, url_len, path_start);
    return lyrics_starts_with_ignore_case(url + path_start,
                                          path_end - path_start, prefix,
                                          prefix_len);
}

static bool
lyrics_url_path_ends_with(char *url, int32 url_len, int32 path_start,
                          char *suffix, int32 suffix_len) {
    int32 path_end;
    int32 suffix_start;

    path_end = lyrics_url_path_end(url, url_len, path_start);
    if (path_end - path_start < suffix_len) {
        return false;
    }
    suffix_start = path_end - suffix_len;
    return lyrics_starts_with_ignore_case(url + suffix_start, suffix_len,
                                          suffix, suffix_len);
}

static bool
lyrics_url_path_has_segments(char *url, int32 url_len, int32 path_start,
                             int32 min_segments) {
    int32 path_end;
    int32 segments;
    bool in_segment;

    path_end = lyrics_url_path_end(url, url_len, path_start);
    segments = 0;
    in_segment = false;
    for (int32 i = path_start; i < path_end; i += 1) {
        if (url[i] == '/') {
            if (in_segment) {
                segments += 1;
                in_segment = false;
            }
        } else {
            in_segment = true;
        }
    }
    if (in_segment) {
        segments += 1;
    }
    return segments >= min_segments;
}

static bool
lyrics_url_query_has_key(char *url, int32 url_len, char *key,
                         int32 key_len) {
    int32 query;

    query = lyrics_find_char(url, url_len, '?', 0);
    if (query < 0) {
        return false;
    }

    query += 1;
    while (query < url_len) {
        int32 equal;
        int32 end;

        equal = lyrics_find_char(url, url_len, '=', query);
        end = lyrics_find_char(url, url_len, '&', query);
        if (end < 0) {
            end = lyrics_find_char(url, url_len, '#', query);
        }
        if (end < 0) {
            end = url_len;
        }
        if ((equal > query) && (equal < end) && (equal - query == key_len)
            && lyrics_starts_with_ignore_case(url + query, equal - query,
                                              key, key_len)) {
            return true;
        }
        query = end + 1;
    }
    return false;
}

static bool
lyrics_url_looks_like_song_page(NcmLyricsFetcherDef *fetcher, char *url,
                                int32 url_len) {
    int32 path_start;
    int32 path_end;

    path_start = lyrics_url_path_start(url, url_len);
    if (path_start < 0) {
        return false;
    }

    switch (fetcher->type) {
    case NCM_LYRICS_FETCHER_AZLYRICS:
        return lyrics_url_path_starts_with(url, url_len, path_start,
                                           STRLIT_ARGS("/lyrics/"))
               && lyrics_url_path_has_segments(url, url_len, path_start, 3)
               && lyrics_url_path_ends_with(url, url_len, path_start,
                                            STRLIT_ARGS(".html"));
    case NCM_LYRICS_FETCHER_GENIUS:
        return lyrics_url_path_has_segments(url, url_len, path_start, 1)
               && lyrics_url_path_ends_with(url, url_len, path_start,
                                            STRLIT_ARGS("-lyrics"));
    case NCM_LYRICS_FETCHER_MUSIXMATCH:
        return (lyrics_url_path_starts_with(url, url_len, path_start,
                                            STRLIT_ARGS("/lyrics/"))
                || lyrics_url_path_starts_with(url, url_len, path_start,
                                               STRLIT_ARGS("/letras/")))
               && lyrics_url_path_has_segments(url, url_len, path_start, 3);
    case NCM_LYRICS_FETCHER_LETRASMUS:
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        return lyrics_url_path_has_segments(url, url_len, path_start, 2);
    case NCM_LYRICS_FETCHER_MUSICA:
        return lyrics_url_path_ends_with(url, url_len, path_start,
                                         STRLIT_ARGS("/letras.asp"))
               && lyrics_url_query_has_key(url, url_len,
                                           STRLIT_ARGS("letra"));
    case NCM_LYRICS_FETCHER_PAROLES:
        path_end = lyrics_url_path_end(url, url_len, path_start);
        return lyrics_url_path_has_segments(url, url_len, path_start, 2)
               && (lyrics_find_ignore_case(url + path_start,
                                           path_end - path_start,
                                           STRLIT_ARGS("/paroles-"), 0) >= 0)
               && !lyrics_url_path_ends_with(url, url_len, path_start,
                                             STRLIT_ARGS("-traduction"));
    case NCM_LYRICS_FETCHER_VAGALUME:
        return lyrics_url_path_has_segments(url, url_len, path_start, 2)
               && lyrics_url_path_ends_with(url, url_len, path_start,
                                            STRLIT_ARGS(".html"));
    case NCM_LYRICS_FETCHER_UNKNOWN:
    case NCM_LYRICS_FETCHER_INTERNET:
    case NCM_LYRICS_FETCHER_LAST:
    default:
        return false;
    }
}

static int32
lyrics_slug_match_score(char *wanted, int32 wanted_len, char *candidate,
                        int32 candidate_len) {
    if ((wanted == NULL) || (candidate == NULL) || (wanted_len <= 0)
        || (candidate_len <= 0)) {
        return 0;
    }

    if (STREQUAL(wanted, wanted_len, candidate, candidate_len)) {
        return 50;
    }
    if ((candidate_len > wanted_len)
        && lyrics_starts_with_ignore_case(candidate, candidate_len, wanted,
                                          wanted_len)
        && (candidate[wanted_len] == '-')) {
        return 40;
    }
    if ((candidate_len > wanted_len)
        && lyrics_starts_with_ignore_case(
            candidate + candidate_len - wanted_len, wanted_len, wanted,
            wanted_len)
        && (candidate[candidate_len - wanted_len - 1] == '-')) {
        return 35;
    }
    if ((candidate_len > wanted_len)
        && (lyrics_find(candidate, candidate_len, wanted, wanted_len, 0)
            > 0)) {
        int32 pos;

        pos = lyrics_find(candidate, candidate_len, wanted, wanted_len, 0);
        if ((candidate[pos - 1] == '-')
            && (pos + wanted_len < candidate_len)
            && (candidate[pos + wanted_len] == '-')) {
            return 30;
        }
    }
    if ((wanted_len > candidate_len)
        && lyrics_starts_with_ignore_case(wanted, wanted_len, candidate,
                                          candidate_len)
        && (wanted[candidate_len] == '-')) {
        return 20;
    }
    return 0;
}

static void
lyrics_trim_url_segment_suffix(char **segment, int32 *segment_len,
                               char *suffix, int32 suffix_len) {
    int32 suffix_start;

    if ((*segment_len < suffix_len) || (suffix_len <= 0)) {
        return;
    }

    suffix_start = *segment_len - suffix_len;
    if (lyrics_starts_with_ignore_case(*segment + suffix_start, suffix_len,
                                       suffix, suffix_len)) {
        *segment_len = suffix_start;
    }
    return;
}

static int32
lyrics_url_segment_slug_score(NcmLyricsFetcherDef *fetcher, char *segment,
                              int32 segment_len, StrBuilder *wanted) {
    StrBuilder decoded;
    StrBuilder slug;
    int32 score;

    lyrics_trim_url_segment_suffix(&segment, &segment_len,
                                   STRLIT_ARGS(".html"));
    lyrics_trim_url_segment_suffix(&segment, &segment_len,
                                   STRLIT_ARGS("-lyrics"));
    if (segment_len <= 0) {
        return 0;
    }

    sb_init(&decoded);
    sb_init(&slug);
    lyrics_percent_decode(&decoded, segment, segment_len);
    if (!lyrics_append_slug(&slug, fetcher->type, decoded.data,
                            decoded.len)) {
        score = 0;
    } else {
        score = lyrics_slug_match_score(wanted->data, wanted->len,
                                        slug.data, slug.len);
    }
    sb_free(&slug);
    sb_free(&decoded);
    return score;
}

static int32
lyrics_url_best_slug_score(NcmLyricsFetcherDef *fetcher, char *url,
                           int32 url_len, StrBuilder *wanted) {
    int32 path_start;
    int32 path_end;
    int32 segment_start;
    int32 best;

    path_start = lyrics_url_path_start(url, url_len);
    if (path_start < 0) {
        return 0;
    }

    path_end = lyrics_url_path_end(url, url_len, path_start);
    segment_start = -1;
    best = 0;
    for (int32 i = path_start; i <= path_end; i += 1) {
        if ((i < path_end) && (url[i] != '/')) {
            if (segment_start < 0) {
                segment_start = i;
            }
            continue;
        }
        if (segment_start >= 0) {
            int32 score;

            score = lyrics_url_segment_slug_score(fetcher, url + segment_start,
                                                  i - segment_start, wanted);
            if (score > best) {
                best = score;
            }
            segment_start = -1;
        }
    }
    return best;
}

static int32
lyrics_search_candidate_score(NcmLyricsFetcherDef *fetcher, char *url,
                              int32 url_len, char *artist, int32 artist_len,
                              char *title, int32 title_len) {
    StrBuilder wanted_artist;
    StrBuilder wanted_title;
    char *domain;
    int32 domain_len;
    int32 artist_score;
    int32 title_score;
    int32 score;

    domain = lyrics_type_domain(fetcher->type, &domain_len);
    if ((domain_len <= 0)
        || !lyrics_url_domain_matches(url, url_len, domain, domain_len)
        || !lyrics_url_looks_like_song_page(fetcher, url, url_len)) {
        return 0;
    }
    if (fetcher->type == NCM_LYRICS_FETCHER_MUSICA) {
        return 1;
    }

    sb_init(&wanted_artist);
    sb_init(&wanted_title);
    if (!lyrics_append_slug(&wanted_artist, fetcher->type, artist, artist_len)
        || !lyrics_append_slug(&wanted_title, fetcher->type, title,
                               title_len)) {
        score = 0;
        goto cleanup;
    }

    artist_score = lyrics_url_best_slug_score(fetcher, url, url_len,
                                              &wanted_artist);
    title_score = lyrics_url_best_slug_score(fetcher, url, url_len,
                                             &wanted_title);
    if ((artist_score <= 0) || (title_score <= 0)) {
        score = 0;
    } else {
        score = title_score*2 + artist_score;
    }

cleanup:
    sb_free(&wanted_title);
    sb_free(&wanted_artist);
    return score;
}

static bool
lyrics_search_wrapper(char *url, int32 url_len) {
    if (lyrics_starts_with_ignore_case(url, url_len,
                                       STRLIT_ARGS("/url?"))
        || lyrics_starts_with_ignore_case(url, url_len,
                                          STRLIT_ARGS("/l/?"))) {
        return true;
    }
    return (lyrics_find_ignore_case(url, url_len,
                                    STRLIT_ARGS("google.com/url?"), 0)
            >= 0)
           || (lyrics_find_ignore_case(url, url_len,
                                       STRLIT_ARGS("duckduckgo.com/l/?"), 0)
               >= 0);
}

static bool
lyrics_unwrap_search_url(StrBuilder *out, char *url, int32 url_len) {
    int32 query;

    sb_clear(out);
    if (!lyrics_search_wrapper(url, url_len)) {
        if (lyrics_starts_with_ignore_case(url, url_len,
                                           STRLIT_ARGS("http://"))
            || lyrics_starts_with_ignore_case(url, url_len,
                                              STRLIT_ARGS("https://"))) {
            sb_append(out, url, url_len);
            return true;
        }
        return false;
    }

    query = lyrics_find_char(url, url_len, '?', 0);
    if (query < 0) {
        return false;
    }
    query += 1;
    while (query < url_len) {
        int32 equal;
        int32 end;

        equal = lyrics_find_char(url, url_len, '=', query);
        end = lyrics_find_char(url, url_len, '&', query);
        if (end < 0) {
            end = url_len;
        }
        if ((equal > query) && (equal < end)
            && (STREQUAL(url + query, equal - query, STRLIT_ARGS("q"))
                || STREQUAL(url + query, equal - query,
                            STRLIT_ARGS("url"))
                || STREQUAL(url + query, equal - query,
                            STRLIT_ARGS("uddg")))) {
            lyrics_percent_decode(out, url + equal + 1, end - equal - 1);
            if (lyrics_starts_with_ignore_case(out->data, out->len,
                                               STRLIT_ARGS("http://"))
                || lyrics_starts_with_ignore_case(out->data, out->len,
                                                  STRLIT_ARGS("https://"))) {
                return true;
            }
            sb_clear(out);
        }
        query = end + 1;
    }
    return false;
}

static bool
lyrics_insert_search_url(StrBuilderArray *urls, int32 *scores, char *url,
                         int32 url_len, int32 score) {
    StrBuilder *item;
    int32 pos;

    if ((urls->len >= LYRICS_SEARCH_MAX_CANDIDATES)
        && (score <= scores[LYRICS_SEARCH_MAX_CANDIDATES - 1])) {
        return true;
    }

    item = str_builder_array_append(urls);
    if (item == NULL) {
        return false;
    }

    sb_append(item, url, url_len);
    scores[urls->len - 1] = score;
    pos = urls->len - 1;
    while ((pos > 0) && (scores[pos] > scores[pos - 1])) {
        StrBuilder temp_url;
        int32 temp_score;

        temp_url = urls->items[pos - 1];
        urls->items[pos - 1] = urls->items[pos];
        urls->items[pos] = temp_url;

        temp_score = scores[pos - 1];
        scores[pos - 1] = scores[pos];
        scores[pos] = temp_score;
        pos -= 1;
    }
    if (urls->len > LYRICS_SEARCH_MAX_CANDIDATES) {
        urls->len -= 1;
        sb_free(&urls->items[urls->len]);
    }
    return true;
}

static bool
lyrics_collect_search_urls(NcmLyricsFetcherDef *fetcher, StrBuilderArray *out,
                           char *data, int32 data_len, char *artist,
                           int32 artist_len, char *title, int32 title_len) {
    StrBuilder numeric_unescaped;
    StrBuilder unescaped;
    StrBuilder candidate;
    int32 scores[LYRICS_SEARCH_MAX_CANDIDATES];
    int32 pos;
    bool ok;

    numeric_unescaped = ncm_html_unescape_utf8(data, data_len);
    unescaped = ncm_html_unescape_entities(numeric_unescaped.data,
                                           numeric_unescaped.len);
    sb_free(&numeric_unescaped);
    sb_init(&candidate);
    str_builder_array_clear(out);
    pos = 0;
    ok = true;
    while (pos < unescaped.len) {
        int32 href;
        int32 value_start;
        int32 value_end;
        char quote;

        href = lyrics_find_ignore_case(unescaped.data, unescaped.len,
                                       STRLIT_ARGS("href"), pos);
        if (href < 0) {
            break;
        }
        value_start = href + STRLIT_LEN("href");
        while ((value_start < unescaped.len)
               && ((unescaped.data[value_start] == ' ')
                   || (unescaped.data[value_start] == '\t')
                   || (unescaped.data[value_start] == '\r')
                   || (unescaped.data[value_start] == '\n'))) {
            value_start += 1;
        }
        if ((value_start >= unescaped.len)
            || (unescaped.data[value_start] != '=')) {
            pos = value_start;
            continue;
        }
        value_start += 1;
        while ((value_start < unescaped.len)
               && ((unescaped.data[value_start] == ' ')
                   || (unescaped.data[value_start] == '\t')
                   || (unescaped.data[value_start] == '\r')
                   || (unescaped.data[value_start] == '\n'))) {
            value_start += 1;
        }
        if (value_start >= unescaped.len) {
            break;
        }

        quote = unescaped.data[value_start];
        if ((quote == '\'') || (quote == '"')) {
            value_start += 1;
            value_end = lyrics_find_char(unescaped.data, unescaped.len, quote,
                                         value_start);
        } else {
            value_end = value_start;
            while ((value_end < unescaped.len)
                   && (unescaped.data[value_end] != ' ')
                   && (unescaped.data[value_end] != '\t')
                   && (unescaped.data[value_end] != '\r')
                   && (unescaped.data[value_end] != '\n')
                   && (unescaped.data[value_end] != '>')) {
                value_end += 1;
            }
        }
        if (value_end < 0) {
            break;
        }
        if (lyrics_unwrap_search_url(&candidate,
                                     unescaped.data + value_start,
                                     value_end - value_start)
            && !lyrics_url_collected(out, candidate.data,
                                     candidate.len)) {
            int32 score;

            score = lyrics_search_candidate_score(fetcher, candidate.data,
                                                  candidate.len, artist,
                                                  artist_len, title,
                                                  title_len);
            if (score <= 0) {
                pos = value_end + 1;
                continue;
            }
            if (!lyrics_insert_search_url(out, scores, candidate.data,
                                          candidate.len, score)) {
                ok = false;
                break;
            }
        }
        pos = value_end + 1;
    }

    sb_free(&candidate);
    sb_free(&unescaped);
    return ok && (out->len > 0);
}

static bool
lyrics_parse_hex4(char *data, int32 data_len, int32 start, uint32 *value) {
    uint32 result;

    if ((start < 0) || (start + 4 > data_len)) {
        return false;
    }
    result = 0;
    for (int32 i = 0; i < 4; i += 1) {
        int32 digit;

        digit = lyrics_hex_value(data[start + i]);
        if (digit < 0) {
            return false;
        }
        result = (result << 4) | (uint32)digit;
    }
    *value = result;
    return true;
}

static void
lyrics_append_rune(StrBuilder *out, uint32 rune) {
    char encoded[4];
    int32 encoded_len;

    encoded_len = utf8_encode(rune, encoded, SIZEOF(encoded));
    if (encoded_len > 0) {
        sb_append(out, encoded, encoded_len);
    }
    return;
}

static bool
lyrics_decode_quoted(StrBuilder *out, char *data, int32 data_len, int32 start,
                     char quote, int32 *end) {
    int32 i;

    sb_clear(out);
    i = start;
    while (i < data_len) {
        char escaped;

        if (data[i] == quote) {
            *end = i + 1;
            return true;
        }
        if (data[i] != '\\') {
            sb_append_byte(out, data[i]);
            i += 1;
            continue;
        }

        i += 1;
        if (i >= data_len) {
            return false;
        }
        escaped = data[i];
        i += 1;
        switch (escaped) {
        case 'b':
            sb_append_byte(out, '\b');
            break;
        case 'f':
            sb_append_byte(out, '\f');
            break;
        case 'n':
            sb_append_byte(out, '\n');
            break;
        case 'r':
            sb_append_byte(out, '\r');
            break;
        case 't':
            sb_append_byte(out, '\t');
            break;
        case 'u': {
            uint32 first;
            uint32 rune;

            if (!lyrics_parse_hex4(data, data_len, i, &first)) {
                return false;
            }
            i += 4;
            rune = first;
            if ((first >= 0xd800) && (first <= 0xdbff)
                && (i + 6 <= data_len) && (data[i] == '\\')
                && (data[i + 1] == 'u')) {
                uint32 second;

                if (lyrics_parse_hex4(data, data_len, i + 2, &second)
                    && (second >= 0xdc00) && (second <= 0xdfff)) {
                    rune = 0x10000 + ((first - 0xd800) << 10)
                           + (second - 0xdc00);
                    i += 6;
                }
            }
            lyrics_append_rune(out, rune);
            break;
        }
        case '\n':
            break;
        case '\r':
            if ((i < data_len) && (data[i] == '\n')) {
                i += 1;
            }
            break;
        default:
            sb_append_byte(out, escaped);
            break;
        }
    }
    return false;
}

static bool
lyrics_json_value_start(char *data, int32 data_len, char *key,
                        int32 key_len, int32 start, int32 *value_start) {
    StrBuilder pattern;
    int32 key_pos;
    int32 pos;

    sb_init(&pattern);
    sb_append_byte(&pattern, '"');
    sb_append(&pattern, key, key_len);
    sb_append_byte(&pattern, '"');
    key_pos = lyrics_find(data, data_len, pattern.data, pattern.len, start);
    sb_free(&pattern);
    if (key_pos < 0) {
        return false;
    }

    pos = key_pos + key_len + 2;
    while ((pos < data_len)
           && ((data[pos] == ' ') || (data[pos] == '\t')
               || (data[pos] == '\r') || (data[pos] == '\n'))) {
        pos += 1;
    }
    if ((pos >= data_len) || (data[pos] != ':')) {
        return false;
    }
    pos += 1;
    while ((pos < data_len)
           && ((data[pos] == ' ') || (data[pos] == '\t')
               || (data[pos] == '\r') || (data[pos] == '\n'))) {
        pos += 1;
    }
    if ((pos >= data_len) || (data[pos] != '"')) {
        return false;
    }
    *value_start = pos + 1;
    return true;
}

static bool
lyrics_extract_genius(StrBuilder *out, char *data, int32 data_len) {
    StrBuilder json;
    StrBuilder html;
    int32 marker;
    int32 json_end;
    int32 lyrics_data;
    int32 value_start;
    int32 value_end;
    bool ok;

    sb_init(&json);
    sb_init(&html);
    marker = lyrics_find(data, data_len,
                         STRLIT_ARGS("window.__PRELOADED_STATE__ = "
                                     "JSON.parse('"),
                         0);
    ok = false;
    if (marker >= 0) {
        marker += STRLIT_LEN("window.__PRELOADED_STATE__ = JSON.parse('");
        if (lyrics_decode_quoted(&json, data, data_len, marker, '\'',
                                 &json_end)) {
            lyrics_data = lyrics_find(json.data, json.len,
                                      STRLIT_ARGS("\"lyricsData\""), 0);
            if ((lyrics_data >= 0)
                && lyrics_json_value_start(json.data, json.len,
                                           STRLIT_ARGS("html"), lyrics_data,
                                           &value_start)
                && lyrics_decode_quoted(&html, json.data, json.len,
                                        value_start, '"', &value_end)) {
                sb_clear(out);
                sb_append(out, html.data, html.len);
                ok = out->len > 0;
            }
        }
    }

    sb_free(&html);
    sb_free(&json);
    if (!ok) {
        ok = lyrics_extract_divs(out, data, data_len,
                                 STRLIT_ARGS("data-lyrics-container=\"true\""),
                                 true);
    }
    return ok;
}

static bool
lyrics_extract_musixmatch(StrBuilder *out, char *data, int32 data_len) {
    int32 track_info;
    int32 lyrics;
    int32 value_start;
    int32 value_end;

    track_info = lyrics_find(data, data_len, STRLIT_ARGS("\"trackInfo\""), 0);
    if (track_info < 0) {
        track_info = 0;
    }
    lyrics = lyrics_find(data, data_len, STRLIT_ARGS("\"lyrics\""),
                         track_info);
    if (lyrics < 0) {
        return false;
    }
    if (!lyrics_json_value_start(data, data_len, STRLIT_ARGS("body"), lyrics,
                                 &value_start)) {
        return false;
    }
    return lyrics_decode_quoted(out, data, data_len, value_start, '"',
                                &value_end);
}

static void
lyrics_update_first_match(char *data, int32 data_len, int32 start,
                          char *needle, int32 needle_len, int32 *best) {
    int32 pos;

    pos = lyrics_find_ignore_case(data, data_len, needle, needle_len, start);
    if ((pos >= 0) && ((*best < 0) || (pos < *best))) {
        *best = pos;
    }
    return;
}

static int32
lyrics_musica_content_start(char *data, int32 data_len) {
    int32 marker;
    int32 content_start;

    marker = lyrics_find_ignore_case(data, data_len, STRLIT_ARGS(">LETRA<"), 0);
    if (marker < 0) {
        return -1;
    }

    content_start = lyrics_find_char(data, data_len, '>',
                                     marker + STRLIT_LEN(">LETRA<") - 1);
    if (content_start < 0) {
        return -1;
    }
    return content_start + 1;
}

static int32
lyrics_musica_content_end(char *data, int32 data_len, int32 start) {
    int32 best;

    best = -1;
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("Significado de la letra"), &best);
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("Puntuar "), &best);
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("Imprimir letra"), &best);
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("Letra añadida"), &best);
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("+ Letras de"), &best);
    return best;
}

static bool
lyrics_extract_musica(StrBuilder *out, char *data, int32 data_len) {
    int32 start;
    int32 end;

    sb_clear(out);
    start = lyrics_musica_content_start(data, data_len);
    if (start < 0) {
        return false;
    }

    end = lyrics_musica_content_end(data, data_len, start);
    if (end < 0) {
        return false;
    }
    if (end <= start) {
        return false;
    }

    sb_append(out, data + start, end - start);
    return true;
}

static int32
lyrics_paroles_content_start(char *data, int32 data_len) {
    int32 marker;
    int32 second_marker;
    int32 content_start;

    marker = lyrics_find_ignore_case(
        data, data_len, STRLIT_ARGS("Paroles de la chanson"), 0);
    if (marker < 0) {
        return -1;
    }

    second_marker = lyrics_find_ignore_case(
        data, data_len, STRLIT_ARGS("Paroles de la chanson"),
        marker + STRLIT_LEN("Paroles de la chanson"));
    if (second_marker >= 0) {
        marker = second_marker;
    }

    content_start = lyrics_find_char(data, data_len, '>', marker);
    if (content_start < 0) {
        return -1;
    }
    return content_start + 1;
}

static int32
lyrics_paroles_content_end(char *data, int32 data_len, int32 start) {
    int32 best;

    best = -1;
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("Paroles.net dispose"), &best);
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("Sélection du moment"), &best);
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("Les plus grands succès"), &best);
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("On aime"), &best);
    lyrics_update_first_match(data, data_len, start,
                              STRLIT_ARGS("Top traduction"), &best);
    return best;
}

static bool
lyrics_extract_paroles(StrBuilder *out, char *data, int32 data_len) {
    int32 start;
    int32 end;

    sb_clear(out);
    start = lyrics_paroles_content_start(data, data_len);
    if (start < 0) {
        return false;
    }

    end = lyrics_paroles_content_end(data, data_len, start);
    if (end < 0) {
        return false;
    }
    if (end <= start) {
        return false;
    }

    sb_append(out, data + start, end - start);
    return true;
}

static bool
lyrics_extract_content(NcmLyricsFetcherDef *fetcher, StrBuilder *out,
                       char *data, int32 data_len, bool *plain_text) {
    *plain_text = false;
    switch (fetcher->type) {
    case NCM_LYRICS_FETCHER_AZLYRICS:
        return lyrics_extract_after_until(
            out, data, data_len, STRLIT_ARGS("Usage of azlyrics.com"),
            STRLIT_ARGS("-->"), STRLIT_ARGS("</div>"));
    case NCM_LYRICS_FETCHER_GENIUS:
        return lyrics_extract_genius(out, data, data_len);
    case NCM_LYRICS_FETCHER_LETRASMUS:
        return lyrics_extract_divs(out, data, data_len,
                                   STRLIT_ARGS("class=\"lyric-original\""),
                                   false);
    case NCM_LYRICS_FETCHER_MUSICA:
        return lyrics_extract_musica(out, data, data_len);
    case NCM_LYRICS_FETCHER_PAROLES:
        return lyrics_extract_paroles(out, data, data_len);
    case NCM_LYRICS_FETCHER_MUSIXMATCH:
        *plain_text = true;
        return lyrics_extract_musixmatch(out, data, data_len);
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        return lyrics_extract_divs(out, data, data_len,
                                   STRLIT_ARGS("class=\"inner-text\""),
                                   false);
    case NCM_LYRICS_FETCHER_VAGALUME:
        if (lyrics_extract_divs(out, data, data_len,
                                STRLIT_ARGS("id=\"lyrics\""), false)) {
            return true;
        }
        return lyrics_extract_divs(out, data, data_len,
                                   STRLIT_ARGS("id=lyrics"), false);
    case NCM_LYRICS_FETCHER_UNKNOWN:
    case NCM_LYRICS_FETCHER_INTERNET:
    case NCM_LYRICS_FETCHER_LAST:
    default:
        sb_clear(out);
        return false;
    }
}

static CURLcode
lyrics_curl_perform(StrBuilder *data, char *url, int32 url_len, char *referer,
                    int32 referer_len, bool follow_redirect,
                    int32 timeout_seconds) {
    if (lyrics_test_perform) {
        return lyrics_test_perform(data, url, url_len, referer, referer_len,
                                   follow_redirect, timeout_seconds,
                                   lyrics_test_user);
    }
    return ncm_curl_perform(data, url, url_len, referer, referer_len,
                            follow_redirect, timeout_seconds);
}

static char *
lyrics_curl_error(CURLcode code, int32 *message_len) {
    if (code == CURLE_OPERATION_TIMEDOUT) {
        *message_len = STRLIT_LEN(LYRICS_MSG_TIMED_OUT);
        return LYRICS_MSG_TIMED_OUT;
    }
    *message_len = STRLIT_LEN(LYRICS_MSG_DOWNLOAD_FAILED);
    return LYRICS_MSG_DOWNLOAD_FAILED;
}

static bool
lyrics_fetch_page(NcmLyricsFetcherDef *fetcher, NcmLyricsResult *result,
                  StrBuilder *url, char *referer, int32 referer_len,
                  bool *retry) {
    CURLcode code;
    StrBuilder data;
    StrBuilder lyrics;
    StrBuilder cleaned;
    char *message;
    int32 message_len;
    bool extracted;
    bool plain_text;

    sb_init(&data);
    sb_init(&lyrics);
    sb_init(&cleaned);
    *retry = false;
    code = lyrics_curl_perform(&data, url->data, url->len, referer, referer_len,
                               true, 15);
    if (code != CURLE_OK) {
        message = lyrics_curl_error(code, &message_len);
        ncm_lyrics_result_set(result, false, message, message_len);
        *retry = code != CURLE_OPERATION_TIMEDOUT;
        goto cleanup;
    }

    if ((fetcher->type == NCM_LYRICS_FETCHER_AZLYRICS)
        && (lyrics_find(data.data, data.len,
                        STRLIT_ARGS("request for access"), 0)
            >= 0)) {
        ncm_lyrics_result_set(result, false,
                              STRLIT_ARGS(LYRICS_MSG_ACCESS_DENIED));
        goto cleanup;
    }

    extracted = lyrics_extract_content(fetcher, &lyrics, data.data, data.len,
                                        &plain_text);
    if (!extracted || (lyrics.len <= 0)) {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS(LYRICS_MSG_NOT_FOUND));
        *retry = true;
        goto cleanup;
    }

    if (plain_text) {
        lyrics_append_clean_lines(&cleaned, lyrics.data, lyrics.len);
    } else {
        ncm_lyrics_cleanup_html(&cleaned, lyrics.data, lyrics.len);
    }
    if (cleaned.len <= 0) {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS(LYRICS_MSG_NOT_FOUND));
        *retry = true;
        goto cleanup;
    }
    ncm_lyrics_result_set(result, true, cleaned.data, cleaned.len);

cleanup:
    sb_free(&cleaned);
    sb_free(&lyrics);
    sb_free(&data);
    return true;
}

static bool
lyrics_fetch_search(NcmLyricsFetcherDef *fetcher, NcmLyricsResult *result,
                    char *artist, int32 artist_len, char *title,
                    int32 title_len) {
    CURLcode code;
    StrBuilder search_url;
    StrBuilder data;
    StrBuilderArray page_urls;
    char *message;
    int32 message_len;
    bool retry;
    bool ok;

    sb_init(&search_url);
    sb_init(&data);
    str_builder_array_init(&page_urls);
    ok = ncm_lyrics_fetcher_build_url(fetcher, &search_url, artist,
                                      artist_len, title, title_len);
    if (!ok) {
        goto cleanup;
    }

    code = lyrics_curl_perform(&data, search_url.data, search_url.len, NULL, 0,
                               true, 15);
    if (code != CURLE_OK) {
        message = lyrics_curl_error(code, &message_len);
        ncm_lyrics_result_set(result, false, message, message_len);
        ok = true;
        goto cleanup;
    }
    if (!lyrics_collect_search_urls(fetcher, &page_urls, data.data,
                                    data.len, artist, artist_len, title,
                                    title_len)) {
        ncm_lyrics_result_set(result, false,
                              STRLIT_ARGS(LYRICS_MSG_NOT_FOUND));
        ok = true;
        goto cleanup;
    }

    for (int32 i = 0; i < page_urls.len; i += 1) {
        ok = lyrics_fetch_page(fetcher, result, &page_urls.items[i],
                               search_url.data, search_url.len, &retry);
        if (!ok || result->success || !retry) {
            break;
        }
    }

cleanup:
    str_builder_array_destroy(&page_urls);
    sb_free(&data);
    sb_free(&search_url);
    return ok;
}

static bool
lyrics_fetch_internet(NcmLyricsFetcherDef *fetcher, NcmLyricsResult *result,
                      char *artist, int32 artist_len, char *title,
                      int32 title_len) {
    StrBuilder url;
    StrBuilder message;
    bool ok;

    sb_init(&url);
    sb_init(&message);
    ok = ncm_lyrics_fetcher_build_url(fetcher, &url, artist, artist_len, title,
                                      title_len);
    if (ok) {
        sb_append(&message,
                          STRLIT_ARGS("The following search may contain lyrics "
                                      "for this song: "));
        sb_append(&message, url.data, url.len);
        ncm_lyrics_result_set(result, false, message.data, message.len);
    }
    sb_free(&message);
    sb_free(&url);
    return ok;
}

static void
lyrics_trim_view(char **data, int32 *len) {
    char *text;
    int32 text_len;

    text = *data;
    text_len = *len;
    while ((text_len > 0)
           && ((text[0] == ' ') || (text[0] == '\t') || (text[0] == '\r')
               || (text[0] == '\n'))) {
        text += 1;
        text_len -= 1;
    }
    while ((text_len > 0)
           && ((text[text_len - 1] == ' ') || (text[text_len - 1] == '\t')
               || (text[text_len - 1] == '\r')
               || (text[text_len - 1] == '\n'))) {
        text_len -= 1;
    }
    *data = text;
    *len = text_len;
    return;
}

static void
lyrics_trim_buffer(StrBuilder *buffer) {
    char *text;
    int32 text_len;
    StrBuilder tmp;

    text = buffer->data;
    text_len = buffer->len;
    lyrics_trim_view(&text, &text_len);
    sb_init(&tmp);
    sb_append(&tmp, text, text_len);
    sb_clear(buffer);
    sb_append(buffer, tmp.data, tmp.len);
    sb_free(&tmp);
    return;
}

static void
lyrics_append_clean_lines(StrBuilder *out, char *data, int32 data_len) {
    int32 line_start;
    bool previous_empty;

    sb_clear(out);
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
                    sb_append_byte(out, '\n');
                }
                sb_append(out, line, line_len);
                previous_empty = false;
            } else if (!previous_empty) {
                sb_append_byte(out, '\n');
                previous_empty = true;
            }
            if ((i < data_len) && (data[i] == '\r')
                && (i + 1 < data_len) && (data[i + 1] == '\n')) {
                i += 1;
            }
            line_start = i + 1;
        }
    }
    lyrics_trim_buffer(out);
    return;
}

#endif /* NCMPCPP_LYRICS_FETCHER_C */
