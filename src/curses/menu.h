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

#ifndef NCMPCPP_MENU_H
#define NCMPCPP_MENU_H

#include <cassert>
#include <functional>
#include <iterator>
#include <memory>
#include <new>
#include <set>
#include <stdexcept>
#include <type_traits>

#include "curses/formatted_color.h"
#include "curses/nc_menu.h"
#include "curses/strbuffer.h"
#include "curses/window.h"
#include "utility/any_iterator.h"
#include "utility/const.h"
#include "utility/transform_iterator.h"

namespace NC {

struct List
{
	struct Properties
	{
		enum Type {
			None       = 0,
			Selectable = (1 << 0),
			Selected   = (1 << 1),
			Inactive   = (1 << 2),
			Separator  = (1 << 3)
		};

		Properties(Type properties = Selectable)
		: m_properties(properties)
		{ }

		void setSelectable(bool is_selectable)
		{
			if (is_selectable)
				m_properties |= Selectable;
			else
				m_properties &= ~(Selectable | Selected);
		}
		void setSelected(bool is_selected)
		{
			if (!isSelectable())
				return;
			if (is_selected)
				m_properties |= Selected;
			else
				m_properties &= ~Selected;
		}
		void setInactive(bool is_inactive)
		{
			if (is_inactive)
				m_properties |= Inactive;
			else
				m_properties &= ~Inactive;
		}
		void setSeparator(bool is_separator)
		{
			if (is_separator)
				m_properties |= Separator;
			else
				m_properties &= ~Separator;
		}

		bool isSelectable() const { return m_properties & Selectable; }
		bool isSelected() const { return m_properties & Selected; }
		bool isInactive() const { return m_properties & Inactive; }
		bool isSeparator() const { return m_properties & Separator; }

	private:
		unsigned m_properties;
	};

	template <typename ValueT>
	using PropertiesIterator = Utility::AnyRandomAccessIterator<ValueT>;

	typedef PropertiesIterator<Properties> Iterator;
	typedef PropertiesIterator<const Properties> ConstIterator;

	virtual ~List() { }

	virtual bool empty() const = 0;
	virtual size_t size() const = 0;
	virtual size_t choice() const = 0;
	virtual void highlight(size_t pos) = 0;

	virtual Iterator currentP() = 0;
	virtual ConstIterator currentP() const = 0;
	virtual Iterator beginP() = 0;
	virtual ConstIterator beginP() const = 0;
	virtual Iterator endP() = 0;
	virtual ConstIterator endP() const = 0;
};

inline List::Properties::Type operator|(List::Properties::Type lhs, List::Properties::Type rhs)
{
	return List::Properties::Type(unsigned(lhs) | unsigned(rhs));
}
inline List::Properties::Type &operator|=(List::Properties::Type &lhs, List::Properties::Type rhs)
{
	lhs = lhs | rhs;
	return lhs;
}
inline List::Properties::Type operator&(List::Properties::Type lhs, List::Properties::Type rhs)
{
	return List::Properties::Type(unsigned(lhs) & unsigned(rhs));
}
inline List::Properties::Type &operator&=(List::Properties::Type &lhs, List::Properties::Type rhs)
{
	lhs = lhs & rhs;
	return lhs;
}

// for range-based for loop
inline List::Iterator begin(List &list) { return list.beginP(); }
inline List::ConstIterator begin(const List &list) { return list.beginP(); }
inline List::Iterator end(List &list) { return list.endP(); }
inline List::ConstIterator end(const List &list) { return list.endP(); }

/// Generic menu capable of holding any std::vector compatible values.
template <typename ItemT>
struct Menu: Window, List
{
	struct Item
	{
		friend struct Menu<ItemT>;

		typedef ItemT Type;

		Item()
			: m_impl(std::make_shared<std::tuple<ItemT, Properties>>())
		{ }

		template <typename ValueT, typename PropertiesT>
		Item(ValueT &&value_, PropertiesT properties_)
			: m_impl(
				std::make_shared<std::tuple<ItemT, List::Properties>>(
					std::forward<ValueT>(value_),
					std::forward<PropertiesT>(properties_)))
		{ }

		ItemT &value() { return std::get<0>(*m_impl); }
		const ItemT &value() const { return std::get<0>(*m_impl); }

		Properties &properties() { return std::get<1>(*m_impl); }
		const Properties &properties() const { return std::get<1>(*m_impl); }

