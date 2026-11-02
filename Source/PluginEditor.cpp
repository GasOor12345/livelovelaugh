#include "PluginEditor.h"

using namespace juce;

namespace lllui
{
static Font serif (float size, int style = Font::italic)
{
    return Font (FontOptions ("Georgia", size, style));
}

static Font sans (float size, int style = Font::plain)
{
    return Font (FontOptions (size, style));
}

static void drawSparkle (Graphics& g, Point<float> c, float r, Colour col)
{
    Path p;
    p.startNewSubPath (c.x, c.y - r);
    p.quadraticTo (c.x, c.y, c.x + r, c.y);
    p.quadraticTo (c.x, c.y, c.x, c.y + r);
    p.quadraticTo (c.x, c.y, c.x - r, c.y);
    p.quadraticTo (c.x, c.y, c.x, c.y - r);
    p.closeSubPath();
    g.setColour (col);
    g.fillPath (p);
}

void PedalLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                         float startAngle, float endAngle, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f);
    const float radius = jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const bool big = radius > 40.0f;

    // tick marks around the knob
    const bool stepped = s.getInterval() >= 1.0;
    const int ticks = stepped ? (int) std::round (s.getMaximum() - s.getMinimum()) + 1 : 11;
    for (int i = 0; i < ticks; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (ticks - 1);
        const auto pt = c.getPointOnCircumference (radius - 1.5f, a);
        g.setColour (softInk.withAlpha (0.8f));
        g.fillEllipse (Rectangle<float> (3.0f, 3.0f).withCentre (pt));
    }

    const float kr = radius * 0.80f;
    // soft shadow
    g.setColour (Colours::black.withAlpha (0.18f));
    g.fillEllipse (Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (c.translated (0.0f, kr * 0.10f)).expanded (2.0f));
    // skirt
    g.setColour (ink);
    g.fillEllipse (Rectangle<float> (kr * 2.0f, kr * 2.0f).withCentre (c));
    // cap
    const float cr = kr * (big ? 0.80f : 0.78f);
    ColourGradient cap (Colour (0xFF3A3A3A), c.x - cr * 0.5f, c.y - cr * 0.6f,
                        Colour (0xFF0B0B0B), c.x + cr * 0.6f, c.y + cr * 0.7f, true);
    g.setGradientFill (cap);
    g.fillEllipse (Rectangle<float> (cr * 2.0f, cr * 2.0f).withCentre (c));
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawEllipse (Rectangle<float> (cr * 2.0f, cr * 2.0f).withCentre (c), 1.0f);

    // pointer
    const float a = startAngle + pos * (endAngle - startAngle);
    const auto p1 = c.getPointOnCircumference (kr * 0.30f, a);
    const auto p2 = c.getPointOnCircumference (kr * 0.92f, a);
    g.setColour (body);
    g.drawLine ({ p1, p2 }, big ? 3.5f : 2.5f);
}

void PedalLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float,
                                         float, Slider::SliderStyle, Slider&)
{
    const auto r = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float cy = r.getCentreY();
    // track
    g.setColour (softInk);
    g.fillRoundedRectangle (r.getX(), cy - 1.5f, r.getWidth(), 3.0f, 1.5f);
    // centre notch
    g.setColour (ink);
    g.fillRect (r.getCentreX() - 1.0f, cy - 6.0f, 2.0f, 12.0f);
    // thumb
    auto thumb = Rectangle<float> (12.0f, (float) h - 2.0f).withCentre ({ pos, cy });
    g.setColour (Colours::black.withAlpha (0.2f));
    g.fillRoundedRectangle (thumb.translated (0.0f, 1.5f), 3.0f);
    g.setColour (ink);
    g.fillRoundedRectangle (thumb, 3.0f);
    g.setColour (body);
    g.fillRect (thumb.getCentreX() - 0.75f, thumb.getY() + 3.0f, 1.5f, thumb.getHeight() - 6.0f);
}

void PedalLookAndFeel::drawToggleButton (Graphics& g, ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? ink : bodyShade);
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (ink);
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);
    const float d = r.getHeight() - 5.0f;
    const float kx = on ? r.getRight() - d - 2.5f : r.getX() + 2.5f;
    g.setColour (on ? body : ink);
    g.fillEllipse (kx, r.getY() + 2.5f, d, d);
}

