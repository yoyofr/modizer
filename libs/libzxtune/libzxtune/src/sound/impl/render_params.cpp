/**
*
* @file
*
* @brief  Render params implementation
*
* @author vitamin.caig@gmail.com
*
**/

//library includes
#include <sound/render_params.h>
#include <sound/sound_parameters.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Sound
{
  class RenderParametersImpl : public RenderParameters
  {
  public:
    explicit RenderParametersImpl(Parameters::Accessor::Ptr params)
      : Params(params)
    {
    }

    virtual uint_t Version() const
    {
      return Params->Version();
    }

    virtual uint_t SoundFreq() const
    {
      using namespace Parameters::ZXTune::Sound;
      return static_cast<uint_t>(FoundProperty(FREQUENCY, FREQUENCY_DEFAULT));
    }

    virtual Time::Microseconds FrameDuration() const
    {
      using namespace Parameters::ZXTune::Sound;
      return Time::Microseconds(FoundProperty(FRAMEDURATION, FRAMEDURATION_DEFAULT));
    }

    virtual bool Looped() const
    {
      using namespace Parameters::ZXTune::Sound;
      return 0 != FoundProperty(LOOPED, 0);
    }

    virtual uint_t SamplesPerFrame() const
    {
      const uint_t freq = SoundFreq();
      const Time::Microseconds frameDuration = FrameDuration();
      return static_cast<uint_t>(frameDuration.Get() * freq / frameDuration.PER_SECOND);
    }
  private:
    Parameters::IntType FoundProperty(const Parameters::NameType& name, Parameters::IntType defVal) const
    {
      Parameters::IntType ret = defVal;
      Params->FindValue(name, ret);
      return ret;
    }
  private:
    const Parameters::Accessor::Ptr Params;
  };
}

namespace Sound
{
  RenderParameters::Ptr RenderParameters::Create(Parameters::Accessor::Ptr soundParameters)
  {
    return boost::make_shared<RenderParametersImpl>(soundParameters);
  }
}
