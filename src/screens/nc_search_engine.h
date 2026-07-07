#if !defined(NCMPCPP_NC_SEARCH_ENGINE_H)
#define NCMPCPP_NC_SEARCH_ENGINE_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT 11

typedef struct NativeSearchEngineScreen {
    NcScreen screen;
    NcSearchRowMenu rows;
    NcWindow window;
    NcmBuffer constraints[NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT];
    NcmBuffer search_constraint;
    NcmRegex filter_regex;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;
    int64 lines_scrolled;

    bool mouse_list_scroll_whole_page;
    bool match_to_pattern;
    bool filter_enabled;
    bool registered;
} NativeSearchEngineScreen;

void native_search_engine_screen_init(NativeSearchEngineScreen *screen,
                                      int64 start_x, int64 width,
                                      int64 main_start_y,
                                      int64 main_height, NcColor color,
                                      NcBorder border);
void native_search_engine_screen_destroy(NativeSearchEngineScreen *screen);
NcScreen *native_search_engine_screen_base(NativeSearchEngineScreen *screen);
NcSearchRowMenu *native_search_engine_screen_rows(
    NativeSearchEngineScreen *screen);
NcMenu *native_search_engine_screen_menu(NativeSearchEngineScreen *screen);
NcWindow *native_search_engine_screen_window(NativeSearchEngineScreen *screen);
void native_search_engine_screen_set_geometry(
    NativeSearchEngineScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height);
void native_search_engine_screen_clear(NativeSearchEngineScreen *screen);
void native_search_engine_screen_reset(NativeSearchEngineScreen *screen);
bool native_search_engine_screen_add_song_copy(
    NativeSearchEngineScreen *screen, NcmSong *song);
bool native_search_engine_screen_add_buffer(
    NativeSearchEngineScreen *screen, NcBuffer *buffer);
bool native_search_engine_screen_set_constraint(
    NativeSearchEngineScreen *screen, int32 idx, char *data, int32 data_len);
NcmStringView native_search_engine_screen_constraint(
    NativeSearchEngineScreen *screen, int32 idx);
bool native_search_engine_screen_build_query(
    NativeSearchEngineScreen *screen, NcmBuffer *query);
bool native_search_engine_screen_current_song(
    NativeSearchEngineScreen *screen, NcmSong *song);
bool native_search_engine_screen_selected_songs(
    NativeSearchEngineScreen *screen, NcmSongArray *songs);
bool native_search_engine_screen_apply_filter(
    NativeSearchEngineScreen *screen, char *pattern, int32 pattern_len,
    NcmError *error);
void native_search_engine_screen_clear_filter(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_search(NativeSearchEngineScreen *screen,
                                        char *pattern, int32 pattern_len,
                                        bool forward, bool wrap,
                                        bool skip_current,
                                        NcmError *error);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SEARCH_ENGINE_H */
