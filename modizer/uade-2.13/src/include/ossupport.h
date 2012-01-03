#ifndef _UADE_OSSUPPORT_H_
#define _UADE_OSSUPPORT_H_

#include "unixsupport.h"

#ifndef _UADE_MEMMEM_H_
#define _UADE_MEMMEM_H_

#include <stdio.h>

void *memmem(const void *haystack, size_t haystacklen,
	     const void *needle, size_t needlelen);

#endif

#include <string.h>
#ifndef _CANONICALIZE_FILE_NAME_REP_H_
#define _CANONICALIZE_FILE_NAME_REP_H_

char *canonicalize_file_name(const char *path);

#endif

#endif
