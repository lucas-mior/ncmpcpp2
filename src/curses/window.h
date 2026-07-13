#ifndef NCMPCPP_WINDOW_H
#define NCMPCPP_WINDOW_H

#define NCURSES_NOMACROS 1

#include "config.h"

#include "curses/nc_window.h"

#include <optional>
#include <functional>
#include <string>
#include <tuple>
#include <cstdint>
#include <stdexcept>
#include <utility>

#if NCURSES_MOUSE_VERSION == 1
# define BUTTON5_PRESSED (1U << 27)
#endif // NCURSES_MOUSE_VERSION == 1

/// NC namespace provides set of easy-to-use
/// wrappers over original curses library.
namespace NC {

/// Thrown if Ctrl-C or Ctrl-G is pressed during the call to Window::getString()
/// @see Window::getString()
struct PromptAborted : std::exception
{
	PromptAborted() { }

	template <typename ArgT>
	PromptAborted(ArgT &&prompt)
		: m_prompt(std::forward<ArgT>(prompt)) { }

	virtual const char *what() const noexcept override { return m_prompt.c_str(); }

private:
	std::string m_prompt;
};

struct Color
{
	friend struct Window;
	friend NcColor toNcColor(Color color);

	static const short transparent;
	static const short current;

	Color() : m_impl(0, 0, true, false) { }
	Color(short foreground_value, short background_value,
			 bool is_default = false, bool is_end = false)
	: m_impl(foreground_value, background_value, is_default, is_end)
	{
		if (isDefault() && isEnd())
			throw std::logic_error("Color flag can't be marked as both 'default' and 'end'");
	}

	bool operator==(const Color &rhs) const { return m_impl == rhs.m_impl; }
	bool operator!=(const Color &rhs) const { return m_impl != rhs.m_impl; }
	bool operator<(const Color &rhs) const { return m_impl < rhs.m_impl; }

	bool isDefault() const { return std::get<2>(m_impl); }
	bool isEnd() const { return std::get<3>(m_impl); }

	int pairNumber() const;

	static Color Default;
	static Color Black;
	static Color Red;
	static Color Green;
	static Color Yellow;
	static Color Blue;
	static Color Magenta;
	static Color Cyan;
	static Color White;
	static Color End;

private:
	short foreground() const { return std::get<0>(m_impl); }
	short background() const { return std::get<1>(m_impl); }
	bool currentBackground() const { return background() == current; }

	std::tuple<short, short, bool, bool> m_impl;
};

typedef std::optional<Color> Border;

/// Terminal manipulation functions
enum class TermManip { ClearToEOL };

enum NcFormat reverseFormat(enum NcFormat fmt);

/// Initializes readline callbacks used by legacy prompts.
void initReadline();

/// Pauses the screen (e.g. for running an external command)
void pauseScreen();

/// Unpauses the screen
void unpauseScreen();

/// Struct used for going to given coordinates
/// @see Window::operator<<()
struct XY
{
	XY(int xx, int yy) : x(xx), y(yy) { }
	int x;
	int y;
};

/// Main class of NCurses namespace, used as base for other specialized windows
struct Window
{
	/// Helper function that is periodically invoked
	// inside Window::getString() function
	/// @see Window::getString()
	typedef std::function<bool(const char *)> PromptHook;

	struct ScopedTimeout
	{
		ScopedTimeout(Window &w, int new_timeout)
			: m_w(w)
		{
			m_timeout = w.getTimeout();
			w.setTimeout(new_timeout);
		}

		~ScopedTimeout()
		{
			m_w.setTimeout(m_timeout);
		}

	private:
		Window &m_w;
		int m_timeout;
	};

	Window();
	
	/// Constructs an empty window with given parameters
	/// @param startx X position of left upper corner of constructed window
	/// @param starty Y position of left upper corner of constructed window
	/// @param width width of constructed window
	/// @param height height of constructed window
	/// @param title title of constructed window
	/// @param color base color of constructed window
	/// @param border border of constructed window
	Window(size_t startx, size_t starty, size_t width, size_t height,
	        std::string title, Color color, Border border);
	
	Window(const Window &rhs);
	Window(Window &&rhs);
	Window &operator=(Window w);
	
	virtual ~Window();
	
	/// Allows for direct access to internal WINDOW pointer in case there
	/// is no wrapper for a function from curses library one may want to use
	/// @return internal WINDOW pointer
	WINDOW *raw() const { return m_window; }
	
	/// @return window's width
	size_t getWidth() const;
	
