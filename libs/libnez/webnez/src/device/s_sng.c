#include "kmsnddev.h"
#include "divfix.h"
#include "s_logtbl.h"
#include "s_sng.h"

//TODO:  MODIZER changes start / YOYOFR
#define MDZ_VOL_BOOST 3
#include "../../../../src/ModizerVoicesData.h"
extern int nezChan_output[4+11];
//TODO:  MODIZER changes end / YOYOFR


#define CPS_SHIFT 18
#define LOG_KEYOFF (31 << LOG_BITS)

#define FB_WNOISE   0xc000
#define FB_PNOISE   0x8000

#define SN76489AN_PRESET 0x0001
//#define SG76489_PRESET 0x0F35
#define SG76489_PRESET 0x0001

#define RENDERS 7
#define NOISE_RENDERS 2

typedef struct {
	Uint32 cycles;
	Uint32 spd;
	Uint32 vol;
	Uint8 adr;
	Uint8 mute;
	Uint8 pad4[2];
	Uint32 count;
	Uint32 output;
} SNG_SQUARE;

typedef struct {
	Uint32 cycles;
	Uint32 spd;
	Uint32 vol;
	Uint32 rng;
	Uint32 fb;
	Uint8 step1;
	Uint8 step2;
	Uint8 mode;
	Uint8 mute;
	Uint8 pad4[2];
	Uint8 count;
	Uint32 output;
} SNG_NOISE;

typedef struct {
	KMIF_SOUND_DEVICE kmif;
	KMIF_LOGTABLE *logtbl;
	SNG_SQUARE square[3];
	SNG_NOISE noise;
	struct {
		Uint32 cps;
		Uint32 ncps;
		Int32 mastervolume;
		Uint8 first;
		Uint8 ggs;
	} common;
	Uint8 type;
	Uint8 regs[0x11];
} SNGSOUND;

#define V(a) (((a * (1 << LOG_BITS)) / 3) << 1)
const static Uint32 voltbl[16] = {
	V(0x0), V(0x1), V(0x2),V(0x3),V(0x4), V(0x5), V(0x6),V(0x7),
	V(0x8), V(0x9), V(0xA),V(0xB),V(0xC), V(0xD), V(0xE),LOG_KEYOFF
};
#undef V

__inline static Int32 SNGSoundSquareSynth(SNGSOUND *sndp, SNG_SQUARE *ch)
{
	Int32 outputbuf=0,count=0;
	if (ch->spd < (0x4 << CPS_SHIFT))
	{
		return LogToLin(sndp->logtbl, ch->vol + sndp->common.mastervolume, LOG_LIN_BITS - 21-2);
	}
	ch->cycles += sndp->common.cps<<RENDERS;
	ch->output = LogToLin(sndp->logtbl, ch->vol + sndp->common.mastervolume, LOG_LIN_BITS - 21-2);
	ch->output *= !(ch->adr & 1);
	while (ch->cycles >= ch->spd)
	{
		outputbuf += ch->output;
		count++;

		ch->cycles -= ch->spd;
		ch->count++;
		if(ch->count >= 1<<RENDERS){
			ch->count = 0;
			ch->adr++;
			ch->output = LogToLin(sndp->logtbl, ch->vol + sndp->common.mastervolume, LOG_LIN_BITS - 21-2);
			ch->output *= !(ch->adr & 1);
		}
	}
	outputbuf += ch->output;
	count++;
	if (ch->mute) return 0;
	return outputbuf / count;
}

