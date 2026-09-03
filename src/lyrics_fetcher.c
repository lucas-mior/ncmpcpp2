#if !defined(NCMPCPP_LYRICS_FETCHER_C)
#define NCMPCPP_LYRICS_FETCHER_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "curl_handle.h"
#include "lyrics_fetcher.h"

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
    LYRICS_SLUG_PROFILE_UNDERSCORE_FOLDED,
    LYRICS_SLUG_PROFILE_COMPACT_PERCENT,
    LYRICS_SLUG_PROFILE_HYPHEN_PERCENT,
    LYRICS_SLUG_PROFILE_UNDERSCORE_PERCENT,
} LyricsSlugProfile;

enum LyricsProviderFlags {
    LYRICS_PROVIDER_DIRECT_URLS = 1u << 0,
    LYRICS_PROVIDER_SEARCH_URLS = 1u << 1,
    LYRICS_PROVIDER_NUMERIC_PAGE_IDS = 1u << 2,
    LYRICS_PROVIDER_TRANSLATION_PAGES = 1u << 3,
};

typedef struct LyricsProviderProfile {
    char *name;
    char *domain;

    int32 name_len;
    int32 domain_len;
    LyricsSlugProfile slug_profile;
    uint32 flags;
} LyricsProviderProfile;

typedef struct LyricsDirectSlugPair {
    LyricsSlugProfile artist;
    LyricsSlugProfile title;
} LyricsDirectSlugPair;

static void lyrics_string_destroy(char **data, int32 *len, int32 *cap);
static void lyrics_fetcher_array_destroy_item(void *item);
static LyricsProviderProfile *lyrics_provider_profile(
    enum NcmLyricsFetcherType type
);
static bool lyrics_provider_has_flag(enum NcmLyricsFetcherType type,
                                     uint32 flag);
static char *lyrics_type_domain(enum NcmLyricsFetcherType type, int32 *len);
static LyricsSlugProfile lyrics_slug_profile(enum NcmLyricsFetcherType type);
static bool lyrics_slug_rune_should_be_skipped(uint32 rune);
static int32 lyrics_append_slug_profile(StrBuilder *buffer,
                                        LyricsSlugProfile profile, char *string,
                                        int32 string_len);
static int32 lyrics_append_slug(StrBuilder *buffer,
                                enum NcmLyricsFetcherType type, char *string,
                                int32 string_len);
static void lyrics_append_query(StrBuilder *buffer,
                                char *string, int32 string_len);
static int32 lyrics_hex_value(char ch);
static bool lyrics_url_is_collected(StrBuilderArray *urls, char *url,
                                    int32 url_len);
static int32 lyrics_fetch_page(NcmLyricsFetcherDef *fetcher,
                               NcmLyricsResult *result, StrBuilder *url,
                               char *referer, int32 referer_len,
                               bool *retry);
static void lyrics_append_clean_lines(StrBuilder *out, char *data,
                                      int32 data_len);

static NcmArrayItemCallbacks lyrics_fetcher_callbacks = {
    .destroy = lyrics_fetcher_array_destroy_item,
};

NCM_ARRAY_DEFINE_CLEAR(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray,
                       &lyrics_fetcher_callbacks)
