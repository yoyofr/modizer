/**
 * Furnace Tracker - Modizer interface
 * Copyright (C) 2021-2026 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * FurnacePlayer.h - Modizer wrapper class for DivEngine
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Forward declaration to avoid including the full engine header
class DivEngine;

/**
 * Live state of a channel — values reflect the current engine state including
 * all active effects (vibrato, portamento, tremolo, fine pitch…).
 */
struct FurnaceChannelLiveState {
  // --- Note & instrument ---
  // note = pattern_note - 60  (e.g. A-4 = pattern 57 → note = -3)
  // To display: patNote = note+60; octave = patNote/12; semitone = patNote%12
  // (semitone: 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=A#, 11=B)
  int  note;        // -60..119, or INT_MIN if no note playing
  int  instrument;  // last used instrument index, -1 = none
  int  volume;      // current volume 0-127 (upper byte of engine's 16-bit fixed-point)

  // --- Pitch ---
  // pitch offset in ~1/128-semitone units (includes E5xx, vibrato, portamento).
  int  pitch;

  // Frequency in Hz for the current note + pitch offset.
  // Formula: song.tuning * 2^((note + 3 + pitch/128) / 12)
  // (tuning = song's A4 frequency, default 440 Hz; A-4 → note=-3 → 440 Hz ✓)
  double freqHz;

  // --- Modulation ---
  int  vibratoDepth; // 0 = off
  int  vibratoRate;
  int  tremoloDepth; // 0 = off
  int  tremoloRate;

  // --- Key / portamento state ---
  bool keyOn;    // note just triggered this tick
  bool keyOff;   // note releasing
  bool active;   // channel is producing sound
  bool inPorta;  // portamento slide in progress
  int  portaNote; // portamento target note (-1 = none)
};

/**
 * One note cell in a pattern — equivalent to ModPlugNote for Furnace data.
 * Layout: array[row * numChannels + chan]
 */
struct FurnacePatternNote {
  int16_t note;        // 0-179 (C-0..B-14), 253=note off, 254=release, -1=empty
  int16_t instrument;  // 0-based instrument index, -1=empty
  int16_t volume;      // 0-255, -1=empty
  int16_t fx[4];       // effect codes  (columns 0-3), -1=empty
  int16_t fxVal[4];    // effect values (columns 0-3), -1=empty
};

/**
 * One row of pattern data for a single channel, at the current playhead.
 */
struct FurnaceChannelRow {
  int  note;        // 0-179 = C-(-5)..B-9; 253=note off; 254=release; -1=empty
  int  instrument;  // instrument index, -1 = empty
  int  volume;      // volume, -1 = empty
  int  numEffects;  // number of effect columns present
  int  fx[8];       // effect codes   (up to 8), -1 = empty
  int  fxVal[8];    // effect values  (up to 8), -1 = empty
};

/**
 * Oscilloscope data for one channel — a snapshot of the most recent samples.
 */
struct FurnaceOscData {
  const short* data;       // pointer into the circular buffer (65536 samples at 65536 Hz)
  unsigned short readPos;  // write needle (most recent sample index) — read backwards from here
  size_t        rate;      // actual chip sample rate in Hz (buffer always runs at 65536 Hz)
  bool          valid;     // false if this channel has no osc buffer

  // Buffer always runs at OSC_RATE Hz regardless of output sample rate.
  static constexpr int OSC_RATE = 65536;
};

/**
 * Information about one sound system slot in a song.
 * A song can combine up to DIV_MAX_CHIPS systems simultaneously.
 */
struct FurnaceSystemInfo {
  int         index;        // system slot index (0 .. systemCount-1)
  const char* name;         // human-readable name, e.g. "NES", "Game Boy"
  int         firstChannel; // global index of the first channel of this system
  int         channelCount; // number of channels contributed by this system
};

/**
 * Per-channel system mapping — tells which system and sub-channel a global
 * channel index belongs to.
 */
struct FurnaceChannelSystemInfo {
  int         systemIndex;  // system slot index (same as FurnaceSystemInfo::index)
  const char* systemName;   // human-readable system name
  int         subChannel;   // channel index within that system (-1 if unmapped)
};

/**
 * Song information returned by getInfo().
 */