__inline static Int32 SNGSoundNoiseSynth(SNGSOUND *sndp, SNG_NOISE *ch)
{
	Int32 outputbuf=0,count=0;
	//if (ch->spd < (0x1 << (CPS_SHIFT - 1))) return 0;
	ch->cycles += (sndp->common.ncps >> 1) <<NOISE_RENDERS;
	ch->output = LogToLin(sndp->logtbl, ch->vol + sndp->common.mastervolume, LOG_LIN_BITS - 21-2);
	ch->output *= !(ch->rng & 1);
	while (ch->cycles >= ch->spd)
	{
		outputbuf += ch->output;
		count++;

		ch->cycles -= ch->spd;
		ch->count++;
		if(ch->count >= 1<<NOISE_RENDERS){
			ch->count = 0;
			if (ch->rng & 1) ch->rng ^= ch->fb;
			//ch->rng += ch->rng + (((ch->rng >> ch->step1)/* ^ (ch->rng >> ch->step2)*/) & 1);
			ch->rng >>= 1;

			ch->output = LogToLin(sndp->logtbl, ch->vol + sndp->common.mastervolume, LOG_LIN_BITS - 21-2);
			ch->output *= !(ch->rng & 1);
		}
	}
	outputbuf += ch->output;
	count++;
	if (ch->mute) return 0;
	return outputbuf / count;
}

static void SNGSoundSquareReset(SNG_SQUARE *ch)
{
	XMEMSET(ch, 0, sizeof(SNG_SQUARE));
	ch->vol = LOG_KEYOFF;
}

static void SNGSoundNoiseReset(SNG_NOISE *ch)
{
	XMEMSET(ch, 0, sizeof(SNG_NOISE));
	ch->vol = LOG_KEYOFF;
	//ch->rng = sndp->type == SNG_TYPE_SN76496 ? SN76489AN_PRESET : SG76489_PRESET;
}


static void sndsynth(void *ctx, Int32 *p)
{
	SNGSOUND *sndp = (SNGSOUND*)ctx;
	Uint32 ch;
	Int32 accum = 0;
    
    //YOYOFR
    for (int c=0;c<3;c++) {
        sndp->square[c].mute=generic_mute_mask&(1<<c);
    }
    sndp->noise.mute=generic_mute_mask&(1<<3);
    //YOYOFR
    
	for (ch = 0; ch < 3; ch++)
	{
		accum = SNGSoundSquareSynth(sndp, &sndp->square[ch]);
		if (chmask[DEV_SN76489_SQ1 + ch]){
			if ((sndp->common.ggs >> ch) & 0x10) p[0] += accum;
			if ((sndp->common.ggs >> ch) & 0x01) p[1] += accum;
            
            if ((sndp->common.ggs >> ch) & 0x11) nezChan_output[ch] += accum; //YOYOFR
		}
	}
	accum = SNGSoundNoiseSynth(sndp, &sndp->noise) * chmask[DEV_SN76489_NOISE];
    
	if (sndp->common.ggs & 0x80) p[0] += accum;
	if (sndp->common.ggs & 0x08) p[1] += accum;
    
    if (sndp->common.ggs & 0x88) nezChan_output[3] += accum; //YOYOFR
    
    //YOYOFR
    for (int c=0;c<3;c++) {
        if (chmask[DEV_SN76489_SQ1 + c]){
            if (((sndp->common.ggs >> c) & 0x11) && sndp->square[c].spd && sndp->square[c].vol) {
                double half_period_samples = (double)sndp->square[c].spd / (1 << CPS_SHIFT);
                vgm_last_note[c]=(float)(44100 / (2.0 * half_period_samples));
                vgm_last_instr[c]=c;
                vgm_last_vol[c]=(sndp->square[c].vol >> LOG_BITS)+1;
            }
        }
    }
    if (chmask[DEV_SN76489_NOISE]) {
        if (sndp->noise.vol < LOG_KEYOFF && sndp->noise.spd) {
            uint8_t noise_mode = sndp->noise.step1 & 0x03; // NF bits [1:0]
            uint8_t is_white = (sndp->noise.fb == FB_WNOISE);
            
            // Mode 3 = slavé sur canal 3 → pas de fréquence propre au noise
            // → utiliser la fréquence du canal 3 ou ignorer
            if (noise_mode == 3) {
                // slavé : la "fréquence" est celle du canal tone 3
                if (sndp->square[2].spd) {
                    double half_period = (double)sndp->square[2].spd / (1 << CPS_SHIFT);
                    vgm_last_note[3] = (float)(44100 / (2.0 * half_period));
                } else {
                    vgm_last_note[3] = 0;
                }
            } else {
                // Modes 0/1/2 : fréquences fixes clock/512, /1024, /2048
                // ncps est le cps du noise (peut différer de cps)
                if (sndp->noise.spd) {
                    double half_period = (double)sndp->noise.spd / (1 << CPS_SHIFT);
                    vgm_last_note[3] = (float)(44100 / (2.0 * half_period));
                } else {
                    vgm_last_note[3] = 0;
                }
            }
            
            vgm_last_instr[3] = is_white ? 4 : 3; // distinguer bruit blanc/périodique
            vgm_last_vol[3] = (sndp->noise.vol >> LOG_BITS) + 1;
        }
    }
    //YOYOFR
}

