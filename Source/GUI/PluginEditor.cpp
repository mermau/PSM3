#include "PluginEditor.h"

using psm3::ParamId;
using ui::Cap;

namespace
{
// Geometria (pixel alle dimensioni di progetto)
constexpr float kLeft = 34.0f, kRight = 1206.0f;                // bordi interni dei fianchi in legno
constexpr float kUpTop = 20.0f, kUpBottom = 788.0f;             // pannello verticale
constexpr float kLowTop = 800.0f, kLowBottom = 1400.0f;         // pannello orizzontale
constexpr float kKbTop = 1420.0f, kKbBottom = 1690.0f;          // tastiera DK1
constexpr float kRowY[4] = { 122.0f, 314.0f, 506.0f, 700.0f };  // centri delle quattro file di manopole
constexpr float kMidX = 634.0f;                                 // divisione tra oscillatori e trattamenti

struct KnobSpec
{
    ParamId id; float x, y, half; Cap cap; bool vernier;
    const char* label;      // sopra la manopola
    const char* tag;        // numero di riga o lettera di colonna della matrice, sotto
    bool roundTag;
};

const KnobSpec kKnobs[] = {
    // Oscillatori
    { ParamId::osc1Freq,   135, kRowY[0], 62, Cap::Blue,  true,  "FREQUENCY", "I", false },
    { ParamId::osc1Shape,  300, kRowY[0], 48, Cap::Green, false, "SHAPE", "", false },
    { ParamId::osc1Sine,   440, kRowY[0], 48, Cap::White, false, "", "1", false },
    { ParamId::osc1Ramp,   565, kRowY[0], 48, Cap::White, false, "", "2", false },
    { ParamId::osc2Freq,   135, kRowY[1], 62, Cap::Blue,  true,  "FREQUENCY", "J", false },
    { ParamId::osc2Shape,  300, kRowY[1], 48, Cap::Green, false, "SHAPE", "", false },
    { ParamId::osc2Square, 440, kRowY[1], 48, Cap::White, false, "", "3", false },
    { ParamId::osc2Tri,    565, kRowY[1], 48, Cap::White, false, "", "4", false },
    { ParamId::osc3Freq,   135, kRowY[2], 62, Cap::Blue,  true,  "FREQUENCY", "K", false },
    { ParamId::osc3Shape,  300, kRowY[2], 48, Cap::Green, false, "SHAPE", "", false },
    { ParamId::osc3Square, 440, kRowY[2], 48, Cap::White, false, "", "5", false },
    { ParamId::osc3Tri,    565, kRowY[2], 48, Cap::White, false, "", "6", false },
    // Rumore e ingressi
    { ParamId::noiseColour, 125, kRowY[3], 48, Cap::White, false, "COLOUR", "", false },
    { ParamId::noiseLevel,  255, kRowY[3], 48, Cap::White, false, "LEVEL", "7", false },
    { ParamId::in1Level,    410, kRowY[3], 48, Cap::White, false, "CHANNEL 1", "8", false },
    { ParamId::in2Level,    545, kRowY[3], 48, Cap::White, false, "CHANNEL 2", "9", false },
    // Modulatore ad anello e filtro
    { ParamId::ringLevel,    717, kRowY[0], 48, Cap::White, false, "LEVEL", "13", false },
    { ParamId::filtFreq,     885, kRowY[0], 62, Cap::Blue,  true,  "FREQUENCY", "N", false },
    { ParamId::filtResponse, 1035, kRowY[0], 48, Cap::Green, false, "RESPONSE", "", false },
    { ParamId::filtLevel,    1150, kRowY[0], 48, Cap::White, false, "LEVEL", "10", false },
    // Inviluppo
    { ParamId::envAttack, 705,  kRowY[1], 48, Cap::Red, false, "ATTACK", "", false },
    { ParamId::envOn,     845,  kRowY[1], 48, Cap::Red, false, "ON", "", false },
    { ParamId::envDecay,  995,  kRowY[1], 48, Cap::Red, false, "DECAY", "L", false },
    { ParamId::envOff,    1135, kRowY[1], 48, Cap::Red, false, "OFF", "", false },
    { ParamId::envTrapezoid, 995,  kRowY[2], 48, Cap::White, false, "TRAPEZOID", "11", false },
    { ParamId::envSignal,    1135, kRowY[2], 48, Cap::White, false, "SIGNAL", "12", false },
    // Riverbero
    { ParamId::revMix,   705, kRowY[2], 48, Cap::Green, false, "MIX", "M", false },
    { ParamId::revLevel, 835, kRowY[2], 48, Cap::White, false, "LEVEL", "14", false },
    // Filtri d'uscita
    { ParamId::outFilter1, 715, 686, 44, Cap::Yellow, false, "CHANNEL 1", "", false },
    { ParamId::outFilter2, 850, 686, 44, Cap::Yellow, false, "CHANNEL 2", "", false },
    // Pannello orizzontale
    { ParamId::rangeX,    880,  890,  44, Cap::White, false, "", "15", false },
    { ParamId::rangeY,    840,  1065, 44, Cap::White, false, "", "16", false },
    { ParamId::out1Level, 845,  1295, 52, Cap::White, false, "LEVEL", "O", false },
    { ParamId::out1Pan,   960,  1300, 42, Cap::White, false, "PAN (EXT.)", "", false },
    { ParamId::out2Pan,   1060, 1300, 42, Cap::White, false, "PAN (EXT.)", "", false },
    { ParamId::out2Level, 1155, 1295, 52, Cap::White, false, "LEVEL", "P", false },
    // DK1
    { ParamId::kbFreq,     88,  1500, 38, Cap::White, false, "FREQUENCY", "", false },
    { ParamId::kbTuning,   180, 1500, 38, Cap::White, false, "TUNING", "", false },
    { ParamId::kbLevel,    88,  1612, 38, Cap::White, false, "LEVEL", "", false },
    { ParamId::kbDynRange, 180, 1612, 38, Cap::White, false, "DYNAMIC", "", false },
};

struct ToggleSpec { ParamId id; float x, y, w, h; ui::Toggle::Style style; bool invert; };
const ToggleSpec kToggles[] = {
    { ParamId::speaker1,     715,  764,  30, 40, ui::Toggle::Style::Lever, false },
    { ParamId::speaker2,     850,  764,  30, 40, ui::Toggle::Style::Lever, false },
    { ParamId::meterMode,    1170, 690,  30, 44, ui::Toggle::Style::Lever, true },
    { ParamId::mains,        1085, 885,  32, 46, ui::Toggle::Style::Lever, true },    // acceso = levetta in giu', verso ON
    { ParamId::attackButton, 1140, 1065, 60, 60, ui::Toggle::Style::Button, false },
    { ParamId::kbCh1Mode,    275,  1500, 30, 44, ui::Toggle::Style::Lever, false },
    { ParamId::kbCh2Mode,    275,  1612, 30, 44, ui::Toggle::Style::Lever, false },
};

const juce::Rectangle<int> kMatrixBounds (300, 890, 432, 432);
const juce::Rectangle<int> kNameplate (70, 1350, 440, 44);
}

