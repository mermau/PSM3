#include "PSM3Engine.h"
#include <thread>

namespace psm3
{
// Righe della matrice (sorgenti) e colonne (destinazioni), come sul pannello.
enum Row { rOsc1Sine, rOsc1Ramp, rOsc2Square, rOsc2Tri, rOsc3Square, rOsc3Tri, rNoise, rIn1, rIn2,
           rFilter, rTrapezoid, rEnvelope, rRing, rReverb, rStickX, rStickY };
enum Col { cMeter, cOut1, cOut2, cEnvelope, cRingA, cRingB, cReverb, cFilter,
           cFreq1, cFreq2, cFreq3, cDecay, cRevMix, cFiltFreq, cLevel1, cLevel2 };

PSM3Engine::PSM3Engine()
{
    for (int i = 0; i < kNumParams; ++i) target[i] = current[i] = paramInfo((ParamId) i).def;
}

void PSM3Engine::prepare(double hostSampleRate, int)
{
    hostRate = hostSampleRate > 1000.0 ? hostSampleRate : 48000.0;
    factor = hostRate >= 88200.0 ? 2 : requestedFactor;      // 4x sotto 88,2 kHz, 2x sopra
    innerRate = hostRate * factor;
    smoothCoeff = 1.0 - std::exp(-(double) kControlInterval / (0.005 * innerRate));     // manopole lisciate su 5 ms
    mainsStep = 1.0 / (0.030 * hostRate);

    const uint32_t d = driftEnabled ? 1u : 0u;
    osc1.setDriftSeed(11u * d); osc2.setDriftSeed(22u * d); osc3.setDriftSeed(33u * d);
    filter.setDriftSeed(44u * d); ring.setDriftSeed(55u);
    osc2.setTrackingError(driftEnabled ? 1.5 : 0.0);         // Osc 1 e 2 non inseguono mai alla perfezione (PR20)
    noise.setSeed(0x5EEDu);

    osc1.prepare(innerRate); osc2.prepare(innerRate); osc3.prepare(innerRate);
    noise.prepare(innerRate); ring.prepare(innerRate); filter.prepare(innerRate);
    env.prepare(innerRate); reverb.prepare(innerRate);
    out1.prepare(innerRate); out2.prepare(innerRate); meter.prepare(innerRate); dk1.prepare(innerRate);
    decL.prepare(factor); decR.prepare(factor); upL.prepare(factor); upR.prepare(factor);
    latency = decL.latencySamples();
    prepared = true;
    reset();
}

void PSM3Engine::reset()
{
    osc1.reset(); osc2.reset(); osc3.reset(); noise.reset(); ring.reset(); filter.reset();
    env.reset(); reverb.reset(); out1.reset(); out2.reset(); meter.reset(); dk1.reset();
    decL.reset(); decR.reset(); upL.reset(); upR.reset();
    rows.fill(0.0); cols.fill(0.0);
    controlCounter = 0;
    mainsGain = mainsOn ? 1.0 : 0.0;
    for (int i = 0; i < kNumParams; ++i) current[i] = target[i];
    updateControls(true);
}

void PSM3Engine::setParam(ParamId id, double value)
{
    const int i = (int) id;
    if (i < 0 || i >= kNumParams || ! std::isfinite(value)) return;
    target[i] = paramInfo(id).isSwitch ? (value >= 0.5 ? 1.0 : 0.0) : std::min(10.0, std::max(0.0, value));
}

void PSM3Engine::noteOn(int note, double velocity) { if (mainsOn) dk1.noteOn(note, velocity); }
void PSM3Engine::noteOff(int note) { dk1.noteOff(note); }
void PSM3Engine::allNotesOff() { dk1.allNotesOff(); }

void PSM3Engine::setPins(const PinGrid& pins, uint32_t toleranceSeed)
{
    MatrixSolution s;
    Matrix::build(pins, toleranceSeed, kRowOutOhms, s);
    while (pinLock.test_and_set(std::memory_order_acquire)) std::this_thread::yield();
    staged = s;
    pinsDirty.store(true, std::memory_order_release);
    pinLock.clear(std::memory_order_release);
}

void PSM3Engine::fetchPins()
{
    if (! pinsDirty.load(std::memory_order_acquire)) return;
    if (pinLock.test_and_set(std::memory_order_acquire)) return;        // occupato: si riprova al prossimo blocco
    active = staged;
    pinsDirty.store(false, std::memory_order_release);
    pinLock.clear(std::memory_order_release);
}

void PSM3Engine::applyParam(ParamId id, double v)
{
    switch (id)
    {
        case ParamId::osc1Freq:     osc1.setFrequencyDial(v); break;
        case ParamId::osc1Shape:    osc1.setShape(v); break;
        case ParamId::osc1Sine:     osc1.setSineLevel(v); break;
        case ParamId::osc1Ramp:     osc1.setRampLevel(v); break;
        case ParamId::osc2Freq:     osc2.setFrequencyDial(v); break;
        case ParamId::osc2Shape:    osc2.setShape(v); break;
        case ParamId::osc2Square:   osc2.setSquareLevel(v); break;
        case ParamId::osc2Tri:      osc2.setTriangleLevel(v); break;
        case ParamId::osc3Freq:     osc3.setFrequencyDial(v); break;
        case ParamId::osc3Shape:    osc3.setShape(v); break;
        case ParamId::osc3Square:   osc3.setSquareLevel(v); break;
        case ParamId::osc3Tri:      osc3.setTriangleLevel(v); break;
        case ParamId::noiseColour:  noise.setColour(v); break;
        case ParamId::noiseLevel:   noise.setLevel(v); break;
        case ParamId::in1Level:     in1.setLevel(v); break;
        case ParamId::in2Level:     in2.setLevel(v); break;
        case ParamId::ringLevel:    ring.setLevel(v); break;
        case ParamId::filtFreq:     filter.setFrequencyDial(v); break;
        case ParamId::filtResponse: filter.setResponse(v); break;
        case ParamId::filtLevel:    filter.setLevel(v); break;
        case ParamId::envAttack:    env.setAttack(v); break;
        case ParamId::envOn:        env.setOn(v); break;
        case ParamId::envDecay:     env.setDecay(v); break;
        case ParamId::envOff:       env.setOff(v); break;
        case ParamId::envTrapezoid: env.setTrapezoidLevel(v); break;
        case ParamId::envSignal:    env.setSignalLevel(v); break;
        case ParamId::attackButton: attackHeld = v >= 0.5; break;
        case ParamId::revMix:       reverb.setMix(v); break;
        case ParamId::revLevel:     reverb.setLevel(v); break;
        case ParamId::outFilter1:   out1.setFilter(v); break;
        case ParamId::outFilter2:   out2.setFilter(v); break;
        case ParamId::speaker1:     out1.setSpeaker(v >= 0.5); break;
        case ParamId::speaker2:     out2.setSpeaker(v >= 0.5); break;
        case ParamId::meterMode:    meter.setMode(v >= 0.5 ? Meter::Mode::SignalLevel : Meter::Mode::ControlVoltage); break;
        case ParamId::joyX: case ParamId::rangeX:
            joyX = (current[(int) ParamId::joyX] - 5.0) / 5.0 * 2.0 * dialNorm(current[(int) ParamId::rangeX]); break;
        case ParamId::joyY: case ParamId::rangeY:
            joyY = (current[(int) ParamId::joyY] - 5.0) / 5.0 * 2.0 * dialNorm(current[(int) ParamId::rangeY]); break;
        case ParamId::out1Level:    out1.setLevel(v); break;
        case ParamId::out1Pan:      out1.setPan(v); break;
        case ParamId::out2Level:    out2.setLevel(v); break;
        case ParamId::out2Pan:      out2.setPan(v); break;
        case ParamId::mains:        mainsOn = v >= 0.5; break;
        case ParamId::kbFreq:       dk1.setFrequency(v); break;
        case ParamId::kbTuning:     dk1.setTuningSpread(v); break;
        case ParamId::kbLevel:      dk1.setLevel(v); break;
        case ParamId::kbDynRange:   dk1.setDynamicRange(v); break;
        case ParamId::kbCh1Mode:    dk1.setCh1Signal(v < 0.5); break;
        case ParamId::kbCh2Mode:    dk1.setCh2Dynamic(v >= 0.5); break;
        case ParamId::Count:        break;
    }
}

void PSM3Engine::updateControls(bool force)
{
    for (int i = 0; i < kNumParams; ++i)
    {
        const double diff = target[i] - current[i];
        if (! force && diff == 0.0) continue;
        if (force || paramInfo((ParamId) i).isSwitch || std::fabs(diff) < 1e-4) current[i] = target[i];
        else current[i] += smoothCoeff * diff;
        applyParam((ParamId) i, current[i]);
    }
    const bool w = warmUp.load(std::memory_order_relaxed);
    if (force || w != warmUpApplied) { noise.setWarmUp(w); warmUpApplied = w; }
}

// Un campione interno: le uscite del campione precedente attraversano la matrice e alimentano i dispositivi.
void PSM3Engine::tick(double ext1, double ext2, double& left, double& right)
{
    if (++controlCounter >= kControlInterval) { controlCounter = 0; updateControls(false); }

    Matrix::apply(active, rows, cols);
    const auto& used = active.rowUsed;

    // tastiera e ingressi
    const auto kb = dk1.process();
    if (dk1.takeTrigger()) env.trigger();
    env.setGate(attackHeld || dk1.gate());
    rows[rIn1] = in1.process(ext1 + kb.ch1);
    rows[rIn2] = in2.process(ext2 + kb.ch2);

    // sorgenti
    const auto o3 = osc3.process(cols[cFreq3]);
    const double bleed = osc3.bleed();
    const auto o1 = osc1.process(cols[cFreq1]);
    const auto o2 = osc2.process(cols[cFreq2], bleed);
    rows[rOsc1Sine] = o1.sine + 0.3 * bleed; rows[rOsc1Ramp] = o1.ramp + 0.3 * bleed;
    rows[rOsc2Square] = o2.square; rows[rOsc2Tri] = o2.triangle;
    rows[rOsc3Square] = o3.square; rows[rOsc3Tri] = o3.triangle;
    rows[rNoise] = used[rNoise] ? noise.process() : 0.0;
    rows[rStickX] = joyX; rows[rStickY] = joyY;

    // trattamenti (calcolati solo se la loro uscita e' collegata)
    if (used[rFilter])
    {
        double v = filter.process(cols[cFilter], cols[cFiltFreq]);
        if (sanitize(v)) filter.reset();
        rows[rFilter] = v;
    }
    else rows[rFilter] = 0.0;

    rows[rRing] = used[rRing] ? ring.process(cols[cRingA], cols[cRingB]) : 0.0;

    const auto e = env.process(cols[cEnvelope], cols[cDecay]);
    rows[rTrapezoid] = e.trapezoid; rows[rEnvelope] = e.signal;

    if (used[rReverb])
    {
        double v = reverb.process(cols[cReverb], cols[cRevMix]);
        if (sanitize(v)) reverb.reset();
        rows[rReverb] = v;
    }
    else rows[rReverb] = 0.0;

    for (auto& r : rows) { if (sanitize(r)) r = 0.0; r = clampRails(r); }

    // uscite
    if (active.colUsed[cMeter]) meter.process(cols[cMeter]); else meter.process(0.0);
    double a = 0.0, b = 0.0;
    if (active.colUsed[cOut1]) { a = out1.process(cols[cOut1], cols[cLevel1]); if (sanitize(a)) out1.reset(); }
    if (active.colUsed[cOut2]) { b = out2.process(cols[cOut2], cols[cLevel2]); if (sanitize(b)) out2.reset(); }
    double l1, r1, l2, r2;
    out1.panGains(l1, r1); out2.panGains(l2, r2);
    // Limite di sicurezza: lineare fino a 1,3 V, poi piega dolcemente e non supera 1,7 V. Il margine
    // copre la sovraelongazione del filtro di decimazione, cosi' l'uscita resta sotto 0 dBFS.
    auto limit = [] (double v)
    {
        const double m = std::fabs(v);
        if (m <= 1.3) return v;
        const double y = 1.3 + 0.4 * std::tanh((m - 1.3) / 0.4);
        return v < 0.0 ? -y : y;
    };
    left = limit(a * l1 + b * l2);
    right = limit(a * r1 + b * r2);
}

void PSM3Engine::process(const float* inL, const float* inR, float* outL, float* outR, int numSamples)
{
    if (! prepared) { for (int i = 0; i < numSamples; ++i) { outL[i] = 0.0f; outR[i] = 0.0f; } return; }

    fetchPins();
    // gli interruttori (Mains, Attack) hanno effetto subito, non al prossimo giro di controllo
    mainsOn = target[(int) ParamId::mains] >= 0.5;

    const bool wantLight = ! (active.colUsed[cMeter] || active.colUsed[cOut1] || active.colUsed[cOut2]);
    if (wantLight != lightMode)
    {
        lightMode = wantLight;
        decL.reset(); decR.reset();
        rows.fill(0.0);
    }
    const bool off = ! mainsOn && mainsGain <= 0.0;
    idle.store(lightMode || off, std::memory_order_relaxed);

    if (off)
    {
        for (int i = 0; i < numSamples; ++i) { outL[i] = 0.0f; outR[i] = 0.0f; }
        dk1.allNotesOff();
        lamp.store(false, std::memory_order_relaxed);
        needle.store(target[(int) ParamId::meterMode] >= 0.5 ? 0.0 : 0.5, std::memory_order_relaxed);
        return;
    }

    if (lightMode)
    {
        // Nessun pin verso uscite o strumento: resta vivo solo l'inviluppo (per la spia) e la tastiera.
        for (int i = 0; i < numSamples; ++i)
        {
            outL[i] = 0.0f; outR[i] = 0.0f;
            for (int k = 0; k < factor; ++k)
            {
                if (++controlCounter >= kControlInterval) { controlCounter = 0; updateControls(false); }
                dk1.process();
                if (dk1.takeTrigger()) env.trigger();
                env.setGate(attackHeld || dk1.gate());
                env.process(0.0, 0.0);
                meter.process(0.0);
            }
        }
    }
    else
    {
        double upA[4] = { 0, 0, 0, 0 }, upB[4] = { 0, 0, 0, 0 }, bufL[4], bufR[4];
        const double inScale = 0.9 / kVoltsToFullScale;
        for (int i = 0; i < numSamples; ++i)
        {
            if (inL != nullptr) upL.process((double) inL[i] * inScale, upA);
            if (inR != nullptr) upR.process((double) inR[i] * inScale, upB);
            for (int k = 0; k < factor; ++k) tick(upA[k], upB[k], bufL[k], bufR[k]);
            double l = decL.process(bufL) * kVoltsToFullScale, r = decR.process(bufR) * kVoltsToFullScale;

            if (mainsOn) mainsGain = std::min(1.0, mainsGain + mainsStep);
            else         mainsGain = std::max(0.0, mainsGain - mainsStep);
            l *= mainsGain; r *= mainsGain;
            if (! std::isfinite(l)) l = 0.0;
            if (! std::isfinite(r)) r = 0.0;
            // ultimo fermo: i rari picchi di sovraelongazione del decimatore non superano 0 dBFS
            l = std::min(1.0, std::max(-1.0, l));
            r = std::min(1.0, std::max(-1.0, r));
            outL[i] = (float) l; outR[i] = (float) r;
        }
        if (! mainsOn && mainsGain <= 0.0) { const bool keep = mainsOn; reset(); mainsOn = keep; mainsGain = 0.0; }
    }

    if (mainsOn && lightMode) mainsGain = 1.0;
    else if (! mainsOn && lightMode) mainsGain = 0.0;
    lamp.store(env.lampOn(), std::memory_order_relaxed);
    needle.store(meter.needle(), std::memory_order_relaxed);
}
}
