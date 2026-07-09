#if !defined(NCMPCPP_LYRICS_FETCHER_H)
#define NCMPCPP_LYRICS_FETCHER_H

#include "config.h"

#include <stdbool.h>

#include <curl/curl.h>

#include "c/ncm_array.h"
#include "c/ncm_defs.h"
#include "c/ncm_song.h"
#include "curl_handle.h"

NCM_EXTERN_C_BEGIN

enum NcmLyricsFetcherType {
    NCM_LYRICS_FETCHER_UNKNOWN,
    NCM_LYRICS_FETCHER_JUSTSOMELYRICS,
    NCM_LYRICS_FETCHER_JAHLYRICS,
    NCM_LYRICS_FETCHER_PLYRICS,
    NCM_LYRICS_FETCHER_AZLYRICS,
    NCM_LYRICS_FETCHER_TEKSTOWO,
    NCM_LYRICS_FETCHER_ZENESZOVEG,
    NCM_LYRICS_FETCHER_INTERNET,
    NCM_LYRICS_FETCHER_TAGS,
};

typedef struct NcmLyricsResult {
    char *text;
    int32 text_len;
    int32 text_cap;
    bool success;
} NcmLyricsResult;

typedef struct NcmLyricsFetcherDef {
    char *name;
    char *url_template;
    char *match_regex;

    int32 name_len;
    int32 name_cap;
    int32 url_template_len;
    int32 url_template_cap;
    int32 match_regex_len;
    int32 match_regex_cap;

    enum NcmLyricsFetcherType type;
    bool enabled;
} NcmLyricsFetcherDef;

NCM_ARRAY_DECLARE(ncm_lyrics_fetcher_array,
                  NcmLyricsFetcherArray,
                  NcmLyricsFetcherDef)

typedef struct NcmLyricsFetcherRegistry {
    NcmLyricsFetcherArray fetchers;
} NcmLyricsFetcherRegistry;

typedef CURLcode (*NcmLyricsCurlPerformFn)(NcmBuffer *data,
                                           char *url,
                                           int32 url_len,
                                           char *referer,
                                           int32 referer_len,
                                           bool follow_redirect,
                                           int32 timeout_seconds,
                                           void *user);

typedef CURLcode (*NcmLyricsCurlEscapeFn)(NcmBuffer *out,
                                          char *string,
                                          int32 string_len,
                                          void *user);

void ncm_lyrics_result_init(NcmLyricsResult *result);
void ncm_lyrics_result_destroy(NcmLyricsResult *result);
void ncm_lyrics_result_clear(NcmLyricsResult *result);
bool ncm_lyrics_result_set(NcmLyricsResult *result, bool success,
                           char *text, int32 text_len);

void ncm_lyrics_fetcher_def_init(NcmLyricsFetcherDef *fetcher);
void ncm_lyrics_fetcher_def_destroy(NcmLyricsFetcherDef *fetcher);
bool ncm_lyrics_fetcher_def_copy(NcmLyricsFetcherDef *dest,
                                 NcmLyricsFetcherDef *source);
void ncm_lyrics_fetcher_def_move(NcmLyricsFetcherDef *dest,
                                 NcmLyricsFetcherDef *source);
bool ncm_lyrics_fetcher_def_set_name(NcmLyricsFetcherDef *fetcher,
                                     char *name, int32 name_len);
char *ncm_lyrics_fetcher_name(NcmLyricsFetcherDef *fetcher);
int32 ncm_lyrics_fetcher_name_len(NcmLyricsFetcherDef *fetcher);

void ncm_lyrics_fetcher_registry_init(NcmLyricsFetcherRegistry *registry);
void ncm_lyrics_fetcher_registry_destroy(NcmLyricsFetcherRegistry *registry);
void ncm_lyrics_fetcher_registry_clear(NcmLyricsFetcherRegistry *registry);
NcmLyricsFetcherDef *ncm_lyrics_fetcher_registry_append(
    NcmLyricsFetcherRegistry *registry);
bool ncm_lyrics_fetcher_registry_append_name(
    NcmLyricsFetcherRegistry *registry,
    char *name,
    int32 name_len);
bool ncm_lyrics_fetcher_registry_add_defaults(
    NcmLyricsFetcherRegistry *registry);

bool ncm_lyrics_fetcher_fetch(NcmLyricsFetcherDef *fetcher,
                              NcmLyricsResult *result,
                              char *artist,
                              int32 artist_len,
                              char *title,
                              int32 title_len,
                              NcmSong *song);
bool ncm_lyrics_fetcher_build_url(NcmLyricsFetcherDef *fetcher,
                                  NcmBuffer *url,
                                  char *artist,
                                  int32 artist_len,
                                  char *title,
                                  int32 title_len);
void ncm_lyrics_cleanup_html(NcmBuffer *out, char *data, int32 data_len);

void ncm_lyrics_fetcher_set_io_for_tests(NcmLyricsCurlPerformFn perform,
                                         NcmLyricsCurlEscapeFn escape,
                                         void *user);

NCM_EXTERN_C_END

#endif /* NCMPCPP_LYRICS_FETCHER_H */
