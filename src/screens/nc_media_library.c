#if !defined(NCMPCPP_NC_MEDIA_LIBRARY_C)
#define NCMPCPP_NC_MEDIA_LIBRARY_C

#include "cbase.h"

#include "c/ncm_c.h"
#include "global.h"
#include "helpers.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

static NcWindow *library_active_window(NcScreen *screen);
static void library_refresh(NcScreen *screen);
static void library_refresh_window(NcScreen *screen);
static void library_scroll(NcScreen *screen, enum NcScroll where);
static void library_finish_list_change(NcScreen *screen);
static void library_mouse_button_pressed(NcScreen *screen,
                                         MEVENT event);
static void library_switch_to(NcScreen *screen);
static void library_resize(NcScreen *screen);
static int32 library_window_timeout(NcScreen *screen);
static char *library_title(NcScreen *screen);
static void library_update(NcScreen *screen);
static void library_destroy_callback(NcScreen *screen);
static void library_print_buffer(NcWindow *window, NcBuffer *buffer);
static bool library_copy_song_at(MediaLibraryScreen *screen,
                                 NcmSongArray *songs, int32 pos);
static NcMenu *library_column_menu(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column);
static bool library_column_has_visible_items(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column);
static bool library_menu_item_is_separator(NcMenu *menu, void *item);
static bool library_tag_matches(MediaLibraryScreen *screen,
                                NcMediaLibraryTagRow *row,
                                NcmRegex *regex);
static bool library_album_matches(MediaLibraryScreen *screen,
                                  NcMediaLibraryAlbumRow *row,
                                  NcmRegex *regex);
static bool library_song_matches(MediaLibraryScreen *screen,
                                 NcmSong *song, NcmRegex *regex);
static bool library_collect_selected_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error);
static bool library_collect_current_item_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error);
static void library_mouse_scroll(MediaLibraryScreen *screen,
                                 enum NcScroll where);
static bool library_mouse_select(
    MediaLibraryScreen *screen, enum MediaLibraryColumn column,
    NcMenu *menu, int32 y, bool right_click);
static MediaLibraryColumnState *library_active_column_state(
    MediaLibraryScreen *screen);
static NcMenuDisplayCallbacks library_display_callbacks(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column, bool filter_enabled);
static void library_update_menu_highlights(
    MediaLibraryScreen *screen);
static void library_update_titles(MediaLibraryScreen *screen,
                                  bool update_windows);
static void library_refresh_menu(NcMenu *menu, NcWindow *window);
static bool library_active_item_matches(
    MediaLibraryScreen *screen, NcMenu *menu, int32 pos,
    NcmRegex *regex);
static bool library_search_position(NcMenu *menu, int32 pos,
                                    void *user);
static void library_layout(MediaLibraryScreen *screen);
static int32 library_ratio_value(NcmInt32Array *ratios,
                                 int32 idx, int32 fallback);
static bool library_set_owned_string(char **dest, int32 *dest_len,
                                     int32 *dest_cap, char *source,
                                     int32 source_len);
static void library_free_owned_string(char **data, int32 *len,
                                      int32 *cap);
static bool library_mpd_list_tags(void *user,
                                  enum mpd_tag_type tag_type,
                                  NcmMpdStringList *tags,
                                  NcmError *ncm_error);
static bool library_mpd_list_all_songs(void *user,
                                       NcmMpdSongList *songs,
                                       NcmError *ncm_error);
static bool library_mpd_search_songs(
    void *user, MediaLibrarySongQuery *query,
    NcmMpdSongList *songs, NcmError *ncm_error);
static bool library_mpd_add_songs(void *user,
                                  NcmSongArray *songs,
                                  bool play, NcmError *ncm_error);
static void library_tag_array_item_destroy(void *item);
static void library_album_array_item_init(void *item);
static void library_album_array_item_destroy(void *item);
static bool library_update_tags(
    MediaLibraryScreen *screen, NcmError *ncm_error);
static bool library_update_albums(
    MediaLibraryScreen *screen, NcmError *ncm_error);
static bool library_update_songs(
    MediaLibraryScreen *screen, NcmError *ncm_error);
static bool library_replace_tags(
    MediaLibraryScreen *screen, MediaLibraryTagArray *tags);
static bool library_replace_albums(
    MediaLibraryScreen *screen, MediaLibraryAlbumArray *albums);
static bool library_replace_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs);
static void library_apply_column_filter(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column, NcMenu *menu);
static void library_restore_tag_identity(
    NcMediaLibraryTagMenu *menu, NcMediaLibraryTagRow *identity,
    bool identity_valid, int32 fallback);
static void library_restore_album_identity(
    NcMediaLibraryAlbumMenu *menu, NcMediaLibraryAlbumRow *identity,
    bool identity_valid, int32 fallback);
static void library_restore_song_identity(
    NcMediaLibrarySongMenu *menu, NcmSong *identity,
    bool identity_valid, int32 fallback);
static bool library_tag_identity_equal(
    NcMediaLibraryTagRow *left, NcMediaLibraryTagRow *right);
static bool library_album_identity_equal(
    NcMediaLibraryAlbumRow *left, NcMediaLibraryAlbumRow *right);
static void library_set_observed_tag(
    MediaLibraryScreen *screen, NcMediaLibraryTagRow *tag);
static void library_set_observed_album(
    MediaLibraryScreen *screen, NcMediaLibraryAlbumRow *album);
static void library_reset_observed_highlights(
    MediaLibraryScreen *screen);
static void library_restart_update_timer(
    MediaLibraryScreen *screen);
static bool library_tags_pending(
    MediaLibraryScreen *screen);
static bool library_albums_pending(
    MediaLibraryScreen *screen);
static bool library_songs_pending(
    MediaLibraryScreen *screen);
static bool library_fetch_delay_elapsed(
    MediaLibraryScreen *screen);
static void library_set_conversion_error(NcmError *ncm_error,
                                         char *message,
                                         int32 message_len);
static void library_request_all_updates(
    MediaLibraryScreen *screen);

typedef struct MediaLibrarySearchContext {
    MediaLibraryScreen *screen;
    NcmRegex *regex;
} MediaLibrarySearchContext;

static NcScreenOps library_callbacks = {
    .active_window = library_active_window,
    .refresh = library_refresh,
    .refresh_window = library_refresh_window,
    .scroll = library_scroll,
    .list_change_finished = library_finish_list_change,
    .switch_to = library_switch_to,
    .resize = library_resize,
    .window_timeout_callback = library_window_timeout,
    .title = library_title,
    .update = library_update,
    .mouse_button_pressed = library_mouse_button_pressed,
    .lockable = true,
    .mergable = true,
    .destroy = library_destroy_callback,
};

static NcmArrayItemCallbacks library_tag_array_callbacks = {
    .destroy = library_tag_array_item_destroy,
};

static NcmArrayItemCallbacks library_album_array_callbacks = {
    .init = library_album_array_item_init,
    .destroy = library_album_array_item_destroy,
};

NCM_ARRAY_DEFINE_CLEAR(media_library_tag_array,
                       MediaLibraryTagArray,
                       &library_tag_array_callbacks)
NCM_ARRAY_DEFINE_DESTROY(media_library_tag_array,
                         MediaLibraryTagArray)
NCM_ARRAY_DEFINE_MOVE(media_library_tag_array,
                      MediaLibraryTagArray)
NCM_ARRAY_DEFINE_RESERVE(media_library_tag_array,
                         MediaLibraryTagArray)
NCM_ARRAY_DEFINE_APPEND(media_library_tag_array,
                        MediaLibraryTagArray,
                        NcMediaLibraryTagRow,
                        &library_tag_array_callbacks)
NCM_ARRAY_DEFINE_REMOVE_ORDERED(media_library_tag_array,
                                MediaLibraryTagArray,
                                &library_tag_array_callbacks)

NCM_ARRAY_DEFINE_CLEAR(media_library_album_array,
                       MediaLibraryAlbumArray,
                       &library_album_array_callbacks)
NCM_ARRAY_DEFINE_DESTROY(media_library_album_array,
                         MediaLibraryAlbumArray)
NCM_ARRAY_DEFINE_MOVE(media_library_album_array,
                      MediaLibraryAlbumArray)
NCM_ARRAY_DEFINE_RESERVE(media_library_album_array,
                         MediaLibraryAlbumArray)
NCM_ARRAY_DEFINE_APPEND(media_library_album_array,
                        MediaLibraryAlbumArray,
                        MediaLibraryAlbumItem,
                        &library_album_array_callbacks)
NCM_ARRAY_DEFINE_REMOVE_ORDERED(media_library_album_array,
                                MediaLibraryAlbumArray,
                                &library_album_array_callbacks)

MediaLibraryHooks
media_library_mpd_hooks(NcmMpdClient *client) {
    MediaLibraryHooks hooks = {0};

    hooks.list_tags = library_mpd_list_tags;
    hooks.list_all_songs = library_mpd_list_all_songs;
    hooks.search_songs = library_mpd_search_songs;
    hooks.add_songs = library_mpd_add_songs;
    hooks.user = client;
    return hooks;
}

void
media_library_screen_init(MediaLibraryScreen *screen,
                          MediaLibraryHooks hooks,
                          int32 start_x, int32 width,
                          int32 main_start_y, int32 main_height,
                          NcColor color, NcBorder border) {
    NcMenuDisplayCallbacks callbacks;

    nc_media_library_tag_menu_init(&screen->tags);
    nc_media_library_album_menu_init(&screen->albums);
    nc_media_library_song_menu_init(&screen->songs);
    screen->hooks = hooks;

    for (uint32 i = 0; i < MEDIA_LIBRARY_COLUMN_COUNT; i += 1) {
        screen->column_state[i].filter_constraint = (StrBuilder){0};
        screen->column_state[i].search_constraint = (StrBuilder){0};
        screen->column_state[i].filter_regex = (NcmRegex){0};
        screen->column_state[i].search_regex = (NcmRegex){0};
        screen->column_state[i].filter_enabled = false;
        screen->column_state[i].search_enabled = false;
    }
    screen->tags_title = (StrBuilder){0};
    screen->albums_title = (StrBuilder){0};
    screen->songs_title = (StrBuilder){0};
    screen->observed_tag = (NcMediaLibraryTagRow){0};
    screen->observed_album = (NcMediaLibraryAlbumRow){0};

    screen->update_timer.ns = 0;
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    if (Config.data_fetching_delay) {
        screen->fetching_delay_ms = MEDIA_LIBRARY_FETCH_DELAY_MS;
        screen->window_timeout_ms = MEDIA_LIBRARY_FETCH_DELAY_MS;
    } else {
        screen->fetching_delay_ms = -1;
        screen->window_timeout_ms = NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }

    screen->mode = MEDIA_LIBRARY_MODE_THREE_COLUMNS;
    screen->active_column = MEDIA_LIBRARY_COLUMN_TAGS;
    screen->tags_update_request = false;
    screen->albums_update_request = false;
    screen->songs_update_request = false;
    screen->sort_by_mtime = Config.media_library_sort_by_mtime;
    screen->observed_tag_valid = false;
    screen->observed_album_valid = false;
    screen->registered = false;

    library_update_titles(screen, false);
    nc_window_init(&screen->tags_window, start_x, main_start_y, width,
                   main_height, screen->tags_title.data,
                   screen->tags_title.len, color, border);
    nc_window_init(&screen->albums_window, start_x, main_start_y, width,
                   main_height, screen->albums_title.data,
                   screen->albums_title.len, color, border);
    nc_window_init(&screen->songs_window, start_x, main_start_y, width,
                   main_height, screen->songs_title.data,
                   screen->songs_title.len, color, border);

    callbacks = library_display_callbacks(
        screen, MEDIA_LIBRARY_COLUMN_TAGS, false);
    nc_menu_set_display_callbacks(
        nc_media_library_tag_menu_base(&screen->tags), callbacks);
    callbacks = library_display_callbacks(
        screen, MEDIA_LIBRARY_COLUMN_ALBUMS, false);
    nc_menu_set_display_callbacks(
        nc_media_library_album_menu_base(&screen->albums), callbacks);
    callbacks = library_display_callbacks(
        screen, MEDIA_LIBRARY_COLUMN_SONGS, false);
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

    nc_screen_init_ops(&screen->screen, library_callbacks, screen,
                       NC_SCREEN_TYPE_MEDIA_LIBRARY);
    library_update_menu_highlights(screen);
    library_layout(screen);
    return;
}

void
media_library_screen_destroy(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }

    for (uint32 i = 0; i < MEDIA_LIBRARY_COLUMN_COUNT; i += 1) {
        ncm_regex_destroy(&screen->column_state[i].search_regex);
        ncm_regex_destroy(&screen->column_state[i].filter_regex);
        sb_free(&screen->column_state[i].search_constraint);
        sb_free(&screen->column_state[i].filter_constraint);
    }
    sb_free(&screen->songs_title);
    sb_free(&screen->albums_title);
    sb_free(&screen->tags_title);
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
    screen->hooks = (MediaLibraryHooks){0};
    return;
}

