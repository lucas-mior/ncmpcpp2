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

#ifndef NCMPCPP_UTILITY_REGEX_H
#define NCMPCPP_UTILITY_REGEX_H

#include "config.h"

#ifdef BOOST_REGEX_ICU
# include <boost/regex/icu.hpp>
# include <unicode/errorcode.h>
# include <unicode/translit.h>
#endif // BOOST_REGEX_ICU

#include <boost/regex.hpp>

#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

#include "utility/functional.h"

namespace {

#ifdef BOOST_REGEX_ICU

struct StripDiacritics
{
	static void convert(icu::UnicodeString &s)
	{
		if (m_converter == nullptr)
		{
			icu::ErrorCode result;
			m_converter = icu::Transliterator::createInstance(
				"NFD; [:M:] Remove; NFC", UTRANS_FORWARD, result);
			if (result.isFailure())
				throw std::runtime_error(
					"instantiation of transliterator instance failed with "
					+ std::string(result.errorName()));
		}
		m_converter->transliterate(s);
	}

private:
	static icu::Transliterator *m_converter;
};

icu::Transliterator *StripDiacritics::m_converter;

#endif // BOOST_REGEX_ICU

}

namespace Regex {

typedef boost::regex::flag_type Flags;
typedef boost::bad_expression Error;

typedef
#ifdef BOOST_REGEX_ICU
	boost::u32regex
#else
	boost::regex
#endif // BOOST_REGEX_ICU
Regex;

inline Flags literalCaseInsensitive()
{
	return boost::regex::icase | boost::regex::literal;
}

inline Flags basicCaseInsensitive()
{
	return boost::regex::icase | boost::regex::basic;
}

inline Flags extendedCaseInsensitive()
{
	return boost::regex::icase | boost::regex::extended;
}

inline Flags perlCaseInsensitive()
{
	return boost::regex::icase | boost::regex::perl;
}

template <typename StringT>
inline Regex make(StringT &&s, Flags flags)
{
	return
#ifdef BOOST_REGEX_ICU
	boost::make_u32regex
#else
	boost::regex
#endif // BOOST_REGEX_ICU
	(std::forward<StringT>(s), flags);
}

template <typename CharT>
inline bool search(const std::basic_string<CharT> &s,
                   const Regex &rx,
                   bool ignore_diacritics)
{
	try {
#ifdef BOOST_REGEX_ICU
		if (ignore_diacritics)
		{
			auto us = icu::UnicodeString::fromUTF8(
				icu::StringPiece(convertString<char, CharT>::apply(s)));
			StripDiacritics::convert(us);
			return boost::u32regex_search(us, rx);
		}
		else
			return boost::u32regex_search(s, rx);
#else
		return boost::regex_search(s, rx);
#endif // BOOST_REGEX_ICU
	} catch (std::out_of_range &e) {
		// Invalid UTF-8 sequence, ignore the string.
		std::cerr << "Regex::search: error while processing \""
		          << s
		          << "\": "
		          << e.what()
		          << "\n";
		return false;
	}
}

template <typename CallbackT>
inline bool forEachMatch(const std::string &s,
                         const std::string &constraint,
                         Flags flags,
                         CallbackT &&callback)
{
	boost::regex rx(constraint, flags);
	auto first = boost::sregex_iterator(s.begin(), s.end(), rx);
	auto last = boost::sregex_iterator();
	bool success = first != last;
	for (; first != last; ++first)
		callback(first->position(), first->length());
	return success;
}

}

#endif // NCMPCPP_UTILITY_REGEX_H
