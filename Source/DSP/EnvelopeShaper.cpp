#include "EnvelopeShaper.h"

namespace psm3
{
void EnvelopeShaper::reset() { e = 0.0; timer = 0.0; ph = Phase::Off; gate = false; manual = false; }

void EnvelopeShaper::setOff(double d)
{
    offForever = d > 7.0;                   // posizione "manuale": nessun riciclo
    offSec = 0.010 * std::pow(500.0, std::min(1.0, std::max(0.0, d / 7.0)));
}

void EnvelopeShaper::setGate(bool held)
{
    if (held && ! gate) { ph = Phase::Attack; manual = true; timer = 0.0; }
    gate = held;
}

void EnvelopeShaper::trigger() { ph = Phase::Attack; manual = true; timer = 0.0; }

EnvelopeShaper::Out EnvelopeShaper::process(double in, double decayCv)
{
    const double dt = 1.0 / sr;
    switch (ph)
    {
        case Phase::Attack:
            e += dt / attackSec;
            if (e >= 1.0) { e = 1.0; ph = Phase::On; timer = 0.0; }
            break;
        case Phase::On:
            // Avviato dal pulsante il tempo di On dura circa il doppio che nel riciclo automatico.
            if (! gate)
            {
                timer += dt;
                if (timer >= (manual ? 2.0 * onSec : onSec)) ph = Phase::Decay;
            }
            break;
        case Phase::Decay:
        {
            const double cv = std::isfinite(decayCv) ? std::min(4.0, std::max(-4.0, decayCv)) : 0.0;
            const double t = std::min(120.0, std::max(0.0005, decaySec * std::exp2(cv / 0.4)));
            e *= std::exp(-6.9078 * dt / t);            // -60 dB nel tempo impostato
            if (e <= kFloor) { e = 0.0; ph = Phase::Off; timer = 0.0; }
            break;
        }
        case Phase::Off:
            if (! offForever)
            {
                timer += dt;
                if (timer >= offSec) { ph = Phase::Attack; manual = false; }
            }
            break;
    }

    Out o;
    o.trapezoid = trapLevel * (4.0 - 7.0 * e);          // +4 V a riposo, -3 V in On
    o.signal = signalLevel * in * (kSignalGain * e + 0.0015);   // residuo a inviluppo chiuso: circa -56 dB
    return o;
}
}
