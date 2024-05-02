#include "headers_fmgen.h"
#include "file_fmgen.h"
#include "sjis2utf.h"


#ifdef _WIN32
#if !defined(_MBCS) && !defined(_UNICODE)
#else
#if defined(_MBCS)
#include <mbstring.h>
#if defined(_UNICODE)
#undef _UNICODE
#endif
#endif
#if defined(_UNICODE)
#if defined(_MBCS)
#undef _MBCS
#endif
#endif
#endif

// ---------------------------------------------------------------------------
//	構築
// ---------------------------------------------------------------------------

FilePath::FilePath() : EmptyChar('\0')
{
}

// ---------------------------------------------------------------------------
//	ファイル名をクリア
// ---------------------------------------------------------------------------

TCHAR* FilePath::Clear(TCHAR* dest, size_t size)
{
	return (TCHAR*)std::memset(reinterpret_cast<void *>(dest), 0, size * sizeof(TCHAR));
}

// ---------------------------------------------------------------------------
//	ファイル名が空文字列か確認
// ---------------------------------------------------------------------------

bool FilePath::IsEmpty(const TCHAR* src)
{
	return (*src == EmptyChar);
}

// ---------------------------------------------------------------------------
//	ファイル名がクリアされているか確認
// ---------------------------------------------------------------------------

const TCHAR* FilePath::GetEmptyStr(void)
{
	return &EmptyChar;
}

// ---------------------------------------------------------------------------
//	文字列の長さを取得
// ---------------------------------------------------------------------------

size_t FilePath::Strlen(const TCHAR *str)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return std::strlen(reinterpret_cast<const char *>(str));
#else
#ifdef _MBCS
	return std::strlen(reinterpret_cast<const char *>(str));
#endif
	
#ifdef _UNICODE
	return std::wcslen(reinterpret_cast<const wchar_t *>(str));
#endif
#endif
}

// ---------------------------------------------------------------------------
//	文字列を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Strcmp(const TCHAR *str1, const TCHAR *str2)
{
	return _tcscmp(str1, str2);
}

// ---------------------------------------------------------------------------
//	サイズを指定して文字列を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Strncmp(const TCHAR *str1, const TCHAR *str2, size_t size)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return std::strncmp(reinterpret_cast<const char *>(str1), reinterpret_cast<const char *>(str2), size);
#else
#ifdef _MBCS
	return std::strncmp(reinterpret_cast<const char *>(str1), reinterpret_cast<const char *>(str2), size);
#endif
	
#ifdef _UNICODE
	return std::wcsncmp(reinterpret_cast<const wchar_t *>(str1), reinterpret_cast<const wchar_t *>(str2), size);
#endif
#endif
}

// ---------------------------------------------------------------------------
//	大文字、小文字を同一視して文字列を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Stricmp(const TCHAR *str1, const TCHAR *str2)
{
	return _tcsicmp(str1, str2);
}

// ---------------------------------------------------------------------------
//	大文字、小文字を同一視、サイズを指定して文字列を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Strnicmp(const TCHAR *str1, const TCHAR *str2, size_t size)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return _strnicmp(reinterpret_cast<const char *>(str1), reinterpret_cast<const char *>(str2), size);
#else
#ifdef _MBCS
	return _mbsnicmp(reinterpret_cast<const unsigned char *>(str1), reinterpret_cast<const unsigned char *>(str2), size);
#endif
	
#ifdef _UNICODE
	return _wcsnicmp(reinterpret_cast<const wchar_t *>(str1), reinterpret_cast<const wchar_t *>(str2), size);
#endif
#endif
}

// ---------------------------------------------------------------------------
//	文字列をコピー
// ---------------------------------------------------------------------------

TCHAR* FilePath::Strcpy(TCHAR* dest, const TCHAR* src)
{
	return _tcscpy(dest, src);
}

// ---------------------------------------------------------------------------
//	長さを指定して文字列をコピー
// ---------------------------------------------------------------------------

TCHAR* FilePath::Strncpy(TCHAR* dest, const TCHAR* src, size_t size)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return (TCHAR *)std::strncpy(reinterpret_cast<char *>(dest), reinterpret_cast<const char *>(src), size);
#else
#ifdef _MBCS
	return (TCHAR *)std::strncpy(reinterpret_cast<char *>(dest), reinterpret_cast<const char *>(src), size);
#endif
	
#ifdef _UNICODE
	return (TCHAR *)std::wcsncpy(reinterpret_cast<wchar_t *>(dest), reinterpret_cast<const wchar_t *>(src), size);
#endif
#endif
}

