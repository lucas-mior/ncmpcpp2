#if !defined(SRC_CURSES_SCROLLPAD_COMPAT_IMPL_H)
#define SRC_CURSES_SCROLLPAD_COMPAT_IMPL_H

#include <iostream>

#include "utility/storage_kind.h"

namespace {

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

inline void Scrollpad::scroll(enum NcScroll where)
{
	nc_scrollpad_scroll(&m_scrollpad, cWindow(), where);
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

inline bool Scrollpad::setProperties(enum NcFormat begin, const std::string &s,
                              enum NcFormat end, Regex::Flags flags, size_t id)
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
