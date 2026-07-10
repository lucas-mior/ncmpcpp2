#include "screens/nc_sort_playlist.h"

#include <errno.h>

#include "app_controller.h"
#include "c/ncm_base.h"
#include "c/ncm_playlist_sort.h"
#include "c/ncm_string.h"
#include "cbase/base_macros.h"
#include "screens/nc_playlist.h"
#include "screens/screen_switcher.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

static NativeSortPlaylistDialog *sort_dialog_from_screen(NcScreen *screen);
static NcScreenCallbacks sort_dialog_callbacks(void);
static NcWindow *sort_dialog_active_window_callback(NcScreen *screen);
static void sort_dialog_refresh_callback(NcScreen *screen);
static void sort_dialog_scroll_callback(NcScreen *screen,
                                        enum NcScroll where);
static bool sort_dialog_can_run_current_callback(NcScreen *screen);
static bool sort_dialog_run_current_callback(NcScreen *screen);
static void sort_dialog_switch_to_callback(NcScreen *screen);
static void sort_dialog_resize_callback(NcScreen *screen);
static int32 sort_dialog_timeout_callback(NcScreen *screen);
static char *sort_dialog_title_callback(NcScreen *screen);
static void sort_dialog_update_callback(NcScreen *screen);
static void sort_dialog_mouse_callback(NcScreen *screen, MEVENT event);
static bool sort_dialog_lockable_callback(NcScreen *screen);
static bool sort_dialog_mergable_callback(NcScreen *screen);
static void sort_dialog_destroy_callback(NcScreen *screen);
static bool sort_dialog_position_is_sort_key(NcMenu *menu, int64 pos);
static void sort_dialog_show_move_hint(void *user);
static void sort_dialog_run_sort(void *user);
static void sort_dialog_cancel(void *user);
static bool sort_dialog_label_set(NcEditorSortRow *row, char *label,
                                  int32 label_len);
static void sort_dialog_apply_geometry(NativeSortPlaylistDialog *dialog);
static void sort_dialog_finish(NativeSortPlaylistDialog *dialog);

void
native_sort_playlist_dialog_init(NativeSortPlaylistDialog *dialog,
                                 int64 start_x, int64 start_y,
                                 int64 width, int64 height,
                                 NcColor color, NcBorder border) {
    NcScreenCallbacks callbacks;

    callbacks = sort_dialog_callbacks();
    nc_editor_sort_menu_init(&dialog->rows);
    nc_window_init(&dialog->window, start_x, start_y, width, height,
                   STRLIT_ARGS("Sort songs by..."), color, border);
    ncm_song_array_init(&dialog->songs);
    dialog->playlist = NULL;
    dialog->previous_screen = NULL;
    dialog->client = NULL;
    dialog->start_x = start_x;
    dialog->start_y = start_y;
    dialog->width = width;
    dialog->height = height;
    dialog->start_position = 0;
    dialog->ignore_leading_the = false;
    dialog->ready = false;
    nc_screen_init(&dialog->screen, callbacks, dialog,
                   NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG);
    native_sort_playlist_dialog_populate_defaults(dialog);
    return;
}

