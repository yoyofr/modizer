/**
* 
* @file
*
* @brief  ProTracker v3.x compiled modules support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "protracker3_detail.h"
#include "formats/chiptune/container.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <range_checker.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//std includes
#include <cctype>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/range/end.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::ProTracker3");
}

namespace Formats
{
namespace Chiptune
{
  namespace ProTracker3
  {
    const std::size_t MIN_SIZE = 200;
    const std::size_t MAX_SIZE = 0xc000;

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawId
    {
      char TrackName[32];
      char Optional2[4]; //' by '
      char TrackAuthor[32];

      bool HasAuthor() const
      {
        const Char BY_DELIMITER[] =
        {
          'B', 'Y', 0
        };

        const String id(FromCharArray(Optional2));
        const String trimId(boost::algorithm::trim_copy_if(id, boost::algorithm::is_from_range(' ', ' ')));
        return boost::algorithm::iequals(trimId, BY_DELIMITER);
      }
    } PACK_POST;

    PACK_PRE struct RawHeader
    {
      uint8_t Id[13];        //'ProTracker 3.'
      uint8_t Subversion;
      uint8_t Optional1[16]; //' compilation of '
      RawId Metainfo;
      uint8_t Mode;
      uint8_t FreqTableNum;
      uint8_t Tempo;
      uint8_t Length;
      uint8_t Loop;
      ruint16_t PatternsOffset;
      ruint16_t SamplesOffsets[MAX_SAMPLES_COUNT];
      ruint16_t OrnamentsOffsets[MAX_ORNAMENTS_COUNT];
      uint8_t Positions[1];//finished by marker
    } PACK_POST;

    const uint8_t POS_END_MARKER = 0xff;

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

    PACK_PRE struct RawSample : RawObject
    {
      PACK_PRE struct Line
      {
        //SUoooooE
        //NkKTLLLL
        //OOOOOOOO
        //OOOOOOOO
        // S - vol slide
        // U - vol slide up
        // o - noise or env offset
        // E - env mask
        // L - level
        // T - tone mask
        // K - keep noise or env offset
        // k - keep tone offset
        // N - noise mask
        // O - tone offset
        uint8_t VolSlideEnv;
        uint8_t LevelKeepers;
        rint16_t ToneOffset;

        bool GetEnvelopeMask() const
        {
          return 0 != (VolSlideEnv & 1);
        }

        int_t GetNoiseOrEnvelopeOffset() const
        {
          const uint8_t noeoff = (VolSlideEnv & 62) >> 1;
          return static_cast<int8_t>(noeoff & 16 ? noeoff | 0xf0 : noeoff);
        }

        int_t GetVolSlide() const
        {
          return (VolSlideEnv & 128) ? ((VolSlideEnv & 64) ? +1 : -1) : 0;
        }

        uint_t GetLevel() const
        {
          return LevelKeepers & 15;
        }

        bool GetToneMask() const
        {
          return 0 != (LevelKeepers & 16);
        }

        bool GetKeepNoiseOrEnvelopeOffset() const
        {
          return 0 != (LevelKeepers & 32);
        }

        bool GetKeepToneOffset() const
        {
          return 0 != (LevelKeepers & 64);
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelKeepers & 128);
        }

        int_t GetToneOffset() const
        {
          return fromLE(ToneOffset);
        }
      } PACK_POST;

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + std::min<std::size_t>(GetSize() * sizeof(Line), 256);
      }

      Line GetLine(uint_t idx) const
      {
        BOOST_STATIC_ASSERT(0 == (sizeof(Line) & (sizeof(Line) - 1)));
        const uint_t maxLines = 256 / sizeof(Line);
        const Line* const src = safe_ptr_cast<const Line*>(this + 1);
        return src[idx % maxLines];
      }
    } PACK_POST;

    PACK_PRE struct RawOrnament : RawObject
    {
      typedef int8_t Line;

      std::size_t GetUsedSize() const
      {
        return sizeof(RawObject) + GetSize() * sizeof(Line);
      }

      Line GetLine(uint_t idx) const
      {
        const int8_t* const src = safe_ptr_cast<const int8_t*>(this + 1);
        //using 8-bit offsets
        uint8_t offset = static_cast<uint8_t>(idx * sizeof(Line));
        return src[offset];
      }
    } PACK_POST;

    PACK_PRE struct RawPattern
    {
      ruint16_t Offsets[3];

      bool Check() const
      {
        return Offsets[0] && Offsets[1] && Offsets[2];
      }
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 202);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 2);
    BOOST_STATIC_ASSERT(sizeof(RawSample::Line) == 4);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 2);

    class StubBuilder : public Builder
    {
    public:
      virtual MetaBuilder& GetMetaBuilder()
      {
        return GetStubMetaBuilder();
      }
      virtual void SetVersion(uint_t /*version*/) {}
      virtual void SetNoteTable(NoteTable /*table*/) {}
      virtual void SetMode(uint_t /*mode*/) {}
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
      virtual void SetGlissade(uint_t /*period*/, int_t /*val*/) {}
      virtual void SetNoteGliss(uint_t /*period*/, int_t /*val*/, uint_t /*limit*/) {}
      virtual void SetSampleOffset(uint_t /*offset*/) {}
      virtual void SetOrnamentOffset(uint_t /*offset*/) {}
      virtual void SetVibrate(uint_t /*ontime*/, uint_t /*offtime*/) {}
      virtual void SetEnvelopeSlide(uint_t /*period*/, int_t /*val*/) {}
      virtual void SetEnvelope(uint_t /*type*/, uint_t /*value*/) {}
      virtual void SetNoEnvelope() {}
      virtual void SetNoiseBase(uint_t /*val*/) {}
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
        const uint8_t* const dataBegin = hdr->Id;
        const uint8_t* const dataEnd = dataBegin + std::min(data.GetSize(), MAX_POSITIONS_COUNT + offsetof(RawHeader, Positions) + 1);
        const uint8_t* const lastPosition = std::find(hdr->Positions, dataEnd, POS_END_MARKER);
        if (lastPosition != dataEnd && 
            lastPosition == std::find_if(hdr->Positions, lastPosition, std::bind2nd(std::modulus<std::size_t>(), 3)))
        {
          return lastPosition + 1 - dataBegin;
        }
      }
      return 0;
    }

    void CheckTempo(uint_t tempo)
    {
      Require(tempo >= 1);
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
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(String(Source.Id, Source.Optional1));
        const RawId& id = Source.Metainfo;
        if (id.HasAuthor())
        {
          meta.SetTitle(FromCharArray(id.TrackName));
          meta.SetAuthor(FromCharArray(id.TrackAuthor));
        }
        else
        {
          meta.SetTitle(String(id.TrackName, boost::end(id.TrackAuthor)));
        }
        const uint_t version = std::isdigit(Source.Subversion) ? Source.Subversion - '0' : 6;
        builder.SetVersion(version);
        if (Math::InRange<uint_t>(Source.FreqTableNum, PROTRACKER, NATURAL))
        {
          builder.SetNoteTable(static_cast<NoteTable>(Source.FreqTableNum));
        }
        else
        {
          builder.SetNoteTable(PROTRACKER);
        }
        builder.SetMode(Source.Mode);
        const uint_t tempo = Source.Tempo;
        CheckTempo(tempo);
        builder.SetInitialTempo(tempo);
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<uint_t> positions;
        for (const uint8_t* pos = Source.Positions; *pos != POS_END_MARKER; ++pos)
        {
          const uint_t patNum = *pos;
          Require(0 == patNum % 3);
          positions.push_back(patNum / 3);
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
        //samples are mandatory
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
          builder.SetSample(samIdx, result);
        }
        Require(hasValidSamples || hasPartialSamples);
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        Dbg("Ornaments: %1% to parse", ornaments.Count());
        //ornaments are not mandatory
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

      uint16_t PeekLEWord(std::size_t offset) const
      {
		const RawInt16* const ptr= Delegate.GetField<RawInt16>(offset);
        Require(ptr != 0);
        return fromLE(ptr->Val);
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

        ParserState state(rangesStarts);
        PatternBuilder& patBuilder = builder.StartPattern(patIndex);
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
          else if (0 == chan && 0x00 == PeekByte(state.Offset))
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
        std::vector<uint_t> commands;
        int_t note = -1;
        while (state.Offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd < 0x10)
          {
            commands.push_back(cmd);
          }
          else if ((cmd < 0x20)
                || (cmd >= 0xb2 && cmd <= 0xbf)
                || (cmd >= 0xf0))
          {
            const bool hasEnv(cmd >= 0x11 && cmd <= 0xbf);
            const bool hasOrn(cmd >= 0xf0);
            const bool hasSmp(cmd < 0xb2 || cmd > 0xbf);

            if (hasEnv) //has envelope command
            {
              const uint_t type = cmd - (cmd >= 0xb2 ? 0xb1 : 0x10);
              const uint_t tone = fromBE(PeekLEWord(state.Offset));
              state.Offset += 2;
              builder.SetEnvelope(type, tone);
            }
            else
            {
              builder.SetNoEnvelope();
            }
            if (hasOrn) //has ornament command
            {
              const uint_t num = cmd - 0xf0;
              builder.SetOrnament(num);
            }
            if (hasSmp)
            {
              const uint_t doubleSampNum = PeekByte(state.Offset++);
              const bool sampValid(doubleSampNum < MAX_SAMPLES_COUNT * 2 && 0 == (doubleSampNum & 1));
              builder.SetSample(sampValid ? (doubleSampNum / 2) : 0);
            }
          }
          else if (cmd < 0x40)
          {
            const uint_t noiseBase = cmd - 0x20;
            builder.SetNoiseBase(noiseBase);
          }
          else if (cmd < 0x50)
          {
            const uint_t num = cmd - 0x40;
            builder.SetOrnament(num);
          }
          else if (cmd < 0xb0)
          {
            note = cmd - 0x50;
            break;
          }
          else if (cmd == 0xb0)
          {
            builder.SetNoEnvelope();
          }
          else if (cmd == 0xb1)
          {
            state.Period = static_cast<uint8_t>(PeekByte(state.Offset++) - 1);
          }
          else if (cmd == 0xc0)
          {
            builder.SetRest();
            break;
          }
          else if (cmd < 0xd0)
          {
            const uint_t val = cmd - 0xc0;
            builder.SetVolume(val);
          }
          else if (cmd == 0xd0)
          {
            break;
          }  
          else if (cmd < 0xf0)
          {
            const uint_t num = cmd - 0xd0;
            builder.SetSample(num);
          }
        }
        for (std::vector<uint_t>::const_reverse_iterator it = commands.rbegin(), lim = commands.rend(); it != lim; ++it)
        {
          switch (*it)
          {
          case 1://gliss
            {
              const uint_t period = PeekByte(state.Offset++);
              const int_t step = static_cast<int16_t>(PeekLEWord(state.Offset));
              state.Offset += 2;
              builder.SetGlissade(period, step);
            }
            break;
          case 2://glissnote (portamento)
            {
              const uint_t period = PeekByte(state.Offset++);
              const uint_t limit = PeekLEWord(state.Offset);
              state.Offset += 2;
              const int_t step = static_cast<int16_t>(PeekLEWord(state.Offset));
              state.Offset += 2;
              builder.SetNoteGliss(period, step, limit);
            }
            break;
          case 3://sampleoffset
            {
              const uint_t val = PeekByte(state.Offset++);
              builder.SetSampleOffset(val);
            }
            break;
          case 4://ornamentoffset
            {
              const uint_t val = PeekByte(state.Offset++);
              builder.SetOrnamentOffset(val);
            }
            break;
          case 5://vibrate
            {
              const uint_t ontime = PeekByte(state.Offset++);
              const uint_t offtime = PeekByte(state.Offset++);
              builder.SetVibrate(ontime, offtime);
            }
            break;
          case 8://slide envelope
            {
              const uint_t period = PeekByte(state.Offset++);
              const int_t step = static_cast<int16_t>(PeekLEWord(state.Offset));
              state.Offset += 2;
              builder.SetEnvelopeSlide(period, step);
            }
            break;
          case 9://tempo
            {
              const uint_t tempo = PeekByte(state.Offset++);
              patBuilder.SetTempo(tempo);
            }
            break;
          }
        }
        if (-1 != note)
        {
          builder.SetNote(note);
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
        res.VolumeSlideAddon = line.GetVolSlide();
        res.ToneMask = line.GetToneMask();
        res.ToneOffset = line.GetToneOffset();
        res.KeepToneOffset = line.GetKeepToneOffset();
        res.NoiseMask = line.GetNoiseMask();
        res.EnvMask = line.GetEnvelopeMask();
        res.NoiseOrEnvelopeOffset = line.GetNoiseOrEnvelopeOffset();
        res.KeepNoiseOrEnvelopeOffset = line.GetKeepNoiseOrEnvelopeOffset();
        return res;
      }

      static void ParseOrnament(const RawOrnament& src, uint_t size, Ornament& dst)
      {
        dst.Lines.resize(src.GetSize());
        for (uint_t idx = 0; idx < size; ++idx)
        {
          dst.Lines[idx] = src.GetLine(idx);
        }
        dst.Loop = std::min<uint_t>(src.Loop, dst.Lines.size());
      }
    private:
      const Binary::TypedContainer& Delegate;
      RangesMap Ranges;
      const RawHeader& Source;
    };

    bool AddTSPatterns(uint_t patOffset, Indices& target)
    {
      Require(!target.Empty());
      const uint_t patsCount = target.Maximum();
      if (patOffset > patsCount && patOffset < patsCount * 2)
      {
        std::vector<uint_t> mirrored;
        for (Indices::Iterator it = target.Items(); it; ++it)
        {
          mirrored.push_back(patOffset - 1 - *it);
        }
        target.Insert(mirrored.begin(), mirrored.end());
        return true;
      }
      return false;
    }

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
      "?{13}"      // uint8_t Id[13];        //'ProTracker 3.'
      "?"          // uint8_t Subversion;
      "?{16}"      // uint8_t Optional1[16]; //' compilation of '
      "?{32}"      // char TrackName[32];
      "?{4}"       // uint8_t Optional2[4]; //' by '
      "?{32}"      // char TrackAuthor[32];
      "?"          // uint8_t Mode;
      "?"          // uint8_t FreqTableNum;
      "01-ff"      // uint8_t Tempo;
      "?"          // uint8_t Length;
      "?"          // uint8_t Loop;
      "?00-01"     // uint16_t PatternsOffset;
      "(?00-bf){32}" //boost::array<uint16_t, MAX_SAMPLES_COUNT> SamplesOffsets;
      //some of the modules has invalid offsets
      "(?00-d9){16}" //boost::array<uint16_t, MAX_ORNAMENTS_COUNT> OrnamentsOffsets;
      "*3&00-fe"     // at least one position
      "*3"      // next position or limiter (255 % 3 == 0)
    );

    class BinaryDecoder : public Decoder
    {
    public:
      BinaryDecoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::PROTRACKER3_DECODER_DESCRIPTION;
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
        return Formats::Chiptune::ProTracker3::Parse(rawData, stub);
      }

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const
      {
        return Formats::Chiptune::ProTracker3::Parse(data, target);
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

        StatisticCollectingBuilder statistic(target);
        format.ParseCommonProperties(statistic);
        format.ParsePositions(statistic);
        Indices usedPatterns = statistic.GetUsedPatterns();
        const uint_t mode = statistic.GetMode();
        if (mode != SINGLE_AY_MODE
         && !AddTSPatterns(mode, usedPatterns))
        {
          target.SetMode(SINGLE_AY_MODE);
        }
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

    Decoder::Ptr CreateDecoder()
    {
      return boost::make_shared<BinaryDecoder>();
    }
  }// namespace ProTracker3

  Decoder::Ptr CreateProTracker3Decoder()
  {
    return ProTracker3::CreateDecoder();
  }
}// namespace Chiptune
}// namespace Formats
