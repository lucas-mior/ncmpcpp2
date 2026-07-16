#include "lastfm_service.h"

#include <string.h>

#include "c/ncm_base.h"
#include "c/ncm_html.h"
#include "c/ncm_regex.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "cbase/cbase.h"
#include "curl_handle.h"

#define LASTFM_API_URL \
    "http://ws.audioscrobbler.com/2.0/?api_key=" \
    "d94e5b6e26469a2d1ffae8ef20131b79&method="
#define LASTFM_INVALID_RESPONSE "Invalid response"

static NcmLastfmCurlPerformFn lastfm_test_perform;
static NcmLastfmCurlEscapeFn lastfm_test_escape;
static void *lastfm_test_user;

static bool lastfm_string_set(char **data, int32 *len, int32 *cap,
                              char *source, int32 source_len);
static void lastfm_string_destroy(char **data, int32 *len, int32 *cap);
static CURLcode lastfm_curl_perform(NcmBuffer *data, char *url,
                                    int32 url_len, char *referer,
                                    int32 referer_len,
                                    bool follow_redirect,
                                    int32 timeout_seconds);
static CURLcode lastfm_curl_escape(NcmBuffer *out, char *string,
                                   int32 string_len);
static void lastfm_append_escaped(NcmBuffer *buffer, char *string,
                                  int32 string_len);
static bool lastfm_action_failed(char *data, int32 data_len);
static int32 lastfm_find(char *data, int32 data_len, char *needle,
                         int32 needle_len, int32 start);
static bool lastfm_extract_between(NcmBuffer *out, char *data,
                                   int32 data_len, char *start,
                                   int32 start_len, char *end,
                                   int32 end_len);
static void lastfm_trim_view(char **data, int32 *len);
static void lastfm_trim_buffer(NcmBuffer *buffer);
static void lastfm_strip_unescape_trim(NcmBuffer *out, char *data,
                                       int32 data_len);
static void lastfm_append_similars(NcmBuffer *out, char *data,
                                   int32 data_len, char *section_start,
                                   int32 section_start_len,
                                   char *section_end,
                                   int32 section_end_len,
                                   char *heading,
                                   int32 heading_len);
static bool lastfm_fetch_artist_info(NcmLastfmService *service,
                                     NcmLastfmResult *result);

