#include "check.h"
#include "measure.h"
#include "DSP/PSM3Engine.h"
#include "DSP/KeyboardDK1.h"
#include <atomic>
#include <thread>
#include <vector>
using namespace psm3;

// Griglia da sigle come sul pannello: "B1" = colonna B, riga 1.
static PinGrid grid(std::initializer_list<const char*> pins, Pin p = Pin::White)
{
    PinGrid g; g.fill(0);
    for (const char* s : pins) { const int col = s[0] - 'A', row = std::atoi(s + 1) - 1; g[(size_t) (row * 16 + col)] = (uint8_t) p; }
    return g;
}
static void setRed(PinGrid& g, const char* s) { g[(size_t) ((std::atoi(s + 1) - 1) * 16 + (s[0] - 'A'))] = (uint8_t) Pin::Red; }

struct Host
{
    PSM3Engine e; double sr; int block;
    explicit Host(double sampleRate = 48000.0, int blockSize = 512) : sr(sampleRate), block(blockSize)
    {
        e.setDriftEnabled(false);
        e.prepare(sr, block);
    }
    void set(ParamId id, double v) { e.setParam(id, v); }
    std::vector<double> run(double seconds, std::vector<double>* right = nullptr)
    {
        std::vector<float> l((size_t) block), r((size_t) block);
        std::vector<double> out;
        long remaining = (long) (seconds * sr);
        while (remaining > 0)
        {
            const int n = (int) std::min<long>(block, remaining);
            e.process(nullptr, nullptr, l.data(), r.data(), n);
            for (int i = 0; i < n; ++i) { out.push_back(l[(size_t) i]); if (right) right->push_back(r[(size_t) i]); }
            remaining -= n;
        }
        return out;
    }
};
static void keyboardPatch(Host& h)
{
    auto g = grid({ "D1", "B12", "C12" }); setRed(g, "I8");
    h.e.setPins(g, 99);
    h.set(ParamId::osc1Freq, 6.0 + 0.5 / 1.5);          // do centrale = 261,6 Hz
    h.set(ParamId::osc1Sine, 7); h.set(ParamId::envSignal, 8); h.set(ParamId::envOff, 10);
    h.set(ParamId::envAttack, 0); h.set(ParamId::envOn, 0); h.set(ParamId::envDecay, 5);
    h.set(ParamId::in1Level, KeyboardDK1::kChromaticInputLevel); h.set(ParamId::kbCh1Mode, 1);
    h.set(ParamId::out1Level, 5); h.set(ParamId::out2Level, 5);
}
static bool allFinite(const std::vector<double>& x, double limit)
{
    for (double v : x) if (! std::isfinite(v) || std::fabs(v) > limit) return false;
    return true;
}

