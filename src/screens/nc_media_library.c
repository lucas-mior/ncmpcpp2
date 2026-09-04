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

// callbacks
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

static int32
library_mpd_search_songs(void *user,
                         MediaLibrarySongQuery *query,
                         NcmMpdSongList *songs,
                         NcmError *ncm_error) {
    NcmMpdClient *client = user;
    int32 status;

    ASSERT(client != NULL);
    ASSERT(query != NULL);
    ASSERT(songs != NULL);

    status = ncm_mpd_client_start_search(client, true, ncm_error);
    if ((status == 0) && query->match_primary_tag) {
        status = ncm_mpd_client_add_search_tag(
            client, query->primary_tag, query->primary_value, ncm_error);
    }
    if ((status == 0) && query->match_album) {
        status = ncm_mpd_client_add_search_tag(
            client, MPD_TAG_ALBUM, query->album, ncm_error);
    }
    if ((status == 0) && query->match_date) {
        status = ncm_mpd_client_add_search_tag(
            client, MPD_TAG_DATE, query->date, ncm_error);
    }
    if (status == 0) {
        status = ncm_mpd_client_commit_search_songs(client, songs,
                                                    ncm_error);
    }

    return status;
}

static int32
library_mpd_list_all_songs(void *user, NcmMpdSongList *songs,
                           NcmError *ncm_error) {
    NcmMpdClient *client = user;

    ASSERT(client != NULL);
    return ncm_mpd_client_get_directory_recursive(client, "/",
                                                  songs, ncm_error);
}

static int32
library_mpd_list_tags(void *user, enum mpd_tag_type tag_type,
                      NcmStringViewList *tags, NcmError *ncm_error) {
    NcmMpdClient *client = user;

    ASSERT(client != NULL);
    return ncm_mpd_client_get_list(client, tag_type, tags, ncm_error);
}

