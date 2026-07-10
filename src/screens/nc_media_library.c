#include "screens/nc_media_library.h"

#include <errno.h>

#include "settings.h"
#include "ui_state.h"
#include "c/ncm_base.h"
#include "c/ncm_charset.h"
#include "c/ncm_display.h"
#include "c/ncm_string.h"
#include "screens/screen_switcher.h"

static NativeMediaLibraryScreen *native_library_from_screen(NcScreen *screen);
static NcWindow *native_library_active_window(NcScreen *screen);
static void native_library_refresh(NcScreen *screen);
static void native_library_refresh_window(NcScreen *screen);
static void native_library_scroll(NcScreen *screen, enum NcScroll where);
static void native_library_switch_to(NcScreen *screen);
static void native_library_resize(NcScreen *screen);
static int32 native_library_window_timeout(NcScreen *screen);
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
static void native_library_draw_tag(NcMenu *menu, NcWindow *window,
                                    void *item, int64 pos, void *user);
static void native_library_draw_album(NcMenu *menu, NcWindow *window,
                                      void *item, int64 pos, void *user);
static void native_library_draw_song(NcMenu *menu, NcWindow *window,
                                     void *item, int64 pos, void *user);
static void native_library_print_buffer(NcWindow *window, NcBuffer *buffer);
static bool native_library_copy_song_at(NativeMediaLibraryScreen *screen,
                                        NcmSongArray *songs, int64 pos);
static bool native_library_has_tag(NativeMediaLibraryScreen *screen,
                                   NcmStringView *view);
static bool native_library_tag_matches(NcMediaLibraryTagRow *row,
                                       NcmRegex *regex);
static bool native_library_album_matches(NcMediaLibraryAlbumRow *row,
                                         NcmRegex *regex);
static bool native_library_song_matches(NcmSong *song, NcmRegex *regex);
static NativeMediaLibraryColumnState *native_library_active_column_state(
    NativeMediaLibraryScreen *screen);
static NcMenuDisplayCallbacks native_library_display_callbacks(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column, bool filter_enabled);
static void native_library_update_menu_highlights(
    NativeMediaLibraryScreen *screen);
static void native_library_update_titles(NativeMediaLibraryScreen *screen,
                                         bool update_windows);
static void native_library_refresh_menu(NcMenu *menu, NcWindow *window);
static bool native_library_active_item_matches(
    NativeMediaLibraryScreen *screen, NcMenu *menu, int64 pos,
    NcmRegex *regex);
static void native_library_layout(NativeMediaLibraryScreen *screen);
static int32 native_library_ratio_value(NcmInt32Array *ratios,
                                        int32 idx, int32 fallback);
static bool native_library_set_owned_string(char **dest, int32 *dest_len,
                                            int32 *dest_cap, char *source,
                                            int32 source_len);
static void native_library_free_owned_string(char **data, int32 *len,
                                             int32 *cap);
static int32 native_library_cstring_len(char *string);
static char *native_library_query_cstring(NcmBuffer *buffer, char *string,
                                          int32 string_len);
static bool native_library_mpd_list_tags(void *user,
                                         enum mpd_tag_type tag_type,
                                         NcmMpdStringList *tags,
                                         NcmError *error);
static bool native_library_mpd_list_all_songs(void *user,
                                              NcmMpdSongList *songs,
                                              NcmError *error);
static bool native_library_mpd_search_songs(
    void *user, NativeMediaLibrarySongQuery *query,
    NcmMpdSongList *songs, NcmError *error);
static bool native_library_mpd_add_songs(void *user,
                                         NcmSongArray *songs,
                                         bool play, NcmError *error);

static NcScreenCallbacks native_library_callbacks = {
    .active_window = native_library_active_window,
    .refresh = native_library_refresh,
    .refresh_window = native_library_refresh_window,
    .scroll = native_library_scroll,
    .switch_to = native_library_switch_to,
    .resize = native_library_resize,
    .window_timeout = native_library_window_timeout,
    .title = native_library_title,
    .update = native_library_update,
    .is_lockable = native_library_is_lockable,
    .is_mergable = native_library_is_mergable,
    .destroy = native_library_destroy_callback,
};

NativeMediaLibraryHooks
native_media_library_mpd_hooks(NcmMpdClient *client) {
    NativeMediaLibraryHooks hooks = {0};

    hooks.list_tags = native_library_mpd_list_tags;
    hooks.list_all_songs = native_library_mpd_list_all_songs;
    hooks.search_songs = native_library_mpd_search_songs;
    hooks.add_songs = native_library_mpd_add_songs;
    hooks.user = client;
    return hooks;
}

