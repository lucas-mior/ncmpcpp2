#if !defined(NCMPCPP_NC_BUFFER_H)
#define NCMPCPP_NC_BUFFER_H

#include "curses/nc_formatted_color.h"

#define ENUM_NAME NcBufferPropertyType
#define ENUM_PREFIX_ NC_BUFFER_PROPERTY_
#define ENUM_BITFLAGS 0
#define ENUM_FIELDS \
    X(NC_BUFFER_PROPERTY_COLOR) \
    X(NC_BUFFER_PROPERTY_FORMAT) \
    X(NC_BUFFER_PROPERTY_FORMATTED_COLOR) \
    X(NC_BUFFER_PROPERTY_FORMATTED_COLOR_END)
#include "cbase/xenums.c"

typedef struct NcBufferProperty {
    union {
        NcColor color;
        enum NcFormat format;
        NcFormattedColor formatted_color;
    } value;

    int64 id;
    int32 position;
    enum NcBufferPropertyType type;
} NcBufferProperty;

typedef struct NcBuffer {
    char *data;
    NcBufferProperty *properties;

    int32 len;
    int32 cap;
} NcBuffer;

void nc_buffer_init(NcBuffer *buffer);
void nc_buffer_copy(NcBuffer *dest, NcBuffer *source);
void nc_buffer_move(NcBuffer *dest, NcBuffer *source);
void nc_buffer_destroy(NcBuffer *buffer);
void nc_buffer_clear(NcBuffer *buffer);
bool nc_buffer_empty(NcBuffer *buffer);

char *nc_buffer_data(NcBuffer *buffer);
int32 nc_buffer_len(NcBuffer *buffer);
NcBufferProperty *nc_buffer_properties(NcBuffer *buffer);
int32 nc_buffer_property_count(NcBuffer *buffer);

void nc_buffer_append_data(NcBuffer *buffer, char *data, int32 data_len);
void nc_buffer_append_cstring(NcBuffer *buffer, char *string);
void nc_buffer_append_char(NcBuffer *buffer, char ch);
void nc_buffer_append_int32(NcBuffer *buffer, int32 value);
void nc_buffer_append_int64(NcBuffer *buffer, int32 value);

void nc_buffer_add_color(NcBuffer *buffer, int32 position, NcColor color,
                         int64 id);
void nc_buffer_add_format(NcBuffer *buffer, int32 position,
                          enum NcFormat format, int64 id);
void nc_buffer_add_formatted_color(NcBuffer *buffer, int32 position,
                                   NcFormattedColor *formatted_color,
                                   int64 id);
void nc_buffer_add_formatted_color_end(NcBuffer *buffer, int32 position,
                                       NcFormattedColor *formatted_color,
                                       int64 id);
void nc_buffer_remove_properties(NcBuffer *buffer, int64 id);
void nc_buffer_apply_property(NcWindow *window, NcBufferProperty *property);

#endif /* NCMPCPP_NC_BUFFER_H */
