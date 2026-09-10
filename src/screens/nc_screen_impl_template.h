#include "cbase.h"
#include "ncmpcpp2.h"

#include "nc_screens.h"

#if defined(__INCLUDE_LEVEL__) && (__INCLUDE_LEVEL__ == 0)
typedef struct NcScreenImplDummy {
    NcScreen nc_screen_impl_base_field;
    NcWindow nc_screen_impl_window_field;
    NcScrollpad nc_screen_impl_scrollpad_field;
} NcScreenImplDummy;

NcScreen *nc_screen_impl_dummy_base(NcScreenImplDummy *);

#define NC_SCREEN_IMPL_TYPE              NcScreenImplDummy
#define NC_SCREEN_IMPL_PREFIX            nc_screen_impl_dummy
#define NC_SCREEN_IMPL_PUBLIC_PREFIX     nc_screen_impl_dummy
#define NC_SCREEN_IMPL_BASE_FIELD        nc_screen_impl_base_field
#define NC_SCREEN_IMPL_BASE_EXPR(screen) (&(screen)->NC_SCREEN_IMPL_BASE_FIELD)
#define NC_SCREEN_IMPL_WINDOW_FIELD      nc_screen_impl_window_field
#define NC_SCREEN_IMPL_SCROLLPAD_FIELD   nc_screen_impl_scrollpad_field
#define NC_SCREEN_IMPL_NO_GEOMETRY_ACCESSORS
#define NC_SCREEN_IMPL_REFRESH_CALLBACK(screen) ((void)(screen))
#endif

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
#if !defined(NC_SCREEN_IMPL_WINDOW_FIELD)                                      \
    && !defined(NC_SCREEN_IMPL_WINDOW)
#error "screen implementation needs a window field or window expression"
#endif
#if !defined(NC_SCREEN_IMPL_SCROLL_CALLBACK)                                   \
    && !defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)                                \
    && !defined(NC_SCREEN_IMPL_SCROLL_MENU)                                    \
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
#if !defined(NC_SCREEN_IMPL_CAPABILITIES)
#define NC_SCREEN_IMPL_CAPABILITIES 0
#endif
#if !defined(NC_SCREEN_IMPL_WINDOW)
#define NC_SCREEN_IMPL_WINDOW(screen)                                          \
    (&(screen)->NC_SCREEN_IMPL_WINDOW_FIELD)
#endif
#if !defined(NC_SCREEN_IMPL_SCROLL_MENU)                                       \
    && defined(NC_SCREEN_IMPL_MENU)
#define NC_SCREEN_IMPL_SCROLL_MENU(screen) NC_SCREEN_IMPL_MENU(screen)
#endif
#if !defined(NC_SCREEN_IMPL_SCROLL_HEIGHT)                                     \
    && defined(NC_SCREEN_IMPL_SCROLL_MENU)
#define NC_SCREEN_IMPL_SCROLL_HEIGHT(screen)                                   \
    nc_window_height(NC_SCREEN_IMPL_WINDOW(screen))
#endif
#if !defined(NC_SCREEN_IMPL_TAG_MENU) && defined(NC_SCREEN_IMPL_MENU)
#define NC_SCREEN_IMPL_TAG_MENU(screen) NC_SCREEN_IMPL_MENU(screen)
#endif
#if (defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)                                   \
     || defined(NC_SCREEN_IMPL_SCROLLPAD_BASE))                                \
    && !defined(NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR)
#if defined(NC_SCREEN_IMPL_SCROLLPAD_BASE)
#define NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen)                             \
    (&(screen)->NC_SCREEN_IMPL_SCROLLPAD_BASE)
#else
#define NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen)                             \
    (&(screen)->NC_SCREEN_IMPL_BASE_FIELD)
#endif
#endif
#if !defined(NC_SCREEN_IMPL_BASE_EXPR)
#if defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)                                    \
    || defined(NC_SCREEN_IMPL_SCROLLPAD_BASE)
#define NC_SCREEN_IMPL_BASE_EXPR(screen)                                       \
    nc_scrollpad_screen_base(NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR(screen))
#else
#define NC_SCREEN_IMPL_BASE_EXPR(screen)                                       \
    (&(screen)->NC_SCREEN_IMPL_BASE_FIELD)
#endif
#endif

