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

#ifndef NCMPCPP_FORMATTED_COLOR_H
#define NCMPCPP_FORMATTED_COLOR_H

#include <vector>

#include "curses/nc_formatted_color.h"
#include "curses/window.h"
#include "utility/storage_kind.h"

namespace NC {

struct FormattedColor
{
	template <StorageKind storage = StorageKind::Reference>
	struct End
	{
		explicit End(const FormattedColor &fc)
			: m_fc(fc)
		{ }

		const FormattedColor &base() const { return m_fc; }

		template <StorageKind otherStorage>
		bool operator==(const End<otherStorage> &rhs) const
		{
			return m_fc == rhs.m_fc;
		}

		explicit operator End<StorageKind::Value>() const
		{
			return End<StorageKind::Value>(m_fc);
		}

	private:
		typename std::conditional<storage == StorageKind::Reference,
		                          const FormattedColor &,
		                          FormattedColor>::type m_fc;
	};

	typedef std::vector<Format> Formats;

	FormattedColor();
	FormattedColor(Color color_, Formats formats_);
	explicit FormattedColor(NcFormattedColor *formatted_color);
	FormattedColor(const FormattedColor &rhs);
	FormattedColor(FormattedColor &&rhs) noexcept;
	FormattedColor &operator=(const FormattedColor &rhs);
	FormattedColor &operator=(FormattedColor &&rhs) noexcept;
	~FormattedColor();

	const Color &color() const;
	Formats formats() const;

	NcFormattedColor *cFormattedColor();
	const NcFormattedColor *cFormattedColor() const;

private:
	NcFormattedColor m_impl;
	mutable Color m_color_cache;
};

inline bool operator==(const FormattedColor &lhs, const FormattedColor &rhs)
{
	return nc_formatted_color_equal(
		const_cast<NcFormattedColor *>(lhs.cFormattedColor()),
		const_cast<NcFormattedColor *>(rhs.cFormattedColor()));
}

NcColor toNcColor(Color color);
Color fromNcColor(NcColor color);
NcFormat toNcFormat(Format format);
Format fromNcFormat(NcFormat format);

std::istream &operator>>(std::istream &is, FormattedColor &fc);

template <typename OutputStreamT>
OutputStreamT &operator<<(OutputStreamT &os, const FormattedColor &fc)
{
	os << fc.color();
	for (auto &fmt : fc.formats())
		os << fmt;
	return os;
}

template <typename OutputStreamT, StorageKind storage>
OutputStreamT &operator<<(OutputStreamT &os,
                          const FormattedColor::End<storage> &rfc)
{
	if (rfc.base().color() != Color::Default)
		os << Color::End;
	auto formats = rfc.base().formats();
	for (auto it = formats.rbegin(); it != formats.rend(); ++it)
		os << reverseFormat(*it);
	return os;
}

}

#endif // NCMPCPP_FORMATTED_COLOR_H
