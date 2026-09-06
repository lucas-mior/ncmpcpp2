#if !defined(NC_CURSES_H)
#define NC_CURSES_H

#define NCURSES_NOMACROS 1

#include "cbase.h"

#include <curses.h>

#define NCURSES_NOMACROS 1

#include <curses.h>

#if NCURSES_MOUSE_VERSION == 1
#define BUTTON5_PRESSED (1U << 27)
#endif

typedef uint64 NcKey;

#define NC_KEY_NONE      ((NcKey)-1)
#define NC_KEY_SPECIAL   (((NcKey)1) << 63)
#define NC_KEY_ALT       (((NcKey)1) << 62)
#define NC_KEY_CTRL      (((NcKey)1) << 61)
#define NC_KEY_SHIFT     (((NcKey)1) << 60)
#define NC_KEY_NULL      ((NcKey)0)
#define NC_KEY_SPACE     ((NcKey)32)
#define NC_KEY_BACKSPACE ((NcKey)127)
#define NC_KEY_CTRL_A    ((NcKey)1)
#define NC_KEY_CTRL_B    ((NcKey)2)
#define NC_KEY_CTRL_C    ((NcKey)3)
#define NC_KEY_CTRL_D    ((NcKey)4)
#define NC_KEY_CTRL_E    ((NcKey)5)
#define NC_KEY_CTRL_F    ((NcKey)6)
#define NC_KEY_CTRL_G    ((NcKey)7)
#define NC_KEY_CTRL_H    ((NcKey)8)
#define NC_KEY_CTRL_I    ((NcKey)9)
#define NC_KEY_CTRL_J    ((NcKey)10)
#define NC_KEY_CTRL_K    ((NcKey)11)
#define NC_KEY_CTRL_L    ((NcKey)12)
#define NC_KEY_CTRL_M    ((NcKey)13)
#define NC_KEY_CTRL_N    ((NcKey)14)
#define NC_KEY_CTRL_O    ((NcKey)15)
#define NC_KEY_CTRL_P    ((NcKey)16)
#define NC_KEY_CTRL_Q    ((NcKey)17)
#define NC_KEY_CTRL_R    ((NcKey)18)
#define NC_KEY_CTRL_S    ((NcKey)19)
#define NC_KEY_CTRL_T    ((NcKey)20)
#define NC_KEY_CTRL_U    ((NcKey)21)
#define NC_KEY_CTRL_V    ((NcKey)22)
#define NC_KEY_CTRL_W    ((NcKey)23)
#define NC_KEY_CTRL_X    ((NcKey)24)
#define NC_KEY_CTRL_Y    ((NcKey)25)
#define NC_KEY_CTRL_Z    ((NcKey)26)
#define NC_KEY_CTRL_LEFT_BRACKET  ((NcKey)27)
#define NC_KEY_CTRL_BACKSLASH     ((NcKey)28)
#define NC_KEY_CTRL_RIGHT_BRACKET ((NcKey)29)
#define NC_KEY_CTRL_CARET         ((NcKey)30)
#define NC_KEY_CTRL_UNDERSCORE    ((NcKey)31)
#define NC_KEY_TAB       ((NcKey)9)
#define NC_KEY_ENTER     ((NcKey)13)
#define NC_KEY_ESCAPE    ((NcKey)27)
#define NC_KEY_INSERT    (NC_KEY_SPECIAL | 256)
#define NC_KEY_DELETE    (NC_KEY_SPECIAL | 257)
#define NC_KEY_HOME      (NC_KEY_SPECIAL | 258)
#define NC_KEY_END       (NC_KEY_SPECIAL | 259)
#define NC_KEY_PAGE_UP   (NC_KEY_SPECIAL | 260)
#define NC_KEY_PAGE_DOWN (NC_KEY_SPECIAL | 261)
#define NC_KEY_UP        (NC_KEY_SPECIAL | 262)
#define NC_KEY_DOWN      (NC_KEY_SPECIAL | 263)
#define NC_KEY_LEFT      (NC_KEY_SPECIAL | 264)
#define NC_KEY_RIGHT     (NC_KEY_SPECIAL | 265)
#define NC_KEY_F1        (NC_KEY_SPECIAL | 266)
#define NC_KEY_F2        (NC_KEY_SPECIAL | 267)
#define NC_KEY_F3        (NC_KEY_SPECIAL | 268)
#define NC_KEY_F4        (NC_KEY_SPECIAL | 269)
#define NC_KEY_F5        (NC_KEY_SPECIAL | 270)
#define NC_KEY_F6        (NC_KEY_SPECIAL | 271)
#define NC_KEY_F7        (NC_KEY_SPECIAL | 272)
#define NC_KEY_F8        (NC_KEY_SPECIAL | 273)
#define NC_KEY_F9        (NC_KEY_SPECIAL | 274)
#define NC_KEY_F10       (NC_KEY_SPECIAL | 275)
#define NC_KEY_F11       (NC_KEY_SPECIAL | 276)
#define NC_KEY_F12       (NC_KEY_SPECIAL | 277)
#define NC_KEY_MOUSE     (NC_KEY_SPECIAL | 278)
#define NC_KEY_EOF       (NC_KEY_SPECIAL | 279)

