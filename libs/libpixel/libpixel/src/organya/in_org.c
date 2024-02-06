// ORG input plug-in for WinAmp

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "winamp/in2.h"

#include "Wave100.inc"

// Org-02 drums
#include "Bass01.inc"
#include "Bass02.inc"
#include "Snare01.inc"
#include "Snare02.inc"
#include "Tom01.inc"
#include "HiClose.inc"
#include "HiOpen.inc"
#include "Crash.inc"
#include "Per01.inc"
#include "Per02.inc"
#include "Bass03.inc"
#include "Tom02.inc"

// Org-01 (beta) drums
#include "Bass01Beta.inc"
#include "CrashBeta.inc"
#include "HiCloseBeta.inc"
#include "HiOpenBeta.inc"
#include "Snare01Beta.inc"

// OrgMaker 2 drums
#include "Bass04.inc"
#include "Bass05.inc"
#include "Snare03.inc"
#include "Snare04.inc"
#include "HiClos2.inc"
#include "HiOpen2.inc"
#include "HiClos3.inc"
#include "HiOpen3.inc"
#include "Crash02.inc"
#include "RevSym.inc"
#include "Ride01.inc"
#include "Tom03.inc"
#include "Tom04.inc"
#include "Orcdrm.inc"
#include "Bell.inc"
#include "Cat.inc"

#include "in_org.h"

#define VERSION "v1.08 config"

extern In_Module mod;
char lastfn[MAX_PATH];
int paused;
int seek_needed; // if != -1, it is the point that the decode thread should seek to, in ms.
int decode_pos_ms; // current decoding position, in milliseconds
int play_pos_ms;
char ini_path[250]={0};

// post this to the main window at end of file (after playback as stopped)
#define WM_WA_MPEG_EOF WM_USER+2

// Currently only 1 or 2 channel is supported.
// Can do any BPS and SAMPLERATE.
#define NCH (2)
#define BPS (16)			// use same as for PXtone
#define SAMPLERATE (44100)
char sample_buffer[576*NCH*(BPS/8)*2]; // sample buffer

BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved);
DWORD WINAPI __stdcall PlayThread(void *b);
__declspec(dllexport) In_Module *winampGetInModule2(void);

HANDLE thread_handle=INVALID_HANDLE_VALUE;
int killThread=0;

BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	(void)hInst;
	(void)ul_reason_for_call;
	(void)lpReserved;
	return TRUE;
}

#define CHANNELS 16
typedef struct
{
	unsigned int position;
	unsigned char key,len,vol,pan;
} orgnote_t;

typedef struct
{
	unsigned short pitch;
	unsigned char inst,pi;
	unsigned short num_notes;
	double pos;
	const unsigned char *block;
	unsigned int length;
	unsigned short freq;
	short wave;
	orgnote_t *notes;
	struct
	{
		unsigned int position;
		unsigned char key,len,pan,dimmed;
		unsigned short vol;
	} playing;
} orgchan_t;

typedef struct
{
	unsigned char loaded;
	char version_string[7];
	unsigned char version;
	unsigned short tempo;
	unsigned char steps,beats;
	unsigned int loop_start,loop_end,
		step,tick,num_channels,num_instruments,nonlooping;
	orgchan_t chan[CHANNELS];
} org_t;
org_t org;

