#ifndef AUDIOSYS_H__
#define AUDIOSYS_H__

#include "nestypes.h"
#include "audiohandler.h"
#include "nezplug.h"

#ifdef __cplusplus
extern "C" {
#endif

enum
{
   NES_AUDIO_FILTER_NONE,
   NES_AUDIO_FILTER_LOWPASS,
   NES_AUDIO_FILTER_WEIGHTED,
   NES_AUDIO_FILTER_LOWPASS_WEIGHTED
};


void NESAudioRender(NEZ_PLAY*, Int16 *bufp, Uint buflen);
void NESAudioHandlerInstall(NEZ_PLAY *, const NES_AUDIO_HANDLER * ph);
void NESAudioHandlerTerminate(NEZ_PLAY *);
void NESAudioFrequencySet(NEZ_PLAY *, Uint freq);
Uint NESAudioFrequencyGet(NEZ_PLAY *);
void NESAudioChannelSet(NEZ_PLAY *, Uint ch);
Uint NESAudioChannelGet(NEZ_PLAY *);
void NESAudioHandlerInitialize(NEZ_PLAY *);
void NESVolumeHandlerInstall(NEZ_PLAY *, const NES_VOLUME_HANDLER * ph);
void NESVolumeHandlerTerminate(NEZ_PLAY *);
void NESVolume(NEZ_PLAY *, Uint volume);
void NESAudioFilterSet(NEZ_PLAY*, Uint filter);

#ifdef __cplusplus
}
#endif

#endif /* AUDIOSYS_H__ */