// ---------------------------------------------------------------------------
//	文字列を追加
// ---------------------------------------------------------------------------

TCHAR* FilePath::Strcat(TCHAR* dest, const TCHAR* src)
{
	return _tcscat(dest, src);
}

// ---------------------------------------------------------------------------
//	文字数を指定して文字列を追加
// ---------------------------------------------------------------------------

TCHAR* FilePath::Strncat(TCHAR* dest, const TCHAR* src, size_t count)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return (TCHAR *)std::strncat(reinterpret_cast<char *>(dest), reinterpret_cast<const char *>(src), count);
#else
#ifdef _MBCS
	return (TCHAR *)std::strncat(reinterpret_cast<char *>(dest), reinterpret_cast<const char *>(src), count);
#endif
	
#ifdef _UNICODE
	return (TCHAR *)std::wcsncat(reinterpret_cast<wchar_t *>(dest), reinterpret_cast<const wchar_t *>(src), count);
#endif
#endif
}

// ---------------------------------------------------------------------------
//	指定された文字の最初の出現箇所を検索
// ---------------------------------------------------------------------------

const TCHAR* FilePath::Strchr(const TCHAR *str, TCHAR c)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return (const TCHAR *)std::strchr(reinterpret_cast<const char *>(str), (int)c);
#else
#ifdef _MBCS
	return (const TCHAR *)_mbschr(reinterpret_cast<const unsigned char *>(str), (unsigned int)c);
#endif
	
#ifdef _UNICODE
	return (const TCHAR *)wcschr(reinterpret_cast<const wchar_t *>(str), (wchar_t)c);
#endif
#endif
}

TCHAR* FilePath::Strchr(TCHAR *str, TCHAR c)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return (TCHAR *)std::strchr(reinterpret_cast<char *>(str), (int)c);
#else
#ifdef _MBCS
	return (TCHAR *)_mbschr(reinterpret_cast<unsigned char *>(str), (unsigned int)c);
#endif
	
#ifdef _UNICODE
	return (TCHAR *)wcschr(reinterpret_cast<wchar_t *>(str), (wchar_t)c);
#endif
#endif
}

// ---------------------------------------------------------------------------
//	指定された文字の最後の出現箇所を検索
// ---------------------------------------------------------------------------

const TCHAR* FilePath::Strrchr(const TCHAR *str, TCHAR c)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return (const TCHAR *)std::strrchr(reinterpret_cast<const char *>(str), (int)c);
#else
#ifdef _MBCS
	return (const TCHAR *)_mbsrchr(reinterpret_cast<const unsigned char *>(str), (unsigned int)c);
#endif
	
#ifdef _UNICODE
	return (const TCHAR *)wcsrchr(reinterpret_cast<const wchar_t *>(str), (wchar_t)c);
#endif
#endif
}

TCHAR* FilePath::Strrchr(TCHAR *str, TCHAR c)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return (TCHAR *)std::strrchr(reinterpret_cast<char *>(str), (int)c);
#else
#ifdef _MBCS
	return (TCHAR *)_mbsrchr(reinterpret_cast<unsigned char *>(str), (unsigned int)c);
#endif
	
#ifdef _UNICODE
	return (TCHAR *)wcsrchr(reinterpret_cast<wchar_t *>(str), (wchar_t)c);
#endif
#endif
}

// ---------------------------------------------------------------------------
//	文字列の末尾に「\」を付与
// ---------------------------------------------------------------------------

TCHAR* FilePath::AddDelimiter(TCHAR* str)
{
	if(Strrchr(str, _T('\\')) != &str[Strlen(str)-1]) {
		Strcat(str, _T("\\"));
	}
	return str;
}

// ---------------------------------------------------------------------------
//	ファイル名を分割
// ---------------------------------------------------------------------------

void FilePath::Splitpath(const TCHAR *path, TCHAR *drive, TCHAR *dir, TCHAR *fname, TCHAR *ext)
{
	_tsplitpath(path, drive, dir, fname, ext);
}

// ---------------------------------------------------------------------------
//	ファイル名を合成
// ---------------------------------------------------------------------------

void FilePath::Makepath(TCHAR *path, const TCHAR *drive, const TCHAR *dir, const TCHAR *fname, const TCHAR *ext)
{
	_tmakepath(path, drive, dir, fname, ext);
}

// ---------------------------------------------------------------------------
//	ファイル名を合成(ディレクトリ＋ファイル名)
// ---------------------------------------------------------------------------

