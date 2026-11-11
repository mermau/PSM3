#include "check.h"
#include "measure.h"
#include "DSP/NoiseGenerator.h"
#include <algorithm>
#include <vector>
using namespace psm3;

static double bandDb(const std::vector<double>& x, double sr, double f1, double f2)
{
    double p = 0.0; int n = 0;
    for (double f = f1; f <= f2; f *= 1.05, ++n) { const double a = std::pow(10.0, meas::goertzelDb(x, sr, f) / 20.0); p += a * a; }
    return 10.0 * std::log10(p / n + 1e-30);
}
static std::vector<double> run(NoiseGenerator& n, double seconds, double sr)
{
    std::vector<double> x((size_t) (seconds * sr)); for (auto& v : x) v = n.process(); return x;
}

int main()
{
    const double SR = 192000.0;
    NoiseGenerator n; n.prepare(SR); n.setSeed(1); n.setLevel(10); n.setColour(5); n.reset();
    auto white = run(n, 1.0, SR);
    auto sorted = white; std::sort(sorted.begin(), sorted.end());
    const double lo = sorted[(size_t) (sorted.size() * 0.0015)], hi = sorted[(size_t) (sorted.size() * 0.9985)];
    CHECK_REL(hi - lo, 3.0, 0.10);
    CHECK_NEAR(meas::mean(white), 0.0, 0.02);
    const double flat = bandDb(white, SR, 100, 400) - bandDb(white, SR, 4000, 8000);
    CHECK(std::fabs(flat) < 4.0);

    n.setColour(0); n.reset(); auto dark = run(n, 1.0, SR);
    CHECK(bandDb(dark, SR, 100, 400) - bandDb(dark, SR, 4000, 8000) > flat + 20.0);
    n.setColour(10); n.reset(); auto light = run(n, 1.0, SR);
    CHECK(bandDb(light, SR, 100, 400) - bandDb(light, SR, 4000, 8000) < flat - 15.0);

    n.setLevel(0); n.setColour(5); n.reset();
    CHECK(meas::peakToPeak(run(n, 0.1, SR)) < 1e-6);

    NoiseGenerator a, b; a.prepare(SR); b.prepare(SR); a.setLevel(10); b.setLevel(10);
    a.setSeed(42); b.setSeed(42); a.reset(); b.reset();
    bool same = true; for (int i = 0; i < 1000; ++i) same = same && a.process() == b.process();
    CHECK(same);
    b.setSeed(43); b.reset(); a.reset();
    int diff = 0; for (int i = 0; i < 1000; ++i) if (a.process() != b.process()) ++diff;
    CHECK(diff > 900);

    { // riscaldamento come l'originale: 20-30 secondi
        const double sr = 48000.0;
        NoiseGenerator w; w.prepare(sr); w.setLevel(10); w.setColour(5); w.setWarmUp(true); w.reset();
        auto cold = run(w, 19.0, sr);
        CHECK(meas::peakToPeak(cold) == 0.0);
        run(w, 11.0, sr);
        CHECK(meas::rms(run(w, 1.0, sr)) > 0.3);
        w.setWarmUp(false); w.reset();
        CHECK(meas::rms(run(w, 0.5, sr)) > 0.3);
    }
    return checkSummary();
}
