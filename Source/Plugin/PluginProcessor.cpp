#include "PluginProcessor.h"
#include "../GUI/PluginEditor.h"

using psm3::ParamId;
using psm3::paramInfo;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PSM3Processor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < kNumParams; ++i)
    {
        const auto& info = paramInfo ((ParamId) i);
        if (info.isSwitch)
            layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { info.key, 1 }, info.name, info.def >= 0.5));
        else
            layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { info.key, 1 }, info.name,
                                                                     juce::NormalisableRange<float> (0.0f, 10.0f), (float) info.def));
    }
    return layout;
}

// Uscita stereo. L'ingresso audio (facoltativo, verso Input Ch 1 e 2) non viene dichiarato come
// Audio Unit: uno strumento AU con ingressi non supera la validazione di alcuni host.
juce::AudioProcessor::BusesProperties PSM3Processor::makeBuses()
{
    auto buses = BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true);
    if (juce::PluginHostType::getPluginLoadedAs() != juce::AudioProcessor::wrapperType_AudioUnit)
        buses = buses.withInput ("Input", juce::AudioChannelSet::stereo(), false);
    return buses;
}

PSM3Processor::PSM3Processor()
    : AudioProcessor (makeBuses()),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < kNumParams; ++i)
    {
        const char* key = paramInfo ((ParamId) i).key;
        params[(size_t) i] = apvts.getParameter (key);
        raw[(size_t) i] = apvts.getRawParameterValue (key);
    }
    for (auto& s : sourceToParam) s.store (-1);
    pins.fill (0);

    // mappa MIDI: quella salvata dall'utente come predefinita, altrimenti quella di fabbrica
    resetDefaultMappings();
    const auto mapFile = getUserFolder().getChildFile ("midimap.xml");
    if (mapFile.existsAsFile())
        if (auto xml = juce::parseXML (mapFile))
            if (xml->hasTagName ("PSM3MIDI"))
                mappingsFromString (xml->getStringAttribute ("map"));

    loadFactoryPreset (0);
}

//==============================================================================
bool PSM3Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    const auto in = layouts.getMainInputChannelSet();
    return in.isDisabled() || in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void PSM3Processor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlock = juce::jmax (16, samplesPerBlock);
    engine.setQuality (quality.load());
    engine.setWarmUp (warmUp.load());
    for (int i = 0; i < kNumParams; ++i) engine.setParam ((ParamId) i, (double) raw[(size_t) i]->load());
    engine.prepare (sampleRate, currentBlock);
    pushPinsToEngine();
    inputCopy.setSize (2, currentBlock, false, true, true);
    setLatencySamples (engine.latencySamples());
    keyboardState.reset();
    isPrepared = true;
}

// Cambio di qualita' (2x/4x) dal menu: si riprepara il motore fuori dal thread audio, a elaborazione sospesa.
void PSM3Processor::setQuality (int factor)
{
    factor = factor == 2 ? 2 : 4;
    if (quality.exchange (factor) == factor || ! isPrepared) return;
    suspendProcessing (true);
    {
        const juce::ScopedLock lock (getCallbackLock());
        engine.setQuality (factor);
        engine.prepare (currentSampleRate, currentBlock);
    }
    pushPinsToEngine();
    setLatencySamples (engine.latencySamples());
    suspendProcessing (false);
}

void PSM3Processor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    engine.allNotesOff();           // un note-off perso durante il bypass non deve lasciare una nota tenuta
    AudioProcessor::processBlockBypassed (buffer, midi);
}

void PSM3Processor::render (juce::AudioBuffer<float>& out, const juce::AudioBuffer<float>* in, int start, int end)
{
    const int chunkMax = inputCopy.getNumSamples();
    if (chunkMax <= 0) { out.clear (start, end - start); return; }      // non ancora preparato
    auto* outL = out.getWritePointer (0);
    auto* outR = out.getNumChannels() > 1 ? out.getWritePointer (1) : nullptr;
    while (start < end)
    {
        const int n = juce::jmin (chunkMax, end - start);
        const float* inL = nullptr; const float* inR = nullptr;
        if (in != nullptr && in->getNumChannels() > 0)
        {
            // l'uscita riscrive la stessa memoria dell'ingresso: prima se ne fa una copia
            inputCopy.copyFrom (0, 0, *in, 0, start, n);
            inputCopy.copyFrom (1, 0, *in, in->getNumChannels() > 1 ? 1 : 0, start, n);
            inL = inputCopy.getReadPointer (0); inR = inputCopy.getReadPointer (1);
        }
        if (outR != nullptr)
            engine.process (inL, inR, outL + start, outR + start, n);
        else
        {
            float* scratch = inputCopy.getWritePointer (1);     // uscita mono (non dovrebbe capitare): si scarta il destro
            engine.process (inL, inL, outL + start, scratch, n);
        }
        start += n;
    }
}

