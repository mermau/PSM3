// PSM-3 — Modulatore ad anello a transistor (doppio bilanciamento), accoppiato in continua.
#pragma once
#include "Volts.h"

namespace psm3
{
class RingModulator
{
public:
    void prepare(double sampleRate) { sr = sampleRate; drift.prepare(sr, driftSeed); }
    void reset() { drift.prepare(sr, driftSeed); }
    void setLevel(double d) { level = dialNorm(d); }
    void setDriftSeed(uint32_t s) { driftSeed = s; drift.prepare(sr, s); }
    double process(double a, double b)
    {
        // Ingressi lineari fino a circa 1,5 V p-p, poi saturazione dolce delle coppie differenziali.
        const double sa = std::tanh(a), sb = std::tanh(b);
        double v = kGain * sa * sb;
        const double q = v / 3.0;
        v /= std::sqrt(std::sqrt(1.0 + q * q * q * q));           // non oltre circa 6 V p-p
        // Residui: l'ingresso B e' stabile (-62 dB), l'ingresso A deriva con la temperatura (-64...-54 dB),
        // piu' un po' di seconda armonica di A (PR9).
        const double leakA = 0.0012 + 0.0007 * drift.process();
        v += leakA * a + 0.0008 * b + 0.0006 * sa * sa;
        return level * v;
    }
    static constexpr double kGain = 4.4;
private:
    double sr = 192000.0, level = 0.0;
    uint32_t driftSeed = 0;
    SlowDrift drift;
};
}
