#include "check.h"
#include "measure.h"
#include "DSP/Oscillator1.h"
#include "DSP/Oscillator2.h"
#include <vector>
using namespace psm3;
static const double SR = 192000.0;

template <typename Fn> static std::vector<double> run(double seconds, Fn fn, double sr = SR)
{
    std::vector<double> x((size_t) (seconds * sr));
    for (auto& v : x) v = fn();
    return x;
}
// Peggior componente non armonica tra 100 Hz e 20 kHz, in dB rispetto alla fondamentale.
static double worstAlias(const std::vector<double>& x, double sr, double f0)
{
    const double fund = meas::goertzelDb(x, sr, f0);
    double worst = -300.0;
    for (double f = 100.0; f < 20000.0; f += 37.0)
    {
        const double k = f / f0;
        if (std::fabs(k - std::round(k)) * f0 < 150.0) continue;
        worst = std::max(worst, meas::goertzelDb(x, sr, f) - fund);
    }
    return worst;
}

int main()
{
    { // Osc 1: taratura del quadrante
        Oscillator1 o; o.prepare(SR); o.setRampLevel(10); o.setSineLevel(10); o.setShape(5);
        const double dials[] = { 4, 6, 8 }, freqs[] = { 32.7, 261.6, 2093.0 };
        for (int i = 0; i < 3; ++i)
        {
            o.reset(); o.setFrequencyDial(dials[i]);
            auto x = run(1.0, [&] { return o.process(0.0).ramp; });
            CHECK_REL(meas::frequency(x, SR), freqs[i], 0.01);
        }
        o.reset(); o.setFrequencyDial(10);
        CHECK(meas::frequency(run(0.2, [&] { return o.process(0.0).sine; }), SR) >= 10000.0);
        o.reset(); o.setFrequencyDial(0);
        { auto x = run(4.0, [&] { return o.process(0.0).sine; }, SR); double f = meas::frequency(x, SR); CHECK(f > 0.0 && f <= 1.1); }

        // controllo in tensione: 0,32 V per ottava
        o.reset(); o.setFrequencyDial(6);
        CHECK_REL(meas::frequency(run(1.0, [&] { return o.process(0.32).ramp; }), SR), 523.2, 0.01);
        o.reset();
        CHECK_REL(meas::frequency(run(1.0, [&] { return o.process(-0.32).ramp; }), SR), 130.8, 0.01);

        // livelli
        o.reset();
        CHECK_REL(meas::peakToPeak(run(0.5, [&] { return o.process(0.0).sine; })), 2.0, 0.05);
        o.reset();
        CHECK_REL(meas::peakToPeak(run(0.5, [&] { return o.process(0.0).ramp; })), 2.0, 0.05);
        o.setSineLevel(0); o.setRampLevel(0); o.reset();
        CHECK(meas::peakToPeak(run(0.5, [&] { auto r = o.process(0.0); return r.sine + r.ramp; })) < 0.001);

        // purezza della sinusoide a shape 5, deformazione agli estremi
        o.setSineLevel(10); o.setShape(5); o.reset();
        auto s5 = run(1.0, [&] { return o.process(0.0).sine; });
        const double f1 = meas::goertzelDb(s5, SR, 261.6);
        CHECK(meas::goertzelDb(s5, SR, 523.2) - f1 < -40.0);
        CHECK(meas::goertzelDb(s5, SR, 784.8) - f1 < -40.0);
        for (double sh : { 0.0, 10.0 })
        {
            o.setShape(sh); o.reset();
            auto s = run(1.0, [&] { return o.process(0.0).sine; });
            CHECK(meas::goertzelDb(s, SR, 523.2) - meas::goertzelDb(s, SR, 261.6) > -20.0);
        }
        // shape 0 e shape 10 sono speculari
        o.setShape(0); o.reset(); auto a = run(0.1, [&] { return o.process(0.0).sine; });
        o.setShape(10); o.reset(); auto b = run(0.1, [&] { return o.process(0.0).sine; });
        CHECK(std::fabs(*std::max_element(a.begin(), a.end()) + *std::min_element(b.begin(), b.end())) < 0.1);

        // alias della rampa a 5 kHz
        o.setRampLevel(10); o.setFrequencyDial(6.0 + std::log2(5000.0 / 261.6) / 1.5); o.reset();
        auto r5k = run(1.0, [&] { return o.process(0.0).ramp; });
        CHECK_REL(meas::frequency(r5k, SR), 5000.0, 0.01);
        CHECK(worstAlias(r5k, SR, meas::frequency(r5k, SR)) < -70.0);
    }
    { // Osc 2
        Oscillator2 o; o.prepare(SR); o.setSquareLevel(10); o.setTriangleLevel(10); o.setShape(5);
        const double dials[] = { 4, 6, 8 }, freqs[] = { 32.7, 261.6, 2093.0 };
        for (int i = 0; i < 3; ++i)
        {
            o.reset(); o.setFrequencyDial(dials[i]);
            CHECK_REL(meas::frequency(run(1.0, [&] { return o.process(0.0).triangle; }), SR), freqs[i], 0.01);
        }
        o.setFrequencyDial(6); o.reset();
        auto sq = run(1.0, [&] { return o.process(0.0).square; });
        o.reset();
        auto tr = run(1.0, [&] { return o.process(0.0).triangle; });
        CHECK_REL(meas::peakToPeak(sq), 2.0, 0.07);
        CHECK_REL(meas::peakToPeak(tr), 3.5, 0.05);
        CHECK_REL(meas::mean(sq), 1.0, 0.05);            // non simmetriche rispetto a 0 V
        CHECK_REL(meas::mean(tr), 1.75, 0.05);
        size_t high = 0; for (double v : sq) if (v > 1.0) ++high;
        CHECK_NEAR((double) high / (double) sq.size(), 0.5, 0.01);
        const double f1 = meas::goertzelDb(sq, SR, 261.6);
        CHECK(meas::goertzelDb(sq, SR, 523.2) - f1 < -40.0);
        CHECK(meas::goertzelDb(sq, SR, 1046.4) - f1 < -40.0);

        o.setShape(0.5); o.reset();
        auto narrow = run(1.0, [&] { return o.process(0.0).square; });
        high = 0; for (double v : narrow) if (v > 1.0) ++high;
        CHECK_NEAR((double) high / (double) narrow.size(), 0.05, 0.01);

        // shape 0 e 10: rampe opposte
        auto slope = [&](double sh) {
            o.setShape(sh); o.reset(); auto t = run(0.2, [&] { return o.process(0.0).triangle; });
            int up = 0, down = 0; for (size_t i = 1; i < t.size(); ++i) { if (t[i] > t[i - 1]) ++up; else if (t[i] < t[i - 1]) ++down; }
            return up - down; };
        CHECK(slope(0.0) < 0); CHECK(slope(10.0) > 0);

        // alias della quadra a 5 kHz
        o.setShape(5); o.setFrequencyDial(6.0 + std::log2(5000.0 / 261.6) / 1.5); o.reset();
        auto q5k = run(1.0, [&] { return o.process(0.0).square; });
        CHECK(worstAlias(q5k, SR, meas::frequency(q5k, SR)) < -70.0);

        // disturbo dell'Osc 3 sommato all'uscita
        o.setSquareLevel(0); o.setTriangleLevel(0); o.reset();
        CHECK_NEAR(o.process(0.0, 0.004).triangle, 0.004, 1e-9);
    }
    { // tracking Osc 1 / Osc 2 da 261 Hz a 2 kHz
        Oscillator1 a; Oscillator2 b; a.prepare(SR); b.prepare(SR);
        a.setRampLevel(10); b.setTriangleLevel(10); a.setFrequencyDial(6); b.setFrequencyDial(6);
        for (double cv : { 0.0, 0.32, 0.64, 0.95 })
        {
            a.reset(); b.reset();
            const double fa = meas::frequency(run(1.0, [&] { return a.process(cv).ramp; }), SR);
            const double fb = meas::frequency(run(1.0, [&] { return b.process(cv).triangle; }), SR);
            CHECK(std::fabs(fa - fb) < 3.0);
        }
        b.setTrackingError(5.0); b.reset(); a.reset();
        const double fa = meas::frequency(run(1.0, [&] { return a.process(0.64).ramp; }), SR);
        const double fb = meas::frequency(run(1.0, [&] { return b.process(0.64).triangle; }), SR);
        CHECK_REL(fb / fa, std::pow(2.0, 10.0 / 1200.0), 0.001);     // 5 cent/ottava per 2 ottave
    }
    { // deriva
        const double sr = 48000.0;
        Oscillator1 o; o.prepare(sr); o.setRampLevel(10); o.setFrequencyDial(6);
        o.setDriftSeed(7); o.reset();
        auto x = run(10.0, [&] { return o.process(0.0).ramp; }, sr);
        const double cents = 1200.0 * std::log2(meas::frequency(x, sr) / 261.6);
        CHECK(std::fabs(cents) < 10.0);
        std::vector<double> head(x.begin(), x.begin() + 96000), tail(x.end() - 96000, x.end());
        CHECK(std::fabs(meas::frequency(head, sr) - meas::frequency(tail, sr)) > 0.002);
        o.setDriftSeed(0); o.reset();
        auto y = run(4.0, [&] { return o.process(0.0).ramp; }, sr);
        std::vector<double> h2(y.begin(), y.begin() + 96000), t2(y.end() - 96000, y.end());
        CHECK(std::fabs(meas::frequency(h2, sr) - meas::frequency(t2, sr)) < 0.0005);
    }
    return checkSummary();
}
