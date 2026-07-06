#if !defined(SRC_CURSES_FORMATTED_COLOR_COMPAT_IMPL_H)
#define SRC_CURSES_FORMATTED_COLOR_COMPAT_IMPL_H

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

#include <istream>

namespace {

void verifyFormats(const NC::FormattedColor::Formats &formats)
{
	for (auto &fmt : formats)
	{
		if (fmt == NC::Format::NoBold
		    || fmt == NC::Format::NoUnderline
		    || fmt == NC::Format::NoReverse
		    || fmt == NC::Format::NoAltCharset
		    || fmt == NC::Format::NoItalic)
			throw std::logic_error("FormattedColor can't hold disabling formats");
	}
}

}

inline NC::FormattedColor::FormattedColor()
{
	nc_formatted_color_init(&m_impl);
}

inline NC::FormattedColor::FormattedColor(Color color_, Formats formats_)
{
	if (color_ == NC::Color::End)
		throw std::logic_error("FormattedColor can't hold Color::End");
	verifyFormats(formats_);
	nc_formatted_color_init_color(&m_impl, toNcColor(color_));
	for (auto &format : formats_)
		nc_formatted_color_add_format(&m_impl, toNcFormat(format));
}

inline NC::FormattedColor::FormattedColor(NcFormattedColor *formatted_color)
{
	nc_formatted_color_copy(&m_impl, formatted_color);
}

inline NC::FormattedColor::FormattedColor(const FormattedColor &rhs)
{
	nc_formatted_color_copy(&m_impl,
	                        const_cast<NcFormattedColor *>(rhs.cFormattedColor()));
}

inline NC::FormattedColor::FormattedColor(FormattedColor &&rhs) noexcept
{
	nc_formatted_color_move(&m_impl, rhs.cFormattedColor());
}

inline NC::FormattedColor &NC::FormattedColor::operator=(const FormattedColor &rhs)
{
	if (this != &rhs)
	{
		nc_formatted_color_destroy(&m_impl);
		nc_formatted_color_copy(
			&m_impl, const_cast<NcFormattedColor *>(rhs.cFormattedColor()));
	}
	return *this;
}

inline NC::FormattedColor &NC::FormattedColor::operator=(FormattedColor &&rhs) noexcept
{
	if (this != &rhs)
	{
		nc_formatted_color_destroy(&m_impl);
		nc_formatted_color_move(&m_impl, rhs.cFormattedColor());
	}
	return *this;
}

inline NC::FormattedColor::~FormattedColor()
{
	nc_formatted_color_destroy(&m_impl);
}

inline const NC::Color &NC::FormattedColor::color() const
{
	m_color_cache = fromNcColor(m_impl.color);
	return m_color_cache;
}

inline NC::FormattedColor::Formats NC::FormattedColor::formats() const
{
	Formats result;
	auto formats = nc_formatted_color_formats(
		const_cast<NcFormattedColor *>(&m_impl));
	auto count = nc_formatted_color_format_count(
		const_cast<NcFormattedColor *>(&m_impl));
	for (int32 i = 0; i < count; i += 1)
		result.push_back(fromNcFormat(formats[i]));
	return result;
}

inline NcFormattedColor *NC::FormattedColor::cFormattedColor()
{
	return &m_impl;
}

const inline NcFormattedColor *NC::FormattedColor::cFormattedColor() const
{
	return &m_impl;
}

inline NcColor NC::toNcColor(Color color)
{
	if (color.isDefault())
		return nc_color_default();
	if (color.isEnd())
		return nc_color_end();
	return nc_color_make(color.foreground(), color.background(), false, false);
}

inline NC::Color NC::fromNcColor(NcColor color)
{
	if (nc_color_is_default(color))
		return Color::Default;
	if (nc_color_is_end(color))
		return Color::End;
	return Color(color.foreground, color.background, false, false);
}

inline NcFormat NC::toNcFormat(Format format)
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

inline NC::Format NC::fromNcFormat(NcFormat format)
{
	switch (format)
	{
	case NC_FORMAT_BOLD:
		return Format::Bold;
	case NC_FORMAT_NO_BOLD:
		return Format::NoBold;
	case NC_FORMAT_UNDERLINE:
		return Format::Underline;
	case NC_FORMAT_NO_UNDERLINE:
		return Format::NoUnderline;
	case NC_FORMAT_REVERSE:
		return Format::Reverse;
	case NC_FORMAT_NO_REVERSE:
		return Format::NoReverse;
	case NC_FORMAT_ALT_CHARSET:
		return Format::AltCharset;
	case NC_FORMAT_NO_ALT_CHARSET:
		return Format::NoAltCharset;
	case NC_FORMAT_ITALIC:
		return Format::Italic;
	case NC_FORMAT_NO_ITALIC:
		return Format::NoItalic;
	}
	return Format::Bold;
}

inline std::istream &NC::operator>>(std::istream &is, NC::FormattedColor &fc)
{
	NC::Color c;
	is >> c;
	if (!is.eof() && is.peek() == ':')
	{
		is.get();
		NC::FormattedColor::Formats formats;
		while (!is.eof() && isalpha(is.peek()))
		{
			char flag = is.get();
			switch (flag)
			{
			case 'b':
				formats.push_back(NC::Format::Bold);
				break;
			case 'u':
				formats.push_back(NC::Format::Underline);
				break;
			case 'r':
				formats.push_back(NC::Format::Reverse);
				break;
			case 'a':
				formats.push_back(NC::Format::AltCharset);
				break;
			case 'i':
				formats.push_back(NC::Format::Italic);
				break;
			default:
				is.setstate(std::ios::failbit);
				break;
			}
		}
		fc = NC::FormattedColor(c, std::move(formats));
	}
	else
		fc = NC::FormattedColor(c, {});
	return is;
}

#endif /* SRC_CURSES_FORMATTED_COLOR_COMPAT_IMPL_H */
