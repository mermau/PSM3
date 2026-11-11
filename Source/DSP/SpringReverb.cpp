#include "SpringReverb.h"

namespace psm3
{
static constexpr double kAllpassCoeff = 0.6;
static constexpr double kWetGain = 0.9;

void SpringReverb::Spring::prepare(double fs, double seconds, double rt60)
{
    // Il ritardo totale alle frequenze basse e' quello della molla; la catena passa-tutto ritarda di piu' gli acuti.
    const int lowDelay = (int) (kSections * kStretch * (1.0 - kAllpassCoeff) / (1.0 + kAllpassCoeff));
    const int n = std::max(8, (int) (seconds * fs) - lowDelay);
    delay.assign((size_t) n, 0.0);
    gain = std::pow(10.0, -3.0 * seconds / rt60);
    reset();
}

void SpringReverb::Spring::reset()
{
    std::fill(delay.begin(), delay.end(), 0.0);
    for (auto& s : ax) for (auto& v : s) v = 0.0;
    for (auto& s : ay) for (auto& v : s) v = 0.0;
    pos = apos = 0; lp = dc = feedback = 0.0;
}

double SpringReverb::Spring::process(double in, double lpCoeff, double dcCoeff)
{
    // linea di ritardo
    double v = delay[(size_t) pos];
    delay[(size_t) pos] = in - gain * feedback;         // riflessione con inversione
    if (++pos >= (int) delay.size()) pos = 0;
    // dispersione: passa-tutto "stirati" (il ritardo cresce con la frequenza, come in una molla elicoidale)
    for (int s = 0; s < kSections; ++s)
    {
        const double y = kAllpassCoeff * v + ax[s][apos] - kAllpassCoeff * ay[s][apos];
        ax[s][apos] = v; ay[s][apos] = y; v = y;
    }
    if (++apos >= kStretch) apos = 0;
    // perdite: gli acuti si smorzano prima; niente continua
    lp += lpCoeff * (v - lp);
    dc += dcCoeff * (lp - dc);
    feedback = lp - dc;
    if (! std::isfinite(feedback)) reset();
    return feedback;
}

void SpringReverb::prepare(double sampleRate)
{
    sr = sampleRate;
    ratio = std::max(1, (int) std::lround(sr / 48000.0));
    fs = sr / ratio;
    bandCoeff = 1.0 - std::exp(-kTwoPi * 5000.0 / sr);
    lpCoeff = 1.0 - std::exp(-kTwoPi * 4500.0 / fs);
    dcCoeff = 1.0 - std::exp(-kTwoPi * 60.0 / fs);
    springs[0].prepare(fs, 0.025, 2.3);
    springs[1].prepare(fs, 0.030, 2.3);
    reset();
}

void SpringReverb::reset()
{
    for (auto& s : springs) s.reset();
    pre1 = pre2 = post1 = post2 = wetHeld = 0.0; phase = 0;
}

double SpringReverb::step(double drive)
{
    return 0.5 * (springs[0].process(drive, lpCoeff, dcCoeff) + springs[1].process(drive, lpCoeff, dcCoeff));
}

double SpringReverb::process(double in, double mixCv)
{
    if (! std::isfinite(in)) in = 0.0;
    if (! std::isfinite(mixCv)) mixCv = 0.0;
    const double m = std::min(1.0, std::max(0.0, mix + mixCv * 0.25));

    // Stadio pilota: satura in modo simmetrico; i trasduttori limitano la banda.
    const double drive = 1.5 * std::tanh(in / 1.5);
    pre1 += bandCoeff * (drive - pre1);
    pre2 += bandCoeff * (pre1 - pre2);
    if (++phase >= ratio) { phase = 0; wetHeld = step(pre2) * kWetGain; }
    post1 += bandCoeff * (wetHeld - post1);
    post2 += bandCoeff * (post1 - post2);

    const double v = (1.0 - m) * in + m * post2;
    const double q = v / 2.5;                               // non oltre 5 V p-p
    return level * v / std::sqrt(std::sqrt(1.0 + q * q * q * q));
}
}