#define NC_COLOR_TRANSPARENT ((int16)-1)
#define NC_COLOR_CURRENT     ((int16)-2)

typedef struct NcColor {
    int16 foreground;
    int16 background;
    bool is_default;
    bool is_end;
} NcColor;

typedef struct NcBorder {
    NcColor color;
    bool enabled;
} NcBorder;

typedef struct NcFdCallback {
    void (*callback)(void);
    int32 fd;
} NcFdCallback;

#define ENUM_NAME NcTermManip
#define ENUM_PREFIX_ NC_TERM_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS  \
    XX(NC_TERM_CLEAR_TO_EOL)
#include "cbase/xenums.c"

#define ENUM_NAME NcFormat
#define ENUM_PREFIX_ NC_FORMAT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                              \
    XX(NC_FORMAT_BOLD, bold)                     \
    XX(NC_FORMAT_NO_BOLD, no_bold)               \
    XX(NC_FORMAT_UNDERLINE, underline)           \
    XX(NC_FORMAT_NO_UNDERLINE, no_underline)     \
    XX(NC_FORMAT_REVERSE, reverse)               \
    XX(NC_FORMAT_NO_REVERSE, no_reverse)         \
    XX(NC_FORMAT_ALT_CHARSET, alt_charset)       \
    XX(NC_FORMAT_NO_ALT_CHARSET, no_alt_charset) \
    XX(NC_FORMAT_ITALIC, italic)                 \
    XX(NC_FORMAT_NO_ITALIC, no_italic)
#include "cbase/xenums.c"

#define ENUM_NAME NcScroll
#define ENUM_PREFIX_ NC_SCROLL_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                  \
    XX(NC_SCROLL_UP)                 \
    XX(NC_SCROLL_DOWN)               \
    XX(NC_SCROLL_PAGE_UP)            \
    XX(NC_SCROLL_PAGE_DOWN)          \
    XX(NC_SCROLL_HOME)               \
    XX(NC_SCROLL_END)
#include "cbase/xenums.c"

#define ENUM_NAME NcPromptStatus
#define ENUM_PREFIX_ NC_PROMPT_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                  \
    XX(NC_PROMPT_ACCEPTED)           \
    XX(NC_PROMPT_ABORTED)
#include "cbase/xenums.c"

typedef bool NcPromptShouldContinueFunc(char *text, void *user_data);

typedef struct NcPrompt {
    char *initial_text;
    int32 width;
    NcPromptShouldContinueFunc *should_continue;
    void *should_continue_user_data;
    bool encrypted;
    bool remember;
} NcPrompt;

typedef struct NcWindow {
    WINDOW *window;
    char *title;
    NcColor *color_stack;
    NcKey *input_queue;
    NcFdCallback *fd_callbacks;

    int32 start_x;
    int32 start_y;
    int32 width;
    int32 height;
    int32 title_len;
    int32 title_cap;
    int32 window_timeout;
    int32 input_queue_start;

    NcColor color;
    NcColor base_color;
    NcBorder border;
    MEVENT mouse_event;

    bool escape_terminal_sequences;

    int32 bold_counter;
    int32 underline_counter;
    int32 reverse_counter;
    int32 alt_charset_counter;
    int32 italic_counter;
} NcWindow;

NcColor nc_color_make(int16 foreground, int16 background,
                      bool is_default, bool is_end);
NcColor nc_color_default(void);
NcColor nc_color_end(void);
bool nc_color_is_equal(NcColor left, NcColor right);
bool nc_color_is_default(NcColor color);
bool nc_color_is_end(NcColor color);
bool nc_color_has_current_background(NcColor color);
int32 nc_color_pair_number(NcColor color);

NcBorder nc_border_none(void);
NcBorder nc_border_make(NcColor color);
enum NcFormat nc_format_reverse(enum NcFormat format);
int32 nc_key_name(NcKey key, char *buffer, int32 buffer_len);

void nc_mouse_enable(void);
void nc_mouse_disable(void);
void nc_init_readline(void);
void nc_resize_readline_terminal(void);
void nc_init_screen(bool enable_colors, bool enable_mouse);
int32 nc_color_count(void);
void nc_pause_screen(void);
void nc_unpause_screen(void);
void nc_destroy_screen(void);

void nc_window_init_empty(NcWindow *window);
void nc_window_init(NcWindow *window, int32 start_x, int32 start_y,
                    int32 width, int32 height, char *title,
                    int32 title_len, NcColor color, NcBorder border);
void nc_window_destroy(NcWindow *window);

WINDOW *nc_window_raw(NcWindow *window);
int32 nc_window_width(NcWindow *window);
int32 nc_window_height(NcWindow *window);
int32 nc_window_start_x(NcWindow *window);
int32 nc_window_start_y(NcWindow *window);
MEVENT *nc_window_mouse_event(NcWindow *window);

void nc_window_set_color(NcWindow *window, NcColor color);
void nc_window_set_base_color(NcWindow *window, NcColor color);
void nc_window_set_border(NcWindow *window, NcBorder border);
void nc_window_set_timeout(NcWindow *window, int32 timeout);
void nc_window_set_title(NcWindow *window, char *title, int32 title_len);
void nc_window_set_escape_terminal_sequences(NcWindow *window, bool enabled);

void nc_window_display(NcWindow *window);
void nc_window_refresh_border(NcWindow *window);
void nc_window_refresh(NcWindow *window);
void nc_window_move_to(NcWindow *window, int32 new_x, int32 new_y);
void nc_window_adjust_dimensions(NcWindow *window,
                                 int32 width, int32 height);
void nc_window_resize(NcWindow *window, int32 new_width, int32 new_height);
void nc_window_recreate(NcWindow *window, int32 width, int32 height);
void nc_window_clear(NcWindow *window);

void nc_window_add_fd_callback(NcWindow *window,
                               int32 fd, void (*callback)(void));
void nc_window_clear_fd_callbacks(NcWindow *window);
bool nc_window_fd_callbacks_is_empty(NcWindow *window);
NcKey nc_window_read_key(NcWindow *window);
void nc_window_push_key(NcWindow *window, NcKey ch);
enum NcPromptStatus nc_window_prompt(NcWindow *window, NcPrompt *prompt,
                                     char **result);
void nc_window_prompt_result_destroy(char *result);

void nc_window_scroll(NcWindow *window, enum NcScroll where);
void nc_window_apply_term_manip(NcWindow *window, enum NcTermManip tm);
void nc_window_apply_format(NcWindow *window, enum NcFormat format);
void nc_window_push_color(NcWindow *window, NcColor color);
void nc_window_go_to_xy(NcWindow *window, int32 x, int32 y);
int32 nc_window_get_x(NcWindow *window);
int32 nc_window_get_y(NcWindow *window);
bool nc_window_has_coords(NcWindow *window, int32 *x, int32 *y);

void nc_window_print_cstring(NcWindow *window, char *string);
void nc_window_print_data(NcWindow *window, char *string, int32 string_len);
void nc_window_print_char(NcWindow *window, char ch);

typedef struct NcFormattedColor {
    enum NcFormat *formats;
    NcColor color;
} NcFormattedColor;

void nc_formatted_color_init(NcFormattedColor *formatted_color);
void nc_formatted_color_init_color(NcFormattedColor *formatted_color,
                                   NcColor color);
void nc_formatted_color_copy(NcFormattedColor *dest,
                             NcFormattedColor *source);
void nc_formatted_color_move(NcFormattedColor *dest,
                             NcFormattedColor *source);
void nc_formatted_color_destroy(NcFormattedColor *formatted_color);
void nc_formatted_color_add_format(NcFormattedColor *formatted_color,
                                   enum NcFormat format);
enum NcFormat *nc_formatted_color_formats(NcFormattedColor *formatted_color);
int32 nc_formatted_color_format_count(NcFormattedColor *formatted_color);

#define ENUM_NAME NcBufferPropertyType
#define ENUM_PREFIX_ NC_BUFFER_PROPERTY_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                             \
    XX(NC_BUFFER_PROPERTY_COLOR)                \
    XX(NC_BUFFER_PROPERTY_FORMAT)               \
    XX(NC_BUFFER_PROPERTY_FORMATTED_COLOR)      \
    XX(NC_BUFFER_PROPERTY_FORMATTED_COLOR_END)
#include "cbase/xenums.c"

typedef struct NcBufferProperty {
    union {
        NcColor color;
        enum NcFormat format;
        NcFormattedColor formatted_color;
    } value;

    int64 id;
    int32 position;
    enum NcBufferPropertyType type;
} NcBufferProperty;

typedef struct NcBuffer {
    char *data;
    NcBufferProperty *properties;

    int32 len;
    int32 cap;
} NcBuffer;

void nc_buffer_copy(NcBuffer *dest, NcBuffer *source);
void nc_buffer_move(NcBuffer *dest, NcBuffer *source);
void nc_buffer_destroy(NcBuffer *buffer);
void nc_buffer_clear(NcBuffer *buffer);
bool nc_buffer_is_empty(NcBuffer *buffer);

char *nc_buffer_data(NcBuffer *buffer);
NcBufferProperty *nc_buffer_properties(NcBuffer *buffer);

void nc_buffer_append_data(NcBuffer *buffer, char *data, int32 data_len);
void nc_buffer_append_cstring(NcBuffer *buffer, char *string);
void nc_buffer_append_char(NcBuffer *buffer, char ch);
void nc_buffer_append_int64(NcBuffer *buffer, int64 value);

void nc_buffer_add_color(NcBuffer *buffer, int32 position, NcColor color,
                         int64 id);
void nc_buffer_add_format(NcBuffer *buffer, int32 position,
                          enum NcFormat format, int64 id);
void nc_buffer_add_formatted_color(NcBuffer *buffer, int32 position,
                                   NcFormattedColor *formatted_color,
                                   int64 id);
void nc_buffer_add_formatted_color_end(NcBuffer *buffer, int32 position,
                                       NcFormattedColor *formatted_color,
                                       int64 id);
void nc_buffer_remove_properties(NcBuffer *buffer, int64 id);
void nc_buffer_apply_property(NcWindow *window, NcBufferProperty *property);

typedef struct NcMenu NcMenu;

typedef bool NcMenuPositionIsHighlightableFunc(int32 pos, void *user);
typedef bool NcMenuPositionMatchesFunc(NcMenu *menu, int32 pos, void *user);

#define ENUM_NAME NcMenuItemSource
#define ENUM_PREFIX_ NC_MENU_ITEMS_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS                       \
    XX(NC_MENU_ITEMS_ALL)                 \
    XX(NC_MENU_ITEMS_FILTERED)
#include "cbase/xenums.c"

#define ENUM_NAME NcMenuItemFlag
#define ENUM_PREFIX_ NC_MENU_ITEM_
#define ENUM_BITFLAGS 1
#define ENUM_FIELDS                       \
    XX(NC_MENU_ITEM_SELECTABLE)           \
    XX(NC_MENU_ITEM_SELECTED)             \
    XX(NC_MENU_ITEM_INACTIVE)             \
    XX(NC_MENU_ITEM_SEPARATOR)
#include "cbase/xenums.c"

typedef struct NcMenuItemCallbacks {
    int32 item_size;
    void (*construct)(void *dest, void *user);
    void (*copy)(void *dest, void *source, void *user);
    void (*destroy)(void *item, void *user);
    void *user;
} NcMenuItemCallbacks;

typedef struct NcMenuDisplayCallbacks {
    void (*draw)(NcMenu *menu, NcWindow *window,
                 void *item, int32 pos, void *user);
    bool (*matches_filter)(NcMenu *menu, void *item, void *user);
    bool (*is_separator)(void *item, void *user);
    bool (*is_selected)(void *item, void *user);
    bool (*is_inactive)(void *item, void *user);
    void *user;
} NcMenuDisplayCallbacks;

typedef struct NcMenuActionCallbacks {
    void (*activate)(NcMenu *menu, void *item, int32 pos, void *user);
    void (*set_selected)(void *item, bool selected, void *user);
    void *user;
} NcMenuActionCallbacks;

struct NcMenu {
    void **all_items;
    void **filtered_items;
    uint32 *all_item_flags;
    uint32 *filtered_item_flags;
    enum NcMenuItemSource active_items;
    NcMenuItemCallbacks item_callbacks;
    NcMenuDisplayCallbacks display_callbacks;
    NcMenuActionCallbacks action_callbacks;

    NcBuffer highlight_prefix;
    NcBuffer highlight_suffix;
    NcBuffer selected_prefix;
    NcBuffer selected_suffix;

    int32 item_count;
    int32 beginning;
    int32 highlight;
    int32 drawn_position;

    bool highlight_disabled;
    bool cyclic_scroll_enabled;
    bool autocenter_cursor;
};

void nc_menu_destroy(NcMenu *menu);
void nc_menu_copy(NcMenu *dest, NcMenu *source);
void nc_menu_swap(NcMenu *left, NcMenu *right);
void nc_menu_set_item_callbacks(NcMenu *menu, NcMenuItemCallbacks callbacks);
void nc_menu_set_display_callbacks(NcMenu *menu,
                                   NcMenuDisplayCallbacks callbacks);
void nc_menu_set_action_callbacks(NcMenu *menu,
                                  NcMenuActionCallbacks callbacks);
void nc_menu_sync_item_count(NcMenu *menu);
int32 nc_menu_item_count(NcMenu *menu);
int32 nc_menu_all_item_count(NcMenu *menu);
int32 nc_menu_filtered_item_count(NcMenu *menu);
int32 nc_menu_highlight(NcMenu *menu);
bool nc_menu_highlight_is_enabled(NcMenu *menu);
void nc_menu_set_highlight_prefix(NcMenu *menu, NcBuffer *buffer);
void nc_menu_set_highlight_suffix(NcMenu *menu, NcBuffer *buffer);
void nc_menu_set_selected_prefix(NcMenu *menu, NcBuffer *buffer);
void nc_menu_set_selected_suffix(NcMenu *menu, NcBuffer *buffer);
void nc_menu_set_highlighting(NcMenu *menu, bool state);
void nc_menu_set_cyclic_scrolling(NcMenu *menu, bool state);
void nc_menu_set_centered_cursor(NcMenu *menu, bool state);
int32 nc_menu_goto_selectable(NcMenu *menu, int32 y);
int32 nc_menu_search_selectable(NcMenu *menu, int32 height, bool forward,
                                bool wrap, bool skip_current,
                                NcMenuPositionMatchesFunc *matches, void *user,
                                int32 *found_pos);
void nc_menu_prepare_refresh(
    NcMenu *menu, int32 height,
    NcMenuPositionIsHighlightableFunc *is_highlightable, void *user
);
void nc_menu_refresh(NcMenu *menu, NcWindow *window, int32 width,
                     int32 height);
void nc_menu_scroll(
    NcMenu *menu, int32 height, enum NcScroll where,
    NcMenuPositionIsHighlightableFunc *is_highlightable, void *user
);
void nc_menu_scroll_selectable(NcMenu *menu, int32 height,
                               enum NcScroll where);
void nc_menu_reset(NcMenu *menu);
void nc_menu_highlight_position(NcMenu *menu, int32 pos, int32 height);
void nc_menu_add_item(NcMenu *menu, void *item);
void nc_menu_add_item_with_flags(NcMenu *menu, void *item, uint32 flags);
void nc_menu_add_separator(NcMenu *menu);
void nc_menu_insert_item_with_flags(NcMenu *menu, int32 pos, void *item,
                                    uint32 flags);
int32 nc_menu_remove_item(NcMenu *menu, enum NcMenuItemSource source,
                          int32 pos);
int32 nc_menu_replace_item(NcMenu *menu, enum NcMenuItemSource source,
                           int32 pos, void *item);
void nc_menu_clear_items(NcMenu *menu);
void nc_menu_clear_filtered_items(NcMenu *menu);
void nc_menu_apply_filter(NcMenu *menu);
void nc_menu_show_all_items(NcMenu *menu);
bool nc_menu_is_filtered(NcMenu *menu);
bool nc_menu_is_empty(NcMenu *menu);
bool nc_menu_position_is_selectable(NcMenu *menu, int32 pos);
bool nc_menu_position_is_separator(NcMenu *menu, int32 pos);
bool nc_menu_position_is_inactive(NcMenu *menu, int32 pos);
bool nc_menu_position_is_selected(NcMenu *menu, int32 pos);
int32 nc_menu_set_position_selected(NcMenu *menu, int32 pos, bool selected);
void nc_menu_clear_selection(NcMenu *menu);
bool nc_menu_has_selected(NcMenu *menu);
int32 nc_menu_selected_count(NcMenu *menu);
bool nc_menu_current_is_selectable(NcMenu *menu);
int32 nc_menu_set_current_selected(NcMenu *menu, bool selected);
int32 nc_menu_toggle_current_selected(NcMenu *menu);
int32 nc_menu_activate_current(NcMenu *menu);
uint32 nc_menu_item_flags_at(NcMenu *menu, enum NcMenuItemSource source,
                             int32 pos);
int32 nc_menu_set_item_flags_at(NcMenu *menu, enum NcMenuItemSource source,
                                int32 pos, uint32 flags);