		// Forward methods to List::Properties.
		void setSelectable(bool is_selectable) { properties().setSelectable(is_selectable); }
		void setSelected (bool is_selected) { properties().setSelected(is_selected); }
		void setInactive (bool is_inactive) { properties().setInactive(is_inactive); }
		void setSeparator (bool is_separator) { properties().setSeparator(is_separator); }

		bool isSelectable() const { return properties().isSelectable(); }
		bool isSelected() const { return properties().isSelected(); }
		bool isInactive() const { return properties().isInactive(); }
		bool isSeparator() const { return properties().isSeparator(); }

		// Make a deep copy of Item.
		Item copy() const {
			return Item(value(), properties());
		}

	private:
		template <Const const_>
		struct ExtractProperties
		{
			typedef ExtractProperties type;

			typedef typename std::conditional<
				const_ == Const::Yes,
				const Properties,
				Properties>::type Properties_;
			typedef typename std::conditional<
				const_ == Const::Yes,
				const Item,
				Item>::type Item_;

			Properties_ &operator()(Item_ &i) const {
				return i.properties();
			}
		};

		template <Const const_>
		struct ExtractValue
		{
			typedef ExtractValue type;

			typedef typename std::conditional<
				const_ == Const::Yes,
				const ItemT,
				ItemT>::type Value_;
			typedef typename std::conditional<
				const_ == Const::Yes,
				const Item,
				Item>::type Item_;

			Value_ &operator()(Item_ &i) const {
				return i.value();
			}
		};

		static Item mkSeparator()
		{
			Item item;
			item.setSelectable(false);
			item.setSeparator(true);
			return item;
		}
		
		std::shared_ptr<std::tuple<ItemT, Properties>> m_impl;
	};

	template <Const const_>
	struct ItemIterator
	{
		friend struct Menu<ItemT>;

		typedef std::random_access_iterator_tag iterator_category;
		typedef std::ptrdiff_t difference_type;
		typedef Item value_type;
		typedef typename std::conditional<
			const_ == Const::Yes,
			const Item &,
			Item &>::type reference;
		typedef typename std::conditional<
			const_ == Const::Yes,
			const Item *,
			Item *>::type pointer;

		ItemIterator() = default;

		ItemIterator(NcMenu *menu, enum NcMenuItemSource source, int64 pos)
			: m_menu(menu)
			, m_source(source)
			, m_pos(pos)
		{ }

		template <Const other_,
		          typename = std::enable_if_t<
		              const_ == Const::Yes && other_ == Const::No>>
		ItemIterator(const ItemIterator<other_> &rhs)
			: m_menu(rhs.m_menu)
			, m_source(rhs.m_source)
			, m_pos(rhs.m_pos)
		{ }

		reference operator*() const
		{
			return *static_cast<pointer>(
				nc_menu_item_at(m_menu, m_source, m_pos));
		}

		pointer operator->() const
		{
			return std::addressof(operator*());
		}

		reference operator[](difference_type n) const
		{
			return *(*this + n);
		}

		ItemIterator &operator++()
		{
			m_pos += 1;
			return *this;
		}

		ItemIterator operator++(int)
		{
			auto result = *this;
			++*this;
			return result;
		}

		ItemIterator &operator--()
		{
			m_pos -= 1;
			return *this;
		}

		ItemIterator operator--(int)
		{
			auto result = *this;
			--*this;
			return result;
		}

		ItemIterator &operator+=(difference_type n)
		{
			m_pos += n;
			return *this;
		}

		ItemIterator &operator-=(difference_type n)
		{
			m_pos -= n;
			return *this;
		}

		friend ItemIterator operator+(ItemIterator iterator, difference_type n)
		{
			iterator += n;
			return iterator;
		}

		friend ItemIterator operator+(difference_type n, ItemIterator iterator)
		{
			iterator += n;
			return iterator;
		}

		friend ItemIterator operator-(ItemIterator iterator, difference_type n)
		{
			iterator -= n;
			return iterator;
		}

		friend difference_type operator-(ItemIterator lhs, ItemIterator rhs)
		{
			assert(lhs.m_menu == rhs.m_menu);
			assert(lhs.m_source == rhs.m_source);
			return lhs.m_pos - rhs.m_pos;
		}

		friend bool operator==(ItemIterator lhs, ItemIterator rhs)
		{
			return lhs.m_menu == rhs.m_menu
				&& lhs.m_source == rhs.m_source
				&& lhs.m_pos == rhs.m_pos;
		}

