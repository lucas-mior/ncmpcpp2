#include "screens/nc_media_library.h"

#include "c/ncm_base.h"
#include "c/ncm_string.h"

static NativeMediaLibraryScreen *native_library_from_screen(NcScreen *screen);
static NcWindow *native_library_active_window(NcScreen *screen);
static void native_library_refresh(NcScreen *screen);
static void native_library_refresh_window(NcScreen *screen);
static void native_library_scroll(NcScreen *screen, enum NcScroll where);
static void native_library_switch_to(NcScreen *screen);
static void native_library_resize(NcScreen *screen);
static char *native_library_title(NcScreen *screen);
static void native_library_update(NcScreen *screen);
static bool native_library_is_lockable(NcScreen *screen);
static bool native_library_is_mergable(NcScreen *screen);
static void native_library_destroy_callback(NcScreen *screen);
static bool native_library_tag_filter(NcMenu *menu, void *item, void *user);
static bool native_library_album_filter(NcMenu *menu, void *item,
                                        void *user);
static bool native_library_song_filter(NcMenu *menu, void *item,
                                       void *user);
static bool native_library_copy_song_at(NativeMediaLibraryScreen *screen,
                                        NcmSongArray *songs, int64 pos);
static bool native_library_has_tag(NativeMediaLibraryScreen *screen,
                                   NcmStringView *view);
static bool native_library_tag_matches(NcMediaLibraryTagRow *row,
                                       NcmRegex *regex);
static bool native_library_album_matches(NcMediaLibraryAlbumRow *row,
                                         NcmRegex *regex);
static bool native_library_song_matches(NcmSong *song, NcmRegex *regex);
static void native_library_layout(NativeMediaLibraryScreen *screen);
static bool native_library_set_owned_string(char **dest, int32 *dest_len,
                                            int32 *dest_cap, char *source,
                                            int32 source_len);
static void native_library_free_owned_string(char **data, int32 *len,
                                             int32 *cap);

static NcScreenCallbacks native_library_callbacks = {
    .active_window = native_library_active_window,
    .refresh = native_library_refresh,
    .refresh_window = native_library_refresh_window,
    .scroll = native_library_scroll,
    .switch_to = native_library_switch_to,
    .resize = native_library_resize,
    .title = native_library_title,
    .update = native_library_update,
    .is_lockable = native_library_is_lockable,
    .is_mergable = native_library_is_mergable,
    .destroy = native_library_destroy_callback,
};

void
native_media_library_screen_init(NativeMediaLibraryScreen *screen,
                                 int64 start_x, int64 width,
                                 int64 main_start_y, int64 main_height,
                                 NcColor color, NcBorder border) {
    nc_media_library_tag_menu_init(&screen->tags);
    nc_media_library_album_menu_init(&screen->albums);
    nc_media_library_song_menu_init(&screen->songs);
    nc_window_init(&screen->tags_window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    nc_window_init(&screen->albums_window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    nc_window_init(&screen->songs_window, start_x, main_start_y, width,
                   main_height, NULL, 0, color, border);
    ncm_buffer_init(&screen->search_constraint);
    ncm_regex_init(&screen->tag_filter_regex);
    ncm_regex_init(&screen->album_filter_regex);
    ncm_regex_init(&screen->song_filter_regex);

    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    screen->columns = 3;
    screen->active_column = NATIVE_MEDIA_LIBRARY_COLUMN_TAGS;
    screen->tags_update_request = false;
    screen->albums_update_request = false;
    screen->songs_update_request = false;
    screen->filter_enabled = false;
    screen->registered = false;

    nc_screen_init(&screen->screen, native_library_callbacks, screen,
                   NC_SCREEN_TYPE_MEDIA_LIBRARY);
    native_library_layout(screen);
    return;
}

void
native_media_library_screen_destroy(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->song_filter_regex);
    ncm_regex_destroy(&screen->album_filter_regex);
    ncm_regex_destroy(&screen->tag_filter_regex);
    ncm_buffer_destroy(&screen->search_constraint);
    nc_window_destroy(&screen->songs_window);
    nc_window_destroy(&screen->albums_window);
    nc_window_destroy(&screen->tags_window);
    nc_media_library_song_menu_destroy(&screen->songs);
    nc_media_library_album_menu_destroy(&screen->albums);
    nc_media_library_tag_menu_destroy(&screen->tags);
    return;
}

NcScreen *
native_media_library_screen_base(NativeMediaLibraryScreen *screen) {
    return &screen->screen;
}

NcMediaLibraryTagMenu *
native_media_library_screen_tags(NativeMediaLibraryScreen *screen) {
    return &screen->tags;
}

NcMediaLibraryAlbumMenu *
native_media_library_screen_albums(NativeMediaLibraryScreen *screen) {
    return &screen->albums;
}

NcMediaLibrarySongMenu *
native_media_library_screen_songs(NativeMediaLibraryScreen *screen) {
    return &screen->songs;
}

NcMenu *
native_media_library_screen_active_menu(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return nc_media_library_album_menu_base(&screen->albums);
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        return nc_media_library_song_menu_base(&screen->songs);
    }
    return nc_media_library_tag_menu_base(&screen->tags);
}

