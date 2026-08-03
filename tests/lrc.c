#if !defined(NCMPCPP_TESTS_LRC_C)
#define NCMPCPP_TESTS_LRC_C

#define CBASE_IMPLEMENT
#include "cbase.h"

#include "c/ncm_error.c"
#include "c/ncm_lrc.c"

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
    ASSERT_EQUAL(text.len, 0);
    text = ncm_lrc_entry_text(&document, &document.entries[1]);
    ASSERT_EQUAL(text.len, STRLIT_LEN("after blank"));
    ASSERT(memcmp64(text.data, STRLIT("after blank")) == 0);
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
    ASSERT_EQUAL(document.entries_len, 0);

    ncm_error_clear(&error);
    ASSERT(!ncm_lrc_parse(&document,
                          malformed,
                          strlen32(malformed),
                          &error));
    ASSERT(ncm_error_is_set(&error));
    ASSERT_EQUAL(document.entries_len, 0);
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
    lrc_test_rejects_untimed_or_malformed_text();
    exit(EXIT_SUCCESS);
}

#endif /* NCMPCPP_TESTS_LRC_C */
