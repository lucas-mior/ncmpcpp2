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
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <utility>
#include <iostream>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include "utility/readline.h"
#include "utility/string.h"
#include "utility/utf8.h"
#include "window.h"
#include <cassert>

namespace {

namespace rl {

bool aborted;

NC::Window *w;
size_t start_x;
size_t start_y;
size_t width;
bool encrypted;
const char *base;

int read_key(FILE *)
{
	size_t x;
	bool done;
	int result;
	do
	{
		x = w->getX();
		if (w->runPromptHook(rl_line_buffer, &done))
		{
			// Do not end if readline is in one of its commands, e.g. searching
			// through history, as it doesn't actually make readline exit and it
			// becomes stuck in a loop.
			if (!RL_ISSTATE(RL_STATE_DISPATCHING) && done)
			{
				rl_done = 1;
				return EOF;
			}
			w->goToXY(x, start_y);
		}
		w->refresh();
		result = w->readKey();
		if (!w->FDCallbacksListEmpty())
		{
			w->goToXY(x, start_y);
			w->refresh();
		}
	}
	while (result == ERR);
	return result;
}

void display_string()
{
	auto print_string = [](const std::string &s) {
		if (encrypted)
			*w << std::string(Utf8::characters(s), '*');
		else
			*w << s;
	};

	std::string before_cursor(rl_line_buffer, rl_point);
	std::string after_cursor(rl_line_buffer+rl_point, rl_end-rl_point);
	int pos = Utf8::width(before_cursor);

	// clear the area for the string
	mvwhline(w->raw(), start_y, start_x, ' ', width+1);

	w->goToXY(start_x, start_y);
	if (size_t(pos) <= width)
	{
		// if the current position in the string is not bigger than allowed
		// width, print the part of the string before cursor position...

		print_string(before_cursor);

		// ...and then print the rest char-by-char until there is no more area
		size_t cpos = pos;
		for (const auto &c : Utf8::split(after_cursor))
		{
			size_t n = Utf8::width(c);
			if (cpos+n > width)
				break;
			cpos += n;
			print_string(c);
		}
	}
	else
	{
		// if the current position in the string is bigger than allowed
		// width, we always keep the cursor at the end of the line (it
		// would be nice to have more flexible scrolling, but for now
		// let's stick to that) by cutting the beginning of the part
		// of the string before the cursor until it fits the area.

		auto chars = Utf8::split(before_cursor);
		std::string tail;
		pos = 0;
		for (auto it = chars.rbegin(); it != chars.rend(); ++it)
		{
			size_t n = Utf8::width(*it);
			if (size_t(pos)+n > width)
				break;
			pos += n;
			tail.insert(0, *it);
		}
		print_string(tail);
	}
	w->goToXY(start_x+pos, start_y);
}

int add_base()
{
	rl_insert_text(base);
	return 0;
}

}

}

