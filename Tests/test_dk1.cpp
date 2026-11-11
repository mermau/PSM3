#include "check.h"
#include "measure.h"
#include "DSP/KeyboardDK1.h"
#include "DSP/InputAmp.h"
#include "DSP/Matrix.h"
#include "DSP/Oscillator1.h"
#include <vector>
using namespace psm3;
static const double SR = 192000.0;

static KeyboardDK1 make()
{
    KeyboardDK1 k; k.prepare(SR);
    k.setFrequency(5); k.setTuningSpread(5); k.setLevel(10); k.setDynamicRange(10);
    k.setCh1Signal(false); k.setCh2Dynamic(false); k.reset();
    return k;
}

int main()
{
    { // dk1_voltage
        auto k = make();
        k.noteOn(66, 0.8); CHECK_NEAR(k.process().ch1, 0.0, 1e-9);
        k.noteOff(66);
        k.noteOn(78, 0.8); const double up = k.process().ch1;
        CHECK_NEAR(std::fabs(up), 1.0, 1e-6);
        CHECK_NEAR(k.process().ch2, up, 1e-9);                      // Ch 2 in modo Keyboard Voltage
        k.noteOff(78);
        CHECK_NEAR(k.process().ch1, up, 1e-9);                      // la tensione resta memorizzata
        k.noteOn(84, 0.8); const double top = k.process().ch1; k.noteOff(84);
        k.noteOn(48, 0.8); const double bottom = k.process().ch1; k.noteOff(48);
        CHECK_NEAR(std::fabs(top), 1.5, 1e-6);
        CHECK_NEAR(top, -bottom, 1e-9);
    }
    { // dk1_octave: tastiera cromatica attraverso Input Amp, pin rosso e Oscillatore 1
        PinGrid g; g.fill(0); g[(size_t) (7 * 16 + 8)] = (uint8_t) Pin::Red;       // riga 8 (Input 1) -> colonna I
        MatrixSolution sol; Matrix::build(g, 123, kRowOutOhms, sol);
        InputAmp amp; amp.setLevel(KeyboardDK1::kChromaticInputLevel);
        auto freqOf = [&](int note) {
            auto k = make(); Oscillator1 o; o.prepare(SR); o.setRampLevel(10); o.setFrequencyDial(6);
            k.noteOn(note, 0.8);
            std::array<double, 16> rows {}, cols {};
            std::vector<double> x((size_t) SR);
            for (auto& v : x) { rows[7] = amp.process(k.process().ch1); Matrix::apply(sol, rows, cols); v = o.process(cols[8]).ramp; }
            return meas::frequency(x, SR); };
        const double f60 = freqOf(60), f72 = freqOf(72), f61 = freqOf(61);
        CHECK(f72 > f60);                                           // la nota piu' alta e' piu' acuta
        CHECK_REL(f72 / f60, 2.0, 0.005);
        CHECK_REL(f61 / f60, std::pow(2.0, 1.0 / 12.0), 0.002);
        CHECK(KeyboardDK1::kChromaticInputLevel > 5.0 && KeyboardDK1::kChromaticInputLevel < 8.5);
    }
    { // dk1_signal: oscillatore di tastiera
        auto k = make(); k.setCh1Signal(true);
        auto freqOf = [&](int note, double vel = 1.0) {
            k.reset(); k.noteOn(note, vel);
            std::vector<double> x((size_t) SR); for (auto& v : x) v = k.process().ch1;
            k.noteOff(note); return x; };
        CHECK_REL(meas::frequency(freqOf(67), SR), 261.6, 0.01);
        CHECK_REL(meas::frequency(freqOf(79), SR), 523.2, 0.01);
        k.setTuningSpread(2.5);
        CHECK_REL(meas::frequency(freqOf(79), SR) / 261.6, std::sqrt(2.0), 0.01);    // quarti di tono
        k.setTuningSpread(5);
        k.setFrequency(0);  CHECK(meas::frequency(freqOf(48), SR) < 30.0);
        k.setFrequency(10); CHECK(meas::frequency(freqOf(84), SR) > 8000.0);
        k.setFrequency(5);
        CHECK(meas::peakToPeak(freqOf(67, 1.0)) > 1.5 * meas::peakToPeak(freqOf(67, 0.2)));
        k.reset();
        CHECK(k.process().ch1 == 0.0);                              // nessun tasto: silenzio
        k.noteOn(67, 1.0); for (int i = 0; i < 1000; ++i) k.process(); k.noteOff(67);
        double tail = 0.0; for (int i = 0; i < 20000; ++i) { const double v = std::fabs(k.process().ch1); if (i > 10000) tail = std::max(tail, v); }
        CHECK(tail == 0.0);
        k.setLevel(0); k.noteOn(67, 1.0);
        CHECK(std::fabs(k.process().ch1) < 1e-9);
    }
    { // dk1_dynamic
        auto k = make(); k.setCh2Dynamic(true);
        k.noteOn(60, 1.0); CHECK_REL(std::fabs(k.process().ch2), 3.0, 0.05); k.noteOff(60);
        k.noteOn(60, 0.5); CHECK_REL(std::fabs(k.process().ch2), 1.5, 0.05); k.noteOff(60);
        k.setDynamicRange(0); k.noteOn(60, 1.0); CHECK(std::fabs(k.process().ch2) < 1e-9);
    }
    { // dk1_note_stack
        auto k = make();
        CHECK(! k.gate() && ! k.takeTrigger());
        k.noteOn(60, 0.8);
        CHECK(k.gate() && k.takeTrigger() && ! k.takeTrigger());
        const double v60 = k.process().ch1;
        k.noteOn(64, 0.8);
        CHECK(! k.takeTrigger());                                   // legato: nessun nuovo attacco
        CHECK(k.process().ch1 != v60);
        k.noteOff(64);
        CHECK(k.gate() && ! k.takeTrigger());
        CHECK_NEAR(k.process().ch1, v60, 1e-9);                     // torna la nota ancora premuta
        k.noteOff(60);
        CHECK(! k.gate());
        k.noteOn(62, 0.8);
        CHECK(k.takeTrigger());
        k.noteOff(90);                                              // mai suonata
        CHECK(k.gate());
        k.noteOff(62);
        for (int n = 30; n < 50; ++n) k.noteOn(n, 0.5);             // piu' note della pila
        CHECK(k.gate());
        k.allNotesOff();
        CHECK(! k.gate());
        for (int n = 30; n < 50; ++n) k.noteOff(n);
        CHECK(! k.gate());
        k.noteOn(60, 0.8); k.noteOn(60, 0.8); k.noteOff(60);        // stessa nota due volte: un solo rilascio basta
        CHECK(! k.gate());
        k.noteOn(0, 1.0);   CHECK_NEAR(std::fabs(k.process().ch1), 5.5, 1e-6); k.noteOff(0);
        k.noteOn(127, 1.0); CHECK_NEAR(std::fabs(k.process().ch1), 61.0 / 12.0, 1e-6); k.noteOff(127);
        k.noteOn(-5, 1.0); k.noteOn(300, 2.0);                      // valori assurdi: ignorati o limitati
        CHECK(std::isfinite(k.process().ch1) && std::isfinite(k.process().ch2));
    }
    return checkSummary();
}
