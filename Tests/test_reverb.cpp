#include "check.h"
#include "measure.h"
#include "DSP/SpringReverb.h"
#include "DSP/Volts.h"
#include <vector>
using namespace psm3;
static const double SR = 192000.0;

// burst di rumore da 10 ms seguito da silenzio
static std::vector<double> burst(SpringReverb& r, double seconds, double amp = 0.5, double cv = 0.0)
{
    r.reset(); Rng rng; rng.seed(5);
    std::vector<double> y((size_t) (seconds * SR));
    for (size_t i = 0; i < y.size(); ++i) y[i] = r.process(i < (size_t) (0.010 * SR) ? amp * rng.bipolar() : 0.0, cv);
    return y;
}
static double tailDb(const std::vector<double>& y)      // energia dopo 100 ms rispetto ai primi 10 ms
{
    std::vector<double> head(y.begin(), y.begin() + (long) (0.010 * SR)), tail(y.begin() + (long) (0.1 * SR), y.end());
    return meas::db(meas::rms(tail) / std::max(1e-12, meas::rms(head)));
}

int main()
{
    SpringReverb r; r.prepare(SR); r.setLevel(10);

    { // Mix 0: segnale diretto, nessuna coda
        r.setMix(0); r.reset();
        double num = 0.0, den1 = 0.0, den2 = 0.0;
        for (int i = 0; i < 96000; ++i)
        {
            const double in = 0.5 * std::sin(2.0 * M_PI * 440.0 * i / SR), out = r.process(in, 0.0);
            num += in * out; den1 += in * in; den2 += out * out;
        }
        CHECK(num / std::sqrt(den1 * den2) > 0.999);
        CHECK_REL(std::sqrt(den2 / den1), 1.0, 0.05);
        CHECK(tailDb(burst(r, 1.0)) < -70.0);
        r.setLevel(5); r.reset();
        CHECK_NEAR(r.process(0.4, 0.0), 0.2, 0.01);
        r.setLevel(10);
    }
    { // Mix 10: quasi solo molle; primo arrivo a 25 ms; coda di circa 2 secondi
        r.setMix(10);
        auto y = burst(r, 4.0);
        size_t first = 0; while (first < y.size() && std::fabs(y[first]) < 0.01) ++first;
        const double ms = 1000.0 * (double) first / SR;
        CHECK(ms > 20.0 && ms < 35.0);
        CHECK_REL(meas::rt60(std::vector<double>(y.begin() + (long) (0.05 * SR), y.end()), SR), 2.0, 0.25);
        CHECK(tailDb(y) > -40.0);
    }
    { // Mix controllato in tensione
        r.setMix(5);
        CHECK(tailDb(burst(r, 1.0, 0.5, -2.0)) < -60.0);       // -2 V: asciutto
        r.setMix(0);
        CHECK(tailDb(burst(r, 1.0, 0.5, 4.0)) > -40.0);        // tensione positiva: riverbero
    }
    { // sovraccarico e stabilita'
        r.setMix(10); r.reset();
        double peak = 0.0; bool finite = true;
        for (int i = 0; i < 192000; ++i)
        {
            const double v = r.process(6.0 * std::sin(2.0 * M_PI * 300.0 * i / SR), 0.0);
            finite = finite && std::isfinite(v); peak = std::max(peak, std::fabs(v));
        }
        CHECK(finite && peak <= 2.75);
        double late = 0.0;
        for (int i = 0; i < 6 * 192000; ++i) { const double v = r.process(0.0, 0.0); if (i > 5 * 192000) late = std::max(late, std::fabs(v)); }
        CHECK(late < peak * 1e-4);
    }
    { // funziona anche alle altre frequenze interne
        for (double sr : { 88200.0, 96000.0, 176400.0 })
        {
            SpringReverb q; q.prepare(sr); q.setLevel(10); q.setMix(10); q.reset();
            Rng rng; rng.seed(9); std::vector<double> y((size_t) (3.0 * sr));
            for (size_t i = 0; i < y.size(); ++i) y[i] = q.process(i < (size_t) (0.010 * sr) ? 0.5 * rng.bipolar() : 0.0, 0.0);
            CHECK_REL(meas::rt60(std::vector<double>(y.begin() + (long) (0.05 * sr), y.end()), sr), 2.0, 0.3);
        }
    }
    return checkSummary();
}
