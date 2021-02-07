/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2002  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "PsxCommon.h"
#include "spu/spu.h"
#include "driver.h"
#include <dirent.h>

// LOAD STUFF

int sexypsf_missing_psflib;
char sexypsf_psflib_str[256];

static long TimeToMS(const char *str)
{
		 int x,c=0;
         int acc=0;
	     char s[100];

	     strncpy(s,str,100);
	     s[99]=0;

             for(x=strlen(s);x>=0;x--)
#if 0
			  // Will skip this since i don't want
			  // too much precision Wahahaha!! ...
              if(s[x]=='.' || s[x]==',')
              {
               acc=atoi(s+x+1);
               s[x]=0;
              }
              else
#endif
			  if(s[x]==':')
              {
               if(c==0) acc+=atoi(s+x+1)*10;
               else if(c==1) acc+=atoi(s+x+(x?1:0))*10*60;
               c++;
               s[x]=0;
              }
              else if(x==0)
	      {
               if(c==0) acc+=atoi(s+x)*10;
               else if(c==1) acc+=atoi(s+x)*10*60;
               else if(c==2) acc+=atoi(s+x)*10*60*60;
	      }
             acc*=100;  // To milliseconds.
	    return(acc);
}



char *GetFileWithBase(char *f, char *newfile, const char *pathDir)
{
  static char *ret;
  char *tp1;
	struct dirent **filelist = {0};
	char *directory = ".";
	int fcount = -1;
	int i = 0;
	int found=0;
	
	fcount = scandir(pathDir, &filelist, 0, alphasort);
	if(fcount < 0) {
		perror(directory);
		return NULL;
	}
	for(i = 0; i < fcount; i++)  {
        //printf("compare: %s / %s\n",filelist[i]->d_name,newfile);
		if (!strcasecmp(filelist[i]->d_name,newfile)) {
			found=1;
			strcpy(newfile,filelist[i]->d_name);
		}
		free(filelist[i]);
	}
	free(filelist);
	
	
 #if PSS_STYLE==1
     tp1=((char *)strrchr(f,'/'));
 #else
     tp1=((char *)strrchr(f,'\\'));
  #if PSS_STYLE!=3
  {
     char *tp3;

     tp3=((char *)strrchr(f,'/'));
     if(tp1<tp3) tp1=tp3;
  }
  #endif
 #endif
     if(!tp1)
     {
      ret=malloc(strlen(newfile)+1);
      strcpy(ret,newfile);
     }
     else
     {
      ret=malloc(tp1-f+2+strlen(newfile));	// 1(NULL), 1(/).
	  memcpy(ret,f,tp1-f);
      ret[tp1-f]='/';
      ret[tp1-f+1]=0;
      strcat(ret,newfile);
     }
	
	
	
		
	//some aux files come from case insensitive OS, so parsing of dir is needed...
	
	
    return(ret);
}

static int GetKeyVal(char *buf, char **key, char **val)
{
 char *tmp;

 tmp=buf;

 /* First, convert any weirdo ASCII characters to spaces. */
 while(*tmp++) if(*tmp>0 && *tmp<0x20) *tmp=0x20;

 /* Strip off white space off end of string(which should be the "value"). */
 for(tmp=buf+strlen(buf)-1;tmp>=buf;tmp--)
 {
  if(*tmp != 0x20) break;
  *tmp=0;
 }

 /* Now, search for the first non-whitespace character. */
 while(*buf == 0x20) buf++;

 tmp=buf;
 while((*buf != 0x20) && (*buf != '='))
 {
  if(!*buf) return(0);	/* Null character. */
  buf++;
 }

 /* Allocate memory, copy string, and terminate string. */
 if(!(*key=malloc(buf-tmp+1))) return(0);
 strncpy(*key,tmp,buf-tmp);
 (*key)[(buf-tmp)]=0;

 /* Search for "=" character. */
 while(*buf != '=')
 {
  if(!*buf) return(0);  /* Null character. */
  buf++;
 }

 buf++;	/* Skip over equals character. */

 /* Remove leading whitespace on value. */
 while(*buf == 0x20)
 {
  if(!*buf) return(0);  /* Null character. */
  buf++;
 }

 /* Allocate memory, and copy string over.  Trailing whitespace was eliminated
    earlier.
 */

 if(!(*val=malloc(strlen(buf)+1))) return(0);
 strcpy(*val,buf);

 //puts(*key);
 //puts(*val);

 return(1);
}

