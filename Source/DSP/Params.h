// PSM-3 — elenco dei comandi del pannello (manopole 0...10, interruttori 0/1).
#pragma once

namespace psm3
{
enum class ParamId : int
{
    osc1Freq, osc1Shape, osc1Sine, osc1Ramp,
    osc2Freq, osc2Shape, osc2Square, osc2Tri,
    osc3Freq, osc3Shape, osc3Square, osc3Tri,
    noiseColour, noiseLevel,
    in1Level, in2Level,
    ringLevel,
    filtFreq, filtResponse, filtLevel,
    envAttack, envOn, envDecay, envOff, envTrapezoid, envSignal, attackButton,
    revMix, revLevel,
    outFilter1, outFilter2, speaker1, speaker2, meterMode,
    joyX, joyY, rangeX, rangeY,
    out1Level, out1Pan, out2Level, out2Pan,
    mains,
    kbFreq, kbTuning, kbLevel, kbDynRange, kbCh1Mode, kbCh2Mode,
    Count
};

struct ParamInfo
{
    ParamId id;
    const char* key;     // identificatore stabile (stato, automazione, preset)
    const char* name;    // nome mostrato dall'host
    double def;
    bool isSwitch;
};

inline const ParamInfo& paramInfo(ParamId id)
{
    static const ParamInfo table[] = {
        { ParamId::osc1Freq,     "osc1Freq",     "Osc 1 Frequency",        6, false },
        { ParamId::osc1Shape,    "osc1Shape",    "Osc 1 Shape",            5, false },
        { ParamId::osc1Sine,     "osc1Sine",     "Osc 1 Sine Level",       0, false },
        { ParamId::osc1Ramp,     "osc1Ramp",     "Osc 1 Ramp Level",       0, false },
        { ParamId::osc2Freq,     "osc2Freq",     "Osc 2 Frequency",        6, false },
        { ParamId::osc2Shape,    "osc2Shape",    "Osc 2 Shape",            5, false },
        { ParamId::osc2Square,   "osc2Square",   "Osc 2 Square Level",     0, false },
        { ParamId::osc2Tri,      "osc2Tri",      "Osc 2 Triangle Level",   0, false },
        { ParamId::osc3Freq,     "osc3Freq",     "Osc 3 Frequency",        5, false },
        { ParamId::osc3Shape,    "osc3Shape",    "Osc 3 Shape",            5, false },
        { ParamId::osc3Square,   "osc3Square",   "Osc 3 Square Level",     0, false },
        { ParamId::osc3Tri,      "osc3Tri",      "Osc 3 Triangle Level",   0, false },
        { ParamId::noiseColour,  "noiseColour",  "Noise Colour",           5, false },
        { ParamId::noiseLevel,   "noiseLevel",   "Noise Level",            0, false },
        { ParamId::in1Level,     "in1Level",     "Input Level Ch 1",       7, false },
        { ParamId::in2Level,     "in2Level",     "Input Level Ch 2",       7, false },
        { ParamId::ringLevel,    "ringLevel",    "Ring Modulator Level",   0, false },
        { ParamId::filtFreq,     "filtFreq",     "Filter Frequency",       5, false },
        { ParamId::filtResponse, "filtResponse", "Filter Response",        0, false },
        { ParamId::filtLevel,    "filtLevel",    "Filter Level",           0, false },
        { ParamId::envAttack,    "envAttack",    "Envelope Attack",        0, false },
        { ParamId::envOn,        "envOn",        "Envelope On",            0, false },
        { ParamId::envDecay,     "envDecay",     "Envelope Decay",         5, false },
        { ParamId::envOff,       "envOff",       "Envelope Off",          10, false },
        { ParamId::envTrapezoid, "envTrapezoid", "Trapezoid Level",        0, false },
        { ParamId::envSignal,    "envSignal",    "Envelope Signal Level",  0, false },
        { ParamId::attackButton, "attackButton", "Attack Button",          0, true  },
        { ParamId::revMix,       "revMix",       "Reverb Mix",             0, false },
        { ParamId::revLevel,     "revLevel",     "Reverb Level",           0, false },
        { ParamId::outFilter1,   "outFilter1",   "Output Filter Ch 1",     5, false },
        { ParamId::outFilter2,   "outFilter2",   "Output Filter Ch 2",     5, false },
        { ParamId::speaker1,     "speaker1",     "Speaker Ch 1",           0, true  },
        { ParamId::speaker2,     "speaker2",     "Speaker Ch 2",           0, true  },
        { ParamId::meterMode,    "meterMode",    "Meter Signal Levels",    0, true  },
        { ParamId::joyX,         "joyX",         "Joystick Horizontal",    5, false },
        { ParamId::joyY,         "joyY",         "Joystick Vertical",      5, false },
        { ParamId::rangeX,       "rangeX",       "Joystick Range Horiz",  10, false },
        { ParamId::rangeY,       "rangeY",       "Joystick Range Vert",   10, false },
        { ParamId::out1Level,    "out1Level",    "Output Ch 1 Level",      5, false },
        { ParamId::out1Pan,      "out1Pan",      "Output Ch 1 Pan",        5, false },
        { ParamId::out2Level,    "out2Level",    "Output Ch 2 Level",      5, false },
        { ParamId::out2Pan,      "out2Pan",      "Output Ch 2 Pan",        5, false },
        { ParamId::mains,        "mains",        "Mains",                  1, true  },
        { ParamId::kbFreq,       "kbFreq",       "Keyboard Frequency",     5, false },
        { ParamId::kbTuning,     "kbTuning",     "Keyboard Tuning Spread", 5, false },
        { ParamId::kbLevel,      "kbLevel",      "Keyboard Level",         7, false },
        { ParamId::kbDynRange,   "kbDynRange",   "Keyboard Dynamic Range", 5, false },
        { ParamId::kbCh1Mode,    "kbCh1Mode",    "Keyboard Ch 1 Voltage",  1, true  },
        { ParamId::kbCh2Mode,    "kbCh2Mode",    "Keyboard Ch 2 Dynamic",  1, true  },
    };
    return table[(int) id];
}
}