void FootSwitch::paintButton (Graphics& g, bool highlighted, bool down)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (r.translated (0.0f, 3.0f), 14.0f);
    auto plate = r.withTrimmedBottom (3.0f).translated (0.0f, down ? 2.0f : 0.0f);
    ColourGradient grad (Colour (0xFF1E1E1E), plate.getX(), plate.getY(),
                         Colour (0xFF0C0C0C), plate.getX(), plate.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (plate, 14.0f);

    // rubber ridges
    g.setColour (Colours::white.withAlpha (highlighted ? 0.07f : 0.045f));
    for (float yy = plate.getY() + 14.0f; yy < plate.getBottom() - 10.0f; yy += 7.0f)
        g.drawLine (plate.getX() + 18.0f, yy, plate.getRight() - 18.0f, yy, 1.0f);

    // title block
    auto title = plate.withSizeKeepingCentre (plate.getWidth() - 40.0f, 74.0f);
    g.setColour (Colour (0xFF0C0C0C));
    g.fillRoundedRectangle (title, 8.0f);
    g.setColour (body);
    g.setFont (serif (31.0f));
    g.drawText ("live love laugh", title.withTrimmedBottom (26.0f), Justification::centredBottom);
    g.setFont (sans (11.0f));
    g.setColour (body.withAlpha (0.65f));
    g.drawText ("d r e a m   d e l a y", title.withTrimmedTop (54.0f), Justification::centredTop);

    drawSparkle (g, { title.getX() + 10.0f, title.getY() + 14.0f }, 5.0f, blush.withAlpha (0.9f));
    drawSparkle (g, { title.getRight() - 12.0f, title.getBottom() - 16.0f }, 4.0f, lavender.withAlpha (0.9f));
}
} // namespace lllui

//==============================================================================
LiveLoveLaughEditor::LiveLoveLaughEditor (LiveLoveLaughProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    for (auto* k : { &live, &love, &laugh, &duck }) setupKnob (*k);
    setupKnob (time, true);

    filter.setSliderStyle (Slider::LinearHorizontal);
    filter.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    filter.setDoubleClickReturnValue (true, 0.0);
    filter.setTooltip ("DJ filter in the feedback: left = darker, right = thinner");
    addAndMakeVisible (filter);

    addAndMakeVisible (killDry);
    addAndMakeVisible (footSwitch);

    auto& s = proc.apvts;
    aLive   = std::make_unique<SA> (s, "live", live);
    aLove   = std::make_unique<SA> (s, "love", love);
    aLaugh  = std::make_unique<SA> (s, "laugh", laugh);
    aTime   = std::make_unique<SA> (s, "time", time);
    aDuck   = std::make_unique<SA> (s, "duck", duck);
    aFilter = std::make_unique<SA> (s, "filter", filter);
    aKillDry = std::make_unique<BA> (s, "killdry", killDry);
    aPower   = std::make_unique<BA> (s, "power", footSwitch);

    // double-click a knob to return to its default
    for (auto [k, id] : { std::pair<Slider*, const char*> { &live, "live" }, { &love, "love" }, { &laugh, "laugh" }, { &duck, "duck" } })
        if (auto* prm = s.getParameter (id))
            k->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));

    lastPower = s.getRawParameterValue ("power")->load() > 0.5f;
    lastTime = (int) std::lround (s.getRawParameterValue ("time")->load());

    setSize (360, 560);
    startTimerHz (20);
}

LiveLoveLaughEditor::~LiveLoveLaughEditor()
{
    setLookAndFeel (nullptr);
}

void LiveLoveLaughEditor::setupKnob (Slider& k, bool stepped)
{
    k.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    k.setRotaryParameters (MathConstants<float>::pi * 1.2f, MathConstants<float>::pi * 2.8f, true);
    k.setMouseDragSensitivity (stepped ? 90 : 220);
    k.setVelocityBasedMode (false);
    addAndMakeVisible (k);
}

void LiveLoveLaughEditor::timerCallback()
{
    const bool power = proc.apvts.getRawParameterValue ("power")->load() > 0.5f;
    const int t = (int) std::lround (proc.apvts.getRawParameterValue ("time")->load());
    if (power != lastPower || t != lastTime)
    {
        lastPower = power;
        lastTime = t;
        repaint();
    }
}

