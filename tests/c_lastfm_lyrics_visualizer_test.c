#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <mpd/tag.h>

#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "lyrics_fetcher.h"
#include "screens/nc_lastfm.h"
#include "screens/nc_lyrics.h"
#include "screens/nc_visualizer.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

typedef struct TestLyricsIo {
    CURLcode code;
    char *response;
    int32 response_len;
} TestLyricsIo;

static CURLcode test_perform(NcmBuffer *data, char *url, int32 url_len,
                             char *referer, int32 referer_len,
                             bool follow_redirect, int32 timeout_seconds,
                             void *user);
static CURLcode test_escape(NcmBuffer *out, char *string, int32 string_len,
                            void *user);
static void test_lastfm_screen_result_storage(void);
static void test_lyrics_filename_and_fetcher_toggle(void);
static void test_lyrics_background_queue_cleanup(void);
static void test_visualizer_sample_buffers(void);

static CURLcode
test_perform(NcmBuffer *data, char *url, int32 url_len,
             char *referer, int32 referer_len, bool follow_redirect,
             int32 timeout_seconds, void *user) {
    TestLyricsIo *io;

    (void)url;
    (void)url_len;
    (void)referer;
    (void)referer_len;
    (void)follow_redirect;
    (void)timeout_seconds;
    io = user;
    ncm_buffer_clear(data);
    if ((io->code == CURLE_OK) && (io->response != NULL)) {
        ncm_buffer_append(data, io->response, io->response_len);
    }
    return io->code;
}

static CURLcode
test_escape(NcmBuffer *out, char *string, int32 string_len, void *user) {
    (void)user;
    ncm_buffer_clear(out);
    for (int32 i = 0; i < string_len; i += 1) {
        if (string[i] == ' ') {
            ncm_buffer_append_byte(out, '+');
        } else {
            ncm_buffer_append_byte(out, string[i]);
        }
    }
    return CURLE_OK;
}

static void
test_lastfm_screen_result_storage(void) {
    NativeLastfmScreen screen = {0};
    NcmLastfmResult *result;

    native_lastfm_screen_init(&screen, 0, 80, 0, 24,
                              nc_color_default(), nc_border_none(), 1);
    result = native_lastfm_screen_result(&screen);
    assert(!result->success);
    assert(ncm_string_equal(native_lastfm_screen_title(&screen),
                            native_lastfm_screen_title_len(&screen),
                            LIT_ARGS("Last.fm")));
    assert(ncm_lastfm_result_set(result, true, LIT_ARGS("Artist info")));
    assert(result->success);
    assert(ncm_string_equal(result->text, result->text_len,
                            LIT_ARGS("Artist info")));
    native_lastfm_screen_destroy(&screen);
    return;
}

static void
test_lyrics_filename_and_fetcher_toggle(void) {
    NativeLyricsScreen screen = {0};
    NcmLyricsFetcherRegistry registry;
    NcmLyricsFetcherDef *fetcher;
    NcmSong song;
    NcmBuffer *filename;

    native_lyrics_screen_init(&screen, 0, 80, 0, 24,
                              nc_color_default(), nc_border_none(), 1);
    ncm_lyrics_fetcher_registry_init(&registry);
    ncm_song_init(&song);

    assert(ncm_song_set_uri(&song, LIT_ARGS("Music/a.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist A")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Title B")));
    assert(native_lyrics_screen_build_filename(&screen,
                                               &song,
                                               LIT_ARGS("/music"),
                                               LIT_ARGS("/lyrics"),
                                               false,
                                               true));
    filename = native_lyrics_screen_filename(&screen);
    assert(ncm_string_equal(filename->data, filename->len,
                            LIT_ARGS("/lyrics/Artist A - Title B.txt")));

    assert(ncm_lyrics_fetcher_registry_append_name(&registry,
                                                   LIT_ARGS("plyrics")));
    assert(ncm_lyrics_fetcher_registry_append_name(&registry,
                                                   LIT_ARGS("azlyrics")));
    fetcher = native_lyrics_screen_toggle_fetcher(&screen, &registry);
    assert(fetcher == &registry.fetchers.items[0]);
    fetcher = native_lyrics_screen_toggle_fetcher(&screen, &registry);
    assert(fetcher == &registry.fetchers.items[1]);
    fetcher = native_lyrics_screen_toggle_fetcher(&screen, &registry);
    assert(fetcher == NULL);

    ncm_song_destroy(&song);
    ncm_lyrics_fetcher_registry_destroy(&registry);
    native_lyrics_screen_destroy(&screen);
    return;
}

static void
test_lyrics_background_queue_cleanup(void) {
    NativeLyricsScreen screen = {0};
    NcmSong song;
    NcmError error = {0};

    native_lyrics_screen_init(&screen, 0, 80, 0, 24,
                              nc_color_default(), nc_border_none(), 1);
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("a.flac")));
    assert(ncm_song_add_tag(&song, MPD_TAG_ARTIST, LIT_ARGS("Artist")));
    assert(ncm_song_add_tag(&song, MPD_TAG_TITLE, LIT_ARGS("Title")));
    assert(native_lyrics_screen_fetch_in_background(&screen,
                                                    &song,
                                                    true,
                                                    &error));
    assert(native_lyrics_screen_pending_jobs(&screen)
           + native_lyrics_screen_queued_count(&screen) >= 1);
    native_lyrics_screen_stop_downloads(&screen);
    ncm_song_destroy(&song);
    native_lyrics_screen_destroy(&screen);
    return;
}

static void
test_visualizer_sample_buffers(void) {
    NativeVisualizerScreen screen = {0};
    int16 samples[8] = {
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
    };
    int16 dest[8] = {0};
    int32 count;

    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  30, true);
    assert(native_visualizer_clamp_sample(50000) == 32767);
    assert(native_visualizer_clamp_sample(-50000) == -32768);
    assert(native_visualizer_screen_requested_samples(&screen) > 0);
    assert(native_visualizer_screen_push_samples(&screen, samples, 8));
    count = native_visualizer_screen_take_render_samples(&screen,
                                                         dest,
                                                         8);
    assert(count == 8);
    assert(dest[0] == 1);
    assert(native_visualizer_screen_split_stereo(&screen, dest, count) == 4);
    assert(native_visualizer_screen_left_len(&screen) == 4);
    assert(native_visualizer_screen_right_len(&screen) == 4);
    assert(native_visualizer_screen_left_data(&screen)[1] == 3);
    assert(native_visualizer_screen_right_data(&screen)[1] == 4);
    native_visualizer_screen_toggle_type(&screen);
    assert(native_visualizer_screen_type(&screen)
           == NATIVE_VISUALIZER_WAVE_FILLED);
    native_visualizer_screen_destroy(&screen);
    return;
}

int
main(void) {
    TestLyricsIo io;

    io.code = CURLE_OK;
    io.response = (char *)"lyrics";
    io.response_len = STRLIT_LEN("lyrics");
    ncm_lyrics_fetcher_set_io_for_tests(test_perform, test_escape, &io);
    test_lastfm_screen_result_storage();
    test_lyrics_filename_and_fetcher_toggle();
    test_lyrics_background_queue_cleanup();
    test_visualizer_sample_buffers();
    ncm_lyrics_fetcher_set_io_for_tests(NULL, NULL, NULL);
    return EXIT_SUCCESS;
}
