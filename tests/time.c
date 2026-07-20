#if !defined(NCMPCPP_TESTS_TIME_C)
#define NCMPCPP_TESTS_TIME_C

#include <assert.h>

#include "c/ncm_error.c"
#include "c/ncm_time.c"

static void
time_test_elapsed_from_zero(void) {
    NcmTimePoint start;
    NcmTimePoint end;

    start.ns = 0;
    end.ns = 3000000000ll;

    assert(ncm_time_elapsed_ns(start, end) == 3000000000ll);
    assert(ncm_time_elapsed_ms(start, end) == 3000);
    return;
}

static void
time_test_elapsed_across_old_wrap_boundary(void) {
    NcmTimePoint start;
    NcmTimePoint end;

    start.ns = 4000000000ll;
    end.ns = 5500000000ll;

    assert(ncm_time_elapsed_ns(start, end) == 1500000000ll);
    assert(ncm_time_elapsed_ms(start, end) == 1500);
    return;
}

int
main(void) {
    time_test_elapsed_from_zero();
    time_test_elapsed_across_old_wrap_boundary();
    return 0;
}

#endif /* NCMPCPP_TESTS_TIME_C */
