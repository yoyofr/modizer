/* ***************************************************************
CHIPNSFX interactive three-channel musical tracker and data editor
written by Cesar Nicolas-Gonzalez / CNGSOFT since 2017-02-09 14:20
*************************************************************** */



#define MY_FILE "CHIPNSFX"
#define MY_DATE "20240328"
#define MY_NAME "CHIPNSFX tracker"
#define MY_SELF "Cesar Nicolas-Gonzalez / CNGSOFT"

#define GPL_3_INFO \
	"This program comes with ABSOLUTELY NO WARRANTY; for more details" "\n" \
	"please see the GNU General Public License. This is free software" "\n" \
	"and you are welcome to redistribute it under certain conditions."

/* This notice applies to the sources of CHIPNSFX and its binaries.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Contact information: <mailto:cngsoft@gmail.com> */

#define INLINE // 'inline': useless in TCC and GCC4, harmful in GCC5!
INLINE int ucase(int i) { return i>='a'&&i<='z'?i-32:i; }
INLINE int lcase(int i) { return i>='A'&&i<='Z'?i+32:i; }
#define length(x) (sizeof(x)/sizeof(*(x)))

#include <stdio.h>
#include <stdlib.h> // malloc...
#include <string.h> // strcpy...

#include "chp2ym.h"

#define MAXSTRLEN 1024 // a wave frame (44100*2/100=882) or a pattern ("#01 A-4:01 ...":7*(1+128)=903) must fit inside
FILE *f; // I/O operations never require more than one file at once

#define CHIPNSFX_SIZE 112 // 0x70, note/control code threshold: in the target encoding, 0x00..0x6B are notes (C-0..B-8),
	// 0x6C..0x6F are "SFX", "===", "???" and "^^^", 0x70..0xEF are lengths (1..128) and 0xF0..0xFF are commands.
	// "???"/110 is used only when generating length-optimisation "hollow" notes from the source to the target.
	// Notice that within the tracker 0 means "...", 1..108 mean C-0..B-8, 109 is "SFX", 110 is "===" and 112 is "^^^"!

const char chipnsfxid[]=MY_FILE " format",chipnsfxlf[]="===\n",crlf[]="\n", // file format strings
	vubars[][17]={
		#ifdef SDL2
		" ____\034\034\034\034\035\035\035\036\036\037\037"//custom font
		#else
		" ....ooooOOO88@@"//pure ASCII
		//"\040\040\372\372\055\055\376\376\260\260\261\261\262\262\333\333"//OEM chars
		#endif
		,"0123456789ABCDEF"}, // note amplitude, visual or numeric
	notes1[]="CCDDEFFGGAAB",notes2[]="-#-#--#-#-#-"; // C- C# D- D# E- F- F# G- G# A- A# B-

char pianos[]="zsxdcvgbhnjm,q2w3er5t6y7ui9o0pl1a."; // standard QWERTY keyboard: 12+12+6+SFX+REST+BRAKE+ERASE
const char pianoz[][sizeof(pianos)]={
	"ysxdcvgbhnjm,q2w3er5t6z7ui9o0pl1a.", // QWERTZ keyboard
	#ifdef SDL2
	"wsxdcvgbhnj,;a\351z\042er(t-y\350ui\230o\340pl&q:" // AZERTY keyboard, SDL2:ANSI
	#else
	"wsxdcvgbhnj,;a\202z\042er(t-y\212ui\207o\205pl&q:" // AZERTY keyboard, WIN32:OEM
	#endif
	};

// song header fields
int hertz,divisor,transpose,loopto,orders,instrs,loopto_,orders_,instrs_;
// song body contents
unsigned char tmp[MAXSTRLEN],*title0="song_", // work buffer and output label
	title1[MAXSTRLEN],title2[MAXSTRLEN], // song name and signature
	filepath[MAXSTRLEN], // song location
	order[3][256][2],
	order_[3][256][2], // 3 channels * 256 orders * index & transposition
	scrln[256],
	scrln_[256], // 256 score pattern lengths ranging from 0 (void) to 128 (full)
	score[256][128][2],
	score_[256][128][2], // 256 score patterns * 128 rows * note+instrument
	instr[256][25],
	instr_[256][25], // 256 instruments * 8 bytes + 16 ASCIZ + NULL
	clipboard[3][256][2];
int clipboardtype,clipboardwidth,clipboarditems;

// general purpose operations --------------------------------- //

