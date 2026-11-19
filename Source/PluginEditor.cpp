#include "PluginEditor.h"

using namespace juce;

namespace aa22ui
{
namespace col
{
    const Colour panel      (0xffd9d9d7);
    const Colour panelLight (0xffe6e6e4);
    const Colour panelDark  (0xffb9b9b7);
    const Colour ear        (0xffcfcfcd);
    const Colour ink        (0xff2a2a2a);
    const Colour red        (0xffc4122f);
    const Colour vuFace     (0xfff3d9a2);
    const Colour vuRed      (0xffd2232a);
}

static constexpr float knobStart = MathConstants<float>::pi * 1.20f;
static constexpr float knobEnd   = MathConstants<float>::pi * 2.80f;

static Font panelFont (float h, bool bold = false)
{
    return Font (FontOptions (h, bold ? Font::bold : Font::plain)).withExtraKerningFactor (0.06f);
}

//==============================================================================
// LookAndFeel
//==============================================================================
void RackLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, Slider& s)
{
    auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto kind = s.getProperties()["kind"].toString();

    if (kind == "pointer")      drawPointerKnob (g, area, angle);
    else if (kind == "screw")   drawScrewTrim (g, area, angle);
    else                        drawBigKnob (g, area, angle, s);
}

void RackLookAndFeel::drawBigKnob (Graphics& g, Rectangle<float> area, float angle, Slider&)
{
    const auto c = area.getCentre();
    const float r = jmin (area.getWidth(), area.getHeight()) * 0.5f;

    // Drop shadow
    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c.translated (r * 0.12f, r * 0.16f)));

    // Fluted skirt
    const int flutes = 24;
    Path skirt;
    for (int i = 0; i <= flutes * 2; ++i)
    {
        const float a = angle + MathConstants<float>::twoPi * (float) i / (float) (flutes * 2);
        const float rr = (i % 2 == 0) ? r : r * 0.93f;
        const auto p = c.getPointOnCircumference (rr, a);
        if (i == 0) skirt.startNewSubPath (p); else skirt.lineTo (p);
    }
    skirt.closeSubPath();
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), c.x - r, c.y - r,
                                       Colour (0xff050505), c.x + r, c.y + r, false));
    g.fillPath (skirt);

    // Cap
    const float capR = r * 0.72f;
    auto cap = Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c);
    g.setGradientFill (ColourGradient (Colour (0xff4a4a4a), cap.getX(), cap.getY(),
                                       Colour (0xff0d0d0d), cap.getRight(), cap.getBottom(), false));
    g.fillEllipse (cap);
    g.setColour (Colours::white.withAlpha (0.12f));
    g.drawEllipse (cap.reduced (1.0f), 1.0f);

    // Pointer line
    g.setColour (Colour (0xfff2f2f2));
    Path line;
    line.startNewSubPath (c.getPointOnCircumference (r * 0.12f, angle));
    line.lineTo (c.getPointOnCircumference (r * 0.88f, angle));
    g.strokePath (line, PathStrokeType (r * 0.07f, PathStrokeType::curved, PathStrokeType::rounded));
}

