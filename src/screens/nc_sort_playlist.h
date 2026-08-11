#if !defined(NCMPCPP_NC_SORT_PLAYLIST_H)
#define NCMPCPP_NC_SORT_PLAYLIST_H

#include "cbase.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_error.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

typedef struct NcmMpdClient NcmMpdClient;
typedef struct PlaylistScreen PlaylistScreen;

typedef struct SortPlaylistDialog {
    NcScreen screen;
    NcEditorSortMenu rows;
    NcWindow window;
    NcmSongArray songs;

    PlaylistScreen *playlist;
    NcScreen *previous_screen;
    NcmMpdClient *client;

    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;
    int32 start_position;

    bool ignore_leading_the;
    bool ready;
} SortPlaylistDialog;

void sort_playlist_dialog_init(SortPlaylistDialog *dialog,
                                      int32 start_x, int32 start_y,
                                      int32 width, int32 height,
                                      NcColor color, NcBorder border);
void sort_playlist_dialog_destroy(SortPlaylistDialog *dialog);
NcScreen *sort_playlist_dialog_base(
    SortPlaylistDialog *dialog);
NcEditorSortMenu *sort_playlist_dialog_menu(
    SortPlaylistDialog *dialog);
void sort_playlist_dialog_set_geometry(
    SortPlaylistDialog *dialog, int32 start_x, int32 start_y,
    int32 width, int32 height);
void sort_playlist_dialog_populate_defaults(
    SortPlaylistDialog *dialog);
bool sort_playlist_dialog_add_row(SortPlaylistDialog *dialog,
                                         char *label, int32 label_len,
                                         enum NcmSongGetter getter,
                                         void (*run)(void *user),
                                         void *user);
bool sort_playlist_dialog_open(
    SortPlaylistDialog *dialog, PlaylistScreen *playlist,
    NcmMpdClient *client, bool ignore_leading_the, NcmError *error);
bool sort_playlist_dialog_move_current_up(
    SortPlaylistDialog *dialog);
bool sort_playlist_dialog_move_current_down(
    SortPlaylistDialog *dialog);
bool sort_playlist_dialog_run_current(
    SortPlaylistDialog *dialog);
int32 sort_playlist_dialog_get_order(
    SortPlaylistDialog *dialog, enum NcmSongGetter *getters,
    int32 getters_cap);

#endif /* NCMPCPP_NC_SORT_PLAYLIST_H */
