#if !defined(NCM_LRC_C)
#define NCM_LRC_C

#include "cbase.h"
#include "ncmpcpp2.h"

#include "c/ncm_c.h"

#define NCM_LRC_MAX_LINE_TIMESTAMPS 64

void
lrc_document_clear(LrcDocument *document) {
    if (document == NULL) {
        return;
    }

    str_clear(&document->text);
    ARRAY_CLEAR(document->entries);
    document->offset_ms = 0;
    document->has_offset = false;
    return;
}

static void
lrc_document_destroy_unchecked(LrcDocument *document) {
    str_free(&document->text);
    ARRAY_FREE(document->entries);
    *document = (LrcDocument){0};
    return;
}

void
lrc_document_destroy(LrcDocument *document) {
    if (document == NULL) {
        return;
    }

    lrc_document_destroy_unchecked(document);
    return;
}

static int32
lrc_raw_line_len(char *data, int32 data_len, int32 start) {
    int32 len = 0;

    while (((start + len) < data_len) && (data[start + len] != '\n')) {
        len += 1;
    }

    return len;
}

static int32
lrc_trim_line_end(char *data, int32 data_len) {
    while ((data_len > 0) && (data[data_len - 1] == '\r')) {
        data_len -= 1;
    }
    return data_len;
}

static int
lrc_entry_compare(void *left_ptr, void *right_ptr) {
    LrcEntry *left = left_ptr;
    LrcEntry *right = right_ptr;

    if (left->time_ms < right->time_ms) {
        return -1;
    }
    if (left->time_ms > right->time_ms) {
        return 1;
    }
    if (left->source_order < right->source_order) {
        return -1;
    }
    if (left->source_order > right->source_order) {
        return 1;
    }
    return 0;
}

static int32
lrc_malformed_offset(NcmError *ncm_error) {
    return ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE,
                                STRLIT("malformed LRC offset"));
}

static int32
lrc_malformed_line(NcmError *ncm_error) {
    return ncm_error_set_status(ncm_error, -NCM_ERROR_PARSE,
                                STRLIT("malformed LRC line"));
}

static int32
lrc_next_line_pos(char *data, int32 data_len, int32 pos, int32 raw_line_len) {
    pos += raw_line_len;
    if ((pos < data_len) && (data[pos] == '\n')) {
        pos += 1;
    }
    return pos;
}

static int32
lrc_find_tag_close(char *line, int32 line_len, int32 cursor) {
    int32 close;

    ASSERT(line != NULL);
    ASSERT_NON_NEGATIVE(line_len);
    ASSERT_NON_NEGATIVE(cursor);
    ASSERT_LESS_VAR(cursor, line_len);
    ASSERT_EQ(line[cursor], '[');

    close = cursor + 1;
    while ((close < line_len) && (line[close] != ']')) {
        close += 1;
    }
    if (close >= line_len) {
        return -1;
    }
    return close;
}

static int32
lrc_parse_offset_tag(LrcDocument *document, char *tag, int32 tag_len,
                     NcmError *ncm_error) {
    char *value;
    int32 value_len;
    llong signed_value;
    int32 offset_ms;
    int32 status;

    ASSERT(document != NULL);
    ASSERT(tag != NULL);
    ASSERT_NON_NEGATIVE(tag_len);

    if (tag_len < STRLIT_LEN("offset:")) {
        return 0;
    }

    for (int32 i = 0; i < STRLIT_LEN("offset:"); i += 1) {
        uint8 left = (uint8)tag[i];
        uint8 right = (uint8)"offset:"[i];

        if (tolower(left) != tolower(right)) {
            return 0;
        }
    }

    value = tag + STRLIT_LEN("offset:");
    value_len = tag_len - STRLIT_LEN("offset:");
    status = parse_integer(value, value_len, &signed_value);
    if (status < 0) {
        return lrc_malformed_offset(ncm_error);
    }

    if ((signed_value < MINOF(offset_ms))
        || (signed_value > MAXOF(offset_ms))) {
        return lrc_malformed_offset(ncm_error);
    }

    offset_ms = (int32)signed_value;
    document->offset_ms = offset_ms;
    document->has_offset = true;
    return 1;
}

