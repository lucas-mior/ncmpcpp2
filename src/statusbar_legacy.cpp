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


#include "global.h"
#include "app_controller.h"
#include "ui_state.h"
#include "settings_legacy.h"
#include "status_legacy.h"
#include "statusbar_legacy.h"
#include "bindings.h"
#include "screens/playlist.h"
#include "utility/utf8.h"


namespace {

bool progressbar_block_update = false;

NcmTimePoint statusbar_lock_time;
int64 statusbar_lock_delay_seconds = -1;

bool statusbar_block_update = false;
bool statusbar_allow_unlock = true;

}

Progressbar::ScopedLock::ScopedLock() noexcept
{
	progressbar_block_update = true;
}

Progressbar::ScopedLock::~ScopedLock() noexcept
{
	progressbar_block_update = false;
}

bool Progressbar::isUnlocked()
{
	return !progressbar_block_update;
}

void Progressbar::draw(unsigned int elapsed, unsigned int time)
{
	NC::Window *window =
		static_cast<NC::Window *>(ui_state_footer_legacy_window());
	unsigned pb_width = window->getWidth();
	unsigned howlong = time ? pb_width*elapsed/time : 0;
	const auto progressbar = Utf8::split(Config.progressbar);

	window->goToXY(0, 0);
	*window << Config.progressbar_color;
	if (!progressbar[2].empty() && progressbar[2][0] != '\0')
	{
		for (unsigned i = 0; i < pb_width; ++i)
			*window << progressbar[2];
		window->goToXY(0, 0);
	}
	else
	{
		mvwhline(window->raw(), 0, 0, 0, pb_width);
		window->goToXY(0, 0);
	}
	*window << NC::FormattedColor::End<>(Config.progressbar_color);
	if (time)
	{
		*window << Config.progressbar_elapsed_color;
		pb_width = std::min(size_t(howlong), window->getWidth());
		for (unsigned i = 0; i < pb_width; ++i)
			*window << progressbar[0];
		if (howlong < window->getWidth())
			*window << progressbar[1];
		*window << NC::FormattedColor::End<>(Config.progressbar_elapsed_color);
	}
	window->goToXY(0, 0);
}

Statusbar::ScopedLock::ScopedLock() noexcept
{
	// lock
	if (Config.statusbar_visibility)
		statusbar_block_update = true;
	else
		progressbar_block_update = true;
	statusbar_allow_unlock = false;
}

Statusbar::ScopedLock::~ScopedLock() noexcept
{
	// unlock
	statusbar_allow_unlock = true;
	if (statusbar_lock_delay_seconds < 0)
	{
		if (Config.statusbar_visibility)
			statusbar_block_update = false;
		else
			progressbar_block_update = false;
	}
	if (Status::State::player() == MPD::psStop)
	{
		switch (Config.design)
		{
			case NCM_DESIGN_CLASSIC:
				put(); // clear statusbar
				break;
			case NCM_DESIGN_ALTERNATIVE:
				Progressbar::draw(Status::State::elapsedTime(), Status::State::totalTime());
				break;
		}
		static_cast<NC::Window *>(ui_state_footer_legacy_window())->refresh();
	}
}

bool Statusbar::isUnlocked()
{
	return !statusbar_block_update;
}

void Statusbar::tryRedraw()
{
	if (statusbar_lock_delay_seconds > 0
	&&  global_timer_elapsed_seconds(statusbar_lock_time)
	    > statusbar_lock_delay_seconds)
	{
		statusbar_lock_delay_seconds = -1;
		
		if (Config.statusbar_visibility)
			statusbar_block_update = !statusbar_allow_unlock;
		else
			progressbar_block_update = !statusbar_allow_unlock;
		
		if (!statusbar_block_update && !progressbar_block_update)
		{
			switch (Config.design)
			{
				case NCM_DESIGN_CLASSIC:
					switch (Status::State::player())
					{
						case MPD::psUnknown:
						case MPD::psStop:
							put(); // clear statusbar
							break;
						case MPD::psPlay:
						case MPD::psPause:
							Status::Changes::elapsedTime(false);
						break;
					}
					break;
				case NCM_DESIGN_ALTERNATIVE:
					Progressbar::draw(Status::State::elapsedTime(), Status::State::totalTime());
					break;
			}
			static_cast<NC::Window *>(ui_state_footer_legacy_window())->refresh();
		}
	}
}