	/// @return current window's timeout
	int getTimeout() const;
	
	/// Reads the string from standard input using readline library.
	/// @param base base string that has to be edited
	/// @param length max length of the string, unlimited by default
	/// @param width width of area that entry field can take. if it's reached, string
	/// will be scrolled. if value is 0, field will be from cursor position to the end
	/// of current line wide.
	/// @param encrypted if set to true, '*' characters will be displayed instead of
	/// actual text.
	/// @return edited string
	///
	/// @see setPromptHook()
	/// @see SetTimeout()
	std::string prompt(const std::string &base = "", size_t width = -1, bool encrypted = false);
	
	/// Moves cursor to given coordinates
	/// @param x given X position
	/// @param y given Y position
	void goToXY(int x, int y);
	
	/// @return x window coordinate
	int getX();
	
	/// @return y windows coordinate
	int getY();
	
	/// Used to indicate whether given coordinates of main screen lies within
	/// window area or not and if they do, transform them into in-window coords.
	/// Otherwise function doesn't modify its arguments.
	/// @param x X position of main screen to be checked
	/// @param y Y position of main screen to be checked
	/// @return true if it transformed variables, false otherwise
	bool hasCoords(int &x, int &y);
	
	/// Sets hook used in getString()
	/// @param hook pointer to function that matches getStringHelper prototype
	/// @see getString()
	template <typename HookT>
	void setPromptHook(HookT &&hook) {
		m_prompt_hook = std::forward<HookT>(hook);
	}

	/// Run current GetString helper function (if defined).
	/// @see getString()
	/// @return true if helper was run, false otherwise
	bool runPromptHook(const char *arg, bool *done) const;

	/// Sets window's border
	/// @param border new window's border
	void setBorder(Border border);
	
	/// Sets window's timeout
	/// @param timeout window's timeout
	void setTimeout(int timeout);
	
	/// Sets window's title
	/// @param new_title new title for window
	void setTitle(const std::string &new_title);
	
	/// Refreshed whole window and its border
	/// @see refresh()
	void display();

	/// Refreshes window's border
	void refreshBorder() const;

	/// Refreshes whole window, but not the border
	/// @see display()
	virtual void refresh();

	/// Moves the window to new coordinates
	/// @param new_x new X position of left upper corner of window
	/// @param new_y new Y position of left upper corner of window
	virtual void moveTo(size_t new_x, size_t new_y);
	
	/// Resizes the window
	/// @param new_width new window's width
	/// @param new_height new window's height
	virtual void resize(size_t new_width, size_t new_height);
	
	/// Cleares the window
	virtual void clear();
	
	/// Clears list of file descriptors and their callbacks
	void clearFDCallbacksList();
	
	/// Push single character into input queue, so it can get consumed by ReadKey
	void pushChar(NcKey ch);
	
	/// Scrolls the window by amount of lines given in its parameter
	/// @param where indicates how many lines it has to scroll
	virtual void scroll(enum NcScroll where);
	
	Window &operator<<(TermManip tm);
	Window &operator<<(const Color &color);
	Window &operator<<(enum NcFormat format);
	Window &operator<<(const XY &coords);
	Window &operator<<(const char *s);
	Window &operator<<(char c);
	Window &operator<<(int i);
	Window &operator<<(double d);
	Window &operator<<(size_t s);
	Window &operator<<(const std::string &s);

	NcWindow *nativeWindow();
	const NcWindow *nativeWindow() const;
protected:
	NcWindow *cWindow();
	const NcWindow *cWindow() const;
	void syncFromC();
	
	/// internal WINDOW pointers
	WINDOW *m_window;
	
	/// start points and dimensions
	size_t m_start_x;
	size_t m_start_y;
	size_t m_width;
	size_t m_height;
	
	/// window timeout
	int m_window_timeout;
	
	/// current colors
	Color m_color;
	
	/// base colors
	Color m_base_color;
	
	/// current border
	Border m_border;
	
private:
	static NcColor toNcColor(Color color);
	static Color fromNcColor(NcColor color);
	static NcBorder toNcBorder(Border border);
	static Border fromNcBorder(NcBorder border);
	static NcFormat toNcFormat(enum NcFormat format);
	static NcScroll toNcScroll(enum NcScroll scroll);

	NcWindow m_impl;
	PromptHook m_prompt_hook;
	std::string m_title;
};

}

#include "curses/window_compat_impl.h"

#endif // NCMPCPP_WINDOW_H
