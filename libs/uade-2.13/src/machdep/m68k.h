 /* 
  * UAE - The Un*x Amiga Emulator
  * 
  * MC68000 emulation - machine dependent bits
  *
  * Copyright 1996 Bernd Schmidt
  */

struct flag_struct {
    unsigned int cznv;
    unsigned int x;
};

#define SET_ZFLG(y) (regflags.cznv = (regflags.cznv & ~0x40) | (((y) & 1) << 6))
#define SET_CFLG(y) (regflags.cznv = (regflags.cznv & ~1) | ((y) & 1))
#define SET_VFLG(y) (regflags.cznv = (regflags.cznv & ~0x800) | (((y) & 1) << 11))
#define SET_NFLG(y) (regflags.cznv = (regflags.cznv & ~0x80) | (((y) & 1) << 7))
#define SET_XFLG(y) (regflags.x = (y))

#define GET_ZFLG ((regflags.cznv >> 6) & 1)
#define GET_CFLG (regflags.cznv & 1)
#define GET_VFLG ((regflags.cznv >> 11) & 1)
#define GET_NFLG ((regflags.cznv >> 7) & 1)
#define GET_XFLG (regflags.x & 1)

#define CLEAR_CZNV (regflags.cznv = 0)
#define COPY_CARRY (regflags.x = regflags.cznv)

extern struct flag_struct regflags;

static inline int cctrue(int cc)
{
    uae_u32 cznv = regflags.cznv;
    switch(cc){
     case 0: return 1;                       /* T */
     case 1: return 0;                       /* F */
     case 2: return (cznv & 0x41) == 0; /* !GET_CFLG && !GET_ZFLG;  HI */
     case 3: return (cznv & 0x41) != 0; /* GET_CFLG || GET_ZFLG;    LS */
     case 4: return (cznv & 1) == 0;        /* !GET_CFLG;               CC */
     case 5: return (cznv & 1) != 0;           /* GET_CFLG;                CS */
     case 6: return (cznv & 0x40) == 0; /* !GET_ZFLG;               NE */
     case 7: return (cznv & 0x40) != 0; /* GET_ZFLG;                EQ */
     case 8: return (cznv & 0x800) == 0;/* !GET_VFLG;               VC */
     case 9: return (cznv & 0x800) != 0;/* GET_VFLG;                VS */
     case 10:return (cznv & 0x80) == 0; /* !GET_NFLG;               PL */
     case 11:return (cznv & 0x80) != 0; /* GET_NFLG;                MI */
     case 12:return (((cznv << 4) ^ cznv) & 0x800) == 0; /* GET_NFLG == GET_VFLG;             GE */
     case 13:return (((cznv << 4) ^ cznv) & 0x800) != 0;/* GET_NFLG != GET_VFLG;             LT */
     case 14:
	cznv &= 0x8c0;
	return (((cznv << 4) ^ cznv) & 0x840) == 0; /* !GET_ZFLG && (GET_NFLG == GET_VFLG);  GT */
     case 15:
	cznv &= 0x8c0;
	return (((cznv << 4) ^ cznv) & 0x840) != 0; /* GET_ZFLG || (GET_NFLG != GET_VFLG);   LE */
    }
    abort();
    return 0;
}
