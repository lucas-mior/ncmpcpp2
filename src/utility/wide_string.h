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

#ifndef NCMPCPP_UTILITY_WIDE_STRING_H
#define NCMPCPP_UTILITY_WIDE_STRING_H

#include "config.h"

#include <cstdint>
#include <limits>
#include <string>

#ifdef HAVE_TAGLIB_H
# include <tstring.h>
#endif // HAVE_TAGLIB_H

namespace Utf8Compatibility {

class String
{
public:
	String(const std::string &value)
	: m_value(value) { }

	operator std::wstring() const
	{
		std::wstring result;
		for (size_t i = 0; i < m_value.size();)
		{
			uint32_t codepoint;
			size_t length = decode(m_value, i, codepoint);
			if (length == 0)
				break;
			if (codepoint > static_cast<uint32_t>(std::numeric_limits<wchar_t>::max()))
				codepoint = ReplacementCharacter;
			result += static_cast<wchar_t>(codepoint);
			i += length;
		}
		return result;
	}

#ifdef HAVE_TAGLIB_H
	operator TagLib::String() const
	{
		return TagLib::String(m_value, TagLib::String::UTF8);
	}
#endif // HAVE_TAGLIB_H

private:
	static constexpr uint32_t ReplacementCharacter = 0xfffd;

	static bool isContinuation(unsigned char c)
	{
		return (c & 0xc0) == 0x80;
	}

	static size_t decode(const std::string &s, size_t pos, uint32_t &codepoint)
	{
		const auto first = static_cast<unsigned char>(s[pos]);
		if (first < 0x80)
		{
			codepoint = first;
			return 1;
		}

		size_t length;
		uint32_t value;
		if ((first & 0xe0) == 0xc0)
		{
			length = 2;
			value = first & 0x1f;
		}
		else if ((first & 0xf0) == 0xe0)
		{
			length = 3;
			value = first & 0x0f;
		}
		else if ((first & 0xf8) == 0xf0)
		{
			length = 4;
			value = first & 0x07;
		}
		else
		{
			codepoint = ReplacementCharacter;
			return 1;
		}

		if (pos+length > s.size())
		{
			codepoint = ReplacementCharacter;
			return 1;
		}

		for (size_t i = 1; i < length; ++i)
		{
			const auto c = static_cast<unsigned char>(s[pos+i]);
			if (!isContinuation(c))
			{
				codepoint = ReplacementCharacter;
				return 1;
			}
			value = (value << 6) | (c & 0x3f);
		}

		if (!isValid(value, length))
			codepoint = ReplacementCharacter;
		else
			codepoint = value;
		return length;
	}

	static bool isValid(uint32_t codepoint, size_t length)
	{
		if (codepoint > 0x10ffff)
			return false;
		if (codepoint >= 0xd800 && codepoint <= 0xdfff)
			return false;
		if (length == 2 && codepoint < 0x80)
			return false;
		if (length == 3 && codepoint < 0x800)
			return false;
		if (length == 4 && codepoint < 0x10000)
			return false;
		return true;
	}

	std::string m_value;
};

}

inline Utf8Compatibility::String ToWString(const std::string &s)
{
	return Utf8Compatibility::String(s);
}

#endif // NCMPCPP_UTILITY_WIDE_STRING_H
