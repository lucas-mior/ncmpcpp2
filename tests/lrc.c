#if !defined(NCMPCPP_TESTS_LRC_C)
#define NCMPCPP_TESTS_LRC_C

#define CBASE_IMPLEMENT
#include "cbase.h"

#include "c/ncm_error.c"
#include "c/ncm_lrc.c"

typedef struct LrcTestRenderTarget {
    StrBuilder text;
} LrcTestRenderTarget;

static int32
lrc_test_render_position(void *user) {
    LrcTestRenderTarget *target = user;

    return target->text.len;
}

static void
lrc_test_render_append(void *user, char *data, int32 data_len) {
    LrcTestRenderTarget *target = user;

    SB_APPEND(&target->text, data, data_len);
    return;
}

static void
lrc_test_parse_simple_lines(void) {
    NcmLrcDocument document;
    NcmStringView text;
    NcmError error = {0};
    char data[] = "[00:15.60]Come on come on\n"
                  "[00:16.300]I see no changes\n";

    ncm_lrc_document_init(&document);
    ASSERT(ncm_lrc_parse(&document, data, strlen32(data), &error));
    ASSERT_EQUAL(document.entries_len, 2);
    ASSERT_EQUAL(document.entries[0].time_ms, 15600);
    ASSERT_EQUAL(document.entries[1].time_ms, 16300);

    text = ncm_lrc_entry_text(&document, &document.entries[0]);
    ASSERT_EQUAL(text.len, STRLIT_LEN("Come on come on"));
    ASSERT(memcmp64(text.data, STRLIT("Come on come on")) == 0);

    text = ncm_lrc_entry_text(&document, &document.entries[1]);
    ASSERT_EQUAL(text.len, STRLIT_LEN("I see no changes"));
    ASSERT(memcmp64(text.data, STRLIT("I see no changes")) == 0);
    ncm_lrc_document_destroy(&document);
    return;
}

static void
lrc_test_repeated_timestamps_share_line_text(void) {
    NcmLrcDocument document;
    NcmStringView first_text;
    NcmStringView second_text;
    NcmError error = {0};
    char data[] = "[00:12.00][00:34.50]chorus\n";

    ncm_lrc_document_init(&document);
    ASSERT(ncm_lrc_parse(&document, data, strlen32(data), &error));
    ASSERT_EQUAL(document.entries_len, 2);
    ASSERT_EQUAL(document.entries[0].time_ms, 12000);
    ASSERT_EQUAL(document.entries[1].time_ms, 34500);

    first_text = ncm_lrc_entry_text(&document, &document.entries[0]);
    second_text = ncm_lrc_entry_text(&document, &document.entries[1]);
    ASSERT_EQUAL(first_text.len, STRLIT_LEN("chorus"));
    ASSERT_EQUAL(second_text.len, STRLIT_LEN("chorus"));
    ASSERT(memcmp64(first_text.data, STRLIT("chorus")) == 0);
    ASSERT(memcmp64(second_text.data, STRLIT("chorus")) == 0);
    ncm_lrc_document_destroy(&document);
    return;
}

static void
lrc_test_ignores_metadata_and_applies_offset(void) {
    NcmLrcDocument document;
    NcmStringView text;
    NcmError error = {0};
    char data[] = "[ar:Example Artist]\n"
                  "[ti:Example Title]\n"
                  "[offset:+250]\n"
                  "[00:01.00]one\n"
                  "[00:02.000]two\n";

    ncm_lrc_document_init(&document);
    ASSERT(ncm_lrc_parse(&document, data, strlen32(data), &error));
    ASSERT(document.has_offset);
    ASSERT_EQUAL(document.offset_ms, 250);
    ASSERT_EQUAL(document.entries_len, 2);
    ASSERT_EQUAL(document.entries[0].time_ms, 1250);
    ASSERT_EQUAL(document.entries[1].time_ms, 2250);

    text = ncm_lrc_entry_text(&document, &document.entries[0]);
    ASSERT_EQUAL(text.len, STRLIT_LEN("one"));
    ASSERT(memcmp64(text.data, STRLIT("one")) == 0);
    ncm_lrc_document_destroy(&document);
    return;
}

