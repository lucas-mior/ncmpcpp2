#if !defined(NCMPCPP_NC_TINY_TAG_EDITOR_H)
#define NCMPCPP_NC_TINY_TAG_EDITOR_H

#include <stdbool.h>

#include "c/ncm_mutable_song.h"
#include "c/ncm_taglib.h"
#include "curses/nc_app_menus.h"
#include "curses/nc_window.h"
#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NativeTinyTagEditorScreen {
    NcScreen screen;
    NcEditorBufferMenu rows;
    NcWindow window;
    NcmMutableSong edited;
    NcScreen *previous_screen;

    int64 start_x;
    int64 width;
    int64 main_start_y;
    int64 main_height;

    bool has_edited;
    bool registered;
} NativeTinyTagEditorScreen;

void native_tiny_tag_editor_screen_init(
    NativeTinyTagEditorScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height, NcColor color, NcBorder border);
void native_tiny_tag_editor_screen_destroy(
    NativeTinyTagEditorScreen *screen);
NcScreen *native_tiny_tag_editor_screen_base(
    NativeTinyTagEditorScreen *screen);
NcEditorBufferMenu *native_tiny_tag_editor_screen_rows(
    NativeTinyTagEditorScreen *screen);
NcmMutableSong *native_tiny_tag_editor_screen_edited(
    NativeTinyTagEditorScreen *screen);
void native_tiny_tag_editor_screen_set_geometry(
    NativeTinyTagEditorScreen *screen, int64 start_x, int64 width,
    int64 main_start_y, int64 main_height);
bool native_tiny_tag_editor_screen_set_edited_song(
    NativeTinyTagEditorScreen *screen, NcmSong *song);
bool native_tiny_tag_editor_screen_set_edited_mutable_song(
    NativeTinyTagEditorScreen *screen, NcmMutableSong *song);
bool native_tiny_tag_editor_screen_reload_rows(
    NativeTinyTagEditorScreen *screen,
    NcmTaglibAudioProperties *properties,
    bool extended_tags_supported);
bool native_tiny_tag_editor_screen_set_tag_value(
    NativeTinyTagEditorScreen *screen, enum NcmTagsField field,
    char *value, int32 value_len, char *separator, int32 separator_len);
bool native_tiny_tag_editor_screen_set_filename(
    NativeTinyTagEditorScreen *screen, char *name, int32 name_len);
bool native_tiny_tag_editor_screen_save(
    NativeTinyTagEditorScreen *screen, char *music_dir);
bool native_tiny_tag_editor_screen_action_runnable(
    NativeTinyTagEditorScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_TINY_TAG_EDITOR_H */
