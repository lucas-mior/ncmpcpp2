#if !defined(NCM_LRC_C)
#define NCM_LRC_C

#include "cbase.h"

#include "c/ncm_c.h"

#define NCM_LRC_MAX_LINE_TIMESTAMPS 64

void
ncm_lrc_document_clear(NcmLrcDocument *document) {
    if (document == NULL) {
        return;
    }

    sb_clear(&document->text);
    document->entries_len = 0;
    document->offset_ms = 0;
    document->has_offset = false;
    return;
}

void
ncm_lrc_document_destroy(NcmLrcDocument *document) {
    if (document == NULL) {
        return;
    }

    sb_free(&document->text);
    free2(document->entries, document->entries_cap*SIZEOF(*document->entries));
    *document = (NcmLrcDocument){0};
    return;
}

static int32
ncm_lrc_raw_line_len(char *data, int32 data_len, int32 start) {
    int32 len = 0;

    while (((start + len) < data_len) && (data[start + len] != '\n')) {
        len += 1;
    }

    return len;
}

static int32
ncm_lrc_trim_line_end(char *data, int32 data_len) {
    while ((data_len > 0) && (data[data_len - 1] == '\r')) {
        data_len -= 1;
    }
    return data_len;
}

static bool
ncm_lrc_char_is_space(char c) {
    return (c == ' ') || (c == '\t');
}

static bool
ncm_lrc_char_is_digit(char c) {
    return (c >= '0') && (c <= '9');
}

static int32
ncm_lrc_parse_uint(char *data, int32 data_len, int64 *value) {
    int64 result;

    if ((data == NULL) || (data_len <= 0) || (value == NULL)) {
        return -NCM_ERROR_PARSE;
    }

    result = 0;
    for (int32 i = 0; i < data_len; i += 1) {
        int32 digit;

        if (!ncm_lrc_char_is_digit(data[i])) {
            return -NCM_ERROR_PARSE;
        }
        digit = data[i] - '0';
        if (result > (MAXOF(result) - digit)/10) {
            return -NCM_ERROR_PARSE;
        }
        result = result*10 + digit;
    }

    *value = result;
    return 0;
}

