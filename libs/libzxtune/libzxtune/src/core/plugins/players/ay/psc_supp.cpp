/**
* 
* @file
*
* @brief  ProSoundCreator support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "aym_base.h"
#include "aym_base_track.h"
#include "aym_plugin.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/simple_orderlist.h"
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/aym/prosoundcreator.h>
#include <math/numeric.h>

namespace Module
{
namespace ProSoundCreator
{
  /*
    Do not use GLISS_NOTE command due to possible ambiguation while parsing
  */
  //supported commands and parameters
  enum CmdType
  {
    //no parameters
    EMPTY,
    //r13,period or nothing
    ENVELOPE,
    //no parameters
    NOENVELOPE,
    //noise base
    NOISE_BASE,
    //no params
    BREAK_SAMPLE,
    //no params
    BREAK_ORNAMENT,
    //no params
    NO_ORNAMENT,
    //absolute gliss
    GLISS,
    //step
    SLIDE,
    //period, delta
    VOLUME_SLIDE
  };

  struct Sample : public Formats::Chiptune::ProSoundCreator::Sample
  {
    Sample()
      : Formats::Chiptune::ProSoundCreator::Sample()
    {
    }

    Sample(const Formats::Chiptune::ProSoundCreator::Sample& rh)
      : Formats::Chiptune::ProSoundCreator::Sample(rh)
    {
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  };

  struct Ornament : public Formats::Chiptune::ProSoundCreator::Ornament
  {
    Ornament()
      : Formats::Chiptune::ProSoundCreator::Ornament()
    {
    }

    Ornament(const Formats::Chiptune::ProSoundCreator::Ornament& rh)
      : Formats::Chiptune::ProSoundCreator::Ornament(rh)
    {
    }

    uint_t GetSize() const
    {
      return static_cast<uint_t>(Lines.size());
    }

    const Line& GetLine(uint_t idx) const
    {
      static const Line STUB;
      return Lines.size() > idx ? Lines[idx] : STUB;
    }
  };

  template<class Object>
  class ObjectLinesIterator
  {
  public:
    ObjectLinesIterator()
      : Obj()
      , Position()
      , LoopPosition()
      , Break()
    {
    }

    void Set(const Object* obj)
    {
      //do not reset for original position update algo
      Obj = obj;
    }

    void Reset()
    {
      Position = 0;
      Break = false;
    }

    void Disable()
    {
      Position = Obj->GetSize();
    }

    const typename Object::Line* GetLine() const
    {
      return Position < Obj->GetSize()
        ? &Obj->GetLine(Position)
        : 0;
    }

    void SetBreakLoop(bool brk)
    {
      Break = brk;
    }

    void Next()
    {
      const typename Object::Line& curLine = Obj->GetLine(Position);
      if (curLine.LoopBegin)
      {
        LoopPosition = Position;
      }
      if (curLine.LoopEnd)
      {
        if (!Break)
        {
          Position = LoopPosition;
        }
        else
        {
          Break = false;
          ++Position;
        }
      }
      else
      {
        ++Position;
      }
    }
  private:
    const Object* Obj;
    uint_t Position;
    uint_t LoopPosition;
    bool Break;
  };

  class ToneSlider
  {
  public:
    ToneSlider()
      : Sliding()
      , Glissade()
      , Steps()
    {
    }

    void SetSliding(int_t sliding)
    {
      Sliding = sliding;
    }

    void SetSlidingSteps(uint_t steps)
    {
      Steps = steps;
    }

    void SetGlissade(int_t glissade)
    {
      Glissade = glissade;
    }

    void Reset()
    {
      Glissade = 0;
    }

    int_t GetSliding() const
    {
      return Sliding;
    }

    int_t Update()
    {
      if (Glissade)
      {
        Sliding += Glissade;
        if (Steps && !--Steps)
        {
          Glissade = 0;
        }
        return Sliding;
      }
      else
      {
        return 0;
      }
    }
  private:
    int_t Sliding;
    int_t Glissade;
    uint_t Steps;
  };

  class VolumeSlider
  {
  public:
    VolumeSlider()
      : Counter()
      , Period()
      , Delta()
    {
    }

    void Reset()
    {
      Counter = 0;
    }

    void SetParams(uint_t period, int_t delta)
    {
      Period = Counter = period;
      Delta = delta;
    }

    int_t Update()
    {
      if (Counter && !--Counter)
      {
        Counter = Period;
        return Delta;
      }
      else
      {
        return 0;
      }
    }
  private:
    uint_t Counter;
    uint_t Period;
    int_t Delta;
  };

  class ModuleData : public TrackModel
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;

    ModuleData()
      : InitialTempo()
    {
    }

    virtual uint_t GetInitialTempo() const
    {
      return InitialTempo;
    }

    virtual const OrderList& GetOrder() const
    {
      return *Order;
    }

    virtual const PatternsSet& GetPatterns() const
    {
      return *Patterns;
    }

    uint_t InitialTempo;
    OrderList::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Sample> Samples;
    SparsedObjectsStorage<Ornament> Ornaments;
  };

  class DataBuilder : public Formats::Chiptune::ProSoundCreator::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& props)
      : Data(boost::make_shared<ModuleData>())
      , Properties(props)
      , Patterns(PatternsBuilder::Create<AYM::TRACK_CHANNELS>())
    {
      Data->Patterns = Patterns.GetResult();
      Properties.SetFreqtable(TABLE_ASM);
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return Properties;
    }

    virtual void SetInitialTempo(uint_t tempo)
    {
      Data->InitialTempo = tempo;
    }

    virtual void SetSample(uint_t index, const Formats::Chiptune::ProSoundCreator::Sample& sample)
    {
      Data->Samples.Add(index, Sample(sample));
    }

    virtual void SetOrnament(uint_t index, const Formats::Chiptune::ProSoundCreator::Ornament& ornament)
    {
      Data->Ornaments.Add(index, Ornament(ornament));
    }

    virtual void SetPositions(const std::vector<uint_t>& positions, uint_t loop)
    {
      Data->Order = boost::make_shared<SimpleOrderList>(loop, positions.begin(), positions.end());
    }

    virtual Formats::Chiptune::PatternBuilder& StartPattern(uint_t index)
    {
      Patterns.SetPattern(index);
      return Patterns;
    }

    virtual void StartChannel(uint_t index)
    {
      Patterns.SetChannel(index);
    }

    virtual void SetRest()
    {
      Patterns.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      MutableCell& channel = Patterns.GetChannel();
      channel.SetEnabled(true);
      channel.SetNote(note);
    }

    virtual void SetSample(uint_t sample)
    {
      Patterns.GetChannel().SetSample(sample);
    }

    virtual void SetOrnament(uint_t ornament)
    {
      Patterns.GetChannel().SetOrnament(ornament);
    }

    virtual void SetVolume(uint_t vol)
    {
      Patterns.GetChannel().SetVolume(vol);
    }

    virtual void SetEnvelope(uint_t type, uint_t value)
    {
      Patterns.GetChannel().AddCommand(ENVELOPE, type, value);
    }

    virtual void SetEnvelope()
    {
      Patterns.GetChannel().AddCommand(ENVELOPE);
    }

    virtual void SetNoEnvelope()
    {
      Patterns.GetChannel().AddCommand(NOENVELOPE);
    }

    virtual void SetNoiseBase(uint_t val)
    {
      Patterns.GetChannel().AddCommand(NOISE_BASE, val);
    }

    virtual void SetBreakSample()
    {
      Patterns.GetChannel().AddCommand(BREAK_SAMPLE);
    }

    virtual void SetBreakOrnament()
    {
      Patterns.GetChannel().AddCommand(BREAK_ORNAMENT);
    }

    virtual void SetNoOrnament()
    {
      Patterns.GetChannel().AddCommand(NO_ORNAMENT);
    }

    virtual void SetGliss(uint_t absStep)
    {
      Patterns.GetChannel().AddCommand(GLISS, absStep);
    }

    virtual void SetSlide(int_t delta)
    {
      Patterns.GetChannel().AddCommand(SLIDE, delta);
    }

    virtual void SetVolumeSlide(uint_t period, int_t delta)
    {
      Patterns.GetChannel().AddCommand(VOLUME_SLIDE, period, delta);
    }

    ModuleData::Ptr GetResult() const
    {
      return Data;
    }
  private:
    const boost::shared_ptr<ModuleData> Data;
    PropertiesBuilder& Properties;
    PatternsBuilder Patterns;
  };

  struct ChannelState
  {
    ChannelState()
      : Note()
      , ToneAccumulator()
      , Volume()
      , Attenuation()
      , NoiseAccumulator()
      , EnvelopeEnabled()
    {
    }

    ObjectLinesIterator<Sample> SampleIterator;
    ObjectLinesIterator<Ornament> OrnamentIterator;
    ToneSlider ToneSlide;
    VolumeSlider VolumeSlide;
    int_t Note;
    int16_t ToneAccumulator;
    uint_t Volume;
    int_t Attenuation;
    uint_t NoiseAccumulator;
    bool EnvelopeEnabled;
  };

  class DataRenderer : public AYM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
       : Data(data)
       , EnvelopeTone()
       , NoiseBase()
    {
      Reset();
    }

    virtual void Reset()
    {
      const Sample* const stubSample = Data->Samples.Find(0);
      const Ornament* const stubOrnament = Data->Ornaments.Find(0);
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        ChannelState& dst = PlayerState[chan];
        dst = ChannelState();
        dst.SampleIterator.Set(stubSample);
        dst.SampleIterator.SetBreakLoop(false);
        dst.SampleIterator.Disable();
        dst.OrnamentIterator.Set(stubOrnament);
        dst.OrnamentIterator.SetBreakLoop(false);
        dst.OrnamentIterator.Disable();
      }
      EnvelopeTone = 0;
      NoiseBase = 0;
    }

    virtual void SynthesizeData(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (0 == state.Quirk())
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(track);
    }
  private:
    void GetNewLineState(const TrackModelState& state, AYM::TrackBuilder& track)
    {
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            GetNewChannelState(*src, PlayerState[chan], track);
          }
        }
      }
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        PlayerState[chan].NoiseAccumulator += NoiseBase;
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      const uint16_t oldTone = static_cast<uint_t>(track.GetFrequency(dst.Note) + dst.ToneAccumulator + dst.ToneSlide.GetSliding());
      if (const bool* enabled = src.GetEnabled())
      {
        if (*enabled)
        {
          dst.SampleIterator.Reset();
          dst.OrnamentIterator.Reset();
          dst.VolumeSlide.Reset();
          dst.ToneSlide.SetSliding(0);
          dst.ToneAccumulator = 0;
          dst.NoiseAccumulator = 0;
          dst.Attenuation = 0;
        }
        else
        {
          dst.SampleIterator.SetBreakLoop(false);
          dst.SampleIterator.Disable();
          dst.OrnamentIterator.SetBreakLoop(false);
          dst.OrnamentIterator.Disable();
        }
        dst.ToneSlide.Reset();
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleIterator.Set(Data->Samples.Find(*sample));
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentIterator.Set(Data->Ornaments.Find(*ornament));
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
        dst.Attenuation = 0;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case BREAK_SAMPLE:
          dst.SampleIterator.SetBreakLoop(true);
          break;
        case BREAK_ORNAMENT:
          dst.OrnamentIterator.SetBreakLoop(true);
          break;
        case NO_ORNAMENT:
          dst.OrnamentIterator.Disable();
          break;
        case ENVELOPE:
          if (it->Param1 || it->Param2)
          {
            track.SetEnvelopeType(it->Param1);
            track.SetEnvelopeTone(EnvelopeTone = it->Param2);
          }
          else
          {
            dst.EnvelopeEnabled = true;
          }
          break;
        case NOENVELOPE:
          dst.EnvelopeEnabled = false;
          break;
        case NOISE_BASE:
          NoiseBase = it->Param1;
          break;
        case GLISS:
          {
            const int_t sliding = oldTone - track.GetFrequency(dst.Note);
            const int_t newGliss = sliding >= 0 ? -it->Param1 : it->Param1;
            const int_t steps = 0 != it->Param1 ? (1 + Math::Absolute(sliding) / it->Param1) : 0;
            dst.ToneSlide.SetSliding(sliding);
            dst.ToneSlide.SetGlissade(newGliss);
            dst.ToneSlide.SetSlidingSteps(steps);
          }
          break;
        case SLIDE:
          dst.ToneSlide.SetGlissade(it->Param1);
          dst.ToneSlide.SetSlidingSteps(0);
          break;
        case VOLUME_SLIDE:
          dst.VolumeSlide.SetParams(it->Param1, it->Param2);
          break;
        default:
          assert(!"Invalid command");
        }
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != PlayerState.size(); ++chan)
      {
        AYM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(PlayerState[chan], channel, track);
      }
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track)
    {
      const Sample::Line* const curSampleLine = dst.SampleIterator.GetLine();
      if (!curSampleLine)
      {
        channel.SetLevel(0);
        return;
      }
      dst.SampleIterator.Next();

      //apply tone
      if (const Ornament::Line* curOrnamentLine = dst.OrnamentIterator.GetLine())
      {
        dst.NoiseAccumulator += curOrnamentLine->NoiseAddon;
        dst.Note += curOrnamentLine->NoteAddon;
        if (dst.Note < 0)
        {
          dst.Note += 0x56;
        }
        else if (dst.Note > 0x55)
        {
          dst.Note -= 0x56;
        }
        if (dst.Note > 0x55)
        {
          dst.Note = 0x55;
        }
        dst.OrnamentIterator.Next();
      }
      dst.ToneAccumulator += curSampleLine->Tone;
      const int_t toneOffset = dst.ToneAccumulator + dst.ToneSlide.Update();
      channel.SetTone(dst.Note, toneOffset);
      if (curSampleLine->ToneMask)
      {
        channel.DisableTone();
      }

      //apply level
      dst.Attenuation += curSampleLine->VolumeDelta + dst.VolumeSlide.Update();
      const int_t level = Math::Clamp<int_t>(dst.Attenuation + dst.Volume, 0, 15);
      dst.Attenuation = level - dst.Volume;
      const uint_t vol = 1 + static_cast<uint_t>(level);
      channel.SetLevel(vol * curSampleLine->Level >> 4);

      //apply noise+envelope
      const bool envelope = dst.EnvelopeEnabled && curSampleLine->EnableEnvelope;
      if (envelope)
      {
        channel.EnableEnvelope();
      }
      if (envelope && curSampleLine->NoiseMask)
      {
        EnvelopeTone += curSampleLine->Adding;
        track.SetEnvelopeTone(EnvelopeTone);
      }
      else
      {
        dst.NoiseAccumulator += curSampleLine->Adding;
        if (!curSampleLine->NoiseMask)
        {
          track.SetNoise(dst.NoiseAccumulator);
        }
      }
      if (curSampleLine->NoiseMask)
      {
        channel.DisableNoise();
      }
    }
  private:
    const ModuleData::Ptr Data;
    boost::array<ChannelState, AYM::TRACK_CHANNELS> PlayerState;
    uint_t EnvelopeTone;
    uint_t NoiseBase;
  };

  class Chiptune : public AYM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(CreateTrackInfo(Data, AYM::TRACK_CHANNELS))
    {
    }

    virtual Information::Ptr GetInformation() const
    {
      return Info;
    }

    virtual Parameters::Accessor::Ptr GetProperties() const
    {
      return Properties;
    }

    virtual AYM::DataIterator::Ptr CreateDataIterator(AYM::TrackParameters::Ptr trackParams) const
    {
      const TrackStateIterator::Ptr iterator = CreateTrackStateIterator(Data);
      const AYM::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return AYM::CreateDataIterator(trackParams, iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };

  class Factory : public AYM::Factory
  {
  public:
    virtual AYM::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Formats::Chiptune::ProSoundCreator::Parse(rawData, dataBuilder))
      {
        propBuilder.SetSource(*container);
        return boost::make_shared<Chiptune>(dataBuilder.GetResult(), propBuilder.GetResult());
      }
      else
      {
        return AYM::Chiptune::Ptr();
      }
    }
  };
}
}

namespace ZXTune
{
  void RegisterPSCSupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    const Char ID[] = {'P', 'S', 'C', 0};

    const Formats::Chiptune::Decoder::Ptr decoder = Formats::Chiptune::CreateProSoundCreatorDecoder();
    const Module::AYM::Factory::Ptr factory = boost::make_shared<Module::ProSoundCreator::Factory>();
    const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
    registrator.RegisterPlugin(plugin);
  }
}