#define MEMZERO(v) memset((v),0,sizeof((v)))
#define MEMFULL(v) memset((v),255,sizeof((v)))
#define MEMSAVE(v,w) memcpy((v),(w),sizeof((w)))
#define MEMLOAD(v,w) memcpy((v),(w),sizeof((v)))
INLINE int itoradix(int x) { return x<10?x+48:((x-9)+64); } // base36: turns 0..35 into 0123..XYZ;
INLINE int radixtoi(int x) { return x<65?x-48:((x+9)&15); } // flawed on purpose, allowing ":" as 10
char *strcln(unsigned char *s) // remove trailing whitespaces from a string
{
	if (!s)
		return NULL;
	unsigned char *r=s;
	while (*r) ++r;
	while (r>s&&(*--r<33)) *r=0;
	return s;
}
char *strpad(char *s,int l) // pad (or clip!) string with up to `l` spaces
{
	char *r=s;
	while (l&&*r) ++r,--l;
	while (l--) *r++=' ';
	*r=0;
	return s;
}
int strint(const char *s,int i) // perform strchr(s,i) and sanitize the output
	{ const char *t=strchr(s,i); return t?t-s:-1; }

INLINE void keepwithin(int *i,int l,int h) { if (*i<l) *i=l; else if (*i>h) *i=h; }

int flag_q=0,flag_q_last,flag_q_next; // onscreen percentage counters
#define flag_q_init(i) flag_q_last=(i)+1
void flag_q_loop(int i) { if (!flag_q&&!(7&++flag_q_next)) fprintf(stderr,"%02d%%\x0D",100*i/flag_q_last); }
void flag_q_exit(int i) { if (!flag_q) fprintf(stderr,"%d.\n",i); }

// song file I/O ---------------------------------------------- //

char musical_[]="::::::";
char *musical(int n,int i) // note formatting, f.e. `n=57,i=1` into "A-4:01"
{
	if (n>0&&n<=(CHIPNSFX_SIZE-3))
	{
		--n;
		musical_[0]=notes1[n%12];
		musical_[1]=notes2[n%12];
		musical_[2]=itoradix(n/12);
	}
	else
	{
		if (n==CHIPNSFX_SIZE-2) n='=';
		//else if (n==CHIPNSFX_SIZE-1) n='?';
		else if (n==CHIPNSFX_SIZE-0) n='^';
		else n='.';
		musical_[0]=musical_[1]=musical_[2]=n;
		i=0; // special codes lack instruments
	}
	if (i)
		musical_[4]=itoradix(i/16),musical_[5]=itoradix(i%16);
	else
		musical_[4]=musical_[5]='.';
	return musical_;
}
INLINE int addnote(int n,int d) // transpose `n` by `d` and normalise
{
	if (n<1)
		return 0; // minimum!
	if (n>(CHIPNSFX_SIZE-0))
		return (CHIPNSFX_SIZE-0); // maximum!
	if (n>(CHIPNSFX_SIZE-4))
		return n; // SFX, "===", ???", "^^^"
	if ((n+=d)<1)
		return 1; // C-0...
	if (n>(CHIPNSFX_SIZE-4))
		return (CHIPNSFX_SIZE-4); // ...B-8
	return n;
}

void newsong(void) // destroys all song data and starts anew
{
	hertz=50; divisor=6; transpose=0; loopto=-1;
	strcpy(title1,"Song title"); strcpy(title2,"Description");
	MEMZERO(order); MEMZERO(score); MEMZERO(scrln); MEMZERO(instr);
	order[1][0][0]=scrln[0]=scrln[1]=scrln[2]=orders=1;
	order[2][0][0]=instrs=2;
	instr[1][0]=instr[1][1]=0xFF; strcpy(&instr[1][8],"Piano");
}

char flag_nn=0; // song transposition flag

