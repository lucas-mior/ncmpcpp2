#if !defined(NCMPCPP_NC_BUFFER_C)
#define NCMPCPP_NC_BUFFER_C

#include "cbase.h"

#include "curses/nc_curses.h"

static void
nc_buffer_reserve(NcBuffer *buffer, int32 extra) {
    int32 needed;
    int32 new_cap;
    int32 old_cap;

    needed = buffer->len + extra + 1;
    if (buffer->data && (needed <= buffer->cap)) {
        return;
    }

    old_cap = buffer->cap;
    new_cap = buffer->cap;
    if (new_cap <= 0) {
        new_cap = 16;
    }
    while (new_cap < needed) {
        new_cap *= 2;
    }

    buffer->data = realloc2(buffer->data,
                            old_cap, new_cap, SIZEOF(*buffer->data));
    buffer->cap = new_cap;
    if (buffer->len <= 0) {
        buffer->data[0] = '\0';
    }
    return;
}

static void
nc_buffer_add_property(NcBuffer *buffer, NcBufferProperty *property) {
    int32 i = ARRAY_LEN(buffer->properties);
    ARRAY_PUSH(buffer->properties, *property);
    while ((i > 0)
            && (buffer->properties[i - 1].position > property->position)) {
        buffer->properties[i] = buffer->properties[i - 1];
        i -= 1;
    }
    buffer->properties[i] = *property;
    return;
}

static void
nc_buffer_property_destroy(NcBufferProperty *property) {
    switch (property->type) {
    case NC_BUFFER_PROPERTY_COLOR:
    case NC_BUFFER_PROPERTY_FORMAT:
        break;
    case NC_BUFFER_PROPERTY_FORMATTED_COLOR:
    case NC_BUFFER_PROPERTY_FORMATTED_COLOR_END:
        nc_formatted_color_destroy(&property->value.formatted_color);
        break;
    case NC_BUFFER_PROPERTY_COUNT:
    default:
        break;
    }
    return;
}

void
nc_buffer_copy(NcBuffer *dest, NcBuffer *source) {
    *dest = (NcBuffer){0};
    nc_buffer_append_data(dest, source->data, source->len);
    for (int32 i = 0; i < ARRAY_LEN(source->properties); i += 1) {
        NcBufferProperty *source_property = &source->properties[i];
        NcBufferProperty property = {0};

        property.id = source_property->id;
        property.position = source_property->position;
        property.type = source_property->type;

        switch (source_property->type) {
        case NC_BUFFER_PROPERTY_COLOR:
            property.value.color = source_property->value.color;
            break;
        case NC_BUFFER_PROPERTY_FORMAT:
            property.value.format = source_property->value.format;
            break;
        case NC_BUFFER_PROPERTY_FORMATTED_COLOR:
        case NC_BUFFER_PROPERTY_FORMATTED_COLOR_END:
            nc_formatted_color_copy(&property.value.formatted_color,
                                    &source_property->value.formatted_color);
            break;
        case NC_BUFFER_PROPERTY_COUNT:
        default:
            break;
        }

        ARRAY_PUSH(dest->properties, property);
    }
    return;
}

void
nc_buffer_move(NcBuffer *dest, NcBuffer *source) {
    *dest = *source;
    *source = (NcBuffer){0};
    return;
}

void
nc_buffer_destroy(NcBuffer *buffer) {
    for (int32 i = 0; i < ARRAY_LEN(buffer->properties); i += 1) {
        nc_buffer_property_destroy(&buffer->properties[i]);
    }
    ARRAY_FREE(buffer->properties);
    free2(buffer->data, buffer->cap);
    *buffer = (NcBuffer){0};
    return;
}

void
nc_buffer_clear(NcBuffer *buffer) {
    buffer->len = 0;
    if (buffer->data) {
        buffer->data[0] = '\0';
    }

    for (int32 i = 0; i < ARRAY_LEN(buffer->properties); i += 1) {
        nc_buffer_property_destroy(&buffer->properties[i]);
    }
    ARRAY_CLEAR(buffer->properties);
    return;
}

bool
nc_buffer_is_empty(NcBuffer *buffer) {
    return (buffer->len <= 0) && (ARRAY_LEN(buffer->properties) <= 0);
}

char *
nc_buffer_data(NcBuffer *buffer) {
    if (buffer->data == NULL) {
        nc_buffer_reserve(buffer, 0);
    }
    return buffer->data;
}

int32
nc_buffer_len(NcBuffer *buffer) {
    return buffer->len;
}

NcBufferProperty *
nc_buffer_properties(NcBuffer *buffer) {
    return buffer->properties;
}

int32
nc_buffer_property_count(NcBuffer *buffer) {
    return ARRAY_LEN(buffer->properties);
}

void
nc_buffer_append_data(NcBuffer *buffer, char *data, int32 data_len) {
    if (data_len <= 0) {
        return;
    }

    nc_buffer_reserve(buffer, data_len);
    memcpy64(buffer->data + buffer->len, data, data_len);
    buffer->len += data_len;
    buffer->data[buffer->len] = '\0';
    return;
}

void
nc_buffer_append_cstring(NcBuffer *buffer, char *string) {
    if (string == NULL) {
        return;
    }

    nc_buffer_append_data(buffer, string, strlen32(string));
    return;
}

void
nc_buffer_append_char(NcBuffer *buffer, char ch) {
    nc_buffer_reserve(buffer, 1);
    buffer->data[buffer->len] = ch;
    buffer->len += 1;
    buffer->data[buffer->len] = '\0';
    return;
}

