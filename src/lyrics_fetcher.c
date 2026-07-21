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

static NcmLyricsCurlPerformFn lyrics_test_perform;
static void *lyrics_test_user;

static NcmArrayItemCallbacks lyrics_fetcher_callbacks;

static void lyrics_string_destroy(char **data, int32 *len, int32 *cap);
static void lyrics_fetcher_array_init_item(void *item);
static void lyrics_fetcher_array_destroy_item(void *item);
static bool lyrics_name_to_type(char *name, int32 name_len,
                                enum NcmLyricsFetcherType *type);
static char *lyrics_type_name(enum NcmLyricsFetcherType type, int32 *len);
static char *lyrics_type_domain(enum NcmLyricsFetcherType type, int32 *len);
static bool lyrics_append_slug(StrBuilder *buffer,
                               char *string, int32 string_len, bool compact);
static void lyrics_append_query(StrBuilder *buffer,
                                char *string, int32 string_len);
static int32 lyrics_hex_value(char ch);
static bool lyrics_build_direct_url(NcmLyricsFetcherDef *fetcher,
                                    StrBuilder *url, char *artist,
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
ncm_lyrics_result_set(NcmLyricsResult *result, bool success, char *text,
                      int32 text_len) {
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
ncm_lyrics_fetcher_def_set_name(NcmLyricsFetcherDef *fetcher, char *name,
                                int32 name_len) {
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
lyrics_build_direct_url(NcmLyricsFetcherDef *fetcher, StrBuilder *url,
                        char *artist, int32 artist_len, char *title,
                        int32 title_len) {
    bool compact;
    bool valid;

    if ((fetcher == NULL) || (url == NULL) || !fetcher->enabled
        || (artist == NULL) || (artist_len <= 0) || (title == NULL)
        || (title_len <= 0)) {
        return false;
    }

    sb_clear(url);
    compact = false;
    switch (fetcher->type) {
    case NCM_LYRICS_FETCHER_AZLYRICS:
        sb_append(url,
                          STRLIT_ARGS("https://www.azlyrics.com/lyrics/"));
        compact = true;
        valid = lyrics_append_slug(url, artist, artist_len, compact);
        sb_append_byte(url, '/');
        valid = valid && lyrics_append_slug(url, title, title_len, compact);
        sb_append(url, STRLIT_ARGS(".html"));
        break;
    case NCM_LYRICS_FETCHER_GENIUS:
        sb_append(url, STRLIT_ARGS("https://genius.com/"));
        valid = lyrics_append_slug(url, artist, artist_len, compact);
        sb_append_byte(url, '-');
        valid = valid && lyrics_append_slug(url, title, title_len, compact);
        sb_append(url, STRLIT_ARGS("-lyrics"));
        break;
    case NCM_LYRICS_FETCHER_LETRASMUS:
        sb_append(url, STRLIT_ARGS("https://www.letras.mus.br/"));
        valid = lyrics_append_slug(url, artist, artist_len, compact);
        sb_append_byte(url, '/');
        valid = valid && lyrics_append_slug(url, title, title_len, compact);
        sb_append_byte(url, '/');
        break;
    case NCM_LYRICS_FETCHER_MUSIXMATCH:
        sb_append(url,
                          STRLIT_ARGS("https://www.musixmatch.com/lyrics/"));
        valid = lyrics_append_slug(url, artist, artist_len, compact);
        sb_append_byte(url, '/');
        valid = valid && lyrics_append_slug(url, title, title_len, compact);
        break;
    case NCM_LYRICS_FETCHER_TEKSTOWO:
        sb_append(url, STRLIT_ARGS("https://www.tekstowo.pl/"));
        valid = lyrics_append_slug(url, artist, artist_len, compact);
        sb_append_byte(url, '/');
        valid = valid && lyrics_append_slug(url, title, title_len, compact);
        break;
    case NCM_LYRICS_FETCHER_VAGALUME:
        sb_append(url,
                          STRLIT_ARGS("https://www.vagalume.com.br/"));
        valid = lyrics_append_slug(url, artist, artist_len, compact);
        sb_append_byte(url, '/');
        valid = valid && lyrics_append_slug(url, title, title_len, compact);
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
    StrBuilder direct_url;
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

    sb_init(&direct_url);
    ok = lyrics_build_direct_url(fetcher, &direct_url, artist, artist_len,
                                 title, title_len);
    if (ok) {
        ok = lyrics_fetch_page(fetcher, result, &direct_url, NULL, 0, &retry);
    }
    sb_free(&direct_url);
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

static bool
lyrics_append_slug(StrBuilder *buffer, char *string, int32 string_len,
                   bool compact) {
    bool pending_separator;
    bool wrote;

    pending_separator = false;
    wrote = false;
    for (int32 i = 0; i < string_len; i += 1) {
        uint8 byte;

        byte = (uint8)string[i];
        if (lyrics_ascii_alnum(string[i])) {
            if (pending_separator && wrote && !compact) {
                sb_append_byte(buffer, '-');
            }
            sb_append_byte(buffer, lyrics_ascii_lower(string[i]));
            pending_separator = false;
            wrote = true;
        } else if (byte >= 0x80) {
            if (pending_separator && wrote && !compact) {
                sb_append_byte(buffer, '-');
            }
            lyrics_append_percent_byte(buffer, byte);
            pending_separator = false;
            wrote = true;
        } else if ((string[i] == '\'') || (string[i] == '`')) {
            continue;
        } else if (wrote && !compact) {
            pending_separator = true;
        }
    }
    return wrote;
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

static bool
lyrics_url_song_matches(NcmLyricsFetcherDef *fetcher, char *url,
                        int32 url_len, char *artist, int32 artist_len,
                        char *title, int32 title_len) {
    StrBuilder marker;
    bool compact;
    bool include_title;
    bool valid;
    int32 found;

    sb_init(&marker);
    compact = false;
    include_title = true;
    switch (fetcher->type) {
    case NCM_LYRICS_FETCHER_AZLYRICS:
        sb_append(&marker, STRLIT_ARGS("/lyrics/"));
        compact = true;
        valid = lyrics_append_slug(&marker, artist, artist_len, compact);
        sb_append_byte(&marker, '/');
        break;
    case NCM_LYRICS_FETCHER_GENIUS:
        sb_append_byte(&marker, '/');
        valid = lyrics_append_slug(&marker, artist, artist_len, compact);
        sb_append_byte(&marker, '-');
        break;
    case NCM_LYRICS_FETCHER_MUSIXMATCH:
        sb_append(&marker, STRLIT_ARGS("/lyrics/"));
        valid = lyrics_append_slug(&marker, artist, artist_len, compact);
        sb_append_byte(&marker, '/');
        break;
    case NCM_LYRICS_FETCHER_LETRASMUS:
        sb_append_byte(&marker, '/');
        valid = lyrics_append_slug(&marker, artist, artist_len, compact);
        sb_append_byte(&marker, '/');
        include_title = false;
        break;
    case NCM_LYRICS_FETCHER_TEKSTOWO:
    case NCM_LYRICS_FETCHER_VAGALUME:
        sb_append_byte(&marker, '/');
        valid = lyrics_append_slug(&marker, artist, artist_len, compact);
        sb_append_byte(&marker, '/');
        break;
    case NCM_LYRICS_FETCHER_UNKNOWN:
    case NCM_LYRICS_FETCHER_INTERNET:
    case NCM_LYRICS_FETCHER_LAST:
    default:
        valid = false;
        break;
    }
    if (valid && include_title) {
        valid = lyrics_append_slug(&marker, title, title_len, compact);
    }

    found = -1;
    if (valid) {
        found = lyrics_find_ignore_case(url, url_len, marker.data, marker.len,
                                        0);
    }
    if ((found < 0)
        && (fetcher->type == NCM_LYRICS_FETCHER_MUSIXMATCH)) {
        sb_clear(&marker);
        sb_append(&marker, STRLIT_ARGS("/letras/"));
        valid = lyrics_append_slug(&marker, artist, artist_len, false);
        sb_append_byte(&marker, '/');
        if (valid && lyrics_append_slug(&marker, title, title_len, false)) {
            found = lyrics_find_ignore_case(url, url_len, marker.data,
                                            marker.len, 0);
        }
    }
    sb_free(&marker);
    return found >= 0;
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
lyrics_search_candidate_ok(NcmLyricsFetcherDef *fetcher, char *url,
                           int32 url_len, char *artist, int32 artist_len,
                           char *title, int32 title_len) {
    char *domain;
    int32 domain_len;

    domain = lyrics_type_domain(fetcher->type, &domain_len);
    return (domain_len > 0)
           && lyrics_url_domain_matches(url, url_len, domain, domain_len)
           && lyrics_url_song_matches(fetcher, url, url_len, artist,
                                      artist_len, title, title_len);
}

static bool
lyrics_extract_search_url(NcmLyricsFetcherDef *fetcher, StrBuilder *out,
                          char *data, int32 data_len, char *artist,
                          int32 artist_len, char *title, int32 title_len) {
    StrBuilder numeric_unescaped;
    StrBuilder unescaped;
    StrBuilder candidate;
    int32 pos;
    bool found;

    numeric_unescaped = ncm_html_unescape_utf8(data, data_len);
    unescaped = ncm_html_unescape_entities(numeric_unescaped.data,
                                           numeric_unescaped.len);
    sb_free(&numeric_unescaped);
    sb_init(&candidate);
    sb_clear(out);
    pos = 0;
    found = false;
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
            && lyrics_search_candidate_ok(fetcher, candidate.data,
                                          candidate.len, artist, artist_len,
                                          title, title_len)) {
            sb_append(out, candidate.data, candidate.len);
            found = true;
            break;
        }
        pos = value_end + 1;
    }

    sb_free(&candidate);
    sb_free(&unescaped);
    return found;
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
    StrBuilder page_url;
    char *message;
    int32 message_len;
    bool retry;
    bool ok;

    sb_init(&search_url);
    sb_init(&data);
    sb_init(&page_url);
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
    if (!lyrics_extract_search_url(fetcher, &page_url, data.data, data.len,
                                   artist, artist_len, title, title_len)) {
        ncm_lyrics_result_set(result, false, STRLIT_ARGS(LYRICS_MSG_NOT_FOUND));
        ok = true;
        goto cleanup;
    }
    ok = lyrics_fetch_page(fetcher, result, &page_url, search_url.data,
                           search_url.len, &retry);

cleanup:
    sb_free(&page_url);
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
