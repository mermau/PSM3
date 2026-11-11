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

// Deriva lenta: rumore passa-basso a frequenza bassissima, uscita circa in [-1,1].
struct SlowDrift
{
    Rng rng; double y1 = 0.0, y2 = 0.0, coeff = 0.0; bool enabled = false;
    void prepare(double sr, uint32_t seedValue, double cornerHz = 0.05)
    {
        enabled = seedValue != 0;
        rng.seed(seedValue * 2654435761u + 1u);
        coeff = 1.0 - std::exp(-kTwoPi * cornerHz / sr);
        // partenza da un valore casuale, cosi' ogni dispositivo e' gia' "fuori taratura"
        y1 = y2 = enabled ? rng.bipolar() * 0.5 : 0.0;
        gain = 1.0 / std::sqrt(std::max(1e-12, coeff));
    }
    double process()
    {
        if (! enabled) return 0.0;
        y1 += coeff * (rng.bipolar() * gain * 0.6 - y1);
        y2 += coeff * (y1 - y2);
        return std::max(-1.0, std::min(1.0, y2));
    }
    double gain = 1.0;
};
}
