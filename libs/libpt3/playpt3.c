#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <conio.h>
#include "ayumi.h"
#include "pt3player.h"
#include "load_text.h"


static int load_file(const char* name, char** buffer, int* size) {
  FILE* f = fopen(name, "rb");
  if (f == NULL) {
    return 0;
  }
  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  rewind(f);
  *buffer = (char*) malloc(*size + 1);
  if (*buffer == NULL) {
    fclose(f);
    return 0;
  }
  if ((int) fread(*buffer, 1, *size, f) != *size) {
    free(*buffer);
    fclose(f);
    return 0;
  }
  fclose(f);
  (*buffer)[*size] = 0;
  return 1;
}

unsigned char ayreg[14];
struct ayumi ay[10];
int numofchips=0;
int volume = 10000;
int pause = 0;

struct ay_data t;
char files[10][255];
int numfiles=0;

//---------------------------------------------------------------
	//Simple Windows sound streaming code..
static HWAVEOUT waveOutHand;
static WAVEHDR waveHdr;
#define LBUFSIZE 14
static short sndbuf[4<<LBUFSIZE];
//static float sndbuf[4<<LBUFSIZE];
short *sptr;
int lastsamp=0;
int isr_step=1;
int lastleng=0;
short *tmpbuf[10];
int frame[10];
int sample[10];
int fast=0;
int sample_count;
int mute[10];

char* music_buf;
int music_size;

int loadfiles();


static void snd_init(int samprate) {
	DWORD threadId;
	WAVEFORMATEX wfx;

	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = ((wfx.nChannels*wfx.wBitsPerSample)>>3);
	wfx.nSamplesPerSec = samprate;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec*wfx.nBlockAlign;
	waveOutOpen(&waveOutHand,WAVE_MAPPER,&wfx,0,0,0);

	waveHdr.dwFlags = WHDR_BEGINLOOP|WHDR_ENDLOOP;
	waveHdr.lpData = (LPSTR)sndbuf;
	waveHdr.dwBufferLength = sizeof(sndbuf)/sizeof(sndbuf[0]);
	waveHdr.dwLoops = -1;
	waveOutPrepareHeader(waveOutHand,&waveHdr,sizeof(WAVEHDR));
	waveOutWrite(waveOutHand,&waveHdr,sizeof(WAVEHDR));
}

static void snd_end() {
	waveOutReset(waveOutHand);
	waveOutClose(waveOutHand);
	memset(sndbuf,0,sizeof(sndbuf));
}

//static int snd_ready (float **sptr)
static int snd_ready(short **sptr) {
	static int curblk = 0;
	MMTIME mmt;

	mmt.wType = TIME_BYTES;
	waveOutGetPosition(waveOutHand,&mmt,sizeof(MMTIME));
	if (((mmt.u.cb>>(LBUFSIZE+1))&1) == curblk) return(0);
	*sptr = &sndbuf[curblk<<LBUFSIZE];
	curblk ^= 1;
	return(2<<LBUFSIZE);
}

static int get_snd_pos() {
	MMTIME mmt;
	mmt.wType = TIME_SAMPLES;
	waveOutGetPosition(waveOutHand,&mmt,sizeof(MMTIME));
	return mmt.u.sample;
}
//---------------------------------------------------------------


void update_ayumi_state(struct ayumi* ay, uint8_t* r, int ch) {
	func_getregs(r, ch);
	ayumi_set_tone(ay, 0, (r[1] << 8) | r[0]);
	ayumi_set_tone(ay, 1, (r[3] << 8) | r[2]);
	ayumi_set_tone(ay, 2, (r[5] << 8) | r[4]);
	ayumi_set_noise(ay, r[6]);
	ayumi_set_mixer(ay, 0, r[7] & 1, (r[7] >> 3) & 1, r[8] >> 4);
	ayumi_set_mixer(ay, 1, (r[7] >> 1) & 1, (r[7] >> 4) & 1, r[9] >> 4);
	ayumi_set_mixer(ay, 2, (r[7] >> 2) & 1, (r[7] >> 5) & 1, r[10] >> 4);
	ayumi_set_volume(ay, 0, r[8] & 0xf);
	ayumi_set_volume(ay, 1, r[9] & 0xf);
	ayumi_set_volume(ay, 2, r[10] & 0xf);
	ayumi_set_envelope(ay, (r[12] << 8) | r[11]);
	if (r[13] != 255) {
		ayumi_set_envelope_shape(ay, r[13]);
	}

	for (int i=0; i<9; i++) {
		if (i % 3 == 0 && mute[i] && ch == i / 3) {
			ayumi_set_volume(ay, 0, 0);
			ayumi_set_mixer(ay, 0, r[7] & 1, (r[7] >> 3) & 1, 0);
		}
		if (i % 3 == 1 && mute[i] && ch == i / 3) {
			ayumi_set_volume(ay, 1, 0);
			ayumi_set_mixer(ay, 1, (r[7] >> 1) & 1, (r[7] >> 4) & 1, 0);
		}
		if (i % 3 == 2 && mute[i] && ch == i / 3) {
			ayumi_set_volume(ay, 2, 0);
			ayumi_set_mixer(ay, 2, (r[7] >> 2) & 1, (r[7] >> 5) & 1, 0);
		}
	}
}

