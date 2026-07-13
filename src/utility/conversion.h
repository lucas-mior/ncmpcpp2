#ifndef NCMPCPP_UTILITY_CONVERSION_H
#define NCMPCPP_UTILITY_CONVERSION_H

#include <exception>
#include <sstream>
#include <string>
#include <type_traits>

#include "config.h"
#include "utility/string_format.h"

struct ConversionError
{
	ConversionError(std::string source) : m_source_value(source) { }
	
	const std::string &value() { return m_source_value; }
	
private:
	std::string m_source_value;
};

struct OutOfBounds : std::exception
{
	const std::string &errorMessage() { return m_error_message; }
	
	template <typename Type>
	[[noreturn]] static void raise(const Type &value, const Type &lbound, const Type &ubound)
	{
		throw OutOfBounds(stringFormat(
			"value is out of bounds ([%1%, %2%] expected, %3% given)", lbound, ubound, value));
	}
	
	template <typename Type>
	[[noreturn]] static void raiseLower(const Type &value, const Type &lbound)
	{
		throw OutOfBounds(stringFormat(
			"value is out of bounds ([%1%, ->) expected, %2% given)", lbound, value));
	}
	
	virtual const char *what() const noexcept override { return m_error_message.c_str(); }

private:
	OutOfBounds(std::string msg) : m_error_message(msg) { }
	
	std::string m_error_message;
};

template <typename TargetT, bool isUnsigned>
struct unsigned_checker
{
	static void apply(const std::string &) { }
};
template <typename TargetT>
struct unsigned_checker<TargetT, true>
{
	static void apply(const std::string &s)
	{
		if (s[0] == '-')
			throw ConversionError(s);
	}
};

template <typename TargetT>
TargetT fromString(const std::string &source)
{
	unsigned_checker<TargetT, std::is_unsigned<TargetT>::value>::apply(source);
	std::istringstream is(source);
	TargetT result;
	if (!(is >> result))
		throw ConversionError(source);
	is >> std::ws;
	if (!is.eof())
		throw ConversionError(source);
	return result;
}

template <typename Type>
void boundsCheck(const Type &value, const Type &lbound, const Type &ubound)
{
	if (value < lbound || value > ubound)
		OutOfBounds::raise(value, lbound, ubound);
}

template <typename Type>
void lowerBoundCheck(const Type &value, const Type &lbound)
{
	if (value < lbound)
		OutOfBounds::raiseLower(value, lbound);
}

#endif // NCMPCPP_UTILITY_CONVERSION_H
