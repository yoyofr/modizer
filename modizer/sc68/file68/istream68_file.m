/*
 *                         sc68 - FILE stream
 *         Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This program is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */



/* define this if you don't want FILE support. */
#ifndef ISTREAM_NO_FILE

#include <Foundation/Foundation.h>
#include <string.h>

#include "istream68_file.h"
#include "istream68_def.h"
#include "alloc68.h"

/** istream file structure. */
typedef struct {
  istream_t istream; /**< istream function. */
	NSFileHandle* fileHandle;
	NSString* name;
      
  /** Open modes. */
  int mode;

} istream_file_t;

static const char * isf_name(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  return (!isf || !isf->name)
    ? 0
    : [isf->name cStringUsingEncoding:NSUTF8StringEncoding];
}


static int isf_open(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  if (!isf || !isf->name || isf->fileHandle) {
    return -1;
  }

//	NSLog(@"%@",isf->name);
  if (ISTREAM_IS_OPEN_READ(isf->mode) && ISTREAM_IS_OPEN_WRITE(isf->mode)) {
    isf->fileHandle = [NSFileHandle fileHandleForUpdatingAtPath:isf->name];
  } else if (ISTREAM_IS_OPEN_READ(isf->mode)) {
    isf->fileHandle = [NSFileHandle fileHandleForReadingAtPath:isf->name];
  } else if (ISTREAM_IS_OPEN_WRITE(isf->mode)) {
    isf->fileHandle = [NSFileHandle fileHandleForWritingAtPath:isf->name];
  }
  if (isf->fileHandle) {
    [isf->fileHandle retain];
    return 0;
  }
  return -1; 
}

static int isf_close(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  if (!isf) {
    return -1;
  }
  if (isf->fileHandle) {
  	[isf->fileHandle closeFile];
    [isf->fileHandle release];
    isf->fileHandle = nil;
  }
  return 0;
}

static int isf_read(istream_t * istream, void * buffer, int n)
{
  istream_file_t * isf = (istream_file_t *)istream;

  if (!isf || !isf->fileHandle) {
    return -1;
  } else {
  	NSData* data = [isf->fileHandle readDataOfLength:n];
    if (data) {
    	[data getBytes:buffer length:[data length]];
      return [data length];
    }
    return -1;
	}
}

static int isf_write(istream_t * istream, const void * data, int n)
{
  istream_file_t * isf = (istream_file_t *)istream;

  if (!isf || !isf->fileHandle) {
    return -1;
  } else {
		[isf->fileHandle writeData:[NSData dataWithBytes:data length:n]];
    return n;
  }
}


/* We could have store the length value at opening, but this way it handles
 * dynamic changes of file size.
 */
static int isf_length(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  if (!isf || !isf->fileHandle) {
    return -1;
  }
	int len = 0;
  
  unsigned long long pos = [isf->fileHandle offsetInFile];
  [isf->fileHandle seekToEndOfFile];
  len = (int)[isf->fileHandle offsetInFile];
  [isf->fileHandle seekToFileOffset:pos];
  
  return len;
}

static int isf_tell(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;

  if (!isf || !isf->fileHandle) {
  	return -1;
  } else {
  	return (int)[isf->fileHandle offsetInFile];
	}
}

static int isf_seek(istream_t * istream, int offset)
{
  istream_file_t * isf = (istream_file_t *)istream;

  if (isf && isf->fileHandle) {
  	unsigned long long fo = [isf->fileHandle offsetInFile];
    fo += offset;
    @try {
			[isf->fileHandle seekToFileOffset:fo];
			return (int)fo;
  	}
    @catch (NSException* e) {
			return -1;
    }
	}
  return -1;
}

static void isf_destroy(istream_t * istream)
{
  istream_file_t * isf = (istream_file_t *)istream;
	if (isf->fileHandle) {
  	[isf->fileHandle closeFile];
    [isf->fileHandle release];
    isf->fileHandle = nil;
  }
  SC68free(istream);
}

static const istream_t istream_file = {
  isf_name,
  isf_open, isf_close,
  isf_read, isf_write,
  isf_length, isf_tell, isf_seek, isf_seek,
  isf_destroy
};

istream_t * istream_file_create(const char * fname, int mode)
{
  istream_file_t *isf = NULL;

  if (!fname || !fname[0]) {
    return 0;
  }

  /* Don't need 0, because 1 byte already allocated in the
   * istream_file_t::fname.
   */
  isf = SC68alloc(sizeof(istream_file_t));
  if (!isf) {
    return 0;
  }

	isf->name = [[[NSString alloc] initWithCString:fname encoding:NSUTF8StringEncoding] autorelease];

  /* Copy istream functions. */
  memcpy(&isf->istream, &istream_file, sizeof(istream_file));
  /* Clean file handle. */
  isf->fileHandle = nil;
  isf->mode = mode & (ISTREAM_OPEN_READ|ISTREAM_OPEN_WRITE);
  
  return &isf->istream;
}

#else /* #ifndef ISTREAM_NO_FILE */

/* istream file must not be include in this package. Anyway the creation
 * still exist but it always returns error.
 */

istream_t * istream_file_create(const char * fname, int mode)
{
  return 0;
}

#endif /* #ifndef ISTREAM_NO_FILE */
