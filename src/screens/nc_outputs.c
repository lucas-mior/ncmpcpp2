#include "screens/nc_outputs.h"

#include <stddef.h>

#include "cbase/base_macros.h"
#include "cbase/cbase.h"

static NcOutputsScreen *nc_outputs_from_screen(NcScreen *screen);
static NcScreenCallbacks nc_outputs_callbacks(void);
static NcMenuItemCallbacks nc_outputs_item_callbacks(void);
static NcMenuDisplayCallbacks nc_outputs_display_callbacks(void);
static NcWindow *nc_outputs_active_window(NcScreen *screen);
static void nc_outputs_refresh(NcScreen *screen);
static void nc_outputs_refresh_window(NcScreen *screen);
static void nc_outputs_scroll(NcScreen *screen, enum NcScroll where);
static void nc_outputs_switch_to(NcScreen *screen);
static void nc_outputs_resize(NcScreen *screen);
static int32 nc_outputs_window_timeout(NcScreen *screen);
static char *nc_outputs_title(NcScreen *screen);
static void nc_outputs_update(NcScreen *screen);
static void nc_outputs_mouse_button_pressed(NcScreen *screen,
                                            MEVENT event);
static bool nc_outputs_is_lockable(NcScreen *screen);
static bool nc_outputs_is_mergable(NcScreen *screen);
static void nc_outputs_destroy_callback(NcScreen *screen);
static void nc_outputs_item_construct(void *dest, void *user);
static void nc_outputs_item_copy(void *dest, void *source, void *user);
static void nc_outputs_item_destroy(void *item, void *user);
static void nc_outputs_draw_item(NcMenu *menu, NcWindow *window,
                                 void *item, int64 pos, void *user);
static void nc_outputs_display(NcOutputsScreen *outputs);
static bool nc_outputs_mouse_scroll(NcOutputsScreen *outputs,
                                    MEVENT event);

void
nc_outputs_screen_init(NcOutputsScreen *screen,
                       NcOutputsHooks hooks,
                       int64 start_x, int64 width,
                       int64 main_start_y, int64 main_height,
                       NcColor color, NcBorder border,
                       int64 lines_scrolled,
                       bool mouse_scroll_whole_page) {
    screen->hooks = hooks;
    screen->lines_scrolled = lines_scrolled;
    screen->mouse_scroll_whole_page = mouse_scroll_whole_page;
    nc_scrollpad_screen_init(&screen->menu_screen,
                             nc_outputs_callbacks(),
                             hooks.user,
                             NC_SCREEN_TYPE_OUTPUTS,
                             0,
                             0,
                             0,
                             0);
    nc_outputs_screen_set_geometry(screen,
                                   start_x,
                                   width,
                                   main_start_y,
                                   main_height);
    nc_window_init(&screen->window,
                   nc_outputs_screen_start_x(screen),
                   nc_outputs_screen_start_y(screen),
                   nc_outputs_screen_width(screen),
                   nc_outputs_screen_height(screen),
                   STRLIT_ARGS(""),
                   color,
                   border);
    nc_menu_init(&screen->menu);
    nc_menu_set_item_callbacks(&screen->menu,
                               nc_outputs_item_callbacks());
    nc_menu_set_display_callbacks(&screen->menu,
                                  nc_outputs_display_callbacks());
    nc_menu_set_cyclic_scrolling(&screen->menu, false);
    nc_menu_set_centered_cursor(&screen->menu, false);
    return;
}

void
nc_outputs_screen_destroy(NcOutputsScreen *screen) {
    nc_menu_destroy(&screen->menu);
    nc_window_destroy(&screen->window);
    return;
}

void
nc_outputs_screen_set_geometry(NcOutputsScreen *screen,
                               int64 start_x, int64 width,
                               int64 main_start_y, int64 main_height) {
    nc_scrollpad_screen_set_main_area(&screen->menu_screen,
                                      start_x,
                                      width,
                                      main_start_y,
                                      main_height);
    return;
}

void
nc_outputs_screen_set_highlight_prefix(NcOutputsScreen *screen,
                                       NcBuffer *buffer) {
    nc_menu_set_highlight_prefix(&screen->menu, buffer);
    return;
}

void
nc_outputs_screen_set_highlight_suffix(NcOutputsScreen *screen,
                                       NcBuffer *buffer) {
    nc_menu_set_highlight_suffix(&screen->menu, buffer);
    return;
}

void
nc_outputs_screen_fetch_list(NcOutputsScreen *screen) {
    nc_outputs_screen_clear_outputs(screen);
    if (screen->hooks.fetch_outputs) {
        screen->hooks.fetch_outputs(screen->hooks.user, screen);
    }
    nc_menu_sync_item_count(&screen->menu);
    return;
}

