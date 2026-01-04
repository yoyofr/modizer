#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned version;
	unsigned flag;
	unsigned method;
	unsigned time;
	unsigned crc32;
	unsigned size_def;
	unsigned size_inf;
	unsigned size_fn;
	unsigned size_ext;
	char fname[260];
} ZIP_LOCAL_HEADER;
typedef struct UNZIPMEM_T *HUNZM;

HUNZM unzmOpen(void *p, unsigned len);
void unzmGoToFirstFile(HUNZM hunzm);
unsigned unzmGoToNextFile(HUNZM hunzm);
unsigned unzmGetCurrentFileInfo(HUNZM hunzm, ZIP_LOCAL_HEADER *pzlh);
unsigned unzmExtract(HUNZM hunzm, void *pbuf);
void unzmClose(HUNZM hunzm);

#ifdef __cplusplus
}
#endif