static void FreeTags(PSFTAG *tags)
{
 while(tags)
 {
  PSFTAG *tmp=tags->next;

  free(tags->key);
  free(tags->value);
  free(tags);

  tags=tmp;
 }
}

static void AddKV(PSFTAG **tag, char *key, char *val)
{
 PSFTAG *tmp;

 tmp=malloc(sizeof(PSFTAG));

 if( tmp ) {
	memset(tmp,0,sizeof(PSFTAG));

	tmp->key=key;
	tmp->value=val;
	tmp->next=0;
}

 if(!*tag) *tag=tmp;
 else
 {
  PSFTAG *rec;
  rec=*tag;
  while(rec->next) rec=rec->next;
  rec->next=tmp;
 }

}

typedef struct {
	int num;
	char *value;
} LIBNCACHE;

static int ccomp(const void *v1, const void *v2)
{
 const LIBNCACHE *a1,*a2;
 a1=v1; a2=v2;

 return(a1->num - a2->num);
}


#ifdef MEM_SUPPORT
char *memfgets(char *s, int n, char *addr, int size, u32 *offset)
{
	char buf;
	int i, count;

	if( *offset > size ) return NULL;

	for( i = 0; i < n; i++ ) {
		memcpy(&buf, addr+*offset, sizeof(char));
		*offset+=sizeof(char);

		if(buf == '\n') {
			//s[i] = buf;
			s[i] = '\0';
			return s;
		}

		s[i] = buf;
	}
	s[i] = '\0';

	return NULL;
}

static int memread(void *ptr, int len, char *addr, long size, u32 *offset)
{
	int tmp;
	if( (*offset+len) > size ) tmp = size-*offset;
	else tmp = len;
	memcpy(ptr,addr+*offset,tmp);
	*offset+=tmp;
	return tmp;
}

static int memseek(int len, long size, u32 *offset)
{
	int tmp;
	if( (*offset+len) > size ) tmp = size-*offset;
	else tmp = len;
	return tmp;
}

static struct XSFLibN *__psflibs = NULL;
void sexy_setpsflibs(struct XSFLibN *libs)
{
	__psflibs = libs;
}

