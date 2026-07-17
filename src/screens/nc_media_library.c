#include "screens/nc_media_library.h"

#include <errno.h>

#include "global.h"
#include "helpers.h"
#include "settings.h"
#include "statusbar.h"
#include "ui_state.h"
#include "c/ncm_base.h"
#include "c/ncm_charset.h"
#include "c/ncm_comparators.h"
#include "c/ncm_display.h"
#include "c/ncm_format.h"
#include "c/ncm_string.h"
#include "screens/screen_switcher.h"
#include "cbase/util.c"

static NcWindow *native_library_active_window(NcScreen *screen);
static void native_library_refresh(NcScreen *screen);
static void native_library_refresh_window(NcScreen *screen);
static void native_library_scroll(NcScreen *screen, enum NcScroll where);
static void native_library_finish_list_change(NcScreen *screen);
static void native_library_mouse_button_pressed(NcScreen *screen,
                                                MEVENT event);
static void native_library_switch_to(NcScreen *screen);
static void native_library_resize(NcScreen *screen);
static int32 native_library_window_timeout(NcScreen *screen);
static char *native_library_title(NcScreen *screen);
static void native_library_update(NcScreen *screen);
static bool native_library_is_lockable(NcScreen *screen);
static bool native_library_is_mergable(NcScreen *screen);
static void native_library_destroy_callback(NcScreen *screen);
static void native_library_print_buffer(NcWindow *window, NcBuffer *buffer);
static bool native_library_copy_song_at(NativeMediaLibraryScreen *screen,
                                        NcmSongArray *songs, int64 pos);
static NcMenu *native_library_column_menu(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column);
static bool native_library_column_has_visible_items(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column);
static bool native_library_menu_item_is_separator(NcMenu *menu, void *item);
static bool native_library_tag_matches(NativeMediaLibraryScreen *screen,
                                       NcMediaLibraryTagRow *row,
                                       NcmRegex *regex);
static bool native_library_album_matches(NativeMediaLibraryScreen *screen,
                                         NcMediaLibraryAlbumRow *row,
                                         NcmRegex *regex);
static bool native_library_song_matches(NativeMediaLibraryScreen *screen,
                                        NcmSong *song, NcmRegex *regex);
static bool native_library_collect_selected_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error);
static bool native_library_collect_current_item_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error);
static void native_library_mouse_scroll(NativeMediaLibraryScreen *screen,
                                         enum NcScroll where);
static bool native_library_mouse_select(
    NativeMediaLibraryScreen *screen, enum NativeMediaLibraryColumn column,
    NcMenu *menu, int32 y, bool right_click);
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
static int32 optional_strlen32(char *string);
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
static void native_library_tag_array_item_init(void *item);
static void native_library_tag_array_item_destroy(void *item);
static void native_library_album_array_item_init(void *item);
static void native_library_album_array_item_destroy(void *item);
static bool native_library_update_tags(
    NativeMediaLibraryScreen *screen, NcmError *error);
static bool native_library_update_albums(
    NativeMediaLibraryScreen *screen, NcmError *error);
static bool native_library_update_songs(
    NativeMediaLibraryScreen *screen, NcmError *error);
static bool native_library_replace_tags(
    NativeMediaLibraryScreen *screen, NativeMediaLibraryTagArray *tags);
static bool native_library_replace_albums(
    NativeMediaLibraryScreen *screen, NativeMediaLibraryAlbumArray *albums);
static bool native_library_replace_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs);
static void native_library_apply_column_filter(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column, NcMenu *menu);
static void native_library_restore_tag_identity(
    NcMediaLibraryTagMenu *menu, NcMediaLibraryTagRow *identity,
    bool identity_valid, int64 fallback);
static void native_library_restore_album_identity(
    NcMediaLibraryAlbumMenu *menu, NcMediaLibraryAlbumRow *identity,
    bool identity_valid, int64 fallback);
static void native_library_restore_song_identity(
    NcMediaLibrarySongMenu *menu, NcmSong *identity,
    bool identity_valid, int64 fallback);
static bool native_library_tag_identity_equal(
    NcMediaLibraryTagRow *left, NcMediaLibraryTagRow *right);
static bool native_library_album_identity_equal(
    NcMediaLibraryAlbumRow *left, NcMediaLibraryAlbumRow *right);
static void native_library_set_observed_tag(
    NativeMediaLibraryScreen *screen, NcMediaLibraryTagRow *tag);
static void native_library_set_observed_album(
    NativeMediaLibraryScreen *screen, NcMediaLibraryAlbumRow *album);
static void native_library_reset_observed_highlights(
    NativeMediaLibraryScreen *screen);
static void native_library_restart_update_timer(
    NativeMediaLibraryScreen *screen);
static bool native_library_tags_pending(
    NativeMediaLibraryScreen *screen);
static bool native_library_albums_pending(
    NativeMediaLibraryScreen *screen);
static bool native_library_songs_pending(
    NativeMediaLibraryScreen *screen);
static bool native_library_fetch_delay_elapsed(
    NativeMediaLibraryScreen *screen);
static void native_library_set_conversion_error(NcmError *error,
                                                 char *message,
                                                 int32 message_len);
static void native_library_request_all_updates(
    NativeMediaLibraryScreen *screen);

static NcScreenCallbacks native_library_callbacks = {
    .active_window = native_library_active_window,
    .refresh = native_library_refresh,
    .refresh_window = native_library_refresh_window,
    .scroll = native_library_scroll,
    .list_change_finished = native_library_finish_list_change,
    .switch_to = native_library_switch_to,
    .resize = native_library_resize,
    .window_timeout = native_library_window_timeout,
    .title = native_library_title,
    .update = native_library_update,
    .mouse_button_pressed = native_library_mouse_button_pressed,
    .is_lockable = native_library_is_lockable,
    .is_mergable = native_library_is_mergable,
    .destroy = native_library_destroy_callback,
};

static NcmArrayItemCallbacks native_library_tag_array_callbacks = {
    .init = native_library_tag_array_item_init,
    .destroy = native_library_tag_array_item_destroy,
};

static NcmArrayItemCallbacks native_library_album_array_callbacks = {
    .init = native_library_album_array_item_init,
    .destroy = native_library_album_array_item_destroy,
};

NCM_ARRAY_DEFINE_INIT(native_media_library_tag_array,
                      NativeMediaLibraryTagArray)
NCM_ARRAY_DEFINE_CLEAR(native_media_library_tag_array,
                       NativeMediaLibraryTagArray,
                       &native_library_tag_array_callbacks)
NCM_ARRAY_DEFINE_DESTROY(native_media_library_tag_array,
                         NativeMediaLibraryTagArray)
NCM_ARRAY_DEFINE_MOVE(native_media_library_tag_array,
                      NativeMediaLibraryTagArray)
NCM_ARRAY_DEFINE_RESERVE(native_media_library_tag_array,
                         NativeMediaLibraryTagArray)
NCM_ARRAY_DEFINE_APPEND(native_media_library_tag_array,
                        NativeMediaLibraryTagArray,
                        NcMediaLibraryTagRow,
                        &native_library_tag_array_callbacks)
NCM_ARRAY_DEFINE_REMOVE_ORDERED(native_media_library_tag_array,
                                NativeMediaLibraryTagArray,
                                &native_library_tag_array_callbacks)

NCM_ARRAY_DEFINE_INIT(native_media_library_album_array,
                      NativeMediaLibraryAlbumArray)
NCM_ARRAY_DEFINE_CLEAR(native_media_library_album_array,
                       NativeMediaLibraryAlbumArray,
                       &native_library_album_array_callbacks)
NCM_ARRAY_DEFINE_DESTROY(native_media_library_album_array,
                         NativeMediaLibraryAlbumArray)
NCM_ARRAY_DEFINE_MOVE(native_media_library_album_array,
                      NativeMediaLibraryAlbumArray)
NCM_ARRAY_DEFINE_RESERVE(native_media_library_album_array,
                         NativeMediaLibraryAlbumArray)
NCM_ARRAY_DEFINE_APPEND(native_media_library_album_array,
                        NativeMediaLibraryAlbumArray,
                        NativeMediaLibraryAlbumItem,
                        &native_library_album_array_callbacks)
NCM_ARRAY_DEFINE_REMOVE_ORDERED(native_media_library_album_array,
                                NativeMediaLibraryAlbumArray,
                                &native_library_album_array_callbacks)

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
    nc_media_library_tag_row_init(&screen->observed_tag);
    nc_media_library_album_row_init(&screen->observed_album);

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
    screen->observed_tag_valid = false;
    screen->observed_album_valid = false;
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
    nc_media_library_album_row_destroy(&screen->observed_album);
    nc_media_library_tag_row_destroy(&screen->observed_tag);
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

int32
native_media_library_screen_column_count(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return 0;
    }
    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        return 3;
    }
    return 2;
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
native_media_library_screen_item_available(
    NativeMediaLibraryScreen *screen) {
    NcMenu *menu;

    menu = native_media_library_screen_active_menu(screen);
    if (menu == NULL) {
        return false;
    }
    return !nc_menu_empty(menu);
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

bool
native_media_library_screen_current_primary_tag_value(
    NativeMediaLibraryScreen *screen, char **value, int32 *value_len) {
    NcMediaLibraryTagRow *tag;
    NcMediaLibraryAlbumRow *album;

    if ((screen == NULL) || (value == NULL) || (value_len == NULL)) {
        return false;
    }

    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        tag = native_media_library_screen_current_tag(screen);
        if (tag == NULL) {
            return false;
        }
        *value = tag->tag;
        *value_len = tag->tag_len;
        return true;
    }

    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
        return false;
    }

    album = native_media_library_screen_current_album(screen);
    if (album == NULL) {
        return false;
    }
    *value = album->tag;
    *value_len = album->tag_len;
    return true;
}

bool
native_media_library_screen_current_album_value(
    NativeMediaLibraryScreen *screen, char **album, int32 *album_len) {
    NcMediaLibraryAlbumRow *row;

    if ((screen == NULL) || (album == NULL) || (album_len == NULL)) {
        return false;
    }

    row = native_media_library_screen_current_album(screen);
    if ((row == NULL) || row->all_tracks_entry) {
        return false;
    }

    *album = row->album;
    *album_len = row->album_len;
    return true;
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
    ncm_display_song_row(output, &Config.song_library_format, song,
                         NCM_FORMAT_FLAG_ALL);
    return;
}

static void
native_library_tag_array_item_init(void *item) {
    nc_media_library_tag_row_init(item);
    return;
}

static void
native_library_tag_array_item_destroy(void *item) {
    nc_media_library_tag_row_destroy(item);
    return;
}

