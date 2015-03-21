/*
 * mptPathString.h
 * ---------------
 * Purpose: Wrapper class around the platform-native representation of path names. Should be the only type that is used to store path names.
 * Notes  : Currently none.
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

OPENMPT_NAMESPACE_BEGIN

#if defined(MPT_WITH_PATHSTRING)

//#define MPT_DEPRECATED_PATH
#define MPT_DEPRECATED_PATH MPT_DEPRECATED

namespace mpt
{

#if MPT_OS_WINDOWS
typedef std::wstring RawPathString;
#else // !MPT_OS_WINDOWS
typedef std::string RawPathString;
#endif // if MPT_OS_WINDOWS

class PathString
{
private:
	RawPathString path;
private:
	PathString(const RawPathString & path)
		: path(path)
	{
		return;
	}
public:
	PathString()
	{
		return;
	}
	PathString(const PathString & other)
		: path(other.path)
	{
		return;
	}
	PathString & assign(const PathString & other)
	{
		path = other.path;
		return *this;
	}
	PathString & operator = (const PathString & other)
	{
		return assign(other);
	}
	PathString & append(const PathString & other)
	{
		path.append(other.path);
		return *this;
	}
	PathString & operator += (const PathString & other)
	{
		return append(other);
	}
	friend PathString operator + (const PathString & a, const PathString & b)
	{
		return PathString(a).append(b);
	}
	friend bool operator < (const PathString & a, const PathString & b)
	{
		return a.AsNative() < b.AsNative();
	}
	friend bool operator == (const PathString & a, const PathString & b)
	{
		return a.AsNative() == b.AsNative();
	}
	friend bool operator != (const PathString & a, const PathString & b)
	{
		return a.AsNative() != b.AsNative();
	}
	bool empty() const { return path.empty(); }

#if MPT_OS_WINDOWS
	static int CompareNoCase(const PathString & a, const PathString & b);
#endif

public:

#if defined(MODPLUG_TRACKER)

	void SplitPath(PathString *drive, PathString *dir, PathString *fname, PathString *ext) const;
	PathString GetDrive() const;		// Drive letter + colon, e.g. "C:"
	PathString GetDir() const;			// Directory, e.g. "\OpenMPT\"
	PathString GetPath() const;			// Drive + Dir, e.g. "C:\OpenMPT\"
	PathString GetFileName() const;		// File name without extension, e.g. "mptrack"
	PathString GetFileExt() const;		// Extension including dot, e.g. ".exe"
	PathString GetFullFileName() const;	// File name + extension, e.g. "mptrack.exe"+
	
	// Verify if this path represents a valid directory on the file system.
	bool IsDirectory() const { return ::PathIsDirectoryW(path.c_str()) != FALSE; }
	bool IsFile() const
	{
		DWORD dwAttrib = ::GetFileAttributesW(path.c_str());
		return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
	}
	bool FileOrDirectoryExists() const { return ::PathFileExistsW(path.c_str()) != FALSE; }

	// Return the same path string with a different (or appended) extension (including "."), e.g. "foo.bar",".txt" -> "foo.txt" or "C:\OpenMPT\foo",".txt" -> "C:\OpenMPT\foo.txt"
	PathString ReplaceExt(const mpt::PathString &newExt) const;

	// Removes special characters from a filename component and replaces them with a safe replacement character ("_" on windows).
	// Returns the result.
	// Note that this also removes path component separators, so this should only be used on single-component PathString objects.
	PathString SanitizeComponent() const;

	bool HasTrailingSlash() const
	{
		if(empty())
			return false;
		const RawPathString::value_type &c = path[path.length() - 1];
#if MPT_OS_WINDOWS
		if(c == L'\\' || c == L'/')
			return true;
#else
		if(c == '/')
			return true;
#endif
		return false;
	}

	// Relative / absolute paths conversion
	mpt::PathString AbsolutePathToRelative(const mpt::PathString &relativeTo) const;
	mpt::PathString RelativePathToAbsolute(const mpt::PathString &relativeTo) const;

#endif // MODPLUG_TRACKER

public:

#if MPT_OS_WINDOWS

#if !(MPT_WSTRING_CONVERT)
#error "mpt::PathString on Windows depends on MPT_WSTRING_CONVERT)"
#endif
	// conversions
#if defined(MPT_WITH_CHARSET_LOCALE)
	MPT_DEPRECATED_PATH std::string ToLocale() const { return mpt::ToLocale(path); }
#endif
	std::string ToUTF8() const { return mpt::ToCharset(mpt::CharsetUTF8, path); }
	std::wstring ToWide() const { return path; }
	mpt::ustring ToUnicode() const { return mpt::ToUnicode(path); }
#if defined(MPT_WITH_CHARSET_LOCALE)
	MPT_DEPRECATED_PATH static PathString FromLocale(const std::string &path) { return PathString(mpt::ToWide(mpt::CharsetLocale, path)); }
	static PathString FromLocaleSilent(const std::string &path) { return PathString(mpt::ToWide(mpt::CharsetLocale, path)); }
#endif
	static PathString FromUTF8(const std::string &path) { return PathString(mpt::ToWide(mpt::CharsetUTF8, path)); }
	static PathString FromWide(const std::wstring &path) { return PathString(path); }
	static PathString FromUnicode(const mpt::ustring &path) { return PathString(mpt::ToWide(path)); }
	RawPathString AsNative() const { return path; }
	static PathString FromNative(const RawPathString &path) { return PathString(path); }
#if defined(_MFC_VER)
	// CString TCHAR, so this is CHAR or WCHAR, depending on UNICODE
	MPT_DEPRECATED_PATH CString ToCString() const { return mpt::ToCString(path); }
	MPT_DEPRECATED_PATH static PathString FromCString(const CString &path) { return PathString(mpt::ToWide(path)); }
	// Non-warning-generating versions of the above. Use with extra care.
	CString ToCStringSilent() const { return mpt::ToCString(path); }
	static PathString FromCStringSilent(const CString &path) { return PathString(mpt::ToWide(path)); }
	// really special purpose, if !UNICODE, encode unicode in CString as UTF8:
	static mpt::PathString TunnelOutofCString(const CString &path);
	static CString TunnelIntoCString(const mpt::PathString &path);
	// CStringW
#ifdef UNICODE
	MPT_DEPRECATED_PATH CString ToCStringW() const { return mpt::ToCString(path); }
	MPT_DEPRECATED_PATH static PathString FromCStringW(const CString &path) { return PathString(mpt::ToWide(path)); }
#else
	CStringW ToCStringW() const { return mpt::ToCStringW(path); }
	static PathString FromCStringW(const CStringW &path) { return PathString(mpt::ToWide(path)); }
#endif
#endif

#else // !MPT_OS_WINDOWS

	// conversions
#if defined(MPT_WITH_CHARSET_LOCALE)
	std::string ToLocale() const { return path; }
	std::string ToUTF8() const { return mpt::ToCharset(mpt::CharsetUTF8, mpt::CharsetLocale, path); }
#if MPT_WSTRING_CONVERT
	std::wstring ToWide() const { return mpt::ToWide(mpt::CharsetLocale, path); }
#endif
	mpt::ustring ToUnicode() const { return mpt::ToUnicode(mpt::CharsetLocale, path); }
	static PathString FromLocale(const std::string &path) { return PathString(path); }
	static PathString FromLocaleSilent(const std::string &path) { return PathString(path); }
	static PathString FromUTF8(const std::string &path) { return PathString(mpt::ToCharset(mpt::CharsetLocale, mpt::CharsetUTF8, path)); }
#if MPT_WSTRING_CONVERT
	static PathString FromWide(const std::wstring &path) { return PathString(mpt::ToCharset(mpt::CharsetLocale, path)); }
#endif
	static PathString FromUnicode(const mpt::ustring &path) { return PathString(mpt::ToCharset(mpt::CharsetLocale, path)); }
	RawPathString AsNative() const { return path; }
	static PathString FromNative(const RawPathString &path) { return PathString(path); }
#else
	std::string ToUTF8() const { return path; }
#if MPT_WSTRING_CONVERT
	std::wstring ToWide() const { return mpt::ToWide(mpt::CharsetUTF8, path); }
#endif
	mpt::ustring ToUnicode() const { return mpt::ToUnicode(mpt::CharsetUTF8, path); }
	static PathString FromUTF8(const std::string &path) { return path; }
#if MPT_WSTRING_CONVERT
	static PathString FromWide(const std::wstring &path) { return PathString(mpt::ToCharset(mpt::CharsetUTF8, path)); }
#endif
	static PathString FromUnicode(const mpt::ustring &path) { return PathString(mpt::ToCharset(mpt::CharsetUTF8, path)); }
	RawPathString AsNative() const { return path; }
	static PathString FromNative(const RawPathString &path) { return PathString(path); }
#endif

#endif // MPT_OS_WINDOWS

};

#if defined(MPT_WITH_CHARSET_LOCALE)
MPT_DEPRECATED_PATH static inline std::string ToString(const mpt::PathString & x) { return mpt::ToLocale(x.ToUnicode()); }
#endif
#if MPT_WSTRING_FORMAT
static inline std::wstring ToWString(const mpt::PathString & x) { return x.ToWide(); }
#endif

} // namespace mpt

#if MPT_OS_WINDOWS

#define MPT_PATHSTRING(x) mpt::PathString::FromNative( L ## x )

#else // !MPT_OS_WINDOWS

#define MPT_PATHSTRING(x) mpt::PathString::FromNative( x )

#endif // MPT_OS_WINDOWS

#if defined(MODPLUG_TRACKER)

// Sanitize a filename (remove special chars)
void SanitizeFilename(mpt::PathString &filename);

void SanitizeFilename(char *beg, char *end);
void SanitizeFilename(wchar_t *beg, wchar_t *end);

void SanitizeFilename(std::string &str);
void SanitizeFilename(std::wstring &str);

template <std::size_t size>
void SanitizeFilename(char (&buffer)[size])
//-----------------------------------------
{
	STATIC_ASSERT(size > 0);
	SanitizeFilename(buffer, buffer + size);
}

template <std::size_t size>
void SanitizeFilename(wchar_t (&buffer)[size])
//--------------------------------------------
{
	STATIC_ASSERT(size > 0);
	SanitizeFilename(buffer, buffer + size);
}

#if defined(_MFC_VER)
void SanitizeFilename(CString &str);
#endif

#endif // MODPLUG_TRACKER

#endif // MPT_WITH_PATHSTRING

OPENMPT_NAMESPACE_END
