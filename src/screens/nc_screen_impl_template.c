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
#if !defined(NC_SCREEN_IMPL_WINDOW_FIELD)
#error "NC_SCREEN_IMPL_WINDOW_FIELD is undefined"
#endif
#if !defined(NC_SCREEN_IMPL_SCROLL_CALLBACK) \
    && !defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)
#error "screen implementation needs a scroll callback or scrollpad field"
#endif
#if !defined(NC_SCREEN_IMPL_REFRESH_CALLBACK)
#error "NC_SCREEN_IMPL_REFRESH_CALLBACK is undefined"
#endif

#if !defined(NC_SCREEN_IMPL_LOCKABLE)
#define NC_SCREEN_IMPL_LOCKABLE false
#endif
#if !defined(NC_SCREEN_IMPL_MERGABLE)
#define NC_SCREEN_IMPL_MERGABLE false
#endif

#define NC_SCREEN_IMPL_FROM_SCREEN CAT(NC_SCREEN_IMPL_PREFIX, _from_screen)
#define NC_SCREEN_IMPL_OPS CAT(NC_SCREEN_IMPL_PREFIX, _ops)
#define NC_SCREEN_IMPL_ACTIVE_WINDOW \
    CAT(NC_SCREEN_IMPL_PREFIX, _active_window)
#define NC_SCREEN_IMPL_REFRESH CAT(NC_SCREEN_IMPL_PREFIX, _refresh)
#define NC_SCREEN_IMPL_SCROLL CAT(NC_SCREEN_IMPL_PREFIX, _scroll)
#define NC_SCREEN_IMPL_TITLE CAT(NC_SCREEN_IMPL_PREFIX, _title)
#define NC_SCREEN_IMPL_BASE CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _base)
#define NC_SCREEN_IMPL_START_X CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _start_x)
#define NC_SCREEN_IMPL_START_Y CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _start_y)
#define NC_SCREEN_IMPL_WIDTH CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _width)
#define NC_SCREEN_IMPL_HEIGHT CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _height)

_Static_assert(OFFSET_OF(NC_SCREEN_IMPL_TYPE, NC_SCREEN_IMPL_BASE_FIELD) == 0,
               "screen base field must be the first field");

NcScreen *
NC_SCREEN_IMPL_BASE(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_base(&screen->NC_SCREEN_IMPL_BASE_FIELD);
}

int32
NC_SCREEN_IMPL_START_X(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_start_x(&screen->NC_SCREEN_IMPL_BASE_FIELD);
}

int32
NC_SCREEN_IMPL_START_Y(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_start_y(&screen->NC_SCREEN_IMPL_BASE_FIELD);
}

int32
NC_SCREEN_IMPL_WIDTH(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_width(&screen->NC_SCREEN_IMPL_BASE_FIELD);
}

int32
NC_SCREEN_IMPL_HEIGHT(NC_SCREEN_IMPL_TYPE *screen) {
    return nc_scrollpad_screen_height(&screen->NC_SCREEN_IMPL_BASE_FIELD);
}

static NC_SCREEN_IMPL_TYPE *
NC_SCREEN_IMPL_FROM_SCREEN(NcScreen *screen) {
    return (NC_SCREEN_IMPL_TYPE *)screen;
}

static NcWindow *
NC_SCREEN_IMPL_ACTIVE_WINDOW(NcScreen *screen) {
    return &NC_SCREEN_IMPL_FROM_SCREEN(screen)->NC_SCREEN_IMPL_WINDOW_FIELD;
}

static void
NC_SCREEN_IMPL_REFRESH(NcScreen *screen) {
    NC_SCREEN_IMPL_REFRESH_CALLBACK(NC_SCREEN_IMPL_FROM_SCREEN(screen));
    return;
}

#if defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)
static void
NC_SCREEN_IMPL_SCROLL(NcScreen *screen, enum NcScroll where) {
    NC_SCREEN_IMPL_TYPE *impl;

    impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);
    nc_scrollpad_scroll(&impl->NC_SCREEN_IMPL_SCROLLPAD_FIELD,
                        &impl->NC_SCREEN_IMPL_WINDOW_FIELD,
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

static const NcScreenOps NC_SCREEN_IMPL_OPS = {
    .active_window = NC_SCREEN_IMPL_ACTIVE_WINDOW,
    .refresh = NC_SCREEN_IMPL_REFRESH,
    .refresh_window = NC_SCREEN_IMPL_REFRESH,
#if defined(NC_SCREEN_IMPL_SCROLL_CALLBACK)
    .scroll = NC_SCREEN_IMPL_SCROLL_CALLBACK,
#elif defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)
    .scroll = NC_SCREEN_IMPL_SCROLL,
#endif
#if defined(NC_SCREEN_IMPL_SWITCH_TO_CALLBACK)
    .switch_to = NC_SCREEN_IMPL_SWITCH_TO_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_RESIZE_CALLBACK)
    .resize = NC_SCREEN_IMPL_RESIZE_CALLBACK,
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
#endif
};

#undef NC_SCREEN_IMPL_HEIGHT
#undef NC_SCREEN_IMPL_WIDTH
#undef NC_SCREEN_IMPL_START_Y
#undef NC_SCREEN_IMPL_START_X
#undef NC_SCREEN_IMPL_BASE
#undef NC_SCREEN_IMPL_TITLE
#undef NC_SCREEN_IMPL_SCROLL
#undef NC_SCREEN_IMPL_REFRESH
#undef NC_SCREEN_IMPL_ACTIVE_WINDOW
#undef NC_SCREEN_IMPL_OPS
#undef NC_SCREEN_IMPL_FROM_SCREEN
#undef NC_SCREEN_IMPL_MERGABLE
#undef NC_SCREEN_IMPL_LOCKABLE
#undef NC_SCREEN_IMPL_DESTROY_CALLBACK
#undef NC_SCREEN_IMPL_MOUSE_CALLBACK
#undef NC_SCREEN_IMPL_UPDATE_CALLBACK
#undef NC_SCREEN_IMPL_TITLE_CALLBACK
#undef NC_SCREEN_IMPL_TITLE_LITERAL
#undef NC_SCREEN_IMPL_RESIZE_CALLBACK
#undef NC_SCREEN_IMPL_SWITCH_TO_CALLBACK
#undef NC_SCREEN_IMPL_REFRESH_CALLBACK
#undef NC_SCREEN_IMPL_SCROLLPAD_FIELD
#undef NC_SCREEN_IMPL_SCROLL_CALLBACK
#undef NC_SCREEN_IMPL_WINDOW_FIELD
#undef NC_SCREEN_IMPL_BASE_FIELD
#undef NC_SCREEN_IMPL_PUBLIC_PREFIX
#undef NC_SCREEN_IMPL_PREFIX
#undef NC_SCREEN_IMPL_TYPE