unsigned char *loadgets(unsigned char *t) { return (!(t&&fgets(t,MAXSTRLEN,f)))?NULL:strcln(t); }
int fclosein(void) { if (f!=stdin) fclose(f); return 0; }
int loadsong(char *s) // destroys the old song and loads a new one; 0 is ERROR
{
	char *r; int i,j;
	if (!strcmp("-",s))
		f=stdin;
	else if (!*s||!(f=fopen(s,"r")))
		return 0;
	loadgets(tmp);
	if (strcmp((239==tmp[0]&&187==tmp[1]&&191==tmp[2])?&tmp[3]:tmp,chipnsfxid)) // skip unsigned UTF-8 BOM
		return fclosein();
	// header: text+vars
	newsong();
	loadgets(title1);
	loadgets(title2);
	loadgets(tmp); // skip the separator gap
	// "hertz/divisor +trasposition"
	if (!loadgets(tmp)) return fclosein();
	hertz=strtol(tmp,&r,10);
	divisor=strtol(++r,&r,10);
	transpose=strtol(r,NULL,10)+flag_nn;
	//if (!flag_q) printf("%s --- %s\n%d/%d Hz %+d Trans\nOrders:",title1,title2,hertz,divisor,transpose);
	// orders: "#00+00 #01+00 #02+12"
	for (i=0;;)
	{
		if (!loadgets(r=tmp)) return fclosein();
		if (*r!='#') break; // end of order list
		for (j=0;j<3;++j)
		{
			order[j][i][0]=strtol(++r,&r,16); // skip '#'
			order[j][i][1]=strtol(r,&r,10);
			++r; // skip space
		}
		if (*r)//=='*'
			loopto=i;
		//if (!flag_q) printf(" %02X",i);
		++i;
	}
	if ((orders=i)>255) return fclosein(); // overrun!
	// scores: "#00: A-4:01 ...:..", etc.
	//if (!flag_q) printf("\nScores:");
	for (;;)
	{
		if (!loadgets(r=tmp)) return fclosein();
		if (*r!='#') break; // end of score list
		i=strtol(++r,&r,16)&255; // check overrun?
		for (j=0;j<128;++j)
		{
			int n; while ((n=*r)&&n<33) ++r;
			if (!n) break; // end of this score
			if ((n=strint(notes1,n))>=0)
			{
				#if 1
				++r;
				#else
				if (*++r=='+')
				{
					++r;
					n=score[i][j-1][0]+12*(radixtoi(ucase(*r++)-4));
				}
				else
				#endif
				{
					if (*r++=='#')
						++n;
					n+=12*radixtoi(ucase(*r++))+1;
				}
				if (n<1||n>(CHIPNSFX_SIZE-3))
					n=CHIPNSFX_SIZE-3; // old C-B -> new C-9
				score[i][j][1]=strtol(++r,0,16);
			}
			else
			{
				n=*r=='^'?CHIPNSFX_SIZE-0:*r=='='?CHIPNSFX_SIZE-2:0;
				score[i][j][1]=0;
			}
			score[i][j][0]=n;
			while (*r>32) ++r;
		}
		scrln[i]=j;
		//if (!flag_q) printf(" %02X",i);
	}
	// instruments: ":02 FFFE 0000 141 Ding"
	//if (!flag_q) printf("\nInstrs:");
	for (instrs=0;;)
	{
		if (!loadgets(r=tmp)) return fclosein();
		if (*r!=':') break; // end of instrument list
		if ((i=strtol(++r,&r,16))>0&&i<255)
		{
			j=strtol(++r,&r,16);
			instr[i][0]=j>>8; instr[i][1]=j;
			j=strtol(++r,&r,16);
			instr[i][2]=j>>8; instr[i][3]=j;
			j=strtol(++r,&r,16);
			instr[i][4]=j>>8; instr[i][5]=j;
			if (*r)
				++r;
			strcpy(&instr[i][8],r);
			if (instrs<i)
				instrs=i;
			//if (!flag_q) printf(" %02X",i);
		}
	}
	if (++instrs>255) return fclosein(); // overrun!
	#if 0 // let's tolerate this; it can happen in unfinished songs
	for (i=0;i<256;++i) // check for unknown instruments in song
		for (j=0;j<128;++j)
			if (score[i][j][1]>instrs)
				score[i][j][1]=score[i][j][0]=0; // unknown? zero!
	#endif
	//if (!flag_q) printf(crlf);
	if ((char*)filepath!=s) strcpy(filepath,s);
	return fclosein(),1;
}

// target output ---------------------------------------------- //

// the song undergoes several levels of optimisation to store
// as little data as possible: looking for loops and repetitions,
// storing just the instrument params that change between notes, etc

int flag_c=80; // maximum and current line widths

int play_pose,past_pose,play_size,past_size,play_flag[6],past_flag[6]; // <0 unused, 0..255 single, 256 multiple

char flag_a=0,flag_b=0,flag_bb[3]={0,1,2},flag_ll=0,flag_r=0,flag_t=0;

// song playback, see EXPORT and TRACKER ---------------------- //

