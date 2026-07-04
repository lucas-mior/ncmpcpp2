#include "curses/nc_scrollpad.h"

#include <assert.h>
#include <ctype.h>
#include <wctype.h>


typedef struct NcScrollpadWriteState {
    NcBuffer *buffer;
    NcBufferProperty *properties;
    NcWindow *window;

    int32 i;
    int32 property_index;
    int32 property_count;
} NcScrollpadWriteState;

static int32 nc_scrollpad_i32(int64 value);
static void nc_scrollpad_load_properties(NcScrollpadWriteState *state);
static void nc_scrollpad_write_whitespace(NcScrollpadWriteState *state);
static void nc_scrollpad_write_word(NcScrollpadWriteState *state,
                                    bool load_properties);
static int64 nc_scrollpad_write_buffer(NcScrollpadWriteState *state,
                                       bool generate_height_only);
static bool nc_scrollpad_is_space(char ch);
static int64 nc_scrollpad_max_beginning(NcScrollpad *scrollpad,
                                        NcWindow *window);

void
nc_scrollpad_init_empty(NcScrollpad *scrollpad) {
    *scrollpad = (NcScrollpad){0};
    return;
}

void
nc_scrollpad_init(NcScrollpad *scrollpad, int64 height) {
    scrollpad->beginning = 0;
    scrollpad->real_height = height;
    return;
}

int64
nc_scrollpad_beginning(NcScrollpad *scrollpad) {
    return scrollpad->beginning;
}

int64
nc_scrollpad_real_height(NcScrollpad *scrollpad) {
    return scrollpad->real_height;
}

void
nc_scrollpad_set_real_height(NcScrollpad *scrollpad,
                             int64 real_height) {
    scrollpad->real_height = real_height;
    return;
}

void
nc_scrollpad_refresh(NcScrollpad *scrollpad, NcWindow *window) {
    int32 start_y;
    int32 start_x;
    int32 end_y;
    int32 end_x;

    assert(scrollpad->real_height >= window->height);

    if (scrollpad->beginning > nc_scrollpad_max_beginning(scrollpad,
                                                           window)) {
        scrollpad->beginning = nc_scrollpad_max_beginning(scrollpad,
                                                          window);
    }

    start_y = nc_scrollpad_i32(window->start_y);
    start_x = nc_scrollpad_i32(window->start_x);
    end_y = nc_scrollpad_i32(window->start_y + window->height - 1);
    end_x = nc_scrollpad_i32(window->start_x + window->width - 1);

    prefresh(window->window, nc_scrollpad_i32(scrollpad->beginning), 0,
             start_y, start_x, end_y, end_x);
    return;
}

void
nc_scrollpad_resize(NcScrollpad *scrollpad, NcWindow *window,
                    int64 new_width, int64 new_height) {
    (void)scrollpad;
    nc_window_adjust_dimensions(window, new_width, new_height);
    nc_window_recreate(window, new_width, new_height);
    return;
}

void
nc_scrollpad_scroll(NcScrollpad *scrollpad, NcWindow *window,
                    enum NcScroll where) {
    int64 max_beginning;

    assert(scrollpad->real_height >= window->height);

    max_beginning = nc_scrollpad_max_beginning(scrollpad, window);
    switch (where) {
    case NC_SCROLL_UP:
        if (scrollpad->beginning > 0) {
            scrollpad->beginning -= 1;
        }
        break;
    case NC_SCROLL_DOWN:
        if (scrollpad->beginning < max_beginning) {
            scrollpad->beginning += 1;
        }
        break;
    case NC_SCROLL_PAGE_UP:
        if (scrollpad->beginning > window->height) {
            scrollpad->beginning -= window->height;
        } else {
            scrollpad->beginning = 0;
        }
        break;
    case NC_SCROLL_PAGE_DOWN:
        scrollpad->beginning += window->height;
        if (scrollpad->beginning > max_beginning) {
            scrollpad->beginning = max_beginning;
        }
        break;
    case NC_SCROLL_HOME:
        scrollpad->beginning = 0;
        break;
    case NC_SCROLL_END:
        scrollpad->beginning = max_beginning;
        break;
    }
    return;
}

void
nc_scrollpad_clear(NcScrollpad *scrollpad, NcWindow *window) {
    scrollpad->real_height = window->height;
    nc_window_clear(window);
    return;
}

void
nc_scrollpad_prepare_flush(NcScrollpad *scrollpad, NcWindow *window,
                           int64 generated_height) {
    scrollpad->real_height = generated_height;
    if (scrollpad->real_height < window->height) {
        scrollpad->real_height = window->height;
    }
    if (scrollpad->real_height > window->height) {
        nc_window_recreate(window, window->width, scrollpad->real_height);
    } else {
        werase(window->window);
    }
    return;
}

