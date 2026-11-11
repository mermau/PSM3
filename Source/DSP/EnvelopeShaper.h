// PSM-3 — Envelope Shaper: generatore di trapezoide (Attack / On / Decay / Off) con riciclo
// automatico, che comanda un amplificatore controllato in tensione.
#pragma once
#include "Volts.h"

namespace psm3
{
class EnvelopeShaper
{
public:
    enum class Phase { Attack, On, Decay, Off };

    void prepare(double sampleRate) { sr = sampleRate; reset(); }
    void reset();
    void setAttack(double d)  { attackSec = 0.002 * std::pow(500.0, dialNorm(d)); }                 // 2 ms ... 1 s
    void setOn(double d)      { const double n = dialNorm(d); onSec = 2.5 * n * n; }                 // 0 ... 2,5 s
    void setDecay(double d)   { decaySec = 0.003 * std::pow(5000.0, dialNorm(d)); }                  // 3 ms ... 15 s
    void setOff(double d);                                                                             // 10 ms ... 5 s, oltre 7 nessun riciclo
    void setTrapezoidLevel(double d) { trapLevel = dialNorm(d); }
    void setSignalLevel(double d)    { signalLevel = dialNorm(d); }

    // Pulsante Attack o tasto: il fronte di salita avvia un nuovo ciclo; tenuto, resta in On.
    void setGate(bool held);
    void trigger();                         // nuovo ciclo senza tenere (nota ribattuta)

    struct Out { double trapezoid, signal; };
    Out process(double inVolts, double decayCvVolts);   // decayCv = colonna L, 0,4 V per ottava

    bool lampOn() const { return ph == Phase::Attack || ph == Phase::On; }
    Phase phase() const { return ph; }
    bool restingForever() const { return ph == Phase::Off && offForever && ! gate; }

    static constexpr double kFloor = 0.001;         // -60 dB: fine del decadimento
    static constexpr double kSignalGain = 1.2;      // aperto, l'uscita e' almeno pari all'ingresso
private:
    double sr = 192000.0, e = 0.0, timer = 0.0;
    double attackSec = 0.002, onSec = 0.0, decaySec = 0.2, offSec = 5.0;
    double trapLevel = 0.0, signalLevel = 0.0;
    bool offForever = true, gate = false, manual = false;
    Phase ph = Phase::Off;
};
}
