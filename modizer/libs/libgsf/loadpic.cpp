#include "loadpic.h"

IPicture * LoadPic(LPCTSTR lpName, LPCTSTR lpType, HMODULE hInst)
{
	HRSRC		hResInfo;
	HANDLE		hRes;
	LPSTR		lpRes	= NULL; 
	IPicture	* m_pPicture = NULL;

	// Find the resource
	hResInfo = FindResource(hInst, lpName, lpType);

	if (hResInfo == NULL) 
		return 0;

	// Load the resource
	hRes = LoadResource(hInst, hResInfo);

	if (hRes == NULL) 
		return 0;

	// Lock the resource
	lpRes = (char*)LockResource(hRes);

	if (lpRes != NULL)
	{
		int nSize = SizeofResource(hInst, hResInfo);
		HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, nSize);
		void* pData = GlobalLock(hGlobal);
		memcpy(pData, lpRes, nSize);
		GlobalUnlock(hGlobal);

		IStream* pStream = NULL;

		if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) == S_OK)
		{
			if (OleLoadPicture(pStream, nSize, FALSE, IID_IPicture, (LPVOID *)&m_pPicture) != S_OK)
			{
				m_pPicture = NULL;
			}
			pStream->Release();
		}
		UnlockResource(hRes);
	}

	// Free the resource
	FreeResource(hRes);

	return m_pPicture;
}

HRESULT IPicture_getHandle(IPicture * pPicture, OLE_HANDLE * pHandle)
{
	if (pPicture) return pPicture->get_Handle(pHandle);
	return E_POINTER;
}

ULONG IPicture_Release(IPicture * pPicture)
{
	if (pPicture) return pPicture->Release();
	return 0;
}
