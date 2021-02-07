#include <memory>
#include "Sound.h"
#include "GBA.h"
#include "Globals.h"
#include "../common/Port.h"
#include "../apu/Gb_Apu.h"
#include "../apu/Multi_Buffer.h"
#include "../common/SoundDriver.h"
#include "XSFCommon.h"

extern SoundDriver *systemSoundInit();

static const uint32_t NR52 = 0x84;

static std::unique_ptr<SoundDriver> soundDriver;

static const int SOUND_CLOCK_TICKS_ = 167772; // 1/100 second

static uint16_t soundFinalWave[6400];
static long soundSampleRate = 44100;
bool soundInterpolation = true;
static bool soundPaused = true;
static float soundFiltering = 1.0f;
int SOUND_CLOCK_TICKS = SOUND_CLOCK_TICKS_;
int soundTicks = SOUND_CLOCK_TICKS_;

static float soundVolume = 1.0f;
static int soundEnableFlag = 0x3ff; // emulator channels enabled
static float soundFiltering_ = -1;
static float soundVolume_ = -1;

class Gba_Pcm
{
public:
	void init();
	void apply_control(int idx);
	void update(int dac);
	void end_frame(blip_time_t);

private:
	Blip_Buffer *output;
	blip_time_t last_time;
	int last_amp;
	int shift;
};

class Gba_Pcm_Fifo
{
public:
	int which;
	Gba_Pcm pcm;

	void write_control(int data);
	void write_fifo(int data);
	void timer_overflowed(int which_timer);

private:
	int readIndex;
	int count;
	int writeIndex;
	uint8_t fifo[32];
	int dac;
	int timer;
	bool enabled;
};

static Gba_Pcm_Fifo pcm[2];
static std::unique_ptr<Gb_Apu> gb_apu;
static std::unique_ptr<Stereo_Buffer> stereo_buffer;

static Blip_Synth<blip_high_quality, 1> pcm_synth[3]; // 32 kHz, 16 kHz, 8 kHz

static inline blip_time_t blip_time()
{
	return SOUND_CLOCK_TICKS - soundTicks;
}

inline void Gba_Pcm::init()
{
	this->output = nullptr;
	this->last_time = 0;
	this->last_amp = 0;
	this->shift = 0;
}

void Gba_Pcm::apply_control(int idx)
{
	this->shift = ~ioMem[SGCNT0_H] >> (2 + idx) & 1;

	int ch = 0;
	if ((soundEnableFlag >> idx & 0x100) && (ioMem[NR52] & 0x80))
		ch = ioMem[SGCNT0_H + 1] >> (idx * 4) & 3;

	Blip_Buffer *out = nullptr;
	switch (ch)
	{
		case 1:
			out = stereo_buffer->right();
			break;
		case 2:
			out = stereo_buffer->left();
			break;
		case 3:
			out = stereo_buffer->center();
	}

	if (this->output != out)
	{
		if (this->output)
		{
			auto time = blip_time();

			this->output->set_modified();

			int filter = 0;
			if (soundInterpolation)
			{
				// base filtering on how long since last sample was output
				int32_t period = time - this->last_time;

				idx = period / 512;
				if (idx >= 3)
					idx = 3;

				static const int filters[] = { 0, 0, 1, 2 };
				filter = filters[idx];
			}

			pcm_synth[filter].offset(time, -this->last_amp, this->output);
		}
		this->last_amp = 0;
		this->output = out;
	}
}

inline void Gba_Pcm::end_frame(blip_time_t time)
{
	this->last_time -= time;
	if (this->last_time < -2048)
		this->last_time = -2048;

	if (this->output)
		this->output->set_modified();
}

void Gba_Pcm::update(int dac)
{
	if (this->output)
	{
		auto time = blip_time();

		dac = static_cast<int8_t>(dac) >> this->shift;
		int delta = dac - this->last_amp;
		if (delta)
		{
			this->last_amp = dac;

			int filter = 0;
			if (soundInterpolation)
			{
				// base filtering on how long since last sample was output
				int32_t period = time - this->last_time;

				int idx = period / 512;
				if (idx >= 3)
					idx = 3;

				static const int filters[] = { 0, 0, 1, 2 };
				filter = filters[idx];
			}

			pcm_synth[filter].offset(time, delta, this->output);
		}
		this->last_time = time;
	}
}

