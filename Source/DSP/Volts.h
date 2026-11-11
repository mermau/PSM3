// PSM-3 — costanti elettriche e utilità comuni del motore.
// Tutto il motore lavora in volt reali, come lo strumento originale.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace psm3
{
constexpr double kVPos = 12.0;              // alimentazione positiva stabilizzata
constexpr double kVNeg = -9.0;              // alimentazione negativa stabilizzata
constexpr double kVoltsToFullScale = 0.5;   // 2 V p-p di Signal Output = -6 dBFS
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

inline double clampRails(double v) { return std::min(kVPos, std::max(kVNeg, v)); }

// Manopola 0...10 -> 0...1
inline double dialNorm(double dial) { return std::min(1.0, std::max(0.0, dial * 0.1)); }

// Azzera NaN/Inf. Ritorna true se il valore non era finito.
inline bool sanitize(double& v)
{
    if (std::isfinite(v)) return false;
    v = 0.0;
    return true;
}

// Generatore pseudo-casuale piccolo e deterministico (xorshift32).
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    void seed(uint32_t v) { s = v ? v : 0x9E3779B9u; }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    double uniform() { return (double) next() * (1.0 / 4294967296.0); }      // [0,1)
    double bipolar() { return uniform() * 2.0 - 1.0; }                        // [-1,1)
    double gauss()                                                            // media 0, sigma 1
    {
        double a = 0.0;
        for (int i = 0; i < 6; ++i) a += uniform();
        return (a - 3.0) * 1.41421356;
    }
};

// Deriva lenta e limitata in [-1,1]: somma di tre sinusoidi lentissime con fasi e
// frequenze ricavate dal seme. seed 0 = nessuna deriva.
struct SlowDrift
{
    double phase[3] = { 0, 0, 0 }, inc[3] = { 0, 0, 0 };
    bool enabled = false;
    void prepare(double sr, uint32_t seedValue)
    {
        enabled = seedValue != 0;
        Rng rng; rng.seed(seedValue * 2654435761u + 12345u);
        static const double base[3] = { 0.031, 0.083, 0.19 };      // Hz
        for (int i = 0; i < 3; ++i)
        {
            phase[i] = rng.uniform() * kTwoPi;
            inc[i] = kTwoPi * base[i] * (0.7 + 0.6 * rng.uniform()) / sr;
        }
    }
    double process()
    {
        if (! enabled) return 0.0;
        double v = 0.0;
        static const double w[3] = { 0.5, 0.3, 0.2 };
        for (int i = 0; i < 3; ++i)
        {
            phase[i] += inc[i]; if (phase[i] > kTwoPi) phase[i] -= kTwoPi;
            v += w[i] * std::sin(phase[i]);
        }
        return v;
    }
};
}
