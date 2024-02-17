/*
 * Copyright (C) 2004-2010 NXP Software
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**********************************************************************************

     $Author: beq07716 $
     $Revision: 1000 $
     $Date: 2010-06-28 13:08:20 +0200 (Mon, 28 Jun 2010) $

***********************************************************************************/

/**********************************************************************************
   INCLUDE FILES
***********************************************************************************/

#include "VectorArithmetic.h"

/**********************************************************************************
   FUNCTION  MSTO2I_SAT_16X16
***********************************************************************************/

void MSTo2i_Sat_16x16(const LVM_INT16  *srcM,
                      const LVM_INT16  *srcS,
                      LVM_INT16  *dst,
                      LVM_INT16  n )
{
    LVM_INT32 temp,mVal,sVal;
    LVM_INT16 ii;


    for (ii = n; ii != 0; ii--)
    {
        mVal=(LVM_INT32)*srcM;
        srcM++;

        sVal=(LVM_INT32)*srcS;
        srcS++;

        temp = mVal + sVal;

        if (temp > 0x00007FFF)
        {
            *dst = 0x7FFF;
        }
        else if (temp < -0x00008000)
        {
            *dst = - 0x8000;
        }
        else
        {
            *dst = (LVM_INT16)temp;
        }
        dst++;

        temp = mVal - sVal;

        if (temp > 0x00007FFF)
        {
            *dst = 0x7FFF;
        }
        else if (temp < -0x00008000)
        {
            *dst = - 0x8000;
        }
        else
        {
            *dst = (LVM_INT16)temp;
        }
        dst++;
    }

    return;
}
/**********************************************************************************/
