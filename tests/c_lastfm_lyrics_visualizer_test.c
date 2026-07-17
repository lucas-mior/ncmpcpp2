#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <pthread.h>

#include <mpd/tag.h>

#include "c/ncm_base.h"
#include "c/ncm_mpd_connection.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "lastfm_service.h"
#include "lyrics_fetcher.h"
#include "screens/nc_lastfm.h"
#include "screens/nc_lyrics.h"
#include "screens/nc_visualizer.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)
#define TEST_LASTFM_MAX_CALLS 4

typedef struct TestLyricsIo {
    CURLcode code;
    char *response;
    int32 response_len;
} TestLyricsIo;

typedef struct TestLastfmIo {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    NcmBuffer urls[TEST_LASTFM_MAX_CALLS];
    char *responses[TEST_LASTFM_MAX_CALLS];
    int32 response_lens[TEST_LASTFM_MAX_CALLS];
    CURLcode codes[TEST_LASTFM_MAX_CALLS];
    int32 calls;
    bool block[TEST_LASTFM_MAX_CALLS];
    bool release[TEST_LASTFM_MAX_CALLS];
} TestLastfmIo;

typedef struct TestVisualizerDataSourceIo {
    NcmBuffer open_location;
    NcmBuffer open_port;
    int64 read_results[8];
    int32 read_results_len;
    int32 read_result_index;
    int32 fifo_fd;
    int32 udp_fd;
    int32 fifo_open_calls;
    int32 udp_open_calls;
    int32 read_calls;
    int32 close_calls;
    int32 close_fd;
    int32 get_outputs_calls;
    bool get_outputs_ok;
} TestVisualizerDataSourceIo;

static CURLcode test_perform(NcmBuffer *data, char *url, int32 url_len,
                             char *referer, int32 referer_len,
                             bool follow_redirect, int32 timeout_seconds,
                             void *user);
static CURLcode test_escape(NcmBuffer *out, char *string, int32 string_len,
                            void *user);
static CURLcode test_lastfm_perform(NcmBuffer *data, char *url,
                                    int32 url_len, char *referer,
                                    int32 referer_len,
                                    bool follow_redirect,
                                    int32 timeout_seconds, void *user);
static CURLcode test_lastfm_escape(NcmBuffer *out, char *string,
                                   int32 string_len, void *user);
static void test_lastfm_io_init(TestLastfmIo *io);
static void test_lastfm_io_destroy(TestLastfmIo *io);
static void test_lastfm_io_wait_calls(TestLastfmIo *io, int32 calls);
static void test_lastfm_io_release(TestLastfmIo *io, int32 index);
static int32 test_lastfm_io_calls(TestLastfmIo *io);
static bool test_buffer_contains(NcmBuffer *buffer, char *needle,
                                 int32 needle_len);
static int32 test_find_literal(char *data, int32 data_len,
                               char *needle, int32 needle_len);
static bool test_buffer_has_format(NcBuffer *buffer, int32 position,
                                   enum NcFormat format);
static bool test_buffer_has_formatted_color(NcBuffer *buffer,
                                            int32 position);
static bool test_buffer_has_color(NcBuffer *buffer, int32 position,
                                  NcColor color);
static void test_wait_for_lastfm_completed(NativeLastfmScreen *screen,
                                           int32 completed);
static void test_lastfm_service_parsing_with_fake_io(void);
static void test_lastfm_duplicate_artist_requests(void);
static void test_lastfm_stale_result_suppression(void);
static void test_lastfm_success_rendering_properties(void);
static void test_lastfm_error_rendering_properties(void);
static void test_lastfm_screen_result_storage(void);
static void test_lyrics_filename_and_fetcher_toggle(void);
static void test_lyrics_background_queue_cleanup(void);
static void test_visualizer_sample_buffers(void);
static void test_visualizer_rendering_state(void);
static void test_visualizer_auto_scale(void);
static void test_visualizer_sample_consumption(void);
static void test_visualizer_resource_lifecycle(void);
static void test_visualizer_data_source_parsing(void);
static void test_visualizer_data_source_hooks(void);
static void test_visualizer_find_output_id(void);
static void test_visualizer_io_init(TestVisualizerDataSourceIo *io);
static void test_visualizer_io_destroy(TestVisualizerDataSourceIo *io);
static NativeVisualizerDataSourceHooks test_visualizer_hooks(
    TestVisualizerDataSourceIo *io);
static int32 test_visualizer_open_fifo(void *user, char *location,
                                       int32 location_len);
static int32 test_visualizer_open_udp(void *user, char *location,
                                      int32 location_len, char *port,
                                      int32 port_len);
static int64 test_visualizer_read_source(void *user, int32 fd,
                                         void *buffer,
                                         int64 buffer_size);
static void test_visualizer_close_source(void *user, int32 fd);
static bool test_visualizer_get_outputs(void *user,
                                        NcmMpdOutputList *outputs,
                                        NcmError *error);

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

static CURLcode
test_lastfm_perform(NcmBuffer *data, char *url, int32 url_len,
                    char *referer, int32 referer_len,
                    bool follow_redirect, int32 timeout_seconds,
                    void *user) {
    TestLastfmIo *io;
    char *response;
    int32 response_len;
    int32 index;
    CURLcode code;

    (void)referer;
    (void)referer_len;
    (void)follow_redirect;
    (void)timeout_seconds;
    io = user;
    ncm_buffer_clear(data);

    pthread_mutex_lock(&io->mutex);
    index = io->calls;
    assert(index < TEST_LASTFM_MAX_CALLS);
    io->calls += 1;
    ncm_buffer_clear(&io->urls[index]);
    ncm_buffer_append(&io->urls[index], url, url_len);
    pthread_cond_broadcast(&io->cond);
    while (io->block[index] && !io->release[index]) {
        pthread_cond_wait(&io->cond, &io->mutex);
    }
    code = io->codes[index];
    response = io->responses[index];
    response_len = io->response_lens[index];
    pthread_mutex_unlock(&io->mutex);

    if ((code == CURLE_OK) && (response != NULL)) {
        ncm_buffer_append(data, response, response_len);
    }
    return code;
}