static void
native_library_album_array_item_init(void *item) {
    NativeMediaLibraryAlbumItem *album;

    album = item;
    nc_media_library_album_row_init(&album->row);
    album->menu_flags = NC_MENU_ITEM_SELECTABLE;
    return;
}

static void
native_library_album_array_item_destroy(void *item) {
    NativeMediaLibraryAlbumItem *album;

    album = item;
    nc_media_library_album_row_destroy(&album->row);
    album->menu_flags = NC_MENU_ITEM_SELECTABLE;
    return;
}

static int32
native_library_compare_tag_rows(NcMediaLibraryTagRow *left,
                                NcMediaLibraryTagRow *right) {
    char *left_tag;
    char *right_tag;

    if (Config.media_library_sort_by_mtime) {
        if (left->mtime > right->mtime) {
            return -1;
        }
        if (left->mtime < right->mtime) {
            return 1;
        }
        return 0;
    }

    left_tag = left->tag;
    right_tag = right->tag;
    if (left_tag == NULL) {
        left_tag = "";
    }
    if (right_tag == NULL) {
        right_tag = "";
    }
    return ncm_compare_locale_strings(left_tag, left->tag_len,
                                      right_tag, right->tag_len,
                                      Config.ignore_leading_the);
}

static int32
native_library_compare_album_items(NativeMediaLibraryAlbumItem *left,
                                   NativeMediaLibraryAlbumItem *right) {
    NcMediaLibraryAlbumRow *left_row;
    NcMediaLibraryAlbumRow *right_row;
    char *left_data;
    char *right_data;
    int32 result;

    left_row = &left->row;
    right_row = &right->row;
    if (Config.media_library_sort_by_mtime) {
        if (left_row->mtime > right_row->mtime) {
            return -1;
        }
        if (left_row->mtime < right_row->mtime) {
            return 1;
        }
        return 0;
    }

    left_data = left_row->tag;
    right_data = right_row->tag;
    if (left_data == NULL) {
        left_data = "";
    }
    if (right_data == NULL) {
        right_data = "";
    }
    result = ncm_compare_locale_strings(
        left_data, left_row->tag_len,
        right_data, right_row->tag_len,
        Config.ignore_leading_the);
    if (result != 0) {
        return result;
    }

    left_data = left_row->date;
    right_data = right_row->date;
    if (left_data == NULL) {
        left_data = "";
    }
    if (right_data == NULL) {
        right_data = "";
    }
    result = ncm_compare_locale_strings(
        left_data, left_row->date_len,
        right_data, right_row->date_len,
        Config.ignore_leading_the);
    if (result != 0) {
        return result;
    }

    left_data = left_row->album;
    right_data = right_row->album;
    if (left_data == NULL) {
        left_data = "";
    }
    if (right_data == NULL) {
        right_data = "";
    }
    return ncm_compare_locale_strings(
        left_data, left_row->album_len,
        right_data, right_row->album_len,
        Config.ignore_leading_the);
}

static int32
native_library_compare_bytes(char *left, int32 left_len,
                             char *right, int32 right_len) {
    int32 common_len;

    common_len = left_len;
    if (right_len < common_len) {
        common_len = right_len;
    }
    for (int32 i = 0; i < common_len; i += 1) {
        unsigned char left_byte;
        unsigned char right_byte;

        left_byte = (unsigned char)left[i];
        right_byte = (unsigned char)right[i];
        if (left_byte < right_byte) {
            return -1;
        }
        if (left_byte > right_byte) {
            return 1;
        }
    }
    if (left_len < right_len) {
        return -1;
    }
    if (left_len > right_len) {
        return 1;
    }
    return 0;
}

static int32
native_library_compare_song_getter(NcmSong *left, NcmSong *right,
                                   enum NcmSongGetter getter) {
    NcmBuffer left_tags;
    NcmBuffer right_tags;
    char *left_data;
    char *right_data;
    char *separator;
    int32 separator_len;
    int32 result;

    separator = Config.tags_separator;
    separator_len = Config.tags_separator_len;
    if (separator == NULL) {
        separator = "";
        separator_len = 0;
    }
    left_tags = ncm_song_tags_buffer(
        left, getter, separator, separator_len,
        Config.show_duplicate_tags);
    right_tags = ncm_song_tags_buffer(
        right, getter, separator, separator_len,
        Config.show_duplicate_tags);
    left_data = left_tags.data;
    right_data = right_tags.data;
    if (left_data == NULL) {
        left_data = "";
    }
    if (right_data == NULL) {
        right_data = "";
    }
    result = ncm_compare_locale_strings(
        left_data, left_tags.len, right_data, right_tags.len,
        Config.ignore_leading_the);
    ncm_buffer_destroy(&right_tags);
    ncm_buffer_destroy(&left_tags);
    return result;
}

static int32
native_library_compare_songs(NcmSong *left, NcmSong *right) {
    static enum NcmSongGetter getters[] = {
        NCM_SONG_GETTER_DATE,
        NCM_SONG_GETTER_ALBUM,
        NCM_SONG_GETTER_DISC,
        NCM_SONG_GETTER_TRACK_NUMBER,
    };
    NcmBuffer left_text;
    NcmBuffer right_text;
    int32 result;

    for (int32 i = 0; i < (int32)LENGTH(getters); i += 1) {
        result = native_library_compare_song_getter(
            left, right, getters[i]);
        if (result != 0) {
            return result;
        }
    }

    left_text = ncm_format_render_string(
        &Config.song_library_format, left);
    right_text = ncm_format_render_string(
        &Config.song_library_format, right);
    result = native_library_compare_bytes(
        left_text.data, left_text.len, right_text.data, right_text.len);
    ncm_buffer_destroy(&right_text);
    ncm_buffer_destroy(&left_text);
    return result;
}

static void
native_library_sort_tags(NativeMediaLibraryTagArray *tags) {
    for (int32 i = 1; i < tags->len; i += 1) {
        int32 j;

        j = i;
        while ((j > 0)
               && (native_library_compare_tag_rows(
                       &tags->items[j], &tags->items[j - 1]) < 0)) {
            NcMediaLibraryTagRow tmp;

            tmp = tags->items[j];
            tags->items[j] = tags->items[j - 1];
            tags->items[j - 1] = tmp;
            j -= 1;
        }
    }
    return;
}

static void
native_library_sort_albums(NativeMediaLibraryAlbumArray *albums) {
    for (int32 i = 1; i < albums->len; i += 1) {
        int32 j;

        j = i;
        while ((j > 0)
               && (native_library_compare_album_items(
                       &albums->items[j],
                       &albums->items[j - 1]) < 0)) {
            NativeMediaLibraryAlbumItem tmp;

            tmp = albums->items[j];
            albums->items[j] = albums->items[j - 1];
            albums->items[j - 1] = tmp;
            j -= 1;
        }
    }
    return;
}

static void
native_library_sort_songs(NcmSongArray *songs) {
    for (int32 i = 1; i < songs->len; i += 1) {
        int32 j;

        j = i;
        while ((j > 0)
               && (native_library_compare_songs(
                       &songs->items[j], &songs->items[j - 1]) < 0)) {
            NcmSong tmp;

            tmp = songs->items[j];
            songs->items[j] = songs->items[j - 1];
            songs->items[j - 1] = tmp;
            j -= 1;
        }
    }
    return;
}

static int32
native_library_find_tag(NativeMediaLibraryTagArray *tags,
                        char *tag, int32 tag_len) {
    for (int32 i = 0; i < tags->len; i += 1) {
        if (STREQUAL(tags->items[i].tag,
                             tags->items[i].tag_len,
                             tag, tag_len)) {
            return i;
        }
    }
    return -1;
}

static int32
native_library_find_album(NativeMediaLibraryAlbumArray *albums,
                          char *tag, int32 tag_len,
                          char *album, int32 album_len,
                          char *date, int32 date_len) {
    for (int32 i = 0; i < albums->len; i += 1) {
        NcMediaLibraryAlbumRow *row;

        row = &albums->items[i].row;
        if (row->all_tracks_entry) {
            continue;
        }
        if (!STREQUAL(row->tag, row->tag_len,
                              tag, tag_len)) {
            continue;
        }
        if (!STREQUAL(row->album, row->album_len,
                              album, album_len)) {
            continue;
        }
        if (!STREQUAL(row->date, row->date_len,
                              date, date_len)) {
            continue;
        }
        return i;
    }
    return -1;
}

static bool
native_library_append_tag(NativeMediaLibraryTagArray *tags,
                          char *tag, int32 tag_len, time_t mtime) {
    NcMediaLibraryTagRow *row;

    row = native_media_library_tag_array_append(tags);
    if (row == NULL) {
        return false;
    }
    if (!native_library_set_owned_string(&row->tag, &row->tag_len,
                                         &row->tag_cap, tag, tag_len)) {
        native_media_library_tag_array_remove_ordered(
            tags, tags->len - 1);
        return false;
    }
    row->mtime = mtime;
    return true;
}

static bool
native_library_append_album(NativeMediaLibraryAlbumArray *albums,
                            char *tag, int32 tag_len,
                            char *album, int32 album_len,
                            char *date, int32 date_len,
                            time_t mtime, bool all_tracks_entry,
                            uint32 menu_flags) {
    NativeMediaLibraryAlbumItem *item;
    NcMediaLibraryAlbumRow *row;

    item = native_media_library_album_array_append(albums);
    if (item == NULL) {
        return false;
    }
    row = &item->row;
    if (!native_library_set_owned_string(&row->tag, &row->tag_len,
                                         &row->tag_cap, tag, tag_len)
        || !native_library_set_owned_string(
               &row->album, &row->album_len, &row->album_cap,
               album, album_len)
        || !native_library_set_owned_string(
               &row->date, &row->date_len, &row->date_cap,
               date, date_len)) {
        native_media_library_album_array_remove_ordered(
            albums, albums->len - 1);
        return false;
    }
    row->mtime = mtime;
    row->all_tracks_entry = all_tracks_entry;
    item->menu_flags = menu_flags;
    return true;
}

static bool
native_library_song_first_tag(NcmSong *song, enum mpd_tag_type tag,
                              NcmStringView *view) {
    ncm_string_view_init(view);
    return ncm_song_tag_view(song, tag, 0, view);
}

