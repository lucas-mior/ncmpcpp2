#if !defined(NCMPCPP_NC_SEARCH_ENGINE_H)
#define NCMPCPP_NC_SEARCH_ENGINE_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_enums.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT 11
#define NATIVE_SEARCH_ENGINE_FIRST_SEPARATOR_ROW 11
#define NATIVE_SEARCH_ENGINE_SEARCH_SOURCE_ROW 12
#define NATIVE_SEARCH_ENGINE_SEARCH_MODE_ROW 13
#define NATIVE_SEARCH_ENGINE_SECOND_SEPARATOR_ROW 14
#define NATIVE_SEARCH_ENGINE_SEARCH_BUTTON_ROW 15
#define NATIVE_SEARCH_ENGINE_RESET_BUTTON_ROW 16
#define NATIVE_SEARCH_ENGINE_RESULT_SEPARATOR_ROW 17
#define NATIVE_SEARCH_ENGINE_RESULT_SUMMARY_ROW 18
#define NATIVE_SEARCH_ENGINE_RESULT_END_SEPARATOR_ROW 19
#define NATIVE_SEARCH_ENGINE_STATIC_ROW_COUNT 20

enum NativeSearchEngineSearchMode {
    NATIVE_SEARCH_ENGINE_SEARCH_MODE_LITERAL,
    NATIVE_SEARCH_ENGINE_SEARCH_MODE_REGEX,
    NATIVE_SEARCH_ENGINE_SEARCH_MODE_EXACT,
    NATIVE_SEARCH_ENGINE_SEARCH_MODE_LAST,
};

enum NativeSearchEnginePromptResult {
    NATIVE_SEARCH_ENGINE_PROMPT_ERROR,
    NATIVE_SEARCH_ENGINE_PROMPT_ABORTED,
    NATIVE_SEARCH_ENGINE_PROMPT_ACCEPTED,
};

typedef struct NativeSearchEngineHooks {
    NcmMpdClient *client;
    bool (*list_database_songs)(void *user, NcmSongArray *songs,
                                NcmError *error);
    bool (*snapshot_playlist)(void *user, NcmSongArray *songs,
                              NcmError *error);
    enum NativeSearchEnginePromptResult (*prompt_constraint)(
        void *user, char *label, int32 label_len, NcmBuffer *initial,
        NcmBuffer *result);
    void (*status_message)(void *user, char *message, int32 message_len);
    bool (*add_song)(void *user, NcmSong *song, bool play,
                     NcmError *error);
    bool (*format_song)(void *user, NcmSong *song, NcmBuffer *text);
    void *user;
} NativeSearchEngineHooks;

typedef struct NativeSearchEngineScreen {
    NcScreen screen;
    NcSearchRowMenu rows;
    NcWindow window;
    NativeSearchEngineHooks hooks;
    NcmBuffer constraints[NATIVE_SEARCH_ENGINE_CONSTRAINT_COUNT];
    NcmBuffer filter_constraint;
    NcmBuffer search_constraint;
    NcmBuffer row_text;
    NcmBuffer title;
    NcmBuffer column_title;
    NcmRegex filter_regex;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;
    int64 lines_scrolled;
    int32 result_count;

    enum NativeSearchEngineSearchMode search_mode;
    bool search_in_database;
    bool mouse_list_scroll_whole_page;
    bool match_to_pattern;
    bool filter_enabled;
    bool prepared;
    bool result_rows_present;
    bool constraints_locked;
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
void native_search_engine_screen_set_mouse_config(
    NativeSearchEngineScreen *screen, int64 lines_scrolled,
    bool whole_page);
void native_search_engine_screen_set_display_mode(
    NativeSearchEngineScreen *screen, enum DisplayMode mode);
void native_search_engine_screen_clear(NativeSearchEngineScreen *screen);
char *native_search_engine_constraint_name(int32 idx);
char *native_search_engine_search_mode_name(
    enum NativeSearchEngineSearchMode mode);
bool native_search_engine_screen_is_prepared(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_has_result_rows(
    NativeSearchEngineScreen *screen);
int32 native_search_engine_screen_result_count(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_constraints_locked(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_set_title(
    NativeSearchEngineScreen *screen, char *title, int32 title_len);
NcmStringView native_search_engine_screen_title(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_set_column_title(
    NativeSearchEngineScreen *screen, char *title, int32 title_len);
NcmStringView native_search_engine_screen_column_title(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_format_song_text(
    NativeSearchEngineScreen *screen, NcmSong *song, NcmBuffer *text);
void native_search_engine_screen_update_column_title(
    NativeSearchEngineScreen *screen);
void native_search_engine_screen_prepare_static_rows(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_update_constraint_row(
    NativeSearchEngineScreen *screen, int32 idx);
bool native_search_engine_screen_update_search_source_row(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_update_search_mode_row(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_add_result_summary(
    NativeSearchEngineScreen *screen, int32 song_count);
void native_search_engine_screen_set_constraints_locked(
    NativeSearchEngineScreen *screen, bool locked);
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
bool native_search_engine_screen_set_find_constraint(
    NativeSearchEngineScreen *screen, char *data, int32 data_len);
NcmStringView native_search_engine_screen_find_constraint(
    NativeSearchEngineScreen *screen);
void native_search_engine_screen_clear_find_constraint(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_set_search_mode(
    NativeSearchEngineScreen *screen,
    enum NativeSearchEngineSearchMode mode);
enum NativeSearchEngineSearchMode native_search_engine_screen_search_mode(
    NativeSearchEngineScreen *screen);
void native_search_engine_screen_set_search_source(
    NativeSearchEngineScreen *screen, bool search_in_database);
bool native_search_engine_screen_searches_database(
    NativeSearchEngineScreen *screen);
void native_search_engine_screen_set_hooks(
    NativeSearchEngineScreen *screen, NativeSearchEngineHooks hooks);
bool native_search_engine_screen_list_database_songs(
    NativeSearchEngineScreen *screen, NcmSongArray *songs,
    NcmError *error);
bool native_search_engine_screen_snapshot_playlist(
    NativeSearchEngineScreen *screen, NcmSongArray *songs,
    NcmError *error);
enum NativeSearchEnginePromptResult
native_search_engine_screen_prompt_constraint(
    NativeSearchEngineScreen *screen, int32 idx, NcmBuffer *result);
void native_search_engine_screen_status_message(
    NativeSearchEngineScreen *screen, char *message, int32 message_len);
bool native_search_engine_screen_add_song(
    NativeSearchEngineScreen *screen, NcmSong *song, bool play,
    NcmError *error);
bool native_search_engine_screen_execute_search(
    NativeSearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *error);
bool native_search_engine_screen_can_run_current(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_run_current(
    NativeSearchEngineScreen *screen);
bool native_search_engine_screen_start_searching(
    NativeSearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *error);
enum DisplayMode native_search_engine_screen_toggle_display_mode(
    NativeSearchEngineScreen *screen);
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
#endif /* NCMPCPP_NC_SEARCH_ENGINE_H */