static void clear_sound(void)
{
	int i;
	for(i = 0; i < CHANNELS; i++)
	{
		org.chan[i].pos = -2.0;
		org.chan[i].playing.position = 0;
		org.chan[i].playing.len = 0;
	}
}
int maxstep=3; //julian edit
int sstep=0; //julian edit
static int set_step(unsigned int new_step)
{
	int i, j;
	org.step = new_step;
	if(org.step >= org.loop_start && org.nonlooping)
		return 1;
	else if(org.step == org.loop_end)
	{
		sstep++; //julian edit
		if(sstep<maxstep||maxstep==0){ //julian edit
    		clear_sound();  
    		decode_pos_ms = org.loop_start*org.tempo; //julian edit
	       	return set_step(org.loop_start); //julian edit
		}else{ //julian edit
            sstep=0;
            org.nonlooping=1; 
        } //julian edit
	}
	for(i = 0; i < CHANNELS; i++)
	{
		if(org.chan[i].playing.len > 0)
			org.chan[i].playing.len--;

		j = 0;
		if(org.chan[i].playing.position)
			j = org.chan[i].playing.position;
		for(; j < org.chan[i].num_notes; j++)
			if(org.chan[i].notes[j].position == org.step)
			{
				org.chan[i].playing.position = j+1;
				if(org.chan[i].notes[j].key != 0xFF)
				{
					org.chan[i].playing.key = org.chan[i].notes[j].key;
					if(org.chan[i].pi)
						org.chan[i].playing.len = 1;
					else
						org.chan[i].playing.len = org.chan[i].notes[j].len;
					org.chan[i].pos = -1.0;
					org.chan[i].playing.dimmed = FALSE;
				}
				if(org.chan[i].notes[j].vol != 0xFF)
				{
					if (i < CHANNELS/2) {
						org.chan[i].playing.vol = org.chan[i].notes[j].vol+50;
						if (org.chan[i].playing.dimmed)
							org.chan[i].playing.vol = 3 * org.chan[i].playing.vol / 4;
					} else
						org.chan[i].playing.vol = org.chan[i].notes[j].vol+50;
				}
				if(org.chan[i].notes[j].pan != 0xFF)
					org.chan[i].playing.pan = org.chan[i].notes[j].pan;
			}
	}
	return 0;
}

static void unload_org(void)
{
	int i;
	if(!org.loaded)
		return;
	for(i = 0; i < CHANNELS; i++)
		free(org.chan[i].notes);
	org.loaded = 0;
	org.step = 0;
	org.tick = 0;
}

