#include "Oscillator3.h"

namespace psm3
{
void Oscillator3::prepare(double sampleRate)
{
    sr = sampleRate;
    hpCoeff = 1.0 - std::exp(-kTwoPi * 300.0 / sr);
    drift.prepare(sr, driftSeed);
    reset();
}
void Oscillator3::reset() { core.reset(); bleedVolts = bleedHp = 0.0; drift.prepare(sr, driftSeed); }

Oscillator3::Out Oscillator3::process(double cv)
{
    // 500 Hz a 10 (PR21), 63 Hz a 8, periodo di circa un minuto a 0.
    const double octaves = (dial - 10.0) * 1.494 + cv / kVoltsPerOctave + drift.process() * (8.0 / 1200.0);
    double f = 500.0 * std::exp2(std::min(6.0, std::max(-17.0, octaves)));
    f = std::min(0.45 * sr, std::max(0.01, f));

    const auto c = core.process(f / sr);

    // Passaggio capacitivo dei fronti della quadra sulle piste vicine (difetto documentato della scheda C).
    const double sq = core.rawSquare() * kSquareVolts;
    bleedHp += hpCoeff * (sq - bleedHp);
    bleedVolts = (sq - bleedHp) * 0.001 * (std::fabs(shape - 5.0) / 5.0 + 0.2);

    Out o;
    o.square = squareLevel * kSquareVolts * c.square;
    o.triangle = triLevel * kTriangleVolts * c.triangle;
    return o;
}
}
