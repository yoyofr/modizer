/**
* 
* @file
*
* @brief  DAC sample interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <sound/sample.h>
//boost includes
#include <boost/shared_ptr.hpp>

namespace Devices
{
  namespace DAC
  {
    class Sample
    {
    public:
      typedef boost::shared_ptr<const Sample> Ptr;
      virtual ~Sample() {}

      virtual Sound::Sample::Type Get(std::size_t pos) const = 0;
      virtual std::size_t Size() const = 0;
      virtual std::size_t Loop() const = 0;
      virtual uint_t Rms() const = 0;
    };
  }
}
