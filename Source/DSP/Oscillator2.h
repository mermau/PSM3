// PSM-3 — Oscillatore 2: quadra/impulso e triangolo/dente di sega con Shape.
// Contiene anche il nucleo comune usato dall'Oscillatore 3.
#pragma once
#include "Blep.h"
#include "Volts.h"

namespace psm3
{
// Nucleo: fase 0...1, quadra alta per fase < duty, triangolo che sale fino a 'duty' e poi scende.
// Uscite normalizzate 0...1 (unipolari: le forme non sono simmetriche rispetto a 0 V).
class ShapedCore
{
public:
    void reset() { phase = 0.0; sqBlep.reset(); triBlep.reset(); }
    void setShape(double d) { shape = d; }
    struct Out { double square, triangle; };
    Out process(double dt);
    double rawSquare() const { return lastSquare; }
private:
    double phase = 0.0, shape = 5.0, lastSquare = 0.0;
    BlepBuffer sqBlep, triBlep;
};

class Oscillator2
{
public:
    void prepare(double sampleRate);
    void reset();
    void setFrequencyDial(double d) { dial = d; }
    void setShape(double d) { core.setShape(d); }
    void setSquareLevel(double d) { squareLevel = dialNorm(d); }
    void setTriangleLevel(double d) { triLevel = dialNorm(d); }
    void setDriftSeed(uint32_t s) { driftSeed = s; drift.prepare(sr, s); }
    void setTrackingError(double centsPerOctave) { trackCents = centsPerOctave; }

    struct Out { double square, triangle; };
    Out process(double cvVolts, double osc3Bleed = 0.0);

    static constexpr double kVoltsPerOctave = 0.32;
    static constexpr double kSquareVolts = 2.0;     // 2 V p-p
    static constexpr double kTriangleVolts = 3.5;   // 3,5 V p-p
private:
    double sr = 192000.0, dial = 6.0, squareLevel = 0.0, triLevel = 0.0, trackCents = 0.0;
    uint32_t driftSeed = 0;
    SlowDrift drift;
    ShapedCore core;
};
}
