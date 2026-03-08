//
//  DynamicNormalizer.h
//  modizer
//
//  Real-time dynamic volume normalizer for stereo int16 audio.
//  Algorithm: RMS-based AGC with smooth attack/release + soft limiter.
//  No lookahead required - fully real-time capable.
//

#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>

class DynamicNormalizer {
public:

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------
    struct Config {
        float sampleRate        = 44100.0f; // Hz

        // Target loudness (RMS). -18 dBFS is a good starting point:
        // loud enough but leaves plenty of headroom.
        float targetLevelDb     = -18.0f;

        // How fast the gain rises when signal is quiet (ms).
        // Fast = sounds louder sooner, but can amplify transient noise.
        float attackTimeMs      = 20.0f;

        // How fast the gain falls when signal is loud (ms).
        // Slow = smoother, preserves dynamics. Too slow = clipping on peaks.
        float releaseTimeMs     = 1200.0f;

        // Maximum amplification allowed (dB). Prevents over-boosting silence.
        float maxGainDb         = 24.0f;

        // Minimum gain (dB). Allows a bit of attenuation for very loud tracks.
        float minGainDb         = -12.0f;

        // RMS integration window (ms). Controls how "quickly" loudness is measured.
        // ~200ms is perceptually natural (similar to ITU BS.1770 short-term).
        float rmsWindowMs       = 200.0f;

        // Soft limiter knee start (dBFS). Everything above this is gently squashed
        // before the hard int16 clip. Use -1.0 to -3.0 dBFS.
        float limiterThresholdDb = -2.0f;
    };

    // -----------------------------------------------------------------------
    // Constructor / destructor
    // -----------------------------------------------------------------------
    DynamicNormalizer() {
        setConfig(Config{});
        reset();
    }
    explicit DynamicNormalizer(const Config& cfg) {
        setConfig(cfg);
        reset();
    }

    // Reconfigure at runtime (e.g. after a sample-rate change).
    void setConfig(const Config& cfg) {
        m_cfg = cfg;

        // Pre-compute time constants as per-sample exponential coefficients.
        m_attackCoeff  = expTimeConst(cfg.attackTimeMs,  cfg.sampleRate);
        m_releaseCoeff = expTimeConst(cfg.releaseTimeMs, cfg.sampleRate);
        m_rmsCoeff     = expTimeConst(cfg.rmsWindowMs,   cfg.sampleRate);

        // Convert dB limits to linear gain.
        m_maxGain      = std::pow(10.0f, cfg.maxGainDb  / 20.0f);
        m_minGain      = std::pow(10.0f, cfg.minGainDb  / 20.0f);
        m_targetRms    = std::pow(10.0f, cfg.targetLevelDb / 20.0f);  // linear RMS target
        m_limiterThres = std::pow(10.0f, cfg.limiterThresholdDb / 20.0f); // as fraction of 1.0
    }

    // Reset internal state (call on song change).
    void reset() {
        m_rmsEst     = 0.0f;
        m_currentGain = 1.0f;
    }

    // -----------------------------------------------------------------------
    // Processing
    //
    // buffer : interleaved stereo int16 (L, R, L, R, ...)
    // numFrames : number of stereo frames (NOT total samples)
    //
    // Processes in-place.
    // -----------------------------------------------------------------------
    void process(int16_t* buffer, int numFrames) {
        constexpr float kInt16Max   = 32767.0f;
        constexpr float kInt16Scale = 1.0f / kInt16Max; // normalize to [-1, +1]

        for (int i = 0; i < numFrames; ++i) {
            const float L = buffer[i * 2    ] * kInt16Scale;
            const float R = buffer[i * 2 + 1] * kInt16Scale;

            // --- 1. Estimate RMS via exponential moving average of squared amplitude ---
            const float power = 0.5f * (L * L + R * R);
            m_rmsEst = m_rmsCoeff * m_rmsEst + (1.0f - m_rmsCoeff) * power;
            const float rms = std::sqrt(m_rmsEst + 1e-12f); // +epsilon avoids sqrt(0)

            // --- 2. Compute desired gain ---
            const float desiredGain = clampf(m_targetRms / rms, m_minGain, m_maxGain);

            // --- 3. Smooth gain with asymmetric attack / release ---
            if (desiredGain < m_currentGain) {
                // Signal got louder -> reduce gain quickly (release direction)
                m_currentGain = m_releaseCoeff * m_currentGain
                              + (1.0f - m_releaseCoeff) * desiredGain;
            } else {
                // Signal got quieter -> increase gain slowly (attack direction)
                m_currentGain = m_attackCoeff  * m_currentGain
                              + (1.0f - m_attackCoeff)  * desiredGain;
            }

            // --- 4. Apply gain ---
            float outL = L * m_currentGain;
            float outR = R * m_currentGain;

            // --- 5. Soft limiter (avoids hard clipping) ---
            outL = softLimit(outL);
            outR = softLimit(outR);

            // --- 6. Convert back to int16 ---
            buffer[i * 2    ] = static_cast<int16_t>(clampf(outL * kInt16Max, -kInt16Max, kInt16Max));
            buffer[i * 2 + 1] = static_cast<int16_t>(clampf(outR * kInt16Max, -kInt16Max, kInt16Max));
        }
    }

    // -----------------------------------------------------------------------
    // Diagnostic helpers
    // -----------------------------------------------------------------------
    float getCurrentGainDb() const {
        return 20.0f * std::log10(m_currentGain + 1e-12f);
    }

    float getCurrentRmsDb() const {
        return 20.0f * std::log10(std::sqrt(m_rmsEst + 1e-12f));
    }

private:

    // exp(-1 / (ms * sampleRate / 1000)) - time constant for smoothing filters.
    static inline float expTimeConst(float ms, float sr) {
        return std::exp(-1.0f / (ms * 0.001f * sr));
    }

    // C++14-compatible clamp.
    static inline float clampf(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    // Soft limiter using cubic saturation above the threshold.
    // Keeps samples just below 1.0 with a smooth knee instead of hard clipping.
    inline float softLimit(float x) const {
        const float absX = std::abs(x);
        if (absX <= m_limiterThres) return x;

        // Cubic soft knee above threshold:
        // Maps [threshold, +inf) -> [threshold, 1.0) smoothly.
        const float sign  = (x >= 0.0f) ? 1.0f : -1.0f;
        const float over  = (absX - m_limiterThres) / (1.0f - m_limiterThres);
        // over is in [0, +inf). Apply soft saturation: tanh-like via cubic clamp.
        const float clamped = over / (1.0f + over); // 0..1 asymptote
        return sign * (m_limiterThres + (1.0f - m_limiterThres) * clamped);
    }

    Config m_cfg;

    // Per-sample smoothing coefficients (pre-computed).
    float m_attackCoeff   = 0.0f;
    float m_releaseCoeff  = 0.0f;
    float m_rmsCoeff      = 0.0f;

    // Linear equivalents of dB config values.
    float m_maxGain       = 1.0f;
    float m_minGain       = 1.0f;
    float m_targetRms     = 0.0f;
    float m_limiterThres  = 1.0f;

    // Per-sample state.
    float m_rmsEst        = 0.0f;  // exponential moving average of power
    float m_currentGain   = 1.0f;  // smoothed applied gain
};
