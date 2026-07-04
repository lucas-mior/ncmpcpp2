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

#include <chrono>
#include <iomanip>

#include "global.h"
#include "helpers.h"
#include "screens/server_info.h"
#include "statusbar.h"
#include "screens/screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

ServerInfo *myServerInfo;

ServerInfo::ServerInfo()
: m_timer()
{
	NcScreenCallbacks callbacks = {0};

	nc_server_info_screen_init(&m_screen,
	                           callbacks,
	                           this,
	                           COLS,
	                           LINES,
	                           MainHeight);
	w = NC::Scrollpad(nc_server_info_screen_start_x(&m_screen, COLS),
	                  nc_server_info_screen_start_y(&m_screen,
	                                                MainStartY,
	                                                MainHeight),
	                  nc_server_info_screen_width(&m_screen),
	                  nc_server_info_screen_height(&m_screen),
	                  "MPD server info",
	                  Config.main_color,
	                  Config.window_border);
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
	w.display();
}

void ServerInfo::refreshWindow()
{
	w.display();
}

void ServerInfo::scroll(NC::Scroll where)
{
	w.scroll(where);
}

void ServerInfo::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
	{
		SwitchTo::execute(this);
		
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
	else
		switchToPreviousScreen();
}

void ServerInfo::resize()
{
	setDimensions();
	w.resize(nc_server_info_screen_width(&m_screen),
	         nc_server_info_screen_height(&m_screen));
	w.moveTo(nc_server_info_screen_start_x(&m_screen, COLS),
	         nc_server_info_screen_start_y(&m_screen,
	                                       MainStartY,
	                                       MainHeight));
	if (previousScreen() && previousScreen()->hasToBeResized) // resize background window
	{
		previousScreen()->resize();
		previousScreen()->refresh();
	}
	nc_screen_set_has_to_be_resized(&m_screen.base, false);
	hasToBeResized = false;
}

int ServerInfo::windowTimeout()
{
	return defaultWindowTimeout;
}

std::string ServerInfo::title()
{
	return previousScreen()->title();
}

void ServerInfo::update()
{
	if (Global::Timer - m_timer < std::chrono::seconds(1))
		return;
	m_timer = Global::Timer;
	
	MPD::Statistics stats = Mpd.getStatistics();
	if (stats.empty())
		return;
	
	w.clear();
	
	w << NC::Format::Bold << "Version: " << NC::Format::NoBold << "0." << Mpd.Version() << ".*\n";
	w << NC::Format::Bold << "Uptime: " << NC::Format::NoBold;
	ShowTime(w, stats.uptime(), 1);
	w << '\n';
	w << NC::Format::Bold << "Time playing: " << NC::Format::NoBold
	  << MPD::Song::ShowTime(stats.playTime()) << '\n';
	w << '\n';
	w << NC::Format::Bold << "Total playtime: " << NC::Format::NoBold;
	ShowTime(w, stats.dbPlayTime(), 1);
	w << '\n';
	w << NC::Format::Bold << "Artist names: " << NC::Format::NoBold << stats.artists() << '\n';
	w << NC::Format::Bold << "Album names: " << NC::Format::NoBold << stats.albums() << '\n';
	w << NC::Format::Bold << "Songs in database: " << NC::Format::NoBold << stats.songs() << '\n';
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

void ServerInfo::mouseButtonPressed(MEVENT me)
{
	scrollpadMouseButtonPressed(w, me);
}

void ServerInfo::setDimensions()
{
	nc_server_info_screen_set_dimensions(&m_screen,
	                                     COLS,
	                                     LINES,
	                                     MainHeight);
}
