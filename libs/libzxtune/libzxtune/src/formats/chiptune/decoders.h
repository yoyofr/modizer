/**
*
* @file
*
* @brief  Chiptune decoders factories
*
* @author vitamin.caig@gmail.com
*
**/

#pragma once

//library includes
#include <formats/chiptune.h>

namespace Formats
{
  namespace Chiptune
  {
    Decoder::Ptr CreatePSGDecoder();
    Decoder::Ptr CreateDigitalStudioDecoder();
    Decoder::Ptr CreateSoundTrackerDecoder();
    Decoder::Ptr CreateSoundTrackerCompiledDecoder();
    Decoder::Ptr CreateSoundTracker3Decoder();
    Decoder::Ptr CreateSoundTrackerProCompiledDecoder();
    Decoder::Ptr CreateASCSoundMaster0xDecoder();
    Decoder::Ptr CreateASCSoundMaster1xDecoder();
    Decoder::Ptr CreateProTracker2Decoder();
    Decoder::Ptr CreateProTracker3Decoder();
    Decoder::Ptr CreateProSoundMakerCompiledDecoder();
    Decoder::Ptr CreateGlobalTrackerDecoder();
    Decoder::Ptr CreateProTracker1Decoder();
    Decoder::Ptr CreateVTXDecoder();
    Decoder::Ptr CreatePackedYMDecoder();
    Decoder::Ptr CreateYMDecoder();
    Decoder::Ptr CreateTFDDecoder();
    Decoder::Ptr CreateTFCDecoder();
    Decoder::Ptr CreateVortexTracker2Decoder();
    Decoder::Ptr CreateChipTrackerDecoder();
    Decoder::Ptr CreateSampleTrackerDecoder();
    Decoder::Ptr CreateProDigiTrackerDecoder();
    Decoder::Ptr CreateSQTrackerDecoder();
    Decoder::Ptr CreateProSoundCreatorDecoder();
    Decoder::Ptr CreateFastTrackerDecoder();
    Decoder::Ptr CreateETrackerDecoder();
    Decoder::Ptr CreateSQDigitalTrackerDecoder();
    Decoder::Ptr CreateTFMMusicMaker05Decoder();
    Decoder::Ptr CreateTFMMusicMaker13Decoder();
    Decoder::Ptr CreateDigitalMusicMakerDecoder();
    Decoder::Ptr CreateTurboSoundDecoder();
    Decoder::Ptr CreateExtremeTracker1Decoder();
  }
}