NcWindow *
native_media_library_screen_active_window(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return &screen->albums_window;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        return &screen->songs_window;
    }
    return &screen->tags_window;
}

void
native_media_library_screen_set_geometry(NativeMediaLibraryScreen *screen,
                                         int64 start_x, int64 width,
                                         int64 main_start_y,
                                         int64 main_height) {
    if (screen == NULL) {
        return;
    }
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    native_library_layout(screen);
    return;
}

int64
native_media_library_screen_columns(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return 0;
    }
    return screen->columns;
}

void
native_media_library_screen_set_columns(NativeMediaLibraryScreen *screen,
                                        int64 columns) {
    if (screen == NULL) {
        return;
    }
    if (columns < 1) {
        columns = 1;
    }
    if (columns > 3) {
        columns = 3;
    }
    screen->columns = columns;
    if (screen->active_column >= columns) {
        screen->active_column = columns - 1;
    }
    native_library_layout(screen);
    return;
}

bool
native_media_library_screen_previous_column_available(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->active_column > 0;
}

bool
native_media_library_screen_next_column_available(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->active_column + 1 < screen->columns;
}

void
native_media_library_screen_previous_column(NativeMediaLibraryScreen *screen) {
    if (native_media_library_screen_previous_column_available(screen)) {
        screen->active_column -= 1;
    }
    return;
}

void
native_media_library_screen_next_column(NativeMediaLibraryScreen *screen) {
    if (native_media_library_screen_next_column_available(screen)) {
        screen->active_column += 1;
    }
    return;
}

void
native_media_library_screen_clear(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_media_library_tag_menu_base(&screen->tags));
    nc_menu_clear_items(nc_media_library_album_menu_base(&screen->albums));
    nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));
    return;
}

bool
native_media_library_screen_add_tag(NativeMediaLibraryScreen *screen,
                                    char *tag, int32 tag_len,
                                    time_t mtime) {
    NcMediaLibraryTagRow row;

    if (screen == NULL || tag == NULL || tag_len <= 0) {
        return false;
    }
    nc_media_library_tag_row_init(&row);
    if (!native_library_set_owned_string(&row.tag, &row.tag_len,
                                         &row.tag_cap, tag, tag_len)) {
        nc_media_library_tag_row_destroy(&row);
        return false;
    }
    row.mtime = mtime;
    nc_media_library_tag_menu_add(&screen->tags, &row);
    nc_media_library_tag_row_destroy(&row);
    return true;
}

bool
native_media_library_screen_add_album(NativeMediaLibraryScreen *screen,
                                      char *tag, int32 tag_len,
                                      char *album, int32 album_len,
                                      char *date, int32 date_len,
                                      time_t mtime,
                                      bool all_tracks_entry) {
    NcMediaLibraryAlbumRow row;
    bool result;

    if (screen == NULL) {
        return false;
    }
    nc_media_library_album_row_init(&row);
    result = native_library_set_owned_string(&row.tag, &row.tag_len,
                                             &row.tag_cap, tag, tag_len);
    if (result) {
        result = native_library_set_owned_string(&row.album,
                                                 &row.album_len,
                                                 &row.album_cap,
                                                 album, album_len);
    }
    if (result) {
        result = native_library_set_owned_string(&row.date, &row.date_len,
                                                 &row.date_cap, date,
                                                 date_len);
    }
    if (result) {
        row.mtime = mtime;
        row.all_tracks_entry = all_tracks_entry;
        nc_media_library_album_menu_add(&screen->albums, &row);
    }
    nc_media_library_album_row_destroy(&row);
    return result;
}

