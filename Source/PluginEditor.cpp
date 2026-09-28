#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
constexpr int baseW = 1400;
constexpr int baseH = 500;
const juce::Colour ink (0xffefeee4);
const juce::Colour muted (0xffaebdb3);
const juce::Colour amber (0xffff9410);

juce::Rectangle<int> scaledRect (juce::Rectangle<int> r, float sx, float sy)
{
    return { juce::roundToInt (r.getX() * sx), juce::roundToInt (r.getY() * sy),
             juce::roundToInt (r.getWidth() * sx), juce::roundToInt (r.getHeight() * sy) };
}

juce::Image loadEmbeddedImageByOriginalFilename (const juce::String& filename)
{
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String original (BinaryData::originalFilenames[i]);
        if (original == filename || juce::File (original).getFileName() == filename)
        {
            int size = 0;
            if (const auto* data = BinaryData::getNamedResource (BinaryData::namedResourceList[i], size))
                return juce::ImageFileFormat::loadFrom (data, (size_t) size);
        }
    }

    return {};
}
}

AKNStepFilterAudioProcessorEditor::AKNStepFilterAudioProcessorEditor (AKNStepFilterAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    panelImage = loadEmbeddedImageByOriginalFilename ("aksf-panel.png");

    configureControls();

    setResizable (true, true);
    setResizeLimits (840, 300, 1960, 700);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
    setSize (baseW, baseH);
    startTimerHz (15);
}

AKNStepFilterAudioProcessorEditor::~AKNStepFilterAudioProcessorEditor() = default;

