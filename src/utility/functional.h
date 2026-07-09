#ifndef NCMPCPP_UTILITY_FUNCTIONAL_H
#define NCMPCPP_UTILITY_FUNCTIONAL_H

#include <algorithm>
#include <string>
#include <type_traits>
#include <utility>

/// Map over the first element in range satisfying the predicate.
template <typename InputIterator, typename PredicateT, typename MapT>
InputIterator find_map_first(InputIterator first, InputIterator last, PredicateT &&p, MapT &&f)
{
	auto it = std::find(first, last, std::forward<PredicateT>(p));
	if (it != last)
		f(*it);
	return it;
}

/// Map over all elements in range satisfying the predicate.
template <typename InputIterator, typename PredicateT, typename MapT>
void find_map_all(InputIterator first, InputIterator last, PredicateT &&p, MapT &&f)
{
	InputIterator it = first;
	do
		it = find_map_first(it, last, p, f);
	while (it != last);
}

// convert string to appropriate type
template <typename TargetT, typename SourceT>
struct convertString
{
	static std::basic_string<TargetT> apply(const std::basic_string<SourceT> &s)
	{
		static_assert(std::is_same_v<TargetT, SourceT>,
		              "non-UTF-8 string conversions are not supported");
		return s;
	}
};
template <typename TargetT>
struct convertString<TargetT, TargetT>
{
	static const std::basic_string<TargetT> &apply(const std::basic_string<TargetT> &s)
	{
		return s;
	}
};


#endif // NCMPCPP_UTILITY_FUNCTIONAL_H
