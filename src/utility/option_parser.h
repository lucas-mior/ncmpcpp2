#ifndef NCMPCPP_UTILITY_OPTION_PARSER_H
#define NCMPCPP_UTILITY_OPTION_PARSER_H

#include <cassert>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "c/ncm_enums.h"
#include "c/ncm_option_parser.h"

[[noreturn]] inline void invalid_value(const std::string &v)
{
	throw std::runtime_error("invalid value: " + v);
}

inline std::string trim_copy(std::string s)
{
	auto first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	auto last = s.find_last_not_of(" \t\r\n");
	return s.substr(first, last-first+1);
}

template <typename DestT>
DestT parse_value(const std::string &v)
{
	if constexpr (std::is_same_v<DestT, std::string>)
		return v;
	else
	{
		std::istringstream is(v);
		DestT result;
		if (!(is >> result))
			invalid_value(v);
		is >> std::ws;
		if (!is.eof())
			invalid_value(v);
		return result;
	}
}

inline std::vector<std::string> split_escaped_list(const std::string &v,
                                                   const std::string &escape,
                                                   const std::string &sep,
                                                   const std::string &quote)
{
	std::vector<std::string> result;
	std::string current;
	bool quoted = false;
	char escape_char = escape.empty() ? '\0' : escape.front();
	char quote_char = quote.empty() ? '\0' : quote.front();
	for (size_t i = 0; i < v.size(); ++i)
	{
		char c = v[i];
		if (escape_char != '\0' && c == escape_char && i+1 < v.size())
		{
			current += v[++i];
			continue;
		}
		if (quote_char != '\0' && c == quote_char)
		{
			quoted = !quoted;
			continue;
		}
		if (!quoted && sep.find(c) != std::string::npos)
		{
			result.push_back(trim_copy(current));
			current.clear();
			continue;
		}
		current += c;
	}
	result.push_back(trim_copy(current));
	return result;
}

template <typename ValueT, typename ConvertT>
std::vector<ValueT> list_of(const std::string &v,
                            ConvertT convert,
                            const typename std::vector<ValueT>::size_type length,
                            const std::string &escape,
                            const std::string &sep,
                            const std::string &quote)
{
	std::vector<ValueT> result;
	auto elems = split_escaped_list(v, escape, sep, quote);
	for (auto &value : elems)
		result.push_back(convert(value));
	if (result.empty())
		throw std::runtime_error("empty list");
	if (length > 0 && result.size() != length)
		throw std::runtime_error("invalid list length");
	return result;
}

template <typename ValueT, typename ConvertT>
std::vector<ValueT> list_of(const std::string &v, ConvertT convert)
{
	return list_of<ValueT>(v, convert, 0, "\\", ",", "\"");
}

template <typename ValueT>
std::vector<ValueT> list_of(const std::string &v)
{
	return list_of<ValueT>(v, parse_value<ValueT>);
}

inline bool yes_no(const std::string &v)
{
    bool result;

    if (!ncm_option_parser_yes_no(const_cast<char *>(v.data()),
                                  static_cast<int32>(v.size()),
                                  &result))
        invalid_value(v);
    return result;
}

inline std::vector<size_t> parse_ratio(
    const std::string &v, const std::vector<size_t>::size_type length)
{
    std::vector<size_t> ret =
        list_of<size_t>(v, parse_value<size_t>, length, "", ":", "");

    size_t total = 0;
    for (auto i : ret)
        total += i;
    if (total == 0)
        invalid_value(v);

    return ret;
}

template <>
inline SearchDirection parse_value<SearchDirection>(const std::string &v)
{
    SearchDirection result;
    if (!ncm_search_direction_parse(const_cast<char *>(v.data()),
                                    static_cast<int32>(v.length()),
                                    &result))
        invalid_value(v);
    return result;
}

template <>
inline SpaceAddMode parse_value<SpaceAddMode>(const std::string &v)
{
    SpaceAddMode result;
    if (!ncm_space_add_mode_parse(const_cast<char *>(v.data()),
                                  static_cast<int32>(v.length()),
                                  &result))
        invalid_value(v);
    return result;
}

template <>
inline SortMode parse_value<SortMode>(const std::string &v)
{
    SortMode result;
    if (!ncm_sort_mode_parse(const_cast<char *>(v.data()),
                             static_cast<int32>(v.length()),
                             &result))
        invalid_value(v);
    return result;
}

