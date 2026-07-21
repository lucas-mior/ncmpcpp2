#if !defined(NCMPCPP_LYRICS_FETCHER_H)
#define NCMPCPP_LYRICS_FETCHER_H

#include "config.h"

#include <stdbool.h>

#include <curl/curl.h>

#include "c/ncm_array.h"
#include "c/ncm_defs.h"

#define NCM_LYRICS_FETCHER_LIST(X) \
    X(AMALGAMA, "amalgama-lab.com") \
    X(AZLYRICS, "azlyrics.com") \
    X(GENIUS, "genius.com") \
    X(LETRASMUS, "letras.mus.br") \
    X(LACOCCINELLE, "lacoccinelle.net") \
    X(MUSICA, "musica.com") \
    X(PAROLES, "paroles.net") \
    X(MUSIXMATCH, "musixmatch.com") \
    X(TEKSTOWO, "tekstowo.pl") \
    X(VAGALUME, "vagalume.com.br") \
    X(INTERNET, "the Internet")


enum NcmLyricsFetcherType {
    NCM_LYRICS_FETCHER_UNKNOWN,
#define NCM_LYRICS_FETCHER_ENUM(NAME, DISPLAY_NAME) \
    NCM_LYRICS_FETCHER_##NAME,
    NCM_LYRICS_FETCHER_LIST(NCM_LYRICS_FETCHER_ENUM)
#undef NCM_LYRICS_FETCHER_ENUM
    NCM_LYRICS_FETCHER_LAST,
};

#undef NCM_LYRICS_FETCHER_LIST

typedef struct NcmLyricsResult {
    char *text;
    int32 text_len;
    int32 text_cap;
    bool success;
} NcmLyricsResult;

typedef struct NcmLyricsFetcherDef {
    char *name;

    int32 name_len;
    int32 name_cap;

    enum NcmLyricsFetcherType type;
    bool enabled;
} NcmLyricsFetcherDef;

NCM_ARRAY_DECLARE_TYPE(NcmLyricsFetcherArray, NcmLyricsFetcherDef)
NCM_ARRAY_DECLARE_INIT(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DECLARE_CLEAR(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DECLARE_APPEND(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray,
                         NcmLyricsFetcherDef)

typedef struct NcmLyricsFetcherRegistry {
    NcmLyricsFetcherArray fetchers;
} NcmLyricsFetcherRegistry;

typedef CURLcode (*NcmLyricsCurlPerformFn)(StrBuilder *data, char *url,
                                           int32 url_len, char *referer,
                                           int32 referer_len,
                                           bool follow_redirect,
                                           int32 timeout_seconds, void *user);

void ncm_lyrics_result_init(NcmLyricsResult *result);
void ncm_lyrics_result_destroy(NcmLyricsResult *result);
void ncm_lyrics_result_clear(NcmLyricsResult *result);
bool ncm_lyrics_result_set(NcmLyricsResult *result, bool success, char *text,
                           int32 text_len);

void ncm_lyrics_fetcher_def_init(NcmLyricsFetcherDef *fetcher);
void ncm_lyrics_fetcher_def_destroy(NcmLyricsFetcherDef *fetcher);
bool ncm_lyrics_fetcher_def_set_name(NcmLyricsFetcherDef *fetcher, char *name,
                                     int32 name_len);
char *ncm_lyrics_fetcher_name(NcmLyricsFetcherDef *fetcher);
int32 ncm_lyrics_fetcher_name_len(NcmLyricsFetcherDef *fetcher);

void ncm_lyrics_fetcher_registry_init(NcmLyricsFetcherRegistry *registry);
void ncm_lyrics_fetcher_registry_destroy(NcmLyricsFetcherRegistry *registry);
void ncm_lyrics_fetcher_registry_clear(NcmLyricsFetcherRegistry *registry);
NcmLyricsFetcherDef *
ncm_lyrics_fetcher_registry_append(NcmLyricsFetcherRegistry *registry);
bool ncm_lyrics_fetcher_registry_append_name(NcmLyricsFetcherRegistry *registry,
                                             char *name, int32 name_len);

bool ncm_lyrics_fetcher_fetch(NcmLyricsFetcherDef *fetcher,
                              NcmLyricsResult *result, char *artist,
                              int32 artist_len, char *title, int32 title_len);
bool ncm_lyrics_fetcher_build_url(NcmLyricsFetcherDef *fetcher, StrBuilder *url,
                                  char *artist, int32 artist_len, char *title,
                                  int32 title_len);
void ncm_lyrics_cleanup_html(StrBuilder *out, char *data, int32 data_len);

#endif /* NCMPCPP_LYRICS_FETCHER_H */
