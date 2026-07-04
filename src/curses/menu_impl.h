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

#ifndef NCMPCPP_MENU_IMPL_H
#define NCMPCPP_MENU_IMPL_H

#include "menu.h"

namespace NC {

template <typename ItemT>
Menu<ItemT>::Menu()
{
	nc_menu_init(&m_menu);
	m_items = &m_all_items;
	syncMenuSize();
}

template <typename ItemT>
Menu<ItemT>::Menu(size_t startx,
                  size_t starty,
                  size_t width,
                  size_t height,
                  const std::string &title,
                  Color color,
                  Border border)
	: Window(startx, starty, width, height, title, color, border)
	, m_item_displayer(nullptr)
	, m_filter_predicate(nullptr)
{
	nc_menu_init(&m_menu);
	auto fc = FormattedColor(m_base_color, {Format::Reverse});
	m_highlight_prefix << fc;
	m_highlight_suffix << FormattedColor::End<>(fc);
	m_items = &m_all_items;
	syncMenuSize();
}

template <typename ItemT>
Menu<ItemT>::Menu(const Menu &rhs)
	: Window(rhs)
	, m_item_displayer(rhs.m_item_displayer)
	, m_filter_predicate(rhs.m_filter_predicate)
	, m_highlight_prefix(rhs.m_highlight_prefix)
	, m_highlight_suffix(rhs.m_highlight_suffix)
	, m_selected_prefix(rhs.m_selected_prefix)
	, m_selected_suffix(rhs.m_selected_suffix)
{
	nc_menu_copy(&m_menu, const_cast<NcMenu *>(&rhs.m_menu));
	// TODO: move filtered items
	m_all_items.reserve(rhs.m_all_items.size());
	for (const auto &item : rhs.m_all_items)
		m_all_items.push_back(item.copy());
	m_items = &m_all_items;
	syncMenuSize();
}

template <typename ItemT>
Menu<ItemT>::Menu(Menu &&rhs)
	: Window(rhs)
	, m_item_displayer(std::move(rhs.m_item_displayer))
	, m_filter_predicate(std::move(rhs.m_filter_predicate))
	, m_all_items(std::move(rhs.m_all_items))
	, m_filtered_items(std::move(rhs.m_filtered_items))
	, m_highlight_prefix(std::move(rhs.m_highlight_prefix))
	, m_highlight_suffix(std::move(rhs.m_highlight_suffix))
	, m_selected_prefix(std::move(rhs.m_selected_prefix))
	, m_selected_suffix(std::move(rhs.m_selected_suffix))
{
	nc_menu_copy(&m_menu, &rhs.m_menu);
	if (rhs.m_items == &rhs.m_all_items)
		m_items = &m_all_items;
	else
		m_items = &m_filtered_items;
	syncMenuSize();
}

template <typename ItemT>
Menu<ItemT> &Menu<ItemT>::operator=(Menu rhs)
{
	std::swap(static_cast<Window &>(*this), static_cast<Window &>(rhs));
	std::swap(m_item_displayer, rhs.m_item_displayer);
	std::swap(m_filter_predicate, rhs.m_filter_predicate);
	std::swap(m_all_items, rhs.m_all_items);
	std::swap(m_filtered_items, rhs.m_filtered_items);
	nc_menu_swap(&m_menu, &rhs.m_menu);
	std::swap(m_highlight_prefix, rhs.m_highlight_prefix);
	std::swap(m_highlight_suffix, rhs.m_highlight_suffix);
	std::swap(m_selected_prefix, rhs.m_selected_prefix);
	std::swap(m_selected_suffix, rhs.m_selected_suffix);
	if (rhs.m_items == &rhs.m_all_items)
		m_items = &m_all_items;
	else
		m_items = &m_filtered_items;
	syncMenuSize();
	return *this;
}

template <typename ItemT> template <typename ItemDisplayerT>
void Menu<ItemT>::setItemDisplayer(ItemDisplayerT &&displayer)
{
	m_item_displayer = std::forward<ItemDisplayerT>(displayer);
}

template <typename ItemT>
void Menu<ItemT>::resizeList(size_t new_size)
{
	m_all_items.resize(new_size);
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::addItem(ItemT item, Properties::Type properties)
{
	m_all_items.push_back(Item(std::move(item), properties));
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::addSeparator()
{
	m_all_items.push_back(Item::mkSeparator());
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::insertItem(size_t pos, ItemT item, Properties::Type properties)
{
	m_all_items.insert(m_all_items.begin()+pos, Item(std::move(item), properties));
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::insertSeparator(size_t pos)
{
	m_all_items.insert(m_all_items.begin()+pos, Item::mkSeparator());
	syncMenuSize();
}

template <typename ItemT>
bool Menu<ItemT>::Goto(size_t y)
{
	syncMenuSize();
	return nc_menu_goto(&m_menu, static_cast<int64>(y),
	                    &Menu<ItemT>::isHighlightableCallback, this);
}

template <typename ItemT>
void Menu<ItemT>::refresh()
{
	syncMenuSize();
	if (m_menu.item_count == 0)
	{
		Window::clear();
		Window::refresh();
		return;
	}

	nc_menu_prepare_refresh(&m_menu, static_cast<int64>(m_height),
	                        &Menu<ItemT>::isHighlightableCallback, this);

	size_t line = 0;
	const size_t end_ = static_cast<size_t>(m_menu.beginning) + m_height;
	for (size_t pos = static_cast<size_t>(m_menu.beginning); pos < end_; ++pos, ++line)
	{
		nc_menu_set_drawn_position(&m_menu, static_cast<int64>(pos));
		goToXY(0, line);
		if (pos >= m_items->size())
		{
			for (; line < m_height; ++line)
				mvwhline(m_window, line, 0, NC::Key::Space, m_width);
			break;
		}
		if ((*m_items)[pos].isSeparator())
		{
			mvwhline(m_window, line, 0, 0, m_width);
			continue;
		}
		if (m_menu.highlight_enabled && pos == static_cast<size_t>(m_menu.highlight))
			*this << m_highlight_prefix;
		if ((*m_items)[pos].isSelected())
			*this << m_selected_prefix;
		*this << NC::TermManip::ClearToEOL;
		if (m_item_displayer)
			m_item_displayer(*this);
		if ((*m_items)[pos].isSelected())
			*this << m_selected_suffix;
		if (m_menu.highlight_enabled && pos == static_cast<size_t>(m_menu.highlight))
			*this << m_highlight_suffix;
	}
	Window::refresh();
}

template <typename ItemT>
void Menu<ItemT>::scroll(Scroll where)
{
	syncMenuSize();
	nc_menu_scroll(&m_menu, static_cast<int64>(m_height),
	               Menu<ItemT>::toNcScroll(where),
	               &Menu<ItemT>::isHighlightableCallback, this);
}

template <typename ItemT>
void Menu<ItemT>::reset()
{
	nc_menu_reset(&m_menu);
}

template <typename ItemT>
void Menu<ItemT>::clear()
{
	// Don't clear filter related stuff here.
	m_all_items.clear();
	m_filtered_items.clear();
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::highlight(size_t pos)
{
	syncMenuSize();
	assert(pos < m_items->size());
	nc_menu_highlight_position(&m_menu, static_cast<int64>(pos),
	                           static_cast<int64>(m_height));
}

template <typename ItemT>
size_t Menu<ItemT>::choice() const
{
	assert(!empty());
	return static_cast<size_t>(m_menu.highlight);
}

template <typename ItemT> template <typename PredicateT>
void Menu<ItemT>::applyFilter(PredicateT &&pred)
{
	m_filter_predicate = std::forward<PredicateT>(pred);
	m_filtered_items.clear();

	for (const auto &item : m_all_items)
		if (m_filter_predicate(item))
			m_filtered_items.push_back(item);

	m_items = &m_filtered_items;
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::reapplyFilter()
{
	applyFilter(m_filter_predicate);
}

template <typename ItemT> template <typename TargetT>
const TargetT *Menu<ItemT>::filterPredicate() const
{
	return m_filter_predicate.template target<TargetT>();
}

template <typename ItemT>
void Menu<ItemT>::clearFilter()
{
	m_filter_predicate = nullptr;
	m_filtered_items.clear();
	m_items = &m_all_items;
	syncMenuSize();
}

}

#endif // NCMPCPP_MENU_IMPL_H
