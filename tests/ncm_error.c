#if !defined(NCMPCPP_TESTS_NCM_ERROR_C)
#define NCMPCPP_TESTS_NCM_ERROR_C

#define CBASE_IMPLEMENT
#include "cbase.h"

#include "c/ncm_error.c"

static void
ncm_error_test_project_codes_are_positive(void) {
    ASSERT(NCM_ERROR_INVALID_STATE > 0);
    ASSERT(NCM_ERROR_NOT_FOUND > 0);
    ASSERT(NCM_ERROR_UNAVAILABLE > 0);
    ASSERT(NCM_ERROR_CANCELLED > 0);
    ASSERT(NCM_ERROR_PARSE > 0);
    ASSERT(NCM_ERROR_MPD > 0);
    ASSERT(NCM_ERROR_TAGLIB > 0);
    ASSERT(NCM_ERROR_NETWORK > 0);
    ASSERT(NCM_ERROR_EXTERNAL_COMMAND > 0);
    return;
}

static void
ncm_error_test_status_from_error_code(void) {
    ASSERT_ZERO(ncm_status_from_error_code(0));
    ASSERT_EQUAL(ncm_status_from_error_code(EINVAL), -EINVAL);
    ASSERT_EQUAL(ncm_status_from_error_code(-EINVAL), -EINVAL);
    ASSERT_EQUAL(ncm_status_from_error_code(NCM_ERROR_PARSE),
                 -NCM_ERROR_PARSE);
    return;
}

static void
ncm_error_test_error_status(void) {
    NcmError ncm_error = {0};

    ASSERT_ZERO(ncm_error_status(NULL));
    ASSERT_ZERO(ncm_error_status(&ncm_error));

    ncm_error_set(&ncm_error, EINVAL, STRLIT("invalid input"));
    ASSERT_EQUAL(ncm_error_status(&ncm_error), -EINVAL);

    ncm_error_set(&ncm_error, -EINVAL, STRLIT("legacy negative"));
    ASSERT_EQUAL(ncm_error_status(&ncm_error), -EINVAL);
    return;
}

static void
ncm_error_test_set_status(void) {
    NcmError ncm_error = {0};
    int32 status;

    status = ncm_error_set_status(&ncm_error, -NCM_ERROR_PARSE,
                                  STRLIT("parse error"));
    ASSERT_EQUAL(status, -NCM_ERROR_PARSE);
    ASSERT(ncm_error_is_set(&ncm_error));
    ASSERT_EQUAL(ncm_error.code, NCM_ERROR_PARSE);
    ASSERT_EQUAL(ncm_error.message, "parse error");

    status = ncm_error_set_status(&ncm_error, EINVAL,
                                  STRLIT("invalid input"));
    ASSERT_EQUAL(status, -EINVAL);
    ASSERT(ncm_error_is_set(&ncm_error));
    ASSERT_EQUAL(ncm_error.code, EINVAL);
    ASSERT_EQUAL(ncm_error.message, "invalid input");
    return;
}

static void
ncm_error_test_ok_clears_error(void) {
    NcmError ncm_error = {0};
    int32 status;

    ncm_error_set(&ncm_error, EINVAL, STRLIT("invalid input"));
    status = ncm_error_ok(&ncm_error);

    ASSERT_ZERO(status);
    ASSERT(!ncm_error_is_set(&ncm_error));
    return;
}

int
main(void) {
    ncm_error_test_project_codes_are_positive();
    ncm_error_test_status_from_error_code();
    ncm_error_test_error_status();
    ncm_error_test_set_status();
    ncm_error_test_ok_clears_error();
    return 0;
}

#endif /* NCMPCPP_TESTS_NCM_ERROR_C */
