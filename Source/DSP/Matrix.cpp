#include "Matrix.h"
#include <cmath>

namespace psm3
{
// Resistenze d'uscita viste dalla matrice (manuale: da 300 ohm a 3 kohm secondo il dispositivo).
const std::array<double, 16> kRowOutOhms = {
    1000.0, 1000.0,     // Osc 1 sinusoide, rampa
    1000.0, 1000.0,     // Osc 2 quadra, triangolo
    1000.0, 1000.0,     // Osc 3 quadra, triangolo
    1000.0,             // Noise
    600.0, 600.0,       // Input 1, 2
    1000.0,             // Filter
    1000.0,             // Trapezoid
    1000.0,             // Envelope signal
    1000.0,             // Ring modulator
    1000.0,             // Reverberation
    2000.0, 2000.0      // Stick
};

static double hash01(uint32_t seed, int row, int col)
{
    uint32_t h = seed * 2654435761u ^ (uint32_t) (row * 16 + col + 1) * 2246822519u;
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
    return (double) h / 4294967296.0;       // [0,1)
}

double Matrix::pinOhms(Pin p, int row, int col, uint32_t seed)
{
    double nominal = 2700.0, tol = 0.05;
    switch (p)
    {
        case Pin::White: break;
        case Pin::Red:   tol = 0.0; break;      // pin "di precisione": valore esatto (vedi nota in Matrix.h)
        case Pin::Green: nominal = 68000.0; break;
        case Pin::None:  return 0.0;
    }
    if (seed == 0) return nominal;
    return nominal * (1.0 + tol * (hash01(seed, row, col) * 2.0 - 1.0));
}

void Matrix::build(const PinGrid& pins, uint32_t seed, const std::array<double, 16>& rowOutOhms, MatrixSolution& out)
{
    out = MatrixSolution();
    double g[16][16];                       // conduttanze dei pin
    for (int r = 0; r < 16; ++r)
        for (int c = 0; c < 16; ++c)
        {
            uint8_t raw = pins[(size_t) (r * 16 + c)];
            const Pin p = raw <= 3 ? (Pin) raw : Pin::None;
            g[r][c] = p == Pin::None ? 0.0 : 1.0 / pinOhms(p, r, c, seed);
            if (p != Pin::None) { out.rowUsed[(size_t) r] = true; out.colUsed[(size_t) c] = true; }
        }

    // Incognite: tensione dei nodi di riga usati, poi delle colonne usate.
    int rowIdx[16], colIdx[16], n = 0;
    for (int r = 0; r < 16; ++r) rowIdx[r] = out.rowUsed[(size_t) r] ? n++ : -1;
    const int nRows = n;
    for (int c = 0; c < 16; ++c) colIdx[c] = out.colUsed[(size_t) c] ? n++ : -1;
    if (n == 0) return;

    static constexpr int kMax = 32;
    double a[kMax][kMax + 16];              // [A | B], B = termini noti per ciascuna sorgente di riga
    for (int i = 0; i < n; ++i) for (int j = 0; j < n + nRows; ++j) a[i][j] = 0.0;

    for (int r = 0; r < 16; ++r)
    {
        const int i = rowIdx[r]; if (i < 0) continue;
        const double gOut = 1.0 / (rowOutOhms[(size_t) r] > 1e-3 ? rowOutOhms[(size_t) r] : 1e-3);
        a[i][i] += gOut;
        a[i][n + i] = gOut;                 // sorgente r
        for (int c = 0; c < 16; ++c)
        {
            if (g[r][c] == 0.0) continue;
            const int j = colIdx[c];
            a[i][i] += g[r][c]; a[i][j] -= g[r][c];
            a[j][j] += g[r][c]; a[j][i] -= g[r][c];
        }
    }
    for (int c = 0; c < 16; ++c) if (colIdx[c] >= 0) a[colIdx[c]][colIdx[c]] += 1.0 / kRin;

    // Eliminazione di Gauss-Jordan con pivot parziale.
    const int width = n + nRows;
    for (int k = 0; k < n; ++k)
    {
        int piv = k;
        for (int i = k + 1; i < n; ++i) if (std::fabs(a[i][k]) > std::fabs(a[piv][k])) piv = i;
        if (piv != k) for (int j = 0; j < width; ++j) { const double t = a[k][j]; a[k][j] = a[piv][j]; a[piv][j] = t; }
        const double d = a[k][k];
        if (std::fabs(d) < 1e-30) continue;
        for (int j = 0; j < width; ++j) a[k][j] /= d;
        for (int i = 0; i < n; ++i)
        {
            if (i == k) continue;
            const double f = a[i][k];
            if (f == 0.0) continue;
            for (int j = 0; j < width; ++j) a[i][j] -= f * a[k][j];
        }
    }

    for (int r = 0; r < 16; ++r)
    {
        const int i = rowIdx[r]; if (i < 0) continue;
        for (int c = 0; c < 16; ++c)
        {
            const int j = colIdx[c]; if (j < 0) continue;
            const double v = a[j][n + i];
            out.t[(size_t) (r * 16 + c)] = std::isfinite(v) ? (float) v : 0.0f;
        }
    }
}

void Matrix::apply(const MatrixSolution& s, const std::array<double, 16>& rowVolts, std::array<double, 16>& colVolts)
{
    colVolts.fill(0.0);
    for (int r = 0; r < 16; ++r)
    {
        if (! s.rowUsed[(size_t) r]) continue;
        const double v = rowVolts[(size_t) r];
        const float* t = &s.t[(size_t) (r * 16)];
        for (int c = 0; c < 16; ++c) colVolts[(size_t) c] += (double) t[c] * v;
    }
}
}
