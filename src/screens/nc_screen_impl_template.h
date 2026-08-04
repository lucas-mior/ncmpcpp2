#if !defined(NC_SCREEN_IMPL_TYPE)
#error "NC_SCREEN_IMPL_TYPE is undefined"
#endif
#if !defined(NC_SCREEN_IMPL_PREFIX)
#error "NC_SCREEN_IMPL_PREFIX is undefined"
#endif
#if !defined(NC_SCREEN_IMPL_PUBLIC_PREFIX)
#error "NC_SCREEN_IMPL_PUBLIC_PREFIX is undefined"
#endif
#if !defined(NC_SCREEN_IMPL_BASE_FIELD)
#error "NC_SCREEN_IMPL_BASE_FIELD is undefined"
#endif
#if !defined(NC_SCREEN_IMPL_WINDOW_FIELD) \
    && !defined(NC_SCREEN_IMPL_WINDOW)
#error "screen implementation needs a window field or window expression"
#endif
#if !defined(NC_SCREEN_IMPL_SCROLL_CALLBACK) \
    && !defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD) \
    && !defined(NC_SCREEN_IMPL_SCROLL_MENU) \
    && !defined(NC_SCREEN_IMPL_MENU)
#error "screen implementation needs a scroll callback or scrollable object"
#endif
#if !defined(NC_SCREEN_IMPL_REFRESH_CALLBACK)
#error "NC_SCREEN_IMPL_REFRESH_CALLBACK is undefined"
#endif

#if !defined(NC_SCREEN_IMPL_FIRST_FIELD)
#define NC_SCREEN_IMPL_FIRST_FIELD NC_SCREEN_IMPL_BASE_FIELD
#endif
#if !defined(NC_SCREEN_IMPL_LOCKABLE)
#define NC_SCREEN_IMPL_LOCKABLE false
#endif
#if !defined(NC_SCREEN_IMPL_MERGABLE)
#define NC_SCREEN_IMPL_MERGABLE false
#endif
#if !defined(NC_SCREEN_IMPL_WINDOW)
#define NC_SCREEN_IMPL_WINDOW(screen) \
    (&(screen)->NC_SCREEN_IMPL_WINDOW_FIELD)
#endif
#if !defined(NC_SCREEN_IMPL_SCROLL_MENU) \
    && defined(NC_SCREEN_IMPL_MENU)
#define NC_SCREEN_IMPL_SCROLL_MENU(screen) NC_SCREEN_IMPL_MENU(screen)
#endif
#if !defined(NC_SCREEN_IMPL_SCROLL_HEIGHT) \
    && defined(NC_SCREEN_IMPL_SCROLL_MENU)
#define NC_SCREEN_IMPL_SCROLL_HEIGHT(screen) \
    nc_window_height(NC_SCREEN_IMPL_WINDOW(screen))
#endif
#if (defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD) \
     || defined(NC_SCREEN_IMPL_SCROLLPAD_BASE)) \
    && !defined(NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR)
#if defined(NC_SCREEN_IMPL_SCROLLPAD_BASE)
#define NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen) \
    (&(screen)->NC_SCREEN_IMPL_SCROLLPAD_BASE)
#else
#define NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen) \
    (&(screen)->NC_SCREEN_IMPL_BASE_FIELD)
#endif
#endif
#if !defined(NC_SCREEN_IMPL_BASE_EXPR)
#if defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD) \
    || defined(NC_SCREEN_IMPL_SCROLLPAD_BASE)
#define NC_SCREEN_IMPL_BASE_EXPR(screen) \
    nc_scrollpad_screen_base(NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen))
#else
#define NC_SCREEN_IMPL_BASE_EXPR(screen) \
    (&(screen)->NC_SCREEN_IMPL_BASE_FIELD)
#endif
#endif

#define NC_SCREEN_IMPL_FROM_SCREEN CAT(NC_SCREEN_IMPL_PREFIX, _from_screen)
#define NC_SCREEN_IMPL_OPS CAT(NC_SCREEN_IMPL_PREFIX, _ops)
#define NC_SCREEN_IMPL_ACTIVE_WINDOW \
    CAT(NC_SCREEN_IMPL_PREFIX, _active_window)
#define NC_SCREEN_IMPL_REFRESH CAT(NC_SCREEN_IMPL_PREFIX, _refresh)
#define NC_SCREEN_IMPL_REFRESH_WINDOW \
    CAT(NC_SCREEN_IMPL_PREFIX, _refresh_window)
