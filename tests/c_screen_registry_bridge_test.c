#include <assert.h>
#include <stddef.h>

#include "app_controller.h"
#include "screens/screen_type.h"

int
main(void) {
    NcScreen screen;
    NcScreenCallbacks callbacks = {0};
    int32 native_type;

    app_controller_init();

    native_type = screen_type_to_native_type(NCM_SCREEN_TYPE_HELP);
    assert(native_type == NC_SCREEN_TYPE_HELP);
    assert(app_controller_find_screen_type(native_type) == NULL);

    nc_screen_init(&screen, callbacks, NULL, native_type);
    assert(app_controller_register_screen(&screen));
    assert(app_controller_find_screen_type(native_type) == &screen);
    assert(app_controller_unregister_screen(&screen));
    assert(app_controller_find_screen_type(native_type) == NULL);

    return 0;
}
