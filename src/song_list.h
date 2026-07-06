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

#ifndef NCMPCPP_SONG_LIST_H
#define NCMPCPP_SONG_LIST_H

#include <cassert>
#include <type_traits>
#include <utility>
#include <vector>

#include "c/ncm_song_list.h"
#include "curses/menu.h"
#include "song.h"
#include "utility/any_iterator.h"
#include "utility/const.h"
#include "utility/transform_iterator.h"

struct SongProperties
{
	enum class State { Undefined, Const, Mutable };

	SongProperties()
		: m_state(State::Undefined)
	{ }

	SongProperties &assign(NC::List::Properties *properties_,
	                       MPD::Song *song_)
	{
		m_state = State::Mutable;
		m_properties = properties_;
		m_song = song_;
		return *this;
	}

	SongProperties &assign(const NC::List::Properties *properties_,
	                       const MPD::Song *song_)
	{
		m_state = State::Const;
		m_const_properties = properties_;
		m_const_song = song_;
		return *this;
	}

	const NC::List::Properties &properties() const
	{
		assert(m_state != State::Undefined);
		return *m_const_properties;
	}
	const MPD::Song *song() const
	{
		assert(m_state != State::Undefined);
		return m_const_song;
	}

	NC::List::Properties &properties()
	{
		assert(m_state == State::Mutable);
		return *m_properties;
	}
	MPD::Song *song()
	{
		assert(m_state == State::Mutable);
		return m_song;
	}

private:
	State m_state;

	union {
		NC::List::Properties *m_properties;
		const NC::List::Properties *m_const_properties;
	};
	union {
		MPD::Song *m_song;
		const MPD::Song *m_const_song;
	};
};

template <Const const_>
using SongIteratorT = Utility::AnyRandomAccessIterator<
	typename std::conditional<
		const_ == Const::Yes,
		const SongProperties,
		SongProperties>::type>;

typedef SongIteratorT<Const::No> SongIterator;
typedef SongIteratorT<Const::Yes> ConstSongIterator;

struct SongList
{
	virtual SongIterator currentS() = 0;
	virtual ConstSongIterator currentS() const = 0;
	virtual SongIterator beginS() = 0;
	virtual ConstSongIterator beginS() const = 0;
	virtual SongIterator endS() = 0;
	virtual ConstSongIterator endS() const = 0;

	virtual std::vector<MPD::Song> getSelectedSongs() = 0;
};

inline SongIterator begin(SongList &list) { return list.beginS(); }
inline ConstSongIterator begin(const SongList &list) { return list.beginS(); }
inline SongIterator end(SongList &list) { return list.endS(); }
inline ConstSongIterator end(const SongList &list) { return list.endS(); }

template <typename SongT>
struct SongPropertiesExtractor
{
	template <typename ItemT>
	auto &operator()(ItemT &item) const
	{
		return m_cache.assign(&item.properties(), &item.value());
	}

private:
	mutable SongProperties m_cache;
};

template <typename IteratorT>
inline SongIterator makeSongIterator(IteratorT it)
{
	typedef SongPropertiesExtractor<
		typename IteratorT::value_type::Type
		> Extractor;
	static_assert(
		std::is_convertible<
		std::invoke_result_t<Extractor, typename IteratorT::reference>,
		SongProperties &
		>::value, "invalid result type of SongPropertiesExtractor");
	return SongIterator(Utility::makeTransformIterator(it, Extractor{}));
}

template <typename ConstIteratorT>
inline ConstSongIterator makeConstSongIterator(ConstIteratorT it)
{
	typedef SongPropertiesExtractor<
		typename ConstIteratorT::value_type::Type
		> Extractor;
	static_assert(
		std::is_convertible<
		std::invoke_result_t<Extractor, typename ConstIteratorT::reference>,
		const SongProperties &
		>::value, "invalid result type of SongPropertiesExtractor");
	return ConstSongIterator(Utility::makeTransformIterator(it, Extractor{}));
}

struct SongMenu: NC::Menu<MPD::Song>, SongList
{
	SongMenu() { }
	SongMenu(NC::Menu<MPD::Song> &&base)
	: NC::Menu<MPD::Song>(std::move(base)) { }

	virtual SongIterator currentS() override;
	virtual ConstSongIterator currentS() const override;
	virtual SongIterator beginS() override;
	virtual ConstSongIterator beginS() const override;
	virtual SongIterator endS() override;
	virtual ConstSongIterator endS() const override;

	virtual std::vector<MPD::Song> getSelectedSongs() override;
};

inline SongIterator SongMenu::currentS()
{
	return makeSongIterator(current());
}

inline ConstSongIterator SongMenu::currentS() const
{
	return makeConstSongIterator(current());
}

inline SongIterator SongMenu::beginS()
{
	return makeSongIterator(begin());
}

inline ConstSongIterator SongMenu::beginS() const
{
	return makeConstSongIterator(begin());
}

inline SongIterator SongMenu::endS()
{
	return makeSongIterator(end());
}

inline ConstSongIterator SongMenu::endS() const
{
	return makeConstSongIterator(end());
}

inline std::vector<MPD::Song> SongMenu::getSelectedSongs()
{
	std::vector<MPD::Song> result;
	for (auto it = begin(); it != end(); ++it)
		if (it->isSelected())
			result.push_back(it->value());
	if (result.empty() && !empty())
		result.push_back(current()->value());
	return result;
}

#endif // NCMPCPP_SONG_LIST_H
