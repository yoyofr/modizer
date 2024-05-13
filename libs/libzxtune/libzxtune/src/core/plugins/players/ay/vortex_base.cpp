/**
* 
* @file
*
* @brief  VortexTracker-based chiptunes support
*
* @author vitamin.caig@gmail.com
*
**/

//local includes
#include "vortex_base.h"
//library includes
#include <math/numeric.h>
//boost includes
#include <boost/make_shared.hpp>

namespace Module
{
namespace Vortex
{
  const uint_t LIMITER = ~uint_t(0);

  typedef boost::array<uint8_t, 256> VolumeTable;

  //Volume table of Pro Tracker 3.3x - 3.4x
  const VolumeTable Vol33_34 =
  { {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09,
    0x00, 0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x09, 0x0a, 0x0b,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  } };

  //Volume table of Pro Tracker 3.5x
  const VolumeTable Vol35 =
  { {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
    0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04,
    0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06,
    0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07,
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08,
    0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x04, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09,
    0x00, 0x01, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x09, 0x0a,
    0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b,
    0x00, 0x01, 0x02, 0x02, 0x03, 0x04, 0x05, 0x06, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c,
    0x00, 0x01, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0a, 0x0b, 0x0c, 0x0d,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
  } };

  //helper for sliding processing
  struct Slider
  {
    Slider() : Period(), Value(), Counter(), Delta()
    {
    }
    uint_t Period;
    int_t Value;
    uint_t Counter;
    int_t Delta;

    bool Update()
    {
      if (Counter && !--Counter)
      {
        Value += Delta;
        Counter = Period;
        return true;
      }
      return false;
    }

    void Reset()
    {
      Counter = 0;
      Value = 0;
    }
  };

  //internal per-channel state type
  struct ChannelState
  {
    ChannelState()
      : Enabled(false), Envelope(false)
      , Note()
      , SampleNum(0), PosInSample(0)
      , OrnamentNum(0), PosInOrnament(0)
      , Volume(15), VolSlide(0)
      , ToneSlider(), SlidingTargetNote(LIMITER), ToneAccumulator(0)
      , EnvSliding(), NoiseSliding()
      , VibrateCounter(0), VibrateOn(), VibrateOff()
    {
    }

    bool Enabled;
    bool Envelope;
    uint_t Note;
    uint_t SampleNum;
    uint_t PosInSample;
    uint_t OrnamentNum;
    uint_t PosInOrnament;
    uint_t Volume;
    int_t VolSlide;
    Slider ToneSlider;
    uint_t SlidingTargetNote;
    int_t ToneAccumulator;
    int_t EnvSliding;
    int_t NoiseSliding;
    uint_t VibrateCounter;
    uint_t VibrateOn;
    uint_t VibrateOff;
  };

  //internal common state type
  struct CommonState
  {
    CommonState()
      : EnvBase()
      , NoiseBase()
    {
    }

    uint_t EnvBase;
    Slider EnvSlider;
    uint_t NoiseBase;
  };

  struct State
  {
    boost::array<ChannelState, AYM::TRACK_CHANNELS> ChanState;
    CommonState CommState;
  };

  //simple player type
  class DataRenderer : public AYM::DataRenderer
  {
  public:
    DataRenderer(ModuleData::Ptr data, uint_t trackChannelStart)
      : Data(data)
      , Version(Data->Version)
      , VolTable(Version <= 4 ? Vol33_34 : Vol35)
      , TrackChannelStart(trackChannelStart)
    {
    }

    virtual void Reset()
    {
      PlayerState = State();
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
      if (0 == state.Line())
      {
        PlayerState.CommState.NoiseBase = 0;
      }

      if (const Line::Ptr line = state.LineObject())
      {
        for (uint_t chan = 0; chan != PlayerState.ChanState.size(); ++chan)
        {
          if (const Cell::Ptr src = line->GetChannel(TrackChannelStart + chan))
          {
            GetNewChannelState(*src, PlayerState.ChanState[chan], track);
          }
        }
      }
    }