bool
native_media_library_screen_add_song_copy(NativeMediaLibraryScreen *screen,
                                          NcmSong *song) {
    if (screen == NULL || song == NULL) {
        return false;
    }
    nc_media_library_song_menu_add(&screen->songs, song);
    return true;
}

bool
native_media_library_screen_group_tags_from_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    enum mpd_tag_type tag_type) {
    NcmStringView view;

    if (screen == NULL || songs == NULL) {
        return false;
    }
    nc_menu_clear_items(nc_media_library_tag_menu_base(&screen->tags));
    for (int32 i = 0; i < songs->len; i += 1) {
        if (!ncm_song_tag_view(&songs->items[i], tag_type, 0, &view)) {
            continue;
        }
        if (view.len <= 0 || native_library_has_tag(screen, &view)) {
            continue;
        }
        if (!native_media_library_screen_add_tag(screen, view.data,
                                                 view.len,
                                                 songs->items[i]
                                                     .last_modified)) {
            return false;
        }
    }
    return true;
}

bool
native_media_library_screen_current_song(NativeMediaLibraryScreen *screen,
                                         NcmSong *song) {
    NcmSong *current;

    if (screen == NULL || song == NULL) {
        return false;
    }
    current = nc_media_library_song_menu_current(&screen->songs);
    if (current == NULL) {
        return false;
    }
    return ncm_song_copy(song, current);
}

bool
native_media_library_screen_selected_songs(NativeMediaLibraryScreen *screen,
                                           NcmSongArray *songs) {
    NcMenu *menu;
    bool any_selected;

    if (screen == NULL || songs == NULL) {
        return false;
    }
    menu = nc_media_library_song_menu_base(&screen->songs);
    any_selected = nc_menu_has_selected(menu);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!native_library_copy_song_at(screen, songs, i)) {
            return false;
        }
    }
    return true;
}

bool
native_media_library_screen_apply_filter(NativeMediaLibraryScreen *screen,
                                         char *pattern, int32 pattern_len,
                                         NcmError *error) {
    NcMenuDisplayCallbacks tag_callbacks = {0};
    NcMenuDisplayCallbacks album_callbacks = {0};
    NcMenuDisplayCallbacks song_callbacks = {0};
    bool result;

    if (screen == NULL) {
        return false;
    }
    if (pattern_len <= 0) {
        native_media_library_screen_clear_filter(screen);
        return true;
    }

    result = ncm_regex_compile(&screen->tag_filter_regex, pattern,
                               pattern_len,
                               NCM_REGEX_LITERAL_CASE_INSENSITIVE, error);
    if (result) {
        result = ncm_regex_compile(&screen->album_filter_regex, pattern,
                                   pattern_len,
                                   NCM_REGEX_LITERAL_CASE_INSENSITIVE,
                                   error);
    }
    if (result) {
        result = ncm_regex_compile(&screen->song_filter_regex, pattern,
                                   pattern_len,
                                   NCM_REGEX_LITERAL_CASE_INSENSITIVE,
                                   error);
    }
    if (!result) {
        return false;
    }
    if (!ncm_buffer_set(&screen->search_constraint, pattern, pattern_len)) {
        return false;
    }

    tag_callbacks.filter = native_library_tag_filter;
    tag_callbacks.user = screen;
    album_callbacks.filter = native_library_album_filter;
    album_callbacks.user = screen;
    song_callbacks.filter = native_library_song_filter;
    song_callbacks.user = screen;
    nc_menu_set_display_callbacks(nc_media_library_tag_menu_base(
                                      &screen->tags),
                                  tag_callbacks);
    nc_menu_set_display_callbacks(nc_media_library_album_menu_base(
                                      &screen->albums),
                                  album_callbacks);
    nc_menu_set_display_callbacks(nc_media_library_song_menu_base(
                                      &screen->songs),
                                  song_callbacks);
    screen->filter_enabled = true;
    nc_menu_apply_filter(nc_media_library_tag_menu_base(&screen->tags));
    nc_menu_apply_filter(nc_media_library_album_menu_base(&screen->albums));
    nc_menu_apply_filter(nc_media_library_song_menu_base(&screen->songs));
    return true;
}

