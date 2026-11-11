#include "check.h"
#include "measure.h"
#include "DSP/InputAmp.h"
#include "DSP/OutputAmp.h"
#include "DSP/Meter.h"
#include <vector>
using namespace psm3;
static const double SR = 192000.0;

static double gainDb(OutputAmp& a, double freq, double cv = 0.0, double amp = 0.1)
{
    a.reset(); std::vector<double> y;
    for (int i = 0; i < (int) (SR * 0.6); ++i)
    {
        const double v = a.process(amp * std::sin(2.0 * M_PI * freq * i / SR), cv);
        if (i > (int) (SR * 0.3)) y.push_back(v);
    }
    return meas::goertzelDb(y, SR, freq) - meas::db(amp);
}

int main()
{
    { // amplificatore d'ingresso: invertente, guadagno 2 a fondo corsa, potenziometro logaritmico
        InputAmp in; in.setLevel(10);
        CHECK_NEAR(in.process(1.0), -2.0, 1e-9);
        in.setLevel(0);
        CHECK(in.process(1.0) == 0.0);
        in.setLevel(5);
        CHECK(std::fabs(in.process(1.0)) > 0.1 && std::fabs(in.process(1.0)) < 0.4);
        in.setLevel(10);
        CHECK(in.process(100.0) == -9.0 && in.process(-100.0) == 12.0);     // satura alle alimentazioni
    }
    { // amplificatore d'uscita
        OutputAmp a; a.prepare(SR); a.setFilter(5); a.setPan(5);
        a.setLevel(0); a.reset();
        bool silent = true; for (int i = 0; i < 1000; ++i) silent = silent && a.process(2.0 * std::sin(i * 0.1), 0.0) == 0.0;
        CHECK(silent);                                                       // -70 dB tarato a zero (PR4)
        a.setLevel(5);  const double g5 = gainDb(a, 1000.0);
        CHECK_NEAR(g5, -3.5, 0.6);                                           // rampa piena -> circa 1 V p-p a 5
        a.setLevel(10); const double g10 = gainDb(a, 1000.0);
        CHECK_NEAR(g10, 6.0, 0.6);
        double prev = -200.0; bool mono = true;
        for (double l = 0.5; l <= 10.0; l += 0.5) { a.setLevel(l); const double g = gainDb(a, 1000.0, 0.0, 0.01); mono = mono && g > prev; prev = g; }
        CHECK(mono);
        // controllo in tensione: 15 dB per volt
        a.setLevel(3);
        CHECK_NEAR(gainDb(a, 1000.0, 1.0, 0.01) - gainDb(a, 1000.0, 0.0, 0.01), 15.0, 1.0);
        a.setLevel(10); a.reset();
        silent = true; for (int i = 0; i < 1000; ++i) silent = silent && a.process(2.0 * std::sin(i * 0.1), -6.0) == 0.0;
        CHECK(silent);                                                       // una tensione grande chiude il canale
        // filtro di tono
        a.setLevel(5);
        a.setFilter(5);
        CHECK(std::fabs(gainDb(a, 100.0) - gainDb(a, 1000.0)) < 1.0 && std::fabs(gainDb(a, 8000.0) - gainDb(a, 1000.0)) < 1.0);
        a.setFilter(0);  CHECK(gainDb(a, 100.0) - gainDb(a, 8000.0) > 12.0);
        a.setFilter(10); CHECK(gainDb(a, 8000.0) - gainDb(a, 100.0) > 12.0);
        a.setFilter(5);
        // pan a potenza costante
        double l, r;
        a.setPan(0);  a.panGains(l, r); CHECK_NEAR(l, 1.0, 1e-6); CHECK_NEAR(r, 0.0, 1e-6);
        a.setPan(10); a.panGains(l, r); CHECK_NEAR(l, 0.0, 1e-6); CHECK_NEAR(r, 1.0, 1e-6);
        a.setPan(5);  a.panGains(l, r); CHECK_NEAR(l, 0.7071, 0.01); CHECK_NEAR(r, 0.7071, 0.01);
        // altoparlante interno: banda stretta
        a.setSpeaker(true);
        CHECK(gainDb(a, 1000.0) - gainDb(a, 40.0) > 15.0);
        a.setSpeaker(false);
        // limite d'uscita
        a.setLevel(10); a.reset(); double peak = 0.0;
        for (int i = 0; i < 20000; ++i) peak = std::max(peak, std::fabs(a.process(10.0 * std::sin(i * 0.01), 0.0)));
        CHECK(peak <= 2.75 && peak > 2.0);
        CHECK(std::isfinite(a.process(NAN, NAN)));
    }
    { // strumento
        Meter m; m.prepare(SR);
        m.setMode(Meter::Mode::ControlVoltage); m.reset();
        for (int i = 0; i < (int) SR; ++i) m.process(0.0);
        CHECK_NEAR(m.needle(), 0.5, 0.01);
        for (int i = 0; i < (int) SR; ++i) m.process(2.0);
        CHECK_NEAR(m.needle(), 0.7, 0.02);
        for (int i = 0; i < (int) SR; ++i) m.process(-50.0);
        CHECK(m.needle() >= 0.0 && m.needle() < 0.02);
        m.setMode(Meter::Mode::SignalLevel); m.reset();
        for (int i = 0; i < (int) SR; ++i) m.process(0.0);
        CHECK_NEAR(m.needle(), 0.0, 0.01);
        for (int i = 0; i < (int) (2 * SR); ++i) m.process(0.75 * std::sin(2.0 * M_PI * 261.0 * i / SR));
        CHECK_NEAR(m.needle(), 0.375, 0.05);                                 // 1,5 V p-p
        // balistica: non arriva subito
        m.setMode(Meter::Mode::ControlVoltage); m.reset();
        for (int i = 0; i < (int) (0.01 * SR); ++i) m.process(5.0);
        CHECK(m.needle() < 0.7);
    }
    return checkSummary();
}
