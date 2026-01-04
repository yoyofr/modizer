#include <stdlib.h>
#include <memory.h>
#include "zlib.h"

typedef unsigned char Uint8;
typedef unsigned Uint;
typedef unsigned Uint32;

#include "memzip.h"

enum {
	ZIPMETHOD_STORED		= 0,
	ZIPMETHOD_SHRUNK		= 1,
	ZIPMETHOD_REDUCED1		= 2,
	ZIPMETHOD_REDUCED2		= 3,
	ZIPMETHOD_REDUCED3		= 4,
	ZIPMETHOD_REDUCED4		= 5,
	ZIPMETHOD_IMPLODED		= 6,
	ZIPMETHOD_TOKENIZED		= 7,
	ZIPMETHOD_DEFLATED		= 8,
	ZIPMETHOD_ENHDEFLATED	= 9,
	ZIPMETHOD_DCLIMPLODED	= 10,
	ZIPMETHOD_MAX,
	ZIPMETHOD_UNKONOWN = -1
};

static Uint32 GetDwordLE(Uint8 *p)
{
	return p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);
}

static Uint GetWordLE(Uint8 *p)
{
	return p[0] + (p[1] << 8);
}

static Uint8 *GetLocalHeader(ZIP_LOCAL_HEADER *pzlh, Uint8 *p)
{
	/* MAGIC */
	if (GetDwordLE(p) != 0x04034b50) return 0;
	/* ZIP LOCAL HEADER */
	pzlh->version	= GetWordLE( p + 0x04);
	pzlh->flag		= GetWordLE( p + 0x06);
	pzlh->method	= GetWordLE( p + 0x08);
	pzlh->time		= GetDwordLE(p + 0x0a);
	pzlh->crc32		= GetDwordLE(p + 0x0e);
	pzlh->size_def	= GetDwordLE(p + 0x12);
	pzlh->size_inf	= GetDwordLE(p + 0x16);
	pzlh->size_fn	= GetWordLE( p + 0x1a);
	pzlh->size_ext	= GetWordLE( p + 0x1c);
	p += 0x1e;
	memcpy(pzlh->fname, p, pzlh->size_fn);
	pzlh->fname[pzlh->size_fn] = '\0';
	return p + pzlh->size_fn + pzlh->size_ext;
}

struct UNZIPMEM_T
{
	unsigned len;
	Uint8 *top;
	Uint8 *cur;
};

HUNZM unzmOpen(void *p, unsigned len)
{
	HUNZM hunzm;
	if (len < 0x1e || GetDwordLE(p) != 0x04034b50) return 0;
	hunzm = malloc(sizeof(struct UNZIPMEM_T));
	if (!hunzm) return 0;
	hunzm->len = len;
	hunzm->cur = hunzm->top = p;
	return hunzm;
}

void unzmGoToFirstFile(HUNZM hunzm)
{
	hunzm->cur = hunzm->top;
}
unsigned unzmGoToNextFile(HUNZM hunzm)
{
	Uint8 *p;
	ZIP_LOCAL_HEADER zlh;
	if (hunzm->cur >= hunzm->top + hunzm->len) return 1;
	p = GetLocalHeader(&zlh, hunzm->cur);
	if (!p) return 1;
	hunzm->cur = p + zlh.size_def;
	if (hunzm->cur >= hunzm->top + hunzm->len) return 1;
	return 0;
}
unsigned unzmGetCurrentFileInfo(HUNZM hunzm, ZIP_LOCAL_HEADER *pzlh)
{
	if (hunzm->cur >= hunzm->top + hunzm->len) return 1;
	GetLocalHeader(pzlh, hunzm->cur);
	return 0;
}

unsigned unzmExtract(HUNZM hunzm, void *pbuf)
{
	Uint8 *p;
	int ret;
	ZIP_LOCAL_HEADER zlh;
	z_stream str;
	if (hunzm->cur >= hunzm->top + hunzm->len) return 0;
	p = GetLocalHeader(&zlh, hunzm->cur);
	if (!p) return 0;
	if (zlh.method == ZIPMETHOD_STORED)
	{
		memcpy(pbuf, p, zlh.size_inf);
		return zlh.size_inf;
	}
	if (zlh.method != ZIPMETHOD_DEFLATED) return 0;

    str.total_out = 0;
	str.zalloc = (alloc_func)0;
	str.zfree = (free_func)0;
	str.opaque = (voidpf)0; 
	if (inflateInit2(&str, -MAX_WBITS) < 0)
		return 0;

	str.next_in = p;
	str.avail_in = zlh.size_def;
	str.next_out = pbuf;
	str.avail_out = zlh.size_inf;
	ret = inflate(&str, Z_SYNC_FLUSH);
	inflateEnd(&str);

	if (ret < 0) return 0;
	return zlh.size_inf;
}

void unzmClose(HUNZM hunzm)
{
	free(hunzm);
}
