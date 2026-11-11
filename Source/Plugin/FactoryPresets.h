// PSM-3 — preset di fabbrica. Solo libreria standard: usato sia dal plugin sia dai test nativi.
// Pin: sigle come sul pannello ("B1" = colonna B, riga 1); suffisso r = pin rosso, g = pin verde.
// Manopole: "chiave=valore" (chiavi di Params.h); quelle non citate restano al valore predefinito.
#pragma once
#include "../DSP/KeyboardDK1.h"
#include "../DSP/Matrix.h"
#include "../DSP/Params.h"
#include <cstdlib>
#include <cstring>
#include <string>

namespace psm3
{
struct FactoryPreset
{
    const char* name;
    bool keyboard;          // si suona dalla tastiera
    const char* pins;
    const char* params;
    const char* note;       // da dove viene e che cosa fa
};

// in1Level che rende la tastiera cromatica: 6.71873 sugli oscillatori (0,32 V/ottava, pin rosso),
// 5.756 sul filtro (0,20 V/ottava, pin rosso). osc1Freq/osc2Freq 6.3333: il do centrale suona a 261,6 Hz.
inline const FactoryPreset* factoryPresets(int& count)
{
    static const FactoryPreset table[] = {
        { "Tastiera base", true,
          "D1 B12 C12 I8r",
          "osc1Freq=6.3333 osc1Sine=7 envSignal=8 envOff=10 envDecay=5 in1Level=6.71873 kbCh1Mode=1 kbCh2Mode=1 out1Level=6 out2Level=6",
          "Punto di partenza: Osc 1 nell'inviluppo, tastiera cromatica sulla frequenza." },
        { "Tastiera con filtro", true,
          "H2 H3 D10 B12 C12 I8r J8r N9 G12 C14",
          "osc1Freq=6.3333 osc2Freq=6.3433 osc1Ramp=3 osc2Square=2.5 osc2Shape=4 filtFreq=6 filtResponse=4.5 filtLevel=9 envSignal=8 envAttack=1 envOff=10 envDecay=6 in1Level=6.71873 in2Level=6 kbCh1Mode=1 kbCh2Mode=1 revMix=3 revLevel=8 out1Level=7 out2Level=7",
          "Due oscillatori nel filtro; la forza del tocco apre il filtro; un po' di riverbero a destra." },
        { "01 Coloured Noise", false,
          "H7 D10 B12 C12 N6 L1 L3 O15 P16",
          "noiseLevel=7 noiseColour=5 filtFreq=5 filtResponse=5 filtLevel=9 envAttack=2 envOn=2 envDecay=5 envOff=3 envSignal=8 osc3Freq=4 osc3Tri=5 osc1Freq=2 osc1Sine=4 osc2Freq=1 osc2Square=3 out1Level=7 out2Level=7",
          "Manuale d'uso, sezione 3 ed esperimento 1: rumore nel filtro e nell'inviluppo; il joystick dosa le due uscite." },
        { "03 Wind", false,
          "H7 B10 C10 N6 N4",
          "noiseLevel=7 noiseColour=5 filtFreq=5.5 filtResponse=5.3 filtLevel=9 osc3Freq=2.5 osc3Tri=4 osc2Freq=0.6 osc2Tri=2 out1Level=7 out2Level=7",
          "Esperimento 3: rumore bianco nel filtro quasi risonante, mosso da oscillatori lenti." },
        { "04 Bells", false,
          "E1 F4 H13 D10 G12 B14 C14",
          "osc1Freq=7.5 osc1Sine=4.5 osc2Freq=7.7 osc2Tri=3 ringLevel=9 filtFreq=8.6 filtResponse=3 filtLevel=10 envAttack=0 envOn=0 envDecay=6.5 envOff=6 envSignal=9 revMix=4 revLevel=9 out1Level=8 out2Level=8",
          "Esperimento 4: due oscillatori acuti nel modulatore ad anello, poi filtro, inviluppo e riverbero." },
        { "05 Bird Song", false,
          "N1 N4 N6 D10 B12 C12 I15 J16 K15 L16",
          "osc1Freq=3 osc1Sine=5 osc2Freq=5 osc2Tri=5 osc3Freq=7 osc3Tri=5 filtFreq=8 filtResponse=10 filtLevel=7 envAttack=0 envOn=1 envDecay=5 envOff=2 envSignal=8 joyX=6 joyY=4 out1Level=7 out2Level=7",
          "Esperimento 5: tre oscillatori muovono il filtro che oscilla; il joystick cambia il canto." },
        { "06 Bounces", false,
          "B14 C14 E3 F1 G13 I3 J6",
          "osc1Freq=10 osc1Sine=5 osc2Freq=1.5 osc2Square=5 osc3Freq=4.5 osc3Shape=10 osc3Tri=5 ringLevel=8 revMix=3 revLevel=8 out1Level=7 out2Level=7",
          "Esperimento 6: la pallina da ping-pong che rimbalza." },
        { "09 Pizzicato", true,
          "E1 F8 I8r B13 C13 G13 B14",
          "osc1Freq=6.3333 osc1Sine=5 ringLevel=9 in1Level=6.71873 kbCh1Mode=1 revMix=3 revLevel=7 out1Level=8 out2Level=8",
          "Esperimento 9: il modulatore ad anello 'differenzia' la tensione di tastiera: note cortissime anche a tasto tenuto." },
        { "10 Chords", false,
          "B1 C2 C3 B4 C5 B6",
          "osc1Freq=5.5 osc1Sine=5 osc1Ramp=4 osc2Freq=5.94 osc2Square=3 osc2Tri=4 osc3Freq=9.4 osc3Square=2 osc3Tri=3 out1Level=6 out2Level=6",
          "Esperimento 10: accordo a tre parti, timbri diversi sui due canali." },
        { "11 Scales", false,
          "B1 C1 I4 I6",
          "osc1Freq=5.5 osc1Sine=6 osc2Freq=2.5 osc2Shape=0 osc2Tri=3 osc3Freq=3 osc3Shape=10 osc3Tri=4 out1Level=7 out2Level=7",
          "Esperimento 11: Osc 2 e Osc 3 con Shape opposti fanno suonare scale all'Osc 1." },
        { "12 Bowing", false,
          "E1 F16 G13 B14 C14 I15",
          "osc1Freq=6 osc1Sine=4.5 ringLevel=9 revMix=3 revLevel=8 rangeX=6 rangeY=10 out1Level=8 out2Level=8",
          "Esperimento 12: si sente solo mentre il joystick si muove, come un archetto." },
        { "13 Melodies with Noise", true,
          "N8r H7 D10 B12 C12",
          "noiseLevel=7 filtFreq=6.3 filtResponse=5.6 filtLevel=9 envOff=10 envDecay=5 envSignal=8 in1Level=5.756 kbCh1Mode=1 out1Level=8 out2Level=8",
          "Esperimento 13: la tastiera sposta il filtro sul rumore; alzando Response emerge la melodia." },
        { "14 Timbre Changes", true,
          "E2 F4 H13 D10 B12 C12 I8r J8r N9",
          "osc1Freq=6.3333 osc2Freq=6.3333 osc1Ramp=4 osc2Tri=3 ringLevel=8 filtFreq=5.5 filtResponse=3 filtLevel=9 envOff=10 envDecay=5 envSignal=8 in1Level=6.71873 in2Level=6 kbCh1Mode=1 kbCh2Mode=1 out1Level=8 out2Level=8",
          "Esperimento 14: il timbro dipende dalla forza con cui si batte il tasto." },
        { "16 Filtering", true,
          "H8 B10 C10 N6",
          "kbCh1Mode=0 kbLevel=7 in1Level=7 filtFreq=6 filtResponse=6 filtLevel=9 osc3Freq=5 osc3Tri=4 out1Level=7 out2Level=7",
          "Esperimento 16: 'wah-wah' automatico. Qui lo strumento esterno e' l'oscillatore della tastiera; con l'ingresso audio attivo si puo' filtrare qualsiasi segnale." },
        { "18 Reverberation Chop", true,
          "G8 C8 B14 C14 M5",
          "kbCh1Mode=0 kbLevel=7 in1Level=7 osc3Freq=6.5 osc3Square=4 revMix=5 revLevel=8 out1Level=7 out2Level=7",
          "Esperimento 18: Osc 3 'trita' il riverbero controllato in tensione." },
        { "19 Ring Modulation", true,
          "F8 E1 B13 C13",
          "kbCh1Mode=0 kbLevel=6 in1Level=6 osc1Freq=6 osc1Sine=4.5 ringLevel=8 out1Level=7 out2Level=7",
          "Esperimento 19: lo strumento (qui l'oscillatore di tastiera) modulato ad anello con una sinusoide." },
        { "22 Beats", false,
          "B1 B10 C1 C10",
          "osc1Freq=6 osc1Sine=5 filtFreq=5.02 filtResponse=10 filtLevel=5 out1Level=6 out2Level=6",
          "Esperimento 22: due sinusoidi quasi all'unisono battono." },
        { "25 Vibrato (FM)", false,
          "B1 C1 I4",
          "osc1Freq=6 osc1Sine=5 osc2Freq=2 osc2Shape=5 osc2Tri=5 out1Level=6 out2Level=6",
          "Esperimento 25 (ii): Osc 2 modula la frequenza di Osc 1." },
    };
    count = (int) (sizeof(table) / sizeof(table[0]));
    return table;
}

inline int paramIndexForKey(const char* key, size_t len)
{
    for (int i = 0; i < (int) ParamId::Count; ++i)
    {
        const char* k = paramInfo((ParamId) i).key;
        if (std::strlen(k) == len && std::strncmp(k, key, len) == 0) return i;
    }
    return -1;
}

// Converte un preset in griglia di pin e valori delle manopole. Ritorna false se contiene qualcosa di non valido.
inline bool parseFactoryPreset(const FactoryPreset& p, PinGrid& pins, double values[(int) ParamId::Count])
{
    bool ok = true;
    pins.fill(0);
    for (int i = 0; i < (int) ParamId::Count; ++i) values[i] = paramInfo((ParamId) i).def;

    for (const char* s = p.pins; *s != 0;)
    {
        while (*s == ' ') ++s;
        if (*s == 0) break;
        const int col = *s - 'A';
        char* end = nullptr;
        const long row = std::strtol(s + 1, &end, 10);
        Pin pin = Pin::White;
        if (*end == 'r') { pin = Pin::Red; ++end; } else if (*end == 'g') { pin = Pin::Green; ++end; }
        if (col < 0 || col > 15 || row < 1 || row > 16 || (*end != ' ' && *end != 0)) { ok = false; while (*end != ' ' && *end != 0) ++end; }
        else pins[(size_t) ((row - 1) * 16 + col)] = (uint8_t) pin;
        s = end;
    }
    for (const char* s = p.params; *s != 0;)
    {
        while (*s == ' ') ++s;
        if (*s == 0) break;
        const char* eq = std::strchr(s, '=');
        if (eq == nullptr) { ok = false; break; }
        char* end = nullptr;
        const double v = std::strtod(eq + 1, &end);
        const int idx = paramIndexForKey(s, (size_t) (eq - s));
        if (idx < 0 || end == eq + 1 || v < 0.0 || v > 10.0) ok = false; else values[idx] = v;
        s = end;
    }
    return ok;
}
}
