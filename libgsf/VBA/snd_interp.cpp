#include <math.h>

#include "libresample.h"

#include "GBA.h"
#include "Globals.h"
#include "Sound.h"
#include "snd_interp.h"

#define t_double float

// this was once borrowed from libmodplug, and was also used to generate the FIR coefficient
// tables that ZSNES uses for its "FIR" interpolation mode

/* 
  ------------------------------------------------------------------------------------------------
   fir interpolation doc,
	(derived from "an engineer's guide to fir digital filters", n.j. loy)

	calculate coefficients for ideal lowpass filter (with cutoff = fc in 0..1 (mapped to 0..nyquist))
	  c[-N..N] = (i==0) ? fc : sin(fc*pi*i)/(pi*i)

	then apply selected window to coefficients
	  c[-N..N] *= w(0..N)
	with n in 2*N and w(n) being a window function (see loy)

	then calculate gain and scale filter coefs to have unity gain.
  ------------------------------------------------------------------------------------------------
*/
// quantizer scale of window coefs

#ifndef NO_INTERPOLATION

#define WFIR_QUANTBITS		14
#define WFIR_QUANTSCALE		(1L<<WFIR_QUANTBITS)
#define WFIR_8SHIFT			(WFIR_QUANTBITS-8)
#define WFIR_16BITSHIFT		(WFIR_QUANTBITS)
// log2(number)-1 of precalculated taps range is [4..12]
#define WFIR_FRACBITS		12
#define WFIR_LUTLEN			((1L<<(WFIR_FRACBITS+1))+1)
// number of samples in window
#define WFIR_LOG2WIDTH		3
#define WFIR_WIDTH			(1L<<WFIR_LOG2WIDTH)
#define WFIR_SMPSPERWING	((WFIR_WIDTH-1)>>1)
// cutoff (1.0 == pi/2)
#define WFIR_CUTOFF			0.95f
// wfir type
#define WFIR_HANN			0
#define WFIR_HAMMING		1
#define WFIR_BLACKMANEXACT	2
#define WFIR_BLACKMAN3T61	3
#define WFIR_BLACKMAN3T67	4
#define WFIR_BLACKMAN4T92	5
#define WFIR_BLACKMAN4T74	6
#define WFIR_KAISER4T		7
#define WFIR_LANCZOS		8
#define WFIR_TYPE			WFIR_LANCZOS
// wfir help
#ifndef M_zPI
#define M_zPI			3.1415926535897932384626433832795
#endif
#define M_zEPS			1e-8
#define M_zBESSELEPS	1e-21

