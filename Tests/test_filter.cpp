#include "check.h"
#include "measure.h"
#include "DSP/DiodeLadderFilter.h"
#include <vector>
using namespace psm3;
static const double SR = 192000.0;

static std::vector<double> oscillate(DiodeLadderFilter& f, double seconds, double cv = 0.0, double settle = 1.0)
{
    f.reset();
    for (int i = 0; i < (int) (SR * settle); ++i) f.process(0.0, cv);       // innesco
    std::vector<double> x((size_t) (seconds * SR)); for (auto& v : x) v = f.process(0.0, cv); return x;
}
// guadagno a regime per una sinusoide di ampiezza amp
static double gainDb(DiodeLadderFilter& f, double freq, double amp = 0.05)
{
    f.reset();
    const int settle = (int) (SR * 0.5), n = (int) (SR * 0.5);
    std::vector<double> y((size_t) n);
    for (int i = 0; i < settle + n; ++i)
    {
        const double v = f.process(amp * std::sin(2.0 * M_PI * freq * i / SR), 0.0);
        if (i >= settle) y[(size_t) (i - settle)] = v;
    }
    return meas::goertzelDb(y, SR, freq) - meas::db(amp);
}

int main()
{
    DiodeLadderFilter f; f.prepare(SR);

    { // oscillatore: 261 Hz con la manopola a 5 (PR6), circa 1,5 V p-p, ragionevolmente sinusoidale
        f.setFrequencyDial(5); f.setResponse(10); f.setLevel(10);
        auto x = oscillate(f, 1.0);
        CHECK_REL(meas::frequency(x, SR), 261.6, 0.02);
        CHECK_REL(meas::peakToPeak(x), 1.5, 0.20);
        const double f0 = meas::frequency(x, SR);
        CHECK(meas::goertzelDb(x, SR, 3.0 * f0) - meas::goertzelDb(x, SR, f0) < -25.0);
        // 0,2 V per ottava
        CHECK_REL(meas::frequency(oscillate(f, 1.0, 0.20), SR), 523.2, 0.03);
        CHECK_REL(meas::frequency(oscillate(f, 1.0, -0.20), SR), 130.8, 0.03);
        // livello
        f.setLevel(5);
        CHECK_REL(meas::peakToPeak(oscillate(f, 0.5)), 0.75, 0.25);
        f.setLevel(10);
    }
    { // Response fa oscillare il filtro solo sopra 5, a tutte le frequenze
        for (double dial : { 1.0, 5.0, 9.0 })
        {
            f.setFrequencyDial(dial);
            const double settle = dial < 3.0 ? 8.0 : 1.0;                   // alle frequenze basse l'innesco e' lento
            f.setResponse(4.5);
            CHECK(meas::rms(oscillate(f, 0.5, 0.0, settle)) < 0.005);
            f.setResponse(7.5);
            CHECK(meas::peakToPeak(oscillate(f, 0.5, 0.0, settle)) > 0.3);
        }
        // gamma: circa 5 Hz - oltre 10 kHz
        f.setResponse(10);
        f.setFrequencyDial(10); CHECK(meas::frequency(oscillate(f, 0.3), SR) > 10000.0);
        f.setFrequencyDial(1);  { const double lf = meas::frequency(oscillate(f, 3.0, 0.0, 8.0), SR); CHECK(lf > 5.0 && lf < 20.0); }
    }
    { // passa-basso: 12 dB nella prima ottava, 18 dB/ottava dopo
        f.setFrequencyDial(5); f.setResponse(0); f.setLevel(10);
        const double g5 = gainDb(f, 5.0), g10 = gainDb(f, 10.0);
        CHECK(std::fabs(g5 - g10) < 1.0);                       // banda passante piatta
        CHECK(g5 > -6.0 && g5 < 3.0);                           // il segnale attraversa il filtro circa a livello
        const double g261 = gainDb(f, 261.6), g523 = gainDb(f, 523.2), g1046 = gainDb(f, 1046.4);
        CHECK(g261 < g5 - 6.0);
        CHECK(g261 - g523 > 10.0 && g261 - g523 < 15.0);
        CHECK(g523 - g1046 > 15.0 && g523 - g1046 < 21.0);
    }
    { // passa-banda / risonanza
        f.setFrequencyDial(5); f.setResponse(5.5);
        double peak = -300.0; for (double fr = 150.0; fr < 400.0; fr *= 1.03) peak = std::max(peak, gainDb(f, fr, 0.005));
        CHECK(peak - gainDb(f, 261.6 / 4.0, 0.005) > 12.0);
    }
    { // saturazione: il filtro lavora meglio con segnali bassi
        f.setFrequencyDial(8); f.setResponse(0);
        auto thdAt = [&](double amp) {
            f.reset(); std::vector<double> y;
            for (int i = 0; i < (int) SR; ++i) { const double v = f.process(amp * std::sin(2.0 * M_PI * 100.0 * i / SR), 0.0); if (i > (int) SR / 2) y.push_back(v); }
            return meas::thd(y, SR, 100.0); };
        CHECK(thdAt(3.0) > 0.03);
        CHECK(thdAt(0.05) < 0.005);
    }
    { // robustezza
        f.setFrequencyDial(5); f.setResponse(10); f.reset();
        bool ok = true;
        for (int i = 0; i < (int) (2.0 * SR); ++i)
        {
            const double in = (i / 300) % 2 ? 12.0 : -12.0, cv = (i / 100) % 2 ? 2.0 : -2.0;
            const double v = f.process(in, cv);
            ok = ok && std::isfinite(v) && std::fabs(v) <= 12.0;
        }
        CHECK(ok);
        CHECK(std::isfinite(f.process(NAN, 0.0)));
        CHECK(std::isfinite(f.process(0.0, 0.0)));
    }
    return checkSummary();
}
