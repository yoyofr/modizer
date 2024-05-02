//=============================================================================
//		File I/O Interface Class : IFILEIO
//=============================================================================

#ifndef IFILEIO_H
#define IFILEIO_H

#include "portability_fmgen.h"

//=============================================================================
//	COM 風 interface class(File I/O interface)
//=============================================================================
// interpositioned IUnknown
class interposIUnknown {
public:
	virtual uint32_t AddRef(void) = 0;
	
	virtual uint32_t Release(void) = 0;
};

class IFILEIO : public interposIUnknown {
public:
	enum Flags
	{
		flags_open		= 0x000001,
		flags_readonly	= 0x000002,
		flags_create	= 0x000004,
	};
	
	enum SeekMethod
	{
		seekmethod_begin = 0, seekmethod_current = 1, seekmethod_end = 2,
	};
	
	enum Error
	{
		error_success = 0,
		error_file_not_found,
		error_sharing_violation,
		error_unknown = -1
	};
	
	virtual int64_t WINAPI GetFileSize(const TCHAR* filename) = 0;
	virtual bool WINAPI Open(const TCHAR* filename, uint32_t flg = 0) = 0;
	virtual void WINAPI Close() = 0;
	virtual int32_t WINAPI Read(void* dest, int32_t len) = 0;
	virtual bool WINAPI Seek(int32_t fpos, SeekMethod method) = 0;
	virtual int32_t WINAPI Tellp() = 0;
};


//=============================================================================
//	COM 風 interface class(IFILEIO 設定)
//=============================================================================
class ISETFILEIO : public interposIUnknown {
public:
	virtual void WINAPI setfileio(IFILEIO* pfileio) = 0;
};




#endif	// IFILEIO_H