static int
ncm_lrc_entry_compare(void *left_ptr, void *right_ptr) {
    NcmLrcEntry *left = left_ptr;
    NcmLrcEntry *right = right_ptr;

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

int32
ncm_lrc_parse(NcmLrcDocument *document,
              char *data, int32 data_len,
              NcmError *ncm_error) {
    NcmLrcDocument parsed = {0};
    int32 source_order;
    int32 raw_line_len;
    int32 line_len;
    int32 blank_lines_before;
    int32 status;
    int32 pos;

    if (document == NULL) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing LRC document"));
    }
    if ((data == NULL) || (data_len <= 0)) {
        return ncm_error_set_status(ncm_error, -EINVAL,
                                    STRLIT("missing LRC data"));
    }

    pos = 0;
    while (pos < data_len) {
        char *line;
        int32 cursor;

        raw_line_len = ncm_lrc_raw_line_len(data, data_len, pos);
        line_len = ncm_lrc_trim_line_end(data + pos, raw_line_len);
        line = data + pos;
        cursor = 0;
        while ((cursor < line_len) && (line[cursor] == '[')) {
            int32 close;
            char *tag;
            int32 tag_len;
            bool matched = false;

            close = cursor + 1;
            while ((close < line_len) && (line[close] != ']')) {
                close += 1;
            }
            if (close >= line_len) {
                break;
            }

            tag = line + cursor + 1;
            tag_len = close - cursor - 1;
            if (tag_len >= STRLIT_LEN("offset:")) {
                matched = true;
                for (int32 i = 0; i < STRLIT_LEN("offset:"); i += 1) {
                    uint8 left = (uint8)tag[i];
                    uint8 right = (uint8)"offset:"[i];

                    if (tolower(left) != tolower(right)) {
                        matched = false;
                        break;
                    }
                }
            }
            if (matched) {
                char *value = tag + STRLIT_LEN("offset:");
                int32 value_len = tag_len - STRLIT_LEN("offset:");
                int32 sign = 1;
                int32 start = 0;
                int64 unsigned_value;
                int64 signed_value;
                int32 offset_ms;

                while ((value_len > 0) && ncm_lrc_char_is_space(*value)) {
                    value += 1;
                    value_len -= 1;
                }
                while ((value_len > 0)
                       && ncm_lrc_char_is_space(value[value_len - 1])) {
                    value_len -= 1;
                }
                if (value_len <= 0) {
                    ncm_lrc_document_destroy(&parsed);
                    return ncm_error_set_status(
                        ncm_error, -NCM_ERROR_PARSE,
                        STRLIT("malformed LRC offset"));
                }

                if ((value[0] == '+') || (value[0] == '-')) {
                    if (value[0] == '-') {
                        sign = -1;
                    }
                    start = 1;
                }
                if (start >= value_len) {
                    ncm_lrc_document_destroy(&parsed);
                    return ncm_error_set_status(
                        ncm_error, -NCM_ERROR_PARSE,
                        STRLIT("malformed LRC offset"));
                }

                status = ncm_lrc_parse_uint(value + start,
                                            value_len - start,
                                            &unsigned_value);
                if (status < 0) {
                    ncm_lrc_document_destroy(&parsed);
                    return ncm_error_set_status(
                        ncm_error, status, STRLIT("malformed LRC offset"));
                }

                signed_value = sign*unsigned_value;
                if ((signed_value < MINOF(offset_ms))
                    || (signed_value > MAXOF(offset_ms))) {
                    ncm_lrc_document_destroy(&parsed);
                    return ncm_error_set_status(
                        ncm_error, -NCM_ERROR_PARSE,
                        STRLIT("malformed LRC offset"));
                }

                offset_ms = (int32)signed_value;
                parsed.offset_ms = offset_ms;
                parsed.has_offset = true;
            }
            cursor = close + 1;
        }

        pos += raw_line_len;
        if ((pos < data_len) && (data[pos] == '\n')) {
            pos += 1;
        }
    }

    source_order = 0;
    blank_lines_before = 0;
    pos = 0;
    while (pos < data_len) {
        raw_line_len = ncm_lrc_raw_line_len(data, data_len, pos);
        line_len = ncm_lrc_trim_line_end(data + pos, raw_line_len);
        if (line_len <= 0) {
            if (source_order > 0) {
                blank_lines_before += 1;
            }
        } else {
            int32 times[NCM_LRC_MAX_LINE_TIMESTAMPS];
            int32 times_len;
            int32 cursor;
            char *text;
            int32 text_len;

            times_len = 0;
            cursor = 0;
            while ((cursor < line_len) && (data[pos + cursor] == '[')) {
                int32 close;
                char *tag;
                int32 tag_len;
                int32 time_ms = 0;
                bool tag_matched = false;

                close = cursor + 1;
                while ((close < line_len)
                       && (data[pos + close] != ']')) {
                    close += 1;
                }
                if (close >= line_len) {
                    break;
                }

                tag = data + pos + cursor + 1;
                tag_len = close - cursor - 1;
                if ((tag_len > 0) && ncm_lrc_char_is_digit(tag[0])) {
                    int32 colon;
                    int32 dot;
                    int32 frac_len;
                    int64 minutes;
                    int64 seconds;
                    int64 milliseconds;
                    int64 value;

                    if (tag_len < STRLIT_LEN("0:00")) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }

                    colon = -1;
                    dot = -1;
                    for (int32 i = 0; i < tag_len; i += 1) {
                        if (tag[i] == ':') {
                            if (colon >= 0) {
                                ncm_lrc_document_destroy(&parsed);
                                return ncm_error_set_status(
                                    ncm_error, -NCM_ERROR_PARSE,
                                    STRLIT("malformed LRC line"));
                            }
                            colon = i;
                        } else if (tag[i] == '.') {
                            if (dot >= 0) {
                                ncm_lrc_document_destroy(&parsed);
                                return ncm_error_set_status(
                                    ncm_error, -NCM_ERROR_PARSE,
                                    STRLIT("malformed LRC line"));
                            }
                            dot = i;
                        }
                    }
                    if ((colon <= 0) || (colon + 2 >= tag_len)) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }
                    if ((dot >= 0) && (dot != colon + 3)) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }
                    if ((dot < 0) && ((colon + 3) != tag_len)) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }

                    status = ncm_lrc_parse_uint(tag, colon, &minutes);
                    if (status < 0) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, status,
                            STRLIT("malformed LRC line"));
                    }
                    status = ncm_lrc_parse_uint(tag + colon + 1, 2,
                                                &seconds);
                    if (status < 0) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, status,
                            STRLIT("malformed LRC line"));
                    }
                    if (seconds >= 60) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }

                    milliseconds = 0;
                    if (dot >= 0) {
                        frac_len = tag_len - dot - 1;
                        if ((frac_len <= 0) || (frac_len > 3)) {
                            ncm_lrc_document_destroy(&parsed);
                            return ncm_error_set_status(
                                ncm_error, -NCM_ERROR_PARSE,
                                STRLIT("malformed LRC line"));
                        }
                        status = ncm_lrc_parse_uint(tag + dot + 1, frac_len,
                                                    &milliseconds);
                        if (status < 0) {
                            ncm_lrc_document_destroy(&parsed);
                            return ncm_error_set_status(
                                ncm_error, status,
                                STRLIT("malformed LRC line"));
                        }
                        if (frac_len == 1) {
                            milliseconds *= 100;
                        } else if (frac_len == 2) {
                            milliseconds *= 10;
                        }
                    }

                    if (minutes > (MAXOF(value) - seconds)/60) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }
                    value = minutes*60 + seconds;
                    if (value > (MAXOF(value) - milliseconds)/1000) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }
                    value = value*1000 + milliseconds;
                    if ((parsed.offset_ms > 0)
                        && (value > (MAXOF(value) - parsed.offset_ms))) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }
                    if ((parsed.offset_ms < 0)
                        && (value < (MINOF(value) - parsed.offset_ms))) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }
                    value += parsed.offset_ms;
                    if ((value < MINOF(time_ms))
                        || (value > MAXOF(time_ms))) {
                        ncm_lrc_document_destroy(&parsed);
                        return ncm_error_set_status(
                            ncm_error, -NCM_ERROR_PARSE,
                            STRLIT("malformed LRC line"));
                    }

                    time_ms = (int32)value;
                    tag_matched = true;
                }
                if (tag_matched && (times_len < LENGTH(times))) {
                    times[times_len] = time_ms;
                    times_len += 1;
                }
                cursor = close + 1;
            }
            if (times_len > 0) {
                text = data + pos + cursor;
                text_len = line_len - cursor;
                for (int32 i = 0; i < times_len; i += 1) {
                    NcmLrcEntry *entry;

                    if (parsed.entries_len >= parsed.entries_cap) {
                        int32 new_cap = parsed.entries_cap;

                        if (new_cap <= 0) {
                            new_cap = 8;
                        } else {
                            new_cap *= 2;
                        }
                        parsed.entries = realloc2(
                            parsed.entries, parsed.entries_cap,
                            new_cap, SIZEOF(*parsed.entries));
                        parsed.entries_cap = new_cap;
                    }

                    entry = &parsed.entries[parsed.entries_len];
                    parsed.entries_len += 1;
                    entry->time_ms = times[i];
                    entry->text_start = parsed.text.len;
                    entry->text_len = text_len;
                    entry->buffer_start = NCM_LRC_NO_BUFFER_POSITION;
                    entry->buffer_end = NCM_LRC_NO_BUFFER_POSITION;
                    entry->source_order = source_order;
                    if (i == 0) {
                        entry->blank_lines_before = blank_lines_before;
                    } else {
                        entry->blank_lines_before = 0;
                    }
                    source_order += 1;
                    SB_APPEND(&parsed.text, text, text_len);
                }
            }
            blank_lines_before = 0;
        }
        pos += raw_line_len;
        if ((pos < data_len) && (data[pos] == '\n')) {
            pos += 1;
        }
    }

    if (parsed.entries_len <= 0) {
        ncm_lrc_document_destroy(&parsed);
        return ncm_error_set_code(ncm_error, NCM_ERROR_PARSE,
                                  STRLIT("no synchronized LRC lines"));
    }
    if (parsed.entries_len > 1) {
        qsort64(parsed.entries,
                parsed.entries_len,
                SIZEOF(*parsed.entries),
                ncm_lrc_entry_compare);
    }

    ncm_lrc_document_destroy(document);
    *document = parsed;
    ncm_error_clear(ncm_error);
    return 0;
}

