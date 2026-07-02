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

#include "c/ncm_html.h"
#include "utility/html.h"

namespace {

int32 stringSize(const std::string &s)
{
	return static_cast<int32>(s.size());
}

std::string toStringAndDestroy(NcmBuffer &buffer)
{
	std::string result;
	if (buffer.len > 0)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

}

std::string unescapeHtmlUtf8(const std::string &data)
{
	NcmBuffer result = ncm_html_unescape_utf8(
		const_cast<char *>(data.data()), stringSize(data));
	return toStringAndDestroy(result);
}

void unescapeHtmlEntities(std::string &s)
{
	NcmBuffer result = ncm_html_unescape_entities(s.data(), stringSize(s));
	s = toStringAndDestroy(result);
}

void stripHtmlTags(std::string &s)
{
	NcmBuffer result = ncm_html_strip_tags(s.data(), stringSize(s));
	s = toStringAndDestroy(result);
}
