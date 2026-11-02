#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace lllui
{
const juce::Colour body      { 0xFFF4F1EA };
const juce::Colour bodyShade { 0xFFE6E0D5 };
const juce::Colour ink       { 0xFF151515 };
const juce::Colour softInk   { 0xFF8A857C };
const juce::Colour lavender  { 0xFFC9B8FF };
const juce::Colour blush     { 0xFFFFC6E0 };
const juce::Colour ledOn     { 0xFFFF5A8A };

class PedalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos,
                           float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

// DJ-style filter slider: snaps ("clicks") into the centre, double-click resets.
class FilterSlider : public juce::Slider
{
public:
    double snapValue (double v, DragMode) override { return std::abs (v) < 0.08 ? 0.0 : v; }
};

// The black treadle at the bottom: click anywhere on it to switch the pedal on/off.
class FootSwitch : public juce::Button
{
public:
    FootSwitch() : juce::Button ("power") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};
} // namespace lllui

class LiveLoveLaughEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LiveLoveLaughEditor (LiveLoveLaughProcessor&);
    ~LiveLoveLaughEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, bool stepped = false);

    LiveLoveLaughProcessor& proc;
    lllui::PedalLookAndFeel lnf;

    juce::Slider live, love, laugh, time, duck;
    lllui::FilterSlider filter;
    juce::ToggleButton killDry;
    lllui::FootSwitch footSwitch;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> aLive, aLove, aLaugh, aTime, aDuck, aFilter;
    std::unique_ptr<BA> aKillDry, aPower;

    bool lastPower = true;
    int lastTime = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveLoveLaughEditor)
};
