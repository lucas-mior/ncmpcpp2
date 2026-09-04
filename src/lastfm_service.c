#if !defined(NCMPCPP_LASTFM_SERVICE_C)
#define NCMPCPP_LASTFM_SERVICE_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "curl_handle.h"
#include "lastfm_service.h"

#define LASTFM_API_URL \
    "http://ws.audioscrobbler.com/2.0/?api_key=" \
    "d94e5b6e26469a2d1ffae8ef20131b79&method="
#define LASTFM_INVALID_RESPONSE "Invalid response"

static NcmLastfmCurlPerformFn lastfm_test_perform;
static NcmLastfmCurlEscapeFn lastfm_test_escape;
static void *lastfm_test_user;

static void lastfm_string_destroy(char **data, int32 *len, int32 *cap);
static void
lastfm_string_set(char **data, int32 *len, int32 *cap, char *source,
                  int32 source_len) {
    char *new_data;
    int32 new_cap;

    lastfm_string_destroy(data, len, cap);
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
lastfm_string_destroy(char **data, int32 *len, int32 *cap) {
    free2(*data, *cap);
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}

void
ncm_lastfm_result_destroy(NcmLastfmResult *result) {
    if (result == NULL) {
        return;
    }
    lastfm_string_destroy(&result->text, &result->text_len, &result->text_cap);
    result->success = false;
    return;
}

void
ncm_lastfm_result_clear(NcmLastfmResult *result) {
    if (result == NULL) {
        return;
    }
    lastfm_string_destroy(&result->text, &result->text_len, &result->text_cap);
    result->success = false;
    return;
}

int32
ncm_lastfm_result_set(NcmLastfmResult *result, bool success, char *text,
                      int32 text_len) {
    if (result == NULL) {
        return -EINVAL;
    }
    lastfm_string_set(&result->text, &result->text_len, &result->text_cap,
                      text, text_len);
    result->success = success;
    return 0;
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

int32
ncm_lastfm_artist_info_init(NcmLastfmService *service, char *artist,
                            int32 artist_len, char *lang, int32 lang_len) {
    if ((service == NULL) || (artist == NULL) || (artist_len <= 0)) {
        return -EINVAL;
    }
    ncm_lastfm_service_destroy(service);
    *service = (NcmLastfmService){0};
    service->type = NCM_LASTFM_SERVICE_ARTIST_INFO;
    lastfm_string_set(&service->artist, &service->artist_len,
                      &service->artist_cap, artist, artist_len);
    lastfm_string_set(&service->lang, &service->lang_len,
                      &service->lang_cap, lang, lang_len);
    return 0;
}

bool
ncm_lastfm_service_is_equal(NcmLastfmService *left, NcmLastfmService *right) {
    if ((left == NULL) || (right == NULL)) {
        return false;
    }
    if (left->type != right->type) {
        return false;
    }
    if (!STREQUAL(left->artist, left->artist_len,
                  right->artist, right->artist_len)) {
        return false;
    }
    return STREQUAL(left->lang, left->lang_len, right->lang, right->lang_len);
}

char *
ncm_lastfm_service_name(NcmLastfmService *service) {
    if (service
        && (service->type == NCM_LASTFM_SERVICE_ARTIST_INFO)) {
        return "Artist info";
    }
    return "Last.fm";
}

enum NcmLastfmServiceType
ncm_lastfm_service_type(NcmLastfmService *service) {
    if (service == NULL) {
        return NCM_LASTFM_SERVICE_NONE;
    }
    return service->type;
}

static int32
lastfm_append_escaped(StrBuilder *buffer, char *string, int32 string_len) {
    StrBuilder escaped = {0};
    int32 status;

    if (lastfm_test_escape) {
        status = lastfm_test_escape(&escaped, string, string_len,
                                    lastfm_test_user);
    } else {
        status = ncm_curl_escape(&escaped, string, string_len);
    }
    if (status == 0) {
        SB_APPEND(buffer, escaped.data, escaped.len);
    }
    sb_free(&escaped);
    return status;
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
        if (BEGINS_WITH(data + i, data_len - i, needle, needle_len)) {
            return i;
        }
    }
    return -1;
}

static int32
lastfm_extract_between(StrBuilder *out,
                       char *data, int32 data_len,
                       char *start, int32 start_len,
                       char *end, int32 end_len) {
    int32 a;
    int32 b;

    if ((out == NULL) || (data == NULL) || (data_len < 0)
        || (start == NULL) || (start_len <= 0)
        || (end == NULL) || (end_len <= 0)) {
        return -EINVAL;
    }

    sb_clear(out);
    a = lastfm_find(data, data_len, start, start_len, 0);
    if (a < 0) {
        return -NCM_ERROR_NOT_FOUND;
    }
    a += start_len;
    b = lastfm_find(data, data_len, end, end_len, a);
    if (b < 0) {
        return -NCM_ERROR_NOT_FOUND;
    }
    SB_APPEND(out, data + a, b - a);
    return 0;
}

static void
lastfm_strip_unescape_trim(StrBuilder *out, char *data, int32 data_len) {
    StrBuilder stripped;
    StrBuilder unescaped;
    StrBuilder tmp = {0};
    char *text;
    int32 text_len;

    sb_clear(out);
    stripped = ncm_html_strip_tags(data, data_len);
    unescaped = ncm_html_unescape_utf8(stripped.data, stripped.len);
    SB_APPEND(out, unescaped.data, unescaped.len);

    text = out->data;
    text_len = out->len;
    while ((text_len > 0)
           && ((text[0] == ' ') || (text[0] == '\t') || (text[0] == '\n')
               || (text[0] == '\r'))) {
        text += 1;
        text_len -= 1;
    }
    while ((text_len > 0)
           && ((text[text_len - 1] == ' ') || (text[text_len - 1] == '\t')
               || (text[text_len - 1] == '\n')
               || (text[text_len - 1] == '\r'))) {
        text_len -= 1;
    }
    SB_APPEND(&tmp, text, text_len);
    sb_clear(out);
    SB_APPEND(out, tmp.data, tmp.len);
    sb_free(&tmp);
    sb_free(&unescaped);
    sb_free(&stripped);
    return;
}

static void
lastfm_append_similars(StrBuilder *out, char *data, int32 data_len,
                       char *section_start, int32 section_start_len,
                       char *section_end, int32 section_end_len, char *heading,
                       int32 heading_len) {
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
        StrBuilder name = {0};
        StrBuilder url = {0};
        StrBuilder clean_name = {0};
        StrBuilder clean_url = {0};
        int32 item_end;
        bool have_name;
        bool have_url;

        item_end = lastfm_find(data, b, STRLIT("</artist>"), pos);
        if (item_end < 0) {
            item_end = lastfm_find(data, b, STRLIT("</tag>"), pos);
        }
        if (item_end < 0) {
            break;
        }

        have_name = lastfm_extract_between(&name, data + pos, item_end - pos,
                                           STRLIT("<name>"),
                                           STRLIT("</name>")) == 0;
        have_url = lastfm_extract_between(&url, data + pos, item_end - pos,
                                          STRLIT("<url>"),
                                          STRLIT("</url>")) == 0;
        if (have_name && have_url) {
            lastfm_strip_unescape_trim(&clean_name, name.data, name.len);
            lastfm_strip_unescape_trim(&clean_url, url.data, url.len);
            if (!wrote_heading) {
                SB_APPEND(out, heading, heading_len);
                wrote_heading = true;
            }
            SB_APPEND(out, "\n*");
            SB_APPEND(out, clean_name.data, clean_name.len);
            SB_APPEND(out, " (");
            SB_APPEND(out, clean_url.data, clean_url.len);
            sb_append_byte(out, ')');
        }
        sb_free(&clean_url);
        sb_free(&clean_name);
        sb_free(&url);
        sb_free(&name);
        pos = item_end + 1;
    }
    return;
}

