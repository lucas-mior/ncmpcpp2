#include <assert.h>
#include <stdlib.h>

#include "actions.h"
#include "app_controller.h"
#include "bindings.h"
#include "screens/native_c_screens.h"
#include "screens/nc_visualizer.h"

static NativeVisualizerScreen test_visualizer;
static int32 test_register_calls;
static int32 test_switch_type_calls;
static enum ScreenType test_switch_type;
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

bool
__wrap_native_c_screens_switch_to_type(enum ScreenType screen_type) {
    test_switch_type_calls += 1;
    test_switch_type = screen_type;
    return true;
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
test_visualizer_mapping_uses_registered_screen(void) {
    NcmActionRuntime runtime;
    NcmBindingAction action;
    NcmBinding binding;
    enum NcmActionType type;

    ncm_action_runtime_init(&runtime);
    assert(ncm_action_type_parse(STRLIT_ARGS("show_visualizer"), &type));
    assert(type == NCM_ACTION_SHOW_VISUALIZER);

    ncm_binding_action_init(&action);
    action.kind = NCM_BINDING_ACTION_NORMAL;
    action.type = type;
    ncm_binding_init(&binding);
    assert(ncm_binding_append_action(&binding, &action));
    assert(ncm_binding_execute_default(&binding));
    ncm_binding_action_destroy(&action);
    ncm_binding_destroy(&binding);

    assert(test_register_calls == 1);
    assert(test_switch_type_calls == 1);
    assert(test_switch_type == NCM_SCREEN_TYPE_VISUALIZER);
    assert(test_screen_calls == 0);
    assert(test_toggle_calls == 0);

    assert(ncm_action_runtime_can_run(
        &runtime, NCM_ACTION_TOGGLE_VISUALIZATION_TYPE));
    assert(ncm_action_runtime_run(
        &runtime, NCM_ACTION_TOGGLE_VISUALIZATION_TYPE));
    assert(test_register_calls == 1);
    assert(test_switch_type_calls == 1);
    assert(test_screen_calls == 1);
    assert(test_toggle_calls == 1);
    return;
}

int
main(void) {
    test_visualizer_mapping_uses_registered_screen();
    exit(EXIT_SUCCESS);
}
