#if !defined(NCMPCPP_NC_TINY_TAG_EDITOR_H)
#define NCMPCPP_NC_TINY_TAG_EDITOR_H

#include <stdbool.h>

#include "c/ncm_mutable_song.h"
#include "c/ncm_taglib.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define NATIVE_TINY_TAG_EDITOR_TAG_ROW(FIELD) \
    (NATIVE_TINY_TAG_EDITOR_FIRST_TAG_ROW + (FIELD))

enum NativeTinyTagEditorRow {
    NATIVE_TINY_TAG_EDITOR_FILE_NAME_INFO_ROW,
    NATIVE_TINY_TAG_EDITOR_DIRECTORY_INFO_ROW,
    NATIVE_TINY_TAG_EDITOR_UNUSED_INFO_ROW,
    NATIVE_TINY_TAG_EDITOR_LENGTH_INFO_ROW,
    NATIVE_TINY_TAG_EDITOR_BITRATE_INFO_ROW,
    NATIVE_TINY_TAG_EDITOR_SAMPLE_RATE_INFO_ROW,
    NATIVE_TINY_TAG_EDITOR_CHANNELS_INFO_ROW,
    NATIVE_TINY_TAG_EDITOR_FIRST_SEPARATOR_ROW,
    NATIVE_TINY_TAG_EDITOR_FIRST_TAG_ROW,
    NATIVE_TINY_TAG_EDITOR_LAST_TAG_ROW =
        NATIVE_TINY_TAG_EDITOR_FIRST_TAG_ROW + NCM_TAGS_FIELD_LAST - 1,
    NATIVE_TINY_TAG_EDITOR_SECOND_SEPARATOR_ROW,
    NATIVE_TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW,
    NATIVE_TINY_TAG_EDITOR_THIRD_SEPARATOR_ROW,
    NATIVE_TINY_TAG_EDITOR_SAVE_ROW,
    NATIVE_TINY_TAG_EDITOR_CANCEL_ROW,
    NATIVE_TINY_TAG_EDITOR_ROW_COUNT,
};

enum NativeTinyTagEditorOpenResult {
    NATIVE_TINY_TAG_EDITOR_OPEN_SUCCESS,
    NATIVE_TINY_TAG_EDITOR_OPEN_INVALID_ARGUMENT,
    NATIVE_TINY_TAG_EDITOR_OPEN_STREAM,
    NATIVE_TINY_TAG_EDITOR_OPEN_MISSING_MUSIC_DIRECTORY,
    NATIVE_TINY_TAG_EDITOR_OPEN_UNREADABLE_FILE,
    NATIVE_TINY_TAG_EDITOR_OPEN_PREPARE_FAILED,
};

enum NativeTinyTagEditorPromptResult {
    NATIVE_TINY_TAG_EDITOR_PROMPT_ERROR,
    NATIVE_TINY_TAG_EDITOR_PROMPT_ABORTED,
    NATIVE_TINY_TAG_EDITOR_PROMPT_ACCEPTED,
};

typedef struct NativeTinyTagEditorHooks {
    enum NativeTinyTagEditorPromptResult (*prompt)(
        void *user, char *label, int32 label_len, NcmStringView initial,
        NcmBuffer *result);
    void (*status_message)(void *user, char *message, int32 message_len);
    bool (*taglib_open)(void *user, NcmTaglibFile *file, char *path,
                        int32 path_len);
    bool (*taglib_audio_properties)(
        void *user, NcmTaglibFile *file,
        NcmTaglibAudioProperties *properties);
    bool (*taglib_extended_set_supported)(void *user, NcmTaglibFile *file);
    void (*taglib_close)(void *user, NcmTaglibFile *file);
    bool (*write_song)(void *user, NcmMutableSong *song, char *music_dir);
    void (*update_directory)(void *user, char *directory,
                             int32 directory_len);
    void (*update_playlist_song)(void *user, NcmMutableSong *song);
    void (*request_browser_update)(void *user);
    void (*switch_to_screen)(void *user, NcScreen *screen);
    void *user;
} NativeTinyTagEditorHooks;

typedef struct NativeTinyTagEditorScreen {
    NcScreen screen;
    NcEditorBufferMenu rows;
    NcWindow window;
    NativeTinyTagEditorHooks hooks;
    NcmMutableSong edited;
    NcmBuffer music_dir;
    NcmBuffer tag_separator;
    NcScreen *previous_screen;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;

    bool has_edited;
    bool show_duplicate_tags;
    bool registered;
} NativeTinyTagEditorScreen;

void native_tiny_tag_editor_screen_init(
    NativeTinyTagEditorScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height, NcColor color, NcBorder border);
void native_tiny_tag_editor_screen_destroy(
    NativeTinyTagEditorScreen *screen);
NcScreen *native_tiny_tag_editor_screen_base(
    NativeTinyTagEditorScreen *screen);

void native_tiny_tag_editor_screen_set_hooks(
    NativeTinyTagEditorScreen *screen, NativeTinyTagEditorHooks hooks);
NcEditorBufferMenu *native_tiny_tag_editor_screen_rows(
    NativeTinyTagEditorScreen *screen);
NcmMutableSong *native_tiny_tag_editor_screen_edited(
    NativeTinyTagEditorScreen *screen);
void native_tiny_tag_editor_screen_set_geometry(
    NativeTinyTagEditorScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height);
bool native_tiny_tag_editor_screen_set_edited_song(
    NativeTinyTagEditorScreen *screen, NcmSong *song);
enum NativeTinyTagEditorOpenResult
native_tiny_tag_editor_screen_open_song(
    NativeTinyTagEditorScreen *screen, NcmSong *song,
    char *music_dir, int32 music_dir_len, char *tag_separator,
    int32 tag_separator_len, bool show_duplicate_tags, NcmBuffer *path);
bool native_tiny_tag_editor_screen_set_edited_mutable_song(
    NativeTinyTagEditorScreen *screen, NcmMutableSong *song);
bool native_tiny_tag_editor_screen_reload_rows(
    NativeTinyTagEditorScreen *screen,
    NcmTaglibAudioProperties *properties,
    bool extended_tags_supported, char *tag_separator,
    int32 tag_separator_len, bool show_duplicate_tags);
bool native_tiny_tag_editor_screen_set_tag_value(
    NativeTinyTagEditorScreen *screen, enum NcmTagsField field,
    char *value, int32 value_len, char *separator, int32 separator_len);
bool native_tiny_tag_editor_screen_set_filename(
    NativeTinyTagEditorScreen *screen, char *name, int32 name_len);
bool native_tiny_tag_editor_screen_set_filename_stem(
    NativeTinyTagEditorScreen *screen, char *stem, int32 stem_len);
bool native_tiny_tag_editor_screen_save(
    NativeTinyTagEditorScreen *screen, char *music_dir);
bool native_tiny_tag_editor_screen_run_row(
    NativeTinyTagEditorScreen *screen, int64 row);
bool native_tiny_tag_editor_screen_run_current(
    NativeTinyTagEditorScreen *screen);
bool native_tiny_tag_editor_screen_action_runnable(
    NativeTinyTagEditorScreen *screen);

#endif /* NCMPCPP_NC_TINY_TAG_EDITOR_H */
