// PSM-3 — Riverbero: due molle (25 e 30 ms) con dispersione, Mix controllabile in tensione.
#pragma once
#include "Volts.h"
#include <vector>

namespace psm3
{
class SpringReverb
{
public:
    void prepare(double sampleRate);
    void reset();
    void setMix(double d) { mix = dialNorm(d); }
    void setLevel(double d) { level = dialNorm(d); }
    double process(double inVolts, double mixCvVolts);      // mixCv = colonna M: -2 V asciutto, +2 V tutto riverbero
private:
    struct Spring
    {
        static constexpr int kSections = 30, kStretch = 4;
        std::vector<double> delay; int pos = 0;
        double ax[kSections][kStretch] {}, ay[kSections][kStretch] {};
        int apos = 0;
        double lp = 0.0, dc = 0.0, feedback = 0.0, gain = 0.9;
        void prepare(double fs, double seconds, double rt60);
        void reset();
        double process(double in, double lpCoeff, double dcCoeff);
    };
    double step(double drive);

    double sr = 192000.0, fs = 48000.0, mix = 0.0, level = 0.0;
    int ratio = 4, phase = 0;
    double pre1 = 0.0, pre2 = 0.0, post1 = 0.0, post2 = 0.0, bandCoeff = 0.0;
    double wetHeld = 0.0, lpCoeff = 0.0, dcCoeff = 0.0;
    Spring springs[2];
};
}