// LoadPSF for Memory
static PSFINFO *memLoadPSF(char *addr, long size, int level, int type) // Type==1 for just info load.
{
    u8 *in,*out=0;
	u8 head[4];

    u32 reserved;
    u32 complen;
    u32 crc32;
    uLongf outlen;
	PSFINFO *psfi=NULL;
	PSFINFO *tmpi=NULL;
	u32 poffset, plength;
	u32 offset = 0;

	if( addr == NULL || size == 0 ) return(0);

	memread((void*)(head), 4, addr, size, &offset);
	if(memcmp(head,"PSF\x01",4)) return(0);

	psfi=malloc(sizeof(PSFINFO));
	memset(psfi,0,sizeof(PSFINFO));
    psfi->stop=~0;
    psfi->fade=0;

	memread((void*)(&reserved), 4, addr, size, &offset);
	memread((void*)(&complen), 4, addr, size, &offset);
	complen=BFLIP32(complen);

	memread((void*)(&crc32), 4, addr, size, &offset);
	crc32=BFLIP32(crc32);

	memseek(reserved, size, &offset);

	if(type) {
		memseek(complen, size, &offset);
	}
    else {
        in=malloc(complen);
        out=malloc(1024*1024*2+0x800);
		if( !in || !out ) { free(psfi); return (0); }
		memread((void*)(in), complen, addr, size, &offset);

        outlen=1024*1024*2;
        uncompress(out,&outlen,in,complen);
        free(in);

        psxRegs->pc = out[0x10] | out[0x11]<<8 | out[0x12]<<16 | out[0x13]<<24;
		psxRegs->GPR.n.gp = out[0x14] | out[0x15]<<8 | out[0x16]<<16 | out[0x17]<<24;
		psxRegs->GPR.n.sp = out[0x30] | out[0x31]<<8 | out[0x32]<<16 | out[0x33]<<24;

        if (psxRegs->GPR.n.sp == 0) psxRegs->GPR.n.sp = 0x801fff00;

#ifdef DEBUG
		fprintf(stderr, "%d Level: PC %x GP %x SP %x\n",
			level, psxRegs->pc, psxRegs->GPR.n.gp, psxRegs->GPR.n.sp);
#endif

 		if(level) {
			poffset = out[0x18] | out[0x19]<<8 | out[0x1a]<<16 | out[0x1b]<<24;
			poffset &= 0x3fffffff;	// kill any MIPS cache segment indicators
			plength = out[0x1c] | out[0x1d]<<8 | out[0x1e]<<16 | out[0x1f]<<24;

#ifdef DEBUG
			fprintf(stderr, "%d Level: offset %x plength: %d\n", level, poffset, plength);
#endif

			LoadPSXMem(poffset,plength,out+0x800);
			free(out);

			// I don't really know how does this
			// work, but apparently it breaks the
			// loop that makes ff6 and ct to take
			// several seconds to start ...
			if( crc32 == 0xEFDE8EEE || crc32 == 0x545D9F65 )
				psxMemWrite32(0x5A66C, 0);

			// Same as above for Popolocrois and Einhänder
			if( crc32 == 0xF535726 )
				psxMemWrite32(0x5A990, 0);
		}
	}

    {
		u8 tagdata[5];
		if( memread((void*)(tagdata), 5, addr, size, &offset) == 5 ) {
			if(!memcmp(tagdata,"[TAG]",5)) {
				char linebuf[1024];

				while( memfgets(linebuf, 1024, addr, size, &offset)>0 ) {
					int x;
					char *key=0,*value=0;

					if(!GetKeyVal(linebuf,&key,&value)) {
						if(key) free(key);
						if(value) free(value);
						continue;
					}

					AddKV(&psfi->tags,key,value);

					if(!level) {
       					static char *yoinks[8]={"title","artist","game","year","genre",
                                  "copyright","psfby","comment"};
						char **yoinks2[8]={&psfi->title,&psfi->artist,&psfi->game,&psfi->year,&psfi->genre,
                                    &psfi->copyright,&psfi->psfby,&psfi->comment};
						for(x=0;x<8;x++)
							if(!strcasecmp(key,yoinks[x]))
								*yoinks2[x]=value;
						if(!strcasecmp(key,"length"))
							psfi->stop=TimeToMS(value);
						else if(!strcasecmp(key,"fade"))
							psfi->fade=TimeToMS(value);
					}

					if(!strcasecmp(key,"_lib") && !type && __psflibs) {

						/* Search file name "value" from the __psflibs array */
						int n, found = 0;
						for(n=0;n<__psflibs->count;n++)
							if( !strcasecmp(value, __psflibs->libn[n].name) ) {
								found = 1;
								break;
							}

						if(!found || !(tmpi=memLoadPSF(__psflibs->libn[n].addr, __psflibs->libn[n].size, level+1,0))) {
							free(key);
							free(value);
							if(!level) free(out);

							FreeTags(psfi->tags);
							free(psfi);
							return(0);
						}

						FreeTags(tmpi->tags);
						free(tmpi);
					}
				}
			}
		}
	}

	/* Now, if we're at level 0(main PSF), load the main executable, and any libN stuff*/
    if(!level && !type) {
    	poffset = out[0x18] | out[0x19]<<8 | out[0x1a]<<16 | out[0x1b]<<24;
		poffset &= 0x3fffffff;	// kill any MIPS cache segment indicators
		plength = out[0x1c] | out[0x1d]<<8 | out[0x1e]<<16 | out[0x1f]<<24;

		// TODO - investigate: should i just make
		//        plength = outlen-0x800?

		// Philosoma has an illegal "plength".  *sigh*
		if (plength > (outlen-0x800))
		{
			plength = outlen-0x800;
		}

		if( psfi->game ) {
			// Suikoden Tenmei has an illegal "plength". *sigh sigh*
			if( !strncmp(psfi->game, "Suikoden: Tenmei no Chikai", 26) ) {
				plength = outlen-0x800;
			}

			// Einhänder has an illegal "plength". *sigh sigh sigh*
			if( !strncmp(psfi->game, "Einhänder", 9) ) {
				plength = outlen-0x800;
			}
		}

#ifdef DEBUG
		fprintf(stderr, "%d Level: offset %x plength: %d\n", level, poffset, plength);
#endif

		LoadPSXMem(poffset,plength,out+0x800);
		free(out);
    }

	if(!type && __psflibs)	/* Load libN */ {
		LIBNCACHE *cache;
		PSFTAG *tag;
		unsigned int libncount=0;
		unsigned int cur=0;

		tag=psfi->tags;
		while(tag) {
			if(!strncasecmp(tag->key,"_lib",4) && tag->key[4])
			libncount++;
			tag=tag->next;
		}

		if(libncount) {
			cache=malloc(sizeof(LIBNCACHE)*libncount);

			tag=psfi->tags;
			while(tag) {
				if(!strncasecmp(tag->key,"_lib",4) && tag->key[4]) {
					cache[cur].num=atoi(&tag->key[4]);
					cache[cur].value=tag->value;
					cur++;
				}
				tag=tag->next;
			}
			qsort(cache, libncount, sizeof(LIBNCACHE), ccomp);
			for(cur=0;cur<libncount;cur++) {
				u32 ba[3];

 				if(cache[cur].num < 2) continue;

				ba[0]=psxRegs->pc;
				ba[1]=psxRegs->GPR.n.gp;
				ba[2]=psxRegs->GPR.n.sp;

				/* Search file name "value" from the __psflibs array */
				int n, found = 0;
				for(n=0;n<__psflibs->count;n++)
					if( !strcasecmp(cache[cur].value, __psflibs->libn[n].name) ) {
						found = 1;
						break;
					}

				if(!found || !(tmpi=memLoadPSF(__psflibs->libn[n].addr,__psflibs->libn[n].size,level+1,0))) {
					//free(key);
					//free(value);
					//free(tmpfn);
					//fclose(fp);
					//return(0);
				}
				FreeTags(tmpi->tags);
				free(tmpi);

				psxRegs->pc=ba[0];
       			psxRegs->GPR.n.gp=ba[1];
				psxRegs->GPR.n.sp=ba[2];
			}
			free(cache);
		}	// if(libncount)

	}	// if(!type)

	return(psfi);
}
#endif

