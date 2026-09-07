#if !defined(CURL_HANDLE_H)
#define CURL_HANDLE_H

#include "cbase.h"

#include <curl/curl.h>

#include "c/ncm_c.h"

int32 ncm_curl_perform(StrBuilder *, char *url, int32 url_len, char *referer,
                       int32 referer_len, bool, int32 timeout_seconds);
int32 ncm_curl_escape(StrBuilder *, char *, int32);

#endif /* CURL_HANDLE_H */