    void GetNewChannelState(const Cell& src, ChannelState& dst, AYM::TrackBuilder& track)
    {
      const int_t prevSlide = dst.ToneSlider.Value;
      if (const bool* enabled = src.GetEnabled())
      {
        dst.PosInSample = dst.PosInOrnament = 0;
        dst.VolSlide = dst.EnvSliding = dst.NoiseSliding = 0;
        dst.ToneSlider.Reset();
        dst.ToneAccumulator = 0;
        dst.VibrateCounter = 0;
        dst.Enabled = *enabled;
      }
      if (const uint_t* note = src.GetNote())
      {
        dst.Note = *note;
      }
      if (const uint_t* sample = src.GetSample())
      {
        dst.SampleNum = *sample;
      }
      if (const uint_t* ornament = src.GetOrnament())
      {
        dst.OrnamentNum = *ornament;
        dst.PosInOrnament = 0;
      }
      if (const uint_t* volume = src.GetVolume())
      {
        dst.Volume = *volume;
      }
      for (CommandsIterator it = src.GetCommands(); it; ++it)
      {
        switch (it->Type)
        {
        case GLISS:
          dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
          dst.ToneSlider.Delta = it->Param2;
          dst.SlidingTargetNote = LIMITER;
          dst.VibrateCounter = 0;
          if (0 == dst.ToneSlider.Counter && Version >= 7)
          {
            ++dst.ToneSlider.Counter;
          }
          break;
        case GLISS_NOTE:
          dst.ToneSlider.Period = dst.ToneSlider.Counter = it->Param1;
          dst.ToneSlider.Delta = it->Param2;
          dst.SlidingTargetNote = it->Param3;
          dst.VibrateCounter = 0;
          if (Version >= 6)
          {
            dst.ToneSlider.Value = prevSlide;
          }
          //tone up                                     freq up
          if (bool(dst.Note < dst.SlidingTargetNote) != bool(dst.ToneSlider.Delta < 0))
          {
            dst.ToneSlider.Delta = -dst.ToneSlider.Delta;
          }
          break;
        case SAMPLEOFFSET:
          dst.PosInSample = it->Param1;
          break;
        case ORNAMENTOFFSET:
          dst.PosInOrnament = it->Param1;
          break;
        case VIBRATE:
          dst.VibrateCounter = dst.VibrateOn = it->Param1;
          dst.VibrateOff = it->Param2;
          dst.ToneSlider.Value = 0;
          dst.ToneSlider.Counter = 0;
          break;
        case SLIDEENV:
          PlayerState.CommState.EnvSlider.Period = PlayerState.CommState.EnvSlider.Counter = it->Param1;
          PlayerState.CommState.EnvSlider.Delta = it->Param2;
          break;
        case ENVELOPE:
          track.SetEnvelopeType(it->Param1);
          PlayerState.CommState.EnvBase = it->Param2;
          dst.Envelope = true;
          PlayerState.CommState.EnvSlider.Reset();
          dst.PosInOrnament = 0;
          break;
        case NOENVELOPE:
          dst.Envelope = false;
          dst.PosInOrnament = 0;
          break;
        case NOISEBASE:
          PlayerState.CommState.NoiseBase = it->Param1;
          break;
        }
      }
    }

    void SynthesizeChannelsData(AYM::TrackBuilder& track)
    {
      int_t envelopeAddon = 0;
      for (uint_t chan = 0; chan != PlayerState.ChanState.size(); ++chan)
      {
        AYM::ChannelBuilder chanSynt = track.GetChannel(chan);
        ChannelState& dst = PlayerState.ChanState[chan];
        SynthesizeChannel(dst, chanSynt, track, envelopeAddon);
        //update vibrato
        if (dst.VibrateCounter > 0 && !--dst.VibrateCounter)
        {
          dst.Enabled = !dst.Enabled;
          dst.VibrateCounter = dst.Enabled ? dst.VibrateOn : dst.VibrateOff;
        }
      }
      const int_t envPeriod = envelopeAddon + PlayerState.CommState.EnvSlider.Value + PlayerState.CommState.EnvBase;
      track.SetEnvelopeTone(envPeriod);
      PlayerState.CommState.EnvSlider.Update();
    }