void LiveLoveLaughEditor::paint (Graphics& g)
{
    using namespace lllui;
    const auto r = getLocalBounds().toFloat();

    // enclosure
    g.fillAll (Colour (0xFF1A1A1A));
    auto encl = r.reduced (4.0f);
    ColourGradient bodyGrad (body, encl.getX(), encl.getY(), bodyShade, encl.getX(), encl.getBottom(), false);
    g.setGradientFill (bodyGrad);
    g.fillRoundedRectangle (encl, 22.0f);

    // dreamy haze
    auto haze = [&] (Point<float> c, float rad, Colour col)
    {
        ColourGradient cg (col.withAlpha (0.38f), c.x, c.y, col.withAlpha (0.0f), c.x + rad, c.y, true);
        g.setGradientFill (cg);
        g.fillEllipse (Rectangle<float> (rad * 2.0f, rad * 2.0f).withCentre (c));
    };
    haze ({ 84.0f, 100.0f }, 110.0f, lavender);
    haze ({ 276.0f, 100.0f }, 110.0f, blush);
    haze ({ 180.0f, 190.0f }, 120.0f, lavender.interpolatedWith (blush, 0.5f));

    // screws
    for (auto pt : { Point<float> (22, 22), Point<float> (338, 22), Point<float> (22, 340), Point<float> (338, 340) })
    {
        g.setColour (softInk.withAlpha (0.55f));
        g.fillEllipse (Rectangle<float> (9.0f, 9.0f).withCentre (pt));
        g.setColour (body);
        g.drawLine (pt.x - 3.0f, pt.y - 3.0f, pt.x + 3.0f, pt.y + 3.0f, 1.2f);
    }

    // sparkles
    drawSparkle (g, { 150.0f, 56.0f }, 5.0f, ink.withAlpha (0.55f));
    drawSparkle (g, { 214.0f, 70.0f }, 3.5f, ink.withAlpha (0.35f));
    drawSparkle (g, { 120.0f, 236.0f }, 3.0f, ink.withAlpha (0.35f));
    drawSparkle (g, { 244.0f, 246.0f }, 4.5f, ink.withAlpha (0.5f));

    // LED
    const bool on = lastPower;
    const auto ledC = Point<float> (180.0f, 28.0f);
    if (on)
    {
        ColourGradient glow (ledOn.withAlpha (0.55f), ledC.x, ledC.y, ledOn.withAlpha (0.0f), ledC.x + 16.0f, ledC.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (Rectangle<float> (32.0f, 32.0f).withCentre (ledC));
    }
    g.setColour (on ? ledOn : Colour (0xFF5A2A38));
    g.fillEllipse (Rectangle<float> (9.0f, 9.0f).withCentre (ledC));
    g.setColour (ink.withAlpha (0.6f));
    g.drawEllipse (Rectangle<float> (11.0f, 11.0f).withCentre (ledC), 1.0f);

    // labels
    g.setColour (ink);
    g.setFont (serif (19.0f));
    g.drawText ("live",  Rectangle<float> (28.0f, 156.0f, 112.0f, 22.0f), Justification::centred);
    g.drawText ("laugh", Rectangle<float> (220.0f, 156.0f, 112.0f, 22.0f), Justification::centred);
    g.drawText ("love",  Rectangle<float> (124.0f, 240.0f, 112.0f, 22.0f), Justification::centred);

    g.setFont (sans (10.0f));
    g.setColour (softInk);
    g.drawText ("lo", Rectangle<float> (250.0f, 202.0f, 20.0f, 12.0f), Justification::centredLeft);
    g.drawText ("hi", Rectangle<float> (314.0f, 202.0f, 20.0f, 12.0f), Justification::centredRight);

    g.setColour (ink);
    g.setFont (sans (11.0f, Font::bold));
    g.drawText ("time", Rectangle<float> (24.0f, 322.0f, 80.0f, 14.0f), Justification::centred);
    g.drawText ("duck", Rectangle<float> (256.0f, 322.0f, 80.0f, 14.0f), Justification::centred);
    g.drawText ("kill dry", Rectangle<float> (140.0f, 312.0f, 80.0f, 14.0f), Justification::centred);

    g.setFont (sans (10.0f));
    g.setColour (softInk);
    g.drawText (lll::Engine::noteName (lastTime < 0 ? 2 : lastTime),
                Rectangle<float> (24.0f, 336.0f, 80.0f, 13.0f), Justification::centred);
}

void LiveLoveLaughEditor::resized()
{
    live.setBounds (28, 44, 112, 112);
    laugh.setBounds (220, 44, 112, 112);
    love.setBounds (124, 128, 112, 112);
    filter.setBounds (252, 180, 80, 22);
    time.setBounds (34, 262, 60, 60);
    duck.setBounds (266, 262, 60, 60);
    killDry.setBounds (160, 288, 40, 20);
    footSwitch.setBounds (18, 364, 324, 180);
}
