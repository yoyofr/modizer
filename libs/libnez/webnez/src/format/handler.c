#include "handler.h"
#include "nezplug.h"

/* --------------- */
/*  Reset Handler  */
/* --------------- */

void NESReset(void *pNezPlay)
{
	NES_RESET_HANDLER *ph;
	Uint prio;
	if (!pNezPlay) return;
	for (prio = 0; prio < 0x10; prio++)
		for (ph = ((NEZ_PLAY*)pNezPlay)->nrh[prio]; ph; ph = ph->next) ph->Proc(pNezPlay);
}
static void InstallPriorityResetHandler(NES_RESET_HANDLER **nrh, const NES_RESET_HANDLER *ph)
{
	NES_RESET_HANDLER *nh;
	Uint prio = ph->priority;
	if (prio > 0xF) prio = 0xF;

	/* Add to tail of list*/
	nh = XMALLOC(sizeof(NES_RESET_HANDLER));
	nh->priority = prio;
	nh->Proc = ph->Proc;
	nh->next = 0;

	if (nrh[prio])
	{
		NES_RESET_HANDLER *p = nrh[prio];
		while (p->next) p = p->next;
		p->next = nh;
	}
	else
	{
		nrh[prio] = nh;
	}
}
void NESResetHandlerInstall(NES_RESET_HANDLER** nrh, const NES_RESET_HANDLER *ph)
{
	for (; ph->Proc; ph++) InstallPriorityResetHandler(nrh, ph);
}
static void NESResetHandlerInitialize(NES_RESET_HANDLER **nrh)
{
	Uint prio;
	for (prio = 0; prio < 0x10; prio++) nrh[prio] = 0;
}
static void NESResetHandlerTerminate(NES_RESET_HANDLER **nrh)
{
	Uint prio;
	NES_RESET_HANDLER *next, *p;
	for (prio = 0; prio < 0x10; prio++) {
		if (nrh[prio]) {
			if (nrh[prio]->next) {
				p = nrh[prio]->next;
				while (p) {
					next = p->next;
					XFREE(p);
					p = next;
				}
			}
			XFREE(nrh[prio]);
		}
	}
}


/* ------------------- */
/*  Terminate Handler  */
/* ------------------- */
void NESTerminate(void *pNezPlay)
{
	NES_TERMINATE_HANDLER *ph;
	if (!pNezPlay) return;
	for (ph = ((NEZ_PLAY*)pNezPlay)->nth; ph; ph = ph->next) ph->Proc(pNezPlay);
	NESHandlerTerminate(((NEZ_PLAY*)pNezPlay)->nrh, ((NEZ_PLAY*)pNezPlay)->nth);
}
void NESTerminateHandlerInstall(NES_TERMINATE_HANDLER **nth, const NES_TERMINATE_HANDLER *ph)
{
	/* Add to head of list*/
	NES_TERMINATE_HANDLER *nh;
	nh = XMALLOC(sizeof(NES_TERMINATE_HANDLER));
	nh->Proc = ph->Proc;
	nh->next = *nth;
	*nth = nh;
}
static void NESTerminateHandlerInitialize(NES_TERMINATE_HANDLER **nth)
{
	*nth = 0;
}

static void NESTerminateHandlerTerminate(NES_TERMINATE_HANDLER **nth)
{
	NES_TERMINATE_HANDLER *p = *nth, *next;
	while (p) {
		next = p->next;
		XFREE(p);
		p = next;
	}
}

void NESHandlerInitialize(NES_RESET_HANDLER** nrh, NES_TERMINATE_HANDLER* nth)
{
	NESResetHandlerInitialize(nrh);
	NESTerminateHandlerInitialize(&nth);
}

void NESHandlerTerminate(NES_RESET_HANDLER** nrh, NES_TERMINATE_HANDLER* nth)
{
	NESResetHandlerTerminate(nrh);
	NESTerminateHandlerTerminate(&nth);
}