void RackLookAndFeel::drawPointerKnob (Graphics& g, Rectangle<float> area, float angle)
{
    const auto c = area.getCentre();
    const float r = jmin (area.getWidth(), area.getHeight()) * 0.5f;

    g.setColour (Colours::black.withAlpha (0.35f));
    g.fillEllipse (Rectangle<float> (r * 1.6f, r * 1.6f).withCentre (c.translated (r * 0.12f, r * 0.15f)));

    // Round body
    auto body = Rectangle<float> (r * 1.55f, r * 1.55f).withCentre (c);
    g.setGradientFill (ColourGradient (Colour (0xff3c3c3c), body.getX(), body.getY(),
                                       Colour (0xff070707), body.getRight(), body.getBottom(), false));
    g.fillEllipse (body);

    // Chicken-head pointer
    Path ptr;
    const float pw = r * 0.34f;
    ptr.addRoundedRectangle (-pw * 0.5f, -r * 0.98f, pw, r * 1.5f, pw * 0.5f);
    ptr.applyTransform (AffineTransform::rotation (angle).translated (c));
    g.setGradientFill (ColourGradient (Colour (0xff2c2c2c), c.x - r, c.y - r,
                                       Colour (0xff000000), c.x + r, c.y + r, false));
    g.fillPath (ptr);

    g.setColour (Colour (0xfff2f2f2));
    Path line;
    line.startNewSubPath (c.getPointOnCircumference (r * 0.15f, angle));
    line.lineTo (c.getPointOnCircumference (r * 0.88f, angle));
    g.strokePath (line, PathStrokeType (r * 0.07f, PathStrokeType::curved, PathStrokeType::rounded));
}

void RackLookAndFeel::drawScrewTrim (Graphics& g, Rectangle<float> area, float angle)
{
    const auto c = area.getCentre();
    const float r = jmin (area.getWidth(), area.getHeight()) * 0.5f;

    // Hex nut
    Path hex;
    for (int i = 0; i < 6; ++i)
    {
        const auto p = c.getPointOnCircumference (r * 0.98f, MathConstants<float>::pi / 6.0f + i * MathConstants<float>::pi / 3.0f);
        if (i == 0) hex.startNewSubPath (p); else hex.lineTo (p);
    }
    hex.closeSubPath();
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillPath (hex, AffineTransform::translation (1.5f, 2.0f));
    g.setGradientFill (ColourGradient (Colour (0xffe8e8e8), c.x - r, c.y - r, Colour (0xff8a8a8a), c.x + r, c.y + r, false));
    g.fillPath (hex);
    g.setColour (Colour (0xff6d6d6d));
    g.strokePath (hex, PathStrokeType (0.8f));

    // Screw head
    auto head = Rectangle<float> (r * 1.15f, r * 1.15f).withCentre (c);
    g.setGradientFill (ColourGradient (Colour (0xffd0d0d0), head.getX(), head.getY(), Colour (0xff7a7a7a), head.getRight(), head.getBottom(), false));
    g.fillEllipse (head);
    g.setColour (Colour (0xff5a5a5a));
    g.drawEllipse (head, 0.8f);

    // Slot
    Path slot;
    slot.addRectangle (-r * 0.5f, -r * 0.07f, r * 1.0f, r * 0.14f);
    slot.applyTransform (AffineTransform::rotation (angle).translated (c));
    g.setColour (Colour (0xff3a3a3a));
    g.fillPath (slot);
}

void RackLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool, bool)
{
    auto area = b.getLocalBounds().toFloat();
    const auto c = area.getCentre();
    const float r = jmin (area.getWidth(), area.getHeight() * 0.6f) * 0.5f;
    const bool up = b.getToggleState();

    // Hex nut
    Path hex;
    for (int i = 0; i < 6; ++i)
    {
        const auto p = c.getPointOnCircumference (r, i * MathConstants<float>::pi / 3.0f);
        if (i == 0) hex.startNewSubPath (p); else hex.lineTo (p);
    }
    hex.closeSubPath();
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillPath (hex, AffineTransform::translation (2.0f, 2.5f));
    g.setGradientFill (ColourGradient (Colour (0xfff0f0f0), c.x - r, c.y - r, Colour (0xff8c8c8c), c.x + r, c.y + r, false));
    g.fillPath (hex);
    g.setColour (Colour (0xff707070));
    g.strokePath (hex, PathStrokeType (0.8f));

    auto ring = Rectangle<float> (r * 1.1f, r * 1.1f).withCentre (c);
    g.setGradientFill (ColourGradient (Colour (0xff9a9a9a), ring.getX(), ring.getY(), Colour (0xffe0e0e0), ring.getRight(), ring.getBottom(), false));
    g.fillEllipse (ring);

    // Bat lever
    const float dir = up ? -1.0f : 1.0f;
    const auto tip = c.translated (0.0f, dir * r * 1.55f);
    g.setColour (Colours::black.withAlpha (0.3f));
    g.drawLine (Line<float> (c.translated (2.0f, 3.0f), tip.translated (3.0f, 4.0f)), r * 0.42f);

    Path lever;
    lever.startNewSubPath (c.translated (-r * 0.16f, 0.0f));
    lever.lineTo (tip.translated (-r * 0.24f, 0.0f));
    lever.lineTo (tip.translated (r * 0.24f, 0.0f));
    lever.lineTo (c.translated (r * 0.16f, 0.0f));
    lever.closeSubPath();
    g.setGradientFill (ColourGradient (Colour (0xff7b7b7b), c.x - r * 0.3f, c.y, Colour (0xfff5f5f5), c.x + r * 0.1f, c.y, false));
    g.fillPath (lever);

    auto ball = Rectangle<float> (r * 0.62f, r * 0.62f).withCentre (tip);
    g.setGradientFill (ColourGradient (Colour (0xffffffff), ball.getX(), ball.getY(), Colour (0xff6f6f6f), ball.getRight(), ball.getBottom(), true));
    g.fillEllipse (ball);
    g.setColour (Colour (0xff666666));
    g.drawEllipse (ball, 0.7f);
}

//==============================================================================
// VU meter
//==============================================================================
VUMeter::VUMeter (AA22CompProcessor& p) : proc (p)
{
    setInterceptsMouseClicks (false, false);
    needle = dbToNeedle (0.0f);
    startTimerHz (60);
}

float VUMeter::dbToNeedle (float db) const
{
    // Real VU scales are linear in voltage: +3 VU at full scale.
    db = jlimit (-30.0f, 4.0f, db);
    return std::pow (10.0f, db / 20.0f) / std::pow (10.0f, 3.0f / 20.0f);
}

void VUMeter::timerCallback()
{
    const int mode = (int) proc.apvts.getRawParameterValue ("meter")->load();
    const bool on  = proc.apvts.getRawParameterValue ("power")->load() > 0.5f;

    float target;
    if (! on)
        target = 0.0f;
    else if (mode == 1)
        target = dbToNeedle (-proc.meterGainReductionDb.load());
    else
    {
        const float rms = proc.meterOutputRms.load();
        const float dbfs = 20.0f * std::log10 (rms + 1.0e-9f);
        const float ref = (mode == 0) ? 12.0f : 18.0f; // +10: 0 VU = -12 dBFS, +4: 0 VU = -18 dBFS
        target = (dbfs < -60.0f) ? 0.0f : dbToNeedle (dbfs + ref);
    }

    // Second-order needle ballistics (~300 ms, slight damping like a real VU)
    const float dt = 1.0f / 60.0f, w = 16.0f, zeta = 0.78f;
    const float acc = w * w * (target - needle) - 2.0f * zeta * w * velocity;
    velocity += acc * dt;
    needle   += velocity * dt;
    needle = jlimit (-0.02f, 1.06f, needle);

    lamp += ((on ? 1.0f : 0.0f) - lamp) * 0.15f;
    repaint();
}

void VUMeter::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Bezel
    g.setColour (Colours::black.withAlpha (0.3f));
    g.fillRect (b.translated (3.0f, 4.0f));
    g.setColour (Colour (0xff6e7378));
    g.fillRect (b);
    auto inner = b.reduced (b.getWidth() * 0.07f, b.getHeight() * 0.11f);
    // bevels
    Path top; top.startNewSubPath (b.getTopLeft()); top.lineTo (b.getTopRight()); top.lineTo (inner.getTopRight()); top.lineTo (inner.getTopLeft()); top.closeSubPath();
    g.setColour (Colour (0xff8e9397)); g.fillPath (top);
    Path left; left.startNewSubPath (b.getTopLeft()); left.lineTo (inner.getTopLeft()); left.lineTo (inner.getBottomLeft()); left.lineTo (b.getBottomLeft()); left.closeSubPath();
    g.setColour (Colour (0xff7c8186)); g.fillPath (left);
    Path bottom; bottom.startNewSubPath (b.getBottomLeft()); bottom.lineTo (inner.getBottomLeft()); bottom.lineTo (inner.getBottomRight()); bottom.lineTo (b.getBottomRight()); bottom.closeSubPath();
    g.setColour (Colour (0xff53575b)); g.fillPath (bottom);
    g.setColour (Colour (0xff3e4245));
    g.drawRect (b, 1.0f);

    // Face
    auto face = inner;
    const auto faceCol = col::vuFace.interpolatedWith (Colour (0xff6f6650), 1.0f - lamp);
    g.setGradientFill (ColourGradient (faceCol.brighter (0.15f), face.getCentreX(), face.getCentreY(),
                                       faceCol.darker (0.25f), face.getX(), face.getY(), true));
    g.fillRect (face);
    g.setColour (Colours::black.withAlpha (0.55f));
    g.drawRect (face, 1.5f);

    Graphics::ScopedSaveState ss (g);
    g.reduceClipRegion (face.toNearestInt());

    const float fw = face.getWidth(), fh = face.getHeight();
    const Point<float> pivot (face.getCentreX(), face.getBottom() + fh * 0.55f);
    const float arcR = fh * 1.12f;
    const float halfSpan = degreesToRadians (40.0f);
    auto angleFor = [&] (float pos) { return -halfSpan + pos * 2.0f * halfSpan; };
    auto posForDb = [&] (float db) { return dbToNeedle (db); };

    const Colour ink (0xff1e1a14);

    // Title
    g.setColour (ink.withAlpha (0.85f));
    g.setFont (panelFont (fh * 0.085f));
    g.drawText ("VU LEVEL INDICATOR", face.withHeight (fh * 0.17f).translated (0, fh * 0.03f), Justification::centred);

    // Main arc (black up to 0, red from 0 to +3)
    const float pos0 = posForDb (0.0f);
    Path arcBlack, arcRed;
    arcBlack.addCentredArc (pivot.x, pivot.y, arcR, arcR, 0.0f, angleFor (0.0f), angleFor (pos0), true);
    arcRed.addCentredArc   (pivot.x, pivot.y, arcR, arcR, 0.0f, angleFor (pos0), angleFor (1.0f), true);
    g.setColour (ink);
    g.strokePath (arcBlack, PathStrokeType (1.4f));
    g.setColour (col::vuRed);
    g.strokePath (arcRed, PathStrokeType (fh * 0.035f));

    // dB ticks and labels
    const float dbMarks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
    g.setFont (panelFont (fh * 0.095f));
    for (float db : dbMarks)
    {
        const float a = angleFor (posForDb (db));
        const bool red = db > 0.0f;
        g.setColour (red ? col::vuRed : ink);
        g.drawLine (Line<float> (pivot.getPointOnCircumference (arcR, a), pivot.getPointOnCircumference (arcR + fh * 0.07f, a)), 1.3f);
        const auto lp = pivot.getPointOnCircumference (arcR + fh * 0.15f, a);
        String txt = db > 0 ? "+" + String ((int) db) : String ((int) db);
        g.drawText (txt, Rectangle<float> (fw * 0.12f, fh * 0.12f).withCentre (lp), Justification::centred);
    }
    // minor ticks
    for (float db = -20.0f; db < -10.0f; db += 2.0f)
    {
        const float a = angleFor (posForDb (db));
        g.setColour (ink);
        g.drawLine (Line<float> (pivot.getPointOnCircumference (arcR, a), pivot.getPointOnCircumference (arcR + fh * 0.035f, a)), 1.0f);
    }

    // Percentage dots below the arc
    g.setFont (panelFont (fh * 0.075f));
    for (int pct : { 0, 20, 40, 60, 80, 100 })
    {
        const float p = (float) pct / 100.0f * pos0;
        const float a = angleFor (p);
        g.setColour (ink);
        g.fillEllipse (Rectangle<float> (3.2f, 3.2f).withCentre (pivot.getPointOnCircumference (arcR - fh * 0.06f, a)));
        if (pct >= 40)
            g.drawText (String (pct), Rectangle<float> (fw * 0.1f, fh * 0.1f).withCentre (pivot.getPointOnCircumference (arcR - fh * 0.15f, a)), Justification::centred);
    }

    // VU labels
    g.setFont (panelFont (fh * 0.14f));
    g.setColour (ink);
    g.drawText ("VU", Rectangle<float> (fw * 0.12f, fh * 0.15f).withCentre ({ face.getX() + fw * 0.09f, face.getY() + fh * 0.36f }), Justification::centred);
    g.setColour (col::vuRed);
    g.drawText ("VU", Rectangle<float> (fw * 0.12f, fh * 0.15f).withCentre ({ face.getRight() - fw * 0.09f, face.getY() + fh * 0.36f }), Justification::centred);

    // Brand in the face
    g.setColour (ink.withAlpha (0.9f));
    {
        Font f (FontOptions (fh * 0.20f, Font::bold | Font::italic));
        f.setHorizontalScale (0.85f);
        g.setFont (f);
        g.drawText ("22AA", Rectangle<float> (fw * 0.4f, fh * 0.2f).withCentre ({ face.getCentreX() + fw * 0.02f, face.getY() + fh * 0.72f }), Justification::centred);
        g.setFont (panelFont (fh * 0.06f));
        g.drawText ("LEVELING  AMPLIFIER", Rectangle<float> (fw * 0.4f, fh * 0.08f).withCentre ({ face.getCentreX() + fw * 0.02f, face.getY() + fh * 0.84f }), Justification::centred);
    }

    // Needle
    const float a = angleFor (needle);
    g.setColour (Colours::black.withAlpha (0.18f));
    g.drawLine (Line<float> (pivot.translated (3.0f, 2.0f), pivot.getPointOnCircumference (arcR + fh * 0.1f, a).translated (3.0f, 2.0f)), 1.6f);
    g.setColour (Colour (0xff151515));
    g.drawLine (Line<float> (pivot, pivot.getPointOnCircumference (arcR + fh * 0.1f, a)), 1.6f);

    // Glass glare
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.18f * lamp + 0.05f), face.getX(), face.getY(),
                                       Colours::transparentWhite, face.getX(), face.getCentreY(), false));
    g.fillRect (face.withHeight (fh * 0.45f));
}

//==============================================================================
// Content (the panel itself, in base coordinates)
//==============================================================================
Content::Content (AA22CompProcessor& p) : proc (p), vu (p)
{
    setLookAndFeel (&lnf);

    auto setupKnob = [this] (Slider& s, const char* kind, double resetValue)
    {
        s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        s.getProperties().set ("kind", kind);
        s.setDoubleClickReturnValue (true, resetValue);
        s.setMouseDragSensitivity (300);
        addAndMakeVisible (s);
    };

    setupKnob (gainKnob, "big", 50.0);
    setupKnob (peakKnob, "big", 0.0);
    setupKnob (emphasisScrew, "screw", 0.0);
    setupKnob (meterKnob, "pointer", 1.0);

    gainKnob.setRotaryParameters (knobStart, knobEnd, true);
    peakKnob.setRotaryParameters (knobStart, knobEnd, true);
    emphasisScrew.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    meterKnob.setRotaryParameters (MathConstants<float>::pi * 1.75f, MathConstants<float>::pi * 2.25f, true);
    meterKnob.setMouseDragSensitivity (120);

    gainKnob.setTooltip ("Gain (make-up)");
    peakKnob.setTooltip ("Peak Reduction (amount of compression)");
    emphasisScrew.setTooltip ("HF Emphasis: makes the sidechain react more to highs (de-essing style)");
    meterKnob.setTooltip ("Meter: Output +10 / Gain Reduction / Output +4");

    for (auto* b : { &limitSwitch, &powerSwitch })
    {
        b->setClickingTogglesState (true);
        addAndMakeVisible (*b);
    }
    limitSwitch.setTooltip ("Up = Limit, Down = Compress");
    powerSwitch.setTooltip ("Power (bypass when off)");

    addAndMakeVisible (vu);

    auto& s = proc.apvts;
    gainAtt     = std::make_unique<SA> (s, "gain", gainKnob);
    peakAtt     = std::make_unique<SA> (s, "peakReduction", peakKnob);
    meterAtt    = std::make_unique<SA> (s, "meter", meterKnob);
    emphasisAtt = std::make_unique<SA> (s, "emphasis", emphasisScrew);
    limitAtt    = std::make_unique<BA> (s, "limit", limitSwitch);
    powerAtt    = std::make_unique<BA> (s, "power", powerSwitch);
}

