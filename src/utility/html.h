#ifndef NCMPCPP_UTILITY_HTML_H
#define NCMPCPP_UTILITY_HTML_H

#include <string>

#include "c/ncm_html.h"

namespace ncmpcpp_html_detail {

inline int32 string_size(const std::string &s)
{
	return static_cast<int32>(s.size());
}

inline char *string_data(const std::string &s)
{
	return const_cast<char *>(s.data());
}

inline std::string take_buffer(NcmBuffer &buffer)
{
	std::string result;
	if (buffer.len > 0)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

}

inline std::string unescapeHtmlUtf8(const std::string &s)
{
	NcmBuffer result = ncm_html_unescape_utf8(
		ncmpcpp_html_detail::string_data(s),
		ncmpcpp_html_detail::string_size(s));
	return ncmpcpp_html_detail::take_buffer(result);
}

inline void unescapeHtmlEntities(std::string &s)
{
	NcmBuffer result = ncm_html_unescape_entities(
		s.data(), ncmpcpp_html_detail::string_size(s));
	s = ncmpcpp_html_detail::take_buffer(result);
}

inline void stripHtmlTags(std::string &s)
{
	NcmBuffer result = ncm_html_strip_tags(
		s.data(), ncmpcpp_html_detail::string_size(s));
	s = ncmpcpp_html_detail::take_buffer(result);
}

#endif // NCMPCPP_UTILITY_HTML_H
