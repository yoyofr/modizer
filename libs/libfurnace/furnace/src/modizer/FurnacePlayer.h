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
  unsigned short readPos;  // current read needle — start reading from here
  size_t        rate;      // actual chip sample rate in Hz
  bool          valid;     // false if this channel has no osc buffer
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
   * Seek to the given position (in seconds) within the current subsong.
   * Finds the closest order boundary and jumps there.
   * @param seconds  Target time in seconds (clamped to [0, duration]).
   * @return true if the seek succeeded.
   */
  bool seek(double seconds);

private:
  DivEngine* engine;
  int        sampleRate;
  bool       engineReady;
  bool       songLoaded;
  bool       userStopped;   // true when stop() was called explicitly

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
