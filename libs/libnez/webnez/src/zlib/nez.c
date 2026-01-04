/*
	NEZtoNSF filter
	nez.c
*/

#include <stdio.h>
#include <stdlib.h>
#include "nez.h"
#include "memzip.h"

#include "nezuzext.h"

static char *GetExt(char *filename) {
	char *p = filename, *p2 = 0;
	while (*p)
	{
		int l = mblen(p, MB_CUR_MAX);
		if (!l) break;
		if (*p == '/' || *p == '\\' || *p == ':') p2 = 0;
		if (*p == '.') p2 = p;
		p += l;
	}
	if (p2) return p2;
	return filename;
}

int xstricmp(char *d, char *s)
{
	while (*d && *s)
	{
		int l;
		l = mblen(d, MB_CUR_MAX);
		if (!l) break;
		if (l > 1)
		{
			while (l--)
			{
				if (*d != *s) return ((unsigned char)*d) - ((unsigned char)*s);
				d++;
				s++;
			}
		}
		else
		{
			int dc, sc;
			dc = (unsigned char)*d;
			sc = (unsigned char)*s;
			if ('a' <= dc && dc <= 'z') dc += 'A' - 'a';
			if ('a' <= sc && sc <= 'z') sc += 'A' - 'a';
			if (dc != sc) return ((unsigned char)*d) - ((unsigned char)*s);
			d++;
			s++;
		}
	}
	return ((unsigned char)*d) - ((unsigned char)*s);
}

static int CheckExt(char *filename)
{
	int i;
	char *ext = GetExt(filename);
	for (i = 0; nez_extlist[i]; i++) if (!xstricmp(ext, nez_extlist[i])) return 1;
	return 0;
}

unsigned NEZ_extractMem(void *data, unsigned len, void **ppbuf)
{
	HUNZM hunzm = 0;
	void *outputbuf = 0;
	/* try */
	do
	{
		ZIP_LOCAL_HEADER unzinfo;
		unsigned outputsize;

		hunzm = unzmOpen(data, len);
		if (hunzm == 0) break;
		unzmGoToFirstFile(hunzm);
		do
		{
			unzmGetCurrentFileInfo(hunzm, &unzinfo);
			if (CheckExt(unzinfo.fname))
			{
				int len = -1;

				outputbuf = malloc(unzinfo.size_inf + 8);
				if (outputbuf == 0) break;
				outputsize = unzmExtract(hunzm, outputbuf);
				if (outputsize == 0) break;
				unzmClose(hunzm);
				*ppbuf = outputbuf;
				return outputsize;
			}
		} while (unzmGoToNextFile(hunzm) == 0);
		break;
	} while(0);
	/* finaly */
	if (outputbuf != 0) free(outputbuf);
	if (hunzm != 0) unzmClose(hunzm);
	return 0;
}

unsigned NEZ_extract(char *lpszSrcFile, void **ppbuf)
{
	unsigned ret = 0;
	void *bufp = 0;
	FILE *fp = 0;
	/* try */
	do
	{
		size_t s;
		unsigned u;
		long l;
		fp = fopen(lpszSrcFile, "rb");
		if (!fp) break;
		if (fseek(fp, 0, SEEK_END)) break;
		s = l = ftell(fp);
		if (l == -1L) break;
		if (fseek(fp, 0, SEEK_SET)) break;
		bufp = malloc(s);
		if (!bufp) break;
		if (s != fread(bufp, 1, s, fp)) break;
		fclose(fp); fp = 0;	/* ”O‚Ì‚½‚ß */
		u = NEZ_extractMem(bufp, s, ppbuf);
		if (u)
		{
			ret = u;
		}
		else
		{
			ret= s;
			*ppbuf = bufp;
			bufp = 0;
		}
	} while(0);
	/* finaly */
	if (bufp) free(bufp);
	if (fp) fclose(fp);
	return ret;
}