NcScreen *
media_library_screen_base(MediaLibraryScreen *screen) {
    return &screen->screen;
}

NcMenu *
media_library_screen_active_menu(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return nc_media_library_album_menu_base(&screen->albums);
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS) {
        return nc_media_library_song_menu_base(&screen->songs);
    }
    return nc_media_library_tag_menu_base(&screen->tags);
}

NcWindow *
media_library_screen_active_window(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return &screen->albums_window;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS) {
        return &screen->songs_window;
    }
    return &screen->tags_window;
}

void
media_library_screen_set_geometry(MediaLibraryScreen *screen,
                                  int32 start_x, int32 width,
                                  int32 main_start_y,
                                  int32 main_height) {
    if (screen == NULL) {
        return;
    }
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    library_layout(screen);
    return;
}

int32
media_library_screen_column_count(
    MediaLibraryScreen *screen
) {
    if (screen == NULL) {
        return 0;
    }
    if (screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        return 3;
    }
    return 2;
}

bool
media_library_screen_set_mode(MediaLibraryScreen *screen,
                              enum MediaLibraryMode mode) {
    if (screen == NULL) {
        return false;
    }
    if ((mode < MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        || (mode >= MEDIA_LIBRARY_MODE_COUNT)) {
        return false;
    }
    if (screen->mode == mode) {
        return true;
    }

    screen->mode = mode;
    media_library_screen_clear(screen);
    nc_menu_reset(nc_media_library_album_menu_base(&screen->albums));
    if ((mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS)) {
        screen->active_column = MEDIA_LIBRARY_COLUMN_ALBUMS;
    }
    library_update_titles(screen, true);
    library_update_menu_highlights(screen);
    library_layout(screen);
    return true;
}

enum MediaLibraryMode
media_library_screen_toggle_mode(MediaLibraryScreen *screen) {
    enum MediaLibraryMode next_mode;

    if (screen == NULL) {
        return MEDIA_LIBRARY_MODE_THREE_COLUMNS;
    }

    switch (screen->mode) {
    case MEDIA_LIBRARY_MODE_THREE_COLUMNS:
        next_mode = MEDIA_LIBRARY_MODE_TWO_COLUMNS;
        break;
    case MEDIA_LIBRARY_MODE_TWO_COLUMNS:
        next_mode = MEDIA_LIBRARY_MODE_ALBUM_ONLY;
        break;
    case MEDIA_LIBRARY_MODE_ALBUM_ONLY:
        next_mode = MEDIA_LIBRARY_MODE_THREE_COLUMNS;
        break;
    case MEDIA_LIBRARY_MODE_COUNT:
    default:
        next_mode = MEDIA_LIBRARY_MODE_THREE_COLUMNS;
        break;
    }
    media_library_screen_set_mode(screen, next_mode);
    return screen->mode;
}

enum MediaLibraryColumn
media_library_screen_active_column(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return MEDIA_LIBRARY_COLUMN_TAGS;
    }
    return screen->active_column;
}

bool
media_library_screen_item_available(
    MediaLibraryScreen *screen
) {
    NcMenu *menu;

    if ((menu = media_library_screen_active_menu(screen)) == NULL) {
        return false;
    }
    return !nc_menu_empty(menu);
}

bool
media_library_screen_set_active_column(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column
) {
    if (screen == NULL) {
        return false;
    }
    if ((column < MEDIA_LIBRARY_COLUMN_TAGS)
        || (column >= MEDIA_LIBRARY_COLUMN_COUNT)) {
        return false;
    }
    if ((column == MEDIA_LIBRARY_COLUMN_TAGS)
        && (screen->mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS)) {
        return false;
    }
    screen->active_column = column;
    library_update_menu_highlights(screen);
    return true;
}

bool
media_library_screen_column_visible(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column
) {
    if (screen == NULL) {
        return false;
    }
    if ((column < MEDIA_LIBRARY_COLUMN_TAGS)
        || (column >= MEDIA_LIBRARY_COLUMN_COUNT)) {
        return false;
    }
    if (column == MEDIA_LIBRARY_COLUMN_TAGS) {
        return screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS;
    }
    return true;
}

MediaLibraryColumnState *
media_library_screen_column_state(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column
) {
    if (screen == NULL) {
        return NULL;
    }
    if ((column < MEDIA_LIBRARY_COLUMN_TAGS)
        || (column >= MEDIA_LIBRARY_COLUMN_COUNT)) {
        return NULL;
    }
    return &screen->column_state[column];
}

StrBuilder *
media_library_screen_active_filter_constraint(
    MediaLibraryScreen *screen
) {
    MediaLibraryColumnState *state;

    if ((state = library_active_column_state(screen)) == NULL) {
        return NULL;
    }
    return &state->filter_constraint;
}

StrBuilder *
media_library_screen_active_search_constraint(
    MediaLibraryScreen *screen
) {
    MediaLibraryColumnState *state;

    if ((state = library_active_column_state(screen)) == NULL) {
        return NULL;
    }
    return &state->search_constraint;
}

NcMediaLibraryTagRow *
media_library_screen_current_tag(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_media_library_tag_menu_current(&screen->tags);
}

NcMediaLibraryAlbumRow *
media_library_screen_current_album(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return nc_media_library_album_menu_current(&screen->albums);
}

bool
media_library_screen_current_primary_tag_value(
    MediaLibraryScreen *screen, char **value, int32 *value_len
) {
    NcMediaLibraryTagRow *tag;
    NcMediaLibraryAlbumRow *album;

    if ((screen == NULL) || (value == NULL) || (value_len == NULL)) {
        return false;
    }

    if (screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        if ((tag = media_library_screen_current_tag(screen)) == NULL) {
            return false;
        }
        *value = tag->tag;
        *value_len = tag->tag_len;
        return true;
    }

    if (screen->mode == MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
        return false;
    }

    if ((album = media_library_screen_current_album(screen)) == NULL) {
        return false;
    }
    *value = album->tag;
    *value_len = album->tag_len;
    return true;
}

bool
media_library_screen_current_album_value(
    MediaLibraryScreen *screen, char **album, int32 *album_len
) {
    NcMediaLibraryAlbumRow *row;

    if ((screen == NULL) || (album == NULL) || (album_len == NULL)) {
        return false;
    }

    if (((row = media_library_screen_current_album(screen)) == NULL)
        || row->all_tracks_entry) {
        return false;
    }

    *album = row->album;
    *album_len = row->album_len;
    return true;
}

void
media_library_screen_format_tag_row(
    MediaLibraryScreen *screen, NcMediaLibraryTagRow *row,
    StrBuilder *output
) {
    StrBuilder converted;

    (void)screen;
    if (output == NULL) {
        return;
    }
    sb_clear(output);
    if (row == NULL) {
        return;
    }
    if ((row->tag == NULL) || (row->tag_len <= 0)) {
        if (Config.empty_tag && (Config.empty_tag_len > 0)) {
            SB_APPEND(output, Config.empty_tag,
                      Config.empty_tag_len);
        }
        return;
    }

    converted = ncm_charset_utf8_to_locale(row->tag, row->tag_len);
    SB_APPEND(output, converted.data, converted.len);
    sb_free(&converted);
    return;
}

void
media_library_screen_format_album_row(
    MediaLibraryScreen *screen, NcMediaLibraryAlbumRow *row,
    StrBuilder *output
) {
    StrBuilder raw = {0};
    StrBuilder converted;

    if (output == NULL) {
        return;
    }
    sb_clear(output);
    if (row == NULL) {
        return;
    }
    if (row->all_tracks_entry) {
        SB_APPEND(output, "All tracks");
        return;
    }

    if (screen
        && (screen->mode == MEDIA_LIBRARY_MODE_TWO_COLUMNS)) {
        if ((row->tag == NULL) || (row->tag_len <= 0)) {
            if (Config.empty_tag
                && (Config.empty_tag_len > 0)) {
                SB_APPEND(&raw, Config.empty_tag,
                          Config.empty_tag_len);
            }
        } else {
            SB_APPEND(&raw, row->tag, row->tag_len);
        }
        SB_APPEND(&raw, " - ");
    }
    if ((Config.media_lib_primary_tag != MPD_TAG_DATE)
        && !Config.media_lib_hide_album_dates
        && row->date && (row->date_len > 0)) {
        sb_append_byte(&raw, '(');
        SB_APPEND(&raw, row->date, row->date_len);
        SB_APPEND(&raw, ") ");
    }
    if ((row->album == NULL) || (row->album_len <= 0)) {
        SB_APPEND(&raw, "<no album>");
    } else {
        SB_APPEND(&raw, row->album, row->album_len);
    }

    converted = ncm_charset_utf8_to_locale(raw.data, raw.len);
    SB_APPEND(output, converted.data, converted.len);
    sb_free(&converted);
    sb_free(&raw);
    return;
}

