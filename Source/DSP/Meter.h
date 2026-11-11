// PSM-3 — Strumento a bobina mobile sulla colonna A.
#pragma once
#include "Volts.h"

namespace psm3
{
class Meter
{
public:
    enum class Mode { ControlVoltage, SignalLevel };
    void prepare(double sampleRate)
    {
        sr = sampleRate;
        ballCoeff = 1.0 - std::exp(-1.0 / (0.040 * sr));
        acCoeff = 1.0 - std::exp(-kTwoPi * 5.0 / sr);
        releaseCoeff = std::exp(-1.0 / (0.3 * sr));
        reset();
    }
    void reset() { n1 = n2 = mode == Mode::ControlVoltage ? 0.5 : 0.0; ac = peak = 0.0; }
    void setMode(Mode m) { mode = m; }
    void process(double v)
    {
        if (! std::isfinite(v)) v = 0.0;
        double target;
        if (mode == Mode::ControlVoltage)
            target = 0.5 + v * 0.1;                             // zero al centro, fondo scala +/-5 V
        else
        {
            ac += acCoeff * (v - ac);
            const double a = std::fabs(v - ac);
            peak = a > peak ? a : peak * releaseCoeff;
            target = 2.0 * peak / 4.0;                          // fondo scala 4 V p-p
        }
        target = std::min(1.0, std::max(0.0, target));
        n1 += ballCoeff * (target - n1);
        n2 += ballCoeff * (n1 - n2);
    }
    double needle() const { return n2; }                        // 0...1
private:
    double sr = 192000.0, n1 = 0.5, n2 = 0.5, ac = 0.0, peak = 0.0, ballCoeff = 0.0, acCoeff = 0.0, releaseCoeff = 0.0;
    Mode mode = Mode::ControlVoltage;
};
}
