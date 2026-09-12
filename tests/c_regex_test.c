#include "cbase.h"
#include "ncmpcpp2.h"

#define main ncmpcpp2_application_main
#include "main.c"
#undef main

static bool
regex_test_count_callback(int32 start, int32 len, void *user) {
    int32 *count = user;

    ASSERT_NON_NEGATIVE(start);
    ASSERT_NON_NEGATIVE(len);
    *count += 1;
    return true;
}

int
main(void) {
    NcmRegex regex = {0};
    NcmError ncm_error = {0};
    int32 count = 0;

    ASSERT_ZERO(ncm_regex_compile(&regex, STRLIT(""), 0, &ncm_error));
    ASSERT(!ncm_regex_matches(&regex, NULL, 0));
    ASSERT(ncm_regex_for_each_match(&regex, NULL, 0,
                                    regex_test_count_callback,
                                    &count) == -EINVAL);
    ASSERT_ZERO(count);

    ASSERT(ncm_regex_for_each_match(&regex, STRLIT(""),
                                    regex_test_count_callback, &count) == 1);
    ASSERT(count == 1);

    ncm_regex_destroy(&regex);
    return 0;
}
