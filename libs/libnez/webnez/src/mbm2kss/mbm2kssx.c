// Mamiya's mbm2kss converter logic (see https://nezplug.sourceforge.net/ )
// somewhat improved and adapted for use in webNEZ (added "edit mode" format support. 
// avoid auto-looping) 
//
// file naming considerations: original MSX music file names are limited to 8+3 (MSX-)DOS
// garbage which makes them quite illegible and forces you to actually read the meta info
// within the MBM file to know what is inside. The "original" names were often arbitrarily
// chosen - by whoever ripped the songs from some obfuscated music disk (or it is even the
// original obfuscated names). The only "advantage" that these names have is that they
// will work in the orginal MSX machines. I'll rather have real names to ease file system
// level collection browsing.

// known limitation: MBM files stupidly lack the information if they were designed to run
// on 50Hz or on 60Hz machines, i.e. it is impossible to run all files correctly without
// respective "manual" user intervention. Seems that the next best workaround is to assume
// a 60Hz default and use a "50Hz" tag in the filename for every song that needs it.

// Original MBM files have exceptionally poor data quality with regard referenced MBK lib
// files and "original" players seem to use various fallback strategies to find some
// MBK file that might be used.. this is idiotic since a wrong MBK will make a song
// sound horrible. Also in the Web context this "let's search what MBK files we might find"
// approach may lead to a poor user experience, i.e. Web servers may just blacklist and
// block user sessions that repeatedly try to access non existing files (suspecting some 
// kind of denial of service attack). Cleanup garbage files before trying to play them!

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>

#define KSS_HEADER_SIZE 0x20
#define HEADER_SIZE 0x200
static char kss_header[KSS_HEADER_SIZE + HEADER_SIZE] = {
#include "mbm2kssh.h"
};

#include <libgen.h>

static int mbr143_replay_size = 4992;
static const unsigned char mbr143_replay[] = {
#include "mbr143.h"
};

static int drv_size;
static int mbplay;
static int mbkload;
static int mbmload;
static unsigned char *drv_top;
static unsigned char drv_code[0x8000];

#define MAX_PATH_LEN 512

static void fut_mbkname(char *d, char *s)
{
	unsigned l = 8;
	d += strlen(d);
	while (l-- && *s != ' ') *d++ = *s++;
	*d = '\0';
}

// get the path from the filename in s
static void fut_getpath(char *d, char *s)
{
    char *s2 = strdup(s);
	char* dir = dirname(s2);
	strcpy(d, dir);
    free(s2);

	int len= strlen(d);
	d[len] = '/';
	d[len+1] = 0;
}

static void fut_getnonext(char *d, char *s)
{
    char *s2 = strdup(s);
	int len= strlen(s2);

	if ( (strcmp(&s2[len - 4], ".MBM") == 0) ||
		(strcmp(&s2[len - 4], ".mbm") == 0) )
	{
		s2[len - 4] = 0;
	}
	strcpy(d, s2);
    free(s2);
}

static int bin_search(unsigned char *pp, int pl, unsigned char *bp, int bl)
{
	int p;
	for (p = 0; p + pl < bl; p++) if (!memcmp(pp, bp + p, pl)) return p;
	return 0;
}

static int bin_readword(unsigned char *b)
{
	return (((int)b[1]) << 8) + b[0];
}