#define NC_SCREEN_IMPL_SCROLL CAT(NC_SCREEN_IMPL_PREFIX, _scroll)
#define NC_SCREEN_IMPL_TITLE CAT(NC_SCREEN_IMPL_PREFIX, _title)
#define NC_SCREEN_IMPL_DESTROY CAT(NC_SCREEN_IMPL_PREFIX, _destroy)
#define NC_SCREEN_IMPL_BASE CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _base)
#define NC_SCREEN_IMPL_START_X CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _start_x)
#define NC_SCREEN_IMPL_START_Y CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _start_y)
#define NC_SCREEN_IMPL_WIDTH CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _width)
#define NC_SCREEN_IMPL_HEIGHT CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _height)

_Static_assert(OFFSET_OF(NC_SCREEN_IMPL_TYPE, NC_SCREEN_IMPL_FIRST_FIELD) == 0,
               "screen first field must start at offset zero");

NcScreen *
NC_SCREEN_IMPL_BASE(NC_SCREEN_IMPL_TYPE *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return NC_SCREEN_IMPL_BASE_EXPR(screen);
}

#if (defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD) \
     || defined(NC_SCREEN_IMPL_SCROLLPAD_BASE)) \
    && !defined(NC_SCREEN_IMPL_NO_GEOMETRY_ACCESSORS)
int32
NC_SCREEN_IMPL_START_X(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_start_x(
        NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen));
}

int32
NC_SCREEN_IMPL_START_Y(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_start_y(
        NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen));
}

int32
NC_SCREEN_IMPL_WIDTH(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_width(
        NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen));
}

int32
NC_SCREEN_IMPL_HEIGHT(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_height(
        NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen));
}
#endif

static NC_SCREEN_IMPL_TYPE *
NC_SCREEN_IMPL_FROM_SCREEN(NcScreen *screen) {
    return (NC_SCREEN_IMPL_TYPE *)screen;
}

static NcWindow *
NC_SCREEN_IMPL_ACTIVE_WINDOW(NcScreen *screen) {
    NC_SCREEN_IMPL_TYPE *impl;

    impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);
    return NC_SCREEN_IMPL_WINDOW(impl);
}

static void
NC_SCREEN_IMPL_REFRESH(NcScreen *screen) {
    NC_SCREEN_IMPL_REFRESH_CALLBACK(NC_SCREEN_IMPL_FROM_SCREEN(screen));
    return;
}

static void
NC_SCREEN_IMPL_REFRESH_WINDOW(NcScreen *screen) {
    NC_SCREEN_IMPL_REFRESH(screen);
    return;
}

#if defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)
static void
NC_SCREEN_IMPL_SCROLL(NcScreen *screen, enum NcScroll where) {
    NC_SCREEN_IMPL_TYPE *impl;

    impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);
    nc_scrollpad_scroll(&impl->NC_SCREEN_IMPL_SCROLLPAD_FIELD,
                        NC_SCREEN_IMPL_WINDOW(impl),
                        where);
    return;
}
#elif defined(NC_SCREEN_IMPL_SCROLL_MENU)
static void
NC_SCREEN_IMPL_SCROLL(NcScreen *screen, enum NcScroll where) {
    NC_SCREEN_IMPL_TYPE *impl;

    impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);
    nc_menu_scroll_selectable(NC_SCREEN_IMPL_SCROLL_MENU(impl),
                              NC_SCREEN_IMPL_SCROLL_HEIGHT(impl),
                              where);
    return;
}
#endif

#if defined(NC_SCREEN_IMPL_TITLE_LITERAL)
static char *
NC_SCREEN_IMPL_TITLE(NcScreen *screen) {
    static char title[] = NC_SCREEN_IMPL_TITLE_LITERAL;

    (void)screen;
    return title;
}
#endif

#if defined(NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK)
static void
NC_SCREEN_IMPL_DESTROY(NcScreen *screen) {
    NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK(NC_SCREEN_IMPL_FROM_SCREEN(screen));
    return;
}
#endif

