#include <assert.h>
#include <stddef.h>

#include "cbase/base_macros.h"
#include "cbase/cbase.h"
#include "curses/nc_menu.h"

typedef struct TestMenuItem {
    int32 value;
    bool constructed;
} TestMenuItem;

typedef struct TestMenuState {
    int32 copies;
    int32 destroys;
} TestMenuState;

static void test_item_construct(void *dest, void *user);
static void test_item_copy(void *dest, void *source, void *user);
static void test_item_destroy(void *item, void *user);
static bool test_filter(NcMenu *menu, void *item, void *user);
static void test_generic_row_flags(void);
static void test_filter_keeps_row_flags(void);
static void test_row_ownership_helpers(void);
static void test_navigation_clamped_after_shrink(void);

int
main(void) {
    test_generic_row_flags();
    test_filter_keeps_row_flags();
    test_row_ownership_helpers();
    test_navigation_clamped_after_shrink();
    return 0;
}

static void
test_item_construct(void *dest, void *user) {
    TestMenuItem *item;

    (void)user;
    item = dest;
    item->value = 0;
    item->constructed = true;
    return;
}

static void
test_item_copy(void *dest, void *source, void *user) {
    TestMenuItem *dest_item;
    TestMenuItem *source_item;
    TestMenuState *state;

    dest_item = dest;
    source_item = source;
    state = user;
    assert(dest_item->constructed);
    dest_item->value = source_item->value;
    if (state != NULL) {
        state->copies += 1;
    }
    return;
}

static void
test_item_destroy(void *item, void *user) {
    TestMenuItem *test_item;
    TestMenuState *state;

    test_item = item;
    state = user;
    assert(test_item->constructed);
    test_item->value = 0;
    test_item->constructed = false;
    if (state != NULL) {
        state->destroys += 1;
    }
    return;
}

static bool
test_filter(NcMenu *menu, void *item, void *user) {
    TestMenuItem *test_item;

    (void)menu;
    (void)user;
    test_item = item;
    return test_item->value >= 20;
}

static NcMenuItemCallbacks
test_item_callbacks(TestMenuState *state) {
    NcMenuItemCallbacks callbacks = {0};

    callbacks.item_size = SIZEOF(TestMenuItem);
    callbacks.construct = test_item_construct;
    callbacks.copy = test_item_copy;
    callbacks.destroy = test_item_destroy;
    callbacks.user = state;
    return callbacks;
}

static void
test_generic_row_flags(void) {
    NcMenu menu;
    TestMenuState state = {0};
    TestMenuItem item;

    nc_menu_init(&menu);
    nc_menu_set_item_callbacks(&menu, test_item_callbacks(&state));

    item.value = 10;
    nc_menu_add_item(&menu, &item);
    item.value = 20;
    nc_menu_add_item_with_flags(&menu, &item,
                                NC_MENU_ITEM_SELECTABLE
                                | NC_MENU_ITEM_INACTIVE);
    nc_menu_add_separator(&menu);

    assert(nc_menu_item_count(&menu) == 3);
    assert(nc_menu_position_is_selectable(&menu, 0));
    assert(!nc_menu_position_is_selectable(&menu, 1));
    assert(nc_menu_position_is_inactive(&menu, 1));
    assert(nc_menu_position_is_separator(&menu, 2));
    assert(!nc_menu_position_is_selectable(&menu, 2));

    assert(nc_menu_set_position_selected(&menu, 0, true));
    assert(nc_menu_position_is_selected(&menu, 0));
    assert(nc_menu_selected_count(&menu) == 1);
    assert(nc_menu_first_selected_position(&menu) == 0);
    nc_menu_reverse_selection(&menu);
    assert(!nc_menu_position_is_selected(&menu, 0));
    assert(!nc_menu_position_is_selected(&menu, 1));
    assert(!nc_menu_position_is_selected(&menu, 2));

    nc_menu_destroy(&menu);
    assert(state.copies == 2);
    assert(state.destroys == 3);
    return;
}

