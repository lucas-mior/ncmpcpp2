#ifndef NCMPCPP_UTILITY_STRING_FORMAT_H
#define NCMPCPP_UTILITY_STRING_FORMAT_H

#include <cctype>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace StringFormat {

template <typename ValueT>
std::string toString(ValueT &&value)
{
	std::ostringstream os;
	os << std::forward<ValueT>(value);
	return os.str();
}

inline void appendArgument(std::string &result,
                           const std::vector<std::string> &args,
                           size_t idx)
{
	if (idx < args.size())
		result += args[idx];
}

template <typename... Args>
std::string apply(const std::string &fmt, Args&&... args)
{
	std::vector<std::string> values { toString(std::forward<Args>(args))... };
	std::string result;
	size_t next_arg = 0;

	for (size_t i = 0; i < fmt.size(); )
	{
		if (fmt[i] != '%' || i+1 == fmt.size())
		{
			result += fmt[i++];
			continue;
		}

		if (fmt[i+1] == '%')
		{
			result += '%';
			i += 2;
			continue;
		}

		if (fmt[i+1] == 's')
		{
			appendArgument(result, values, next_arg++);
			i += 2;
			continue;
		}

		if (std::isdigit(static_cast<unsigned char>(fmt[i+1])))
		{
			size_t idx = 0;
			size_t j = i+1;
			for (; j < fmt.size() && std::isdigit(static_cast<unsigned char>(fmt[j])); ++j)
				idx = idx*10 + static_cast<size_t>(fmt[j]-'0');
			if (idx > 0 && j < fmt.size() && fmt[j] == '%')
			{
				appendArgument(result, values, idx-1);
				i = j+1;
				continue;
			}
		}

		result += fmt[i++];
	}

	return result;
}

}

template <typename... Args>
std::string stringFormat(const std::string &fmt, Args&&... args)
{
	return StringFormat::apply(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
std::string stringFormat(const char *fmt, Args&&... args)
{
	return StringFormat::apply(std::string(fmt), std::forward<Args>(args)...);
}

#endif // NCMPCPP_UTILITY_STRING_FORMAT_H