static CURLcode
test_lastfm_escape(NcmBuffer *out, char *string, int32 string_len,
                   void *user) {
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
test_lastfm_io_init(TestLastfmIo *io) {
    *io = (TestLastfmIo){0};
    pthread_mutex_init(&io->mutex, NULL);
    pthread_cond_init(&io->cond, NULL);
    for (int32 i = 0; i < TEST_LASTFM_MAX_CALLS; i += 1) {
        ncm_buffer_init(&io->urls[i]);
        io->codes[i] = CURLE_OK;
    }
    return;
}

static void
test_lastfm_io_destroy(TestLastfmIo *io) {
    for (int32 i = 0; i < TEST_LASTFM_MAX_CALLS; i += 1) {
        ncm_buffer_destroy(&io->urls[i]);
    }
    pthread_cond_destroy(&io->cond);
    pthread_mutex_destroy(&io->mutex);
    return;
}

static void
test_lastfm_io_wait_calls(TestLastfmIo *io, int32 calls) {
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 1000000,
    };

    for (int32 i = 0; i < 5000; i += 1) {
        if (test_lastfm_io_calls(io) >= calls) {
            return;
        }
        nanosleep(&delay, NULL);
    }
    assert(false);
    return;
}

static void
test_lastfm_io_release(TestLastfmIo *io, int32 index) {
    pthread_mutex_lock(&io->mutex);
    assert((index >= 0) && (index < TEST_LASTFM_MAX_CALLS));
    io->release[index] = true;
    pthread_cond_broadcast(&io->cond);
    pthread_mutex_unlock(&io->mutex);
    return;
}

static int32
test_lastfm_io_calls(TestLastfmIo *io) {
    int32 result;

    pthread_mutex_lock(&io->mutex);
    result = io->calls;
    pthread_mutex_unlock(&io->mutex);
    return result;
}

static bool
test_buffer_contains(NcmBuffer *buffer, char *needle, int32 needle_len) {
    return test_find_literal(buffer->data, buffer->len,
                             needle, needle_len) >= 0;
}

static int32
test_find_literal(char *data, int32 data_len,
                  char *needle, int32 needle_len) {
    if ((data == NULL) || (needle == NULL) || (needle_len <= 0)) {
        return -1;
    }
    for (int32 i = 0; i + needle_len <= data_len; i += 1) {
        if (ncm_string_starts_with(data + i, data_len - i,
                                   needle, needle_len)) {
            return i;
        }
    }
    return -1;
}

static bool
test_buffer_has_format(NcBuffer *buffer, int32 position,
                       enum NcFormat format) {
    NcBufferProperty *properties;
    int32 count;

    properties = nc_buffer_properties(buffer);
    count = nc_buffer_property_count(buffer);
    for (int32 i = 0; i < count; i += 1) {
        if ((properties[i].type == NC_BUFFER_PROPERTY_FORMAT)
            && (properties[i].position == position)
            && (properties[i].value.format == format)) {
            return true;
        }
    }
    return false;
}

static bool
test_buffer_has_formatted_color(NcBuffer *buffer, int32 position) {
    NcBufferProperty *properties;
    int32 count;

    properties = nc_buffer_properties(buffer);
    count = nc_buffer_property_count(buffer);
    for (int32 i = 0; i < count; i += 1) {
        if ((properties[i].type == NC_BUFFER_PROPERTY_FORMATTED_COLOR)
            && (properties[i].position == position)) {
            return true;
        }
    }
    return false;
}

static bool
test_buffer_has_color(NcBuffer *buffer, int32 position, NcColor color) {
    NcBufferProperty *properties;
    int32 count;

    properties = nc_buffer_properties(buffer);
    count = nc_buffer_property_count(buffer);
    for (int32 i = 0; i < count; i += 1) {
        if ((properties[i].type == NC_BUFFER_PROPERTY_COLOR)
            && (properties[i].position == position)
            && nc_color_equal(properties[i].value.color, color)) {
            return true;
        }
    }
    return false;
}

static void
test_wait_for_lastfm_completed(NativeLastfmScreen *screen,
                               int32 completed) {
    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 1000000,
    };

    for (int32 i = 0; i < 5000; i += 1) {
        if (native_lastfm_screen_completed_jobs(screen) >= completed) {
            return;
        }
        nanosleep(&delay, NULL);
    }
    assert(false);
    return;
}

