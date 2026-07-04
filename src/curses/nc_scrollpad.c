#include "curses/nc_scrollpad.h"

#include <assert.h>


static int32 nc_scrollpad_i32(int64 value);
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
