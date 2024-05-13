/**
* 
* @file
*
* @brief  DigitalMusicMaker support interface
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//local includes
#include "formats/chiptune/builder_meta.h"
#include "formats/chiptune/builder_pattern.h"
//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    namespace DigitalMusicMaker
    {
      class ChannelBuilder
      {
      public:
        virtual ~ChannelBuilder() {}

        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetVolume(uint_t volume) = 0;

        virtual void SetFloat(int_t direction) = 0;
        virtual void SetFloatParam(uint_t step) = 0;
        virtual void SetVibrato() = 0;
        virtual void SetVibratoParams(uint_t step, uint_t period) = 0;
        virtual void SetArpeggio() = 0;
        virtual void SetArpeggioParams(uint_t step, uint_t period) = 0;
        virtual void SetSlide(int_t direction) = 0;
        virtual void SetSlideParams(uint_t step, uint_t period) = 0;
        virtual void SetDoubleNote() = 0;
        virtual void SetDoubleNoteParam(uint_t period) = 0;
        virtual void SetVolumeAttack() = 0;
        virtual void SetVolumeAttackParams(uint_t limit, uint_t period) = 0;
        virtual void SetVolumeDecay() = 0;
        virtual void SetVolumeDecayParams(uint_t limit, uint_t period) = 0;
        virtual void SetMixSample(uint_t idx) = 0;
        virtual void SetNoEffects() = 0;
      };

      class Builder
      {
      public:
        virtual ~Builder() {}

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //common properties
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples
        virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr sample) = 0;
        virtual std::auto_ptr<ChannelBuilder> SetSampleMixin(uint_t index, uint_t period) = 0;
        //patterns
        virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop) = 0;

        virtual PatternBuilder& StartPattern(uint_t index) = 0;
        //! @invariant Channels are built sequentally
        virtual std::auto_ptr<ChannelBuilder> StartChannel(uint_t index) = 0;
      };

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
      Builder& GetStubBuilder();
    }
  }
}
