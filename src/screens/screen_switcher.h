#if !defined(NCMPCPP_SCREEN_SWITCHER_H)
#define NCMPCPP_SCREEN_SWITCHER_H

#include <stdbool.h>

#include "screens/nc_screen.h"

#if defined(__cplusplus)
extern "C" {
#endif

bool nc_screen_switcher_switch_to(NcScreen *screen,
                                  bool has_to_be_resized);
bool nc_screen_switcher_finish_switch(NcScreen *screen);

#if defined(__cplusplus)
}
#endif

#endif /* NCMPCPP_SCREEN_SWITCHER_H */
