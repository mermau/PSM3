// PSM-3 — finestra del plugin: pannello verticale, pannello orizzontale con matrice e joystick, tastiera DK1.
#pragma once
#include "Widgets.h"

// Tutto il contenuto alle dimensioni di progetto; l'editor lo scala.
class PanelContent : public juce::Component
{
public:
    explicit PanelContent (PSM3Processor&);
    void paint (juce::Graphics&) override;
    void refresh();                                  // spia, strumento, pin, tasti, MIDI Learn

    static constexpr int baseW = 1240, baseH = 1700;

private:
    struct HotSpot : public juce::Component
    {
        std::function<void()> onClick;
        void mouseDown (const juce::MouseEvent&) override { if (onClick) onClick(); }
    };
    void showParamMenu (juce::Component& target, int paramIndex);
    void showMainMenu();
    void askPresetName();
    void drawUpperPanel (juce::Graphics&);
    void drawLowerPanel (juce::Graphics&);
    void drawKeyboardPanel (juce::Graphics&);

    PSM3Processor& proc;
    juce::OwnedArray<ui::Knob> knobs;
    juce::OwnedArray<ui::Toggle> toggles;
    ui::Lamp envLamp, mainsLamp;
    ui::MeterView meter;
    ui::JoystickView joystick;
    ui::MatrixView matrix;
    ui::KeyboardView keyboard;
    HotSpot nameplate;
    juce::String shownPreset;
    int shownLearning = -2;
};

class PSM3Editor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PSM3Editor (PSM3Processor&);
    ~PSM3Editor() override = default;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    PSM3Processor& proc;
    PanelContent content;
    juce::TooltipWindow tooltips { this, 700 };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PSM3Editor)
};
