#if !defined(NC_PLAYLIST_EDITOR_C)
#define NC_PLAYLIST_EDITOR_C

#include "cbase.h"

#include "actions.h"
#include "app_controller.h"
#include "c/ncm_c.h"
#include "global.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"

static void
playlist_edit_update_titles(PlaylistEditScreen *screen, bool update_windows) {
    ASSERT(screen != NULL);

    if (screen->last_known_content_count >= 0) {
        screen->last_known_content_count =
            nc_menu_item_count(nc_song_menu_base(&screen->content));
    }

    sb_clear(&screen->playlists_title);
    sb_clear(&screen->content_title);
    if (Config.titles_visibility) {
        SB_APPEND(&screen->playlists_title, "Playlists");
        SB_APPEND(&screen->content_title, "Content");
        if (screen->last_known_content_count >= 0) {
            SB_APPEND(&screen->content_title, " (");
            {
                char digits[32];
                int32 len = 0;
                int32 value = screen->last_known_content_count;

                if (value == 0) {
                    sb_append_byte(&screen->content_title, '0');
                } else {
                    while (value > 0) {
                        digits[len] = (char)('0' + (value % 10));
                        value /= 10;
                        len += 1;
                    }
                    for (int32 i = len - 1; i >= 0; i -= 1) {
                        sb_append_byte(&screen->content_title, digits[i]);
                    }
                }
            }
            if (screen->last_known_content_count == 1) {
                SB_APPEND(&screen->content_title, " item)");
            } else {
                SB_APPEND(&screen->content_title, " items)");
            }
        }
    }

    if (update_windows) {
        nc_window_set_title(&screen->playlists_window,
                            screen->playlists_title.data,
                            screen->playlists_title.len);
        nc_window_set_title(&screen->content_window,
                            screen->content_title.data,
                            screen->content_title.len);
    }
    return;
}

static void
playlist_edit_update_menu_highlights(PlaylistEditScreen *screen) {
    NcMenu *playlists = nc_playlist_entry_menu_base(&screen->playlists);
    NcMenu *content = nc_song_menu_base(&screen->content);

    nc_menu_set_highlight_prefix(playlists,
                                 &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(playlists,
                                 &Config.current_item_inactive_column_suffix);
    nc_menu_set_highlight_prefix(content,
                                 &Config.current_item_inactive_column_prefix);
    nc_menu_set_highlight_suffix(content,
                                 &Config.current_item_inactive_column_suffix);

    {
        NcMenu *active = playlist_edit_screen_active_menu(screen);
        nc_menu_set_highlight_prefix(active, &Config.current_item_prefix);
        nc_menu_set_highlight_suffix(active, &Config.current_item_suffix);
    }
    return;
}

static void
playlist_edit_reset_content_timer(PlaylistEditScreen *screen) {
    ASSERT(screen != NULL);
    screen->timer = global_timer;
    return;
}

static PlaylistEditScreen *
playlist_edit_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcMenu *
playlist_edit_menu_capability(NcScreen *base) {
    return playlist_edit_screen_active_menu((PlaylistEditScreen *)base);
}

static int32
playlist_edit_menu_height_capability(NcScreen *base) {
    PlaylistEditScreen *screen = (PlaylistEditScreen *)base;

    return screen->main_height;
}

static StringView
playlist_edit_filter_constraint_capability(NcScreen *base) {
    PlaylistEditScreen *screen = (PlaylistEditScreen *)base;
    StrBuilder *constraint;

    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        constraint = &screen->content_filter_constraint;
    } else {
        constraint = &screen->playlist_filter_constraint;
    }
    return ncm_string_view(constraint->data, constraint->len);
}

static int32
playlist_edit_filter_apply_capability(NcScreen *base, char *pattern,
                                      int32 pattern_len, uint32 regex_flags,
                                      NcmError *ncm_error) {
    return playlist_edit_screen_apply_active_filter((PlaylistEditScreen *)base,
                                                   pattern, pattern_len,
                                                   regex_flags, ncm_error);
}

static StringView
playlist_edit_search_constraint_capability(NcScreen *base) {
    PlaylistEditScreen *screen = (PlaylistEditScreen *)base;
    StrBuilder *constraint;

    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        constraint = &screen->content_search_constraint;
    } else {
        constraint = &screen->playlist_search_constraint;
    }
    return ncm_string_view(constraint->data, constraint->len);
}

static void
playlist_edit_search_clear_capability(NcScreen *base) {
    PlaylistEditScreen *screen = (PlaylistEditScreen *)base;
    StrBuilder *constraint;
    bool *enabled;

    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        constraint = &screen->content_search_constraint;
        enabled = &screen->content_search_enabled;
    } else {
        constraint = &screen->playlist_search_constraint;
        enabled = &screen->playlist_search_enabled;
    }
    *enabled = false;
    sb_clear(constraint);
    return;
}

static int32
playlist_edit_search_capability(NcScreen *base, enum SearchDirection direction,
                                char *pattern, int32 pattern_len,
                                uint32 regex_flags, bool wrap,
                                bool skip_current, NcmError *ncm_error) {
    bool forward;

    forward = direction == NCM_SEARCH_DIRECTION_FORWARD;
    return playlist_edit_screen_search_active((PlaylistEditScreen *)base,
                                              pattern, pattern_len,
                                              regex_flags, forward, wrap,
                                              skip_current, ncm_error);
}

static int32
playlist_edit_current_song_capability(NcScreen *base, NcmSong *song) {
    return nc_screen_optional_song_status(
        playlist_edit_screen_current_song((PlaylistEditScreen *)base, song));
}

static int32
playlist_edit_selected_songs_capability(NcScreen *base, NcmSongArray *songs) {
    return playlist_edit_screen_selected_songs((PlaylistEditScreen *)base,
                                               songs);
}

NC_SCREEN_COLUMN_CAPABILITY_CALLBACKS(
    playlist_edit, PlaylistEditScreen,
    playlist_edit_screen_can_move_to_previous_column,
    playlist_edit_screen_can_move_to_next_column,
    playlist_edit_screen_previous_column, playlist_edit_screen_next_column)

static NcMenu *
playlist_edit_tag_menu_capability(NcScreen *base) {
    PlaylistEditScreen *screen = (PlaylistEditScreen *)base;

    if (screen->active_column != PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return NULL;
    }
    return nc_song_menu_base(&screen->content);
}