NC::Window &Statusbar::put()
{
	NC::Window *window =
		static_cast<NC::Window *>(ui_state_footer_legacy_window());
	int y = Config.statusbar_visibility ? 1 : 0;

	window->goToXY(0, y);
	*window << NC::TermManip::ClearToEOL;
	window->goToXY(0, y);
	return *window;
}

void Statusbar::print(int delay, const std::string &message)
{
	if (statusbar_allow_unlock)
	{
		NC::Window &window = put();

		if (delay)
		{
			statusbar_lock_time = global_timer;
			statusbar_lock_delay_seconds = delay;
			if (Config.statusbar_visibility)
				statusbar_block_update = true;
			else
				progressbar_block_update = true;
		}
		window << message << NC::TermManip::ClearToEOL;
		window.goToXY(0, Config.statusbar_visibility ? 1 : 0);
		window.refresh();
	}
}

void Statusbar::Helpers::mpd()
{
	Status::update(Mpd.noidle());
}

bool Statusbar::Helpers::mainHook(const char *)
{
	Status::trace();
	return true;
}

char Statusbar::Helpers::promptReturnOneOf(const std::vector<char> &values)
{
	if (values.empty())
		throw std::logic_error("empty vector of acceptable input");
	NcKey result;
	do
	{
		static_cast<NC::Window *>(ui_state_footer_legacy_window())->refresh();
		result = ncm_read_key(static_cast<NC::Window *>(ui_state_footer_legacy_window())->nativeWindow());
		if (result == NC_KEY_CTRL_C || result == NC_KEY_CTRL_G)
			throw NC::PromptAborted();
	}
	while (std::find(values.begin(), values.end(), result) == values.end());
	return result;
}

bool Statusbar::Helpers::ImmediatelyReturnOneOf::operator()(const char *s) const
{
	Status::trace();
	return !isOneOf(s);
}

bool Statusbar::Helpers::ApplyFilterImmediately::operator()(const char *s)
{
	Status::trace();
	try {
		if (m_w->allowsFiltering() && m_w->currentFilter() != s)
		{
			m_w->applyFilter(s);
			NcScreen *current_screen = app_controller_current_screen();

			if (current_screen == myPlaylist->nativeScreen())
				myPlaylist->enableHighlighting();
			if (current_screen != nullptr)
				nc_screen_refresh_window(current_screen);
		}
	} catch (Regex::Error &) { }
	return true;
}

bool Statusbar::Helpers::FindImmediately::operator()(const char *s)
{
	Status::trace();
	try {
		if (m_w->allowsSearching() && m_w->searchConstraint() != s)
		{
			m_w->setSearchConstraint(s);
			m_w->search(m_direction, Config.wrapped_search, false);
			NcScreen *current_screen = app_controller_current_screen();

			if (current_screen == myPlaylist->nativeScreen())
				myPlaylist->enableHighlighting();
			if (current_screen != nullptr)
				nc_screen_refresh_window(current_screen);
		}
	} catch (Regex::Error &) { }
	return true;
}

bool Statusbar::Helpers::TryExecuteImmediateCommand::operator()(const char *s)
{
	bool continue_ = true;
	if (m_s != s)
	{
		m_s = s;
		auto cmd = ncm_bindings_configuration_find_command(
			&Bindings, const_cast<char *>(m_s.data()),
			static_cast<int32>(m_s.size()));
		if (cmd && cmd->immediate)
			continue_ = false;
	}
	Status::trace();
	return continue_;
}
