#ifndef NSF6502_H__
#define NSF6502_H__

#include "handler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef Uint (__fastcall *READHANDLER)(void*, Uint a);
typedef void (__fastcall *WRITEHANDLER)(void*, Uint a, Uint v);

typedef struct NES_READ_HANDLER_TAG {
	Uint min;
	Uint max;
	READHANDLER Proc;
	struct NES_READ_HANDLER_TAG *next;
} NES_READ_HANDLER;

typedef struct NES_WRITE_HANDLER_TAG {
	Uint min;
	Uint max;
	WRITEHANDLER Proc;
	struct NES_WRITE_HANDLER_TAG *next;
} NES_WRITE_HANDLER;

void NESMemoryHandlerInitialize(NEZ_PLAY *);
void NESReadHandlerInstall(NEZ_PLAY *, NES_READ_HANDLER *ph);
void NESWriteHandlerInstall(NEZ_PLAY *, NES_WRITE_HANDLER *ph);

Uint NSF6502Install(NEZ_PLAY*);
Uint NES6502GetCycles(NEZ_PLAY*);
void NES6502Irq(NEZ_PLAY*);
//void NES6502SetIrqCount(NEZ_PLAY *pNezPlay, Int A);
Uint NES6502ReadDma(NEZ_PLAY*, Uint A);
Uint NES6502Read(NEZ_PLAY*, Uint A);
void NES6502Write(NEZ_PLAY*, Uint A, Uint V);


#ifdef __cplusplus
}
#endif

#endif /* NSF6502_H__ */
