#include "ui_state.h"

static void *header_window;
static void *footer_window;
static int64 screen_width;
static int64 screen_height;
static int64 main_start_y;
static int64 main_height;

void
ui_state_set_header_window(void *window) {
    header_window = window;
    return;
}

void *
ui_state_header_window(void) {
    return header_window;
}

void
ui_state_set_footer_window(void *window) {
    footer_window = window;
    return;
}

void *
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

void
ui_state_set_main_start_y(int64 value) {
    main_start_y = value;
    return;
}

int64
ui_state_main_start_y(void) {
    return main_start_y;
}

void
ui_state_set_main_height(int64 value) {
    main_height = value;
    return;
}

int64
ui_state_main_height(void) {
    return main_height;
}