void PSM3Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    if (numSamples <= 0) return;

    keyboardState.processNextMidiBuffer (midi, 0, numSamples, true);

    for (int i = 0; i < kNumParams; ++i) engine.setParam ((ParamId) i, (double) raw[(size_t) i]->load());

    auto inBus = getBusBuffer (buffer, true, 0);
    auto outBus = getBusBuffer (buffer, false, 0);
    const juce::AudioBuffer<float>* in = inBus.getNumChannels() > 0 ? &inBus : nullptr;
    if (outBus.getNumChannels() == 0) return;

    int pos = 0;
    for (const auto meta : midi)
    {
        const int t = juce::jlimit (0, numSamples, meta.samplePosition);
        if (t > pos) { render (outBus, in, pos, t); pos = t; }
        handleMidi (meta.getMessage());
        // un interruttore mosso via MIDI (es. il pedale sul pulsante Attack) vale subito
        for (int i = 0; i < kNumParams; ++i) engine.setParam ((ParamId) i, (double) raw[(size_t) i]->load());
    }
    if (pos < numSamples) render (outBus, in, pos, numSamples);

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, numSamples);
}

//==============================================================================
void PSM3Processor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                               engine.noteOn (m.getNoteNumber(), (double) m.getFloatVelocity());
    else if (m.isNoteOff())                         engine.noteOff (m.getNoteNumber());
    else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
    else if (m.isController())                      handleSource (m.getControllerNumber(), (float) m.getControllerValue() / 127.0f);
    else if (m.isPitchWheel())                      handleSource (kPitchBend, (float) m.getPitchWheelValue() / 16383.0f);
}

void PSM3Processor::handleSource (int source, float value01)
{
    if (source < 0 || source >= kNumSources) return;
    const int learning = learningParam.load();
    if (learning >= 0 && learning < kNumParams)
    {
        assignSource (source, learning);
        learningParam.store (-1);
    }
    const int idx = sourceToParam[(size_t) source].load();
    if (idx < 0 || idx >= kNumParams) return;
    auto* p = params[(size_t) idx];
    const float v = paramInfo ((ParamId) idx).isSwitch ? (value01 >= 0.5f ? 1.0f : 0.0f) : juce::jlimit (0.0f, 1.0f, value01);
    if (std::abs (p->getValue() - v) > 1.0e-6f) p->setValueNotifyingHost (v);
}

int PSM3Processor::getSourceForParam (int paramIndex) const
{
    for (int s = 0; s < kNumSources; ++s) if (sourceToParam[(size_t) s].load() == paramIndex) return s;
    return -1;
}

void PSM3Processor::assignSource (int source, int paramIndex)
{
    if (source < 0 || source >= kNumSources || paramIndex < 0 || paramIndex >= kNumParams) return;
    for (auto& s : sourceToParam) if (s.load() == paramIndex) s.store (-1);     // un comando, una sola sorgente
    sourceToParam[(size_t) source].store (paramIndex);                          // una sorgente, un solo comando
    mapVersion.fetch_add (1);
}

void PSM3Processor::clearMappingForParam (int paramIndex)
{
    for (auto& s : sourceToParam) if (s.load() == paramIndex) s.store (-1);
    mapVersion.fetch_add (1);
}

void PSM3Processor::clearAllMappings()
{
    for (auto& s : sourceToParam) s.store (-1);
    mapVersion.fetch_add (1);
}

void PSM3Processor::resetDefaultMappings()
{
    for (auto& s : sourceToParam) s.store (-1);
    sourceToParam[1].store ((int) ParamId::joyY);                    // Mod Wheel
    sourceToParam[(size_t) kPitchBend].store ((int) ParamId::joyX);  // Pitch Bend
    sourceToParam[64].store ((int) ParamId::attackButton);           // pedale sustain
    mapVersion.fetch_add (1);
}

juce::String PSM3Processor::sourceName (int source)
{
    if (source == kPitchBend) return "Pitch Bend";
    if (source == 1) return "CC 1 (Mod Wheel)";
    if (source >= 0 && source < 128) return "CC " + juce::String (source);
    return {};
}

juce::String PSM3Processor::mappingsToString() const
{
    juce::StringArray items;
    for (int s = 0; s < kNumSources; ++s)
    {
        const int idx = sourceToParam[(size_t) s].load();
        if (idx >= 0 && idx < kNumParams) items.add (juce::String (s) + ":" + paramInfo ((ParamId) idx).key);
    }
    return items.joinIntoString (";");
}

