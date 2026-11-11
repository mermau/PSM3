// PSM-3 — colori, caratteri incorporati e piccoli strumenti di disegno.
// Carattere: TeX Gyre Adventor (GUST Font License, vedi resources/fonts/GUST-FONT-LICENSE.txt)
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace ui
{
struct Theme
{
    juce::Colour panel      { 0xffdcdcd8 };   // alluminio verniciato grigio chiaro
    juce::Colour panelShade { 0xffc9c9c4 };
    juce::Colour ink        { 0xff161616 };   // serigrafia nera
    juce::Colour woodLight  { 0xff9a5f38 };
    juce::Colour woodDark   { 0xff5d341c };
    juce::Colour board      { 0xff141414 };   // piastra della matrice
    juce::Colour skirt      { 0xff1b1b1b };   // corpo delle manopole
    juce::Colour capBlue    { 0xff2d6fd2 };
    juce::Colour capGreen   { 0xff2aa860 };
    juce::Colour capWhite   { 0xfff1f1ec };
    juce::Colour capRed     { 0xffd3302b };
    juce::Colour capYellow  { 0xffe7c32f };
    juce::Colour pinWhite   { 0xfff3f1e8 };
    juce::Colour pinRed     { 0xffd92d27 };
    juce::Colour pinGreen   { 0xff2fae5c };
    juce::Colour learn      { 0xffff8a1e };   // evidenzia il comando in attesa di MIDI Learn
    juce::Typeface::Ptr regular, bold;

    Theme()
    {
        regular = juce::Typeface::createSystemTypefaceFor (BinaryData::texgyreadventorregular_otf,
                                                           BinaryData::texgyreadventorregular_otfSize);
        bold    = juce::Typeface::createSystemTypefaceFor (BinaryData::texgyreadventorbold_otf,
                                                           BinaryData::texgyreadventorbold_otfSize);
    }
};

inline const Theme& theme() { static const Theme t; return t; }

inline juce::Font font (float size, bool bold = false)
{
    const auto& t = theme();
    auto tf = bold ? t.bold : t.regular;
    if (tf == nullptr) return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
    return juce::Font (juce::FontOptions (tf).withHeight (size));
}

// Testo centrato su un punto.
inline void textAt (juce::Graphics& g, const juce::String& text, float cx, float cy, float size,
                    bool bold = false, juce::Justification just = juce::Justification::centred, float width = 260.0f)
{
    g.setFont (font (size, bold));
    juce::Rectangle<float> r (width, size * 1.5f);
    if (just.testFlags (juce::Justification::left))        r.setPosition (cx, cy - r.getHeight() * 0.5f);
    else if (just.testFlags (juce::Justification::right))  r.setPosition (cx - width, cy - r.getHeight() * 0.5f);
    else                                                   r.setCentre (cx, cy);
    g.drawText (text, r, just, false);
}

// Etichetta bianca su fondo nero (titoli di sezione e numeri/lettere della matrice).
inline void tag (juce::Graphics& g, const juce::String& text, float cx, float cy, float size, bool round = false)
{
    const auto f = font (size, true);
    const float w = juce::jmax (size * 1.25f, juce::GlyphArrangement::getStringWidth (f, text) + size * 0.7f), h = size * 1.3f;
    juce::Rectangle<float> r (w, h); r.setCentre (cx, cy);
    g.setColour (theme().ink);
    if (round) g.fillRoundedRectangle (r, h * 0.5f); else g.fillRect (r);
    g.setColour (juce::Colours::white);
    g.setFont (f);
    g.drawText (text, r, juce::Justification::centred, false);
}

inline void titleBar (juce::Graphics& g, const juce::String& text, float x, float y, float size = 15.0f)
{
    const auto f = font (size, true);
    const float w = juce::GlyphArrangement::getStringWidth (f, text) + 18.0f, h = size * 1.45f;
    g.setColour (theme().ink);
    g.fillRect (juce::Rectangle<float> (x, y, w, h));
    g.setColour (juce::Colours::white);
    g.setFont (f);
    g.drawText (text, juce::Rectangle<float> (x, y, w, h), juce::Justification::centred, false);
}

// Freccia sottile da un punto a un altro (le frecce "Low / High" dell'originale).
inline void arrow (juce::Graphics& g, juce::Point<float> from, juce::Point<float> to, float thickness = 1.3f)
{
    g.drawArrow (juce::Line<float> (from, to), thickness, 6.0f, 7.0f);
}
}
