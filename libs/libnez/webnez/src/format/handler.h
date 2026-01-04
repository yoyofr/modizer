#ifndef HANDLER_H__
#define HANDLER_H__

#include "nestypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (__fastcall *RESETHANDLER)(void*);
typedef void (__fastcall *TERMINATEHANDLER)(void*);

typedef struct NES_RESET_HANDLER_TAG {
	Uint priority;	/* 0(first) - 15(last) */
	RESETHANDLER Proc;
	struct NES_RESET_HANDLER_TAG *next;
} NES_RESET_HANDLER;
#define NES_RESET_SYS_FIRST 4
#define NES_RESET_SYS_NOMAL 8
#define NES_RESET_SYS_LAST 12

typedef struct NES_TERMINATE_HANDLER_TAG {
	TERMINATEHANDLER Proc;
	struct NES_TERMINATE_HANDLER_TAG *next;
} NES_TERMINATE_HANDLER;

void NESReset(void*);
void NESResetHandlerInstall(NES_RESET_HANDLER**, const NES_RESET_HANDLER *ph);
void NESTerminate(void*);
void NESTerminateHandlerInstall(NES_TERMINATE_HANDLER**, const NES_TERMINATE_HANDLER *ph);
void NESHandlerInitialize(NES_RESET_HANDLER**, NES_TERMINATE_HANDLER *);
void NESHandlerTerminate(NES_RESET_HANDLER**, NES_TERMINATE_HANDLER *);

#ifdef __cplusplus
}
#endif

#endif /* HANDLER_H__ */