namespace NC {


std::string keyToString(Key::Type key)
{
	std::string result;
	if (key & Key::Ctrl)
		result += "Ctrl+";
	if (key & Key::Alt)
		result += "Alt+";
	if (key & Key::Shift)
		result += "Shift+";

	Key::Type base = key & ~(Key::Ctrl | Key::Alt | Key::Shift);
	if (base >= Key::F1 && base <= Key::F12)
		result += "F" + std::to_string(base-Key::F1+1);
	else
	{
		switch (base)
		{
			case Key::Space: result += "Space"; break;
			case Key::Backspace: result += "Backspace"; break;
			case Key::Tab: result += "Tab"; break;
			case Key::Enter: result += "Enter"; break;
			case Key::Escape: result += "Escape"; break;
			case Key::Insert: result += "Insert"; break;
			case Key::Delete: result += "Delete"; break;
			case Key::Home: result += "Home"; break;
			case Key::End: result += "End"; break;
			case Key::PageUp: result += "PageUp"; break;
			case Key::PageDown: result += "PageDown"; break;
			case Key::Up: result += "Up"; break;
			case Key::Down: result += "Down"; break;
			case Key::Left: result += "Left"; break;
			case Key::Right: result += "Right"; break;
			case Key::Mouse: result += "Mouse"; break;
			case Key::EoF: result += "EoF"; break;
			default:
				if (base >= 32 && base <= 126)
					result += static_cast<char>(base);
				else if (base >= 1 && base <= 26)
				{
					if (result.empty())
						result += "Ctrl+";
					result += static_cast<char>('A'+base-1);
				}
				else
					result += std::to_string(base);
		}
	}
	return result;
}

const short Color::transparent = -1;
const short Color::current = -2;

Color Color::Default(0, 0, true, false);
Color Color::Black(COLOR_BLACK, Color::current);
Color Color::Red(COLOR_RED, Color::current);
Color Color::Green(COLOR_GREEN, Color::current);
Color Color::Yellow(COLOR_YELLOW, Color::current);
Color Color::Blue(COLOR_BLUE, Color::current);
Color Color::Magenta(COLOR_MAGENTA, Color::current);
Color Color::Cyan(COLOR_CYAN, Color::current);
Color Color::White(COLOR_WHITE, Color::current);
Color Color::End(0, 0, false, true);

int Color::pairNumber() const
{
	return nc_color_pair_number(
		nc_color_make(foreground(), background(), isDefault(), isEnd()));
}

std::istream &operator>>(std::istream &is, Color &c)
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

NC::Format reverseFormat(NC::Format fmt)
{
	switch (fmt)
	{
	case NC::Format::Bold:
		return NC::Format::NoBold;
	case NC::Format::NoBold:
		return NC::Format::Bold;
	case NC::Format::Underline:
		return NC::Format::NoUnderline;
	case NC::Format::NoUnderline:
		return NC::Format::Underline;
	case NC::Format::Reverse:
		return NC::Format::NoReverse;
	case NC::Format::NoReverse:
		return NC::Format::Reverse;
	case NC::Format::AltCharset:
		return NC::Format::NoAltCharset;
	case NC::Format::NoAltCharset:
		return NC::Format::AltCharset;
	case NC::Format::Italic:
		return NC::Format::NoItalic;
	case NC::Format::NoItalic:
		return NC::Format::Italic;
	}
	// Unreachable, silence GCC.
	return fmt;
}

namespace Mouse {

void enable()
{
	nc_mouse_enable();
}

void disable()
{
	nc_mouse_disable();
}

}

int colorCount()
{
	return nc_color_count();
}

void initScreen(bool enable_colors, bool enable_mouse)
{
	nc_init_screen(enable_colors, enable_mouse);

	// initialize readline (needed, otherwise we get segmentation
	// fault on SIGWINCH). also, initialize first as doing this
	// later erases keys bound with rl_bind_key for some users.
	rl_initialize();
	// disable autocompletion
	rl_attempted_completion_function = [](const char *, int, int) -> char ** {
		rl_attempted_completion_over = 1;
		return nullptr;
	};
	auto abort_prompt = [](int, int) -> int {
		rl::aborted = true;
		rl_done = 1;
		return 0;
	};
	// if ctrl-c or ctrl-g is pressed, abort the prompt
	rl_bind_key('\3', abort_prompt);
	rl_bind_key('\7', abort_prompt);
	// do not change the state of the terminal
	rl_prep_term_function = nullptr;
	rl_deprep_term_function = nullptr;
	// do not catch signals
	rl_catch_signals = 0;
	rl_catch_sigwinch = 0;
	// overwrite readline callbacks
	rl_getc_function = rl::read_key;
	rl_redisplay_function = rl::display_string;
	rl_startup_hook = rl::add_base;
}

void pauseScreen()
{
	nc_pause_screen();
}

void unpauseScreen()
{
	nc_unpause_screen();
}

void destroyScreen()
{
	nc_destroy_screen();
}

Window::Window()
{
	nc_window_init_empty(&m_impl);
	syncFromC();
}

Window::Window(size_t startx, size_t starty, size_t width, size_t height,
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

Window::Window(const Window &rhs)
	: m_prompt_hook(rhs.m_prompt_hook)
{
	nc_window_copy(&m_impl, const_cast<NcWindow *>(&rhs.m_impl));
	syncFromC();
}

Window::Window(Window &&rhs)
	: m_prompt_hook(std::move(rhs.m_prompt_hook))
{
	nc_window_move(&m_impl, &rhs.m_impl);
	syncFromC();
	rhs.syncFromC();
}

Window &Window::operator=(Window rhs)
{
	nc_window_swap(&m_impl, &rhs.m_impl);
	std::swap(m_prompt_hook, rhs.m_prompt_hook);
	syncFromC();
	rhs.syncFromC();
	return *this;
}

Window::~Window()
{
	nc_window_destroy(&m_impl);
}

NcColor Window::toNcColor(Color color)
{
	return nc_color_make(color.foreground(),
	                     color.background(),
	                     color.isDefault(),
	                     color.isEnd());
}

Color Window::fromNcColor(NcColor color)
{
	if (color.is_default)
		return Color::Default;
	if (color.is_end)
		return Color::End;
	return Color(color.foreground, color.background);
}

NcBorder Window::toNcBorder(Border border)
{
	NcBorder result = {};
	if (border)
	{
		result.enabled = true;
		result.color = toNcColor(*border);
	}
	return result;
}

Border Window::fromNcBorder(NcBorder border)
{
	if (!border.enabled)
		return std::nullopt;
	return fromNcColor(border.color);
}

NcFormat Window::toNcFormat(Format format)
{
	switch (format)
	{
	case Format::Bold:
		return NC_FORMAT_BOLD;
	case Format::NoBold:
		return NC_FORMAT_NO_BOLD;
	case Format::Underline:
		return NC_FORMAT_UNDERLINE;
	case Format::NoUnderline:
		return NC_FORMAT_NO_UNDERLINE;
	case Format::Reverse:
		return NC_FORMAT_REVERSE;
	case Format::NoReverse:
		return NC_FORMAT_NO_REVERSE;
	case Format::AltCharset:
		return NC_FORMAT_ALT_CHARSET;
	case Format::NoAltCharset:
		return NC_FORMAT_NO_ALT_CHARSET;
	case Format::Italic:
		return NC_FORMAT_ITALIC;
	case Format::NoItalic:
		return NC_FORMAT_NO_ITALIC;
	}
	return NC_FORMAT_BOLD;
}

NcScroll Window::toNcScroll(Scroll scroll)
{
	switch (scroll)
	{
	case Scroll::Up:
		return NC_SCROLL_UP;
	case Scroll::Down:
		return NC_SCROLL_DOWN;
	case Scroll::PageUp:
		return NC_SCROLL_PAGE_UP;
	case Scroll::PageDown:
		return NC_SCROLL_PAGE_DOWN;
	case Scroll::Home:
		return NC_SCROLL_HOME;
	case Scroll::End:
		return NC_SCROLL_END;
	}
	return NC_SCROLL_UP;
}

void Window::syncFromC()
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

NcWindow *Window::cWindow()
{
	return &m_impl;
}

const NcWindow *Window::cWindow() const
{
	return &m_impl;
}

void Window::setColor(Color c)
{
	nc_window_set_color(&m_impl, toNcColor(c));
	syncFromC();
}

void Window::setBaseColor(const Color &color)
{
	nc_window_set_base_color(&m_impl, toNcColor(color));
	syncFromC();
}

void Window::setBorder(Border border)
{
	nc_window_set_border(&m_impl, toNcBorder(border));
	syncFromC();
}

void Window::setTitle(const std::string &new_title)
{
	nc_window_set_title(&m_impl,
	                    const_cast<char *>(new_title.c_str()),
	                    static_cast<int32>(new_title.length()));
	syncFromC();
}

void Window::recreate(size_t width, size_t height)
{
	nc_window_recreate(&m_impl, width, height);
	syncFromC();
}

void Window::moveTo(size_t new_x, size_t new_y)
{
	nc_window_move_to(&m_impl, new_x, new_y);
	syncFromC();
}

void Window::adjustDimensions(size_t width, size_t height)
{
	nc_window_adjust_dimensions(&m_impl, width, height);
	syncFromC();
}

void Window::resize(size_t new_width, size_t new_height)
{
	nc_window_resize(&m_impl, new_width, new_height);
	syncFromC();
}

void Window::refreshBorder() const
{
	nc_window_refresh_border(const_cast<NcWindow *>(&m_impl));
}

void Window::display()
{
	refreshBorder();
	refresh();
	refresh();
}

void Window::refresh()
{
	nc_window_refresh(&m_impl);
}

void Window::clear()
{
	nc_window_clear(&m_impl);
	syncFromC();
}

void Window::setTimeout(int timeout)
{
	nc_window_set_timeout(&m_impl, timeout);
	syncFromC();
}

void Window::addFDCallback(int fd, void (*callback)())
{
	nc_window_add_fd_callback(&m_impl, fd, callback);
}

void Window::clearFDCallbacksList()
{
	nc_window_clear_fd_callbacks(&m_impl);
}

bool Window::FDCallbacksListEmpty() const
{
	return nc_window_fd_callbacks_empty(const_cast<NcWindow *>(&m_impl));
}

Key::Type Window::readKey()
{
	return nc_window_read_key(&m_impl);
}

void Window::pushChar(const Key::Type ch)
{
	nc_window_push_key(&m_impl, ch);
}

std::string Window::prompt(const std::string &base, size_t width, bool encrypted)
{
	std::string result;

	rl::aborted = false;
	rl::w = this;
	getyx(m_window, rl::start_y, rl::start_x);
	rl::width = std::min(m_width-rl::start_x-1, width-1);
	rl::encrypted = encrypted;
	rl::base = base.c_str();

	curs_set(1);
	Mouse::disable();
	nc_window_set_escape_terminal_sequences(&m_impl, false);
	char *input = readline(nullptr);
	nc_window_set_escape_terminal_sequences(&m_impl, true);
	Mouse::enable();
	curs_set(0);
	if (input != nullptr)
	{
#ifdef HAVE_READLINE_HISTORY_H
		if (!encrypted && input[0] != 0)
			add_history(input);
#endif // HAVE_READLINE_HISTORY_H
		result = input;
		free(input);
	}

	if (rl::aborted)
		throw PromptAborted(std::move(result));

	return result;
}

void Window::goToXY(int x, int y)
{
	nc_window_go_to_xy(&m_impl, x, y);
}

int Window::getX()
{
	return nc_window_get_x(&m_impl);
}

int Window::getY()
{
	return nc_window_get_y(&m_impl);
}

bool Window::hasCoords(int &x, int &y)
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

bool Window::runPromptHook(const char *arg, bool *done) const
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

size_t Window::getWidth() const
{
	return nc_window_width(const_cast<NcWindow *>(&m_impl));
}

size_t Window::getHeight() const
{
	return nc_window_height(const_cast<NcWindow *>(&m_impl));
}

size_t Window::getStartX() const
{
	return nc_window_start_x(const_cast<NcWindow *>(&m_impl));
}

size_t Window::getStarty() const
{
	return nc_window_start_y(const_cast<NcWindow *>(&m_impl));
}

const std::string &Window::getTitle() const
{
	return m_title;
}

const Color &Window::getColor() const
{
	return m_color;
}

const Border &Window::getBorder() const
{
	return m_border;
}

int Window::getTimeout() const
{
	return m_window_timeout;
}

const MEVENT &Window::getMouseEvent()
{
	return *nc_window_mouse_event(&m_impl);
}

void Window::scroll(Scroll where)
{
	nc_window_scroll(&m_impl, toNcScroll(where));
}

Window &Window::operator<<(const Color &c)
{
	nc_window_push_color(&m_impl, toNcColor(c));
	syncFromC();
	return *this;
}

Window &Window::operator<<(Format format)
{
	nc_window_apply_format(&m_impl, toNcFormat(format));
	return *this;
}

Window &Window::operator<<(TermManip tm)
{
	switch (tm)
	{
	case TermManip::ClearToEOL:
		nc_window_apply_term_manip(&m_impl, NC_TERM_CLEAR_TO_EOL);
		break;
	}
	return *this;
}

Window &Window::operator<<(const XY &coords)
{
	goToXY(coords.x, coords.y);
	return *this;
}

Window &Window::operator<<(const char *s)
{
	nc_window_print_cstring(&m_impl, const_cast<char *>(s));
	return *this;
}

Window &Window::operator<<(char c)
{
	nc_window_print_char(&m_impl, c);
	return *this;
}

Window &Window::operator<<(int i)
{
	nc_window_print_int32(&m_impl, i);
	return *this;
}

Window &Window::operator<<(double d)
{
	nc_window_print_double(&m_impl, d);
	return *this;
}

Window &Window::operator<<(const std::string &s)
{
	nc_window_print_data(&m_impl,
	                     const_cast<char *>(s.c_str()),
	                     static_cast<int32>(s.length()));
	return *this;
}

Window &Window::operator<<(size_t s)
{
	nc_window_print_uint64(&m_impl, s);
	return *this;
}


}