Content::~Content()
{
    setLookAndFeel (nullptr);
}

void Content::resized()
{
    auto centred = [] (float cx, float cy, float w, float h)
    {
        return Rectangle<float> (w, h).withCentre ({ cx, cy }).toNearestInt();
    };

    gainKnob.setBounds      (centred (285.0f, 172.0f, 84.0f, 84.0f));
    peakKnob.setBounds      (centred (715.0f, 172.0f, 84.0f, 84.0f));
    meterKnob.setBounds     (centred (846.0f, 100.0f, 66.0f, 66.0f));
    emphasisScrew.setBounds (centred (158.0f, 188.0f, 34.0f, 34.0f));
    limitSwitch.setBounds   (centred (104.0f, 188.0f, 38.0f, 62.0f));
    powerSwitch.setBounds   (centred (846.0f, 188.0f, 38.0f, 62.0f));
    vu.setBounds            (centred (500.0f, 115.0f, 210.0f, 125.0f));
}

void Content::drawScrew (Graphics& g, float cx, float cy, float r) const
{
    auto b = Rectangle<float> (r * 2.0f, r * 2.0f).withCentre ({ cx, cy });
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillEllipse (b.translated (0.8f, 1.2f));
    g.setGradientFill (ColourGradient (Colour (0xfff4f4f4), b.getX(), b.getY(), Colour (0xff9a9a9a), b.getRight(), b.getBottom(), false));
    g.fillEllipse (b);
    g.setColour (Colour (0xff6a6a6a));
    g.drawEllipse (b, 0.8f);
    // Phillips cross
    g.setColour (Colour (0xff4a4a4a));
    const float k = r * 0.55f;
    g.drawLine (cx - k, cy - k * 0.3f, cx + k, cy + k * 0.3f, 1.4f);
    g.drawLine (cx - k * 0.3f, cy + k, cx + k * 0.3f, cy - k, 1.4f);
}

void Content::drawKnobScale (Graphics& g, Point<float> c, float radius) const
{
    g.setColour (col::ink);
    for (int i = 0; i <= 100; i += 2)
    {
        const float a = knobStart + (knobEnd - knobStart) * (float) i / 100.0f;
        const bool major = (i % 10 == 0);
        const float r1 = radius - (major ? 9.0f : 5.0f);
        g.drawLine (Line<float> (c.getPointOnCircumference (r1, a), c.getPointOnCircumference (radius - 1.0f, a)), major ? 1.3f : 0.8f);
    }
    for (int i = 0; i <= 100; i += 10)
    {
        const float a = knobStart + (knobEnd - knobStart) * (float) i / 100.0f;
        const auto p = c.getPointOnCircumference (radius + 9.0f, a);
        Graphics::ScopedSaveState ss (g);
        g.addTransform (AffineTransform::rotation (a, p.x, p.y));
        g.setFont (panelFont (15.0f));
        g.drawText (String (i), Rectangle<float> (34.0f, 16.0f).withCentre (p), Justification::centred);
    }
}