static int32
lrc_parse_line_offset_tags(LrcDocument *document, char *line, int32 line_len,
                           NcmError *ncm_error) {
    int32 cursor = 0;

    while ((cursor < line_len) && (line[cursor] == '[')) {
        char *tag;
        int32 close;
        int32 tag_len;
        int32 status;

        close = lrc_find_tag_close(line, line_len, cursor);
        if (close < 0) {
            break;
        }

        tag = line + cursor + 1;
        tag_len = close - cursor - 1;
        status = lrc_parse_offset_tag(document, tag, tag_len, ncm_error);
        if (status < 0) {
            return status;
        }

        cursor = close + 1;
    }
    return 0;
}

static int32
lrc_parse_offsets(LrcDocument *document, char *data, int32 data_len,
                  NcmError *ncm_error) {
    int32 pos = 0;

    while (pos < data_len) {
        int32 raw_line_len = lrc_raw_line_len(data, data_len, pos);
        int32 line_len = lrc_trim_line_end(data + pos, raw_line_len);
        char *line = data + pos;
        int32 status;

        status = lrc_parse_line_offset_tags(document, line, line_len,
                                            ncm_error);
        if (status < 0) {
            return status;
        }

        pos = lrc_next_line_pos(data, data_len, pos, raw_line_len);
    }
    return 0;
}

static int32
lrc_parse_time_integer(char *data, int32 data_len, llong *value,
                       NcmError *ncm_error) {
    int32 status;

    status = parse_integer(data, data_len, value);
    if ((status < 0) || (*value < 0)) {
        return lrc_malformed_line(ncm_error);
    }
    return 0;
}

static int32
lrc_parse_time_tag(char *tag, int32 tag_len, int32 offset_ms, int32 *time_ms,
                   NcmError *ncm_error) {
    int32 colon;
    int32 dot;
    int32 frac_len;
    llong minutes;
    llong seconds;
    llong milliseconds;
    llong value;
    int32 status;

    if ((tag_len <= 0) || !isdigit((uint8)tag[0])) {
        return 0;
    }
    if (tag_len < STRLIT_LEN("0:00")) {
        return lrc_malformed_line(ncm_error);
    }

    colon = -1;
    dot = -1;
    for (int32 i = 0; i < tag_len; i += 1) {
        if (tag[i] == ':') {
            if (colon >= 0) {
                return lrc_malformed_line(ncm_error);
            }
            colon = i;
        } else if (tag[i] == '.') {
            if (dot >= 0) {
                return lrc_malformed_line(ncm_error);
            }
            dot = i;
        }
    }

    if ((colon <= 0) || (colon + 2 >= tag_len)) {
        return lrc_malformed_line(ncm_error);
    }
    if ((dot >= 0) && (dot != colon + 3)) {
        return lrc_malformed_line(ncm_error);
    }
    if ((dot < 0) && ((colon + 3) != tag_len)) {
        return lrc_malformed_line(ncm_error);
    }

    status = lrc_parse_time_integer(tag, colon, &minutes, ncm_error);
    if (status < 0) {
        return status;
    }
    status = lrc_parse_time_integer(tag + colon + 1, 2, &seconds, ncm_error);
    if (status < 0) {
        return status;
    }
    if (seconds >= 60) {
        return lrc_malformed_line(ncm_error);
    }

    milliseconds = 0;
    if (dot >= 0) {
        frac_len = tag_len - dot - 1;
        if ((frac_len <= 0) || (frac_len > 3)) {
            return lrc_malformed_line(ncm_error);
        }

        status = lrc_parse_time_integer(tag + dot + 1, frac_len,
                                        &milliseconds, ncm_error);
        if (status < 0) {
            return status;
        }
        if (frac_len == 1) {
            milliseconds *= 100;
        } else if (frac_len == 2) {
            milliseconds *= 10;
        }
    }

    if (minutes > (MAXOF(value) - seconds)/60) {
        return lrc_malformed_line(ncm_error);
    }
    value = minutes*60 + seconds;
    if (value > (MAXOF(value) - milliseconds)/1000) {
        return lrc_malformed_line(ncm_error);
    }
    value = value*1000 + milliseconds;
    if ((offset_ms > 0) && (value > (MAXOF(value) - offset_ms))) {
        return lrc_malformed_line(ncm_error);
    }
    if ((offset_ms < 0) && (value < (MINOF(value) - offset_ms))) {
        return lrc_malformed_line(ncm_error);
    }

    value += offset_ms;
    if ((value < MINOF(*time_ms)) || (value > MAXOF(*time_ms))) {
        return lrc_malformed_line(ncm_error);
    }

    *time_ms = (int32)value;
    return 1;
}

