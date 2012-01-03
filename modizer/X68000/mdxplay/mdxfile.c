/*
  MDXplay : MDX file parser

  Made by Daisuke Nagano <breeze.nagano@nifty.ne.jp>
  Jan.13.1999

  reference : mdxform.doc  ( KOUNO Takeshi )
            : MXDRVWIN.pas ( monlight@tkb.att.ne.jp )
 */

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>

# include <string.h>

# include <unistd.h>
# include <sys/types.h>

# include <iconv.h>
# include <locale.h>

#include "version.h"
#include "mdx.h"
#include "mdxopl3.h"
#include "gettext_wrapper.h"

/* ------------------------------------------------------------------ */
/* local functions */

static void error_file( void );

/* ------------------------------------------------------------------ */

static void*
__alloc_mdxwork(void)
{
  MDX_DATA* mdx = NULL;
  mdx = (MDX_DATA *)malloc(sizeof(MDX_DATA));
  if (mdx) {
    memset((void *)mdx, 0, sizeof(MDX_DATA));
  }
  return mdx;
}

static int
__load_file(MDX_DATA* mdx, char* fnam)
{
  FILE* fp = NULL;
  unsigned char* buf = NULL;
  int len = 0;
  int result = 0;

  fp = fopen( fnam, "r" );
  if ( fp == NULL ) {
    return FLAG_FALSE;
  }

  fseek(fp, 0, SEEK_END);
  len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  buf = (unsigned char *)malloc(sizeof(unsigned char)*len);
  if (!buf) {
    fclose(fp);
    return FLAG_FALSE;
  }

  result = fread( buf, 1, len, fp );
  fclose(fp);

  if (result!=len) {
    free(buf);
    return FLAG_FALSE;
  }

  mdx->length = len;
  mdx->data = buf;

  return FLAG_TRUE;
}

