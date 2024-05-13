/**
* 
* @file
*
* @brief  SampleTracker support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "digital_detail.h"
#include "sampletracker.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
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
#include <boost/mem_fn.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::SampleTracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace SampleTracker
  {
    const std::size_t MAX_MODULE_SIZE = 0x87a0;
    const std::size_t MAX_POSITIONS_COUNT = 0x40;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 16;
    const std::size_t CHANNELS_COUNT = 3;
    const std::size_t SAMPLES_COUNT = 16;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Pattern
    {
      static const uint8_t LIMIT = 0xff;

      PACK_PRE struct Line
      {
        uint8_t Note[CHANNELS_COUNT];
        uint8_t Sample[CHANNELS_COUNT];
      } PACK_POST;

      Line Lines[MAX_PATTERN_SIZE];
      uint8_t Limit;
    } PACK_POST;

    PACK_PRE struct SampleInfo
    {
      uint8_t AddrHi;
      uint8_t SizeHiDoubled;
    } PACK_POST;

    PACK_PRE struct Header
    {
      //+0
      uint8_t Tempo;
      //+1
      uint8_t Positions[MAX_POSITIONS_COUNT];
      //+0x41
      ruint16_t PositionsPtrs[MAX_POSITIONS_COUNT];
      //+0xc1
      char Title[10];
      //+0xcb
      uint8_t LastPositionDoubled;
      //+0xcc
      Pattern Patterns[PATTERNS_COUNT];
      //+0x18dc
      SampleInfo SampleDescriptions[SAMPLES_COUNT];
      //+0x18fc
      uint8_t Padding[4];
      //+0x1900
      uint8_t SampleNames[10][SAMPLES_COUNT];
      //+0x19a0
      uint8_t Samples[1];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Header) == 0x19a1);

    const uint_t NOTE_EMPTY = 0;
    const uint_t NOTE_BASE = 1;

    const uint_t SAMPLE_EMPTY = 0;
    const uint_t PAUSE = 0x10;
    const uint_t SAMPLE_BASE = 0x25;

    const uint_t MODULE_BASE = 0x7260;
    const uint_t SAMPLES_ADDR = MODULE_BASE + offsetof(Header, Samples);
    const uint_t SAMPLES_LIMIT_ADDR = 0xfa00;

    const std::size_t MIN_SIZE = sizeof(Header) + 256;

    const uint64_t Z80_FREQ = 3500000;
    //109+113+113+10=345 ticks/out cycle = 10144 outs/sec
    const uint_t TICKS_PER_CYCLE = 109 + 113 + 113 + 10;
    //C-1 step 22/256 32.7Hz = ~871 samples/sec
    const uint_t C_1_STEP = 22;
    const uint_t SAMPLES_FREQ = Z80_FREQ * C_1_STEP / TICKS_PER_CYCLE / 256;

    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*static_cast<const Header*>(RawData.Start()))
        , Ranges(RangeChecker::Create(rawData.Size()))
        , FixedRanges(RangeChecker::Create(rawData.Size()))
      {
        //info
        AddRange(0, offsetof(Header, Samples));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetTitle(FromCharArray(Source.Title));
        meta.SetProgram(Text::SAMPLETRACKER_DECODER_DESCRIPTION);
      }

      void ParsePositions(Builder& target) const
      {
        const uint_t positionsCount = Source.LastPositionDoubled / 2;
        Require(Math::InRange<uint_t>(positionsCount + 1, 1, MAX_POSITIONS_COUNT));

        std::vector<uint_t> positions(positionsCount);	
//        std::transform(Source.Positions.begin(), Source.Positions.begin() + positionsCount, positions.begin(),
 //         std::bind2nd(std::minus<uint8_t>(), uint8_t(1)));
		for (int i= 0; i<positionsCount; i++) {
			positions.at(i)= Source.Positions[i]-1;
		}		  
		  
        target.SetPositions(positions, 0);
        Dbg("Positions: %1%", positions.size());
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        Dbg("Parse %1% patterns", pats.Count());
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          PatternBuilder& patBuilder = target.StartPattern(patIndex);
          ParsePattern(Source.Patterns[patIndex], patBuilder, target);
          AddFixedRange(offsetof(Header, Patterns) + patIndex * sizeof(Pattern), sizeof(Pattern));
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        Dbg("Parse %1% samples", sams.Count());
        target.SetSamplesFrequency(SAMPLES_FREQ);
        std::size_t validSamples = 0;
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          if (const Binary::Data::Ptr content = GetSample(samIdx))
          {
            target.SetSample(samIdx, content->Size(), content, false);
            ++validSamples;
          }
          else
          {
            Dbg(" Stub sample %1%", samIdx);
            const uint8_t dummy = 128;
            target.SetSample(samIdx, 0, Binary::CreateContainer(&dummy, sizeof(dummy)), false);
          }
        }
        if (sams.Maximum() != SAMPLES_COUNT - 1)
        {
          GetSample(SAMPLES_COUNT - 1);
        }
        Require(validSamples != 0);
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
      static void ParsePattern(const Pattern& src, PatternBuilder& patBuilder, Builder& target)
      {
        const uint_t lastLine = MAX_PATTERN_SIZE - 1;
        for (uint_t lineNum = 0; lineNum <= lastLine ; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          if (srcLine.Note[0] == Pattern::LIMIT)
          {
            patBuilder.Finish(lineNum);
            break;
          }

          if (lineNum != lastLine && IsEmptyLine(srcLine))
          {
            continue;
          }
          patBuilder.StartLine(lineNum);
          ParseLine(srcLine, target);
        }
      }

      static void ParseLine(const Pattern::Line& srcLine, Builder& target)
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          target.StartChannel(chanNum);
          const uint_t note = srcLine.Note[chanNum];
          if (NOTE_EMPTY != note)
          {
            target.SetNote(note - NOTE_BASE);
          }
          switch (const uint_t sample = srcLine.Sample[chanNum])
          {
          case PAUSE:
            target.SetRest();
            break;
          case SAMPLE_EMPTY:
            break;
          default:
            if (sample < SAMPLES_COUNT + SAMPLE_BASE)
            {
              target.SetSample(sample - SAMPLE_BASE);
            }
            break;
          }
        }
      }

      static bool IsEmptyLine(const Pattern::Line& srcLine)
      {
        return IsEmptyChannel(0, srcLine)
            && IsEmptyChannel(1, srcLine)
            && IsEmptyChannel(2, srcLine)
        ;
      }

      static bool IsEmptyChannel(uint_t chan, const Pattern::Line& srcLine)
      {
        return srcLine.Note[chan] == NOTE_EMPTY && (srcLine.Sample[chan] == SAMPLE_EMPTY || srcLine.Sample[chan] >= SAMPLE_BASE + SAMPLES_COUNT);
      }

      Binary::Data::Ptr GetSample(uint_t samIdx) const
      {
        const SampleInfo& info = Source.SampleDescriptions[samIdx];
        const std::size_t absAddr = 256 * info.AddrHi;
        const std::size_t maxSize = 128 * info.SizeHiDoubled;
        if (!absAddr || absAddr < SAMPLES_ADDR || absAddr + maxSize > SAMPLES_LIMIT_ADDR)
        {
          return Binary::Data::Ptr();
        }
        const std::size_t sampleOffset = offsetof(Header, Samples) + (absAddr - SAMPLES_ADDR);
        if (sampleOffset >= RawData.Size())
        {
          return Binary::Data::Ptr();
        }
        const std::size_t sampleAvail = std::min(maxSize, RawData.Size() - sampleOffset);
        Dbg("Sample %1%: start=#%2$04x size=#%3$04x (avail=#%4$04x)", 
          samIdx, absAddr, maxSize, sampleAvail);
        const uint8_t* const sampleStart = Source.Samples + (absAddr - SAMPLES_ADDR);
        const uint8_t* const sampleEnd = std::find(sampleStart, sampleStart + sampleAvail, 0);
        const std::size_t sampleSize = sampleEnd - sampleStart;
        AddRange(sampleOffset, sampleAvail);
        return RawData.GetSubcontainer(sampleOffset, sampleSize);
      }

      void AddRange(std::size_t start, std::size_t size) const
      {
        Require(Ranges->AddRange(start, size));
      }

      void AddFixedRange(std::size_t start, std::size_t size) const
      {
        Require(FixedRanges->AddRange(start, size));
      }
    private:
      const Binary::Container& RawData;
      const Header& Source;
      const RangeChecker::Ptr Ranges;
      const RangeChecker::Ptr FixedRanges;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      const std::size_t size = rawData.Size();
      if (size < sizeof(Header))
      {
        return false;
      }
      const Header& header = *static_cast<const Header*>(rawData.Start());
      return header.LastPositionDoubled >= 2
          && header.LastPositionDoubled <= 128
          && 0 == (header.LastPositionDoubled & 1)
      ;
    }

    const std::string FORMAT(
      "01-10" //tempo
      "01-10{64}" //positions
      "?73-8b"  //first position ptr
      "?{126}"  //other ptrs
      "20-7f{10}" //title
      "%xxxxxxx0" //doubled last position
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SAMPLETRACKER_DECODER_DESCRIPTION;
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
        Builder& stub = Digital::GetStubBuilder();
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

        Digital::StatisticCollectionBuilder statistic(target, PATTERNS_COUNT, SAMPLES_COUNT);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);

        Require(format.GetSize() >= MIN_SIZE);
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
  }//namespace SampleTracker

  Decoder::Ptr CreateSampleTrackerDecoder()
  {
    return boost::make_shared<SampleTracker::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
