#if !defined(NCMPCPP_NC_WINDOW_H)
#define NCMPCPP_NC_WINDOW_H

#define NCURSES_NOMACROS 1

#include "config.h"

#include "curses.h"

#include <stdbool.h>

#include "cbase/primitives.h"

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

enum NcTermManip {
    NC_TERM_CLEAR_TO_EOL,
};

enum NcFormat {
    NC_FORMAT_BOLD,
    NC_FORMAT_NO_BOLD,
    NC_FORMAT_UNDERLINE,
    NC_FORMAT_NO_UNDERLINE,
    NC_FORMAT_REVERSE,
    NC_FORMAT_NO_REVERSE,
    NC_FORMAT_ALT_CHARSET,
    NC_FORMAT_NO_ALT_CHARSET,
    NC_FORMAT_ITALIC,
    NC_FORMAT_NO_ITALIC,
};

enum NcScroll {
    NC_SCROLL_UP,
    NC_SCROLL_DOWN,
    NC_SCROLL_PAGE_UP,
    NC_SCROLL_PAGE_DOWN,
    NC_SCROLL_HOME,
    NC_SCROLL_END,
};

enum NcPromptStatus {
    NC_PROMPT_ACCEPTED,
    NC_PROMPT_ABORTED,
};

typedef bool (*NcPromptHook)(char *text, void *user_data);

typedef struct NcPrompt {
    char *initial_text;
    int64 width;
    NcPromptHook hook;
    void *hook_user_data;
    bool encrypted;
    bool remember;
} NcPrompt;

typedef struct NcWindow {
    WINDOW *window;
    char *title;
    NcColor *color_stack;
    NcKey *input_queue;
    NcFdCallback *fd_callbacks;

    int64 start_x;
    int64 start_y;
    int64 width;
    int64 height;
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
NcColor nc_color_transparent(void);
NcColor nc_color_current(void);
bool nc_color_equal(NcColor left, NcColor right);
bool nc_color_is_default(NcColor color);
bool nc_color_is_end(NcColor color);
bool nc_color_current_background(NcColor color);
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
void nc_window_init(NcWindow *window, int64 start_x, int64 start_y,
                    int64 width, int64 height, char *title,
                    int32 title_len, NcColor color, NcBorder border);
void nc_window_copy(NcWindow *dest, NcWindow *source);
void nc_window_move(NcWindow *dest, NcWindow *source);
void nc_window_swap(NcWindow *left, NcWindow *right);
void nc_window_destroy(NcWindow *window);

WINDOW *nc_window_raw(NcWindow *window);
int64 nc_window_width(NcWindow *window);
int64 nc_window_height(NcWindow *window);
int64 nc_window_start_x(NcWindow *window);
int64 nc_window_start_y(NcWindow *window);
int32 nc_window_timeout(NcWindow *window);
char *nc_window_title(NcWindow *window);
int32 nc_window_title_len(NcWindow *window);
NcColor nc_window_color(NcWindow *window);
NcColor nc_window_base_color(NcWindow *window);
NcBorder nc_window_border(NcWindow *window);
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
void nc_window_move_to(NcWindow *window, int64 new_x, int64 new_y);
void nc_window_adjust_dimensions(NcWindow *window,
                                 int64 width, int64 height);
void nc_window_resize(NcWindow *window, int64 new_width, int64 new_height);
void nc_window_recreate(NcWindow *window, int64 width, int64 height);
void nc_window_clear(NcWindow *window);

void nc_window_add_fd_callback(NcWindow *window,
                               int32 fd, void (*callback)(void));
void nc_window_clear_fd_callbacks(NcWindow *window);
bool nc_window_fd_callbacks_empty(NcWindow *window);
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
void nc_window_print_int32(NcWindow *window, int32 value);
void nc_window_print_uint64(NcWindow *window, uint64 value);
void nc_window_print_double(NcWindow *window, double value);

#endif /* NCMPCPP_NC_WINDOW_H */