static const NcScreenOps NC_SCREEN_IMPL_OPS = {
    .active_window = NC_SCREEN_IMPL_ACTIVE_WINDOW,
    .refresh = NC_SCREEN_IMPL_REFRESH,
    .refresh_window = NC_SCREEN_IMPL_REFRESH_WINDOW,
#if defined(NC_SCREEN_IMPL_SCROLL_CALLBACK)
    .scroll = NC_SCREEN_IMPL_SCROLL_CALLBACK,
#elif defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD) \
      || defined(NC_SCREEN_IMPL_SCROLL_MENU)
    .scroll = NC_SCREEN_IMPL_SCROLL,
#endif
#if defined(NC_SCREEN_IMPL_LIST_CHANGE_FINISHED_CALLBACK)
    .list_change_finished = NC_SCREEN_IMPL_LIST_CHANGE_FINISHED_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_CAN_RUN_CURRENT_CALLBACK)
    .can_run_current = NC_SCREEN_IMPL_CAN_RUN_CURRENT_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_RUN_CURRENT_CALLBACK)
    .run_current = NC_SCREEN_IMPL_RUN_CURRENT_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_SWITCH_TO_CALLBACK)
    .switch_to = NC_SCREEN_IMPL_SWITCH_TO_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_RESIZE_CALLBACK)
    .resize = NC_SCREEN_IMPL_RESIZE_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_WINDOW_TIMEOUT_CALLBACK)
    .window_timeout_callback = NC_SCREEN_IMPL_WINDOW_TIMEOUT_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_TITLE_LITERAL)
    .title = NC_SCREEN_IMPL_TITLE,
#endif
#if defined(NC_SCREEN_IMPL_TITLE_CALLBACK)
    .title = NC_SCREEN_IMPL_TITLE_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_UPDATE_CALLBACK)
    .update = NC_SCREEN_IMPL_UPDATE_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_MOUSE_CALLBACK)
    .mouse_button_pressed = NC_SCREEN_IMPL_MOUSE_CALLBACK,
#endif
    .lockable = NC_SCREEN_IMPL_LOCKABLE,
    .mergable = NC_SCREEN_IMPL_MERGABLE,
#if defined(NC_SCREEN_IMPL_DESTROY_CALLBACK)
    .destroy = NC_SCREEN_IMPL_DESTROY_CALLBACK,
#elif defined(NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK)
    .destroy = NC_SCREEN_IMPL_DESTROY,
#endif
};

#undef NC_SCREEN_IMPL_HEIGHT
#undef NC_SCREEN_IMPL_WIDTH
#undef NC_SCREEN_IMPL_START_Y
#undef NC_SCREEN_IMPL_START_X
#undef NC_SCREEN_IMPL_BASE
#undef NC_SCREEN_IMPL_DESTROY
#undef NC_SCREEN_IMPL_TITLE
#undef NC_SCREEN_IMPL_SCROLL
#undef NC_SCREEN_IMPL_REFRESH_WINDOW
#undef NC_SCREEN_IMPL_REFRESH
#undef NC_SCREEN_IMPL_ACTIVE_WINDOW
#undef NC_SCREEN_IMPL_OPS
#undef NC_SCREEN_IMPL_FROM_SCREEN
#undef NC_SCREEN_IMPL_BASE_EXPR
#undef NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR
#undef NC_SCREEN_IMPL_NO_GEOMETRY_ACCESSORS
#undef NC_SCREEN_IMPL_SCROLL_HEIGHT
#undef NC_SCREEN_IMPL_SCROLL_MENU
#undef NC_SCREEN_IMPL_WINDOW
#undef NC_SCREEN_IMPL_MERGABLE
#undef NC_SCREEN_IMPL_LOCKABLE
#undef NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK
#undef NC_SCREEN_IMPL_DESTROY_CALLBACK
#undef NC_SCREEN_IMPL_MOUSE_CALLBACK
#undef NC_SCREEN_IMPL_UPDATE_CALLBACK
#undef NC_SCREEN_IMPL_TITLE_CALLBACK
#undef NC_SCREEN_IMPL_WINDOW_TIMEOUT_CALLBACK
#undef NC_SCREEN_IMPL_TITLE_LITERAL
#undef NC_SCREEN_IMPL_RESIZE_CALLBACK
#undef NC_SCREEN_IMPL_SWITCH_TO_CALLBACK
#undef NC_SCREEN_IMPL_RUN_CURRENT_CALLBACK
#undef NC_SCREEN_IMPL_CAN_RUN_CURRENT_CALLBACK
#undef NC_SCREEN_IMPL_LIST_CHANGE_FINISHED_CALLBACK
#undef NC_SCREEN_IMPL_REFRESH_CALLBACK
#undef NC_SCREEN_IMPL_SCROLLPAD_BASE
#undef NC_SCREEN_IMPL_SCROLLPAD_FIELD
#undef NC_SCREEN_IMPL_SCROLL_CALLBACK
#undef NC_SCREEN_IMPL_MENU
#undef NC_SCREEN_IMPL_WINDOW_FIELD
#undef NC_SCREEN_IMPL_FIRST_FIELD
#undef NC_SCREEN_IMPL_BASE_FIELD
#undef NC_SCREEN_IMPL_PUBLIC_PREFIX
#undef NC_SCREEN_IMPL_PREFIX
#undef NC_SCREEN_IMPL_TYPE
