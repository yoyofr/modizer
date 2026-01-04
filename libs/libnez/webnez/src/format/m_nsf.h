#ifndef M_NSF_H__
#define M_NSF_H__

#include "nestypes.h"
#include "nezplug.h"
#include "nsf6502.h"
#include "device/kmsnddev.h"
#include "km6502/km6502.h"

#ifdef __cplusplus
extern "C" {
#endif

struct NSF6502 {
	Uint32 cleft;	/* fixed point */
	Uint32 cps;		/* cycles per sample:fixed point */
	Uint32 cycles;
	Uint32 fcycles;
	Uint32 cpf[2];		/* cycles per frame */
	Uint32 total_cycles;
	Uint32 iframe;
	Uint8  breaked;
	Uint8  palntsc;
	Uint8  rom[0x10];		/* nosefart rom */
};

typedef struct NSFNSF_TAG {
	NES_READ_HANDLER  *(nprh[0x10]);
	NES_WRITE_HANDLER *(npwh[0x10]);
	struct K6502_Context work6502;
	struct NSF6502 nsf6502;
	Uint work6502_BP;		/* Break Point */
	Uint work6502_start_cycles;
	Uint8 *bankbase;
	Uint32 banksw;
	Uint32 banknum;
	Uint8 head[0x80];		/* nsf header */
	Uint8 ram[0x800];
#if !NSF_MAPPER_STATIC
	Uint8 *bank[8];
	Uint8 static_area[0x8000 - 0x6000];
	Uint8 zero_area[0x1000];
#else
	Uint8 static_area[0x10000 - 0x6000];
#endif
	unsigned fds_type;
	Int32 apu_volume;
	Int32 dpcm_volume;
	void* apu;
	void* fdssound;
	void* n106s;
	void* vrc6s;
	void* sndp;
	void* mmc5;
	void* psgs;

	Uint8 counter2002;		/* Žb’è */
	Int32 dpcmirq_ct;
	Uint8 vsyncirq_fg;	/* $4015‚Ì6bit–Ú‚ð—§‚½‚¹‚é‚â‚Â */
} NSFNSF;

/* NSF player */
Uint NSFLoad(NEZ_PLAY*, Uint8 *pData, Uint uSize);
Uint8 *NSFGetHeader(NEZ_PLAY*);
Uint NSFDeviceInitialize(NEZ_PLAY*);

#ifdef __cplusplus
}
#endif

#endif /* M_NSF_H__ */
