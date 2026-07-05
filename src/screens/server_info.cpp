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

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iterator>

#include "global.h"
#include "helpers.h"
#include "screens/server_info.h"
#include "screens/screen_switcher.h"
#include "statusbar.h"

using Global::MainHeight;
using Global::MainStartY;

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

ServerInfo *myServerInfo;

ServerInfo::ServerInfo()
: m_timer()
{
	NcScreenCallbacks callbacks = makeCallbacks();

	nc_server_info_screen_init(&m_screen,
	                           callbacks,
	                           this,
	                           COLS,
	                           LINES,
	                           MainStartY,
	                           MainHeight);
	w = NC::Scrollpad(nc_server_info_screen_start_x(&m_screen),
	                  nc_server_info_screen_start_y(&m_screen),
	                  nc_server_info_screen_width(&m_screen),
	                  nc_server_info_screen_height(&m_screen),
	                  "MPD server info",
	                  Config.main_color,
	                  Config.window_border);

	bool register_success = nc_screen_registry_register(
		&Global::myScreenRegistry, nativeScreen());
	assert(register_success);
	(void)register_success;
}

ServerInfo::~ServerInfo()
{
	if (nc_screen_registry_is_registered(&Global::myScreenRegistry,
	                                     nativeScreen()))
	{
		nc_screen_registry_unregister(&Global::myScreenRegistry,
		                              nativeScreen());
	}
}

bool ServerInfo::isActiveWindow(const NC::Window &w_) const
{
	return &w == &w_;
}

NC::Window *ServerInfo::activeWindow()
{
	return &w;
}

const NC::Window *ServerInfo::activeWindow() const
{
	return &w;
}

void ServerInfo::refresh()
{
	nc_screen_refresh(nativeScreen());
}

void ServerInfo::refreshWindow()
{
	nc_screen_refresh_window(nativeScreen());
}

void ServerInfo::scroll(NC::Scroll where)
{
	nc_screen_scroll(nativeScreen(), to_nc_scroll(where));
}

void ServerInfo::switchTo()
{
	nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
	nc_screen_registry_switch_to(&Global::myScreenRegistry, nativeScreen());
}

void ServerInfo::resize()
{
	nc_screen_resize(nativeScreen());
}

int ServerInfo::windowTimeout()
{
	return nc_screen_window_timeout(nativeScreen());
}

std::string ServerInfo::title()
{
	char *result = nc_screen_title(nativeScreen());
	if (result == nullptr)
		return "";
	return result;
}

void ServerInfo::update()
{
	nc_screen_update(nativeScreen());
}

void ServerInfo::mouseButtonPressed(MEVENT me)
{
	nc_screen_mouse_button_pressed(nativeScreen(), me);
}

bool ServerInfo::isLockable()
{
	return nc_screen_is_lockable(nativeScreen());
}

bool ServerInfo::isMergable()
{
	return nc_screen_is_mergable(nativeScreen());
}

NcScreen *ServerInfo::nativeScreen()
{
	return nc_server_info_screen_base(&m_screen);
}

const NcScreen *ServerInfo::nativeScreen() const
{
	return nc_server_info_screen_base(const_cast<NcServerInfoScreen *>(&m_screen));
}

void ServerInfo::loadServerInfoLists()
{
	m_url_handlers.clear();
	std::copy(
		std::make_move_iterator(Mpd.GetURLHandlers()),
		std::make_move_iterator(MPD::StringIterator()),
		std::back_inserter(m_url_handlers)
	);

	m_tag_types.clear();
	std::copy(
		std::make_move_iterator(Mpd.GetTagTypes()),
		std::make_move_iterator(MPD::StringIterator()),
		std::back_inserter(m_tag_types)
	);
}

