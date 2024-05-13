/**
* 
* @file
*
* @brief  DAC support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include <devices/dac.h>
#include <devices/details/freq_table.h>
#include <devices/details/parameters_helper.h>
//common includes
#include <pointers.h>
//library includes
#include <math/numeric.h>
#include <sound/chunk_builder.h>
//std includes
#include <cmath>
//boost includes
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/scoped_array.hpp>

namespace Devices
{
namespace DAC
{
  const uint_t NO_INDEX = uint_t(-1);

  class FastSample
  {
  public:
    typedef boost::shared_ptr<const FastSample> Ptr;

    //use additional sample for interpolation
    explicit FastSample(std::size_t idx, Sample::Ptr in)
      : Index(static_cast<uint_t>(idx))
      , Rms(in->Rms())
      , Data(new Sound::Sample::Type[in->Size() + 1])
      , Size(in->Size())
      , Loop(std::min(Size, in->Loop()))
    {
      for (std::size_t pos = 0; pos != Size; ++pos)
      {
        Data[pos] = in->Get(pos);
      }
      Data[Size] = Data[Size - 1];
    }

    FastSample()
      : Index(NO_INDEX)
      , Rms(0)
      , Data(new Sound::Sample::Type[2])
      , Size(1)
      , Loop(1)
    {
    }

    uint_t GetIndex() const
    {
      return Index;
    }

    uint_t GetRms() const
    {
      return Rms;
    }

    typedef Math::FixedPoint<uint_t, 256> Position;

    class Iterator
    {
    public:
      Iterator()
        : Data()
        , Step()
        , Limit()
        , Loop()
        , Pos()
      {
      }

      bool IsValid() const
      {
        return Data && Pos < Limit;
      }

      uint_t GetPosition() const
      {
        return Pos.Integer();
      }

      Sound::Sample::Type GetNearest() const
      {
        assert(IsValid());
        return Data[Pos.Integer()];
      }

      Sound::Sample::Type GetInterpolated(const uint_t* lookup) const
      {
        assert(IsValid());
        if (const int_t fract = Pos.Fraction())
        {
          const Sound::Sample::Type* const cur = Data + Pos.Integer();
          const int_t curVal = *cur;
          const int_t nextVal = *(cur + 1);
          const int_t delta = nextVal - curVal;
          return static_cast<Sound::Sample::Type>(curVal + delta * lookup[fract] / Position::PRECISION);
        }
        else
        {
          return Data[Pos.Integer()];
        }
      }

      void Reset()
      {
        Pos = 0;
      }

      void Next()
      {
        Pos = GetNextPos();
      }

      void SetPosition(uint_t pos)
      {
        const Position newPos = Position(pos);
        if (newPos < Pos)
        {
          Pos = newPos;
        }
      }

      void SetStep(Position step)
      {
        Step = step;
      }

      void SetSample(const FastSample& sample)
      {
        Data = sample.Data.get();
        Limit = sample.Size;
        Loop = sample.Loop;
        Pos = std::min(Pos, Limit);
      }
    private:
      Position GetNextPos() const
      {
        const Position res = Pos + Step;
        return res < Limit ? res : Loop;
      }
    private:
      const Sound::Sample::Type* Data;
      Position Step;
      Position Limit;
      Position Loop;
      Position Pos;
    };
  private:
    friend class Iterator;
    const uint_t Index;
    const uint_t Rms;
    const boost::scoped_array<Sound::Sample::Type> Data;
    const std::size_t Size;
    const std::size_t Loop;
  };

  class ClockSource
  {
  public:
    ClockSource()
      : SampleFreq()
      , SoundFreq()
    {
      Reset();
    }

    void Reset()
    {
      CurrentTime = Stamp();
    }

    void SetFreq(uint_t sampleFreq, uint_t soundFreq)
    {
      SampleFreq = sampleFreq;
      SoundFreq = soundFreq;
    }

    FastSample::Position GetStep(int_t halftones, int_t slideHz) const
    {
      const int_t halftonesLim = Math::Clamp<int_t>(halftones, 0, Details::FreqTable::SIZE - 1);
      const Details::Frequency baseFreq = Details::FreqTable::GetHalftoneFrequency(0);
      const Details::Frequency freq = Details::FreqTable::GetHalftoneFrequency(halftonesLim) + Details::Frequency(slideHz);
      //step 1 is for first note
      return FastSample::Position(int64_t((freq * SampleFreq).Integer()), (baseFreq * SoundFreq).Integer());
    }

    Stamp GetCurrentTime() const
    {
      return CurrentTime;
    }

    uint_t Advance(Stamp nextTime)
    {
      const Stamp now = CurrentTime;
      CurrentTime = nextTime;
      return static_cast<uint_t>(uint64_t(nextTime.Get() - now.Get()) * SoundFreq / CurrentTime.PER_SECOND);
    }
  private:
    uint_t SampleFreq;
    uint_t SoundFreq;
    Stamp CurrentTime;
  };

  class SamplesStorage
  {
  public:
    SamplesStorage()
      : MaxRms()
    {
    }

    void Add(std::size_t idx, Sample::Ptr sample)
    {
      if (sample)
      {
        Content.resize(std::max(Content.size(), idx + 1));
        const FastSample::Ptr fast = boost::make_shared<FastSample>(idx, sample);
        Content[idx] = fast;
        MaxRms = std::max(MaxRms, fast->GetRms());
      }
    }

    FastSample::Ptr Get(std::size_t idx) const
    {
      static FastSample STUB;
      if (const FastSample::Ptr val = idx < Content.size() ? Content[idx] : FastSample::Ptr())
      {
        return val;
      }
      return MakeSingletonPointer(STUB);
    }

    uint_t GetMaxRms() const
    {
      return MaxRms;
    }
  private:
    uint_t MaxRms;
    std::vector<FastSample::Ptr> Content;
  };

  typedef Math::FixedPoint<int, LevelType::PRECISION> SignedLevelType;

  //channel state type
  struct ChannelState
  {
    ChannelState()
      : Enabled()
      , Note()
      , NoteSlide()
      , FreqSlide()
      , Level(1)
    {
    }

    explicit ChannelState(FastSample::Ptr sample)
      : Enabled()
      , Note()
      , NoteSlide()
      , FreqSlide()
      , Source(sample)
      , Level(1)
    {
    }

    bool Enabled;
    uint_t Note;
    //current slide in halftones
    int_t NoteSlide;
    //current slide in Hz
    int_t FreqSlide;
    //sample
    FastSample::Ptr Source;
    FastSample::Iterator Iterator;
    SignedLevelType Level;

    void Update(const SamplesStorage& samples, const ClockSource& clock, const ChannelData& state)
    {
      //'enabled' field changed
      if (const bool* enabled = state.GetEnabled())
      {
        Enabled = *enabled;
      }
      //note changed
      if (const uint_t* note = state.GetNote())
      {
        Note = *note;
      }
      //note slide changed
      if (const int_t* noteSlide = state.GetNoteSlide())
      {
        NoteSlide = *noteSlide;
      }
      //frequency slide changed
      if (const int_t* freqSlideHz = state.GetFreqSlideHz())
      {
        FreqSlide = *freqSlideHz;
      }
      //sample changed
      if (const uint_t* sampleNum = state.GetSampleNum())
      {
        Source = samples.Get(*sampleNum);
        Iterator.SetSample(*Source);
      }
      //position in sample changed
      if (const uint_t* posInSample = state.GetPosInSample())
      {
        assert(Source);
        Iterator.SetPosition(*posInSample);
      }
      //level changed
      if (const LevelType* level = state.GetLevel())
      {
        Level = *level;
      }
      if (0 != (state.Mask & (ChannelData::NOTE | ChannelData::NOTESLIDE | ChannelData::FREQSLIDEHZ)))
      {
        Iterator.SetStep(clock.GetStep(Note + NoteSlide, FreqSlide));
      }
    }

    Sound::Sample::Type GetNearest() const
    {
      return Enabled
        ? Amplify(Iterator.GetNearest())
        : Sound::Sample::MID;
    }

    Sound::Sample::Type GetInterpolated(const uint_t* lookup) const
    {
      return Enabled
        ? Amplify(Iterator.GetInterpolated(lookup))
        : Sound::Sample::MID;
    }

    void Next()
    {
      if (Enabled)
      {
        Iterator.Next();
        Enabled = Iterator.IsValid();
      }
    }

    Devices::ChannelState Analyze(uint_t maxRms) const
    {
      assert(Enabled);
      const uint_t rms = Source->GetRms();
      const LevelType level = Level * rms / maxRms;
      return Devices::ChannelState(Note + NoteSlide, level);
    }
  private:
    Sound::Sample::Type Amplify(Sound::Sample::Type val) const
    {
      return (Level * val).Round();
    }
  };

  class Renderer
  {
  public:
    virtual ~Renderer() {}

    virtual void RenderData(uint_t samples, Sound::ChunkBuilder& target) = 0;
  };

  template<unsigned Channels>
  class LQRenderer : public Renderer
  {
  public:
    LQRenderer(const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : Mixer(mixer)
      , State(state)
    {
    }

    virtual void RenderData(uint_t samples, Sound::ChunkBuilder& target)
    {
      typename Sound::MultichannelSample<Channels>::Type result;
      for (uint_t counter = samples; counter != 0; --counter)
      {
        for (uint_t chan = 0; chan != Channels; ++chan)
        {
          ChannelState& state = State[chan];
          result[chan] = state.GetNearest();
          state.Next();
        }
        target.Add(Mixer.ApplyData(result));
      }
    }
  private:
    const Sound::FixedChannelsMixer<Channels>& Mixer;
    ChannelState* const State;
  };

  template<unsigned Channels>
  class MQRenderer : public Renderer
  {
  public:
    MQRenderer(const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : Mixer(mixer)
      , State(state)
    {
    }

    virtual void RenderData(uint_t samples, Sound::ChunkBuilder& target)
    {
      static const CosineTable COSTABLE;
      typename Sound::MultichannelSample<Channels>::Type result;
      for (uint_t counter = samples; counter != 0; --counter)
      {
        for (uint_t chan = 0; chan != Channels; ++chan)
        {
          ChannelState& state = State[chan];
          result[chan] = state.GetInterpolated(COSTABLE.Get());
          state.Next();
        }
        target.Add(Mixer.ApplyData(result));
      }
    }
  private:
    class CosineTable
    {
    public:
      CosineTable()
      {
        for (uint_t idx = 0; idx != FastSample::Position::PRECISION; ++idx)
        {
          const double rad = 3.14159265358 * idx / FastSample::Position::PRECISION;
          Table[idx] = static_cast<uint_t>(FastSample::Position::PRECISION * (1.0 - cos(rad)) / 2.0);
        }
      }

      const uint_t* Get() const
      {
        return &Table[0];
      }
    private:
      boost::array<uint_t, FastSample::Position::PRECISION> Table;
    };
  private:
    const Sound::FixedChannelsMixer<Channels>& Mixer;
    ChannelState* const State;
  };

  template<unsigned Channels>
  class RenderersSet
  {
  public:
    RenderersSet(const Sound::FixedChannelsMixer<Channels>& mixer, ChannelState* state)
      : LQ(mixer, state)
      , MQ(mixer, state)
      , Current()
      , State(state)
    {
    }

    void Reset()
    {
      Current = 0;
    }

    void SetInterpolation(bool type)
    {
      if (type)
      {
        Current = &MQ;
      }
      else
      {
        Current = &LQ;
      }
    }

    void RenderData(uint_t samples, Sound::ChunkBuilder& target)
    {
      Current->RenderData(samples, target);
    }

    void DropData(uint_t samples)
    {
      for (uint_t count = samples; count != 0; --count)
      {
        std::for_each(State, State + Channels, std::mem_fun_ref(&ChannelState::Next));
      }
    }
  private:
    LQRenderer<Channels> LQ;
    MQRenderer<Channels> MQ;
    Renderer* Current;
    ChannelState* const State;
  };

  template<unsigned Channels>
  class FixedChannelsChip : public Chip
  {
  public:
    FixedChannelsChip(ChipParameters::Ptr params, typename Sound::FixedChannelsMixer<Channels>::Ptr mixer, Sound::Receiver::Ptr target)
      : Params(params)
      , Mixer(mixer)
      , Target(target)
      , Clock()
      , Renderers(*Mixer, &State[0])
    {
      FixedChannelsChip::Reset();
    }

    /// Set sample for work
    virtual void SetSample(uint_t idx, Sample::Ptr sample)
    {
      Samples.Add(idx, sample);
    }

    virtual void RenderData(const DataChunk& src)
    {
      SynchronizeParameters();
      if (Clock.GetCurrentTime() < src.TimeStamp)
      {
        RenderChunksTill(src.TimeStamp);
      }
      std::for_each(src.Data.begin(), src.Data.end(),
        boost::bind(&FixedChannelsChip::UpdateChannelState, this, _1));
    }

    virtual void UpdateState(const DataChunk& src)
    {
      SynchronizeParameters();
      if (Clock.GetCurrentTime() < src.TimeStamp)
      {
        DropChunksTill(src.TimeStamp);
      }
      std::for_each(src.Data.begin(), src.Data.end(),
        boost::bind(&FixedChannelsChip::UpdateChannelState, this, _1));
    }

    virtual void GetState(MultiChannelState& state) const
    {
      MultiChannelState res;
      res.reserve(State.size());
      for (const ChannelState* it = State.begin(), *lim = State.end(); it != lim; ++it)
      {
        if (it->Enabled)
        {
          res.push_back(it->Analyze(Samples.GetMaxRms()));
        }
      }
      res.swap(state);
    }

    /// reset internal state to initial
    virtual void Reset()
    {
      Params.Reset();
      Clock.Reset();
      Renderers.Reset();
      std::fill(State.begin(), State.end(), ChannelState(Samples.Get(0)));
    }

  private:
    void SynchronizeParameters()
    {
      if (Params.IsChanged())
      {
        Clock.SetFreq(Params->BaseSampleFreq(), Params->SoundFreq());
        Renderers.SetInterpolation(Params->Interpolate());
      }
    }

    void RenderChunksTill(Stamp stamp)
    {
      const uint_t samples = Clock.Advance(stamp);
      Sound::ChunkBuilder builder;
      builder.Reserve(samples);
      Renderers.RenderData(samples, builder);
      Target->ApplyData(builder.GetResult());
      Target->Flush();
    }

    void DropChunksTill(Stamp stamp)
    {
      const uint_t samples = Clock.Advance(stamp);
      Renderers.DropData(samples);
    }

    void UpdateChannelState(const ChannelData& state)
    {
      assert(state.Channel < State.size());
      State[state.Channel].Update(Samples, Clock, state);
    }
  private:
    Details::ParametersHelper<ChipParameters> Params;
    const typename Sound::FixedChannelsMixer<Channels>::Ptr Mixer;
    const Sound::Receiver::Ptr Target;
    SamplesStorage Samples;
    ClockSource Clock;
    boost::array<ChannelState, Channels> State;
    RenderersSet<Channels> Renderers;
  };

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::ThreeChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target)
  {
    return boost::make_shared<FixedChannelsChip<3> >(params, mixer, target);
  }

  Chip::Ptr CreateChip(ChipParameters::Ptr params, Sound::FourChannelsMixer::Ptr mixer, Sound::Receiver::Ptr target)
  {
    return boost::make_shared<FixedChannelsChip<4> >(params, mixer, target);
  }
}
}
