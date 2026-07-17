#if !defined(NCMPCPP_UI_STATE_C)
#define NCMPCPP_UI_STATE_C

#include "ui_state.h"

static NcWindow *header_window;
static NcWindow *footer_window;
static int64 screen_width;
static int64 screen_height;
static int64 main_start_y;
static int64 main_height;
static int64 header_height;
static int64 footer_height;
static int64 footer_start_y;
static bool statusbar_visibility_baseline;

void
ui_state_set_header_window(NcWindow *window) {
    header_window = window;
    return;
}

NcWindow *
ui_state_header_window(void) {
    return header_window;
}

void
ui_state_set_footer_window(NcWindow *window) {
    footer_window = window;
    return;
}

NcWindow *
ui_state_footer_window(void) {
    return footer_window;
}

void
ui_state_set_screen_size(int64 width, int64 height) {
    screen_width = width;
    screen_height = height;
    return;
}

int64
ui_state_screen_width(void) {
    return screen_width;
}

int64
ui_state_screen_height(void) {
    return screen_height;
}

void
ui_state_set_main_geometry(int64 start_y, int64 height) {
    main_start_y = start_y;
    main_height = height;
    return;
}

int64
ui_state_main_start_y(void) {
    return main_start_y;
}

int64
ui_state_main_height(void) {
    return main_height;
}

void
ui_state_set_header_height(int64 value) {
    header_height = value;
    return;
}

int64
ui_state_header_height(void) {
    return header_height;
}

void
ui_state_set_footer_height(int64 value) {
    footer_height = value;
    return;
}

int64
ui_state_footer_height(void) {
    return footer_height;
}

void
ui_state_set_footer_start_y(int64 value) {
    footer_start_y = value;
    return;
}

int64
ui_state_footer_start_y(void) {
    return footer_start_y;
}

void
ui_state_set_statusbar_visibility_baseline(bool value) {
    statusbar_visibility_baseline = value;
    return;
}

bool
ui_state_statusbar_visibility_baseline(void) {
    return statusbar_visibility_baseline;
}

#endif /* NCMPCPP_UI_STATE_C */
