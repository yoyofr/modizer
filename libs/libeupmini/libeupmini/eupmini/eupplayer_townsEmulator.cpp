// $Id: eupplayer_townsEmulator.cc,v 1.24 2000/04/12 23:14:35 hayasaka Exp $

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <cstring>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
static int m_voice_ofs;
extern int eup_mutemask;
//TODO:  MODIZER changes end / YOYOFR


// note: in EMSCRIPTEN context __GNUC__ is defined!

#include "eupplayer_townsEmulator.hpp"
#include "sintbl.hpp"


//#define USE_ORIG_OPL2_IMPL

#ifdef USE_ORIG_OPL2_IMPL
#include "TownsFmEmulator.hpp"
#else
#include "TownsFmEmulator2.hpp"
#endif



#if EUPPLAYER_LITTLE_ENDIAN
static inline uint16_t P2(u_char const * const p)
{
    return *(uint16_t const *)p;
}
static inline uint32_t P4(u_char const * const p)
{
    return *(uint32_t const *)p;
}
#else
static inline uint16_t P2(u_char const * const p)
{
    uint16_t const x0 = *p;
    uint16_t const x1 = *(p + 1);
    return x0 + (x1 << 8);
}
static inline uint32_t P4(u_char const * const p)
{
    uint32_t const x0 = P2(p);
    uint32_t const x1 = P2(p + 2);
    return x0 + (x1 << 16);
}
#endif

#ifdef DEBUG
#include <stdarg.h>
static void dbprintf(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}
#define DB(a) //dbprintf##a
#else
#define DB(a)
#endif

#define CHECK_CHANNEL_NUM(funcname, channel) \
  if (channel < 0 || _maxChannelNum <= channel) { \
    fprintf(stderr, "%s: channel number %d out of range\n", funcname, channel); \
    fflush(stderr); \
    return; \
  } \
  (void)false

/* TownsPcmSound */

class TownsPcmSound {
    char _name[9];
    int _id;
    int _numSamples;
    int _loopStart;
    int _loopLength;
    int _samplingRate;
    int _keyOffset;
	int _adjustedSamplingRate;
    int _keyNote;
    signed char *_samples;
public:
    TownsPcmSound(u_char const *p);
    ~TownsPcmSound();
    int id() const						{ return _id; }
    int numSamples() const				{ return _numSamples; }
    int loopStart() const				{ return _loopStart; }
    int64_t loopLength() const			{ return _loopLength; }
    int samplingRate() const			{ return _samplingRate; }
    int keyOffset() const				{ return _keyOffset; }
    int adjustedSamplingRate() const	{ return _adjustedSamplingRate; }
    int keyNote() const					{ return _keyNote; }
    signed char const * samples() const	{ return _samples; }
};

TownsPcmSound::TownsPcmSound(u_char const *p)
{
    {
        u_int n = 0;
        for (; n < sizeof(_name)-1; n++) {
            _name[n] = p[n];
        }
        _name[n] = '\0';
    }
    _id = P4(p+8);
    _numSamples = P4(p+12);
    _loopStart = P4(p+16);
    _loopLength = P4(p+20);
	_samplingRate = P2(p+24);
    _keyOffset = (int16_t)P2(p+26);
	 _adjustedSamplingRate = (_samplingRate + _keyOffset) * (1000*0x10000/0x62);
    _keyNote = *(u_char*)(p+28);
/*
  if (_loopLength == 0) {
    fprintf(stderr, "TownsPcmSound::TownsPcmSound: loopStart=0x%08x, loopLength=0x%08llx, numSamples=0x%08x\n", _loopStart, _loopLength, _numSamples);
  } else {
    uint32_t diff = _numSamples - (_loopStart + _loopLength);
    fprintf(stderr, "TownsPcmSound::TownsPcmSound: loopStart=0x%08x, loopLength=0x%08llx, numSamples=0x%08x, diff=%08x\n", _loopStart, _loopLength, _numSamples, diff);
  }
  */
	_samples = new signed char[_numSamples + 1]; // append 1 sample, in order to avoid buffer overflow in liner interpolation process
	for (int i = 0; i < _numSamples; i++) {
        int n = p[32+i];
        _samples[i] = (n >= 0x80) ? (n & 0x7f) : (-n);
    }
	_samples[_numSamples] = 0;		// only for non-looping case where signal ends anyway

	if (_loopStart >= _numSamples) {
		fprintf(stderr, "TownsPcmSound::TownsPcmSound: too large loopStart.  loopStart zeroed.  loopStart=0x%08x, numSamples=0x%08x\n", _loopStart, _numSamples);
		_loopStart = 0;
	}
	if (_loopLength > _numSamples - _loopStart) {
		fprintf(stderr, "TownsPcmSound::TownsPcmSound: too large loopLength.  loop disabled.  loopStart=0x%08x, loopLength=0x%08x, numSamples=0x%08x\n", _loopStart, _loopLength, _numSamples);
		_loopLength = 0;
	}
	if (_loopLength != 0 && _loopStart + _loopLength < _numSamples) {
		_numSamples = _loopStart + _loopLength;
	}
}

