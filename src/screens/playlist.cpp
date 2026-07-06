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
#include <sstream>

#include "curses/menu_impl.h"
#include "display.h"
#include "app_controller.h"
#include "global.h"
#include "helpers.h"
#include "screens/playlist.h"
#include "screens/screen_switcher.h"
#include "song.h"
#include "status.h"
#include "statusbar.h"
#include "format_impl.h"
#include "helpers/song_iterator_maker.h"
#include "utility/comparators.h"
#include "utility/functional.h"
#include "title.h"

using Global::MainHeight;
using Global::MainStartY;

namespace ph = std::placeholders;

Playlist *myPlaylist;

namespace {

enum NcScroll to_nc_scroll(NC::Scroll where);
std::string songToString(const MPD::Song &s);
bool playlistEntryMatcher(const Regex::Regex &rx, const MPD::Song &s);

}

Playlist::Playlist()
: m_total_length(0), m_remaining_time(0), m_scroll_begin(0)
, m_timer()
, m_reload_total_length(false), m_reload_remaining(false)
{
	w = NC::Menu<MPD::Song>(0, MainStartY, COLS, MainHeight, Config.playlist_display_mode == DisplayMode::Columns && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::Border());
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	setHighlightFixes(w);
	w.setSelectedPrefix(Config.selected_item_prefix);
	w.setSelectedSuffix(Config.selected_item_suffix);
	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Classic:
			w.setItemDisplayer(std::bind(
				Display::Songs, ph::_1, std::cref(w), std::cref(Config.song_list_format)
			));
			break;
		case DisplayMode::Columns:
			w.setItemDisplayer(std::bind(
				Display::SongsInColumns, ph::_1, std::cref(w)
			));
			break;
	}
	w.setItemActivator([](NC::Menu<MPD::Song> &,
	                         NC::Menu<MPD::Song>::Item &item) {
		Mpd.PlayID(item.value().getID());
	});

	NcScreenCallbacks callbacks = makeCallbacks();
	nc_playlist_screen_init(&m_screen,
	                        callbacks,
	                        this,
	                        w.nativeMenu(),
	                        0,
	                        COLS,
	                        MainStartY,
	                        MainHeight);
	nc_playlist_screen_set_mouse_config(&m_screen,
	                                    Config.lines_scrolled,
	                                    Config.mouse_list_scroll_whole_page);

	bool register_success = app_controller_register_screen(nativeScreen());
	assert(register_success);
	(void)register_success;
}

Playlist::~Playlist()
{
	if (app_controller_is_screen_registered(nativeScreen()))
	{
		app_controller_unregister_screen(nativeScreen());
	}
}

void Playlist::refresh()
{
	nc_screen_refresh(nativeScreen());
}

void Playlist::refreshWindow()
{
	nc_screen_refresh_window(nativeScreen());
}

void Playlist::scroll(NC::Scroll where)
{
	nc_screen_scroll(nativeScreen(), to_nc_scroll(where));
}

void Playlist::switchTo()
{
	nc_screen_set_has_to_be_resized(nativeScreen(), hasToBeResized);
	app_controller_switch_to_screen(nativeScreen());
}

void Playlist::resize()
{
	nc_screen_resize(nativeScreen());
}

int Playlist::windowTimeout()
{
	return nc_screen_window_timeout(nativeScreen());
}

std::string Playlist::title()
{
	char *result = nc_screen_title(nativeScreen());
	if (result == nullptr)
		return "";
	return result;
}

void Playlist::update()
{
	nc_screen_update(nativeScreen());
}

void Playlist::mouseButtonPressed(MEVENT me)
{
	nc_screen_mouse_button_pressed(nativeScreen(), me);
}

bool Playlist::isLockable()
{
	return nc_screen_is_lockable(nativeScreen());
}

bool Playlist::isMergable()
{
	return nc_screen_is_mergable(nativeScreen());
}

NcScreen *Playlist::nativeScreen()
{
	return nc_playlist_screen_base(&m_screen);
}

const NcScreen *Playlist::nativeScreen() const
{
	return nc_playlist_screen_base(const_cast<NcPlaylistScreen *>(&m_screen));
}

/***********************************************************************/

bool Playlist::allowsSearching()
{
	return true;
}

const std::string &Playlist::searchConstraint()
{
	return m_search_predicate.constraint();
}

void Playlist::setSearchConstraint(const std::string &constraint)
{
	m_search_predicate = Regex::Filter<MPD::Song>(
		constraint,
		Config.regex_type,
		playlistEntryMatcher);
}

