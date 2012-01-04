//	$Id: file.cpp,v 1.1 2001/04/23 22:25:33 kaoru-k Exp $

#include <ctype.h>
#include "headers.h"
#include "file.h"

// ---------------------------------------------------------------------------
//	構築/消滅
// ---------------------------------------------------------------------------

FileIO::FileIO()
{
	flags = 0;
}

FileIO::FileIO(const char* filename, uint flg)
{
	flags = 0;
	Open(filename, flg);
}

FileIO::~FileIO()
{
	Close();
}

// ---------------------------------------------------------------------------
//	ファイルを開く
// ---------------------------------------------------------------------------

#ifdef WINDOWS
bool FileIO::Open(const char* filename, uint flg)
{
	Close();

	strncpy(path, filename, MAX_PATH);

	DWORD access = (flg & readonly ? 0 : GENERIC_WRITE) | GENERIC_READ;
	DWORD share = (flg & readonly) ? FILE_SHARE_READ : 0;
	DWORD creation = flg & create ? CREATE_ALWAYS : OPEN_EXISTING;

	hfile = CreateFile(filename, access, share, 0, creation, 0, 0);
	
	flags = (flg & readonly) | (hfile == INVALID_HANDLE_VALUE ? 0 : open);
	if (!(flags & open))
	{
		switch (GetLastError())
		{
		case ERROR_FILE_NOT_FOUND:		error = file_not_found; break;
		case ERROR_SHARING_VIOLATION:	error = sharing_violation; break;
		default: error = unknown; break;
		}
	}
	SetLogicalOrigin(0);

	return !!(flags & open);
}
#else  // !WINDOWS
bool FileIO::Open(const char* filename, uint flg)
{
	Close();

	strncpy(path, filename, FILENAME_MAX);

	if (!(flg & readonly))
	    error = unknown;

	hfile = fopen(filename, "rb");
	if (!hfile) {
	    int i;
	    char *lowername;

	    lowername = strdup(filename);
	    for (i = 0; i < strlen(lowername); i ++)
			lowername[i] = tolower(lowername[i]);

	    hfile = fopen(lowername, "rb");
	    free(lowername);

	    if (!hfile)
			error = file_not_found;
	    else
			flags = open;
	} else
	  flags = open;
	      
	return !!(flags & open);
}
#endif // !WINDOWS

// ---------------------------------------------------------------------------
//	ファイルがない場合は作成
// ---------------------------------------------------------------------------

#ifdef WINDOWS
bool FileIO::CreateNew(const char* filename)
{
	Close();

	strncpy(path, filename, MAX_PATH);

	DWORD access = GENERIC_WRITE | GENERIC_READ;
	DWORD share = 0;
	DWORD creation = CREATE_NEW;

	hfile = CreateFile(filename, access, share, 0, creation, 0, 0);
	
	flags = (hfile == INVALID_HANDLE_VALUE ? 0 : open);
	SetLogicalOrigin(0);

	return !!(flags & open);
}
#else  // !WINDOWS
bool FileIO::CreateNew(const char* filename)
{
	Close();

	return !!(flags & open);
}
#endif // !WINDOWS

// ---------------------------------------------------------------------------
//	ファイルを作り直す
// ---------------------------------------------------------------------------

bool FileIO::Reopen(uint flg)
{
	if (!(flags & open)) return false;
	if ((flags & readonly) && (flg & create)) return false;

	if (flags & readonly) flg |= readonly;

	Close();

#ifdef WINDOWS
	DWORD access = (flg & readonly ? 0 : GENERIC_WRITE) | GENERIC_READ;
	DWORD share = flg & readonly ? FILE_SHARE_READ : 0;
	DWORD creation = flg & create ? CREATE_ALWAYS : OPEN_EXISTING;

	hfile = CreateFile(path, access, share, 0, creation, 0, 0);
	
	flags = (flg & readonly) | (hfile == INVALID_HANDLE_VALUE ? 0 : open);
	SetLogicalOrigin(0);
#endif // WINDOWS

	return !!(flags & open);
}

// ---------------------------------------------------------------------------
//	ファイルを閉じる
// ---------------------------------------------------------------------------

void FileIO::Close()
{
	if (GetFlags() & open)
	{
#ifdef WINDOWS
		CloseHandle(hfile);
#else  // !WINDOWS
		fclose(hfile);
#endif
		flags = 0;
	}
}

// ---------------------------------------------------------------------------
//	ファイル殻の読み出し
// ---------------------------------------------------------------------------

int32 FileIO::Read(void* dest, int32 size)
{
	if (!(GetFlags() & open))
		return -1;
	
#ifdef WINDOWS
	DWORD readsize;
	if (!ReadFile(hfile, dest, size, &readsize, 0))
		return -1;
#else
	size_t readsize;
	readsize = fread (dest, 1, size, hfile);
#endif
	return (int32)readsize;
}

// ---------------------------------------------------------------------------
//	ファイルへの書き出し
// ---------------------------------------------------------------------------

int32 FileIO::Write(const void* dest, int32 size)
{
	if (!(GetFlags() & open) || (GetFlags() & readonly))
		return -1;
	
#ifdef WINDOWS
	DWORD writtensize;
	if (!WriteFile(hfile, dest, size, &writtensize, 0))
		return -1;
	return writtensize;
#else
	return 0;
#endif
}

// ---------------------------------------------------------------------------
//	ファイルをシーク
// ---------------------------------------------------------------------------

bool FileIO::Seek(int32 pos, SeekMethod method)
{
	if (!(GetFlags() & open))
		return false;
	
#ifdef WINDOWS
	DWORD wmethod;
	switch (method)
	{
	case begin:	
		wmethod = FILE_BEGIN; pos += lorigin; 
		break;
	case current:	
		wmethod = FILE_CURRENT; 
		break;
	case end:		
		wmethod = FILE_END; 
		break;
	default:
		return false;
	}

	return 0xffffffff != SetFilePointer(hfile, pos, 0, wmethod);
#else
	int wmethod;
	switch (method) {
	case begin:	
		wmethod = SEEK_SET;
		break;
	case current:	
	    wmethod = SEEK_CUR;
		break;
	case end:		
		wmethod = SEEK_END; 
		break;
	default:
		return false;
	}

	return fseek(hfile, pos, wmethod) != 0;
#endif
}

// ---------------------------------------------------------------------------
//	ファイルの位置を得る
// ---------------------------------------------------------------------------

int32 FileIO::Tellp()
{
	if (!(GetFlags() & open))
		return 0;

#ifdef WINDOWS
	return SetFilePointer(hfile, 0, 0, FILE_CURRENT) - lorigin;
#else
	return (int32)ftell (hfile);
#endif
}

// ---------------------------------------------------------------------------
//	現在の位置をファイルの終端とする
// ---------------------------------------------------------------------------

bool FileIO::SetEndOfFile()
{
	if (!(GetFlags() & open))
		return false;
#ifdef WINNDOWS
	return ::SetEndOfFile(hfile) != 0;
#else
	return fseek (hfile, 0, SEEK_END) != 0;
#endif
}