void FilePath::Makepath_dir_filename(TCHAR* path, const TCHAR* dir, const TCHAR* filename)
{
	Strcpy(path, dir);
	if(Strrchr(dir, _T('\\')) != &dir[Strlen(dir)-1] && Strlen(path) > 0) {
		Strcat(path, _T("\\"));
	}
	Strcat(path, filename);
}

// ---------------------------------------------------------------------------
//	ファイルパスから指定された要素を抽出
// ---------------------------------------------------------------------------

void FilePath::Extractpath(TCHAR *dest, const TCHAR *src, uint32_t flg)
{
	TCHAR	drive[_MAX_PATH];
	TCHAR	dir[_MAX_PATH];
	TCHAR	filename[_MAX_PATH];
	TCHAR	ext[_MAX_PATH];
	TCHAR*	pdrive;
	TCHAR*	pdir;
	TCHAR*	pfilename;
	TCHAR*	pext;

	*dest = EmptyChar;
	if(flg & extractpath_drive) {
		pdrive = drive;
	} else {
		pdrive = NULL;
	}

	if(flg & extractpath_dir) {
		pdir = dir;
	} else {
		pdir = NULL;
	}

	if(flg & extractpath_filename) {
		pfilename = filename;
	} else {
		pfilename = NULL;
	}

	if(flg & extractpath_ext) {
		pext = ext;
	} else {
		pext = NULL;
	}

	Splitpath(src, pdrive, pdir, pfilename, pext);
	Makepath(dest, pdrive, pdir, pfilename, pext);
}

// ---------------------------------------------------------------------------
//	ファイルパスから指定された要素を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Comparepath(TCHAR *filename1, const TCHAR *filename2, uint32_t flg)
{
	TCHAR	extfilename1[_MAX_PATH];
	TCHAR	extfilename2[_MAX_PATH];
	
	Extractpath(extfilename1, filename1, flg);
	Extractpath(extfilename2, filename2, flg);
	return Stricmp(extfilename1, extfilename2);
}

// ---------------------------------------------------------------------------
//	拡張子を変更
// ---------------------------------------------------------------------------

TCHAR* FilePath::ExchangeExt(TCHAR *dest, TCHAR *src, const TCHAR *ext)
{
	TCHAR	drive2[_MAX_PATH];
	TCHAR	dir2[_MAX_PATH];
	TCHAR	filename2[_MAX_PATH];
	TCHAR	ext2[_MAX_PATH];
	
	Splitpath(src, drive2, dir2, filename2, ext2);
	Makepath(dest, drive2, dir2, filename2, ext);
	
	return dest;
}

// ---------------------------------------------------------------------------
//	char配列→TCHAR配列に変換
// ---------------------------------------------------------------------------

TCHAR* FilePath::CharToTCHAR(TCHAR *dest, const char *src)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	return std::strcpy(dest, src);
#else
#ifdef _MBCS
	return std::strcpy(reinterpret_cast<char *>(dest), src);
#endif
	
#ifdef _UNICODE
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, reinterpret_cast<LPWSTR>(dest), std::strlen(src));
	return dest;
#endif
#endif
}

// ---------------------------------------------------------------------------
//	char配列→TCHAR配列に変換(文字数指定)
// ---------------------------------------------------------------------------

TCHAR* FilePath::CharToTCHARn(TCHAR *dest, const char *src, size_t count)
{
#if !defined(_MBCS) && !defined(_UNICODE)
	Clear(dest, count);
	return Strncpy(dest, src, count);
#else
#ifdef _MBCS
	Clear(dest, count);
	return Strncpy(dest, src, count);
#endif
	
#ifdef _UNICODE
	Clear(dest, count);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, reinterpret_cast<LPWSTR>(dest), count);
	return dest;
#endif
#endif
}


// ---------------------------------------------------------------------------
//	構築/消滅
// ---------------------------------------------------------------------------

FileIO::FileIO()
{
	uRefCount = 0;
	flags = 0;
	lorigin = 0;
	error = error_success;
	path = new TCHAR[_MAX_PATH];
	
	hfile = 0;
}

FileIO::~FileIO()
{
	Close();
	delete[] path;
}

// ---------------------------------------------------------------------------
//	ファイル名で示されたファイルのサイズを取得する
// ---------------------------------------------------------------------------

int64_t WINAPI FileIO::GetFileSize(const TCHAR* filename)
{
	HANDLE	handle;
	WIN32_FIND_DATA	FindFileData;
	
	if ((handle = FindFirstFile(filename, &FindFileData)) == INVALID_HANDLE_VALUE) {
		return -1;		// 取得不可
	}
	else {
		FindClose(handle);
		return (((int64_t)FindFileData.nFileSizeHigh) << 32) + FindFileData.nFileSizeLow;
	}
}

