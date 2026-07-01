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

#include "utility/utf8.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <cwchar>
#include <cwctype>

namespace {

size_t nextChar(const std::string &s, size_t pos, wchar_t &wc)
{
	std::mbstate_t state = std::mbstate_t();
	const char *start = s.data()+pos;
	size_t length = std::mbrtowc(&wc, start, s.size()-pos, &state);
	if (length == 0)
	{
		wc = L'\0';
		return 1;
	}
	if (length == static_cast<size_t>(-1) || length == static_cast<size_t>(-2))
	{
		wc = L'.';
		return 1;
	}
	return length;
}

int charWidth(wchar_t wc)
{
	int result = wcwidth(wc);
	return result < 0 ? 1 : result;
}

std::string encode(wchar_t wc, const std::string &fallback)
{
	std::mbstate_t state = std::mbstate_t();
	std::array<char, MB_LEN_MAX> buf;
	size_t length = std::wcrtomb(buf.data(), wc, &state);
	if (length == static_cast<size_t>(-1))
		return fallback;
	return std::string(buf.data(), length);
}

}

namespace Utf8 {

size_t characters(const std::string &s)
{
	size_t result = 0;
	for (size_t i = 0; i < s.size(); ++result)
	{
		wchar_t wc;
		i += nextChar(s, i, wc);
	}
	return result;
}


size_t bytePosition(const std::string &s, size_t character)
{
	size_t byte = 0;
	for (size_t i = 0; i < character && byte < s.size(); ++i)
		byte = nextPosition(s, byte);
	return byte;
}

size_t nextPosition(const std::string &s, size_t byte)
{
	if (byte >= s.size())
		return s.size();
	wchar_t wc;
	return std::min(s.size(), byte+nextChar(s, byte, wc));
}

size_t width(const std::string &s)
{
	size_t result = 0;
	for (size_t i = 0; i < s.size();)
	{
		wchar_t wc;
		i += nextChar(s, i, wc);
		result += charWidth(wc);
	}
	return result;
}

void cut(std::string &s, size_t max_width)
{
	size_t i = 0;
	size_t byte_pos = 0;
	size_t result_width = 0;
	for (; byte_pos < s.size(); ++i)
	{
		wchar_t wc;
		size_t length = nextChar(s, byte_pos, wc);
		size_t next_width = result_width + charWidth(wc);
		if (next_width > max_width)
		{
			s.resize(byte_pos);
			return;
		}
		result_width = next_width;
		byte_pos += length;
	}
}

std::string shorten(const std::string &s, size_t max_width)
{
	if (width(s) <= max_width)
		return s;
	if (max_width <= 2)
		return std::string(max_width, '.');

	const size_t half_max = max_width/2 - 1;
	std::string result;
	size_t result_width = 0;
	for (size_t i = 0; i < s.size();)
	{
		wchar_t wc;
		size_t length = nextChar(s, i, wc);
		size_t next_width = result_width + charWidth(wc);
		if (next_width > half_max)
			break;
		result += s.substr(i, length);
		result_width = next_width;
		i += length;
	}

	auto chars = split(s);
	std::string end;
	result_width = 0;
	for (auto it = chars.rbegin(); it != chars.rend(); ++it)
	{
		size_t next_width = result_width + width(*it);
		if (next_width > half_max)
			break;
		end.insert(0, *it);
		result_width = next_width;
	}

	result += "..";
	result += end;
	return result;
}

std::vector<std::string> split(const std::string &s)
{
	std::vector<std::string> result;
	for (size_t i = 0; i < s.size();)
	{
		wchar_t wc;
		size_t length = nextChar(s, i, wc);
		result.push_back(s.substr(i, length));
		i += length;
	}
	return result;
}

std::string repeat(const std::string &s, size_t count)
{
	std::string result;
	for (size_t i = 0; i < count; ++i)
		result += s;
	return result;
}

void resize(std::string &s, size_t count, char fill)
{
	auto chars = split(s);
	if (chars.size() > count)
	{
		s.clear();
		for (size_t i = 0; i < count; ++i)
			s += chars[i];
	}
	else
	{
		while (chars.size() < count)
		{
			s += fill;
			chars.push_back(std::string(1, fill));
		}
	}
}

std::string capitalizeFirstLetters(const std::string &s)
{
	std::string result;
	wchar_t prev = 0;
	for (size_t i = 0; i < s.size();)
	{
		wchar_t wc;
		size_t length = nextChar(s, i, wc);
		std::string fallback = s.substr(i, length);
		if (!std::iswalpha(prev) && prev != L'\'')
			wc = std::towupper(wc);
		result += encode(wc, fallback);
		prev = wc;
		i += length;
	}
	return result;
}

}