static int load_org(const char *fn)
{
	FILE *fp;
	int i, j;
	if(!strcmp(fn,lastfn) && org.loaded)
	{
		clear_sound();
		set_step(0);
		return 0;
	}
	unload_org();
	fp = fopen(fn,"rb");
	if(!fp)
		return 1;

	fread(org.version_string, 1, 6, fp);
	org.version_string[6] = '\0';
	if(strncmp(org.version_string,"Org-0",5) || org.version_string[5] < '1' || org.version_string[5] > '3')
	{
		char msg[256];
		fclose(fp);
		sprintf(msg,"Invalid Org header '%s' (Only 'Org-01' through 'Org-03' supported)",org.version_string);
		MessageBox(mod.hMainWindow,msg,"ORG Player Error",MB_OK);
		return 1;
	}
	org.version = atoi(&org.version_string[4]);
	fread(&org.tempo, 2, 1, fp);
	org.steps = getc(fp); // steps per beat
	org.beats = getc(fp); // beats per bar
	fread(&org.loop_start, 4, 1, fp);
	fread(&org.loop_end, 4, 1, fp);

	org.num_channels = CHANNELS;
	org.num_instruments = CHANNELS/2;
	for(i = 0; i < CHANNELS; i++)
	{
		memset(&org.chan[i],0,sizeof(org.chan[i]));
		memset(&org.chan[i].playing,0,sizeof(org.chan[i].playing));
		fread(&org.chan[i].pitch, 2, 1, fp);
		fread(&org.chan[i].inst, 1, 1, fp);
		if(i < CHANNELS/2)
		{
			org.chan[i].block = &Wave100[(org.chan[i].inst)*WAVELENGTH];
			org.chan[i].length = WAVELENGTH+1;
			org.chan[i].freq = 44100+org.chan[i].pitch-1000;
		}
		else
		{
			if (org.version == 1)
			{
			org.chan[i].freq = 22050;
			switch(org.chan[i].inst)
			{
				case 0:
					org.chan[i].block = Bass01Beta;
					org.chan[i].length = BASS01BETALENGTH;
					break;
				case 1:
					org.chan[i].block = Bass02;
					org.chan[i].length = BASS02LENGTH;
					break;
				case 2:
					org.chan[i].block = Snare01Beta;
					org.chan[i].length = SNARE01BETALENGTH;
					break;
				case 3:
					org.chan[i].block = Snare02;
					org.chan[i].length = SNARE02LENGTH;
					break;
				case 4:
					org.chan[i].block = Tom01;
					org.chan[i].length = TOM01LENGTH;
					break;
				case 5:
					org.chan[i].block = HiCloseBeta;
					org.chan[i].length = HICLOSEBETALENGTH;
					break;
				case 6:
					org.chan[i].block = HiOpenBeta;
					org.chan[i].length = HIOPENBETALENGTH;
					break;
				case 7:
					org.chan[i].block = CrashBeta;
					org.chan[i].length = CRASHBETALENGTH;
					break;
				case 8:
					org.chan[i].block = Per01;
					org.chan[i].length = PER01LENGTH;
					break;
				case 9:
					org.chan[i].block = Per02;
					org.chan[i].length = PER02LENGTH;
					break;
				case 10:
					org.chan[i].block = Bass03;
					org.chan[i].length = BASS03LENGTH;
					break;
				case 11:
					org.chan[i].block = Tom02;
					org.chan[i].length = TOM02LENGTH;
					break;
				default:
					org.chan[i].block = NULL;
					org.chan[i].length = 0;
					{
						char msg[256];
						sprintf(msg,"Invalid Org instrument %d",org.chan[i].inst);
						MessageBox(mod.hMainWindow,msg,"ORG Player Error",MB_OK);
					}
					break;
			}
			}
			else if (org.version == 2)
			{
			org.chan[i].freq = 22050;
			switch(org.chan[i].inst)
			{
				case 0:
					org.chan[i].block = Bass01;
					org.chan[i].length = BASS01LENGTH;
					break;
				case 1:
					org.chan[i].block = Bass02;
					org.chan[i].length = BASS02LENGTH;
					break;
				case 2:
					org.chan[i].block = Snare01;
					org.chan[i].length = SNARE01LENGTH;
					break;
				case 3:
					org.chan[i].block = Snare02;
					org.chan[i].length = SNARE02LENGTH;
					break;
				case 4:
					org.chan[i].block = Tom01;
					org.chan[i].length = TOM01LENGTH;
					break;
				case 5:
					org.chan[i].block = HiClose;
					org.chan[i].length = HICLOSELENGTH;
					break;
				case 6:
					org.chan[i].block = HiOpen;
					org.chan[i].length = HIOPENLENGTH;
					break;
				case 7:
					org.chan[i].block = Crash;
					org.chan[i].length = CRASHLENGTH;
					break;
				case 8:
					org.chan[i].block = Per01;
					org.chan[i].length = PER01LENGTH;
					break;
				case 9:
					org.chan[i].block = Per02;
					org.chan[i].length = PER02LENGTH;
					break;
				case 10:
					org.chan[i].block = Bass03;
					org.chan[i].length = BASS03LENGTH;
					break;
				case 11:
					org.chan[i].block = Tom02;
					org.chan[i].length = TOM02LENGTH;
					break;
				// OrgMaker 2 EN
				case 12:
					org.chan[i].block = Bass04;
					org.chan[i].length = BASS04LENGTH;
					break;
				case 13:
					org.chan[i].block = Bass05;
					org.chan[i].length = BASS05LENGTH;
					break;
				case 14: // Snare03
					org.chan[i].block = Snare03;
					org.chan[i].length = SNARE03LENGTH;
					break;
				case 15: // Snare04
					org.chan[i].block = Snare04;
					org.chan[i].length = SNARE04LENGTH;
					break;
				case 16: // HiClos2
					org.chan[i].block = HiClos2;
					org.chan[i].length = HICLOS2LENGTH;
					break;
				case 17: // HiOpen2
					org.chan[i].block = HiOpen2;
					org.chan[i].length = HIOPEN2LENGTH;
					break;
				case 18: // HiClos3
					org.chan[i].block = HiClos3;
					org.chan[i].length = HICLOS3LENGTH;
					break;
				case 19: // HiOpen3
					org.chan[i].block = HiOpen3;
					org.chan[i].length = HIOPEN3LENGTH;
					break;
				case 20:
					org.chan[i].block = Crash02;
					org.chan[i].length = CRASH02LENGTH;
					break;
				case 21:
					org.chan[i].block = RevSym;
					org.chan[i].length = REVSYMLENGTH;
					break;
				case 22: // Ride01
					org.chan[i].block = Ride01;
					org.chan[i].length = RIDE01LENGTH;
					break;
				case 23: // Tom03
					org.chan[i].block = Tom03;
					org.chan[i].length = TOM03LENGTH;
					break;
				case 24: // Tom04
					org.chan[i].block = Tom04;
					org.chan[i].length = TOM04LENGTH;
					break;
				case 25:
					org.chan[i].block = Orcdrm;
					org.chan[i].length = ORCDRMLENGTH;
					break;
				case 26:
					org.chan[i].block = Bell;
					org.chan[i].length = BELLLENGTH;
					break;
				case 27:
					org.chan[i].block = Cat;
					org.chan[i].length = CATLENGTH;
					break;
				default:
					org.chan[i].block = NULL;
					org.chan[i].length = 0;
					{
						char msg[256];
						sprintf(msg,"Invalid Org instrument %d",org.chan[i].inst);
						MessageBox(mod.hMainWindow,msg,"ORG Player Error",MB_OK);
					}
					break;
			}
			}
			org.chan[i].freq += org.chan[i].pitch-1000;
		}
		fread(&org.chan[i].pi, 1, 1, fp);
		fread(&org.chan[i].num_notes, 2, 1, fp);
		if(org.chan[i].num_notes > 0)
		{
			org.chan[i].notes = malloc(sizeof(orgnote_t)*org.chan[i].num_notes);
			memset(org.chan[i].notes,0,sizeof(*org.chan[i].notes));
		}
		else
		{
			org.num_channels--;
			if(i < CHANNELS/2)
				org.num_instruments--;
		}
	}

	org.nonlooping = 1;
	for(i = 0; i < CHANNELS; i++)
	{
		if(org.chan[i].num_notes <= 0)
			continue;
		for(j = 0; j < org.chan[i].num_notes; j++)
		{
			fread(&org.chan[i].notes[j].position, 4, 1, fp);
			if(org.chan[i].notes[j].position > org.loop_start
			&& org.chan[i].notes[j].position < org.loop_end)
				org.nonlooping = 0;
		}
		for(j = 0; j < org.chan[i].num_notes; j++)
			fread(&org.chan[i].notes[j].key, 1, 1, fp);
		for(j = 0; j < org.chan[i].num_notes; j++)
			fread(&org.chan[i].notes[j].len, 1, 1, fp);
		for(j = 0; j < org.chan[i].num_notes; j++)
			fread(&org.chan[i].notes[j].vol, 1, 1, fp);
		for(j = 0; j < org.chan[i].num_notes; j++)
			fread(&org.chan[i].notes[j].pan, 1, 1, fp);
	}
	fclose(fp);

	org.loaded = 1;
	clear_sound();
	set_step(0);
	return 0;
}

