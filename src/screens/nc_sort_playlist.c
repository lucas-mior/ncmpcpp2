#if !defined(NCMPCPP_NC_SORT_PLAYLIST_C)
#define NCMPCPP_NC_SORT_PLAYLIST_C

#include "cbase.h"

#include "app_controller.h"
#include "c/ncm_c.h"
#include "screens/nc_screens.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "ui_state.h"

static void sort_dialog_refresh_rows(SortPlaylistDialog *dialog);
static void sort_dialog_draw_row(NcMenu *menu, NcWindow *window,
                                 void *item, int32 pos,
                                 void *user);
static bool sort_dialog_can_run_current_callback(NcScreen *screen);
static int32 sort_dialog_run_current_callback(NcScreen *screen);
static void sort_dialog_switch_to_callback(NcScreen *screen);
static void sort_dialog_resize_callback(NcScreen *screen);
static char *sort_dialog_title_callback(NcScreen *screen);
static void sort_dialog_update_callback(NcScreen *screen);
static void sort_dialog_mouse_callback(NcScreen *screen, MEVENT event);
static bool sort_dialog_position_is_sort_key(NcMenu *menu, int32 pos);
static void sort_dialog_show_move_hint(void *user);
static void sort_dialog_run_sort(void *user);
static void sort_dialog_cancel(void *user);
static void sort_dialog_label_set(NcEditorSortRow *row, char *label,
                                  int32 label_len);
static void sort_dialog_apply_geometry(SortPlaylistDialog *dialog);
static void sort_dialog_finish(SortPlaylistDialog *dialog);

#define NC_SCREEN_IMPL_TYPE SortPlaylistDialog
#define NC_SCREEN_IMPL_PREFIX sort_dialog
#define NC_SCREEN_IMPL_PUBLIC_PREFIX sort_playlist_dialog
#define NC_SCREEN_IMPL_BASE_FIELD screen
#define NC_SCREEN_IMPL_WINDOW_FIELD window
#define NC_SCREEN_IMPL_MENU(dialog) nc_editor_sort_menu_base(&(dialog)->rows)
#define NC_SCREEN_IMPL_REFRESH_CALLBACK sort_dialog_refresh_rows
#define NC_SCREEN_IMPL_CAN_RUN_CURRENT_CALLBACK \
    sort_dialog_can_run_current_callback
#define NC_SCREEN_IMPL_RUN_CURRENT_CALLBACK sort_dialog_run_current_callback
#define NC_SCREEN_IMPL_SWITCH_TO_CALLBACK sort_dialog_switch_to_callback
#define NC_SCREEN_IMPL_RESIZE_CALLBACK sort_dialog_resize_callback
#define NC_SCREEN_IMPL_TITLE_CALLBACK sort_dialog_title_callback
#define NC_SCREEN_IMPL_UPDATE_CALLBACK sort_dialog_update_callback
#define NC_SCREEN_IMPL_MOUSE_CALLBACK sort_dialog_mouse_callback
#define NC_SCREEN_IMPL_DESTROY_TYPED_CALLBACK \
    sort_playlist_dialog_destroy
#include "screens/nc_screen_impl_template.h"

void
sort_playlist_dialog_init(SortPlaylistDialog *dialog,
                          int32 start_x, int32 start_y,
                          int32 width, int32 height,
                          NcColor color, NcBorder border) {
    NcMenuDisplayCallbacks display_callbacks = {0};
    NcMenu *menu;

    nc_editor_sort_menu_init(&dialog->rows);
    menu = nc_editor_sort_menu_base(&dialog->rows);
    display_callbacks.draw = sort_dialog_draw_row;
    nc_menu_set_display_callbacks(menu, display_callbacks);
    nc_menu_set_highlight_prefix(menu, &Config.current_item_prefix);
    nc_menu_set_highlight_suffix(menu, &Config.current_item_suffix);
    nc_menu_set_cyclic_scrolling(menu, Config.use_cyclic_scrolling);
    nc_menu_set_centered_cursor(menu, Config.centered_cursor);
    nc_window_init(&dialog->window, start_x, start_y, width, height,
                   STRLIT("Sort songs by..."), color, border);
    dialog->songs = (NcmSongArray){0};
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
    nc_screen_init_ops(&dialog->screen, sort_dialog_ops, dialog,
                       NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG);
    sort_playlist_dialog_populate_defaults(dialog);
    return;
}

