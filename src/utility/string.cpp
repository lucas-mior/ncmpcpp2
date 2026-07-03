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

#include <cassert>
#include <algorithm>
#include <cstring>
#include "c/ncm_base.h"
#include "c/ncm_path.h"
#include "c/ncm_string.h"
#include "utility/string.h"

namespace {

int32 stringSize(const std::string &s)
{
	return static_cast<int32>(s.size());
}

}

std::string getBasename(const std::string &path)
{
	int32 start = ncm_path_basename_start(
		const_cast<char *>(path.data()), stringSize(path));
	return path.substr(static_cast<size_t>(start));
}

std::string getParentDirectory(std::string path)
{
	int32 len = ncm_path_parent_directory_len(path.data(), stringSize(path));
	path.resize(static_cast<size_t>(len));
	return path;
}

std::string getSharedDirectory(const std::string &dir1, const std::string &dir2)
{
	size_t i = 0;
	size_t min_len = std::min(dir1.length(), dir2.length());
	while (i < min_len && !dir1.compare(i, 1, dir2, i, 1))
		++i;
	i = dir1.rfind("/", i);
	if (i == std::string::npos)
		return "/";
	else
		return dir1.substr(0, i);
}


std::string lowercaseAscii(std::string s)
{
	ncm_string_lowercase_ascii(s.data(), stringSize(s));
	return s;
}

std::string getEnclosedString(const std::string &s, char a, char b, size_t *pos)
{
	std::string result;
	size_t i = s.find(a, pos ? *pos : 0);
	if (i != std::string::npos)
	{
		++i;
		while (i < s.length() && s[i] != b)
		{
			if (s[i] == '\\' && i+1 < s.length() && (s[i+1] == '\\' || s[i+1] == b))
				result += s[++i];
			else
				result += s[i];
			++i;
		}
		// we want to set pos to char after b if possible
		if (i < s.length())
			++i;
		else // we reached end of string and didn't encounter closing char
			result.clear();
	}
	if (pos != 0)
		*pos = i;
	return result;
}

void removeInvalidCharsFromFilename(std::string &filename, bool win32_compatible)
{
	char win32_unallowed_chars[] = "\"*/:<>?\\|";
	char unix_unallowed_chars[] = "/";
	char *unallowed_chars;
	int32 filename_len = stringSize(filename);
	int32 unallowed_chars_len;

	if (win32_compatible)
		unallowed_chars = win32_unallowed_chars;
	else
		unallowed_chars = unix_unallowed_chars;
	unallowed_chars_len = static_cast<int32>(std::strlen(unallowed_chars));

	ncm_string_remove_chars(filename.data(), &filename_len,
	                        unallowed_chars, unallowed_chars_len);
	filename.resize(static_cast<size_t>(filename_len));
}

void escapeSingleQuotes(std::string &filename)
{
	NcmBuffer buffer;

	ncm_buffer_init(&buffer);
	ncm_string_append_shell_escaped_single_quotes(&buffer,
	                                              filename.data(),
	                                              stringSize(filename));
	if (buffer.data != nullptr)
		filename.assign(buffer.data, static_cast<size_t>(buffer.len));
	else
		filename.clear();
	ncm_buffer_destroy(&buffer);
}
