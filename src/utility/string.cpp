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

#include <limits>
#include "c/ncm_base.h"
#include "c/ncm_string.h"
#include "utility/string.h"

namespace {

int32 stringSize(const std::string &s)
{
	return static_cast<int32>(s.size());
}

std::string takeBuffer(NcmBuffer &buffer)
{
	std::string result;
	if (buffer.data != nullptr)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

int32 startPosition(const std::string &s, size_t *pos)
{
	if (pos == nullptr)
		return 0;
	if (*pos > static_cast<size_t>(std::numeric_limits<int32>::max()))
		return stringSize(s);
	return static_cast<int32>(*pos);
}

}

std::string getBasename(const std::string &path)
{
	int32 start = ncm_string_basename_start(
		const_cast<char *>(path.data()), stringSize(path));
	return path.substr(static_cast<size_t>(start));
}

std::string getParentDirectory(std::string path)
{
	int32 len = ncm_string_parent_directory_len(path.data(), stringSize(path));
	path.resize(static_cast<size_t>(len));
	return path;
}

std::string getSharedDirectory(const std::string &dir1, const std::string &dir2)
{
	NcmBuffer buffer = ncm_string_shared_directory(
		const_cast<char *>(dir1.data()), stringSize(dir1),
		const_cast<char *>(dir2.data()), stringSize(dir2));
	return takeBuffer(buffer);
}

std::string lowercaseAscii(std::string s)
{
	ncm_string_lowercase_ascii(s.data(), stringSize(s));
	return s;
}

std::string getEnclosedString(const std::string &s, char a, char b, size_t *pos)
{
	int32 c_pos;
	NcmBuffer buffer = ncm_string_get_enclosed(
		const_cast<char *>(s.data()), stringSize(s),
		a, b, startPosition(s, pos), &c_pos);
	if (pos != nullptr)
	{
		if (c_pos < 0)
			*pos = std::string::npos;
		else
			*pos = static_cast<size_t>(c_pos);
	}
	return takeBuffer(buffer);
}

void removeInvalidCharsFromFilename(std::string &filename, bool win32_compatible)
{
	int32 filename_len = stringSize(filename);
	ncm_string_remove_invalid_filename_chars(filename.data(), &filename_len,
	                                             win32_compatible);
	filename.resize(static_cast<size_t>(filename_len));
}

void escapeSingleQuotes(std::string &filename)
{
	NcmBuffer buffer;

	ncm_buffer_init(&buffer);
	ncm_string_append_shell_escaped_single_quotes(&buffer,
	                                              filename.data(),
	                                              stringSize(filename));
	filename = takeBuffer(buffer);
}