void
sort_playlist_dialog_destroy(SortPlaylistDialog *dialog) {
    if (dialog == NULL) {
        return;
    }
    (void)app_controller_unregister_screen(
        sort_playlist_dialog_base(dialog));
    ncm_song_array_destroy(&dialog->songs);
    nc_editor_sort_menu_destroy(&dialog->rows);
    nc_window_destroy(&dialog->window);
    dialog->playlist = NULL;
    dialog->previous_screen = NULL;
    dialog->client = NULL;
    dialog->ready = false;
    return;
}

NcEditorSortMenu *
sort_playlist_dialog_menu(SortPlaylistDialog *dialog) {
    if (dialog == NULL) {
        return NULL;
    }
    return &dialog->rows;
}

void
sort_playlist_dialog_set_geometry(SortPlaylistDialog *dialog,
                                  int32 start_x, int32 start_y,
                                  int32 width, int32 height) {
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
sort_playlist_dialog_populate_defaults(
    SortPlaylistDialog *dialog
) {
    if (dialog == NULL) {
        return;
    }
    nc_menu_clear_items(nc_editor_sort_menu_base(&dialog->rows));
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Artist"), NCM_SONG_GETTER_ARTIST,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Album artist"),
        NCM_SONG_GETTER_ALBUM_ARTIST, sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Album"), NCM_SONG_GETTER_ALBUM,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Disc"), NCM_SONG_GETTER_DISC,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Track"), NCM_SONG_GETTER_TRACK,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Genre"), NCM_SONG_GETTER_GENRE,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Date"), NCM_SONG_GETTER_DATE,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Composer"), NCM_SONG_GETTER_COMPOSER,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Performer"), NCM_SONG_GETTER_PERFORMER,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Title"), NCM_SONG_GETTER_TITLE,
        sort_dialog_show_move_hint, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Filename"), NCM_SONG_GETTER_URI,
        sort_dialog_show_move_hint, dialog);
    nc_editor_sort_menu_add_separator(&dialog->rows);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Sort"), NCM_SONG_GETTER_NONE,
        sort_dialog_run_sort, dialog);
    (void)sort_playlist_dialog_add_row(
        dialog, STRLIT("Cancel"), NCM_SONG_GETTER_NONE,
        sort_dialog_cancel, dialog);
    return;
}

int32
sort_playlist_dialog_add_row(SortPlaylistDialog *dialog,
                             char *label, int32 label_len,
                             enum NcmSongGetter getter,
                             void (*run)(void *user), void *user) {
    NcEditorSortRow row;

    if (dialog == NULL) {
        return -EINVAL;
    }
    row = (NcEditorSortRow){0};
    row.getter = getter;
    row.action.run = run;
    row.action.user = user;
    sort_dialog_label_set(&row, label, label_len);
    nc_editor_sort_menu_add(&dialog->rows, &row);
    nc_editor_sort_row_destroy(&row);
    return 0;
}

int32
sort_playlist_dialog_open(
    SortPlaylistDialog *dialog, PlaylistScreen *playlist,
    NcmMpdClient *client, bool ignore_leading_the, NcmError *ncm_error
) {
    NcmSongArray songs;
    NcScreen *current;
    int32 start_position;
    int32 status;

    if (dialog == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing sort dialog"));
    }
    if (playlist == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing playlist screen"));
    }
    if (client == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing MPD client"));
    }

    current = nc_screen_switcher_current();
    if (current != playlist_screen_base(playlist)) {
        return ncm_error_set_status(
            ncm_error, -EINVAL,
            STRLIT("sort dialog requires playlist screen"));
    }

    songs = (NcmSongArray){0};
    status = playlist_screen_copy_sort_range(
        playlist, &songs, &start_position, ncm_error);
    if (status < 0) {
        ncm_song_array_destroy(&songs);
        return status;
    }

    sort_playlist_dialog_populate_defaults(dialog);
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

    status = nc_screen_switcher_switch_to(
        sort_playlist_dialog_base(dialog), false);
    if (status < 0) {
        ncm_song_array_clear(&dialog->songs);
        dialog->playlist = NULL;
        dialog->previous_screen = NULL;
        dialog->client = NULL;
        dialog->ready = false;
        return ncm_error_set_status(
            ncm_error, status, STRLIT("sort dialog is not registered"));
    }

    ncm_error_clear(ncm_error);
    return 0;
}