#define NC_SCREEN_IMPL_FROM_SCREEN CAT(NC_SCREEN_IMPL_PREFIX, _from_screen)
#define NC_SCREEN_IMPL_OPS CAT(NC_SCREEN_IMPL_PREFIX, _ops)
#define NC_SCREEN_IMPL_ACTIVE_WINDOW                                           \
    CAT(NC_SCREEN_IMPL_PREFIX, _active_window)
#define NC_SCREEN_IMPL_CURRENT_MENU                                            \
    CAT(NC_SCREEN_IMPL_PREFIX, _current_menu)
#define NC_SCREEN_IMPL_CURRENT_MENU_HEIGHT                                     \
    CAT(NC_SCREEN_IMPL_PREFIX, _current_menu_height)
#define NC_SCREEN_IMPL_CURRENT_FILTER                                          \
    CAT(NC_SCREEN_IMPL_PREFIX, _current_filter)
#define NC_SCREEN_IMPL_APPLY_FILTER                                            \
    CAT(NC_SCREEN_IMPL_PREFIX, _apply_filter)
#define NC_SCREEN_IMPL_CAN_SEARCH                                              \
    CAT(NC_SCREEN_IMPL_PREFIX, _can_search)
#define NC_SCREEN_IMPL_CURRENT_SEARCH_CONSTRAINT                               \
    CAT(NC_SCREEN_IMPL_PREFIX, _current_search_constraint)
#define NC_SCREEN_IMPL_CLEAR_SEARCH_CONSTRAINT                                 \
    CAT(NC_SCREEN_IMPL_PREFIX, _clear_search_constraint)
#define NC_SCREEN_IMPL_SEARCH CAT(NC_SCREEN_IMPL_PREFIX, _search)
#define NC_SCREEN_IMPL_CURRENT_SONG                                            \
    CAT(NC_SCREEN_IMPL_PREFIX, _current_song)
#define NC_SCREEN_IMPL_SELECTED_SONGS                                          \
    CAT(NC_SCREEN_IMPL_PREFIX, _selected_songs)
#define NC_SCREEN_IMPL_TAG_MENU_CALLBACK                                       \
    CAT(NC_SCREEN_IMPL_PREFIX, _tag_menu)
#define NC_SCREEN_IMPL_SONG_TAG_AT                                             \
    CAT(NC_SCREEN_IMPL_PREFIX, _song_tag_at)
#define NC_SCREEN_IMPL_REFRESH CAT(NC_SCREEN_IMPL_PREFIX, _refresh)
#define NC_SCREEN_IMPL_REFRESH_WINDOW                                          \
    CAT(NC_SCREEN_IMPL_PREFIX, _refresh_window)
#define NC_SCREEN_IMPL_SCROLL CAT(NC_SCREEN_IMPL_PREFIX, _scroll)
#define NC_SCREEN_IMPL_TITLE CAT(NC_SCREEN_IMPL_PREFIX, _title)
#define NC_SCREEN_IMPL_DESTROY CAT(NC_SCREEN_IMPL_PREFIX, _destroy)
#define NC_SCREEN_IMPL_BASE CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _base)
#define NC_SCREEN_IMPL_START_X CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _start_x)
#define NC_SCREEN_IMPL_START_Y CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _start_y)
#define NC_SCREEN_IMPL_WIDTH CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _width)
#define NC_SCREEN_IMPL_HEIGHT CAT(NC_SCREEN_IMPL_PUBLIC_PREFIX, _height)

_Static_assert(offsetof(NC_SCREEN_IMPL_TYPE, NC_SCREEN_IMPL_FIRST_FIELD) == 0,
               "screen first field must start at offset zero");

NcScreen *
NC_SCREEN_IMPL_BASE(NC_SCREEN_IMPL_TYPE *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return NC_SCREEN_IMPL_BASE_EXPR(screen);
}

#if (defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)                                   \
     || defined(NC_SCREEN_IMPL_SCROLLPAD_BASE))                                \
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
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    return NC_SCREEN_IMPL_WINDOW(impl);
}

#if defined(NC_SCREEN_IMPL_MENU_CAPABILITY)
static NcMenu *
NC_SCREEN_IMPL_CURRENT_MENU(NcScreen *screen) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    return NC_SCREEN_IMPL_MENU(impl);
}
#endif

