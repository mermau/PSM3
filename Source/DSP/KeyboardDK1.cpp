#include "KeyboardDK1.h"

namespace psm3
{
void KeyboardDK1::reset()
{
    count = 0; current = kCentreNote; velocity = 0.0; dynamicVolts = 0.0; phase = 0.0; pendingTrigger = false;
}

void KeyboardDK1::remove(int note)
{
    int w = 0;
    for (int i = 0; i < count; ++i) if (held[i] != note) held[w++] = held[i];
    count = w;
}

void KeyboardDK1::noteOn(int note, double vel)
{
    if (note < 0 || note > 127) return;
    vel = std::isfinite(vel) ? std::min(1.0, std::max(0.0, vel)) : 0.0;
    const bool wasFree = count == 0;
    remove(note);
    if (count == kMaxHeld) { for (int i = 1; i < count; ++i) held[i - 1] = held[i]; --count; }
    held[count++] = note;
    current = note;
    // Il circuito di tocco e l'inviluppo scattano solo se il tasto precedente e' stato rilasciato.
    if (wasFree) { pendingTrigger = true; velocity = vel; dynamicVolts = 3.0 * vel; }
}

void KeyboardDK1::noteOff(int note)
{
    if (count == 0) return;
    remove(note);
    if (count > 0) current = held[count - 1];       // torna l'ultima nota ancora premuta, senza nuovo attacco
}

void KeyboardDK1::allNotesOff() { count = 0; }

KeyboardDK1::Out KeyboardDK1::process()
{
    // Gli amplificatori d'ingresso invertono: la DK1 presenta tensioni di segno opposto, cosi' alla
    // matrice la tensione cresce con l'altezza della nota e con la forza del tocco.
    const double kv = -(double) (current - kCentreNote) * kVoltsPerSemitone;
    const double dyn = -dynamicVolts * dynRange;

    double signal = 0.0;
    if (ch1Signal && count > 0)
    {
        // Oscillatore di tastiera: 261 Hz al sol centrale con Frequency a 5; Tuning Spread = intervallo fra i tasti.
        double f = 261.6 * std::exp2((freqDial - 5.0) + (double) (current - 67) / 12.0 * spread);
        f = std::min(0.4 * sr, std::max(1.0, f));
        const double dt = f / sr;
        phase += dt; if (phase >= 1.0) phase -= 1.0;
        double saw = 2.0 * phase - 1.0;
        if (phase < dt) { const double t = phase / dt; saw -= t + t - t * t - 1.0; }                    // polyBLEP
        else if (phase > 1.0 - dt) { const double t = (phase - 1.0) / dt; saw -= t * t + t + t + 1.0; }
        signal = saw * level * (0.3 + 0.7 * velocity);
    }
    Out o;
    o.ch1 = ch1Signal ? signal : kv;
    o.ch2 = ch2Dynamic ? dyn : kv;
    return o;
}
}
