#if !defined(NCMPCPP_NC_SCROLLPAD_H)
#define NCMPCPP_NC_SCROLLPAD_H

#include "cbase.h"

#include "curses/nc_buffer.h"
#include "curses/nc_window.h"

typedef struct NcScrollpad {
    int32 beginning;
    int32 real_height;
} NcScrollpad;

void nc_scrollpad_init(NcScrollpad *scrollpad, int32 height);
void nc_scrollpad_refresh(NcScrollpad *scrollpad, NcWindow *window);
void nc_scrollpad_resize(NcScrollpad *scrollpad, NcWindow *window,
                         int32 new_width, int32 new_height);
void nc_scrollpad_scroll(NcScrollpad *scrollpad, NcWindow *window,
                         enum NcScroll where);
void nc_scrollpad_prepare_flush(NcScrollpad *scrollpad, NcWindow *window,
                                int32 generated_height);
void nc_scrollpad_flush(NcScrollpad *scrollpad, NcWindow *window,
                        NcBuffer *buffer);
int32 nc_scrollpad_max_beginning(NcScrollpad *scrollpad, NcWindow *window);
int32 nc_scrollpad_buffer_position_row(NcBuffer *buffer, int32 width,
                                       int32 position);
int32 nc_scrollpad_buffer_height(NcBuffer *buffer, int32 width);
void nc_scrollpad_center_on_buffer_position(NcScrollpad *scrollpad,
                                            NcWindow *window,
                                            NcBuffer *buffer,
                                            int32 position);
void nc_scrollpad_reset(NcScrollpad *scrollpad);

#endif /* NCMPCPP_NC_SCROLLPAD_H */
