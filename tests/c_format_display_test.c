#include "c/ncm_format.h"

#include <stdio.h>
#include <string.h>

#include "c/ncm_base.h"
#include "cbase/base_macros.h"

static int
check_string(char *name, char *actual, int32 actual_len, char *expected) {
    int32 expected_len;

    expected_len = (int32)strlen(expected);
    if ((actual_len != expected_len)
        || (memcmp(actual, expected, (size_t)expected_len) != 0)) {
        fprintf(stderr, "%s: expected '%s', got '%.*s'\n",
                name, expected, actual_len, actual);
        return 1;
    }
    return 0;
}

static int
test_format_parse_and_render_literal(void) {
    NcmFormatAst ast;
    NcmBuffer rendered;
    NcmError error;
    int failed;

    ncm_format_ast_init(&ast);
    ncm_error_clear(&error);
    if (!ncm_format_parse(&ast, STRLIT_ARGS("abc $$ %%"),
                          NCM_FORMAT_FLAG_ALL, &error)) {
        fprintf(stderr, "parse failed: %s\n", error.message);
        return 1;
    }

    rendered = ncm_format_render_string(&ast, NULL);
    failed = check_string("literal", rendered.data, rendered.len,
                          "abc $ %");
    ncm_buffer_destroy(&rendered);
    ncm_format_ast_destroy(&ast);
    return failed;
}

static int
test_format_columns_builder(void) {
    NcmFormatAst ast;
    enum NcmSongGetter getters[2];
    NcmBuffer rendered;
    int failed;

    ncm_format_ast_init(&ast);
    getters[0] = NCM_SONG_GETTER_ARTIST;
    getters[1] = NCM_SONG_GETTER_TITLE;
    ncm_format_ast_append_first_of_getters(&ast, getters, 2);
    ncm_format_ast_append_text(&ast, STRLIT_ARGS(" tail"));

    rendered = ncm_format_render_string(&ast, NULL);
    failed = check_string("missing song", rendered.data, rendered.len,
                          " tail");
    ncm_buffer_destroy(&rendered);
    ncm_format_ast_destroy(&ast);
    return failed;
}

int
main(void) {
    int failed;

    failed = 0;
    failed += test_format_parse_and_render_literal();
    failed += test_format_columns_builder();
    return failed == 0 ? 0 : 1;
}