static void
test_lastfm_service_parsing_with_fake_io(void) {
    char *xml =
        "<lfm status=\"ok\"><artist>"
        "<url>https://last.fm/music/Test+Artist</url>"
        "<bio><content> Bio &amp; <b>stuff</b>. </content></bio>"
        "<similar><artist><name>Similar A</name>"
        "<url>https://last.fm/music/Similar+A</url>"
        "</artist></similar>"
        "<tags><tag><name>Rock &amp; Roll</name>"
        "<url>https://last.fm/tag/rock</url></tag></tags>"
        "</artist></lfm>";
    char *expected =
        "Bio & stuff.\n\nSimilar artists:\n"
        "\n * Similar A (https://last.fm/music/Similar+A)"
        "\n\nSimilar tags:\n"
        "\n * Rock & Roll (https://last.fm/tag/rock)"
        "\n\nhttps://last.fm/music/Test+Artist";
    TestLastfmIo io;
    NcmLastfmService service;
    NcmLastfmResult result;

    test_lastfm_io_init(&io);
    io.responses[0] = xml;
    io.response_lens[0] = (int32)strlen(xml);
    ncm_lastfm_service_set_io_for_tests(test_lastfm_perform,
                                        test_lastfm_escape,
                                        &io);

    ncm_lastfm_service_init(&service);
    ncm_lastfm_result_init(&result);
    assert(ncm_lastfm_artist_info_init(&service,
                                       LIT_ARGS("Test Artist"),
                                       LIT_ARGS("pt BR")));
    assert(ncm_lastfm_service_fetch(&service, &result));
    assert(result.success);
    assert(ncm_string_equal(result.text, result.text_len,
                            expected, (int32)strlen(expected)));
    assert(test_buffer_contains(&io.urls[0],
                                LIT_ARGS("artist=Test+Artist")));
    assert(test_buffer_contains(&io.urls[0], LIT_ARGS("lang=pt+BR")));

    ncm_lastfm_result_destroy(&result);
    ncm_lastfm_service_destroy(&service);
    ncm_lastfm_service_set_io_for_tests(NULL, NULL, NULL);
    test_lastfm_io_destroy(&io);
    return;
}

static void
test_lastfm_duplicate_artist_requests(void) {
    char *xml =
        "<lfm status=\"ok\"><artist>"
        "<url>https://last.fm/music/Dup</url>"
        "<bio><content>Duplicate bio</content></bio>"
        "</artist></lfm>";
    NativeLastfmScreen screen = {0};
    TestLastfmIo io;
    NcmError error = {0};

    test_lastfm_io_init(&io);
    io.responses[0] = xml;
    io.response_lens[0] = (int32)strlen(xml);
    io.block[0] = true;
    ncm_lastfm_service_set_io_for_tests(test_lastfm_perform,
                                        test_lastfm_escape,
                                        &io);

    native_lastfm_screen_init(&screen, 0, 80, 0, 24,
                              nc_color_default(), nc_border_none(), 1);
    assert(native_lastfm_screen_queue_artist_info(&screen,
                                                  LIT_ARGS("Dup"),
                                                  LIT_ARGS(""),
                                                  &error));
    test_lastfm_io_wait_calls(&io, 1);
    assert(native_lastfm_screen_has_service(&screen));
    assert(ncm_string_equal(native_lastfm_screen_title(&screen),
                            native_lastfm_screen_title_len(&screen),
                            LIT_ARGS("Artist info")));
    assert(native_lastfm_screen_queue_artist_info(&screen,
                                                  LIT_ARGS("Dup"),
                                                  LIT_ARGS(""),
                                                  &error));
    assert(test_lastfm_io_calls(&io) == 1);

    test_lastfm_io_release(&io, 0);
    test_wait_for_lastfm_completed(&screen, 1);
    assert(native_lastfm_screen_dispatch_jobs(&screen) == 1);
    assert(native_lastfm_screen_result(&screen)->success);
    assert(ncm_string_equal(native_lastfm_screen_result(&screen)->text,
                            native_lastfm_screen_result(&screen)->text_len,
                            LIT_ARGS("Duplicate bio\n\n"
                                     "https://last.fm/music/Dup")));

    native_lastfm_screen_destroy(&screen);
    ncm_lastfm_service_set_io_for_tests(NULL, NULL, NULL);
    test_lastfm_io_destroy(&io);
    return;
}

static void
test_lastfm_stale_result_suppression(void) {
    char *one_xml =
        "<lfm status=\"ok\"><artist>"
        "<url>https://last.fm/music/One</url>"
        "<bio><content>One bio</content></bio>"
        "</artist></lfm>";
    char *two_xml =
        "<lfm status=\"ok\"><artist>"
        "<url>https://last.fm/music/Two</url>"
        "<bio><content>Two bio</content></bio>"
        "</artist></lfm>";
    NativeLastfmScreen screen = {0};
    TestLastfmIo io;
    NcmError error = {0};

    test_lastfm_io_init(&io);
    io.responses[0] = one_xml;
    io.response_lens[0] = (int32)strlen(one_xml);
    io.responses[1] = two_xml;
    io.response_lens[1] = (int32)strlen(two_xml);
    io.block[0] = true;
    io.block[1] = true;
    ncm_lastfm_service_set_io_for_tests(test_lastfm_perform,
                                        test_lastfm_escape,
                                        &io);

    native_lastfm_screen_init(&screen, 0, 80, 0, 24,
                              nc_color_default(), nc_border_none(), 1);
    assert(native_lastfm_screen_queue_artist_info(&screen,
                                                  LIT_ARGS("One"),
                                                  LIT_ARGS(""),
                                                  &error));
    test_lastfm_io_wait_calls(&io, 1);
    assert(native_lastfm_screen_queue_artist_info(&screen,
                                                  LIT_ARGS("Two"),
                                                  LIT_ARGS(""),
                                                  &error));
    test_lastfm_io_release(&io, 0);
    test_wait_for_lastfm_completed(&screen, 1);
    test_lastfm_io_wait_calls(&io, 2);
    assert(native_lastfm_screen_dispatch_jobs(&screen) == 1);
    assert(!native_lastfm_screen_result(&screen)->success);

    test_lastfm_io_release(&io, 1);
    test_wait_for_lastfm_completed(&screen, 1);
    assert(native_lastfm_screen_dispatch_jobs(&screen) == 1);
    assert(native_lastfm_screen_result(&screen)->success);
    assert(ncm_string_equal(native_lastfm_screen_result(&screen)->text,
                            native_lastfm_screen_result(&screen)->text_len,
                            LIT_ARGS("Two bio\n\n"
                                     "https://last.fm/music/Two")));

    native_lastfm_screen_destroy(&screen);
    ncm_lastfm_service_set_io_for_tests(NULL, NULL, NULL);
    test_lastfm_io_destroy(&io);
    return;
}

