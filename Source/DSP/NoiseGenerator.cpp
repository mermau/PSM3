#include "NoiseGenerator.h"

namespace psm3
{
void NoiseGenerator::prepare(double sampleRate)
{
    sr = sampleRate;
    bandCoeff = 1.0 - std::exp(-kTwoPi * 20000.0 / sr);         // banda della sorgente
    // sigma 0,5 V dopo il limitatore di banda: 99,7 % dei campioni entro 3 V p-p
    norm = 0.5 / std::sqrt(bandCoeff / (2.0 - bandCoeff));
    setColour(colour);
    reset();
}

void NoiseGenerator::reset() { rng.seed(seed); bandState = lpState = hpState = 0.0; age = 0.0; }

void NoiseGenerator::setColour(double d)
{
    colour = d;
    const double c = std::min(1.0, std::max(-1.0, (d - 5.0) / 5.0));
    // sinistra: scuro (passa-basso fino a 200 Hz); destra: chiaro (passa-alto fino a 4 kHz)
    const double lpHz = c < 0.0 ? 20000.0 * std::pow(200.0 / 20000.0, -c) : 20000.0;
    const double hpHz = c > 0.0 ? 20.0 * std::pow(4000.0 / 20.0, c) : 0.0;
    lpCoeff = c < 0.0 ? 1.0 - std::exp(-kTwoPi * lpHz / sr) : 1.0;
    hpCoeff = c > 0.0 ? 1.0 - std::exp(-kTwoPi * hpHz / sr) : 0.0;
}

double NoiseGenerator::process()
{
    bandState += bandCoeff * (rng.gauss() * norm - bandState);
    double v = bandState;
    lpState += lpCoeff * (v - lpState); v = lpState;
    hpState += hpCoeff * (v - hpState); v -= hpState;

    double gain = level;
    if (warmUp)
    {
        // L'originale impiega 20-30 secondi a partire dopo l'accensione.
        age += 1.0 / sr;
        if (age < 20.0) return 0.0;
        gain *= std::min(1.0, (age - 20.0) / 8.0);
    }
    return v * gain;
}
}
