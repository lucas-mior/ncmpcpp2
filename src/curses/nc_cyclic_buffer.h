#if !defined(NCMPCPP_NC_CYCLIC_BUFFER_H)
#define NCMPCPP_NC_CYCLIC_BUFFER_H

#include <stdbool.h>

#include "c/ncm_base.h"
#include "curses/nc_buffer.h"
#include "curses/nc_window.h"

void nc_cyclic_text_write(NcmBuffer *output, char *string,
                          int32 string_len, int32 *start_pos,
                          int32 width, char *separator,
                          int32 separator_len, bool scrolling_enabled);
void nc_cyclic_buffer_write(NcBuffer *buffer, NcWindow *window,
                            int32 *start_pos, int32 width,
                            char *separator, int32 separator_len);

#endif /* NCMPCPP_NC_CYCLIC_BUFFER_H */
