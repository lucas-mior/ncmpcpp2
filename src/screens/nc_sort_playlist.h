#if !defined(NCMPCPP_NC_SORT_PLAYLIST_H)
#define NCMPCPP_NC_SORT_PLAYLIST_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NativeSortPlaylistDialog {
    NcScreen screen;
    NcEditorSortMenu rows;
    NcWindow window;

    int64 start_x;
    int64 start_y;
    int64 width;
    int64 height;

    bool sort_requested;
    bool cancel_requested;
    bool registered;
} NativeSortPlaylistDialog;

void native_sort_playlist_dialog_init(NativeSortPlaylistDialog *dialog,
                                      int64 start_x, int64 start_y,
                                      int64 width, int64 height,
                                      NcColor color, NcBorder border);
void native_sort_playlist_dialog_destroy(NativeSortPlaylistDialog *dialog);
NcScreen *native_sort_playlist_dialog_base(
    NativeSortPlaylistDialog *dialog);
NcEditorSortMenu *native_sort_playlist_dialog_menu(
    NativeSortPlaylistDialog *dialog);
void native_sort_playlist_dialog_set_geometry(
    NativeSortPlaylistDialog *dialog, int64 start_x, int64 start_y,
    int64 width, int64 height);
void native_sort_playlist_dialog_populate_defaults(
    NativeSortPlaylistDialog *dialog);
bool native_sort_playlist_dialog_add_row(NativeSortPlaylistDialog *dialog,
                                         char *label, int32 label_len,
                                         enum NcmSongGetter getter,
                                         void (*run)(void *user),
                                         void *user);
bool native_sort_playlist_dialog_move_current_up(
    NativeSortPlaylistDialog *dialog);
bool native_sort_playlist_dialog_move_current_down(
    NativeSortPlaylistDialog *dialog);
bool native_sort_playlist_dialog_run_current(
    NativeSortPlaylistDialog *dialog);
int32 native_sort_playlist_dialog_get_order(
    NativeSortPlaylistDialog *dialog, enum NcmSongGetter *getters,
    int32 getters_cap);
bool native_sort_playlist_dialog_sort_requested(
    NativeSortPlaylistDialog *dialog);
bool native_sort_playlist_dialog_cancel_requested(
    NativeSortPlaylistDialog *dialog);
void native_sort_playlist_dialog_clear_requests(
    NativeSortPlaylistDialog *dialog);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SORT_PLAYLIST_H */
