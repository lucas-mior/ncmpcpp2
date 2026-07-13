#ifndef NCMPCPP_UTILITY_STRING_H
#define NCMPCPP_UTILITY_STRING_H

#include <cstdint>
#include <string>

#include "c/ncm_base.h"
#include "c/ncm_string.h"

template <size_t N> size_t const_strlen(const char (&)[N]) {
	return N-1;
}

// Similar helpers usually support std::string, but we want a more general version.
namespace ncmpcpp_utility_detail {

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
	if (buffer.data != nullptr)
		result.assign(buffer.data, static_cast<size_t>(buffer.len));
	ncm_buffer_destroy(&buffer);
	return result;
}

}

inline std::string getBasename(const std::string &path)
{
	int32 start = ncm_string_basename_start(
		ncmpcpp_utility_detail::string_data(path),
		ncmpcpp_utility_detail::string_size(path));
	return path.substr(static_cast<size_t>(start));
}

inline std::string getParentDirectory(std::string path)
{
	int32 len = ncm_string_parent_directory_len(
		path.data(), ncmpcpp_utility_detail::string_size(path));
	path.resize(static_cast<size_t>(len));
	return path;
}

inline std::string getSharedDirectory(const std::string &dir1,
                                      const std::string &dir2)
{
	NcmBuffer buffer = ncm_string_shared_directory(
		ncmpcpp_utility_detail::string_data(dir1),
		ncmpcpp_utility_detail::string_size(dir1),
		ncmpcpp_utility_detail::string_data(dir2),
		ncmpcpp_utility_detail::string_size(dir2));
	return ncmpcpp_utility_detail::take_buffer(buffer);
}

inline std::string lowercaseAscii(std::string s)
{
	ncm_string_lowercase_ascii(
		s.data(), ncmpcpp_utility_detail::string_size(s));
	return s;
}

inline void removeInvalidCharsFromFilename(std::string &filename,
                                           bool win32_compatible)
{
	int32 filename_len = ncmpcpp_utility_detail::string_size(filename);
	ncm_string_remove_invalid_filename_chars(
		filename.data(), &filename_len, win32_compatible);
	filename.resize(static_cast<size_t>(filename_len));
}

#endif // NCMPCPP_UTILITY_STRING_H
