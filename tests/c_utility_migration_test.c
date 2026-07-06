#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "c/ncm_charset.h"
#include "c/ncm_comparators.h"
#include "c/ncm_macro_utilities.h"
#include "c/ncm_option_parser.h"
#include "cbase/base_macros.h"

static void test_option_parser_line(void);
static void test_yes_no(void);
static void test_comparator(void);
static void test_charset_copy(void);

static void
test_option_parser_line(void) {
    NcmOptionLine parsed;
    char quoted[] = "answer = \"hello world\" ignored";
    char unquoted[] = "name = value  \t";

    assert(ncm_option_parser_parse_line(
        quoted, (int32)strlen(quoted), &parsed));
    assert(parsed.option_len == 6);
    assert(strncmp(parsed.option, "answer", 6) == 0);
    assert(parsed.value_len == 11);
    assert(strncmp(parsed.value, "hello world", 11) == 0);

    assert(ncm_option_parser_parse_line(
        unquoted, (int32)strlen(unquoted), &parsed));
    assert(parsed.option_len == 4);
    assert(strncmp(parsed.option, "name", 4) == 0);
    assert(parsed.value_len == 5);
    assert(strncmp(parsed.value, "value", 5) == 0);
    return;
}

static void
test_yes_no(void) {
    bool value;

    assert(ncm_option_parser_yes_no(STRLIT_ARGS("yes"), &value));
    assert(value);
    assert(ncm_option_parser_yes_no(STRLIT_ARGS("no"), &value));
    assert(!value);
    assert(!ncm_option_parser_yes_no(STRLIT_ARGS("maybe"), &value));
    return;
}

static void
test_comparator(void) {
    assert(ncm_compare_locale_strings(STRLIT_ARGS("10"),
                                      STRLIT_ARGS("2"), false) > 0);
    assert(ncm_compare_locale_strings(STRLIT_ARGS("The Album"),
                                      STRLIT_ARGS("Album"), true) == 0);
    return;
}

static void
test_charset_copy(void) {
    NcmBuffer buffer;

    buffer = ncm_charset_utf8_to_locale(STRLIT_ARGS("abc"));
    assert(buffer.len == 3);
    assert(memcmp(buffer.data, "abc", 3) == 0);
    ncm_buffer_destroy(&buffer);
    return;
}

int
main(void) {
    test_option_parser_line();
    test_yes_no();
    test_comparator();
    test_charset_copy();
    return 0;
}