void
native_media_library_screen_init(NativeMediaLibraryScreen *screen,
                                 NativeMediaLibraryHooks hooks,
                                 int64 start_x, int64 width,
                                 int64 main_start_y, int64 main_height,
                                 NcColor color, NcBorder border) {
    NcMenuDisplayCallbacks callbacks;

    nc_media_library_tag_menu_init(&screen->tags);
    nc_media_library_album_menu_init(&screen->albums);
    nc_media_library_song_menu_init(&screen->songs);
    screen->hooks = hooks;

    for (uint32 i = 0; i < NATIVE_MEDIA_LIBRARY_COLUMN_LAST; i += 1) {
        ncm_buffer_init(&screen->column_state[i].filter_constraint);
        ncm_buffer_init(&screen->column_state[i].search_constraint);
        ncm_regex_init(&screen->column_state[i].filter_regex);
        ncm_regex_init(&screen->column_state[i].search_regex);
        screen->column_state[i].filter_enabled = false;
        screen->column_state[i].search_enabled = false;
    }
    ncm_buffer_init(&screen->tags_title);
    ncm_buffer_init(&screen->albums_title);
    ncm_buffer_init(&screen->songs_title);

    screen->update_timer.ns = 0;
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    if (Config.data_fetching_delay) {
        screen->fetching_delay_ms = NATIVE_MEDIA_LIBRARY_FETCH_DELAY_MS;
        screen->window_timeout_ms = NATIVE_MEDIA_LIBRARY_FETCH_DELAY_MS;
    } else {
        screen->fetching_delay_ms = -1;
        screen->window_timeout_ms = NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }

    screen->mode = NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS;
    screen->active_column = NATIVE_MEDIA_LIBRARY_COLUMN_TAGS;
    screen->tags_update_request = false;
    screen->albums_update_request = false;
    screen->songs_update_request = false;
    screen->sort_by_mtime = Config.media_library_sort_by_mtime;
    screen->registered = false;

    native_library_update_titles(screen, false);
    nc_window_init(&screen->tags_window, start_x, main_start_y, width,
                   main_height, screen->tags_title.data,
                   screen->tags_title.len, color, border);
    nc_window_init(&screen->albums_window, start_x, main_start_y, width,
                   main_height, screen->albums_title.data,
                   screen->albums_title.len, color, border);
    nc_window_init(&screen->songs_window, start_x, main_start_y, width,
                   main_height, screen->songs_title.data,
                   screen->songs_title.len, color, border);

    callbacks = native_library_display_callbacks(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS, false);
    nc_menu_set_display_callbacks(
        nc_media_library_tag_menu_base(&screen->tags), callbacks);
    callbacks = native_library_display_callbacks(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS, false);
    nc_menu_set_display_callbacks(
        nc_media_library_album_menu_base(&screen->albums), callbacks);
    callbacks = native_library_display_callbacks(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS, false);
    nc_menu_set_display_callbacks(
        nc_media_library_song_menu_base(&screen->songs), callbacks);

    nc_menu_set_selected_prefix(
        nc_media_library_tag_menu_base(&screen->tags),
        &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(
        nc_media_library_tag_menu_base(&screen->tags),
        &Config.selected_item_suffix);
    nc_menu_set_selected_prefix(
        nc_media_library_album_menu_base(&screen->albums),
        &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(
        nc_media_library_album_menu_base(&screen->albums),
        &Config.selected_item_suffix);
    nc_menu_set_selected_prefix(
        nc_media_library_song_menu_base(&screen->songs),
        &Config.selected_item_prefix);
    nc_menu_set_selected_suffix(
        nc_media_library_song_menu_base(&screen->songs),
        &Config.selected_item_suffix);

    nc_menu_set_cyclic_scrolling(
        nc_media_library_tag_menu_base(&screen->tags),
        Config.use_cyclic_scrolling);
    nc_menu_set_cyclic_scrolling(
        nc_media_library_album_menu_base(&screen->albums),
        Config.use_cyclic_scrolling);
    nc_menu_set_cyclic_scrolling(
        nc_media_library_song_menu_base(&screen->songs),
        Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(
        nc_media_library_tag_menu_base(&screen->tags),
        Config.centered_cursor);
    nc_menu_set_centered_cursor(
        nc_media_library_album_menu_base(&screen->albums),
        Config.centered_cursor);
    nc_menu_set_centered_cursor(
        nc_media_library_song_menu_base(&screen->songs),
        Config.centered_cursor);

    nc_screen_init(&screen->screen, native_library_callbacks, screen,
                   NC_SCREEN_TYPE_MEDIA_LIBRARY);
    native_library_update_menu_highlights(screen);
    native_library_layout(screen);
    return;
}

void
native_media_library_screen_destroy(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }

    for (uint32 i = 0; i < NATIVE_MEDIA_LIBRARY_COLUMN_LAST; i += 1) {
        ncm_regex_destroy(&screen->column_state[i].search_regex);
        ncm_regex_destroy(&screen->column_state[i].filter_regex);
        ncm_buffer_destroy(&screen->column_state[i].search_constraint);
        ncm_buffer_destroy(&screen->column_state[i].filter_constraint);
    }
    ncm_buffer_destroy(&screen->songs_title);
    ncm_buffer_destroy(&screen->albums_title);
    ncm_buffer_destroy(&screen->tags_title);
    nc_window_destroy(&screen->songs_window);
    nc_window_destroy(&screen->albums_window);
    nc_window_destroy(&screen->tags_window);
    nc_media_library_song_menu_destroy(&screen->songs);
    nc_media_library_album_menu_destroy(&screen->albums);
    nc_media_library_tag_menu_destroy(&screen->tags);
    if (screen->hooks.destroy) {
        screen->hooks.destroy(screen->hooks.user);
    }
    screen->hooks = (NativeMediaLibraryHooks){0};
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

enum NativeMediaLibraryMode
native_media_library_screen_mode(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS;
    }
    return screen->mode;
}

bool
native_media_library_screen_set_mode(NativeMediaLibraryScreen *screen,
                                     enum NativeMediaLibraryMode mode) {
    if (screen == NULL) {
        return false;
    }
    if ((mode < NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        || (mode >= NATIVE_MEDIA_LIBRARY_MODE_LAST)) {
        return false;
    }
    if (screen->mode == mode) {
        return true;
    }

    screen->mode = mode;
    native_media_library_screen_clear(screen);
    nc_menu_reset(nc_media_library_album_menu_base(&screen->albums));
    if ((mode != NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)) {
        screen->active_column = NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS;
    }
    native_library_update_titles(screen, true);
    native_library_update_menu_highlights(screen);
    native_library_layout(screen);
    return true;
}

enum NativeMediaLibraryMode
native_media_library_screen_toggle_mode(NativeMediaLibraryScreen *screen) {
    enum NativeMediaLibraryMode next_mode;

    if (screen == NULL) {
        return NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS;
    }

    switch (screen->mode) {
    case NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS:
        next_mode = NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS;
        break;
    case NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS:
        next_mode = NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY;
        break;
    case NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY:
        next_mode = NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS;
        break;
    default:
        next_mode = NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS;
        break;
    }
    native_media_library_screen_set_mode(screen, next_mode);
    return screen->mode;
}

enum NativeMediaLibraryColumn
native_media_library_screen_active_column(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NATIVE_MEDIA_LIBRARY_COLUMN_TAGS;
    }
    return screen->active_column;
}

bool
native_media_library_screen_set_active_column(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column) {
    if (screen == NULL) {
        return false;
    }
    if ((column < NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)
        || (column >= NATIVE_MEDIA_LIBRARY_COLUMN_LAST)) {
        return false;
    }
    if ((column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)
        && (screen->mode != NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)) {
        return false;
    }
    screen->active_column = column;
    native_library_update_menu_highlights(screen);
    return true;
}

bool
native_media_library_screen_column_visible(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column) {
    if (screen == NULL) {
        return false;
    }
    if ((column < NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)
        || (column >= NATIVE_MEDIA_LIBRARY_COLUMN_LAST)) {
        return false;
    }
    if (column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS) {
        return screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS;
    }
    return true;
}

NativeMediaLibraryColumnState *
native_media_library_screen_column_state(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column) {
    if (screen == NULL) {
        return NULL;
    }
    if ((column < NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)
        || (column >= NATIVE_MEDIA_LIBRARY_COLUMN_LAST)) {
        return NULL;
    }
    return &screen->column_state[column];
}

NcmBuffer *
native_media_library_screen_active_filter_constraint(
    NativeMediaLibraryScreen *screen) {
    NativeMediaLibraryColumnState *state;

    state = native_library_active_column_state(screen);
    if (state == NULL) {
        return NULL;
    }
    return &state->filter_constraint;
}

NcmBuffer *
native_media_library_screen_active_search_constraint(
    NativeMediaLibraryScreen *screen) {
    NativeMediaLibraryColumnState *state;

    state = native_library_active_column_state(screen);
    if (state == NULL) {
        return NULL;
    }
    return &state->search_constraint;
}

NcMediaLibraryTagRow *
native_media_library_screen_current_tag(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_media_library_tag_menu_current(&screen->tags);
}

NcMediaLibraryAlbumRow *
native_media_library_screen_current_album(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_media_library_album_menu_current(&screen->albums);
}

int64
native_media_library_screen_visible_song_count(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return 0;
    }
    return nc_menu_item_count(
        nc_media_library_song_menu_base(&screen->songs));
}

NcmSong *
native_media_library_screen_visible_song_at(
    NativeMediaLibraryScreen *screen, int64 pos) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_menu_active_item_at(
        nc_media_library_song_menu_base(&screen->songs), pos);
}

