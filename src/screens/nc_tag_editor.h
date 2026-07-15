#if !defined(NCMPCPP_NC_TAG_EDITOR_H)
#define NCMPCPP_NC_TAG_EDITOR_H

#include <stdbool.h>

#include "c/ncm_app_arrays.h"
#include "c/ncm_regex.h"
#include "c/ncm_tags.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

enum NativeTagEditorColumn {
    NATIVE_TAG_EDITOR_COLUMN_DIRECTORIES,
    NATIVE_TAG_EDITOR_COLUMN_TAG_TYPES,
    NATIVE_TAG_EDITOR_COLUMN_TAGS,
};

enum NativeTagEditorParserMode {
    NATIVE_TAG_EDITOR_PARSER_NONE,
    NATIVE_TAG_EDITOR_PARSER_TAGS_FROM_FILENAME,
    NATIVE_TAG_EDITOR_PARSER_RENAME_FILES,
};


typedef struct NativeTagEditorBridge {
    NcWindow *(*active_window)(void *user);
    void (*refresh)(void *user);
    void (*refresh_window)(void *user);
    void (*scroll)(void *user, enum NcScroll where);
    bool (*action_runnable)(void *user);
    bool (*run_action)(void *user);
    void (*switch_to)(void *user);
    void (*resize)(void *user);
    char *(*title)(void *user);
    void (*update)(void *user);
    void (*clear_directories)(void *user);
    void (*mouse_button_pressed)(void *user, MEVENT event);
    void *user;
} NativeTagEditorBridge;

typedef struct NativeTagEditorScreen {
    NcScreen screen;
    NcEditorPairMenu directories;
    NcEditorStringMenu tag_types;
    NcTagRowMenu tags;
    NcEditorStringMenu parser_dialog;
    NcEditorStringMenu parser_rows;
    NcWindow directories_window;
    NcWindow tag_types_window;
    NcWindow tags_window;
    NcWindow parser_dialog_window;
    NcWindow parser_window;
    NcWindow parser_helper_window;
    NativeTagEditorBridge bridge;
    NcmBuffer current_dir;
    NcmBuffer displayed_dir;
    NcmBuffer observed_dir;
    NcmBuffer highlighted_dir;
    NcmBuffer directories_title;
    NcmBuffer tag_types_title;
    NcmBuffer tags_title;
    NcmBuffer parser_dialog_title;
    NcmBuffer parser_title;
    NcmBuffer parser_helper_title;
    NcmBuffer directory_filter_constraint;
    NcmBuffer tag_filter_constraint;
    NcmBuffer directory_search_constraint;
    NcmBuffer tag_search_constraint;
    NcmBuffer pattern;

    NcmRegex directory_filter_regex;
    NcmRegex tag_filter_regex;
    NcmRegex directory_search_regex;
    NcmRegex tag_search_regex;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;
    int64 left_width;
    int64 middle_start_x;
    int64 middle_width;
    int64 right_start_x;
    int64 right_width;
    int64 active_column;
    int64 last_directory_highlight;
    int64 last_known_directory_count;
    int64 last_known_tag_count;
    int32 window_timeout_ms;

    enum NativeTagEditorParserMode parser_mode;
    bool directories_update_requested;
    bool tags_update_requested;
    bool directory_filter_enabled;
    bool tag_filter_enabled;
    bool directory_search_enabled;
    bool tag_search_enabled;
    bool parser_preview_enabled;
    bool displayed_dir_valid;
    bool observed_dir_valid;
    bool registered;
} NativeTagEditorScreen;

void native_tag_editor_screen_init(NativeTagEditorScreen *screen,
                                   int64 start_x, int64 width,
                                   int64 main_start_y,
                                   int64 main_height, NcColor color,
                                   NcBorder border);
void native_tag_editor_screen_destroy(NativeTagEditorScreen *screen);
NcScreen *native_tag_editor_screen_base(NativeTagEditorScreen *screen);

void native_tag_editor_screen_set_bridge(NativeTagEditorScreen *screen,
                                         NativeTagEditorBridge bridge);
NcEditorPairMenu *native_tag_editor_screen_directories(
    NativeTagEditorScreen *screen);
NcEditorStringMenu *native_tag_editor_screen_tag_types(
    NativeTagEditorScreen *screen);
