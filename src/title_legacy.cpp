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

#include <cstring>
#include <iostream>

#include "app_controller.h"
#include "global.h"
#include "ui_state.h"
#include "settings_legacy.h"
#include "title_legacy.h"
#include "utility/utf8.h"

namespace {

char *current_screen_title()
{
	NcScreen *screen = app_controller_current_screen();
	char *title;

	if (screen == nullptr)
		return const_cast<char *>("");
	title = nc_screen_title(screen);
	if (title == nullptr)
		return const_cast<char *>("");
	return title;
}

}

void windowTitle(const std::string &status)
{
	if (Config.set_window_title)
		std::cout << "\033]0;" << status << "\7" << std::flush;
}

void drawHeader()
{

	if (!Config.header_visibility)
		return;
	switch (Config.design)
	{
		case NCM_DESIGN_CLASSIC:
			*static_cast<NC::Window *>(ui_state_header_legacy_window()) << NC::XY(0, 0)
			         << NC::TermManip::ClearToEOL
			         << NC_FORMAT_BOLD
			         << current_screen_title()
			         << NC_FORMAT_NO_BOLD
			         << NC::XY(static_cast<NC::Window *>(ui_state_header_legacy_window())->getWidth()-global_volume_state_len(), 0)
			         << Config.volume_color
			         << global_volume_state_cstr()
			         << NC::FormattedColor::End<>(Config.volume_color);
			break;
		case NCM_DESIGN_ALTERNATIVE:
			std::string title = current_screen_title();
			*static_cast<NC::Window *>(ui_state_header_legacy_window()) << NC::XY(0, 3)
			         << NC::TermManip::ClearToEOL
			         << Config.alternative_ui_separator_color;
			mvwhline(static_cast<NC::Window *>(ui_state_header_legacy_window())->raw(), 2, 0, 0, COLS);
			mvwhline(static_cast<NC::Window *>(ui_state_header_legacy_window())->raw(), 4, 0, 0, COLS);
			*static_cast<NC::Window *>(ui_state_header_legacy_window()) << NC::FormattedColor::End<>(Config.alternative_ui_separator_color)
			         << NC::XY((COLS-Utf8::width(title))/2, 3)
			         << NC_FORMAT_BOLD
			         << title
			         << NC_FORMAT_NO_BOLD;
			break;
	}
	static_cast<NC::Window *>(ui_state_header_legacy_window())->refresh();
}