//==============================================================================
PanelContent::PanelContent (PSM3Processor& p) : proc (p), joystick (p), matrix (p), keyboard (p)
{
    setOpaque (true);
    auto menu = [this] (ui::ParamWidget& w, int index) { showParamMenu (w, index); };

    for (const auto& s : kKnobs)
    {
        auto* k = knobs.add (new ui::Knob (proc, s.id, s.cap, s.vernier));
        k->setBounds (juce::Rectangle<float> (s.half * 2.0f, s.half * 2.0f).withCentre ({ s.x, s.y }).toNearestInt());
        k->onContextMenu = menu;
        addAndMakeVisible (k);
    }
    for (const auto& s : kToggles)
    {
        auto* t = toggles.add (new ui::Toggle (proc, s.id, s.style, s.invert));
        t->setBounds (juce::Rectangle<float> (s.w, s.h).withCentre ({ s.x, s.y }).toNearestInt());
        t->onContextMenu = menu;
        addAndMakeVisible (t);
    }

    envLamp.setBounds (760, 236, 26, 26);       addAndMakeVisible (envLamp);
    mainsLamp.setBounds (1122, 870, 30, 30);    addAndMakeVisible (mainsLamp);
    meter.setBounds (952, 622, 180, 136);       addAndMakeVisible (meter);
    joystick.setBounds (915, 980, 170, 170);    addAndMakeVisible (joystick);
    joystick.onContextMenu = [this] (juce::Component& c, int index) { showParamMenu (c, index); };
    matrix.setBounds (kMatrixBounds);           addAndMakeVisible (matrix);
    keyboard.setBounds (345, 1432, 855, 248);   addAndMakeVisible (keyboard);
    nameplate.setBounds (kNameplate);           addAndMakeVisible (nameplate);
    nameplate.onClick = [this] { showMainMenu(); };
    nameplate.setMouseCursor (juce::MouseCursor::PointingHandCursor);

    setSize (baseW, baseH);
    refresh();
}

