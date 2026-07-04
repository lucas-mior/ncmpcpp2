#if !defined(NCMPCPP_NC_SCROLLPAD_H)
#define NCMPCPP_NC_SCROLLPAD_H

#include "curses/nc_buffer.h"
#include "curses/nc_window.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NcScrollpad {
    int64 beginning;
    int64 real_height;
} NcScrollpad;

void nc_scrollpad_init_empty(NcScrollpad *scrollpad);
void nc_scrollpad_init(NcScrollpad *scrollpad, int64 height);
int64 nc_scrollpad_beginning(NcScrollpad *scrollpad);
int64 nc_scrollpad_real_height(NcScrollpad *scrollpad);
void nc_scrollpad_set_real_height(NcScrollpad *scrollpad,
                                  int64 real_height);
void nc_scrollpad_refresh(NcScrollpad *scrollpad, NcWindow *window);
void nc_scrollpad_resize(NcScrollpad *scrollpad, NcWindow *window,
                         int64 new_width, int64 new_height);
void nc_scrollpad_scroll(NcScrollpad *scrollpad, NcWindow *window,
                         enum NcScroll where);
void nc_scrollpad_clear(NcScrollpad *scrollpad, NcWindow *window);
void nc_scrollpad_prepare_flush(NcScrollpad *scrollpad, NcWindow *window,
                                int64 generated_height);
void nc_scrollpad_flush(NcScrollpad *scrollpad, NcWindow *window,
                        NcBuffer *buffer);
void nc_scrollpad_reset(NcScrollpad *scrollpad);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_NC_SCROLLPAD_H */