TownsPcmSound::~TownsPcmSound()
{
    if (_samples != NULL) {
        delete _samples;
    }
}

/* TownsPcmEnvelope */

class TownsPcmEnvelope {
    enum State { _s_ready=0, _s_attacking, _s_decaying, _s_sustaining, _s_releasing };
    State _state;
    State _oldState;
    int _currentLevel;
    int _rate;
    int _tickCount;
    int _totalLevel;
    int _attackRate;
    int _decayRate;
    int _sustainLevel;
    int _sustainRate;
    int _releaseLevel;
    int _releaseRate;
    int _rootKeyOffset;
public:
    TownsPcmEnvelope(TownsPcmEnvelope const *e);
    TownsPcmEnvelope(u_char const *p);
    ~TownsPcmEnvelope();
    void start(int rate);
    void release();
    int nextTick();
    int state()			{ return (int)(_state); }
    int rootKeyOffset()	{ return _rootKeyOffset; }
};

TownsPcmEnvelope::TownsPcmEnvelope(TownsPcmEnvelope const *e)
{
    memcpy(this, e, sizeof(*this));
}

TownsPcmEnvelope::TownsPcmEnvelope(u_char const *p)
{
    _state = _s_ready;
    _oldState = _s_ready;
    _currentLevel = 0;
    _totalLevel = *(u_char*)(p+0);
    _attackRate = *(u_char*)(p+1) * 10;
    _decayRate = *(u_char*)(p+2) * 10;
    _sustainLevel = *(u_char*)(p+3);
    _sustainRate = *(u_char*)(p+4) * 20;
    _releaseRate = *(u_char*)(p+5) * 10;
    _rootKeyOffset = *(char*)(p+6);
    //cerr << this->describe() << '\n';
}

TownsPcmEnvelope::~TownsPcmEnvelope()
{
}

void TownsPcmEnvelope::start(int rate)
{
    _state = _s_attacking;
    _currentLevel = 0;
    _rate = rate;
    _tickCount = 0;
}

void TownsPcmEnvelope::release()
{
    if (_state == _s_ready) {
        return;
    }
    _state = _s_releasing;
    _releaseLevel = _currentLevel;
    _tickCount = 0;
}