static int32
playlist_edit_tag_at_capability(NcScreen *base, int32 pos,
                                enum SongGetter getter, StrBuilder *tag) {
    PlaylistEditScreen *screen = (PlaylistEditScreen *)base;

    if (screen->active_column != PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    return nc_screen_menu_song_tag_at(nc_song_menu_base(&screen->content),
                                      pos, getter, tag,
                                      nc_screen_menu_item_as_song);
}

static NcWindow *
playlist_edit_active_window_callback(NcScreen *screen) {
    PlaylistEditScreen *editor = playlist_edit_from_screen(screen);
    return playlist_edit_screen_active_window(editor);
}

static void
playlist_edit_refresh_window(NcWindow *window, NcMenu *menu) {
    nc_menu_prepare_refresh(menu, nc_window_height(window), NULL, NULL);
    nc_window_display(window);
    nc_menu_refresh(menu, window, nc_window_width(window),
                    nc_window_height(window));
    return;
}

static void
playlist_edit_refresh_window_callback(NcScreen *screen) {
    PlaylistEditScreen *editor = playlist_edit_from_screen(screen);
    NcWindow *window = playlist_edit_screen_active_window(editor);
    NcMenu *menu = playlist_edit_screen_active_menu(editor);

    playlist_edit_update_titles(editor, true);
    playlist_edit_update_menu_highlights(editor);
    playlist_edit_refresh_window(window, menu);
    return;
}

static int32
playlist_edit_separator_width(int32 width) {
    if (width >= 3) {
        return 1;
    }
    return 0;
}

static void
playlist_edit_refresh_callback(NcScreen *screen) {
    PlaylistEditScreen *editor = playlist_edit_from_screen(screen);
    NcMenu *playlists = nc_playlist_entry_menu_base(&editor->playlists);
    NcMenu *content = nc_song_menu_base(&editor->content);

    playlist_edit_update_titles(editor, true);
    playlist_edit_update_menu_highlights(editor);
    playlist_edit_refresh_window(&editor->playlists_window, playlists);
    if (playlist_edit_separator_width(editor->width) > 0) {
        nc_screen_draw_vertical_separator(editor->right_start_x - 1);
    }
    playlist_edit_refresh_window(&editor->content_window, content);

    return;
}

static void
playlist_edit_scroll_callback(NcScreen *screen, enum NcScroll where) {
    PlaylistEditScreen *editor = playlist_edit_from_screen(screen);
    NcMenu *menu = playlist_edit_screen_active_menu(editor);
    nc_menu_scroll_selectable(menu, editor->main_height, where);
    return;
}

static bool
playlist_edit_has_current_playlist_path(PlaylistEditScreen *screen,
                                          char **path, int32 *path_len) {
    NcmPlaylist *playlist;

    if ((playlist
            = nc_playlist_entry_menu_current(&screen->playlists)) == NULL) {
        return false;
    }
    *path = playlist->path;
    *path_len = playlist->path_len;
    return true;
}

static void
playlist_edit_observe_current_playlist(PlaylistEditScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu = nc_playlist_entry_menu_base(&screen->playlists);

    screen->last_playlist_highlight = nc_menu_highlight(menu);

    if (!playlist_edit_has_current_playlist_path(screen, &path, &path_len)) {
        sb_clear(&screen->observed_playlist_path);
        screen->observed_playlist_valid = false;
        return;
    }

    sb_set(&screen->observed_playlist_path, path, path_len);
    screen->observed_playlist_valid = true;
    return;
}

static void
playlist_edit_clear_stale_content(PlaylistEditScreen *screen) {
    nc_menu_clear_items(nc_song_menu_base(&screen->content));
    sb_clear(&screen->displayed_playlist_path);
    screen->displayed_playlist_valid = false;
    screen->content_update_requested = true;
    screen->last_known_content_count = -1;
    playlist_edit_reset_content_timer(screen);
    playlist_edit_update_titles(screen, true);
    return;
}

static void
playlist_edit_finish_playlist_change(PlaylistEditScreen *screen) {
    char *path;
    int32 path_len;
    NcMenu *menu;
    bool changed;

    ASSERT(screen != NULL);
    if (screen->active_column != PLAYLIST_EDITOR_COLUMN_PLAYLISTS) {
        return;
    }

    menu = nc_playlist_entry_menu_base(&screen->playlists);
    if (!playlist_edit_has_current_playlist_path(screen, &path, &path_len)) {
        changed = screen->observed_playlist_valid;
        playlist_edit_observe_current_playlist(screen);
    } else {
        changed = !screen->observed_playlist_valid
                  || !STREQUAL(screen->observed_playlist_path.data,
                               screen->observed_playlist_path.len,
                               path, path_len)
                  || (screen->last_playlist_highlight
                      != nc_menu_highlight(menu));
        if (changed) {
            playlist_edit_observe_current_playlist(screen);
        }
    }
    if (changed) {
        playlist_edit_clear_stale_content(screen);
    }
    return;
}

static void
playlist_edit_finish_list_change_callback(NcScreen *screen) {
    PlaylistEditScreen *editor;

    editor = playlist_edit_from_screen(screen);
    playlist_edit_finish_playlist_change(editor);
    return;
}

static void
playlist_edit_switch_to_callback(NcScreen *screen) {
    playlist_edit_refresh_callback(screen);
    return;
}

static void
playlist_edit_resize_callback(NcScreen *screen) {
    PlaylistEditScreen *editor = playlist_edit_from_screen(screen);
    NcScreenResizeParams params = nc_screen_resize_params(screen);

    playlist_edit_screen_set_geometry(editor,
                                      params.x_offset, params.width,
                                      editor->main_start_y,
                                      editor->main_height);
    nc_screen_clear_resize_request(screen);
    return;
}

static bool
playlist_edit_displayed_playlist_is_current(PlaylistEditScreen *screen) {
    char *path;
    int32 path_len;

    if (!screen->displayed_playlist_valid) {
        return false;
    }
    if (!playlist_edit_has_current_playlist_path(screen, &path, &path_len)) {
        return false;
    }
    return STREQUAL(screen->displayed_playlist_path.data,
                    screen->displayed_playlist_path.len, path, path_len);
}

static int32
playlist_edit_timeout_callback(NcScreen *screen) {
    PlaylistEditScreen *editor = playlist_edit_from_screen(screen);
    NcMenu *playlists = nc_playlist_entry_menu_base(&editor->playlists);
    NcMenu *content = nc_song_menu_base(&editor->content);

    if ((editor->fetching_delay_ms >= 0)
        && (nc_menu_item_count(content) <= 0)
        && ((nc_menu_item_count(playlists) <= 0)
            || !playlist_edit_displayed_playlist_is_current(editor))) {
        return editor->window_timeout_ms;
    }
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
playlist_edit_title_callback(NcScreen *screen) {
    (void)screen;
    return "Playlist editor";
}

static void
playlist_edit_report_error(char *context, int32 context_len,
                           NcmError *ncm_error) {
    StrBuilder message = {0};

    ASSERT(ncm_error != NULL);

    SB_APPEND(&message, context, context_len);
    if (ncm_error->message[0] != 0) {
        SB_APPEND(&message, ": ");
        SB_APPEND(&message, ncm_error->message, ncm_error->message_len);
    }
    ncm_statusbar_print(Config.message_delay_time, message.data, message.len);
    sb_free(&message);
    return;
}

static void
playlist_edit_update_callback(NcScreen *screen) {
    PlaylistEditScreen *editor;
    NcmError ncm_error = {0};
    NcMenu *playlists;
    bool content_fetch_due;
    int32 changed;
    int32 status;

    editor = playlist_edit_from_screen(screen);
    playlist_edit_finish_playlist_change(editor);

    changed = 0;
    ncm_error_clear(&ncm_error);
    playlists = nc_playlist_entry_menu_base(&editor->playlists);
    if (editor->playlists_update_requested
        || (nc_menu_item_count(playlists) <= 0)) {
        status = playlist_edit_screen_reload_playlists_from_mpd(editor,
                                                                &global_mpd,
                                                                &ncm_error);
        if (status < 0) {
            editor->playlists_update_requested = false;
            playlist_edit_report_error(STRLIT("Could not fetch playlists"),
                                       &ncm_error);
            ncm_error_clear(&ncm_error);
            playlist_edit_update_titles(editor, true);
            goto update_finished;
        }
        changed = 1;
    }

    playlist_edit_finish_playlist_change(editor);
    content_fetch_due = false;
    if (nc_menu_item_count(playlists) > 0) {
        NcMenu *content = nc_song_menu_base(&editor->content);
        bool displayed_is_current =
            playlist_edit_displayed_playlist_is_current(editor);
        if (editor->content_update_requested && displayed_is_current) {
            content_fetch_due = true;
        } else if (!displayed_is_current
                   && !((editor->last_known_content_count == 0)
                        && editor->displayed_playlist_valid)
                   && (nc_menu_item_count(content) <= 0)) {
            if (editor->fetching_delay_ms < 0) {
                content_fetch_due = true;
            } else {
                content_fetch_due = global_timer_elapsed_ms(editor->timer)
                                    > editor->fetching_delay_ms;
            }
        }
    }
    if (!content_fetch_due) {
        playlist_edit_update_titles(editor, true);
        goto update_finished;
    }

    ncm_error_clear(&ncm_error);
    status = playlist_edit_screen_reload_content_from_mpd(editor, &global_mpd,
                                                          &ncm_error);
    if (status < 0) {
        editor->content_update_requested = false;
        playlist_edit_report_error(STRLIT("Could not fetch playlist content"),
                                   &ncm_error);
        ncm_error_clear(&ncm_error);
        playlist_edit_update_titles(editor, true);
        goto update_finished;
    }
    changed = 1;
    playlist_edit_update_titles(editor, true);

update_finished:
    nc_screen_clear_update_request(screen);
    if ((changed > 0) && app_controller_is_screen_visible(screen)) {
        nc_screen_refresh(screen);
    }
    return;
}

static void
playlist_edit_mouse_scroll(PlaylistEditScreen *screen, enum NcScroll where) {
    enum NcScroll effective = where;
    NcMenu *menu = playlist_edit_screen_active_menu(screen);
    int32 count = Config.lines_scrolled;

    if (Config.mouse_list_scroll_whole_page) {
        count = 1;
        if (where == NC_SCROLL_DOWN) {
            effective = NC_SCROLL_PAGE_DOWN;
        } else if (where == NC_SCROLL_UP) {
            effective = NC_SCROLL_PAGE_UP;
        }
    }
    if (count < 1) {
        count = 1;
    }
    for (int32 i = 0; i < count; i += 1) {
        nc_menu_scroll_selectable(menu, screen->main_height, effective);
    }
    playlist_edit_finish_playlist_change(screen);
    return;
}

static void
playlist_edit_mouse_callback(NcScreen *screen, MEVENT event) {
    PlaylistEditScreen *editor = playlist_edit_from_screen(screen);
    int32 x = event.x;
    int32 y = event.y;

    if (nc_window_has_coords(&editor->playlists_window, &x, &y)) {
        if (editor->active_column != PLAYLIST_EDITOR_COLUMN_PLAYLISTS) {
            if (!playlist_edit_screen_can_move_to_previous_column(editor)) {
                return;
            }
            playlist_edit_screen_previous_column(editor);
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu = nc_playlist_entry_menu_base(&editor->playlists);

            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)) {
                playlist_edit_finish_playlist_change(editor);
                if (event.bstate & BUTTON3_PRESSED) {
                    NcmError ncm_error = {0};
                    NcmPlaylist *playlist;
                    bool loaded;
                    int32 status;

                    playlist =
                        nc_playlist_entry_menu_current(&editor->playlists);
                    loaded = false;
                    ncm_error_clear(&ncm_error);
                    status = ncm_mpd_client_load_playlist(&global_mpd,
                                                          playlist->path,
                                                          &loaded,
                                                          &ncm_error);
                    if (status < 0) {
                        char *context = "Could not load playlist";

                        playlist_edit_report_error(context, strlen32(context),
                                                   &ncm_error);
                    } else if (loaded) {
                        StrBuilder message = {0};

                        SB_APPEND(&message, "Playlist \"");
                        SB_APPEND(&message, playlist->path, playlist->path_len);
                        SB_APPEND(&message, "\" loaded");

                        ncm_statusbar_print(Config.message_delay_time,
                                            message.data, message.len);
                        sb_free(&message);
                        ncm_status_update_full(&global_mpd, NULL, &ncm_error);
                    }
                }
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            playlist_edit_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            playlist_edit_mouse_scroll(editor, NC_SCROLL_UP);
        }
        playlist_edit_finish_playlist_change(editor);
        return;
    }

    x = event.x;
    y = event.y;
    if (nc_window_has_coords(&editor->content_window, &x, &y)) {
        if (editor->active_column != PLAYLIST_EDITOR_COLUMN_CONTENT) {
            if (!playlist_edit_screen_can_move_to_next_column(editor)) {
                return;
            }
            playlist_edit_screen_next_column(editor);
        }
        if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
            NcMenu *menu = nc_song_menu_base(&editor->content);

            if ((y >= 0) && (y < nc_menu_item_count(menu))
                && (nc_menu_goto_selectable(menu, y) >= 0)
                && (event.bstate & BUTTON3_PRESSED)) {
                NcmSong *song = nc_song_menu_current(&editor->content);

                ncm_action_add_song_to_playlist(song, true, -1);
            }
        } else if (event.bstate & BUTTON5_PRESSED) {
            playlist_edit_mouse_scroll(editor, NC_SCROLL_DOWN);
        } else if (event.bstate & BUTTON4_PRESSED) {
            playlist_edit_mouse_scroll(editor, NC_SCROLL_UP);
        }
    }
    return;
}