//static void config(HWND hwnd)
//{
//	DialogBox(mod.hDllInstance, (const char *)sstep, hwnd, config_dialog);
//	MessageBox(hwnd,"No configuration necessary.","ORG Player Configuration",MB_OK);
//}


static BOOL CALLBACK WndCfgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){ 
    char maxintval[20]; 
      
     switch (message)
     {
     case WM_INITDIALOG:
            //Anstatt WM_CREATE heisst es hier WM_INITDIALOG            
            SetDlgItemInt(hWnd, IDC_EditIter, maxstep,FALSE);
            return TRUE; 
     case WM_CLOSE:             
            maxstep=GetDlgItemInt(hWnd, IDC_EditIter,NULL , FALSE);
            
            itoa(maxstep,maxintval,10);
            WritePrivateProfileString( L"winamp_in_org_plugin",L"maxIteration",maxintval,ini_path );
            EndDialog(hWnd,0);
            return TRUE;  
     case WM_COMMAND:
            if (LOWORD(wParam) == IDC_OK) // is it the OK button?
            {
                      
                maxstep=GetDlgItemInt(hWnd, IDC_EditIter,NULL , FALSE);
                
                itoa(maxstep,maxintval,10);
                WritePrivateProfileString( L"winamp_in_org_plugin",L"maxIteration",maxintval,ini_path );
            
               EndDialog(hWnd, LOWORD(wParam));
               return (INT_PTR)TRUE;
            }
            break;
     }
     return FALSE;
}
    
