#define main ncmpcpp2_application_main
#include "main.c"
#undef main

static void
test_append_int64_appends_at_current_end(void) {
    NcBuffer buffer = {0};

    nc_buffer_append_data(&buffer, STRLIT("Search results: Found "));
    nc_buffer_append_int64(&buffer, 14);
    nc_buffer_append_data(&buffer, STRLIT(" songs"));

    ASSERT_EQUAL(buffer.data, buffer.len,
                 "Search results: Found 14 songs");

    nc_buffer_destroy(&buffer);
    return;
}

int
main(void) {
    test_append_int64_appends_at_current_end();
    return 0;
}