static void
test_lastfm_success_rendering_properties(void) {
    char *xml =
        "<lfm status=\"ok\"><artist>"
        "<url>https://last.fm/music/Render</url>"
        "<bio><content>Render bio</content></bio>"
        "<similar><artist><name>Other</name>"
        "<url>https://last.fm/music/Other</url></artist></similar>"
        "<tags><tag><name>Tag</name>"
        "<url>https://last.fm/tag/tag</url></tag></tags>"
        "</artist></lfm>";
    char *artist_heading = "\n\nSimilar artists:\n";
    char *tag_heading = "\n\nSimilar tags:\n";
    char *bullet = "\n * ";
    NativeLastfmScreen screen = {0};
    TestLastfmIo io;
    NcmError error = {0};
    int32 artist_pos;
    int32 tag_pos;
    int32 first_bullet_pos;
    int32 second_bullet_pos;

    test_lastfm_io_init(&io);
    io.responses[0] = xml;
    io.response_lens[0] = (int32)strlen(xml);
    ncm_lastfm_service_set_io_for_tests(test_lastfm_perform,
                                        test_lastfm_escape,
                                        &io);

    native_lastfm_screen_init(&screen, 0, 80, 0, 24,
                              nc_color_default(), nc_border_none(), 1);
    assert(native_lastfm_screen_queue_artist_info(&screen,
                                                  LIT_ARGS("Render"),
                                                  LIT_ARGS(""),
                                                  &error));
    test_wait_for_lastfm_completed(&screen, 1);
    assert(native_lastfm_screen_dispatch_jobs(&screen) == 1);
    assert(native_lastfm_screen_result(&screen)->success);
    assert(ncm_string_equal(screen.buffer.data, screen.buffer.len,
                            native_lastfm_screen_result(&screen)->text,
                            native_lastfm_screen_result(&screen)->text_len));

    artist_pos = test_find_literal(screen.buffer.data, screen.buffer.len,
                                   artist_heading,
                                   (int32)strlen(artist_heading));
    tag_pos = test_find_literal(screen.buffer.data, screen.buffer.len,
                                tag_heading,
                                (int32)strlen(tag_heading));
    first_bullet_pos = test_find_literal(screen.buffer.data,
                                         screen.buffer.len,
                                         bullet,
                                         (int32)strlen(bullet));

    assert(artist_pos >= 0);
    assert(tag_pos >= 0);
    assert(first_bullet_pos >= 0);

    second_bullet_pos = test_find_literal(screen.buffer.data
                                          + first_bullet_pos + 1,
                                          screen.buffer.len
                                          - first_bullet_pos - 1,
                                          bullet,
                                          (int32)strlen(bullet));
    second_bullet_pos += first_bullet_pos + 1;
    assert(second_bullet_pos > first_bullet_pos);
    assert(test_buffer_has_format(&screen.buffer,
                                  artist_pos,
                                  NC_FORMAT_BOLD));
    assert(test_buffer_has_format(&screen.buffer,
                                  artist_pos
                                  + (int32)strlen(artist_heading),
                                  NC_FORMAT_NO_BOLD));
    assert(test_buffer_has_format(&screen.buffer,
                                  tag_pos,
                                  NC_FORMAT_BOLD));
    assert(test_buffer_has_format(&screen.buffer,
                                  tag_pos + (int32)strlen(tag_heading),
                                  NC_FORMAT_NO_BOLD));
    assert(test_buffer_has_formatted_color(&screen.buffer,
                                           first_bullet_pos));
    assert(test_buffer_has_formatted_color(&screen.buffer,
                                           second_bullet_pos));

    native_lastfm_screen_destroy(&screen);
    ncm_lastfm_service_set_io_for_tests(NULL, NULL, NULL);
    test_lastfm_io_destroy(&io);
    return;
}