void
native_media_library_screen_clear_filter(NativeMediaLibraryScreen *screen) {
    NcMenuDisplayCallbacks callbacks = {0};

    if (screen == NULL) {
        return;
    }
    ncm_regex_destroy(&screen->tag_filter_regex);
    ncm_regex_destroy(&screen->album_filter_regex);
    ncm_regex_destroy(&screen->song_filter_regex);
    ncm_regex_init(&screen->tag_filter_regex);
    ncm_regex_init(&screen->album_filter_regex);
    ncm_regex_init(&screen->song_filter_regex);
    ncm_buffer_clear(&screen->search_constraint);
    screen->filter_enabled = false;
    nc_menu_set_display_callbacks(nc_media_library_tag_menu_base(
                                      &screen->tags),
                                  callbacks);
    nc_menu_set_display_callbacks(nc_media_library_album_menu_base(
                                      &screen->albums),
                                  callbacks);
    nc_menu_set_display_callbacks(nc_media_library_song_menu_base(
                                      &screen->songs),
                                  callbacks);
    nc_menu_show_all_items(nc_media_library_tag_menu_base(&screen->tags));
    nc_menu_show_all_items(nc_media_library_album_menu_base(&screen->albums));
    nc_menu_show_all_items(nc_media_library_song_menu_base(&screen->songs));
    return;
}

