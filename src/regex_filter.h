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

}

#endif // NCMPCPP_REGEX_FILTER_H
