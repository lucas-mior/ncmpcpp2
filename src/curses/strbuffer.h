#ifndef NCMPCPP_STRBUFFER_H
#define NCMPCPP_STRBUFFER_H

#include <cassert>
#include <map>
#include <sstream>
#include <utility>
#include <variant>

#include "curses/formatted_color.h"
#include "curses/nc_buffer.h"
#include "curses/window.h"

namespace NC {

/// Buffer template class that stores text
/// along with its properties (colors/formatting).
template <typename CharT> class BasicBuffer
{
	struct Property
	{
		template <typename ArgT>
		Property(ArgT &&arg, size_t id_)
		: m_impl(std::forward<ArgT>(arg)), m_id(id_) { }
		
		size_t id() const { return m_id; }

		bool operator==(const Property &rhs) const
		{
			return m_id == rhs.m_id && m_impl == rhs.m_impl;
		}

		template <typename OutputStreamT>
		friend OutputStreamT &operator<<(OutputStreamT &os, const Property &p)
		{
			std::visit([&os](const auto &v) { os << v; }, p.m_impl);
			return os;
		}
		
	private:
		std::variant<Color,
		             enum NcFormat,
		             FormattedColor,
		             FormattedColor::End<StorageKind::Value>
		             > m_impl;
		size_t m_id;
	};

public:
	typedef std::basic_string<CharT> StringType;
	typedef std::multimap<size_t, Property> Properties;

	BasicBuffer()
		: m_string_cache_valid(false), m_properties_cache_valid(false)
	{
		nc_buffer_init(&m_buffer);
	}

	BasicBuffer(const BasicBuffer &rhs)
		: m_string_cache_valid(false), m_properties_cache_valid(false)
	{
		nc_buffer_copy(&m_buffer, const_cast<NcBuffer *>(rhs.cBuffer()));
	}

	BasicBuffer(BasicBuffer &&rhs) noexcept
		: m_string_cache(std::move(rhs.m_string_cache)),
		  m_properties_cache(std::move(rhs.m_properties_cache)),
		  m_string_cache_valid(rhs.m_string_cache_valid),
		  m_properties_cache_valid(rhs.m_properties_cache_valid)
	{
		nc_buffer_move(&m_buffer, rhs.cBuffer());
		rhs.m_string_cache_valid = false;
		rhs.m_properties_cache_valid = false;
	}

	BasicBuffer &operator=(const BasicBuffer &rhs)
	{
		if (this != &rhs)
		{
			nc_buffer_destroy(&m_buffer);
			nc_buffer_copy(&m_buffer, const_cast<NcBuffer *>(rhs.cBuffer()));
			invalidateCaches();
		}
		return *this;
	}

	BasicBuffer &operator=(BasicBuffer &&rhs) noexcept
	{
		if (this != &rhs)
		{
			nc_buffer_destroy(&m_buffer);
			nc_buffer_move(&m_buffer, rhs.cBuffer());
			m_string_cache = std::move(rhs.m_string_cache);
			m_properties_cache = std::move(rhs.m_properties_cache);
			m_string_cache_valid = rhs.m_string_cache_valid;
			m_properties_cache_valid = rhs.m_properties_cache_valid;
			rhs.m_string_cache_valid = false;
			rhs.m_properties_cache_valid = false;
		}
		return *this;
	}

	~BasicBuffer()
	{
		nc_buffer_destroy(&m_buffer);
	}
	
	const StringType &str() const
	{
		refreshStringCache();
		return m_string_cache;
	}

	const Properties &properties() const
	{
		refreshPropertiesCache();
		return m_properties_cache;
	}

	NcBuffer *cBuffer()
	{
		return &m_buffer;
	}

	const NcBuffer *cBuffer() const
	{
		return &m_buffer;
	}
	
	void addProperty(size_t position, const Color &color, size_t id = -1)
	{
		assert(position <= static_cast<size_t>(nc_buffer_len(&m_buffer)));
		nc_buffer_add_color(&m_buffer,
		                    static_cast<int32>(position),
		                    toNcColor(color),
		                    static_cast<uint64>(id));
		invalidatePropertiesCache();
	}

	void addProperty(size_t position, enum NcFormat format, size_t id = -1)
	{
		assert(position <= static_cast<size_t>(nc_buffer_len(&m_buffer)));
		nc_buffer_add_format(&m_buffer,
		                     static_cast<int32>(position),
		                     toNcFormat(format),
		                     static_cast<uint64>(id));
		invalidatePropertiesCache();
	}

	void addProperty(size_t position,
	                 const FormattedColor &formatted_color,
	                 size_t id = -1)
	{
		assert(position <= static_cast<size_t>(nc_buffer_len(&m_buffer)));
		nc_buffer_add_formatted_color(
			&m_buffer,
			static_cast<int32>(position),
			const_cast<NcFormattedColor *>(formatted_color.cFormattedColor()),
			static_cast<uint64>(id));
		invalidatePropertiesCache();
	}

	template <StorageKind storage>
	void addProperty(size_t position,
	                 const FormattedColor::End<storage> &formatted_color_end,
	                 size_t id = -1)
	{
		assert(position <= static_cast<size_t>(nc_buffer_len(&m_buffer)));
		nc_buffer_add_formatted_color_end(
			&m_buffer,
			static_cast<int32>(position),
			const_cast<NcFormattedColor *>(
				formatted_color_end.base().cFormattedColor()),
			static_cast<uint64>(id));
		invalidatePropertiesCache();
	}

	void removeProperties(size_t id = -1)
	{
		nc_buffer_remove_properties(&m_buffer, static_cast<uint64>(id));
		invalidatePropertiesCache();
	}

	bool empty() const
	{
		return nc_buffer_empty(const_cast<NcBuffer *>(&m_buffer));
	}

	void clear()
	{
		nc_buffer_clear(&m_buffer);
		invalidateCaches();
	}
	
	BasicBuffer<CharT> &operator<<(int n)
	{
		nc_buffer_append_int32(&m_buffer, static_cast<int32>(n));
		invalidateStringCache();
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(long int n)
	{
		nc_buffer_append_int64(&m_buffer, static_cast<int64>(n));
		invalidateStringCache();
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(unsigned int n)
	{
		nc_buffer_append_uint32(&m_buffer, static_cast<uint32>(n));
		invalidateStringCache();
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(unsigned long int n)
	{
		nc_buffer_append_uint64(&m_buffer, static_cast<uint64>(n));
		invalidateStringCache();
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(CharT c)
	{
		nc_buffer_append_char(&m_buffer, static_cast<char>(c));
		invalidateStringCache();
		return *this;
	}

	BasicBuffer<CharT> &operator<<(const CharT *s)
	{
		nc_buffer_append_cstring(&m_buffer, const_cast<char *>(s));
		invalidateStringCache();
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(const StringType &s)
	{
		nc_buffer_append_data(
			&m_buffer,
			const_cast<char *>(reinterpret_cast<const char *>(s.data())),
			static_cast<int32>(s.size()));
		invalidateStringCache();
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(const Color &color)
	{
		addProperty(static_cast<size_t>(nc_buffer_len(&m_buffer)), color);
		return *this;
	}
	
	BasicBuffer<CharT> &operator<<(enum NcFormat format)
	{
		addProperty(static_cast<size_t>(nc_buffer_len(&m_buffer)), format);
		return *this;
	}

	// static variadic initializer. used instead of a proper constructor because
	// it's too polymorphic and would end up invoked as a copy/move constructor.
	template <typename... Args>
	static BasicBuffer init(Args&&... args)
	{
		BasicBuffer result;
		result.construct(std::forward<Args>(args)...);
		return result;
	}

private:
	void construct() { }
	template <typename ArgT, typename... Args>
	void construct(ArgT &&arg, Args&&... args)
	{
		*this << std::forward<ArgT>(arg);
		construct(std::forward<Args>(args)...);
	}

	void invalidateStringCache()
	{
		m_string_cache_valid = false;
	}

	void invalidatePropertiesCache()
	{
		m_properties_cache_valid = false;
	}

	void invalidateCaches()
	{
		invalidateStringCache();
		invalidatePropertiesCache();
	}

	void refreshStringCache() const
	{
		if (m_string_cache_valid)
			return;
		m_string_cache.assign(nc_buffer_data(const_cast<NcBuffer *>(&m_buffer)),
		                      static_cast<size_t>(
			                      nc_buffer_len(const_cast<NcBuffer *>(&m_buffer))));
		m_string_cache_valid = true;
	}

	void refreshPropertiesCache() const
	{
		if (m_properties_cache_valid)
			return;

		m_properties_cache.clear();
		auto properties = nc_buffer_properties(const_cast<NcBuffer *>(&m_buffer));
		auto count = nc_buffer_property_count(const_cast<NcBuffer *>(&m_buffer));
		for (int32 i = 0; i < count; i += 1)
		{
			auto &property = properties[i];
			switch (property.type)
			{
			case NC_BUFFER_PROPERTY_COLOR:
				m_properties_cache.emplace(
					property.position,
					Property(fromNcColor(property.value.color), property.id));
				break;
			case NC_BUFFER_PROPERTY_FORMAT:
				m_properties_cache.emplace(
					property.position,
					Property(fromNcFormat(property.value.format), property.id));
				break;
			case NC_BUFFER_PROPERTY_FORMATTED_COLOR:
				m_properties_cache.emplace(
					property.position,
					Property(
						FormattedColor(&property.value.formatted_color),
						property.id));
				break;
			case NC_BUFFER_PROPERTY_FORMATTED_COLOR_END:
				m_properties_cache.emplace(
					property.position,
					Property(
						FormattedColor::End<StorageKind::Value>(
							FormattedColor(&property.value.formatted_color)),
						property.id));
				break;
			}
		}
		m_properties_cache_valid = true;
	}

	NcBuffer m_buffer;
	mutable StringType m_string_cache;
	mutable Properties m_properties_cache;
	mutable bool m_string_cache_valid;
	mutable bool m_properties_cache_valid;
};

typedef BasicBuffer<char> Buffer;

template <typename CharT>
bool operator==(const BasicBuffer<CharT> &lhs, const BasicBuffer<CharT> &rhs)
{
	return nc_buffer_equal(const_cast<NcBuffer *>(lhs.cBuffer()),
	                       const_cast<NcBuffer *>(rhs.cBuffer()));
}


template <typename OutputStreamT, typename CharT>
OutputStreamT &operator<<(OutputStreamT &os, const BasicBuffer<CharT> &buffer)
{
	if (buffer.properties().empty())
		os << buffer.str();
	else
	{
		auto &s = buffer.str();
		auto &ps = buffer.properties();
		auto p = ps.begin();
		for (size_t i = 0;; ++i)
		{
			for (; p != ps.end() && p->first == i; ++p)
				os << p->second;
			if (i < s.size())
				os << s[i];
			else
				break;
		}
	}
	return os;
}

}

#endif // NCMPCPP_STRBUFFER_H