static void
playlist_edit_destroy_callback(NcScreen *screen) {
    playlist_edit_screen_destroy(playlist_edit_from_screen(screen));
    return;
}

static int32
playlist_edit_toggle_display_mode(NcScreen *base) {
    Config.playlist_edit_display_mode =
        nc_screen_next_display_mode(Config.playlist_edit_display_mode);
    nc_screen_request_resize(base);
    return 0;
}

static bool
playlist_edit_search_text_matches(NcmRegex *regex, char *data, int32 len) {
    if (data == NULL) {
        data = "";
        len = 0;
    }
    return ncm_regex_matches(regex, data, len);
}

static bool
playlist_edit_playlist_matches_regex(NcmRegex *regex, NcmPlaylist *playlist) {
    return playlist_edit_search_text_matches(regex, playlist->path,
                                               playlist->path_len);
}

static bool
playlist_filter_callback(NcMenu *menu, void *item, void *user) {
    PlaylistEditScreen *editor = user;
    NcmPlaylist *playlist = item;

    (void)menu;
    if (!editor->playlist_filter_enabled) {
        return true;
    }
    return playlist_edit_playlist_matches_regex(&editor->playlist_filter_regex,
                                                playlist);
}

static bool
playlist_edit_content_matches_regex(NcmRegex *regex, NcmSong *song) {
    NcBuffer buffer = {0};
    bool result;

    if (Config.playlist_edit_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        ncm_display_song_row(&buffer, &Config.song_columns_mode_format,
                             song, NCM_FORMAT_FLAG_ALL);
    } else {
        ncm_display_song_row(&buffer, &Config.song_list_format, song,
                             NCM_FORMAT_FLAG_ALL);
    }
    result = playlist_edit_search_text_matches(regex, buffer.data, buffer.len);
    nc_buffer_destroy(&buffer);
    return result;
}

static bool
content_filter_callback(NcMenu *menu, void *item, void *user) {
    PlaylistEditScreen *editor = user;

    (void)menu;

    if (!editor->content_filter_enabled) {
        return true;
    }
    return playlist_edit_content_matches_regex(&editor->content_filter_regex,
                                               item);
}

static void
playlist_draw_callback(NcMenu *menu, NcWindow *window, void *item,
                       int32 pos, void *user) {
    NcmPlaylist *playlist = item;

    (void)menu;
    (void)pos;
    (void)user;

    nc_window_print_data(window, playlist->path, playlist->path_len);
    return;
}

