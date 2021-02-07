// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>


extern int loadedsize;
extern "C" {
//#include <png.h>
    char gsf_libfile[1024];
}

extern "C" int relvolume;
extern "C" int defvolume;

#if 0
#include "unrarlib.h"
#endif

#include "System.h"
#include "NLS.h"
#include "Util.h"
//#include "Flash.h"
#include "GBA.h"
#include "Globals.h"
//#include "RTC.h"
#include "Port.h"



extern "C" {
#include "memgzio.h"
#include "psftag.h"
}

#ifndef _MSC_VER
#define _stricmp strcasecmp
#endif // ! _MSC_VER

//extern int systemColorDepth;
//extern int systemRedShift;
//extern int systemGreenShift;
//extern int systemBlueShift;

//extern u16 systemColorMap16[0x10000];
//extern u32 systemColorMap32[0x10000];

static int (ZEXPORT *utilGzWriteFunc)(gzFile, const voidp, unsigned int) = NULL;
static int (ZEXPORT *utilGzReadFunc)(gzFile, voidp, unsigned int) = NULL;
static int (ZEXPORT *utilGzCloseFunc)(gzFile) = NULL;
/*
bool utilWritePNGFile(const char *fileName, int w, int h, u8 *pix)
{
  u8 writeBuffer[512 * 3];
  
  FILE *fp = fopen(fileName,"wb");

  if(!fp) {
    systemMessage(MSG_ERROR_CREATING_FILE, N_("Error creating file %s"), fileName);
    return false;
  }
  
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                                NULL,
                                                NULL,
                                                NULL);
  if(!png_ptr) {
    fclose(fp);
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);

  if(!info_ptr) {
    png_destroy_write_struct(&png_ptr,NULL);
    fclose(fp);
    return false;
  }

  if(setjmp(png_ptr->jmpbuf)) {
    png_destroy_write_struct(&png_ptr,NULL);
    fclose(fp);
    return false;
  }

  png_init_io(png_ptr,fp);

  png_set_IHDR(png_ptr,
               info_ptr,
               w,
               h,
               8,
               PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE,
               PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png_ptr,info_ptr);

  u8 *b = writeBuffer;

  int sizeX = w;
  int sizeY = h;

  switch(systemColorDepth) {
  case 16:
    {
      u16 *p = (u16 *)(pix+(w+2)*2); // skip first black line
      for(int y = 0; y < sizeY; y++) {
         for(int x = 0; x < sizeX; x++) {
          u16 v = *p++;
          
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G 
          *b++ = ((v >> systemBlueShift) & 0x01f) << 3; // B
        }
        p++; // skip black pixel for filters
        p++; // skip black pixel for filters
        png_write_row(png_ptr,writeBuffer);
        
        b = writeBuffer;
      }
    }
    break;
  case 24:
    {
      u8 *pixU8 = (u8 *)pix;
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          if(systemRedShift < systemBlueShift) {
            *b++ = *pixU8++; // R
            *b++ = *pixU8++; // G
            *b++ = *pixU8++; // B
          } else {
            int blue = *pixU8++;
            int green = *pixU8++;
            int red = *pixU8++;
            
            *b++ = red;
            *b++ = green;
            *b++ = blue;
          }
        }
        png_write_row(png_ptr,writeBuffer);
        
        b = writeBuffer;
      }
    }
    break;
  case 32:
    {
      u32 *pixU32 = (u32 *)(pix+4*(w+1));
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u32 v = *pixU32++;
          
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G
          *b++ = ((v >> systemBlueShift) & 0x001f) << 3; // B
        }
        pixU32++;
        
        png_write_row(png_ptr,writeBuffer);
        
        b = writeBuffer;
      }
    }
    break;
  }
  
  png_write_end(png_ptr, info_ptr);

  png_destroy_write_struct(&png_ptr, &info_ptr);

  fclose(fp);

  return true;  
}*/

void utilPutDword(u8 *p, u32 value)
{
  *p++ = value & 255;
  *p++ = (value >> 8) & 255;
  *p++ = (value >> 16) & 255;
  *p = (value >> 24) & 255;
}

