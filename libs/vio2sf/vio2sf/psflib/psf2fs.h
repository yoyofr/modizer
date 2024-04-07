/*
PSFLIB - PSF2FS implementation
 
Copyright (c) 2012-2015 Christopher Snowhill
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef PSF2FS_H
#define PSF2FS_H

#include "vio_psflib.h"

#ifdef __cplusplus
extern "C" {
#endif

void * psf2fs_create();

void psf2fs_delete( void * );

int psf2fs_load_callback(void * psf2vfs, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size);

int psf2fs_virtual_readfile(void *psf2vfs, const char *path, int offset, char *buffer, int length);

#ifdef __cplusplus
}
#endif

#endif
