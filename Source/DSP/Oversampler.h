// PSM-3 — sovracampionamento: il motore gira a 2x o 4x la frequenza dell'host.
#pragma once
#include <vector>

namespace psm3
{
// Stadio FIR passa-basso a meta' banda (finestra di Kaiser), usato sia per dimezzare sia per raddoppiare.
class HalfbandStage
{
public:
    void design(int taps, double transition);       // transition = larghezza di transizione / frequenza alta dello stadio
    void reset();
    double decimate(double a, double b);             // due campioni in ingresso, uno in uscita
    void interpolate(double in, double& a, double& b);
    int taps() const { return (int) h.size(); }
private:
    void push(double v);
    double convolve() const;
    std::vector<double> h, buf;
    int pos = 0;
};

class Decimator
{
public:
    void prepare(int factor);                        // 2 o 4
    void reset();
    double process(const double* in);                // 'factor' campioni interni -> 1 campione host
    int latencySamples() const { return latency; }   // in campioni host
private:
    int factor = 4, latency = 0;
    HalfbandStage first, last;                       // first: 4x->2x, last: 2x->1x
};

class Interpolator
{
public:
    void prepare(int factor);
    void reset();
    void process(double in, double* out);            // 1 campione host -> 'factor' campioni interni
private:
    int factor = 4;
    HalfbandStage first, last;                       // first: 1x->2x, last: 2x->4x
};
}