void Gba_Pcm_Fifo::timer_overflowed(int which_timer)
{
	if (which_timer == this->timer && this->enabled)
	{
		/* Mother 3 fix, refined to not break Metroid Fusion */
		if (this->count == 16 || !this->count)
		{
			// Need to fill FIFO
			int saved_count = this->count;
			CPUCheckDMA(3, this->which ? 4 : 2);
			if (!saved_count && this->count == 16)
				CPUCheckDMA(3, this->which ? 4 : 2);
			if (!this->count)
			{
				// Not filled by DMA, so fill with 16 bytes of silence
				int regi = this->which ? FIFOB_L : FIFOA_L;
				for (int n = 8; n--; )
				{
					soundEvent(regi, static_cast<uint16_t>(0));
					soundEvent(regi + 2, static_cast<uint16_t>(0));
				}
			}
		}

		// Read next sample from FIFO
		--this->count;
		this->dac = this->fifo[this->readIndex];
		this->readIndex = (this->readIndex + 1) & 31;
		this->pcm.update(this->dac);
	}
}

void Gba_Pcm_Fifo::write_control(int data)
{
	this->enabled = !!(data & 0x0300);
	this->timer = !!(data & 0x0400);

	if (data & 0x0800)
	{
		// Reset
		this->writeIndex = 0;
		this->readIndex = 0;
		this->count = 0;
		this->dac = 0;
		memset(this->fifo, 0, sizeof(this->fifo));
	}

	this->pcm.apply_control(this->which);
	this->pcm.update(this->dac);
}

inline void Gba_Pcm_Fifo::write_fifo(int data)
{
	this->fifo[this->writeIndex] = data & 0xFF;
	this->fifo[this->writeIndex + 1] = data >> 8;
	this->count += 2;
	this->writeIndex = (this->writeIndex + 2) & 31;
}

static void apply_control()
{
	pcm[0].pcm.apply_control(0);
	pcm[1].pcm.apply_control(1);
}

static int gba_to_gb_sound(int addr)
{
	static const int table[] =
	{
		0xFF10, 0, 0xFF11, 0xFF12, 0xFF13, 0xFF14, 0, 0,
		0xFF16, 0xFF17, 0, 0, 0xFF18, 0xFF19, 0, 0,
		0xFF1A, 0, 0xFF1B, 0xFF1C, 0xFF1D, 0xFF1E, 0, 0,
		0xFF20, 0xFF21, 0, 0, 0xFF22, 0xFF23, 0, 0,
		0xFF24, 0xFF25, 0, 0, 0xFF26, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0,
		0xFF30, 0xFF31, 0xFF32, 0xFF33, 0xFF34, 0xFF35, 0xFF36, 0xFF37,
		0xFF38, 0xFF39, 0xFF3A, 0xFF3B, 0xFF3C, 0xFF3D, 0xFF3E, 0xFF3F,
	};
	if (addr >= 0x60 && addr < 0xA0)
		return table[addr - 0x60];
	return 0;
}

void soundEvent(uint32_t address, uint8_t data)
{
	int gb_addr = gba_to_gb_sound(address);
	if (gb_addr)
	{
		ioMem[address] = data;
		gb_apu->write_register(blip_time(), gb_addr, data);

		if (address == NR52)
			apply_control();
	}

	ioMem[NR52] = (ioMem[NR52] & 0x80) | (gb_apu->read_status() & 0x7f);

	// TODO: what about byte writes to SGCNT0_H etc.?
}

static void apply_volume(bool apu_only = false)
{
	if (!apu_only)
		soundVolume_ = soundVolume;

	if (gb_apu)
	{
		static const float apu_vols[] = { 0.25, 0.5, 1, 0.25 };
		gb_apu->volume(soundVolume_ * apu_vols[ioMem[SGCNT0_H] & 3]);
	}

	if (!apu_only)
		for (int i = 0; i < 3; ++i)
			pcm_synth[i].volume(0.66 / 256 * soundVolume_);
}

static void write_SGCNT0_H(int data)
{
	WRITE16LE(&ioMem[SGCNT0_H], data & 0x770F);
	pcm[0].write_control(data);
	pcm[1].write_control(data >> 4);
	apply_volume(true);
}

void soundEvent(uint32_t address, uint16_t data)
{
	switch (address)
	{
		case SGCNT0_H:
			write_SGCNT0_H(data);
			break;

		case FIFOA_L:
		case FIFOA_H:
			pcm[0].write_fifo(data);
			WRITE16LE(&ioMem[address], data);
			break;

		case FIFOB_L:
		case FIFOB_H:
			pcm[1].write_fifo(data);
			WRITE16LE(&ioMem[address], data);
			break;

		case 0x88:
			data &= 0xC3FF;
			WRITE16LE(&ioMem[address], data);
			break;

		default:
			soundEvent(address & ~1, static_cast<uint8_t>(data)); // even
			soundEvent(address | 1, static_cast<uint8_t>(data >> 8)); // odd
	}
}

void soundTimerOverflow(int timer)
{
	pcm[0].timer_overflowed(timer);
	pcm[1].timer_overflowed(timer);
}

static void end_frame(blip_time_t time)
{
	pcm[0].pcm.end_frame(time);
	pcm[1].pcm.end_frame(time);

	gb_apu->end_frame(time);
	stereo_buffer->end_frame(time);
}

