#include "check.h"
#include "measure.h"
#include "DSP/EnvelopeShaper.h"
#include <vector>
using namespace psm3;

// inviluppo interno ricavato dal trapezoide a livello 10: trap = 4 - 7e
static double envOf(const EnvelopeShaper::Out& o) { return (4.0 - o.trapezoid) / 7.0; }

struct Rig
{
    EnvelopeShaper e; double sr;
    explicit Rig(double sampleRate, double a, double on, double d, double off) : sr(sampleRate)
    {
        e.prepare(sr); e.setAttack(a); e.setOn(on); e.setDecay(d); e.setOff(off);
        e.setTrapezoidLevel(10); e.setSignalLevel(10); e.reset();
    }
    double step(double in = 0.0, double cv = 0.0) { return envOf(e.process(in, cv)); }
    void press() { e.setGate(true); }
    void release() { e.setGate(false); }
    // secondi finche' la condizione diventa vera (o -1)
    template <typename Cond> double timeUntil(Cond c, double maxSeconds, double cv = 0.0)
    {
        const long n = (long) (maxSeconds * sr);
        for (long i = 0; i < n; ++i) if (c(step(0.0, cv))) return (double) i / sr;
        return -1.0;
    }
};

int main()
{
    { // tutto a zero: riciclo spontaneo alla frequenza massima (circa 60 Hz)
        Rig r(192000.0, 0, 0, 0, 0);
        std::vector<double> x(192000); for (auto& v : x) v = r.step();
        const double f = meas::frequency(x, r.sr);
        CHECK(f > 40.0 && f < 70.0);
    }
    { // attacco massimo: 1 secondo
        Rig r(48000.0, 10, 0, 5, 10);
        r.press();
        CHECK_REL(r.timeUntil([](double e) { return e >= 0.99; }, 3.0), 1.0, 0.15);
    }
    { // attacco minimo: 2 ms
        Rig r(192000.0, 0, 10, 5, 10);
        r.press();
        CHECK_REL(r.timeUntil([](double e) { return e >= 0.99; }, 1.0), 0.002, 0.3);
    }
    { // decadimento massimo 15 s, minimo 3 ms
        Rig r(48000.0, 0, 0, 10, 10);
        r.press(); r.timeUntil([](double e) { return e >= 0.999; }, 1.0); r.release();
        CHECK_REL(r.timeUntil([](double e) { return e <= 0.001; }, 30.0), 15.0, 0.20);
        Rig q(192000.0, 0, 0, 0, 10);
        q.press(); q.timeUntil([](double e) { return e >= 0.999; }, 1.0); q.release();
        CHECK_REL(q.timeUntil([](double e) { return e <= 0.001; }, 1.0), 0.003, 0.3);
    }
    { // decadimento controllato in tensione: 0,4 V per ottava
        Rig r(48000.0, 0, 0, 5, 10);
        r.press(); r.timeUntil([](double e) { return e >= 0.999; }, 1.0); r.release();
        const double t0 = r.timeUntil([](double e) { return e <= 0.001; }, 30.0);
        r.press(); r.timeUntil([](double e) { return e >= 0.999; }, 1.0); r.release();
        const double t1 = r.timeUntil([](double e) { return e <= 0.001; }, 30.0, 0.4);
        CHECK_REL(t1 / t0, 2.0, 0.10);
    }
    { // Off a 10: nessun riciclo; Off a 5: ricicla
        Rig r(48000.0, 0, 0, 2, 10);
        CHECK(r.timeUntil([](double e) { return e > 0.5; }, 5.0) < 0.0);           // all'accensione resta a riposo
        r.press(); r.step(); r.release();
        r.timeUntil([](double e) { return e <= 0.001; }, 5.0);
        CHECK(r.timeUntil([](double e) { return e > 0.01; }, 20.0) < 0.0);
        CHECK(r.e.phase() == EnvelopeShaper::Phase::Off);
        Rig q(48000.0, 0, 0, 2, 5);
        q.press(); q.step(); q.release();
        q.timeUntil([](double e) { return e <= 0.001; }, 5.0);
        CHECK(q.timeUntil([](double e) { return e > 0.5; }, 5.0) > 0.0);
        Rig s(48000.0, 0, 0, 2, 7.5);                                              // gia' sopra 7 non ricicla
        s.press(); s.step(); s.release();
        s.timeUntil([](double e) { return e <= 0.001; }, 5.0);
        CHECK(s.timeUntil([](double e) { return e > 0.01; }, 20.0) < 0.0);
    }
    { // pulsante tenuto: resta in On, spia accesa; al rilascio decade
        Rig r(48000.0, 0, 0, 3, 10);
        r.press();
        bool held = true;
        for (int i = 0; i < 3 * 48000; ++i) { const double e = r.step(); if (i > 4800) held = held && e > 0.99 && r.e.lampOn(); }
        CHECK(held);
        CHECK(r.e.phase() == EnvelopeShaper::Phase::On);
        r.release();
        const double t = r.timeUntil([](double e) { return e <= 0.001; }, 5.0);
        CHECK(t > 0.0);
        CHECK(! r.e.lampOn());
    }
    { // tempo di On: massimo 2,5 s nel riciclo automatico, piu' lungo (circa doppio) dopo il pulsante
        Rig r(48000.0, 0, 10, 0, 0);
        r.timeUntil([](double e) { return e >= 0.999; }, 1.0);
        CHECK_REL(r.timeUntil([](double e) { return e < 0.9; }, 10.0), 2.5, 0.15);
        Rig q(48000.0, 0, 10, 0, 10);
        q.press(); q.step(); q.release();
        q.timeUntil([](double e) { return e >= 0.999; }, 1.0);
        const double t = q.timeUntil([](double e) { return e < 0.9; }, 10.0);
        CHECK(t >= 4.0 && t <= 6.0);
    }
    { // uscita Trapezoid: -3 V in On, +4 V in Off; manopola a 0 = niente
        Rig r(48000.0, 0, 0, 2, 10);
        CHECK_NEAR(r.e.process(0.0, 0.0).trapezoid, 4.0, 0.1);
        r.press(); for (int i = 0; i < 4800; ++i) r.step();
        CHECK_NEAR(r.e.process(0.0, 0.0).trapezoid, -3.0, 0.1);
        r.e.setTrapezoidLevel(0);
        CHECK(r.e.process(0.0, 0.0).trapezoid == 0.0);
    }
    { // VCA: aperto lascia passare almeno a livello d'ingresso, chiuso sotto i 5 mV p-p
        Rig r(192000.0, 0, 0, 2, 10);
        std::vector<double> closed, open;
        for (int i = 0; i < 19200; ++i) closed.push_back(r.e.process(0.5 * std::sin(2.0 * M_PI * 261.0 * i / r.sr), 0.0).signal);
        CHECK(meas::peakToPeak(closed) < 0.005);
        r.press();
        for (int i = 0; i < 38400; ++i) { const double v = r.e.process(0.5 * std::sin(2.0 * M_PI * 261.0 * i / r.sr), 0.0).signal; if (i > 19200) open.push_back(v); }
        CHECK(meas::peakToPeak(open) >= 1.0);
        r.e.setSignalLevel(0);
        CHECK(std::fabs(r.e.process(0.5, 0.0).signal) < 1e-9);
    }
    { // un nuovo fronte durante il decadimento riparte dal livello corrente
        Rig r(48000.0, 5, 0, 8, 10);
        r.press(); r.timeUntil([](double e) { return e >= 0.999; }, 2.0); r.release();
        r.timeUntil([](double e) { return e <= 0.5; }, 20.0);
        r.press();
        const double e1 = r.step();
        CHECK(e1 > 0.45 && e1 < 0.55 && r.e.phase() == EnvelopeShaper::Phase::Attack);
        r.release(); r.e.trigger();
        CHECK(r.e.phase() == EnvelopeShaper::Phase::Attack);
    }
    return checkSummary();
}
