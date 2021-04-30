//=============================================================================
//	Professional Music Driver [P.M.D.] version 4.8
//		Constant Tables
//			Programmed By M.Kajihara
//			Windows Converted by C60
//=============================================================================

#ifndef TABLE_H
#define TABLE_H

typedef unsigned int uint;

typedef struct efftbltag
{
	int			priority;
	const int	*table;
} EFFTBL;


extern const int part_table[][3];
extern const int fnum_data[];
extern const int psg_tune_data[];
extern const int pcm_tune_data[];
extern const uint p86_tune_data[];
extern const int ppz_tune_data[];
extern const int carrier_table[];
extern const int rhydat[][3];
extern const int ppzpandata[];
extern const EFFTBL efftbl[];

#endif	// TABLE_H
