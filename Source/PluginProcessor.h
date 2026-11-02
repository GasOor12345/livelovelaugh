#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/LLLEngine.h"

class LiveLoveLaughProcessor : public juce::AudioProcessor
{
public:
    LiveLoveLaughProcessor();
    ~LiveLoveLaughProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Live Love Laugh"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

private:
    lll::Engine engine;
    std::atomic<float>* pLive = nullptr, *pLove = nullptr, *pLaugh = nullptr, *pFilter = nullptr,
                      * pTime = nullptr, *pDuck = nullptr, *pKillDry = nullptr, *pPower = nullptr;
    double lastBpm = 120.0;
    std::vector<float> monoTmp;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LiveLoveLaughProcessor)
};
