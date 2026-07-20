#if !defined(NCMPCPP_NC_SORT_PLAYLIST_H)
#define NCMPCPP_NC_SORT_PLAYLIST_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_error.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

typedef struct NcmMpdClient NcmMpdClient;
typedef struct NativePlaylistScreen NativePlaylistScreen;

typedef struct NativeSortPlaylistDialog {
    NcScreen screen;
    NcEditorSortMenu rows;
    NcWindow window;
    NcmSongArray songs;

    NativePlaylistScreen *playlist;
    NcScreen *previous_screen;
    NcmMpdClient *client;

    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;
    int32 start_position;

    bool ignore_leading_the;
    bool ready;
} NativeSortPlaylistDialog;

void native_sort_playlist_dialog_init(NativeSortPlaylistDialog *dialog,
                                      int32 start_x, int32 start_y,
                                      int32 width, int32 height,
                                      NcColor color, NcBorder border);
void native_sort_playlist_dialog_destroy(NativeSortPlaylistDialog *dialog);
NcScreen *native_sort_playlist_dialog_base(
    NativeSortPlaylistDialog *dialog);
NcEditorSortMenu *native_sort_playlist_dialog_menu(
    NativeSortPlaylistDialog *dialog);
void native_sort_playlist_dialog_set_geometry(
    NativeSortPlaylistDialog *dialog, int32 start_x, int32 start_y,
    int32 width, int32 height);
void native_sort_playlist_dialog_populate_defaults(
    NativeSortPlaylistDialog *dialog);
bool native_sort_playlist_dialog_add_row(NativeSortPlaylistDialog *dialog,
                                         char *label, int32 label_len,
                                         enum NcmSongGetter getter,
                                         void (*run)(void *user),
                                         void *user);
bool native_sort_playlist_dialog_open(
    NativeSortPlaylistDialog *dialog, NativePlaylistScreen *playlist,
    NcmMpdClient *client, bool ignore_leading_the, NcmError *error);
bool native_sort_playlist_dialog_move_current_up(
    NativeSortPlaylistDialog *dialog);
bool native_sort_playlist_dialog_move_current_down(
    NativeSortPlaylistDialog *dialog);
bool native_sort_playlist_dialog_run_current(
    NativeSortPlaylistDialog *dialog);
int32 native_sort_playlist_dialog_get_order(
    NativeSortPlaylistDialog *dialog, enum NcmSongGetter *getters,
    int32 getters_cap);

#endif /* NCMPCPP_NC_SORT_PLAYLIST_H */