void *nc_menu_item_at(NcMenu *menu, enum NcMenuItemSource source, int32 pos);
void *nc_menu_active_item_at(NcMenu *menu, int32 pos);
void *nc_menu_current_item(NcMenu *menu);
void nc_menu_swap_item_slots(NcMenu *menu, enum NcMenuItemSource source,
                             int32 left, int32 right);

typedef struct NcScrollpad {
    int32 beginning;
    int32 real_height;
} NcScrollpad;

void nc_scrollpad_init(NcScrollpad *scrollpad, int32 height);
void nc_scrollpad_refresh(NcScrollpad *scrollpad, NcWindow *window);
void nc_scrollpad_resize(NcScrollpad *scrollpad, NcWindow *window,
                         int32 new_width, int32 new_height);
void nc_scrollpad_scroll(NcScrollpad *scrollpad, NcWindow *window,
                         enum NcScroll where);
void nc_scrollpad_flush(NcScrollpad *scrollpad, NcWindow *window,
                        NcBuffer *buffer);
int32 nc_scrollpad_max_beginning(NcScrollpad *scrollpad, NcWindow *window);
int32 nc_scrollpad_buffer_position_row(NcBuffer *buffer, int32 width,
                                       int32 position);
void nc_scrollpad_center_on_buffer_position(NcScrollpad *scrollpad,
                                            NcWindow *window,
                                            NcBuffer *buffer,
                                            int32 position);
void nc_scrollpad_reset(NcScrollpad *scrollpad);

void nc_cyclic_text_write(StrBuilder *output, char *string,
                          int32 string_len, int32 *start_pos,
                          int32 width, char *separator,
                          int32 separator_len, bool scrolling_enabled);
void nc_cyclic_buffer_write(NcBuffer *buffer, NcWindow *window,
                            int32 *start_pos, int32 width,
                            char *separator, int32 separator_len);

#include "c/ncm_c.h"

typedef struct StrBuilderPair {
    StrBuilder first;
    StrBuilder second;
} StrBuilderPair;

typedef struct NcSearchRow {
    NcmSong song;
    NcBuffer buffer;
    bool is_song;
} NcSearchRow;

typedef struct NcMediaLibraryTagRow {
    char *tag;
    int32 tag_len;
    int32 tag_cap;
    time_t mtime;
} NcMediaLibraryTagRow;

typedef struct NcMediaLibraryAlbumRow {
    char *tag;
    char *album;
    char *date;

    int32 tag_len;
    int32 album_len;
    int32 date_len;

    time_t mtime;
    bool all_tracks_entry;
} NcMediaLibraryAlbumRow;

typedef struct NcEditorActionRow {
    char *label;
    int32 label_len;
    int32 label_cap;

    void (*run)(void *user);
    void *user;
} NcEditorActionRow;

typedef struct NcEditorSortRow {
    NcEditorActionRow action;
    enum NcmSongGetter getter;
} NcEditorSortRow;

#define NC_TYPED_MENU_DECLARE_TYPE(TYPE_NAME)                                  \
    typedef struct TYPE_NAME {                                                 \
        NcMenu menu;                                                           \
    } TYPE_NAME

#define NC_TYPED_MENU_DECLARE_INIT(TYPE_NAME, PREFIX)                          \
    void PREFIX##_init(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_DESTROY(TYPE_NAME, PREFIX)                       \
    void PREFIX##_destroy(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_BASE(TYPE_NAME, PREFIX)                          \
    NcMenu *PREFIX##_base(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_ADD(TYPE_NAME, PREFIX, ITEM_TYPE)                \
    void PREFIX##_add(TYPE_NAME *menu, ITEM_TYPE *item)

#define NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(TYPE_NAME, PREFIX, ITEM_TYPE)     \
    void PREFIX##_add_with_flags(TYPE_NAME *menu, ITEM_TYPE *item,             \
                                 uint32 flags)

#define NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(TYPE_NAME, PREFIX)                 \
    void PREFIX##_add_separator(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_INSERT_WITH_FLAGS(TYPE_NAME, PREFIX, ITEM_TYPE)  \
    void PREFIX##_insert_with_flags(TYPE_NAME *menu, int32 pos,                \
                                    ITEM_TYPE *item, uint32 flags)

#define NC_TYPED_MENU_DECLARE_ITEM_AT(TYPE_NAME, PREFIX, ITEM_TYPE)            \
    ITEM_TYPE *PREFIX##_item_at(TYPE_NAME *menu,                               \
                                enum NcMenuItemSource source,                  \
                                int32 pos)