// ---------------------------------------------------------------------------
//	ファイルを開く
// ---------------------------------------------------------------------------

bool WINAPI FileIO::Open(const TCHAR* filename, uint32_t flg)
{
	FilePath	filepath;
	
	Close();
	
	filepath.Strncpy(path, filename, _MAX_PATH);
	
	uint32_t access = (flg & flags_readonly ? 0 : GENERIC_WRITE) | GENERIC_READ;
	uint32_t share = (flg & flags_readonly) ? FILE_SHARE_READ : 0;
	uint32_t creation = flg & flags_create ? CREATE_ALWAYS : OPEN_EXISTING;
	
	hfile = CreateFile(filename, access, share, 0, creation, 0, 0);
	
	flags = (flg & flags_readonly) | (hfile == INVALID_HANDLE_VALUE ? 0 : flags_open);
	if (!(flags & flags_open))
	{
		switch (GetLastError())
		{
		case ERROR_FILE_NOT_FOUND:		error = error_file_not_found; break;
		case ERROR_SHARING_VIOLATION:	error = error_sharing_violation; break;
		default: error = error_unknown; break;
		}
	}
	SetLogicalOrigin(0);
	
	return !!(flags & flags_open);
}

// ---------------------------------------------------------------------------
//	ファイルがない場合は作成
// ---------------------------------------------------------------------------

bool WINAPI FileIO::CreateNew(const TCHAR* filename)
{
	FilePath	filepath;
	Close();
	
	filepath.Strncpy(path, filename, _MAX_PATH);
	
	uint32_t access = GENERIC_WRITE | GENERIC_READ;
	uint32_t share = 0;
	uint32_t creation = CREATE_NEW;
	
	hfile = CreateFile(filename, access, share, 0, creation, 0, 0);
	
	flags = (hfile == INVALID_HANDLE_VALUE ? 0 : flags_open);
	SetLogicalOrigin(0);
	
	return !!(flags & flags_open);
}

// ---------------------------------------------------------------------------
//	ファイルを作り直す
// ---------------------------------------------------------------------------

bool WINAPI FileIO::Reopen(uint32_t flg)
{
	if (!(flags & flags_open)) return false;
	if ((flags & flags_readonly) && (flg & flags_create)) return false;
	
	if (flags & flags_readonly) flg |= flags_readonly;
	
	Close();
	
	uint32_t access = (flg & flags_readonly ? 0 : GENERIC_WRITE) | GENERIC_READ;
	uint32_t share = flg & flags_readonly ? FILE_SHARE_READ : 0;
	uint32_t creation = flg & flags_create ? CREATE_ALWAYS : OPEN_EXISTING;
	
	hfile = CreateFile(path, access, share, 0, creation, 0, 0);
	
	flags = (flg & flags_readonly) | (hfile == INVALID_HANDLE_VALUE ? 0 : flags_open);
	SetLogicalOrigin(0);
	
	return !!(flags & flags_open);
}

// ---------------------------------------------------------------------------
//	ファイルを閉じる
// ---------------------------------------------------------------------------

void WINAPI FileIO::Close()
{
	if (GetFlags() & flags_open)
	{
		CloseHandle(hfile);
		hfile = 0;
		flags = 0;
	}
}

// ---------------------------------------------------------------------------
//	ファイルからの読み出し
// ---------------------------------------------------------------------------

int32_t WINAPI FileIO::Read(void* dest, int32_t size)
{
	if (!(GetFlags() & flags_open))
		return -1;
	
	DWORD readsize;
	if (!ReadFile(hfile, dest, size, &readsize, 0))
		return -1;
	return readsize;
}

// ---------------------------------------------------------------------------
//	ファイルへの書き出し
// ---------------------------------------------------------------------------

int32_t WINAPI FileIO::Write(const void* dest, int32_t size)
{
	if (!(GetFlags() & flags_open) || (GetFlags() & flags_readonly))
		return -1;
	
	DWORD writtensize;
	if (!WriteFile(hfile, dest, size, &writtensize, 0))
		return -1;
	return writtensize;
}

// ---------------------------------------------------------------------------
//	ファイルをシーク
// ---------------------------------------------------------------------------

bool WINAPI FileIO::Seek(int32_t pos, SeekMethod method)
{
	if (!(GetFlags() & flags_open))
		return false;
	
	uint32_t wmethod;
	switch (method)
	{
	case seekmethod_begin:
		wmethod = FILE_BEGIN; pos += lorigin;
		break;
	case seekmethod_current:
		wmethod = FILE_CURRENT;
		break;
	case seekmethod_end:
		wmethod = FILE_END;
		break;
	default:
		return false;
	}
	
	return 0xffffffff != SetFilePointer(hfile, pos, 0, wmethod);
}

