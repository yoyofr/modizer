/**
* 
* @file
*
* @brief  SQTracker compiled modules support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "sqtracker.h"
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
//boost includes
#include <boost/array.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::SQTracker");
}

namespace Formats
{
namespace Chiptune
{
  namespace SQTracker
  {
    const std::size_t MIN_MODULE_SIZE = 256;
    const std::size_t MAX_MODULE_SIZE = 0x3600;
    const std::size_t MAX_POSITIONS_COUNT = 120;
    const std::size_t MIN_PATTERN_SIZE = 11;
    const std::size_t MAX_PATTERN_SIZE = 64;
    const std::size_t MAX_PATTERNS_COUNT = 99;
    const std::size_t MAX_SAMPLES_COUNT = 26;
    const std::size_t MAX_ORNAMENTS_COUNT = 26;
    const std::size_t SAMPLE_SIZE = 32;
    const std::size_t ORNAMENT_SIZE = 32;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawUInt16	// needed for unaligned data access
    {
      ruint16_t Val;
    } PACK_POST;

    PACK_PRE struct RawSample
    {
      uint8_t Loop;
      uint8_t LoopSize;
      PACK_PRE struct Line
      {
        //nnnnvvvv
        uint8_t VolumeAndNoise;
        //nTNDdddd
        //n- low noise bit
        //d- high delta bits
        //D- delta sign (1 for positive)
        //N- enable noise
        //T- enable tone
        uint8_t Flags;
        //dddddddd - low delta bits
        uint8_t Delta;

        uint_t GetLevel() const
        {
          return VolumeAndNoise & 15;
        }

        uint_t GetNoise() const
        {
          return ((VolumeAndNoise & 240) >> 3) | ((Flags & 128) >> 7);
        }

        bool GetEnableNoise() const
        {
          return 0 != (Flags & 32);
        }

        bool GetEnableTone() const
        {
          return 0 != (Flags & 64);
        }

        int_t GetToneDeviation() const
        {
          const int delta = int_t(Delta) | ((Flags & 15) << 8);
          return 0 != (Flags & 16) ? delta : -delta;
        }
      } PACK_POST;
      Line Data[SAMPLE_SIZE];
    } PACK_POST;

    PACK_PRE struct RawPosEntry
    {
      PACK_PRE struct Channel
      {
        //0 is end
        //Eppppppp
        //p - pattern idx
        //E - enable effect
        uint8_t Pattern;
        //TtttVVVV
        //signed transposition
        uint8_t TranspositionAndAttenuation;

        uint_t GetPattern() const
        {
          return Pattern & 127;
        }

        int_t GetTransposition() const
        {
          const int_t transpos = TranspositionAndAttenuation >> 4;
          return transpos < 9 ? transpos : -(transpos - 9) - 1;
        }

        uint_t GetAttenuation() const
        {
          return TranspositionAndAttenuation & 15;
        }

        bool GetEnableEffects() const
        {
          return 0 != (Pattern & 128);
        }
      } PACK_POST;

      Channel ChannelC;
      Channel ChannelB;
      Channel ChannelA;
      uint8_t Tempo;
    } PACK_POST;

    PACK_PRE struct RawOrnament
    {
      uint8_t Loop;
      uint8_t LoopSize;
      boost::array<int8_t, ORNAMENT_SIZE> Lines;
    } PACK_POST;

    PACK_PRE struct RawHeader
    {
      ruint16_t Size;
      ruint16_t SamplesOffset;
      ruint16_t OrnamentsOffset;
      ruint16_t PatternsOffset;
      ruint16_t PositionsOffset;
      ruint16_t LoopPositionOffset;
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }
      //samples+ornaments
      virtual void SetSample(uint_t /*index*/, const Sample& /*sample*/) {}
      virtual void SetOrnament(uint_t /*index*/, const Ornament& /*ornament*/) {}
      //patterns
      virtual void SetPositions(const std::vector<PositionEntry>& /*positions*/, uint_t /*loop*/) {}
      virtual PatternBuilder& StartPattern(uint_t /*index*/)
      {
        return GetStubPatternBuilder();
      }
      virtual void SetTempoAddon(uint_t /*add*/) {}
      virtual void SetRest() {}
      virtual void SetNote(uint_t /*note*/) {}
      virtual void SetSample(uint_t /*sample*/) {}
      virtual void SetOrnament(uint_t /*ornament*/) {}
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
      virtual void SetGlissade(int_t /*step*/) {}
      virtual void SetAttenuation(uint_t /*att*/) {}
      virtual void SetAttenuationAddon(int_t /*add*/) {}
      virtual void SetGlobalAttenuation(uint_t /*att*/) {}
      virtual void SetGlobalAttenuationAddon(int_t /*add*/) {}
    };

    class StatisticCollectingBuilder : public Builder
    {
    public:
      explicit StatisticCollectingBuilder(Builder& delegate)
        : Delegate(delegate)
        , UsedPatterns(1, MAX_PATTERNS_COUNT)
        , UsedSamples(1, MAX_SAMPLES_COUNT)
        , UsedOrnaments(1, MAX_ORNAMENTS_COUNT)
      {
      }

      virtual MetaBuilder& GetMetaBuilder()
      {
        return Delegate.GetMetaBuilder();
      }

      virtual void SetSample(uint_t index, const Sample& sample)
      {
        assert(UsedSamples.Contain(index));
        return Delegate.SetSample(index, sample);
      }

      virtual void SetOrnament(uint_t index, const Ornament& ornament)
      {
        assert(UsedOrnaments.Contain(index));
        return Delegate.SetOrnament(index, ornament);
      }

      virtual void SetPositions(const std::vector<PositionEntry>& positions, uint_t loop)
      {
        UsedPatterns.Clear();
        for (std::vector<PositionEntry>::const_iterator it = positions.begin(), lim = positions.end(); it != lim; ++it)
        {
          for (uint_t chan = 0; chan != 3; ++chan)
          {
            UsedPatterns.Insert(it->Channels[chan].Pattern);
          }
        }
        Require(!UsedPatterns.Empty());
        return Delegate.SetPositions(positions, loop);
      }

      virtual PatternBuilder& StartPattern(uint_t index)
      {
        assert(UsedPatterns.Contain(index));
        return Delegate.StartPattern(index);
      }

      virtual void SetTempoAddon(uint_t add)
      {
        return Delegate.SetTempoAddon(add);
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

      virtual void SetEnvelope(uint_t type, uint_t value)
      {
        return Delegate.SetEnvelope(type, value);
      }

      virtual void SetGlissade(int_t step)
      {
        return Delegate.SetGlissade(step);
      }

      virtual void SetAttenuation(uint_t att)
      {
        return Delegate.SetAttenuation(att);
      }

      virtual void SetAttenuationAddon(int_t add)
      {
        return Delegate.SetAttenuationAddon(add);
      }

      virtual void SetGlobalAttenuation(uint_t att)
      {
        return Delegate.SetGlobalAttenuation(att);
      }

      virtual void SetGlobalAttenuationAddon(int_t add)
      {
        return Delegate.SetGlobalAttenuationAddon(add);
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
        : ServiceRanges(RangeChecker::CreateSimple(limit))
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

    std::size_t GetFixDelta(const RawHeader& hdr)
    {
      const uint_t samplesAddr = fromLE(hdr.SamplesOffset);
      //since ornaments,samples and patterns tables are 1-based, real delta is 2 bytes shorter
      const uint_t delta = offsetof(RawHeader, LoopPositionOffset);
      Require(samplesAddr >= delta);
      return samplesAddr - delta;
    }

    class Format
    {
    public:
      explicit Format(const Binary::TypedContainer& data)
        : Delegate(data)
        , Ranges(data.GetSize())
        , Source(*Delegate.GetField<RawHeader>(0))
        , Delta(GetFixDelta(Source))
      {
        Ranges.AddService(0, sizeof(Source));
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.GetMetaBuilder().SetProgram(Text::SQTRACKER_DECODER_DESCRIPTION);
      }

      void ParsePositions(Builder& builder) const
      {
        const std::size_t loopPositionOffset = fromLE(Source.LoopPositionOffset) - Delta;
        std::size_t posOffset = fromLE(Source.PositionsOffset) - Delta;
        std::vector<PositionEntry> result;
        uint_t loopPos = 0;
        for (uint_t pos = 0;; ++pos)
        {
          if (posOffset == loopPositionOffset)
          {
            loopPos = pos;
          }
          if (const RawPosEntry* fullEntry = Delegate.GetField<RawPosEntry>(posOffset))
          {
            Ranges.AddService(posOffset, sizeof(*fullEntry));
            if (0 == fullEntry->ChannelC.GetPattern()
             || 0 == fullEntry->ChannelB.GetPattern()
             || 0 == fullEntry->ChannelA.GetPattern())
            {
              break;
            }
            PositionEntry dst;
            ParsePositionChannel(fullEntry->ChannelC, dst.Channels[2]);
            ParsePositionChannel(fullEntry->ChannelB, dst.Channels[1]);
            ParsePositionChannel(fullEntry->ChannelA, dst.Channels[0]);
            dst.Tempo = fullEntry->Tempo;
            posOffset += sizeof(*fullEntry);
            result.push_back(dst);
          }
          else
          {
            const RawPosEntry::Channel& partialEntry = GetServiceObject<RawPosEntry::Channel>(posOffset);
            Require(0 == partialEntry.GetPattern());
            const std::size_t tailSize = std::min(sizeof(RawPosEntry), Delegate.GetSize() - posOffset);
            Ranges.Add(posOffset, tailSize);
            break;
          }
        }
        builder.SetPositions(result, loopPos);
        Dbg("Positions: %1% entries, loop to %2%", result.size(), loopPos);
      }

      void ParsePatterns(const Indices& pats, Builder& builder) const
      {
        for (Indices::Iterator it = pats.Items(); it; ++it)
        {
          const uint_t patIndex = *it;
          Dbg("Parse pattern %1%", patIndex);
          ParsePattern(patIndex, builder);
        }
      }

      void ParseSamples(const Indices& samples, Builder& builder) const
      {
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          Dbg("Parse sample %1%", samIdx);
          Sample result;
          const RawSample& src = GetSample(samIdx);
          ParseSample(src, result);
          builder.SetSample(samIdx, result);
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        if (ornaments.Empty())
        {
          Dbg("No ornaments used");
          return;
        }
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          Dbg("Parse ornament %1%", ornIdx);
          Ornament result;
          const RawOrnament& src = GetOrnament(ornIdx);
          ParseOrnament(src, result);
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
      template<class T>
      const T& GetServiceObject(std::size_t offset) const
      {
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != 0);
        Ranges.AddService(offset, sizeof(T));
        return *src;
      }

      template<class T>
      const T& GetObject(std::size_t offset) const
      {
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != 0);
        Ranges.Add(offset, sizeof(T));
        return *src;
      }

      uint8_t PeekByte(std::size_t offset) const
      {
        const uint8_t* const data = Delegate.GetField<uint8_t>(offset);
        Require(data != 0);
        return *data;
      }

      std::size_t GetPatternOffset(uint_t index) const
      {
        const std::size_t entryAddr = fromLE(Source.PatternsOffset) + index * sizeof(uint16_t);
        Require(entryAddr >= Delta);
        const std::size_t patternAddr = fromLE(GetServiceObject<RawUInt16>(entryAddr - Delta).Val);
        Require(patternAddr >= Delta);
        return patternAddr - Delta;
      }

      const RawSample& GetSample(uint_t index) const
      {
        const std::size_t entryAddr = fromLE(Source.SamplesOffset) + index * sizeof(uint16_t);
        Require(entryAddr >= Delta);
        const std::size_t sampleAddr = fromLE(GetServiceObject<RawUInt16>(entryAddr - Delta).Val);
        Require(sampleAddr >= Delta);
        return GetObject<RawSample>(sampleAddr - Delta);
      }

      const RawOrnament& GetOrnament(uint_t index) const
      {
        const std::size_t entryAddr = fromLE(Source.OrnamentsOffset) + index * sizeof(uint16_t);
        Require(entryAddr >= Delta);
        const std::size_t ornamentAddr = fromLE(GetServiceObject<RawUInt16>(entryAddr - Delta).Val);
        Require(ornamentAddr >= Delta);
        return GetObject<RawOrnament>(ornamentAddr - Delta);
      }

      struct ParserState
      {
        uint_t Counter;
        std::size_t Cursor;
        uint_t LastNote;
        std::size_t LastNoteStart;
        bool RepeatLastNote;

        ParserState(std::size_t cursor)
          : Counter()
          , Cursor(cursor)
          , LastNote()
          , LastNoteStart()
          , RepeatLastNote()
        {
        }
      };

      void ParsePattern(std::size_t patIndex, Builder& builder) const
      {
        const std::size_t patOffset = GetPatternOffset(patIndex);
        const std::size_t patSize = PeekByte(patOffset);
        Require(Math::InRange(patSize, MIN_PATTERN_SIZE, MAX_PATTERN_SIZE));

        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
        ParserState state(patOffset + 1);
        uint_t lineIdx = 0;
        for (; lineIdx < patSize; ++lineIdx)
        {
          if (state.Counter)
          {
            --state.Counter;
            if (state.RepeatLastNote)
            {
              patBuilder.StartLine(lineIdx);
              ParseNote(state.LastNoteStart, patBuilder, builder);
            }
          }
          else
          {
            patBuilder.StartLine(lineIdx);
            ParseLine(state, patBuilder, builder);
          }
        }
        patBuilder.Finish(lineIdx);
        const std::size_t start = patOffset;
        if (start >= Delegate.GetSize())
        {
          Dbg("Invalid offset (%1%)", start);
        }
        else
        {
          const std::size_t stop = std::min(Delegate.GetSize(), state.Cursor);
          Ranges.AddFixed(start, stop - start);
        }
      }

      void ParseLine(ParserState& state, PatternBuilder& patBuilder, Builder& builder) const
      {
        state.RepeatLastNote = false;
        const uint_t cmd = PeekByte(state.Cursor++);
        if (cmd <= 0x5f)
        {
          state.LastNote = cmd;
          state.LastNoteStart = state.Cursor - 1;
          builder.SetNote(cmd);
          state.Cursor = ParseNoteParameters(state.Cursor, patBuilder, builder);
        }
        else if (cmd <= 0x6e)
        {
          ParseEffect(cmd - 0x60, PeekByte(state.Cursor++), patBuilder, builder);
        }
        else if (cmd == 0x6f)
        {
          builder.SetRest();
        }
        else if (cmd <= 0x7f)
        {
          builder.SetRest();
          ParseEffect(cmd - 0x6f, PeekByte(state.Cursor++), patBuilder, builder);
        }
        else if (cmd <= 0x9f)
        {
          Require(0 != state.LastNoteStart);
          const uint_t addon = cmd & 15;
          if (0 != (cmd & 16))
          {
            state.LastNote -= addon;
          }
          else
          {
            state.LastNote += addon;
          }
          builder.SetNote(state.LastNote);
          ParseNote(state.LastNoteStart, patBuilder, builder);
        }
        else if (cmd <= 0xbf)
        {
          state.Counter = cmd & 15;
          if (cmd & 16)
          {
            state.RepeatLastNote |= state.Counter != 0;
            ParseNote(state.LastNoteStart, patBuilder, builder);
          }
        }
        else
        {
          state.LastNoteStart = state.Cursor - 1;
          builder.SetSample(cmd & 31);
        }
      }

      void ParseNote(std::size_t cursor, PatternBuilder& patBuilder, Builder& builder) const
      {
        const uint_t cmd = PeekByte(cursor);
        if (cmd < 0x80)
        {
          ParseNoteParameters(cursor + 1, patBuilder, builder);
        }
        else
        {
          builder.SetSample(cmd & 31);
        }
      }

      std::size_t ParseNoteParameters(std::size_t start, PatternBuilder& patBuilder, Builder& builder) const
      {
        std::size_t cursor = start;
        const uint_t cmd = PeekByte(cursor++);
        if (0 != (cmd & 128))
        {
          if (const uint_t sample = (cmd >> 1) & 31)
          {
            builder.SetSample(sample);
          }
          if (0 != (cmd & 64))
          {
            const uint_t param = PeekByte(cursor++);
            if (const uint_t ornament = (param >> 4) | ((cmd & 1) << 4))
            {
              builder.SetOrnament(ornament);
            }
            if (const uint_t effect = param & 15)
            {
              ParseEffect(effect, PeekByte(cursor++), patBuilder, builder);
            }
          }
        }
        else
        {
          ParseEffect(cmd, PeekByte(cursor++), patBuilder, builder);
        }
        return cursor;
      }

      static void ParseEffect(uint_t code, uint_t param, PatternBuilder& patBuilder, Builder& builder)
      {
        switch (code - 1)
        {
        case 0:
          builder.SetAttenuation(param & 15);
          break;
        case 1:
          builder.SetAttenuationAddon(static_cast<int8_t>(param));
          break;
        case 2:
          builder.SetGlobalAttenuation(param);
          break;
        case 3:
          builder.SetGlobalAttenuationAddon(static_cast<int8_t>(param));
          break;
        case 4:
          patBuilder.SetTempo(param & 31 ? param & 31 : 32);
          break;
        case 5:
          builder.SetTempoAddon(param);
          break;
        case 6:
          builder.SetGlissade(-int_t(param));
          break;
        case 7:
          builder.SetGlissade(int_t(param));
          break;
        default:
          builder.SetEnvelope((code - 1) & 15, param);
          break;
        }
      }

      static void ParsePositionChannel(const RawPosEntry::Channel& src, PositionEntry::Channel& dst)
      {
        dst.Pattern = src.GetPattern();
        dst.Transposition = src.GetTransposition();
        dst.Attenuation = src.GetAttenuation();
        dst.EnabledEffects = src.GetEnableEffects();
      }

      static void ParseSample(const RawSample& src, Sample& dst)
      {
        dst.Lines.resize(SAMPLE_SIZE);
        for (uint_t idx = 0; idx < SAMPLE_SIZE; ++idx)
        {
          const RawSample::Line& line = src.Data[idx];
          Sample::Line& res = dst.Lines[idx];
          res.Level = line.GetLevel();
          res.Noise = line.GetNoise();
          res.EnableNoise = line.GetEnableNoise();
          res.EnableTone = line.GetEnableTone();
          res.ToneDeviation = line.GetToneDeviation();
        }
        dst.Loop = std::min<uint_t>(src.Loop, SAMPLE_SIZE);
        dst.LoopSize = std::min<uint_t>(src.LoopSize, SAMPLE_SIZE - dst.Loop);
      }

      static void ParseOrnament(const RawOrnament& src, Ornament& dst)
      {
        dst.Lines.assign(src.Lines.begin(), src.Lines.end());
        dst.Loop = std::min<uint_t>(src.Loop, ORNAMENT_SIZE);
        dst.LoopSize = std::min<uint_t>(src.LoopSize, ORNAMENT_SIZE - dst.Loop);
      }
    private:
      const Binary::TypedContainer Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
      const std::size_t Delta;
    };

    enum AreaTypes
    {
      HEADER,
      SAMPLES_OFFSETS,
      ORNAMENTS_OFFSETS,
      PATTERNS_OFFSETS,
      SAMPLES,
      ORNAMENTS,
      PATTERNS,
      POSITIONS,
      LOOPED_POSITION,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const Binary::TypedContainer& data, std::size_t unfixDelta, std::size_t size)
      {
        const RawHeader& hdr = *data.GetField<RawHeader>(0);
        AddArea(HEADER, 0);
        const std::size_t sample1AddrOffset = fromLE(hdr.SamplesOffset) + sizeof(uint16_t) - unfixDelta;
        AddArea(SAMPLES_OFFSETS, sample1AddrOffset);
        const std::size_t ornament1AddrOffset = fromLE(hdr.OrnamentsOffset) + sizeof(uint16_t) - unfixDelta;
        if (hdr.OrnamentsOffset != hdr.PatternsOffset)
        {
          AddArea(ORNAMENTS_OFFSETS, ornament1AddrOffset);
        }
        const std::size_t pattern1AddOffset = fromLE(hdr.PatternsOffset) + sizeof(uint16_t) - unfixDelta;
        AddArea(PATTERNS_OFFSETS, pattern1AddOffset);
        AddArea(POSITIONS, fromLE(hdr.PositionsOffset) - unfixDelta);
        if (hdr.PositionsOffset != hdr.LoopPositionOffset)
        {
          AddArea(LOOPED_POSITION, fromLE(hdr.LoopPositionOffset) - unfixDelta);
        }
        AddArea(END, size);
        if (!CheckHeader())
        {
          return;
        }
		
        if (CheckSamplesOffsets())
        {
          const std::size_t sample1Addr = fromLE(data.GetField<RawUInt16>(sample1AddrOffset)->Val) - unfixDelta;
          AddArea(SAMPLES, sample1Addr);
        }
        if (CheckOrnamentsOffsets())
        {
          const std::size_t ornament1Addr = fromLE(data.GetField<RawUInt16>(ornament1AddrOffset)->Val) - unfixDelta;
          AddArea(ORNAMENTS, ornament1Addr);
        }
      }

      bool CheckHeader() const
      {
        return sizeof(RawHeader) == GetAreaSize(HEADER) && Undefined == GetAreaSize(END);
      }

      bool CheckSamples() const
      {
        const std::size_t size = GetAreaSize(SAMPLES);
        return size != Undefined && size >= sizeof(RawSample);
      }

      bool CheckOrnaments() const
      {
        if (GetAreaAddress(ORNAMENTS_OFFSETS) == Undefined)
        {
          return true;
        }
        else if (!CheckOrnamentsOffsets())
        {
          return false;
        }
        const std::size_t ornamentsSize = GetAreaSize(ORNAMENTS);
        return ornamentsSize != Undefined && ornamentsSize >= sizeof(RawOrnament);
      }

      bool CheckPatterns() const
      {
        const std::size_t size = GetAreaSize(PATTERNS_OFFSETS);
        return size != Undefined && size >= sizeof(uint16_t);
      }

      bool CheckPositions() const
      {
        if (GetAreaAddress(LOOPED_POSITION) != Undefined)
        {
          const std::size_t baseSize = GetAreaSize(POSITIONS);
          if (baseSize == Undefined || 0 != baseSize % sizeof(RawPosEntry))
          {
            return false;
          }
          const std::size_t restSize = GetAreaSize(LOOPED_POSITION);
          return restSize != Undefined && restSize >= sizeof(RawPosEntry) + sizeof(RawPosEntry::Channel);
        }
        else
        {
          const std::size_t baseSize = GetAreaSize(POSITIONS);
          return baseSize != Undefined && baseSize >= sizeof(RawPosEntry) + sizeof(RawPosEntry::Channel);
        }
      }
    private:
      bool CheckSamplesOffsets() const
      {
        const std::size_t size = GetAreaSize(SAMPLES_OFFSETS);
        return size != Undefined && size >= sizeof(uint16_t);
      }

      bool CheckOrnamentsOffsets() const
      {
        const std::size_t size = GetAreaSize(ORNAMENTS_OFFSETS);
        return size != Undefined && size >= sizeof(uint16_t);
      }
    };

    bool FastCheck(const Areas& areas)
    {
      if (!areas.CheckHeader())
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
      if (!areas.CheckPatterns())
      {
        return false;
      }
      if (!areas.CheckPositions())
      {
        return false;
      }
      return true;
    }

    Binary::TypedContainer CreateContainer(const Binary::Container& rawData)
    {
      return Binary::TypedContainer(rawData, std::min(rawData.Size(), MAX_MODULE_SIZE));
    }

    bool FastCheck(const Binary::TypedContainer& data)
    {
      const RawHeader* const hdr = data.GetField<RawHeader>(0);
      if (0 == hdr)
      {
        return false;
      }
      const std::size_t minSamplesAddr = offsetof(RawHeader, LoopPositionOffset);
      const std::size_t samplesAddr = fromLE(hdr->SamplesOffset);
      const std::size_t ornamentAddr = fromLE(hdr->OrnamentsOffset);
      const std::size_t patternAddr = fromLE(hdr->PatternsOffset);
      const std::size_t positionAddr = fromLE(hdr->PositionsOffset);
      const std::size_t loopPosAddr = fromLE(hdr->LoopPositionOffset);
      if (samplesAddr < minSamplesAddr
       || samplesAddr >= ornamentAddr
       || ornamentAddr > patternAddr
       || patternAddr >= positionAddr
       || positionAddr > loopPosAddr)
      {
        return false;
      }
      const std::size_t compileAddr = samplesAddr - minSamplesAddr;
      const Areas areas(data, compileAddr, data.GetSize());
      return FastCheck(areas);
    }

    bool FastCheck(const Binary::Container& rawData)
    {
      const Binary::TypedContainer data = CreateContainer(rawData);
      return FastCheck(data);
    }

    //TODO: size may be <256
    const std::string FORMAT(
      "?01-30"       //uint16_t Size;
      "?00|60-fb"    //uint16_t SamplesOffset;
      "?00|60-fb"    //uint16_t OrnamentsOffset;
      "?00|60-fb"    //uint16_t PatternsOffset;
      "?01-2d|61-ff" //uint16_t PositionsOffset;
      "?01-30|61-ff" //uint16_t LoopPositionOffset;
      //sample1 offset
      "?00|60-fb"
      //pattern1 offset minimal
      "?00-01|60-fc"
    );

    class Decoder : public Formats::Chiptune::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_MODULE_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SQTRACKER_DECODER_DESCRIPTION;
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
        return ParseCompiled(rawData, stub);
      }
    private:
      const Binary::Format::Ptr Format;
    };

    Formats::Chiptune::Container::Ptr ParseCompiled(const Binary::Container& rawData, Builder& target)
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
        format.ParseSamples(usedSamples, statistic);
        const Indices& usedOrnaments = statistic.GetUsedOrnaments();
        format.ParseOrnaments(usedOrnaments, target);

        Require(format.GetSize() >= MIN_MODULE_SIZE);
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
  }// namespace SQTracker

  Decoder::Ptr CreateSQTrackerDecoder()
  {
    return boost::make_shared<SQTracker::Decoder>();
  }
}// namespace Chiptune
}// namespace Formats
