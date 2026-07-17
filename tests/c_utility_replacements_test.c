#include <stdio.h>
#include <stdlib.h>

#include "c/ncm_conversion.h"
#include "c/ncm_functional.h"
#include "c/ncm_iterator.h"
#include "c/ncm_regex.h"
#include "c/ncm_scoped_value.h"
#include "c/ncm_shared_resource.h"
#include "c/ncm_string.h"
#include "c/ncm_string_format.h"
#include "c/ncm_wide_string.h"
#include "cbase/base_macros.h"

#define LIT_ARGS(S) (char *)S, STRLIT_LEN(S)

#define REQUIRE(COND) do {                                                \
    if (!(COND)) {                                                        \
        fail(__FILE__, __LINE__, (char *)#COND);                         \
    }                                                                     \
} while (0)

#define REQUIRE_INT(ACTUAL, EXPECTED)                                     \
    require_int(__FILE__, __LINE__, (char *)#ACTUAL, ACTUAL, EXPECTED)

#define REQUIRE_STRING(ACTUAL, ACTUAL_LEN, EXPECTED)                      \
    require_string(__FILE__, __LINE__, (char *)#ACTUAL,                   \
                   ACTUAL, ACTUAL_LEN, LIT_ARGS(EXPECTED))

typedef struct TestItem {
    int32 value;
    int32 mapped;
} TestItem;

static void fail(char *file, int32 line, char *condition);
static void require_int(char *file, int32 line, char *name,
                        int32 actual, int32 expected);
static void require_string(char *file, int32 line, char *name,
                           char *actual, int32 actual_len,
                           char *expected, int32 expected_len);
static bool test_item_gt(void *item, void *user);
static void test_item_mark(void *item, void *user);
static void *test_item_transform(void *item, void *user);
static void test_conversion(void);
static void test_string_format(void);
static void test_wide_string(void);
static void test_regex_flags(void);
static void test_functional(void);
static void test_scoped_value(void);
static void test_shared_resource(void);
static void test_iterators(void);

static void
fail(char *file, int32 line, char *condition) {
    fprintf(stderr, "%s:%d: requirement failed: %s\n",
            file, line, condition);
    exit(EXIT_FAILURE);
}

static void
require_int(char *file, int32 line, char *name,
            int32 actual, int32 expected) {
    if (actual != expected) {
        fprintf(stderr, "%s:%d: %s: expected %d, got %d\n",
                file, line, name, expected, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static void
require_string(char *file, int32 line, char *name,
               char *actual, int32 actual_len,
               char *expected, int32 expected_len) {
    if (!STREQUAL(actual, actual_len, expected, expected_len)) {
        fprintf(stderr, "%s:%d: %s: expected '%.*s', got '%.*s'\n",
                file, line, name,
                expected_len, expected, actual_len, actual);
        exit(EXIT_FAILURE);
    }
    return;
}

static bool
test_item_gt(void *item, void *user) {
    TestItem *test_item;
    int32 *threshold;

    test_item = item;
    threshold = user;
    return test_item->value > *threshold;
}

static void
test_item_mark(void *item, void *user) {
    TestItem *test_item;
    int32 *amount;

    test_item = item;
    amount = user;
    test_item->mapped += *amount;
    return;
}

static void *
test_item_transform(void *item, void *user) {
    TestItem *test_item;
    int32 *scratch;

    test_item = item;
    scratch = user;
    *scratch = test_item->value*2;
    return scratch;
}

static void
test_conversion(void) {
    NcmError error;
    int32 i32;
    uint32 u32;
    double f64;

    ncm_error_clear(&error);
    REQUIRE(ncm_parse_int32(LIT_ARGS(" 42 "), &i32, &error));
    REQUIRE_INT(i32, 42);
    REQUIRE(ncm_parse_uint32(LIT_ARGS("255"), &u32, &error));
    REQUIRE_INT((int32)u32, 255);
    REQUIRE(!ncm_parse_uint32(LIT_ARGS("-1"), &u32, &error));
    REQUIRE(ncm_error_is_set(&error));
    REQUIRE(ncm_parse_double(LIT_ARGS("12.5"), &f64, &error));
    REQUIRE((f64 > 12.4) && (f64 < 12.6));
    REQUIRE(ncm_bounds_check_u64(42, 0, 100, &error));
    REQUIRE(!ncm_bounds_check_u64(101, 0, 100, &error));
    return;
}

static void
test_string_format(void) {
    NcmStringFormatArg args[3];
    NcmBuffer output;

    args[0] = ncm_string_format_arg_string(LIT_ARGS("tracks"));
    args[1] = ncm_string_format_arg_i64(3);
    args[2] = ncm_string_format_arg_bool(true);
    output = ncm_string_format_make(LIT_ARGS("%2% %1% %% %s"),
                                    args, NCM_ARRAY_LEN(args));
    REQUIRE_STRING(output.data, output.len, "3 tracks % tracks");
    ncm_buffer_destroy(&output);
    return;
}

static void
test_wide_string(void) {
    NcmWideString wide;

    ncm_wide_string_init(&wide);
    REQUIRE(ncm_wide_string_from_utf8(&wide, LIT_ARGS("A\xe2\x82\xac")));
    REQUIRE_INT(wide.len, 2);
    REQUIRE(wide.data[0] == L'A');
    REQUIRE(wide.data[1] == (wchar_t)0x20ac);
    ncm_wide_string_destroy(&wide);
    return;
}

static void
test_regex_flags(void) {
    NcmRegex regex;
    NcmError error;

    ncm_regex_init(&regex);
    ncm_error_clear(&error);
    REQUIRE(ncm_regex_compile(&regex, LIT_ARGS("A+B"),
                              NCM_REGEX_LITERAL_CASE_INSENSITIVE,
                              &error));
    REQUIRE(ncm_regex_search(&regex, LIT_ARGS("a+b")));
    REQUIRE(!ncm_regex_search(&regex, LIT_ARGS("axb")));
    ncm_regex_destroy(&regex);
    return;
}

static void
test_functional(void) {
    TestItem items[4];
    int32 threshold;
    int32 amount;

    items[0] = (TestItem){.value = 1};
    items[1] = (TestItem){.value = 3};
    items[2] = (TestItem){.value = 5};
    items[3] = (TestItem){.value = 2};
    threshold = 2;
    amount = 7;

    REQUIRE_INT(ncm_find_map_first(items, NCM_ARRAY_LEN(items),
                                   SIZEOF(items[0]), test_item_gt,
                                   &threshold, test_item_mark,
                                   &amount), 1);
    REQUIRE_INT(items[1].mapped, 7);
    REQUIRE_INT(ncm_find_map_all(items, NCM_ARRAY_LEN(items),
                                 SIZEOF(items[0]), test_item_gt,
                                 &threshold, test_item_mark,
                                 &amount), 2);
    REQUIRE_INT(items[1].mapped, 14);
    REQUIRE_INT(items[2].mapped, 7);
    return;
}

static void
test_scoped_value(void) {
    NcmScopedValue scope;
    bool flag;
    bool temporary;

    ncm_scoped_value_init(&scope);
    flag = true;
    temporary = false;
    REQUIRE(ncm_scoped_value_begin(&scope, &flag, &temporary,
                                   SIZEOF(flag)));
    REQUIRE(!flag);
    ncm_scoped_value_restore(&scope);
    REQUIRE(flag);
    return;
}

static void
test_shared_resource(void) {
    NcmSharedResource shared;
    NcmError error;
    int32 value;
    int32 *locked;

    ncm_error_clear(&error);
    value = 5;
    REQUIRE(ncm_shared_resource_init_borrowed(&shared, &value, &error));
    locked = ncm_shared_resource_acquire(&shared, &error);
    REQUIRE(locked != NULL);
    *locked += 2;
    ncm_shared_resource_release(&shared, &error);
    REQUIRE_INT(value, 7);
    ncm_shared_resource_destroy(&shared);
    return;
}

static void
test_iterators(void) {
    TestItem items[3];
    NcmAnyIterator begin;
    NcmAnyIterator end;
    NcmTransformIterator transform;
    TestItem *item;
    int32 scratch;
    int32 *mapped;

    items[0] = (TestItem){.value = 4};
    items[1] = (TestItem){.value = 8};
    items[2] = (TestItem){.value = 12};

    begin = ncm_any_iterator_begin(items, NCM_ARRAY_LEN(items),
                                   SIZEOF(items[0]));
    end = ncm_any_iterator_end(items, NCM_ARRAY_LEN(items),
                               SIZEOF(items[0]));
    REQUIRE_INT(ncm_any_iterator_distance(&end, &begin), 3);
    item = ncm_any_iterator_deref(&begin);
    REQUIRE_INT(item->value, 4);
    ncm_any_iterator_next(&begin);
    item = ncm_any_iterator_deref(&begin);
    REQUIRE_INT(item->value, 8);

    scratch = 0;
    ncm_transform_iterator_init(&transform, begin, test_item_transform,
                                &scratch);
    mapped = ncm_transform_iterator_deref(&transform);
    REQUIRE(mapped == &scratch);
    REQUIRE_INT(*mapped, 16);
    return;
}

int32
main(void) {
    test_conversion();
    test_string_format();
    test_wide_string();
    test_regex_flags();
    test_functional();
    test_scoped_value();
    test_shared_resource();
    test_iterators();
    return 0;
}
