/*
PSFLIB - Main PSF parser implementation
 
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

#ifndef PSFLIB_H
#define PSFLIB_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vio_psf_file_callbacks
{
    /* list of characters which act as path separators, null terminated */
    const char * path_separators;

    /* opens the file pointed to by path read-only in binary mode, 
     * accepts UTF-8 encoding, returns file handle */
    void * (* fopen )(const char * path);

    /* reads to specified buffer, returns count of size bytes read */
    size_t (* fread )(void * buffer, size_t size, size_t count, void * handle);

    /* returns zero on success, -1 on error */
    int    (* fseek )(void * handle, int64_t, int);

    /* returns zero on success, -1 on error */
    int    (* fclose)(void * handle);

    /* returns current file offset */
    long   (* ftell )(void * handle);
} vio_psf_file_callbacks;

/* Receives exe and reserved bodies, with deepest _lib->_lib->_lib etc head first, followed
 * by the specified file itself, then followed by numbered library chains.
 *
 * Example:
 *
 * outermost file, a.psf, has _lib=b.psf and _lib2=c.psf tags; b.psf has _lib=d.psf:
 *
 * the callback will be passed the contents of d.psf, then b.psf, then a.psf, then c.psf
 *
 * Returning non-zero indicates an error.
 */
typedef int (* vio_psf_load_callback)(void * context, const uint8_t * exe, size_t exe_size,
                                  const uint8_t * reserved, size_t reserved_size);

/* Receives the name/value pairs from the outermost file, one at a time.
 *
 * Returning non-zero indicates an error.
 */
typedef int (* vio_psf_info_callback)(void * context, const char * name, const char * value);

/* Receives any status messages, which should be appended to one big message. */
typedef void (* vio_psf_status_callback)(void * context, const char * message);

/* Loads the PSF chain starting with uri, opened using file_callbacks, passes the tags,
 * if any, to the optional info_target callback, then passes all loaded data to load_target
 * with the highest priority file first.
 *
 * allowed_version may be set to zero to probe the file version, but is not recommended when
 * actually loading files into an emulator state.
 *
 * Both load_target and info_target are optional.
 *
 * Returns negative on error, PSF version on success.
 */
int vio_psf_load( const char * uri, const vio_psf_file_callbacks * file_callbacks, uint8_t allowed_version,
              vio_psf_load_callback load_target, void * load_context, vio_psf_info_callback info_target,
              void * info_context, int info_want_nested_tags, vio_psf_status_callback status_target,
              void * status_context);

#ifdef __cplusplus
}
#endif

#endif // PSFLIB_H