static void
lrc_append_line_entries(LrcDocument *document, int32 *source_order,
                        int32 blank_lines_before, int32 *times,
                        int32 times_len, char *text, int32 text_len) {
    ASSERT(document != NULL);
    ASSERT(source_order != NULL);
    ASSERT(times != NULL);
    ASSERT_NON_NEGATIVE(times_len);

    for (int32 i = 0; i < times_len; i += 1) {
        LrcEntry entry;

        entry.time_ms = times[i];
        entry.text_start = document->text.len;
        entry.text_len = text_len;
        entry.buffer_start = NCM_LRC_NO_BUFFER_POSITION;
        entry.buffer_end = NCM_LRC_NO_BUFFER_POSITION;
        entry.source_order = *source_order;

        if (i == 0) {
            entry.blank_lines_before = blank_lines_before;
        } else {
            entry.blank_lines_before = 0;
        }

        ARRAY_PUSH(document->entries, entry);
        *source_order += 1;
        STR_APPEND(&document->text, text, text_len);
    }

    return;
}

static int32
lrc_parse_line_entries(LrcDocument *document, char *line, int32 line_len,
                       int32 *source_order, int32 blank_lines_before,
                       NcmError *ncm_error) {
    int32 times[NCM_LRC_MAX_LINE_TIMESTAMPS];
    int32 times_len;
    int32 cursor;

    times_len = 0;
    cursor = 0;
    while ((cursor < line_len) && (line[cursor] == '[')) {
        char *tag;
        int32 close;
        int32 tag_len;
        int32 time_ms;
        int32 status;

        close = lrc_find_tag_close(line, line_len, cursor);
        if (close < 0) {
            break;
        }

        tag = line + cursor + 1;
        tag_len = close - cursor - 1;
        status = lrc_parse_time_tag(tag, tag_len, document->offset_ms,
                                    &time_ms, ncm_error);
        if (status < 0) {
            return status;
        }
        if ((status > 0) && (times_len < LENGTH(times))) {
            times[times_len] = time_ms;
            times_len += 1;
        }

        cursor = close + 1;
    }

    if (times_len > 0) {
        lrc_append_line_entries(document, source_order, blank_lines_before,
                                times, times_len, line + cursor,
                                line_len - cursor);
    }
    return 0;
}

static int32
lrc_parse_entries(LrcDocument *document, char *data, int32 data_len,
                  NcmError *ncm_error) {
    int32 source_order = 0;
    int32 blank_lines_before = 0;
    int32 pos = 0;

    while (pos < data_len) {
        int32 raw_line_len = lrc_raw_line_len(data, data_len, pos);
        int32 line_len = lrc_trim_line_end(data + pos, raw_line_len);
        char *line = data + pos;
        int32 status;

        if (line_len <= 0) {
            if (source_order > 0) {
                blank_lines_before += 1;
            }
        } else {
            status = lrc_parse_line_entries(document, line, line_len,
                                            &source_order, blank_lines_before,
                                            ncm_error);
            if (status < 0) {
                return status;
            }
            blank_lines_before = 0;
        }

        pos = lrc_next_line_pos(data, data_len, pos, raw_line_len);
    }
    return 0;
}

