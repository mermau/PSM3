#include "DiodeLadderFilter.h"

namespace psm3
{
// Taratura (vedi Tests/test_filter.cpp e la procedura PR6 del manuale di servizio).
// Il modello lavora in unita' di 2*eta*VT (circa 95 mV). Misurato sul modello con Response a 10:
// oscillazione di 4,563 unita' p-p a 0,9396 volte la pulsazione nominale 'a'.
static constexpr double kOutputScale = 1.5 / 4.563;                // -> 1,5 V p-p in oscillazione a livello 10
static constexpr double kInputScale = 0.7 / (0.6 * kOutputScale);  // guadagno 0,7 in banda passante per segnali piccoli
static constexpr double kOscFreqNorm = 0.9396;                     // -> 261 Hz con la manopola a 5 (PR6)
static constexpr double kStateLimit = 40.0;                        // i condensatori non superano le alimentazioni
static constexpr double kOutputClip = 3.0;                         // l'amplificatore differenziale d'uscita satura a circa 6 V p-p

// tanh razionale: abbastanza precisa per i diodi e molto piu' economica.
static inline double tanhFast(double v)
{
    if (v > 3.0) return 1.0;
    if (v < -3.0) return -1.0;
    const double v2 = v * v;
    return v * (27.0 + v2) / (27.0 + 9.0 * v2);
}
static inline double sech2Fast(double t) { return 1.0 - t * t; }

void DiodeLadderFilter::prepare(double sampleRate) { sr = sampleRate; drift.prepare(sr, driftSeed); reset(); }

void DiodeLadderFilter::reset()
{
    for (int i = 0; i < 4; ++i) x[i] = fPrev[i] = 0.0;
    thermal.seed(0xC0FFEEu);
    drift.prepare(sr, driftSeed);
}

void DiodeLadderFilter::setResponse(double d)
{
    // Soglia di oscillazione poco sopra 5 sulla manopola, come da manuale di servizio.
    const double r = std::min(10.0, std::max(0.0, d));
    K = kThresholdK * std::pow(r / 5.8, 1.2);
}

double DiodeLadderFilter::process(double in, double cv)
{
    if (! std::isfinite(in)) in = 0.0;
    if (! std::isfinite(cv)) cv = 0.0;

    const double octaves = (dial - 5.0) * 1.15 + cv / kVoltsPerOctave + drift.process() * (10.0 / 1200.0);
    double f = 261.6 * std::exp2(std::min(8.0, std::max(-8.0, octaves)));
    f = std::min(16000.0, std::max(4.0, f));
    // 'a' = I0 / (4 C eta VT); integrazione trapezoidale con pre-distorsione alla frequenza di lavoro.
    const double h = 2.0 * std::tan(kPi * f / sr) / kOscFreqNorm;
    const double hh = 0.5 * h;

    const double u = clampRails(in) * kInputScale + thermal.bipolar() * 1e-4;     // rumore termico: innesca l'oscillazione
    const double kf = K + 0.5;

    double y[4] = { x[0], x[1], x[2], x[3] };
    double F[4] = { fPrev[0], fPrev[1], fPrev[2], fPrev[3] };
    for (int iter = 0; iter < 4; ++iter)
    {
        const double tIn = tanhFast(u - kf * y[3]);
        const double t12 = tanhFast(y[1] - y[0]);
        const double t23 = tanhFast(y[2] - y[1]);
        const double t34 = tanhFast(y[3] - y[2]);
        const double t4 = tanhFast(y[3] * (1.0 / 3.0));
        F[0] = tIn + t12; F[1] = t23 - t12; F[2] = t34 - t23; F[3] = -t4 - t34;

        const double dIn = sech2Fast(tIn), d12 = sech2Fast(t12), d23 = sech2Fast(t23), d34 = sech2Fast(t34), d4 = sech2Fast(t4) / 3.0;
        // Jacobiano J = I - hh * dF/dy  (tridiagonale + un termine d'angolo per la retroazione)
        double J[4][5] = {
            { 1.0 + hh * d12,           -hh * d12,                 0.0,                       hh * dIn * kf,              0.0 },
            { -hh * d12,                1.0 + hh * (d12 + d23),    -hh * d23,                 0.0,                        0.0 },
            { 0.0,                      -hh * d23,                 1.0 + hh * (d23 + d34),    -hh * d34,                  0.0 },
            { 0.0,                      0.0,                       -hh * d34,                 1.0 + hh * (d34 + d4),      0.0 } };
        double maxRes = 0.0;
        for (int i = 0; i < 4; ++i)
        {
            const double res = y[i] - x[i] - hh * (fPrev[i] + F[i]);
            J[i][4] = -res;
            maxRes = std::max(maxRes, std::fabs(res));
        }
        if (maxRes < 1e-9) break;
        for (int k = 0; k < 4; ++k)                 // eliminazione di Gauss (matrice a dominanza diagonale)
        {
            const double inv = 1.0 / J[k][k];
            for (int i = k + 1; i < 4; ++i)
            {
                const double m = J[i][k] * inv;
                if (m == 0.0) continue;
                for (int j = k; j < 5; ++j) J[i][j] -= m * J[k][j];
            }
        }
        double dy[4];
        for (int i = 3; i >= 0; --i)
        {
            double s = J[i][4];
            for (int j = i + 1; j < 4; ++j) s -= J[i][j] * dy[j];
            dy[i] = s / J[i][i];
        }
        for (int i = 0; i < 4; ++i) y[i] += std::min(2.0, std::max(-2.0, dy[i]));     // passo limitato
    }

    // derivata al punto accettato, per il passo successivo
    {
        const double tIn = tanhFast(u - kf * y[3]), t12 = tanhFast(y[1] - y[0]), t23 = tanhFast(y[2] - y[1]);
        const double t34 = tanhFast(y[3] - y[2]), t4 = tanhFast(y[3] * (1.0 / 3.0));
        fPrev[0] = tIn + t12; fPrev[1] = t23 - t12; fPrev[2] = t34 - t23; fPrev[3] = -t4 - t34;
    }
    bool bad = false;
    for (int i = 0; i < 4; ++i)
    {
        bad = bad || ! std::isfinite(y[i]);
        x[i] = std::min(kStateLimit, std::max(-kStateLimit, y[i]));
    }
    if (bad) { for (int i = 0; i < 4; ++i) x[i] = fPrev[i] = 0.0; return 0.0; }

    const double v = kOutputScale * kf * x[3];
    const double q = v / kOutputClip;
    return level * v / std::sqrt(std::sqrt(1.0 + q * q * q * q));
}
}
