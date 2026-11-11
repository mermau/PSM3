// PSM-3 — comandi del pannello: manopole, quadranti, interruttori, matrice, joystick, strumento, tastiera.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Plugin/PluginProcessor.h"
#include "Theme.h"

namespace ui
{
// Base dei comandi legati a un parametro: valore, gesto di automazione, menu MIDI Learn.
class ParamWidget : public juce::Component, public juce::SettableTooltipClient
{
public:
    ParamWidget (PSM3Processor& p, psm3::ParamId id);
    int paramIndex() const { return index; }
    void setLearning (bool on) { if (on != learning) { learning = on; repaint(); } }
    std::function<void (ParamWidget&, int paramIndex)> onContextMenu;
protected:
    void beginDrag();
    void dragTo (float newValue);
    void endDrag();
    void setComplete (float newValue);
    float value = 0.0f;                 // 0...10 per le manopole, 0/1 per gli interruttori
    float defaultValue = 0.0f;
    bool learning = false, dragging = false;
    PSM3Processor& proc;
private:
    int index;
    juce::ParameterAttachment attachment;
};

enum class Cap { Blue, Green, White, Red, Yellow };

// Manopola con gonna nera e cappuccio colorato; 'vernier' = quadrante a demoltiplica delle frequenze.
class Knob : public ParamWidget
{
public:
    Knob (PSM3Processor& p, psm3::ParamId id, Cap cap, bool vernier = false);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
private:
    Cap cap; bool vernier; float dragStart = 0.0f;
};

// Interruttore a levetta a due posizioni. 'momentary' = pulsante (Attack).
class Toggle : public ParamWidget
{
public:
    enum class Style { Lever, Button };
    Toggle (PSM3Processor& p, psm3::ParamId id, Style s, bool invert = false);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    Style style; bool invert;
};

class Lamp : public juce::Component
{
public:
    void setOn (bool b) { if (b != on) { on = b; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    bool on = false;
};

class MeterView : public juce::Component
{
public:
    void setNeedle (float v) { if (std::abs (v - needle) > 0.002f) { needle = v; repaint(); } }
    void paint (juce::Graphics&) override;
private:
    float needle = 0.5f;
};

class JoystickView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit JoystickView (PSM3Processor& p);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void setLearning (int axis) { if (axis != learningAxis) { learningAxis = axis; repaint(); } }   // -1, 0 = orizzontale, 1 = verticale
    std::function<void (juce::Component&, int paramIndex)> onContextMenu;
private:
    void moveTo (juce::Point<float> p);
    float x = 5.0f, y = 5.0f; int learningAxis = -1; bool dragging = false;
    juce::ParameterAttachment attX, attY;
};

class MatrixView : public juce::Component, public juce::TooltipClient
{
public:
    explicit MatrixView (PSM3Processor& p) : proc (p) { refresh(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::String getTooltip() override;
    void refresh();                         // rilegge i pin dal processore se sono cambiati
    static const char* rowName (int row);
    static const char* colName (int col);
private:
    bool cellAt (juce::Point<float> p, int& row, int& col) const;
    PSM3Processor& proc;
    psm3::PinGrid pins {};
    int version = -1;
};

class KeyboardView : public juce::Component
{
public:
    explicit KeyboardView (PSM3Processor& p) : proc (p) {}
    ~KeyboardView() override { release(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void refresh();
    static constexpr int kLowNote = 48, kHighNote = 84;     // 3 ottave do-do
private:
    int noteAt (juce::Point<float> p, float& velocity) const;
    juce::Rectangle<float> keyBounds (int note) const;
    void press (int note, float velocity);
    void release();
    PSM3Processor& proc;
    int held = -1;
    juce::uint64 shown[2] = { 0, 0 };
};
}
