#if !defined(SRC_CURSES_SCROLLPAD_COMPAT_IMPL_H)
#define SRC_CURSES_SCROLLPAD_COMPAT_IMPL_H

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

#include "utility/storage_kind.h"

namespace {

NcScroll nc_scrollpad_compat_to_nc_scroll(NC::Scroll scroll)
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

inline Scrollpad::Scrollpad()
{
	nc_scrollpad_init_empty(&m_scrollpad);
}

inline Scrollpad::Scrollpad(size_t startx,
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

inline void Scrollpad::refresh()
{
	nc_scrollpad_refresh(&m_scrollpad, cWindow());
	syncFromC();
}

inline void Scrollpad::resize(size_t new_width, size_t new_height)
{
	nc_scrollpad_resize(&m_scrollpad, cWindow(), new_width, new_height);
	syncFromC();
	flush();
}

inline void Scrollpad::scroll(Scroll where)
{
	nc_scrollpad_scroll(&m_scrollpad, cWindow(), nc_scrollpad_compat_to_nc_scroll(where));
}

inline void Scrollpad::clear()
{
	m_buffer.clear();
	nc_scrollpad_clear(&m_scrollpad, cWindow());
	syncFromC();
}

inline const std::string &Scrollpad::buffer()
{
	return m_buffer.str();
}

inline void Scrollpad::flush()
{
	nc_scrollpad_flush(&m_scrollpad, cWindow(), m_buffer.cBuffer());
	syncFromC();
}

inline void Scrollpad::reset()
{
	nc_scrollpad_reset(&m_scrollpad);
}

inline bool Scrollpad::setProperties(const Color &begin, const std::string &s,
                              const Color &end, Regex::Flags flags, size_t id)
{
	return regexSearch(m_buffer, begin, s, end, flags, id);
}

inline bool Scrollpad::setProperties(const Format &begin, const std::string &s,
                              const Format &end, Regex::Flags flags, size_t id)
{
	return regexSearch(m_buffer, begin, s, end, flags, id);
}

inline bool Scrollpad::setProperties(const FormattedColor &fc, const std::string &s,
                              Regex::Flags flags, size_t id)
{
	return regexSearch(m_buffer,
	                   fc,
	                   s,
	                   FormattedColor::End<StorageKind::Value>(fc),
	                   flags,
	                   id);
}

inline void Scrollpad::removeProperties(size_t id)
{
	m_buffer.removeProperties(id);
}

}

#endif /* SRC_CURSES_SCROLLPAD_COMPAT_IMPL_H */
