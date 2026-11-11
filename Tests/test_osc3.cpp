#include "check.h"
#include "measure.h"
#include "DSP/Oscillator3.h"
#include <vector>
using namespace psm3;

template <typename Fn> static std::vector<double> run(double seconds, double sr, Fn fn)
{
    std::vector<double> x((size_t) (seconds * sr));
    for (auto& v : x) v = fn();
    return x;
}

int main()
{
    const double SR = 192000.0;
    Oscillator3 o; o.prepare(SR); o.setSquareLevel(10); o.setTriangleLevel(10); o.setShape(5);

    o.setFrequencyDial(10); o.reset();
    CHECK_REL(meas::frequency(run(1.0, SR, [&] { return o.process(0.0).triangle; }), SR), 500.0, 0.01);
    o.setFrequencyDial(8); o.reset();
    auto tri = run(2.0, SR, [&] { return o.process(0.0).triangle; });
    CHECK_REL(meas::frequency(tri, SR), 63.0, 0.02);
    o.reset();
    auto sq = run(2.0, SR, [&] { return o.process(0.0).square; });
    CHECK_REL(meas::peakToPeak(tri), 6.0, 0.05);
    CHECK_REL(meas::peakToPeak(sq), 5.0, 0.07);
    CHECK(meas::mean(tri) > 2.5 && meas::mean(sq) > 2.0);

    o.reset();   // 0,26 V per ottava
    CHECK_REL(meas::frequency(run(2.0, SR, [&] { return o.process(0.26).triangle; }), SR), 126.0, 0.02);

    { // periodo a quadrante 0: oltre 20 secondi
        const double sr = 48000.0;
        Oscillator3 slow; slow.prepare(sr); slow.setTriangleLevel(10); slow.setShape(5); slow.setFrequencyDial(0);
        auto x = run(150.0, sr, [&] { return slow.process(0.0).triangle; });
        const double f = meas::frequency(x, sr);
        CHECK(f > 0.0 && 1.0 / f > 20.0 && 1.0 / f < 90.0);
    }

    auto slope = [&](double sh) {
        o.setShape(sh); o.reset(); auto t = run(0.5, SR, [&] { return o.process(0.0).triangle; });
        int up = 0, down = 0; for (size_t i = 1; i < t.size(); ++i) { if (t[i] > t[i - 1]) ++up; else if (t[i] < t[i - 1]) ++down; }
        return up - down; };
    CHECK(slope(0.0) < 0); CHECK(slope(10.0) > 0);

    // disturbo verso gli altri oscillatori: piccolo, e piu' forte con Shape lontano dal centro
    o.setShape(5); o.reset(); double b5 = 0.0; for (int i = 0; i < 20000; ++i) { o.process(0.0); b5 = std::max(b5, std::fabs(o.bleed())); }
    o.setShape(10); o.reset(); double b10 = 0.0; for (int i = 0; i < 20000; ++i) { o.process(0.0); b10 = std::max(b10, std::fabs(o.bleed())); }
    CHECK(b5 > 0.0 && b5 < 0.02 && b10 > b5);
    return checkSummary();
}