#if defined(NC_SCREEN_IMPL_MENU_CAPABILITY_HEIGHT)
static int32
NC_SCREEN_IMPL_CURRENT_MENU_HEIGHT(NcScreen *screen) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    return NC_SCREEN_IMPL_MENU_CAPABILITY_HEIGHT(impl);
}
#endif
#if defined(NC_SCREEN_IMPL_FILTER_CONSTRAINT_FIELD)
static StringView
NC_SCREEN_IMPL_CURRENT_FILTER(NcScreen *screen) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);
    StrBuilder *constraint = &impl->NC_SCREEN_IMPL_FILTER_CONSTRAINT_FIELD;

    return ncm_string_view(constraint->data, constraint->len);
}
#endif

#if defined(NC_SCREEN_IMPL_FILTER_APPLY_CALLBACK)
static int32
NC_SCREEN_IMPL_APPLY_FILTER(NcScreen *screen, char *pattern,
                            int32 pattern_len, uint32 regex_flags,
                            NcmError *ncm_error) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    (void)regex_flags;
    return NC_SCREEN_IMPL_FILTER_APPLY_CALLBACK(impl, pattern, pattern_len,
                                                ncm_error);
}
#endif

#if defined(NC_SCREEN_IMPL_SEARCH_CAN_CALLBACK)
static bool
NC_SCREEN_IMPL_CAN_SEARCH(NcScreen *screen) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    return NC_SCREEN_IMPL_SEARCH_CAN_CALLBACK(impl);
}
#endif

#if defined(NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD)
static StringView
NC_SCREEN_IMPL_CURRENT_SEARCH_CONSTRAINT(NcScreen *screen) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);
    StrBuilder *constraint = &impl->NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD;

    return ncm_string_view(constraint->data, constraint->len);
}
#endif

#if defined(NC_SCREEN_IMPL_SEARCH_CLEAR_CALLBACK)
static void
NC_SCREEN_IMPL_CLEAR_SEARCH_CONSTRAINT(NcScreen *screen) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    NC_SCREEN_IMPL_SEARCH_CLEAR_CALLBACK(impl);
    return;
}
#elif defined(NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD)
static void
NC_SCREEN_IMPL_CLEAR_SEARCH_CONSTRAINT(NcScreen *screen) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    sb_clear(&impl->NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD);
    return;
}
#endif

#if defined(NC_SCREEN_IMPL_SEARCH_CALLBACK)
static int32
NC_SCREEN_IMPL_SEARCH(NcScreen *screen, enum SearchDirection direction,
                      char *pattern, int32 pattern_len, uint32 regex_flags,
                      bool wrap, bool skip_current, NcmError *ncm_error) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);
    int32 status;
    bool forward;

    (void)regex_flags;
    forward = direction == NCM_SEARCH_DIRECTION_FORWARD;
    status = NC_SCREEN_IMPL_SEARCH_CALLBACK(impl, pattern, pattern_len,
                                            forward, wrap, skip_current,
                                            ncm_error);
#if defined(NC_SCREEN_IMPL_SEARCH_SAVE_ON_SUCCESS)
    if (status >= 0) {
        sb_set(&impl->NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD, pattern,
               pattern_len);
    }
#endif
    return status;
}
#elif defined(NC_SCREEN_IMPL_FIND_CALLBACK)
static int32
NC_SCREEN_IMPL_SEARCH(NcScreen *screen, enum SearchDirection direction,
                      char *pattern, int32 pattern_len, uint32 regex_flags,
                      bool wrap, bool skip_current, NcmError *ncm_error) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    (void)direction;
    (void)regex_flags;
    (void)wrap;
    (void)skip_current;
    return NC_SCREEN_IMPL_FIND_CALLBACK(impl, pattern, pattern_len, ncm_error);
}
#endif

#if defined(NC_SCREEN_IMPL_CURRENT_SONG_CALLBACK)
static int32
NC_SCREEN_IMPL_CURRENT_SONG(NcScreen *screen, NcmSong *song) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

#if defined(NC_SCREEN_IMPL_CURRENT_SONG_OPTIONAL_STATUS)
    return nc_screen_optional_song_status(
        NC_SCREEN_IMPL_CURRENT_SONG_CALLBACK(impl, song));
#else
    return NC_SCREEN_IMPL_CURRENT_SONG_CALLBACK(impl, song);
#endif
}
#endif

#if defined(NC_SCREEN_IMPL_SELECTED_SONGS_CALLBACK)
static int32
NC_SCREEN_IMPL_SELECTED_SONGS(NcScreen *screen, NcmSongArray *songs) {
    return NC_SCREEN_IMPL_SELECTED_SONGS_CALLBACK(
        NC_SCREEN_IMPL_FROM_SCREEN(screen), songs);
}
#endif