void utilPutWord(u8 *p, u16 value)
{
  *p++ = value & 255;
  *p = (value >> 8) & 255;
}
/*
void utilWriteBMP(char *buf, int w, int h, u8 *pix)
{
  u8 *b = (u8 *)buf;

  int sizeX = w;
  int sizeY = h;

  switch(systemColorDepth) {
  case 16:
    {
      u16 *p = (u16 *)(pix+(w+2)*(h)*2); // skip first black line
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u16 v = *p++;

          *b++ = ((v >> systemBlueShift) & 0x01f) << 3; // B      
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G 
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
        }
        p++; // skip black pixel for filters
        p++; // skip black pixel for filters
        p -= 2*(w+2);
      }
    }
    break;
  case 24:
    {
      u8 *pixU8 = (u8 *)pix+3*w*(h-1);
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          if(systemRedShift > systemBlueShift) {
            *b++ = *pixU8++; // B
            *b++ = *pixU8++; // G
            *b++ = *pixU8++; // R           
          } else {
            int red = *pixU8++;
            int green = *pixU8++;
            int blue = *pixU8++;
            
            *b++ = blue;
            *b++ = green;
            *b++ = red;
          }
        }
        pixU8 -= 2*3*w;
      }
    }
    break;
  case 32:
    {
      u32 *pixU32 = (u32 *)(pix+4*(w+1)*(h));
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u32 v = *pixU32++;

          *b++ = ((v >> systemBlueShift) & 0x001f) << 3; // B     
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
        }
        pixU32++;
        pixU32 -= 2*(w+1);
      }
    }
    break;
  }  
}*/
/*
bool utilWriteBMPFile(const char *fileName, int w, int h, u8 *pix)
{
  u8 writeBuffer[512 * 3];
  
  FILE *fp = fopen(fileName,"wb");

  if(!fp) {
    systemMessage(MSG_ERROR_CREATING_FILE, N_("Error creating file %s"), fileName);
    return false;
  }

  struct {
    u8 ident[2];
    u8 filesize[4];
    u8 reserved[4];
    u8 dataoffset[4];
    u8 headersize[4];
    u8 width[4];
    u8 height[4];
    u8 planes[2];
    u8 bitsperpixel[2];
    u8 compression[4];
    u8 datasize[4];
    u8 hres[4];
    u8 vres[4];
    u8 colors[4];
    u8 importantcolors[4];
    //    u8 pad[2];
  } bmpheader;
  memset(&bmpheader, 0, sizeof(bmpheader));

  bmpheader.ident[0] = 'B';
  bmpheader.ident[1] = 'M';

  u32 fsz = sizeof(bmpheader) + w*h*3;
  utilPutDword(bmpheader.filesize, fsz);
  utilPutDword(bmpheader.dataoffset, 0x36);
  utilPutDword(bmpheader.headersize, 0x28);
  utilPutDword(bmpheader.width, w);
  utilPutDword(bmpheader.height, h);
  utilPutDword(bmpheader.planes, 1);
  utilPutDword(bmpheader.bitsperpixel, 24);
  utilPutDword(bmpheader.datasize, 3*w*h);

  fwrite(&bmpheader, 1, sizeof(bmpheader), fp);

  u8 *b = writeBuffer;

  int sizeX = w;
  int sizeY = h;

  switch(systemColorDepth) {
  case 16:
    {
      u16 *p = (u16 *)(pix+(w+2)*(h)*2); // skip first black line
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u16 v = *p++;

          *b++ = ((v >> systemBlueShift) & 0x01f) << 3; // B      
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G 
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
        }
        p++; // skip black pixel for filters
        p++; // skip black pixel for filters
        p -= 2*(w+2);
        fwrite(writeBuffer, 1, 3*w, fp);
        
        b = writeBuffer;
      }
    }
    break;
  case 24:
    {
      u8 *pixU8 = (u8 *)pix+3*w*(h-1);
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          if(systemRedShift > systemBlueShift) {
            *b++ = *pixU8++; // B
            *b++ = *pixU8++; // G
            *b++ = *pixU8++; // R           
          } else {
            int red = *pixU8++;
            int green = *pixU8++;
            int blue = *pixU8++;
            
            *b++ = blue;
            *b++ = green;
            *b++ = red;
          }
        }
        pixU8 -= 2*3*w;
        fwrite(writeBuffer, 1, 3*w, fp);
        
        b = writeBuffer;
      }
    }
    break;
  case 32:
    {
      u32 *pixU32 = (u32 *)(pix+4*(w+1)*(h));
      for(int y = 0; y < sizeY; y++) {
        for(int x = 0; x < sizeX; x++) {
          u32 v = *pixU32++;

          *b++ = ((v >> systemBlueShift) & 0x001f) << 3; // B     
          *b++ = ((v >> systemGreenShift) & 0x001f) << 3; // G
          *b++ = ((v >> systemRedShift) & 0x001f) << 3; // R
        }
        pixU32++;
        pixU32 -= 2*(w+1);
        
        fwrite(writeBuffer, 1, 3*w, fp);
        
        b = writeBuffer;
      }
    }
    break;
  }

  fclose(fp);

  return true;
}
*/
static int utilReadInt2(FILE *f)
{
  int res = 0;
  int c = fgetc(f);
  if(c == EOF)
    return -1;
  res = c;
  c = fgetc(f);
  if(c == EOF)
    return -1;
  return c + (res<<8);
}

static int utilReadInt3(FILE *f)
{
  int res = 0;
  int c = fgetc(f);
  if(c == EOF)
    return -1;
  res = c;
  c = fgetc(f);
  if(c == EOF)
    return -1;
  res = c + (res<<8);
  c = fgetc(f);
  if(c == EOF)
    return -1;
  return c + (res<<8);
}