void
native_sort_playlist_dialog_destroy(NativeSortPlaylistDialog *dialog) {
    if (dialog == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(
        native_sort_playlist_dialog_base(dialog));
    ncm_song_array_destroy(&dialog->songs);
    nc_editor_sort_menu_destroy(&dialog->rows);
    nc_window_destroy(&dialog->window);
    dialog->playlist = NULL;
    dialog->previous_screen = NULL;
    dialog->client = NULL;
    dialog->ready = false;
    return;
}

NcScreen *
native_sort_playlist_dialog_base(NativeSortPlaylistDialog *dialog) {
    if (dialog == NULL) {
        return NULL;
    }
    return &dialog->screen;
}

NcEditorSortMenu *
native_sort_playlist_dialog_menu(NativeSortPlaylistDialog *dialog) {
    if (dialog == NULL) {
        return NULL;
    }
    return &dialog->rows;
}

void
native_sort_playlist_dialog_set_geometry(NativeSortPlaylistDialog *dialog,
                                         int64 start_x, int64 start_y,
                                         int64 width, int64 height) {
    if (dialog == NULL) {
        return;
    }
    dialog->start_x = start_x;
    dialog->start_y = start_y;
    dialog->width = width;
    dialog->height = height;
    nc_window_resize(&dialog->window, width, height);
    nc_window_move_to(&dialog->window, start_x, start_y);
    return;
}

void
native_sort_playlist_dialog_populate_defaults(
    NativeSortPlaylistDialog *dialog) {
    if (dialog == NULL) {
        return;
    }
    nc_menu_clear_items(nc_editor_sort_menu_base(&dialog->rows));
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Artist"), NCM_SONG_GETTER_ARTIST,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Album artist"),
        NCM_SONG_GETTER_ALBUM_ARTIST, sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Album"), NCM_SONG_GETTER_ALBUM,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Disc"), NCM_SONG_GETTER_DISC,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Track"), NCM_SONG_GETTER_TRACK,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Genre"), NCM_SONG_GETTER_GENRE,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Date"), NCM_SONG_GETTER_DATE,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Composer"), NCM_SONG_GETTER_COMPOSER,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Performer"), NCM_SONG_GETTER_PERFORMER,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Title"), NCM_SONG_GETTER_TITLE,
        sort_dialog_show_move_hint, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Filename"), NCM_SONG_GETTER_URI,
        sort_dialog_show_move_hint, dialog);
    nc_editor_sort_menu_add_separator(&dialog->rows);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Sort"), NCM_SONG_GETTER_NONE,
        sort_dialog_run_sort, dialog);
    (void)native_sort_playlist_dialog_add_row(
        dialog, STRLIT_ARGS("Cancel"), NCM_SONG_GETTER_NONE,
        sort_dialog_cancel, dialog);
    return;
}

bool
native_sort_playlist_dialog_add_row(NativeSortPlaylistDialog *dialog,
                                    char *label, int32 label_len,
                                    enum NcmSongGetter getter,
                                    void (*run)(void *user), void *user) {
    NcEditorSortRow row;
    bool ok;

    if (dialog == NULL) {
        return false;
    }
    nc_editor_sort_row_init(&row);
    row.getter = getter;
    row.action.run = run;
    row.action.user = user;
    ok = sort_dialog_label_set(&row, label, label_len);
    if (ok) {
        nc_editor_sort_menu_add(&dialog->rows, &row);
    }
    nc_editor_sort_row_destroy(&row);
    return ok;
}

bool
native_sort_playlist_dialog_open(
    NativeSortPlaylistDialog *dialog, NativePlaylistScreen *playlist,
    NcmMpdClient *client, bool ignore_leading_the, NcmError *error) {
    NcmSongArray songs;
    NcScreen *current;
    uint32 start_position;

    if (dialog == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing sort dialog"));
        return false;
    }
    if (playlist == NULL) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("missing native playlist"));
        return false;
    }
    if (client == NULL) {
        ncm_error_set(error, EINVAL, STRLIT_ARGS("missing MPD client"));
        return false;
    }

    current = nc_screen_switcher_current();
    if (current != native_playlist_screen_base(playlist)) {
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("sort dialog requires native playlist"));
        return false;
    }

    ncm_song_array_init(&songs);
    if (!native_playlist_screen_copy_sort_range(
            playlist, &songs, &start_position, error)) {
        ncm_song_array_destroy(&songs);
        return false;
    }

    native_sort_playlist_dialog_populate_defaults(dialog);
    nc_menu_reset(nc_editor_sort_menu_base(&dialog->rows));
    sort_dialog_apply_geometry(dialog);

    ncm_song_array_destroy(&dialog->songs);
    dialog->songs = songs;
    dialog->playlist = playlist;
    dialog->previous_screen = current;
    dialog->client = client;
    dialog->start_position = start_position;
    dialog->ignore_leading_the = ignore_leading_the;
    dialog->ready = true;

    if (!nc_screen_switcher_switch_to(
            native_sort_playlist_dialog_base(dialog), false)) {
        ncm_song_array_clear(&dialog->songs);
        dialog->playlist = NULL;
        dialog->previous_screen = NULL;
        dialog->client = NULL;
        dialog->ready = false;
        ncm_error_set(error, EINVAL,
                      STRLIT_ARGS("sort dialog is not registered"));
        return false;
    }

    ncm_error_clear(error);
    return true;
}

