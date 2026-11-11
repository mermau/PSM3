// PSM-3 — Filtro/Oscillatore: scala a diodi (D10-D23, C33-C36) secondo il modello non lineare
// di Fontana e Civolani (DAFx 2008, IEEE TASLP 2010). Quattro condensatori accoppiati dai diodi,
// coppia differenziale in ingresso, retroazione regolata da RESPONSE.
#pragma once
#include "Volts.h"

namespace psm3
{
class DiodeLadderFilter
{
public:
    void prepare(double sampleRate);
    void reset();
    void setFrequencyDial(double d) { dial = d; }
    void setResponse(double d);
    void setLevel(double d) { level = dialNorm(d); }
    void setDriftSeed(uint32_t s) { driftSeed = s; drift.prepare(sr, s); }
    double process(double inVolts, double cvVolts);     // cv = tensione della colonna N

    static constexpr double kVoltsPerOctave = 0.20;
    static constexpr double kThresholdK = 9.3218;       // guadagno d'anello a cui il modello linearizzato oscilla
private:
    double sr = 192000.0, dial = 5.0, level = 0.0, K = 0.0;
    double x[4] = { 0, 0, 0, 0 }, fPrev[4] = { 0, 0, 0, 0 };
    uint32_t driftSeed = 0;
    SlowDrift drift;
    Rng thermal;
};
}
