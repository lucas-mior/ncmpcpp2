#include <assert.h>
#include <stddef.h>

#include "app_controller.h"
#include "settings.h"
#include "screens/native_c_screens.h"
#include "screens/screen_switcher.h"
#include "screens/screen_type.h"
#include "ui_state.h"


typedef struct TestScreenState {
    NcWindow window;
    int32 refresh_count;
    int32 refresh_window_count;
    int32 scroll_count;
    int32 switch_count;
    int32 resize_count;
    int32 update_count;
    int32 mouse_count;
    bool lockable;
    bool mergable;
    char *title;
} TestScreenState;

static NcWindow *test_active_window(NcScreen *screen);
static void test_refresh(NcScreen *screen);
static void test_refresh_window(NcScreen *screen);
static void test_scroll(NcScreen *screen, enum NcScroll where);
static void test_switch_to(NcScreen *screen);
static void test_resize(NcScreen *screen);
static char *test_title(NcScreen *screen);
static void test_update(NcScreen *screen);
static void test_mouse_button_pressed(NcScreen *screen, MEVENT event);
static bool test_is_lockable(NcScreen *screen);
static bool test_is_mergable(NcScreen *screen);
static NcScreenCallbacks test_callbacks(void);
static void test_native_screen_callbacks(void);
static void test_screen_switcher_helpers(void);
static void test_native_bridge_screen_api(void);
static void test_native_screen_switch_path(void);

int
main(void) {
    test_native_screen_callbacks();
    test_screen_switcher_helpers();
    test_native_bridge_screen_api();
    test_native_screen_switch_path();
    return 0;
}

static NcWindow *
test_active_window(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    return &state->window;
}

static void
test_refresh(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    state->refresh_count += 1;
    return;
}

static void
test_refresh_window(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    state->refresh_window_count += 1;
    return;
}

static void
test_scroll(NcScreen *screen, enum NcScroll where) {
    TestScreenState *state;

    (void)where;
    state = nc_screen_user(screen);
    state->scroll_count += 1;
    return;
}

static void
test_switch_to(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    state->switch_count += 1;
    return;
}

static void
test_resize(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    state->resize_count += 1;
    return;
}

static char *
test_title(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    return state->title;
}

static void
test_update(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    state->update_count += 1;
    return;
}

static void
test_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    TestScreenState *state;

    (void)event;
    state = nc_screen_user(screen);
    state->mouse_count += 1;
    return;
}

static bool
test_is_lockable(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    return state->lockable;
}

static bool
test_is_mergable(NcScreen *screen) {
    TestScreenState *state;

    state = nc_screen_user(screen);
    return state->mergable;
}

static NcScreenCallbacks
test_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = test_active_window;
    callbacks.refresh = test_refresh;
    callbacks.refresh_window = test_refresh_window;
    callbacks.scroll = test_scroll;
    callbacks.switch_to = test_switch_to;
    callbacks.resize = test_resize;
    callbacks.title = test_title;
    callbacks.update = test_update;
    callbacks.mouse_button_pressed = test_mouse_button_pressed;
    callbacks.is_lockable = test_is_lockable;
    callbacks.is_mergable = test_is_mergable;
    return callbacks;
}

static void
test_native_screen_callbacks(void) {
    NcScreen screen;
    TestScreenState state = {0};
    MEVENT event = {0};

    state.title = "native-test";
    nc_screen_init(&screen, test_callbacks(), &state, NC_SCREEN_TYPE_HELP);

    assert(nc_screen_active_window(&screen) == &state.window);
    assert(nc_screen_is_active_window(&screen, &state.window));
    assert(nc_screen_title(&screen) == state.title);
    assert(nc_screen_type(&screen) == NC_SCREEN_TYPE_HELP);

    nc_screen_request_resize(&screen);
    nc_screen_request_update(&screen);
    assert(nc_screen_has_to_be_resized(&screen));
    assert(nc_screen_has_to_be_updated(&screen));

    nc_screen_refresh(&screen);
    nc_screen_refresh_window(&screen);
    nc_screen_scroll(&screen, NC_SCROLL_DOWN);
    nc_screen_mouse_button_pressed(&screen, event);
    nc_screen_resize(&screen);
    nc_screen_update(&screen);

    assert(state.refresh_count == 1);
    assert(state.refresh_window_count == 1);
    assert(state.scroll_count == 1);
    assert(state.mouse_count == 1);
    assert(state.resize_count == 1);
    assert(state.update_count == 1);
    assert(!nc_screen_has_to_be_resized(&screen));
    assert(!nc_screen_has_to_be_updated(&screen));
    return;
}

