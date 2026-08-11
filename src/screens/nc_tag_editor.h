#if !defined(NCMPCPP_NC_TAG_EDITOR_H)
#define NCMPCPP_NC_TAG_EDITOR_H

#include "cbase.h"

#include "c/ncm_app_arrays.h"
#include "c/ncm_regex.h"
#include "c/ncm_tags.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define ENUM_NAME TagEditorColumn
#define ENUM_PREFIX_ TAG_EDITOR_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(TAG_EDITOR_COLUMN_DIRECTORIES) \
    X(TAG_EDITOR_COLUMN_TAG_TYPES) \
    X(TAG_EDITOR_COLUMN_TAGS)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditorParserMode
#define ENUM_PREFIX_ TAG_EDITOR_PARSER_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(TAG_EDITOR_PARSER_NONE) \
    X(TAG_EDITOR_PARSER_TAGS_FROM_FILENAME) \
    X(TAG_EDITOR_PARSER_RENAME_FILES)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditorFocus
#define ENUM_PREFIX_ TAG_EDITOR_FOCUS_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(TAG_EDITOR_FOCUS_DIRECTORIES) \
    X(TAG_EDITOR_FOCUS_TAG_TYPES) \
    X(TAG_EDITOR_FOCUS_TAGS) \
    X(TAG_EDITOR_FOCUS_PARSER_CHOICE) \
    X(TAG_EDITOR_FOCUS_PARSER_ACTIONS) \
    X(TAG_EDITOR_FOCUS_PARSER_LEGEND) \
    X(TAG_EDITOR_FOCUS_PARSER_PREVIEW)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditorPromptResult
