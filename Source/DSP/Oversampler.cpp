#include "Oversampler.h"
#include "Volts.h"
#include <cmath>

namespace psm3
{
static double besselI0(double x)
{
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 40; ++k) { term *= (x / (2.0 * k)) * (x / (2.0 * k)); sum += term; }
    return sum;
}

void HalfbandStage::design(int taps, double transition)
{
    (void) transition;
    if (taps % 2 == 0) ++taps;
    h.assign((size_t) taps, 0.0);
    const double beta = 0.1102 * (96.0 - 8.7);        // ~96 dB di attenuazione
    const int mid = taps / 2;
    double sum = 0.0;
    for (int i = 0; i < taps; ++i)
    {
        const int n = i - mid;
        const double sinc = n == 0 ? 0.5 : std::sin(0.5 * kPi * n) / (kPi * n);     // taglio a fs/4
        const double r = (double) n / (double) mid;
        const double win = besselI0(beta * std::sqrt(std::max(0.0, 1.0 - r * r))) / besselI0(beta);
        h[(size_t) i] = sinc * win;
        sum += h[(size_t) i];
    }
    for (auto& c : h) c /= sum;
    buf.assign(h.size(), 0.0);
    pos = 0;
}

void HalfbandStage::reset() { std::fill(buf.begin(), buf.end(), 0.0); pos = 0; }

void HalfbandStage::push(double v)
{
    buf[(size_t) pos] = v;
    if (++pos >= (int) buf.size()) pos = 0;
}

double HalfbandStage::convolve() const
{
    const int n = (int) h.size();
    double acc = 0.0;
    int idx = pos;                                    // campione piu' vecchio
    for (int i = 0; i < n; ++i)
    {
        acc += h[(size_t) i] * buf[(size_t) idx];
        if (++idx >= n) idx = 0;
    }
    return acc;
}

double HalfbandStage::decimate(double a, double b) { push(a); push(b); return convolve(); }

void HalfbandStage::interpolate(double in, double& a, double& b)
{
    push(2.0 * in); a = convolve();
    push(0.0);      b = convolve();
}

// Lunghezze: lo stadio vicino alla frequenza dell'host ha la transizione stretta (20 kHz -> 28 kHz).
static constexpr int kTapsNarrow = 95;
static constexpr int kTapsWide = 27;

void Decimator::prepare(int f)
{
    factor = f == 2 ? 2 : 4;
    first.design(kTapsWide, 0.29);
    last.design(kTapsNarrow, 0.083);
    latency = kTapsNarrow / 4 + (factor == 4 ? kTapsWide / 8 : 0) + 1;
    reset();
}
void Decimator::reset() { first.reset(); last.reset(); }

double Decimator::process(const double* in)
{
    if (factor == 2) return last.decimate(in[0], in[1]);
    const double a = first.decimate(in[0], in[1]);
    const double b = first.decimate(in[2], in[3]);
    return last.decimate(a, b);
}

void Interpolator::prepare(int f)
{
    factor = f == 2 ? 2 : 4;
    first.design(kTapsNarrow, 0.083);
    last.design(kTapsWide, 0.29);
    reset();
}
void Interpolator::reset() { first.reset(); last.reset(); }

void Interpolator::process(double in, double* out)
{
    if (factor == 2) { first.interpolate(in, out[0], out[1]); return; }
    double a, b;
    first.interpolate(in, a, b);
    last.interpolate(a, out[0], out[1]);
    last.interpolate(b, out[2], out[3]);
}
}