static bool
native_library_add_three_column_album(
    NativeMediaLibraryAlbumArray *albums, NcmSong *song,
    char *selected_tag, int32 selected_tag_len) {
    NcmStringView album;
    NcmStringView date;
    int32 existing;

    native_library_song_first_tag(song, MPD_TAG_ALBUM, &album);
    native_library_song_first_tag(song, MPD_TAG_DATE, &date);
    if (!Config.media_library_albums_split_by_date) {
        ncm_string_view_clear(&date);
    }

    existing = native_library_find_album(
        albums, selected_tag, selected_tag_len,
        album.data, album.len, date.data, date.len);
    if (existing >= 0) {
        if (song->last_modified
            > albums->items[existing].row.mtime) {
            albums->items[existing].row.mtime = song->last_modified;
        }
        return true;
    }
    return native_library_append_album(
        albums, selected_tag, selected_tag_len,
        album.data, album.len, date.data, date.len,
        song->last_modified, false, NC_MENU_ITEM_SELECTABLE);
}

static bool
native_library_add_two_column_albums(
    NativeMediaLibraryAlbumArray *albums, NcmSong *song,
    enum NativeMediaLibraryMode mode, enum mpd_tag_type primary_tag) {
    NcmStringView album;
    NcmStringView date;
    NcmStringView primary_value;

    native_library_song_first_tag(song, MPD_TAG_ALBUM, &album);
    native_library_song_first_tag(song, MPD_TAG_DATE, &date);
    if (!Config.media_library_albums_split_by_date) {
        ncm_string_view_clear(&date);
    }

    for (uint32 i = 0;
         ncm_song_tag_view(song, primary_tag, i, &primary_value);
         i += 1) {
        char *tag;
        int32 tag_len;
        int32 existing;

        tag = primary_value.data;
        tag_len = primary_value.len;
        if (mode == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
            tag = NULL;
            tag_len = 0;
        }
        existing = native_library_find_album(
            albums, tag, tag_len, album.data, album.len,
            date.data, date.len);
        if (existing >= 0) {
            albums->items[existing].row.mtime = song->last_modified;
            continue;
        }
        if (!native_library_append_album(
                albums, tag, tag_len, album.data, album.len,
                date.data, date.len, song->last_modified, false,
                NC_MENU_ITEM_SELECTABLE)) {
            return false;
        }
    }
    return true;
}

bool
native_media_library_tags_from_strings(
    NativeMediaLibraryTagArray *tags, NcmMpdStringList *strings) {
    NativeMediaLibraryTagArray replacement;

    if ((tags == NULL) || (strings == NULL)) {
        return false;
    }

    native_media_library_tag_array_init(&replacement);
    for (int32 i = 0; i < ncm_mpd_string_list_count(strings); i += 1) {
        NcmMpdString *string;

        string = ncm_mpd_string_list_at(strings, i);
        if (string == NULL) {
            native_media_library_tag_array_destroy(&replacement);
            return false;
        }
        if (native_library_find_tag(&replacement, string->value,
                                    string->value_len) >= 0) {
            continue;
        }
        if (!native_library_append_tag(
                &replacement, string->value, string->value_len, 0)) {
            native_media_library_tag_array_destroy(&replacement);
            return false;
        }
    }
    native_library_sort_tags(&replacement);
    native_media_library_tag_array_move(tags, &replacement);
    return true;
}

bool
native_media_library_tags_from_songs(
    NativeMediaLibraryTagArray *tags, NcmMpdSongList *songs,
    enum mpd_tag_type primary_tag) {
    NativeMediaLibraryTagArray replacement;

    if ((tags == NULL) || (songs == NULL)
        || (primary_tag == MPD_TAG_UNKNOWN)) {
        return false;
    }

    native_media_library_tag_array_init(&replacement);
    for (int32 i = 0; i < ncm_mpd_song_list_count(songs); i += 1) {
        NcmSong *song;
        NcmStringView primary_value;

        song = ncm_mpd_song_list_at(songs, i);
        if (song == NULL) {
            native_media_library_tag_array_destroy(&replacement);
            return false;
        }
        for (uint32 j = 0;
             ncm_song_tag_view(song, primary_tag, j, &primary_value);
             j += 1) {
            int32 existing;

            existing = native_library_find_tag(
                &replacement, primary_value.data, primary_value.len);
            if (existing >= 0) {
                if (song->last_modified
                    > replacement.items[existing].mtime) {
                    replacement.items[existing].mtime =
                        song->last_modified;
                }
                continue;
            }
            if (!native_library_append_tag(
                    &replacement, primary_value.data,
                    primary_value.len, song->last_modified)) {
                native_media_library_tag_array_destroy(&replacement);
                return false;
            }
        }
    }
    native_library_sort_tags(&replacement);
    native_media_library_tag_array_move(tags, &replacement);
    return true;
}

