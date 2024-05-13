/**
* 
* @file
*
* @brief  SAA parameters helpers
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <devices/saa.h>
#include <parameters/accessor.h>

namespace Module
{
  namespace SAA
  {
    Devices::SAA::ChipParameters::Ptr CreateChipParameters(Parameters::Accessor::Ptr params);
  }
}