void PSM3Processor::mappingsFromString (const juce::String& text)
{
    for (auto& s : sourceToParam) s.store (-1);
    juce::StringArray items;
    items.addTokens (text, ";", "");
    for (const auto& item : items)
    {
        const int source = item.upToFirstOccurrenceOf (":", false, false).getIntValue();
        const auto key = item.fromFirstOccurrenceOf (":", false, false).trim();
        if (key.isEmpty() || ! item.containsChar (':')) continue;
        const int idx = psm3::paramIndexForKey (key.toRawUTF8(), (size_t) key.getNumBytesAsUTF8());
        if (idx >= 0 && source >= 0 && source < kNumSources) sourceToParam[(size_t) source].store (idx);
    }
    mapVersion.fetch_add (1);
}

bool PSM3Processor::saveMappingsAsUserDefault() const
{
    const auto folder = getUserFolder();
    if (! folder.createDirectory()) return false;
    juce::XmlElement xml ("PSM3MIDI");
    xml.setAttribute ("map", mappingsToString());
    return xml.writeTo (folder.getChildFile ("midimap.xml"));
}

//==============================================================================
void PSM3Processor::setParamValue (int index, double value0to10)
{
    if (index < 0 || index >= kNumParams) return;
    auto* p = params[(size_t) index];
    p->setValueNotifyingHost (p->convertTo0to1 ((float) value0to10));
}

bool PSM3Processor::isMainsOn() const { return raw[(size_t) ParamId::mains]->load() >= 0.5f; }

psm3::PinGrid PSM3Processor::getPins() const
{
    const juce::SpinLock::ScopedLockType lock (pinLock);
    return pins;
}

void PSM3Processor::pushPinsToEngine()
{
    const juce::ScopedLock order (pushLock);        // due thread non audio non devono scavalcarsi
    psm3::PinGrid copy;
    { const juce::SpinLock::ScopedLockType lock (pinLock); copy = pins; }
    engine.setPins (copy, toleranceSeed.load());
}

void PSM3Processor::setPin (int row, int col, psm3::Pin p)
{
    if (row < 0 || row > 15 || col < 0 || col > 15) return;
    { const juce::SpinLock::ScopedLockType lock (pinLock); pins[(size_t) (row * 16 + col)] = (juce::uint8) p; }
    pushPinsToEngine();
    pinsVersion.fetch_add (1);
}

void PSM3Processor::setAllPins (const psm3::PinGrid& newPins, juce::uint32 seed)
{
    {
        const juce::SpinLock::ScopedLockType lock (pinLock);
        for (size_t i = 0; i < pins.size(); ++i) pins[i] = newPins[i] <= 3 ? newPins[i] : (juce::uint8) 0;
    }
    toleranceSeed.store (seed == 0 ? 2026u : seed);
    pushPinsToEngine();
    pinsVersion.fetch_add (1);
}

void PSM3Processor::clearPins()
{
    psm3::PinGrid empty; empty.fill (0);
    setAllPins (empty, toleranceSeed.load());
}

//==============================================================================
juce::ValueTree PSM3Processor::captureState (bool global) const
{
    juce::ValueTree state ("PSM3");
    state.setProperty ("version", 1, nullptr);
    {
        const juce::SpinLock::ScopedLockType lock (nameLock);
        state.setProperty ("preset", presetName, nullptr);
    }

    juce::ValueTree values ("PARAMS");
    for (int i = 0; i < kNumParams; ++i)
        values.setProperty (paramInfo ((ParamId) i).key, (double) raw[(size_t) i]->load(), nullptr);
    state.appendChild (values, nullptr);

    juce::String pinText;
    const auto grid = getPins();
    for (auto v : grid) pinText << (int) v;
    state.setProperty ("pins", pinText, nullptr);
    state.setProperty ("seed", (juce::int64) toleranceSeed.load(), nullptr);

    if (global)
    {
        state.setProperty ("midimap", mappingsToString(), nullptr);
        state.setProperty ("uiScale", (double) uiScale.load(), nullptr);
        state.setProperty ("quality", quality.load(), nullptr);
        state.setProperty ("warmUp", warmUp.load(), nullptr);
    }
    return state;
}