HWND config_control_hwnd;
void config(struct In_Module *this_mod){
     ShowWindow((
         config_control_hwnd=CreateDialog(
             mod.hDllInstance,
             MAKEINTRESOURCE(IDD_DLGFIRST),
             mod.hMainWindow,
             WndCfgProc)
        ),
     SW_SHOW);
}

static void about(HWND hwnd)
{
	MessageBox(hwnd,"ORG Player "VERSION", by JTE (http://www.echidnatribe.org/)\nOrganya Data format by Studio Pixel","About ORG Player",MB_OK);
}

static void init(void)
{       
    char dir[32];    
    sprintf(dir, "%s", (char*)SendMessage(mod.hMainWindow,WM_WA_IPC,0,IPC_GETINIDIRECTORY));
    lstrcpyn(ini_path, dir, MAX_PATH - 32); // 32 will be (more than) enough room for \Plugins\plugin.ini
    lstrcat(ini_path, "\\Plugins");
    CreateDirectory(ini_path, NULL); // can't guarantee that this will exist
    lstrcat(ini_path, "\\in_org.ini");
    
    
	maxstep=GetPrivateProfileInt( L"winamp_in_org_plugin",L"maxIteration",maxstep,ini_path);
    
	org.loaded = 0;
}

static void quit(void)
{
	unload_org();
}

static int getlength(void)
{
	if(!org.loaded)
		return -1000;
	if(org.nonlooping)
		return org.loop_start*org.tempo;
	return org.loop_end*org.tempo;
}

static void getfileinfo(const char *fn, char *title, int *length_in_ms)
{
	int loaded;
	if(!fn || !*fn)
		fn=lastfn;
	loaded = !strcmp(fn,lastfn);
	if(title)
	{
		const char *p=fn+strlen(fn);
		char *pp;
		while (*p != '\\' && p >= fn) p--;
		strcpy(title,++p);
		pp=title+strlen(title);
		while (*pp != '.' && pp >= title) pp--;
		if(*pp == '.') *pp = '\0';
	}
	if (length_in_ms)
	{
		*length_in_ms=-1000;
		if(loaded)
			*length_in_ms=getlength();
	}
}

static int infoDlg(const char *fn, HWND hwnd)
{
	int loaded;
	char *msg;
	if(!fn || !*fn)
		fn = lastfn;
	loaded = !strcmp(fn,lastfn);

	msg = malloc(1024);
	{
		const char *p=fn+strlen(fn);
		while (*p != '\\' && p >= fn) p--;
		strncpy(msg,++p,32);
	}
	if(loaded)
	{
		wsprintf(msg,
			"%s\n"
			"Format: %s\n"
			"Channels: %d (%d instruments, %d drums)\n"
			"Tempo: %d, Steps/Beats: %d/%d",
			msg,
			org.version_string,
			org.num_channels,org.num_instruments,org.num_channels-org.num_instruments,
			org.tempo,org.steps,org.beats);
		if(org.nonlooping)
		{
			wsprintf(msg,
				"%s\n"
				"Non-looping (Plays once)",
				msg);
		}
		else
		{
			wsprintf(msg,
				"%s\n"
				"Loop starts at %d:%02d (%d:%02d long loop)",
				msg,
				(org.loop_start*org.tempo/1000)/60,
				(org.loop_start*org.tempo/1000)%60,
				((org.loop_end-org.loop_start)*org.tempo/1000)/60,
				((org.loop_end-org.loop_start)*org.tempo/1000)%60);
		}
	}
	else
		wsprintf(msg,"%s\nNo information available.\n(Try again while playing)",msg);
	MessageBox(hwnd, msg, "ORG File Info", MB_OK);
	free(msg);
	return 0;
}

static int isourfile(const char *fn) { (void)fn; return 0; }