int TownsPcmEnvelope::nextTick()
{
    if (_oldState != _state) {
        switch (_oldState) {
        default:
            break;
        };
        _oldState = _state;
    }
    switch (_state) {
    case _s_ready:
        return 0;
        break;
    case _s_attacking:
        if (_attackRate == 0) {
            _currentLevel = _totalLevel;
        }
        else if (_attackRate >= 1270) {
            _currentLevel = 0;
        }
        else {
            _currentLevel = (_totalLevel*(_tickCount++)*(int64_t)1000) / (_attackRate*_rate);
        }
        if (_currentLevel >= _totalLevel) {
            _currentLevel = _totalLevel;
            _state = _s_decaying;
            _tickCount = 0;
        }
        break;
    case _s_decaying:
        if (_decayRate == 0) {
            _currentLevel = _sustainLevel;
        }
        else if (_decayRate >= 1270) {
            _currentLevel = _totalLevel;
        }
        else {
            _currentLevel = _totalLevel;
            _currentLevel -= ((_totalLevel-_sustainLevel)*(_tickCount++)*(int64_t)1000) / (_decayRate*_rate);
        }
        if (_currentLevel <= _sustainLevel) {
            _currentLevel = _sustainLevel;
            _state = _s_sustaining;
            _tickCount = 0;
        }
        break;
    case _s_sustaining:
        if (_sustainRate == 0) {
            _currentLevel = 0;
        }
        else if (_sustainRate >= 2540) {
            _currentLevel = _sustainLevel;
        }
        else {
            _currentLevel = _sustainLevel;
            _currentLevel -= (_sustainLevel*(_tickCount++)*(int64_t)1000) / (_sustainRate*_rate);
        }
        if (_currentLevel <= 0) {
            _currentLevel = 0;
            _state = _s_ready;
            _tickCount = 0;
        }
        break;
    case _s_releasing:
        if (_releaseRate == 0) {
            _currentLevel = 0;
        }
        else if (_releaseRate >= 1270) {
            _currentLevel = _releaseLevel;
        }
        else {
            _currentLevel = _releaseLevel;
            _currentLevel -= (_releaseLevel*(_tickCount++)*(int64_t)1000) / (_releaseRate*_rate);
        }
        if (_currentLevel <= 0) {
            _currentLevel = 0;
            _state = _s_ready;
        }
        break;
    default:
        // you shouldn't come here
        break;
    };

    return _currentLevel;
}

/* TownsPcmInstrument */

class TownsPcmInstrument {
    enum { _maxSplitNum = 8, };
    char _name[9];
    int _split[_maxSplitNum];
    int _soundId[_maxSplitNum];
    TownsPcmSound const *_sound[_maxSplitNum];
    TownsPcmEnvelope *_envelope[_maxSplitNum];
public:
    TownsPcmInstrument(u_char const *p);
    TownsPcmInstrument();
    void registerSound(TownsPcmSound const *sound);
    TownsPcmSound const *findSound(int note) const;
    TownsPcmEnvelope const *findEnvelope(int note) const;
};

TownsPcmInstrument::TownsPcmInstrument(u_char const *p)
{
    {
        u_int n = 0;
        for (; n < sizeof(_name)-1; n++) {
            _name[n] = p[n];
        }
        _name[n] = '\0';
    }
    for (int n = 0; n < _maxSplitNum; n++) {
        _split[n] = P2(p+16+2*n);
        _soundId[n] = P4(p+32+4*n);
        _sound[n] = NULL;
        _envelope[n] = new TownsPcmEnvelope(p+64+8*n);
    }
    //cerr << this->describe() << '\n';
}

TownsPcmInstrument::TownsPcmInstrument()
{
}

void TownsPcmInstrument::registerSound(TownsPcmSound const *sound)
{
    for (int i = 0; i < _maxSplitNum; i++) {
        if (_soundId[i] == sound->id()) {
            _sound[i] = sound;
        }
	}
}

TownsPcmSound const *TownsPcmInstrument::findSound(int note) const
{
    // At least one split should be selectable
    int splitNum;
    for (splitNum = 0; splitNum < _maxSplitNum-1; splitNum++) {	// i.e. last entry is always used as last resort
        if (note <= _split[splitNum]) {
            break;
        }
	}
    return _sound[splitNum];
}

TownsPcmEnvelope const *TownsPcmInstrument::findEnvelope(int note) const
{
    // At least one split should be selectable
    int splitNum;
    for (splitNum = 0; splitNum < _maxSplitNum-1; splitNum++) {	// i.e. last entry is always used as last resort
        if (note <= _split[splitNum]) {
            break;
        }
	}
    return _envelope[splitNum];
}



