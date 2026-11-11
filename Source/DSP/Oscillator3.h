// PSM-3 — Oscillatore 3: lento, 0,025 Hz - 500 Hz, 0,26 V/ottava, quadra e triangolo.
#pragma once
#include "Oscillator2.h"

namespace psm3
{
class Oscillator3
{
public:
    void prepare(double sampleRate);
    void reset();
    void setFrequencyDial(double d) { dial = d; }
    void setShape(double d) { shape = d; core.setShape(d); }
    void setSquareLevel(double d) { squareLevel = dialNorm(d); }
    void setTriangleLevel(double d) { triLevel = dialNorm(d); }
    void setDriftSeed(uint32_t s) { driftSeed = s; drift.prepare(sr, s); }

    struct Out { double square, triangle; };
    Out process(double cvVolts);
    double bleed() const { return bleedVolts; }     // disturbo che passa in Osc 2 (e meno in Osc 1)

    static constexpr double kVoltsPerOctave = 0.26;
    static constexpr double kSquareVolts = 5.0;
    static constexpr double kTriangleVolts = 6.0;
private:
    double sr = 192000.0, dial = 5.0, shape = 5.0, squareLevel = 0.0, triLevel = 0.0, bleedVolts = 0.0, bleedHp = 0.0, hpCoeff = 0.0;
    uint32_t driftSeed = 0;
    SlowDrift drift;
    ShapedCore core;
};
}
