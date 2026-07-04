#if !defined(NCMPCPP_NC_MENU_H)
#define NCMPCPP_NC_MENU_H

#include <stdbool.h>

#include "cbase/primitives.h"
#include "curses/nc_window.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef bool (*NcMenuHighlightableFunc)(int64 pos, void *user);

typedef struct NcMenu {
    int64 item_count;
    int64 beginning;
    int64 highlight;
    int64 drawn_position;

    bool highlight_enabled;
    bool cyclic_scroll_enabled;
    bool autocenter_cursor;
} NcMenu;

void nc_menu_init(NcMenu *menu);
void nc_menu_copy(NcMenu *dest, NcMenu *source);
void nc_menu_swap(NcMenu *left, NcMenu *right);
void nc_menu_set_item_count(NcMenu *menu, int64 item_count);
int64 nc_menu_item_count(NcMenu *menu);
int64 nc_menu_beginning(NcMenu *menu);
int64 nc_menu_highlight(NcMenu *menu);
int64 nc_menu_drawn_position(NcMenu *menu);
void nc_menu_set_drawn_position(NcMenu *menu, int64 drawn_position);
bool nc_menu_highlight_enabled(NcMenu *menu);
void nc_menu_set_highlighting(NcMenu *menu, bool state);
void nc_menu_set_cyclic_scrolling(NcMenu *menu, bool state);
void nc_menu_set_centered_cursor(NcMenu *menu, bool state);
bool nc_menu_goto(NcMenu *menu, int64 y,
                  NcMenuHighlightableFunc is_highlightable, void *user);
void nc_menu_prepare_refresh(NcMenu *menu, int64 height,
                             NcMenuHighlightableFunc is_highlightable,
                             void *user);
void nc_menu_scroll(NcMenu *menu, int64 height, enum NcScroll where,
                    NcMenuHighlightableFunc is_highlightable, void *user);
void nc_menu_reset(NcMenu *menu);
void nc_menu_highlight_position(NcMenu *menu, int64 pos, int64 height);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_MENU_H */