static PSFINFO *LoadPSF(char *path, int level, int type,const char *pathDir) // Type==1 for just info load.
{
    FILE *fp;
    u8 *in,*out=0;
	u8 head[4];
    u32 reserved;
    u32 complen;
    u32 crc32;
    uLongf outlen;
	PSFINFO *psfi=NULL;
	PSFINFO *tmpi=NULL;

    u32 offset, plength;
	
	sexypsf_missing_psflib=0;

#ifdef DEBUG
	fprintf(stderr, "Searching (%s)\n", path);
#endif

	fp=fopen(path,"rb");
    if(!fp) return 0;

	fread(head,1,4,fp);
	if(memcmp(head,"PSF\x01",4)) return(0);

	psfi=malloc(sizeof(PSFINFO));
	if( psfi == NULL ) return(0);
	memset(psfi,0,sizeof(PSFINFO));
    psfi->stop=~0;
    psfi->fade=0;

    fread(&reserved,1,4,fp);
    fread(&complen,1,4,fp);
	complen=BFLIP32(complen);

    fread(&crc32,1,4,fp);
	crc32=BFLIP32(crc32);

#ifdef DEBUG
	fprintf(stderr, "CRC32 = 0x%x\n", crc32);
#endif

    fseek(fp,reserved,SEEK_CUR);

    if(type)
		fseek(fp,complen,SEEK_CUR);
    else {
        in=malloc(complen);
		if( !in ) return(0);
        out=malloc(1024*1024*2+0x800);
		if( !out ) {
			free(in);
			return(0);
		}
        fread(in,1,complen,fp);
        outlen=1024*1024*2;
        uncompress(out,&outlen,in,complen);
        free(in);

		psxRegs->pc = out[0x10] | out[0x11]<<8 | out[0x12]<<16 | out[0x13]<<24;
		psxRegs->GPR.n.gp = out[0x14] | out[0x15]<<8 | out[0x16]<<16 | out[0x17]<<24;
		psxRegs->GPR.n.sp = out[0x30] | out[0x31]<<8 | out[0x32]<<16 | out[0x33]<<24;

        if (psxRegs->GPR.n.sp == 0) psxRegs->GPR.n.sp = 0x801fff00;

#ifdef DEBUG
		fprintf(stderr, "%d Level: PC %x GP %x SP %x\n",
			level, psxRegs->pc, psxRegs->GPR.n.gp, psxRegs->GPR.n.sp);
#endif

 		if(level) {
			offset = out[0x18] | out[0x19]<<8 | out[0x1a]<<16 | out[0x1b]<<24;
			offset &= 0x3fffffff;	// kill any MIPS cache segment indicators
			plength = out[0x1c] | out[0x1d]<<8 | out[0x1e]<<16 | out[0x1f]<<24;

#ifdef DEBUG
			fprintf(stderr, "%d Level: offset %x plength: %d [%d]\n", level, offset, plength, plength);
#endif

			LoadPSXMem(offset,plength,(char*)out+0x800);
			free(out);

			// I don't really know how does this
			// work, but apparently it breaks the
			// loop that makes ff6 and ct to take
			// several seconds to start ...
			if( crc32 == 0xEFDE8EEE || crc32 == 0x545D9F65 )
				psxMemWrite32(0x5A66C, 0);

			// Same as above for Popolocrois
			if( crc32 == 0xF535726 )
				psxMemWrite32(0x5A990, 0);
		}
	}

    {
		u8 tagdata[5];
        if(fread(tagdata,1,5,fp)==5) {
			if(!memcmp(tagdata,"[TAG]",5)) {
				char linebuf[1024];
				while(fgets(linebuf,1024,fp)>0) {
					int x;
					char *key=0,*value=0;

					if(!GetKeyVal(linebuf,&key,&value)) {
						if(key) free(key);
						if(value) free(value);
						continue;
					}

					AddKV(&psfi->tags,key,value);

					if(!level) {
       					static char *yoinks[8]={"title","artist","game","year","genre",
                                  "copyright","psfby","comment"};
						char **yoinks2[8]={&psfi->title,&psfi->artist,&psfi->game,&psfi->year,&psfi->genre,
                                    &psfi->copyright,&psfi->psfby,&psfi->comment};
						for(x=0;x<8;x++)
							if(!strcasecmp(key,yoinks[x]))
								*yoinks2[x]=value;
						if(!strcasecmp(key,"length"))
							psfi->stop=TimeToMS(value);
						else if(!strcasecmp(key,"fade"))
							psfi->fade=TimeToMS(value);
					}

					if(!strcasecmp(key,"_lib") && !type) {
						char *tmpfn;
						/* Load file name "value" from the directory specified in
						   the full path(directory + file name) "path"
						*/
                        //printf("yo %s\nya %s\nyu%s\n",path,value,pathDir);
						tmpfn=GetFileWithBase(path,value,pathDir);
                        //printf("ru: %s\n",tmpfn);
						if(!(tmpi=LoadPSF(tmpfn,level+1,0,pathDir))) {
							//free(key);
							//free(value);
							sexypsf_missing_psflib=1;
							strcpy(sexypsf_psflib_str,value);
							free(tmpfn);
 							if(!level) free(out);
							fclose(fp);
							FreeTags(psfi->tags);
							free(psfi);
							return(0);
						}
						FreeTags(tmpi->tags);
						free(tmpi);
						free(tmpfn);
					}
				}
			}
		}
	}

    fclose(fp);

	/* Now, if we're at level 0(main PSF), load the main executable, and any libN stuff */
    if(!level && !type) {
		offset = out[0x18] | out[0x19]<<8 | out[0x1a]<<16 | out[0x1b]<<24;
		offset &= 0x3fffffff;	// kill any MIPS cache segment indicators
		plength = out[0x1c] | out[0x1d]<<8 | out[0x1e]<<16 | out[0x1f]<<24;

		// TODO - investigate: should i just make
		//        plength = outlen-0x800?

		// Philosoma has an illegal "plength".  *sigh*
		if (plength > (outlen-0x800))
		{
			plength = outlen-0x800;
		}

		if( psfi->game ) {
			// Suikoden Tenmei has an illegal "plength". *sigh sigh*
			if( !strncmp(psfi->game, "Suikoden: Tenmei no Chikai", 26) ) {
				plength = outlen-0x800;
			}

			// Einhänder has an illegal "plength". *sigh sigh sigh*
			if( !strncmp(psfi->game, "Einhänder", 9) ) {
				plength = outlen-0x800;
			}
		}

#ifdef DEBUG
		fprintf(stderr, "%d Level: offset %x plength: %d [%d]\n", level, offset, plength, outlen-2048);
#endif

		LoadPSXMem(offset,plength,(char*)out+0x800);
		free(out);
    }

	if(!type)	/* Load libN */{
		LIBNCACHE *cache;
		PSFTAG *tag;
		unsigned int libncount=0;
		unsigned int cur=0;

		tag=psfi->tags;
		while(tag) {
			if(!strncasecmp(tag->key,"_lib",4) && tag->key[4])
			libncount++;
			tag=tag->next;
		}

		if(libncount) {
			cache=malloc(sizeof(LIBNCACHE)*libncount);

			tag=psfi->tags;
			while(tag) {
				if(!strncasecmp(tag->key,"_lib",4) && tag->key[4]) {
					cache[cur].num=atoi(&tag->key[4]);
					cache[cur].value=tag->value;
					cur++;
				}
				tag=tag->next;
			}
			qsort(cache, libncount, sizeof(LIBNCACHE), ccomp);
			for(cur=0;cur<libncount;cur++) {
				u32 ba[3];
				char *tmpfn;

 				if(cache[cur].num < 2) continue;

				ba[0]=psxRegs->pc;
				ba[1]=psxRegs->GPR.n.gp;
				ba[2]=psxRegs->GPR.n.sp;

				/* Load file name "value" from the directory specified in
				the full path(directory + file name) "path"
				*/
				tmpfn=GetFileWithBase(path,cache[cur].value,pathDir);
				if(!(tmpi=LoadPSF(tmpfn,level+1,0,pathDir))) {
					//free(key);
					//free(value);
					//free(tmpfn);
                    sexypsf_missing_psflib=1;
                    strcpy(sexypsf_psflib_str,cache[cur].value);
                    if (tmpfn) free(tmpfn);
                    if (cache) free(cache);
                    return(0);
				} else {
                    FreeTags(tmpi->tags);
                    free(tmpi);
                }
                free(tmpfn);


				psxRegs->pc=ba[0];
       			psxRegs->GPR.n.gp=ba[1];
				psxRegs->GPR.n.sp=ba[2];
			}
			free(cache);
		}	// if(libncount)

	}	// if(!type)

	return(psfi);
}