MDX_DATA *mdx_open_mdx( char *name ) {

  int i,j;
  int ptr;
  unsigned char *buf;
  MDX_DATA *mdx;

  /* allocate work area */

  mdx = __alloc_mdxwork();
  if ( mdx == NULL ) return NULL;

  /* data read */
  if (!__load_file(mdx, name)) {
    goto error_end;
  }

  /* title parsing */

  for ( i=0 ; i<MDX_MAX_TITLE_LENGTH ; i++ ) {
    mdx->data_title[i] = '\0';
  }
  i=0;
  ptr=0;
  buf = mdx->data;
  mdx->data_title[i]=0;
  if (mdx->length<3) {
    goto error_end;
  }
  while(1) {
    if ( buf[ptr+0] == 0x0d &&
	 buf[ptr+1] == 0x0a &&
	 buf[ptr+2] == 0x1a ) break;

    mdx->data_title[i++]=buf[ptr++];  /* warning! this text is SJIS */
    if ( i>=MDX_MAX_TITLE_LENGTH ) i--;
    if ( ptr > mdx->length ) error_file();
  }
  mdx->data_title[i++]=0;


  /* pdx name */

  ptr+=3;
  for ( i=0 ; i<MDX_MAX_PDX_FILENAME_LENGTH ; i++ ) {
    mdx->pdx_name[i]='\0';
  }
  i=0;
  j=0;
  mdx->haspdx=FLAG_FALSE;
  while(1) {
    if ( buf[ptr] == 0x00 ) break;

    mdx->haspdx=FLAG_TRUE;
    mdx->pdx_name[i++] = buf[ptr++];
    if ( strcasecmp( ".pdx", (char *)(buf+ptr-1) )==0 ) j=1;
    if ( i>= MDX_MAX_PDX_FILENAME_LENGTH ) i--;
    if ( ptr > mdx->length ) error_file();
  }
  if ( mdx->haspdx==FLAG_TRUE && j==0 ) {
    mdx->pdx_name[i+0] = '.';
    mdx->pdx_name[i+1] = 'p';
    mdx->pdx_name[i+2] = 'd';
    mdx->pdx_name[i+3] = 'x';
  }

  /* get voice data offset */

  ptr++;
  mdx->base_pointer = ptr;
  mdx->voice_data_offset =
    (unsigned int)buf[ptr+0]*256 +
    (unsigned int)buf[ptr+1] + mdx->base_pointer;

  if ( mdx->voice_data_offset > mdx->length ) error_file();

   /* get MML data offset */

  mdx->mml_data_offset[0] =
    (unsigned int)buf[ptr+2+0] * 256 +
    (unsigned int)buf[ptr+2+1] + mdx->base_pointer;
  if ( mdx->mml_data_offset[0] > mdx->length ) error_file();

  if ( buf[mdx->mml_data_offset[0]] == MDX_SET_PCM8_MODE ) {
    mdx->ispcm8mode = 1;
    mdx->tracks = 16;
  } else {
    mdx->ispcm8mode = 0;
    mdx->tracks = 9;
  }

  for ( i=0 ; i<mdx->tracks ; i++ ) {
    mdx->mml_data_offset[i] =
      (unsigned int)buf[ptr+i*2+2+0] * 256 +
      (unsigned int)buf[ptr+i*2+2+1] + mdx->base_pointer;
    if ( mdx->mml_data_offset[i] > mdx->length ) error_file();
  }


  /* init. configuration */

  mdx->is_use_pcm8 = FLAG_TRUE;
  mdx->is_use_fm   = FLAG_TRUE;
  mdx->is_use_opl3 = FLAG_TRUE;

  i = strlen(MDX_VERSION_TEXT1);
  if ( i > MDX_VERSION_TEXT_SIZE ) i=MDX_VERSION_TEXT_SIZE;
  strncpy( (char *)mdx->version_1, MDX_VERSION_TEXT1, i );
  i = strlen(MDX_VERSION_TEXT2);
  if ( i > MDX_VERSION_TEXT_SIZE ) i=MDX_VERSION_TEXT_SIZE;
  strncpy( (char *)mdx->version_2, MDX_VERSION_TEXT2, i );

  return mdx;

error_end:
  if (mdx) {
    if (mdx->data) {
      free(mdx->data);
      mdx->data = NULL;
    }

    free(mdx);
  }
  return NULL;
}

int mdx_close_mdx ( MDX_DATA *mdx ) {

  if ( mdx == NULL ) return 1;

  if ( mdx->data != NULL ) free(mdx->data);
  free(mdx);

  return 0;
}


# define dump_voices(a,b) (1)

int mdx_get_voice_parameter( MDX_DATA *mdx ) {

  int i;
  int ptr;
  int num;
  unsigned char *buf;

  ptr = mdx->voice_data_offset;
  buf = mdx->data;

  while ( ptr < mdx->length ) {

    if ( mdx->length-ptr < 27 ) break;

    num = buf[ptr++];
    if ( num >= MDX_MAX_VOICE_NUMBER ) return 1;

    mdx->voice[num].v0 = buf[ptr];

    mdx->voice[num].con = buf[ptr  ]&0x07;
    mdx->voice[num].fl  = (buf[ptr++] >> 3)&0x07;
    mdx->voice[num].slot_mask = buf[ptr++];

    /* DT1 & MUL */
    for ( i=0 ; i<4 ; i++ ) {
      mdx->voice[num].v1[i] = buf[ptr];

      mdx->voice[num].mul[i] = buf[ptr] & 0x0f;
      mdx->voice[num].dt1[i] = (buf[ptr] >> 4)&0x07;
      ptr++;
    }
    /* TL */
    for ( i=0 ; i<4 ; i++ ) {
      mdx->voice[num].v2[i] = buf[ptr];

      mdx->voice[num].tl[i] = buf[ptr];
      ptr++;
    }
    /* KS & AR */
    for ( i=0 ; i<4 ; i++ ) {
      mdx->voice[num].v3[i] = buf[ptr];

      mdx->voice[num].ar[i] = buf[ptr] & 0x1f;
      mdx->voice[num].ks[i] = (buf[ptr] >> 6)&0x03;
      ptr++;
    }
    /* AME & D1R */
    for ( i=0 ; i<4 ; i++ ) {
      mdx->voice[num].v4[i] = buf[ptr];

      mdx->voice[num].d1r[i] = buf[ptr] & 0x1f;
      mdx->voice[num].ame[i] = (buf[ptr] >> 7)&0x01;
      ptr++;
    }
    /* DT2 & D2R */
    for ( i=0 ; i<4 ; i++ ) {
      mdx->voice[num].v5[i] = buf[ptr];

      mdx->voice[num].d2r[i] = buf[ptr] & 0x1f;
      mdx->voice[num].dt2[i] = (buf[ptr] >> 6)&0x03;
      ptr++;
    }
    /* SL & RR */
    for ( i=0 ; i<4 ; i++ ) {
      mdx->voice[num].v6[i] = buf[ptr];

      mdx->voice[num].rr[i] = buf[ptr] & 0x0f;
      mdx->voice[num].sl[i] = (buf[ptr] >> 4)&0x0f;
      ptr++;
    }

    if ( mdx->dump_voice == FLAG_TRUE ) {
      dump_voices(mdx, num);
    }
  }

  return 0;
}