/* TownsPcmEmulator */

TownsPcmEmulator::TownsPcmEmulator()
{
    _control7 =  100; // power-on reset value (100) is defined in midi spec
	_expression = 127;
    this->velocity(0);
    _gateTime = 0;
    _frequencyOffs = 0x2000;
    _currentInstrument = NULL;
    _currentEnvelope = NULL;
    _currentSound = NULL;

	_volL = _volR = 0x10;	// correctly only 0xf - but lets keep it shift friendly
}

TownsPcmEmulator::~TownsPcmEmulator()
{
}

void TownsPcmEmulator::setControlParameter(int control, int value)
{
    switch (control) {
	 case 0: 					// Bank Select (for devices with more than 128 programs)
		// base for "program change" commands
		if (value > 0)
			fprintf(stderr, "warning: unsupported Bank Select: %d\n", value);

		break;

	case 1: 					// Modulation controls a vibrato effect (pitch, loudness, brighness)
		if (value >0)
			fprintf(stderr, "warning: use of unimplemented PCM Modulation: %d\n", value);
		break;

	case 7:
        _control7 = value;
        break;

	case 10:
        // panpot - rf5c68 seems to have an 8-bit pan register where low-nibble controls output to left
		// speaker and high-nibble the right..

		value -= 0x40;
		_volL = 0x10 - (0x10*value/0x40);
		_volR = 0x10 + (0x10*value/0x40);
        break;

	case 11:				// MIDI: Expression  - testcase; Passionate Ocean
		// Expression is a "percentage" of volume (CC7).
		_expression= value;
		if(value != 127)
			fprintf(stderr, "warning: song uses unimplemented Expression control\n");
        break;

	case 64: 					// Sustain Pedal (on/off) testcase: Bach: Aria on G String"
		if (value >0)
			fprintf(stderr, "warning: use of unimplemented PCM Sustain Pedal: %d\n", value);
		break;
    default:
        fprintf(stderr, "TownsFmEmulator::setControlParameter: unknown control %d, val=%d\n", control, value);
        fflush(stderr);
        break;
    };
}

void TownsPcmEmulator::setInstrumentParameter(u_char const *fmInst,
        u_char const *pcmInst)
{
    u_char const *instrument = pcmInst;
    if (instrument == NULL) {
        fprintf(stderr, "%s@%p: can not set null instrument\n",
                "TownsPcmEmulator::setInstrumentParameter", this);
        fflush(stderr);
        return;
    }
    _currentInstrument = (TownsPcmInstrument*)instrument;
    //fprintf(stderr, "0x%08x: program change (to 0x%08x)\n", this, instrument);
    //fflush(stderr);
}

