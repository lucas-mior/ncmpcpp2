#include "cbase_config.h"
#include "cbase.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

#include "util.c"
#include "sort.c"

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

void
cbase_application_mode_link_anchor(void) {
    generic_functions_sink();
    assert_functions_sink();
    arena_functions_sink();
    hash_functions_sink_alloc_map();
    memory_functions_sink();
    minmax_functions_sink();
    utf8_functions_sink();
    util_functions_sink();
    sort_functions_sink();
    return;
}