float fir_coef( int _PCnr, float _POfs, float _PCut, int _PWidth, int _PType ) //float _PPos, float _PFc, int _PLen )
{
	t_double	_LWidthM1		= _PWidth-1;
	t_double	_LWidthM1Half	= 0.5*_LWidthM1;
	t_double	_LPosU			= ((t_double)_PCnr - _POfs);
	t_double	_LPos			= _LPosU-_LWidthM1Half;
	t_double	_LPIdl			= 2.0*M_zPI/_LWidthM1;
	t_double	_LWc,_LSi;
	if( fabs(_LPos)<M_zEPS )
	{	_LWc	= 1.0;
	_LSi	= _PCut;
	}
	else
	{	switch( _PType )
	{	case WFIR_HANN:
	_LWc = 0.50 - 0.50 * cos(_LPIdl*_LPosU);
	break;
					case WFIR_HAMMING:
						_LWc = 0.54 - 0.46 * cos(_LPIdl*_LPosU);
						break;
					case WFIR_BLACKMANEXACT:
						_LWc = 0.42 - 0.50 * cos(_LPIdl*_LPosU) + 0.08 * cos(2.0*_LPIdl*_LPosU);
						break;
					case WFIR_BLACKMAN3T61:
						_LWc = 0.44959 - 0.49364 * cos(_LPIdl*_LPosU) + 0.05677 * cos(2.0*_LPIdl*_LPosU);
						break;
					case WFIR_BLACKMAN3T67:
						_LWc = 0.42323 - 0.49755 * cos(_LPIdl*_LPosU) + 0.07922 * cos(2.0*_LPIdl*_LPosU);
						break;
					case WFIR_BLACKMAN4T92:
						_LWc = 0.35875 - 0.48829 * cos(_LPIdl*_LPosU) + 0.14128 * cos(2.0*_LPIdl*_LPosU) - 0.01168 * cos(3.0*_LPIdl*_LPosU);
						break;
					case WFIR_BLACKMAN4T74:
						_LWc = 0.40217 - 0.49703 * cos(_LPIdl*_LPosU) + 0.09392 * cos(2.0*_LPIdl*_LPosU) - 0.00183 * cos(3.0*_LPIdl*_LPosU);
						break;
					case WFIR_KAISER4T:
						_LWc = 0.40243 - 0.49804 * cos(_LPIdl*_LPosU) + 0.09831 * cos(2.0*_LPIdl*_LPosU) - 0.00122 * cos(3.0*_LPIdl*_LPosU);
						break;
					case WFIR_LANCZOS:
						_LWc = 1 - (sin(_LPIdl*_LPosU) / (_LPIdl*_LPosU));
						break;
					default:
						_LWc = 1.0;
						break;
	}
	_LPos	 *= M_zPI;
	_LSi	 = sin(_PCut*_LPos)/_LPos;
	}
	return (float)(_LWc*_LSi);
}

signed short fir_lut[WFIR_LUTLEN*WFIR_WIDTH];

void init_fir_table()
{	int _LPcl;
	float _LPcllen	= (float)(1L<<WFIR_FRACBITS);	// number of precalculated lines for 0..1 (-1..0)
	float _LNorm	= 1.0f / (float)(2.0f * _LPcllen);
	float _LCut		= WFIR_CUTOFF;
	float _LScale	= (float)WFIR_QUANTSCALE;
	float _LGain,_LCoefs[WFIR_WIDTH];
	for( _LPcl=0;_LPcl<WFIR_LUTLEN;_LPcl++ )
	{
		float _LOfs		= ((float)_LPcl-_LPcllen)*_LNorm;
		int _LCc,_LIdx	= _LPcl<<WFIR_LOG2WIDTH;
		for( _LCc=0,_LGain=0.0f;_LCc<WFIR_WIDTH;_LCc++ )
		{	_LGain	+= (_LCoefs[_LCc] = fir_coef( _LCc, _LOfs, _LCut, WFIR_WIDTH, WFIR_TYPE ));
		}
		_LGain = 1.0f/_LGain;
		for( _LCc=0;_LCc<WFIR_WIDTH;_LCc++ )
		{	float _LCoef = (float)floor( 0.5 + _LScale*_LCoefs[_LCc]*_LGain );
			fir_lut[_LIdx+_LCc] = (signed short)( (_LCoef<-_LScale)?-_LScale:((_LCoef>_LScale)?_LScale:_LCoef) );
		}
	}
}

template <class T, unsigned buffer_size>
class sample_buffer
{
	unsigned ptr, filled;
	T * buffer;

public:
	sample_buffer() : ptr(0), filled(0), buffer(0) {}
	~sample_buffer()
	{
		if (buffer) delete [] buffer;
	}

	void clear()
	{
		if (buffer)
		{
			delete [] buffer;
			buffer = 0;
		}
		ptr = filled = 0;
	}

	inline unsigned size() const
	{
		return filled;
	}

	void push_back(T sample)
	{
		if (!buffer) buffer = new T[buffer_size];
		buffer[ptr] = sample;
		if (++ptr >= buffer_size) ptr = 0;
		if (filled < buffer_size) filled++;
	}

	void erase(unsigned count)
	{
		if (count > filled) filled = 0;
		else filled -= count;
	}