void TownsPcmEmulator::nextTick(int *outbuf, int buflen)
{
    // steptime advance one step

    if (_currentEnvelope == NULL) {
        return;
    }
   if (_gateTime > 0 && --_gateTime <= 0) {
        this->velocity(_offVelocity);
        _currentEnvelope->release();
    }
    if (_currentEnvelope->state() == 0) {
        this->velocity(0);
    }
    if (this->velocity() == 0) {
        delete _currentEnvelope;
        _currentEnvelope = NULL;
        return;
    }

    uint32_t  phaseStep;
    {
        int64_t ps = frequencyTable[_note];

		// replaced original impl with more transparent logic
 //       ps *= powtbl[_frequencyOffs>>4];

		ps *= 0x10000;							// neutral pos from old table impl
		double bend = ((double)_frequencyOffs - 0x2000) / 0x2000;	// -1..1
		ps = pow(1.0594789723915, 2*bend) * ps;		// factor (halftone step) tuned to above table


        ps /= frequencyTable[_currentSound->keyNote() - _currentEnvelope->rootKeyOffset()];

        ps *= _currentSound->adjustedSamplingRate();

        ps /= this->rate();		// output sample rate, e.g. 44100

        ps >>= 16;
        phaseStep = ps;
    }

	// testcase: COMIC9.eup long initial sample has a numSamples of 52635 (i.e. 16 bit with msb set!)
	// original "<< 16" below turned this into a negative int which immediately triggers the stop (-> changed to unsigned)

	uint32_t  loopLength = _currentSound->loopLength() << 16;
    uint32_t  numSamples = _currentSound->numSamples() << 16;

    
    //TODO:  MODIZER changes start / YOYOFR
    //Expect to be called 8 times for 8 PCM channels
    m_voice_ofs++;
    //TODO:  MODIZER changes end / YOYOFR
    
    //YOYOFR
        if ( !(eup_mutemask&(1<<m_voice_ofs)) && (_volL+_volR) && this->velocity() && _control7 && phaseStep) {
            vgm_last_note[m_voice_ofs]=440.0*(float)phaseStep/65536.0;
            vgm_last_instr[m_voice_ofs]=m_voice_ofs;
            vgm_last_vol[m_voice_ofs]=1;
        }
    //YOYOFR
    
	signed char const *soundSamples = _currentSound->samples();
	for (int i = 0, j = 0; i < buflen; i++) {
		if (loopLength > 0) {
			while (_phase >= numSamples) {
				// note: it seems to always be the end section of the sample that is looped
				// and loopStart() + loopLength() = numSamples()
				_phase -= loopLength;
			}
		}
		else if (_phase >= numSamples) {
			_gateTime = 0;
			this->velocity(0);
			delete _currentEnvelope;
			_currentEnvelope = NULL;
			break;
		}

		int output;
		{
			uint32_t phase0 = _phase;
			uint32_t phase1 = _phase + 0x10000;

			if (phase1 >= numSamples) {
				// it's safe even if loopLength == 0, because soundSamples[] is extended by 1 and filled with 0 (see TownsPcmSound::TownsPcmSound).
				phase1 -= loopLength;
			}

			phase0 >>= 16;
			phase1 >>= 16;

			int weight1 = _phase & 0xffff;
			int weight0 = 0x10000 - weight1;

			output = soundSamples[phase0] * weight0 + soundSamples[phase1] * weight1;
			output >>= 16;
		}

		output *= this->velocity(); // Believe it or not, it's different from FM.
		output <<= 1;
		output *= _currentEnvelope->nextTick();
		output >>= 7;
		output *= _control7;	// What is the correct amount of attenuation?
		output >>= 7;
		// Balance volume with FM.
		output *= 185; // How much?
		output >>= 8;

        //YOYOFR
        if (eup_mutemask&(1<<m_voice_ofs)) output=0; //mute
        
		outbuf[j++] += (_volL * output) >> 4;
		outbuf[j++] += (_volR * output) >> 4;
        
        //TODO:  MODIZER changes start / YOYOFR
        if (m_voice_ofs>=0) {
            int64_t smplIncr=(int64_t)44100*(1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT)/m_voice_current_samplerate;
            int64_t ofs_start=m_voice_current_ptr[m_voice_ofs];
            int64_t ofs_end=(m_voice_current_ptr[m_voice_ofs]+smplIncr);
            for (;;) {
                m_voice_buff[m_voice_ofs][(ofs_start>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)&(SOUND_BUFFER_SIZE_SAMPLE*2*4-1)]=LIMIT8((((_volL+_volR) * output) >> 12));
                
                ofs_start+=1<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT;
                if (ofs_start>=ofs_end) break;
            }
            while ((ofs_end>>MODIZER_OSCILLO_OFFSET_FIXEDPOINT)>=SOUND_BUFFER_SIZE_SAMPLE*2*4) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE*2*4<<MODIZER_OSCILLO_OFFSET_FIXEDPOINT);
            m_voice_current_ptr[m_voice_ofs]=ofs_end;
        }
        //TODO:  MODIZER changes end / YOYOFR

		_phase += phaseStep;
	}
}

