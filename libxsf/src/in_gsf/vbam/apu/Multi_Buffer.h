// Multi-channel sound buffer interface, and basic mono and stereo buffers

// Blip_Buffer 0.4.1
#pragma once

#include "blargg_common.h"
#include "Blip_Buffer.h"

// Interface to one or more Blip_Buffers mapped to one or more channels
// consisting of left, center, and right buffers.
class Multi_Buffer
{
public:
	Multi_Buffer(int samples_per_frame);
	virtual ~Multi_Buffer() { }

	// Sets the number of channels available and optionally their types
	// (type information used by Effects_Buffer)
	enum { type_index_mask = 0xFF };
	enum { wave_type = 0x100, noise_type = 0x200, mixed_type = wave_type | noise_type };
	virtual void set_channel_count(int, const int *types = nullptr);
	int channel_count() const { return this->channel_count_; }

	// Gets indexed channel, from 0 to channel count - 1
	struct channel_t
	{
		Blip_Buffer *center;
		Blip_Buffer *left;
		Blip_Buffer *right;
	};
	virtual channel_t channel(int index);

	// See Blip_Buffer.h
	virtual void set_sample_rate(long rate, long msec = blip_default_length);
	virtual void clock_rate(long) { }
	virtual void bass_freq(int) { }
	virtual void clear() { }
	long sample_rate() const;

	// Length of buffer, in milliseconds
	int32_t length() const;

	// See Blip_Buffer.h
	virtual void end_frame(blip_time_t) { }

	// Number of samples per output frame (1 = mono, 2 = stereo)
	int samples_per_frame() const;

	// Count of changes to channel configuration. Incremented whenever
	// a change is made to any of the Blip_Buffers for any channel.
	unsigned channels_changed_count() { return this->channels_changed_count_; }

	// See Blip_Buffer.h
	virtual long read_samples(blip_sample_t *, long) { return 0; }
	virtual long samples_avail() const { return 0; }

	void disable_immediate_removal() { this->immediate_removal_ = false; }
protected:
	bool immediate_removal() const { return this->immediate_removal_; }
	const int *channel_types() const { return this->channel_types_; }
	void channels_changed() { ++this->channels_changed_count_; }
private:
	// noncopyable
	Multi_Buffer(const Multi_Buffer &);
	Multi_Buffer &operator=(const Multi_Buffer &);

	unsigned channels_changed_count_;
	long sample_rate_;
	int32_t length_;
	int channel_count_;
	const int samples_per_frame_;
	const int *channel_types_;
	bool immediate_removal_;
};

class Tracked_Blip_Buffer : public Blip_Buffer
{
public:
	// Non-zero if buffer still has non-silent samples in it. Requires that you call
	// set_modified() appropriately.
	uint32_t non_silent() const;

	long read_samples(blip_sample_t *, long);
	void remove_silence(long);
	void remove_samples(long);
	Tracked_Blip_Buffer();
	void clear();
	void end_frame(blip_time_t);
private:
	int32_t last_non_silence;
	void remove_(long);
};

class Stereo_Mixer
{
public:
	Tracked_Blip_Buffer *bufs[3];
	int32_t samples_read;

	Stereo_Mixer() : samples_read(0) { }
	void read_pairs(blip_sample_t *out, int count);
private:
	void mix_mono(blip_sample_t *out, int pair_count);
	void mix_stereo(blip_sample_t *out, int pair_count);
};

// Uses three buffers (one for center) and outputs stereo sample pairs.
class Stereo_Buffer : public Multi_Buffer
{
public:
	// Buffers used for all channels
	Blip_Buffer *center() { return &this->bufs[2]; }
	Blip_Buffer *left() { return &this->bufs[0]; }
	Blip_Buffer *right() { return &this->bufs[1]; }

	Stereo_Buffer();
	~Stereo_Buffer();
	void set_sample_rate(long, long msec = blip_default_length);
	void clock_rate(long);
	void bass_freq(int);
	void clear();
	channel_t channel(int) { return this->chan; }
	void end_frame(blip_time_t);

	long samples_avail() const { return (this->bufs[0].samples_avail() - this->mixer.samples_read) * 2; }
	long read_samples(blip_sample_t *, long);

private:
	enum { bufs_size = 3 };
	typedef Tracked_Blip_Buffer buf_t;
	buf_t bufs[bufs_size];
	Stereo_Mixer mixer;
	channel_t chan;
};

inline void Multi_Buffer::set_sample_rate(long rate, long msec)
{
	this->sample_rate_ = rate;
	this->length_ = msec;
}

inline int Multi_Buffer::samples_per_frame() const { return this->samples_per_frame_; }

inline long Multi_Buffer::sample_rate() const { return this->sample_rate_; }

inline int32_t Multi_Buffer::length() const { return this->length_; }

inline void Multi_Buffer::set_channel_count(int n, const int *types)
{
	this->channel_count_ = n;
	this->channel_types_ = types;
}
