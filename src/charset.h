#ifndef NCMPCPP_CHARSET_H
#define NCMPCPP_CHARSET_H

#include <string>
#include <utility>

#include "c/ncm_charset.h"

namespace Charset {

namespace Detail {

inline int32 stringSize(const std::string &s)
{
	return static_cast<int32>(s.size());
}

inline char *stringData(const std::string &s)
{
	return const_cast<char *>(s.data());
}

inline std::string takeBuffer(NcmBuffer &buffer)
{
	std::string result;
	if (buffer.len > 0)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

}

inline std::string utf8ToLocale(const std::string &s)
{
	NcmBuffer result = ncm_charset_utf8_to_locale(
		Detail::stringData(s), Detail::stringSize(s));
	return Detail::takeBuffer(result);
}

inline std::string utf8ToLocale(std::string &&s)
{
	return std::move(s);
}

}

#endif // NCMPCPP_CHARSET_H
