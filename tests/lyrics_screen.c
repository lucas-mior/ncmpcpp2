#if !defined(NCMPCPP_TESTS_LYRICS_SCREEN_C)
#define NCMPCPP_TESTS_LYRICS_SCREEN_C

#define CBASE_IMPLEMENT
#include "cbase.h"
#include "curses.h"

#include <unistd.h>

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7
#define BUTTON4_PRESSED (((uint64)1) << 0)
#define BUTTON5_PRESSED (((uint64)1) << 1)
#define COLS 80

int32 color_set(int16 pair, void *opts);
int32 mvvline(int32 y, int32 x, chtype ch, int32 n);
int32 refresh(void);
int32 standend(void);

static int32 lyrics_test_x;
static int32 lyrics_test_y;
static int32 lyrics_test_print_count;
static int64 lyrics_test_elapsed_ms;
static int32 lyrics_test_player_state;
static StrBuilder lyrics_test_status_message;

#include "c/ncm_error.c"
#include "c/ncm_path.c"
#include "c/ncm_string.c"
#include "c/ncm_fs.c"
#include "c/ncm_regex.c"
#include "c/ncm_charset.c"
#include "c/ncm_lrc.c"
#include "curses/nc_formatted_color.c"
#include "curses/nc_buffer.c"
#include "curses/nc_scrollpad.c"
#include "curses/nc_cyclic_buffer.c"
#include "screens/nc_screen.c"
#include "screens/nc_scrollpad_screen.c"
#include "screens/nc_lyrics.c"

Configuration Config;

static NcmJob lyrics_test_pushed_job;
static bool lyrics_test_has_pushed_job;

int32
prefresh(WINDOW *pad, int32 pminrow, int32 pmincol,
         int32 sminrow, int32 smincol,
         int32 smaxrow, int32 smaxcol) {
    (void)pad;
    (void)pminrow;
    (void)pmincol;
    (void)sminrow;
    (void)smincol;
    (void)smaxrow;
    (void)smaxcol;
    return 0;
}

int32
werase(WINDOW *win) {
    (void)win;
    return 0;
}

int32
wclrtoeol(WINDOW *win) {
    (void)win;
    return 0;
}

int32
mvwhline(WINDOW *win, int32 y, int32 x, chtype ch, int32 n) {
    (void)win;
    (void)y;
    (void)x;
    (void)ch;
    (void)n;
    return 0;
}

int32
color_set(int16 pair, void *opts) {
    (void)pair;
    (void)opts;
    return 0;
}

int32
mvvline(int32 y, int32 x, chtype ch, int32 n) {
    (void)y;
    (void)x;
    (void)ch;
    (void)n;
    return 0;
}

int32
refresh(void) {
    return 0;
}

int32
standend(void) {
    return 0;
}

NcColor
nc_color_make(int16 foreground, int16 background,
              bool is_default, bool is_end) {
    NcColor color;

    color.foreground = foreground;
    color.background = background;
    color.is_default = is_default;
    color.is_end = is_end;
    return color;
}

NcColor
nc_color_default(void) {
    return nc_color_make(0, 0, true, false);
}

NcColor
nc_color_end(void) {
    return nc_color_make(0, 0, false, true);
}

bool
nc_color_equal(NcColor left, NcColor right) {
    return (left.foreground == right.foreground)
           && (left.background == right.background)
           && (left.is_default == right.is_default)
           && (left.is_end == right.is_end);
}

bool
nc_color_is_default(NcColor color) {
    return color.is_default;
}

bool
nc_color_is_end(NcColor color) {
    return color.is_end;
}

bool
nc_color_current_background(NcColor color) {
    (void)color;
    return false;
}

int32
nc_color_pair_number(NcColor color) {
    (void)color;
    return 0;
}

void
nc_window_init(NcWindow *window, int32 start_x, int32 start_y,
               int32 width, int32 height, char *title,
               int32 title_len, NcColor color, NcBorder border) {
    memset64(window, 0, SIZEOF(*window));
    window->start_x = start_x;
    window->start_y = start_y;
    window->width = width;
    window->height = height;
    window->color = color;
    window->base_color = color;
    window->border = border;
    nc_window_set_title(window, title, title_len);
    return;
}

void
nc_window_destroy(NcWindow *window) {
    free2(window->title, window->title_cap);
    ARRAY_FREE(window->color_stack);
    ARRAY_FREE(window->input_queue);
    ARRAY_FREE(window->fd_callbacks);
    memset64(window, 0, SIZEOF(*window));
    return;
}

void
nc_window_adjust_dimensions(NcWindow *window,
                            int32 width, int32 height) {
    window->width = width;
    window->height = height;
    return;
}

void
nc_window_recreate(NcWindow *window, int32 width, int32 height) {
    window->width = width;
    window->height = height;
    lyrics_test_x = 0;
    lyrics_test_y = 0;
    return;
}

void
nc_window_resize(NcWindow *window, int32 new_width, int32 new_height) {
    nc_window_adjust_dimensions(window, new_width, new_height);
    return;
}

void
nc_window_move_to(NcWindow *window, int32 new_x, int32 new_y) {
    window->start_x = new_x;
    window->start_y = new_y;
    return;
}

void
nc_window_set_timeout(NcWindow *window, int32 timeout) {
    window->window_timeout = timeout;
    return;
}

void
nc_window_set_title(NcWindow *window, char *title, int32 title_len) {
    free2(window->title, window->title_cap);
    window->title = NULL;
    window->title_len = 0;
    window->title_cap = 0;
    if ((title == NULL) || (title_len <= 0)) {
        return;
    }
    window->title = malloc2(title_len + 1);
    window->title_len = title_len;
    window->title_cap = title_len + 1;
    memcpy64(window->title, title, title_len);
    window->title[title_len] = '\0';
    return;
}

