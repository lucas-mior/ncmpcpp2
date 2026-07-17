#if !defined(NCMPCPP_LASTFM_SERVICE_H)
#define NCMPCPP_LASTFM_SERVICE_H

#include "config.h"

#include <stdbool.h>

#include <curl/curl.h>

#include "c/ncm_defs.h"

enum NcmLastfmServiceType {
    NCM_LASTFM_SERVICE_NONE,
    NCM_LASTFM_SERVICE_ARTIST_INFO,
};

typedef struct NcmLastfmResult {
    char *text;
    int32 text_len;
    int32 text_cap;
    bool success;
} NcmLastfmResult;

typedef CURLcode (*NcmLastfmCurlPerformFn)(NcmBuffer *data,
                                           char *url,
                                           int32 url_len,
                                           char *referer,
                                           int32 referer_len,
                                           bool follow_redirect,
                                           int32 timeout_seconds,
                                           void *user);

typedef CURLcode (*NcmLastfmCurlEscapeFn)(NcmBuffer *out,
                                          char *string,
                                          int32 string_len,
                                          void *user);

typedef struct NcmLastfmService {
    char *artist;
    char *lang;

    int32 artist_len;
    int32 artist_cap;
    int32 lang_len;
    int32 lang_cap;

    enum NcmLastfmServiceType type;
} NcmLastfmService;

void ncm_lastfm_result_init(NcmLastfmResult *result);
void ncm_lastfm_result_destroy(NcmLastfmResult *result);
void ncm_lastfm_result_clear(NcmLastfmResult *result);
bool ncm_lastfm_result_set(NcmLastfmResult *result, bool success,
                           char *text, int32 text_len);

void ncm_lastfm_service_init(NcmLastfmService *service);
void ncm_lastfm_service_destroy(NcmLastfmService *service);
bool ncm_lastfm_artist_info_init(NcmLastfmService *service,
                                 char *artist,
                                 int32 artist_len,
                                 char *lang,
                                 int32 lang_len);
bool ncm_lastfm_service_equal(NcmLastfmService *left,
                              NcmLastfmService *right);
char *ncm_lastfm_service_name(NcmLastfmService *service);
enum NcmLastfmServiceType ncm_lastfm_service_type(
    NcmLastfmService *service);
bool ncm_lastfm_service_fetch(NcmLastfmService *service,
                              NcmLastfmResult *result);


#endif /* NCMPCPP_LASTFM_SERVICE_H */
