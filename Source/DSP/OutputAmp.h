// PSM-3 — Amplificatore d'uscita: filtro di tono, modulatore a FET controllato in tensione, pan.
#pragma once
#include "Volts.h"

namespace psm3
{
class OutputAmp
{
public:
    void prepare(double sampleRate);
    void reset();
    void setFilter(double d) { tilt = std::min(1.0, std::max(-1.0, (d - 5.0) / 5.0)); }
    void setLevel(double d);
    void setPan(double d) { pan = dialNorm(d); }
    void setSpeaker(bool on) { speaker = on; }
    double process(double inVolts, double levelCvVolts);    // volt all'uscita di linea, prima del pan
    void panGains(double& left, double& right) const;
private:
    double sr = 192000.0, tilt = 0.0, levelDb = -70.0, pan = 0.5;
    double toneState = 0.0, toneCoeff = 0.0;
    double spHp1 = 0.0, spHp2 = 0.0, spLp1 = 0.0, spLp2 = 0.0, spHpCoeff = 0.0, spLpCoeff = 0.0;
    bool speaker = false;
};
}