/*void utilApplyIPS(const char *ips, u8 **r, int *s)
{
  // from the IPS spec at http://zerosoft.zophar.net/ips.htm
  FILE *f = fopen(ips, "rb");
  if(!f)
    return;
  u8 *rom = *r;
  int size = *s;
  if(fgetc(f) == 'P' &&
     fgetc(f) == 'A' &&
     fgetc(f) == 'T' &&
     fgetc(f) == 'C' &&
     fgetc(f) == 'H') {
    int b;
    int offset;
    int len;
    for(;;) {
      // read offset
      offset = utilReadInt3(f);
      // if offset == EOF, end of patch
      if(offset == 0x454f46)
        break;
      // read length
      len = utilReadInt2(f);
      if(!len) {
        // len == 0, RLE block
        len = utilReadInt2(f);
        // byte to fill
        int c = fgetc(f);
        if(c == -1)
          break;
        b = (u8)c;
      } else
        b= -1;
      // check if we need to reallocate our ROM
      if((offset + len) >= size) {
        size *= 2;
        rom = (u8 *)realloc(rom, size);
        *r = rom;
        *s = size;
      }      
      if(b == -1) {
        // normal block, just read the data
        if(fread(&rom[offset], 1, len, f) != (size_t)len)
          break;
      } else {
        // fill the region with the given byte
        while(len--) {
          rom[offset++] = b;
        }
      }
    }
  }
  // close the file
  fclose(f);
}
*/

extern bool cpuIsMultiBoot;

Byte *compbuf;
Byte *uncompbuf;

extern "C" int TrackLength;
extern "C" int FadeLength;
extern "C" int IgnoreTrackLength;
extern "C" int deflen,deffade,silencelength,silencedetected;


extern "C"
{
int LengthFromString(const char * timestring) {
	int c=0,decimalused=0,multiplier=1;
	int total=0;
	if (strlen(timestring) == 0) return 0;
	for (c=strlen(timestring)-1; c >= 0; c--) {
		if (timestring[c]=='.' || timestring[c]==',') {
			decimalused=1;
			total*=1000/multiplier;
			multiplier=1000;
		} else if (timestring[c]==':') multiplier=multiplier*6/10;
		else {
			total+=(timestring[c]-'0')*multiplier;
			multiplier*=10;
		}
	}
	if (!decimalused) total*=1000;
	return total;
}

}

extern "C"
{
	int VolumeFromString(const char * volumestring) {
	int c=0,decimalused=0,multiplier=1;
	int total=0;
	if(strlen(volumestring) == 0) return 0;
	for(c=strlen(volumestring)-1; c >= 0; c--) {
		if (volumestring[c]=='.' || volumestring[c]==',') {
			decimalused=1;
			total*=1000/multiplier;
			multiplier=1000;
		} 
		else if ((volumestring[c]>='0')&& (volumestring[c]<='9')) {
			total+=(volumestring[c]-'0')*multiplier;
			multiplier*=10;
		}
		else
			break;
	}
	if (!decimalused) total*=1000;
	//return (float) total / 1000.;
	return total;
}
}

#ifdef LINUX
	#include "../types.h"
#else
	#include <windows.h>
#endif

/*void gsfDisplayError (char * Message, ...) {
	char Msg[400];
	va_list ap;

	va_start( ap, Message );
	vsprintf( Msg, Message, ap );
	va_end( ap );
	MessageBox(NULL,Msg,"Error",MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
	//SetActiveWindow(hMainWindow);
}*/

struct GSF_FILE {
	Byte *program;
	Byte *reserved;
	char psftag[50001];
	char libname[0x40];
	bool gsfloaded;
};

static int copy_int(unsigned int *dst, unsigned char *src)
{
	*dst = src[0] | (src[1]<<8) | (src[2]<<16) | (src[3]<<24);
    return 0;
}

static int fread_int(unsigned int *dst, FILE *f)
{
	unsigned char tmpbuf[4];
	int r;

	r = fread(tmpbuf,1,4,f);
	if (r<0) { return r; }
	
	*dst = tmpbuf[0] | (tmpbuf[1]<<8) | (tmpbuf[2]<<16) | (tmpbuf[3]<<24);

	return r;
}