static int play(const char *fn) {
	DWORD tmp;
	int maxlatency;
	if(load_org(fn))
		return 1;
	strcpy(lastfn,fn);
	paused=0;
	decode_pos_ms=0;
	seek_needed=-1;
	memset(sample_buffer,0,sizeof(sample_buffer));

	maxlatency = mod.outMod->Open(SAMPLERATE,NCH,BPS,0,0);
	if (maxlatency < 0)
		return 1;
	mod.SetInfo((SAMPLERATE*BPS*NCH)/1000,SAMPLERATE/1000,NCH>1,1);
	if (mod.SAVSAInit) mod.SAVSAInit(maxlatency,SAMPLERATE);
	if (mod.VSASetInfo) mod.VSASetInfo(SAMPLERATE,NCH);
	//mod.outMod->SetVolume(-666);
//	mod.outMod->SetVolume(255);

	killThread=0;
	thread_handle = (HANDLE)CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)PlayThread,(void *)&killThread,0,&tmp);
	return 0;
}

static void pause(void) { paused=1; mod.outMod->Pause(1); }
static void unpause(void) { paused=0; mod.outMod->Pause(0); }
static int ispaused(void) { return paused; }

static void stop(void) {
	if (thread_handle != INVALID_HANDLE_VALUE)
	{
		killThread=1;
		if (WaitForSingleObject(thread_handle,INFINITE) == WAIT_TIMEOUT)
		{
			MessageBox(mod.hMainWindow,"error asking thread to die!\n","error killing decode thread",0);
			TerminateThread(thread_handle,0);
		}
		CloseHandle(thread_handle);
		thread_handle = INVALID_HANDLE_VALUE;
	}
	mod.outMod->Close();
	if (mod.SAVSADeInit) mod.SAVSADeInit();
	unload_org();
}

static int getoutputtime(void)
{
	int time;
	if (mod.outMod->GetOutputTime() <= (signed)(org.loop_start*org.tempo))
		return mod.outMod->GetOutputTime();
	time = mod.outMod->GetOutputTime() - org.loop_start*org.tempo;
	return org.loop_start*org.tempo + time%((org.loop_end-org.loop_start)*org.tempo);
}
static void setoutputtime(int time_in_ms) { seek_needed=time_in_ms; }

static void setvolume(int volume) { mod.outMod->SetVolume(volume); }
static void setpan(int pan) { mod.outMod->SetPan(pan); }

static void eq_set(int on, char data[10], int preamp) { (void)on; (void)data; (void)preamp; }

In_Module mod = {
	IN_VER,
	"JTE's ORG Player "VERSION,
	NULL, // hMainWindow
	NULL, // hDllInstance
	"org\0Organya Music Data (*.org)\0",
	1, // is_seekable
	1, // uses output
	config,
	about,
	init,
	quit,
	getfileinfo,
	infoDlg,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,

	getlength,
	getoutputtime,
	setoutputtime,
	setvolume,
	setpan,
	NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL, // vis stuff
	NULL,NULL, // dsp
	eq_set,
	NULL, // SetInfo
	NULL // outMod
};
__declspec(dllexport) In_Module *winampGetInModule2(void) { return &mod; }