void
nc_window_apply_format(NcWindow *window, enum NcFormat format) {
    (void)window;
    (void)format;
    return;
}

void
nc_window_push_color(NcWindow *window, NcColor color) {
    (void)window;
    (void)color;
    return;
}

void
nc_window_print_char(NcWindow *window, char ch) {
    lyrics_test_print_count += 1;
    if (ch == '\n') {
        lyrics_test_x = 0;
        lyrics_test_y += 1;
        return;
    }
    if (lyrics_test_x >= window->width) {
        lyrics_test_x = 0;
        lyrics_test_y += 1;
    }
    lyrics_test_x += 1;
    return;
}

void
nc_window_print_data(NcWindow *window, char *data, int32 data_len) {
    for (int32 i = 0; i < data_len; i += 1) {
        nc_window_print_char(window, data[i]);
    }
    return;
}

void
nc_window_go_to_xy(NcWindow *window, int32 x, int32 y) {
    (void)window;
    lyrics_test_x = x;
    lyrics_test_y = y;
    return;
}

int32
nc_window_get_x(NcWindow *window) {
    (void)window;
    return lyrics_test_x;
}

int32
nc_window_get_y(NcWindow *window) {
    (void)window;
    return lyrics_test_y;
}

void
nc_window_refresh_border(NcWindow *window) {
    (void)window;
    return;
}

NcBorder
nc_border_none(void) {
    NcBorder border;

    border.color = nc_color_default();
    border.enabled = false;
    return border;
}

void
ncm_job_queue_init(NcmJobQueue *queue) {
    memset64(queue, 0, SIZEOF(*queue));
    return;
}

bool
ncm_job_queue_start(NcmJobQueue *queue, NcmError *error) {
    (void)queue;
    ncm_error_clear(error);
    return true;
}

bool
ncm_job_queue_push(NcmJobQueue *queue, NcmJob job,
                   NcmError *error) {
    (void)queue;
    ASSERT(!lyrics_test_has_pushed_job);
    lyrics_test_pushed_job = job;
    lyrics_test_has_pushed_job = true;
    ncm_error_clear(error);
    return true;
}

int32
ncm_job_queue_dispatch_completed(NcmJobQueue *queue) {
    (void)queue;
    return 0;
}

void
ncm_job_queue_destroy(NcmJobQueue *queue) {
    (void)queue;
    return;
}

int32
ncm_job_queue_pending_count(NcmJobQueue *queue) {
    (void)queue;
    return 0;
}

int32
ncm_job_queue_completed_count(NcmJobQueue *queue) {
    (void)queue;
    return 0;
}

void
ncm_lyrics_result_init(NcmLyricsResult *result) {
    memset64(result, 0, SIZEOF(*result));
    return;
}

void
ncm_lyrics_result_destroy(NcmLyricsResult *result) {
    free2(result->text, result->text_cap);
    ncm_lyrics_result_init(result);
    return;
}

void
ncm_lyrics_result_clear(NcmLyricsResult *result) {
    free2(result->text, result->text_cap);
    ncm_lyrics_result_init(result);
    return;
}

bool
ncm_lyrics_result_set(NcmLyricsResult *result, bool success,
                      char *text, int32 text_len) {
    ncm_lyrics_result_clear(result);
    result->success = success;
    if ((text == NULL) || (text_len <= 0)) {
        return true;
    }
    result->text = malloc2(text_len + 1);
    result->text_len = text_len;
    result->text_cap = text_len + 1;
    memcpy64(result->text, text, text_len);
    result->text[text_len] = '\0';
    return true;
}

char *
ncm_lyrics_fetcher_name(NcmLyricsFetcherDef *fetcher) {
    if (fetcher == NULL) {
        return NULL;
    }
    return fetcher->name;
}

int32
ncm_lyrics_fetcher_name_len(NcmLyricsFetcherDef *fetcher) {
    if (fetcher == NULL) {
        return 0;
    }
    return fetcher->name_len;
}

bool
ncm_lyrics_fetcher_fetch(NcmLyricsFetcherDef *fetcher,
                         NcmLyricsResult *result, char *artist,
                         int32 artist_len, char *title, int32 title_len) {
    (void)fetcher;
    (void)result;
    (void)artist;
    (void)artist_len;
    (void)title;
    (void)title_len;
    return false;
}

void
ncm_song_init(NcmSong *song) {
    memset64(song, 0, SIZEOF(*song));
    return;
}

void
ncm_song_destroy(NcmSong *song) {
    if (song == NULL) {
        return;
    }
    free2(song->uri, song->uri_len + 1);
    for (int32 i = 0; i < song->tags_len; i += 1) {
        free2(song->tags[i].value, song->tags[i].value_len + 1);
    }
    free2(song->tags, song->tags_cap*SIZEOF(*song->tags));
    ncm_song_init(song);
    return;
}

void
ncm_song_move(NcmSong *dest, NcmSong *source) {
    ncm_song_destroy(dest);
    *dest = *source;
    ncm_song_init(source);
    return;
}

bool
ncm_song_set_uri(NcmSong *song, char *uri, int32 uri_len) {
    free2(song->uri, song->uri_len + 1);
    song->uri = malloc2(uri_len + 1);
    memcpy64(song->uri, uri, uri_len);
    song->uri[uri_len] = '\0';
    song->uri_len = uri_len;
    return true;
}

