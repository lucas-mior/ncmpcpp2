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

typedef struct NativeSearchEngineBridge {
    NcWindow *(*active_window)(void *user);
    void (*refresh)(void *user);
    void (*refresh_window)(void *user);
    void (*scroll)(void *user, enum NcScroll where);
    void (*switch_to)(void *user);
    void (*resize)(void *user);
    char *(*title)(void *user);
    void (*update)(void *user);
    void (*mouse_button_pressed)(void *user, MEVENT event);
    void *user;
} NativeSearchEngineBridge;

typedef struct NativeSearchEngineScreen {
    NcScreen screen;
    NcSearchRowMenu rows;
    NcWindow window;
    NativeSearchEngineBridge bridge;
    NcmBuffer constraints[NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT];
    NcmBuffer filter_constraint;
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
bool native_search_engine_screen_add_song_copy_with_flags(
    NativeSearchEngineScreen *screen, NcmSong *song, uint32 flags);
bool native_search_engine_screen_add_buffer_with_flags(
    NativeSearchEngineScreen *screen, NcBuffer *buffer, uint32 flags);
bool native_search_engine_screen_set_constraint(
    NativeSearchEngineScreen *screen, int32 idx, char *data, int32 data_len);
NcmStringView native_search_engine_screen_constraint(
    NativeSearchEngineScreen *screen, int32 idx);
bool native_search_engine_screen_build_query(
    NativeSearchEngineScreen *screen, NcmBuffer *query);
bool native_search_engine_screen_allows_search(
    NativeSearchEngineScreen *screen);
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
void native_search_engine_screen_set_bridge(
    NativeSearchEngineScreen *screen, NativeSearchEngineBridge bridge);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SEARCH_ENGINE_H */
