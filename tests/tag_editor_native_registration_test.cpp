#include <cassert>
#include <cstdlib>

#include "app_controller.h"
#include "curses/nc_window.h"
#include "settings.h"
#include "screens/native_c_screens.h"
#include "screens/screen_cpp_compat.h"
#include "screens/screen_switcher.h"
#include "ui_state.h"


extern "C" void __wrap_nc_window_init(NcWindow *window,
                                       int64 start_x, int64 start_y,
                                       int64 width, int64 height,
                                       char *title, int32 title_len,
                                       NcColor color, NcBorder border)
{
	(void)title;
	nc_window_init_empty(window);
	window->start_x = start_x;
	window->start_y = start_y;
	window->width = width;
	window->height = height;
	window->title_len = title_len;
	window->color = color;
	window->base_color = color;
	window->border = border;
}

extern "C" void __wrap_nc_window_destroy(NcWindow *window)
{
	nc_window_init_empty(window);
}

extern "C" void __wrap_nc_window_move_to(NcWindow *window,
                                          int64 start_x, int64 start_y)
{
	window->start_x = start_x;
	window->start_y = start_y;
}

extern "C" void __wrap_nc_window_resize(NcWindow *window,
                                         int64 width, int64 height)
{
	window->width = width;
	window->height = height;
}

extern "C" void __wrap_nc_window_set_title(NcWindow *window,
                                            char *title, int32 title_len)
{
	(void)title;
	window->title_len = title_len;
}

extern "C" void __wrap_nc_screen_draw_vertical_separator(int64 x)
{
	(void)x;
}

namespace {

struct TestScreenState
{
	bool lockable;
	bool mergable;
};

bool test_screen_is_lockable(NcScreen *screen)
{
	TestScreenState *state;

	state = static_cast<TestScreenState *>(nc_screen_user(screen));
	return state->lockable;
}

bool test_screen_is_mergable(NcScreen *screen)
{
	TestScreenState *state;

	state = static_cast<TestScreenState *>(nc_screen_user(screen));
	return state->mergable;
}

NcScreenCallbacks test_screen_callbacks()
{
	NcScreenCallbacks callbacks = {0};

	callbacks.is_lockable = test_screen_is_lockable;
	callbacks.is_mergable = test_screen_is_mergable;
	return callbacks;
}

void test_full_tag_editor_native_registration()
{
#if defined(HAVE_TAGLIB_H)
	NcScreen *tag_editor;

	app_controller_init();
	ui_state_set_screen_size(100, 30);

	native_c_screens_register_native_only();
	tag_editor = app_controller_find_screen_type(NC_SCREEN_TYPE_TAG_EDITOR);
	assert(tag_editor == native_c_screen_tag_editor_native());
	assert(native_c_screens_is_registered_type(NCM_SCREEN_TYPE_TAG_EDITOR));
	assert(BaseScreen::legacyFromNativeScreen(tag_editor) == nullptr);

	native_c_screen_tag_editor_register();
	assert(app_controller_find_screen_type(NC_SCREEN_TYPE_TAG_EDITOR)
	       == tag_editor);
#endif
}

void test_full_tag_editor_switch_without_legacy_owner()
{
#if defined(HAVE_TAGLIB_H)
	NcScreen locked;
	NcScreen *tag_editor;
	TestScreenState state = {0};

	app_controller_init();
	ui_state_set_screen_size(100, 30);

	state.lockable = true;
	state.mergable = true;
	nc_screen_init(&locked, test_screen_callbacks(), &state,
	               NC_SCREEN_TYPE_UNKNOWN);
	assert(app_controller_register_screen(&locked));
	native_c_screens_register_native_only();
	tag_editor = native_c_screen_tag_editor_native();

	assert(nc_screen_switcher_switch_to(&locked, true));
	assert(app_controller_lock_current_screen());
	native_c_screen_tag_editor_switch_to();

	assert(nc_screen_switcher_current() == tag_editor);
	assert(nc_screen_switcher_previous() == &locked);
	assert(nc_screen_switcher_locked() == &locked);
	assert(nc_screen_switcher_inactive() == &locked);
	assert(BaseScreen::legacyFromNativeScreen(tag_editor) == nullptr);
#endif
}

}

int main()
{
	test_full_tag_editor_native_registration();
	test_full_tag_editor_switch_without_legacy_owner();
	return EXIT_SUCCESS;
}