static bool
lyrics_test_song_reserve_tags(NcmSong *song) {
    int32 old_cap;
    int32 new_cap;

    if (song->tags_len < song->tags_cap) {
        return true;
    }
    old_cap = song->tags_cap;
    if (old_cap <= 0) {
        new_cap = 4;
    } else {
        new_cap = old_cap*2;
    }
    song->tags = realloc2(song->tags, old_cap, new_cap,
                          SIZEOF(*song->tags));
    for (int32 i = old_cap; i < new_cap; i += 1) {
        song->tags[i].value = NULL;
        song->tags[i].value_len = 0;
        song->tags[i].type = MPD_TAG_UNKNOWN;
    }
    song->tags_cap = new_cap;
    return true;
}

bool
ncm_song_add_tag(NcmSong *song, enum mpd_tag_type type,
                 char *value, int32 value_len) {
    NcmSongTag *tag;

    ASSERT(lyrics_test_song_reserve_tags(song));
    tag = &song->tags[song->tags_len];
    tag->value = malloc2(value_len + 1);
    tag->value_len = value_len;
    tag->type = type;
    memcpy64(tag->value, value, value_len);
    tag->value[value_len] = '\0';
    song->tags_len += 1;
    return true;
}

bool
ncm_song_copy(NcmSong *dest, NcmSong *source) {
    ncm_song_destroy(dest);
    if (source->uri) {
        ASSERT(ncm_song_set_uri(dest, source->uri, source->uri_len));
    }
    for (int32 i = 0; i < source->tags_len; i += 1) {
        ASSERT(ncm_song_add_tag(dest, source->tags[i].type,
                                source->tags[i].value,
                                source->tags[i].value_len));
    }
    dest->duration = source->duration;
    dest->position = source->position;
    dest->id = source->id;
    dest->priority = source->priority;
    dest->last_modified = source->last_modified;
    return true;
}

bool
ncm_song_empty(NcmSong *song) {
    return (song == NULL) || (song->uri_len <= 0);
}

bool
ncm_song_tag_view(NcmSong *song, enum mpd_tag_type tag,
                  int32 idx, NcmStringView *view) {
    int32 found;

    found = 0;
    for (int32 i = 0; i < song->tags_len; i += 1) {
        if (song->tags[i].type != tag) {
            continue;
        }
        if (found == idx) {
            ncm_string_view_set(view,
                                song->tags[i].value,
                                song->tags[i].value_len);
            return true;
        }
        found += 1;
    }
    ncm_string_view_clear(view);
    return false;
}

bool
ncm_song_uri_view(NcmSong *song, int32 idx, NcmStringView *view) {
    if ((idx != 0) || (song == NULL) || (song->uri_len <= 0)) {
        ncm_string_view_clear(view);
        return false;
    }
    ncm_string_view_set(view, song->uri, song->uri_len);
    return true;
}

bool
ncm_song_name_view(NcmSong *song, int32 idx, NcmStringView *view) {
    int32 basename_start;

    if ((idx != 0) || (song == NULL) || (song->uri_len <= 0)) {
        ncm_string_view_clear(view);
        return false;
    }
    basename_start = ncm_string_basename_start(song->uri, song->uri_len);
    ncm_string_view_set(view,
                        song->uri + basename_start,
                        song->uri_len - basename_start);
    return true;
}

bool
ncm_song_is_from_database(NcmSong *song) {
    if ((song == NULL) || (song->uri == NULL)) {
        return false;
    }
    return !ncm_song_is_stream(song);
}

bool
ncm_song_is_stream(NcmSong *song) {
    if ((song == NULL) || (song->uri == NULL)) {
        return false;
    }
    return memmem64(song->uri, song->uri_len, STRLIT("://")) != NULL;
}

bool
ncm_song_equal(NcmSong *a, NcmSong *b) {
    if ((a == NULL) || (b == NULL)) {
        return false;
    }
    return STREQUAL(a->uri, a->uri_len, b->uri, b->uri_len);
}

StrBuilder
ncm_format_render_string(NcmFormatAst *ast, NcmSong *song) {
    StrBuilder result = {0};

    (void)ast;
    (void)song;
    return result;
}

int64
ncm_status_state_elapsed_time_ms(void) {
    return lyrics_test_elapsed_ms;
}

enum NcmStatusPlayerState
ncm_status_state_player(void) {
    return (enum NcmStatusPlayerState)lyrics_test_player_state;
}

NcmStringFormatArg
ncm_string_format_arg_string(char *data, int32 len) {
    NcmStringFormatArg arg;

    arg.type = NCM_STRING_FORMAT_ARG_STRING;
    ncm_string_view_set(&arg.value.string, data, len);
    return arg;
}

NcmStringFormatArg
ncm_string_format_arg_cstring(char *data) {
    return ncm_string_format_arg_string(data, optional_strlen32(data));
}

void
ncm_statusbar_format(int32 delay_seconds, char *format, int32 format_len,
                     NcmStringFormatArg *args, int32 args_len) {
    int32 idx;

    (void)delay_seconds;
    sb_clear(&lyrics_test_status_message);
    for (int32 i = 0; i < format_len; i += 1) {
        if (format[i] != '%') {
            sb_append_byte(&lyrics_test_status_message, format[i]);
            continue;
        }

        idx = -1;
        if ((i + 2 < format_len)
            && (format[i + 2] == '%')
            && (format[i + 1] >= '1')
            && (format[i + 1] <= '9')) {
            idx = format[i + 1] - '1';
        }

        if ((idx >= 0) && (idx < args_len)
            && (args[idx].type == NCM_STRING_FORMAT_ARG_STRING)) {
            SB_APPEND(&lyrics_test_status_message,
                      args[idx].value.string.data,
                      args[idx].value.string.len);
            i += 2;
            continue;
        }

        sb_append_byte(&lyrics_test_status_message, format[i]);
    }
    return;
}

void
ncm_title_draw_header(char *title, int32 title_len) {
    (void)title;
    (void)title_len;
    return;
}

int32
global_volume_state_len(void) {
    return 0;
}