void flush_samples(Multi_Buffer *buffer)
{
	// We want to write the data frame by frame to support legacy audio drivers
	// that don't use the length parameter of the write method.
	// TODO: Update the Win32 audio drivers (DS, OAL, XA2), and flush all the
	// samples at once to help reducing the audio delay on all platforms.
	int32_t soundBufferLen = (soundSampleRate / 60) * 4;

	// soundBufferLen should have a whole number of sample pairs
	assert(!(soundBufferLen % (2 * sizeof(*soundFinalWave))));

	// number of samples in output buffer
	int32_t out_buf_size = soundBufferLen / sizeof(*soundFinalWave);

	while (buffer->samples_avail())
	{
		long samples_read = buffer->read_samples(reinterpret_cast<blip_sample_t *>(soundFinalWave), out_buf_size);
		if (soundPaused)
			soundResume();

		soundDriver->write(soundFinalWave, samples_read * sizeof(*soundFinalWave));
	}
}

static void apply_filtering()
{
	soundFiltering_ = soundFiltering;

	int base_freq = static_cast<int>(32768 - soundFiltering_ * 16384);
	int32_t nyquist = stereo_buffer->sample_rate() / 2;

	for (int i = 0; i < 3; ++i)
	{
		int32_t cutoff = base_freq >> i;
		if (cutoff > nyquist)
			cutoff = nyquist;
		pcm_synth[i].treble_eq(blip_eq_t(0, 0, stereo_buffer->sample_rate(), cutoff));
	}
}

void psoundTickfn()
{
 	if (gb_apu && stereo_buffer)
	{
		// Run sound hardware to present
		end_frame(SOUND_CLOCK_TICKS);

		flush_samples(stereo_buffer.get());

		if (!fEqual(soundFiltering_, soundFiltering))
			apply_filtering();

		if (!fEqual(soundVolume_, soundVolume))
			apply_volume();
	}
}

static void apply_muting()
{
	if (!stereo_buffer)
		return;

	// PCM
	apply_control();

	if (gb_apu)
		// APU
		for (int i = 0; i < 4; ++i)
		{
			if (soundEnableFlag >> i & 1)
				gb_apu->set_output(stereo_buffer->center(), stereo_buffer->left(), stereo_buffer->right(), i);
			else
				gb_apu->set_output(nullptr, nullptr, nullptr, i);
		}
}

static void reset_apu()
{
	if (gb_apu)
	{
		gb_apu->reduce_clicks(true);
		gb_apu->reset(Gb_Apu::mode_agb, true);
	}

	if (stereo_buffer)
		stereo_buffer->clear();

	soundTicks = SOUND_CLOCK_TICKS;
}

static void remake_stereo_buffer()
{
	// Clears pointers kept to old stereo_buffer
	pcm[0].pcm.init();
	pcm[1].pcm.init();

	// Stereo_Buffer
	stereo_buffer.reset(new Stereo_Buffer); // TODO: handle out of memory
	stereo_buffer->set_sample_rate(soundSampleRate); // TODO: handle out of memory

	// PCM
	pcm[0].which = 0;
	pcm[1].which = 1;
	apply_filtering();

	// APU
	if (!gb_apu)
	{
		gb_apu.reset(new Gb_Apu); // TODO: handle out of memory
		reset_apu();
		gb_apu->treble_eq(blip_eq_t(0, 0, soundSampleRate, soundSampleRate / 2));
	}
	stereo_buffer->clock_rate(gb_apu->clock_rate);

	// Volume Level
	apply_muting();
	apply_volume();
}

void soundShutdown()
{
	soundDriver.reset();

	// APU
	gb_apu.reset();

	// Stereo_Buffer
	stereo_buffer.reset();
}

void soundPause()
{
	soundPaused = true;
	if (soundDriver)
		soundDriver->pause();
}

void soundResume()
{
	soundPaused = false;
	if (soundDriver)
		soundDriver->resume();
}

void soundSetEnable(int channels)
{
	soundEnableFlag = channels;
	apply_muting();
}

void soundReset()
{
	soundDriver->reset();

	remake_stereo_buffer();
	reset_apu();

	soundPaused = true;
	SOUND_CLOCK_TICKS = soundTicks = SOUND_CLOCK_TICKS_;

	soundEvent(NR52, static_cast<uint8_t>(0x80));
}

bool soundInit()
{
	soundDriver.reset(systemSoundInit());
	if (!soundDriver)
		return false;

	if (!soundDriver->init(soundSampleRate))
		return false;

	soundPaused = true;
	return true;
}

void soundSetSampleRate(long sampleRate)
{
	if (soundSampleRate != sampleRate)
	{
		soundSampleRate = sampleRate;

		remake_stereo_buffer();
	}
}
