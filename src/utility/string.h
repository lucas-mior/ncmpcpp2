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

#ifndef NCMPCPP_UTILITY_STRING_H
#define NCMPCPP_UTILITY_STRING_H

#include <cstdint>
#include <limits>
#include <locale>
#include <string>
#include <vector>

#include "c/ncm_base.h"
#include "c/ncm_string.h"

template <size_t N> size_t const_strlen(const char (&)[N]) {
	return N-1;
}

// Similar helpers usually support std::string, but we want a more general version.
template <typename StringT, typename CollectionT>
StringT join(const CollectionT &collection, const StringT &separator)
{
	StringT result;
	auto first = std::begin(collection), last = std::end(collection);
	if (first != last)
	{
		while (true)
		{
			result += *first;
			++first;
			if (first != last)
				result += separator;
			else
				break;
		}
	}
	return result;
}

namespace ncmpcpp_utility_detail {

inline int32 string_size(const std::string &s)
{
	return static_cast<int32>(s.size());
}

inline char *string_data(const std::string &s)
{
	return const_cast<char *>(s.data());
}

inline std::string take_buffer(NcmBuffer &buffer)
{
	std::string result;
	if (buffer.data != nullptr)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

inline int32 start_position(const std::string &s, size_t *pos)
{
	if (pos == nullptr)
		return 0;
	if (*pos > static_cast<size_t>(std::numeric_limits<int32>::max()))
		return string_size(s);
	return static_cast<int32>(*pos);
}

}

inline std::string getBasename(const std::string &path)
{
	int32 start = ncm_string_basename_start(
		ncmpcpp_utility_detail::string_data(path),
		ncmpcpp_utility_detail::string_size(path));
	return path.substr(static_cast<size_t>(start));
}

inline std::string getParentDirectory(std::string path)
{
	int32 len = ncm_string_parent_directory_len(
		path.data(), ncmpcpp_utility_detail::string_size(path));
	path.resize(static_cast<size_t>(len));
	return path;
}

inline std::string getSharedDirectory(const std::string &dir1,
                                      const std::string &dir2)
{
	NcmBuffer buffer = ncm_string_shared_directory(
		ncmpcpp_utility_detail::string_data(dir1),
		ncmpcpp_utility_detail::string_size(dir1),
		ncmpcpp_utility_detail::string_data(dir2),
		ncmpcpp_utility_detail::string_size(dir2));
	return ncmpcpp_utility_detail::take_buffer(buffer);
}

inline std::string lowercaseAscii(std::string s)
{
	ncm_string_lowercase_ascii(
		s.data(), ncmpcpp_utility_detail::string_size(s));
	return s;
}

inline std::string getEnclosedString(const std::string &s,
                                     char a, char b, size_t *pos)
{
	int32 c_pos;
	NcmBuffer buffer = ncm_string_get_enclosed(
		ncmpcpp_utility_detail::string_data(s),
		ncmpcpp_utility_detail::string_size(s),
		a, b, ncmpcpp_utility_detail::start_position(s, pos), &c_pos);
	if (pos != nullptr)
	{
		if (c_pos < 0)
			*pos = std::string::npos;
		else
			*pos = static_cast<size_t>(c_pos);
	}
	return ncmpcpp_utility_detail::take_buffer(buffer);
}

inline void removeInvalidCharsFromFilename(std::string &filename,
                                           bool win32_compatible)
{
	int32 filename_len = ncmpcpp_utility_detail::string_size(filename);
	ncm_string_remove_invalid_filename_chars(
		filename.data(), &filename_len, win32_compatible);
	filename.resize(static_cast<size_t>(filename_len));
}

inline void escapeSingleQuotes(std::string &filename)
{
	NcmBuffer buffer;

	ncm_buffer_init(&buffer);
	ncm_string_append_shell_escaped_single_quotes(
		&buffer, filename.data(), ncmpcpp_utility_detail::string_size(filename));
	filename = ncmpcpp_utility_detail::take_buffer(buffer);
}

#endif // NCMPCPP_UTILITY_STRING_H