#define NC_TYPED_MENU_DECLARE_CURRENT(TYPE_NAME, PREFIX, ITEM_TYPE)            \
    ITEM_TYPE *PREFIX##_current(TYPE_NAME *menu)

#define NC_TYPED_MENU_DECLARE_COMMON(TYPE_NAME, PREFIX)                        \
    NC_TYPED_MENU_DECLARE_TYPE(TYPE_NAME);                                     \
    NC_TYPED_MENU_DECLARE_INIT(TYPE_NAME, PREFIX);                             \
    NC_TYPED_MENU_DECLARE_DESTROY(TYPE_NAME, PREFIX);                          \
    NC_TYPED_MENU_DECLARE_BASE(TYPE_NAME, PREFIX)

NC_TYPED_MENU_DECLARE_COMMON(NcSongMenu, nc_song_menu);
NC_TYPED_MENU_DECLARE_ADD(NcSongMenu, nc_song_menu, NcmSong);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcSongMenu, nc_song_menu, NcmSong);
NC_TYPED_MENU_DECLARE_CURRENT(NcSongMenu, nc_song_menu, NcmSong);

NC_TYPED_MENU_DECLARE_COMMON(NcBrowserEntryMenu, nc_browser_entry_menu);
NC_TYPED_MENU_DECLARE_ADD(NcBrowserEntryMenu,
                          nc_browser_entry_menu,
                          NcmMpdItem);
NC_TYPED_MENU_DECLARE_CURRENT(NcBrowserEntryMenu,
                              nc_browser_entry_menu,
                              NcmMpdItem);

NC_TYPED_MENU_DECLARE_COMMON(NcPlaylistEntryMenu,
                             nc_playlist_entry_menu);
NC_TYPED_MENU_DECLARE_ADD(NcPlaylistEntryMenu,
                          nc_playlist_entry_menu,
                          NcmPlaylist);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcPlaylistEntryMenu,
                              nc_playlist_entry_menu,
                              NcmPlaylist);
NC_TYPED_MENU_DECLARE_CURRENT(NcPlaylistEntryMenu,
                              nc_playlist_entry_menu,
                              NcmPlaylist);

NC_TYPED_MENU_DECLARE_COMMON(NcTagRowMenu, nc_tag_row_menu);
NC_TYPED_MENU_DECLARE_ADD(NcTagRowMenu,
                          nc_tag_row_menu,
                          NcmMutableSong);
NC_TYPED_MENU_DECLARE_CURRENT(NcTagRowMenu,
                              nc_tag_row_menu,
                              NcmMutableSong);

NC_TYPED_MENU_DECLARE_COMMON(NcSearchRowMenu, nc_search_row_menu);
NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(NcSearchRowMenu,
                                     nc_search_row_menu,
                                     NcSearchRow);
NC_TYPED_MENU_DECLARE_INSERT_WITH_FLAGS(NcSearchRowMenu,
                                        nc_search_row_menu,
                                        NcSearchRow);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcSearchRowMenu,
                              nc_search_row_menu,
                              NcSearchRow);
NC_TYPED_MENU_DECLARE_CURRENT(NcSearchRowMenu,
                              nc_search_row_menu,
                              NcSearchRow);

NC_TYPED_MENU_DECLARE_COMMON(NcMediaLibraryTagMenu,
                             nc_media_library_tag_menu);
NC_TYPED_MENU_DECLARE_ADD(NcMediaLibraryTagMenu,
                          nc_media_library_tag_menu,
                          NcMediaLibraryTagRow);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcMediaLibraryTagMenu,
                              nc_media_library_tag_menu,
                              NcMediaLibraryTagRow);
NC_TYPED_MENU_DECLARE_CURRENT(NcMediaLibraryTagMenu,
                              nc_media_library_tag_menu,
                              NcMediaLibraryTagRow);

NC_TYPED_MENU_DECLARE_COMMON(NcMediaLibraryAlbumMenu,
                             nc_media_library_album_menu);
NC_TYPED_MENU_DECLARE_ADD(NcMediaLibraryAlbumMenu,
                          nc_media_library_album_menu,
                          NcMediaLibraryAlbumRow);
NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(NcMediaLibraryAlbumMenu,
                                     nc_media_library_album_menu,
                                     NcMediaLibraryAlbumRow);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcMediaLibraryAlbumMenu,
                              nc_media_library_album_menu,
                              NcMediaLibraryAlbumRow);
