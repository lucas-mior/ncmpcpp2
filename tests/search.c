#if !defined(NCMPCPP_TESTS_SEARCH_C)
#define NCMPCPP_TESTS_SEARCH_C

#include <assert.h>
#include <stdlib.h>

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
#include "curses/nc_formatted_color.c"
#include "curses/nc_buffer.c"
#include "curses/nc_menu.c"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

typedef struct SearchTestContext {
    NcmRegex regex;
} SearchTestContext;

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
test_prompt_state_does_not_cache_failed_incremental_search(void) {
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

int
main(void) {
    test_prompt_search_accepts_current_match();
    test_forward_repeat_skips_current_match();
    test_backward_repeat_wraps_to_previous_match();
    test_search_skips_unselectable_matches();
    test_search_without_wrap_preserves_position_on_failure();
    test_prompt_state_reuses_successful_result();
    test_prompt_state_does_not_cache_failed_incremental_search();
    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_TESTS_SEARCH_C */
