/**
* 
* @file
*
* @brief  DigitalStudio support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "digitalstudio.h"
#include "digital_detail.h"
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
  const Debug::Stream Dbg("Formats::Chiptune::DigitalStudio");
}

namespace Formats
{
namespace Chiptune
{
  namespace DigitalStudio
  {
    const std::size_t COMPILED_MODULE_SIZE = 0x1c200;
    const std::size_t MODULE_SIZE = 0x1b200;
    const std::size_t MIN_POSITIONS_COUNT = 1;
    const std::size_t MAX_POSITIONS_COUNT = 99;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t PATTERNS_COUNT = 32;
    const std::size_t CHANNELS_COUNT = 3;
    const std::size_t SAMPLES_COUNT = 16;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct Pattern
    {
      PACK_PRE struct Line
      {
        PACK_PRE struct Channel
        {
          uint8_t Note;
          uint8_t Sample;
        } PACK_POST;

        Channel Channels[CHANNELS_COUNT];
      } PACK_POST;

      Line Lines[MAX_PATTERN_SIZE];
    } PACK_POST;

    PACK_PRE struct SampleInfo
    {
      ruint16_t Start;
      ruint16_t Loop;
      uint8_t Page;
      uint8_t NumberInBank;
      ruint16_t Size;
      char Name[8];
    } PACK_POST;

    typedef boost::array<uint8_t, 0x38> ZeroesArray;

    PACK_PRE struct Header
    {
      //+0
      uint8_t Loop;
      //+1
      uint8_t Positions[MAX_POSITIONS_COUNT];
      //+0x64
      uint8_t Tempo;
      //+0x65
      uint8_t Length;
      //+0x66
      char Title[28];
      //+0x82
      uint8_t Unknown[0x46];
      //+0xc8
      ZeroesArray Zeroes;
      //+0x100
      SampleInfo Samples[SAMPLES_COUNT];
      //+0x200
      uint8_t FirstPage[0x4000];
      //+0x4200
      Pattern Patterns[PATTERNS_COUNT];
      //0x7200
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(Header) == 0x7200);

    const std::size_t MIN_SIZE = sizeof(Header);

    const uint_t NOTE_EMPTY = 0;
    const uint_t NOTE_BASE = 1;
    const uint_t NOTE_PAUSE = 0x80;
    const uint_t NOTE_SPEED = 0x81;
    const uint_t NOTE_END = 0x82;

    class SamplesSet
    {
    public:
      void Add(uint_t idx, std::size_t loop, Binary::Data::Ptr data)
      {
        const bool is4bit = CheckIfSample4Bit(static_cast<const uint8_t*>(data->Start()), data->Size());
        Dbg(" size #%1$05x, loop #%2$04x%3%", data->Size(), loop, is4bit ? " 4bit" : "");
        Samples.push_back(Description(idx, loop, data, is4bit));
      }

      bool Is4Bit() const
      {
        const std::size_t specific = std::count_if(Samples.begin(), Samples.end(), boost::mem_fn(&Description::Is4Bit));
        Dbg("%1% 4-bit samples out of %2%", specific, Samples.size());
        return specific >= Samples.size() / 2;
      }

      void Apply(Builder& builder)
      {
        const bool is4Bit = Is4Bit();
        for (std::vector<Description>::const_iterator it = Samples.begin(), lim = Samples.end(); it != lim; ++it)
        {
          builder.SetSample(it->Index, it->Loop, it->Content, is4Bit && it->Is4Bit);
        }
      }
    private:
      static bool CheckIfSample4Bit(const uint8_t* start, std::size_t size)
      {
        const std::size_t specific = std::count_if(start, start + size, &Is4BitSample);
        return specific >= size / 2;
      }

      static bool Is4BitSample(uint8_t val)
      {
        return (val & 0xf0) == 0xa0;
      }
      
      struct Description
      {
        uint_t Index;
        std::size_t Loop;
        Binary::Data::Ptr Content;
        bool Is4Bit;

        Description()
          : Index()
          , Loop()
          , Content()
          , Is4Bit()
        {
        }

        Description(uint_t idx, std::size_t loop, Binary::Data::Ptr content, bool is4bit)
          : Index(idx)
          , Loop(loop)
          , Content(content)
          , Is4Bit(is4bit)
        {
        }
      };
      std::vector<Description> Samples;
    };

    //TODO: extract
    Binary::Data::Ptr CreateCompositeData(const Binary::Data& lh, const Binary::Data& rh)
    {
      const std::size_t size1 = lh.Size();
      const std::size_t size2 = rh.Size();
      std::auto_ptr<Dump> res(new Dump(size1 + size2));
      std::memcpy(&res->front(), lh.Start(), size1);
      std::memcpy(&res->front() + size1, rh.Start(), size2);
      return Binary::CreateContainer(res);
    }

    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*static_cast<const Header*>(RawData.Start()))
        , IsCompiled(Source.Zeroes != ZeroesArray())
        , Ranges(RangeChecker::Create(GetSize()))
      {
        Require(GetSize() <= RawData.Size());
        //info
        Require(Ranges->AddRange(0, 0x200));
        Require(Ranges->AddRange(offsetof(Header, Patterns), IsCompiled ? 0x4000 : 0x3000));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        target.GetMetaBuilder().SetTitle(FromCharArray(Source.Title));
      }

      void ParsePositions(Builder& target) const
      {
        Require(Math::InRange<std::size_t>(Source.Length, MIN_POSITIONS_COUNT, MAX_POSITIONS_COUNT));
 //       const std::vector<uint_t> positions(Source.Positions.begin(), Source.Positions.begin() + Source.Length);
        std::vector<uint_t> positions(Source.Length);
		for (int i= 0; i<Source.Length; i++) {
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
          PatternBuilder& patBuilder = target.StartPattern(patIndex);
          ParsePattern(Source.Patterns[patIndex], patBuilder, target);
        }
      }

      void ParseSamples(const Indices& sams, SamplesSet& samples) const
      {
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const SampleInfo& info = Source.Samples[samIdx];
          Dbg("Sample %1%: start=#%2$04x loop=#%3$04x page=#%4$02x size=#%5$04x", 
            samIdx, fromLE(info.Start), fromLE(info.Loop), unsigned(info.Page), fromLE(info.Size));
          std::size_t loop = 0;
          if (const Binary::Data::Ptr sam = ParseSample(info, loop))
          {
            samples.Add(samIdx, loop, sam);
          }
          else
          {
            Dbg(" Stub sample");
            const uint8_t dummy = 128;
            samples.Add(samIdx, 0, Binary::CreateContainer(&dummy, 1));
          }
        }
      }

      std::size_t GetSize() const
      {
        return IsCompiled ? COMPILED_MODULE_SIZE : MODULE_SIZE;
      }
    private:
      //truncate samples here due to possible samples overlap in descriptions
      Binary::Data::Ptr GetSampleData(std::size_t offset, std::size_t size) const
      {
        const Binary::Data::Ptr total = RawData.GetSubcontainer(offset, size);
        const uint8_t* const start = static_cast<const uint8_t*>(total->Start());
        const uint8_t* const end = start + total->Size();
        const uint8_t* const sampleEnd = std::find(start, end, 0xff);
        if (const std::size_t newSize = sampleEnd - start)
        {
          Require(Ranges->AddRange(offset, newSize));
          return RawData.GetSubcontainer(offset, newSize);
        }
        else
        {
          return Binary::Data::Ptr();
        }
      }

      static void ParsePattern(const Pattern& src, PatternBuilder& patBuilder, Builder& target)
      {
        const uint_t lastLine = MAX_PATTERN_SIZE - 1;
        for (uint_t lineNum = 0; lineNum <= lastLine ; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          if (lineNum != lastLine && IsEmptyLine(srcLine))
          {
            continue;
          }
          patBuilder.StartLine(lineNum);
          if (!ParseLine(srcLine, patBuilder, target))
          {
            patBuilder.Finish(lineNum + 1);
            break;
          }
        }
      }

      static bool ParseLine(const Pattern::Line& srcLine, PatternBuilder& patBuilder, Builder& target)
      {
        bool result = true;
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          const Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
          const uint_t note = srcChan.Note;
          if (NOTE_EMPTY == note)
          {
            continue;
          }
          target.StartChannel(chanNum);
          if (note >= NOTE_PAUSE)
          {
            Require((note & 0xf0) == (NOTE_PAUSE & 0xf0));
            if (NOTE_PAUSE == note)
            {
              target.SetRest();
            }
            else if (NOTE_SPEED == note)
            {
              patBuilder.SetTempo(srcChan.Sample);
            }
            else if (NOTE_END == note)
            {
              result = false;
            }
          }
          else
          {
            target.SetNote(note - NOTE_BASE);
            target.SetSample(srcChan.Sample);
          }
        }
        return result;
      }

      static bool IsEmptyLine(const Pattern::Line& srcLine)
      {
        return srcLine.Channels[0].Note == NOTE_EMPTY
            && srcLine.Channels[1].Note == NOTE_EMPTY
            && srcLine.Channels[2].Note == NOTE_EMPTY;
      }

      Binary::Data::Ptr ParseSample(const SampleInfo& info, std::size_t& loop) const
      {
        const std::size_t ZX_PAGE_SIZE = 0x4000;
        const std::size_t LO_MEM_ADDR = 0x8000;
        const std::size_t HI_MEM_ADDR = 0xc000;
        static const std::size_t SAMPLES_OFFSETS[8] = {0x200, 0x7200, 0x0, 0xb200, 0xf200, 0x0, 0x13200, 0x17200};
        static const std::size_t SAMPLES_OFFSETS_COMPILED[8] = {0x200, 0x8200, 0x0, 0xc200, 0x10200, 0x0, 0x14200, 0x18200};

        if (info.Size == 0)
        {
          return Binary::Data::Ptr();
        }
        Require(info.Page >= 0x51 && info.Page <= 0x57);

        const std::size_t* const offsets = IsCompiled ? SAMPLES_OFFSETS_COMPILED : SAMPLES_OFFSETS;

        const std::size_t pageNumber = info.Page & 0x7;
        Require(0 != offsets[pageNumber]);

        const bool isLoMemSample = 0x7 == pageNumber;
        const std::size_t BASE_ADDR = isLoMemSample ? LO_MEM_ADDR : HI_MEM_ADDR;
        const std::size_t MAX_SIZE = isLoMemSample ? 2 * ZX_PAGE_SIZE : ZX_PAGE_SIZE;

        const std::size_t sampleStart = fromLE(info.Start);
        const std::size_t sampleSize = fromLE(info.Size);
        const std::size_t sampleLoop = fromLE(info.Loop);

        Require(sampleStart >= BASE_ADDR);
        Require(sampleSize <= MAX_SIZE);
        Require(Math::InRange(sampleLoop, sampleStart, sampleStart + sampleSize));

        loop = sampleLoop - sampleStart;
        const std::size_t sampleOffsetInPage = sampleStart - BASE_ADDR;
        if (isLoMemSample)
        {
          const bool isLoMemSampleInPage = sampleOffsetInPage >= ZX_PAGE_SIZE;
          const bool isSplitSample = !isLoMemSampleInPage && sampleOffsetInPage + sampleSize > ZX_PAGE_SIZE;

          if (isSplitSample)
          {
            const std::size_t firstOffset = offsets[0] + sampleOffsetInPage;
            const std::size_t firstCopy = ZX_PAGE_SIZE - sampleOffsetInPage;
            const Binary::Data::Ptr part1 = GetSampleData(firstOffset, firstCopy);
            Require(part1 != 0);
            const std::size_t secondOffset = offsets[7];
            const std::size_t secondCopy = sampleOffsetInPage + sampleSize - ZX_PAGE_SIZE;
            Dbg(" Two parts in low memory: #%1$05x..#%2$05x + #%3$05x..#%4$05x", 
              firstOffset, firstOffset + firstCopy, secondOffset, secondOffset + secondCopy);
            if (const Binary::Data::Ptr part2 = GetSampleData(secondOffset, secondCopy))
            {
              Dbg(" Using two parts with sizes #%1$05x + #%2$05x", part1->Size(), part2->Size());
              return CreateCompositeData(*part1, *part2);
            }
            else
            {
              Dbg(" Using first part");
              return part1;
            }
          }
          else
          {
            const std::size_t dataOffset = isLoMemSampleInPage 
              ? (offsets[7] + (sampleOffsetInPage - ZX_PAGE_SIZE))
              : offsets[0] + sampleOffsetInPage;
            Dbg(" One part in low memory: #%1$05x..#%2$05x", 
              dataOffset, dataOffset + sampleSize);
            return GetSampleData(dataOffset, sampleSize);
          }
        }
        else
        {
          const std::size_t dataOffset = offsets[pageNumber] + sampleOffsetInPage;
          Dbg(" Hi memory: #%1$05x..#%2$05x", 
            dataOffset, dataOffset + sampleSize);
          return GetSampleData(dataOffset, sampleSize);
        }
      }
    private:
      const Binary::Container& RawData;
      const Header& Source;
      const bool IsCompiled;
      const RangeChecker::Ptr Ranges;
    };

    bool FastCheck(const Binary::Container& rawData)
    {
      //at least
      return rawData.Size() >= MODULE_SIZE;
    }

    const std::string FORMAT(
      "00-63"     //loop
      "00-1f{99}" //positions
      "02-0f"     //tempo
      "01-64"     //length
      "20-7f{28}" //title
      //skip compiled
      "?{44}"
      "ff{10}"
      "????????"//"ae7eae7e51000000"
      "20{8}"
    );

    const uint64_t Z80_FREQ = 3500000;
    // step is not changed in AY and SounDrive versions
    const uint_t C_1_STEP = 88;
    const uint_t SD_TICKS_PER_CYCLE = 356;
    const uint_t AY_TICKS_PER_CYCLE = 344;

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::DIGITALSTUDIO_DECODER_DESCRIPTION;
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
        SamplesSet samples;
        format.ParseSamples(usedSamples, samples);

        Require(format.GetSize() >= MIN_SIZE);
        const uint_t cycleTicks = samples.Is4Bit() ? AY_TICKS_PER_CYCLE : SD_TICKS_PER_CYCLE;
        target.SetSamplesFrequency(Z80_FREQ * C_1_STEP / cycleTicks / 256);

        const String program = String(Text::DIGITALSTUDIO_DECODER_DESCRIPTION) + 
          (samples.Is4Bit() ? Text::DIGITALSTUDIO_VERSION_AY : Text::DIGITALSTUDIO_VERSION_DAC);
        target.GetMetaBuilder().SetProgram(program);
        samples.Apply(target);
        const Binary::Container::Ptr subData = data.GetSubcontainer(0, format.GetSize());
        const std::size_t patternsOffset = offsetof(Header, Patterns);
        return CreateCalculatingCrcContainer(subData, patternsOffset, format.GetSize() - patternsOffset);
      }
      catch (const std::exception&)
      {
        Dbg("Failed to create");
        return Formats::Chiptune::Container::Ptr();
      }
    }
  }//namespace DigitalStudio

  Decoder::Ptr CreateDigitalStudioDecoder()
  {
    return boost::make_shared<DigitalStudio::Decoder>();
  }
} //namespace Chiptune
} //namespace Formats
