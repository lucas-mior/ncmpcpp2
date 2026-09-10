#include "cbase.h"
#include "ncmpcpp2.h"

#define main ncmpcpp2_application_main
#include "main.c"
#undef main

static void
test_database_update_state_is_quiet_without_change(void) {
    status_db_updating = 0;

    ASSERT(!status_database_update_state_changed(0));
    ASSERT_ZERO(status_db_updating);

    ASSERT(status_database_update_state_changed(12));
    ASSERT(status_db_updating == 'U');

    ASSERT(!status_database_update_state_changed(13));
    ASSERT(status_db_updating == 'U');

    ASSERT(status_database_update_state_changed(0));
    ASSERT_ZERO(status_db_updating);

    ASSERT(!status_database_update_state_changed(0));
    ASSERT_ZERO(status_db_updating);
    return;
}

int
main(void) {
    test_database_update_state_is_quiet_without_change();
    return 0;
}
