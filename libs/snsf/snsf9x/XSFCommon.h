/*
 * xSF - Common functions
 * By Naram Qashat (CyberBotX) [cyberbotx@cyberbotx.com]
 * Last modification on 2014-09-24
 *
 * Partially based on the vio*sf framework
 */

#pragma once

#include <limits>
#include <fstream>
#if defined(_WIN32) && !defined(_MSC_VER)
# include "fstream_wfopen.h"
#endif
#include <string>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <cstring>
#include "convert.h"

// Code from http://learningcppisfun.blogspot.com/2010/04/comparing-floating-point-numbers.html
template<typename T> inline bool fEqual(T x, T y, int N = 1)
{
	T diff = std::abs(x - y);
	T tolerance = N * std::numeric_limits<T>::epsilon();
	return diff <= tolerance * std::abs(x) && diff <= tolerance * std::abs(y);
}

inline uint32_t Get32BitsLE(const uint8_t *input)
{
	return input[0] | (input[1] << 8) | (input[2] << 16) | (input[3] << 24);
}

inline uint32_t Get32BitsLE(std::ifstream &input)
{
	uint8_t bytes[4];
	input.read(reinterpret_cast<char *>(bytes), 4);
	return Get32BitsLE(bytes);
}

// This gets the directory for the path, including the final forward/backward slash
template<typename T> inline std::basic_string<T> ExtractDirectoryFromPath(const std::basic_string<T> &fullPath)
{
	auto lastSlash = fullPath.rfind('\\');
	if (lastSlash == std::basic_string<T>::npos)
		lastSlash = fullPath.rfind('/');
	return lastSlash != std::basic_string<T>::npos ? fullPath.substr(0, lastSlash + 1) : std::basic_string<T>();
}

// This gets the filename for the path
template<typename T> inline std::basic_string<T> ExtractFilenameFromPath(const std::basic_string<T> &fullPath)
{
	auto lastSlash = fullPath.rfind('\\');
	if (lastSlash == std::basic_string<T>::npos)
		lastSlash = fullPath.rfind('/');
	return lastSlash != std::basic_string<T>::npos ? fullPath.substr(lastSlash + 1) : std::basic_string<T>();
}

// Code from the following answer on Stack Overflow:
// http://stackoverflow.com/a/15479212
template<typename T> inline T NextHighestPowerOf2(T value)
{
	if (value < 1)
		return 1;
	--value;
	for (size_t i = 1; i < sizeof(T) * std::numeric_limits<unsigned char>::digits; i <<= 1)
		value |= value >> i;
	return value + 1;
}

// Clamp a value between a minimum and maximum value
template<typename T1, typename T2> inline void clamp(T1 &valueToClamp, const T2 &minValue, const T2 &maxValue)
{
	if (valueToClamp < minValue)
		valueToClamp = minValue;
	else if (valueToClamp > maxValue)
		valueToClamp = maxValue;
}

inline bool FileExists(const std::string &filename)
{
	std::ifstream file(filename.c_str());
	return !!file;
}

#ifdef _WIN32
inline bool FileExists(const std::wstring &filename)
{
#ifdef _MSC_VER
	std::ifstream file(filename.c_str());
#else
	ifstream_wfopen file(filename.c_str());
#endif
	return !!file;
}
#endif

inline void CopyToString(const std::wstring &src, wchar_t *dst)
{
	wcscpy(dst, src.c_str());
}

inline void CopyToString(const std::string &src, wchar_t *dst)
{
	wcscpy(dst, ConvertFuncs::StringToWString(src).c_str());
}

inline void CopyToString(const std::string &src, char *dst)
{
	strcpy(dst, src.c_str());
}

inline void CopyToString(const std::wstring &src, char *dst)
{
	strcpy(dst, ConvertFuncs::WStringToString(src).c_str());
}
