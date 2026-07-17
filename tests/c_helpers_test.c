#include "helpers.h"

#include <assert.h>
#include <string.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"
#include "cbase/util.c"

typedef struct IntRow {
    int32 value;
    bool selected;
} IntRow;

typedef struct CountState {
    int32 count;
    int32 sum;
} CountState;

typedef struct SearchState {
    int32 wanted;
} SearchState;

static bool
count_song_list_item(NcmSongListItem *item, int32 idx, void *user) {
    CountState *state;

    (void)item;
    state = user;
    state->count += 1;
    state->sum += idx;
    return true;
}

static bool
match_song_list_index(NcmSongListItem *item, int32 idx, void *user) {
    SearchState *state;

    (void)item;
    state = user;
    return idx == state->wanted;
}

static bool
count_mpd_song(NcmSong *song, int32 idx, void *user) {
    CountState *state;

    (void)song;
    state = user;
    state->count += 1;
    state->sum += idx;
    return true;
}

static bool
match_mpd_song_index(NcmSong *song, int32 idx, void *user) {
    SearchState *state;

    (void)song;
    state = user;
    return idx == state->wanted;
}

static void
copy_int_row(void *dest, void *source, void *user) {
    (void)user;
    memcpy64(dest, source, SIZEOF(IntRow));
    return;
}

static bool
int_row_is_selected(void *item, void *user) {
    IntRow *row;

    (void)user;
    row = item;
    return row->selected;
}

static void
int_row_set_selected(void *item, bool selected, void *user) {
    IntRow *row;

    (void)user;
    row = item;
    row->selected = selected;
    return;
}

static bool
count_menu_item(void *item, int64 pos, void *user) {
    CountState *state;
    IntRow *row;

    row = item;
    state = user;
    state->count += 1;
    state->sum += row->value + (int32)pos;
    return true;
}

static bool
match_menu_value(void *item, int64 pos, void *user) {
    SearchState *state;
    IntRow *row;

    (void)pos;
    row = item;
    state = user;
    return row->value == state->wanted;
}

static void
test_song_list_iteration_helpers(void) {
    NcmSongList list;
    CountState count = {0};
    SearchState search;
    int32 first;
    int32 last;

    ncm_song_list_init(&list);
    assert(ncm_song_list_append(&list) != NULL);
    assert(ncm_song_list_append(&list) != NULL);
    assert(ncm_song_list_append(&list) != NULL);
    assert(ncm_song_list_set_current(&list, 1));

    assert(!ncm_song_list_has_selected_item(&list));
    ncm_song_list_select_current_if_none_selected(&list);
    assert(ncm_song_list_has_selected_item(&list));
    assert(ncm_song_list_find_selected_range(&list, &first, &last));
    assert(first == 1);
    assert(last == 2);
    assert(ncm_song_list_find_full_selected_range(&list, &first, &last));

    assert(ncm_song_list_each_selected(&list, count_song_list_item, &count));
    assert(count.count == 1);
    assert(count.sum == 1);

    ncm_song_list_reverse_selection(&list);
    assert(ncm_song_list_find_selected_range(&list, &first, &last));
    assert(first == 0);
    assert(last == 3);
    assert(!ncm_song_list_find_full_selected_range(&list, &first, &last));

    search.wanted = 0;
    assert(ncm_song_list_wrapped_search(&list, 1,
                                        NCM_SEARCH_DIRECTION_FORWARD,
                                        true, true,
                                        match_song_list_index,
                                        &search) == 0);

    ncm_song_list_destroy(&list);
    return;
}

static void
test_mpd_song_list_iteration_helpers(void) {
    NcmSong songs[3] = {0};
    NcmMpdSongList list;
    CountState count = {0};
    SearchState search;

    list.items = songs;
    list.count = NCM_ARRAY_LEN(songs);
    list.capacity = NCM_ARRAY_LEN(songs);

    assert(ncm_mpd_song_list_each_song(&list, count_mpd_song, &count));
    assert(count.count == 3);
    assert(count.sum == 3);

    search.wanted = 2;
    assert(ncm_mpd_song_list_wrapped_search(&list, 0,
                                            NCM_SEARCH_DIRECTION_FORWARD,
                                            true, true,
                                            match_mpd_song_index,
                                            &search) == 2);
    return;
}

