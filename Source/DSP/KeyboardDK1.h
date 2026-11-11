// PSM-3 — Tastiera DK1 "Cricklewood": monofonica, 3 ottave, con oscillatore proprio,
// tensione di tastiera e tensione dinamica (velocita' del tasto).
#pragma once
#include "Volts.h"

namespace psm3
{
class KeyboardDK1
{
public:
    void prepare(double sampleRate) { sr = sampleRate; reset(); }
    void reset();
    void noteOn(int midiNote, double velocity01);
    void noteOff(int midiNote);
    void allNotesOff();

    void setFrequency(double d) { freqDial = d; }               // frequenza centrale dell'oscillatore di tastiera
    void setTuningSpread(double d) { spread = d / 5.0; }        // 5 = cromatico, 2,5 = quarti di tono
    void setLevel(double d) { level = dialNorm(d); }
    void setDynamicRange(double d) { dynRange = dialNorm(d); }
    void setCh1Signal(bool signal) { ch1Signal = signal; }      // true = SIGNAL, false = KEYBOARD VOLTAGE
    void setCh2Dynamic(bool dynamic) { ch2Dynamic = dynamic; }  // false = KEYBOARD VOLTAGE, true = DYNAMIC VOLTAGE

    struct Out { double ch1, ch2; };                            // tensioni agli High Level degli Input Amp
    Out process();

    bool gate() const { return count > 0; }
    bool takeTrigger() { const bool t = pendingTrigger; pendingTrigger = false; return t; }

    // 0 V al fa diesis centrale, 1,5 V a tre ottave di estensione (+/-18 semitoni): 1 V per ottava.
    static constexpr int kCentreNote = 66;
    static constexpr double kVoltsPerSemitone = 1.5 / 18.0;
    // Posizione di INPUT LEVEL Ch 1 che rende la tastiera cromatica con un pin rosso sulla colonna I
    // (guadagno 0,2128 x 2, pin 2k7 su ingresso da 10k, 0,32 V per ottava). Verificata in test_dk1.
    static constexpr double kChromaticInputLevel = 6.71873;
private:
    static constexpr int kMaxHeld = 16;
    void remove(int note);
    double sr = 192000.0, freqDial = 5.0, spread = 1.0, level = 0.7, dynRange = 0.5;
    bool ch1Signal = false, ch2Dynamic = true, pendingTrigger = false;
    int held[kMaxHeld] {}, count = 0, current = kCentreNote;
    double velocity = 0.0, dynamicVolts = 0.0, phase = 0.0;
};
}
