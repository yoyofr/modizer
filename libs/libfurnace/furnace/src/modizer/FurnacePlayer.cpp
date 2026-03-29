/**
 * Furnace Tracker - Modizer interface
 * Copyright (C) 2021-2026 tildearrow and contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * FurnacePlayer.cpp - Modizer wrapper class for DivEngine
 */

#include "FurnacePlayer.h"

// Full engine header (must be on the include path when compiling)
#include "../engine/engine.h"
#include "../engine/song.h"
#include "../engine/dispatch.h"
#include "../engine/defines.h"
#include "../timeutils.h"

#include <cstring>
#include <algorithm>
#include <cmath>
#include <pthread.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static inline int16_t floatToS16(float v) {
  // Clamp to [-1, 1] then scale to INT16 range
  if (v >  1.0f) v =  1.0f;
  if (v < -1.0f) v = -1.0f;
  return static_cast<int16_t>(v * 32767.0f);
}

// ---------------------------------------------------------------------------
// FurnacePlayer
// ---------------------------------------------------------------------------

FurnacePlayer::FurnacePlayer()
  : engine(nullptr)
  , sampleRate(44100)
  , engineReady(false)
  , songLoaded(false)
  , userStopped(false)
  , floatBuf(nullptr)
  , floatBufSize(0)
  , timestampsReady(false)
  , totalDurationCache(-1.0)
{}

FurnacePlayer::~FurnacePlayer() {
  close();
  if (engineReady) {
    engine->quit(false);
    engineReady = false;
  }
  delete engine;
  engine = nullptr;
  freeFloatBuf();
}

// ---------------------------------------------------------------------------
// init
// ---------------------------------------------------------------------------

bool FurnacePlayer::init(int rate) {
  sampleRate = rate;

  if (engine) {
    // Already initialised — just update the rate in the config and restart
    if (engineReady) {
      engine->quit(false);
      engineReady = false;
    }
    delete engine;
    engine = nullptr;
  }

  engine = new DivEngine();

  // Use DIV_AUDIO_DUMMY so the engine does NOT open any real audio device.
  // We drive it manually through nextBuf().
  engine->setAudio(DIV_AUDIO_DUMMY);

  // Provide a minimal config: sample rate and buffer size.
  // prePreInit() reads/writes a config file; calling it here lets the engine
  // set up its internal config path without requiring an existing file.
  engine->prePreInit();

  // Propagate the desired sample rate before init() reads the config.
  engine->getAudioDescWant().rate    = static_cast<double>(rate);
  engine->getAudioDescWant().bufsize = 1024;
  engine->getAudioDescWant().outChans = 2;
  engine->getAudioDescWant().outFormat = TA_AUDIO_FORMAT_F32;

  engine->preInit(true /* noSafeMode */);

  if (!engine->init()) {
    return false;
  }

  engineReady = true;
  return true;
}

// ---------------------------------------------------------------------------
// load
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// loadOnLargeStack — run DivEngine::load on a thread with 8 MB stack to
// avoid stack overflow in loadFTM (DivSong + large local arrays ~200-400 KB).
// ---------------------------------------------------------------------------

struct LoadContext {
  DivEngine*    engine;
  uint8_t*      data;    // heap-allocated copy, owned by this struct
  size_t        dataLen;
  const char*   filename;
  bool          result;
};

static void* loadThreadFunc(void* arg) {
  LoadContext* ctx = static_cast<LoadContext*>(arg);
  // DivEngine::load() takes ownership of the buffer and frees it.
  ctx->result = ctx->engine->load(ctx->data, ctx->dataLen, ctx->filename);
  return nullptr;
}

bool FurnacePlayer::load(const uint8_t* data, size_t dataLen, const char* filename) {
  if (!engineReady) return false;

  // Allocate a copy of the data; DivEngine::load() will free it via delete[].
  uint8_t* buf = new uint8_t[dataLen];
  std::memcpy(buf, data, dataLen);

  LoadContext ctx = { engine, buf, dataLen, filename, false };

  // Spin a thread with 8 MB stack to safely accommodate loadFTM's heavy
  // local variables (DivSong on stack alone can exceed 200 KB).
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);

  pthread_t thread;
  if (pthread_create(&thread, &attr, loadThreadFunc, &ctx) != 0) {
    pthread_attr_destroy(&attr);
    delete[] buf;
    return false;
  }
  pthread_attr_destroy(&attr);
  pthread_join(thread, nullptr);

  if (!ctx.result) return false;

  songLoaded         = true;
  userStopped        = false;
  timestampsReady    = false;
  totalDurationCache = -1.0;
  engine->play();
  return true;
}

// ---------------------------------------------------------------------------
// getInfo
// ---------------------------------------------------------------------------

