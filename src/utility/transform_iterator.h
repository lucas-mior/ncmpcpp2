#ifndef NCMPCPP_UTILITY_TRANSFORM_ITERATOR_H
#define NCMPCPP_UTILITY_TRANSFORM_ITERATOR_H

#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace Utility {

template <typename Function, typename Iterator>
struct TransformIterator
{
private:
	using IteratorTraits = std::iterator_traits<Iterator>;

public:
	using iterator_category = typename IteratorTraits::iterator_category;
	using difference_type = typename IteratorTraits::difference_type;
	using reference = std::invoke_result_t<
		const Function &,
		typename IteratorTraits::reference>;
	using value_type = std::remove_cv_t<std::remove_reference_t<reference>>;
	using pointer = std::add_pointer_t<std::remove_reference_t<reference>>;

	TransformIterator() = default;
	TransformIterator(Iterator iterator, Function function = Function())
		: m_iterator(std::move(iterator)),
		  m_function(std::move(function))
	{ }

	Iterator base() const { return m_iterator; }

	reference operator*() const
	{
		return m_function(*m_iterator);
	}
	pointer operator->() const
	{
		return std::addressof(operator*());
	}
	reference operator[](difference_type n) const
	{
		return *(*this + n);
	}

	TransformIterator &operator++()
	{
		++m_iterator;
		return *this;
	}
	TransformIterator operator++(int)
	{
		auto result = *this;
		++*this;
		return result;
	}
	TransformIterator &operator--()
	{
		--m_iterator;
		return *this;
	}
	TransformIterator operator--(int)
	{
		auto result = *this;
		--*this;
		return result;
	}

	TransformIterator &operator+=(difference_type n)
	{
		m_iterator += n;
		return *this;
	}
	TransformIterator &operator-=(difference_type n)
	{
		m_iterator -= n;
		return *this;
	}

private:
	Iterator m_iterator = Iterator();
	Function m_function = Function();
};

template <typename Function, typename Iterator>
TransformIterator<Function, Iterator> makeTransformIterator(Iterator iterator, Function function)
{
	return TransformIterator<Function, Iterator>(std::move(iterator), std::move(function));
}

template <typename Function, typename Iterator>
TransformIterator<Function, Iterator> operator+(
	TransformIterator<Function, Iterator> iterator,
	typename TransformIterator<Function, Iterator>::difference_type n)
{
	iterator += n;
	return iterator;
}

template <typename Function, typename Iterator>
TransformIterator<Function, Iterator> operator+(
	typename TransformIterator<Function, Iterator>::difference_type n,
	TransformIterator<Function, Iterator> iterator)
{
	iterator += n;
	return iterator;
}

template <typename Function, typename Iterator>
TransformIterator<Function, Iterator> operator-(
	TransformIterator<Function, Iterator> iterator,
	typename TransformIterator<Function, Iterator>::difference_type n)
{
	iterator -= n;
	return iterator;
}

template <typename Function, typename Iterator>
auto operator-(
	const TransformIterator<Function, Iterator> &lhs,
	const TransformIterator<Function, Iterator> &rhs)
{
	return lhs.base() - rhs.base();
}

template <typename Function, typename Iterator>
bool operator==(
	const TransformIterator<Function, Iterator> &lhs,
	const TransformIterator<Function, Iterator> &rhs)
{
	return lhs.base() == rhs.base();
}

template <typename Function, typename Iterator>
bool operator!=(
	const TransformIterator<Function, Iterator> &lhs,
	const TransformIterator<Function, Iterator> &rhs)
{
	return !(lhs == rhs);
}

template <typename Function, typename Iterator>
bool operator<(
	const TransformIterator<Function, Iterator> &lhs,
	const TransformIterator<Function, Iterator> &rhs)
{
	return lhs.base() < rhs.base();
}

template <typename Function, typename Iterator>
bool operator>(
	const TransformIterator<Function, Iterator> &lhs,
	const TransformIterator<Function, Iterator> &rhs)
{
	return rhs < lhs;
}

template <typename Function, typename Iterator>
bool operator<=(
	const TransformIterator<Function, Iterator> &lhs,
	const TransformIterator<Function, Iterator> &rhs)
{
	return !(rhs < lhs);
}

template <typename Function, typename Iterator>
bool operator>=(
	const TransformIterator<Function, Iterator> &lhs,
	const TransformIterator<Function, Iterator> &rhs)
{
	return !(lhs < rhs);
}

} // namespace Utility

#endif // NCMPCPP_UTILITY_TRANSFORM_ITERATOR_H