void PanelContent::refresh()
{
    envLamp.setOn (proc.envelopeLamp());
    mainsLamp.setOn (proc.isMainsOn());
    meter.setNeedle ((float) proc.meterNeedle());
    matrix.refresh();
    keyboard.refresh();

    const int learning = proc.getLearningParam();
    if (learning != shownLearning)
    {
        shownLearning = learning;
        for (auto* k : knobs) k->setLearning (k->paramIndex() == learning);
        for (auto* t : toggles) t->setLearning (t->paramIndex() == learning);
        joystick.setLearning (learning == (int) ParamId::joyX ? 0 : (learning == (int) ParamId::joyY ? 1 : -1));
    }
    const auto name = proc.getCurrentPresetName();
    if (name != shownPreset) { shownPreset = name; repaint (kNameplate.expanded (4).withWidth (680)); }
}

//==============================================================================
void PanelContent::showParamMenu (juce::Component& target, int paramIndex)
{
    juce::PopupMenu m;
    auto addFor = [&] (int index, int baseId, const juce::String& prefix)
    {
        const int source = proc.getSourceForParam (index);
        m.addSectionHeader (prefix + juce::String (psm3::paramInfo ((ParamId) index).name));
        m.addItem (baseId, "Assegna MIDI: muovi un controllo della tastiera...");
        m.addItem (baseId + 1, source >= 0 ? "Rimuovi assegnazione (" + PSM3Processor::sourceName (source) + ")" : juce::String ("Nessuna assegnazione"),
                   source >= 0);
        m.addItem (baseId + 2, "Valore predefinito");
    };
    const bool isStick = paramIndex == (int) ParamId::joyX;
    addFor (paramIndex, 100, {});
    if (isStick) addFor ((int) ParamId::joyY, 200, {});
    if (proc.getLearningParam() >= 0) { m.addSeparator(); m.addItem (1, "Annulla l'attesa MIDI"); }

    juce::Component::SafePointer<PanelContent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&target),
                     [safe, paramIndex] (int result)
                     {
                         if (safe == nullptr || result == 0) return;
                         auto& pr = safe->proc;
                         if (result == 1) { pr.cancelLearn(); return; }
                         const int index = result >= 200 ? (int) ParamId::joyY : paramIndex;
                         switch (result % 100)
                         {
                             case 0: pr.startLearn (index); break;
                             case 1: pr.clearMappingForParam (index); break;
                             case 2: pr.setParamValue (index, psm3::paramInfo ((ParamId) index).def); break;
                             default: break;
                         }
                     });
}

void PanelContent::askPresetName()
{
    auto* w = new juce::AlertWindow ("Salva preset", "Nome del preset:", juce::MessageBoxIconType::NoIcon, this);
    w->addTextEditor ("name", proc.getCurrentPresetName());
    w->addButton ("Salva", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Annulla", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PanelContent> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (safe != nullptr && result == 1) safe->proc.saveUserPreset (w->getTextEditorContents ("name"));
    }), true);
}