int32
sort_playlist_dialog_move_current_up(
    SortPlaylistDialog *dialog
) {
    NcMenu *menu;
    int32 pos;

    if (dialog == NULL) {
        return -EINVAL;
    }
    menu = nc_editor_sort_menu_base(&dialog->rows);
    pos = nc_menu_highlight(menu);
    if ((pos <= 0) || !sort_dialog_position_is_sort_key(menu, pos)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!sort_dialog_position_is_sort_key(menu, pos - 1)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    nc_menu_swap_item_slots(menu, NC_MENU_ITEMS_ALL, pos, pos - 1);
    nc_menu_highlight_position(menu, pos - 1, nc_menu_item_count(menu));
    return 0;
}

int32
sort_playlist_dialog_move_current_down(
    SortPlaylistDialog *dialog
) {
    NcMenu *menu;
    int32 pos;

    if (dialog == NULL) {
        return -EINVAL;
    }
    menu = nc_editor_sort_menu_base(&dialog->rows);
    pos = nc_menu_highlight(menu);
    if (!sort_dialog_position_is_sort_key(menu, pos)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (!sort_dialog_position_is_sort_key(menu, pos + 1)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    nc_menu_swap_item_slots(menu, NC_MENU_ITEMS_ALL, pos, pos + 1);
    nc_menu_highlight_position(menu, pos + 1, nc_menu_item_count(menu));
    return 0;
}

int32
sort_playlist_dialog_run_current(SortPlaylistDialog *dialog) {
    NcEditorSortRow *row;

    if (dialog == NULL) {
        return -EINVAL;
    }
    if (!dialog->ready) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    if (((row = nc_editor_sort_menu_current(&dialog->rows)) == NULL)
        || (row->action.run == NULL)) {
        return -NCM_ERROR_UNAVAILABLE;
    }
    row->action.run(row->action.user);
    return 0;
}

int32
sort_playlist_dialog_get_order(
    SortPlaylistDialog *dialog, enum NcmSongGetter *getters,
    int32 getters_cap
) {
    NcMenu *menu;
    int32 len;

    if ((dialog == NULL) || (getters == NULL) || (getters_cap <= 0)) {
        return 0;
    }
    menu = nc_editor_sort_menu_base(&dialog->rows);
    len = 0;
    for (int32 i = 0; i < nc_menu_all_item_count(menu); i += 1) {
        NcEditorSortRow *row;

        row = nc_editor_sort_menu_item_at(&dialog->rows,
                                          NC_MENU_ITEMS_ALL, i);
        if ((row == NULL) || (row->getter == NCM_SONG_GETTER_NONE)) {
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

static void
sort_dialog_draw_row(NcMenu *menu, NcWindow *window, void *item,
                     int32 pos, void *user) {
    NcEditorSortRow *row;

    (void)menu;
    (void)pos;
    (void)user;
    row = item;
    if ((row == NULL) || (row->action.label == NULL)
        || (row->action.label_len <= 0)) {
        return;
    }
    nc_window_print_data(window, row->action.label,
                         row->action.label_len);
    return;
}

static void
sort_dialog_refresh_rows(SortPlaylistDialog *dialog) {
    NcMenu *menu;

    menu = nc_editor_sort_menu_base(&dialog->rows);
    nc_menu_prepare_refresh(menu, dialog->window.height, NULL, NULL);
    nc_window_display(&dialog->window);
    nc_menu_refresh(menu, &dialog->window, dialog->window.width,
                    dialog->window.height);
    return;
}

static bool
sort_dialog_can_run_current_callback(NcScreen *screen) {
    SortPlaylistDialog *dialog;
    NcEditorSortRow *row;

    if (((dialog = sort_dialog_from_screen(screen)) == NULL)
        || !dialog->ready) {
        return false;
    }
    row = nc_editor_sort_menu_current(&dialog->rows);
    return row && row->action.run;
}

static int32
sort_dialog_run_current_callback(NcScreen *screen) {
    return sort_playlist_dialog_run_current(
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


static char *
sort_dialog_title_callback(NcScreen *screen) {
    SortPlaylistDialog *dialog;

    dialog = sort_dialog_from_screen(screen);
    if (dialog->previous_screen) {
        return nc_screen_title(dialog->previous_screen);
    }
    return "Sort playlist";
}

static void
sort_dialog_update_callback(NcScreen *screen) {
    nc_screen_clear_update_request(screen);
    return;
}

static void
sort_dialog_mouse_callback(NcScreen *screen, MEVENT event) {
    SortPlaylistDialog *dialog;
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
            (void)sort_playlist_dialog_run_current(dialog);
        }
    }
    return;
}

static bool
sort_dialog_position_is_sort_key(NcMenu *menu, int32 pos) {
    NcEditorSortRow *row;

    if ((row = nc_menu_item_at(menu, NC_MENU_ITEMS_ALL, pos)) == NULL) {
        return false;
    }
    return row->getter != NCM_SONG_GETTER_NONE;
}

static void
sort_dialog_show_move_hint(void *user) {
    (void)user;
    ncm_statusbar_print_cstring(
        Config.message_delay_time,
        "Move tag types up and down to adjust sort order");
    return;
}

static void
sort_dialog_run_sort(void *user) {
    enum NcmSongGetter getters[16];
    SortPlaylistDialog *dialog;
    NcmError ncm_error;
    int32 status;
    int32 getters_len;

    dialog = user;
    if ((dialog == NULL) || !dialog->ready) {
        return;
    }

    getters_len = sort_playlist_dialog_get_order(
        dialog, getters, LENGTH(getters));
    ncm_statusbar_print_cstring(Config.message_delay_time,
                                "Sorting...");
    ncm_error_clear(&ncm_error);
    status = ncm_playlist_sort_range(
        &dialog->songs, dialog->start_position, getters, getters_len,
        dialog->ignore_leading_the, dialog->client, &ncm_error);
    if (status == 0) {
        status = ncm_status_update_full(dialog->client, NULL, &ncm_error);
    }

    if (status == 0) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Range sorted");
    } else if (ncm_error_is_set(&ncm_error)) {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    ncm_error.message);
    } else {
        ncm_statusbar_print_cstring(Config.message_delay_time,
                                    "Could not sort playlist");
    }

    sort_dialog_finish(dialog);
    return;
}

static void
sort_dialog_cancel(void *user) {
    SortPlaylistDialog *dialog;

    dialog = user;
    if ((dialog == NULL) || !dialog->ready) {
        return;
    }
    sort_dialog_finish(dialog);
    return;
}

static void
sort_dialog_label_set(NcEditorSortRow *row, char *label, int32 label_len) {
    ASSERT(row != NULL);
    if ((label == NULL) || (label_len <= 0)) {
        return;
    }
    row->action.label_cap = label_len + 1;
    row->action.label = malloc2(row->action.label_cap);
    memcpy64(row->action.label, label, label_len);
    row->action.label[label_len] = '\0';
    row->action.label_len = label_len;
    return;
}

static void
sort_dialog_apply_geometry(SortPlaylistDialog *dialog) {
    int32 main_height;
    int32 height;
    int32 width;
    int32 start_x;
    int32 start_y;

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
    sort_playlist_dialog_set_geometry(
        dialog, start_x, start_y, width, height);
    return;
}

static void
sort_dialog_finish(SortPlaylistDialog *dialog) {
    NcScreen *previous;

    previous = dialog->previous_screen;
    dialog->ready = false;
    if (previous) {
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

#endif /* NCMPCPP_NC_SORT_PLAYLIST_C */