static void
test_filter_keeps_row_flags(void) {
    NcMenu menu;
    NcMenuDisplayCallbacks display_callbacks = {0};
    TestMenuState state = {0};
    TestMenuItem item;

    nc_menu_init(&menu);
    nc_menu_set_item_callbacks(&menu, test_item_callbacks(&state));
    display_callbacks.filter = test_filter;
    nc_menu_set_display_callbacks(&menu, display_callbacks);

    item.value = 10;
    nc_menu_add_item(&menu, &item);
    item.value = 20;
    nc_menu_add_item(&menu, &item);
    item.value = 30;
    nc_menu_add_item_with_flags(&menu, &item,
                                NC_MENU_ITEM_SELECTABLE
                                | NC_MENU_ITEM_SELECTED);

    nc_menu_apply_filter(&menu);
    assert(nc_menu_is_filtered(&menu));
    assert(nc_menu_item_count(&menu) == 2);
    assert(!nc_menu_position_is_selected(&menu, 0));
    assert(nc_menu_position_is_selected(&menu, 1));
    assert(nc_menu_set_position_selected(&menu, 0, true));

    nc_menu_show_all_items(&menu);
    assert(nc_menu_position_is_selected(&menu, 1));
    assert(nc_menu_position_is_selected(&menu, 2));

    nc_menu_destroy(&menu);
    assert(state.copies == 3);
    assert(state.destroys == 3);
    return;
}

static void
test_row_ownership_helpers(void) {
    NcMenu menu;
    TestMenuState state = {0};
    TestMenuItem item;
    TestMenuItem *stored;

    nc_menu_init(&menu);
    nc_menu_set_item_callbacks(&menu, test_item_callbacks(&state));

    item.value = 1;
    nc_menu_add_item(&menu, &item);
    item.value = 3;
    nc_menu_add_item(&menu, &item);
    item.value = 2;
    nc_menu_insert_item(&menu, 1, &item);
    assert(nc_menu_item_count(&menu) == 3);

    stored = nc_menu_item_at(&menu, NC_MENU_ITEMS_ALL, 1);
    assert(stored->value == 2);

    item.value = 22;
    assert(nc_menu_replace_item(&menu, NC_MENU_ITEMS_ALL, 1, &item));
    stored = nc_menu_item_at(&menu, NC_MENU_ITEMS_ALL, 1);
    assert(stored->value == 22);

    assert(nc_menu_remove_item(&menu, NC_MENU_ITEMS_ALL, 1));
    assert(nc_menu_item_count(&menu) == 2);
    stored = nc_menu_item_at(&menu, NC_MENU_ITEMS_ALL, 1);
    assert(stored->value == 3);

    nc_menu_destroy(&menu);
    assert(state.copies == 4);
    assert(state.destroys == 4);
    return;
}

static void
test_navigation_clamped_after_shrink(void) {
    NcMenu menu;
    TestMenuState state = {0};
    TestMenuItem item;

    nc_menu_init(&menu);
    nc_menu_set_item_callbacks(&menu, test_item_callbacks(&state));

    item.value = 1;
    nc_menu_add_item(&menu, &item);
    item.value = 2;
    nc_menu_add_item(&menu, &item);
    item.value = 3;
    nc_menu_add_item(&menu, &item);

    nc_menu_highlight_position(&menu, 2, 1);
    assert(nc_menu_highlight(&menu) == 2);
    assert(nc_menu_beginning(&menu) == 2);

    nc_menu_resize_all_items(&menu, 2);
    assert(nc_menu_item_count(&menu) == 2);
    assert(nc_menu_highlight(&menu) == 1);
    assert(nc_menu_beginning(&menu) == 1);

    assert(nc_menu_remove_item(&menu, NC_MENU_ITEMS_ALL, 1));
    assert(nc_menu_item_count(&menu) == 1);
    assert(nc_menu_highlight(&menu) == 0);
    assert(nc_menu_beginning(&menu) == 0);

    nc_menu_clear_items(&menu);
    assert(nc_menu_item_count(&menu) == 0);
    assert(nc_menu_highlight(&menu) == 0);
    assert(nc_menu_beginning(&menu) == 0);

    nc_menu_destroy(&menu);
    assert(state.destroys == 3);
    return;
}
