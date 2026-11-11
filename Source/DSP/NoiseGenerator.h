// PSM-3 — Generatore di rumore: bianco con comando Colour, 3 V p-p.
#pragma once
#include "Volts.h"

namespace psm3
{
class NoiseGenerator
{
public:
    void prepare(double sampleRate);
    void reset();
    void setSeed(uint32_t s) { seed = s; }
    void setColour(double d);
    void setLevel(double d) { level = dialNorm(d); }
    void setWarmUp(bool enabled) { warmUp = enabled; }
    double process();
private:
    double sr = 192000.0, level = 0.0, colour = 5.0;
    double bandState = 0.0, bandCoeff = 0.0, norm = 1.0;
    double lpState = 0.0, lpCoeff = 1.0, hpState = 0.0, hpCoeff = 0.0;
    bool warmUp = false; double age = 0.0;
    uint32_t seed = 1;
    Rng rng;
};
}