static int mbdrv_bin_load()
{
	int drvtop;
	memset(drv_code, 0, sizeof(drv_code));

	drv_size= mbr143_replay_size;
	memcpy(drv_code, mbr143_replay, drv_size);

	if (drv_size < 256) return 0;
    drvtop = bin_search("AB\0", 4, drv_code, drv_size);	// 235

	// unfortunately the crappy MSX-BASIC player program does not
	// directly interact with the below driver to control the
	// "looping" behavior. but instead sets some var in the separate
	// Moonsoft player lib.. it is unclear if the "no-loop" feature
	// even is available on the driver level or if needs to be
	// handled externally.. (useless for kss version)

    if (!drvtop) return 0;
    drv_top = drv_code + drvtop;
    drv_size -= drvtop;

	// the lib exposes various functions for use in MSX-BASIC progs, e.g.
	// MBPLAY, MBSTOP, MBCONT, MBHALT, MBCHIP, MBBANK, MBADDR, MBFADE,
	// MBVER, MBMLOAD, MBKLOAD

    mbplay = bin_search("MBPLAY", 7, drv_top, drv_size);
    if (!mbplay) return 0;
    mbplay = bin_readword(drv_top + mbplay + 7);

    mbmload = bin_search("MBMLOAD", 8, drv_top, drv_size);
    if (!mbmload) return 0;
    mbmload = bin_readword(drv_top + mbmload + 8);

    mbkload = bin_search("MBKLOAD", 8, drv_top, drv_size);
    if (!mbkload) return 0;
    mbkload = bin_readword(drv_top + mbkload + 8);

	drv_top[0x4000 - 0x4000] = (drv_size >> 0) & 255;
	drv_top[0x4001 - 0x4000] = (drv_size >> 8) & 255;
	drv_top[0x4002 - 0x4000] = '\0';
	drv_top[0x4003 - 0x4000] = '\0';
	drv_top[0x4004 - 0x4000] = '\xc3';
	drv_top[0x4005 - 0x4000] = (mbplay >> 0) & 255;
	drv_top[0x4006 - 0x4000] = (mbplay >> 8) & 255;
	drv_top[0x4007 - 0x4000] = '\0';
	drv_top[0x4008 - 0x4000] = '\xc3';
	drv_top[0x4009 - 0x4000] = (mbkload >> 0) & 255;
	drv_top[0x400a - 0x4000] = (mbkload >> 8) & 255;
	drv_top[0x400b - 0x4000] = '\0';
	drv_top[0x400c - 0x4000] = '\xc3';
	drv_top[0x400d - 0x4000] = (mbmload >> 0) & 255;
	drv_top[0x400e - 0x4000] = (mbmload >> 8) & 255;
	drv_top[0x400f - 0x4000] = '\0';

	return 1;
}

extern int ems_request_file(const char *filename); // must be implemented on JavaScript side (also see callback.js)


int mbm_initialized= 0;

void mbm2kss_init()
{
	if (!mbm_initialized)
	{
		if (!mbdrv_bin_load()) {
			fprintf(stderr, "ERROR could not initialize MBM converter\n");
		}
		mbm_initialized = 1;
	}
}


static char mbm_title[0x29];

void copyTitle(unsigned char *data, unsigned int size)
{
	if (size < (0x0CF +0x28) ) {
		mbm_title[0] = 0;
	}
	else {
		int i;
		for ( i = 0; i < 0x28; i++)
			mbm_title[i] = (char)data[0x0CF + i];
		mbm_title[i] = 0;
	}
}

char *mbm2kss_title(unsigned char *data, unsigned int size)
{
	return mbm_title;
}

// memory buffer containing the last successful conversion's result
static unsigned char *kss_buffer = NULL;
static int kss_buffer_len = 0;
static int kss_buf_pos = 0;


void kss_init(int len)
{
	if (kss_buffer) free(kss_buffer);	// last result is never freed (doesn't matter for these small files)

	kss_buffer_len = len;
	kss_buffer = (unsigned char *)malloc(kss_buffer_len);
	kss_buf_pos = 0;
}

void kss_append(unsigned char *data, int len)
{
	memcpy(&kss_buffer[kss_buf_pos], data, len);
	kss_buf_pos+= len;
}

void mbm2kss_result(unsigned char **buffer, unsigned int *length)
{
	*buffer = kss_buffer;
	*length = kss_buf_pos;
}

void subShort(unsigned char *buffer, unsigned char offset) {
	// allow for unaligned memeory access just in case
	unsigned short s= buffer[0] | ((unsigned short)buffer[1] << 8);

	s -= offset;

	buffer[0] = s & 0xff;
	buffer[1] = s >> 8;	
}


int realLen;

int mbm2kss_length() {
	return realLen;
}


