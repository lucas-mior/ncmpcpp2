#if !defined(NCMPCPP_TESTS_EXAMPLE_C)
#define NCMPCPP_TESTS_EXAMPLE_C

#include <assert.h>
#include <stdlib.h>

int
main(void) {
    int value;

    value = 1 + 1;
    assert(value == 2);

    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_TESTS_EXAMPLE_C */