void PanelContent::showMainMenu()
{
    enum { idSave = 1, idOpenFolder, idClearPins, idMidiClear, idMidiSaveDefault, idMidiReset, idQuality2, idQuality4, idWarmUp, idAbout,
           idFactory = 1000, idUser = 2000 };
    juce::PopupMenu m, factory, user, midi, quality;

    const int n = PSM3Processor::getNumFactoryPresets();
    for (int i = 0; i < n; ++i) factory.addItem (idFactory + i, PSM3Processor::getFactoryPresetName (i));
    const auto files = proc.getUserPresets();
    for (int i = 0; i < files.size(); ++i) user.addItem (idUser + i, files[i].getFileNameWithoutExtension());

    m.addSectionHeader ("Preset: " + proc.getCurrentPresetName());
    m.addSubMenu ("Preset di fabbrica", factory);
    m.addSubMenu ("Preset utente", user, files.size() > 0);
    m.addItem (idSave, "Salva preset...");
    m.addItem (idOpenFolder, "Apri la cartella dei preset");
    m.addSeparator();
    m.addItem (idClearPins, "Togli tutti i pin");
    m.addSeparator();
    midi.addItem (idMidiClear, "Azzera tutte le assegnazioni");
    midi.addItem (idMidiReset, "Ripristina quelle di fabbrica");
    midi.addItem (idMidiSaveDefault, "Salva le attuali come predefinite");
    m.addSubMenu ("MIDI", midi);
    quality.addItem (idQuality4, "Alta (4x)", true, proc.getQuality() == 4);
    quality.addItem (idQuality2, "Leggera (2x)", true, proc.getQuality() == 2);
    m.addSubMenu (juce::String (juce::CharPointer_UTF8 ("Qualit\xc3\xa0")), quality);
    m.addItem (idWarmUp, "Riscaldamento originale del rumore (20-30 s)", true, proc.getWarmUp());
    m.addSeparator();
    m.addItem (idAbout, "Informazioni");

    juce::Component::SafePointer<PanelContent> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&nameplate),
                     [safe, files] (int result)
                     {
                         if (safe == nullptr || result == 0) return;
                         auto& pr = safe->proc;
                         if (result >= idUser)         { if (result - idUser < files.size()) pr.loadUserPreset (files[result - idUser]); }
                         else if (result >= idFactory) pr.loadFactoryPreset (result - idFactory);
                         else switch (result)
                         {
                             case idSave:            safe->askPresetName(); break;
                             case idOpenFolder:      PSM3Processor::getUserPresetFolder().createDirectory(); PSM3Processor::getUserPresetFolder().revealToUser(); break;
                             case idClearPins:       pr.clearPins(); break;
                             case idMidiClear:       pr.clearAllMappings(); break;
                             case idMidiReset:       pr.resetDefaultMappings(); break;
                             case idMidiSaveDefault: pr.saveMappingsAsUserDefault(); break;
                             case idQuality2:        pr.setQuality (2); break;
                             case idQuality4:        pr.setQuality (4); break;
                             case idWarmUp:          pr.setWarmUp (! pr.getWarmUp()); break;
                             case idAbout:
                                 juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon, "PSM-3 (PsychoMatrix-3)",
                                     juce::String (juce::CharPointer_UTF8 (
                                         "Collateral Effects\n\nEmulazione del sintetizzatore a matrice EMS VCS3 (1969) con tastiera DK1, "
                                         "ricavata dal manuale d'uso, dal manuale di servizio e dagli schemi originali.\n\n"
                                         "Clic su un foro della matrice: pin bianco, rosso, verde, vuoto. Clic destro: toglie il pin.\n"
                                         "Clic destro su un comando: MIDI Learn. Doppio clic: valore predefinito. Maiusc: regolazione fine.")));
                                 break;
                             default: break;
                         }
                     });
}