static void
content_draw_callback(NcMenu *menu, NcWindow *window, void *item,
                      int32 pos, void *user) {
    NcBuffer buffer = {0};
    int32 list_width;
    bool use_colors;

    (void)user;

    if (Config.playlist_edit_display_mode == NCM_DISPLAY_MODE_COLUMNS) {
        list_width = nc_window_width(window) - nc_window_get_x(window);
        if (nc_menu_position_is_selected(menu, pos)) {
            list_width -= utf8_width(menu->selected_suffix.data,
                                     menu->selected_suffix.len);
        }
        if (!menu->highlight_disabled && (pos == menu->highlight)) {
            list_width -= utf8_width(menu->highlight_suffix.data,
                                     menu->highlight_suffix.len);
        }
        if (list_width < 0) {
            list_width = 0;
        }
        use_colors = !Config.discard_colors_if_item_is_selected
                     || !nc_menu_position_is_selected(menu, pos);
        ncm_display_song_columns(&buffer, item,
                                 Config.song_columns_list_format.items,
                                 Config.song_columns_list_format.len,
                                 list_width, use_colors);
    } else {
        ncm_display_song_row(&buffer, &Config.song_list_format, item,
                             NCM_FORMAT_FLAG_ALL);
    }
    {
        NcBufferProperty *properties = nc_buffer_properties(&buffer);
        char *data = nc_buffer_data(&buffer);
        int32 len = buffer.len;
        int32 property_len = ARRAY_LEN(buffer.properties);
        int32 property_index = 0;

        for (int32 i = 0;; i += 1) {
            while ((property_index < property_len)
                   && (properties[property_index].position == i)) {
                nc_buffer_apply_property(window, &properties[property_index]);
                property_index += 1;
            }
            if (i >= len) {
                break;
            }
            nc_window_print_char(window, data[i]);
        }
    }
    nc_buffer_destroy(&buffer);
    return;
}

void
playlist_edit_screen_init(PlaylistEditScreen *screen,
                            int32 start_x, int32 width,
                            int32 main_start_y, int32 main_height,
                            NcColor color, NcBorder border) {
    NcScreenOps callbacks = {0};
    int32 initial_left_width;
    int32 initial_right_width;

    if (width < 1) {
        width = 1;
    }
    if (main_height < 1) {
        main_height = 1;
    }
    initial_left_width = width / 2;
    if (initial_left_width < 1) {
        initial_left_width = 1;
    }
    initial_right_width = width - initial_left_width;
    if (initial_right_width < 1) {
        initial_right_width = 1;
    }

    callbacks.active_window = playlist_edit_active_window_callback;
    callbacks.refresh = playlist_edit_refresh_callback;
    callbacks.refresh_window = playlist_edit_refresh_window_callback;
    callbacks.scroll = playlist_edit_scroll_callback;
    callbacks.list_change_finished = playlist_edit_finish_list_change_callback;
    callbacks.switch_to = playlist_edit_switch_to_callback;
    callbacks.resize = playlist_edit_resize_callback;
    callbacks.window_timeout_callback = playlist_edit_timeout_callback;
    callbacks.title = playlist_edit_title_callback;
    callbacks.update = playlist_edit_update_callback;
    callbacks.mouse_button_pressed = playlist_edit_mouse_callback;
    callbacks.lockable = true;
    callbacks.mergable = true;
    callbacks.destroy = playlist_edit_destroy_callback;
    callbacks.capabilities = NC_SCREEN_CAPABILITY_MENU
                             |NC_SCREEN_CAPABILITY_FILTER
                             |NC_SCREEN_CAPABILITY_SEARCH
                             |NC_SCREEN_CAPABILITY_SONGS
                             |NC_SCREEN_CAPABILITY_COLUMNS
                             |NC_SCREEN_CAPABILITY_TAGS
                             |NC_SCREEN_CAPABILITY_DISPLAY_MODE;
    callbacks.current_menu = playlist_edit_menu_capability;
    callbacks.current_menu_height = playlist_edit_menu_height_capability;
    callbacks.current_filter = playlist_edit_filter_constraint_capability;
    callbacks.apply_filter = playlist_edit_filter_apply_capability;
    callbacks.current_search_constraint =
        playlist_edit_search_constraint_capability;
    callbacks.clear_search_constraint = playlist_edit_search_clear_capability;
    callbacks.search = playlist_edit_search_capability;
    callbacks.current_song = playlist_edit_current_song_capability;
    callbacks.selected_songs = playlist_edit_selected_songs_capability;
    NC_SCREEN_COLUMN_CAPABILITY_SET_OPS(callbacks, playlist_edit);
    callbacks.tag_menu = playlist_edit_tag_menu_capability;
    callbacks.song_tag_at = playlist_edit_tag_at_capability;
    callbacks.toggle_display_mode = playlist_edit_toggle_display_mode;

    nc_playlist_entry_menu_init(&screen->playlists);
    nc_song_menu_init(&screen->content);

    screen->playlist_filter_constraint = (StrBuilder){0};
    screen->content_filter_constraint = (StrBuilder){0};
    screen->playlist_search_constraint = (StrBuilder){0};
    screen->content_search_constraint = (StrBuilder){0};
    screen->playlists_title = (StrBuilder){0};
    screen->content_title = (StrBuilder){0};
    screen->displayed_playlist_path = (StrBuilder){0};
    screen->observed_playlist_path = (StrBuilder){0};

    screen->playlist_filter_regex = (NcmRegex){0};
    screen->content_filter_regex = (NcmRegex){0};
    screen->playlist_search_regex = (NcmRegex){0};
    screen->content_search_regex = (NcmRegex){0};
    playlist_edit_reset_content_timer(screen);

    screen->active_column = PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
    screen->column_ratio_left = 1;
    screen->column_ratio_right = 1;
    screen->playlists_update_requested = true;
    screen->content_update_requested = true;
    screen->playlist_filter_enabled = false;
    screen->content_filter_enabled = false;
    screen->playlist_search_enabled = false;
    screen->content_search_enabled = false;
    screen->displayed_playlist_valid = false;
    screen->observed_playlist_valid = false;
    screen->last_playlist_highlight = -1;
    screen->last_known_content_count = -1;

    if (Config.data_fetching_delay) {
        screen->fetching_delay_ms = PLAYLIST_EDITOR_FETCH_DELAY_MS;
        screen->window_timeout_ms = PLAYLIST_EDITOR_FETCH_DELAY_MS;
    } else {
        screen->fetching_delay_ms = -1;
        screen->window_timeout_ms = NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
    }
    screen->registered = false;

    playlist_edit_update_titles(screen, false);
    nc_window_init(&screen->playlists_window, start_x, main_start_y,
                   initial_left_width, main_height,
                   screen->playlists_title.data,
                   screen->playlists_title.len, color, border);
    nc_window_init(&screen->content_window,
                   start_x + initial_left_width, main_start_y,
                   initial_right_width, main_height,
                   screen->content_title.data, screen->content_title.len,
                   color, border);
    playlist_edit_screen_set_geometry(screen, start_x, width,
                                        main_start_y, main_height);
    nc_screen_init_ops(&screen->screen, callbacks, screen,
                       NC_SCREEN_TYPE_PLAYLIST_EDITOR);
    {
        NcMenu *playlists = nc_playlist_entry_menu_base(&screen->playlists);
        NcMenu *content = nc_song_menu_base(&screen->content);

        nc_menu_set_selected_prefix(playlists, &Config.selected_item_prefix);
        nc_menu_set_selected_suffix(playlists, &Config.selected_item_suffix);
        nc_menu_set_selected_prefix(content, &Config.selected_item_prefix);
        nc_menu_set_selected_suffix(content, &Config.selected_item_suffix);

        nc_menu_set_cyclic_scrolling(playlists, Config.cyclic_scrolling);
        nc_menu_set_cyclic_scrolling(content, Config.cyclic_scrolling);
        nc_menu_set_centered_cursor(playlists, Config.centered_cursor);
        nc_menu_set_centered_cursor(content, Config.centered_cursor);

        playlist_edit_update_menu_highlights(screen);
    }
    {
        NcMenuDisplayCallbacks display_callbacks = {0};
        NcMenu *playlists = nc_playlist_entry_menu_base(&screen->playlists);
        NcMenu *content = nc_song_menu_base(&screen->content);

        display_callbacks.draw = playlist_draw_callback;
        display_callbacks.matches_filter = playlist_filter_callback;
        display_callbacks.user = screen;
        nc_menu_set_display_callbacks(playlists, display_callbacks);

        display_callbacks = (NcMenuDisplayCallbacks){0};
        display_callbacks.draw = content_draw_callback;
        display_callbacks.matches_filter = content_filter_callback;
        display_callbacks.user = screen;
        nc_menu_set_display_callbacks(content, display_callbacks);
    }

    return;
}

