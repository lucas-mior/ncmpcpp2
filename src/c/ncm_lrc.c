#if !defined(NCM_LRC_C)
#define NCM_LRC_C

#include "cbase.h"

#include "c/ncm_base.h"
#include "c/ncm_lrc.h"

#define NCM_LRC_MAX_LINE_TIMESTAMPS 64

static bool ncm_lrc_char_is_digit(char c);
static bool ncm_lrc_char_is_space(char c);
static bool ncm_lrc_parse_uint(char *data, int32 data_len,
                               int64 *value);
static bool ncm_lrc_parse_offset_tag(char *tag, int32 tag_len,
                                     int32 *offset_ms);
static bool ncm_lrc_parse_timestamp_tag(char *tag, int32 tag_len,
                                        int32 offset_ms,
                                        int32 *time_ms);
static int32 ncm_lrc_trim_line_end(char *data, int32 data_len);
static int32 ncm_lrc_raw_line_len(char *data, int32 data_len,
                                  int32 start);
static void ncm_lrc_parse_offset_tags(NcmLrcDocument *document,
                                      char *data, int32 data_len);
static bool ncm_lrc_parse_line(NcmLrcDocument *document,
                               char *line, int32 line_len,
                               int32 *source_order);
static NcmLrcEntry *ncm_lrc_document_append_entry(
    NcmLrcDocument *document);
static void ncm_lrc_document_clear_buffer_positions(
    NcmLrcDocument *document);
static int ncm_lrc_entry_compare(void *left_ptr, void *right_ptr);

void
ncm_lrc_document_init(NcmLrcDocument *document) {
    sb_init(&document->text);
    document->entries = NULL;
    document->entries_len = 0;
    document->entries_cap = 0;
    document->offset_ms = 0;
    document->has_offset = false;
    return;
}

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
    if (document->entries) {
        free2(document->entries,
              document->entries_cap*SIZEOF(*document->entries));
    }
    ncm_lrc_document_init(document);
    return;
}

