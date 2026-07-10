#include <assert.h>
#include <stdlib.h>

#include "actions.h"
#include "app_controller.h"
#include "screens/native_c_screens.h"
#include "settings.h"
#include "ui_state.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

static void test_registered_native_media_library_actions(void);

int
main(void) {
    test_registered_native_media_library_actions();
    exit(EXIT_SUCCESS);
}

static void
test_registered_native_media_library_actions(void) {
    NativeMediaLibraryScreen *library;
    NcScreen *registered;
    NcmSong song;

    app_controller_init();
    ui_state_set_screen_size(100, 30);
    Config.media_lib_primary_tag = MPD_TAG_ARTIST;
    Config.media_library_sort_by_mtime = false;

    library = native_c_screen_media_library();
    native_media_library_screen_clear(library);
    assert(native_media_library_screen_set_mode(
        library, NATIVE_MEDIA_LIBRARY_MODE_THREE_COLUMNS));
    assert(native_media_library_screen_set_active_column(
        library, NATIVE_MEDIA_LIBRARY_COLUMN_TAGS));
    assert(native_media_library_screen_add_album(
        library, LIT_ARGS("Artist"), LIT_ARGS("Album"),
        LIT_ARGS("2026"), 0, false));

    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("album/song.flac")));
    assert(native_media_library_screen_add_song_copy(library, &song));
    ncm_song_destroy(&song);

    assert(ncm_action_runtime_run(NULL, NCM_ACTION_SHOW_MEDIA_LIBRARY));
    registered = app_controller_find_screen_type(
        NC_SCREEN_TYPE_MEDIA_LIBRARY);
    assert(registered == native_c_screen_media_library_native());
    assert(app_controller_current_screen() == registered);

    assert(ncm_action_runtime_run(NULL, NCM_ACTION_NEXT_COLUMN));
    assert(native_media_library_screen_active_column(library)
           == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_NEXT_COLUMN));
    assert(native_media_library_screen_active_column(library)
           == NATIVE_MEDIA_LIBRARY_COLUMN_SONGS);
    assert(ncm_action_runtime_run(NULL, NCM_ACTION_PREVIOUS_COLUMN));
    assert(native_media_library_screen_active_column(library)
           == NATIVE_MEDIA_LIBRARY_COLUMN_ALBUMS);

    assert(ncm_action_runtime_run(
        NULL, NCM_ACTION_TOGGLE_MEDIA_LIBRARY_COLUMNS_MODE));
    assert(native_media_library_screen_mode(library)
           == NATIVE_MEDIA_LIBRARY_MODE_TWO_COLUMNS);
    assert(app_controller_find_screen_type(NC_SCREEN_TYPE_MEDIA_LIBRARY)
           == native_media_library_screen_base(library));

    assert(native_media_library_screen_add_album(
        library, LIT_ARGS("Artist"), LIT_ARGS("Album"),
        LIT_ARGS("2026"), 0, false));
    ncm_song_init(&song);
    assert(ncm_song_set_uri(&song, LIT_ARGS("album/song.flac")));
    assert(native_media_library_screen_add_song_copy(library, &song));
    ncm_song_destroy(&song);
    assert(native_media_library_screen_current_album(library));
    assert(native_media_library_screen_visible_song_count(library) == 1);

    native_media_library_screen_clear_update_requests(library);
    assert(ncm_action_runtime_run(
        NULL, NCM_ACTION_TOGGLE_LIBRARY_TAG_TYPE));
    assert(Config.media_lib_primary_tag == MPD_TAG_ALBUM_ARTIST);
    assert(native_media_library_screen_current_album(library) == NULL);
    assert(native_media_library_screen_visible_song_count(library) == 0);
    assert(!library->tags_update_request);
    assert(library->albums_update_request);
    assert(library->songs_update_request);
    return;
}
