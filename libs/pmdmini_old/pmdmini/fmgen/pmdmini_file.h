//	$Id: pmdmini_file.h,v 1.6 1999/11/26 10:14:09 cisc Exp $

#if !defined(win32_file_h)
#define win32_file_h

#include "types.h"
#include "portability.h"

// ---------------------------------------------------------------------------

class FilePath
{
public:
	enum Extractpath_Flags
	{
		extractpath_null		= 0x000000,
		extractpath_drive		= 0x000001,
		extractpath_dir			= 0x000002,
		extractpath_filename	= 0x000004,
		extractpath_ext			= 0x000008,
	};
	
	FilePath();
	
	const TCHAR EmptyChar;
	
	int64 GetFileSize(const TCHAR* filename);
	
	TCHAR* Clear(TCHAR* dest, size_t size);
	bool IsEmpty(const TCHAR* src);
	const TCHAR* GetEmptyStr(void);
	
	size_t Strlen(const TCHAR *str);
	
	int Strcmp(const TCHAR *str1, const TCHAR *str2);
	int Strncmp(const TCHAR *str1, const TCHAR *str2, size_t size);
	int Stricmp(const TCHAR *str1, const TCHAR *str2);
	int Strnicmp(const TCHAR *str1, const TCHAR *str2, size_t size);
	
	TCHAR* Strcpy(TCHAR* dest, const TCHAR* src);
	TCHAR* Strncpy(TCHAR* dest, const TCHAR* src, size_t size);
	TCHAR* Strcat(TCHAR* dest, const TCHAR* src);
	TCHAR* Strncat(TCHAR* dest, const TCHAR* src, size_t count);
	
	const TCHAR* Strchr(const TCHAR *str, TCHAR c);
	const TCHAR* Strrchr(const TCHAR *str, TCHAR c);
	TCHAR* AddDelimiter(TCHAR* str);
	
	void Extractpath(TCHAR *dest, const TCHAR *src, uint flg);
	int Comparepath(TCHAR *filename1, const TCHAR *filename2, uint flg);
	void Makepath(TCHAR *path, const TCHAR *drive, const TCHAR *dir, const TCHAR *fname, const TCHAR *ext);
	void Makepath_dir_filename(TCHAR* path, const TCHAR* dir, const TCHAR* filename);
	TCHAR* ExchangeExt(TCHAR *dest, TCHAR *src, const TCHAR *ext);
	
	TCHAR* CharToTCHAR(TCHAR *dest, const char *src);
	TCHAR* CharToTCHARn(TCHAR *dest, const char *src, size_t count);
	
protected:
	void Splitpath(const TCHAR *path, TCHAR *drive, TCHAR *dir, TCHAR *fname, TCHAR *ext);
	
private:
	FilePath(const FilePath&);
	const FilePath& operator=(const FilePath&);

};


class FileIO
{
public:
	enum Flags
	{
		open		= 0x000001,
		readonly	= 0x000002,
		create		= 0x000004,
	};
	
	enum SeekMethod
	{
		begin = 0, current = 1, end = 2,
	};
	
	enum Error
	{
		success = 0,
		file_not_found,
		sharing_violation,
		unknown = -1
	};
	
public:
	FileIO();
	FileIO(const TCHAR* filename, uint flg = 0);
	virtual ~FileIO();
	
	bool Open(const TCHAR* filename, uint flg = 0);
	bool CreateNew(const TCHAR* filename);
	bool Reopen(uint flg = 0);
	void Close();
	Error GetError() { return error; }
	
	int32 Read(void* dest, int32 len);
	int32 Write(const void* src, int32 len);
	bool Seek(int32 fpos, SeekMethod method);
	int32 Tellp();
	bool SetEndOfFile();
	
	uint GetFlags() { return flags; }
	void SetLogicalOrigin(int32 origin) { lorigin = origin; }
	
private:
	uint flags;
	uint32 lorigin;
	Error error;
	TCHAR* path;
	
#ifdef _WIN32
	HANDLE hfile;
#else
	FILE*	fp;
#endif
	
	FileIO(const FileIO&);
	const FileIO& operator=(const FileIO&);
};


#endif // 
