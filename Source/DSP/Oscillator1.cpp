#include "Oscillator1.h"

namespace psm3
{
void Oscillator1::prepare(double sampleRate)
{
    sr = sampleRate;
    spikeDecay = std::exp(-1.0 / (30e-6 * sr));
    drift.prepare(sr, driftSeed);
    reset();
}

void Oscillator1::reset()
{
    phase = 0.0; spike = 0.0;
    rampBlep.reset(); sineBlep.reset();
    drift.prepare(sr, driftSeed);
}

Oscillator1::Out Oscillator1::process(double cv)
{
    // 1,5 ottave per divisione; 261,6 Hz a 6 (PR15); deriva di pochi cent.
    const double octaves = (dial - 6.0) * 1.5 + cv / kVoltsPerOctave + drift.process() * (6.0 / 1200.0);
    double f = 261.6 * std::exp2(std::min(12.0, std::max(-14.0, octaves)));
    f = std::min(0.45 * sr, std::max(0.05, f));
    const double dt = f / sr;

    // Shape: 5 = sinusoide pura; verso 0 e 10 semionde arrotondate di verso opposto.
    const double k = std::min(1.0, std::max(-1.0, (shape - 5.0) / 5.0));
    const double pure = 1.0 - std::fabs(k);

    phase += dt;
    if (phase >= 1.0)
    {
        phase -= 1.0;
        const double tau = phase / dt;
        rampBlep.addStep(tau, -2.0);                        // ritorno della rampa
        sineBlep.addCorner(tau, 4.0 * kPi * k * dt);        // cuspide della semionda
        spike = 0.03;                                       // residuo di ritorno sul picco (PR17)
    }

    const double rect = 1.0 - 2.0 * std::fabs(std::sin(kPi * phase));
    const double sineNaive = pure * std::cos(kTwoPi * phase) - k * rect;
    spike *= spikeDecay;

    Out o;
    o.ramp = rampLevel * kRampPeak * rampBlep.push(2.0 * phase - 1.0);
    o.sine = sineLevel * kSinePeak * (sineBlep.push(sineNaive) + spike * pure);
    return o;
}
}
