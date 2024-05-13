/**
* 
* @file
*
* @brief  AYM-based stream chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "aym_chiptune.h"

namespace Module
{
  namespace AYM
  {
    class StreamModel
    {
    public:
      typedef boost::shared_ptr<const StreamModel> Ptr;
      virtual ~StreamModel() {}

      virtual uint_t Size() const = 0;
      virtual uint_t Loop() const = 0;
      virtual Devices::AYM::Registers Get(uint_t pos) const = 0;
    };

    Chiptune::Ptr CreateStreamedChiptune(StreamModel::Ptr model, Parameters::Accessor::Ptr properties);
  }
}