int main()
{
    { // engine_basic
        Host h;
        CHECK(h.e.isIdle());
        auto silent = h.run(0.2);
        CHECK(meas::peakToPeak(silent) == 0.0);
        h.e.setPins(grid({ "B1" }), 1);
        h.set(ParamId::osc1Sine, 6); h.set(ParamId::out1Level, 7);
        std::vector<double> r; auto l = h.run(1.5, &r);
        CHECK(! h.e.isIdle());
        std::vector<double> tail(l.end() - 48000, l.end()), rtail(r.end() - 48000, r.end());
        CHECK_REL(meas::frequency(tail, h.sr), 261.6, 0.01);
        CHECK(meas::peakToPeak(tail) > 0.05 && meas::peakToPeak(tail) < 1.0);
        CHECK_REL(meas::rms(rtail), meas::rms(tail), 0.01);                 // pan al centro
        h.set(ParamId::out1Pan, 0);
        r.clear(); h.run(0.5, &r); r.clear(); h.run(0.2, &r);
        CHECK(meas::peakToPeak(r) < 1e-4);                                  // tutto a sinistra
        h.e.setPins(grid({}), 1);
        h.run(0.3);
        CHECK(meas::peakToPeak(h.run(0.2)) == 0.0);
        CHECK(h.e.latencySamples() >= 0 && h.e.latencySamples() < 128);
    }
    { // engine_patch1: rumore -> filtro -> inviluppo -> uscite (esempio del manuale, sezione 3)
        Host h;
        h.e.setPins(grid({ "H7", "D10", "B12", "C12" }), 5);
        h.set(ParamId::noiseLevel, 7); h.set(ParamId::filtFreq, 8); h.set(ParamId::filtLevel, 9);
        h.set(ParamId::out1Level, 7); h.set(ParamId::out2Level, 7);
        h.set(ParamId::envAttack, 2); h.set(ParamId::envOn, 2); h.set(ParamId::envDecay, 4); h.set(ParamId::envOff, 3);
        h.set(ParamId::envSignal, 8);
        auto x = h.run(4.0);
        CHECK(meas::rms(x) > 0.005);
        // pulsa: confronta finestre da 50 ms
        double lo = 1e9, hi = 0.0;
        for (size_t i = 0; i + 2400 <= x.size(); i += 2400) { std::vector<double> w(x.begin() + (long) i, x.begin() + (long) i + 2400); const double r = meas::rms(w); lo = std::min(lo, r); hi = std::max(hi, r); }
        CHECK(hi > 10.0 * (lo + 1e-9));
        CHECK(allFinite(x, 2.0));
    }
    { // engine_fm: Osc 2 modula la frequenza di Osc 1
        Host h;
        h.e.setPins(grid({ "B1", "I4" }), 1);
        h.set(ParamId::osc1Sine, 6); h.set(ParamId::out1Level, 6);
        h.set(ParamId::osc2Freq, 6.0 + std::log2(5.0 / 261.6) / 1.5); h.set(ParamId::osc2Tri, 3);
        auto x = h.run(2.0);
        double fmin = 1e9, fmax = 0.0;
        for (size_t i = 4800; i + 1920 <= x.size(); i += 960) { std::vector<double> w(x.begin() + (long) i, x.begin() + (long) i + 1920); const double f = meas::frequency(w, h.sr); if (f > 0) { fmin = std::min(fmin, f); fmax = std::max(fmax, f); } }
        CHECK(fmax > fmin * 1.01);
    }
    { // engine_keyboard
        Host h; keyboardPatch(h);
        CHECK(meas::peakToPeak(h.run(0.3)) < 1e-3);                         // nessun tasto: inviluppo chiuso
        h.e.noteOn(60, 0.8);
        auto a = h.run(1.0);
        CHECK(h.e.envelopeLamp());
        std::vector<double> a2(a.end() - 24000, a.end());
        CHECK_REL(meas::frequency(a2, h.sr), 261.6, 0.01);
        h.e.noteOff(60); h.run(0.05);
        CHECK(! h.e.envelopeLamp());
        h.run(1.0);
        CHECK(meas::rms(h.run(0.2)) < 0.003 * meas::rms(a2));               // chiuso: resta solo il residuo del VCA (circa -56 dB)
        h.e.noteOn(72, 0.8);
        auto b = h.run(1.0); std::vector<double> b2(b.end() - 24000, b.end());
        CHECK_REL(meas::frequency(b2, h.sr) / meas::frequency(a2, h.sr), 2.0, 0.01);
        h.e.noteOn(69, 0.8); h.e.noteOff(72);                               // legato verso il la
        auto c = h.run(0.5); std::vector<double> c2(c.end() - 12000, c.end());
        CHECK_REL(meas::frequency(c2, h.sr), 440.0, 0.01);
        h.e.allNotesOff(); h.run(2.0);
        CHECK(meas::rms(h.run(0.2)) < 3e-4);
        // pulsante Attack
        h.set(ParamId::attackButton, 1); h.run(0.2);
        CHECK(h.e.envelopeLamp() && meas::rms(h.run(0.2)) > 0.01);
        h.set(ParamId::attackButton, 0);
    }
    { // engine_stability: tutti i pin, tutte le manopole a 10
        Host h;
        PinGrid full; full.fill((uint8_t) Pin::White);
        h.e.setPins(full, 7);
        for (int i = 0; i < (int) ParamId::Count; ++i)
            if (! paramInfo((ParamId) i).isSwitch) h.set((ParamId) i, 10);
        auto x = h.run(10.0);
        CHECK(allFinite(x, 2.0));
        h.e.setPins(grid({}), 7);
        h.run(1.0);
        CHECK(meas::peakToPeak(h.run(0.2)) == 0.0);
        for (int i = 0; i < (int) ParamId::Count; ++i) h.set((ParamId) i, paramInfo((ParamId) i).def);
        h.e.setPins(grid({ "B1" }), 7);
        h.set(ParamId::osc1Sine, 6); h.set(ParamId::out1Level, 7);
        auto y = h.run(1.0); std::vector<double> y2(y.end() - 24000, y.end());
        CHECK_REL(meas::frequency(y2, h.sr), 261.6, 0.01);                  // dopo, suona ancora
        // retroazione estrema: filtro su se stesso (segnale e frequenza) e nel riverbero
        h.e.setPins(grid({ "H10", "N10", "G10", "H14", "B10", "C14", "M10" }), 7);
        h.set(ParamId::filtResponse, 10); h.set(ParamId::filtLevel, 10); h.set(ParamId::revLevel, 10); h.set(ParamId::revMix, 10);
        h.set(ParamId::out1Level, 10); h.set(ParamId::out2Level, 10);
        CHECK(allFinite(h.run(5.0), 1.0));                                  // l'uscita del plugin non supera mai 0 dBFS
        for (double sr : { 44100.0, 192000.0 })
        {
            Host q(sr, 512);
            // caso peggiore: tutto a 10 nelle due uscite, entrambe girate a sinistra
            q.e.setPins(grid({ "H10", "N10", "B10", "C10", "B2", "C2", "B3", "C3", "B5", "C5", "B7", "C7", "O5", "P6" }), 7);
            for (int i = 0; i < (int) ParamId::Count; ++i) if (! paramInfo((ParamId) i).isSwitch) q.set((ParamId) i, 10);
            q.set(ParamId::out1Pan, 0); q.set(ParamId::out2Pan, 0);
            CHECK(allFinite(q.run(3.0), 1.0));
        }
    }
    { // engine_pin_swap_live: pin cambiati da un altro thread mentre suona
        Host h; keyboardPatch(h);
        h.e.noteOn(60, 0.8);
        auto g1 = grid({ "D1", "B12", "C12", "I8" }), g2 = grid({ "D2", "B12", "C12", "I8", "H1", "B10" });
        std::atomic<bool> stop { false };
        std::thread t([&] { bool flip = false; while (! stop) { h.e.setPins(flip ? g1 : g2, 3); flip = ! flip; std::this_thread::sleep_for(std::chrono::microseconds(700)); } });
        auto x = h.run(2.0);
        stop = true; t.join();
        CHECK(allFinite(x, 2.0));
        double jump = 0.0; for (size_t i = 1; i < x.size(); ++i) jump = std::max(jump, std::fabs(x[i] - x[i - 1]));
        CHECK(jump < 1.0);
        h.e.noteOff(60);
    }
    { // engine_rates: stessa intonazione a ogni frequenza di campionamento e dimensione di blocco
        for (double sr : { 44100.0, 48000.0, 96000.0, 192000.0 })
            for (int block : { 1, 64, 4096 })
            {
                if (block == 1 && sr > 48000.0) continue;
                Host h(sr, block);
                h.e.prepare(sr, block); h.e.prepare(sr, block);             // prepare ripetuto
                h.e.setPins(grid({ "B1" }), 1);
                h.set(ParamId::osc1Sine, 6); h.set(ParamId::out1Level, 7);
                auto x = h.run(1.0); std::vector<double> t(x.begin() + (long) (sr * 0.5), x.end());
                CHECK_REL(meas::frequency(t, sr), 261.6, 0.005);
            }
        Host q(48000.0, 256); q.e.setQuality(2); q.e.prepare(48000.0, 256);
        q.e.setPins(grid({ "B1" }), 1); q.set(ParamId::osc1Sine, 6); q.set(ParamId::out1Level, 7);
        auto x = q.run(1.0); std::vector<double> t(x.begin() + 24000, x.end());
        CHECK_REL(meas::frequency(t, 48000.0), 261.6, 0.005);
    }
    { // engine_alias: rampa a 5 kHz
        Host h;
        h.e.setPins(grid({ "B2" }), 1);
        h.set(ParamId::osc1Freq, 6.0 + std::log2(5000.0 / 261.6) / 1.5); h.set(ParamId::osc1Ramp, 5); h.set(ParamId::out1Level, 5);
        auto x = h.run(1.5); std::vector<double> t(x.end() - 48000, x.end());
        const double f0 = meas::frequency(t, h.sr), fund = meas::goertzelDb(t, h.sr, f0);
        double worst = -300.0;
        for (double f = 100.0; f < 20000.0; f += 41.0)
        {
            const double k = f / f0; if (std::fabs(k - std::round(k)) * f0 < 150.0) continue;
            worst = std::max(worst, meas::goertzelDb(t, h.sr, f) - fund);
        }
        CHECK(worst < -70.0);
    }
    { // engine_mains e ingresso audio
        Host h;
        h.e.setPins(grid({ "B1" }), 1); h.set(ParamId::osc1Sine, 6); h.set(ParamId::out1Level, 7);
        h.run(0.3);
        h.set(ParamId::mains, 0); h.run(0.05);
        CHECK(meas::peakToPeak(h.run(0.2)) == 0.0 && h.e.isIdle());
        h.set(ParamId::mains, 1);
        CHECK(meas::peakToPeak(h.run(0.3)) > 0.05);

        Host in;                                                            // ingresso audio -> Input Ch 1 -> uscita
        in.e.setPins(grid({ "B8" }), 1); in.set(ParamId::in1Level, 8); in.set(ParamId::out1Level, 7); in.set(ParamId::kbCh1Mode, 0);
        std::vector<float> src(48000), l(48000), r(48000);
        for (size_t i = 0; i < src.size(); ++i) src[i] = 0.25f * (float) std::sin(2.0 * M_PI * 440.0 * (double) i / 48000.0);
        for (int i = 0; i < 48000; i += 480) in.e.process(src.data() + i, src.data() + i, l.data() + i, r.data() + i, 480);
        std::vector<double> y(l.begin() + 24000, l.end());
        CHECK_REL(meas::frequency(y, 48000.0), 440.0, 0.01);
        CHECK(meas::peakToPeak(y) > 0.02);
    }
    { // spia e strumento
        Host h;
        h.e.setPins(grid({ "A16" }), 1);                                    // joystick verticale sul meter
        h.set(ParamId::joyY, 10); h.run(1.0);
        CHECK(h.e.meterNeedle() > 0.6);
        h.set(ParamId::joyY, 0); h.run(1.0);
        CHECK(h.e.meterNeedle() < 0.4);
        h.set(ParamId::envOff, 3); h.set(ParamId::envOn, 5);
        bool on = false, off = false;
        for (int i = 0; i < 100; ++i) { h.run(0.05); if (h.e.envelopeLamp()) on = true; else off = true; }
        CHECK(on && off);                                                   // l'inviluppo ricicla e la spia lampeggia
    }
    return checkSummary();
}