void
nc_outputs_screen_clear_outputs(NcOutputsScreen *screen) {
    nc_menu_clear_items(&screen->menu);
    return;
}

void
nc_outputs_screen_add_output(NcOutputsScreen *screen,
                             uint32 id,
                             char *name,
                             int32 name_len,
                             bool enabled) {
    NcOutputsItem item;

    item.name = name;
    item.name_len = name_len;
    item.id = id;
    item.enabled = enabled;
    nc_menu_add_item(&screen->menu, &item);
    return;
}

bool
nc_outputs_screen_toggle_current(NcOutputsScreen *screen) {
    NcOutputsItem *output;
    bool result;

    output = nc_menu_current_item(&screen->menu);
    if (output == NULL) {
        return false;
    }
    if (screen->hooks.toggle_output == NULL) {
        return false;
    }

    result = screen->hooks.toggle_output(screen->hooks.user,
                                         output->id,
                                         output->enabled,
                                         output->name,
                                         output->name_len);
    if (result) {
        output->enabled = !output->enabled;
        nc_outputs_refresh_window(nc_outputs_screen_base(screen));
    }
    return result;
}

NcScreen *
nc_outputs_screen_base(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_base(&screen->menu_screen);
}

NcWindow *
nc_outputs_screen_window(NcOutputsScreen *screen) {
    return &screen->window;
}

int64
nc_outputs_screen_start_x(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_start_x(&screen->menu_screen);
}

int64
nc_outputs_screen_start_y(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_start_y(&screen->menu_screen);
}

int64
nc_outputs_screen_width(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_width(&screen->menu_screen);
}

int64
nc_outputs_screen_height(NcOutputsScreen *screen) {
    return nc_scrollpad_screen_height(&screen->menu_screen);
}

static NcOutputsScreen *
nc_outputs_from_screen(NcScreen *screen) {
    return (NcOutputsScreen *)screen;
}

static NcScreenCallbacks
nc_outputs_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = nc_outputs_active_window;
    callbacks.refresh = nc_outputs_refresh;
    callbacks.refresh_window = nc_outputs_refresh_window;
    callbacks.scroll = nc_outputs_scroll;
    callbacks.switch_to = nc_outputs_switch_to;
    callbacks.resize = nc_outputs_resize;
    callbacks.window_timeout = nc_outputs_window_timeout;
    callbacks.title = nc_outputs_title;
    callbacks.update = nc_outputs_update;
    callbacks.mouse_button_pressed = nc_outputs_mouse_button_pressed;
    callbacks.is_lockable = nc_outputs_is_lockable;
    callbacks.is_mergable = nc_outputs_is_mergable;
    callbacks.destroy = nc_outputs_destroy_callback;
    return callbacks;
}

static NcMenuItemCallbacks
nc_outputs_item_callbacks(void) {
    NcMenuItemCallbacks callbacks = {0};

    callbacks.item_size = SIZEOF(NcOutputsItem);
    callbacks.construct = nc_outputs_item_construct;
    callbacks.copy = nc_outputs_item_copy;
    callbacks.destroy = nc_outputs_item_destroy;
    callbacks.user = NULL;
    return callbacks;
}

static NcMenuDisplayCallbacks
nc_outputs_display_callbacks(void) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.draw = nc_outputs_draw_item;
    callbacks.user = NULL;
    return callbacks;
}

static NcWindow *
nc_outputs_active_window(NcScreen *screen) {
    return &nc_outputs_from_screen(screen)->window;
}

static void
nc_outputs_refresh(NcScreen *screen) {
    nc_outputs_display(nc_outputs_from_screen(screen));
    return;
}

static void
nc_outputs_refresh_window(NcScreen *screen) {
    nc_outputs_display(nc_outputs_from_screen(screen));
    return;
}

static void
nc_outputs_scroll(NcScreen *screen, enum NcScroll where) {
    NcOutputsScreen *outputs;

    outputs = nc_outputs_from_screen(screen);
    nc_menu_scroll_selectable(&outputs->menu,
                              nc_window_height(&outputs->window),
                              where);
    return;
}

static void
nc_outputs_switch_to(NcScreen *screen) {
    NcOutputsScreen *outputs;

    outputs = nc_outputs_from_screen(screen);
    if (outputs->hooks.switch_to) {
        outputs->hooks.switch_to(outputs->hooks.user);
    }
    return;
}

static void
nc_outputs_resize(NcScreen *screen) {
    NcOutputsScreen *outputs;

    outputs = nc_outputs_from_screen(screen);
    if (outputs->hooks.resize_layout) {
        outputs->hooks.resize_layout(outputs->hooks.user, outputs);
    }
    nc_window_resize(&outputs->window,
                     nc_outputs_screen_width(outputs),
                     nc_outputs_screen_height(outputs));
    nc_window_move_to(&outputs->window,
                      nc_outputs_screen_start_x(outputs),
                      nc_outputs_screen_start_y(outputs));
    if (outputs->hooks.resize_background) {
        outputs->hooks.resize_background(outputs->hooks.user);
    }
    return;
}

