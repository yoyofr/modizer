/*
 * test.cpp
 * --------
 * Purpose: Unit tests for OpenMPT.
 * Notes  : We need FAAAAAAAR more unit tests!
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#include "stdafx.h"
#include "test.h"


#ifdef ENABLE_TESTS


#include "../common/version.h"
#include "../common/misc_util.h"
#include "../common/StringFixer.h"
#include "../common/serialization_utils.h"
#include "../soundlib/Sndfile.h"
#include "../common/FileReader.h"
#include "../soundlib/MIDIEvents.h"
#include "../soundlib/MIDIMacros.h"
#include "../soundlib/SampleFormatConverters.h"
#include "../soundlib/ITCompression.h"
#ifdef MODPLUG_TRACKER
#include "../mptrack/mptrack.h"
#include "../mptrack/moddoc.h"
#include "../mptrack/MainFrm.h"
#include "../mptrack/Settings.h"
#endif // MODPLUG_TRACKER
#ifndef MODPLUG_TRACKER
#include "../common/mptFileIO.h"
#endif // !MODPLUG_TRACKER
#include <limits>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#if MPT_OS_WINDOWS
#include <windows.h>
#endif

#ifdef _DEBUG
#if MPT_COMPILER_MSVC && defined(_MFC_VER)
	#define new DEBUG_NEW
#endif
#endif

#include "TestTools.h"



OPENMPT_NAMESPACE_BEGIN



namespace Test {



static noinline void TestVersion();
static noinline void TestTypes();
static noinline void TestMisc();
static noinline void TestCharsets();
static noinline void TestStringFormatting();
static noinline void TestSettings();
static noinline void TestStringIO();
static noinline void TestMIDIEvents();
static noinline void TestSampleConversion();
static noinline void TestITCompression();
static noinline void TestPCnoteSerialization();
static noinline void TestLoadSaveFile();



static mpt::PathString *PathPrefix = nullptr;



static mpt::PathString GetPathPrefix()
//------------------------------------
{
	if((*PathPrefix).empty())
	{
		return MPT_PATHSTRING("");
	}
	return *PathPrefix + MPT_PATHSTRING("/");
}


void DoTests()
//------------
{

	#if MPT_OS_WINDOWS

		// prefix for test suite
		std::wstring pathprefix = std::wstring();

		// set path prefix for test files (if provided)
		std::vector<WCHAR> buf(GetEnvironmentVariableW(L"srcdir", NULL, 0) + 1);
		if(GetEnvironmentVariableW(L"srcdir", &buf[0], static_cast<DWORD>(buf.size())) > 0)
		{
			pathprefix = &buf[0];
		}

		PathPrefix = new mpt::PathString(mpt::PathString::FromNative(pathprefix));

	#else

		// prefix for test suite
		std::string pathprefix = std::string();

		// set path prefix for test files (if provided)
		std::string env_srcdir = std::getenv( "srcdir" ) ? std::getenv( "srcdir" ) : std::string();
		if ( !env_srcdir.empty() ) {
			pathprefix = env_srcdir;
		}

		PathPrefix = new mpt::PathString(mpt::PathString::FromNative(pathprefix));

	#endif

	DO_TEST(TestVersion);
	DO_TEST(TestTypes);
	DO_TEST(TestMisc);
	DO_TEST(TestCharsets);
	DO_TEST(TestStringFormatting);
	DO_TEST(TestSettings);
	DO_TEST(TestStringIO);
	DO_TEST(TestMIDIEvents);
	DO_TEST(TestSampleConversion);
	DO_TEST(TestITCompression);

	// slower tests, require opening a CModDoc
	DO_TEST(TestPCnoteSerialization);
	DO_TEST(TestLoadSaveFile);

	delete PathPrefix;
	PathPrefix = nullptr;
}


static void RemoveFile(const mpt::PathString &filename)
//-----------------------------------------------------
{
	#if MPT_OS_WINDOWS
		for(int retry=0; retry<10; retry++)
		{
			if(DeleteFileW(filename.AsNative().c_str()) != FALSE)
			{
				break;
			}
			// wait for windows virus scanners
			Sleep(10);
		}
	#else
		remove(filename.AsNative().c_str());
	#endif
}


// Test if functions related to program version data work
static noinline void TestVersion()
//--------------------------------
{
	//Verify that macros and functions work.
	{
		VERIFY_EQUAL( MptVersion::ToNum(MptVersion::ToStr(MptVersion::num)), MptVersion::num );
		VERIFY_EQUAL( MptVersion::ToStr(MptVersion::ToNum(MptVersion::str)), MptVersion::str );
		VERIFY_EQUAL( MptVersion::ToStr(18285096), "1.17.02.28" );
		VERIFY_EQUAL( MptVersion::ToNum("1.17.02.28"), MptVersion::VersionNum(18285096) );
		VERIFY_EQUAL( MptVersion::ToNum("1.fe.02.28"), MptVersion::VersionNum(0x01fe0228) );
		VERIFY_EQUAL( MptVersion::ToNum("01.fe.02.28"), MptVersion::VersionNum(0x01fe0228) );
		VERIFY_EQUAL( MptVersion::ToNum("1.22"), MptVersion::VersionNum(0x01220000) );
		VERIFY_EQUAL( MptVersion::ToNum(MptVersion::str), MptVersion::num );
		VERIFY_EQUAL( MptVersion::ToStr(MptVersion::num), MptVersion::str );
		VERIFY_EQUAL( MptVersion::RemoveBuildNumber(MAKE_VERSION_NUMERIC(1,19,02,00)), MAKE_VERSION_NUMERIC(1,19,02,00));
		VERIFY_EQUAL( MptVersion::RemoveBuildNumber(MAKE_VERSION_NUMERIC(1,18,03,20)), MAKE_VERSION_NUMERIC(1,18,03,00));
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,01,13)), true);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,19,01,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,17,02,54)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,00,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,02,00)), false);
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,18,02,01)), true);

		// Ensure that versions ending in .00.00 (which are ambiguous to truncated version numbers in certain file formats (e.g. S3M and IT) do not get qualified as test builds.
		VERIFY_EQUAL( MptVersion::IsTestBuild(MAKE_VERSION_NUMERIC(1,23,00,00)), false);

		STATIC_ASSERT( MAKE_VERSION_NUMERIC(1,17,2,28) == 18285096 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(1,17,02,48) == 18285128 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,02,52) == 18285138 );
		// Ensure that bit-shifting works (used in some mod loaders for example)
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,00,00) == 0x0117 << 16 );
		STATIC_ASSERT( MAKE_VERSION_NUMERIC(01,17,03,00) >> 8 == 0x011703 );
	}

#ifdef MODPLUG_TRACKER
	//Verify that the version obtained from the executable file is the same as
	//defined in MptVersion.
	{
		char  szFullPath[MAX_PATH];
		DWORD dwVerHnd;
		DWORD dwVerInfoSize;

		// Get version information from the application
		::GetModuleFileName(NULL, szFullPath, sizeof(szFullPath));
		dwVerInfoSize = ::GetFileVersionInfoSize(szFullPath, &dwVerHnd);
		if (!dwVerInfoSize)
			throw std::runtime_error("!dwVerInfoSize is true");

		char *pVersionInfo;
		try
		{
			pVersionInfo = new char[dwVerInfoSize];
		} catch(MPTMemoryException)
		{
			throw std::runtime_error("Could not allocate memory for pVersionInfo");
		}

		char* szVer = NULL;
		UINT uVerLength;
		if (!(::GetFileVersionInfo((LPTSTR)szFullPath, (DWORD)dwVerHnd,
								   (DWORD)dwVerInfoSize, (LPVOID)pVersionInfo)))
		{
			delete[] pVersionInfo;
			throw std::runtime_error("GetFileVersionInfo() returned false");
		}
		if (!(::VerQueryValue(pVersionInfo, TEXT("\\StringFileInfo\\040904b0\\FileVersion"),
							  (LPVOID*)&szVer, &uVerLength))) {
			delete[] pVersionInfo;
			throw std::runtime_error("VerQueryValue() returned false");
		}

		std::string version = szVer;
		delete[] pVersionInfo;

		//version string should be like: 1,17,2,38  Change ',' to '.' to get format 1.17.2.38
		version = mpt::String::Replace(version, ",", ".");

		VERIFY_EQUAL( version, MptVersion::str );
		VERIFY_EQUAL( MptVersion::ToNum(version), MptVersion::num );
	}
#endif
}


// Test if data types are interpreted correctly
static noinline void TestTypes()
//------------------------------
{
	VERIFY_EQUAL(int8_min, (std::numeric_limits<int8>::min)());
	VERIFY_EQUAL(int8_max, (std::numeric_limits<int8>::max)());
	VERIFY_EQUAL(uint8_max, (std::numeric_limits<uint8>::max)());

	VERIFY_EQUAL(int16_min, (std::numeric_limits<int16>::min)());
	VERIFY_EQUAL(int16_max, (std::numeric_limits<int16>::max)());
	VERIFY_EQUAL(uint16_max, (std::numeric_limits<uint16>::max)());

	VERIFY_EQUAL(int32_min, (std::numeric_limits<int32>::min)());
	VERIFY_EQUAL(int32_max, (std::numeric_limits<int32>::max)());
	VERIFY_EQUAL(uint32_max, (std::numeric_limits<uint32>::max)());

	VERIFY_EQUAL(int64_min, (std::numeric_limits<int64>::min)());
	VERIFY_EQUAL(int64_max, (std::numeric_limits<int64>::max)());
	VERIFY_EQUAL(uint64_max, (std::numeric_limits<uint64>::max)());
}



#ifdef MODPLUG_TRACKER

// In tracker debug builds, the sprintf-like function is retained in order to be able to validate our own formatting against sprintf.

// There are 4 reasons why this is not available for library code:
//  1. printf-like functionality is not type-safe.
//  2. There are portability problems with char/wchar_t and the semantics of %s/%ls/%S .
//  3. There are portability problems with specifying format for 64bit integers.
//  4. Formatting of floating point values depends on the currently set C locale.
//     A library is not allowed to mock with that and thus cannot influence the behavior in this case.

static std::string MPT_PRINTF_FUNC(1,2) StringFormat(const char * format, ...);

static std::string StringFormat(const char *format, ...)
{
	#if MPT_COMPILER_MSVC
		va_list argList;
		va_start(argList, format);

		// Count the needed array size.
		const size_t nCount = _vscprintf(format, argList); // null character not included.
		std::vector<char> buf(nCount + 1); // + 1 is for null terminator.
		vsprintf_s(&(buf[0]), buf.size(), format, argList);

		va_end(argList);
		return &(buf[0]);
	#else
		va_list argList;
		va_start(argList, format);
		int size = vsnprintf(NULL, 0, format, argList); // get required size, requires c99 compliant vsnprintf which msvc does not have
		va_end(argList);
		std::vector<char> temp(size + 1);
		va_start(argList, format);
		vsnprintf(&(temp[0]), size + 1, format, argList);
		va_end(argList);
		return &(temp[0]);
	#endif
}

#endif

static void TestFloatFormat(double x, const char * format, mpt::FormatFlags f, std::size_t width = 0, int precision = -1)
{
#ifdef MODPLUG_TRACKER
	std::string str_sprintf = StringFormat(format, x);
#endif
	std::string str_iostreams = mpt::Format().SetFlags(f).SetWidth(width).SetPrecision(precision).ToString(x);
	std::string str_parsed = mpt::Format().ParsePrintf(format).ToString(x);
	//Log("%s", str_sprintf.c_str());
	//Log("%s", str_iostreams.c_str());
	//Log("%s", str_iostreams.c_str());
#ifdef MODPLUG_TRACKER
	VERIFY_EQUAL(str_iostreams, str_sprintf); // this will fail with a set c locale (and there is nothing that can be done about that in libopenmpt)
#endif
	VERIFY_EQUAL(str_iostreams, str_parsed);
}


static void TestFloatFormats(double x)
{

	TestFloatFormat(x, "%g", mpt::fmt::NotaNrm | mpt::fmt::FillOff);
	TestFloatFormat(x, "%.8g", mpt::fmt::NotaNrm | mpt::fmt::FillOff, 0, 8);

	TestFloatFormat(x, "%f", mpt::fmt::NotaFix | mpt::fmt::FillOff);

	TestFloatFormat(x, "%.0f", mpt::fmt::NotaFix | mpt::fmt::FillOff, 0, 0);
	TestFloatFormat(x, "%.1f", mpt::fmt::NotaFix | mpt::fmt::FillOff, 0, 1);
	TestFloatFormat(x, "%.2f", mpt::fmt::NotaFix | mpt::fmt::FillOff, 0, 2);
	TestFloatFormat(x, "%.3f", mpt::fmt::NotaFix | mpt::fmt::FillOff, 0, 3);
	TestFloatFormat(x, "%1.1f", mpt::fmt::NotaFix | mpt::fmt::FillSpc, 1, 1);
	TestFloatFormat(x, "%3.1f", mpt::fmt::NotaFix | mpt::fmt::FillSpc, 3, 1);
	TestFloatFormat(x, "%4.1f", mpt::fmt::NotaFix | mpt::fmt::FillSpc, 4, 1);
	TestFloatFormat(x, "%6.3f", mpt::fmt::NotaFix | mpt::fmt::FillSpc, 6, 3);
	TestFloatFormat(x, "%0.1f", mpt::fmt::NotaFix | mpt::fmt::FillNul, 0, 1);
	TestFloatFormat(x, "%02.0f", mpt::fmt::NotaFix | mpt::fmt::FillNul, 2, 0);
}



static bool BeginsWith(const std::string &str, const std::string &match)
{
	return (str.find(match) == 0);
}
static bool EndsWith(const std::string &str, const std::string &match)
{
	return (str.rfind(match) == (str.length() - match.length()));
}

#if MPT_WSTRING_CONVERT
static bool BeginsWith(const std::wstring &str, const std::wstring &match)
{
	return (str.find(match) == 0);
}
static bool EndsWith(const std::wstring &str, const std::wstring &match)
{
	return (str.rfind(match) == (str.length() - match.length()));
}
#endif

#if MPT_USTRING_MODE_UTF8
static bool BeginsWith(const mpt::ustring &str, const mpt::ustring &match)
{
	return (str.find(match) == 0);
}
static bool EndsWith(const mpt::ustring &str, const mpt::ustring &match)
{
	return (str.rfind(match) == (str.length() - match.length()));
}
#endif



#ifdef MODPLUG_TRACKER
static bool IsEqualUUID(const UUID &lhs, const UUID &rhs)
{
	return std::memcmp(&lhs, &rhs, sizeof(UUID)) == 0;
}
#endif


static noinline void TestStringFormatting()
//-----------------------------------------
{

	VERIFY_EQUAL(Stringify(1.5f), "1.5");
	VERIFY_EQUAL(Stringify(true), "1");
	VERIFY_EQUAL(Stringify(false), "0");
	//VERIFY_EQUAL(Stringify('A'), "A"); // deprecated
	//VERIFY_EQUAL(Stringify(L'A'), "A"); // deprecated

	VERIFY_EQUAL(Stringify(0), "0");
	VERIFY_EQUAL(Stringify(-23), "-23");
	VERIFY_EQUAL(Stringify(42), "42");

	VERIFY_EQUAL(mpt::fmt::hex<3>((int32)-1), "ffffffff");
	VERIFY_EQUAL(mpt::fmt::hex(0x123e), "123e");
	VERIFY_EQUAL(mpt::fmt::hex0<6>(0x123e), "00123e");
	VERIFY_EQUAL(mpt::fmt::hex0<2>(0x123e), "123e");

#if MPT_WSTRING_FORMAT
	VERIFY_EQUAL(mpt::wfmt::hex<3>((int32)-1), L"ffffffff");
	VERIFY_EQUAL(mpt::wfmt::hex(0x123e), L"123e");
	VERIFY_EQUAL(mpt::wfmt::hex0<6>(0x123e), L"00123e");
	VERIFY_EQUAL(mpt::wfmt::hex0<2>(0x123e), L"123e");
#endif

	VERIFY_EQUAL(Stringify(-87.0f), "-87");
	if(Stringify(-0.5e-6) != "-5e-007"
		&& Stringify(-0.5e-6) != "-5e-07"
		&& Stringify(-0.5e-6) != "-5e-7"
		)
	{
		VERIFY_EQUAL(true, false);
	}
	VERIFY_EQUAL(Stringify(58.65403492763), "58.654");
	VERIFY_EQUAL(mpt::Format("%3.1f").ToString(23.42), "23.4");

	VERIFY_EQUAL(ConvertStrTo<uint32>("586"), 586u);
	VERIFY_EQUAL(ConvertStrTo<uint32>("2147483647"), (uint32)int32_max);
	VERIFY_EQUAL(ConvertStrTo<uint32>("4294967295"), uint32_max);

	VERIFY_EQUAL(ConvertStrTo<int64>("-9223372036854775808"), int64_min);
	VERIFY_EQUAL(ConvertStrTo<int64>("-159"), -159);
	VERIFY_EQUAL(ConvertStrTo<int64>("9223372036854775807"), int64_max);

	VERIFY_EQUAL(ConvertStrTo<uint64>("85059"), 85059u);
	VERIFY_EQUAL(ConvertStrTo<uint64>("9223372036854775807"), (uint64)int64_max);
	VERIFY_EQUAL(ConvertStrTo<uint64>("18446744073709551615"), uint64_max);

	VERIFY_EQUAL(ConvertStrTo<float>("-87.0"), -87.0);
	VERIFY_EQUAL(ConvertStrTo<double>("-0.5e-6"), -0.5e-6);
	VERIFY_EQUAL(ConvertStrTo<double>("58.65403492763"), 58.65403492763);

	VERIFY_EQUAL(ConvertStrTo<float>(Stringify(-87.0)), -87.0);
	VERIFY_EQUAL(ConvertStrTo<double>(Stringify(-0.5e-6)), -0.5e-6);

	TestFloatFormats(0.0f);
	TestFloatFormats(1.0f);
	TestFloatFormats(-1.0f);
	TestFloatFormats(0.1f);
	TestFloatFormats(-0.1f);
	TestFloatFormats(1000000000.0f);
	TestFloatFormats(-1000000000.0f);
	TestFloatFormats(0.0000000001f);
	TestFloatFormats(-0.0000000001f);
	TestFloatFormats(6.12345f);

	TestFloatFormats(42.1234567890);
	TestFloatFormats(0.1234567890);
	TestFloatFormats(1234567890000000.0);
	TestFloatFormats(0.0000001234567890);

	VERIFY_EQUAL(mpt::Format().ParsePrintf("%7.3f").ToString(6.12345), "  6.123");
	VERIFY_EQUAL(mpt::fmt::flt(6.12345, 7, 3), "  6.123");
	VERIFY_EQUAL(mpt::fmt::fix(6.12345, 7, 3), "  6.123");
	VERIFY_EQUAL(mpt::fmt::flt(6.12345, 0, 4), "6.123");
	#if !MPT_OS_EMSCRIPTEN
	VERIFY_EQUAL(mpt::fmt::fix(6.12345, 0, 4), "6.1235");
	#else
	// emscripten(1.21)/nodejs(v0.10.25) print 6.1234 instead of 6.1235 for unknown reasons.
	// As this test case is not fatal, ignore it for now in order to make the test cases pass.
	#endif

#if MPT_WSTRING_FORMAT
	VERIFY_EQUAL(mpt::wfmt::flt(6.12345, 7, 3), L"  6.123");
	VERIFY_EQUAL(mpt::wfmt::fix(6.12345, 7, 3), L"  6.123");
	VERIFY_EQUAL(mpt::wfmt::flt(6.12345, 0, 4), L"6.123");
#endif

	// basic functionality
	VERIFY_EQUAL(mpt::String::Print("%1%2%3",1,2,3), "123");
	VERIFY_EQUAL(mpt::String::Print("%1%1%1",1,2,3), "111");
	VERIFY_EQUAL(mpt::String::Print("%3%3%3",1,2,3), "333");

	// template argument deduction of string type
	VERIFY_EQUAL(mpt::String::Print(std::string("%1%2%3"),1,2,3), "123");
#if MPT_WSTRING_FORMAT
	VERIFY_EQUAL(mpt::String::Print(std::wstring(L"%1%2%3"),1,2,3), L"123");
	VERIFY_EQUAL(mpt::String::Print(L"%1%2%3",1,2,3), L"123");
	VERIFY_EQUAL(mpt::String::PrintW(L"%1%2%3",1,2,3), L"123");
#endif

	// escaping and error behviour of '%'
	VERIFY_EQUAL(mpt::String::Print("%"), "%");
	VERIFY_EQUAL(mpt::String::Print("%%"), "%");
	VERIFY_EQUAL(mpt::String::Print("%%%"), "%%");
	VERIFY_EQUAL(mpt::String::Print("%1", "a"), "a");
	VERIFY_EQUAL(mpt::String::Print("%1%", "a"), "a%");
	VERIFY_EQUAL(mpt::String::Print("%1%%", "a"), "a%");
	VERIFY_EQUAL(mpt::String::Print("%1%%%", "a"), "a%%");
	VERIFY_EQUAL(mpt::String::Print("%%1", "a"), "%1");
	VERIFY_EQUAL(mpt::String::Print("%%%1", "a"), "%a");
	VERIFY_EQUAL(mpt::String::Print("%b", "a"), "%b");

}


static noinline void TestMisc()
//-----------------------------
{

	VERIFY_EQUAL(EncodeIEEE754binary32(1.0f), 0x3f800000u);
	VERIFY_EQUAL(EncodeIEEE754binary32(-1.0f), 0xbf800000u);
	VERIFY_EQUAL(DecodeIEEE754binary32(0x00000000u), 0.0f);
	VERIFY_EQUAL(DecodeIEEE754binary32(0x41840000u), 16.5f);
	VERIFY_EQUAL(DecodeIEEE754binary32(0x3faa0000u),  1.328125f);
	VERIFY_EQUAL(DecodeIEEE754binary32(0xbfaa0000u), -1.328125f);
	VERIFY_EQUAL(DecodeIEEE754binary32(0x3f800000u),  1.0f);
	VERIFY_EQUAL(DecodeIEEE754binary32(0x00000000u),  0.0f);
	VERIFY_EQUAL(DecodeIEEE754binary32(0xbf800000u), -1.0f);
	VERIFY_EQUAL(DecodeIEEE754binary32(0x3f800000u),  1.0f);
	VERIFY_EQUAL(IEEE754binary32LE(1.0f).GetInt32(), 0x3f800000u);
	VERIFY_EQUAL(IEEE754binary32BE(1.0f).GetInt32(), 0x3f800000u);
	VERIFY_EQUAL(IEEE754binary32LE(0x00,0x00,0x80,0x3f), 1.0f);
	VERIFY_EQUAL(IEEE754binary32BE(0x3f,0x80,0x00,0x00), 1.0f);
	VERIFY_EQUAL(IEEE754binary32LE(1.0f), IEEE754binary32LE(0x00,0x00,0x80,0x3f));
	VERIFY_EQUAL(IEEE754binary32BE(1.0f), IEEE754binary32BE(0x3f,0x80,0x00,0x00));

	VERIFY_EQUAL(EncodeIEEE754binary64(1.0), 0x3ff0000000000000ull);
	VERIFY_EQUAL(EncodeIEEE754binary64(-1.0), 0xbff0000000000000ull);
	VERIFY_EQUAL(DecodeIEEE754binary64(0x0000000000000000ull), 0.0);
	VERIFY_EQUAL(DecodeIEEE754binary64(0x4030800000000000ull), 16.5);
	VERIFY_EQUAL(DecodeIEEE754binary64(0x3FF5400000000000ull),  1.328125);
	VERIFY_EQUAL(DecodeIEEE754binary64(0xBFF5400000000000ull), -1.328125);
	VERIFY_EQUAL(DecodeIEEE754binary64(0x3ff0000000000000ull),  1.0);
	VERIFY_EQUAL(DecodeIEEE754binary64(0x0000000000000000ull),  0.0);
	VERIFY_EQUAL(DecodeIEEE754binary64(0xbff0000000000000ull), -1.0);
	VERIFY_EQUAL(DecodeIEEE754binary64(0x3ff0000000000000ull),  1.0);
	VERIFY_EQUAL(IEEE754binary64LE(1.0).GetInt64(), 0x3ff0000000000000ull);
	VERIFY_EQUAL(IEEE754binary64BE(1.0).GetInt64(), 0x3ff0000000000000ull);
	VERIFY_EQUAL(IEEE754binary64LE(0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x3f), 1.0);
	VERIFY_EQUAL(IEEE754binary64BE(0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00), 1.0);
	VERIFY_EQUAL(IEEE754binary64LE(1.0), IEEE754binary64LE(0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x3f));
	VERIFY_EQUAL(IEEE754binary64BE(1.0), IEEE754binary64BE(0x3f,0xf0,0x00,0x00,0x00,0x00,0x00,0x00));

	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_MAX), false);
	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_PC), true);
	VERIFY_EQUAL(ModCommand::IsPcNote(NOTE_PCS), true);

	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".mod"), MOD_TYPE_MOD);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("mod"), MOD_TYPE_MOD);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".s3m"), MOD_TYPE_S3M);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("s3m"), MOD_TYPE_S3M);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".xm"), MOD_TYPE_XM);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("xm"), MOD_TYPE_XM);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".it"), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("it"), MOD_TYPE_IT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(".itp"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("itp"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("mptm"), MOD_TYPE_MPT);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("invalidExtension"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("ita"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType("s2m"), MOD_TYPE_NONE);
	VERIFY_EQUAL(CModSpecifications::ExtensionToType(""), MOD_TYPE_NONE);

	VERIFY_EQUAL( Util::Round(1.99), 2.0 );
	VERIFY_EQUAL( Util::Round(1.5), 2.0 );
	VERIFY_EQUAL( Util::Round(1.1), 1.0 );
	VERIFY_EQUAL( Util::Round(-0.1), 0.0 );
	VERIFY_EQUAL( Util::Round(-0.5), -1.0 );
	VERIFY_EQUAL( Util::Round(-0.9), -1.0 );
	VERIFY_EQUAL( Util::Round(-1.4), -1.0 );
	VERIFY_EQUAL( Util::Round(-1.7), -2.0 );
	VERIFY_EQUAL( Util::Round<int32>(int32_max + 0.1), int32_max );
	VERIFY_EQUAL( Util::Round<int32>(int32_max - 0.4), int32_max );
	VERIFY_EQUAL( Util::Round<int32>(int32_min + 0.1), int32_min );
	VERIFY_EQUAL( Util::Round<int32>(int32_min - 0.1), int32_min );
	VERIFY_EQUAL( Util::Round<uint32>(uint32_max + 0.499), uint32_max );
	VERIFY_EQUAL( Util::Round<int8>(110.1), 110 );
	VERIFY_EQUAL( Util::Round<int8>(-110.1), -110 );

	// trivials
	VERIFY_EQUAL( mpt::saturate_cast<int>(-1), -1 );
	VERIFY_EQUAL( mpt::saturate_cast<int>(0), 0 );
	VERIFY_EQUAL( mpt::saturate_cast<int>(1), 1 );
	VERIFY_EQUAL( mpt::saturate_cast<int>(std::numeric_limits<int>::min()), std::numeric_limits<int>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int>(std::numeric_limits<int>::max()), std::numeric_limits<int>::max() );

	// signed / unsigned
	VERIFY_EQUAL( mpt::saturate_cast<int16>(std::numeric_limits<uint16>::min()), std::numeric_limits<uint16>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int16>(std::numeric_limits<uint16>::max()), std::numeric_limits<int16>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<uint32>::min()), (int32)std::numeric_limits<uint32>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<uint32>::max()), std::numeric_limits<int32>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<int64>(std::numeric_limits<uint64>::min()), (int64)std::numeric_limits<uint64>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int64>(std::numeric_limits<uint64>::max()), std::numeric_limits<int64>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>(std::numeric_limits<int16>::min()), std::numeric_limits<uint16>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>(std::numeric_limits<int16>::max()), std::numeric_limits<int16>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int32>::min()), std::numeric_limits<uint32>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int32>::max()), (uint32)std::numeric_limits<int32>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<uint64>(std::numeric_limits<int64>::min()), std::numeric_limits<uint64>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint64>(std::numeric_limits<int64>::max()), (uint64)std::numeric_limits<int64>::max() );

	// overflow
	VERIFY_EQUAL( mpt::saturate_cast<int16>(std::numeric_limits<int16>::min() - 1), std::numeric_limits<int16>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int16>(std::numeric_limits<int16>::max() + 1), std::numeric_limits<int16>::max() );
	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<int32>::min() - int64(1)), std::numeric_limits<int32>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<int32>::max() + int64(1)), std::numeric_limits<int32>::max() );

	VERIFY_EQUAL( mpt::saturate_cast<uint16>(std::numeric_limits<int16>::min() - 1), std::numeric_limits<uint16>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>(std::numeric_limits<int16>::max() + 1), (uint16)std::numeric_limits<int16>::max() + 1 );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int32>::min() - int64(1)), std::numeric_limits<uint32>::min() );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int32>::max() + int64(1)), (uint32)std::numeric_limits<int32>::max() + 1 );
	
	VERIFY_EQUAL( mpt::saturate_cast<int8>( int16(32000) ), 127 );
	VERIFY_EQUAL( mpt::saturate_cast<int8>( int16(-32000) ), -128 );
	VERIFY_EQUAL( mpt::saturate_cast<int8>( uint16(32000) ), 127 );
	VERIFY_EQUAL( mpt::saturate_cast<int8>( uint16(64000) ), 127 );
	VERIFY_EQUAL( mpt::saturate_cast<uint8>( int16(32000) ), 255 );
	VERIFY_EQUAL( mpt::saturate_cast<uint8>( int16(-32000) ), 0 );
	VERIFY_EQUAL( mpt::saturate_cast<uint8>( uint16(32000) ), 255 );
	VERIFY_EQUAL( mpt::saturate_cast<uint8>( uint16(64000) ), 255 );
	VERIFY_EQUAL( mpt::saturate_cast<int16>( int16(32000) ), 32000 );
	VERIFY_EQUAL( mpt::saturate_cast<int16>( int16(-32000) ), -32000 );
	VERIFY_EQUAL( mpt::saturate_cast<int16>( uint16(32000) ), 32000 );
	VERIFY_EQUAL( mpt::saturate_cast<int16>( uint16(64000) ), 32767 );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>( int16(32000) ), 32000 );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>( int16(-32000) ), 0 );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>( uint16(32000) ), 32000 );
	VERIFY_EQUAL( mpt::saturate_cast<uint16>( uint16(64000) ), 64000 );
	VERIFY_EQUAL( mpt::saturate_cast<int32>( int16(32000) ), 32000 );
	VERIFY_EQUAL( mpt::saturate_cast<int32>( int16(-32000) ), -32000 );
	VERIFY_EQUAL( mpt::saturate_cast<int32>( uint16(32000) ), 32000 );
	VERIFY_EQUAL( mpt::saturate_cast<int32>( uint16(64000) ), 64000 );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>( int16(32000) ), 32000 );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>( int16(-32000) ), 0 );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>( uint16(32000) ), 32000 );
	VERIFY_EQUAL( mpt::saturate_cast<uint32>( uint16(64000) ), 64000 );
	
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(std::numeric_limits<int64>::max() - 1), std::numeric_limits<uint32>::max() );

	VERIFY_EQUAL( mpt::saturate_cast<int32>(std::numeric_limits<uint64>::max() - 1), std::numeric_limits<int32>::max() );
	
	VERIFY_EQUAL( mpt::saturate_cast<uint32>(static_cast<double>(std::numeric_limits<int64>::max())), std::numeric_limits<uint32>::max() );

	VERIFY_EQUAL( mpt::String::LTrim(std::string(" ")), "" );
	VERIFY_EQUAL( mpt::String::RTrim(std::string(" ")), "" );
	VERIFY_EQUAL( mpt::String::Trim(std::string(" ")), "" );

	// weird things with std::string containing \0 in the middle and trimming \0
	VERIFY_EQUAL( std::string("\0\ta\0b ",6).length(), (std::size_t)6 );
	VERIFY_EQUAL( mpt::String::RTrim(std::string("\0\ta\0b ",6)), std::string("\0\ta\0b",5) );
	VERIFY_EQUAL( mpt::String::Trim(std::string("\0\ta\0b\0",6),std::string("\0",1)), std::string("\ta\0b",4) );

	// These should fail to compile
	//Util::Round<std::string>(1.0);
	//Util::Round<int64>(1.0);
	//Util::Round<uint64>(1.0);

	// This should trigger assert in Round.
	//VERIFY_EQUAL( Util::Round<int8>(-129), 0 );

	// Check for completeness of supported effect list in mod specifications
	for(size_t i = 0; i < CountOf(ModSpecs::Collection); i++)
	{
		VERIFY_EQUAL(strlen(ModSpecs::Collection[i]->commands), (size_t)MAX_EFFECTS);
		VERIFY_EQUAL(strlen(ModSpecs::Collection[i]->volcommands), (size_t)MAX_VOLCMDS);
	}

	// UUID
#ifdef MODPLUG_TRACKER
	VERIFY_EQUAL(Util::IsValid(Util::CreateGUID()), true);
	VERIFY_EQUAL(Util::IsValid(Util::CreateUUID()), true);
	VERIFY_EQUAL(Util::IsValid(Util::CreateLocalUUID()), true);
	UUID uuid = Util::CreateUUID();
	VERIFY_EQUAL(IsEqualUUID(uuid, Util::StringToUUID(Util::UUIDToString(uuid))), true);
	VERIFY_EQUAL(IsEqualUUID(uuid, Util::StringToGUID(Util::GUIDToString(uuid))), true);
	VERIFY_EQUAL(IsEqualUUID(uuid, Util::StringToIID(Util::IIDToString(uuid))), true);
	VERIFY_EQUAL(IsEqualUUID(uuid, Util::StringToCLSID(Util::CLSIDToString(uuid))), true);
#endif

}


static noinline void TestCharsets()
//---------------------------------
{

	// MPT_UTF8 version

	// Charset conversions (basic sanity checks)
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetUTF8, MPT_USTRING("a")), "a");
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetISO8859_1, MPT_USTRING("a")), "a");
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetASCII, MPT_USTRING("a")), "a");
	VERIFY_EQUAL(mpt::ToUnicode(mpt::CharsetUTF8, "a"), MPT_USTRING("a"));
	VERIFY_EQUAL(mpt::ToUnicode(mpt::CharsetISO8859_1, "a"), MPT_USTRING("a"));
	VERIFY_EQUAL(mpt::ToUnicode(mpt::CharsetASCII, "a"), MPT_USTRING("a"));
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(mpt::ToLocale(MPT_USTRING("a")), "a");
	VERIFY_EQUAL(mpt::ToUnicode(mpt::CharsetLocale, "a"), MPT_USTRING("a"));
#endif
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetUTF8, MPT_UTF8("a")), "a");
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetISO8859_1, MPT_UTF8("a")), "a");
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetASCII, MPT_UTF8("a")), "a");
	VERIFY_EQUAL(mpt::ToUnicode(mpt::CharsetUTF8, "a"), MPT_UTF8("a"));
	VERIFY_EQUAL(mpt::ToUnicode(mpt::CharsetISO8859_1, "a"), MPT_UTF8("a"));
	VERIFY_EQUAL(mpt::ToUnicode(mpt::CharsetASCII, "a"), MPT_UTF8("a"));
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(mpt::ToLocale(MPT_UTF8("a")), "a");
	VERIFY_EQUAL(mpt::ToUnicode(mpt::CharsetLocale, "a"), MPT_UTF8("a"));
#endif

	// Check that some character replacement is done (and not just empty strings or truncated strings are returned)
	// We test german umlaut-a (U+00E4) (\xC3\xA4) and CJK U+5BB6 (\xE5\xAE\xB6)

	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetASCII,MPT_UTF8("abc\xC3\xA4xyz")),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetISO8859_1,MPT_UTF8("abc\xC3\xA4xyz")),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetCP437,MPT_UTF8("abc\xC3\xA4xyz")),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetUTF8,MPT_UTF8("abc\xC3\xA4xyz")),"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetASCII,MPT_UTF8("abc\xC3\xA4xyz")),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetISO8859_1,MPT_UTF8("abc\xC3\xA4xyz")),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetCP437,MPT_UTF8("abc\xC3\xA4xyz")),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetUTF8,MPT_UTF8("abc\xC3\xA4xyz")),"abc"),true);
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetLocale,MPT_UTF8("abc\xC3\xA4xyz")),"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetLocale,MPT_UTF8("abc\xC3\xA4xyz")),"abc"),true);
#endif

	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetASCII,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetISO8859_1,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetCP437,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetUTF8,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetASCII,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetISO8859_1,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetCP437,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetUTF8,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"abc"),true);
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetLocale,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetLocale,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"abc"),true);
#endif

	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetASCII,"abc\xC3\xA4xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetISO8859_1,"abc\xC3\xA4xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetCP437,"abc\xC3\xA4xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetUTF8,"abc\xC3\xA4xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetASCII,"abc\xC3\xA4xyz"),MPT_USTRING("abc")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetISO8859_1,"abc\xC3\xA4xyz"),MPT_USTRING("abc")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetCP437,"abc\xC3\xA4xyz"),MPT_USTRING("abc")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetUTF8,"abc\xC3\xA4xyz"),MPT_USTRING("abc")),true);
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetLocale,"abc\xC3\xA4xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetLocale,"abc\xC3\xA4xyz"),MPT_USTRING("abc")),true);
#endif

	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetASCII,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetISO8859_1,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetCP437,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetUTF8,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetASCII,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("abc")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetISO8859_1,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("abc")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetCP437,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("abc")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetUTF8,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("abc")),true);
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(EndsWith(mpt::ToUnicode(mpt::CharsetLocale,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("xyz")),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToUnicode(mpt::CharsetLocale,"abc\xE5\xAE\xB6xyz"),MPT_USTRING("abc")),true);
#endif

	// Check that characters are correctly converted
	// We test german umlaut-a (U+00E4) and CJK U+5BB6

	// cp437
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetCP437,MPT_UTF8("abc\xC3\xA4xyz")),"abc\x84xyz");
	VERIFY_EQUAL(MPT_UTF8("abc\xC3\xA4xyz"),mpt::ToUnicode(mpt::CharsetCP437,"abc\x84xyz"));

	// iso8859
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetISO8859_1,MPT_UTF8("abc\xC3\xA4xyz")),"abc\xE4xyz");
	VERIFY_EQUAL(MPT_UTF8("abc\xC3\xA4xyz"),mpt::ToUnicode(mpt::CharsetISO8859_1,"abc\xE4xyz"));

	// utf8
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetUTF8,MPT_UTF8("abc\xC3\xA4xyz")),"abc\xC3\xA4xyz");
	VERIFY_EQUAL(MPT_UTF8("abc\xC3\xA4xyz"),mpt::ToUnicode(mpt::CharsetUTF8,"abc\xC3\xA4xyz"));
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetUTF8,MPT_UTF8("abc\xE5\xAE\xB6xyz")),"abc\xE5\xAE\xB6xyz");
	VERIFY_EQUAL(MPT_UTF8("abc\xE5\xAE\xB6xyz"),mpt::ToUnicode(mpt::CharsetUTF8,"abc\xE5\xAE\xB6xyz"));


#if MPT_WSTRING_CONVERT

	// wide L"" version

	// Charset conversions (basic sanity checks)
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetUTF8, L"a"), "a");
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetISO8859_1, L"a"), "a");
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetASCII, L"a"), "a");
	VERIFY_EQUAL(mpt::ToWide(mpt::CharsetUTF8, "a"), L"a");
	VERIFY_EQUAL(mpt::ToWide(mpt::CharsetISO8859_1, "a"), L"a");
	VERIFY_EQUAL(mpt::ToWide(mpt::CharsetASCII, "a"), L"a");
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(mpt::ToLocale(L"a"), "a");
	VERIFY_EQUAL(mpt::ToWide(mpt::CharsetLocale, "a"), L"a");
#endif

	// Check that some character replacement is done (and not just empty strings or truncated strings are returned)
	// We test german umlaut-a (U+00E4) and CJK U+5BB6

	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetASCII,L"abc\u00E4xyz"),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetISO8859_1,L"abc\u00E4xyz"),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetCP437,L"abc\u00E4xyz"),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetUTF8,L"abc\u00E4xyz"),"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetASCII,L"abc\u00E4xyz"),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetISO8859_1,L"abc\u00E4xyz"),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetCP437,L"abc\u00E4xyz"),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetUTF8,L"abc\u00E4xyz"),"abc"),true);
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetLocale,L"abc\u00E4xyz"),"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetLocale,L"abc\u00E4xyz"),"abc"),true);
#endif

	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetASCII,L"abc\u5BB6xyz"),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetISO8859_1,L"abc\u5BB6xyz"),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetCP437,L"abc\u5BB6xyz"),"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetUTF8,L"abc\u5BB6xyz"),"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetASCII,L"abc\u5BB6xyz"),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetISO8859_1,L"abc\u5BB6xyz"),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetCP437,L"abc\u5BB6xyz"),"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetUTF8,L"abc\u5BB6xyz"),"abc"),true);
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(EndsWith(mpt::ToCharset(mpt::CharsetLocale,L"abc\u5BB6xyz"),"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToCharset(mpt::CharsetLocale,L"abc\u5BB6xyz"),"abc"),true);
#endif

	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetASCII,"abc\xC3\xA4xyz"),L"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetISO8859_1,"abc\xC3\xA4xyz"),L"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetCP437,"abc\xC3\xA4xyz"),L"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetUTF8,"abc\xC3\xA4xyz"),L"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetASCII,"abc\xC3\xA4xyz"),L"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetISO8859_1,"abc\xC3\xA4xyz"),L"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetCP437,"abc\xC3\xA4xyz"),L"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetUTF8,"abc\xC3\xA4xyz"),L"abc"),true);
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetLocale,"abc\xC3\xA4xyz"),L"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetLocale,"abc\xC3\xA4xyz"),L"abc"),true);
#endif

	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetASCII,"abc\xE5\xAE\xB6xyz"),L"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetISO8859_1,"abc\xE5\xAE\xB6xyz"),L"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetCP437,"abc\xE5\xAE\xB6xyz"),L"xyz"),true);
	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetUTF8,"abc\xE5\xAE\xB6xyz"),L"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetASCII,"abc\xE5\xAE\xB6xyz"),L"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetISO8859_1,"abc\xE5\xAE\xB6xyz"),L"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetCP437,"abc\xE5\xAE\xB6xyz"),L"abc"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetUTF8,"abc\xE5\xAE\xB6xyz"),L"abc"),true);
#if defined(MPT_WITH_CHARSET_LOCALE)
	VERIFY_EQUAL(EndsWith(mpt::ToWide(mpt::CharsetLocale,"abc\xE5\xAE\xB6xyz"),L"xyz"),true);
	VERIFY_EQUAL(BeginsWith(mpt::ToWide(mpt::CharsetLocale,"abc\xE5\xAE\xB6xyz"),L"abc"),true);
#endif

	// Check that characters are correctly converted
	// We test german umlaut-a (U+00E4) and CJK U+5BB6

	// cp437
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetCP437,L"abc\u00E4xyz"),"abc\x84xyz");
	VERIFY_EQUAL(L"abc\u00E4xyz",mpt::ToWide(mpt::CharsetCP437,"abc\x84xyz"));

	// iso8859
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetISO8859_1,L"abc\u00E4xyz"),"abc\xE4xyz");
	VERIFY_EQUAL(L"abc\u00E4xyz",mpt::ToWide(mpt::CharsetISO8859_1,"abc\xE4xyz"));

	// utf8
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetUTF8,L"abc\u00E4xyz"),"abc\xC3\xA4xyz");
	VERIFY_EQUAL(L"abc\u00E4xyz",mpt::ToWide(mpt::CharsetUTF8,"abc\xC3\xA4xyz"));
	VERIFY_EQUAL(mpt::ToCharset(mpt::CharsetUTF8,L"abc\u5BB6xyz"),"abc\xE5\xAE\xB6xyz");
	VERIFY_EQUAL(L"abc\u5BB6xyz",mpt::ToWide(mpt::CharsetUTF8,"abc\xE5\xAE\xB6xyz"));

#endif


	// Path conversions
#ifdef MODPLUG_TRACKER
	const mpt::PathString exePath = MPT_PATHSTRING("C:\\OpenMPT\\");
	VERIFY_EQUAL(MPT_PATHSTRING("C:\\OpenMPT\\").AbsolutePathToRelative(exePath), MPT_PATHSTRING(".\\"));
	VERIFY_EQUAL(MPT_PATHSTRING("c:\\OpenMPT\\foo").AbsolutePathToRelative(exePath), MPT_PATHSTRING(".\\foo"));
	VERIFY_EQUAL(MPT_PATHSTRING("C:\\foo").AbsolutePathToRelative(exePath), MPT_PATHSTRING("\\foo"));
	VERIFY_EQUAL(MPT_PATHSTRING(".\\").RelativePathToAbsolute(exePath), MPT_PATHSTRING("C:\\OpenMPT\\"));
	VERIFY_EQUAL(MPT_PATHSTRING(".\\foo").RelativePathToAbsolute(exePath), MPT_PATHSTRING("C:\\OpenMPT\\foo"));
	VERIFY_EQUAL(MPT_PATHSTRING("\\foo").RelativePathToAbsolute(exePath), MPT_PATHSTRING("C:\\foo"));
	VERIFY_EQUAL(MPT_PATHSTRING("\\\\server\\path\\file").AbsolutePathToRelative(exePath), MPT_PATHSTRING("\\\\server\\path\\file"));
	VERIFY_EQUAL(MPT_PATHSTRING("\\\\server\\path\\file").RelativePathToAbsolute(exePath), MPT_PATHSTRING("\\\\server\\path\\file"));
#endif

}


#ifdef MODPLUG_TRACKER

struct CustomSettingsTestType
{
	float x;
	float y;
	CustomSettingsTestType(float x_ = 0.0f, float y_ = 0.0f) : x(x_), y(y_) { }
};

} // namespace Test

template <>
inline Test::CustomSettingsTestType FromSettingValue(const SettingValue &val)
{
	MPT_ASSERT(val.GetTypeTag() == "myType");
	std::string xy = val.as<std::string>();
	if(xy.empty())
	{
		return Test::CustomSettingsTestType(0.0f, 0.0f);
	}
	std::size_t pos = xy.find("|");
	std::string x = xy.substr(0, pos);
	std::string y = xy.substr(pos + 1);
	return Test::CustomSettingsTestType(ConvertStrTo<float>(x.c_str()), ConvertStrTo<float>(y.c_str()));
}

template <>
inline SettingValue ToSettingValue(const Test::CustomSettingsTestType &val)
{
	return SettingValue(Stringify(val.x) + "|" + Stringify(val.y), "myType");
}

namespace Test {

#endif // MODPLUG_TRACKER

static noinline void TestSettings()
//---------------------------------
{

#ifdef MODPLUG_TRACKER

	VERIFY_EQUAL(SettingPath("a","b") < SettingPath("a","c"), true);
	VERIFY_EQUAL(!(SettingPath("c","b") < SettingPath("a","c")), true);

	{
		DefaultSettingsContainer conf;

		int32 foobar = conf.Read("Test", "bar", 23, SettingMetadata("foobar"));
		conf.Write("Test", "bar", 64);
		conf.Write("Test", "bar", 42);
		conf.Read("Test", "baz", 4711);
		foobar = conf.Read("Test", "bar", 28);
	}

	{
		DefaultSettingsContainer conf;

		int32 foobar = conf.Read("Test", "bar", 28, SettingMetadata("foobar"));
		VERIFY_EQUAL(foobar, 42);
		conf.Write("Test", "bar", 43);
	}

	{
		DefaultSettingsContainer conf;

		int32 foobar = conf.Read("Test", "bar", 123, SettingMetadata("foobar"));
		VERIFY_EQUAL(foobar, 43);
		conf.Write("Test", "bar", 88);
	}

	{
		DefaultSettingsContainer conf;

		Setting<int> foo(conf, "Test", "bar", 99, SettingMetadata("something"));

		VERIFY_EQUAL(foo, 88);

		foo = 7;

	}

	{
		DefaultSettingsContainer conf;
		Setting<int> foo(conf, "Test", "bar", 99, SettingMetadata("something"));
		VERIFY_EQUAL(foo, 7);
	}


	{
		DefaultSettingsContainer conf;
		conf.Read("Test", "struct", std::string(""));
		conf.Write("Test", "struct", std::string(""));
	}

	{
		DefaultSettingsContainer conf;
		CustomSettingsTestType dummy = conf.Read("Test", "struct", CustomSettingsTestType(1.0f, 1.0f));
		dummy = CustomSettingsTestType(0.125f, 32.0f);
		conf.Write("Test", "struct", dummy);
	}

	{
		DefaultSettingsContainer conf;
		Setting<CustomSettingsTestType> dummyVar(conf, "Test", "struct", CustomSettingsTestType(1.0f, 1.0f));
		CustomSettingsTestType dummy = dummyVar;
		VERIFY_EQUAL(dummy.x, 0.125f);
		VERIFY_EQUAL(dummy.y, 32.0f);
	}

#endif // MODPLUG_TRACKER

}


// Test MIDI Event generating / reading
static noinline void TestMIDIEvents()
//-----------------------------------
{
	uint32 midiEvent;

	midiEvent = MIDIEvents::CC(MIDIEvents::MIDICC_Balance_Coarse, 13, 40);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evControllerChange);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 13);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), MIDIEvents::MIDICC_Balance_Coarse);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 40);

	midiEvent = MIDIEvents::NoteOn(10, 50, 120);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evNoteOn);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 10);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 50);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 120);

	midiEvent = MIDIEvents::NoteOff(15, 127, 42);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evNoteOff);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 15);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 42);

	midiEvent = MIDIEvents::ProgramChange(1, 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evProgramChange);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 1);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 127);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0);

	midiEvent = MIDIEvents::PitchBend(2, MIDIEvents::pitchBendCentre);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evPitchBend);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), 2);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 0x00);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0x40);

	midiEvent = MIDIEvents::System(MIDIEvents::sysStart);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetTypeFromEvent(midiEvent), MIDIEvents::evSystem);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetChannelFromEvent(midiEvent), MIDIEvents::sysStart);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte1FromEvent(midiEvent), 0);
	VERIFY_EQUAL_NONCONT(MIDIEvents::GetDataByte2FromEvent(midiEvent), 0);
}


// Check if our test file was loaded correctly.
static void TestLoadXMFile(const CSoundFile &sndFile)
//---------------------------------------------------
{
#ifdef MODPLUG_TRACKER
	const CModDoc *pModDoc = sndFile.GetpModDoc();
#endif // MODPLUG_TRACKER

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "Test Module");
	VERIFY_EQUAL_NONCONT(sndFile.songMessage.at(0), 'O');
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, 139);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 5);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultGlobalVolume, 128);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 42);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 23);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_EMBEDMIDICFG | SONG_LINEARSLIDES | SONG_EXFILTERRANGE);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_COMPATIBLE_PLAY), true);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_MIDICC_BUGEMULATION), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_OLDVOLSWING), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_OLD_MIDI_PITCHBENDS), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, tempo_mode_modern);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerBeat, 6);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerMeasure, 12);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwCreatedWithVersion, MAKE_VERSION_NUMERIC(1, 19, 02, 05));
	VERIFY_EQUAL_NONCONT(sndFile.m_nRestartPos, 1);

	// Macros
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(0), sfx_reso);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(1), sfx_drywet);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetFixedMacroType(), zxx_resomode);

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[0].szName, "First Channel"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nMixPlugin, 0);

	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[1].szName, "Second Channel"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nMixPlugin, 1);

	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 3);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[1], "Pulse Sample"), 0);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[2], "Empty Sample"), 0);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[3], "Unassigned Sample"), 0);
#ifdef MODPLUG_TRACKER
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(1), 1);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(2), 1);
	VERIFY_EQUAL_NONCONT(pModDoc->FindSampleParent(3), INSTRUMENTINDEX_INVALID);
#endif // MODPLUG_TRACKER
	const ModSample &sample = sndFile.GetSample(1);
	VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
	VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16);
	VERIFY_EQUAL_NONCONT(sample.nFineTune, 35);
	VERIFY_EQUAL_NONCONT(sample.RelativeTone, 1);
	VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
	VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
	VERIFY_EQUAL_NONCONT(sample.nPan, 160);
	VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_PANNING | CHN_LOOP | CHN_PINGPONGLOOP);

	VERIFY_EQUAL_NONCONT(sample.nLoopStart, 1);
	VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 8);

	VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SQUARE);
	VERIFY_EQUAL_NONCONT(sample.nVibSweep, 3);
	VERIFY_EQUAL_NONCONT(sample.nVibRate, 4);
	VERIFY_EQUAL_NONCONT(sample.nVibDepth, 5);

	// Sample Data
	for(size_t i = 0; i < 6; i++)
	{
		VERIFY_EQUAL_NONCONT(sample.pSample8[i], 18);
	}
	for(size_t i = 6; i < 16; i++)
	{
		VERIFY_EQUAL_NONCONT(sample.pSample8[i], 0);
	}

	// Instruments
	VERIFY_EQUAL_NONCONT(sndFile.GetNumInstruments(), 1);
	const ModInstrument *pIns = sndFile.Instruments[1];
	VERIFY_EQUAL_NONCONT(pIns->nFadeOut, 1024);
	VERIFY_EQUAL_NONCONT(pIns->nPan, 128);
	VERIFY_EQUAL_NONCONT(pIns->dwFlags, InstrumentFlags(0));

	VERIFY_EQUAL_NONCONT(pIns->nPPS, 0);
	VERIFY_EQUAL_NONCONT(pIns->nPPC, NOTE_MIDDLEC - 1);

	VERIFY_EQUAL_NONCONT(pIns->nVolRampUp, 1200);
	VERIFY_EQUAL_NONCONT(pIns->nResampling, (unsigned)SRCMODE_POLYPHASE);

	VERIFY_EQUAL_NONCONT(pIns->IsCutoffEnabled(), false);
	VERIFY_EQUAL_NONCONT(pIns->GetCutoff(), 0);
	VERIFY_EQUAL_NONCONT(pIns->IsResonanceEnabled(), false);
	VERIFY_EQUAL_NONCONT(pIns->GetResonance(), 0);
	VERIFY_EQUAL_NONCONT(pIns->nFilterMode, FLTMODE_UNCHANGED);

	VERIFY_EQUAL_NONCONT(pIns->nVolSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nPanSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nCutSwing, 0);
	VERIFY_EQUAL_NONCONT(pIns->nResSwing, 0);

	VERIFY_EQUAL_NONCONT(pIns->nNNA, NNA_NOTECUT);
	VERIFY_EQUAL_NONCONT(pIns->nDCT, DCT_NONE);

	VERIFY_EQUAL_NONCONT(pIns->nMixPlug, 1);
	VERIFY_EQUAL_NONCONT(pIns->nMidiChannel, 16);
	VERIFY_EQUAL_NONCONT(pIns->nMidiProgram, 64);
	VERIFY_EQUAL_NONCONT(pIns->wMidiBank, 2);
	VERIFY_EQUAL_NONCONT(pIns->midiPWD, 8);

	VERIFY_EQUAL_NONCONT(pIns->pTuning, sndFile.GetDefaultTuning());

	VERIFY_EQUAL_NONCONT(pIns->wPitchToTempoLock, 0);

	VERIFY_EQUAL_NONCONT(pIns->nPluginVelocityHandling, PLUGIN_VELOCITYHANDLING_VOLUME);
	VERIFY_EQUAL_NONCONT(pIns->nPluginVolumeHandling, PLUGIN_VOLUMEHANDLING_MIDI);

	for(size_t i = sndFile.GetModSpecifications().noteMin; i < sndFile.GetModSpecifications().noteMax; i++)
	{
		VERIFY_EQUAL_NONCONT(pIns->Keyboard[i], (i == NOTE_MIDDLEC - 1) ? 2 : 1);
	}

	VERIFY_EQUAL_NONCONT(pIns->VolEnv.dwFlags, ENV_ENABLED | ENV_SUSTAIN);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nNodes, 3);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.Ticks[2], 96);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.Values[2], 0);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nSustainStart, 1);
	VERIFY_EQUAL_NONCONT(pIns->VolEnv.nSustainEnd, 1);

	VERIFY_EQUAL_NONCONT(pIns->PanEnv.dwFlags, ENV_LOOP);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nNodes, 12);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopStart, 9);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopEnd, 11);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.Ticks[9], 46);
	VERIFY_EQUAL_NONCONT(pIns->PanEnv.Values[9], 23);

	VERIFY_EQUAL_NONCONT(pIns->PitchEnv.dwFlags, EnvelopeFlags(0));
	VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nNodes, 0);

	// Sequences
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetNumSequences(), 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order[0], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order[1], 1);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetName(), "First Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(0), true);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetName(), "Second Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 32);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumChannels(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->note, NOTE_NONE);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->instr, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->volcmd, VOLCMD_VIBRATOSPEED);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->vol, 15);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 0)->IsEmpty(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsEmpty(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->instr, 45);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->volcmd, VOLCMD_VOLSLIDEDOWN);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->vol, 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->param, 0xFF);

	// Test 4-Bit Panning conversion
	for(int i = 0; i < 16; i++)
	{
		VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(10 + i, 0)->vol, i * 4);
	}

	// Plugins
	const SNDMIXPLUGIN &plug = sndFile.m_MixPlugins[0];
	VERIFY_EQUAL_NONCONT(strcmp(plug.GetName(), "First Plugin"), 0);
	VERIFY_EQUAL_NONCONT(plug.fDryRatio, 0.26f);
	VERIFY_EQUAL_NONCONT(plug.IsMasterEffect(), true);
	VERIFY_EQUAL_NONCONT(plug.GetGain(), 11);
}


// Check if our test file was loaded correctly.
static void TestLoadMPTMFile(const CSoundFile &sndFile)
//-----------------------------------------------------
{

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "Test Module_____________X");
	VERIFY_EQUAL_NONCONT(sndFile.songMessage.at(0), 'O');
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, 139);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 5);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultGlobalVolume, 128);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 42);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 23);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_EMBEDMIDICFG | SONG_LINEARSLIDES | SONG_EXFILTERRANGE | SONG_ITCOMPATGXX | SONG_ITOLDEFFECTS);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_COMPATIBLE_PLAY), true);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_MIDICC_BUGEMULATION), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_OLDVOLSWING), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetModFlag(MSF_OLD_MIDI_PITCHBENDS), false);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, tempo_mode_modern);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerBeat, 6);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultRowsPerMeasure, 12);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwCreatedWithVersion, MAKE_VERSION_NUMERIC(1, 19, 02, 05));
	VERIFY_EQUAL_NONCONT(sndFile.m_nRestartPos, 1);

	// Edit history
	VERIFY_EQUAL_NONCONT(sndFile.GetFileHistory().size() > 0, true);
	const FileHistory &fh = sndFile.GetFileHistory().at(0);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_year, 111);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_mon, 5);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_mday, 14);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_hour, 21);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_min, 8);
	VERIFY_EQUAL_NONCONT(fh.loadDate.tm_sec, 32);
	VERIFY_EQUAL_NONCONT((uint32)((double)fh.openTime / HISTORY_TIMER_PRECISION), 31);

	// Macros
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(0), sfx_reso);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(1), sfx_drywet);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetParameteredMacroType(2), sfx_polyAT);
	VERIFY_EQUAL_NONCONT(sndFile.m_MidiCfg.GetFixedMacroType(), zxx_resomode);

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[0].szName, "First Channel"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nPan, 32);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nVolume, 32);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].dwFlags, CHN_MUTE);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nMixPlugin, 0);

	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[1].szName, "Second Channel"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nPan, 128);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nVolume, 16);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].dwFlags, CHN_SURROUND);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nMixPlugin, 1);

	VERIFY_EQUAL_NONCONT(strcmp(sndFile.ChnSettings[69].szName, "Last Channel______X"), 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nPan, 256);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nVolume, 7);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].dwFlags, ChannelFlags(0));
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[69].nMixPlugin, 1);
	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 4);
	{
		const ModSample &sample = sndFile.GetSample(1);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 9001);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 16);
		VERIFY_EQUAL_NONCONT(sample.nPan, 160);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_PANNING | CHN_LOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 1);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 8);
		VERIFY_EQUAL_NONCONT(sample.nSustainStart, 1);
		VERIFY_EQUAL_NONCONT(sample.nSustainEnd, 7);

		VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SQUARE);
		VERIFY_EQUAL_NONCONT(sample.nVibSweep, 3);
		VERIFY_EQUAL_NONCONT(sample.nVibRate, 4);
		VERIFY_EQUAL_NONCONT(sample.nVibDepth, 5);

		// Sample Data
		for(size_t i = 0; i < 6; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample8[i], 18);
		}
		for(size_t i = 6; i < 16; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample8[i], 0);
		}
	}

	{
		const ModSample &sample = sndFile.GetSample(2);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[2], "Stereo / 16-Bit"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 4);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 16 * 4);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 16000);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_16BIT | CHN_STEREO | CHN_LOOP);

		// Sample Data (Stereo Interleaved)
		for(size_t i = 0; i < 7; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample16[4 + i], int16(-32768));
		}
	}

	// External sample
	{
		const ModSample &sample = sndFile.GetSample(4);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[4], "Overridden Name"), 0);
		VERIFY_EQUAL_NONCONT(strcmp(sample.filename, "External"), 0);
#ifdef MPT_EXTERNAL_SAMPLES
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 64);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 42);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 55);
		VERIFY_EQUAL_NONCONT(sample.nSustainStart, 42);
		VERIFY_EQUAL_NONCONT(sample.nSustainEnd, 55);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP | CHN_SUSTAINLOOP | CHN_PINGPONGSUSTAIN | SMP_KEEPONDISK);
#endif // MPT_EXTERNAL_SAMPLES
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_MPT), 10101);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 26 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 26);
		VERIFY_EQUAL_NONCONT(sample.nPan, 26 * 4);

		VERIFY_EQUAL_NONCONT(sample.nVibType, VIB_SINE);
		VERIFY_EQUAL_NONCONT(sample.nVibSweep, 37);
		VERIFY_EQUAL_NONCONT(sample.nVibRate, 42);
		VERIFY_EQUAL_NONCONT(sample.nVibDepth, 23);

		// Sample Data
#ifdef MPT_EXTERNAL_SAMPLES
		for(size_t i = 0; i < 16; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample8[i], int8(45));
		}
#endif // MPT_EXTERNAL_SAMPLES
	}

	// Instruments
	VERIFY_EQUAL_NONCONT(sndFile.GetNumInstruments(), 2);
	for(INSTRUMENTINDEX ins = 1; ins <= 2; ins++)
	{
		const ModInstrument *pIns = sndFile.Instruments[ins];
		VERIFY_EQUAL_NONCONT(pIns->nGlobalVol, 32);
		VERIFY_EQUAL_NONCONT(pIns->nFadeOut, 1024);
		VERIFY_EQUAL_NONCONT(pIns->nPan, 64);
		VERIFY_EQUAL_NONCONT(pIns->dwFlags, INS_SETPANNING);

		VERIFY_EQUAL_NONCONT(pIns->nPPS, 16);
		VERIFY_EQUAL_NONCONT(pIns->nPPC, (NOTE_MIDDLEC - NOTE_MIN) + 6);	// F#5

		VERIFY_EQUAL_NONCONT(pIns->nVolRampUp, 1200);
		VERIFY_EQUAL_NONCONT(pIns->nResampling, (unsigned)SRCMODE_POLYPHASE);

		VERIFY_EQUAL_NONCONT(pIns->IsCutoffEnabled(), true);
		VERIFY_EQUAL_NONCONT(pIns->GetCutoff(), 0x32);
		VERIFY_EQUAL_NONCONT(pIns->IsResonanceEnabled(), true);
		VERIFY_EQUAL_NONCONT(pIns->GetResonance(), 0x64);
		VERIFY_EQUAL_NONCONT(pIns->nFilterMode, FLTMODE_HIGHPASS);

		VERIFY_EQUAL_NONCONT(pIns->nVolSwing, 0x30);
		VERIFY_EQUAL_NONCONT(pIns->nPanSwing, 0x18);
		VERIFY_EQUAL_NONCONT(pIns->nCutSwing, 0x0C);
		VERIFY_EQUAL_NONCONT(pIns->nResSwing, 0x3C);

		VERIFY_EQUAL_NONCONT(pIns->nNNA, NNA_CONTINUE);
		VERIFY_EQUAL_NONCONT(pIns->nDCT, DCT_NOTE);
		VERIFY_EQUAL_NONCONT(pIns->nDNA, DNA_NOTEFADE);

		VERIFY_EQUAL_NONCONT(pIns->nMixPlug, 1);
		VERIFY_EQUAL_NONCONT(pIns->nMidiChannel, 16);
		VERIFY_EQUAL_NONCONT(pIns->nMidiProgram, 64);
		VERIFY_EQUAL_NONCONT(pIns->wMidiBank, 2);
		VERIFY_EQUAL_NONCONT(pIns->midiPWD, ins);

		VERIFY_EQUAL_NONCONT(pIns->pTuning, sndFile.GetDefaultTuning());

		VERIFY_EQUAL_NONCONT(pIns->wPitchToTempoLock, 130);

		VERIFY_EQUAL_NONCONT(pIns->nPluginVelocityHandling, PLUGIN_VELOCITYHANDLING_VOLUME);
		VERIFY_EQUAL_NONCONT(pIns->nPluginVolumeHandling, PLUGIN_VOLUMEHANDLING_MIDI);

		for(size_t i = 0; i < NOTE_MAX; i++)
		{
			VERIFY_EQUAL_NONCONT(pIns->Keyboard[i], (i == NOTE_MIDDLEC - 1) ? 99 : 1);
			VERIFY_EQUAL_NONCONT(pIns->NoteMap[i], (i == NOTE_MIDDLEC - 1) ? (i + 13) : (i + 1));
		}

		VERIFY_EQUAL_NONCONT(pIns->VolEnv.dwFlags, ENV_ENABLED | ENV_CARRY);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.nNodes, 3);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.nReleaseNode, 1);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.Ticks[2], 96);
		VERIFY_EQUAL_NONCONT(pIns->VolEnv.Values[2], 0);

		VERIFY_EQUAL_NONCONT(pIns->PanEnv.dwFlags, ENV_LOOP);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nNodes, 76);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopStart, 22);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nLoopEnd, 29);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.nReleaseNode, ENV_RELEASE_NODE_UNSET);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.Ticks[75], 427);
		VERIFY_EQUAL_NONCONT(pIns->PanEnv.Values[75], 27);

		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.dwFlags, ENV_ENABLED | ENV_CARRY | ENV_SUSTAIN | ENV_FILTER);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nNodes, 3);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nSustainStart, 1);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.nSustainEnd, 2);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.Ticks[1], 96);
		VERIFY_EQUAL_NONCONT(pIns->PitchEnv.Values[1], 64);
	}
	// Sequences
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetNumSequences(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0).GetLengthTailTrimmed(), 3);
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0).GetName(), "First Sequence");
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0)[0], sndFile.Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0)[1], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(0)[2], sndFile.Order.GetIgnoreIndex());

	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(1).GetLengthTailTrimmed(), 2);
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(1).GetName(), "Second Sequence");
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(1)[0], 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetSequence(1)[1], 2);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetName(), "First Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerBeat(), 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetRowsPerMeasure(), 10);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(0), true);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetName(), "Second Pattern");
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 32);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumChannels(), 70);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerBeat(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetRowsPerMeasure(), 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->IsPcNote(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->note, NOTE_PC);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->instr, 99);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->GetValueVolCol(), 1);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(0, 0)->GetValueEffectCol(), 200);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 0)->IsEmpty(), true);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsEmpty(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->IsPcNote(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->note, NOTE_MIDDLEC + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->instr, 45);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->volcmd, VOLCMD_VOLSLIDEDOWN);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->vol, 5);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->command, CMD_PANNING8);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(31, 1)->param, 0xFF);

	// Plugins
	const SNDMIXPLUGIN &plug = sndFile.m_MixPlugins[0];
	VERIFY_EQUAL_NONCONT(strcmp(plug.GetName(), "First Plugin"), 0);
	VERIFY_EQUAL_NONCONT(plug.fDryRatio, 0.26f);
	VERIFY_EQUAL_NONCONT(plug.IsMasterEffect(), true);
	VERIFY_EQUAL_NONCONT(plug.GetGain(), 11);

#ifdef MODPLUG_TRACKER
	// MIDI Mapping
	VERIFY_EQUAL_NONCONT(sndFile.GetMIDIMapper().GetCount(), 1);
	const CMIDIMappingDirective &mapping = sndFile.GetMIDIMapper().GetDirective(0);
	VERIFY_EQUAL_NONCONT(mapping.GetAllowPatternEdit(), true);
	VERIFY_EQUAL_NONCONT(mapping.GetCaptureMIDI(), false);
	VERIFY_EQUAL_NONCONT(mapping.IsActive(), true);
	VERIFY_EQUAL_NONCONT(mapping.GetAnyChannel(), false);
	VERIFY_EQUAL_NONCONT(mapping.GetChannel(), 5);
	VERIFY_EQUAL_NONCONT(mapping.GetPlugIndex(), 1);
	VERIFY_EQUAL_NONCONT(mapping.GetParamIndex(), 0);
	VERIFY_EQUAL_NONCONT(mapping.GetEvent(), MIDIEvents::evControllerChange);
	VERIFY_EQUAL_NONCONT(mapping.GetController(), MIDIEvents::MIDICC_ModulationWheel_Coarse);
#endif

}


// Check if our test file was loaded correctly.
static void TestLoadS3MFile(const CSoundFile &sndFile, bool resaved)
//------------------------------------------------------------------
{

	// Global Variables
	VERIFY_EQUAL_NONCONT(sndFile.GetTitle(), "S3M_Test__________________X");
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultTempo, 33);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultSpeed, 254);
	VERIFY_EQUAL_NONCONT(sndFile.m_nDefaultGlobalVolume, 32 * 4);
	VERIFY_EQUAL_NONCONT(sndFile.m_nVSTiVolume, 48);
	VERIFY_EQUAL_NONCONT(sndFile.m_nSamplePreAmp, 16);
	VERIFY_EQUAL_NONCONT((sndFile.m_SongFlags & SONG_FILE_FLAGS), SONG_FASTVOLSLIDES);
	VERIFY_EQUAL_NONCONT(sndFile.GetMixLevels(), mixLevels_compatible);
	VERIFY_EQUAL_NONCONT(sndFile.m_nTempoMode, tempo_mode_classic);
	VERIFY_EQUAL_NONCONT(sndFile.m_dwLastSavedWithVersion, resaved ? (MptVersion::num & 0xFFFF0000) : MAKE_VERSION_NUMERIC(1, 20, 00, 00));
	VERIFY_EQUAL_NONCONT(sndFile.m_nRestartPos, 0);

	// Channels
	VERIFY_EQUAL_NONCONT(sndFile.GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].nPan, 0);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[0].dwFlags, ChannelFlags(0));

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].nPan, 256);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[1].dwFlags, CHN_MUTE);

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[2].nPan, 85);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[2].dwFlags, ChannelFlags(0));

	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[3].nPan, 171);
	VERIFY_EQUAL_NONCONT(sndFile.ChnSettings[3].dwFlags, CHN_MUTE);

	// Samples
	VERIFY_EQUAL_NONCONT(sndFile.GetNumSamples(), 3);
	{
		const ModSample &sample = sndFile.GetSample(1);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[1], "Sample_1__________________X"), 0);
		VERIFY_EQUAL_NONCONT(strcmp(sample.filename, "Filename_1_X"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 1);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 60);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 9001);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 32 * 4);
		VERIFY_EQUAL_NONCONT(sample.nGlobalVol, 64);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 16);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 60);

		// Sample Data
		for(size_t i = 0; i < 30; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample8[i], 127);
		}
		for(size_t i = 31; i < 60; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample8[i], -128);
		}
	}

	{
		const ModSample &sample = sndFile.GetSample(2);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[2], "Empty"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 16384);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 2 * 4);
	}

	{
		const ModSample &sample = sndFile.GetSample(3);
		VERIFY_EQUAL_NONCONT(strcmp(sndFile.m_szNames[3], "Stereo / 16-Bit"), 0);
		VERIFY_EQUAL_NONCONT(strcmp(sample.filename, "Filename_3_X"), 0);
		VERIFY_EQUAL_NONCONT(sample.GetBytesPerSample(), 4);
		VERIFY_EQUAL_NONCONT(sample.GetNumChannels(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetElementarySampleSize(), 2);
		VERIFY_EQUAL_NONCONT(sample.GetSampleSizeInBytes(), 64);
		VERIFY_EQUAL_NONCONT(sample.GetSampleRate(MOD_TYPE_S3M), 16000);
		VERIFY_EQUAL_NONCONT(sample.nVolume, 0);
		VERIFY_EQUAL_NONCONT(sample.uFlags, CHN_LOOP | CHN_16BIT | CHN_STEREO);

		VERIFY_EQUAL_NONCONT(sample.nLoopStart, 0);
		VERIFY_EQUAL_NONCONT(sample.nLoopEnd, 16);

		// Sample Data (Stereo Interleaved)
		for(size_t i = 0; i < 7; i++)
		{
			VERIFY_EQUAL_NONCONT(sample.pSample16[4 + i], int16(-32768));
		}
	}

	// Orders
	VERIFY_EQUAL_NONCONT(sndFile.Order.GetLengthTailTrimmed(), 5);
	VERIFY_EQUAL_NONCONT(sndFile.Order[0], 0);
	VERIFY_EQUAL_NONCONT(sndFile.Order[1], sndFile.Order.GetIgnoreIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order[2], sndFile.Order.GetInvalidPatIndex());
	VERIFY_EQUAL_NONCONT(sndFile.Order[3], 1);
	VERIFY_EQUAL_NONCONT(sndFile.Order[4], 0);

	// Patterns
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.GetNumPatterns(), 2);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetNumChannels(), 4);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetOverrideSignature(), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 0)->note, NOTE_MIN + 12);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 0)->note, NOTE_MIN + 107);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 1)->volcmd, VOLCMD_VOLUME);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 1)->vol, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 1)->volcmd, VOLCMD_VOLUME);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(1, 1)->vol, 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(2, 1)->volcmd, VOLCMD_PANNING);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(2, 1)->vol, 0);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(3, 1)->volcmd, VOLCMD_PANNING);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(3, 1)->vol, 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 3)->command, CMD_SPEED);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[0].GetpModCommand(0, 3)->param, 0x11);

	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns.IsPatternEmpty(1), false);
	VERIFY_EQUAL_NONCONT(sndFile.Patterns[1].GetpModCommand(63, 3)->param, 0x04);
}



#ifdef MODPLUG_TRACKER

static const char * debugPaths [] = { "mptrack\\Debug\\", "bin\\Win32-Debug\\", "bin\\x64-Debug\\" };

static bool PathEndsIn(const mpt::PathString &path_, const mpt::PathString &match_)
{
	std::wstring path = path_.ToWide();
	std::wstring match = match_.ToWide();
	return path.rfind(match) == (path.length() - match.length());
}

static bool ShouldRunTests()
{
	mpt::PathString theFile = theApp.GetAppDirPath();
	// Only run the tests when we're in the project directory structure.
	for(std::size_t i = 0; i < CountOf(debugPaths); ++i)
	{
		const mpt::PathString debugPath = mpt::PathString::FromUTF8(debugPaths[i]);
		if(PathEndsIn(theFile, debugPath))
		{
			return true;
		}
	}
	return false;
}

static mpt::PathString GetTestFilenameBase()
{
	mpt::PathString theFile = theApp.GetAppDirPath();
	for(std::size_t i = 0; i < CountOf(debugPaths); ++i)
	{
		const mpt::PathString debugPath = mpt::PathString::FromUTF8(debugPaths[i]);
		if(PathEndsIn(theFile, debugPath))
		{
			theFile = mpt::PathString::FromWide(theFile.ToWide().substr(0, theFile.ToWide().length() - debugPath.ToWide().length()));
			break;
		}
	}
	theFile += MPT_PATHSTRING("test/test.");
	return theFile;
}

static mpt::PathString GetTempFilenameBase()
{
	return GetTestFilenameBase();
}

typedef CModDoc *TSoundFileContainer;

static CSoundFile &GetrSoundFile(TSoundFileContainer &sndFile)
{
	return sndFile->GetrSoundFile();
}


static TSoundFileContainer CreateSoundFileContainer(const mpt::PathString &filename)
{
	CModDoc *pModDoc = (CModDoc *)theApp.OpenDocumentFile(filename, FALSE);
	return pModDoc;
}

static void DestroySoundFileContainer(TSoundFileContainer &sndFile)
{
	sndFile->OnCloseDocument();
}

static void SaveIT(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	sndFile->DoSave(filename);
	// Saving the file puts it in the MRU list...
	theApp.RemoveMruItem(0);
}

static void SaveXM(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	sndFile->DoSave(filename);
	// Saving the file puts it in the MRU list...
	theApp.RemoveMruItem(0);
}

static void SaveS3M(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	sndFile->DoSave(filename);
	// Saving the file puts it in the MRU list...
	theApp.RemoveMruItem(0);
}

#else

static bool ShouldRunTests()
{
	return true;
}

static mpt::PathString GetTestFilenameBase()
{
	return Test::GetPathPrefix() + MPT_PATHSTRING("./test/test.");
}

static mpt::PathString GetTempFilenameBase()
{
	return MPT_PATHSTRING("./test.");
}

typedef MPT_SHARED_PTR<CSoundFile> TSoundFileContainer;

static CSoundFile &GetrSoundFile(TSoundFileContainer &sndFile)
{
	return *sndFile.get();
}

static TSoundFileContainer CreateSoundFileContainer(const mpt::PathString &filename)
{
	mpt::ifstream stream(filename, std::ios::binary);
	FileReader file(&stream);
	MPT_SHARED_PTR<CSoundFile> pSndFile = mpt::make_shared<CSoundFile>();
	pSndFile->Create(file, CSoundFile::loadCompleteModule);
	return pSndFile;
}

static void DestroySoundFileContainer(TSoundFileContainer & /* sndFile */ )
{
	return;
}

