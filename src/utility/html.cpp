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
#include "c/ncm_utf8.h"
#include "utility/html.h"

namespace {

void replaceAll(std::string &s, const std::string &from, const std::string &to)
{
	for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
		s.replace(pos, from.size(), to);
}

}

std::string unescapeHtmlUtf8(const std::string &data)
{
	int base;
	size_t offset;
	std::string result;
	for (size_t i = 0, j; i < data.length(); ++i)
	{
		if (data[i] == '&' && data[i+1] == '#' && (j = data.find(';', i)) != std::string::npos)
		{
			if (data[i+2] == 'x')
			{
				offset = 3;
				base = 16;
			}
			else
			{
				offset = 2;
				base = 10;
			}
			int n = strtol(&data.c_str()[i+offset], nullptr, base);
			char buffer[4];
			int32 len = ncm_utf8_encode(static_cast<uint32>(n), buffer, sizeof(buffer));
			if (len > 0)
				result.append(buffer, static_cast<size_t>(len));
			i = j;
		}
		else
			result += data[i];
	}
	return result;
}

void unescapeHtmlEntities(std::string &s)
{
	// well, at least some of them.
	replaceAll(s, "&apos;", "'");
	replaceAll(s, "&amp;", "&");
	replaceAll(s, "&gt;", ">");
	replaceAll(s, "&lt;", "<");
	replaceAll(s, "&nbsp;", " ");
	replaceAll(s, "&quot;", "\"");
	replaceAll(s, "&ndash;", "–");
	replaceAll(s, "&mdash;", "—");
}

void stripHtmlTags(std::string &s)
{
	// Erase newlines so they don't duplicate with HTML ones.
	s.erase(std::remove_if(s.begin(), s.end(), [](char c) {
				return c == '\n' || c == '\r';
			}), s.end());

	bool is_newline;
	for (size_t i = s.find("<"); i != std::string::npos; i = s.find("<"))
	{
		size_t j = s.find(">", i);
		if (j != std::string::npos)
		{
			++j;
			is_newline
				=  s.compare(i, std::min<size_t>(3, j-i), "<p ") == 0
				|| s.compare(i, j-i, "<p>") == 0
				|| s.compare(i, j-i, "</p>") == 0
				|| s.compare(i, j-i, "<br>") == 0
				|| s.compare(i, j-i, "<br/>") == 0
				|| s.compare(i, std::min<size_t>(4, j-i), "<br ") == 0;
			if (is_newline)
				s.replace(i, j-i, "\n");
			else
				s.replace(i, j-i, "");
		}
		else
			break;
	}
	unescapeHtmlEntities(s);
}
