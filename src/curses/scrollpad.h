#ifndef NCMPCPP_SCROLLPAD_H
#define NCMPCPP_SCROLLPAD_H

#include "curses/window.h"
#include "curses/nc_scrollpad.h"
#include "curses/strbuffer.h"
#include "utility/regex.h"

namespace NC {

/// Scrollpad is specialized window that holds large portions of text and
/// supports scrolling if the amount of it is bigger than the window area.
struct Scrollpad: public Window
{
	Scrollpad();
	
	Scrollpad(size_t startx, size_t starty, size_t width, size_t height,
	          const std::string &title, Color color, Border border);
	
	// override a few Window functions
	virtual void refresh() override;
	virtual void scroll(enum NcScroll where) override;
	virtual void resize(size_t new_width, size_t new_height) override;
	virtual void clear() override;
	
	void flush();
	void reset();
	
	bool setProperties(enum NcFormat begin, const std::string &s, enum NcFormat end,
	                   Regex::Flags flags, size_t id = -2);
	void removeProperties(size_t id = -2);
	
	Scrollpad &operator<<(int n) { return write(n); }
	Scrollpad &operator<<(long int n) { return write(n); }
	Scrollpad &operator<<(unsigned int n) { return write(n); }
	Scrollpad &operator<<(unsigned long int n) { return write(n); }
	Scrollpad &operator<<(char c) { return write(c); }
	Scrollpad &operator<<(const char *s) { return write(s); }
	Scrollpad &operator<<(const std::string &s) { return write(s); }
	Scrollpad &operator<<(Color color) { return write(color); }
	Scrollpad &operator<<(enum NcFormat format) { return write(format); }

private:
	template <typename ItemT>
	Scrollpad &write(ItemT &&item)
	{
		m_buffer << std::forward<ItemT>(item);
		return *this;
	}

	Buffer m_buffer;
	NcScrollpad m_scrollpad;
};

}

#include "curses/scrollpad_compat_impl.h"

#endif // NCMPCPP_SCROLLPAD_H