//==============================================================================
void PanelContent::paint (juce::Graphics& g)
{
    const auto& t = ui::theme();
    // mobile in legno
    juce::ColourGradient wood (t.woodLight, 0.0f, 0.0f, t.woodDark, (float) baseW, (float) baseH, false);
    wood.addColour (0.5, t.woodLight.interpolatedWith (t.woodDark, 0.35f));
    g.setGradientFill (wood);
    g.fillAll();
    g.setColour (juce::Colours::black.withAlpha (0.10f));
    for (int i = 0; i < 60; ++i)           // venature
        g.drawLine ((float) ((i * 37) % 34), 0.0f, (float) ((i * 37) % 34) + 3.0f, (float) baseH, 0.6f);
    for (int i = 0; i < 60; ++i)
        g.drawLine (kRight + (float) ((i * 29) % 34), 0.0f, kRight + (float) ((i * 29) % 34) - 3.0f, (float) baseH, 0.6f);

    auto panel = [&] (float top, float bottom)
    {
        const juce::Rectangle<float> r (kLeft, top, kRight - kLeft, bottom - top);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRect (r.translated (0.0f, 3.0f));
        juce::ColourGradient cg (t.panel, 0.0f, top, t.panelShade, 0.0f, bottom, false);
        g.setGradientFill (cg);
        g.fillRect (r);
    };
    panel (kUpTop, kUpBottom);
    panel (kLowTop, kLowBottom);
    panel (kKbTop, kKbBottom);

    drawUpperPanel (g);
    drawLowerPanel (g);
    drawKeyboardPanel (g);

    // etichette e sigle delle manopole
    g.setColour (t.ink);
    for (const auto& s : kKnobs)
    {
        if (s.label[0] != 0) ui::textAt (g, s.label, s.x, s.y - s.half - 10.0f, s.half > 40.0f ? 13.0f : 11.0f);
        if (s.tag[0] != 0)   ui::tag (g, s.tag, s.x + (s.vernier ? s.half * 0.95f : 0.0f), s.y + s.half + (s.vernier ? -8.0f : 10.0f), 11.0f, s.roundTag);
        g.setColour (t.ink);
    }
}