#define SND_QUANTUM 441
#define SND_QUALITY (SND_QUANTUM*100) // 100 frames/second
#define SND_FIXED 8 // extra bits in comparison to the YM3B standard, 2 MHz
const int snd_freqs[]={(0x3BB907),(0x385EEB),(0x3534F9),(0x32387D),(0x2F66E9),(0x2CBDD4),
	(0x2A3AF9),(0x27DC33),(0x259F7C),(0x2382E9),(0x2184AD),(0x1FA314)}; // C-0..B#0 @ 4<<SND_FIXED MHz
const unsigned char snd_ampls[16]={0,1,1,1,2,3,4,5,8,11,15,21,30,43,60,85}; // = (255/3)*(2^((n-15)/2))

int snd_notefreq(int i) // calculate wavelength (not really frequency) of note `i`
{
	return (i<0||i>(CHIPNSFX_SIZE-4))?0:(((snd_freqs[i%12])>>(i/12))+1)/2;
}

int snd_playing=0,snd_looping=0,snd_order=0,snd_score=0,snd_testn,snd_testd,snd_testi=0,
	snd_ticks=0,snd_count=0,snd_noise=0,snd_noises,snd_noiser=1,snd_noisez=0,snd_noised=0;
int snd_bool[3]={1,1,1},snd_note[3],snd_freq[3],snd_freq0[3],snd_ampl[3],snd_ampl0[3],snd_ampld[3],
	snd_brake[3],snd_noisy[3],snd_nois0[3],snd_noisd[3],snd_sfx[3],snd_sfx0[3],snd_sfxd[3],snd_freqz[3];

void snd_stop(void)
{
	snd_playing=0;
	snd_ampl[0]=snd_ampl[1]=snd_ampl[2]=0;
}
void snd_play(int m) // 0=normal,!0=pattern
{
	if (orders&&scrln[order[0][0][0]]&&scrln[order[1][0][0]]&&scrln[order[2][0][0]])
	{
		int c;
		for (snd_noisez=snd_noised=c=0;c<3;++c)
		{
			snd_freq[c]=snd_ampl[c]=snd_noisy[c]=snd_sfx[c]=
				snd_freq0[c]=snd_freqz[c]=snd_ampld[c]=
				snd_brake[c]=snd_nois0[c]=snd_noisd[c]=
				snd_sfx0[c]=snd_sfxd[c]=0;
			snd_note[c]=snd_ampl0[c]=255;
		}
		snd_count=snd_ticks=snd_noise=0;
		snd_looping=m;
		snd_playing=1;
	}
}

