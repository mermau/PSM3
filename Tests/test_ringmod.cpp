#include "check.h"
#include "measure.h"
#include "DSP/RingModulator.h"
#include <vector>
using namespace psm3;
static const double SR = 192000.0;

template <typename A, typename B> static std::vector<double> run(RingModulator& r, A a, B b, double seconds = 1.0)
{
    r.reset(); std::vector<double> x((size_t) (seconds * SR));
    for (size_t i = 0; i < x.size(); ++i) x[i] = r.process(a((double) i / SR), b((double) i / SR));
    return x;
}
static auto sine(double f, double amp) { return [=](double t) { return amp * std::sin(2.0 * M_PI * f * t); }; }
static auto dc(double v) { return [=](double) { return v; }; }

int main()
{
    RingModulator r; r.prepare(SR); r.setLevel(10); r.setDriftSeed(3);

    { // stesso segnale ai due ingressi: frequenza doppia, almeno grande quanto l'ingresso
        auto x = run(r, sine(261.0, 0.75), sine(261.0, 0.75));
        CHECK(meas::peakToPeak(x) >= 1.5);
        CHECK(meas::goertzelDb(x, SR, 261.0) - meas::goertzelDb(x, SR, 522.0) < -40.0);
    }
    { // somma e differenza
        auto x = run(r, sine(300.0, 0.75), sine(1000.0, 0.75));
        const double side = meas::goertzelDb(x, SR, 700.0);
        CHECK_NEAR(side, meas::goertzelDb(x, SR, 1300.0), 0.5);
        CHECK(meas::goertzelDb(x, SR, 300.0) < side - 45.0);
        CHECK(meas::goertzelDb(x, SR, 1000.0) < side - 45.0);
    }
    { // un solo ingresso: reiezione di almeno 50 dB
        auto xa = run(r, sine(261.0, 0.75), dc(0.0));
        CHECK(meas::db(meas::rms(xa) / (0.75 / std::sqrt(2.0))) < -50.0);
        CHECK(meas::rms(xa) > 0.0);                                  // ma non perfetta
        auto xb = run(r, dc(0.0), sine(261.0, 0.75));
        CHECK(meas::db(meas::rms(xb) / (0.75 / std::sqrt(2.0))) < -55.0);
    }
    { // accoppiato in continua: una tensione costante lascia passare l'altro ingresso
        auto x = run(r, sine(261.0, 0.5), dc(1.0));
        CHECK(meas::goertzelDb(x, SR, 261.0) > meas::db(0.5));
        auto y = run(r, sine(261.0, 0.5), dc(-1.0));
        CHECK(x[1000] * y[1000] < 0.0);                              // segno invertito
    }
    { // sovraccarico: non oltre circa 6 V p-p
        auto x = run(r, sine(300.0, 6.0), sine(1000.0, 6.0));
        CHECK(meas::peakToPeak(x) <= 6.6 && meas::peakToPeak(x) >= 4.5);
    }
    r.setLevel(0);
    CHECK(meas::peakToPeak(run(r, sine(300.0, 0.75), sine(1000.0, 0.75), 0.1)) < 1e-6);
    return checkSummary();
}