void
playlist_edit_screen_destroy(PlaylistEditScreen *screen) {
    if (screen == NULL) {
        return;
    }

    app_controller_unregister_screen(playlist_edit_screen_base(screen));
    nc_window_destroy(&screen->content_window);
    nc_window_destroy(&screen->playlists_window);
    nc_song_menu_destroy(&screen->content);
    nc_playlist_entry_menu_destroy(&screen->playlists);
    ncm_regex_destroy(&screen->content_search_regex);
    ncm_regex_destroy(&screen->playlist_search_regex);
    ncm_regex_destroy(&screen->content_filter_regex);
    ncm_regex_destroy(&screen->playlist_filter_regex);

    sb_free(&screen->observed_playlist_path);
    sb_free(&screen->displayed_playlist_path);
    sb_free(&screen->content_title);
    sb_free(&screen->playlists_title);
    sb_free(&screen->content_search_constraint);
    sb_free(&screen->playlist_search_constraint);
    sb_free(&screen->content_filter_constraint);
    sb_free(&screen->playlist_filter_constraint);

    screen->registered = false;
    return;
}

NcScreen *
playlist_edit_screen_base(PlaylistEditScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->screen;
}

NcPlaylistEntryMenu *
playlist_edit_screen_playlists(PlaylistEditScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->playlists;
}

NcSongMenu *
playlist_edit_screen_content(PlaylistEditScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    return &screen->content;
}

NcMenu *
playlist_edit_screen_active_menu(PlaylistEditScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return nc_song_menu_base(&screen->content);
    }
    return nc_playlist_entry_menu_base(&screen->playlists);
}

NcWindow *
playlist_edit_screen_active_window(PlaylistEditScreen *screen) {
    if (screen == NULL) {
        return NULL;
    }
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        return &screen->content_window;
    }
    return &screen->playlists_window;
}

static void
playlist_edit_apply_geometry(PlaylistEditScreen *screen) {
    int32 total;
    int32 separator_width;
    int32 left_width;

    if (screen->width < 1) {
        screen->width = 1;
    }
    if (screen->main_height < 1) {
        screen->main_height = 1;
    }
    if (screen->column_ratio_left < 1) {
        screen->column_ratio_left = 1;
    }
    if (screen->column_ratio_right < 1) {
        screen->column_ratio_right = 1;
    }

    total = screen->column_ratio_left + screen->column_ratio_right;
    separator_width = playlist_edit_separator_width(screen->width);
    left_width = screen->width*screen->column_ratio_left / total
                 - separator_width;
    if (left_width < 1) {
        left_width = 1;
    }
    if ((left_width + separator_width + 1) > screen->width) {
        left_width = screen->width - separator_width - 1;
    }
    if (left_width < 1) {
        left_width = 1;
    }

    screen->left_width = left_width;
    screen->right_start_x = screen->start_x + screen->left_width
                            + separator_width;
    screen->right_width = screen->width - screen->left_width
                          - separator_width;
    if (screen->right_width < 1) {
        screen->right_width = 1;
    }

    nc_window_resize(&screen->playlists_window,
                     screen->left_width, screen->main_height);
    nc_window_move_to(&screen->playlists_window,
                      screen->start_x, screen->main_start_y);
    nc_window_resize(&screen->content_window,
                     screen->right_width, screen->main_height);
    nc_window_move_to(&screen->content_window,
                      screen->right_start_x, screen->main_start_y);
    return;
}

void
playlist_edit_screen_set_geometry(PlaylistEditScreen *screen,
                                  int32 start_x, int32 width,
                                  int32 main_start_y, int32 main_height) {
    if (screen == NULL) {
        return;
    }
    screen->start_x = start_x;
    screen->width = width;
    screen->main_start_y = main_start_y;
    screen->main_height = main_height;
    playlist_edit_apply_geometry(screen);
    return;
}

void
playlist_edit_screen_set_column_ratio(PlaylistEditScreen *screen,
                                        int32 left, int32 right) {
    if (screen == NULL) {
        return;
    }
    if (left < 1) {
        left = 1;
    }
    if (right < 1) {
        right = 1;
    }
    screen->column_ratio_left = left;
    screen->column_ratio_right = right;
    playlist_edit_apply_geometry(screen);
    return;
}

