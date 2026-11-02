// Renders the pedal UI to PNG files (used by CI to check the look).
#include "PluginProcessor.h"
#include "PluginEditor.h"

static void save (juce::AudioProcessorEditor& e, const juce::String& name)
{
    auto img = e.createComponentSnapshot (e.getLocalBounds(), true, 2.0f);
    auto f = juce::File::getCurrentWorkingDirectory().getChildFile (name);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    LiveLoveLaughProcessor p;
    {
        std::unique_ptr<juce::AudioProcessorEditor> e (p.createEditor());
        save (*e, "pedal_on.png");
    }
    if (auto* pw = p.apvts.getParameter ("power")) pw->setValueNotifyingHost (0.0f);
    if (auto* f = p.apvts.getParameter ("filter")) f->setValueNotifyingHost (0.2f);
    if (auto* k = p.apvts.getParameter ("killdry")) k->setValueNotifyingHost (1.0f);
    {
        std::unique_ptr<juce::AudioProcessorEditor> e (p.createEditor());
        save (*e, "pedal_off.png");
    }
    return 0;
}
