/**
* 
* @file
*
* @brief  ExtremeTracker v1.xx support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "extremetracker1.h"
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
#include <strings/format.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::ExtremeTracker1");
}

namespace Formats
{
namespace Chiptune
{
  namespace ExtremeTracker1
  {
    const std::size_t CHANNELS_COUNT = 4;
    const std::size_t MAX_POSITIONS_COUNT = 100;
    const std::size_t MAX_SAMPLES_COUNT = 16;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_PATTERN_SIZE = 64;

    const uint64_t Z80_FREQ = 3500000;

    const uint_t TICKS_PER_CYCLE_31 = 450;
    const uint_t C_1_STEP_31 = 0x5f;
    const uint_t SAMPLES_FREQ_31 = Z80_FREQ * C_1_STEP_31 / TICKS_PER_CYCLE_31 / 256;
    
    const uint_t TICKS_PER_CYCLE_32 = 374;
    const uint_t C_1_STEP_32 = 0x50;
    const uint_t SAMPLES_FREQ_32 = Z80_FREQ * C_1_STEP_32 / TICKS_PER_CYCLE_32 / 256;
    
    /*
      Module memory map (as in original editor)
      0x0200 -> 0x7e00@50 (hdr)
      0x4000 -> 0xc000@50 (pat)
      0x3a00 -> 0xc000@51
      0x4000 -> 0xc000@53
      0x4000 -> 0xc000@54
      0x4000 -> 0xc000@56
      0x7c00 -> 0x8400@57
      
      Known versions:
        v1.31 - .m files (0xfc+0xbc=0x1b8 sectors)
      
        v1.32 - VOLUME cmd changed to GLISS, added REST cmd, .D files (same sizes)
       
        v1.41 -
        
        ??? - empty sample changed from 7ebc to 4000
    */
  
