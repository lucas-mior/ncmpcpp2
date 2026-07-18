#if !defined(NCMPCPP_TESTS_SEARCH_C)
#define NCMPCPP_TESTS_SEARCH_C

#include <assert.h>
#include <stdlib.h>

#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wpsabi"
#pragma GCC diagnostic ignored "-Wabi"
#endif

#include "c/ncm_base.c"
#include "c/ncm_error.c"
#include "c/ncm_regex.c"
#include "c/ncm_search_prompt.c"
#include "c/ncm_type_conversions.c"
#include "c/ncm_format.c"
#include "curses/nc_formatted_color.c"
#include "curses/nc_buffer.c"
#include "curses/nc_menu.c"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

typedef struct SearchTestContext {
    NcmRegex regex;
} SearchTestContext;

typedef struct SearchSongTestContext {
    NcmRegex regex;
    NcmFormatAst *format;
} SearchSongTestContext;

enum SearchTestSongId {
    SEARCH_TEST_SONG_CHOROS = 1,
    SEARCH_TEST_SONG_OHNE_DICH,
    SEARCH_TEST_SONG_OHNE_DICH_LIVE,
};

static char *
search_song_value(NcmSong *song, enum NcmSongGetter getter) {
    if (song == NULL) {
        return NULL;
    }

    switch (song->id) {
    case SEARCH_TEST_SONG_CHOROS:
        switch (getter) {
        case NCM_SONG_GETTER_ARTIST:
            return "São Paulo Symphony Orchestra; John Neschling";
        case NCM_SONG_GETTER_TITLE:
            return "Choros No 6 for orchestra";
        case NCM_SONG_GETTER_NAME:
            return "Choros No 6 for orchestra.flac";
        case NCM_SONG_GETTER_ALBUM:
            return "Choros No 1, 4, 6, 8 and 9";
        case NCM_SONG_GETTER_LENGTH:
            return "25:08";
        default:
            return NULL;
        }
    case SEARCH_TEST_SONG_OHNE_DICH:
        switch (getter) {
        case NCM_SONG_GETTER_ARTIST:
            return "Rammstein";
        case NCM_SONG_GETTER_TITLE:
            return "Ohne Dich";
        case NCM_SONG_GETTER_NAME:
            return "Ohne Dich.flac";
        case NCM_SONG_GETTER_ALBUM:
            return "Reise, Reise";
        case NCM_SONG_GETTER_LENGTH:
            return "4:32";
        default:
            return NULL;
        }
    case SEARCH_TEST_SONG_OHNE_DICH_LIVE:
        switch (getter) {
        case NCM_SONG_GETTER_ARTIST:
            return "Rammstein";
        case NCM_SONG_GETTER_NAME:
            return "Ohne Dich Live.flac";
        case NCM_SONG_GETTER_ALBUM:
            return "Live aus Berlin";
        case NCM_SONG_GETTER_LENGTH:
            return "5:01";
        default:
            return NULL;
        }
    default:
        return NULL;
    }
}

NcmBuffer
ncm_song_tags_buffer(NcmSong *song, enum NcmSongGetter getter,
                     char *separator, int32 separator_len,
                     bool show_duplicates) {
    NcmBuffer result;
    char *value;

    (void)separator;
    (void)separator_len;
    (void)show_duplicates;
    ncm_buffer_init(&result);
    value = search_song_value(song, getter);
    if (value != NULL) {
        ncm_buffer_append(&result, value, strlen32(value));
    }
    return result;
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
    NcColor color;

    color.foreground = 0;
    color.background = 0;
    color.is_default = true;
    color.is_end = false;
    return color;
}

