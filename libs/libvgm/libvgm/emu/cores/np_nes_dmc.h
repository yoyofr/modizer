#ifndef __NP_NES_DMC_H__
#define __NP_NES_DMC_H__

void* NES_DMC_np_Create(UINT32 clock, UINT32 rate);
void NES_DMC_np_Destroy(void *chip);
void NES_DMC_np_Reset(void *chip);
void NES_DMC_np_SetRate(void* chip, UINT32 rate);
void NES_DMC_np_SetPal(void* chip, bool is_pal);
void NES_DMC_np_SetAPU(void* chip, void* apu_);
UINT32 NES_DMC_np_Render(void* chip, INT32 b[2]);
void NES_DMC_np_SetMemory(void* chip, const UINT8* r);
bool NES_DMC_np_Write(void* chip, UINT16 adr, UINT8 val);
bool NES_DMC_np_Read(void* chip, UINT16 adr, UINT8* val);
void NES_DMC_np_SetClock(void* chip, UINT32 rate);
void NES_DMC_np_SetOption(void* chip, int id, int val);
int NES_DMC_np_GetDamp(void* chip);
void NES_DMC_np_SetMask(void* chip, int m);
void NES_DMC_np_SetStereoMix(void* chip, int trk, INT16 mixl, INT16 mixr);

//YOYOFR
double NES_DMC_np_GetFreq(void *chip, int trk);
int NES_DMC_np_GetKeyOn(void *chip, int trk);
//YOYOFR

#endif	// __NP_NES_DMC_H__