static int32
library_ratio_value(NcmInt32Array *ratios, int32 idx,
                    int32 fallback) {
    ASSERT(ratios != NULL);
    if ((idx < 0) || (idx >= ratios->len)) {
        return fallback;
    }
    if (ratios->items[idx] <= 0) {
        return fallback;
    }
    return ratios->items[idx];
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

    if (screen->width <= 0) {
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

        nc_window_move_to(&screen->tags_window,
                          screen->start_x, screen->main_start_y);
        nc_window_resize(&screen->tags_window,
                         left_width, screen->main_height);
        nc_window_move_to(&screen->albums_window,
                          middle_x, screen->main_start_y);
        nc_window_resize(&screen->albums_window,
                         middle_width, screen->main_height);
        nc_window_move_to(&screen->songs_window,
                          right_x, screen->main_start_y);
        nc_window_resize(&screen->songs_window,
                         right_width, screen->main_height);
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

    nc_window_move_to(&screen->albums_window, middle_x, screen->main_start_y);
    nc_window_resize(&screen->albums_window, middle_width, screen->main_height);
    nc_window_move_to(&screen->songs_window, right_x, screen->main_start_y);
    nc_window_resize(&screen->songs_window, right_width, screen->main_height);

    return;
}

static bool
library_album_matches(MediaLibraryScreen *screen,
                      NcMediaLibraryAlbumRow *row,
                      NcmRegex *regex) {
    StrBuilder text = {0};
    bool result;

    ASSERT(row != NULL);
    media_library_screen_format_album_row(screen, row, &text);
    result = ncm_regex_matches(regex, text.data, text.len);
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
    result = ncm_regex_matches(regex, text.data, text.len);
    sb_free(&text);
    return result;
}

static bool
library_tag_matches(MediaLibraryScreen *screen,
                    NcMediaLibraryTagRow *row, NcmRegex *regex) {
    StrBuilder text = {0};
    bool result;

    ASSERT(row != NULL);
    media_library_screen_format_tag_row(screen, row, &text);
    result = ncm_regex_matches(regex, text.data, text.len);
    sb_free(&text);
    return result;
}

static void
library_refresh_menu(NcMenu *menu, NcWindow *window) {
    nc_window_display(window);
    nc_menu_refresh(menu, window,
                    nc_window_width(window), nc_window_height(window));
    return;
}

static void
library_update_titles(MediaLibraryScreen *screen,
                      bool update_windows) {
    sb_clear(&screen->tags_title);
    sb_clear(&screen->albums_title);
    sb_clear(&screen->songs_title);
    if (Config.titles_visibility) {
        char *tag_type_name = ncm_tag_type_name(Config.media_lib_primary_tag);
        int32 tag_type_name_len = optional_strlen32(tag_type_name);

        SB_APPEND(&screen->tags_title, tag_type_name, tag_type_name_len);
        sb_append_byte(&screen->tags_title, 's');
        SB_APPEND(&screen->albums_title, "Albums");
        SB_APPEND(&screen->songs_title, "Songs");

        if (screen->mode == MEDIA_LIBRARY_MODE_TWO_COLUMNS) {
            SB_APPEND(&screen->albums_title, " (sorted by ");
            for (int32 i = 0; i < tag_type_name_len; i += 1) {
                char ch = tag_type_name[i];

                if ((ch >= 'A') && (ch <= 'Z')) {
                    ch = (char)(ch - 'A' + 'a');
                }
                sb_append_byte(&screen->albums_title, ch);
            }
            if (screen->sort_by_mtime) {
                SB_APPEND(&screen->albums_title, " and mtime");
            }
            sb_append_byte(&screen->albums_title, ')');
        } else if ((screen->mode == MEDIA_LIBRARY_MODE_ALBUM_ONLY)
                   && screen->sort_by_mtime) {
            SB_APPEND(&screen->albums_title, " (sorted by mtime)");
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

static void library_request_all_updates(MediaLibraryScreen *screen);

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

static void
library_tag_array_item_destroy(void *item) {
    nc_media_library_tag_row_destroy(item);
    return;
}

static void
library_album_array_item_init(void *item) {
    MediaLibraryAlbumItem *album = item;

    album->row = (NcMediaLibraryAlbumRow){0};
    album->menu_flags = NC_MENU_ITEM_SELECTABLE;

    return;
}

static void
library_album_array_item_destroy(void *item) {
    MediaLibraryAlbumItem *album = item;

    nc_media_library_album_row_destroy(&album->row);
    album->menu_flags = NC_MENU_ITEM_SELECTABLE;

    return;
}

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

static int32
library_mpd_add_songs(void *user, NcmSongArray *songs, bool play,
                      NcmError *ncm_error) {
    NcmMpdClient *client = user;
    NcmMpdSongList additions = {0};
    int32 status;

    ASSERT(client != NULL);
    ASSERT(songs != NULL);

    for (int32 i = 0; i < songs->len; i += 1) {
        ncm_mpd_song_list_append_copy(&additions, &songs->items[i]);
    }

    {
        int32 play_pos = -1;
        if (play) {
            play_pos = ncm_status_state_playlist_length();
        }
        status = ncm_mpd_client_add_song_list(client, &additions, -1,
                                              ncm_error);
        if ((status == 0) && play) {
            status = ncm_mpd_client_play_pos(client, play_pos, ncm_error);
        }
    }
    ncm_mpd_song_list_destroy(&additions);
    return status;
}

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
    NcBuffer text = {0};

    (void)menu;
    (void)pos;

    media_library_screen_format_song_row(user, item, &text);
    {
        NcBufferProperty *properties = nc_buffer_properties(&text);
        char *data = nc_buffer_data(&text);
        int32 property_count = nc_buffer_property_count(&text);
        int32 property_index = 0;
        int32 len = nc_buffer_len(&text);

        for (int32 i = 0;; i += 1) {
            while ((property_index < property_count)
                   && (properties[property_index].position == i)) {
                nc_buffer_apply_property(
                    window, &properties[property_index]);
                property_index += 1;
            }
            if (i >= len) {
                break;
            }
            nc_window_print_char(window, data[i]);
        }
    }
    nc_buffer_destroy(&text);
    return;
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

static bool
library_album_filter(NcMenu *menu, void *item, void *user) {
    MediaLibraryScreen *screen = user;
    NcMediaLibraryAlbumRow *row = item;

    ASSERT(menu != NULL);
    ASSERT(item != NULL);
    for (int32 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        if ((nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, i) == item)
            && (nc_menu_item_flags_at(menu, NC_MENU_ITEMS_ALL, i)
                & NC_MENU_ITEM_SEPARATOR)) {
            return true;
        }
    }
    if (row->all_tracks_entry) {
        return true;
    }
    return library_album_matches(
        screen, row,
        &screen->column_state[MEDIA_LIBRARY_COLUMN_ALBUMS].filter_regex);
}

static bool
library_song_filter(NcMenu *menu, void *item, void *user) {
    MediaLibraryScreen *screen = user;

    (void)menu;

    return library_song_matches(
        screen, item,
        &screen->column_state[MEDIA_LIBRARY_COLUMN_SONGS].filter_regex);
}

static bool
library_tag_filter(NcMenu *menu, void *item, void *user) {
    MediaLibraryScreen *screen = user;

    (void)menu;
    return library_tag_matches(
        screen, item,
        &screen->column_state[MEDIA_LIBRARY_COLUMN_TAGS].filter_regex);
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
            callbacks.matches_filter = library_tag_filter;
        }
        break;
    case MEDIA_LIBRARY_COLUMN_ALBUMS:
        callbacks.draw = library_draw_album;
        if (filter_enabled) {
            callbacks.matches_filter = library_album_filter;
        }
        break;
    case MEDIA_LIBRARY_COLUMN_SONGS:
        callbacks.draw = library_draw_song;
        if (filter_enabled) {
            callbacks.matches_filter = library_song_filter;
        }
        break;
    case MEDIA_LIBRARY_COLUMN_COUNT:
    default:
        break;
    }
    return callbacks;
}

static void
library_update_menu_highlights(MediaLibraryScreen *screen) {
    NcMenu *tags;
    NcMenu *albums;
    NcMenu *songs;
    NcMenu *active;

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

    screen->update_timer = 0;
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

int32
media_library_screen_set_mode(MediaLibraryScreen *screen,
                              enum MediaLibraryMode mode) {
    if (screen == NULL) {
        return -EINVAL;
    }
    if ((mode < MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        || (mode >= MEDIA_LIBRARY_MODE_COUNT)) {
        return -EINVAL;
    }
    if (screen->mode == mode) {
        return 0;
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
    return 0;
}

int32
media_library_screen_toggle_mode(MediaLibraryScreen *screen,
                                 enum MediaLibraryMode *mode) {
    enum MediaLibraryMode next_mode;

    if (screen == NULL) {
        return -EINVAL;
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
    if (mode) {
        *mode = screen->mode;
    }
    return 0;
}

enum MediaLibraryColumn
media_library_screen_active_column(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return MEDIA_LIBRARY_COLUMN_TAGS;
    }
    return screen->active_column;
}

bool
media_library_screen_has_available_item(MediaLibraryScreen *screen) {
    NcMenu *menu;

    if ((menu = media_library_screen_active_menu(screen)) == NULL) {
        return false;
    }
    return !nc_menu_is_empty(menu);
}

int32
media_library_screen_set_active_column(MediaLibraryScreen *screen,
                                       enum MediaLibraryColumn column) {
    if (screen == NULL) {
        return -EINVAL;
    }
    if ((column < MEDIA_LIBRARY_COLUMN_TAGS)
        || (column >= MEDIA_LIBRARY_COLUMN_COUNT)) {
        return -EINVAL;
    }
    if ((column == MEDIA_LIBRARY_COLUMN_TAGS)
        && (screen->mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS)) {
        return -EINVAL;
    }
    screen->active_column = column;
    library_update_menu_highlights(screen);
    return 0;
}

bool
media_library_screen_column_is_visible(MediaLibraryScreen *screen,
                                       enum MediaLibraryColumn column) {
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
media_library_screen_column_state(MediaLibraryScreen *screen,
                                  enum MediaLibraryColumn column) {
    if (screen == NULL) {
        return NULL;
    }
    if ((column < MEDIA_LIBRARY_COLUMN_TAGS)
        || (column >= MEDIA_LIBRARY_COLUMN_COUNT)) {
        return NULL;
    }
    return &screen->column_state[column];
}

static MediaLibraryColumnState *
library_active_column_state(MediaLibraryScreen *screen) {
    ASSERT(screen != NULL);
    ASSERT(screen->active_column >= MEDIA_LIBRARY_COLUMN_TAGS);
    ASSERT(screen->active_column < MEDIA_LIBRARY_COLUMN_COUNT);
    return &screen->column_state[screen->active_column];
}

StrBuilder *
media_library_screen_active_filter_constraint(MediaLibraryScreen *screen) {
    MediaLibraryColumnState *state;

    state = library_active_column_state(screen);
    return &state->filter_constraint;
}

StrBuilder *
media_library_screen_active_search_constraint(MediaLibraryScreen *screen) {
    MediaLibraryColumnState *state = library_active_column_state(screen);
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
media_library_screen_has_current_primary_tag_value(MediaLibraryScreen *screen,
                                                   char **value,
                                                   int32 *value_len) {
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
media_library_screen_has_current_album_value(
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
media_library_screen_format_tag_row(MediaLibraryScreen *screen,
                                    NcMediaLibraryTagRow *row,
                                    StrBuilder *output) {
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
            SB_APPEND(output, Config.empty_tag, Config.empty_tag_len);
        }
        return;
    }

    SB_APPEND(output, row->tag, row->tag_len);
    return;
}

void
media_library_screen_format_album_row(MediaLibraryScreen *screen,
                                      NcMediaLibraryAlbumRow *row,
                                      StrBuilder *output) {
    StrBuilder raw = {0};

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
                SB_APPEND(&raw, Config.empty_tag, Config.empty_tag_len);
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

    SB_APPEND(output, raw.data, raw.len);
    sb_free(&raw);
    return;
}

void
media_library_screen_format_song_row(MediaLibraryScreen *screen,
                                     NcmSong *song, NcBuffer *output) {
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

static void
library_sort_tags(MediaLibraryTagArray *tags) {
    for (int32 i = 1; i < tags->len; i += 1) {
        int32 j = i;

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
        NcMediaLibraryAlbumRow *row = &albums->items[i].row;

        if (row->all_tracks_entry) {
            continue;
        }

        if (!STREQUAL(row->tag, row->tag_len, tag, tag_len)) {
            continue;
        }
        if (!STREQUAL(row->album, row->album_len, album, album_len)) {
            continue;
        }
        if (!STREQUAL(row->date, row->date_len, date, date_len)) {
            continue;
        }

        return i;
    }
    return -1;
}

static void
library_append_tag(MediaLibraryTagArray *tags,
                   char *tag, int32 tag_len, time_t mtime) {
    NcMediaLibraryTagRow *row;

    row = media_library_tag_array_append(tags);
    stupid_string_set(&row->tag, &row->tag_len, &row->tag_cap,
                             tag, tag_len);
    row->mtime = mtime;
    return;
}

static void
library_append_album(MediaLibraryAlbumArray *albums,
                     char *tag, int32 tag_len,
                     char *album, int32 album_len,
                     char *date, int32 date_len,
                     time_t mtime, bool all_tracks_entry,
                     uint32 menu_flags) {
    MediaLibraryAlbumItem *item;
    NcMediaLibraryAlbumRow *row;

    item = media_library_album_array_append(albums);
    row = &item->row;
    ASSERT((tag != NULL) || (tag_len == 0));
    ASSERT((album != NULL) || (album_len == 0));
    ASSERT((date != NULL) || (date_len == 0));

    if (tag_len > 0) {
        row->tag = xstrndup(tag, tag_len);
        row->tag_len = tag_len;
    }
    if (album_len > 0) {
        row->album = xstrndup(album, album_len);
        row->album_len = album_len;
    }
    if (date_len > 0) {
        row->date = xstrndup(date, date_len);
        row->date_len = date_len;
    }
    row->mtime = mtime;
    row->all_tracks_entry = all_tracks_entry;
    item->menu_flags = menu_flags;
    return;
}

static bool
library_song_has_first_tag(NcmSong *song, enum mpd_tag_type tag,
                       NcmStringView *view) {
    ASSERT(view != NULL);
    *view = (NcmStringView){0};
    return ncm_song_has_tag_view(song, tag, 0, view);
}

int32
media_library_tags_from_strings(MediaLibraryTagArray *tags,
                                NcmStringViewList *strings) {
    MediaLibraryTagArray replacement = {0};

    if ((tags == NULL) || (strings == NULL)) {
        return -EINVAL;
    }

    for (int32 i = 0; i < ncm_mpd_string_list_count(strings); i += 1) {
        NcmStringView *string;

        string = ncm_mpd_string_list_at(strings, i);
        if (library_find_tag(&replacement, string->data, string->len) >= 0) {
            continue;
        }
        library_append_tag(&replacement, string->data, string->len, 0);
    }
    library_sort_tags(&replacement);
    media_library_tag_array_move(tags, &replacement);
    return 0;
}

int32
media_library_tags_from_songs(
    MediaLibraryTagArray *tags, NcmMpdSongList *songs,
    enum mpd_tag_type primary_tag
) {
    MediaLibraryTagArray replacement;

    if ((tags == NULL) || (songs == NULL)
        || (primary_tag == MPD_TAG_UNKNOWN)) {
        return -EINVAL;
    }

    replacement = (MediaLibraryTagArray){0};
    for (int32 i = 0; i < ncm_mpd_song_list_count(songs); i += 1) {
        NcmSong *song;
        NcmStringView primary_value;

        song = ncm_mpd_song_list_at(songs, i);
        for (int32 j = 0;
             ncm_song_has_tag_view(song, primary_tag, j, &primary_value);
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
            library_append_tag(&replacement, primary_value.data,
                               primary_value.len, song->last_modified);
        }
    }
    library_sort_tags(&replacement);
    media_library_tag_array_move(tags, &replacement);
    return 0;
}

int32
media_library_albums_from_songs(
    MediaLibraryAlbumArray *albums, NcmMpdSongList *songs,
    enum MediaLibraryMode mode, enum mpd_tag_type primary_tag,
    char *selected_tag, int32 selected_tag_len
) {
    MediaLibraryAlbumArray replacement = {0};
    MediaLibraryAlbumItem *separator;
    int32 album_count;

    if ((albums == NULL) || (songs == NULL)
        || (mode < MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        || (mode >= MEDIA_LIBRARY_MODE_COUNT)
        || (primary_tag == MPD_TAG_UNKNOWN)
        || (selected_tag_len < 0)
        || ((selected_tag == NULL) && (selected_tag_len > 0))) {
        return -EINVAL;
    }

    for (int32 i = 0; i < ncm_mpd_song_list_count(songs); i += 1) {
        NcmSong *song;

        song = ncm_mpd_song_list_at(songs, i);
        if (mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
            NcmStringView album = {0};
            NcmStringView date = {0};
            int32 existing;

            library_song_has_first_tag(song, MPD_TAG_ALBUM, &album);
            library_song_has_first_tag(song, MPD_TAG_DATE, &date);
            if (!Config.media_library_albums_split_by_date) {
                ncm_string_view_clear(&date);
            }

            existing = library_find_album(
                &replacement, selected_tag, selected_tag_len,
                album.data, album.len, date.data, date.len);
            if (existing >= 0) {
                if (song->last_modified
                    > replacement.items[existing].row.mtime) {
                    replacement.items[existing].row.mtime =
                        song->last_modified;
                }
            } else {
                library_append_album(
                    &replacement, selected_tag, selected_tag_len,
                    album.data, album.len, date.data, date.len,
                    song->last_modified, false, NC_MENU_ITEM_SELECTABLE);
            }
        } else {
            NcmStringView album = {0};
            NcmStringView date = {0};
            NcmStringView primary_value = {0};

            library_song_has_first_tag(song, MPD_TAG_ALBUM, &album);
            library_song_has_first_tag(song, MPD_TAG_DATE, &date);
            if (!Config.media_library_albums_split_by_date) {
                ncm_string_view_clear(&date);
            }

            for (int32 j = 0;
                 ncm_song_has_tag_view(
                     song, primary_tag, j, &primary_value);
                 j += 1) {
                char *tag = primary_value.data;
                int32 tag_len = primary_value.len;
                int32 existing;

                if (mode == MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
                    tag = NULL;
                    tag_len = 0;
                }
                existing = library_find_album(
                    &replacement, tag, tag_len, album.data, album.len,
                    date.data, date.len);
                if (existing >= 0) {
                    replacement.items[existing].row.mtime =
                        song->last_modified;
                    continue;
                }
                library_append_album(
                    &replacement, tag, tag_len, album.data, album.len,
                    date.data, date.len, song->last_modified, false,
                    NC_MENU_ITEM_SELECTABLE);
            }
        }
    }

    for (int32 i = 1; i < replacement.len; i += 1) {
        int32 j = i;

        while ((j > 0)
               && (library_compare_album_items(
                   &replacement.items[j],
                   &replacement.items[j - 1]) < 0)) {
            MediaLibraryAlbumItem tmp = replacement.items[j];

            replacement.items[j] = replacement.items[j - 1];
            replacement.items[j - 1] = tmp;
            j -= 1;
        }
    }

    album_count = replacement.len;
    if ((mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (album_count > 1)) {
        separator = media_library_album_array_append(&replacement);
        separator->menu_flags = NC_MENU_ITEM_SEPARATOR;
        library_append_album(&replacement, selected_tag, selected_tag_len,
                             NULL, 0, NULL, 0, 0, true,
                             NC_MENU_ITEM_SELECTABLE);
    }

    media_library_album_array_move(albums, &replacement);
    return 0;
}

int32
media_library_songs_from_list(NcmSongArray *songs,
                              NcmMpdSongList *source) {
    static enum NcmSongGetter getters[] = {
        NCM_SONG_GETTER_DATE,
        NCM_SONG_GETTER_ALBUM,
        NCM_SONG_GETTER_DISC,
        NCM_SONG_GETTER_TRACK_NUMBER,
    };
    NcmSongArray replacement = {0};

    if ((songs == NULL) || (source == NULL)) {
        return -EINVAL;
    }

    for (int32 i = 0; i < ncm_mpd_song_list_count(source); i += 1) {
        NcmSong *song;

        song = ncm_mpd_song_list_at(source, i);
        ncm_song_array_append_copy(&replacement, song);
    }

    for (int32 i = 1; i < replacement.len; i += 1) {
        int32 j = i;

        while (j > 0) {
            NcmSong *left = &replacement.items[j];
            NcmSong *right = &replacement.items[j - 1];
            int32 result = 0;

            for (int32 k = 0; k < LENGTH(getters); k += 1) {
                StrBuilder left_tags;
                StrBuilder right_tags;
                char *left_data;
                char *right_data;
                char *separator = Config.tags_separator;
                int32 separator_len = Config.tags_separator_len;

                if (separator == NULL) {
                    separator = "";
                    separator_len = 0;
                }
                left_tags = ncm_song_tags_buffer(
                    left, getters[k], separator, separator_len,
                    Config.show_duplicate_tags);
                right_tags = ncm_song_tags_buffer(
                    right, getters[k], separator, separator_len,
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
                    left_data, left_tags.len,
                    right_data, right_tags.len,
                    Config.ignore_leading_the);
                sb_free(&right_tags);
                sb_free(&left_tags);
                if (result != 0) {
                    break;
                }
            }

            if (result == 0) {
                StrBuilder left_text;
                StrBuilder right_text;
                int32 common_len;

                left_text = ncm_format_render_string(
                    &Config.song_library_format, left);
                right_text = ncm_format_render_string(
                    &Config.song_library_format, right);
                common_len = left_text.len;
                if (right_text.len < common_len) {
                    common_len = right_text.len;
                }
                for (int32 k = 0; k < common_len; k += 1) {
                    uint8 left_byte = (uint8)left_text.data[k];
                    uint8 right_byte = (uint8)right_text.data[k];

                    if (left_byte < right_byte) {
                        result = -1;
                        break;
                    }
                    if (left_byte > right_byte) {
                        result = 1;
                        break;
                    }
                }
                if (result == 0) {
                    if (left_text.len < right_text.len) {
                        result = -1;
                    } else if (left_text.len > right_text.len) {
                        result = 1;
                    }
                }
                sb_free(&right_text);
                sb_free(&left_text);
            }

            if (result >= 0) {
                break;
            }
            {
                NcmSong tmp = replacement.items[j];

                replacement.items[j] = replacement.items[j - 1];
                replacement.items[j - 1] = tmp;
            }
            j -= 1;
        }
    }

    ncm_song_array_move(songs, &replacement);
    return 0;
}

int32
media_library_screen_toggle_sort_mode(MediaLibraryScreen *screen,
                                      bool *enabled) {
    if (screen == NULL) {
        return -EINVAL;
    }

    screen->sort_by_mtime = !screen->sort_by_mtime;
    Config.media_library_sort_by_mtime = screen->sort_by_mtime;
    library_update_titles(screen, true);
    media_library_screen_request_tags_update(screen);
    media_library_screen_request_albums_update(screen);
    media_library_screen_request_songs_update(screen);
    if (enabled) {
        *enabled = screen->sort_by_mtime;
    }
    return 0;
}

static void
library_set_observed_tag(MediaLibraryScreen *screen,
                         NcMediaLibraryTagRow *tag) {
    nc_media_library_tag_row_destroy(&screen->observed_tag);
    screen->observed_tag = (NcMediaLibraryTagRow){0};
    screen->observed_tag_valid = false;
    if (tag) {
        nc_media_library_tag_row_copy(&screen->observed_tag, tag);
        screen->observed_tag_valid = true;
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
        nc_media_library_album_row_copy(&screen->observed_album, album);
        screen->observed_album_valid = true;
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

int32
media_library_screen_set_primary_tag_type(MediaLibraryScreen *screen,
                                          enum mpd_tag_type tag_type) {
    if ((screen == NULL) || (tag_type == MPD_TAG_UNKNOWN)) {
        return -EINVAL;
    }

    if (Config.media_lib_primary_tag == tag_type) {
        library_update_titles(screen, true);
        return 0;
    }

    Config.media_lib_primary_tag = tag_type;
    nc_menu_clear_items(nc_media_library_tag_menu_base(&screen->tags));
    nc_menu_clear_items(nc_media_library_album_menu_base(&screen->albums));
    nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));
    library_reset_observed_highlights(screen);
    library_update_titles(screen, true);
    media_library_screen_request_tags_update(screen);
    nc_screen_finish_list_change(&screen->screen);
    return 0;
}

void
media_library_screen_request_database_update(MediaLibraryScreen *screen) {
    library_request_all_updates(screen);
    return;
}

int32
media_library_screen_refresh_inactive_songs(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return -EINVAL;
    }
    if (!media_library_screen_column_is_visible(
        screen, MEDIA_LIBRARY_COLUMN_SONGS)) {
        return 0;
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS) {
        return 0;
    }

    library_refresh_menu(
        nc_media_library_song_menu_base(&screen->songs),
        &screen->songs_window);
    return 1;
}

static NcMenu *
library_column_menu(MediaLibraryScreen *screen,
                    enum MediaLibraryColumn column) {
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
library_column_has_visible_items(MediaLibraryScreen *screen,
                                 enum MediaLibraryColumn column) {
    NcMenu *menu;

    if (!media_library_screen_column_is_visible(screen, column)) {
        return false;
    }
    menu = library_column_menu(screen, column);
    return nc_menu_all_item_count(menu) > 0;
}

bool
media_library_screen_can_move_to_previous_column(MediaLibraryScreen *screen) {
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
media_library_screen_can_move_to_next_column(MediaLibraryScreen *screen) {
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
media_library_screen_previous_column(MediaLibraryScreen *screen) {
    if (!media_library_screen_can_move_to_previous_column(screen)) {
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
    if (!media_library_screen_can_move_to_next_column(screen)) {
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

int32
media_library_screen_current_song(MediaLibraryScreen *screen,
                                  NcmSong *song) {
    NcmSong *current;

    if ((screen == NULL) || (song == NULL)) {
        return -EINVAL;
    }
    if ((current = nc_media_library_song_menu_current(&screen->songs))
        == NULL) {
        return 0;
    }
    ncm_song_copy(song, current);
    return 1;
}

int32
media_library_screen_selected_songs(MediaLibraryScreen *screen,
                                    NcmSongArray *songs) {
    NcmError ncm_error;
    int32 status;

    ncm_error_clear(&ncm_error);
    status = media_library_screen_selected_songs_checked(
        screen, songs, &ncm_error);
    if ((status < 0) && ncm_error_is_set(&ncm_error)) {
        ncm_statusbar_print_cstring(
            Config.message_delay_time, ncm_error.message);
    }
    return status;
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

static int32
library_append_query_songs(MediaLibraryScreen *screen,
                           MediaLibrarySongQuery *query,
                           NcmSongArray *songs, NcmError *ncm_error) {
    NcmMpdSongList source = {0};
    NcmSongArray sorted = {0};
    int32 status;

    status = media_library_screen_search_songs(screen, query, &source,
                                               ncm_error);
    if (status == 0) {
        media_library_songs_from_list(&sorted, &source);
        for (int32 i = 0; i < sorted.len; i += 1) {
            ncm_song_array_append_copy(songs, &sorted.items[i]);
        }
    }
    if (status > 0) {
        status = 0;
    }
    ncm_song_array_destroy(&sorted);
    ncm_mpd_song_list_destroy(&source);
    return status;
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

static void
library_copy_song_at(MediaLibraryScreen *screen,
                     NcmSongArray *songs, int32 pos) {
    NcmSong *song;

    song = nc_menu_active_item_at(
        nc_media_library_song_menu_base(&screen->songs), pos);
    if (song == NULL) {
        return;
    }
    ncm_song_array_append_copy(songs, song);
    return;
}

int32
media_library_screen_selected_songs_checked(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error
) {
    if ((screen == NULL) || (songs == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing media-library songs"));
    }

    if (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS) {
        NcMenu *menu = nc_media_library_tag_menu_base(&screen->tags);
        bool any_selected = nc_menu_has_selected(menu);

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            MediaLibrarySongQuery query = {0};
            NcMediaLibraryTagRow *row;
            int32 status;

            if (any_selected && !nc_menu_position_is_selected(menu, i)) {
                continue;
            }
            if (!any_selected && (i != nc_menu_highlight(menu))) {
                continue;
            }
            row = nc_menu_active_item_at(menu, i);
            library_query_from_tag(screen, row, &query);
            status = library_append_query_songs(
                screen, &query, songs, ncm_error);
            if (status < 0) {
                return status;
            }
        }
        return 0;
    }

    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        NcMenu *menu = nc_media_library_album_menu_base(&screen->albums);
        bool any_selected = nc_menu_has_selected(menu);

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            MediaLibrarySongQuery query = {0};
            NcMediaLibraryAlbumRow *row;
            int32 status;

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
            library_query_from_album(screen, row, &query);
            status = library_append_query_songs(
                screen, &query, songs, ncm_error);
            if (status < 0) {
                return status;
            }
        }
        return 0;
    }

    {
        NcMenu *menu = nc_media_library_song_menu_base(&screen->songs);
        bool any_selected = nc_menu_has_selected(menu);

        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            if (any_selected && !nc_menu_position_is_selected(menu, i)) {
                continue;
            }
            if (!any_selected && (i != nc_menu_highlight(menu))) {
                continue;
            }
            library_copy_song_at(screen, songs, i);
        }
    }
    return 0;
}

int32
media_library_screen_copy_visible_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error
) {
    NcMenu *menu;

    if ((screen == NULL) || (songs == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing media-library songs"));
    }

    menu = nc_media_library_song_menu_base(&screen->songs);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        library_copy_song_at(screen, songs, i);
    }
    return 0;
}

int32
media_library_screen_apply_filter(
    MediaLibraryScreen *screen, char *pattern, int32 pattern_len,
    NcmError *ncm_error
) {
    MediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;
    NcMenu *menu;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing media library"));
    }
    if (pattern_len <= 0) {
        media_library_screen_clear_filter(screen);
        return ncm_error_ok(ncm_error);
    }
    if (pattern == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing filter pattern"));
    }

    state = library_active_column_state(screen);
    if ((status = ncm_regex_compile(&state->filter_regex,
                                    pattern, pattern_len,
                                    Config.regex_flags, ncm_error)) < 0) {
        return status;
    }
    sb_set(&state->filter_constraint, pattern, pattern_len);

    callbacks = library_display_callbacks(
        screen, screen->active_column, true);
    menu = media_library_screen_active_menu(screen);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_apply_filter(menu);
    state->filter_enabled = true;
    nc_screen_finish_list_change(&screen->screen);
    return ncm_error_ok(ncm_error);
}

void
media_library_screen_clear_filter(MediaLibraryScreen *screen) {
    MediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;
    NcMenu *menu;

    if (screen == NULL) {
        return;
    }

    state = library_active_column_state(screen);
    ncm_regex_destroy(&state->filter_regex);
    state->filter_regex = (NcmRegex){0};
    sb_clear(&state->filter_constraint);
    menu = media_library_screen_active_menu(screen);
    callbacks = library_display_callbacks(screen, screen->active_column, false);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_show_all_items(menu);
    state->filter_enabled = false;
    nc_screen_finish_list_change(&screen->screen);

    return;
}

static bool
library_search_position(NcMenu *menu, int32 pos, void *user) {
    MediaLibrarySearchContext *context = user;
    MediaLibraryScreen *screen = context->screen;
    NcmRegex *regex = context->regex;
    void *item = nc_menu_active_item_at(menu, pos);

    ASSERT(item != NULL);
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        NcMediaLibraryAlbumRow *album = item;

        if (nc_menu_position_is_separator(menu, pos)
            || album->all_tracks_entry) {
            return false;
        }
        return library_album_matches(screen, album, regex);
    }
    if (screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS) {
        return library_song_matches(screen, item, regex);
    }
    return library_tag_matches(screen, item, regex);
}

int32
media_library_screen_search(MediaLibraryScreen *screen,
                            char *pattern, int32 pattern_len,
                            bool forward, bool wrap,
                            bool skip_current, NcmError *ncm_error) {
    MediaLibrarySearchContext context;
    MediaLibraryColumnState *state;
    NcMenu *menu;
    bool found;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing media library"));
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing search pattern"));
    }

    state = library_active_column_state(screen);
    if ((status = ncm_regex_compile(&state->search_regex,
                                    pattern, pattern_len,
                                    Config.regex_flags, ncm_error)) < 0) {
        return status;
    }
    sb_set(&state->search_constraint, pattern, pattern_len);
    state->search_enabled = true;

    menu = media_library_screen_active_menu(screen);
    context.screen = screen;
    context.regex = &state->search_regex;
    found = nc_menu_search_selectable(menu, screen->main_height, forward,
                                      wrap, skip_current,
                                      library_search_position,
                                      &context, NULL) == 0;
    if (found) {
        nc_screen_finish_list_change(&screen->screen);
        return 1;
    }
    return 0;
}

void
media_library_screen_clear_search(MediaLibraryScreen *screen) {
    MediaLibraryColumnState *state;

    if (screen == NULL) {
        return;
    }
    state = library_active_column_state(screen);
    ncm_regex_destroy(&state->search_regex);
    state->search_regex = (NcmRegex){0};
    sb_clear(&state->search_constraint);
    state->search_enabled = false;
    return;
}

void
media_library_screen_request_tags_update(MediaLibraryScreen *screen) {
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
media_library_screen_request_albums_update(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->albums_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
media_library_screen_request_songs_update(MediaLibraryScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->songs_update_request = true;
    nc_screen_request_update(&screen->screen);
    return;
}

static bool
library_album_identity_is_equal(NcMediaLibraryAlbumRow *left,
                                NcMediaLibraryAlbumRow *right) {
    return left->all_tracks_entry == right->all_tracks_entry
           && STREQUAL(left->tag, left->tag_len,
                       right->tag, right->tag_len)
           && STREQUAL(left->album, left->album_len,
                       right->album, right->album_len)
           && STREQUAL(left->date, left->date_len,
                       right->date, right->date_len);
}

static void
library_restart_update_timer(MediaLibraryScreen *screen) {
    screen->update_timer = global_timer;
    return;
}

static bool
library_tag_identity_is_equal(NcMediaLibraryTagRow *left,
                              NcMediaLibraryTagRow *right) {
    return STREQUAL(left->tag, left->tag_len, right->tag, right->tag_len);
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
        tag_changed = !library_tag_identity_is_equal(&screen->observed_tag,
                                                     tag);
    }
    album_changed = screen->observed_album_valid != album_valid;
    if (!album_changed && album_valid) {
        album_changed
            = !library_album_identity_is_equal(&screen->observed_album, album);
    }

    if (tag_changed) {
        nc_menu_clear_items(nc_media_library_album_menu_base(&screen->albums));
        nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));
        library_restart_update_timer(screen);
        library_set_observed_tag(screen, tag);
        library_set_observed_album(screen, NULL);
        nc_screen_request_update(&screen->screen);
        return;
    }

    if (album_changed) {
        nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));
        library_restart_update_timer(screen);
        library_set_observed_album(screen, album);
        nc_screen_request_update(&screen->screen);
        return;
    }

    library_set_observed_tag(screen, tag);
    library_set_observed_album(screen, album);
    return;
}

static bool
library_has_pending_albums(MediaLibraryScreen *screen) {
    NcMenu *albums;

    if ((screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && (media_library_screen_current_tag(screen) == NULL)) {
        return false;
    }
    albums = nc_media_library_album_menu_base(&screen->albums);
    return screen->albums_update_request
           || (nc_menu_all_item_count(albums) <= 0);
}

static void
library_apply_column_filter(MediaLibraryScreen *screen,
                            enum MediaLibraryColumn column,
                            NcMenu *menu) {
    MediaLibraryColumnState *state;
    NcMenuDisplayCallbacks callbacks;

    state = media_library_screen_column_state(screen, column);
    ASSERT(state != NULL);
    ASSERT(menu != NULL);

    callbacks = library_display_callbacks(screen, column,
                                          state->filter_enabled);
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

    if ((count = nc_menu_item_count(menu)) <= 0) {
        return;
    }
    if (highlight < 0) {
        highlight = 0;
    }
    if (highlight >= count) {
        highlight = count - 1;
    }
    nc_menu_goto_selectable(menu, highlight);
    return;
}

static bool
library_has_pending_songs(MediaLibraryScreen *screen) {
    NcMenu *songs;

    if (media_library_screen_current_album(screen) == NULL) {
        return false;
    }
    songs = nc_media_library_song_menu_base(&screen->songs);
    return screen->songs_update_request
           || (nc_menu_all_item_count(songs) <= 0);
}

static bool
library_has_pending_tags(MediaLibraryScreen *screen) {
    NcMenu *tags;

    if (screen->mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
        return false;
    }
    tags = nc_media_library_tag_menu_base(&screen->tags);
    return screen->tags_update_request
           || (nc_menu_all_item_count(tags) <= 0);
}

static bool
library_has_fetch_delay_elapsed(MediaLibraryScreen *screen) {
    ASSERT(screen != NULL);
    if (screen->fetching_delay_ms < 0) {
        return true;
    }
    return global_timer_elapsed_ms(screen->update_timer)
           > screen->fetching_delay_ms;
}

int32
media_library_screen_update(MediaLibraryScreen *screen,
                            NcmError *ncm_error) {
    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing media library"));
    }
    ncm_error_clear(ncm_error);

    if (library_has_pending_tags(screen)) {
        MediaLibraryTagArray tags = {0};
        NcmStringViewList strings = {0};
        NcmMpdSongList songs = {0};
        int32 status;

        if (screen->sort_by_mtime) {
            status = media_library_screen_list_all_songs(
                screen, &songs, ncm_error);
            if (status < 0) {
                bool ignored;

                screen->tags_update_request = true;
                if (screen->sort_by_mtime) {
                    media_library_screen_toggle_sort_mode(screen, &ignored);
                }
                ncm_mpd_song_list_destroy(&songs);
                ncm_mpd_string_list_destroy(&strings);
                media_library_tag_array_destroy(&tags);
                return status;
            }
            media_library_tags_from_songs(
                &tags, &songs, Config.media_lib_primary_tag);
        } else {
            status = media_library_screen_list_tags(
                screen, Config.media_lib_primary_tag, &strings, ncm_error);
            if (status < 0) {
                screen->tags_update_request = true;
                ncm_mpd_song_list_destroy(&songs);
                ncm_mpd_string_list_destroy(&strings);
                media_library_tag_array_destroy(&tags);
                return status;
            }
            media_library_tags_from_strings(&tags, &strings);
        }

        {
            NcMediaLibraryTagMenu replacement;
            NcMediaLibraryTagRow identity = {0};
            NcMediaLibraryTagRow *current;
            NcMenu *menu;
            NcMenu *replacement_menu;
            int32 highlight;
            bool identity_valid;

            menu = nc_media_library_tag_menu_base(&screen->tags);
            current = nc_media_library_tag_menu_current(&screen->tags);
            highlight = nc_menu_highlight(menu);
            identity_valid = false;

            if (current) {
                nc_media_library_tag_row_copy(&identity, current);
                identity_valid = true;
                library_set_observed_tag(screen, current);
            } else {
                library_set_observed_tag(screen, NULL);
            }

            nc_media_library_tag_menu_init(&replacement);
            replacement_menu = nc_media_library_tag_menu_base(&replacement);
            nc_menu_copy(replacement_menu, menu);
            for (int32 i = 0; i < tags.len; i += 1) {
                nc_media_library_tag_menu_add(
                    &replacement, &tags.items[i]);
            }
            library_apply_column_filter(
                screen, MEDIA_LIBRARY_COLUMN_TAGS, replacement_menu);
            {
                NcMenu *base =
                    nc_media_library_tag_menu_base(&replacement);
                bool restored = false;

                if (identity_valid) {
                    for (int32 i = 0; i < nc_menu_item_count(base);
                         i += 1) {
                        NcMediaLibraryTagRow *candidate;

                        candidate = nc_media_library_tag_menu_item_at(
                            &replacement, base->active_items, i);
                        if (library_tag_identity_is_equal(
                                candidate, &identity)
                            && (nc_menu_goto_selectable(base, i) == 0)) {
                            restored = true;
                            break;
                        }
                    }
                }
                if (!restored) {
                    library_restore_highlight(base, highlight);
                }
            }

            nc_menu_swap(menu, replacement_menu);
            nc_media_library_tag_menu_destroy(&replacement);
            nc_media_library_tag_row_destroy(&identity);
            nc_screen_finish_list_change(&screen->screen);
            screen->tags_update_request = false;
        }

        ncm_mpd_song_list_destroy(&songs);
        ncm_mpd_string_list_destroy(&strings);
        media_library_tag_array_destroy(&tags);
        return 0;
    }

    if (library_has_pending_albums(screen)
        && ((screen->mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS)
            || screen->albums_update_request
            || library_has_fetch_delay_elapsed(screen))) {
        MediaLibraryAlbumArray albums = {0};
        MediaLibrarySongQuery query = {0};
        NcMediaLibraryTagRow *tag;
        NcmMpdSongList songs = {0};
        char *selected_tag = NULL;
        int32 selected_tag_len = 0;
        int32 status;

        if (screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS) {
            if ((tag = media_library_screen_current_tag(screen)) == NULL) {
                ncm_mpd_song_list_destroy(&songs);
                media_library_album_array_destroy(&albums);
                return 0;
            }
            selected_tag = tag->tag;
            selected_tag_len = tag->tag_len;
            query.primary_value = tag->tag;
            query.primary_value_len = tag->tag_len;
            query.primary_tag = Config.media_lib_primary_tag;
            query.match_primary_tag = true;
            status = media_library_screen_search_songs(
                screen, &query, &songs, ncm_error);
        } else {
            status = media_library_screen_list_all_songs(
                screen, &songs, ncm_error);
            if (status < 0) {
                media_library_screen_toggle_mode(screen, NULL);
                library_request_all_updates(screen);
            }
        }

        if (status < 0) {
            screen->albums_update_request = true;
            ncm_mpd_song_list_destroy(&songs);
            media_library_album_array_destroy(&albums);
            return status;
        }

        media_library_albums_from_songs(
            &albums, &songs, screen->mode, Config.media_lib_primary_tag,
            selected_tag, selected_tag_len);
        {
            NcMediaLibraryAlbumMenu replacement;
            NcMediaLibraryAlbumRow identity = {0};
            NcMediaLibraryAlbumRow *current;
            NcMenu *menu;
            NcMenu *replacement_menu;
            int32 highlight;
            bool identity_valid;

            menu = nc_media_library_album_menu_base(&screen->albums);
            current = nc_media_library_album_menu_current(&screen->albums);
            highlight = nc_menu_highlight(menu);
            identity_valid = false;

            if (current) {
                nc_media_library_album_row_copy(&identity, current);
                identity_valid = true;
                library_set_observed_album(screen, current);
            } else {
                library_set_observed_album(screen, NULL);
            }

            nc_media_library_album_menu_init(&replacement);
            replacement_menu =
                nc_media_library_album_menu_base(&replacement);
            nc_menu_copy(replacement_menu, menu);
            for (int32 i = 0; i < albums.len; i += 1) {
                nc_media_library_album_menu_add_with_flags(
                    &replacement, &albums.items[i].row,
                    albums.items[i].menu_flags);
            }
            library_apply_column_filter(
                screen, MEDIA_LIBRARY_COLUMN_ALBUMS, replacement_menu);
            {
                NcMenu *base =
                    nc_media_library_album_menu_base(&replacement);
                bool restored = false;

                if (identity_valid) {
                    for (int32 i = 0; i < nc_menu_item_count(base);
                         i += 1) {
                        NcMediaLibraryAlbumRow *candidate;

                        candidate = nc_media_library_album_menu_item_at(
                            &replacement, base->active_items, i);
                        if (library_album_identity_is_equal(
                                candidate, &identity)
                            && (nc_menu_goto_selectable(base, i) == 0)) {
                            restored = true;
                            break;
                        }
                    }
                }
                if (!restored) {
                    library_restore_highlight(base, highlight);
                }
            }

            nc_menu_swap(menu, replacement_menu);
            nc_media_library_album_menu_destroy(&replacement);
            nc_media_library_album_row_destroy(&identity);
            nc_screen_finish_list_change(&screen->screen);
            screen->albums_update_request = false;
        }

        ncm_mpd_song_list_destroy(&songs);
        media_library_album_array_destroy(&albums);
        return 0;
    }

    if (library_has_pending_songs(screen)
        && (screen->songs_update_request
            || library_has_fetch_delay_elapsed(screen))) {
        MediaLibrarySongQuery query = {0};
        NcMediaLibraryAlbumRow *album;
        NcmMpdSongList source = {0};
        NcmSongArray songs = {0};
        int32 status;

        if ((album = media_library_screen_current_album(screen)) == NULL) {
            return 0;
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

        status = media_library_screen_search_songs(
            screen, &query, &source, ncm_error);
        if (status < 0) {
            screen->songs_update_request = true;
            ncm_song_array_destroy(&songs);
            ncm_mpd_song_list_destroy(&source);
            return status;
        }

        media_library_songs_from_list(&songs, &source);
        {
            NcMediaLibrarySongMenu replacement;
            NcmSong identity = {0};
            NcmSong *current;
            NcMenu *menu;
            NcMenu *replacement_menu;
            int32 highlight;
            bool identity_valid;

            menu = nc_media_library_song_menu_base(&screen->songs);
            current = nc_media_library_song_menu_current(&screen->songs);
            highlight = nc_menu_highlight(menu);
            identity_valid = false;
            if (current) {
                ncm_song_copy(&identity, current);
                identity_valid = true;
            }

            nc_media_library_song_menu_init(&replacement);
            replacement_menu =
                nc_media_library_song_menu_base(&replacement);
            nc_menu_copy(replacement_menu, menu);
            for (int32 i = 0; i < songs.len; i += 1) {
                nc_media_library_song_menu_add(
                    &replacement, &songs.items[i]);
            }
            library_apply_column_filter(
                screen, MEDIA_LIBRARY_COLUMN_SONGS, replacement_menu);
            {
                NcMenu *base =
                    nc_media_library_song_menu_base(&replacement);
                bool restored = false;

                if (identity_valid) {
                    for (int32 i = 0; i < nc_menu_item_count(base);
                         i += 1) {
                        NcmSong *candidate;

                        candidate = nc_media_library_song_menu_item_at(
                            &replacement, base->active_items, i);
                        if (ncm_song_is_equal(candidate, &identity)
                            && (nc_menu_goto_selectable(base, i) == 0)) {
                            restored = true;
                            break;
                        }
                    }
                }
                if (!restored) {
                    library_restore_highlight(base, highlight);
                }
            }

            nc_menu_swap(menu, replacement_menu);
            nc_media_library_song_menu_destroy(&replacement);
            ncm_song_destroy(&identity);
            screen->songs_update_request = false;
        }

        ncm_song_array_destroy(&songs);
        ncm_mpd_song_list_destroy(&source);
        return 0;
    }

    if (!library_has_pending_tags(screen)
        && !library_has_pending_albums(screen)
        && !library_has_pending_songs(screen)) {
        nc_screen_clear_update_request(&screen->screen);
    }
    return 0;
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

    state = media_library_screen_column_state(screen, column);
    menu = library_column_menu(screen, column);
    ASSERT(state != NULL);
    ASSERT(menu != NULL);

    ncm_regex_destroy(&state->filter_regex);
    state->filter_regex = (NcmRegex){0};
    sb_clear(&state->filter_constraint);
    callbacks = library_display_callbacks(screen, column, false);
    nc_menu_set_display_callbacks(menu, callbacks);
    nc_menu_show_all_items(menu);
    state->filter_enabled = false;
    return;
}

static int32
library_move_to_tag(MediaLibraryScreen *screen,
                    char *tag, int32 tag_len) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    ASSERT_NON_NEGATIVE(tag_len);
    ASSERT((tag != NULL) || (tag_len == 0));

    menu = nc_media_library_tag_menu_base(&screen->tags);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMediaLibraryTagRow *row;

        row = nc_menu_active_item_at(menu, i);
        ASSERT(row != NULL);
        if (STREQUAL(row->tag, row->tag_len, tag, tag_len)) {
            nc_menu_goto_selectable(menu, i);
            return 1;
        }
    }
    return 0;
}

static int32
library_move_to_album(MediaLibraryScreen *screen,
                      char *tag, int32 tag_len,
                      char *album, int32 album_len,
                      char *date, int32 date_len,
                      bool consider_date) {
    NcMenu *menu;

    ASSERT(screen != NULL);
    ASSERT_NON_NEGATIVE(tag_len);
    ASSERT_NON_NEGATIVE(album_len);
    ASSERT_NON_NEGATIVE(date_len);
    ASSERT((tag != NULL) || (tag_len == 0));
    ASSERT((album != NULL) || (album_len == 0));
    ASSERT((date != NULL) || (date_len == 0));

    menu = nc_media_library_album_menu_base(&screen->albums);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcMediaLibraryAlbumRow *row;
        bool tag_matches;
        bool date_matches;

        if (nc_menu_position_is_separator(menu, i)) {
            continue;
        }

        row = nc_menu_active_item_at(menu, i);
        ASSERT(row != NULL);
        tag_matches = (screen->mode == MEDIA_LIBRARY_MODE_ALBUM_ONLY)
                      || (screen->mode
                          == MEDIA_LIBRARY_MODE_THREE_COLUMNS)
                      || STREQUAL(row->tag, row->tag_len, tag, tag_len);
        if (!tag_matches) {
            continue;
        }
        if (!STREQUAL(row->album, row->album_len, album, album_len)) {
            continue;
        }

        date_matches = !consider_date
                       || !Config.media_library_albums_split_by_date
                       || STREQUAL(row->date, row->date_len, date, date_len);
        if (date_matches) {
            nc_menu_goto_selectable(menu, i);
            return 1;
        }
    }

    if (consider_date) {
        return library_move_to_album(
            screen, tag, tag_len, album, album_len, date, date_len, false);
    }
    return 0;
}

int32
media_library_screen_list_tags(
    MediaLibraryScreen *screen, enum mpd_tag_type tag_type,
    NcmStringViewList *tags, NcmError *ncm_error
) {
    if ((screen == NULL) || (screen->hooks.list_tags == NULL)) {
        return ncm_error_set_status(ncm_error, -ENOSYS,
                                    STRLIT("tag hook is unavailable"));
    }
    if (tags == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing tag list"));
    }
    return screen->hooks.list_tags(screen->hooks.user, tag_type, tags,
                                   ncm_error);
}

int32
media_library_screen_list_all_songs(
    MediaLibraryScreen *screen, NcmMpdSongList *songs,
    NcmError *ncm_error
) {
    if ((screen == NULL) || (screen->hooks.list_all_songs == NULL)) {
        return ncm_error_set_status(
            ncm_error, -ENOSYS, STRLIT("song-list hook is unavailable"));
    }
    if (songs == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing song list"));
    }
    return screen->hooks.list_all_songs(screen->hooks.user, songs, ncm_error);
}

int32
media_library_screen_search_songs(
    MediaLibraryScreen *screen,
    MediaLibrarySongQuery *query, NcmMpdSongList *songs,
    NcmError *ncm_error
) {
    if ((screen == NULL) || (screen->hooks.search_songs == NULL)) {
        return ncm_error_set_status(
            ncm_error, -ENOSYS, STRLIT("song-search hook is unavailable"));
    }
    if ((query == NULL) || (songs == NULL)) {
        return ncm_error_set_status(
            ncm_error, -EINVAL, STRLIT("missing song search arguments"));
    }
    if ((query->match_primary_tag && (query->primary_value == NULL))
        || (query->match_album && (query->album == NULL))
        || (query->match_date && (query->date == NULL))) {
        return ncm_error_set_status(
            ncm_error, -EINVAL, STRLIT("missing song search value"));
    }
    return screen->hooks.search_songs(screen->hooks.user, query, songs,
                                      ncm_error);
}

int32
media_library_screen_add_songs(
    MediaLibraryScreen *screen, NcmSongArray *songs, bool play,
    NcmError *ncm_error
) {
    if ((screen == NULL) || (screen->hooks.add_songs == NULL)) {
        return ncm_error_set_status(
            ncm_error, -ENOSYS, STRLIT("add-songs hook is unavailable"));
    }
    if ((songs == NULL) || (songs->len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing songs"));
    }
    return screen->hooks.add_songs(screen->hooks.user, songs, play,
                                   ncm_error);
}

int32
media_library_screen_add_item_to_playlist(
    MediaLibraryScreen *screen, bool play, NcmError *ncm_error
) {
    NcmSongArray songs = {0};
    int32 status = 0;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing media library"));
    }

    if (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS) {
        MediaLibrarySongQuery query = {0};
        NcMediaLibraryTagRow *tag;

        if ((tag = media_library_screen_current_tag(screen))) {
            library_query_from_tag(screen, tag, &query);
            status = library_append_query_songs(
                screen, &query, &songs, ncm_error);
        }
    } else if (screen->active_column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        MediaLibrarySongQuery query = {0};
        NcMediaLibraryAlbumRow *album;

        if ((album = media_library_screen_current_album(screen))) {
            library_query_from_album(screen, album, &query);
            status = library_append_query_songs(
                screen, &query, &songs, ncm_error);
        }
    } else {
        NcMenu *menu = nc_media_library_song_menu_base(&screen->songs);

        library_copy_song_at(screen, &songs, nc_menu_highlight(menu));
    }

    if ((status == 0) && (songs.len <= 0)) {
        status = ncm_error_set_status(ncm_error, -ENOENT,
                                      STRLIT("no selected songs"));
    }
    if (status == 0) {
        status = media_library_screen_add_songs(
            screen, &songs, play, ncm_error);
    }

    if ((status == 0) || (songs.len > 0)) {
        NcMediaLibraryAlbumRow *album;
        StrBuilder message = {0};
        char *tag_name;
        bool result = status == 0;

        if (screen->active_column == MEDIA_LIBRARY_COLUMN_TAGS) {
            NcMediaLibraryTagRow *tag =
                media_library_screen_current_tag(screen);

            tag_name = ncm_tag_type_name(Config.media_lib_primary_tag);

            SB_APPEND(&message, "Songs with ");
            for (int32 i = 0; tag_name[i] != '\0'; i += 1) {
                char ch = tag_name[i];

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
                    char ch = tag_name[i];

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
                SB_APPEND(&message, "Songs from album \"");
                if (album && album->album) {
                    SB_APPEND(&message, album->album, album->album_len);
                }
                SB_APPEND(&message, "\" added");
            }
            SB_APPEND(&message, ncm_helpers_with_errors(result),
                      optional_strlen32(ncm_helpers_with_errors(result)));
        } else if (result && (songs.len == 1)) {
            StrBuilder rendered = ncm_format_render_string(
                &Config.song_status_format, &songs.items[0]);

            SB_APPEND(&message, "Added to playlist: ");
            SB_APPEND(&message, rendered.data, rendered.len);
            sb_free(&rendered);
        } else if (result) {
            SB_APPEND(&message, "Songs added");
            SB_APPEND(&message, ncm_helpers_with_errors(result),
                      optional_strlen32(ncm_helpers_with_errors(result)));
        }

        if (message.len > 0) {
            ncm_statusbar_print(Config.message_delay_time,
                                message.data, message.len);
        }
        sb_free(&message);
    }

    ncm_song_array_destroy(&songs);
    return status;
}

int32
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
    int32 status;

    if ((screen == NULL) || (song == NULL)) {
        return ncm_error_set_status(
            ncm_error, -EINVAL, STRLIT("missing locate-song argument"));
    }
    ncm_error_clear(ncm_error);
    if (!ncm_song_has_tag_view(song, Config.media_lib_primary_tag,
                               0, &primary_value)
        || (primary_value.len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("song has no primary tag"));
    }
    if (!ncm_song_is_from_database(song)) {
        return ncm_error_set_status(
            ncm_error, -EINVAL, STRLIT("song is not from the database"));
    }

    library_song_has_first_tag(song, MPD_TAG_ALBUM, &album);
    library_song_has_first_tag(song, MPD_TAG_DATE, &date);
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
        library_clear_column_filter(screen, MEDIA_LIBRARY_COLUMN_TAGS);
        if (nc_menu_is_empty(tags_menu)) {
            media_library_screen_request_tags_update(screen);
            status = media_library_screen_update(screen, ncm_error);
            if (status < 0) {
                return status;
            }
        }
        if (!library_move_to_tag(screen, primary_value.data,
                                 primary_value.len)) {
            NcMediaLibraryTagRow row = {0};
            NcMenu *base;

            stupid_string_set(
                &row.tag, &row.tag_len, &row.tag_cap,
                primary_value.data, primary_value.len);
            row.mtime = song->last_modified;
            nc_media_library_tag_menu_add(&screen->tags, &row);
            base = nc_media_library_tag_menu_base(&screen->tags);
            for (int32 i = 1; i < nc_menu_all_item_count(base); i += 1) {
                int32 j = i;

                while (j > 0) {
                    NcMediaLibraryTagRow *left;
                    NcMediaLibraryTagRow *right;

                    left = nc_media_library_tag_menu_item_at(
                        &screen->tags, NC_MENU_ITEMS_ALL, j);
                    right = nc_media_library_tag_menu_item_at(
                        &screen->tags, NC_MENU_ITEMS_ALL, j - 1);
                    ASSERT(left != NULL);
                    ASSERT(right != NULL);
                    if (library_compare_tag_rows(left, right) >= 0) {
                        break;
                    }
                    nc_menu_swap_item_slots(
                        base, NC_MENU_ITEMS_ALL, j, j - 1);
                    j -= 1;
                }
            }
            nc_menu_show_all_items(base);
            nc_media_library_tag_row_destroy(&row);

            if (!library_move_to_tag(screen, primary_value.data,
                                     primary_value.len)) {
                return ncm_error_set_status(
                    ncm_error, -ENOENT,
                    STRLIT("song tag is not in library"));
            }
        }
        nc_screen_finish_list_change(&screen->screen);
    }

    library_clear_column_filter(screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
    if (nc_menu_is_empty(albums_menu)) {
        media_library_screen_request_albums_update(screen);
        status = media_library_screen_update(screen, ncm_error);
        if (status < 0) {
            return status;
        }
    }

    if ((screen->mode == MEDIA_LIBRARY_MODE_THREE_COLUMNS)
        && nc_menu_is_empty(albums_menu)) {
        media_library_screen_set_active_column(
            screen, MEDIA_LIBRARY_COLUMN_TAGS);
        return ncm_error_set_status(
            ncm_error, -ENOENT, STRLIT("song album is not in library"));
    }

    album_tag = primary_value.data;
    album_tag_len = primary_value.len;
    if (screen->mode == MEDIA_LIBRARY_MODE_ALBUM_ONLY) {
        album_tag = NULL;
        album_tag_len = 0;
    }

    if (!library_move_to_album(screen,
                               primary_value.data, primary_value.len,
                               album.data, album.len,
                               date.data, date.len, true)) {
        NcMediaLibraryAlbumRow row = {0};
        NcMenu *base;

        ASSERT((album_tag != NULL) || (album_tag_len == 0));
        ASSERT((album.data != NULL) || (album.len == 0));
        ASSERT((album_date != NULL) || (album_date_len == 0));

        if (album_tag_len > 0) {
            row.tag = xstrndup(album_tag, album_tag_len);
            row.tag_len = album_tag_len;
        }
        if (album.len > 0) {
            row.album = xstrndup(album.data, album.len);
            row.album_len = album.len;
        }
        if (album_date_len > 0) {
            row.date = xstrndup(album_date, album_date_len);
            row.date_len = album_date_len;
        }
        row.mtime = song->last_modified;
        row.all_tracks_entry = false;
        nc_media_library_album_menu_add_with_flags(
            &screen->albums, &row, NC_MENU_ITEM_SELECTABLE);
        base = nc_media_library_album_menu_base(&screen->albums);
        for (int32 i = 1; i < nc_menu_all_item_count(base); i += 1) {
            int32 j = i;

            while (j > 0) {
                MediaLibraryAlbumItem left;
                MediaLibraryAlbumItem right;
                NcMediaLibraryAlbumRow *left_row;
                NcMediaLibraryAlbumRow *right_row;
                int32 left_rank;
                int32 right_rank;

                left_row = nc_media_library_album_menu_item_at(
                    &screen->albums, NC_MENU_ITEMS_ALL, j);
                right_row = nc_media_library_album_menu_item_at(
                    &screen->albums, NC_MENU_ITEMS_ALL, j - 1);
                ASSERT(left_row != NULL);
                ASSERT(right_row != NULL);

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
                    nc_menu_swap_item_slots(
                        base, NC_MENU_ITEMS_ALL, j, j - 1);
                    j -= 1;
                    continue;
                }
                if (left_rank != 0) {
                    break;
                }
                if (library_compare_album_items(&left, &right) >= 0) {
                    break;
                }
                nc_menu_swap_item_slots(
                    base, NC_MENU_ITEMS_ALL, j, j - 1);
                j -= 1;
            }
        }
        nc_menu_show_all_items(base);
        nc_media_library_album_row_destroy(&row);

        if (!library_move_to_album(
                screen, primary_value.data, primary_value.len,
                album.data, album.len, date.data, date.len, true)) {
            return ncm_error_set_status(
                ncm_error, -ENOENT,
                STRLIT("song album is not in library"));
        }
    }
    media_library_screen_set_active_column(
        screen, MEDIA_LIBRARY_COLUMN_ALBUMS);
    nc_screen_finish_list_change(&screen->screen);

    library_clear_column_filter(screen, MEDIA_LIBRARY_COLUMN_SONGS);
    media_library_screen_request_songs_update(screen);
    status = media_library_screen_update(screen, ncm_error);
    if (status < 0) {
        return status;
    }
    if (nc_menu_is_empty(songs_menu)) {
        nc_menu_clear_items(albums_menu);
        media_library_screen_set_active_column(screen,
                                               MEDIA_LIBRARY_COLUMN_ALBUMS);
        return ncm_error_set_status(
            ncm_error, -ENOENT,
            STRLIT("song is not visible in media library"));
    }
    {
        bool found = false;

        for (int32 i = 0; i < nc_menu_item_count(songs_menu); i += 1) {
            NcmSong *candidate;

            candidate = nc_menu_active_item_at(songs_menu, i);
            ASSERT(candidate != NULL);
            if (ncm_song_is_equal(candidate, song)) {
                nc_menu_goto_selectable(songs_menu, i);
                found = true;
                break;
            }
        }
        if (!found) {
            media_library_screen_set_active_column(
                screen, MEDIA_LIBRARY_COLUMN_SONGS);
            return ncm_error_set_status(
                ncm_error, -ENOENT,
                STRLIT("song is not visible in media library"));
        }
    }

    media_library_screen_set_active_column(screen, MEDIA_LIBRARY_COLUMN_SONGS);
    ncm_error_clear(ncm_error);
    return 0;
}

static MediaLibraryScreen *
library_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcWindow *
library_active_window(NcScreen *screen) {
    return media_library_screen_active_window(library_from_screen(screen));
}

static void
library_refresh(NcScreen *screen) {
    MediaLibraryScreen *library = library_from_screen(screen);
    NcMenu *albums;

    library_update_menu_highlights(library);
    if (media_library_screen_column_is_visible(
        library, MEDIA_LIBRARY_COLUMN_TAGS)) {
        library_refresh_menu(nc_media_library_tag_menu_base(&library->tags),
                             &library->tags_window);
        nc_screen_draw_vertical_separator(
            nc_window_start_x(&library->albums_window) - 1);
    }

    albums = nc_media_library_album_menu_base(&library->albums);
    library_refresh_menu(albums, &library->albums_window);
    if (nc_menu_is_empty(albums)) {
        nc_window_go_to_xy(&library->albums_window, 0, 0);
        nc_window_print_data(&library->albums_window,
                             STRLIT("No albums found."));
        nc_window_refresh(&library->albums_window);
    }
    nc_screen_draw_vertical_separator(
        nc_window_start_x(&library->songs_window) - 1);
    library_refresh_menu(nc_media_library_song_menu_base(&library->songs),
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
    MediaLibraryScreen *library = library_from_screen(screen);

    nc_menu_scroll_selectable(media_library_screen_active_menu(library),
                              library->main_height, where);
    return;
}

static void
library_finish_list_change(NcScreen *screen) {
    media_library_screen_finish_list_change(library_from_screen(screen));
    return;
}

static void
library_mouse_scroll(MediaLibraryScreen *screen, enum NcScroll where) {
    enum NcScroll effective = where;
    int32 count = Config.lines_scrolled;

    if (Config.mouse_list_scroll_whole_page) {
        if (where == NC_SCROLL_DOWN) {
            effective = NC_SCROLL_PAGE_DOWN;
        } else if (where == NC_SCROLL_UP) {
            effective = NC_SCROLL_PAGE_UP;
        }
        count = 1;
    }

    for (int32 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(media_library_screen_active_menu(screen),
                                  screen->main_height, effective);
    }
    return;
}

static int32
library_mouse_select(MediaLibraryScreen *screen,
                     enum MediaLibraryColumn column, NcMenu *menu,
                     int32 y, bool right_click) {
    NcmError ncm_error;
    bool play;

    if ((y < 0) || (y >= nc_menu_item_count(menu))) {
        return 0;
    }
    if (nc_menu_goto_selectable(menu, y) < 0) {
        return 0;
    }
    if (right_click) {
        ncm_error_clear(&ncm_error);
        play = screen->active_column == MEDIA_LIBRARY_COLUMN_SONGS;
        if (media_library_screen_add_item_to_playlist(
            screen, play, &ncm_error) < 0) {
            if (ncm_error_is_set(&ncm_error)) {
                ncm_statusbar_print_cstring(
                    Config.message_delay_time, ncm_error.message);
            }
        }
    }

    if (column == MEDIA_LIBRARY_COLUMN_TAGS) {
        nc_menu_clear_items(nc_media_library_album_menu_base(&screen->albums));
        nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));

        screen->albums_update_request = true;
        screen->songs_update_request = true;

        library_set_observed_album(screen, NULL);
        library_restart_update_timer(screen);
        nc_screen_request_update(&screen->screen);
    } else if (column == MEDIA_LIBRARY_COLUMN_ALBUMS) {
        nc_menu_clear_items(nc_media_library_song_menu_base(&screen->songs));
        screen->songs_update_request = true;
        library_restart_update_timer(screen);
        nc_screen_request_update(&screen->screen);
    }
    return 1;
}

static void
library_mouse_button_pressed(NcScreen *screen,
                             MEVENT event) {
    MediaLibraryScreen *library;
    int32 x;
    int32 y;

    library = library_from_screen(screen);
    x = event.x;
    y = event.y;
    if (media_library_screen_column_is_visible(
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
        if (!((event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
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
        media_library_screen_set_active_column(library,
                                               MEDIA_LIBRARY_COLUMN_SONGS);
        if (!((event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
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

    library = library_from_screen(screen);
    if (library_has_pending_albums(library)
        || library_has_pending_songs(library)) {
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
    MediaLibraryScreen *library = library_from_screen(screen);
    NcmError ncm_error;
    bool update_due = false;
    int32 status;

    if (library_has_pending_tags(library)) {
        update_due = true;
    } else if (library_has_pending_albums(library)
               && ((library->mode != MEDIA_LIBRARY_MODE_THREE_COLUMNS)
                   || library->albums_update_request
                   || library_has_fetch_delay_elapsed(library))) {
        update_due = true;
    } else if (library_has_pending_songs(library)
               && (library->songs_update_request
                   || library_has_fetch_delay_elapsed(library))) {
        update_due = true;
    }

    ncm_error_clear(&ncm_error);

    status = media_library_screen_update(library, &ncm_error);
    if (status < 0) {
        if (ncm_error_is_set(&ncm_error)) {
            ncm_statusbar_print_cstring(Config.message_delay_time,
                                        ncm_error.message);
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

#endif /* NCMPCPP_NC_MEDIA_LIBRARY_C */
