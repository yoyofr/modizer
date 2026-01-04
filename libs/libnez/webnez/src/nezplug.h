#ifndef NEZPLUG_H__
#define NEZPLUG_H__

#include "nestypes.h"
#include "format/songinfo.h"
#include "format/handler.h"
#include "format/audiohandler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NEZPLAY_TAG {
	SONG_INFO *song;
	Uint volume;
	Uint frequency;
	Uint channel;
	NES_AUDIO_HANDLER *nah;
	NES_VOLUME_HANDLER *nvh;
	NES_RESET_HANDLER *(nrh[0x10]);
	NES_TERMINATE_HANDLER *nth;
	Uint naf_type;
	Uint32 naf_prev[2];
	void *nsf;
	void *gbrdmg;
	void *heshes;
	void *zxay;
	void *kssseq;
	void *sgcseq;
	void *nsdp;
} NEZ_PLAY;

/* NEZ player */
NEZ_PLAY* NEZNew();
void NEZDelete(NEZ_PLAY*);

Uint NEZLoad(NEZ_PLAY*, Uint8*, Uint);
void NEZSetSongNo(NEZ_PLAY*, Uint uSongNo);
void NEZSetFrequency(NEZ_PLAY*, Uint freq);
void NEZSetChannel(NEZ_PLAY*, Uint ch);
void NEZReset(NEZ_PLAY*);
void NEZSetFilter(NEZ_PLAY *, Uint filter);
void NEZVolume(NEZ_PLAY*, Uint uVolume);
void NEZAPUVolume(NEZ_PLAY*, Int32 uVolume);
void NEZDPCMVolume(NEZ_PLAY*, Int32 uVolume);
void NEZRender(NEZ_PLAY*, void *bufp, Uint buflen);

Uint NEZGetSongNo(NEZ_PLAY*);
Uint NEZGetSongStart(NEZ_PLAY*);
Uint NEZGetSongMax(NEZ_PLAY*);
Uint NEZGetChannel(NEZ_PLAY*);
Uint NEZGetFrequency(NEZ_PLAY*);
void NEZGetFileInfo(char **p1, char **p2, char **p3, char **p4);

#ifdef __cplusplus
}
#endif

#endif /* NEZPLUG_H__ */