void convertEdit2UserMode(unsigned char **buffer, size_t *length) {
	if ((*buffer)[0] == 0xff) {

		(*length)--;	// remove '0xff' marker
		memmove(*buffer, (*buffer)+1, *length);	// not counted in below pattern addresses..

		const unsigned char songLen = (*buffer)[0];
		realLen = songLen; // used to end looping
		
		
		// proper conversion to "user mode" file requires trimming of 
		// "unused data" in multiple instances:
		// 1) instead of fixed 200 "Position table" entries there must 
		//    only be the "songLen" used ones
		// 2) Instead of using the fixed maximum of 76 "Pattern Address table" 
		//    entries the highest used "Position table" entry defines the 
		//    number of entries.
		// 3) there is an extra "song length" byte between the "Position table" entries
		//    and the "Pattern Address table" - which must be removed
		// 4) the little endian "Pattern Address table" entries must be adjusted 
		//    to reflect the distance that the data is moved (see above)
		
		// for a test song that exists in a "edit" as well as in a "user" version
		// the above adjustments interestingly DID not yield a converted 
		// file with the correct "Pattern Address table" offsets.. (there seems to be
		// a discepancy of +10 that I cannot explain so far).. the few songs available
		// do not seem to be worth the trouble to put more time into reverse engineering..

		// a slight hack is therefore used here: the "edit" mode file layout is 
		// almost correct if this was a 199 length song, so rather than moving
		// the data around a fake size is used - which is compensated for 
		// by using the actual length in the existing "loop ending" logic
		// known limitation: the song will play past the songs end if a playtime
		// is configured manually - the songs cannot be run in looped mode..
		
		// change songlength to account for fixed number of 
		// "Position table" entries in the "edit mode" file
		
		(*buffer)[0]= 199;	
		
		// only remove (redundant) "edit mode" specific length byte
		
		memmove( &((*buffer)[0x240]), &((*buffer)[0x241]), (*length)- 0x241);	
		
		// adjust "Pattern Address table" entries (for removed length byte)
		
		for (int i = 0; i<76; i++) {
			subShort(&(*buffer)[0x240+i*2], 1);
		}
	}
	else {
		realLen= (*buffer)[0];
	}
}

