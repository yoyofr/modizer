/*
 *                 sc68 - Windows registry access
 *   Copyright (C) 2001-2003 Benjamin Gerard <ben@sashipa.com>
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



#ifndef _MSC_VER

#include "registry68.h"

const registry68_key_t registry68_CRK = 0;
const registry68_key_t registry68_CCK = 0;
const registry68_key_t registry68_CUK = 0;
const registry68_key_t registry68_LMK = 0;
const registry68_key_t registry68_UK  = 0;
const registry68_key_t registry68_IK  = 0;

char registry68ErrorStr[] = "Registry not supported";

registry68_key_t registry68_open(registry68_key_t hkey, char *kname)
{
  return registry68_IK;
}

int registry68_gets(registry68_key_t hkey, const char *kname,
		    char *kdata, int kdatasz)
{
  return -1;
}

#else

#include <windows.h>
#include <winreg.h>
#include <stdio.h>
#include <string.h>

#include "registry68.h"

const registry68_key_t registry68_CRK = HKEY_CLASSES_ROOT;
const registry68_key_t registry68_CCK = HKEY_CURRENT_CONFIG;
const registry68_key_t registry68_CUK = HKEY_CURRENT_USER;
const registry68_key_t registry68_LMK = HKEY_LOCAL_MACHINE;
const registry68_key_t registry68_UK  = HKEY_USERS;
const registry68_key_t registry68_IK  = (registry68_key_t)0;

char registry68_errorstr[256];

static void SetSystemError(void)
{
  const int max = sizeof(registry68_errorstr)-1;
  int err = GetLastError();
  
  registry68_errorstr[0] = 0;
  FormatMessage(
		// source and processing options
		FORMAT_MESSAGE_FROM_SYSTEM,
		// pointer to  message source
		0,
		// requested message identifier
		err,
		// language identifier for requested message
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		// pointer to message buffer
		registry68_errorstr,
		// maximum size of message buffer
		max,
		// pointer to array of message inserts
		0
		);
  registry68_errorstr[sizeof(registry68_errorstr)-1] = 0;
}

static int GetOneLevelKey(HKEY hkey, char **kname, HKEY *hkeyres)
{
  char *s;
  LONG hres;

  /* Find next sub-key */
  s=strchr(*kname,'/');
  if (s) {
    s[0] = 0;
  }

  /* Open this sub-key */
  hres =
    RegOpenKeyEx(hkey,     // handle to open key
		 *kname,   // address of name of subkey to open
		 0,        // reserved
		 KEY_READ, // security access mask
		 hkeyres   // address of handle to open key
		 );

  /* If next sub-key exist, advance pointer to beginning of key-name */
  if (s) {
    s[0] = '/';
    ++s;
  }

  if (hres != ERROR_SUCCESS) {
    *hkeyres = registry68_IK;
    return -1;
  } else {
    *kname = s;
    return 0;
  }
}

static int rGetRegistryKey(HKEY hkey, char *kname, HKEY *hkeyres)
{
  HKEY subkey = (HKEY) registry68_IK;
  int err;

  if (!kname) {
    *hkeyres = hkey;
    return 0;
  }

  err = GetOneLevelKey(hkey, &kname, &subkey);
  *hkeyres = subkey;
  if (!err && kname) {
    err = rGetRegistryKey(subkey, kname, hkeyres);
    RegCloseKey(subkey);
  }
  return err;
}

registry68_key_t registry68_open(registry68_key_t hkey, char *kname)
{
  HKEY hdl;

  if (rGetRegistryKey((HKEY)hkey, kname, (HKEY *)&hdl)) {
    SetSystemError();
    return registry68_IK;
  } else {
    return (registry68_key_t)hdl;
  }
}

/** Get a string value from register path
 */
int registry68_gets(registry68_key_t rootkey, const char * kname_cst,
		    char *kdata, int kdatasz)
{
  DWORD vtype;
  HKEY hkey;
  int err, len;
  LONG hres;
  char * name;
  char kname[1024];

  if (!kname_cst|| !kdata || kdatasz < 0) {
    return -1;
  }

  len = strlen(kname_cst);
  if (len >= sizeof(kname)) {
    return -1;
  }
  memcpy(kname, kname_cst, len+1);
  name = strrchr(kname,'/');
  if (name) {
    *name++ = 0;
  }
  
  err = rGetRegistryKey((HKEY)rootkey, kname, &hkey);
  if (!err) {
    hres =
      RegQueryValueEx(hkey,             // handle to key to query
		      name,             // address of name of value to query
		      NULL,             // reserved
		      &vtype,           // address of buffer for value type
		      (LPBYTE)kdata,    // address of data buffer
		      (LPDWORD)&kdatasz // address of data buffer size
		      );
    err = (hres==ERROR_SUCCESS && vtype == REG_SZ) ? 0 : -1;
    RegCloseKey(hkey);
  }
  if (err) {
    SetSystemError();
  }
  if (name) {
    *--name = '/';
  }
  return err;
}

#endif /* #ifndef _MSC_VER */