static void sndvolume(void *ctx, Int32 volume)
{
	SNGSOUND *sndp = (SNGSOUND*)ctx;
	volume = (volume << (LOG_BITS - 8)) << 1;
	sndp->common.mastervolume = volume;
}

static Uint32 sndread(void *ctx, Uint32 a)
{
	return 0;
}

static void sndwrite(void *ctx, Uint32 a, Uint32 v)
{
	SNGSOUND *sndp = (SNGSOUND*)ctx;
	if ((a & 1) && sndp->type  == SNG_TYPE_GAMEGEAR)
	{
		if (sndp->type == SNG_TYPE_GAMEGEAR) sndp->common.ggs = v;
		sndp->regs[0x10] = v;
	}
	else if (sndp->common.first)
	{
		Uint32 ch = (sndp->common.first >> 5) & 3;
		if (sndp->type == SNG_TYPE_SN76489AN) {
			//0x000Ç™àÍî‘í·Ç≠ÅA0x001Ç™àÍî‘çÇÇ¢ÅB
			sndp->square[ch].spd = (((((v & 0x3F) << 4) + (sndp->common.first & 0xF)) + 0x3ff)&0x3ff) +1;
		} else {
			sndp->square[ch].spd = (((v & 0x3F) << 4) + (sndp->common.first & 0xF));
		}
		sndp->regs[ch*2]=sndp->square[ch].spd&0xff;
		sndp->regs[ch*2+1]=(sndp->square[ch].spd>>8) & 0x03;

		sndp->square[ch].spd <<= CPS_SHIFT;
		if (ch == 2 && sndp->noise.mode == 3)
		{
			sndp->noise.spd = ((sndp->square[2].spd ? sndp->square[2].spd : (1<<CPS_SHIFT)) & (0x7ff<<(CPS_SHIFT)));
		}
		sndp->common.first = 0;
	}
	else
	{
		Uint32 ch;
		if(v >= 0x80)sndp->regs[(v & 0xF0)>>4]=v;
		switch (v & 0xF0)
		{
			case 0x80:	case 0xA0:	case 0xC0:
				sndp->common.first = v;
				break;
			case 0x90:	case 0xB0:	case 0xD0:
				ch = (v & 0x60) >> 5;
				sndp->square[ch].vol = voltbl[v & 0xF];
				break;
			case 0xE0:
				//éËéùÇøÇÃSN76489ANÇ™ÅAÇ±Ç±Ç…èëÇ¢ÇΩÇÁÉäÉZÉbÉgÇµÇƒÇΩÇÃÇ≈
				sndp->noise.rng = sndp->type == SNG_TYPE_SN76489AN ? SN76489AN_PRESET : SG76489_PRESET;
				sndp->noise.mode = v & 0x3;
				sndp->noise.fb = (v & 4) ? FB_WNOISE : FB_PNOISE;
				//sndp->noise.step1 = (v & 4) ? (14) : (3);
				//sndp->noise.step2 = (v & 4) ? (13) : (3);

				if (sndp->noise.mode == 3)
					sndp->noise.spd = ((sndp->square[2].spd ? sndp->square[2].spd : (1<<CPS_SHIFT)) & (0x7ff<<(CPS_SHIFT)));
				else
					sndp->noise.spd = 1 << (4 + sndp->noise.mode + CPS_SHIFT);
				break;
			case 0xF0:
				sndp->noise.vol = voltbl[v & 0xF];
				break;
		}
	}

}

