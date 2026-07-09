#include "curses/nc_cyclic_buffer.h"

#include <stddef.h>

#include "c/ncm_utf8.h"

static int32 nc_cyclic_normalize_start(int64 *start_pos,
                                       int32 total_characters);
static void nc_cyclic_increment_start(int64 *start_pos,
                                      int32 total_characters);
static int32 nc_cyclic_next_position(char *string, int32 string_len,
                                     int32 byte, int32 *char_width);
static void nc_cyclic_text_append(NcmBuffer *output, char *string,
                                  int32 string_len, int32 start_byte,
                                  int32 *written_width, int32 width);
static void nc_cyclic_buffer_write_all(NcBuffer *buffer, NcWindow *window);
static void nc_cyclic_buffer_apply_properties(NcWindow *window,
                                              NcBufferProperty *properties,
                                              int32 property_count,
                                              int32 *property_index,
                                              int32 position,
                                              bool include_position);
static void nc_cyclic_buffer_write_segment(NcBuffer *buffer,
                                           NcWindow *window,
                                           int32 start_byte,
                                           int32 *property_index,
                                           int32 *written_width,
                                           int32 width);
static void nc_cyclic_window_write_text(NcWindow *window, char *string,
                                        int32 string_len, int32 start_byte,
                                        int32 *written_width, int32 width);

void
nc_cyclic_text_write(NcmBuffer *output, char *string, int32 string_len,
                     int64 *start_pos, int32 width, char *separator,
                     int32 separator_len, bool scrolling_enabled) {
    int32 string_width;
    int32 start;
    int32 start_byte;
    int32 string_characters;
    int32 separator_characters;
    int32 written_width;

    if (output == NULL) {
        return;
    }
    ncm_buffer_clear(output);

    if (string == NULL) {
        string_len = 0;
    }
    if (string_len < 0) {
        string_len = 0;
    }
    if (separator == NULL) {
        separator_len = 0;
    }
    if (separator_len < 0) {
        separator_len = 0;
    }
    if (width < 0) {
        width = 0;
    }

    string_width = ncm_utf8_width(string, string_len);
    if (!scrolling_enabled || (string_width <= width)) {
        ncm_buffer_append(output, string, string_len);
        return;
    }

    string_characters = ncm_utf8_characters(string, string_len);
    separator_characters = ncm_utf8_characters(separator, separator_len);
    start = nc_cyclic_normalize_start(
        start_pos, string_characters + separator_characters);

    start_byte = ncm_utf8_byte_position(string, string_len, start);
    written_width = 0;
    nc_cyclic_text_append(output, string, string_len, start_byte,
                          &written_width, width);

    if (start > string_characters) {
        int32 separator_start;
        int32 separator_byte;

        separator_start = start - string_characters;
        separator_byte = ncm_utf8_byte_position(separator, separator_len,
                                                separator_start);
        nc_cyclic_text_append(output, separator, separator_len,
                              separator_byte, &written_width, width);
    } else {
        nc_cyclic_text_append(output, separator, separator_len, 0,
                              &written_width, width);
    }
    nc_cyclic_text_append(output, string, string_len, 0,
                          &written_width, width);

    nc_cyclic_increment_start(
        start_pos, string_characters + separator_characters);
    return;
}

void
nc_cyclic_buffer_write(NcBuffer *buffer, NcWindow *window,
                       int64 *start_pos, int32 width, char *separator,
                       int32 separator_len) {
    char *string;
    int32 string_len;
    int32 string_width;
    int32 string_characters;
    int32 separator_characters;
    int32 start;
    int32 start_byte;
    int32 property_index;
    int32 written_width;

    if ((buffer == NULL) || (window == NULL)) {
        return;
    }
    if (separator == NULL) {
        separator_len = 0;
    }
    if (separator_len < 0) {
        separator_len = 0;
    }
    if (width < 0) {
        width = 0;
    }

    string = nc_buffer_data(buffer);
    string_len = nc_buffer_len(buffer);
    string_width = ncm_utf8_width(string, string_len);
    if (string_width <= width) {
        nc_cyclic_buffer_write_all(buffer, window);
        return;
    }

    string_characters = ncm_utf8_characters(string, string_len);
    separator_characters = ncm_utf8_characters(separator, separator_len);
    start = nc_cyclic_normalize_start(
        start_pos, string_characters + separator_characters);

    start_byte = ncm_utf8_byte_position(string, string_len, start);
    property_index = 0;
    written_width = 0;
    nc_cyclic_buffer_apply_properties(
        window, nc_buffer_properties(buffer),
        nc_buffer_property_count(buffer), &property_index, start_byte,
        false);
    nc_cyclic_buffer_write_segment(buffer, window, start_byte,
                                   &property_index, &written_width,
                                   width);

    if (start > string_characters) {
        int32 separator_start;
        int32 separator_byte;

        separator_start = start - string_characters;
        separator_byte = ncm_utf8_byte_position(separator, separator_len,
                                                separator_start);
        nc_cyclic_window_write_text(window, separator, separator_len,
                                    separator_byte, &written_width,
                                    width);
    } else {
        nc_cyclic_window_write_text(window, separator, separator_len, 0,
                                    &written_width, width);
    }

    property_index = 0;
    nc_cyclic_buffer_write_segment(buffer, window, 0, &property_index,
                                   &written_width, width);

    nc_cyclic_increment_start(
        start_pos, string_characters + separator_characters);
    return;
}