bool
native_sort_playlist_dialog_move_current_up(
    NativeSortPlaylistDialog *dialog) {
    NcMenu *menu;
    int64 pos;

    if (dialog == NULL) {
        return false;
    }
    menu = nc_editor_sort_menu_base(&dialog->rows);
    pos = nc_menu_highlight(menu);
    if (pos <= 0 || !sort_dialog_position_is_sort_key(menu, pos)) {
        return false;
    }
    if (!sort_dialog_position_is_sort_key(menu, pos - 1)) {
        return false;
    }
    nc_menu_swap_item_slots(menu, NC_MENU_ITEMS_ALL, pos, pos - 1);
    nc_menu_highlight_position(menu, pos - 1, nc_menu_item_count(menu));
    return true;
}

bool
native_sort_playlist_dialog_move_current_down(
    NativeSortPlaylistDialog *dialog) {
    NcMenu *menu;
    int64 pos;

    if (dialog == NULL) {
        return false;
    }
    menu = nc_editor_sort_menu_base(&dialog->rows);
    pos = nc_menu_highlight(menu);
    if (!sort_dialog_position_is_sort_key(menu, pos)) {
        return false;
    }
    if (!sort_dialog_position_is_sort_key(menu, pos + 1)) {
        return false;
    }
    nc_menu_swap_item_slots(menu, NC_MENU_ITEMS_ALL, pos, pos + 1);
    nc_menu_highlight_position(menu, pos + 1, nc_menu_item_count(menu));
    return true;
}

bool
native_sort_playlist_dialog_run_current(NativeSortPlaylistDialog *dialog) {
    NcEditorSortRow *row;

    if (dialog == NULL || !dialog->ready) {
        return false;
    }
    row = nc_editor_sort_menu_current(&dialog->rows);
    if (row == NULL || row->action.run == NULL) {
        return false;
    }
    row->action.run(row->action.user);
    return true;
}

int32
native_sort_playlist_dialog_get_order(
    NativeSortPlaylistDialog *dialog, enum NcmSongGetter *getters,
    int32 getters_cap) {
    NcMenu *menu;
    int32 len;

    if (dialog == NULL || getters == NULL || getters_cap <= 0) {
        return 0;
    }
    menu = nc_editor_sort_menu_base(&dialog->rows);
    len = 0;
    for (int64 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        NcEditorSortRow *row;

        row = nc_editor_sort_menu_item_at(&dialog->rows,
                                          NC_MENU_ITEMS_ALL, i);
        if (row == NULL || row->getter == NCM_SONG_GETTER_NONE) {
            continue;
        }
        if (len >= getters_cap) {
            break;
        }
        getters[len] = row->getter;
        len += 1;
    }
    return len;
}

static NativeSortPlaylistDialog *
sort_dialog_from_screen(NcScreen *screen) {
    return nc_screen_user(screen);
}

