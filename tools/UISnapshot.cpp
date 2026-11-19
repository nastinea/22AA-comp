// Renders the plugin editor to a PNG so the UI can be checked without a DAW.
#include <juce_audio_utils/juce_audio_utils.h>
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String out = argc > 1 ? argv[1] : "ui.png";

    std::unique_ptr<AA22CompProcessor> proc (static_cast<AA22CompProcessor*> (createPluginFilter()));
    proc->apvts.getParameter ("peakReduction")->setValueNotifyingHost (0.45f);
    proc->apvts.getParameter ("gain")->setValueNotifyingHost (0.55f);
    proc->meterGainReductionDb.store (4.0f);

    std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
    ed->setSize (1600, (int) (1600.0 * 276.0 / 1000.0));

    // Let the VU needle settle
    for (int i = 0; i < 90; ++i)
        juce::MessageManager::getInstance()->runDispatchLoopUntil (17);

    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (out);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    ed.reset();
    return 0;
}