static void
test_lastfm_error_rendering_properties(void) {
    char *xml = "<lfm status=\"failed\"><error>bad</error></lfm>";
    NativeLastfmScreen screen = {0};
    TestLastfmIo io;
    NcmError error = {0};
    NcColor red;

    test_lastfm_io_init(&io);
    io.responses[0] = xml;
    io.response_lens[0] = (int32)strlen(xml);
    ncm_lastfm_service_set_io_for_tests(test_lastfm_perform,
                                        test_lastfm_escape,
                                        &io);

    native_lastfm_screen_init(&screen, 0, 80, 0, 24,
                              nc_color_default(), nc_border_none(), 1);
    assert(native_lastfm_screen_queue_artist_info(&screen,
                                                  LIT_ARGS("Broken"),
                                                  LIT_ARGS(""),
                                                  &error));
    test_wait_for_lastfm_completed(&screen, 1);
    assert(native_lastfm_screen_dispatch_jobs(&screen) == 1);
    assert(!native_lastfm_screen_result(&screen)->success);
    assert(ncm_string_equal(screen.buffer.data, screen.buffer.len,
                            LIT_ARGS(" Invalid response")));

    red = nc_color_make(COLOR_RED, NC_COLOR_CURRENT, false, false);
    assert(test_buffer_has_color(&screen.buffer, 1, red));
    assert(test_buffer_has_color(&screen.buffer,
                                 screen.buffer.len,
                                 nc_color_end()));

    native_lastfm_screen_destroy(&screen);
    ncm_lastfm_service_set_io_for_tests(NULL, NULL, NULL);
    test_lastfm_io_destroy(&io);
    return;
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
test_visualizer_io_init(TestVisualizerDataSourceIo *io) {
    ncm_buffer_init(&io->open_location);
    ncm_buffer_init(&io->open_port);
    io->read_results_len = 0;
    io->read_result_index = 0;
    io->fifo_fd = 71;
    io->udp_fd = 72;
    io->fifo_open_calls = 0;
    io->udp_open_calls = 0;
    io->read_calls = 0;
    io->close_calls = 0;
    io->close_fd = -1;
    io->get_outputs_calls = 0;
    io->get_outputs_ok = true;
    return;
}

static void
test_visualizer_io_destroy(TestVisualizerDataSourceIo *io) {
    ncm_buffer_destroy(&io->open_port);
    ncm_buffer_destroy(&io->open_location);
    return;
}

static NativeVisualizerDataSourceHooks
test_visualizer_hooks(TestVisualizerDataSourceIo *io) {
    NativeVisualizerDataSourceHooks hooks = {0};

    hooks.open_fifo = test_visualizer_open_fifo;
    hooks.open_udp = test_visualizer_open_udp;
    hooks.read_source = test_visualizer_read_source;
    hooks.close_source = test_visualizer_close_source;
    hooks.get_outputs = test_visualizer_get_outputs;
    hooks.user = io;
    return hooks;
}

static int32
test_visualizer_open_fifo(void *user, char *location,
                          int32 location_len) {
    TestVisualizerDataSourceIo *io;

    io = user;
    io->fifo_open_calls += 1;
    ncm_buffer_set(&io->open_location, location, location_len);
    ncm_buffer_clear(&io->open_port);
    return io->fifo_fd;
}

static int32
test_visualizer_open_udp(void *user, char *location,
                         int32 location_len, char *port,
                         int32 port_len) {
    TestVisualizerDataSourceIo *io;

    io = user;
    io->udp_open_calls += 1;
    ncm_buffer_set(&io->open_location, location, location_len);
    ncm_buffer_set(&io->open_port, port, port_len);
    return io->udp_fd;
}

static int64
test_visualizer_read_source(void *user, int32 fd, void *buffer,
                            int64 buffer_size) {
    TestVisualizerDataSourceIo *io;
    int64 result;

    (void)fd;
    (void)buffer;
    (void)buffer_size;
    io = user;
    io->read_calls += 1;
    result = 0;
    if (io->read_result_index < io->read_results_len) {
        result = io->read_results[io->read_result_index];
        io->read_result_index += 1;
    }
    return result;
}

static void
test_visualizer_close_source(void *user, int32 fd) {
    TestVisualizerDataSourceIo *io;

    io = user;
    io->close_calls += 1;
    io->close_fd = fd;
    return;
}

static bool
test_visualizer_get_outputs(void *user, NcmMpdOutputList *outputs,
                            NcmError *error) {
    TestVisualizerDataSourceIo *io;
    NcmMpdOutput output;

    io = user;
    io->get_outputs_calls += 1;
    ncm_error_clear(error);
    if (!io->get_outputs_ok) {
        ncm_error_set(error, 1, LIT_ARGS("test output error"));
        return false;
    }

    ncm_mpd_output_init(&output);
    assert(ncm_mpd_output_set(&output, 3,
                              LIT_ARGS("Speakers"), true));
    assert(ncm_mpd_output_list_append_copy(outputs, &output));
    assert(ncm_mpd_output_set(&output, 42,
                              LIT_ARGS("Visualizer feed"), true));
    assert(ncm_mpd_output_list_append_copy(outputs, &output));
    ncm_mpd_output_destroy(&output);
    return true;
}

static void
test_visualizer_data_source_parsing(void) {
    TestVisualizerDataSourceIo io;
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config = {
        .source_location = "127.0.0.1:5555",
        .output_name = "Visualizer feed",
        .source_location_len = STRLIT_LEN("127.0.0.1:5555"),
        .output_name_len = STRLIT_LEN("Visualizer feed"),
        .fps = 30,
        .spectrum_dft_size = 2,
        .spectrum_gain = 10.0,
        .spectrum_hz_min = 20.0,
        .spectrum_hz_max = 20000.0,
        .stereo = false,
    };

    test_visualizer_io_init(&io);
    config.data_source_hooks = test_visualizer_hooks(&io);
    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    assert(ncm_string_equal(screen.source_location.data,
                            screen.source_location.len,
                            LIT_ARGS("127.0.0.1")));
    assert(ncm_string_equal(screen.source_port.data,
                            screen.source_port.len,
                            LIT_ARGS("5555")));

    native_visualizer_screen_init_data_source(
        &screen, LIT_ARGS("/tmp/mpd:visualizer"));
    assert(ncm_string_equal(screen.source_location.data,
                            screen.source_location.len,
                            LIT_ARGS("/tmp/mpd:visualizer")));
    assert(screen.source_port.len == 0);

    native_visualizer_screen_init_data_source(
        &screen, LIT_ARGS("localhost:"));
    assert(ncm_string_equal(screen.source_location.data,
                            screen.source_location.len,
                            LIT_ARGS("localhost")));
    assert(screen.source_port.len == 0);

    native_visualizer_screen_destroy(&screen);
    test_visualizer_io_destroy(&io);
    return;
}

static void
test_visualizer_data_source_hooks(void) {
    TestVisualizerDataSourceIo io;
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config = {
        .source_location = "localhost:5555",
        .source_location_len = STRLIT_LEN("localhost:5555"),
        .fps = 30,
        .spectrum_dft_size = 2,
        .spectrum_gain = 10.0,
        .spectrum_hz_min = 20.0,
        .spectrum_hz_max = 20000.0,
        .stereo = false,
    };

    test_visualizer_io_init(&io);
    config.data_source_hooks = test_visualizer_hooks(&io);
    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  &config);

    assert(native_visualizer_screen_open_data_source(&screen));
    assert(screen.source_fd == io.udp_fd);
    assert(io.udp_open_calls == 1);
    assert(io.fifo_open_calls == 0);
    assert(ncm_string_equal(io.open_location.data,
                            io.open_location.len,
                            LIT_ARGS("localhost")));
    assert(ncm_string_equal(io.open_port.data,
                            io.open_port.len,
                            LIT_ARGS("5555")));
    assert(native_visualizer_screen_open_data_source(&screen));
    assert(io.udp_open_calls == 1);

    io.read_results[0] = 8;
    io.read_results[1] = 4;
    io.read_results[2] = -1;
    io.read_results_len = 3;
    io.read_result_index = 0;
    assert(native_visualizer_screen_drain_data_source(&screen) == 12);
    assert(io.read_calls == 3);

    native_visualizer_screen_close_data_source(&screen);
    assert(screen.source_fd == -1);
    assert(io.close_calls == 1);
    assert(io.close_fd == io.udp_fd);
    native_visualizer_screen_close_data_source(&screen);
    assert(io.close_calls == 1);

    native_visualizer_screen_init_data_source(
        &screen, LIT_ARGS("/tmp/mpd.fifo"));
    assert(native_visualizer_screen_open_data_source(&screen));
    assert(screen.source_fd == io.fifo_fd);
    assert(io.fifo_open_calls == 1);
    assert(ncm_string_equal(io.open_location.data,
                            io.open_location.len,
                            LIT_ARGS("/tmp/mpd.fifo")));
    assert(io.open_port.len == 0);

    io.read_results[0] = 6;
    io.read_results[1] = 0;
    io.read_results_len = 2;
    io.read_result_index = 0;
    native_visualizer_screen_clear(&screen);
    assert(io.read_calls == 5);

    native_visualizer_screen_destroy(&screen);
    assert(io.close_calls == 2);
    assert(io.close_fd == io.fifo_fd);
    test_visualizer_io_destroy(&io);
    return;
}

