/**
* 
* @file
*
* @brief  DigitalStudio support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "digital.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace DigitalStudio
    {
      typedef Digital::Builder Builder;

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    }
  }
}
