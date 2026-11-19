// Quick numerical checks for the optical compressor core.
// Build: g++ -std=c++17 -O2 -I../Source dsp_test.cpp -o dsp_test && ./dsp_test
#include "OptoCompressor.h"
#include <cstdio>
#include <vector>
#include <cmath>

using aa22::OptoCompressor;

static float runSine (OptoCompressor& comp, float ampDb, float seconds, float fs, float freq = 1000.0f)
{
    const int n = (int) (seconds * fs);
    std::vector<float> l (n), r (n);
    const float a = std::pow (10.0f, ampDb / 20.0f);
    for (int i = 0; i < n; ++i) l[i] = r[i] = a * std::sin (2.0f * 3.14159265f * freq * i / fs);
    float* ch[2] = { l.data(), r.data() };
    comp.process (ch, 2, n);
    float pk = 0; for (int i = n / 2; i < n; ++i) pk = std::max (pk, std::abs (l[i]));
    return 20.0f * std::log10 (pk + 1e-12f);
}

int main()
{
    const float fs = 48000.0f;
    int failures = 0;

    // 1) Peak reduction 0, gain 50 -> near-unity
    {
        OptoCompressor c; c.prepare (fs);
        OptoCompressor::Params p; p.peakReduction = 0; p.gain = 50; c.setParams (p); c.reset();
        float out = runSine (c, -12.0f, 1.0f, fs);
        std::printf ("Unity check: in -12 dB -> out %.2f dB, GR %.2f dB\n", out, c.getGainReductionDb());
        if (std::abs (out + 12.0f) > 0.5f) ++failures;
    }

    // 2) Static curve, Compress vs Limit
    for (int limit = 0; limit < 2; ++limit)
    {
        std::printf ("%s mode, PR=60:\n", limit ? "LIMIT" : "COMPRESS");
        for (float in = -40; in <= 0; in += 8)
        {
            OptoCompressor c; c.prepare (fs);
            OptoCompressor::Params p; p.peakReduction = 60; p.gain = 50; p.limit = limit; c.setParams (p); c.reset();
            float out = runSine (c, in, 2.0f, fs);
            std::printf ("  in %6.1f  out %6.2f  GR %5.2f\n", in, out, c.getGainReductionDb());
            if (! std::isfinite (out)) ++failures;
        }
    }

    // 3) Release: short burst vs long burst (program dependency)
    for (float burst : { 0.3f, 8.0f })
    {
        OptoCompressor c; c.prepare (fs);
        OptoCompressor::Params p; p.peakReduction = 70; c.setParams (p); c.reset();
        runSine (c, -6.0f, burst, fs);
        float gr0 = c.getGainReductionDb();
        float t50 = -1, t90 = -1;
        const int block = 48; // 1 ms
        std::vector<float> z (block, 0.0f); float* ch[2] = { z.data(), z.data() };
        for (int ms = 1; ms < 15000; ++ms)
        {
            std::fill (z.begin(), z.end(), 0.0f);
            c.process (ch, 1, block);
            float gr = c.getGainReductionDb();
            if (t50 < 0 && gr <= gr0 * 0.5f) t50 = ms;
            if (t90 < 0 && gr <= gr0 * 0.1f) { t90 = ms; break; }
        }
        std::printf ("Burst %.1fs: GR %.2f dB, 50%% release %.0f ms, 90%% release %.0f ms\n", burst, gr0, t50, t90);
    }

    // 4) Attack time to 90% of final GR
    {
        OptoCompressor c; c.prepare (fs);
        OptoCompressor::Params p; p.peakReduction = 70; c.setParams (p); c.reset();
        runSine (c, -6.0f, 0.5f, fs); float fin = c.getGainReductionDb();
        c.reset();
        int n = 0; float t = -1;
        for (int ms = 1; ms < 500; ++ms) { runSine (c, -6.0f, 0.001f, fs); if (c.getGainReductionDb() >= 0.9f * fin) { t = ms; break; } }
        (void) n;
        std::printf ("Attack to 90%% of %.1f dB GR: %.0f ms\n", fin, t);
    }

    std::printf (failures ? "FAILURES: %d\n" : "OK\n", failures);
    return failures;
}
