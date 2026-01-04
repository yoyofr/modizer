#ifdef __cplusplus
extern "C" {
#endif
unsigned NEZ_extract(char *lpszSrcFile, void **ppbuf);
unsigned NEZ_extractMem(void *data, unsigned len, void **ppbuf);
#ifdef __cplusplus
}
#endif