static int renday(void *snd, int leng, struct ayumi* ay, struct ay_data* t, int ch)
{
	isr_step = t->sample_rate / t->frame_rate;
	int isr_counter = 0;
	int16_t *buf = snd;
	int i = 0;
	if (fast) isr_step /= 4;
	lastleng=leng;
	while (leng>0)
	{
		if (!pause) {
			if (sample[ch] >= isr_step) {
				func_play_tick(ch);
				update_ayumi_state(ay,ayreg,ch);
				sample[ch] = 0;
				frame[ch]++;
			}
			ayumi_process(ay);
			if (t->dc_filter_on) {
				ayumi_remove_dc(ay);
			}
			buf[i] = (short) (ay->left * volume);
			buf[i+1] = (short) (ay->right * volume);
		}
		else {
			buf[i] = (short) (0);
			buf[i+1] = (short) (0);
		}

		sample[ch]++;
		leng-=4;
		i+=2;
	}
	return 1;	
}


void rewindto(struct ay_data* t, int skip) {
	snd_end();
	if (skip<0) skip=0;
//	printf("\nrewind to %i", skip);

	for (int ch=0; ch<numofchips; ch++) {
		func_restart_music(ch);
		frame[ch] = 0;
		sample[ch] = 0;
	}
	for (int i=0;i<skip;i++) {
		for (int ch=0; ch<numofchips; ch++) {
			func_play_tick(ch);
			frame[ch] += 1;
//			for (int in=0; in<isr_step; in++)
//				ayumi_skip(&ay[ch]);
		}
	}
	snd_init(t->sample_rate);
//	printf("\n->%i\n", frame[0]);
}


void ayumi_play(struct ayumi ay[6], struct ay_data* t) {
	snd_init(t->sample_rate);
	int i, j, skip, curpos;
	while (1) {
		if (i = snd_ready(&sptr)) {
			for (int ch = 0; ch<numofchips; ch++) {
				renday(tmpbuf[ch], i, &ay[ch], t, ch);
			}

			for (int j=0; j<i/2; j++) { //stereo = 16bit x 2
				int tv=0;
				for (int ch = 0; ch < numofchips; ch++) { //collecting
					tv += *(int16_t*)(tmpbuf[ch]+j);
				}
				sptr[j] = tv/numofchips;
			}

			skip = frame[0];
			lastsamp = get_snd_pos();
//			printf("\rpos=%i",frame[0]);

		}

		Sleep(10);

		if (isr_step>1) {
			int skip1 = get_snd_pos();
//			if (lastsamp > skip1) lastsamp = skip;
			int skip3 = (skip1 - lastsamp) / isr_step; //number of frames
			curpos = skip + skip3;
			if (curpos>=0) printf("\rpos=%i   ",curpos);
		}
		else curpos = skip;


		if (kbhit())
		{
			i = getch();
			if (i == 27)
				break;
			if (i == 'f')
				fast = !fast;
			if (i == ' ')
				pause = !pause;
			if (i == 'r') {
				printf("\n", curpos);
				skip = frame[0];
				loadfiles(0);
				curpos-=25;
				if (curpos<0) curpos=0;
				rewindto(t, curpos);
				skip = frame[0];
			}


			if (i >= '1' && i <= '9') {
				mute[i-'1']=!mute[i-'1'];
				printf("    (1=%s 2=%s 3=%s) (4=%s 5=%s 6=%s) (7=%s 8=%s 9=%s) \r",
				!mute[0]?"on ":"off",
				!mute[1]?"on ":"off",
				!mute[2]?"on ":"off",
				!mute[3]?"on ":"off",
				!mute[4]?"on ":"off",
				!mute[5]?"on ":"off",
				!mute[6]?"on ":"off",
				!mute[7]?"on ":"off",
				!mute[8]?"on ":"off");
			}

				
			if (i == 224) {
				i = getch();
				if (i==73) { //pgup
					volume *=1.1;
					if (volume >15000) volume = 15000;
					printf("   vol=%.3i%%  \r", volume / 150);
				}
				if (i==81) { //pgdown
					volume /=1.1;
					if (volume <10) volume = 10;
					printf("   vol=%.3i%%  \r", volume / 150);
				}
				if (i==71) { //home
					rewindto(t,0);
					skip = frame[0];
//					printf("restart\n");
				}
				if (i==75) { //left
					int sk=curpos-100;
					if (sk<0) sk=0;
					rewindto(t,sk);
					skip = frame[0];
				}
				if (i==77) { //right
					int sk=curpos+100;
					rewindto(t,sk);
					skip = frame[0];
				}

			}
		}

	}
	printf(".\n");
	snd_end();

}


