#if !defined(SRC_CURSES_WINDOW_COMPAT_IMPL_H)
#define SRC_CURSES_WINDOW_COMPAT_IMPL_H

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
#include <cstdlib>
#include <utility>
#include <iostream>
#include <cassert>

namespace NC {


inline std::string keyToString(NcKey key)
{
	char buffer[64];

	if (nc_key_name(key, buffer, sizeof(buffer)) < 0)
		return std::to_string(key);
	return buffer;
}

inline const short Color::transparent = -1;
inline const short Color::current = -2;

inline Color Color::Default(0, 0, true, false);
inline Color Color::Black(COLOR_BLACK, Color::current);
inline Color Color::Red(COLOR_RED, Color::current);
inline Color Color::Green(COLOR_GREEN, Color::current);
inline Color Color::Yellow(COLOR_YELLOW, Color::current);
inline Color Color::Blue(COLOR_BLUE, Color::current);
inline Color Color::Magenta(COLOR_MAGENTA, Color::current);
inline Color Color::Cyan(COLOR_CYAN, Color::current);
inline Color Color::White(COLOR_WHITE, Color::current);
inline Color Color::End(0, 0, false, true);

inline int Color::pairNumber() const
{
	return nc_color_pair_number(
		nc_color_make(foreground(), background(), isDefault(), isEnd()));
}

inline std::istream &operator>>(std::istream &is, Color &c)
{
	const short invalid_color_value = -1337;
	auto get_single_color = [](const std::string &s, bool background) {
		short result = invalid_color_value;
		if (s == "black")
			result = COLOR_BLACK;
		else if (s == "red")
			result = COLOR_RED;
		else if (s == "green")
			result = COLOR_GREEN;
		else if (s == "yellow")
			result = COLOR_YELLOW;
		else if (s == "blue")
			result = COLOR_BLUE;
		else if (s == "magenta")
			result = COLOR_MAGENTA;
		else if (s == "cyan")
			result = COLOR_CYAN;
		else if (s == "white")
			result = COLOR_WHITE;
		else if (background && s == "transparent")
			result = NC::Color::transparent;
		else if (background && s == "current")
			result = NC::Color::current;
		else if (std::all_of(s.begin(), s.end(), isdigit))
		{
			result = atoi(s.c_str());
			if (result < (background ? 0 : 1) || result > 256)
				result = invalid_color_value;
			else
				--result;
		}
		return result;
	};

	auto get_color = [](std::istream &is_) {
		std::string result;
		while (!is_.eof() && isalnum(is_.peek()))
			result.push_back(is_.get());
		return result;
	};

	std::string sc = get_color(is);

	if (sc == "default")
		c = Color::Default;
	else if (sc == "end")
		c = Color::End;
	else
	{
		short fg = get_single_color(sc, false);
		if (fg == invalid_color_value)
			is.setstate(std::ios::failbit);
		// Check if there is background color
		else if (!is.eof() && is.peek() == '_')
		{
			is.get();
			sc = get_color(is);
			short bg = get_single_color(sc, true);
			if (bg == invalid_color_value)
				is.setstate(std::ios::failbit);
			else
				c = Color(fg, bg);
		}
		else
			c = Color(fg, NC::Color::current);
	}
	return is;
}

inline enum NcFormat reverseFormat(enum NcFormat format)
{
	return nc_format_reverse(format);
}


inline void initReadline()
{
	nc_init_readline();
}


inline void initScreen(bool enable_colors, bool enable_mouse)
{
	nc_init_screen(enable_colors, enable_mouse);
	initReadline();
}

inline int colorCount()
{
	return nc_color_count();
}

inline void pauseScreen()
{
	nc_pause_screen();
}

inline void unpauseScreen()
{
	nc_unpause_screen();
}

inline void destroyScreen()
{
	nc_destroy_screen();
}

inline Window::Window()
{
	nc_window_init_empty(&m_impl);
	syncFromC();
}

inline Window::Window(size_t startx, size_t starty, size_t width, size_t height,
               std::string title, Color color, Border border)
	: m_prompt_hook(0)
{
	nc_window_init(&m_impl,
	               startx,
	               starty,
	               width,
	               height,
	               const_cast<char *>(title.c_str()),
	               static_cast<int32>(title.length()),
	               toNcColor(color),
	               toNcBorder(border));
	syncFromC();
}

inline Window::Window(const Window &rhs)
	: m_prompt_hook(rhs.m_prompt_hook)
{
	nc_window_copy(&m_impl, const_cast<NcWindow *>(&rhs.m_impl));
	syncFromC();
}

inline Window::Window(Window &&rhs)
	: m_prompt_hook(std::move(rhs.m_prompt_hook))
{
	nc_window_move(&m_impl, &rhs.m_impl);
	syncFromC();
	rhs.syncFromC();
}

inline Window &Window::operator=(Window rhs)
{
	nc_window_swap(&m_impl, &rhs.m_impl);
	std::swap(m_prompt_hook, rhs.m_prompt_hook);
	syncFromC();
	rhs.syncFromC();
	return *this;
}

inline Window::~Window()
{
	nc_window_destroy(&m_impl);
}

inline NcColor Window::toNcColor(Color color)
{
	return nc_color_make(color.foreground(),
	                     color.background(),
	                     color.isDefault(),
	                     color.isEnd());
}

inline Color Window::fromNcColor(NcColor color)
{
	if (color.is_default)
		return Color::Default;
	if (color.is_end)
		return Color::End;
	return Color(color.foreground, color.background);
}

inline NcBorder Window::toNcBorder(Border border)
{
	NcBorder result = {};
	if (border)
	{
		result.enabled = true;
		result.color = toNcColor(*border);
	}
	return result;
}

inline Border Window::fromNcBorder(NcBorder border)
{
	if (!border.enabled)
		return std::nullopt;
	return fromNcColor(border.color);
}

inline NcFormat Window::toNcFormat(enum NcFormat format)
{
	return format;
}


inline NcScroll Window::toNcScroll(enum NcScroll scroll)
{
	return scroll;
}


inline void Window::syncFromC()
{
	m_window = nc_window_raw(&m_impl);
	m_start_x = m_impl.start_x;
	m_start_y = m_impl.start_y;
	m_width = m_impl.width;
	m_height = m_impl.height;
	m_window_timeout = nc_window_timeout(&m_impl);
	m_color = fromNcColor(nc_window_color(&m_impl));
	m_base_color = fromNcColor(nc_window_base_color(&m_impl));
	m_border = fromNcBorder(nc_window_border(&m_impl));

	char *title = nc_window_title(&m_impl);
	int32 title_len = nc_window_title_len(&m_impl);
	if (title != nullptr)
		m_title.assign(title, title_len);
	else
		m_title.clear();
}

inline NcWindow *Window::cWindow()
{
	return &m_impl;
}

const inline NcWindow *Window::cWindow() const
{
	return &m_impl;
}

inline NcWindow *Window::nativeWindow()
{
	return cWindow();
}

const inline NcWindow *Window::nativeWindow() const
{
	return cWindow();
}

inline void Window::setColor(Color c)
{
	nc_window_set_color(&m_impl, toNcColor(c));
	syncFromC();
}

inline void Window::setBaseColor(const Color &color)
{
	nc_window_set_base_color(&m_impl, toNcColor(color));
	syncFromC();
}

inline void Window::setBorder(Border border)
{
	nc_window_set_border(&m_impl, toNcBorder(border));
	syncFromC();
}

inline void Window::setTitle(const std::string &new_title)
{
	nc_window_set_title(&m_impl,
	                    const_cast<char *>(new_title.c_str()),
	                    static_cast<int32>(new_title.length()));
	syncFromC();
}

inline void Window::recreate(size_t width, size_t height)
{
	nc_window_recreate(&m_impl, width, height);
	syncFromC();
}

inline void Window::moveTo(size_t new_x, size_t new_y)
{
	nc_window_move_to(&m_impl, new_x, new_y);
	syncFromC();
}

inline void Window::adjustDimensions(size_t width, size_t height)
{
	nc_window_adjust_dimensions(&m_impl, width, height);
	syncFromC();
}

inline void Window::resize(size_t new_width, size_t new_height)
{
	nc_window_resize(&m_impl, new_width, new_height);
	syncFromC();
}

inline void Window::refreshBorder() const
{
	nc_window_refresh_border(const_cast<NcWindow *>(&m_impl));
}

inline void Window::display()
{
	refreshBorder();
	refresh();
	refresh();
}

inline void Window::refresh()
{
	nc_window_refresh(&m_impl);
}

inline void Window::clear()
{
	nc_window_clear(&m_impl);
	syncFromC();
}

inline void Window::setTimeout(int timeout)
{
	nc_window_set_timeout(&m_impl, timeout);
	syncFromC();
}

inline void Window::addFDCallback(int fd, void (*callback)())
{
	nc_window_add_fd_callback(&m_impl, fd, callback);
}

inline void Window::clearFDCallbacksList()
{
	nc_window_clear_fd_callbacks(&m_impl);
}

inline bool Window::FDCallbacksListEmpty() const
{
	return nc_window_fd_callbacks_empty(const_cast<NcWindow *>(&m_impl));
}

inline NcKey Window::readKey()
{
	return nc_window_read_key(&m_impl);
}

inline void Window::pushChar(NcKey ch)
{
	nc_window_push_key(&m_impl, ch);
}

inline bool windowPromptHook(char *arg, void *data)
{
	Window *window = static_cast<Window *>(data);
	bool done = false;

	if (!window->runPromptHook(arg, &done))
		return true;
	return !done;
}

inline std::string Window::prompt(const std::string &base, size_t width, bool encrypted)
{
	std::string result;
	char *input = nullptr;
	NcPrompt prompt = {};

	prompt.initial_text = const_cast<char *>(base.c_str());
	if (width == static_cast<size_t>(-1))
		prompt.width = -1;
	else
		prompt.width = static_cast<int64>(width);
	prompt.hook = windowPromptHook;
	prompt.hook_user_data = this;
	prompt.encrypted = encrypted;
	prompt.remember = !encrypted;

	if (nc_window_prompt(&m_impl, &prompt, &input) == NC_PROMPT_ACCEPTED)
	{
		if (input != nullptr)
			result = input;
		nc_window_prompt_result_destroy(input);
		return result;
	}

	if (input != nullptr)
	{
		result = input;
		nc_window_prompt_result_destroy(input);
	}
	throw PromptAborted(std::move(result));
}

inline void Window::goToXY(int x, int y)
{
	nc_window_go_to_xy(&m_impl, x, y);
}

inline int Window::getX()
{
	return nc_window_get_x(&m_impl);
}

inline int Window::getY()
{
	return nc_window_get_y(&m_impl);
}

inline bool Window::hasCoords(int &x, int &y)
{
	int32 local_x = x;
	int32 local_y = y;
	bool result = nc_window_has_coords(&m_impl, &local_x, &local_y);
	if (result)
	{
		x = local_x;
		y = local_y;
	}
	return result;
}

inline bool Window::runPromptHook(const char *arg, bool *done) const
{
	if (m_prompt_hook)
	{
		bool continue_ = m_prompt_hook(arg);
		if (done != nullptr)
			*done = !continue_;
		return true;
	}
	else
		return false;
}

inline size_t Window::getWidth() const
{
	return nc_window_width(const_cast<NcWindow *>(&m_impl));
}

inline size_t Window::getHeight() const
{
	return nc_window_height(const_cast<NcWindow *>(&m_impl));
}

inline size_t Window::getStartX() const
{
	return nc_window_start_x(const_cast<NcWindow *>(&m_impl));
}

inline size_t Window::getStarty() const
{
	return nc_window_start_y(const_cast<NcWindow *>(&m_impl));
}

inline const std::string &Window::getTitle() const
{
	return m_title;
}

inline const Color &Window::getColor() const
{
	return m_color;
}

inline const Border &Window::getBorder() const
{
	return m_border;
}

inline int Window::getTimeout() const
{
	return m_window_timeout;
}

inline const MEVENT &Window::getMouseEvent()
{
	return *nc_window_mouse_event(&m_impl);
}

inline void Window::scroll(enum NcScroll where)
{
	nc_window_scroll(&m_impl, toNcScroll(where));
}

inline Window &Window::operator<<(const Color &c)
{
	nc_window_push_color(&m_impl, toNcColor(c));
	syncFromC();
	return *this;
}

inline Window &Window::operator<<(enum NcFormat format)
{
	nc_window_apply_format(&m_impl, toNcFormat(format));
	return *this;
}

inline Window &Window::operator<<(TermManip tm)
{
	switch (tm)
	{
	case TermManip::ClearToEOL:
		nc_window_apply_term_manip(&m_impl, NC_TERM_CLEAR_TO_EOL);
		break;
	}
	return *this;
}

inline Window &Window::operator<<(const XY &coords)
{
	goToXY(coords.x, coords.y);
	return *this;
}

inline Window &Window::operator<<(const char *s)
{
	nc_window_print_cstring(&m_impl, const_cast<char *>(s));
	return *this;
}

inline Window &Window::operator<<(char c)
{
	nc_window_print_char(&m_impl, c);
	return *this;
}

inline Window &Window::operator<<(int i)
{
	nc_window_print_int32(&m_impl, i);
	return *this;
}

inline Window &Window::operator<<(double d)
{
	nc_window_print_double(&m_impl, d);
	return *this;
}

inline Window &Window::operator<<(const std::string &s)
{
	nc_window_print_data(&m_impl,
	                     const_cast<char *>(s.c_str()),
	                     static_cast<int32>(s.length()));
	return *this;
}

inline Window &Window::operator<<(size_t s)
{
	nc_window_print_uint64(&m_impl, s);
	return *this;
}


}

#endif /* SRC_CURSES_WINDOW_COMPAT_IMPL_H */