NcmStringView
ncm_lrc_entry_text(NcmLrcDocument *document, NcmLrcEntry *entry) {
    NcmStringView view = {0};

    if ((document == NULL) || (entry == NULL)) {
        return view;
    }
    if ((entry->text_start < 0) || (entry->text_len < 0)) {
        return view;
    }
    if ((entry->text_start + entry->text_len) > document->text.len) {
        return view;
    }
    if (entry->text_len <= 0) {
        view.data = "";
        view.len = 0;
        return view;
    }
    if (document->text.data == NULL) {
        return view;
    }

    view.data = document->text.data + entry->text_start;
    view.len = entry->text_len;
    return view;
}

static void
ncm_lrc_document_clear_buffer_positions(NcmLrcDocument *document) {
    ASSERT(document != NULL);

    for (int32 i = 0; i < document->entries_len; i += 1) {
        document->entries[i].buffer_start = NCM_LRC_NO_BUFFER_POSITION;
        document->entries[i].buffer_end = NCM_LRC_NO_BUFFER_POSITION;
    }

    return;
}

int32
ncm_lrc_document_render_plain(NcmLrcDocument *document,
                              NcmLrcRenderTarget *target) {
    char line_break[] = "\n";

    if ((document == NULL) || (target == NULL)) {
        return -EINVAL;
    }
    if ((target->position == NULL) || (target->append == NULL)) {
        return -EINVAL;
    }

    ncm_lrc_document_clear_buffer_positions(document);
    for (int32 i = 0; i < document->entries_len; i += 1) {
        NcmLrcEntry *entry;
        NcmStringView text;

        entry = &document->entries[i];
        if (i > 0) {
            target->append(target->user, line_break, STRLIT_LEN("\n"));
            for (int32 j = 0; j < entry->blank_lines_before; j += 1) {
                target->append(target->user, line_break, STRLIT_LEN("\n"));
            }
        }

        text = ncm_lrc_entry_text(document, entry);
        if ((text.len > 0) && (text.data == NULL)) {
            ncm_lrc_document_clear_buffer_positions(document);
            return -EINVAL;
        }

        entry->buffer_start = target->position(target->user);
        target->append(target->user, text.data, text.len);
        entry->buffer_end = target->position(target->user);
    }

    return 0;
}

int32
ncm_lrc_document_entry_at_time(NcmLrcDocument *document,
                               int64 elapsed_ms) {
    int32 next;

    if ((document == NULL) || (document->entries_len <= 0)) {
        return -1;
    }
    if (elapsed_ms < 0) {
        return -1;
    }

    next = ncm_lrc_document_next_entry_after_time(document, elapsed_ms);
    if (next < 0) {
        return document->entries_len - 1;
    }

    return next - 1;
}

int32
ncm_lrc_document_next_entry_after_time(NcmLrcDocument *document,
                                       int64 elapsed_ms) {
    int32 left;
    int32 right;

    if ((document == NULL) || (document->entries_len <= 0)) {
        return -1;
    }

    left = 0;
    right = document->entries_len;
    while (left < right) {
        int32 middle;

        middle = left + (right - left)/2;
        if ((int64)document->entries[middle].time_ms <= elapsed_ms) {
            left = middle + 1;
        } else {
            right = middle;
        }
    }

    if (left >= document->entries_len) {
        return -1;
    }
    return left;
}

#endif /* NCM_LRC_C */
