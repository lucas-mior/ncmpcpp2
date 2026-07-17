#include <assert.h>
#include <stdlib.h>

#include "settings.h"
#include "statusbar.h"

static void test_prompt_return_one_of_accepts_value(void);
static void test_prompt_return_one_of_aborts(void);
static void test_statusbar_scoped_lock_unlocks(void);
static void test_progressbar_scoped_lock_unlocks(void);

int
main(void) {
    test_prompt_return_one_of_accepts_value();
    test_prompt_return_one_of_aborts();
    test_statusbar_scoped_lock_unlocks();
    test_progressbar_scoped_lock_unlocks();
    exit(EXIT_SUCCESS);
}

static void
test_prompt_return_one_of_accepts_value(void) {
    NcWindow window;
    char result;

    nc_window_init_empty(&window);
    nc_window_push_key(&window, 'z');
    nc_window_push_key(&window, 'y');

    result = '\0';
    assert(ncm_statusbar_prompt_return_one_of(&window,
                                              "yn", 2,
                                              &result));
    assert(result == 'y');

    nc_window_destroy(&window);
    return;
}

static void
test_prompt_return_one_of_aborts(void) {
    NcWindow window;
    char result;

    nc_window_init_empty(&window);
    nc_window_push_key(&window, NC_KEY_CTRL_G);

    result = 'x';
    assert(!ncm_statusbar_prompt_return_one_of(&window,
                                               "yn", 2,
                                               &result));
    assert(result == 'x');

    nc_window_destroy(&window);
    return;
}

static void
test_statusbar_scoped_lock_unlocks(void) {
    NcmStatusbarScopedLock lock;

    Config.statusbar_visibility = true;
    assert(ncm_statusbar_is_unlocked());
    ncm_statusbar_scoped_lock_init(&lock);
    assert(!ncm_statusbar_is_unlocked());
    ncm_statusbar_scoped_lock_destroy(&lock);
    assert(ncm_statusbar_is_unlocked());
    return;
}

static void
test_progressbar_scoped_lock_unlocks(void) {
    NcmStatusbarScopedLock lock;

    assert(ncm_progressbar_is_unlocked());
    ncm_progressbar_scoped_lock_init(&lock);
    assert(!ncm_progressbar_is_unlocked());
    ncm_progressbar_scoped_lock_destroy(&lock);
    assert(ncm_progressbar_is_unlocked());
    return;
}
