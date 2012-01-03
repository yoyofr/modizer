/***************************************************************************
                          resid.h  -  ReSid Builder
                             -------------------
    begin                : Fri Apr 4 2001
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

#ifndef _resid_h_
#define _resid_h_

/* Since ReSID is not part of this project we are actually
 * creating a wrapper instead of implementing a SID emulation
 */

#include <vector>
//#include "sidplayfp/sidbuilder.h"
#include "sidbuilder.h"

/**
* ReSID Builder Class
*/
class SID_EXTERN ReSIDBuilder: public sidbuilder
{
protected:
    std::vector<sidemu *> sidobjs;

private:
    static const char  *ERR_FILTER_DEFINITION;
    char        m_errorBuffer[100];
    const char *m_error;

public:
    ReSIDBuilder  (const char * const name);
    ~ReSIDBuilder (void);

    /**
    * true will give you the number of used devices.
    *    return values: 0 none, positive is used sids
    * false will give you all available sids.
    *    return values: 0 endless, positive is available sids.
    * use bool operator to determine error
    */
    uint        devices (bool used);
    uint        create  (uint sids);
    sidemu     *lock    (c64env *env, sid2_model_t model);
    void        unlock  (sidemu *device);
    void        remove  (void);
    const char *error   (void) const { return m_error; }
    const char *credits (void);

    /// @name global settings
    /// Settings that affect all SIDs
    //@{
    /// enable/disable filter
    void filter   (bool enable);

    /// @deprecated does nothing
    SID_DEPRECATED void filter   (const sid_filter_t *filter) {};

    /**
    * The bias is given in millivolts, and a maximum reasonable
    * control range is approximately -500 to 500.
    */
    void bias     (const float dac_bias);
    //@}
};

#endif // _resid_h_