// ---------------------------------------------------------------------------
//	ファイルの位置を得る
// ---------------------------------------------------------------------------

int32_t WINAPI FileIO::Tellp()
{
	if (!(GetFlags() & flags_open))
		return 0;
	
	return SetFilePointer(hfile, 0, 0, FILE_CURRENT) - lorigin;
}

// ---------------------------------------------------------------------------
//	現在の位置をファイルの終端とする
// ---------------------------------------------------------------------------

bool WINAPI FileIO::SetEndOfFile()
{
	if (!(GetFlags() & flags_open))
		return false;
	return ::SetEndOfFile(hfile) != 0;
}


#else
#include <iostream>
// ---------------------------------------------------------------------------
//	構築
// ---------------------------------------------------------------------------

FilePath::FilePath() : EmptyChar('\0')
{
}

// ---------------------------------------------------------------------------
//	ファイル名をクリア
// ---------------------------------------------------------------------------

TCHAR* FilePath::Clear(TCHAR* dest, size_t size)
{
	return (TCHAR*)std::memset(reinterpret_cast<void *>(dest), 0, size * sizeof(TCHAR));
}

// ---------------------------------------------------------------------------
//	ファイル名が空文字列か確認
// ---------------------------------------------------------------------------

bool FilePath::IsEmpty(const TCHAR* src)
{
	return (*src == EmptyChar);
}

// ---------------------------------------------------------------------------
//	ファイル名がクリアされているか確認
// ---------------------------------------------------------------------------

const TCHAR* FilePath::GetEmptyStr(void)
{
	return &EmptyChar;
}

// ---------------------------------------------------------------------------
//	文字列の長さを取得
// ---------------------------------------------------------------------------

size_t FilePath::Strlen(const TCHAR *str)
{
	return std::strlen(reinterpret_cast<const char *>(str));
}

// ---------------------------------------------------------------------------
//	文字列を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Strcmp(const TCHAR *str1, const TCHAR *str2)
{
	return std::strcmp(reinterpret_cast<const char *>(str1), reinterpret_cast<const char *>(str2));
}

// ---------------------------------------------------------------------------
//	サイズを指定して文字列を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Strncmp(const TCHAR *str1, const TCHAR *str2, size_t size)
{
	return std::strncmp(reinterpret_cast<const char *>(str1), reinterpret_cast<const char *>(str2), size);
}

// ---------------------------------------------------------------------------
//	大文字、小文字を同一視して文字列を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Stricmp(const TCHAR *str1, const TCHAR *str2)
{
	return strcasecmp(str1, str2);
}

// ---------------------------------------------------------------------------
//	大文字、小文字を同一視、サイズを指定して文字列を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Strnicmp(const TCHAR *str1, const TCHAR *str2, size_t size)
{
	return strncasecmp(str1, str2, size);
}

// ---------------------------------------------------------------------------
//	文字列をコピー
// ---------------------------------------------------------------------------

TCHAR* FilePath::Strcpy(TCHAR* dest, const TCHAR* src)
{
	return std::strcpy(dest, src);
}

// ---------------------------------------------------------------------------
//	長さを指定して文字列をコピー
// ---------------------------------------------------------------------------

TCHAR* FilePath::Strncpy(TCHAR* dest, const TCHAR* src, size_t size)
{
	return std::strncpy(reinterpret_cast<char *>(dest), reinterpret_cast<const char *>(src), size);
}

// ---------------------------------------------------------------------------
//	文字列を追加
// ---------------------------------------------------------------------------

TCHAR* FilePath::Strcat(TCHAR* dest, const TCHAR* src)
{
	return std::strcat(reinterpret_cast<char *>(dest), reinterpret_cast<const char *>(src));
}

// ---------------------------------------------------------------------------
//	文字数を指定して文字列を追加
// ---------------------------------------------------------------------------

TCHAR* FilePath::Strncat(TCHAR* dest, const TCHAR* src, size_t count)
{
	return (TCHAR*)std::strncat(reinterpret_cast<char *>(dest), reinterpret_cast<const char *>(src), count);
}

// ---------------------------------------------------------------------------
//	指定された文字の最初の出現箇所を検索
// ---------------------------------------------------------------------------

const TCHAR* FilePath::Strchr(const TCHAR *str, TCHAR c)
{
	return (const TCHAR*)std::strchr(reinterpret_cast<const char *>(str), (int)c);
}