GSF_FILE *decompressGSF(const char * file, int libnum=1)
{
	GSF_FILE *gsffile;
	char libtag[0x40];
	char libname[0x8];
	unsigned int filesize;
    unsigned int header;
    unsigned int reserved;
    unsigned int program;
    unsigned int ccrc;
    unsigned long decompsize=12;
	unsigned int tmpval;
	FILE *f;
    
    gsffile=(GSF_FILE*)malloc(sizeof(GSF_FILE));
    if (!gsffile) return NULL;
    
	memset(gsffile->psftag,0,sizeof(gsffile->psftag));
	gsffile->program=NULL;
	gsffile->reserved=NULL;
	memset(gsffile->libname,0,sizeof(gsffile->libname));
	gsffile->gsfloaded = false;
	
	f=fopen(file,"rb");
	 
     if(f==NULL) {
#ifdef LINUX
		 perror("fopen");
#endif
		  return gsffile;
	 }
	  fseek(f,0,SEEK_END); filesize=ftell(f); fseek(f,0,SEEK_SET);
	
	  if((filesize<0x10)||(filesize>0x4000000))
	  {
		  fclose(f);
#ifdef LINUX
			  printf("Bad file size\n");
#endif
		  return gsffile;
	  }
//	  fread(&header,1,4,f);
	  fread_int(&header, f);
	  
	  if(header!=0x22465350)
	  {
		  fclose(f);
#ifdef LINUX
			  printf("Bad header\n");
#endif
		  return gsffile;
	  }
	  fread_int(&reserved, f);
//	  fread(&reserved,1,4,f);
	  fread_int(&program, f);
//	  fread(&program,1,4,f);
//	  fread(&ccrc,1,4,f);
	  fread_int(&ccrc, f);
	  
	  if((reserved+program+16)>filesize)
	  {
		  fclose(f);
#ifdef LINUX
			  printf("Incoherant sizes\n");
#endif
		  return gsffile;
	  }

	  if(reserved>0)
	  {
		  gsffile->reserved = (Byte*) malloc(reserved);
		  if(gsffile->reserved==NULL)
		  {
			  fclose(f);
#ifdef LINUX
			  printf("1: Malloc failed %d\n", reserved);
#endif
			  return gsffile;
		  }
		  fread(gsffile->reserved,1,reserved,f);
	  }
	
	  if(program>0)
	  {
			compbuf = (Byte*) malloc(program);
			if(compbuf==NULL)
			{
				fclose(f);
#ifdef LINUX
			  printf("2: Malloc failed %d\n", program);
#endif
				return gsffile;
			}
			 
			fread(compbuf,1,program,f);
	//  		fread_int(&program, f);
			  
			if(ccrc != crc32(crc32(0L, Z_NULL, 0), compbuf, program))
			{
				fclose(f);
				free(compbuf);
#ifdef LINUX
			  printf("Bad crc\n");
#endif
				return gsffile;
			}
			uncompbuf = (Byte*) malloc(decompsize);
			if (uncompbuf == NULL)
			{
				fclose(f);
				free(compbuf);
#ifdef LINUX
			  printf("3: Malloc failed %ld\n", decompsize);
#endif
				return gsffile;
			}
			if(uncompress(uncompbuf,&decompsize,compbuf,program)!=Z_BUF_ERROR)
			{
#ifdef LINUX
			  printf("uncompression error\n");
#endif
				fclose(f);
				free(compbuf);
				free(uncompbuf);
				return gsffile;
			}
			//memcpy(&decompsize,uncompbuf+8,sizeof(decompsize));
		
			//memcpy(&tmpval,uncompbuf+8,sizeof(tmpval));
			copy_int(&tmpval, uncompbuf+8);
			
			decompsize = tmpval;
			free(uncompbuf);
			decompsize +=12;
			uncompbuf = (Byte*) malloc(decompsize);
			if (uncompbuf == NULL)
			{
				fclose(f);
				free(compbuf);
#ifdef LINUX
				perror("malloc");
			  printf("4: Malloc failed %ld\n", decompsize);
#endif
				return gsffile;
			}

			
			if(uncompress(uncompbuf,&decompsize,compbuf,program) != Z_OK)
			{
				fclose(f);
				free(compbuf);
				free(uncompbuf);
#ifdef LINUX
			  printf("uncompress error\n");
#endif
				return gsffile;
			}
			 
			uncompbuf+=3;
			if((*uncompbuf==0x02)&&(libnum==1))
				cpuIsMultiBoot = true;
			uncompbuf-=3;
			free(compbuf);
			gsffile->program=uncompbuf;
	  }
	  fread(gsffile->psftag,1,5,f);
#ifdef LINUX
	  if(!strcasecmp(gsffile->psftag,"[TAG]"))
	  {
	    fread(gsffile->psftag,1,50000,f);
	  }
	
#else
	  if(!stricmp(gsffile->psftag,"[TAG]"))
	  {
	    fread(gsffile->psftag,1,50000,f);
	  }
#endif

	  if(libnum==1)
		  sprintf(libname,"_lib");
	  else
		  sprintf(libname,"_lib%d",libnum);
	  if(!psftag_raw_getvar(gsffile->psftag,libname,libtag,sizeof(libtag)-1))
	  {
		  memcpy(gsffile->libname,libtag,sizeof(gsffile->libname));
	  }

	  fclose(f);
	  if((program+reserved)==0)
	  {
		  return gsffile;
	  }

	gsffile->gsfloaded = true;


	return gsffile;
}

#define MAX_GSFLIB 11