void sexy_freepsfinfo(PSFINFO *info)
{
 FreeTags(info->tags);
 free(info);
}

PSFINFO *sexy_getpsfinfo(char *path,const char*pathDir)
{
	PSFINFO *ret;
        if(!(ret=LoadPSF(path,0,1,pathDir))) return(0);
        if(ret->stop==~0) ret->fade=0;
        ret->length=ret->stop+ret->fade;
        return(ret);
}

#ifdef MEM_SUPPORT
PSFINFO *sexy_memload(char *addr, int size)
{
	PSFINFO *ret;

        psxInit();
        psxReset();

        sexySPUinit();
        sexySPUopen();

	if(!(ret=memLoadPSF(addr,size,0,0)))
	{
	 psxShutdown();
	 return(0);
	}

	// Taken from aosdk's eng_psf.c file ...
	// patch illegal Chocobo Dungeon 2 code - CaitSith2 put a jump in the delay slot from a BNE
	// and rely on Highly Experimental's buggy-ass CPU to rescue them.  Verified on real hardware
	// that the initial code is wrong.
	if (ret->game) {
		if (!strcmp(ret->game, "Chocobo Dungeon 2")) {
			if (psxMu32(0xbc090) == (0x0802f040))
			{
			 psxMemWrite32(0xbc090, 0);
			 psxMemWrite32(0xbc094, 0x0802f040);
			 psxMemWrite32(0xbc098, 0);
			}
		}
	}
    
	if(ret->stop==~0) ret->fade=0; // Infinity+anything is still infinity...or is it?
	setlength(ret->stop,ret->fade);
	ret->length=ret->stop+ret->fade;

        return(ret);
}
#endif