bool
native_media_library_albums_from_songs(
    NativeMediaLibraryAlbumArray *albums, NcmMpdSongList *songs,
    enum NativeMediaLibraryMode mode, enum mpd_tag_type primary_tag,
    char *selected_tag, int32 selected_tag_len) {
    NativeMediaLibraryAlbumArray replacement;
    int32 album_count;

    if ((albums == NULL) || (songs == NULL)
        || (mode < NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        || (mode >= NATIVE_MEDIA_LIBRARY_MODE_LAST)
        || (primary_tag == MPD_TAG_UNKNOWN)
        || (selected_tag_len < 0)
        || ((selected_tag == NULL) && (selected_tag_len > 0))) {
        return false;
    }

    native_media_library_album_array_init(&replacement);
    for (int32 i = 0; i < ncm_mpd_song_list_count(songs); i += 1) {
        NcmSong *song;
        bool result;

        song = ncm_mpd_song_list_at(songs, i);
        if (song == NULL) {
            native_media_library_album_array_destroy(&replacement);
            return false;
        }
        if (mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
            result = native_library_add_three_column_album(
                &replacement, song, selected_tag, selected_tag_len);
        } else {
            result = native_library_add_two_column_albums(
                &replacement, song, mode, primary_tag);
        }
        if (!result) {
            native_media_library_album_array_destroy(&replacement);
            return false;
        }
    }

    native_library_sort_albums(&replacement);
    album_count = replacement.len;
    if ((mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (album_count > 1)) {
        NativeMediaLibraryAlbumItem *separator;

        separator = native_media_library_album_array_append(&replacement);
        if (separator == NULL) {
            native_media_library_album_array_destroy(&replacement);
            return false;
        }
        separator->menu_flags = NC_MENU_ITEM_SEPARATOR;
        if (!native_library_append_album(
                &replacement, selected_tag, selected_tag_len,
                NULL, 0, NULL, 0, 0, true,
                NC_MENU_ITEM_SELECTABLE)) {
            native_media_library_album_array_destroy(&replacement);
            return false;
        }
    }

    native_media_library_album_array_move(albums, &replacement);
    return true;
}

bool
native_media_library_songs_from_list(
    NcmSongArray *songs, NcmMpdSongList *source) {
    NcmSongArray replacement;

    if ((songs == NULL) || (source == NULL)) {
        return false;
    }

    ncm_song_array_init(&replacement);
    for (int32 i = 0; i < ncm_mpd_song_list_count(source); i += 1) {
        NcmSong *song;

        song = ncm_mpd_song_list_at(source, i);
        if ((song == NULL)
            || !ncm_song_array_append_copy(&replacement, song)) {
            ncm_song_array_destroy(&replacement);
            return false;
        }
    }
    native_library_sort_songs(&replacement);
    ncm_song_array_move(songs, &replacement);
    return true;
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
native_media_library_screen_set_primary_tag_type(
    NativeMediaLibraryScreen *screen, enum mpd_tag_type tag_type) {
    if ((screen == NULL) || (tag_type == MPD_TAG_UNKNOWN)) {
        return false;
    }

    if (Config.media_lib_primary_tag == tag_type) {
        native_library_update_titles(screen, true);
        return true;
    }

    Config.media_lib_primary_tag = tag_type;
    nc_menu_clear_items(nc_media_library_tag_menu_base(&screen->tags));
    nc_menu_clear_items(nc_media_library_album_menu_base(&screen->albums));
    nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));
    native_library_reset_observed_highlights(screen);
    native_library_update_titles(screen, true);
    native_media_library_screen_request_tags_update(screen);
    nc_screen_finish_list_change(&screen->screen);
    return true;
}

void
native_media_library_screen_request_database_update(
    NativeMediaLibraryScreen *screen) {
    native_library_request_all_updates(screen);
    return;
}

bool
native_media_library_screen_refresh_inactive_songs(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (!native_media_library_screen_column_visible(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS)) {
        return false;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        return false;
    }

    native_library_refresh_menu(
        nc_media_library_song_menu_base(&screen->songs),
        &screen->songs_window);
    return true;
}

bool
native_media_library_screen_previous_column_available(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        return native_library_column_has_visible_items(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return native_library_column_has_visible_items(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS);
    }
    return false;
}

bool
native_media_library_screen_next_column_available(
    NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS) {
        return native_library_column_has_visible_items(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return native_library_column_has_visible_items(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
    }
    return false;
}

void
native_media_library_screen_previous_column(
    NativeMediaLibraryScreen *screen) {
    if (!native_media_library_screen_previous_column_available(screen)) {
        return;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        native_media_library_screen_set_active_column(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    } else {
        native_media_library_screen_set_active_column(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS);
    }
    return;
}

void
native_media_library_screen_next_column(NativeMediaLibraryScreen *screen) {
    if (!native_media_library_screen_next_column_available(screen)) {
        return;
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS) {
        native_media_library_screen_set_active_column(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    } else {
        native_media_library_screen_set_active_column(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
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
    native_library_reset_observed_highlights(screen);
    return;
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
native_media_library_screen_selected_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs) {
    NcmError error;
    bool result;

    ncm_error_clear(&error);
    result = native_media_library_screen_selected_songs_checked(
        screen, songs, &error);
    if (!result && ncm_error_is_set(&error)) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time, error.message);
    }
    return result;
}

bool
native_media_library_screen_selected_songs_checked(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error) {
    if ((screen == NULL) || (songs == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing media-library songs"));
        return false;
    }
    return native_library_collect_selected_songs(screen, songs, error);
}

bool
native_media_library_screen_copy_visible_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error) {
    NcMenu *menu;

    if ((screen == NULL) || (songs == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing media-library songs"));
        return false;
    }

    menu = nc_media_library_song_menu_base(&screen->songs);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!native_library_copy_song_at(screen, songs, i)) {
            native_library_set_conversion_error(
                error,
                STRLIT_ARGS("failed to copy media-library songs"));
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
                           Config.regex_type, error)) {
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
    nc_screen_finish_list_change(&screen->screen);
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
    nc_screen_finish_list_change(&screen->screen);
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
            if (nc_menu_goto_selectable(menu, pos)) {
                nc_screen_finish_list_change(&screen->screen);
                return true;
            }
            return false;
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
    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        screen->tags_update_request = true;
    } else {
        screen->albums_update_request = true;
        screen->songs_update_request = true;
    }
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
native_media_library_screen_finish_list_change(
    NativeMediaLibraryScreen *screen) {
    NcMediaLibraryTagRow *tag;
    NcMediaLibraryAlbumRow *album;
    bool tag_valid;
    bool album_valid;
    bool tag_changed;
    bool album_changed;

    if (screen == NULL) {
        return;
    }

    tag = NULL;
    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        tag = native_media_library_screen_current_tag(screen);
    }
    album = native_media_library_screen_current_album(screen);
    tag_valid = tag != NULL;
    album_valid = album != NULL;

    tag_changed = screen->observed_tag_valid != tag_valid;
    if (!tag_changed && tag_valid) {
        tag_changed = !native_library_tag_identity_equal(
            &screen->observed_tag, tag);
    }
    album_changed = screen->observed_album_valid != album_valid;
    if (!album_changed && album_valid) {
        album_changed = !native_library_album_identity_equal(
            &screen->observed_album, album);
    }

    if (tag_changed) {
        nc_menu_clear_items(
            nc_media_library_album_menu_base(&screen->albums));
        nc_menu_clear_items(
            nc_media_library_song_menu_base(&screen->songs));
        native_library_restart_update_timer(screen);
        native_library_set_observed_tag(screen, tag);
        native_library_set_observed_album(screen, NULL);
        nc_screen_request_update(&screen->screen);
        return;
    }

    if (album_changed) {
        nc_menu_clear_items(
            nc_media_library_song_menu_base(&screen->songs));
        native_library_restart_update_timer(screen);
        native_library_set_observed_album(screen, album);
        nc_screen_request_update(&screen->screen);
        return;
    }

    native_library_set_observed_tag(screen, tag);
    native_library_set_observed_album(screen, album);
    return;
}

bool
native_media_library_screen_update(NativeMediaLibraryScreen *screen,
                                   NcmError *error) {
    if (screen == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing media library"));
        return false;
    }
    ncm_error_clear(error);

    if (native_library_tags_pending(screen)) {
        return native_library_update_tags(screen, error);
    }

    if (native_library_albums_pending(screen)
        && ((screen->mode != NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
            || screen->albums_update_request
            || native_library_fetch_delay_elapsed(screen))) {
        return native_library_update_albums(screen, error);
    }

    if (native_library_songs_pending(screen)
        && (screen->songs_update_request
            || native_library_fetch_delay_elapsed(screen))) {
        return native_library_update_songs(screen, error);
    }

    if (!native_library_tags_pending(screen)
        && !native_library_albums_pending(screen)
        && !native_library_songs_pending(screen)) {
        nc_screen_clear_update_request(&screen->screen);
    }
    return true;
}

static bool
native_library_update_tags(NativeMediaLibraryScreen *screen,
                           NcmError *error) {
    NativeMediaLibraryTagArray tags;
    NcmMpdStringList strings;
    NcmMpdSongList songs;
    bool result;

    native_media_library_tag_array_init(&tags);
    ncm_mpd_string_list_init(&strings);
    ncm_mpd_song_list_init(&songs);

    if (screen->sort_by_mtime) {
        result = native_media_library_screen_list_all_songs(
            screen, &songs, error);
        if (!result) {
            screen->tags_update_request = true;
            if (screen->sort_by_mtime) {
                native_media_library_screen_toggle_sort_mode(screen);
            }
            ncm_mpd_song_list_destroy(&songs);
            ncm_mpd_string_list_destroy(&strings);
            native_media_library_tag_array_destroy(&tags);
            return false;
        }
        result = native_media_library_tags_from_songs(
            &tags, &songs, Config.media_lib_primary_tag);
    } else {
        result = native_media_library_screen_list_tags(
            screen, Config.media_lib_primary_tag, &strings, error);
        if (!result) {
            screen->tags_update_request = true;
            ncm_mpd_song_list_destroy(&songs);
            ncm_mpd_string_list_destroy(&strings);
            native_media_library_tag_array_destroy(&tags);
            return false;
        }
        result = native_media_library_tags_from_strings(&tags, &strings);
    }

    if (!result) {
        screen->tags_update_request = true;
        native_library_set_conversion_error(
            error, STRLIT_ARGS("failed to build media-library tags"));
    } else {
        result = native_library_replace_tags(screen, &tags);
        if (!result) {
            screen->tags_update_request = true;
            native_library_set_conversion_error(
                error, STRLIT_ARGS("failed to replace media-library tags"));
        } else {
            screen->tags_update_request = false;
        }
    }

    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_string_list_destroy(&strings);
    native_media_library_tag_array_destroy(&tags);
    return result;
}

static bool
native_library_update_albums(NativeMediaLibraryScreen *screen,
                             NcmError *error) {
    NativeMediaLibraryAlbumArray albums;
    NativeMediaLibrarySongQuery query = {0};
    NcMediaLibraryTagRow *tag;
    NcmMpdSongList songs;
    char *selected_tag;
    int32 selected_tag_len;
    bool result;

    native_media_library_album_array_init(&albums);
    ncm_mpd_song_list_init(&songs);
    selected_tag = NULL;
    selected_tag_len = 0;

    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        tag = native_media_library_screen_current_tag(screen);
        if (tag == NULL) {
            ncm_mpd_song_list_destroy(&songs);
            native_media_library_album_array_destroy(&albums);
            return true;
        }
        selected_tag = tag->tag;
        selected_tag_len = tag->tag_len;
        query.primary_value = tag->tag;
        query.primary_value_len = tag->tag_len;
        query.primary_tag = Config.media_lib_primary_tag;
        query.match_primary_tag = true;
        result = native_media_library_screen_search_songs(
            screen, &query, &songs, error);
    } else {
        result = native_media_library_screen_list_all_songs(
            screen, &songs, error);
        if (!result) {
            native_media_library_screen_toggle_mode(screen);
            native_library_request_all_updates(screen);
        }
    }

    if (!result) {
        screen->albums_update_request = true;
        ncm_mpd_song_list_destroy(&songs);
        native_media_library_album_array_destroy(&albums);
        return false;
    }

    result = native_media_library_albums_from_songs(
        &albums, &songs, screen->mode, Config.media_lib_primary_tag,
        selected_tag, selected_tag_len);
    if (!result) {
        screen->albums_update_request = true;
        native_library_set_conversion_error(
            error, STRLIT_ARGS("failed to build media-library albums"));
    } else {
        result = native_library_replace_albums(screen, &albums);
        if (!result) {
            screen->albums_update_request = true;
            native_library_set_conversion_error(
                error,
                STRLIT_ARGS("failed to replace media-library albums"));
        } else {
            screen->albums_update_request = false;
        }
    }

    ncm_mpd_song_list_destroy(&songs);
    native_media_library_album_array_destroy(&albums);
    return result;
}

static bool
native_library_update_songs(NativeMediaLibraryScreen *screen,
                            NcmError *error) {
    NativeMediaLibrarySongQuery query = {0};
    NcMediaLibraryAlbumRow *album;
    NcmMpdSongList source;
    NcmSongArray songs;
    bool result;

    album = native_media_library_screen_current_album(screen);
    if (album == NULL) {
        return true;
    }

    query.primary_tag = Config.media_lib_primary_tag;
    if (screen->mode != NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
        query.primary_value = album->tag;
        query.primary_value_len = album->tag_len;
        query.match_primary_tag = true;
    }
    if (!album->all_tracks_entry) {
        query.album = album->album;
        query.album_len = album->album_len;
        query.match_album = true;
        if (Config.media_library_albums_split_by_date) {
            query.date = album->date;
            query.date_len = album->date_len;
            query.match_date = true;
        }
    }

    ncm_mpd_song_list_init(&source);
    ncm_song_array_init(&songs);
    result = native_media_library_screen_search_songs(
        screen, &query, &source, error);
    if (!result) {
        screen->songs_update_request = true;
        ncm_song_array_destroy(&songs);
        ncm_mpd_song_list_destroy(&source);
        return false;
    }

    result = native_media_library_songs_from_list(&songs, &source);
    if (!result) {
        screen->songs_update_request = true;
        native_library_set_conversion_error(
            error, STRLIT_ARGS("failed to build media-library songs"));
    } else {
        result = native_library_replace_songs(screen, &songs);
        if (!result) {
            screen->songs_update_request = true;
            native_library_set_conversion_error(
                error, STRLIT_ARGS("failed to replace media-library songs"));
        } else {
            screen->songs_update_request = false;
        }
    }

    ncm_song_array_destroy(&songs);
    ncm_mpd_song_list_destroy(&source);
    return result;
}

static bool
native_library_replace_tags(NativeMediaLibraryScreen *screen,
                            NativeMediaLibraryTagArray *tags) {
    NcMediaLibraryTagMenu replacement;
    NcMediaLibraryTagRow identity;
    NcMediaLibraryTagRow *current;
    NcMenu *menu;
    NcMenu *replacement_menu;
    int64 highlight;
    bool identity_valid;

    if ((screen == NULL) || (tags == NULL)) {
        return false;
    }

    menu = nc_media_library_tag_menu_base(&screen->tags);
    current = nc_media_library_tag_menu_current(&screen->tags);
    highlight = nc_menu_highlight(menu);
    identity_valid = false;
    nc_media_library_tag_row_init(&identity);
    if (current) {
        identity_valid = nc_media_library_tag_row_copy(&identity, current);
        native_library_set_observed_tag(screen, current);
    } else {
        native_library_set_observed_tag(screen, NULL);
    }

    nc_media_library_tag_menu_init(&replacement);
    replacement_menu = nc_media_library_tag_menu_base(&replacement);
    nc_menu_copy(replacement_menu, menu);
    for (int32 i = 0; i < tags->len; i += 1) {
        nc_media_library_tag_menu_add(&replacement, &tags->items[i]);
    }
    native_library_apply_column_filter(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS, replacement_menu);
    native_library_restore_tag_identity(
        &replacement, &identity, identity_valid, highlight);

    nc_menu_swap(menu, replacement_menu);
    nc_media_library_tag_menu_destroy(&replacement);
    nc_media_library_tag_row_destroy(&identity);
    nc_screen_finish_list_change(&screen->screen);
    return true;
}

static bool
native_library_replace_albums(NativeMediaLibraryScreen *screen,
                              NativeMediaLibraryAlbumArray *albums) {
    NcMediaLibraryAlbumMenu replacement;
    NcMediaLibraryAlbumRow identity;
    NcMediaLibraryAlbumRow *current;
    NcMenu *menu;
    NcMenu *replacement_menu;
    int64 highlight;
    bool identity_valid;

    if ((screen == NULL) || (albums == NULL)) {
        return false;
    }

    menu = nc_media_library_album_menu_base(&screen->albums);
    current = nc_media_library_album_menu_current(&screen->albums);
    highlight = nc_menu_highlight(menu);
    identity_valid = false;
    nc_media_library_album_row_init(&identity);
    if (current) {
        identity_valid = nc_media_library_album_row_copy(&identity, current);
        native_library_set_observed_album(screen, current);
    } else {
        native_library_set_observed_album(screen, NULL);
    }

    nc_media_library_album_menu_init(&replacement);
    replacement_menu = nc_media_library_album_menu_base(&replacement);
    nc_menu_copy(replacement_menu, menu);
    for (int32 i = 0; i < albums->len; i += 1) {
        nc_media_library_album_menu_add_with_flags(
            &replacement, &albums->items[i].row,
            albums->items[i].menu_flags);
    }
    native_library_apply_column_filter(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS, replacement_menu);
    native_library_restore_album_identity(
        &replacement, &identity, identity_valid, highlight);

    nc_menu_swap(menu, replacement_menu);
    nc_media_library_album_menu_destroy(&replacement);
    nc_media_library_album_row_destroy(&identity);
    nc_screen_finish_list_change(&screen->screen);
    return true;
}

static bool
native_library_replace_songs(NativeMediaLibraryScreen *screen,
                             NcmSongArray *songs) {
    NcMediaLibrarySongMenu replacement;
    NcmSong identity;
    NcmSong *current;
    NcMenu *menu;
    NcMenu *replacement_menu;
    int64 highlight;
    bool identity_valid;

    if ((screen == NULL) || (songs == NULL)) {
        return false;
    }

    menu = nc_media_library_song_menu_base(&screen->songs);
    current = nc_media_library_song_menu_current(&screen->songs);
    highlight = nc_menu_highlight(menu);
    identity_valid = false;
    ncm_song_init(&identity);
    if (current) {
        identity_valid = ncm_song_copy(&identity, current);
    }

    nc_media_library_song_menu_init(&replacement);
    replacement_menu = nc_media_library_song_menu_base(&replacement);
    nc_menu_copy(replacement_menu, menu);
    for (int32 i = 0; i < songs->len; i += 1) {
        nc_media_library_song_menu_add(&replacement, &songs->items[i]);
    }
    native_library_apply_column_filter(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS, replacement_menu);
    native_library_restore_song_identity(
        &replacement, &identity, identity_valid, highlight);

    nc_menu_swap(menu, replacement_menu);
    nc_media_library_song_menu_destroy(&replacement);
    ncm_song_destroy(&identity);
    return true;
}

static void
native_library_apply_column_filter(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column, NcMenu *menu) {
    NativeMediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;

    state = native_media_library_screen_column_state(screen, column);
    if ((state == NULL) || (menu == NULL)) {
        return;
    }

    callbacks = native_library_display_callbacks(
        screen, column, state->filter_enabled);
    nc_menu_set_display_callbacks(menu, callbacks);
    if (state->filter_enabled) {
        nc_menu_apply_filter(menu);
    } else {
        nc_menu_show_all_items(menu);
    }
    return;
}

static void
native_library_restore_highlight(NcMenu *menu, int64 highlight) {
    int64 count;

    count = nc_menu_item_count(menu);
    if (count <= 0) {
        return;
    }
    if (highlight < 0) {
        highlight = 0;
    }
    if (highlight >= count) {
        highlight = count - 1;
    }
    (void)nc_menu_goto_selectable(menu, highlight);
    return;
}

static void
native_library_restore_tag_identity(
    NcMediaLibraryTagMenu *menu, NcMediaLibraryTagRow *identity,
    bool identity_valid, int64 fallback) {
    NcMenu *base;

    base = nc_media_library_tag_menu_base(menu);
    if (identity_valid) {
        for (int64 i = 0; i < nc_menu_item_count(base); i += 1) {
            NcMediaLibraryTagRow *candidate;

            candidate = nc_media_library_tag_menu_item_at(
                menu, base->active_items, i);
            if (candidate
                && native_library_tag_identity_equal(candidate, identity)
                && nc_menu_goto_selectable(base, i)) {
                return;
            }
        }
    }
    native_library_restore_highlight(base, fallback);
    return;
}

static void
native_library_restore_album_identity(
    NcMediaLibraryAlbumMenu *menu, NcMediaLibraryAlbumRow *identity,
    bool identity_valid, int64 fallback) {
    NcMenu *base;

    base = nc_media_library_album_menu_base(menu);
    if (identity_valid) {
        for (int64 i = 0; i < nc_menu_item_count(base); i += 1) {
            NcMediaLibraryAlbumRow *candidate;

            candidate = nc_media_library_album_menu_item_at(
                menu, base->active_items, i);
            if (candidate
                && native_library_album_identity_equal(candidate, identity)
                && nc_menu_goto_selectable(base, i)) {
                return;
            }
        }
    }
    native_library_restore_highlight(base, fallback);
    return;
}

static void
native_library_restore_song_identity(
    NcMediaLibrarySongMenu *menu, NcmSong *identity,
    bool identity_valid, int64 fallback) {
    NcMenu *base;

    base = nc_media_library_song_menu_base(menu);
    if (identity_valid) {
        for (int64 i = 0; i < nc_menu_item_count(base); i += 1) {
            NcmSong *candidate;

            candidate = nc_media_library_song_menu_item_at(
                menu, base->active_items, i);
            if (candidate && ncm_song_equal(candidate, identity)
                && nc_menu_goto_selectable(base, i)) {
                return;
            }
        }
    }
    native_library_restore_highlight(base, fallback);
    return;
}

static bool
native_library_tag_identity_equal(NcMediaLibraryTagRow *left,
                                  NcMediaLibraryTagRow *right) {
    if ((left == NULL) || (right == NULL)) {
        return left == right;
    }
    return STREQUAL(left->tag, left->tag_len,
                            right->tag, right->tag_len);
}

static bool
native_library_album_identity_equal(NcMediaLibraryAlbumRow *left,
                                    NcMediaLibraryAlbumRow *right) {
    if ((left == NULL) || (right == NULL)) {
        return left == right;
    }
    return left->all_tracks_entry == right->all_tracks_entry
           && STREQUAL(left->tag, left->tag_len,
                               right->tag, right->tag_len)
           && STREQUAL(left->album, left->album_len,
                               right->album, right->album_len)
           && STREQUAL(left->date, left->date_len,
                               right->date, right->date_len);
}

static void
native_library_set_observed_tag(NativeMediaLibraryScreen *screen,
                                NcMediaLibraryTagRow *tag) {
    nc_media_library_tag_row_destroy(&screen->observed_tag);
    nc_media_library_tag_row_init(&screen->observed_tag);
    screen->observed_tag_valid = false;
    if (tag) {
        screen->observed_tag_valid = nc_media_library_tag_row_copy(
            &screen->observed_tag, tag);
    }
    return;
}

static void
native_library_set_observed_album(NativeMediaLibraryScreen *screen,
                                  NcMediaLibraryAlbumRow *album) {
    nc_media_library_album_row_destroy(&screen->observed_album);
    nc_media_library_album_row_init(&screen->observed_album);
    screen->observed_album_valid = false;
    if (album) {
        screen->observed_album_valid = nc_media_library_album_row_copy(
            &screen->observed_album, album);
    }
    return;
}

static void
native_library_reset_observed_highlights(
    NativeMediaLibraryScreen *screen) {
    native_library_set_observed_tag(screen, NULL);
    native_library_set_observed_album(screen, NULL);
    return;
}

static void
native_library_restart_update_timer(NativeMediaLibraryScreen *screen) {
    screen->update_timer = global_timer;
    return;
}

static bool
native_library_tags_pending(NativeMediaLibraryScreen *screen) {
    NcMenu *tags;

    if ((screen == NULL)
        || (screen->mode != NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)) {
        return false;
    }
    tags = nc_media_library_tag_menu_base(&screen->tags);
    return screen->tags_update_request
           || (nc_menu_all_item_count(tags) <= 0);
}

static bool
native_library_albums_pending(NativeMediaLibraryScreen *screen) {
    NcMenu *albums;

    if (screen == NULL) {
        return false;
    }
    if ((screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (native_media_library_screen_current_tag(screen) == NULL)) {
        return false;
    }
    albums = nc_media_library_album_menu_base(&screen->albums);
    return screen->albums_update_request
           || (nc_menu_all_item_count(albums) <= 0);
}

static bool
native_library_songs_pending(NativeMediaLibraryScreen *screen) {
    NcMenu *songs;

    if ((screen == NULL)
        || (native_media_library_screen_current_album(screen) == NULL)) {
        return false;
    }
    songs = nc_media_library_song_menu_base(&screen->songs);
    return screen->songs_update_request
           || (nc_menu_all_item_count(songs) <= 0);
}

static bool
native_library_fetch_delay_elapsed(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return false;
    }
    if (screen->fetching_delay_ms < 0) {
        return true;
    }
    return global_timer_elapsed_ms(screen->update_timer)
           > screen->fetching_delay_ms;
}

static bool
native_library_update_due(NativeMediaLibraryScreen *screen) {
    if (native_library_tags_pending(screen)) {
        return true;
    }
    if (native_library_albums_pending(screen)
        && ((screen->mode != NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
            || screen->albums_update_request
            || native_library_fetch_delay_elapsed(screen))) {
        return true;
    }
    if (native_library_songs_pending(screen)
        && (screen->songs_update_request
            || native_library_fetch_delay_elapsed(screen))) {
        return true;
    }
    return false;
}

static void
native_library_set_conversion_error(NcmError *error, char *message,
                                    int32 message_len) {
    if ((error != NULL) && !ncm_error_is_set(error)) {
        ncm_error_set(error, EINVAL, message, message_len);
    }
    return;
}

static void
native_library_request_all_updates(NativeMediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->tags_update_request = true;
    screen->albums_update_request = true;
    screen->songs_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

static void
native_library_clear_column_filter(NativeMediaLibraryScreen *screen,
                                   enum NativeMediaLibraryColumn column) {
    NativeMediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }

    state = native_media_library_screen_column_state(screen, column);
    menu = native_library_column_menu(screen, column);
    if ((state == NULL) || (menu == NULL)) {
        return;
    }

    ncm_regex_destroy(&state->filter_regex);
    ncm_regex_init(&state->filter_regex);
    ncm_buffer_clear(&state->filter_constraint);
    callbacks = native_library_display_callbacks(screen, column, false);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_show_all_items(menu);
    state->filter_enabled = false;
    return;
}

static void
native_library_sort_tag_menu(NcMediaLibraryTagMenu *menu) {
    NcMenu *base;

    if (menu == NULL) {
        return;
    }

    base = nc_media_library_tag_menu_base(menu);
    for (int64 i = 1; i < nc_menu_all_item_count(base); i += 1) {
        int64 j;

        j = i;
        while (j > 0) {
            NcMediaLibraryTagRow *left;
            NcMediaLibraryTagRow *right;

            left = nc_media_library_tag_menu_item_at(
                menu, NC_MENU_ITEMS_ALL, j);
            right = nc_media_library_tag_menu_item_at(
                menu, NC_MENU_ITEMS_ALL, j - 1);
            if ((left == NULL) || (right == NULL)
                || (native_library_compare_tag_rows(left, right) >= 0)) {
                break;
            }
            nc_menu_swap_item_slots(base, NC_MENU_ITEMS_ALL, j, j - 1);
            j -= 1;
        }
    }
    nc_menu_show_all_items(base);
    return;
}

static void
native_library_sort_album_menu(NcMediaLibraryAlbumMenu *menu) {
    NcMenu *base;

    if (menu == NULL) {
        return;
    }

    base = nc_media_library_album_menu_base(menu);
    for (int64 i = 1; i < nc_menu_all_item_count(base); i += 1) {
        int64 j;

        j = i;
        while (j > 0) {
            NativeMediaLibraryAlbumItem left;
            NativeMediaLibraryAlbumItem right;
            NcMediaLibraryAlbumRow *left_row;
            NcMediaLibraryAlbumRow *right_row;
            int32 left_rank;
            int32 right_rank;

            left_row = nc_media_library_album_menu_item_at(
                menu, NC_MENU_ITEMS_ALL, j);
            right_row = nc_media_library_album_menu_item_at(
                menu, NC_MENU_ITEMS_ALL, j - 1);
            if ((left_row == NULL) || (right_row == NULL)) {
                break;
            }

            left.row = *left_row;
            left.menu_flags = nc_menu_item_flags_at(
                base, NC_MENU_ITEMS_ALL, j);
            right.row = *right_row;
            right.menu_flags = nc_menu_item_flags_at(
                base, NC_MENU_ITEMS_ALL, j - 1);

            left_rank = 0;
            right_rank = 0;
            if (left.menu_flags & NC_MENU_ITEM_SEPARATOR) {
                left_rank = 1;
            }
            if (right.menu_flags & NC_MENU_ITEM_SEPARATOR) {
                right_rank = 1;
            }
            if (left.row.all_tracks_entry) {
                left_rank = 2;
            }
            if (right.row.all_tracks_entry) {
                right_rank = 2;
            }
            if (left_rank > right_rank) {
                break;
            }
            if (left_rank < right_rank) {
                nc_menu_swap_item_slots(base, NC_MENU_ITEMS_ALL, j, j - 1);
                j -= 1;
                continue;
            }
            if (left_rank != 0) {
                break;
            }
            if (native_library_compare_album_items(&left, &right) >= 0) {
                break;
            }
            nc_menu_swap_item_slots(base, NC_MENU_ITEMS_ALL, j, j - 1);
            j -= 1;
        }
    }
    nc_menu_show_all_items(base);
    return;
}

static bool
native_library_move_to_tag(NativeMediaLibraryScreen *screen,
                           char *tag, int32 tag_len) {
    NcMenu *menu;

    if ((screen == NULL) || (tag_len < 0)
        || ((tag == NULL) && (tag_len > 0))) {
        return false;
    }

    menu = nc_media_library_tag_menu_base(&screen->tags);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMediaLibraryTagRow *row;

        row = nc_menu_active_item_at(menu, i);
        if ((row != NULL)
            && STREQUAL(row->tag, row->tag_len, tag, tag_len)) {
            return nc_menu_goto_selectable(menu, i);
        }
    }
    return false;
}

static bool
native_library_move_to_album(NativeMediaLibraryScreen *screen,
                             char *tag, int32 tag_len,
                             char *album, int32 album_len,
                             char *date, int32 date_len,
                             bool consider_date) {
    NcMenu *menu;

    if ((screen == NULL) || (tag_len < 0) || (album_len < 0)
        || (date_len < 0) || ((tag == NULL) && (tag_len > 0))
        || ((album == NULL) && (album_len > 0))
        || ((date == NULL) && (date_len > 0))) {
        return false;
    }

    menu = nc_media_library_album_menu_base(&screen->albums);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMediaLibraryAlbumRow *row;
        bool tag_matches;
        bool date_matches;

        if (nc_menu_position_is_separator(menu, i)) {
            continue;
        }

        row = nc_menu_active_item_at(menu, i);
        if (row == NULL) {
            continue;
        }

        tag_matches = (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY)
                      || (screen->mode
                          == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
                      || STREQUAL(row->tag, row->tag_len,
                                          tag, tag_len);
        if (!tag_matches) {
            continue;
        }
        if (!STREQUAL(row->album, row->album_len,
                              album, album_len)) {
            continue;
        }

        date_matches = !consider_date
                       || !Config.media_library_albums_split_by_date
                       || STREQUAL(row->date, row->date_len,
                                           date, date_len);
        if (date_matches) {
            return nc_menu_goto_selectable(menu, i);
        }
    }

    if (consider_date) {
        return native_library_move_to_album(
            screen, tag, tag_len, album, album_len, date, date_len, false);
    }
    return false;
}

static bool
native_library_move_to_song(NativeMediaLibraryScreen *screen,
                            NcmSong *song) {
    NcMenu *menu;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }

    menu = nc_media_library_song_menu_base(&screen->songs);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmSong *candidate;

        candidate = nc_menu_active_item_at(menu, i);
        if ((candidate != NULL) && ncm_song_equal(candidate, song)) {
            return nc_menu_goto_selectable(menu, i);
        }
    }
    return false;
}

static bool
native_library_insert_locate_tag(NativeMediaLibraryScreen *screen,
                                 char *tag, int32 tag_len, time_t mtime) {
    NcMediaLibraryTagRow row;
    bool result;

    if ((screen == NULL) || (tag_len < 0)
        || ((tag == NULL) && (tag_len > 0))) {
        return false;
    }

    nc_media_library_tag_row_init(&row);
    result = native_library_set_owned_string(&row.tag, &row.tag_len,
                                             &row.tag_cap, tag, tag_len);
    if (result) {
        row.mtime = mtime;
        nc_media_library_tag_menu_add(&screen->tags, &row);
        native_library_sort_tag_menu(&screen->tags);
    }
    nc_media_library_tag_row_destroy(&row);
    return result;
}

static bool
native_library_insert_locate_album(NativeMediaLibraryScreen *screen,
                                   char *tag, int32 tag_len,
                                   char *album, int32 album_len,
                                   char *date, int32 date_len,
                                   time_t mtime) {
    NcMediaLibraryAlbumRow row;
    bool result;

    if ((screen == NULL) || (tag_len < 0) || (album_len < 0)
        || (date_len < 0) || ((tag == NULL) && (tag_len > 0))
        || ((album == NULL) && (album_len > 0))
        || ((date == NULL) && (date_len > 0))) {
        return false;
    }

    nc_media_library_album_row_init(&row);
    result = native_library_set_owned_string(&row.tag, &row.tag_len,
                                             &row.tag_cap, tag, tag_len);
    if (result) {
        result = native_library_set_owned_string(
            &row.album, &row.album_len, &row.album_cap,
            album, album_len);
    }
    if (result) {
        result = native_library_set_owned_string(
            &row.date, &row.date_len, &row.date_cap, date, date_len);
    }
    if (result) {
        row.mtime = mtime;
        row.all_tracks_entry = false;
        nc_media_library_album_menu_add_with_flags(
            &screen->albums, &row, NC_MENU_ITEM_SELECTABLE);
        native_library_sort_album_menu(&screen->albums);
    }
    nc_media_library_album_row_destroy(&row);
    return result;
}

static bool
native_library_locate_song_requirements(NcmSong *song,
                                        NcmStringView *primary_value,
                                        NcmError *error) {
    if ((song == NULL) || (primary_value == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing locate-song argument"));
        return false;
    }
    if (!ncm_song_tag_view(song, Config.media_lib_primary_tag,
                           0, primary_value)
        || (primary_value->len <= 0)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("song has no primary tag"));
        return false;
    }
    if (!ncm_song_is_from_database(song)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("song is not from the database"));
        return false;
    }
    return true;
}

