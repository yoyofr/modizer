#include <olectl.h>
#include <ocidl.h>

#ifdef __cplusplus
extern "C" {
#endif

IPicture * LoadPic(LPCTSTR lpName, LPCTSTR lpType, HMODULE hInst);
HRESULT IPicture_getHandle(IPicture * pPicture, OLE_HANDLE * pHandle);
ULONG IPicture_Release(IPicture * pPicture);

#ifdef __cplusplus
}
#endif
