// 22AA comp — optical leveling amplifier model
// Pure C++ (no JUCE dependency) so it can be unit-tested on its own.
//
// Signal flow (per sample, stereo-linked):
//   in -> [sidechain: HF emphasis -> RMS detector -> gain computer -> T4 opto cell]
//      -> gain reduction -> tube/transformer colour -> make-up gain -> out
//
// The T4 opto cell is modelled with two coupled envelopes:
//   * a fast part (≈10 ms attack, ≈60 ms release) — the electroluminescent panel
//   * a slow "memory" part that charges while the cell is lit and releases over
//     ~0.5 s .. 5 s. The longer and harder you compress, the slower the release,
//     which is the program-dependent behaviour the original hardware is known for.
#pragma once

#include <algorithm>
#include <cmath>

namespace aa22
{

class OptoCompressor
{
public:
    struct Params
    {
        float peakReduction = 0.0f;  // 0..100
        float gain          = 50.0f; // 0..100 (50 = unity)
        bool  limit         = false; // false = Compress, true = Limit
        float emphasis      = 0.0f;  // 0..100 sidechain HF emphasis ("R37" trim)
    };

    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        rmsCoef      = coefFromMs (5.0f);
        fastAttack   = coefFromMs (10.0f);
        fastRelease  = coefFromMs (60.0f);
        slowAttack   = coefFromMs (400.0f);
        memoryCharge = coefFromMs (2000.0f);
        memoryDecay  = coefFromMs (8000.0f);
        smoothCoef   = coefFromMs (20.0f);
        dcCoef       = 1.0f - (2.0f * 3.14159265f * 8.0f / fs); // ~8 Hz DC blocker
        reset();
        updateEmphasisFilter();
    }

    void reset()
    {
        rms = 0.0f; fastEnv = 0.0f; slowEnv = 0.0f; memory = 0.0f;
        grDb = 0.0f;
        for (int c = 0; c < 2; ++c) { hpState[c] = 0.0f; dcX[c] = 0.0f; dcY[c] = 0.0f; }
        smoothedMakeup = targetMakeup;
        smoothedThreshold = targetThreshold;
    }

    void setParams (const Params& p)
    {
        // Peak Reduction acts like sidechain gain: higher = lower threshold.
        targetThreshold = 10.0f - std::clamp (p.peakReduction, 0.0f, 100.0f) * 0.55f; // +10 .. -45 dBFS
        targetMakeup    = (std::clamp (p.gain, 0.0f, 100.0f) - 50.0f) * 0.4f;          // -20 .. +20 dB
        if (p.limit) { ratio = 20.0f; knee = 6.0f; }
        else         { ratio = 3.0f;  knee = 14.0f; }   // very soft knee, gentle ratio

        if (std::abs (p.emphasis - emphasis) > 0.01f)
        {
            emphasis = p.emphasis;
            updateEmphasisFilter();
        }
    }

    // Process in place. numChannels: 1 or 2.
    void process (float* const* channels, int numChannels, int numSamples)
    {
        numChannels = std::min (numChannels, 2);
        if (numChannels <= 0) return;

        float peakOut = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            smoothedMakeup    += smoothCoef * (targetMakeup - smoothedMakeup);
            smoothedThreshold += smoothCoef * (targetThreshold - smoothedThreshold);

            // ---- Sidechain (stereo linked) ----
            float scPower = 0.0f;
            for (int c = 0; c < numChannels; ++c)
            {
                const float x = channels[c][i];
                // One-pole high-pass, mixed back in = high-frequency shelf boost.
                hpState[c] += hpCoef * (x - hpState[c]);
                const float hf = x - hpState[c];
                const float sc = x + emphasisAmount * hf;
                scPower = std::max (scPower, sc * sc);
            }
            rms += rmsCoef * (scPower - rms);
            const float levelDb = 10.0f * std::log10 (rms + 1.0e-12f) + 3.0f; // RMS of a sine -> peak-ish

            // ---- Gain computer (soft knee) ----
            const float over = levelDb - smoothedThreshold;
            float target = 0.0f;
            const float slope = 1.0f - 1.0f / ratio;
            if (over <= -knee * 0.5f)       target = 0.0f;
            else if (over >= knee * 0.5f)   target = over * slope;
            else { const float t = over + knee * 0.5f; target = slope * t * t / (2.0f * knee); }

            // ---- T4 optical cell ----
            // Fast component (EL panel + fast part of the photocell)
            fastEnv += (target > fastEnv ? fastAttack : fastRelease) * (target - fastEnv);

            // Memory: charges while compressing, decays slowly when idle.
            const float lit = std::min (target / 6.0f, 1.0f);
            memory += (lit > memory ? memoryCharge : memoryDecay) * (lit - memory);

            // Slow component: release time scales 0.5 s .. 5 s with memory.
            const float slowRelMs = 500.0f + 4500.0f * memory;
            const float slowRel = coefFromMs (slowRelMs);
            slowEnv += (target > slowEnv ? slowAttack : slowRel) * (target - slowEnv);

            grDb = std::max (fastEnv, 0.45f * slowEnv);

            const float grGain     = dbToGain (-grDb);
            const float makeupGain = dbToGain (smoothedMakeup);

            // ---- Apply + colour ----
            for (int c = 0; c < numChannels; ++c)
            {
                float y = channels[c][i] * grGain;
                y = colour (y, c);
                y *= makeupGain;
                channels[c][i] = y;
                peakOut = std::max (peakOut, std::abs (y));
            }
        }

        lastPeakOut = peakOut;
    }

    float getGainReductionDb() const { return grDb; }
    float getLastPeakOut() const     { return lastPeakOut; }

private:
    // Gentle tube/transformer style saturation: mostly 2nd harmonic at normal
    // levels, smooth soft clipping when driven hard. DC from the even term is removed.
    float colour (float x, int c)
    {
        constexpr float drive = 0.06f;
        float y = x + drive * x * x;
        y = y / (1.0f + std::abs (y) * 0.08f);  // soft ceiling, transparent at low level
        // DC blocker
        const float out = y - dcX[c] + dcCoef * dcY[c];
        dcX[c] = y; dcY[c] = out;
        return out;
    }

    void updateEmphasisFilter()
    {
        // Emphasis: 0 = flat sidechain, 100 = strong boost above ~2 kHz
        const float fc = 2000.0f;
        hpCoef = 1.0f - std::exp (-2.0f * 3.14159265f * fc / fs);
        emphasisAmount = std::clamp (emphasis, 0.0f, 100.0f) / 100.0f * 3.0f; // up to ~+12 dB
    }

    float coefFromMs (float ms) const { return 1.0f - std::exp (-1.0f / (0.001f * ms * fs)); }
    static float dbToGain (float db)  { return std::pow (10.0f, db * 0.05f); }

    float fs = 44100.0f;
    float rmsCoef = 0, fastAttack = 0, fastRelease = 0, slowAttack = 0;
    float memoryCharge = 0, memoryDecay = 0, smoothCoef = 0, dcCoef = 0.999f;

    float rms = 0, fastEnv = 0, slowEnv = 0, memory = 0, grDb = 0;
    float hpState[2] {}, dcX[2] {}, dcY[2] {};
    float hpCoef = 0, emphasisAmount = 0, emphasis = 0;

    float ratio = 3.0f, knee = 14.0f;
    float targetThreshold = 10.0f, smoothedThreshold = 10.0f;
    float targetMakeup = 0.0f, smoothedMakeup = 0.0f;
    float lastPeakOut = 0.0f;
};

} // namespace aa22
