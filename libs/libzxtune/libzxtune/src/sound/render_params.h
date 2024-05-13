/**
*
* @file
*
* @brief  Rendering parameters definition. Used as POD-helper
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <parameters/accessor.h>
#include <time/stamp.h>

namespace Sound
{
  //! @brief Input parameters for rendering
  class RenderParameters
  {
  public:
    typedef boost::shared_ptr<const RenderParameters> Ptr;

    virtual ~RenderParameters() {}

    virtual uint_t Version() const = 0;

    //! Rendering sound frequency
    virtual uint_t SoundFreq() const = 0;
    //! Frame duration in us
    virtual Time::Microseconds FrameDuration() const = 0;
    //! Loop mode
    virtual bool Looped() const = 0;
    //! Sound samples count per one frame
    virtual uint_t SamplesPerFrame() const = 0;

    static Ptr Create(Parameters::Accessor::Ptr soundParameters);
  };
}