void ServerInfo::renderServerInfo()
{
	if (Global::Timer - m_timer < std::chrono::seconds(1))
		return;
	m_timer = Global::Timer;

	MPD::Statistics stats = Mpd.getStatistics();
	if (stats.empty())
		return;

	w.clear();

	w << NC::Format::Bold << "Version: " << NC::Format::NoBold
	  << "0." << Mpd.Version() << ".*\n";
	w << NC::Format::Bold << "Uptime: " << NC::Format::NoBold;
	ShowTime(w, stats.uptime(), 1);
	w << '\n';
	w << NC::Format::Bold << "Time playing: " << NC::Format::NoBold
	  << MPD::Song::ShowTime(stats.playTime()) << '\n';
	w << '\n';
	w << NC::Format::Bold << "Total playtime: " << NC::Format::NoBold;
	ShowTime(w, stats.dbPlayTime(), 1);
	w << '\n';
	w << NC::Format::Bold << "Artist names: " << NC::Format::NoBold
	  << stats.artists() << '\n';
	w << NC::Format::Bold << "Album names: " << NC::Format::NoBold
	  << stats.albums() << '\n';
	w << NC::Format::Bold << "Songs in database: " << NC::Format::NoBold
	  << stats.songs() << '\n';
	w << '\n';
	w << NC::Format::Bold << "Last DB update: " << NC::Format::NoBold
	  << Timestamp(stats.dbUpdateTime()) << '\n';
	w << '\n';
	w << NC::Format::Bold << "URL Handlers:" << NC::Format::NoBold;
	for (auto it = m_url_handlers.begin(); it != m_url_handlers.end(); ++it)
		w << (it != m_url_handlers.begin() ? ", " : " ") << *it;
	w << "\n\n";
	w << NC::Format::Bold << "Tag Types:" << NC::Format::NoBold;
	for (auto it = m_tag_types.begin(); it != m_tag_types.end(); ++it)
		w << (it != m_tag_types.begin() ? ", " : " ") << *it;

	w.flush();
	w.refresh();
}

void ServerInfo::setDimensions()
{
	nc_server_info_screen_set_dimensions(&m_screen,
	                                     COLS,
	                                     LINES,
	                                     MainStartY,
	                                     MainHeight);
}

NcScreenCallbacks ServerInfo::makeCallbacks()
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

ServerInfo *ServerInfo::fromScreen(NcScreen *screen)
{
	return static_cast<ServerInfo *>(nc_screen_user(screen));
}

NcWindow *ServerInfo::activeWindowCallback(NcScreen *screen)
{
	return fromScreen(screen)->w.nativeWindow();
}

void ServerInfo::refreshCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void ServerInfo::refreshWindowCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void ServerInfo::scrollCallback(NcScreen *screen, enum NcScroll where)
{
	fromScreen(screen)->w.scroll(to_cpp_scroll(where));
}

void ServerInfo::switchToCallback(NcScreen *screen)
{
	ServerInfo *server_info = fromScreen(screen);
	using Global::myScreen;

	if (myScreen != server_info)
	{
		SwitchTo::finishNativeSwitch(server_info);
		server_info->loadServerInfoLists();
	}
	else
		server_info->switchToPreviousScreen();
}

void ServerInfo::resizeCallback(NcScreen *screen)
{
	ServerInfo *server_info = fromScreen(screen);

	server_info->setDimensions();
	server_info->w.resize(nc_server_info_screen_width(&server_info->m_screen),
	                      nc_server_info_screen_height(&server_info->m_screen));
	server_info->w.moveTo(nc_server_info_screen_start_x(&server_info->m_screen),
	                      nc_server_info_screen_start_y(&server_info->m_screen));
	if (server_info->previousScreen()
	 && server_info->previousScreen()->hasToBeResized) // resize background window
	{
		server_info->previousScreen()->resize();
		server_info->previousScreen()->refresh();
	}
	server_info->hasToBeResized = false;
}

int32 ServerInfo::windowTimeoutCallback(NcScreen *screen)
{
	(void)screen;
	return defaultWindowTimeout;
}

char *ServerInfo::titleCallback(NcScreen *screen)
{
	ServerInfo *server_info = fromScreen(screen);

	if (server_info->previousScreen())
		server_info->m_title_cache = server_info->previousScreen()->title();
	else
		server_info->m_title_cache.clear();
	return const_cast<char *>(server_info->m_title_cache.c_str());
}

void ServerInfo::updateCallback(NcScreen *screen)
{
	fromScreen(screen)->renderServerInfo();
}

void ServerInfo::mouseButtonPressedCallback(NcScreen *screen, MEVENT event)
{
	scrollpadMouseButtonPressed(fromScreen(screen)->w, event);
}

bool ServerInfo::isLockableCallback(NcScreen *screen)
{
	(void)screen;
	return false;
}

bool ServerInfo::isMergableCallback(NcScreen *screen)
{
	(void)screen;
	return false;
}

void ServerInfo::destroyCallback(NcScreen *screen)
{
	ServerInfo *server_info = fromScreen(screen);

	if (myServerInfo == server_info)
		myServerInfo = nullptr;
	delete server_info;
}
