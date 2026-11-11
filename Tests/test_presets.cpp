// Ogni preset di fabbrica deve essere valido, suonare e non saturare.
// Con PSM3_RENDER_DIR=<cartella> salva anche un WAV per preset, per l'ascolto.
#include "check.h"
#include "measure.h"
#include "DSP/PSM3Engine.h"
#include "Plugin/FactoryPresets.h"
#include <cstdio>
#include <string>
#include <vector>
using namespace psm3;

static void writeWav(const std::string& path, const std::vector<float>& l, const std::vector<float>& r, int sr)
{
    FILE* f = std::fopen(path.c_str(), "wb"); if (f == nullptr) return;
    const uint32_t n = (uint32_t) l.size(), bytes = n * 4;
    auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); }; auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); u32(36 + bytes); std::fwrite("WAVEfmt ", 1, 8, f); u32(16); u16(1); u16(2);
    u32((uint32_t) sr); u32((uint32_t) sr * 4); u16(4); u16(16); std::fwrite("data", 1, 4, f); u32(bytes);
    for (uint32_t i = 0; i < n; ++i)
    {
        auto s16 = [](float v) { return (int16_t) std::lround(std::max(-1.0f, std::min(1.0f, v)) * 32767.0f); };
        const int16_t a = s16(l[i]), b = s16(r[i]); std::fwrite(&a, 2, 1, f); std::fwrite(&b, 2, 1, f);
    }
    std::fclose(f);
}

int main()
{
    const char* dir = std::getenv("PSM3_RENDER_DIR");
    int count = 0; const FactoryPreset* presets = factoryPresets(count);
    CHECK(count >= 15);
    CHECK(std::string(presets[0].name) == "Tastiera base");

    for (int p = 0; p < count; ++p)
    {
        PinGrid pins; double values[(int) ParamId::Count];
        const bool valid = parseFactoryPreset(presets[p], pins, values);
        if (! valid) std::printf("  preset non valido: %s\n", presets[p].name);
        CHECK(valid);
        int pinCount = 0; for (auto v : pins) if (v) ++pinCount;
        CHECK(pinCount >= 2);

        const int sr = 48000, block = 256; const double seconds = 8.0;
        PSM3Engine e; e.prepare(sr, block);
        for (int i = 0; i < (int) ParamId::Count; ++i) e.setParam((ParamId) i, values[i]);
        e.reset(); e.setPins(pins, 2026);

        std::vector<float> L, R, l((size_t) block), r((size_t) block);
        const int notes[] = { 60, 64, 67, 72, 60, 67 };
        const long total = (long) (seconds * sr);
        for (long pos = 0; pos < total; pos += block)
        {
            if (presets[p].keyboard)
            {
                // una nota al secondo, tenuta 0,6 s
                const long step = sr, phase = pos % step; const int idx = (int) (pos / step) % 6;
                if (phase < block) e.noteOn(notes[idx], 0.5 + 0.08 * idx);
                if (phase >= (long) (0.6 * sr) && phase < (long) (0.6 * sr) + block) e.noteOff(notes[idx]);
            }
            else
            {
                // il joystick descrive un cerchio lento (serve a "Bowing" e "Bird Song")
                const double t = (double) pos / sr;
                e.setParam(ParamId::joyX, values[(int) ParamId::joyX] * 0.2 + 5.0 * 0.8 + 4.0 * std::sin(2.0 * M_PI * 0.7 * t));
                e.setParam(ParamId::joyY, values[(int) ParamId::joyY] * 0.2 + 5.0 * 0.8 + 4.0 * std::cos(2.0 * M_PI * 0.45 * t));
            }
            e.process(nullptr, nullptr, l.data(), r.data(), block);
            L.insert(L.end(), l.begin(), l.end()); R.insert(R.end(), r.begin(), r.end());
        }
        double peak = 0.0, sum = 0.0; bool finite = true;
        for (size_t i = 0; i < L.size(); ++i)
        {
            finite = finite && std::isfinite(L[i]) && std::isfinite(R[i]);
            peak = std::max(peak, (double) std::max(std::fabs(L[i]), std::fabs(R[i])));
            sum += 0.5 * ((double) L[i] * L[i] + (double) R[i] * R[i]);
        }
        const double rmsDb = 10.0 * std::log10(sum / (double) L.size() + 1e-20), peakDb = 20.0 * std::log10(peak + 1e-12);
        std::printf("  %-26s rms %6.1f dBFS   picco %6.1f dBFS\n", presets[p].name, rmsDb, peakDb);
        CHECK(finite);
        CHECK(rmsDb > -45.0);       // si sente
        CHECK(peakDb < -1.0);       // non satura l'uscita
        if (dir != nullptr) writeWav(std::string(dir) + "/" + presets[p].name + ".wav", L, R, sr);
    }
    return checkSummary();
}
