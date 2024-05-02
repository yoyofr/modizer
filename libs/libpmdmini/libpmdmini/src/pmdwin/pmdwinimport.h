//=============================================================================
//	Professional Music Driver [P.M.D.] version 4.8
//			DLL Import Header
//			Programmed By M.Kajihara
//			Windows Converted by C60
//=============================================================================

#ifndef PMDWINIMPORT_H
#define PMDWINIMPORT_H

#include "ipmdwin.h"


//=============================================================================
//	DLL Import Functions
//=============================================================================

#if defined _WIN32
  #ifdef USE_DLL
    #ifdef BUILDING_PROJECT
      #define API_ATTRIBUTE_EXPORT __declspec(dllexport)
      #define API_ATTRIBUTE __declspec(dllexport)
    #else
      #define API_ATTRIBUTE_IMPORT __declspec(dllimport)
      #define API_ATTRIBUTE __declspec(dllimport)
    #endif
  #else
    #define API_ATTRIBUTE
  #endif
#else
  #ifdef USE_DLL
    #if __GNUC__ >= 4
      #define API_ATTRIBUTE_IMPORT __attribute__ ((visibility ("default")))
      #define API_ATTRIBUTE_EXPORT __attribute__ ((visibility ("default")))
      #define API_ATTRIBUTE __attribute__ ((visibility ("default")))
    #else
      #define API_ATTRIBUTE_IMPORT
      #define API_ATTRIBUTE_EXPORT
      #define API_ATTRIBUTE
    #endif
  #else
    #define API_ATTRIBUTE
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

API_ATTRIBUTE int32_t WINAPI getversion(void);
API_ATTRIBUTE int32_t WINAPI getinterfaceversion(void);
API_ATTRIBUTE bool WINAPI pmdwininit(TCHAR *path);
API_ATTRIBUTE bool WINAPI loadrhythmsample(TCHAR *path);
API_ATTRIBUTE bool WINAPI setpcmdir(TCHAR **path);
API_ATTRIBUTE void WINAPI setpcmrate(int32_t rate);
API_ATTRIBUTE void WINAPI setppzrate(int32_t rate);
API_ATTRIBUTE void WINAPI setppsuse(bool value);
API_ATTRIBUTE void WINAPI setrhythmwithssgeffect(bool value);
API_ATTRIBUTE void WINAPI setpmd86pcmmode(bool value);
API_ATTRIBUTE bool WINAPI getpmd86pcmmode(void);
API_ATTRIBUTE int32_t WINAPI music_load(TCHAR *filename);
API_ATTRIBUTE int32_t WINAPI music_load2(uint8_t *musdata, int32_t size);
API_ATTRIBUTE void WINAPI music_start(void);
API_ATTRIBUTE void WINAPI music_stop(void);
API_ATTRIBUTE void WINAPI fadeout(int32_t speed);
API_ATTRIBUTE void WINAPI fadeout2(int32_t speed);
API_ATTRIBUTE void WINAPI getpcmdata(short *buf, int32_t nsamples);
API_ATTRIBUTE void WINAPI setfmcalc55k(bool flag);
API_ATTRIBUTE void WINAPI setppsinterpolation(bool ip);
API_ATTRIBUTE void WINAPI setp86interpolation(bool ip);
API_ATTRIBUTE void WINAPI setppzinterpolation(bool ip);
API_ATTRIBUTE char * WINAPI getmemo(char *dest, uint8_t *musdata, int32_t size, int32_t al);
API_ATTRIBUTE char * WINAPI getmemo2(char *dest, uint8_t *musdata, int32_t size, int32_t al);
API_ATTRIBUTE char * WINAPI getmemo3(char *dest, uint8_t *musdata, int32_t size, int32_t al);
API_ATTRIBUTE int32_t WINAPI fgetmemo(char *dest, TCHAR *filename, int32_t al);
API_ATTRIBUTE int32_t WINAPI fgetmemo2(char *dest, TCHAR *filename, int32_t al);
API_ATTRIBUTE int32_t WINAPI fgetmemo3(char *dest, TCHAR *filename, int32_t al);
API_ATTRIBUTE TCHAR * WINAPI getmusicfilename(TCHAR *dest);
API_ATTRIBUTE TCHAR * WINAPI getpcmfilename(TCHAR *dest);
API_ATTRIBUTE TCHAR * WINAPI getppcfilename(TCHAR *dest);
API_ATTRIBUTE TCHAR * WINAPI getppsfilename(TCHAR *dest);
API_ATTRIBUTE TCHAR * WINAPI getp86filename(TCHAR *dest);
API_ATTRIBUTE TCHAR * WINAPI getppzfilename(TCHAR *dest, int32_t bufnum);
API_ATTRIBUTE int32_t WINAPI ppc_load(TCHAR *filename);
API_ATTRIBUTE int32_t WINAPI pps_load(TCHAR *filename);
API_ATTRIBUTE int32_t WINAPI p86_load(TCHAR *filename);
API_ATTRIBUTE int32_t WINAPI ppz_load(TCHAR *filename, int32_t bufnum);
API_ATTRIBUTE int32_t WINAPI maskon(int32_t ch);
API_ATTRIBUTE int32_t WINAPI maskoff(int32_t ch);
API_ATTRIBUTE void WINAPI setfmvoldown(int32_t voldown);
API_ATTRIBUTE void WINAPI setssgvoldown(int32_t voldown);
API_ATTRIBUTE void WINAPI setrhythmvoldown(int32_t voldown);
API_ATTRIBUTE void WINAPI setadpcmvoldown(int32_t voldown);
API_ATTRIBUTE void WINAPI setppzvoldown(int32_t voldown);
API_ATTRIBUTE int32_t WINAPI getfmvoldown(void);
API_ATTRIBUTE int32_t WINAPI getfmvoldown2(void);
API_ATTRIBUTE int32_t WINAPI getssgvoldown(void);
API_ATTRIBUTE int32_t WINAPI getssgvoldown2(void);
API_ATTRIBUTE int32_t WINAPI getrhythmvoldown(void);
API_ATTRIBUTE int32_t WINAPI getrhythmvoldown2(void);
API_ATTRIBUTE int32_t WINAPI getadpcmvoldown(void);
API_ATTRIBUTE int32_t WINAPI getadpcmvoldown2(void);
API_ATTRIBUTE int32_t WINAPI getppzvoldown(void);
API_ATTRIBUTE int32_t WINAPI getppzvoldown2(void);
API_ATTRIBUTE void WINAPI setpos(int32_t pos);
API_ATTRIBUTE void WINAPI setpos2(int32_t pos);
API_ATTRIBUTE int32_t WINAPI getpos(void);
API_ATTRIBUTE int32_t WINAPI getpos2(void);
API_ATTRIBUTE bool WINAPI getlength(TCHAR *filename, int32_t *length, int32_t *loop);
API_ATTRIBUTE bool WINAPI getlength2(TCHAR *filename, int32_t *length, int32_t *loop);
API_ATTRIBUTE int32_t WINAPI getloopcount(void);
API_ATTRIBUTE void WINAPI setfmwait(int32_t nsec);
API_ATTRIBUTE void WINAPI setssgwait(int32_t nsec);
API_ATTRIBUTE void WINAPI setrhythmwait(int32_t nsec);
API_ATTRIBUTE void WINAPI setadpcmwait(int32_t nsec);
API_ATTRIBUTE OPEN_WORK * WINAPI getopenwork(void);
API_ATTRIBUTE QQ * WINAPI getpartwork(int32_t ch);

#ifdef __cplusplus
}
#endif


#endif // PMDWINIMPORT_H