bool
ncm_lrc_parse(NcmLrcDocument *document,
              char *data, int32 data_len,
              NcmError *ncm_error) {
    NcmLrcDocument parsed;
    int32 source_order;
    int32 raw_line_len;
    int32 line_len;
    int32 pos;

    if (document == NULL) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing LRC document"));
        return false;
    }
    if ((data == NULL) || (data_len <= 0)) {
        ncm_error_set(ncm_error, EINVAL, STRLIT("missing LRC data"));
        return false;
    }

    ncm_lrc_document_init(&parsed);
    ncm_lrc_parse_offset_tags(&parsed, data, data_len);

    source_order = 0;
    pos = 0;
    while (pos < data_len) {
        raw_line_len = ncm_lrc_raw_line_len(data, data_len, pos);
        line_len = ncm_lrc_trim_line_end(data + pos, raw_line_len);
        (void)ncm_lrc_parse_line(&parsed,
                                 data + pos,
                                 line_len,
                                 &source_order);
        pos += raw_line_len;
        if ((pos < data_len) && (data[pos] == '\n')) {
            pos += 1;
        }
    }

    if (parsed.entries_len <= 0) {
        ncm_lrc_document_destroy(&parsed);
        ncm_error_set(ncm_error, EINVAL, STRLIT("no synchronized LRC lines"));
        return false;
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
    return true;
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

bool
ncm_lrc_document_render_plain(NcmLrcDocument *document,
                              NcmLrcRenderTarget *target) {
    char line_break[] = "\n";

    if ((document == NULL) || (target == NULL)) {
        return false;
    }
    if ((target->position == NULL) || (target->append == NULL)) {
        return false;
    }

    ncm_lrc_document_clear_buffer_positions(document);
    for (int32 i = 0; i < document->entries_len; i += 1) {
        NcmLrcEntry *entry;
        NcmStringView text;

        if (i > 0) {
            target->append(target->user, line_break, STRLIT_LEN("\n"));
        }

        entry = &document->entries[i];
        text = ncm_lrc_entry_text(document, entry);
        if ((text.len > 0) && (text.data == NULL)) {
            ncm_lrc_document_clear_buffer_positions(document);
            return false;
        }

        entry->buffer_start = target->position(target->user);
        target->append(target->user, text.data, text.len);
        entry->buffer_end = target->position(target->user);
    }

    return true;
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

static bool
ncm_lrc_char_is_digit(char c) {
    return (c >= '0') && (c <= '9');
}

static bool
ncm_lrc_char_is_space(char c) {
    return (c == ' ') || (c == '\t');
}

static bool
ncm_lrc_parse_uint(char *data, int32 data_len, int64 *value) {
    int64 result;

    if ((data == NULL) || (data_len <= 0) || (value == NULL)) {
        return false;
    }

    result = 0;
    for (int32 i = 0; i < data_len; i += 1) {
        int32 digit;

        if (!ncm_lrc_char_is_digit(data[i])) {
            return false;
        }
        digit = data[i] - '0';
        if (result > (MAXOF(result) - digit)/10) {
            return false;
        }
        result = result*10 + digit;
    }

    *value = result;
    return true;
}

static bool
ncm_lrc_tag_begins_with(char *tag, int32 tag_len, char *prefix,
                        int32 prefix_len) {
    if ((tag == NULL) || (tag_len < prefix_len)) {
        return false;
    }

    for (int32 i = 0; i < prefix_len; i += 1) {
        uint8 left;
        uint8 right;

        left = (uint8)tag[i];
        right = (uint8)prefix[i];
        if (tolower(left) != tolower(right)) {
            return false;
        }
    }
    return true;
}

static bool
ncm_lrc_parse_signed_ms(char *data, int32 data_len, int32 *value) {
    int32 sign;
    int32 start;
    int64 unsigned_value;
    int64 signed_value;

    if ((data == NULL) || (data_len <= 0) || (value == NULL)) {
        return false;
    }

    sign = 1;
    start = 0;
    if ((data[0] == '+') || (data[0] == '-')) {
        if (data[0] == '-') {
            sign = -1;
        }
        start = 1;
    }
    if (start >= data_len) {
        return false;
    }
    if (!ncm_lrc_parse_uint(data + start,
                            data_len - start,
                            &unsigned_value)) {
        return false;
    }

    signed_value = sign*unsigned_value;
    if ((signed_value < MINOF(*value)) || (signed_value > MAXOF(*value))) {
        return false;
    }

    *value = (int32)signed_value;
    return true;
}

static bool
ncm_lrc_parse_offset_tag(char *tag, int32 tag_len, int32 *offset_ms) {
    char *value;
    int32 value_len;

    if (!ncm_lrc_tag_begins_with(tag, tag_len, STRLIT("offset:"))) {
        return false;
    }

    value = tag + STRLIT_LEN("offset:");
    value_len = tag_len - STRLIT_LEN("offset:");
    while ((value_len > 0) && ncm_lrc_char_is_space(*value)) {
        value += 1;
        value_len -= 1;
    }
    while ((value_len > 0) && ncm_lrc_char_is_space(value[value_len - 1])) {
        value_len -= 1;
    }

    return ncm_lrc_parse_signed_ms(value, value_len, offset_ms);
}

static bool
ncm_lrc_parse_timestamp_tag(char *tag, int32 tag_len,
                            int32 offset_ms, int32 *time_ms) {
    int32 colon;
    int32 dot;
    int32 frac_len;
    int64 minutes;
    int64 seconds;
    int64 milliseconds;
    int64 value;

    if ((tag == NULL) || (tag_len < STRLIT_LEN("0:00"))) {
        return false;
    }

    colon = -1;
    dot = -1;
    for (int32 i = 0; i < tag_len; i += 1) {
        if (tag[i] == ':') {
            if (colon >= 0) {
                return false;
            }
            colon = i;
        } else if (tag[i] == '.') {
            if (dot >= 0) {
                return false;
            }
            dot = i;
        }
    }
    if ((colon <= 0) || (colon + 2 >= tag_len)) {
        return false;
    }
    if ((dot >= 0) && (dot != colon + 3)) {
        return false;
    }
    if ((dot < 0) && ((colon + 3) != tag_len)) {
        return false;
    }
    if (!ncm_lrc_parse_uint(tag, colon, &minutes)) {
        return false;
    }
    if (!ncm_lrc_parse_uint(tag + colon + 1, 2, &seconds)) {
        return false;
    }
    if (seconds >= 60) {
        return false;
    }

    milliseconds = 0;
    if (dot >= 0) {
        frac_len = tag_len - dot - 1;
        if ((frac_len <= 0) || (frac_len > 3)) {
            return false;
        }
        if (!ncm_lrc_parse_uint(tag + dot + 1, frac_len, &milliseconds)) {
            return false;
        }
        if (frac_len == 1) {
            milliseconds *= 100;
        } else if (frac_len == 2) {
            milliseconds *= 10;
        }
    }

    if (minutes > (MAXOF(value) - seconds)/60) {
        return false;
    }
    value = minutes*60 + seconds;
    if (value > (MAXOF(value) - milliseconds)/1000) {
        return false;
    }
    value = value*1000 + milliseconds;
    if ((offset_ms > 0) && (value > (MAXOF(value) - offset_ms))) {
        return false;
    }
    if ((offset_ms < 0) && (value < (MINOF(value) - offset_ms))) {
        return false;
    }
    value += offset_ms;
    if ((value < MINOF(*time_ms)) || (value > MAXOF(*time_ms))) {
        return false;
    }

    *time_ms = (int32)value;
    return true;
}

static int32
ncm_lrc_trim_line_end(char *data, int32 data_len) {
    while ((data_len > 0) && (data[data_len - 1] == '\r')) {
        data_len -= 1;
    }
    return data_len;
}

static int32
ncm_lrc_raw_line_len(char *data, int32 data_len, int32 start) {
    int32 len;

    len = 0;
    while (((start + len) < data_len) && (data[start + len] != '\n')) {
        len += 1;
    }
    return len;
}

static void
ncm_lrc_parse_offset_tags(NcmLrcDocument *document,
                          char *data, int32 data_len) {
    int32 raw_line_len;
    int32 line_len;
    int32 pos;

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

            close = cursor + 1;
            while ((close < line_len) && (line[close] != ']')) {
                close += 1;
            }
            if (close >= line_len) {
                break;
            }
            if (ncm_lrc_parse_offset_tag(line + cursor + 1,
                                         close - cursor - 1,
                                         &document->offset_ms)) {
                document->has_offset = true;
            }
            cursor = close + 1;
        }

        pos += raw_line_len;
        if ((pos < data_len) && (data[pos] == '\n')) {
            pos += 1;
        }
    }

    return;
}