void TownsPcmEmulator::note(int n, int onVelo, int offVelo, int gateTime)
{
//	fprintf(stderr, "inst: #%d  note: %d \n", _currentSound->id(),  n);

    _note = n;

    this->velocity(onVelo);
    _offVelocity = offVelo;
    _gateTime = gateTime;
    _phase = 0;

    if (_currentInstrument != NULL) {
        _currentSound = _currentInstrument->findSound(n);
        _currentEnvelope = new TownsPcmEnvelope(_currentInstrument->findEnvelope(n));
        _currentEnvelope->start(this->rate());
    }
    else {
        _currentSound = NULL;
    }
}

void TownsPcmEmulator::pitchBend(int value)
{
    _frequencyOffs = value;
}

/* EUP_TownsEmulator_Channel */

EUP_TownsEmulator_Channel::EUP_TownsEmulator_Channel()
{
    _dev[0] = NULL;
    _lastNotedDeviceNum = 0;
}

EUP_TownsEmulator_Channel::~EUP_TownsEmulator_Channel()
{
    for (int n = 0; _dev[n] != NULL; n++) {
        delete _dev[n];
    }
}

void EUP_TownsEmulator_Channel::add(EUP_TownsEmulator_MonophonicAudioSynthesizer *device)
{
    for (int n = 0; n < _maxDevices; n++)
        if (_dev[n] == NULL) {
            _dev[n] = device;
            _dev[n+1] = NULL;
            break;
        }
}

void EUP_TownsEmulator_Channel::note(int note, int onVelo, int offVelo, int gateTime)
{
    int n = _lastNotedDeviceNum;
    if (_dev[n] == NULL || _dev[n+1] == NULL) {
        n = 0;
    }
    else {
        n++;
    }

    if (_dev[n] != NULL) {
        _dev[n]->note(note, onVelo, offVelo, gateTime);
    }

    _lastNotedDeviceNum = n;
    //fprintf(stderr, "ch=%08x, dev=%d, note=%d, on=%d, off=%d, gate=%d\n",
    // this, n, note, onVelo, offVelo, gateTime);
    //fflush(stderr);
}

void EUP_TownsEmulator_Channel::setControlParameter(int control, int value)
{
    // Is this okay?
    for (int n = 0; _dev[n] != NULL; n++) {
        _dev[n]->setControlParameter(control, value);
    }
}

void EUP_TownsEmulator_Channel::setInstrumentParameter(u_char const *fmInst,
        u_char const *pcmInst)
{
    for (int n = 0; _dev[n] != NULL; n++) {
        _dev[n]->setInstrumentParameter(fmInst, pcmInst);
    }
}

void EUP_TownsEmulator_Channel::pitchBend(int value)
{
    // Is this okay?
    for (int n = 0; _dev[n] != NULL; n++) {
        _dev[n]->pitchBend(value);
    }
}

void EUP_TownsEmulator_Channel::nextTick(int *outbuf, int buflen)
{
    for (int n = 0; _dev[n] != NULL; n++) {
        _dev[n]->nextTick(outbuf, buflen);
    }
}

void EUP_TownsEmulator_Channel::rate(int r)
{
    for (int n = 0; _dev[n] != NULL; n++) {
        _dev[n]->rate(r);
    }
}

/* EUP_TownsEmulator */

EUP_TownsEmulator::EUP_TownsEmulator()
{
	_mixBufLen = 0;
	_mixBuf = NULL;

    for (int n = 0; n < _maxChannelNum; n++) {
        _channel[n] = new EUP_TownsEmulator_Channel;
        _enabled[n] = true;
    }

    this->rate(8000);									// 8 MHz (per milli)

	memset(&_fmInstrumentData[0], 0, sizeof(_fmInstrumentData));

    for (int n = 0; n < _maxFmInstrumentNum; n++) {
        _fmInstrument[n] = _fmInstrumentData + 8 + 48*n;
    }
    for (int n = 0; n < _maxPcmInstrumentNum; n++) {
        _pcmInstrument[n] = NULL;
    }
    for (int n = 0; n < _maxPcmSoundNum; n++) {
        _pcmSound[n] = NULL;
    }
}

