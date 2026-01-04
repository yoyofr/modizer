#ifndef AUDIOHANDLER_H__
#define AUDIOHANDLER_H__

#include "nestypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (__fastcall *AUDIOHANDLER2)(void*, Int32 *p);
typedef Int32 (__fastcall *AUDIOHANDLER)(void*);
typedef struct NES_AUDIO_HANDLER_TAG {
	Uint fMode;
	AUDIOHANDLER Proc;
	AUDIOHANDLER2 Proc2;
	struct NES_AUDIO_HANDLER_TAG *next;
} NES_AUDIO_HANDLER;

typedef void (__fastcall *VOLUMEHANDLER)(void*, Uint volume);
typedef struct NES_VOLUME_HANDLER_TAG {
	VOLUMEHANDLER Proc;
	struct NES_VOLUME_HANDLER_TAG *next;
} NES_VOLUME_HANDLER;

#ifdef __cplusplus
}
#endif

#endif /* AUDIOHANDLER_H__ */
