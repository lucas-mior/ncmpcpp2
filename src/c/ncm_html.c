#if !defined(NCM_HTML_C)
#define NCM_HTML_C

#include "cbase.h"

#include "c/ncm_c.h"

typedef struct HtmlEntity {
    char *from;
    char *to;
    int32 from_len;
    int32 to_len;
} HtmlEntity;

#define HTML_ENTITY(FROM, TO) { \
    .from = FROM, \
    .to = TO, \
    .from_len = SIZEOF(FROM) - 1, \
    .to_len = SIZEOF(TO) - 1, \
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

StrBuilder
ncm_html_unescape_utf8(char *data, int32 data_len) {
    StrBuilder out = {0};
    int32 i;

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
                uint32 value;
                int32 base;
                int32 start;
                bool valid_number;

                entity_len = entity_end - entity_start;
                start = 0;
                base = 10;
                valid_number = entity_len > 0;
                if ((entity_len >= 2)
                    && ((data[entity_start] == 'x')
                        || (data[entity_start] == 'X'))) {
                    start = 1;
                    base = 16;
                }
                if (start >= entity_len) {
                    valid_number = false;
                }

                value = 0;
                for (int32 j = start;
                     valid_number && (j < entity_len); j += 1) {
                    int32 digit;
                    char c = data[entity_start + j];

                    if (base == 16) {
                        if ((c >= '0') && (c <= '9')) {
                            digit = c - '0';
                        } else if ((c >= 'a') && (c <= 'f')) {
                            digit = c - 'a' + 10;
                        } else if ((c >= 'A') && (c <= 'F')) {
                            digit = c - 'A' + 10;
                        } else {
                            digit = -1;
                        }
                    } else if ((c >= '0') && (c <= '9')) {
                        digit = c - '0';
                    } else {
                        digit = -1;
                    }

                    if ((digit < 0)
                        || (value > ((0x10ffffu - (uint32)digit)
                                     / (uint32)base))) {
                        valid_number = false;
                    } else {
                        value = value*(uint32)base + (uint32)digit;
                    }
                }

                if (valid_number) {
                    rune = value;
                    encoded_len = utf8_encode(rune, encoded,
                                              SIZEOF(encoded));
                    if (encoded_len > 0) {
                        SB_APPEND(&out, encoded, encoded_len);
                        i = entity_end + 1;
                        replaced = true;
                    }
                }
            }
        }

        if (!replaced) {
            sb_append_byte(&out, data[i]);
            i += 1;
        }
    }

    return out;
}

StrBuilder
ncm_html_unescape_entities(char *data, int32 data_len) {
    StrBuilder out = {0};
    int32 i;

    i = 0;
    while (i < data_len) {
        bool replaced;

        replaced = false;
        if (data[i] == '&') {
            for (int32 j = 0; j < LENGTH(html_entities); j += 1) {
                HtmlEntity *entity = html_entities + j;

                if (BEGINS_WITH(data + i, data_len - i,
                                entity->from, entity->from_len)) {
                    SB_APPEND(&out, entity->to, entity->to_len);
                    i += entity->from_len;
                    replaced = true;
                    break;
                }
            }
        }

        if (!replaced) {
            sb_append_byte(&out, data[i]);
            i += 1;
        }
    }

    return out;
}

StrBuilder
ncm_html_strip_tags(char *data, int32 data_len) {
    StrBuilder stripped = {0};
    StrBuilder result;
    int32 i;

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
                        sb_append_byte(&stripped, data[i]);
                    }
                    i += 1;
                }
            } else {
                tag_len = tag_end - i + 1;
                if (BEGINS_WITH(data + i, tag_len, "<p ")
                    || STREQUAL(data + i, tag_len, "<p>")
                    || STREQUAL(data + i, tag_len, "</p>")
                    || STREQUAL(data + i, tag_len, "<br>")
                    || STREQUAL(data + i, tag_len, "<br/>")
                    || BEGINS_WITH(data + i, tag_len, "<br ")) {
                    sb_append_byte(&stripped, '\n');
                }
                i = tag_end + 1;
            }
        } else {
            sb_append_byte(&stripped, data[i]);
            i += 1;
        }
    }

    result = ncm_html_unescape_entities(stripped.data, stripped.len);
    sb_free(&stripped);
    return result;
}

#endif /* NCM_HTML_C */