static void
test_screen_switcher_helpers(void) {
    NcScreen left;
    NcScreen right;
    TestScreenState left_state = {0};
    TestScreenState right_state = {0};
    NcScreenResizeParams left_params;
    NcScreenResizeParams right_params;
    int64 x_offset;
    int64 width;

    app_controller_init();
    ui_state_set_screen_size(100, 30);
    Config.locked_screen_width_part = 0.40;

    left_state.lockable = true;
    left_state.mergable = true;
    right_state.mergable = true;
    nc_screen_init(&left, test_callbacks(), &left_state,
                   screen_type_to_native_type(NCM_SCREEN_TYPE_HELP));
    nc_screen_init(&right, test_callbacks(), &right_state,
                   screen_type_to_native_type(NCM_SCREEN_TYPE_LYRICS));

    assert(app_controller_register_screen(&left));
    assert(app_controller_register_screen(&right));
    assert(nc_screen_switcher_switch_to(&left, true));
    assert(nc_screen_switcher_finish_switch(&left));
    assert(nc_screen_switcher_is_current(&left));
    assert(nc_screen_switcher_is_visible(&left));
    assert(left_state.switch_count == 1);
    assert(left_state.resize_count == 1);

    assert(app_controller_lock_current_screen());
    assert(nc_screen_switcher_locked() == &left);
    assert(nc_screen_switcher_switch_to(&right, true));
    assert(nc_screen_switcher_is_current(&right));
    assert(nc_screen_switcher_is_previous(&left));
    assert(nc_screen_switcher_is_visible(&left));
    assert(nc_screen_switcher_is_visible(&right));

    left_params = nc_screen_switcher_resize_params(&left, false);
    right_params = nc_screen_switcher_resize_params(&right, false);
    assert(left_params.x_offset == 0);
    assert(left_params.width == 40);
    assert(right_params.x_offset == 41);
    assert(right_params.width == 59);

    nc_screen_switcher_get_resize_params(&right, &x_offset, &width, false);
    assert(x_offset == 41);
    assert(width == 59);

    nc_screen_switcher_request_visible_resize();
    nc_screen_switcher_request_visible_update();
    assert(nc_screen_has_to_be_resized(&left));
    assert(nc_screen_has_to_be_resized(&right));
    assert(nc_screen_has_to_be_updated(&left));
    assert(nc_screen_has_to_be_updated(&right));

    app_controller_resize_visible_screens();
    app_controller_update_visible_screens();
    assert(!nc_screen_has_to_be_resized(&left));
    assert(!nc_screen_has_to_be_resized(&right));
    assert(!nc_screen_has_to_be_updated(&left));
    assert(!nc_screen_has_to_be_updated(&right));
    return;
}

static void
test_native_bridge_screen_api(void) {
    NcScreen *(*help_screen)(void);
    NcScreen *(*outputs_screen)(void);
    NcScreen *(*server_info_screen)(void);
    NcScreen *(*song_info_screen)(void);
    bool (*help_is_current)(void);
    bool (*outputs_is_current)(void);
    void (*outputs_toggle)(void);

    help_screen = native_c_screen_help_native;
    outputs_screen = native_c_screen_outputs_native;
    server_info_screen = native_c_screen_server_info_native;
    song_info_screen = native_c_screen_song_info_native;
    help_is_current = native_c_screen_help_is_current;
    outputs_is_current = native_c_screen_outputs_is_current;
    outputs_toggle = native_c_screen_outputs_toggle;

    assert(help_screen != NULL);
    assert(outputs_screen != NULL);
    assert(server_info_screen != NULL);
    assert(song_info_screen != NULL);
    assert(help_is_current != NULL);
    assert(outputs_is_current != NULL);
    assert(outputs_toggle != NULL);
    return;
}

static void
test_native_screen_switch_path(void) {
    app_controller_init();
    ui_state_set_screen_size(100, 30);

    native_c_screen_browser_register();
    assert(native_c_screens_switch_to_type(NCM_SCREEN_TYPE_BROWSER));
    assert(native_c_screens_current_type() == NCM_SCREEN_TYPE_BROWSER);
    assert(native_c_screen_browser_is_current());
    assert(nc_screen_switcher_current() == native_c_screen_browser_native());
    return;
}