#ifndef MODPLUG_NO_FILESAVE

static void SaveIT(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	sndFile->SaveIT(filename, false);
}

static void SaveXM(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	sndFile->SaveXM(filename, false);
}

static void SaveS3M(const TSoundFileContainer &sndFile, const mpt::PathString &filename)
{
	sndFile->SaveS3M(filename);
}

#endif

#endif



// Test file loading and saving
static noinline void TestLoadSaveFile()
//-------------------------------------
{
	if(!ShouldRunTests())
	{
		return;
	}
	mpt::PathString filenameBaseSrc = GetTestFilenameBase();
	mpt::PathString filenameBase = GetTempFilenameBase();

	// Test MPTM file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBaseSrc + MPT_PATHSTRING("mptm"));

		TestLoadMPTMFile(GetrSoundFile(sndFileContainer));

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			SaveIT(sndFileContainer, filenameBase + MPT_PATHSTRING("saved.mptm"));
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + MPT_PATHSTRING("saved.mptm"));

		TestLoadMPTMFile(GetrSoundFile(sndFileContainer));

		DestroySoundFileContainer(sndFileContainer);

		RemoveFile(filenameBase + MPT_PATHSTRING("saved.mptm"));
	}
	#endif

	// Test XM file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBaseSrc + MPT_PATHSTRING("xm"));

		TestLoadXMFile(GetrSoundFile(sndFileContainer));

		// In OpenMPT 1.20 (up to revision 1459), there was a bug in the XM saver
		// that would create broken XMs if the sample map contained samples that
		// were only referenced below C-1 or above B-8 (such samples should not
		// be written). Let's insert a sample there and check if re-loading the
		// file still works.
		GetrSoundFile(sndFileContainer).m_nSamples++;
		GetrSoundFile(sndFileContainer).Instruments[1]->Keyboard[110] = GetrSoundFile(sndFileContainer).GetNumSamples();

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			SaveXM(sndFileContainer, filenameBase + MPT_PATHSTRING("saved.xm"));
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + MPT_PATHSTRING("saved.xm"));

		TestLoadXMFile(GetrSoundFile(sndFileContainer));

		DestroySoundFileContainer(sndFileContainer);

		RemoveFile(filenameBase + MPT_PATHSTRING("saved.xm"));
	}
	#endif

	// Test S3M file loading
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBaseSrc + MPT_PATHSTRING("s3m"));

		TestLoadS3MFile(GetrSoundFile(sndFileContainer), false);

		#ifndef MODPLUG_NO_FILESAVE
			// Test file saving
			SaveS3M(sndFileContainer, filenameBase + MPT_PATHSTRING("saved.s3m"));
		#endif

		DestroySoundFileContainer(sndFileContainer);
	}

	// Reload the saved file and test if everything is still working correctly.
	#ifndef MODPLUG_NO_FILESAVE
	{
		TSoundFileContainer sndFileContainer = CreateSoundFileContainer(filenameBase + MPT_PATHSTRING("saved.s3m"));

		TestLoadS3MFile(GetrSoundFile(sndFileContainer), true);

		DestroySoundFileContainer(sndFileContainer);

		RemoveFile(filenameBase + MPT_PATHSTRING("saved.s3m"));
	}
	#endif

	// General file I/O tests
	{
		std::ostringstream f;
		size_t bytesWritten;
		mpt::IO::WriteVarInt(f, uint16(0), &bytesWritten);		VERIFY_EQUAL_NONCONT(bytesWritten, 1);
		mpt::IO::WriteVarInt(f, uint16(127), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 1);
		mpt::IO::WriteVarInt(f, uint16(128), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 2);
		mpt::IO::WriteVarInt(f, uint16(16383), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 2);
		mpt::IO::WriteVarInt(f, uint16(16384), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 3);
		mpt::IO::WriteVarInt(f, uint16(65535), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 3);
		mpt::IO::WriteVarInt(f, uint64(0xFFFFFFFFFFFFFFFFull), &bytesWritten);	VERIFY_EQUAL_NONCONT(bytesWritten, 10);
		std::string data = f.str();
		FileReader file(&data[0], data.size());
		uint64 v;
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 0);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 127);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 128);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 16383);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 16384);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 65535);
		file.ReadVarInt(v); VERIFY_EQUAL_NONCONT(v, 0xFFFFFFFFFFFFFFFFull);
	}
}


