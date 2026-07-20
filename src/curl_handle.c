#if !defined(NCMPCPP_CURL_HANDLE_C)
#define NCMPCPP_CURL_HANDLE_C

#include "curl_handle.h"

#include <stddef.h>
#include <stdint.h>

#include "c/ncm_base.h"

static size_t write_data(char *buffer, size_t size, size_t nmemb,
                         void *data);
static void append_c_string(NcmBuffer *buffer, char *string, int32 string_len);

void
ncm_curl_response_writer_init(NcmCurlResponseWriter *writer,
                              NcmBuffer *buffer) {
    writer->buffer = buffer;
    return;
}

void
ncm_curl_response_writer_destroy(NcmCurlResponseWriter *writer) {
    writer->buffer = NULL;
    return;
}

CURLcode
ncm_curl_perform(NcmBuffer *data, char *url, int32 url_len, char *referer,
                 int32 referer_len, bool follow_redirect,
                 int32 timeout_seconds) {
    CURLcode result;
    CURL *curl;
    NcmBuffer url_string;
    NcmBuffer referer_string;
    NcmCurlResponseWriter writer;

    ncm_buffer_clear(data);
    ncm_buffer_init(&url_string);
    ncm_buffer_init(&referer_string);
    ncm_curl_response_writer_init(&writer, data);

    append_c_string(&url_string, url, url_len);
    if (referer && (referer_len > 0)) {
        append_c_string(&referer_string, referer, referer_len);
    }

    curl = curl_easy_init();
    if (curl == NULL) {
        result = CURLE_FAILED_INIT;
        goto cleanup;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url_string.data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writer);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, (long)timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)timeout_seconds);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_USERAGENT,
                     "Mozilla/5.0 (compatible; ncmpcpp/" PACKAGE_VERSION ")");
    if (follow_redirect) {
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    }
    if (referer_string.len > 0) {
        curl_easy_setopt(curl, CURLOPT_REFERER, referer_string.data);
    }

    result = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

cleanup:
    ncm_curl_response_writer_destroy(&writer);
    ncm_buffer_destroy(&referer_string);
    ncm_buffer_destroy(&url_string);
    return result;
}

CURLcode
ncm_curl_escape(NcmBuffer *out, char *string, int32 string_len) {
    char *escaped;

    ncm_buffer_clear(out);
    escaped = curl_easy_escape(NULL, string, string_len);
    if (escaped == NULL) {
        return CURLE_OUT_OF_MEMORY;
    }

    for (int32 i = 0; escaped[i] != '\0'; i += 1) {
        ncm_buffer_append_byte(out, escaped[i]);
    }
    curl_free(escaped);
    return CURLE_OK;
}

static size_t
write_data(char *buffer, size_t size, size_t nmemb, void *data) {
    NcmCurlResponseWriter *writer;
    size_t bytes;

    if ((size != 0) && (nmemb > SIZE_MAX/size)) {
        return 0;
    }
    bytes = size*nmemb;
    writer = data;
    if ((writer == NULL) || (writer->buffer == NULL)) {
        return 0;
    }
    if ((writer->buffer->len >= INT32_MAX)
        || (bytes > (size_t)(INT32_MAX - writer->buffer->len - 1))) {
        return 0;
    }
    if (bytes > 0) {
        ncm_buffer_append(writer->buffer, buffer, (int32)bytes);
    }

    return bytes;
}

static void
append_c_string(NcmBuffer *buffer, char *string, int32 string_len) {
    if (string && (string_len > 0)) {
        ncm_buffer_append(buffer, string, string_len);
    }
    ncm_buffer_append_byte(buffer, '\0');
    return;
}

#endif /* NCMPCPP_CURL_HANDLE_C */