#if defined(NC_SCREEN_IMPL_TAG_ITEM_SONG_CALLBACK)
static NcMenu *
NC_SCREEN_IMPL_TAG_MENU_CALLBACK(NcScreen *screen) {
    return NC_SCREEN_IMPL_TAG_MENU(NC_SCREEN_IMPL_FROM_SCREEN(screen));
}

static int32
NC_SCREEN_IMPL_SONG_TAG_AT(NcScreen *screen, int32 pos,
                           enum SongGetter getter, StrBuilder *tag) {
    return nc_screen_menu_song_tag_at(
        NC_SCREEN_IMPL_TAG_MENU(NC_SCREEN_IMPL_FROM_SCREEN(screen)),
        pos, getter, tag, NC_SCREEN_IMPL_TAG_ITEM_SONG_CALLBACK);
}
#endif


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
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    nc_scrollpad_scroll(&impl->NC_SCREEN_IMPL_SCROLLPAD_FIELD,
                        NC_SCREEN_IMPL_WINDOW(impl), where);
    return;
}
#elif defined(NC_SCREEN_IMPL_SCROLL_MENU)
static void
NC_SCREEN_IMPL_SCROLL(NcScreen *screen, enum NcScroll where) {
    NC_SCREEN_IMPL_TYPE *impl = NC_SCREEN_IMPL_FROM_SCREEN(screen);

    nc_menu_scroll_selectable(NC_SCREEN_IMPL_SCROLL_MENU(impl),
                              NC_SCREEN_IMPL_SCROLL_HEIGHT(impl), where);
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
    .capabilities = NC_SCREEN_IMPL_CAPABILITIES
#if defined(NC_SCREEN_IMPL_MENU_CAPABILITY)
                    |NC_SCREEN_CAPABILITY_MENU
#endif
#if defined(NC_SCREEN_IMPL_FILTER_APPLY_CALLBACK)
                    |NC_SCREEN_CAPABILITY_FILTER
#endif
#if defined(NC_SCREEN_IMPL_SEARCH_CALLBACK)
                    |NC_SCREEN_CAPABILITY_SEARCH
#endif
#if defined(NC_SCREEN_IMPL_FIND_CALLBACK)
                    |NC_SCREEN_CAPABILITY_SEARCH
                    |NC_SCREEN_CAPABILITY_FIND
#endif
#if defined(NC_SCREEN_IMPL_CURRENT_SONG_CALLBACK) \
    || defined(NC_SCREEN_IMPL_SELECTED_SONGS_CALLBACK)
                    |NC_SCREEN_CAPABILITY_SONGS
#endif
#if defined(NC_SCREEN_IMPL_TAG_ITEM_SONG_CALLBACK)
                    |NC_SCREEN_CAPABILITY_TAGS
#endif
                    ,
    .active_window = NC_SCREEN_IMPL_ACTIVE_WINDOW,
    .refresh = NC_SCREEN_IMPL_REFRESH,
    .refresh_window = NC_SCREEN_IMPL_REFRESH_WINDOW,
#if defined(NC_SCREEN_IMPL_SCROLL_CALLBACK)
    .scroll = NC_SCREEN_IMPL_SCROLL_CALLBACK,
#elif defined(NC_SCREEN_IMPL_SCROLLPAD_FIELD)                                  \
      || defined(NC_SCREEN_IMPL_SCROLL_MENU)
    .scroll = NC_SCREEN_IMPL_SCROLL,
#endif
#if defined(NC_SCREEN_IMPL_LIST_CHANGE_FINISHED_CALLBACK)
    .list_change_finished = NC_SCREEN_IMPL_LIST_CHANGE_FINISHED_CALLBACK,
#endif
#if defined(NC_SCREEN_IMPL_ACTION_CHANGE_FINISHED_CALLBACK)
    .action_change_finished = NC_SCREEN_IMPL_ACTION_CHANGE_FINISHED_CALLBACK,
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
#if defined(NC_SCREEN_IMPL_MENU_CAPABILITY)
    .current_menu = NC_SCREEN_IMPL_CURRENT_MENU,
#endif
#if defined(NC_SCREEN_IMPL_MENU_CAPABILITY_HEIGHT)
    .current_menu_height = NC_SCREEN_IMPL_CURRENT_MENU_HEIGHT,
#endif
#if defined(NC_SCREEN_IMPL_FILTER_CONSTRAINT_FIELD)
    .current_filter = NC_SCREEN_IMPL_CURRENT_FILTER,
#endif
#if defined(NC_SCREEN_IMPL_FILTER_APPLY_CALLBACK)
    .apply_filter = NC_SCREEN_IMPL_APPLY_FILTER,
#endif
#if defined(NC_SCREEN_IMPL_SEARCH_CAN_CALLBACK)
    .can_search = NC_SCREEN_IMPL_CAN_SEARCH,
#endif
#if defined(NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD)
    .current_search_constraint = NC_SCREEN_IMPL_CURRENT_SEARCH_CONSTRAINT,
#endif
#if defined(NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD) \
    || defined(NC_SCREEN_IMPL_SEARCH_CLEAR_CALLBACK)
    .clear_search_constraint = NC_SCREEN_IMPL_CLEAR_SEARCH_CONSTRAINT,
#endif
#if defined(NC_SCREEN_IMPL_SEARCH_CALLBACK) \
    || defined(NC_SCREEN_IMPL_FIND_CALLBACK)
    .search = NC_SCREEN_IMPL_SEARCH,
#endif
#if defined(NC_SCREEN_IMPL_CURRENT_SONG_CALLBACK)
    .current_song = NC_SCREEN_IMPL_CURRENT_SONG,
#endif
#if defined(NC_SCREEN_IMPL_SELECTED_SONGS_CALLBACK)
    .selected_songs = NC_SCREEN_IMPL_SELECTED_SONGS,
#endif
#if defined(NC_SCREEN_IMPL_TAG_ITEM_SONG_CALLBACK)
    .tag_menu = NC_SCREEN_IMPL_TAG_MENU_CALLBACK,
    .song_tag_at = NC_SCREEN_IMPL_SONG_TAG_AT,
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
#undef NC_SCREEN_IMPL_SONG_TAG_AT
#undef NC_SCREEN_IMPL_TAG_MENU_CALLBACK
#undef NC_SCREEN_IMPL_SELECTED_SONGS
#undef NC_SCREEN_IMPL_CURRENT_SONG
#undef NC_SCREEN_IMPL_SEARCH
#undef NC_SCREEN_IMPL_CLEAR_SEARCH_CONSTRAINT
#undef NC_SCREEN_IMPL_CURRENT_SEARCH_CONSTRAINT
#undef NC_SCREEN_IMPL_CAN_SEARCH
#undef NC_SCREEN_IMPL_APPLY_FILTER
#undef NC_SCREEN_IMPL_CURRENT_FILTER
#undef NC_SCREEN_IMPL_CURRENT_MENU_HEIGHT
#undef NC_SCREEN_IMPL_CURRENT_MENU
#undef NC_SCREEN_IMPL_ACTIVE_WINDOW
#undef NC_SCREEN_IMPL_OPS
#undef NC_SCREEN_IMPL_FROM_SCREEN
#undef NC_SCREEN_IMPL_BASE_EXPR
#undef NC_SCREEN_IMPL_SCROLLPAD_BASE_EXPR
#undef NC_SCREEN_IMPL_NO_GEOMETRY_ACCESSORS
#undef NC_SCREEN_IMPL_MENU_CAPABILITY_HEIGHT
#undef NC_SCREEN_IMPL_FIND_CALLBACK
#undef NC_SCREEN_IMPL_TAG_ITEM_SONG_CALLBACK
#undef NC_SCREEN_IMPL_CURRENT_SONG_OPTIONAL_STATUS
#undef NC_SCREEN_IMPL_SELECTED_SONGS_CALLBACK
#undef NC_SCREEN_IMPL_CURRENT_SONG_CALLBACK
#undef NC_SCREEN_IMPL_SEARCH_SAVE_ON_SUCCESS
#undef NC_SCREEN_IMPL_SEARCH_CLEAR_CALLBACK
#undef NC_SCREEN_IMPL_SEARCH_CALLBACK
#undef NC_SCREEN_IMPL_SEARCH_CAN_CALLBACK
#undef NC_SCREEN_IMPL_SEARCH_CONSTRAINT_FIELD
#undef NC_SCREEN_IMPL_FILTER_APPLY_CALLBACK
#undef NC_SCREEN_IMPL_FILTER_CONSTRAINT_FIELD
#undef NC_SCREEN_IMPL_TAG_MENU
#undef NC_SCREEN_IMPL_MENU_CAPABILITY
#undef NC_SCREEN_IMPL_CAPABILITIES
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
#undef NC_SCREEN_IMPL_ACTION_CHANGE_FINISHED_CALLBACK
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