void
nc_buffer_append_int32(NcBuffer *buffer, int32 value) {
    char string[64];
    int32 len;

    len = SNPRINTF(string, "%d", value);
    nc_buffer_append_data(buffer, string, len);
    return;
}

void
nc_buffer_append_int64(NcBuffer *buffer, int64 value) {
    char string[64];
    int32 len;

    len = SNPRINTF(string, "%lld", value);
    nc_buffer_append_data(buffer, string, len);
    return;
}

void
nc_buffer_add_color(NcBuffer *buffer, int32 position, NcColor color, int64 id) {
    NcBufferProperty property;

    property.value.color = color;
    property.id = id;
    property.position = position;
    property.type = NC_BUFFER_PROPERTY_COLOR;

    nc_buffer_add_property(buffer, &property);
    return;
}

void
nc_buffer_add_format(NcBuffer *buffer, int32 position,
                     enum NcFormat format, int64 id) {
    NcBufferProperty property;

    property.value.format = format;
    property.id = id;
    property.position = position;
    property.type = NC_BUFFER_PROPERTY_FORMAT;

    nc_buffer_add_property(buffer, &property);
    return;
}

void
nc_buffer_add_formatted_color(NcBuffer *buffer, int32 position,
                              NcFormattedColor *formatted_color,
                              int64 id) {
    NcBufferProperty property;

    nc_formatted_color_copy(&property.value.formatted_color, formatted_color);
    property.id = id;
    property.position = position;
    property.type = NC_BUFFER_PROPERTY_FORMATTED_COLOR;

    nc_buffer_add_property(buffer, &property);
    return;
}

void
nc_buffer_add_formatted_color_end(NcBuffer *buffer, int32 position,
                                  NcFormattedColor *formatted_color,
                                  int64 id) {
    NcBufferProperty property;

    nc_formatted_color_copy(&property.value.formatted_color, formatted_color);
    property.id = id;
    property.position = position;
    property.type = NC_BUFFER_PROPERTY_FORMATTED_COLOR_END;

    nc_buffer_add_property(buffer, &property);
    return;
}

void
nc_buffer_remove_properties(NcBuffer *buffer, int64 id) {
    int32 out;
    int32 count;

    out = 0;
    count = ARRAY_LEN(buffer->properties);
    for (int32 i = 0; i < count; i += 1) {
        if (buffer->properties[i].id == id) {
            nc_buffer_property_destroy(&buffer->properties[i]);
        } else {
            if (out != i) {
                buffer->properties[out] = buffer->properties[i];
                buffer->properties[i].type = NC_BUFFER_PROPERTY_COLOR;
                buffer->properties[i].value.color = nc_color_default();
            }
            out += 1;
        }
    }
    if (buffer->properties) {
        ARRAY_HEADER(buffer->properties)->count = out;
    }
    return;
}

void
nc_buffer_apply_property(NcWindow *window, NcBufferProperty *property) {
    NcFormattedColor *formatted_color;
    enum NcFormat *formats;
    enum NcFormat format;
    int32 count;

    switch (property->type) {
    case NC_BUFFER_PROPERTY_COLOR:
        nc_window_push_color(window, property->value.color);
        break;
    case NC_BUFFER_PROPERTY_FORMAT:
        nc_window_apply_format(window, property->value.format);
        break;
    case NC_BUFFER_PROPERTY_FORMATTED_COLOR:
        formatted_color = &property->value.formatted_color;
        nc_window_push_color(window, formatted_color->color);
        formats = nc_formatted_color_formats(formatted_color);
        count = nc_formatted_color_format_count(formatted_color);
        for (int32 i = 0; i < count; i += 1) {
            nc_window_apply_format(window, formats[i]);
        }
        break;
    case NC_BUFFER_PROPERTY_FORMATTED_COLOR_END:
        formatted_color = &property->value.formatted_color;
        if (!nc_color_is_default(formatted_color->color)) {
            nc_window_push_color(window, nc_color_end());
        }
        formats = nc_formatted_color_formats(formatted_color);
        count = nc_formatted_color_format_count(formatted_color);
        for (int32 i = count - 1; i >= 0; i -= 1) {
            switch (formats[i]) {
            case NC_FORMAT_BOLD:
                format = NC_FORMAT_NO_BOLD;
                break;
            case NC_FORMAT_NO_BOLD:
                format = NC_FORMAT_BOLD;
                break;
            case NC_FORMAT_UNDERLINE:
                format = NC_FORMAT_NO_UNDERLINE;
                break;
            case NC_FORMAT_NO_UNDERLINE:
                format = NC_FORMAT_UNDERLINE;
                break;
            case NC_FORMAT_REVERSE:
                format = NC_FORMAT_NO_REVERSE;
                break;
            case NC_FORMAT_NO_REVERSE:
                format = NC_FORMAT_REVERSE;
                break;
            case NC_FORMAT_ALT_CHARSET:
                format = NC_FORMAT_NO_ALT_CHARSET;
                break;
            case NC_FORMAT_NO_ALT_CHARSET:
                format = NC_FORMAT_ALT_CHARSET;
                break;
            case NC_FORMAT_ITALIC:
                format = NC_FORMAT_NO_ITALIC;
                break;
            case NC_FORMAT_NO_ITALIC:
                format = NC_FORMAT_ITALIC;
                break;
            case NC_FORMAT_COUNT:
            default:
                format = NC_FORMAT_NO_BOLD;
                break;
            }
            nc_window_apply_format(window, format);
        }
        break;
    case NC_BUFFER_PROPERTY_COUNT:
    default:
        break;
    }
    return;
}

#endif /* NCMPCPP_NC_BUFFER_C */
