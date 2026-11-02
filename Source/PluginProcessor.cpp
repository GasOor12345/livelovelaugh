#include "PluginProcessor.h"
#include "PluginEditor.h"

LiveLoveLaughProcessor::LiveLoveLaughProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LLL", createLayout())
{
    pLive    = apvts.getRawParameterValue ("live");
    pLove    = apvts.getRawParameterValue ("love");
    pLaugh   = apvts.getRawParameterValue ("laugh");
    pFilter  = apvts.getRawParameterValue ("filter");
    pTime    = apvts.getRawParameterValue ("time");
    pDuck    = apvts.getRawParameterValue ("duck");
    pKillDry = apvts.getRawParameterValue ("killdry");
    pPower   = apvts.getRawParameterValue ("power");
}

juce::AudioProcessorValueTreeState::ParameterLayout LiveLoveLaughProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> ps;

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + "%"; });

    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "live", 1 }, "Live", NormalisableRange<float> (0.0f, 1.0f), 0.35f, pct));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "love", 1 }, "Love", NormalisableRange<float> (0.0f, 1.0f), 0.25f, pct));
    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "laugh", 1 }, "Laugh", NormalisableRange<float> (0.0f, 1.0f), 0.4f, pct));

    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "filter", 1 }, "Filter", NormalisableRange<float> (-1.0f, 1.0f), 0.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) {
            if (std::abs (v) < 0.01f) return String ("open");
            return (v < 0 ? String ("LP ") : String ("HP ")) + String (juce::roundToInt (std::abs (v) * 100.0f)) + "%";
        })));

    StringArray times;
    for (int i = 0; i < lll::Engine::kNumTimes; ++i) times.add (lll::Engine::noteName (i));
    ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "time", 1 }, "Delay Time", times, 2));

    ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "duck", 1 }, "Ducking", NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { "killdry", 1 }, "Kill Dry", false));
    ps.push_back (std::make_unique<AudioParameterBool> (ParameterID { "power", 1 }, "On", true));

    return { ps.begin(), ps.end() };
}

bool LiveLoveLaughProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void LiveLoveLaughProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    monoTmp.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.0f);
}

void LiveLoveLaughProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();

    for (int ch = numIn; ch < numOut; ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, n); // mono in -> both sides

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 0.0) lastBpm = *bpm;

    lll::Engine::Params p;
    p.live = pLive->load();
    p.love = pLove->load();
    p.laugh = pLaugh->load();
    p.filter = std::abs (pFilter->load()) < 0.01f ? 0.0f : pFilter->load();
    p.timeIndex = (int) std::lround (pTime->load());
    p.duck = pDuck->load();
    p.killDry = pKillDry->load() > 0.5f;
    p.power = pPower->load() > 0.5f;
    p.bpm = lastBpm;

    float* L = buffer.getWritePointer (0);
    if (numOut >= 2)
    {
        engine.process (L, buffer.getWritePointer (1), n, p);
    }
    else
    {
        if ((int) monoTmp.size() < n) monoTmp.resize ((size_t) n);
        std::copy (L, L + n, monoTmp.data());
        engine.process (L, monoTmp.data(), n, p);
        for (int i = 0; i < n; ++i) L[i] = 0.5f * (L[i] + monoTmp[(size_t) i]);
    }
}

juce::AudioProcessorEditor* LiveLoveLaughProcessor::createEditor()
{
    return new LiveLoveLaughEditor (*this);
}

void LiveLoveLaughProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void LiveLoveLaughProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LiveLoveLaughProcessor();
}
