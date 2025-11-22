/**
 * @file Constants.hpp
 * @brief Holds Milkdrop constants like number of Q variables etc.
 */
#pragma once

namespace libprojectM {
namespace MilkdropPreset {

static constexpr int QVarCount = 64; //!< Number of Q variables available.
static constexpr int TVarCount = 8; //!< Number of T variables available.

static constexpr int CustomWaveformCount = 16; //!< Number of custom waveforms (expression-driven) which can be used in a preset.
static constexpr int CustomShapeCount = 16; //!< Number of custom shapes (expression-driven) which can be used in a preset.

static constexpr int WaveformMaxPoints = 512; //!< Maximum number of waveform points.

} // namespace MilkdropPreset
} // namespace libprojectM