bool utildecompGSF(const char * file)
{
//	unsigned int decompsize=12;
	unsigned int offset;
	unsigned int size;
	char filename[1024];
	char tempname[1024];
	char libtag[0x40];
	char libname[8];
	char length[256],fade[256],volume[256];

	int i, j;

	GSF_FILE *gsffile, *gsflib[MAX_GSFLIB];
    for (int ii=0;ii<MAX_GSFLIB;ii++) {
        gsflib[ii]=NULL;
        if (ii>=2) gsflib[ii]=(GSF_FILE*)malloc(sizeof(GSF_FILE));
    }
	TrackLength=0;
	FadeLength=0;


	gsffile = decompressGSF(file,1);
	gsflib[0]=gsffile;
	if(gsffile->gsfloaded == false) {
		printf("Failed to load\n");
        
        for (int ii=0;ii<MAX_GSFLIB;ii++) {
            if (gsflib[ii]) free(gsflib[ii]);
        }
		return false;
	}

   
	
	//if(gsffile.libname[0]!=0) {
	if(!psftag_raw_getvar(gsffile->psftag,"_lib",libtag,sizeof(libtag)-1)) {

		utilGetBasePath(file,tempname);
#ifdef LINUX
		sprintf(filename,"%s/%s",tempname,libtag);
#else
		sprintf(filename,"%s\\%s",tempname,libtag);
#endif        
		gsflib[1] = decompressGSF(filename,2);

		if(gsflib[1]->gsfloaded == false)
		{
			printf("Failed to load library\n");
            strcpy(gsf_libfile,libtag);
			free(uncompbuf);
            
            for (int ii=0;ii<MAX_GSFLIB;ii++) {
                if (gsflib[ii]) free(gsflib[ii]);
            }
			return false;
		}

//		memcpy(&offset,gsffile->program+4,sizeof(offset));
		copy_int(&offset, gsffile->program+4);
//		memcpy(&size,gsffile->program+8,sizeof(size));
		copy_int(&size, gsffile->program+8);
		
		offset&=0x01FFFFFF;
		memcpy(gsflib[1]->program+12+offset,gsffile->program+12,size);
		free(gsffile->program);
		uncompbuf=gsflib[1]->program;


		for(i=2;i<MAX_GSFLIB;i++)
		{
			sprintf(libname,"_lib%d",i);
			for(j=0;j<i;j++)
			{
				if(!psftag_raw_getvar(gsflib[j]->psftag,libname,libtag,sizeof(libtag)-1)) {
#ifdef LINUX
                    sprintf(filename,"%s/%s",tempname,libtag);
#else
					sprintf(filename,"%s\\%s",tempname,libtag);
#endif
					gsflib[i] = decompressGSF(filename,i+1);
					if(gsflib[i]->gsfloaded == false)
					{
						free(uncompbuf);
                        
                        for (int ii=0;ii<MAX_GSFLIB;ii++) {
                            if (gsflib[ii]) free(gsflib[ii]);
                        }
						return false;
					}
//					memcpy(&offset,gsflib[i]->program+4,sizeof(offset));
					copy_int(&offset, gsflib[i]->program+4);
					//memcpy(&size,gsflib[i]->program+8,sizeof(size));
					copy_int(&size, gsflib[i]->program+8);
					offset&=0x01FFFFFF;
					memcpy(gsflib[1]->program+12+offset,gsflib[i]->program+12,size);
					free(gsflib[i]->program);
					break;
				}
			}
			if(gsflib[i]->program == NULL)
				break;
		}

		uncompbuf=gsflib[1]->program;

	}
	else
		uncompbuf=gsffile->program;

	psftag_raw_getvar(gsffile->psftag,"length",length,sizeof(length)-1);
	if (/*!IgnoreTrackLength &&*/ strlen(length))
		TrackLength=LengthFromString(length);
	if (TrackLength <= 0) {
	if (IgnoreTrackLength)
		TrackLength=0;
	}
	psftag_raw_getvar(gsffile->psftag,"fade",fade,sizeof(fade)-1);
	if (/*!IgnoreTrackLength &&*/ strlen(fade)) {
		FadeLength=LengthFromString(fade);
		TrackLength+=FadeLength; // comply with PSF standard timing:
	}							 // "length" tag specifies length before fade,
								// "fade" specifies length of fade
	if (TrackLength <= 0) {
	TrackLength=(deflen+deffade)*1000;
	FadeLength=deffade*1000;
	}
	relvolume=0;

	psftag_raw_getvar(gsffile->psftag,"volume",volume,sizeof(volume)-1);
	if(strlen(volume))
		relvolume=VolumeFromString(volume);
	if(relvolume==0) {
		relvolume=defvolume;
	}
	  

    for (int ii=0;ii<MAX_GSFLIB;ii++) {
        if (gsflib[ii]) free(gsflib[ii]);
    }
	return true;

}


bool utilIsGSF(const char * file)
{
  bool IsGSF = false;
  

  if(strlen(file) > 4) {
    const char *p = strrchr(file,'.');

	if(p != NULL) {
	  if(_stricmp(p, ".gsf") == 0)
	    IsGSF = true;
	  if(_stricmp(p, ".minigsf") == 0)
	    IsGSF = true;
	}
  }

  return IsGSF;
}

bool utilIsGBAImage(const char * file)
{
  cpuIsMultiBoot = false;
  if(strlen(file) > 4) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      //if(_stricmp(p, ".gba") == 0)
      //  return true;
      //if(_stricmp(p, ".agb") == 0)
      //  return true;
      //if(_stricmp(p, ".bin") == 0)
      //  return true;
      //if(_stricmp(p, ".elf") == 0)
      //  return true;
	  if(utilIsGSF(file)) {
//		  printf("%s: Is gsf\n", file);
	    return utildecompGSF(file);
	  }
      //if(_stricmp(p, ".mb") == 0) {
      //  cpuIsMultiBoot = true;
      //  return true;
      //}
    }
  }

  return false;
}

//bool utilIsGBImage(const char * file)
//{
//  if(strlen(file) > 4) {
//    char * p = strrchr(file,'.');

//    if(p != NULL) {
//      if(_stricmp(p, ".gb") == 0)
//        return true;
//      if(_stricmp(p, ".gbc") == 0)
//        return true;
//      if(_stricmp(p, ".cgb") == 0)
//        return true;
//      if(_stricmp(p, ".sgb") == 0)
//        return true;      
//    }
//  }

//  return false;
//}

//bool utilIsZipFile(const char *file)
//{
//  if(strlen(file) > 4) {
//    char * p = strrchr(file,'.');

//    if(p != NULL) {
//      if(_stricmp(p, ".zip") == 0)
//        return true;
//    }
//  }