static void
lrc_test_negative_offset_is_allowed(void) {
    NcmLrcDocument document;
    NcmError error = {0};
    char data[] = "[offset:-750]\n[00:01.00]early\n";

    ncm_lrc_document_init(&document);
    ASSERT(ncm_lrc_parse(&document, data, strlen32(data), &error));
    ASSERT(document.has_offset);
    ASSERT_EQUAL(document.offset_ms, -750);
    ASSERT_EQUAL(document.entries_len, 1);
    ASSERT_EQUAL(document.entries[0].time_ms, 250);
    ncm_lrc_document_destroy(&document);
    return;
}

static void
lrc_test_sorts_entries_with_stable_equal_times(void) {
    NcmLrcDocument document;
    NcmStringView text;
    NcmError error = {0};
    char data[] = "[00:10.00]ten\n"
                  "[00:05.00]five\n"
                  "[00:05.00]five again\n";

    ncm_lrc_document_init(&document);
    ASSERT(ncm_lrc_parse(&document, data, strlen32(data), &error));
    ASSERT_EQUAL(document.entries_len, 3);
    ASSERT_EQUAL(document.entries[0].time_ms, 5000);
    ASSERT_EQUAL(document.entries[1].time_ms, 5000);
    ASSERT_EQUAL(document.entries[2].time_ms, 10000);

    text = ncm_lrc_entry_text(&document, &document.entries[0]);
    ASSERT(memcmp64(text.data, STRLIT("five")) == 0);
    text = ncm_lrc_entry_text(&document, &document.entries[1]);
    ASSERT(memcmp64(text.data, STRLIT("five again")) == 0);
    text = ncm_lrc_entry_text(&document, &document.entries[2]);
    ASSERT(memcmp64(text.data, STRLIT("ten")) == 0);
    ncm_lrc_document_destroy(&document);
    return;
}

static void
lrc_test_preserves_blank_lyric_lines(void) {
    NcmLrcDocument document;
    NcmStringView text;
    NcmError error = {0};
    char data[] = "[00:00.50]\n[00:01.00]after blank\r\n";

    ncm_lrc_document_init(&document);
    ASSERT(ncm_lrc_parse(&document, data, strlen32(data), &error));
    ASSERT_EQUAL(document.entries_len, 2);

    text = ncm_lrc_entry_text(&document, &document.entries[0]);
    ASSERT_ZERO(text.len);
    text = ncm_lrc_entry_text(&document, &document.entries[1]);
    ASSERT_EQUAL(text.len, STRLIT_LEN("after blank"));
    ASSERT(memcmp64(text.data, STRLIT("after blank")) == 0);
    ncm_lrc_document_destroy(&document);
    return;
}

static void
lrc_test_renders_plain_text_and_buffer_ranges(void) {
    LrcTestRenderTarget target = {0};
    NcmLrcRenderTarget render_target = {0};
    NcmLrcDocument document;
    NcmError error = {0};
    char data[] = "[00:03.00]three\n"
                  "[00:01.00]one\n"
                  "[00:02.00]two\n"
                  "[00:04.00]\n";

    ncm_lrc_document_init(&document);
    ASSERT(ncm_lrc_parse(&document, data, strlen32(data), &error));

    render_target.user = &target;
    render_target.position = lrc_test_render_position;
    render_target.append = lrc_test_render_append;
    ASSERT(ncm_lrc_document_render_plain(&document, &render_target));

    ASSERT_EQUAL(target.text.len, STRLIT_LEN("one\ntwo\nthree\n"));
    ASSERT(memcmp64(target.text.data, STRLIT("one\ntwo\nthree\n")) == 0);
    ASSERT_ZERO(document.entries[0].buffer_start);
    ASSERT_EQUAL(document.entries[0].buffer_end, 3);
    ASSERT_EQUAL(document.entries[1].buffer_start, 4);
    ASSERT_EQUAL(document.entries[1].buffer_end, 7);
    ASSERT_EQUAL(document.entries[2].buffer_start, 8);
    ASSERT_EQUAL(document.entries[2].buffer_end, 13);
    ASSERT_EQUAL(document.entries[3].buffer_start, 14);
    ASSERT_EQUAL(document.entries[3].buffer_end, 14);

    sb_free(&target.text);
    ncm_lrc_document_destroy(&document);
    return;
}