static NcScreenCallbacks
sort_dialog_callbacks(void) {
    NcScreenCallbacks callbacks = {0};

    callbacks.active_window = sort_dialog_active_window_callback;
    callbacks.refresh = sort_dialog_refresh_callback;
    callbacks.refresh_window = sort_dialog_refresh_callback;
    callbacks.scroll = sort_dialog_scroll_callback;
    callbacks.can_run_current = sort_dialog_can_run_current_callback;
    callbacks.run_current = sort_dialog_run_current_callback;
    callbacks.switch_to = sort_dialog_switch_to_callback;
    callbacks.resize = sort_dialog_resize_callback;
    callbacks.window_timeout = sort_dialog_timeout_callback;
    callbacks.title = sort_dialog_title_callback;
    callbacks.update = sort_dialog_update_callback;
    callbacks.mouse_button_pressed = sort_dialog_mouse_callback;
    callbacks.is_lockable = sort_dialog_lockable_callback;
    callbacks.is_mergable = sort_dialog_mergable_callback;
    callbacks.destroy = sort_dialog_destroy_callback;
    return callbacks;
}

static NcWindow *
sort_dialog_active_window_callback(NcScreen *screen) {
    return &sort_dialog_from_screen(screen)->window;
}

static void
sort_dialog_refresh_callback(NcScreen *screen) {
    NativeSortPlaylistDialog *dialog;
    NcMenu *menu;

    dialog = sort_dialog_from_screen(screen);
    menu = nc_editor_sort_menu_base(&dialog->rows);
    nc_menu_prepare_refresh(menu, dialog->height, NULL, NULL);
    nc_window_display(&dialog->window);
    nc_menu_refresh(menu, &dialog->window, dialog->width, dialog->height);
    return;
}

static void
sort_dialog_scroll_callback(NcScreen *screen, enum NcScroll where) {
    NativeSortPlaylistDialog *dialog;

    dialog = sort_dialog_from_screen(screen);
    nc_menu_scroll_selectable(nc_editor_sort_menu_base(&dialog->rows),
                              dialog->height, where);
    return;
}

static bool
sort_dialog_can_run_current_callback(NcScreen *screen) {
    NativeSortPlaylistDialog *dialog;
    NcEditorSortRow *row;

    dialog = sort_dialog_from_screen(screen);
    if (dialog == NULL || !dialog->ready) {
        return false;
    }
    row = nc_editor_sort_menu_current(&dialog->rows);
    return (row != NULL) && (row->action.run != NULL);
}

static bool
sort_dialog_run_current_callback(NcScreen *screen) {
    return native_sort_playlist_dialog_run_current(
        sort_dialog_from_screen(screen));
}

static void
sort_dialog_switch_to_callback(NcScreen *screen) {
    (void)screen;
    return;
}

static void
sort_dialog_resize_callback(NcScreen *screen) {
    sort_dialog_apply_geometry(sort_dialog_from_screen(screen));
    nc_screen_clear_resize_request(screen);
    return;
}

static int32
sort_dialog_timeout_callback(NcScreen *screen) {
    (void)screen;
    return NC_SCREEN_DEFAULT_WINDOW_TIMEOUT;
}

static char *
sort_dialog_title_callback(NcScreen *screen) {
    NativeSortPlaylistDialog *dialog;

    dialog = sort_dialog_from_screen(screen);
    if (dialog->previous_screen != NULL) {
        return nc_screen_title(dialog->previous_screen);
    }
    return (char *)"Sort playlist";
}

static void
sort_dialog_update_callback(NcScreen *screen) {
    nc_screen_clear_update_request(screen);
    return;
}

static void
sort_dialog_mouse_callback(NcScreen *screen, MEVENT event) {
    NativeSortPlaylistDialog *dialog;
    int32 x;
    int32 y;

    dialog = sort_dialog_from_screen(screen);
    x = event.x;
    y = event.y;
    if (!nc_window_has_coords(&dialog->window, &x, &y)) {
        return;
    }
    if (event.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED)) {
        (void)nc_menu_goto_selectable(nc_editor_sort_menu_base(
                                          &dialog->rows), y);
        if (event.bstate & BUTTON3_PRESSED) {
            (void)native_sort_playlist_dialog_run_current(dialog);
        }
    }
    return;
}

