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

#ifndef NCMPCPP_UTILITY_HTML_H
#define NCMPCPP_UTILITY_HTML_H

#include <string>

#include "c/ncm_html.h"

namespace ncmpcpp_html_detail {

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
	if (buffer.len > 0)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

}

inline std::string unescapeHtmlUtf8(const std::string &s)
{
	NcmBuffer result = ncm_html_unescape_utf8(
		ncmpcpp_html_detail::string_data(s),
		ncmpcpp_html_detail::string_size(s));
	return ncmpcpp_html_detail::take_buffer(result);
}

inline void unescapeHtmlEntities(std::string &s)
{
	NcmBuffer result = ncm_html_unescape_entities(
		s.data(), ncmpcpp_html_detail::string_size(s));
	s = ncmpcpp_html_detail::take_buffer(result);
}

inline void stripHtmlTags(std::string &s)
{
	NcmBuffer result = ncm_html_strip_tags(
		s.data(), ncmpcpp_html_detail::string_size(s));
	s = ncmpcpp_html_detail::take_buffer(result);
}

#endif // NCMPCPP_UTILITY_HTML_H