void
native_media_library_screen_format_tag_row(
    NativeMediaLibraryScreen *screen, NcMediaLibraryTagRow *row,
    NcmBuffer *output) {
    NcmBuffer converted;

    (void)screen;
    if (output == NULL) {
        return;
    }
    ncm_buffer_clear(output);
    if (row == NULL) {
        return;
    }
    if ((row->tag == NULL) || (row->tag_len <= 0)) {
        if ((Config.empty_tag != NULL) && (Config.empty_tag_len > 0)) {
            ncm_buffer_append(output, Config.empty_tag,
                              Config.empty_tag_len);
        }
        return;
    }

    converted = ncm_charset_utf8_to_locale(row->tag, row->tag_len);
    ncm_buffer_append(output, converted.data, converted.len);
    ncm_buffer_destroy(&converted);
    return;
}

void
native_media_library_screen_format_album_row(
    NativeMediaLibraryScreen *screen, NcMediaLibraryAlbumRow *row,
    NcmBuffer *output) {
    NcmBuffer raw;
    NcmBuffer converted;

    if (output == NULL) {
        return;
    }
    ncm_buffer_clear(output);
    if (row == NULL) {
        return;
    }
    if (row->all_tracks_entry) {
        ncm_buffer_append(output, STRLIT_ARGS("All tracks"));
        return;
    }

    ncm_buffer_init(&raw);
    if ((screen != NULL)
        && (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS)) {
        if ((row->tag == NULL) || (row->tag_len <= 0)) {
            if ((Config.empty_tag != NULL)
                && (Config.empty_tag_len > 0)) {
                ncm_buffer_append(&raw, Config.empty_tag,
                                  Config.empty_tag_len);
            }
        } else {
            ncm_buffer_append(&raw, row->tag, row->tag_len);
        }
        ncm_buffer_append(&raw, STRLIT_ARGS(" - "));
    }
    if ((Config.media_lib_primary_tag != MPD_TAG_DATE)
        && !Config.media_lib_hide_album_dates
        && (row->date != NULL) && (row->date_len > 0)) {
        ncm_buffer_append_byte(&raw, '(');
        ncm_buffer_append(&raw, row->date, row->date_len);
        ncm_buffer_append(&raw, STRLIT_ARGS(") "));
    }
    if ((row->album == NULL) || (row->album_len <= 0)) {
        ncm_buffer_append(&raw, STRLIT_ARGS("<no album>"));
    } else {
        ncm_buffer_append(&raw, row->album, row->album_len);
    }

    converted = ncm_charset_utf8_to_locale(raw.data, raw.len);
    ncm_buffer_append(output, converted.data, converted.len);
    ncm_buffer_destroy(&converted);
    ncm_buffer_destroy(&raw);
    return;
}

