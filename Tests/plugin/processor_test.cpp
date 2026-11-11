// Test del plugin completo (processore JUCE): parametri, note MIDI, stato, MIDI Learn, preset.
#include <juce_audio_processors/juce_audio_processors.h>
#include "Plugin/PluginProcessor.h"
#include <cstdio>
#include <thread>

static int fails = 0;
static void check (bool ok, const char* what) { std::printf ("%s  %s\n", ok ? "ok " : "ERR", what); if (! ok) ++fails; }

static double run (PSM3Processor& p, int blocks, juce::MidiBuffer* first = nullptr)
{
    juce::AudioBuffer<float> buf (2, 512); double sum = 0.0; long n = 0;
    for (int b = 0; b < blocks; ++b)
    {
        buf.clear();
        juce::MidiBuffer m; if (first != nullptr && b == 0) m = *first;
        p.processBlock (buf, m);
        for (int i = 0; i < 512; ++i) { const double v = buf.getSample (0, i); sum += v * v; ++n; }
    }
    return std::sqrt (sum / (double) n);
}
static void send (PSM3Processor& p, const juce::MidiMessage& msg) { juce::MidiBuffer m; m.addEvent (msg, 0); run (p, 1, &m); }
static float value (PSM3Processor& p, psm3::ParamId id) { return p.apvts.getRawParameterValue (psm3::paramInfo (id).key)->load(); }

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    using psm3::ParamId;
    const double fs = 48000.0;

    { // params_complete
        PSM3Processor p;
        bool all = true;
        for (int i = 0; i < PSM3Processor::kNumParams; ++i) all = all && p.apvts.getParameter (psm3::paramInfo ((ParamId) i).key) != nullptr;
        check (all, "tutti i parametri di Params.h esistono");
        check ((int) p.getParameters().size() == PSM3Processor::kNumParams, "nessun parametro in piu'");
        check (p.getCurrentPresetName() == "Tastiera base", "all'avvio e' caricato 'Tastiera base'");
    }
    { // plays
        PSM3Processor p; p.setPlayConfigDetails (0, 2, fs, 512); p.prepareToPlay (fs, 512);
        check (run (p, 20) < 1.0e-3, "senza note: silenzio");
        juce::MidiBuffer on; on.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0);
        check (run (p, 10, &on) > 0.01, "nota 60: suona entro 100 ms");
        juce::MidiBuffer off; off.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
        run (p, 190, &off);
        check (run (p, 10) < 1.0e-3, "dopo il rilascio e 2 s: silenzio");
        // tastiera a schermo
        p.keyboardState.noteOn (1, 64, 0.8f);
        check (run (p, 10) > 0.01, "tastiera a schermo: suona");
        p.keyboardState.noteOff (1, 64, 0.0f); run (p, 190);
        check (run (p, 10) < 1.0e-3, "tastiera a schermo: rilascio");
    }
    { // state_roundtrip
        PSM3Processor a; a.setPlayConfigDetails (0, 2, fs, 512); a.prepareToPlay (fs, 512);
        const ParamId ids[] = { ParamId::osc1Freq, ParamId::osc2Shape, ParamId::filtResponse, ParamId::envDecay, ParamId::revMix,
                                ParamId::out1Pan, ParamId::kbTuning, ParamId::speaker1, ParamId::meterMode, ParamId::noiseColour };
        for (int i = 0; i < 10; ++i) a.setParamValue ((int) ids[i], psm3::paramInfo (ids[i]).isSwitch ? 1.0 : 1.0 + 0.7 * i);
        for (int i = 0; i < 12; ++i) a.setPin (i, 15 - i, (psm3::Pin) (1 + i % 3));
        a.assignSource (74, (int) ParamId::filtFreq);

        std::atomic<bool> stop { false };
        std::thread audio ([&] { while (! stop) run (a, 1); });
        juce::MemoryBlock mb; a.getStateInformation (mb);
        PSM3Processor b; b.setPlayConfigDetails (0, 2, fs, 512); b.prepareToPlay (fs, 512);
        std::thread audio2 ([&] { while (! stop) run (b, 1); });
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        stop = true; audio.join(); audio2.join();

        bool same = true;
        for (int i = 0; i < PSM3Processor::kNumParams; ++i)
            same = same && std::abs (value (a, (ParamId) i) - value (b, (ParamId) i)) < 2.0e-3f;
        check (same, "stato: parametri identici dopo salvataggio e ripristino");
        check (a.getPins() == b.getPins(), "stato: pin identici");
        check (b.getSourceForParam ((int) ParamId::filtFreq) == 74, "stato: mappa MIDI ripristinata");
    }
    { // state_garbage
        PSM3Processor p;
        p.setParamValue ((int) ParamId::osc1Freq, 3.21); p.setPin (2, 3, psm3::Pin::Red);
        const auto pinsBefore = p.getPins();
        juce::MemoryBlock good; p.getStateInformation (good);

        p.setStateInformation (nullptr, 0);
        p.setStateInformation (good.getData(), 0);
        juce::Random rnd (7); juce::MemoryBlock junk (4096); for (size_t i = 0; i < junk.getSize(); ++i) junk[i] = (char) rnd.nextInt (256);
        p.setStateInformation (junk.getData(), (int) junk.getSize());
        p.setStateInformation (good.getData(), (int) good.getSize() / 2);          // XML troncato
        check (std::abs (value (p, ParamId::osc1Freq) - 3.21f) < 2.0e-3f && p.getPins() == pinsBefore, "stato corrotto: nulla cambia");

        juce::ValueTree future ("PSM3");
        future.setProperty ("version", 99, nullptr);
        future.setProperty ("pins", "troppo corta", nullptr);
        future.setProperty ("qualcosaDiNuovo", 42, nullptr);
        juce::ValueTree values ("PARAMS"); values.setProperty ("osc1Freq", 7.5, nullptr); values.setProperty ("parametroFuturo", 1.0, nullptr);
        values.setProperty ("filtFreq", 1.0e9, nullptr);
        future.appendChild (values, nullptr); future.appendChild (juce::ValueTree ("BLOCCO_SCONOSCIUTO"), nullptr);
        check (p.applyState (future, true), "stato di versione futura: accettato");
        check (std::abs (value (p, ParamId::osc1Freq) - 7.5f) < 2.0e-3f, "versione futura: applicati i campi riconosciuti");
        check (value (p, ParamId::filtFreq) <= 10.0f, "versione futura: valori fuori scala limitati");
        check (p.getPins() == pinsBefore, "versione futura: pin non validi ignorati");
        check (! p.applyState (juce::ValueTree ("ALTRO"), true), "stato di un altro plugin: rifiutato");
    }
    { // midi_defaults e MIDI Learn
        PSM3Processor p; p.setPlayConfigDetails (0, 2, fs, 512); p.prepareToPlay (fs, 512);
        p.resetDefaultMappings();
        send (p, juce::MidiMessage::controllerEvent (1, 1, 127));
        check (value (p, ParamId::joyY) > 9.99f, "CC1 = 127: joystick verticale a 10");
        send (p, juce::MidiMessage::pitchWheel (1, 8192));
        check (std::abs (value (p, ParamId::joyX) - 5.0f) < 0.01f, "Pitch Bend al centro: joystick orizzontale a 5");
        send (p, juce::MidiMessage::controllerEvent (1, 64, 127));
        check (value (p, ParamId::attackButton) > 0.5f, "CC64 premuto: pulsante Attack");
        send (p, juce::MidiMessage::controllerEvent (1, 64, 0));
        check (value (p, ParamId::attackButton) < 0.5f, "CC64 rilasciato");

        p.startLearn ((int) ParamId::filtFreq);
        send (p, juce::MidiMessage::controllerEvent (1, 74, 127));
        check (p.getLearningParam() == -1 && p.getSourceForParam ((int) ParamId::filtFreq) == 74, "MIDI Learn: CC74 assegnato a Filter Frequency");
        check (value (p, ParamId::filtFreq) > 9.99f, "MIDI Learn: il primo valore viene gia' applicato");
        p.startLearn ((int) ParamId::revMix);
        send (p, juce::MidiMessage::controllerEvent (1, 74, 0));
        check (p.getSourceForParam ((int) ParamId::filtFreq) == -1 && p.getSourceForParam ((int) ParamId::revMix) == 74, "un CC comanda un solo parametro");
        p.clearMappingForParam ((int) ParamId::revMix);
        check (p.getSourceForParam ((int) ParamId::revMix) == -1, "assegnazione rimossa");

        juce::MidiBuffer on; on.addEvent (juce::MidiMessage::noteOn (1, 60, 0.8f), 0); run (p, 10, &on);
        send (p, juce::MidiMessage::allNotesOff (1)); run (p, 190);
        check (run (p, 10) < 1.0e-3, "All Notes Off: nessuna nota bloccata");
    }
    { // preset
        PSM3Processor p; p.setPlayConfigDetails (0, 2, fs, 512); p.prepareToPlay (fs, 512);
        bool ok = true;
        for (int i = 0; i < PSM3Processor::getNumFactoryPresets(); ++i)
        {
            p.loadFactoryPreset (i);
            ok = ok && p.getCurrentPresetName() == PSM3Processor::getFactoryPresetName (i) && std::isfinite (run (p, 20));
        }
        check (ok, "tutti i preset di fabbrica si caricano");
        check (value (p, ParamId::mains) > 0.5f, "i preset non spengono lo strumento");

        p.loadFactoryPreset (2);
        const auto pins = p.getPins(); const float f = value (p, ParamId::filtFreq);
        const auto xml = p.captureState (false).createXml();
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("psm3_test_preset.psm3");
        check (xml != nullptr && xml->writeTo (file), "preset utente: scritto");
        p.loadFactoryPreset (0);
        check (p.loadUserPreset (file) && p.getPins() == pins && std::abs (value (p, ParamId::filtFreq) - f) < 2.0e-3f, "preset utente: riletto identico");
        file.replaceWithText ("questo non e' un preset");
        check (! p.loadUserPreset (file) && p.getPins() == pins, "preset utente corrotto: rifiutato senza danni");
        file.deleteFile();
    }

    std::printf (fails == 0 ? "\nTUTTO OK\n" : "\n%d ERRORI\n", fails);
    return fails == 0 ? 0 : 1;
}