static bool
sort_dialog_lockable_callback(NcScreen *screen) {
    (void)screen;
    return false;
}

static bool
sort_dialog_mergable_callback(NcScreen *screen) {
    (void)screen;
    return false;
}

static void
sort_dialog_destroy_callback(NcScreen *screen) {
    native_sort_playlist_dialog_destroy(sort_dialog_from_screen(screen));
    return;
}

static bool
sort_dialog_position_is_sort_key(NcMenu *menu, int64 pos) {
    NcEditorSortRow *row;

    row = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, pos);
    if (row == NULL) {
        return false;
    }
    return row->getter != NCM_SONG_GETTER_NONE;
}

static void
sort_dialog_show_move_hint(void *user) {
    (void)user;
    ncm_statusbar_print_cstring(
        (int32)Config.message_delay_time,
        (char *)"Move tag types up and down to adjust sort order");
    return;
}

static void
sort_dialog_run_sort(void *user) {
    enum NcmSongGetter getters[16];
    NativeSortPlaylistDialog *dialog;
    NcmError error;
    bool success;
    int32 getters_len;

    dialog = user;
    if (dialog == NULL || !dialog->ready) {
        return;
    }

    getters_len = native_sort_playlist_dialog_get_order(
        dialog, getters, NCM_ARRAY_LEN(getters));
    ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                (char *)"Sorting...");
    ncm_error_clear(&error);
    success = ncm_playlist_sort_range(
        &dialog->songs, dialog->start_position, getters, getters_len,
        dialog->ignore_leading_the, dialog->client, &error);
    if (success) {
        success = ncm_status_update_full(dialog->client, NULL, &error);
    }

    if (success) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Range sorted");
    } else if (ncm_error_is_set(&error)) {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    error.message);
    } else {
        ncm_statusbar_print_cstring((int32)Config.message_delay_time,
                                    (char *)"Could not sort playlist");
    }

    sort_dialog_finish(dialog);
    return;
}

static void
sort_dialog_cancel(void *user) {
    NativeSortPlaylistDialog *dialog;

    dialog = user;
    if (dialog == NULL || !dialog->ready) {
        return;
    }
    sort_dialog_finish(dialog);
    return;
}

static bool
sort_dialog_label_set(NcEditorSortRow *row, char *label, int32 label_len) {
    if (row == NULL) {
        return false;
    }
    if (label == NULL || label_len <= 0) {
        return true;
    }
    row->action.label_cap = label_len + 1;
    row->action.label = ncm_malloc(row->action.label_cap);
    ncm_memcpy(row->action.label, label, label_len);
    row->action.label[label_len] = '\0';
    row->action.label_len = label_len;
    return true;
}

static void
sort_dialog_apply_geometry(NativeSortPlaylistDialog *dialog) {
    int64 main_height;
    int64 height;
    int64 width;
    int64 start_x;
    int64 start_y;

    main_height = ui_state_main_height();
    height = main_height;
    if (height > 17) {
        height = 17;
    }
    if (height < 0) {
        height = 0;
    }
    width = 30;
    start_x = (ui_state_screen_width() - width)/2;
    start_y = (main_height - height)/2 + ui_state_main_start_y();
    native_sort_playlist_dialog_set_geometry(
        dialog, start_x, start_y, width, height);
    return;
}

static void
sort_dialog_finish(NativeSortPlaylistDialog *dialog) {
    NcScreen *previous;

    previous = dialog->previous_screen;
    dialog->ready = false;
    if (previous != NULL) {
        (void)nc_screen_switcher_switch_to(
            previous, nc_screen_has_to_be_resized(previous));
    }

    ncm_song_array_clear(&dialog->songs);
    dialog->playlist = NULL;
    dialog->previous_screen = NULL;
    dialog->client = NULL;
    dialog->start_position = 0;
    dialog->ignore_leading_the = false;
    return;
}