	T operator[] (int index) const
	{
		index += (int)(ptr - filled);
		if (index < 0) index += (int)buffer_size;
		else if (index >= (int)buffer_size) index -= (int)buffer_size;
		return buffer[index];
	}
};

class foo_null : public foo_interpolate
{
	int sample;

public:
	foo_null() : sample(0) {}
	~foo_null() {}

	void reset() {}

	void push(int psample)
	{
		sample = psample;
	}

	int pop(t_double rate)
	{
		return sample;
	}
};

class foo_linear : public foo_interpolate
{
	sample_buffer<int,4> samples;

	unsigned position;

	inline int smp(int index)
	{
		return samples[index];
	}

public:
	foo_linear()
	{
		position = 0;
	}

	~foo_linear() {}

	void reset()
	{
		position = 0;
		samples.clear();
	}

	void push(int sample)
	{
		samples.push_back(sample);
	}

	int pop(t_double rate)
	{
		int ret;
		unsigned lrate;

		if (position > 0x7fff)
		{
			unsigned howmany = position >> 15;
			position &= 0x7fff;
			samples.erase(howmany);
		}

		if (samples.size() < 2) return 0;

		ret  = smp(0) * (0x8000 - position);
		ret += smp(1) * position;
		ret >>= 15;

		// wahoo, takes care of drifting
		if (samples.size() > 2)
		{
			rate += (.5 / 32768.);
		}

		lrate = (unsigned)(32768. * rate);
		position += lrate;

		return ret;
	}
};

// and this integer cubic interpolation implementation was kind of borrowed from either TiMidity
// or the P.E.Op.S. SPU project, or is in use in both, or something...

class foo_cubic : public foo_interpolate
{
	sample_buffer<int,12> samples;

	unsigned position;

	inline int smp(int index)
	{
		return samples[index];
	}

public:
	foo_cubic()
	{
		position = 0;
	}

	~foo_cubic() {}

	void reset()
	{
		position = 0;
		samples.clear();
	}

	void push(int sample)
	{
		samples.push_back(sample);
	}

	int pop(t_double rate)
	{
		int ret;
		unsigned lrate;

		if (position > 0x7fff)
		{
			unsigned howmany = position >> 15;
			position &= 0x7fff;
			samples.erase(howmany);
		}

		if (samples.size() < 4) return 0;

		ret  = smp(3) - 3 * smp(2) + 3 * smp(1) - smp(0);
		ret *= ((signed)position - (2 << 15)) / 6;
		ret >>= 15;
		ret += smp(2) - 2 * smp(1) + smp(0);
		ret *= ((signed)position - (1 << 15)) >> 1;
		ret >>= 15;
		ret += smp(1) - smp(0);
		ret *= (signed)position;
		ret >>= 15;
		ret += smp(0);

		if (ret > 32767) ret = 32767;
		else if (ret < -32768) ret = -32768;

		// wahoo, takes care of drifting
		if (samples.size() > 8)
		{
			rate += (.5 / 32768.);
		}

		lrate = (unsigned)(32768. * rate);
		position += lrate;

		return ret;
	}
};

class foo_fir : public foo_interpolate
{
	sample_buffer<int,24> samples;

	unsigned position;

	inline int smp(int index)
	{
		return samples[index];
	}

public:
	foo_fir()
	{
		position = 0;
	}

	~foo_fir() {}

	void reset()
	{
		position = 0;
		samples.clear();
	}

	void push(int sample)
	{
		samples.push_back(sample);
	}