int32
ui_state_screen_width(void) {
    return 80;
}

int32
ui_state_main_start_y(void) {
    return 0;
}

int32
ui_state_main_height(void) {
    return 10;
}

void
nc_screen_switcher_get_resize_params(NcScreen *screen,
                                      int32 *x_offset, int32 *width,
                                      bool adjust_locked_screen) {
    (void)screen;
    (void)adjust_locked_screen;
    *x_offset = 0;
    *width = 40;
    return;
}

static void
lyrics_screen_test_write_file(char *path, char *text, int32 text_len) {
    FILE *file;
    int32 written;

    file = fopen(path, "wb");
    ASSERT(file != NULL);
    written = (int32)fwrite64(text, 1, text_len, file);
    ASSERT_EQUAL(written, text_len);
    ASSERT(fclose(file) == 0);
    return;
}

static void
lyrics_screen_test_remove_file(char *path) {
    if (unlink(path) != 0) {
        ASSERT(errno == ENOENT);
    }
    return;
}

static void
lyrics_screen_test_clear_pushed_job(void) {
    if (!lyrics_test_has_pushed_job) {
        return;
    }

    if (lyrics_test_pushed_job.destroy) {
        lyrics_test_pushed_job.destroy(lyrics_test_pushed_job.user);
    }
    memset64(&lyrics_test_pushed_job, 0, SIZEOF(lyrics_test_pushed_job));
    lyrics_test_has_pushed_job = false;
    return;
}

static NativeLyricsJob *
lyrics_screen_test_pushed_lyrics_job(void) {
    ASSERT(lyrics_test_has_pushed_job);
    return lyrics_test_pushed_job.user;
}

static void
lyrics_screen_test_assert_file_text(char *path, char *text, int32 text_len) {
    char *bytes;
    int32 bytes_len;

    ASSERT(read_entire_file(path, &bytes, &bytes_len));
    ASSERT(STREQUAL(bytes, bytes_len, text, text_len));
    free2(bytes, bytes_len + 1);
    return;
}

static void
lyrics_screen_test_clear_status_message(void) {
    sb_clear(&lyrics_test_status_message);
    return;
}

static void
lyrics_screen_test_assert_status_message(char *message,
                                         int32 message_len) {
    ASSERT(STREQUAL(lyrics_test_status_message.data,
                    lyrics_test_status_message.len,
                    message,
                    message_len));
    return;
}

static void
lyrics_screen_test_path(char *buffer, int32 buffer_cap,
                        char *directory, char *name) {
    int32 len;

    len = snprintf2(buffer, buffer_cap, "%s/%s", directory, name);
    ASSERT(len < buffer_cap);
    return;
}

static void
lyrics_screen_test_prepare_directory(char *directory, int32 directory_cap) {
    int32 len;

    len = snprintf2(directory, directory_cap,
                    "/tmp/ncmpcpp2-lyrics-screen-%d", getpid());
    ASSERT(len < directory_cap);
    (void)mkdir(directory, 0700);
    return;
}

static void
lyrics_screen_test_setup_config(char *directory) {
    Config.lyrics_directory = directory;
    Config.lyrics_directory_len = strlen32(directory);
    Config.mpd_music_dir = NULL;
    Config.mpd_music_dir_len = 0;
    Config.store_lyrics_in_song_dir = false;
    Config.generate_win32_compatible_filenames = false;
    Config.regex_flags = 0;
    Config.lines_scrolled = 1;
    Config.message_delay_time = 0;
    Config.header_text_scrolling = false;
    Config.design = NCM_DESIGN_CLASSIC;
    return;
}

static void
lyrics_screen_test_song(NcmSong *song) {
    ncm_song_init(song);
    ASSERT(ncm_song_set_uri(song, STRLIT("song.flac")));
    ASSERT(ncm_song_add_tag(song, MPD_TAG_ARTIST, STRLIT("Artist")));
    ASSERT(ncm_song_add_tag(song, MPD_TAG_TITLE, STRLIT("Title")));
    return;
}

static void
lyrics_screen_test_init(NativeLyricsScreen *screen) {
    native_lyrics_screen_init(screen,
                              0,
                              40,
                              0,
                              4,
                              nc_color_default(),
                              nc_border_none(),
                              1);
    return;
}

static bool
lyrics_screen_test_has_property(NcBuffer *buffer, int64 id) {
    NcBufferProperty *properties;

    properties = nc_buffer_properties(buffer);
    for (int32 i = 0; i < nc_buffer_property_count(buffer); i += 1) {
        if (properties[i].id == id) {
            return true;
        }
    }
    return false;
}

static bool
lyrics_screen_test_formatted_color_matches(NcFormattedColor *color) {
    enum NcFormat *formats;

    if (!nc_color_equal(color->color,
                        nc_color_make(COLOR_WHITE, COLOR_BLACK,
                                      false, false))) {
        return false;
    }

    formats = nc_formatted_color_formats(color);
    if (nc_formatted_color_format_count(color) != 1) {
        return false;
    }
    if (formats[0] != NC_FORMAT_BOLD) {
        return false;
    }
    return true;
}

static bool
lyrics_screen_test_has_sync_property(NcBuffer *buffer, int32 position,
                                     enum NcBufferPropertyType type) {
    NcBufferProperty *properties;

    properties = nc_buffer_properties(buffer);
    for (int32 i = 0; i < nc_buffer_property_count(buffer); i += 1) {
        if ((properties[i].id == NATIVE_LYRICS_SYNC_PROPERTY_ID)
            && (properties[i].position == position)
            && (properties[i].type == type)
            && lyrics_screen_test_formatted_color_matches(
                &properties[i].value.formatted_color)) {
            return true;
        }
    }
    return false;
}

