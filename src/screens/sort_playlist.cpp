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

#include "curses/menu_impl.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "ui_state.h"
#include "helpers_legacy.h"
#include "screens/playlist.h"
#include "settings_legacy.h"
#include "screens/sort_playlist.h"
#include "statusbar_legacy.h"
#include "utility/comparators.h"
#include "screens/screen_cpp_switcher.h"

SortPlaylistDialog *mySortPlaylistDialog;

SortPlaylistDialog::SortPlaylistDialog()
{
	typedef WindowType::Item::Type Entry;
	
			
	setDimensions();
	w = WindowType((COLS-m_width)/2, (static_cast<size_t>(ui_state_main_height())-m_height)/2+static_cast<size_t>(ui_state_main_start_y()), m_width, m_height, "Sort songs by...", Config.main_color, Config.window_border);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer([](Self::WindowType &menu) {
		menu << Charset::utf8ToLocale(menu.drawn()->value().item().first);
	});
	
	w.addItem(Entry(std::make_pair("Artist", NCM_SONG_GETTER_ARTIST),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Album artist", NCM_SONG_GETTER_ALBUM_ARTIST),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Album", NCM_SONG_GETTER_ALBUM),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Disc", NCM_SONG_GETTER_DISC),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Track", NCM_SONG_GETTER_TRACK),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Genre", NCM_SONG_GETTER_GENRE),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Date", NCM_SONG_GETTER_DATE),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Composer", NCM_SONG_GETTER_COMPOSER),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Performer", NCM_SONG_GETTER_PERFORMER),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Title", NCM_SONG_GETTER_TITLE),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Filename", NCM_SONG_GETTER_URI),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addSeparator();
	w.addItem(Entry(std::make_pair("Sort", NCM_SONG_GETTER_NONE),
		std::bind(&Self::sort, this)
	));
	w.addItem(Entry(std::make_pair("Cancel", NCM_SONG_GETTER_NONE),
		std::bind(&Self::cancel, this)
	));
}

void SortPlaylistDialog::switchTo()
{
	SwitchTo::execute(this);
	w.reset();
}

void SortPlaylistDialog::resize()
{
			setDimensions();
	w.resize(m_width, m_height);
	w.moveTo((COLS-m_width)/2, (static_cast<size_t>(ui_state_main_height())-m_height)/2+static_cast<size_t>(ui_state_main_start_y()));
	hasToBeResized = false;
}

std::string SortPlaylistDialog::title()
{
	return previousScreen()->title();
}

void SortPlaylistDialog::mouseButtonPressed(MEVENT me)
{
	if (w.hasCoords(me.x, me.y))
	{
		if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
		{
			w.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				runAction();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

/**********************************************************************/

bool SortPlaylistDialog::actionRunnable()
{
	return !w.empty();
}

void SortPlaylistDialog::runAction()
{
	w.current()->value().run();
}

/**********************************************************************/

void SortPlaylistDialog::moveSortOrderDown()
{
	auto cur = w.currentV();
	if ((cur+1)->item().second != NCM_SONG_GETTER_NONE)
	{
		std::iter_swap(cur, cur+1);
		w.scroll(NC_SCROLL_DOWN);
	}
}

void SortPlaylistDialog::moveSortOrderUp()
{
	auto cur = w.currentV();
	if (cur > w.beginV() && cur->item().second != NCM_SONG_GETTER_NONE)
	{
		std::iter_swap(cur, cur-1);
		w.scroll(NC_SCROLL_UP);
	}
}

void SortPlaylistDialog::moveSortOrderHint() const
{
	Statusbar::print("Move tag types up and down to adjust sort order");
}

void SortPlaylistDialog::sort() const
{
	auto &pl = myPlaylist->main();
	auto begin = pl.begin(), end = pl.end();
	if (!findSelectedRange(begin, end))
		return;

	size_t start_pos = begin - pl.begin();
	std::vector<MPD::Song> playlist;
	playlist.reserve(end - begin);
	for (; begin != end; ++begin)
		playlist.push_back(begin->value());
	
	typedef std::vector<MPD::Song>::iterator Iterator;
	LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
	std::function<void(Iterator, Iterator)> iter_swap, quick_sort;
	auto song_cmp = [this, &cmp](const MPD::Song &a, const MPD::Song &b) -> bool {
		for (auto it = w.beginV(); it->item().second != NCM_SONG_GETTER_NONE; ++it)
		{
			int res = cmp(a.getTags(it->item().second),
			              b.getTags(it->item().second));
			if (res != 0)
				return res < 0;
		}
		return a.getPosition() < b.getPosition();
	};
	iter_swap = [&playlist, &start_pos](Iterator a, Iterator b) {
		std::iter_swap(a, b);
		Mpd.Swap(start_pos+a-playlist.begin(), start_pos+b-playlist.begin());
	};
	quick_sort = [&song_cmp, &quick_sort, &iter_swap](Iterator first, Iterator last) {
		if (last-first > 1)
		{
			Iterator pivot = first + ncm_random_range_i32(&global_random,
			                                      last - first);
			iter_swap(pivot, last-1);
			pivot = last-1;
			
			Iterator tmp = first;
			for (Iterator i = first; i != pivot; ++i)
				if (song_cmp(*i, *pivot))
					iter_swap(i, tmp++);
			iter_swap(tmp, pivot);
			
			quick_sort(first, tmp);
			quick_sort(tmp+1, last);
		}
	};
	
	Statusbar::print("Sorting...");
	Mpd.StartCommandsList();
	quick_sort(playlist.begin(), playlist.end());
	Mpd.CommitCommandsList();
	Statusbar::print("Range sorted");
	switchToPreviousScreen();
}

void SortPlaylistDialog::cancel() const
{
	switchToPreviousScreen();
}

void SortPlaylistDialog::setDimensions()
{
	m_height = std::min(size_t(17), static_cast<size_t>(ui_state_main_height()));
	m_width = 30;
}
