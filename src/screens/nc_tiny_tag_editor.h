#if !defined(NCMPCPP_NC_TINY_TAG_EDITOR_H)
#define NCMPCPP_NC_TINY_TAG_EDITOR_H

#include "cbase.h"

#include "c/ncm_mutable_song.h"
#include "c/ncm_taglib.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#define TINY_TAG_EDITOR_TAG_ROW(FIELD) \
    ((int32) TINY_TAG_EDITOR_FIRST_TAG_ROW + (int32)(FIELD))

enum TinyTagEditorRow {
    TINY_TAG_EDITOR_FILE_NAME_INFO_ROW,
    TINY_TAG_EDITOR_DIRECTORY_INFO_ROW,
    TINY_TAG_EDITOR_UNUSED_INFO_ROW,
    TINY_TAG_EDITOR_LENGTH_INFO_ROW,
    TINY_TAG_EDITOR_BITRATE_INFO_ROW,
    TINY_TAG_EDITOR_SAMPLE_RATE_INFO_ROW,
    TINY_TAG_EDITOR_CHANNELS_INFO_ROW,
    TINY_TAG_EDITOR_FIRST_SEPARATOR_ROW,
    TINY_TAG_EDITOR_FIRST_TAG_ROW,
    TINY_TAG_EDITOR_LAST_TAG_ROW =
        TINY_TAG_EDITOR_FIRST_TAG_ROW + NCM_TAGS_FIELD_LAST - 1,
    TINY_TAG_EDITOR_SECOND_SEPARATOR_ROW,
    TINY_TAG_EDITOR_FILE_NAME_EDIT_ROW,
    TINY_TAG_EDITOR_THIRD_SEPARATOR_ROW,
    TINY_TAG_EDITOR_SAVE_ROW,
    TINY_TAG_EDITOR_CANCEL_ROW,
    TINY_TAG_EDITOR_ROW_COUNT,
};

#define ENUM_NAME TinyTagEditorOpenResult
#define ENUM_PREFIX_ TINY_TAG_EDITOR_OPEN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(TINY_TAG_EDITOR_OPEN_SUCCESS) \
    X(TINY_TAG_EDITOR_OPEN_INVALID_ARGUMENT) \
    X(TINY_TAG_EDITOR_OPEN_STREAM) \
    X(TINY_TAG_EDITOR_OPEN_MISSING_MUSIC_DIRECTORY) \
    X(TINY_TAG_EDITOR_OPEN_UNREADABLE_FILE) \
    X(TINY_TAG_EDITOR_OPEN_PREPARE_FAILED)
#include "cbase/xenums.c"

#define ENUM_NAME TinyTagEditorPromptResult
#define ENUM_PREFIX_ TINY_TAG_EDITOR_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(TINY_TAG_EDITOR_PROMPT_ERROR) \
    X(TINY_TAG_EDITOR_PROMPT_ABORTED) \
    X(TINY_TAG_EDITOR_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct TinyTagEditorHooks {
    enum TinyTagEditorPromptResult (*prompt)(
        void *user, char *label, int32 label_len, NcmStringView initial,
        StrBuilder *result);
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
} TinyTagEditorHooks;

typedef struct TinyTagEditorScreen {
    NcScreen screen;
    NcEditorBufferMenu rows;
    NcWindow window;
    TinyTagEditorHooks hooks;
    NcmMutableSong edited;
    StrBuilder music_dir;
    StrBuilder tag_separator;
    NcScreen *previous_screen;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;

    bool has_edited;
    bool show_duplicate_tags;
    bool registered;
} TinyTagEditorScreen;

void tiny_tag_editor_screen_init(
    TinyTagEditorScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height, NcColor color, NcBorder border);
void tiny_tag_editor_screen_destroy(
    TinyTagEditorScreen *screen);
NcScreen *tiny_tag_editor_screen_base(
    TinyTagEditorScreen *screen);

void tiny_tag_editor_screen_set_hooks(
    TinyTagEditorScreen *screen, TinyTagEditorHooks hooks);
NcEditorBufferMenu *tiny_tag_editor_screen_rows(
    TinyTagEditorScreen *screen);
void tiny_tag_editor_screen_set_geometry(
    TinyTagEditorScreen *screen, int32 start_x, int32 width,
    int32 main_start_y, int32 main_height);
bool tiny_tag_editor_screen_set_edited_song(
    TinyTagEditorScreen *screen, NcmSong *song);
enum TinyTagEditorOpenResult
tiny_tag_editor_screen_open_song(
    TinyTagEditorScreen *screen, NcmSong *song,
    char *music_dir, int32 music_dir_len, char *tag_separator,
    int32 tag_separator_len, bool show_duplicate_tags, StrBuilder *path);
bool tiny_tag_editor_screen_reload_rows(
    TinyTagEditorScreen *screen,
    NcmTaglibAudioProperties *properties,
    bool extended_tags_supported, char *tag_separator,
    int32 tag_separator_len, bool show_duplicate_tags);
bool tiny_tag_editor_screen_set_tag_value(
    TinyTagEditorScreen *screen, enum NcmTagsField field,
    char *value, int32 value_len, char *separator, int32 separator_len);
bool tiny_tag_editor_screen_set_filename(
    TinyTagEditorScreen *screen, char *name, int32 name_len);
bool tiny_tag_editor_screen_set_filename_stem(
    TinyTagEditorScreen *screen, char *stem, int32 stem_len);
bool tiny_tag_editor_screen_run_row(
    TinyTagEditorScreen *screen, int32 row);
bool tiny_tag_editor_screen_run_current(
    TinyTagEditorScreen *screen);
bool tiny_tag_editor_screen_action_runnable(
    TinyTagEditorScreen *screen);

#endif /* NCMPCPP_NC_TINY_TAG_EDITOR_H */