FurnaceSongInfo FurnacePlayer::getInfo() const {
  FurnaceSongInfo info{};
  if (!songLoaded) return info;

  info.title        = engine->song.name.c_str();
  info.author       = engine->song.author.c_str();
  info.systemName   = engine->song.systemName.c_str();
  info.comment      = engine->song.notes.c_str();
  info.subsongCount = static_cast<int>(engine->song.subsong.size());
  info.currentSubsong = static_cast<int>(engine->getCurrentSubSong());
  info.channels     = engine->getTotalChannelCount();
  info.sampleRate   = engine->getAudioDescGot().rate;

  if (info.currentSubsong < info.subsongCount && info.subsongCount > 0) {
    const DivSubSong* ss = engine->song.subsong[info.currentSubsong];
    if (ss) info.subsongName = ss->name.c_str();
  }

  return info;
}

// ---------------------------------------------------------------------------
// selectSong
// ---------------------------------------------------------------------------

bool FurnacePlayer::selectSong(int index) {
  if (!songLoaded) return false;
  int count = static_cast<int>(engine->song.subsong.size());
  if (index < 0 || index >= count) return false;

  engine->changeSongP(static_cast<size_t>(index));
  timestampsReady = false;   // new subsong needs fresh timestamps
  engine->play();
  return true;
}

// ---------------------------------------------------------------------------
// render
// ---------------------------------------------------------------------------

void FurnacePlayer::render(int16_t* buffer, int frameCount) {
  if (!engineReady || !songLoaded || frameCount <= 0) {
    // Fill with silence
    std::memset(buffer, 0, frameCount * 2 * sizeof(int16_t));
    return;
  }

  allocateFloatBuf(frameCount);

  // Zero the float buffers before calling nextBuf
  std::memset(floatBuf[0], 0, frameCount * sizeof(float));
  std::memset(floatBuf[1], 0, frameCount * sizeof(float));

  // DivEngine renders float stereo into floatBuf[0] (L) and floatBuf[1] (R)
  engine->nextBuf(nullptr, floatBuf, 0, 2, static_cast<unsigned int>(frameCount));

  // Convert float → int16, interleaved L/R
  for (int i = 0; i < frameCount; ++i) {
    buffer[i * 2 + 0] = floatToS16(floatBuf[0][i]); // Left
    buffer[i * 2 + 1] = floatToS16(floatBuf[1][i]); // Right
  }
}

// ---------------------------------------------------------------------------
// setPlaying
// ---------------------------------------------------------------------------

void FurnacePlayer::setPlaying(bool playing) {
  if (!engineReady) return;
  if (playing) {
    engine->play();
  } else {
    // Halt the engine tick loop without resetting position
    engine->halt();
  }
}

// ---------------------------------------------------------------------------
// stop
// ---------------------------------------------------------------------------

void FurnacePlayer::stop() {
  if (!engineReady) return;
  userStopped = true;
  engine->stop();
}

// ---------------------------------------------------------------------------
// close
// ---------------------------------------------------------------------------

void FurnacePlayer::close() {
  if (!engineReady) return;
  if (songLoaded) {
    engine->stop();
    songLoaded = false;
  }
}

// ---------------------------------------------------------------------------
// getLastError
// ---------------------------------------------------------------------------

std::string FurnacePlayer::getLastError() const {
  if (!engine) return "";
  return engine->getLastError().c_str();
}

// ---------------------------------------------------------------------------
// isEndOfSong
// ---------------------------------------------------------------------------

bool FurnacePlayer::isEndOfSong() const {
  // The engine sets playing=false when the song finishes naturally.
  // We distinguish this from an explicit stop() via userStopped.
  if (!engine || !songLoaded) return false;
  return !userStopped && !engine->isPlaying();
}

// ---------------------------------------------------------------------------
// isPlaying
// ---------------------------------------------------------------------------

bool FurnacePlayer::isPlaying() const {
  if (!engine) return false;
  return engine->isPlaying();
}

// ---------------------------------------------------------------------------
// getChannelRow
// ---------------------------------------------------------------------------

FurnaceChannelRow FurnacePlayer::getChannelRow(int chan) const {
  FurnaceChannelRow row{};
  row.note = row.instrument = row.volume = -1;
  for (int i = 0; i < 8; i++) row.fx[i] = row.fxVal[i] = -1;

  if (!songLoaded || chan < 0 || chan >= engine->getTotalChannelCount()) return row;

  int cur     = static_cast<int>(engine->getCurrentSubSong());
  int order   = static_cast<int>(engine->getOrder());
  int rowIdx  = engine->getRow();

  DivSubSong* ss = engine->song.subsong[cur];
  if (order < 0 || order >= ss->ordersLen) return row;

  int patIndex = ss->orders.ord[chan][order];
  DivPattern* pat = ss->pat[chan].getPattern(patIndex, false);
  if (!pat) return row;

  const short* rd = pat->newData[rowIdx];
  row.note       = rd[DIV_PAT_NOTE];
  row.instrument = rd[DIV_PAT_INS];
  row.volume     = rd[DIV_PAT_VOL];

  int effectCols = ss->pat[chan].effectCols;
  row.numEffects = effectCols;
  for (int i = 0; i < effectCols && i < 8; i++) {
    row.fx[i]    = rd[DIV_PAT_FX(i)];
    row.fxVal[i] = rd[DIV_PAT_FXVAL(i)];
  }
  return row;
}