void
native_media_library_screen_format_song_row(
    NativeMediaLibraryScreen *screen, NcmSong *song, NcBuffer *output) {
    (void)screen;
    if (output == NULL) {
        return;
    }
    nc_buffer_clear(output);
    if (song == NULL) {
        return;
    }
    ncm_display_song_row(output, &Config.song_library_format, song, 0);
    return;
}

NcmTimePoint
native_media_library_screen_update_timer(NativeMediaLibraryScreen *screen) {
    NcmTimePoint timer = {0};

    if (screen) {
        timer = screen->update_timer;
    }
    return timer;
}

void
native_media_library_screen_set_update_timer(
    NativeMediaLibraryScreen *screen, NcmTimePoint timer) {
    if (screen == NULL) {
        return;
    }
    screen->update_timer = timer;
    return;
}

int64
native_media_library_screen_fetching_delay_ms(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return -1;
    }
    return screen->fetching_delay_ms;
}

int32
native_media_library_screen_window_timeout_ms(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }
    return screen->window_timeout_ms;
}

bool
native_media_library_screen_sort_by_mtime(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    return screen->sort_by_mtime;
}

bool
native_media_library_screen_toggle_sort_mode(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }

    screen->sort_by_mtime = !screen->sort_by_mtime;
    Config.media_library_sort_by_mtime = screen->sort_by_mtime;
    native_library_update_titles(screen, true);
    native_media_library_screen_request_tags_update(screen);
    native_media_library_screen_request_albums_update(screen);
    native_media_library_screen_request_songs_update(screen);
    return screen->sort_by_mtime;
}

bool
native_media_library_screen_previous_column_available(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        return true;
    }
    if ((screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS)) {
        return true;
    }
    return false;
}

bool
native_media_library_screen_next_column_available(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return true;
    }
    if ((screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)) {
        return true;
    }
    return false;
}

void
native_media_library_screen_previous_column(NativeMediaLibraryScreen *screen) {
    if (!native_media_library_screen_previous_column_available(screen)) {
        return;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        screen->active_column = NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS;
    } else {
        screen->active_column = NATIVE_MEDIA_LIBRARY_COLUMN_TAGS;
    }
    native_library_update_menu_highlights(screen);
    return;
}