bool
playlist_edit_screen_can_move_to_previous_column(PlaylistEditScreen *screen) {
    NcMenu *playlists;

    if (screen == NULL) {
        return false;
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    return (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT)
           && (nc_menu_all_item_count(playlists) > 0);
}

bool
playlist_edit_screen_can_move_to_next_column(PlaylistEditScreen *screen) {
    NcMenu *content;

    if (screen == NULL) {
        return false;
    }
    content = nc_song_menu_base(&screen->content);
    return (screen->active_column == PLAYLIST_EDITOR_COLUMN_PLAYLISTS)
           && (nc_menu_all_item_count(content) > 0);
}

void
playlist_edit_screen_previous_column(PlaylistEditScreen *screen) {
    if (playlist_edit_screen_can_move_to_previous_column(screen)) {
        screen->active_column = PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
        playlist_edit_update_menu_highlights(screen);
    }
    return;
}

void
playlist_edit_screen_next_column(PlaylistEditScreen *screen) {
    if (playlist_edit_screen_can_move_to_next_column(screen)) {
        screen->active_column = PLAYLIST_EDITOR_COLUMN_CONTENT;
        playlist_edit_update_menu_highlights(screen);
    }
    return;
}

static bool
playlist_edit_store_current_playlist_path(PlaylistEditScreen *screen,
                                            StrBuilder *buffer) {
    char *path;
    int32 path_len;

    sb_clear(buffer);
    if (!playlist_edit_has_current_playlist_path(screen, &path, &path_len)) {
        return false;
    }
    sb_set(buffer, path, path_len);
    return true;
}

static void
playlist_edit_restore_playlist_path(PlaylistEditScreen *screen,
                                      StrBuilder *buffer) {
    NcMenu *menu;

    if (buffer->len <= 0) {
        return;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmPlaylist *playlist = nc_menu_active_item_at(menu, i);

        if (STREQUAL(playlist->path, playlist->path_len,
                     buffer->data, buffer->len)) {
            nc_menu_highlight_position(menu, i, screen->main_height);
            return;
        }
    }
    return;
}

int32
playlist_edit_screen_load_playlists(PlaylistEditScreen *screen,
                                      NcmMpdPlaylistList *playlists) {
    StrBuilder preserved = {0};
    NcMenu *menu;
    bool had_preserved;

    if ((screen == NULL) || (playlists == NULL)) {
        return -EINVAL;
    }
    had_preserved = playlist_edit_store_current_playlist_path(screen,
                                                              &preserved);
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    nc_menu_show_all_items(menu);
    nc_menu_clear_items(menu);
    for (int32 i = 0; i < playlists->count; i += 1) {
        nc_playlist_entry_menu_add(&screen->playlists, &playlists->items[i]);
    }
    if (had_preserved) {
        playlist_edit_restore_playlist_path(screen, &preserved);
    }
    if (screen->playlist_filter_enabled) {
        nc_menu_apply_filter(menu);
        if (had_preserved) {
            playlist_edit_restore_playlist_path(screen, &preserved);
        }
    }
    if (screen->displayed_playlist_valid
        && !playlist_edit_displayed_playlist_is_current(screen)) {
        playlist_edit_clear_stale_content(screen);
    }
    playlist_edit_observe_current_playlist(screen);
    screen->playlists_update_requested = false;
    sb_free(&preserved);
    return 0;
}

int32
playlist_edit_screen_reload_playlists_from_mpd(PlaylistEditScreen *screen,
                                                 MpdClient *client,
                                                 NcmError *ncm_error) {
    NcmMpdPlaylistList playlists = {0};
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
    }

    status = ncm_mpd_client_get_playlists(client, &playlists, ncm_error);
    if (status == 0) {
        for (int32 i = 1; i < playlists.count; i += 1) {
            NcmPlaylist current = {0};
            int32 j = i;

            ncm_playlist_move(&current, &playlists.items[i]);
            while (j > 0) {
                NcmPlaylist *left = &playlists.items[j - 1];
                NcmPlaylist *right = &current;

                if (ncm_compare_locale_strings(left->path, left->path_len,
                                               right->path, right->path_len,
                                               Config.ignore_leading_the)
                        <= 0) {
                    break;
                }
                ncm_playlist_move(&playlists.items[j], &playlists.items[j - 1]);
                j -= 1;
            }
            ncm_playlist_move(&playlists.items[j], &current);
            ncm_playlist_destroy(&current);
        }
        playlist_edit_screen_load_playlists(screen, &playlists);
    }
    ncm_mpd_playlist_list_destroy(&playlists);
    return status;
}

static void
playlist_edit_restore_content_song(PlaylistEditScreen *screen, NcmSong *song) {
    NcMenu *menu = nc_song_menu_base(&screen->content);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmSong *item = nc_menu_active_item_at(menu, i);

        if (ncm_song_is_equal(item, song)) {
            nc_menu_highlight_position(menu, i, screen->main_height);
            return;
        }
    }
    return;
}

static bool
playlist_edit_store_current_song(PlaylistEditScreen *screen, NcmSong *song) {
    NcmSong *current;

    if ((current = nc_song_menu_current(&screen->content)) == NULL) {
        return false;
    }
    ncm_song_copy(song, current);
    return true;
}

int32
playlist_edit_screen_load_content(PlaylistEditScreen *screen,
                                    NcmMpdSongList *songs) {
    NcMenu *menu;
    NcmSong preserved_song = {0};
    bool had_preserved_song;

    if ((screen == NULL) || (songs == NULL)) {
        return -EINVAL;
    }

    had_preserved_song = playlist_edit_store_current_song(screen,
                                                          &preserved_song);

    menu = nc_song_menu_base(&screen->content);
    nc_menu_show_all_items(menu);
    nc_menu_clear_items(menu);
    for (int32 i = 0; i < songs->count; i += 1) {
        nc_song_menu_add(&screen->content, &songs->items[i]);
    }
    if (screen->content_filter_enabled) {
        nc_menu_apply_filter(menu);
    }
    if (had_preserved_song) {
        playlist_edit_restore_content_song(screen, &preserved_song);
    }
    {
        char *path;
        int32 path_len;

        if (!playlist_edit_has_current_playlist_path(screen,
                                                     &path, &path_len)) {
            sb_clear(&screen->displayed_playlist_path);
            screen->displayed_playlist_valid = false;
        } else {
            sb_set(&screen->displayed_playlist_path, path, path_len);
            screen->displayed_playlist_valid = true;
        }
    }
    playlist_edit_observe_current_playlist(screen);
    screen->last_known_content_count = nc_menu_all_item_count(menu);
    screen->content_update_requested = false;
    playlist_edit_update_titles(screen, true);
    ncm_song_destroy(&preserved_song);
    return 0;
}

int32
playlist_edit_screen_reload_content_from_mpd(PlaylistEditScreen *screen,
                                               MpdClient *client,
                                               NcmError *ncm_error) {
    NcmMpdSongList songs;
    NcmPlaylist *playlist;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
    }
    if ((playlist = nc_playlist_entry_menu_current(&screen->playlists))
        == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist"));
    }

    songs = (NcmMpdSongList){0};
    status = ncm_mpd_client_get_playlist_content(client, playlist->path,
                                                 &songs, ncm_error);
    if (status == 0) {
        playlist_edit_screen_load_content(screen, &songs);
    }
    ncm_mpd_song_list_destroy(&songs);
    return status;
}

static void
playlist_edit_clear_playlist_filter(PlaylistEditScreen *screen) {
    StrBuilder path = {0};
    bool has_path = playlist_edit_store_current_playlist_path(screen, &path);

    screen->playlist_filter_enabled = false;
    sb_clear(&screen->playlist_filter_constraint);
    nc_menu_show_all_items(nc_playlist_entry_menu_base(&screen->playlists));
    if (has_path) {
        playlist_edit_restore_playlist_path(screen, &path);
    }
    sb_free(&path);
    playlist_edit_update_titles(screen, true);
    return;
}

static void
playlist_edit_clear_content_filter(PlaylistEditScreen *screen) {
    NcmSong song = {0};
    bool has_song = playlist_edit_store_current_song(screen, &song);

    screen->content_filter_enabled = false;
    sb_clear(&screen->content_filter_constraint);
    nc_menu_show_all_items(nc_song_menu_base(&screen->content));
    if (has_song) {
        playlist_edit_restore_content_song(screen, &song);
    }
    ncm_song_destroy(&song);
    playlist_edit_update_titles(screen, true);
    return;
}

static int32
playlist_edit_show_screen(PlaylistEditScreen *screen) {
    int32 status;

    ASSERT(screen != NULL);
    if (!app_controller_is_screen_registered(&screen->screen)) {
        if ((status = app_controller_register_screen(&screen->screen)) < 0) {
            return status;
        }
        screen->registered = true;
    }
    return nc_screen_switcher_switch_to(&screen->screen,
                                        screen->screen.has_to_be_resized);
}

int32
playlist_edit_screen_locate_playlist(
    PlaylistEditScreen *screen, MpdClient *client,
    char *path, int32 path_len, NcmError *ncm_error) {
    NcMenu *menu;
    int32 pos;
    int32 status;

    if ((screen == NULL) || (path == NULL) || (path_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist"));
    }
    status = playlist_edit_screen_reload_playlists_from_mpd(screen, client,
                                                              ncm_error);
    if (status < 0) {
        return status;
    }

    playlist_edit_clear_playlist_filter(screen);
    pos = -ENOENT;
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
        NcmPlaylist *playlist = nc_menu_active_item_at(menu, i);

        if (STREQUAL(playlist->path, playlist->path_len, path, path_len)) {
            pos = i;
            break;
        }
    }
    if (pos < 0) {
        return ncm_error_set_status(ncm_error, -ENOENT,
                                    STRLIT("playlist not found"));
    }

    menu = nc_playlist_entry_menu_base(&screen->playlists);
    nc_menu_highlight_position(menu, pos,
                               nc_window_height(&screen->playlists_window));
    screen->active_column = PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
    playlist_edit_update_menu_highlights(screen);
    playlist_edit_clear_content_filter(screen);
    playlist_edit_clear_stale_content(screen);
    status = playlist_edit_screen_reload_content_from_mpd(screen, client,
                                                          ncm_error);
    if (status < 0) {
        return status;
    }
    return playlist_edit_show_screen(screen);
}

