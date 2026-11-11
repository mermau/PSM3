// Salva l'interfaccia del plugin in PNG (uso: gui_snapshot out.png [scala] [indice preset])
// e controlla che ogni comando sia dentro la finestra.
#include <juce_gui_basics/juce_gui_basics.h>
#include "Plugin/PluginProcessor.h"
#include "GUI/PluginEditor.h"
#include <cstdio>
#include <set>
#include <typeinfo>

static int checkChildren (juce::Component& parent, int depth = 0)
{
    int bad = 0;
    for (auto* c : parent.getChildren())
    {
        if (! c->isVisible()) continue;
        const auto b = c->getBounds();
        if (b.isEmpty() || ! parent.getLocalBounds().contains (b))
        {
            std::printf ("ERR  componente fuori posto: %s  %s\n", typeid (*c).name(), b.toString().toRawUTF8());
            ++bad;
        }
        if (depth < 2) bad += checkChildren (*c, depth + 1);
    }
    return bad;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 2) { std::printf ("uso: gui_snapshot out.png [scala] [preset]\n"); return 2; }
    PSM3Processor proc;
    if (argc > 3) proc.loadFactoryPreset (std::atoi (argv[3]));
    std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
    const float scale = argc > 2 ? (float) std::atof (argv[2]) : 1.0f;
    ed->setSize (juce::roundToInt (PanelContent::baseW * scale), juce::roundToInt (PanelContent::baseH * scale));

    int bad = 0;
    for (auto* c : ed->getChildren())
        if (auto* content = dynamic_cast<PanelContent*> (c)) bad += checkChildren (*content);

    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    out.deleteFile();
    { juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os); }

    std::set<juce::uint32> colours;
    for (int y = 0; y < img.getHeight(); y += 3)
        for (int x = 0; x < img.getWidth(); x += 3) colours.insert (img.getPixelAt (x, y).getARGB());
    std::printf ("%d x %d, %d colori distinti, %d componenti fuori posto\n", img.getWidth(), img.getHeight(), (int) colours.size(), bad);
    if (colours.size() < 2000) { std::printf ("ERR  immagine troppo povera: l'interfaccia non e' stata disegnata\n"); ++bad; }
    return bad == 0 ? 0 : 1;
}