void
native_media_library_screen_next_column(NativeMediaLibraryScreen *screen) {
    if (!native_media_library_screen_next_column_available(screen)) {
        return;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS) {
        screen->active_column = NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS;
    } else {
        screen->active_column = NATIVE_MEDIA_LIBRARY_COLUMN_SONGS;
    }
    native_library_update_menu_highlights(screen);
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

    if ((screen == NULL) || (tag_len < 0)
        || ((tag == NULL) && (tag_len > 0))) {
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
    if ((screen == NULL) || (song == NULL)) {
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

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }
    nc_menu_clear_items(nc_media_library_tag_menu_base(&screen->tags));
    for (int32 i = 0; i < songs->len; i += 1) {
        if (!ncm_song_tag_view(&songs->items[i], tag_type, 0, &view)) {
            continue;
        }
        if ((view.len <= 0) || native_library_has_tag(screen, &view)) {
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

    if ((screen == NULL) || (song == NULL)) {
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

    if ((screen == NULL) || (songs == NULL)) {
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
native_media_library_screen_apply_filter(
    NativeMediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    NcmError *error) {
    NativeMediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;
    NcMenu *menu;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing media library"));
        return false;
    }
    if (pattern_len <= 0) {
        native_media_library_screen_clear_filter(screen);
        return true;
    }
    if (pattern == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing filter pattern"));
        return false;
    }

    state = native_library_active_column_state(screen);
    if (state == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("invalid active column"));
        return false;
    }
    if (!ncm_regex_compile(&state->filter_regex, pattern, pattern_len,
                           NCM_REGEX_LITERAL_CASE_INSENSITIVE, error)) {
        return false;
    }
    if (!ncm_buffer_set(&state->filter_constraint, pattern, pattern_len)) {
        ncm_error_set(error, ENOMEM, STRLIT_ARGS("cannot save filter"));
        return false;
    }

    callbacks = native_library_display_callbacks(
        screen, screen->active_column, true);
    menu = native_media_library_screen_active_menu(screen);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_apply_filter(menu);
    state->filter_enabled = true;
    return true;
}

void
native_media_library_screen_clear_filter(NativeMediaLibraryScreen *screen) {
    NativeMediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }

    state = native_library_active_column_state(screen);
    if (state == NULL) {
        return;
    }
    ncm_regex_destroy(&state->filter_regex);
    ncm_regex_init(&state->filter_regex);
    ncm_buffer_clear(&state->filter_constraint);
    menu = native_media_library_screen_active_menu(screen);
    callbacks = native_library_display_callbacks(
        screen, screen->active_column, false);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_show_all_items(menu);
    state->filter_enabled = false;
    return;
}

bool
native_media_library_screen_search(NativeMediaLibraryScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   bool forward, bool wrap,
                                   bool skip_current, NcmError *error) {
    NativeMediaLibraryColumnState *state;
    NcMenu *menu;
    int64 count;
    int64 start;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing media library"));
        return false;
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing search pattern"));
        return false;
    }

    state = native_library_active_column_state(screen);
    if (state == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("invalid active column"));
        return false;
    }
    if (!ncm_regex_compile(&state->search_regex, pattern, pattern_len,
                           Config.regex_type, error)) {
        return false;
    }
    if (!ncm_buffer_set(&state->search_constraint, pattern, pattern_len)) {
        ncm_error_set(error, ENOMEM, STRLIT_ARGS("cannot save search"));
        return false;
    }
    state->search_enabled = true;

    menu = native_media_library_screen_active_menu(screen);
    count = nc_menu_item_count(menu);
    start = nc_menu_highlight(menu);
    if (skip_current) {
        if (forward) {
            start += 1;
        } else {
            start -= 1;
        }
    }

    for (int64 i = 0; i < count; i += 1) {
        int64 pos;

        if (forward) {
            pos = start + i;
        } else {
            pos = start - i;
        }
        if (wrap) {
            while (pos < 0) {
                pos += count;
            }
            while (pos >= count) {
                pos -= count;
            }
        }
        if ((pos < 0) || (pos >= count)) {
            continue;
        }
        if (native_library_active_item_matches(
                screen, menu, pos, &state->search_regex)) {
            return nc_menu_goto_selectable(menu, pos);
        }
    }
    return false;
}

void
native_media_library_screen_clear_search(NativeMediaLibraryScreen *screen) {
    NativeMediaLibraryColumnState *state;

    state = native_library_active_column_state(screen);
    if (state == NULL) {
        return;
    }
    ncm_regex_destroy(&state->search_regex);
    ncm_regex_init(&state->search_regex);
    ncm_buffer_clear(&state->search_constraint);
    state->search_enabled = false;
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
native_media_library_screen_list_tags(
    NativeMediaLibraryScreen *screen, enum mpd_tag_type tag_type,
    NcmMpdStringList *tags, NcmError *error) {
    if ((screen == NULL) || (screen->hooks.list_tags == NULL)) {
        ncm_error_set(error, ENOSYS, STRLIT_ARGS("tag hook is unavailable"));
        return false;
    }
    if (tags == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing tag list"));
        return false;
    }
    return screen->hooks.list_tags(screen->hooks.user, tag_type, tags,
                                   error);
}

bool
native_media_library_screen_list_all_songs(
    NativeMediaLibraryScreen *screen, NcmMpdSongList *songs,
    NcmError *error) {
    if ((screen == NULL) || (screen->hooks.list_all_songs == NULL)) {
        ncm_error_set(error, ENOSYS,
                      STRLIT_ARGS("song-list hook is unavailable"));
        return false;
    }
    if (songs == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing song list"));
        return false;
    }
    return screen->hooks.list_all_songs(screen->hooks.user, songs, error);
}

bool
native_media_library_screen_search_songs(
    NativeMediaLibraryScreen *screen,
    NativeMediaLibrarySongQuery *query, NcmMpdSongList *songs,
    NcmError *error) {
    if ((screen == NULL) || (screen->hooks.search_songs == NULL)) {
        ncm_error_set(error, ENOSYS,
                      STRLIT_ARGS("song-search hook is unavailable"));
        return false;
    }
    if ((query == NULL) || (songs == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing song search arguments"));
        return false;
    }
    return screen->hooks.search_songs(screen->hooks.user, query, songs,
                                      error);
}

bool
native_media_library_screen_add_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs, bool play,
    NcmError *error) {
    if ((screen == NULL) || (screen->hooks.add_songs == NULL)) {
        ncm_error_set(error, ENOSYS,
                      STRLIT_ARGS("add-songs hook is unavailable"));
        return false;
    }
    if ((songs == NULL) || (songs->len <= 0)) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing songs"));
        return false;
    }
    return screen->hooks.add_songs(screen->hooks.user, songs, play,
                                   error);
}

bool
native_media_library_screen_add_item_to_playlist(
    NativeMediaLibraryScreen *screen, bool play, NcmError *error) {
    NcmSongArray songs;
    bool result;

    if (screen == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing media library"));
        return false;
    }

    ncm_song_array_init(&songs);
    result = native_media_library_screen_selected_songs(screen, &songs);
    if (result && (songs.len <= 0)) {
        ncm_error_set(error, ENOENT, STRLIT_ARGS("no visible songs"));
        result = false;
    }
    if (result) {
        result = native_media_library_screen_add_songs(screen, &songs,
                                                       play, error);
    }
    ncm_song_array_destroy(&songs);
    return result;
}

bool
native_media_library_screen_locate_song(NativeMediaLibraryScreen *screen,
                                        NcmSong *song, NcmError *error) {
    NcMenu *menu;
    NcmSong *candidate;

    if ((screen == NULL) || (song == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing locate-song argument"));
        return false;
    }
    menu = nc_media_library_song_menu_base(&screen->songs);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        candidate = nc_menu_active_item_at(menu, i);
        if (candidate && ncm_song_equal(candidate, song)) {
            if (nc_menu_goto_selectable(menu, i)) {
                ncm_error_clear(error);
                return true;
            }
            break;
        }
    }
    ncm_error_set(error, ENOENT,
                  STRLIT_ARGS("song is not visible in media library"));
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
    NcMenu *albums;

    library = native_library_from_screen(screen);
    native_library_update_menu_highlights(library);
    if (native_media_library_screen_column_visible(
            library, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)) {
        native_library_refresh_menu(
            nc_media_library_tag_menu_base(&library->tags),
            &library->tags_window);
        nc_screen_draw_vertical_separator(
            nc_window_start_x(&library->albums_window) - 1);
    }

    albums = nc_media_library_album_menu_base(&library->albums);
    native_library_refresh_menu(albums, &library->albums_window);
    if (nc_menu_empty(albums)) {
        nc_window_go_to_xy(&library->albums_window, 0, 0);
        nc_window_print_data(&library->albums_window,
                             STRLIT_ARGS("No albums found."));
        nc_window_refresh(&library->albums_window);
    }
    nc_screen_draw_vertical_separator(
        nc_window_start_x(&library->songs_window) - 1);
    native_library_refresh_menu(
        nc_media_library_song_menu_base(&library->songs),
        &library->songs_window);
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
    int64 x_offset;
    int64 width;

    library = native_library_from_screen(screen);
    nc_screen_switcher_get_resize_params(screen, &x_offset, &width, true);
    native_media_library_screen_set_geometry(
        library, x_offset, width, ui_state_main_start_y(),
        ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
native_library_window_timeout(NcScreen *screen) {
    NativeMediaLibraryScreen *library;

    library = native_library_from_screen(screen);
    if (library == NULL) {
        return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }
    if (nc_menu_empty(nc_media_library_album_menu_base(&library->albums))
        || nc_menu_empty(nc_media_library_song_menu_base(&library->songs))) {
        return library->window_timeout_ms;
    }
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
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
    return native_library_tag_matches(
        item,
        &screen->column_state[NATIVE_MEDIA_LIBRARY_COLUMN_TAGS]
             .filter_regex);
}

static bool
native_library_album_filter(NcMenu *menu, void *item, void *user) {
    NativeMediaLibraryScreen *screen;

    (void)menu;
    screen = user;
    return native_library_album_matches(
        item,
        &screen->column_state[NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS]
             .filter_regex);
}

static bool
native_library_song_filter(NcMenu *menu, void *item, void *user) {
    NativeMediaLibraryScreen *screen;

    (void)menu;
    screen = user;
    return native_library_song_matches(
        item,
        &screen->column_state[NATIVE_MEDIA_LIBRARY_COLUMN_SONGS]
             .filter_regex);
}

static void
native_library_draw_tag(NcMenu *menu, NcWindow *window,
                        void *item, int64 pos, void *user) {
    NcmBuffer text;

    (void)menu;
    (void)pos;
    ncm_buffer_init(&text);
    native_media_library_screen_format_tag_row(user, item, &text);
    nc_window_print_data(window, text.data, text.len);
    ncm_buffer_destroy(&text);
    return;
}

static void
native_library_draw_album(NcMenu *menu, NcWindow *window,
                          void *item, int64 pos, void *user) {
    NcmBuffer text;

    (void)menu;
    (void)pos;
    ncm_buffer_init(&text);
    native_media_library_screen_format_album_row(user, item, &text);
    nc_window_print_data(window, text.data, text.len);
    ncm_buffer_destroy(&text);
    return;
}

static void
native_library_draw_song(NcMenu *menu, NcWindow *window,
                         void *item, int64 pos, void *user) {
    NcBuffer text;

    (void)menu;
    (void)pos;
    nc_buffer_init(&text);
    native_media_library_screen_format_song_row(user, item, &text);
    native_library_print_buffer(window, &text);
    nc_buffer_destroy(&text);
    return;
}

static void
native_library_print_buffer(NcWindow *window, NcBuffer *buffer) {
    NcBufferProperty *properties;
    char *data;
    int32 property_count;
    int32 property_index;
    int32 len;

    data = nc_buffer_data(buffer);
    len = nc_buffer_len(buffer);
    properties = nc_buffer_properties(buffer);
    property_count = nc_buffer_property_count(buffer);
    property_index = 0;

    for (int32 i = 0;; i += 1) {
        while ((property_index < property_count)
               && (properties[property_index].position == i)) {
            nc_buffer_apply_property(window, &properties[property_index]);
            property_index += 1;
        }
        if (i >= len) {
            break;
        }
        nc_window_print_char(window, data[i]);
    }
    return;
}

static bool
native_library_copy_song_at(NativeMediaLibraryScreen *screen,
                            NcmSongArray *songs, int64 pos) {
    NcmSong *song;

    song = nc_menu_active_item_at(
        nc_media_library_song_menu_base(&screen->songs), pos);
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
        if (row && ncm_string_equal(row->tag, row->tag_len,
                                            view->data, view->len)) {
            return true;
        }
    }
    return false;
}

static NativeMediaLibraryColumnState *
native_library_active_column_state(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return native_media_library_screen_column_state(screen,
                                                     screen->active_column);
}

static NcMenuDisplayCallbacks
native_library_display_callbacks(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column, bool filter_enabled) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.user = screen;
    switch (column) {
    case NATIVE_MEDIA_LIBRARY_COLUMN_TAGS:
        callbacks.draw = native_library_draw_tag;
        if (filter_enabled) {
            callbacks.filter = native_library_tag_filter;
        }
        break;
    case NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS:
        callbacks.draw = native_library_draw_album;
        if (filter_enabled) {
            callbacks.filter = native_library_album_filter;
        }
        break;
    case NATIVE_MEDIA_LIBRARY_COLUMN_SONGS:
        callbacks.draw = native_library_draw_song;
        if (filter_enabled) {
            callbacks.filter = native_library_song_filter;
        }
        break;
    default:
        break;
    }
    return callbacks;
}

static void
native_library_update_menu_highlights(
    NativeMediaLibraryScreen *screen) {
    NcMenu *tags;
    NcMenu *albums;
    NcMenu *songs;
    NcMenu *active;

    if (screen == NULL) {
        return;
    }
    tags = nc_media_library_tag_menu_base(&screen->tags);
    albums = nc_media_library_album_menu_base(&screen->albums);
    songs = nc_media_library_song_menu_base(&screen->songs);

    nc_menu_set_highlight_prefix(
        tags, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        tags, &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(
        albums, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        albums, &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(
        songs, &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(
        songs, &Config.current_item_inactive_column_suffix);

    active = native_media_library_screen_active_menu(screen);
    nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    return;
}

static void
native_library_update_titles(NativeMediaLibraryScreen *screen,
                             bool update_windows) {
    char *tag_type_name;
    int32 tag_type_name_len;

    ncm_buffer_clear(&screen->tags_title);
    ncm_buffer_clear(&screen->albums_title);
    ncm_buffer_clear(&screen->songs_title);
    if (Config.titles_visibility) {
        tag_type_name = ncm_tag_type_name(Config.media_lib_primary_tag);
        tag_type_name_len = native_library_cstring_len(tag_type_name);
        ncm_buffer_append(&screen->tags_title, tag_type_name,
                          tag_type_name_len);
        ncm_buffer_append_byte(&screen->tags_title, 's');
        ncm_buffer_append(&screen->albums_title,
                          STRLIT_ARGS("Albums"));
        ncm_buffer_append(&screen->songs_title, STRLIT_ARGS("Songs"));

        if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS) {
            ncm_buffer_append(&screen->albums_title,
                              STRLIT_ARGS(" (sorted by "));
            for (int32 i = 0; i < tag_type_name_len; i += 1) {
                char ch = tag_type_name[i];

                if ((ch >= 'A') && (ch <= 'Z')) {
                    ch = (char)(ch - 'A' + 'a');
                }
                ncm_buffer_append_byte(&screen->albums_title, ch);
            }
            if (screen->sort_by_mtime) {
                ncm_buffer_append(&screen->albums_title,
                                  STRLIT_ARGS(" and mtime"));
            }
            ncm_buffer_append_byte(&screen->albums_title, ')');
        } else if ((screen->mode
                    == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY)
                   && screen->sort_by_mtime) {
            ncm_buffer_append(&screen->albums_title,
                              STRLIT_ARGS(" (sorted by mtime)"));
        }
    }

    if (update_windows) {
        nc_window_set_title(&screen->tags_window,
                            screen->tags_title.data,
                            screen->tags_title.len);
        nc_window_set_title(&screen->albums_window,
                            screen->albums_title.data,
                            screen->albums_title.len);
        nc_window_set_title(&screen->songs_window,
                            screen->songs_title.data,
                            screen->songs_title.len);
    }
    return;
}

static void
native_library_refresh_menu(NcMenu *menu, NcWindow *window) {
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static bool
native_library_active_item_matches(NativeMediaLibraryScreen *screen,
                                   NcMenu *menu, int64 pos,
                                   NcmRegex *regex) {
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return native_library_album_matches(
            nc_menu_active_item_at(menu, pos), regex);
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        return native_library_song_matches(
            nc_menu_active_item_at(menu, pos), regex);
    }
    return native_library_tag_matches(nc_menu_active_item_at(menu, pos),
                                      regex);
}

static bool
native_library_tag_matches(NcMediaLibraryTagRow *row, NcmRegex *regex) {
    if ((row == NULL) || (row->tag == NULL)) {
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
    if (row->album
        && ncm_regex_search(regex, row->album, row->album_len)) {
        return true;
    }
    if (row->tag
        && ncm_regex_search(regex, row->tag, row->tag_len)) {
        return true;
    }
    if (row->date
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
    int64 left_width;
    int64 middle_width;
    int64 right_width;
    int64 middle_x;
    int64 right_x;
    int32 left_ratio;
    int32 middle_ratio;
    int32 right_ratio;
    int32 ratio_sum;

    if ((screen == NULL) || (screen->width <= 0)) {
        return;
    }

    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        left_ratio = native_library_ratio_value(
            &Config.media_library_column_width_ratio_three, 0, 1);
        middle_ratio = native_library_ratio_value(
            &Config.media_library_column_width_ratio_three, 1, 1);
        right_ratio = native_library_ratio_value(
            &Config.media_library_column_width_ratio_three, 2, 1);
        ratio_sum = left_ratio + middle_ratio + right_ratio;

        left_width = screen->width*left_ratio/ratio_sum - 1;
        middle_width = screen->width*middle_ratio/ratio_sum;
        right_width = screen->width - left_width - middle_width - 2;
        if (left_width <= 0) {
            left_width = 1;
        }
        if (middle_width <= 0) {
            middle_width = 1;
        }
        if (right_width <= 0) {
            right_width = 1;
        }
        middle_x = screen->start_x + left_width + 1;
        right_x = middle_x + middle_width + 1;

        nc_window_move_to(&screen->tags_window, screen->start_x,
                          screen->main_start_y);
        nc_window_resize(&screen->tags_window, left_width,
                         screen->main_height);
        nc_window_move_to(&screen->albums_window, middle_x,
                          screen->main_start_y);
        nc_window_resize(&screen->albums_window, middle_width,
                         screen->main_height);
        nc_window_move_to(&screen->songs_window, right_x,
                          screen->main_start_y);
        nc_window_resize(&screen->songs_window, right_width,
                         screen->main_height);
        return;
    }

    left_ratio = native_library_ratio_value(
        &Config.media_library_column_width_ratio_two, 0, 1);
    right_ratio = native_library_ratio_value(
        &Config.media_library_column_width_ratio_two, 1, 1);
    ratio_sum = left_ratio + right_ratio;
    middle_width = screen->width*left_ratio/ratio_sum;
    right_width = screen->width - middle_width - 1;
    if (middle_width <= 0) {
        middle_width = 1;
    }
    if (right_width <= 0) {
        right_width = 1;
    }
    middle_x = screen->start_x;
    right_x = middle_x + middle_width + 1;

    nc_window_move_to(&screen->albums_window, middle_x,
                      screen->main_start_y);
    nc_window_resize(&screen->albums_window, middle_width,
                     screen->main_height);
    nc_window_move_to(&screen->songs_window, right_x,
                      screen->main_start_y);
    nc_window_resize(&screen->songs_window, right_width,
                     screen->main_height);
    return;
}

static int32
native_library_ratio_value(NcmInt32Array *ratios, int32 idx,
                           int32 fallback) {
    if ((ratios == NULL) || (idx < 0) || (idx >= ratios->len)) {
        return fallback;
    }
    if (ratios->items[idx] <= 0) {
        return fallback;
    }
    return ratios->items[idx];
}

static bool
native_library_set_owned_string(char **dest, int32 *dest_len,
                                int32 *dest_cap, char *source,
                                int32 source_len) {
    char *copy;
    int32 cap;

    if ((dest == NULL) || (dest_len == NULL) || (dest_cap == NULL)) {
        return false;
    }
    native_library_free_owned_string(dest, dest_len, dest_cap);
    if ((source == NULL) || (source_len <= 0)) {
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
    if (*data) {
        ncm_free(*data, *cap);
    }
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}

static int32
native_library_cstring_len(char *string) {
    int32 len;

    if (string == NULL) {
        return 0;
    }
    len = 0;
    while (string[len] != '\0') {
        len += 1;
    }
    return len;
}

static char *
native_library_query_cstring(NcmBuffer *buffer, char *string,
                             int32 string_len) {
    if ((buffer == NULL) || (string == NULL)) {
        return NULL;
    }
    if (string_len < 0) {
        string_len = native_library_cstring_len(string);
    }
    ncm_buffer_clear(buffer);
    ncm_buffer_append(buffer, string, string_len);
    ncm_buffer_append_byte(buffer, '\0');
    buffer->len = string_len;
    return buffer->data;
}

static bool
native_library_mpd_list_tags(void *user, enum mpd_tag_type tag_type,
                             NcmMpdStringList *tags, NcmError *error) {
    NcmMpdClient *client;

    client = user;
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    return ncm_mpd_client_get_list(client, tag_type, tags, error);
}

static bool
native_library_mpd_list_all_songs(void *user, NcmMpdSongList *songs,
                                  NcmError *error) {
    NcmMpdClient *client;

    client = user;
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }
    return ncm_mpd_client_get_directory_recursive(client, (char *)"/",
                                                   songs, error);
}

static bool
native_library_mpd_search_songs(void *user,
                                NativeMediaLibrarySongQuery *query,
                                NcmMpdSongList *songs,
                                NcmError *error) {
    NcmMpdClient *client;
    NcmBuffer primary;
    NcmBuffer album;
    NcmBuffer date;
    char *value;
    bool result;

    client = user;
    if ((client == NULL) || (query == NULL) || (songs == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing MPD search argument"));
        return false;
    }

    ncm_buffer_init(&primary);
    ncm_buffer_init(&album);
    ncm_buffer_init(&date);
    result = ncm_mpd_client_start_search(client, true, error);
    if (result && query->match_primary_tag) {
        value = native_library_query_cstring(
            &primary, query->primary_value, query->primary_value_len);
        if (value == NULL) {
            ncm_error_set(error, EINVAL,
                          STRLIT_ARGS("missing primary tag value"));
            result = false;
        } else {
            result = ncm_mpd_client_add_search_tag(
                client, query->primary_tag, value, error);
        }
    }
    if (result && query->match_album) {
        value = native_library_query_cstring(
            &album, query->album, query->album_len);
        if (value == NULL) {
            ncm_error_set(error, EINVAL,
                          STRLIT_ARGS("missing album value"));
            result = false;
        } else {
            result = ncm_mpd_client_add_search_tag(
                client, MPD_TAG_ALBUM, value, error);
        }
    }
    if (result && query->match_date) {
        value = native_library_query_cstring(
            &date, query->date, query->date_len);
        if (value == NULL) {
            ncm_error_set(error, EINVAL,
                          STRLIT_ARGS("missing date value"));
            result = false;
        } else {
            result = ncm_mpd_client_add_search_tag(
                client, MPD_TAG_DATE, value, error);
        }
    }
    if (result) {
        result = ncm_mpd_client_commit_search_songs(client, songs, error);
    }
    ncm_buffer_destroy(&date);
    ncm_buffer_destroy(&album);
    ncm_buffer_destroy(&primary);
    return result;
}

static bool
native_library_mpd_add_songs(void *user, NcmSongArray *songs, bool play,
                             NcmError *error) {
    NcmMpdClient *client;
    NcmMpdSongList queue;
    NcmMpdSongList additions;
    int32 play_pos;
    bool result;

    client = user;
    if ((client == NULL) || (songs == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing MPD add argument"));
        return false;
    }

    ncm_mpd_song_list_init(&queue);
    ncm_mpd_song_list_init(&additions);
    result = true;
    for (int32 i = 0; i < songs->len; i += 1) {
        if (!ncm_mpd_song_list_append_copy(&additions,
                                           &songs->items[i])) {
            ncm_error_set(error, ENOMEM,
                          STRLIT_ARGS("cannot copy songs for MPD"));
            result = false;
            break;
        }
    }

    play_pos = -1;
    if (result && play) {
        result = ncm_mpd_client_get_queue(client, &queue, error);
        if (result) {
            play_pos = ncm_mpd_song_list_count(&queue);
        }
    }
    if (result) {
        result = ncm_mpd_client_add_song_list(client, &additions, -1,
                                              error);
    }
    if (result && play) {
        result = ncm_mpd_client_play_pos(client, play_pos, error);
    }
    ncm_mpd_song_list_destroy(&additions);
    ncm_mpd_song_list_destroy(&queue);
    return result;
}
