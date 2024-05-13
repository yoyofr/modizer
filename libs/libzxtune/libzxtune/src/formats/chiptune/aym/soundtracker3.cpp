/**
* 
* @file
*
* @brief  SoundTracker v3.x support implementation
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "soundtracker_detail.h"
#include "formats/chiptune/container.h"
#include "formats/chiptune/metainfo.h"
//common includes
#include <byteorder.h>
#include <contract.h>
#include <iterator.h>
#include <range_checker.h>
//library includes
#include <binary/format_factories.h>
#include <binary/typed_container.h>
#include <debug/log.h>
#include <math/numeric.h>
//std includes
#include <cstring>
#include <iostream>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
//text includes
#include <formats/text/chiptune.h>

namespace
{
  const Debug::Stream Dbg("Formats::Chiptune::SoundTracker3");
}

namespace Formats
{
namespace Chiptune
{
  namespace SoundTracker3
  {
    using namespace SoundTracker;

    const std::size_t MIN_SIZE = 0x180;
    const std::size_t MAX_SIZE = 0x1800;

    /*
      Typical module structure

      Header            13
      Identifier        optional - used to store data from player
      Samples           2*130=260 .. 16*130=2080
      PositionsInfo     1+1*2=3 .. 1+256*2=513
      SamplesInfo       1+2*2=5 .. 1+16*2=33
      Ornaments         2*32=64 .. 16*32=512
      OrnamentsInfo     1+2*2=5 .. 1+16*2=33
      Patterns data     ~16 .. ~0x3000
      PatternsInfo      6 .. 252 (multiply by 6)
    */

#ifdef USE_PRAGMA_PACK
#pragma pack(push,1)
#endif
    PACK_PRE struct RawHeader
    {
      uint8_t Tempo;
      ruint16_t PositionsOffset;
      ruint16_t SamplesOffset;
      ruint16_t OrnamentsOffset;
      ruint16_t PatternsOffset;
    } PACK_POST;

    const uint8_t ID[] =
    {
      'K', 'S', 'A', ' ', 'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', ' ',
      'C', 'O', 'M', 'P', 'I', 'L', 'A', 'T', 'I', 'O', 'N', ' ', 'O', 'F', ' '
    };

    PACK_PRE struct RawId
    {
      uint8_t Id[sizeof(ID)];
      char Title[27];

      bool Check() const
      {
        return 0 == std::memcmp(Id, ID, sizeof(Id));
      }
    } PACK_POST;

    PACK_PRE struct RawPositions
    {
      uint8_t Length;
      PACK_PRE struct PosEntry
      {
        int8_t Transposition;
        uint8_t PatternOffset;//*6
      } PACK_POST;
      PosEntry Data[1];
    } PACK_POST;

    PACK_PRE struct RawUInt16	// we need to use wrapper when pointers to the contained value are used..
    {
      ruint16_t Val;
    } PACK_POST;
	
    PACK_PRE struct RawPattern
    {
      RawUInt16 Offsets[3];
    } PACK_POST;

    PACK_PRE struct RawSamples
    {
      uint8_t Count;
      RawUInt16 Offsets[1];
    } PACK_POST;

    PACK_PRE struct RawOrnaments
    {
      uint8_t Count;
      RawUInt16 Offsets[1];
    } PACK_POST;

    PACK_PRE struct RawOrnament
    {
      int8_t Data[ORNAMENT_SIZE];
    } PACK_POST;

    PACK_PRE struct RawSample
    {
      uint8_t Loop;
      uint8_t LoopLimit;

      //NxxTAAAA
      //nnnnnnnn
      // N - noise mask
      // T - tone mask
      // A - Level
      // n - noise
      PACK_PRE struct Line
      {
        rint16_t Vibrato;
        uint8_t LevelAndMask;
        uint8_t Noise;

        uint_t GetLevel() const
        {
          return LevelAndMask & 15;
        }

        int_t GetEffect() const
        {
          return fromLE(Vibrato);
        }

        uint_t GetNoise() const
        {
          return Noise;
        }

        bool GetNoiseMask() const
        {
          return 0 != (LevelAndMask & 128);
        }

        bool GetToneMask() const
        {
          return 0 != (LevelAndMask & 16);
        }
      } PACK_POST;
      Line Data[SAMPLE_SIZE];
    } PACK_POST;