TCHAR* FilePath::Strchr(TCHAR *str, TCHAR c)
{
	return (TCHAR *)std::strchr(reinterpret_cast<char *>(str), (int)c);
}

// ---------------------------------------------------------------------------
//	指定された文字の最後の出現箇所を検索
// ---------------------------------------------------------------------------

const TCHAR* FilePath::Strrchr(const TCHAR *str, TCHAR c)
{
	return (const TCHAR*)std::strrchr(reinterpret_cast<const char*>(str), (int)c);
}

TCHAR* FilePath::Strrchr(TCHAR *str, TCHAR c)
{
	return (TCHAR *)std::strrchr(reinterpret_cast<char *>(str), (int)c);
}

// ---------------------------------------------------------------------------
//	文字列の末尾に「/」を付与
// ---------------------------------------------------------------------------

TCHAR* FilePath::AddDelimiter(TCHAR* str)
{
	if(Strrchr(str, _T('/')) != &str[Strlen(str)-1]) {
		Strcat(str, _T("/"));
	}
	return str;
}

// ---------------------------------------------------------------------------
//	ファイル名を分割
// ---------------------------------------------------------------------------

void FilePath::Splitpath(const TCHAR *path, TCHAR *drive, TCHAR *dir, TCHAR *fname, TCHAR *ext)
{
	if(path == NULL) {
		if(drive != NULL) {
			*drive = EmptyChar;
		}
		
		if(dir != NULL) {
			*dir = EmptyChar;
		}
		
		if(fname != NULL) {
			*fname = EmptyChar;
		}
		
		if(ext != NULL) {
			*ext = EmptyChar;
		}
		return;
	}
	if(*path == EmptyChar) {
		if(drive != NULL) {
			*drive = EmptyChar;
		}
		
		if(dir != NULL) {
			*dir = EmptyChar;
		}
		
		if(fname != NULL) {
			*fname = EmptyChar;
		}
		
		if(ext != NULL) {
			*ext = EmptyChar;
		}
		return;
	}
	
	const TCHAR* p1 = Strrchr(path, '/');
	const TCHAR* p2 = Strrchr(path, '.');
	
	if(p1 != NULL && p2 != NULL && p1 > p2) {
		p2 = const_cast<TCHAR*>(path) + Strlen(path);
	}
	
	if(p1 == NULL) {
		p1 = const_cast<TCHAR*>(path);
	}
	
	if(p2 == NULL) {
		p2 = const_cast<TCHAR*>(path) + Strlen(path);
	}
	
	if(drive != NULL) {
		*drive = EmptyChar;
	}
	
	if(dir != NULL) {
		Strncpy(dir, path, p1 - path);
		dir[p1 - path] = EmptyChar;
	}
	
	if(fname != NULL) {
		Strncpy(fname, p1 + 1, p2 - p1 - 1);
		fname[p2 - p1 - 1] = EmptyChar;
	}
	
	if(ext != NULL) {
		Strcpy(ext, p2);
	}
}

// ---------------------------------------------------------------------------
//	ファイル名を合成
// ---------------------------------------------------------------------------

void FilePath::Makepath(TCHAR *path, const TCHAR *drive, const TCHAR *dir, const TCHAR *fname, const TCHAR *ext)
{
	// driveは無視
	
	if(dir == NULL) {
		if (path != NULL) { *path = EmptyChar; }
	} else {
		if (0 != std::strncmp(path, dir, std::strlen(dir))) { std::strcpy(path, dir); }
		if(std::strlen(path) >= 1) {
			if(path[std::strlen(path)-1] != '/') {
				strcat(path, "/");
			}
		}
	}
	if(fname != NULL) {
		strcat(path, fname);
	}
	
	if(ext != NULL) {
		strcat(path, ext);
	}
}

// ---------------------------------------------------------------------------
//	ファイル名を合成(ディレクトリ＋ファイル名)
// ---------------------------------------------------------------------------

void FilePath::Makepath_dir_filename(TCHAR* path, const TCHAR* dir, const TCHAR* filename)
{
	Strcpy(path, dir);
	if(Strrchr(dir, _T('/')) != &dir[Strlen(dir)-1] && Strlen(path) > 0) {
		Strcat(path, _T("/"));
	}
	Strcat(path, filename);
}

// ---------------------------------------------------------------------------
//	ファイルパスから指定された要素を抽出
// ---------------------------------------------------------------------------