//  return false;  
//}

//#if 0
//bool utilIsRarFile(const char *file)
//{
//  if(strlen(file) > 4) {
//    char * p = strrchr(file,'.');

//    if(p != NULL) {
//      if(_stricmp(p, ".rar") == 0)
//        return true;
//    }
//  }

//  return false;  
//}
//#endif

//bool utilIsGzipFile(const char *file)
//{
//  if(strlen(file) > 3) {
//    char * p = strrchr(file,'.');

//    if(p != NULL) {
//      if(_stricmp(p, ".gz") == 0)
//        return true;
//      if(_stricmp(p, ".z") == 0)
//        return true;
//    }
//  }

//  return false;  
//}

void utilGetBaseName(const char *file, char *buffer)
{
  strcpy(buffer, file);

  //if(utilIsGzipFile(file)) {
  //  char *p = strrchr(buffer, '.');

  //  if(p)
  //    *p = 0;
  //}
}
void utilGetBasePath(const char *file, char *buffer)
{
	strcpy(buffer,file);

#ifdef LINUX
	char *p = strrchr(buffer, '/');
#else
	char *p = strrchr(buffer, '\\');
#endif

	
	if(p)
		*p = 0;
	else {
		strcpy(buffer, "./"); // no path	
	}
}

IMAGE_TYPE utilFindType(const char *file)
{
    char buffer[2048];
  
  //if(utilIsZipFile(file)) {
  //  unzFile unz = unzOpen(file);
    
 //   if(unz == NULL) {
      //systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s"), file);
 //     return IMAGE_UNKNOWN;
 //   }
    
 //   int r = unzGoToFirstFile(unz);
 /*   
    if(r != UNZ_OK) {
      unzClose(unz);
      //systemMessage(MSG_BAD_ZIP_FILE, N_("Bad ZIP file %s"), file);
      return IMAGE_UNKNOWN;
    }
    
    IMAGE_TYPE found = IMAGE_UNKNOWN;
    
    unz_file_info info;
    
    while(true) {
      r = unzGetCurrentFileInfo(unz,
                                &info,
                                buffer,
                                sizeof(buffer),
                                NULL,
                                0,
                                NULL,
                                0);
      
      if(r != UNZ_OK) {
        unzClose(unz);
        //systemMessage(MSG_BAD_ZIP_FILE, N_("Bad ZIP file %s"), file);
        return IMAGE_UNKNOWN;
      }
      
      if(utilIsGBAImage(buffer)) {
        found = IMAGE_GBA;
        break;
      }

      //if(utilIsGBImage(buffer)) {
      //  found = IMAGE_GB;
      //  break;
      //}
        
      r = unzGoToNextFile(unz);
      
      if(r != UNZ_OK)
        break;
    }
    unzClose(unz);
    
    if(found == IMAGE_UNKNOWN) {
      //systemMessage(MSG_NO_IMAGE_ON_ZIP,
      //              N_("No image found on ZIP file %s"), file);
      return found;
    }
    return found;
//#if 0
//  } //else 
	  //if(utilIsRarFile(file)) {
    //IMAGE_TYPE found = IMAGE_UNKNOWN;
    
    //ArchiveList_struct *rarList = NULL;
    //if(urarlib_list((void *)file, (ArchiveList_struct *)&rarList)) {
    //  ArchiveList_struct *p = rarList;

    //  while(p) {
    //    if(utilIsGBAImage(p->item.Name)) {
    //      found = IMAGE_GBA;
    //      break;
    //    }

        //if(utilIsGBImage(p->item.Name)) {
        //  found = IMAGE_GB;
        //  break;
        //}
    //    p = p->next;
    //  }
      
    //  urarlib_freelist(rarList);
    //}
//    return found;
//#endif
  } else {
*/    //if(utilIsGzipFile(file))
    //  utilGetBaseName(file, buffer);
    //else
    strcpy(buffer, file);
    
    if(utilIsGBAImage(buffer))
      return IMAGE_GBA;
    //if(utilIsGBImage(buffer))
    //  return IMAGE_GB;
//  }
  return IMAGE_UNKNOWN;  
}

static int utilGetSize(int size)
{
  int res = 1;
  while(res < size)
    res <<= 1;
  return res;
}

