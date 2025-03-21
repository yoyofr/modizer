/**
* 
* @file
*
* @brief  ProTracker v1.x support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "protracker1.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <range_checker.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::ProTracker1");
}

namespace Formats
{
namespace Chiptune
{
  namespace ProTracker1
  {
    const std::size_t MIN_SIZE = 256;
    const std::size_t MAX_SIZE = 0x2800;
    const std::size_t MAX_POSITIONS_COUNT = 255;
    const std::size_t MIN_PATTERN_SIZE = 5;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MAX_SAMPLES_COUNT = 16;
    const std::size_t MAX_ORNAMENTS_COUNT = 16;
    const std::size_t MAX_SAMPLE_SIZE = 64;

    /*
      Typical module structure

      Header
      Positions,0xff
      Patterns (6 bytes each)
      Patterns data
      Samples
      Ornaments
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      ruint16_t SamplesOffsets[MAX_SAMPLES_COUNT];
      ruint16_t OrnamentsOffsets[MAX_ORNAMENTS_COUNT];
      ruint16_t PatternsOffset;
      char Name[30];
      uint8_t Positions[1];
    } PACK_POST;

    const uint8_t POS_END_MARKER = 0xff;

    PACK_PRE struct RawObject
    {
      uint8_t Size;
      uint8_t Loop;

      uint_t GetSize() const
      {
        return Size;
      }
    } PACK_POST;

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        //HHHHaaaa
        //NTsnnnnn
        //LLLLLLLL

        //HHHHLLLLLLLL - vibrato
        //s - vibrato sign
        //a - level
        //N - noise off
        //T - tone off
        //n - noise value
        uint8_t LevelHiVibrato;
        uint8_t NoiseAndFlags;
        uint8_t LoVibrato;

        bool GetNoiseMask() const
        {
          return 0 != (NoiseAndFlags & 128);
        }

        bool GetToneMask() const
        {
          return 0 != (NoiseAndFlags & 64);
        }

        uint_t GetNoise() const
        {
          return NoiseAndFlags & 31;
        }

        uint_t GetLevel() const
        {
          return LevelHiVibrato & 15;
        }

        int_t GetVibrato() const
        {
          const int_t val((int_t(LevelHiVibrato & 0xf0) << 4) | LoVibrato);
          return (NoiseAndFlags & 32) ? val : -val;
        }
      } PACK_POST;

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }

      Line GetLine(uint_t idx) const
      {
        const uint8_t* const src = safe_ptr_cast<const uint8_t*>(this + 1);
        //using 8-bit offsets
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        Line res;
        res.LevelHiVibrato = src[offset++];
        res.NoiseAndFlags = src[offset++];
        res.LoVibrato = src[offset++];
        return res;
      }
    } PACK_POST;

    PACK_PRE struct RawOrnament
    {
      typedef int8_t Line;
      Line Data[1];

      std::size_t GetSize() const
      {
        return MAX_SAMPLE_SIZE;
      }

      std::size_t GetUsedSize() const
      {
        return MAX_SAMPLE_SIZE;
      }

      Line GetLine(uint_t idx) const
      {
        return Data[idx];
      }
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      ruint16_t Offsets[3];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 100);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 1);

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }
      virtual void SetInitialTempo(uint_t /*tempo*/) {}
      virtual void SetSample(uint_t /*index*/, const Sample& /*sample*/) {}
      virtual void SetOrnament(uint_t /*index*/, const Ornament& /*ornament*/) {}
      virtual void SetPositions(const std::vector<uint_t>& /*positions*/, uint_t /*loop*/) {}
      virtual PatternBuilder& StartPattern(uint_t /*index*/)
      {
        return GetStubPatternBuilder();
      }
      virtual void StartChannel(uint_t /*index*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetVolume(uint_t /*vol*/) {}
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
      virtual void SetNoEnvelope() {}
    };

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
        , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
        , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
      {
        UsedSamples.Insert(0);
        UsedOrnaments.Insert(0);
      }

      virtual MetaBuilder& GetMetaBuilder()
      {
        return Delegate.GetMetaBuilder();
      }

      virtual void SetInitialTempo(uint_t tempo)
      {
        return Delegate.SetInitialTempo(tempo);
      }

      virtual void SetSample(uint_t index, const Sample& sample)
      {
        assert(index == 0 || UsedSamples.Contain(index));
        return Delegate.SetSample(index, sample);
      }

      virtual void SetOrnament(uint_t index, const Ornament& ornament)
      {
        assert(index == 0 || UsedOrnaments.Contain(index));
        return Delegate.SetOrnament(index, ornament);
      }

      virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
      {
        UsedPatterns.Assign(positions.begin(), positions.end());
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(positions, loop);
      }

      virtual PatternBuilder& StartPattern(uint_t index)
      {
        assert(UsedPatterns.Contain(index));
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

      virtual void SetOrnament(uint_t ornament)
      {
        UsedOrnaments.Insert(ornament);
        return Delegate.SetOrnament(ornament);
      }

      virtual void SetVolume(uint_t vol)
      {
        return Delegate.SetVolume(vol);
      }

      virtual void SetEnvelope(uint_t type, uint_t value)
      {
        return Delegate.SetEnvelope(type, value);
      }

      virtual void SetNoEnvelope()
      {
        return Delegate.SetNoEnvelope();
      }

      const Indices& GetUsedPatterns() const
      {
        return UsedPatterns;
      }

      const Indices& GetUsedSamples() const
      {
        return UsedSamples;
      }

      const Indices& GetUsedOrnaments() const
      {
        return UsedOrnaments;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
      Indices UsedOrnaments;
    };

    class RangesMap
    {
    public:
      explicit RangesMap(std::size_t limit)
        : ServiceRanges(RangeChecker::CreateShared(limit))
        , TotalRanges(RangeChecker::CreateSimple(limit))
        , FixedRanges(RangeChecker::CreateSimple(limit))
      {
      }

      void AddService(std::size_t offset, std::size_t size) const
      {
        Require(ServiceRanges->AddRange(offset, size));
        Add(offset, size);
      }

      void AddFixed(std::size_t offset, std::size_t size) const
      {
        Require(FixedRanges->AddRange(offset, size));
        Add(offset, size);
      }

      void Add(std::size_t offset, std::size_t size) const
      {
        Dbg(" Affected range %1%..%2%", offset, offset + size);
        Require(TotalRanges->AddRange(offset, size));
      }

      std::size_t GetSize() const
      {
        return TotalRanges->GetAffectedRange().second;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return FixedRanges->GetAffectedRange();
      }
    private:
      const RangeChecker::Ptr ServiceRanges;
      const RangeChecker::Ptr TotalRanges;
      const RangeChecker::Ptr FixedRanges;
    };

    std::size_t GetHeaderSize(const Binary::TypedContainer& data)
    {
      if (const RawHeader* hdr = data.GetField<RawHeader>(0))
      {
        const uint8_t* const dataBegin = &hdr->Tempo;
        const uint8_t* const dataEnd = dataBegin + data.GetSize();
        const uint8_t* const lastPosition = std::find(hdr->Positions, dataEnd, POS_END_MARKER);
        if (lastPosition != dataEnd && 
            lastPosition == std::find_if(hdr->Positions, lastPosition, std::bind2nd(std::greater_equal<std::size_t>(), MAX_PATTERNS_COUNT)))
        {
          return lastPosition + 1 - dataBegin;
        }
      }
      return 0;
    }

    void CheckTempo(uint_t tempo)
    {
      Require(tempo >= 2);
    }

    class Format
    {
    public:
      Format(const Binary::TypedContainer& data)
        : Delegate(data)
        , Ranges(data.GetSize())
        , Source(*Delegate.GetField<RawHeader>(0))
      {
        Ranges.AddService(0, GetHeaderSize(Delegate));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        const uint_t tempo = Source.Tempo;
        CheckTempo(tempo);
        builder.SetInitialTempo(tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Text::PROTRACKER1_DECODER_DESCRIPTION);
        meta.SetTitle(FromCharArray(Source.Name));
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<uint_t> positions;
        for (const uint8_t* pos = Source.Positions; *pos != POS_END_MARKER; ++pos)
        {
          const uint_t patNum = *pos;
          positions.push_back(patNum);
        }
        Require(Math::InRange<std::size_t>(positions.size(), 1, MAX_POSITIONS_COUNT));
        const uint_t loop = Source.Loop;
        builder.SetPositions(positions, loop);
        Dbg("Positions: %1% entries, loop to %2% (header length is %3%)", positions.size(), loop, uint_t(Source.Length));
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: %1% to parse", pats.Count());
        const std::size_t minOffset = fromLE(Source.PatternsOffset) + pats.Maximum() * sizeof(RawPattern);
        bool hasValidPatterns = false;
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          if (ParsePattern(patIndex, minOffset, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        Dbg("Samples: %1% to parse", samples.Count());
        bool hasValidSamples = false, hasPartialSamples = false;
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Sample result;
          if (const std::size_t samOffset = fromLE(Source.SamplesOffsets[samIdx]))
          {
            const std::size_t availSize = Delegate.GetSize() - samOffset;
            if (const RawSample* src = Delegate.GetField<RawSample>(samOffset))
            {
              const std::size_t usedSize = src->GetUsedSize();
              if (usedSize <= availSize)
              {
                Dbg("Parse sample %1%", samIdx);
                Ranges.Add(samOffset, usedSize);
                ParseSample(*src, src->GetSize(), result);
                hasValidSamples = true;
              }
              else
              {
                Dbg("Parse partial sample %1%", samIdx);
                Ranges.Add(samOffset, availSize);
                const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawSample::Line);
                ParseSample(*src, availLines, result);
                hasPartialSamples = true;
              }
            }
            else
            {
              Dbg("Stub sample %1%", samIdx);
            }
          }
          else
          {
            Dbg("Parse invalid sample %1%", samIdx);
            const RawSample::Line& invalidLine = *Delegate.GetField<RawSample::Line>(0);
            result.Lines.push_back(ParseSampleLine(invalidLine));
          }
          builder.SetSample(samIdx, result);
        }
        Require(hasValidSamples || hasPartialSamples);
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: %1% to parse", ornaments.Count());
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Ornament result;
          if (const std::size_t ornOffset = fromLE(Source.OrnamentsOffsets[ornIdx]))
          {
            const std::size_t availSize = Delegate.GetSize() - ornOffset;
            if (const RawOrnament* src = Delegate.GetField<RawOrnament>(ornOffset))
            {
              const std::size_t usedSize = src->GetUsedSize();
              if (usedSize <= availSize)
              {
                Dbg("Parse ornament %1%", ornIdx);
                Ranges.Add(ornOffset, usedSize);
                ParseOrnament(*src, src->GetSize(), result);
              }
              else
              {
                Dbg("Parse partial ornament %1%", ornIdx);
                Ranges.Add(ornOffset, availSize);
                const uint_t availLines = (availSize - sizeof(*src)) / sizeof(RawOrnament::Line);
                ParseOrnament(*src, availLines, result);
              }
            }
            else
            {
              Dbg("Stub ornament %1%", ornIdx);
            }
          }
          else
          {
            Dbg("Parse invalid ornament %1%", ornIdx);
            result.Lines.push_back(*Delegate.GetField<RawOrnament::Line>(0));
          }
          builder.SetOrnament(ornIdx, result);
        }
      }

      std::size_t GetSize() const
      {
        return Ranges.GetSize();
      }

      RangeChecker::Range GetFixedArea() const
      {
        return Ranges.GetFixedArea();
      }
    private:
      const RawPattern& GetPattern(uint_t patIdx) const
      {
        const std::size_t patOffset = fromLE(Source.PatternsOffset) + patIdx * sizeof(RawPattern);
        Ranges.AddService(patOffset, sizeof(RawPattern));
        return *Delegate.GetField<RawPattern>(patOffset);
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const uint8_t* const data = Delegate.GetField<uint8_t>(offset);
        Require(data != 0);
        return *data;
      }

      struct DataCursors : public boost::array<std::size_t, 3>
      {
        explicit DataCursors(const RawPattern& src)
        {
		  for (int i= 0; i<size(); i++) {
			at(i)= fromLE(src.Offsets[i]);
		  }		  
        }
      };

      struct ParserState
      {
        struct ChannelState
        {
          std::size_t Offset;
          uint_t Period;
          uint_t Counter;

          ChannelState()
            : Offset()
            , Period()
            , Counter()
          {
          }

          void Skip(uint_t toSkip)
          {
            Counter -= toSkip;
          }


          static bool CompareByCounter(const ChannelState& lh, const ChannelState& rh)
          {
            return lh.Counter < rh.Counter;
          }
        };

        boost::array<ChannelState, 3> Channels;

        explicit ParserState(const DataCursors& src)
          : Channels()
        {
          for (std::size_t idx = 0; idx != src.size(); ++idx)
          {
            Channels[idx].Offset = src[idx];
          }
        }

        uint_t GetMinCounter() const
        {
          return std::min_element(Channels.begin(), Channels.end(), &ChannelState::CompareByCounter)->Counter;
        }

        void SkipLines(uint_t toSkip)
        {
          std::for_each(Channels.begin(), Channels.end(), std::bind2nd(std::mem_fun_ref(&ChannelState::Skip), toSkip));
        }
      };

      bool ParsePattern(uint_t patIndex, std::size_t minOffset, Builder& builder) const
      {
        const RawPattern& pat = GetPattern(patIndex);
        const DataCursors rangesStarts(pat);
        Require(rangesStarts.end() == std::find_if(rangesStarts.begin(), rangesStarts.end(), !boost::bind(&Math::InRange<std::size_t>, _1, minOffset, Delegate.GetSize() - 1)));

        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        ParserState state(rangesStarts);
        uint_t lineIdx = 0;
        for (; lineIdx < MAX_PATTERN_SIZE; ++lineIdx)
        {
          //skip lines if required
          if (const uint_t linesToSkip = state.GetMinCounter())
          {
            state.SkipLines(linesToSkip);
            lineIdx += linesToSkip;
          }
          if (!HasLine(state))
          {
            patBuilder.Finish(std::max<uint_t>(lineIdx, MIN_PATTERN_SIZE));
            break;
          }
          patBuilder.StartLine(lineIdx);
          ParseLine(state, patBuilder, builder);
        }
        for (uint_t chanNum = 0; chanNum != rangesStarts.size(); ++chanNum)
        {
          const std::size_t start = rangesStarts[chanNum];
          if (start >= Delegate.GetSize())
          {
            Dbg("Invalid offset (%1%)", start);
          }
          else
          {
            const std::size_t stop = std::min(Delegate.GetSize(), state.Channels[chanNum].Offset + 1);
            Ranges.AddFixed(start, stop - start);
          }
        }
        return lineIdx >= MIN_PATTERN_SIZE;
      }

      bool HasLine(const ParserState& src) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          const ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter)
          {
            continue;
          }
          if (state.Offset >= Delegate.GetSize())
          {
            return false;
          }
          else if (0 == chan && 0xff == PeekByte(state.Offset))
          {
            return false;
          }
        }
        return true;
      }

      void ParseLine(ParserState& src, PatternBuilder& patBuilder, Builder& builder) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter--)
          {
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(state, patBuilder, builder);
          state.Counter = state.Period;
        }
      }

      void ParseChannel(ParserState::ChannelState& state, PatternBuilder& patBuilder, Builder& builder) const
      {
        while (state.Offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd <= 0x5f)
          {
            builder.SetNote(cmd);
            break;
          }
          else if (cmd <= 0x6f)
          {
            builder.SetSample(cmd - 0x60);
          }
          else if (cmd <= 0x7f)
          {
            builder.SetOrnament(cmd - 0x70);
          }
          else if (cmd == 0x80)
          {
            builder.SetRest();
            break;
          }
          else if (cmd == 0x81)
          {
            builder.SetNoEnvelope();
          }
          else if (cmd <= 0x8f)
          {
            const uint_t type = cmd - 0x81;
            const uint_t tone = PeekByte(state.Offset) | (uint_t(PeekByte(state.Offset + 1)) << 8);
            state.Offset += 2;
            builder.SetEnvelope(type, tone);
          }
          else if (cmd == 0x90)
          {
            break;
          }
          else if (cmd <= 0xa0)
          {
            patBuilder.SetTempo(cmd - 0x91);
          }
          else if (cmd <= 0xb0)
          {
            builder.SetVolume(cmd - 0xa1);
          }
          else
          {
            state.Period = cmd - 0xb1;
          }
        }
      }

      static void ParseSample(const RawSample& src, uint_t size, Sample& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          dst.Lines[idx] = ParseSampleLine(line);
        }
        dst.Loop = std::min<uint_t>(src.Loop, dst.Lines.size());
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& line)
      {
        Sample::Line res;
        res.Level = line.GetLevel();
        res.Noise = line.GetNoise();
        res.ToneMask = line.GetToneMask();
        res.NoiseMask = line.GetNoiseMask();
        res.Vibrato = line.GetVibrato();
        return res;
      }

      static void ParseOrnament(const RawOrnament& src, uint_t size, Ornament& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          dst.Lines[idx] = src.GetLine(idx);
        }
      }
    private:
      const Binary::TypedContainer& Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
    };

    enum AreaTypes
    {
      HEADER,
      PATTERNS,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        AddArea(PATTERNS, fromLE(header.PatternsOffset));
        AddArea(END, size);
      }

      bool CheckHeader(std::size_t headerSize) const
      {
        return GetAreaSize(HEADER) >= headerSize && Undefined == GetAreaSize(END);
      }

      bool CheckPatterns(std::size_t headerSize) const
      {
        return Undefined != GetAreaSize(PATTERNS) && headerSize == GetAreaAddress(PATTERNS);
      }
    };

    Binary::TypedContainer CreateContainer(const Binary::Container& rawData)
    {
      return Binary::TypedContainer(rawData, std::min(rawData.Size(), MAX_SIZE));
    }

    bool FastCheck(const Binary::TypedContainer& data)
    {
      const std::size_t hdrSize = GetHeaderSize(data);
      if (!Math::InRange<std::size_t>(hdrSize, sizeof(RawHeader) + 1, sizeof(RawHeader) + MAX_POSITIONS_COUNT))
      {
        return false;
      }
      const RawHeader& hdr = *data.GetField<RawHeader>(0);
      const Areas areas(hdr, data.GetSize());
      if (!areas.CheckHeader(hdrSize))
      {
        return false;
      }
      if (!areas.CheckPatterns(hdrSize))
      {
        return false;
      }
      return true;
    }

    bool FastCheck(const Binary::Container& rawData)
    {
      const Binary::TypedContainer data = CreateContainer(rawData);
      return FastCheck(data);
    }

    const std::string FORMAT(
      "02-0f"      // uint8_t Tempo; 2..15
      "01-ff"      // uint8_t Length;
      "00-fe"      // uint8_t Loop;
      //boost::array<uint16_t, 16> SamplesOffsets;
      "(?00-28){16}"
      //boost::array<uint16_t, 16> OrnamentsOffsets;
      "(?00-28){16}"
      "?00-01" // uint16_t PatternsOffset;
      "?{30}"   // char Name[30];
      "00-1f"  // uint8_t Positions[1]; at least one
      "ff|00-1f" //next position or limit
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
        return Text::PROTRACKER1_DECODER_DESCRIPTION;
      }

      virtual Binary::Format::Ptr GetFormat() const
      {
        return Format;
      }

      virtual bool Check(const Binary::Container& rawData) const
      {
        return Format->Match(rawData) && FastCheck(rawData);
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

    Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
    {
      const Binary::TypedContainer data = CreateContainer(rawData);

      if (!FastCheck(data))
      {
        return Formats::Chiptune::Container::Ptr();
      }

      try
      {
        const Format format(data);

        format.ParseCommonProperties(target);

        StatisticCollectingBuilder statistic(target);
        format.ParsePositions(statistic);
        const Indices& usedPatterns = statistic.GetUsedPatterns();
        format.ParsePatterns(usedPatterns, statistic);
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, target);
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

        Require(format.GetSize() >= MIN_SIZE);
        const Binary::Container::Ptr subData = rawData.GetSubcontainer(0, format.GetSize());
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
  }// namespace ProTracker1

  Decoder::Ptr CreateProTracker1Decoder()
  {
    return boost::make_shared<ProTracker1::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats
