#include "Widgets.h"

namespace ui
{
static constexpr float kPi = juce::MathConstants<float>::pi;
// Corsa delle manopole: 300 gradi, da -150 a +150 rispetto alle ore 12.
static float angleFor (float value0to10) { return juce::degreesToRadians (-150.0f + 30.0f * juce::jlimit (0.0f, 10.0f, value0to10)); }
static juce::Point<float> polar (juce::Point<float> c, float r, float angle) { return { c.x + r * std::sin (angle), c.y - r * std::cos (angle) }; }

static juce::Colour capColour (Cap c)
{
    const auto& t = theme();
    switch (c)
    {
        case Cap::Blue:   return t.capBlue;
        case Cap::Green:  return t.capGreen;
        case Cap::White:  return t.capWhite;
        case Cap::Red:    return t.capRed;
        case Cap::Yellow: return t.capYellow;
    }
    return t.capWhite;
}

//==============================================================================
ParamWidget::ParamWidget (PSM3Processor& p, psm3::ParamId id)
    : proc (p), index ((int) id),
      attachment (*p.getParam ((int) id), [this] (float v) { value = v; repaint(); }, nullptr)
{
    defaultValue = (float) psm3::paramInfo (id).def;
    setTooltip (psm3::paramInfo (id).name);
    attachment.sendInitialUpdate();
}

void ParamWidget::beginDrag() { if (! dragging) { dragging = true; attachment.beginGesture(); } }
void ParamWidget::dragTo (float v) { attachment.setValueAsPartOfGesture (v); }
void ParamWidget::endDrag() { if (dragging) { dragging = false; attachment.endGesture(); } }
void ParamWidget::setComplete (float v) { attachment.setValueAsCompleteGesture (v); }

//==============================================================================
Knob::Knob (PSM3Processor& p, psm3::ParamId id, Cap c, bool v) : ParamWidget (p, id), cap (c), vernier (v) {}

void Knob::paint (juce::Graphics& g)
{
    const auto& t = theme();
    const auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float full = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const float a = angleFor (value);

    if (vernier)
    {
        // Quadrante a demoltiplica: corona metallica graduata che ruota, indice fisso in alto.
        const float rDial = full * 0.98f, rSkirt = full * 0.56f;
        juce::ColourGradient metal (juce::Colour (0xfff4f4f2), c.x - rDial * 0.5f, c.y - rDial, juce::Colour (0xff8d8d8a), c.x + rDial * 0.6f, c.y + rDial, false);
        g.setGradientFill (metal);
        g.fillEllipse (juce::Rectangle<float> (rDial * 2.0f, rDial * 2.0f).withCentre (c));
        g.setColour (juce::Colour (0xff5a5a58));
        g.drawEllipse (juce::Rectangle<float> (rDial * 2.0f, rDial * 2.0f).withCentre (c), 1.2f);
        g.setColour (t.ink);
        for (int i = 0; i <= 100; ++i)
        {
            const float ta = juce::degreesToRadians (3.0f * (float) i) - (a + juce::degreesToRadians (150.0f));
            const bool major = i % 10 == 0, mid = i % 5 == 0;
            g.drawLine (juce::Line<float> (polar (c, rDial * (major ? 0.74f : (mid ? 0.80f : 0.85f)), ta), polar (c, rDial * 0.93f, ta)), major ? 1.4f : 0.8f);
            if (major)
            {
                const auto pt = polar (c, rDial * 0.655f, ta);
                g.setFont (font (rDial * 0.17f, true));
                g.drawText (juce::String (i / 10), juce::Rectangle<float> (20.0f, 14.0f).withCentre (pt), juce::Justification::centred, false);
            }
        }
        // indice
        juce::Path idx; idx.addTriangle (c.x - 6.0f, c.y - rDial - 7.0f, c.x + 6.0f, c.y - rDial - 7.0f, c.x, c.y - rDial + 5.0f);
        g.setColour (t.ink); g.fillPath (idx);
        // manopola centrale
        g.setColour (t.skirt);
        g.fillEllipse (juce::Rectangle<float> (rSkirt * 2.0f, rSkirt * 2.0f).withCentre (c));
        const float rCap = rSkirt * 0.74f;
        juce::ColourGradient cg (capColour (cap).brighter (0.35f), c.x - rCap * 0.4f, c.y - rCap * 0.6f, capColour (cap).darker (0.25f), c.x + rCap * 0.5f, c.y + rCap * 0.8f, true);
        g.setGradientFill (cg);
        g.fillEllipse (juce::Rectangle<float> (rCap * 2.0f, rCap * 2.0f).withCentre (c));
        if (dragging)
        {
            // valore in bianco al centro del cappuccio blu, ben leggibile mentre si gira
            g.setColour (juce::Colours::white);
            g.setFont (font (rCap * 0.62f, true));
            g.drawText (juce::String (value, 2), juce::Rectangle<float> (rCap * 2.0f, rCap).withCentre (c), juce::Justification::centred, false);
        }
    }
    else
    {
        // scala 0...10 serigrafata
        const float rScale = full * 0.98f, rSkirt = full * 0.60f;
        g.setColour (t.ink);
        g.setFont (font (full * 0.24f));
        for (int i = 0; i <= 10; ++i)
        {
            const float ta = angleFor ((float) i);
            g.drawLine (juce::Line<float> (polar (c, rSkirt * 1.06f, ta), polar (c, rSkirt * 1.22f, ta)), 1.1f);
            g.drawText (juce::String (i), juce::Rectangle<float> (22.0f, 14.0f).withCentre (polar (c, rScale * 0.88f, ta)), juce::Justification::centred, false);
        }
        // gonna zigrinata
        juce::Path skirt;
        const int flutes = 20;
        for (int i = 0; i < flutes * 2; ++i)
        {
            const float ta = a + kPi * (float) i / (float) flutes;
            const auto pt = polar (c, rSkirt * (i % 2 == 0 ? 1.0f : 0.94f), ta);
            if (i == 0) skirt.startNewSubPath (pt); else skirt.lineTo (pt);
        }
        skirt.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillEllipse (juce::Rectangle<float> (rSkirt * 2.1f, rSkirt * 2.1f).withCentre ({ c.x + 1.5f, c.y + 2.5f }));
        g.setColour (t.skirt);
        g.fillPath (skirt);
        // indice bianco sulla gonna
        g.setColour (juce::Colours::white);
        g.drawLine (juce::Line<float> (polar (c, rSkirt * 0.70f, a), polar (c, rSkirt * 0.96f, a)), 2.2f);
        // cappuccio
        const float rCap = rSkirt * 0.62f;
        juce::ColourGradient cg (capColour (cap).brighter (0.4f), c.x - rCap * 0.4f, c.y - rCap * 0.6f, capColour (cap).darker (0.25f), c.x + rCap * 0.5f, c.y + rCap * 0.8f, true);
        g.setGradientFill (cg);
        g.fillEllipse (juce::Rectangle<float> (rCap * 2.0f, rCap * 2.0f).withCentre (c));
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawEllipse (juce::Rectangle<float> (rCap * 2.0f, rCap * 2.0f).withCentre (c), 1.0f);
    }

    if (learning)
    {
        g.setColour (t.learn);
        g.drawEllipse (b.reduced (2.0f), 3.0f);
    }
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { if (onContextMenu) onContextMenu (*this, paramIndex()); return; }
    dragStart = value;
    beginDrag();
    repaint();
}

void Knob::mouseDrag (const juce::MouseEvent& e)
{
    if (! dragging) return;
    // corsa completa in 220 pixel; il quadrante a demoltiplica e Shift sono piu' fini
    float perPixel = 10.0f / 220.0f;
    if (vernier) perPixel *= 0.3f;
    if (e.mods.isShiftDown()) perPixel *= 0.1f;
    const float delta = (float) (e.getDistanceFromDragStartX() - e.getDistanceFromDragStartY()) * perPixel;
    dragTo (juce::jlimit (0.0f, 10.0f, dragStart + delta));
}

void Knob::mouseUp (const juce::MouseEvent&) { endDrag(); repaint(); }

void Knob::mouseDoubleClick (const juce::MouseEvent& e) { if (! e.mods.isPopupMenu()) setComplete (defaultValue); }

void Knob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w)
{
    const float step = (vernier ? 0.6f : 2.0f) * (e.mods.isShiftDown() ? 0.1f : 1.0f);
    setComplete (juce::jlimit (0.0f, 10.0f, value + w.deltaY * step));
}

//==============================================================================
Toggle::Toggle (PSM3Processor& p, psm3::ParamId id, Style s, bool inv) : ParamWidget (p, id), style (s), invert (inv) {}

void Toggle::paint (juce::Graphics& g)
{
    const auto& t = theme();
    const auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const bool on = value >= 0.5f;
    if (style == Style::Button)
    {
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.42f;
        juce::ColourGradient ring (juce::Colour (0xfff0f0ee), c.x - r, c.y - r, juce::Colour (0xff7e7e7c), c.x + r, c.y + r, false);
        g.setGradientFill (ring);
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
        const float rb = r * (on ? 0.56f : 0.62f);
        g.setColour (on ? t.capRed.darker (0.3f) : t.capRed);
        g.fillEllipse (juce::Rectangle<float> (rb * 2.0f, rb * 2.0f).withCentre (c));
        g.setColour (juce::Colours::white.withAlpha (on ? 0.1f : 0.35f));
        g.fillEllipse (juce::Rectangle<float> (rb * 0.7f, rb * 0.5f).withCentre ({ c.x - rb * 0.25f, c.y - rb * 0.35f }));
    }
    else
    {
        // Ghiera cromata e levetta vista di fronte: e' inclinata di poco, quindi appare corta.
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.24f;
        juce::ColourGradient ring (juce::Colour (0xfff2f2f0), c.x - r, c.y - r, juce::Colour (0xff6f6f6d), c.x + r, c.y + r, false);
        g.setGradientFill (ring);
        g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
        g.setColour (juce::Colour (0xff3a3a3a));
        g.fillEllipse (juce::Rectangle<float> (r * 1.1f, r * 1.1f).withCentre (c));
        const bool up = invert ? ! on : on;
        const float len = r * 0.95f;                                  // sporgenza apparente: circa 20 gradi di inclinazione
        const juce::Point<float> tip (c.x, c.y + (up ? -len : len));
        juce::Path bat;                                               // gambo che si allarga verso la punta
        const float w0 = r * 0.34f, w1 = r * 0.50f;
        bat.startNewSubPath (c.x - w0, c.y); bat.lineTo (tip.x - w1, tip.y); bat.lineTo (tip.x + w1, tip.y); bat.lineTo (c.x + w0, c.y); bat.closeSubPath();
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillEllipse (juce::Rectangle<float> (w1 * 2.2f, w1 * 1.6f).withCentre ({ tip.x + 1.5f, tip.y + 2.0f }));
        g.setColour (t.capRed.darker (0.25f));
        g.fillPath (bat);
        juce::ColourGradient tipFill (t.capRed.brighter (0.45f), tip.x - w1 * 0.4f, tip.y - w1 * 0.5f, t.capRed.darker (0.2f), tip.x + w1, tip.y + w1, true);
        g.setGradientFill (tipFill);
        g.fillEllipse (juce::Rectangle<float> (w1 * 2.0f, w1 * 1.7f).withCentre (tip));
    }
    if (learning) { g.setColour (t.learn); g.drawRect (b.reduced (1.0f), 2.5f); }
}

void Toggle::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { if (onContextMenu) onContextMenu (*this, paramIndex()); return; }
    if (style == Style::Button) { beginDrag(); dragTo (1.0f); }
    else setComplete (value >= 0.5f ? 0.0f : 1.0f);
}

void Toggle::mouseUp (const juce::MouseEvent&)
{
    if (style == Style::Button && dragging) { dragTo (0.0f); endDrag(); }
}

//==============================================================================
void Lamp::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat().reduced (2.0f);
    const auto c = b.getCentre();
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    if (on)
    {
        g.setColour (juce::Colour (0xffff3020).withAlpha (0.25f));
        g.fillEllipse (juce::Rectangle<float> (r * 2.6f, r * 2.6f).withCentre (c));
    }
    juce::ColourGradient cg (on ? juce::Colour (0xffff9a80) : juce::Colour (0xff8a2a22), c.x - r * 0.3f, c.y - r * 0.4f,
                             on ? juce::Colour (0xffe01a10) : juce::Colour (0xff4a1410), c.x + r, c.y + r, true);
    g.setGradientFill (cg);
    g.fillEllipse (juce::Rectangle<float> (r * 1.7f, r * 1.7f).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (juce::Rectangle<float> (r * 1.7f, r * 1.7f).withCentre (c), 1.0f);
}

//==============================================================================
void MeterView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff2a2a2a));
    g.fillRoundedRectangle (b, 5.0f);
    const auto face = b.reduced (5.0f);
    g.setColour (juce::Colour (0xfff4f1e6));
    g.fillRoundedRectangle (face, 3.0f);

    const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + face.getHeight() * 0.25f);
    const float r = face.getHeight() * 1.02f;
    const float span = juce::degreesToRadians (38.0f);
    g.setColour (theme().ink);
    juce::Path arc; arc.addCentredArc (pivot.x, pivot.y, r, r, 0.0f, -span, span, true);
    g.strokePath (arc, juce::PathStrokeType (1.2f));
    g.setFont (font (9.0f));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = -span + 2.0f * span * (float) i / 10.0f;
        g.drawLine (juce::Line<float> (polar (pivot, r, a), polar (pivot, r + (i % 5 == 0 ? 7.0f : 4.0f), a)), 1.0f);
        if (i % 2 == 0)
            g.drawText (juce::String (i), juce::Rectangle<float> (18.0f, 10.0f).withCentre (polar (pivot, r + 14.0f, a)), juce::Justification::centred, false);
    }
    g.setFont (font (10.0f, true));
    g.drawText ("mA", juce::Rectangle<float> (30.0f, 12.0f).withCentre ({ face.getCentreX(), face.getY() + face.getHeight() * 0.62f }), juce::Justification::centred, false);
    // lancetta
    const float a = -span + 2.0f * span * juce::jlimit (0.0f, 1.0f, needle);
    g.setColour (juce::Colour (0xff101010));
    g.saveState();
    g.reduceClipRegion (face.toNearestInt());
    g.drawLine (juce::Line<float> (pivot, polar (pivot, r + 4.0f, a)), 1.6f);
    g.restoreState();
    g.setColour (juce::Colour (0xff2a2a2a));
    g.fillRect (juce::Rectangle<float> (face.getX(), face.getBottom() - face.getHeight() * 0.16f, face.getWidth(), face.getHeight() * 0.16f));
}

//==============================================================================
JoystickView::JoystickView (PSM3Processor& p)
    : attX (*p.getParam ((int) psm3::ParamId::joyX), [this] (float v) { x = v; repaint(); }, nullptr),
      attY (*p.getParam ((int) psm3::ParamId::joyY), [this] (float v) { y = v; repaint(); }, nullptr)
{
    setTooltip ("Joystick");
    attX.sendInitialUpdate(); attY.sendInitialUpdate();
}

void JoystickView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto c = b.getCentre();
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 2.0f;
    g.setColour (juce::Colour (0xff101010));
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
    g.setColour (juce::Colour (0xff3a3a3a));
    g.drawEllipse (juce::Rectangle<float> (r * 1.86f, r * 1.86f).withCentre (c), 2.0f);
    g.setColour (juce::Colour (0xff050505));
    g.fillEllipse (juce::Rectangle<float> (r * 0.9f, r * 0.9f).withCentre (c));
    // la leva si muove in un quadrato inscritto nel foro tondo
    const float reach = r * 0.52f;
    const juce::Point<float> tip (c.x + (x - 5.0f) / 5.0f * reach, c.y - (y - 5.0f) / 5.0f * reach);
    g.setColour (juce::Colour (0xff9a9a98));
    g.drawLine (juce::Line<float> (c, tip), 7.0f);
    juce::ColourGradient cg (juce::Colour (0xfffafafa), tip.x - 5.0f, tip.y - 6.0f, juce::Colour (0xff80807e), tip.x + 8.0f, tip.y + 9.0f, true);
    g.setGradientFill (cg);
    g.fillEllipse (juce::Rectangle<float> (r * 0.36f, r * 0.36f).withCentre (tip));
    if (learningAxis >= 0)
    {
        g.setColour (theme().learn);
        g.drawEllipse (b.reduced (2.0f), 3.0f);
        g.setFont (font (11.0f, true));
        g.drawText (learningAxis == 0 ? "<->" : "^ v", b.withTop (b.getBottom() - 16.0f), juce::Justification::centred, false);
    }
}

void JoystickView::moveTo (juce::Point<float> p)
{
    const auto c = getLocalBounds().toFloat().getCentre();
    const float reach = (juce::jmin (getWidth(), getHeight()) * 0.5f - 2.0f) * 0.52f;
    attX.setValueAsPartOfGesture (juce::jlimit (0.0f, 10.0f, 5.0f + (p.x - c.x) / reach * 5.0f));
    attY.setValueAsPartOfGesture (juce::jlimit (0.0f, 10.0f, 5.0f - (p.y - c.y) / reach * 5.0f));
}

void JoystickView::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        if (onContextMenu) onContextMenu (*this, (int) psm3::ParamId::joyX);
        return;
    }
    dragging = true;
    attX.beginGesture(); attY.beginGesture();
    moveTo (e.position);
}

void JoystickView::mouseDrag (const juce::MouseEvent& e) { if (dragging) moveTo (e.position); }

void JoystickView::mouseUp (const juce::MouseEvent&)
{
    if (! dragging) return;
    dragging = false;
    attX.endGesture(); attY.endGesture();      // la leva resta dove la si lascia, come l'originale
}

void JoystickView::mouseDoubleClick (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) return;
    attX.setValueAsCompleteGesture (5.0f); attY.setValueAsCompleteGesture (5.0f);
}

//==============================================================================
const char* MatrixView::rowName (int row)
{
    static const char* names[16] = { "Osc 1 sinusoide", "Osc 1 rampa", "Osc 2 quadra", "Osc 2 triangolo", "Osc 3 quadra", "Osc 3 triangolo",
                                     "Noise", "Input Ch 1", "Input Ch 2", "Filter", "Trapezoid", "Envelope Signal", "Ring Modulator",
                                     "Reverberation", "Stick orizzontale", "Stick verticale" };
    return row >= 0 && row < 16 ? names[row] : "";
}

const char* MatrixView::colName (int col)
{
    static const char* names[16] = { "Meter", "Output Ch 1", "Output Ch 2", "Envelope", "Ring Mod A", "Ring Mod B", "Reverb", "Filter",
                                     "Frequenza Osc 1", "Frequenza Osc 2", "Frequenza Osc 3", "Decay", "Reverb Mix", "Filter Frequency",
                                     "Output Ch 1 Level", "Output Ch 2 Level" };
    return col >= 0 && col < 16 ? names[col] : "";
}

void MatrixView::refresh()
{
    const int v = proc.getPinsVersion();
    if (v == version) return;
    version = v;
    pins = proc.getPins();
    repaint();
}

bool MatrixView::cellAt (juce::Point<float> p, int& row, int& col) const
{
    const float cw = (float) getWidth() / 16.0f, ch = (float) getHeight() / 16.0f;
    col = (int) std::floor (p.x / cw); row = (int) std::floor (p.y / ch);
    return col >= 0 && col < 16 && row >= 0 && row < 16;
}

void MatrixView::paint (juce::Graphics& g)
{
    const auto& t = theme();
    const auto b = getLocalBounds().toFloat();
    g.setColour (t.board);
    g.fillRoundedRectangle (b, 6.0f);
    const float cw = b.getWidth() / 16.0f, ch = b.getHeight() / 16.0f, hole = juce::jmin (cw, ch) * 0.20f;
    // fori
    for (int r = 0; r < 16; ++r)
        for (int c = 0; c < 16; ++c)
        {
            const juce::Point<float> p ((c + 0.5f) * cw, (r + 0.5f) * ch);
            g.setColour (juce::Colour (0xff6b5a2e));
            g.fillEllipse (juce::Rectangle<float> (hole * 2.4f, hole * 2.4f).withCentre (p));
            g.setColour (juce::Colours::black);
            g.fillEllipse (juce::Rectangle<float> (hole * 1.5f, hole * 1.5f).withCentre (p));
        }
    // pin
    for (int r = 0; r < 16; ++r)
        for (int c = 0; c < 16; ++c)
        {
            const auto pin = (psm3::Pin) pins[(size_t) (r * 16 + c)];
            if (pin == psm3::Pin::None) continue;
            const juce::Point<float> p ((c + 0.5f) * cw, (r + 0.5f) * ch);
            const auto col = pin == psm3::Pin::White ? t.pinWhite : (pin == psm3::Pin::Red ? t.pinRed : t.pinGreen);
            const float pr = juce::jmin (cw, ch) * 0.40f;
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillEllipse (juce::Rectangle<float> (pr * 2.0f, pr * 2.0f).withCentre ({ p.x + 2.0f, p.y + 3.0f }));
            juce::ColourGradient cg (col.brighter (0.5f), p.x - pr * 0.4f, p.y - pr * 0.5f, col.darker (0.35f), p.x + pr * 0.7f, p.y + pr * 0.9f, true);
            g.setGradientFill (cg);
            g.fillEllipse (juce::Rectangle<float> (pr * 2.0f, pr * 2.0f).withCentre (p));
        }
}

void MatrixView::mouseDown (const juce::MouseEvent& e)
{
    int row, col;
    if (! cellAt (e.position, row, col)) return;
    const auto current = (psm3::Pin) pins[(size_t) (row * 16 + col)];
    psm3::Pin next = psm3::Pin::None;
    if (! e.mods.isPopupMenu())      // clic: bianco -> rosso -> verde -> vuoto; clic destro: toglie
    {
        switch (current)
        {
            case psm3::Pin::None:  next = psm3::Pin::White; break;
            case psm3::Pin::White: next = psm3::Pin::Red; break;
            case psm3::Pin::Red:   next = psm3::Pin::Green; break;
            case psm3::Pin::Green: next = psm3::Pin::None; break;
        }
    }
    proc.setPin (row, col, next);
    refresh();
}

juce::String MatrixView::getTooltip()
{
    int row, col;
    if (! cellAt (getMouseXYRelative().toFloat(), row, col)) return {};
    juce::String s;
    s << (char) ('A' + col) << (row + 1) << ":  " << juce::String (juce::CharPointer_UTF8 (rowName (row)))
      << "  ->  " << juce::String (juce::CharPointer_UTF8 (colName (col)));
    return s;
}

//==============================================================================
static bool isBlack (int note) { const int n = note % 12; return n == 1 || n == 3 || n == 6 || n == 8 || n == 10; }
static int whiteIndex (int note)        // numero di tasti bianchi prima di questa nota, da kLowNote
{
    int count = 0;
    for (int n = KeyboardView::kLowNote; n < note; ++n) if (! isBlack (n)) ++count;
    return count;
}

juce::Rectangle<float> KeyboardView::keyBounds (int note) const
{
    const float ww = (float) getWidth() / 22.0f, h = (float) getHeight();
    if (! isBlack (note)) return { whiteIndex (note) * ww, 0.0f, ww, h };
    return { whiteIndex (note) * ww - ww * 0.3f, 0.0f, ww * 0.6f, h * 0.62f };
}

int KeyboardView::noteAt (juce::Point<float> p, float& velocity) const
{
    int found = -1;
    for (int n = kLowNote; n <= kHighNote; ++n) if (isBlack (n) && keyBounds (n).contains (p)) found = n;
    if (found < 0) for (int n = kLowNote; n <= kHighNote; ++n) if (! isBlack (n) && keyBounds (n).contains (p)) found = n;
    if (found >= 0)
    {
        const auto kb = keyBounds (found);       // piu' in basso si preme, piu' forte e' il tocco
        velocity = juce::jlimit (0.2f, 1.0f, 0.2f + 0.8f * (p.y - kb.getY()) / kb.getHeight());
    }
    return found;
}

void KeyboardView::press (int note, float velocity)
{
    if (note == held) return;
    const int old = held;
    held = note;
    if (note >= 0) proc.keyboardState.noteOn (1, note, velocity);       // prima la nuova, poi si rilascia la vecchia: legato
    if (old >= 0) proc.keyboardState.noteOff (1, old, 0.0f);
}

void KeyboardView::release() { if (held >= 0) { proc.keyboardState.noteOff (1, held, 0.0f); held = -1; } }

void KeyboardView::mouseDown (const juce::MouseEvent& e) { float v = 0.8f; const int n = noteAt (e.position, v); if (n >= 0) press (n, v); }
void KeyboardView::mouseDrag (const juce::MouseEvent& e) { float v = 0.8f; const int n = noteAt (e.position, v); if (n >= 0) press (n, v); }
void KeyboardView::mouseUp (const juce::MouseEvent&) { release(); }

void KeyboardView::refresh()
{
    juce::uint64 now[2] = { 0, 0 };
    for (int n = kLowNote; n <= kHighNote; ++n)
        if (proc.keyboardState.isNoteOnForChannels (0xffff, n)) now[(n - kLowNote) / 64] |= (juce::uint64) 1 << ((n - kLowNote) % 64);
    if (now[0] != shown[0] || now[1] != shown[1]) { shown[0] = now[0]; shown[1] = now[1]; repaint(); }
}

void KeyboardView::paint (juce::Graphics& g)
{
    auto isDown = [this] (int n) { return (shown[(n - kLowNote) / 64] >> ((n - kLowNote) % 64) & 1) != 0; };
    for (int n = kLowNote; n <= kHighNote; ++n)
    {
        if (isBlack (n)) continue;
        const auto r = keyBounds (n).reduced (0.6f, 0.0f);
        juce::ColourGradient cg (isDown (n) ? juce::Colour (0xffd8d6cc) : juce::Colour (0xfffbfaf4), 0.0f, r.getY(),
                                 isDown (n) ? juce::Colour (0xffc2c0b6) : juce::Colour (0xffe6e4da), 0.0f, r.getBottom(), false);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (juce::Colour (0xff80807a));
        g.drawRoundedRectangle (r, 2.0f, 0.8f);
        if (n % 12 == 0)
        {
            g.setColour (juce::Colour (0xff9a988e));
            g.setFont (font (10.0f));
            g.drawText ("C" + juce::String (n / 12 - 2), r.withTop (r.getBottom() - 16.0f), juce::Justification::centred, false);
        }
    }
    for (int n = kLowNote; n <= kHighNote; ++n)
    {
        if (! isBlack (n)) continue;
        const auto r = keyBounds (n);
        g.setColour (isDown (n) ? juce::Colour (0xff3a3a3a) : juce::Colour (0xff141414));
        g.fillRoundedRectangle (r, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.fillRect (r.reduced (r.getWidth() * 0.2f, 0.0f).withTrimmedBottom (r.getHeight() * 0.12f).withTrimmedTop (2.0f));
    }
}
}
