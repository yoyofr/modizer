#ifndef SONGINFO_H__
#define SONGINFO_H__

#include "nestypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SONG_INFO_TAG
{
	Uint songno;
	Uint maxsongno;
	Uint startsongno;
	Uint extdevice;
	Uint initaddress;
	Uint playaddress;
	Uint channel;
	Uint initlimit;

    char title[128];
    char artist[128];	
    char copyright[128];
    char detail[1024];
    char type[32];
    char system[32];
} SONG_INFO;

SONG_INFO* SONGINFO_New();
void SONGINFO_Delete(SONG_INFO *info);
Uint SONGINFO_GetSongNo(SONG_INFO*);
void SONGINFO_SetSongNo(SONG_INFO*, Uint v);
Uint SONGINFO_GetStartSongNo(SONG_INFO*);
void SONGINFO_SetStartSongNo(SONG_INFO*, Uint v);
Uint SONGINFO_GetMaxSongNo(SONG_INFO*);
void SONGINFO_SetMaxSongNo(SONG_INFO*, Uint v);
Uint SONGINFO_GetExtendDevice(SONG_INFO*);
void SONGINFO_SetExtendDevice(SONG_INFO*, Uint v);
Uint SONGINFO_GetInitAddress(SONG_INFO*);
void SONGINFO_SetInitAddress(SONG_INFO*, Uint v);
Uint SONGINFO_GetPlayAddress(SONG_INFO*);
void SONGINFO_SetPlayAddress(SONG_INFO*, Uint v);
Uint SONGINFO_GetChannel(SONG_INFO*);
void SONGINFO_SetChannel(SONG_INFO*, Uint v);

void SONGINFO_SetType(SONG_INFO*,const char *type);
char *SONGINFO_GetType(SONG_INFO*);
void SONGINFO_SetSystem(SONG_INFO*,const char *system);
char *SONGINFO_GetSystem(SONG_INFO*);
void SONGINFO_SetTitle(SONG_INFO*,const char *title);
char *SONGINFO_GetTitle(SONG_INFO*);
void SONGINFO_SetArtist(SONG_INFO*,const char *artist);
char *SONGINFO_GetArtist(SONG_INFO*);
void SONGINFO_SetCopyright(SONG_INFO*, const char *copyright);
char *SONGINFO_GetCopyright(SONG_INFO*);
//YOYOFR
//#ifdef EMSCRIPTEN
void SONGINFO_SetDetail(SONG_INFO*, const char *detail);
char *SONGINFO_GetDetail(SONG_INFO*);
//#endif
void SONGINFO_Reset(SONG_INFO*);

#ifdef __cplusplus
}
#endif

#endif /* SONGINFO_H__ */