struct FurnaceSongInfo {
  std::string title;        // Song title
  std::string author;       // Author / composer
  std::string systemName;   // Sound system name (e.g. "YM2612 + SN76489")
  std::string comment;      // Song notes / comments
  int subsongCount;         // Number of subsongs (>= 1)
  int currentSubsong;       // Currently selected subsong index
  std::string subsongName;  // Name of the current subsong
  int channels;             // Total number of channels
  double sampleRate;        // Output sample rate in Hz
};

/**
 * FurnacePlayer - a simple, self-contained playback wrapper around DivEngine
 * for integration with Modizer.
 *
 * Typical lifecycle:
 *   FurnacePlayer player;
 *   player.init(44100);
 *   player.load(data, dataLen, "song.fur");
 *   player.play(buffer, frameCount);  // call repeatedly
 *   player.stop();
 *   player.close();
 */
class FurnacePlayer {
public:
  FurnacePlayer();
  ~FurnacePlayer();

  /**
   * Initialize the engine with the given output sample rate.
   * Must be called before load().
   * @param sampleRate  Output sample rate in Hz (e.g. 44100, 48000).
   * @return true on success.
   */
  bool init(int sampleRate = 44100);

  /**
   * Load a module from a memory buffer.
   * Supports .fur, .dmf, .mod, .s3m, .xm, .it, .ftm, etc.
   * @param data      Pointer to the file data.
   * @param dataLen   Size of the data in bytes.
   * @param filename  Optional filename hint used for format detection.
   * @return true on success.
   */
  bool load(const uint8_t* data, size_t dataLen, const char* filename = nullptr);

  /**
   * Return metadata and playback information about the loaded song.
   * Only valid after a successful load().
   */
  FurnaceSongInfo getInfo() const;

  /**
   * Select a subsong by index (0-based).
   * Has no effect if the song has only one subsong.
   * @param index  Subsong index in [0, subsongCount).
   * @return true if the index is valid.
   */
  bool selectSong(int index);

  /**
   * Render audio into a signed 16-bit stereo interleaved buffer.
   * Call this repeatedly from your audio thread/callback.
   * @param buffer      Output buffer (left, right, left, right, ...).
   * @param frameCount  Number of stereo frames to render.
   */
  void render(int16_t* buffer, int frameCount);

  /**
   * Pause / resume playback without resetting position.
   * @param playing  true to play, false to pause.
   */
  void setPlaying(bool playing);

  /**
   * Stop playback and reset position to the beginning.
   */
  void stop();

  /**
   * Unload the current song and free associated resources.
   * The engine stays initialised; you can call load() again afterwards.
   */
  void close();

  /**
   * Return the last error message from the engine.
   */
  std::string getLastError() const;

  /**
   * Return whether the song has reached its end.
   * Useful for detecting when to advance to the next track.
   */
  bool isEndOfSong() const;

  /**
   * Return true if the engine is currently playing (not paused / stopped).
   */
  bool isPlaying() const;

  /**
   * Get the pattern row currently being played on a given channel.
   * @param chan  Channel index in [0, channels).
   * @return Populated FurnaceChannelRow; note=-1 if channel is invalid.
   */
  FurnaceChannelRow getChannelRow(int chan) const;

  /**
   * Get oscilloscope data for a given channel.
   * The returned pointer is valid until the next call to render().
   * @param chan  Channel index in [0, channels).
   */
  FurnaceOscData getOscData(int chan) const;

  /**
   * Get the live engine state of a channel (note, volume, pitch with effects, etc.).
   * Reflects the actual playing state including vibrato, portamento, fine pitch…
   * @param chan  Channel index in [0, channels).
   * @return Populated FurnaceChannelLiveState; note=-1 if channel is inactive.
   */
  FurnaceChannelLiveState getChannelLiveState(int chan) const;

  /**
   * Get all pattern data for a given order position in the current subsong.
   *
   * Equivalent to xmp_getPattern:numrows: — returns a flat array of
   * FurnacePatternNote laid out as [row * numChannels + chan].
   *
   * The returned buffer is owned by the caller and must be freed with delete[].
   * Returns nullptr if no song is loaded or orderIdx is out of range.
   *
   * @param orderIdx   Order index in [0, ordersLen).
   * @param numrows    Filled with the number of rows in the pattern.
   * @param numchans   Filled with the number of channels.
   */
  FurnacePatternNote* getOrderPattern(int orderIdx, int* numrows, int* numchans) const;