/*static u8 *utilLoadFromZip(const char *file,
                           bool (*accept)(const char *),
                           u8 *data,
                           int &size)
{
  char buffer[2048];
  
  unzFile unz = unzOpen(file);
    
  if(unz == NULL) {
    //systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s"), file);
    return NULL;
  }
  int r = unzGoToFirstFile(unz);
    
  if(r != UNZ_OK) {
    unzClose(unz);
    //systemMessage(MSG_BAD_ZIP_FILE, N_("Bad ZIP file %s"), file);
    return NULL;
  }
    
  bool found = false;
    
  unz_file_info info;
  
  while(true) {
    r = unzGetCurrentFileInfo(unz,
                              &info,
                              buffer,
                              sizeof(buffer),
                              NULL,
                              0,
                              NULL,
                              0);
      
    if(r != UNZ_OK) {
      unzClose(unz);
      //systemMessage(MSG_BAD_ZIP_FILE, N_("Bad ZIP file %s"), file);
      return NULL;
    }

    if(accept(buffer)) {
      found = true;
      break;
    }
    
    r = unzGoToNextFile(unz);
      
    if(r != UNZ_OK)
      break;
  }

  if(!found) {
    unzClose(unz);
    //systemMessage(MSG_NO_IMAGE_ON_ZIP,
    //              N_("No image found on ZIP file %s"), file);
    return NULL;
  }
  
  int fileSize = info.uncompressed_size;
  if(size == 0)
    size = fileSize;
  r = unzOpenCurrentFile(unz);

  if(r != UNZ_OK) {
    unzClose(unz);
    //systemMessage(MSG_ERROR_OPENING_IMAGE, N_("Error opening image %s"), buffer);
    return NULL;
  }

  u8 *image = data;
  
  if(image == NULL) {
    image = (u8 *)malloc(utilGetSize(size));
    if(image == NULL) {
      unzCloseCurrentFile(unz);
      unzClose(unz);
      //systemMessage(MSG_OUT_OF_MEMORY, N_("Failed to allocate memory for %s"),
      //              "data");
      return NULL;
    }
    size = fileSize;
  }
  int read = fileSize <= size ? fileSize : size;
  r = unzReadCurrentFile(unz,
                         image,
                         read);

  unzCloseCurrentFile(unz);
  unzClose(unz);
  
  if(r != (int)read) {
    //systemMessage(MSG_ERROR_READING_IMAGE,
    //              N_("Error reading image %s"), buffer);
    if(data == NULL)
      free(image);
    return NULL;
  }

  size = fileSize;

  return image;
}*/

/*static u8 *utilLoadGzipFile(const char *file,
                            bool (*accept)(const char *),
                            u8 *data,
                            int &size)
{
  FILE *f = fopen(file, "rb");

  if(f == NULL) {
    //systemMessage(MSG_ERROR_OPENING_IMAGE, N_("Error opening image %s"), file);
    return NULL;
  }

  fseek(f, -4, SEEK_END);
  int fileSize = fgetc(f) | (fgetc(f) << 8) | (fgetc(f) << 16) | (fgetc(f) << 24);
  fclose(f);
  if(size == 0)
    size = fileSize;

  gzFile gz = gzopen(file, "rb");

  if(gz == NULL) {
    // should not happen, but who knows?
    //systemMessage(MSG_ERROR_OPENING_IMAGE, N_("Error opening image %s"), file);
    return NULL;
  }

  u8 *image = data;

  if(image == NULL) {
    image = (u8 *)malloc(utilGetSize(size));
    if(image == NULL) {
      //systemMessage(MSG_OUT_OF_MEMORY, N_("Failed to allocate memory for %s"),
      //              "data");
      fclose(f);
      return NULL;
    }
    size = fileSize;
  }
  int read = fileSize <= size ? fileSize : size;
  int r = gzread(gz, image, read);
  gzclose(gz);

  if(r != (int)read) {
    //systemMessage(MSG_ERROR_READING_IMAGE,
    //              N_("Error reading image %s"), file);
    if(data == NULL)
      free(image);
    return NULL;
  }  
  
  size = fileSize;

  return image;  
}*/


/*#if 0
static u8 *utilLoadRarFile(const char *file,
                           bool (*accept)(const char *),
                           u8 *data,
                           int &size)
{
  char buffer[2048];

  ArchiveList_struct *rarList = NULL;
  if(urarlib_list((void *)file, (ArchiveList_struct *)&rarList)) {
    ArchiveList_struct *p = rarList;
    
    bool found = false;
    while(p) {
      if(accept(p->item.Name)) {
        strcpy(buffer, p->item.Name);
        found = true;
        break;
      }
      p = p->next;
    }
    if(found) {
      void *memory = NULL;
      unsigned long lsize = 0;
      size = p->item.UnpSize;
      int r = urarlib_get((void *)&memory, &lsize, buffer, (void *)file, "");
      if(!r) {
        //systemMessage(MSG_ERROR_READING_IMAGE,
        //              N_("Error reading image %s"), buffer);
        urarlib_freelist(rarList);
        return NULL;
      }
      u8 *image = (u8 *)memory;
      if(data != NULL) {
        memcpy(image, data, size);
      }
      urarlib_freelist(rarList);
      return image;
    }
    //systemMessage(MSG_NO_IMAGE_ON_ZIP,
    //              N_("No image found on RAR file %s"), file);
    urarlib_freelist(rarList);
    return NULL;
  }
  // nothing found
  return NULL;
}
#endif*/