int flag_n=0,flag_zz=0;
int snd_playnote(int c,int n,int d,int i) // channel, note, transposition, instrument; 1 if it extends the current note, 0 if new
{
	if (n>0)
	{
		if (n==(CHIPNSFX_SIZE-2))
			return snd_brake[c]=!snd_brake[c],1; // brake
		else if (n<(CHIPNSFX_SIZE-0))
		{
			if (n>(CHIPNSFX_SIZE-4))
			{
				snd_note[c]=(CHIPNSFX_SIZE-4);
				n=0; // sfx
			}
			else
			{
				snd_note[c]=(n+=transpose+d-1);
				n=snd_notefreq(n); // note
			}
			snd_sfx[c]=0;
			if (i) // instrument
			{
				snd_freq0[c]=n;
				snd_ampl[c]=snd_ampl0[c]=instr[i][0];
				snd_ampld[c]=instr[i][1];
				snd_brake[c]=0;
				if (n=snd_noisy[c]=snd_nois0[c]=instr[i][2])
				{
					snd_noisez=n;
					if ((n=snd_noisd[c]=instr[i][3])!=0x80)
						snd_noised=n;
				}
				snd_sfx0[c]=instr[i][4];
				snd_sfxd[c]=instr[i][5];
			}
			else // portamento
				return snd_sfxd[c]=n,snd_sfx0[c]=-1;
		}
		else
			snd_ampl[c]=0; // rest
		return 0;
	}
	return 1;
}
void snd_playchan(int c) // update channel `c`
{
	int i,j;
	if (snd_ampl[c]) // ignore mute channels
	{
		if (!snd_freq0[c])
			snd_freq[c]=0; // allow pure noise
		if (!snd_brake[c])
		{
			i=(unsigned char)snd_ampld[c];
			if (i>=0x60&&i<0xA0)
			{
				if (j=(signed char)(i<<3)) // zero-depth: 0x60 static and 0x80 dynamic
				{
					if (i&0x80) // timbre or tremolo
						if (snd_ampl[c]!=snd_ampl0[c])
							j=0;
					snd_ampl[c]=snd_ampl0[c]+j;
					keepwithin(&snd_ampl[c],0,255);
				}
			}
			else
			{
				// basic amplitude envelope
				if ((snd_ampl[c]+=(signed char)i)<0)
					snd_ampl[c]=0;
				else if (snd_ampl[c]>255)
					snd_ampl[c]=255;
			}
		}
		if (snd_noisd[c]==0x80) // special case: instant noise
			snd_noisy[c]=0;
		if (snd_freq0[c]&&snd_sfxd[c])
		{
			if (!snd_sfx0[c]) // arpeggio
			{
				if ((snd_sfx[c]+=0x40)>((i=(snd_sfxd[c]&0x0F))?0x80:0x40))
					snd_sfx[c]=0;
				if (snd_sfx[c]&0x80)
					;
				else if (snd_sfx[c]&0x40)
					i=snd_sfxd[c]>>4;
				else
					i=0;
				// special cases
				if (i==13)
					i=24; // D=24
				else if (i>13)
				{
					i=(i&1)?-12:-24; // F=-12, E=-24
					snd_sfx[c]=0x40; // lock arpeggio
				}
				snd_freq0[c]=snd_notefreq(snd_note[c]+i);
			}
			else if (snd_sfx0[c]>0) // vibrato et al.
			{
				j=snd_freq0[c]>>(8+1+SND_FIXED); // proportional v.
				if (!(i=(snd_sfxd[c]&7))) // slow glissando?
					i=1;
				i=(j+1)<<(i-1);
				if (snd_sfxd[c]&8)
					i=-i; // signed vibrato
				if (j=(snd_sfxd[c]&0xF0))
				{
					if (((snd_sfx[c]+=16)&0xF0)==j)
					{
						if (j=(snd_sfxd[c]&0x07)) // vibrato?
						{
							if (2&(snd_sfx[c]=((snd_sfx[c]+1)&3))) // +I -I -I +I ...
							//if (!(snd_sfx[c]=((snd_sfx[c]+1)&1))) // ultra soft vibrato
								i=-i;
						}
						else
							snd_sfx[c]=0; // slow glissando
						snd_freq0[c]-=i<<(1+SND_FIXED);
					}
				}
				else if (snd_sfxd[c]) // glissando
				{
					snd_freq0[c]-=i<<(1+SND_FIXED);
					if (snd_sfxd[c]==8) // detect flanging?
						snd_sfxd[c]=0; // handle flanging
				}
				if (snd_freq0[c]&(1<<(8+6+SND_FIXED)))
					snd_freq0[c]=0; // safety!
			}
			else // portamento
				snd_freq0[c]=(snd_sfxd[c]+snd_freq0[c]+(snd_sfxd[c]>snd_freq0[c]))/2;
		}
	}
}

// read one frame of score
int snd_frame(int h) // returns 0 if SONG OVER
{
	int c,i,q=3;
	if (snd_count<=0)
	{
		if (snd_noisez)
		{
			if ((i=snd_noised)==0x7F) // noisy crunch
				snd_noisez=256-snd_noisez;
			else
			{
				snd_noisez+=(signed char)i;
				keepwithin(&snd_noisez,1,255);
			}
		}
		if (snd_ticks<=0)
		{
			if (snd_order>=orders)
				if (snd_playing&&(loopto<0||snd_looping||!scrln[order[0][snd_order=loopto][0]]||(h&&!--flag_n)))
					snd_stop();
			if (snd_playing)
			{
				for (q=c=0;c<3;++c) // walk thru the scores
				{
					i=order[c][snd_order][0];
					if (snd_playnote(c,score[i][snd_score][0],(signed char)order[c][snd_order][1],score[i][snd_score][1]))
						snd_playchan(c);
				}
				if ((++snd_score)>=scrln[i])
				{
					snd_score=0;
					if (!snd_looping)
						++snd_order;
				}
				snd_ticks+=divisor;
			}
		}
		for (c=0;c<q;++c)
			snd_playchan(c);
		--snd_ticks;
		snd_count+=100;
		for (c=0;c<3;++c)
			snd_freqz[c]=(snd_freq0[c]*(2+(flag_zz&1)))/2; // wave shape
	}
	snd_count-=hertz;
	return snd_playing;
}

