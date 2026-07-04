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

#include "c/ncm_utf8.h"

namespace {

int32 stringSize(const std::string &s)
{
	return static_cast<int32>(s.size());
}

char *stringData(const std::string &s)
{
	return const_cast<char *>(s.data());
}

}

namespace Utf8 {

size_t characters(const std::string &s)
{
	return static_cast<size_t>(ncm_utf8_characters(stringData(s), stringSize(s)));
}

size_t bytePosition(const std::string &s, size_t character)
{
	return static_cast<size_t>(ncm_utf8_byte_position(
		stringData(s), stringSize(s), static_cast<int32>(character)));
}

size_t nextPosition(const std::string &s, size_t byte)
{
	return static_cast<size_t>(ncm_utf8_next_position(
		stringData(s), stringSize(s), static_cast<int32>(byte)));
}

size_t width(const std::string &s)
{
	return static_cast<size_t>(ncm_utf8_width(stringData(s), stringSize(s)));
}

void cut(std::string &s, size_t max_width)
{
	int32 new_size = ncm_utf8_cut_width(
		s.data(), stringSize(s), static_cast<int32>(max_width));
	s.resize(static_cast<size_t>(new_size));
}

std::string shorten(const std::string &s, size_t max_width)
{
	if (width(s) <= max_width)
		return s;
	if (max_width <= 2)
		return std::string(max_width, '.');

	const int32 half_max = static_cast<int32>(max_width/2 - 1);
	const int32 prefix_size = ncm_utf8_cut_width(
		stringData(s), stringSize(s), half_max);
	const int32 suffix_position = ncm_utf8_suffix_width_position(
		stringData(s), stringSize(s), half_max);

	std::string result = s.substr(0, static_cast<size_t>(prefix_size));
	result += "..";
	result += s.substr(static_cast<size_t>(suffix_position));
	return result;
}

std::vector<std::string> split(const std::string &s)
{
	std::vector<std::string> result;
	for (size_t i = 0; i < s.size();)
	{
		size_t next = nextPosition(s, i);
		result.push_back(s.substr(i, next-i));
		i = next;
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
	const size_t current_count = characters(s);
	if (current_count > count)
	{
		s.resize(bytePosition(s, count));
	}
	else
	{
		s.append(count-current_count, fill);
	}
}

std::string capitalizeFirstLetters(const std::string &s)
{
	int32 result_size = ncm_utf8_capitalize_first_letters(
		stringData(s), stringSize(s), nullptr, 0);
	std::string result(static_cast<size_t>(result_size), '\0');
	result_size = ncm_utf8_capitalize_first_letters(
		stringData(s), stringSize(s), result.data(), stringSize(result));
	result.resize(static_cast<size_t>(result_size));
	return result;
}

}