void PanelContent::drawUpperPanel (juce::Graphics& g)
{
    const auto& t = ui::theme();
    g.setColour (t.ink);
    const float th = 1.6f;
    // divisioni
    for (float y : { 212.0f, 404.0f, 596.0f }) g.drawLine (kLeft, y, kMidX, y, th);
    g.drawLine (kMidX, kUpTop, kMidX, kUpBottom, th);
    g.drawLine (kMidX, 212.0f, kRight, 212.0f, th);
    g.drawLine (kMidX, 404.0f, 912.0f, 404.0f, th);
    g.drawLine (912.0f, 404.0f, 912.0f, 596.0f, th);
    g.drawLine (kMidX, 596.0f, kRight, 596.0f, th);
    g.drawLine (800.0f, kUpTop, 800.0f, 212.0f, th);
    g.drawLine (335.0f, 596.0f, 335.0f, kUpBottom, th);
    g.drawLine (940.0f, 596.0f, 940.0f, kUpBottom, th);

    ui::titleBar (g, "OSCILLATOR 1", kLeft, kUpTop);
    ui::titleBar (g, "OSCILLATOR 2", kLeft, 212.0f);
    ui::titleBar (g, "OSCILLATOR 3", kLeft, 404.0f);
    ui::titleBar (g, "NOISE GENERATOR", kLeft, 596.0f);
    ui::titleBar (g, "INPUT LEVEL", 335.0f, 596.0f);
    ui::titleBar (g, "RING MODULATOR", kMidX, kUpTop, 13.0f);
    ui::titleBar (g, "FILTER/OSCILLATOR", 800.0f, kUpTop);
    ui::titleBar (g, "ENVELOPE SHAPER", kMidX, 212.0f);
    ui::titleBar (g, "REVERBERATION", kMidX, 404.0f);
    ui::titleBar (g, "OUTPUT FILTER", kMidX, 596.0f);

    g.setColour (t.ink);
    // gamme degli oscillatori
    const char* lo[3] = { "1Hz", "1Hz", ".05Hz" }; const char* hi[3] = { "10KHz", "10KHz", "500Hz" };
    for (int i = 0; i < 3; ++i)
    {
        ui::textAt (g, lo[i], 62.0f, kRowY[i] - 48.0f, 10.0f);
        ui::textAt (g, hi[i], 216.0f, kRowY[i] - 48.0f, 10.0f);
        ui::textAt (g, "LEVEL", 502.0f, kRowY[i] - 66.0f, 13.0f);
    }
    // simboli delle forme d'onda sopra i livelli
    auto wave = [&] (int kind, float cx, float cy)
    {
        juce::Path p; const float w = 26.0f, h = 9.0f;
        if (kind == 0)       { p.startNewSubPath (cx - w / 2, cy); for (int i = 1; i <= 24; ++i) p.lineTo (cx - w / 2 + w * i / 24.0f, cy - h * std::sin (juce::MathConstants<float>::twoPi * i / 24.0f)); }
        else if (kind == 1)  { p.startNewSubPath (cx - w / 2, cy + h); p.lineTo (cx + w / 2, cy - h); p.lineTo (cx + w / 2, cy + h); }
        else if (kind == 2)  { p.startNewSubPath (cx - w / 2, cy + h); p.lineTo (cx - w / 2, cy - h); p.lineTo (cx, cy - h); p.lineTo (cx, cy + h); p.lineTo (cx + w / 2, cy + h); }
        else                 { p.startNewSubPath (cx - w / 2, cy + h); p.lineTo (cx, cy - h); p.lineTo (cx + w / 2, cy + h); }
        g.strokePath (p, juce::PathStrokeType (1.4f));
    };
    wave (0, 440.0f, kRowY[0] - 64.0f); wave (1, 565.0f, kRowY[0] - 64.0f);
    for (int i = 1; i < 3; ++i) { wave (2, 440.0f, kRowY[i] - 64.0f); wave (3, 565.0f, kRowY[i] - 64.0f); }

    // filtro
    ui::textAt (g, "LowPass", 990.0f, kRowY[0] + 58.0f, 9.5f);
    ui::textAt (g, "Osc.", 1082.0f, kRowY[0] + 58.0f, 9.5f);
    // inviluppo
    ui::textAt (g, "Slow", 705.0f, kRowY[1] + 60.0f, 9.5f);
    ui::textAt (g, "Long", 845.0f, kRowY[1] + 60.0f, 9.5f);
    ui::textAt (g, "Slow", 1040.0f, kRowY[1] + 60.0f, 9.5f);
    ui::textAt (g, "Manual", 1135.0f, kRowY[1] + 60.0f, 9.5f);
    ui::textAt (g, "LEVEL", 1065.0f, kRowY[2] - 80.0f, 13.0f);
    // riverbero
    ui::textAt (g, "Dry", 662.0f, kRowY[2] + 58.0f, 9.5f);
    ui::textAt (g, "Reverb", 762.0f, kRowY[2] + 58.0f, 9.5f);
    // rumore
    ui::textAt (g, "Low", 82.0f, kRowY[3] + 58.0f, 9.5f);
    ui::textAt (g, "High", 170.0f, kRowY[3] + 58.0f, 9.5f);
    // filtri d'uscita e muting
    ui::textAt (g, "Low", 676.0f, 742.0f, 9.5f); ui::textAt (g, "High", 756.0f, 742.0f, 9.5f);
    ui::textAt (g, "Low", 811.0f, 742.0f, 9.5f); ui::textAt (g, "High", 891.0f, 742.0f, 9.5f);
    ui::textAt (g, "MUTE", 755.0f, 774.0f, 9.5f, true);
    ui::textAt (g, "MUTE", 890.0f, 774.0f, 9.5f, true);
    // strumento
    ui::tag (g, "A", 1042.0f, 772.0f, 11.0f, true);
    g.setColour (t.ink);
    ui::textAt (g, "CONTROL", 1170.0f, 628.0f, 8.5f); ui::textAt (g, "VOLTAGES", 1170.0f, 640.0f, 8.5f);
    ui::textAt (g, "SIGNAL", 1170.0f, 740.0f, 8.5f);  ui::textAt (g, "LEVELS", 1170.0f, 752.0f, 8.5f);
}