static int32
playlist_edit_find_song_in_content_range(PlaylistEditScreen *screen,
                                         NcmSong *song,
                                         int32 first, int32 last) {
    NcMenu *menu = nc_song_menu_base(&screen->content);

    if (first < 0) {
        first = 0;
    }
    if (last > nc_menu_item_count(menu)) {
        last = nc_menu_item_count(menu);
    }
    for (int32 i = first; i < last; i += 1) {
        NcmSong *candidate = nc_menu_active_item_at(menu, i);

        if (ncm_song_is_equal(candidate, song)) {
            return i;
        }
    }
    return -ENOENT;
}

static int32
playlist_edit_highlight_content_position(PlaylistEditScreen *screen,
                                           int32 pos) {
    NcMenu *menu = nc_song_menu_base(&screen->content);

    if ((pos < 0) || (pos >= nc_menu_item_count(menu))) {
        return -EINVAL;
    }
    nc_menu_highlight_position(menu, pos,
                               nc_window_height(&screen->content_window));
    screen->active_column = PLAYLIST_EDITOR_COLUMN_CONTENT;
    playlist_edit_update_menu_highlights(screen);
    return 0;
}

static int32
playlist_edit_locate_song_in_playlist_range(PlaylistEditScreen *screen,
                                            MpdClient *client,
                                            NcmSong *song,
                                            int32 first, int32 last,
                                            NcmError *ncm_error) {
    NcMenu *menu = nc_playlist_entry_menu_base(&screen->playlists);

    if (first < 0) {
        first = 0;
    }
    if (last > nc_menu_item_count(menu)) {
        last = nc_menu_item_count(menu);
    }
    for (int32 i = first; i < last; i += 1) {
        NcmMpdSongList songs = {0};
        NcmPlaylist *playlist = nc_menu_active_item_at(menu, i);
        int32 song_index;
        int32 status;

        song_index = ncm_mpd_client_get_playlist_content_no_info(client,
                                                                 playlist->path,
                                                                 &songs,
                                                                 ncm_error);
        if (song_index >= 0) {
            song_index = -ENOENT;
            for (int32 j = 0; j < songs.count; j += 1) {
                if (ncm_song_is_equal(&songs.items[j], song)) {
                    song_index = j;
                    break;
                }
            }
            if (song_index == -ENOENT) {
                ncm_error_clear(ncm_error);
            }
        }
        ncm_mpd_song_list_destroy(&songs);
        if (song_index < 0) {
            if (song_index == -ENOENT) {
                continue;
            }
            return song_index;
        }
        nc_menu_highlight_position(menu, i,
                                   nc_window_height(&screen->playlists_window));
        screen->active_column = PLAYLIST_EDITOR_COLUMN_PLAYLISTS;
        playlist_edit_update_menu_highlights(screen);
        playlist_edit_clear_stale_content(screen);
        status = playlist_edit_screen_reload_content_from_mpd(screen, client,
                                                              ncm_error);
        if (status < 0) {
            return status;
        }
        status = playlist_edit_highlight_content_position(screen, song_index);
        if (status < 0) {
            return ncm_error_set_status(ncm_error, status,
                                        STRLIT("song is not in playlist view"));
        }
        return 1;
    }
    ncm_error_clear(ncm_error);
    return 0;
}

int32
playlist_edit_screen_locate_song(PlaylistEditScreen *screen,
                                 MpdClient *client, NcmSong *song,
                                 NcmError *ncm_error) {
    NcMenu *playlists;
    NcMenu *content;
    int32 playlist_pos;
    int32 song_pos;
    int32 found_pos;
    int32 playlist_count;
    int32 content_count;
    int32 status;

    if ((screen == NULL) || (song == NULL)) {
        return ncm_error_set_status(ncm_error, -EINVAL, STRLIT("missing song"));
    }
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    if ((nc_menu_all_item_count(playlists) <= 0)
        || screen->playlists_update_requested) {
        status = playlist_edit_screen_reload_playlists_from_mpd(screen, client,
                                                               ncm_error);
        if (status < 0) {
            return status;
        }
    }
    if (nc_menu_all_item_count(playlists) <= 0) {
        return ncm_error_set_status(ncm_error, -ENOENT,
                                    STRLIT("playlist list is empty"));
    }

    playlist_edit_clear_content_filter(screen);
    playlist_edit_clear_playlist_filter(screen);
    playlists = nc_playlist_entry_menu_base(&screen->playlists);
    content = nc_song_menu_base(&screen->content);
    playlist_count = nc_menu_all_item_count(playlists);
    content_count = nc_menu_all_item_count(content);
    playlist_pos = nc_menu_highlight(playlists);
    song_pos = nc_menu_highlight(content);
    if (song_pos < 0) {
        song_pos = 0;
    }

    found_pos = playlist_edit_find_song_in_content_range(screen, song,
                                                         song_pos + 1,
                                                         content_count);
    if (found_pos >= 0) {
        playlist_edit_highlight_content_position(screen, found_pos);
        return playlist_edit_show_screen(screen);
    }

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Jumping to song..."));
    status = playlist_edit_locate_song_in_playlist_range(screen, client, song,
                                                         playlist_pos + 1,
                                                         playlist_count,
                                                         ncm_error);
    if (status > 0) {
        return playlist_edit_show_screen(screen);
    }
    if (status < 0) {
        return status;
    }

    status = playlist_edit_locate_song_in_playlist_range(screen, client, song,
                                                         0, playlist_pos,
                                                         ncm_error);
    if (status > 0) {
        return playlist_edit_show_screen(screen);
    }
    if (status < 0) {
        return status;
    }

    found_pos = playlist_edit_find_song_in_content_range(screen, song,
                                                         0, song_pos);
    if (found_pos >= 0) {
        playlist_edit_highlight_content_position(screen, found_pos);
        return playlist_edit_show_screen(screen);
    }

    {
        NcmSong current_song = {0};

        status = playlist_edit_screen_current_content_song(screen,
                                                           &current_song);
        if ((status > 0) && ncm_song_is_equal(&current_song, song)) {
            ncm_song_destroy(&current_song);
            screen->active_column = PLAYLIST_EDITOR_COLUMN_CONTENT;
            playlist_edit_update_menu_highlights(screen);
            return playlist_edit_show_screen(screen);
        }
        ncm_song_destroy(&current_song);
    }

    ncm_statusbar_print(Config.message_delay_time,
                        STRLIT("Song was not found in playlists"));
    return ncm_error_set_status(ncm_error, -ENOENT,
                                STRLIT("song was not found in playlists"));
}

int32
playlist_edit_screen_current_playlist(PlaylistEditScreen *screen,
                                      NcmPlaylist *playlist) {
    NcmPlaylist *current;

    if ((screen == NULL) || (playlist == NULL)) {
        return -EINVAL;
    }
    if ((current = nc_playlist_entry_menu_current(&screen->playlists))
        == NULL) {
        return 0;
    }
    ncm_playlist_copy(playlist, current);
    return 1;
}