		friend bool operator!=(ItemIterator lhs, ItemIterator rhs)
		{
			return !(lhs == rhs);
		}

		friend bool operator<(ItemIterator lhs, ItemIterator rhs)
		{
			assert(lhs.m_menu == rhs.m_menu);
			assert(lhs.m_source == rhs.m_source);
			return lhs.m_pos < rhs.m_pos;
		}

		friend bool operator>(ItemIterator lhs, ItemIterator rhs)
		{
			return rhs < lhs;
		}

		friend bool operator<=(ItemIterator lhs, ItemIterator rhs)
		{
			return !(rhs < lhs);
		}

		friend bool operator>=(ItemIterator lhs, ItemIterator rhs)
		{
			return !(lhs < rhs);
		}

		friend void iter_swap(ItemIterator lhs, ItemIterator rhs)
		{
			static_assert(const_ == Const::No);
			assert(lhs.m_menu == rhs.m_menu);
			assert(lhs.m_source == rhs.m_source);
			nc_menu_swap_item_slots(lhs.m_menu, lhs.m_source,
			                        lhs.m_pos, rhs.m_pos);
		}

	private:
		template <Const> friend struct ItemIterator;

		NcMenu *m_menu = nullptr;
		enum NcMenuItemSource m_source = NC_MENU_ITEMS_ALL;
		int64 m_pos = 0;
	};

	typedef ItemIterator<Const::No> Iterator;
	typedef ItemIterator<Const::Yes> ConstIterator;
	typedef std::reverse_iterator<Iterator> ReverseIterator;
	typedef std::reverse_iterator<ConstIterator> ConstReverseIterator;

	typedef Utility::TransformIterator<
		typename Item::template ExtractValue<Const::No>,
		Iterator> ValueIterator;
	typedef Utility::TransformIterator<
		typename Item::template ExtractValue<Const::Yes>,
		ConstIterator> ConstValueIterator;
	typedef std::reverse_iterator<ValueIterator> ReverseValueIterator;
	typedef std::reverse_iterator<ConstValueIterator> ConstReverseValueIterator;
	
	typedef Utility::TransformIterator<
		typename Item::template ExtractProperties<Const::No>,
		Iterator> PropertiesIterator;
	typedef Utility::TransformIterator<
		typename Item::template ExtractProperties<Const::Yes>,
		ConstIterator> ConstPropertiesIterator;

	// Expose standard iterator aliases for utility interoperability.
	typedef Iterator iterator;
	typedef ConstIterator const_iterator;

	/// Function helper prototype used to display each option on the screen.
	/// If not set by setItemDisplayer(), menu won't display anything.
	/// @see setItemDisplayer()
	typedef std::function<void(Menu<ItemT> &)> ItemDisplayer;

	typedef std::function<bool(const Item &)> FilterPredicate;

	Menu();
	
	Menu(size_t startx, size_t starty, size_t width, size_t height,
			const std::string &title, Color color, Border border);
	
	Menu(const Menu &rhs);
	Menu(Menu &&rhs);
	~Menu();
	Menu &operator=(Menu rhs);
	
	/// Sets helper function that is responsible for displaying items
	/// @param ptr function pointer that matches the ItemDisplayer prototype
	template <typename ItemDisplayerT>
	void setItemDisplayer(ItemDisplayerT &&displayer);
	
	/// Resizes the list to given size (adequate to std::vector::resize())
	/// @param size requested size
	void resizeList(size_t new_size);
	
	/// Adds a new option to list
	void addItem(ItemT item, Properties::Type properties = Properties::Selectable);
	
	/// Adds separator to list
	void addSeparator();
	
	/// Inserts a new option to the list at given position
	void insertItem(size_t pos, ItemT item, Properties::Type properties = Properties::Selectable);
	
	/// Inserts separator to list at given position
	/// @param pos initial position of inserted separator
	void insertSeparator(size_t pos);
	
	/// Moves the highlighted position to the given line of window
	/// @param y Y position of menu window to be highlighted
	/// @return true if the position is reachable, false otherwise
	bool Goto(size_t y);
	
	/// Checks if list is empty
	/// @return true if list is empty, false otherwise
	virtual bool empty() const override { return activeItemCount() == 0; }

	/// @return size of the list
	virtual size_t size() const override { return activeItemCount(); }

	/// @return currently highlighted position
	virtual size_t choice() const override;

	/// Highlights given position
	/// @param pos position to be highlighted
	virtual void highlight(size_t position) override;
	