void PanelContent::drawLowerPanel (juce::Graphics& g)
{
    const auto& t = ui::theme();
    const auto mb = kMatrixBounds.toFloat();
    const float cw = mb.getWidth() / 16.0f, ch = mb.getHeight() / 16.0f;
    g.setColour (t.ink);

    // righe: sorgenti e trattamenti
    static const char* rows[16] = { "Oscillator 1  sine", "ramp", "Oscillator 2  square", "triangle", "Oscillator 3  square", "triangle",
                                    "Noise", "Input  Channel 1", "Channel 2", "Filter", "Trapezoid", "Envelope Signal", "Ring Modulator",
                                    "Reverberation", "Stick  horizontal", "vertical" };
    for (int r = 0; r < 16; ++r)
    {
        const float y = mb.getY() + (r + 0.5f) * ch;
        ui::textAt (g, rows[r], mb.getX() - 12.0f, y, 11.5f, false, juce::Justification::centredRight, 200.0f);
        juce::Path tri; tri.addTriangle (mb.getX() - 9.0f, y - 4.0f, mb.getX() - 9.0f, y + 4.0f, mb.getX() - 2.0f, y);
        g.fillPath (tri);
        ui::tag (g, juce::String (r + 1), mb.getRight() + 16.0f, y, 10.5f);
        g.setColour (t.ink);
    }
    g.drawLine (60.0f, mb.getY(), 60.0f, mb.getY() + 10.0f * ch, 1.2f);
    g.drawLine (48.0f, mb.getY() + 9.0f * ch, 48.0f, mb.getY() + 14.0f * ch, 1.2f);
    {
        juce::Graphics::ScopedSaveState save (g);
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, 52.0f, mb.getY() + 4.5f * ch));
        ui::textAt (g, "SOURCES", 52.0f, mb.getY() + 4.5f * ch, 10.0f, true);
    }
    {
        juce::Graphics::ScopedSaveState save (g);
        g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, 40.0f, mb.getY() + 11.5f * ch));
        ui::textAt (g, "TREATMENTS", 40.0f, mb.getY() + 11.5f * ch, 10.0f, true);
    }

    // colonne: ingressi di segnale e di controllo
    static const char* cols[16] = { "Meter", "Output Ch.1", "Output Ch.2", "Envelope", "Ring Mod. A", "Ring Mod. B", "Reverb", "Filter",
                                    "Osc. Freq. 1", "Osc. Freq. 2", "Osc. Freq. 3", "Decay", "Reverb Mix", "Filter Frq.", "Out Level 1", "Out Level 2" };
    for (int c = 0; c < 16; ++c)
    {
        const float x = mb.getX() + (c + 0.5f) * cw;
        {
            juce::Graphics::ScopedSaveState save (g);
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi, x, mb.getY() - 10.0f));
            ui::textAt (g, cols[c], x, mb.getY() - 10.0f, 10.5f, false, juce::Justification::centredLeft, 90.0f);
        }
        ui::tag (g, juce::String::charToString ((juce::juce_wchar) ('A' + c)), x, mb.getBottom() + 14.0f, 10.5f, c < 8);
        g.setColour (t.ink);
    }
    ui::textAt (g, "SIGNAL INPUTS", mb.getX() + 4.0f * cw, 812.0f, 10.5f, true);
    ui::textAt (g, "CONTROL INPUTS", mb.getX() + 12.0f * cw, 812.0f, 10.5f, true);
    g.drawLine (mb.getX() + 2.0f, 820.0f, mb.getX() + 8.0f * cw - 4.0f, 820.0f, 1.0f);
    g.drawLine (mb.getX() + 8.0f * cw + 4.0f, 820.0f, mb.getRight() - 2.0f, 820.0f, 1.0f);

    // targhetta (apre il menu)
    const auto np = kNameplate.toFloat();
    g.setColour (juce::Colour (0xfff4f3ee));
    g.fillRect (np);
    g.setColour (t.ink);
    g.drawRect (np, 1.6f);
    g.drawLine (np.getX(), np.getCentreY() + 3.0f, np.getRight(), np.getCentreY() + 3.0f, 1.0f);
    ui::textAt (g, "PSM-3  (PsychoMatrix-3)", np.getCentreX(), np.getY() + 13.0f, 15.0f, true, juce::Justification::centred, np.getWidth());
    ui::textAt (g, "Collateral Effects", np.getCentreX(), np.getBottom() - 10.0f, 11.5f, false, juce::Justification::centred, np.getWidth());
    ui::textAt (g, "Preset: " + shownPreset, np.getRight() + 14.0f, np.getCentreY(), 11.5f, false, juce::Justification::centredLeft, 230.0f);

    // lato destro
    ui::textAt (g, "RANGE", 880.0f, 826.0f, 12.0f);
    ui::arrow (g, { 918.0f, 826.0f }, { 944.0f, 826.0f }); ui::arrow (g, { 944.0f, 826.0f }, { 918.0f, 826.0f });
    ui::textAt (g, "RANGE", 825.0f, 1000.0f, 12.0f);
    ui::arrow (g, { 868.0f, 990.0f }, { 868.0f, 1012.0f }); ui::arrow (g, { 868.0f, 1012.0f }, { 868.0f, 990.0f });
    ui::textAt (g, "MAINS", 1085.0f, 846.0f, 12.0f);
    ui::textAt (g, "ON", 1085.0f, 924.0f, 11.0f);
    ui::textAt (g, "ATTACK", 1140.0f, 1020.0f, 12.0f);
    ui::titleBar (g, "OUTPUT  CH.1", 800.0f, 1196.0f);
    ui::titleBar (g, "OUTPUT  CH.2", 1020.0f, 1196.0f);
    ui::tag (g, "B", 935.0f, 1207.0f, 11.0f, true);
    ui::tag (g, "C", 1155.0f, 1207.0f, 11.0f, true);
    g.setColour (t.ink);
    ui::textAt (g, "Left", 928.0f, 1354.0f, 9.5f); ui::textAt (g, "Right", 994.0f, 1354.0f, 9.5f);
    ui::textAt (g, "Left", 1028.0f, 1354.0f, 9.5f); ui::textAt (g, "Right", 1094.0f, 1354.0f, 9.5f);
}

