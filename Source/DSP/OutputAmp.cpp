#include "OutputAmp.h"

namespace psm3
{
static constexpr double kFloorDb = -70.0;       // PR4/PR5: -70 dB con la manopola a zero
static const double kFloorGain = std::pow(10.0, kFloorDb / 20.0);

void OutputAmp::prepare(double sampleRate)
{
    sr = sampleRate;
    toneCoeff = 1.0 - std::exp(-kTwoPi * 700.0 / sr);           // rete 2k7 / 47 nF
    spHpCoeff = 1.0 - std::exp(-kTwoPi * 150.0 / sr);
    spLpCoeff = 1.0 - std::exp(-kTwoPi * 5000.0 / sr);
    reset();
}

void OutputAmp::reset() { toneState = spHp1 = spHp2 = spLp1 = spLp2 = 0.0; }

void OutputAmp::setLevel(double d)
{
    // Potenziometro logaritmico sulla tensione di controllo del FET: -70 dB a 0, circa -3,5 dB a 5
    // (rampa piena -> 1 V p-p, come nella procedura di collaudo), +6 dB a 10.
    const double x = 1.0 - dialNorm(d);
    levelDb = 6.0 - 76.0 * x * x * x;
}

double OutputAmp::process(double in, double levelCv)
{
    if (! std::isfinite(in)) in = 0.0;
    if (! std::isfinite(levelCv)) levelCv = 0.0;

    // tono: piatto a 5, taglia gli acuti verso 0 e i bassi verso 10
    toneState += toneCoeff * (in - toneState);
    const double low = toneState, high = in - low;
    double v = tilt < 0.0 ? low + (1.0 + tilt) * high : high + (1.0 - tilt) * low;

    // modulatore a FET: 15 dB per volt
    const double db = std::min(9.0, std::max(kFloorDb, levelDb + 15.0 * levelCv));
    const double gain = std::max(0.0, std::pow(10.0, db / 20.0) - kFloorGain);
    if (gain == 0.0) return 0.0;
    v *= gain;

    const double q = v / 2.5;                                   // non oltre 5 V p-p
    v /= std::sqrt(std::sqrt(1.0 + q * q * q * q));

    if (speaker)
    {
        // altoparlante interno da 5 pollici con finale da 1 W: banda stretta e distorsione asimmetrica
        spHp1 += spHpCoeff * (v - spHp1); v -= spHp1;
        spHp2 += spHpCoeff * (v - spHp2); v -= spHp2;
        spLp1 += spLpCoeff * (v - spLp1);
        spLp2 += spLpCoeff * (spLp1 - spLp2);
        v = spLp2;
        v = v > 0.0 ? 1.2 * std::tanh(v / 1.2) : 1.6 * std::tanh(v / 1.6);
    }
    return v;
}

void OutputAmp::panGains(double& left, double& right) const
{
    left = std::cos(pan * 0.5 * kPi);
    right = std::sin(pan * 0.5 * kPi);
    if (left < 1e-9) left = 0.0;
    if (right < 1e-9) right = 0.0;
}
}
