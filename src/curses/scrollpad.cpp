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

#include <iostream>

#include "curses/scrollpad.h"
#include "utility/storage_kind.h"

namespace {

NcScroll to_nc_scroll(NC::Scroll scroll)
{
	switch (scroll)
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

template <typename BeginT, typename EndT>
bool regexSearch(NC::Buffer &buf, const BeginT &begin, const std::string &ws,
                 const EndT &end, Regex::Flags flags, size_t id)
{
	try {
		return Regex::forEachMatch(
			buf.str(), ws, flags, [&buf, &begin, &end, id](size_t pos, size_t len) {
				buf.addProperty(pos, begin, id);
				buf.addProperty(pos + len, end, id);
			});
	} catch (Regex::Error &e) {
		std::cerr << "regexSearch: bad_expression: " << e.what() << "\n";
		return false;
	}
}

}

namespace NC {

Scrollpad::Scrollpad()
{
	nc_scrollpad_init_empty(&m_scrollpad);
}

Scrollpad::Scrollpad(size_t startx,
size_t starty,
size_t width,
size_t height,
const std::string &title,
Color color,
Border border)
: Window(startx, starty, width, height, title, color, border)
{
	nc_scrollpad_init(&m_scrollpad, static_cast<int64>(height));
}

void Scrollpad::refresh()
{
	nc_scrollpad_refresh(&m_scrollpad, cWindow());
	syncFromC();
}

void Scrollpad::resize(size_t new_width, size_t new_height)
{
	nc_scrollpad_resize(&m_scrollpad, cWindow(), new_width, new_height);
	syncFromC();
	flush();
}

void Scrollpad::scroll(Scroll where)
{
	nc_scrollpad_scroll(&m_scrollpad, cWindow(), to_nc_scroll(where));
}

void Scrollpad::clear()
{
	m_buffer.clear();
	nc_scrollpad_clear(&m_scrollpad, cWindow());
	syncFromC();
}

const std::string &Scrollpad::buffer()
{
	return m_buffer.str();
}

void Scrollpad::flush()
{
	auto &w = static_cast<Window &>(*this);
	const auto &s = m_buffer.str();
	const auto &ps = m_buffer.properties();
	auto p = ps.begin();
	size_t i = 0;
	
	auto load_properties = [&]() {
		for (; p != ps.end() && p->first == i; ++p)
			w << p->second;
	};
	auto write_whitespace = [&]() {
		for (; i < s.length() && iswspace(s[i]); ++i)
		{
			load_properties();
			w << s[i];
		}
	};
	auto write_word = [&](bool load_properties_) {
		for (; i < s.length() && !iswspace(s[i]); ++i)
		{
			if (load_properties_)
				load_properties();
			w << s[i];
		}
	};
	auto write_buffer = [&](bool generate_height_only) -> size_t {
		int new_y;
		size_t height = 1;
		size_t old_i;
		auto old_p = p;
		int x, y;
		i = 0;
		p = ps.begin();
		y = getY();
		while (i < s.length())
		{
			// write all whitespaces.
			write_whitespace();
			
			// if we are generating height, check difference
			// between previous Y coord and current one and
			// update height accordingly.
			if (generate_height_only)
			{
				new_y = getY();
				height += new_y - y;
				y = new_y;
			}
			
			if (i == s.length())
				break;
			
			// save current string position state and get current
			// coordinates as we are before the beginning of a word.
			old_i = i;
			old_p = p;
			x = getX();
			y = getY();
			
			// write word to test if it overflows, but do not load properties
			// yet since if it overflows, we do not want to load them twice.
			write_word(false);
			
			// restore previous indexes state
			i = old_i;
			p = old_p;
			
			// get new Y coord to see if word overflew into next line.
			new_y = getY();
			if (new_y != y)
			{
				if (generate_height_only)
				{
					// if it did, let's update height...
					++height;
				}
				else
				{
					// ...or go to old coordinates, erase
					// part of the string from previous line...
					goToXY(x, y);
					wclrtoeol(m_window);
				}
				
				// ...start at the beginning of next line...
				++y;
				goToXY(0, y);
				
				// ...write word again, this time with properties...
				write_word(true);
				
				if (generate_height_only)
				{
					// ... and check for potential
					// difference in Y coordinates again.
					new_y = getY();
					height += new_y - y;
				}
			}
			else
			{
				// word didn't overflow, rewrite it with properties.
				goToXY(x, y);
				write_word(true);
			}
			
			if (generate_height_only)
			{
				// move to the first line, since when we do
				// generation, m_real_height = m_height and we
				// don't have many lines to use.
				goToXY(getX(), 0);
				y = 0;
			}
		}
		// load remaining properties if there are any
		for (; p != ps.end(); ++p)
			w << p->second;
		return height;
	};
	nc_scrollpad_prepare_flush(&m_scrollpad, cWindow(), write_buffer(true));
	syncFromC();
	write_buffer(false);
}

void Scrollpad::reset()
{
	nc_scrollpad_reset(&m_scrollpad);
}

bool Scrollpad::setProperties(const Color &begin, const std::string &s,
                              const Color &end, Regex::Flags flags, size_t id)
{
	return regexSearch(m_buffer, begin, s, end, flags, id);
}

bool Scrollpad::setProperties(const Format &begin, const std::string &s,
                              const Format &end, Regex::Flags flags, size_t id)
{
	return regexSearch(m_buffer, begin, s, end, flags, id);
}

bool Scrollpad::setProperties(const FormattedColor &fc, const std::string &s,
                              Regex::Flags flags, size_t id)
{
	return regexSearch(m_buffer,
	                   fc,
	                   s,
	                   FormattedColor::End<StorageKind::Value>(fc),
	                   flags,
	                   id);
}

void Scrollpad::removeProperties(size_t id)
{
	m_buffer.removeProperties(id);
}

}
