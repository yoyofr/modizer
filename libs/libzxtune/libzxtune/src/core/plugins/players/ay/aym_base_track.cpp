/**
* 
* @file
*
* @brief  AYM-based track chiptunes support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base_track.h"
//library includes
#include <devices/details/parameters_helper.h>
#include <math/numeric.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
  namespace AYM
  {
    class TrackDataIterator : public DataIterator
    {
    public:
      TrackDataIterator(TrackParameters::Ptr trackParams, TrackStateIterator::Ptr delegate, DataRenderer::Ptr renderer)
        : Params(trackParams)
        , Delegate(delegate)
        , State(Delegate->GetStateObserver())
        , Render(renderer)
      {
      }

      virtual void Reset()
      {
        Params.Reset();
        Delegate->Reset();
        Render->Reset();
      }

      virtual bool IsValid() const
      {
        return Delegate->IsValid();
      }

      virtual void NextFrame(bool looped)
      {
        Delegate->NextFrame(looped);
      }

      virtual TrackState::Ptr GetStateObserver() const
      {
        return State;
      }

      virtual Devices::AYM::Registers GetData() const
      {
        return Delegate->IsValid()
          ? GetCurrentChunk()
          : Devices::AYM::Registers();
      }
    private:
      Devices::AYM::Registers GetCurrentChunk() const
      {
        SynchronizeParameters();
        TrackBuilder builder(Table);
        Render->SynthesizeData(*State, builder);
        return builder.GetResult();
      }

      void SynchronizeParameters() const
      {
        if (Params.IsChanged())
        {
          Params->FreqTable(Table);
        }
      }
    private:
      Devices::Details::ParametersHelper<AYM::TrackParameters> Params;
      const TrackStateIterator::Ptr Delegate;
      const TrackModelState::Ptr State;
      const AYM::DataRenderer::Ptr Render;
      mutable FrequencyTable Table;
    };

    void ChannelBuilder::SetTone(int_t halfTones, int_t offset)
    {
      const int_t halftone = Math::Clamp<int_t>(halfTones, 0, static_cast<int_t>(Table.size()) - 1);
      const uint_t tone = (Table[halftone] + offset) & 0xfff;
      SetTone(tone);
    }

    void ChannelBuilder::SetTone(uint_t tone)
    {
      using namespace Devices::AYM;
      const uint_t reg = Registers::TONEA_L + 2 * Channel;
      Data[static_cast<Registers::Index>(reg)] = static_cast<uint8_t>(tone & 0xff);
      Data[static_cast<Registers::Index>(reg + 1)] = static_cast<uint8_t>(tone >> 8);
    }

    void ChannelBuilder::SetLevel(int_t level)
    {
      using namespace Devices::AYM;
      const uint_t reg = Registers::VOLA + Channel;
      Data[static_cast<Registers::Index>(reg)] = static_cast<uint8_t>(Math::Clamp<int_t>(level, 0, 15));
    }

    void ChannelBuilder::DisableTone()
    {
      Data[Devices::AYM::Registers::MIXER] |= (Devices::AYM::Registers::MASK_TONEA << Channel);
    }

    void ChannelBuilder::EnableEnvelope()
    {
      using namespace Devices::AYM;
      const uint_t reg = Registers::VOLA + Channel;
      Data[static_cast<Registers::Index>(reg)] |= Registers::MASK_ENV;
    }

    void ChannelBuilder::DisableNoise()
    {
      Data[Devices::AYM::Registers::MIXER] |= (Devices::AYM::Registers::MASK_NOISEA << Channel);
    }

    void TrackBuilder::SetNoise(uint_t level)
    {
      Data[Devices::AYM::Registers::TONEN] = static_cast<uint8_t>(level & 31);
    }

    void TrackBuilder::SetEnvelopeType(uint_t type)
    {
      Data[Devices::AYM::Registers::ENV] = static_cast<uint8_t>(type);
    }

    void TrackBuilder::SetEnvelopeTone(uint_t tone)
    {
      Data[Devices::AYM::Registers::TONEE_L] = static_cast<uint8_t>(tone & 0xff);
      Data[Devices::AYM::Registers::TONEE_H] = static_cast<uint8_t>(tone >> 8);
    }

    uint_t TrackBuilder::GetFrequency(int_t halfTone) const
    {
      return Table[halfTone];
    }

    int_t TrackBuilder::GetSlidingDifference(int_t halfToneFrom, int_t halfToneTo) const
    {
      const int_t halfFrom = Math::Clamp<int_t>(halfToneFrom, 0, static_cast<int_t>(Table.size()) - 1);
      const int_t halfTo = Math::Clamp<int_t>(halfToneTo, 0, static_cast<int_t>(Table.size()) - 1);
      const int_t toneFrom = Table[halfFrom];
      const int_t toneTo = Table[halfTo];
      return toneTo - toneFrom;
    }

    DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams, TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer)
    {
      return boost::make_shared<TrackDataIterator>(trackParams, iterator, renderer);
    }
  }
}