void FilePath::Extractpath(TCHAR *dest, const TCHAR *src, uint32_t flg)
{
	TCHAR	drive[_MAX_PATH];
	TCHAR	dir[_MAX_PATH];
	TCHAR	filename[_MAX_PATH];
	TCHAR	ext[_MAX_PATH];
	TCHAR*	pdrive;
	TCHAR*	pdir;
	TCHAR*	pfilename;
	TCHAR*	pext;
	
	*dest = EmptyChar;
	if(flg & extractpath_drive) {
		pdrive = drive;
	} else {
		pdrive = NULL;
	}
	
	if(flg & extractpath_dir) {
		pdir = dir;
	} else {
		pdir = NULL;
	}
	
	if(flg & extractpath_filename) {
		pfilename = filename;
	} else {
		pfilename = NULL;
	}
	
	if(flg & extractpath_ext) {
		pext = ext;
	} else {
		pext = NULL;
	}
	
	Splitpath(src, pdrive, pdir, pfilename, pext);
	Makepath(dest, pdrive, pdir, pfilename, pext);
}

// ---------------------------------------------------------------------------
//	ファイルパスから指定された要素を比較
// ---------------------------------------------------------------------------

int32_t FilePath::Comparepath(TCHAR *filename1, const TCHAR *filename2, uint32_t flg)
{
	TCHAR	extfilename1[_MAX_PATH];
	TCHAR	extfilename2[_MAX_PATH];
	
	Extractpath(extfilename1, filename1, flg);
	Extractpath(extfilename2, filename2, flg);
	return Stricmp(extfilename1, extfilename2);
}

// ---------------------------------------------------------------------------
//	拡張子を変更
// ---------------------------------------------------------------------------

TCHAR* FilePath::ExchangeExt(TCHAR *dest, TCHAR *src, const TCHAR *ext)
{
	TCHAR	drive2[_MAX_PATH];
	TCHAR	dir2[_MAX_PATH];
	TCHAR	filename2[_MAX_PATH];
	TCHAR	ext2[_MAX_PATH];
	
	Splitpath(src, drive2, dir2, filename2, ext2);
	Makepath(dest, drive2, dir2, filename2, ext);
	
	return dest;
}

// ---------------------------------------------------------------------------
//	char配列→TCHAR配列に変換
// ---------------------------------------------------------------------------

TCHAR* FilePath::CharToTCHAR(TCHAR *dest, const char *src)
{
	//	return std::strcpy(dest, src);
	return (TCHAR*)sjis2utf8((uint8_t*)dest, (uint8_t*)src);
}

// ---------------------------------------------------------------------------
//	char配列→TCHAR配列に変換(文字数指定)
// ---------------------------------------------------------------------------

TCHAR* FilePath::CharToTCHARn(TCHAR *dest, const char *src, size_t count)
{
	Clear(dest, count);
	//	return Strncpy(dest, src, count);
	return (TCHAR*)sjis2utf8n((uint8_t*)dest, (uint8_t*)src, count);
}


// ---------------------------------------------------------------------------
//	構築/消滅
// ---------------------------------------------------------------------------

FileIO::FileIO()
{
	uRefCount = 0;
	flags = 0;
	lorigin = 0;
	error = error_success;
	path = new TCHAR[_MAX_PATH];
	
	fp = NULL;
}

FileIO::~FileIO()
{
	Close();
	delete[] path;
}

 // ---------------------------------------------------------------------------
//	ファイル名で示されたファイルのサイズを取得する
// ---------------------------------------------------------------------------

int64_t WINAPI FileIO::GetFileSize(const TCHAR* filename)
{
	int32_t fd;
	struct stat buf;
	
	if ((fd = open(filename, O_RDONLY)) == -1) {
		return -1;		// 取得不可
	
	}
	else if (fstat(fd, &buf) != 0) {
		close(fd);
		return -1;		// 取得不可
	}
	
	close(fd);
	return buf.st_size;
}

// ---------------------------------------------------------------------------
//	ファイルを開く
// ---------------------------------------------------------------------------

bool WINAPI FileIO::Open(const TCHAR* filename, uint32_t flg)
{
	FilePath	filepath;

	Close();

	filepath.Strncpy(path, filename, _MAX_PATH);

	char	mode[4] = { 'r', 'b', '\0', '\0' };

	if(flg & flags_readonly)
	{
		std::strcpy(mode, "rb");
	}
	else if(flg & flags_create)
	{
		std::strcpy(mode, "wb");
	}
	if (nullptr != filename) { std::cout << "fopen( " << filename << ", " << mode << ");\n"; }
	fp = fopen(filename, mode);

	flags = (flg & flags_readonly) | (fp == nullptr ? 0 : flags_open);
	if (!(flags & flags_open))
	{
		error = error_unknown;		// とりあえずのエラー設定
	}
	SetLogicalOrigin(0);

	return !!(flags & flags_open);
}

