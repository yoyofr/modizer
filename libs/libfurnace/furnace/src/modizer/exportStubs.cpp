/**
 * Furnace Tracker - Modizer export stubs
 *
 * These stubs replace export-related DivEngine methods that are called from
 * engine internals (engine.cpp, platform/sid3.cpp) but whose full
 * implementations (wavOps.cpp, vgmOps.cpp, export*.cpp) are excluded from
 * the libfurnace build to reduce binary size.
 *
 * If a method is only ever called from GUI/CLI code (saveAudio, saveVGM,
 * buildROM, …), no stub is needed because those callers are not compiled in.
 */

#include "../engine/engine.h"
#include "../ta-utils.h"
#include "../ta-log.h"

// Defined in main.cpp in the furnace executable build.
// In the libfurnace build there is no GUI/OS dialog — just log the error.
void reportError(String what) {
  logE("%s", what.c_str());
}

// Called from platform/sid3.cpp to skip heavy processing during export.
// In a playback-only build there is never an export in progress.
// NOTE: registerROMExports() and isROMExportViable() are defined in exportDef.cpp.
bool DivEngine::isExporting() {
  return false;
}

// Stubs for the remaining public API symbols declared in engine.h.
// These are never called from code included in libfurnace; they exist only
// so that a consumer who accidentally references them gets a graceful no-op
// rather than a link error.

bool DivEngine::saveAudio(const char* path, DivAudioExportOptions options) {
  (void)path; (void)options;
  return false;
}

SafeWriter* DivEngine::saveVGM(bool* sysToExport, bool loop, int version,
                                bool patternHints, bool directStream,
                                int trailingTicks, bool dpcm07,
                                int correctedRate) {
  (void)sysToExport; (void)loop; (void)version; (void)patternHints;
  (void)directStream; (void)trailingTicks; (void)dpcm07; (void)correctedRate;
  return nullptr;
}

DivROMExport* DivEngine::buildROM(DivROMExportOptions sys) {
  (void)sys;
  return nullptr;
}

void DivEngine::waitAudioFile() {}
bool DivEngine::haltAudioFile() { return false; }
void DivEngine::finishAudioFile() {}