static void
native_library_print_add_status(NativeMediaLibraryScreen *screen,
                                NcmSongArray *songs, bool result) {
    NcMediaLibraryAlbumRow *album;
    NcmBuffer message;
    NcmBuffer rendered;
    char *tag_name;

    if ((screen == NULL) || (songs == NULL)) {
        return;
    }
    if (!result && (songs->len <= 0)) {
        return;
    }

    ncm_buffer_init(&message);
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS) {
        NcMediaLibraryTagRow *tag;

        tag = native_media_library_screen_current_tag(screen);
        tag_name = ncm_tag_type_name(Config.media_lib_primary_tag);
        ncm_buffer_append(&message, STRLIT_ARGS("Songs with "));
        for (int32 i = 0; tag_name[i] != '\0'; i += 1) {
            char ch;

            ch = tag_name[i];
            if ((ch >= 'A') && (ch <= 'Z')) {
                ch = (char)(ch - 'A' + 'a');
            }
            ncm_buffer_append_byte(&message, ch);
        }
        ncm_buffer_append(&message, STRLIT_ARGS(" \""));
        if ((tag != NULL) && (tag->tag != NULL)) {
            ncm_buffer_append(&message, tag->tag, tag->tag_len);
        }
        ncm_buffer_append(&message, STRLIT_ARGS("\" added"));
        ncm_buffer_append(&message, ncm_helpers_with_errors(result),
                          optional_strlen32(
                              ncm_helpers_with_errors(result)));
    } else if (screen->active_column
               == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        album = native_media_library_screen_current_album(screen);
        if ((album != NULL) && album->all_tracks_entry) {
            tag_name = ncm_tag_type_name(Config.media_lib_primary_tag);
            ncm_buffer_append(&message, STRLIT_ARGS("Songs with "));
            for (int32 i = 0; tag_name[i] != '\0'; i += 1) {
                char ch;

                ch = tag_name[i];
                if ((ch >= 'A') && (ch <= 'Z')) {
                    ch = (char)(ch - 'A' + 'a');
                }
                ncm_buffer_append_byte(&message, ch);
            }
            ncm_buffer_append(&message, STRLIT_ARGS(" \""));
            if (album->tag != NULL) {
                ncm_buffer_append(&message, album->tag, album->tag_len);
            }
            ncm_buffer_append(&message, STRLIT_ARGS("\" added"));
        } else {
            ncm_buffer_append(&message,
                              STRLIT_ARGS("Songs from album \""));
            if ((album != NULL) && (album->album != NULL)) {
                ncm_buffer_append(&message, album->album,
                                  album->album_len);
            }
            ncm_buffer_append(&message, STRLIT_ARGS("\" added"));
        }
        ncm_buffer_append(&message, ncm_helpers_with_errors(result),
                          optional_strlen32(
                              ncm_helpers_with_errors(result)));
    } else if (result && (songs->len == 1)) {
        rendered = ncm_format_render_string(&Config.song_status_format,
                                            &songs->items[0]);
        ncm_buffer_append(&message, STRLIT_ARGS("Added to playlist: "));
        ncm_buffer_append(&message, rendered.data, rendered.len);
        ncm_buffer_destroy(&rendered);
    } else if (result) {
        ncm_buffer_append(&message, STRLIT_ARGS("Songs added"));
        ncm_buffer_append(&message, ncm_helpers_with_errors(result),
                          optional_strlen32(
                              ncm_helpers_with_errors(result)));
    } else {
        ncm_buffer_destroy(&message);
        return;
    }

    ncm_statusbar_print((int32)Config.message_delay_time,
                        message.data, message.len);
    ncm_buffer_destroy(&message);
    return;
}

