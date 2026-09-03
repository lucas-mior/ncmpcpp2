#if !defined(NCMPCPP_CURL_HANDLE_H)
#define NCMPCPP_CURL_HANDLE_H

#include "cbase.h"

#include <curl/curl.h>

#include "c/ncm_c.h"

int32 ncm_curl_perform(StrBuilder *data, char *url, int32 url_len,
                       char *referer, int32 referer_len,
                       bool follow_redirect, int32 timeout_seconds);
int32 ncm_curl_escape(StrBuilder *out, char *string, int32 string_len);

#endif /* NCMPCPP_CURL_HANDLE_H */
