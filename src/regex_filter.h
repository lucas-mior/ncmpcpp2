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

#ifndef NCMPCPP_REGEX_FILTER_H
#define NCMPCPP_REGEX_FILTER_H

#include <cassert>
#include <functional>
#include <string>
#include <utility>

#include "utility/regex.h"

namespace Regex {

template <typename T>
struct Filter
{
	typedef NC::Menu<T> MenuT;
	typedef typename NC::Menu<T>::Item Item;
	typedef std::function<bool(const Regex &, const T &)> FilterFunction;

	Filter() { }

	template <typename FilterT>
	Filter(const std::string &constraint_,
	       Flags flags,
	       FilterT &&filter)
		: m_rx(make(constraint_, flags))
		, m_constraint(constraint_)
		, m_filter(std::forward<FilterT>(filter))
	{ }

	void clear()
	{
		m_filter = nullptr;
	}

	const std::string &constraint() const {
		return m_constraint;
	}

	bool operator()(const Item &item) const {
		assert(defined());
		return m_filter(m_rx, item.value());
	}

	bool defined() const
	{
		return m_filter.operator bool();
	}

private:
	Regex m_rx;
	std::string m_constraint;
	FilterFunction m_filter;
};

template <typename T> struct ItemFilter
{
	typedef NC::Menu<T> MenuT;
	typedef typename NC::Menu<T>::Item Item;
	typedef std::function<bool(const Regex &, const Item &)> FilterFunction;
	
	ItemFilter() { }

	template <typename FilterT>
	ItemFilter(const std::string &constraint_,
	           Flags flags,
	           FilterT &&filter)
		: m_rx(make(constraint_, flags))
		, m_constraint(constraint_)
		, m_filter(std::forward<FilterT>(filter))
	{ }
	
	void clear()
	{
		m_filter = nullptr;
	}

	const std::string &constraint() const {
		return m_constraint;
	}

	bool operator()(const Item &item) {
		return m_filter(m_rx, item);
	}
	
	bool defined() const
	{
		return m_filter.operator bool();
	}

private:
	Regex m_rx;
	std::string m_constraint;
	FilterFunction m_filter;
};

}

#endif // NCMPCPP_REGEX_FILTER_H
