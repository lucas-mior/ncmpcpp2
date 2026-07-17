#if !defined(NCMPCPP_NC_MENU_H)
#define NCMPCPP_NC_MENU_H

#include <stdbool.h>

#include "cbase/primitives.h"
#include "curses/nc_buffer.h"
#include "curses/nc_window.h"

typedef struct NcMenu NcMenu;

typedef bool (*NcMenuHighlightableFunc)(int64 pos, void *user);

enum NcMenuItemSource {
    NC_MENU_ITEMS_ALL,
    NC_MENU_ITEMS_FILTERED,
};

enum NcMenuItemFlag {
    NC_MENU_ITEM_SELECTABLE = 1U << 0,
    NC_MENU_ITEM_SELECTED = 1U << 1,
    NC_MENU_ITEM_INACTIVE = 1U << 2,
    NC_MENU_ITEM_SEPARATOR = 1U << 3,
};

typedef struct NcMenuItemCallbacks {
    int64 item_size;
    void (*construct)(void *dest, void *user);
    void (*copy)(void *dest, void *source, void *user);
    void (*destroy)(void *item, void *user);
    void *user;
} NcMenuItemCallbacks;

typedef struct NcMenuDisplayCallbacks {
    void (*draw)(NcMenu *menu, NcWindow *window, void *item,
                 int64 pos, void *user);
    bool (*filter)(NcMenu *menu, void *item, void *user);
    bool (*is_separator)(void *item, void *user);
    bool (*is_selected)(void *item, void *user);
    bool (*is_inactive)(void *item, void *user);
    void *user;
} NcMenuDisplayCallbacks;

typedef struct NcMenuActionCallbacks {
    void (*activate)(NcMenu *menu, void *item, int64 pos, void *user);
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

    int64 item_count;
    int64 beginning;
    int64 highlight;
    int64 drawn_position;

    bool highlight_enabled;
    bool cyclic_scroll_enabled;
    bool autocenter_cursor;
};

void nc_menu_init(NcMenu *menu);
void nc_menu_destroy(NcMenu *menu);
void nc_menu_copy(NcMenu *dest, NcMenu *source);
void nc_menu_swap(NcMenu *left, NcMenu *right);
void nc_menu_set_item_callbacks(NcMenu *menu, NcMenuItemCallbacks callbacks);
void nc_menu_set_display_callbacks(NcMenu *menu,
                                   NcMenuDisplayCallbacks callbacks);
void nc_menu_set_action_callbacks(NcMenu *menu,
                                  NcMenuActionCallbacks callbacks);
void nc_menu_sync_item_count(NcMenu *menu);
int64 nc_menu_item_count(NcMenu *menu);
int64 nc_menu_all_item_count(NcMenu *menu);
int64 nc_menu_filtered_item_count(NcMenu *menu);
int64 nc_menu_highlight(NcMenu *menu);
bool nc_menu_highlight_enabled(NcMenu *menu);
void nc_menu_set_highlight_prefix(NcMenu *menu, NcBuffer *buffer);
void nc_menu_set_highlight_suffix(NcMenu *menu, NcBuffer *buffer);
void nc_menu_set_selected_prefix(NcMenu *menu, NcBuffer *buffer);
void nc_menu_set_selected_suffix(NcMenu *menu, NcBuffer *buffer);
void nc_menu_set_highlighting(NcMenu *menu, bool state);
void nc_menu_set_cyclic_scrolling(NcMenu *menu, bool state);
void nc_menu_set_centered_cursor(NcMenu *menu, bool state);
bool nc_menu_goto(NcMenu *menu, int64 y,
                  NcMenuHighlightableFunc is_highlightable, void *user);
bool nc_menu_goto_selectable(NcMenu *menu, int64 y);
void nc_menu_prepare_refresh(NcMenu *menu, int64 height,
                             NcMenuHighlightableFunc is_highlightable,
                             void *user);
void nc_menu_refresh(NcMenu *menu, NcWindow *window, int64 width,
                     int64 height);
void nc_menu_scroll(NcMenu *menu, int64 height, enum NcScroll where,
                    NcMenuHighlightableFunc is_highlightable, void *user);
void nc_menu_scroll_selectable(NcMenu *menu, int64 height,
                               enum NcScroll where);
void nc_menu_reset(NcMenu *menu);
void nc_menu_highlight_position(NcMenu *menu, int64 pos, int64 height);
void nc_menu_add_item(NcMenu *menu, void *item);
void nc_menu_add_item_with_flags(NcMenu *menu, void *item, uint32 flags);
void nc_menu_add_separator(NcMenu *menu);
void nc_menu_insert_item_with_flags(NcMenu *menu, int64 pos, void *item,
                                    uint32 flags);
bool nc_menu_remove_item(NcMenu *menu, enum NcMenuItemSource source,
                         int64 pos);
bool nc_menu_replace_item(NcMenu *menu, enum NcMenuItemSource source,
                          int64 pos, void *item);
void nc_menu_clear_items(NcMenu *menu);
void nc_menu_clear_filtered_items(NcMenu *menu);
void nc_menu_add_filtered_item_ref(NcMenu *menu, void *item);
void nc_menu_apply_filter(NcMenu *menu);
void nc_menu_show_all_items(NcMenu *menu);
void nc_menu_show_filtered_items(NcMenu *menu);
bool nc_menu_is_filtered(NcMenu *menu);
bool nc_menu_empty(NcMenu *menu);
bool nc_menu_position_is_selectable(NcMenu *menu, int64 pos);
bool nc_menu_position_is_separator(NcMenu *menu, int64 pos);
bool nc_menu_position_is_inactive(NcMenu *menu, int64 pos);
bool nc_menu_position_is_selected(NcMenu *menu, int64 pos);
bool nc_menu_set_position_selected(NcMenu *menu, int64 pos, bool selected);
bool nc_menu_toggle_position_selected(NcMenu *menu, int64 pos);
void nc_menu_clear_selection(NcMenu *menu);
bool nc_menu_has_selected(NcMenu *menu);
int64 nc_menu_selected_count(NcMenu *menu);
int64 nc_menu_first_selected_position(NcMenu *menu);
bool nc_menu_current_is_selectable(NcMenu *menu);
bool nc_menu_set_current_selected(NcMenu *menu, bool selected);
bool nc_menu_toggle_current_selected(NcMenu *menu);
bool nc_menu_activate_position(NcMenu *menu, int64 pos);
bool nc_menu_activate_current(NcMenu *menu);
uint32 nc_menu_item_flags_at(NcMenu *menu, enum NcMenuItemSource source,
                             int64 pos);
bool nc_menu_set_item_flags_at(NcMenu *menu, enum NcMenuItemSource source,
                               int64 pos, uint32 flags);
void *nc_menu_item_at(NcMenu *menu, enum NcMenuItemSource source, int64 pos);
void *nc_menu_active_item_at(NcMenu *menu, int64 pos);
void *nc_menu_current_item(NcMenu *menu);
void nc_menu_swap_item_slots(NcMenu *menu, enum NcMenuItemSource source,
                             int64 left, int64 right);

#endif /* NCMPCPP_NC_MENU_H */
