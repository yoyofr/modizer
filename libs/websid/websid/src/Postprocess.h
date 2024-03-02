/*
* Add-on audio postprocessing.
*
* This postprocessing is NOT based on original SID chip functionality: It
* adds pseudo stereo as well as per voice stereo panning.
*
*
* WebSid (c) 2023 Jürgen Wothke
* version 0.96
*
* Terms of Use: This software is licensed under a CC BY-NC-SA
* (http://creativecommons.org/licenses/by-nc-sa/4.0/).
*/

#ifndef WEBSID_POSTPROCESS_H
#define WEBSID_POSTPROCESS_H

#include "base.h"

namespace websid {

	class Postprocess {
	public:
		static void init(int32_t chunk_size, uint32_t sample_rate);

		static void setReverbLevel(int32_t chunk_size, uint16_t reverb_level);
		static uint16_t getReverbLevel();

		static void setStereoLevel(int32_t chunk_size, int32_t effect_level);
		static int32_t getStereoLevel();

		static void setHeadphoneMode(int32_t chunk_size, uint8_t mode);
		static uint8_t getHeadphoneMode();

		static void applyStereoEnhance(int16_t* buffer, uint16_t chunk_size);

		static void setPanning(uint8_t sid_idx, uint8_t voice_idx, float panning);
		static float getPanning(uint8_t sid_idx, uint8_t voice_idx);

		static void initPanningCfg(float p0, float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9,
								float p10, float p11, float p12, float p13, float p14, float p15, float p16, float p17, float p18, float p19,
								float p20, float p21, float p22, float p23, float p24, float p25, float p26, float p27, float p28, float p29);
	};
};




#endif