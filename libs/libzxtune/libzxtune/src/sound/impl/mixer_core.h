/**
*
* @file
*
* @brief  Declaration of fast mixer
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <sound/gain.h>
#include <sound/multichannel_sample.h>
//boost includes
#include <boost/static_assert.hpp>

namespace Sound
{
  template<int_t ChannelsCount>
  class MixerCore
  {
  public:
    typedef boost::array<Gain, ChannelsCount> MatrixType;
    typedef typename MultichannelSample<ChannelsCount>::Type InType;

    MixerCore()
    {
      const Coeff val = Coeff(1, ChannelsCount);
      for (uint_t inChan = 0; inChan != ChannelsCount; ++inChan)
      {
        CoeffRow& row = Matrix[inChan];
        for (uint_t outChan = 0; outChan != row.size(); ++outChan)
        {
          row[outChan] = val;
        }
      }
    }

    Sample Mix(const InType& in) const
    {
      Coeff out[Sample::CHANNELS];
      for (uint_t inChan = 0; inChan != ChannelsCount; ++inChan)
      {
        const int_t val = in[inChan];
        const CoeffRow& row = Matrix[inChan];
        for (uint_t outChan = 0; outChan != row.size(); ++outChan)
        {
          out[outChan] += row[outChan] * val;
        }
      }
      BOOST_STATIC_ASSERT(Sample::CHANNELS == 2);
      return Sample(out[0].Integer(), out[1].Integer());
    }

    void SetMatrix(const MatrixType& matrix)
    {
      for (uint_t inChan = 0; inChan != ChannelsCount; ++inChan)
      {
        const Gain& in = matrix[inChan];
        CoeffRow& out = Matrix[inChan];
        out[0] = Coeff(in.Left() / ChannelsCount);
        out[1] = Coeff(in.Right() / ChannelsCount);
      }
    }
  private:
    static const int_t PRECISION = 256;
    typedef Math::FixedPoint<int_t, PRECISION> Coeff;
    typedef boost::array<Coeff, Sample::CHANNELS> CoeffRow;
    typedef boost::array<CoeffRow, ChannelsCount> CoeffMatrix;
    CoeffMatrix Matrix;
  };
}