static bool
lastfm_string_set(char **data, int32 *len, int32 *cap,
                  char *source, int32 source_len) {
    char *new_data;
    int32 new_cap;

    lastfm_string_destroy(data, len, cap);
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
lastfm_string_destroy(char **data, int32 *len, int32 *cap) {
    if (*data != NULL) {
        ncm_free(*data, *cap);
    }
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}

void
ncm_lastfm_result_init(NcmLastfmResult *result) {
    result->text = NULL;
    result->text_len = 0;
    result->text_cap = 0;
    result->success = false;
    return;
}

void
ncm_lastfm_result_destroy(NcmLastfmResult *result) {
    if (result == NULL) {
        return;
    }
    lastfm_string_destroy(&result->text, &result->text_len,
                          &result->text_cap);
    result->success = false;
    return;
}

void
ncm_lastfm_result_clear(NcmLastfmResult *result) {
    if (result == NULL) {
        return;
    }
    lastfm_string_destroy(&result->text, &result->text_len,
                          &result->text_cap);
    result->success = false;
    return;
}

bool
ncm_lastfm_result_set(NcmLastfmResult *result, bool success,
                      char *text, int32 text_len) {
    if (result == NULL) {
        return false;
    }
    if (!lastfm_string_set(&result->text, &result->text_len,
                           &result->text_cap, text, text_len)) {
        return false;
    }
    result->success = success;
    return true;
}

void
ncm_lastfm_service_init(NcmLastfmService *service) {
    service->artist = NULL;
    service->lang = NULL;
    service->artist_len = 0;
    service->artist_cap = 0;
    service->lang_len = 0;
    service->lang_cap = 0;
    service->type = NCM_LASTFM_SERVICE_NONE;
    return;
}

void
ncm_lastfm_service_destroy(NcmLastfmService *service) {
    if (service == NULL) {
        return;
    }
    lastfm_string_destroy(&service->artist, &service->artist_len,
                          &service->artist_cap);
    lastfm_string_destroy(&service->lang, &service->lang_len,
                          &service->lang_cap);
    service->type = NCM_LASTFM_SERVICE_NONE;
    return;
}

bool
ncm_lastfm_artist_info_init(NcmLastfmService *service, char *artist,
                            int32 artist_len, char *lang, int32 lang_len) {
    if (service == NULL) {
        return false;
    }
    ncm_lastfm_service_destroy(service);
    ncm_lastfm_service_init(service);
    service->type = NCM_LASTFM_SERVICE_ARTIST_INFO;
    if (!lastfm_string_set(&service->artist, &service->artist_len,
                           &service->artist_cap, artist, artist_len)) {
        return false;
    }
    if (!lastfm_string_set(&service->lang, &service->lang_len,
                           &service->lang_cap, lang, lang_len)) {
        return false;
    }
    return true;
}

bool
ncm_lastfm_service_equal(NcmLastfmService *left, NcmLastfmService *right) {
    if ((left == NULL) || (right == NULL)) {
        return false;
    }
    if (left->type != right->type) {
        return false;
    }
    if (!ncm_string_equal(left->artist, left->artist_len,
                          right->artist, right->artist_len)) {
        return false;
    }
    return ncm_string_equal(left->lang, left->lang_len,
                            right->lang, right->lang_len);
}

char *
ncm_lastfm_service_name(NcmLastfmService *service) {
    if ((service != NULL)
        && (service->type == NCM_LASTFM_SERVICE_ARTIST_INFO)) {
        return (char *)"Artist info";
    }
    return (char *)"Last.fm";
}

enum NcmLastfmServiceType
ncm_lastfm_service_type(NcmLastfmService *service) {
    if (service == NULL) {
        return NCM_LASTFM_SERVICE_NONE;
    }
    return service->type;
}

bool
ncm_lastfm_service_fetch(NcmLastfmService *service,
                         NcmLastfmResult *result) {
    if ((service == NULL) || (result == NULL)) {
        return false;
    }
    ncm_lastfm_result_clear(result);
    if (service->type == NCM_LASTFM_SERVICE_ARTIST_INFO) {
        return lastfm_fetch_artist_info(service, result);
    }
    ncm_lastfm_result_set(result, false, STRLIT_ARGS(LASTFM_INVALID_RESPONSE));
    return true;
}

void
ncm_lastfm_service_set_io_for_tests(NcmLastfmCurlPerformFn perform,
                                    NcmLastfmCurlEscapeFn escape,
                                    void *user) {
    lastfm_test_perform = perform;
    lastfm_test_escape = escape;
    lastfm_test_user = user;
    return;
}

static CURLcode
lastfm_curl_perform(NcmBuffer *data, char *url, int32 url_len,
                    char *referer, int32 referer_len,
                    bool follow_redirect, int32 timeout_seconds) {
    if (lastfm_test_perform != NULL) {
        return lastfm_test_perform(data, url, url_len, referer, referer_len,
                                   follow_redirect, timeout_seconds,
                                   lastfm_test_user);
    }
    return ncm_curl_perform(data, url, url_len, referer, referer_len,
                            follow_redirect, timeout_seconds);
}

static CURLcode
lastfm_curl_escape(NcmBuffer *out, char *string, int32 string_len) {
    if (lastfm_test_escape != NULL) {
        return lastfm_test_escape(out, string, string_len,
                                  lastfm_test_user);
    }
    return ncm_curl_escape(out, string, string_len);
}

static void
lastfm_append_escaped(NcmBuffer *buffer, char *string, int32 string_len) {
    NcmBuffer escaped;

    ncm_buffer_init(&escaped);
    if (lastfm_curl_escape(&escaped, string, string_len) == CURLE_OK) {
        ncm_buffer_append(buffer, escaped.data, escaped.len);
    }
    ncm_buffer_destroy(&escaped);
    return;
}

static bool
lastfm_action_failed(char *data, int32 data_len) {
    NcmRegex regex;
    NcmError error;
    bool failed;

    ncm_error_clear(&error);
    ncm_regex_init(&regex);
    failed = false;
    if (ncm_regex_compile(&regex, STRLIT_ARGS("status=\"failed\""),
                          NCM_REGEX_EXTENDED, &error)) {
        failed = ncm_regex_search(&regex, data, data_len);
    }
    ncm_regex_destroy(&regex);
    return failed;
}

static int32
lastfm_find(char *data, int32 data_len, char *needle, int32 needle_len,
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
lastfm_extract_between(NcmBuffer *out, char *data, int32 data_len,
                       char *start, int32 start_len, char *end,
                       int32 end_len) {
    int32 a;
    int32 b;

    ncm_buffer_clear(out);
    a = lastfm_find(data, data_len, start, start_len, 0);
    if (a < 0) {
        return false;
    }
    a += start_len;
    b = lastfm_find(data, data_len, end, end_len, a);
    if (b < 0) {
        return false;
    }
    ncm_buffer_append(out, data + a, b - a);
    return true;
}

static void
lastfm_trim_view(char **data, int32 *len) {
    char *text;
    int32 text_len;

    text = *data;
    text_len = *len;
    while ((text_len > 0)
           && ((text[0] == ' ') || (text[0] == '\t')
               || (text[0] == '\n') || (text[0] == '\r'))) {
        text += 1;
        text_len -= 1;
    }
    while ((text_len > 0)
           && ((text[text_len - 1] == ' ')
               || (text[text_len - 1] == '\t')
               || (text[text_len - 1] == '\n')
               || (text[text_len - 1] == '\r'))) {
        text_len -= 1;
    }
    *data = text;
    *len = text_len;
    return;
}

static void
lastfm_trim_buffer(NcmBuffer *buffer) {
    char *text;
    int32 text_len;
    NcmBuffer tmp;

    text = buffer->data;
    text_len = buffer->len;
    lastfm_trim_view(&text, &text_len);
    ncm_buffer_init(&tmp);
    ncm_buffer_append(&tmp, text, text_len);
    ncm_buffer_clear(buffer);
    ncm_buffer_append(buffer, tmp.data, tmp.len);
    ncm_buffer_destroy(&tmp);
    return;
}

static void
lastfm_strip_unescape_trim(NcmBuffer *out, char *data, int32 data_len) {
    NcmBuffer stripped;
    NcmBuffer unescaped;

    ncm_buffer_clear(out);
    stripped = ncm_html_strip_tags(data, data_len);
    unescaped = ncm_html_unescape_utf8(stripped.data, stripped.len);
    ncm_buffer_append(out, unescaped.data, unescaped.len);
    lastfm_trim_buffer(out);
    ncm_buffer_destroy(&unescaped);
    ncm_buffer_destroy(&stripped);
    return;
}

static void
lastfm_append_similars(NcmBuffer *out, char *data, int32 data_len,
                       char *section_start, int32 section_start_len,
                       char *section_end, int32 section_end_len,
                       char *heading, int32 heading_len) {
    int32 a;
    int32 b;
    int32 pos;
    bool wrote_heading;

    a = lastfm_find(data, data_len, section_start, section_start_len, 0);
    b = lastfm_find(data, data_len, section_end, section_end_len, 0);
    if ((a < 0) || (b < 0) || (b <= a)) {
        return;
    }

    pos = a;
    wrote_heading = false;
    while (pos < b) {
        NcmBuffer name;
        NcmBuffer url;
        NcmBuffer clean_name;
        NcmBuffer clean_url;
        int32 item_end;
        bool have_name;
        bool have_url;

        item_end = lastfm_find(data, b, STRLIT_ARGS("</artist>"), pos);
        if (item_end < 0) {
            item_end = lastfm_find(data, b, STRLIT_ARGS("</tag>"), pos);
        }
        if (item_end < 0) {
            break;
        }

        ncm_buffer_init(&name);
        ncm_buffer_init(&url);
        ncm_buffer_init(&clean_name);
        ncm_buffer_init(&clean_url);
        have_name = lastfm_extract_between(&name, data + pos, item_end - pos,
                                           STRLIT_ARGS("<name>"),
                                           STRLIT_ARGS("</name>"));
        have_url = lastfm_extract_between(&url, data + pos, item_end - pos,
                                          STRLIT_ARGS("<url>"),
                                          STRLIT_ARGS("</url>"));
        if (have_name && have_url) {
            lastfm_strip_unescape_trim(&clean_name, name.data, name.len);
            lastfm_strip_unescape_trim(&clean_url, url.data, url.len);
            if (!wrote_heading) {
                ncm_buffer_append(out, heading, heading_len);
                wrote_heading = true;
            }
            ncm_buffer_append(out, STRLIT_ARGS("\n * "));
            ncm_buffer_append(out, clean_name.data, clean_name.len);
            ncm_buffer_append(out, STRLIT_ARGS(" ("));
            ncm_buffer_append(out, clean_url.data, clean_url.len);
            ncm_buffer_append_byte(out, ')');
        }
        ncm_buffer_destroy(&clean_url);
        ncm_buffer_destroy(&clean_name);
        ncm_buffer_destroy(&url);
        ncm_buffer_destroy(&name);
        pos = item_end + 1;
    }
    return;
}

static bool
lastfm_fetch_artist_info(NcmLastfmService *service,
                         NcmLastfmResult *result) {
    NcmBuffer url;
    NcmBuffer data;
    NcmBuffer content;
    NcmBuffer desc;
    NcmBuffer original_link;
    NcmBuffer output;
    CURLcode code;
    bool ok;

    ncm_buffer_init(&url);
    ncm_buffer_init(&data);
    ncm_buffer_init(&content);
    ncm_buffer_init(&desc);
    ncm_buffer_init(&original_link);
    ncm_buffer_init(&output);
    ok = true;

    ncm_buffer_append(&url, STRLIT_ARGS(LASTFM_API_URL));
    ncm_buffer_append(&url, STRLIT_ARGS("artist.getinfo&artist="));
    lastfm_append_escaped(&url, service->artist, service->artist_len);
    if (service->lang_len > 0) {
        ncm_buffer_append(&url, STRLIT_ARGS("&lang="));
        lastfm_append_escaped(&url, service->lang, service->lang_len);
    }

    code = lastfm_curl_perform(&data, url.data, url.len, NULL, 0,
                               false, 10);
    if (code != CURLE_OK) {
        char *message = (char *)curl_easy_strerror(code);
        ncm_lastfm_result_set(result, false, message, strlen32(message));
        goto cleanup;
    }
    if (lastfm_action_failed(data.data, data.len)) {
        ncm_lastfm_result_set(result, false,
                              STRLIT_ARGS(LASTFM_INVALID_RESPONSE));
        goto cleanup;
    }
    if (!lastfm_extract_between(&content, data.data, data.len,
                                STRLIT_ARGS("<content>"),
                                STRLIT_ARGS("</content>"))) {
        ncm_lastfm_result_set(result, false,
                              STRLIT_ARGS(LASTFM_INVALID_RESPONSE));
        goto cleanup;
    }
    if (content.len <= 0) {
        ncm_lastfm_result_set(result, false,
                              STRLIT_ARGS("No description available for "
                                          "this artist."));
        goto cleanup;
    }

    lastfm_strip_unescape_trim(&desc, content.data, content.len);
    ncm_buffer_append(&output, desc.data, desc.len);
    lastfm_append_similars(&output, data.data, data.len,
                           STRLIT_ARGS("<similar>"),
                           STRLIT_ARGS("</similar>"),
                           STRLIT_ARGS("\n\nSimilar artists:\n"));
    lastfm_append_similars(&output, data.data, data.len,
                           STRLIT_ARGS("<tags>"),
                           STRLIT_ARGS("</tags>"),
                           STRLIT_ARGS("\n\nSimilar tags:\n"));
    if (lastfm_extract_between(&original_link, data.data, data.len,
                               STRLIT_ARGS("<url>"),
                               STRLIT_ARGS("</url>"))) {
        NcmBuffer clean_url;

        ncm_buffer_init(&clean_url);
        lastfm_strip_unescape_trim(&clean_url, original_link.data,
                                   original_link.len);
        if (clean_url.len > 0) {
            ncm_buffer_append(&output, STRLIT_ARGS("\n\n"));
            ncm_buffer_append(&output, clean_url.data, clean_url.len);
        }
        ncm_buffer_destroy(&clean_url);
    }
    ncm_lastfm_result_set(result, true, output.data, output.len);

cleanup:
    ncm_buffer_destroy(&output);
    ncm_buffer_destroy(&original_link);
    ncm_buffer_destroy(&desc);
    ncm_buffer_destroy(&content);
    ncm_buffer_destroy(&data);
    ncm_buffer_destroy(&url);
    return ok;
}