void AKNStepFilterAudioProcessorEditor::configureControls()
{
    auto add = [this] (LabelledKnob& k)
    {
        addAndMakeVisible (k);
    };

    add (frequency); add (resonance); add (drive); add (morph); add (filterType);
    add (lfoAmount); add (lfoRate); add (lfoWave); add (lfoPhase); add (dryWet); add (volume);

    // Choice parameters are presented as discrete rotary selectors.
    frequency.slider().setRange (0.0, 9.0, 1.0);
    filterType.slider().setRange (0.0, 3.0, 1.0);
    lfoWave.slider().setRange (0.0, 6.0, 1.0);

    frequency.setReadoutFunction ([] (double v)
    {
        static const char* labels[] { "70 Hz", "100 Hz", "150 Hz", "250 Hz", "500 Hz", "1 kHz", "2 kHz", "3 kHz", "5 kHz", "7.5 kHz" };
        return juce::String (labels[juce::jlimit (0, 9, (int) std::lround (v))]);
    });
    resonance.setReadoutFunction ([] (double v) { return juce::String (v, 1) + " %"; });
    drive.setReadoutFunction ([] (double v) { return juce::String (v, 1) + " dB"; });
    morph.setReadoutFunction ([] (double v) { return juce::String (v, 0) + " %"; });
    filterType.setReadoutFunction ([] (double v)
    {
        static const char* labels[] { "HIGH PASS", "BELL", "NOTCH", "MORPH" };
        return juce::String (labels[juce::jlimit (0, 3, (int) std::lround (v))]);
    });
    lfoAmount.setReadoutFunction ([] (double v) { return juce::String (v, 1) + " st"; });
    lfoWave.setReadoutFunction ([] (double v)
    {
        static const char* labels[] { "SINE", "SQUARE", "TRIANGLE", "SAW UP", "SAW DOWN", "S&H ST", "S&H MONO" };
        return juce::String (labels[juce::jlimit (0, 6, (int) std::lround (v))]);
    });
    lfoPhase.setReadoutFunction ([] (double v) { return juce::String ((int) std::lround (v)) + juce::String::charToString (0x00b0); });
    dryWet.setReadoutFunction ([] (double v) { return juce::String (v, 0) + " %"; });
    volume.setReadoutFunction ([] (double v) { return v <= -69.9 ? juce::String ("-inf") : juce::String (v, 1) + " dB"; });

    syncButton.setClickingTogglesState (true);
    syncButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff071b19));
    syncButton.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff31574d));
    syncButton.setColour (juce::TextButton::textColourOffId, ink);
    syncButton.setColour (juce::TextButton::textColourOnId, ink);
    addAndMakeVisible (syncButton);

    auto& s = processor.apvts;
    frequencyAttachment = std::make_unique<SliderAttachment> (s, "frequency", frequency.slider());
    resonanceAttachment = std::make_unique<SliderAttachment> (s, "resonance", resonance.slider());
    driveAttachment = std::make_unique<SliderAttachment> (s, "drive", drive.slider());
    morphAttachment = std::make_unique<SliderAttachment> (s, "morph", morph.slider());
    filterTypeAttachment = std::make_unique<SliderAttachment> (s, "filterType", filterType.slider());
    lfoAmountAttachment = std::make_unique<SliderAttachment> (s, "lfoAmount", lfoAmount.slider());
    lfoRateAttachment = std::make_unique<SliderAttachment> (s, "lfoRate", lfoRate.slider());
    lfoWaveAttachment = std::make_unique<SliderAttachment> (s, "lfoWave", lfoWave.slider());
    lfoPhaseAttachment = std::make_unique<SliderAttachment> (s, "lfoPhase", lfoPhase.slider());
    dryWetAttachment = std::make_unique<SliderAttachment> (s, "dryWet", dryWet.slider());
    volumeAttachment = std::make_unique<SliderAttachment> (s, "output", volume.slider());
    syncAttachment = std::make_unique<ButtonAttachment> (s, "lfoSync", syncButton);

    // Set actual ranges after attachments have connected to the parameters.
    resonance.slider().setRange (0.0, 125.0, 0.01);
    drive.slider().setRange (0.0, 10.0, 0.01);
    morph.slider().setRange (0.0, 100.0, 0.01);
    lfoAmount.slider().setRange (0.0, 30.0, 0.01);
    lfoRate.slider().setRange (0.0, 1.0, 0.0001);
    lfoPhase.slider().setRange (0.0, 360.0, 0.1);
    dryWet.slider().setRange (0.0, 100.0, 0.01);
    volume.slider().setRange (-70.0, 6.0, 0.01);

    // Rack defaults. Double-click returns to the current standalone preset state.
    dryWet.slider().setDoubleClickReturnValue (true, (90.0 / 127.0) * 100.0);
    resonance.slider().setDoubleClickReturnValue (true, (59.0 / 127.0) * 125.0);
    drive.slider().setDoubleClickReturnValue (true, (54.0 / 127.0) * 10.0);
    frequency.slider().setDoubleClickReturnValue (true, 6.0);
    volume.slider().setDoubleClickReturnValue (true, 6.0);
    filterType.slider().setDoubleClickReturnValue (true, 3.0);
    morph.slider().setDoubleClickReturnValue (true, 100.0);
    lfoAmount.slider().setDoubleClickReturnValue (true, (73.0 / 127.0) * 30.0);
    lfoRate.slider().setDoubleClickReturnValue (true, 75.0 / 127.0);
    lfoWave.slider().setDoubleClickReturnValue (true, 0.0);
    lfoPhase.slider().setDoubleClickReturnValue (true, 0.0);

    updateDynamicReadouts();
}

void AKNStepFilterAudioProcessorEditor::timerCallback()
{
    updateDynamicReadouts();
}

void AKNStepFilterAudioProcessorEditor::updateDynamicReadouts()
{
    const bool sync = processor.apvts.getRawParameterValue ("lfoSync")->load() >= 0.5f;
    syncButton.setButtonText (sync ? "SYNC" : "FREE");
    syncButton.setToggleState (sync, juce::dontSendNotification);

    const auto rate = (float) lfoRate.slider().getValue();
    if (sync)
    {
        const int idx = juce::jlimit (0, 21, (int) std::lround (rate * 21.0f));
        lfoRate.setReadoutFunction ([idx] (double) { return StepFilterDSP::getSyncRateLabel (idx); });
    }
    else
    {
        lfoRate.setReadoutFunction ([] (double v)
        {
            const float hz = StepFilterDSP::getFreeRateHz ((float) v);
            return juce::String (hz, hz < 1.0f ? 2 : 1) + " Hz";
        });
    }
    repaint();
}