static void sndreset(void *ctx, Uint32 clock, Uint32 freq)
{
	SNGSOUND *sndp = (SNGSOUND*)ctx;
	XMEMSET(&sndp->common, 0, sizeof(sndp->common));
	sndp->common.cps = DivFix(clock, 16 * freq, CPS_SHIFT);
	sndp->common.ncps = DivFix(clock, (sndp->type == SNG_TYPE_SN76489AN ? 16 : 17) * freq, CPS_SHIFT);
	sndp->common.ggs = 0xff;
	sndp->regs[0x10] = sndp->common.ggs;
	SNGSoundSquareReset(&sndp->square[0]);
	SNGSoundSquareReset(&sndp->square[1]);
	SNGSoundSquareReset(&sndp->square[2]);
	SNGSoundNoiseReset(&sndp->noise);
	sndwrite(sndp, 0, 0xE0);
	sndwrite(sndp, 0, 0x9F);
	sndwrite(sndp, 0, 0xBF);
	sndwrite(sndp, 0, 0xDF);
	sndwrite(sndp, 0, 0xFF);
	sndwrite(sndp, 0, 0x80);
	sndwrite(sndp, 0, 0x00);
	sndwrite(sndp, 0, 0xA0);
	sndwrite(sndp, 0, 0x00);
	sndwrite(sndp, 0, 0xC0);
	sndwrite(sndp, 0, 0x00);
}

static void sndrelease(void *ctx)
{
	SNGSOUND *sndp = (SNGSOUND*)ctx;
	if (sndp) {
		if (sndp->logtbl) sndp->logtbl->release(sndp->logtbl->ctx);
		XFREE(sndp);
	}
}

static void setinst(void *ctx, Uint32 n, void *p, Uint32 l){}

//Ç±Ç±Ç©ÇÁÉåÉWÉXÉ^ÉrÉÖÉAÅ[ê›íË
static Uint8 *regdata;
Uint32 (*ioview_ioread_DEV_SN76489)(Uint32 a);
static Uint32 ioview_ioread_bf(Uint32 a){
	switch(a){
	case 0:	case 1:	case 2:	case 3:	case 4:	case 5:	case 0x10:
		return regdata[a];
	case 9:	case 0xb:	case 0xd:	case 0xe:	case 0xf:	
		return regdata[a]&0xf;
	}
	return 0x100;
}
//Ç±Ç±Ç‹Ç≈ÉåÉWÉXÉ^ÉrÉÖÉAÅ[ê›íË

KMIF_SOUND_DEVICE *SNGSoundAlloc(Uint32 sng_type)
{
	SNGSOUND *sndp;
	sndp = XMALLOC(sizeof(SNGSOUND));
	if (!sndp) return 0;
	XMEMSET(sndp, 0, sizeof(SNGSOUND));
	sndp->type = sng_type;
	sndp->kmif.ctx = sndp;
	sndp->kmif.release = sndrelease;
	sndp->kmif.reset = sndreset;
	sndp->kmif.synth = sndsynth;
	sndp->kmif.volume = sndvolume;
	sndp->kmif.write = sndwrite;
	sndp->kmif.read = sndread;
	sndp->kmif.setinst = setinst;
	sndp->logtbl = LogTableAddRef();
	if (!sndp->logtbl)
	{
		sndrelease(sndp);
		return 0;
	}
	//Ç±Ç±Ç©ÇÁÉåÉWÉXÉ^ÉrÉÖÉAÅ[ê›íË
	regdata = sndp->regs;
	ioview_ioread_DEV_SN76489 = ioview_ioread_bf;
	//Ç±Ç±Ç‹Ç≈ÉåÉWÉXÉ^ÉrÉÖÉAÅ[ê›íË
	return &sndp->kmif;
}
