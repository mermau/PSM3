// PSM-3 (PsychoMatrix-3) — Collateral Effects
// Processore JUCE: parametri, MIDI (note, MIDI Learn), pin della matrice, stato e preset.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include "../DSP/PSM3Engine.h"
#include "FactoryPresets.h"

class PSM3Processor : public juce::AudioProcessor
{
public:
    PSM3Processor();
    ~PSM3Processor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    static constexpr int kNumParams = (int) psm3::ParamId::Count;
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;              // tastiera a schermo della DK1

    juce::RangedAudioParameter* getParam (int index) const { return params[(size_t) index]; }
    void setParamValue (int index, double value0to10);  // manopole 0...10, interruttori 0/1

    //==========================================================================
    // Pin della matrice (thread dell'interfaccia o dello stato, mai quello audio)
    psm3::PinGrid getPins() const;
    void setPin (int row, int col, psm3::Pin p);
    void setAllPins (const psm3::PinGrid& pins, juce::uint32 seed);
    void clearPins();
    juce::uint32 getToleranceSeed() const { return toleranceSeed.load(); }
    int getPinsVersion() const { return pinsVersion.load(); }

    //==========================================================================
    // Stato. 'global' comprende cio' che non appartiene al patch: mappa MIDI, scala della finestra, opzioni.
    juce::ValueTree captureState (bool global) const;
    bool applyState (const juce::ValueTree& state, bool global);

    //==========================================================================
    // MIDI Learn: sorgenti 0...127 = Control Change, 128 = Pitch Bend
    static constexpr int kPitchBend = 128, kNumSources = 129;
    void startLearn (int paramIndex)     { learningParam.store (paramIndex); }
    void cancelLearn()                   { learningParam.store (-1); }
    int  getLearningParam() const        { return learningParam.load(); }
    int  getSourceForParam (int paramIndex) const;
    void assignSource (int source, int paramIndex);
    void clearMappingForParam (int paramIndex);
    void clearAllMappings();
    void resetDefaultMappings();          // Mod Wheel -> joystick verticale, Pitch Bend -> orizzontale, CC64 -> Attack
    bool saveMappingsAsUserDefault() const;
    int  getMapVersion() const           { return mapVersion.load(); }
    static juce::String sourceName (int source);

    //==========================================================================
    // Preset
    static int getNumFactoryPresets();
    static juce::String getFactoryPresetName (int index);
    static juce::String getFactoryPresetNote (int index);
    void loadFactoryPreset (int index);
    static juce::File getUserFolder();
    static juce::File getUserPresetFolder();
    juce::Array<juce::File> getUserPresets() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    juce::String getCurrentPresetName() const;

    //==========================================================================
    // Opzioni e indicatori
    void setQuality (int factor)         { quality.store (factor == 2 ? 2 : 4); }
    int  getQuality() const              { return quality.load(); }
    void setWarmUp (bool on)             { warmUp.store (on); engine.setWarmUp (on); }
    bool getWarmUp() const               { return warmUp.load(); }
    void setUiScale (float s)            { uiScale.store (s); }
    float getUiScale() const             { return uiScale.load(); }
    double meterNeedle() const           { return engine.meterNeedle(); }
    bool envelopeLamp() const            { return engine.envelopeLamp(); }
    bool isMainsOn() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void handleMidi (const juce::MidiMessage&);
    void handleSource (int source, float value01);
    void render (juce::AudioBuffer<float>& out, const juce::AudioBuffer<float>* in, int start, int end);
    juce::String mappingsToString() const;
    void mappingsFromString (const juce::String&);
    void pushPinsToEngine();

    psm3::PSM3Engine engine;
    std::array<juce::RangedAudioParameter*, (size_t) kNumParams> params {};
    std::array<std::atomic<float>*, (size_t) kNumParams> raw {};

    mutable juce::SpinLock pinLock;
    psm3::PinGrid pins {};
    std::atomic<juce::uint32> toleranceSeed { 2026 };
    std::atomic<int> pinsVersion { 0 };

    std::array<std::atomic<int>, (size_t) kNumSources> sourceToParam;
    std::atomic<int> learningParam { -1 }, mapVersion { 0 };

    std::atomic<int> quality { 4 };
    int activeQuality = 4;
    std::atomic<bool> warmUp { false };
    std::atomic<float> uiScale { 0.0f };       // 0 = non ancora scelta
    mutable juce::SpinLock nameLock;
    juce::String presetName { "Tastiera base" };

    juce::AudioBuffer<float> inputCopy;
    double currentSampleRate = 48000.0;
    int currentBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PSM3Processor)
};