static char save_path[1024];
static char save_pathDir[1024];

PSFINFO *sexy_load(char *path,const char *pathDir,int loop_infinite)
{
	PSFINFO *ret;

        if( psxInit() < 0 ) return(0);
        psxReset();

        sexySPUinit();
        sexySPUopen();

	if(!(ret=LoadPSF(path,0,0,pathDir)))
	{
	 psxShutdown();
	 return(0);
	}
	strcpy(save_path,path);
	strcpy(save_pathDir,pathDir);	

	// Taken from aosdk's eng_psf.c file ...
	// patch illegal Chocobo Dungeon 2 code - CaitSith2 put a jump in the delay slot from a BNE
	// and rely on Highly Experimental's buggy-ass CPU to rescue them.  Verified on real hardware
	// that the initial code is wrong.
	if (ret->game) {
		if (!strcmp(ret->game, "Chocobo Dungeon 2")) {
			if (psxMu32(0xbc090) == (0x0802f040))
			{
			 psxMemWrite32(0xbc090, 0);
			 psxMemWrite32(0xbc094, 0x0802f040);
			 psxMemWrite32(0xbc098, 0);
			}
		}
	}
	
	if(ret->stop==~0) {
		ret->fade=10000; // Infinity ? limit to 3 minutes
		ret->stop=170000;
	}
    
    if (loop_infinite) {
		ret->stop=~0;
	}

	sexysetlength(ret->stop,ret->fade);
	ret->length=ret->stop+ret->fade;

	return(ret);
}