    void SynthesizeChannel(ChannelState& dst, AYM::ChannelBuilder& channel, AYM::TrackBuilder& track, int_t& envelopeAddon)
    {
      if (!dst.Enabled)
      {
        channel.SetLevel(0);
        return;
      }

      const Sample& curSample = Data->Samples.Get(dst.SampleNum);
      const Sample::Line& curSampleLine = curSample.GetLine(dst.PosInSample);
      const Ornament& curOrnament = Data->Ornaments.Get(dst.OrnamentNum);

      //calculate tone
      const int_t toneAddon = curSampleLine.ToneOffset + dst.ToneAccumulator;
      if (curSampleLine.KeepToneOffset)
      {
        dst.ToneAccumulator = toneAddon;
      }
      const int_t halfTones = int_t(dst.Note) + curOrnament.GetLine(dst.PosInOrnament);
      const int_t toneOffset = dst.ToneSlider.Value + toneAddon;

      //apply tone
      channel.SetTone(halfTones, toneOffset);
      if (curSampleLine.ToneMask)
      {
        channel.DisableTone();
      }
      //apply level
      dst.VolSlide = Math::Clamp<int_t>(dst.VolSlide + curSampleLine.VolumeSlideAddon, -15, 15);
      channel.SetLevel(GetVolume(dst.Volume, Math::Clamp<int_t>(dst.VolSlide + curSampleLine.Level, 0, 15)));
      //apply envelope
      if (dst.Envelope && !curSampleLine.EnvMask)
      {
        channel.EnableEnvelope();
      }
      //apply noise
      if (curSampleLine.NoiseMask)
      {
        const int_t envAddon = curSampleLine.NoiseOrEnvelopeOffset + dst.EnvSliding;
        if (curSampleLine.KeepNoiseOrEnvelopeOffset)
        {
          dst.EnvSliding = envAddon;
        }
        envelopeAddon += envAddon;
        channel.DisableNoise();
      }
      else
      {
        const int_t noiseAddon = curSampleLine.NoiseOrEnvelopeOffset + dst.NoiseSliding;
        if (curSampleLine.KeepNoiseOrEnvelopeOffset)
        {
          dst.NoiseSliding = noiseAddon;
        }
        track.SetNoise(PlayerState.CommState.NoiseBase + noiseAddon);
      }

      if (dst.ToneSlider.Update() &&
          LIMITER != dst.SlidingTargetNote)
      {
        const int_t absoluteSlidingRange = track.GetSlidingDifference(dst.Note, dst.SlidingTargetNote);
        const int_t realSlidingRange = absoluteSlidingRange - toneOffset;

        if ((dst.ToneSlider.Delta > 0 && realSlidingRange < dst.ToneSlider.Delta) ||
          (dst.ToneSlider.Delta < 0 && realSlidingRange > dst.ToneSlider.Delta))
        {
          //slided to target note
          dst.Note = dst.SlidingTargetNote;
          dst.SlidingTargetNote = LIMITER;
          dst.ToneSlider.Value = 0;
          dst.ToneSlider.Counter = 0;
        }
      }

      if (++dst.PosInSample >= curSample.GetSize())
      {
        dst.PosInSample = curSample.GetLoop();
      }
      if (++dst.PosInOrnament >= curOrnament.GetSize())
      {
        dst.PosInOrnament = curOrnament.GetLoop();
      }
    }

    uint_t GetVolume(uint_t volume, uint_t level)
    {
      return VolTable[volume * 16 + level];
    }
  private:
    const ModuleData::Ptr Data;
    const uint_t Version;
    const VolumeTable& VolTable;
    const uint_t TrackChannelStart;
    State PlayerState;
  };

  String GetFreqTable(NoteTable table, uint_t version)
  {
    switch (table)
    {
    case PROTRACKER:
      return version <= 3 ? TABLE_PROTRACKER3_3 : TABLE_PROTRACKER3_4;
    case SOUNDTRACKER:
      return TABLE_PROTRACKER3_ST;
    case ASM:
      return version <= 3 ? TABLE_PROTRACKER3_3_ASM : TABLE_PROTRACKER3_4_ASM;
    case REAL:
      return version <= 3 ? TABLE_PROTRACKER3_3_REAL : TABLE_PROTRACKER3_4_REAL;
    case NATURAL:
      return TABLE_NATURAL_SCALED;
    default:
      //assert(!"Unknown frequency table for Vortex-based modules");
      return TABLE_PROTRACKER3_3;
    }
  }

  AYM::DataRenderer::Ptr CreateDataRenderer(ModuleData::Ptr data, uint_t trackChannelStart)
  {
    return boost::make_shared<DataRenderer>(data, trackChannelStart);
  }
}
}
