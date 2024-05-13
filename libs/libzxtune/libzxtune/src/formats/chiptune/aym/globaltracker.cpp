/**
* 
* @file
*
* @brief  GlobalTracker support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "globaltracker.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/metainfo.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <indices.h>
#include <iterator.h>
#include <range_checker.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
#include <strings/format.h>
//std includes
#include <cstring>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::GlobalTracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace GlobalTracker
  {
    const std::size_t MIN_SIZE = 1500;
    const std::size_t MAX_SIZE = 0x2800;
    const std::size_t MAX_SAMPLES_COUNT = 15;
    const std::size_t MAX_ORNAMENTS_COUNT = 16;
    const std::size_t MAX_PATTERNS_COUNT = 32;
    const std::size_t MIN_PATTERN_SIZE = 4;
    const std::size_t MAX_PATTERN_SIZE = 64;

    /*
      Typical module structure

      Header
      Patterns
      Samples
      Ornaments
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawPattern
    {
      ruint16_t Offsets[3];
 //     boost::array<uint16_t, 3> Offsets;
    } PACK_POST;

    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      uint8_t ID[3];
      uint8_t Version;
      ruint16_t Address;
      char Title[32];
	  
      ruint16_t SamplesOffsets[MAX_SAMPLES_COUNT];
      ruint16_t OrnamentsOffsets[MAX_ORNAMENTS_COUNT];
      RawPattern Patterns[MAX_PATTERNS_COUNT];
	  
      uint8_t Length;
      uint8_t Loop;
      uint8_t Positions[1];
    } PACK_POST;

    PACK_PRE struct RawObject
    {
      uint8_t Loop;
      uint8_t Size;

      uint_t GetSize() const
      {
        return Size;
      }
    } PACK_POST;

    PACK_PRE struct RawInt16
    {
      ruint16_t Val;
    } PACK_POST;

    PACK_PRE struct RawOrnament : RawObject
    {
      typedef int8_t Line;

      Line GetLine(uint_t idx) const
      {
        const int8_t* const src = safe_ptr_cast<const int8_t*>(this + 1);
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        return src[offset];
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }
    } PACK_POST;

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        // aaaaaaaa
        // ENTnnnnn
        // vvvvvvvv
        // vvvvvvvv

        // a - level
        // T - tone mask
        // N - noise mask
        // E - env mask
        // n - noise
        // v - signed vibrato
        uint8_t Level;
        uint8_t NoiseAndFlag;
        ruint16_t Vibrato;

        uint_t GetLevel() const
        {
          return Level;
        }

        bool GetToneMask() const
        {
          return 0 != (NoiseAndFlag & 32);
        }

        bool GetNoiseMask() const
        {
          return 0 != (NoiseAndFlag & 64);
        }

        bool GetEnvelopeMask() const
        {
          return 0 != (NoiseAndFlag & 128);
        }

        uint_t GetNoise() const
        {
          return NoiseAndFlag & 31;
        }

        uint_t GetVibrato() const
        {
          return fromLE(Vibrato);
        }
      } PACK_POST;

      Line GetLine(uint_t idx) const
      {
        BOOST_STATIC_ASSERT(0 == (sizeof(Line) & (sizeof(Line) - 1)));
        const uint_t maxLines = 256 / sizeof(Line);
        const Line* const src = safe_ptr_cast<const Line*>(this + 1);
        return src[idx % maxLines];
      }

      uint_t GetSize() const
      {
        Require(0 == Size % sizeof(Line));
        return (Size ? Size : 256) / sizeof(Line);
      }

      uint_t GetLoop() const
      {
        Require(0 == Loop % sizeof(Line));
        return Loop / sizeof(Line);
      }

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 296);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 6);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawSample::Line) == 4);

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
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
      virtual void SetNoEnvelope() {}
      virtual void SetVolume(uint_t /*vol*/) {}
    };

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(0, MAX_PATTERNS_COUNT - 1)
        , UsedSamples(0, MAX_SAMPLES_COUNT - 1)
        , UsedOrnaments(0, MAX_ORNAMENTS_COUNT - 1)
        , NonEmptyPatterns(false)
        , NonEmptySamples(false)
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
        assert(0 == index || UsedSamples.Contain(index));
        if (IsSampleSounds(sample))
        {
          NonEmptySamples = true;
        }
        return Delegate.SetSample(index, sample);
      }

      virtual void SetOrnament(uint_t index, const Ornament& ornament)
      {
        assert(0 == index || UsedOrnaments.Contain(index));
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
        NonEmptyPatterns = true;
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

      virtual void SetEnvelope(uint_t type, uint_t value)
      {
        return Delegate.SetEnvelope(type, value);
      }

      virtual void SetNoEnvelope()
      {
        return Delegate.SetNoEnvelope();
      }

      virtual void SetVolume(uint_t vol)
      {
        return Delegate.SetVolume(vol);
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

      bool HasNonEmptyPatterns() const
      {
        return NonEmptyPatterns;
      }

      bool HasNonEmptySamples() const
      {
        return NonEmptySamples;
      }
    private:
      static bool IsSampleSounds(const Sample& smp)
      {
        if (smp.Lines.empty())
        {
          return false;
        }
        for (uint_t idx = 0; idx != smp.Lines.size(); ++idx)
        {
          const Sample::Line& line = smp.Lines[idx];
          if ((!line.ToneMask && line.Level) || !line.NoiseMask)
          {
            return true;//has envelope or tone with volume
          }
        }
        return false;
      }
    private:
      Builder& Delegate;
      Indices UsedPatterns;
      Indices UsedSamples;
      Indices UsedOrnaments;
      bool NonEmptyPatterns;
      bool NonEmptySamples;
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

    bool IsInvalidPosEntry(uint8_t entry)
    {
      return entry % sizeof(RawPattern) != 0
          || !Math::InRange<uint_t>(entry / sizeof(RawPattern) + 1, 1, MAX_PATTERNS_COUNT);
    }

    class Format
    {
    public:
      explicit Format(const Binary::TypedContainer& data)
        : Delegate(data)
        , Ranges(Delegate.GetSize())
        , Source(*Delegate.GetField<RawHeader>(0))
        , HeaderSize(sizeof(Source) - 1 + Source.Length)
        , UnfixDelta(fromLE(Source.Address))
      {
        Ranges.AddService(0, sizeof(Source) - sizeof(Source.Positions));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Strings::Format(Text::GLOBALTRACKER1_EDITOR, Source.Version & 15));
        meta.SetTitle(FromCharArray(Source.Title));
      }

      void ParsePositions(Builder& builder) const
      {
        const uint8_t* const begin = safe_ptr_cast<const uint8_t*>(&Source);
        const uint8_t* const posStart = Source.Positions;
        const uint8_t* const posEnd = posStart + Source.Length;
        Ranges.AddService(posStart - begin, posEnd - posStart);
        Require(posEnd == std::find_if(posStart, posEnd, &IsInvalidPosEntry));
        std::vector<uint_t> positions(posEnd - posStart);
        std::transform(posStart, posEnd, positions.begin(), std::bind2nd(std::divides<uint_t>(), sizeof(RawPattern)));
        const uint_t loop = std::min<uint_t>(Source.Loop, positions.size());
        builder.SetPositions(positions, loop);
        Dbg("Positions: %1% entries, loop to %2%", positions.size(), loop);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        Dbg("Patterns: %1% to parse", pats.Count());
        bool hasValidPatterns = false;
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          if (ParsePattern(patIndex, builder))
          {
            hasValidPatterns = true;
          }
        }
        Require(hasValidPatterns);
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        bool hasValidSamples = false, hasPartialSamples = false;
        Dbg("Samples: %1% to parse", samples.Count());
        const std::size_t minOffset = HeaderSize;
        const std::size_t maxOffset = Delegate.GetSize();
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const std::size_t samOffset = GetSampleOffset(samIdx);
          Require(Math::InRange(samOffset, minOffset, maxOffset));
          const std::size_t availSize = Delegate.GetSize() - samOffset;
          const RawSample* const src = Delegate.GetField<RawSample>(samOffset);
          Require(src != 0);
          Sample result;
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
          if (const std::size_t ornOffset = GetOrnamentOffset(ornIdx))
          {
            Require(ornOffset >= HeaderSize);
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
        const std::size_t result = Ranges.GetSize();
        Require(result + UnfixDelta <= 0x10000);
        return result;
      }

      RangeChecker::Range GetFixedArea() const
      {
        return Ranges.GetFixedArea();
      }
    private:
      std::size_t GetSampleOffset(uint_t index) const
      {
        if (const uint16_t offset = fromLE(Source.SamplesOffsets[index]))
        {
          Require(offset >= UnfixDelta);
          return offset - UnfixDelta;
        }
        else
        {
          return 0;
        }
      }

      std::size_t GetOrnamentOffset(uint_t index) const
      {
        if (const uint16_t offset = fromLE(Source.OrnamentsOffsets[index]))
        {
          Require(offset >= UnfixDelta);
          return offset - UnfixDelta;
        }
        else
        {
          return 0;
        }
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const uint8_t* const data = Delegate.GetField<uint8_t>(offset);
        Require(data != 0);
        return *data;
      }

      struct DataCursors : public boost::array<std::size_t, 3>
      {
        DataCursors(const RawPattern& src, uint_t minOffset, uint_t unfixDelta)
        {
          for (std::size_t idx = 0; idx != size(); ++idx)
          {
            const std::size_t offset = fromLE(src.Offsets[idx]);
            Require(offset >= minOffset + unfixDelta);
            at(idx) = offset - unfixDelta;
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


      bool ParsePattern(uint_t patIndex, Builder& builder) const
      {
        const RawPattern& src = Source.Patterns[patIndex];
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        const DataCursors rangesStarts(src, HeaderSize, UnfixDelta);
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
          ParseLine(state, builder);
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

      void ParseLine(ParserState& src, Builder& builder) const
      {
        for (uint_t chan = 0; chan < 3; ++chan)
        {
          ParserState::ChannelState& state = src.Channels[chan];
          if (state.Counter--)
          {
            continue;
          }
          builder.StartChannel(chan);
          ParseChannel(state, builder);
          state.Counter = state.Period;
        }
      }

      void ParseChannel(ParserState::ChannelState& state, Builder& builder) const
      {
        state.Period = 0;
        while (state.Offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd <= 0x5f)//set note
          {
            builder.SetNote(cmd);
            break;
          }
          else if (cmd <= 0x6f)//set sample
          {
            builder.SetSample(cmd - 0x60);
          }
          else if (cmd <= 0x7f) //set ornament
          {
            builder.SetOrnament(cmd - 0x70);
            if (NewVersion())
            {
              builder.SetNoEnvelope();
            }
          }
          else if (cmd <= 0xbf)
          {
            state.Period = cmd - 0x80;
          }
          else if (cmd <= 0xcf)
          {
            const uint_t envType = cmd - 0xc0;
            const uint_t envTone = PeekByte(state.Offset++);
            builder.SetEnvelope(envType, envTone);
          }
          else if (cmd <= 0xdf)
          {
            break;
          }
          else if (cmd == 0xe0)
          {
            builder.SetRest();
            if (NewVersion())
            {
              break;
            }
          }
          else if (cmd <= 0xef)
          {
            const uint_t volume = 15 - (cmd - 0xe0);
            builder.SetVolume(volume);
          }
        }
      }

      bool NewVersion() const
      {
        return Source.Version != 0x10;
      }

      static void ParseSample(const RawSample& src, uint_t size, Sample& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          const RawSample::Line& line = src.GetLine(idx);
          dst.Lines[idx] = ParseSampleLine(line);
        }
        dst.Loop = std::min<int_t>(src.GetLoop(), size);
      }

      static Sample::Line ParseSampleLine(const RawSample::Line& line)
      {
        Sample::Line res;
        res.Level = line.GetLevel();
        res.Noise = line.GetNoise();
        res.ToneMask = line.GetToneMask();
        res.NoiseMask = line.GetNoiseMask();
        res.EnvelopeMask = line.GetEnvelopeMask();
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
        dst.Loop = std::min<int_t>(src.Loop, size);
      }
    private:
      const Binary::TypedContainer Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
      const std::size_t HeaderSize;
      const uint_t UnfixDelta;
    };

    Binary::TypedContainer CreateContainer(const Binary::Container& rawData)
    {
      return Binary::TypedContainer(rawData, std::min(rawData.Size(), MAX_SIZE));
    }

    std::size_t GetHeaderSize(const Binary::TypedContainer& data)
    {
      if (const RawHeader* hdr = data.GetField<RawHeader>(0))
      {
        const std::size_t hdrSize = sizeof(*hdr) - 1 + hdr->Length;
        if (hdrSize >= data.GetSize())
        {
          return 0;
        }
        const uint8_t* const posStart = hdr->Positions;
        const uint8_t* const posEnd = posStart + hdr->Length;
        if (posEnd != std::find_if(posStart, posEnd, &IsInvalidPosEntry))
        {
          return 0;
        }
        return hdrSize;
      }
      return 0;
    }

    enum AreaTypes
    {
      HEADER,
      PATTERNS,
      SAMPLES,
      ORNAMENTS,
      END,
    };

    std::size_t GetStart(const uint16_t* begin, const uint16_t* end, std::size_t start)
    {
      std::vector<std::size_t> offsets; 
//      std::transform(begin, end, std::back_inserter(offsets), &fromLE<uint16_t>);// input buffer may be unaligned.. so this wont work
	  int i, size = ((end-begin)>>1)+1;
	  for (i= 0; i<size; i++) {
		offsets.push_back(fromLE(((RawInt16*)begin)[i].Val));
	  }

      std::sort(offsets.begin(), offsets.end());	  
      std::vector<std::size_t>::const_iterator it = offsets.begin();
      if (it != offsets.end() && *it == 0)
      {
        ++it;
      }
      return it != offsets.end() && *it >= start
        ? *it - start : 0;
    }

    struct Areas : public AreaController
    {
      Areas(const RawHeader& hdr, std::size_t size)
        : StartAddr(fromLE(hdr.Address))
      {	  
        AddArea(HEADER, 0);
        AddArea(PATTERNS, GetStart(&(hdr.Patterns[0].Offsets[0]), &(hdr.Patterns[MAX_PATTERNS_COUNT-1].Offsets[2]), StartAddr));
        AddArea(SAMPLES, GetStart(&(hdr.SamplesOffsets[0]), &(hdr.SamplesOffsets[MAX_SAMPLES_COUNT-1]), StartAddr));
        AddArea(ORNAMENTS, GetStart(&(hdr.OrnamentsOffsets[0]), &(hdr.OrnamentsOffsets[MAX_ORNAMENTS_COUNT-1]), StartAddr));
        AddArea(END, size);
      }

      bool CheckHeader(std::size_t headerSize) const
      {
        return headerSize + StartAddr < 0x10000
            && GetAreaSize(HEADER) >= headerSize
            && Undefined == GetAreaSize(END);
      }

      bool CheckPatterns() const
      {
        return GetAreaSize(PATTERNS) != 0;
      }

      bool CheckSamples() const
      {
        return GetAreaSize(SAMPLES) != 0;
      }

      bool CheckOrnaments() const
      {
        return GetAreaSize(ORNAMENTS) != 0;
      }
    private:
      const std::size_t StartAddr;
    };

    bool FastCheck(const Binary::TypedContainer& data)
    {
      const std::size_t hdrSize = GetHeaderSize(data);
      if (!Math::InRange<std::size_t>(hdrSize, sizeof(RawHeader), data.GetSize()))
      {
        return false;
      }
      const RawHeader& hdr = *data.GetField<RawHeader>(0);
      const Areas areas(hdr, data.GetSize());
      if (!areas.CheckHeader(hdrSize))
      {
        return false;
      }
      if (!areas.CheckPatterns())
      {
        return false;
      }
      if (!areas.CheckSamples())
      {
        return false;
      }
      if (!areas.CheckOrnaments())
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
      "03-0f"          //uint8_t Tempo;
      "???"            //uint8_t ID[3];
      "10-12"          //uint8_t Version; who knows?
      "??"             //uint16_t Address;
      "?{32}"          //char Title[32];
      "?{30}"          //boost::array<uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
      "?{32}"          //boost::array<uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
      "?{192}"         //boost::array<RawPattern, MAX_PATTERNS_COUNT> Patterns;
      "01-ff"          //uint8_t Length;
      "00-fe"          //uint8_t Loop;
      "*6&00-ba"       //uint8_t Positions[1];
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
        return Text::GLOBALTRACKER_DECODER_DESCRIPTION;
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
        Require(statistic.HasNonEmptyPatterns());
        const Indices& usedSamples = statistic.GetUsedSamples();
        format.ParseSamples(usedSamples, statistic);
        Require(statistic.HasNonEmptySamples());
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
  }// namespace GlobalTracker

  Decoder::Ptr CreateGlobalTrackerDecoder()
  {
    return boost::make_shared<GlobalTracker::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats
