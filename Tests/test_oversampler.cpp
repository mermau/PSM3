#include "check.h"
#include "measure.h"
#include "DSP/Oversampler.h"
#include <vector>
using namespace psm3;

static std::vector<double> sine(double f, double sr, double seconds, double amp = 1.0)
{
    std::vector<double> x((size_t) (sr * seconds));
    for (size_t i = 0; i < x.size(); ++i) x[i] = amp * std::sin(2.0 * M_PI * f * (double) i / sr);
    return x;
}
static std::vector<double> decimate(const std::vector<double>& in, int factor)
{
    Decimator d; d.prepare(factor);
    std::vector<double> out;
    for (size_t i = 0; i + (size_t) factor <= in.size(); i += (size_t) factor) out.push_back(d.process(&in[i]));
    out.erase(out.begin(), out.begin() + 400);   // transitorio del filtro
    return out;
}

int main()
{
    for (int factor : { 4, 2 })
    {
        const double host = 48000.0, inner = host * factor;
        auto lo = decimate(sine(1000.0, inner, 0.5), factor);
        CHECK_NEAR(meas::goertzelDb(lo, host, 1000.0), 0.0, 0.1);
        auto top = decimate(sine(18000.0, inner, 0.5), factor);
        CHECK_NEAR(meas::goertzelDb(top, host, 18000.0), 0.0, 0.5);
        // 40 kHz ricade a 8 kHz dopo la decimazione: deve essere soppresso
        auto hi = decimate(sine(40000.0, inner, 0.5), factor);
        CHECK(meas::db(meas::rms(hi)) < -85.0);

        Interpolator up; up.prepare(factor);
        Decimator down; down.prepare(factor);
        auto src = sine(1000.0, host, 0.5);
        std::vector<double> rt; std::vector<double> tmp((size_t) factor);
        for (double s : src) { up.process(s, tmp.data()); rt.push_back(down.process(tmp.data())); }
        rt.erase(rt.begin(), rt.begin() + 400);
        CHECK_NEAR(meas::goertzelDb(rt, host, 1000.0), 0.0, 0.2);
        CHECK(down.latencySamples() >= 0 && down.latencySamples() < 64);
    }
    return checkSummary();
}