static int32
nc_cyclic_normalize_start(int64 *start_pos, int32 total_characters) {
    if (total_characters <= 0) {
        if (start_pos) {
            *start_pos = 0;
        }
        return 0;
    }
    if (start_pos == NULL) {
        return 0;
    }
    if ((*start_pos < 0) || (*start_pos >= total_characters)) {
        *start_pos = 0;
    }

    return (int32)*start_pos;
}

static void
nc_cyclic_increment_start(int64 *start_pos, int32 total_characters) {
    if ((start_pos == NULL) || (total_characters <= 0)) {
        return;
    }

    *start_pos += 1;
    if (*start_pos >= total_characters) {
        *start_pos = 0;
    }
    return;
}

static int32
nc_cyclic_next_position(char *string, int32 string_len, int32 byte,
                        int32 *char_width) {
    int32 length;
    int32 result;
    uint32 rune;

    if (byte >= string_len) {
        if (char_width) {
            *char_width = 0;
        }
        return string_len;
    }

    length = ncm_utf8_decode(string + byte, string_len - byte, &rune);
    result = byte + length;
    if (result > string_len) {
        result = string_len;
    }
    if (char_width) {
        *char_width = ncm_utf8_char_width(rune);
    }

    return result;
}

static void
nc_cyclic_text_append(NcmBuffer *output, char *string, int32 string_len,
                      int32 start_byte, int32 *written_width,
                      int32 width) {
    int32 byte;

    if ((string == NULL) || (string_len <= 0) || (written_width == NULL)) {
        return;
    }
    if (start_byte < 0) {
        start_byte = 0;
    }
    if (start_byte > string_len) {
        start_byte = string_len;
    }

    byte = start_byte;
    while ((byte < string_len) && (*written_width < width)) {
        int32 char_width;
        int32 next_byte;

        next_byte = nc_cyclic_next_position(string, string_len, byte,
                                            &char_width);
        if ((*written_width + char_width) > width) {
            break;
        }
        ncm_buffer_append(output, string + byte, next_byte - byte);
        *written_width += char_width;
        byte = next_byte;
    }
    return;
}

static void
nc_cyclic_buffer_write_all(NcBuffer *buffer, NcWindow *window) {
    NcBufferProperty *properties;
    char *string;
    int32 property_count;
    int32 property_index;
    int32 string_len;
    int32 byte;

    string = nc_buffer_data(buffer);
    string_len = nc_buffer_len(buffer);
    properties = nc_buffer_properties(buffer);
    property_count = nc_buffer_property_count(buffer);
    property_index = 0;

    byte = 0;
    while (byte < string_len) {
        int32 next_byte;

        nc_cyclic_buffer_apply_properties(
            window, properties, property_count, &property_index, byte,
            true);
        next_byte = ncm_utf8_next_position(string, string_len, byte);
        nc_window_print_data(window, string + byte, next_byte - byte);
        byte = next_byte;
    }

    nc_cyclic_buffer_apply_properties(
        window, properties, property_count, &property_index, string_len,
        true);
    return;
}

static void
nc_cyclic_buffer_apply_properties(NcWindow *window,
                                  NcBufferProperty *properties,
                                  int32 property_count,
                                  int32 *property_index,
                                  int32 position,
                                  bool include_position) {
    while (*property_index < property_count) {
        bool should_apply;

        if (include_position) {
            should_apply = properties[*property_index].position <= position;
        } else {
            should_apply = properties[*property_index].position < position;
        }
        if (!should_apply) {
            break;
        }

        nc_buffer_apply_property(window, &properties[*property_index]);
        *property_index += 1;
    }
    return;
}

static void
nc_cyclic_buffer_write_segment(NcBuffer *buffer, NcWindow *window,
                               int32 start_byte, int32 *property_index,
                               int32 *written_width, int32 width) {
    NcBufferProperty *properties;
    char *string;
    int32 property_count;
    int32 string_len;
    int32 byte;

    string = nc_buffer_data(buffer);
    string_len = nc_buffer_len(buffer);
    properties = nc_buffer_properties(buffer);
    property_count = nc_buffer_property_count(buffer);

    if (start_byte < 0) {
        start_byte = 0;
    }
    if (start_byte > string_len) {
        start_byte = string_len;
    }

    byte = start_byte;
    while ((byte < string_len) && (*written_width < width)) {
        int32 char_width;
        int32 next_byte;

        nc_cyclic_buffer_apply_properties(
            window, properties, property_count, property_index, byte,
            true);
        next_byte = nc_cyclic_next_position(string, string_len, byte,
                                            &char_width);
        if ((*written_width + char_width) > width) {
            break;
        }
        nc_window_print_data(window, string + byte, next_byte - byte);
        *written_width += char_width;
        byte = next_byte;
    }

    nc_cyclic_buffer_apply_properties(
        window, properties, property_count, property_index, string_len,
        true);
    return;
}

static void
nc_cyclic_window_write_text(NcWindow *window, char *string, int32 string_len,
                            int32 start_byte, int32 *written_width,
                            int32 width) {
    int32 byte;

    if ((string == NULL) || (string_len <= 0) || (written_width == NULL)) {
        return;
    }
    if (start_byte < 0) {
        start_byte = 0;
    }
    if (start_byte > string_len) {
        start_byte = string_len;
    }

    byte = start_byte;
    while ((byte < string_len) && (*written_width < width)) {
        int32 char_width;
        int32 next_byte;

        next_byte = nc_cyclic_next_position(string, string_len, byte,
                                            &char_width);
        if ((*written_width + char_width) > width) {
            break;
        }
        nc_window_print_data(window, string + byte, next_byte - byte);
        *written_width += char_width;
        byte = next_byte;
    }
    return;
}