void PanelContent::drawKeyboardPanel (juce::Graphics& g)
{
    const auto& t = ui::theme();
    g.setColour (t.ink);
    g.drawRect (juce::Rectangle<float> (kLeft + 8.0f, kKbTop + 8.0f, 296.0f, kKbBottom - kKbTop - 16.0f), 1.4f);
    ui::textAt (g, "KEYBOARD OSCILLATOR", 134.0f, kKbTop + 22.0f, 10.5f, true);
    ui::textAt (g, "SPREAD", 180.0f, 1548.0f, 9.5f);
    ui::textAt (g, "RANGE", 180.0f, 1660.0f, 9.5f);
    ui::textAt (g, "INP. CH.1", 275.0f, 1456.0f, 9.5f, true);
    ui::textAt (g, "kbd voltage", 275.0f, 1470.0f, 8.5f);
    ui::textAt (g, "signal", 275.0f, 1532.0f, 8.5f);
    ui::textAt (g, "INP. CH.2", 275.0f, 1568.0f, 9.5f, true);
    ui::textAt (g, "dynamic v.", 275.0f, 1582.0f, 8.5f);
    ui::textAt (g, "kbd voltage", 275.0f, 1644.0f, 8.5f);
    ui::textAt (g, "DK1", 300.0f, kKbBottom - 18.0f, 10.0f, true);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRect (juce::Rectangle<float> (341.0f, 1428.0f, 863.0f, 256.0f));
}

//==============================================================================
PSM3Editor::PSM3Editor (PSM3Processor& p) : AudioProcessorEditor (p), proc (p), content (p)
{
    addAndMakeVisible (content);
    setResizable (true, true);
    const double ratio = (double) PanelContent::baseW / (double) PanelContent::baseH;
    setResizeLimits (PanelContent::baseW * 2 / 5, PanelContent::baseH * 2 / 5, PanelContent::baseW * 2, PanelContent::baseH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio (ratio);

    // Scala iniziale: quella salvata, altrimenti quanto basta a stare nello schermo.
    float scale = proc.getUiScale();
    if (scale <= 0.0f)
    {
        scale = 0.6f;
        if (const auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
            scale = (float) display->userArea.getHeight() * 0.86f / (float) PanelContent::baseH;
    }
    scale = juce::jlimit (0.4f, 2.0f, scale);
    setSize (juce::roundToInt (PanelContent::baseW * scale), juce::roundToInt (PanelContent::baseH * scale));
    startTimerHz (30);
}

void PSM3Editor::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }

void PSM3Editor::resized()
{
    const float s = (float) getWidth() / (float) PanelContent::baseW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, PanelContent::baseW, PanelContent::baseH);
    proc.setUiScale (s);
}

void PSM3Editor::timerCallback() { content.refresh(); }