	/// Refreshes the menu window
	/// @see Window::refresh()
	virtual void refresh() override;
	
	/// Scrolls by given amount of lines
	/// @param where indicated where exactly one wants to go
	/// @see Window::scroll()
	virtual void scroll(Scroll where) override;
	
	/// Cleares all options, used filters etc. It doesn't reset highlighted position though.
	/// @see reset()
	virtual void clear() override;
	
	/// Sets highlighted position to 0
	void reset();

	/// Apply filter predicate to items in the menu and show the ones for which it
	/// returned true.
	template <typename PredicateT>
	void applyFilter(PredicateT &&pred);

	/// Reapply previously applied filter.
	void reapplyFilter();

	/// Get current filter predicate.
	template <typename TargetT>
	const TargetT *filterPredicate() const;

	/// Clear results of applyFilter and show all items.
	void clearFilter();

	/// @return true if menu is filtered.
	bool isFiltered() const { return nc_menu_is_filtered(const_cast<NcMenu *>(&m_menu)); }

	/// Show all items.
	void showAllItems() { nc_menu_show_all_items(&m_menu); }

	/// Show filtered items.
	void showFilteredItems() { nc_menu_show_filtered_items(&m_menu); }

	/// Sets prefix, that is put before each selected item to indicate its selection
	/// Note that the passed variable is not deleted along with menu object.
	/// @param b pointer to buffer that contains the prefix
	void setSelectedPrefix(const Buffer &b)
		{
			m_selected_prefix = b;
			syncMenuPrefixes();
		}
	
	/// Sets suffix, that is put after each selected item to indicate its selection
	/// Note that the passed variable is not deleted along with menu object.
	/// @param b pointer to buffer that contains the suffix
	void setSelectedSuffix(const Buffer &b)
		{
			m_selected_suffix = b;
			syncMenuPrefixes();
		}

	void setHighlightPrefix(const Buffer &b)
		{
			m_highlight_prefix = b;
			syncMenuPrefixes();
		}
	void setHighlightSuffix(const Buffer &b)
		{
			m_highlight_suffix = b;
			syncMenuPrefixes();
		}

	const Buffer &highlightPrefix() const { return m_highlight_prefix; }
	const Buffer &highlightSuffix() const { return m_highlight_suffix; }

	/// @return state of highlighting
	bool isHighlighted() { return nc_menu_highlight_enabled(&m_menu); }

	NcMenu *nativeMenu() { return &m_menu; }
	const NcMenu *nativeMenu() const { return &m_menu; }
	
	/// Turns on/off highlighting
	/// @param state state of hihglighting
	void setHighlighting(bool state) { nc_menu_set_highlighting(&m_menu, state); }
	
	/// Turns on/off cyclic scrolling
	/// @param state state of cyclic scrolling
	void cyclicScrolling(bool state) { nc_menu_set_cyclic_scrolling(&m_menu, state); }
	
	/// Turns on/off centered cursor
	/// @param state state of centered cursor
	void centeredCursor(bool state) { nc_menu_set_centered_cursor(&m_menu, state); }
	
	/// @return currently drawn item. The result is defined only within
	/// drawing function that is called by refresh()
	/// @see refresh()
	ConstIterator drawn() const { return begin() + nc_menu_drawn_position(const_cast<NcMenu *>(&m_menu)); }
	
	/// @param pos requested position
	/// @return reference to item at given position
	/// @throw std::out_of_range if given position is out of range
	Menu<ItemT>::Item &at(size_t pos) { return activeItemAt(pos); }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	/// @throw std::out_of_range if given position is out of range
	const Menu<ItemT>::Item &at(size_t pos) const { return activeItemAt(pos); }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	const Menu<ItemT>::Item &operator[](size_t pos) const { return activeItemAt(pos); }
	
	/// @param pos requested position
	/// @return const reference to item at given position
	Menu<ItemT>::Item &operator[](size_t pos) { return activeItemAt(pos); }
	
	Iterator current() {
		return Iterator(&m_menu, activeItemSource(), nc_menu_highlight(&m_menu));
	}
	ConstIterator current() const {
		return ConstIterator(
			const_cast<NcMenu *>(&m_menu),
			activeItemSource(),
			nc_menu_highlight(const_cast<NcMenu *>(&m_menu)));
	}
	ReverseIterator rcurrent() {
		if (empty())
			return rend();
		else
			return ReverseIterator(++current());
	}
	ConstReverseIterator rcurrent() const {
		if (empty())
			return rend();
		else
			return ConstReverseIterator(++current());
	}

