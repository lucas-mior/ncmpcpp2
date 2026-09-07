#if !defined(LYRICS_FETCHER_H)
#define LYRICS_FETCHER_H

#include "cbase.h"

#include <curl/curl.h>

#include "c/ncm_c.h"

#define NCM_LYRICS_FETCHER_LIST(XX)                 \
    XX(AMALGAMA,     "amalgama-lab.com")            \
    XX(AZLYRICS,     "azlyrics.com")                \
    XX(GENIUS,       "genius.com")                  \
    XX(LETRASMUS,    "letras.mus.br")               \
    XX(LACOCCINELLE, "lacoccinelle.net")            \
    XX(MUSICA,       "musica.com")                  \
    XX(PAROLES,      "paroles.net")                 \
    XX(MUSIXMATCH,   "musixmatch.com")              \
    XX(TEKSTOWO,     "tekstowo.pl")                 \
    XX(VAGALUME,     "vagalume.com.br")             \
    XX(INTERNET,     "the Internet")

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
    bool success;
} NcmLyricsResult;

typedef struct NcmLyricsFetcherDef {
    char *name;

    int32 name_len;

    enum NcmLyricsFetcherType type;
    bool enabled;
} NcmLyricsFetcherDef;

NCM_ARRAY_DECLARE_TYPE(NcmLyricsFetcherArray, NcmLyricsFetcherDef)
NCM_ARRAY_DECLARE_CLEAR(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray)
NCM_ARRAY_DECLARE_APPEND(ncm_lyrics_fetcher_array, NcmLyricsFetcherArray,
                         NcmLyricsFetcherDef)

typedef struct NcmLyricsFetcherRegistry {
    NcmLyricsFetcherArray fetchers;
} NcmLyricsFetcherRegistry;

typedef int32 (NcmLyricsCurlPerformFn)(StrBuilder *data, char *url,
                                         int32 url_len, char *referer,
                                         int32 referer_len,
                                         bool follow_redirect,
                                         int32 timeout_seconds, void *user);

void ncm_lyrics_result_destroy(NcmLyricsResult *);
void ncm_lyrics_result_clear(NcmLyricsResult *);
int32 ncm_lyrics_result_set(NcmLyricsResult *, bool, char *, int32);

void ncm_lyrics_fetcher_def_destroy(NcmLyricsFetcherDef *);
int32 ncm_lyrics_fetcher_def_set_name(NcmLyricsFetcherDef *, char *, int32);
char *ncm_lyrics_fetcher_name(NcmLyricsFetcherDef *);
int32 ncm_lyrics_fetcher_name_len(NcmLyricsFetcherDef *);

void ncm_lyrics_fetcher_registry_destroy(NcmLyricsFetcherRegistry *);
void ncm_lyrics_fetcher_registry_clear(NcmLyricsFetcherRegistry *);
NcmLyricsFetcherDef *ncm_lyrics_fetcher_registry_append(
    NcmLyricsFetcherRegistry *);
int32 ncm_lyrics_fetcher_registry_append_name(NcmLyricsFetcherRegistry *,
                                              char *, int32);

int32 ncm_lyrics_fetcher_fetch(NcmLyricsFetcherDef *, NcmLyricsResult *,
                               char *artist, int32 artist_len, char *title,
                               int32 title_len);
int32 ncm_lyrics_fetcher_build_url(NcmLyricsFetcherDef *, StrBuilder *,
                                   char *artist, int32 artist_len, char *title,
                                   int32 title_len);
void ncm_lyrics_cleanup_html(StrBuilder *, char *, int32);

#endif /* LYRICS_FETCHER_H */
