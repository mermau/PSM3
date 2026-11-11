// PSM-3 — Amplificatore d'ingresso: invertente, guadagno 2 dall'ingresso High Level.
#pragma once
#include "Volts.h"

namespace psm3
{
struct InputAmp
{
    // Potenziometro di livello logaritmico: la tastiera diventa cromatica intorno a 6,7.
    void setLevel(double d) { gain = (std::pow(100.0, dialNorm(d)) - 1.0) / 99.0; }
    double process(double v) const { return gain == 0.0 ? 0.0 : clampRails(-2.0 * gain * v); }
    double gain = 0.0;
};
}