#ifdef USE_PRAGMA_PACK
#pragma pack(pop)
#endif

    BOOST_STATIC_ASSERT(sizeof(RawHeader) == 9);
    BOOST_STATIC_ASSERT(sizeof(RawId) == 55);
    BOOST_STATIC_ASSERT(sizeof(RawPositions) == 3);
    BOOST_STATIC_ASSERT(sizeof(RawPattern) == 6);
    BOOST_STATIC_ASSERT(sizeof(RawSamples) == 3);
    BOOST_STATIC_ASSERT(sizeof(RawOrnaments) == 3);
    BOOST_STATIC_ASSERT(sizeof(RawOrnament) == 32);
    BOOST_STATIC_ASSERT(sizeof(RawSample) == 130);

    class Format
    {
    public:
      explicit Format(const Binary::TypedContainer& data)
        : Delegate(data)
        , Source(*Delegate.GetField<RawHeader>(0))
        , Id(*Delegate.GetField<RawId>(sizeof(Source)))
        , TotalRanges(RangeChecker::CreateSimple(Delegate.GetSize()))
        , FixedRanges(RangeChecker::CreateSimple(Delegate.GetSize()))
      {
        AddRange(0, sizeof(Source));
        if (Id.Check())
        {
          AddRange(sizeof(Source), sizeof(Id));
        }
      }

      void ParseCommonProperties(Builder& builder) const
      {
        builder.SetInitialTempo(Source.Tempo);
        MetaBuilder& meta = builder.GetMetaBuilder();
        meta.SetProgram(Text::SOUNDTRACKER3_DECODER_DESCRIPTION);
        if (Id.Check())
        {
          meta.SetTitle(FromCharArray(Id.Title));
        }
      }

      void ParsePositions(Builder& builder) const
      {
        std::vector<PositionEntry> positions;
        for (RangeIterator<const RawPositions::PosEntry*> iter = GetPositions(); iter; ++iter)
        {
          const RawPositions::PosEntry& src = *iter;
          Require(0 == src.PatternOffset % sizeof(RawPattern));
          PositionEntry dst;
          dst.PatternIndex = src.PatternOffset / sizeof(RawPattern);
          dst.Transposition = src.Transposition;
          positions.push_back(dst);
        }
        builder.SetPositions(positions);
        Dbg("Positions: %1% entries", positions.size());
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
        const RawSamples& table = GetObject<RawSamples>(fromLE(Source.SamplesOffset));
        const uint_t availSamples = table.Count;
        Require(samples.Maximum() < availSamples);
        for (Indices::Iterator it = samples.Items(); it; ++it)
        {
          const uint_t samIdx = *it;
          const std::size_t samOffset = fromLE(table.Offsets[samIdx].Val);
          Dbg("Parse sample %1% at %2%", samIdx, samOffset);
          const RawSample& src = GetObject<RawSample>(samOffset);
          const Sample& result = ParseSample(src);
          builder.SetSample(samIdx, result);
        }
      }

      void ParseOrnaments(const Indices& ornaments, Builder& builder) const
      {
        const RawOrnaments& table = GetObject<RawOrnaments>(fromLE(Source.OrnamentsOffset));
        const uint_t availOrnaments = table.Count;
        Require(ornaments.Maximum() < availOrnaments);
        for (Indices::Iterator it = ornaments.Items(); it; ++it)
        {
          const uint_t ornIdx = *it;
          const std::size_t ornOffset = fromLE(table.Offsets[ornIdx].Val);
          Dbg("Parse ornament %1% at %2%", ornIdx, ornOffset);
          const RawOrnament& src = GetObject<RawOrnament>(ornOffset);
          const Ornament result(&(src.Data[0]), &(src.Data[ORNAMENT_SIZE]));
          builder.SetOrnament(ornIdx, result);
        }
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
      RangeIterator<const RawPositions::PosEntry*> GetPositions() const
      {
        const std::size_t offset = fromLE(Source.PositionsOffset);
        const RawPositions* const positions = Delegate.GetField<RawPositions>(offset);
        Require(positions != 0);
        const uint_t length = positions->Length;
        AddRange(offset, sizeof(*positions) + (length - 1) * sizeof(RawPositions::PosEntry));
        const RawPositions::PosEntry* const firstEntry = positions->Data;
        const RawPositions::PosEntry* const lastEntry = firstEntry + length;
        return RangeIterator<const RawPositions::PosEntry*>(firstEntry, lastEntry);
      }

      template<class T>
      const T& GetObject(std::size_t offset) const
      {
        const T* const src = Delegate.GetField<T>(offset);
        Require(src != 0);
        AddRange(offset, sizeof(T));
        return *src;
      }

      const RawPattern& GetPattern(uint_t idx) const
      {
        const std::size_t patOffset = fromLE(Source.PatternsOffset) + idx * sizeof(RawPattern);
        return GetObject<RawPattern>(patOffset);
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
				at(i)= fromLE(src.Offsets[i].Val);
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

      void ParsePattern(uint_t patIndex, Builder& builder) const
      {
        const RawPattern& src = GetPattern(patIndex);
        const DataCursors rangesStarts(src);
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
            Dbg("Affected ranges %1%..%2%", start, stop);
            AddFixedRange(start, stop - start);
          }
        }
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
        while (state.Offset < Delegate.GetSize())
        {
          const uint_t cmd = PeekByte(state.Offset++);
          if (cmd <= 0x5f)//note
          {
            builder.SetNote(cmd);
            break;
          }
          else if (cmd <= 0x6f)//sample
          {
            builder.SetSample(cmd - 0x60);
          }
          else if (cmd <= 0x7f)//ornament
          {
            builder.SetNoEnvelope();
            builder.SetOrnament(cmd - 0x70);
          }
          else if (cmd == 0x80)//reset
          {
            builder.SetRest();
            break;
          }
          else if (cmd == 0x81)//empty
          {
            break;
          }
          else if (cmd == 0x82) //orn 0 without envelope
          {
            builder.SetOrnament(0);
            builder.SetNoEnvelope();
          }
          else if (cmd <= 0x8e)//orn 0, with envelope
          {
            builder.SetOrnament(0);
            const uint_t envPeriod = PeekByte(state.Offset++);
            builder.SetEnvelope(cmd - 0x80, envPeriod);
          }
          else
          {
            state.Period = (cmd - 0xa1) & 0xff;
          }
        }
      }

      static Sample ParseSample(const RawSample& src)
      {
        Sample dst;
        dst.Lines.resize(SAMPLE_SIZE);
        for (uint_t idx = 0; idx < SAMPLE_SIZE; ++idx)
        {
          const RawSample::Line& line = src.Data[idx];
          Sample::Line& res = dst.Lines[idx];
          res.Level = line.GetLevel();
          res.Noise = line.GetNoise();
          res.NoiseMask = line.GetNoiseMask();
          res.EnvelopeMask = line.GetToneMask();
          res.Effect = line.GetEffect();
        }
        dst.Loop = std::min<uint_t>(src.Loop, SAMPLE_SIZE);
        dst.LoopLimit = std::min<uint_t>(src.Loop + src.LoopLimit + 1, SAMPLE_SIZE);
        return dst;
      }

      void AddRange(std::size_t offset, std::size_t size) const
      {
        Require(TotalRanges->AddRange(offset, size));
      }

      void AddFixedRange(std::size_t offset, std::size_t size) const
      {
        AddRange(offset, size);
        Require(FixedRanges->AddRange(offset, size));
      }
    private:
      const Binary::TypedContainer Delegate;
      const RawHeader& Source;
      const RawId& Id;
      const RangeChecker::Ptr TotalRanges;
      const RangeChecker::Ptr FixedRanges;
    };

    enum AreaTypes
    {
      HEADER,
      IDENTIFIER,
      SAMPLES,
      POSITIONS_INFO,
      SAMPLES_INFO,
      ORNAMENTS,
      ORNAMENTS_INFO,
      PATTERNS_INFO,
      END
    };

    struct Areas : public AreaController
    {
      Areas(const RawHeader& header, std::size_t size)
      {
        AddArea(HEADER, 0);
        const std::size_t idOffset = sizeof(header);
        if (idOffset + sizeof(RawId) <= size)
        {
          const RawId* const id = safe_ptr_cast<const RawId*>(&header + 1);
          if (id->Check())
          {
            AddArea(IDENTIFIER, sizeof(header));
          }
        }
        AddArea(POSITIONS_INFO, fromLE(header.PositionsOffset));
        AddArea(SAMPLES_INFO, fromLE(header.SamplesOffset));
        AddArea(ORNAMENTS_INFO, fromLE(header.OrnamentsOffset));
        AddArea(PATTERNS_INFO, fromLE(header.PatternsOffset));
        AddArea(END, size);
      }

      bool CheckHeader() const
      {
        return sizeof(RawHeader) <= GetAreaSize(HEADER) && Undefined == GetAreaSize(END);
      }

      bool CheckPositions(uint_t count) const
      {
        if (!Math::InRange<uint_t>(count, 1, MAX_POSITIONS_COUNT))
        {
          return false;
        }
        const std::size_t realSize = GetAreaSize(POSITIONS_INFO);
        const std::size_t requiredSize = sizeof(RawPositions) + (count - 1) * sizeof(RawPositions::PosEntry);
        return realSize != Undefined && realSize >= requiredSize;
      }

      bool CheckSamples(uint_t count) const
      {
        if (!Math::InRange<uint_t>(count, 1, MAX_SAMPLES_COUNT))
        {
          return false;
        }
        {
          const std::size_t realSize = GetAreaSize(SAMPLES_INFO);
          const std::size_t requiredSize = sizeof(RawSamples) + (count - 1) * sizeof(uint16_t);
          if (realSize == Undefined || realSize < requiredSize)
          {
            return false;
          }
        }
        {
          const std::size_t realSize = GetAreaSize(SAMPLES);
          const std::size_t requiredSize = count * sizeof(RawSample);
          return realSize != Undefined && realSize >= requiredSize;
        }
      }

      bool CheckOrnaments(uint_t count) const
      {
        if (!Math::InRange<uint_t>(count, 1, MAX_ORNAMENTS_COUNT))
        {
          return false;
        }
        {
          const std::size_t realSize = GetAreaSize(ORNAMENTS_INFO);
          const std::size_t requiredSize = sizeof(RawOrnaments) + (count - 1) * sizeof(uint16_t);
          if (realSize == Undefined || realSize < requiredSize)
          {
            return false;
          }
        }
        {
          const std::size_t realSize = GetAreaSize(ORNAMENTS);
          const std::size_t requiredSize = count * sizeof(RawOrnament);
          return realSize != Undefined && realSize >= requiredSize;
        }
      }
    };

    Binary::TypedContainer CreateContainer(const Binary::Container& rawData)
    {
      const std::size_t size = std::min(rawData.Size(), MAX_SIZE);
      return Binary::TypedContainer(rawData, size);
    }

    bool IsInvalidPosEntry(const RawPositions::PosEntry& entry)
    {
      return 0 != entry.PatternOffset % sizeof(RawPattern);
    }

    bool AreSequenced(ruint16_t lh, ruint16_t rh, std::size_t multiply)
    {
      const std::size_t lhNorm = fromLE(lh);
      const std::size_t rhNorm = fromLE(rh);
      if (lhNorm > rhNorm)
      {
        return false;
      }
      else if (lhNorm < rhNorm)
      {
        return 0 == (rhNorm - lhNorm) % multiply;
      }
      return true;
    }

	template <class T>
	const RawUInt16* adjacent_find(const T* const rawdata){
	  // the original code does not work because the uint16_t array "Offsets" might not be aligned:
	 //	return std::adjacent_find(rawdata->Offsets, rawdata->Offsets + rawdata->Count,
	 //		!boost::bind(&AreSequenced, _1, _2, sizeof(T)))
		
		size_t first= 0, last= rawdata->Count;	// last is the pos behind the last check field..
		if (first == last) {
			return &rawdata->Offsets[last];
		}
		size_t next = first;
		++next;
		for (; next != last; ++next, ++first) {
			if (AreSequenced(rawdata->Offsets[first].Val, rawdata->Offsets[next].Val, sizeof(T))) {
				return &rawdata->Offsets[first];
			}
		}
		return &rawdata->Offsets[last];
	}

	const RawPositions::PosEntry* find_if(const RawPositions* positions, const uint_t count){		
//		return std::find_if(positions->Data, positions->Data + count, &IsInvalidPosEntry);

		for (int i= 0; i<count; i++) {
			const RawPositions::PosEntry &p= positions->Data[i];
			if (IsInvalidPosEntry(p))
				return &p;
		}
		return positions->Data + count*sizeof(RawPositions::PosEntry);
	}
	
    bool FastCheck(const Binary::TypedContainer& data)
    {
      const RawHeader* const hdr = data.GetField<RawHeader>(0);
      if (0 == hdr)
      {
        return false;
      }
      Areas areas(*hdr, data.GetSize());
      const RawSamples* const samples = data.GetField<RawSamples>(areas.GetAreaAddress(SAMPLES_INFO));
      const RawOrnaments* const ornaments = data.GetField<RawOrnaments>(areas.GetAreaAddress(ORNAMENTS_INFO));
      if (!samples || !ornaments)
      {
        return false;
      }
      areas.AddArea(SAMPLES, fromLE(samples->Offsets[0].Val));
      areas.AddArea(ORNAMENTS, fromLE(ornaments->Offsets[0].Val));

      if (!areas.CheckHeader())
      {
        return false;
      }
      if (!areas.CheckSamples(samples->Count))
      {
        return false;
      }
      if (!areas.CheckOrnaments(ornaments->Count))
      {
        return false;
      }
//      if (samples->Offsets + samples->Count != adjacent_find<RawSamples>(samples))
      if (((char*)samples->Offsets + (samples->Count*sizeof(uint16_t))) != (char*)adjacent_find(samples))
      {
        return false;
      }
  //    if (ornaments->Offsets + ornaments->Count != adjacent_find<RawOrnaments>(ornaments))
      if (((char*)ornaments->Offsets + (ornaments->Count*sizeof(uint16_t))) != (char*)adjacent_find(ornaments))
      {
        return false;
      }
      if (const RawPositions* positions = data.GetField<RawPositions>(areas.GetAreaAddress(POSITIONS_INFO)))
      {
        const uint_t count = positions->Length;
        if (!areas.CheckPositions(count))
        {
          return false;
        }
        if (positions->Data + count*sizeof(RawPositions::PosEntry) != find_if(positions, count))
 //       if (positions->Data + count != find_if(positions, count))
        {
          return false;
        }
      }
      else
      {
        return false;
      }
      return true;
    }

    bool FastCheck(const Binary::Container& rawData)
    {
      const Binary::TypedContainer data(CreateContainer(rawData));
      return FastCheck(data);
    }

    const std::string FORMAT(
    "03-0f"       // uint8_t Tempo; 1..15
    "?01-08"      // uint16_t PositionsOffset;
    "?01-08"      // uint16_t SamplesOffset;
    "?01-0a"      // uint16_t OrnamentsOffset;
    "?02-16"      // uint16_t PatternsOffset;
    );

    class Decoder : public Formats::Chiptune::SoundTracker::Decoder
    {
    public:
      Decoder()
        : Format(Binary::CreateFormat(FORMAT, MIN_SIZE))
      {
      }

      virtual String GetDescription() const
      {
        return Text::SOUNDTRACKER3_DECODER_DESCRIPTION;
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
        return SoundTracker::Ver3::Parse(rawData, stub);
      }

      virtual Formats::Chiptune::Container::Ptr Parse(const Binary::Container& data, Builder& target) const
      {
        return SoundTracker::Ver3::Parse(data, target);
      }
    private:
      const Binary::Format::Ptr Format;
    };
  }//SoundTracker3

  namespace SoundTracker
  {
    namespace Ver3
    {
      Decoder::Ptr CreateDecoder()
      {
        return boost::make_shared<SoundTracker3::Decoder>();
      }

      Formats::Chiptune::Container::Ptr Parse(const Binary::Container& rawData, Builder& target)
      {
        using namespace SoundTracker3;

        const Binary::TypedContainer data(CreateContainer(rawData));

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

      Binary::Container::Ptr InsertMetainformation(const Binary::Container& rawData, const Dump& info)
      {
        using namespace SoundTracker3;
        StatisticCollectingBuilder statistic(GetStubBuilder());
        if (Binary::Container::Ptr parsed = Parse(rawData, statistic))
        {
          const Binary::TypedContainer typedHelper(CreateContainer(*parsed));
          const RawHeader& header = *typedHelper.GetField<RawHeader>(0);
          const std::size_t headerSize = sizeof(header);
          const std::size_t infoSize = info.size();
          const PatchedDataBuilder::Ptr patch = PatchedDataBuilder::Create(*parsed);
          const RawId* const id = typedHelper.GetField<RawId>(headerSize);
          if (id && id->Check())
          {
            patch->OverwriteData(headerSize, info);
          }
          else
          {
            patch->InsertData(headerSize, info);
            const int_t delta = static_cast<int_t>(infoSize);
            patch->FixLEWord(offsetof(RawHeader, PositionsOffset), delta);
            patch->FixLEWord(offsetof(RawHeader, SamplesOffset), delta);
            patch->FixLEWord(offsetof(RawHeader, OrnamentsOffset), delta);
            patch->FixLEWord(offsetof(RawHeader, PatternsOffset), delta);
            const std::size_t patternsStart = fromLE(header.PatternsOffset);
            const Indices& usedPatterns = statistic.GetUsedPatterns();
            for (Indices::Iterator it = usedPatterns.Items(); it; ++it)
            {
              const std::size_t patOffsets = patternsStart + *it * sizeof(RawPattern);
              patch->FixLEWord(patOffsets + 0, delta);
              patch->FixLEWord(patOffsets + 2, delta);
              patch->FixLEWord(patOffsets + 4, delta);
            }
            //fix all samples and ornaments
            const std::size_t ornamentsStart = fromLE(header.OrnamentsOffset);
            const RawOrnaments& orns = *typedHelper.GetField<RawOrnaments>(ornamentsStart);
            for (uint_t idx = 0; idx != orns.Count; ++idx)
            {
              const std::size_t ornOffset = ornamentsStart + offsetof(RawOrnaments, Offsets) + idx * sizeof(uint16_t);
              patch->FixLEWord(ornOffset, delta);
            }
            const std::size_t samplesStart = fromLE(header.SamplesOffset);
            const RawSamples& sams = *typedHelper.GetField<RawSamples>(samplesStart);
            for (uint_t idx = 0; idx != sams.Count; ++idx)
            {
              const std::size_t samOffset = samplesStart + offsetof(RawSamples, Offsets) + idx * sizeof(uint16_t);
              patch->FixLEWord(samOffset, delta);
            }
          }
          return patch->GetResult();
        }
        return Binary::Container::Ptr();
      }
    }
  }

  Formats::Chiptune::Decoder::Ptr CreateSoundTracker3Decoder()
  {
    return SoundTracker::Ver3::CreateDecoder();
  }
}// namespace Chiptune
}// namespace Formats