static void RunITCompressionTest(const std::vector<int8> &sampleData, FlagSet<ChannelFlags> smpFormat, bool it215)
//----------------------------------------------------------------------------------------------------------------
{

	ModSample smp;
	smp.uFlags = smpFormat;
	smp.pSample = const_cast<int8 *>(&sampleData[0]);
	smp.nLength = sampleData.size() / smp.GetBytesPerSample();

	std::string data;

	{
		std::ostringstream f;
		ITCompression compression(smp, it215, &f);
		data = f.str();
	}

	{
		FileReader file(&data[0], data.length());

		std::vector<int8> sampleDataNew(sampleData.size(), 0);
		smp.pSample = &sampleDataNew[0];

		ITDecompression decompression(file, smp, it215);
		VERIFY_EQUAL_NONCONT(memcmp(&sampleData[0], &sampleDataNew[0], sampleData.size()), 0);
	}
}


static noinline void TestITCompression()
//--------------------------------------
{
	// Test loading / saving of IT-compressed samples
	const int sampleDataSize = 65536;
	std::vector<int8> sampleData(sampleDataSize, 0);
	std::srand(0);
	for(int i = 0; i < sampleDataSize; i++)
	{
		sampleData[i] = (int8)std::rand();
	}

	// Run each compression test with IT215 compression and without.
	for(int i = 0; i < 2; i++)
	{
		RunITCompressionTest(sampleData, ChannelFlags(0), i == 0);
		RunITCompressionTest(sampleData, CHN_16BIT, i == 0);
		RunITCompressionTest(sampleData, CHN_STEREO, i == 0);
		RunITCompressionTest(sampleData, CHN_16BIT | CHN_STEREO, i == 0);
	}
}


static double Rand01() {return rand() / double(RAND_MAX);}
template <class T>
T Rand(const T min, const T max) {return Util::Round<T>(min + Rand01() * (max - min));}



static void GenerateCommands(CPattern& pat, const double dProbPcs, const double dProbPc)
//--------------------------------------------------------------------------------------
{
	const double dPcxProb = dProbPcs + dProbPc;
	const CPattern::const_iterator end = pat.End();
	for(CPattern::iterator i = pat.Begin(); i != end; i++)
	{
		const double rand = Rand01();
		if(rand < dPcxProb)
		{
			if(rand < dProbPcs)
				i->note = NOTE_PCS;
			else
				i->note = NOTE_PC;

			i->instr = Rand<ModCommand::INSTR>(0, MAX_MIXPLUGINS);
			i->SetValueVolCol(Rand<uint16>(0, ModCommand::maxColumnValue));
			i->SetValueEffectCol(Rand<uint16>(0, ModCommand::maxColumnValue));
		}
		else
			i->Clear();
	}
}


// Test PC note serialization
static noinline void TestPCnoteSerialization()
//--------------------------------------------
{
	FileReader file;
	MPT_SHARED_PTR<CSoundFile> pSndFile = mpt::make_shared<CSoundFile>();
	CSoundFile &sndFile = *pSndFile.get();
	sndFile.ChangeModTypeTo(MOD_TYPE_MPT);
	sndFile.Patterns.DestroyPatterns();
	sndFile.m_nChannels = ModSpecs::mptm.channelsMax;

	sndFile.Patterns.Insert(0, ModSpecs::mptm.patternRowsMin);
	sndFile.Patterns.Insert(1, 64);
	GenerateCommands(sndFile.Patterns[1], 0.3, 0.3);
	sndFile.Patterns.Insert(2, ModSpecs::mptm.patternRowsMax);
	GenerateCommands(sndFile.Patterns[2], 0.5, 0.5);

	//
	std::vector<ModCommand> pat[3];
	const size_t numCommands[] = {	sndFile.GetNumChannels() * sndFile.Patterns[0].GetNumRows(),
									sndFile.GetNumChannels() * sndFile.Patterns[1].GetNumRows(),
									sndFile.GetNumChannels() * sndFile.Patterns[2].GetNumRows()
								 };
	pat[0].resize(numCommands[0]);
	pat[1].resize(numCommands[1]);
	pat[2].resize(numCommands[2]);

	for(int i = 0; i < 3; i++) // Copy pattern data for comparison.
	{
		CPattern::const_iterator iter = sndFile.Patterns[i].Begin();
		for(size_t j = 0; j < numCommands[i]; j++, iter++) pat[i][j] = *iter;
	}

	std::stringstream mem;
	WriteModPatterns(mem, sndFile.Patterns);

	VERIFY_EQUAL_NONCONT( mem.good(), true );

	// Clear patterns.
	sndFile.Patterns[0].ClearCommands();
	sndFile.Patterns[1].ClearCommands();
	sndFile.Patterns[2].ClearCommands();

	// Read data back.
	ReadModPatterns(mem, sndFile.Patterns);

	// Compare.
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[0].GetNumRows(), ModSpecs::mptm.patternRowsMin);
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[1].GetNumRows(), 64);
	VERIFY_EQUAL_NONCONT( sndFile.Patterns[2].GetNumRows(), ModSpecs::mptm.patternRowsMax);
	for(int i = 0; i < 3; i++)
	{
		bool bPatternDataMatch = true;
		CPattern::const_iterator iter = sndFile.Patterns[i].Begin();
		for(size_t j = 0; j < numCommands[i]; j++, iter++)
		{
			if(pat[i][j] != *iter)
			{
				bPatternDataMatch = false;
				break;
			}
		}
		VERIFY_EQUAL( bPatternDataMatch, true);
	}
}


// Test String I/O functionality
static noinline void TestStringIO()
//---------------------------------
{
	char src0[4] = { '\0', 'X', ' ', 'X' };		// Weird empty buffer
	char src1[4] = { 'X', ' ', '\0', 'X' };		// Weird buffer (hello Impulse Tracker)
	char src2[4] = { 'X', 'Y', 'Z', ' ' };		// Full buffer, last character space
	char src3[4] = { 'X', 'Y', 'Z', '!' };		// Full buffer, last character non-space
	char dst1[6];	// Destination buffer, larger than source buffer
	char dst2[3];	// Destination buffer, smaller than source buffer

#define ReadTest(mode, dst, src, expectedResult) \
	mpt::String::Read<mpt::String:: mode >(dst, src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, CountOf(dst)), 0); /* Ensure that the strings are identical */ \
	for(size_t i = strlen(dst); i < CountOf(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */

#define WriteTest(mode, dst, src, expectedResult) \
	mpt::String::Write<mpt::String:: mode >(dst, src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, CountOf(dst)), 0);  /* Ensure that the strings are identical */ \
	for(size_t i = mpt::strnlen(dst, CountOf(dst)); i < CountOf(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */

	// Check reading of null-terminated string into larger buffer
	ReadTest(nullTerminated, dst1, src0, "");
	ReadTest(nullTerminated, dst1, src1, "X ");
	ReadTest(nullTerminated, dst1, src2, "XYZ");
	ReadTest(nullTerminated, dst1, src3, "XYZ");

	// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
	ReadTest(maybeNullTerminated, dst1, src0, "");
	ReadTest(maybeNullTerminated, dst1, src1, "X ");
	ReadTest(maybeNullTerminated, dst1, src2, "XYZ ");
	ReadTest(maybeNullTerminated, dst1, src3, "XYZ!");

	// Check reading of space-padded strings with ignored last character
	ReadTest(spacePaddedNull, dst1, src0, " X");
	ReadTest(spacePaddedNull, dst1, src1, "X");
	ReadTest(spacePaddedNull, dst1, src2, "XYZ");
	ReadTest(spacePaddedNull, dst1, src3, "XYZ");

	// Check reading of space-padded strings
	ReadTest(spacePadded, dst1, src0, " X X");
	ReadTest(spacePadded, dst1, src1, "X  X");
	ReadTest(spacePadded, dst1, src2, "XYZ");
	ReadTest(spacePadded, dst1, src3, "XYZ!");

	///////////////////////////////

	// Check reading of null-terminated string into smaller buffer
	ReadTest(nullTerminated, dst2, src0, "");
	ReadTest(nullTerminated, dst2, src1, "X ");
	ReadTest(nullTerminated, dst2, src2, "XY");
	ReadTest(nullTerminated, dst2, src3, "XY");

	// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
	ReadTest(maybeNullTerminated, dst2, src0, "");
	ReadTest(maybeNullTerminated, dst2, src1, "X ");
	ReadTest(maybeNullTerminated, dst2, src2, "XY");
	ReadTest(maybeNullTerminated, dst2, src3, "XY");

	// Check reading of space-padded strings with ignored last character
	ReadTest(spacePaddedNull, dst2, src0, " X");
	ReadTest(spacePaddedNull, dst2, src1, "X");
	ReadTest(spacePaddedNull, dst2, src2, "XY");
	ReadTest(spacePaddedNull, dst2, src3, "XY");

	// Check reading of space-padded strings
	ReadTest(spacePadded, dst2, src0, " X");
	ReadTest(spacePadded, dst2, src1, "X");
	ReadTest(spacePadded, dst2, src2, "XY");
	ReadTest(spacePadded, dst2, src3, "XY");

	///////////////////////////////

	// Check writing of null-terminated string into larger buffer
	WriteTest(nullTerminated, dst1, src0, "");
	WriteTest(nullTerminated, dst1, src1, "X ");
	WriteTest(nullTerminated, dst1, src2, "XYZ ");
	WriteTest(nullTerminated, dst1, src3, "XYZ!");

	// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
	WriteTest(maybeNullTerminated, dst1, src0, "");
	WriteTest(maybeNullTerminated, dst1, src1, "X ");
	WriteTest(maybeNullTerminated, dst1, src2, "XYZ ");
	WriteTest(maybeNullTerminated, dst1, src3, "XYZ!");

	// Check writing of space-padded strings with last character set to null
	WriteTest(spacePaddedNull, dst1, src0, "     ");
	WriteTest(spacePaddedNull, dst1, src1, "X    ");
	WriteTest(spacePaddedNull, dst1, src2, "XYZ  ");
	WriteTest(spacePaddedNull, dst1, src3, "XYZ! ");

	// Check writing of space-padded strings
	WriteTest(spacePadded, dst1, src0, "      ");
	WriteTest(spacePadded, dst1, src1, "X     ");
	WriteTest(spacePadded, dst1, src2, "XYZ   ");
	WriteTest(spacePadded, dst1, src3, "XYZ!  ");

	///////////////////////////////

	// Check writing of null-terminated string into smaller buffer
	WriteTest(nullTerminated, dst2, src0, "");
	WriteTest(nullTerminated, dst2, src1, "X ");
	WriteTest(nullTerminated, dst2, src2, "XY");
	WriteTest(nullTerminated, dst2, src3, "XY");

	// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
	WriteTest(maybeNullTerminated, dst2, src0, "");
	WriteTest(maybeNullTerminated, dst2, src1, "X ");
	WriteTest(maybeNullTerminated, dst2, src2, "XYZ");
	WriteTest(maybeNullTerminated, dst2, src3, "XYZ");

	// Check writing of space-padded strings with last character set to null
	WriteTest(spacePaddedNull, dst2, src0, "  ");
	WriteTest(spacePaddedNull, dst2, src1, "X ");
	WriteTest(spacePaddedNull, dst2, src2, "XY");
	WriteTest(spacePaddedNull, dst2, src3, "XY");

	// Check writing of space-padded strings
	WriteTest(spacePadded, dst2, src0, "   ");
	WriteTest(spacePadded, dst2, src1, "X  ");
	WriteTest(spacePadded, dst2, src2, "XYZ");
	WriteTest(spacePadded, dst2, src3, "XYZ");

#undef ReadTest
#undef WriteTest

	{

		std::string dststring;
		std::string src0string = std::string(src0, CountOf(src0));
		std::string src1string = std::string(src1, CountOf(src1));
		std::string src2string = std::string(src2, CountOf(src2));
		std::string src3string = std::string(src3, CountOf(src3));

#define ReadTest(mode, dst, src, expectedResult) \
	mpt::String::Read<mpt::String:: mode >(dst, src); \
	VERIFY_EQUAL_NONCONT(dst, expectedResult); /* Ensure that the strings are identical */ \

#define WriteTest(mode, dst, src, expectedResult) \
	mpt::String::Write<mpt::String:: mode >(dst, src); \
	VERIFY_EQUAL_NONCONT(strncmp(dst, expectedResult, CountOf(dst)), 0);  /* Ensure that the strings are identical */ \
	for(size_t i = mpt::strnlen(dst, CountOf(dst)); i < CountOf(dst); i++) \
		VERIFY_EQUAL_NONCONT(dst[i], '\0'); /* Ensure that rest of the buffer is completely nulled */

		// Check reading of null-terminated string into std::string
		ReadTest(nullTerminated, dststring, src0, "");
		ReadTest(nullTerminated, dststring, src1, "X ");
		ReadTest(nullTerminated, dststring, src2, "XYZ");
		ReadTest(nullTerminated, dststring, src3, "XYZ");

		// Check reading of string that should be null-terminated, but is maybe too long to still hold the null character.
		ReadTest(maybeNullTerminated, dststring, src0, "");
		ReadTest(maybeNullTerminated, dststring, src1, "X ");
		ReadTest(maybeNullTerminated, dststring, src2, "XYZ ");
		ReadTest(maybeNullTerminated, dststring, src3, "XYZ!");

		// Check reading of space-padded strings with ignored last character
		ReadTest(spacePaddedNull, dststring, src0, " X");
		ReadTest(spacePaddedNull, dststring, src1, "X");
		ReadTest(spacePaddedNull, dststring, src2, "XYZ");
		ReadTest(spacePaddedNull, dststring, src3, "XYZ");

		// Check reading of space-padded strings
		ReadTest(spacePadded, dststring, src0, " X X");
		ReadTest(spacePadded, dststring, src1, "X  X");
		ReadTest(spacePadded, dststring, src2, "XYZ");
		ReadTest(spacePadded, dststring, src3, "XYZ!");

		///////////////////////////////

		// Check writing of null-terminated string into larger buffer
		WriteTest(nullTerminated, dst1, src0string, "");
		WriteTest(nullTerminated, dst1, src1string, "X ");
		WriteTest(nullTerminated, dst1, src2string, "XYZ ");
		WriteTest(nullTerminated, dst1, src3string, "XYZ!");

		// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
		WriteTest(maybeNullTerminated, dst1, src0string, "");
		WriteTest(maybeNullTerminated, dst1, src1string, "X ");
		WriteTest(maybeNullTerminated, dst1, src2string, "XYZ ");
		WriteTest(maybeNullTerminated, dst1, src3string, "XYZ!");

		// Check writing of space-padded strings with last character set to null
		WriteTest(spacePaddedNull, dst1, src0string, "     ");
		WriteTest(spacePaddedNull, dst1, src1string, "X    ");
		WriteTest(spacePaddedNull, dst1, src2string, "XYZ  ");
		WriteTest(spacePaddedNull, dst1, src3string, "XYZ! ");

		// Check writing of space-padded strings
		WriteTest(spacePadded, dst1, src0string, "      ");
		WriteTest(spacePadded, dst1, src1string, "X     ");
		WriteTest(spacePadded, dst1, src2string, "XYZ   ");
		WriteTest(spacePadded, dst1, src3string, "XYZ!  ");

		///////////////////////////////

		// Check writing of null-terminated string into smaller buffer
		WriteTest(nullTerminated, dst2, src0string, "");
		WriteTest(nullTerminated, dst2, src1string, "X ");
		WriteTest(nullTerminated, dst2, src2string, "XY");
		WriteTest(nullTerminated, dst2, src3string, "XY");

		// Check writing of string that should be null-terminated, but is maybe too long to still hold the null character.
		WriteTest(maybeNullTerminated, dst2, src0string, "");
		WriteTest(maybeNullTerminated, dst2, src1string, "X ");
		WriteTest(maybeNullTerminated, dst2, src2string, "XYZ");
		WriteTest(maybeNullTerminated, dst2, src3string, "XYZ");

		// Check writing of space-padded strings with last character set to null
		WriteTest(spacePaddedNull, dst2, src0string, "  ");
		WriteTest(spacePaddedNull, dst2, src1string, "X ");
		WriteTest(spacePaddedNull, dst2, src2string, "XY");
		WriteTest(spacePaddedNull, dst2, src3string, "XY");

		// Check writing of space-padded strings
		WriteTest(spacePadded, dst2, src0string, "   ");
		WriteTest(spacePadded, dst2, src1string, "X  ");
		WriteTest(spacePadded, dst2, src2string, "XYZ");
		WriteTest(spacePadded, dst2, src3string, "XYZ");

		///////////////////////////////

#undef ReadTest
#undef WriteTest

	}

	// Test FixNullString()
	mpt::String::FixNullString(src1);
	mpt::String::FixNullString(src2);
	mpt::String::FixNullString(src3);
	VERIFY_EQUAL_NONCONT(strncmp(src1, "X ", CountOf(src1)), 0);
	VERIFY_EQUAL_NONCONT(strncmp(src2, "XYZ", CountOf(src2)), 0);
	VERIFY_EQUAL_NONCONT(strncmp(src3, "XYZ", CountOf(src3)), 0);

}


static noinline void TestSampleConversion()
//-----------------------------------------
{
	uint8 *sourceBuf = new uint8[65536 * 4];
	void *targetBuf = new uint8[65536 * 6];


	// Signed 8-Bit Integer PCM
	// Unsigned 8-Bit Integer PCM
	// Delta 8-Bit Integer PCM
	{
		uint8 *source8 = sourceBuf;
		for(size_t i = 0; i < 256; i++)
		{
			source8[i] = static_cast<uint8>(i);
		}

		int8 *signed8 = static_cast<int8 *>(targetBuf);
		uint8 *unsigned8 = static_cast<uint8 *>(targetBuf) + 256;
		int8 *delta8 = static_cast<int8 *>(targetBuf) + 512;
		int8 delta = 0;
		CopySample<SC::DecodeInt8>(signed8, 256, 1, reinterpret_cast<const char *>(source8), 256, 1);
		CopySample<SC::DecodeUint8>(reinterpret_cast<int8 *>(unsigned8), 256, 1, reinterpret_cast<const char *>(source8), 256, 1);
		CopySample<SC::DecodeInt8Delta>(delta8, 256, 1, reinterpret_cast<const char *>(source8), 256, 1);

		for(size_t i = 0; i < 256; i++)
		{
			delta += static_cast<int8>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed8[i], static_cast<int8>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned8[i], static_cast<uint8>(i + 0x80u));
			VERIFY_EQUAL_QUIET_NONCONT(delta8[i], static_cast<int8>(delta));
		}
	}

	// Signed 16-Bit Integer PCM
	// Unsigned 16-Bit Integer PCM
	// Delta 16-Bit Integer PCM
	{
		// Little Endian

		uint8 *source16 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			source16[i * 2 + 0] = static_cast<uint8>(i & 0xFF);
			source16[i * 2 + 1] = static_cast<uint8>(i >> 8);
		}

		int16 *signed16 = static_cast<int16 *>(targetBuf);
		uint16 *unsigned16 = static_cast<uint16 *>(targetBuf) + 65536;
		int16 *delta16 = static_cast<int16 *>(targetBuf) + 65536 * 2;
		int16 delta = 0;
		CopySample<SC::DecodeInt16<0, littleEndian16> >(signed16, 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16<0x8000u, littleEndian16> >(reinterpret_cast<int16*>(unsigned16), 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16Delta<littleEndian16> >(delta16, 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			delta += static_cast<int16>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed16[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned16[i], static_cast<uint16>(i + 0x8000u));
			VERIFY_EQUAL_QUIET_NONCONT(delta16[i], static_cast<int16>(delta));
		}

		// Big Endian

		for(size_t i = 0; i < 65536; i++)
		{
			source16[i * 2 + 0] = static_cast<uint8>(i >> 8);
			source16[i * 2 + 1] = static_cast<uint8>(i & 0xFF);
		}

		CopySample<SC::DecodeInt16<0, bigEndian16> >(signed16, 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16<0x8000u, bigEndian16> >(reinterpret_cast<int16*>(unsigned16), 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);
		CopySample<SC::DecodeInt16Delta<bigEndian16> >(delta16, 65536, 1, reinterpret_cast<const char *>(source16), 65536 * 2, 1);

		delta = 0;
		for(size_t i = 0; i < 65536; i++)
		{
			delta += static_cast<int16>(i);
			VERIFY_EQUAL_QUIET_NONCONT(signed16[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(unsigned16[i], static_cast<uint16>(i + 0x8000u));
			VERIFY_EQUAL_QUIET_NONCONT(delta16[i], static_cast<int16>(delta));
		}

	}

	// Signed 24-Bit Integer PCM
	{
		uint8 *source24 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			source24[i * 3 + 0] = 0;
			source24[i * 3 + 1] = static_cast<uint8>(i & 0xFF);
			source24[i * 3 + 2] = static_cast<uint8>(i >> 8);
		}

		int16 *truncated16 = static_cast<int16 *>(targetBuf);
		ModSample sample;
		sample.Initialize();
		sample.nLength = 65536;
		sample.uFlags.set(CHN_16BIT);
		sample.pSample = (static_cast<int16 *>(targetBuf) + 65536);
		CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, int32>, SC::DecodeInt24<0, littleEndian24> > >(sample, reinterpret_cast<const char *>(source24), 3*65536);
		CopySample<SC::ConversionChain<SC::ConvertShift<int16, int32, 16>, SC::DecodeInt24<0, littleEndian24> > >(truncated16, 65536, 1, reinterpret_cast<const char *>(source24), 65536 * 3, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			VERIFY_EQUAL_QUIET_NONCONT(sample.pSample16[i], static_cast<int16>(i));
			VERIFY_EQUAL_QUIET_NONCONT(truncated16[i], static_cast<int16>(i));
		}
	}

	// Float 32-Bit
	{
		uint8 *source32 = sourceBuf;
		for(size_t i = 0; i < 65536; i++)
		{
			IEEE754binary32BE floatbits = IEEE754binary32BE((static_cast<float>(i) / 65536.0f) - 0.5f);
			source32[i * 4 + 0] = floatbits.GetByte(0);
			source32[i * 4 + 1] = floatbits.GetByte(1);
			source32[i * 4 + 2] = floatbits.GetByte(2);
			source32[i * 4 + 3] = floatbits.GetByte(3);
		}

		int16 *truncated16 = static_cast<int16 *>(targetBuf);
		ModSample sample;
		sample.Initialize();
		sample.nLength = 65536;
		sample.uFlags.set(CHN_16BIT);
		sample.pSample = static_cast<int16 *>(targetBuf) + 65536;
		CopyAndNormalizeSample<SC::NormalizationChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(sample, reinterpret_cast<const char *>(source32), 4*65536);
		CopySample<SC::ConversionChain<SC::Convert<int16, float32>, SC::DecodeFloat32<bigEndian32> > >(truncated16, 65536, 1, reinterpret_cast<const char *>(source32), 65536 * 4, 1);

		for(size_t i = 0; i < 65536; i++)
		{
			VERIFY_EQUAL_QUIET_NONCONT(sample.pSample16[i], static_cast<int16>(i - 0x8000u));
			if(abs(truncated16[i] - static_cast<int16>((i - 0x8000u) / 2)) > 1)
			{
				VERIFY_EQUAL_QUIET_NONCONT(true, false);
			}
		}
	}

	// Range checks
	{
		int8 oneSample = 1;
		char *signed8 = reinterpret_cast<char *>(targetBuf);
		memset(signed8, 0, 4);
		CopySample<SC::DecodeInt8>(reinterpret_cast<int8*>(targetBuf), 4, 1, reinterpret_cast<const char*>(&oneSample), sizeof(oneSample), 1);
		VERIFY_EQUAL_NONCONT(signed8[0], 1);
		VERIFY_EQUAL_NONCONT(signed8[1], 0);
		VERIFY_EQUAL_NONCONT(signed8[2], 0);
		VERIFY_EQUAL_NONCONT(signed8[3], 0);
	}

	delete[] sourceBuf;
	delete[] static_cast<uint8*>(targetBuf);
}


} // namespace Test

OPENMPT_NAMESPACE_END

#else //Case: ENABLE_TESTS is not defined.

OPENMPT_NAMESPACE_BEGIN

namespace Test {

void DoTests()
//------------
{
	return;
}

} // namespace Test

OPENMPT_NAMESPACE_END

#endif
