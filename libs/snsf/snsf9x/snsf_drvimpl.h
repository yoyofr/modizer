#define XSF_FALSE (0)
#define XSF_TRUE (!XSF_FALSE)

typedef unsigned long DWORD;
typedef int INT32;
typedef unsigned int UINT32;
typedef unsigned char BYTE;


#ifdef __cplusplus
extern "C" {
#endif

int snsf_start(const unsigned char *lrombuf, INT32 lromsize, const unsigned char *srambuf, INT32 sramsize);
int snsf_gen(void *pbuffer, unsigned samples);
void snsf_term(void);
void snsf_set_extend_param(unsigned dwId, const wchar_t *lpPtr);
void snsf_set_extend_param_void(unsigned dwId, const void* lpPtr);
extern unsigned long dwChannelMute;

#ifdef __cplusplus
}
#endif
