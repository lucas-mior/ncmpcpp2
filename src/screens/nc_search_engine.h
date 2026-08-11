#if !defined(NCMPCPP_NC_SEARCH_ENGINE_H)
#define NCMPCPP_NC_SEARCH_ENGINE_H

#include "cbase.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_enums.h"
#include "c/ncm_mpd_client.h"
#include "c/ncm_regex.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define SEARCH_ENGINE_CONSTRAINT_COUNT 11
#define SEARCH_ENGINE_FIRST_SEPARATOR_ROW 11
#define SEARCH_ENGINE_SEARCH_SOURCE_ROW 12
#define SEARCH_ENGINE_SEARCH_MODE_ROW 13
#define SEARCH_ENGINE_SECOND_SEPARATOR_ROW 14
#define SEARCH_ENGINE_SEARCH_BUTTON_ROW 15
#define SEARCH_ENGINE_RESET_BUTTON_ROW 16
#define SEARCH_ENGINE_RESULT_SEPARATOR_ROW 17
#define SEARCH_ENGINE_RESULT_SUMMARY_ROW 18
#define SEARCH_ENGINE_RESULT_END_SEPARATOR_ROW 19
#define SEARCH_ENGINE_STATIC_ROW_COUNT 20

#define ENUM_NAME SearchEngineSearchMode
#define ENUM_PREFIX_ SEARCH_ENGINE_SEARCH_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(SEARCH_ENGINE_SEARCH_MODE_LITERAL) \
    X(SEARCH_ENGINE_SEARCH_MODE_REGEX) \
    X(SEARCH_ENGINE_SEARCH_MODE_EXACT)
#include "cbase/xenums.c"