bool PSM3Processor::applyState (const juce::ValueTree& state, bool global)
{
    if (! state.isValid() || ! state.hasType ("PSM3")) return false;

    // Si applica solo cio' che si riconosce: uno stato di una versione futura non rompe nulla.
    const auto values = state.getChildWithName ("PARAMS");
    if (values.isValid())
        for (int i = 0; i < kNumParams; ++i)
        {
            if ((ParamId) i == ParamId::attackButton) continue;      // un pulsante non si ripristina premuto
            const juce::Identifier key (paramInfo ((ParamId) i).key);
            if (! values.hasProperty (key)) continue;
            const double v = (double) values.getProperty (key);
            if (std::isfinite (v)) setParamValue (i, juce::jlimit (0.0, 10.0, v));
        }

    const auto pinText = state.getProperty ("pins").toString();
    if (pinText.length() == 256 && pinText.containsOnly ("0123"))
    {
        psm3::PinGrid grid;
        for (int i = 0; i < 256; ++i) grid[(size_t) i] = (juce::uint8) (pinText[i] - '0');
        const auto seed = (juce::uint32) (juce::int64) state.getProperty ("seed", (juce::int64) 2026);
        setAllPins (grid, seed);
    }

    if (state.hasProperty ("preset"))
    {
        const juce::SpinLock::ScopedLockType lock (nameLock);
        presetName = state.getProperty ("preset").toString();
    }

    if (global)
    {
        if (state.hasProperty ("midimap")) mappingsFromString (state.getProperty ("midimap").toString());
        if (state.hasProperty ("uiScale")) uiScale.store (juce::jlimit (0.0f, 2.0f, (float) (double) state.getProperty ("uiScale")));
        if (state.hasProperty ("quality")) setQuality ((int) state.getProperty ("quality"));
        if (state.hasProperty ("warmUp"))  setWarmUp ((bool) state.getProperty ("warmUp"));
    }
    return true;
}

void PSM3Processor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = captureState (true).createXml())
        copyXmlToBinary (*xml, destData);
}

void PSM3Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0) return;
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        applyState (juce::ValueTree::fromXml (*xml), true);
}

//==============================================================================
int PSM3Processor::getNumFactoryPresets() { int n = 0; psm3::factoryPresets (n); return n; }

juce::String PSM3Processor::getFactoryPresetName (int index)
{
    int n = 0; const auto* table = psm3::factoryPresets (n);
    return index >= 0 && index < n ? juce::String (juce::CharPointer_UTF8 (table[index].name)) : juce::String();
}

juce::String PSM3Processor::getFactoryPresetNote (int index)
{
    int n = 0; const auto* table = psm3::factoryPresets (n);
    return index >= 0 && index < n ? juce::String (juce::CharPointer_UTF8 (table[index].note)) : juce::String();
}

void PSM3Processor::loadFactoryPreset (int index)
{
    int n = 0; const auto* table = psm3::factoryPresets (n);
    if (index < 0 || index >= n) return;
    psm3::PinGrid grid; double values[kNumParams];
    psm3::parseFactoryPreset (table[index], grid, values);
    for (int i = 0; i < kNumParams; ++i)
    {
        // un preset non spegne lo strumento e non tocca il pulsante Attack
        if ((ParamId) i == ParamId::mains || (ParamId) i == ParamId::attackButton) continue;
        setParamValue (i, values[i]);
    }
    setAllPins (grid, 2026);
    const juce::SpinLock::ScopedLockType lock (nameLock);
    presetName = juce::String (juce::CharPointer_UTF8 (table[index].name));
}

juce::File PSM3Processor::getUserFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Collateral Effects").getChildFile ("PSM-3");
}

juce::File PSM3Processor::getUserPresetFolder() { return getUserFolder().getChildFile ("Presets"); }

juce::Array<juce::File> PSM3Processor::getUserPresets() const
{
    auto files = getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.psm3");
    files.sort();
    return files;
}

bool PSM3Processor::saveUserPreset (const juce::String& name)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    const auto folder = getUserPresetFolder();
    if (! folder.createDirectory()) return false;
    {
        const juce::SpinLock::ScopedLockType lock (nameLock);
        presetName = clean;
    }
    if (auto xml = captureState (false).createXml())
        return xml->writeTo (folder.getChildFile (clean + ".psm3"));
    return false;
}

bool PSM3Processor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::parseXML (file);
    if (xml == nullptr) return false;
    if (! applyState (juce::ValueTree::fromXml (*xml), false)) return false;
    const juce::SpinLock::ScopedLockType lock (nameLock);
    presetName = file.getFileNameWithoutExtension();
    return true;
}

juce::String PSM3Processor::getCurrentPresetName() const
{
    const juce::SpinLock::ScopedLockType lock (nameLock);
    return presetName;
}

//==============================================================================
juce::AudioProcessorEditor* PSM3Processor::createEditor() { return new PSM3Editor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PSM3Processor(); }