#define ENUM_PREFIX_ TAG_EDITOR_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(TAG_EDITOR_PROMPT_ERROR) \
    X(TAG_EDITOR_PROMPT_ABORTED) \
    X(TAG_EDITOR_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct TagEditorHooks {
    enum TagEditorPromptResult (*prompt)(
        void *user, char *label, int32 label_len, NcmStringView initial,
        StrBuilder *result);
    bool (*confirm)(void *user, char *message, int32 message_len);
    void (*status_message)(void *user, char *message, int32 message_len);
    void (*update_directory)(void *user, char *directory,
                             int32 directory_len);
    void *user;
} TagEditorHooks;

typedef struct TagEditorScreen {
    NcScreen screen;
    NcEditorPairMenu directories;
    NcEditorStringMenu tag_types;
    NcTagRowMenu tags;
    NcEditorStringMenu parser_dialog;
    NcEditorStringMenu parser_rows;
    NcEditorStringMenu parser_actions;
    NcWindow directories_window;
    NcWindow tag_types_window;
    NcWindow tags_window;
    NcWindow parser_dialog_window;
    NcWindow parser_window;
    NcWindow parser_helper_window;
    TagEditorHooks hooks;
    StrBuilder current_dir;
    StrBuilder displayed_dir;
    StrBuilder observed_dir;
    StrBuilder highlighted_dir;
    StrBuilder directories_title;
    StrBuilder tag_types_title;
    StrBuilder tags_title;
    StrBuilder parser_dialog_title;
    StrBuilder parser_title;
    StrBuilder parser_helper_title;
    StrBuilder parser_legend;
    StrBuilder parser_preview;
    StrBuilderArray recent_patterns;
    StrBuilder directory_filter_constraint;
    StrBuilder tag_filter_constraint;
    StrBuilder directory_search_constraint;
    StrBuilder tag_search_constraint;
    StrBuilder pattern;

    NcmRegex directory_filter_regex;
    NcmRegex tag_filter_regex;
    NcmRegex directory_search_regex;
    NcmRegex tag_search_regex;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 left_width;
    int32 middle_start_x;
    int32 middle_width;
    int32 right_start_x;
    int32 right_width;
    int32 parser_dialog_start_x;
    int32 parser_dialog_start_y;
    int32 parser_dialog_width;
    int32 parser_dialog_height;
    int32 parser_start_x;
    int32 parser_start_y;
    int32 parser_width;
    int32 parser_width_one;
    int32 parser_width_two;
    int32 parser_height;
    int32 parser_helper_start_x;
    enum TagEditorColumn active_column;
    int32 last_directory_highlight;
    int32 last_tag_type_highlight;
    int32 last_known_directory_count;
    int32 last_known_tag_count;
    int32 window_timeout_ms;

    enum TagEditorParserMode parser_mode;
    enum TagEditorFocus active_focus;
    bool directories_update_requested;
    bool tags_update_requested;
    bool directory_filter_enabled;
    bool tag_filter_enabled;
    bool directory_search_enabled;
    bool tag_search_enabled;
    bool parser_preview_enabled;
    bool recent_patterns_loaded;
    bool displayed_dir_valid;
    bool observed_dir_valid;
    bool registered;
} TagEditorScreen;

void tag_editor_screen_init(TagEditorScreen *screen,
                            int32 start_x, int32 width,
                            int32 main_start_y,
                            int32 main_height, NcColor color,
                            NcBorder border);
void tag_editor_screen_destroy(TagEditorScreen *screen);
NcScreen *tag_editor_screen_base(TagEditorScreen *screen);

void tag_editor_screen_set_hooks(TagEditorScreen *screen,
                                 TagEditorHooks hooks);
NcMenu *tag_editor_screen_active_menu(TagEditorScreen *screen);
NcWindow *tag_editor_screen_active_window(TagEditorScreen *screen);
void tag_editor_screen_set_geometry(TagEditorScreen *screen,
                                    int32 start_x, int32 width,
                                    int32 main_start_y,
                                    int32 main_height);
void tag_editor_screen_clear_directories(
    TagEditorScreen *screen);
void tag_editor_screen_clear_stale_tags(
    TagEditorScreen *screen);
void tag_editor_screen_finish_directory_change(
    TagEditorScreen *screen);
bool tag_editor_screen_set_current_dir(TagEditorScreen *screen,
                                       char *dir, int32 dir_len);
bool tag_editor_screen_current_dir(TagEditorScreen *screen,
                                   NcmStringView *view);
bool tag_editor_screen_current_directory_path(
    TagEditorScreen *screen, NcmStringView *view);
bool tag_editor_screen_enter_directory(TagEditorScreen *screen);
bool tag_editor_screen_go_to_parent(TagEditorScreen *screen);
bool tag_editor_screen_locate_song(TagEditorScreen *screen,
                                   NcmSong *song);
bool tag_editor_screen_rename_directory_available(
    TagEditorScreen *screen, char *music_dir, int32 music_dir_len);
bool tag_editor_screen_rename_current_directory(
    TagEditorScreen *screen, char *music_dir, int32 music_dir_len);
bool tag_editor_screen_add_directory(TagEditorScreen *screen,
                                     char *label, int32 label_len,
                                     char *path, int32 path_len);
bool tag_editor_screen_load_songs(TagEditorScreen *screen,
                                  NcmSongArray *songs);
bool tag_editor_screen_add_mutable_song(
    TagEditorScreen *screen, NcmMutableSong *song);
bool tag_editor_screen_selected_songs(TagEditorScreen *screen,
                                      NcmSongArray *songs);
bool tag_editor_screen_previous_column_available(
    TagEditorScreen *screen);
bool tag_editor_screen_next_column_available(
    TagEditorScreen *screen);
void tag_editor_screen_previous_column(TagEditorScreen *screen);
void tag_editor_screen_next_column(TagEditorScreen *screen);
bool tag_editor_screen_apply_tag_to_selection(
    TagEditorScreen *screen, enum NcmTagsField field, char *value,
    int32 value_len, char *separator, int32 separator_len);
bool tag_editor_screen_number_tracks(TagEditorScreen *screen,
                                     bool extended);
void tag_editor_screen_capitalize_first_letters(
    TagEditorScreen *screen);
void tag_editor_screen_lower_all_letters(TagEditorScreen *screen);
void tag_editor_screen_clear_modifications(
    TagEditorScreen *screen);
bool tag_editor_screen_save_modified(TagEditorScreen *screen,
                                     char *music_dir);
bool tag_editor_screen_save_action_available(
    TagEditorScreen *screen);
bool tag_editor_screen_apply_directory_filter(
    TagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *ncm_error);
bool tag_editor_screen_apply_tag_filter(
    TagEditorScreen *screen, char *pattern, int32 pattern_len,
    uint32 regex_flags, NcmError *ncm_error);
bool tag_editor_screen_search(
    TagEditorScreen *screen, char *pattern, int32 pattern_len,
    bool forward, bool wrap, bool skip_current, NcmError *ncm_error);
bool tag_editor_screen_prepare_parser_rows(
    TagEditorScreen *screen, enum TagEditorParserMode mode,
    char *pattern, int32 pattern_len);
void tag_editor_screen_show_parser_dialog(
    TagEditorScreen *screen);
void tag_editor_screen_show_parser_actions(
    TagEditorScreen *screen, enum TagEditorParserMode mode);
void tag_editor_screen_show_parser_legend(
    TagEditorScreen *screen);
void tag_editor_screen_show_parser_preview(
    TagEditorScreen *screen);
void tag_editor_screen_close_parser(
    TagEditorScreen *screen);
bool tag_editor_parse_filename(NcmMutableSong *song, char *mask,
                               int32 mask_len, bool preview,
                               StrBuilder *preview_buffer);
bool tag_editor_generate_filename(NcmMutableSong *song,
                                  char *pattern, int32 pattern_len,
                                  StrBuilder *filename);
bool tag_editor_song_display_value(NcmMutableSong *song,
                                   enum NcmTagsField field,
                                   StrBuilder *buffer);

#endif /* NCMPCPP_NC_TAG_EDITOR_H */