EUP_TownsEmulator::~EUP_TownsEmulator()
{
    for (int n = 0; n < _maxChannelNum; n++) {
        delete _channel[n];
    }
    for (int n = 0; n < _maxPcmInstrumentNum; n++)
        if (_pcmInstrument[n] != NULL) {
            delete _pcmInstrument[n];
        }
    for (int n = 0; n < _maxPcmSoundNum; n++)
        if (_pcmSound[n] != NULL) {
            delete _pcmSound[n];
        }
}

void EUP_TownsEmulator::assignFmDeviceToChannel(int eupChannel, int fmChannel)
{
    CHECK_CHANNEL_NUM("EUP_TownsEmulator::assignFmDeviceToChannel", eupChannel);

#ifdef USE_ORIG_OPL2_IMPL
    EUP_TownsEmulator_MonophonicAudioSynthesizer *dev = new TownsFmEmulator();			// old homegrown emulation
#else
    EUP_TownsEmulator_MonophonicAudioSynthesizer *dev = new TownsFmEmulator2(fmChannel);	// "state of the art" OPN2 emulation
#endif
    dev->rate(_rate);
    _channel[eupChannel]->add(dev);
}

void EUP_TownsEmulator::assignPcmDeviceToChannel(int channel)
{
	// testcase: "Miracle Magic Kazuma" uses out of range channel 16 (while leaving
	// available 13-15 unused..) - doesn't seem to make any difference when mapping it
	// to another channel..

    CHECK_CHANNEL_NUM("EUP_TownsEmulator::assignPcmDeviceToChannel", channel);

	if (channel >= _maxChannelNum) return;	// original impl created out of bounds write to _channel[]

    EUP_TownsEmulator_MonophonicAudioSynthesizer *dev = new TownsPcmEmulator;
    dev->rate(_rate);
    _channel[channel]->add(dev);
}

void EUP_TownsEmulator::setFmInstrumentParameter(int num, u_char const *instrument)
{
    if (num < 0 || num >= _maxFmInstrumentNum) {
        fprintf(stderr, "%s: FM instrument number %d out of range\n",
                "EUP_TownsEmulator::setFmInstrumentParameter",  num);
        fflush(stderr);
        return;
    }
    memcpy(_fmInstrument[num], instrument, 48);
}

void EUP_TownsEmulator::setPcmInstrumentParameters(u_char const *instrument, size_t size)
{
    for (int n = 0; n < _maxPcmInstrumentNum; n++) {
        if (_pcmInstrument[n] != NULL) {
            delete _pcmInstrument[n];
        }
        _pcmInstrument[n] = new TownsPcmInstrument(instrument+8+128*n);
    }
    u_char const *p = instrument + 8 + 128*32;
    for (int m = 0; m < _maxPcmSoundNum && p<(instrument+size); m++) {
        if (_pcmSound[m] != NULL) {
            delete _pcmSound[m];
        }
        _pcmSound[m] = new TownsPcmSound(p);
        for (int n = 0; n < _maxPcmInstrumentNum; n++) {
            _pcmInstrument[n]->registerSound(_pcmSound[m]);
        }
        p += 32 + P4(p+12);
    }
}

void EUP_TownsEmulator::enable(int ch, bool en)
{
    DB(("enable ch=%d, en=%d\n", ch, en));
    CHECK_CHANNEL_NUM("EUP_TownsEmulator::enable", ch);
    _enabled[ch] = en;
}

int *EUP_TownsEmulator::getMixBuffer(int ticks)
{
	if (ticks > _mixBufLen) {
		if (_mixBuf) free(_mixBuf);
		_mixBuf= (int*)malloc(sizeof(int)*streamAudioChannelsNum*ticks);
	}
	return _mixBuf;
}

inline int16_t clip(int in) {
	// testcase: YUKINO.eup
	return (in > 0x7fff) ? 0x7fff : (in < -0x8000) ? -0x8000 : in;
}

