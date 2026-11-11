#include "check.h"
#include "measure.h"
#include "DSP/Volts.h"
#include "DSP/Params.h"
#include <cmath>
#include <vector>

int main()
{
    const double sr = 192000.0;
    std::vector<double> x((size_t) sr);
    for (size_t i = 0; i < x.size(); ++i)
        x[i] = 0.75 * std::sin(2.0 * M_PI * 261.6 * (double) i / sr);

    CHECK_REL(meas::frequency(x, sr), 261.6, 0.001);
    CHECK_NEAR(meas::peakToPeak(x), 1.5, 0.01);
    CHECK_NEAR(meas::goertzelDb(x, sr, 261.6), 20.0 * std::log10(0.75), 0.2);
    CHECK(meas::goertzelDb(x, sr, 1000.0) < -60.0);
    CHECK_NEAR(meas::rms(x), 0.75 / std::sqrt(2.0), 0.001);
    CHECK_NEAR(meas::mean(x), 0.0, 0.001);

    double v = NAN;
    CHECK(psm3::sanitize(v) && v == 0.0);
    double w = 1.5;
    CHECK(! psm3::sanitize(w) && w == 1.5);
    CHECK(psm3::clampRails(20.0) == 12.0);
    CHECK(psm3::clampRails(-20.0) == -9.0);
    CHECK(psm3::dialNorm(5.0) == 0.5);
    CHECK(psm3::dialNorm(15.0) == 1.0);

    CHECK(psm3::paramInfo(psm3::ParamId::osc1Freq).def == 6.0);
    CHECK(psm3::paramInfo(psm3::ParamId::envOff).def == 10.0);
    CHECK(psm3::paramInfo(psm3::ParamId::mains).isSwitch);
    CHECK((int) psm3::ParamId::Count == 49);
    for (int i = 0; i < (int) psm3::ParamId::Count; ++i)
        CHECK((int) psm3::paramInfo((psm3::ParamId) i).id == i);
    return checkSummary();
}