static void
lyrics_screen_test_assert_sync_highlight(NativeLyricsScreen *screen,
                                         int32 entry_index) {
    NcmLrcEntry *entry;

    ASSERT(entry_index >= 0);
    ASSERT(entry_index < screen->lrc.entries_len);
    entry = &screen->lrc.entries[entry_index];
    ASSERT(lyrics_screen_test_has_sync_property(
        &screen->display, entry->buffer_start,
        NC_BUFFER_PROPERTY_FORMATTED_COLOR));
    ASSERT(lyrics_screen_test_has_sync_property(
        &screen->display, entry->buffer_end,
        NC_BUFFER_PROPERTY_FORMATTED_COLOR_END));
    return;
}

static bool
lyrics_screen_test_dummy_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
lyrics_screen_test_lrc_preferred_over_txt(void) {
    NativeLyricsScreen screen;
    NcmSong song;
    NcmError error = {0};
    NativeLyricsMode mode;
    char directory[256];
    char lrc_path[512];
    char txt_path[512];

    lyrics_screen_test_prepare_directory(directory, SIZEOF(directory));
    lyrics_screen_test_setup_config(directory);
    lyrics_screen_test_path(lrc_path, SIZEOF(lrc_path), directory,
                            "Artist - Title.lrc");
    lyrics_screen_test_path(txt_path, SIZEOF(txt_path), directory,
                            "Artist - Title.txt");
    lyrics_screen_test_write_file(lrc_path, STRLIT("[00:01.00]synced\n"));
    lyrics_screen_test_write_file(txt_path, STRLIT("plain\n"));

    lyrics_screen_test_song(&song);
    lyrics_screen_test_init(&screen);
    lyrics_screen_test_clear_status_message();
    ASSERT(native_lyrics_screen_fetch(&screen, &song, NULL, &error));
    lyrics_screen_test_assert_status_message(
        STRLIT("Artist - Title.lrc found; Artist - Title.txt found"));
    mode = native_lyrics_screen_mode(&screen);
    ASSERT_EQUAL((int32)mode, (int32)NATIVE_LYRICS_MODE_SYNCHRONIZED);
    ASSERT(STREQUAL(screen.filename.data, screen.filename.len,
                    lrc_path, strlen32(lrc_path)));
    ASSERT(STREQUAL(screen.display.data, screen.display.len,
                    STRLIT("synced")));

    native_lyrics_screen_destroy(&screen);
    ncm_song_destroy(&song);
    lyrics_screen_test_remove_file(lrc_path);
    lyrics_screen_test_remove_file(txt_path);
    ASSERT(rmdir(directory) == 0);
    return;
}

static void
lyrics_screen_test_txt_used_when_lrc_missing(void) {
    NativeLyricsScreen screen;
    NcmSong song;
    NcmError error = {0};
    NativeLyricsMode mode;
    char directory[256];
    char txt_path[512];

    lyrics_screen_test_prepare_directory(directory, SIZEOF(directory));
    lyrics_screen_test_setup_config(directory);
    lyrics_screen_test_path(txt_path, SIZEOF(txt_path), directory,
                            "Artist - Title.txt");
    lyrics_screen_test_write_file(txt_path, STRLIT("plain\nline\n"));

    lyrics_screen_test_song(&song);
    lyrics_screen_test_init(&screen);
    lyrics_screen_test_clear_status_message();
    ASSERT(native_lyrics_screen_fetch(&screen, &song, NULL, &error));
    lyrics_screen_test_assert_status_message(
        STRLIT("Artist - Title.lrc not found; Artist - Title.txt found"));
    mode = native_lyrics_screen_mode(&screen);
    ASSERT_EQUAL((int32)mode, (int32)NATIVE_LYRICS_MODE_PLAIN);
    ASSERT(STREQUAL(screen.filename.data, screen.filename.len,
                    txt_path, strlen32(txt_path)));
    ASSERT(STREQUAL(screen.display.data, screen.display.len,
                    STRLIT("plain\nline")));

    native_lyrics_screen_destroy(&screen);
    ncm_song_destroy(&song);
    lyrics_screen_test_remove_file(txt_path);
    ASSERT(rmdir(directory) == 0);
    return;
}

static void
lyrics_screen_test_invalid_lrc_falls_back_to_txt(void) {
    NativeLyricsScreen screen;
    NcmSong song;
    NcmError error = {0};
    NativeLyricsMode mode;
    char directory[256];
    char lrc_path[512];
    char txt_path[512];

    lyrics_screen_test_prepare_directory(directory, SIZEOF(directory));
    lyrics_screen_test_setup_config(directory);
    lyrics_screen_test_path(lrc_path, SIZEOF(lrc_path), directory,
                            "Artist - Title.lrc");
    lyrics_screen_test_path(txt_path, SIZEOF(txt_path), directory,
                            "Artist - Title.txt");
    lyrics_screen_test_write_file(lrc_path, STRLIT("untimed\n"));
    lyrics_screen_test_write_file(txt_path, STRLIT("fallback\n"));

    lyrics_screen_test_song(&song);
    lyrics_screen_test_init(&screen);
    lyrics_screen_test_clear_status_message();
    ASSERT(native_lyrics_screen_fetch(&screen, &song, NULL, &error));
    lyrics_screen_test_assert_status_message(
        STRLIT("Artist - Title.lrc found; Artist - Title.txt found"));
    mode = native_lyrics_screen_mode(&screen);
    ASSERT_EQUAL((int32)mode, (int32)NATIVE_LYRICS_MODE_PLAIN);
    ASSERT(STREQUAL(screen.filename.data, screen.filename.len,
                    txt_path, strlen32(txt_path)));
    ASSERT(STREQUAL(screen.display.data, screen.display.len,
                    STRLIT("fallback")));
    ASSERT_EQUAL(screen.lrc.entries_len, 0);

    native_lyrics_screen_destroy(&screen);
    ncm_song_destroy(&song);
    lyrics_screen_test_remove_file(lrc_path);
    lyrics_screen_test_remove_file(txt_path);
    ASSERT(rmdir(directory) == 0);
    return;
}

