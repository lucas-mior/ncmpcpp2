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
#include <time.h>

#include "c/ncm_enums.h"
#include "helpers_legacy.h"
#include "format_impl.h"
#include "screens/playlist.h"
#include "statusbar_legacy.h"
#include "utility/functional.h"

const MPD::Song *currentSong(const BaseScreen *screen)
{
	const MPD::Song *ptr = nullptr;
	const auto *list = dynamic_cast<const SongList *>(screen->activeWindow());
	if (list != nullptr)
	{
		const auto it = list->currentS();
		if (it != list->endS())
			ptr = it->song();
	}
	return ptr;
}

void deleteSelectedSongsFromPlaylist(NC::Menu<MPD::Song> &playlist)
{
	selectCurrentIfNoneSelected(playlist);
	bool in_range = false;
	int range_end = 0;
	Mpd.StartCommandsList();
	for (auto it = playlist.rbegin(); it != playlist.rend(); ++it)
	{
		auto &s = *it;
		if (s.isSelected())
		{
			s.setSelected(false);
			if (!in_range)
			{
				range_end = s.value().getPosition() + 1;
				in_range = true;
			}
		}
		else if (in_range)
		{
			Mpd.DeleteRange(s.value().getPosition() + 1, range_end);
			in_range = false;
		}
	}
	if (in_range)
		Mpd.DeleteRange(0, range_end);
	Mpd.CommitCommandsList();
}

void removeSongFromPlaylist(const SongMenu &playlist, const MPD::Song &s)
{
	Mpd.StartCommandsList();
	for (auto it = playlist.rbegin(); it != playlist.rend(); ++it)
	{
		const auto &item = *it;
		if (item.value() == s)
			Mpd.Delete(item.value().getPosition());
	}
	Mpd.CommitCommandsList();
}

bool addSongToPlaylist(const MPD::Song &s, bool play, int position)
{
	bool result = false;
	if (Config.space_add_mode == NCM_SPACE_ADD_MODE_ADD_REMOVE
	&&  myPlaylist->checkForSong(s)
	   )
	{
		result = true;
		if (play)
		{
			const auto begin = myPlaylist->main().beginV(), end = myPlaylist->main().endV();
			auto it = find_map_first(begin, end, s, [](const MPD::Song &found) {
				Mpd.PlayID(found.getID());
			});
			assert(it != end);
		}
		else
			removeSongFromPlaylist(myPlaylist->main(), s);
		return result;
	}
	int id = Mpd.AddSong(s.cSong(), position);
	if (id >= 0)
	{
		Statusbar::printf("Added to playlist: %s",
			Format::stringify<char>(Config.song_status_format, &s)
		);
		if (play)
			Mpd.PlayID(id);
		result = true;
	}
	return result;
}

std::string timeFormat(const char *format, time_t t)
{
	char result[32];
	tm tinfo;
	localtime_r(&t, &tinfo);
	strftime(result, sizeof(result), format, &tinfo);
	return result;
}

std::string Timestamp(time_t t)
{
	char result[32];
	tm info;
	result[strftime(result, 31, "%x %X", localtime_r(&t, &info))] = 0;
	return result;
}

std::string Scroller(const std::string &str, size_t &pos, size_t width)
{
	std::string s(str);
	if (!Config.header_text_scrolling)
		return s;
	std::string result;
	size_t len = Utf8::width(s);
	
	if (len > width)
	{
		s += " ** ";
		auto chars = Utf8::split(s);
		len = 0;
		size_t i = pos;
		for (; i < chars.size() && len < width; ++i)
		{
			if ((len += Utf8::width(chars[i])) > width)
				break;
			result += chars[i];
		}
		if (++pos >= chars.size())
			pos = 0;
		for (i = 0; len < width && i < chars.size(); ++i)
		{
			if ((len += Utf8::width(chars[i])) > width)
				break;
			result += chars[i];
		}
	}
	else
		result = s;
	return result;
}

void writeCyclicBuffer(const NC::Buffer &buf, NC::Window &w, size_t &start_pos,
                       size_t width, const std::string &separator)
{
	const auto &s = buf.str();
	size_t string_width = Utf8::width(s);
	if (string_width > width)
	{
		const auto string_length = Utf8::characters(s);
		const auto separator_length = Utf8::characters(separator);
		if (start_pos >= string_length + separator_length)
			start_pos = 0;

		size_t len = 0;
		const auto &ps = buf.properties();
		auto p = ps.begin();
		
		// load attributes from before starting pos
		const size_t start_byte = Utf8::bytePosition(s, start_pos);
		for (; p != ps.end() && p->first < start_byte; ++p)
			w << p->second;
		
		auto write_buffer = [&](size_t start) {
			for (size_t i = start; i < s.size() && len < width;)
			{
				for (; p != ps.end() && p->first == i; ++p)
					w << p->second;
				size_t next = Utf8::nextPosition(s, i);
				std::string ch = s.substr(i, next-i);
				len += Utf8::width(ch);
				if (len > width)
					break;
				w << ch;
				i = next;
			}
			for (; p != ps.end(); ++p)
				w << p->second;
			p = ps.begin();
		};
		
		write_buffer(start_byte);

		auto separator_chars = Utf8::split(separator);
		size_t i = 0;
		if (start_pos > string_length)
			i = start_pos - string_length;
		for (; i < separator_chars.size() && len < width; ++i)
		{
			len += Utf8::width(separator_chars[i]);
			if (len > width)
				break;
			w << separator_chars[i];
		}
		write_buffer(0);
		
		++start_pos;
		if (start_pos >= string_length + separator_length)
			start_pos = 0;
	}
	else
		w << buf;
}
