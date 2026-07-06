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

#ifndef NCMPCPP_CHARSET_H
#define NCMPCPP_CHARSET_H

#include <cstring>
#include <locale>
#include <stdexcept>
#include <string>
#include <utility>

#include "c/ncm_charset.h"

namespace Charset {

namespace Detail {

inline int32 stringSize(const std::string &s)
{
	return static_cast<int32>(s.size());
}

inline char *stringData(const std::string &s)
{
	return const_cast<char *>(s.data());
}

inline std::string takeBuffer(NcmBuffer &buffer)
{
	std::string result;
	if (buffer.len > 0)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

}

inline std::locale internalLocale()
{
	try
	{
		return std::locale("");
	}
	catch (const std::runtime_error &)
	{
		return std::locale::classic();
	}
}

inline std::string toUtf8From(const std::string &s, const char *charset)
{
	NcmBuffer result = ncm_charset_to_utf8_from(
		Detail::stringData(s), Detail::stringSize(s),
		const_cast<char *>(charset), charset == nullptr ? 0 : strlen(charset));
	return Detail::takeBuffer(result);
}

inline std::string fromUtf8To(const std::string &s, const char *charset)
{
	NcmBuffer result = ncm_charset_from_utf8_to(
		Detail::stringData(s), Detail::stringSize(s),
		const_cast<char *>(charset), charset == nullptr ? 0 : strlen(charset));
	return Detail::takeBuffer(result);
}

inline std::string utf8ToLocale(const std::string &s)
{
	NcmBuffer result = ncm_charset_utf8_to_locale(
		Detail::stringData(s), Detail::stringSize(s));
	return Detail::takeBuffer(result);
}

inline std::string utf8ToLocale(std::string &&s)
{
	return std::move(s);
}

inline std::string localeToUtf8(const std::string &s)
{
	NcmBuffer result = ncm_charset_locale_to_utf8(
		Detail::stringData(s), Detail::stringSize(s));
	return Detail::takeBuffer(result);
}

inline std::string localeToUtf8(std::string &&s)
{
	return std::move(s);
}

}

#endif // NCMPCPP_CHARSET_H
