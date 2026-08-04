#if !defined(NCMPCPP_NC_OUTPUTS_C)
#define NCMPCPP_NC_OUTPUTS_C

#include "cbase.h"

#include "screens/nc_outputs.h"

static NcMenuItemCallbacks nc_outputs_item_callbacks(void);
static NcMenuDisplayCallbacks nc_outputs_display_callbacks(void);
static void nc_outputs_switch_to(NcScreen *screen);
static void nc_outputs_resize(NcScreen *screen);
static void nc_outputs_mouse_button_pressed(NcScreen *screen,
                                            MEVENT event);
static void nc_outputs_destroy_callback(NcScreen *screen);
static void nc_outputs_item_construct(void *dest, void *user);
static void nc_outputs_item_copy(void *dest, void *source, void *user);
static void nc_outputs_item_destroy(void *item, void *user);
static void nc_outputs_draw_item(NcMenu *menu, NcWindow *window,
                                 void *item, int32 pos, void *user);
static void nc_outputs_display(NcOutputsScreen *outputs);
static bool nc_outputs_mouse_scroll(NcOutputsScreen *outputs,
                                    MEVENT event);

#define NC_SCREEN_IMPL_TYPE NcOutputsScreen
#define NC_SCREEN_IMPL_PREFIX nc_outputs
#define NC_SCREEN_IMPL_PUBLIC_PREFIX nc_outputs_screen
#define NC_SCREEN_IMPL_BASE_FIELD menu_screen
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_SCROLLPAD_BASE menu_screen
#define NC_SCREEN_IMPL_MENU(screen) (&(screen)->menu)
#define NC_SCREEN_IMPL_REFRESH_CALLBACK nc_outputs_display
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK nc_outputs_switch_to
#define NC_SCREEN_IMPL_RESIZE_CALLBACK nc_outputs_resize
#define NC_SCREEN_IMPL_TITLE_LITERAL "Outputs"
#define NC_SCREEN_IMPL_MOUSE_CALLBACK nc_outputs_mouse_button_pressed
#define NC_SCREEN_IMPL_DESTROY_CALLBACK nc_outputs_destroy_callback
#define NC_SCREEN_IMPL_LOCKABLE true
#define NC_SCREEN_IMPL_MERGABLE true
#include "screens/nc_screen_impl_template.c"

void
nc_outputs_screen_init(NcOutputsScreen *screen,
                       NcOutputsHooks hooks,
                       int32 start_x, int32 width,
                       int32 main_start_y, int32 main_height,
                       NcColor color, NcBorder border,
                       int32 lines_scrolled,
                       bool mouse_scroll_whole_page) {
    screen->hooks = hooks;
    screen->lines_scrolled = lines_scrolled;
    screen->mouse_scroll_whole_page = mouse_scroll_whole_page;
    nc_scrollpad_screen_init(&screen->menu_screen,
                             nc_outputs_ops,
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
                   STRLIT(""),
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
nc_outputs_screen_set_geometry(NcOutputsScreen *screen,
                               int32 start_x, int32 width,
                               int32 main_start_y, int32 main_height) {
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
                             int32 id,
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

    if ((output = nc_menu_current_item(&screen->menu)) == NULL) {
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
        dest_item->name = malloc2(source_item->name_len + 1);
        memcpy64(dest_item->name,
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
    if (output->name) {
        free2(output->name, output->name_len + 1);
    }
    *output = (NcOutputsItem){0};
    return;
}

static void
nc_outputs_draw_item(NcMenu *menu, NcWindow *window,
                     void *item, int32 pos, void *user) {
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
    int32 count;

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

    for (int32 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(&outputs->menu,
                                  nc_window_height(&outputs->window),
                                  scroll);
    }
    return true;
}

#endif /* NCMPCPP_NC_OUTPUTS_C */
