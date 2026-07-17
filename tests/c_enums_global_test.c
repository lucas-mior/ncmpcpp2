#include <stdio.h>
#include <stddef.h>

#include "c/ncm_enums.h"
#include "global.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/screen_type.h"

static int32 test_enums(void);
static int32 test_screen_type(void);
static int32 test_global_state(void);

static int32
test_enums(void) {
    enum DisplayMode display_mode;
    enum Design design;
    enum SortMode sort_mode;
    enum SpaceAddMode space_add_mode;
    enum VisualizerType visualizer_type;

    if (!ncm_display_mode_parse(STRLIT_ARGS("columns"), &display_mode)) {
        return 1;
    }
    if (display_mode != NCM_DISPLAY_MODE_COLUMNS) {
        return 2;
    }
    if (!ncm_design_parse(STRLIT_ARGS("alternative"), &design)) {
        return 3;
    }
    if (design != NCM_DESIGN_ALTERNATIVE) {
        return 4;
    }
    if (!ncm_sort_mode_parse(STRLIT_ARGS("mtime"), &sort_mode)) {
        return 5;
    }
    if (sort_mode != NCM_SORT_MODE_MODIFICATION_TIME) {
        return 6;
    }
    if (!ncm_space_add_mode_parse(STRLIT_ARGS("always_add"),
                                  &space_add_mode)) {
        return 7;
    }
    if (space_add_mode != NCM_SPACE_ADD_MODE_ALWAYS_ADD) {
        return 8;
    }
    if (!ncm_visualizer_type_parse(STRLIT_ARGS("wave_filled"),
                                   &visualizer_type)) {
        return 9;
    }
    if (visualizer_type != NCM_VISUALIZER_TYPE_WAVE_FILLED) {
        return 10;
    }

    return 0;
}

static int32
test_screen_type(void) {
    enum ScreenType screen_type;

    if (!screen_type_parse_startup(STRLIT_ARGS("playlist"),
                                   &screen_type)) {
        return 1;
    }
    if (screen_type != NCM_SCREEN_TYPE_PLAYLIST) {
        return 2;
    }
    if (!screen_type_parse(STRLIT_ARGS("server_info"), &screen_type)) {
        return 3;
    }
    if (screen_type != NCM_SCREEN_TYPE_SERVER_INFO) {
        return 4;
    }
    if (screen_type_parse_startup(STRLIT_ARGS("server_info"),
                                  &screen_type)) {
        return 5;
    }
    if (screen_type != NCM_SCREEN_TYPE_UNKNOWN) {
        return 6;
    }

    return 0;
}

static int32
test_global_state(void) {
    NcmTimePoint start;

    global_state_init();
    global_volume_state_set(STRLIT_ARGS(" Vol: "));
    global_volume_state_append(STRLIT_ARGS("42%"));
    if (global_volume_state_len() != 9) {
        return 1;
    }
    if (!STREQUAL(global_volume_state_cstr(), global_volume_state_len(),
                          STRLIT_ARGS(" Vol: 42%"))) {
        return 2;
    }

    if (!global_timer_update(NULL)) {
        return 3;
    }
    start = global_timer;
    if (global_timer_elapsed_ms(start) < 0) {
        return 4;
    }

    ncm_random_init(&global_random, 1);
    if (ncm_random_range_i32(&global_random, 10) < 0) {
        return 5;
    }

    global_state_destroy();
    return 0;
}

int32
main(void) {
    int32 result;

    result = test_enums();
    if (result != 0) {
        printf("test_enums failed: %d\n", result);
        return result;
    }

    result = test_screen_type();
    if (result != 0) {
        printf("test_screen_type failed: %d\n", result);
        return result;
    }

    result = test_global_state();
    if (result != 0) {
        printf("test_global_state failed: %d\n", result);
        return result;
    }

    return 0;
}
