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

enum LyricsFetcherType {
    NCM_LYRICS_FETCHER_UNKNOWN,
#define NCM_LYRICS_FETCHER_ENUM(NAME, DISPLAY_NAME) \
    NCM_LYRICS_FETCHER_##NAME,
    NCM_LYRICS_FETCHER_LIST(NCM_LYRICS_FETCHER_ENUM)
#undef NCM_LYRICS_FETCHER_ENUM
    NCM_LYRICS_FETCHER_LAST,
};

#undef NCM_LYRICS_FETCHER_LIST

typedef struct LyricsResult {
    char *text;
    int32 text_len;
    bool success;
} LyricsResult;

typedef struct LyricsFetcherDef {
    char *name;

    int32 name_len;

    enum LyricsFetcherType type;
    bool enabled;
} LyricsFetcherDef;

NCM_ARRAY_DECLARE_TYPE(LyricsFetcherArray, LyricsFetcherDef)
NCM_ARRAY_DECLARE_CLEAR(ncm_lyrics_fetcher_array, LyricsFetcherArray)
NCM_ARRAY_DECLARE_DESTROY(ncm_lyrics_fetcher_array, LyricsFetcherArray)
NCM_ARRAY_DECLARE_RESERVE(ncm_lyrics_fetcher_array, LyricsFetcherArray)
NCM_ARRAY_DECLARE_APPEND(ncm_lyrics_fetcher_array, LyricsFetcherArray,
                         LyricsFetcherDef)

typedef struct LyricsFetcherRegistry {
    LyricsFetcherArray fetchers;
} LyricsFetcherRegistry;

typedef int32 (LyricsCurlPerformFn)(StrBuilder *data,
                                    char *url, int32 url_len,
                                    char *referer, int32 referer_len,
                                    bool follow_redirect, int32 timeout_seconds,
                                    void *user);

void ncm_lyrics_result_destroy(LyricsResult *);
void ncm_lyrics_result_clear(LyricsResult *);
int32 ncm_lyrics_result_set(LyricsResult *, bool, char *, int32);

void ncm_lyrics_fetcher_def_destroy(LyricsFetcherDef *);
int32 ncm_lyrics_fetcher_def_set_name(LyricsFetcherDef *, char *, int32);
char *ncm_lyrics_fetcher_name(LyricsFetcherDef *);
int32 ncm_lyrics_fetcher_name_len(LyricsFetcherDef *);

void ncm_lyrics_fetcher_registry_destroy(LyricsFetcherRegistry *);
void ncm_lyrics_fetcher_registry_clear(LyricsFetcherRegistry *);
LyricsFetcherDef *ncm_lyrics_fetcher_registry_append(
    LyricsFetcherRegistry *);
int32 ncm_lyrics_fetcher_registry_append_name(LyricsFetcherRegistry *,
                                              char *, int32);

int32 ncm_lyrics_fetcher_fetch(LyricsFetcherDef *, LyricsResult *,
                               char *artist, int32 artist_len, char *title,
                               int32 title_len);
int32 ncm_lyrics_fetcher_build_url(LyricsFetcherDef *, StrBuilder *,
                                   char *artist, int32 artist_len, char *title,
                                   int32 title_len);
void ncm_lyrics_cleanup_html(StrBuilder *, char *, int32);

#endif /* LYRICS_FETCHER_H */
