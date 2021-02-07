/***************************************************************************
                          SidDatabase.h  -  songlength database support
                             -------------------
    begin                : Sun Mar 11 2001
    copyright            : (C) 2001 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _siddatabase_h_
#define _siddatabase_h_

#include "sidplayfp/sidconfig.h"
#include "sidplayfp/sidtypes.h"

class SidTuneMod;
class iniParser;

class SID_EXTERN SidDatabase
{
private:
    static const char *ERR_DATABASE_CORRUPT;
    static const char *ERR_NO_DATABASE_LOADED;
    static const char *ERR_NO_SELECTED_SONG;
    static const char *ERR_MEM_ALLOC;
    static const char *ERR_UNABLE_TO_LOAD_DATABASE;

    iniParser  *m_parser;
    const char *errorString;

    class parseError {};

    static const char* parseTime(const char* str, long &result);

public:
    SidDatabase  ();
    ~SidDatabase ();

    int           open   (const char *filename);
    void          close  ();
    int_least32_t length (SidTuneMod &tune);
    int_least32_t length (const char *md5, uint_least16_t song);
    const char *  error  (void) { return errorString; }
};

#endif // _siddatabase_h_
