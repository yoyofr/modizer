/**
* 
* @file
*
* @brief  DigitalMusicMaker support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "digitalmusicmaker.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <range_checker.h>
//library includes
#include <binary/container_factories.h>
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::DigitalMusicMaker");
}

#define END_OF_BANKS_SIZE 6

namespace Formats
{
namespace Chiptune
{
  namespace DigitalMusicMaker
  {
    const std::size_t MAX_POSITIONS_COUNT = 0x32;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 24;
    const std::size_t CHANNELS_COUNT = 3;
    const std::size_t SAMPLES_COUNT = 16;//15 really

    const std::size_t SAMPLES_ADDR = 0xc000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Pattern
    {
      PACK_PRE struct Line
      {
        PACK_PRE struct Channel
        {
          uint8_t NoteCommand;
          uint8_t SampleParam;
          uint8_t Effect;
        } PACK_POST;

        Channel Channels[CHANNELS_COUNT];
      } PACK_POST;

      Line Lines[1];//at least 1
    } PACK_POST;

    PACK_PRE struct SampleInfo
    {
      uint8_t Name[9];
      ruint16_t Start;
      uint8_t Bank;
      ruint16_t Limit;
      ruint16_t Loop;
    } PACK_POST;

    PACK_PRE struct MixedLine
    {
      Pattern::Line::Channel Mixin;
      uint8_t Period;
    } PACK_POST;

    PACK_PRE struct Header
    {
      //+0
      ruint16_t EndOfBanks[END_OF_BANKS_SIZE];
      //+0x0c
      uint8_t PatternSize;
      //+0x0d
      uint8_t Padding1;
      //+0x0e
      uint8_t Positions[0x32];
      //+0x40
      uint8_t Tempo;
      //+0x41
      uint8_t Loop;
      //+0x42
      uint8_t Padding2;
      //+0x43
      uint8_t Length;
      //+0x44
      uint8_t HeaderSizeSectors;
      //+0x45
      MixedLine Mixings[5];
      //+0x59
      uint8_t Padding3;
      //+0x5a
      SampleInfo SampleDescriptions[SAMPLES_COUNT];
      //+0x15a
      uint8_t Padding4[4];
      //+0x15e
      //patterns starts here
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(MixedLine) == 4);
    BOOST_STATIC_ASSERT(sizeof(SampleInfo) == 16);
    BOOST_STATIC_ASSERT(sizeof(Header) == 0x15e);
    BOOST_STATIC_ASSERT(sizeof(Pattern::Line) == 9);

    const std::size_t MODULE_SIZE = sizeof(Header);

    enum
    {
      NOTE_BASE = 1,
      NO_DATA = 70,
      REST_NOTE = 61,
      SET_TEMPO = 62,
      SET_FREQ_FLOAT = 63,
      SET_VIBRATO = 64,
      SET_ARPEGGIO = 65,
      SET_SLIDE = 66,
      SET_DOUBLE = 67,
      SET_ATTACK = 68,
      SET_DECAY = 69,

      FX_FLOAT_UP = 1,
      FX_FLOAT_DN = 2,
      FX_VIBRATO = 3,
      FX_ARPEGGIO = 4,
      FX_STEP_UP = 5,
      FX_STEP_DN = 6,
      FX_DOUBLE = 7,
      FX_ATTACK = 8,
      FX_DECAY = 9,
      FX_MIX = 10,
      FX_DISABLE = 15
    };

    class StubChannelBuilder : public ChannelBuilder
    {
    public:
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetVolume(uint_t /*volume*/) {}

      virtual void SetFloat(int_t /*direction*/) {}
      virtual void SetFloatParam(uint_t /*step*/) {}
      virtual void SetVibrato() {}
      virtual void SetVibratoParams(uint_t /*step*/, uint_t /*period*/) {}
      virtual void SetArpeggio() {}
      virtual void SetArpeggioParams(uint_t /*step*/, uint_t /*period*/) {}
      virtual void SetSlide(int_t /*direction*/) {}
      virtual void SetSlideParams(uint_t /*step*/, uint_t /*period*/) {}
      virtual void SetDoubleNote() {}
      virtual void SetDoubleNoteParam(uint_t /*period*/) {}
      virtual void SetVolumeAttack() {}
      virtual void SetVolumeAttackParams(uint_t /*limit*/, uint_t /*period*/) {}
      virtual void SetVolumeDecay() {}
      virtual void SetVolumeDecayParams(uint_t /*limit*/, uint_t /*period*/) {}
      virtual void SetMixSample(uint_t /*idx*/) {}
      virtual void SetNoEffects() {}
    };

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::Data::Ptr /*sample*/) {}
      virtual std::auto_ptr<ChannelBuilder> SetSampleMixin(uint_t /*index*/, uint_t /*period*/)
      {
        return std::auto_ptr<ChannelBuilder>(new StubChannelBuilder());
      }
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}
      virtual PatternBuilder& StartPattern(uint_t /*index*/)
      {
        return GetStubPatternBuilder();
      }
      virtual std::auto_ptr<ChannelBuilder> StartChannel(uint_t /*index*/)
      {
        return std::auto_ptr<ChannelBuilder>(new StubChannelBuilder());
      }
    };

    //Do not collect samples info due to high complexity of intermediate layers
    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, PATTERNS_COUNT - 1)
      {
      }

      virtual MetaBuilder& GetMetaBuilder()
      {
        return Delegate.GetMetaBuilder();
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        return Delegate.SetInitialTempo(tempo);
      }

      virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr data)
      {
        return Delegate.SetSample(index, loop, data);
      }

      virtual std::auto_ptr<ChannelBuilder> SetSampleMixin(uint_t index, uint_t period)
      {
        return Delegate.SetSampleMixin(index, period);
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        UsedPatterns.Assign(positions.begin(), positions.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(positions, loop);
      }

      virtual PatternBuilder& StartPattern(uint_t index)
      {
        return Delegate.StartPattern(index);
      }

      virtual std::auto_ptr<ChannelBuilder> StartChannel(uint_t index)
      {
        return Delegate.StartChannel(index);
      }

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*static_cast<const Header*>(RawData.Start()))
        , Ranges(RangeChecker::Create(RawData.Size()))
        , FixedRanges(RangeChecker::Create(RawData.Size()))
      {
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetProgram(Text::DIGITALMUSICMAKER_DECODER_DESCRIPTION);
      }

      void ParsePositions(Builder& target) const
      {
 //       const std::vector<uint_t> positions(Source.Positions.begin(), Source.Positions.begin() + Source.Length + 1);
        std::vector<uint_t> positions(Source.Length + 1);
		for(int i= 0; i<Source.Length+1; i++) {
			positions.at(i)= Source.Positions[i];
		}		
		
        target.SetPositions(positions, Source.Loop);
        Dbg("Positions: %1%, loop to %2%", positions.size(), unsigned(Source.Loop));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          ParsePattern(patIndex, target);
        }
      }

      void ParseMixins(Builder& target) const
      {
        //big mixins amount support
        for (std::size_t mixIdx = 0; mixIdx != 64; ++mixIdx)
        {
          const MixedLine& src = Source.Mixings[mixIdx];
          std::auto_ptr<ChannelBuilder> dst = target.SetSampleMixin(mixIdx, src.Period);
          ParseChannel(src.Mixin, *dst);
        }
      }

      void ParseSamples(Builder& target) const
      {
        const bool is4bitSamples = true;//TODO: detect
        const std::size_t limit = RawData.Size();
        Binary::Container::Ptr regions[8];
        for (std::size_t layIdx = 0, lastData = 256 * Source.HeaderSizeSectors; layIdx != END_OF_BANKS_SIZE; ++layIdx)
        {
          static const uint_t BANKS[] = {0, 1, 3, 4, 6, 7};

          const uint_t bankNum = BANKS[layIdx];
          const std::size_t bankEnd = fromLE(Source.EndOfBanks[layIdx]);
          Require(bankEnd >= SAMPLES_ADDR);
          if (bankEnd == SAMPLES_ADDR)
          {
            Dbg("Skipping empty bank #%1$02x (end=#%2$04x)", bankNum, bankEnd);
            continue;
          }
          const std::size_t bankSize = bankEnd - SAMPLES_ADDR;
          const std::size_t alignedBankSize = Math::Align<std::size_t>(bankSize, 256);
          if (is4bitSamples)
          {
            const std::size_t realSize = 256 * (1 + alignedBankSize / 512);
            Require(lastData + realSize <= limit);
            regions[bankNum] = RawData.GetSubcontainer(lastData, realSize);
            Dbg("Added unpacked bank #%1$02x (end=#%2$04x, size=#%3$04x) offset=#%4$05x", bankNum, bankEnd, realSize, lastData);
            AddRange(lastData, realSize);
            lastData += realSize;
          }
          else
          {
            Require(lastData + alignedBankSize <= limit);
            regions[bankNum] = RawData.GetSubcontainer(lastData, alignedBankSize);
            Dbg("Added bank #%1$02x (end=#%2$04x, size=#%3$04x) offset=#%4$05x", bankNum, bankEnd, alignedBankSize, lastData);
            AddRange(lastData, alignedBankSize);
            lastData += alignedBankSize;
          }
        }
        
        for (uint_t samIdx = 1; samIdx != SAMPLES_COUNT; ++samIdx)
        {
          const SampleInfo& srcSample = Source.SampleDescriptions[samIdx - 1];
          if (srcSample.Name[0] == '.')
          {
            Dbg("No sample %1%", samIdx);
            continue;
          }
          const std::size_t sampleStart = fromLE(srcSample.Start);
          const std::size_t sampleEnd = fromLE(srcSample.Limit);
          const std::size_t sampleLoop = fromLE(srcSample.Loop);
          Dbg("Processing sample %1% (bank #%2$02x #%3$04x..#%4$04x loop #%5$04x)", samIdx, uint_t(srcSample.Bank), sampleStart, sampleEnd, sampleLoop);
          Require(sampleStart >= SAMPLES_ADDR && sampleStart <= sampleEnd && sampleStart <= sampleLoop);
          Require((srcSample.Bank & 0xf8) == 0x50);
          const uint_t bankIdx = srcSample.Bank & 0x07;
          const Binary::Container::Ptr bankData = regions[bankIdx];
          Require(bankData != 0);
          const std::size_t offsetInBank = sampleStart - SAMPLES_ADDR;
          const std::size_t limitInBank = sampleEnd - SAMPLES_ADDR;
          const std::size_t sampleSize = limitInBank - offsetInBank;
          const uint_t multiplier = is4bitSamples ? 2 : 1;
          Require(limitInBank <= multiplier * bankData->Size());
          const std::size_t realSampleSize = sampleSize >= 12 ? (sampleSize - 12) : sampleSize;
          if (const Binary::Data::Ptr content = bankData->GetSubcontainer(offsetInBank / multiplier, realSampleSize / multiplier))
          {
            const std::size_t loop = sampleLoop - sampleStart;
            target.SetSample(samIdx, loop, content);
          }
        }
      }

      std::size_t GetSize() const
      {
        return Ranges->GetAffectedRange().second;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return FixedRanges->GetAffectedRange();
      }
    private:
      void ParsePattern(uint_t idx, Builder& target) const
      {
        const uint_t patternSize = Source.PatternSize;
        const std::size_t patStart = sizeof(Source) + sizeof(Pattern::Line) * idx * patternSize;
        const std::size_t limit = RawData.Size();
        Require(patStart < limit);
        const uint_t availLines = (limit - patStart) / sizeof(Pattern::Line);
        PatternBuilder& patBuilder = target.StartPattern(idx);
        const Pattern& src = *safe_ptr_cast<const Pattern*>(static_cast<const uint8_t*>(RawData.Start()) + patStart);
        uint_t lineNum = 0;
        for (const uint_t lines = std::min(availLines, patternSize); lineNum < lines; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          patBuilder.StartLine(lineNum);
          ParseLine(srcLine, patBuilder, target);
        }
        const std::size_t patSize = lineNum * sizeof(Pattern::Line);
        AddFixedRange(patStart, patSize);
      }

      static void ParseLine(const Pattern::Line& line, PatternBuilder& patBuilder, Builder& target)
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          const Pattern::Line::Channel& srcChan = line.Channels[chanNum];
          std::auto_ptr<ChannelBuilder> dstChan = target.StartChannel(chanNum);
          ParseChannel(srcChan, *dstChan);
          if (srcChan.NoteCommand == SET_TEMPO && srcChan.SampleParam)
          {
            patBuilder.SetTempo(srcChan.SampleParam);
          }
        }
      }

      static void ParseChannel(const Pattern::Line::Channel& srcChan, ChannelBuilder& dstChan)
      {
        const uint_t note = srcChan.NoteCommand;
        if (NO_DATA == note)
        {
          return;
        }
        if (note < SET_TEMPO)
        {
          if (note)
          {
            if (note != REST_NOTE)
            {
              dstChan.SetNote(note - NOTE_BASE);
            }
            else
            {
              dstChan.SetRest();
            }
          }
          const uint_t params = srcChan.SampleParam;
          if (const uint_t sample = params >> 4)
          {
            dstChan.SetSample(sample);
          }
          if (const uint_t volume = params & 15)
          {
            dstChan.SetVolume(volume);
          }
          switch (srcChan.Effect)
          {
          case 0:
            break;
          case FX_FLOAT_UP:
            dstChan.SetFloat(1);
            break;
          case FX_FLOAT_DN:
            dstChan.SetFloat(-1);
            break;
          case FX_VIBRATO:
            dstChan.SetVibrato();
            break;
          case FX_ARPEGGIO:
            dstChan.SetArpeggio();
            break;
          case FX_STEP_UP:
            dstChan.SetSlide(1);
            break;
          case FX_STEP_DN:
            dstChan.SetSlide(-1);
            break;
          case FX_DOUBLE:
            dstChan.SetDoubleNote();
            break;
          case FX_ATTACK:
            dstChan.SetVolumeAttack();
            break;
          case FX_DECAY:
            dstChan.SetVolumeDecay();
            break;
          case FX_DISABLE:
            dstChan.SetNoEffects();
            break;
          default:
            {
              const uint_t mixNum = srcChan.Effect - FX_MIX;
              //according to player there can be up to 64 mixins (with enabled 4)
              dstChan.SetMixSample(mixNum % 64);
            }
            break; 
          }
        }
        else
        {
          switch (note)
          {
          case SET_TEMPO:
            break;
          case SET_FREQ_FLOAT:
            dstChan.SetFloatParam(srcChan.SampleParam);
            break;
          case SET_VIBRATO:
            dstChan.SetVibratoParams(srcChan.SampleParam, srcChan.Effect);
            break;
          case SET_ARPEGGIO:
            dstChan.SetArpeggioParams(srcChan.SampleParam, srcChan.Effect);
            break;
          case SET_SLIDE:
            dstChan.SetSlideParams(srcChan.SampleParam, srcChan.Effect);
            break;
          case SET_DOUBLE:
            dstChan.SetDoubleNoteParam(srcChan.SampleParam);
            break;
          case SET_ATTACK:
            dstChan.SetVolumeAttackParams(srcChan.SampleParam & 15, srcChan.Effect);
            break;
          case SET_DECAY:
            dstChan.SetVolumeDecayParams(srcChan.SampleParam & 15, srcChan.Effect);
            break;
          }
        }
      }

      void AddRange(std::size_t start, std::size_t size) const
      {
        Require(Ranges->AddRange(start, size));
      }

      void AddFixedRange(std::size_t start, std::size_t size) const
      {
        Require(FixedRanges->AddRange(start, size));
        Require(Ranges->AddRange(start, size));
      }
    private:
      const Binary::Container& RawData;
      const Header& Source;
      const RangeChecker::Ptr Ranges;
      const RangeChecker::Ptr FixedRanges;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size(rawData.Size());
      if (sizeof(Header) > size)
      {
        return false;
      }
      const Header& header = *static_cast<const Header*>(rawData.Start());
      if (!(header.PatternSize == 64 || header.PatternSize == 48 || header.PatternSize == 32 || header.PatternSize == 24))
      {
        return false;
      }
      return true;
    }

    const std::string FORMAT(
      //bank ends
      "(?c0-ff){6}"
      //pat size: 64,48,32,24
      "%0xxxx000 ?"
      //positions
      "(00-17){50}"
      //tempo (3..30)
      "03-1e"
      //loop position
      "00-32 ?"
      //length
      "01-32"
      //base size
      "02-38"
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MODULE_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::DIGITALMUSICMAKER_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return FastCheck(rawData) && Format->Match(rawData);
      }

      virtual Formats::Chiptune::Container::Ptr Decode(const Binary::Container& rawData) const
      {
        if (!Format->Match(rawData))
        {
          return Formats::Chiptune::Container::Ptr();
        }
        Builder& stub = GetStubBuilder();
        return Parse(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target)
    {
      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectionBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        format.ParseMixins(target);
        format.ParseSamples(target);

        const Binary::Container::Ptr subData = data.GetSubcontainer(0, format.GetSize());
        const RangeChecker::Range fixedRange = format.GetFixedArea();
        return CreateCalculatingCrcContainer(subData, fixedRange.first, fixedRange.second - fixedRange.first);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }

    Builder& GetStubBuilder()
    {
      static StubBuilder stub;
      return stub;
    }
  }//namespace DigitalMusicMaker

  Decoder::Ptr CreateDigitalMusicMakerDecoder()
  {
    return boost::make_shared<DigitalMusicMaker::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