	int pop(t_double rate)
	{
		int ret;
		unsigned lrate;

		if (position > 0x7fff)
		{
			unsigned howmany = position >> 15;
			position &= 0x7fff;
			samples.erase(howmany);
		}

		if (samples.size() < 8) return 0;

		ret  = smp(0) * fir_lut[(position & ~7)    ];
		ret += smp(1) * fir_lut[(position & ~7) + 1];
		ret += smp(2) * fir_lut[(position & ~7) + 2];
		ret += smp(3) * fir_lut[(position & ~7) + 3];
		ret += smp(4) * fir_lut[(position & ~7) + 4];
		ret += smp(5) * fir_lut[(position & ~7) + 5];
		ret += smp(6) * fir_lut[(position & ~7) + 6];
		ret += smp(7) * fir_lut[(position & ~7) + 7];
		ret >>= WFIR_QUANTBITS;

		if (ret > 32767) ret = 32767;
		else if (ret < -32768) ret = -32768;

		// wahoo, takes care of drifting
		if (samples.size() > 16)
		{
			rate += (.5 / 32768.);
		}

		lrate = (unsigned)(32768. * rate);
		position += lrate;

		return ret;
	}
};

class foo_libresample : public foo_interpolate
{
	sample_buffer<float,32> samples;

	void * resampler;

public:
	foo_libresample()
	{
		resampler = 0;
	}

	~foo_libresample()
	{
		reset();
	}

	void reset()
	{
		samples.clear();
		if (resampler)
		{
			resample_close(resampler);
			resampler = 0;
		}
	}

	void push(int sample)
	{
		samples.push_back(float(sample));
	}

	int pop(t_double rate)
	{
		int ret;

		if (!resampler)
		{
			resampler = resample_open(0, .25, 44100. / 4000.);
		}

		{
			int count = samples.size();
			float in[32];
			float out;
			int used, returned;

			for (used = 0; used < count; used++)
			{
				in[used] = samples[used];
			}

			returned = resample_process(resampler, 1. / rate, in, count, 0, &used, &out, 1);

			if (used)
			{
				samples.erase(used);
			}

			if (returned < 1) return 0;

			ret = (int)out;
		}

		if (ret > 32767) ret = 32767;
		else if (ret < -32768) ret = -32768;

		return ret;
	}
};

foo_interpolate * get_filter(int which)
{
	switch (which)
	{
	default:
		return new foo_null;
	case 1:
		return new foo_linear;
	case 2:
		return new foo_cubic;
	case 3:
		return new foo_fir;
	case 4:
		return new foo_libresample;
	}
}

// and here is the implementation specific code, in a messier state than the stuff above

#endif

extern bool timer0On;
extern int timer0Reload;
extern int timer0ClockReload;
extern bool timer1On;
extern int timer1Reload;
extern int timer1ClockReload;

t_double calc_rate(int timer)
{
	if (timer ? timer1On : timer0On)
	{
		return t_double(SOUND_CLOCK_TICKS) /
			t_double((0x10000 - (timer ? timer1Reload : timer0Reload)) * 
			(timer ? timer1ClockReload : timer0ClockReload));
	}
	else
	{
		return 1.;
	}
}

#ifndef NO_INTERPOLATION

static foo_interpolate * interp[2];

static int interpolation = 0;

#endif

void interp_setup(int which)
{
#ifndef NO_INTERPOLATION
	init_fir_table();
	for (int i = 0; i < 2; i++)
	{
		interp[i] = get_filter(which);
	}
	interpolation = which;
#endif
}

void interp_cleanup()
{
#ifndef NO_INTERPOLATION
	for (int i = 0; i < 2; i++)
	{
		delete interp[i];
	}
#endif
}

void interp_switch(int which)
{
#ifndef NO_INTERPOLATION
	for (int i = 0; i < 2; i++)
	{
		delete interp[i];
		interp[i] = get_filter(which);
	}

	interpolation = which;
#endif
}

void interp_reset(int ch)
{
#ifndef NO_INTERPOLATION
	if (soundInterpolation != interpolation) interp_switch(soundInterpolation);

	if (soundInterpolation) interp[ch]->reset();
#endif
}

void interp_push(int ch, int sample)
{
#ifndef NO_INTERPOLATION
	if (soundInterpolation != interpolation) interp_switch(soundInterpolation);

	interp[ch]->push(sample);
#endif
}

int interp_pop(int ch, t_double rate)
{
#ifndef NO_INTERPOLATION
	return interp[ch]->pop(rate);
#else
	return 0;
#endif
}
