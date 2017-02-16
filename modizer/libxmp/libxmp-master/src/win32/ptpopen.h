#ifndef _PT_POPEN_H
#define _PT_POPEN_H 1

#include <stdio.h>

#undef  popen
#define popen pt_popen
#undef  pclose
#define pclose pt_pclose

#ifdef __cplusplus
extern "C" {
#endif

FILE *	pt_popen(const char *cmd, const char *mode);
int	pt_pclose(FILE *fle);

#ifdef __cplusplus
}
#endif

#endif /* _PT_POPEN_H */