void
media_library_screen_format_song_row(
    MediaLibraryScreen *screen, NcmSong *song, NcBuffer *output
) {
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
library_tag_array_item_destroy(void *item) {
    nc_media_library_tag_row_destroy(item);
    return;
}

static void
library_album_array_item_init(void *item) {
    MediaLibraryAlbumItem *album;

    album = item;
    album->row = (NcMediaLibraryAlbumRow){0};
    album->menu_flags = NC_MENU_ITEM_SELECTABLE;
    return;
}

static void
library_album_array_item_destroy(void *item) {
    MediaLibraryAlbumItem *album;

    album = item;
    nc_media_library_album_row_destroy(&album->row);
    album->menu_flags = NC_MENU_ITEM_SELECTABLE;
    return;
}

static int32
library_compare_tag_rows(NcMediaLibraryTagRow *left,
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
library_compare_album_items(MediaLibraryAlbumItem *left,
                            MediaLibraryAlbumItem *right) {
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
library_compare_bytes(char *left, int32 left_len,
                      char *right, int32 right_len) {
    int32 common_len;

    common_len = left_len;
    if (right_len < common_len) {
        common_len = right_len;
    }
    for (int32 i = 0; i < common_len; i += 1) {
        uint8 left_byte;
        uint8 right_byte;

        left_byte = (uint8)left[i];
        right_byte = (uint8)right[i];
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
library_compare_song_getter(NcmSong *left, NcmSong *right,
                            enum NcmSongGetter getter) {
    StrBuilder left_tags;
    StrBuilder right_tags;
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
    sb_free(&right_tags);
    sb_free(&left_tags);
    return result;
}

static int32
library_compare_songs(NcmSong *left, NcmSong *right) {
    static enum NcmSongGetter getters[] = {
        NCM_SONG_GETTER_DATE,
        NCM_SONG_GETTER_ALBUM,
        NCM_SONG_GETTER_DISC,
        NCM_SONG_GETTER_TRACK_NUMBER,
    };
    StrBuilder left_text;
    StrBuilder right_text;
    int32 result;

    for (int32 i = 0; i < LENGTH(getters); i += 1) {
        result = library_compare_song_getter(
            left, right, getters[i]);
        if (result != 0) {
            return result;
        }
    }

    left_text = ncm_format_render_string(
        &Config.song_library_format, left);
    right_text = ncm_format_render_string(
        &Config.song_library_format, right);
    result = library_compare_bytes(
        left_text.data, left_text.len, right_text.data, right_text.len);
    sb_free(&right_text);
    sb_free(&left_text);
    return result;
}

static void
library_sort_tags(MediaLibraryTagArray *tags) {
    for (int32 i = 1; i < tags->len; i += 1) {
        int32 j;

        j = i;
        while ((j > 0)
               && (library_compare_tag_rows(
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
library_sort_albums(MediaLibraryAlbumArray *albums) {
    for (int32 i = 1; i < albums->len; i += 1) {
        int32 j;

        j = i;
        while ((j > 0)
               && (library_compare_album_items(
                   &albums->items[j],
                   &albums->items[j - 1]) < 0)) {
            MediaLibraryAlbumItem tmp;

            tmp = albums->items[j];
            albums->items[j] = albums->items[j - 1];
            albums->items[j - 1] = tmp;
            j -= 1;
        }
    }
    return;
}

static void
library_sort_songs(NcmSongArray *songs) {
    for (int32 i = 1; i < songs->len; i += 1) {
        int32 j;

        j = i;
        while ((j > 0)
               && (library_compare_songs(
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
library_find_tag(MediaLibraryTagArray *tags,
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
library_find_album(MediaLibraryAlbumArray *albums,
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
library_append_tag(MediaLibraryTagArray *tags,
                   char *tag, int32 tag_len, time_t mtime) {
    NcMediaLibraryTagRow *row;

    if ((row = media_library_tag_array_append(tags)) == NULL) {
        return false;
    }
    if (!library_set_owned_string(&row->tag, &row->tag_len,
                                  &row->tag_cap, tag, tag_len)) {
        media_library_tag_array_remove_ordered(
            tags, tags->len - 1);
        return false;
    }
    row->mtime = mtime;
    return true;
}

static bool
library_append_album(MediaLibraryAlbumArray *albums,
                     char *tag, int32 tag_len,
                     char *album, int32 album_len,
                     char *date, int32 date_len,
                     time_t mtime, bool all_tracks_entry,
                     uint32 menu_flags) {
    MediaLibraryAlbumItem *item;
    NcMediaLibraryAlbumRow *row;

    if ((item = media_library_album_array_append(albums)) == NULL) {
        return false;
    }
    row = &item->row;
    if (!library_set_owned_string(&row->tag, &row->tag_len,
                                  &row->tag_cap, tag, tag_len)
        || !library_set_owned_string(
            &row->album, &row->album_len, &row->album_cap,
            album, album_len)
        || !library_set_owned_string(
            &row->date, &row->date_len, &row->date_cap,
            date, date_len)) {
        media_library_album_array_remove_ordered(
            albums, albums->len - 1);
        return false;
    }
    row->mtime = mtime;
    row->all_tracks_entry = all_tracks_entry;
    item->menu_flags = menu_flags;
    return true;
}

static bool
library_song_first_tag(NcmSong *song, enum mpd_tag_type tag,
                       NcmStringView *view) {
    if (view) {
        *view = (NcmStringView){0};
    }
    return ncm_song_tag_view(song, tag, 0, view);
}

static bool
library_add_three_column_album(
    MediaLibraryAlbumArray *albums, NcmSong *song,
    char *selected_tag, int32 selected_tag_len
) {
    NcmStringView album;
    NcmStringView date;
    int32 existing;

    library_song_first_tag(song, MPD_TAG_ALBUM, &album);
    library_song_first_tag(song, MPD_TAG_DATE, &date);
    if (!Config.media_library_albums_split_by_date) {
        ncm_string_view_clear(&date);
    }

    existing = library_find_album(
        albums, selected_tag, selected_tag_len,
        album.data, album.len, date.data, date.len);
    if (existing >= 0) {
        if (song->last_modified
            > albums->items[existing].row.mtime) {
            albums->items[existing].row.mtime = song->last_modified;
        }
        return true;
    }
    return library_append_album(
        albums, selected_tag, selected_tag_len,
        album.data, album.len, date.data, date.len,
        song->last_modified, false, NC_MENU_ITEM_SELECTABLE);
}

static bool
library_add_two_column_albums(
    MediaLibraryAlbumArray *albums, NcmSong *song,
    enum MediaLibraryMode mode, enum mpd_tag_type primary_tag
) {
    NcmStringView album;
    NcmStringView date;
    NcmStringView primary_value;

    library_song_first_tag(song, MPD_TAG_ALBUM, &album);
    library_song_first_tag(song, MPD_TAG_DATE, &date);
    if (!Config.media_library_albums_split_by_date) {
        ncm_string_view_clear(&date);
    }

    for (int32 i = 0;
         ncm_song_tag_view(song, primary_tag, i, &primary_value);
         i += 1) {
        char *tag;
        int32 tag_len;
        int32 existing;

        tag = primary_value.data;
        tag_len = primary_value.len;
        if (mode == MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
            tag = NULL;
            tag_len = 0;
        }
        existing = library_find_album(
            albums, tag, tag_len, album.data, album.len,
            date.data, date.len);
        if (existing >= 0) {
            albums->items[existing].row.mtime = song->last_modified;
            continue;
        }
        if (!library_append_album(
            albums, tag, tag_len, album.data, album.len,
            date.data, date.len, song->last_modified, false,
            NC_MENU_ITEM_SELECTABLE)) {
            return false;
        }
    }
    return true;
}

bool
media_library_tags_from_strings(
    MediaLibraryTagArray *tags, NcmMpdStringList *strings
) {
    MediaLibraryTagArray replacement;

    if ((tags == NULL) || (strings == NULL)) {
        return false;
    }

    replacement = (MediaLibraryTagArray){0};
    for (int32 i = 0; i < ncm_mpd_string_list_count(strings); i += 1) {
        NcmMpdString *string;

        if ((string = ncm_mpd_string_list_at(strings, i)) == NULL) {
            media_library_tag_array_destroy(&replacement);
            return false;
        }
        if (library_find_tag(&replacement, string->value,
                             string->value_len) >= 0) {
            continue;
        }
        if (!library_append_tag(
            &replacement, string->value, string->value_len, 0)) {
            media_library_tag_array_destroy(&replacement);
            return false;
        }
    }
    library_sort_tags(&replacement);
    media_library_tag_array_move(tags, &replacement);
    return true;
}

bool
media_library_tags_from_songs(
    MediaLibraryTagArray *tags, NcmMpdSongList *songs,
    enum mpd_tag_type primary_tag
) {
    MediaLibraryTagArray replacement;

    if ((tags == NULL) || (songs == NULL)
        || (primary_tag == MPD_TAG_UNKNOWN)) {
        return false;
    }

    replacement = (MediaLibraryTagArray){0};
    for (int32 i = 0; i < ncm_mpd_song_list_count(songs); i += 1) {
        NcmSong *song;
        NcmStringView primary_value;

        if ((song = ncm_mpd_song_list_at(songs, i)) == NULL) {
            media_library_tag_array_destroy(&replacement);
            return false;
        }
        for (int32 j = 0;
             ncm_song_tag_view(song, primary_tag, j, &primary_value);
             j += 1) {
            int32 existing;

            existing = library_find_tag(
                &replacement, primary_value.data, primary_value.len);
            if (existing >= 0) {
                if (song->last_modified
                    > replacement.items[existing].mtime) {
                    replacement.items[existing].mtime =
                        song->last_modified;
                }
                continue;
            }
            if (!library_append_tag(
                &replacement, primary_value.data,
                primary_value.len, song->last_modified)) {
                media_library_tag_array_destroy(&replacement);
                return false;
            }
        }
    }
    library_sort_tags(&replacement);
    media_library_tag_array_move(tags, &replacement);
    return true;
}

bool
media_library_albums_from_songs(
    MediaLibraryAlbumArray *albums, NcmMpdSongList *songs,
    enum MediaLibraryMode mode, enum mpd_tag_type primary_tag,
    char *selected_tag, int32 selected_tag_len
) {
    MediaLibraryAlbumArray replacement;
    int32 album_count;

    if ((albums == NULL) || (songs == NULL)
        || (mode < MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        || (mode >= MEDIA_LIBRARY_MODE_COUNT)
        || (primary_tag == MPD_TAG_UNKNOWN)
        || (selected_tag_len < 0)
        || ((selected_tag == NULL) && (selected_tag_len > 0))) {
        return false;
    }

    replacement = (MediaLibraryAlbumArray){0};
    for (int32 i = 0; i < ncm_mpd_song_list_count(songs); i += 1) {
        NcmSong *song;
        bool result;

        if ((song = ncm_mpd_song_list_at(songs, i)) == NULL) {
            media_library_album_array_destroy(&replacement);
            return false;
        }
        if (mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
            result = library_add_three_column_album(
                &replacement, song, selected_tag, selected_tag_len);
        } else {
            result = library_add_two_column_albums(
                &replacement, song, mode, primary_tag);
        }
        if (!result) {
            media_library_album_array_destroy(&replacement);
            return false;
        }
    }

    library_sort_albums(&replacement);
    album_count = replacement.len;
    if ((mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (album_count > 1)) {
        MediaLibraryAlbumItem *separator;

        if ((separator = media_library_album_array_append(&replacement))
            == NULL) {
            media_library_album_array_destroy(&replacement);
            return false;
        }
        separator->menu_flags = NC_MENU_ITEM_SEPARATOR;
        if (!library_append_album(
            &replacement, selected_tag, selected_tag_len,
            NULL, 0, NULL, 0, 0, true,
            NC_MENU_ITEM_SELECTABLE)) {
            media_library_album_array_destroy(&replacement);
            return false;
        }
    }

    media_library_album_array_move(albums, &replacement);
    return true;
}

bool
media_library_songs_from_list(
    NcmSongArray *songs, NcmMpdSongList *source
) {
    NcmSongArray replacement;
    int32 err;

    if ((songs == NULL) || (source == NULL)) {
        return false;
    }

    replacement = (NcmSongArray){0};
    for (int32 i = 0; i < ncm_mpd_song_list_count(source); i += 1) {
        NcmSong *song;

        if ((song = ncm_mpd_song_list_at(source, i)) == NULL) {
            ncm_song_array_destroy(&replacement);
            return false;
        }
        if ((err = ncm_song_array_append_copy(&replacement, song)) < 0) {
            ncm_song_array_destroy(&replacement);
            return false;
        }
    }
    library_sort_songs(&replacement);
    ncm_song_array_move(songs, &replacement);
    return true;
}

bool
media_library_screen_toggle_sort_mode(
    MediaLibraryScreen *screen
) {
    if (screen == NULL) {
        return false;
    }

    screen->sort_by_mtime = !screen->sort_by_mtime;
    Config.media_library_sort_by_mtime = screen->sort_by_mtime;
    library_update_titles(screen, true);
    media_library_screen_request_tags_update(screen);
    media_library_screen_request_albums_update(screen);
    media_library_screen_request_songs_update(screen);
    return screen->sort_by_mtime;
}

bool
media_library_screen_set_primary_tag_type(
    MediaLibraryScreen *screen, enum mpd_tag_type tag_type
) {
    if ((screen == NULL) || (tag_type == MPD_TAG_UNKNOWN)) {
        return false;
    }

    if (Config.media_lib_primary_tag == tag_type) {
        library_update_titles(screen, true);
        return true;
    }

    Config.media_lib_primary_tag = tag_type;
    nc_menu_clear_items(nc_media_library_tag_menu_base(&screen->tags));
    nc_menu_clear_items(nc_media_library_album_menu_base(&screen->albums));
    nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));
    library_reset_observed_highlights(screen);
    library_update_titles(screen, true);
    media_library_screen_request_tags_update(screen);
    nc_screen_finish_list_change(&screen->screen);
    return true;
}

void
media_library_screen_request_database_update(
    MediaLibraryScreen *screen
) {
    library_request_all_updates(screen);
    return;
}

bool
media_library_screen_refresh_inactive_songs(
    MediaLibraryScreen *screen
) {
    if (screen == NULL) {
        return false;
    }
    if (!media_library_screen_column_visible(
        screen, MEDIA_LIBRARY_COLUMN_SONGS)) {
        return false;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS) {
        return false;
    }

    library_refresh_menu(
        nc_media_library_song_menu_base(&screen->songs),
        &screen->songs_window);
    return true;
}

bool
media_library_screen_previous_column_available(
    MediaLibraryScreen *screen
) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS) {
        return library_column_has_visible_items(
            screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return library_column_has_visible_items(
            screen, MEDIA_LIBRARY_COLUMN_TAGS);
    }
    return false;
}

bool
media_library_screen_next_column_available(
    MediaLibraryScreen *screen
) {
    if (screen == NULL) {
        return false;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS) {
        return library_column_has_visible_items(
            screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return library_column_has_visible_items(
            screen, MEDIA_LIBRARY_COLUMN_SONGS);
    }
    return false;
}

void
media_library_screen_previous_column(
    MediaLibraryScreen *screen
) {
    if (!media_library_screen_previous_column_available(screen)) {
        return;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS) {
        media_library_screen_set_active_column(
            screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
    } else {
        media_library_screen_set_active_column(
            screen, MEDIA_LIBRARY_COLUMN_TAGS);
    }
    return;
}

void
media_library_screen_next_column(MediaLibraryScreen *screen) {
    if (!media_library_screen_next_column_available(screen)) {
        return;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS) {
        media_library_screen_set_active_column(
            screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
    } else {
        media_library_screen_set_active_column(
            screen, MEDIA_LIBRARY_COLUMN_SONGS);
    }
    return;
}

void
media_library_screen_clear(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    nc_menu_clear_items(nc_media_library_tag_menu_base(&screen->tags));
    nc_menu_clear_items(nc_media_library_album_menu_base(&screen->albums));
    nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));
    library_reset_observed_highlights(screen);
    return;
}

bool
media_library_screen_current_song(MediaLibraryScreen *screen,
                                  NcmSong *song) {
    NcmSong *current;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }
    if ((current = nc_media_library_song_menu_current(&screen->songs))
        == NULL) {
        return false;
    }
    return ncm_song_copy(song, current) == 0;
}

bool
media_library_screen_selected_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs
) {
    NcmError ncm_error;
    bool result;

    ncm_error_clear(&ncm_error);
    result = media_library_screen_selected_songs_checked(
        screen, songs, &ncm_error);
    if (!result && ncm_error_is_set(&ncm_error)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, ncm_error.message);
    }
    return result;
}

bool
media_library_screen_selected_songs_checked(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error
) {
    if ((screen == NULL) || (songs == NULL)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing media-library songs"));
        return false;
    }
    return library_collect_selected_songs(screen, songs, ncm_error);
}

bool
media_library_screen_copy_visible_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error
) {
    NcMenu *menu;

    if ((screen == NULL) || (songs == NULL)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing media-library songs"));
        return false;
    }

    menu = nc_media_library_song_menu_base(&screen->songs);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (!library_copy_song_at(screen, songs, i)) {
            library_set_conversion_error(
                ncm_error,
                STRLIT("failed to copy media-library songs"));
            return false;
        }
    }
    return true;
}

bool
media_library_screen_apply_filter(
    MediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    NcmError *ncm_error
) {
    MediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;
    NcMenu *menu;

    if (screen == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing media library"));
        return false;
    }
    if (pattern_len <= 0) {
        media_library_screen_clear_filter(screen);
        return true;
    }
    if (pattern == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing filter pattern"));
        return false;
    }

    if ((state = library_active_column_state(screen)) == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("invalid active column"));
        return false;
    }
    if (ncm_regex_compile(&state->filter_regex, pattern, pattern_len,
                          Config.regex_flags, ncm_error) < 0) {
        return false;
    }
    if (sb_set(&state->filter_constraint, pattern, pattern_len) < 0) {
        ncm_error_set(ncm_error, ENOMEM, STRLIT("cannot save filter"));
        return false;
    }

    callbacks = library_display_callbacks(
        screen, screen->active_column, true);
    menu = media_library_screen_active_menu(screen);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_apply_filter(menu);
    state->filter_enabled = true;
    nc_screen_finish_list_change(&screen->screen);
    return true;
}

void
media_library_screen_clear_filter(MediaLibraryScreen *screen) {
    MediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }

    if ((state = library_active_column_state(screen)) == NULL) {
        return;
    }
    ncm_regex_destroy(&state->filter_regex);
    state->filter_regex = (NcmRegex){0};
    sb_clear(&state->filter_constraint);
    menu = media_library_screen_active_menu(screen);
    callbacks = library_display_callbacks(
        screen, screen->active_column, false);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_show_all_items(menu);
    state->filter_enabled = false;
    nc_screen_finish_list_change(&screen->screen);
    return;
}

bool
media_library_screen_search(MediaLibraryScreen *screen,
                            char *pattern, int32 pattern_len,
                            bool forward, bool wrap,
                            bool skip_current, NcmError *ncm_error) {
    MediaLibrarySearchContext context;
    MediaLibraryColumnState *state;
    NcMenu *menu;
    bool result;

    if (screen == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing media library"));
        return false;
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing search pattern"));
        return false;
    }

    if ((state = library_active_column_state(screen)) == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("invalid active column"));
        return false;
    }
    if (ncm_regex_compile(&state->search_regex, pattern, pattern_len,
                          Config.regex_flags, ncm_error) < 0) {
        return false;
    }
    if (sb_set(&state->search_constraint, pattern, pattern_len) < 0) {
        ncm_error_set(ncm_error, ENOMEM, STRLIT("cannot save search"));
        return false;
    }
    state->search_enabled = true;

    menu = media_library_screen_active_menu(screen);
    context.screen = screen;
    context.regex = &state->search_regex;
    result = nc_menu_search_selectable(menu, screen->main_height, forward,
                                       wrap, skip_current,
                                       library_search_position,
                                       &context, NULL);
    if (result) {
        nc_screen_finish_list_change(&screen->screen);
    }
    return result;
}

static bool
library_search_position(NcMenu *menu, int32 pos,
                        void *user) {
    MediaLibrarySearchContext *context;

    context = user;
    return library_active_item_matches(
        context->screen, menu, pos, context->regex);
}

void
media_library_screen_clear_search(MediaLibraryScreen *screen) {
    MediaLibraryColumnState *state;

    if ((state = library_active_column_state(screen)) == NULL) {
        return;
    }
    ncm_regex_destroy(&state->search_regex);
    state->search_regex = (NcmRegex){0};
    sb_clear(&state->search_constraint);
    state->search_enabled = false;
    return;
}

void
media_library_screen_request_tags_update(
    MediaLibraryScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    if (screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        screen->tags_update_request = true;
    } else {
        screen->albums_update_request = true;
        screen->songs_update_request = true;
    }
    nc_screen_request_update(&screen->screen);
    return;
}

void
media_library_screen_request_albums_update(
    MediaLibraryScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    screen->albums_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
media_library_screen_request_songs_update(
    MediaLibraryScreen *screen
) {
    if (screen == NULL) {
        return;
    }
    screen->songs_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
media_library_screen_finish_list_change(
    MediaLibraryScreen *screen
) {
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
    if (screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        tag = media_library_screen_current_tag(screen);
    }
    album = media_library_screen_current_album(screen);
    tag_valid = tag;
    album_valid = album;

    tag_changed = screen->observed_tag_valid != tag_valid;
    if (!tag_changed && tag_valid) {
        tag_changed = !library_tag_identity_equal(
            &screen->observed_tag, tag);
    }
    album_changed = screen->observed_album_valid != album_valid;
    if (!album_changed && album_valid) {
        album_changed = !library_album_identity_equal(
            &screen->observed_album, album);
    }

    if (tag_changed) {
        nc_menu_clear_items(
            nc_media_library_album_menu_base(&screen->albums));
        nc_menu_clear_items(
            nc_media_library_song_menu_base(&screen->songs));
        library_restart_update_timer(screen);
        library_set_observed_tag(screen, tag);
        library_set_observed_album(screen, NULL);
        nc_screen_request_update(&screen->screen);
        return;
    }

    if (album_changed) {
        nc_menu_clear_items(
            nc_media_library_song_menu_base(&screen->songs));
        library_restart_update_timer(screen);
        library_set_observed_album(screen, album);
        nc_screen_request_update(&screen->screen);
        return;
    }

    library_set_observed_tag(screen, tag);
    library_set_observed_album(screen, album);
    return;
}

bool
media_library_screen_update(MediaLibraryScreen *screen,
                            NcmError *ncm_error) {
    if (screen == NULL) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing media library"));
        return false;
    }
    ncm_error_clear(ncm_error);

    if (library_tags_pending(screen)) {
        return library_update_tags(screen, ncm_error);
    }

    if (library_albums_pending(screen)
        && ((screen->mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS)
            || screen->albums_update_request
            || library_fetch_delay_elapsed(screen))) {
        return library_update_albums(screen, ncm_error);
    }

    if (library_songs_pending(screen)
        && (screen->songs_update_request
            || library_fetch_delay_elapsed(screen))) {
        return library_update_songs(screen, ncm_error);
    }

    if (!library_tags_pending(screen)
        && !library_albums_pending(screen)
        && !library_songs_pending(screen)) {
        nc_screen_clear_update_request(&screen->screen);
    }
    return true;
}

static bool
library_update_tags(MediaLibraryScreen *screen,
                    NcmError *ncm_error) {
    MediaLibraryTagArray tags;
    NcmMpdStringList strings;
    NcmMpdSongList songs;
    bool result;

    tags = (MediaLibraryTagArray){0};
    strings = (NcmMpdStringList){0};
    songs = (NcmMpdSongList){0};

    if (screen->sort_by_mtime) {
        result = media_library_screen_list_all_songs(
            screen, &songs, ncm_error);
        if (!result) {
            screen->tags_update_request = true;
            if (screen->sort_by_mtime) {
                media_library_screen_toggle_sort_mode(screen);
            }
            ncm_mpd_song_list_destroy(&songs);
            ncm_mpd_string_list_destroy(&strings);
            media_library_tag_array_destroy(&tags);
            return false;
        }
        result = media_library_tags_from_songs(
            &tags, &songs, Config.media_lib_primary_tag);
    } else {
        result = media_library_screen_list_tags(
            screen, Config.media_lib_primary_tag, &strings, ncm_error);
        if (!result) {
            screen->tags_update_request = true;
            ncm_mpd_song_list_destroy(&songs);
            ncm_mpd_string_list_destroy(&strings);
            media_library_tag_array_destroy(&tags);
            return false;
        }
        result = media_library_tags_from_strings(&tags, &strings);
    }

    if (!result) {
        screen->tags_update_request = true;
        library_set_conversion_error(
            ncm_error, STRLIT("failed to build media-library tags"));
    } else {
        if (!(result = library_replace_tags(screen, &tags))) {
            screen->tags_update_request = true;
            library_set_conversion_error(
                ncm_error, STRLIT("failed to replace media-library tags"));
        } else {
            screen->tags_update_request = false;
        }
    }

    ncm_mpd_song_list_destroy(&songs);
    ncm_mpd_string_list_destroy(&strings);
    media_library_tag_array_destroy(&tags);
    return result;
}

static bool
library_update_albums(MediaLibraryScreen *screen,
                      NcmError *ncm_error) {
    MediaLibraryAlbumArray albums;
    MediaLibrarySongQuery query = {0};
    NcMediaLibraryTagRow *tag;
    NcmMpdSongList songs;
    char *selected_tag;
    int32 selected_tag_len;
    bool result;

    albums = (MediaLibraryAlbumArray){0};
    songs = (NcmMpdSongList){0};
    selected_tag = NULL;
    selected_tag_len = 0;

    if (screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        if ((tag = media_library_screen_current_tag(screen)) == NULL) {
            ncm_mpd_song_list_destroy(&songs);
            media_library_album_array_destroy(&albums);
            return true;
        }
        selected_tag = tag->tag;
        selected_tag_len = tag->tag_len;
        query.primary_value = tag->tag;
        query.primary_value_len = tag->tag_len;
        query.primary_tag = Config.media_lib_primary_tag;
        query.match_primary_tag = true;
        result = media_library_screen_search_songs(
            screen, &query, &songs, ncm_error);
    } else {
        result = media_library_screen_list_all_songs(
            screen, &songs, ncm_error);
        if (!result) {
            media_library_screen_toggle_mode(screen);
            library_request_all_updates(screen);
        }
    }

    if (!result) {
        screen->albums_update_request = true;
        ncm_mpd_song_list_destroy(&songs);
        media_library_album_array_destroy(&albums);
        return false;
    }

    result = media_library_albums_from_songs(
        &albums, &songs, screen->mode, Config.media_lib_primary_tag,
        selected_tag, selected_tag_len);
    if (!result) {
        screen->albums_update_request = true;
        library_set_conversion_error(
            ncm_error, STRLIT("failed to build media-library albums"));
    } else {
        if (!(result = library_replace_albums(screen, &albums))) {
            screen->albums_update_request = true;
            library_set_conversion_error(
                ncm_error,
                STRLIT("failed to replace media-library albums"));
        } else {
            screen->albums_update_request = false;
        }
    }

    ncm_mpd_song_list_destroy(&songs);
    media_library_album_array_destroy(&albums);
    return result;
}

static bool
library_update_songs(MediaLibraryScreen *screen,
                     NcmError *ncm_error) {
    MediaLibrarySongQuery query = {0};
    NcMediaLibraryAlbumRow *album;
    NcmMpdSongList source;
    NcmSongArray songs;
    bool result;

    if ((album = media_library_screen_current_album(screen)) == NULL) {
        return true;
    }

    query.primary_tag = Config.media_lib_primary_tag;
    if (screen->mode != MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
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

    source = (NcmMpdSongList){0};
    songs = (NcmSongArray){0};
    result = media_library_screen_search_songs(
        screen, &query, &source, ncm_error);
    if (!result) {
        screen->songs_update_request = true;
        ncm_song_array_destroy(&songs);
        ncm_mpd_song_list_destroy(&source);
        return false;
    }

    if (!(result = media_library_songs_from_list(&songs, &source))) {
        screen->songs_update_request = true;
        library_set_conversion_error(
            ncm_error, STRLIT("failed to build media-library songs"));
    } else {
        if (!(result = library_replace_songs(screen, &songs))) {
            screen->songs_update_request = true;
            library_set_conversion_error(
                ncm_error, STRLIT("failed to replace media-library songs"));
        } else {
            screen->songs_update_request = false;
        }
    }

    ncm_song_array_destroy(&songs);
    ncm_mpd_song_list_destroy(&source);
    return result;
}

static bool
library_replace_tags(MediaLibraryScreen *screen,
                     MediaLibraryTagArray *tags) {
    NcMediaLibraryTagMenu replacement;
    NcMediaLibraryTagRow identity;
    NcMediaLibraryTagRow *current;
    NcMenu *menu;
    NcMenu *replacement_menu;
    int32 highlight;
    bool identity_valid;

    ASSERT(screen != NULL);
    ASSERT(tags != NULL);

    menu = nc_media_library_tag_menu_base(&screen->tags);
    current = nc_media_library_tag_menu_current(&screen->tags);
    highlight = nc_menu_highlight(menu);
    identity_valid = false;
    identity = (NcMediaLibraryTagRow){0};
    if (current) {
        identity_valid = nc_media_library_tag_row_copy(
            &identity, current) >= 0;
        library_set_observed_tag(screen, current);
    } else {
        library_set_observed_tag(screen, NULL);
    }

    nc_media_library_tag_menu_init(&replacement);
    replacement_menu = nc_media_library_tag_menu_base(&replacement);
    nc_menu_copy(replacement_menu, menu);
    for (int32 i = 0; i < tags->len; i += 1) {
        nc_media_library_tag_menu_add(&replacement, &tags->items[i]);
    }
    library_apply_column_filter(
        screen, MEDIA_LIBRARY_COLUMN_TAGS, replacement_menu);
    library_restore_tag_identity(
        &replacement, &identity, identity_valid, highlight);

    nc_menu_swap(menu, replacement_menu);
    nc_media_library_tag_menu_destroy(&replacement);
    nc_media_library_tag_row_destroy(&identity);
    nc_screen_finish_list_change(&screen->screen);
    return true;
}

static bool
library_replace_albums(MediaLibraryScreen *screen,
                       MediaLibraryAlbumArray *albums) {
    NcMediaLibraryAlbumMenu replacement;
    NcMediaLibraryAlbumRow identity;
    NcMediaLibraryAlbumRow *current;
    NcMenu *menu;
    NcMenu *replacement_menu;
    int32 highlight;
    bool identity_valid;

    ASSERT(screen != NULL);
    ASSERT(albums != NULL);

    menu = nc_media_library_album_menu_base(&screen->albums);
    current = nc_media_library_album_menu_current(&screen->albums);
    highlight = nc_menu_highlight(menu);
    identity_valid = false;
    identity = (NcMediaLibraryAlbumRow){0};
    if (current) {
        identity_valid = nc_media_library_album_row_copy(
            &identity, current) >= 0;
        library_set_observed_album(screen, current);
    } else {
        library_set_observed_album(screen, NULL);
    }

    nc_media_library_album_menu_init(&replacement);
    replacement_menu = nc_media_library_album_menu_base(&replacement);
    nc_menu_copy(replacement_menu, menu);
    for (int32 i = 0; i < albums->len; i += 1) {
        nc_media_library_album_menu_add_with_flags(
            &replacement, &albums->items[i].row,
            albums->items[i].menu_flags);
    }
    library_apply_column_filter(
        screen, MEDIA_LIBRARY_COLUMN_ALBUMS, replacement_menu);
    library_restore_album_identity(
        &replacement, &identity, identity_valid, highlight);

    nc_menu_swap(menu, replacement_menu);
    nc_media_library_album_menu_destroy(&replacement);
    nc_media_library_album_row_destroy(&identity);
    nc_screen_finish_list_change(&screen->screen);
    return true;
}

static bool
library_replace_songs(MediaLibraryScreen *screen,
                      NcmSongArray *songs) {
    NcMediaLibrarySongMenu replacement;
    NcmSong identity;
    NcmSong *current;
    NcMenu *menu;
    NcMenu *replacement_menu;
    int32 highlight;
    bool identity_valid;

    ASSERT(screen != NULL);
    ASSERT(songs != NULL);

    menu = nc_media_library_song_menu_base(&screen->songs);
    current = nc_media_library_song_menu_current(&screen->songs);
    highlight = nc_menu_highlight(menu);
    identity_valid = false;
    identity = (NcmSong){0};
    if (current != NULL) {
        identity_valid = ncm_song_copy(&identity, current) == 0;
    }

    nc_media_library_song_menu_init(&replacement);
    replacement_menu = nc_media_library_song_menu_base(&replacement);
    nc_menu_copy(replacement_menu, menu);
    for (int32 i = 0; i < songs->len; i += 1) {
        nc_media_library_song_menu_add(&replacement, &songs->items[i]);
    }
    library_apply_column_filter(
        screen, MEDIA_LIBRARY_COLUMN_SONGS, replacement_menu);
    library_restore_song_identity(
        &replacement, &identity, identity_valid, highlight);

    nc_menu_swap(menu, replacement_menu);
    nc_media_library_song_menu_destroy(&replacement);
    ncm_song_destroy(&identity);
    return true;
}

static void
library_apply_column_filter(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column, NcMenu *menu
) {
    MediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;

    state = media_library_screen_column_state(screen, column);
    ASSERT(state != NULL);
    ASSERT(menu != NULL);

    callbacks = library_display_callbacks(
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
library_restore_highlight(NcMenu *menu, int32 highlight) {
    int32 count;

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
library_restore_tag_identity(
    NcMediaLibraryTagMenu *menu, NcMediaLibraryTagRow *identity,
    bool identity_valid, int32 fallback
) {
    NcMenu *base;

    base = nc_media_library_tag_menu_base(menu);
    if (identity_valid) {
        for (int32 i = 0; i < nc_menu_item_count(base); i += 1) {
            NcMediaLibraryTagRow *candidate;

            candidate = nc_media_library_tag_menu_item_at(
                menu, base->active_items, i);
            if (candidate
                && library_tag_identity_equal(candidate, identity)
                && nc_menu_goto_selectable(base, i)) {
                return;
            }
        }
    }
    library_restore_highlight(base, fallback);
    return;
}

static void
library_restore_album_identity(
    NcMediaLibraryAlbumMenu *menu, NcMediaLibraryAlbumRow *identity,
    bool identity_valid, int32 fallback
) {
    NcMenu *base;

    base = nc_media_library_album_menu_base(menu);
    if (identity_valid) {
        for (int32 i = 0; i < nc_menu_item_count(base); i += 1) {
            NcMediaLibraryAlbumRow *candidate;

            candidate = nc_media_library_album_menu_item_at(
                menu, base->active_items, i);
            if (candidate
                && library_album_identity_equal(candidate, identity)
                && nc_menu_goto_selectable(base, i)) {
                return;
            }
        }
    }
    library_restore_highlight(base, fallback);
    return;
}

static void
library_restore_song_identity(
    NcMediaLibrarySongMenu *menu, NcmSong *identity,
    bool identity_valid, int32 fallback
) {
    NcMenu *base;

    base = nc_media_library_song_menu_base(menu);
    if (identity_valid) {
        for (int32 i = 0; i < nc_menu_item_count(base); i += 1) {
            NcmSong *candidate;

            candidate = nc_media_library_song_menu_item_at(
                menu, base->active_items, i);
            if (candidate && ncm_song_is_equal(candidate, identity)
                && nc_menu_goto_selectable(base, i)) {
                return;
            }
        }
    }
    library_restore_highlight(base, fallback);
    return;
}

static bool
library_tag_identity_equal(NcMediaLibraryTagRow *left,
                           NcMediaLibraryTagRow *right) {
    if ((left == NULL) || (right == NULL)) {
        return left == right;
    }
    return STREQUAL(left->tag, left->tag_len, right->tag, right->tag_len);
}

static bool
library_album_identity_equal(NcMediaLibraryAlbumRow *left,
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
library_set_observed_tag(MediaLibraryScreen *screen,
                         NcMediaLibraryTagRow *tag) {
    nc_media_library_tag_row_destroy(&screen->observed_tag);
    screen->observed_tag = (NcMediaLibraryTagRow){0};
    screen->observed_tag_valid = false;
    if (tag) {
        screen->observed_tag_valid = nc_media_library_tag_row_copy(
            &screen->observed_tag, tag) >= 0;
    }
    return;
}

static void
library_set_observed_album(MediaLibraryScreen *screen,
                           NcMediaLibraryAlbumRow *album) {
    nc_media_library_album_row_destroy(&screen->observed_album);
    screen->observed_album = (NcMediaLibraryAlbumRow){0};
    screen->observed_album_valid = false;
    if (album) {
        screen->observed_album_valid = nc_media_library_album_row_copy(
            &screen->observed_album, album) >= 0;
    }
    return;
}

static void
library_reset_observed_highlights(
    MediaLibraryScreen *screen
) {
    library_set_observed_tag(screen, NULL);
    library_set_observed_album(screen, NULL);
    return;
}

static void
library_restart_update_timer(MediaLibraryScreen *screen) {
    screen->update_timer = global_timer;
    return;
}

static bool
library_tags_pending(MediaLibraryScreen *screen) {
    NcMenu *tags;

    if ((screen == NULL)
        || (screen->mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS)) {
        return false;
    }
    tags = nc_media_library_tag_menu_base(&screen->tags);
    return screen->tags_update_request
           || (nc_menu_all_item_count(tags) <= 0);
}

static bool
library_albums_pending(MediaLibraryScreen *screen) {
    NcMenu *albums;

    if (screen == NULL) {
        return false;
    }
    if ((screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (media_library_screen_current_tag(screen) == NULL)) {
        return false;
    }
    albums = nc_media_library_album_menu_base(&screen->albums);
    return screen->albums_update_request
           || (nc_menu_all_item_count(albums) <= 0);
}

static bool
library_songs_pending(MediaLibraryScreen *screen) {
    NcMenu *songs;

    if ((screen == NULL)
        || (media_library_screen_current_album(screen) == NULL)) {
        return false;
    }
    songs = nc_media_library_song_menu_base(&screen->songs);
    return screen->songs_update_request
           || (nc_menu_all_item_count(songs) <= 0);
}

static bool
library_fetch_delay_elapsed(MediaLibraryScreen *screen) {
    ASSERT(screen != NULL);
    if (screen->fetching_delay_ms < 0) {
        return true;
    }
    return global_timer_elapsed_ms(screen->update_timer)
           > screen->fetching_delay_ms;
}

static bool
library_update_due(MediaLibraryScreen *screen) {
    if (library_tags_pending(screen)) {
        return true;
    }
    if (library_albums_pending(screen)
        && ((screen->mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS)
            || screen->albums_update_request
            || library_fetch_delay_elapsed(screen))) {
        return true;
    }
    if (library_songs_pending(screen)
        && (screen->songs_update_request
            || library_fetch_delay_elapsed(screen))) {
        return true;
    }
    return false;
}

static void
library_set_conversion_error(NcmError *ncm_error, char *message,
                             int32 message_len) {
    if (ncm_error && !ncm_error_is_set(ncm_error)) {
        ncm_error_set(ncm_error, EINVAL, message, message_len);
    }
    return;
}

static void
library_request_all_updates(MediaLibraryScreen *screen) {
    ASSERT(screen != NULL);
    screen->tags_update_request = true;
    screen->albums_update_request = true;
    screen->songs_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

static void
library_clear_column_filter(MediaLibraryScreen *screen,
                            enum MediaLibraryColumn column) {
    MediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }

    if (((state = media_library_screen_column_state(screen, column)) == NULL)
        || ((menu = library_column_menu(screen, column)) == NULL)) {
        return;
    }

    ncm_regex_destroy(&state->filter_regex);
    state->filter_regex = (NcmRegex){0};
    sb_clear(&state->filter_constraint);
    callbacks = library_display_callbacks(screen, column, false);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_show_all_items(menu);
    state->filter_enabled = false;
    return;
}

static void
library_sort_tag_menu(NcMediaLibraryTagMenu *menu) {
    NcMenu *base;

    ASSERT(menu != NULL);

    base = nc_media_library_tag_menu_base(menu);
    for (int32 i = 1; i < nc_menu_all_item_count(base); i += 1) {
        int32 j;

        j = i;
        while (j > 0) {
            NcMediaLibraryTagRow *left;
            NcMediaLibraryTagRow *right;

            if (((left = nc_media_library_tag_menu_item_at(
                      menu, NC_MENU_ITEMS_ALL, j)) == NULL)
                || ((right = nc_media_library_tag_menu_item_at(
                         menu, NC_MENU_ITEMS_ALL, j - 1)) == NULL)
                || (library_compare_tag_rows(left, right) >= 0)) {
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
library_sort_album_menu(NcMediaLibraryAlbumMenu *menu) {
    NcMenu *base;

    ASSERT(menu != NULL);

    base = nc_media_library_album_menu_base(menu);
    for (int32 i = 1; i < nc_menu_all_item_count(base); i += 1) {
        int32 j;

        j = i;
        while (j > 0) {
            MediaLibraryAlbumItem left;
            MediaLibraryAlbumItem right;
            NcMediaLibraryAlbumRow *left_row;
            NcMediaLibraryAlbumRow *right_row;
            int32 left_rank;
            int32 right_rank;

            if (((left_row = nc_media_library_album_menu_item_at(
                      menu, NC_MENU_ITEMS_ALL, j)) == NULL)
                || ((right_row = nc_media_library_album_menu_item_at(
                         menu, NC_MENU_ITEMS_ALL, j - 1)) == NULL)) {
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
            if (library_compare_album_items(&left, &right) >= 0) {
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
library_move_to_tag(MediaLibraryScreen *screen,
                    char *tag, int32 tag_len) {
    NcMenu *menu;

    if ((screen == NULL) || (tag_len < 0)
        || ((tag == NULL) && (tag_len > 0))) {
        return false;
    }

    menu = nc_media_library_tag_menu_base(&screen->tags);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMediaLibraryTagRow *row;

        row = nc_menu_active_item_at(menu, i);
        if (row
            && STREQUAL(row->tag, row->tag_len, tag, tag_len)) {
            return nc_menu_goto_selectable(menu, i);
        }
    }
    return false;
}

static bool
library_move_to_album(MediaLibraryScreen *screen,
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
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMediaLibraryAlbumRow *row;
        bool tag_matches;
        bool date_matches;

        if (nc_menu_position_is_separator(menu, i)) {
            continue;
        }

        if ((row = nc_menu_active_item_at(menu, i)) == NULL) {
            continue;
        }

        tag_matches = (screen->mode == MEDIA_LIBRARY_MODE_ALBUM_ONLY)
                      || (screen->mode
                          == MEDIA_LIBRARY_MODE_THREE_COLUMNS)
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
        return library_move_to_album(
            screen, tag, tag_len, album, album_len, date, date_len, false);
    }
    return false;
}

static bool
library_move_to_song(MediaLibraryScreen *screen,
                     NcmSong *song) {
    NcMenu *menu;

    if ((screen == NULL) || (song == NULL)) {
        return false;
    }

    menu = nc_media_library_song_menu_base(&screen->songs);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmSong *candidate;

        if ((candidate = nc_menu_active_item_at(menu, i))
            && ncm_song_is_equal(candidate, song)) {
            return nc_menu_goto_selectable(menu, i);
        }
    }
    return false;
}

static bool
library_insert_locate_tag(MediaLibraryScreen *screen,
                          char *tag, int32 tag_len, time_t mtime) {
    NcMediaLibraryTagRow row;
    bool result;

    if ((screen == NULL) || (tag_len < 0)
        || ((tag == NULL) && (tag_len > 0))) {
        return false;
    }

    row = (NcMediaLibraryTagRow){0};
    result = library_set_owned_string(&row.tag, &row.tag_len,
                                      &row.tag_cap, tag, tag_len);
    if (result) {
        row.mtime = mtime;
        nc_media_library_tag_menu_add(&screen->tags, &row);
        library_sort_tag_menu(&screen->tags);
    }
    nc_media_library_tag_row_destroy(&row);
    return result;
}

static bool
library_insert_locate_album(MediaLibraryScreen *screen,
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

    row = (NcMediaLibraryAlbumRow){0};
    result = library_set_owned_string(&row.tag, &row.tag_len,
                                      &row.tag_cap, tag, tag_len);
    if (result) {
        result = library_set_owned_string(
            &row.album, &row.album_len, &row.album_cap,
            album, album_len);
    }
    if (result) {
        result = library_set_owned_string(
            &row.date, &row.date_len, &row.date_cap, date, date_len);
    }
    if (result) {
        row.mtime = mtime;
        row.all_tracks_entry = false;
        nc_media_library_album_menu_add_with_flags(
            &screen->albums, &row, NC_MENU_ITEM_SELECTABLE);
        library_sort_album_menu(&screen->albums);
    }
    nc_media_library_album_row_destroy(&row);
    return result;
}

static bool
library_locate_song_requirements(NcmSong *song,
                                 NcmStringView *primary_value,
                                 NcmError *ncm_error) {
    if ((song == NULL) || (primary_value == NULL)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing locate-song argument"));
        return false;
    }
    if (!ncm_song_tag_view(song, Config.media_lib_primary_tag,
                           0, primary_value)
        || (primary_value->len <= 0)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("song has no primary tag"));
        return false;
    }
    if (!ncm_song_is_from_database(song)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("song is not from the database"));
        return false;
    }
    return true;
}

static void
library_print_add_status(MediaLibraryScreen *screen,
                         NcmSongArray *songs, bool result) {
    NcMediaLibraryAlbumRow *album;
    StrBuilder message = {0};
    StrBuilder rendered;
    char *tag_name;

    ASSERT(screen != NULL);
    ASSERT(songs != NULL);
    if (!result && (songs->len <= 0)) {
        return;
    }

    if (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS) {
        NcMediaLibraryTagRow *tag;

        tag = media_library_screen_current_tag(screen);
        tag_name = ncm_tag_type_name(Config.media_lib_primary_tag);
        SB_APPEND(&message, "Songs with ");
        for (int32 i = 0; tag_name[i] != '\0'; i += 1) {
            char ch;

            ch = tag_name[i];
            if ((ch >= 'A') && (ch <= 'Z')) {
                ch = (char)(ch - 'A' + 'a');
            }
            sb_append_byte(&message, ch);
        }
        SB_APPEND(&message, " \"");
        if (tag && tag->tag) {
            SB_APPEND(&message, tag->tag, tag->tag_len);
        }
        SB_APPEND(&message, "\" added");
        sb_printf(&message, "%s", ncm_helpers_with_errors(result));
    } else if (screen->active_column
               == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        if ((album = media_library_screen_current_album(screen))
            && album->all_tracks_entry) {
            tag_name = ncm_tag_type_name(Config.media_lib_primary_tag);
            SB_APPEND(&message, "Songs with ");
            for (int32 i = 0; tag_name[i] != '\0'; i += 1) {
                char ch;

                ch = tag_name[i];
                if ((ch >= 'A') && (ch <= 'Z')) {
                    ch = (char)(ch - 'A' + 'a');
                }
                sb_append_byte(&message, ch);
            }
            SB_APPEND(&message, " \"");
            if (album->tag) {
                SB_APPEND(&message, album->tag, album->tag_len);
            }
            SB_APPEND(&message, "\" added");
        } else {
            SB_APPEND(&message,
                      "Songs from album \"");
            if (album && album->album) {
                SB_APPEND(&message, album->album,
                          album->album_len);
            }
            SB_APPEND(&message, "\" added");
        }
        SB_APPEND(&message, ncm_helpers_with_errors(result),
                  optional_strlen32(
                      ncm_helpers_with_errors(result)));
    } else if (result && (songs->len == 1)) {
        rendered = ncm_format_render_string(&Config.song_status_format,
                                            &songs->items[0]);
        SB_APPEND(&message, "Added to playlist: ");
        SB_APPEND(&message, rendered.data, rendered.len);
        sb_free(&rendered);
    } else if (result) {
        SB_APPEND(&message, "Songs added");
        SB_APPEND(&message, ncm_helpers_with_errors(result),
                  optional_strlen32(
                      ncm_helpers_with_errors(result)));
    } else {
        sb_free(&message);
        return;
    }

    ncm_statusbar_print(Config.message_delay_time,
                        message.data, message.len);
    sb_free(&message);
    return;
}

static void
library_print_add_error(NcmError *ncm_error) {
    if (ncm_error && ncm_error_is_set(ncm_error)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, ncm_error->message);
    }
    return;
}

bool
media_library_screen_list_tags(
    MediaLibraryScreen *screen, enum mpd_tag_type tag_type,
    NcmMpdStringList *tags, NcmError *ncm_error
) {
    if ((screen == NULL) || (screen->hooks.list_tags == NULL)) {
        ncm_error_set(ncm_error, ENOSYS, STRLIT("tag hook is unavailable"));
        return false;
    }
    if (tags == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing tag list"));
        return false;
    }
    return screen->hooks.list_tags(screen->hooks.user, tag_type, tags,
                                   ncm_error);
}

bool
media_library_screen_list_all_songs(
    MediaLibraryScreen *screen, NcmMpdSongList *songs,
    NcmError *ncm_error
) {
    if ((screen == NULL) || (screen->hooks.list_all_songs == NULL)) {
        ncm_error_set(ncm_error, ENOSYS,
                      STRLIT("song-list hook is unavailable"));
        return false;
    }
    if (songs == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing song list"));
        return false;
    }
    return screen->hooks.list_all_songs(screen->hooks.user, songs, ncm_error);
}

bool
media_library_screen_search_songs(
    MediaLibraryScreen *screen,
    MediaLibrarySongQuery *query, NcmMpdSongList *songs,
    NcmError *ncm_error
) {
    if ((screen == NULL) || (screen->hooks.search_songs == NULL)) {
        ncm_error_set(ncm_error, ENOSYS,
                      STRLIT("song-search hook is unavailable"));
        return false;
    }
    if ((query == NULL) || (songs == NULL)) {
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing song search arguments"));
        return false;
    }
    return screen->hooks.search_songs(screen->hooks.user, query, songs,
                                      ncm_error);
}

bool
media_library_screen_add_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs, bool play,
    NcmError *ncm_error
) {
    if ((screen == NULL) || (screen->hooks.add_songs == NULL)) {
        ncm_error_set(ncm_error, ENOSYS,
                      STRLIT("add-songs hook is unavailable"));
        return false;
    }
    if ((songs == NULL) || (songs->len <= 0)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing songs"));
        return false;
    }
    return screen->hooks.add_songs(screen->hooks.user, songs, play,
                                   ncm_error);
}

bool
media_library_screen_add_item_to_playlist(
    MediaLibraryScreen *screen, bool play, NcmError *ncm_error
) {
    NcmSongArray songs;
    bool result;

    if (screen == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing media library"));
        return false;
    }

    songs = (NcmSongArray){0};
    result = library_collect_current_item_songs(screen, &songs, ncm_error);
    if (result && (songs.len <= 0)) {
        ncm_error_set(ncm_error, ENOENT, STRLIT("no selected songs"));
        result = false;
    }
    if (result) {
        result = media_library_screen_add_songs(screen, &songs,
                                                play, ncm_error);
    }
    library_print_add_status(screen, &songs, result);
    ncm_song_array_destroy(&songs);
    return result;
}

bool
media_library_screen_locate_song(MediaLibraryScreen *screen,
                                 NcmSong *song, NcmError *ncm_error) {
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
        ncm_error_set(ncm_error, EINVAL,
                      STRLIT("missing locate-song argument"));
        return false;
    }
    ncm_error_clear(ncm_error);
    if (!library_locate_song_requirements(
        song, &primary_value, ncm_error)) {
        return false;
    }

    library_song_first_tag(song, MPD_TAG_ALBUM, &album);
    library_song_first_tag(song, MPD_TAG_DATE, &date);
    album_date = date.data;
    album_date_len = date.len;
    if (!Config.media_library_albums_split_by_date) {
        album_date = NULL;
        album_date_len = 0;
    }

    tags_menu = nc_media_library_tag_menu_base(&screen->tags);
    albums_menu = nc_media_library_album_menu_base(&screen->albums);
    songs_menu = nc_media_library_song_menu_base(&screen->songs);
    if (screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        library_clear_column_filter(
            screen, MEDIA_LIBRARY_COLUMN_TAGS);
        if (nc_menu_empty(tags_menu)) {
            media_library_screen_request_tags_update(screen);
            if (!media_library_screen_update(screen, ncm_error)) {
                return false;
            }
        }
        if (!library_move_to_tag(screen, primary_value.data,
                                 primary_value.len)) {
            if (!library_insert_locate_tag(
                screen, primary_value.data, primary_value.len,
                song->last_modified)) {
                ncm_error_set(ncm_error, ENOMEM,
                              STRLIT("failed to insert locate tag"));
                return false;
            }
            if (!library_move_to_tag(screen, primary_value.data,
                                     primary_value.len)) {
                ncm_error_set(ncm_error, ENOENT,
                              STRLIT("song tag is not in library"));
                return false;
            }
        }
        nc_screen_finish_list_change(&screen->screen);
    }

    library_clear_column_filter(
        screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
    if (nc_menu_empty(albums_menu)) {
        media_library_screen_request_albums_update(screen);
        if (!media_library_screen_update(screen, ncm_error)) {
            return false;
        }
    }

    if ((screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && nc_menu_empty(albums_menu)) {
        media_library_screen_set_active_column(
            screen, MEDIA_LIBRARY_COLUMN_TAGS);
        ncm_error_set(ncm_error, ENOENT,
                      STRLIT("song album is not in library"));
        return false;
    }

    album_tag = primary_value.data;
    album_tag_len = primary_value.len;
    if (screen->mode == MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
        album_tag = NULL;
        album_tag_len = 0;
    }

    if (!library_move_to_album(
        screen, primary_value.data, primary_value.len,
        album.data, album.len, date.data, date.len, true)) {
        if (!library_insert_locate_album(
            screen, album_tag, album_tag_len, album.data, album.len,
            album_date, album_date_len, song->last_modified)) {
            ncm_error_set(ncm_error, ENOMEM,
                          STRLIT("failed to insert locate album"));
            return false;
        }
        if (!library_move_to_album(
            screen, primary_value.data, primary_value.len,
            album.data, album.len, date.data, date.len, true)) {
            ncm_error_set(ncm_error, ENOENT,
                          STRLIT("song album is not in library"));
            return false;
        }
    }
    media_library_screen_set_active_column(
        screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
    nc_screen_finish_list_change(&screen->screen);

    library_clear_column_filter(
        screen, MEDIA_LIBRARY_COLUMN_SONGS);
    media_library_screen_request_songs_update(screen);
    if (!media_library_screen_update(screen, ncm_error)) {
        return false;
    }
    if (nc_menu_empty(songs_menu)) {
        nc_menu_clear_items(albums_menu);
        media_library_screen_set_active_column(
            screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
        ncm_error_set(ncm_error, ENOENT,
                      STRLIT("song is not visible in media library"));
        return false;
    }
    if (!library_move_to_song(screen, song)) {
        media_library_screen_set_active_column(
            screen, MEDIA_LIBRARY_COLUMN_SONGS);
        ncm_error_set(ncm_error, ENOENT,
                      STRLIT("song is not visible in media library"));
        return false;
    }

    media_library_screen_set_active_column(
        screen, MEDIA_LIBRARY_COLUMN_SONGS);
    ncm_error_clear(ncm_error);
    return true;
}

static MediaLibraryScreen *
library_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
library_active_window(NcScreen *screen) {
    return media_library_screen_active_window(
        library_from_screen(screen));
}

static void
library_refresh(NcScreen *screen) {
    MediaLibraryScreen *library;
    NcMenu *albums;

    library = library_from_screen(screen);
    library_update_menu_highlights(library);
    if (media_library_screen_column_visible(
        library, MEDIA_LIBRARY_COLUMN_TAGS)) {
        library_refresh_menu(
            nc_media_library_tag_menu_base(&library->tags),
            &library->tags_window);
        nc_screen_draw_vertical_separator(
            nc_window_start_x(&library->albums_window) - 1);
    }

    albums = nc_media_library_album_menu_base(&library->albums);
    library_refresh_menu(albums, &library->albums_window);
    if (nc_menu_empty(albums)) {
        nc_window_go_to_xy(&library->albums_window, 0, 0);
        nc_window_print_data(&library->albums_window,
                             STRLIT("No albums found."));
        nc_window_refresh(&library->albums_window);
    }
    nc_screen_draw_vertical_separator(
        nc_window_start_x(&library->songs_window) - 1);
    library_refresh_menu(
        nc_media_library_song_menu_base(&library->songs),
        &library->songs_window);
    return;
}

static void
library_refresh_window(NcScreen *screen) {
    library_refresh(screen);
    return;
}

static void
library_scroll(NcScreen *screen, enum NcScroll where) {
    MediaLibraryScreen *library;

    library = library_from_screen(screen);
    nc_menu_scroll_selectable(media_library_screen_active_menu(library),
                              library->main_height, where);
    return;
}

static void
library_finish_list_change(NcScreen *screen) {
    media_library_screen_finish_list_change(
        library_from_screen(screen));
    return;
}

static void
library_mouse_button_pressed(NcScreen *screen,
                             MEVENT event) {
    MediaLibraryScreen *library;
    int32 x;
    int32 y;

    if ((library = library_from_screen(screen)) == NULL) {
        return;
    }

    x = event.x;
    y = event.y;
    if (media_library_screen_column_visible(
        library, MEDIA_LIBRARY_COLUMN_TAGS)
        && nc_window_has_coords(&library->tags_window, &x, &y)) {
        media_library_screen_set_active_column(
            library, MEDIA_LIBRARY_COLUMN_TAGS);
        if (!((event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
              && library_mouse_select(
                  library, MEDIA_LIBRARY_COLUMN_TAGS,
                  nc_media_library_tag_menu_base(&library->tags), y,
                  (event.bstate & BUTTON3_PRESSED) != 0))) {
            if (event.bstate & BUTTON5_PRESSED) {
                library_mouse_scroll(library, NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                library_mouse_scroll(library, NC_SCROLL_UP);
            }
        }
        nc_screen_finish_list_change(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&library->albums_window, &x, &y)) {
        media_library_screen_set_active_column(
            library, MEDIA_LIBRARY_COLUMN_ALBUMS);
        if (!(((event.bstate
                & (BUTTON1_PRESSED | BUTTON3_PRESSED)) != 0)
              && library_mouse_select(
                  library, MEDIA_LIBRARY_COLUMN_ALBUMS,
                  nc_media_library_album_menu_base(&library->albums), y,
                  (event.bstate & BUTTON3_PRESSED) != 0))) {
            if (event.bstate & BUTTON5_PRESSED) {
                library_mouse_scroll(library, NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                library_mouse_scroll(library, NC_SCROLL_UP);
            }
        }
        nc_screen_finish_list_change(screen);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&library->songs_window, &x, &y)) {
        media_library_screen_set_active_column(
            library, MEDIA_LIBRARY_COLUMN_SONGS);
        if (!(((event.bstate
                & (BUTTON1_PRESSED | BUTTON3_PRESSED)) != 0)
              && library_mouse_select(
                  library, MEDIA_LIBRARY_COLUMN_SONGS,
                  nc_media_library_song_menu_base(&library->songs), y,
                  (event.bstate & BUTTON3_PRESSED) != 0))) {
            if (event.bstate & BUTTON5_PRESSED) {
                library_mouse_scroll(library, NC_SCROLL_DOWN);
            } else if (event.bstate & BUTTON4_PRESSED) {
                library_mouse_scroll(library, NC_SCROLL_UP);
            }
        }
        nc_screen_finish_list_change(screen);
    }
    return;
}

static void
library_mouse_scroll(MediaLibraryScreen *screen,
                     enum NcScroll where) {
    enum NcScroll effective;
    int32 count;

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

    for (int32 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(
            media_library_screen_active_menu(screen),
            screen->main_height, effective);
    }
    return;
}

static bool
library_mouse_select(
    MediaLibraryScreen *screen, enum MediaLibraryColumn column,
    NcMenu *menu, int32 y, bool right_click
) {
    NcmError ncm_error;
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
        ncm_error_clear(&ncm_error);
        play = screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS;
        if (!media_library_screen_add_item_to_playlist(
            screen, play, &ncm_error)) {
            library_print_add_error(&ncm_error);
        }
    }

    if (column == MEDIA_LIBRARY_COLUMN_TAGS) {
        nc_menu_clear_items(
            nc_media_library_album_menu_base(&screen->albums));
        nc_menu_clear_items(
            nc_media_library_song_menu_base(&screen->songs));
        screen->albums_update_request = true;
        screen->songs_update_request = true;
        library_set_observed_album(screen, NULL);
        library_restart_update_timer(screen);
        nc_screen_request_update(&screen->screen);
    } else if (column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        nc_menu_clear_items(
            nc_media_library_song_menu_base(&screen->songs));
        screen->songs_update_request = true;
        library_restart_update_timer(screen);
        nc_screen_request_update(&screen->screen);
    }
    return true;
}

static void
library_switch_to(NcScreen *screen) {
    (void)screen;
    return;
}

static void
library_resize(NcScreen *screen) {
    MediaLibraryScreen *library;
    int32 x_offset;
    int32 width;

    library = library_from_screen(screen);
    nc_screen_switcher_get_resize_params(screen, &x_offset, &width, true);
    media_library_screen_set_geometry(
        library, x_offset, width, ui_state_main_start_y(),
        ui_state_main_height());
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
library_window_timeout(NcScreen *screen) {
    MediaLibraryScreen *library;

    if ((library = library_from_screen(screen)) == NULL) {
        return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }
    if (library_albums_pending(library)
        || library_songs_pending(library)) {
        return library->window_timeout_ms;
    }
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
library_title(NcScreen *screen) {
    (void)screen;
    return "Media library";
}

static void
library_update(NcScreen *screen) {
    MediaLibraryScreen *library;
    NcmError ncm_error;
    bool update_due;

    library = library_from_screen(screen);
    update_due = library_update_due(library);
    ncm_error_clear(&ncm_error);
    if (!media_library_screen_update(library, &ncm_error)) {
        if (ncm_error_is_set(&ncm_error)) {
            ncm_statusbar_print_cstring(
                Config.message_delay_time, ncm_error.message);
        }
        return;
    }
    if (update_due) {
        library_refresh(screen);
    }
    return;
}



static void
library_destroy_callback(NcScreen *screen) {
    media_library_screen_destroy(library_from_screen(screen));
    return;
}

static bool
library_tag_filter(NcMenu *menu, void *item, void *user) {
    MediaLibraryScreen *screen;

    (void)menu;
    screen = user;
    return library_tag_matches(
        screen, item,
        &screen->column_state[MEDIA_LIBRARY_COLUMN_TAGS].filter_regex);
}

static bool
library_album_filter(NcMenu *menu, void *item, void *user) {
    MediaLibraryScreen *screen;
    NcMediaLibraryAlbumRow *row;

    screen = user;
    row = item;
    if (library_menu_item_is_separator(menu, item)
        || (row && row->all_tracks_entry)) {
        return true;
    }
    return library_album_matches(
        screen, row,
        &screen->column_state[MEDIA_LIBRARY_COLUMN_ALBUMS].filter_regex);
}

static bool
library_song_filter(NcMenu *menu, void *item, void *user) {
    MediaLibraryScreen *screen;

    (void)menu;
    screen = user;
    return library_song_matches(
        screen, item,
        &screen->column_state[MEDIA_LIBRARY_COLUMN_SONGS].filter_regex);
}

static void
library_draw_tag(NcMenu *menu, NcWindow *window,
                 void *item, int32 pos, void *user) {
    StrBuilder text = {0};

    (void)menu;
    (void)pos;
    media_library_screen_format_tag_row(user, item, &text);
    nc_window_print_data(window, text.data, text.len);
    sb_free(&text);
    return;
}

static void
library_draw_album(NcMenu *menu, NcWindow *window,
                   void *item, int32 pos, void *user) {
    StrBuilder text = {0};

    (void)menu;
    (void)pos;
    media_library_screen_format_album_row(user, item, &text);
    nc_window_print_data(window, text.data, text.len);
    sb_free(&text);
    return;
}

static void
library_draw_song(NcMenu *menu, NcWindow *window,
                  void *item, int32 pos, void *user) {
    NcBuffer text;

    (void)menu;
    (void)pos;
    text = (NcBuffer){0};
    media_library_screen_format_song_row(user, item, &text);
    library_print_buffer(window, &text);
    nc_buffer_destroy(&text);
    return;
}

static void
library_print_buffer(NcWindow *window, NcBuffer *buffer) {
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
library_copy_song_at(MediaLibraryScreen *screen,
                     NcmSongArray *songs, int32 pos) {
    NcmSong *song;

    if ((song = nc_menu_active_item_at(
             nc_media_library_song_menu_base(&screen->songs), pos))
        == NULL) {
        return true;
    }
    return ncm_song_array_append_copy(songs, song) >= 0;
}

static NcMenu *
library_column_menu(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column
) {
    ASSERT(screen != NULL);
    switch (column) {
    case MEDIA_LIBRARY_COLUMN_TAGS:
        return nc_media_library_tag_menu_base(&screen->tags);
    case MEDIA_LIBRARY_COLUMN_ALBUMS:
        return nc_media_library_album_menu_base(&screen->albums);
    case MEDIA_LIBRARY_COLUMN_SONGS:
        return nc_media_library_song_menu_base(&screen->songs);
    case MEDIA_LIBRARY_COLUMN_COUNT:
    default:
        return NULL;
    }
}

static bool
library_column_has_visible_items(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column
) {
    NcMenu *menu;

    if (!media_library_screen_column_visible(screen, column)) {
        return false;
    }
    if ((menu = library_column_menu(screen, column)) == NULL) {
        return false;
    }
    return nc_menu_all_item_count(menu) > 0;
}

static bool
library_menu_item_is_separator(NcMenu *menu, void *item) {
    ASSERT(menu != NULL);
    ASSERT(item != NULL);
    for (int32 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        if ((nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i) == item)
            && (nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i)
                & NC_MENU_ITEM_SEPARATOR)) {
            return true;
        }
    }
    return false;
}

static void
library_query_from_tag(MediaLibraryScreen *screen,
                       NcMediaLibraryTagRow *tag,
                       MediaLibrarySongQuery *query) {
    (void)screen;
    query->primary_tag = Config.media_lib_primary_tag;
    if (tag) {
        query->primary_value = tag->tag;
        query->primary_value_len = tag->tag_len;
        query->match_primary_tag = true;
    }
    return;
}

static void
library_query_from_album(MediaLibraryScreen *screen,
                         NcMediaLibraryAlbumRow *album,
                         MediaLibrarySongQuery *query) {
    ASSERT(screen != NULL);
    ASSERT(album != NULL);
    query->primary_tag = Config.media_lib_primary_tag;
    if (screen->mode != MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
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
library_append_query_songs(
    MediaLibraryScreen *screen, MediaLibrarySongQuery *query,
    NcmSongArray *songs, NcmError *ncm_error
) {
    NcmMpdSongList source;
    NcmSongArray sorted;
    int32 err;
    bool result;

    source = (NcmMpdSongList){0};
    sorted = (NcmSongArray){0};
    result = media_library_screen_search_songs(
        screen, query, &source, ncm_error);
    if (result) {
        result = media_library_songs_from_list(&sorted, &source);
        if (!result) {
            library_set_conversion_error(
                ncm_error, STRLIT("failed to build selected songs"));
        }
    }
    if (result) {
        for (int32 i = 0; i < sorted.len; i += 1) {
            if ((err = ncm_song_array_append_copy(
                     songs, &sorted.items[i])) < 0) {
                ncm_error_set_status(ncm_error, err,
                                     STRLIT("failed to copy selected songs"));
                result = false;
                break;
            }
        }
    }
    ncm_song_array_destroy(&sorted);
    ncm_mpd_song_list_destroy(&source);
    return result;
}

static bool
library_collect_tag_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error
) {
    MediaLibrarySongQuery query;
    NcMediaLibraryTagRow *row;
    NcMenu *menu;
    bool any_selected;

    menu = nc_media_library_tag_menu_base(&screen->tags);
    any_selected = nc_menu_has_selected(menu);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!any_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }
        if ((row = nc_menu_active_item_at(menu, i)) == NULL) {
            continue;
        }
        query = (MediaLibrarySongQuery){0};
        library_query_from_tag(screen, row, &query);
        if (!library_append_query_songs(
            screen, &query, songs, ncm_error)) {
            return false;
        }
    }
    return true;
}

static bool
library_collect_album_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error
) {
    MediaLibrarySongQuery query;
    NcMediaLibraryAlbumRow *row;
    NcMenu *menu;
    bool any_selected;

    menu = nc_media_library_album_menu_base(&screen->albums);
    any_selected = nc_menu_has_selected(menu);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (nc_menu_position_is_separator(menu, i)) {
            continue;
        }
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!any_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }
        if ((row = nc_menu_active_item_at(menu, i)) == NULL) {
            continue;
        }
        query = (MediaLibrarySongQuery){0};
        library_query_from_album(screen, row, &query);
        if (!library_append_query_songs(
            screen, &query, songs, ncm_error)) {
            return false;
        }
    }
    return true;
}

static bool
library_collect_visible_song_rows(
    MediaLibraryScreen *screen, NcmSongArray *songs
) {
    NcMenu *menu;
    bool any_selected;

    menu = nc_media_library_song_menu_base(&screen->songs);
    any_selected = nc_menu_has_selected(menu);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        if (any_selected && !nc_menu_position_is_selected(menu, i)) {
            continue;
        }
        if (!any_selected && (i != nc_menu_highlight(menu))) {
            continue;
        }
        if (!library_copy_song_at(screen, songs, i)) {
            return false;
        }
    }
    return true;
}

static bool
library_collect_selected_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error
) {
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS) {
        return library_collect_tag_songs(screen, songs, ncm_error);
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        return library_collect_album_songs(screen, songs, ncm_error);
    }
    return library_collect_visible_song_rows(screen, songs);
}

static bool
library_collect_current_item_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error
) {
    MediaLibrarySongQuery query = {0};
    NcMediaLibraryTagRow *tag;
    NcMediaLibraryAlbumRow *album;
    NcMenu *menu;

    if (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS) {
        if ((tag = media_library_screen_current_tag(screen)) == NULL) {
            return true;
        }
        library_query_from_tag(screen, tag, &query);
        return library_append_query_songs(screen, &query, songs,
                                          ncm_error);
    }

    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        if ((album = media_library_screen_current_album(screen)) == NULL) {
            return true;
        }
        library_query_from_album(screen, album, &query);
        return library_append_query_songs(screen, &query, songs,
                                          ncm_error);
    }

    menu = nc_media_library_song_menu_base(&screen->songs);
    return library_copy_song_at(screen, songs, nc_menu_highlight(menu));
}

static MediaLibraryColumnState *
library_active_column_state(MediaLibraryScreen *screen) {
    ASSERT(screen != NULL);
    return media_library_screen_column_state(screen,
                                             screen->active_column);
}

static NcMenuDisplayCallbacks
library_display_callbacks(
    MediaLibraryScreen *screen,
    enum MediaLibraryColumn column, bool filter_enabled
) {
    NcMenuDisplayCallbacks callbacks = {0};

    callbacks.user = screen;
    switch (column) {
    case MEDIA_LIBRARY_COLUMN_TAGS:
        callbacks.draw = library_draw_tag;
        if (filter_enabled) {
            callbacks.filter = library_tag_filter;
        }
        break;
    case MEDIA_LIBRARY_COLUMN_ALBUMS:
        callbacks.draw = library_draw_album;
        if (filter_enabled) {
            callbacks.filter = library_album_filter;
        }
        break;
    case MEDIA_LIBRARY_COLUMN_SONGS:
        callbacks.draw = library_draw_song;
        if (filter_enabled) {
            callbacks.filter = library_song_filter;
        }
        break;
    case MEDIA_LIBRARY_COLUMN_COUNT:
    default:
        break;
    }
    return callbacks;
}

static void
library_update_menu_highlights(
    MediaLibraryScreen *screen
) {
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

    active = media_library_screen_active_menu(screen);
    nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    return;
}

static void
library_update_titles(MediaLibraryScreen *screen,
                      bool update_windows) {
    char *tag_type_name;
    int32 tag_type_name_len;

    sb_clear(&screen->tags_title);
    sb_clear(&screen->albums_title);
    sb_clear(&screen->songs_title);
    if (Config.titles_visibility) {
        tag_type_name = ncm_tag_type_name(Config.media_lib_primary_tag);
        tag_type_name_len = optional_strlen32(tag_type_name);
        SB_APPEND(&screen->tags_title, tag_type_name,
                  tag_type_name_len);
        sb_append_byte(&screen->tags_title, 's');
        SB_APPEND(&screen->albums_title,
                  "Albums");
        SB_APPEND(&screen->songs_title, "Songs");

        if (screen->mode == MEDIA_LIBRARY_MODE_TWO_COLUMNS) {
            SB_APPEND(&screen->albums_title,
                      " (sorted by ");
            for (int32 i = 0; i < tag_type_name_len; i += 1) {
                char ch = tag_type_name[i];

                if ((ch >= 'A') && (ch <= 'Z')) {
                    ch = (char)(ch - 'A' + 'a');
                }
                sb_append_byte(&screen->albums_title, ch);
            }
            if (screen->sort_by_mtime) {
                SB_APPEND(&screen->albums_title,
                          " and mtime");
            }
            sb_append_byte(&screen->albums_title, ')');
        } else if ((screen->mode
                    == MEDIA_LIBRARY_MODE_ALBUM_ONLY)
                   && screen->sort_by_mtime) {
            SB_APPEND(&screen->albums_title,
                      " (sorted by mtime)");
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
library_refresh_menu(NcMenu *menu, NcWindow *window) {
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static bool
library_active_item_matches(MediaLibraryScreen *screen,
                            NcMenu *menu, int32 pos,
                            NcmRegex *regex) {
    void *item;

    item = nc_menu_active_item_at(menu, pos);
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        NcMediaLibraryAlbumRow *album;

        album = item;
        if (nc_menu_position_is_separator(menu, pos)
            || (album && album->all_tracks_entry)) {
            return false;
        }
        return library_album_matches(screen, album, regex);
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS) {
        return library_song_matches(screen, item, regex);
    }
    return library_tag_matches(screen, item, regex);
}

static bool
library_tag_matches(MediaLibraryScreen *screen,
                    NcMediaLibraryTagRow *row, NcmRegex *regex) {
    StrBuilder text = {0};
    bool result;

    ASSERT(row != NULL);
    media_library_screen_format_tag_row(screen, row, &text);
    result = ncm_regex_search(regex, text.data, text.len);
    sb_free(&text);
    return result;
}

static bool
library_album_matches(MediaLibraryScreen *screen,
                      NcMediaLibraryAlbumRow *row,
                      NcmRegex *regex) {
    StrBuilder text = {0};
    bool result;

    ASSERT(row != NULL);
    media_library_screen_format_album_row(screen, row, &text);
    result = ncm_regex_search(regex, text.data, text.len);
    sb_free(&text);
    return result;
}

static bool
library_song_matches(MediaLibraryScreen *screen,
                     NcmSong *song, NcmRegex *regex) {
    StrBuilder text;
    bool result;

    (void)screen;
    ASSERT(song != NULL);
    text = ncm_format_render_string(&Config.song_library_format, song);
    result = ncm_regex_search(regex, text.data, text.len);
    sb_free(&text);
    return result;
}

static void
library_layout(MediaLibraryScreen *screen) {
    int32 left_width;
    int32 middle_width;
    int32 right_width;
    int32 middle_x;
    int32 right_x;
    int32 left_ratio;
    int32 middle_ratio;
    int32 right_ratio;
    int32 ratio_sum;

    if ((screen == NULL) || (screen->width <= 0)) {
        return;
    }

    if (screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        left_ratio = library_ratio_value(
            &Config.media_library_column_width_ratio_three, 0, 1);
        middle_ratio = library_ratio_value(
            &Config.media_library_column_width_ratio_three, 1, 1);
        right_ratio = library_ratio_value(
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

    left_ratio = library_ratio_value(
        &Config.media_library_column_width_ratio_two, 0, 1);
    right_ratio = library_ratio_value(
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
library_ratio_value(NcmInt32Array *ratios, int32 idx,
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
library_set_owned_string(char **dest, int32 *dest_len,
                         int32 *dest_cap, char *source,
                         int32 source_len) {
    char *copy;
    int32 cap;

    if ((dest == NULL) || (dest_len == NULL) || (dest_cap == NULL)) {
        return false;
    }
    library_free_owned_string(dest, dest_len, dest_cap);
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
library_free_owned_string(char **data, int32 *len, int32 *cap) {
    free2(*data, *cap);
    *data = NULL;
    *len = 0;
    *cap = 0;
    return;
}

static char *
library_query_cstring(StrBuilder *buffer, char *string,
                      int32 string_len) {
    ASSERT(buffer != NULL);
    if (string_len < 0) {
        if (string == NULL) {
            return NULL;
        }
        string_len = optional_strlen32(string);
    }
    if ((string == NULL) && (string_len > 0)) {
        return NULL;
    }

    sb_clear(buffer);
    SB_APPEND(buffer, string, string_len);
    return buffer->data;
}

static bool
library_mpd_list_tags(void *user, enum mpd_tag_type tag_type,
                      NcmMpdStringList *tags, NcmError *ncm_error) {
    NcmMpdClient *client;

    if ((client = user) == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing MPD client"));
        return false;
    }
    return ncm_mpd_client_get_list(client, tag_type, tags, ncm_error) == 0;
}

static bool
library_mpd_list_all_songs(void *user, NcmMpdSongList *songs,
                           NcmError *ncm_error) {
    NcmMpdClient *client;

    if ((client = user) == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing MPD client"));
        return false;
    }
    return ncm_mpd_client_get_directory_recursive(client, "/",
                                                  songs, ncm_error) == 0;
}

static bool
library_mpd_search_songs(void *user,
                         MediaLibrarySongQuery *query,
                         NcmMpdSongList *songs,
                         NcmError *ncm_error) {
    NcmMpdClient *client;
    StrBuilder primary = {0};
    StrBuilder album = {0};
    StrBuilder date = {0};
    char *value;
    bool result;

    client = user;
    ASSERT(client != NULL);
    ASSERT(query != NULL);
    ASSERT(songs != NULL);

    if ((result = ncm_mpd_client_start_search(client, true,
                                              ncm_error) == 0)
        && query->match_primary_tag) {
        if ((value = library_query_cstring(
                 &primary, query->primary_value, query->primary_value_len))
            == NULL) {
            ncm_error_set(ncm_error, EINVAL,
                          STRLIT("missing primary tag value"));
            result = false;
        } else {
            result = ncm_mpd_client_add_search_tag(
                client, query->primary_tag, value, ncm_error) == 0;
        }
    }
    if (result && query->match_album) {
        if ((value = library_query_cstring(
                 &album, query->album, query->album_len)) == NULL) {
            ncm_error_set(ncm_error, EINVAL,
                          STRLIT("missing album value"));
            result = false;
        } else {
            result = ncm_mpd_client_add_search_tag(
                client, MPD_TAG_ALBUM, value, ncm_error) == 0;
        }
    }
    if (result && query->match_date) {
        if ((value = library_query_cstring(
                 &date, query->date, query->date_len)) == NULL) {
            ncm_error_set(ncm_error, EINVAL,
                          STRLIT("missing date value"));
            result = false;
        } else {
            result = ncm_mpd_client_add_search_tag(
                client, MPD_TAG_DATE, value, ncm_error) == 0;
        }
    }
    if (result) {
        result = ncm_mpd_client_commit_search_songs(client, songs,
                                                    ncm_error) == 0;
    }
    sb_free(&date);
    sb_free(&album);
    sb_free(&primary);
    return result;
}

static bool
library_mpd_add_songs(void *user, NcmSongArray *songs, bool play,
                      NcmError *ncm_error) {
    NcmMpdClient *client;
    NcmMpdSongList additions;
    int32 play_pos;
    int32 err;
    bool result;

    client = user;
    ASSERT(client != NULL);
    ASSERT(songs != NULL);

    additions = (NcmMpdSongList){0};
    result = true;
    for (int32 i = 0; i < songs->len; i += 1) {
        if ((err = ncm_mpd_song_list_append_copy(
                 &additions, &songs->items[i])) < 0) {
            ncm_error_set_status(ncm_error, err,
                                 STRLIT("cannot copy songs for MPD"));
            result = false;
            break;
        }
    }

    play_pos = -1;
    if (play) {
        play_pos = ncm_status_state_playlist_length();
    }
    if (result) {
        result = ncm_mpd_client_add_song_list(client, &additions, -1,
                                              ncm_error) == 0;
    }
    if (result && play) {
        result = ncm_mpd_client_play_pos(client, play_pos, ncm_error) == 0;
    }
    ncm_mpd_song_list_destroy(&additions);
    return result;
}

#endif /* NCMPCPP_NC_MEDIA_LIBRARY_C */