static void
lrc_test_finds_active_entry_at_time(void) {
    NcmLrcDocument document;
    NcmError error = {0};
    char data[] = "[00:10.00]ten\n"
                  "[00:05.00]five\n"
                  "[00:05.00]five again\n"
                  "[00:12.50]later\n";

    ncm_lrc_document_init(&document);
    ASSERT(ncm_lrc_parse(&document, data, strlen32(data), &error));

    ASSERT_EQUAL(ncm_lrc_document_entry_at_time(&document, -1), -1);
    ASSERT_EQUAL(ncm_lrc_document_entry_at_time(&document, 4999), -1);
    ASSERT_EQUAL(ncm_lrc_document_entry_at_time(&document, 5000), 1);
    ASSERT_EQUAL(ncm_lrc_document_entry_at_time(&document, 9999), 1);
    ASSERT_EQUAL(ncm_lrc_document_entry_at_time(&document, 10000), 2);
    ASSERT_EQUAL(ncm_lrc_document_entry_at_time(&document, 12500), 3);
    ASSERT_EQUAL(ncm_lrc_document_entry_at_time(&document, 20000), 3);

    ASSERT_ZERO(ncm_lrc_document_next_entry_after_time(&document, -1));
    ASSERT_ZERO(ncm_lrc_document_next_entry_after_time(&document, 4999));
    ASSERT_EQUAL(ncm_lrc_document_next_entry_after_time(&document, 5000), 2);
    ASSERT_EQUAL(ncm_lrc_document_next_entry_after_time(&document, 9999), 2);
    ASSERT_EQUAL(ncm_lrc_document_next_entry_after_time(&document, 10000), 3);
    ASSERT_EQUAL(ncm_lrc_document_next_entry_after_time(&document, 12500), -1);
    ASSERT_EQUAL(ncm_lrc_document_next_entry_after_time(&document, 20000), -1);

    ncm_lrc_document_destroy(&document);
    return;
}

static void
lrc_test_rejects_untimed_or_malformed_text(void) {
    NcmLrcDocument document;
    NcmError error = {0};
    char untimed[] = "plain text\nmore text\n";
    char malformed[] = "[00:61.00]bad seconds\n[00:01.0000]bad ms\n";

    ncm_lrc_document_init(&document);
    ASSERT(!ncm_lrc_parse(&document, untimed, strlen32(untimed), &error));
    ASSERT(ncm_error_is_set(&error));
    ASSERT_ZERO(document.entries_len);

    ncm_error_clear(&error);
    ASSERT(!ncm_lrc_parse(&document,
                          malformed,
                          strlen32(malformed),
                          &error));
    ASSERT(ncm_error_is_set(&error));
    ASSERT_ZERO(document.entries_len);
    ncm_lrc_document_destroy(&document);
    return;
}

int
main(void) {
    lrc_test_parse_simple_lines();
    lrc_test_repeated_timestamps_share_line_text();
    lrc_test_ignores_metadata_and_applies_offset();
    lrc_test_negative_offset_is_allowed();
    lrc_test_sorts_entries_with_stable_equal_times();
    lrc_test_preserves_blank_lyric_lines();
    lrc_test_renders_plain_text_and_buffer_ranges();
    lrc_test_finds_active_entry_at_time();
    lrc_test_rejects_untimed_or_malformed_text();
    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_TESTS_LRC_C */