int32
ncm_lastfm_service_fetch(NcmLastfmService *service, NcmLastfmResult *result) {
    StrBuilder url = {0};
    StrBuilder data = {0};
    StrBuilder content = {0};
    StrBuilder desc = {0};
    StrBuilder original_link = {0};
    StrBuilder output = {0};
    char *message;
    int32 status;

    if ((service == NULL) || (result == NULL)) {
        return -EINVAL;
    }
    ncm_lastfm_result_clear(result);
    if (service->type != NCM_LASTFM_SERVICE_ARTIST_INFO) {
        return ncm_lastfm_result_set(result, false,
                                     STRLIT(LASTFM_INVALID_RESPONSE));
    }

    status = 0;
    SB_APPEND(&url, STRLIT(LASTFM_API_URL));
    SB_APPEND(&url, "artist.getinfo&artist=");
    status = lastfm_append_escaped(&url, service->artist, service->artist_len);
    if (status < 0) {
        goto cleanup;
    }
    if (service->lang_len > 0) {
        SB_APPEND(&url, "&lang=");
        status = lastfm_append_escaped(&url, service->lang, service->lang_len);
        if (status < 0) {
            goto cleanup;
        }
    }

    if (lastfm_test_perform) {
        status = lastfm_test_perform(&data, url.data, url.len, NULL, 0,
                                     false, 10, lastfm_test_user);
    } else {
        status = ncm_curl_perform(&data, url.data, url.len, NULL, 0,
                                  false, 10);
    }
    if (status < 0) {
        message = "Network error";
        if (status == -ETIMEDOUT) {
            message = "Request timed out";
        }
        (void)ncm_lastfm_result_set(result, false,
                                    message, strlen32(message));
        goto cleanup;
    }
    {
        NcmRegex regex = {0};
        NcmError ncm_error = {0};
        bool failed;

        ncm_error_clear(&ncm_error);
        failed = false;
        if (ncm_regex_compile(&regex, STRLIT("status=\"failed\""),
                              NCM_REGEX_EXTENDED, &ncm_error) == 0) {
            failed = ncm_regex_matches(&regex, data.data, data.len);
        }
        ncm_regex_destroy(&regex);
        if (failed) {
            status = ncm_lastfm_result_set(result, false,
                                           STRLIT(LASTFM_INVALID_RESPONSE));
            goto cleanup;
        }
    }
    if (lastfm_extract_between(&content, data.data, data.len,
                               STRLIT("<content>"),
                               STRLIT("</content>")) < 0) {
        status = ncm_lastfm_result_set(result, false,
                                       STRLIT(LASTFM_INVALID_RESPONSE));
        goto cleanup;
    }
    if (content.len <= 0) {
        status = ncm_lastfm_result_set(result, false,
                                       STRLIT("No description available for "
                                              "this artist."));
        goto cleanup;
    }

    lastfm_strip_unescape_trim(&desc, content.data, content.len);
    SB_APPEND(&output, desc.data, desc.len);
    lastfm_append_similars(&output, data.data, data.len,
                           STRLIT("<similar>"), STRLIT("</similar>"),
                           STRLIT("\n\nSimilar artists:\n"));
    lastfm_append_similars(&output, data.data, data.len, STRLIT("<tags>"),
                           STRLIT("</tags>"),
                           STRLIT("\n\nSimilar tags:\n"));
    if (lastfm_extract_between(&original_link, data.data, data.len,
                               STRLIT("<url>"), STRLIT("</url>")) == 0) {
        StrBuilder clean_url = {0};

        lastfm_strip_unescape_trim(&clean_url, original_link.data,
                                   original_link.len);
        if (clean_url.len > 0) {
            SB_APPEND(&output, "\n\n");
            SB_APPEND(&output, clean_url.data, clean_url.len);
        }
        sb_free(&clean_url);
    }
    status = ncm_lastfm_result_set(result, true, output.data, output.len);

cleanup:
    sb_free(&output);
    sb_free(&original_link);
    sb_free(&desc);
    sb_free(&content);
    sb_free(&data);
    sb_free(&url);
    return status;
}

#endif /* NCMPCPP_LASTFM_SERVICE_C */