int sexy_reload() {
	PSFINFO *ret;
	
	if( psxInit() < 0 ) return -1;
	psxReset();
	
	sexySPUinit();
	sexySPUopen();
	
	if(!(ret=LoadPSF(save_path,0,0,save_pathDir)))
	{
		psxShutdown();
		return -2;
	}
	
	// Taken from aosdk's eng_psf.c file ...
	// patch illegal Chocobo Dungeon 2 code - CaitSith2 put a jump in the delay slot from a BNE
	// and rely on Highly Experimental's buggy-ass CPU to rescue them.  Verified on real hardware
	// that the initial code is wrong.
	if (ret->game) {
		if (!strcmp(ret->game, "Chocobo Dungeon 2")) {
			if (psxMu32(0xbc090) == (0x0802f040))
			{
				psxMemWrite32(0xbc090, 0);
				psxMemWrite32(0xbc094, 0x0802f040);
				psxMemWrite32(0xbc098, 0);
			}
		}
	}
	
	if(ret->stop==~0) {
		ret->fade=10000; // Infinity ? limit to 3 minutes
		ret->stop=170000;
	}
	sexysetlength(ret->stop,ret->fade);
	ret->length=ret->stop+ret->fade;
	
	sexy_freepsfinfo(ret);
	
	return 0;
}

void sexy_execute(void)
{
 psxCpu->Execute();
}

#ifdef BENCHMARK
extern void intExecute_slice();
void sexy_execute_slice(void)
{
 psxCpu->ExecuteSlice();
}
#endif
