/*
 * SampleFormatConverters.h
 * ------------------------
 * Purpose: Functions and functors for reading and converting pretty much any uncompressed sample format supported by OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once


OPENMPT_NAMESPACE_BEGIN


struct ModSample;


// Byte offsets, from lowest significant to highest significant byte (for various functor template parameters)
#define littleEndian32 0, 1, 2, 3
#define littleEndian24 0, 1, 2
#define littleEndian16 0, 1

#define bigEndian32 3, 2, 1, 0
#define bigEndian24 2, 1, 0
#define bigEndian16 1, 0



namespace SC { // SC = _S_ample_C_onversion




// Every sample decoding functor has to typedef its input_t and output_t
// and has to provide a static const input_inc member
// which describes by how many input_t elements inBuf has to be incremented between invocations.
// input_inc is normally 1 except when decoding e.g. bigger sample values
// from multiple char values.


// decodes signed 7bit values stored as signed int8
struct DecodeInt7
{
	typedef char input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return Clamp(int8(*inBuf), int8(-64), int8(63)) * 2;
	}
};

struct DecodeInt8
{
	typedef char input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return int8(*inBuf);
	}
};

struct DecodeUint8
{
	typedef char input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return int8(int(uint8(*inBuf)) - 128);
	}
};

struct DecodeInt8Delta
{
	typedef char input_t;
	typedef int8 output_t;
	static const int input_inc = 1;
	uint8 delta;
	DecodeInt8Delta() : delta(0) { }
	forceinline output_t operator() (const input_t *inBuf)
	{
		delta += uint8(*inBuf);
		return int8(delta);
	}
};

template <uint16 offset, size_t loByteIndex, size_t hiByteIndex>
struct DecodeInt16
{
	typedef char input_t;
	typedef int16 output_t;
	static const int input_inc = 2;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return (uint8(inBuf[loByteIndex]) | (uint8(inBuf[hiByteIndex]) << 8)) - offset;
	}
};

template <size_t loByteIndex, size_t hiByteIndex>
struct DecodeInt16Delta
{
	typedef char input_t;
	typedef int16 output_t;
	static const int input_inc = 2;
	uint16 delta;
	DecodeInt16Delta() : delta(0) { }
	forceinline output_t operator() (const input_t *inBuf)
	{
		delta += uint8(inBuf[loByteIndex]) | (uint8(inBuf[hiByteIndex]) << 8);
		return int16(delta);
	}
};

struct DecodeInt16Delta8
{
	typedef char input_t;
	typedef int16 output_t;
	static const int input_inc = 2;
	uint16 delta;
	DecodeInt16Delta8() : delta(0) { }
	forceinline output_t operator() (const input_t *inBuf)
	{
		delta += uint8(inBuf[0]);
		int16 result = delta & 0xFF;
		delta += uint8(inBuf[1]);
		result |= (delta << 8);
		return result;
	}
};

template <uint32 offset, size_t loByteIndex, size_t midByteIndex, size_t hiByteIndex>
struct DecodeInt24
{
	typedef char input_t;
	typedef int32 output_t;
	static const int input_inc = 3;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return ((uint8(inBuf[loByteIndex]) << 8) | (uint8(inBuf[midByteIndex]) << 16) | (uint8(inBuf[hiByteIndex]) << 24)) - offset;
	}
};

template <uint32 offset, size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct DecodeInt32
{
	typedef char input_t;
	typedef int32 output_t;
	static const int input_inc = 4;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return (uint8(inBuf[loLoByteIndex]) | (uint8(inBuf[loHiByteIndex]) << 8) | (uint8(inBuf[hiLoByteIndex]) << 16) | (uint8(inBuf[hiHiByteIndex]) << 24)) - offset;
	}
};

template <size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct DecodeFloat32
{
	typedef char input_t;
	typedef float32 output_t;
	static const int input_inc = 4;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return IEEE754binary32LE(uint8(inBuf[loLoByteIndex]), uint8(inBuf[loHiByteIndex]), uint8(inBuf[hiLoByteIndex]), uint8(inBuf[hiHiByteIndex]));
	}
};

template <size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct DecodeScaledFloat32
{
	typedef char input_t;
	typedef float32 output_t;
	static const int input_inc = 4;
	float factor;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return factor * IEEE754binary32LE(uint8(inBuf[loLoByteIndex]), uint8(inBuf[loHiByteIndex]), uint8(inBuf[hiLoByteIndex]), uint8(inBuf[hiHiByteIndex]));
	}
	forceinline DecodeScaledFloat32(float scaleFactor)
		: factor(scaleFactor)
	{
		return;
	}
};

template <typename Tsample>
struct DecodeIdentity
{
	typedef Tsample input_t;
	typedef Tsample output_t;
	static const int input_inc = 1;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return *inBuf;
	}
};



// Shift input_t down by shift and saturate to output_t.
template <typename Tdst, typename Tsrc, int shift>
struct ConvertShift
{
	typedef Tsrc input_t;
	typedef Tdst output_t;
	forceinline output_t operator() (input_t val)
	{
		return mpt::saturate_cast<output_t>(val >> shift);
	}
};




// Every sample conversion functor has to typedef its input_t and output_t.
// The input_t argument is taken by value because we only deal with per-single-sample conversions here.


// straight forward type conversions, clamping when converting from floating point.
template <typename Tdst, typename Tsrc>
struct Convert;

template <typename Tid>
struct Convert<Tid, Tid>
{
	typedef Tid input_t;
	typedef Tid output_t;
	forceinline output_t operator() (input_t val)
	{
		return val;
	}
};

template <>
struct Convert<int8, int16>
{
	typedef int16 input_t;
	typedef int8 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int8(val >> 8);
	}
};

template <>
struct Convert<int8, int24>
{
	typedef int24 input_t;
	typedef int8 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int8(val >> 16);
	}
};

template <>
struct Convert<int8, int32>
{
	typedef int32 input_t;
	typedef int8 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int8(val >> 24);
	}
};

template <>
struct Convert<int8, float32>
{
	typedef float32 input_t;
	typedef int8 output_t;
	forceinline output_t operator() (input_t val)
	{
		Limit(val, -1.0f, 1.0f);
		val *= 128.0f;
		// MSVC with x87 floating point math calls floor for the more intuitive version
		// return mpt::saturate_cast<int8>(static_cast<int>(std::floor(val + 0.5f)));
		return mpt::saturate_cast<int8>(static_cast<int>(val * 2.0f + 1.0f) >> 1);
	}
};

template <>
struct Convert<int16, int8>
{
	typedef int8 input_t;
	typedef int16 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int16(val << 8);
	}
};

template <>
struct Convert<int16, int24>
{
	typedef int24 input_t;
	typedef int16 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int16(val >> 8);
	}
};

template <>
struct Convert<int16, int32>
{
	typedef int32 input_t;
	typedef int16 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int16(val >> 16);
	}
};

template <>
struct Convert<int16, float32>
{
	typedef float32 input_t;
	typedef int16 output_t;
	forceinline output_t operator() (input_t val)
	{
		Limit(val, -1.0f, 1.0f);
		val *= 32768.0f;
		// MSVC with x87 floating point math calls floor for the more intuitive version
		// return mpt::saturate_cast<int16>(static_cast<int>(std::floor(val + 0.5f)));
		return mpt::saturate_cast<int16>(static_cast<int>(val * 2.0f + 1.0f) >> 1);
	}
};

template <>
struct Convert<int8, double>
{
	typedef double input_t;
	typedef int8 output_t;
	forceinline output_t operator() (input_t val)
	{
		Limit(val, -1.0, 1.0);
		val *= 128.0;
		// MSVC with x87 floating point math calls floor for the more intuitive version
		// return mpt::saturate_cast<int16>(static_cast<int>(std::floor(val + 0.5)));
		return mpt::saturate_cast<int8>(static_cast<int>(val * 2.0 + 1.0) >> 1);
	}
};

template <>
struct Convert<int16, double>
{
	typedef double input_t;
	typedef int16 output_t;
	forceinline output_t operator() (input_t val)
	{
		Limit(val, -1.0, 1.0);
		val *= 32768.0;
		// MSVC with x87 floating point math calls floor for the more intuitive version
		// return mpt::saturate_cast<int16>(static_cast<int>(std::floor(val + 0.5)));
		return mpt::saturate_cast<int16>(static_cast<int>(val * 2.0 + 1.0) >> 1);
	}
};

template <>
struct Convert<int24, int8>
{
	typedef int8 input_t;
	typedef int24 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int24(val << 16);
	}
};

template <>
struct Convert<int24, int16>
{
	typedef int16 input_t;
	typedef int24 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int24(val << 8);
	}
};

template <>
struct Convert<int24, int32>
{
	typedef int32 input_t;
	typedef int24 output_t;
	forceinline output_t operator() (input_t val)
	{
		return int24(val >> 8);
	}
};

template <>
struct Convert<float32, int8>
{
	typedef int8 input_t;
	typedef float32 output_t;
	forceinline output_t operator() (input_t val)
	{
		return val * (1.0f / static_cast<float>((unsigned int)1<<7));
	}
};

template <>
struct Convert<float32, int16>
{
	typedef int16 input_t;
	typedef float32 output_t;
	forceinline output_t operator() (input_t val)
	{
		return val * (1.0f / static_cast<float>((unsigned int)1<<15));
	}
};

template <>
struct Convert<float32, int32>
{
	typedef int32 input_t;
	typedef float32 output_t;
	forceinline output_t operator() (input_t val)
	{
		return val * (1.0f / static_cast<float>((unsigned int)1<<31));
	}
};

template <>
struct Convert<double, int8>
{
	typedef int8 input_t;
	typedef double output_t;
	forceinline output_t operator() (input_t val)
	{
		return val * (1.0 / static_cast<double>((unsigned int)1<<7));
	}
};

template <>
struct Convert<double, int16>
{
	typedef int16 input_t;
	typedef double output_t;
	forceinline output_t operator() (input_t val)
	{
		return val * (1.0 / static_cast<double>((unsigned int)1<<15));
	}
};

template <>
struct Convert<double, int32>
{
	typedef int32 input_t;
	typedef double output_t;
	forceinline output_t operator() (input_t val)
	{
		return val * (1.0 / static_cast<double>((unsigned int)1<<31));
	}
};

template <>
struct Convert<double, float>
{
	typedef float input_t;
	typedef double output_t;
	forceinline output_t operator() (input_t val)
	{
		return val;
	}
};

template <>
struct Convert<float, double>
{
	typedef double input_t;
	typedef float output_t;
	forceinline output_t operator() (input_t val)
	{
		return static_cast<float>(val);
	}
};


template <typename Tdst, typename Tsrc, int fractionalBits, bool clipOutput>
struct ConvertFixedPoint;

template <int fractionalBits, bool clipOutput>
struct ConvertFixedPoint<uint8, int32, fractionalBits, clipOutput>
{
	typedef int32 input_t;
	typedef uint8 output_t;
	static const int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	forceinline output_t operator() (input_t val)
	{
		STATIC_ASSERT(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		STATIC_ASSERT(shiftBits >= 1);
		val = (val + (1<<(shiftBits-1))) >> shiftBits; // round
		if(val < int8_min) val = int8_min;
		if(val > int8_max) val = int8_max;
		return (uint8)(val+0x80); // unsigned
	}
};

template <int fractionalBits, bool clipOutput>
struct ConvertFixedPoint<int16, int32, fractionalBits, clipOutput>
{
	typedef int32 input_t;
	typedef int16 output_t;
	static const int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	forceinline output_t operator() (input_t val)
	{
		STATIC_ASSERT(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		STATIC_ASSERT(shiftBits >= 1);
		val = (val + (1<<(shiftBits-1))) >> shiftBits; // round
		if(val < int16_min) val = int16_min;
		if(val > int16_max) val = int16_max;
		return (int16)val;
	}
};

template <int fractionalBits, bool clipOutput>
struct ConvertFixedPoint<int24, int32, fractionalBits, clipOutput>
{
	typedef int32 input_t;
	typedef int24 output_t;
	static const int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	forceinline output_t operator() (input_t val)
	{
		STATIC_ASSERT(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		STATIC_ASSERT(shiftBits >= 1);
		val = (val + (1<<(shiftBits-1))) >> shiftBits; // round
		if(val < int24_min) val = int24_min;
		if(val > int24_max) val = int24_max;
		return (int24)val;
	}
};

template <int fractionalBits, bool clipOutput>
struct ConvertFixedPoint<int32, int32, fractionalBits, clipOutput>
{
	typedef int32 input_t;
	typedef int32 output_t;
	forceinline output_t operator() (input_t val)
	{
		STATIC_ASSERT(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		return (int32)(Clamp(val, (int)-((1<<fractionalBits)-1), (int)(1<<fractionalBits)-1)) << (sizeof(input_t)*8-1-fractionalBits);
	}
};

template <int fractionalBits, bool clipOutput>
struct ConvertFixedPoint<float32, int32, fractionalBits, clipOutput>
{
	typedef int32 input_t;
	typedef float32 output_t;
	const float factor;
	forceinline ConvertFixedPoint()
		: factor( 1.0f / static_cast<float>(1 << fractionalBits) )
	{
		return;
	}
	forceinline output_t operator() (input_t val)
	{
		STATIC_ASSERT(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		if(clipOutput)
		{
			float32 out = val * factor;
			if(out < -1.0f) out = -1.0f;
			if(out > 1.0f) out = 1.0f;
			return out;
		} else
		{
			return val * factor;
		}
	}
};




// Reads sample data with Func and passes it directly to Func2.
// Func1::output_t and Func2::input_t must be identical
template <typename Func2, typename Func1>
struct ConversionChain
{
	typedef typename Func1::input_t input_t;
	typedef typename Func2::output_t output_t;
	static const int input_inc = Func1::input_inc;
	Func1 func1;
	Func2 func2;
	forceinline output_t operator() (const input_t *inBuf)
	{
		return func2(func1(inBuf));
	}
	forceinline ConversionChain(Func2 f2 = Func2(), Func1 f1 = Func1())
		: func1(f1)
		, func2(f2)
	{
		return;
	}
};




template <typename Tsample>
struct Normalize;

template <>
struct Normalize<int32>
{
	typedef int32 input_t;
	typedef int32 output_t;
	typedef uint32 peak_t;
	uint32 maxVal;
	forceinline Normalize() : maxVal(0) { }
	forceinline void FindMax(input_t val)
	{
		if(val < 0)
		{
			if(val == int32_min)
			{
				maxVal = static_cast<uint32>(-static_cast<int64>(int32_min));
				return;
			}
			val = -val;
		}
		if(static_cast<uint32>(val) > maxVal)
		{
			maxVal = static_cast<uint32>(val);
		}
	}
	forceinline bool IsSilent() const
	{
		return maxVal == 0;
	}
	forceinline output_t operator() (input_t val)
	{
		return Util::muldivrfloor(val, (uint32)1 << 31, maxVal);
	}
	forceinline peak_t GetSrcPeak() const
	{
		return maxVal;
	}
};

template <>
struct Normalize<float32>
{
	typedef float32 input_t;
	typedef float32 output_t;
	typedef float32 peak_t;
	float maxVal;
	float maxValInv;
	forceinline Normalize() : maxVal(0.0f), maxValInv(1.0f) { }
	forceinline void FindMax(input_t val)
	{
		float absval = std::fabs(val);
		if(absval > maxVal)
		{
			maxVal = absval;
		}
	}
	forceinline bool IsSilent()
	{
		if(maxVal == 0.0f)
		{
			maxValInv = 1.0f;
			return true;
		} else
		{
			maxValInv = 1.0f / maxVal;
			return false;
		}
	}
	forceinline output_t operator() (input_t val)
	{
		return val * maxValInv;
	}
	forceinline peak_t GetSrcPeak() const
	{
		return maxVal;
	}
};


// Reads sample data with Func1, then normalizes the sample data, and then converts it with Func2.
// Func1::output_t and Func2::input_t must be identical.
// Func1 can also be the identity decode (DecodeIdentity<T>).
// Func2 can also be the identity conversion (Convert<T,T>).
template <typename Func2, typename Func1>
struct NormalizationChain
{
	typedef typename Func1::input_t input_t;
	typedef typename Func1::output_t normalize_t;
	typedef typename Normalize<normalize_t>::peak_t peak_t;
	typedef typename Func2::output_t output_t;
	static const int input_inc = Func1::input_inc;
	Func1 func1;
	Normalize<normalize_t> normalize;
	Func2 func2;
	forceinline void FindMax(const input_t *inBuf)
	{
		normalize.FindMax(func1(inBuf));
	}
	forceinline bool IsSilent()
	{
		return normalize.IsSilent();
	}
	forceinline output_t operator() (const input_t *inBuf)
	{
		return func2(normalize(func1(inBuf)));
	}
	forceinline peak_t GetSrcPeak() const
	{
		return normalize.GetSrcPeak();
	}
	forceinline NormalizationChain(Func2 f2 = Func2(), Func1 f1 = Func1())
		: func1(f1)
		, func2(f2)
	{
		return;
	}
};




} // namespace SC



//////////////////////////////////////////////////////
// Actual sample conversion functions

// Copy a sample data buffer.
// targetBuffer: Buffer in which the sample should be copied into.
// numSamples: Number of samples of size T that should be copied. targetBuffer is expected to be able to hold "numSamples * incTarget" samples.
// incTarget: Number of samples by which the target data pointer is increased each time.
// sourceBuffer: Buffer from which the samples should be read.
// sourceSize: Size of source buffer, in bytes.
// incSource: Number of samples by which the source data pointer is increased each time.
//
// Template arguments:
// SampleConversion: Functor of type SampleConversionFunctor to apply sample conversion (see above for existing functors).
template <typename SampleConversion>
size_t CopySample(typename SampleConversion::output_t * MPT_RESTRICT outBuf, size_t numSamples, size_t incTarget, const typename SampleConversion::input_t * MPT_RESTRICT inBuf, size_t sourceSize, size_t incSource, SampleConversion conv = SampleConversion())
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	const size_t sampleSize = incSource * SampleConversion::input_inc * sizeof(typename SampleConversion::input_t);
	LimitMax(numSamples, sourceSize / sampleSize);
	const size_t copySize = numSamples * sampleSize;

	SampleConversion sampleConv(conv);
	while(numSamples--)
	{
		*outBuf = sampleConv(inBuf);
		outBuf += incTarget;
		inBuf += incSource * SampleConversion::input_inc;
	}

	return copySize;
}


// Copy numChannels interleaved sample streams.
template <typename SampleConversion>
void CopyInterleavedSampleStreams(typename SampleConversion::output_t * MPT_RESTRICT outBuf, const typename SampleConversion::input_t * MPT_RESTRICT inBuf, size_t numFrames, size_t numChannels, SampleConversion *conv)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	while(numFrames--)
	{
		for(size_t channel = 0; channel < numChannels; ++channel)
		{
			*outBuf = conv[channel](*inBuf);
			inBuf++;
			outBuf++;
		}
	}
}


// Copy numChannels interleaved sample streams.
template <typename SampleConversion>
void CopyInterleavedSampleStreams(typename SampleConversion::output_t * MPT_RESTRICT outBuf, const typename SampleConversion::input_t * MPT_RESTRICT inBuf, size_t numFrames, size_t numChannels, std::vector<SampleConversion> &conv)
//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	MPT_ASSERT(conv.size() >= numChannels);
	CopyInterleavedSampleStreams(outBuf, inBuf, numFrames, numChannels, &(conv[0]));
}


// Copy a mono sample data buffer.
template <typename SampleConversion>
size_t CopyMonoSample(ModSample &sample, const char *sourceBuffer, size_t sourceSize, SampleConversion conv = SampleConversion())
//-------------------------------------------------------------------------------------------------------------------------------
{
	MPT_ASSERT(sample.GetNumChannels() == 1);
	MPT_ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	const size_t frameSize =  SampleConversion::input_inc;
	const size_t countFrames = std::min<size_t>(sourceSize / frameSize, sample.nLength);
	size_t numFrames = countFrames;
	SampleConversion sampleConv(conv);
	const char * MPT_RESTRICT inBuf = sourceBuffer;
	typename SampleConversion::output_t * MPT_RESTRICT outBuf = static_cast<typename SampleConversion::output_t *>(sample.pSample);
	while(numFrames--)
	{
		*outBuf = sampleConv(inBuf);
		inBuf += SampleConversion::input_inc;
		outBuf++;
	}
	return frameSize * countFrames;
}


// Copy a stereo interleaved sample data buffer.
template <typename SampleConversion>
size_t CopyStereoInterleavedSample(ModSample &sample, const char *sourceBuffer, size_t sourceSize, SampleConversion conv = SampleConversion())
//--------------------------------------------------------------------------------------------------------------------------------------------
{
	MPT_ASSERT(sample.GetNumChannels() == 2);
	MPT_ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	const size_t frameSize = 2 * SampleConversion::input_inc;
	const size_t countFrames = std::min<size_t>(sourceSize / frameSize, sample.nLength);
	size_t numFrames = countFrames;
	SampleConversion sampleConvLeft(conv);
	SampleConversion sampleConvRight(conv);
	const char * MPT_RESTRICT inBuf = sourceBuffer;
	typename SampleConversion::output_t * MPT_RESTRICT outBuf = static_cast<typename SampleConversion::output_t *>(sample.pSample);
	while(numFrames--)
	{
		*outBuf = sampleConvLeft(inBuf);
		inBuf += SampleConversion::input_inc;
		outBuf++;
		*outBuf = sampleConvRight(inBuf);
		inBuf += SampleConversion::input_inc;
		outBuf++;
	}
	return frameSize * countFrames;
}


// Copy a stereo split sample data buffer.
template <typename SampleConversion>
size_t CopyStereoSplitSample(ModSample &sample, const char *sourceBuffer, size_t sourceSize, SampleConversion conv = SampleConversion())
//--------------------------------------------------------------------------------------------------------------------------------------
{
	MPT_ASSERT(sample.GetNumChannels() == 2);
	MPT_ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	const size_t sampleSize = SampleConversion::input_inc;
	const size_t sourceSizeLeft = std::min<size_t>(sample.nLength * SampleConversion::input_inc, sourceSize);
	const size_t sourceSizeRight = std::min<size_t>(sample.nLength * SampleConversion::input_inc, sourceSize - sourceSizeLeft);
	const size_t countSamplesLeft = sourceSizeLeft / sampleSize;
	const size_t countSamplesRight = sourceSizeRight / sampleSize;

	size_t numSamplesLeft = countSamplesLeft;
	SampleConversion sampleConvLeft(conv);
	const char * MPT_RESTRICT inBufLeft = sourceBuffer;
	typename SampleConversion::output_t * MPT_RESTRICT outBufLeft = static_cast<typename SampleConversion::output_t *>(sample.pSample);
	while(numSamplesLeft--)
	{
		*outBufLeft = sampleConvLeft(inBufLeft);
		inBufLeft += SampleConversion::input_inc;
		outBufLeft += 2;
	}

	size_t numSamplesRight = countSamplesRight;
	SampleConversion sampleConvRight(conv);
	const char * MPT_RESTRICT inBufRight = sourceBuffer + sample.nLength * SampleConversion::input_inc;
	typename SampleConversion::output_t * MPT_RESTRICT outBufRight = static_cast<typename SampleConversion::output_t *>(sample.pSample) + 1;
	while(numSamplesRight--)
	{
		*outBufRight = sampleConvRight(inBufRight);
		inBufRight += SampleConversion::input_inc;
		outBufRight += 2;
	}

	return (countSamplesLeft + countSamplesRight) * sampleSize;
}


// Copy a sample data buffer and normalize it. Requires slightly advanced sample conversion functor.
template<typename SampleConversion>
size_t CopyAndNormalizeSample(ModSample &sample, const char *sourceBuffer, size_t sourceSize, typename SampleConversion::peak_t *srcPeak = nullptr, SampleConversion conv = SampleConversion())
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	const size_t inSize = sizeof(typename SampleConversion::input_t);

	MPT_ASSERT(sample.GetElementarySampleSize() == sizeof(typename SampleConversion::output_t));

	size_t numSamples = sample.nLength * sample.GetNumChannels();
	LimitMax(numSamples, sourceSize / inSize);

	const char * inBuf = sourceBuffer;
	// Finding max value
	SampleConversion sampleConv(conv);
	for(size_t i = numSamples; i != 0; i--)
	{
		sampleConv.FindMax(inBuf);
		inBuf += SampleConversion::input_inc;
	}

	// If buffer is silent (maximum is 0), don't bother normalizing the sample - just keep the already silent buffer.
	if(!sampleConv.IsSilent())
	{
		const char * inBuf = sourceBuffer;
		// Copying buffer.
		typename SampleConversion::output_t *outBuf = static_cast<typename SampleConversion::output_t *>(sample.pSample);

		for(size_t i = numSamples; i != 0; i--)
		{
			*outBuf = sampleConv(inBuf);
			outBuf++;
			inBuf += SampleConversion::input_inc;
		}
	}

	if(srcPeak)
	{
		*srcPeak = sampleConv.GetSrcPeak();
	}

	return numSamples * inSize;
}


template<int fractionalBits, bool clipOutput, typename Tsample, typename Tfixed>
void ConvertInterleavedFixedPointToInterleaved(Tsample * MPT_RESTRICT p, const Tfixed * MPT_RESTRICT mixbuffer, std::size_t channels, std::size_t count)
//------------------------------------------------------------------------------------------------------------------------------------------------------
{
	SC::ConvertFixedPoint<Tsample, int, fractionalBits, clipOutput> conv;
	count *= channels;
	for(std::size_t i = 0; i < count; ++i)
	{
		p[i] = conv(mixbuffer[i]);
	}
}

template<int fractionalBits, bool clipOutput, typename Tsample, typename Tfixed>
void ConvertInterleavedFixedPointToNonInterleaved(Tsample * const * const MPT_RESTRICT buffers, const Tfixed * MPT_RESTRICT mixbuffer, std::size_t channels, std::size_t count)
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	SC::ConvertFixedPoint<Tsample, int, fractionalBits, clipOutput> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			buffers[channel][i] = conv(*mixbuffer);
			mixbuffer++;
		}
	}
}


// Copy from an interleaed buffer of #channels.
template <typename SampleConversion>
void CopyInterleavedToChannel(typename SampleConversion::output_t * MPT_RESTRICT dst, const typename SampleConversion::input_t * MPT_RESTRICT src, std::size_t channels, std::size_t countChunk, std::size_t channel, SampleConversion conv = SampleConversion())
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
{
	SampleConversion sampleConv(conv);
	src += channel;
	for(std::size_t i = 0; i < countChunk; ++i)
	{
		*dst = sampleConv(*src);
		src += channels;
		dst++;
	}
}


OPENMPT_NAMESPACE_END