void set_default_data(struct ay_data* t) {
	memset(t, 0, sizeof(struct ay_data));
	t->sample_rate = 44100;
	t->eqp_stereo_on = 1;
	t->dc_filter_on = 1;
	t->is_ym = 1;
	t->pan[0]=0.1;
	t->pan[1]=0.5;
	t->pan[2]=0.9;
	t->clock_rate = 1750000;
	t->frame_rate = 50;
	t->note_table = -1;
}

int loadfiles(int first)
{
	load_text_file("playpt3.txt", &t);
	forced_notetable=t.note_table;

	numofchips=0;
	for (int fn=0; fn<numfiles;fn++) {
		if(!load_file(files[fn], &music_buf, &music_size)) {
			printf("Load error\n");
			return 1;
		}
		printf("*** Loaded \"%s\" %i bytes\n",files[fn],music_size);

		int num = func_setup_music(music_buf, music_size, numofchips, first);

		numofchips+=num;
		if (first) printf("Number of chips: %i\n",num);
	}


	if (first) printf("Total number of chips: %i\n",numofchips);

	for (int ch=0; ch<numofchips; ch++) {
		if (!ayumi_configure(&ay[ch], t.is_ym, t.clock_rate, t.sample_rate)) {
			printf("ayumi_configure error (wrong sample rate?)\n");
			return 1;
		}
		ayumi_set_pan(&ay[ch], 0, t.pan[0], t.eqp_stereo_on);
		ayumi_set_pan(&ay[ch], 1, t.pan[1], t.eqp_stereo_on);
		ayumi_set_pan(&ay[ch], 2, t.pan[2], t.eqp_stereo_on);
		if (tmpbuf[ch]==0) tmpbuf[ch] = malloc(sizeof(sndbuf));
		else tmpbuf[ch] = realloc(tmpbuf[ch],sizeof(sndbuf));
		frame[ch]=0;
		sample[ch]=0;
		if (first) printf("Ayumi #%i configured\n",ch);
	}
	pause = 0;
}

int main(int argc, char** argv) {
	if (argc < 2) {
		printf("playpt3.exe <file.pt3> ...\n");
		return 1;
	}

	printf(
"esc - exit\n"
"pgup/pgdown - change volume\n"
"f - switch fast forward (x4)\n"
"space - pause/unpause\n"
"r - reload song files, and keep playing / unpause\n"
"home - rewind to the beginning\n"
"left/right - rewind 2s backward/forward\n"
"1,2,3, 4,5,6, 7,8,9 - switch off/on particular channel/track\n\n");


	set_default_data(&t);
	for (int ch=0; ch<10; ch++) {
		tmpbuf[ch]=0;
	}

	for (int fn=1; fn<argc; fn++) {
		strcpy(files[fn-1],argv[fn]);
		numfiles++;
	}

//	printf("numfiles=%i\n",numfiles);
//	printf("files[0]=%s\n",files[0]);
	for (int i=0;i<10;i++)
		mute[i]=0;

	if (loadfiles(1)) {
	//	printf("Skipping...\n");
		//  for (int i=0;i<5376;i++)  //5376
		//	  func_play_tick(0);
		printf("Playing...\n");

		ayumi_play(ay, &t);
		printf("Finished\n");
		return 0;
	}
	else {
		printf("Failed.\n");
		return 1;
	}
}