static void
lyrics_screen_test_missing_sidecars_report_status(void) {
    NativeLyricsScreen screen;
    NcmSong song;
    NcmError error = {0};
    char directory[256];

    lyrics_screen_test_clear_pushed_job();
    lyrics_screen_test_prepare_directory(directory, SIZEOF(directory));
    lyrics_screen_test_setup_config(directory);

    lyrics_screen_test_song(&song);
    lyrics_screen_test_init(&screen);
    lyrics_screen_test_clear_status_message();
    ASSERT(native_lyrics_screen_fetch(&screen, &song, NULL, &error));
    lyrics_screen_test_assert_status_message(
        STRLIT("Artist - Title.lrc not found; Artist - Title.txt not found"));

    lyrics_screen_test_clear_pushed_job();
    native_lyrics_screen_destroy(&screen);
    ncm_song_destroy(&song);
    ASSERT(rmdir(directory) == 0);
    return;
}

static void
lyrics_screen_test_update_at(int64 elapsed_ms,
                             NativeLyricsScreen *screen,
                             int32 expected_line) {
    lyrics_test_elapsed_ms = elapsed_ms;
    lyrics_test_player_state = NCM_STATUS_PLAYER_PLAY;
    native_lyrics_screen_update(screen);
    ASSERT_EQUAL(native_lyrics_screen_active_lrc_line(screen),
                 expected_line);
    return;
}

static void
lyrics_screen_test_active_line_selection(void) {
    NativeLyricsScreen screen;
    NcmError error = {0};
    char path[] = "/tmp/ncmpcpp2-lyrics-screen-active.lrc";

    lyrics_screen_test_setup_config("/tmp");
    lyrics_screen_test_write_file(path,
                                  STRLIT("[00:05.00]five\n"
                                         "[00:10.00]ten\n"
                                         "[00:12.50]twelve\n"));
    lyrics_screen_test_init(&screen);
    ASSERT(native_lyrics_screen_load_file(&screen,
                                          path,
                                          strlen32(path),
                                          &error));

    lyrics_screen_test_update_at(4999, &screen, -1);
    lyrics_screen_test_update_at(5000, &screen, 0);
    lyrics_screen_test_update_at(9999, &screen, 0);
    lyrics_screen_test_update_at(10000, &screen, 1);
    lyrics_screen_test_update_at(12500, &screen, 2);
    lyrics_screen_test_update_at(999999, &screen, 2);

    native_lyrics_screen_destroy(&screen);
    lyrics_screen_test_remove_file(path);
    return;
}

static void
lyrics_screen_test_search_and_sync_properties_are_independent(void) {
    NativeLyricsScreen screen;
    NcmError error = {0};
    char path[] = "/tmp/ncmpcpp2-lyrics-screen-highlight.lrc";

    lyrics_screen_test_setup_config("/tmp");
    lyrics_screen_test_write_file(path,
                                  STRLIT("[00:01.00]alpha\n"
                                         "[00:02.00]beta\n"));
    lyrics_screen_test_init(&screen);
    ASSERT(native_lyrics_screen_load_file(&screen,
                                          path,
                                          strlen32(path),
                                          &error));
    ASSERT(native_lyrics_screen_find(&screen, STRLIT("alpha"), &error));
    ASSERT(lyrics_screen_test_has_property(
        &screen.display, NATIVE_LYRICS_SEARCH_PROPERTY_ID));

    lyrics_screen_test_update_at(1000, &screen, 0);
    ASSERT(lyrics_screen_test_has_property(
        &screen.display, NATIVE_LYRICS_SEARCH_PROPERTY_ID));
    ASSERT(lyrics_screen_test_has_property(
        &screen.display, NATIVE_LYRICS_SYNC_PROPERTY_ID));

    native_lyrics_buffer_clear_sync_highlight(&screen.display);
    ASSERT(lyrics_screen_test_has_property(
        &screen.display, NATIVE_LYRICS_SEARCH_PROPERTY_ID));
    ASSERT(!lyrics_screen_test_has_property(
        &screen.display, NATIVE_LYRICS_SYNC_PROPERTY_ID));

    native_lyrics_screen_destroy(&screen);
    lyrics_screen_test_remove_file(path);
    return;
}

static void
lyrics_screen_test_auto_scroll_clamps(void) {
    NativeLyricsScreen screen;
    NcmError error = {0};
    char path[] = "/tmp/ncmpcpp2-lyrics-screen-scroll.lrc";

    lyrics_screen_test_setup_config("/tmp");
    lyrics_screen_test_write_file(path,
                                  STRLIT("[00:01.00]zero\n"
                                         "[00:02.00]one\n"
                                         "[00:03.00]two\n"
                                         "[00:04.00]three\n"
                                         "[00:05.00]four\n"
                                         "[00:06.00]five\n"));
    lyrics_screen_test_init(&screen);
    ASSERT(native_lyrics_screen_load_file(&screen,
                                          path,
                                          strlen32(path),
                                          &error));
    nc_scrollpad_flush(&screen.scrollpad, &screen.window, &screen.display);

    lyrics_screen_test_update_at(1000, &screen, 0);
    ASSERT_EQUAL(screen.scrollpad.beginning, 0);

    lyrics_screen_test_update_at(6000, &screen, 5);
    ASSERT_EQUAL(screen.scrollpad.beginning,
                 nc_scrollpad_max_beginning(&screen.scrollpad,
                                            &screen.window));

    native_lyrics_screen_destroy(&screen);
    lyrics_screen_test_remove_file(path);
    return;
}