NC_TYPED_MENU_DECLARE_CURRENT(NcMediaLibraryAlbumMenu,
                              nc_media_library_album_menu,
                              NcMediaLibraryAlbumRow);

NC_TYPED_MENU_DECLARE_COMMON(NcMediaLibrarySongMenu,
                             nc_media_library_song_menu);
NC_TYPED_MENU_DECLARE_ADD(NcMediaLibrarySongMenu,
                          nc_media_library_song_menu,
                          NcmSong);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcMediaLibrarySongMenu,
                              nc_media_library_song_menu,
                              NcmSong);
NC_TYPED_MENU_DECLARE_CURRENT(NcMediaLibrarySongMenu,
                              nc_media_library_song_menu,
                              NcmSong);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorStringMenu,
                             nc_editor_string_menu);
NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(NcEditorStringMenu,
                                     nc_editor_string_menu,
                                     StrBuilder);
NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(NcEditorStringMenu,
                                    nc_editor_string_menu);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorPairMenu, nc_editor_pair_menu);
NC_TYPED_MENU_DECLARE_ADD(NcEditorPairMenu,
                          nc_editor_pair_menu,
                          StrBuilderPair);
NC_TYPED_MENU_DECLARE_CURRENT(NcEditorPairMenu,
                              nc_editor_pair_menu,
                              StrBuilderPair);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorActionMenu,
                             nc_editor_action_menu);
NC_TYPED_MENU_DECLARE_ADD(NcEditorActionMenu,
                          nc_editor_action_menu,
                          NcEditorActionRow);
NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(NcEditorActionMenu,
                                    nc_editor_action_menu);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcEditorActionMenu,
                              nc_editor_action_menu,
                              NcEditorActionRow);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorSortMenu, nc_editor_sort_menu);
NC_TYPED_MENU_DECLARE_ADD(NcEditorSortMenu,
                          nc_editor_sort_menu,
                          NcEditorSortRow);
NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(NcEditorSortMenu,
                                    nc_editor_sort_menu);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcEditorSortMenu,
                              nc_editor_sort_menu,
                              NcEditorSortRow);
NC_TYPED_MENU_DECLARE_CURRENT(NcEditorSortMenu,
                              nc_editor_sort_menu,
                              NcEditorSortRow);

NC_TYPED_MENU_DECLARE_COMMON(NcEditorBufferMenu,
                             nc_editor_buffer_menu);
NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS(NcEditorBufferMenu,
                                     nc_editor_buffer_menu,
                                     NcBuffer);
NC_TYPED_MENU_DECLARE_ADD_SEPARATOR(NcEditorBufferMenu,
                                    nc_editor_buffer_menu);
NC_TYPED_MENU_DECLARE_ITEM_AT(NcEditorBufferMenu,
                              nc_editor_buffer_menu,
                              NcBuffer);

#undef NC_TYPED_MENU_DECLARE_TYPE
#undef NC_TYPED_MENU_DECLARE_INIT
#undef NC_TYPED_MENU_DECLARE_DESTROY
#undef NC_TYPED_MENU_DECLARE_BASE
#undef NC_TYPED_MENU_DECLARE_ADD
#undef NC_TYPED_MENU_DECLARE_ADD_WITH_FLAGS
#undef NC_TYPED_MENU_DECLARE_ADD_SEPARATOR
#undef NC_TYPED_MENU_DECLARE_INSERT_WITH_FLAGS
#undef NC_TYPED_MENU_DECLARE_ITEM_AT
#undef NC_TYPED_MENU_DECLARE_CURRENT
#undef NC_TYPED_MENU_DECLARE_COMMON

void nc_search_row_destroy(NcSearchRow *row);
int32 nc_search_row_copy(NcSearchRow *dest, NcSearchRow *source);

void nc_media_library_tag_row_destroy(NcMediaLibraryTagRow *row);
int32 nc_media_library_tag_row_copy(NcMediaLibraryTagRow *dest,
                                    NcMediaLibraryTagRow *source);

void nc_media_library_album_row_destroy(NcMediaLibraryAlbumRow *row);
int32 nc_media_library_album_row_copy(NcMediaLibraryAlbumRow *dest,
                                      NcMediaLibraryAlbumRow *source);

void nc_editor_action_row_destroy(NcEditorActionRow *row);
int32 nc_editor_action_row_copy(NcEditorActionRow *dest,
                                NcEditorActionRow *source);

void nc_editor_sort_row_destroy(NcEditorSortRow *row);
int32 nc_editor_sort_row_copy(NcEditorSortRow *dest, NcEditorSortRow *source);

#endif /* NC_CURSES_H */