NcTagRowMenu *native_tag_editor_screen_tags(NativeTagEditorScreen *screen);
NcEditorStringMenu *native_tag_editor_screen_parser_rows(
    NativeTagEditorScreen *screen);
NcMenu *native_tag_editor_screen_active_menu(NativeTagEditorScreen *screen);
NcWindow *native_tag_editor_screen_active_window(NativeTagEditorScreen *screen);
void native_tag_editor_screen_set_geometry(NativeTagEditorScreen *screen,
                                           int64 start_x, int64 width,
                                           int64 main_start_y,
                                           int64 main_height);
void native_tag_editor_screen_clear(NativeTagEditorScreen *screen);
void native_tag_editor_screen_clear_directories(
    NativeTagEditorScreen *screen);
void native_tag_editor_screen_clear_stale_tags(
    NativeTagEditorScreen *screen);
void native_tag_editor_screen_finish_directory_change(
    NativeTagEditorScreen *screen);
bool native_tag_editor_screen_displayed_dir(NativeTagEditorScreen *screen,
                                            NcmStringView *view);
bool native_tag_editor_screen_observed_dir(NativeTagEditorScreen *screen,
                                           NcmStringView *view);
bool native_tag_editor_screen_set_current_dir(NativeTagEditorScreen *screen,
                                              char *dir, int32 dir_len);
bool native_tag_editor_screen_current_dir(NativeTagEditorScreen *screen,
                                          NcmStringView *view);
bool native_tag_editor_screen_add_directory(NativeTagEditorScreen *screen,
                                            char *label, int32 label_len,
                                            char *path, int32 path_len);
bool native_tag_editor_screen_load_directories(
    NativeTagEditorScreen *screen, NcmDirectoryArray *directories);
bool native_tag_editor_screen_load_songs(NativeTagEditorScreen *screen,
                                         NcmSongArray *songs);
bool native_tag_editor_screen_add_mutable_song(
    NativeTagEditorScreen *screen, NcmMutableSong *song);
bool native_tag_editor_screen_current_song(NativeTagEditorScreen *screen,
                                           NcmMutableSong *song);
bool native_tag_editor_screen_selected_songs(NativeTagEditorScreen *screen,
                                             NcmSongArray *songs);
bool native_tag_editor_screen_previous_column_available(
    NativeTagEditorScreen *screen);
bool native_tag_editor_screen_next_column_available(
    NativeTagEditorScreen *screen);
void native_tag_editor_screen_previous_column(NativeTagEditorScreen *screen);
void native_tag_editor_screen_next_column(NativeTagEditorScreen *screen);
bool native_tag_editor_screen_apply_tag_to_selection(
    NativeTagEditorScreen *screen, enum NcmTagsField field, char *value,
    int32 value_len, char *separator, int32 separator_len);
bool native_tag_editor_screen_number_tracks(NativeTagEditorScreen *screen,
                                            bool extended);
void native_tag_editor_screen_capitalize_first_letters(
    NativeTagEditorScreen *screen);
void native_tag_editor_screen_lower_all_letters(NativeTagEditorScreen *screen);
void native_tag_editor_screen_clear_modifications(
    NativeTagEditorScreen *screen);
bool native_tag_editor_screen_save_modified(NativeTagEditorScreen *screen,
                                            char *music_dir);
bool native_tag_editor_screen_apply_directory_filter(
    NativeTagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error);
bool native_tag_editor_screen_apply_tag_filter(
    NativeTagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *error);
void native_tag_editor_screen_clear_filters(NativeTagEditorScreen *screen);
bool native_tag_editor_screen_search(
    NativeTagEditorScreen *screen, char *pattern, int32 pattern_len,
    bool forward, bool wrap, bool skip_current, NcmError *error);
bool native_tag_editor_screen_prepare_parser_rows(
    NativeTagEditorScreen *screen, enum NativeTagEditorParserMode mode,
    char *pattern, int32 pattern_len);
bool native_tag_editor_parse_filename(NcmMutableSong *song, char *mask,
                                      int32 mask_len, bool preview,
                                      NcmBuffer *preview_buffer);
bool native_tag_editor_generate_filename(NcmMutableSong *song,
                                         char *pattern, int32 pattern_len,
                                         NcmBuffer *filename);
bool native_tag_editor_song_display_value(NcmMutableSong *song,
                                          enum NcmTagsField field,
                                          NcmBuffer *buffer);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_TAG_EDITOR_H */