template <>
inline DisplayMode parse_value<DisplayMode>(const std::string &v)
{
    DisplayMode result;
    if (!ncm_display_mode_parse(const_cast<char *>(v.data()),
                                static_cast<int32>(v.length()),
                                &result))
        invalid_value(v);
    return result;
}

template <>
inline Design parse_value<Design>(const std::string &v)
{
    Design result;
    if (!ncm_design_parse(const_cast<char *>(v.data()),
                          static_cast<int32>(v.length()),
                          &result))
        invalid_value(v);
    return result;
}

template <>
inline VisualizerType parse_value<VisualizerType>(const std::string &v)
{
    VisualizerType result;
    if (!ncm_visualizer_type_parse(const_cast<char *>(v.data()),
                                   static_cast<int32>(v.length()),
                                   &result))
        invalid_value(v);
    return result;
}

////////////////////////////////////////////////////////////////////////////////

class option_parser
{
	template <typename DestT>
	struct worker
	{
		template <typename MapT>
		worker(DestT *dest, MapT &&map)
			: m_dest(dest), m_map(std::forward<MapT>(map)), m_dest_set(false)
		{ }

		void operator()(std::string value)
		{
			if (m_dest_set)
				throw std::runtime_error("option already set");
			assign<DestT, void>::apply(m_dest, m_map, value);
			m_dest_set = true;
		}

	private:
		template <typename ValueT, typename VoidT>
		struct assign {
			static void apply(ValueT *dest,
			                  std::function<DestT(std::string)> &map,
			                  std::string &value)	{
				*dest = map(std::move(value));
			}
		};
		template <typename VoidT>
		struct assign<void, VoidT> {
			static void apply(void *,
			                  std::function<void(std::string)> &map,
			                  std::string &value) {
				map(std::move(value));
			}
		};

		DestT *m_dest;
		std::function<DestT(std::string)> m_map;
		bool m_dest_set;
	};

	struct parser {
		template <typename StringT, typename SetterT>
		parser(StringT &&default_, SetterT &&setter_)
			: m_used(false)
			, m_default_value(std::forward<StringT>(default_))
			, m_worker(std::forward<SetterT>(setter_))
		{ }

		bool used() const
		{
			return m_used;
		}

		void parse(std::string v)
		{
			m_worker(std::move(v));
			m_used = true;
		}

		void parse_default() const
		{
			assert(!m_used);
			m_worker(m_default_value);
		}

	private:
		bool m_used;
		std::string m_default_value;
		std::function<void(std::string)> m_worker;
	};

	std::unordered_map<std::string, parser> m_parsers;

public:
	template <typename DestT, typename MapT>
	void add(std::string option, DestT *dest, std::string default_, MapT &&map)
	{
		assert(m_parsers.count(option) == 0);
		m_parsers.emplace(
			std::move(option),
			parser(
				std::move(default_),
				worker<DestT>(dest, std::forward<MapT>(map))));
	}

	template <typename DestT>
	void add(std::string option, DestT *dest, std::string default_)
	{
		add(std::move(option), dest, std::move(default_), parse_value<DestT>);
	}

	bool run(std::istream &is, bool warn_on_errors);
	bool initialize_undefined(bool warn_on_errors);
};

inline bool option_parser::run(std::istream &is, bool ignore_errors)
{
	std::string line;
	while (std::getline(is, line))
	{
		NcmOptionLine parsed;
		if (ncm_option_parser_parse_line(
			line.data(), static_cast<int32>(line.size()), &parsed))
		{
			std::string option(parsed.option,
			                   static_cast<size_t>(parsed.option_len));
			auto it = m_parsers.find(option);
			if (it != m_parsers.end())
			{
				try
				{
					it->second.parse(std::string(
						parsed.value,
						static_cast<size_t>(parsed.value_len)));
				}
				catch (std::exception &e)
				{
					std::cerr << "Error while processing option \""
					          << option << "\": " << e.what() << "\n";
					if (!ignore_errors)
						return false;
				}
			}
			else
			{
				std::cerr << "Unknown option: " << option << "\n";
				if (!ignore_errors)
					return false;
			}
		}
	}
	return true;
}

inline bool option_parser::initialize_undefined(bool ignore_errors)
{
	for (auto &pp : m_parsers)
	{
		auto &p = pp.second;
		if (!p.used())
		{
			try
			{
				p.parse_default();
			}
			catch (std::exception &e)
			{
				std::cerr << "Error while initializing option \""
				          << pp.first << "\": " << e.what() << "\n";
				if (ignore_errors)
					return false;
			}
		}
	}
	return true;
}

#endif // NCMPCPP_UTILITY_OPTION_PARSER_H
