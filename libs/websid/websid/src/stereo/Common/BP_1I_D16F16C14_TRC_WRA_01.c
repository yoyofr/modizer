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

/************************************************************************/
/*                                                                      */
/*     Project::                                                        */
/*     $Author: beq07716 $*/
/*     $Revision: 1000 $*/
/*     $Date: 2010-06-28 13:08:20 +0200 (Mon, 28 Jun 2010) $*/
/*                                                                      */
/************************************************************************/

#include "BIQUAD.h"
#include "BP_1I_D16F16Css_TRC_WRA_01_Private.h"
#include "LVM_Macros.h"


/**************************************************************************
 ASSUMPTIONS:
 COEFS-
 pBiquadState->coefs[0] is A0,
 pBiquadState->coefs[1] is -B2,
 pBiquadState->coefs[2] is -B1, these are in Q14 format

 DELAYS-
 pBiquadState->pDelays[0] is x(n-1)L in Q0 format
 pBiquadState->pDelays[1] is x(n-2)L in Q0 format
 pBiquadState->pDelays[2] is y(n-1)L in Q0 format
 pBiquadState->pDelays[3] is y(n-2)L in Q0 format
***************************************************************************/

void BP_1I_D16F16C14_TRC_WRA_01 ( Biquad_Instance_t       *pInstance,
                                  LVM_INT16               *pDataIn,
                                  LVM_INT16               *pDataOut,
                                  LVM_INT16               NrSamples)


    {
        LVM_INT32 ynL;
        LVM_INT16 ii;
        PFilter_State pBiquadState = (PFilter_State) pInstance;

         for (ii = NrSamples; ii != 0; ii--)
         {


            /**************************************************************************
                            PROCESSING OF THE LEFT CHANNEL
            ***************************************************************************/
            // ynL= (A0 (Q14) * (x(n)L (Q0) - x(n-2)L (Q0) ) )  in Q14
            ynL=(LVM_INT32)pBiquadState->coefs[0]* ((*pDataIn)-pBiquadState->pDelays[1]);

            // ynL+= ((-B2 (Q14) * y(n-2)L (Q0) ) ) in Q14
            ynL+=(LVM_INT32)pBiquadState->coefs[1]*pBiquadState->pDelays[3];

            // ynL+= ((-B1 (Q30) * y(n-1)L (Q0) ) ) in Q14
            ynL+=(LVM_INT32)pBiquadState->coefs[2]*pBiquadState->pDelays[2];

            ynL=(LVM_INT16)(ynL>>14); // ynL in Q0
            /**************************************************************************
                            UPDATING THE DELAYS
            ***************************************************************************/
            pBiquadState->pDelays[3]=pBiquadState->pDelays[2]; // y(n-2)L=y(n-1)L
            pBiquadState->pDelays[1]=pBiquadState->pDelays[0]; // x(n-2)L=x(n-1)L
            pBiquadState->pDelays[2]=ynL; // Update y(n-1)L in Q0
            pBiquadState->pDelays[0]=(*pDataIn++); // Update x(n-1)L in Q0

            /**************************************************************************
                            WRITING THE OUTPUT
            ***************************************************************************/
            *pDataOut++=(LVM_INT16)ynL; // Write Left output in Q0

        }

    }

