/**
* 
* @file
*
* @brief  TFMMusicMaker support plugin
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "tfm_base.h"
#include "tfm_base_track.h"
#include "tfm_plugin.h"
#include "core/plugins/registrator.h"
#include "core/plugins/players/simple_orderlist.h"
//common includes
#include <contract.h>
#include <pointers.h>
//library includes
#include <formats/chiptune/decoders.h>
#include <formats/chiptune/fm/tfmmusicmaker.h>
#include <math/fixedpoint.h>
//boost includes
#include <boost/make_shared.hpp>
#include <boost/scoped_ptr.hpp>

namespace Module
{
namespace TFMMusicMaker
{
  enum CmdType
  {
    EMPTY,
    //add1, add2
    ARPEGGIO,
    //step
    TONESLIDE,
    //step
    PORTAMENTO,
    //speed, depth
    VIBRATO,
    //op, value
    LEVEL,
    //speedup, speeddown
    VOLSLIDE,
    //on/off
    SPECMODE,
    //op, offset
    TONEOFFSET,
    //op, value
    MULTIPLE,
    //mask
    MIXING,
    //val
    PANE,
    //period
    NOTERETRIG,
    //pos
    NOTECUT,
    //pos
    NOTEDELAY,
    //
    DROPEFFECTS,
    //val
    FEEDBACK,
    //val
    TEMPO_INTERLEAVE,
    //even, odd
    TEMPO_VALUES,
    //
    LOOP_START,
    //additional count
    LOOP_STOP
  };

  typedef Formats::Chiptune::TFMMusicMaker::Instrument Instrument;

  class ModuleData
  {
  public:
    typedef boost::shared_ptr<const ModuleData> Ptr;

    ModuleData()
      : EvenInitialTempo()
      , OddInitialTempo()
      , InitialTempoInterleave()
    {
    }

    uint_t EvenInitialTempo;
    uint_t OddInitialTempo;
    uint_t InitialTempoInterleave;
    OrderList::Ptr Order;
    PatternsSet::Ptr Patterns;
    SparsedObjectsStorage<Instrument> Instruments;
  };

  class DataBuilder : public Formats::Chiptune::TFMMusicMaker::Builder
  {
  public:
    explicit DataBuilder(PropertiesBuilder& props)
      : Data(boost::make_shared<ModuleData>())
      , Properties(props)
      , Patterns(PatternsBuilder::Create<TFM::TRACK_CHANNELS>())
    {
      Data->Patterns = Patterns.GetResult();
    }

    virtual Formats::Chiptune::MetaBuilder& GetMetaBuilder()
    {
      return Properties;
    }

    virtual void SetTempo(uint_t evenTempo, uint_t oddTempo, uint_t interleavePeriod)
    {
      Data->EvenInitialTempo = evenTempo;
      Data->OddInitialTempo = oddTempo;
      Data->InitialTempoInterleave = interleavePeriod;
    }

    virtual void SetDate(const Formats::Chiptune::TFMMusicMaker::Date& /*created*/, const Formats::Chiptune::TFMMusicMaker::Date& /*saved*/)
    {
    }

    virtual void SetComment(const String& comment)
    {
      Properties.SetComment(comment);
    }

    virtual void SetInstrument(uint_t index, const Formats::Chiptune::TFMMusicMaker::Instrument& instrument)
    {
      Data->Instruments.Add(index, instrument);
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

    virtual void SetKeyOff()
    {
      Patterns.GetChannel().SetEnabled(false);
    }

    virtual void SetNote(uint_t note)
    {
      Patterns.GetChannel().SetNote(note);
    }

    virtual void SetVolume(uint_t vol)
    {
      Patterns.GetChannel().SetVolume(vol);
    }

    virtual void SetInstrument(uint_t ins)
    {
      Patterns.GetChannel().SetSample(ins);
    }

    virtual void SetArpeggio(uint_t add1, uint_t add2)
    {
      Patterns.GetChannel().AddCommand(ARPEGGIO, add1, add2);
    }

    virtual void SetSlide(int_t step)
    {
      Patterns.GetChannel().AddCommand(TONESLIDE, step);
    }

    virtual void SetPortamento(int_t step)
    {
      Patterns.GetChannel().AddCommand(PORTAMENTO, step);
    }

    virtual void SetVibrato(uint_t speed, uint_t depth)
    {
      Patterns.GetChannel().AddCommand(VIBRATO, speed, depth);
    }

    virtual void SetTotalLevel(uint_t op, uint_t value)
    {
      Patterns.GetChannel().AddCommand(LEVEL, op, value);
    }

    virtual void SetVolumeSlide(uint_t up, uint_t down)
    {
      Patterns.GetChannel().AddCommand(VOLSLIDE, up, down);
    }

    virtual void SetSpecialMode(bool on)
    {
      Patterns.GetChannel().AddCommand(SPECMODE, on);
    }

    virtual void SetToneOffset(uint_t op, uint_t offset)
    {
      Patterns.GetChannel().AddCommand(TONEOFFSET, op, offset);
    }

    virtual void SetMultiple(uint_t op, uint_t val)
    {
      Patterns.GetChannel().AddCommand(MULTIPLE, op, val);
    }

    virtual void SetOperatorsMixing(uint_t mask)
    {
      Patterns.GetChannel().AddCommand(MIXING, mask);
    }

    virtual void SetLoopStart()
    {
      Patterns.GetChannel().AddCommand(LOOP_START);
    }

    virtual void SetLoopEnd(uint_t additionalCount)
    {
      Patterns.GetChannel().AddCommand(LOOP_STOP, additionalCount);
    }

    virtual void SetPane(uint_t pane)
    {
      Patterns.GetChannel().AddCommand(PANE, pane);
    }

    virtual void SetNoteRetrig(uint_t period)
    {
      Patterns.GetChannel().AddCommand(NOTERETRIG, period);
    }

    virtual void SetNoteCut(uint_t quirk)
    {
      Patterns.GetChannel().AddCommand(NOTECUT, quirk);
    }

    virtual void SetNoteDelay(uint_t quirk)
    {
      Patterns.GetChannel().AddCommand(NOTEDELAY, quirk);
    }

    virtual void SetDropEffects()
    {
      Patterns.GetChannel().AddCommand(DROPEFFECTS);
    }

    virtual void SetFeedback(uint_t val)
    {
      Patterns.GetChannel().AddCommand(FEEDBACK, val);
    }

    virtual void SetTempoInterleave(uint_t val)
    {
      Patterns.GetChannel().AddCommand(TEMPO_INTERLEAVE, val);
    }

    virtual void SetTempoValues(uint_t even, uint_t odd)
    {
      Patterns.GetChannel().AddCommand(TEMPO_VALUES, even, odd);
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

  struct Halftones
  {
    typedef Math::FixedPoint<int_t, 32> Type;

    static Type Min()
    {
      return FromFraction(0);
    }

    static Type Max()
    {
      return FromFraction(0xbff);
    }

    static Type Stub()
    {
      return FromFraction(-1);
    }

    static Type FromFraction(int_t val)
    {
      return Type(val, Type::PRECISION);
    }

    static Type FromInteger(int_t val)
    {
      return Type(val);
    }
  };

  struct Level
  {
    typedef Math::FixedPoint<int_t, 8> Type;

    static Type Min()
    {
      return FromFraction(0);
    }

    static Type Max()
    {
      return FromFraction(0xf8);
    }

    static Type FromFraction(int_t val)
    {
      return Type(val, Type::PRECISION);
    }

    static Type FromInteger(int_t val)
    {
      return Type(val);
    }
  };

  struct ArpeggioState
  {
  public:
    ArpeggioState()
      : Position()
      , Addons()
      , Value()
    {
    }

    void Reset()
    {
      Position = 0;
      std::fill(Addons.begin(), Addons.end(), 0);
      Value = 0;
    }

    void SetAddons(uint_t add1, uint_t add2)
    {
      if (add1 == 0xf && add2 == 0xf)
      {
        Addons[1] = Addons[2] = 0;
      }
      else
      {
        Addons[1] = add1;
        Addons[2] = add2;
      }
    }

    bool Update()
    {
      const uint_t prev = Value;
      Value = Addons[Position];
      if (++Position = Addons.size())
      {
        Position = 0;
      }
      return Value != prev;
    }

    Halftones::Type GetValue() const
    {
      return Halftones::FromInteger(Value);
    }
  private:
    uint_t Position;
    boost::array<uint_t, 3> Addons;
    uint_t Value;
  };

  template<class Category>
  struct SlideState
  {
  public:
    SlideState()
      : Enabled()
      , UpDelta()
      , DownDelta()
    {
    }

    void Disable()
    {
      Enabled = false;
    }

    void SetDelta(int_t delta)
    {
      Enabled = true;
      if (delta > 0)
      {
        UpDelta = delta;
      }
      else if (delta < 0)
      {
        DownDelta = delta;
      }
    }

    bool Update(typename Category::Type& val) const
    {
      if (!Enabled)
      {
        return false;
      }
      const typename Category::Type prev = val;
      if (UpDelta)
      {
        val += Category::FromFraction(UpDelta);
        val = std::min<typename Category::Type>(val, Category::Max());
      }
      if (DownDelta)
      {
        val += Category::FromFraction(DownDelta);
        val = std::max<typename Category::Type>(val, Category::Min());
      }
      return val != prev;
    }
  private:
    bool Enabled;
    int_t UpDelta;
    int_t DownDelta;
  };

  typedef SlideState<Halftones> ToneSlideState;

  struct VibratoState
  {
  public:
    VibratoState()
      : Enabled()
      , Position()
      , Speed()
      , Depth()
      , Value()
    {
    }

    void Disable()
    {
      Enabled = false;
    }

    void ResetValue()
    {
      Value = 0;
    }

    void SetParameters(uint_t speed, int_t depth)
    {
      Enabled = true;
      if (speed)
      {
        Speed = speed;
      }
      if (depth)
      {
        Depth = depth;
      }
      if (speed || depth)
      {
        Position = Value = 0;
      }
    }

    bool Update()
    {
      if (!Enabled)
      {
        if (Value != 0)
        {
          Value = 0;
          Position = 0;
          return true;
        }
        return false;
      }
      //use signed values
      const int_t PRECISION = 256;
      const int_t TABLE[] =
      {
        0, 49, 97, 142, 181, 212, 236, 251, 256, 251, 236, 212, 181, 142, 97, 49,
        0, -49, -97, -142, -181, -212, -236, -251, -256, -251, -236, -212, -181, -142, -97, -49
      };      
      const int_t prevVal = Value;
      Value = TABLE[Position / 2] * Depth / PRECISION;
      Position = (Position + Speed) & 0x3f;
      return prevVal != Value;
    }

    Halftones::Type GetValue() const
    {
      return Halftones::FromFraction(Value);
    }
  private:
    bool Enabled;
    uint_t Position;
    uint_t Speed;
    int_t Depth;
    int_t Value;
  };

  typedef SlideState<Level> VolumeSlideState;

  struct PortamentoState
  {
  public:
    PortamentoState()
      : Enabled()
      , Step()
      , Target(Halftones::Stub())
    {
    }

    void Disable()
    {
      Enabled = false;
    }

    void SetTarget(Halftones::Type tgt)
    {
      Enabled = true;
      Target = tgt;
    }

    void SetStep(uint_t step)
    {
      Enabled = true;
      if (step)
      {
        Step = Halftones::FromFraction(step);
      }
    }

    bool Update(Halftones::Type& note) const
    {
      if (!Enabled
       || Target == Halftones::Stub()
       || Target == note
       || Step == Halftones::FromFraction(0))
      {
        return false;
      }
      else if (Target > note)
      {
        PortamentoUp(note);
      }
      else
      {
        PortamentoDown(note);
      }
      return true;
    }
  private:
    void PortamentoUp(Halftones::Type& note) const
    {
      const Halftones::Type next = note + Step;
      note = std::min(next, Target);
    }

    void PortamentoDown(Halftones::Type& note) const
    {
      const Halftones::Type next = note - Step;
      note = std::max(next, Target);
    }
  private:
    bool Enabled;
    Halftones::Type Step;
    Halftones::Type Target;
  };

  const uint_t NO_VALUE = ~uint_t(0);
  const uint_t SPECIAL_MODE_CHANNEL = 2;
  const uint_t OPERATORS_COUNT = 4;

  struct ChannelState
  {
    ChannelState()
      : CurInstrument(0)
      , Algorithm(NO_VALUE)
      , TotalLevel()
      , Note(Halftones::Stub())
      , Volume(Level::Max())
      , HasToneChange(false)
      , HasVolumeChange(false)
      , Arpeggio()
      , ToneSlide()
      , Vibrato()
      , VolumeSlide()
      , Portamento()
      , NoteRetrig(NO_VALUE)
      , NoteCut(NO_VALUE)
      , NoteDelay(NO_VALUE)
    {
    }

    const Instrument* CurInstrument;
    uint_t Algorithm;
    uint_t TotalLevel[OPERATORS_COUNT];
    Halftones::Type Note;
    Level::Type Volume;
    bool HasToneChange;
    bool HasVolumeChange;

    ArpeggioState Arpeggio;
    ToneSlideState ToneSlide;
    VibratoState Vibrato;
    VolumeSlideState VolumeSlide;
    PortamentoState Portamento;
    uint_t NoteRetrig;
    uint_t NoteCut;
    uint_t NoteDelay;
  };

  struct PlayerState
  {
    PlayerState()
      : SpecialMode(false)
      , ToneOffset()
    {
    }

    bool SpecialMode;
    boost::array<int_t, OPERATORS_COUNT> ToneOffset;
    boost::array<ChannelState, TFM::TRACK_CHANNELS> Channels;
  };

  class DataRenderer : public TFM::DataRenderer
  {
  public:
    explicit DataRenderer(ModuleData::Ptr data)
      : Data(data)
    {
    }

    virtual void Reset()
    {
      State = PlayerState();
    }

    virtual void SynthesizeData(const TrackModelState& state, TFM::TrackBuilder& track)
    {
      const uint_t quirk = state.Quirk();
      if (0 == quirk)
      {
        GetNewLineState(state, track);
      }
      SynthesizeChannelsData(quirk, track);
    }
  private:
    void GetNewLineState(const TrackModelState& state, TFM::TrackBuilder& track)
    {
      ResetOneLineEffects();
      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != State.Channels.size(); ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(chan))
          {
            TFM::ChannelBuilder channel = track.GetChannel(chan);
            GetNewChannelState(*src, State.Channels[chan], track, channel);
          }
        }
      }
    }

    void ResetOneLineEffects()
    {
      for (uint_t chan = 0; chan != State.Channels.size(); ++chan)
      {
        ChannelState& dst = State.Channels[chan];
        //portamento, vibrato, volume and tone slide are applicable only when effect is specified
        dst.ToneSlide.Disable();
        dst.Vibrato.Disable();
        dst.VolumeSlide.Disable();
        dst.Portamento.Disable();
        dst.NoteRetrig = dst.NoteCut = dst.NoteDelay = NO_VALUE;
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, TFM::TrackBuilder& track, TFM::ChannelBuilder& channel)
    {
      const int_t* multiplies[OPERATORS_COUNT] = {0, 0, 0, 0};
      bool dropEffects = false;
      bool hasPortamento = false;
      bool hasOpMixer = false;
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case PORTAMENTO:
          hasPortamento = true;
          break;
        case SPECMODE:
          SetSpecialMode(it->Param1 != 0, track);
          break;
        case TONEOFFSET:
          State.ToneOffset[it->Param1] = it->Param2;
          break;
        case MULTIPLE:
          multiplies[it->Param1] = &it->Param2;
          break;
        case MIXING:
          hasOpMixer = true;
          break;
        case PANE:
          if (1 == it->Param1)
          {
            channel.SetPane(0x80);
          }
          else if (2 == it->Param1)
          {
            channel.SetPane(0x40);
          }
          else
          {
            channel.SetPane(0xc0);
          }
          break;
        case NOTERETRIG:
          dst.NoteRetrig = it->Param1;
          break;
        case NOTECUT:
          dst.NoteCut = it->Param1;
          break;
        case NOTEDELAY:
          dst.NoteDelay = it->Param1;
          break;
        case DROPEFFECTS:
          dropEffects = true;
          break;
        }
      }

      const bool hasNoteDelay = dst.NoteDelay != NO_VALUE;

      if (const bool* enabled = src.GetEnabled())
      {
        if (!*enabled)
        {
          channel.KeyOff();
        }
      }
      if (const uint_t* note = src.GetNote())
      {
        if (!hasPortamento)
        {
          channel.KeyOff();
        }
        const Instrument* newInstrument = GetNewInstrument(src);
        if (!dst.CurInstrument && !newInstrument)
        {
          newInstrument = Data->Instruments.Find(1);
        }
        if (dropEffects && dst.CurInstrument && !newInstrument)
        {
          newInstrument = dst.CurInstrument;
        }
        if ((newInstrument && newInstrument != dst.CurInstrument) || dropEffects)
        {
          if (const uint_t* volume = src.GetVolume())
          {
            dst.Volume = Level::FromInteger(*volume);
          }
          dst.CurInstrument = newInstrument;
          LoadInstrument(multiplies, dst, channel);
          dst.HasVolumeChange = true;
        }
        if (hasPortamento)
        {
          dst.Portamento.SetTarget(Halftones::FromInteger(*note));
        }
        else
        {
          dst.Note = Halftones::FromInteger(*note);
          dst.Arpeggio.Reset();
          dst.Vibrato.ResetValue();
          dst.HasToneChange = true;
          if (!hasNoteDelay && !hasOpMixer)
          {
            channel.KeyOn();
          }
        }
      }
      if (const uint_t* volume = src.GetVolume())
      {
        const Level::Type newVol = Level::FromInteger(*volume);
        if (newVol != dst.Volume)
        {
          dst.Volume = newVol;
          dst.HasVolumeChange = true;
        }
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case ARPEGGIO:
          dst.Arpeggio.SetAddons(it->Param1, it->Param2);
          break;
        case TONESLIDE:
          dst.ToneSlide.SetDelta(it->Param1);
          break;
        case PORTAMENTO:
          dst.Portamento.SetStep(it->Param1);
          break;
        case VIBRATO:
          //parameter in 1/16 of halftone
          dst.Vibrato.SetParameters(it->Param1, it->Param2 * Halftones::Type::PRECISION / 16);
          break;
        case LEVEL:
          dst.TotalLevel[it->Param1] = it->Param2;
          dst.HasVolumeChange = true;
          break;
        case VOLSLIDE:
          dst.VolumeSlide.SetDelta(it->Param1);
          dst.VolumeSlide.SetDelta(-it->Param2);
          break;
        case MULTIPLE:
          channel.SetDetuneMultiple(it->Param1, dst.CurInstrument->Operators[it->Param1].Detune, it->Param2);
          break;
        case MIXING:
          channel.SetKey(it->Param1);
          break;
        case FEEDBACK:
          channel.SetupConnection(dst.Algorithm, it->Param1);
          break;
        }
      }
    }

    void SetSpecialMode(bool enabled, TFM::TrackBuilder& track)
    {
      if (enabled != State.SpecialMode)
      {
        State.SpecialMode = enabled;
        track.GetChannel(SPECIAL_MODE_CHANNEL).SetMode(enabled ? 0x40 : 0x00);
      }
    }

    const Instrument* GetNewInstrument(const Cell& src) const
    {
      if (const uint_t* instrument = src.GetSample())
      {
        return Data->Instruments.Find(*instrument);
      }
      else
      {
        return 0;
      }
    }

    void LoadInstrument(const int_t* multiplies[], ChannelState& dst, TFM::ChannelBuilder& channel)
    {
      const Instrument& ins = *dst.CurInstrument;
      channel.SetupConnection(dst.Algorithm = ins.Algorithm, ins.Feedback);
      for (uint_t opIdx = 0; opIdx != OPERATORS_COUNT; ++opIdx)
      {
        const Instrument::Operator& op = ins.Operators[opIdx];
        dst.TotalLevel[opIdx] = op.TotalLevel;
        const uint_t multiple = multiplies[opIdx] ? *multiplies[opIdx] : op.Multiple;
        channel.SetDetuneMultiple(opIdx, op.Detune, multiple);
        channel.SetRateScalingAttackRate(opIdx, op.RateScaling, op.Attack);
        channel.SetDecay(opIdx, op.Decay);
        channel.SetSustain(opIdx, op.Sustain);
        channel.SetSustainLevelReleaseRate(opIdx, op.SustainLevel, op.Release);
        channel.SetEnvelopeType(opIdx, op.EnvelopeType);
      }
    }

    void SynthesizeChannelsData(uint_t quirk, TFM::TrackBuilder& track)
    {
      for (uint_t chan = 0; chan != State.Channels.size(); ++chan)
      {
        TFM::ChannelBuilder channel = track.GetChannel(chan);
        SynthesizeChannel(chan, channel);
        ProcessNoteEffects(quirk, chan, channel);
      }
    }

    void SynthesizeChannel(uint_t idx, TFM::ChannelBuilder& channel)
    {
      ChannelState& state = State.Channels[idx];
      if (state.Portamento.Update(state.Note))
      {
        state.HasToneChange = true;
      }
      if (state.Vibrato.Update())
      {
        state.HasToneChange = true;
      }
      if (state.ToneSlide.Update(state.Note))
      {
        state.HasToneChange = true;
      }
      if (state.Arpeggio.Update())
      {
        state.HasToneChange = true;
      }

      if (state.HasToneChange)
      {
        SetTone(idx, state, channel);
        state.HasToneChange = false;
      }

      if (state.VolumeSlide.Update(state.Volume))
      {
        state.HasVolumeChange = true;
      }
      if (state.HasVolumeChange)
      {
        SetLevel(state, channel);
        state.HasVolumeChange = false;
      }
    }

    void ProcessNoteEffects(uint_t quirk, uint_t idx, TFM::ChannelBuilder& channel)
    {
      ChannelState& state = State.Channels[idx];
      if (state.NoteRetrig != NO_VALUE && 0 == quirk % state.NoteRetrig)
      {
        channel.KeyOff();
        channel.KeyOn();
      }
      if (quirk == state.NoteCut)
      {
        channel.KeyOff();
      }
      if (quirk == state.NoteDelay)
      {
        channel.KeyOff();
        channel.KeyOn();
      }
    }

    struct RawNote
    {
      RawNote()
        : Octave()
        , Freq()
      {
      }

      RawNote(uint_t octave, uint_t freq)
        : Octave(octave)
        , Freq(freq)
      {
      }

      uint_t Octave;
      uint_t Freq;
    };

    void SetTone(uint_t idx, const ChannelState& state, TFM::ChannelBuilder& channel) const
    {
      const Halftones::Type note = state.Note + state.Arpeggio.GetValue() + state.Vibrato.GetValue();
      const RawNote rawNote = ConvertNote(Clamp(note));
      channel.SetTone(rawNote.Octave, rawNote.Freq);
      if (idx == SPECIAL_MODE_CHANNEL && State.SpecialMode)
      {
        for (uint_t op = 1; op != OPERATORS_COUNT; ++op)
        {
          const Halftones::Type opNote = note + Halftones::FromInteger(State.ToneOffset[op]);
          const RawNote rawOpNote = ConvertNote(Clamp(opNote));
          channel.SetTone(op, rawOpNote.Octave, rawOpNote.Freq);
        }
      }
    }

    static Halftones::Type Clamp(Halftones::Type val)
    {
      if (val > Halftones::Max())
      {
        return Halftones::Max();
      }
      else if (val < Halftones::Min())
      {
        return Halftones::Min();
      }
      else
      {
        return val;
      }
    }

    static RawNote ConvertNote(Halftones::Type note)
    {
      static const uint_t FREQS[] =
      {
        707, 749, 793, 840, 890, 943, 999, 1059, 1122, 1189, 1259, 1334, 1413, 1497
      };
      const uint_t totalHalftones = note.Integer();
      const uint_t octave = totalHalftones / 12;
      const uint_t halftone = totalHalftones % 12;
      const uint_t freq = FREQS[halftone] + ((FREQS[halftone + 1] - FREQS[halftone]) * note.Fraction()/* + note.PRECISION / 2*/) / note.PRECISION;
      return RawNote(octave, freq);
    }

    void SetLevel(const ChannelState& state, TFM::ChannelBuilder& channel) const
    {
      static const uint_t MIXER_TABLE[8] =
      {
        0x8, 0x8, 0x8, 0x8, 0x0c, 0xe, 0xe, 0x0f
      };
      static const uint_t LEVELS_TABLE[32] =
      {
        0x00, 0x00, 0x58, 0x5a, 0x5b, 0x5d, 0x5f, 0x60,
        0x61, 0x62, 0x64, 0x66, 0x68, 0x6a, 0x6b, 0x6d,
        0x6e, 0x70, 0x71, 0x72, 0x73, 0x74, 0x76, 0x77,
        0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f
      };
      if (state.Algorithm == NO_VALUE)
      {
        return;
      }
      const uint_t mix = MIXER_TABLE[state.Algorithm];
      const uint_t level = LEVELS_TABLE[state.Volume.Integer()];
      for (uint_t op = 0; op != OPERATORS_COUNT; ++op)
      {
        const uint_t out = 0 != (mix & (1 << op)) ? ScaleTL(state.TotalLevel[op], level) : state.TotalLevel[op];
        channel.SetTotalLevel(op, out);
      }
    }

    static uint_t ScaleTL(uint_t tl, uint_t scale)
    {
      return 0x7f - ((0x7f - tl) * scale / 127);
    }
  private:
    const ModuleData::Ptr Data;
    PlayerState State;
  };

  // TrackStateModel and TrackStateIterator with alternative tempo logic and loop support
  //TODO: refactor with common code and remove C&P
  class StubPattern : public Pattern
  {
    StubPattern()
    {
    }
  public:
    virtual Line::Ptr GetLine(uint_t /*row*/) const
    {
      return Line::Ptr();
    }

    virtual uint_t GetSize() const
    {
      return 0;
    }

    static Ptr Create()
    {
      static StubPattern instance;
      return MakeSingletonPointer(instance);
    }
  };
  
  struct PlainTrackState
  {
    uint_t Frame;
    uint_t Position;
    uint_t Pattern;
    uint_t Line;
    uint_t Quirk;
    uint_t EvenTempo;
    uint_t OddTempo;
    uint_t TempoInterleavePeriod;
    uint_t TempoInterleaveCounter;

    PlainTrackState()
      : Frame(), Position(), Pattern(), Line(), Quirk(), EvenTempo(), OddTempo(), TempoInterleavePeriod(), TempoInterleaveCounter()
    {
    }

    uint_t GetTempo() const
    {
      return TempoInterleaveCounter >= TempoInterleavePeriod ? OddTempo : EvenTempo;
    }

    void NextLine()
    {
      Quirk = 0;
      ++Line;
      if (++TempoInterleaveCounter >= 2 * TempoInterleavePeriod)
      {
        TempoInterleaveCounter -= 2 * TempoInterleavePeriod;
      }
    }
  };

  struct LoopState
  {
  public:
    LoopState()
      : Counter()
    {
    }

    void Start(const PlainTrackState& state)
    {
      if (!Begin.get() || Begin->Line != state.Line || Begin->Position != state.Position)
      {
        Begin.reset(new PlainTrackState(state));
        Counter = 0;
      }
    }

    const PlainTrackState* Stop(uint_t repeatCount)
    {
      if (Counter >= repeatCount)
      {
        return 0;
      }
      else
      {
        //from original loop processing logic
        if (++Counter >= repeatCount)
        {
          Counter = 16;//max repeat count+1
        }
        return Begin.get();
      }
    }
  private:
    boost::scoped_ptr<const PlainTrackState> Begin;
    uint_t Counter;
  };

  class TrackStateCursor : public TrackModelState
  {
  public:
    typedef boost::shared_ptr<TrackStateCursor> Ptr;

    explicit TrackStateCursor(ModuleData::Ptr data)
      : Data(data)
      , Order(*data->Order)
      , Patterns(*data->Patterns)
      , NextLineState()
    {
      Reset();
    }

    //TrackState
    virtual uint_t Position() const
    {
      return Plain.Position;
    }

    virtual uint_t Pattern() const
    {
      return Plain.Pattern;
    }

    virtual uint_t PatternSize() const
    {
      return CurPatternObject->GetSize();
    }

    virtual uint_t Line() const
    {
      return Plain.Line;
    }

    virtual uint_t Tempo() const
    {
      return Plain.GetTempo();
    }

    virtual uint_t Quirk() const
    {
      return Plain.Quirk;
    }

    virtual uint_t Frame() const
    {
      return Plain.Frame;
    }

    virtual uint_t Channels() const
    {
      return CurLineObject ? CurLineObject->CountActiveChannels() : 0;
    }

    //TrackModelState
    virtual Pattern::Ptr PatternObject() const
    {
      return CurPatternObject;
    }

    virtual Line::Ptr LineObject() const
    {
      return CurLineObject;
    }

    //navigation
    bool IsValid() const
    {
      return Plain.Position < Order.GetSize();
    }

    const PlainTrackState& GetState() const
    {
      return Plain;
    }

    void Reset()
    {
      Plain.Frame = 0;
      Plain.EvenTempo = Data->EvenInitialTempo;
      Plain.OddTempo = Data->OddInitialTempo;
      Plain.TempoInterleavePeriod = Data->InitialTempoInterleave;
      Plain.TempoInterleaveCounter = 0;
      SetPosition(0);
      NextLineState = 0;
    }

    void SetState(const PlainTrackState& state)
    {
      GoTo(state);
      Plain.Frame = state.Frame;
    }

    void Seek(uint_t position)
    {
      if (Plain.Position > position ||
          (Plain.Position == position && (0 != Plain.Line || 0 != Plain.Quirk)))
      {
        Reset();
      }
      while (IsValid() && Plain.Position != position)
      {
        Plain.Frame += Plain.GetTempo();
        if (!NextLine())
        {
          NextPosition();
        }
      }
    }

    bool NextFrame()
    {
      return NextQuirk() || NextLine() || NextPosition();
    }
  private:
    void SetPosition(uint_t pos)
    {
      Plain.Position = pos;
      if (IsValid())
      {
        SetPattern(Order.GetPatternIndex(Plain.Position));
      }
      else
      {
        SetStubPattern();
      }
    }

    void SetStubPattern()
    {
      Plain.Pattern = 0;
      CurPatternObject = StubPattern::Create();
      SetLine(0);
    }

    void SetPattern(uint_t pat)
    {
      Plain.Pattern = pat;
      CurPatternObject = Patterns.Get(Plain.Pattern);
      SetLine(0);
    }

    void SetLine(uint_t line)
    {
      Plain.Quirk = 0;
      Plain.Line = line;
      LoadLine();
    }

    void LoadLine()
    {
      if (CurLineObject = CurPatternObject->GetLine(Plain.Line))
      {
        LoadNewLoopTempoParameters();
      }
    }

    bool NextQuirk()
    {
      ++Plain.Frame;
      return ++Plain.Quirk < Plain.GetTempo();
    }

    bool NextLine()
    {
      if (NextLineState)
      {
        GoTo(*NextLineState);
      }
      else
      {
        Plain.NextLine();
        if (Plain.Line >= CurPatternObject->GetSize())
        {
          return false;
        }
        LoadLine();
      }
      return true;
    }

    bool NextPosition()
    {
      SetPosition(Plain.Position + 1);
      return IsValid();
    }

    void GoTo(const PlainTrackState& state)
    {
      SetPosition(state.Position);
      assert(Plain.Pattern == state.Pattern);
      SetLine(state.Line);
      Plain.Quirk = state.Quirk;
      Plain.EvenTempo = state.EvenTempo;
      Plain.OddTempo = state.OddTempo;
      Plain.TempoInterleavePeriod = state.TempoInterleavePeriod;
      Plain.TempoInterleaveCounter = state.TempoInterleaveCounter;
      NextLineState = 0;
    }

    void LoadNewLoopTempoParameters()
    {
      for (uint_t idx = 0; idx != TFM::TRACK_CHANNELS; ++idx)
      {
        if (const Cell::Ptr chan = CurLineObject->GetChannel(idx))
        {
          LoadNewLoopTempoParameters(*chan);
        }
      }
    }

    void LoadNewLoopTempoParameters(const Cell& chan)
    {
      //TODO: chan.FindCommand ?
      for (CommandsIterator it = chan.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case TEMPO_INTERLEAVE:
          Plain.TempoInterleavePeriod = it->Param1;
          break;
        case TEMPO_VALUES:
          Plain.EvenTempo = it->Param1;
          Plain.OddTempo = it->Param2;
          break;
        case LOOP_START:
          Loop.Start(Plain);
          break;
        case LOOP_STOP:
          NextLineState = Loop.Stop(it->Param1);
          break;
        }
      }
    }
  private:
    //context
    const ModuleData::Ptr Data;
    const OrderList& Order;
    const PatternsSet& Patterns;
    //state
    PlainTrackState Plain;
    Pattern::Ptr CurPatternObject;
    Line::Ptr CurLineObject;
    LoopState Loop;
    const PlainTrackState* NextLineState;
  };

  class TrackStateIteratorImpl : public TrackStateIterator
  {
  public:
    explicit TrackStateIteratorImpl(ModuleData::Ptr data)
      : Data(data)
      , Cursor(boost::make_shared<TrackStateCursor>(data))
    {
    }

    virtual void Reset()
    {
      Cursor->Reset();
    }

    virtual bool IsValid() const
    {
      return Cursor->IsValid();
    }

    virtual void NextFrame(bool looped)
    {
      if (!Cursor->IsValid())
      {
        return;
      }
      else if (!Cursor->NextFrame() && looped)
      {
        MoveToLoop();
      }
    }

    virtual TrackModelState::Ptr GetStateObserver() const
    {
      return Cursor;
    }
  private:
    void MoveToLoop()
    {
      if (LoopState.get())
      {
        Cursor->SetState(*LoopState);
      }
      else
      {
        Cursor->Seek(Data->Order->GetLoopPosition());
        const PlainTrackState& loop = Cursor->GetState();
        LoopState.reset(new PlainTrackState(loop));
      }
    }
  private:
    const ModuleData::Ptr Data;
    const TrackStateCursor::Ptr Cursor;
    boost::scoped_ptr<const PlainTrackState> LoopState;
  };

  class InformationImpl : public Information
  {
  public:
    InformationImpl(ModuleData::Ptr data)
      : Data(data)
      , Frames(), LoopFrameNum()
    {
    }

    virtual uint_t PositionsCount() const
    {
      return Data->Order->GetSize();
    }

    virtual uint_t LoopPosition() const
    {
      return Data->Order->GetLoopPosition();
    }

    virtual uint_t PatternsCount() const
    {
      return Data->Patterns->GetSize();
    }

    virtual uint_t FramesCount() const
    {
      Initialize();
      return Frames;
    }

    virtual uint_t LoopFrame() const
    {
      Initialize();
      return LoopFrameNum;
    }

    virtual uint_t ChannelsCount() const
    {
      return TFM::TRACK_CHANNELS;
    }

    virtual uint_t Tempo() const
    {
      return Data->EvenInitialTempo;
    }
  private:
    void Initialize() const
    {
      if (Frames)
      {
        return;//initialized
      }
      TrackStateCursor cursor(Data);
      cursor.Seek(Data->Order->GetLoopPosition());
      LoopFrameNum = cursor.GetState().Frame;
      cursor.Seek(Data->Order->GetSize());
      Frames = cursor.GetState().Frame;
    }
  private:
    const ModuleData::Ptr Data;
    mutable uint_t Frames;
    mutable uint_t LoopFrameNum;
  };

  class Chiptune : public TFM::Chiptune
  {
  public:
    Chiptune(ModuleData::Ptr data, Parameters::Accessor::Ptr properties)
      : Data(data)
      , Properties(properties)
      , Info(boost::make_shared<InformationImpl>(Data))
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

    virtual TFM::DataIterator::Ptr CreateDataIterator() const
    {
      const TrackStateIterator::Ptr iterator = boost::make_shared<TrackStateIteratorImpl>(Data);
      const TFM::DataRenderer::Ptr renderer = boost::make_shared<DataRenderer>(Data);
      return TFM::CreateDataIterator(iterator, renderer);
    }
  private:
    const ModuleData::Ptr Data;
    const Parameters::Accessor::Ptr Properties;
    const Information::Ptr Info;
  };

  class Factory : public TFM::Factory
  {
  public:
    explicit Factory(Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder)
      : Decoder(decoder)
    {
    }

    virtual TFM::Chiptune::Ptr CreateChiptune(PropertiesBuilder& propBuilder, const Binary::Container& rawData) const
    {
      DataBuilder dataBuilder(propBuilder);
      if (const Formats::Chiptune::Container::Ptr container = Decoder->Parse(rawData, dataBuilder))
      {
        propBuilder.SetSource(*container);
        return boost::make_shared<Chiptune>(dataBuilder.GetResult(), propBuilder.GetResult());
      }
      else
      {
        return TFM::Chiptune::Ptr();
      }
    }
  private:
    const Formats::Chiptune::TFMMusicMaker::Decoder::Ptr Decoder;
  };
}
}

namespace ZXTune
{
  void RegisterTFESupport(PlayerPluginsRegistrator& registrator)
  {
    //plugin attributes
    {
      const Char ID[] = {'T', 'F', '0', 0};
      const Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder = Formats::Chiptune::TFMMusicMaker::Ver05::CreateDecoder();
      const Module::TFM::Factory::Ptr factory = boost::make_shared<Module::TFMMusicMaker::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
    {
      const Char ID[] = {'T', 'F', 'E', 0};
      const Formats::Chiptune::TFMMusicMaker::Decoder::Ptr decoder = Formats::Chiptune::TFMMusicMaker::Ver13::CreateDecoder();
      const Module::TFM::Factory::Ptr factory = boost::make_shared<Module::TFMMusicMaker::Factory>(decoder);
      const PlayerPlugin::Ptr plugin = CreatePlayerPlugin(ID, decoder, factory);
      registrator.RegisterPlugin(plugin);
    }
  }
}