static void
test_visualizer_find_output_id(void) {
    char output_name[] = "Visualizer feed";
    TestVisualizerDataSourceIo io;
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config = {
        .source_location = "/tmp/mpd.fifo",
        .output_name = output_name,
        .source_location_len = STRLIT_LEN("/tmp/mpd.fifo"),
        .output_name_len = STRLIT_LEN("Visualizer feed"),
        .fps = 30,
        .spectrum_dft_size = 2,
        .spectrum_gain = 10.0,
        .spectrum_hz_min = 20.0,
        .spectrum_hz_max = 20000.0,
        .stereo = false,
    };

    test_visualizer_io_init(&io);
    config.data_source_hooks = test_visualizer_hooks(&io);
    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    output_name[0] = 'X';

    assert(native_visualizer_screen_find_output_id(&screen));
    assert(screen.output_id == 42);
    assert(io.get_outputs_calls == 1);
    assert(ncm_string_equal(screen.output_name.data,
                            screen.output_name.len,
                            LIT_ARGS("Visualizer feed")));

    native_visualizer_screen_init_data_source(
        &screen, LIT_ARGS("127.0.0.1:5555"));
    assert(native_visualizer_screen_find_output_id(&screen));
    assert(screen.output_id == -1);
    assert(io.get_outputs_calls == 1);

    native_visualizer_screen_destroy(&screen);
    test_visualizer_io_destroy(&io);
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

    NativeVisualizerScreenConfig config = {
        .source_location = "/tmp/test-visualizer.fifo",
        .source_location_len = STRLIT_LEN("/tmp/test-visualizer.fifo"),
        .fps = 30,
        .spectrum_dft_size = 2,
        .spectrum_gain = 10.0,
        .spectrum_hz_min = 20.0,
        .spectrum_hz_max = 20000.0,
        .stereo = true,
    };

    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  &config);
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

