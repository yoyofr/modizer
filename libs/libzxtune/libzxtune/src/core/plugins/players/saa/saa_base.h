/**
* 
* @file
*
* @brief  SAA-based chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "saa_parameters.h"
#include "core/plugins/players/tracking.h"
//library includes
#include <core/module_holder.h>
#include <sound/render_params.h>

namespace Module
{
  namespace SAA
  {
    const uint_t TRACK_CHANNELS = 6;

    class ChannelBuilder
    {
    public:
      ChannelBuilder(uint_t chan, Devices::SAA::Registers& regs);

      void SetVolume(int_t left, int_t right);
      void SetTone(uint_t octave, uint_t note);
      void SetNoise(uint_t type);
      void AddNoise(uint_t type);
      void SetEnvelope(uint_t type);
      void EnableTone();
      void EnableNoise();
    private:
      void SetRegister(uint_t reg, uint_t val)
      {
        Regs.Data[reg] = static_cast<uint8_t>(val);
        Regs.Mask |= 1 << reg;
      }

      void SetRegister(uint_t reg, uint_t val, uint_t mask)
      {
        Regs.Data[reg] &= ~mask;
        AddRegister(reg, val);
      }

      void AddRegister(uint_t reg, uint_t val)
      {
        Regs.Data[reg] |= static_cast<uint8_t>(val);
        Regs.Mask |= 1 << reg;
      }
    private:
      const uint_t Channel;
      Devices::SAA::Registers& Regs;
    };

    class TrackBuilder
    {
    public:
      ChannelBuilder GetChannel(uint_t chan)
      {
        return ChannelBuilder(chan, Regs);
      }

      void GetResult(Devices::SAA::Registers& result) const
      {
        result = Regs;
      }
    private:
      Devices::SAA::Registers Regs;
    };

    class DataRenderer
    {
    public:
      typedef boost::shared_ptr<DataRenderer> Ptr;

      virtual ~DataRenderer() {}

      virtual void SynthesizeData(const TrackModelState& state, TrackBuilder& track) = 0;
      virtual void Reset() = 0;
    };

    class DataIterator : public StateIterator
    {
    public:
      typedef boost::shared_ptr<DataIterator> Ptr;

      virtual Devices::SAA::Registers GetData() const = 0;
    };

    class Chiptune
    {
    public:
      typedef boost::shared_ptr<const Chiptune> Ptr;
      virtual ~Chiptune() {}

      virtual Information::Ptr GetInformation() const = 0;
      virtual Parameters::Accessor::Ptr GetProperties() const = 0;
      virtual DataIterator::Ptr CreateDataIterator() const = 0;
    };

    Analyzer::Ptr CreateAnalyzer(Devices::SAA::Device::Ptr device);

    DataIterator::Ptr CreateDataIterator(TrackStateIterator::Ptr iterator, DataRenderer::Ptr renderer);

    Renderer::Ptr CreateRenderer(Sound::RenderParameters::Ptr params, DataIterator::Ptr iterator, Devices::SAA::Device::Ptr device);

    Holder::Ptr CreateHolder(Chiptune::Ptr chiptune);
  }
}
