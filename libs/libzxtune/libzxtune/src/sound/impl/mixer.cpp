/**
*
* @file
*
* @brief  Mixer implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "mixer_core.h"
//common includes
#include <error_tools.h>
//library includes
#include <l10n/api.h>
#include <math/numeric.h>
#include <sound/matrix_mixer.h>
//std includes
#include <algorithm>
#include <numeric>
//boost includes
#include <boost/make_shared.hpp>

#define FILE_TAG 278565B1

namespace Sound
{
  const L10n::TranslateFunctor translate = L10n::TranslateFunctor("sound");

  template<unsigned Channels>
  class Mixer : public FixedChannelsMatrixMixer<Channels>
  {
    typedef FixedChannelsMatrixMixer<Channels> Base;
  public:
    Mixer()
    {
      const Gain::Type INVALID_GAIN_VALUE(Gain::Type::PRECISION, 1);
      const Gain INVALID_GAIN(INVALID_GAIN_VALUE, INVALID_GAIN_VALUE);
      std::fill(LastMatrix.begin(), LastMatrix.end(), INVALID_GAIN);
    }

    virtual Sample ApplyData(const typename Base::InDataType& in) const
    {
      return Core.Mix(in);
    }

    virtual void SetMatrix(const typename Base::Matrix& data)
    {
      const typename Base::Matrix::const_iterator it = std::find_if(data.begin(), data.end(), std::not1(std::mem_fun_ref(&Gain::IsNormalized)));
      if (it != data.end())
      {
        throw Error(THIS_LINE, translate("Failed to set mixer matrix: gain is out of range."));
      }
      if (LastMatrix != data)
      {
        Core.SetMatrix(data);
        LastMatrix = data;
      }
    }
  private:
    MixerCore<Channels> Core;
    typename Base::Matrix LastMatrix;
  };
}

namespace Sound
{
  template<>
  OneChannelMatrixMixer::Ptr OneChannelMatrixMixer::Create()
  {
    return boost::make_shared<Mixer<1> >();
  }

  template<>
  TwoChannelsMatrixMixer::Ptr TwoChannelsMatrixMixer::Create()
  {
    return boost::make_shared<Mixer<2> >();
  }

  template<>
  ThreeChannelsMatrixMixer::Ptr ThreeChannelsMatrixMixer::Create()
  {
    return boost::make_shared<Mixer<3> >();
  }

  template<>
  FourChannelsMatrixMixer::Ptr FourChannelsMatrixMixer::Create()
  {
    return boost::make_shared<Mixer<4> >();
  }
}