static void error_file( void ) {
}


/* ------------------------------------------------------------------ */

char *mdx_make_sjis_to_syscharset(char* str) {
#if 0	
	iconv_t fd = 0;
	size_t len = 0;
	char* result = NULL;
	char* rr = NULL;
	size_t remain=0, oremain=0;
	char* cur = NULL;
	const char* loc = (const char *)"UTF-8";
	unsigned char src[3];
	unsigned char dst[7];
	int ptr = 0;
	size_t sl=0;
	size_t dl=0;
	
	cur = setlocale(LC_CTYPE, "");
	if (!cur) {
		goto error_end;
	}
	if (strstr(cur, "eucJP")) {
		loc = (const char *)"EUC-JP";
	}
	
	fd = iconv_open(loc, "SHIFT-JIS");
	if (fd<0) {
		goto error_end;
	}
	
	/* enough for store utf8 */
	remain = strlen(str);
	if (remain==0) {
		goto error_end;
	}
	oremain = remain*6;
	result = (char *)malloc(sizeof(char)*oremain);
	if (!result) {
		goto error_end;
	}
	memset((void *)result, 0, oremain);
	rr = result;
	
	ptr=0;
	/* to omit X68k specific chars, process charconv one by one */
	do {
		char *sp, *dp;
		unsigned char c = 0;
		
		memset((void *)src, 0, sizeof(src));
		memset((void *)dst, 0, sizeof(dst));
		oremain = 0;
		
		c = src[0] = (unsigned char)str[ptr++];
		sl=1;
		if (!c) break;
		if (c==0x80 || (c>=0x81 && c<=0x84) || (c>=0x88 && c<=0x9f) ||
			(c>=0xe0 && c<=0xea) || (c>=0xf0 && c<=0xf3) || c==0xf6) {
			src[1] = (unsigned char)str[ptr++];
			if (!src[1]) {
				strcat(result, ".");
				break;
			}
			sl++;
		}
		
		sp = (char *)src;
		dp = (char *)dst;
		
		dl = 7;
		len = iconv(fd, (char **)&sp, &sl, (char **)&dp, &dl);
		if (len==(size_t)(-1)) {
			strcat(result, ".");
		} else {
			strcat(result, (char *)dst);
		}
	} while (1);
	
	iconv_close(fd);
	return result;
	
error_end:
	if (result) {
		free(result);
	}
	if (fd>=0) {
		iconv_close(fd);
	}
#endif	
	return strdup(str);
}

unsigned char *mdx_get_title( MDX_DATA *mdx ) {
	unsigned char *message;
//	message = (unsigned char *)mdx_make_sjis_to_syscharset(mdx->data_title);	
	message = (unsigned char *)strdup(mdx->data_title);
	return message;
}
