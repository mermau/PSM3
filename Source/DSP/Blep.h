// PSM-3 — correzione a banda limitata di gradini (BLEP) e spigoli (BLAMP), a 4 punti.
// Introduce un ritardo fisso di 2 campioni interni (circa 10 microsecondi).
#pragma once

namespace psm3
{
class BlepBuffer
{
public:
    void reset() { for (auto& c : corr) c = 0.0; for (auto& n : naive) n = 0.0; idx = 0; }

    // Gradino di altezza 'height' avvenuto 'tau' campioni fa (0 <= tau < 1).
    void addStep(double tau, double height)
    {
        const double d = tau, d2 = d * d, d3 = d2 * d, d4 = d3 * d;
        add(0, height * (d4 / 24.0));
        add(1, height * (-d4 / 8.0 + d3 / 6.0 + d2 / 4.0 + d / 6.0 + 1.0 / 24.0));
        add(2, height * (d4 / 8.0 - d3 / 3.0 + 2.0 * d / 3.0 - 0.5));
        add(3, height * (-d4 / 24.0 + d3 / 6.0 - d2 / 4.0 + d / 6.0 - 1.0 / 24.0));
    }
    // Cambio di pendenza 'slope' (volt per campione) avvenuto 'tau' campioni fa.
    void addCorner(double tau, double slope)
    {
        const double d = tau, d2 = d * d, d3 = d2 * d, d4 = d3 * d, d5 = d4 * d;
        add(0, slope * (d5 / 120.0));
        add(1, slope * (-d5 / 40.0 + d4 / 24.0 + d3 / 12.0 + d2 / 12.0 + d / 24.0 + 1.0 / 120.0));
        add(2, slope * (d5 / 40.0 - d4 / 12.0 + d2 / 3.0 - d / 2.0 + 7.0 / 30.0));
        add(3, slope * (-d5 / 120.0 + d4 / 24.0 - d3 / 12.0 + d2 / 12.0 - d / 24.0 + 1.0 / 120.0));
    }
    // Inserisce il campione ingenuo corrente e restituisce quello di 2 campioni fa, corretto.
    double push(double naiveNow)
    {
        naive[(idx + 2) & 3] = naiveNow;
        const double out = naive[idx] + corr[idx];
        corr[idx] = 0.0;
        idx = (idx + 1) & 3;
        return out;
    }
private:
    // slot 0 = campione n-2 (quello che uscira' adesso), 1 = n-1, 2 = n (corrente), 3 = n+1
    void add(int slot, double v) { corr[(idx + slot) & 3] += v; }
    double corr[4] = { 0, 0, 0, 0 }, naive[4] = { 0, 0, 0, 0 };
    int idx = 0;
};
}
