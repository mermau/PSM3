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
    { // ingressi accoppiati in alternata: con una tensione costante su un ingresso l'uscita si spegne
      // ("differenzia": manuale d'uso, esperimenti 9, 12 e 19) e ricompare solo quando la tensione cambia
        r.reset(); std::vector<double> early, late, moved;
        for (int i = 0; i < (int) (3.0 * SR); ++i)
        {
            const double v = r.process(0.5 * std::sin(2.0 * M_PI * 261.0 * i / SR), 1.0);
            if (i < (int) (0.02 * SR)) early.push_back(v);
            if (i > (int) (2.5 * SR)) late.push_back(v);
        }
        CHECK(meas::rms(early) > 0.2);                               // il gradino iniziale fa passare il suono
        CHECK(meas::db(meas::rms(late) / meas::rms(early)) < -40.0); // poi tace
        for (int i = 0; i < (int) (0.02 * SR); ++i) moved.push_back(r.process(0.5 * std::sin(2.0 * M_PI * 261.0 * i / SR), -1.0));
        CHECK(meas::rms(moved) > 0.2);                               // la tensione si muove: il suono torna
        // un'onda quadra lenta spezzetta il suono senza spegnerlo (esperimento 19, E5)
        r.reset(); std::vector<double> chopped;
        for (int i = 0; i < (int) (1.0 * SR); ++i) chopped.push_back(r.process(0.5 * std::sin(2.0 * M_PI * 261.0 * i / SR), (i / 9600) % 2 ? 1.0 : -1.0));
        CHECK(meas::rms(chopped) > 0.2);
    }
    { // sovraccarico: non oltre circa 6 V p-p
        auto x = run(r, sine(300.0, 6.0), sine(1000.0, 6.0));
        CHECK(meas::peakToPeak(x) <= 6.6 && meas::peakToPeak(x) >= 4.5);
    }
    r.setLevel(0);
    CHECK(meas::peakToPeak(run(r, sine(300.0, 0.75), sine(1000.0, 0.75), 0.1)) < 1e-6);
    return checkSummary();
}
