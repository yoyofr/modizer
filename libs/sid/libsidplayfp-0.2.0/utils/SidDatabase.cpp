/***************************************************************************
                          dbget.cpp  -  Get time from database
                             -------------------
    begin                : Fri Jun 2 2000
    copyright            : (C) 2000 by Simon White
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

#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "SidDatabase.h"
#include "SidTuneMod.h"
#include "iniParser.h"
#include "MD5/MD5.h"

const char *SidDatabase::ERR_DATABASE_CORRUPT        = "SID DATABASE ERROR: Database seems to be corrupt.";
const char *SidDatabase::ERR_NO_DATABASE_LOADED      = "SID DATABASE ERROR: Songlength database not loaded.";
const char *SidDatabase::ERR_NO_SELECTED_SONG        = "SID DATABASE ERROR: No song selected for retrieving song length.";
const char *SidDatabase::ERR_MEM_ALLOC               = "SID DATABASE ERROR: Memory Allocation Failure.";
const char *SidDatabase::ERR_UNABLE_TO_LOAD_DATABASE = "SID DATABASE ERROR: Unable to load the songlegnth database.";

SidDatabase::SidDatabase () :
m_parser(0),
errorString(ERR_NO_DATABASE_LOADED)
{}

SidDatabase::~SidDatabase ()
{
    close ();
}

const char* SidDatabase::parseTime(const char* str, long &result) {

    char *end;
    const long minutes = strtol (str, &end, 10);
    if (*end != ':')
        throw parseError();

    const long seconds = strtol (++end, &end, 10);
    result = (minutes * 60) + seconds;

    while (!isspace (*end))
        end++;

    return end;
}


int SidDatabase::open (const char *filename)
{
    close ();
    m_parser = new iniParser();

    if (!m_parser->open (filename))
    {
        errorString = ERR_UNABLE_TO_LOAD_DATABASE;
        return -1;
    }

    return 0;
}

void SidDatabase::close ()
{
    delete m_parser;
    m_parser = 0;
}

int_least32_t SidDatabase::length (SidTuneMod &tune)
{
    char md5[SIDTUNE_MD5_LENGTH+1];
    uint_least16_t song = tune.getInfo().currentSong;
    if (!song)
    {
        errorString = ERR_NO_SELECTED_SONG;
        return -1;
    }
    tune.createMD5 (md5);
    return length  (md5, song);
}

int_least32_t SidDatabase::length (const char *md5, uint_least16_t song)
{
    if (!m_parser)
    {
        errorString = ERR_NO_DATABASE_LOADED;
        return -1;
    }

    // Read Time (and check times before hand)
    if (!m_parser->setSection ("Database"))
    {
        errorString = ERR_DATABASE_CORRUPT;
        return -1;
    }

    const char* timeStamp = m_parser->getValue  (md5);
    // If return is null then no entry found in database
    if (!timeStamp)
    {
        errorString = ERR_DATABASE_CORRUPT;
        return -1;
    }

    const char* str = timeStamp;
    long  time = 0;

    for (uint_least16_t i = 0; i < song; i++)
    {
        // Validate Time
        try {
            str = parseTime(str, time);
        } catch (parseError& e) {
            errorString = ERR_DATABASE_CORRUPT;
            return -1;
        }
    }

    return time;
}
