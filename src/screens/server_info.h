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

#ifndef NCMPCPP_SERVER_INFO_H
#define NCMPCPP_SERVER_INFO_H

#include <chrono>
#include <string>
#include <vector>

#include "interfaces.h"
#include "screens/nc_server_info.h"
#include "screens/screen.h"

struct ServerInfo: BaseScreen, Tabbable
{
	ServerInfo();
	
	// BaseScreen implementation
	virtual bool isActiveWindow(const NC::Window &w_) const override;
	virtual NC::Window *activeWindow() override;
	virtual const NC::Window *activeWindow() const override;
	virtual void refresh() override;
	virtual void refreshWindow() override;
	virtual void scroll(NC::Scroll where) override;
	virtual void switchTo() override;
	virtual void resize() override;
	virtual int windowTimeout() override;
	virtual std::string title() override;
	virtual ScreenType type() override { return ScreenType::ServerInfo; }
	virtual void update() override;
	virtual void mouseButtonPressed(MEVENT me) override;
	virtual bool isLockable() override { return false; }
	virtual bool isMergable() override { return false; }
	
private:
	void setDimensions();
	
	NC::Scrollpad w;
	NcServerInfoScreen m_screen;
	std::chrono::steady_clock::time_point m_timer;

	std::vector<std::string> m_url_handlers;
	std::vector<std::string> m_tag_types;
};

extern ServerInfo *myServerInfo;

#endif // NCMPCPP_SERVER_INFO_H
