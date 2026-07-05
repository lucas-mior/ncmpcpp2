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
	initItemStorage();
	syncMenuPrefixes();
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
	initItemStorage();
	auto fc = FormattedColor(m_base_color, {Format::Reverse});
	m_highlight_prefix << fc;
	m_highlight_suffix << FormattedColor::End<>(fc);
	syncMenuPrefixes();
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
	nc_menu_init(&m_menu);
	initItemStorage();
	nc_menu_copy(&m_menu, const_cast<NcMenu *>(&rhs.m_menu));
	syncDisplayCallbacks();
	syncActionCallbacks();
	// TODO: move filtered items
	for (int64 i = 0;
	     i < nc_menu_all_item_count(const_cast<NcMenu *>(&rhs.m_menu));
	     i += 1)
	{
		Item item = static_cast<const Item *>(
			nc_menu_item_at(
				const_cast<NcMenu *>(&rhs.m_menu),
				NC_MENU_ITEMS_ALL,
				i))->copy();
		nc_menu_add_item(&m_menu, &item);
	}
	nc_menu_show_all_items(&m_menu);
	syncMenuSize();
}

template <typename ItemT>
Menu<ItemT>::Menu(Menu &&rhs)
	: Window(rhs)
	, m_item_displayer(std::move(rhs.m_item_displayer))
	, m_filter_predicate(std::move(rhs.m_filter_predicate))
	, m_highlight_prefix(std::move(rhs.m_highlight_prefix))
	, m_highlight_suffix(std::move(rhs.m_highlight_suffix))
	, m_selected_prefix(std::move(rhs.m_selected_prefix))
	, m_selected_suffix(std::move(rhs.m_selected_suffix))
{
	nc_menu_init(&m_menu);
	initItemStorage();
	nc_menu_swap(&m_menu, &rhs.m_menu);
	syncDisplayCallbacks();
	syncActionCallbacks();
	rhs.syncDisplayCallbacks();
	rhs.syncActionCallbacks();
}

template <typename ItemT>
Menu<ItemT>::~Menu()
{
	nc_menu_destroy(&m_menu);
}

template <typename ItemT>
Menu<ItemT> &Menu<ItemT>::operator=(Menu rhs)
{
	std::swap(static_cast<Window &>(*this), static_cast<Window &>(rhs));
	std::swap(m_item_displayer, rhs.m_item_displayer);
	std::swap(m_filter_predicate, rhs.m_filter_predicate);
	nc_menu_swap(&m_menu, &rhs.m_menu);
	std::swap(m_highlight_prefix, rhs.m_highlight_prefix);
	std::swap(m_highlight_suffix, rhs.m_highlight_suffix);
	std::swap(m_selected_prefix, rhs.m_selected_prefix);
	std::swap(m_selected_suffix, rhs.m_selected_suffix);
	syncDisplayCallbacks();
	syncActionCallbacks();
	rhs.syncDisplayCallbacks();
	rhs.syncActionCallbacks();
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
	nc_menu_resize_all_items(&m_menu, static_cast<int64>(new_size));
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::addItem(ItemT item, Properties::Type properties)
{
	Item menu_item(std::move(item), properties);

	nc_menu_add_item(&m_menu, &menu_item);
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::addSeparator()
{
	Item item = Item::mkSeparator();

	nc_menu_add_item(&m_menu, &item);
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::insertItem(size_t pos, ItemT item, Properties::Type properties)
{
	Item menu_item(std::move(item), properties);

	nc_menu_insert_item(&m_menu, static_cast<int64>(pos), &menu_item);
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::insertSeparator(size_t pos)
{
	Item item = Item::mkSeparator();

	nc_menu_insert_item(&m_menu, static_cast<int64>(pos), &item);
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
	nc_menu_refresh(&m_menu, cWindow(), static_cast<int64>(m_width),
	                static_cast<int64>(m_height));
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
	// Don't clear filter predicate here.
	nc_menu_clear_items(&m_menu);
	syncMenuSize();
}

template <typename ItemT>
void Menu<ItemT>::highlight(size_t pos)
{
	syncMenuSize();
	assert(pos < size());
	nc_menu_highlight_position(&m_menu, static_cast<int64>(pos),
	                           static_cast<int64>(m_height));
}

template <typename ItemT>
size_t Menu<ItemT>::choice() const
{
	assert(!empty());
	return static_cast<size_t>(
		nc_menu_highlight(const_cast<NcMenu *>(&m_menu)));
}

template <typename ItemT> template <typename PredicateT>
void Menu<ItemT>::applyFilter(PredicateT &&pred)
{
	m_filter_predicate = std::forward<PredicateT>(pred);
	nc_menu_apply_filter(&m_menu);
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
	nc_menu_clear_filtered_items(&m_menu);
	nc_menu_show_all_items(&m_menu);
	syncMenuSize();
}

}

#endif // NCMPCPP_MENU_IMPL_H