static int get_576_samples(char *buf)
{
	int l, wave;
	double pan, move;
	if(org.nonlooping && org.step >= org.loop_start)
		return 0;
	for(l = 0; l < 576*NCH*(BPS/8); l++)
	{
		int i;
		// Calculate the current waves
		if(l%(NCH*(BPS/8)) == 0)
		{
			for(i = 0; i < CHANNELS; i++)
			{
				double fraction;
				if(org.chan[i].pos < -1.0)
					continue;
				if(((i < CHANNELS/2 && org.chan[i].playing.len <= 0) || i >= CHANNELS/2)
					&& org.chan[i].pos > 0.0 && (unsigned int)org.chan[i].pos >= org.chan[i].length+1)
				{
					org.chan[i].pos = -2.0;
					org.chan[i].wave = 0;
					continue;
				}
				fraction = org.chan[i].pos - floor(org.chan[i].pos);
				if (i >= CHANNELS/2) {
					if ((int)org.chan[i].pos > 0)
						org.chan[i].wave = (signed)(org.chan[i].block[(int)(org.chan[i].pos-1)]-0x80)*(1.0-fraction);
					else
						org.chan[i].wave = 0;
					if (((unsigned int)org.chan[i].pos)+1 < org.chan[i].length)
						org.chan[i].wave += (signed)(org.chan[i].block[(int)org.chan[i].pos]-0x80)*fraction;
				}
				else {
					if ((int)org.chan[i].pos > 0)
						org.chan[i].wave = (signed char)org.chan[i].block[(int)(org.chan[i].pos-1)]*(1.0-fraction);
					else
						org.chan[i].wave = 0;
					if (((unsigned int)org.chan[i].pos)+1 < org.chan[i].length)
						org.chan[i].wave += (signed char)org.chan[i].block[(int)org.chan[i].pos]*fraction;
				}
				org.chan[i].wave *= org.chan[i].playing.vol / 256.0;
			}
		}

		// Copy in the current waves
		wave = 0x00;
		for(i = 0; i < CHANNELS; i++)
		{
			if(org.chan[i].pos < -1.0)
				continue;
			pan = 1.0;
			if(NCH == 2)
			{
				pan = org.chan[i].playing.pan/8.0;
				if((l/(BPS/8))%NCH == 0)
					pan = 1.5-pan;
				if(pan > 1.0)
					pan = 1.0;
			}
			//if (i>=CHANNELS/2)
			wave += org.chan[i].wave*pan;
		}

		// Clip the wave, don't overflow!
		if (wave > 0x7F)
			wave = 0x7F;
		if (wave < -0x80)
			wave = -0x80;
		if (BPS == 8)
			buf[l] = wave+0x80;
		else
			buf[l] = wave;

		// Move the waves along
		if(l%(NCH*(BPS/8)) == 0)
		{
			for(i = 0; i < CHANNELS; i++)
			{
				if(org.chan[i].pos < -1.0)
					continue;
				if(i < CHANNELS/2)
					move = pow(2.0,(org.chan[i].playing.key/12.0)-2.4);
				else
					move = org.chan[i].playing.key/28.0;
				if (move < 0.0)
					move = -move;
				move *= (double)org.chan[i].freq/SAMPLERATE;
				org.chan[i].pos += move;
				if (!org.chan[i].playing.dimmed && org.chan[i].pos >= org.chan[i].length)
				{
					org.chan[i].playing.vol = 3 * org.chan[i].playing.vol / 4;
					org.chan[i].playing.dimmed = TRUE;
				}
				while(i < CHANNELS/2 && org.chan[i].pos >= org.chan[i].length && org.chan[i].playing.len > 0)
					org.chan[i].pos -= org.chan[i].length;
			}
			org.tick++;
			if(org.tick >= (unsigned)org.tempo*SAMPLERATE/1000)
			{
				org.tick = 0;
				if(set_step(org.step+1))
					break;
			}
		}

	}
	return l;
}

DWORD WINAPI __stdcall PlayThread(void *b)
{
	int done = 0;
	while (! *((int *)b) )
	{
		if (seek_needed != -1)
		{
			int offs;
			decode_pos_ms = seek_needed-(seek_needed%1000);
			seek_needed = -1;
			done = 0;
			mod.outMod->Flush(decode_pos_ms);
			offs = decode_pos_ms/org.tempo;
			clear_sound();
			set_step(offs);
		}
		if (done)
		{
			mod.outMod->CanWrite();
			if (!mod.outMod->IsPlaying())
			{
				PostMessage(mod.hMainWindow,WM_WA_MPEG_EOF,0,0);
				return 0;
			}
			Sleep(10);
		}
		else if (mod.outMod->CanWrite() >= ((576*NCH*(BPS/8))<<(mod.dsp_isactive()?1:0)))
		{
			int l=576*NCH*(BPS/8);
			l=get_576_samples(sample_buffer);
			if(!l)
				done = 1;
			else
			{
				//if (mod.dsp_isactive()) l=mod.dsp_dosamples((short int *)sample_buffer,l/NCH/(BPS/8),BPS,NCH,SAMPLERATE)*(NCH*(BPS/8));
				if (mod.SAAddPCMData) mod.SAAddPCMData((char *)sample_buffer,NCH,BPS,decode_pos_ms);
				if (mod.VSAAddPCMData) mod.VSAAddPCMData((char *)sample_buffer,NCH,BPS,decode_pos_ms);
				decode_pos_ms+=(l/NCH/(BPS/8))*1000/SAMPLERATE;
				mod.outMod->Write(sample_buffer,l);
			}
		}
		else Sleep(20);
	}
	return 0;
}