	ValueIterator currentV() {
		return ValueIterator(current());
	}
	ConstValueIterator currentV() const {
		return ConstValueIterator(current());
	}
	ReverseValueIterator rcurrentV() {
		if (empty())
			return rendV();
		else
			return ReverseValueIterator(++currentV());
	}
	ConstReverseValueIterator rcurrentV() const {
		if (empty())
			return rendV();
		else
			return ConstReverseValueIterator(++currentV());
	}
	
	Iterator begin() { return Iterator(&m_menu, activeItemSource(), 0); }
	ConstIterator begin() const {
		return ConstIterator(const_cast<NcMenu *>(&m_menu), activeItemSource(), 0);
	}
	Iterator end() {
		return Iterator(&m_menu, activeItemSource(), static_cast<int64>(size()));
	}
	ConstIterator end() const {
		return ConstIterator(
			const_cast<NcMenu *>(&m_menu),
			activeItemSource(),
			static_cast<int64>(size()));
	}
	
	ReverseIterator rbegin() { return ReverseIterator(end()); }
	ConstReverseIterator rbegin() const { return ConstReverseIterator(end()); }
	ReverseIterator rend() { return ReverseIterator(begin()); }
	ConstReverseIterator rend() const { return ConstReverseIterator(begin()); }
	
	ValueIterator beginV() { return ValueIterator(begin()); }
	ConstValueIterator beginV() const { return ConstValueIterator(begin()); }
	ValueIterator endV() { return ValueIterator(end()); }
	ConstValueIterator endV() const { return ConstValueIterator(end()); }
	
	ReverseValueIterator rbeginV() { return ReverseValueIterator(endV()); }
	ConstReverseIterator rbeginV() const { return ConstReverseValueIterator(endV()); }
	ReverseValueIterator rendV() { return ReverseValueIterator(beginV()); }
	ConstReverseValueIterator rendV() const { return ConstReverseValueIterator(beginV()); }
	
	virtual List::Iterator currentP() override {
		return List::Iterator(PropertiesIterator(current()));
	}
	virtual List::ConstIterator currentP() const override {
		return List::ConstIterator(ConstPropertiesIterator(current()));
	}
	virtual List::Iterator beginP() override {
		return List::Iterator(PropertiesIterator(begin()));
	}
	virtual List::ConstIterator beginP() const override {
		return List::ConstIterator(ConstPropertiesIterator(begin()));
	}
	virtual List::Iterator endP() override {
		return List::Iterator(PropertiesIterator(end()));
	}
	virtual List::ConstIterator endP() const override {
		return List::ConstIterator(ConstPropertiesIterator(end()));
	}

private:
	static enum NcScroll toNcScroll(Scroll scroll)
	{
		switch (scroll)
		{
		case Scroll::Up:
			return NC_SCROLL_UP;
		case Scroll::Down:
			return NC_SCROLL_DOWN;
		case Scroll::PageUp:
			return NC_SCROLL_PAGE_UP;
		case Scroll::PageDown:
			return NC_SCROLL_PAGE_DOWN;
		case Scroll::Home:
			return NC_SCROLL_HOME;
		case Scroll::End:
			return NC_SCROLL_END;
		}
		return NC_SCROLL_HOME;
	}

	static bool isHighlightableCallback(int64 pos, void *user)
	{
		return static_cast<Menu<ItemT> *>(user)->isHighlightable(
			static_cast<size_t>(pos));
	}

		static NcMenuItemCallbacks itemCallbacks()
		{
			NcMenuItemCallbacks callbacks;

			callbacks.item_size = sizeof(Item);
			callbacks.construct = &Menu<ItemT>::constructItem;
			callbacks.copy = &Menu<ItemT>::copyItem;
			callbacks.destroy = &Menu<ItemT>::destroyItem;
			callbacks.user = nullptr;
			return callbacks;
		}

		NcMenuDisplayCallbacks displayCallbacks()
		{
			NcMenuDisplayCallbacks callbacks;

			callbacks.draw = &Menu<ItemT>::drawItemCallback;
			callbacks.filter = &Menu<ItemT>::filterItemCallback;
			callbacks.is_separator = &Menu<ItemT>::isSeparatorCallback;
			callbacks.is_selected = &Menu<ItemT>::isSelectedCallback;
			callbacks.is_inactive = &Menu<ItemT>::isInactiveCallback;
			callbacks.user = this;
			return callbacks;
		}