void Playlist::clearSearchConstraint()
{
	m_search_predicate.clear();
}

bool Playlist::search(SearchDirection direction, bool wrap, bool skip_current)
{
	return ::search(w, m_search_predicate, direction, wrap, skip_current);
}

/***********************************************************************/

bool Playlist::allowsFiltering()
{
	return allowsSearching();
}

std::string Playlist::currentFilter()
{
	std::string result;
	if (auto pred = w.filterPredicate<Regex::Filter<MPD::Song>>())
		result = pred->constraint();
	return result;
}

void Playlist::applyFilter(const std::string &constraint)
{
	if (!constraint.empty())
	{
		w.applyFilter(Regex::Filter<MPD::Song>(
			              constraint,
			              Config.regex_type,
			              playlistEntryMatcher));
	}
	else
		w.clearFilter();
}

/***********************************************************************/

bool Playlist::itemAvailable()
{
	return !w.empty();
}

bool Playlist::addItemToPlaylist(bool play)
{
	if (play)
		return nc_playlist_screen_activate_current(&m_screen);
	return true;
}

std::vector<MPD::Song> Playlist::getSelectedSongs()
{
	return w.getSelectedSongs();
}

/***********************************************************************/

MPD::Song Playlist::nowPlayingSong()
{
	MPD::Song s;
	if (Status::State::player() != MPD::psUnknown)
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter(ReapplyFilter::No, w);
		auto sp = Status::State::currentSongPosition();
		if (sp >= 0 && size_t(sp) < w.size())
			s = w.at(sp).value();
	}
	return s;
}

void Playlist::locateSong(const MPD::Song &s)
{
	if (!w.isFiltered())
		w.highlight(s.getPosition());
	else
	{
		auto cmp = [](const MPD::Song &a, const MPD::Song &b) {
			return a.getPosition() < b.getPosition();
		};
		auto first = w.beginV(), last = w.endV();
		auto it = std::lower_bound(first, last, s, cmp);
		if (it != last && it->getPosition() == s.getPosition())
			w.highlight(it - first);
		else
			Statusbar::print("Song is filtered out");
	}
}

void Playlist::enableHighlighting()
{
	w.setHighlighting(true);
	m_timer = Global::Timer;
}

std::string Playlist::getTotalLength()
{
	std::ostringstream result;

	if (m_reload_total_length)
	{
		m_total_length = 0;
		for (const auto &s : w)
			m_total_length += s.value().getDuration();
		m_reload_total_length = false;
	}
	if (Config.playlist_show_remaining_time && m_reload_remaining)
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter(ReapplyFilter::No, w);
		m_remaining_time = 0;
		for (size_t i = Status::State::currentSongPosition(); i < w.size(); ++i)
			m_remaining_time += w[i].value().getDuration();
		m_reload_remaining = false;
	}

	result << '(' << w.size() << (w.size() == 1 ? " item" : " items");

	if (w.isFiltered())
	{
		ScopedUnfilteredMenu<MPD::Song> sunfilter(ReapplyFilter::No, w);
		result << " (out of " << w.size() << ")";
	}

	if (m_total_length)
	{
		result << ", length: ";
		ShowTime(result, m_total_length, Config.playlist_shorten_total_times);
	}
	if (Config.playlist_show_remaining_time && m_remaining_time && w.size() > 1)
	{
		result << ", remaining: ";
		ShowTime(result, m_remaining_time, Config.playlist_shorten_total_times);
	}
	result << ')';
	return result.str();
}

void Playlist::setSelectedItemsPriority(int prio)
{
	auto list = getSelectedOrCurrent(w.begin(), w.end(), w.current());
	Mpd.StartCommandsList();
	for (auto it = list.begin(); it != list.end(); ++it)
		Mpd.SetPriority((*it)->value(), prio);
	Mpd.CommitCommandsList();
	Statusbar::print("Priority set");
}

bool Playlist::checkForSong(const MPD::Song &s)
{
	return m_song_refs.find(s) != m_song_refs.end();
}

void Playlist::registerSong(const MPD::Song &s)
{
	++m_song_refs[s];
}

void Playlist::unregisterSong(const MPD::Song &s)
{
	auto it = m_song_refs.find(s);
	assert(it != m_song_refs.end());
	if (it->second == 1)
		m_song_refs.erase(it);
	else
		--it->second;
}

NcScreenCallbacks Playlist::makeCallbacks()
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

Playlist *Playlist::fromScreen(NcScreen *screen)
{
	return static_cast<Playlist *>(nc_screen_user(screen));
}