static bool
ncm_lrc_parse_line(NcmLrcDocument *document,
                   char *line, int32 line_len,
                   int32 *source_order) {
    int32 times[NCM_LRC_MAX_LINE_TIMESTAMPS];
    int32 times_len;
    int32 cursor;
    char *text;
    int32 text_len;
    bool saw_tag;

    times_len = 0;
    cursor = 0;
    saw_tag = false;
    while ((cursor < line_len) && (line[cursor] == '[')) {
        int32 close;
        int32 time_ms;

        close = cursor + 1;
        while ((close < line_len) && (line[close] != ']')) {
            close += 1;
        }
        if (close >= line_len) {
            break;
        }

        saw_tag = true;
        if (ncm_lrc_parse_timestamp_tag(line + cursor + 1,
                                        close - cursor - 1,
                                        document->offset_ms,
                                        &time_ms)) {
            if (times_len < LENGTH(times)) {
                times[times_len] = time_ms;
                times_len += 1;
            }
        }
        cursor = close + 1;
    }
    if ((times_len <= 0) || !saw_tag) {
        return false;
    }

    text = line + cursor;
    text_len = line_len - cursor;
    for (int32 i = 0; i < times_len; i += 1) {
        NcmLrcEntry *entry;

        entry = ncm_lrc_document_append_entry(document);
        entry->time_ms = times[i];
        entry->text_start = document->text.len;
        entry->text_len = text_len;
        entry->buffer_start = NCM_LRC_NO_BUFFER_POSITION;
        entry->buffer_end = NCM_LRC_NO_BUFFER_POSITION;
        entry->source_order = *source_order;
        *source_order += 1;
        SB_APPEND(&document->text, text, text_len);
    }

    return true;
}

static NcmLrcEntry *
ncm_lrc_document_append_entry(NcmLrcDocument *document) {
    int32 new_cap;
    NcmLrcEntry *entry;

    if (document->entries_len >= document->entries_cap) {
        new_cap = document->entries_cap;
        if (new_cap <= 0) {
            new_cap = 8;
        } else {
            new_cap *= 2;
        }
        document->entries = realloc2(document->entries,
                                     document->entries_cap,
                                     new_cap,
                                     SIZEOF(*document->entries));
        document->entries_cap = new_cap;
    }

    entry = &document->entries[document->entries_len];
    document->entries_len += 1;
    return entry;
}

static void
ncm_lrc_document_clear_buffer_positions(NcmLrcDocument *document) {
    if (document == NULL) {
        return;
    }

    for (int32 i = 0; i < document->entries_len; i += 1) {
        document->entries[i].buffer_start = NCM_LRC_NO_BUFFER_POSITION;
        document->entries[i].buffer_end = NCM_LRC_NO_BUFFER_POSITION;
    }

    return;
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

#endif /* NCM_LRC_C */
