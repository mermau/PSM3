// Strumenti di misura per i test del motore.
#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace meas
{
inline double mean(const std::vector<double>& x)
{
    double s = 0.0; for (double v : x) s += v; return x.empty() ? 0.0 : s / (double) x.size();
}
inline double rms(const std::vector<double>& x)
{
    double s = 0.0; for (double v : x) s += v * v; return x.empty() ? 0.0 : std::sqrt(s / (double) x.size());
}
inline double peakToPeak(const std::vector<double>& x)
{
    if (x.empty()) return 0.0;
    auto mm = std::minmax_element(x.begin(), x.end());
    return *mm.second - *mm.first;
}
// Frequenza fondamentale dagli attraversamenti in salita del livello medio, con isteresi.
inline double frequency(const std::vector<double>& x, double sr)
{
    if (x.size() < 4) return 0.0;
    auto mm = std::minmax_element(x.begin(), x.end());
    const double mid = 0.5 * (*mm.first + *mm.second);
    const double hyst = 0.1 * (*mm.second - *mm.first);
    if (hyst <= 0.0) return 0.0;
    bool armed = false; double first = -1.0, last = -1.0; int count = 0;
    for (size_t i = 1; i < x.size(); ++i)
    {
        if (x[i] < mid - hyst) armed = true;
        if (armed && x[i - 1] < mid && x[i] >= mid)
        {
            const double t = (double) (i - 1) + (mid - x[i - 1]) / (x[i] - x[i - 1]);
            if (first < 0.0) first = t;
            last = t; ++count; armed = false;
        }
    }
    if (count < 2) return 0.0;
    return (double) (count - 1) * sr / (last - first);
}
// Ampiezza della componente a freq, in dB rispetto a una sinusoide di ampiezza 1 (finestra di Hann).
inline double goertzelDb(const std::vector<double>& x, double sr, double freq)
{
    const size_t n = x.size();
    if (n == 0) return -300.0;
    double re = 0.0, im = 0.0, wsum = 0.0;
    const double w = 2.0 * M_PI * freq / sr;
    for (size_t i = 0; i < n; ++i)
    {
        const double win = 0.5 - 0.5 * std::cos(2.0 * M_PI * (double) i / (double) (n - 1));
        re += x[i] * win * std::cos(w * (double) i);
        im -= x[i] * win * std::sin(w * (double) i);
        wsum += win;
    }
    const double amp = 2.0 * std::sqrt(re * re + im * im) / wsum;
    return 20.0 * std::log10(std::max(amp, 1e-15));
}
inline double db(double ratio) { return 20.0 * std::log10(std::max(ratio, 1e-15)); }
// Distorsione armonica totale (armoniche 2..10 rispetto alla fondamentale), come rapporto.
inline double thd(const std::vector<double>& x, double sr, double f0)
{
    const double fund = std::pow(10.0, goertzelDb(x, sr, f0) / 20.0);
    double s = 0.0;
    for (int h = 2; h <= 10 && f0 * h < 0.45 * sr; ++h)
    {
        const double a = std::pow(10.0, goertzelDb(x, sr, f0 * h) / 20.0);
        s += a * a;
    }
    return std::sqrt(s) / std::max(fund, 1e-15);
}
// Tempo di riverberazione (-60 dB) per integrazione all'indietro di Schroeder, estrapolato da -5...-35 dB.
inline double rt60(const std::vector<double>& x, double sr)
{
    const size_t n = x.size();
    if (n < 2) return 0.0;
    std::vector<double> e(n);
    double acc = 0.0;
    for (size_t i = n; i-- > 0;) { acc += x[i] * x[i]; e[i] = acc; }
    if (e[0] <= 0.0) return 0.0;
    double t5 = -1.0, t35 = -1.0;
    for (size_t i = 0; i < n; ++i)
    {
        const double d = 10.0 * std::log10(std::max(e[i] / e[0], 1e-30));
        if (t5 < 0.0 && d <= -5.0) t5 = (double) i / sr;
        if (t35 < 0.0 && d <= -35.0) { t35 = (double) i / sr; break; }
    }
    if (t5 < 0.0 || t35 < 0.0) return 0.0;
    return (t35 - t5) * 2.0;
}
}