static void
test_menu_selection_helpers(void) {
    NcMenu menu;
    IntRow row;
    CountState count = {0};
    SearchState search;
    int64 first;
    int64 last;

    nc_menu_init(&menu);
    nc_menu_set_item_callbacks(
        &menu,
        (NcMenuItemCallbacks){
            .item_size = SIZEOF(IntRow),
            .copy = copy_int_row,
        }
    );
    nc_menu_set_display_callbacks(
        &menu,
        (NcMenuDisplayCallbacks){
            .is_selected = int_row_is_selected,
        }
    );
    nc_menu_set_action_callbacks(
        &menu,
        (NcMenuActionCallbacks){
            .set_selected = int_row_set_selected,
        }
    );

    row = (IntRow){
        .value = 10,
    };
    nc_menu_add_item(&menu, &row);
    row = (IntRow){
        .value = 20,
    };
    nc_menu_add_item(&menu, &row);
    row = (IntRow){
        .value = 30,
    };
    nc_menu_add_item(&menu, &row);
    nc_menu_sync_item_count(&menu);
    nc_menu_highlight_position(&menu, 1, 3);

    assert(!ncm_menu_has_selected_item(&menu, NC_MENU_ITEMS_ALL));
    ncm_menu_select_current_if_none_selected(&menu);
    assert(ncm_menu_has_selected_item(&menu, NC_MENU_ITEMS_ALL));
    assert(ncm_menu_find_selected_range(&menu, NC_MENU_ITEMS_ALL,
                                        &first, &last));
    assert(first == 1);
    assert(last == 2);

    assert(ncm_menu_each_selected(&menu, NC_MENU_ITEMS_ALL,
                                  count_menu_item, &count));
    assert(count.count == 1);
    assert(count.sum == 21);

    ncm_menu_reverse_selection(&menu, NC_MENU_ITEMS_ALL);
    assert(ncm_menu_find_selected_range(&menu, NC_MENU_ITEMS_ALL,
                                        &first, &last));
    assert(first == 0);
    assert(last == 3);
    assert(!ncm_menu_find_full_selected_range(&menu, NC_MENU_ITEMS_ALL,
                                              &first, &last));

    search.wanted = 10;
    assert(ncm_menu_wrapped_search(&menu, NC_MENU_ITEMS_ALL, 1,
                                   NCM_SEARCH_DIRECTION_FORWARD,
                                   true, true, match_menu_value,
                                   &search) == 0);

    nc_menu_destroy(&menu);
    return;
}

static void
test_native_menu_selection_helpers(void) {
    NcMenu menu;
    IntRow row;
    int64 first;
    int64 last;

    nc_menu_init(&menu);
    nc_menu_set_item_callbacks(
        &menu,
        (NcMenuItemCallbacks){
            .item_size = SIZEOF(IntRow),
            .copy = copy_int_row,
        }
    );

    row = (IntRow){
        .value = 10,
    };
    nc_menu_add_item(&menu, &row);
    row = (IntRow){
        .value = 20,
    };
    nc_menu_add_item(&menu, &row);
    row = (IntRow){
        .value = 30,
    };
    nc_menu_add_item(&menu, &row);
    nc_menu_sync_item_count(&menu);

    assert(nc_menu_set_position_selected(&menu, 1, true));
    assert(nc_menu_set_position_selected(&menu, 2, true));
    assert(ncm_menu_has_selected_item(&menu, NC_MENU_ITEMS_ALL));
    assert(ncm_menu_find_full_selected_range(&menu, NC_MENU_ITEMS_ALL,
                                              &first, &last));
    assert(first == 1);
    assert(last == 3);

    ncm_menu_reverse_selection(&menu, NC_MENU_ITEMS_ALL);
    assert(ncm_menu_find_selected_range(&menu, NC_MENU_ITEMS_ALL,
                                        &first, &last));
    assert(first == 0);
    assert(last == 1);

    nc_menu_destroy(&menu);
    return;
}

static void
test_format_helpers(void) {
    NcmBuffer buffer;
    char song_time[32];

    assert(strcmp(ncm_helpers_with_errors(true), "") == 0);
    assert(strcmp(ncm_helpers_with_errors(false), " (with errors)") == 0);
    assert(ncm_helpers_show_song_time(65, song_time,
                                      (int32)SIZEOF(song_time)) > 0);

    ncm_buffer_init(&buffer);
    assert(ncm_helpers_timestamp(&buffer, (time_t)0));
    assert(buffer.len > 0);
    ncm_buffer_destroy(&buffer);
    return;
}

int
main(void) {
    test_song_list_iteration_helpers();
    test_mpd_song_list_iteration_helpers();
    test_menu_selection_helpers();
    test_native_menu_selection_helpers();
    test_format_helpers();
    return 0;
}