static void
lyrics_screen_test_resize_reflushes_plain_lyrics(void) {
    NativeLyricsScreen screen;
    NcmError error = {0};
    char path[] = "/tmp/ncmpcpp2-lyrics-screen-resize.txt";

    lyrics_screen_test_setup_config("/tmp");
    lyrics_screen_test_write_file(path, STRLIT("alpha\nbeta\n"));
    lyrics_screen_test_init(&screen);
    ASSERT(native_lyrics_screen_load_file(&screen,
                                          path,
                                          strlen32(path),
                                          &error));
    ASSERT(native_lyrics_screen_mode(&screen) == NATIVE_LYRICS_MODE_PLAIN);

    lyrics_test_print_count = 0;
    native_lyrics_screen_set_geometry(&screen, 0, 20, 0, 4);
    ASSERT(lyrics_test_print_count > 0);

    native_lyrics_screen_destroy(&screen);
    lyrics_screen_test_remove_file(path);
    return;
}

static void
lyrics_screen_test_lrc_integration_fixture(void) {
    NativeLyricsScreen screen;
    NativeLyricsScreen plain_screen;
    NcmSong song;
    NcmError error = {0};
    char directory[256];
    char lrc_path[512];
    char txt_path[512];
    int32 first_beginning;

    lyrics_screen_test_prepare_directory(directory, SIZEOF(directory));
    lyrics_screen_test_setup_config(directory);
    lyrics_screen_test_path(lrc_path, SIZEOF(lrc_path), directory,
                            "Artist - Title.lrc");
    lyrics_screen_test_path(txt_path, SIZEOF(txt_path), directory,
                            "Artist - Title.txt");
    lyrics_screen_test_write_file(lrc_path,
                                  STRLIT("[00:01.00]first line\n"
                                         "[00:02.00]second line\n"
                                         "[00:03.00]third line\n"
                                         "[00:04.00]fourth line\n"
                                         "[00:05.00]fifth line\n"
                                         "[00:06.00]sixth line\n"
                                         "[00:07.00]seventh line\n"));
    lyrics_screen_test_write_file(txt_path, STRLIT("plain fallback\n"));

    lyrics_screen_test_song(&song);
    lyrics_screen_test_init(&screen);
    ASSERT(native_lyrics_screen_fetch(&screen, &song, NULL, &error));
    ASSERT(native_lyrics_screen_mode(&screen)
           == NATIVE_LYRICS_MODE_SYNCHRONIZED);
    ASSERT(STREQUAL(screen.display.data, screen.display.len,
                    STRLIT("first line\n"
                           "second line\n"
                           "third line\n"
                           "fourth line\n"
                           "fifth line\n"
                           "sixth line\n"
                           "seventh line")));
    ASSERT(memmem64(screen.display.data,
                    screen.display.len,
                    STRLIT("[00:")) == NULL);

    lyrics_screen_test_update_at(1000, &screen, 0);
    lyrics_screen_test_assert_sync_highlight(&screen, 0);
    first_beginning = screen.scrollpad.beginning;

    lyrics_screen_test_update_at(7000, &screen, 6);
    lyrics_screen_test_assert_sync_highlight(&screen, 6);
    ASSERT(screen.scrollpad.beginning > first_beginning);

    native_lyrics_screen_destroy(&screen);

    lyrics_screen_test_remove_file(lrc_path);
    lyrics_screen_test_init(&plain_screen);
    ASSERT(native_lyrics_screen_fetch(&plain_screen, &song, NULL, &error));
    ASSERT(native_lyrics_screen_mode(&plain_screen)
           == NATIVE_LYRICS_MODE_PLAIN);
    ASSERT(STREQUAL(plain_screen.display.data, plain_screen.display.len,
                    STRLIT("plain fallback")));

    native_lyrics_screen_destroy(&plain_screen);
    ncm_song_destroy(&song);
    lyrics_screen_test_remove_file(txt_path);
    ASSERT(rmdir(directory) == 0);
    return;
}

static void
lyrics_screen_test_refetch_writes_txt_without_removing_lrc(void) {
    NativeLyricsScreen screen;
    NativeLyricsJob *job;
    NcmSong song;
    NcmError error = {0};
    char directory[256];
    char lrc_path[512];
    char txt_path[512];

    lyrics_screen_test_clear_pushed_job();
    lyrics_screen_test_prepare_directory(directory, SIZEOF(directory));
    lyrics_screen_test_setup_config(directory);
    lyrics_screen_test_path(lrc_path, SIZEOF(lrc_path), directory,
                            "Artist - Title.lrc");
    lyrics_screen_test_path(txt_path, SIZEOF(txt_path), directory,
                            "Artist - Title.txt");
    lyrics_screen_test_write_file(lrc_path, STRLIT("[00:01.00]synced\n"));
    lyrics_screen_test_write_file(txt_path, STRLIT("old plain\n"));

    lyrics_screen_test_song(&song);
    lyrics_screen_test_init(&screen);
    ASSERT(native_lyrics_screen_fetch(&screen, &song, NULL, &error));
    ASSERT(native_lyrics_screen_mode(&screen)
           == NATIVE_LYRICS_MODE_SYNCHRONIZED);

    native_lyrics_screen_refetch_current(&screen, &error);
    ASSERT(lyrics_test_has_pushed_job);
    ASSERT(ncm_fs_exists(lrc_path, strlen32(lrc_path)));
    ASSERT(!ncm_fs_exists(txt_path, strlen32(txt_path)));
    ASSERT(STREQUAL(screen.filename.data, screen.filename.len,
                    txt_path, strlen32(txt_path)));

    job = lyrics_screen_test_pushed_lyrics_job();
    ASSERT(STREQUAL(job->filename.data, job->filename.len,
                    txt_path, strlen32(txt_path)));
    ASSERT(ncm_lyrics_result_set(&job->result, true,
                                 STRLIT("downloaded plain\n")));
    lyrics_test_pushed_job.complete(true, &error, job);
    ASSERT(ncm_fs_exists(lrc_path, strlen32(lrc_path)));
    lyrics_screen_test_assert_file_text(txt_path,
                                        STRLIT("downloaded plain\n"));
    ASSERT(native_lyrics_screen_mode(&screen) == NATIVE_LYRICS_MODE_PLAIN);

    lyrics_screen_test_clear_pushed_job();
    native_lyrics_screen_destroy(&screen);
    ncm_song_destroy(&song);
    lyrics_screen_test_remove_file(lrc_path);
    lyrics_screen_test_remove_file(txt_path);
    ASSERT(rmdir(directory) == 0);
    return;
}