void
native_media_library_screen_request_tags_update(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->tags_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
native_media_library_screen_request_albums_update(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->albums_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
native_media_library_screen_request_songs_update(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->songs_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
native_media_library_screen_clear_update_requests(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->tags_update_request = false;
    screen->albums_update_request = false;
    screen->songs_update_request = false;
    nc_screen_clear_update_request(&screen->screen);
    return;
}

bool
native_media_library_screen_locate_song(NativeMediaLibraryScreen *screen,
                                        NcmSong *song) {
    NcMenu *menu;
    NcmSong *candidate;

    if (screen == NULL || song == NULL) {
        return false;
    }
    menu = nc_media_library_song_menu_base(&screen->songs);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        candidate = nc_menu_active_item_at(menu, i);
        if (candidate != NULL && ncm_song_equal(candidate, song)) {
            return nc_menu_goto_selectable(menu, i);
        }
    }
    return false;
}

static NativeMediaLibraryScreen *
native_library_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
native_library_active_window(NcScreen *screen) {
    return native_media_library_screen_active_window(
        native_library_from_screen(screen));
}

static void
native_library_refresh(NcScreen *screen) {
    NativeMediaLibraryScreen *library;

    library = native_library_from_screen(screen);
    nc_menu_prepare_refresh(native_media_library_screen_active_menu(library),
                            library->main_height, NULL, NULL);
    return;
}

static void
native_library_refresh_window(NcScreen *screen) {
    native_library_refresh(screen);
    return;
}

static void
native_library_scroll(NcScreen *screen, enum NcScroll where) {
    NativeMediaLibraryScreen *library;

    library = native_library_from_screen(screen);
    nc_menu_scroll_selectable(native_media_library_screen_active_menu(library),
                              library->main_height, where);
    return;
}

static void
native_library_switch_to(NcScreen *screen) {
    (void)screen;
    return;
}

static void
native_library_resize(NcScreen *screen) {
    NativeMediaLibraryScreen *library;

    library = native_library_from_screen(screen);
    native_media_library_screen_set_geometry(library, library->start_x,
                                             library->width,
                                             library->main_start_y,
                                             library->main_height);
    nc_screen_clear_resize_request(screen);
    return;
}

static char *
native_library_title(NcScreen *screen) {
    (void)screen;
    return (char *)"Media library";
}

static void
native_library_update(NcScreen *screen) {
    native_media_library_screen_clear_update_requests(
        native_library_from_screen(screen));
    return;
}

static bool
native_library_is_lockable(NcScreen *screen) {
    (void)screen;
    return true;
}

static bool
native_library_is_mergable(NcScreen *screen) {
    (void)screen;
    return true;
}

static void
native_library_destroy_callback(NcScreen *screen) {
    native_media_library_screen_destroy(native_library_from_screen(screen));
    return;
}

static bool
native_library_tag_filter(NcMenu *menu, void *item, void *user) {
    NativeMediaLibraryScreen *screen;

    (void)menu;
    screen = user;
    return native_library_tag_matches(item, &screen->tag_filter_regex);
}

static bool
native_library_album_filter(NcMenu *menu, void *item, void *user) {
    NativeMediaLibraryScreen *screen;

    (void)menu;
    screen = user;
    return native_library_album_matches(item, &screen->album_filter_regex);
}

static bool
native_library_song_filter(NcMenu *menu, void *item, void *user) {
    NativeMediaLibraryScreen *screen;

    (void)menu;
    screen = user;
    return native_library_song_matches(item, &screen->song_filter_regex);
}

static bool
native_library_copy_song_at(NativeMediaLibraryScreen *screen,
                            NcmSongArray *songs, int64 pos) {
    NcmSong *song;

    song = nc_menu_active_item_at(nc_media_library_song_menu_base(
                                      &screen->songs),
                                  pos);
    if (song == NULL) {
        return true;
    }
    return ncm_song_array_append_copy(songs, song);
}

static bool
native_library_has_tag(NativeMediaLibraryScreen *screen,
                       NcmStringView *view) {
    NcMenu *menu;
    NcMediaLibraryTagRow *row;

    menu = nc_media_library_tag_menu_base(&screen->tags);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        row = nc_menu_active_item_at(menu, i);
        if (row != NULL && ncm_string_equal(row->tag, row->tag_len,
                                            view->data, view->len)) {
            return true;
        }
    }
    return false;
}

static bool
native_library_tag_matches(NcMediaLibraryTagRow *row, NcmRegex *regex) {
    if (row == NULL || row->tag == NULL) {
        return false;
    }
    return ncm_regex_search(regex, row->tag, row->tag_len);
}

static bool
native_library_album_matches(NcMediaLibraryAlbumRow *row,
                             NcmRegex *regex) {
    if (row == NULL) {
        return false;
    }
    if ((row->album != NULL)
        && ncm_regex_search(regex, row->album, row->album_len)) {
        return true;
    }
    if ((row->tag != NULL)
        && ncm_regex_search(regex, row->tag, row->tag_len)) {
        return true;
    }
    if ((row->date != NULL)
        && ncm_regex_search(regex, row->date, row->date_len)) {
        return true;
    }
    return false;
}

static bool
native_library_song_matches(NcmSong *song, NcmRegex *regex) {
    NcmStringView view;

    if (song == NULL) {
        return false;
    }
    if (!ncm_song_uri_view(song, 0, &view)) {
        return false;
    }
    return ncm_regex_search(regex, view.data, view.len);
}

static void
native_library_layout(NativeMediaLibraryScreen *screen) {
    int64 column_width;
    int64 x;

    if (screen->columns <= 0) {
        return;
    }
    column_width = screen->width / screen->columns;
    if (column_width <= 0) {
        column_width = screen->width;
    }

    x = screen->start_x;
    nc_window_move_to(&screen->tags_window, x, screen->main_start_y);
    nc_window_resize(&screen->tags_window, column_width,
                     screen->main_height);

    x += column_width;
    nc_window_move_to(&screen->albums_window, x, screen->main_start_y);
    nc_window_resize(&screen->albums_window, column_width,
                     screen->main_height);

    x += column_width;
    nc_window_move_to(&screen->songs_window, x, screen->main_start_y);
    nc_window_resize(&screen->songs_window,
                     screen->width - 2*column_width,
                     screen->main_height);
    return;
}

static bool
native_library_set_owned_string(char **dest, int32 *dest_len,
                                int32 *dest_cap, char *source,
                                int32 source_len) {
    char *copy;
    int32 cap;

    if (dest == NULL || dest_len == NULL || dest_cap == NULL) {
        return false;
    }
    native_library_free_owned_string(dest, dest_len, dest_cap);
    if (source == NULL || source_len <= 0) {
        return true;
    }
    cap = source_len + 1;
    copy = ncm_malloc(cap);
    ncm_memcpy(copy, source, source_len);
    copy[source_len] = '\0';
    *dest = copy;
    *dest_len = source_len;
    *dest_cap = cap;
    return true;
}

static void
native_library_free_owned_string(char **data, int32 *len, int32 *cap) {
    if (*data != NULL) {
        ncm_free(*data, *cap);
    }
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}