int32
playlist_edit_screen_current_song(PlaylistEditScreen *screen, NcmSong *song) {
    return playlist_edit_screen_current_content_song(screen, song);
}

int32
playlist_edit_screen_current_content_song(PlaylistEditScreen *screen,
                                          NcmSong *song) {
    NcmSong *current;

    if ((screen == NULL) || (song == NULL)) {
        return -EINVAL;
    }
    if ((current = nc_song_menu_current(&screen->content)) == NULL) {
        return 0;
    }
    ncm_song_copy(song, current);
    return 1;
}

int32
playlist_edit_screen_selected_playlist_count(PlaylistEditScreen *screen) {
    NcMenu *menu;

    if (screen == NULL) {
        return 0;
    }
    menu = nc_playlist_entry_menu_base(&screen->playlists);
    return nc_menu_selected_count(menu);
}

static int32
append_content_item_from_source(PlaylistEditScreen *screen,
                                enum NcMenuItemSource source, int32 pos,
                                NcmSongArray *songs) {
    NcmSong *song;

    song = nc_menu_item_at(nc_song_menu_base(&screen->content), source, pos);
    if (song == NULL) {
        return -ENOENT;
    }
    ncm_song_array_append_copy(songs, song);
    return 0;
}

static int32
append_content_item(PlaylistEditScreen *screen, int32 pos,
                    NcmSongArray *songs) {
    enum NcMenuItemSource source = NC_MENU_ITEMS_ALL;
    NcMenu *menu = nc_song_menu_base(&screen->content);

    if (nc_menu_is_filtered(menu)) {
        source = NC_MENU_ITEMS_FILTERED;
    }
    return append_content_item_from_source(screen, source, pos, songs);
}

int32
playlist_edit_screen_selected_songs(PlaylistEditScreen *screen,
                                    NcmSongArray *songs) {
    int32 status;

    if (songs) {
        ncm_song_array_clear(songs);
    }
    if ((screen == NULL) || (songs == NULL)) {
        return -EINVAL;
    }
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        NcMenu *menu = nc_song_menu_base(&screen->content);
        if (!nc_menu_has_selected(menu)) {
            return append_content_item(screen, nc_menu_highlight(menu), songs);
        }
        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            if (!nc_menu_position_is_selected(menu, i)) {
                continue;
            }
            append_content_item(screen, i, songs);
        }
        return 0;
    }
    if (playlist_edit_screen_selected_playlist_count(screen) > 0) {
        NcMenu *menu = nc_playlist_entry_menu_base(&screen->playlists);
        for (int32 i = 0; i < nc_menu_item_count(menu); i += 1) {
            NcmMpdSongList list = {0};
            NcmError ncm_error = {0};
            NcmPlaylist *playlist;

            if (!nc_menu_position_is_selected(menu, i)) {
                continue;
            }
            playlist = nc_menu_active_item_at(menu, i);

            ncm_error_clear(&ncm_error);
            status = ncm_mpd_client_get_playlist_content(&global_mpd,
                                                         playlist->path,
                                                         &list, &ncm_error);
            if (status < 0) {
                char *context = "Could not fetch playlist content";

                playlist_edit_report_error(context, strlen32(context),
                                           &ncm_error);
                ncm_error_clear(&ncm_error);
                ncm_mpd_song_list_destroy(&list);
                ncm_song_array_clear(songs);
                return status;
            }

            for (int32 j = 0; j < list.count; j += 1) {
                ncm_song_array_append_copy(songs, &list.items[j]);
            }
            ncm_mpd_song_list_destroy(&list);
        }
        return 0;
    }

    {
        NcMenu *menu = nc_song_menu_base(&screen->content);
        for (int32 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
            append_content_item_from_source(screen, NC_MENU_ITEMS_ALL,
                                            i, songs);
        }
    }
    return 0;
}

int32
playlist_edit_screen_apply_active_filter(PlaylistEditScreen *screen,
                                         char *pattern, int32 pattern_len,
                                         uint32 regex_flags,
                                         NcmError *ncm_error) {
    NcMenu *menu;
    NcmRegex *regex;
    StrBuilder *constraint;
    bool *enabled;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
    }

    menu = playlist_edit_screen_active_menu(screen);
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        regex = &screen->content_filter_regex;
        constraint = &screen->content_filter_constraint;
        enabled = &screen->content_filter_enabled;
    } else {
        regex = &screen->playlist_filter_regex;
        constraint = &screen->playlist_filter_constraint;
        enabled = &screen->playlist_filter_enabled;
    }

    if ((pattern == NULL) || (pattern_len <= 0)) {
        *enabled = false;
        sb_clear(constraint);
        nc_menu_show_all_items(menu);
        playlist_edit_update_titles(screen, true);
        return ncm_error_ok(ncm_error);
    }
    if ((status = ncm_regex_compile(regex, pattern, pattern_len,
                                    regex_flags, ncm_error)) < 0) {
        return status;
    }
    sb_set(constraint, pattern, pattern_len);
    *enabled = true;
    nc_menu_apply_filter(menu);
    playlist_edit_update_titles(screen, true);
    return ncm_error_ok(ncm_error);
}

static bool
playlist_edit_search_position(NcMenu *menu, int32 pos, void *user) {
    NcmRegex *regex = user;
    void *item = nc_menu_active_item_at(menu, pos);

    if (menu->item_callbacks.item_size == SIZEOF(NcmPlaylist)) {
        NcmPlaylist *playlist = item;

        return playlist_edit_playlist_matches_regex(regex, playlist);
    }
    return playlist_edit_content_matches_regex(regex, item);
}

int32
playlist_edit_screen_search_active(PlaylistEditScreen *screen,
                                   char *pattern, int32 pattern_len,
                                   uint32 regex_flags,
                                   bool forward, bool wrap, bool skip_current,
                                   NcmError *ncm_error) {
    StrBuilder *constraint;
    NcmRegex *regex;
    NcMenu *menu;
    bool *enabled;
    int32 status;

    if (screen == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist editor"));
    }
    if (screen->active_column == PLAYLIST_EDITOR_COLUMN_CONTENT) {
        regex = &screen->content_search_regex;
        constraint = &screen->content_search_constraint;
        enabled = &screen->content_search_enabled;
    } else {
        regex = &screen->playlist_search_regex;
        constraint = &screen->playlist_search_constraint;
        enabled = &screen->playlist_search_enabled;
    }
    if ((pattern == NULL) || (pattern_len <= 0)) {
        *enabled = false;
        sb_clear(constraint);
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing search pattern"));
    }
    if ((status = ncm_regex_compile(regex, pattern, pattern_len,
                                    regex_flags, ncm_error)) < 0) {
        return status;
    }
    sb_set(constraint, pattern, pattern_len);
    *enabled = true;
    menu = playlist_edit_screen_active_menu(screen);
    if (nc_menu_search_selectable(menu, screen->main_height, forward,
                                  wrap, skip_current,
                                  playlist_edit_search_position,
                                  regex, NULL) == 0) {
        playlist_edit_finish_playlist_change(screen);
        return 1;
    }
    return 0;
}

void
playlist_edit_screen_request_playlists_update(PlaylistEditScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->playlists_update_requested = true;
    nc_screen_request_update(&screen->screen);
    return;
}

void
playlist_edit_screen_request_content_update(PlaylistEditScreen *screen) {
    if (screen == NULL) {
        return;
    }
    screen->content_update_requested = true;
    playlist_edit_reset_content_timer(screen);
    nc_screen_request_update(&screen->screen);
    return;
}

#endif /* NC_PLAYLIST_EDITOR_C */