NCM_ARRAY_DEFINE_DESTROY(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DEFINE_RESERVE(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DEFINE_APPEND(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray,
                        NcmLyricsFetcherDef, &lyrics_fetcher_callbacks)

static LyricsProviderProfile lyrics_provider_profiles[] = {
    [NCM_LYRICS_FETCHER_AMALGAMA] = {
        .name = "amalgama-lab.com",
        .domain = "amalgama-lab.com",
        .name_len = STRLIT_LEN("amalgama-lab.com"),
        .domain_len = STRLIT_LEN("amalgama-lab.com"),
        .slug_profile = LYRICS_SLUG_PROFILE_UNDERSCORE_FOLDED,
        .flags = LYRICS_PROVIDER_DIRECT_URLS|LYRICS_PROVIDER_SEARCH_URLS
                 |LYRICS_PROVIDER_TRANSLATION_PAGES,
    },
    [NCM_LYRICS_FETCHER_AZLYRICS] = {
        .name = "azlyrics.com",
        .domain = "azlyrics.com",
        .name_len = STRLIT_LEN("azlyrics.com"),
        .domain_len = STRLIT_LEN("azlyrics.com"),
        .slug_profile = LYRICS_SLUG_PROFILE_COMPACT_FOLDED,
        .flags = LYRICS_PROVIDER_DIRECT_URLS|LYRICS_PROVIDER_SEARCH_URLS,
    },
    [NCM_LYRICS_FETCHER_GENIUS] = {
        .name = "genius.com",
        .domain = "genius.com",
        .name_len = STRLIT_LEN("genius.com"),
        .domain_len = STRLIT_LEN("genius.com"),
        .slug_profile = LYRICS_SLUG_PROFILE_HYPHEN_FOLDED,
        .flags = LYRICS_PROVIDER_DIRECT_URLS|LYRICS_PROVIDER_SEARCH_URLS,
    },
    [NCM_LYRICS_FETCHER_LETRASMUS] = {
        .name = "letras.mus.br",
        .domain = "letras.mus.br",
        .name_len = STRLIT_LEN("letras.mus.br"),
        .domain_len = STRLIT_LEN("letras.mus.br"),
        .slug_profile = LYRICS_SLUG_PROFILE_HYPHEN_FOLDED,
        .flags = LYRICS_PROVIDER_DIRECT_URLS|LYRICS_PROVIDER_SEARCH_URLS,
    },
    [NCM_LYRICS_FETCHER_LACOCCINELLE] = {
        .name = "lacoccinelle.net",
        .domain = "lacoccinelle.net",
        .name_len = STRLIT_LEN("lacoccinelle.net"),
        .domain_len = STRLIT_LEN("lacoccinelle.net"),
        .slug_profile = LYRICS_SLUG_PROFILE_HYPHEN_FOLDED,
        .flags = LYRICS_PROVIDER_SEARCH_URLS
                 |LYRICS_PROVIDER_NUMERIC_PAGE_IDS
                 |LYRICS_PROVIDER_TRANSLATION_PAGES,
    },
    [NCM_LYRICS_FETCHER_MUSICA] = {
        .name = "musica.com",
        .domain = "musica.com",
        .name_len = STRLIT_LEN("musica.com"),
        .domain_len = STRLIT_LEN("musica.com"),
        .slug_profile = LYRICS_SLUG_PROFILE_NONE,
        .flags = LYRICS_PROVIDER_SEARCH_URLS
                 |LYRICS_PROVIDER_NUMERIC_PAGE_IDS,
    },
    [NCM_LYRICS_FETCHER_PAROLES] = {
        .name = "paroles.net",
        .domain = "paroles.net",
        .name_len = STRLIT_LEN("paroles.net"),
        .domain_len = STRLIT_LEN("paroles.net"),
        .slug_profile = LYRICS_SLUG_PROFILE_HYPHEN_FOLDED,
        .flags = LYRICS_PROVIDER_DIRECT_URLS|LYRICS_PROVIDER_SEARCH_URLS
                 |LYRICS_PROVIDER_TRANSLATION_PAGES,
    },
    [NCM_LYRICS_FETCHER_MUSIXMATCH] = {
        .name = "musixmatch.com",
        .domain = "musixmatch.com",
        .name_len = STRLIT_LEN("musixmatch.com"),
        .domain_len = STRLIT_LEN("musixmatch.com"),
        .slug_profile = LYRICS_SLUG_PROFILE_HYPHEN_FOLDED,
        .flags = LYRICS_PROVIDER_DIRECT_URLS|LYRICS_PROVIDER_SEARCH_URLS,
    },
    [NCM_LYRICS_FETCHER_TEKSTOWO] = {
        .name = "tekstowo.pl",
        .domain = "tekstowo.pl",
        .name_len = STRLIT_LEN("tekstowo.pl"),
        .domain_len = STRLIT_LEN("tekstowo.pl"),
        .slug_profile = LYRICS_SLUG_PROFILE_HYPHEN_FOLDED,
        .flags = LYRICS_PROVIDER_DIRECT_URLS|LYRICS_PROVIDER_SEARCH_URLS,
    },
    [NCM_LYRICS_FETCHER_VAGALUME] = {
        .name = "vagalume.com.br",
        .domain = "vagalume.com.br",
        .name_len = STRLIT_LEN("vagalume.com.br"),
        .domain_len = STRLIT_LEN("vagalume.com.br"),
        .slug_profile = LYRICS_SLUG_PROFILE_HYPHEN_FOLDED,
        .flags = LYRICS_PROVIDER_DIRECT_URLS|LYRICS_PROVIDER_SEARCH_URLS,
    },
    [NCM_LYRICS_FETCHER_INTERNET] = {
        .name = "the Internet",
        .domain = "",
        .name_len = STRLIT_LEN("the Internet"),
        .domain_len = 0,
        .slug_profile = LYRICS_SLUG_PROFILE_NONE,
        .flags = LYRICS_PROVIDER_SEARCH_URLS,
    },
};

static void
lyrics_string_set(char **data, int32 *len, int32 *cap,
                  char *source, int32 source_len) {
    char *new_data;
    int32 new_cap;

    lyrics_string_destroy(data, len, cap);
    if ((source == NULL) || (source_len <= 0)) {
        return;
    }

    new_cap = source_len + 1;
    new_data = malloc2(new_cap);
    memcpy64(new_data, source, source_len);
    new_data[source_len] = '\0';

    *data = new_data;
    *len = source_len;
    *cap = new_cap;

    return;
}

static void
lyrics_string_destroy(char **data, int32 *len, int32 *cap) {
    free2(*data, *cap);
    *data = NULL;
    *len = 0;
    *cap = 0;
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

int32
ncm_lyrics_result_set(NcmLyricsResult *result, bool success,
                      char *text, int32 text_len) {
    if (result == NULL) {
        return -EINVAL;
    }
    lyrics_string_set(&result->text, &result->text_len, &result->text_cap,
                      text, text_len);
    result->success = success;
    return 0;
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

int32
ncm_lyrics_fetcher_def_set_name(NcmLyricsFetcherDef *fetcher,
                                char *name, int32 name_len) {
    LyricsProviderProfile *profile;
    enum NcmLyricsFetcherType type;
    char *display_name;
    int32 display_name_len;

    if ((fetcher == NULL) || (name == NULL) || (name_len <= 0)) {
        return -EINVAL;
    }

    type = NCM_LYRICS_FETCHER_UNKNOWN;
    if (STREQUAL(name, name_len, "amalgama")
        || STREQUAL(name, name_len, "amalgamalab")
        || STREQUAL(name, name_len, "amalgama-lab")
        || STREQUAL(name, name_len, "amalgama-lab.com")) {
        type = NCM_LYRICS_FETCHER_AMALGAMA;
    } else if (STREQUAL(name, name_len, "azlyrics")
               || STREQUAL(name, name_len, "azlyrics.com")) {
        type = NCM_LYRICS_FETCHER_AZLYRICS;
    } else if (STREQUAL(name, name_len, "genius")
               || STREQUAL(name, name_len, "genius.com")) {
        type = NCM_LYRICS_FETCHER_GENIUS;
    } else if (STREQUAL(name, name_len, "letras")
               || STREQUAL(name, name_len, "letrasmus")
               || STREQUAL(name, name_len, "letras.mus.br")) {
        type = NCM_LYRICS_FETCHER_LETRASMUS;
    } else if (STREQUAL(name, name_len, "lacoccinelle")
               || STREQUAL(name, name_len, "lacoccinelle.net")) {
        type = NCM_LYRICS_FETCHER_LACOCCINELLE;
    } else if (STREQUAL(name, name_len, "musica")
               || STREQUAL(name, name_len, "musica.com")) {
        type = NCM_LYRICS_FETCHER_MUSICA;
    } else if (STREQUAL(name, name_len, "paroles")
               || STREQUAL(name, name_len, "paroles.net")) {
        type = NCM_LYRICS_FETCHER_PAROLES;
    } else if (STREQUAL(name, name_len, "musixmatch")
               || STREQUAL(name, name_len, "musixmatch.com")) {
        type = NCM_LYRICS_FETCHER_MUSIXMATCH;
    } else if (STREQUAL(name, name_len, "tekstowo")
               || STREQUAL(name, name_len, "tekstowo.pl")) {
        type = NCM_LYRICS_FETCHER_TEKSTOWO;
    } else if (STREQUAL(name, name_len, "vagalume")
               || STREQUAL(name, name_len, "vagalume.com.br")) {
        type = NCM_LYRICS_FETCHER_VAGALUME;
    } else if (STREQUAL(name, name_len, "internet")
               || STREQUAL(name, name_len, "the Internet")) {
        type = NCM_LYRICS_FETCHER_INTERNET;
    }
    if (type == NCM_LYRICS_FETCHER_UNKNOWN) {
        return -NCM_ERROR_NOT_FOUND;
    }

    profile = lyrics_provider_profile(type);
    if ((profile == NULL) || (profile->name == NULL)) {
        display_name = "";
        display_name_len = 0;
    } else {
        display_name = profile->name;
        display_name_len = profile->name_len;
    }

    ncm_lyrics_fetcher_def_destroy(fetcher);
    *fetcher = (NcmLyricsFetcherDef){0};
    fetcher->type = type;
    fetcher->enabled = true;
    lyrics_string_set(&fetcher->name, &fetcher->name_len,
                      &fetcher->name_cap, display_name, display_name_len);
    return 0;
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

int32
ncm_lyrics_fetcher_registry_append_name(NcmLyricsFetcherRegistry *registry,
                                        char *name, int32 name_len) {
    NcmLyricsFetcherDef *fetcher;
    int32 status;

    if ((fetcher = ncm_lyrics_fetcher_registry_append(registry)) == NULL) {
        return -EINVAL;
    }
    status = ncm_lyrics_fetcher_def_set_name(fetcher, name, name_len);
    if (status < 0) {
        registry->fetchers.len -= 1;
        ncm_lyrics_fetcher_def_destroy(fetcher);
        return status;
    }
    return 0;
}

static bool
lyrics_url_is_collected(StrBuilderArray *urls, char *url, int32 url_len) {
    ASSERT(urls != NULL);
    if ((url == NULL) || (url_len <= 0)) {
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


int32
ncm_lyrics_fetcher_build_url(NcmLyricsFetcherDef *fetcher, StrBuilder *url,
                             char *artist, int32 artist_len, char *title,
                             int32 title_len) {
    char *domain;
    int32 domain_len;

    if ((fetcher == NULL) || (url == NULL) || (artist == NULL)
        || (artist_len <= 0) || (title == NULL) || (title_len <= 0)) {
        return -EINVAL;
    }
    if (!fetcher->enabled) {
        return -NCM_ERROR_INVALID_STATE;
    }

    sb_clear(url);
    SB_APPEND(url,
              "https://www.google.com/search?hl=en&q=");
    if (fetcher->type == NCM_LYRICS_FETCHER_INTERNET) {
        SB_APPEND(url, "lyrics+");
    } else {
        domain = lyrics_type_domain(fetcher->type, &domain_len);
        if (domain_len <= 0) {
            sb_clear(url);
            return -NCM_ERROR_UNAVAILABLE;
        }
        lyrics_append_query(url, STRLIT("site:"));
        lyrics_append_query(url, domain, domain_len);
        sb_append_byte(url, '+');
    }
    lyrics_append_query(url, artist, artist_len);
    sb_append_byte(url, '+');
    lyrics_append_query(url, title, title_len);
    if (fetcher->type != NCM_LYRICS_FETCHER_INTERNET) {
        SB_APPEND(url, "+lyrics");
    }
    return 0;
}

void
ncm_lyrics_cleanup_html(StrBuilder *out, char *data, int32 data_len) {
    StrBuilder unescaped = {0};
    StrBuilder stripped = {0};

    sb_clear(out);
    unescaped = ncm_html_unescape_utf8(data, data_len);
    stripped = ncm_html_strip_tags(unescaped.data, unescaped.len);
    lyrics_append_clean_lines(out, stripped.data, stripped.len);
    sb_free(&stripped);
    sb_free(&unescaped);
    return;
}

static void
lyrics_fetcher_array_destroy_item(void *item) {
    ASSERT(item != NULL);
    ncm_lyrics_fetcher_def_destroy(item);
    return;
}

static LyricsProviderProfile *
lyrics_provider_profile(enum NcmLyricsFetcherType type) {
    if ((type <= NCM_LYRICS_FETCHER_UNKNOWN)
        || (type >= NCM_LYRICS_FETCHER_LAST)) {
        return NULL;
    }
    return &lyrics_provider_profiles[type];
}

static bool
lyrics_provider_has_flag(enum NcmLyricsFetcherType type, uint32 flag) {
    LyricsProviderProfile *profile;

    if ((profile = lyrics_provider_profile(type)) == NULL) {
        return false;
    }
    return (profile->flags & flag) != 0;
}

static char *
lyrics_type_domain(enum NcmLyricsFetcherType type, int32 *len) {
    LyricsProviderProfile *profile;

    if (((profile = lyrics_provider_profile(type)) == NULL)
        || (profile->domain == NULL)) {
        *len = 0;
        return "";
    }
    *len = profile->domain_len;
    return profile->domain;
}

static LyricsSlugProfile
lyrics_slug_profile(enum NcmLyricsFetcherType type) {
    LyricsProviderProfile *profile;

    if ((profile = lyrics_provider_profile(type)) == NULL) {
        return LYRICS_SLUG_PROFILE_NONE;
    }
    return profile->slug_profile;
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

static bool
lyrics_slug_rune_should_be_skipped(uint32 rune) {
    return (rune == '\'') || (rune == '`') || (rune == 0x2018)
           || (rune == 0x2019) || (rune == 0x02bc);
}

static int32
lyrics_append_slug_profile(StrBuilder *buffer, LyricsSlugProfile profile,
                           char *string, int32 string_len) {
    bool compact;
    bool folded_profile;
    bool pending_separator;
    bool wrote;
    char separator;

    if ((buffer == NULL) || (string == NULL) || (string_len <= 0)) {
        return -EINVAL;
    }
    if (profile == LYRICS_SLUG_PROFILE_NONE) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    compact = false;
    folded_profile = false;
    separator = '\0';
    switch (profile) {
    case LYRICS_SLUG_PROFILE_COMPACT_FOLDED:
        compact = true;
        folded_profile = true;
        break;
    case LYRICS_SLUG_PROFILE_HYPHEN_FOLDED:
        folded_profile = true;
        separator = '-';
        break;
    case LYRICS_SLUG_PROFILE_UNDERSCORE_FOLDED:
        folded_profile = true;
        separator = '_';
        break;
    case LYRICS_SLUG_PROFILE_COMPACT_PERCENT:
        compact = true;
        break;
    case LYRICS_SLUG_PROFILE_HYPHEN_PERCENT:
        separator = '-';
        break;
    case LYRICS_SLUG_PROFILE_UNDERSCORE_PERCENT:
        separator = '_';
        break;
    case LYRICS_SLUG_PROFILE_NONE:
    default:
        break;
    }
    pending_separator = false;
    wrote = false;
    for (int32 i = 0; i < string_len; i += 1) {
        uint8 byte = (uint8)string[i];
        uint32 rune;
        int32 rune_len;
        char folded[2];
        int32 folded_len = 0;

        if (lyrics_ascii_alnum(string[i])) {
            if (pending_separator && wrote && !compact) {
                sb_append_byte(buffer, separator);
            }
            sb_append_byte(buffer, lyrics_ascii_lower(string[i]));
            pending_separator = false;
            wrote = true;
            continue;
        }
        if (byte < 0x80) {
            if (lyrics_slug_rune_should_be_skipped((uint32)string[i])) {
                continue;
            }
            if (wrote && !compact) {
                pending_separator = true;
            }
            continue;
        }

        rune_len = utf8_decode(string + i, string_len - i, &rune);
        if (rune_len <= 0) {
            return -NCM_ERROR_PARSE;
        }
        if (lyrics_slug_rune_should_be_skipped(rune)) {
            i += rune_len - 1;
            continue;
        }
        if ((rune == 0x00a0) || (rune == 0x1680)
            || ((rune >= 0x2000) && (rune <= 0x200a))
            || (rune == 0x202f) || (rune == 0x205f)
            || (rune == 0x3000)
            || ((rune >= 0x2010) && (rune <= 0x2015))
            || (rune == 0x2212)) {
            if (wrote && !compact) {
                pending_separator = true;
            }
            i += rune_len - 1;
            continue;
        }
        if (pending_separator && wrote && !compact) {
            sb_append_byte(buffer, separator);
        }
        {
            bool has_folded_form;

            has_folded_form = false;
            if (folded_profile) {
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
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x00c6:
                case 0x00e6:
                    folded[0] = 'a';
                    folded[1] = 'e';
                    folded_len = 2;
                    has_folded_form = true;
                    break;
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
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x00d0:
                case 0x00f0:
                case 0x010e:
                case 0x010f:
                case 0x0110:
                case 0x0111:
                    folded[0] = 'd';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
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
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x011c:
                case 0x011d:
                case 0x011e:
                case 0x011f:
                case 0x0120:
                case 0x0121:
                case 0x0122:
                case 0x0123:
                    folded[0] = 'g';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x0124:
                case 0x0125:
                case 0x0126:
                case 0x0127:
                    folded[0] = 'h';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
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
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x0134:
                case 0x0135:
                    folded[0] = 'j';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x0136:
                case 0x0137:
                    folded[0] = 'k';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
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
                    folded_len = 1;
                    has_folded_form = true;
                    break;
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
                    folded_len = 1;
                    has_folded_form = true;
                    break;
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
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x0152:
                case 0x0153:
                    folded[0] = 'o';
                    folded[1] = 'e';
                    folded_len = 2;
                    has_folded_form = true;
                    break;
                case 0x0154:
                case 0x0155:
                case 0x0156:
                case 0x0157:
                case 0x0158:
                case 0x0159:
                    folded[0] = 'r';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x015a:
                case 0x015b:
                case 0x015c:
                case 0x015d:
                case 0x015e:
                case 0x015f:
                case 0x0160:
                case 0x0161:
                    folded[0] = 's';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x00df:
                    folded[0] = 's';
                    folded[1] = 's';
                    folded_len = 2;
                    has_folded_form = true;
                    break;
                case 0x00de:
                case 0x00fe:
                    folded[0] = 't';
                    folded[1] = 'h';
                    folded_len = 2;
                    has_folded_form = true;
                    break;
                case 0x0162:
                case 0x0163:
                case 0x0164:
                case 0x0165:
                case 0x0166:
                case 0x0167:
                    folded[0] = 't';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
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
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x0174:
                case 0x0175:
                    folded[0] = 'w';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x00dd:
                case 0x00fd:
                case 0x00ff:
                case 0x0176:
                case 0x0177:
                case 0x0178:
                    folded[0] = 'y';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                case 0x0179:
                case 0x017a:
                case 0x017b:
                case 0x017c:
                case 0x017d:
                case 0x017e:
                    folded[0] = 'z';
                    folded_len = 1;
                    has_folded_form = true;
                    break;
                default:
                    break;
                }
            }
            if (has_folded_form) {
                SB_APPEND(buffer, folded, folded_len);
            } else {
                for (int32 j = 0; j < rune_len; j += 1) {
                    lyrics_append_percent_byte(buffer, (uint8)string[i + j]);
                }
            }
        }
        pending_separator = false;
        wrote = true;
        i += rune_len - 1;
    }
    if (!wrote) {
        return -NCM_ERROR_NOT_FOUND;
    }
    return 0;
}

static int32
lyrics_append_slug(StrBuilder *buffer, enum NcmLyricsFetcherType type,
                   char *string, int32 string_len) {
    return lyrics_append_slug_profile(buffer, lyrics_slug_profile(type),
                                      string, string_len);
}

static void
lyrics_append_query(StrBuilder *buffer, char *string, int32 string_len) {
    for (int32 i = 0; i < string_len; i += 1) {
        uint8 byte = (uint8)string[i];

        if (lyrics_ascii_alnum(string[i]) || (string[i] == '-')
            || (string[i] == '.') || (string[i] == '_')
            || (string[i] == '~')) {
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
lyrics_starts_with_ignore_case(char *string, int32 string_len, char *prefix,
                               int32 prefix_len) {
    if ((string == NULL) || (prefix == NULL) || (prefix_len < 0)
        || (string_len < prefix_len)) {
        return false;
    }
    for (int32 i = 0; i < prefix_len; i += 1) {
        if (lyrics_ascii_lower(string[i]) != lyrics_ascii_lower(prefix[i])) {
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
            int32 high = lyrics_hex_value(data[i + 1]);
            int32 low = lyrics_hex_value(data[i + 2]);

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
lyrics_find_tag_end(char *data, int32 data_len, int32 start) {
    char quote = '\0';

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
lyrics_extract_divs(StrBuilder *out, char *data, int32 data_len, char *marker,
                    int32 marker_len, bool append_all) {
    int32 pos;
    int32 status;
    bool found;

    sb_clear(out);
    pos = 0;
    status = -NCM_ERROR_NOT_FOUND;
    found = false;
    while (pos < data_len) {
        int32 open;
        int32 open_end;
        int32 close;
        int32 close_end;

        {
            char *match = memmem64(data + pos, data_len - pos, STRLIT("<div"));

            if (match == NULL) {
                break;
            }
            open = (int32)(match - data);
        }
        if ((open_end = lyrics_find_tag_end(data, data_len,
                                            open + STRLIT_LEN("<div"))) < 0) {
            status = -NCM_ERROR_PARSE;
            break;
        }
        if (memmem64(data + open, open_end - open + 1,
                     marker, marker_len) == NULL) {
            pos = open_end + 1;
            continue;
        }

        {
            int32 depth = 1;
            int32 div_pos = open_end + 1;

            close = -1;
            close_end = -1;
            while (div_pos < data_len) {
                char *open_match;
                char *close_match;
                int32 nested_open;
                int32 nested_close;

                open_match = memmem64(data + div_pos, data_len - div_pos,
                                      STRLIT("<div"));
                close_match = memmem64(data + div_pos, data_len - div_pos,
                                       STRLIT("</div"));
                if (open_match == NULL) {
                    nested_open = -1;
                } else {
                    nested_open = (int32)(open_match - data);
                }
                if (close_match == NULL) {
                    nested_close = -1;
                } else {
                    nested_close = (int32)(close_match - data);
                }

                if (nested_close < 0) {
                    break;
                }
                if ((nested_open >= 0) && (nested_open < nested_close)) {
                    int32 tag_end;

                    tag_end = lyrics_find_tag_end(
                        data, data_len, nested_open + STRLIT_LEN("<div"));
                    if (tag_end < 0) {
                        break;
                    }
                    depth += 1;
                    div_pos = tag_end + 1;
                } else {
                    int32 tag_end;

                    tag_end = lyrics_find_tag_end(
                        data, data_len, nested_close + STRLIT_LEN("</div"));
                    if (tag_end < 0) {
                        break;
                    }
                    depth -= 1;
                    if (depth == 0) {
                        close = nested_close;
                        close_end = tag_end + 1;
                        break;
                    }
                    div_pos = tag_end + 1;
                }
            }
        }
        if (close < 0) {
            status = -NCM_ERROR_PARSE;
            break;
        }

        if (found) {
            SB_APPEND(out, "\n\n");
        }
        SB_APPEND(out, data + open_end + 1, close - open_end - 1);
        found = true;
        pos = close_end;
        if (!append_all) {
            break;
        }
    }
    if (found) {
        return 0;
    }
    return status;
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

static int32
lyrics_url_path_start(char *url, int32 url_len) {
    int32 scheme;
    int32 path_start;

    if ((scheme = lyrics_find_ignore_case(url, url_len,
                                          STRLIT("://"), 0)) < 0) {
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
    int32 path_end = path_start;

    while ((path_end < url_len) && (url[path_end] != '?')
           && (url[path_end] != '#')) {
        path_end += 1;
    }
    return path_end;
}

static bool
lyrics_url_path_starts_with(char *url, int32 url_len, int32 path_start,
                            char *prefix, int32 prefix_len) {
    int32 path_end = lyrics_url_path_end(url, url_len, path_start);
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
lyrics_slug_match_separator(char ch) {
    return (ch == '-') || (ch == '_');
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
lyrics_url_best_slug_score(NcmLyricsFetcherDef *fetcher,
                           char *url, int32 url_len, StrBuilder *wanted) {
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
            StrBuilder decoded = {0};
            StrBuilder slug = {0};
            char *segment;
            int32 segment_len;
            int32 score;

            segment = url + segment_start;
            segment_len = i - segment_start;
            lyrics_trim_url_segment_suffix(&segment, &segment_len,
                                           STRLIT(".html"));
            lyrics_trim_url_segment_suffix(&segment, &segment_len,
                                           STRLIT("-lyrics"));
            score = 0;
            if (segment_len > 0) {
                lyrics_percent_decode(&decoded, segment, segment_len);
                if (lyrics_append_slug(&slug, fetcher->type, decoded.data,
                                       decoded.len) >= 0) {
                    char *candid = slug.data;
                    char *wanted_data = wanted->data;
                    int32 candid_len = slug.len;
                    int32 wanted_len = wanted->len;

                    if ((wanted_data != NULL) && (candid != NULL)
                        && (wanted_len > 0) && (candid_len > 0)) {
                        if (STREQUAL(wanted_data, wanted_len,
                                     candid, candid_len)) {
                            score = 50;
                        } else if (candid_len > wanted_len) {
                            char *match;

                            match = memmem64(candid, candid_len,
                                             wanted_data, wanted_len);
                            if (lyrics_starts_with_ignore_case(
                                    candid, candid_len,
                                    wanted_data, wanted_len)
                                && lyrics_slug_match_separator(
                                    candid[wanted_len])) {
                                score = 40;
                            } else if (lyrics_starts_with_ignore_case(
                                           candid + candid_len - wanted_len,
                                           wanted_len,
                                           wanted_data, wanted_len)
                                       && lyrics_slug_match_separator(
                                           candid[candid_len
                                                  - wanted_len - 1])) {
                                score = 35;
                            } else if ((match != NULL) && (match > candid)) {
                                int32 match_pos;

                                match_pos = (int32)(match - candid);
                                if (lyrics_slug_match_separator(
                                        candid[match_pos - 1])
                                    && (match_pos + wanted_len < candid_len)
                                    && lyrics_slug_match_separator(
                                        candid[match_pos + wanted_len])) {
                                    score = 30;
                                }
                            }
                        } else if ((wanted_len > candid_len)
                                   && lyrics_starts_with_ignore_case(
                                       wanted_data, wanted_len,
                                       candid, candid_len)
                                   && lyrics_slug_match_separator(
                                       wanted_data[candid_len])) {
                            score = 20;
                        }
                    }
                }
            }
            sb_free(&slug);
            sb_free(&decoded);

            if (score > best) {
                best = score;
            }
            segment_start = -1;
        }
    }
    return best;
}


static int32
lyrics_parse_hex4(char *data, int32 data_len, int32 start, uint32 *value) {
    uint32 result;

    if ((data == NULL) || (data_len < 0) || (value == NULL) || (start < 0)) {
        return -EINVAL;
    }
    if ((data_len - start) < 4) {
        return -NCM_ERROR_PARSE;
    }

    result = 0;
    for (int32 i = 0; i < 4; i += 1) {
        int32 digit;

        digit = lyrics_hex_value(data[start + i]);
        if (digit < 0) {
            return -NCM_ERROR_PARSE;
        }
        result = (result << 4) | (uint32)digit;
    }
    *value = result;
    return 0;
}

static int32
lyrics_decode_quoted(StrBuilder *out, char *data, int32 data_len,
                     int32 start, char quote, int32 *end) {
    int32 i;

    if ((out == NULL) || (data == NULL) || (data_len < 0)
        || (start < 0) || (end == NULL)) {
        return -EINVAL;
    }

    sb_clear(out);
    i = start;
    while (i < data_len) {
        char escaped;

        if (data[i] == quote) {
            *end = i + 1;
            return 0;
        }
        if (data[i] != '\\') {
            sb_append_byte(out, data[i]);
            i += 1;
            continue;
        }

        i += 1;
        if (i >= data_len) {
            return -NCM_ERROR_PARSE;
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

            if (lyrics_parse_hex4(data, data_len, i, &first) < 0) {
                return -NCM_ERROR_PARSE;
            }
            i += 4;
            rune = first;
            if ((first >= 0xd800) && (first <= 0xdbff)
                && (i + 6 <= data_len) && (data[i] == '\\')
                && (data[i + 1] == 'u')) {
                uint32 second;

                if ((lyrics_parse_hex4(data, data_len, i + 2, &second) == 0)
                    && (second >= 0xdc00) && (second <= 0xdfff)) {
                    rune = 0x10000 + ((first - 0xd800) << 10)
                           + (second - 0xdc00);
                    i += 6;
                }
            }
            {
                char encoded[4];
                int32 encoded_len;

                encoded_len = utf8_encode(rune, encoded, SIZEOF(encoded));
                if (encoded_len > 0) {
                    SB_APPEND(out, encoded, encoded_len);
                }
            }
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
    return -NCM_ERROR_PARSE;
}

static int32
lyrics_json_value_start(char *data, int32 data_len, char *key,
                        int32 key_len, int32 start, int32 *value_start) {
    StrBuilder pattern = {0};
    char *match;
    int32 key_pos;
    int32 pos;

    if ((data == NULL) || (key == NULL) || (value_start == NULL)) {
        return -EINVAL;
    }
    if ((data_len < 0) || (key_len <= 0) || (start < 0)) {
        return -EINVAL;
    }
    if ((data_len == 0) || (start >= data_len)) {
        return -NCM_ERROR_NOT_FOUND;
    }

    sb_append_byte(&pattern, '"');
    SB_APPEND(&pattern, key, key_len);
    sb_append_byte(&pattern, '"');
    match = memmem64(data + start, data_len - start,
                     pattern.data, pattern.len);
    sb_free(&pattern);
    if (match == NULL) {
        return -NCM_ERROR_NOT_FOUND;
    }
    key_pos = (int32)(match - data);

    pos = key_pos + key_len + 2;
    while ((pos < data_len)
           && ((data[pos] == ' ') || (data[pos] == '\t')
               || (data[pos] == '\r') || (data[pos] == '\n'))) {
        pos += 1;
    }
    if ((pos >= data_len) || (data[pos] != ':')) {
        return -NCM_ERROR_PARSE;
    }
    pos += 1;
    while ((pos < data_len)
           && ((data[pos] == ' ') || (data[pos] == '\t')
               || (data[pos] == '\r') || (data[pos] == '\n'))) {
        pos += 1;
    }
    if ((pos >= data_len) || (data[pos] != '"')) {
        return -NCM_ERROR_PARSE;
    }
    *value_start = pos + 1;
    return 0;
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
lyrics_status_error(int32 status, int32 *message_len) {
    if (status == -ETIMEDOUT) {
        *message_len = STRLIT_LEN(LYRICS_MSG_TIMED_OUT);
        return LYRICS_MSG_TIMED_OUT;
    }
    *message_len = STRLIT_LEN(LYRICS_MSG_DOWNLOAD_FAILED);
    return LYRICS_MSG_DOWNLOAD_FAILED;
}

static int32
lyrics_fetch_page(NcmLyricsFetcherDef *fetcher, NcmLyricsResult *result,
                  StrBuilder *url, char *referer, int32 referer_len,
                  bool *retry) {
    StrBuilder data = {0};
    StrBuilder lyrics = {0};
    StrBuilder cleaned = {0};
    char *message;
    int32 message_len;
    int32 status;
    bool plain_text;

    *retry = false;
    status = lyrics_curl_perform(&data, url->data, url->len, referer,
                                 referer_len, true, 15);
    if (status < 0) {
        message = lyrics_status_error(status, &message_len);
        (void)ncm_lyrics_result_set(result, false, message, message_len);
        *retry = status != -ETIMEDOUT;
        goto cleanup;
    }

    if ((fetcher->type == NCM_LYRICS_FETCHER_AZLYRICS)
        && memmem64(data.data, data.len, STRLIT("request for access"))) {
        status = ncm_lyrics_result_set(result, false,
                                       STRLIT(LYRICS_MSG_ACCESS_DENIED));
        goto cleanup;
    }

    {
        StrBuilder *out = &lyrics;
        char *content_data = data.data;
        bool *plain_text_out = &plain_text;
        int32 content_len = data.len;

        int32 extract_status;

        *plain_text_out = false;
        extract_status = -NCM_ERROR_UNAVAILABLE;
        switch (fetcher->type) {
        case NCM_LYRICS_FETCHER_AMALGAMA: {
            char *match;
            int32 marker;
            int32 start;
            int32 end;
            int32 first_status;
            int32 second_status;

            first_status = lyrics_extract_divs(
                out, content_data, content_len,
                STRLIT("class=\"original\""), true);
            if (first_status == 0) {
                extract_status = 0;
                break;
            }
            second_status = lyrics_extract_divs(
                out, content_data, content_len,
                STRLIT("class='original'"), true);
            if (second_status == 0) {
                extract_status = 0;
                break;
            }

            sb_clear(out);
            marker = lyrics_find_ignore_case(
                content_data, content_len, STRLIT("(оригинал"), 0);
            if (marker < 0) {
                marker = lyrics_find_ignore_case(
                    content_data, content_len, STRLIT("original"), 0);
            }
            start = -1;
            if (marker >= 0) {
                start = lyrics_find_ignore_case(
                    content_data, content_len, STRLIT("</h2>"), marker);
                if (start >= 0) {
                    start += STRLIT_LEN("</h2>");
                } else {
                    match = memchr64(content_data + marker, '>',
                                     content_len - marker);
                    if (match != NULL) {
                        start = (int32)(match - content_data) + 1;
                    }
                }
            }
            if (start < 0) {
                if (first_status == -NCM_ERROR_PARSE) {
                    extract_status = first_status;
                } else if (second_status == -NCM_ERROR_PARSE) {
                    extract_status = second_status;
                } else {
                    extract_status = -NCM_ERROR_NOT_FOUND;
                }
                break;
            }

            end = -1;
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT(
                                          "Понравился "
                                          "перевод"),
                                      &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Добавить видео"),
                                      &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Другие песни"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Комментарии"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("<footer"), &end);
            if ((end < 0) || (end <= start)) {
                extract_status = -NCM_ERROR_PARSE;
                break;
            }

            SB_APPEND(out, content_data + start, end - start);
            extract_status = 0;
            break;
        }
        case NCM_LYRICS_FETCHER_AZLYRICS: {
            char *match;
            int32 start;
            int32 end;

            sb_clear(out);
            match = memmem64(content_data, content_len,
                             STRLIT("Usage of azlyrics.com"));
            if (match == NULL) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }
            start = (int32)(match - content_data)
                    + STRLIT_LEN("Usage of azlyrics.com");
            match = memmem64(content_data + start, content_len - start,
                             STRLIT("-->"));
            if (match == NULL) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }
            start = (int32)(match - content_data) + STRLIT_LEN("-->");
            match = memmem64(content_data + start, content_len - start,
                             STRLIT("</div>"));
            if (match == NULL) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }
            end = (int32)(match - content_data);
            SB_APPEND(out, content_data + start, end - start);
            extract_status = 0;
            break;
        }
        case NCM_LYRICS_FETCHER_GENIUS: {
            StrBuilder json = {0};
            StrBuilder html = {0};
            char *match;
            int32 marker;
            int32 json_end;
            int32 lyrics_data;
            int32 value_start;
            int32 value_end;
            int32 fallback_status;

            match = memmem64(
                content_data, content_len,
                STRLIT("window.__PRELOADED_STATE__ = JSON.parse('"));
            if (match == NULL) {
                marker = -1;
            } else {
                marker = (int32)(match - content_data);
            }
            extract_status = -NCM_ERROR_NOT_FOUND;
            if (marker >= 0) {
                marker += STRLIT_LEN(
                    "window.__PRELOADED_STATE__ = JSON.parse('");
                extract_status = lyrics_decode_quoted(
                    &json, content_data, content_len, marker, '\'', &json_end);
                if (extract_status == 0) {
                    match = memmem64(json.data, json.len,
                                     STRLIT("\"lyricsData\""));
                    if (match == NULL) {
                        lyrics_data = -1;
                    } else {
                        lyrics_data = (int32)(match - json.data);
                    }
                    if (lyrics_data < 0) {
                        extract_status = -NCM_ERROR_NOT_FOUND;
                    } else {
                        extract_status = lyrics_json_value_start(
                            json.data, json.len, STRLIT("html"), lyrics_data,
                            &value_start);
                        if (extract_status == 0) {
                            extract_status = lyrics_decode_quoted(
                                &html, json.data, json.len, value_start, '"',
                                &value_end);
                            if (extract_status == 0) {
                                sb_clear(out);
                                SB_APPEND(out, html.data, html.len);
                                if (out->len <= 0) {
                                    extract_status = -NCM_ERROR_NOT_FOUND;
                                }
                            }
                        }
                    }
                }
            }

            sb_free(&html);
            sb_free(&json);
            if (extract_status == 0) {
                break;
            }

            fallback_status = lyrics_extract_divs(
                out, content_data, content_len,
                STRLIT("data-lyrics-container=\"true\""), true);
            if (fallback_status == 0) {
                extract_status = 0;
            } else if (extract_status != -NCM_ERROR_PARSE) {
                extract_status = fallback_status;
            }
            break;
        }
        case NCM_LYRICS_FETCHER_LETRASMUS:
            extract_status = lyrics_extract_divs(
                out, content_data, content_len,
                STRLIT("class=\"lyric-original\""), false);
            break;
        case NCM_LYRICS_FETCHER_LACOCCINELLE: {
            char *match;
            int32 marker;
            int32 second_marker;
            int32 start;
            int32 end;

            sb_clear(out);
            marker = lyrics_find_ignore_case(
                content_data, content_len,
                STRLIT("Paroles et traduction de la chanson"), 0);
            if (marker < 0) {
                marker = lyrics_find_ignore_case(
                    content_data, content_len,
                    STRLIT("Paroles de la chanson"), 0);
            }
            if (marker < 0) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }

            second_marker = lyrics_find_ignore_case(
                content_data, content_len,
                STRLIT("Paroles et traduction de la chanson"),
                marker + STRLIT_LEN("Paroles et traduction de la chanson"));
            if (second_marker >= 0) {
                marker = second_marker;
            }

            match = memchr64(content_data + marker, '>', content_len - marker);
            if (match == NULL) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }
            start = (int32)(match - content_data) + 1;

            end = -1;
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Publié par"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Chanteurs :"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Albums :"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Vos commentaires"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Commentaires"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Téléchargement"), &end);
            if ((end < 0) || (end <= start)) {
                extract_status = -NCM_ERROR_PARSE;
                break;
            }

            SB_APPEND(out, content_data + start, end - start);
            extract_status = 0;
            break;
        }
        case NCM_LYRICS_FETCHER_MUSICA: {
            char *match;
            int32 marker;
            int32 start;
            int32 end;

            sb_clear(out);
            marker = lyrics_find_ignore_case(
                content_data, content_len, STRLIT(">LETRA<"), 0);
            if (marker < 0) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }

            start = marker + STRLIT_LEN(">LETRA<") - 1;
            match = memchr64(content_data + start, '>', content_len - start);
            if (match == NULL) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }
            start = (int32)(match - content_data) + 1;

            end = -1;
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Significado de la letra"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Puntuar "), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Imprimir letra"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Letra añadida"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("+ Letras de"), &end);
            if ((end < 0) || (end <= start)) {
                extract_status = -NCM_ERROR_PARSE;
                break;
            }

            SB_APPEND(out, content_data + start, end - start);
            extract_status = 0;
            break;
        }
        case NCM_LYRICS_FETCHER_PAROLES: {
            char *match;
            int32 marker;
            int32 second_marker;
            int32 start;
            int32 end;

            sb_clear(out);
            marker = lyrics_find_ignore_case(
                content_data, content_len, STRLIT("Paroles de la chanson"), 0);
            if (marker < 0) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }

            second_marker = lyrics_find_ignore_case(
                content_data, content_len, STRLIT("Paroles de la chanson"),
                marker + STRLIT_LEN("Paroles de la chanson"));
            if (second_marker >= 0) {
                marker = second_marker;
            }

            match = memchr64(content_data + marker, '>', content_len - marker);
            if (match == NULL) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }
            start = (int32)(match - content_data) + 1;

            end = -1;
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Paroles.net dispose"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Sélection du moment"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Les plus grands succès"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("On aime"), &end);
            lyrics_update_first_match(content_data, content_len, start,
                                      STRLIT("Top traduction"), &end);
            if ((end < 0) || (end <= start)) {
                extract_status = -NCM_ERROR_PARSE;
                break;
            }

            SB_APPEND(out, content_data + start, end - start);
            extract_status = 0;
            break;
        }
        case NCM_LYRICS_FETCHER_MUSIXMATCH: {
            char *match;
            int32 track_info;
            int32 lyrics_pos;
            int32 value_start;
            int32 value_end;

            *plain_text_out = true;
            match = memmem64(
                content_data, content_len, STRLIT("\"trackInfo\""));
            if (match == NULL) {
                track_info = 0;
            } else {
                track_info = (int32)(match - content_data);
            }

            match = memmem64(content_data + track_info,
                             content_len - track_info, STRLIT("\"lyrics\""));
            if (match == NULL) {
                extract_status = -NCM_ERROR_NOT_FOUND;
                break;
            }
            lyrics_pos = (int32)(match - content_data);

            extract_status = lyrics_json_value_start(
                content_data, content_len, STRLIT("body"), lyrics_pos,
                &value_start);
            if (extract_status == 0) {
                extract_status = lyrics_decode_quoted(
                    out, content_data, content_len, value_start, '"',
                    &value_end);
            }
            break;
        }
        case NCM_LYRICS_FETCHER_TEKSTOWO:
            extract_status = lyrics_extract_divs(
                out, content_data, content_len,
                STRLIT("class=\"inner-text\""), false);
            break;
        case NCM_LYRICS_FETCHER_VAGALUME: {
            int32 fallback_status;

            extract_status = lyrics_extract_divs(
                out, content_data, content_len, STRLIT("id=\"lyrics\""), false);
            if (extract_status == 0) {
                break;
            }
            fallback_status = lyrics_extract_divs(
                out, content_data, content_len, STRLIT("id=lyrics"), false);
            if (fallback_status == 0) {
                extract_status = 0;
            } else if (fallback_status == -NCM_ERROR_PARSE) {
                extract_status = fallback_status;
            }
            break;
        }
        case NCM_LYRICS_FETCHER_UNKNOWN:
        case NCM_LYRICS_FETCHER_INTERNET:
        case NCM_LYRICS_FETCHER_LAST:
        default:
            sb_clear(out);
            extract_status = -NCM_ERROR_UNAVAILABLE;
            break;
        }
        status = extract_status;
    }
    if ((status < 0) || (lyrics.len <= 0)) {
        status = ncm_lyrics_result_set(result, false,
                                       STRLIT(LYRICS_MSG_NOT_FOUND));
        *retry = true;
        goto cleanup;
    }

    if (plain_text) {
        lyrics_append_clean_lines(&cleaned, lyrics.data, lyrics.len);
    } else {
        ncm_lyrics_cleanup_html(&cleaned, lyrics.data, lyrics.len);
    }
    if (cleaned.len <= 0) {
        status = ncm_lyrics_result_set(result, false,
                                       STRLIT(LYRICS_MSG_NOT_FOUND));
        *retry = true;
        goto cleanup;
    }
    status = ncm_lyrics_result_set(result, true, cleaned.data, cleaned.len);

cleanup:
    sb_free(&cleaned);
    sb_free(&lyrics);
    sb_free(&data);
    return status;
}



int32
ncm_lyrics_fetcher_fetch(NcmLyricsFetcherDef *fetcher, NcmLyricsResult *result,
                         char *artist, int32 artist_len, char *title,
                         int32 title_len) {
    StrBuilderArray direct_urls = {0};
    int32 status;
    bool retry;

    if ((fetcher == NULL) || (result == NULL)) {
        return -EINVAL;
    }

    ncm_lyrics_result_clear(result);
    if (!fetcher->enabled
        || (fetcher->type <= NCM_LYRICS_FETCHER_UNKNOWN)
        || (fetcher->type >= NCM_LYRICS_FETCHER_LAST)) {
        (void)ncm_lyrics_result_set(result, false,
                                    STRLIT(LYRICS_MSG_INVALID_FETCHER));
        return -EINVAL;
    }
    if (fetcher->type == NCM_LYRICS_FETCHER_INTERNET) {
        StrBuilder url = {0};
        StrBuilder message = {0};
        status = ncm_lyrics_fetcher_build_url(fetcher, &url, artist, artist_len,
                                              title, title_len);
        if (status == 0) {
            SB_APPEND(&message,
                      "The following search may contain lyrics for this "
                      "song: ");
            SB_APPEND(&message, url.data, url.len);
            status = ncm_lyrics_result_set(result, false,
                                           message.data, message.len);
        }
        sb_free(&message);
        sb_free(&url);
        return status;
    }

    retry = false;
    str_builder_array_init(&direct_urls);
    {
        StrBuilderArray *urls = &direct_urls;

        LyricsSlugProfile profile;
        LyricsSlugProfile legacy_profile;
        LyricsDirectSlugPair pairs[4] = {0};
        StrBuilder candidate = {0};
        int32 direct_status;

        direct_status = 0;
        do {
            ASSERT(fetcher != NULL);
            ASSERT(urls != NULL);
            if (!fetcher->enabled) {
                direct_status = -NCM_ERROR_INVALID_STATE;
                break;
            }
            if ((artist == NULL) || (artist_len <= 0)
                || (title == NULL) || (title_len <= 0)) {
                direct_status = -EINVAL;
                break;
            }

            str_builder_array_clear(urls);
            if (!lyrics_provider_has_flag(fetcher->type,
                                          LYRICS_PROVIDER_DIRECT_URLS)) {
                break;
            }

            profile = lyrics_slug_profile(fetcher->type);
            if (profile == LYRICS_SLUG_PROFILE_NONE) {
                direct_status = -NCM_ERROR_UNAVAILABLE;
                break;
            }
            legacy_profile = profile;
            switch (profile) {
            case LYRICS_SLUG_PROFILE_COMPACT_FOLDED:
                legacy_profile = LYRICS_SLUG_PROFILE_COMPACT_PERCENT;
                break;
            case LYRICS_SLUG_PROFILE_HYPHEN_FOLDED:
                legacy_profile = LYRICS_SLUG_PROFILE_HYPHEN_PERCENT;
                break;
            case LYRICS_SLUG_PROFILE_UNDERSCORE_FOLDED:
                legacy_profile = LYRICS_SLUG_PROFILE_UNDERSCORE_PERCENT;
                break;
            case LYRICS_SLUG_PROFILE_COMPACT_PERCENT:
            case LYRICS_SLUG_PROFILE_HYPHEN_PERCENT:
            case LYRICS_SLUG_PROFILE_UNDERSCORE_PERCENT:
            case LYRICS_SLUG_PROFILE_NONE:
            default:
                break;
            }

            pairs[0] = (LyricsDirectSlugPair){profile, profile};
            pairs[1] = (LyricsDirectSlugPair){legacy_profile, legacy_profile};
            pairs[2] = (LyricsDirectSlugPair){profile, legacy_profile};
            pairs[3] = (LyricsDirectSlugPair){legacy_profile, profile};

            for (int32 i = 0; i < LENGTH(pairs); i += 1) {
                StrBuilder *item;

                sb_clear(&candidate);
                switch (fetcher->type) {
                case NCM_LYRICS_FETCHER_AMALGAMA: {
                    StrBuilder artist_slug = {0};
                    StrBuilder title_slug = {0};

                    direct_status = lyrics_append_slug_profile(
                        &artist_slug, pairs[i].artist, artist, artist_len);
                    if (direct_status >= 0) {
                        direct_status = lyrics_append_slug_profile(
                            &title_slug, pairs[i].title, title, title_len);
                    }
                    if (direct_status >= 0) {
                        SB_APPEND(&candidate,
                                  "https://www.amalgama-lab.com/songs/");
                        sb_append_byte(&candidate, artist_slug.data[0]);
                        sb_append_byte(&candidate, '/');
                        SB_APPEND(&candidate,
                                  artist_slug.data, artist_slug.len);
                        sb_append_byte(&candidate, '/');
                        SB_APPEND(&candidate, title_slug.data, title_slug.len);
                        SB_APPEND(&candidate, ".html");
                    }
                    sb_free(&title_slug);
                    sb_free(&artist_slug);
                    break;
                }
                case NCM_LYRICS_FETCHER_AZLYRICS:
                    SB_APPEND(&candidate, "https://www.azlyrics.com/lyrics/");
                    direct_status = lyrics_append_slug_profile(
                        &candidate, pairs[i].artist, artist, artist_len);
                    if (direct_status >= 0) {
                        sb_append_byte(&candidate, '/');
                        direct_status = lyrics_append_slug_profile(
                            &candidate, pairs[i].title, title, title_len);
                    }
                    if (direct_status >= 0) {
                        SB_APPEND(&candidate, ".html");
                    }
                    break;
                case NCM_LYRICS_FETCHER_GENIUS:
                    SB_APPEND(&candidate, "https://genius.com/");
                    direct_status = lyrics_append_slug_profile(
                        &candidate, pairs[i].artist, artist, artist_len);
                    if (direct_status >= 0) {
                        sb_append_byte(&candidate, '-');
                        direct_status = lyrics_append_slug_profile(
                            &candidate, pairs[i].title, title, title_len);
                    }
                    if (direct_status >= 0) {
                        SB_APPEND(&candidate, "-lyrics");
                    }
                    break;
                case NCM_LYRICS_FETCHER_LETRASMUS:
                    SB_APPEND(&candidate, "https://www.letras.mus.br/");
                    direct_status = lyrics_append_slug_profile(
                        &candidate, pairs[i].artist, artist, artist_len);
                    if (direct_status >= 0) {
                        sb_append_byte(&candidate, '/');
                        direct_status = lyrics_append_slug_profile(
                            &candidate, pairs[i].title, title, title_len);
                    }
                    if (direct_status >= 0) {
                        sb_append_byte(&candidate, '/');
                    }
                    break;
                case NCM_LYRICS_FETCHER_LACOCCINELLE:
                case NCM_LYRICS_FETCHER_MUSICA:
                    direct_status = -NCM_ERROR_UNAVAILABLE;
                    break;
                case NCM_LYRICS_FETCHER_PAROLES:
                    SB_APPEND(&candidate, "https://www.paroles.net/");
                    direct_status = lyrics_append_slug_profile(
                        &candidate, pairs[i].artist, artist, artist_len);
                    if (direct_status >= 0) {
                        SB_APPEND(&candidate, "/paroles-");
                        direct_status = lyrics_append_slug_profile(
                            &candidate, pairs[i].title, title, title_len);
                    }
                    break;
                case NCM_LYRICS_FETCHER_MUSIXMATCH:
                    SB_APPEND(&candidate, "https://www.musixmatch.com/lyrics/");
                    direct_status = lyrics_append_slug_profile(
                        &candidate, pairs[i].artist, artist, artist_len);
                    if (direct_status >= 0) {
                        sb_append_byte(&candidate, '/');
                        direct_status = lyrics_append_slug_profile(
                            &candidate, pairs[i].title, title, title_len);
                    }
                    break;
                case NCM_LYRICS_FETCHER_TEKSTOWO:
                    SB_APPEND(&candidate, "https://www.tekstowo.pl/");
                    direct_status = lyrics_append_slug_profile(
                        &candidate, pairs[i].artist, artist, artist_len);
                    if (direct_status >= 0) {
                        sb_append_byte(&candidate, '/');
                        direct_status = lyrics_append_slug_profile(
                            &candidate, pairs[i].title, title, title_len);
                    }
                    break;
                case NCM_LYRICS_FETCHER_VAGALUME:
                    SB_APPEND(&candidate, "https://www.vagalume.com.br/");
                    direct_status = lyrics_append_slug_profile(
                        &candidate, pairs[i].artist, artist, artist_len);
                    if (direct_status >= 0) {
                        sb_append_byte(&candidate, '/');
                        direct_status = lyrics_append_slug_profile(
                            &candidate, pairs[i].title, title, title_len);
                    }
                    if (direct_status >= 0) {
                        SB_APPEND(&candidate, ".html");
                    }
                    break;
                case NCM_LYRICS_FETCHER_UNKNOWN:
                case NCM_LYRICS_FETCHER_INTERNET:
                case NCM_LYRICS_FETCHER_LAST:
                default:
                    direct_status = -NCM_ERROR_UNAVAILABLE;
                    break;
                }

                if (direct_status < 0) {
                    sb_clear(&candidate);
                    break;
                }
                if (lyrics_url_is_collected(
                        urls, candidate.data, candidate.len)) {
                    continue;
                }
                item = str_builder_array_append(urls);
                if (item == NULL) {
                    direct_status = -NCM_ERROR_INVALID_STATE;
                    break;
                }
                SB_APPEND(item, candidate.data, candidate.len);
            }
        } while (false);

        sb_free(&candidate);
        status = direct_status;
    }
    if (status == 0) {
        if (direct_urls.len == 0) {
            retry = true;
        }
        for (int32 i = 0; i < direct_urls.len; i += 1) {
            status = lyrics_fetch_page(fetcher, result, &direct_urls.items[i],
                                       NULL, 0, &retry);
            if (result->success || !retry) {
                break;
            }
        }
    } else {
        retry = true;
    }
    str_builder_array_destroy(&direct_urls);
    if (!result->success && retry
        && lyrics_provider_has_flag(fetcher->type,
                                    LYRICS_PROVIDER_SEARCH_URLS)) {
        StrBuilder search_url = {0};
        StrBuilder data = {0};
        StrBuilderArray page_urls = {0};
        char *message;
        int32 message_len;
        int32 search_status;
        bool search_retry;

        str_builder_array_init(&page_urls);
        search_status = ncm_lyrics_fetcher_build_url(
            fetcher, &search_url, artist, artist_len, title, title_len);
        if (search_status < 0) {
            goto search_cleanup;
        }

        search_status = lyrics_curl_perform(
            &data, search_url.data, search_url.len, NULL, 0, true, 15);
        if (search_status < 0) {
            message = lyrics_status_error(search_status, &message_len);
            (void)ncm_lyrics_result_set(result, false, message, message_len);
            goto search_cleanup;
        }
        {
            StrBuilderArray *out = &page_urls;
            char *search_data = data.data;
            int32 search_data_len = data.len;

            StrBuilder numeric_unescaped = {0};
            StrBuilder unescaped = {0};
            StrBuilder candidate = {0};
            int32 scores[LYRICS_SEARCH_MAX_CANDIDATES] = {0};
            int32 pos;
            int32 collect_status;

            numeric_unescaped = ncm_html_unescape_utf8(
                search_data, search_data_len);
            unescaped = ncm_html_unescape_entities(numeric_unescaped.data,
                                                   numeric_unescaped.len);
            sb_free(&numeric_unescaped);
            str_builder_array_clear(out);
            pos = 0;
            collect_status = 0;
            while (pos < unescaped.len) {
                int32 href;
                int32 value_start;
                int32 value_end;
                char quote;

                if ((href = lyrics_find_ignore_case(
                         unescaped.data, unescaped.len, STRLIT("href"), pos))
                    < 0) {
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
                    char *match;

                    value_start += 1;
                    match = memchr64(unescaped.data + value_start, quote,
                                     unescaped.len - value_start);
                    if (match == NULL) {
                        value_end = -1;
                    } else {
                        value_end = (int32)(match - unescaped.data);
                    }
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
                {
                    char *candidate_url;
                    char *match;
                    int32 candidate_url_len;
                    int32 query;
                    int32 unwrap_status;
                    bool is_wrapper;

                    candidate_url = unescaped.data + value_start;
                    candidate_url_len = value_end - value_start;
                    unwrap_status = 0;
                    sb_clear(&candidate);
                    is_wrapper = lyrics_starts_with_ignore_case(
                                     candidate_url, candidate_url_len,
                                     STRLIT("/url?"))
                                 || lyrics_starts_with_ignore_case(
                                     candidate_url, candidate_url_len,
                                     STRLIT("/l/?"))
                                 || (lyrics_find_ignore_case(
                                         candidate_url, candidate_url_len,
                                         STRLIT("google.com/url?"), 0)
                                     >= 0)
                                 || (lyrics_find_ignore_case(
                                         candidate_url, candidate_url_len,
                                         STRLIT("duckduckgo.com/l/?"), 0)
                                     >= 0);
                    if (!is_wrapper) {
                        if (lyrics_starts_with_ignore_case(
                                candidate_url, candidate_url_len,
                                STRLIT("http://"))
                            || lyrics_starts_with_ignore_case(
                                candidate_url, candidate_url_len,
                                STRLIT("https://"))) {
                            SB_APPEND(&candidate,
                                      candidate_url, candidate_url_len);
                            if ((candidate.data != NULL)
                                && (candidate.len > 0)) {
                                unwrap_status = 1;
                            }
                        }
                    } else {
                        match = memchr64(candidate_url, '?', candidate_url_len);
                        if (match != NULL) {
                            query = (int32)(match - candidate_url) + 1;
                            while (query < candidate_url_len) {
                                char *equal_match;
                                char *end_match;
                                int32 equal;
                                int32 end;

                                equal_match = memchr64(
                                    candidate_url + query, '=',
                                    candidate_url_len - query);
                                end_match = memchr64(candidate_url + query, '&',
                                                    candidate_url_len - query);
                                if (equal_match == NULL) {
                                    equal = -1;
                                } else {
                                    equal = (int32)(
                                        equal_match - candidate_url);
                                }
                                if (end_match == NULL) {
                                    end = candidate_url_len;
                                } else {
                                    end = (int32)(end_match - candidate_url);
                                }

                                if ((equal > query) && (equal < end)
                                    && (STREQUAL(candidate_url + query,
                                                 equal - query, "q")
                                        || STREQUAL(candidate_url + query,
                                                    equal - query, "url")
                                        || STREQUAL(candidate_url + query,
                                                    equal - query, "uddg"))) {
                                    lyrics_percent_decode(
                                        &candidate, candidate_url + equal + 1,
                                        end - equal - 1);
                                    if ((candidate.data != NULL)
                                        && (candidate.len > 0)
                                        && (lyrics_starts_with_ignore_case(
                                                candidate.data, candidate.len,
                                                STRLIT("http://"))
                                            || lyrics_starts_with_ignore_case(
                                                candidate.data, candidate.len,
                                                STRLIT("https://")))) {
                                        unwrap_status = 1;
                                        break;
                                    }
                                    sb_clear(&candidate);
                                }
                                query = end + 1;
                            }
                        }
                    }

                    if ((unwrap_status > 0) && (candidate.data != NULL)
                        && (candidate.len > 0)
                        && !lyrics_url_is_collected(out, candidate.data,
                                                     candidate.len)) {
                        char *url = candidate.data;
                        int32 url_len = candidate.len;

                        StrBuilder wanted_artist = {0};
                        StrBuilder wanted_title = {0};
                        char *domain;
                        int32 domain_len;
                        int32 score;

                        score = 0;
                        do {
                            bool domain_matches;
                            bool song_page;
                            int32 host_start;
                            int32 host_end;
                            int32 suffix_start;
                            int32 path_start;
                            int32 path_end;

                            domain = lyrics_type_domain(
                                fetcher->type, &domain_len);
                            domain_matches = false;
                            if (domain_len > 0) {
                                host_start = lyrics_find_ignore_case(
                                    url, url_len, STRLIT("://"), 0);
                                if (host_start >= 0) {
                                    host_start += STRLIT_LEN("://");
                                    host_end = host_start;
                                    while ((host_end < url_len)
                                           && (url[host_end] != '/')
                                           && (url[host_end] != '?')
                                           && (url[host_end] != '#')
                                           && (url[host_end] != ':')) {
                                        host_end += 1;
                                    }
                                    if (host_end - host_start >= domain_len) {
                                        suffix_start = host_end - domain_len;
                                        domain_matches =
                                            lyrics_starts_with_ignore_case(
                                                url + suffix_start, domain_len,
                                                domain, domain_len)
                                            && ((suffix_start == host_start)
                                                || (url[suffix_start - 1]
                                                    == '.'));
                                    }
                                }
                            }
                            if (!domain_matches) {
                                break;
                            }

                            song_page = false;
                            path_start = lyrics_url_path_start(url, url_len);
                            if (path_start < 0) {
                                break;
                            }
                            switch (fetcher->type) {
                            case NCM_LYRICS_FETCHER_AMALGAMA:
                                song_page = lyrics_url_path_starts_with(
                                                url, url_len, path_start,
                                                STRLIT("/songs/"))
                                            && lyrics_url_path_has_segments(
                                                url, url_len, path_start, 4)
                                            && lyrics_url_path_ends_with(
                                                url, url_len, path_start,
                                                STRLIT(".html"));
                                break;
                            case NCM_LYRICS_FETCHER_AZLYRICS:
                                song_page = lyrics_url_path_starts_with(
                                                url, url_len, path_start,
                                                STRLIT("/lyrics/"))
                                            && lyrics_url_path_has_segments(
                                                url, url_len, path_start, 3)
                                            && lyrics_url_path_ends_with(
                                                url, url_len, path_start,
                                                STRLIT(".html"));
                                break;
                            case NCM_LYRICS_FETCHER_GENIUS:
                                song_page = lyrics_url_path_has_segments(
                                                url, url_len, path_start, 1)
                                            && lyrics_url_path_ends_with(
                                                url, url_len, path_start,
                                                STRLIT("-lyrics"));
                                break;
                            case NCM_LYRICS_FETCHER_MUSIXMATCH:
                                song_page = (lyrics_url_path_starts_with(
                                                 url, url_len, path_start,
                                                STRLIT("/lyrics/"))
                                             || lyrics_url_path_starts_with(
                                                 url, url_len, path_start,
                                                STRLIT("/letras/")))
                                            && lyrics_url_path_has_segments(
                                                url, url_len, path_start, 3);
                                break;
                            case NCM_LYRICS_FETCHER_LETRASMUS:
                            case NCM_LYRICS_FETCHER_TEKSTOWO:
                                song_page = lyrics_url_path_has_segments(
                                    url, url_len, path_start, 2);
                                break;
                            case NCM_LYRICS_FETCHER_LACOCCINELLE:
                                song_page = lyrics_url_path_has_segments(
                                                url, url_len, path_start, 1)
                                            && lyrics_url_path_ends_with(
                                                url, url_len, path_start,
                                                STRLIT(".html"));
                                break;
                            case NCM_LYRICS_FETCHER_MUSICA: {
                                char *match;
                                int32 query;
                                bool have_letra;

                                have_letra = false;
                                match = memchr64(url, '?', url_len);
                                if (match != NULL) {
                                    query = (int32)(match - url) + 1;
                                    while (query < url_len) {
                                        char *equal_match;
                                        char *end_match;
                                        int32 equal;
                                        int32 end;

                                        equal_match = memchr64(
                                            url + query, '=', url_len - query);
                                        end_match = memchr64(
                                            url + query, '&', url_len - query);
                                        if (equal_match == NULL) {
                                            equal = -1;
                                        } else {
                                            equal = (int32)(equal_match - url);
                                        }
                                        if (end_match == NULL) {
                                            end_match = memchr64(
                                                url + query, '#',
                                                url_len - query);
                                        }
                                        if (end_match == NULL) {
                                            end = url_len;
                                        } else {
                                            end = (int32)(end_match - url);
                                        }
                                        if ((equal > query) && (equal < end)
                                            && (equal - query
                                                == STRLIT_LEN("letra"))
                                            && lyrics_starts_with_ignore_case(
                                                url + query, equal - query,
                                                STRLIT("letra"))) {
                                            have_letra = true;
                                            break;
                                        }
                                        query = end + 1;
                                    }
                                }
                                song_page = lyrics_url_path_ends_with(
                                                url, url_len, path_start,
                                                STRLIT("/letras.asp"))
                                            && have_letra;
                                break;
                            }
                            case NCM_LYRICS_FETCHER_PAROLES:
                                path_end = lyrics_url_path_end(
                                    url, url_len, path_start);
                                song_page = lyrics_url_path_has_segments(
                                                url, url_len, path_start, 2)
                                            && (lyrics_find_ignore_case(
                                                    url + path_start,
                                                    path_end - path_start,
                                                    STRLIT("/paroles-"), 0)
                                                >= 0)
                                            && !lyrics_url_path_ends_with(
                                                url, url_len, path_start,
                                                STRLIT("-traduction"));
                                break;
                            case NCM_LYRICS_FETCHER_VAGALUME:
                                song_page = lyrics_url_path_has_segments(
                                                url, url_len, path_start, 2)
                                            && lyrics_url_path_ends_with(
                                                url, url_len, path_start,
                                                STRLIT(".html"));
                                break;
                            case NCM_LYRICS_FETCHER_UNKNOWN:
                            case NCM_LYRICS_FETCHER_INTERNET:
                            case NCM_LYRICS_FETCHER_LAST:
                            default:
                                break;
                            }
                            if (!song_page) {
                                break;
                            }

                            if ((lyrics_append_slug(
                                     &wanted_artist, fetcher->type,
                                     artist, artist_len) < 0)
                                || (lyrics_append_slug(
                                        &wanted_title, fetcher->type,
                                        title, title_len) < 0)) {
                                if (lyrics_provider_has_flag(
                                        fetcher->type,
                                        LYRICS_PROVIDER_NUMERIC_PAGE_IDS)) {
                                    score = 1;
                                }
                                break;
                            }

                            {
                                int32 artist_score;
                                int32 title_score;

                                artist_score = lyrics_url_best_slug_score(
                                    fetcher, url, url_len, &wanted_artist);
                                title_score = lyrics_url_best_slug_score(
                                    fetcher, url, url_len, &wanted_title);
                                if ((artist_score > 0) && (title_score > 0)) {
                                    score = title_score*2 + artist_score;
                                } else if (lyrics_provider_has_flag(
                                               fetcher->type,
                                               LYRICS_PROVIDER_NUMERIC_PAGE_IDS)
                                           && (artist_score == 0)
                                           && (title_score == 0)) {
                                    bool path_has_ascii_letter;

                                    path_has_ascii_letter = false;
                                    path_start = lyrics_url_path_start(
                                        url, url_len);
                                    if (path_start >= 0) {
                                        path_end = lyrics_url_path_end(
                                    url, url_len, path_start);
                                        for (int32 i = path_start;
                                             i < path_end; i += 1) {
                                            if (lyrics_starts_with_ignore_case(
                                                    url + i, path_end - i,
                                                    STRLIT(".html"))) {
                                                i += STRLIT_LEN(".html") - 1;
                                                continue;
                                            }
                                            if (((url[i] >= 'a')
                                                 && (url[i] <= 'z'))
                                                || ((url[i] >= 'A')
                                                    && (url[i] <= 'Z'))) {
                                                path_has_ascii_letter = true;
                                                break;
                                            }
                                        }
                                    }
                                    if (!path_has_ascii_letter) {
                                        score = 1;
                                    }
                                }
                            }
                        } while (false);

                        sb_free(&wanted_title);
                        sb_free(&wanted_artist);

                        if (score > 0) {
                            StrBuilder *item;
                            int32 insert_pos;

                            if ((out->len >= LYRICS_SEARCH_MAX_CANDIDATES)
                                && (score
                                    <= scores[
                                        LYRICS_SEARCH_MAX_CANDIDATES - 1])) {
                                collect_status = 0;
                            } else {
                                if (out->len >= LYRICS_SEARCH_MAX_CANDIDATES) {
                                    insert_pos =
                                        LYRICS_SEARCH_MAX_CANDIDATES - 1;
                                    item = &out->items[insert_pos];
                                    sb_clear(item);
                                } else {
                                    item = str_builder_array_append(out);
                                    if (item == NULL) {
                                        collect_status =
                                            -NCM_ERROR_INVALID_STATE;
                                    } else {
                                        insert_pos = out->len - 1;
                                    }
                                }
                                if (collect_status >= 0) {
                                    SB_APPEND(item, url, url_len);
                                    scores[insert_pos] = score;
                                    while ((insert_pos > 0)
                                           && (scores[insert_pos]
                                               > scores[insert_pos - 1])) {
                                        StrBuilder temp_url = {0};
                                        int32 temp_score;

                                        temp_url = out->items[insert_pos - 1];
                                        out->items[insert_pos - 1]
                                            = out->items[insert_pos];
                                        out->items[insert_pos] = temp_url;

                                        temp_score = scores[insert_pos - 1];
                                        scores[insert_pos - 1]
                                            = scores[insert_pos];
                                        scores[insert_pos] = temp_score;
                                        insert_pos -= 1;
                                    }
                                }
                            }
                            if (collect_status < 0) {
                                break;
                            }
                        }
                    }
                }
                pos = value_end + 1;
            }

            sb_free(&candidate);
            sb_free(&unescaped);
            if (collect_status < 0) {
                search_status = collect_status;
            } else if (out->len <= 0) {
                search_status = -NCM_ERROR_NOT_FOUND;
            } else {
                search_status = 0;
            }
        }
        if (search_status < 0) {
            (void)ncm_lyrics_result_set(result, false,
                                        STRLIT(LYRICS_MSG_NOT_FOUND));
            goto search_cleanup;
        }

        for (int32 i = 0; i < page_urls.len; i += 1) {
            search_status = lyrics_fetch_page(
                fetcher, result, &page_urls.items[i], search_url.data,
                search_url.len, &search_retry);
            if (result->success || !search_retry) {
                break;
            }
        }

    search_cleanup:
        str_builder_array_destroy(&page_urls);
        sb_free(&data);
        sb_free(&search_url);
        status = search_status;
    }
    if ((status == 0) && !result->success && (result->text_len <= 0)) {
        (void)ncm_lyrics_result_set(result, false,
                                    STRLIT(LYRICS_MSG_NOT_FOUND));
    }
    return status;
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
                SB_APPEND(out, line, line_len);
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
    {
        char *text = out->data;
        int32 text_len = out->len;
        StrBuilder tmp = {0};

        lyrics_trim_view(&text, &text_len);
        SB_APPEND(&tmp, text, text_len);
        sb_clear(out);
        SB_APPEND(out, tmp.data, tmp.len);
        sb_free(&tmp);
    }
    return;
}

#endif /* NCMPCPP_LYRICS_FETCHER_C */
