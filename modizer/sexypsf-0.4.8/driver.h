#include "types.h"


#define SPSFVERSION     "0.4.8"

typedef struct __PSFTAG
{
 char *key;
 char *value;
 struct __PSFTAG *next;
} PSFTAG;

typedef struct {
        u32 length;
        u32 stop;
        u32 fade;
        char *title,*artist,*game,*year,*genre,*psfby,*comment,*copyright;
        PSFTAG *tags;
} PSFINFO;

void sexy_seek(u32 t);
void sexy_stop(void);
void sexy_execute(void);
void sexy_execute_slice();
void sexyd_update(unsigned char* pSound,long lBytes);
int sexyd_updateseek(int progress);


PSFINFO *sexy_load(char *path,const char *pathDir,int loop_infinite);
int sexy_reload();

#ifdef ZIP_SUPPORT
PSFINFO *zip_sexy_load(char *zip_path, char *path);
#endif
#ifdef MEM_SUPPORT
// Must pass pointer to psf library first !!
// char *memGetPSFLibName(char *addr, int size);
void sexy_setpsflibs(struct XSFLibN *libs);
PSFINFO *sexy_memload(char *addr, int size);
#endif
PSFINFO *sexy_getpsfinfo(char *path,const char*pathDir);
void sexy_freepsfinfo(PSFINFO *info);

void sexyPSF_setInterpolation(int interpol);
int sexyPSF_getInterpolation(int interpol);
void sexyPSF_setReverb(int rev);
int sexyPSF_getReverb(int rev);