static void
native_library_print_add_error(NcmError *error) {
    if ((error != NULL) && ncm_error_is_set(error)) {
        ncm_statusbar_print_cstring(
            (int32)Config.message_delay_time, error->message);
    }
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
    result = native_library_collect_current_item_songs(screen, &songs, error);
    if (result && (songs.len <= 0)) {
        ncm_error_set(error, ENOENT, STRLIT_ARGS("no selected songs"));
        result = false;
    }
    if (result) {
        result = native_media_library_screen_add_songs(screen, &songs,
                                                       play, error);
    }
    native_library_print_add_status(screen, &songs, result);
    ncm_song_array_destroy(&songs);
    return result;
}

bool
native_media_library_screen_locate_song(NativeMediaLibraryScreen *screen,
                                        NcmSong *song, NcmError *error) {
    NcmStringView primary_value;
    NcmStringView album;
    NcmStringView date;
    char *album_date;
    char *album_tag;
    int32 album_date_len;
    int32 album_tag_len;
    NcMenu *tags_menu;
    NcMenu *albums_menu;
    NcMenu *songs_menu;

    if ((screen == NULL) || (song == NULL)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing locate-song argument"));
        return false;
    }
    ncm_error_clear(error);
    if (!native_library_locate_song_requirements(
            song, &primary_value, error)) {
        return false;
    }

    native_library_song_first_tag(song, MPD_TAG_ALBUM, &album);
    native_library_song_first_tag(song, MPD_TAG_DATE, &date);
    album_date = date.data;
    album_date_len = date.len;
    if (!Config.media_library_albums_split_by_date) {
        album_date = NULL;
        album_date_len = 0;
    }

    tags_menu = nc_media_library_tag_menu_base(&screen->tags);
    albums_menu = nc_media_library_album_menu_base(&screen->albums);
    songs_menu = nc_media_library_song_menu_base(&screen->songs);
    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        native_library_clear_column_filter(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS);
        if (nc_menu_empty(tags_menu)) {
            native_media_library_screen_request_tags_update(screen);
            if (!native_media_library_screen_update(screen, error)) {
                return false;
            }
        }
        if (!native_library_move_to_tag(screen, primary_value.data,
                                        primary_value.len)) {
            if (!native_library_insert_locate_tag(
                    screen, primary_value.data, primary_value.len,
                    song->last_modified)) {
                ncm_error_set(error, ENOMEM,
                              STRLIT_ARGS("failed to insert locate tag"));
                return false;
            }
            if (!native_library_move_to_tag(screen, primary_value.data,
                                            primary_value.len)) {
                ncm_error_set(error, ENOENT,
                              STRLIT_ARGS("song tag is not in library"));
                return false;
            }
        }
        nc_screen_finish_list_change(&screen->screen);
    }

    native_library_clear_column_filter(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    if (nc_menu_empty(albums_menu)) {
        native_media_library_screen_request_albums_update(screen);
        if (!native_media_library_screen_update(screen, error)) {
            return false;
        }
    }

    if ((screen->mode == NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && nc_menu_empty(albums_menu)) {
        native_media_library_screen_set_active_column(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS);
        ncm_error_set(error, ENOENT,
                      STRLIT_ARGS("song album is not in library"));
        return false;
    }

    album_tag = primary_value.data;
    album_tag_len = primary_value.len;
    if (screen->mode == NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
        album_tag = NULL;
        album_tag_len = 0;
    }

    if (!native_library_move_to_album(
            screen, primary_value.data, primary_value.len,
            album.data, album.len, date.data, date.len, true)) {
        if (!native_library_insert_locate_album(
                screen, album_tag, album_tag_len, album.data, album.len,
                album_date, album_date_len, song->last_modified)) {
            ncm_error_set(error, ENOMEM,
                          STRLIT_ARGS("failed to insert locate album"));
            return false;
        }
        if (!native_library_move_to_album(
                screen, primary_value.data, primary_value.len,
                album.data, album.len, date.data, date.len, true)) {
            ncm_error_set(error, ENOENT,
                          STRLIT_ARGS("song album is not in library"));
            return false;
        }
    }
    native_media_library_screen_set_active_column(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    nc_screen_finish_list_change(&screen->screen);

    native_library_clear_column_filter(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
    native_media_library_screen_request_songs_update(screen);
    if (!native_media_library_screen_update(screen, error)) {
        return false;
    }
    if (nc_menu_empty(songs_menu)) {
        nc_menu_clear_items(albums_menu);
        native_media_library_screen_set_active_column(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
        ncm_error_set(error, ENOENT,
                      STRLIT_ARGS("song is not visible in media library"));
        return false;
    }
    if (!native_library_move_to_song(screen, song)) {
        native_media_library_screen_set_active_column(
            screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
        ncm_error_set(error, ENOENT,
                      STRLIT_ARGS("song is not visible in media library"));
        return false;
    }

    native_media_library_screen_set_active_column(
        screen, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
    ncm_error_clear(error);
    return true;
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
native_library_finish_list_change(NcScreen *screen) {
    native_media_library_screen_finish_list_change(
        native_library_from_screen(screen));
    return;
}

static void
native_library_mouse_button_pressed(NcScreen *screen,
                                    MEVENT event) {
    NativeMediaLibraryScreen *library;
    int32 x;
    int32 y;

    library = native_library_from_screen(screen);
    if (library == NULL) {
        return;
    }

    x = event.x;
    y = event.y;
    if (native_media_library_screen_column_visible(
            library, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS)
        && nc_window_has_coords(&library->tags_window, &x, &y)) {
        native_media_library_screen_set_active_column(
            library, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS);
        if (!(((event.bstate
                 & (BUTTON1_PRESSED | BUTTON3_PRESSED)) != 0)
              && native_library_mouse_select(
                  library, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS,
                  nc_media_library_tag_menu_base(&library->tags), y,
                  (event.bstate & BUTTON3_PRESSED) != 0))) {
            if (event.bstate & BUTTON5_PRESSED) {
                native_library_mouse_scroll(library, NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                native_library_mouse_scroll(library, NC_SCROLL_UP);
            }
        }
        nc_screen_finish_list_change(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&library->albums_window, &x, &y)) {
        native_media_library_screen_set_active_column(
            library, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
        if (!(((event.bstate
                 & (BUTTON1_PRESSED | BUTTON3_PRESSED)) != 0)
              && native_library_mouse_select(
                  library, NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS,
                  nc_media_library_album_menu_base(&library->albums), y,
                  (event.bstate & BUTTON3_PRESSED) != 0))) {
            if (event.bstate & BUTTON5_PRESSED) {
                native_library_mouse_scroll(library, NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                native_library_mouse_scroll(library, NC_SCROLL_UP);
            }
        }
        nc_screen_finish_list_change(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&library->songs_window, &x, &y)) {
        native_media_library_screen_set_active_column(
            library, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
        if (!(((event.bstate
                 & (BUTTON1_PRESSED | BUTTON3_PRESSED)) != 0)
              && native_library_mouse_select(
                  library, NATIVE_MEDIA_LIBRARY_COLUMN_SONGS,
                  nc_media_library_song_menu_base(&library->songs), y,
                  (event.bstate & BUTTON3_PRESSED) != 0))) {
            if (event.bstate & BUTTON5_PRESSED) {
                native_library_mouse_scroll(library, NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                native_library_mouse_scroll(library, NC_SCROLL_UP);
            }
        }
        nc_screen_finish_list_change(screen);
    }
    return;
}

static void
native_library_mouse_scroll(NativeMediaLibraryScreen *screen,
                            enum NcScroll where) {
    enum NcScroll effective;
    int64 count;

    effective = where;
    count = Config.lines_scrolled;
    if (Config.mouse_list_scroll_whole_page) {
        if (where == NC_SCROLL_DOWN) {
            effective = NC_SCROLL_PAGE_DOWN;
        } else if (where == NC_SCROLL_UP) {
            effective = NC_SCROLL_PAGE_UP;
        }
        count = 1;
    }

    for (int64 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(
            native_media_library_screen_active_menu(screen),
            screen->main_height, effective);
    }
    return;
}

static bool
native_library_mouse_select(
    NativeMediaLibraryScreen *screen, enum NativeMediaLibraryColumn column,
    NcMenu *menu, int32 y, bool right_click) {
    NcmError error;
    bool play;

    if ((menu == NULL)
        || (y < 0)
        || (y >= nc_menu_item_count(menu))) {
        return false;
    }
    if (!nc_menu_goto_selectable(menu, y)) {
        return false;
    }
    if (right_click) {
        ncm_error_clear(&error);
        play = screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS;
        if (!native_media_library_screen_add_item_to_playlist(
                screen, play, &error)) {
            native_library_print_add_error(&error);
        }
    }

    if (column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS) {
        nc_menu_clear_items(
            nc_media_library_album_menu_base(&screen->albums));
        nc_menu_clear_items(
            nc_media_library_song_menu_base(&screen->songs));
        screen->albums_update_request = true;
        screen->songs_update_request = true;
        native_library_set_observed_album(screen, NULL);
        native_library_restart_update_timer(screen);
        nc_screen_request_update(&screen->screen);
    } else if (column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        nc_menu_clear_items(
            nc_media_library_song_menu_base(&screen->songs));
        screen->songs_update_request = true;
        native_library_restart_update_timer(screen);
        nc_screen_request_update(&screen->screen);
    }
    return true;
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
    if (native_library_albums_pending(library)
        || native_library_songs_pending(library)) {
        return library->window_timeout_ms;
    }
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
native_library_title(NcScreen *screen) {
    (void)screen;
    return "Media library";
}

static void
native_library_update(NcScreen *screen) {
    NativeMediaLibraryScreen *library;
    NcmError error;
    bool update_due;

    library = native_library_from_screen(screen);
    update_due = native_library_update_due(library);
    ncm_error_clear(&error);
    if (!native_media_library_screen_update(library, &error)) {
        if (ncm_error_is_set(&error)) {
            ncm_statusbar_print_cstring(
                (int32)Config.message_delay_time, error.message);
        }
        return;
    }
    if (update_due) {
        native_library_refresh(screen);
    }
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
        screen, item,
        &screen->column_state[NATIVE_MEDIA_LIBRARY_COLUMN_TAGS]
             .filter_regex);
}

static bool
native_library_album_filter(NcMenu *menu, void *item, void *user) {
    NativeMediaLibraryScreen *screen;
    NcMediaLibraryAlbumRow *row;

    screen = user;
    row = item;
    if (native_library_menu_item_is_separator(menu, item)
        || ((row != NULL) && row->all_tracks_entry)) {
        return true;
    }
    return native_library_album_matches(
        screen, row,
        &screen->column_state[NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS]
             .filter_regex);
}

static bool
native_library_song_filter(NcMenu *menu, void *item, void *user) {
    NativeMediaLibraryScreen *screen;

    (void)menu;
    screen = user;
    return native_library_song_matches(
        screen, item,
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


static NcMenu *
native_library_column_menu(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column) {
    if (screen == NULL) {
        return NULL;
    }
    switch (column) {
    case NATIVE_MEDIA_LIBRARY_COLUMN_TAGS:
        return nc_media_library_tag_menu_base(&screen->tags);
    case NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS:
        return nc_media_library_album_menu_base(&screen->albums);
    case NATIVE_MEDIA_LIBRARY_COLUMN_SONGS:
        return nc_media_library_song_menu_base(&screen->songs);
    default:
        return NULL;
    }
}

static bool
native_library_column_has_visible_items(
    NativeMediaLibraryScreen *screen,
    enum NativeMediaLibraryColumn column) {
    NcMenu *menu;

    if (!native_media_library_screen_column_visible(screen, column)) {
        return false;
    }
    menu = native_library_column_menu(screen, column);
    if (menu == NULL) {
        return false;
    }
    return nc_menu_all_item_count(menu) > 0;
}

static bool
native_library_menu_item_is_separator(NcMenu *menu, void *item) {
    if ((menu == NULL) || (item == NULL)) {
        return false;
    }
    for (int64 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        if ((nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i) == item)
            && (nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i)
                & NC_MENU_ITEM_SEPARATOR)) {
            return true;
        }
    }
    return false;
}

static void
native_library_query_from_tag(NativeMediaLibraryScreen *screen,
                              NcMediaLibraryTagRow *tag,
                              NativeMediaLibrarySongQuery *query) {
    (void)screen;
    query->primary_tag = Config.media_lib_primary_tag;
    if (tag != NULL) {
        query->primary_value = tag->tag;
        query->primary_value_len = tag->tag_len;
        query->match_primary_tag = true;
    }
    return;
}

static void
native_library_query_from_album(NativeMediaLibraryScreen *screen,
                                NcMediaLibraryAlbumRow *album,
                                NativeMediaLibrarySongQuery *query) {
    if ((screen == NULL) || (album == NULL)) {
        return;
    }
    query->primary_tag = Config.media_lib_primary_tag;
    if (screen->mode != NATIVE_MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
        query->primary_value = album->tag;
        query->primary_value_len = album->tag_len;
        query->match_primary_tag = true;
    }
    if (!album->all_tracks_entry) {
        query->album = album->album;
        query->album_len = album->album_len;
        query->match_album = true;
        if (Config.media_library_albums_split_by_date) {
            query->date = album->date;
            query->date_len = album->date_len;
            query->match_date = true;
        }
    }
    return;
}

static bool
native_library_append_query_songs(
    NativeMediaLibraryScreen *screen, NativeMediaLibrarySongQuery *query,
    NcmSongArray *songs, NcmError *error) {
    NcmMpdSongList source;
    NcmSongArray sorted;
    bool result;

    ncm_mpd_song_list_init(&source);
    ncm_song_array_init(&sorted);
    result = native_media_library_screen_search_songs(
        screen, query, &source, error);
    if (result) {
        result = native_media_library_songs_from_list(&sorted, &source);
        if (!result) {
            native_library_set_conversion_error(
                error, STRLIT_ARGS("failed to build selected songs"));
        }
    }
    if (result) {
        for (int32 i = 0; i < sorted.len; i += 1) {
            result = ncm_song_array_append_copy(songs, &sorted.items[i]);
            if (!result) {
                native_library_set_conversion_error(
                    error, STRLIT_ARGS("failed to copy selected songs"));
                break;
            }
        }
    }
    ncm_song_array_destroy(&sorted);
    ncm_mpd_song_list_destroy(&source);
    return result;
}

static bool
native_library_collect_tag_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error) {
    NativeMediaLibrarySongQuery query;
    NcMediaLibraryTagRow *row;
    NcMenu *menu;
    bool any_selected;

    menu = nc_media_library_tag_menu_base(&screen->tags);
    any_selected = nc_menu_has_selected(menu);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!any_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }
        row = nc_menu_active_item_at(menu, i);
        if (row == NULL) {
            continue;
        }
        query = (NativeMediaLibrarySongQuery){0};
        native_library_query_from_tag(screen, row, &query);
        if (!native_library_append_query_songs(
                screen, &query, songs, error)) {
            return false;
        }
    }
    return true;
}

static bool
native_library_collect_album_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error) {
    NativeMediaLibrarySongQuery query;
    NcMediaLibraryAlbumRow *row;
    NcMenu *menu;
    bool any_selected;

    menu = nc_media_library_album_menu_base(&screen->albums);
    any_selected = nc_menu_has_selected(menu);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (nc_menu_position_is_separator(menu, i)) {
            continue;
        }
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!any_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }
        row = nc_menu_active_item_at(menu, i);
        if (row == NULL) {
            continue;
        }
        query = (NativeMediaLibrarySongQuery){0};
        native_library_query_from_album(screen, row, &query);
        if (!native_library_append_query_songs(
                screen, &query, songs, error)) {
            return false;
        }
    }
    return true;
}

static bool
native_library_collect_visible_song_rows(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs) {
    NcMenu *menu;
    bool any_selected;

    menu = nc_media_library_song_menu_base(&screen->songs);
    any_selected = nc_menu_has_selected(menu);
    for (int64 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!any_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }
        if (!native_library_copy_song_at(screen, songs, i)) {
            return false;
        }
    }
    return true;
}

static bool
native_library_collect_selected_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error) {
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS) {
        return native_library_collect_tag_songs(screen, songs, error);
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return native_library_collect_album_songs(screen, songs, error);
    }
    return native_library_collect_visible_song_rows(screen, songs);
}

static bool
native_library_collect_current_item_songs(
    NativeMediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *error) {
    NativeMediaLibrarySongQuery query = {0};
    NcMediaLibraryTagRow *tag;
    NcMediaLibraryAlbumRow *album;
    NcMenu *menu;

    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_TAGS) {
        tag = native_media_library_screen_current_tag(screen);
        if (tag == NULL) {
            return true;
        }
        native_library_query_from_tag(screen, tag, &query);
        return native_library_append_query_songs(screen, &query, songs,
                                                 error);
    }

    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        album = native_media_library_screen_current_album(screen);
        if (album == NULL) {
            return true;
        }
        native_library_query_from_album(screen, album, &query);
        return native_library_append_query_songs(screen, &query, songs,
                                                 error);
    }

    menu = nc_media_library_song_menu_base(&screen->songs);
    return native_library_copy_song_at(screen, songs, nc_menu_highlight(menu));
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
        tag_type_name_len = optional_strlen32(tag_type_name);
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
    void *item;

    item = nc_menu_active_item_at(menu, pos);
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS) {
        NcMediaLibraryAlbumRow *album;

        album = item;
        if (nc_menu_position_is_separator(menu, pos)
            || ((album != NULL) && album->all_tracks_entry)) {
            return false;
        }
        return native_library_album_matches(screen, album, regex);
    }
    if (screen->active_column == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS) {
        return native_library_song_matches(screen, item, regex);
    }
    return native_library_tag_matches(screen, item, regex);
}

