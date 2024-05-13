/**
* 
* @file
*
* @brief  ProSoundCreator support interface
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
    namespace ProSoundCreator
    {
      struct Sample
      {
        struct Line
        {
          Line()
            : Level(), Tone(), ToneMask(true), NoiseMask(true), Adding()
            , EnableEnvelope(), VolumeDelta()
            , LoopBegin()
            , LoopEnd()
          {
          }

          uint_t Level;//0-15
          uint_t Tone;
          bool ToneMask;
          bool NoiseMask;
          int_t Adding;
          bool EnableEnvelope;
          int_t VolumeDelta;//0/+1/-1
          bool LoopBegin;
          bool LoopEnd;
        };

        std::vector<Line> Lines;
      };

      struct Ornament
      {
        struct Line
        {
          Line()
            : NoteAddon(), NoiseAddon()
            , LoopBegin(), LoopEnd()
          {
          }
          int_t NoteAddon;
          int_t NoiseAddon;
          bool LoopBegin;
          bool LoopEnd;
        };

        std::vector<Line> Lines;
      };

      class Builder
      {
      public:
        virtual ~Builder() {}

        virtual MetaBuilder& GetMetaBuilder() = 0;
        //common properties
        virtual void SetInitialTempo(uint_t tempo) = 0;
        //samples+ornaments
        virtual void SetSample(uint_t index, const Sample& sample) = 0;
        virtual void SetOrnament(uint_t index, const Ornament& ornament) = 0;
        //patterns
        virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop) = 0;

        virtual PatternBuilder& StartPattern(uint_t index) = 0;

        //! @invariant Channels are built sequentally
        virtual void StartChannel(uint_t index) = 0;
        virtual void SetRest() = 0;
        virtual void SetNote(uint_t note) = 0;
        virtual void SetSample(uint_t sample) = 0;
        virtual void SetOrnament(uint_t ornament) = 0;
        virtual void SetVolume(uint_t vol) = 0;
        //cmds
        virtual void SetEnvelope(uint_t type, uint_t tone) = 0;
        virtual void SetEnvelope() = 0;
        virtual void SetNoEnvelope() = 0;
        virtual void SetNoiseBase(uint_t val) = 0;
        virtual void SetBreakSample() = 0;
        virtual void SetBreakOrnament() = 0;
        virtual void SetNoOrnament() = 0;
        virtual void SetGliss(uint_t absStep) = 0;
        virtual void SetSlide(int_t delta) = 0;
        virtual void SetVolumeSlide(uint_t period, int_t delta) = 0;
      };

      Builder& GetStubBuilder();
      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target);
    }
  }
}
