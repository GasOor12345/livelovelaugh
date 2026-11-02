// Offline checks for the Live Love Laugh engine.
// Build: g++ -O2 -std=c++17 -I../Source tests/engine_test.cpp -o engine_test
#include "DSP/LLLEngine.h"
#include <cstdio>
#include <cmath>
#include <vector>

static int failures = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf ("FAIL: %s\n", msg); ++failures; } else std::printf ("ok:   %s\n", msg); } while (0)

struct Stats { float peak = 0, rms = 0; bool finite = true; float corrLR = 0; };

// Feeds a short synth-like burst, then silence; returns stats of the tail.
static Stats run (lll::Engine::Params p, double sr, float seconds, float burstSec = 0.3f,
                  float tailStart = 0.5f, bool powerOffAfterBurst = false)
{
    lll::Engine e;
    e.prepare (sr);
    const int block = 256;
    const int total = (int) (seconds * sr);
    std::vector<float> L (block), R (block);
    Stats st;
    double sumSq = 0, sLL = 0, sRR = 0, sLR = 0; long count = 0;
    float ph = 0;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        for (int i = 0; i < n; ++i)
        {
            const float t = (float) (pos + i) / (float) sr;
            float x = 0;
            if (t < burstSec) { ph += 440.0f / (float) sr; x = 0.5f * std::sin (2 * lll::kPi * ph) + 0.25f * std::sin (2 * lll::kPi * ph * 1.5f); }
            L[i] = x; R[i] = x;
        }
        auto q = p;
        if (powerOffAfterBurst && (float) pos / sr > burstSec) q.power = false;
        e.process (L.data(), R.data(), n, q);
        for (int i = 0; i < n; ++i)
        {
            if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) st.finite = false;
            st.peak = std::max (st.peak, std::max (std::abs (L[i]), std::abs (R[i])));
            if ((float) (pos + i) / sr > tailStart)
            {
                sumSq += 0.5 * (L[i] * L[i] + R[i] * R[i]);
                sLL += L[i] * L[i]; sRR += R[i] * R[i]; sLR += L[i] * R[i];
                ++count;
            }
        }
    }
    st.rms = (float) std::sqrt (sumSq / std::max (1L, count));
    st.corrLR = (float) (sLR / std::sqrt (std::max (1e-20, sLL * sRR)));
    return st;
}

int main()
{
    using P = lll::Engine::Params;

    { P p; p.live = 0.0f; p.love = 0.0f; auto s = run (p, 48000, 2.0f);
      CHECK (s.finite && s.rms < 1e-4f, "live=0 & love=0: no tail (dry only)"); }

    { P p; p.live = 0.5f; p.laugh = 0.4f; auto s = run (p, 48000, 2.0f);
      std::printf ("      live tail rms=%.4f\n", s.rms);
      CHECK (s.finite && s.rms > 1e-3f, "live=0.5: delay tail present"); }

    { P lo; lo.live = 0.15f; lo.laugh = 0.5f; auto a = run (lo, 48000, 2.0f);
      P hi; hi.live = 1.0f;  hi.laugh = 0.5f; auto b = run (hi, 48000, 2.0f);
      std::printf ("      L/R correlation low=%.3f high=%.3f\n", a.corrLR, b.corrLR);
      CHECK (b.corrLR < a.corrLR, "higher Live = wider stereo (lower L/R correlation)");
      CHECK (a.corrLR < 0.98f, "low Live already has a little stereo"); }

    { P p; p.live = 0.0f; p.love = 1.0f; p.laugh = 0.3f; auto s = run (p, 48000, 3.0f);
      std::printf ("      love tail rms=%.4f\n", s.rms);
      CHECK (s.finite && s.rms > 1e-3f, "love=1: granular tail present"); }

    { P p; p.live = 1.0f; p.love = 1.0f; p.laugh = 1.0f; p.timeIndex = 0; auto s = run (p, 48000, 20.0f, 2.0f);
      std::printf ("      max feedback peak=%.3f\n", s.peak);
      CHECK (s.finite && s.peak < 4.0f, "max everything, 20 s: stays bounded (limiter works)"); }

    for (int ti = 0; ti < lll::Engine::kNumTimes; ++ti)
    { P p; p.live = 0.8f; p.love = 0.8f; p.laugh = 0.9f; p.timeIndex = ti; p.bpm = 60; p.filter = (ti % 3 - 1) * 0.9f;
      auto s = run (p, 96000, 3.0f);
      char msg[96]; std::snprintf (msg, sizeof msg, "time %s @60bpm/96k with filter: finite", lll::Engine::noteName (ti));
      CHECK (s.finite, msg); }

    { P p; p.live = 0.6f; p.laugh = 0.5f; p.duck = 0.0f; auto a = run (p, 48000, 1.0f, 1.0f, 0.4f);
      p.duck = 1.0f; auto b = run (p, 48000, 1.0f, 1.0f, 0.4f);
      p.live = 0.0f; auto dry = run (p, 48000, 1.0f, 1.0f, 0.4f);
      const float wetA = a.rms - dry.rms, wetB = b.rms - dry.rms;
      std::printf ("      wet over dry while playing: duck0=%.4f duck1=%.4f\n", wetA, wetB);
      CHECK (wetB < wetA * 0.5f, "ducking lowers the wet signal while input plays"); }

    { P p; p.live = 0.6f; p.laugh = 0.6f; p.killDry = true; p.timeIndex = 2;
      auto s = run (p, 48000, 0.2f, 0.2f, 0.0f);
      // before the first repeat (1/8 @120 = 250 ms) output should be ~silent
      CHECK (s.finite && s.peak < 0.05f, "kill dry: no dry signal before first echo"); }

    { P p; p.live = 0.6f; p.laugh = 0.6f; auto s = run (p, 48000, 2.0f, 0.3f, 0.5f, true);
      std::printf ("      tail after power off rms=%.4f\n", s.rms);
      CHECK (s.rms > 1e-3f, "power off keeps trails ringing"); }

    std::printf ("\n%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
    return failures ? 1 : 0;
}
