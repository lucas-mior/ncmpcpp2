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

#ifndef NCMPCPP_PLAYLIST_H
#define NCMPCPP_PLAYLIST_H

#include <unordered_map>

#include "c/ncm_time.h"
#include "interfaces.h"
#include "regex_filter.h"
#include "screens/nc_playlist.h"
#include "screens/screen.h"
#include "song.h"
#include "song_list.h"

struct Playlist: Screen<SongMenu>, Filterable, HasSongs, Searchable, Tabbable
{
	Playlist();
	virtual ~Playlist();
	
	// Screen<SongMenu> implementation
	virtual void refresh() override;
	virtual void refreshWindow() override;
	virtual void scroll(enum NcScroll where) override;
	virtual void switchTo() override;
	virtual void resize() override;
	virtual int windowTimeout() override;
	
	virtual std::string title() override;
	virtual ScreenType type() override { return NCM_SCREEN_TYPE_PLAYLIST; }
	
	virtual void update() override;
	
	virtual void mouseButtonPressed(MEVENT me) override;
	
	virtual bool isLockable() override;
	virtual bool isMergable() override;
	virtual NcScreen *nativeScreen() override;
	virtual const NcScreen *nativeScreen() const override;
	
	// Searchable implementation
	virtual bool allowsSearching() override;
	virtual const std::string &searchConstraint() override;
	virtual void setSearchConstraint(const std::string &constraint) override;
	virtual void clearSearchConstraint() override;
	virtual bool search(SearchDirection direction, bool wrap, bool skip_current) override;

	// Filterable implementation
	virtual bool allowsFiltering() override;
	virtual std::string currentFilter() override;
	virtual void applyFilter(const std::string &filter) override;
	
	// HasSongs implementation
	virtual bool itemAvailable() override;
	virtual bool addItemToPlaylist(bool play) override;
	virtual std::vector<MPD::Song> getSelectedSongs() override;
	
	// other members
	MPD::Song nowPlayingSong();

	// Locate song in playlist.
	void locateSong(const MPD::Song &s);

	void enableHighlighting();
	
	void setSelectedItemsPriority(int prio);

	bool checkForSong(const MPD::Song &s);
	void registerSong(const MPD::Song &s);
	void unregisterSong(const MPD::Song &s);
	
	void reloadTotalLength() { m_reload_total_length = true; }
	void reloadRemaining() { m_reload_remaining = true; }
	
private:
	NcScreenCallbacks makeCallbacks();

	static Playlist *fromScreen(NcScreen *screen);
	static NcWindow *activeWindowCallback(NcScreen *screen);
	static void refreshCallback(NcScreen *screen);
	static void refreshWindowCallback(NcScreen *screen);
	static void scrollCallback(NcScreen *screen, enum NcScroll where);
	static void switchToCallback(NcScreen *screen);
	static void resizeCallback(NcScreen *screen);
	static int32 windowTimeoutCallback(NcScreen *screen);
	static char *titleCallback(NcScreen *screen);
	static void updateCallback(NcScreen *screen);
	static void mouseButtonPressedCallback(NcScreen *screen, MEVENT event);
	static bool isLockableCallback(NcScreen *screen);
	static bool isMergableCallback(NcScreen *screen);
	static void destroyCallback(NcScreen *screen);

	std::string getTotalLength();

	std::string m_stats;
	std::string m_title_cache;
	
	std::unordered_map<MPD::Song, int, MPD::Song::Hash> m_song_refs;
	
	size_t m_total_length;;
	size_t m_remaining_time;
	size_t m_scroll_begin;
	
	NcmTimePoint m_timer;

	bool m_reload_total_length;
	bool m_reload_remaining;

	NcPlaylistScreen m_screen;

	Regex::Filter<MPD::Song> m_search_predicate;
};

extern Playlist *myPlaylist;

#endif // NCMPCPP_PLAYLIST_H