  /**
   * Number of orders in the current subsong.
   */
  int getOrderCount() const;

  /**
   * Duration of the current subsong in seconds.
   * Returns 0 if no song is loaded or timestamps have not been calculated.
   * Triggers timestamp calculation on first call after load/selectSong.
   */
  double getDuration();

  /**
   * Total duration of all subsongs in seconds.
   * Triggers timestamp calculation for every subsong on first call.
   */
  double getTotalDuration();

  /**
   * Current playback position in the current subsong, in seconds.
   * Returns 0 if no song is loaded or position cannot be determined.
   */
  double getPosition() const;
  
  /**
   * Current order index
   * Returns -1 if no song is loaded or order index cannot be determined.
   */
  int getCurrentOrder();
  
  /**
   * Current row
   * Returns -1 if no song is loaded or row cannot be determined.
   */
  int getCurrentRow();

  /**
   * Seek to the given position (in seconds) within the current subsong.
   * Finds the closest order boundary and jumps there.
   * @param seconds  Target time in seconds (clamped to [0, duration]).
   * @return true if the seek succeeded.
   */
  bool seek(double seconds);

  /**
   * Return the number of sound systems active in the loaded song.
   * Returns 0 if no song is loaded.
   */
  int getSystemCount() const;

  /**
   * Return information about one system slot (index in [0, getSystemCount())).
   * FurnaceSystemInfo::name is valid for the lifetime of the loaded song.
   */
  FurnaceSystemInfo getSystemInfo(int sysIdx) const;

  /**
   * Return the system mapping for a global channel index.
   * Tells which system the channel belongs to and its sub-channel index within
   * that system.  systemName is valid for the lifetime of the loaded song.
   * @param chan  Channel index in [0, channels).
   */
  FurnaceChannelSystemInfo getChannelSystemInfo(int chan) const;

  /**
   * Set playback speed as a ratio (1.0 = normal, 2.0 = 2× faster, 0.5 = 2× slower).
   * Implemented via the engine's virtualTempoN/D mechanism.
   * @param ratio  Speed multiplier, clamped to [0.01, 100].
   */
  void setPlaybackSpeed(float ratio);

  /**
   * Mute or unmute a channel.
   * @param chan  Channel index in [0, channels).
   * @param mute  true to mute, false to unmute.
   */
  void setMute(int chan, bool mute);

  /**
   * Return the mute state of a channel.
   * @param chan  Channel index in [0, channels).
   * @return true if muted, false otherwise (also false for an invalid channel).
   */
  bool isMuted(int chan) const;

  /**
   * Return the short name of a channel (signal type: FM, PCM, square, wave…).
   * The pointer is valid for the lifetime of the loaded song.
   * @param chan  Channel index in [0, channels).
   * @return Short name string, or "??" for an invalid channel.
   */
  const char* getChannelShortName(int chan) const;

  /**
   * Return the long name of a channel (device + signal type, e.g. "YM2612 CH1").
   * The pointer is valid for the lifetime of the loaded song.
   * @param chan  Channel index in [0, channels).
   * @return Long name string, or "??" for an invalid channel.
   */
  const char* getChannelLongName(int chan) const;

private:
  DivEngine* engine;
  int        sampleRate;
  bool       engineReady;
  bool       songLoaded;
  bool       userStopped;   // true when stop() was called explicitly
  float      playbackSpeed; // current speed ratio (1.0 = normal)

  // Internal float stereo buffer used to bridge DivEngine (float) → int16_t
  float** floatBuf;       // floatBuf[0] = left, floatBuf[1] = right
  int     floatBufSize;   // current capacity in frames

  void allocateFloatBuf(int frames);
  void freeFloatBuf();

  // true after calcSongTimestamps() has been called for the current subsong
  bool timestampsReady;

  // cached total duration across all subsongs (-1 = not yet computed)
  double totalDurationCache;

  void ensureTimestamps();
};
