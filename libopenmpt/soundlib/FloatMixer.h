/*
 * FloatMixer.h
 * ------------
 * Purpose: Floating point mixer classes
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "MixerInterface.h"
#include "Resampler.h"

OPENMPT_NAMESPACE_BEGIN

template<int channelsOut, int channelsIn, typename out, typename in, int int2float>
struct IntToFloatTraits : public MixerTraits<channelsOut, channelsIn, out, in>
{
	static_assert(std::numeric_limits<input_t>::is_integer, "Input must be integer");
	static_assert(!std::numeric_limits<output_t>::is_integer, "Output must be floating point");

	static forceinline output_t Convert(const input_t x)
	{
		return static_cast<output_t>(x) * (static_cast<output_t>(1.0f) / static_cast<output_t>(int2float));
	}
};

typedef IntToFloatTraits<2, 1, mixsample_t, int8,  -int8_min>  Int8MToFloatS;
typedef IntToFloatTraits<2, 1, mixsample_t, int16, -int16_min> Int16MToFloatS;
typedef IntToFloatTraits<2, 2, mixsample_t, int8,  -int8_min>  Int8SToFloatS;
typedef IntToFloatTraits<2, 2, mixsample_t, int16, -int16_min> Int16SToFloatS;


//////////////////////////////////////////////////////////////////////////
// Interpolation templates

template<class Traits>
struct LinearInterpolation
{
	forceinline void Start(const ModChannel &, const CResampler &) { }

	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const Traits::output_t fract = CResampler::LinearTablef[posLo >> 8];

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			Traits::output_t srcVol = Traits::Convert(inBuffer[i]);
			Traits::output_t destVol = Traits::Convert(inBuffer[i + Traits::numChannelsIn]);

			outSample[i] = srcVol + fract * (destVol - srcVol);
		}
	}
};


template<class Traits>
struct FastSincInterpolation
{
	forceinline void Start(const ModChannel &, const CResampler &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const Traits::output_t *lut = CResampler::FastSincTablef + ((posLo >> 6) & 0x3FC);

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] =
				  lut[0] * Traits::Convert(inBuffer[i - Traits::numChannelsIn])
				+ lut[1] * Traits::Convert(inBuffer[i])
				+ lut[2] * Traits::Convert(inBuffer[i + Traits::numChannelsIn])
				+ lut[3] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn]);
		}
	}
};


template<class Traits>
struct PolyphaseInterpolation
{
	const typename Traits::output_t *sinc;

	forceinline void Start(const ModChannel &chn, const CResampler &resampler)
	{
		sinc = (((chn.nInc > 0x13000) || (chn.nInc < -0x13000)) ?
			(((chn.nInc > 0x18000) || (chn.nInc < -0x18000)) ? resampler.gDownsample2x : resampler.gDownsample13x) : resampler.gKaiserSinc);
	}

	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const Traits::output_t *lut = sinc + ((posLo >> (16 - SINC_PHASES_BITS)) & SINC_MASK) * SINC_WIDTH;

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] =
				  lut[0] * Traits::Convert(inBuffer[i - 3 * Traits::numChannelsIn])
				+ lut[1] * Traits::Convert(inBuffer[i - 2 * Traits::numChannelsIn])
				+ lut[2] * Traits::Convert(inBuffer[i - Traits::numChannelsIn])
				+ lut[3] * Traits::Convert(inBuffer[i])
				+ lut[4] * Traits::Convert(inBuffer[i + Traits::numChannelsIn])
				+ lut[5] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn])
				+ lut[6] * Traits::Convert(inBuffer[i + 3 * Traits::numChannelsIn])
				+ lut[7] * Traits::Convert(inBuffer[i + 4 * Traits::numChannelsIn]);
		}
	}
};


template<class Traits>
struct FIRFilterInterpolation
{
	const typename Traits::output_t *WFIRlut;

	forceinline void Start(const ModChannel &, const CResampler &resampler)
	{
		WFIRlut = resampler.m_WindowedFIR.lut;
	}

	forceinline void End(const ModChannel &) { }

	forceinline void operator() (typename Traits::outbuf_t &outSample, const typename Traits::input_t * const inBuffer, const int32 posLo)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");
		const Traits::output_t * const lut = WFIRlut + (((posLo + WFIR_FRACHALVE) >> WFIR_FRACSHIFT) & WFIR_FRACMASK);

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			outSample[i] =
				  lut[0] * Traits::Convert(inBuffer[i - 3 * Traits::numChannelsIn])
				+ lut[1] * Traits::Convert(inBuffer[i - 2 * Traits::numChannelsIn])
				+ lut[2] * Traits::Convert(inBuffer[i - Traits::numChannelsIn])
				+ lut[3] * Traits::Convert(inBuffer[i])
				+ lut[4] * Traits::Convert(inBuffer[i + Traits::numChannelsIn])
				+ lut[5] * Traits::Convert(inBuffer[i + 2 * Traits::numChannelsIn])
				+ lut[6] * Traits::Convert(inBuffer[i + 3 * Traits::numChannelsIn])
				+ lut[7] * Traits::Convert(inBuffer[i + 4 * Traits::numChannelsIn]);
		}
	}
};


//////////////////////////////////////////////////////////////////////////
// Mixing templates (add sample to stereo mix)

template<class Traits>
struct NoRamp
{
	typename Traits::output_t lVol, rVol;

	forceinline void Start(const ModChannel &chn)
	{
		lVol = static_cast<Traits::output_t>(chn.leftVol) * (1.0f / 4096.0f);
		rVol = static_cast<Traits::output_t>(chn.rightVol) * (1.0f / 4096.0f);
	}

	forceinline void End(const ModChannel &) { }
};


struct Ramp
{
	int32 lRamp, rRamp;

	forceinline void Start(const ModChannel &chn)
	{
		lRamp = chn.rampLeftVol;
		rRamp = chn.rampRightVol;
	}

	forceinline void End(ModChannel &chn)
	{
		chn.rampLeftVol = lRamp; chn.leftVol = lRamp >> VOLUMERAMPPRECISION;
		chn.rampRightVol = rRamp; chn.rightVol = rRamp >> VOLUMERAMPPRECISION;
	}
};


// Legacy optimization: If chn.nLeftVol == chn.nRightVol, save one multiplication instruction
template<class Traits>
struct MixMonoFastNoRamp : public NoRamp<Traits>
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &chn, typename Traits::output_t * const outBuffer)
	{
		Traits::output_t vol = outSample[0] * lVol;
		for(int i = 0; i < Traits::numChannelsOut; i++)
		{
			outBuffer[i] += vol;
		}
	}
};


template<class Traits>
struct MixMonoNoRamp : public NoRamp<Traits>
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &, typename Traits::output_t * const outBuffer)
	{
		outBuffer[0] += outSample[0] * lVol;
		outBuffer[1] += outSample[0] * rVol;
	}
};


template<class Traits>
struct MixMonoRamp : public Ramp
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &chn, typename Traits::output_t * const outBuffer)
	{
		// TODO volume is not float, can we optimize this?
		lRamp += chn.leftRamp;
		rRamp += chn.rightRamp;
		outBuffer[0] += outSample[0] * (lRamp >> VOLUMERAMPPRECISION) * (1.0f / 4096.0f);
		outBuffer[1] += outSample[0] * (rRamp >> VOLUMERAMPPRECISION) * (1.0f / 4096.0f);
	}
};


template<class Traits>
struct MixStereoNoRamp : public NoRamp<Traits>
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &, typename Traits::output_t * const outBuffer)
	{
		outBuffer[0] += outSample[0] * lVol;
		outBuffer[1] += outSample[1] * rVol;
	}
};


template<class Traits>
struct MixStereoRamp : public Ramp
{
	forceinline void operator() (const typename Traits::outbuf_t &outSample, const ModChannel &chn, typename Traits::output_t * const outBuffer)
	{
		// TODO volume is not float, can we optimize this?
		lRamp += chn.leftRamp;
		rRamp += chn.rightRamp;
		outBuffer[0] += outSample[0] * (lRamp >> VOLUMERAMPPRECISION) * (1.0f / 4096.0f);
		outBuffer[1] += outSample[1] * (rRamp >> VOLUMERAMPPRECISION) * (1.0f / 4096.0f);
	}
};


//////////////////////////////////////////////////////////////////////////
// Filter templates


template<class Traits>
struct NoFilter
{
	forceinline void Start(const ModChannel &) { }
	forceinline void End(const ModChannel &) { }

	forceinline void operator() (const typename Traits::outbuf_t &, const ModChannel &) { }
};


// Resonant filter
template<class Traits>
struct ResonantFilter
{
	// Filter history
	typename Traits::output_t fy[Traits::numChannelsIn][2];

	forceinline void Start(const ModChannel &chn)
	{
		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			fy[i][0] = chn.nFilter_Y[i][0];
			fy[i][1] = chn.nFilter_Y[i][1];
		}
	}

	forceinline void End(ModChannel &chn)
	{
		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			chn.nFilter_Y[i][0] = fy[i][0];
			chn.nFilter_Y[i][1] = fy[i][1];
		}
	}

	// Filter values are clipped to double the input range
#define ClipFilter(x) Clamp(x, static_cast<Traits::output_t>(-2.0f), static_cast<Traits::output_t>(2.0f))

	forceinline void operator() (typename Traits::outbuf_t &outSample, const ModChannel &chn)
	{
		static_assert(Traits::numChannelsIn <= Traits::numChannelsOut, "Too many input channels");

		for(int i = 0; i < Traits::numChannelsIn; i++)
		{
			Traits::output_t val = outSample[i] * chn.nFilter_A0 + ClipFilter(fy[i][0]) * chn.nFilter_B0 + ClipFilter(fy[i][1]) * chn.nFilter_B1;
			fy[i][1] = fy[i][0];
			fy[i][0] = val - (outSample[i] * chn.nFilter_HP);
			outSample[i] = val;
		}
	}

#undef ClipFilter
};


OPENMPT_NAMESPACE_END