static void
lyrics_screen_test_background_fetch_respects_lrc_and_txt(void) {
    NativeLyricsScreen screen;
    NativeLyricsJob *job;
    NcmSong song;
    NcmError error = {0};
    char directory[256];
    char lrc_path[512];
    char txt_path[512];

    lyrics_screen_test_clear_pushed_job();
    lyrics_screen_test_prepare_directory(directory, SIZEOF(directory));
    lyrics_screen_test_setup_config(directory);
    lyrics_screen_test_path(lrc_path, SIZEOF(lrc_path), directory,
                            "Artist - Title.lrc");
    lyrics_screen_test_path(txt_path, SIZEOF(txt_path), directory,
                            "Artist - Title.txt");
    lyrics_screen_test_write_file(lrc_path, STRLIT("[00:01.00]synced\n"));

    lyrics_screen_test_song(&song);
    lyrics_screen_test_init(&screen);
    ASSERT(native_lyrics_screen_fetch_in_background(
        &screen, &song, false, &error));
    ASSERT(!lyrics_test_has_pushed_job);

    lyrics_screen_test_remove_file(lrc_path);
    lyrics_screen_test_write_file(txt_path, STRLIT("plain\n"));
    ASSERT(native_lyrics_screen_fetch_in_background(
        &screen, &song, false, &error));
    ASSERT(!lyrics_test_has_pushed_job);

    lyrics_screen_test_remove_file(txt_path);
    ASSERT(native_lyrics_screen_fetch_in_background(
        &screen, &song, false, &error));
    ASSERT(lyrics_test_has_pushed_job);
    job = lyrics_screen_test_pushed_lyrics_job();
    ASSERT(STREQUAL(job->filename.data, job->filename.len,
                    txt_path, strlen32(txt_path)));

    lyrics_screen_test_clear_pushed_job();
    native_lyrics_screen_destroy(&screen);
    ncm_song_destroy(&song);
    lyrics_screen_test_remove_file(txt_path);
    ASSERT(rmdir(directory) == 0);
    return;
}

static void
lyrics_screen_test_locked_visible_update_refreshes_lrc(void) {
    NativeLyricsScreen screen;
    NcScreenRegistry registry;
    NcScreenCallbacks callbacks = {0};
    NcScreen dummy;
    NcmError error = {0};
    char path[] = "/tmp/ncmpcpp2-lyrics-screen-visible.lrc";

    lyrics_screen_test_setup_config("/tmp");
    lyrics_screen_test_write_file(path,
                                  STRLIT("[00:01.00]visible\n"));
    lyrics_screen_test_init(&screen);
    ASSERT(native_lyrics_screen_load_file(&screen,
                                          path,
                                          strlen32(path),
                                          &error));

    callbacks.is_mergable = lyrics_screen_test_dummy_is_mergable;
    nc_screen_init(&dummy, callbacks, NULL, NC_SCREEN_TYPE_BROWSER);
    nc_screen_registry_init(&registry);
    ASSERT(nc_screen_registry_register(
        &registry, native_lyrics_screen_base(&screen)));
    ASSERT(nc_screen_registry_register(&registry, &dummy));
    ASSERT(nc_screen_registry_switch_to(
        &registry, native_lyrics_screen_base(&screen)));
    ASSERT(nc_screen_registry_lock_current(&registry));
    ASSERT(nc_screen_registry_switch_to(&registry, &dummy));

    lyrics_test_elapsed_ms = 1000;
    lyrics_test_player_state = NCM_STATUS_PLAYER_PLAY;
    nc_screen_registry_update_visible(&registry);
    ASSERT_EQUAL(native_lyrics_screen_active_lrc_line(&screen), 0);
    lyrics_screen_test_assert_sync_highlight(&screen, 0);

    native_lyrics_screen_destroy(&screen);
    lyrics_screen_test_remove_file(path);
    return;
}

int
main(void) {
    lyrics_screen_test_lrc_preferred_over_txt();
    lyrics_screen_test_txt_used_when_lrc_missing();
    lyrics_screen_test_invalid_lrc_falls_back_to_txt();
    lyrics_screen_test_missing_sidecars_report_status();
    lyrics_screen_test_active_line_selection();
    lyrics_screen_test_search_and_sync_properties_are_independent();
    lyrics_screen_test_auto_scroll_clamps();
    lyrics_screen_test_resize_reflushes_plain_lyrics();
    lyrics_screen_test_lrc_integration_fixture();
    lyrics_screen_test_refetch_writes_txt_without_removing_lrc();
    lyrics_screen_test_background_fetch_respects_lrc_and_txt();
    lyrics_screen_test_locked_visible_update_refreshes_lrc();
    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_TESTS_LYRICS_SCREEN_C */
