#ifndef NCMPCPP_UTILITY_COMPARATORS_H
#define NCMPCPP_UTILITY_COMPARATORS_H

#include <cstring>
#include <locale>
#include <stdexcept>
#include <string>

#include "c/ncm_comparators.h"
#include "curses/menu.h"
#include "format_impl.h"
#include "mpdpp.h"
#include "runnable_item.h"
#include "settings_legacy.h"

class LocaleStringComparison
{
	std::locale m_locale;
	bool m_ignore_the;

public:
	LocaleStringComparison(const std::locale &loc, bool ignore_the)
	: m_locale(loc), m_ignore_the(ignore_the) { }

	int operator()(const char *a, const char *b) const {
		return compare(a, strlen(a), b, strlen(b));
	}
	int operator()(const std::string &a, const std::string &b) const {
		return compare(a.c_str(), a.length(), b.c_str(), b.length());
	}

	int compare(const char *a, size_t a_len,
	            const char *b, size_t b_len) const {
		(void)m_locale;
		return ncm_compare_locale_strings(
			const_cast<char *>(a), static_cast<int32>(a_len),
			const_cast<char *>(b), static_cast<int32>(b_len),
			m_ignore_the);
	}
};

class LocaleBasedSorting
{
	LocaleStringComparison m_cmp;

public:
	LocaleBasedSorting(const std::locale &loc, bool ignore_the) : m_cmp(loc, ignore_the) { }

	bool operator()(const std::string &a, const std::string &b) const {
		return m_cmp(a, b) < 0;
	}

	bool operator()(const MPD::Playlist &a, const MPD::Playlist &b) const {
		return m_cmp(a.path(), b.path()) < 0;
	}

	bool operator()(const MPD::Song &a, const MPD::Song &b) const {
		return m_cmp(a.getName(), b.getName()) < 0;
	}

	template <typename A, typename B>
	bool operator()(const std::pair<A, B> &a, const std::pair<A, B> &b) const {
		return m_cmp(a.first, b.first) < 0;
	}

	template <typename ItemT, typename FunT>
	bool operator()(const RunnableItem<ItemT, FunT> &a,
	                const RunnableItem<ItemT, FunT> &b) const {
		return m_cmp(a.item(), b.item()) < 0;
	}
};

class LocaleBasedItemSorting
{
	LocaleBasedSorting m_cmp;
	SortMode m_sort_mode;

public:
	LocaleBasedItemSorting(const std::locale &loc, bool ignore_the, SortMode mode)
	: m_cmp(loc, ignore_the), m_sort_mode(mode) { }

	bool operator()(const MPD::Item &a, const MPD::Item &b) const {
		bool result = false;
		if (a.type() == b.type())
		{
			switch (m_sort_mode)
			{
				case NCM_SORT_MODE_TYPE:
					result = a.type() > b.type();
					break;
				case NCM_SORT_MODE_NAME:
					switch (a.type())
					{
						case MPD::Item::Type::Directory:
							result = m_cmp(a.directory().path(), b.directory().path());
							break;
						case MPD::Item::Type::Playlist:
							result = m_cmp(a.playlist().path(), b.playlist().path());
							break;
						case MPD::Item::Type::Song:
							result = m_cmp(a.song(), b.song());
							break;
					}
					break;
				case NCM_SORT_MODE_CUSTOM_FORMAT:
					switch (a.type())
					{
						case MPD::Item::Type::Directory:
							result = m_cmp(a.directory().path(), b.directory().path());
							break;
						case MPD::Item::Type::Playlist:
							result = m_cmp(a.playlist().path(), b.playlist().path());
							break;
						case MPD::Item::Type::Song:
							result = m_cmp(
								Format::stringify<char>(Config.browser_sort_format, &a.song()),
								Format::stringify<char>(Config.browser_sort_format, &b.song()));
							break;
					}
					break;
				case NCM_SORT_MODE_MODIFICATION_TIME:
					switch (a.type())
					{
						case MPD::Item::Type::Directory:
							result = a.directory().lastModified() > b.directory().lastModified();
							break;
						case MPD::Item::Type::Playlist:
							result = a.playlist().lastModified() > b.playlist().lastModified();
							break;
						case MPD::Item::Type::Song:
							result = a.song().getMTime() > b.song().getMTime();
							break;
					}
					break;
				case NCM_SORT_MODE_NONE:
					throw std::logic_error("can't sort with None sorting mode");
			}
		}
		else
			result = a.type() < b.type();
		return result;
	}

	bool operator()(const NC::Menu<MPD::Item>::Item &a,
	                const NC::Menu<MPD::Item>::Item &b) const
	{
		return (*this)(a.value(), b.value());
	}
};

#endif // NCMPCPP_UTILITY_COMPARATORS_H