// ---------------------------------------------------------------------------
//	ファイルがない場合は作成
// ---------------------------------------------------------------------------

bool WINAPI FileIO::CreateNew(const TCHAR* filename)
{
	FilePath	filepath;
	Close();
	
	filepath.Strncpy(path, filename, _MAX_PATH);
	
	if((fp = fopen(filename,"rb")) != NULL)
	{
		fclose( fp);
		flags = 0;
	}
	else
	{
		fp = fopen(filename, "w+b");
		
		flags = (fp == NULL ? 0 : flags_open);
	}
	SetLogicalOrigin(0);
	
	return !!(flags & flags_open);
}

// ---------------------------------------------------------------------------
//	ファイルを作り直す
// ---------------------------------------------------------------------------

bool WINAPI FileIO::Reopen(uint32_t flg)
{
	if (!(flags & flags_open)) return false;
	if ((flags & flags_readonly) && (flg & flags_create)) return false;
	
	if (flags & flags_readonly) flg |= flags_readonly;
	
	Close();
	
	char	mode[4];
	
	if(flg & flags_readonly)
	{
		std::strcpy(mode, "rb");
	}
	else if(flg & flags_create)
	{
		std::strcpy(mode, "wb");
	}
	
	fp = fopen(path, mode);
	
	flags = (flg & flags_readonly) | (fp == NULL ? 0 : flags_open);
	SetLogicalOrigin(0);
	
	return !!(flags & flags_open);
}

// ---------------------------------------------------------------------------
//	ファイルを閉じる
// ---------------------------------------------------------------------------

void WINAPI FileIO::Close()
{
	if (GetFlags() & flags_open)
	{
		fclose(fp);
		fp = NULL;
		flags = 0;
	}
}

// ---------------------------------------------------------------------------
//	ファイルからの読み出し
// ---------------------------------------------------------------------------

int32_t WINAPI FileIO::Read(void* dest, int32_t size)
{
	if (!(GetFlags() & flags_open))
		return -1;
	
	uint32_t readsize;
	readsize = fread(dest, 1, size, fp);
	if (!readsize && size > 0)
		return -1;
	return readsize;
}

// ---------------------------------------------------------------------------
//	ファイルへの書き出し
// ---------------------------------------------------------------------------

int32_t WINAPI FileIO::Write(const void* dest, int32_t size)
{
	if (!(GetFlags() & flags_open) || (GetFlags() & flags_readonly))
		return -1;
	
	uint32_t writtensize;
	writtensize = fwrite(dest, 1, size, fp);
	if (!writtensize && size > 0)
		return -1;
	return writtensize;
}

// ---------------------------------------------------------------------------
//	ファイルをシーク
// ---------------------------------------------------------------------------

bool WINAPI FileIO::Seek(int32_t pos, SeekMethod method)
{
	if (!(GetFlags() & flags_open))
		return false;
	
	int32_t wmethod;
	switch (method)
	{
	case seekmethod_begin:
		wmethod = SEEK_SET; pos += lorigin;
		break;
	case seekmethod_current:
		wmethod = SEEK_CUR;
		break;
	case seekmethod_end:
		wmethod = SEEK_END;
		break;
	default:
		return false;
	}
	
	return 0 == fseek(fp, pos, wmethod);
}

// ---------------------------------------------------------------------------
//	ファイルの位置を得る
// ---------------------------------------------------------------------------

int32_t WINAPI FileIO::Tellp()
{
	if (!(GetFlags() & flags_open))
		return 0;
	
	return ftell(fp) - lorigin;
}

// ---------------------------------------------------------------------------
//	現在の位置をファイルの終端とする
// ---------------------------------------------------------------------------

bool WINAPI FileIO::SetEndOfFile()
{
	if (!(GetFlags() & flags_open))
		return false;
	ftruncate(fileno(fp), Tellp());
	return true;
}


#endif


//=============================================================================
//	interpositioned IUnknown Interface(AddRef)
//=============================================================================
uint32_t WINAPI FileIO::AddRef(void)
{
	return ++uRefCount;
}


//=============================================================================
//	interpositioned IUnknown Interface(Release)
//=============================================================================
uint32_t WINAPI FileIO::Release(void)
{
	uint32_t ref = --uRefCount;
	if (ref == 0) {
		delete this;
	}
	return ref;
}