int32
lrc_parse(LrcDocument *document, char *data, int32 data_len,
          NcmError *ncm_error) {
    LrcDocument parsed = {0};
    int32 status;

    if (document == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing LRC document"));
    }
    if ((data == NULL) || (data_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing LRC data"));
    }

    status = lrc_parse_offsets(&parsed, data, data_len, ncm_error);
    if (status < 0) {
        lrc_document_destroy_unchecked(&parsed);
        return status;
    }

    status = lrc_parse_entries(&parsed, data, data_len, ncm_error);
    if (status < 0) {
        lrc_document_destroy_unchecked(&parsed);
        return status;
    }

    if (ARRAY_LEN(parsed.entries) <= 0) {
        lrc_document_destroy_unchecked(&parsed);
        return ncm_error_set_code(ncm_error, NCM_ERROR_PARSE,
                                  STRLIT("no synchronized LRC lines"));
    }
    if (ARRAY_LEN(parsed.entries) > 1) {
        qsort64(parsed.entries, ARRAY_LEN(parsed.entries),
                SIZEOF(*parsed.entries), lrc_entry_compare);
    }

    lrc_document_destroy_unchecked(document);
    *document = parsed;
    ncm_error_clear(ncm_error);
    return 0;
}

static StrView
lrc_entry_text_unchecked(LrcDocument *document, LrcEntry *entry) {
    StrView view;

    if (entry->text_len <= 0) {
        return (StrView){.data = ""};
    }

    view.data = document->text.data + entry->text_start;
    view.len = entry->text_len;
    return view;
}

static void
lrc_document_clear_buffer_positions(LrcDocument *document) {
    ASSERT(document != NULL);

    for (int32 i = 0; i < ARRAY_LEN(document->entries); i += 1) {
        document->entries[i].buffer_start = NCM_LRC_NO_BUFFER_POSITION;
        document->entries[i].buffer_end = NCM_LRC_NO_BUFFER_POSITION;
    }

    return;
}

int32
lrc_document_render_plain(LrcDocument *document, LrcRenderTarget *target) {
    char line_break[] = "\n";

    if ((document == NULL) || (target == NULL)) {
        return -EINVAL;
    }
    if ((target->position == NULL) || (target->append == NULL)) {
        return -EINVAL;
    }

    lrc_document_clear_buffer_positions(document);
    for (int32 i = 0; i < ARRAY_LEN(document->entries); i += 1) {
        LrcEntry *entry = &document->entries[i];
        StrView text;

        if (i > 0) {
            target->append(target->user, line_break, STRLIT_LEN("\n"));
            for (int32 j = 0; j < entry->blank_lines_before; j += 1) {
                target->append(target->user, line_break, STRLIT_LEN("\n"));
            }
        }

        text = lrc_entry_text_unchecked(document, entry);

        entry->buffer_start = target->position(target->user);
        target->append(target->user, text.data, text.len);
        entry->buffer_end = target->position(target->user);
    }

    return 0;
}

static int32
lrc_document_next_entry_after_time_unchecked(LrcDocument *document,
                                             int64 elapsed_ms) {
    int32 left = 0;
    int32 right = ARRAY_LEN(document->entries);

    while (left < right) {
        int32 middle = left + (right - left)/2;

        if ((int64)document->entries[middle].time_ms <= elapsed_ms) {
            left = middle + 1;
        } else {
            right = middle;
        }
    }

    if (left >= ARRAY_LEN(document->entries)) {
        return -1;
    }
    return left;
}

int32
lrc_document_entry_at_time(LrcDocument *document, int64 elapsed_ms) {
    int32 next;

    if ((document == NULL) || (ARRAY_LEN(document->entries) <= 0)) {
        return -1;
    }
    if (elapsed_ms < 0) {
        return -1;
    }

    next = lrc_document_next_entry_after_time_unchecked(document, elapsed_ms);
    if (next < 0) {
        return ARRAY_LEN(document->entries) - 1;
    }

    return next - 1;
}

int32
lrc_document_next_entry_after_time(LrcDocument *document,
                                   int64 elapsed_ms) {
    if ((document == NULL) || (ARRAY_LEN(document->entries) <= 0)) {
        return -1;
    }

    return lrc_document_next_entry_after_time_unchecked(document, elapsed_ms);
}

#endif /* NCM_LRC_C */
