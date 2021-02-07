/***************************************************************************
                          SidFilter.cpp  -  filter type decoding support
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

#include "sidplayfp/sidtypes.h"

class iniParser;

class SID_EXTERN SidFilter
{
protected:
    enum { FILTER_RESID=1, FILTER_RESIDFP=2 };
    int            m_status;
    const char    *m_errorString;
    sid_filter_t   m_filter;
    sid_filterfp_t m_filterfp;

protected:
    bool parsePoint(const char* str, long &a, long &b);
    void readType1 (iniParser *ini);
    void readType2 (iniParser *ini);
    void readType3 (iniParser *ini);

public:
    SidFilter ();
    ~SidFilter () {};

    void                read      (const char *filename);
    void                calcType2 (double fs, double fm, double ft);
    void                read      (const char *filename, const char* section);
    const char*         error     (void) { return m_errorString; }
    const sid_filter_t* provide   () const;
    const sid_filterfp_t* providefp   () const;

    operator bool () { return m_status!=0; }
    const SidFilter&    operator= (const SidFilter    &filter);
    const sid_filter_t &operator= (const sid_filter_t &filter);
    const sid_filter_t *operator= (const sid_filter_t *filter);
    const sid_filterfp_t &operator= (const sid_filterfp_t &filter);
    const sid_filterfp_t *operator= (const sid_filterfp_t *filter);
};