void AKNStepFilterAudioProcessorEditor::paint (juce::Graphics& g)
{
    if (panelImage.isValid())
        g.drawImage (panelImage, getLocalBounds().toFloat());
    else
        g.fillAll (juce::Colour (0xff06312e));

    const float sx = getWidth() / (float) baseW;
    const float sy = getHeight() / (float) baseH;

    auto text = [&] (juce::String s, float x, float y, float size, juce::Colour c, juce::Justification just)
    {
        g.setColour (c);
        g.setFont (juce::FontOptions (size * sy));
        g.drawText (s, juce::Rectangle<float> (x * sx, (y - size) * sy, 400.0f * sx, size * 1.5f * sy), just, false);
    };

    text ("Akuen_Step_Filter", 48, 63, 30, ink, juce::Justification::centredLeft);
    text ("STEP FILTER", 48, 84, 11, muted, juce::Justification::centredLeft);
    text ("FILTER", 49, 140, 18, ink, juce::Justification::centredLeft);
    text ("MODULATION", 754, 140, 18, ink, juce::Justification::centredLeft);
    text ("OUTPUT", 1168, 140, 18, ink, juce::Justification::centredLeft);

    g.setColour (ink.withAlpha (0.5f));
    g.drawLine (135.0f * sx, 132.0f * sy, 683.0f * sx, 132.0f * sy, 1.0f);
    g.drawLine (905.0f * sx, 132.0f * sy, 1105.0f * sx, 132.0f * sy, 1.0f);
    g.drawLine (1250.0f * sx, 132.0f * sy, 1352.0f * sx, 132.0f * sy, 1.0f);

    drawFrequencyTicks (g, { 168.0f * sx, 270.0f * sy }, 88.0f * sx);
}

void AKNStepFilterAudioProcessorEditor::drawFrequencyTicks (juce::Graphics& g, juce::Point<float> centre, float radius) const
{
    static const char* labels[] { "70", "100", "150", "250", "500", "1k", "2k", "3k", "5k", "7.5k" };
    const float sx = getWidth() / (float) baseW;
    const float sy = getHeight() / (float) baseH;
    const float rr = radius;

    g.setColour (ink);
    g.setFont (juce::FontOptions (13.0f * sy));

    for (int i = 0; i < 10; ++i)
    {
        const float deg = -180.0f + (210.0f * (float) i / 9.0f);
        const float a = juce::degreesToRadians (deg);
        const auto dir = juce::Point<float> (std::cos (a), std::sin (a));
        const auto p1 = centre + dir * rr;
        const auto p2 = centre + dir * (rr + 8.0f * sx);
        g.drawLine ({ p1, p2 }, 2.0f * sx);
        const auto pt = centre + dir * (rr + 27.0f * sx);
        g.drawText (labels[i], juce::Rectangle<float> (pt.x - 24.0f * sx, pt.y - 8.0f * sy, 48.0f * sx, 18.0f * sy), juce::Justification::centred);
    }
}

void AKNStepFilterAudioProcessorEditor::resized()
{
    const float sx = getWidth() / (float) baseW;
    const float sy = getHeight() / (float) baseH;

    auto knobBounds = [&] (int cx, int cy, int r, bool large)
    {
        const int pad = 10;
        const int w = 2 * (r + pad);
        const int labelH = large ? 26 : 22;
        const int readoutH = large ? 22 : 18;
        const int h = w + labelH + readoutH;
        return scaledRect ({ cx - w / 2, cy - w / 2, w, h }, sx, sy);
    };

    frequency.setBounds (knobBounds (168, 270, 74, true));
    resonance.setBounds (knobBounds (355, 222, 48, false));
    drive.setBounds (knobBounds (480, 222, 48, false));
    morph.setBounds (knobBounds (605, 222, 48, false));
    filterType.setBounds (knobBounds (485, 365, 38, false));
    lfoAmount.setBounds (knobBounds (820, 220, 48, false));
    lfoRate.setBounds (knobBounds (1010, 220, 48, false));
    lfoWave.setBounds (knobBounds (870, 365, 38, false));
    lfoPhase.setBounds (knobBounds (1030, 365, 38, false));
    dryWet.setBounds (knobBounds (1210, 245, 52, false));
    volume.setBounds (knobBounds (1310, 245, 52, false));

    syncButton.setBounds (scaledRect ({ 754, 326, 112, 28 }, sx, sy));
}