void
nc_scrollpad_flush(NcScrollpad *scrollpad, NcWindow *window,
                   NcBuffer *buffer) {
    NcScrollpadWriteState state;
    int64 height;

    state.buffer = buffer;
    state.properties = nc_buffer_properties(buffer);
    state.window = window;
    state.i = 0;
    state.property_index = 0;
    state.property_count = nc_buffer_property_count(buffer);

    height = nc_scrollpad_write_buffer(&state, true);
    nc_scrollpad_prepare_flush(scrollpad, window, height);

    state.i = 0;
    state.property_index = 0;
    state.property_count = nc_buffer_property_count(buffer);
    state.properties = nc_buffer_properties(buffer);
    nc_scrollpad_write_buffer(&state, false);
    return;
}

void
nc_scrollpad_reset(NcScrollpad *scrollpad) {
    scrollpad->beginning = 0;
    return;
}

static int32
nc_scrollpad_i32(int64 value) {
    return (int32)value;
}

static int64
nc_scrollpad_max_beginning(NcScrollpad *scrollpad, NcWindow *window) {
    return scrollpad->real_height - window->height;
}


static void
nc_scrollpad_load_properties(NcScrollpadWriteState *state) {
    while ((state->property_index < state->property_count)
           && (state->properties[state->property_index].position
               == state->i)) {
        nc_buffer_apply_property(
            state->window, &state->properties[state->property_index]);
        state->property_index += 1;
    }
    return;
}

static void
nc_scrollpad_write_whitespace(NcScrollpadWriteState *state) {
    char *data;
    int32 len;

    data = nc_buffer_data(state->buffer);
    len = nc_buffer_len(state->buffer);
    while ((state->i < len) && nc_scrollpad_is_space(data[state->i])) {
        nc_scrollpad_load_properties(state);
        nc_window_print_char(state->window, data[state->i]);
        state->i += 1;
    }
    return;
}

static void
nc_scrollpad_write_word(NcScrollpadWriteState *state,
                         bool load_properties) {
    char *data;
    int32 len;

    data = nc_buffer_data(state->buffer);
    len = nc_buffer_len(state->buffer);
    while ((state->i < len) && !nc_scrollpad_is_space(data[state->i])) {
        if (load_properties) {
            nc_scrollpad_load_properties(state);
        }
        nc_window_print_char(state->window, data[state->i]);
        state->i += 1;
    }
    return;
}

static int64
nc_scrollpad_write_buffer(NcScrollpadWriteState *state,
                          bool generate_height_only) {
    int32 new_y;
    int32 x;
    int32 y;
    int32 old_i;
    int32 old_property_index;
    int64 height;
    int32 len;

    state->i = 0;
    state->property_index = 0;
    len = nc_buffer_len(state->buffer);
    height = 1;
    y = nc_window_get_y(state->window);

    while (state->i < len) {
        nc_scrollpad_write_whitespace(state);

        if (generate_height_only) {
            new_y = nc_window_get_y(state->window);
            height += new_y - y;
            y = new_y;
        }

        if (state->i == len) {
            break;
        }

        old_i = state->i;
        old_property_index = state->property_index;
        x = nc_window_get_x(state->window);
        y = nc_window_get_y(state->window);

        nc_scrollpad_write_word(state, false);

        state->i = old_i;
        state->property_index = old_property_index;

        new_y = nc_window_get_y(state->window);
        if (new_y != y) {
            if (generate_height_only) {
                height += 1;
            } else {
                nc_window_go_to_xy(state->window, x, y);
                wclrtoeol(state->window->window);
            }

            y += 1;
            nc_window_go_to_xy(state->window, 0, y);
            nc_scrollpad_write_word(state, true);

            if (generate_height_only) {
                new_y = nc_window_get_y(state->window);
                height += new_y - y;
            }
        } else {
            nc_window_go_to_xy(state->window, x, y);
            nc_scrollpad_write_word(state, true);
        }

        if (generate_height_only) {
            nc_window_go_to_xy(state->window, nc_window_get_x(state->window),
                               0);
            y = 0;
        }
    }

    while (state->property_index < state->property_count) {
        nc_buffer_apply_property(
            state->window, &state->properties[state->property_index]);
        state->property_index += 1;
    }

    return height;
}

static bool
nc_scrollpad_is_space(char ch) {
    return iswspace((wint_t)(unsigned char)ch);
}