void EUP_TownsEmulator::nextTick(bool nosound)
{
    // steptime advance one step

    struct timeval tv = this->timeStep();
    int64_t buflen = (int64_t)_rate * (int64_t)tv.tv_usec; /* I need to improve my accuracy  */
    //buflen /= 1000 * 1000;
    buflen /= 1000 * 1012; // Around 1010 to 1015, at home. Depends on the song.
    buflen++;

    int *buf0 = getMixBuffer(buflen);
    memset(buf0, 0, sizeof(int) * buflen * streamAudioChannelsNum);

    //TODO:  MODIZER changes start / YOYOFR
    //search first voice linked to current chip
    m_voice_ofs=6-1;
    if (!m_voice_current_samplerate) {
        m_voice_current_samplerate=44100;
        //printf("voice sample rate null\n");
    }
    //TODO:  MODIZER changes end / YOYOFR


	// reminder: "player channels" here are wrappers, each of which
	// has its own list of 6 FM and 8 PCM emulators!
    if (!nosound) {
        for (int i = 0; i < _maxChannelNum; i++) {
            if (_enabled[i]) {
                _channel[i]->nextTick(buf0, buflen);
            }
        }
    }

    //eup_pcm.write_pos = 0;

	int renderData = (eup_pcm.write_pos);
	int16_t * renderBuffer =(int16_t *) eup_pcm.buffer;
    
    if (nosound) {
        renderData+=buflen*2;
        if (renderData>=streamAudioBufferSamples) renderData-=streamAudioBufferSamples;
    } else {
        for (int i = 0, j = 0; i < buflen; i++) {
            // "volume" is "always" 256! (unused feature to change output volume)
            int dL = buf0[j++] * this->volume();	// fixme: get rid of useless volume() - or also use it to scale fmSample below..
            int dR = buf0[j++] * this->volume();
            
#ifdef USE_ORIG_OPL2_IMPL
            renderBuffer[renderData++] = clip((int16_t)(dL >> 10));
            renderBuffer[renderData++] = clip((int16_t)(dR >> 10));
#else
            int* fmSample= TownsFmEmulator2::getSample();
            renderBuffer[renderData++] = clip((fmSample[0]) + (dL >> 10));
            renderBuffer[renderData++] = clip((fmSample[1]) + (dR >> 10));
            
            if (renderData>=streamAudioBufferSamples) renderData=0;
#endif
        }
    }
    eup_pcm.write_pos = renderData;
}

void EUP_TownsEmulator::note(int channel, int n,
                             int onVelo, int offVelo, int gateTime)
{
    CHECK_CHANNEL_NUM("EUP_TownsEmulator::note", channel);
	_channel[channel]->note(n, onVelo, offVelo, gateTime);
}

void EUP_TownsEmulator::pitchBend(int channel, int value)
{
    CHECK_CHANNEL_NUM("EUP_TownsEmulator::pitchBend", channel);
    _channel[channel]->pitchBend(value);
}

void EUP_TownsEmulator::controlChange(int channel, int control, int value)
{
    CHECK_CHANNEL_NUM("EUP_TownsEmulator::controlChange", channel);
    _channel[channel]->setControlParameter(control, value);
}

void EUP_TownsEmulator::programChange(int channel, int num)
{
    CHECK_CHANNEL_NUM("EUP_TownsEmulator::programChange", channel);

    u_char *fminst = NULL;
    u_char *pcminst = NULL;

    if (0 <= num && num < _maxFmInstrumentNum) {
        fminst = _fmInstrument[num];
    }

    if (0 <= num && num < _maxPcmInstrumentNum) {
        pcminst = (u_char*)_pcmInstrument[num];
    }

   _channel[channel]->setInstrumentParameter(fminst, pcminst);
}

void EUP_TownsEmulator::rate(int r)
{
    _rate = r;
    for (int n = 0; n < _maxChannelNum; n++) {
        _channel[n]->rate(r);
    }
}
