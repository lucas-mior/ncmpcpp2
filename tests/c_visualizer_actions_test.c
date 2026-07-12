#include <assert.h>
#include <stdlib.h>

#include "actions.h"
#include "app_controller.h"
#include "screens/native_c_screens.h"
#include "screens/nc_visualizer.h"

static NativeVisualizerScreen test_visualizer;
static int32 test_register_calls;
static int32 test_switch_calls;
static int32 test_screen_calls;
static int32 test_toggle_calls;

NcScreen *
__wrap_app_controller_current_screen(void) {
    return NULL;
}

void
__wrap_native_c_screen_visualizer_register(void) {
    test_register_calls += 1;
    return;
}

void
__wrap_native_c_screen_visualizer_switch_to(void) {
    test_switch_calls += 1;
    return;
}

NativeVisualizerScreen *
__wrap_native_c_screen_visualizer(void) {
    test_screen_calls += 1;
    return &test_visualizer;
}

void
__wrap_native_visualizer_screen_toggle_type(
    NativeVisualizerScreen *screen) {
    assert(screen == &test_visualizer);
    test_toggle_calls += 1;
    return;
}

static void
test_visualizer_actions_use_c_runtime(void) {
    NcmActionRuntime runtime;

    ncm_action_runtime_init(&runtime);
    assert(ncm_action_runtime_can_run(
        &runtime, NCM_ACTION_SHOW_VISUALIZER));
    assert(ncm_action_runtime_run(
        &runtime, NCM_ACTION_SHOW_VISUALIZER));
    assert(test_register_calls == 1);
    assert(test_switch_calls == 1);
    assert(test_screen_calls == 0);
    assert(test_toggle_calls == 0);

    assert(ncm_action_runtime_can_run(
        &runtime, NCM_ACTION_TOGGLE_VISUALIZATION_TYPE));
    assert(ncm_action_runtime_run(
        &runtime, NCM_ACTION_TOGGLE_VISUALIZATION_TYPE));
    assert(test_register_calls == 1);
    assert(test_switch_calls == 1);
    assert(test_screen_calls == 1);
    assert(test_toggle_calls == 1);
    return;
}

int
main(void) {
    test_visualizer_actions_use_c_runtime();
    exit(EXIT_SUCCESS);
}
