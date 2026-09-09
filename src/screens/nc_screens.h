#if !defined(NC_SCREENS_H)
#define NC_SCREENS_H

#include "cbase.h"

#include "c/ncm_c.h"
#include "curses/nc_curses.h"
#include "configura.h"
#include "lastfm_service.h"
#include "lyrics_fetcher.h"

#include <fftw3.h>

/* screens/screen_defs.h */

#define SCREEN_FLAG_NONE 0
#define SCREEN_FLAG_STARTUP 1

#define SCREEN_TYPE_BROWSER_ENTRY(XX)                                      \
    XX(SCREEN_TYPE_BROWSER, NC_SCREEN_TYPE_BROWSER, 1, browser,            \
      SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_HELP_ENTRY(XX)                                         \
    XX(SCREEN_TYPE_HELP, NC_SCREEN_TYPE_HELP, 2, help,                     \
      SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_LASTFM_ENTRY(XX)                                       \
    XX(SCREEN_TYPE_LASTFM, NC_SCREEN_TYPE_LASTFM, 3, last_fm,              \
      SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_LYRICS_ENTRY(XX)                                       \
    XX(SCREEN_TYPE_LYRICS, NC_SCREEN_TYPE_LYRICS, 4, lyrics,               \
      SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(XX)                                \
    XX(SCREEN_TYPE_MEDIA_LIBRARY, NC_SCREEN_TYPE_MEDIA_LIBRARY, 5,         \
      media_library, SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_OUTPUTS_ENTRY(XX)                                      \
    XX(SCREEN_TYPE_OUTPUTS, NC_SCREEN_TYPE_OUTPUTS, 6, outputs,            \
      SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_PLAYLIST_ENTRY(XX)                                     \
    XX(SCREEN_TYPE_PLAYLIST, NC_SCREEN_TYPE_PLAYLIST, 7, playlist,         \
      SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(XX)                              \
    XX(SCREEN_TYPE_PLAYLIST_EDITOR, NC_SCREEN_TYPE_PLAYLIST_EDITOR,        \
      8, playlist_edit, SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_SEARCH_ENGINE_ENTRY(XX)                                \
    XX(SCREEN_TYPE_SEARCH_ENGINE, NC_SCREEN_TYPE_SEARCH_ENGINE,            \
      9, search_engine, SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(XX)                         \
    XX(SCREEN_TYPE_SELECTED_ITEMS_ADDER,                                   \
      NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER, 10, selected_items_adder,       \
      SCREEN_FLAG_NONE)
#define SCREEN_TYPE_SERVER_INFO_ENTRY(XX)                                  \
    XX(SCREEN_TYPE_SERVER_INFO, NC_SCREEN_TYPE_SERVER_INFO, 11,            \
      server_info, SCREEN_FLAG_NONE)
#define SCREEN_TYPE_SONG_INFO_ENTRY(XX)                                    \
    XX(SCREEN_TYPE_SONG_INFO, NC_SCREEN_TYPE_SONG_INFO, 12,                \
      song_info, SCREEN_FLAG_NONE)
#define SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(XX)                         \
    XX(SCREEN_TYPE_SORT_PLAYLIST_DIALOG,                                   \
      NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG, 13, sort_playlist_dialog,       \
      SCREEN_FLAG_NONE)
#define SCREEN_TYPE_TAG_EDIT_ENTRY(XX)                                     \
    XX(SCREEN_TYPE_TAG_EDIT, NC_SCREEN_TYPE_TAG_EDIT, 14,                  \
      tag_edit, SCREEN_FLAG_STARTUP)
#define SCREEN_TYPE_TINY_TAG_EDIT_ENTRY(XX)                                \
    XX(SCREEN_TYPE_TINY_TAG_EDIT, NC_SCREEN_TYPE_TINY_TAG_EDIT,            \
      15, tiny_tag_edit, SCREEN_FLAG_NONE)
#define SCREEN_TYPE_VISUALIZER_ENTRY(XX)                                   \
    XX(SCREEN_TYPE_VISUALIZER, NC_SCREEN_TYPE_VISUALIZER, 16,              \
      visualizer, SCREEN_FLAG_STARTUP)

#define SCREEN_ALL_TYPES(XX)                                               \
    SCREEN_TYPE_BROWSER_ENTRY(XX)                                          \
    SCREEN_TYPE_HELP_ENTRY(XX)                                             \
    SCREEN_TYPE_LASTFM_ENTRY(XX)                                           \
    SCREEN_TYPE_LYRICS_ENTRY(XX)                                           \
    SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(XX)                                    \
    SCREEN_TYPE_OUTPUTS_ENTRY(XX)                                          \
    SCREEN_TYPE_PLAYLIST_ENTRY(XX)                                         \
    SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(XX)                                  \
    SCREEN_TYPE_SEARCH_ENGINE_ENTRY(XX)                                    \
    SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(XX)                             \
    SCREEN_TYPE_SERVER_INFO_ENTRY(XX)                                      \
    SCREEN_TYPE_SONG_INFO_ENTRY(XX)                                        \
    SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(XX)                             \
    SCREEN_TYPE_TAG_EDIT_ENTRY(XX)                                         \
    SCREEN_TYPE_TINY_TAG_EDIT_ENTRY(XX)                                    \
    SCREEN_TYPE_VISUALIZER_ENTRY(XX)

#if defined(ENABLE_OUTPUTS)
#define SCREEN_ENABLED_OUTPUTS_TYPES(XX)                                   \
    SCREEN_TYPE_OUTPUTS_ENTRY(XX)
#else
#define SCREEN_ENABLED_OUTPUTS_TYPES(XX)
#endif

#if defined(HAVE_TAGLIB_H)
#define SCREEN_ENABLED_TAG_EDIT_TYPES(XX)                                  \
    SCREEN_TYPE_TAG_EDIT_ENTRY(XX)                                         \
    SCREEN_TYPE_TINY_TAG_EDIT_ENTRY(XX)
#else
#define SCREEN_ENABLED_TAG_EDIT_TYPES(XX)
#endif

#if defined(ENABLE_VISUALIZER)
#define SCREEN_ENABLED_VISUALIZER_TYPES(XX)                                \
    SCREEN_TYPE_VISUALIZER_ENTRY(XX)
#else
#define SCREEN_ENABLED_VISUALIZER_TYPES(XX)
#endif

#define SCREEN_TYPES(XX)                           \
    SCREEN_TYPE_BROWSER_ENTRY(XX)                  \
    SCREEN_TYPE_HELP_ENTRY(XX)                     \
    SCREEN_TYPE_LASTFM_ENTRY(XX)                   \
    SCREEN_TYPE_LYRICS_ENTRY(XX)                   \
    SCREEN_TYPE_MEDIA_LIBRARY_ENTRY(XX)            \
    SCREEN_ENABLED_OUTPUTS_TYPES(XX)               \
    SCREEN_TYPE_PLAYLIST_ENTRY(XX)                 \
    SCREEN_TYPE_PLAYLIST_EDITOR_ENTRY(XX)          \
    SCREEN_TYPE_SEARCH_ENGINE_ENTRY(XX)            \
    SCREEN_TYPE_SELECTED_ITEMS_ADDER_ENTRY(XX)     \
    SCREEN_TYPE_SERVER_INFO_ENTRY(XX)              \
    SCREEN_TYPE_SONG_INFO_ENTRY(XX)                \
    SCREEN_TYPE_SORT_PLAYLIST_DIALOG_ENTRY(XX)     \
    SCREEN_ENABLED_TAG_EDIT_TYPES(XX)              \
    SCREEN_ENABLED_VISUALIZER_TYPES(XX)

#define SCREEN_NC_TYPE_ENUM_FIELD(                     \
    screen_type, nc_type, nc_value, alias, flags       \
)                                                      \
    nc_type = nc_value,

#define SCREEN_TYPE_XENUM_FIELD(                       \
    screen_type, nc_type, nc_value, alias, flags       \
)                                                      \
    XX(screen_type, alias)

#define SCREEN_TYPE_ENUM_FIELDS                        \
    SCREEN_TYPES(SCREEN_TYPE_XENUM_FIELD)


#define NCM_APP_SCREEN_DIRECT_STORAGE_TYPES(XX)                \
    XX(BrowserScreen, browser_screen)                          \
    XX(LastfmScreen, lastfm_screen)                            \
    XX(LyricsScreen, lyrics_screen)                            \
    XX(VisualizerScreen, visualizer_screen)                    \
    XX(PlaylistScreen, playlist_screen)                        \
    XX(PlaylistEditScreen, playlist_edit_screen)               \
    XX(SelectedItemsAdderScreen, selected_items_adder_screen)  \
    XX(SortPlaylistDialog, sort_playlist_dialog)               \
    XX(SearchEngineScreen, search_engine_screen)               \
    XX(MediaLibraryScreen, media_library_screen)               \
    XX(TagEditScreen, tag_edit_screen)                         \
    XX(TinyTagEditScreen, tiny_tag_edit_screen)

#define NCM_APP_SCREEN_WRAPPED_STORAGE_TYPES(XX)               \
    XX(HelpScreen, help_screen)                                \
    XX(OutputsScreen, outputs_screen)                          \
    XX(ServerInfoScreen, server_info_screen)                   \
    XX(SongInfoScreen, song_info_screen)

#define NCM_APP_SCREEN_INIT_FLAGS(XX)                          \
    XX(browser_screen_initialized)                             \
    XX(lastfm_screen_initialized)                              \
    XX(lyrics_screen_initialized)                              \
    XX(visualizer_screen_initialized)                          \
    XX(playlist_edit_screen_initialized)                       \
    XX(selected_items_adder_screen_initialized)                \
    XX(sort_playlist_dialog_initialized)                       \
    XX(search_engine_screen_initialized)                       \
    XX(media_library_screen_initialized)                       \
    XX(tag_edit_screen_initialized)                            \
    XX(tiny_tag_edit_screen_initialized)                       \
    XX(playlist_screen_initialized)

#define NCM_APP_SCREEN_DIRECT_ACCESSOR_TYPES(XX)                               \
  XX(browser, BrowserScreen, browser_screen,                                   \
    browser_screen_base(&browser_screen))                                      \
  XX(lastfm, LastfmScreen, lastfm_screen,                                      \
    lastfm_screen_base(&lastfm_screen))                                        \
  XX(lyrics, LyricsScreen, lyrics_screen,                                      \
    lyrics_screen_base(&lyrics_screen))                                        \
  XX(playlist, PlaylistScreen, playlist_screen,                                \
    playlist_screen_base(&playlist_screen))                                    \
  XX(playlist_edit, PlaylistEditScreen, playlist_edit_screen,                  \
    playlist_edit_screen_base(&playlist_edit_screen))                          \
  XX(selected_items_adder, SelectedItemsAdderScreen,                           \
    selected_items_adder_screen,                                               \
    selected_items_adder_screen_base(&selected_items_adder_screen))            \
  XX(sort_playlist_dialog, SortPlaylistDialog, sort_playlist_dialog,           \
    sort_playlist_dialog_base(&sort_playlist_dialog))                          \
  XX(search_engine, SearchEngineScreen, search_engine_screen,                  \
    search_engine_screen_base(&search_engine_screen))                          \
  XX(media_library, MediaLibraryScreen, media_library_screen,                  \
    media_library_screen_base(&media_library_screen))                          \
  XX(tag_edit, TagEditScreen, tag_edit_screen,                                 \
    tag_edit_screen_base(&tag_edit_screen))                                    \
  XX(tiny_tag_edit, TinyTagEditScreen, tiny_tag_edit_screen,                   \
    tiny_tag_edit_screen_base(&tiny_tag_edit_screen))

#define NCM_APP_SCREEN_WRAPPED_ACCESSOR_TYPES(XX)                              \
    XX(help, nc_help_screen_base(&help_screen.screen))                         \
    XX(server_info, nc_server_info_screen_base(&server_info_screen.screen))    \
    XX(song_info, nc_song_info_screen_base(&song_info_screen.screen))

#define NCM_APP_SCREEN_TYPED_WRAPPED_ACCESSOR_TYPES(XX)                        \
    XX(help, app_screen_help, NcHelpScreen, &help_screen.screen)

#define NCM_APP_SCREEN_STANDARD_REGISTER_TYPES(XX)                             \
    XX(browser)                                                                \
    XX(help)                                                                   \
    XX(lastfm)                                                                 \
    XX(lyrics)                                                                 \
    XX(visualizer)                                                             \
    XX(playlist)                                                               \
    XX(playlist_edit)                                                        \
    XX(search_engine)                                                          \
    XX(media_library)                                                          \
    XX(tag_edit)                                                             \
    XX(tiny_tag_edit)                                                        \
    XX(song_info)                                                              \
    XX(server_info)                                                            \
    XX(outputs)

#define NCM_APP_SCREEN_REPLACE_REGISTER_TYPES(XX)                 \
    XX(selected_items_adder, NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER) \
    XX(sort_playlist_dialog, NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG)

#define NCM_APP_SCREEN_SIMPLE_SWITCH_TYPES(XX)                    \
    XX(browser)                                                   \
    XX(help)                                                      \
    XX(playlist)                                                  \
    XX(playlist_edit)                                           \
    XX(selected_items_adder)                                      \
    XX(search_engine)                                             \
    XX(media_library)                                             \
    XX(tag_edit)                                                \
    XX(song_info)                                                 \
    XX(server_info)                                               \
    XX(outputs)

#define NCM_APP_SCREEN_REGISTER_SWITCH_TYPES(XX)                  \
    XX(tiny_tag_edit)

#define NCM_APP_SCREEN_IS_CURRENT_TYPES(XX)                       \
    XX(browser)                                                   \
    XX(help)                                                      \
    XX(lastfm)                                                    \
    XX(lyrics)                                                    \
    XX(visualizer)                                                \
    XX(playlist)                                                  \
    XX(playlist_edit)                                           \
    XX(selected_items_adder)                                      \
    XX(sort_playlist_dialog)                                      \
    XX(search_engine)                                             \
    XX(media_library)                                             \
    XX(tag_edit)                                                \
    XX(tiny_tag_edit)                                           \
    XX(song_info)                                                 \
    XX(server_info)                                               \
    XX(outputs)

#if defined(ENABLE_OUTPUTS)
#define NCM_APP_SCREEN_ENABLED_OUTPUTS(XX) XX(outputs)
#define NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(XX)               \
    XX(outputs, NC_SCREEN_TYPE_OUTPUTS)
#else
#define NCM_APP_SCREEN_ENABLED_OUTPUTS(XX)
#define NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(XX)
#endif

#if defined(HAVE_TAGLIB_H)
#define NCM_APP_SCREEN_ENABLED_TAG_EDIT(XX)                   \
    XX(tag_edit)                                              \
    XX(tiny_tag_edit)
#define NCM_APP_SCREEN_ENABLED_TAG_EDIT_RESIZE(XX)            \
    XX(tag_edit, NC_SCREEN_TYPE_TAG_EDIT)                   \
    XX(tiny_tag_edit, NC_SCREEN_TYPE_TINY_TAG_EDIT)
#else
#define NCM_APP_SCREEN_ENABLED_TAG_EDIT(XX)
#define NCM_APP_SCREEN_ENABLED_TAG_EDIT_RESIZE(XX)
#endif

#if defined(ENABLE_VISUALIZER)
#define NCM_APP_SCREEN_ENABLED_VISUALIZER(XX) XX(visualizer)
#define NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(XX)            \
    XX(visualizer, NC_SCREEN_TYPE_VISUALIZER)
#else
#define NCM_APP_SCREEN_ENABLED_VISUALIZER(XX)
#define NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(XX)
#endif

#define NCM_APP_SCREEN_INIT_ALL_TYPES(XX)                         \
    XX(browser)                                                   \
    XX(help)                                                      \
    XX(lastfm)                                                    \
    XX(lyrics)                                                    \
    XX(media_library)                                             \
    XX(playlist)                                                  \
    XX(playlist_edit)                                           \
    XX(search_engine)                                             \
    XX(selected_items_adder)                                      \
    XX(server_info)                                               \
    XX(song_info)                                                 \
    XX(sort_playlist_dialog)                                      \
    NCM_APP_SCREEN_ENABLED_TAG_EDIT(XX)                         \
    NCM_APP_SCREEN_ENABLED_VISUALIZER(XX)                         \
    NCM_APP_SCREEN_ENABLED_OUTPUTS(XX)

#define NCM_APP_SCREEN_REGISTER_INITIAL_TYPES(XX)                 \
    XX(browser)                                                   \
    XX(help)                                                      \
    XX(lastfm)                                                    \
    XX(media_library)                                             \
    XX(search_engine)                                             \
    XX(selected_items_adder)                                      \
    XX(song_info)                                                 \
    XX(server_info)                                               \
    NCM_APP_SCREEN_ENABLED_VISUALIZER(XX)                         \
    NCM_APP_SCREEN_ENABLED_TAG_EDIT(XX)                         \
    NCM_APP_SCREEN_ENABLED_OUTPUTS(XX)                            \
    XX(playlist)                                                  \
    XX(playlist_edit)

#define NCM_APP_SCREEN_RESIZE_REQUEST_TYPES(XX)                   \
    XX(browser, NC_SCREEN_TYPE_BROWSER)                           \
    XX(help, NC_SCREEN_TYPE_HELP)                                 \
    XX(lastfm, NC_SCREEN_TYPE_LASTFM)                             \
    XX(lyrics, NC_SCREEN_TYPE_LYRICS)                             \
    XX(media_library, NC_SCREEN_TYPE_MEDIA_LIBRARY)               \
    XX(playlist, NC_SCREEN_TYPE_PLAYLIST)                         \
    XX(playlist_edit, NC_SCREEN_TYPE_PLAYLIST_EDITOR)           \
    XX(search_engine, NC_SCREEN_TYPE_SEARCH_ENGINE)               \
    XX(selected_items_adder, NC_SCREEN_TYPE_SELECTED_ITEMS_ADDER) \
    XX(server_info, NC_SCREEN_TYPE_SERVER_INFO)                   \
    XX(song_info, NC_SCREEN_TYPE_SONG_INFO)                       \
    XX(sort_playlist_dialog, NC_SCREEN_TYPE_SORT_PLAYLIST_DIALOG) \
    NCM_APP_SCREEN_ENABLED_TAG_EDIT_RESIZE(XX)                  \
    NCM_APP_SCREEN_ENABLED_VISUALIZER_RESIZE(XX)                  \
    NCM_APP_SCREEN_ENABLED_OUTPUTS_RESIZE(XX)

/* screens/nc_screen.h */
#define NC_SCREEN_DEFAULT_WINDOW_TIMEOUT 500

enum NcScreenType {
    NC_SCREEN_TYPE_UNKNOWN = 0,
    SCREEN_ALL_TYPES(SCREEN_NC_TYPE_ENUM_FIELD)
};

#define NC_SCREEN_REGISTRY_MAX_SCREENS 64

typedef struct NcScreen NcScreen;
typedef struct NcScreenRegistry NcScreenRegistry;
typedef struct NcScreenResizeParams NcScreenResizeParams;
typedef void NcScreenEachCallback(NcScreen *, void *);

typedef struct NcScreenResizeParams {
    int32 x_offset;
    int32 width;
} NcScreenResizeParams;

typedef struct NcScreenOps {
    NcWindow *(*active_window)(NcScreen *);
    void (*refresh)(NcScreen *);
    void (*refresh_window)(NcScreen *);
    void (*scroll)(NcScreen *, enum NcScroll);
    void (*list_change_finished)(NcScreen *);
    bool (*can_run_current)(NcScreen *);
    int32 (*run_current)(NcScreen *);
    void (*switch_to)(NcScreen *);
    void (*resize)(NcScreen *);
    int32 (*window_timeout_callback)(NcScreen *);
    char *(*title)(NcScreen *);
    void (*update)(NcScreen *);
    void (*mouse_button_pressed)(NcScreen *, MEVENT);
    bool (*is_lockable_callback)(NcScreen *);
    bool (*is_mergable_callback)(NcScreen *);
    void (*destroy)(NcScreen *);

    int32 window_timeout;

    bool lockable;
    bool mergable;
} NcScreenOps;

struct NcScreen {
    const NcScreenOps *ops;
    NcScreenOps ops_storage;
    void *user;
    enum NcScreenType type;
    bool has_to_be_resized;
    bool has_to_be_updated;
};

extern const NcScreenOps nc_screen_default_ops;

struct NcScreenRegistry {
    NcScreen *screens[NC_SCREEN_REGISTRY_MAX_SCREENS];
    NcScreen *current_screen;
    NcScreen *previous_screen;
    NcScreen *locked_screen;
    NcScreen *inactive_screen;
    int32 screens_len;
};

void nc_screen_init_ops(NcScreen *, NcScreenOps, void *, enum NcScreenType);
NcWindow *nc_screen_default_active_window(NcScreen *);
void nc_screen_noop_refresh(NcScreen *);
void nc_screen_noop_refresh_window(NcScreen *);
void nc_screen_noop_scroll(NcScreen *, enum NcScroll);
void nc_screen_noop_list_change_finished(NcScreen *);
bool nc_screen_default_can_run_current(NcScreen *);
int32 nc_screen_default_run_current(NcScreen *);
void nc_screen_noop_switch_to(NcScreen *);
void nc_screen_noop_resize(NcScreen *);
char *nc_screen_default_title(NcScreen *);
void nc_screen_noop_update(NcScreen *);
void nc_screen_noop_mouse_button_pressed(NcScreen *, MEVENT);
void nc_screen_noop_destroy(NcScreen *);
NcWindow *nc_screen_active_window(NcScreen *);
void nc_screen_refresh(NcScreen *);
void nc_screen_refresh_window(NcScreen *);
void nc_screen_scroll(NcScreen *, enum NcScroll);
void nc_screen_finish_list_change(NcScreen *);
bool nc_screen_can_run_current(NcScreen *);
int32 nc_screen_run_current(NcScreen *);
void nc_screen_switch_to(NcScreen *);
void nc_screen_resize(NcScreen *);
int32 nc_screen_window_timeout(NcScreen *);
char *nc_screen_title(NcScreen *);
enum NcScreenType nc_screen_type(NcScreen *);
void nc_screen_mouse_button_pressed(NcScreen *, MEVENT);
bool nc_screen_is_lockable(NcScreen *);
bool nc_screen_is_mergable(NcScreen *);
void nc_screen_set_has_to_be_resized(NcScreen *, bool);
void nc_screen_set_has_to_be_updated(NcScreen *, bool);
void nc_screen_request_resize(NcScreen *);
void nc_screen_request_update(NcScreen *);
void nc_screen_clear_resize_request(NcScreen *);
void nc_screen_clear_update_request(NcScreen *);
NcScreenResizeParams nc_screen_resize_params(NcScreen *);
void nc_screen_get_resize_params(NcScreen *, int32 *x_offset, int32 *width);
void nc_screen_draw_vertical_separator(int32);
void *nc_screen_user(NcScreen *);

int32 nc_screen_registry_register(NcScreenRegistry *, NcScreen *);
int32 nc_screen_registry_unregister(NcScreenRegistry *, NcScreen *);
NcScreen *nc_screen_registry_find(NcScreenRegistry *, enum NcScreenType);
NcScreen *nc_screen_registry_current(NcScreenRegistry *);
NcScreen *nc_screen_registry_previous(NcScreenRegistry *);
NcScreen *nc_screen_registry_locked(NcScreenRegistry *);
bool nc_screen_registry_is_registered(NcScreenRegistry *, NcScreen *);
bool nc_screen_registry_is_current(NcScreenRegistry *, NcScreen *);
void nc_screen_registry_request_resize_current(NcScreenRegistry *);
void nc_screen_registry_request_update_current(NcScreenRegistry *);
NcScreenResizeParams nc_screen_registry_resize_params(NcScreenRegistry *,
                                                      NcScreen *, bool);
int32 nc_screen_registry_switch_to(NcScreenRegistry *, NcScreen *);
int32 nc_screen_registry_lock_current(NcScreenRegistry *);
void nc_screen_registry_unlock(NcScreenRegistry *);
bool nc_screen_registry_is_visible(NcScreenRegistry *, NcScreen *);
void nc_screen_registry_each_visible(NcScreenRegistry *,
                                     NcScreenEachCallback *, void *);
void nc_screen_registry_update_visible(NcScreenRegistry *);
void nc_screen_registry_resize_current(NcScreenRegistry *);
void nc_screen_registry_resize_visible(NcScreenRegistry *);

/* screens/screen_type.h */
#define ENUM_NAME ScreenType
#define ENUM_PREFIX_ SCREEN_TYPE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS SCREEN_TYPE_ENUM_FIELDS
#include "cbase/xenums.c"

enum NcScreenType screen_type_to_nc_type(enum ScreenType);
enum ScreenType screen_type_from_nc_type(enum NcScreenType);
bool screen_type_is_startup(enum ScreenType);
int32 screen_type_parse_startup(char *, int32, enum ScreenType *);
int32 screen_type_parse(char *, int32, enum ScreenType *);

/* screens/screen_switcher.h */ NcScreen *nc_screen_switcher_current(void);
NcScreen *nc_screen_switcher_previous(void);
bool nc_screen_switcher_is_current(NcScreen *);
bool nc_screen_switcher_is_visible(NcScreen *);
int32 nc_screen_switcher_switch_to(NcScreen *, bool);
void nc_screen_switcher_finish_switch(NcScreen *);
void nc_screen_switcher_get_resize_params(NcScreen *, int32 *x_offset,
                                          int32 *width, bool);

/* screens/nc_scrollpad_screen.h */
typedef struct NcScrollpadScreen {
    NcScreen base;

    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;
} NcScrollpadScreen;

void nc_scrollpad_screen_init(NcScrollpadScreen *, NcScreenOps, void *,
                              enum NcScreenType, int32 start_x, int32 start_y,
                              int32 width, int32 height);
void nc_scrollpad_screen_set_geometry(NcScrollpadScreen *, int32 start_x,
                                      int32 start_y, int32 width, int32 height);
void nc_scrollpad_screen_set_main_area(NcScrollpadScreen *, int32 start_x,
                                       int32 width, int32 main_start_y,
                                       int32 main_height);
void nc_scrollpad_screen_set_centered_box(NcScrollpadScreen *, int32 cols,
                                          int32 lines, int32 main_start_y,
                                          int32 main_height, int32 width_num,
                                          int32 width_den, int32 height_num,
                                          int32 height_den);
NcScreen *nc_scrollpad_screen_base(NcScrollpadScreen *);
int32 nc_scrollpad_screen_start_x(NcScrollpadScreen *);
int32 nc_scrollpad_screen_start_y(NcScrollpadScreen *);
int32 nc_scrollpad_screen_width(NcScrollpadScreen *);
int32 nc_scrollpad_screen_height(NcScrollpadScreen *);

/* screens/nc_help.h */
typedef struct NcHelpScreen NcHelpScreen;

typedef struct NcHelpHooks {
    int32 (*render)(void *, NcBuffer *);
    void (*switch_to)(void *);
    void (*resize_layout)(void *, NcHelpScreen *);
    void (*resize_background)(void *);
    void (*destroy)(void *);
    void *user;
} NcHelpHooks;

struct NcHelpScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    StrBuilder search_constraint;
    NcHelpHooks hooks;

    int32 lines_scrolled;
};

void nc_help_screen_init(NcHelpScreen *, NcHelpHooks, int32 start_x,
                         int32 width, int32 main_start_y, int32 main_height,
                         NcColor, NcBorder, int32 lines_scrolled);
void nc_help_screen_set_geometry(NcHelpScreen *, int32 start_x, int32 width,
                                 int32 main_start_y, int32 main_height);
int32 nc_help_screen_reload(NcHelpScreen *);
int32 nc_help_screen_find(NcHelpScreen *, char *, int32, NcmError *);
void nc_help_screen_clear_search(NcHelpScreen *);
NcScreen *nc_help_screen_base(NcHelpScreen *);
int32 nc_help_screen_start_x(NcHelpScreen *);
int32 nc_help_screen_start_y(NcHelpScreen *);
int32 nc_help_screen_width(NcHelpScreen *);
int32 nc_help_screen_height(NcHelpScreen *);

/* screens/nc_lastfm.h */
#include "lastfm_service.h"

typedef struct NcLastfmScreen {
    NcScrollpadScreen scrollpad_screen;
} NcLastfmScreen;

typedef struct LastfmScreen {
    NcLastfmScreen screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    StrBuilder search_constraint;

    NcmLastfmService service;
    NcmLastfmResult result;
    NcmJobQueue jobs;

    char *title;
    int32 title_len;
    int32 title_cap;

    bool has_service;
    bool refresh_window;
    bool initialized;
} LastfmScreen;

void nc_lastfm_screen_init(NcLastfmScreen *, NcScreenOps, void *,
                           int32 start_x, int32 width, int32 main_start_y,
                           int32 main_height);
void nc_lastfm_screen_set_geometry(NcLastfmScreen *, int32 start_x,
                                   int32 width, int32 main_start_y,
                                   int32 main_height);
int32 nc_lastfm_screen_start_x(NcLastfmScreen *);
int32 nc_lastfm_screen_start_y(NcLastfmScreen *);
int32 nc_lastfm_screen_width(NcLastfmScreen *);
int32 nc_lastfm_screen_height(NcLastfmScreen *);

void lastfm_screen_init(LastfmScreen *, int32 start_x, int32 width,
                        int32 main_start_y, int32 main_height, NcColor,
                        NcBorder, int32 lines_scrolled);
void lastfm_screen_destroy(LastfmScreen *);
NcScreen *lastfm_screen_base(LastfmScreen *);
void lastfm_screen_set_geometry(LastfmScreen *, int32 start_x, int32 width,
                                int32 main_start_y, int32 main_height);
int32 lastfm_screen_queue_artist_info(LastfmScreen *, char *artist,
                                      int32 artist_len, char *lang,
                                      int32 lang_len, NcmError *);
int32 lastfm_screen_dispatch_jobs(LastfmScreen *);
void lastfm_screen_update(LastfmScreen *);
char *lastfm_screen_title(LastfmScreen *);
int32 lastfm_screen_find(LastfmScreen *, char *, int32, NcmError *);

/* screens/nc_lyrics.h */
#include "lyrics_fetcher.h"

typedef struct LyricsJob LyricsJob;

typedef enum LyricsMode {
    LYRICS_MODE_PLAIN,
    LYRICS_MODE_SYNCHRONIZED,
    LYRICS_MODE_FETCH_LOG,
} LyricsMode;

typedef struct NcLyricsScreen {
    NcScrollpadScreen scrollpad_screen;

    int32 scroll_begin;
    bool refresh_window;
} NcLyricsScreen;

typedef struct LyricsQueuedSong {
    NcmSong song;
    bool notify;
} LyricsQueuedSong;

typedef struct LyricsScreen {
    NcLyricsScreen screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer display;
    StrBuilder search_constraint;

    StrBuilder title;
    NcmSong song;
    StrBuilder filename;
    NcmLrcDocument lrc;
    NcmLyricsResult result;
    NcmJobQueue jobs;
    LyricsJob *foreground_job;
    LyricsQueuedSong *queued_songs;
    StrBuilder consumer_message;

    NcmLyricsFetcherDef *fetcher;
    int32 queued_songs_len;
    int32 queued_songs_cap;
    int32 active_lrc_line;
    LyricsMode mode;

    bool has_song;
    bool initialized;
} LyricsScreen;

void nc_lyrics_screen_init(NcLyricsScreen *, NcScreenOps, void *,
                           int32 start_x, int32 width, int32 main_start_y,
                           int32 main_height);
void nc_lyrics_screen_set_geometry(NcLyricsScreen *, int32 start_x,
                                   int32 width, int32 main_start_y,
                                   int32 main_height);
NcScreen *nc_lyrics_screen_base(NcLyricsScreen *);
int32 nc_lyrics_screen_start_x(NcLyricsScreen *);
int32 nc_lyrics_screen_start_y(NcLyricsScreen *);
int32 nc_lyrics_screen_width(NcLyricsScreen *);
int32 nc_lyrics_screen_height(NcLyricsScreen *);
void nc_lyrics_screen_request_refresh(NcLyricsScreen *);
int32 nc_lyrics_screen_take_refresh_request(NcLyricsScreen *);
void nc_lyrics_screen_reset_scroll_begin(NcLyricsScreen *);
int32 nc_lyrics_screen_scroll_begin(NcLyricsScreen *);
void nc_lyrics_screen_set_scroll_begin(NcLyricsScreen *, int32);

void lyrics_queued_song_destroy(LyricsQueuedSong *);
void lyrics_queued_song_move(LyricsQueuedSong *dest, LyricsQueuedSong *source);

void lyrics_screen_init(LyricsScreen *, int32 start_x, int32 width,
                        int32 main_start_y, int32 main_height, NcColor,
                        NcBorder, int32 lines_scrolled);
void lyrics_screen_destroy(LyricsScreen *);
NcScreen *lyrics_screen_base(LyricsScreen *);
NcWindow *lyrics_screen_window(LyricsScreen *);
void lyrics_screen_set_geometry(LyricsScreen *, int32 start_x, int32 width,
                                int32 main_start_y, int32 main_height);
int32 lyrics_screen_build_filename(LyricsScreen *, NcmSong *, char *music_dir,
                                   int32 music_dir_len, char *lyrics_dir,
                                   int32 lyrics_dir_len,
                                   bool store_in_song_dir, bool win32_filename);
int32 lyrics_screen_load_file(LyricsScreen *, char *, int32, NcmError *);
int32 lyrics_screen_save_file(LyricsScreen *, char *filename,
                              int32 filename_len, char *lyrics,
                              int32 lyrics_len, NcmError *);
int32 lyrics_screen_fetch(LyricsScreen *, NcmSong *, NcmLyricsFetcherDef *,
                          NcmError *);
int32 lyrics_screen_fetch_in_background(LyricsScreen *, NcmSong *, bool,
                                        NcmError *);
int32 lyrics_screen_dispatch_jobs(LyricsScreen *);
void lyrics_screen_update(LyricsScreen *);
void lyrics_screen_refetch_current(LyricsScreen *, NcmError *);
NcmLyricsFetcherDef *lyrics_screen_toggle_fetcher(LyricsScreen *,
                                                  NcmLyricsFetcherRegistry *);
int32 lyrics_screen_try_take_consumer_message(LyricsScreen *, StrBuilder *);
NcmSong *lyrics_screen_song(LyricsScreen *);
StrBuilder *lyrics_screen_filename(LyricsScreen *);
NcmLrcDocument *lyrics_screen_lrc(LyricsScreen *);
int32 lyrics_buffer_find(NcBuffer *, char *, int32, NcmError *);
void lyrics_buffer_clear_sync_highlight(NcBuffer *);
void lyrics_buffer_highlight_sync_line(NcBuffer *, int32 start, int32 end);
int32 lyrics_screen_find(LyricsScreen *, char *, int32, NcmError *);

/* screens/nc_outputs.h */
typedef struct NcOutputsScreen NcOutputsScreen;

typedef struct NcOutputsHooks {
    void (*fetch_outputs)(void *, NcOutputsScreen *);
    int32 (*toggle_output)(void *, int32 id, bool, char *, int32 name_len);
    void (*switch_to)(void *);
    void (*resize_layout)(void *, NcOutputsScreen *);
    void (*resize_background)(void *);
    void (*destroy)(void *);
    void *user;
} NcOutputsHooks;

typedef struct NcOutputsItem {
    char *name;
    int32 name_len;
    int32 id;
    bool enabled;
} NcOutputsItem;

struct NcOutputsScreen {
    NcScrollpadScreen menu_screen;
    NcWindow window;
    NcMenu menu;
    NcOutputsHooks hooks;
    int32 lines_scrolled;
    bool mouse_scroll_whole_page;
};

void nc_outputs_screen_init(NcOutputsScreen *, NcOutputsHooks, int32 start_x,
                            int32 width, int32 main_start_y, int32 main_height,
                            NcColor, NcBorder, int32 lines_scrolled, bool);
void nc_outputs_screen_set_geometry(NcOutputsScreen *, int32 start_x,
                                    int32 width, int32 main_start_y,
                                    int32 main_height);
void nc_outputs_screen_set_highlight_prefix(NcOutputsScreen *, NcBuffer *);
void nc_outputs_screen_set_highlight_suffix(NcOutputsScreen *, NcBuffer *);
void nc_outputs_screen_fetch_list(NcOutputsScreen *);
void nc_outputs_screen_clear_outputs(NcOutputsScreen *);
void nc_outputs_screen_add_output(NcOutputsScreen *, int32 id, char *,
                                  int32 name_len, bool);
int32 nc_outputs_screen_toggle_current(NcOutputsScreen *);
NcScreen *nc_outputs_screen_base(NcOutputsScreen *);
int32 nc_outputs_screen_start_x(NcOutputsScreen *);
int32 nc_outputs_screen_start_y(NcOutputsScreen *);
int32 nc_outputs_screen_width(NcOutputsScreen *);
int32 nc_outputs_screen_height(NcOutputsScreen *);

/* screens/nc_song_info.h */
typedef struct NcSongInfoScreen NcSongInfoScreen;

typedef struct NcSongInfoHooks {
    int32 (*render)(void *, NcSongInfoScreen *, NcBuffer *);
    void (*switch_to)(void *, NcSongInfoScreen *);
    void (*resize_layout)(void *, NcSongInfoScreen *);
    void (*destroy)(void *);
    void *user;
} NcSongInfoHooks;

struct NcSongInfoScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    NcSongInfoHooks hooks;

    int32 lines_scrolled;
};

void nc_song_info_screen_init(NcSongInfoScreen *, NcSongInfoHooks,
                              int32 start_x, int32 width, int32 main_start_y,
                              int32 main_height, NcColor, NcBorder,
                              int32 lines_scrolled);
void nc_song_info_screen_set_geometry(NcSongInfoScreen *, int32 start_x,
                                      int32 width, int32 main_start_y,
                                      int32 main_height);
int32 nc_song_info_screen_prepare_current(NcSongInfoScreen *);
NcScreen *nc_song_info_screen_base(NcSongInfoScreen *);
int32 nc_song_info_screen_start_x(NcSongInfoScreen *);
int32 nc_song_info_screen_start_y(NcSongInfoScreen *);
int32 nc_song_info_screen_width(NcSongInfoScreen *);
int32 nc_song_info_screen_height(NcSongInfoScreen *);

/* screens/nc_server_info.h */
typedef struct NcServerInfoHooks {
    void (*load_lists)(void *);
    int32 (*render)(void *, NcBuffer *);
    void (*switch_to)(void *);
    void (*resize_layout)(void *);
    void (*resize_background)(void *);
    char *(*title)(void *);
    void (*destroy)(void *);
    void *user;
} NcServerInfoHooks;

typedef struct NcServerInfoScreen {
    NcScrollpadScreen scrollpad_screen;
    NcWindow window;
    NcScrollpad scrollpad;
    NcBuffer buffer;
    NcServerInfoHooks hooks;
} NcServerInfoScreen;

void nc_server_info_screen_init(NcServerInfoScreen *, NcServerInfoHooks,
                                int32 cols, int32 lines, int32 main_start_y,
                                int32 main_height, NcColor, NcBorder);
void nc_server_info_screen_set_dimensions(NcServerInfoScreen *, int32 cols,
                                          int32 lines, int32 main_start_y,
                                          int32 main_height);
NcScreen *nc_server_info_screen_base(NcServerInfoScreen *);
int32 nc_server_info_screen_width(NcServerInfoScreen *);
int32 nc_server_info_screen_height(NcServerInfoScreen *);
int32 nc_server_info_screen_start_x(NcServerInfoScreen *);
int32 nc_server_info_screen_start_y(NcServerInfoScreen *);

/* screens/nc_visualizer.h */
#include "configura.h"

#if defined(HAVE_FFTW3_H)
#define FFTW_NO_Complex 1
#include <fftw3.h>
#endif

#if defined(HAVE_FFTW3_H)
#define VISUALIZER_FREQUENCY_FIELD                                             \
    XX(VISUALIZER_FREQUENCY, spectrum)
#else
#define VISUALIZER_FREQUENCY_FIELD
#endif

#define ENUM_NAME VisualizerScreenType
#define ENUM_PREFIX_ VISUALIZER_TYPE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(VISUALIZER_WAVE, wave)                                                  \
    XX(VISUALIZER_WAVE_FILLED, wave_filled)                                    \
    VISUALIZER_FREQUENCY_FIELD                                                 \
    XX(VISUALIZER_ELLIPSE, ellipse)
#include "cbase/xenums.c"
#undef VISUALIZER_FREQUENCY_FIELD

struct NcmError;
struct MpdClient;
struct NcmMpdOutputList;

typedef struct VisualizerDataSourceHooks {
    int32 (*open_fifo)(void *, char *, int32);
    int32 (*open_udp)(void *, char *location, int32 location_len, char *port,
                      int32 port_len);
    int32 (*read_source)(void *user, int32 fd, void *buffer, int32 buffer_size);
    void (*close_source)(void *, int32);
    int32 (*get_outputs)(void *, struct NcmMpdOutputList *, struct NcmError *);
    int32 (*disable_output)(void *, int32, struct NcmError *);
    int32 (*enable_output)(void *, int32, struct NcmError *);
    void (*sleep_microseconds)(void *, int32);
    void *user;
} VisualizerDataSourceHooks;

typedef struct VisualizerScreenConfig {
    char *source_location;
    char *output_name;
    char *visualizer_chars;
    NcFormattedColor *visualizer_colors;

    int32 source_location_len;
    int32 output_name_len;
    int32 visualizer_chars_len;
    int32 visualizer_colors_len;
    int32 fps;
    int32 spectrum_dft_size;

    double spectrum_gain;
    double spectrum_hz_min;
    double spectrum_hz_max;

    VisualizerDataSourceHooks data_source_hooks;
    enum VisualizerScreenType visualization_type;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;
} VisualizerScreenConfig;

#if defined(HAVE_FFTW3_H)
typedef struct VisualizerBarHeight {
    int32 column;
    double height;
} VisualizerBarHeight;

typedef struct VisualizerFftState {
    double *input;
    fftw_complex *output;
    fftw_plan plan;

    double *freqs_mags;
    double *dft_frequency_space;
    VisualizerBarHeight *bar_heights;

    int32 results_len;
    int32 dft_nonzero_size;
    int32 dft_total_size;

    int32 freqs_mags_len;
    int32 dft_frequency_space_len;
    int32 dft_frequency_space_cap;
    int32 bar_heights_len;
    int32 bar_heights_cap;

    double dynamic_range;
    double hz_min;
    double hz_max;
    double gain;
} VisualizerFftState;
#endif

typedef struct VisualizerScreen {
    NcScreen screen;
    NcWindow window;

    StrBuilder source_location;
    StrBuilder source_port;
    StrBuilder output_name;
    StrBuilder visualizer_chars;
    NcFormattedColor *visualizer_colors;
    VisualizerDataSourceHooks data_source_hooks;

    NcmSampleBuffer samples_in;
    NcmSampleBuffer samples_buf;
    NcmSampleBuffer samples_rendered;
    NcmSampleBuffer left_ch;
    NcmSampleBuffer right_ch;

#if defined(HAVE_FFTW3_H)
    VisualizerFftState fft;
#endif

    double auto_scale_multiplier;
    enum VisualizerScreenType visualization_type;

    int32 source_fd;
    int32 output_id;
    int32 fps;
    int32 sample_rate;
    int64 sample_clock;
    int64 sample_clock_frame_remainder;
    int32 visualizer_colors_len;
    int32 point_char_offset;
    int32 point_char_len;
    int32 bar_char_offset;
    int32 bar_char_len;

    bool reset_output;
    bool sample_clock_initialized;
    bool autoscale;
    bool stereo;
    bool spectrum_smooth_look;
    bool spectrum_smooth_look_legacy_chars;
    bool spectrum_log_scale_x;
    bool spectrum_log_scale_y;
    bool initialized;
} VisualizerScreen;

void visualizer_screen_init(VisualizerScreen *, int32 start_x, int32 start_y,
                            int32 width, int32 height, NcColor, NcBorder,
                            VisualizerScreenConfig *);
void visualizer_screen_destroy(VisualizerScreen *);
VisualizerDataSourceHooks visualizer_data_source_system_hooks(
    struct MpdClient *);
void visualizer_screen_init_data_source(VisualizerScreen *, char *, int32);
int32 visualizer_screen_open_data_source(VisualizerScreen *);
void visualizer_screen_close_data_source(VisualizerScreen *);
int32 visualizer_screen_drain_data_source(VisualizerScreen *);
int32 visualizer_screen_find_output_id(VisualizerScreen *);
NcScreen *visualizer_screen_base(VisualizerScreen *);
NcWindow *visualizer_screen_window(VisualizerScreen *);
void visualizer_screen_set_geometry(VisualizerScreen *, int32 start_x,
                                    int32 start_y, int32 width, int32 height);
void visualizer_screen_init_visualization(VisualizerScreen *);
void visualizer_screen_clear(VisualizerScreen *);
void visualizer_screen_reset_audio_state(VisualizerScreen *);
void visualizer_screen_reset_auto_scale_multiplier(VisualizerScreen *);
void visualizer_screen_toggle_type(VisualizerScreen *);
int32 visualizer_screen_requested_samples(VisualizerScreen *);
void visualizer_screen_push_samples(VisualizerScreen *, int16 *, int32);
int32 visualizer_screen_take_render_samples(VisualizerScreen *, int16 *, int32);
int32 visualizer_screen_split_stereo(VisualizerScreen *, int16 *, int32);
void visualizer_screen_apply_auto_scale(VisualizerScreen *, int16 *, int32);
void visualizer_screen_draw(VisualizerScreen *, int16 *, int32);
int16 visualizer_clamp_sample(int32);

/* screens/nc_media_library.h */
#define MEDIA_LIBRARY_FETCH_DELAY_MS 250

#define ENUM_NAME MediaLibraryMode
#define ENUM_PREFIX_ MEDIA_LIBRARY_MODE_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(MEDIA_LIBRARY_MODE_THREE_COLUMNS, three_columns)                        \
    XX(MEDIA_LIBRARY_MODE_TWO_COLUMNS, two_columns)                            \
    XX(MEDIA_LIBRARY_MODE_ALBUM_ONLY, album_only)
#include "cbase/xenums.c"

#define ENUM_NAME MediaLibraryColumn
#define ENUM_PREFIX_ MEDIA_LIBRARY_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(MEDIA_LIBRARY_COLUMN_TAGS, Tags)                                        \
    XX(MEDIA_LIBRARY_COLUMN_ALBUMS, Albums)                                    \
    XX(MEDIA_LIBRARY_COLUMN_SONGS, Songs)
#include "cbase/xenums.c"

typedef struct MediaLibraryAlbumItem {
    NcMediaLibraryAlbumRow row;
    uint32 menu_flags;
} MediaLibraryAlbumItem;

NCM_ARRAY_DECLARE_TYPE(MediaLibraryTagArray, NcMediaLibraryTagRow)
NCM_ARRAY_DECLARE_CLEAR(media_library_tag_array, MediaLibraryTagArray)
NCM_ARRAY_DECLARE_DESTROY(media_library_tag_array, MediaLibraryTagArray)
NCM_ARRAY_DECLARE_MOVE(media_library_tag_array, MediaLibraryTagArray)
NCM_ARRAY_DECLARE_RESERVE(media_library_tag_array, MediaLibraryTagArray)

NCM_ARRAY_DECLARE_APPEND(media_library_tag_array, MediaLibraryTagArray,
                         NcMediaLibraryTagRow)

NCM_ARRAY_DECLARE_REMOVE_ORDERED(media_library_tag_array, MediaLibraryTagArray)

NCM_ARRAY_DECLARE_TYPE(MediaLibraryAlbumArray, MediaLibraryAlbumItem)
NCM_ARRAY_DECLARE_CLEAR(media_library_album_array, MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_DESTROY(media_library_album_array, MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_MOVE(media_library_album_array, MediaLibraryAlbumArray)
NCM_ARRAY_DECLARE_RESERVE(media_library_album_array, MediaLibraryAlbumArray)

NCM_ARRAY_DECLARE_APPEND(media_library_album_array, MediaLibraryAlbumArray,
                         MediaLibraryAlbumItem)

NCM_ARRAY_DECLARE_REMOVE_ORDERED(media_library_album_array,
                                 MediaLibraryAlbumArray)

typedef struct MediaLibrarySongQuery {
    char *primary_value;
    char *album;
    char *date;

    int32 primary_value_len;
    int32 album_len;
    int32 date_len;

    enum mpd_tag_type primary_tag;
    bool match_primary_tag;
    bool match_album;
    bool match_date;
} MediaLibrarySongQuery;

typedef struct MediaLibraryColumnState {
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    NcmRegex filter_regex;
    NcmRegex search_regex;
    bool filter_enabled;
    bool search_enabled;
} MediaLibraryColumnState;

typedef struct MediaLibraryHooks {
    int32 (*list_tags)(void *, enum mpd_tag_type, StringViewList *,
                       NcmError *);
    int32 (*list_all_songs)(void *, NcmMpdSongList *, NcmError *);
    int32 (*search_songs)(void *, MediaLibrarySongQuery *, NcmMpdSongList *,
                          NcmError *);
    int32 (*add_songs)(void *, NcmSongArray *, bool, NcmError *);
    void (*destroy)(void *);
    void *user;
} MediaLibraryHooks;

typedef struct MediaLibraryScreen {
    NcScreen screen;
    NcMediaLibraryTagMenu tags;
    NcMediaLibraryAlbumMenu albums;
    NcMediaLibrarySongMenu songs;
    NcWindow tags_window;
    NcWindow albums_window;
    NcWindow songs_window;
    MediaLibraryHooks hooks;

    MediaLibraryColumnState column_state[
        MEDIA_LIBRARY_COLUMN_COUNT];
    StrBuilder tags_title;
    StrBuilder albums_title;
    StrBuilder songs_title;
    int64 update_timer;
    NcMediaLibraryTagRow observed_tag;
    NcMediaLibraryAlbumRow observed_album;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 fetching_delay_ms;
    int32 window_timeout_ms;

    enum MediaLibraryMode mode;
    enum MediaLibraryColumn active_column;

    bool tags_update_request;
    bool albums_update_request;
    bool songs_update_request;
    bool sort_by_mtime;
    bool observed_tag_valid;
    bool observed_album_valid;
    bool registered;
} MediaLibraryScreen;

MediaLibraryHooks media_library_mpd_hooks(MpdClient *);
void media_library_screen_init(MediaLibraryScreen *, MediaLibraryHooks,
                               int32 start_x, int32 width, int32 main_start_y,
                               int32 main_height, NcColor, NcBorder);
void media_library_screen_destroy(MediaLibraryScreen *);
NcScreen *media_library_screen_base(MediaLibraryScreen *);
NcMenu *media_library_screen_active_menu(MediaLibraryScreen *);
NcWindow *media_library_screen_active_window(MediaLibraryScreen *);
void media_library_screen_set_geometry(MediaLibraryScreen *, int32 start_x,
                                       int32 width, int32 main_start_y,
                                       int32 main_height);

int32 media_library_screen_column_count(MediaLibraryScreen *);
int32 media_library_screen_set_mode(MediaLibraryScreen *,
                                    enum MediaLibraryMode);

int32 media_library_screen_toggle_mode(MediaLibraryScreen *,
                                       enum MediaLibraryMode *);
enum MediaLibraryColumn media_library_screen_active_column(
    MediaLibraryScreen *);
bool media_library_screen_has_available_item(MediaLibraryScreen *);
int32 media_library_screen_set_active_column(MediaLibraryScreen *,
                                             enum MediaLibraryColumn);
bool media_library_screen_column_is_visible(MediaLibraryScreen *,
                                            enum MediaLibraryColumn);
MediaLibraryColumnState *media_library_screen_column_state(
    MediaLibraryScreen *, enum MediaLibraryColumn);
StrBuilder *media_library_screen_active_filter_constraint(MediaLibraryScreen *);
StrBuilder *media_library_screen_active_search_constraint(MediaLibraryScreen *);
NcMediaLibraryTagRow *media_library_screen_current_tag(MediaLibraryScreen *);
NcMediaLibraryAlbumRow *media_library_screen_current_album(
    MediaLibraryScreen *);

bool media_library_screen_has_current_primary_tag_value(MediaLibraryScreen *,
                                                        char **, int32 *);
bool media_library_screen_has_current_album_value(MediaLibraryScreen *,
                                                  char **, int32 *);
void media_library_screen_format_tag_row(MediaLibraryScreen *,
                                         NcMediaLibraryTagRow *, StrBuilder *);
void media_library_screen_format_album_row(MediaLibraryScreen *,
                                           NcMediaLibraryAlbumRow *,
                                           StrBuilder *);
void media_library_screen_format_song_row(MediaLibraryScreen *, NcmSong *,
                                          NcBuffer *);

int32 media_library_tags_from_strings(MediaLibraryTagArray *,
                                      StringViewList *);
int32 media_library_tags_from_songs(MediaLibraryTagArray *, NcmMpdSongList *,
                                    enum mpd_tag_type);
int32 media_library_albums_from_songs(MediaLibraryAlbumArray *,
                                      NcmMpdSongList *, enum MediaLibraryMode,
                                      enum mpd_tag_type, char *, int32);
int32 media_library_songs_from_list(NcmSongArray *, NcmMpdSongList *);

int32 media_library_screen_toggle_sort_mode(MediaLibraryScreen *, bool *);
int32 media_library_screen_set_primary_tag_type(MediaLibraryScreen *,
                                                enum mpd_tag_type);
void media_library_screen_request_database_update(MediaLibraryScreen *);
int32 media_library_screen_refresh_inactive_songs(MediaLibraryScreen *);

bool media_library_screen_can_move_to_previous_column(MediaLibraryScreen *);
bool media_library_screen_can_move_to_next_column(MediaLibraryScreen *);
void media_library_screen_previous_column(MediaLibraryScreen *);
void media_library_screen_next_column(MediaLibraryScreen *);
void media_library_screen_clear(MediaLibraryScreen *);
int32 media_library_screen_current_song(MediaLibraryScreen *, NcmSong *);
int32 media_library_screen_selected_songs(MediaLibraryScreen *, NcmSongArray *);
int32 media_library_screen_selected_songs_checked(MediaLibraryScreen *,
                                                  NcmSongArray *, NcmError *);
int32 media_library_screen_copy_visible_songs(MediaLibraryScreen *,
                                              NcmSongArray *, NcmError *);
int32 media_library_screen_apply_filter(MediaLibraryScreen *, char *, int32,
                                        NcmError *);
void media_library_screen_clear_filter(MediaLibraryScreen *);
int32 media_library_screen_search(MediaLibraryScreen *, char *, int32,
                                  bool forward, bool wrap, bool skip_current,
                                  NcmError *);
void media_library_screen_clear_search(MediaLibraryScreen *);
void media_library_screen_request_tags_update(MediaLibraryScreen *);
void media_library_screen_request_albums_update(MediaLibraryScreen *);
void media_library_screen_request_songs_update(MediaLibraryScreen *);
void media_library_screen_finish_list_change(MediaLibraryScreen *);
int32 media_library_screen_update(MediaLibraryScreen *, NcmError *);

int32 media_library_screen_list_tags(MediaLibraryScreen *, enum mpd_tag_type,
                                     StringViewList *, NcmError *);
int32 media_library_screen_list_all_songs(MediaLibraryScreen *,
                                          NcmMpdSongList *, NcmError *);
int32 media_library_screen_search_songs(MediaLibraryScreen *,
                                        MediaLibrarySongQuery *,
                                        NcmMpdSongList *, NcmError *);
int32 media_library_screen_add_songs(MediaLibraryScreen *, NcmSongArray *,
                                     bool, NcmError *);
int32 media_library_screen_add_item_to_playlist(MediaLibraryScreen *, bool,
                                                NcmError *);
int32 media_library_screen_locate_song(MediaLibraryScreen *, NcmSong *,
                                       NcmError *);

/* screens/nc_playlist_edit.h */
#define ENUM_NAME PlaylistEditColumn
#define ENUM_PREFIX_ PLAYLIST_EDITOR_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(PLAYLIST_EDITOR_COLUMN_PLAYLISTS, Playlists)                            \
    XX(PLAYLIST_EDITOR_COLUMN_CONTENT, Content)
#include "cbase/xenums.c"

#define PLAYLIST_EDITOR_FETCH_DELAY_MS 250

#define ENUM_NAME PlaylistEditCommandType
#define ENUM_PREFIX_ PLAYLIST_EDITOR_COMMAND_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                  \
    XX(PLAYLIST_EDITOR_COMMAND_NONE, none)           \
    XX(PLAYLIST_EDITOR_COMMAND_LOAD, Load)           \
    XX(PLAYLIST_EDITOR_COMMAND_SAVE, Save)           \
    XX(PLAYLIST_EDITOR_COMMAND_RENAME, Rename)       \
    XX(PLAYLIST_EDITOR_COMMAND_DELETE, Delete)
#include "cbase/xenums.c"

typedef struct PlaylistEditCommand {
    enum PlaylistEditCommandType type;
    char *playlist;
    char *target;
    int32 playlist_len;
    int32 playlist_cap;
    int32 target_len;
    int32 target_cap;
} PlaylistEditCommand;

typedef struct PlaylistEditScreen {
    NcScreen screen;
    NcPlaylistEntryMenu playlists;
    NcSongMenu content;
    NcWindow playlists_window;
    NcWindow content_window;
    StrBuilder playlist_filter_constraint;
    StrBuilder content_filter_constraint;
    StrBuilder playlist_search_constraint;
    StrBuilder content_search_constraint;
    StrBuilder playlists_title;
    StrBuilder content_title;
    StrBuilder displayed_playlist_path;
    StrBuilder observed_playlist_path;

    NcmRegex playlist_filter_regex;
    NcmRegex content_filter_regex;
    NcmRegex playlist_search_regex;
    NcmRegex content_search_regex;

    int64 timer;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 left_width;
    int32 right_start_x;
    int32 right_width;
    int32 column_ratio_left;
    int32 column_ratio_right;
    int32 fetching_delay_ms;
    int32 last_playlist_highlight;
    int32 last_known_content_count;
    int32 window_timeout_ms;
    int32 active_column;

    bool playlists_update_requested;
    bool content_update_requested;
    bool playlist_filter_enabled;
    bool content_filter_enabled;
    bool playlist_search_enabled;
    bool content_search_enabled;
    bool displayed_playlist_valid;
    bool observed_playlist_valid;
    bool registered;
} PlaylistEditScreen;

void playlist_edit_screen_init(PlaylistEditScreen *, int32 start_x,
                               int32 width, int32 main_start_y,
                               int32 main_height, NcColor, NcBorder);
void playlist_edit_screen_destroy(PlaylistEditScreen *);
NcScreen *playlist_edit_screen_base(PlaylistEditScreen *);

NcPlaylistEntryMenu *playlist_edit_screen_playlists(PlaylistEditScreen *);
NcSongMenu *playlist_edit_screen_content(PlaylistEditScreen *);
NcMenu *playlist_edit_screen_active_menu(PlaylistEditScreen *);
NcWindow *playlist_edit_screen_active_window(PlaylistEditScreen *);

void playlist_edit_screen_set_geometry(PlaylistEditScreen *, int32 start_x,
                                       int32 width, int32 main_start_y,
                                       int32 main_height);
void playlist_edit_screen_set_column_ratio(PlaylistEditScreen *, int32 left,
                                           int32 right);
bool playlist_edit_screen_can_move_to_previous_column(PlaylistEditScreen *);
bool playlist_edit_screen_can_move_to_next_column(PlaylistEditScreen *);
void playlist_edit_screen_previous_column(PlaylistEditScreen *);
void playlist_edit_screen_next_column(PlaylistEditScreen *);
int32 playlist_edit_screen_load_playlists(PlaylistEditScreen *,
                                          NcmMpdPlaylistList *);
int32 playlist_edit_screen_reload_playlists_from_mpd(PlaylistEditScreen *,
                                                     MpdClient *,
                                                     NcmError *);
int32 playlist_edit_screen_load_content(PlaylistEditScreen *, NcmMpdSongList *);
int32 playlist_edit_screen_reload_content_from_mpd(PlaylistEditScreen *,
                                                   MpdClient *, NcmError *);
int32 playlist_edit_screen_locate_playlist(PlaylistEditScreen *,
                                           MpdClient *, char *, int32,
                                           NcmError *);
int32 playlist_edit_screen_locate_song(PlaylistEditScreen *, MpdClient *,
                                       NcmSong *, NcmError *);
int32 playlist_edit_screen_current_playlist(PlaylistEditScreen *,
                                            NcmPlaylist *);
int32 playlist_edit_screen_current_song(PlaylistEditScreen *, NcmSong *);
int32 playlist_edit_screen_current_content_song(PlaylistEditScreen *,
                                                NcmSong *);
int32 playlist_edit_screen_selected_playlist_count(PlaylistEditScreen *);
int32 playlist_edit_screen_selected_songs(PlaylistEditScreen *, NcmSongArray *);
int32 playlist_edit_screen_apply_active_filter(PlaylistEditScreen *, char *,
                                               int32, uint32, NcmError *);
int32 playlist_edit_screen_search_active(PlaylistEditScreen *, char *, int32,
                                         uint32, bool forward, bool wrap,
                                         bool skip_current, NcmError *);
void playlist_edit_screen_request_playlists_update(PlaylistEditScreen *);
void playlist_edit_screen_request_content_update(PlaylistEditScreen *);

/* screens/nc_playlist.h */
typedef struct NcPlaylistScreen {
    NcScreen screen;
    NcMenu *menu;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 lines_scrolled;

    bool mouse_list_scroll_whole_page;
} NcPlaylistScreen;

typedef struct PlaylistScreen {
    NcPlaylistScreen screen;
    NcSongMenu songs;
    NcWindow window;
    StrBuilder title_cache;
    StrBuilder column_title;
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    NcmRegex filter_regex;

    uint64 total_length;
    uint64 remaining_time;
    int32 scroll_begin;
    int64 highlight_timer;

    bool reload_total_length;
    bool reload_remaining;
    bool registered;
    bool highlighting_requested;
} PlaylistScreen;

void nc_playlist_screen_init(NcPlaylistScreen *, NcScreenOps, void *, NcMenu *,
                             int32 start_x, int32 width, int32 main_start_y,
                             int32 main_height);
void nc_playlist_screen_set_geometry(NcPlaylistScreen *, int32 start_x,
                                     int32 width, int32 main_start_y,
                                     int32 main_height);
void nc_playlist_screen_set_menu(NcPlaylistScreen *, NcMenu *);
void nc_playlist_screen_set_mouse_config(NcPlaylistScreen *, int32, bool);
NcScreen *nc_playlist_screen_base(NcPlaylistScreen *);
NcMenu *nc_playlist_screen_menu(NcPlaylistScreen *);
int32 nc_playlist_screen_height(NcPlaylistScreen *);
void nc_playlist_screen_scroll(NcPlaylistScreen *, enum NcScroll);
int32 nc_playlist_screen_goto_y(NcPlaylistScreen *, int32);
int32 nc_playlist_screen_activate_current(NcPlaylistScreen *);
void nc_playlist_screen_mouse_button_pressed(NcPlaylistScreen *, MEVENT);

void playlist_screen_init(PlaylistScreen *, int32 start_x, int32 width,
                          int32 main_start_y, int32 main_height, NcColor,
                          NcBorder);
void playlist_screen_destroy(PlaylistScreen *);
int32 playlist_screen_unregister(PlaylistScreen *);
NcScreen *playlist_screen_base(PlaylistScreen *);
NcPlaylistScreen *playlist_screen_playlist(PlaylistScreen *);
NcSongMenu *playlist_screen_song_menu(PlaylistScreen *);
NcMenu *playlist_screen_menu(PlaylistScreen *);
NcWindow *playlist_screen_window(PlaylistScreen *);
void playlist_screen_update_column_title(PlaylistScreen *);
void playlist_screen_set_geometry(PlaylistScreen *, int32 start_x, int32 width,
                                  int32 main_start_y, int32 main_height);
void playlist_screen_set_mouse_config(PlaylistScreen *, int32, bool);
void playlist_screen_set_highlighting(PlaylistScreen *, bool);
bool playlist_screen_is_highlighting(PlaylistScreen *);
void playlist_screen_request_highlighting(PlaylistScreen *);
void playlist_screen_clear(PlaylistScreen *);
int32 playlist_screen_reload_from_mpd(PlaylistScreen *, MpdClient *,
                                      int32 version, int32 playlist_length,
                                      NcmError *);
int32 playlist_screen_song_count(PlaylistScreen *);
bool playlist_screen_is_empty(PlaylistScreen *);
int32 playlist_screen_current_song(PlaylistScreen *, NcmSong *);
int32 playlist_screen_update_current_mutable_song(PlaylistScreen *,
                                                  NcmMutableSong *);
int32 playlist_screen_now_playing_song(PlaylistScreen *, int32, NcmSong *);
int32 playlist_screen_locate_position(PlaylistScreen *, int32);
int32 playlist_screen_selected_songs(PlaylistScreen *, NcmSongArray *);
bool playlist_screen_has_sortable_range(PlaylistScreen *);
int32 playlist_screen_copy_sort_range(PlaylistScreen *, NcmSongArray *,
                                      int32 *, NcmError *);
int32 playlist_screen_apply_filter(PlaylistScreen *, char *, int32, NcmError *);
void playlist_screen_clear_filter(PlaylistScreen *);
int32 playlist_screen_search(PlaylistScreen *, char *, int32, bool forward,
                             bool wrap, bool skip_current, NcmError *);
int32 playlist_screen_set_selected_priority(PlaylistScreen *, MpdClient *,
                                            int32, NcmError *);
void playlist_screen_reload_total_length(PlaylistScreen *);
void playlist_screen_reload_remaining(PlaylistScreen *);

/* screens/nc_search_engine.h */
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
#define ENUM_FIELDS                                                            \
    XX(SEARCH_ENGINE_SEARCH_MODE_LITERAL,                                      \
       Match if tag contains searched phrase (no regexes))                    \
    XX(SEARCH_ENGINE_SEARCH_MODE_REGEX,                                        \
       Match if tag contains searched phrase (regexes supported))              \
    XX(SEARCH_ENGINE_SEARCH_MODE_EXACT,                                        \
       Match only if both values are the same)
#include "cbase/xenums.c"

#define ENUM_NAME SearchEnginePromptResult
#define ENUM_PREFIX_ SEARCH_ENGINE_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(SEARCH_ENGINE_PROMPT_ERROR)                                             \
    XX(SEARCH_ENGINE_PROMPT_ABORTED)                                           \
    XX(SEARCH_ENGINE_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct SearchEngineHooks {
    MpdClient *client;
    int32 (*list_database_songs)(void *, NcmSongArray *, NcmError *);
    int32 (*snapshot_playlist)(void *, NcmSongArray *, NcmError *);
    enum SearchEnginePromptResult (*prompt_constraint)(void *, char *, int32,
                                                       StrBuilder *initial,
                                                       StrBuilder *result);
    void (*status_message)(void *, char *, int32);
    int32 (*add_song)(void *, NcmSong *, bool, NcmError *);
    int32 (*format_song)(void *, NcmSong *, StrBuilder *);
    void *user;
} SearchEngineHooks;

typedef struct SearchEngineScreen {
    NcScreen screen;
    NcSearchRowMenu rows;
    NcWindow window;
    SearchEngineHooks hooks;
    StrBuilder constraints[SEARCH_ENGINE_CONSTRAINT_COUNT];
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

void search_engine_screen_init(SearchEngineScreen *, int32 start_x,
                               int32 width, int32 main_start_y,
                               int32 main_height, NcColor, NcBorder);
void search_engine_screen_destroy(SearchEngineScreen *);
NcScreen *search_engine_screen_base(SearchEngineScreen *);
NcMenu *search_engine_screen_menu(SearchEngineScreen *);
NcWindow *search_engine_screen_window(SearchEngineScreen *);
void search_engine_screen_set_mouse_config(SearchEngineScreen *, int32, bool);
bool search_engine_screen_has_locked_constraints(SearchEngineScreen *);
int32 search_engine_screen_format_song_text(SearchEngineScreen *, NcmSong *,
                                            StrBuilder *);
void search_engine_screen_update_column_title(SearchEngineScreen *);
void search_engine_screen_prepare_static_rows(SearchEngineScreen *);
int32 search_engine_screen_update_search_source_row(SearchEngineScreen *);
void search_engine_screen_reset(SearchEngineScreen *);
int32 search_engine_screen_add_buffer_with_flags(SearchEngineScreen *,
                                                 NcBuffer *, uint32);
int32 search_engine_screen_set_search_mode(SearchEngineScreen *,
                                           enum SearchEngineSearchMode);
void search_engine_screen_set_search_source(SearchEngineScreen *, bool);
void search_engine_screen_set_hooks(SearchEngineScreen *, SearchEngineHooks);
void search_engine_screen_status_message(SearchEngineScreen *, char *, int32);
bool search_engine_screen_can_run_current(SearchEngineScreen *);
int32 search_engine_screen_start_searching(SearchEngineScreen *,
                                           MpdClient *, NcmError *);
enum DisplayMode search_engine_screen_toggle_display_mode(SearchEngineScreen *);
bool search_engine_screen_can_search(SearchEngineScreen *);
int32 search_engine_screen_current_song(SearchEngineScreen *, NcmSong *);
int32 search_engine_screen_selected_songs(SearchEngineScreen *, NcmSongArray *);
int32 search_engine_screen_apply_filter(SearchEngineScreen *, char *, int32,
                                        NcmError *);
void search_engine_screen_clear_filter(SearchEngineScreen *);
int32 search_engine_screen_search(SearchEngineScreen *, char *, int32,
                                  bool forward, bool wrap, bool skip_current,
                                  NcmError *);

/* screens/nc_sel_items_adder.h */
typedef struct PlaylistScreen PlaylistScreen;

#define ENUM_NAME SelectedItemsAdderMenu
#define ENUM_PREFIX_ SELECTED_ITEMS_ADDER_MENU_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(SELECTED_ITEMS_ADDER_MENU_PLAYLISTS, Playlists)                         \
    XX(SELECTED_ITEMS_ADDER_MENU_POSITIONS, Positions)
#include "cbase/xenums.c"

typedef struct SelectedItemsAdderScreen {
    NcScreen screen;
    NcEditorActionMenu playlist_selector;
    NcEditorActionMenu position_selector;
    NcWindow playlist_window;
    NcWindow position_window;
    NcmSongArray selected_songs;
    NcmRegex search_regex;
    StrBuilder search_constraint;
    PlaylistScreen *playlist;
    NcScreen *previous_screen;
    MpdClient *client;

    int32 playlist_width;
    int32 playlist_height;
    int32 position_width;
    int32 position_height;
    int32 active_menu;

    bool local_browser;
    bool search_enabled;
    bool registered;
    bool ready;
} SelectedItemsAdderScreen;

void selected_items_adder_screen_init(SelectedItemsAdderScreen *,
                                      int32 start_x, int32 start_y,
                                      int32 width, int32 height, NcColor,
                                      NcBorder);
void selected_items_adder_screen_destroy(SelectedItemsAdderScreen *);
NcScreen *selected_items_adder_screen_base(SelectedItemsAdderScreen *);
NcMenu *selected_items_adder_screen_active_menu(SelectedItemsAdderScreen *);
NcWindow *selected_items_adder_screen_active_window(SelectedItemsAdderScreen *);
int32 selected_items_adder_screen_open(SelectedItemsAdderScreen *,
                                       NcmSongArray *, PlaylistScreen *,
                                       MpdClient *, NcmError *);
int32 selected_items_adder_screen_run_current(SelectedItemsAdderScreen *);
int32 selected_items_adder_screen_return_to_previous(
    SelectedItemsAdderScreen *);
int32 selected_items_adder_screen_search(SelectedItemsAdderScreen *, char *,
                                         int32, uint32, bool forward,
                                         bool wrap, bool skip_current,
                                         NcmError *);

/* screens/nc_sort_playlist.h */
typedef struct MpdClient MpdClient;
typedef struct PlaylistScreen PlaylistScreen;

typedef struct SortPlaylistDialog {
    NcScreen screen;
    NcEditorSortMenu rows;
    NcWindow window;
    NcmSongArray songs;

    PlaylistScreen *playlist;
    NcScreen *previous_screen;
    MpdClient *client;

    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;
    int32 start_position;

    bool ignore_leading_the;
    bool ready;
} SortPlaylistDialog;

void sort_playlist_dialog_init(SortPlaylistDialog *, int32 start_x,
                               int32 start_y, int32 width, int32 height,
                               NcColor, NcBorder);
void sort_playlist_dialog_destroy(SortPlaylistDialog *);
NcScreen *sort_playlist_dialog_base(SortPlaylistDialog *);
NcEditorSortMenu *sort_playlist_dialog_menu(SortPlaylistDialog *);
int32 sort_playlist_dialog_open(SortPlaylistDialog *, PlaylistScreen *,
                                MpdClient *, bool, NcmError *);
int32 sort_playlist_dialog_move_current_up(SortPlaylistDialog *);
int32 sort_playlist_dialog_move_current_down(SortPlaylistDialog *);

/* screens/nc_tag_edit.h */
#define ENUM_NAME TagEditColumn
#define ENUM_PREFIX_ TAG_EDIT_COLUMN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                  \
    XX(TAG_EDIT_COLUMN_DIRECTORIES, Directories)                   \
    XX(TAG_EDIT_COLUMN_TAG_TYPES, Tag types)                       \
    XX(TAG_EDIT_COLUMN_TAGS, Tags)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditParserMode
#define ENUM_PREFIX_ TAG_EDIT_PARSER_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                  \
    XX(TAG_EDIT_PARSER_NONE, Pattern)                              \
    XX(TAG_EDIT_PARSER_TAGS_FROM_FILENAME, Get tags from filename) \
    XX(TAG_EDIT_PARSER_RENAME_FILES, Rename files)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditFocus
#define ENUM_PREFIX_ TAG_EDIT_FOCUS_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                  \
    XX(TAG_EDIT_FOCUS_DIRECTORIES, Directories)                    \
    XX(TAG_EDIT_FOCUS_TAG_TYPES, Tag types)                        \
    XX(TAG_EDIT_FOCUS_TAGS, Tags)                                  \
    XX(TAG_EDIT_FOCUS_PARSER_CHOICE, Pattern)                      \
    XX(TAG_EDIT_FOCUS_PARSER_ACTIONS, Pattern actions)             \
    XX(TAG_EDIT_FOCUS_PARSER_LEGEND, Legend)                       \
    XX(TAG_EDIT_FOCUS_PARSER_PREVIEW, Preview)
#include "cbase/xenums.c"

#define ENUM_NAME TagEditPromptResult
#define ENUM_PREFIX_ TAG_EDIT_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                  \
    XX(TAG_EDIT_PROMPT_ERROR)                                      \
    XX(TAG_EDIT_PROMPT_ABORTED)                                    \
    XX(TAG_EDIT_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct TagEditHooks {
    enum TagEditPromptResult (*prompt)(void *, char *, int32, StringView,
                                         StrBuilder *);
    bool (*confirm)(void *, char *, int32);
    void (*status_message)(void *, char *, int32);
    void (*update_directory)(void *, char *, int32);
    void *user;
} TagEditHooks;

typedef struct TagEditScreen {
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
    TagEditHooks hooks;
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
    enum TagEditColumn active_column;
    int32 last_directory_highlight;
    int32 last_tag_type_highlight;
    int32 last_known_directory_count;
    int32 last_known_tag_count;
    int32 window_timeout_ms;

    enum TagEditParserMode parser_mode;
    enum TagEditFocus active_focus;
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
} TagEditScreen;

void tag_edit_screen_init(TagEditScreen *, int32 start_x, int32 width,
                          int32 main_start_y, int32 main_height, NcColor,
                          NcBorder);
void tag_edit_screen_destroy(TagEditScreen *);
NcScreen *tag_edit_screen_base(TagEditScreen *);

void tag_edit_screen_set_hooks(TagEditScreen *, TagEditHooks);
NcMenu *tag_edit_screen_active_menu(TagEditScreen *);
NcWindow *tag_edit_screen_active_window(TagEditScreen *);
void tag_edit_screen_set_geometry(TagEditScreen *, int32 start_x, int32 width,
                                  int32 main_start_y, int32 main_height);
void tag_edit_screen_clear_directories(TagEditScreen *);
void tag_edit_screen_clear_stale_tags(TagEditScreen *);
void tag_edit_screen_finish_directory_change(TagEditScreen *);
void tag_edit_screen_set_current_dir(TagEditScreen *, char *, int32);
int32 tag_edit_screen_current_dir(TagEditScreen *, StringView *);
int32 tag_edit_screen_current_directory_path(TagEditScreen *, StringView *);
int32 tag_edit_screen_enter_directory(TagEditScreen *);
int32 tag_edit_screen_go_to_parent(TagEditScreen *);
int32 tag_edit_screen_locate_song(TagEditScreen *, NcmSong *);
bool tag_edit_screen_rename_directory_available(TagEditScreen *, char *, int32);
int32 tag_edit_screen_rename_current_directory(TagEditScreen *, char *, int32);
void tag_edit_screen_add_directory(TagEditScreen *, char *label,
                                   int32 label_len, char *path, int32 path_len);
void tag_edit_screen_load_songs(TagEditScreen *, NcmSongArray *);
void tag_edit_screen_add_mutable_song(TagEditScreen *, NcmMutableSong *);
int32 tag_edit_screen_selected_songs(TagEditScreen *, NcmSongArray *);
bool tag_edit_screen_previous_column_available(TagEditScreen *);
bool tag_edit_screen_next_column_available(TagEditScreen *);
void tag_edit_screen_previous_column(TagEditScreen *);
void tag_edit_screen_next_column(TagEditScreen *);
int32 tag_edit_screen_apply_tag_to_selection(TagEditScreen *,
                                             enum NcmTagsField, char *value,
                                             int32 value_len, char *separator,
                                             int32 separator_len);
int32 tag_edit_screen_number_tracks(TagEditScreen *, bool);
void tag_edit_screen_capitalize_first_letters(TagEditScreen *);
void tag_edit_screen_lower_all_letters(TagEditScreen *);
void tag_edit_screen_clear_modifications(TagEditScreen *);
int32 tag_edit_screen_save_modified(TagEditScreen *, char *);
bool tag_edit_screen_save_action_available(TagEditScreen *);
int32 tag_edit_screen_apply_directory_filter(TagEditScreen *, char *, int32,
                                             uint32, NcmError *);
int32 tag_edit_screen_apply_tag_filter(TagEditScreen *, char *, int32, uint32,
                                       NcmError *);
int32 tag_edit_screen_search(TagEditScreen *, char *, int32, bool forward,
                             bool wrap, bool skip_current, NcmError *);
void tag_edit_screen_prepare_parser_rows(TagEditScreen *,
                                         enum TagEditParserMode, char *, int32);
void tag_edit_screen_show_parser_dialog(TagEditScreen *);
void tag_edit_screen_show_parser_actions(TagEditScreen *,
                                         enum TagEditParserMode);
void tag_edit_screen_show_parser_legend(TagEditScreen *);
void tag_edit_screen_show_parser_preview(TagEditScreen *);
void tag_edit_screen_close_parser(TagEditScreen *);
int32 tag_edit_parse_filename(NcmMutableSong *, char *, int32, bool,
                              StrBuilder *);
int32 tag_edit_generate_filename(NcmMutableSong *, char *, int32, StrBuilder *);
int32 tag_edit_song_display_value(NcmMutableSong *, enum NcmTagsField,
                                  StrBuilder *);

/* screens/nc_tiny_tag_edit.h */
#define TINY_TAG_EDIT_TAG_ROW(FIELD) \
    ((int32)TINY_TAG_EDIT_FIRST_TAG_ROW + (int32)(FIELD))

enum TinyTagEditRow {
    TINY_TAG_EDIT_FILE_NAME_INFO_ROW,
    TINY_TAG_EDIT_DIRECTORY_INFO_ROW,
    TINY_TAG_EDIT_UNUSED_INFO_ROW,
    TINY_TAG_EDIT_LENGTH_INFO_ROW,
    TINY_TAG_EDIT_BITRATE_INFO_ROW,
    TINY_TAG_EDIT_SAMPLE_RATE_INFO_ROW,
    TINY_TAG_EDIT_CHANNELS_INFO_ROW,
    TINY_TAG_EDIT_FIRST_SEPARATOR_ROW,
    TINY_TAG_EDIT_FIRST_TAG_ROW,
    TINY_TAG_EDIT_LAST_TAG_ROW = TINY_TAG_EDIT_FIRST_TAG_ROW
                                   + NCM_TAGS_FIELD_COUNT - 1,
    TINY_TAG_EDIT_SECOND_SEPARATOR_ROW,
    TINY_TAG_EDIT_FILE_NAME_EDIT_ROW,
    TINY_TAG_EDIT_THIRD_SEPARATOR_ROW,
    TINY_TAG_EDIT_SAVE_ROW,
    TINY_TAG_EDIT_CANCEL_ROW,
    TINY_TAG_EDIT_ROW_COUNT,
};

#define ENUM_NAME TinyTagEditOpenResult
#define ENUM_PREFIX_ TINY_TAG_EDIT_OPEN_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(TINY_TAG_EDIT_OPEN_SUCCESS)                                           \
    XX(TINY_TAG_EDIT_OPEN_INVALID_ARGUMENT)                                  \
    XX(TINY_TAG_EDIT_OPEN_STREAM)                                            \
    XX(TINY_TAG_EDIT_OPEN_MISSING_MUSIC_DIRECTORY)                           \
    XX(TINY_TAG_EDIT_OPEN_UNREADABLE_FILE)                                   \
    XX(TINY_TAG_EDIT_OPEN_PREPARE_FAILED)
#include "cbase/xenums.c"

#define ENUM_NAME TinyTagEditPromptResult
#define ENUM_PREFIX_ TINY_TAG_EDIT_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                                                            \
    XX(TINY_TAG_EDIT_PROMPT_ERROR)                                           \
    XX(TINY_TAG_EDIT_PROMPT_ABORTED)                                         \
    XX(TINY_TAG_EDIT_PROMPT_ACCEPTED)
#include "cbase/xenums.c"

typedef struct TinyTagEditHooks {
    enum TinyTagEditPromptResult (*prompt)(void *, char *, int32,
                                             StringView, StrBuilder *);
    void (*status_message)(void *, char *, int32);
    int32 (*taglib_open)(void *, NcmTaglibFile *, char *, int32);
    int32 (*taglib_audio_properties)(void *, NcmTaglibFile *,
                                     NcmTaglibAudioProperties *);
    bool (*taglib_file_can_set_extended_tags)(void *, NcmTaglibFile *);
    void (*taglib_close)(void *, NcmTaglibFile *);
    int32 (*write_song)(void *, NcmMutableSong *, char *);
    void (*update_directory)(void *, char *, int32);
    void (*update_playlist_song)(void *, NcmMutableSong *);
    void (*request_browser_update)(void *);
    void (*switch_to_screen)(void *, NcScreen *);
    void *user;
} TinyTagEditHooks;

typedef struct TinyTagEditScreen {
    NcScreen screen;
    NcEditorBufferMenu rows;
    NcWindow window;
    TinyTagEditHooks hooks;
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
} TinyTagEditScreen;

void tiny_tag_edit_screen_init(TinyTagEditScreen *, int32 start_x, int32 width,
                               int32 main_start_y, int32 main_height, NcColor,
                               NcBorder);
void tiny_tag_edit_screen_destroy(TinyTagEditScreen *);
NcScreen *tiny_tag_edit_screen_base(TinyTagEditScreen *);

void tiny_tag_edit_screen_set_hooks(TinyTagEditScreen *, TinyTagEditHooks);
NcEditorBufferMenu *tiny_tag_edit_screen_rows(TinyTagEditScreen *);
enum TinyTagEditOpenResult tiny_tag_edit_screen_open_song(
    TinyTagEditScreen *, NcmSong *, char *music_dir, int32 music_dir_len,
    char *tag_separator, int32 tag_separator_len, bool, StrBuilder *);
int32 tiny_tag_edit_screen_run_row(TinyTagEditScreen *, int32);

/* screens/nc_browser.h */
typedef struct BrowserScreen {
    NcScreen screen;
    NcBrowserEntryMenu entries;
    NcWindow window;
    StrBuilder current_directory;
    StrBuilder last_highlighted_directory;
    StrBuilder title_text;
    StrBuilder column_title_text;
    StrBuilder filter_constraint;
    StrBuilder search_constraint;
    StrBuilder item_text_buffer;
    StrBuilder path_buffer;
    StrBuilder scratch_buffer;
    StrBuilderArray supported_extensions;
    NcmRegex filter_regex;

    int32 start_x;
    int32 width;
    int32 main_start_y;
    int32 main_height;
    int32 lines_scrolled;
    int32 title_scroll_beginning;

    enum DisplayMode active_display_mode;

    bool mouse_list_scroll_whole_page;
    bool redraw_header;
    bool update_requested;
    bool local_browser;
    bool filter_enabled;
    bool registered;
} BrowserScreen;

void browser_screen_init(BrowserScreen *, int32 start_x, int32 width,
                         int32 main_start_y, int32 main_height, NcColor,
                         NcBorder);
void browser_screen_destroy(BrowserScreen *);
NcScreen *browser_screen_base(BrowserScreen *);
NcBrowserEntryMenu *browser_screen_entries(BrowserScreen *);
NcMenu *browser_screen_menu(BrowserScreen *);
NcWindow *browser_screen_window(BrowserScreen *);
void browser_screen_set_mouse_config(BrowserScreen *, int32, bool);
void browser_screen_clear(BrowserScreen *);
void browser_screen_add_item_move(BrowserScreen *, NcmMpdItem *);
int32 browser_screen_sort(BrowserScreen *);
int32 browser_screen_set_current_directory(BrowserScreen *, char *, int32);
StringView browser_screen_current_directory(BrowserScreen *);
void browser_screen_update_title_text(BrowserScreen *);
void browser_screen_update_column_title(BrowserScreen *);
void browser_screen_draw_header(BrowserScreen *);
void browser_screen_set_display_mode(BrowserScreen *, enum DisplayMode);
int32 browser_screen_fetch_supported_extensions(BrowserScreen *,
                                                MpdClient *, NcmError *);
void browser_screen_clear_update_request(BrowserScreen *);
bool browser_screen_is_in_root_directory(BrowserScreen *);
void browser_screen_set_local(BrowserScreen *, bool);
bool browser_screen_is_local(BrowserScreen *);
int32 browser_screen_change_browse_mode(BrowserScreen *, MpdClient *,
                                        NcmError *);
NcmMpdItem *browser_screen_current_item(BrowserScreen *);
int32 browser_screen_current_song(BrowserScreen *, NcmSong *);
int32 browser_screen_selected_songs(BrowserScreen *, NcmSongArray *);
int32 browser_screen_delete_items(BrowserScreen *, MpdClient *, NcmError *);
bool browser_screen_has_current_directory_path(BrowserScreen *,
                                               StringView *);
bool browser_screen_has_current_playlist_path(BrowserScreen *, StringView *);
bool browser_screen_can_rename_directory(BrowserScreen *);
bool browser_screen_can_rename_playlist(BrowserScreen *);
int32 browser_screen_rename_current_directory(BrowserScreen *, char *, int32,
                                              MpdClient *, NcmError *);
int32 browser_screen_rename_current_playlist(BrowserScreen *, char *, int32,
                                             MpdClient *, NcmError *);
int32 browser_screen_locate_song(BrowserScreen *, NcmSong *, MpdClient *,
                                 NcmError *);
int32 browser_screen_enter_directory(BrowserScreen *);
int32 browser_screen_go_to_parent(BrowserScreen *);
int32 browser_screen_apply_filter(BrowserScreen *, char *, int32, NcmError *);
void browser_screen_clear_filter(BrowserScreen *);
int32 browser_screen_search(BrowserScreen *, char *, int32, bool forward,
                            bool wrap, bool skip_current, NcmError *);
void browser_screen_request_update(BrowserScreen *);
bool browser_screen_item_is_parent(NcmMpdItem *);

/* screens/song_info.h */
typedef struct NcmSongInfoMetadata {
    char *name;
    enum SongGetter get;
    enum NcmTagsField field;
} NcmSongInfoMetadata;

extern NcmSongInfoMetadata ncm_song_info_tags[];

/* screens/app_screens.h */
typedef struct HelpScreen HelpScreen;
typedef struct OutputsScreen OutputsScreen;
typedef struct SearchEngineScreen SearchEngineScreen;
typedef struct ServerInfoScreen ServerInfoScreen;
typedef struct SongInfoScreen SongInfoScreen;

void app_screens_init_all(void);
void app_screens_register_initial(void);
void app_screens_request_registered_resize(void);
NcScreen *app_screens_find_type(enum ScreenType);
int32 app_screens_switch_to_type(enum ScreenType);
int32 app_screens_lock_current(void);
enum ScreenType app_screens_current_type(void);

#define NCM_APP_SCREEN_DECLARE_COMMON(suffix)                                  \
    void app_screen_##suffix##_init(void);                                     \
    void app_screen_##suffix##_register(void);                                 \
    bool app_screen_##suffix##_is_current(void);                               \
    NcScreen *app_screen_##suffix##_base(void);

NCM_APP_SCREEN_IS_CURRENT_TYPES(NCM_APP_SCREEN_DECLARE_COMMON)

#undef NCM_APP_SCREEN_DECLARE_COMMON

#define NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR(                                \
    suffix, type, storage, base_expr                                           \
)                                                                              \
    type *app_screen_##suffix(void);

NCM_APP_SCREEN_DIRECT_ACCESSOR_TYPES(NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR)

#undef NCM_APP_SCREEN_DECLARE_DIRECT_ACCESSOR

#define NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR(                         \
    suffix, function, type, expr                                               \
)                                                                              \
    type *function(void);

NCM_APP_SCREEN_TYPED_WRAPPED_ACCESSOR_TYPES(
    NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR)

#undef NCM_APP_SCREEN_DECLARE_TYPED_WRAPPED_ACCESSOR

#define NCM_APP_SCREEN_DECLARE_VOID_SWITCH(suffix)                             \
    void app_screen_##suffix##_switch_to(void);

NCM_APP_SCREEN_SIMPLE_SWITCH_TYPES(NCM_APP_SCREEN_DECLARE_VOID_SWITCH)
NCM_APP_SCREEN_REGISTER_SWITCH_TYPES(NCM_APP_SCREEN_DECLARE_VOID_SWITCH)

#undef NCM_APP_SCREEN_DECLARE_VOID_SWITCH

void app_screen_lastfm_switch_to(void);
VisualizerScreen *app_screen_visualizer(void);
void app_screen_lyrics_set_resize(void);
void app_screen_lyrics_switch_to(void);
void app_screen_browser_fetch_supported_extensions(void);
int32 app_screen_selected_items_adder_open(NcmSongArray *, NcmError *);
int32 app_screen_sort_playlist_dialog_switch_to(void);
void app_screen_outputs_toggle(void);
void app_screen_outputs_fetch_list(void);
void app_screen_outputs_refresh_if_visible(void);

#endif /* NC_SCREENS_H */
