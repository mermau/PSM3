// PSM-3 — Oscillatore 1: sinusoide (con Shape) e rampa. 1 Hz - 10 kHz, 0,32 V/ottava.
#pragma once
#include "Blep.h"
#include "Volts.h"

namespace psm3
{
class Oscillator1
{
public:
    void prepare(double sampleRate);
    void reset();
    void setFrequencyDial(double d) { dial = d; }
    void setShape(double d) { shape = d; }
    void setSineLevel(double d) { sineLevel = dialNorm(d); }
    void setRampLevel(double d) { rampLevel = dialNorm(d); }
    void setDriftSeed(uint32_t s) { driftSeed = s; drift.prepare(sr, s); }

    struct Out { double sine, ramp; };
    Out process(double cvVolts);        // cv = tensione della colonna I

    static constexpr double kVoltsPerOctave = 0.32;
    static constexpr double kSinePeak = 1.0;    // 2 V p-p a livello 10
    static constexpr double kRampPeak = 1.0;
private:
    double sr = 192000.0, phase = 0.0, dial = 6.0, shape = 5.0, sineLevel = 0.0, rampLevel = 0.0;
    double spike = 0.0, spikeDecay = 0.0;
    uint32_t driftSeed = 0;
    SlowDrift drift;
    BlepBuffer rampBlep, sineBlep;
};
}