#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct SampleInfo
    {
      ruint16_t Start;
      ruint16_t Loop;
      uint8_t Page;
      uint8_t IndexInPage;
      uint8_t Blocks;
      uint8_t Padding;
      char Name[8];
    } PACK_POST;
    
    PACK_PRE struct MemoryDescr
    {
      uint8_t RestBlocks;
      uint8_t Page;
      uint8_t FreeStartBlock;
      uint8_t Samples;
    } PACK_POST;
    
    enum Command
    {
      //real
      NONE,
      VOLUME_OR_GLISS,
      TEMPO,
      REST,
      //synthetic
      VOLUME,
      GLISS
    };

    PACK_PRE struct Pattern
    {
      PACK_PRE struct Line
      {
        PACK_PRE struct Channel
        {
          uint8_t NoteCmd;
          uint8_t SampleParam;
          
          uint_t GetNote() const
          {
            return NoteCmd & 0x3f;
          }
          
          uint_t GetCmd() const
          {
            return NoteCmd >> 6;
          }
          
          uint_t GetSample() const
          {
            return SampleParam & 0x0f;
          }
          
          uint_t GetRawCmdParam() const
          {
            return SampleParam >> 4;
          }
          
          uint_t GetTempo() const
          {
            return GetRawCmdParam();
          }
          
          uint_t GetVolume() const
          {
            return GetRawCmdParam();
          }
          
          int_t GetGliss() const
          {
            const int_t val = SampleParam >> 4;
            return 2 * (val > 8 ? 8 - val : val);
          }
          
          bool IsEmpty() const
          {
            return NoteCmd + SampleParam == 0;
          }
        } PACK_POST;

        bool IsEmpty() const
        {
          return Channels[0].IsEmpty()
              && Channels[1].IsEmpty()
              && Channels[2].IsEmpty()
              && Channels[3].IsEmpty()
          ;
        }
        
        Channel Channels[CHANNELS_COUNT];
      } PACK_POST;

      Line Lines[MAX_PATTERN_SIZE];
    } PACK_POST;
    
    PACK_PRE struct Header
    {
      uint8_t LoopPosition;
      uint8_t Tempo;
      uint8_t Length;
      //+0x03
      char Title[30];
      //+0x21
      uint8_t Unknown1;
      //+0x22
      uint8_t Positions[MAX_POSITIONS_COUNT];
      //+0x86
      uint8_t PatternsSizes[MAX_PATTERNS_COUNT];
      //+0xa6
      uint8_t Unknown2[2];
      //+0xa8
      MemoryDescr Pages[5];
      //+0xbc
      uint8_t EmptySample[0x0f];
      //+0xcb
      uint8_t Signature[44];
      //+0xf7
      uint8_t Zeroes[9];
      //+0x100
      SampleInfo Samples[MAX_SAMPLES_COUNT];
      //+0x200
      Pattern Patterns[MAX_PATTERNS_COUNT];
      //+0x4200
    } PACK_POST;

    BOOST_STATIC_ASSERT(sizeof(Pattern) == 0x200);
    BOOST_STATIC_ASSERT(sizeof(Header) == 0x4200);
    
    const std::size_t MODULE_SIZE = 0x1b800;

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }

      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSamplesFrequency(uint_t /*freq*/) {}
      virtual void SetSample(uint_t /*index*/, std::size_t /*loop*/, Binary::Data::Ptr /*content*/) {}
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}

      virtual PatternBuilder& StartPattern(uint_t /*index*/)
      {
        return GetStubPatternBuilder();
      }

      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetVolume(uint_t /*volume*/) {}
      virtual void SetGliss(int_t /*gliss*/) {}
    };

    class StatisticCollectionBuilder : public Builder
    {
    public:
      explicit StatisticCollectionBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
        , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
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

      virtual void SetSamplesFrequency(uint_t freq)
      {
        return Delegate.SetSamplesFrequency(freq);
      }
      
      virtual void SetSample(uint_t index, std::size_t loop, Binary::Data::Ptr data)
      {
        return Delegate.SetSample(index, loop, data);
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

      virtual void StartChannel(uint_t index)
      {
        return Delegate.StartChannel(index);
      }

      virtual void SetRest()
      {
        return Delegate.SetRest();
      }

      virtual void SetNote(uint_t note)
      {
        return Delegate.SetNote(note);
      }

      virtual void SetSample(uint_t sample)
      {
        UsedSamples.Insert(sample);
        return Delegate.SetSample(sample);
      }

      virtual void SetVolume(uint_t volume)
      {
        return Delegate.SetVolume(volume);
      }

      virtual void SetGliss(int_t gliss)
      {
        return Delegate.SetGliss(gliss);
      }
      
      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

      const Indices& GetUsedSamples() const
      {
        Require(!UsedSamples.Empty());
        return UsedSamples;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
    };
    
    class VersionTraits
    {
    public:
      explicit VersionTraits(const Header& hdr)
        : HasRestCmd()
        , GlissVolParamsMask()
      {
        Analyze(hdr);
      }
      
      String GetEditorString() const
      {
        return IsNewVersion()
          ? Text::EXTREMETRACKER132
          : Text::EXTREMETRACKER131;
      }
      
      uint_t GetSamplesFrequency() const
      {
        return IsNewVersion()
          ? SAMPLES_FREQ_32
          : SAMPLES_FREQ_31;
      }
      
      Command DecodeCommand(uint_t cmd) const
      {
        if (cmd == VOLUME_OR_GLISS)
        {
          return IsNewVersion()
            ? GLISS
            : VOLUME;
        }
        else
        {
          return static_cast<Command>(cmd);
        }
      }
    private:
      void Analyze(const Header& hdr)
      {
        for (uint_t pat = 0; pat != MAX_PATTERNS_COUNT; ++pat)
     //   for (uint_t pat = 0; pat != hdr.Patterns.size(); ++pat)
        {
          Analyze(hdr.Patterns[pat]);
        }
      }

      void Analyze(const Pattern& pat)
      {
        for (uint_t line = 0; line != MAX_PATTERN_SIZE; ++line)
        {
          Analyze(pat.Lines[line]);
        }
      }
      
      void Analyze(const Pattern::Line& line)
      {
        for (uint_t chan = 0; chan != CHANNELS_COUNT; ++chan)
        {
          Analyze(line.Channels[chan]);
        }
      }
      
      void Analyze(const Pattern::Line::Channel& chan)
      {
        switch (chan.GetCmd())
        {
        case REST:
          HasRestCmd = true;
          break;
        case VOLUME_OR_GLISS:
          GlissVolParamsMask |= 1 << chan.GetRawCmdParam();
          break;
        }
      }
      
      bool IsNewVersion() const
      {
        // Only newer versions has R-- command
        // TODO: research about gliss/vol parameters factors
        // VOL-0 is used
        return HasRestCmd;
      }
    private:
      bool HasRestCmd;
      uint_t GlissVolParamsMask;
    };

    class Format
    {
    public:
      explicit Format(const Binary::Container& rawData)
        : RawData(rawData)
        , Source(*static_cast<const Header*>(RawData.Start()))
        , Version(Source)
        , Ranges(RangeChecker::Create(RawData.Size()))
      {
        //info
        AddRange(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& target) const
      {
        target.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = target.GetMetaBuilder();
        meta.SetTitle(FromCharArray(Source.Title));
        meta.SetProgram(Version.GetEditorString());
      }

      void ParsePositions(Builder& target) const
      {
   //     const std::vector<uint_t> positions(Source.Positions.begin(), Source.Positions.begin() + Source.Length);
        std::vector<uint_t> positions(Source.Length);
		for(int i= 0; i<Source.Length; i++) {
			positions.at(i)= Source.Positions[i];
		}
		
        target.SetPositions(positions, Source.LoopPosition);
        Dbg("Positions: %1%, loop to %2%", positions.size(), unsigned(Source.LoopPosition));
      }

      void ParsePatterns(const Indices& pats, Builder& target) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          const Pattern& source = Source.Patterns[patIndex];
          ParsePattern(source, patIndex, target);
        }
      }

      void ParseSamples(const Indices& sams, Builder& target) const
      {
        struct BankLayout
        {
          std::size_t FileOffset;
          std::size_t Addr;
          std::size_t End;
        };

        static const BankLayout LAYOUTS[8] =
        {
          {0, 0, 0},
          {0x4200, 0xc000, 0xfa00},
          {0, 0, 0},
          {0x7c00, 0xc000, 0x10000},
          {0xbc00, 0xc000, 0x10000},
          {0, 0, 0},
          {0xfc00, 0xc000, 0x10000},
          {0x13c00, 0x8400, 0x10000}
        };
        
        target.SetSamplesFrequency(Version.GetSamplesFrequency());
        Dbg("Parse %1% samples", sams.Count());
        for (Indices::Iterator it = sams.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const SampleInfo& info = Source.Samples[samIdx];
          const std::size_t rawAddr = fromLE(info.Start);
          const std::size_t rawLoop = fromLE(info.Loop);
          const std::size_t rawEnd = rawAddr + 256 * info.Blocks;
          const BankLayout bank = LAYOUTS[info.Page & 7];
          if (0 == bank.FileOffset || 0 == info.Blocks
           || rawAddr < bank.Addr || rawEnd > bank.End
           || rawLoop < rawAddr || rawLoop > rawEnd)
          {
            Dbg("Skip sample %1%", samIdx);
            continue;
          }
          const std::size_t size = rawEnd - rawAddr;
          const std::size_t offset = bank.FileOffset + (rawAddr - bank.Addr);
          if (const Binary::Data::Ptr sample = GetSample(offset, size))
          {
            Dbg("Sample %1%: start=#%2$04x loop=#%3$04x size=#%4$04x bank=%5%", 
              samIdx, rawAddr, rawLoop, sample->Size(), uint_t(info.Page));
            const std::size_t loop = rawLoop - bank.Addr;
            target.SetSample(samIdx, loop, sample);
          }
          else
          {
            Dbg("Empty sample %1%", samIdx);
          }
        }
      }

      std::size_t GetSize() const
      {
        return MODULE_SIZE;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return RangeChecker::Range(offsetof(Header, Patterns), sizeof(Source.Patterns));
      }
    private:
      void ParsePattern(const Pattern& src, uint_t patIndex, Builder& target) const
      {
        PatternBuilder& patBuilder = target.StartPattern(patIndex);
        const uint_t lastLine = Source.PatternsSizes[patIndex] - 1;
        for (uint_t lineNum = 0; lineNum <= lastLine; ++lineNum)
        {
          const Pattern::Line& srcLine = src.Lines[lineNum];
          if (lineNum != lastLine && srcLine.IsEmpty())
          {
            continue;
          }
          patBuilder.StartLine(lineNum);
          if (!ParseLine(srcLine, patBuilder, target))
          {
            break;
          }
        }
      }

      bool ParseLine(const Pattern::Line& srcLine, PatternBuilder& patBuilder, Builder& target) const
      {
        for (uint_t chanNum = 0; chanNum != CHANNELS_COUNT; ++chanNum)
        {
          const Pattern::Line::Channel& srcChan = srcLine.Channels[chanNum];
          if (srcChan.IsEmpty())
          {
            continue;
          }
          target.StartChannel(chanNum);
          if (const uint_t note = srcChan.GetNote())
          {
            Require(note <= 48);
            target.SetNote(note - 1);
            target.SetSample(srcChan.GetSample());
          }
          switch (Version.DecodeCommand(srcChan.GetCmd()))
          {
          case NONE:
            break;
          case TEMPO:
            patBuilder.SetTempo(srcChan.GetTempo());
            break;
          case VOLUME:
            target.SetVolume(srcChan.GetVolume());
            break;
          case GLISS:
            target.SetGliss(srcChan.GetGliss());
            break;
          case REST:
            target.SetRest();
            break;
          default:
            Require(!"Unknown command");
            break;
          }
        }
        return true;
      }

      void AddRange(std::size_t start, std::size_t size) const
      {
        Require(Ranges->AddRange(start, size));
      }

      Binary::Data::Ptr GetSample(std::size_t offset, std::size_t size) const
      {
        const uint8_t* const start = static_cast<const uint8_t*>(RawData.Start()) + offset;
        const uint8_t* const end = std::find(start, start + size, 0);
        return RawData.GetSubcontainer(offset, end - start);
      }
    private:
      const Binary::Container& RawData;
      const Header& Source;
      const VersionTraits Version;
      const RangeChecker::Ptr Ranges;
    };

    const std::string FORMAT(
      //loop
      "00-63"
      //tempo
      "03-0f"
      //length
      "01-64"
      //title
      "20-7f{30}"
      //unknown
      "?"
      //positions
      "00-1f{100}"
      //pattern sizes
      "04-40{32}"
      //unknown
      "??"
      //memory
      "(00-7c 51|53|54|56|57 00|84-ff 00-10){5}"
      //unknown
      "?{15}"
      //signature
      "?{44}"
      //zeroes
      "?{9}"
      //samples. Hi addr is usually 7e-ff, but some tracks has another values (40)
      "(?? ?? 51|53|54|56|57 00-10 00-7c ? ?{8}){16}"
      //patterns
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
        return Text::EXTREMETRACKER1_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData);
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
      try
      {
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectionBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);

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
  }//namespace ExtremeTracker1

  Decoder::Ptr CreateExtremeTracker1Decoder()
  {
    return boost::make_shared<ExtremeTracker1::Decoder>();
  }
}
}