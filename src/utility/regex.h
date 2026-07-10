#ifndef NCMPCPP_UTILITY_REGEX_H
#define NCMPCPP_UTILITY_REGEX_H

#include <cstddef>
#include <iostream>
#include <regex>
#include <string>
#include <utility>

#include "utility/functional.h"

namespace {

inline std::string escapeRegexLiteral(const std::string &s)
{
	std::string result;
	result.reserve(s.size() * 2);
	for (char c : s)
	{
		switch (c)
		{
			case '\\':
			case '^':
			case '$':
			case '.':
			case '|':
			case '?':
			case '*':
			case '+':
			case '(':
			case ')':
			case '[':
			case ']':
			case '{':
			case '}':
				result += '\\';
				break;
		}
		result += c;
	}
	return result;
}

}

namespace Regex {

class Flags
{
public:
	using Syntax = std::regex_constants::syntax_option_type;

	Flags()
		: m_syntax(std::regex_constants::extended)
		, m_literal(false)
	{ }

	Flags(int)
		: Flags()
	{ }

	Flags(Syntax syntax, bool literal = false)
		: m_syntax(syntax)
		, m_literal(literal)
	{ }

	Syntax syntax() const
	{
		return m_syntax;
	}

	bool literal() const
	{
		return m_literal;
	}

private:
	Syntax m_syntax;
	bool m_literal;
};

typedef std::regex_error Error;

class Regex
{
public:
	Regex()
		: m_empty(true)
	{ }

	Regex(const std::string &s, Flags flags)
		: m_rx(flags.literal() ? escapeRegexLiteral(s) : s, flags.syntax())
		, m_empty(false)
	{ }

	const std::regex &get() const
	{
		return m_rx;
	}

	bool empty() const
	{
		return m_empty;
	}

private:
	std::regex m_rx;
	bool m_empty;
};

inline Flags literalCaseInsensitive()
{
	return Flags(std::regex_constants::ECMAScript | std::regex_constants::icase,
	             true);
}

inline Flags basicCaseInsensitive()
{
	return Flags(std::regex_constants::basic | std::regex_constants::icase);
}

inline Flags extendedCaseInsensitive()
{
	return Flags(std::regex_constants::extended | std::regex_constants::icase);
}

template <typename StringT>
inline Regex make(StringT &&s, Flags flags)
{
	return Regex(std::forward<StringT>(s), flags);
}

template <typename CharT>
inline bool search(const std::basic_string<CharT> &s, const Regex &rx)
{
	try {
		if (rx.empty())
			return false;
		const auto &str = convertString<char, CharT>::apply(s);
		return std::regex_search(str, rx.get());
	} catch (std::out_of_range &e) {
		// Invalid UTF-8 sequence, ignore the string.
		std::cerr << "Regex::search: error while processing \""
		          << s
		          << "\": "
		          << e.what()
		          << "\n";
		return false;
	}
}

template <typename CallbackT>
inline bool forEachMatch(const std::string &s,
                         const std::string &constraint,
                         Flags flags,
                         CallbackT &&callback)
{
	Regex rx = make(constraint, flags);
	auto first = std::sregex_iterator(s.begin(), s.end(), rx.get());
	auto last = std::sregex_iterator();
	bool success = first != last;
	for (; first != last; ++first)
		callback(first->position(), first->length());
	return success;
}

}

#endif // NCMPCPP_UTILITY_REGEX_H