// @return -1 file not ready; 0 success; 1 fail
int mbm2kss(char *mbmpath, int devtype, int isntsc)
{
	int res = 0;	// success

	FILE *ifp/* = 0*/;
	char *mbmp = 0;
	char *mbkp = 0;
	FILE *ofp = 0;
	char ksspath[MAX_PATH_LEN];
	char mbkpath[MAX_PATH_LEN];
	enum {
		phase_mbmload,
		/*phase_mbkload,*/
		phase_ksssave,
		phase_ok
	} phase;
	while (1)
	{
		size_t mbms;
		size_t mbks;
		int tmp;
		phase = phase_mbmload;

		// no async handling needed here since song file
		// has already been loaded..

		ifp = fopen(mbmpath, "rb");
		if (!ifp) break;
		fseek(ifp, 0, SEEK_END);
		mbms = ftell(ifp);
		fseek(ifp, 0, SEEK_SET);

		if (mbms < 0x100) break;	// testcase: MG_ALERT.MBM

		mbmp = malloc(mbms);
		if (!mbmp) break;
		if (mbms != fread(mbmp, 1, mbms, ifp)) break;
		fclose(ifp);/* ifp = 0; */
		ifp = 0;

		convertEdit2UserMode((unsigned char **)&mbmp, &mbms);
		copyTitle((unsigned char *)mbmp, mbms);

/*		
		if (*mbmp =='\xff') {
			fprintf(stderr, "error: unsupported 'edit' mode MoonBlaster file detected.\n");
			res = 1;
			break;
		}
*/
		/* phase = phase_mbkload; */
		for (tmp = 0; tmp < 2; tmp++)
		{
			int quit = 0;
			switch (tmp)
			{
				case 0:
					// with the heaps of inconsistent/incomplete garbage song files
					// floating around it is difficult to say what the correct semantics of the
					// "NONE" entry actually is. Many songs seem to have been ripped from some
					// runtime environment that used a specific drumkit and didn't care about
					// whatever unused entry might be specified in the songs. Many songs that
					// originally contain "NONE" audibly play incorrectly without a drumkit.

					if (!strncmp("NONE    ", mbmp + 0x140, 8)) {

						quit= 1;
						break;		// don't allow use of fallback names
//						continue;	// allow fallback to song name (pretty useless with non-DOS filename length..)
					}
					fut_getpath(mbkpath, mbmpath);
					fut_mbkname(mbkpath, mbmp + 0x140);
					break;
				case 1:
					fut_getnonext(mbkpath, mbmpath);
					break;
					/*
				case 2:
					// I never actually saw this used in any of the songs I found..
					// so there is no point to use is

					fut_getpath(mbkpath, mbmpath);
					strcat(mbkpath, "DEFAULT");
					break;
					*/
			}

			// all the files I saw only used upper case MBK filename
			// references, so only use upper case for extension as well

			strcat(mbkpath, ".MBK");

			if (quit) break;

			if (ems_request_file(mbkpath) < 0)	{
				if (mbmp) free(mbmp);
				 return -1;	// retry later (handle async loading)
			}

			ifp = fopen(mbkpath, "rb");
			if (ifp)
			{
				fseek(ifp, 0, SEEK_END);
				mbks = ftell(ifp);
				fseek(ifp, 0, SEEK_SET);

				if (mbks > 0)	// testcase: 5CIRCLES.MBK etc
				{
					char *p = calloc(1, mbks);
					if (p && mbks == fread(p, 1, mbks, ifp))
					{
						mbkp = p;
						p = 0;
					}
					if (p) free(p);
				}
				fclose(ifp); ifp = 0;
			}
			if (mbkp) {
				res= 0;	// reset
				break;
			}
			if (tmp == 0) {
				fprintf(stderr, "mbk load error(%s).\n", mbkpath);
				res= 1;
			}
		}

		drv_top[0x4008 - 0x4000] = mbkp ? '\xc3' : '\xc9';

		tmp = drv_size + HEADER_SIZE + (mbkp ? 0x38 : 0);
		kss_header[0x6] = (tmp >> 0) & 255;
		kss_header[0x7] = (tmp >> 8) & 255;
		kss_header[0xc] = mbkp ? 4 : 6;
		kss_header[0xd] = mbkp ? 3 : 1;
		switch (devtype)
		{
			default:
			case 0:	/* msx-audio */
				kss_header[0xf] = 4 + 8;
				kss_header[KSS_HEADER_SIZE + 0x03] = '0';
				break;
			case 1:	/* msx-music */
				kss_header[0xf] = 4 + 1;
				kss_header[KSS_HEADER_SIZE + 0x03] = '1';
				break;
			case 2:	/* both */
				kss_header[0xf] = 4 + 8 + 1;
				kss_header[KSS_HEADER_SIZE + 0x03] = '2';
				break;
			case 3:	/* stereo */
				kss_header[0xf] = 4 + 8 + 1 + 16;
				kss_header[KSS_HEADER_SIZE + 0x03] = '2';
				break;
		}
		if (!isntsc) kss_header[0xf] |= 0x40;
		memset(&kss_header[KSS_HEADER_SIZE + 0x10], 0, 0x30);
		memcpy(&kss_header[KSS_HEADER_SIZE + 0x10], mbmp + 0xcf, 0x28);

		phase = phase_ksssave;

		kss_init(KSS_HEADER_SIZE + HEADER_SIZE + drv_size + (mbkp ? 0x8038 : 0) + mbms);

		kss_append(kss_header, KSS_HEADER_SIZE + HEADER_SIZE);
		kss_append(drv_top, drv_size);
		if (mbkp)
			kss_append(mbkp, 0x8038);
		kss_append(mbmp, mbms);

		phase = phase_ok;
		break;
	}
	switch (phase)
	{
		case phase_mbmload:
		/*
			if (mbmp && *mbmp =='\xff')
				fprintf(stderr, "format error(%s).\n", mbmpath);
			else
		*/
				fprintf(stderr, "mbmload load error(%s).\n", mbmpath);
				if (res != -1) res= 1;
			break;
		case phase_ksssave:
			fprintf(stderr, "ksssave load error(%s).\n", ksspath);
			if (res != -1) res= 1;
			break;
		default:
			break;
	}
	if (mbmp) free(mbmp);
	if (mbkp) free(mbkp);
	if (ofp) fclose(ofp);
	return res;
}


