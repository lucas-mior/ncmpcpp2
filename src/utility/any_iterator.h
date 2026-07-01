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

#ifndef NCMPCPP_UTILITY_ANY_ITERATOR_H
#define NCMPCPP_UTILITY_ANY_ITERATOR_H

#include <cassert>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace Utility {

template <typename ValueT>
struct AnyRandomAccessIterator
{
	typedef std::random_access_iterator_tag iterator_category;
	typedef std::remove_cv_t<ValueT> value_type;
	typedef std::ptrdiff_t difference_type;
	typedef ValueT &reference;
	typedef ValueT *pointer;

	AnyRandomAccessIterator() = default;

	template <typename IteratorT,
	          typename ReferenceT = typename std::iterator_traits<IteratorT>::reference,
	          typename = std::enable_if_t<
	              !std::is_same_v<std::decay_t<IteratorT>, AnyRandomAccessIterator>
	              && std::is_convertible_v<ReferenceT, reference>>>
	AnyRandomAccessIterator(IteratorT iterator)
		: m_iterator(std::make_unique<IteratorModel<IteratorT>>(std::move(iterator)))
	{ }

	AnyRandomAccessIterator(const AnyRandomAccessIterator &rhs)
		: m_iterator(rhs.m_iterator ? rhs.m_iterator->clone() : nullptr)
	{ }

	AnyRandomAccessIterator(AnyRandomAccessIterator &&rhs) = default;

	AnyRandomAccessIterator &operator=(const AnyRandomAccessIterator &rhs)
	{
		m_iterator = rhs.m_iterator ? rhs.m_iterator->clone() : nullptr;
		return *this;
	}

	AnyRandomAccessIterator &operator=(AnyRandomAccessIterator &&rhs) = default;

	reference operator*() const
	{
		assert(m_iterator != nullptr);
		return m_iterator->dereference();
	}

	pointer operator->() const
	{
		return std::addressof(operator*());
	}

	reference operator[](difference_type n) const
	{
		return *(*this + n);
	}

	AnyRandomAccessIterator &operator++()
	{
		assert(m_iterator != nullptr);
		m_iterator->increment();
		return *this;
	}

	AnyRandomAccessIterator operator++(int)
	{
		auto result = *this;
		++*this;
		return result;
	}

	AnyRandomAccessIterator &operator--()
	{
		assert(m_iterator != nullptr);
		m_iterator->decrement();
		return *this;
	}

	AnyRandomAccessIterator operator--(int)
	{
		auto result = *this;
		--*this;
		return result;
	}

	AnyRandomAccessIterator &operator+=(difference_type n)
	{
		assert(m_iterator != nullptr);
		m_iterator->advance(n);
		return *this;
	}

	AnyRandomAccessIterator &operator-=(difference_type n)
	{
		return *this += -n;
	}

	friend AnyRandomAccessIterator operator+(AnyRandomAccessIterator iterator, difference_type n)
	{
		iterator += n;
		return iterator;
	}

	friend AnyRandomAccessIterator operator+(difference_type n, AnyRandomAccessIterator iterator)
	{
		iterator += n;
		return iterator;
	}

	friend AnyRandomAccessIterator operator-(AnyRandomAccessIterator iterator, difference_type n)
	{
		iterator -= n;
		return iterator;
	}

	friend difference_type operator-(
	        const AnyRandomAccessIterator &lhs,
	        const AnyRandomAccessIterator &rhs)
	{
		if (lhs.m_iterator == nullptr || rhs.m_iterator == nullptr) {
			assert(lhs.m_iterator == nullptr && rhs.m_iterator == nullptr);
			return 0;
		}
		return lhs.m_iterator->distance(*rhs.m_iterator);
	}

	friend bool operator==(
	        const AnyRandomAccessIterator &lhs,
	        const AnyRandomAccessIterator &rhs)
	{
		if (lhs.m_iterator == nullptr || rhs.m_iterator == nullptr)
			return lhs.m_iterator == nullptr && rhs.m_iterator == nullptr;
		return lhs.m_iterator->equals(*rhs.m_iterator);
	}

	friend bool operator!=(
	        const AnyRandomAccessIterator &lhs,
	        const AnyRandomAccessIterator &rhs)
	{
		return !(lhs == rhs);
	}

	friend bool operator<(
	        const AnyRandomAccessIterator &lhs,
	        const AnyRandomAccessIterator &rhs)
	{
		assert(lhs.m_iterator != nullptr && rhs.m_iterator != nullptr);
		return lhs.m_iterator->less(*rhs.m_iterator);
	}

	friend bool operator>(
	        const AnyRandomAccessIterator &lhs,
	        const AnyRandomAccessIterator &rhs)
	{
		return rhs < lhs;
	}

	friend bool operator<=(
	        const AnyRandomAccessIterator &lhs,
	        const AnyRandomAccessIterator &rhs)
	{
		return !(rhs < lhs);
	}

	friend bool operator>=(
	        const AnyRandomAccessIterator &lhs,
	        const AnyRandomAccessIterator &rhs)
	{
		return !(lhs < rhs);
	}

private:
	struct IteratorConcept
	{
		virtual ~IteratorConcept() = default;
		virtual std::unique_ptr<IteratorConcept> clone() const = 0;
		virtual reference dereference() const = 0;
		virtual void increment() = 0;
		virtual void decrement() = 0;
		virtual void advance(difference_type n) = 0;
		virtual difference_type distance(const IteratorConcept &rhs) const = 0;
		virtual bool equals(const IteratorConcept &rhs) const = 0;
		virtual bool less(const IteratorConcept &rhs) const = 0;
	};

	template <typename IteratorT>
	struct IteratorModel: IteratorConcept
	{
		IteratorModel(IteratorT iterator)
			: m_iterator(std::move(iterator))
		{ }

		virtual std::unique_ptr<IteratorConcept> clone() const override
		{
			return std::make_unique<IteratorModel>(m_iterator);
		}

		virtual reference dereference() const override
		{
			return *m_iterator;
		}

		virtual void increment() override
		{
			++m_iterator;
		}

		virtual void decrement() override
		{
			--m_iterator;
		}

		virtual void advance(difference_type n) override
		{
			m_iterator += n;
		}

		virtual difference_type distance(const IteratorConcept &rhs) const override
		{
			auto rhs_model = dynamic_cast<const IteratorModel *>(&rhs);
			assert(rhs_model != nullptr);
			return m_iterator - rhs_model->m_iterator;
		}

		virtual bool equals(const IteratorConcept &rhs) const override
		{
			auto rhs_model = dynamic_cast<const IteratorModel *>(&rhs);
			assert(rhs_model != nullptr);
			return rhs_model != nullptr && m_iterator == rhs_model->m_iterator;
		}

		virtual bool less(const IteratorConcept &rhs) const override
		{
			auto rhs_model = dynamic_cast<const IteratorModel *>(&rhs);
			assert(rhs_model != nullptr);
			return rhs_model != nullptr && m_iterator < rhs_model->m_iterator;
		}

		IteratorT m_iterator;
	};

	std::unique_ptr<IteratorConcept> m_iterator;
};

} // namespace Utility

#endif // NCMPCPP_UTILITY_ANY_ITERATOR_H