void Content::paint (Graphics& g)
{
    const float W = (float) baseW, H = (float) baseH;

    // Rack ears
    g.setColour (col::ear);
    g.fillRect (0.0f, 0.0f, W, H);
    for (float x : { 0.0f, W - 46.0f })
    {
        auto ear = Rectangle<float> (x, 0.0f, 46.0f, H - 4.0f);
        g.setGradientFill (ColourGradient (Colour (0xffd8d8d6), ear.getX(), 0, Colour (0xffbdbdbb), ear.getRight(), 0, false));
        g.fillRect (ear);
        for (float y : { 72.0f, 193.0f })
        {
            auto slot = Rectangle<float> (x < 1.0f ? -6.0f : x + 22.0f, y - 7.0f, 30.0f, 14.0f);
            g.setColour (Colour (0xff111111));
            g.fillRoundedRectangle (slot, 7.0f);
        }
    }

    // Main panel
    auto panel = Rectangle<float> (50.0f, 0.0f, W - 100.0f, H - 6.0f);
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillRect (panel.translated (0.0f, 2.0f));
    g.setGradientFill (ColourGradient (col::panelLight, panel.getX(), panel.getY(), col::panel, panel.getX(), panel.getBottom(), false));
    g.fillRect (panel);
    g.setColour (col::panelDark);
    g.drawRect (panel, 1.0f);

    // Bottom hinge strip
    auto hinge = Rectangle<float> (50.0f, H - 6.0f, W - 100.0f, 6.0f);
    g.setColour (Colour (0xff8f8f8d));
    g.fillRect (hinge);
    g.setColour (Colour (0xff5a5a58));
    for (float x = hinge.getX(); x < hinge.getRight(); x += 14.0f)
        g.drawVerticalLine ((int) x, hinge.getY(), hinge.getBottom());

    // Screws
    for (auto p : { Point<float> (102, 15), Point<float> (898, 15), Point<float> (77, 254), Point<float> (923, 254),
                    Point<float> (358, 254), Point<float> (640, 254), Point<float> (595, 47) })
        drawScrew (g, p.x, p.y, 7.0f);

    // ---- Logo ----
    {
        g.setColour (col::red);
        Font logo (FontOptions (44.0f, Font::bold | Font::italic));
        logo.setHorizontalScale (0.85f);
        g.setFont (logo);
        g.drawText ("22AA", Rectangle<float> (104.0f, 38.0f, 120.0f, 46.0f), Justification::centredLeft, false);

        g.setFont (panelFont (9.5f, true).withExtraKerningFactor (0.55f));
        g.drawText ("COMP  AUDIO", Rectangle<float> (106.0f, 81.0f, 130.0f, 12.0f), Justification::centredLeft, false);

        // Double red rule with the slanted kick-up (vintage badge style)
        Path rule;
        rule.startNewSubPath (100.0f, 95.0f);
        rule.lineTo (218.0f, 95.0f);
        rule.lineTo (246.0f, 44.0f);
        rule.lineTo (372.0f, 44.0f);
        g.strokePath (rule, PathStrokeType (1.6f));
        g.strokePath (rule, PathStrokeType (1.6f), AffineTransform::translation (0.0f, 4.0f));

        g.setColour (col::ink);
        g.setFont (panelFont (10.5f));
        g.drawText ("LEVELING  AMPLIFIER", Rectangle<float> (245.0f, 56.0f, 130.0f, 14.0f), Justification::centred, false);
        g.drawText ("MODEL  22AA  COMP", Rectangle<float> (245.0f, 71.0f, 130.0f, 14.0f), Justification::centred, false);
    }

    // ---- Centre emblem ----
    {
        const Point<float> c (500.0f, 30.0f);
        Path d;
        d.startNewSubPath (c.x - 26.0f, c.y);
        d.lineTo (c.x, c.y - 12.0f);
        d.lineTo (c.x + 26.0f, c.y);
        d.lineTo (c.x, c.y + 12.0f);
        d.closeSubPath();
        g.setColour (col::ink);
        g.strokePath (d, PathStrokeType (1.5f));
        g.setFont (Font (FontOptions (12.0f, Font::bold | Font::italic)));
        g.drawText ("22", Rectangle<float> (40.0f, 18.0f).withCentre (c), Justification::centred);
    }

    // ---- Knob scales + labels ----
    drawKnobScale (g, { 285.0f, 172.0f }, 55.0f);
    drawKnobScale (g, { 715.0f, 172.0f }, 55.0f);

    g.setColour (col::ink);
    g.setFont (panelFont (12.0f));
    g.drawText ("GAIN", Rectangle<float> (120.0f, 16.0f).withCentre ({ 285.0f, 243.0f }), Justification::centred);
    g.drawText ("PEAK  REDUCTION", Rectangle<float> (160.0f, 16.0f).withCentre ({ 715.0f, 243.0f }), Justification::centred);

    g.drawText ("LIMIT", Rectangle<float> (80.0f, 14.0f).withCentre ({ 104.0f, 152.0f }), Justification::centred);
    g.drawText ("COMPRESS", Rectangle<float> (90.0f, 14.0f).withCentre ({ 104.0f, 226.0f }), Justification::centred);
    g.setFont (panelFont (8.0f));
    g.drawText ("EMPH", Rectangle<float> (60.0f, 12.0f).withCentre ({ 158.0f, 212.0f }), Justification::centred);

    g.setFont (panelFont (11.0f));
    g.drawText ("GAIN  REDUCTION", Rectangle<float> (140.0f, 14.0f).withCentre ({ 846.0f, 42.0f }), Justification::centred);
    g.drawText ("OUTPUT  +10", Rectangle<float> (100.0f, 14.0f).withCentre ({ 796.0f, 58.0f }), Justification::centred);
    g.drawText ("OUTPUT  +4", Rectangle<float> (100.0f, 14.0f).withCentre ({ 898.0f, 58.0f }), Justification::centred);

    g.drawText ("ON", Rectangle<float> (60.0f, 14.0f).withCentre ({ 846.0f, 152.0f }), Justification::centred);
    g.drawText ("POWER", Rectangle<float> (80.0f, 14.0f).withCentre ({ 846.0f, 226.0f }), Justification::centred);

    // Pilot hole under the meter
    g.setColour (Colour (0xff111111));
    g.fillEllipse (Rectangle<float> (7.0f, 7.0f).withCentre ({ 500.0f, 205.0f }));
}

} // namespace aa22ui

//==============================================================================
AA22CompEditor::AA22CompEditor (AA22CompProcessor& p)
    : AudioProcessorEditor (&p), content (p)
{
    addAndMakeVisible (content);
    content.setBounds (0, 0, aa22ui::baseW, aa22ui::baseH);

    setResizable (true, true);
    setResizeLimits (aa22ui::baseW / 2, aa22ui::baseH / 2, aa22ui::baseW * 2, aa22ui::baseH * 2);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) aa22ui::baseW / (double) aa22ui::baseH);

    setSize (900, (int) std::round (900.0 * aa22ui::baseH / aa22ui::baseW));
}

void AA22CompEditor::paint (Graphics& g)
{
    g.fillAll (aa22ui::col::ear);
}

void AA22CompEditor::resized()
{
    const float scale = (float) getWidth() / (float) aa22ui::baseW;
    content.setTransform (AffineTransform::scale (scale));
}