static bool
native_library_tag_matches(NativeMediaLibraryScreen *screen,
                           NcMediaLibraryTagRow *row, NcmRegex *regex) {
    NcmBuffer text;
    bool result;

    if (row == NULL) {
        return false;
    }
    ncm_buffer_init(&text);
    native_media_library_screen_format_tag_row(screen, row, &text);
    result = ncm_regex_search(regex, text.data, text.len);
    ncm_buffer_destroy(&text);
    return result;
}

static bool
native_library_album_matches(NativeMediaLibraryScreen *screen,
                             NcMediaLibraryAlbumRow *row,
                             NcmRegex *regex) {
    NcmBuffer text;
    bool result;

    if (row == NULL) {
        return false;
    }
    ncm_buffer_init(&text);
    native_media_library_screen_format_album_row(screen, row, &text);
    result = ncm_regex_search(regex, text.data, text.len);
    ncm_buffer_destroy(&text);
    return result;
}

static bool
native_library_song_matches(NativeMediaLibraryScreen *screen,
                            NcmSong *song, NcmRegex *regex) {
    NcmBuffer text;
    bool result;

    (void)screen;
    if (song == NULL) {
        return false;
    }
    text = ncm_format_render_string(&Config.song_library_format, song);
    result = ncm_regex_search(regex, text.data, text.len);
    ncm_buffer_destroy(&text);
    return result;
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
    copy = malloc2(cap);
    memcpy64(copy, source, source_len);
    copy[source_len] = '\0';
    *dest = copy;
    *dest_len = source_len;
    *dest_cap = cap;
    return true;
}

static void
native_library_free_owned_string(char **data, int32 *len, int32 *cap) {
    if (*data) {
        free2(*data, *cap);
    }
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}

static char *
native_library_query_cstring(NcmBuffer *buffer, char *string,
                             int32 string_len) {
    if (buffer == NULL) {
        return NULL;
    }
    if (string_len < 0) {
        if (string == NULL) {
            return NULL;
        }
        string_len = optional_strlen32(string);
    }
    if ((string == NULL) && (string_len > 0)) {
        return NULL;
    }

    ncm_buffer_clear(buffer);
    if (string_len > 0) {
        ncm_buffer_append(buffer, string, string_len);
    }
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
    return ncm_mpd_client_get_directory_recursive(client, "/",
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