static int32
nc_outputs_window_timeout(NcScreen *screen) {
    (void)screen;
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
nc_outputs_title(NcScreen *screen) {
    static char title[] = "Outputs";

    (void)screen;
    return title;
}

static void
nc_outputs_update(NcScreen *screen) {
    (void)screen;
    return;
}

static void
nc_outputs_mouse_button_pressed(NcScreen *screen, MEVENT event) {
    NcOutputsScreen *outputs;
    int32 x;
    int32 y;

    outputs = nc_outputs_from_screen(screen);
    if (nc_outputs_mouse_scroll(outputs, event)) {
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_menu_empty(&outputs->menu)
        || !nc_window_has_coords(&outputs->window, &x, &y)
        || (y < 0)
        || (y >= nc_menu_item_count(&outputs->menu))) {
        return;
    }

    if ((event.bstate & BUTTON1_PRESSED)
        || (event.bstate & BUTTON3_PRESSED)) {
        nc_menu_goto_selectable(&outputs->menu, y);
        if (event.bstate & BUTTON3_PRESSED) {
            nc_outputs_screen_toggle_current(outputs);
        }
    }
    return;
}

static bool
nc_outputs_is_lockable(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
nc_outputs_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
nc_outputs_destroy_callback(NcScreen *screen) {
    NcOutputsScreen *outputs;

    outputs = nc_outputs_from_screen(screen);
    if (outputs->hooks.destroy) {
        outputs->hooks.destroy(outputs->hooks.user);
    }
    return;
}

static void
nc_outputs_item_construct(void *dest, void *user) {
    NcOutputsItem *item;

    (void)user;
    item = dest;
    *item = (NcOutputsItem){0};
    return;
}

static void
nc_outputs_item_copy(void *dest, void *source, void *user) {
    NcOutputsItem *dest_item;
    NcOutputsItem *source_item;

    (void)user;
    dest_item = dest;
    source_item = source;
    *dest_item = *source_item;
    dest_item->name = NULL;
    if (source_item->name_len > 0) {
        dest_item->name = cbase_malloc(source_item->name_len + 1);
        cbase_memcpy(dest_item->name,
                     source_item->name,
                     source_item->name_len);
        dest_item->name[source_item->name_len] = '\0';
    }
    return;
}

static void
nc_outputs_item_destroy(void *item, void *user) {
    NcOutputsItem *output;

    (void)user;
    output = item;
    if (output->name != NULL) {
        cbase_free(output->name, output->name_len + 1);
    }
    *output = (NcOutputsItem){0};
    return;
}

static void
nc_outputs_draw_item(NcMenu *menu, NcWindow *window,
                     void *item, int64 pos, void *user) {
    NcOutputsItem *output;

    (void)menu;
    (void)pos;
    (void)user;
    output = item;
    if (output->enabled) {
        nc_window_apply_format(window, NC_FORMAT_BOLD);
    }
    nc_window_print_data(window, output->name, output->name_len);
    if (output->enabled) {
        nc_window_apply_format(window, NC_FORMAT_NO_BOLD);
    }
    return;
}

static void
nc_outputs_display(NcOutputsScreen *outputs) {
    nc_window_refresh_border(&outputs->window);
    nc_menu_refresh(&outputs->menu,
                    &outputs->window,
                    nc_window_width(&outputs->window),
                    nc_window_height(&outputs->window));
    nc_menu_refresh(&outputs->menu,
                    &outputs->window,
                    nc_window_width(&outputs->window),
                    nc_window_height(&outputs->window));
    return;
}

static bool
nc_outputs_mouse_scroll(NcOutputsScreen *outputs, MEVENT event) {
    enum NcScroll scroll;
    int64 count;

    if (event.bstate & BUTTON5_PRESSED) {
        scroll = NC_SCROLL_DOWN;
    } else if (event.bstate & BUTTON4_PRESSED) {
        scroll = NC_SCROLL_UP;
    } else {
        return false;
    }

    if (outputs->mouse_scroll_whole_page) {
        if (scroll == NC_SCROLL_DOWN) {
            scroll = NC_SCROLL_PAGE_DOWN;
        } else {
            scroll = NC_SCROLL_PAGE_UP;
        }
        count = 1;
    } else {
        count = outputs->lines_scrolled;
    }

    for (int64 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(&outputs->menu,
                                  nc_window_height(&outputs->window),
                                  scroll);
    }
    return true;
}
