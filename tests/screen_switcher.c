#if !defined(NCMPCPP_TESTS_SCREEN_SWITCHER_C)
#define NCMPCPP_TESTS_SCREEN_SWITCHER_C

#define CBASE_IMPLEMENT
#include "cbase.h"
#include "curses.h"

int32 color_set(int16 pair, void *opts);
int32 mvvline(int32 y, int32 x, chtype ch, int32 n);
int32 refresh(void);
int32 standend(void);

#include "screens/nc_screen.c"
#include "app_state.c"
#include "app_controller.c"
#include "screens/nc_screen_switcher.c"

Configuration Config;

static int32 screen_switcher_header_draws;
static char screen_switcher_header[128];

typedef struct ScreenSwitcherTestScreen {
    NcScreen screen;
    char *title;
    char *switch_title;
    int32 switch_count;
} ScreenSwitcherTestScreen;

static char *screen_switcher_test_title(NcScreen *screen);
static void screen_switcher_test_switch_to(NcScreen *screen);
static void screen_switcher_test_screen_init(ScreenSwitcherTestScreen *screen,
                                            char *title, int32 type);
static void screen_switcher_reset_header(void);
static void test_switch_updates_header(void);
static void test_switch_to_same_screen_does_not_redraw_header(void);
static void test_failed_switch_does_not_redraw_header(void);

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

int32
nc_color_pair_number(NcColor color) {
    (void)color;
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
ncm_title_draw_current_header(void) {
    NcScreen *screen;
    char *title;
    int32 title_len;

    screen_switcher_header_draws += 1;
    screen = app_controller_current_screen();
    title = nc_screen_title(screen);
    title_len = optional_strlen32(title);
    if (title_len >= LENGTH(screen_switcher_header)) {
        title_len = LENGTH(screen_switcher_header) - 1;
    }
    memcpy64(screen_switcher_header, title, title_len);
    screen_switcher_header[title_len] = '\0';
    return;
}

static char *
screen_switcher_test_title(NcScreen *screen) {
    ScreenSwitcherTestScreen *test_screen;

    test_screen = nc_screen_user(screen);
    return test_screen->title;
}

static void
screen_switcher_test_switch_to(NcScreen *screen) {
    ScreenSwitcherTestScreen *test_screen;

    test_screen = nc_screen_user(screen);
    test_screen->switch_count += 1;
    if (test_screen->switch_title) {
        test_screen->title = test_screen->switch_title;
    }
    return;
}

static void
screen_switcher_test_screen_init(ScreenSwitcherTestScreen *screen,
                                 char *title, int32 type) {
    NcScreenCallbacks callbacks = {0};

    callbacks.switch_to = screen_switcher_test_switch_to;
    callbacks.title = screen_switcher_test_title;
    screen->title = title;
    screen->switch_title = NULL;
    screen->switch_count = 0;
    nc_screen_init(&screen->screen, callbacks, screen, type);
    return;
}

static void
screen_switcher_reset_header(void) {
    screen_switcher_header_draws = 0;
    screen_switcher_header[0] = '\0';
    app_controller_init();
    return;
}

static void
test_switch_updates_header(void) {
    ScreenSwitcherTestScreen playlist;
    ScreenSwitcherTestScreen search;

    screen_switcher_reset_header();
    screen_switcher_test_screen_init(&playlist, "Playlist",
                                     NC_SCREEN_TYPE_PLAYLIST);
    screen_switcher_test_screen_init(&search, "Old title",
                                     NC_SCREEN_TYPE_SEARCH_ENGINE);
    search.switch_title = "Search engine";
    ASSERT(app_controller_register_screen(&playlist.screen));
    ASSERT(app_controller_register_screen(&search.screen));

    ASSERT(nc_screen_switcher_switch_to(&playlist.screen, false));
    ASSERT_EQUAL(screen_switcher_header_draws, 1);
    ASSERT(strequal(screen_switcher_header, "Playlist"));

    ASSERT(nc_screen_switcher_switch_to(&search.screen, false));
    ASSERT_EQUAL(search.switch_count, 1);
    ASSERT_EQUAL(screen_switcher_header_draws, 2);
    ASSERT(strequal(screen_switcher_header, "Search engine"));
    return;
}

static void
test_switch_to_same_screen_does_not_redraw_header(void) {
    ScreenSwitcherTestScreen playlist;

    screen_switcher_reset_header();
    screen_switcher_test_screen_init(&playlist, "Playlist",
                                     NC_SCREEN_TYPE_PLAYLIST);
    ASSERT(app_controller_register_screen(&playlist.screen));

    ASSERT(nc_screen_switcher_switch_to(&playlist.screen, false));
    ASSERT(nc_screen_switcher_switch_to(&playlist.screen, false));
    ASSERT_EQUAL(playlist.switch_count, 2);
    ASSERT_EQUAL(screen_switcher_header_draws, 1);
    ASSERT(strequal(screen_switcher_header, "Playlist"));
    return;
}

static void
test_failed_switch_does_not_redraw_header(void) {
    ScreenSwitcherTestScreen playlist;

    screen_switcher_reset_header();
    screen_switcher_test_screen_init(&playlist, "Playlist",
                                     NC_SCREEN_TYPE_PLAYLIST);

    ASSERT(!nc_screen_switcher_switch_to(&playlist.screen, false));
    ASSERT_ZERO(screen_switcher_header_draws);
    ASSERT(strequal(screen_switcher_header, ""));
    return;
}

int
main(void) {
    test_switch_updates_header();
    test_switch_to_same_screen_does_not_redraw_header();
    test_failed_switch_does_not_redraw_header();
    return 0;
}

#endif /* NCMPCPP_TESTS_SCREEN_SWITCHER_C */