u8 *utilLoad(const char *file,
             bool (*accept)(const char *),
             u8 *data,
             int &size)
{
  //if(utilIsZipFile(file)) {
  //  return utilLoadFromZip(file, accept, data, size);
  //}
  //if(utilIsGzipFile(file)) {
  //  return utilLoadGzipFile(file, accept, data, size);
  //}
//#if 0
//  if(utilIsRarFile(file)) {
//    return utilLoadRarFile(file, accept, data, size);
//  }
//#endif
  unsigned int programsize;

  u8 *image = data;

  if(utilIsGSF(file)) {
//	 memcpy(&programsize,uncompbuf+8,sizeof(programsize));
	 copy_int(&programsize, uncompbuf+8);
	 loadedsize = size = programsize;
	 if(image == NULL) {
		image = (u8 *)malloc(utilGetSize(size));
		loadedsize = utilGetSize(size);
		if(image == NULL) {
		//systemMessage(MSG_OUT_OF_MEMORY, N_("Failed to allocate memory for %s"),
		//				"data");
		return NULL;
		}
     }
	 memcpy(image,uncompbuf+12,programsize);
	 free(uncompbuf);
	 return image;
  }
  
  

  FILE *f = fopen(file, "rb");

  if(!f) {
    //systemMessage(MSG_ERROR_OPENING_IMAGE, N_("Error opening image %s"), file);
    return NULL;
  }

  fseek(f,0,SEEK_END);
  int fileSize = ftell(f);
  fseek(f,0,SEEK_SET);
  if(size == 0)
    size = fileSize;

  if(image == NULL) {
    image = (u8 *)malloc(utilGetSize(size));
    if(image == NULL) {
      //systemMessage(MSG_OUT_OF_MEMORY, N_("Failed to allocate memory for %s"),
      //              "data");
      fclose(f);
      return NULL;
    }
    size = fileSize;
  }
  int read = fileSize <= size ? fileSize : size;
  int r = fread(image, 1, read, f);
  fclose(f);

  if(r != (int)read) {
    //systemMessage(MSG_ERROR_READING_IMAGE,
    //              N_("Error reading image %s"), file);
    if(data == NULL)
      free(image);
    return NULL;
  }  

  size = fileSize;
  
  return image;
}

void utilWriteInt(gzFile gzFile, int i)
{
  utilGzWrite(gzFile, &i, sizeof(int));
}

int utilReadInt(gzFile gzFile)
{
  int i = 0;
  utilGzRead(gzFile, &i, sizeof(int));
  return i;
}

void utilReadData(gzFile gzFile, variable_desc* data)
{
  while(data->address) {
    utilGzRead(gzFile, data->address, data->size);
    data++;
  }
}

void utilWriteData(gzFile gzFile, variable_desc *data)
{
  while(data->address) {
    utilGzWrite(gzFile, data->address, data->size);
    data++;
  }
}

gzFile utilGzOpen(const char *file, const char *mode)
{
  utilGzWriteFunc = (int (ZEXPORT *)(gzFile,void * const, unsigned int))gzwrite;
  utilGzReadFunc = gzread;
  utilGzCloseFunc = gzclose;

  return gzopen(file, mode);
}

gzFile utilMemGzOpen(char *memory, int available, char *mode)
{
  utilGzWriteFunc = memgzwrite;
  utilGzReadFunc = memgzread;
  utilGzCloseFunc = memgzclose;

  return memgzopen(memory, available, mode);  
}

int utilGzWrite(gzFile file, const voidp buffer, unsigned int len)
{
  return utilGzWriteFunc(file, buffer, len);
}

int utilGzRead(gzFile file, voidp buffer, unsigned int len)
{
  return utilGzReadFunc(file, buffer, len);
}

int utilGzClose(gzFile file)
{
  return utilGzCloseFunc(file);
}

long utilGzMemTell(gzFile file)
{
  return memtell(file);
}

/*void utilGBAFindSave(const u8 *data, const int size)
{
  u32 *p = (u32 *)data;
  u32 *end = (u32 *)(data + size);
  int saveType = 0;
  int flashSize = 0x10000;
  bool rtcFound = false;

  while(p  < end) {
    u32 d = READ32LE(p);
    
    if(d == 0x52504545) {
      if(memcmp(p, "EEPROM_", 7) == 0) {
        if(saveType == 0)
          saveType = 1;
      }
    } else if (d == 0x4D415253) {
      if(memcmp(p, "SRAM_", 5) == 0) {
        if(saveType == 0)
          saveType = 2;
      }
    } else if (d == 0x53414C46) {
      if(memcmp(p, "FLASH1M_", 8) == 0) {
        if(saveType == 0) {
          saveType = 3;
          flashSize = 0x20000;
        }
      } else if(memcmp(p, "FLASH", 5) == 0) {
        if(saveType == 0) {
          saveType = 3;
          flashSize = 0x10000;
        }
      }
    } else if (d == 0x52494953) {
      if(memcmp(p, "SIIRTC_V", 8) == 0)
        rtcFound = true;
    }
    p++;
  } 
  // if no matches found, then set it to NONE
  if(saveType == 0) {
    saveType = 5;
  }
  //rtcEnable(rtcFound);
  //cpuSaveType = saveType;
  //flashSetSize(flashSize);
}*/

/*void utilUpdateSystemColorMaps()
{
  switch(systemColorDepth) {
  case 16: 
    {
      for(int i = 0; i < 0x10000; i++) {
        systemColorMap16[i] = ((i & 0x1f) << systemRedShift) |
          (((i & 0x3e0) >> 5) << systemGreenShift) |
          (((i & 0x7c00) >> 10) << systemBlueShift);
      }
    }
    break;
  case 24:
  case 32:
    {
      for(int i = 0; i < 0x10000; i++) {
        systemColorMap32[i] = ((i & 0x1f) << systemRedShift) |
          (((i & 0x3e0) >> 5) << systemGreenShift) |
          (((i & 0x7c00) >> 10) << systemBlueShift);
      }
    }
    break;
  }
}*/
