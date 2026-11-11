#include "Oscillator2.h"

namespace psm3
{
ShapedCore::Out ShapedCore::process(double dt)
{
    const double lim = std::max(0.005, 1.25 * dt);
    const double duty = std::min(1.0 - lim, std::max(lim, shape * 0.1));
    const double cornerSlope = (1.0 / duty + 1.0 / (1.0 - duty)) * dt;     // cambio di pendenza per campione

    const double p0 = phase, p1 = phase + dt;       // posizioni non riavvolte
    // Eventi nell'intervallo (p0, p1]: fronte di discesa a 'duty', fronte di salita a 1, discesa a 1+duty.
    if (p0 < duty && p1 >= duty)
    {
        const double tau = (p1 - duty) / dt;
        sqBlep.addStep(tau, -1.0); triBlep.addCorner(tau, -cornerSlope);
    }
    if (p1 >= 1.0)
    {
        const double tau = (p1 - 1.0) / dt;
        sqBlep.addStep(tau, 1.0); triBlep.addCorner(tau, cornerSlope);
        if (p1 >= 1.0 + duty)
        {
            const double tau2 = (p1 - 1.0 - duty) / dt;
            sqBlep.addStep(tau2, -1.0); triBlep.addCorner(tau2, -cornerSlope);
        }
    }
    phase = p1 >= 1.0 ? p1 - 1.0 : p1;

    lastSquare = phase < duty ? 1.0 : 0.0;
    const double tri = phase < duty ? phase / duty : (1.0 - phase) / (1.0 - duty);
    Out o;
    o.square = sqBlep.push(lastSquare);
    o.triangle = triBlep.push(tri);
    return o;
}

void Oscillator2::prepare(double sampleRate) { sr = sampleRate; drift.prepare(sr, driftSeed); reset(); }
void Oscillator2::reset() { core.reset(); drift.prepare(sr, driftSeed); }

Oscillator2::Out Oscillator2::process(double cv, double osc3Bleed)
{
    double octaves = (dial - 6.0) * 1.5 + cv / kVoltsPerOctave;
    octaves *= 1.0 + trackCents / 1200.0;           // disuguaglianza di tracking rispetto all'Osc 1 (PR20)
    octaves += drift.process() * (6.0 / 1200.0);
    double f = 261.6 * std::exp2(std::min(12.0, std::max(-14.0, octaves)));
    f = std::min(0.45 * sr, std::max(0.05, f));

    const auto c = core.process(f / sr);
    Out o;
    o.square = squareLevel * kSquareVolts * c.square + osc3Bleed;
    o.triangle = triLevel * kTriangleVolts * c.triangle + osc3Bleed;
    return o;
}
}