static void
test_visualizer_rendering_state(void) {
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config = {
        .source_location = "/tmp/test-visualizer.fifo",
        .source_location_len = STRLIT_LEN("/tmp/test-visualizer.fifo"),
        .fps = 30,
        .spectrum_dft_size = 2,
        .spectrum_gain = 10.0,
        .spectrum_hz_min = 20.0,
        .spectrum_hz_max = 20000.0,
        .visualization_type = NATIVE_VISUALIZER_WAVE,
        .stereo = false,
    };

    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    assert(screen.sample_consumption_rate == 5);
    assert(ncm_sample_buffer_capacity(&screen.incoming_samples) == 22050);
    assert(ncm_sample_buffer_capacity(&screen.buffered_samples) == 22050);
    assert(ncm_sample_buffer_capacity(&screen.rendered_samples) == 15200);
    assert(ncm_sample_buffer_capacity(&screen.left_channel) == 0);
    assert(ncm_sample_buffer_capacity(&screen.right_channel) == 0);
    assert(native_visualizer_screen_requested_samples(&screen) == 2367);

    native_visualizer_screen_set_stereo(&screen, true);
    assert(ncm_sample_buffer_capacity(&screen.incoming_samples) == 44100);
    assert(ncm_sample_buffer_capacity(&screen.buffered_samples) == 44100);
    assert(ncm_sample_buffer_capacity(&screen.rendered_samples) == 30400);
    assert(ncm_sample_buffer_capacity(&screen.left_channel) == 15200);
    assert(ncm_sample_buffer_capacity(&screen.right_channel) == 15200);
    assert(native_visualizer_screen_requested_samples(&screen) == 4734);

    native_visualizer_screen_set_fps(&screen, 60);
    assert(ncm_sample_buffer_capacity(&screen.rendered_samples) == 16000);
    assert(native_visualizer_screen_requested_samples(&screen) == 2367);

    screen.window.width = 81;
    native_visualizer_screen_init_visualization(&screen);
    assert(ncm_sample_buffer_capacity(&screen.rendered_samples) == 16200);

    native_visualizer_screen_set_type(&screen,
                                      NATIVE_VISUALIZER_ELLIPSE);
    assert(ncm_sample_buffer_capacity(&screen.rendered_samples) == 2940);
#if defined(HAVE_FFTW3_H)
    native_visualizer_screen_set_type(&screen,
                                      NATIVE_VISUALIZER_FREQUENCY);
    assert(ncm_sample_buffer_capacity(&screen.rendered_samples)
           == screen.fft.dft_nonzero_size*2);
#endif

    native_visualizer_screen_set_type(&screen, NATIVE_VISUALIZER_WAVE);
    native_visualizer_screen_toggle_type(&screen);
    assert(native_visualizer_screen_type(&screen)
           == NATIVE_VISUALIZER_WAVE_FILLED);
    native_visualizer_screen_toggle_type(&screen);
#if defined(HAVE_FFTW3_H)
    assert(native_visualizer_screen_type(&screen)
           == NATIVE_VISUALIZER_FREQUENCY);
    native_visualizer_screen_toggle_type(&screen);
#endif
    assert(native_visualizer_screen_type(&screen)
           == NATIVE_VISUALIZER_ELLIPSE);
    native_visualizer_screen_toggle_type(&screen);
    assert(native_visualizer_screen_type(&screen)
           == NATIVE_VISUALIZER_WAVE);

    native_visualizer_screen_set_type(
        &screen, NATIVE_VISUALIZER_TYPE_LAST);
    assert(native_visualizer_screen_type(&screen)
           == NATIVE_VISUALIZER_WAVE);
    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_visualizer_auto_scale(void) {
    NativeVisualizerScreen screen = {0};
    int16 samples[3] = {
        1000,
        -2000,
        0,
    };
    int16 loud_samples[2] = {
        -32768,
        32767,
    };
    int16 silent_sample = 0;
    int16 disabled_sample = 1234;
    NativeVisualizerScreenConfig config = {
        .source_location = "/tmp/test-visualizer.fifo",
        .source_location_len = STRLIT_LEN("/tmp/test-visualizer.fifo"),
        .fps = 10,
        .spectrum_dft_size = 2,
        .spectrum_gain = 10.0,
        .spectrum_hz_min = 20.0,
        .spectrum_hz_max = 20000.0,
        .visualization_type = NATIVE_VISUALIZER_WAVE,
        .autoscale = true,
        .stereo = false,
    };

    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    assert(native_visualizer_screen_push_samples(&screen, samples, 3));
    assert(screen.auto_scale_multiplier > 1.09);
    assert(screen.auto_scale_multiplier < 1.11);
    assert(samples[0] == 1100);
    assert(samples[1] == -2200);
    assert(samples[2] == 0);

    screen.auto_scale_multiplier = 2.0;
    native_visualizer_screen_apply_auto_scale(&screen, loud_samples, 2);
    assert(screen.auto_scale_multiplier == 1.0);
    assert(loud_samples[0] == -32768);
    assert(loud_samples[1] == 32767);

    screen.auto_scale_multiplier = 50.0;
    native_visualizer_screen_apply_auto_scale(&screen, &silent_sample, 1);
    assert(screen.auto_scale_multiplier > 50.0);
    assert(silent_sample == 0);

    screen.sample_consumption_rate = 7;
    native_visualizer_screen_reset_auto_scale_multiplier(&screen);
    assert(screen.auto_scale_multiplier == 1.0);
    assert(screen.sample_consumption_rate == 7);

    screen.autoscale = false;
    native_visualizer_screen_apply_auto_scale(&screen, &disabled_sample, 1);
    assert(disabled_sample == 1234);
    assert(screen.auto_scale_multiplier == 1.0);
    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_visualizer_sample_consumption(void) {
    NativeVisualizerScreen screen = {0};
    int16 samples[3000] = {0};
    int16 one_sample = 0;
    int16 dest = 0;
    NativeVisualizerScreenConfig config = {
        .source_location = "/tmp/test-visualizer.fifo",
        .source_location_len = STRLIT_LEN("/tmp/test-visualizer.fifo"),
        .fps = 30,
        .spectrum_dft_size = 2,
        .spectrum_gain = 10.0,
        .spectrum_hz_min = 20.0,
        .spectrum_hz_max = 20000.0,
        .visualization_type = NATIVE_VISUALIZER_WAVE,
        .stereo = false,
    };

    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    for (int32 i = 0; i < 9; i += 1) {
        assert(native_visualizer_screen_push_samples(&screen,
                                                     samples,
                                                     3000));
        assert(native_visualizer_screen_take_render_samples(
                   &screen, &dest, 1) > 0);
    }
    assert(screen.sample_consumption_rate == 6);
    assert(screen.sample_consumption_rate_up_ctr == 0);

    ncm_sample_buffer_clear(&screen.buffered_samples);
    screen.sample_consumption_rate_up_ctr = 3;
    screen.sample_consumption_rate_dn_ctr = 2;
    assert(native_visualizer_screen_take_render_samples(
               &screen, &dest, 1) == 0);
    assert(screen.sample_consumption_rate == 6);
    assert(screen.sample_consumption_rate_up_ctr == 3);
    assert(screen.sample_consumption_rate_dn_ctr == 2);

    screen.sample_consumption_rate_up_ctr = 0;
    screen.sample_consumption_rate_dn_ctr = 0;
    for (int32 i = 0; i < 5; i += 1) {
        assert(native_visualizer_screen_push_samples(&screen,
                                                     &one_sample,
                                                     1));
        assert(native_visualizer_screen_take_render_samples(
                   &screen, &dest, 1) == 1);
    }
    assert(screen.sample_consumption_rate == 5);
    assert(screen.sample_consumption_rate_dn_ctr == 0);
    native_visualizer_screen_destroy(&screen);
    return;
}

static void
test_visualizer_resource_lifecycle(void) {
    char source_location[] = "127.0.0.1:5555";
    char output_name[] = "Visualizer feed";
    NativeVisualizerScreen screen = {0};
    NativeVisualizerScreenConfig config = {
        .source_location = source_location,
        .output_name = output_name,
        .source_location_len = STRLIT_LEN("127.0.0.1:5555"),
        .output_name_len = STRLIT_LEN("Visualizer feed"),
        .fps = 60,
        .spectrum_dft_size = 3,
        .spectrum_gain = 15.0,
        .spectrum_hz_min = 30.0,
        .spectrum_hz_max = 18000.0,
        .stereo = true,
    };
    int32 descriptors[2];
    int32 source_fd;

    assert(pipe(descriptors) == 0);
    source_fd = descriptors[0];
    native_visualizer_screen_init(&screen, 0, 0, 80, 24,
                                  nc_color_default(), nc_border_none(),
                                  &config);
    screen.source_fd = source_fd;
    source_location[0] = 'X';
    output_name[0] = 'X';

    assert(screen.output_id == -1);
    assert(!screen.reset_output);
    assert(screen.source_location.len == STRLIT_LEN("127.0.0.1"));
    assert(ncm_string_equal(screen.source_location.data,
                            screen.source_location.len,
                            LIT_ARGS("127.0.0.1")));
    assert(ncm_string_equal(screen.source_port.data,
                            screen.source_port.len,
                            LIT_ARGS("5555")));
    assert(ncm_string_equal(screen.output_name.data,
                            screen.output_name.len,
                            LIT_ARGS("Visualizer feed")));
    assert(ncm_sample_buffer_capacity(&screen.incoming_samples) == 44100);
    assert(ncm_sample_buffer_capacity(&screen.buffered_samples) == 44100);
    assert(ncm_sample_buffer_capacity(&screen.rendered_samples) == 16000);
    assert(screen.sample_consumption_rate == 5);
    assert(!screen.autoscale);
#if defined(HAVE_FFTW3_H)
    assert(screen.fft.dft_nonzero_size == 20480);
    assert(screen.fft.dft_total_size == 32768);
    assert(screen.fft.results_len == 16385);
    assert(screen.fft.input != NULL);
    assert(screen.fft.output != NULL);
    assert(screen.fft.plan != NULL);
    assert(screen.fft.frequency_magnitudes != NULL);
    assert(screen.fft.dft_frequency_space != NULL);
    assert(screen.fft.bar_heights != NULL);
#endif

    native_visualizer_screen_destroy(&screen);
    errno = 0;
    assert(fcntl(source_fd, F_GETFD) == -1);
    assert(errno == EBADF);
    assert(screen.source_fd == -1);
    assert(screen.source_location.data == NULL);
    assert(screen.source_port.data == NULL);
    assert(screen.output_name.data == NULL);
    assert(screen.incoming_samples.data == NULL);
    assert(screen.rendered_samples.data == NULL);
#if defined(HAVE_FFTW3_H)
    assert(screen.fft.plan == NULL);
    assert(screen.fft.input == NULL);
    assert(screen.fft.output == NULL);
    assert(screen.fft.frequency_magnitudes == NULL);
    assert(screen.fft.dft_frequency_space == NULL);
    assert(screen.fft.bar_heights == NULL);
#endif
    native_visualizer_screen_destroy(&screen);
    close(descriptors[1]);
    return;
}

int
main(void) {
    TestLyricsIo io;

    io.code = CURLE_OK;
    io.response = "lyrics";
    io.response_len = STRLIT_LEN("lyrics");
    ncm_lyrics_fetcher_set_io_for_tests(test_perform, test_escape, &io);
    test_lastfm_service_parsing_with_fake_io();
    test_lastfm_screen_result_storage();
    test_lastfm_duplicate_artist_requests();
    test_lastfm_stale_result_suppression();
    test_lastfm_success_rendering_properties();
    test_lastfm_error_rendering_properties();
    test_lyrics_filename_and_fetcher_toggle();
    test_lyrics_background_queue_cleanup();
    test_visualizer_data_source_parsing();
    test_visualizer_data_source_hooks();
    test_visualizer_find_output_id();
    test_visualizer_sample_buffers();
    test_visualizer_rendering_state();
    test_visualizer_auto_scale();
    test_visualizer_sample_consumption();
    test_visualizer_resource_lifecycle();
    ncm_lyrics_fetcher_set_io_for_tests(NULL, NULL, NULL);
    return EXIT_SUCCESS;
}