		NcMenuActionCallbacks actionCallbacks()
		{
			NcMenuActionCallbacks callbacks;

			callbacks.activate = nullptr;
			callbacks.set_selected = &Menu<ItemT>::setSelectedCallback;
			callbacks.user = this;
			return callbacks;
		}

		static void constructItem(void *dest, void *)
		{
			new (dest) Item();
		}

		static void copyItem(void *dest, void *source, void *)
		{
			new (dest) Item(*static_cast<Item *>(source));
		}

		static void destroyItem(void *item, void *)
		{
			static_cast<Item *>(item)->~Item();
		}

		static void drawItemCallback(NcMenu *, NcWindow *, void *item,
		                             int64, void *user)
		{
			(void)item;
			auto menu = static_cast<Menu<ItemT> *>(user);
			if (menu->m_item_displayer)
				menu->m_item_displayer(*menu);
		}

		static bool filterItemCallback(NcMenu *, void *item, void *user)
		{
			auto menu = static_cast<Menu<ItemT> *>(user);
			if (!menu->m_filter_predicate)
				return false;
			return menu->m_filter_predicate(*static_cast<Item *>(item));
		}

		static bool isSeparatorCallback(void *item, void *)
		{
			return static_cast<Item *>(item)->isSeparator();
		}

		static bool isSelectedCallback(void *item, void *)
		{
			return static_cast<Item *>(item)->isSelected();
		}

		static bool isInactiveCallback(void *item, void *)
		{
			return static_cast<Item *>(item)->isInactive();
		}

		static void setSelectedCallback(void *item, bool selected, void *)
		{
			static_cast<Item *>(item)->setSelected(selected);
		}

		void initItemStorage()
		{
			nc_menu_set_item_callbacks(&m_menu, itemCallbacks());
			syncDisplayCallbacks();
			syncActionCallbacks();
		}

		void syncDisplayCallbacks()
		{
			nc_menu_set_display_callbacks(&m_menu, displayCallbacks());
		}

		void syncActionCallbacks()
		{
			nc_menu_set_action_callbacks(&m_menu, actionCallbacks());
		}

		void syncMenuPrefixes()
		{
			nc_menu_set_highlight_prefix(&m_menu, m_highlight_prefix.cBuffer());
			nc_menu_set_highlight_suffix(&m_menu, m_highlight_suffix.cBuffer());
			nc_menu_set_selected_prefix(&m_menu, m_selected_prefix.cBuffer());
			nc_menu_set_selected_suffix(&m_menu, m_selected_suffix.cBuffer());
		}

		void syncMenuSize()
		{
			nc_menu_sync_item_count(&m_menu);
		}

		size_t activeItemCount() const
	{
		return static_cast<size_t>(
			nc_menu_item_count(const_cast<NcMenu *>(&m_menu)));
	}

	enum NcMenuItemSource activeItemSource() const
	{
		if (nc_menu_is_filtered(const_cast<NcMenu *>(&m_menu)))
			return NC_MENU_ITEMS_FILTERED;
		else
			return NC_MENU_ITEMS_ALL;
	}

	Item &activeItemAt(size_t pos)
	{
		if (pos >= size())
			throw std::out_of_range("NC::Menu::at");
		return *static_cast<Item *>(
			nc_menu_active_item_at(&m_menu, static_cast<int64>(pos)));
	}

	const Item &activeItemAt(size_t pos) const
	{
		if (pos >= size())
			throw std::out_of_range("NC::Menu::at");
		return *static_cast<const Item *>(
			nc_menu_active_item_at(
				const_cast<NcMenu *>(&m_menu),
				static_cast<int64>(pos)));
	}

	Item &allItemAt(size_t pos)
	{
		return *static_cast<Item *>(
			nc_menu_item_at(&m_menu, NC_MENU_ITEMS_ALL,
			                static_cast<int64>(pos)));
	}

	bool isHighlightable(size_t pos)
	{
		return !activeItemAt(pos).isSeparator()
			&& !activeItemAt(pos).isInactive();
	}

	ItemDisplayer m_item_displayer;
	FilterPredicate m_filter_predicate;

	NcMenu m_menu;

	Buffer m_highlight_prefix;
	Buffer m_highlight_suffix;

	Buffer m_selected_prefix;
	Buffer m_selected_suffix;
};

}

#endif // NCMPCPP_MENU_H