// export output ---------------------------------------------- //

int fputiiii(int x) { fputc(x,f); fputc(x>>8,f); fputc(x>>16,f); return fputc(x>>24,f); }


int generate_ym3(uint8_t **out_data,int i) // exports the current song as an YM3B file; 0 is ERROR
{
	int l=0,o=0;
	// measure song in advance
	if (i<0)
		i=0; // beware of non-looping songs!
	snd_order=i;
	while (i<orders)
	{
		if (i==loopto)
			o=l;
		l+=scrln[order[0][i++][0]];
	}
	l*=divisor;
	o*=divisor;
	if (hertz<50)
	{
		l<<=1;
		o<<=1;
	}
	else if (hertz>50)
	{
		l>>=1;
		o>>=1;
	}
	char *y=(char*)malloc(l*14);
	if (!y)
		return 0;
    int out_data_size=4+l*14; //YM3!
    if (o) out_data_size+=4; //YM3B
    *out_data=(uint8_t*)malloc(out_data_size);
    uint8_t *out_data_ptr=*out_data;
	if (!(*out_data))
	{
		free(y);
		return 0;
	}
	//fwrite(o?"YM3b":"YM3!",1,4,f);
    memcpy(out_data_ptr,o?"YM3b":"YM3!",4);
    out_data_ptr+=4;
    
	//loopto=-1; // forbid looping
	snd_play(i=0);
	flag_q_init(l);
	while (i<l)
	{
		flag_q_loop(i);
		snd_frame(!0); snd_frame(!0); // reduce 100 Hz to 50 Hz
		int j;
		//y[i     ]=j=((int)(snd_freq0[0]*1.7734/2)>>(SND_FIXED)); // YM3B @ 2 MHz
        y[i     ]=j=((int)(snd_freq0[0])>>(SND_FIXED)); // YM3B @ 2 MHz
		y[i+l   ]=j>>8;
		//y[i+l* 2]=j=((int)(snd_freq0[1]*1.7734/2)>>(SND_FIXED));
        y[i+l* 2]=j=((int)(snd_freq0[1])>>(SND_FIXED));
		y[i+l* 3]=j>>8;
		//y[i+l* 4]=j=((int)(snd_freq0[2]*1.7734/2)>>(SND_FIXED));
        y[i+l* 4]=j=((int)(snd_freq0[2])>>(SND_FIXED));
		y[i+l* 5]=j>>8;
		y[i+l* 6]=snd_noisez>>3; // 0..31
		y[i+l* 7]=(snd_noisy[0]?0:8)|(snd_noisy[1]?0:16)|(snd_noisy[2]?0:32)|((snd_freq0[0]&&snd_ampl[0])?0:1)|((snd_freq0[1]&&snd_ampl[1])?0:2)|((snd_freq0[2]&&snd_ampl[2])?0:4);
		y[i+l* 8]=snd_ampl[0]>>4; // 0..15
		y[i+l* 9]=snd_ampl[1]>>4;
		y[i+l*10]=snd_ampl[2]>>4;
		y[i+l*11]=y[i+l*12]=0;
		y[i+l*13]=-1; // IDLE
		++i;
	}
	//fwrite(y,1,l*=14,f);
    memcpy(out_data_ptr,y,l*=14);
    out_data_ptr+=l;
    
    if (o) {
        memcpy(out_data_ptr,&o,4);
        out_data_ptr+=4;
        l+=4;
        //l+=4,fputiiii(o);
    }
	free(y);
	//fclose(f);
	flag_q_exit(l+4);
	return out_data_size;
}

// MAIN() ----------------------------------------------------- //

const char txt_load_error[]="cannot read song",txt_save_error[]="cannot save song";

int printerror1(const char *s) { fprintf(stderr,"error: %s!\n",s); return 1; }
int chp2ym(char *filename,uint8_t **ym_data)
{
	int i=*filepath=0,j,k;
	char flag_w=0,flag_y=0/*,flag_s=0*/;
    
    flag_y=1; //ym3 convert mode
    
	flag_c-=10; // margin for ",$00FX,$XX"
	
// console operations ----------------------------------------- //

	if (!loadsong(filename))
		return printerror1(txt_load_error);
	if (flag_n<1)
		flag_n=1;
    
    
    return generate_ym3(ym_data,flag_y>1?loopto:0);
// the tracker itself ----------------------------------------- //
}