#define ENUM_NAME SearchEnginePromptResult
#define ENUM_PREFIX_ SEARCH_ENGINE_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(SEARCH_ENGINE_PROMPT_ERROR) \
    X(SEARCH_ENGINE_PROMPT_ABORTED) \
    X(SEARCH_ENGINE_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct SearchEngineHooks {
    NcmMpdClient *client;
    bool (*list_database_songs)(void *user, NcmSongArray *songs,
                                NcmError *ncm_error);
    bool (*snapshot_playlist)(void *user, NcmSongArray *songs,
                              NcmError *ncm_error);
    enum SearchEnginePromptResult (*prompt_constraint)(
        void *user, char *label, int32 label_len, StrBuilder *initial,
        StrBuilder *result);
    void (*status_message)(void *user, char *message, int32 message_len);
    bool (*add_song)(void *user, NcmSong *song, bool play,
                     NcmError *ncm_error);
    bool (*format_song)(void *user, NcmSong *song, StrBuilder *text);
    void *user;
} SearchEngineHooks;

typedef struct SearchEngineScreen {
    NcScreen screen;
    NcSearchRowMenu rows;
    NcWindow window;
    SearchEngineHooks hooks;
    StrBuilder constraints[ SEARCH_ENGINE_CONSTRAINT_COUNT];
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    StrBuilder row_text;
    StrBuilder title;
    StrBuilder column_title;
    NcmRegex filter_regex;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 lines_scrolled;
    int32 result_count;

    enum SearchEngineSearchMode search_mode;
    bool search_in_database;
    bool mouse_list_scroll_whole_page;
    bool match_to_pattern;
    bool filter_enabled;
    bool prepared;
    bool result_rows_present;
    bool constraints_locked;
    bool registered;
} SearchEngineScreen;

void search_engine_screen_init(SearchEngineScreen *screen,
                               int32 start_x, int32 width,
                               int32 main_start_y,
                               int32 main_height, NcColor color,
                               NcBorder border);
void search_engine_screen_destroy(SearchEngineScreen *screen);
NcScreen *search_engine_screen_base(SearchEngineScreen *screen);
NcMenu *search_engine_screen_menu(SearchEngineScreen *screen);
NcWindow *search_engine_screen_window(SearchEngineScreen *screen);
void search_engine_screen_set_geometry(
    SearchEngineScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height);
void search_engine_screen_set_mouse_config(
    SearchEngineScreen *screen, int32 lines_scrolled,
    bool whole_page);
void search_engine_screen_set_display_mode(
    SearchEngineScreen *screen, enum DisplayMode mode);
void search_engine_screen_clear(SearchEngineScreen *screen);
char *search_engine_constraint_name(int32 idx);
char *search_engine_search_mode_name(
    enum SearchEngineSearchMode mode);
bool search_engine_screen_constraints_locked(
    SearchEngineScreen *screen);
bool search_engine_screen_format_song_text(
    SearchEngineScreen *screen, NcmSong *song, StrBuilder *text);
void search_engine_screen_update_column_title(
    SearchEngineScreen *screen);
void search_engine_screen_prepare_static_rows(
    SearchEngineScreen *screen);
bool search_engine_screen_update_constraint_row(
    SearchEngineScreen *screen, int32 idx);
bool search_engine_screen_update_search_source_row(
    SearchEngineScreen *screen);
bool search_engine_screen_update_search_mode_row(
    SearchEngineScreen *screen);
bool search_engine_screen_add_result_summary(
    SearchEngineScreen *screen, int32 song_count);
void search_engine_screen_set_constraints_locked(
    SearchEngineScreen *screen, bool locked);
void search_engine_screen_reset(SearchEngineScreen *screen);
bool search_engine_screen_add_song_copy(
    SearchEngineScreen *screen, NcmSong *song);
bool search_engine_screen_add_song_copy_with_flags(
    SearchEngineScreen *screen, NcmSong *song, uint32 flags);
bool search_engine_screen_add_buffer_with_flags(
    SearchEngineScreen *screen, NcBuffer *buffer, uint32 flags);
bool search_engine_screen_set_constraint(
    SearchEngineScreen *screen, int32 idx, char *data, int32 data_len);
void search_engine_screen_clear_find_constraint(
    SearchEngineScreen *screen);
bool search_engine_screen_set_search_mode(
    SearchEngineScreen *screen,
    enum SearchEngineSearchMode mode);
void search_engine_screen_set_search_source(
    SearchEngineScreen *screen, bool search_in_database);
void search_engine_screen_set_hooks(
    SearchEngineScreen *screen, SearchEngineHooks hooks);
bool search_engine_screen_list_database_songs(
    SearchEngineScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error);
bool search_engine_screen_snapshot_playlist(
    SearchEngineScreen *screen, NcmSongArray *songs,
    NcmError *ncm_error);
enum SearchEnginePromptResult
search_engine_screen_prompt_constraint(
    SearchEngineScreen *screen, int32 idx, StrBuilder *result);
void search_engine_screen_status_message(
    SearchEngineScreen *screen, char *message, int32 message_len);
bool search_engine_screen_add_song(
    SearchEngineScreen *screen, NcmSong *song, bool play,
    NcmError *ncm_error);
bool search_engine_screen_execute_search(
    SearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *ncm_error);
bool search_engine_screen_can_run_current(
    SearchEngineScreen *screen);
bool search_engine_screen_run_current(
    SearchEngineScreen *screen);
bool search_engine_screen_start_searching(
    SearchEngineScreen *screen, NcmMpdClient *client,
    NcmError *ncm_error);
enum DisplayMode search_engine_screen_toggle_display_mode(
    SearchEngineScreen *screen);
bool search_engine_screen_allows_search(
    SearchEngineScreen *screen);
bool search_engine_screen_current_song(
    SearchEngineScreen *screen, NcmSong *song);
bool search_engine_screen_selected_songs(
    SearchEngineScreen *screen, NcmSongArray *songs);
bool search_engine_screen_apply_filter(
    SearchEngineScreen *screen, char *pattern, int32 pattern_len,
    NcmError *ncm_error);
void search_engine_screen_clear_filter(
    SearchEngineScreen *screen);
bool search_engine_screen_search(SearchEngineScreen *screen,
                                 char *pattern, int32 pattern_len,
                                 bool forward, bool wrap,
                                 bool skip_current,
                                 NcmError *ncm_error);
#endif /* NCMPCPP_NC_SEARCH_ENGINE_H */
