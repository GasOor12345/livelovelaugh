// Live Love Laugh - audio engine
// Plain C++ (no JUCE), so it can be tested on its own.
//
// Signal flow (as designed):
//
//  in ─┬────────────────────────────────────────────► dry ───────────────┐
//      ├─► LIVE  pingpong delay + chorus + shimmer ◄─ feedback loop ─┐    ├─► out
//      │         (Laugh amount, DJ filter, soft limiter)             ├─► ducking
//      └─► LOVE  granular reverse-octave delay ◄─ feedback loop ─────┤
//                (Laugh amount, DJ filter, soft limiter)             │
//                └── part of LOVE output ──► LIVE input ─────────────┘
//
// Delay Time (tempo synced) drives both LIVE and LOVE.
// Ducking: the wet signal gets quieter while the dry input is playing.
// Power off = input stops feeding the effects, but the tails ring out ("trails").

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace lll
{
constexpr float kPi = 3.14159265358979f;

// Smooth saturating "limiter" curve (rational tanh approximation). |out| <= 1.
inline float softClip (float x)
{
    if (x > 3.0f) return 1.0f;
    if (x < -3.0f) return -1.0f;
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

inline float smoothstep (float a, float b, float x)
{
    const float t = std::clamp ((x - a) / (b - a), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// One-pole parameter smoother (avoids clicks/zipper noise when knobs move).
struct Smoother
{
    float y = 0.0f, a = 0.0f;
    void setTime (float sampleRate, float seconds) { a = std::exp (-1.0f / (sampleRate * seconds)); }
    void reset (float v) { y = v; }
    float next (float target) { y = target + a * (y - target); return y; }
};

// Circular buffer with fractional (linearly interpolated) reads.
struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, w = 0;

    void init (int minSize)
    {
        int n = 1;
        while (n < minSize) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }

    // d = how many samples ago (1 = most recently pushed sample).
    float read (float d) const
    {
        const float pos = (float) w - d;
        const int i0 = (int) std::floor (pos);
        const float fr = pos - (float) i0;
        const float a = buf[(size_t) (i0 & mask)];
        const float b = buf[(size_t) ((i0 + 1) & mask)];
        return a + fr * (b - a);
    }
};

// Topology-preserving state variable filter (low-pass and high-pass outputs).
struct SVF
{
    float ic1 = 0, ic2 = 0, k = 1.4142f, a1 = 1, a2 = 0, a3 = 0;
    void set (float sr, float fc, float q)
    {
        fc = std::clamp (fc, 10.0f, sr * 0.45f);
        const float g = std::tan (kPi * fc / sr);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() { ic1 = ic2 = 0; }
    void process (float v0, float& lp, float& hp)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        hp = v0 - k * v1 - v2;
    }
};

// DJ-style filter: -1 = dark (low-pass), 0 = open, +1 = thin (high-pass).
struct DJFilter
{
    SVF svf;
    float mode = 0.0f;
    void set (float sr, float amount)
    {
        mode = amount;
        if (amount < 0.0f)
            svf.set (sr, 20000.0f * std::pow (300.0f / 20000.0f, -amount), 0.8f);
        else
            svf.set (sr, 20.0f * std::pow (150.0f, amount), 0.8f);
    }
    void reset() { svf.reset(); }
    float process (float x)
    {
        float lp, hp;
        svf.process (x, lp, hp);
        return mode < 0.0f ? lp : hp;
    }
};

// Removes DC offset build-up inside feedback loops.
struct DCBlock
{
    float x1 = 0, y1 = 0, r = 0.995f;
    void reset() { x1 = y1 = 0; }
    float process (float x) { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
};

// Simple +12 semitone pitch shifter (two crossfaded sweeping taps) for the shimmer.
struct OctaveUp
{
    DelayLine d;
    float phase = 0.0f, window = 1.0f;
    void init (float sr)
    {
        window = 0.07f * sr;
        d.init ((int) (window * 2.0f) + 16);
        phase = 0.0f;
    }
    void reset() { d.clear(); phase = 0.0f; }
    float process (float x)
    {
        d.push (x);
        phase += 1.0f / window;
        if (phase >= 1.0f) phase -= 1.0f;
        float p2 = phase + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;
        float g1 = std::sin (kPi * phase); g1 *= g1;
        float g2 = std::sin (kPi * p2);    g2 *= g2;
        return d.read (2.0f + (1.0f - phase) * window) * g1
             + d.read (2.0f + (1.0f - p2) * window) * g2;
    }
};

struct Rng
{
    uint32_t s = 0x9E3779B9u;
    float uni() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xFFFFFF) / 16777216.0f; }
    float bi() { return uni() * 2.0f - 1.0f; }
};

class Engine
{
public:
    struct Params
    {
        float live = 0.35f;   // 0..1
        float love = 0.0f;    // 0..1
        float laugh = 0.4f;   // 0..1 feedback
        float filter = 0.0f;  // -1 (LP) .. 0 .. +1 (HP)
        float duck = 0.0f;    // 0..1
        int timeIndex = 2;    // see noteName()
        bool killDry = false;
        bool power = true;
        double bpm = 120.0;
    };

    static constexpr int kNumTimes = 8;
    static double noteBeats (int i)
    {
        static const double beats[kNumTimes] = { 0.125, 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0 };
        return beats[std::clamp (i, 0, kNumTimes - 1)];
    }
    static const char* noteName (int i)
    {
        static const char* names[kNumTimes] = { "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars" };
        return names[std::clamp (i, 0, kNumTimes - 1)];
    }

    static constexpr float kMaxDelaySeconds = 16.0f;

    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        const int maxDelay = (int) (kMaxDelaySeconds * sr);
        for (auto* d : { &liveL, &liveR }) d->init (maxDelay + (int) (0.03f * sr) + 8);
        maxGrain = 0.35f * sr;
        for (auto* d : { &loveL, &loveR }) d->init (maxDelay + (int) (4.0f * maxGrain) + 16);
        octL.init (sr); octR.init (sr);

        for (auto* s : { &sLive, &sLove, &sLaugh, &sFilter, &sDuck, &sDry })
            s->setTime (sr, 0.03f);
        sSend.setTime (sr, 0.01f);
        sDelay.setTime (sr, 0.12f);
        sDuckGain.setTime (sr, 0.005f);
        envAtt = std::exp (-1.0f / (0.005f * sr));
        envRel = std::exp (-1.0f / (0.30f * sr));
        reset();
        firstBlock = true;
    }

    void reset()
    {
        for (auto* d : { &liveL, &liveR, &loveL, &loveR }) d->clear();
        octL.reset(); octR.reset();
        for (auto* f : { &fLiveL, &fLiveR, &fLoveL, &fLoveR }) f->reset();
        for (auto* d : { &dcLiveL, &dcLiveR, &dcLoveL, &dcLoveR }) d->reset();
        for (auto& g : grains) g.active = false;
        loveWriteCount = 0;
        grainTimer = 0.0f;
        loveOutL = loveOutR = 0.0f;
        lfo = 0.0f;
        env = 0.0f;
    }

    static float delaySamplesFor (const Params& p, float sampleRate)
    {
        const double bpm = std::clamp (p.bpm, 20.0, 400.0);
        const double sec = noteBeats (p.timeIndex) * 60.0 / bpm;
        return (float) std::clamp (sec * sampleRate, 64.0, (double) (kMaxDelaySeconds * sampleRate));
    }

    // In-place stereo processing.
    void process (float* L, float* R, int n, const Params& p)
    {
        const float targetDelay = delaySamplesFor (p, sr);
        const float targetSend = p.power ? 1.0f : 0.0f;
        const float targetDry = (! p.power || ! p.killDry) ? 1.0f : 0.0f;

        if (firstBlock)
        {
            sLive.reset (p.live); sLove.reset (p.love); sLaugh.reset (p.laugh);
            sFilter.reset (p.filter); sDuck.reset (p.duck); sDry.reset (targetDry);
            sSend.reset (targetSend); sDelay.reset (targetDelay); sDuckGain.reset (1.0f);
            firstBlock = false;
        }

        for (int i = 0; i < n; ++i)
        {
            const float live = sLive.next (p.live);
            const float love = sLove.next (p.love);
            const float laugh = sLaugh.next (p.laugh);
            const float filt = sFilter.next (p.filter);
            const float duck = sDuck.next (p.duck);
            const float dryG = sDry.next (targetDry);
            const float send = sSend.next (targetSend);
            const float D = sDelay.next (targetDelay);

            if ((coefCounter++ & 15) == 0)
                for (auto* f : { &fLiveL, &fLiveR, &fLoveL, &fLoveR }) f->set (sr, filt);

            const float inL = L[i], inR = R[i];
            const float fb = laugh * 0.95f;

            // ---------- LOVE: granular reverse octave ----------
            const float loveToLive = love * 0.5f;
            {
                const float wL = softClip (send * inL + fb * dcLoveL.process (fLoveL.process (loveOutL)));
                const float wR = softClip (send * inR + fb * dcLoveR.process (fLoveR.process (loveOutR)));
                loveL.push (wL); loveR.push (wR);
                ++loveWriteCount;

                const float grainLen = std::clamp (D * 0.5f, 0.04f * sr, maxGrain);
                grainTimer -= 1.0f;
                if (grainTimer <= 0.0f)
                {
                    spawnGrain (D, grainLen);
                    grainTimer = grainLen * 0.5f * (0.85f + 0.3f * rng.uni());
                }

                float gl = 0.0f, gr = 0.0f;
                for (auto& g : grains)
                {
                    if (! g.active) continue;
                    const float back = (float) (loveWriteCount - g.pos); // samples ago
                    const float s = 0.5f * (loveL.read (back) + loveR.read (back));
                    float e = std::sin (kPi * g.t); e *= e;
                    gl += s * e * g.gainL;
                    gr += s * e * g.gainR;
                    g.pos -= 2.0;                 // reverse + octave up
                    g.t += g.dt;
                    if (g.t >= 1.0f) g.active = false;
                }
                loveOutL = gl; loveOutR = gr;
            }

            // ---------- LIVE: pingpong delay, chorus, shimmer ----------
            float liveOutL, liveOutR;
            {
                const float pp = live;                                 // pingpong amount
                const float chorus = smoothstep (0.55f, 1.0f, live);
                const float shimmer = 0.4f * smoothstep (0.7f, 1.0f, live);

                const float mono = 0.5f * (inL + inR);
                const float xL = send * ((1.0f - pp) * inL + pp * mono) + loveToLive * loveOutL;
                const float xR = send * ((1.0f - pp) * inR) + loveToLive * loveOutR;

                lfo += 0.45f / sr;
                if (lfo >= 1.0f) lfo -= 1.0f;
                const float depth = chorus * 0.0025f * sr;
                const float mL = depth * (0.5f + 0.5f * std::sin (2.0f * kPi * lfo));
                const float mR = depth * (0.5f - 0.5f * std::sin (2.0f * kPi * lfo));

                // A small L/R time offset gives a bit of stereo even at low settings.
                const float spread = 0.004f * sr * (0.4f + 0.6f * live);
                float yL = liveL.read (D + mL);
                float yR = liveR.read (D + mR + spread);

                const float shL = octL.process (yL);
                const float shR = octR.process (yR);
                yL = (1.0f - 0.5f * shimmer) * yL + shimmer * shL;
                yR = (1.0f - 0.5f * shimmer) * yR + shimmer * shR;

                const float cL = (1.0f - pp) * yL + pp * yR;
                const float cR = (1.0f - pp) * yR + pp * yL;
                const float fL = dcLiveL.process (fLiveL.process (cL));
                const float fR = dcLiveR.process (fLiveR.process (cR));

                liveL.push (softClip (xL + fb * fL));
                liveR.push (softClip (xR + fb * fR));

                // Stereo width grows with the Live knob.
                const float width = 0.5f + live;
                const float m = 0.5f * (yL + yR), sd = 0.5f * (yL - yR) * width;
                liveOutL = m + sd;
                liveOutR = m - sd;
            }

            // ---------- Mix, ducking ----------
            const float liveGain = std::pow (live, 0.8f);
            float wetL = liveGain * liveOutL + love * loveOutL;
            float wetR = liveGain * liveOutR + love * loveOutR;
            wetL = softClip (wetL * 0.8f) * 1.25f;
            wetR = softClip (wetR * 0.8f) * 1.25f;

            const float lvl = std::max (std::abs (inL), std::abs (inR));
            env = lvl > env ? lvl + envAtt * (env - lvl) : lvl + envRel * (env - lvl);
            const float duckGain = sDuckGain.next (1.0f / (1.0f + duck * 12.0f * env));

            L[i] = dryG * inL + duckGain * wetL;
            R[i] = dryG * inR + duckGain * wetR;
        }
    }

private:
    struct Grain
    {
        bool active = false;
        double pos = 0;       // absolute read position in the love buffer
        float t = 0, dt = 0;  // envelope phase 0..1
        float gainL = 1, gainR = 1;
    };

    void spawnGrain (float D, float grainLen)
    {
        for (auto& g : grains)
        {
            if (g.active) continue;
            g.active = true;
            const float jitter = rng.uni() * grainLen * 0.5f;
            g.pos = (double) loveWriteCount - (double) (D + jitter);
            g.t = 0.0f;
            g.dt = 1.0f / grainLen;
            const float pan = 0.8f * rng.bi();
            g.gainL = std::sqrt (1.0f - pan);
            g.gainR = std::sqrt (1.0f + pan);
            return;
        }
    }

    float sr = 44100.0f, maxGrain = 1.0f;
    DelayLine liveL, liveR, loveL, loveR;
    OctaveUp octL, octR;
    DJFilter fLiveL, fLiveR, fLoveL, fLoveR;
    DCBlock dcLiveL, dcLiveR, dcLoveL, dcLoveR;
    Smoother sLive, sLove, sLaugh, sFilter, sDuck, sDry, sSend, sDelay, sDuckGain;
    Grain grains[8];
    Rng rng;
    int64_t loveWriteCount = 0;
    float grainTimer = 0.0f, loveOutL = 0.0f, loveOutR = 0.0f;
    float lfo = 0.0f, env = 0.0f, envAtt = 0.0f, envRel = 0.0f;
    unsigned coefCounter = 0;
    bool firstBlock = true;
};

} // namespace lll