NcColor
nc_color_end(void) {
    NcColor color;

    color.foreground = 0;
    color.background = 0;
    color.is_default = false;
    color.is_end = true;
    return color;
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
nc_window_clear(NcWindow *window) {
    (void)window;
    return;
}

void
nc_window_refresh(NcWindow *window) {
    (void)window;
    return;
}

void
nc_window_go_to_xy(NcWindow *window, int32 x, int32 y) {
    (void)window;
    (void)x;
    (void)y;
    return;
}

WINDOW *
nc_window_raw(NcWindow *window) {
    (void)window;
    return NULL;
}

void
nc_window_apply_term_manip(NcWindow *window, enum NcTermManip manip) {
    (void)window;
    (void)manip;
    return;
}

void
nc_window_print_char(NcWindow *window, char ch) {
    (void)window;
    (void)ch;
    return;
}

void
nc_window_push_color(NcWindow *window, NcColor color) {
    (void)window;
    (void)color;
    return;
}

void
nc_window_apply_format(NcWindow *window, enum NcFormat format) {
    (void)window;
    (void)format;
    return;
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

static void
search_item_copy(void *dest, void *source, void *user) {
    (void)user;
    *(char **)dest = *(char **)source;
    return;
}

static void
search_menu_init(NcMenu *menu) {
    NcMenuItemCallbacks callbacks;

    nc_menu_init(menu);
    callbacks.item_size = SIZEOF(char *);
    callbacks.construct = NULL;
    callbacks.copy = search_item_copy;
    callbacks.destroy = NULL;
    callbacks.user = NULL;
    nc_menu_set_item_callbacks(menu, callbacks);
    return;
}

static void
search_add_item(NcMenu *menu, char *text, uint32 flags) {
    nc_menu_add_item_with_flags(menu, &text, flags);
    return;
}

static void
search_add_song(NcMenu *menu, NcmSong *song) {
    nc_menu_add_item_with_flags(menu, &song, NC_MENU_ITEM_SELECTABLE);
    return;
}

static bool
search_matches(NcMenu *menu, int64 pos, void *user) {
    SearchTestContext *context;
    char **item;
    char *text;

    context = user;
    item = nc_menu_active_item_at(menu, pos);
    if (item == NULL) {
        return false;
    }

    text = *item;
    return ncm_regex_search(&context->regex, text, strlen32(text));
}

static bool
search_song_matches(NcMenu *menu, int64 pos, void *user) {
    SearchSongTestContext *context;
    NcmBuffer rendered;
    NcmSong **item;
    bool result;

    context = user;
    item = nc_menu_active_item_at(menu, pos);
    if ((item == NULL) || (*item == NULL)) {
        return false;
    }

    rendered = ncm_format_render_string(context->format, *item);
    result = ncm_regex_search(&context->regex, rendered.data,
                              rendered.len);
    ncm_buffer_destroy(&rendered);
    return result;
}

static void
search_compile_literal(SearchTestContext *context, char *pattern) {
    NcmError error;

    ncm_error_clear(&error);
    assert(ncm_regex_compile(&context->regex, pattern, strlen32(pattern),
                             NCM_REGEX_LITERAL_CASE_INSENSITIVE,
                             &error));
    assert(!ncm_error_is_set(&error));
    return;
}

static void
search_context_init(SearchTestContext *context, char *pattern) {
    ncm_regex_init(&context->regex);
    search_compile_literal(context, pattern);
    return;
}

static void
search_context_destroy(SearchTestContext *context) {
    ncm_regex_destroy(&context->regex);
    return;
}

static void
search_song_context_init(SearchSongTestContext *context,
                         NcmFormatAst *format, char *pattern) {
    NcmError error;

    context->format = format;
    ncm_regex_init(&context->regex);
    ncm_error_clear(&error);
    assert(ncm_regex_compile(&context->regex, pattern, strlen32(pattern),
                             NCM_REGEX_LITERAL_CASE_INSENSITIVE,
                             &error));
    assert(!ncm_error_is_set(&error));
    return;
}

static void
search_song_context_destroy(SearchSongTestContext *context) {
    ncm_regex_destroy(&context->regex);
    return;
}

static void
search_column_format_init(NcmFormatAst *format) {
    ncm_format_ast_init(format);
    assert(ncm_format_ast_append_column_types(
        format, STRLIT_ARGS("a")));
    assert(ncm_format_ast_append_column_types(
        format, STRLIT_ARGS("N")));
    assert(ncm_format_ast_append_column_types(
        format, STRLIT_ARGS("tf")));
    assert(ncm_format_ast_append_column_types(
        format, STRLIT_ARGS("b")));
    assert(ncm_format_ast_append_column_types(
        format, STRLIT_ARGS("l")));
    return;
}

static void
search_add_many(NcMenu *menu, char **items, int32 count) {
    for (int32 i = 0; i < count; i += 1) {
        search_add_item(menu, items[i], NC_MENU_ITEM_SELECTABLE);
    }
    return;
}

static void
test_prompt_search_accepts_current_match(void) {
    NcMenu menu;
    SearchTestContext context;
    char *items[] = {
        "Alpha",
        "beta",
        "alphabet",
        "gamma",
    };
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "alp");
    search_add_many(&menu, items, NCM_ARRAY_LEN(items));
    menu.highlight = 0;

    found = -1;
    assert(nc_menu_search_selectable(&menu, 3, true, true, false,
                                     search_matches, &context, &found));
    assert(found == 0);
    assert(nc_menu_highlight(&menu) == 0);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_forward_repeat_skips_current_match(void) {
    NcMenu menu;
    SearchTestContext context;
    char *items[] = {
        "alpha",
        "beta",
        "alphabet",
        "gamma",
    };
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "alp");
    search_add_many(&menu, items, NCM_ARRAY_LEN(items));
    menu.highlight = 0;

    found = -1;
    assert(nc_menu_search_selectable(&menu, 3, true, true, true,
                                     search_matches, &context, &found));
    assert(found == 2);
    assert(nc_menu_highlight(&menu) == 2);
    assert(menu.beginning == 1);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_forward_repeat_wraps_to_first_match(void) {
    NcMenu menu;
    SearchTestContext context;
    char *items[] = {
        "target first",
        "plain",
        "other",
        "target current",
    };
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "target");
    search_add_many(&menu, items, NCM_ARRAY_LEN(items));
    menu.highlight = 3;

    found = -1;
    assert(nc_menu_search_selectable(&menu, 3, true, true, true,
                                     search_matches, &context, &found));
    assert(found == 0);
    assert(nc_menu_highlight(&menu) == 0);
    assert(menu.beginning == 0);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_backward_repeat_wraps_to_previous_match(void) {
    NcMenu menu;
    SearchTestContext context;
    char *items[] = {
        "alpha",
        "beta",
        "alphabet",
        "gamma",
        "alpine",
    };
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "alp");
    search_add_many(&menu, items, NCM_ARRAY_LEN(items));
    menu.highlight = 0;

    found = -1;
    assert(nc_menu_search_selectable(&menu, 4, false, true, true,
                                     search_matches, &context, &found));
    assert(found == 4);
    assert(nc_menu_highlight(&menu) == 4);
    assert(menu.beginning == 2);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_search_skips_unselectable_matches(void) {
    NcMenu menu;
    SearchTestContext context;
    char *first;
    char *second;
    char *third;
    char *fourth;
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "target");
    first = "plain";
    second = "target separator";
    third = "target inactive";
    fourth = "target selectable";
    search_add_item(&menu, first, NC_MENU_ITEM_SELECTABLE);
    search_add_item(&menu, second, NC_MENU_ITEM_SEPARATOR);
    search_add_item(&menu, third, NC_MENU_ITEM_SELECTABLE
                                  |NC_MENU_ITEM_INACTIVE);
    search_add_item(&menu, fourth, NC_MENU_ITEM_SELECTABLE);
    menu.highlight = 0;

    found = -1;
    assert(nc_menu_search_selectable(&menu, 3, true, true, true,
                                     search_matches, &context, &found));
    assert(found == 3);
    assert(nc_menu_highlight(&menu) == 3);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_search_without_wrap_preserves_position_on_failure(void) {
    NcMenu menu;
    SearchTestContext context;
    char *items[] = {
        "target first",
        "plain",
        "target current",
    };
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "target");
    search_add_many(&menu, items, NCM_ARRAY_LEN(items));
    menu.highlight = 2;
    menu.beginning = 1;

    found = -7;
    assert(!nc_menu_search_selectable(&menu, 2, true, false, true,
                                      search_matches, &context, &found));
    assert(found == -7);
    assert(nc_menu_highlight(&menu) == 2);
    assert(menu.beginning == 1);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}


static void
test_prompt_state_reuses_successful_result(void) {
    NcmSearchPromptState state;
    bool found;

    ncm_search_prompt_state_init(&state, NCM_SEARCH_DIRECTION_FORWARD);
    assert(!ncm_search_prompt_state_cached_result(
        &state, STRLIT_ARGS("alp"), &found));

    assert(ncm_search_prompt_state_finish_result(
        &state, STRLIT_ARGS("alp"), true, true));
    found = false;
    assert(ncm_search_prompt_state_cached_result(
        &state, STRLIT_ARGS("alp"), &found));
    assert(found);
    assert(!ncm_search_prompt_state_cached_result(
        &state, STRLIT_ARGS("alpha"), &found));

    ncm_search_prompt_state_destroy(&state);
    return;
}

static void
test_prompt_state_does_not_cache_search_error(void) {
    NcmSearchPromptState state;
    bool found;

    ncm_search_prompt_state_init(&state, NCM_SEARCH_DIRECTION_FORWARD);
    assert(ncm_search_prompt_state_finish_result(
        &state, STRLIT_ARGS("alp"), true, true));
    assert(ncm_search_prompt_state_finish_result(
        &state, STRLIT_ARGS("alp["), false, false));

    found = true;
    assert(!ncm_search_prompt_state_cached_result(
        &state, STRLIT_ARGS("alp["), &found));
    assert(found);

    assert(ncm_search_prompt_state_finish_result(
        &state, STRLIT_ARGS("alp[ha]"), true, true));
    found = false;
    assert(ncm_search_prompt_state_cached_result(
        &state, STRLIT_ARGS("alp[ha]"), &found));
    assert(found);

    ncm_search_prompt_state_destroy(&state);
    return;
}

static void
test_backward_search_without_wrap_preserves_position(void) {
    NcMenu menu;
    SearchTestContext context;
    char *items[] = {
        "plain current",
        "target later",
        "other",
    };
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "target");
    search_add_many(&menu, items, NCM_ARRAY_LEN(items));
    menu.highlight = 0;
    menu.beginning = 0;

    found = -5;
    assert(!nc_menu_search_selectable(&menu, 2, false, false, true,
                                      search_matches, &context, &found));
    assert(found == -5);
    assert(nc_menu_highlight(&menu) == 0);
    assert(menu.beginning == 0);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_search_failure_with_wrap_preserves_position(void) {
    NcMenu menu;
    SearchTestContext context;
    char *items[] = {
        "alpha",
        "beta",
        "gamma",
    };
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "missing");
    search_add_many(&menu, items, NCM_ARRAY_LEN(items));
    menu.highlight = 1;
    menu.beginning = 1;

    found = -9;
    assert(!nc_menu_search_selectable(&menu, 2, true, true, true,
                                      search_matches, &context, &found));
    assert(found == -9);
    assert(nc_menu_highlight(&menu) == 1);
    assert(menu.beginning == 1);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_single_match_repeat_does_not_reselect_current(void) {
    NcMenu menu;
    SearchTestContext context;
    char *items[] = {
        "plain",
        "only target",
        "other",
    };
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "target");
    search_add_many(&menu, items, NCM_ARRAY_LEN(items));
    menu.highlight = 1;
    menu.beginning = 0;

    found = -1;
    assert(!nc_menu_search_selectable(&menu, 3, true, true, true,
                                      search_matches, &context, &found));
    assert(found == -1);
    assert(nc_menu_highlight(&menu) == 1);
    assert(menu.beginning == 0);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_search_uses_only_active_filtered_items(void) {
    NcMenu menu;
    SearchTestContext context;
    char *first;
    char *second;
    char *third;
    int64 found;

    search_menu_init(&menu);
    search_context_init(&context, "target");
    first = "target hidden";
    second = "plain visible";
    third = "target visible";
    search_add_item(&menu, first, NC_MENU_ITEM_SELECTABLE);
    search_add_item(&menu, second, NC_MENU_ITEM_SELECTABLE);
    search_add_item(&menu, third, NC_MENU_ITEM_SELECTABLE);
    nc_menu_add_filtered_item_ref(
        &menu, nc_menu_item_at(&menu, NC_MENU_ITEMS_ALL, 1));
    nc_menu_add_filtered_item_ref(
        &menu, nc_menu_item_at(&menu, NC_MENU_ITEMS_ALL, 2));
    nc_menu_show_filtered_items(&menu);
    menu.highlight = 0;

    found = -1;
    assert(nc_menu_search_selectable(&menu, 2, true, true, false,
                                     search_matches, &context, &found));
    assert(found == 1);
    assert(nc_menu_highlight(&menu) == 1);
    assert(*(char **)nc_menu_current_item(&menu) == third);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    return;
}

static void
test_prompt_state_caches_no_match(void) {
    NcmSearchPromptState state;
    bool found;

    ncm_search_prompt_state_init(&state, NCM_SEARCH_DIRECTION_FORWARD);
    assert(ncm_search_prompt_state_finish_result(
        &state, STRLIT_ARGS("ohne"), true, false));

    found = true;
    assert(ncm_search_prompt_state_cached_result(
        &state, STRLIT_ARGS("ohne"), &found));
    assert(!found);

    ncm_search_prompt_state_destroy(&state);
    return;
}

static void
test_column_search_uses_every_displayed_column(void) {
    NcmFormatAst format;
    NcmBuffer choros;
    NcmBuffer ohne;
    NcmSong choros_song = {0};
    NcmSong ohne_song = {0};
    SearchTestContext context;

    choros_song.id = SEARCH_TEST_SONG_CHOROS;
    ohne_song.id = SEARCH_TEST_SONG_OHNE_DICH;
    search_column_format_init(&format);
    choros = ncm_format_render_string(&format, &choros_song);
    ohne = ncm_format_render_string(&format, &ohne_song);
    search_context_init(&context, "ohn");

    assert(ncm_regex_search(&context.regex, choros.data, choros.len));
    search_context_destroy(&context);
    search_context_init(&context, "ohne");
    assert(!ncm_regex_search(&context.regex, choros.data, choros.len));
    assert(ncm_regex_search(&context.regex, ohne.data, ohne.len));
    assert(strstr(choros.data, "John Neschling") != NULL);
    assert(strstr(choros.data, "Choros No 6") != NULL);
    assert(strstr(ohne.data, "Rammstein") != NULL);
    assert(strstr(ohne.data, "Ohne Dich") != NULL);

    search_context_destroy(&context);
    ncm_buffer_destroy(&ohne);
    ncm_buffer_destroy(&choros);
    ncm_format_ast_destroy(&format);
    return;
}

static void
test_repeat_search_advances_between_column_matches(void) {
    NcMenu menu;
    NcmFormatAst format;
    NcmSong choros_song = {0};
    NcmSong ohne_song = {0};
    NcmSong live_song = {0};
    SearchSongTestContext context;
    int64 found;

    choros_song.id = SEARCH_TEST_SONG_CHOROS;
    ohne_song.id = SEARCH_TEST_SONG_OHNE_DICH;
    live_song.id = SEARCH_TEST_SONG_OHNE_DICH_LIVE;
    search_menu_init(&menu);
    search_column_format_init(&format);
    search_song_context_init(&context, &format, "ohne");
    search_add_song(&menu, &choros_song);
    search_add_song(&menu, &ohne_song);
    search_add_song(&menu, &live_song);
    menu.highlight = 0;

    found = -1;
    assert(nc_menu_search_selectable(&menu, 3, true, true, true,
                                     search_song_matches, &context,
                                     &found));
    assert(found == 1);
    assert(nc_menu_highlight(&menu) == 1);

    found = -1;
    assert(nc_menu_search_selectable(&menu, 3, true, true, true,
                                     search_song_matches, &context,
                                     &found));
    assert(found == 2);
    assert(nc_menu_highlight(&menu) == 2);

    found = -1;
    assert(nc_menu_search_selectable(&menu, 3, true, true, true,
                                     search_song_matches, &context,
                                     &found));
    assert(found == 1);
    assert(nc_menu_highlight(&menu) == 1);

    found = -1;
    assert(nc_menu_search_selectable(&menu, 3, false, true, true,
                                     search_song_matches, &context,
                                     &found));
    assert(found == 2);
    assert(nc_menu_highlight(&menu) == 2);

    search_song_context_destroy(&context);
    ncm_format_ast_destroy(&format);
    nc_menu_destroy(&menu);
    return;
}

static void
test_playlist_fixture_finds_ohne_dich(void) {
    NcMenu menu;
    SearchTestContext context;
    char *data;
    char *lines[128];
    int32 data_len;
    int32 count;
    int32 target;
    int64 found;

    data = read_entire_file("tests/playlist.m3u", &data_len);
    count = 0;
    target = -1;
    if (data_len > 0) {
        lines[count++] = data;
    }
    for (int32 i = 0; i < data_len; i += 1) {
        if (data[i] != '\n') {
            continue;
        }
        data[i] = '\0';
        if ((i + 1 < data_len) && (count < (int32)LENGTH(lines))) {
            lines[count++] = data + i + 1;
        }
    }

    search_menu_init(&menu);
    for (int32 i = 0; i < count; i += 1) {
        if (STREQUAL(lines[i], strlen32(lines[i]),
                     STRLIT_ARGS(
                         "Rammstein/2004 - Reise, Reise/Ohne Dich.flac"))) {
            target = i;
        }
        search_add_item(&menu, lines[i], NC_MENU_ITEM_SELECTABLE);
    }
    assert(target == 62);
    search_context_init(&context, "ohne");
    menu.highlight = 25;

    found = -1;
    assert(nc_menu_search_selectable(&menu, 10, true, true, true,
                                     search_matches, &context, &found));
    assert(found == target);
    assert(nc_menu_highlight(&menu) == target);

    found = -1;
    assert(!nc_menu_search_selectable(&menu, 10, true, true, true,
                                      search_matches, &context, &found));
    assert(found == -1);
    assert(nc_menu_highlight(&menu) == target);

    search_context_destroy(&context);
    nc_menu_destroy(&menu);
    free2(data, data_len + 1);
    return;
}

int
main(void) {
    test_prompt_search_accepts_current_match();
    test_forward_repeat_skips_current_match();
    test_forward_repeat_wraps_to_first_match();
    test_backward_repeat_wraps_to_previous_match();
    test_search_skips_unselectable_matches();
    test_search_without_wrap_preserves_position_on_failure();
    test_backward_search_without_wrap_preserves_position();
    test_search_failure_with_wrap_preserves_position();
    test_single_match_repeat_does_not_reselect_current();
    test_search_uses_only_active_filtered_items();
    test_prompt_state_reuses_successful_result();
    test_prompt_state_caches_no_match();
    test_prompt_state_does_not_cache_search_error();
    test_column_search_uses_every_displayed_column();
    test_repeat_search_advances_between_column_matches();
    test_playlist_fixture_finds_ohne_dich();
    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_TESTS_SEARCH_C */
