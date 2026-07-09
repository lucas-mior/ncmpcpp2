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

#include "global.h"
#include "app_controller.h"
#include "ui_state.h"
#include "settings_legacy.h"
#include "status_legacy.h"
#include "statusbar_legacy.h"
#include "statusbar.h"
#include "bindings.h"
#include "screens/playlist.h"
#include "utility/utf8.h"


namespace {

int32
statusbar_legacy_cstring_len(const char *string)
{
	if (string == nullptr)
		return 0;
	return static_cast<int32>(std::strlen(string));
}

}

Progressbar::ScopedLock::ScopedLock() noexcept
{
	ncm_progressbar_scoped_lock_init(nullptr);
}

Progressbar::ScopedLock::~ScopedLock() noexcept
{
	ncm_progressbar_scoped_lock_destroy(nullptr);
}

bool Progressbar::isUnlocked()
{
	return ncm_progressbar_is_unlocked();
}

void Progressbar::draw(unsigned int elapsed, unsigned int time)
{
	ncm_progressbar_draw(elapsed, time);
}


Statusbar::ScopedLock::ScopedLock() noexcept
{
	ncm_statusbar_scoped_lock_init(nullptr);
}

Statusbar::ScopedLock::~ScopedLock() noexcept
{
	ncm_statusbar_scoped_lock_destroy(nullptr);
}


bool Statusbar::isUnlocked()
{
	return ncm_statusbar_is_unlocked();
}

void Statusbar::tryRedraw()
{
	ncm_statusbar_try_redraw();
}


NC::Window &Statusbar::put()
{
	ncm_statusbar_put();
	return *static_cast<NC::Window *>(ui_state_footer_legacy_window());
}


void Statusbar::print(int delay, const std::string &message)
{
	ncm_statusbar_print(delay, const_cast<char *>(message.data()),
	                    static_cast<int32>(message.size()));
}


void Statusbar::Helpers::mpd()
{
	Status::update(Mpd.noidle());
}

bool Statusbar::Helpers::mainHook(const char *s)
{
	return ncm_statusbar_main_hook(const_cast<char *>(s),
	                               statusbar_legacy_cstring_len(s));
}

char Statusbar::Helpers::promptReturnOneOf(const std::vector<char> &values)
{
	char result;

	if (values.empty())
		throw std::logic_error("empty vector of acceptable input");
	if (!ncm_statusbar_prompt_return_one_of(
	        ui_state_footer_window(), const_cast<char *>(values.data()),
	        static_cast<int32>(values.size()), &result))
		throw NC::PromptAborted();
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
