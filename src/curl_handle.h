#if !defined(NCMPCPP_CURL_HANDLE_H)
#define NCMPCPP_CURL_HANDLE_H

#include "config.h"

#include <stdbool.h>

#include <curl/curl.h>

#include "c/ncm_defs.h"

typedef struct NcmCurlResponseWriter {
    NcmBuffer *buffer;
} NcmCurlResponseWriter;

void ncm_curl_response_writer_init(NcmCurlResponseWriter *writer,
                                   NcmBuffer *buffer);
void ncm_curl_response_writer_destroy(NcmCurlResponseWriter *writer);

CURLcode ncm_curl_perform(NcmBuffer *data, char *url, int32 url_len,
                          char *referer, int32 referer_len,
                          bool follow_redirect, int32 timeout_seconds);
CURLcode ncm_curl_escape(NcmBuffer *out, char *string, int32 string_len);

#endif /* NCMPCPP_CURL_HANDLE_H */