NcWindow *Playlist::activeWindowCallback(NcScreen *screen)
{
	return fromScreen(screen)->w.nativeWindow();
}

void Playlist::refreshCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void Playlist::refreshWindowCallback(NcScreen *screen)
{
	fromScreen(screen)->w.display();
}

void Playlist::scrollCallback(NcScreen *screen, enum NcScroll where)
{
	Playlist *playlist = fromScreen(screen);

	nc_playlist_screen_scroll(&playlist->m_screen, where);
}

void Playlist::switchToCallback(NcScreen *screen)
{
	Playlist *playlist = fromScreen(screen);

	SwitchTo::finishNativeSwitch(playlist);
	playlist->m_scroll_begin = 0;
	drawHeader();
}

void Playlist::resizeCallback(NcScreen *screen)
{
	Playlist *playlist = fromScreen(screen);
	size_t x_offset;
	size_t width;

	playlist->getWindowResizeParams(x_offset, width);
	nc_playlist_screen_set_geometry(&playlist->m_screen,
	                                static_cast<int64>(x_offset),
	                                static_cast<int64>(width),
	                                MainStartY,
	                                MainHeight);
	playlist->w.resize(nc_playlist_screen_width(&playlist->m_screen),
	                   nc_playlist_screen_height(&playlist->m_screen));
	playlist->w.moveTo(nc_playlist_screen_start_x(&playlist->m_screen),
	                   nc_playlist_screen_start_y(&playlist->m_screen));

	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Columns:
			if (Config.titles_visibility)
				playlist->w.setTitle(Display::Columns(playlist->w.getWidth()));
			break;
		case DisplayMode::Classic:
			playlist->w.setTitle("");
			break;
	}

	nc_playlist_screen_set_mouse_config(&playlist->m_screen,
	                                    Config.lines_scrolled,
	                                    Config.mouse_list_scroll_whole_page);
	playlist->hasToBeResized = false;
}

int32 Playlist::windowTimeoutCallback(NcScreen *screen)
{
	(void)screen;
	return defaultWindowTimeout;
}

char *Playlist::titleCallback(NcScreen *screen)
{
	Playlist *playlist = fromScreen(screen);

	playlist->m_title_cache = "Playlist ";
	if (Config.playlist_show_mpd_host)
	{
		playlist->m_title_cache += "on ";
		playlist->m_title_cache += Mpd.GetHostname();
		playlist->m_title_cache += " ";
	}
	if (playlist->m_reload_total_length || playlist->m_reload_remaining)
		playlist->m_stats = playlist->getTotalLength();
	playlist->m_title_cache += Scroller(
		playlist->m_stats,
		playlist->m_scroll_begin,
		COLS - Utf8::width(playlist->m_title_cache)
		     - (Config.design == Design::Alternative
		        ? 2
		        : Global::VolumeState.length()));
	return const_cast<char *>(playlist->m_title_cache.c_str());
}

void Playlist::updateCallback(NcScreen *screen)
{
	Playlist *playlist = fromScreen(screen);

	if (playlist->w.isHighlighted()
	&&  Config.playlist_disable_highlight_delay > std::chrono::seconds(0)
	&&  Global::Timer - playlist->m_timer > Config.playlist_disable_highlight_delay)
	{
		playlist->w.setHighlighting(false);
		playlist->w.refresh();
	}
}

void Playlist::mouseButtonPressedCallback(NcScreen *screen, MEVENT event)
{
	Playlist *playlist = fromScreen(screen);

	nc_playlist_screen_mouse_button_pressed(&playlist->m_screen, event);
}

bool Playlist::isLockableCallback(NcScreen *screen)
{
	(void)screen;
	return true;
}

bool Playlist::isMergableCallback(NcScreen *screen)
{
	(void)screen;
	return true;
}

void Playlist::destroyCallback(NcScreen *screen)
{
	Playlist *playlist = fromScreen(screen);

	if (app_controller_is_screen_registered(playlist->nativeScreen()))
	{
		app_controller_unregister_screen(playlist->nativeScreen());
	}
	delete playlist;
}

namespace {


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

std::string songToString(const MPD::Song &s)
{
	std::string result;
	switch (Config.playlist_display_mode)
	{
		case DisplayMode::Classic:
			result = Format::stringify<char>(Config.song_list_format, &s);
			break;
		case DisplayMode::Columns:
			result = Format::stringify<char>(Config.song_columns_mode_format, &s);
	}
	return result;
}

bool playlistEntryMatcher(const Regex::Regex &rx, const MPD::Song &s)
{
	return Regex::search(songToString(s), rx);
}

}
