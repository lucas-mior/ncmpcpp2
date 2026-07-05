/***************************************************************************
 *   Copyright (C) 2008-2021 by Andrzej Rybczak                            *
 *   andrzej@rybczak.net                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <cassert>

#include "screens/outputs.h"

#ifdef ENABLE_OUTPUTS

#include "curses/menu_impl.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "screens/screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

namespace {

NC::Scroll to_cpp_scroll(enum NcScroll where)
{
	switch (where)
	{
		case NC_SCROLL_UP:
			return NC::Scroll::Up;
		case NC_SCROLL_DOWN:
			return NC::Scroll::Down;
		case NC_SCROLL_PAGE_UP:
			return NC::Scroll::PageUp;
		case NC_SCROLL_PAGE_DOWN:
			return NC::Scroll::PageDown;
		case NC_SCROLL_HOME:
			return NC::Scroll::Home;
		case NC_SCROLL_END:
			return NC::Scroll::End;
	}
	return NC::Scroll::Up;
}

enum NcScroll to_nc_scroll(NC::Scroll where)
{
	switch (where)
	{
		case NC::Scroll::Up:
			return NC_SCROLL_UP;
		case NC::Scroll::Down:
			return NC_SCROLL_DOWN;
		case NC::Scroll::PageUp:
			return NC_SCROLL_PAGE_UP;
		case NC::Scroll::PageDown:
			return NC_SCROLL_PAGE_DOWN;
		case NC::Scroll::Home:
			return NC_SCROLL_HOME;
		case NC::Scroll::End:
			return NC_SCROLL_END;
	}
	return NC_SCROLL_UP;
}

}

Outputs *myOutputs;

Outputs::Outputs()
: w(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::Border())
{
	NcScreenCallbacks callbacks = makeCallbacks();

	nc_outputs_screen_init(&m_screen,
	                       callbacks,
	                       this,
	                       0,
	                       COLS,
	                       MainStartY,
	                       MainHeight);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	setHighlightFixes(w);
	w.setItemDisplayer([](NC::Menu<MPD::Output> &menu) {
			auto &output = menu.drawn()->value();
			if (output.enabled())
				menu << NC::Format::Bold;
			menu << Charset::utf8ToLocale(output.name());
			if (output.enabled())
				menu << NC::Format::NoBold;
	});

	bool register_success = nc_screen_registry_register(
		&Global::myScreenRegistry, nativeScreen());
	assert(register_success);
	(void)register_success;
}

Outputs::~Outputs()
{
	if (nc_screen_registry_is_registered(&Global::myScreenRegistry,
	                                     nativeScreen()))
	{
		nc_screen_registry_unregister(&Global::myScreenRegistry,
		                              nativeScreen());
	}
}

bool Outputs::isActiveWindow(const NC::Window &w_) const
{
	return &w == &w_;
}

NC::Window *Outputs::activeWindow()
{
	return &w;
}

const NC::Window *Outputs::activeWindow() const
{
	return &w;
}

void Outputs::refresh()
{
	nc_screen_refresh(nativeScreen());
}

void Outputs::refreshWindow()
{
	nc_screen_refresh_window(nativeScreen());
}

void Outputs::scroll(NC::Scroll where)
{
	nc_screen_scroll(nativeScreen(), to_nc_scroll(where));
}

void Outputs::switchTo()
{
	nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
	nc_screen_registry_switch_to(&Global::myScreenRegistry, nativeScreen());
}

void Outputs::resize()
{
	nc_screen_resize(nativeScreen());
}

int Outputs::windowTimeout()
{
	return nc_screen_window_timeout(nativeScreen());
}

std::string Outputs::title()
{
	char *result = nc_screen_title(nativeScreen());
	if (result == nullptr)
		return "";
	return result;
}

void Outputs::update()
{
	nc_screen_update(nativeScreen());
}

void Outputs::mouseButtonPressed(MEVENT me)
{
	nc_screen_mouse_button_pressed(nativeScreen(), me);
}

bool Outputs::isLockable()
{
	return nc_screen_is_lockable(nativeScreen());
}

bool Outputs::isMergable()
{
	return nc_screen_is_mergable(nativeScreen());
}

NcScreen *Outputs::nativeScreen()
{
	return nc_outputs_screen_base(&m_screen);
}

const NcScreen *Outputs::nativeScreen() const
{
	return nc_outputs_screen_base(const_cast<NcOutputsScreen *>(&m_screen));
}

void Outputs::fetchList()
{
	w.clear();
	for (MPD::OutputIterator out = Mpd.GetOutputs(), end; out != end; ++out)
		w.addItem(std::move(*out));
}

void Outputs::toggleOutput()
{
	if (w.current()->value().enabled())
	{
		Mpd.DisableOutput(w.choice());
		Statusbar::printf("Output \"%s\" disabled", w.current()->value().name());
	}
	else
	{
		Mpd.EnableOutput(w.choice());
		Statusbar::printf("Output \"%s\" enabled", w.current()->value().name());
	}
}

NcScreenCallbacks Outputs::makeCallbacks()
{
	NcScreenCallbacks callbacks = {0};

	callbacks.active_window = activeWindowCallback;
	callbacks.refresh = refreshCallback;
	callbacks.refresh_window = refreshWindowCallback;
	callbacks.scroll = scrollCallback;
	callbacks.switch_to = switchToCallback;
	callbacks.resize = resizeCallback;
	callbacks.window_timeout = windowTimeoutCallback;
	callbacks.title = titleCallback;
	callbacks.update = updateCallback;
	callbacks.mouse_button_pressed = mouseButtonPressedCallback;
	callbacks.is_lockable = isLockableCallback;
	callbacks.is_mergable = isMergableCallback;
	callbacks.destroy = destroyCallback;
	return callbacks;
}

Outputs *Outputs::fromScreen(NcScreen *screen)
{
	return static_cast<Outputs *>(nc_screen_user(screen));
}

NcWindow *Outputs::activeWindowCallback(NcScreen *screen)
{
	return fromScreen(screen)->w.nativeWindow();
}

void Outputs::refreshCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void Outputs::refreshWindowCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void Outputs::scrollCallback(NcScreen *screen, enum NcScroll where)
{
	fromScreen(screen)->w.scroll(to_cpp_scroll(where));
}

void Outputs::switchToCallback(NcScreen *screen)
{
	Outputs *outputs = fromScreen(screen);

	if (myScreen != outputs)
		SwitchTo::execute(outputs);
	drawHeader();
}

void Outputs::resizeCallback(NcScreen *screen)
{
	Outputs *outputs = fromScreen(screen);
	size_t x_offset;
	size_t width;

	outputs->getWindowResizeParams(x_offset, width);
	nc_outputs_screen_set_geometry(&outputs->m_screen,
	                               static_cast<int64>(x_offset),
	                               static_cast<int64>(width),
	                               MainStartY,
	                               MainHeight);
	outputs->w.resize(nc_outputs_screen_width(&outputs->m_screen),
	                  nc_outputs_screen_height(&outputs->m_screen));
	outputs->w.moveTo(nc_outputs_screen_start_x(&outputs->m_screen),
	                  nc_outputs_screen_start_y(&outputs->m_screen));
	outputs->hasToBeResized = false;
}

int32 Outputs::windowTimeoutCallback(NcScreen *screen)
{
	(void)screen;
	return defaultWindowTimeout;
}

char *Outputs::titleCallback(NcScreen *screen)
{
	static char title[] = "Outputs";

	(void)screen;
	return title;
}

void Outputs::updateCallback(NcScreen *screen)
{
	(void)screen;
}

void Outputs::mouseButtonPressedCallback(NcScreen *screen, MEVENT event)
{
	Outputs *outputs = fromScreen(screen);

	if (outputs->w.empty()
	 || !outputs->w.hasCoords(event.x, event.y)
	 || size_t(event.y) >= outputs->w.size())
		return;
	if (event.bstate & BUTTON1_PRESSED || event.bstate & BUTTON3_PRESSED)
	{
		outputs->w.Goto(event.y);
		if (event.bstate & BUTTON3_PRESSED)
			outputs->toggleOutput();
	}
	else
		genericMouseButtonPressed(outputs->w, event);
}

bool Outputs::isLockableCallback(NcScreen *screen)
{
	(void)screen;
	return true;
}

bool Outputs::isMergableCallback(NcScreen *screen)
{
	(void)screen;
	return true;
}

void Outputs::destroyCallback(NcScreen *screen)
{
	Outputs *outputs = fromScreen(screen);

	if (myOutputs == outputs)
		myOutputs = nullptr;
	delete outputs;
}

#endif // ENABLE_OUTPUTS
