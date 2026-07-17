#include "c/ncm_html.h"

#include <stdbool.h>

#include "c/ncm_string.h"
#include "cbase/utf8.c"
#include "cbase/base_macros.h"
#include "cbase/util.c"

typedef struct HtmlEntity {
    char *from;
    char *to;
    int32 from_len;
    int32 to_len;
} HtmlEntity;

#define HTML_ENTITY(FROM, TO) { \
    .from = FROM, \
    .to = TO, \
    .from_len = STRLIT_LEN(FROM), \
    .to_len = STRLIT_LEN(TO), \
}

static HtmlEntity html_entities[] = {
    HTML_ENTITY("&apos;", "'"),
    HTML_ENTITY("&amp;", "&"),
    HTML_ENTITY("&gt;", ">"),
    HTML_ENTITY("&lt;", "<"),
    HTML_ENTITY("&nbsp;", " "),
    HTML_ENTITY("&quot;", "\""),
    HTML_ENTITY("&ndash;", "–"),
    HTML_ENTITY("&mdash;", "—"),
};

#undef HTML_ENTITY

static int32
hex_value(char c) {
    int32 result;

    if ((c >= '0') && (c <= '9')) {
        result = c - '0';
    } else if ((c >= 'a') && (c <= 'f')) {
        result = c - 'a' + 10;
    } else if ((c >= 'A') && (c <= 'F')) {
        result = c - 'A' + 10;
    } else {
        result = -1;
    }

    return result;
}

static bool
parse_entity_number(char *data, int32 data_len, uint32 *rune) {
    uint32 value;
    int32 base;
    int32 start;

    if (data_len <= 0) {
        return false;
    }

    start = 0;
    base = 10;
    if ((data_len >= 2) && ((data[0] == 'x') || (data[0] == 'X'))) {
        start = 1;
        base = 16;
    }
    if (start >= data_len) {
        return false;
    }

    value = 0;
    for (int32 i = start; i < data_len; i += 1) {
        int32 digit;

        if (base == 16) {
            digit = hex_value(data[i]);
        } else if ((data[i] >= '0') && (data[i] <= '9')) {
            digit = data[i] - '0';
        } else {
            digit = -1;
        }

        if (digit < 0) {
            return false;
        }
        if (value > ((0x10ffffu - (uint32)digit) / (uint32)base)) {
            return false;
        }
        value = value*(uint32)base + (uint32)digit;
    }

    *rune = value;
    return true;
}

static bool
is_newline_tag(char *tag, int32 tag_len) {
    if (ncm_string_starts_with(tag, tag_len, STRLIT_ARGS("<p "))) {
        return true;
    }
    if (STREQUAL(tag, tag_len, STRLIT_ARGS("<p>"))) {
        return true;
    }
    if (STREQUAL(tag, tag_len, STRLIT_ARGS("</p>"))) {
        return true;
    }
    if (STREQUAL(tag, tag_len, STRLIT_ARGS("<br>"))) {
        return true;
    }
    if (STREQUAL(tag, tag_len, STRLIT_ARGS("<br/>"))) {
        return true;
    }
    if (ncm_string_starts_with(tag, tag_len, STRLIT_ARGS("<br "))) {
        return true;
    }

    return false;
}

NcmBuffer
ncm_html_unescape_utf8(char *data, int32 data_len) {
    NcmBuffer out;
    int32 i;

    ncm_buffer_init(&out);
    i = 0;
    while (i < data_len) {
        int32 entity_start;
        int32 entity_len;
        int32 entity_end;
        uint32 rune;
        char encoded[4];
        int32 encoded_len;
        bool replaced;

        replaced = false;
        if (((i + 3) < data_len)
            && (data[i] == '&')
            && (data[i + 1] == '#')) {
            entity_start = i + 2;
            entity_end = entity_start;
            while ((entity_end < data_len) && (data[entity_end] != ';')) {
                entity_end += 1;
            }

            if (entity_end < data_len) {
                entity_len = entity_end - entity_start;
                if (parse_entity_number(data + entity_start,
                                        entity_len, &rune)) {
                    encoded_len = utf8_encode(rune, encoded,
                                                  (int32)SIZEOF(encoded));
                    if (encoded_len > 0) {
                        ncm_buffer_append(&out, encoded, encoded_len);
                        i = entity_end + 1;
                        replaced = true;
                    }
                }
            }
        }

        if (!replaced) {
            ncm_buffer_append_byte(&out, data[i]);
            i += 1;
        }
    }

    return out;
}

NcmBuffer
ncm_html_unescape_entities(char *data, int32 data_len) {
    NcmBuffer out;
    int32 i;

    ncm_buffer_init(&out);
    i = 0;
    while (i < data_len) {
        bool replaced;

        replaced = false;
        if (data[i] == '&') {
            for (int32 j = 0; j < (int32)LENGTH(html_entities); j += 1) {
                HtmlEntity *entity = html_entities + j;

                if (ncm_string_starts_with(data + i, data_len - i,
                                           entity->from, entity->from_len)) {
                    ncm_buffer_append(&out, entity->to, entity->to_len);
                    i += entity->from_len;
                    replaced = true;
                    break;
                }
            }
        }

        if (!replaced) {
            ncm_buffer_append_byte(&out, data[i]);
            i += 1;
        }
    }

    return out;
}

NcmBuffer
ncm_html_strip_tags(char *data, int32 data_len) {
    NcmBuffer stripped;
    NcmBuffer result;
    int32 i;

    ncm_buffer_init(&stripped);
    i = 0;
    while (i < data_len) {
        int32 tag_end;
        int32 tag_len;

        if ((data[i] == '\n') || (data[i] == '\r')) {
            i += 1;
        } else if (data[i] == '<') {
            tag_end = i + 1;
            while ((tag_end < data_len) && (data[tag_end] != '>')) {
                tag_end += 1;
            }

            if (tag_end >= data_len) {
                while (i < data_len) {
                    if ((data[i] != '\n') && (data[i] != '\r')) {
                        ncm_buffer_append_byte(&stripped, data[i]);
                    }
                    i += 1;
                }
            } else {
                tag_len = tag_end - i + 1;
                if (is_newline_tag(data + i, tag_len)) {
                    ncm_buffer_append_byte(&stripped, '\n');
                }
                i = tag_end + 1;
            }
        } else {
            ncm_buffer_append_byte(&stripped, data[i]);
            i += 1;
        }
    }

    result = ncm_html_unescape_entities(stripped.data, stripped.len);
    ncm_buffer_destroy(&stripped);
    return result;
}
