// PSM-3 — motore completo: tutti i dispositivi del VCS3 collegati dalla matrice a pin.
// Lavora in volt reali a 2x o 4x la frequenza dell'host. Nessuna dipendenza da JUCE.
#pragma once
#include "DiodeLadderFilter.h"
#include "EnvelopeShaper.h"
#include "InputAmp.h"
#include "KeyboardDK1.h"
#include "Matrix.h"
#include "Meter.h"
#include "NoiseGenerator.h"
#include "Oscillator1.h"
#include "Oscillator2.h"
#include "Oscillator3.h"
#include "OutputAmp.h"
#include "Oversampler.h"
#include "Params.h"
#include "RingModulator.h"
#include "SpringReverb.h"
#include <atomic>

namespace psm3
{
class PSM3Engine
{
public:
    PSM3Engine();

    void prepare(double hostSampleRate, int maxBlock);
    void reset();

    // Thread audio.
    void setParam(ParamId id, double value);            // manopole 0...10, interruttori 0/1
    void noteOn(int note, double velocity01);
    void noteOff(int note);
    void allNotesOff();
    void process(const float* inL, const float* inR, float* outL, float* outR, int numSamples);   // in* possono essere nullptr

    // Qualsiasi thread non audio.
    void setPins(const PinGrid& pins, uint32_t toleranceSeed);
    void setQuality(int factor) { requestedFactor = factor == 2 ? 2 : 4; }    // effettivo al prossimo prepare
    void setWarmUp(bool enabled) { warmUp.store(enabled); }
    void setDriftEnabled(bool enabled) { driftEnabled = enabled; }             // prima di prepare

    double meterNeedle() const { return needle.load(std::memory_order_relaxed); }
    bool envelopeLamp() const { return lamp.load(std::memory_order_relaxed); }
    bool isIdle() const { return idle.load(std::memory_order_relaxed); }
    int latencySamples() const { return latency; }
    double internalSampleRate() const { return innerRate; }

private:
    void applyParam(ParamId id, double v);
    void updateControls(bool force);
    void fetchPins();
    void tick(double ext1, double ext2, double& left, double& right);

    static constexpr int kNumParams = (int) ParamId::Count;
    static constexpr int kControlInterval = 16;         // campioni interni tra due aggiornamenti delle manopole

    double hostRate = 48000.0, innerRate = 192000.0;
    int factor = 4, requestedFactor = 4, latency = 0, controlCounter = 0;
    bool driftEnabled = true, prepared = false;

    double target[kNumParams] {}, current[kNumParams] {};
    double smoothCoeff = 1.0;

    // dispositivi
    Oscillator1 osc1; Oscillator2 osc2; Oscillator3 osc3;
    NoiseGenerator noise; RingModulator ring; DiodeLadderFilter filter;
    EnvelopeShaper env; SpringReverb reverb;
    InputAmp in1, in2; OutputAmp out1, out2; Meter meter; KeyboardDK1 dk1;
    Decimator decL, decR; Interpolator upL, upR;

    std::array<double, 16> rows {}, cols {};
    double joyX = 0.0, joyY = 0.0;
    bool attackHeld = false, mainsOn = true, lightMode = true;
    double mainsGain = 1.0, mainsStep = 0.0;

    // matrice: preparata fuori dal thread audio, presa con un try-lock (il thread audio non aspetta mai)
    MatrixSolution active, staged;
    std::atomic_flag pinLock = ATOMIC_FLAG_INIT;
    std::atomic<bool> pinsDirty { false };

    std::atomic<bool> warmUp { false }, lamp { false }, idle { true };
    std::atomic<double> needle { 0.5 };
    bool warmUpApplied = false;
};
}