// ---------------------------------------------------------------------------
// getOscData
// ---------------------------------------------------------------------------

FurnaceOscData FurnacePlayer::getOscData(int chan) const {
  FurnaceOscData osc{};
  osc.valid = false;

  if (!songLoaded || chan < 0) return osc;

  DivDispatchOscBuffer* buf = engine->getOscBuffer(chan);
  if (!buf) return osc;

  osc.data    = buf->data;
  osc.readPos = buf->readNeedle;
  osc.rate    = buf->rate;
  osc.valid   = true;
  return osc;
}

// ---------------------------------------------------------------------------
// ensureTimestamps
// ---------------------------------------------------------------------------

void FurnacePlayer::ensureTimestamps() {
  if (timestampsReady) return;
  if (!songLoaded || !engineReady) return;
  engine->calcSongTimestamps();
  timestampsReady = true;
}

// ---------------------------------------------------------------------------
// getDuration
// ---------------------------------------------------------------------------

double FurnacePlayer::getDuration() {
  if (!songLoaded) return 0.0;
  ensureTimestamps();
  int cur = static_cast<int>(engine->getCurrentSubSong());
  if (cur < 0 || cur >= static_cast<int>(engine->song.subsong.size())) return 0.0;
  return engine->song.subsong[cur]->ts.totalTime.toDouble();
}

// ---------------------------------------------------------------------------
// getTotalDuration
// ---------------------------------------------------------------------------

double FurnacePlayer::getTotalDuration() {
  if (!songLoaded) return 0.0;
  if (totalDurationCache >= 0.0) return totalDurationCache;

  int cur = static_cast<int>(engine->getCurrentSubSong());
  int count = static_cast<int>(engine->song.subsong.size());
  double total = 0.0;

  for (int i = 0; i < count; ++i) {
    // Switch to subsong to compute its timestamps, then switch back.
    engine->changeSongP(static_cast<size_t>(i));
    engine->calcSongTimestamps();
    total += engine->song.subsong[i]->ts.totalTime.toDouble();
  }

  // Restore original subsong
  engine->changeSongP(static_cast<size_t>(cur));
  timestampsReady = true;   // current subsong timestamps freshly computed

  totalDurationCache = total;
  return total;
}

// ---------------------------------------------------------------------------
// getPosition
// ---------------------------------------------------------------------------

double FurnacePlayer::getPosition() const {
  if (!songLoaded || !timestampsReady) return 0.0;
  int cur = static_cast<int>(engine->getCurrentSubSong());
  if (cur < 0 || cur >= static_cast<int>(engine->song.subsong.size())) return 0.0;

  int order = static_cast<int>(engine->getOrder());
  int row   = engine->getRow();
  TimeMicros pos = engine->song.subsong[cur]->ts.getTimes(order, row);
  if (pos.seconds < 0) return 0.0;   // row not touched yet
  return pos.toDouble();
}

// ---------------------------------------------------------------------------
// seek
// ---------------------------------------------------------------------------

bool FurnacePlayer::seek(double seconds) {
  if (!songLoaded || !engineReady) return false;
  ensureTimestamps();

  int cur = static_cast<int>(engine->getCurrentSubSong());
  if (cur < 0 || cur >= static_cast<int>(engine->song.subsong.size())) return false;

  DivSubSong* ss = engine->song.subsong[cur];
  double duration = ss->ts.totalTime.toDouble();
  if (seconds < 0.0) seconds = 0.0;
  if (duration > 0.0 && seconds > duration) seconds = duration;

  // Find the order whose start time is closest to (but not past) the target.
  int targetOrder = 0;
  for (int o = 0; o < ss->ordersLen; ++o) {
    TimeMicros t = ss->ts.getTimes(o, 0);
    if (t.seconds < 0) break;          // order not in timeline
    if (t.toDouble() > seconds) break;
    targetOrder = o;
  }

  engine->setOrder(static_cast<unsigned char>(targetOrder));
  return true;
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void FurnacePlayer::allocateFloatBuf(int frames) {
  if (frames <= floatBufSize) return;

  freeFloatBuf();

  floatBuf = new float*[2];
  floatBuf[0] = new float[frames];
  floatBuf[1] = new float[frames];
  floatBufSize = frames;
}

void FurnacePlayer::freeFloatBuf() {
  if (!floatBuf) return;
  delete[] floatBuf[0];
  delete[] floatBuf[1];
  delete[] floatBuf;
  floatBuf = nullptr;
  floatBufSize = 0;
}
